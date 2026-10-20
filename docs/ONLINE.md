# Online play

Play Animal Crossing (GameCube) with friends and siblings: towns live on a server,
up to four residents share a town, and residents who are online at the same time
play together live.

## Overview

```
  Player A (PC)                  Server (e.g. Railway)                Player B (PC)
┌────────────────┐         ┌──────────────────────────┐         ┌────────────────┐
│ Game + mod     │◄───────►│ • Accounts and towns     │◄───────►│ Game + mod     │
│ Start menu     │  HTTPS  │ • Town saves (cloud)     │  HTTPS  │ Start menu     │
│                │   +     │ • Town lock (one host)   │   +     │                │
│                │  live   │ • Live message relay     │  live   │                │
└────────────────┘         └──────────────────────────┘         └────────────────┘
```

The server stores towns and relays live messages; it does not run the game. The
first resident to open a town becomes its host: their game is the authority for the
town while others play in it (see `src/game/m_netplay.c`).

## Start menu

| Option | What happens |
|---|---|
| My towns | Towns you live in, and who is online. If nobody is playing, you host it; otherwise you join the session. |
| Create a town | The normal intro runs; the first save uploads the town and gives you an invite code. |
| Join with a code | Become a resident of a town with a free slot (max 4): create your character, get a free house. |
| Visit | Go to someone's town with your own character (the game's original visitor slot). Needs the host online. |
| Play offline | Local save, as before. |

## Server

Go, standard library only (`server/`). REST over HTTPS for accounts, towns, saves
and the town lock; WebSocket for live messages. Data is plain files in `DATA_DIR`:
one folder per town with its save and the last backups. See `server/README.md`.

## Stages

1. Server: accounts, towns, save upload/download, town lock, backups, Railway deploy.
2. Game ↔ cloud: download the town on start, upload on save.
3. Start menu.
4. Live play through the server (WebSocket) instead of direct connections.
5. Windows build and GitHub Actions releases.
6. Visiting other towns.
7. Game gaps: letters, villagers, shop, museum, fishing.
8. Up to four players at once.

## What is never distributed

No game assets: everyone uses their own disc image. Saves hold game state only.
