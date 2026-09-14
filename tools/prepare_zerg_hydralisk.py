from pathlib import Path
from PIL import Image

SOURCE = Path("assets/sprites/enemies/zerg_hydralisk_source.png")
DEST = Path("assets/sprites/enemies/t2_hormagaunt_strip_master_1x.png")
MASTER = Path("assets/sprites/enemies/t2_hydralisk_walk_strip_master_1x.png")

# The upper 7x9 grid is the Hydralisk walk cycle: 9 directions, 7 poses.
# Use four poses from the first orientation for the current DS enemy format.
im = Image.open(SOURCE).convert("RGBA")
cell_width = 42
cell_height = 55
column_pitch = 45
row_pitch = 58

master = Image.new("RGBA", (cell_width * 7, cell_height * 9), (0, 0, 0, 0))
for direction in range(9):
    for pose in range(7):
        cell = im.crop((2 + pose * column_pitch, 2 + direction * row_pitch,
                        2 + pose * column_pitch + cell_width,
                        2 + direction * row_pitch + cell_height))
        pixels = cell.load()
        for y in range(cell.height):
            for x in range(cell.width):
                r, g, b, a = pixels[x, y]
                if abs(r - g) < 8 and abs(g - b) < 8 and 80 <= r <= 180:
                    pixels[x, y] = (0, 0, 0, 0)
        master.alpha_composite(cell, (pose * cell_width, direction * cell_height))
MASTER.parent.mkdir(parents=True, exist_ok=True)
master.save(MASTER)

frames = []
for column in (0, 2, 4, 6):
    cell = master.crop((column * cell_width, 0,
                        (column + 1) * cell_width, cell_height))
    cell = cell.resize((24, 24), Image.Resampling.NEAREST)
    frames.append(cell)

out = Image.new("RGBA", (24 * len(frames), 24), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    out.alpha_composite(frame, (i * 24, 0))
DEST.parent.mkdir(parents=True, exist_ok=True)
out.save(DEST)
print(f"wrote {DEST} ({out.width}x{out.height})")
