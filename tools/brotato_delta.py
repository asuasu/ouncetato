#!/usr/bin/env python3
import argparse
import hashlib
from pathlib import Path
import struct

import pck

MAGIC = b"BRDL"
VERSION = 1
COPY = 0
LITERAL = 1
ANCHOR_SIZE = 16
MAX_CANDIDATES = 64
OP = struct.Struct("<IQQ")


def make_operations(source, target):
    anchors = {}
    for offset in range(max(0, len(source) - ANCHOR_SIZE + 1)):
        key = source[offset:offset + ANCHOR_SIZE]
        positions = anchors.setdefault(key, [])
        if len(positions) < MAX_CANDIDATES:
            positions.append(offset)

    operations = []
    target_offset = 0
    literal_start = 0
    while target_offset < len(target):
        if target_offset + ANCHOR_SIZE > len(target):
            break
        positions = anchors.get(target[target_offset:target_offset + ANCHOR_SIZE], ())
        best_source = -1
        best_length = 0
        for source_offset in positions:
            length = ANCHOR_SIZE
            while (source_offset + length < len(source) and target_offset + length < len(target)
                   and source[source_offset + length] == target[target_offset + length]):
                length += 1
            if length > best_length:
                best_source = source_offset
                best_length = length
        if best_source < 0:
            target_offset += 1
            continue

        while (best_source > 0 and target_offset > literal_start
               and source[best_source - 1] == target[target_offset - 1]):
            best_source -= 1
            target_offset -= 1
            best_length += 1
        if literal_start < target_offset:
            operations.append((LITERAL, 0, target[literal_start:target_offset]))
        operations.append((COPY, best_source, best_length))
        target_offset += best_length
        literal_start = target_offset

    if literal_start < len(target):
        operations.append((LITERAL, 0, target[literal_start:]))
    return operations


def apply_operations(source, operations, result_size):
    output = bytearray()
    for kind, source_offset, value in operations:
        if kind == COPY:
            if source_offset + value > len(source):
                raise ValueError("Delta copy exceeds source resource")
            output.extend(source[source_offset:source_offset + value])
        elif kind == LITERAL:
            output.extend(value)
        else:
            raise ValueError(f"Unknown delta operation: {kind}")
        if len(output) > result_size:
            raise ValueError("Delta result exceeds expected size")
    if len(output) != result_size:
        raise ValueError(f"Delta result size mismatch: expected {result_size}, got {len(output)}")
    return bytes(output)


def read_entry(pack_path, entry):
    with pack_path.open("rb") as source:
        source.seek(entry["offset"])
        data = source.read(entry["size"])
    if len(data) != entry["size"]:
        raise ValueError(f"Incomplete resource: {entry['path']}")
    return data


def create_records(original_path, overlay_dir):
    original_pack = pck.read_pack(original_path)
    original_hash = hashlib.sha256(original_path.read_bytes()).digest()
    records = []
    for entry in original_pack["entries"]:
        resource_path = entry["path"]
        replacement = overlay_dir / Path(resource_path.removeprefix("res://"))
        if not replacement.is_file():
            continue
        target = replacement.read_bytes()
        source = read_entry(original_path, entry)
        if source == target:
            continue
        operations = make_operations(source, target)
        result = apply_operations(source, operations, len(target))
        if result != target:
            raise ValueError(f"Delta self-check failed: {resource_path}")
        records.append({
            "path": resource_path,
            "source": source,
            "target": target,
            "operations": operations,
        })
    return original_hash, records


def write_delta(output_path, original_hash, records):
    output = bytearray(struct.pack("<4sI", MAGIC, VERSION))
    output.extend(original_hash)
    output.extend(struct.pack("<I", len(records)))
    for record in records:
        path = record["path"].encode("utf-8")
        source = record["source"]
        target = record["target"]
        output.extend(struct.pack("<I", len(path)))
        output.extend(path)
        output.extend(struct.pack("<QQ", len(source), len(target)))
        output.extend(hashlib.md5(source).digest())
        output.extend(hashlib.md5(target).digest())
        output.extend(struct.pack("<I", len(record["operations"])))
        for kind, source_offset, value in record["operations"]:
            if kind == COPY:
                output.extend(OP.pack(kind, source_offset, value))
            else:
                output.extend(OP.pack(kind, 0, len(value)))
                output.extend(value)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = output_path.with_name(output_path.name + ".tmp")
    temporary.write_bytes(output)
    temporary.replace(output_path)


class Reader:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def read(self, size):
        end = self.offset + size
        if end > len(self.data):
            raise ValueError("Truncated delta bundle")
        result = self.data[self.offset:end]
        self.offset = end
        return result

    def u32(self):
        return struct.unpack("<I", self.read(4))[0]

    def u64(self):
        return struct.unpack("<Q", self.read(8))[0]


def read_delta(delta_path):
    reader = Reader(delta_path.read_bytes())
    if reader.read(4) != MAGIC:
        raise ValueError("Not a Brotato resource delta bundle")
    version = reader.u32()
    if version != VERSION:
        raise ValueError(f"Unsupported delta version: {version}")
    original_hash = reader.read(32)
    records = []
    for _ in range(reader.u32()):
        path = reader.read(reader.u32()).decode("utf-8")
        source_size = reader.u64()
        result_size = reader.u64()
        source_md5 = reader.read(16)
        result_md5 = reader.read(16)
        operations = []
        for _ in range(reader.u32()):
            kind, source_offset, length = OP.unpack(reader.read(OP.size))
            value = reader.read(length) if kind == LITERAL else length
            operations.append((kind, source_offset, value))
        records.append({
            "path": path,
            "source_size": source_size,
            "result_size": result_size,
            "source_md5": source_md5,
            "result_md5": result_md5,
            "operations": operations,
        })
    if reader.offset != len(reader.data):
        raise ValueError("Trailing data in delta bundle")
    return original_hash, records


def verify_delta(original_path, delta_path):
    original_hash, records = read_delta(delta_path)
    actual_hash = hashlib.sha256(original_path.read_bytes()).digest()
    if actual_hash != original_hash:
        raise ValueError("Original PCK SHA-256 does not match the delta bundle")
    pack = pck.read_pack(original_path)
    entries = {entry["path"]: entry for entry in pack["entries"]}
    for record in records:
        source = read_entry(original_path, entries[record["path"]])
        if len(source) != record["source_size"] or hashlib.md5(source).digest() != record["source_md5"]:
            raise ValueError(f"Source resource mismatch: {record['path']}")
        result = apply_operations(source, record["operations"], record["result_size"])
        if hashlib.md5(result).digest() != record["result_md5"]:
            raise ValueError(f"Result resource mismatch: {record['path']}")
        print(f"Verified {record['path']}: {len(source)} -> {len(result)} bytes")
    print(f"Verified {len(records)} resource deltas for {original_path}")


def main():
    parser = argparse.ArgumentParser(description="Verify binary resource deltas for Brotato")
    parser.add_argument("original", type=Path)
    parser.add_argument("delta", type=Path)
    args = parser.parse_args()
    verify_delta(args.original, args.delta)


if __name__ == "__main__":
    main()
