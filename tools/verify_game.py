#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path


def verify_checksum(pck_path, manifest_path):
    manifest = json.loads(Path(manifest_path).read_text())
    digest = hashlib.sha256(Path(pck_path).read_bytes()).hexdigest()
    expected = manifest["pck_sha256"]
    if digest != expected:
        raise ValueError(f"PCK checksum mismatch: expected {expected}, got {digest}")
    print(f"Verified Brotato {manifest['game_version']} PCK checksum: {digest}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--pck", required=True, type=Path)
    args = parser.parse_args()
    try:
        verify_checksum(args.pck, args.manifest)
    except ValueError as error:
        raise SystemExit(str(error))


if __name__ == "__main__":
    main()
