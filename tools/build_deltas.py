#!/usr/bin/env python3
import argparse
from pathlib import Path

import brotato_delta


def main():
    parser = argparse.ArgumentParser(description="Build runtime resource deltas from compatibility overlays")
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--compat", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    original_hash, records = brotato_delta.create_records(args.game, overlay_dir=args.compat)
    brotato_delta.write_delta(args.output, original_hash, records)
    brotato_delta.verify_delta(args.game, args.output)
    print(f"Created {args.output}: {len(records)} resources, {args.output.stat().st_size} bytes")


if __name__ == "__main__":
    main()
