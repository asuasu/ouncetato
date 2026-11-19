#!/usr/bin/env python3
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

import gdre_tool
import pck
import unified_patch


def patch_resource(patch_file):
    with patch_file.open() as source:
        for line in source:
            if line.startswith("diff --git a/"):
                return line.split()[2][2:]
    raise ValueError(f"Missing resource path in {patch_file}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--gdre", type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--game-pack", required=True, type=Path)
    args = parser.parse_args()
    gdre = gdre_tool.resolve_gdre(args.gdre, Path(__file__).resolve().parent.parent)

    patch_roots = [args.source / "common", args.source / args.profile]
    patch_files = []
    for root in patch_roots:
        if root.is_dir():
            patch_files.extend(sorted(root.rglob("*.patch")))
    if not patch_files:
        raise ValueError("No compatibility patches found")

    if args.output.exists():
        shutil.rmtree(args.output)
    args.output.mkdir(parents=True, exist_ok=True)
    recovered = args.output.parent / "recovered"
    if recovered.exists():
        shutil.rmtree(recovered)
    subprocess.run([
        str(gdre),
        "--headless",
        f"--recover={args.game_pack}",
        f"--output={recovered}",
        "--scripts-only",
    ], check=True)

    for patch_file in patch_files:
        resource = patch_resource(patch_file)
        if not resource.endswith(".gd"):
            pck.extract_file(args.game_pack, "res://" + resource, recovered / resource)

    for patch_file in patch_files:
        unified_patch.apply_patch(recovered / patch_resource(patch_file), patch_file)

    with tempfile.TemporaryDirectory(dir=args.output.parent) as temporary:
        temporary = Path(temporary)
        for patch_file in patch_files:
            resource = patch_resource(patch_file)
            if resource.endswith(".gd"):
                subprocess.run([
                    str(gdre),
                    "--headless",
                    f"--compile={recovered / resource}",
                    "--bytecode=3.5.0-stable",
                    f"--output={temporary}",
                ], check=True)
                compiled = temporary / (Path(resource).stem + ".gdc")
                destination = args.output / Path(resource).with_suffix(".gdc")
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(compiled, destination)
                compiled.unlink()
                print(f"Compiled {resource}")
            else:
                destination = args.output / resource
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(recovered / resource, destination)
                print(f"Copied {resource}")
    print(f"Built {len(patch_files)} compatibility patches")


if __name__ == "__main__":
    main()
