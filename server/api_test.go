package main

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"strconv"
	"testing"
	"time"
)

type client struct {
	t     *testing.T
	base  string
	token string
}

func (c *client) do(method, path string, body io.Reader, hdr map[string]string) *http.Response {
	c.t.Helper()
	req, err := http.NewRequest(method, c.base+path, body)
	if err != nil {
		c.t.Fatal(err)
	}
	if c.token != "" {
		req.Header.Set("Authorization", "Bearer "+c.token)
	}
	for k, v := range hdr {
		req.Header.Set(k, v)
	}
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		c.t.Fatal(err)
	}
	return resp
}

func (c *client) json(method, path string, in, out any, wantStatus int) {
	c.t.Helper()
	var body io.Reader
	if in != nil {
		b, _ := json.Marshal(in)
		body = bytes.NewReader(b)
	}
	resp := c.do(method, path, body, nil)
	defer resp.Body.Close()
	data, _ := io.ReadAll(resp.Body)
	if resp.StatusCode != wantStatus {
		c.t.Fatalf("%s %s: status %d, want %d: %s", method, path, resp.StatusCode, wantStatus, data)
	}
	if out != nil {
		if err := json.Unmarshal(data, out); err != nil {
			c.t.Fatalf("%s %s: %v: %s", method, path, err, data)
		}
	}
}

func newTestServer(t *testing.T, dir string) *httptest.Server {
	t.Helper()
	store, err := OpenStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	srv := httptest.NewServer(NewAPI(store).Handler())
	t.Cleanup(srv.Close)
	return srv
}

func registerUser(t *testing.T, base, name string) *client {
	t.Helper()
	c := &client{t: t, base: base}
	var tok tokenResponse
	c.json("POST", "/api/v1/register", credentials{name, "secret123"}, &tok, http.StatusCreated)
	c.token = tok.Token
	return c
}

func fakeSave(fill byte) []byte {
	data := bytes.Repeat([]byte{fill}, gciSize)
	copy(data, "GAFE01")
	return data
}

func TestTownLifecycle(t *testing.T) {
	dir := t.TempDir()
	srv := newTestServer(t, dir)

	ana := registerUser(t, srv.URL, "ana")
	beto := registerUser(t, srv.URL, "beto")

	// Duplicate names and bad credentials are rejected.
	anon := &client{t: t, base: srv.URL}
	anon.json("POST", "/api/v1/register", credentials{"ana", "secret123"}, nil, http.StatusConflict)
	anon.json("POST", "/api/v1/login", credentials{"ana", "wrongpass"}, nil, http.StatusUnauthorized)
	anon.json("GET", "/api/v1/towns", nil, nil, http.StatusUnauthorized)

	var town townResponse
	ana.json("POST", "/api/v1/towns", map[string]string{"name": "Hojita"}, &town, http.StatusCreated)
	if town.FreeSlots != 3 || town.Code == "" || town.Host != nil {
		t.Fatalf("new town: %+v", town)
	}

	// Beto can't see the town until he joins with the code (lowercase, no dash works too).
	beto.json("GET", "/api/v1/towns/"+town.ID, nil, nil, http.StatusForbidden)
	var joined townResponse
	code := town.Code[:4] + town.Code[5:]
	beto.json("POST", "/api/v1/towns/join", map[string]string{"code": string(bytes.ToLower([]byte(code)))}, &joined, http.StatusOK)
	if joined.ID != town.ID || len(joined.Members) != 2 {
		t.Fatalf("join: %+v", joined)
	}

	// No save yet.
	resp := ana.do("GET", "/api/v1/towns/"+town.ID+"/save", nil, nil)
	resp.Body.Close()
	if resp.StatusCode != http.StatusNotFound {
		t.Fatalf("empty save: %d", resp.StatusCode)
	}

	// Ana hosts; Beto sees her as the host and can't take the lock.
	var lock HostLock
	ana.json("POST", "/api/v1/towns/"+town.ID+"/lock", nil, &lock, http.StatusOK)
	var other HostLock
	beto.json("POST", "/api/v1/towns/"+town.ID+"/lock", nil, &other, http.StatusConflict)
	if other.Host != "ana" || other.LockID != "" {
		t.Fatalf("lock seen by beto: %+v", other)
	}

	// Uploading needs the lock and the base version.
	put := func(c *client, lockID string, base int, data []byte) int {
		r := c.do("PUT", "/api/v1/towns/"+town.ID+"/save", bytes.NewReader(data),
			map[string]string{lockIDHd: lockID, saveVersionHd: strconv.Itoa(base)})
		r.Body.Close()
		return r.StatusCode
	}
	if s := put(beto, "nope", 0, fakeSave(1)); s != http.StatusConflict {
		t.Fatalf("put without lock: %d", s)
	}
	if s := put(ana, lock.LockID, 0, []byte("not a save")); s != http.StatusBadRequest {
		t.Fatalf("put garbage: %d", s)
	}
	if s := put(ana, lock.LockID, 0, fakeSave(1)); s != http.StatusOK {
		t.Fatalf("put v1: %d", s)
	}
	if s := put(ana, lock.LockID, 0, fakeSave(2)); s != http.StatusConflict {
		t.Fatalf("stale put: %d", s)
	}
	if s := put(ana, lock.LockID, 1, fakeSave(2)); s != http.StatusOK {
		t.Fatalf("put v2: %d", s)
	}

	// Any resident can download the latest save.
	resp = beto.do("GET", "/api/v1/towns/"+town.ID+"/save", nil, nil)
	data, _ := io.ReadAll(resp.Body)
	resp.Body.Close()
	if resp.StatusCode != http.StatusOK || resp.Header.Get(saveVersionHd) != "2" || !bytes.Equal(data, fakeSave(2)) {
		t.Fatalf("get save: %d v=%s len=%d", resp.StatusCode, resp.Header.Get(saveVersionHd), len(data))
	}

	// Releasing frees the town for Beto.
	resp = ana.do("DELETE", "/api/v1/towns/"+town.ID+"/lock", nil, map[string]string{lockIDHd: lock.LockID})
	resp.Body.Close()
	beto.json("POST", "/api/v1/towns/"+town.ID+"/lock", nil, &other, http.StatusOK)

	// Everything survives a restart.
	srv2 := newTestServer(t, dir)
	ana2 := &client{t: t, base: srv2.URL}
	var tok tokenResponse
	ana2.json("POST", "/api/v1/login", credentials{"ana", "secret123"}, &tok, http.StatusOK)
	ana2.token = tok.Token
	var towns []townResponse
	ana2.json("GET", "/api/v1/towns", nil, &towns, http.StatusOK)
	if len(towns) != 1 || towns[0].SaveVersion != 2 || towns[0].SaveBy != "ana" || len(towns[0].Members) != 2 {
		t.Fatalf("after restart: %+v", towns)
	}
	// The old session token still works too.
	ana.base = srv2.URL
	ana.json("GET", "/api/v1/towns/"+town.ID, nil, nil, http.StatusOK)
}

func TestTownFull(t *testing.T) {
	srv := newTestServer(t, t.TempDir())
	owner := registerUser(t, srv.URL, "owner")
	var town townResponse
	owner.json("POST", "/api/v1/towns", map[string]string{"name": "Llena"}, &town, http.StatusCreated)

	for _, name := range []string{"pepe", "lola", "toto"} {
		registerUser(t, srv.URL, name).json("POST", "/api/v1/towns/join", map[string]string{"code": town.Code}, nil, http.StatusOK)
	}
	registerUser(t, srv.URL, "memo").json("POST", "/api/v1/towns/join", map[string]string{"code": town.Code}, nil, http.StatusConflict)
	registerUser(t, srv.URL, "nico").json("POST", "/api/v1/towns/join", map[string]string{"code": "ZZZZ-ZZZZ"}, nil, http.StatusNotFound)
}

func TestLockExpires(t *testing.T) {
	l := newLocks()
	now := time.Unix(1000, 0)
	l.now = func() time.Time { return now }

	lk, ok := l.Acquire("town", "ana")
	if !ok {
		t.Fatal("first acquire failed")
	}
	if _, ok := l.Acquire("town", "beto"); ok {
		t.Fatal("beto took a held lock")
	}
	if again, ok := l.Acquire("town", "ana"); !ok || again.LockID != lk.LockID {
		t.Fatal("re-acquire by the host should keep the same lock")
	}

	now = now.Add(lockTTL / 2)
	if _, ok := l.Renew("town", lk.LockID); !ok {
		t.Fatal("renew failed")
	}
	now = now.Add(lockTTL - time.Second)
	if !l.Holds("town", lk.LockID) {
		t.Fatal("renewed lock expired early")
	}

	// The host's game died: after the TTL anyone can host.
	now = now.Add(2 * time.Second)
	if _, ok := l.Acquire("town", "beto"); !ok {
		t.Fatal("expired lock not released")
	}
	if l.Holds("town", lk.LockID) {
		t.Fatal("old lock still valid")
	}
}

func TestBackupsArePruned(t *testing.T) {
	dir := t.TempDir()
	s, err := OpenStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	s.CreateUser("ana", "secret123")
	town, _ := s.CreateTown("ana", "Hojita")
	for v := 0; v < maxBackups+5; v++ {
		if _, err := s.WriteSave("ana", town.ID, v, fakeSave(byte(v))); err != nil {
			t.Fatal(err)
		}
	}
	backups, _ := filepathGlob(dir, town.ID)
	if len(backups) != maxBackups {
		t.Fatalf("%d backups kept, want %d", len(backups), maxBackups)
	}
}

func filepathGlob(dir, townID string) ([]string, error) {
	return filepath.Glob(filepath.Join(dir, "towns", townID, "backups", "save.*.gci"))
}

func TestWorldMap(t *testing.T) {
	srv := newTestServer(t, t.TempDir())
	ana := registerUser(t, srv.URL, "ana")
	beto := registerUser(t, srv.URL, "beto")

	var town townResponse
	ana.json("POST", "/api/v1/towns", map[string]string{"name": "Hojita"}, &town, http.StatusCreated)

	// Private by default: not on the map, can't move in without the ticket.
	var world []worldTown
	beto.json("GET", "/api/v1/world", nil, &world, http.StatusOK)
	if len(world) != 0 {
		t.Fatalf("private town on the map: %+v", world)
	}
	beto.json("POST", "/api/v1/towns/"+town.ID+"/move-in", nil, nil, http.StatusNotFound)

	// Only the owner publishes.
	beto.json("POST", "/api/v1/towns/"+town.ID+"/public", map[string]bool{"public": true}, nil, http.StatusForbidden)
	ana.json("POST", "/api/v1/towns/"+town.ID+"/public", map[string]bool{"public": true}, nil, http.StatusOK)

	beto.json("GET", "/api/v1/world", nil, &world, http.StatusOK)
	if len(world) != 1 || world[0].Name != "Hojita" || world[0].Mine || world[0].FreeSlots != 3 {
		t.Fatalf("world: %+v", world)
	}
	x, y := world[0].X, world[0].Y
	if x <= 0 || x >= 1 || y <= 0 || y >= 1 {
		t.Fatalf("map spot out of range: %v %v", x, y)
	}

	// Strangers see whether someone is playing, never who.
	ana.json("POST", "/api/v1/towns/"+town.ID+"/lock", nil, nil, http.StatusOK)
	var raw []map[string]any
	beto.json("GET", "/api/v1/world", nil, &raw, http.StatusOK)
	if raw[0]["playing"] != true || raw[0]["members"] != nil || raw[0]["code"] != nil {
		t.Fatalf("world leaks or misses data: %+v", raw[0])
	}

	beto.json("POST", "/api/v1/towns/"+town.ID+"/move-in", nil, nil, http.StatusOK)
	beto.json("GET", "/api/v1/world", nil, &world, http.StatusOK)
	if !world[0].Mine || world[0].Residents != 2 || world[0].X != x {
		t.Fatalf("after moving in: %+v", world)
	}

	// Unlisting hides it again.
	ana.json("POST", "/api/v1/towns/"+town.ID+"/public", map[string]bool{"public": false}, nil, http.StatusOK)
	beto.json("GET", "/api/v1/world", nil, &world, http.StatusOK)
	if len(world) != 0 {
		t.Fatalf("unlisted town still on the map")
	}
}
