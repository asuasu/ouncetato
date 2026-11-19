BUILD_IMAGE ?= ounce-godot-build:latest
GODOT_SOURCE ?= /work/work/godot
TARGET ?= release
JOBS ?=
GDRE ?=
GAME_PCK ?= work/Brotato.pck
PROFILE ?= v1.2.0.3

.PHONY: image engine smoke compat delta verify brotato release clean

image:
	docker build -f docker/Dockerfile -t $(BUILD_IMAGE) .

engine:
	IMAGE=$(BUILD_IMAGE) GODOT_SOURCE=$(GODOT_SOURCE) TARGET=$(TARGET) JOBS=$(JOBS) sh tools/docker-build-engine.sh

smoke:
	python3 tools/pck.py create smoke/source dist/switch/godot-switch2/godot-switch2.pck --engine-version 3.7.0
	mkdir -p dist/switch/godot-switch2
	cp work/godot/bin/godot-switch2_$(TARGET).nro dist/switch/godot-switch2/godot-switch2.nro

compat:
	python3 tools/build_compat.py --gdre "$(GDRE)" --source compat --profile $(PROFILE) --output build/compat --game-pack "$(GAME_PCK)"

delta: compat
	python3 tools/build_deltas.py --game "$(GAME_PCK)" --compat build/compat --output assets/brotato.bpd

verify:
	python3 tools/verify_game.py --manifest manifests/$(PROFILE).json --pck "$(GAME_PCK)"

brotato: engine delta
	TARGET=$(TARGET) GAME_PCK="$(GAME_PCK)" sh tools/package-brotato-nro.sh
	cp "$(GAME_PCK)" dist/switch/brotato/brotato.pck
	cp assets/override.cfg assets/brotato.bpd dist/switch/brotato/

release: engine delta
	TARGET=$(TARGET) sh tools/package-brotato-nro.sh
	python3 tools/package_release.py
	cp "$(GAME_PCK)" dist/release/brotato-switch2/brotato/brotato.pck

clean:
	rm -rf build dist
