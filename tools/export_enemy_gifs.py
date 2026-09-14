from pathlib import Path
from PIL import Image

SOURCE_DIR = Path("assets/sprites/enemies")
DEST_DIR = Path("artifacts/enemy-gifs")
ENEMIES = [
    ("t0_larva", 4),
    ("t1_ripper", 4),
    ("t2_hormagaunt", 4),
    ("t3_ravener", 4),
    ("t4_carnifex", 4),
    ("t5_hierophant", 4),
]

DEST_DIR.mkdir(parents=True, exist_ok=True)
for name, frame_count in ENEMIES:
    source = SOURCE_DIR / f"{name}_strip_master_1x.png"
    strip = Image.open(source).convert("RGBA")
    frame_width = strip.width // frame_count
    frames = [strip.crop((i * frame_width, 0, (i + 1) * frame_width, strip.height))
              for i in range(frame_count)]
    frames[0].save(
        DEST_DIR / f"{name}.gif",
        save_all=True,
        append_images=frames[1:],
        duration=120,
        loop=0,
        disposal=2,
        transparency=0,
    )
    print(f"wrote {DEST_DIR / f'{name}.gif'} ({frame_width}x{strip.height}, {frame_count} frames)")

# Hydralisk walk-cycle previews: one looping GIF per direction.
source = Image.open(SOURCE_DIR / "t2_hydralisk_walk_strip_master_1x.png").convert("RGBA")
cell_width = 42
cell_height = 55
scale = 4
for direction in range(9):
    frames = []
    for pose in range(7):
        frames.append(source.crop((pose * cell_width, direction * cell_height,
                                   (pose + 1) * cell_width,
                                   (direction + 1) * cell_height)))

    # Use the union of all non-transparent pixels so every frame shares a
    # tight, stable canvas without gray extraction-sheet margins.
    bbox = None
    for frame in frames:
        current = frame.getbbox()
        if current:
            bbox = current if bbox is None else (
                min(bbox[0], current[0]), min(bbox[1], current[1]),
                max(bbox[2], current[2]), max(bbox[3], current[3]))
    if bbox is None:
        bbox = (0, 0, cell_width, cell_height)
    frames = [frame.crop(bbox).resize(
        ((bbox[2] - bbox[0]) * scale, (bbox[3] - bbox[1]) * scale),
        Image.Resampling.NEAREST) for frame in frames]
    frames[0].save(DEST_DIR / f"t2_hydralisk_walk_dir{direction}_4x.gif",
                    save_all=True, append_images=frames[1:],
                    duration=140, loop=0, disposal=2, transparency=0)
    print(f"wrote direction {direction} ({frames[0].width}x{frames[0].height}, 7 poses)")

# One animated 3x3 table: each square keeps one direction fixed while all
# nine squares advance through their own seven-frame walk cycle together.
table_frames = []
table_cell_width = cell_width * scale
table_cell_height = cell_height * scale
for pose in range(7):
    table = Image.new("RGBA", (table_cell_width * 3, table_cell_height * 3), (0, 0, 0, 0))
    for direction in range(9):
        cell = source.crop((pose * cell_width, direction * cell_height,
                            (pose + 1) * cell_width,
                            (direction + 1) * cell_height))
        cell = cell.resize((table_cell_width, table_cell_height), Image.Resampling.NEAREST)
        table.alpha_composite(cell, ((direction % 3) * table_cell_width,
                                     (direction // 3) * table_cell_height))
    table_frames.append(table)

table_frames[0].save(DEST_DIR / "t2_hydralisk_walk_all_views_4x.gif",
                     save_all=True, append_images=table_frames[1:],
                     duration=140, loop=0, disposal=2, transparency=0)
print(f"wrote {DEST_DIR / 't2_hydralisk_walk_all_views_4x.gif'} (3x3 table, 7 poses)")
