package main

import (
	"sync"
	"time"
)

// A town is hosted by one resident at a time: their game is the authority while
// others play in it. The host renews the lock while playing; if the game dies the
// lock simply expires. Locks live in memory only: after a server restart every town
// is free again, which is the safe direction.

const lockTTL = 60 * time.Second

type HostLock struct {
	TownID  string    `json:"town_id"`
	Host    string    `json:"host"`
	LockID  string    `json:"lock_id,omitempty"` // only shown to the host
	Since   time.Time `json:"since"`
	Expires time.Time `json:"expires"`
}

type Locks struct {
	mu    sync.Mutex
	locks map[string]*HostLock
	now   func() time.Time
}

func newLocks() *Locks {
	return &Locks{locks: map[string]*HostLock{}, now: time.Now}
}

func (l *Locks) activeLocked(townID string) *HostLock {
	lk := l.locks[townID]
	if lk != nil && l.now().After(lk.Expires) {
		delete(l.locks, townID)
		return nil
	}
	return lk
}

// Acquire returns the town's lock: a new one for the user if the town was free or
// already theirs, otherwise the current host's lock without its id. Taking it again gets a
// fresh id: the newest session hosts, and an older one (a game that was restarted) can no
// longer renew, upload or release it.
func (l *Locks) Acquire(townID, user string) (lk HostLock, acquired bool) {
	l.mu.Lock()
	defer l.mu.Unlock()

	cur := l.activeLocked(townID)
	if cur != nil && cur.Host != user {
		other := *cur
		other.LockID = ""
		return other, false
	}
	now := l.now()
	if cur == nil {
		cur = &HostLock{TownID: townID, Host: user, Since: now}
		l.locks[townID] = cur
	}
	cur.LockID = randomHex(16)
	cur.Expires = now.Add(lockTTL)
	return *cur, true
}

func (l *Locks) Renew(townID, lockID string) (HostLock, bool) {
	l.mu.Lock()
	defer l.mu.Unlock()

	cur := l.activeLocked(townID)
	if cur == nil || cur.LockID != lockID {
		return HostLock{}, false
	}
	cur.Expires = l.now().Add(lockTTL)
	return *cur, true
}

func (l *Locks) Release(townID, lockID string) bool {
	l.mu.Lock()
	defer l.mu.Unlock()

	cur := l.activeLocked(townID)
	if cur == nil || cur.LockID != lockID {
		return false
	}
	delete(l.locks, townID)
	return true
}

// Holds reports whether lockID is the town's current lock.
func (l *Locks) Holds(townID, lockID string) bool {
	l.mu.Lock()
	defer l.mu.Unlock()

	cur := l.activeLocked(townID)
	return cur != nil && cur.LockID == lockID
}

// Host returns the current host of the town, if any (without the lock id).
func (l *Locks) Host(townID string) *HostLock {
	l.mu.Lock()
	defer l.mu.Unlock()

	cur := l.activeLocked(townID)
	if cur == nil {
		return nil
	}
	c := *cur
	c.LockID = ""
	return &c
}
