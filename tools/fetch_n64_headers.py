#!/usr/bin/env python3
"""Fetch the N64 SDK headers the PC port needs from decompals/ultralib (same files and
patch as configure.py), without running the full decomp configure."""
import os
import re
import sys
import urllib.request

FILES = [
    "include/PR/abi.h",
    "include/PR/gbi.h",
    "include/PR/gs2dex.h",
    "include/PR/mbi.h",
    "include/PR/ultratypes.h",
    "include/compiler/gcc/stdlib.h",
]
URL = "https://raw.githubusercontent.com/decompals/ultralib/main/{}"


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    for path in FILES:
        local = os.path.join(root, "include", path.split("include/", 1)[1])
        if os.path.exists(local):
            continue
        os.makedirs(os.path.dirname(local), exist_ok=True)
        req = urllib.request.Request(URL.format(path), headers={"User-Agent": "acgc-online"})
        data = urllib.request.urlopen(req).read()
        if path.endswith("PR/gbi.h"):
            data = re.sub(rb"unsigned char\s+param:8;", b"unsigned int\tparam:8;", data)
        with open(local, "wb") as f:
            f.write(data)
        print("fetched", path)


if __name__ == "__main__":
    sys.exit(main())
