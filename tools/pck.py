#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

MAGIC = 0x43504447
FORMAT_VERSION = 1
HEADER_SIZE = 88


def encode_path(path):
    raw = path.encode("utf-8") + b"\0"
    return raw + b"\0" * (-len(raw) % 4)


def file_digest(path):
    digest = hashlib.md5()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()


def disk_entry(resource_path, source_path):
    size = source_path.stat().st_size
    return {
        "path": resource_path,
        "size": size,
        "md5": file_digest(source_path),
        "source": str(source_path),
        "offset": 0,
    }


def read_pack(path):
    with path.open("rb") as source:
        header = source.read(HEADER_SIZE)
        if len(header) != HEADER_SIZE:
            raise ValueError("Incomplete PCK header")
        magic, format_version, major, minor, patch = struct.unpack_from("<5I", header)
        if magic != MAGIC:
            raise ValueError("Not a Godot PCK")
        if format_version != FORMAT_VERSION:
            raise ValueError(f"Unsupported PCK format: {format_version}")
        count = struct.unpack_from("<I", header, 84)[0]
        entries = []
        for _ in range(count):
            path_length = struct.unpack("<I", source.read(4))[0]
            resource_path = source.read(path_length).rstrip(b"\0").decode("utf-8")
            offset, size = struct.unpack("<QQ", source.read(16))
            md5 = source.read(16)
            entries.append({
                "path": resource_path,
                "size": size,
                "md5": md5,
                "source": str(path),
                "offset": offset,
            })
        return {
            "major": major,
            "minor": minor,
            "patch": patch,
            "entries": entries,
        }


def copy_range(source, output, size):
    remaining = size
    while remaining:
        chunk = source.read(min(1024 * 1024, remaining))
        if not chunk:
            raise ValueError("Unexpected end of source data")
        output.write(chunk)
        remaining -= len(chunk)


def write_pack(output_path, major, minor, patch, entries):
    encoded_paths = [encode_path(entry["path"]) for entry in entries]
    data_offset = HEADER_SIZE + sum(4 + len(path) + 32 for path in encoded_paths)
    offsets = []
    for entry in entries:
        offsets.append(data_offset)
        data_offset += entry["size"]

    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = output_path.with_name(output_path.name + ".tmp")
    sources = {}
    try:
        with temporary.open("wb") as output:
            output.write(struct.pack("<5I", MAGIC, FORMAT_VERSION, major, minor, patch))
            output.write(b"\0" * 64)
            output.write(struct.pack("<I", len(entries)))
            for entry, encoded_path, offset in zip(entries, encoded_paths, offsets):
                output.write(struct.pack("<I", len(encoded_path)))
                output.write(encoded_path)
                output.write(struct.pack("<QQ", offset, entry["size"]))
                output.write(entry["md5"])
            for entry in entries:
                source_path = entry["source"]
                if source_path not in sources:
                    sources[source_path] = open(source_path, "rb")
                source = sources[source_path]
                source.seek(entry["offset"])
                copy_range(source, output, entry["size"])
        os.replace(temporary, output_path)
    finally:
        for source in sources.values():
            source.close()
        if temporary.exists():
            temporary.unlink()


def create(source_dir, output_path, engine_version):
    source_dir = source_dir.resolve()
    entries = []
    for path in sorted(source_dir.rglob("*")):
        if path.is_file():
            resource_path = "res://" + path.relative_to(source_dir).as_posix()
            entries.append(disk_entry(resource_path, path))
    major, minor, patch = (int(value) for value in engine_version.split("."))
    write_pack(output_path, major, minor, patch, entries)
    print(f"Created {output_path} with {len(entries)} files")


def inspect(path):
    pack = read_pack(path)
    print(json.dumps({
        "engine": f'{pack["major"]}.{pack["minor"]}.{pack["patch"]}',
        "files": len(pack["entries"]),
        "data_bytes": sum(entry["size"] for entry in pack["entries"]),
    }, indent=2))


def extract_file(pack_path, resource_path, output_path):
    pack = read_pack(pack_path)
    for entry in pack["entries"]:
        if entry["path"] == resource_path:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            with pack_path.open("rb") as source:
                source.seek(entry["offset"])
                output_path.write_bytes(source.read(entry["size"]))
            print(f"Extracted {resource_path}")
            return
    raise ValueError(f"Resource not found: {resource_path}")


def verify(path):
    pack = read_pack(path)
    with path.open("rb") as source:
        for index, entry in enumerate(pack["entries"], 1):
            source.seek(entry["offset"])
            remaining = entry["size"]
            digest = hashlib.md5()
            while remaining:
                chunk = source.read(min(1024 * 1024, remaining))
                if not chunk:
                    raise ValueError(f'Unexpected end of data in {entry["path"]}')
                digest.update(chunk)
                remaining -= len(chunk)
            if digest.digest() != entry["md5"]:
                raise ValueError(f'Checksum mismatch: {entry["path"]}')
            if index % 1000 == 0:
                print(f"Verified {index}/{len(pack['entries'])} files")
    print(f"Verified {len(pack['entries'])} files")


def main():
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    create_parser = commands.add_parser("create")
    create_parser.add_argument("source", type=Path)
    create_parser.add_argument("output", type=Path)
    create_parser.add_argument("--engine-version", default="3.7.0")
    inspect_parser = commands.add_parser("inspect")
    inspect_parser.add_argument("input", type=Path)
    extract_parser = commands.add_parser("extract")
    extract_parser.add_argument("input", type=Path)
    extract_parser.add_argument("resource", type=str)
    extract_parser.add_argument("output", type=Path)
    verify_parser = commands.add_parser("verify")
    verify_parser.add_argument("input", type=Path)
    args = parser.parse_args()

    if args.command == "create":
        create(args.source, args.output, args.engine_version)
    elif args.command == "verify":
        verify(args.input)
    elif args.command == "extract":
        extract_file(args.input, args.resource, args.output)
    else:
        inspect(args.input)


if __name__ == "__main__":
    main()
