# Town server

Stores towns for online play: accounts, towns with up to four residents, cloud saves
with backups, and the lock that decides which resident hosts a town. One Go binary,
standard library only, plain files on disk.

## Run locally

```bash
cd server
go run .                      # http://localhost:8080, data in ./data
go test ./...
```

Environment:

| Variable | Default | |
|---|---|---|
| `PORT` | `8080` | Railway sets it |
| `DATA_DIR` | `./data` | Mount a persistent volume here |

## Deploy on Railway

1. New project → **Deploy from GitHub repo** → pick this repository.
2. Service **Settings → Root Directory**: `/server`. Railway builds the `Dockerfile`
   (see `railway.json`).
3. Service **Volumes → New volume**, mount path `/data`. Without a volume every
   redeploy loses all towns.
4. **Settings → Networking → Generate Domain**. That URL is what players put in the game.

A town save is 456 KB; with 10 backups each, a 0.5 GB volume holds about 100 towns.

## API

All endpoints except register/login take `Authorization: Bearer <token>`.
Errors are `{"error": "..."}`.

| Method | Path | |
|---|---|---|
| `POST` | `/api/v1/register` | `{"username","password"}` → `{"username","token"}` |
| `POST` | `/api/v1/login` | same |
| `GET` | `/api/v1/towns` | towns you live in, with `free_slots` and the current `host` |
| `POST` | `/api/v1/towns` | `{"name"}` → new town with its invite `code` |
| `POST` | `/api/v1/towns/join` | `{"code"}` → become a resident (409 if it already has 4) |
| `GET` | `/api/v1/towns/{id}` | one town |
| `GET` | `/api/v1/towns/{id}/save` | the latest save (GCI); version in `X-Save-Version` |
| `PUT` | `/api/v1/towns/{id}/save` | upload; needs `X-Lock-Id` and the `X-Save-Version` it was loaded from |
| `POST` | `/api/v1/towns/{id}/lock` | become the host (200 with `lock_id`), or 409 with who is hosting |
| `POST` | `/api/v1/towns/{id}/lock/renew` | keep hosting (`X-Lock-Id`); locks expire after 60 s |
| `DELETE` | `/api/v1/towns/{id}/lock` | stop hosting |

### How a game uses it

1. Log in once and keep the token.
2. Opening a town: `POST .../lock`.
   - 200: you host. Download the save, play, renew every ~20 s, upload on save
     (`X-Save-Version` = the version you downloaded, then the one returned).
   - 409: someone is hosting: join their live session instead (stage 4).
3. Closing: `DELETE .../lock`.

An upload based on an old version is refused (409), so a stale game can never
overwrite newer progress.
