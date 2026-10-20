package main

import (
	"crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
	"time"
)

// On-disk layout under the data dir:
//
//	users/<username>.json
//	towns/<id>/meta.json
//	towns/<id>/save.gci
//	towns/<id>/backups/save.<version>.gci   (last maxBackups versions)
//
// Everything is also indexed in memory; files are the source of truth on startup.

const (
	maxResidents = 4
	maxBackups   = 10
)

var (
	errNotFound   = errors.New("not found")
	errExists     = errors.New("already exists")
	errTownFull   = errors.New("town is full")
	errNotMember  = errors.New("not a resident of this town")
	errStaleSave  = errors.New("save version is out of date")
	errBadRequest = errors.New("bad request")
)

type User struct {
	Username string    `json:"username"`
	PassHash string    `json:"pass_hash"`
	Salt     string    `json:"salt"`
	Tokens   []string  `json:"tokens"` // sha256 of each session token
	Created  time.Time `json:"created"`
}

type Town struct {
	ID          string    `json:"id"`
	Name        string    `json:"name"`
	Code        string    `json:"code"`
	Owner       string    `json:"owner"`
	Members     []string  `json:"members"` // residents, owner first
	Created     time.Time `json:"created"`
	SaveVersion int       `json:"save_version"` // 0 = no save uploaded yet
	SaveUpdated time.Time `json:"save_updated,omitzero"`
	SaveBy      string    `json:"save_by,omitzero"`
}

func (t *Town) isMember(username string) bool {
	for _, m := range t.Members {
		if m == username {
			return true
		}
	}
	return false
}

type Store struct {
	dir string

	mu     sync.Mutex
	users  map[string]*User
	tokens map[string]string // token hash -> username
	towns  map[string]*Town
	codes  map[string]string // invite code -> town id
}

func OpenStore(dir string) (*Store, error) {
	s := &Store{
		dir:    dir,
		users:  map[string]*User{},
		tokens: map[string]string{},
		towns:  map[string]*Town{},
		codes:  map[string]string{},
	}
	for _, sub := range []string{"users", "towns"} {
		if err := os.MkdirAll(filepath.Join(dir, sub), 0o755); err != nil {
			return nil, err
		}
	}

	userFiles, err := filepath.Glob(filepath.Join(dir, "users", "*.json"))
	if err != nil {
		return nil, err
	}
	for _, f := range userFiles {
		var u User
		if err := readJSON(f, &u); err != nil {
			return nil, fmt.Errorf("%s: %w", f, err)
		}
		s.users[u.Username] = &u
		for _, t := range u.Tokens {
			s.tokens[t] = u.Username
		}
	}

	townFiles, err := filepath.Glob(filepath.Join(dir, "towns", "*", "meta.json"))
	if err != nil {
		return nil, err
	}
	for _, f := range townFiles {
		var t Town
		if err := readJSON(f, &t); err != nil {
			return nil, fmt.Errorf("%s: %w", f, err)
		}
		s.towns[t.ID] = &t
		s.codes[t.Code] = t.ID
	}
	return s, nil
}

// --- users ---

func (s *Store) CreateUser(username, password string) (string, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if _, ok := s.users[username]; ok {
		return "", errExists
	}
	salt := randomHex(16)
	u := &User{
		Username: username,
		PassHash: hashPassword(password, salt),
		Salt:     salt,
		Created:  time.Now().UTC(),
	}
	token := s.addTokenLocked(u)
	if err := s.saveUserLocked(u); err != nil {
		return "", err
	}
	s.users[username] = u
	return token, nil
}

func (s *Store) Login(username, password string) (string, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	u, ok := s.users[username]
	if !ok || !checkPassword(password, u.Salt, u.PassHash) {
		return "", errNotFound
	}
	token := s.addTokenLocked(u)
	if err := s.saveUserLocked(u); err != nil {
		return "", err
	}
	return token, nil
}

const maxTokensPerUser = 8

func (s *Store) addTokenLocked(u *User) string {
	token := randomHex(32)
	h := tokenHash(token)
	u.Tokens = append(u.Tokens, h)
	if len(u.Tokens) > maxTokensPerUser {
		for _, old := range u.Tokens[:len(u.Tokens)-maxTokensPerUser] {
			delete(s.tokens, old)
		}
		u.Tokens = u.Tokens[len(u.Tokens)-maxTokensPerUser:]
	}
	s.tokens[h] = u.Username
	return token
}

func (s *Store) UserForToken(token string) (string, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	name, ok := s.tokens[tokenHash(token)]
	return name, ok
}

func (s *Store) saveUserLocked(u *User) error {
	return writeJSON(filepath.Join(s.dir, "users", u.Username+".json"), u)
}

// --- towns ---

func (s *Store) CreateTown(owner, name string) (*Town, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	t := &Town{
		ID:      randomHex(8),
		Name:    name,
		Code:    s.newCodeLocked(),
		Owner:   owner,
		Members: []string{owner},
		Created: time.Now().UTC(),
	}
	if err := os.MkdirAll(filepath.Join(s.townDir(t.ID), "backups"), 0o755); err != nil {
		return nil, err
	}
	if err := s.saveTownLocked(t); err != nil {
		return nil, err
	}
	s.towns[t.ID] = t
	s.codes[t.Code] = t.ID
	return copyTown(t), nil
}

// JoinTown makes the user a resident of the town with that invite code.
func (s *Store) JoinTown(username, code string) (*Town, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	id, ok := s.codes[normalizeCode(code)]
	if !ok {
		return nil, errNotFound
	}
	t := s.towns[id]
	if t.isMember(username) {
		return copyTown(t), nil
	}
	if len(t.Members) >= maxResidents {
		return nil, errTownFull
	}
	t.Members = append(t.Members, username)
	if err := s.saveTownLocked(t); err != nil {
		t.Members = t.Members[:len(t.Members)-1]
		return nil, err
	}
	return copyTown(t), nil
}

func (s *Store) TownsFor(username string) []*Town {
	s.mu.Lock()
	defer s.mu.Unlock()

	var out []*Town
	for _, t := range s.towns {
		if t.isMember(username) {
			out = append(out, copyTown(t))
		}
	}
	sort.Slice(out, func(i, j int) bool { return out[i].Created.Before(out[j].Created) })
	return out
}

// Town returns the town if the user lives there.
func (s *Store) Town(username, id string) (*Town, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	t, ok := s.towns[id]
	if !ok {
		return nil, errNotFound
	}
	if !t.isMember(username) {
		return nil, errNotMember
	}
	return copyTown(t), nil
}

func (s *Store) ReadSave(username, id string) ([]byte, int, error) {
	t, err := s.Town(username, id)
	if err != nil {
		return nil, 0, err
	}
	if t.SaveVersion == 0 {
		return nil, 0, errNotFound
	}
	data, err := os.ReadFile(filepath.Join(s.townDir(id), "save.gci"))
	return data, t.SaveVersion, err
}

// WriteSave stores a new save. baseVersion must be the version the client started from,
// so a stale game can never overwrite newer progress.
func (s *Store) WriteSave(username, id string, baseVersion int, data []byte) (int, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	t, ok := s.towns[id]
	if !ok {
		return 0, errNotFound
	}
	if !t.isMember(username) {
		return 0, errNotMember
	}
	if baseVersion != t.SaveVersion {
		return 0, errStaleSave
	}

	dir := s.townDir(id)
	next := t.SaveVersion + 1
	if err := writeFileAtomic(filepath.Join(dir, "save.gci"), data); err != nil {
		return 0, err
	}
	if err := writeFileAtomic(filepath.Join(dir, "backups", fmt.Sprintf("save.%d.gci", next)), data); err != nil {
		return 0, err
	}
	pruneBackups(filepath.Join(dir, "backups"), next)

	t.SaveVersion = next
	t.SaveUpdated = time.Now().UTC()
	t.SaveBy = username
	if err := s.saveTownLocked(t); err != nil {
		return 0, err
	}
	return next, nil
}

func (s *Store) townDir(id string) string {
	return filepath.Join(s.dir, "towns", id)
}

func (s *Store) saveTownLocked(t *Town) error {
	return writeJSON(filepath.Join(s.townDir(t.ID), "meta.json"), t)
}

// Invite codes look like "HOJA-7K2M"; the alphabet skips look-alike characters.
const codeAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"

func (s *Store) newCodeLocked() string {
	for {
		b := make([]byte, 8)
		if _, err := rand.Read(b); err != nil {
			panic(err)
		}
		for i := range b {
			b[i] = codeAlphabet[int(b[i])%len(codeAlphabet)]
		}
		code := string(b[:4]) + "-" + string(b[4:])
		if _, taken := s.codes[code]; !taken {
			return code
		}
	}
}

func normalizeCode(code string) string {
	code = strings.ToUpper(strings.TrimSpace(code))
	code = strings.ReplaceAll(code, " ", "")
	if len(code) == 8 && !strings.Contains(code, "-") {
		code = code[:4] + "-" + code[4:]
	}
	return code
}

func copyTown(t *Town) *Town {
	c := *t
	c.Members = append([]string(nil), t.Members...)
	return &c
}

func pruneBackups(dir string, latest int) {
	for v := latest - maxBackups; v > 0; v-- {
		p := filepath.Join(dir, fmt.Sprintf("save.%d.gci", v))
		if err := os.Remove(p); err != nil {
			break // older ones were pruned on earlier writes
		}
	}
}

// --- files ---

func readJSON(path string, v any) error {
	data, err := os.ReadFile(path)
	if err != nil {
		return err
	}
	return json.Unmarshal(data, v)
}

func writeJSON(path string, v any) error {
	data, err := json.MarshalIndent(v, "", "  ")
	if err != nil {
		return err
	}
	return writeFileAtomic(path, data)
}

func writeFileAtomic(path string, data []byte) error {
	tmp := path + ".tmp"
	if err := os.WriteFile(tmp, data, 0o644); err != nil {
		return err
	}
	return os.Rename(tmp, path)
}

func randomHex(n int) string {
	b := make([]byte, n)
	if _, err := rand.Read(b); err != nil {
		panic(err)
	}
	return hex.EncodeToString(b)
}

func tokenHash(token string) string {
	h := sha256.Sum256([]byte(token))
	return hex.EncodeToString(h[:])
}
