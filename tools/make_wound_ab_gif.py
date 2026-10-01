#!/usr/bin/env python3
"""Side-by-side A/B GIF built from two towerds DeSmuME capture sequences.

Both sequences are cropped and scaled with the same rectangle and frame stride, then
laid out as [ A | B ] per frame, so a single GIF shows two behaviours side by side.

Usage:
  python3 tools/make_wound_ab_gif.py out.gif x0 y0 x1 y1 scale duration_ms step \
      dirA globA dirB globB
"""
import glob
import os
import sys

from PIL import Image

GAP = 3


def main():
    out = sys.argv[1]
    x0, y0, x1, y1, scale, duration, step = (int(v) for v in sys.argv[2:9])
    dir_a, glob_a, dir_b, glob_b = sys.argv[9], sys.argv[10], sys.argv[11], sys.argv[12]

    paths_a = sorted(glob.glob(os.path.join(dir_a, glob_a)))[::step]
    paths_b = sorted(glob.glob(os.path.join(dir_b, glob_b)))[::step]
    count = min(len(paths_a), len(paths_b))
    if count == 0:
        raise SystemExit("no captures matched")

    cw, ch = x1 - x0, y1 - y0
    pw, ph = cw * scale, ch * scale
    frames = []
    for i in range(count):
        canvas = Image.new("RGB", (pw * 2 + GAP, ph), (24, 24, 28))
        for idx, path in enumerate((paths_a[i], paths_b[i])):
            tile = Image.open(path).convert("RGB").crop((x0, y0, x1, y1))
            canvas.paste(tile.resize((pw, ph), Image.Resampling.NEAREST), (idx * (pw + GAP), 0))
        frames.append(canvas)

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
    print(f"wrote {out} ({pw * 2 + GAP}x{ph}, {count} frames, {size_kb:.0f} KB)")


main()
