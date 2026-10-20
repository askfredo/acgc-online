package main

import (
	"crypto/pbkdf2"
	"crypto/sha256"
	"crypto/subtle"
	"encoding/hex"
	"net"
	"net/http"
	"strings"
	"sync"
	"time"
)

const pbkdf2Iterations = 210_000

func hashPassword(password, salt string) string {
	key, err := pbkdf2.Key(sha256.New, password, []byte(salt), pbkdf2Iterations, 32)
	if err != nil {
		panic(err)
	}
	return hex.EncodeToString(key)
}

func checkPassword(password, salt, want string) bool {
	got := hashPassword(password, salt)
	return subtle.ConstantTimeCompare([]byte(got), []byte(want)) == 1
}

func bearerToken(r *http.Request) string {
	h := r.Header.Get("Authorization")
	if t, ok := strings.CutPrefix(h, "Bearer "); ok {
		return strings.TrimSpace(t)
	}
	return ""
}

// rateLimiter allows `burst` requests per `window` per client IP (used on login/register).
type rateLimiter struct {
	mu     sync.Mutex
	window time.Duration
	burst  int
	hits   map[string][]time.Time
}

func newRateLimiter(burst int, window time.Duration) *rateLimiter {
	return &rateLimiter{window: window, burst: burst, hits: map[string][]time.Time{}}
}

func (l *rateLimiter) allow(ip string) bool {
	l.mu.Lock()
	defer l.mu.Unlock()

	now := time.Now()
	recent := l.hits[ip][:0]
	for _, t := range l.hits[ip] {
		if now.Sub(t) < l.window {
			recent = append(recent, t)
		}
	}
	if len(recent) >= l.burst {
		l.hits[ip] = recent
		return false
	}
	l.hits[ip] = append(recent, now)
	return true
}

// clientIP prefers the proxy's header: on Railway every request comes through its edge.
func clientIP(r *http.Request) string {
	if fwd := r.Header.Get("X-Forwarded-For"); fwd != "" {
		ip, _, _ := strings.Cut(fwd, ",")
		return strings.TrimSpace(ip)
	}
	host, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		return r.RemoteAddr
	}
	return host
}
