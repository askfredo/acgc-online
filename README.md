# ACGC Online

Online play for the Animal Crossing (GameCube) PC port: share a town with friends and
siblings, up to four residents, and play in it together at the same time, something the
original game never had. Towns are saved on a server you can deploy yourself.

Built on [ACGC-Deluxe-PC-Port](https://github.com/imcynic/ACGC-Deluxe-PC-Port). Work in
progress: see [docs/ONLINE.md](docs/ONLINE.md) for the design and stages, and
[server/](server/) for the town server.

What works today (two players):

- The guest downloads the host's town and plays their own resident in it; their
  progress is saved into the shared town.
- You see each other with your own faces and clothes, tools in hand, walking,
  running and reacting, and you can't walk through each other.
- Items on the ground, fruit, digging and burying stay in sync; trees sway and doors
  open on both screens.
- Emotes on the D-pad (L + D-pad for more), with the villagers' feeling icons.
- A resident in a menu stays visible with a "thinking" icon.

Play with `--host` and `--join IP` for now; the start menu and cloud saves are next.
Like the original port, no game assets are included: you need your own disc image.

---

# Animal Crossing PC Port - Deluxe Edition (original README)

Native PC port of Animal Crossing (GameCube) with Deluxe content, built on [ac-decomp](https://github.com/ACreTeam/ac-decomp) and [ACGC-PC-Port](https://github.com/flyngmt/ACGC-PC-Port).

Runs the original decompiled C natively on x86 with a GX-to-OpenGL 3.3 translation layer. No game assets or assembly included, you need your own copy.

Supported version: GAFE01_00 Rev 0 (USA)

## Why this exists

I had no plans to release this publicly. I kept it closed-source out of respect for the modding community. That changed when Cuyler pushed for my removal from the community, spread false claims about my save editor, and tried to retroactively justify it by citing my use of AI tooling, which anyone paying attention can see had nothing to do with it.

Not interested in relitigating that. The work speaks for itself.

The port is mostly complete, nearly everything is implemented but there are some bugs and workarounds still in there. No assets included for legal reasons, bring your own. There's some dead code related to a Twitch integration that was in progress.

I moved on to the NBA Street Vol. 2 decompilation before finishing polish on this, and that's where my focus is now. People have been asking for this in various places, so here it is. Use it, learn from it, build on it.

## What's Deluxe?

Adds support for content from the Animal Crossing Deluxe ROM hack:

- New NPCs and interactions
- Monuments (clock, windmill, fountain, statue, bench, streetlight, bell, flower clock)
- Bulletin board system
- Countdown timer
- Additional furniture, insects, items
- Option to skip Tom Nook's intro delivery chores

Deluxe features are on by default (`VERSION >= VER_DELUXE` at compile time).

## Requirements

### Windows

- **MSYS2** (https://www.msys2.org/) - use the **MINGW32** shell
- Animal Crossing (USA) disc image (ISO, GCM, or CISO)

```bash
pacman -S mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-SDL2
```

### Linux

- 32-bit cross-compilation toolchain
- SDL2 (32-bit)

Debian/Ubuntu:
```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install gcc-i686-linux-gnu g++-i686-linux-gnu cmake libsdl2-dev:i386
```

## Building

### Windows (MSYS2 MINGW32)

```bash
./build_pc.sh
```

### Linux

```bash
mkdir -p pc/build32 && cd pc/build32
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-linux32.cmake
make -j$(nproc)
```

### Running

1. Put your disc image in `pc/build32/bin/rom/` (or `rom/` at the repo root)
2. Run `pc/build32/bin/AnimalCrossing` (`.exe` on Windows)

Assets are read straight from the disc image at startup. No extraction needed.

## Controls

Keyboard bindings are customizable via `keybindings.ini` (created on first run).

### Keyboard defaults

| Key | Action |
|-----|--------|
| WASD | Move (left stick) |
| Arrow Keys | Camera (C-stick) |
| Space | A button |
| Left Shift | B button |
| Enter | Start |
| X | X button |
| Y | Y button |
| Q / E | L / R triggers |
| Z | Z trigger |
| I / J / K / L | D-pad |

### Gamepad

SDL2 controllers work with automatic hotplug. Standard GameCube button layout.

## Command Line Options

| Flag | Description |
|------|-------------|
| `--verbose` | Diagnostic logging |
| `--no-framelimit` | Unlock FPS |
| `--model-viewer [index]` | Debug model viewer |
| `--time HOUR` | Override in-game hour (0-23) |

## Settings

Edit `settings.ini` (created on first run):

- Resolution (up to 4K)
- Fullscreen / borderless / windowed
- VSync
- MSAA
- Texture pack preloading

## Texture Packs

Drop Dolphin-compatible texture packs (XXHash64, DDS) in `texture_pack/` next to the executable.

## Save Data

Saves go in `save/` as standard GCI files. Compatible with Dolphin, drop a GCI export in there to import.

## Credits

- [ACreTeam](https://github.com/ACreTeam) - AC decompilation
- [flyngmt](https://github.com/flyngmt/ACGC-PC-Port) - original PC port
