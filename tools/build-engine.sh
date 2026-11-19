#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
GODOT_SOURCE=${GODOT_SOURCE:-/work/work/godot}
TARGET=${TARGET:-release}
JOBS=${JOBS:-$(nproc)}

marker_dir="$GODOT_SOURCE/.switch2-applied-patches"
mkdir -p "$marker_dir"
for patch_name in \
    seaven-os-api.patch \
    coop-os-api.patch \
    coop-controller-color.patch \
    gles2-switch2.patch \
    libc-file-access.patch \
    pck-ram-cache.patch
do
    patch="$ROOT/engine/patches/$patch_name"
    patch_hash=$(sha256sum "$patch")
    patch_hash=${patch_hash%% *}
    marker="$marker_dir/$(basename "$patch").$patch_hash"
    if [ -e "$marker" ]; then
        continue
    fi
    if git -C "$GODOT_SOURCE" apply --reverse --check --recount "$patch" >/dev/null 2>&1; then
        touch "$marker"
        continue
    fi
    git -C "$GODOT_SOURCE" apply --recount "$patch"
    touch "$marker"
done

rm -rf "$GODOT_SOURCE/platform/switch2" "$GODOT_SOURCE/drivers/audren"
cp -R "$ROOT/engine/platform/switch2" "$GODOT_SOURCE/platform/switch2"
cp -R "$ROOT/engine/drivers/audren" "$GODOT_SOURCE/drivers/audren"

cd "$GODOT_SOURCE"
exec scons \
    platform=switch2 \
    target="$TARGET" \
    tools=no \
    disable_3d=yes \
    module_visual_script_enabled=no \
    module_enet_enabled=no \
    module_webrtc_enabled=no \
    module_websocket_enabled=no \
    module_upnp_enabled=no \
    module_mbedtls_enabled=no \
    module_webm_enabled=no \
    module_theora_enabled=no \
    module_bullet_enabled=no \
    module_csg_enabled=no \
    module_fbx_enabled=no \
    module_gltf_enabled=no \
    builtin_pcre2_with_jit=no \
    lto=none \
    debug_symbols=no \
    -j"$JOBS" \
    "$@"
