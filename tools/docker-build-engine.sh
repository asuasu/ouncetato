#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=${IMAGE:-ounce-godot-build:latest}

docker run --rm \
    -v "$ROOT:/work" \
    -w /work \
    -e GODOT_SOURCE="${GODOT_SOURCE:-/work/work/godot}" \
    -e TARGET="${TARGET:-release}" \
    -e JOBS="${JOBS:-}" \
    "$IMAGE" \
    /work/tools/build-engine.sh "$@"
