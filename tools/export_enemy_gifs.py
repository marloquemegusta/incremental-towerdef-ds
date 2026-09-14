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

# Hydralisk walk-cycle collage: 9 directions x 7 poses, synchronized.
source = Image.open(SOURCE_DIR / "t2_hydralisk_walk_strip_master_1x.png").convert("RGBA")
cell_width = 42
cell_height = 55
display_size = 32
collage_frames = []
for pose in range(7):
    collage = Image.new("RGBA", (display_size * 9, display_size * 9), (96, 96, 96, 255))
    for direction in range(9):
        cell = source.crop((pose * cell_width, direction * cell_height,
                            (pose + 1) * cell_width,
                            (direction + 1) * cell_height))
        cell = cell.resize((display_size, display_size), Image.Resampling.NEAREST)
        collage.alpha_composite(cell, ((direction % 3) * display_size,
                                       (direction // 3) * display_size))
    collage_frames.append(collage)

collage_frames[0].save(DEST_DIR / "t2_hydralisk_walk_views.gif",
                        save_all=True, append_images=collage_frames[1:],
                        duration=140, loop=0, disposal=2)
print(f"wrote {DEST_DIR / 't2_hydralisk_walk_views.gif'} (9 views, 7 poses)")
