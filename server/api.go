package main

import (
	"bufio"
	"bytes"
	"crypto/sha256"
	"encoding/binary"
	"encoding/json"
	"errors"
	"io"
	"log"
	"net"
	"net/http"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"
)

// A GameCube GCI file of an Animal Crossing (GAFE) town: 64-byte header + 0x72000 bytes.
const (
	gciSize       = 64 + 0x72000
	maxTownName   = 16
	minPassword   = 6
	maxPassword   = 128
	maxJSONBody   = 4 << 10
	saveVersionHd = "X-Save-Version"
	lockIDHd      = "X-Lock-Id"
)

var usernameRe = regexp.MustCompile(`^[a-z0-9_]{3,20}$`)

type API struct {
	store *Store
	locks *Locks
	relay *Live
	authL *rateLimiter
}

func NewAPI(store *Store) *API {
	return &API{store: store, locks: newLocks(), relay: newLive(), authL: newRateLimiter(10, time.Minute)}
}

func (a *API) Handler() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /healthz", func(w http.ResponseWriter, r *http.Request) { io.WriteString(w, "ok\n") })

	mux.HandleFunc("POST /api/v1/register", a.register)
	mux.HandleFunc("POST /api/v1/login", a.login)

	mux.HandleFunc("GET /api/v1/towns", a.auth(a.listTowns))
	mux.HandleFunc("POST /api/v1/towns", a.auth(a.createTown))
	mux.HandleFunc("POST /api/v1/towns/join", a.auth(a.joinTown))
	mux.HandleFunc("GET /api/v1/towns/{id}", a.auth(a.getTown))
	mux.HandleFunc("GET /api/v1/towns/{id}/save", a.auth(a.getSave))
	mux.HandleFunc("PUT /api/v1/towns/{id}/save", a.auth(a.putSave))
	mux.HandleFunc("POST /api/v1/towns/{id}/lock", a.auth(a.acquireLock))
	mux.HandleFunc("POST /api/v1/towns/{id}/lock/renew", a.auth(a.renewLock))
	mux.HandleFunc("DELETE /api/v1/towns/{id}/lock", a.auth(a.releaseLock))

	mux.HandleFunc("GET /api/v1/world", a.auth(a.world))
	mux.HandleFunc("POST /api/v1/towns/{id}/public", a.auth(a.setPublic))
	mux.HandleFunc("POST /api/v1/towns/{id}/move-in", a.auth(a.moveIn))
	mux.HandleFunc("GET /api/v1/towns/{id}/live", a.auth(a.live))
	return logRequests(mux)
}

type authedHandler func(w http.ResponseWriter, r *http.Request, user string)

func (a *API) auth(h authedHandler) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		user, ok := a.store.UserForToken(bearerToken(r))
		if !ok {
			writeError(w, http.StatusUnauthorized, "login required")
			return
		}
		h(w, r, user)
	}
}

// --- accounts ---

type credentials struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

type tokenResponse struct {
	Username string `json:"username"`
	Token    string `json:"token"`
}

func (a *API) readCredentials(w http.ResponseWriter, r *http.Request) (credentials, bool) {
	var c credentials
	if !a.authL.allow(clientIP(r)) {
		writeError(w, http.StatusTooManyRequests, "too many attempts, wait a minute")
		return c, false
	}
	if !readJSONBody(w, r, &c) {
		return c, false
	}
	c.Username = strings.ToLower(strings.TrimSpace(c.Username))
	if !usernameRe.MatchString(c.Username) {
		writeError(w, http.StatusBadRequest, "username: 3-20 characters, a-z 0-9 _")
		return c, false
	}
	if len(c.Password) < minPassword || len(c.Password) > maxPassword {
		writeError(w, http.StatusBadRequest, "password: at least 6 characters")
		return c, false
	}
	return c, true
}

func (a *API) register(w http.ResponseWriter, r *http.Request) {
	c, ok := a.readCredentials(w, r)
	if !ok {
		return
	}
	token, err := a.store.CreateUser(c.Username, c.Password)
	if errors.Is(err, errExists) {
		writeError(w, http.StatusConflict, "username taken")
		return
	}
	if err != nil {
		internalError(w, err)
		return
	}
	writeJSONResponse(w, http.StatusCreated, tokenResponse{c.Username, token})
}

func (a *API) login(w http.ResponseWriter, r *http.Request) {
	c, ok := a.readCredentials(w, r)
	if !ok {
		return
	}
	token, err := a.store.Login(c.Username, c.Password)
	if errors.Is(err, errNotFound) {
		writeError(w, http.StatusUnauthorized, "wrong username or password")
		return
	}
	if err != nil {
		internalError(w, err)
		return
	}
	writeJSONResponse(w, http.StatusOK, tokenResponse{c.Username, token})
}

// --- towns ---

type townResponse struct {
	*Town
	FreeSlots int       `json:"free_slots"`
	Host      *HostLock `json:"host"` // who is playing it right now, null if nobody
}

func (a *API) townResponse(t *Town) townResponse {
	return townResponse{Town: t, FreeSlots: maxResidents - len(t.Members), Host: a.locks.Host(t.ID)}
}

func (a *API) listTowns(w http.ResponseWriter, r *http.Request, user string) {
	towns := a.store.TownsFor(user)
	out := make([]townResponse, 0, len(towns))
	for _, t := range towns {
		out = append(out, a.townResponse(t))
	}
	writeJSONResponse(w, http.StatusOK, out)
}

func (a *API) createTown(w http.ResponseWriter, r *http.Request, user string) {
	var body struct {
		Name string `json:"name"`
	}
	if !readJSONBody(w, r, &body) {
		return
	}
	body.Name = strings.TrimSpace(body.Name)
	if body.Name == "" || utf8.RuneCountInString(body.Name) > maxTownName {
		writeError(w, http.StatusBadRequest, "town name: 1-16 characters")
		return
	}
	t, err := a.store.CreateTown(user, body.Name)
	if err != nil {
		internalError(w, err)
		return
	}
	writeJSONResponse(w, http.StatusCreated, a.townResponse(t))
}

func (a *API) joinTown(w http.ResponseWriter, r *http.Request, user string) {
	var body struct {
		Code string `json:"code"`
	}
	if !readJSONBody(w, r, &body) {
		return
	}
	t, err := a.store.JoinTown(user, body.Code)
	switch {
	case errors.Is(err, errNotFound):
		writeError(w, http.StatusNotFound, "no town with that code")
	case errors.Is(err, errTownFull):
		writeError(w, http.StatusConflict, "this town already has 4 residents")
	case err != nil:
		internalError(w, err)
	default:
		writeJSONResponse(w, http.StatusOK, a.townResponse(t))
	}
}

func (a *API) getTown(w http.ResponseWriter, r *http.Request, user string) {
	t, err := a.store.Town(user, r.PathValue("id"))
	if writeTownError(w, err) {
		return
	}
	writeJSONResponse(w, http.StatusOK, a.townResponse(t))
}

// --- world map ---

const maxWorldTowns = 100

// worldTown is what strangers see of a town: no member names, no invite code.
type worldTown struct {
	ID        string  `json:"id"`
	Name      string  `json:"name"`
	Residents int     `json:"residents"`
	FreeSlots int     `json:"free_slots"`
	Playing   bool    `json:"playing"` // someone is in it right now
	Players   int     `json:"players"` // how many are playing it live (host and guest)
	Mine      bool    `json:"mine"`    // you live there
	X         float64 `json:"x"`       // stable spot on the map, 0..1
	Y         float64 `json:"y"`
}

// mapSpot places a town on the world map from its id, so it never moves.
func mapSpot(id string) (float64, float64) {
	h := sha256.Sum256([]byte(id))
	x := float64(binary.BigEndian.Uint32(h[0:4])) / float64(^uint32(0))
	y := float64(binary.BigEndian.Uint32(h[4:8])) / float64(^uint32(0))
	return 0.08 + 0.84*x, 0.12 + 0.76*y // keep clear of the edges
}

func (a *API) world(w http.ResponseWriter, r *http.Request, user string) {
	towns := a.store.PublicTowns()
	if len(towns) > maxWorldTowns {
		towns = towns[len(towns)-maxWorldTowns:] // the newest ones
	}
	out := make([]worldTown, 0, len(towns))
	for _, t := range towns {
		x, y := mapSpot(t.ID)
		out = append(out, worldTown{
			ID: t.ID, Name: t.Name, Residents: len(t.Members), FreeSlots: maxResidents - len(t.Members),
			Playing: a.locks.Host(t.ID) != nil, Players: a.relay.Players(t.ID), Mine: t.isMember(user), X: x, Y: y,
		})
	}
	writeJSONResponse(w, http.StatusOK, out)
}

func (a *API) setPublic(w http.ResponseWriter, r *http.Request, user string) {
	var body struct {
		Public bool `json:"public"`
	}
	if !readJSONBody(w, r, &body) {
		return
	}
	t, err := a.store.SetPublic(user, r.PathValue("id"), body.Public)
	if errors.Is(err, errNotOwner) {
		writeError(w, http.StatusForbidden, err.Error())
		return
	}
	if writeTownError(w, err) {
		return
	}
	writeJSONResponse(w, http.StatusOK, a.townResponse(t))
}

func (a *API) moveIn(w http.ResponseWriter, r *http.Request, user string) {
	t, err := a.store.MoveIn(user, r.PathValue("id"))
	switch {
	case errors.Is(err, errNotFound):
		writeError(w, http.StatusNotFound, "that town is not on the map")
	case errors.Is(err, errTownFull):
		writeError(w, http.StatusConflict, "this town already has 4 residents")
	case err != nil:
		internalError(w, err)
	default:
		writeJSONResponse(w, http.StatusOK, a.townResponse(t))
	}
}

// --- saves ---

func (a *API) getSave(w http.ResponseWriter, r *http.Request, user string) {
	data, version, err := a.store.ReadSave(user, r.PathValue("id"))
	if errors.Is(err, errNotFound) {
		writeError(w, http.StatusNotFound, "this town has no save yet")
		return
	}
	if writeTownError(w, err) {
		return
	}
	w.Header().Set("Content-Type", "application/octet-stream")
	w.Header().Set(saveVersionHd, strconv.Itoa(version))
	w.Header().Set("Content-Length", strconv.Itoa(len(data)))
	w.Write(data)
}

// putSave requires holding the town's lock and the save version it was based on.
func (a *API) putSave(w http.ResponseWriter, r *http.Request, user string) {
	id := r.PathValue("id")
	// Read the save before any refusal: answering early would cut the upload off and the
	// game would only see a dropped connection instead of the reason.
	data, readErr := io.ReadAll(http.MaxBytesReader(w, r.Body, gciSize+1))

	if _, err := a.store.Town(user, id); writeTownError(w, err) {
		return
	}
	if !a.locks.Holds(id, r.Header.Get(lockIDHd)) {
		writeError(w, http.StatusConflict, "you are not hosting this town")
		return
	}
	base, err := strconv.Atoi(r.Header.Get(saveVersionHd))
	if err != nil {
		writeError(w, http.StatusBadRequest, "missing "+saveVersionHd)
		return
	}
	if readErr != nil || len(data) != gciSize || !bytes.HasPrefix(data, []byte("GAF")) {
		writeError(w, http.StatusBadRequest, "not an Animal Crossing town save")
		return
	}

	version, err := a.store.WriteSave(user, id, base, data)
	if errors.Is(err, errStaleSave) {
		writeError(w, http.StatusConflict, "the town changed since you loaded it")
		return
	}
	if writeTownError(w, err) {
		return
	}
	w.Header().Set(saveVersionHd, strconv.Itoa(version))
	writeJSONResponse(w, http.StatusOK, map[string]int{"save_version": version})
}

// --- host lock ---

func (a *API) acquireLock(w http.ResponseWriter, r *http.Request, user string) {
	id := r.PathValue("id")
	if _, err := a.store.Town(user, id); writeTownError(w, err) {
		return
	}
	lk, acquired := a.locks.Acquire(id, user)
	if !acquired {
		// Someone else is hosting: the game joins their session instead.
		writeJSONResponse(w, http.StatusConflict, lk)
		return
	}
	writeJSONResponse(w, http.StatusOK, lk)
}

func (a *API) renewLock(w http.ResponseWriter, r *http.Request, user string) {
	lk, ok := a.locks.Renew(r.PathValue("id"), r.Header.Get(lockIDHd))
	if !ok {
		writeError(w, http.StatusConflict, "lock expired")
		return
	}
	writeJSONResponse(w, http.StatusOK, lk)
}

func (a *API) releaseLock(w http.ResponseWriter, r *http.Request, user string) {
	a.locks.Release(r.PathValue("id"), r.Header.Get(lockIDHd))
	w.WriteHeader(http.StatusNoContent)
}

// --- helpers ---

func writeTownError(w http.ResponseWriter, err error) bool {
	switch {
	case err == nil:
		return false
	case errors.Is(err, errNotFound):
		writeError(w, http.StatusNotFound, "town not found")
	case errors.Is(err, errNotMember):
		writeError(w, http.StatusForbidden, "you don't live in this town")
	default:
		internalError(w, err)
	}
	return true
}

func readJSONBody(w http.ResponseWriter, r *http.Request, v any) bool {
	dec := json.NewDecoder(http.MaxBytesReader(w, r.Body, maxJSONBody))
	if err := dec.Decode(v); err != nil {
		writeError(w, http.StatusBadRequest, "invalid JSON body")
		return false
	}
	return true
}

func writeJSONResponse(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(v)
}

func writeError(w http.ResponseWriter, status int, msg string) {
	writeJSONResponse(w, status, map[string]string{"error": msg})
}

func internalError(w http.ResponseWriter, err error) {
	log.Printf("internal error: %v", err)
	writeError(w, http.StatusInternalServerError, "server error")
}

type statusRecorder struct {
	http.ResponseWriter
	status int
}

func (s *statusRecorder) WriteHeader(code int) {
	s.status = code
	s.ResponseWriter.WriteHeader(code)
}

// Hijack lets the live relay take over the connection through the request logger.
func (s *statusRecorder) Hijack() (net.Conn, *bufio.ReadWriter, error) {
	hj, ok := s.ResponseWriter.(http.Hijacker)
	if !ok {
		return nil, nil, errors.New("hijacking not supported")
	}
	s.status = http.StatusSwitchingProtocols
	return hj.Hijack()
}

func logRequests(h http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		rec := &statusRecorder{ResponseWriter: w, status: http.StatusOK}
		h.ServeHTTP(rec, r)
		if r.URL.Path != "/healthz" {
			log.Printf("%s %s %d %s", r.Method, r.URL.Path, rec.status, time.Since(start).Round(time.Millisecond))
		}
	})
}
