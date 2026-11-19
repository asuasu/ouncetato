# Brotato Switch 2 Homebrew Port

Native Switch 2 homebrew port of **Brotato 1.2.0.3 (Steam)**, built with the open-source **Godot Engine** and **ounce-devkit**.

The project compiles a native AArch64 NRO using **libounce**, **OunceVK**, and **kaiHEN**. It runs Brotato's original GDScript and game data; it is not a Linux emulator, interpreter shim, or game reimplementation.

## Current state

Working features include:

- Original game with full content + both DLCs
- Native **1920×1080, 60 Hz** output with an FPS counter
- Vulkan 1.4 through OunceVK/NVK, with Godot's GLES3 renderer through Zink
- Music and sound effects through `audren`
- Single-player and **two-player local co-op**, including detached Joy-Cons in horizontal mode

Known limitations:

- Only **Brotato 1.2.0.3 from Steam** is supported. Other versions need a compatibility review.
- Steam/GOG online services, announcements, achievements APIs, mods, GameShare, and Streamplay are disabled.
- Co-op is tested with two detached Joy-Cons. Three- and four-player modes are not yet validated.
- Loading times improved but not identical to PC.

## Source and asset policy

This repository contains **engine and build code, small compatibility changes, and no Brotato game assets**.

The build recovers scripts from the **user's own PCK**, applies the compatibility changes, compiles the changed scripts, generates small binary resource deltas, and extracts the NRO icon from that PCK. The console applies those deltas in RAM without modifying the original PCK.

Users must supply their own legally owned copy of Brotato.

## Install a prebuilt release

A release built with `make release` contains the compiled engine, override configuration, and binary resource deltas. It contains no PCK.

You do not need Python, Docker, devkitPro, GDRE Tools, Godot sources, Git, or a compiler.

1. Download and unzip the [latest release](https://github.com/asuasu/ouncetato/releases).
2. Copy your original Steam Brotato 1.2.0.3 PCK file (Brotato.pck from the game directory in Steam) into the release's `brotato/` directory with the filename `brotato.pck`.
3. Copy the complete `brotato/` directory to `/switch/brotato/` on the SD card.

At startup, the engine verifies the original PCK and applies the resource deltas in RAM. Your PCK is never modified.

## Full build

### 1. Build the Ounce development image

If `ounce-devkit:latest` is not already available:

```sh
git clone https://github.com/ouncebrew/ounce-devkit.git
cd ounce-devkit
docker build -t ounce-devkit .
```

Then, in this repository:

```sh
make image
```

This creates `ounce-godot-build:latest`, with devkitA64, libounce, OunceVK, libkaihen, SCons, and packaging tools.

### 2. Fetch the exact Godot source

From the repository root:

```sh
mkdir -p work/godot
git -C work/godot init
git -C work/godot remote add origin https://github.com/godotengine/godot.git
git -C work/godot fetch --depth 1 origin 0a9011743f8b62e5f4505fd17ca61fc3621394ef
git -C work/godot checkout FETCH_HEAD
```

### 3. Supply Brotato's PCK

Place your Steam `Brotato.pck` here:

```text
work/Brotato.pck
```

Optional manual checksum check:

```sh
shasum -a 256 work/Brotato.pck
```

The expected hash is recorded in `manifests/v1.2.0.3.json`. `make verify` checks it and stops on a mismatch.

### 4. Compile and create the release

From the repository root:

```sh
make release
```

This compiles the engine and changed game scripts, generates `assets/brotato.bpd`, and creates `dist/release/brotato-switch2/` plus its ZIP. It also copies `work/Brotato.pck` unchanged into the release's `brotato/` directory, so that directory is ready to copy to the console. The ZIP does not contain a PCK.

All source checkouts, supplied game data, and build-time tools stay under the Git-ignored `work/` directory.

### 5. Copy the game to Switch 2

Copy the `brotato/` directory from the release to `/switch/brotato/` on the SD card. It must contain these files:

```text
/switch/brotato/
  brotato.nro
  brotato.pck
  brotato.bpd
  override.cfg
```

Launch **Brotato** through kaiHEN's hbmenu.

## Runtime files

The game creates these additional files on the SD card:

```text
/switch/brotato/user/          # Saves and settings
/switch/brotato/brotato.log    # Engine and script log, only when enabled
/switch/brotato/perf_log.csv   # Performance samples, only when enabled
```

File and performance logging are disabled by default. To enable them, set `file_logging=true` and/or `performance_logging=true` under `[brotato]` in `override.cfg`.

## Controls

Regular controls in single-player mode.
**Plus** exits the game from the main menu.
Single Joy-Cons in horizontal mode are supported. Join co-op by holding **A** on each Joy-Con.

- Left Joy-Con is player 1; right Joy-Con is player 2.
- Horizontal stick axes are rotated to match the controller orientation.
- **X → A, A → B, Y → X, B → Y** on the right Joy-Con.
- **Down → A, Left → B, Up → X, Right → Y** on the left Joy-Con.
- **SL/SR** switch tabs in menus.
- **Minus/Plus** open the menu.
- **L** toggles scrolling up/down for player 1.
- **R** toggles scrolling up/down for player 2.

The first L or R press scrolls up; the next press scrolls down. This gives each player independent description scrolling despite having only one analog stick.

## Build architecture

- **Godot Engine**, the open-source engine used by Brotato.
- **[Homebrodot/Godot](https://github.com/Homebrodot/Godot)**, whose homebrew Switch platform code provided the starting reference for EGL, input, application loop, packaging, and audio.
- **libounce**, the libnx-compatible Switch 2 runtime.
- **OunceVK/NVK and Zink**, providing Vulkan, OpenGL, OpenGL ES, and EGL.
- **kaiHEN**, the homebrew environment.
- **devkitA64 / devkitPro**, providing the compiler and NRO packaging tools.

The NRO is linked statically. The original PCK stays external and unchanged. A bounded cache holds it in RAM, where verified binary deltas replace the changed resources in memory.

## Updating Brotato or Godot

For a new game release:

1. Record its PCK hash, engine version/commit, PCK format, and GDScript bytecode revision in a new manifest.
2. Compare the new PCK's recovered scripts with the current version.
3. Port the small compatibility changes to the recovered scripts; do not replace new scripts wholesale with old modified copies.
4. Recompile changed scripts against the release's exact bytecode revision and regenerate the binary resource deltas.
5. Rebuild the engine if the Godot commit changed.
6. Test saves, DLC detection, menus, co-op, audio, and representative waves.
7. Keep the previous working NRO, PCK, and delta bundle available for rollback.

A data-only update with the same engine commit and bytecode requires a new PCK checksum and matching delta bundle, but normally no NRO rebuild. A new Godot major version, PCK format, or native dependency requires a larger engine review.

## Useful make targets

```sh
make image    # Build the compiler image
make engine   # Compile the Godot release template
make verify   # Verify the supplied PCK checksum
make compat   # Recover and compile only the modified scripts/resources
make delta    # Generate and verify the binary resource delta bundle
make brotato  # Package the engine, original PCK, and resource deltas
make release  # Create a standalone release without a game PCK
make clean    # Remove build and distribution outputs
```

## License and acknowledgements

Godot and Homebrodot are MIT-licensed. libounce is ISC/MIT, OunceVK is MIT, and libkaihen is ISC. devkitPro and its components retain their own licenses.

Brotato and its scripts, artwork, audio, translations, and other game data are proprietary. They are not included in this repository.
