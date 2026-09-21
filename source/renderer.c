#include "game.h"
#include "tiles.h"
#include "enemy_data.h"

// Hardware VRAM backbuffers
uint16_t *g_backbuffer = NULL;
static uint8_t s_top_backbuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4))); // 48 KB 8-bit backbuffer!
uint8_t *g_top_backbuffer = s_top_backbuffer;
static int s_bot_fb_idx = 1; // Start drawing into VRAM_B while FB0 displays VRAM_A

static u16 *s_top_vram = NULL;
static int s_top_bg = 0;



#include "turret_data.h"

void format_number_compact(char *buf, size_t buf_size, uint64_t val) {
    if (val >= 1000000000ULL) {
        snprintf(buf, buf_size, "%lluB", val / 1000000000ULL);
    } else if (val >= 1000000ULL) {
        snprintf(buf, buf_size, "%lluM", val / 1000000ULL);
    } else if (val >= 1000ULL) {
        snprintf(buf, buf_size, "%lluK", val / 1000ULL);
    } else {
        snprintf(buf, buf_size, "%llu", val);
    }
}

// Font 4x6
static const uint8_t font4x6[128][6] = {
    [' '] = {0, 0, 0, 0, 0, 0},
    ['0'] = {0x6, 0x9, 0x9, 0x9, 0x6, 0},
    ['1'] = {0x2, 0x6, 0x2, 0x2, 0x7, 0},
    ['2'] = {0x6, 0x9, 0x2, 0x4, 0xF, 0},
    ['3'] = {0xE, 0x1, 0x6, 0x1, 0xE, 0},
    ['4'] = {0x9, 0x9, 0xF, 0x1, 0x1, 0},
    ['5'] = {0xF, 0x8, 0xE, 0x1, 0xE, 0},
    ['6'] = {0x6, 0x8, 0xE, 0x9, 0x6, 0},
    ['7'] = {0xF, 0x1, 0x2, 0x4, 0x4, 0},
    ['8'] = {0x6, 0x9, 0x6, 0x9, 0x6, 0},
    ['9'] = {0x6, 0x9, 0x7, 0x1, 0x6, 0},
    ['A'] = {0x6, 0x9, 0xF, 0x9, 0x9, 0},
    ['B'] = {0xE, 0x9, 0xE, 0x9, 0xE, 0},
    ['C'] = {0x7, 0x8, 0x8, 0x8, 0x7, 0},
    ['D'] = {0xE, 0x9, 0x9, 0x9, 0xE, 0},
    ['E'] = {0xF, 0x8, 0xE, 0x8, 0xF, 0},
    ['F'] = {0xF, 0x8, 0xE, 0x8, 0x8, 0},
    ['G'] = {0x7, 0x8, 0xB, 0x9, 0x7, 0},
    ['H'] = {0x9, 0x9, 0xF, 0x9, 0x9, 0},
    ['I'] = {0x7, 0x2, 0x2, 0x2, 0x7, 0},
    ['J'] = {0x1, 0x1, 0x1, 0x9, 0x6, 0},
    ['K'] = {0x9, 0xA, 0xC, 0xA, 0x9, 0},
    ['L'] = {0x8, 0x8, 0x8, 0x8, 0xF, 0},
    ['M'] = {0x9, 0xF, 0xD, 0x9, 0x9, 0},
    ['N'] = {0x9, 0xD, 0xB, 0x9, 0x9, 0},
    ['O'] = {0x6, 0x9, 0x9, 0x9, 0x6, 0},
    ['P'] = {0xE, 0x9, 0xE, 0x8, 0x8, 0},
    ['Q'] = {0x6, 0x9, 0x9, 0xA, 0x5, 0},
    ['R'] = {0xE, 0x9, 0xE, 0xA, 0x9, 0},
    ['S'] = {0x7, 0x8, 0x6, 0x1, 0xE, 0},
    ['T'] = {0x7, 0x2, 0x2, 0x2, 0x2, 0},
    ['U'] = {0x9, 0x9, 0x9, 0x9, 0x6, 0},
    ['V'] = {0x9, 0x9, 0x9, 0x5, 0x2, 0},
    ['W'] = {0x9, 0x9, 0xD, 0xF, 0x9, 0},
    ['X'] = {0x9, 0x5, 0x2, 0x5, 0x9, 0},
    ['Y'] = {0x5, 0x5, 0x2, 0x2, 0x2, 0},
    ['Z'] = {0xF, 0x1, 0x2, 0x4, 0xF, 0},
    [':'] = {0x0, 0x2, 0x0, 0x2, 0x0, 0},
    ['/'] = {0x1, 0x2, 0x4, 0x8, 0x0, 0},
    ['%'] = {0x9, 0x2, 0x4, 0x9, 0x0, 0},
    ['.'] = {0x0, 0x0, 0x0, 0x0, 0x2, 0},
    ['+'] = {0x0, 0x2, 0x7, 0x2, 0x0, 0},
    ['-'] = {0x0, 0x0, 0x7, 0x0, 0x0, 0},
    ['('] = {0x2, 0x4, 0x4, 0x4, 0x2, 0},
    [')'] = {0x4, 0x2, 0x2, 0x2, 0x4, 0},
    ['x'] = {0x0, 0x5, 0x2, 0x5, 0x0, 0},
    ['$'] = {0x7, 0xA, 0x7, 0x2, 0x7, 0},
    ['!'] = {0x2, 0x2, 0x2, 0x0, 0x2, 0},
    ['<'] = {0x2, 0x4, 0x8, 0x4, 0x2, 0},
    ['>'] = {0x8, 0x4, 0x2, 0x4, 0x8, 0},
};

void renderer_init(void) {
    // 1. Bottom Screen: Main engine in Direct FB mode with double VRAM banks (A & B)
    lcdMainOnBottom();
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    videoSetMode(MODE_FB0); // Initially display VRAM_A

    // 2. Top Screen: Sub engine Native 8-bit Mode 5 (BgType_Bmp8)
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    s_top_bg = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    s_top_vram = (u16 *)bgGetGfxPtr(s_top_bg);

    // Setup Sub BG 256-color palette
    for (int i = 0; i < 256; i++) {
        BG_PALETTE_SUB[i] = g_enemy_palette[i];
    }
    BG_PALETTE_SUB[TOP_COLOR_TRANSPARENT]    = 0;
    BG_PALETTE_SUB[TOP_C_CONC_DARK]          = RGB15(4, 5, 6);
    BG_PALETTE_SUB[TOP_C_JOINT]              = RGB15(7, 9, 10);
    BG_PALETTE_SUB[TOP_C_CONC_BASE]          = RGB15(10, 14, 14);
    BG_PALETTE_SUB[TOP_C_CONC_LIGHT]         = RGB15(16, 18, 18);
    BG_PALETTE_SUB[TOP_C_CONC_BEVEL]         = RGB15(21, 22, 22);
    BG_PALETTE_SUB[TOP_C_OIL_DARK]           = RGB15(2, 3, 5);
    BG_PALETTE_SUB[TOP_C_DRAIN_HOLE]         = RGB15(2, 4, 7);

    BG_PALETTE_SUB[TOP_COLOR_BLACK]          = RGB15(0, 0, 0);
    BG_PALETTE_SUB[TOP_COLOR_WHITE]          = RGB15(31, 31, 31);
    BG_PALETTE_SUB[TOP_COLOR_AMBER]          = RGB15(31, 22, 2);
    BG_PALETTE_SUB[TOP_COLOR_LED_GREEN]      = RGB15(2, 31, 4);
    BG_PALETTE_SUB[TOP_COLOR_LED_RED]        = RGB15(31, 2, 2);
    BG_PALETTE_SUB[TOP_COLOR_IRON_LIGHT]     = RGB15(16, 17, 18);
    BG_PALETTE_SUB[TOP_COLOR_IRON_PANEL]     = RGB15(10, 11, 12);
    BG_PALETTE_SUB[TOP_COLOR_IRON_BORDER]    = RGB15(6, 6, 7);
    BG_PALETTE_SUB[TOP_COLOR_PHOSPHOR_GREEN] = RGB15(2, 31, 6);
    BG_PALETTE_SUB[TOP_COLOR_BLOOD]          = RGB15(16, 2, 8);
    BG_PALETTE_SUB[TOP_COLOR_DARK_GRAY]      = RGB15(6, 6, 7);
    BG_PALETTE_SUB[TOP_COLOR_XENOS_GORE_CORE]= COLOR_XENOS_GORE_CORE;
    BG_PALETTE_SUB[TOP_COLOR_XENOS_GORE_MID] = COLOR_XENOS_GORE_MID;
    BG_PALETTE_SUB[TOP_COLOR_XENOS_GORE_DARK]= COLOR_XENOS_GORE_DARK;

    s_bot_fb_idx = 1;
    g_backbuffer = (uint16_t *)VRAM_B;
    g_top_backbuffer = s_top_backbuffer;

    // 3. Asset generator
    tiles_init();

    // 4. Initial ground load into all VRAM buffers
    tiles_full_screen_refresh();
}

void renderer_clear(uint16_t color) {
    uint32_t color32 = ((uint32_t)color << 16) | color;
    uint32_t *dest = (uint32_t *)g_backbuffer;
    int count = (SCREEN_W * SCREEN_H) / 2;
    for (int i = 0; i < count; i++) {
        dest[i] = color32;
    }
}

void renderer_draw_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        g_backbuffer[y * SCREEN_W + x] = color;
    }
}

void top_draw_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        g_top_backbuffer[y * SCREEN_W + x] = color;
    }
}

void renderer_draw_rect(int x, int y, int w, int h, uint16_t color) {
    for (int i = x; i < x + w; i++) {
        renderer_draw_pixel(i, y, color);
        renderer_draw_pixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        renderer_draw_pixel(x, j, color);
        renderer_draw_pixel(x + w - 1, j, color);
    }
}

void renderer_fill_rect(int x, int y, int w, int h, uint16_t color) {
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : (x + w);
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : (y + h);
    for (int j = y0; j < y1; j++) {
        uint16_t *line = &g_backbuffer[j * SCREEN_W];
        for (int i = x0; i < x1; i++) {
            line[i] = color;
        }
    }
}

void top_fill_rect(int x, int y, int w, int h, uint8_t color) {
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : (x + w);
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : (y + h);
    for (int j = y0; j < y1; j++) {
        uint8_t *line = &g_top_backbuffer[j * SCREEN_W];
        memset(&line[x0], color, x1 - x0);
    }
}

void renderer_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        renderer_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void renderer_draw_circle(int cx, int cy, int radius, uint16_t color, int filled) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            int d2 = x * x + y * y;
            if (filled) {
                if (d2 <= radius * radius) {
                    renderer_draw_pixel(cx + x, cy + y, color);
                }
            } else {
                if (d2 <= radius * radius && d2 >= (radius - 1) * (radius - 1)) {
                    renderer_draw_pixel(cx + x, cy + y, color);
                }
            }
        }
    }
}

void renderer_draw_text(int x, int y, const char *str, uint16_t color) {
    while (*str) {
        char c = *str++;
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if ((unsigned char)c < 128) {
            for (int row = 0; row < 6; row++) {
                uint8_t bits = font4x6[(unsigned char)c][row];
                for (int col = 0; col < 4; col++) {
                    if (bits & (1 << (3 - col))) {
                        renderer_draw_pixel(x + col, y + row, color);
                    }
                }
            }
        }
        x += 5;
    }
}

void top_draw_text(int x, int y, const char *str, uint8_t color) {
    while (*str) {
        char c = *str++;
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if ((unsigned char)c < 128) {
            for (int row = 0; row < 6; row++) {
                uint8_t bits = font4x6[(unsigned char)c][row];
                for (int col = 0; col < 4; col++) {
                    if (bits & (1 << (3 - col))) {
                        top_draw_pixel(x + col, y + row, color);
                    }
                }
            }
        }
        x += 5;
    }
}

void renderer_draw_battlefield_top(void) {
    // 60 FPS Deduplicated Dirty Blocks (8x8): Erase previous frame entities
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].prev_top_active) {
            tiles_dirty_mark_rect(g_enemies[i].prev_top_x, g_enemies[i].prev_top_y,
                                  g_enemies[i].prev_top_w, g_enemies[i].prev_top_h, 0, 0);
            g_enemies[i].prev_top_active = 0;
        }
    }
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (g_death_particles[i].prev_top_active) {
            tiles_dirty_mark_rect(g_death_particles[i].prev_top_x - 1, g_death_particles[i].prev_top_y - 1, 4, 4, 0, 0);
            g_death_particles[i].prev_top_active = 0;
        }
    }
    tiles_dirty_restore(g_top_backbuffer, 0, 0);
}


void renderer_draw_wall(void) {
    // 1. Draw Active Turrets on Sockets (Wall Base and Ammo Depot are pre-baked in s_ground_bottom_cache at Y=144)
    int active_mask = 0;
    if (g_wall.active_turrets == 1) active_mask = (1 << 1);
    else if (g_wall.active_turrets == 2) active_mask = (1 << 1) | (1 << 2);
    else if (g_wall.active_turrets == 3) active_mask = (1 << 0) | (1 << 1) | (1 << 2);
    else active_mask = 0x0F;

    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        if (!(active_mask & (1 << s))) continue;
        int sx = c_wall_sockets[s].x;
        int sy = g_wall.screen_y + c_wall_sockets[s].y;
        int angle = g_wall.turret_angles[s];

        // Mechanical hydraulic recoil kickback
        static const struct { int8_t dx, dy; } s_recoil_offsets[5] = {
            {  1, 1 }, // Angle 0 (aiming far left)
            {  1, 2 }, // Angle 1 (aiming mid left)
            {  0, 2 }, // Angle 2 (aiming straight up)
            { -1, 2 }, // Angle 3 (aiming mid right)
            { -1, 1 }, // Angle 4 (aiming far right)
        };
        int r_frames = g_wall.turret_recoil[s];
        int r_dist = (r_frames >= 2) ? 2 : (r_frames == 1 ? 1 : 0);
        int r_ang = (angle >= 0 && angle < 5) ? angle : 2;
        int rx = s_recoil_offsets[r_ang].dx * r_dist;
        int ry = s_recoil_offsets[r_ang].dy * r_dist;

        int dest_x = sx - TURRET_PIVOT_X + rx;
        int dest_y = sy - TURRET_PIVOT_Y + ry;

        wall_draw_turret_sprite(g_backbuffer, dest_x, dest_y, angle);
        tiles_dirty_mark_rect(dest_x - 3, dest_y - 3, TURRET_SPRITE_W + 6, TURRET_SPRITE_H + 22, 1, s_bot_fb_idx);

        // Diegetic Ammo & Reload indicator beneath each turret cupola
        int bar_w = 16;
        int bar_x = sx - bar_w / 2;
        int bar_y = sy + 5;
        if (bar_y >= 0 && bar_y + 2 < SCREEN_H) {
            renderer_fill_rect(bar_x - 1, bar_y - 1, bar_w + 2, 3, COLOR_BLACK);
            if (g_wall.ammo[s] <= 0 || g_wall.is_reloading[s]) {
                // Out of ammo: blinking RELOAD alert banner
                static int s_blink_timer = 0;
                s_blink_timer++;
                uint16_t warn_col = ((s_blink_timer / 15) % 2 == 0) ? COLOR_LED_RED : COLOR_AMBER;
                renderer_fill_rect(bar_x, bar_y, bar_w, 1, warn_col);
                if ((s_blink_timer / 15) % 2 == 0) {
                    renderer_draw_text(sx - 14, bar_y - 12, "RELOAD", COLOR_LED_RED);
                }
            } else {
                // Ammo drum gauge: amber gold turning LED red when <= 2 rounds
                int max_a = (g_wall.max_ammo[s] > 0) ? g_wall.max_ammo[s] : 1;
                int ammo_fill = (g_wall.ammo[s] * bar_w) / max_a;
                if (ammo_fill < 1 && g_wall.ammo[s] > 0) ammo_fill = 1;
                uint16_t ammo_col = (g_wall.ammo[s] > 2) ? COLOR_AMBER : COLOR_LED_RED;
                if (ammo_fill > 0) {
                    renderer_fill_rect(bar_x, bar_y, ammo_fill, 1, ammo_col);
                }
            }
        }

        // High-energy Muzzle Flash
        if (g_wall.muzzle_flash_timer[s] > 0) {
            int alt = g_wall.muzzle_flash_barrel[s];
            const TurretCalibratedPoints *pts = &c_turret_points[angle];
            int mx = dest_x + (alt == 0 ? pts->ml_x : pts->mr_x);
            int my = dest_y + (alt == 0 ? pts->ml_y : pts->mr_y);

            if (mx >= 2 && mx < SCREEN_W - 3 && my >= 2 && my < SCREEN_H - 3) {
                // Incandescent plasma core
                g_backbuffer[my * SCREEN_W + mx] = COLOR_WHITE;
                g_backbuffer[my * SCREEN_W + mx + 1] = COLOR_WHITE;
                g_backbuffer[(my - 1) * SCREEN_W + mx] = COLOR_WHITE;
                g_backbuffer[(my - 1) * SCREEN_W + mx + 1] = COLOR_MUZZLE_FLASH;

                // Fiery corona flare
                g_backbuffer[(my - 2) * SCREEN_W + mx] = COLOR_MUZZLE_FLASH;
                g_backbuffer[(my + 1) * SCREEN_W + mx] = COLOR_MUZZLE_FLASH;
                g_backbuffer[(my + 1) * SCREEN_W + mx + 1] = RGB15(31, 16, 2) | BIT(15);
                g_backbuffer[my * SCREEN_W + (mx - 1)] = COLOR_MUZZLE_FLASH;
                g_backbuffer[my * SCREEN_W + (mx + 2)] = COLOR_MUZZLE_FLASH;
                g_backbuffer[(my - 1) * SCREEN_W + (mx - 1)] = RGB15(31, 16, 2) | BIT(15);
                g_backbuffer[(my - 1) * SCREEN_W + (mx + 2)] = RGB15(31, 16, 2) | BIT(15);

                tiles_dirty_mark_rect(mx - 2, my - 3, 6, 6, 1, s_bot_fb_idx);
            }
        }
    }

    // 4. Diegetic 32 Cathode Bulbs Wall Health Row along the bottom edge (Y=188, X=4..252)
    {
        int num_bulbs = 32;
        int max_hp = (g_wall.max_hp > 0) ? g_wall.max_hp : 1;
        int lit_bulbs = (int)((g_wall.hp * num_bulbs + max_hp - 1) / max_hp);
        if (g_wall.hp > 0 && lit_bulbs == 0) lit_bulbs = 1;
        if (lit_bulbs > num_bulbs) lit_bulbs = num_bulbs;

        int is_flashing = (g_wall.damage_flash_timer > 0);

        uint16_t col_hot, col_glow;
        if (is_flashing) {
            // Trauma flash: bright white-gold shockwave across active bulbs
            col_hot  = RGB15(31, 31, 28) | BIT(15);
            col_glow = RGB15(31, 22, 4)  | BIT(15);
        } else if (g_wall.hp * 4 > (uint64_t)max_hp) {
            // Healthy (>25%): Emerald Green Phosphor
            col_hot  = RGB15(24, 31, 24) | BIT(15);
            col_glow = RGB15(2, 31, 6)   | BIT(15);
        } else {
            // Critical (<=25%): Pulsing Crimson Alert
            static int s_pulse_timer = 0;
            s_pulse_timer++;
            if ((s_pulse_timer / 8) % 2 == 0) {
                col_hot  = RGB15(31, 24, 24) | BIT(15);
                col_glow = RGB15(31, 4, 4)   | BIT(15);
            } else {
                col_hot  = RGB15(24, 6, 6)   | BIT(15);
                col_glow = RGB15(16, 2, 2)   | BIT(15);
            }
        }
        // Distinct lifeless extinguished socket: dark gray ring, black hollow center
        uint16_t col_dead_rim   = RGB15(6, 6, 7) | BIT(15);
        uint16_t col_dead_core  = RGB15(1, 1, 2) | BIT(15);
        uint16_t col_bezel      = RGB15(3, 3, 4) | BIT(15);

        tiles_dirty_mark_rect(0, 186, SCREEN_W, 6, 1, s_bot_fb_idx);
        int cy = 188;
        for (int i = 0; i < num_bulbs; i++) {
            int cx = 4 + i * 8;
            int is_lit = (i < lit_bulbs);
            uint16_t c_core = is_lit ? col_hot : col_dead_core;
            uint16_t c_rim  = is_lit ? col_glow : col_dead_rim;

            // 4x4 circular bezel housing
            // Row 0 (cy - 1): . # # .
            g_backbuffer[(cy - 1) * SCREEN_W + cx - 1] = col_bezel;
            g_backbuffer[(cy - 1) * SCREEN_W + cx]     = c_rim;
            g_backbuffer[(cy - 1) * SCREEN_W + cx + 1] = c_rim;
            g_backbuffer[(cy - 1) * SCREEN_W + cx + 2] = col_bezel;

            // Row 1 (cy):     # O O #
            g_backbuffer[cy * SCREEN_W + cx - 1] = c_rim;
            g_backbuffer[cy * SCREEN_W + cx]     = c_core;
            g_backbuffer[cy * SCREEN_W + cx + 1] = c_core;
            g_backbuffer[cy * SCREEN_W + cx + 2] = c_rim;

            // Row 2 (cy + 1): # O O #
            g_backbuffer[(cy + 1) * SCREEN_W + cx - 1] = c_rim;
            g_backbuffer[(cy + 1) * SCREEN_W + cx]     = c_core;
            g_backbuffer[(cy + 1) * SCREEN_W + cx + 1] = c_core;
            g_backbuffer[(cy + 1) * SCREEN_W + cx + 2] = c_rim;

            // Row 3 (cy + 2): . # # .
            g_backbuffer[(cy + 2) * SCREEN_W + cx - 1] = col_bezel;
            g_backbuffer[(cy + 2) * SCREEN_W + cx]     = c_rim;
            g_backbuffer[(cy + 2) * SCREEN_W + cx + 1] = c_rim;
            g_backbuffer[(cy + 2) * SCREEN_W + cx + 2] = col_bezel;
        }
    }

    // 3. Draw Casings (Heavy Tumbling Brass Artillery Particles)
    for (int i = 0; i < MAX_CASINGS; i++) {
        if (!g_casings[i].active) continue;
        int cx = FROM_FP(g_casings[i].x);
        int cz = FROM_FP(g_casings[i].z);
        int floor_y = FROM_FP(g_casings[i].y);
        int cy = floor_y - cz;

        if (cx >= 2 && cx < SCREEN_W - 3 && cy >= 2 && cy < SCREEN_H - 3) {
            uint16_t c_hi   = (cz > 1) ? (RGB15(31, 29, 12) | BIT(15)) : (RGB15(30, 25, 8) | BIT(15)); // Specular gold
            uint16_t c_body = (cz > 1) ? (RGB15(27, 21, 5)  | BIT(15)) : (RGB15(22, 16, 4) | BIT(15)); // Brass mid
            uint16_t c_dark = (cz > 1) ? (RGB15(15, 10, 2)  | BIT(15)) : (RGB15(11, 7, 2)  | BIT(15)); // Base rim

            // Projected 3D drop shadow on the bunker deck while airborne
            if (cz > 1 && floor_y >= 0 && floor_y < SCREEN_H - 1) {
                g_backbuffer[floor_y * SCREEN_W + cx] = RGB15(1, 1, 2) | BIT(15);
                g_backbuffer[floor_y * SCREEN_W + cx + 1] = RGB15(1, 1, 2) | BIT(15);
                g_backbuffer[(floor_y + 1) * SCREEN_W + cx] = RGB15(1, 1, 2) | BIT(15);
            }

            int ang_idx = ((g_casings[i].angle % 360) / 45) % 4;
            if (ang_idx == 0) {
                // Horizontal 3x2:
                // [c_hi]   [c_hi]   [c_body]
                // [c_body] [c_dark] [c_dark]
                g_backbuffer[cy * SCREEN_W + cx - 1]     = c_hi;
                g_backbuffer[cy * SCREEN_W + cx]         = c_hi;
                g_backbuffer[cy * SCREEN_W + cx + 1]     = c_body;
                g_backbuffer[(cy + 1) * SCREEN_W + cx - 1] = c_body;
                g_backbuffer[(cy + 1) * SCREEN_W + cx]     = c_dark;
                g_backbuffer[(cy + 1) * SCREEN_W + cx + 1] = c_dark;
            } else if (ang_idx == 1) {
                // Diagonal 3x3:
                g_backbuffer[(cy - 1) * SCREEN_W + cx]     = c_hi;
                g_backbuffer[(cy - 1) * SCREEN_W + cx + 1] = c_hi;
                g_backbuffer[cy * SCREEN_W + cx - 1]       = c_hi;
                g_backbuffer[cy * SCREEN_W + cx]           = c_body;
                g_backbuffer[cy * SCREEN_W + cx + 1]       = c_dark;
                g_backbuffer[(cy + 1) * SCREEN_W + cx - 1] = c_body;
                g_backbuffer[(cy + 1) * SCREEN_W + cx]     = c_dark;
            } else if (ang_idx == 2) {
                // Vertical 2x3:
                // [c_hi]   [c_hi]
                // [c_body] [c_body]
                // [c_dark] [c_dark]
                g_backbuffer[(cy - 1) * SCREEN_W + cx]     = c_hi;
                g_backbuffer[(cy - 1) * SCREEN_W + cx + 1] = c_hi;
                g_backbuffer[cy * SCREEN_W + cx]           = c_body;
                g_backbuffer[cy * SCREEN_W + cx + 1]       = c_body;
                g_backbuffer[(cy + 1) * SCREEN_W + cx]     = c_dark;
                g_backbuffer[(cy + 1) * SCREEN_W + cx + 1] = c_dark;
            } else {
                // Diagonal 3x3 reverse:
                g_backbuffer[(cy - 1) * SCREEN_W + cx - 1] = c_hi;
                g_backbuffer[(cy - 1) * SCREEN_W + cx]     = c_hi;
                g_backbuffer[cy * SCREEN_W + cx - 1]       = c_dark;
                g_backbuffer[cy * SCREEN_W + cx]           = c_body;
                g_backbuffer[cy * SCREEN_W + cx + 1]       = c_hi;
                g_backbuffer[(cy + 1) * SCREEN_W + cx]     = c_dark;
                g_backbuffer[(cy + 1) * SCREEN_W + cx + 1] = c_body;
            }

            int dirty_y0 = (cz > 1 && cy > floor_y) ? floor_y - 1 : cy - 2;
            int dirty_y1 = (cz > 1 && floor_y > cy) ? floor_y + 2 : cy + 2;
            tiles_dirty_mark_rect(cx - 2, dirty_y0, 6, (dirty_y1 - dirty_y0) + 3, 1, s_bot_fb_idx);
            g_casings[i].prev_cx = cx;
            g_casings[i].prev_cy = cy;
            g_casings[i].prev_active = 1;
        }
    }

    // 4. Draw Bullet Darts (Hypersonic Tracer Slugs)
    for (int i = 0; i < MAX_BULLET_DARTS; i++) {
        if (!g_bullet_darts[i].active) continue;
        int bx = FROM_FP(g_bullet_darts[i].x);
        int by = FROM_FP(g_bullet_darts[i].y);

        if (bx >= 2 && bx < SCREEN_W - 2 && by >= 2 && by < SCREEN_H - 2) {
            g_backbuffer[by * SCREEN_W + bx] = COLOR_WHITE;
            int vx_sign = (g_bullet_darts[i].vx > 0) ? 1 : ((g_bullet_darts[i].vx < 0) ? -1 : 0);
            int vy_sign = (g_bullet_darts[i].vy > 0) ? 1 : -1;

            // Tracer Segment 1 (Brilliant bolter yellow)
            int t1x = bx - vx_sign;
            int t1y = by - vy_sign;
            if (t1x >= 0 && t1x < SCREEN_W && t1y >= 0 && t1y < SCREEN_H) {
                g_backbuffer[t1y * SCREEN_W + t1x] = COLOR_BOLTER_TRACER;
            }
            // Tracer Segment 2 (Burning propellant orange)
            int t2x = bx - 2 * vx_sign;
            int t2y = by - 2 * vy_sign;
            if (t2x >= 0 && t2x < SCREEN_W && t2y >= 0 && t2y < SCREEN_H) {
                g_backbuffer[t2y * SCREEN_W + t2x] = RGB15(31, 16, 2) | BIT(15);
            }
            // Lateral illumination glow
            if (by + 1 < SCREEN_H) {
                g_backbuffer[(by + 1) * SCREEN_W + bx] = COLOR_BOLTER_TRACER;
            }

            int min_dx = t2x < bx ? t2x : bx;
            int min_dy = t2y < by ? t2y : by;
            tiles_dirty_mark_rect(min_dx - 1, min_dy - 1, abs(bx - t2x) + 3, abs(by - t2y) + 3, 1, s_bot_fb_idx);
            g_bullet_darts[i].prev_bx = bx;
            g_bullet_darts[i].prev_by = by;
            g_bullet_darts[i].prev_active = 1;
        }
    }
}

void renderer_draw_battlefield_bottom(void) {
    // 60 FPS Deduplicated Dirty Blocks (8x8):
    // In VRAM double-buffering, restore the exact dirty blocks marked when this buffer was drawn 2 frames ago.
    tiles_dirty_restore(g_backbuffer, 1, s_bot_fb_idx);
}

void renderer_draw_turret(const Turret *t, int is_selected) {
    if (!t->placed) return;
    
    // Selected or target highlight
    if (is_selected) {
        renderer_draw_circle(t->x, t->y, 16, COLOR_AMBER, 0);
        renderer_draw_circle(t->x, t->y, t->range, COLOR_AMBER, 0);
    }

    // Draw canonical 16-angle animated RotSprite turret
    int angle_16 = ((t->current_angle + 8) >> 4) & 15;
    turret_draw_frame_angle(t->x, t->y, t->type, t->anim_frame, angle_16, is_selected);

    // Ammo bar above turret
    int bar_w = 20;
    int bar_x = t->x - bar_w / 2;
    int bar_y = t->y - 18;
    renderer_fill_rect(bar_x - 1, bar_y - 1, bar_w + 2, 4, COLOR_BLACK);
    int ammo_fill = (t->max_ammo > 0) ? (t->ammo * bar_w / t->max_ammo) : 0;
    uint16_t ammo_col = (t->ammo > t->max_ammo / 4) ? COLOR_AMBER : COLOR_LED_RED;
    if (ammo_fill > 0) {
        renderer_fill_rect(bar_x, bar_y, ammo_fill, 2, ammo_col);
    }

    // Health bar if damaged
    if (t->hp < t->max_hp) {
        int hp_y = t->y + 16;
        renderer_fill_rect(bar_x - 1, hp_y - 1, bar_w + 2, 4, COLOR_BLACK);
        int hp_fill = (t->max_hp > 0) ? (t->hp * bar_w / t->max_hp) : 0;
        if (hp_fill > 0) {
            renderer_fill_rect(bar_x, hp_y, hp_fill, 2, COLOR_LED_GREEN);
        }
    }
}

static int renderer_collect_sorted_enemies(int *out, int bottom_screen) {
    int heads[SCREEN_H];
    int next[MAX_ENEMIES];
    for (int y = 0; y < SCREEN_H; y++) heads[y] = -1;
    int min_bucket = SCREEN_H;
    int max_bucket = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;
        int local_y = FROM_FP(g_enemies[i].y) - (bottom_screen ? 192 : 0);
        if (local_y < -32 || local_y >= SCREEN_H) continue;
        int bucket = (local_y < 0) ? 0 : local_y;
        next[i] = heads[bucket];
        heads[bucket] = i;
        if (bucket < min_bucket) min_bucket = bucket;
        if (bucket > max_bucket) max_bucket = bucket;
    }
    if (max_bucket < min_bucket) return 0;
    int count = 0;
    for (int y = min_bucket; y <= max_bucket; y++) {
        for (int i = heads[y]; i >= 0; i = next[i]) out[count++] = i;
    }
    return count;
}

void renderer_draw_enemies_top(void) {
    int visible_indices[MAX_ENEMIES];
    int count = renderer_collect_sorted_enemies(visible_indices, 0);

    // Render sorted back-to-front (smaller Y first, larger Y drawn on top)
    for (int idx = 0; idx < count; idx++) {
        int i = visible_indices[idx];
        if (g_enemies[i].dying) continue; // liquefaction renders on the lower screen
        int gx = FROM_FP(g_enemies[i].x);
        int gy = FROM_FP(g_enemies[i].y);
        enemy_draw_sprite_to_buffer8(g_top_backbuffer, gx, gy, g_enemies[i].variant,
                                    g_enemies[i].anim_frame, g_enemies[i].dir,
                                    (g_enemies[i].biting_target == 99),
                                    &g_enemies[i].prev_top_x, &g_enemies[i].prev_top_y,
                                    &g_enemies[i].prev_top_w, &g_enemies[i].prev_top_h);
        g_enemies[i].prev_top_active = 1;
        // Health bar if damaged
        if (g_enemies[i].hp < g_enemies[i].max_hp) {
            int bw = 14;
            int bx = gx - bw / 2;
            int by = gy - 10;
            top_fill_rect(bx - 1, by - 1, bw + 2, 3, TOP_COLOR_BLACK);
            int fill = (g_enemies[i].max_hp > 0) ? (int)((g_enemies[i].hp * bw) / g_enemies[i].max_hp) : 0;
            if (fill > 0) {
                top_fill_rect(bx, by, fill, 1, TOP_COLOR_LED_RED);
            }
            if (bx - 1 < g_enemies[i].prev_top_x) g_enemies[i].prev_top_x = bx - 1;
            if (by - 1 < g_enemies[i].prev_top_y) g_enemies[i].prev_top_y = by - 1;
        }
    }
}

void renderer_draw_enemies_bottom(void) {
    int visible_indices[MAX_ENEMIES];
    int count = renderer_collect_sorted_enemies(visible_indices, 1);

    // Render sorted back-to-front
    for (int idx = 0; idx < count; idx++) {
        int i = visible_indices[idx];
        int gx = FROM_FP(g_enemies[i].x);
        int gy = FROM_FP(g_enemies[i].y);
        int ly = gy - 192;
        // The wall draw happens after enemies. If the complete sprite is
        // below the wall edge, it cannot contribute visible pixels.
        if (ly - 16 >= g_wall.screen_y) {
            g_enemies[i].prev_bot_active = 0;
            continue;
        }
        int ex, ey, ew, eh;
        if (g_enemies[i].dying) {
            int prog = (g_enemies[i].death_timer * 255) / ENEMY_DEATH_FRAMES;
            enemy_draw_melting_sprite(g_backbuffer, gx, ly, g_enemies[i].variant,
                                      g_enemies[i].anim_frame, g_enemies[i].dir, prog,
                                      g_enemies[i].death_dir_x, g_enemies[i].death_dir_y,
                                      &ex, &ey, &ew, &eh);
        } else {
            enemy_draw_sprite_to_buffer(g_backbuffer, gx, ly, g_enemies[i].variant,
                                       g_enemies[i].anim_frame, g_enemies[i].dir,
                                       (g_enemies[i].biting_target == 99),
                                       &ex, &ey, &ew, &eh);
        }
        tiles_dirty_mark_rect(ex, ey, ew, eh, 1, s_bot_fb_idx);
        // Health bar if damaged
        if (!g_enemies[i].dying && g_enemies[i].hp < g_enemies[i].max_hp) {
            int bw = 14;
            int bx = gx - bw / 2;
            int by = ly - 10;
            renderer_fill_rect(bx - 1, by - 1, bw + 2, 3, COLOR_BLACK);
            int fill = (g_enemies[i].max_hp > 0) ? (int)((g_enemies[i].hp * bw) / g_enemies[i].max_hp) : 0;
            if (fill > 0) {
                renderer_fill_rect(bx, by, fill, 1, COLOR_LED_RED);
            }
            tiles_dirty_mark_rect(bx - 1, by - 1, bw + 2, 3, 1, s_bot_fb_idx);
        }
    }
}

void renderer_draw_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) continue;
        int bx = FROM_FP(g_bullets[i].x);
        int by = FROM_FP(g_bullets[i].y);
        renderer_draw_pixel(bx, by, COLOR_BOLTER_TRACER);
        renderer_draw_pixel(bx + 1, by, COLOR_WHITE);
        renderer_draw_pixel(bx, by + 1, COLOR_WHITE);
        tiles_dirty_mark_rect(bx, by, 2, 2, 1, s_bot_fb_idx);

        g_bullets[i].prev_x = bx;
        g_bullets[i].prev_y = by;
        g_bullets[i].prev_active = 1;
    }
}

void renderer_draw_splatters_top(void) {
    // Persistent blood is already stored in the ground cache.
}

void renderer_draw_splatters_bottom(void) {
    // Persistent blood is already stored in the ground cache.
}

void renderer_draw_death_particles_top(void) {
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;
        int gx = FROM_FP(g_death_particles[i].x);
        int gy = FROM_FP(g_death_particles[i].y);
        int gz = FROM_FP(g_death_particles[i].z);
        int draw_y = gy - gz;
        if (draw_y >= 0 && draw_y < SCREEN_H && gx >= 0 && gx < SCREEN_W) {
            int sz = g_death_particles[i].size;
            uint8_t c8 = (g_death_particles[i].color == COLOR_XENOS_CHITIN) ? TOP_COLOR_IRON_LIGHT :
                         ((g_death_particles[i].color == COLOR_XENOS_GORE_CORE) ? TOP_COLOR_XENOS_GORE_CORE : TOP_COLOR_XENOS_GORE_MID);
            if (sz == 3) {
                // Flash pop
                top_draw_pixel(gx, draw_y, TOP_COLOR_WHITE);
                if (gx > 0) top_draw_pixel(gx - 1, draw_y, TOP_COLOR_WHITE);
                if (gx + 1 < SCREEN_W) top_draw_pixel(gx + 1, draw_y, TOP_COLOR_WHITE);
                if (draw_y > 0) top_draw_pixel(gx, draw_y - 1, TOP_COLOR_WHITE);
                if (draw_y + 1 < SCREEN_H) top_draw_pixel(gx, draw_y + 1, TOP_COLOR_WHITE);
            } else if (sz == 2) {
                // 2x2 chunk
                top_draw_pixel(gx, draw_y, c8);
                if (gx + 1 < SCREEN_W) top_draw_pixel(gx + 1, draw_y, c8);
                if (draw_y + 1 < SCREEN_H) top_draw_pixel(gx, draw_y + 1, c8);
                if (gx + 1 < SCREEN_W && draw_y + 1 < SCREEN_H) top_draw_pixel(gx + 1, draw_y + 1, c8);
            } else if (sz == 1) {
                // 2x1 fragment
                top_draw_pixel(gx, draw_y, c8);
                if (gx + 1 < SCREEN_W) top_draw_pixel(gx + 1, draw_y, c8);
            } else {
                top_draw_pixel(gx, draw_y, c8);
            }
            g_death_particles[i].prev_top_x = gx;
            g_death_particles[i].prev_top_y = draw_y;
            g_death_particles[i].prev_top_active = 1;
        }
    }
}

void renderer_draw_death_particles_bottom(void) {
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;
        int gx = FROM_FP(g_death_particles[i].x);
        int gy = FROM_FP(g_death_particles[i].y) - 192;
        int gz = FROM_FP(g_death_particles[i].z);
        int draw_y = gy - gz;
        if (draw_y >= 0 && draw_y < SCREEN_H && gx >= 0 && gx < SCREEN_W) {
            uint16_t c = g_death_particles[i].color;
            int sz = g_death_particles[i].size;
            g_death_particles[i].prev_bot_has_shadow = 0;

            if (sz == 3) {
                // Flash / kinetic burst (3x3 cross)
                renderer_draw_pixel(gx, draw_y, c);
                if (gx > 0) renderer_draw_pixel(gx - 1, draw_y, c);
                if (gx + 1 < SCREEN_W) renderer_draw_pixel(gx + 1, draw_y, c);
                if (draw_y > 0) renderer_draw_pixel(gx, draw_y - 1, c);
                if (draw_y + 1 < SCREEN_H) renderer_draw_pixel(gx, draw_y + 1, c);
                tiles_dirty_mark_rect(gx - 2, draw_y - 2, 5, 5, 1, s_bot_fb_idx);
            } else if (sz == 2) {
                // Heavy 2x2 chunk of chitin/flesh
                renderer_draw_pixel(gx, draw_y, c);
                if (gx + 1 < SCREEN_W) renderer_draw_pixel(gx + 1, draw_y, c);
                if (draw_y + 1 < SCREEN_H) renderer_draw_pixel(gx, draw_y + 1, c);
                if (gx + 1 < SCREEN_W && draw_y + 1 < SCREEN_H) renderer_draw_pixel(gx + 1, draw_y + 1, c);
                tiles_dirty_mark_rect(gx - 1, draw_y - 1, 4, 4, 1, s_bot_fb_idx);

                // Ground drop shadow (converges when landing, separates in air!)
                if (gy >= 0 && gy < SCREEN_H && gz > 1) {
                    uint16_t shd = RGB15(1, 1, 2) | BIT(15);
                    renderer_draw_pixel(gx, gy, shd);
                    if (gx + 1 < SCREEN_W) renderer_draw_pixel(gx + 1, gy, shd);
                    tiles_dirty_mark_rect(gx - 1, gy - 1, 4, 3, 1, s_bot_fb_idx);
                    g_death_particles[i].prev_bot_has_shadow = 1;
                    g_death_particles[i].prev_bot_sy = gy;
                }
            } else if (sz == 1) {
                // Medium 2x1 fragment
                renderer_draw_pixel(gx, draw_y, c);
                if (gx + 1 < SCREEN_W) renderer_draw_pixel(gx + 1, draw_y, c);
                tiles_dirty_mark_rect(gx - 1, draw_y - 1, 4, 3, 1, s_bot_fb_idx);

                if (gy >= 0 && gy < SCREEN_H && gz > 2) {
                    renderer_draw_pixel(gx, gy, RGB15(2, 2, 3) | BIT(15));
                    tiles_dirty_mark_rect(gx, gy, 2, 2, 1, s_bot_fb_idx);
                    g_death_particles[i].prev_bot_has_shadow = 1;
                    g_death_particles[i].prev_bot_sy = gy;
                }
            } else {
                // 1px blood droplet
                renderer_draw_pixel(gx, draw_y, c);
                tiles_dirty_mark_rect(gx - 1, draw_y - 1, 3, 3, 1, s_bot_fb_idx);
            }

            g_death_particles[i].prev_bot_x = gx;
            g_death_particles[i].prev_bot_y = draw_y;
            g_death_particles[i].prev_bot_active = 1;
        }
    }
}


void renderer_draw_gore_chunks_bottom(void) {
    for (int i = 0; i < MAX_GORE_CHUNKS; i++) {
        if (!g_gore_chunks[i].active) continue;
        int cx = FROM_FP(g_gore_chunks[i].x);
        int cy = FROM_FP(g_gore_chunks[i].y) - 192;
        int cz = FROM_FP(g_gore_chunks[i].z);
        int w = g_gore_chunks[i].w;
        int h = g_gore_chunks[i].h;
        int draw_y = cy - cz;

        // Ground shadow while the chunk is airborne (converges as it lands)
        if (cz > 1 && cy >= 0 && cy < SCREEN_H) {
            uint16_t shd = RGB15(1, 1, 2) | BIT(15);
            for (int xx = 0; xx < w; xx++) {
                int px = cx + xx;
                if (px < 0 || px >= SCREEN_W) continue;
                renderer_draw_pixel(px, cy, shd);
            }
            tiles_dirty_mark_rect(cx - 1, cy - 1, w + 2, 3, 1, s_bot_fb_idx);
        }

        // The real sprite pixels
        for (int yy = 0; yy < h; yy++) {
            int py = draw_y + yy;
            if (py < 0 || py >= SCREEN_H) continue;
            for (int xx = 0; xx < w; xx++) {
                int px = cx + xx;
                if (px < 0 || px >= SCREEN_W) continue;
                uint8_t v = g_gore_chunks[i].idx[yy * w + xx];
                if (!v) continue;
                g_backbuffer[py * SCREEN_W + px] = g_enemy_palette[v];
            }
        }
        tiles_dirty_mark_rect(cx - 2, draw_y - 2, w + 4, h + 4, 1, s_bot_fb_idx);
    }
}

void renderer_draw_ui_wave(void) {
    // Top HUD banner
    top_fill_rect(0, 0, SCREEN_W, 28, TOP_COLOR_BLACK);
    char buf[64];
    snprintf(buf, sizeof(buf), "ETAPA %d/5", g_game.wave_number);
    top_draw_text(6, 2, buf, TOP_COLOR_AMBER);

    int sec_left = g_game.wave_timer / 60;
    int m = sec_left / 60;
    int s = sec_left % 60;
    snprintf(buf, sizeof(buf), "TIME: %d:%02d", m, s);
    top_draw_text(74, 2, buf, TOP_COLOR_WHITE);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    top_draw_text(165, 2, buf, TOP_COLOR_PHOSPHOR_GREEN);

    // Profiler overlay (ALWAYS visible on row 2)
    snprintf(buf, sizeof(buf), "FPS:%2d T:%d B:%d P:%d S:%d E:%d",
             g_game.prof_fps, g_game.prof_top_ticks, g_game.prof_bot_ticks,
             g_game.prof_pres_ticks, g_game.prof_sim_ticks, g_game.prof_enemies_active);
    top_draw_text(6, 10, buf, TOP_COLOR_WHITE);

    // Peak alert / telegraphing on row 3 (does NOT cover profiler stats)
    if (g_game.wave_timer <= 2400 && g_game.wave_timer > 1800) {
        int peak_sec = (g_game.wave_timer - 1800) / 60;
        snprintf(buf, sizeof(buf), "! ALERTA PICO EN %ds !", peak_sec + 1);
        uint8_t col = (g_game.sim_ticks_elapsed & 8) ? TOP_COLOR_AMBER : TOP_COLOR_WHITE;
        top_draw_text(68, 19, buf, col);
    } else if (g_game.wave_timer <= 1800 && g_game.wave_timer > 0) {
        uint8_t col = (g_game.sim_ticks_elapsed & 12) ? TOP_COLOR_LED_RED : TOP_COLOR_AMBER;
        top_draw_text(58, 19, "!! PICO DE ETAPA ACTIVO !!", col);
    } else if (g_game.wave_timer <= 0) {
        snprintf(buf, sizeof(buf), "! LIMPIAR REMANENTES: %d !", g_game.enemies_alive);
        top_draw_text(52, 19, buf, TOP_COLOR_PHOSPHOR_GREEN);
    }

    // If currently dragging ammo crate
    if (g_game.is_dragging_ammo) {
        int x0 = g_game.drag_x - AMMO_CRATE_W / 2;
        int y0 = g_game.drag_y - AMMO_CRATE_H / 2;
        for (int dy = 0; dy < AMMO_CRATE_H; dy++) {
            int py = y0 + dy;
            if (py < 0 || py >= SCREEN_H) continue;
            for (int dx = 0; dx < AMMO_CRATE_W; dx++) {
                int px = x0 + dx;
                if (px < 0 || px >= SCREEN_W) continue;
                uint16_t c = c_ammo_crate_sprite[dy * AMMO_CRATE_W + dx];
                if (c & BIT(15)) {
                    g_backbuffer[py * SCREEN_W + px] = c;
                }
            }
        }
        tiles_dirty_mark_rect(x0, y0, AMMO_CRATE_W, AMMO_CRATE_H, 1, s_bot_fb_idx);
        g_game.prev_drag_x = g_game.drag_x;
        g_game.prev_drag_y = g_game.drag_y;
        g_game.prev_drag_active = 1;
    }
}

void renderer_draw_ui_prep(void) {
    renderer_draw_ui_pause();
}

void renderer_draw_ui_pause(void) {
    // Top HUD banner
    top_fill_rect(0, 0, SCREEN_W, 28, TOP_COLOR_BLACK);
    char buf[64];
    snprintf(buf, sizeof(buf), "ETAPA %d/5", g_game.wave_number);
    top_draw_text(6, 4, buf, TOP_COLOR_AMBER);
    if (g_game.stage_completed_flag) {
        top_draw_text(74, 4, "! ETAPA SUPERADA !", TOP_COLOR_PHOSPHOR_GREEN);
    } else {
        top_draw_text(90, 4, "[PAUSA]", TOP_COLOR_WHITE);
    }

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    top_draw_text(160, 4, buf, TOP_COLOR_PHOSPHOR_GREEN);

    snprintf(buf, sizeof(buf), "FPS:%2d T:%d B:%d P:%d S:%d E:%d",
             g_game.prof_fps, g_game.prof_top_ticks, g_game.prof_bot_ticks,
             g_game.prof_pres_ticks, g_game.prof_sim_ticks, g_game.prof_enemies_active);
    top_draw_text(6, 16, buf, TOP_COLOR_WHITE);

    int is_fresh = (g_game.wave_timer >= STAGE_DURATION_FRAMES || g_game.enemies_spawned == 0);

    // Bunker status and repair banner (x: 10..246, y: 114..138)
    if (g_wall.hp < g_wall.max_hp) {
        renderer_fill_rect(10, 114, 236, 24, COLOR_IRON_PANEL);
        uint16_t rep_border = (g_game.scrap >= 30) ? COLOR_PHOSPHOR_GREEN : COLOR_AMBER;
        renderer_draw_rect(10, 114, 236, 24, rep_border);

        char hp_buf[32];
        snprintf(hp_buf, sizeof(hp_buf), "BUNKER: %d/%d HP", (int)g_wall.hp, (int)g_wall.max_hp);
        uint16_t hp_col = (g_wall.hp <= 35) ? COLOR_LED_RED : COLOR_AMBER;
        renderer_draw_text(16, 122, hp_buf, hp_col);

        uint16_t rep_txt_col = (g_game.scrap >= 30) ? COLOR_PHOSPHOR_GREEN : RGB15(15, 15, 17) | BIT(15);
        renderer_draw_text(126, 122, "[REPARAR +25: 30$]", rep_txt_col);
    } else {
        renderer_draw_text(20, 124, "BUNKER: 100/100 HP  [INTEGRIDAD OPTIMA]", COLOR_PHOSPHOR_GREEN);
    }

    // 1. [TIENDA] Button (x: 10..84, y: 146..182)
    renderer_fill_rect(10, 146, 74, 36, COLOR_IRON_PANEL);
    renderer_draw_rect(10, 146, 74, 36, COLOR_AMBER);
    renderer_draw_text(26, 154, "TIENDA", COLOR_WHITE);
    renderer_draw_text(18, 168, "+MEJORAS", COLOR_AMBER);

    // 2. [CALIBRAR] Button (x: 90..164, y: 146..182)
    renderer_fill_rect(90, 146, 74, 36, COLOR_IRON_PANEL);
    renderer_draw_rect(90, 146, 74, 36, COLOR_AMBER);
    renderer_draw_text(98, 154, "CALIBRAR", COLOR_WHITE);
    renderer_draw_text(102, 168, "(STATS)", COLOR_PHOSPHOR_GREEN);

    // 3. [JUGAR / REANUDAR] Button (x: 170..246, y: 146..182)
    renderer_fill_rect(170, 146, 76, 36, COLOR_LED_GREEN);
    renderer_draw_rect(170, 146, 76, 36, COLOR_WHITE);
    if (g_game.stage_completed_flag) {
        renderer_draw_text(176, 154, "SIGUIENTE", COLOR_BLACK);
        renderer_draw_text(186, 168, "ETAPA", COLOR_BLACK);
    } else if (is_fresh) {
        renderer_draw_text(186, 154, "INICIAR", COLOR_BLACK);
        renderer_draw_text(184, 168, "COMBATE", COLOR_BLACK);
    } else {
        renderer_draw_text(180, 154, "REANUDAR", COLOR_BLACK);
        renderer_draw_text(186, 168, "[START]", COLOR_BLACK);
    }
}

void renderer_draw_ui_game_over(void) {
    renderer_fill_rect(36, 36, 184, 120, COLOR_BLACK);
    renderer_draw_rect(36, 36, 184, 120, COLOR_LED_RED);
    renderer_draw_rect(38, 38, 180, 116, COLOR_AMBER);
    renderer_draw_text(76, 48, "SANCTUM CAIDO", COLOR_LED_RED);
    
    char buf[64];
    snprintf(buf, sizeof(buf), "ETAPA ALCANZADA: %d/5", g_game.wave_number);
    renderer_draw_text(48, 70, buf, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "BAJAS CONFIRMADAS: %llu", (unsigned long long)g_game.enemies_killed);
    renderer_draw_text(48, 86, buf, COLOR_WHITE);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "TOTAL SCRAP: %s", scrap_buf);
    renderer_draw_text(48, 102, buf, COLOR_AMBER);

    renderer_draw_text(44, 128, "TOCA O PULSA B PARA REINTENTAR", COLOR_PHOSPHOR_GREEN);
}

void renderer_draw_ui_victory(void) {
    renderer_fill_rect(24, 28, 208, 136, COLOR_BLACK);
    renderer_draw_rect(24, 28, 208, 136, COLOR_PHOSPHOR_GREEN);
    renderer_draw_rect(26, 30, 204, 132, COLOR_AMBER);

    renderer_draw_text(48, 40, "SECTOR 1 ASEGURADO!", COLOR_PHOSPHOR_GREEN);
    renderer_draw_text(72, 56, "VICTORIA TOTAL", COLOR_AMBER);

    char buf[64];
    snprintf(buf, sizeof(buf), "ETAPAS SUPERADAS: 5/5");
    renderer_draw_text(40, 78, buf, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "ENEMIGOS ELIMINADOS: %llu", (unsigned long long)g_game.enemies_killed);
    renderer_draw_text(40, 94, buf, COLOR_WHITE);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "CHATARRA RECOLECTADA: %s", scrap_buf);
    renderer_draw_text(40, 110, buf, COLOR_AMBER);

    renderer_draw_text(35, 136, "TOCA O PULSA B PARA NUEVO CICLO", COLOR_PHOSPHOR_GREEN);
}

void renderer_draw_ui_upgrades(void) {
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    renderer_draw_rect(2, 2, SCREEN_W - 4, SCREEN_H - 4, COLOR_IRON_BORDER);
    renderer_draw_text(65, 8, "SANCTUM UPGRADES", COLOR_AMBER);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    char buf[64];
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    renderer_draw_text(160, 8, buf, COLOR_PHOSPHOR_GREEN);

    static const struct {
        int x, y, w, h;
        const char *title;
    } s_card_pos[7] = {
        { 10, 24, 110, 32, "CALIBER" },
        { 130, 24, 110, 32, "FIRE RATE" },
        { 10, 62, 110, 32, "MAG SIZE" },
        { 130, 62, 110, 32, "BIO HARVEST" },
        { 10, 100, 110, 32, "AUTO SUPPLY" },
        { 130, 100, 110, 32, "AUTO TARGET" },
        { 10, 138, 110, 32, "EXTRA TURRETS" }
    };

    for (int i = 0; i < 7; i++) {
        int x = s_card_pos[i].x;
        int y = s_card_pos[i].y;
        int w = s_card_pos[i].w;
        int h = s_card_pos[i].h;

        uint64_t cost = upgrade_get_cost(i);
        int can_buy = upgrade_can_afford(i);
        int is_flashing = (g_game.upgrade_flash_timer > 0 && g_game.upgrade_flash_idx == i);

        uint16_t border_col = is_flashing ? COLOR_WHITE : (can_buy ? COLOR_PHOSPHOR_GREEN : COLOR_IRON_BORDER);
        uint16_t bg_col = is_flashing ? COLOR_IRON_LIGHT : COLOR_IRON_PANEL;
        uint16_t text_col = can_buy ? COLOR_WHITE : RGB15(15, 15, 17) | BIT(15);
        uint16_t cost_col = can_buy ? COLOR_AMBER : RGB15(18, 14, 8) | BIT(15);

        renderer_fill_rect(x, y, w, h, bg_col);
        renderer_draw_rect(x, y, w, h, border_col);

        char cost_str[32];
        format_number_compact(cost_str, sizeof(cost_str), cost);

        if (i == 0) {
            snprintf(buf, sizeof(buf), "CALIBER LV%d", g_game.upgrades.caliber_lvl);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            snprintf(buf, sizeof(buf), "+DMG %s$", cost_str);
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 1) {
            snprintf(buf, sizeof(buf), "FIRE RATE LV%d", g_game.upgrades.firerate_lvl);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            snprintf(buf, sizeof(buf), "+ROF %s$", cost_str);
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 2) {
            snprintf(buf, sizeof(buf), "MAG SIZE LV%d", g_game.upgrades.mag_size_lvl);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            snprintf(buf, sizeof(buf), "+AMMO %s$", cost_str);
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 3) {
            snprintf(buf, sizeof(buf), "HARVEST LV%d", g_game.upgrades.bio_harvest_lvl);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            snprintf(buf, sizeof(buf), "+SCRAP %s$", cost_str);
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 4) {
            snprintf(buf, sizeof(buf), "SUPPLY LV%d", g_game.upgrades.conveyor_lvl);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            snprintf(buf, sizeof(buf), "FEED %s$", cost_str);
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 5) {
            int auto_lvl = 0;
            if (g_game.upgrades.continuous_fire) auto_lvl = 1;
            if (g_game.upgrades.auto_target) auto_lvl = 2;

            if (auto_lvl == 0) {
                snprintf(buf, sizeof(buf), "FIRE: MANUAL");
                renderer_draw_text(x + 4, y + 4, buf, text_col);
                snprintf(buf, sizeof(buf), "+HOLD %s$", cost_str);
                renderer_draw_text(x + 4, y + 16, buf, cost_col);
            } else if (auto_lvl == 1) {
                snprintf(buf, sizeof(buf), "FIRE: HOLD");
                renderer_draw_text(x + 4, y + 4, buf, text_col);
                snprintf(buf, sizeof(buf), "+AIM  %s$", cost_str);
                renderer_draw_text(x + 4, y + 16, buf, cost_col);
            } else {
                snprintf(buf, sizeof(buf), "AUTO TARGET");
                renderer_draw_text(x + 4, y + 4, buf, text_col);
                snprintf(buf, sizeof(buf), "MAXED");
                renderer_draw_text(x + 4, y + 16, buf, cost_col);
            }
        } else if (i == 6) {
            int ext = g_game.upgrades.extra_turrets;
            snprintf(buf, sizeof(buf), "SOCKET %d", ext + 2);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            if (ext == 0 && g_game.upgrades.firerate_lvl < 2) {
                renderer_draw_text(x + 4, y + 16, "REQ ROF 2", COLOR_AMBER);
            } else if (ext == 1 && g_game.upgrades.firerate_lvl < 4) {
                renderer_draw_text(x + 4, y + 16, "REQ ROF 4", COLOR_AMBER);
            } else if (ext >= 2) {
                renderer_draw_text(x + 4, y + 16, "MAXED", cost_col);
            } else {
                snprintf(buf, sizeof(buf), "+1  %s$", cost_str);
                renderer_draw_text(x + 4, y + 16, buf, cost_col);
            }
        }
    }

    if (g_game.upgrade_flash_timer > 0) g_game.upgrade_flash_timer--;

    // Return button
    renderer_fill_rect(130, 174, 76, 16, COLOR_LED_GREEN);
    renderer_draw_rect(130, 174, 76, 16, COLOR_WHITE);
    renderer_draw_text(153, 178, "BACK", COLOR_BLACK);
}

void renderer_draw_ui_calibration(void) {
    // Background plate
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    renderer_draw_rect(2, 2, SCREEN_W - 4, SCREEN_H - 4, COLOR_IRON_BORDER);

    // Title banner
    renderer_draw_text(6, 4, "CALIBRATION", COLOR_AMBER);
    renderer_draw_text(145, 4, "X <TAB   TAB> Y", COLOR_WHITE);
    if (g_game.calib_saved_timer > 0) {
        g_game.calib_saved_timer--;
        renderer_draw_text(180, 4, "SAVED (SD)", COLOR_PHOSPHOR_GREEN);
    }

    if (g_game.calib_page != 0) {
        char buf[64];
        static const char *page_titles[4] = { "ETAPAS (1..5)", "ENEMY STATS", "BASE / STATS MEJORAS", "COSTES TIENDA" };
        const char *title = page_titles[g_game.calib_page];
        renderer_draw_text(6, 18, title, COLOR_WHITE);
        snprintf(buf, sizeof(buf), "PAGE %d/4", g_game.calib_page + 1);
        renderer_draw_text(190, 18, buf, COLOR_AMBER);
        int first = (g_game.calib_row / 10) * 10;
        int last = (g_game.calib_page == 1) ? 40 : ((g_game.calib_page == 2) ? 21 : 22);
        static const char *enemy_labels[40] = {
            "SCOURGE HP", "SCOURGE SPD", "SCOURGE SCRAP", "SCOURGE DMG", "SCOURGE FRM",
            "ZERGLING HP", "ZERGLING SPD", "ZERGLING SCRAP", "ZERGLING DMG", "ZERGLING FRM",
            "HYDRA HP", "HYDRA SPD", "HYDRA SCRAP", "HYDRA DMG", "HYDRA FRM",
            "MUTA HP", "MUTA SPD", "MUTA SCRAP", "MUTA DMG", "MUTA FRM",
            "DEFILER HP", "DEFILER SPD", "DEFILER SCRAP", "DEFILER DMG", "DEFILER FRM",
            "LURKER HP", "LURKER SPD", "LURKER SCRAP", "LURKER DMG", "LURKER FRM",
            "GUARDIAN HP", "GUARDIAN SPD", "GUARDIAN SCRAP", "GUARDIAN DMG", "GUARDIAN FRM",
            "ULTRA HP", "ULTRA SPD", "ULTRA SCRAP", "ULTRA DMG", "ULTRA FRM"
        };
        static const char *base_labels[21] = {
            "BUNKER START HP",
            "DAMAGE LV0", "DAMAGE LV1", "DAMAGE LV2", "DAMAGE LV3", "DAMAGE LV4",
            "CADENCE LV0 (FRM)", "CADENCE LV1 (FRM)", "CADENCE LV2 (FRM)", "CADENCE LV3 (FRM)", "CADENCE LV4 (FRM)",
            "CONVEYOR LV0 (FRM)", "CONVEYOR LV1 (FRM)", "CONVEYOR LV2 (FRM)", "CONVEYOR LV3 (FRM)", "CONVEYOR LV4 (FRM)",
            "MAGAZINE LV0", "MAGAZINE LV1", "MAGAZINE LV2", "MAGAZINE LV3", "MAGAZINE LV4"
        };
        static const char *cost_labels[22] = {
            "CALIBER LV1 COST", "CALIBER LV2 COST", "CALIBER LV3 COST", "CALIBER LV4 COST",
            "CADENCE LV1 COST", "CADENCE LV2 COST", "CADENCE LV3 COST", "CADENCE LV4 COST",
            "MAGAZINE LV1 COST", "MAGAZINE LV2 COST", "MAGAZINE LV3 COST", "MAGAZINE LV4 COST",
            "BIO HARVEST LV1", "BIO HARVEST LV2",
            "AUTO SUPPLY LV1", "AUTO SUPPLY LV2", "AUTO SUPPLY LV3", "AUTO SUPPLY LV4",
            "HOLD FIRE COST", "AUTO TARGET COST",
            "SOCKET 2 (ROF>=2)", "SOCKET 3 (ROF>=4)"
        };
        for (int n = 0; n < 10 && first + n < last; n++) {
            int r = first + n, val = 0;
            if (g_game.calib_page == 1) {
                int e = r / 5, f = r % 5;
                if (e >= 8) e = 7;
                val = (f == 0) ? g_balance.enemy_hp[e] : (f == 1) ? g_balance.enemy_speed[e] : (f == 2) ? g_balance.enemy_scrap[e] : (f == 3) ? g_balance.enemy_bite_damage[e] : g_balance.enemy_bite_interval[e];
            } else if (g_game.calib_page == 2) {
                if (r == 0) val = g_balance.bunker_start_hp;
                else if (r <= 5) val = g_balance.turret_damage[r - 1];
                else if (r <= 10) val = g_balance.turret_fire_interval[r - 6];
                else if (r <= 15) val = g_balance.conveyor_reload_interval[r - 11];
                else val = g_balance.turret_magazine[r - 16];
            } else {
                if (r >= 0 && r <= 3) val = (int)g_balance.upgrade_costs[0][r];
                else if (r >= 4 && r <= 7) val = (int)g_balance.upgrade_costs[1][r - 4];
                else if (r >= 8 && r <= 11) val = (int)g_balance.upgrade_costs[2][r - 8];
                else if (r >= 12 && r <= 13) val = (int)g_balance.upgrade_costs[3][r - 12];
                else if (r >= 14 && r <= 17) val = (int)g_balance.upgrade_costs[4][r - 14];
                else if (r == 18) val = (int)g_balance.upgrade_costs[5][0];
                else if (r == 19) val = (int)g_balance.upgrade_costs[5][1];
                else if (r >= 20 && r <= 21) val = (int)g_balance.upgrade_costs[6][r - 20];
            }
            int y = 32 + n * 13; int sel = (r == g_game.calib_row);
            renderer_fill_rect(6, y, 244, 12, sel ? COLOR_IRON_LIGHT : COLOR_IRON_PANEL);
            renderer_draw_rect(6, y, 244, 12, sel ? COLOR_AMBER : COLOR_IRON_BORDER);
            if (g_game.calib_page == 1) renderer_draw_text(10, y + 2, enemy_labels[r], COLOR_WHITE);
            else if (g_game.calib_page == 2) renderer_draw_text(10, y + 2, base_labels[r], COLOR_WHITE);
            else renderer_draw_text(10, y + 2, cost_labels[r], COLOR_WHITE);
            snprintf(buf, sizeof(buf), "%d  [-] [+]", val); renderer_draw_text(150, y + 2, buf, COLOR_PHOSPHOR_GREEN);
        }
        renderer_draw_text(8, 166, "X <TAB   TAB> Y   B BACK", COLOR_AMBER);
        renderer_draw_text(8, 178, "UP/DOWN NAV (HOLD=FAST)  L/R EDIT", COLOR_PHOSPHOR_GREEN);
        return;
    }

    // Stage Selector Bar: [<] ETAPA X/5 [>]
    renderer_fill_rect(8, 16, 26, 16, COLOR_IRON_PANEL);
    renderer_draw_rect(8, 16, 26, 16, COLOR_AMBER);
    renderer_draw_text(18, 20, "<", COLOR_AMBER);

    renderer_fill_rect(222, 16, 26, 16, COLOR_IRON_PANEL);
    renderer_draw_rect(222, 16, 26, 16, COLOR_AMBER);
    renderer_draw_text(232, 20, ">", COLOR_AMBER);

    char buf[64];
    snprintf(buf, sizeof(buf), "SELECT ETAPA: %d/5 (L/R)", g_game.calib_stage_idx + 1);
    renderer_draw_text(52, 20, buf, COLOR_WHITE);
    renderer_draw_text(190, 20, "TAB 1/4", COLOR_AMBER);

    int s = g_game.calib_stage_idx;
    if (s < 0) s = 0;
    if (s >= STAGE_COUNT) s = STAGE_COUNT - 1;
    const StageConfig *st = &g_balance.stages[s];

    static const char *stage_row_labels[8] = {
        "1. ZERG BASE DELAY",
        "2. SCOURGE BASE DEL",
        "3. HYDRA BASE DELAY",
        "4. ZERG PEAK DELAY",
        "5. SCOURGE PEAK DEL",
        "6. HYDRA PEAK DELAY",
        "7. ULTRA PEAK DELAY",
        "8. STAGE REWARD $"
    };

    for (int r = 0; r < 8; r++) {
        int val = 0;
        switch (r) {
            case 0: val = st->zergling_delay_base; break;
            case 1: val = st->scourge_delay_base; break;
            case 2: val = st->hydralisk_delay_base; break;
            case 3: val = st->zergling_delay_peak; break;
            case 4: val = st->scourge_delay_peak; break;
            case 5: val = st->hydralisk_delay_peak; break;
            case 6: val = st->ultralisk_delay_peak; break;
            case 7: val = st->stage_reward_scrap; break;
        }

        int y = 36 + r * 14;
        int is_sel = (g_game.calib_row == r);
        uint16_t row_bg = is_sel ? COLOR_IRON_LIGHT : COLOR_IRON_PANEL;
        uint16_t row_border = is_sel ? COLOR_AMBER : COLOR_IRON_BORDER;
        uint16_t txt_col = is_sel ? COLOR_WHITE : RGB15(20, 20, 22) | BIT(15);

        renderer_fill_rect(6, y, 244, 13, row_bg);
        renderer_draw_rect(6, y, 244, 13, row_border);

        renderer_draw_text(10, y + 2, stage_row_labels[r], txt_col);

        snprintf(buf, sizeof(buf), "%d", val);
        renderer_draw_text(144, y + 3, buf, COLOR_PHOSPHOR_GREEN);

        // [-] button
        renderer_fill_rect(178, y + 2, 24, 10, COLOR_BLACK);
        renderer_draw_rect(178, y + 2, 24, 10, COLOR_IRON_BORDER);
        renderer_draw_text(187, y + 3, "-", COLOR_WHITE);

        // [+] button
        renderer_fill_rect(214, y + 2, 24, 10, COLOR_BLACK);
        renderer_draw_rect(214, y + 2, 24, 10, COLOR_IRON_BORDER);
        renderer_draw_text(223, y + 3, "+", COLOR_WHITE);
    }

    // Bottom action buttons: [RESTART E1] [RESET DEFAULTS] [RESUME]
    renderer_fill_rect(8, 158, 76, 26, COLOR_LED_RED);
    renderer_draw_rect(8, 158, 76, 26, COLOR_WHITE);
    renderer_draw_text(14, 166, "RESTART E1", COLOR_WHITE);

    renderer_fill_rect(90, 158, 72, 26, COLOR_IRON_PANEL);
    renderer_draw_rect(90, 158, 72, 26, COLOR_AMBER);
    renderer_draw_text(98, 166, "DEFAULTS", COLOR_AMBER);

    renderer_fill_rect(168, 158, 78, 26, COLOR_LED_GREEN);
    renderer_draw_rect(168, 158, 78, 26, COLOR_WHITE);
    renderer_draw_text(182, 166, "RESUME", COLOR_BLACK);
}

void renderer_draw_ui_sandbox(void) {
    // --- TOP SCREEN: Telemetry & Config HUD ---
    // Keep the battlefield and spawned enemies underneath the diagnostic HUD.
    top_fill_rect(0, 0, SCREEN_W, 20, TOP_COLOR_BLACK);
    for (int x = 0; x < SCREEN_W; x++) {
        top_draw_pixel(x, 20, TOP_COLOR_PHOSPHOR_GREEN);
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "FPS:%d T:%d B:%d P:%d S:%d E:%d",
             g_game.prof_fps, g_game.prof_top_ticks, g_game.prof_bot_ticks,
             g_game.prof_pres_ticks, g_game.prof_sim_ticks,
             g_game.prof_enemies_active);
    top_draw_text(4, 1, buf, TOP_COLOR_WHITE);
    top_draw_text(4, 9, "DEBUG SANDBOX", TOP_COLOR_AMBER);
    top_draw_text(180, 9, g_game.sandbox.run_sim ? "RUN" : "PAUSE",
                  g_game.sandbox.run_sim ? TOP_COLOR_LED_GREEN : TOP_COLOR_AMBER);

    if (g_game.sandbox.profiler_compact) {
        // Keep only dynamic evidence in the low-overhead measurement mode.
        top_fill_rect(0, 108, SCREEN_W, 7, TOP_COLOR_IRON_PANEL);
        snprintf(buf, sizeof(buf), "TR:%d TE:%d R:%d E:%d", g_game.prof_top_restore_ticks, g_game.prof_top_enemy_ticks, g_game.prof_bot_base_ticks, g_game.prof_bot_enemy_ticks);
        top_draw_text(6, 108, buf, TOP_COLOR_WHITE);
        snprintf(buf, sizeof(buf), "Q:%d/%d/%d", g_game.prof_sep_checks,
                 g_game.prof_target_candidates, g_game.prof_collision_candidates);
        top_draw_text(154, 108, buf, TOP_COLOR_AMBER);
        top_fill_rect(0, 176, SCREEN_W, 16, TOP_COLOR_IRON_PANEL);
        snprintf(buf, sizeof(buf), "ALIVE:%d TOTAL:%d", g_game.enemies_alive, g_game.sandbox.spawn_count);
        top_draw_text(12, 180, buf, TOP_COLOR_PHOSPHOR_GREEN);
        snprintf(buf, sizeof(buf), "H:%d", g_game.sandbox.last_keys_held & 0x0FFF);
        top_draw_text(214, 180, buf, TOP_COLOR_AMBER);
        renderer_fill_rect(0, 148, SCREEN_W, 44, COLOR_BLACK);
        return;
    }

    // D-Pad adjustable parameters (Rows 0..4)
    static const char *s_tier_names[8] = { "SCOURGE", "ZERGLING", "HYDRALISK", "MUTALISK", "DEFILER", "LURKER", "GUARDIAN", "ULTRALISK" };
    const char *labels[5] = { "ENEMY SPECIES", "ENEMY HP", "ENEMY SPEED", "FIRE CADENCE", "BULLET DMG" };
    for (int r = 0; r < 5; r++) {
        int y = 26 + r * 14;
        int is_sel = (g_game.sandbox.edit_row == r);
        uint16_t row_col = is_sel ? COLOR_WHITE : COLOR_IRON_LIGHT;
        uint16_t val_col = is_sel ? COLOR_AMBER : COLOR_PHOSPHOR_GREEN;

        if (is_sel) {
            top_fill_rect(2, y - 1, SCREEN_W - 4, 13, TOP_COLOR_IRON_PANEL);
            top_draw_text(4, y + 2, ">", TOP_COLOR_AMBER);
        }

        top_draw_text(12, y + 2, labels[r], row_col);

        switch (r) {
            case 0:
                snprintf(buf, sizeof(buf), "%s", s_tier_names[g_game.sandbox.enemy_tier]);
                break;
            case 1:
                snprintf(buf, sizeof(buf), "%d HP", g_game.sandbox.enemy_hp);
                break;
            case 2:
                if (g_game.sandbox.enemy_speed == 0) {
                    snprintf(buf, sizeof(buf), "0 px/s (FROZEN)");
                } else {
                    snprintf(buf, sizeof(buf), "%d px/s", g_game.sandbox.enemy_speed);
                }
                break;
            case 3:
                    int fire_rate = g_game.sandbox.turret_firerate;
                    if (fire_rate < 1) fire_rate = 1;
                    snprintf(buf, sizeof(buf), "%d f (%d/s)", fire_rate, 60 / fire_rate);
                break;
            case 4:
                snprintf(buf, sizeof(buf), "%d DMG", g_game.sandbox.turret_damage);
                break;
        }
        top_draw_text(120, y + 2, buf, val_col);
    }

    // Sandbox-local profiler row; the normal battlefield HUD is covered by
    // this diagnostic panel, so repeat the phase timings here as evidence.
    top_fill_rect(0, 108, SCREEN_W, 7, TOP_COLOR_IRON_PANEL);
    snprintf(buf, sizeof(buf), "R:%d E:%d F:%d U:%d", g_game.prof_bot_base_ticks,
             g_game.prof_bot_enemy_ticks, g_game.prof_bot_fx_ticks,
             g_game.prof_bot_ui_ticks);
    top_draw_text(6, 108, buf, TOP_COLOR_WHITE);
    top_fill_rect(210, 115, 46, 7, TOP_COLOR_BLACK);
    top_draw_text(214, 116, g_game.sandbox.separation_enabled ? "SEP ON" : "SEP OFF", TOP_COLOR_AMBER);
    snprintf(buf, sizeof(buf), "Q:%d/%d/%d", g_game.prof_sep_checks,
             g_game.prof_target_candidates, g_game.prof_collision_candidates);
    top_draw_text(154, 108, buf, TOP_COLOR_AMBER);

    // Bottom telemetry stats on top screen
    for (int x = 0; x < SCREEN_W; x++) {
        top_draw_pixel(x, 114, TOP_COLOR_IRON_BORDER);
    }
    top_draw_text(8, 120, "CONTROLS:", TOP_COLOR_AMBER);
    top_draw_text(12, 132, "TOUCH: Drop Enemy at pos", TOP_COLOR_IRON_LIGHT);
    top_draw_text(12, 144, "D-PAD: Select & Tune params", TOP_COLOR_IRON_LIGHT);
    top_draw_text(12, 156, "START: Run/Pause  Y: Step 1F", TOP_COLOR_IRON_LIGHT);
    top_draw_text(12, 168, "X: Clear Entities L+SEL: Exit", TOP_COLOR_IRON_LIGHT);

    top_fill_rect(0, 176, SCREEN_W, 16, TOP_COLOR_IRON_PANEL);
    snprintf(buf, sizeof(buf), "ALIVE:%d TOTAL:%d", g_game.enemies_alive, g_game.sandbox.spawn_count);
    top_draw_text(12, 180, buf, TOP_COLOR_PHOSPHOR_GREEN);
    snprintf(buf, sizeof(buf), "H:%d", g_game.sandbox.last_keys_held & 0x0FFF);
    top_draw_text(214, 180, buf, TOP_COLOR_AMBER);

    // --- BOTTOM SCREEN: Battlefield Overlay & Touch Control Bar ---
    // Bottom control bar (y >= 148, h = 44)
    tiles_dirty_mark_rect(0, 148, SCREEN_W, 44, 1, s_bot_fb_idx);
    renderer_fill_rect(0, 148, SCREEN_W, 44, COLOR_BLACK);
    renderer_draw_line(0, 148, SCREEN_W, 148, COLOR_IRON_BORDER);

    // [CLEAR] button (6..50)
    renderer_fill_rect(6, 152, 44, 18, COLOR_LED_RED);
    renderer_draw_rect(6, 152, 44, 18, COLOR_WHITE);
    renderer_draw_text(12, 157, "CLEAR", COLOR_WHITE);

    // [RUN / PAUSE] button (54..110)
    uint16_t run_btn_bg = g_game.sandbox.run_sim ? TOP_COLOR_LED_GREEN : TOP_COLOR_AMBER;
    renderer_fill_rect(54, 152, 56, 18, run_btn_bg);
    renderer_draw_rect(54, 152, 56, 18, COLOR_WHITE);
    renderer_draw_text(60, 157, g_game.sandbox.run_sim ? "RUNNING" : "PAUSED", COLOR_BLACK);

    // [STEP 1F] button (114..160)
    renderer_fill_rect(114, 152, 46, 18, COLOR_IRON_PANEL);
    renderer_draw_rect(114, 152, 46, 18, COLOR_WHITE);
    renderer_draw_text(120, 157, "STEP 1F", COLOR_AMBER);

    // [INF AMMO] button (164..205)
    uint16_t inf_bg = g_game.sandbox.turret_infinite_ammo ? COLOR_PHOSPHOR_GREEN : COLOR_IRON_PANEL;
    renderer_fill_rect(164, 152, 42, 18, inf_bg);
    renderer_draw_rect(164, 152, 42, 18, COLOR_WHITE);
    renderer_draw_text(168, 157, "INF AMMO", COLOR_BLACK);

    // [EXIT] button (210..250)
    renderer_fill_rect(210, 152, 40, 18, COLOR_IRON_BORDER);
    renderer_draw_rect(210, 152, 40, 18, COLOR_WHITE);
    renderer_draw_text(218, 157, "EXIT", COLOR_WHITE);

    // Hint in bottom bar
    renderer_draw_text(8, 175, "TOUCH FIELD TO DROP ENEMY", COLOR_AMBER);
}

void renderer_present(void) {
    // Current draw buffer was s_bot_fb_idx (0=VRAM_A, 1=VRAM_B).
    // Flip display to the buffer we just finished drawing:
    videoSetMode(s_bot_fb_idx ? MODE_FB1 : MODE_FB0);
    // Next frame will draw into the opposite buffer:
    s_bot_fb_idx ^= 1;
    g_backbuffer = s_bot_fb_idx ? (uint16_t *)VRAM_B : (uint16_t *)VRAM_A;
}

void renderer_refresh_top_vram(void) {
    if (s_top_vram && g_top_backbuffer) {
        DC_FlushRange(g_top_backbuffer, SCREEN_W * SCREEN_H);
        dmaCopyWords(1, g_top_backbuffer, s_top_vram, SCREEN_W * SCREEN_H);
    }
}

void top_screen_present(void) {
    if (s_top_vram) {
        // Flush CPU D-cache before DMA transfer to ensure hardware coherency!
        DC_FlushRange(g_top_backbuffer, SCREEN_W * SCREEN_H);
        dmaCopyWords(1, g_top_backbuffer, s_top_vram, SCREEN_W * SCREEN_H);
    }
}
