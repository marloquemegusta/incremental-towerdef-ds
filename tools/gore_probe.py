#!/usr/bin/env python3
"""Gore pixel probe for towerds DeSmuME captures (plain RGB8 PNGs, filter 0).

Used to debug blood persistence: it counts gore pixels in a region across a
capture sequence, so you can see whether a puddle stays or gets wiped.

Usage:
  python3 tools/gore_probe.py region  <dir> <glob> x0 y0 x1 y1
  python3 tools/gore_probe.py count   <dir> <glob>
  python3 tools/gore_probe.py bbox    <a.png> <b.png> [y_min]
  python3 tools/gore_probe.py scan    <dir> <glob>

Notes:
  - Coordinates are in the stitched 256x384 screenshot space
    (top screen rows 0..191, bottom screen rows 192..383).
  - Gore mask: COLOR_XENOS_GORE_MID/DARK. Bright xeno purple (enemies) has
    green ~ 0, so it is excluded.
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


# Blood is now a RED palette (arterial -> dried), so it can never be confused with
# the purple xeno swarm. Caveat: the HUD health bars and xeno eyes are also red, so
# keep measured regions away from UI and expect a few stray pixels near enemies.
def is_gore(r, g, b):
    return r >= 130 and g <= 45 and b <= 70 and r >= g + 70


def is_gore_dark(r, g, b):
    return 40 <= r < 130 and g <= 32 and b <= 45 and r >= g + 25


def is_blood(r, g, b):
    return is_gore(r, g, b) or is_gore_dark(r, g, b)


def cmd_region(d, pattern, x0, y0, x1, y1):
    for path in sorted(glob.glob(os.path.join(d, pattern))):
        rgb = read_png(path)
        n = 0
        for y in range(y0, y1):
            base = y * WIDTH * 3
            for x in range(x0, x1):
                i = base + x * 3
                if is_blood(rgb[i], rgb[i + 1], rgb[i + 2]):
                    n += 1
        print(f"{os.path.basename(path)},{n}")


def cmd_count(d, pattern):
    print("frame,mid,dark")
    for path in sorted(glob.glob(os.path.join(d, pattern))):
        rgb = read_png(path)
        mid = dark = 0
        for i in range(0, len(rgb), 3):
            if is_gore(rgb[i], rgb[i + 1], rgb[i + 2]):
                mid += 1
            elif is_gore_dark(rgb[i], rgb[i + 1], rgb[i + 2]):
                dark += 1
        print(f"{os.path.basename(path)},{mid},{dark}")


def cmd_bbox(a, b, y_min=0):
    ra, rb = read_png(a), read_png(b)
    minx, miny, maxx, maxy, n = WIDTH, HEIGHT, -1, -1, 0
    for y in range(y_min, HEIGHT):
        for x in range(WIDTH):
            i = (y * WIDTH + x) * 3
            if is_blood(rb[i], rb[i + 1], rb[i + 2]) and not is_blood(ra[i], ra[i + 1], ra[i + 2]):
                n += 1
                minx = min(minx, x); maxx = max(maxx, x)
                miny = min(miny, y); maxy = max(maxy, y)
    print(f"n={n} bbox=({minx},{miny})-({maxx},{maxy})")


def cmd_scan(d, pattern):
    files = sorted(glob.glob(os.path.join(d, pattern)))
    prev = read_png(files[0])
    for path in files[1:]:
        cur = read_png(path)
        n = 0
        minx, miny, maxx, maxy = WIDTH, HEIGHT, -1, -1
        for y in range(200, HEIGHT):
            for x in range(WIDTH):
                i = (y * WIDTH + x) * 3
                if is_blood(cur[i], cur[i + 1], cur[i + 2]) and not is_blood(prev[i], prev[i + 1], prev[i + 2]):
                    n += 1
                    minx = min(minx, x); maxx = max(maxx, x)
                    miny = min(miny, y); maxy = max(maxy, y)
        if n >= 5:
            print(f"{os.path.basename(path)}: n={n} bbox=({minx},{miny})-({maxx},{maxy})")
        prev = cur


if __name__ == "__main__":
    mode = sys.argv[1]
    if mode == "region":
        cmd_region(sys.argv[2], sys.argv[3], *(int(v) for v in sys.argv[4:8]))
    elif mode == "count":
        cmd_count(sys.argv[2], sys.argv[3])
    elif mode == "bbox":
        cmd_bbox(sys.argv[2], sys.argv[3], int(sys.argv[4]) if len(sys.argv) > 4 else 0)
    elif mode == "scan":
        cmd_scan(sys.argv[2], sys.argv[3])
    else:
        print(__doc__)
        sys.exit(2)
