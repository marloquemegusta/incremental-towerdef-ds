from pathlib import Path
from PIL import Image

SOURCE = Path("assets/sprites/enemies/zerg_hydralisk_source.png")
DEST = Path("assets/sprites/enemies/t2_hormagaunt_strip_master_1x.png")

# The upper 7x9 grid is the Hydralisk walk cycle: 9 directions, 7 poses.
# Use four poses from the first orientation for the current DS enemy format.
im = Image.open(SOURCE).convert("RGBA")
frames = []
for column in (0, 2, 4, 6):
    cell = im.crop((1 + column * 45, 1, 1 + (column + 1) * 45, 46))
    cell = cell.resize((24, 24), Image.Resampling.NEAREST)
    pixels = cell.load()
    for y in range(cell.height):
        for x in range(cell.width):
            r, g, b, a = pixels[x, y]
            if 115 <= r <= 245 and 115 <= g <= 245 and 115 <= b <= 245 and abs(r - g) < 8 and abs(g - b) < 8:
                pixels[x, y] = (0, 0, 0, 0)
    frames.append(cell)

out = Image.new("RGBA", (24 * len(frames), 24), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    out.alpha_composite(frame, (i * 24, 0))
DEST.parent.mkdir(parents=True, exist_ok=True)
out.save(DEST)
print(f"wrote {DEST} ({out.width}x{out.height})")
