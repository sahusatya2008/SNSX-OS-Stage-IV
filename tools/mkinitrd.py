#!/usr/bin/env python3

import os
import struct
import sys

INITRD_MAGIC = 0x4D594653
INITRD_VERSION = 1
ENTRY_STRUCT = struct.Struct("<32sIII")
HEADER_STRUCT = struct.Struct("<III")


def gather_files(root_dir):
    files = []
    for dirpath, dirnames, filenames in os.walk(root_dir):
        dirnames.sort()
        filenames.sort()
        for filename in filenames:
            full_path = os.path.join(dirpath, filename)
            relative_path = os.path.relpath(full_path, root_dir).replace(os.sep, "/")
            with open(full_path, "rb") as handle:
                files.append((relative_path, handle.read()))
    return files


def entry_type(relative_path):
    return 2 if relative_path.endswith(".app") else 1


def main():
    if len(sys.argv) != 3:
        print("usage: mkinitrd.py OUTPUT_IMAGE ROOTFS_DIR", file=sys.stderr)
        return 1

    output_path = sys.argv[1]
    root_dir = sys.argv[2]
    files = gather_files(root_dir)

    header = HEADER_STRUCT.pack(INITRD_MAGIC, INITRD_VERSION, len(files))
    offset = HEADER_STRUCT.size + ENTRY_STRUCT.size * len(files)
    entries = []
    payload = bytearray()

    for relative_path, content in files:
        encoded_name = relative_path.encode("ascii")
        if len(encoded_name) >= 32:
            raise SystemExit(f"path too long for initrd entry: {relative_path}")
        entries.append(
            ENTRY_STRUCT.pack(
                encoded_name.ljust(32, b"\0"),
                offset,
                len(content),
                entry_type(relative_path),
            )
        )
        payload.extend(content)
        offset += len(content)

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as handle:
        handle.write(header)
        for entry in entries:
            handle.write(entry)
        handle.write(payload)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
