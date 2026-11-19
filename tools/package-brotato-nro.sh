#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TARGET=${TARGET:-release}
APP_TITLE=${APP_TITLE:-Brotato}
APP_VERSION=${APP_VERSION:-1.2.0.3}
GAME_PCK=${GAME_PCK:-"$ROOT/work/Brotato.pck"}
OUTPUT_DIR=${OUTPUT_DIR:-dist/switch/brotato}
ICON_SOURCE="$ROOT/build/brotato-icon.png"

mkdir -p "$ROOT/build" "$ROOT/$OUTPUT_DIR"
python3 "$ROOT/tools/pck.py" extract "$GAME_PCK" res://brotato_icon.png "$ICON_SOURCE"
sips -s format jpeg "$ICON_SOURCE" --out "$ROOT/build/brotato-icon.jpg" >/dev/null

docker run --rm \
    -v "$ROOT:/work" \
    -w /work \
    -e TARGET="$TARGET" \
    -e APP_TITLE="$APP_TITLE" \
    -e APP_VERSION="$APP_VERSION" \
    -e OUTPUT_DIR="$OUTPUT_DIR" \
    ounce-devkit:latest \
    sh -c 'case "$TARGET" in release) ELF_SUFFIX=switch2.opt.64 ;; release_debug) ELF_SUFFIX=switch2.opt.debug.64 ;; *) ELF_SUFFIX=switch2.debug.64 ;; esac; nacptool --create "$APP_TITLE" Blobfish "$APP_VERSION" build/brotato.nacp && elf2nro "/work/work/godot/bin/godot-switch2.$ELF_SUFFIX" "$OUTPUT_DIR/brotato.nro" --icon=build/brotato-icon.jpg --nacp=build/brotato.nacp'
