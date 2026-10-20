package main

import (
	"bufio"
	"crypto/sha1"
	"encoding/base64"
	"encoding/binary"
	"errors"
	"io"
	"net"
	"net/http"
	"strings"
	"sync"
	"time"
)

// A minimal WebSocket (RFC 6455) server side: the handshake, unfragmented binary and
// text messages, ping/pong and close. Enough for the live relay, with no dependencies.

const (
	wsOpContinuation = 0x0
	wsOpText         = 0x1
	wsOpBinary       = 0x2
	wsOpClose        = 0x8
	wsOpPing         = 0x9
	wsOpPong         = 0xA

	wsMaxMessage = 4 << 20 // a town save is 456 KB
	wsGUID       = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
)

var errWSClosed = errors.New("websocket closed")

type wsConn struct {
	conn net.Conn
	r    *bufio.Reader
	wmu  sync.Mutex // writes come from the relay and the pinger
}

// upgradeWS answers the WebSocket handshake and takes over the connection.
func upgradeWS(w http.ResponseWriter, r *http.Request) (*wsConn, error) {
	if !strings.EqualFold(r.Header.Get("Upgrade"), "websocket") ||
		!strings.Contains(strings.ToLower(r.Header.Get("Connection")), "upgrade") {
		return nil, errors.New("not a websocket request")
	}
	key := r.Header.Get("Sec-WebSocket-Key")
	if key == "" {
		return nil, errors.New("missing Sec-WebSocket-Key")
	}
	hj, ok := w.(http.Hijacker)
	if !ok {
		return nil, errors.New("connection can't be taken over")
	}
	conn, rw, err := hj.Hijack()
	if err != nil {
		return nil, err
	}
	sum := sha1.Sum([]byte(key + wsGUID))
	resp := "HTTP/1.1 101 Switching Protocols\r\n" +
		"Upgrade: websocket\r\nConnection: Upgrade\r\n" +
		"Sec-WebSocket-Accept: " + base64.StdEncoding.EncodeToString(sum[:]) + "\r\n\r\n"
	conn.SetDeadline(time.Time{})
	if _, err := conn.Write([]byte(resp)); err != nil {
		conn.Close()
		return nil, err
	}
	return &wsConn{conn: conn, r: rw.Reader}, nil
}

// readMessage returns the next text or binary message, answering pings on the way.
func (c *wsConn) readMessage() (op byte, data []byte, err error) {
	for {
		var hdr [2]byte
		if _, err = io.ReadFull(c.r, hdr[:]); err != nil {
			return 0, nil, err
		}
		fin := hdr[0]&0x80 != 0
		op = hdr[0] & 0x0F
		masked := hdr[1]&0x80 != 0
		n := uint64(hdr[1] & 0x7F)
		switch n {
		case 126:
			var ext [2]byte
			if _, err = io.ReadFull(c.r, ext[:]); err != nil {
				return 0, nil, err
			}
			n = uint64(binary.BigEndian.Uint16(ext[:]))
		case 127:
			var ext [8]byte
			if _, err = io.ReadFull(c.r, ext[:]); err != nil {
				return 0, nil, err
			}
			n = binary.BigEndian.Uint64(ext[:])
		}
		if n > wsMaxMessage || !masked || !fin || op == wsOpContinuation {
			// Clients must mask; the game never fragments.
			return 0, nil, errors.New("unsupported websocket frame")
		}
		var mask [4]byte
		if _, err = io.ReadFull(c.r, mask[:]); err != nil {
			return 0, nil, err
		}
		data = make([]byte, n)
		if _, err = io.ReadFull(c.r, data); err != nil {
			return 0, nil, err
		}
		for i := range data {
			data[i] ^= mask[i%4]
		}

		switch op {
		case wsOpPing:
			if err = c.write(wsOpPong, data); err != nil {
				return 0, nil, err
			}
		case wsOpPong:
		case wsOpClose:
			c.write(wsOpClose, nil)
			return 0, nil, errWSClosed
		case wsOpText, wsOpBinary:
			return op, data, nil
		default:
			return 0, nil, errors.New("unknown websocket opcode")
		}
	}
}

func (c *wsConn) write(op byte, data []byte) error {
	var hdr [10]byte
	hdr[0] = 0x80 | op
	n := len(hdr[:2])
	switch {
	case len(data) < 126:
		hdr[1] = byte(len(data))
	case len(data) <= 0xFFFF:
		hdr[1] = 126
		binary.BigEndian.PutUint16(hdr[2:], uint16(len(data)))
		n = 4
	default:
		hdr[1] = 127
		binary.BigEndian.PutUint64(hdr[2:], uint64(len(data)))
		n = 10
	}

	c.wmu.Lock()
	defer c.wmu.Unlock()
	c.conn.SetWriteDeadline(time.Now().Add(15 * time.Second))
	if _, err := c.conn.Write(hdr[:n]); err != nil {
		return err
	}
	_, err := c.conn.Write(data)
	return err
}

func (c *wsConn) close() {
	c.write(wsOpClose, nil)
	c.conn.Close()
}
