#!/usr/bin/env python3
"""Crop/scale/compose towerds captures into a single comparison PNG.

Reads plain RGB8 filter-0 PNGs (what the DeSmuME runner writes), crops a
rectangle in the stitched 256x384 space, upscales it with nearest-neighbour
(so hard pixel edges stay crisp) and writes a PNG.

Usage:
  python3 tools/make_ab_image.py out.png x0 y0 x1 y1 scale imgA imgB imgC imgD

Images are laid out in reading order (A B / C D). Any of B..D may be "-" to
leave that tile blank.
"""
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


def write_png(path, w, h, rgb):
    raw = b"".join(b"\x00" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    open(path, "wb").write(png)


def main():
    out, x0, y0, x1, y1, scale = sys.argv[1], *[int(v) for v in sys.argv[2:7]]
    srcs = sys.argv[7:11]
    cw, ch = (x1 - x0), (y1 - y0)
    tw, th = cw * scale, ch * scale
    comp_w, comp_h = tw * 2, th * 2
    canvas = bytearray(b"\x20" * (comp_w * comp_h * 3))
    for idx, src in enumerate(srcs):
        if src == "-":
            continue
        rgb = read_png(src)
        ox, oy = (idx % 2) * tw, (idx // 2) * th
        for y in range(ch):
            for x in range(cw):
                i = ((y0 + y) * WIDTH + (x0 + x)) * 3
                px = rgb[i:i + 3]
                for dy in range(scale):
                    base = ((oy + y * scale + dy) * comp_w + ox + x * scale) * 3
                    for dx in range(scale):
                        canvas[base + dx * 3:base + dx * 3 + 3] = px
    write_png(out, comp_w, comp_h, bytes(canvas))
    print(f"wrote {out} ({comp_w}x{comp_h})")


if __name__ == "__main__":
    main()
