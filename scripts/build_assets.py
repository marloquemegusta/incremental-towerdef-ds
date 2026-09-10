#!/usr/bin/env python3
"""
build_assets.py - Unified Nintendo DS Asset Pipeline for towerds.

This script converts canonical master PNG assets into optimized C data arrays:
  1. Tileset: assets/tiles/sector1/T*_1x.png -> source/sector1_data.c
  2. Enemies: assets/sprites/enemies/*_strip_master_1x.png -> source/enemy_data.c & include/enemy_data.h
  3. Turrets: assets/sprites/turrets/*_strip_master_1x.png -> source/turret_data.c & include/turret_data.h

Usage:
  python scripts/build_assets.py            # Converts assets to C source
  python scripts/build_assets.py --rebuild  # Converts assets and re-builds game.nds
"""

import os
import sys
import argparse
import subprocess
from PIL import Image

def to_bgr555(r, g, b, a=255):
    """Converts RGBA color to Nintendo DS 15-bit RGB with alpha bit 15."""
    if a < 128:
        return 0
    return ((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10) | 0x8000

def get_image_pixels(img):
    """Safely retrieves RGBA pixel tuples across Pillow versions."""
    if hasattr(img, "get_flattened_data"):
        return list(img.get_flattened_data())
    return list(img.getdata())

def build_tiles():
    print("[1/3] Building Sector 1 Tileset...")
    tiles_dir = "assets/tiles/sector1"
    
    # Expected canonical 17 tiles in order T00 to T16
    tile_files = [
        "T00_road_plain_1x.png",
        "T01_road_dash_1x.png",
        "T02_road_cracked_1x.png",
        "T03_road_crater_1x.png",
        "T04_curb_straight_1x.png",
        "T05_curb_storm_drain_1x.png",
        "T06_curb_culvert_pipe_1x.png",
        "T07_curb_stairs_down_1x.png",
        "T08_sidewalk_slabs_1x.png",
        "T09_sidewalk_cracked_1x.png",
        "T10_manhole_1x.png",
        "T11_roof_plain_1x.png",
        "T12_vent_grate_1x.png",
        "T13_roof_edge_shadow_1x.png",
        "T14_roof_ac_unit_1x.png",
        "T15_roof_access_hut_1x.png",
        "T16_bunker_door_1x.png",
    ]
    
    tile_count = len(tile_files)
    tile_data = []
    
    for idx, tf in enumerate(tile_files):
        path = os.path.join(tiles_dir, tf)
        if not os.path.exists(path):
            raise FileNotFoundError(f"Missing tile asset: {path}")
        
        im = Image.open(path).convert("RGBA")
        if im.size != (16, 16):
            raise ValueError(f"Tile {tf} must be 16x16 pixels, got {im.size}")
        
        pixels = get_image_pixels(im)
        bgr_pixels = [to_bgr555(*p) for p in pixels]
        tile_data.append((idx, tf, bgr_pixels))

    # Read existing map layout if present, otherwise use default
    map_layout = [
        [ 11, 11, 14, 11, 11,  8,  8,  8,  8,  8,  8, 11, 11, 14, 11, 11 ],
        [ 11, 11, 11, 11, 11,  8,  8,  8,  8,  8,  8, 11, 11, 11, 11, 11 ],
        [ 13, 13, 13, 13, 13,  8,  8,  8,  8,  8,  8, 13, 13, 13, 13, 13 ],
        [  8,  8,  8,  9,  8,  8,  8, 10,  8,  8,  8,  8,  8,  9,  8,  8 ],
        [  4,  4,  5,  4,  4,  4,  6,  4,  4,  4,  5,  4,  4,  4,  7,  4 ],
        [  0,  0,  0,  0,  2,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  0 ],
        [  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1,  1,  1 ],
        [  8,  8, 16,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8, 16,  8,  8 ],
        [  8,  8,  8,  8,  8,  8,  8,  8,  9,  8,  8,  8,  8,  8,  8,  8 ],
        [  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8 ],
        [  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8, 10,  8 ],
        [  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8 ]
    ]

    # Write source/sector1_data.c
    with open("source/sector1_data.c", "w") as f:
        f.write('// Auto-generated Sector 1 assets for Nintendo DS (from assets/tiles/sector1/)\n')
        f.write('#include "game.h"\n#include "tiles.h"\n\n')
        f.write(f'#define S1_TILE_COUNT {tile_count}\n')
        f.write(f'const uint16_t g_s1_tiles[S1_TILE_COUNT][256] __attribute__((aligned(4))) = {{\n')
        
        for idx, tf, pixels in tile_data:
            f.write(f'  // Tile {idx}: {tf}\n  {{\n')
            for row in range(16):
                row_pixels = pixels[row * 16 : (row + 1) * 16]
                line = "    " + ", ".join(f"0x{p:04X}" for p in row_pixels)
                if row < 15 or idx < tile_count - 1:
                    line += ","
                f.write(line + "\n")
            f.write("  }" + ("," if idx < tile_count - 1 else "") + "\n")
        f.write("};\n\n")

        # Map array
        f.write("const uint8_t g_s1_map[12][16] = {\n")
        for r_idx, row in enumerate(map_layout):
            row_str = "  { " + ", ".join(f"{v:2d}" for v in row) + " }"
            if r_idx < len(map_layout) - 1:
                row_str += ","
            f.write(row_str + "\n")
        f.write("};\n")

    print(f"  -> Generated source/sector1_data.c ({tile_count} tiles of 16x16 px)")

def build_enemies():
    print("[2/3] Building Tyranid Enemy Sprites...")
    enemies_dir = "assets/sprites/enemies"

    enemy_defs = [
        ("t0_larva", "t0_larva_strip_master_1x.png", 4, 1, 1),
        ("t1_ripper", "t1_ripper_strip_master_1x.png", 4, 8, 3),
        ("t2_hormagaunt", "t2_hormagaunt_strip_master_1x.png", 4, 40, 10),
        ("t3_ravener", "t3_ravener_strip_master_1x.png", 4, 160, 60),
        ("t4_carnifex", "t4_carnifex_strip_master_1x.png", 4, 2500, 750),
        ("t5_hierophant", "t5_hierophant_strip_master_1x.png", 4, 40000, 20000),
    ]

    enemy_count = len(enemy_defs)

    # Write include/enemy_data.h
    with open("include/enemy_data.h", "w") as fh:
        fh.write(f'''#ifndef ENEMY_DATA_H
#define ENEMY_DATA_H

#include <nds.h>

#define ENEMY_VARIANT_COUNT {enemy_count}

typedef struct {{
    uint8_t w;
    uint8_t h;
    const uint16_t *pixels;
}} EnemyFrameDef;

typedef struct {{
    uint8_t frame_count;
    uint32_t default_hp;
    uint32_t scrap_value;
    EnemyFrameDef frames[4];
}} EnemyTypeDef;

extern const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT];

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir);

#endif // ENEMY_DATA_H
''')

    # Process each strip and write source/enemy_data.c
    with open("source/enemy_data.c", "w") as fc:
        fc.write('#include "enemy_data.h"\n#include "game.h"\n\n')

        enemies_meta = []

        for name, strip_file, num_frames, hp, scrap in enemy_defs:
            path = os.path.join(enemies_dir, strip_file)
            if not os.path.exists(path):
                raise FileNotFoundError(f"Missing enemy master strip: {path}")

            im = Image.open(path).convert("RGBA")
            frame_w = im.width // num_frames
            frame_h = im.height

            frame_data = []
            for f_idx in range(num_frames):
                box = (f_idx * frame_w, 0, (f_idx + 1) * frame_w, frame_h)
                frame_crop = im.crop(box)
                pixels = [to_bgr555(*p) for p in get_image_pixels(frame_crop)]
                frame_data.append(pixels)

                fc.write(f"static const uint16_t s_{name}_f{f_idx}[{frame_w * frame_h}] = {{\n    ")
                for y in range(frame_h):
                    line = ", ".join(f"0x{p:04X}" for p in pixels[y * frame_w : (y + 1) * frame_w])
                    if y < frame_h - 1:
                        line += ", "
                    fc.write(line)
                fc.write("\n};\n\n")

            enemies_meta.append((name, frame_w, frame_h, num_frames, hp, scrap))

        # Array of types
        fc.write("const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT] = {\n")
        for name, fw, fh, nframes, hp, scrap in enemies_meta:
            fc.write(f"    {{ // {name}\n")
            fc.write(f"        {nframes}, {hp}, {scrap},\n        {{\n")
            for f_idx in range(nframes):
                fc.write(f"            {{ {fw}, {fh}, s_{name}_f{f_idx} }},\n")
            fc.write("        }\n    },\n")
        fc.write("};\n\n")

        # Drawing routine
        fc.write('''void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir) {
    if (variant < 0 || variant >= ENEMY_VARIANT_COUNT) variant = 0;
    frame = frame & 3;

    const EnemyFrameDef *fd = &g_enemy_types[variant].frames[frame];
    int w = fd->w;
    int h = fd->h;
    const uint16_t *src = fd->pixels;

    int ox = cx - (w / 2);
    int oy = cy - (h / 2);

    for (int y = 0; y < h; y++) {
        int dst_y = oy + y;
        if (dst_y < 0 || dst_y >= SCREEN_H) continue;

        for (int x = 0; x < w; x++) {
            int draw_x;
            if (dir == 2) {
                draw_x = ox + (w - 1 - x);
            } else {
                draw_x = ox + x;
            }

            if (draw_x < 0 || draw_x >= SCREEN_W) continue;

            uint16_t col = src[y * w + x];
            if (col & 0x8000) {
                g_backbuffer[dst_y * SCREEN_W + draw_x] = col;
            }
        }
    }
}
''')

    print(f"  -> Generated source/enemy_data.c and include/enemy_data.h ({enemy_count} biocasts)")

def build_turrets():
    print("[3/3] Building Turret Arsenal Sprites...")
    turrets_dir = "assets/sprites/turrets"

    turret_defs = [
        ("heavy_bolter", "heavy_bolter_strip_master_1x.png", 32, 32),
        ("lascannon", "lascannon_strip_master_1x.png", 32, 32)
    ]

    # Write include/turret_data.h
    with open("include/turret_data.h", "w") as fh:
        fh.write('''#ifndef TURRET_DATA_H
#define TURRET_DATA_H

#include <nds.h>

#define TURRET_TYPE_BOLTER    0
#define TURRET_TYPE_LASCANNON 1
#define TURRET_TYPE_COUNT     2

#define TURRET_FRAME_SIZE 32

typedef struct {
    uint8_t frame_count;
    uint8_t frame_w;
    uint8_t frame_h;
    const uint16_t *frames;
} TurretSpriteDef;

extern const TurretSpriteDef g_turret_sprites[TURRET_TYPE_COUNT];

void turret_draw_sprite_frame(int cx, int cy, int type, int frame_idx, int is_selected);

#endif // TURRET_DATA_H
''')

    # Write source/turret_data.c
    with open("source/turret_data.c", "w") as fc:
        fc.write('#include "turret_data.h"\n#include "game.h"\n\n')

        turrets_meta = []

        for name, strip_file, fw, fh in turret_defs:
            path = os.path.join(turrets_dir, strip_file)
            if not os.path.exists(path):
                raise FileNotFoundError(f"Missing turret master strip: {path}")

            im = Image.open(path).convert("RGBA")
            num_frames = im.width // fw

            total_pixels = num_frames * fw * fh
            fc.write(f"static const uint16_t s_{name}_frames[{total_pixels}] __attribute__((aligned(4))) = {{\n")

            for f_idx in range(num_frames):
                box = (f_idx * fw, 0, (f_idx + 1) * fw, fh)
                frame_crop = im.crop(box)
                pixels = [to_bgr555(*p) for p in get_image_pixels(frame_crop)]

                fc.write(f"  // Frame {f_idx}\n")
                for y in range(fh):
                    line = "  " + ", ".join(f"0x{p:04X}" for p in pixels[y * fw : (y + 1) * fw]) + ",\n"
                    fc.write(line)

            fc.write("};\n\n")
            turrets_meta.append((name, num_frames, fw, fh))

        # Array of turret sprite definitions
        fc.write("const TurretSpriteDef g_turret_sprites[TURRET_TYPE_COUNT] = {\n")
        for name, nframes, fw, fh in turrets_meta:
            fc.write(f"    {{ {nframes}, {fw}, {fh}, s_{name}_frames }},\n")
        fc.write("};\n\n")

        # Drawing routine for direct rendering onto g_backbuffer
        fc.write('''void turret_draw_sprite_frame(int cx, int cy, int type, int frame_idx, int is_selected) {
    if (type < 0 || type >= TURRET_TYPE_COUNT) type = 0;
    const TurretSpriteDef *def = &g_turret_sprites[type];
    frame_idx = frame_idx % def->frame_count;

    int w = def->frame_w;
    int h = def->frame_h;
    const uint16_t *src = &def->frames[frame_idx * w * h];

    int ox = cx - (w / 2);
    int oy = cy - (h / 2);

    for (int y = 0; y < h; y++) {
        int dst_y = oy + y;
        if (dst_y < 0 || dst_y >= SCREEN_H) continue;

        for (int x = 0; x < w; x++) {
            int dst_x = ox + x;
            if (dst_x < 0 || dst_x >= SCREEN_W) continue;

            uint16_t col = src[y * w + x];
            if (col & 0x8000) {
                g_backbuffer[dst_y * SCREEN_W + dst_x] = col;
            }
        }
    }

    if (is_selected) {
        renderer_draw_rect(ox - 1, oy - 1, w + 2, h + 2, COLOR_HAZARD_YELLOW);
    }
}
''')

    print(f"  -> Generated source/turret_data.c and include/turret_data.h ({len(turret_defs)} turret types)")

def main():
    parser = argparse.ArgumentParser(description="Build Nintendo DS assets from PNG files.")
    parser.add_argument("--rebuild", action="store_true", help="Re-build ROM game.nds after converting assets")
    args = parser.parse_args()

    print("==================================================")
    print("  TOWERDS: ASSET COMPILATION PIPELINE")
    print("==================================================")
    
    build_tiles()
    build_enemies()
    build_turrets()

    print("\n[OK] All assets successfully converted to C source code!")

    if args.rebuild:
        print("\n[+] Triggering BlocksDS Nintendo DS Cartridge Rebuild...")
        res = subprocess.run([
            "powershell", "-ExecutionPolicy", "Bypass",
            "-File", "scripts/build-project.ps1",
            "-ProjectPath", "."
        ])
        if res.returncode != 0:
            print("[-] Build failed!")
            sys.exit(1)
        print("[+] Cartridge rebuilt successfully: game.nds")

if __name__ == "__main__":
    main()
