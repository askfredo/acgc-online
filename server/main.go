// Command server stores Animal Crossing towns for online play: accounts, towns with up
// to four residents, cloud saves with backups, and the lock that picks a town's host.
//
// Configuration (environment):
//
//	PORT      HTTP port (default 8080; Railway sets it)
//	DATA_DIR  where towns and accounts are stored (default ./data; mount a volume here)
package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

func main() {
	dataDir := envOr("DATA_DIR", "./data")
	addr := ":" + envOr("PORT", "8080")

	store, err := OpenStore(dataDir)
	if err != nil {
		log.Fatalf("open data dir %s: %v", dataDir, err)
	}

	srv := &http.Server{
		Addr:              addr,
		Handler:           NewAPI(store).Handler(),
		ReadHeaderTimeout: 10 * time.Second,
		ReadTimeout:       60 * time.Second,
		WriteTimeout:      60 * time.Second,
	}

	go func() {
		log.Printf("listening on %s, data in %s", addr, dataDir)
		if err := srv.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			log.Fatal(err)
		}
	}()

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, syscall.SIGINT, syscall.SIGTERM)
	<-stop
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	srv.Shutdown(ctx)
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
