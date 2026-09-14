from pathlib import Path
from PIL import Image, ImageDraw

OUT = Path('artifacts/sprite-review')
OUT.mkdir(parents=True, exist_ok=True)

SHEETS = [
    # Exact walking blocks, excluding palettes, labels, effects and lower units.
    ('source_a', Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-3e4e02cb-6c72-4612-b90e-dcfa92da1a9f.png'), (0, 0, 408, 640), 8, 8),
    ('source_b', Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-69a5cb75-aca0-44d4-af8a-ae0317fd79d2.png'), (0, 0, 378, 504), 9, 12),
    ('source_c', Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-a664218f-1464-40d2-bb60-30720a8c83fe.png'), (0, 0, 504, 532), 8, 8),
]

def frame(im, box, scale=4):
    f = im.crop(box)
    return f.resize((f.width * scale, f.height * scale), Image.Resampling.NEAREST)

for name, path, region, directions, frames in SHEETS:
    im = Image.open(path).convert('RGBA').crop(region)
    cw, ch = im.width // frames, im.height // directions
    # Source sheets are laid out as directions in columns and walk frames in rows.
    cells = [[frame(im, (d*cw, f*ch, (d+1)*cw, (f+1)*ch)) for f in range(frames)] for d in range(directions)]
    for d, seq in enumerate(cells):
        seq[0].save(OUT / f'v2_{name}_dir{d}_4x.gif', save_all=True, append_images=seq[1:], duration=120, loop=0, disposal=2, transparency=0)
    gap = 8
    collage = Image.new('RGBA', (frames * cells[0][0].width + (frames+1)*gap, directions * cells[0][0].height + (directions+1)*gap), (24,24,24,255))
    draw = ImageDraw.Draw(collage)
    for d, seq in enumerate(cells):
        for f, cell in enumerate(seq):
            x = gap + f * (cell.width + gap)
            y = gap + d * (cell.height + gap)
            collage.alpha_composite(cell, (x, y))
            draw.text((x+3, y+3), f'{d}:{f}', fill=(255,255,255,220))
    collage.save(OUT / f'v2_{name}_walking_grid_4x.png')
    # Also preserve the exact proposed canonical sheet, unscaled.
    im.save(OUT / f'{name}_candidate_source_crop.png')
