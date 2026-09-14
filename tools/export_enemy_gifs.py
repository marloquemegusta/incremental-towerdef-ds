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
