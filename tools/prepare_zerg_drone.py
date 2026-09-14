from pathlib import Path
from PIL import Image

SOURCE = Path("assets/sprites/enemies/zerg_drone_source.png")
DEST = Path("assets/sprites/enemies/t1_ripper_strip_master_1x.png")

# The first Walk row is six evenly spaced 55x45 cells. Keep four evenly
# spaced poses so the existing DS enemy format can display it immediately.
im = Image.open(SOURCE).convert("RGBA")
frames = []
for index in (0, 1, 3, 5):
    cell = im.crop((87 + index * 55, 3, 87 + (index + 1) * 55, 48))
    cell = cell.resize((28, 23), Image.Resampling.NEAREST)
    pixels = cell.load()
    for y in range(cell.height):
        for x in range(cell.width):
            r, g, b, a = pixels[x, y]
            if g > 180 and r < 80 and b < 80:
                pixels[x, y] = (0, 0, 0, 0)
    frames.append(cell)

out = Image.new("RGBA", (28 * len(frames), 23), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    out.alpha_composite(frame, (i * 28, 0))
DEST.parent.mkdir(parents=True, exist_ok=True)
out.save(DEST)
print(f"wrote {DEST} ({out.width}x{out.height})")
