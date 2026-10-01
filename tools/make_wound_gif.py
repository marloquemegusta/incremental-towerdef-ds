#!/usr/bin/env python3
"""Build an animated GIF from a towerds DeSmuME capture sequence.

The captures are stitched 256x384 RGB PNGs (top screen rows 0..191, bottom
192..383). This crops a rectangle, upscales it with nearest neighbour so the
hard pixel edges stay crisp, and writes a looping GIF.

Usage:
  python3 tools/make_wound_gif.py out.gif x0 y0 x1 y1 scale duration_ms step dir glob
"""
import glob
import os
import sys

from PIL import Image

WIDTH, HEIGHT = 256, 384


def main():
    out = sys.argv[1]
    x0, y0, x1, y1, scale, duration, step = (int(v) for v in sys.argv[2:9])
    directory, pattern = sys.argv[9], sys.argv[10]

    paths = sorted(glob.glob(os.path.join(directory, pattern)))
    if not paths:
        raise SystemExit(f"no captures matched {directory}/{pattern}")

    cw, ch = x1 - x0, y1 - y0
    frames = []
    for path in paths[::step]:
        img = Image.open(path).convert("RGB").crop((x0, y0, x1, y1))
        frames.append(img.resize((cw * scale, ch * scale), Image.Resampling.NEAREST))

    frames[0].save(
        out,
        save_all=True,
        append_images=frames[1:],
        duration=duration,
        loop=0,
        disposal=2,
        optimize=True,
    )
    size_kb = os.path.getsize(out) / 1024
    print(f"wrote {out} ({cw * scale}x{ch * scale}, {len(frames)} frames, {size_kb:.0f} KB)")


main()
