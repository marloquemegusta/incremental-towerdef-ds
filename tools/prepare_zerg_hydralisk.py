from pathlib import Path
from PIL import Image

SOURCE = Path("assets/sprites/enemies/zerg_hydralisk_source.png")
DEST = Path("assets/sprites/enemies/t2_hormagaunt_strip_master_1x.png")
MASTER = Path("assets/sprites/enemies/t2_hydralisk_walk_strip_master_1x.png")

# The source has 7 animation rows x 9 source directions. The canonical master
# uses 8 rows in the shared game order: N, NE, E, SE, S, SW, W, NW.
# The source's ninth column is the front/down pose; the source order is not
# the order used by the game, so normalize it here once and for all.
CANONICAL_SOURCE_DIRECTIONS = (0, 1, 2, 3, 8, 7, 6, 5)
im = Image.open(SOURCE).convert("RGBA")
cell_width = 42
cell_height = 55
column_pitch = 45
row_pitch = 58

master = Image.new("RGBA", (cell_width * 7, cell_height * 8), (0, 0, 0, 0))
for canonical_direction, source_direction in enumerate(CANONICAL_SOURCE_DIRECTIONS):
    for pose in range(7):
        # Source layout is 7 animation rows x 9 directions.
        cell = im.crop((2 + source_direction * column_pitch, 2 + pose * row_pitch,
                        2 + source_direction * column_pitch + cell_width,
                        2 + pose * row_pitch + cell_height))
        pixels = cell.load()
        for y in range(cell.height):
            for x in range(cell.width):
                r, g, b, a = pixels[x, y]
                if abs(r - g) < 8 and abs(g - b) < 8 and 80 <= r <= 180:
                    pixels[x, y] = (0, 0, 0, 0)
        master.alpha_composite(cell, (pose * cell_width, canonical_direction * cell_height))
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
