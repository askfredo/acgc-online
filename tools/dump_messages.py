#!/usr/bin/env python3
"""Dump game dialogue messages from your Animal Crossing (GAFE01) disc image.

Reads forest_*.arc from the disc, finds message_data.bin and its table, and prints
messages as text with control codes in msg_tool.py syntax (<<CODE [args]>>).

    tools/dump_messages.py rom/GAFE01.iso 0x9C7 0x9E0     # a range of message numbers
"""
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
import msg_tool  # noqa: E402


def be32(b, o):
    return struct.unpack_from(">I", b, o)[0]


def be16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def read_fst(iso):
    iso.seek(0x424)
    fst_off, fst_size = struct.unpack(">II", iso.read(8))
    iso.seek(fst_off)
    fst = iso.read(fst_size)
    count = be32(fst, 8)
    names = fst[count * 12:]
    files = {}

    def name_at(o):
        end = names.index(b"\0", o)
        return names[o:end].decode("latin-1")

    for i in range(1, count):
        e = fst[i * 12:i * 12 + 12]
        is_dir = e[0]
        name = name_at(be32(e, 0) & 0xFFFFFF)
        if not is_dir:
            files[name] = (be32(e, 4), be32(e, 8))
    return files


def yaz0_decode(src):
    if src[:4] != b"Yaz0":
        return src
    size = be32(src, 4)
    dst = bytearray()
    i = 16
    while len(dst) < size:
        header = src[i]
        i += 1
        for bit in range(7, -1, -1):
            if len(dst) >= size:
                break
            if header & (1 << bit):
                dst.append(src[i])
                i += 1
            else:
                b1, b2 = src[i], src[i + 1]
                i += 2
                dist = ((b1 & 0xF) << 8 | b2) + 1
                n = b1 >> 4
                if n == 0:
                    n = src[i] + 0x12
                    i += 1
                else:
                    n += 2
                start = len(dst) - dist
                for k in range(n):
                    dst.append(dst[start + k])
    return bytes(dst)


def rarc_files(arc):
    """Returns {name: bytes} for every file in a RARC archive."""
    if arc[:4] != b"RARC":
        raise ValueError("not a RARC archive")
    data_off = be32(arc, 0xC) + 0x20
    info = 0x20
    num_entries = be32(arc, info + 0x8)
    entries_off = be32(arc, info + 0xC) + 0x20
    strings_off = be32(arc, info + 0x14) + 0x20
    out = {}
    for i in range(num_entries):
        e = entries_off + i * 20
        attr = arc[e + 4]
        name_off = be16(arc, e + 6)
        off = be32(arc, e + 8)
        size = be32(arc, e + 12)
        end = arc.index(b"\0", strings_off + name_off)
        name = arc[strings_off + name_off:end].decode("latin-1")
        if attr & 0x01 and name not in (".", ".."):
            out[name] = arc[data_off + off:data_off + off + size]
    return out


def find_messages(iso_path):
    with open(iso_path, "rb") as iso:
        files = read_fst(iso)
        for arc_name in ("forest_1st.arc", "forest_2nd.arc"):
            if arc_name not in files:
                continue
            off, size = files[arc_name]
            iso.seek(off)
            arc = rarc_files(yaz0_decode(iso.read(size)))
            if "message_data.bin" in arc:
                return yaz0_decode(arc["message_data.bin"]), yaz0_decode(arc["message_data_table.bin"])
    raise SystemExit("message_data.bin not found in the disc image")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    data, table = find_messages(sys.argv[1])
    first = int(sys.argv[2], 0)
    last = int(sys.argv[3], 0) if len(sys.argv) > 3 else first
    for idx in range(first, last + 1):
        end = be32(table, idx * 4)
        start = be32(table, (idx - 1) * 4) if idx > 0 else 0
        if end == 0 or end < start:
            continue
        sys.stdout.write(msg_tool.decode_entry(bytearray(data[start:end]), 0, end - start, idx))


if __name__ == "__main__":
    main()
