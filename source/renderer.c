#include "game.h"
#include "tiles.h"
#include "enemy_data.h"

// Define VRAM backbuffers aligned for fast DMA
uint16_t g_backbuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));
uint16_t g_top_backbuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));

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
};

void renderer_init(void) {
    // 1. Bottom Screen: Main engine in Direct FB0 mode on bottom LCD
    lcdMainOnBottom();
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);

    // 2. Top Screen: Sub engine 16-bit Bitmap Mode 5
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    s_top_bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    s_top_vram = bgGetGfxPtr(s_top_bg);

    // 3. Asset generator
    tiles_init();

    // 4. Initial clear
    renderer_clear(COLOR_DECK_FLOOR);
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        g_top_backbuffer[i] = COLOR_DECK_FLOOR;
    }
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

void top_draw_pixel(int x, int y, uint16_t color) {
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

void top_fill_rect(int x, int y, int w, int h, uint16_t color) {
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : (x + w);
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : (y + h);
    for (int j = y0; j < y1; j++) {
        uint16_t *line = &g_top_backbuffer[j * SCREEN_W];
        for (int i = x0; i < x1; i++) {
            line[i] = color;
        }
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

void top_draw_text(int x, int y, const char *str, uint16_t color) {
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
    tiles_render_urban_ground(g_top_backbuffer, 0);
}

void renderer_draw_battlefield_bottom(void) {
    tiles_render_urban_ground(g_backbuffer, 192);
    // Sanctum bunker placed at bottom center (128, 172)
    tiles_draw_central_bunker(g_backbuffer, 128, 172, g_game.bunker_hp, g_game.bunker_max_hp);
}

void renderer_draw_turret(const Turret *t, int is_selected) {
    if (!t->placed) return;
    
    // Selected or target highlight
    if (is_selected) {
        renderer_draw_circle(t->x, t->y, 16, COLOR_AMBER, 0);
        renderer_draw_circle(t->x, t->y, t->range, COLOR_AMBER, 0);
    }

    // Draw canonical 32-angle discrete RotSprite turret
    turret_draw_angle(t->x, t->y, t->type, t->current_angle, is_selected);

    // Muzzle flash when firing
    if (t->flash_timer > 0) {
        int tip_dist = 14;
        int fx = t->x + ((fixed_cos(t->current_angle) * tip_dist) >> FP_SHIFT);
        int fy = t->y + ((fixed_sin(t->current_angle) * tip_dist) >> FP_SHIFT);
        renderer_draw_circle(fx, fy, 2, COLOR_MUZZLE_FLASH, 1);
        renderer_draw_pixel(fx, fy, COLOR_WHITE);
    }

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

void renderer_draw_enemies_top(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;
        int gx = FROM_FP(g_enemies[i].x);
        int gy = FROM_FP(g_enemies[i].y);
        // Top screen: Y in [0..191]
        if (gy >= -16 && gy < 192) {
            enemy_draw_sprite_to_buffer(g_top_backbuffer, gx, gy, g_enemies[i].variant,
                                       g_enemies[i].anim_frame, g_enemies[i].dir);
            // Health bar if damaged
            if (g_enemies[i].hp < g_enemies[i].max_hp) {
                int bw = 14;
                int bx = gx - bw / 2;
                int by = gy - 10;
                top_fill_rect(bx - 1, by - 1, bw + 2, 3, COLOR_BLACK);
                int fill = (int)((g_enemies[i].hp * bw) / g_enemies[i].max_hp);
                if (fill > 0) {
                    top_fill_rect(bx, by, fill, 1, COLOR_LED_RED);
                }
            }
        }
    }
}

void renderer_draw_enemies_bottom(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;
        int gx = FROM_FP(g_enemies[i].x);
        int gy = FROM_FP(g_enemies[i].y);
        // Bottom screen: Y in [192..383] -> local Y = gy - 192
        int ly = gy - 192;
        if (ly >= -16 && ly < SCREEN_H) {
            enemy_draw_sprite_to_buffer(g_backbuffer, gx, ly, g_enemies[i].variant,
                                       g_enemies[i].anim_frame, g_enemies[i].dir);
            // Health bar if damaged
            if (g_enemies[i].hp < g_enemies[i].max_hp) {
                int bw = 14;
                int bx = gx - bw / 2;
                int by = ly - 10;
                renderer_fill_rect(bx - 1, by - 1, bw + 2, 3, COLOR_BLACK);
                int fill = (int)((g_enemies[i].hp * bw) / g_enemies[i].max_hp);
                if (fill > 0) {
                    renderer_fill_rect(bx, by, fill, 1, COLOR_LED_RED);
                }
            }
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
    }
}

void renderer_draw_splatters_top(void) {
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life <= 0) continue;
        int gx = g_splatters[i].x;
        int gy = g_splatters[i].y;
        if (gy >= 0 && gy < SCREEN_H) {
            top_draw_pixel(gx, gy, g_splatters[i].color);
            if (g_splatters[i].size > 1) {
                top_draw_pixel(gx + 1, gy, g_splatters[i].color);
                top_draw_pixel(gx, gy + 1, g_splatters[i].color);
            }
        }
    }
}

void renderer_draw_splatters_bottom(void) {
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life <= 0) continue;
        int gx = g_splatters[i].x;
        int gy = g_splatters[i].y - 192;
        if (gy >= 0 && gy < SCREEN_H) {
            renderer_draw_pixel(gx, gy, g_splatters[i].color);
            if (g_splatters[i].size > 1) {
                renderer_draw_pixel(gx + 1, gy, g_splatters[i].color);
                renderer_draw_pixel(gx, gy + 1, g_splatters[i].color);
            }
        }
    }
}

void renderer_draw_death_particles_top(void) {
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;
        int gx = FROM_FP(g_death_particles[i].x);
        int gy = FROM_FP(g_death_particles[i].y);
        if (gy >= 0 && gy < SCREEN_H) {
            top_draw_pixel(gx, gy, g_death_particles[i].color);
        }
    }
}

void renderer_draw_death_particles_bottom(void) {
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;
        int gx = FROM_FP(g_death_particles[i].x);
        int gy = FROM_FP(g_death_particles[i].y) - 192;
        if (gy >= 0 && gy < SCREEN_H) {
            renderer_draw_pixel(gx, gy, g_death_particles[i].color);
        }
    }
}

void renderer_draw_ui_prep(void) {
    // Top HUD banner
    top_fill_rect(0, 0, SCREEN_W, 14, COLOR_BLACK);
    char buf[64];
    snprintf(buf, sizeof(buf), "WAVE %d/20", g_game.wave_number);
    top_draw_text(6, 4, buf, COLOR_AMBER);
    
    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    top_draw_text(160, 4, buf, COLOR_PHOSPHOR_GREEN);

    // Bottom UI: Ammo depot & control buttons
    // Bunker Scrap / Depot dock on left
    renderer_fill_rect(6, 150, 44, 36, COLOR_IRON_PANEL);
    renderer_draw_rect(6, 150, 44, 36, COLOR_IRON_BORDER);
    renderer_draw_text(12, 156, "AMMO", COLOR_AMBER);
    renderer_draw_text(10, 168, "DEPOT", COLOR_WHITE);

    // Start wave button
    renderer_fill_rect(190, 150, 60, 36, COLOR_LED_GREEN);
    renderer_draw_rect(190, 150, 60, 36, COLOR_WHITE);
    renderer_draw_text(198, 164, "START", COLOR_BLACK);

    // Upgrade button
    renderer_fill_rect(60, 156, 50, 24, COLOR_IRON_PANEL);
    renderer_draw_rect(60, 156, 50, 24, COLOR_AMBER);
    renderer_draw_text(65, 164, "UPGRADES", COLOR_AMBER);

    // Instruction banner
    renderer_draw_text(50, 138, "DRAG AMMO TO RELOAD / TAP ENEMY", COLOR_WHITE);
}

void renderer_draw_ui_wave(void) {
    // Top HUD banner
    top_fill_rect(0, 0, SCREEN_W, 14, COLOR_BLACK);
    char buf[64];
    snprintf(buf, sizeof(buf), "WAVE %d/20", g_game.wave_number);
    top_draw_text(6, 4, buf, COLOR_AMBER);

    int sec_left = g_game.wave_timer / 60;
    snprintf(buf, sizeof(buf), "TIME: %ds", sec_left);
    top_draw_text(80, 4, buf, COLOR_WHITE);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    top_draw_text(170, 4, buf, COLOR_PHOSPHOR_GREEN);

    // Bottom UI: Ammo crate dock for quick tactile reload
    renderer_fill_rect(6, 156, 40, 30, COLOR_IRON_PANEL);
    renderer_draw_rect(6, 156, 40, 30, COLOR_IRON_BORDER);
    renderer_draw_text(10, 162, "AMMO", COLOR_AMBER);
    renderer_draw_text(10, 172, "DRAG", COLOR_WHITE);

    // If currently dragging ammo
    if (g_game.is_dragging_ammo) {
        renderer_draw_circle(g_game.drag_x, g_game.drag_y, 8, COLOR_AMBER, 1);
        renderer_draw_text(g_game.drag_x - 6, g_game.drag_y - 3, "BOX", COLOR_BLACK);
    }
}

void renderer_draw_ui_pause(void) {
    renderer_fill_rect(48, 60, 160, 72, COLOR_BLACK);
    renderer_draw_rect(48, 60, 160, 72, COLOR_AMBER);
    renderer_draw_text(100, 75, "PAUSED", COLOR_AMBER);
    renderer_draw_text(70, 95, "PRESS START TO RESUME", COLOR_WHITE);
}

void renderer_draw_ui_game_over(void) {
    renderer_fill_rect(40, 50, 176, 92, COLOR_BLACK);
    renderer_draw_rect(40, 50, 176, 92, COLOR_LED_RED);
    renderer_draw_text(85, 65, "SANCTUM FALLEN", COLOR_LED_RED);
    
    char buf[64];
    snprintf(buf, sizeof(buf), "WAVES SURVIVED: %d", g_game.wave_number - 1);
    renderer_draw_text(65, 85, buf, COLOR_WHITE);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    snprintf(buf, sizeof(buf), "TOTAL SCRAP: %s", scrap_buf);
    renderer_draw_text(65, 100, buf, COLOR_AMBER);

    renderer_draw_text(65, 120, "TAP ANYWHERE TO RETRY", COLOR_PHOSPHOR_GREEN);
}

void renderer_draw_ui_upgrades(void) {
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    renderer_draw_rect(2, 2, SCREEN_W - 4, SCREEN_H - 4, COLOR_IRON_BORDER);
    renderer_draw_text(75, 8, "SANCTUM UPGRADES", COLOR_AMBER);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    char buf[64];
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    renderer_draw_text(160, 8, buf, COLOR_PHOSPHOR_GREEN);

    // Tab 1: Caliber
    renderer_fill_rect(10, 24, 110, 32, COLOR_IRON_PANEL);
    renderer_draw_rect(10, 24, 110, 32, COLOR_IRON_BORDER);
    snprintf(buf, sizeof(buf), "CALIBER LV%d", g_game.upgrades.caliber_lvl);
    renderer_draw_text(14, 28, buf, COLOR_WHITE);
    renderer_draw_text(14, 40, "+DMG 100$", COLOR_AMBER);

    // Tab 2: Cadence
    renderer_fill_rect(130, 24, 110, 32, COLOR_IRON_PANEL);
    renderer_draw_rect(130, 24, 110, 32, COLOR_IRON_BORDER);
    snprintf(buf, sizeof(buf), "FIRE RATE LV%d", g_game.upgrades.firerate_lvl);
    renderer_draw_text(134, 28, buf, COLOR_WHITE);
    renderer_draw_text(134, 40, "+ROF 150$", COLOR_AMBER);

    // Tab 3: Ammo Cap
    renderer_fill_rect(10, 62, 110, 32, COLOR_IRON_PANEL);
    renderer_draw_rect(10, 62, 110, 32, COLOR_IRON_BORDER);
    snprintf(buf, sizeof(buf), "MAG SIZE LV%d", g_game.upgrades.mag_size_lvl);
    renderer_draw_text(14, 66, buf, COLOR_WHITE);
    renderer_draw_text(14, 78, "+50 AMMO 80$", COLOR_AMBER);

    // Tab 4: Bio Harvest
    renderer_fill_rect(130, 62, 110, 32, COLOR_IRON_PANEL);
    renderer_draw_rect(130, 62, 110, 32, COLOR_IRON_BORDER);
    snprintf(buf, sizeof(buf), "BIO HARVEST LV%d", g_game.upgrades.bio_harvest_lvl);
    renderer_draw_text(134, 66, buf, COLOR_WHITE);
    renderer_draw_text(134, 78, "+SCRAP 200$", COLOR_AMBER);

    // Tab 5: Conveyor Loader (Automation)
    renderer_fill_rect(10, 100, 110, 32, COLOR_IRON_PANEL);
    renderer_draw_rect(10, 100, 110, 32, COLOR_IRON_BORDER);
    snprintf(buf, sizeof(buf), "CONVEYOR LV%d", g_game.upgrades.conveyor_lvl);
    renderer_draw_text(14, 104, buf, COLOR_WHITE);
    renderer_draw_text(14, 116, "AUTO-LOAD 500$", COLOR_AMBER);

    // Tab 6: Auto Targeting
    renderer_fill_rect(130, 100, 110, 32, COLOR_IRON_PANEL);
    renderer_draw_rect(130, 100, 110, 32, COLOR_IRON_BORDER);
    snprintf(buf, sizeof(buf), "AUTO-TARGET: %s", g_game.upgrades.auto_target ? "ON" : "OFF");
    renderer_draw_text(134, 104, buf, COLOR_WHITE);
    renderer_draw_text(134, 116, "COGITATOR 750$", COLOR_AMBER);

    // Return button
    renderer_fill_rect(90, 150, 76, 28, COLOR_LED_GREEN);
    renderer_draw_rect(90, 150, 76, 28, COLOR_WHITE);
    renderer_draw_text(108, 160, "BACK", COLOR_BLACK);
}

void renderer_present(void) {
    dmaCopyWords(3, g_backbuffer, VRAM_A, sizeof(g_backbuffer));
}

void top_screen_present(void) {
    if (s_top_vram) {
        dmaCopyWords(1, g_top_backbuffer, s_top_vram, sizeof(g_top_backbuffer));
    }
}
