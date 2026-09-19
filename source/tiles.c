#include "tiles.h"
#include "enemy_data.h"

// Color Palette Shading Ramps
#define C_BLACK       (RGB15(0, 0, 0) | BIT(15))
#define C_FLOOR_DARK  (RGB15(3, 3, 4) | BIT(15))
#define C_FLOOR_MID   (RGB15(6, 6, 8) | BIT(15))
#define C_FLOOR_HI    (RGB15(10, 10, 13) | BIT(15))
#define C_SEAM        (RGB15(1, 1, 2) | BIT(15))
#define C_RIVET       (RGB15(14, 14, 16) | BIT(15))
#define C_RIVET_SHD   (RGB15(2, 2, 3) | BIT(15))

#define C_PIPE_DARK   (RGB15(4, 5, 6) | BIT(15))
#define C_PIPE_MID    (RGB15(9, 11, 13) | BIT(15))
#define C_PIPE_HI     (RGB15(16, 18, 20) | BIT(15))

#define C_VENT_BG     (RGB15(1, 1, 1) | BIT(15))
#define C_VENT_FIN    (RGB15(8, 8, 10) | BIT(15))

#define C_HAZ_Y_HI    (RGB15(31, 29, 6) | BIT(15))
#define C_HAZ_Y_MID   (RGB15(27, 21, 0) | BIT(15))
#define C_HAZ_Y_DARK  (RGB15(16, 12, 0) | BIT(15))
#define C_HAZ_BLACK   (RGB15(2, 2, 2) | BIT(15))

#define C_BRASS_HI    (RGB15(28, 24, 8) | BIT(15))
#define C_BRASS_MID   (RGB15(20, 15, 3) | BIT(15))
#define C_BRASS_DARK  (RGB15(10, 7, 1) | BIT(15))

#define C_TURRET_HI   (RGB15(18, 18, 20) | BIT(15))
#define C_TURRET_MID  (RGB15(11, 11, 13) | BIT(15))
#define C_TURRET_DARK (RGB15(5, 5, 7) | BIT(15))

#define C_MARS_RED_HI  (RGB15(29, 6, 6) | BIT(15))
#define C_MARS_RED_MID (RGB15(22, 3, 3) | BIT(15))
#define C_MARS_RED_SHD (RGB15(12, 1, 2) | BIT(15))

#define C_SPARK_WHITE (RGB15(31, 31, 30) | BIT(15))
#define C_SPARK_GOLD  (RGB15(31, 25, 6) | BIT(15))
#define C_SPARK_RED   (RGB15(30, 10, 2) | BIT(15))

// Canonical Sector 1 Urban Concrete Pavement Colors (Exact BGR555 from generate_sector_maps)
#define C_CONC_BASE   (RGB15(8, 8, 10) | BIT(15))    // (65, 70, 82)
#define C_CONC_LIGHT  (RGB15(10, 11, 12) | BIT(15))  // (82, 88, 102)
#define C_CONC_DARK   (RGB15(5, 6, 7) | BIT(15))     // (45, 48, 58)
#define C_CONC_BEVEL  (RGB15(13, 14, 16) | BIT(15))  // (110, 118, 135)
#define C_JOINT       (RGB15(4, 4, 5) | BIT(15))     // (35, 38, 46)

#define C_GRASS_DEEP  (RGB15(4, 7, 4) | BIT(15))     // (38, 62, 32)
#define C_GRASS_MID   (RGB15(8, 12, 6) | BIT(15))    // (65, 98, 48)
#define C_GRASS_TALL  (RGB15(11, 17, 7) | BIT(15))   // (95, 138, 62)

#define C_OIL_DARK    (RGB15(2, 2, 3) | BIT(15))     // (22, 20, 24)
#define C_OIL_MID     (RGB15(4, 4, 4) | BIT(15))     // (35, 32, 38)
#define C_CRACK_LINE  (RGB15(2, 2, 3) | BIT(15))

#define C_BAG_DARK    (RGB15(7, 6, 4) | BIT(15))     // (58, 48, 32)
#define C_BAG_MID     (RGB15(14, 12, 8) | BIT(15))   // (115, 96, 64)
#define C_BAG_HI      (RGB15(20, 17, 12) | BIT(15))  // (165, 142, 98)

#define C_DRAIN_GRATE (RGB15(3, 3, 3) | BIT(15))
#define C_DRAIN_HOLE  (RGB15(0, 0, 0) | BIT(15))

#define URBAN_TILE_VARIANTS 6
static uint16_t s_tiles[URBAN_TILE_VARIANTS][TILE_SIZE * TILE_SIZE];

// Deterministic noise generator matching generate_sector_maps.py
static inline uint32_t hash_noise(int x, int y, uint32_t seed) {
    uint32_t n = ((uint32_t)x * 374761393U + (uint32_t)y * 668265263U + seed);
    n = (n ^ (n >> 13)) * 1274126177U;
    return (n ^ (n >> 16)) & 0xFF;
}

// 1. Tile 0: Base Urban Concrete Slab with 16x16 Beveled Joint
static void generate_urban_slab(uint16_t *buf, int seed) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col;
            if (x == 15 || y == 15) {
                col = C_JOINT;
            } else if (x == 0 || y == 0) {
                col = C_CONC_BEVEL;
            } else {
                uint32_t v = hash_noise(x, y, seed);
                col = (v < 210) ? C_CONC_BASE : ((v > 240) ? C_CONC_LIGHT : C_CONC_DARK);
            }
            buf[y * 16 + x] = col;
        }
    }
}

// 2. Tile 1: Slab with Perimetral Grass Tuft
static void generate_urban_grass(uint16_t *buf) {
    generate_urban_slab(buf, 201);
    buf[7 * 16 + 15] = C_GRASS_MID;
    buf[6 * 16 + 15] = C_GRASS_TALL;
    buf[8 * 16 + 15] = C_GRASS_DEEP;
    buf[7 * 16 + 14] = C_GRASS_MID;
    buf[5 * 16 + 15] = C_GRASS_DEEP;
    buf[15 * 16 + 7] = C_GRASS_MID;
    buf[15 * 16 + 6] = C_GRASS_TALL;
    buf[15 * 16 + 8] = C_GRASS_DEEP;
}

// 3. Tile 2: Slab with Structural Stress Crack
static void generate_urban_crack(uint16_t *buf) {
    generate_urban_slab(buf, 303);
    static const uint8_t crack_coords[11][2] = {
        {2,3}, {3,4}, {4,5}, {5,5}, {6,6}, {7,6}, {8,7}, {9,8}, {10,9}, {11,10}, {12,11}
    };
    for (int i = 0; i < 11; i++) {
        int cx = crack_coords[i][0];
        int cy = crack_coords[i][1];
        buf[cy * 16 + cx] = C_CRACK_LINE;
        if (cx + 1 < 15) buf[cy * 16 + (cx + 1)] = C_CONC_DARK;
    }
}

// 4. Tile 3: Slab with Dark Industrial Oil Stain
static void generate_urban_oil(uint16_t *buf) {
    generate_urban_slab(buf, 404);
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
            int dist = (dx * 2 - 3) * (dx * 2 - 3) + (dy * 2 - 3) * (dy * 2 - 3);
            if (dist <= 14) {
                buf[(6 + dy) * 16 + (6 + dx)] = C_OIL_MID;
            }
        }
    }
    buf[7 * 16 + 7] = C_OIL_DARK;
    buf[7 * 16 + 8] = C_OIL_DARK;
}

// 5. Tile 4: Cast-Iron Drain Storm Grate (Sumidero de tormenta)
static void generate_urban_drain(uint16_t *buf) {
    generate_urban_slab(buf, 505);
    for (int y = 3; y <= 12; y++) {
        for (int x = 3; x <= 12; x++) {
            if (x == 3 || x == 12 || y == 3 || y == 12) {
                buf[y * 16 + x] = C_CONC_DARK;
            } else if (x % 2 == 0) {
                buf[y * 16 + x] = C_DRAIN_GRATE;
            } else {
                buf[y * 16 + x] = C_DRAIN_HOLE;
            }
        }
    }
}

// 6. Tile 5: Military Sandbag Parapet (Sacos Terreros)
static void generate_urban_sandbags(uint16_t *buf) {
    generate_urban_slab(buf, 808);
    for (int y = 11; y <= 13; y++) {
        for (int x = 0; x < 16; x++) {
            buf[y * 16 + x] = C_BLACK;
        }
    }
    // Lower sandbag row
    for (int seg = 0; seg < 3; seg++) {
        int x0 = seg * 5;
        for (int sx = x0; sx < x0 + 5 && sx < 16; sx++) {
            buf[8 * 16 + sx] = C_BAG_HI;
            buf[9 * 16 + sx] = C_BAG_MID;
            buf[10 * 16 + sx] = C_BAG_DARK;
        }
        buf[9 * 16 + x0] = C_BLACK;
    }
    // Upper offset sandbag row
    for (int seg = 0; seg < 3; seg++) {
        int x0 = seg * 5 + 2;
        for (int sx = x0; sx < x0 + 5 && sx < 16; sx++) {
            buf[5 * 16 + sx] = C_BAG_HI;
            buf[6 * 16 + sx] = C_BAG_MID;
            buf[7 * 16 + sx] = C_BAG_DARK;
        }
    }
}

static uint8_t s_ground_top_cache8[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));
static uint16_t s_ground_top_cache[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));
static uint16_t s_ground_bottom_cache[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));

static void generate_ground_to_buffer(uint16_t *buffer, int y_offset) {
    if (!buffer) return;

    for (int ty = 0; ty < MAP_ROWS; ty++) {
        int global_ty = ty + (y_offset / TILE_SIZE);
        for (int tx = 0; tx < MAP_COLS; tx++) {
            int tid = 0;
            // Rich canonical Sector 1 composition:
            // 70% pristine concrete slabs, 10% grass in joints, 8% cracks, 6% oil stains,
            // 4% drainage sumideros, and tactical sandbags placed near corners
            uint32_t n = hash_noise(tx, global_ty, 777);

            if ((tx == 2 && (global_ty == 3 || global_ty == 15)) ||
                (tx == 13 && (global_ty == 4 || global_ty == 16))) {
                tid = 5; // Sandbag fortification
            } else if (n > 238) {
                tid = 1; // Grass tuft
            } else if (n < 25) {
                tid = 2; // Structural crack
            } else if (n >= 50 && n <= 65) {
                tid = 3; // Oil stain
            } else if (n >= 180 && n <= 190) {
                tid = 4; // Storm drain grate
            } else {
                tid = 0; // Base concrete slab
            }

            const uint16_t *src = s_tiles[tid];
            int px = tx * TILE_SIZE;
            int py = ty * TILE_SIZE;

            for (int y = 0; y < TILE_SIZE; y++) {
                uint16_t *dst = &buffer[(py + y) * SCREEN_W + px];
                for (int x = 0; x < TILE_SIZE; x++) {
                    dst[x] = src[y * TILE_SIZE + x];
                }
            }
        }
    }
}

static uint32_t s_dirty_mask_top[DIRTY_GRID_H];
static uint32_t s_dirty_mask_bot[2][DIRTY_GRID_H];

void tiles_dirty_clear(int is_bottom, int buf_idx) {
    if (is_bottom) {
        uint32_t *mask = s_dirty_mask_bot[buf_idx & 1];
        for (int i = 0; i < DIRTY_GRID_H; i++) mask[i] = 0;
    } else {
        for (int i = 0; i < DIRTY_GRID_H; i++) s_dirty_mask_top[i] = 0;
    }
}

void tiles_dirty_mark_rect(int x, int y, int w, int h, int is_bottom, int buf_idx) {
    if (w <= 0 || h <= 0) return;
    int x0 = x;
    int y0 = y;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x1 < 0 || y1 < 0 || x0 >= SCREEN_W || y0 >= SCREEN_H) return;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;

    int bx0 = x0 >> 3;
    int bx1 = x1 >> 3;
    int by0 = y0 >> 3;
    int by1 = y1 >> 3;

    uint32_t col_mask;
    if (bx1 - bx0 >= 31) {
        col_mask = 0xFFFFFFFFU;
    } else {
        col_mask = ((1U << (bx1 - bx0 + 1)) - 1U) << bx0;
    }

    if (is_bottom) {
        uint32_t *mask = s_dirty_mask_bot[buf_idx & 1];
        for (int by = by0; by <= by1; by++) {
            mask[by] |= col_mask;
        }
    } else {
        for (int by = by0; by <= by1; by++) {
            s_dirty_mask_top[by] |= col_mask;
        }
    }
}

ITCM_CODE __attribute__((target("arm"))) void tiles_dirty_restore(void *dst_buffer, int is_bottom, int buf_idx) {
    if (!dst_buffer) return;
    uint32_t *mask_array = is_bottom ? s_dirty_mask_bot[buf_idx & 1] : s_dirty_mask_top;

    if (is_bottom) {
        uint16_t *dst16 = (uint16_t *)dst_buffer;
        const uint16_t *src16 = s_ground_bottom_cache;
        for (int by = 0; by < DIRTY_GRID_H; by++) {
            uint32_t row_mask = mask_array[by];
            if (!row_mask) continue;

            int base_y = by << 3;
            int bx = 0;
            while (bx < DIRTY_GRID_W) {
                if (!(row_mask & (1U << bx))) {
                    bx++;
                    continue;
                }
                int start_bx = bx;
                while (bx < DIRTY_GRID_W && (row_mask & (1U << bx))) bx++;
                int end_bx = bx;

                int start_x = start_bx << 3;
                int bytes = (end_bx - start_bx) * 16; // 8 px * 2 bytes = 16 bytes per block
                for (int line = 0; line < 8; line++) {
                    int y = base_y + line;
                    memcpy(&dst16[y * SCREEN_W + start_x], &src16[y * SCREEN_W + start_x], bytes);
                }
            }
            mask_array[by] = 0;
        }
    } else {
        // 8-bit Top Screen: 8 bytes per block per line!
        uint8_t *dst8 = (uint8_t *)dst_buffer;
        const uint8_t *src8 = s_ground_top_cache8;
        for (int by = 0; by < DIRTY_GRID_H; by++) {
            uint32_t row_mask = mask_array[by];
            if (!row_mask) continue;

            int base_y = by << 3;
            int bx = 0;
            while (bx < DIRTY_GRID_W) {
                if (!(row_mask & (1U << bx))) {
                    bx++;
                    continue;
                }
                int start_bx = bx;
                while (bx < DIRTY_GRID_W && (row_mask & (1U << bx))) bx++;
                int end_bx = bx;

                int start_x = start_bx << 3;
                int bytes = (end_bx - start_bx) * 8; // 8 px * 1 byte = 8 bytes per block (50% LESS DATA!)
                for (int line = 0; line < 8; line++) {
                    int y = base_y + line;
                    memcpy(&dst8[y * SCREEN_W + start_x], &src8[y * SCREEN_W + start_x], bytes);
                }
            }
            mask_array[by] = 0;
        }
    }
}

void tiles_restore_ground_rect(uint16_t *dst_buffer, int x, int y, int w, int h, int is_bottom) {
    if (!dst_buffer) return;
    const uint16_t *src_cache = is_bottom ? s_ground_bottom_cache : s_ground_top_cache;
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : (x + w);
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : (y + h);
    if (x1 <= x0 || y1 <= y0) return;

    int align_x0 = x0 & ~1;
    int align_x1 = (x1 + 1) & ~1;
    int words = (align_x1 - align_x0) >> 1;

    for (int j = y0; j < y1; j++) {
        uint32_t *d32 = (uint32_t *)&dst_buffer[j * SCREEN_W + align_x0];
        const uint32_t *s32 = (const uint32_t *)&src_cache[j * SCREEN_W + align_x0];
        for (int k = 0; k < words; k++) {
            d32[k] = s32[k];
        }
    }
}

static inline void stamp_ground_pixel(int px, int py, uint16_t c16, uint8_t c8) {
    if (px < 0 || px >= SCREEN_W) return;
    int is_bottom = (py >= 192);
    int sy = is_bottom ? (py - 192) : py;
    if (sy < 0 || sy >= SCREEN_H) return;
    if (is_bottom) {
        s_ground_bottom_cache[sy * SCREEN_W + px] = c16;
        ((uint16_t *)VRAM_A)[sy * SCREEN_W + px] = c16;
        ((uint16_t *)VRAM_B)[sy * SCREEN_W + px] = c16;
    } else {
        s_ground_top_cache8[sy * SCREEN_W + px] = c8;
        if (g_top_backbuffer) {
            g_top_backbuffer[sy * SCREEN_W + px] = c8;
        }
    }
}

void tiles_stamp_splatter_directional(int x, int y, int size, int bvx, int bvy, int variant) {
    (void)variant;
    int is_bottom = (y >= 192);
    int sy = is_bottom ? (y - 192) : y;
    if (x < 2 || x >= SCREEN_W - 2 || sy < 2 || sy >= SCREEN_H - 2) return;

    // Compact base radii: individual kills leave moderate stains; dense carnage builds up with multiple kills!
    // size 1: small enemies (Scourge, Zergling) -> r_base = 2, max_r = 4 px
    // size 2: medium enemies (Hydra, Mutalisk) -> r_base = 3, max_r = 5 px
    // size 3: large enemies (Lurker, Guardian) -> r_base = 5, max_r = 7 px
    // size 4: colossus (Ultralisk) -> r_base = 7, max_r = 10 px
    int r_base = (size <= 1) ? 2 : ((size == 2) ? 3 : ((size == 3) ? 5 : 7));
    int max_r = r_base + (r_base >= 4 ? 3 : 2);

    // Direction vector from bullet velocity
    int len_sq = (bvx * bvx + bvy * bvy) >> 8;
    int dir_x = 0;
    int dir_y = -256; // Default forward momentum if zero (shots travel south to north)
    int has_bullet_dir = 0;

    if (len_sq > 16) {
        int len = 256;
        for (int it = 0; it < 5; it++) {
            if (len > 0) len = (len + len_sq / len) >> 1;
        }
        if (len > 0) {
            dir_x = (bvx << 8) / len;
            dir_y = (bvy << 8) / len;
            has_bullet_dir = 1;
        }
    }

    // Shift center slightly in bullet direction (1 to 2 px)
    int center_shift = (r_base / 2);
    if (center_shift < 1) center_shift = 1;
    if (center_shift > 2) center_shift = 2;
    if (has_bullet_dir) {
        x += (dir_x * center_shift) >> 8;
        sy += (dir_y * center_shift) >> 8;
    }

    int perp_x = -dir_y;
    int perp_y = dir_x;

    int x0 = (x - max_r < 0) ? 0 : (x - max_r);
    int x1 = (x + max_r >= SCREEN_W) ? SCREEN_W - 1 : (x + max_r);
    int y0 = (sy - max_r < 0) ? 0 : (sy - max_r);
    int y1 = (sy + max_r >= SCREEN_H) ? SCREEN_H - 1 : (sy + max_r);

    int r_core_sq = (r_base <= 2) ? 1 : (r_base * r_base / 4);
    int r_mid_sq  = r_base * r_base;
    int r_edge_sq = max_r * max_r;

    uint32_t seed = (uint32_t)(x * 73 + y * 179 + size * 31);

    for (int py = y0; py <= y1; py++) {
        int dy = py - sy;
        for (int px = x0; px <= x1; px++) {
            int dx = px - x;

            int d_long = 0;
            int d_lat  = 0;
            if (has_bullet_dir) {
                d_long = (dx * dir_x + dy * dir_y) >> 8;
                d_lat  = (dx * perp_x + dy * perp_y) >> 8;
            } else {
                d_long = dy;
                d_lat  = dx;
            }

            // Moderate elongation: 1.25x parallel
            int eff_long = (d_long * 205) >> 8;
            int eff_lat  = (d_lat * 270) >> 8;
            int eff_dist_sq = eff_long * eff_long + eff_lat * eff_lat;

            // Pseudo-random organic edge noise
            int noise = (((px * 13) ^ (py * 37) ^ (int)seed) & 15) - 7;
            int noisy_dist = eff_dist_sq + noise * (r_base / 2);

            int abs_py = is_bottom ? (py + 192) : py;
            if (noisy_dist <= r_core_sq) {
                // ZONE 1: WET CORE (Fresh magenta viscera)
                stamp_ground_pixel(px, abs_py, COLOR_XENOS_GORE_CORE, TOP_COLOR_XENOS_GORE_CORE);
            } else if (noisy_dist <= r_mid_sq) {
                // ZONE 2: VISCOUS MID (Dense purple ichor)
                stamp_ground_pixel(px, abs_py, COLOR_XENOS_GORE_MID, TOP_COLOR_XENOS_GORE_MID);
            } else if (noisy_dist <= r_edge_sq) {
                // ZONE 3: COAGULATED PERIMETER WITH BAYER DITHERING
                // 50% checkerboard pattern on outer margin
                if (((px + py) & 1) == 0) {
                    stamp_ground_pixel(px, abs_py, COLOR_XENOS_GORE_DARK, TOP_COLOR_XENOS_GORE_DARK);
                }
            }
        }
    }

    // Directional forward spray: strictly 1 or 2 micro-droplets
    if (has_bullet_dir && size >= 1) {
        int drop_count = (size >= 2) ? 2 : 1;
        for (int d = 0; d < drop_count; d++) {
            int spread = ((d * 47 + (int)seed) % 41) - 20;
            int dist = max_r + 2 + d * 3;
            int ox = (dir_x * dist + perp_x * spread) >> 8;
            int oy = (dir_y * dist + perp_y * spread) >> 8;
            int dpx = x + ox;
            int dpy = sy + oy;
            if (dpx >= 1 && dpx < SCREEN_W - 1 && dpy >= 1 && dpy < SCREEN_H - 1) {
                int abs_y = is_bottom ? (dpy + 192) : dpy;
                stamp_ground_pixel(dpx, abs_y, COLOR_XENOS_GORE_DARK, TOP_COLOR_XENOS_GORE_DARK);
            }
        }
    }
}

void tiles_stamp_particle_droplet(int x, int y, int size, uint16_t color) {
    if (x < 1 || x >= SCREEN_W - 1 || y < 1 || y >= FIELD_H - 1) return;

    if (size <= 0) {
        // 75% of light blood droplets dissipate in air without leaving a mark
        if ((rand() & 3) != 0) return;
        // 25% leave a single 1-pixel dark stain
        stamp_ground_pixel(x, y, COLOR_XENOS_GORE_DARK, TOP_COLOR_XENOS_GORE_DARK);
        return;
    }

    if (size == 1) {
        // Medium fragment: 50% chance of 1-pixel mark
        if ((rand() & 1) != 0) return;
        uint16_t c = (color & BIT(15)) ? color : COLOR_XENOS_GORE_MID;
        stamp_ground_pixel(x, y, c, TOP_COLOR_XENOS_GORE_MID);
        return;
    }

    // Heavy chunk (size >= 2): small 2-pixel mark
    stamp_ground_pixel(x, y, color, TOP_COLOR_XENOS_GORE_MID);
    stamp_ground_pixel(x + 1, y, COLOR_XENOS_GORE_DARK, TOP_COLOR_XENOS_GORE_DARK);
}

void tiles_stamp_splatter(int x, int y, int size, uint16_t color) {
    (void)color;
    tiles_stamp_splatter_directional(x, y, size, 0, 0, 0);
}



void tiles_full_screen_refresh(void) {
    if (g_top_backbuffer) {
        memcpy(g_top_backbuffer, s_ground_top_cache8, SCREEN_W * SCREEN_H);
    }
    renderer_refresh_top_vram();
    dmaCopyWords(2, s_ground_bottom_cache, (void *)VRAM_A, SCREEN_W * SCREEN_H * sizeof(uint16_t));
    dmaCopyWords(2, s_ground_bottom_cache, (void *)VRAM_B, SCREEN_W * SCREEN_H * sizeof(uint16_t));
    tiles_dirty_clear(0, 0);
    tiles_dirty_clear(1, 0);
    tiles_dirty_clear(1, 1);
}

void tiles_init(void) {
    generate_urban_slab(s_tiles[0], 101);
    generate_urban_grass(s_tiles[1]);
    generate_urban_crack(s_tiles[2]);
    generate_urban_oil(s_tiles[3]);
    generate_urban_drain(s_tiles[4]);
    generate_urban_sandbags(s_tiles[5]);

    generate_ground_to_buffer(s_ground_top_cache, 0);
    generate_ground_to_buffer(s_ground_bottom_cache, 192);

    // Pre-bake the military hazard range line across road (X: 32..224, Y: 64)
    int range_y = 64;
    for (int x = 32; x < 224; x++) {
        int is_amber = ((x / 4) % 2 == 0);
        uint16_t stripe_col = is_amber ? (RGB15(31, 22, 2) | BIT(15)) : (RGB15(6, 6, 8) | BIT(15));
        uint16_t shadow_col = RGB15(2, 2, 4) | BIT(15);
        uint16_t hi_col     = is_amber ? (RGB15(31, 28, 12) | BIT(15)) : (RGB15(12, 14, 16) | BIT(15));

        s_ground_bottom_cache[(range_y - 1) * SCREEN_W + x] = shadow_col;
        s_ground_bottom_cache[range_y * SCREEN_W + x]       = hi_col;
        s_ground_bottom_cache[(range_y + 1) * SCREEN_W + x] = stripe_col;

        if ((x % 16) == 0) {
            s_ground_bottom_cache[(range_y + 2) * SCREEN_W + x] = RGB15(31, 20, 0) | BIT(15);
            s_ground_bottom_cache[(range_y + 3) * SCREEN_W + x] = RGB15(24, 14, 0) | BIT(15);
        }
    }

    // Pre-bake the Wall Base Parapet into the bottom ground cache at canonical WALL_DEFAULT_Y (144)
    wall_draw_base(s_ground_bottom_cache, WALL_DEFAULT_Y, 100, 100);

    // Pre-bake the Bunker Ammo Depot Crate at (AMMO_DEPOT_X=128, AMMO_DEPOT_Y=166)
    int crate_x0 = AMMO_DEPOT_X - AMMO_CRATE_W / 2;
    int crate_y0 = AMMO_DEPOT_Y - AMMO_CRATE_H / 2;
    for (int dy = 0; dy < AMMO_CRATE_H; dy++) {
        int py = crate_y0 + dy + 1;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int dx = 0; dx < AMMO_CRATE_W; dx++) {
            int px = crate_x0 + dx + 1;
            if (px < 0 || px >= SCREEN_W) continue;
            if (c_ammo_crate_sprite[dy * AMMO_CRATE_W + dx] & BIT(15)) {
                s_ground_bottom_cache[py * SCREEN_W + px] = RGB15(1, 1, 2) | BIT(15);
            }
        }
    }
    for (int dy = 0; dy < AMMO_CRATE_H; dy++) {
        int py = crate_y0 + dy;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int dx = 0; dx < AMMO_CRATE_W; dx++) {
            int px = crate_x0 + dx;
            if (px < 0 || px >= SCREEN_W) continue;
            uint16_t c = c_ammo_crate_sprite[dy * AMMO_CRATE_W + dx];
            if (c & BIT(15)) {
                s_ground_bottom_cache[py * SCREEN_W + px] = c;
            }
        }
    }

    // Initial full copy to backbuffers
    
    // Build 8-bit top ground cache from 16-bit cache
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint16_t c16 = s_ground_top_cache[y * SCREEN_W + x];
            uint8_t idx = TOP_C_CONC_BASE;
            if (c16 == C_CONC_LIGHT) idx = TOP_C_CONC_LIGHT;
            else if (c16 == C_CONC_DARK) idx = TOP_C_CONC_DARK;
            else if (c16 == C_CONC_BEVEL) idx = TOP_C_CONC_BEVEL;
            else if (c16 == C_JOINT) idx = TOP_C_JOINT;
            else if (c16 == C_GRASS_DEEP) idx = TOP_C_GRASS_DEEP;
            else if (c16 == C_GRASS_MID) idx = TOP_C_GRASS_MID;
            else if (c16 == C_GRASS_TALL) idx = TOP_C_GRASS_TALL;
            else if (c16 == C_OIL_DARK) idx = TOP_C_OIL_DARK;
            else if (c16 == C_OIL_MID) idx = TOP_C_OIL_MID;
            else if (c16 == C_CRACK_LINE) idx = TOP_C_CRACK_LINE;
            else if (c16 == C_BAG_DARK) idx = TOP_C_BAG_DARK;
            else if (c16 == C_BAG_MID) idx = TOP_C_BAG_MID;
            else if (c16 == C_BAG_HI) idx = TOP_C_BAG_HI;
            else if (c16 == C_DRAIN_GRATE) idx = TOP_C_DRAIN_GRATE;
            else if (c16 == C_DRAIN_HOLE) idx = TOP_C_DRAIN_HOLE;
            s_ground_top_cache8[y * SCREEN_W + x] = idx;
        }
    }

    tiles_full_screen_refresh();
}

void tiles_render_urban_ground(uint16_t *buffer, int y_offset) {
    if (!buffer) return;
    const uint16_t *src = (y_offset == 0) ? s_ground_top_cache : s_ground_bottom_cache;
    dmaCopyWords(2, src, buffer, SCREEN_W * SCREEN_H * sizeof(uint16_t));
}


void tiles_draw_central_bunker(uint16_t *buffer, int cx, int cy, uint64_t hp, uint64_t max_hp) {
    if (!buffer) return;

    // Fortress Bunker: 64x42 px centered at (cx, cy)
    int bx = cx - 32;
    int by = cy - 20;

    for (int y = 0; y < 42; y++) {
        int py = by + y;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int x = 0; x < 64; x++) {
            int px = bx + x;
            if (px < 0 || px >= SCREEN_W) continue;

            uint16_t col = C_TURRET_DARK;
            if (x == 0 || x == 63 || y == 0 || y == 41) {
                col = C_BLACK; // Outer border
            } else if (y <= 3) {
                // Hazard upper blast lip
                int stripe = ((x + y) / 3) % 2;
                col = stripe ? C_HAZ_Y_HI : C_HAZ_BLACK;
            } else if (x >= 20 && x <= 43 && y >= 12 && y <= 35) {
                // Reinforced Sanctum Core vault
                if (x == 20 || x == 43 || y == 12 || y == 35) {
                    col = C_BRASS_HI;
                } else if (x >= 28 && x <= 35 && y >= 20 && y <= 27) {
                    col = C_MARS_RED_MID; // Eagle Crest
                } else {
                    col = C_TURRET_HI;
                }
            } else if ((x >= 8 && x <= 16 && y >= 14 && y <= 28) ||
                       (x >= 47 && x <= 55 && y >= 14 && y <= 28)) {
                // Ammo delivery ports
                col = (y % 3 == 0) ? C_BLACK : C_HAZ_Y_DARK;
            } else if (y >= 6 && y <= 10) {
                col = C_TURRET_MID;
            }

            buffer[py * SCREEN_W + px] = col;
        }
    }

    // Health Bar above bunker
    int bar_w = 56;
    int bar_x = cx - bar_w / 2;
    int bar_y = by - 8;
    if (bar_y >= 2) {
        // Outline
        for (int x = 0; x < bar_w; x++) {
            buffer[(bar_y - 1) * SCREEN_W + (bar_x + x)] = C_BLACK;
            buffer[(bar_y + 4) * SCREEN_W + (bar_x + x)] = C_BLACK;
        }
        for (int y = 0; y < 4; y++) {
            buffer[(bar_y + y) * SCREEN_W + (bar_x - 1)] = C_BLACK;
            buffer[(bar_y + y) * SCREEN_W + (bar_x + bar_w)] = C_BLACK;
        }

        int filled = 0;
        if (max_hp > 0) {
            filled = (int)((hp * bar_w) / max_hp);
            if (filled > bar_w) filled = bar_w;
        }

        uint16_t hp_col = RGB15(0, 30, 8) | BIT(15);
        if (hp * 3 < max_hp) hp_col = RGB15(31, 2, 2) | BIT(15);
        else if (hp * 2 < max_hp) hp_col = RGB15(31, 20, 0) | BIT(15);

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < bar_w; x++) {
                buffer[(bar_y + y) * SCREEN_W + (bar_x + x)] = (x < filled) ? hp_col : (RGB15(4, 4, 4) | BIT(15));
            }
        }
    }
}

void tiles_draw_xenos_to_buffer(uint16_t *buffer, int cx, int cy, int dir, int anim_frame, int variant) {
    enemy_draw_sprite_to_buffer(buffer, cx, cy, variant, anim_frame, dir, 0, NULL, NULL, NULL, NULL);
}

void tiles_draw_turret_base(int cx, int cy, int is_selected) {
    for (int dy = -9; dy <= 9; dy++) {
        for (int dx = -9; dx <= 9; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 <= 81) {
                renderer_draw_pixel(cx + dx, cy + dy, C_BLACK);
            }
            if (d2 <= 64) {
                uint16_t col = C_MARS_RED_MID;
                if ((abs(dx) >= 6 || abs(dy) >= 6) && ((dx + dy) % 3 == 0)) {
                    col = C_HAZ_Y_HI;
                } else if (dx + dy < -2) {
                    col = C_MARS_RED_HI;
                } else if (dx + dy > 2) {
                    col = C_MARS_RED_SHD;
                }
                renderer_draw_pixel(cx + dx, cy + dy, col);
            }
            if (d2 <= 36) {
                uint16_t col = C_TURRET_MID;
                if (dx + dy < -1) col = C_TURRET_HI;
                else if (dx + dy > 1) col = C_TURRET_DARK;
                renderer_draw_pixel(cx + dx, cy + dy, col);
            }
            if (d2 <= 9) {
                renderer_draw_pixel(cx + dx, cy + dy, C_BRASS_MID);
            }
        }
    }

    if (is_selected) {
        for (int dy = -11; dy <= 11; dy++) {
            for (int dx = -11; dx <= 11; dx++) {
                int d2 = dx * dx + dy * dy;
                if (d2 >= 100 && d2 <= 121) {
                    if ((abs(dx) <= 2) || (abs(dy) <= 2) || ((dx * dy) > 0)) {
                        renderer_draw_pixel(cx + dx, cy + dy, C_HAZ_Y_HI);
                    }
                }
            }
        }
    }
}

void tiles_draw_twin_bolters(int cx, int cy, int angle, int flash, int recoil_l, int recoil_r, int last_barrel) {
    (void)flash;
    (void)last_barrel;
    int ang = ((angle + 2) & ~3) & 0xFF;
    int fwd_x = fixed_cos(ang);
    int fwd_y = fixed_sin(ang);
    int perp_x = -fixed_sin(ang);
    int perp_y = fixed_cos(ang);

    int d_rec_l = (recoil_l >= 3) ? 4 : (recoil_l == 2 ? 2 : (recoil_l == 1 ? 1 : 0));
    int d_rec_r = (recoil_r >= 3) ? 4 : (recoil_r == 2 ? 2 : (recoil_r == 1 ? 1 : 0));

    int l_start = -2 - d_rec_l;
    int l_end = 11 - d_rec_l;
    int r_start = -2 - d_rec_r;
    int r_end = 11 - d_rec_r;

    for (int dy = -14; dy <= 14; dy++) {
        for (int dx = -14; dx <= 14; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;

            int dot_fwd = (dx * fwd_x + dy * fwd_y + 128) >> 8;
            int dot_perp = (dx * perp_x + dy * perp_y + 128) >> 8;

            // Breech / receiver box
            if (dot_fwd >= -5 && dot_fwd <= 2 && dot_perp >= -5 && dot_perp <= 5) {
                if (dot_fwd == -5 || dot_fwd == 2 || dot_perp == -5 || dot_perp == 5) {
                    renderer_draw_pixel(px, py, C_BLACK);
                } else if (dot_perp == 0) {
                    renderer_draw_pixel(px, py, C_BRASS_MID);
                } else {
                    renderer_draw_pixel(px, py, C_TURRET_MID);
                }
                continue;
            }

            // Left barrel
            if (dot_perp >= -4 && dot_perp <= -2) {
                if (dot_fwd >= l_start && dot_fwd <= l_end) {
                    if (dot_fwd == l_start || dot_fwd == l_end || dot_perp == -4 || dot_perp == -2) {
                        renderer_draw_pixel(px, py, C_BLACK);
                    } else {
                        renderer_draw_pixel(px, py, (dot_perp == -3) ? C_TURRET_HI : C_TURRET_MID);
                    }
                    continue;
                }
            }

            // Right barrel
            if (dot_perp >= 2 && dot_perp <= 4) {
                if (dot_fwd >= r_start && dot_fwd <= r_end) {
                    if (dot_fwd == r_start || dot_fwd == r_end || dot_perp == 2 || dot_perp == 4) {
                        renderer_draw_pixel(px, py, C_BLACK);
                    } else {
                        renderer_draw_pixel(px, py, (dot_perp == 3) ? C_TURRET_HI : C_TURRET_MID);
                    }
                    continue;
                }
            }
        }
    }

    // High velocity sparks from ejection ports when firing
    if (recoil_l > 0) {
        int sp_x = cx + ((fwd_x * 0 - perp_x * 5) >> 8);
        int sp_y = cy + ((fwd_y * 0 - perp_y * 5) >> 8);
        renderer_draw_pixel(sp_x, sp_y, C_SPARK_WHITE);
        renderer_draw_pixel(sp_x - 1, sp_y, C_SPARK_GOLD);
        renderer_draw_pixel(sp_x, sp_y - 1, C_SPARK_RED);
    }
    if (recoil_r > 0) {
        int sp_x = cx + ((fwd_x * 0 + perp_x * 5) >> 8);
        int sp_y = cy + ((fwd_y * 0 + perp_y * 5) >> 8);
        renderer_draw_pixel(sp_x, sp_y, C_SPARK_WHITE);
        renderer_draw_pixel(sp_x + 1, sp_y, C_SPARK_GOLD);
        renderer_draw_pixel(sp_x, sp_y - 1, C_SPARK_RED);
    }
}
