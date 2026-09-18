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
    print("[2/3] Building Tyranid & StarCraft Enemy Sprites...")
    enemies_dir = "assets/sprites/enemies"

    # Tuple: (name, walk_file, attack_file, num_walk_frames, num_attack_frames, hp, scrap, render_mode, is_flying, flight_altitude)
    enemy_defs = [
        ("t0_scourge", "t3_scourge_fly_strip_master_1x.png", None, 5, 0, 18, 4, 1, 1, 10),
        ("t1_zergling", "t1_zergling_walk_strip_master_1x.png", "t1_zergling_attack_strip_master_1x.png", 7, 5, 25, 5, 1, 0, 0),
        ("t2_hydralisk", "t2_hydralisk_walk_strip_master_1x.png", "t2_hydralisk_attack_strip_master_1x.png", 7, 5, 75, 15, 1, 0, 0),
        ("t3_mutalisk", "sc_mutalisk_fly_strip_master_1x.png", None, 5, 0, 160, 35, 1, 1, 14),
        ("t4_defiler", "sc_defiler_walk_strip_master_1x.png", None, 8, 0, 320, 70, 1, 0, 0),
        ("t5_lurker", "sc_lurker_walk_strip_master_1x.png", None, 7, 0, 500, 120, 1, 0, 0),
        ("t6_guardian", "sc_guardian_fly_strip_master_1x.png", None, 7, 0, 1100, 250, 1, 1, 16),
        ("t7_ultralisk", "t4_ultralisk_walk_strip_master_1x.png", "t4_ultralisk_attack_strip_master_1x.png", 9, 6, 2600, 600, 1, 0, 0),
    ]

    enemy_count = len(enemy_defs)

    # Write include/enemy_data.h
    with open("include/enemy_data.h", "w") as fh:
        fh.write(f'''#ifndef ENEMY_DATA_H
#define ENEMY_DATA_H

#include <nds.h>

#define ENEMY_VARIANT_COUNT {enemy_count}
#define ENEMY_RENDER_DIRECTIONAL 1
#define ENEMY_MAX_DIRECTIONS 5
#define ENEMY_MAX_FRAMES 9
#define ENEMY_MAX_ATTACK_FRAMES 6

typedef struct {{
    uint8_t w;
    uint8_t h;
    int8_t offset_x;
    int8_t offset_y;
    int8_t flip_ox;
    const uint16_t *pixels;
}} EnemyFrameDef;

typedef struct {{
    uint8_t render_mode;
    uint8_t is_flying;
    int8_t flight_altitude;
    uint8_t direction_count;
    uint8_t frame_count;
    uint8_t attack_frame_count;
    uint32_t default_hp;
    uint32_t scrap_value;
    EnemyFrameDef frames[ENEMY_MAX_DIRECTIONS][ENEMY_MAX_FRAMES];
    EnemyFrameDef attack_frames[ENEMY_MAX_DIRECTIONS][ENEMY_MAX_ATTACK_FRAMES];
}} EnemyTypeDef;

extern const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT];

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir);
void enemy_draw_sprite_to_buffer(uint16_t *buffer, int cx, int cy, int variant, int frame, int dir, int is_attacking);

#endif // ENEMY_DATA_H
''')

    # Process each strip and write source/enemy_data.c
    with open("source/enemy_data.c", "w") as fc:
        fc.write('#include "enemy_data.h"\n#include "game.h"\n\n')

        enemies_meta = []

        for name, walk_file, att_file, num_frames, num_att, hp, scrap, render_mode, is_flying, flight_alt in enemy_defs:
            if render_mode != 1:
                raise ValueError(f"{name} must provide pre-oriented directional frames")
            path = os.path.join(enemies_dir, walk_file)
            if not os.path.exists(path):
                raise FileNotFoundError(f"Missing enemy master strip: {path}")

            im = Image.open(path).convert("RGBA")
            if render_mode == 1:
                if im.height % 8 != 0 or im.width % num_frames != 0:
                    raise ValueError(
                        f"Directional master {walk_file} must be {num_frames} columns x 8 rows; got {im.size}"
                    )
                source_frame_w = im.width // num_frames
                source_frame_h = im.height // 8
                direction_count = 5
                direction_sizes = []
                parity = source_frame_w % 2
                for d_idx in range(direction_count):
                    bbox = None
                    for f_idx in range(num_frames):
                        frame = im.crop((f_idx * source_frame_w, d_idx * source_frame_h,
                                         (f_idx + 1) * source_frame_w, (d_idx + 1) * source_frame_h))
                        current = frame.getbbox()
                        if current:
                            bbox = current if bbox is None else (
                                min(bbox[0], current[0]), min(bbox[1], current[1]),
                                max(bbox[2], current[2]), max(bbox[3], current[3]))
                    if bbox is None:
                        bbox = (0, 0, source_frame_w, source_frame_h)
                    dw = bbox[2] - bbox[0]
                    ox = bbox[0] - source_frame_w // 2
                    flip_ox = -ox - dw + parity
                    direction_sizes.append((dw, bbox[3] - bbox[1],
                                            ox,
                                            bbox[1] - source_frame_h // 2,
                                            flip_ox,
                                            bbox))
            else:
                frame_w = im.width // num_frames
                frame_h = im.height
                direction_count = 1
                direction_sizes = [(frame_w, frame_h, 0, 0, 0, (0, 0, frame_w, frame_h))]

            # Write walk frame pixel arrays (stored for first 5 directions: 0=N, 1=NE, 2=E, 3=SE, 4=S)
            for d_idx in range(direction_count):
                dir_w, dir_h, offset_x, offset_y, flip_ox, bbox = direction_sizes[d_idx]
                for f_idx in range(num_frames):
                    if render_mode == 1:
                        box = (f_idx * source_frame_w + bbox[0],
                               d_idx * source_frame_h + bbox[1],
                               f_idx * source_frame_w + bbox[2],
                               d_idx * source_frame_h + bbox[3])
                        crop_w, crop_h = dir_w, dir_h
                    else:
                        box = (f_idx * frame_w, 0, (f_idx + 1) * frame_w, frame_h)
                        crop_w, crop_h = frame_w, frame_h
                    frame_crop = im.crop(box)
                    pixels = [to_bgr555(*p) for p in get_image_pixels(frame_crop)]

                    fc.write(f"static const uint16_t s_{name}_d{d_idx}_f{f_idx}[{crop_w * crop_h}] = {{\n    ")
                    for y in range(crop_h):
                        line = ", ".join(f"0x{p:04X}" for p in pixels[y * crop_w : (y + 1) * crop_w])
                        if y < crop_h - 1:
                            line += ", "
                        fc.write(line)
                    fc.write("\n};\n\n")

            # Process attack frames if available
            att_direction_sizes = []
            if att_file and num_att > 0:
                att_path = os.path.join(enemies_dir, att_file)
                im_att = Image.open(att_path).convert("RGBA")
                att_source_w = im_att.width // num_att
                att_source_h = im_att.height // 8
                att_parity = att_source_w % 2
                for d_idx in range(5):
                    bbox = None
                    for f_idx in range(num_att):
                        frame = im_att.crop((f_idx * att_source_w, d_idx * att_source_h,
                                             (f_idx + 1) * att_source_w, (d_idx + 1) * att_source_h))
                        current = frame.getbbox()
                        if current:
                            bbox = current if bbox is None else (
                                min(bbox[0], current[0]), min(bbox[1], current[1]),
                                max(bbox[2], current[2]), max(bbox[3], current[3]))
                    if bbox is None:
                        bbox = (0, 0, att_source_w, att_source_h)
                    adw = bbox[2] - bbox[0]
                    aox = bbox[0] - att_source_w // 2
                    a_flip_ox = -aox - adw + att_parity
                    att_direction_sizes.append((adw, bbox[3] - bbox[1],
                                                aox,
                                                bbox[1] - att_source_h // 2,
                                                a_flip_ox,
                                                bbox))
                for d_idx in range(5):
                    dir_w, dir_h, offset_x, offset_y, flip_ox, bbox = att_direction_sizes[d_idx]
                    for f_idx in range(num_att):
                        box = (f_idx * att_source_w + bbox[0],
                               d_idx * att_source_h + bbox[1],
                               f_idx * att_source_w + bbox[2],
                               d_idx * att_source_h + bbox[3])
                        frame_crop = im_att.crop(box)
                        pixels = [to_bgr555(*p) for p in get_image_pixels(frame_crop)]
                        fc.write(f"static const uint16_t s_{name}_att_d{d_idx}_f{f_idx}[{dir_w * dir_h}] = {{\n    ")
                        for y in range(dir_h):
                            line = ", ".join(f"0x{p:04X}" for p in pixels[y * dir_w : (y + 1) * dir_w])
                            if y < dir_h - 1:
                                line += ", "
                            fc.write(line)
                        fc.write("\n};\n\n")

            enemies_meta.append((name, num_frames, num_att, direction_count, direction_sizes, att_direction_sizes, hp, scrap, render_mode, is_flying, flight_alt))

        # Array of types
        fc.write("const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT] = {\n")
        for name, nframes, natt, ndirections, direction_sizes, att_direction_sizes, hp, scrap, render_mode, is_flying, flight_alt in enemies_meta:
            fc.write(f"    {{ // {name}\n")
            fc.write(f"        {render_mode}, {is_flying}, {flight_alt}, {ndirections}, {nframes}, {natt}, {hp}, {scrap},\n        {{\n")
            # Walk frames (5 directions: 0..4)
            for d_idx in range(5):
                fc.write("            {\n")
                for f_idx in range(9):
                    if d_idx < ndirections and f_idx < nframes:
                        fw, fh, offset_x, offset_y, flip_ox, _ = direction_sizes[d_idx]
                        fc.write(f"                {{ {fw}, {fh}, {offset_x}, {offset_y}, {flip_ox}, s_{name}_d{d_idx}_f{f_idx} }},\n")
                    else:
                        fc.write("                { 0, 0, 0, 0, 0, 0 },\n")
                fc.write("            },\n")
            fc.write("        },\n        {\n")
            # Attack frames (5 directions: 0..4)
            for d_idx in range(5):
                fc.write("            {\n")
                for f_idx in range(6):
                    if natt > 0 and d_idx < 5 and f_idx < natt:
                        fw, fh, offset_x, offset_y, flip_ox, _ = att_direction_sizes[d_idx]
                        fc.write(f"                {{ {fw}, {fh}, {offset_x}, {offset_y}, {flip_ox}, s_{name}_att_d{d_idx}_f{f_idx} }},\n")
                    else:
                        fc.write("                { 0, 0, 0, 0, 0, 0 },\n")
                fc.write("            },\n")
            fc.write("        }\n    },\n")
        fc.write("};\n\n")

        # Drawing routine
        fc.write('''void enemy_draw_sprite_to_buffer(uint16_t *buffer, int cx, int cy, int variant, int frame, int dir, int is_attacking) {
    if (!buffer) return;
    if (variant < 0 || variant >= ENEMY_VARIANT_COUNT) variant = 0;
    const EnemyTypeDef *type = &g_enemy_types[variant];

    // 1. Draw dynamic ground shadow for flying units
    if (type->is_flying && type->flight_altitude > 0) {
        static const int8_t shadow_span[9] = { 4, 7, 9, 10, 10, 10, 9, 7, 4 };
        for (int dy = -4; dy <= 4; dy++) {
            int py = cy + dy;
            if (py < 0 || py >= SCREEN_H) continue;
            int half_w = shadow_span[dy + 4];
            int x_start = cx - half_w;
            int x_end = cx + half_w;
            if (x_start < 0) x_start = 0;
            if (x_end >= SCREEN_W) x_end = SCREEN_W - 1;
            uint16_t *line = &buffer[py * SCREEN_W];
            for (int px = x_start; px <= x_end; px++) {
                line[px] = (line[px] >> 1) & 0x3DEF;
            }
        }
        cy -= type->flight_altitude;
    }

    int source_dir;
    int flip_h = 0;
    int d = dir & 7;
    if (d > 4) {
        flip_h = 1;
        if (d == 5) source_dir = 3;      // SW mirrors SE
        else if (d == 6) source_dir = 2; // W mirrors E
        else source_dir = 1;              // NW mirrors NE
    } else {
        source_dir = d;
    }

    const EnemyFrameDef *fd = 0;
    if (is_attacking && type->attack_frame_count > 0) {
        frame %= type->attack_frame_count;
        fd = &type->attack_frames[source_dir][frame];
    } else {
        if (type->frame_count == 0) return;
        frame %= type->frame_count;
        fd = &type->frames[source_dir][frame];
    }

    int w = fd->w;
    int h = fd->h;
    const uint16_t *src = fd->pixels;
    if (!src || w == 0 || h == 0) return;

    int ox = cx + (flip_h ? fd->flip_ox : fd->offset_x);
    int oy = cy + fd->offset_y;
    int x_min = 0;
    int x_max = w;
    if (ox < 0) x_min = -ox;
    if (ox + w > SCREEN_W) x_max = SCREEN_W - ox;

    for (int y = 0; y < h; y++) {
        int dst_y = oy + y;
        if (dst_y < 0 || dst_y >= SCREEN_H) continue;
        uint16_t *dst_row = &buffer[dst_y * SCREEN_W + ox];
        const uint16_t *row_src = &src[y * w];
        if (!flip_h) {
            for (int x = x_min; x < x_max; x++) {
                uint16_t col = row_src[x];
                if (col & 0x8000) dst_row[x] = col;
            }
        } else {
            int w_minus_1 = w - 1;
            for (int x = x_min; x < x_max; x++) {
                uint16_t col = row_src[w_minus_1 - x];
                if (col & 0x8000) dst_row[x] = col;
            }
        }
    }
}

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir) {
    enemy_draw_sprite_to_buffer(g_backbuffer, cx, cy, variant, frame, dir, 0);
}
''')

    print(f"  -> Generated source/enemy_data.c and include/enemy_data.h ({enemy_count} biocasts)")

def rotsprite_pil(img, angle):
    """Applies RotSprite-like rotation using 8x nearest scaling, bicubic rotation, and nearest downscale."""
    w, h = img.size
    scaled = img.resize((w * 8, h * 8), Image.NEAREST)
    rotated = scaled.rotate(angle, resample=Image.BICUBIC)
    downscaled = rotated.resize((w, h), Image.NEAREST)
    return downscaled

def build_turrets():
    print("[3/3] Building Turret Arsenal Sprites (16 Uniform Angles)...")
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
#define TURRET_ANGLE_COUNT 16

void turret_draw_frame_angle(int cx, int cy, int type, int frame_idx, int angle_16, int is_selected);

#endif // TURRET_DATA_H
''')

    # Write source/turret_data.c
    with open("source/turret_data.c", "w") as fc:
        fc.write('#include "turret_data.h"\n#include "game.h"\n\n')

        angles = [i * (360.0 / 16.0) for i in range(16)]

        for name, strip_file, fw, fh in turret_defs:
            path = os.path.join(turrets_dir, strip_file)
            if not os.path.exists(path):
                raise FileNotFoundError(f"Missing turret master strip: {path}")

            im = Image.open(path).convert("RGBA")
            num_frames = im.width // fw

            fc.write(f"// {name}: {num_frames} frames x 16 angles\n")
            fc.write(f"static const uint16_t s_{name}_anim[{num_frames}][16][1024] __attribute__((aligned(4))) = {{\n")

            for f_idx in range(num_frames):
                fc.write(f"  // Frame {f_idx}\n  {{\n")
                crop = im.crop((f_idx * fw, 0, (f_idx + 1) * fw, fh))

                for a_idx, ang in enumerate(angles):
                    rot = rotsprite_pil(crop, -ang)
                    pixels = [to_bgr555(*p) for p in get_image_pixels(rot)]
                    fc.write(f"    // Angle {a_idx} ({ang:.1f} deg)\n    {{\n")
                    for y in range(fh):
                        line = "      " + ", ".join(f"0x{p:04X}" for p in pixels[y * fw : (y + 1) * fw]) + ",\n"
                        fc.write(line)
                    fc.write("    },\n")
                fc.write("  },\n")

            fc.write("};\n\n")

        # Turret drawing function
        fc.write('''void turret_draw_frame_angle(int cx, int cy, int type, int frame_idx, int angle_16, int is_selected) {
    if (type < 0 || type >= TURRET_TYPE_COUNT) type = 0;
    angle_16 = angle_16 & 15;

    const uint16_t *src;
    if (type == TURRET_TYPE_LASCANNON) {
        if (frame_idx < 0 || frame_idx >= 6) frame_idx = 0;
        src = s_lascannon_anim[frame_idx][angle_16];
    } else {
        if (frame_idx < 0 || frame_idx >= 11) frame_idx = 0;
        src = s_heavy_bolter_anim[frame_idx][angle_16];
    }

    int ox = cx - 16;
    int oy = cy - 16;

    for (int y = 0; y < 32; y++) {
        int dst_y = oy + y;
        if (dst_y < 0 || dst_y >= SCREEN_H) continue;
        int row_idx = dst_y * SCREEN_W;
        int src_idx = y * 32;

        for (int x = 0; x < 32; x++) {
            int dst_x = ox + x;
            if (dst_x < 0 || dst_x >= SCREEN_W) continue;

            uint16_t col = src[src_idx + x];
            if (col & 0x8000) {
                g_backbuffer[row_idx + dst_x] = col;
            }
        }
    }

    if (is_selected) {
        renderer_draw_rect(ox - 1, oy - 1, 34, 34, COLOR_HAZARD_YELLOW);
    }
}
''')

    print(f"  -> Generated source/turret_data.c and include/turret_data.h ({len(turret_defs)} turret types, 16 uniform angles)")

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
