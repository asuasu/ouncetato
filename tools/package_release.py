#!/usr/bin/env python3
import argparse
from pathlib import Path
import shutil


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, default=root / "dist/switch/brotato/brotato.nro")
    parser.add_argument("--output", type=Path, default=root / "dist/release/brotato-switch2")
    args = parser.parse_args()

    if not args.engine.is_file():
        raise SystemExit(f"Missing compiled engine: {args.engine}")
    if args.output.exists():
        shutil.rmtree(args.output)
    args.output.mkdir(parents=True)

    brotato_dir = args.output / "brotato"
    brotato_dir.mkdir()
    shutil.copyfile(args.engine, brotato_dir / "brotato.nro")
    shutil.copyfile(root / "assets/override.cfg", brotato_dir / "override.cfg")
    shutil.copyfile(root / "assets/brotato.bpd", brotato_dir / "brotato.bpd")
    shutil.copyfile(root / "README.md", args.output / "README.md")

    print(f"Standalone release created: {args.output}")
    archive = shutil.make_archive(str(args.output), "zip", root_dir=args.output.parent, base_dir=args.output.name)
    print(f"Release archive created: {archive}")


if __name__ == "__main__":
    main()
