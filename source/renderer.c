#include "game.h"
#include "tiles.h"

uint16_t g_backbuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));

static const uint8_t s_font_letters[26][5] = {
    {0x2, 0x5, 0x7, 0x5, 0x5}, // A
    {0x6, 0x5, 0x6, 0x5, 0x6}, // B
    {0x7, 0x4, 0x4, 0x4, 0x7}, // C
    {0x6, 0x5, 0x5, 0x5, 0x6}, // D
    {0x7, 0x4, 0x6, 0x4, 0x7}, // E
    {0x7, 0x4, 0x6, 0x4, 0x4}, // F
    {0x7, 0x4, 0x5, 0x5, 0x7}, // G
    {0x5, 0x5, 0x7, 0x5, 0x5}, // H
    {0x7, 0x2, 0x2, 0x2, 0x7}, // I
    {0x1, 0x1, 0x1, 0x5, 0x2}, // J
    {0x5, 0x5, 0x6, 0x5, 0x5}, // K
    {0x4, 0x4, 0x4, 0x4, 0x7}, // L
    {0x5, 0x7, 0x5, 0x5, 0x5}, // M
    {0x5, 0x7, 0x7, 0x5, 0x5}, // N
    {0x7, 0x5, 0x5, 0x5, 0x7}, // O
    {0x7, 0x5, 0x7, 0x4, 0x4}, // P
    {0x7, 0x5, 0x5, 0x6, 0x3}, // Q
    {0x7, 0x5, 0x7, 0x6, 0x5}, // R
    {0x3, 0x4, 0x2, 0x1, 0x6}, // S
    {0x7, 0x2, 0x2, 0x2, 0x2}, // T
    {0x5, 0x5, 0x5, 0x5, 0x7}, // U
    {0x5, 0x5, 0x5, 0x5, 0x2}, // V
    {0x5, 0x5, 0x5, 0x7, 0x5}, // W
    {0x5, 0x5, 0x2, 0x5, 0x5}, // X
    {0x5, 0x5, 0x2, 0x2, 0x2}, // Y
    {0x7, 0x1, 0x2, 0x4, 0x7}  // Z
};

static const uint8_t s_font_digits[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
    {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
    {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
    {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
    {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
    {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
    {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
    {0x7, 0x1, 0x2, 0x2, 0x2}, // 7
    {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
    {0x7, 0x5, 0x7, 0x1, 0x7}  // 9
};

void renderer_init(void) {
    lcdMainOnBottom();
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);

    tiles_init();

    renderer_clear(COLOR_DECK_FLOOR);
    renderer_present();
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
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    for (int j = y; j < y + h; j++) {
        uint16_t *row = &g_backbuffer[j * SCREEN_W + x];
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

void renderer_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        renderer_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void renderer_draw_circle(int cx, int cy, int radius, uint16_t color, int filled) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        if (filled) {
            for (int i = cx - x; i <= cx + x; i++) {
                renderer_draw_pixel(i, cy + y, color);
                renderer_draw_pixel(i, cy - y, color);
            }
            for (int i = cx - y; i <= cx + y; i++) {
                renderer_draw_pixel(i, cy + x, color);
                renderer_draw_pixel(i, cy - x, color);
            }
        } else {
            renderer_draw_pixel(cx + x, cy + y, color);
            renderer_draw_pixel(cx + y, cy + x, color);
            renderer_draw_pixel(cx - y, cy + x, color);
            renderer_draw_pixel(cx - x, cy + y, color);
            renderer_draw_pixel(cx - x, cy - y, color);
            renderer_draw_pixel(cx - y, cy - x, color);
            renderer_draw_pixel(cx + y, cy - x, color);
            renderer_draw_pixel(cx + x, cy - y, color);
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void renderer_draw_text(int x, int y, const char *str, uint16_t color) {
    while (*str) {
        char c = *str++;
        const uint8_t *glyph = NULL;
        uint8_t custom[5] = {0};

        if (c >= 'a' && c <= 'z') c -= 32;

        if (c >= 'A' && c <= 'Z') {
            glyph = s_font_letters[c - 'A'];
        } else if (c >= '0' && c <= '9') {
            glyph = s_font_digits[c - '0'];
        } else if (c == '[') {
            custom[0] = 0x6; custom[1] = 0x4; custom[2] = 0x4; custom[3] = 0x4; custom[4] = 0x6;
            glyph = custom;
        } else if (c == ']') {
            custom[0] = 0x3; custom[1] = 0x1; custom[2] = 0x1; custom[3] = 0x1; custom[4] = 0x3;
            glyph = custom;
        } else if (c == '+') {
            custom[0] = 0x0; custom[1] = 0x2; custom[2] = 0x7; custom[3] = 0x2; custom[4] = 0x0;
            glyph = custom;
        } else if (c == '-') {
            custom[0] = 0x0; custom[1] = 0x0; custom[2] = 0x7; custom[3] = 0x0; custom[4] = 0x0;
            glyph = custom;
        } else if (c == '>') {
            custom[0] = 0x4; custom[1] = 0x2; custom[2] = 0x1; custom[3] = 0x2; custom[4] = 0x4;
            glyph = custom;
        } else if (c == '<') {
            custom[0] = 0x1; custom[1] = 0x2; custom[2] = 0x4; custom[3] = 0x2; custom[4] = 0x1;
            glyph = custom;
        }

        if (glyph) {
            for (int r = 0; r < 5; r++) {
                uint8_t row = glyph[r];
                if (row & 0x4) renderer_draw_pixel(x, y + r, color);
                if (row & 0x2) renderer_draw_pixel(x + 1, y + r, color);
                if (row & 0x1) renderer_draw_pixel(x + 2, y + r, color);
            }
        }
        x += 4;
    }
}

void renderer_draw_trench_path(void) {
    tiles_render_map();

    // Adeptus Mechanicus Bunker HP Bar (over Bunker at tile (13,8))
    int hp_width = (g_game.core_hp * 26) / g_game.core_max_hp;
    if (hp_width < 0) hp_width = 0;
    renderer_fill_rect(211, 124, 26, 3, COLOR_HAZARD_BLACK);
    if (hp_width > 0) {
        uint16_t hp_col = (g_game.core_hp > 6) ? COLOR_LED_GREEN : COLOR_LED_RED;
        renderer_fill_rect(211, 124, hp_width, 3, hp_col);
    }
}

void renderer_draw_turret(const Turret *t, int is_selected, int show_cone) {
    if (!t->placed) return;

    // 1. Dotted Warning Arc (Sweep Cone)
    if (show_cone) {
        int r = t->range;
        int left_ang = (t->center_angle - t->sweep_amplitude) & 0xFF;
        int right_ang = (t->center_angle + t->sweep_amplitude) & 0xFF;

        int lx = t->x + ((fixed_cos(left_ang) * r) >> FP_SHIFT);
        int ly = t->y + ((fixed_sin(left_ang) * r) >> FP_SHIFT);
        int rx = t->x + ((fixed_cos(right_ang) * r) >> FP_SHIFT);
        int ry = t->y + ((fixed_sin(right_ang) * r) >> FP_SHIFT);

        renderer_draw_line(t->x, t->y, lx, ly, COLOR_CONE_LINE);
        renderer_draw_line(t->x, t->y, rx, ry, COLOR_CONE_LINE);

        for (int a = t->center_angle - t->sweep_amplitude; a <= t->center_angle + t->sweep_amplitude; a += 4) {
            int ax = t->x + ((fixed_cos(a & 0xFF) * r) >> FP_SHIFT);
            int ay = t->y + ((fixed_sin(a & 0xFF) * r) >> FP_SHIFT);
            renderer_draw_pixel(ax, ay, COLOR_CONE_DASH);
        }
    }

    // 2. Pixel-Art Shaded Turret Base with Rivets and Ammo Canister
    tiles_draw_turret_base(t->x, t->y, is_selected);

    // 3. Shaded Twin Heavy Bolter Barrels with Alternating Recoil and Muzzle Flash
    tiles_draw_twin_bolters(t->x, t->y, t->current_angle, t->flash_timer > 0,
                            t->barrel_recoil_l, t->barrel_recoil_r, t->last_barrel);
}

void renderer_draw_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;
        int ex = FROM_FP(g_enemies[i].x);
        int ey = FROM_FP(g_enemies[i].y);
        tiles_draw_xenos(ex, ey, g_enemies[i].dir, g_enemies[i].anim_frame);
    }
}

void renderer_draw_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) continue;
        int bx = FROM_FP(g_bullets[i].x);
        int by = FROM_FP(g_bullets[i].y);
        int bpx = FROM_FP(g_bullets[i].x - g_bullets[i].vx);
        int bpy = FROM_FP(g_bullets[i].y - g_bullets[i].vy);

        // Heavy Bolter tracer (Golden amber projectile)
        renderer_draw_line(bpx, bpy, bx, by, COLOR_BOLTER_TRACER);
        renderer_draw_pixel(bx, by, COLOR_WHITE);
    }
}

void renderer_draw_splatters(void) {
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life <= 0) continue;
        int sx = g_splatters[i].x;
        int sy = g_splatters[i].y;
        uint16_t col = g_splatters[i].color;

        renderer_draw_pixel(sx, sy, col);
        renderer_draw_pixel(sx + 1, sy, col);
        renderer_draw_pixel(sx, sy + 1, COLOR_BLOOD_DARK);
        if (g_splatters[i].life > 30) {
            renderer_draw_pixel(sx - 1, sy, col);
            renderer_draw_pixel(sx + 1, sy + 1, col);
        }
    }
}

void renderer_draw_ui_prep(void) {
    // 1. Top Command Panel (y: 0..17) - Stamped Iron with Hazard Trim
    renderer_fill_rect(0, 0, SCREEN_W, 18, COLOR_IRON_PANEL);
    renderer_draw_line(0, 18, SCREEN_W - 1, 18, COLOR_IRON_BORDER);

    // Hazard accent bar
    for (int x = 0; x < SCREEN_W; x += 4) {
        renderer_draw_pixel(x, 17, COLOR_HAZARD_YELLOW);
        renderer_draw_pixel(x + 1, 17, COLOR_HAZARD_YELLOW);
    }

    // [PURGE / START] Button: (x: 170..252, y: 2..15)
    renderer_fill_rect(170, 2, 82, 14, COLOR_HAZARD_BLACK);
    renderer_draw_rect(170, 2, 82, 14, COLOR_HAZARD_YELLOW);
    // Green activation LED
    renderer_fill_rect(173, 5, 4, 8, COLOR_LED_GREEN);
    renderer_draw_text(182, 6, "PURGE WAVE", COLOR_HAZARD_YELLOW);

    // Fast-Forward [2X] Button: (x: 110..164, y: 2..15)
    uint16_t ff_led = (g_game.fast_forward == 2) ? COLOR_AMBER : COLOR_DARK_GRAY;
    renderer_fill_rect(110, 2, 54, 14, COLOR_HAZARD_BLACK);
    renderer_draw_rect(110, 2, 54, 14, COLOR_IRON_LIGHT);
    renderer_fill_rect(113, 5, 4, 8, ff_led);
    renderer_draw_text(122, 6, "COG 2X", (g_game.fast_forward == 2) ? COLOR_AMBER : COLOR_IRON_LIGHT);

    // If turret selected, show [RECALL] button: (x: 48..104, y: 2..15)
    if (g_game.selected_turret >= 0 && g_turret.placed) {
        renderer_fill_rect(48, 2, 56, 14, COLOR_HAZARD_BLACK);
        renderer_draw_rect(48, 2, 56, 14, COLOR_RED);
        renderer_fill_rect(51, 5, 4, 8, COLOR_LED_RED);
        renderer_draw_text(60, 6, "RECALL", COLOR_WHITE);
    }

    // 2. Bottom Armory Dock (y: 168..191)
    renderer_fill_rect(0, 168, SCREEN_W, 24, COLOR_IRON_PANEL);
    renderer_draw_line(0, 168, SCREEN_W - 1, 168, COLOR_IRON_BORDER);

    // Dock slot icon for Heavy Bolter: (x: 8..80, y: 171..188)
    renderer_fill_rect(8, 171, 72, 17, COLOR_HAZARD_BLACK);
    renderer_draw_rect(8, 171, 72, 17, (g_game.turret_dock_count > 0) ? COLOR_BRASS : COLOR_DARK_GRAY);

    // Mini twin-barrel turret icon in slot
    renderer_draw_circle(20, 179, 4, COLOR_TURRET_RING, 1);
    renderer_draw_line(20, 178, 26, 178, COLOR_BARREL_STEEL);
    renderer_draw_line(20, 180, 26, 180, COLOR_BARREL_STEEL);

    char dock_label[16];
    sprintf(dock_label, "BOLTER [%d]", g_game.turret_dock_count);
    renderer_draw_text(30, 177, dock_label, (g_game.turret_dock_count > 0) ? COLOR_HAZARD_YELLOW : COLOR_IRON_LIGHT);

    // Dragging ghost preview
    if (g_game.is_dragging_new) {
        int gx = g_game.drag_x;
        int gy = g_game.drag_y;

        // Validity check
        int valid = (gy >= 24 && gy <= 158 && gx >= 12 && gx <= 244);
        for (int i = 0; i < MAX_WAYPOINTS - 1; i++) {
            int x0 = g_waypoints[i].x, y0 = g_waypoints[i].y;
            int x1 = g_waypoints[i + 1].x, y1 = g_waypoints[i + 1].y;
            if (x0 == x1) {
                int minY = (y0 < y1) ? y0 : y1, maxY = (y0 < y1) ? y1 : y0;
                if (gy >= minY - 14 && gy <= maxY + 14 && abs(gx - x0) <= 16) valid = 0;
            } else {
                int minX = (x0 < x1) ? x0 : x1, maxX = (x0 < x1) ? x1 : x0;
                if (gx >= minX - 14 && gx <= maxX + 14 && abs(gy - y0) <= 16) valid = 0;
            }
        }

        uint16_t ghost_col = valid ? COLOR_HAZARD_YELLOW : COLOR_LED_RED;
        renderer_draw_circle(gx, gy, 7, ghost_col, 0);
        renderer_draw_circle(gx, gy, 45, ghost_col, 0);
    }
}

void renderer_draw_ui_workshop(void) {
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_DECK_FLOOR);

    // Title banner: REQUISITION TERMINAL
    renderer_fill_rect(10, 8, 236, 20, COLOR_IRON_PANEL);
    renderer_draw_rect(10, 8, 236, 20, COLOR_BRASS);
    renderer_draw_text(38, 14, "++ OMNISSIAH ARMORY WORKSHOP ++", COLOR_BRASS);

    // Card 1: Sacred Firerate
    renderer_fill_rect(12, 34, 232, 28, COLOR_HAZARD_BLACK);
    renderer_draw_rect(12, 34, 232, 28, COLOR_IRON_LIGHT);
    renderer_draw_text(18, 40, "BOLTER FIRERATE RITE", COLOR_WHITE);
    char buf1[32];
    sprintf(buf1, "LVL %d", g_game.upgrades.firerate_lvl);
    renderer_draw_text(18, 50, buf1, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(175, 38, 64, 20, COLOR_BRASS);
    renderer_draw_rect(175, 38, 64, 20, COLOR_WHITE);
    renderer_draw_text(182, 44, "SANCTIFY", COLOR_BLACK);

    // Card 2: Traverse Servos
    renderer_fill_rect(12, 66, 232, 28, COLOR_HAZARD_BLACK);
    renderer_draw_rect(12, 66, 232, 28, COLOR_IRON_LIGHT);
    renderer_draw_text(18, 72, "TRAVERSE MOTOR SERVOS", COLOR_WHITE);
    char buf2[32];
    sprintf(buf2, "LVL %d", g_game.upgrades.sweep_lvl);
    renderer_draw_text(18, 82, buf2, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(175, 70, 64, 20, COLOR_BRASS);
    renderer_draw_rect(175, 70, 64, 20, COLOR_WHITE);
    renderer_draw_text(182, 76, "SANCTIFY", COLOR_BLACK);

    // Card 3: Tithe / Scrap Reclamation
    renderer_fill_rect(12, 98, 232, 28, COLOR_HAZARD_BLACK);
    renderer_draw_rect(12, 98, 232, 28, COLOR_IRON_LIGHT);
    renderer_draw_text(18, 104, "TITHE RECLAMATION PROTOCOL", COLOR_WHITE);
    char buf3[32];
    sprintf(buf3, "LVL %d", g_game.upgrades.scrap_lvl);
    renderer_draw_text(18, 114, buf3, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(175, 102, 64, 20, COLOR_BRASS);
    renderer_draw_rect(175, 102, 64, 20, COLOR_WHITE);
    renderer_draw_text(182, 108, "SANCTIFY", COLOR_BLACK);

    // Bottom Action: [RETURN TO DEFENSE BASTION]
    renderer_fill_rect(24, 138, 208, 34, COLOR_HAZARD_BLACK);
    renderer_draw_rect(24, 138, 208, 34, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(28, 142, 200, 26, COLOR_IRON_PANEL);
    renderer_draw_text(40, 151, ">> ENGAGE NEXT XENOS WAVE <<", COLOR_HAZARD_YELLOW);
}

void renderer_present(void) {
    swiWaitForVBlank();
    dmaCopyWords(3, g_backbuffer, VRAM_A, sizeof(g_backbuffer));
}
