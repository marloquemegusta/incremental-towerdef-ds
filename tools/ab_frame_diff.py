#!/usr/bin/env python3
"""A/B capture differ for towerds DeSmuME evidence.

Two modes, both reporting changed pixel counts and the bounding box of the
change in the stitched 256x384 screenshot space (top screen rows 0..191,
bottom screen rows 192..383):

  seq  <dirA> <dirB> <glob>
      Frame-by-frame diff of two capture directories (matching file names).
      Isolates WHICH on-screen region a code change moves.

  pair <fileA> <fileB> [x0 y0 x1 y1]
      Diff two arbitrary captures, optionally restricted to a rectangle.
      Used to test a claim like "the dome returns to its idle pose" without
      unrelated on-screen motion (enemies, HUD) contaminating the count.
"""
import glob
import os
import struct
import sys
import zlib

WIDTH = 256
HEIGHT = 384


def read_png(path):
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos = 8
    idat = b""
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + ln]
        if kind == b"IDAT":
            idat += chunk
        pos += 12 + ln
    raw = zlib.decompress(idat)
    stride = WIDTH * 3
    out = []
    for y in range(HEIGHT):
        off = y * (stride + 1)
        out.append(raw[off + 1:off + 1 + stride])
    return b"".join(out)


def diff_rect(a, b, x0=0, y0=0, x1=WIDTH, y1=HEIGHT):
    n = 0
    minx, miny, maxx, maxy = WIDTH, HEIGHT, -1, -1
    for y in range(y0, y1):
        base = y * WIDTH * 3
        for x in range(x0, x1):
            i = base + x * 3
            if a[i] != b[i] or a[i + 1] != b[i + 1] or a[i + 2] != b[i + 2]:
                n += 1
                if x < minx:
                    minx = x
                if x > maxx:
                    maxx = x
                if y < miny:
                    miny = y
                if y > maxy:
                    maxy = y
    bbox = f"({minx},{miny})-({maxx},{maxy})" if n else "-"
    return n, bbox


def cmd_seq(dir_a, dir_b, pattern):
    names = sorted(os.path.basename(p) for p in glob.glob(os.path.join(dir_a, pattern)))
    print("frame,changed,bbox")
    for name in names:
        pb = os.path.join(dir_b, name)
        if not os.path.exists(pb):
            continue
        n, bbox = diff_rect(read_png(os.path.join(dir_a, name)), read_png(pb))
        print(f"{name},{n},{bbox}")


def cmd_pair(file_a, file_b, rect):
    n, bbox = diff_rect(read_png(file_a), read_png(file_b), *rect)
    print(f"{os.path.basename(file_a)} vs {os.path.basename(file_b)}: changed={n} bbox={bbox}")


if __name__ == "__main__":
    mode = sys.argv[1]
    if mode == "seq":
        cmd_seq(sys.argv[2], sys.argv[3], sys.argv[4])
    elif mode == "pair":
        box = [int(v) for v in sys.argv[4:8]] if len(sys.argv) > 7 else [0, 0, WIDTH, HEIGHT]
        cmd_pair(sys.argv[2], sys.argv[3], box)
    else:
        print(__doc__)
        sys.exit(2)
