package main

import (
	"log"
	"net/http"
	"sync"
	"time"
)

// Live play: the host of a town and one guest exchange the game's netplay stream through
// the server, so nobody has to open ports. The relay never looks inside the stream: binary
// messages from the host go to the guest and back. Text messages from the relay tell the
// host when a guest joins or leaves ("guest-joined", "guest-left").

const livePingEvery = 20 * time.Second

type liveTown struct {
	host      *wsConn
	hostUser  string
	guest     *wsConn
	guestUser string
}

type Live struct {
	mu    sync.Mutex
	towns map[string]*liveTown
}

func newLive() *Live {
	return &Live{towns: map[string]*liveTown{}}
}

// Players counts who is in the town right now (host and guest).
func (l *Live) Players(townID string) int {
	l.mu.Lock()
	defer l.mu.Unlock()

	t := l.towns[townID]
	n := 0
	if t != nil && t.host != nil {
		n++
	}
	if t != nil && t.guest != nil {
		n++
	}
	return n
}

func keepAlive(c *wsConn, done <-chan struct{}) {
	t := time.NewTicker(livePingEvery)
	defer t.Stop()
	for {
		select {
		case <-done:
			return
		case <-t.C:
			if c.write(wsOpPing, nil) != nil {
				return
			}
		}
	}
}

func (a *API) live(w http.ResponseWriter, r *http.Request, user string) {
	id := r.PathValue("id")
	if _, err := a.store.Town(user, id); writeTownError(w, err) {
		return
	}

	switch r.URL.Query().Get("role") {
	case "host":
		if !a.locks.Holds(id, r.Header.Get(lockIDHd)) {
			writeError(w, http.StatusConflict, "you are not hosting this town")
			return
		}
		c, err := upgradeWS(w, r)
		if err != nil {
			writeError(w, http.StatusBadRequest, err.Error())
			return
		}
		a.liveHost(id, user, c)

	case "guest":
		a.relay.mu.Lock()
		t := a.relay.towns[id]
		var refuse string
		switch {
		case t == nil || t.host == nil:
			refuse = "nobody is playing this town"
		case t.hostUser == user:
			refuse = "you are already playing this town"
		case t.guest != nil:
			refuse = "someone else is already playing there"
		}
		a.relay.mu.Unlock()
		if refuse != "" {
			writeError(w, http.StatusConflict, refuse)
			return
		}
		c, err := upgradeWS(w, r)
		if err != nil {
			writeError(w, http.StatusBadRequest, err.Error())
			return
		}
		a.liveGuest(id, user, c)

	default:
		writeError(w, http.StatusBadRequest, "role must be host or guest")
	}
}

func (a *API) liveHost(id, user string, c *wsConn) {
	a.relay.mu.Lock()
	t := a.relay.towns[id]
	if t == nil {
		t = &liveTown{}
		a.relay.towns[id] = t
	}
	if t.host != nil {
		t.host.close() // an older session of this host
	}
	t.host, t.hostUser = c, user
	a.relay.mu.Unlock()
	log.Printf("live %s: %s hosting", id, user)

	done := make(chan struct{})
	go keepAlive(c, done)
	defer close(done)

	for {
		op, data, err := c.readMessage()
		if err != nil {
			break
		}
		if op != wsOpBinary {
			continue
		}
		a.relay.mu.Lock()
		guest := t.guest
		a.relay.mu.Unlock()
		if guest != nil {
			guest.write(wsOpBinary, data)
		}
	}

	a.relay.mu.Lock()
	if t.host == c {
		t.host = nil
		if t.guest != nil {
			t.guest.close() // the town goes away with its host
			t.guest = nil
		}
		delete(a.relay.towns, id)
	}
	a.relay.mu.Unlock()
	c.conn.Close()
	log.Printf("live %s: %s stopped hosting", id, user)
}

func (a *API) liveGuest(id, user string, c *wsConn) {
	a.relay.mu.Lock()
	t := a.relay.towns[id]
	if t == nil || t.host == nil || t.guest != nil {
		a.relay.mu.Unlock()
		c.close() // lost a race with another guest or the host leaving
		return
	}
	t.guest, t.guestUser = c, user
	host := t.host
	a.relay.mu.Unlock()
	host.write(wsOpText, []byte("guest-joined"))
	log.Printf("live %s: %s joined", id, user)

	done := make(chan struct{})
	go keepAlive(c, done)
	defer close(done)

	for {
		op, data, err := c.readMessage()
		if err != nil {
			break
		}
		if op != wsOpBinary {
			continue
		}
		a.relay.mu.Lock()
		host := t.host
		a.relay.mu.Unlock()
		if host != nil {
			host.write(wsOpBinary, data)
		}
	}

	a.relay.mu.Lock()
	if t.guest == c {
		t.guest = nil
		if t.host != nil {
			t.host.write(wsOpText, []byte("guest-left"))
		}
	}
	a.relay.mu.Unlock()
	c.conn.Close()
	log.Printf("live %s: %s left", id, user)
}
