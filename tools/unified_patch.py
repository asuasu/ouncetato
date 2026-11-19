from pathlib import Path
import re


def apply_patch(target, patch_file):
    target = Path(target)
    patch_file = Path(patch_file)
    original = target.read_text().splitlines(True)
    patch_lines = patch_file.read_text().splitlines(True)
    output = []
    cursor = 0
    index = 0

    while index < len(patch_lines):
        line = patch_lines[index]
        match = re.match(r"@@ -(\d+)(?:,(\d+))? \+\d+(?:,(\d+))? @@", line)
        if not match:
            index += 1
            continue
        old_start = int(match.group(1))
        old_index = old_start - 1
        if old_index < cursor or old_index > len(original):
            raise ValueError(f"Invalid hunk position in {patch_file}: {line.strip()}")
        output.extend(original[cursor:old_index])
        cursor = old_index
        index += 1

        while index < len(patch_lines):
            line = patch_lines[index]
            if line.startswith("@@ ") or line.startswith("diff --git "):
                break
            prefix = line[:1]
            if prefix == "\\":
                index += 1
                continue
            if prefix not in (" ", "+", "-"):
                raise ValueError(f"Unexpected patch line in {patch_file}: {line!r}")
            if prefix == "+":
                output.append(line[1:])
            else:
                if cursor >= len(original) or original[cursor].rstrip("\r\n") != line[1:].rstrip("\r\n"):
                    raise ValueError(
                        f"Patch context mismatch in {patch_file} at {target}:{cursor + 1}\n"
                        f"Expected: {line[1:]!r}\nActual: {original[cursor]!r}"
                    )
                if prefix == " ":
                    output.append(original[cursor])
                cursor += 1
            index += 1

    output.extend(original[cursor:])
    target.write_text("".join(output))
