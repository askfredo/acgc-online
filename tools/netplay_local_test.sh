#!/usr/bin/env bash
# Run two game instances on this machine connected by netplay (host + join).
# Both start from the same town save so positions line up.
#   tools/netplay_local_test.sh            # uses save from pc/build32/bin/save
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/pc/build32/bin"
RUN="$ROOT/pc/build32/netplay"

for side in host join; do
    dir="$RUN/$side"
    mkdir -p "$dir/save"
    ln -sfn "$BIN/AnimalCrossing" "$dir/AnimalCrossing"
    ln -sfn "$BIN/shaders" "$dir/shaders"
    [ -d "$BIN/rom" ] && ln -sfn "$BIN/rom" "$dir/rom"
    [ -d "$ROOT/rom" ] && [ ! -e "$dir/rom" ] && ln -sfn "$ROOT/rom" "$dir/rom"
    # Fresh copy of the shared town each run.
    if [ -d "$BIN/save" ]; then
        cp -f "$BIN"/save/*.gci "$dir/save/" 2>/dev/null || true
    fi
    [ -f "$BIN/settings.ini" ] && cp -f "$BIN/settings.ini" "$dir/"
    [ -f "$BIN/keybindings.ini" ] && cp -f "$BIN/keybindings.ini" "$dir/"
done

(cd "$RUN/host" && ./AnimalCrossing --verbose --host > host.log 2>&1) &
sleep 1
(cd "$RUN/join" && ./AnimalCrossing --verbose --join 127.0.0.1 > join.log 2>&1) &
wait
