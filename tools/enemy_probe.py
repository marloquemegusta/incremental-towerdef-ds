#!/usr/bin/env python3
"""Locate enemy (xeno purple) blobs in towerds DeSmuME captures.

Reports, per frame, the bounding box and centroid of purple swarm pixels in the
BOTTOM screen (stitched rows 192..383), so a scenario can be aimed at a real
on-screen enemy instead of guessing coordinates.

Purple is reserved for the swarm (DESIGN.md), so the ground/HUD do not match.
Flying units are drawn lifted, so the box is where the sprite actually is.

Usage:
  python3 tools/enemy_probe.py <dir> <glob>
"""
import glob
import os
import struct
import sys
import zlib

WIDTH = 256
HEIGHT = 384
BOTTOM_Y0 = 192


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


def is_purple(r, g, b):
    return b >= 120 and b - g >= 60 and b >= r and g < 110


def main():
    d, pattern = sys.argv[1], sys.argv[2]
    print("frame,count,cx,cy,y0,y1")
    for path in sorted(glob.glob(os.path.join(d, pattern))):
        rgb = read_png(path)
        n = 0
        sx = sy = 0
        y0, y1 = HEIGHT, -1
        for y in range(BOTTOM_Y0, HEIGHT):
            base = y * WIDTH * 3
            for x in range(WIDTH):
                i = base + x * 3
                if is_purple(rgb[i], rgb[i + 1], rgb[i + 2]):
                    n += 1
                    sx += x
                    sy += y
                    if y < y0:
                        y0 = y
                    if y > y1:
                        y1 = y
        if n:
            print(f"{os.path.basename(path)},{n},{sx // n},{sy // n},{y0},{y1}")
        else:
            print(f"{os.path.basename(path)},0,-,-,-,-")


if __name__ == "__main__":
    main()
