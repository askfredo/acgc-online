package main

import (
	"bufio"
	"crypto/rand"
	"encoding/base64"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"net/http"
	"strings"
	"testing"
	"time"
)

// testWS is a tiny WebSocket client for the tests (masked frames, like a real client).
type testWS struct {
	conn net.Conn
	r    *bufio.Reader
}

func dialWS(t *testing.T, base, path, token string, hdr map[string]string) (*testWS, int) {
	t.Helper()
	conn, err := net.Dial("tcp", strings.TrimPrefix(base, "http://"))
	if err != nil {
		t.Fatal(err)
	}
	key := make([]byte, 16)
	rand.Read(key)
	req := fmt.Sprintf("GET %s HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"+
		"Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: %s\r\nAuthorization: Bearer %s\r\n",
		path, base64.StdEncoding.EncodeToString(key), token)
	for k, v := range hdr {
		req += k + ": " + v + "\r\n"
	}
	conn.Write([]byte(req + "\r\n"))
	r := bufio.NewReader(conn)
	resp, err := http.ReadResponse(r, nil)
	if err != nil {
		t.Fatal(err)
	}
	if resp.StatusCode != http.StatusSwitchingProtocols {
		conn.Close()
		return nil, resp.StatusCode
	}
	return &testWS{conn: conn, r: r}, resp.StatusCode
}

func (c *testWS) send(op byte, data []byte) {
	hdr := []byte{0x80 | op, 0}
	switch {
	case len(data) < 126:
		hdr[1] = 0x80 | byte(len(data))
	case len(data) <= 0xFFFF:
		hdr[1] = 0x80 | 126
		hdr = binary.BigEndian.AppendUint16(hdr, uint16(len(data)))
	default:
		hdr[1] = 0x80 | 127
		hdr = binary.BigEndian.AppendUint64(hdr, uint64(len(data)))
	}
	mask := []byte{1, 2, 3, 4}
	masked := make([]byte, len(data))
	for i := range data {
		masked[i] = data[i] ^ mask[i%4]
	}
	c.conn.Write(append(append(hdr, mask...), masked...))
}

func (c *testWS) recv(t *testing.T) (byte, []byte) {
	t.Helper()
	c.conn.SetReadDeadline(time.Now().Add(3 * time.Second))
	for {
		var h [2]byte
		if _, err := io.ReadFull(c.r, h[:]); err != nil {
			t.Fatalf("recv: %v", err)
		}
		n := uint64(h[1] & 0x7F)
		if n == 126 {
			var e [2]byte
			io.ReadFull(c.r, e[:])
			n = uint64(binary.BigEndian.Uint16(e[:]))
		} else if n == 127 {
			var e [8]byte
			io.ReadFull(c.r, e[:])
			n = binary.BigEndian.Uint64(e[:])
		}
		data := make([]byte, n)
		io.ReadFull(c.r, data)
		if op := h[0] & 0x0F; op != wsOpPing {
			return op, data
		}
	}
}

func TestLiveRelay(t *testing.T) {
	srv := newTestServer(t, t.TempDir())
	ana := registerUser(t, srv.URL, "ana")
	beto := registerUser(t, srv.URL, "beto")
	caro := registerUser(t, srv.URL, "caro")

	var town townResponse
	ana.json("POST", "/api/v1/towns", map[string]string{"name": "Hojita"}, &town, http.StatusCreated)
	for _, c := range []*client{beto, caro} {
		c.json("POST", "/api/v1/towns/join", map[string]string{"code": town.Code}, nil, http.StatusOK)
	}
	live := "/api/v1/towns/" + town.ID + "/live"

	// No host yet: guests are turned away, and hosting needs the lock.
	if _, st := dialWS(t, srv.URL, live+"?role=guest", beto.token, nil); st != http.StatusConflict {
		t.Fatalf("guest without host: %d", st)
	}
	if _, st := dialWS(t, srv.URL, live+"?role=host", ana.token, nil); st != http.StatusConflict {
		t.Fatalf("host without lock: %d", st)
	}

	var lock HostLock
	ana.json("POST", "/api/v1/towns/"+town.ID+"/lock", nil, &lock, http.StatusOK)
	host, st := dialWS(t, srv.URL, live+"?role=host", ana.token, map[string]string{lockIDHd: lock.LockID})
	if host == nil {
		t.Fatalf("host connect: %d", st)
	}
	guest, st := dialWS(t, srv.URL, live+"?role=guest", beto.token, nil)
	if guest == nil {
		t.Fatalf("guest connect: %d", st)
	}
	if op, msg := host.recv(t); op != wsOpText || string(msg) != "guest-joined" {
		t.Fatalf("host notice: %d %q", op, msg)
	}

	// The stream goes both ways untouched, including a save-sized message.
	guest.send(wsOpBinary, []byte("hello host"))
	if _, msg := host.recv(t); string(msg) != "hello host" {
		t.Fatalf("host got %q", msg)
	}
	big := make([]byte, gciSize)
	big[0], big[len(big)-1] = 7, 9
	host.send(wsOpBinary, big)
	if _, msg := guest.recv(t); len(msg) != gciSize || msg[0] != 7 || msg[len(msg)-1] != 9 {
		t.Fatalf("guest got %d bytes", len(msg))
	}

	// One guest at a time; the world map counts both players.
	if _, st := dialWS(t, srv.URL, live+"?role=guest", caro.token, nil); st != http.StatusConflict {
		t.Fatalf("second guest: %d", st)
	}
	ana.json("POST", "/api/v1/towns/"+town.ID+"/public", map[string]bool{"public": true}, nil, http.StatusOK)
	var world []worldTown
	caro.json("GET", "/api/v1/world", nil, &world, http.StatusOK)
	if len(world) != 1 || world[0].Players != 2 {
		t.Fatalf("world players: %+v", world)
	}

	// The guest leaving is announced to the host.
	guest.send(wsOpClose, nil)
	if op, msg := host.recv(t); op != wsOpText || string(msg) != "guest-left" {
		t.Fatalf("host notice: %d %q", op, msg)
	}
	host.conn.Close()
}
