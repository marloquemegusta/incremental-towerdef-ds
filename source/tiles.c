#include "tiles.h"

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

#define C_TRENCH_SHD  (RGB15(0, 1, 1) | BIT(15))
#define C_TRENCH_BED  (RGB15(2, 2, 3) | BIT(15))
#define C_TRENCH_RIB  (RGB15(5, 5, 7) | BIT(15))

#define C_BRASS_HI    (RGB15(28, 24, 8) | BIT(15))
#define C_BRASS_MID   (RGB15(20, 15, 3) | BIT(15))
#define C_BRASS_DARK  (RGB15(10, 7, 1) | BIT(15))

#define C_TURRET_HI   (RGB15(18, 18, 20) | BIT(15))
#define C_TURRET_MID  (RGB15(11, 11, 13) | BIT(15))
#define C_TURRET_DARK (RGB15(5, 5, 7) | BIT(15))

#define C_MARS_RED_HI  (RGB15(29, 6, 6) | BIT(15))
#define C_MARS_RED_MID (RGB15(22, 3, 3) | BIT(15))
#define C_MARS_RED_SHD (RGB15(12, 1, 2) | BIT(15))

#define C_BONE_HI      (RGB15(31, 30, 28) | BIT(15))
#define C_BONE_MID     (RGB15(26, 25, 22) | BIT(15))
#define C_BONE_SHD     (RGB15(17, 16, 14) | BIT(15))
#define C_NEON_EYE     (RGB15(10, 31, 4) | BIT(15))
#define C_PURPLE_FLESH (RGB15(20, 4, 13) | BIT(15))
#define C_PURPLE_DARK  (RGB15(9, 2, 7) | BIT(15))

#define C_XENOS_CARAP (RGB15(14, 4, 10) | BIT(15))
#define C_XENOS_FLESH (RGB15(26, 10, 14) | BIT(15))
#define C_XENOS_EYE   (RGB15(31, 28, 2) | BIT(15))
#define C_XENOS_CLAW  (RGB15(8, 2, 5) | BIT(15))

#define C_SPARK_WHITE (RGB15(31, 31, 30) | BIT(15))
#define C_SPARK_GOLD  (RGB15(31, 25, 6) | BIT(15))
#define C_SPARK_RED   (RGB15(30, 10, 2) | BIT(15))

static uint16_t s_tiles[TILE_COUNT][TILE_SIZE * TILE_SIZE];

// The 16x12 Map Layout with a WIDE 32-pixel Seamless Trench
static const uint8_t s_stage_map[MAP_ROWS][MAP_COLS] = {
    // Row 0: Top Header UI reserve
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    // Row 1: Upper Bastion plates with vents
    { 0, 1, 0, 0, 2, 0, 0, 1, 0, 0, 0, 2, 0, 0, 1, 0 },
    // Row 2: Top Trench TOP HALF (Hazard on top only, turn 1 outer corner at tx=13)
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 0, 0 },
    // Row 3: Top Trench BOT HALF (Open turn at tx=12, right wall at tx=13)
    { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 7, 6, 0, 0 },
    // Row 4: Catwalk 1 (Turret platform: y=64..79) & Vertical Trench (tx=12,13)
    { 0, 1, 0, 0, 2, 0, 0, 1, 0, 0, 0, 2, 5, 6, 0, 0 },
    // Row 5: Middle Trench TOP HALF (Left wall at tx=2, open at tx=12, right wall at tx=13)
    { 0, 0, 11, 3, 3, 3, 3, 3, 3, 3, 3, 3, 7, 6, 0, 0 },
    // Row 6: Middle Trench BOT HALF (Left wall at tx=2, open at tx=3, turn 2 outer corner at tx=13)
    { 0, 0, 5, 7, 4, 4, 4, 4, 4, 4, 4, 4, 4, 9, 0, 0 },
    // Row 7: Catwalk 2 & Vertical Trench on left (tx=2,3)
    { 0, 0, 5, 6, 0, 1, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0 },
    // Row 8: Bottom Trench TOP HALF (Left wall at tx=2, open turn at tx=3 into final run)
    { 0, 0, 5, 7, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 16, 17 },
    // Row 9: Bottom Trench BOT HALF (Turn 4 outer corner at tx=2, enters Bunker at tx=14,15)
    { 0, 0, 10, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 18, 19 },
    // Row 10: Lower border
    { 0, 2, 0, 0, 0, 1, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0 },
    // Row 11: Bottom Dock UI reserve
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static void generate_floor_plate(uint16_t *buf) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col = C_FLOOR_MID;
            if (x == 0 || y == 0) col = C_SEAM;
            else if (x == 15 || y == 15) col = C_FLOOR_DARK;
            else if (x == 1 || y == 1) col = C_FLOOR_HI;
            else if ((x + y) % 6 == 0) col = C_FLOOR_DARK;

            if ((x == 3 && y == 3) || (x == 12 && y == 3) ||
                (x == 3 && y == 12) || (x == 12 && y == 12)) {
                col = C_RIVET;
            } else if ((x == 4 && y == 4) || (x == 13 && y == 4) ||
                       (x == 4 && y == 13) || (x == 13 && y == 13)) {
                col = C_RIVET_SHD;
            }
            buf[y * 16 + x] = col;
        }
    }
}

static void generate_floor_vent(uint16_t *buf) {
    generate_floor_plate(buf);
    for (int y = 4; y < 12; y++) {
        for (int x = 4; x < 12; x++) {
            uint16_t col = C_VENT_BG;
            if (y == 4 || x == 4) col = C_SEAM;
            else if (y % 2 == 0) col = C_VENT_FIN;
            buf[y * 16 + x] = col;
        }
    }
}

static void generate_floor_pipes(uint16_t *buf) {
    generate_floor_plate(buf);
    for (int x = 0; x < 16; x++) {
        buf[6 * 16 + x] = C_SEAM;
        buf[7 * 16 + x] = C_PIPE_HI;
        buf[8 * 16 + x] = C_PIPE_MID;
        buf[9 * 16 + x] = C_PIPE_DARK;
        buf[10 * 16 + x] = C_SEAM;

        if (x == 4 || x == 12) {
            buf[5 * 16 + x] = C_BRASS_HI;
            buf[6 * 16 + x] = C_BRASS_MID;
            buf[7 * 16 + x] = C_BRASS_MID;
            buf[8 * 16 + x] = C_BRASS_MID;
            buf[9 * 16 + x] = C_BRASS_MID;
            buf[10 * 16 + x] = C_BRASS_DARK;
            buf[11 * 16 + x] = C_BRASS_DARK;
        }
    }
}

static void generate_trench_open(uint16_t *buf) {
    // 100% open metal grating floor throughout the trench
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col = C_TRENCH_BED;
            if (x % 3 == 0) col = C_TRENCH_RIB;
            else if (y % 2 == 0) col = C_FLOOR_DARK;
            buf[y * 16 + x] = col;
        }
    }
}

static void generate_trench_h_top(uint16_t *buf) {
    // Top half of wide trench: Hazard stripe ONLY on top edge
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col = C_TRENCH_BED;
            if (y <= 2) {
                int stripe = ((x + y) / 3) % 2;
                col = stripe ? (y == 0 ? C_HAZ_Y_HI : C_HAZ_Y_MID) : C_HAZ_BLACK;
            } else if (y == 3) {
                col = C_TRENCH_SHD;
            } else if (y == 4) {
                col = C_BLACK;
            } else {
                if (x % 3 == 0) col = C_TRENCH_RIB;
                else if (y % 2 == 0) col = C_FLOOR_DARK;
            }
            buf[y * 16 + x] = col;
        }
    }
}

static void generate_trench_h_bot(uint16_t *buf) {
    // Bottom half of wide trench: Hazard stripe ONLY on bottom edge
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col = C_TRENCH_BED;
            if (y >= 13) {
                int stripe = ((x + y) / 3) % 2;
                col = stripe ? (y == 13 ? C_HAZ_Y_HI : C_HAZ_Y_MID) : C_HAZ_BLACK;
            } else {
                if (x % 3 == 0) col = C_TRENCH_RIB;
                else if (y % 2 == 0) col = C_FLOOR_DARK;
            }
            buf[y * 16 + x] = col;
        }
    }
}

static void generate_trench_v_left(uint16_t *buf) {
    // Left half of wide vertical trench: Hazard stripe ONLY on left edge
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col = C_TRENCH_BED;
            if (x <= 2) {
                int stripe = ((x + y) / 3) % 2;
                col = stripe ? (x == 0 ? C_HAZ_Y_HI : C_HAZ_Y_MID) : C_HAZ_BLACK;
            } else if (x == 3) {
                col = C_TRENCH_SHD;
            } else if (x == 4) {
                col = C_BLACK;
            } else {
                if (y % 3 == 0) col = C_TRENCH_RIB;
                else if (x % 2 == 0) col = C_FLOOR_DARK;
            }
            buf[y * 16 + x] = col;
        }
    }
}

static void generate_trench_v_right(uint16_t *buf) {
    // Right half of wide vertical trench: Hazard stripe ONLY on right edge
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint16_t col = C_TRENCH_BED;
            if (x >= 13) {
                int stripe = ((x + y) / 3) % 2;
                col = stripe ? (x == 13 ? C_HAZ_Y_HI : C_HAZ_Y_MID) : C_HAZ_BLACK;
            } else {
                if (y % 3 == 0) col = C_TRENCH_RIB;
                else if (x % 2 == 0) col = C_FLOOR_DARK;
            }
            buf[y * 16 + x] = col;
        }
    }
}

// Outer corner: Top & Right walls
static void generate_outer_tr(uint16_t *buf) {
    generate_trench_open(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y <= 2 || x >= 13) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            } else if (y == 3 || x == 12) {
                buf[y * 16 + x] = C_TRENCH_SHD;
            }
        }
    }
}

// Outer corner: Bottom & Right walls
static void generate_outer_br(uint16_t *buf) {
    generate_trench_open(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y >= 13 || x >= 13) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            }
        }
    }
}

// Outer corner: Bottom & Left walls
static void generate_outer_bl(uint16_t *buf) {
    generate_trench_open(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y >= 13 || x <= 2) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            } else if (x == 3 && y < 13) {
                buf[y * 16 + x] = C_TRENCH_SHD;
            }
        }
    }
}

// Outer corner: Top & Left walls
static void generate_outer_tl(uint16_t *buf) {
    generate_trench_open(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y <= 2 || x <= 2) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            } else if (y == 3 || x == 3) {
                buf[y * 16 + x] = C_TRENCH_SHD;
            }
        }
    }
}

// Catwalk corner tiles (Metal floor plate with hazard stripe on inner trench border)
static void generate_catwalk_br(uint16_t *buf) {
    generate_floor_plate(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y >= 13 || x >= 13) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            }
        }
    }
}

static void generate_catwalk_tr(uint16_t *buf) {
    generate_floor_plate(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y <= 2 || x >= 13) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            }
        }
    }
}

static void generate_catwalk_tl(uint16_t *buf) {
    generate_floor_plate(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y <= 2 || x <= 2) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            }
        }
    }
}

static void generate_catwalk_bl(uint16_t *buf) {
    generate_floor_plate(buf);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (y >= 13 || x <= 2) {
                int stripe = ((x + y) / 3) % 2;
                buf[y * 16 + x] = stripe ? C_HAZ_Y_MID : C_HAZ_BLACK;
            }
        }
    }
}

static void generate_bunker_tiles(void) {
    for (int q = 0; q < 4; q++) {
        uint16_t *buf = s_tiles[TILE_BUNKER_TL + q];
        int off_x = (q % 2) * 16;
        int off_y = (q / 2) * 16;

        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                int gx = x + off_x;
                int gy = y + off_y;
                uint16_t col = C_TURRET_DARK;

                if (gx <= 2 || gx >= 29 || gy <= 2 || gy >= 29) {
                    col = C_SEAM;
                } else if (gx == 3 || gy == 3) {
                    col = C_HAZ_Y_HI;
                } else if (gx == 28 || gy == 28) {
                    col = C_HAZ_BLACK;
                } else {
                    int dx = gx - 16;
                    int dy = gy - 16;
                    int d2 = dx * dx + dy * dy;
                    if (d2 <= 121) {
                        col = C_TURRET_MID;
                        if (d2 >= 100 && (dx + dy < 0)) col = C_TURRET_HI;
                        else if (d2 >= 100) col = C_TURRET_DARK;
                        if (d2 <= 16) col = C_BRASS_HI;
                        else if (d2 <= 25) col = C_BRASS_MID;
                    }
                }
                buf[y * 16 + x] = col;
            }
        }
    }
}

void tiles_init(void) {
    generate_floor_plate(s_tiles[TILE_FLOOR_PLATE]);
    generate_floor_vent(s_tiles[TILE_FLOOR_VENT]);
    generate_floor_pipes(s_tiles[TILE_FLOOR_PIPES]);

    generate_trench_h_top(s_tiles[TILE_TRENCH_H_TOP]);
    generate_trench_h_bot(s_tiles[TILE_TRENCH_H_BOT]);
    generate_trench_v_left(s_tiles[TILE_TRENCH_V_LEFT]);
    generate_trench_v_right(s_tiles[TILE_TRENCH_V_RIGHT]);
    generate_trench_open(s_tiles[TILE_TRENCH_OPEN]);

    generate_outer_tr(s_tiles[TILE_OUTER_TR]);
    generate_outer_br(s_tiles[TILE_OUTER_BR]);
    generate_outer_bl(s_tiles[TILE_OUTER_BL]);
    generate_outer_tl(s_tiles[TILE_OUTER_TL]);

    generate_catwalk_br(s_tiles[TILE_CATWALK_BR]);
    generate_catwalk_tr(s_tiles[TILE_CATWALK_TR]);
    generate_catwalk_tl(s_tiles[TILE_CATWALK_TL]);
    generate_catwalk_bl(s_tiles[TILE_CATWALK_BL]);

    generate_bunker_tiles();
}

void tiles_render_map(void) {
    for (int ty = 0; ty < MAP_ROWS; ty++) {
        for (int tx = 0; tx < MAP_COLS; tx++) {
            uint8_t tid = s_stage_map[ty][tx];
            const uint16_t *src = s_tiles[tid];
            int px = tx * TILE_SIZE;
            int py = ty * TILE_SIZE;

            for (int y = 0; y < TILE_SIZE; y++) {
                uint16_t *dst = &g_backbuffer[(py + y) * SCREEN_W + px];
                for (int x = 0; x < TILE_SIZE; x++) {
                    dst[x] = src[y * TILE_SIZE + x];
                }
            }
        }
    }
}

void tiles_render_sector1_map(void) {
    for (int ty = 0; ty < MAP_ROWS; ty++) {
        for (int tx = 0; tx < MAP_COLS; tx++) {
            uint8_t tid = g_s1_map[ty][tx];
            const uint16_t *src = g_s1_tiles[tid];
            int px = tx * TILE_SIZE;
            int py = ty * TILE_SIZE;

            for (int y = 0; y < TILE_SIZE; y++) {
                uint16_t *dst = &g_backbuffer[(py + y) * SCREEN_W + px];
                for (int x = 0; x < TILE_SIZE; x++) {
                    dst[x] = src[y * TILE_SIZE + x];
                }
            }
        }
    }
}

void tiles_draw_turret_base(int cx, int cy, int is_selected) {
    for (int dy = -9; dy <= 9; dy++) {
        for (int dx = -9; dx <= 9; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 <= 81) {
                // Black 1px outline
                renderer_draw_pixel(cx + dx, cy + dy, C_BLACK);
            }
            if (d2 <= 64) {
                // Mars Red Cog ring with hazard yellow teeth
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
                // Shaded Gunmetal Steel Cupola
                uint16_t col = C_TURRET_MID;
                if (dx + dy < -1) col = C_TURRET_HI;
                else if (dx + dy > 1) col = C_TURRET_DARK;
                renderer_draw_pixel(cx + dx, cy + dy, col);
            }
            if (d2 <= 9) {
                // Brass Central Bearing
                uint16_t col = (dx + dy < 0) ? C_BRASS_HI : C_BRASS_MID;
                renderer_draw_pixel(cx + dx, cy + dy, col);
            }
        }
    }

    // Golden Ammo Drum attached to left/top side of the cupola
    renderer_fill_rect(cx - 6, cy - 8, 5, 3, C_BRASS_MID);
    renderer_draw_rect(cx - 6, cy - 8, 5, 3, C_BLACK);
    renderer_draw_line(cx - 5, cy - 7, cx - 2, cy - 7, C_BRASS_HI);
    renderer_draw_line(cx - 3, cy - 5, cx - 3, cy - 3, C_BRASS_MID); // feed chute

    if (is_selected) {
        renderer_draw_rect(cx - 10, cy - 10, 21, 21, C_HAZ_Y_HI);
    }
}

void tiles_draw_twin_bolters(int cx, int cy, int angle, int flash, int recoil_l, int recoil_r, int last_barrel) {
    (void)flash;
    (void)last_barrel;
    int ang = angle & 0xFF;
    int fwd_x = fixed_cos(ang);
    int fwd_y = fixed_sin(ang);
    int perp_x = -fixed_sin(ang);
    int perp_y = fixed_cos(ang);

    // Explosive non-linear ballistic curve: [4, 4, 2, 1, 0]
    int d_rec_l = (recoil_l >= 3) ? 4 : (recoil_l == 2 ? 2 : (recoil_l == 1 ? 1 : 0));
    int d_rec_r = (recoil_r >= 3) ? 4 : (recoil_r == 2 ? 2 : (recoil_r == 1 ? 1 : 0));

    // 1. LEFT BARREL & CARRIAGE (MASSIVE CHUNKY: 3-pixel wide solid metal + outlines)
    // Entire carriage block from cupola (-2) to tip (+11) slides back rigidly by d_rec_l pixels
    int l_start = -2 - d_rec_l;
    int l_end = 11 - d_rec_l;

    // Outlines (top at -5, bottom at -1)
    int l_outlines[2] = {-5, -1};
    for (int k = 0; k < 2; k++) {
        int off = l_outlines[k];
        int x0 = cx + ((perp_x * off + fwd_x * l_start) >> FP_SHIFT);
        int y0 = cy + ((perp_y * off + fwd_y * l_start) >> FP_SHIFT);
        int x1 = cx + ((perp_x * off + fwd_x * l_end) >> FP_SHIFT);
        int y1 = cy + ((perp_y * off + fwd_y * l_end) >> FP_SHIFT);
        renderer_draw_line(x0, y0, x1, y1, C_BLACK);
    }
    // 3px Solid Metal Body (offsets -4, -3, -2)
    for (int off = -4; off <= -2; off++) {
        int x0 = cx + ((perp_x * off + fwd_x * l_start) >> FP_SHIFT);
        int y0 = cy + ((perp_y * off + fwd_y * l_start) >> FP_SHIFT);
        int x1 = cx + ((perp_x * off + fwd_x * (l_end - 1)) >> FP_SHIFT);
        int y1 = cy + ((perp_y * off + fwd_y * (l_end - 1)) >> FP_SHIFT);
        uint16_t col = (off == -4) ? C_PIPE_HI : ((off == -3) ? C_PIPE_MID : C_FLOOR_DARK);
        renderer_draw_line(x0, y0, x1, y1, col);
    }
    // Left heavy muzzle brake cap
    {
        int mx0 = cx + ((perp_x * -5 + fwd_x * l_end) >> FP_SHIFT);
        int my0 = cy + ((perp_y * -5 + fwd_y * l_end) >> FP_SHIFT);
        int mx1 = cx + ((perp_x * -1 + fwd_x * l_end) >> FP_SHIFT);
        int my1 = cy + ((perp_y * -1 + fwd_y * l_end) >> FP_SHIFT);
        renderer_draw_line(mx0, my0, mx1, my1, C_BLACK);
    }

    // 2. RIGHT BARREL & CARRIAGE (MASSIVE CHUNKY: 3-pixel wide solid metal + outlines)
    int r_start = -2 - d_rec_r;
    int r_end = 11 - d_rec_r;

    // Outlines (top at +1, bottom at +5)
    int r_outlines[2] = {1, 5};
    for (int k = 0; k < 2; k++) {
        int off = r_outlines[k];
        int x0 = cx + ((perp_x * off + fwd_x * r_start) >> FP_SHIFT);
        int y0 = cy + ((perp_y * off + fwd_y * r_start) >> FP_SHIFT);
        int x1 = cx + ((perp_x * off + fwd_x * r_end) >> FP_SHIFT);
        int y1 = cy + ((perp_y * off + fwd_y * r_end) >> FP_SHIFT);
        renderer_draw_line(x0, y0, x1, y1, C_BLACK);
    }
    // 3px Solid Metal Body (offsets +2, +3, +4)
    for (int off = 2; off <= 4; off++) {
        int x0 = cx + ((perp_x * off + fwd_x * r_start) >> FP_SHIFT);
        int y0 = cy + ((perp_y * off + fwd_y * r_start) >> FP_SHIFT);
        int x1 = cx + ((perp_x * off + fwd_x * (r_end - 1)) >> FP_SHIFT);
        int y1 = cy + ((perp_y * off + fwd_y * (r_end - 1)) >> FP_SHIFT);
        uint16_t col = (off == 2) ? C_PIPE_HI : ((off == 3) ? C_PIPE_MID : C_FLOOR_DARK);
        renderer_draw_line(x0, y0, x1, y1, col);
    }
    // Right heavy muzzle brake cap
    {
        int mx0 = cx + ((perp_x * 1 + fwd_x * r_end) >> FP_SHIFT);
        int my0 = cy + ((perp_y * 1 + fwd_y * r_end) >> FP_SHIFT);
        int mx1 = cx + ((perp_x * 5 + fwd_x * r_end) >> FP_SHIFT);
        int my1 = cy + ((perp_y * 5 + fwd_y * r_end) >> FP_SHIFT);
        renderer_draw_line(mx0, my0, mx1, my1, C_BLACK);
    }

    // 3. OPTION 3: FRICTION SPARKS FROM BOLT EJECTION PORTS
    if (recoil_l >= 3) {
        // Frame 1: Snap fire - white, gold, red sparks leaping back-left
        int sx1 = cx + ((perp_x * -6 + fwd_x * (l_start - 1)) >> FP_SHIFT);
        int sy1 = cy + ((perp_y * -6 + fwd_y * (l_start - 1)) >> FP_SHIFT);
        int sx2 = cx + ((perp_x * -7 + fwd_x * (l_start - 2)) >> FP_SHIFT);
        int sy2 = cy + ((perp_y * -7 + fwd_y * (l_start - 2)) >> FP_SHIFT);
        int sx3 = cx + ((perp_x * -8 + fwd_x * (l_start - 3)) >> FP_SHIFT);
        int sy3 = cy + ((perp_y * -8 + fwd_y * (l_start - 3)) >> FP_SHIFT);
        renderer_draw_pixel(sx1, sy1, C_SPARK_WHITE);
        renderer_draw_pixel(sx2, sy2, C_SPARK_GOLD);
        renderer_draw_pixel(sx3, sy3, C_SPARK_RED);
    } else if (recoil_l == 2) {
        // Frame 2: Peak hold - trailing gold & cooling red sparks
        int sx1 = cx + ((perp_x * -8 + fwd_x * (l_start - 3)) >> FP_SHIFT);
        int sy1 = cy + ((perp_y * -8 + fwd_y * (l_start - 3)) >> FP_SHIFT);
        int sx2 = cx + ((perp_x * -9 + fwd_x * (l_start - 4)) >> FP_SHIFT);
        int sy2 = cy + ((perp_y * -9 + fwd_y * (l_start - 4)) >> FP_SHIFT);
        renderer_draw_pixel(sx1, sy1, C_SPARK_GOLD);
        renderer_draw_pixel(sx2, sy2, C_SPARK_RED);
    }

    if (recoil_r >= 3) {
        // Frame 1: Snap fire - white, gold, red sparks leaping back-right
        int sx1 = cx + ((perp_x * 6 + fwd_x * (r_start - 1)) >> FP_SHIFT);
        int sy1 = cy + ((perp_y * 6 + fwd_y * (r_start - 1)) >> FP_SHIFT);
        int sx2 = cx + ((perp_x * 7 + fwd_x * (r_start - 2)) >> FP_SHIFT);
        int sy2 = cy + ((perp_y * 7 + fwd_y * (r_start - 2)) >> FP_SHIFT);
        int sx3 = cx + ((perp_x * 8 + fwd_x * (r_start - 3)) >> FP_SHIFT);
        int sy3 = cy + ((perp_y * 8 + fwd_y * (r_start - 3)) >> FP_SHIFT);
        renderer_draw_pixel(sx1, sy1, C_SPARK_WHITE);
        renderer_draw_pixel(sx2, sy2, C_SPARK_GOLD);
        renderer_draw_pixel(sx3, sy3, C_SPARK_RED);
    } else if (recoil_r == 2) {
        // Frame 2: Peak hold - trailing gold & cooling red sparks
        int sx1 = cx + ((perp_x * 8 + fwd_x * (r_start - 3)) >> FP_SHIFT);
        int sy1 = cy + ((perp_y * 8 + fwd_y * (r_start - 3)) >> FP_SHIFT);
        int sx2 = cx + ((perp_x * 9 + fwd_x * (r_start - 4)) >> FP_SHIFT);
        int sy2 = cy + ((perp_y * 9 + fwd_y * (r_start - 4)) >> FP_SHIFT);
        renderer_draw_pixel(sx1, sy1, C_SPARK_GOLD);
        renderer_draw_pixel(sx2, sy2, C_SPARK_RED);
    }
}

void tiles_draw_xenos(int cx, int cy, int dir, int anim_frame) {
    int leg = anim_frame ? 1 : 0;

    if (dir == 1) {
        // Heading SOUTH (+Y)
        // Symmetrical scuttling legs on left & right
        renderer_draw_pixel(cx - 3, cy - 2 + leg, C_BLACK);
        renderer_draw_pixel(cx - 3, cy + 1 - leg, C_BLACK);
        renderer_draw_pixel(cx + 3, cy - 2 + leg, C_BLACK);
        renderer_draw_pixel(cx + 3, cy + 1 - leg, C_BLACK);

        // Body outline & Purple organic flanks
        renderer_draw_rect(cx - 2, cy - 3, 5, 7, C_BLACK);
        renderer_fill_rect(cx - 1, cy - 2, 3, 5, C_PURPLE_FLESH);

        // Segmented Bone Chitin Carapace
        renderer_draw_line(cx - 1, cy - 2, cx - 1, cy + 1, C_BONE_HI);
        renderer_draw_line(cx, cy - 2, cx, cy + 1, C_BONE_MID);
        renderer_draw_line(cx + 1, cy - 2, cx + 1, cy + 1, C_BONE_SHD);

        // Neon Green Eyes & Mandibles pointing SOUTH (+Y)
        renderer_draw_pixel(cx - 1, cy + 3, C_NEON_EYE);
        renderer_draw_pixel(cx + 1, cy + 3, C_NEON_EYE);
        renderer_draw_pixel(cx - 1, cy + 4, C_BONE_HI);
        renderer_draw_pixel(cx + 1, cy + 4, C_BONE_HI);
        renderer_draw_pixel(cx, cy - 4, C_PURPLE_DARK); // tail
    } else if (dir == 2) {
        // Heading WEST (-X)
        // Legs top & bot
        renderer_draw_pixel(cx - 1 + leg, cy - 3, C_BLACK);
        renderer_draw_pixel(cx + 2 - leg, cy - 3, C_BLACK);
        renderer_draw_pixel(cx - 1 + leg, cy + 3, C_BLACK);
        renderer_draw_pixel(cx + 2 - leg, cy + 3, C_BLACK);

        renderer_draw_rect(cx - 2, cy - 2, 5, 5, C_BLACK);
        renderer_fill_rect(cx - 1, cy - 1, 3, 3, C_PURPLE_FLESH);

        // Bone Carapace
        renderer_draw_line(cx - 1, cy - 1, cx + 2, cy - 1, C_BONE_HI);
        renderer_draw_line(cx - 1, cy, cx + 2, cy, C_BONE_MID);
        renderer_draw_line(cx - 1, cy + 1, cx + 2, cy + 1, C_BONE_SHD);

        // Neon Green Eyes & Mandibles pointing WEST (-X)
        renderer_draw_pixel(cx - 3, cy - 1, C_NEON_EYE);
        renderer_draw_pixel(cx - 3, cy + 1, C_NEON_EYE);
        renderer_draw_pixel(cx - 4, cy - 1, C_BONE_HI);
        renderer_draw_pixel(cx - 4, cy + 1, C_BONE_HI);
        renderer_draw_pixel(cx + 3, cy, C_PURPLE_DARK); // tail
    } else {
        // Heading EAST (+X) (default / dir == 0)
        // Legs top & bot
        renderer_draw_pixel(cx - 2 + leg, cy - 3, C_BLACK);
        renderer_draw_pixel(cx + 1 - leg, cy - 3, C_BLACK);
        renderer_draw_pixel(cx - 2 + leg, cy + 3, C_BLACK);
        renderer_draw_pixel(cx + 1 - leg, cy + 3, C_BLACK);

        renderer_draw_rect(cx - 3, cy - 2, 5, 5, C_BLACK);
        renderer_fill_rect(cx - 2, cy - 1, 3, 3, C_PURPLE_FLESH);

        // Bone Carapace
        renderer_draw_line(cx - 2, cy - 1, cx + 1, cy - 1, C_BONE_HI);
        renderer_draw_line(cx - 2, cy, cx + 1, cy, C_BONE_MID);
        renderer_draw_line(cx - 2, cy + 1, cx + 1, cy + 1, C_BONE_SHD);

        // Neon Green Eyes & Mandibles pointing EAST (+X)
        renderer_draw_pixel(cx + 3, cy - 1, C_NEON_EYE);
        renderer_draw_pixel(cx + 3, cy + 1, C_NEON_EYE);
        renderer_draw_pixel(cx + 4, cy - 1, C_BONE_HI);
        renderer_draw_pixel(cx + 4, cy + 1, C_BONE_HI);
        renderer_draw_pixel(cx - 4, cy, C_PURPLE_DARK); // tail
    }
}
