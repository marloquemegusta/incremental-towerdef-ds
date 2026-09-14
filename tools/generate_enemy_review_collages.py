import os
from pathlib import Path
from PIL import Image, ImageDraw

art_dir = Path(r"C:\Users\malfonso\.gemini\antigravity\brain\1b7c5296-7dda-4d22-941e-637e53321ae1")
out_dir = art_dir / "collages"
out_dir.mkdir(parents=True, exist_ok=True)

# 8 Compass directions mapped to 9-column SC1 half-circle:
# SC1 columns: 0=N, 1=NNE, 2=NE, 3=ENE, 4=E, 5=ESE, 6=SE, 7=SSE, 8=S
# For 8 standard compass directions:
# 0 (N):  Col 0
# 1 (NE): Col 2
# 2 (E):  Col 4
# 3 (SE): Col 6
# 4 (S):  Col 8
# 5 (SW): Col 6 flipped H
# 6 (W):  Col 4 flipped H
# 7 (NW): Col 2 flipped H

DIR_MAP = [
    (0, False, "N"),
    (2, False, "NE"),
    (4, False, "E"),
    (6, False, "SE"),
    (8, False, "S"),
    (6, True,  "SW"),
    (4, True,  "W"),
    (2, True,  "NW"),
]

GRID_POS = {
    "NW": (0, 0),
    "N":  (1, 0),
    "NE": (2, 0),
    "W":  (0, 1),
    "E":  (2, 1),
    "SW": (0, 2),
    "S":  (1, 2),
    "SE": (2, 2),
}

def make_collage(source_img, name, action_title, row_indices, col_coords, cell_w, cell_h, frame_duration=130, scale=2):
    frames = []
    num_poses = len(row_indices)
    
    pad = 4
    cw = cell_w * scale
    ch = cell_h * scale
    total_w = 3 * cw + 4 * pad
    total_h = 3 * ch + 4 * pad
    
    for f_idx in range(num_poses):
        row_y = row_indices[f_idx]
        collage = Image.new("RGBA", (total_w, total_h), (20, 24, 30, 255))
        draw = ImageDraw.Draw(collage)
        
        for sc_col, flip, dir_name in DIR_MAP:
            gx, gy = GRID_POS[dir_name]
            px = pad + gx * (cw + pad)
            py = pad + gy * (ch + pad)
            
            draw.rectangle([px, py, px + cw - 1, py + ch - 1], fill=(28, 33, 42, 255), outline=(55, 65, 80, 255))
            
            x1, x2 = col_coords[sc_col]
            cell = source_img.crop((x1, row_y[0], x2, row_y[1]))
            if flip:
                cell = cell.transpose(Image.FLIP_LEFT_RIGHT)
            cell_scaled = cell.resize((cw, ch), Image.Resampling.NEAREST)
            collage.alpha_composite(cell_scaled, (px, py))
            
            draw.text((px + 5, py + 3), dir_name, fill=(220, 230, 245, 230))
            
        cx = pad + 1 * (cw + pad)
        cy = pad + 1 * (ch + pad)
        draw.rectangle([cx, cy, cx + cw - 1, cy + ch - 1], fill=(14, 18, 24, 255), outline=(0, 175, 240, 255), width=2)
        
        act_clean = action_title.upper()
        center_text = f"{name.upper()}\n{act_clean}\n[{f_idx + 1}/{num_poses}]"
        draw.multiline_text((cx + 10, cy + ch // 3 - 6), center_text, fill=(0, 220, 255, 255), align="center")
        
        frames.append(collage)
        
    slug = action_title.lower().replace(" ", "_")
    out_path = out_dir / f"{name}_{slug}.gif"
    frames[0].save(out_path, save_all=True, append_images=frames[1:], duration=frame_duration, loop=0, disposal=2)
    print(f"Wrote: {out_path} ({total_w}x{total_h}, {num_poses} frames)")

# 1. Hydralisk
p_hydra = r"C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-3e4e02cb-6c72-4612-b90e-dcfa92da1a9f.png"
if not os.path.exists(p_hydra):
    p_hydra = "assets/sprites/enemies/zerg_hydralisk_source.png"
im_hydra = Image.open(p_hydra).convert("RGBA")
hydra_cols = [(2 + i * 45, 2 + i * 45 + 42) for i in range(9)]
hydra_walk_rows = [(2 + i * 58, 2 + i * 58 + 55) for i in range(7)]
hydra_attack_rows = [(408 + i * 58, 408 + i * 58 + 55) for i in range(5)]

make_collage(im_hydra, "hydralisk", "WALK", hydra_walk_rows, hydra_cols, 42, 55, frame_duration=130, scale=2)
make_collage(im_hydra, "hydralisk", "ATTACK", hydra_attack_rows, hydra_cols, 42, 55, frame_duration=140, scale=2)

# 2. Zergling
p_zergling = r"C:/Users/malfonso/.gemini/antigravity/brain/1b7c5296-7dda-4d22-941e-637e53321ae1/.user_uploaded/media_1789386636725.png"
im_zergling = Image.open(p_zergling).convert("RGBA")
zergling_cols = [(2, 41), (45, 84), (88, 127), (131, 170), (174, 213), (217, 256), (260, 299), (303, 342), (346, 385)]
zergling_walk_rows = [(2, 40), (44, 82), (86, 124), (128, 166), (170, 208), (212, 250), (254, 292)]
zergling_attack_rows = [(296, 334), (338, 376), (380, 418), (422, 460), (464, 502)]

make_collage(im_zergling, "zergling", "WALK", zergling_walk_rows, zergling_cols, 40, 39, frame_duration=100, scale=2)
make_collage(im_zergling, "zergling", "ATTACK", zergling_attack_rows, zergling_cols, 40, 39, frame_duration=110, scale=2)
