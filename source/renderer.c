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
    ['<'] = {0x2, 0x4, 0x8, 0x4, 0x2, 0},
    ['>'] = {0x8, 0x4, 0x2, 0x4, 0x8, 0},
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
    renderer_fill_rect(54, 156, 48, 24, COLOR_IRON_PANEL);
    renderer_draw_rect(54, 156, 48, 24, COLOR_AMBER);
    // Calibration button
    renderer_fill_rect(108, 156, 36, 24, COLOR_IRON_PANEL);
    renderer_draw_rect(108, 156, 36, 24, COLOR_PHOSPHOR_GREEN);
    renderer_draw_text(112, 164, "CALIB", COLOR_PHOSPHOR_GREEN);

    // Sandbox test button
    renderer_fill_rect(148, 156, 36, 24, COLOR_IRON_PANEL);
    renderer_draw_rect(148, 156, 36, 24, COLOR_AMBER);
    renderer_draw_text(152, 164, "SANDB", COLOR_AMBER);

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
    renderer_draw_text(65, 8, "SANCTUM UPGRADES", COLOR_AMBER);

    char scrap_buf[32];
    format_number_compact(scrap_buf, sizeof(scrap_buf), g_game.scrap);
    char buf[64];
    snprintf(buf, sizeof(buf), "SCRAP: %s", scrap_buf);
    renderer_draw_text(160, 8, buf, COLOR_PHOSPHOR_GREEN);

    static const struct {
        int x, y, w, h;
        const char *title;
    } s_card_pos[8] = {
        { 10, 24, 110, 32, "CALIBER" },
        { 130, 24, 110, 32, "FIRE RATE" },
        { 10, 62, 110, 32, "MAG SIZE" },
        { 130, 62, 110, 32, "BIO HARVEST" },
        { 10, 100, 110, 32, "AUTO SUPPLY" },
        { 130, 100, 110, 32, "AUTO TARGET" },
        { 10, 138, 110, 32, "EXTRA TURRETS" },
        { 130, 138, 110, 32, "RANGE" }
    };

    for (int i = 0; i < 8; i++) {
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
            snprintf(buf, sizeof(buf), "TARGET: %s", g_game.upgrades.auto_target ? "ON" : "OFF");
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            if (!g_game.upgrades.auto_target) {
                snprintf(buf, sizeof(buf), "COGIT %s$", cost_str);
            } else {
                snprintf(buf, sizeof(buf), "MAXED");
            }
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 6) {
            snprintf(buf, sizeof(buf), "TURRETS +%d", g_game.upgrades.extra_turrets);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            snprintf(buf, sizeof(buf), "+1  %s$", cost_str);
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
        } else if (i == 7) {
            int lv = g_game.upgrades.range_lvl;
            snprintf(buf, sizeof(buf), "RANGE LV%d", lv);
            renderer_draw_text(x + 4, y + 4, buf, text_col);
            if (lv < 5) snprintf(buf, sizeof(buf), "%d>%d %s$", g_balance.turret_range[lv], g_balance.turret_range[lv + 1], cost_str);
            else snprintf(buf, sizeof(buf), "MAXED");
            renderer_draw_text(x + 4, y + 16, buf, cost_col);
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
        static const char *page_titles[4] = { "WAVE SPAWNS", "ENEMY STATS", "BASE / TURRETS", "UPGRADES" };
        const char *title = page_titles[g_game.calib_page];
        renderer_draw_text(6, 18, title, COLOR_WHITE);
        snprintf(buf, sizeof(buf), "PAGE %d/4", g_game.calib_page + 1);
        renderer_draw_text(190, 18, buf, COLOR_AMBER);
        int first = (g_game.calib_page == 1) ? (g_game.calib_row / 10) * 10 : (g_game.calib_row / 10) * 10;
        int last = (g_game.calib_page == 3) ? 40 : 24;
        static const char *enemy_labels[24] = { "LARVA HP", "LARVA SCRAP", "LARVA BITE DMG", "LARVA BITE FRAMES", "RIPPER HP", "RIPPER SCRAP", "RIPPER BITE DMG", "RIPPER BITE FRAMES", "HORMAG HP", "HORMAG SCRAP", "HORMAG BITE DMG", "HORMAG BITE FRAMES", "RAVENER HP", "RAVENER SCRAP", "RAVENER BITE DMG", "RAVENER BITE FRAMES", "CARNIFEX HP", "CARNIFEX SCRAP", "CARNIFEX BITE DMG", "CARNIFEX BITE FRAMES", "HIEROPH HP", "HIEROPH SCRAP", "HIEROPH BITE DMG", "HIEROPH BITE FRAMES" };
        static const char *base_labels[24] = { "BUNKER HP", "WAVE FRAMES", "BONUS BASE", "BONUS / WAVE", "DAMAGE LV0", "DAMAGE LV1", "DAMAGE LV2", "DAMAGE LV3", "DAMAGE LV4", "RANGE LV0", "RANGE LV1", "RANGE LV2", "RANGE LV3", "RANGE LV4", "CONVEYOR LV0", "CONVEYOR LV1", "CONVEYOR LV2", "CONVEYOR LV3", "MAGAZINE LV0", "MAGAZINE LV1", "MAGAZINE LV2", "MAGAZINE LV3", "MAGAZINE LV4", "MAGAZINE LV5" };
        for (int n = 0; n < 10 && first + n < last; n++) {
            int r = first + n, val = 0;
            if (g_game.calib_page == 1) { int e=r/4, f=r%4; val = f==0 ? g_balance.enemy_hp[e] : f==1 ? g_balance.enemy_scrap[e] : f==2 ? g_balance.enemy_bite_damage[e] : g_balance.enemy_bite_interval[e]; }
            else if (g_game.calib_page == 2) {
                if (r < 4) { int *p[4] = { &g_balance.bunker_start_hp, &g_balance.wave_duration_frames, &g_balance.wave_bonus_base, &g_balance.wave_bonus_per_wave }; val = *p[r]; }
                else if (r < 9) val = g_balance.turret_damage[r-4]; else if (r < 14) val = g_balance.turret_range[r-9]; else if (r < 18) val = g_balance.conveyor_reload_interval[r-14]; else val = g_balance.turret_magazine[r-18];
            } else {
                int u = r / 5, l = r % 5; val = (int)g_balance.upgrade_costs[u][l];
            }
            int y = 32 + n * 13; int sel = (r == g_game.calib_row);
            renderer_fill_rect(6, y, 244, 12, sel ? COLOR_IRON_LIGHT : COLOR_IRON_PANEL);
            renderer_draw_rect(6, y, 244, 12, sel ? COLOR_AMBER : COLOR_IRON_BORDER);
            if (g_game.calib_page == 1) renderer_draw_text(10, y + 2, enemy_labels[r], COLOR_WHITE);
            else if (g_game.calib_page == 2) renderer_draw_text(10, y + 2, base_labels[r], COLOR_WHITE);
            else {
                static const char *names[7] = { "CALIBER", "FIRE RATE", "MAG SIZE", "BIO HARVEST", "SUPPLY CONVEYOR", "AUTO TARGET", "EXTRA TURRETS" };
                if (r < 35) snprintf(buf, sizeof(buf), "%s LV%d", names[r / 5], r % 5);
                else snprintf(buf, sizeof(buf), "RANGE LV%d", r - 35);
                renderer_draw_text(10, y + 2, buf, COLOR_WHITE);
            }
            snprintf(buf, sizeof(buf), "%d  [-] [+]", val); renderer_draw_text(150, y + 2, buf, COLOR_PHOSPHOR_GREEN);
        }
        renderer_draw_text(8, 166, "X <TAB   TAB> Y   B BACK", COLOR_AMBER);
        renderer_draw_text(8, 178, "UP/DOWN NAV (HOLD=FAST)  L/R EDIT", COLOR_PHOSPHOR_GREEN);
        return;
    }

    // Wave Selector Bar: [<] WAVE X/20 [>]
    renderer_fill_rect(8, 16, 26, 16, COLOR_IRON_PANEL);
    renderer_draw_rect(8, 16, 26, 16, COLOR_AMBER);
    renderer_draw_text(18, 20, "<", COLOR_AMBER);

    renderer_fill_rect(222, 16, 26, 16, COLOR_IRON_PANEL);
    renderer_draw_rect(222, 16, 26, 16, COLOR_AMBER);
    renderer_draw_text(232, 20, ">", COLOR_AMBER);

    char buf[64];
    snprintf(buf, sizeof(buf), "SELECT WAVE: %d/20 (L/R)", g_game.calib_wave_idx + 1);
    renderer_draw_text(52, 20, buf, COLOR_WHITE);
    renderer_draw_text(190, 20, "TAB 1/4", COLOR_AMBER);

    int w = g_game.calib_wave_idx;
    const WaveDef *wd = &g_balance.waves[w];

    // 24 rows, shown ten at a time while scrolling with Up/Down
    static const char *row_labels[24] = {
        "T1 LARVA COUNT", "T1 LARVA DELAY", "T1 LARVA SPEED", "T1 LARVA HP",
        "T2 RIPPER COUNT", "T2 RIPPER DELAY", "T2 RIPPER SPEED", "T2 RIPPER HP",
        "T3 HORMAG COUNT", "T3 HORMAG DELAY", "T3 HORMAG SPEED", "WAVE SCRAP",
        "T4 RAVENER COUNT", "T4 RAVENER DELAY", "T4 RAVENER SPEED", "T4 RAVENER HP",
        "T5 CARNIFEX COUNT", "T5 CARNIFEX DELAY", "T5 CARNIFEX SPEED", "T5 CARNIFEX HP",
        "T6 HIEROPH COUNT", "T6 HIEROPH DELAY", "T6 HIEROPH SPEED", "T6 HIEROPH HP"
    };

    int first_row = (g_game.calib_row / 10) * 10;
    for (int n = 0; n < 10 && first_row + n < 24; n++) {
        int r = first_row + n;
        int tier = r / 4;
        int param = r % 4;
        int val = 0;
        if (r == 11) val = wd->scrap_base;
        else if (tier < 3 && param == 0) val = wd->tiers[tier].count;
        else if (tier < 3 && param == 1) val = wd->tiers[tier].delay;
        else if (tier < 3 && param == 2) val = wd->tiers[tier].speed;
        else if (param == 3) val = g_balance.enemy_hp[tier];
        else if (tier >= 3 && param == 0) val = g_balance.advanced_waves[w][tier - 3].count;
        else if (tier >= 3 && param == 1) val = g_balance.advanced_waves[w][tier - 3].delay;
        else if (tier >= 3 && param == 2) val = g_balance.advanced_waves[w][tier - 3].speed;

        int y = 31 + n * 10;
        int is_sel = (g_game.calib_row == r);
        uint16_t row_bg = is_sel ? COLOR_IRON_LIGHT : COLOR_IRON_PANEL;
        uint16_t row_border = is_sel ? COLOR_AMBER : COLOR_IRON_BORDER;
        uint16_t txt_col = is_sel ? COLOR_WHITE : RGB15(20, 20, 22) | BIT(15);

        // Highlight header tier group with slightly warmer text
        if (param == 0 && !is_sel) txt_col = COLOR_AMBER;

        renderer_fill_rect(6, y, 244, 10, row_bg);
        renderer_draw_rect(6, y, 244, 10, row_border);

        renderer_draw_text(10, y + 1, row_labels[r], txt_col);

        snprintf(buf, sizeof(buf), "%d", val);
        renderer_draw_text(142, y + 1, buf, COLOR_PHOSPHOR_GREEN);

        // [-] button
        renderer_fill_rect(178, y + 1, 24, 10, COLOR_BLACK);
        renderer_draw_rect(178, y + 1, 24, 10, COLOR_IRON_BORDER);
        renderer_draw_text(187, y + 2, "-", COLOR_WHITE);

        // [+] button
        renderer_fill_rect(214, y + 1, 24, 10, COLOR_BLACK);
        renderer_draw_rect(214, y + 1, 24, 10, COLOR_IRON_BORDER);
        renderer_draw_text(223, y + 2, "+", COLOR_WHITE);
    }

    // Bottom action buttons: [RESTART W1] [RESET DEFAULTS] [RESUME]
    renderer_fill_rect(8, 158, 76, 26, COLOR_LED_RED);
    renderer_draw_rect(8, 158, 76, 26, COLOR_WHITE);
    renderer_draw_text(14, 166, "RESTART W1", COLOR_WHITE);

    renderer_fill_rect(90, 158, 72, 26, COLOR_IRON_PANEL);
    renderer_draw_rect(90, 158, 72, 26, COLOR_AMBER);
    renderer_draw_text(98, 166, "DEFAULTS", COLOR_AMBER);

    renderer_fill_rect(168, 158, 78, 26, COLOR_LED_GREEN);
    renderer_draw_rect(168, 158, 78, 26, COLOR_WHITE);
    renderer_draw_text(182, 166, "RESUME", COLOR_BLACK);
}

void renderer_draw_ui_sandbox(void) {
    // --- TOP SCREEN: Telemetry & Config HUD ---
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        g_top_backbuffer[i] = COLOR_BLACK;
    }
    top_fill_rect(0, 0, SCREEN_W, 20, COLOR_IRON_PANEL);
    for (int x = 0; x < SCREEN_W; x++) {
        top_draw_pixel(x, 20, COLOR_PHOSPHOR_GREEN);
    }
    top_draw_text(6, 6, "--- DEBUG SANDBOX LAB ---", COLOR_AMBER);
    top_draw_text(180, 6, g_game.sandbox.run_sim ? "[RUN]" : "[STEP]", 
                  g_game.sandbox.run_sim ? COLOR_LED_GREEN : COLOR_AMBER);

    // D-Pad adjustable parameters (Rows 0..5)
    static const char *s_tier_names[6] = { "T0 LARVA", "T1 RIPPER", "T2 HORMAG", "T3 WARRIOR", "T4 GENEST", "T5 CARNIFEX" };
    char buf[48];

    const char *labels[6] = { "ENEMY TIER", "ENEMY HP", "ENEMY SPEED", "TURRET RANGE", "FIRE CADENCE", "BULLET DMG" };
    for (int r = 0; r < 6; r++) {
        int y = 26 + r * 14;
        int is_sel = (g_game.sandbox.edit_row == r);
        uint16_t row_col = is_sel ? COLOR_WHITE : COLOR_IRON_LIGHT;
        uint16_t val_col = is_sel ? COLOR_AMBER : COLOR_PHOSPHOR_GREEN;

        if (is_sel) {
            top_fill_rect(2, y - 1, SCREEN_W - 4, 13, COLOR_IRON_PANEL);
            top_draw_text(4, y + 2, ">", COLOR_AMBER);
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
                snprintf(buf, sizeof(buf), "%d px", g_game.sandbox.turret_range);
                break;
            case 4:
                snprintf(buf, sizeof(buf), "%d f (%d/s)", g_game.sandbox.turret_firerate, 60 / g_game.sandbox.turret_firerate);
                break;
            case 5:
                snprintf(buf, sizeof(buf), "%d DMG", g_game.sandbox.turret_damage);
                break;
        }
        top_draw_text(120, y + 2, buf, val_col);
    }

    // Bottom telemetry stats on top screen
    for (int x = 0; x < SCREEN_W; x++) {
        top_draw_pixel(x, 114, COLOR_IRON_BORDER);
    }
    top_draw_text(8, 120, "CONTROLS:", COLOR_AMBER);
    top_draw_text(12, 132, "TOUCH: Drop Enemy at pos", COLOR_IRON_LIGHT);
    top_draw_text(12, 144, "D-PAD: Select & Tune params", COLOR_IRON_LIGHT);
    top_draw_text(12, 156, "START: Run/Pause  Y: Step 1F", COLOR_IRON_LIGHT);
    top_draw_text(12, 168, "X: Clear Entities L+SEL: Exit", COLOR_IRON_LIGHT);

    snprintf(buf, sizeof(buf), "ALIVE: %d | TOTAL: %d", g_game.enemies_alive, g_game.sandbox.spawn_count);
    top_draw_text(12, 180, buf, COLOR_PHOSPHOR_GREEN);

    // --- BOTTOM SCREEN: Battlefield Overlay & Touch Control Bar ---
    // Turret range circle preview
    if (g_turrets[0].placed) {
        int tx = g_turrets[0].x;
        int ty = g_turrets[0].y;
        renderer_draw_circle(tx, ty, g_game.sandbox.turret_range, COLOR_AMBER, 0);
    }

    // Bottom control bar (y >= 148, h = 44)
    renderer_fill_rect(0, 148, SCREEN_W, 44, COLOR_BLACK);
    renderer_draw_line(0, 148, SCREEN_W, 148, COLOR_IRON_BORDER);

    // [CLEAR] button (6..50)
    renderer_fill_rect(6, 152, 44, 18, COLOR_LED_RED);
    renderer_draw_rect(6, 152, 44, 18, COLOR_WHITE);
    renderer_draw_text(12, 157, "CLEAR", COLOR_WHITE);

    // [RUN / PAUSE] button (54..110)
    uint16_t run_btn_bg = g_game.sandbox.run_sim ? COLOR_LED_GREEN : COLOR_AMBER;
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
    dmaCopyWords(3, g_backbuffer, VRAM_A, sizeof(g_backbuffer));
}

void top_screen_present(void) {
    if (s_top_vram) {
        dmaCopyWords(1, g_top_backbuffer, s_top_vram, sizeof(g_top_backbuffer));
    }
}
