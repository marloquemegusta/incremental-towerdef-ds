import os
from pathlib import Path
from PIL import Image

DEST_DIR = Path("assets/sprites/enemies")
DEST_DIR.mkdir(parents=True, exist_ok=True)
ARCHIVE_DIR = DEST_DIR / "archive" / "tyranid_legacy"
ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)

# Archive original Tyranid masters if not already archived
legacy_files = [
    "t0_larva_strip_master_1x.png",
    "t1_ripper_strip_master_1x.png",
    "t2_hormagaunt_strip_master_1x.png",
    "t3_ravener_strip_master_1x.png",
    "t4_carnifex_strip_master_1x.png",
    "t5_hierophant_strip_master_1x.png",
]
for f in legacy_files:
    src = DEST_DIR / f
    dst = ARCHIVE_DIR / f
    if src.exists() and not dst.exists():
        import shutil
        shutil.copy(src, dst)
        print(f"Archived legacy master: {f} -> {dst}")

DIR_MAP_9COL = [
    (0, False), # N
    (2, False), # NE
    (4, False), # E
    (6, False), # SE
    (8, False), # S
    (6, True),  # SW
    (4, True),  # W
    (2, True),  # NW
]

DIR_MAP_ULTRA = [
    (0, False), # N
    (2, False), # NE
    (4, False), # E
    (7, False), # SE
    (9, False), # S
    (7, True),  # SW
    (4, True),  # W
    (2, True),  # NW
]

def build_strip(source_img, col_coords, row_coords, dir_map, dest_path, cell_w, cell_h):
    num_frames = len(row_coords)
    num_dirs = len(dir_map) # 8
    
    strip = Image.new("RGBA", (cell_w * num_frames, cell_h * num_dirs), (0, 0, 0, 0))
    for d_idx, (sc_col, flip) in enumerate(dir_map):
        x1, x2 = col_coords[sc_col]
        for f_idx, (y1, y2) in enumerate(row_coords):
            cell = source_img.crop((x1, y1, x2, y2))
            if flip:
                cell = cell.transpose(Image.FLIP_LEFT_RIGHT)
            if cell.size != (cell_w, cell_h):
                cell = cell.resize((cell_w, cell_h), Image.Resampling.NEAREST)
            strip.alpha_composite(cell, (f_idx * cell_w, d_idx * cell_h))
            
    strip.save(dest_path)
    print(f"Generated canonical strip: {dest_path} ({strip.size}, {num_frames} frames x {num_dirs} dirs)")
    return strip

# 1. Larva (5 frames, excluding death frame 6)
p_larva = "assets/sprites/enemies/raw_masters/zerg_larvae_eggs_and_births.png"
im_larva = Image.open(p_larva).convert("RGBA")
arr_l = list(im_larva.getdata())
clean_l = [(0,0,0,0) if (abs(r-74)<20 and abs(g-221)<20 and abs(b-1)<20) else (r,g,b,a) for r,g,b,a in arr_l]
im_larva.putdata(clean_l)
larva_cols = [(2 + i * 33, 2 + i * 33 + 30) for i in range(9)]
larva_rows = [(2 + i * 29, 2 + i * 29 + 26) for i in range(5)]
build_strip(im_larva, larva_cols, larva_rows, DIR_MAP_9COL, DEST_DIR / "sc_larva_walk_strip_master_1x.png", 30, 26)

# 2. Zergling
im_z = Image.open("assets/sprites/enemies/raw_masters/zerg_zergling.png").convert("RGBA")
z_cols = [(2, 41), (45, 84), (88, 127), (131, 170), (174, 213), (217, 256), (260, 299), (303, 342), (346, 385)]
z_walk_rows = [(2, 40), (44, 82), (86, 124), (128, 166), (170, 208), (212, 250), (254, 292)]
z_attack_rows = [(296, 334), (338, 376), (380, 418), (422, 460), (464, 502)]
build_strip(im_z, z_cols, z_walk_rows, DIR_MAP_9COL, DEST_DIR / "t1_zergling_walk_strip_master_1x.png", 40, 39)
build_strip(im_z, z_cols, z_attack_rows, DIR_MAP_9COL, DEST_DIR / "t1_zergling_attack_strip_master_1x.png", 40, 39)

# 3. Hydralisk
im_h = Image.open("assets/sprites/enemies/raw_masters/zerg_hydralisk.png").convert("RGBA")
arr_h = list(im_h.getdata())
clean_h = [(0, 0, 0, 0) if (abs(r-128)<20 and abs(g-127)<20 and abs(b-127)<20) else (r, g, b, a) for r, g, b, a in arr_h]
im_h.putdata(clean_h)
h_cols = [(2 + i * 45, 2 + i * 45 + 42) for i in range(9)]
h_walk_rows = [(2 + i * 58, 2 + i * 58 + 55) for i in range(7)]
h_attack_rows = [(408 + i * 58, 408 + i * 58 + 55) for i in range(5)]
build_strip(im_h, h_cols, h_walk_rows, DIR_MAP_9COL, DEST_DIR / "t2_hydralisk_walk_strip_master_1x.png", 42, 55)
build_strip(im_h, h_cols, h_attack_rows, DIR_MAP_9COL, DEST_DIR / "t2_hydralisk_attack_strip_master_1x.png", 42, 55)

# 4. Scourge
im_s = Image.open("assets/sprites/enemies/raw_masters/zerg_scourge.png").convert("RGBA")
arr_s = list(im_s.getdata())
clean_s = [(0, 0, 0, 0) if (r > 240 and g > 240 and b > 240) else (r, g, b, a) for r, g, b, a in arr_s]
im_s.putdata(clean_s)
s_cols = [(2 + i * 34, 2 + i * 34 + 31) for i in range(9)]
s_fly_rows = [(2 + i * 30, 2 + i * 30 + 27) for i in range(5)]
build_strip(im_s, s_cols, s_fly_rows, DIR_MAP_9COL, DEST_DIR / "t3_scourge_fly_strip_master_1x.png", 31, 27)

# 5. Mutalisk
im_m = Image.open("assets/sprites/enemies/raw_masters/zerg_mutalisk.png").convert("RGBA")
arr_m = list(im_m.getdata())
clean_m = [(0, 0, 0, 0) if (r > 240 and g > 240 and b > 240) else (r, g, b, a) for r, g, b, a in arr_m]
im_m.putdata(clean_m)
m_cols = [(2 + i * 67, 2 + i * 67 + 64) for i in range(9)]
m_fly_rows = [(2 + i * 75, 2 + i * 75 + 72) for i in range(5)]
build_strip(im_m, m_cols, m_fly_rows, DIR_MAP_9COL, DEST_DIR / "sc_mutalisk_fly_strip_master_1x.png", 64, 72)

# 6. Guardian
im_g = Image.open("assets/sprites/enemies/raw_masters/zerg_guardian.png").convert("RGBA")
arr_g = list(im_g.getdata())
clean_g = [(0, 0, 0, 0) if (r > 240 and g > 240 and b > 240) else (r, g, b, a) for r, g, b, a in arr_g]
im_g.putdata(clean_g)
g_cols = [(2 + i * 81, 2 + i * 81 + 78) for i in range(9)]
g_fly_rows = [(70 + i * 74, 70 + i * 74 + 70) for i in range(7)]
build_strip(im_g, g_cols, g_fly_rows, DIR_MAP_9COL, DEST_DIR / "sc_guardian_fly_strip_master_1x.png", 78, 70)

# 7. Lurker
im_lurker = Image.open("assets/sprites/enemies/raw_masters/zerg_lurker.png").convert("RGBA")
arr_lurker = list(im_lurker.getdata())
clean_lurker = [(0,0,0,0) if (abs(r-128)<15 and abs(g-127)<15 and abs(b-127)<15) or (r>240 and g>240 and b>240) else (r,g,b,a) for r,g,b,a in arr_lurker]
im_lurker.putdata(clean_lurker)
lurker_cols = [(2 + i * 72, 2 + i * 72 + 69) for i in range(9)]
lurker_rows = [(64 + i * 67, 64 + i * 67 + 64) for i in range(7)]
build_strip(im_lurker, lurker_cols, lurker_rows, DIR_MAP_9COL, DEST_DIR / "sc_lurker_walk_strip_master_1x.png", 69, 64)

# 8. Defiler (8 frames, excluding burrow frame 9)
im_defiler = Image.open("assets/sprites/enemies/raw_masters/zerg_defiler.png").convert("RGBA")
arr_d = list(im_defiler.getdata())
clean_d = [(0,0,0,0) if (abs(r-85)<20 and abs(g-170)<20 and abs(b-170)<20) else (r,g,b,a) for r,g,b,a in arr_d]
im_defiler.putdata(clean_d)
defiler_cols = [(2 + i * 72, 2 + i * 72 + 69) for i in range(9)]
defiler_rows = [(2 + i * 62, 2 + i * 62 + 59) for i in range(8)]
build_strip(im_defiler, defiler_cols, defiler_rows, DIR_MAP_9COL, DEST_DIR / "sc_defiler_walk_strip_master_1x.png", 69, 59)

# 9. Ultralisk
im_u = Image.open("assets/sprites/enemies/raw_masters/zerg_ultralisk.png").convert("RGBA")
arr_u = list(im_u.getdata())
clean_u = [(0, 0, 0, 0) if (abs(r - 50) < 20 and abs(g - 254) < 20 and abs(b - 194) < 20) else (r, g, b, a) for r, g, b, a in arr_u]
im_u.putdata(clean_u)
u_cols = [(2 + i * 101, 2 + i * 101 + 98) for i in range(10)]
u_walk_rows = [(2 + i * 108, 2 + i * 108 + 105) for i in range(9)]
u_attack_rows = [(2 + i * 108, 2 + i * 108 + 105) for i in range(9, 15)]
build_strip(im_u, u_cols, u_walk_rows, DIR_MAP_ULTRA, DEST_DIR / "t4_ultralisk_walk_strip_master_1x.png", 98, 105)
build_strip(im_u, u_cols, u_attack_rows, DIR_MAP_ULTRA, DEST_DIR / "t4_ultralisk_attack_strip_master_1x.png", 98, 105)

print("All canonical strips generated successfully!")
