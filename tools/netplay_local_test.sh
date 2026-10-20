#!/usr/bin/env bash
# Run two game instances on this machine connected by netplay (host + join).
# The host plays a copy of the town in pc/build32/netplay/host (made from
# pc/build32/bin/save the first time, or again with --reset); the guest gets the
# town from the host over the network and never writes a save of its own.
#   tools/netplay_local_test.sh [--reset]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/pc/build32/bin"
RUN="$ROOT/pc/build32/netplay"

for side in host join; do
    dir="$RUN/$side"
    mkdir -p "$dir"
    ln -sfn "$BIN/AnimalCrossing" "$dir/AnimalCrossing"
    ln -sfn "$BIN/shaders" "$dir/shaders"
    [ -d "$BIN/rom" ] && ln -sfn "$BIN/rom" "$dir/rom"
    [ -f "$BIN/settings.ini" ] && cp -f "$BIN/settings.ini" "$dir/"
    [ -f "$BIN/keybindings.ini" ] && cp -f "$BIN/keybindings.ini" "$dir/"
done

if [ "${1:-}" = "--reset" ] || [ ! -d "$RUN/host/save" ]; then
    rm -rf "$RUN/host/save"
    mkdir -p "$RUN/host/save"
    cp -f "$BIN"/save/*.gci "$RUN/host/save/"
fi
rm -rf "$RUN/join/save"

(cd "$RUN/host" && ./AnimalCrossing --verbose --host > host.log 2>&1) &
sleep 1
(cd "$RUN/join" && ./AnimalCrossing --verbose --join 127.0.0.1 > join.log 2>&1) &
wait
