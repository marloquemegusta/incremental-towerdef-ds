#include "game.h"
#include "tiles.h"
#include "turret_data.h"

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
        } else if (c == ':') {
            custom[0] = 0x0; custom[1] = 0x2; custom[2] = 0x0; custom[3] = 0x2; custom[4] = 0x0;
            glyph = custom;
        } else if (c == '.') {
            custom[0] = 0x0; custom[1] = 0x0; custom[2] = 0x0; custom[3] = 0x0; custom[4] = 0x2;
            glyph = custom;
        } else if (c == '/') {
            custom[0] = 0x1; custom[1] = 0x2; custom[2] = 0x2; custom[3] = 0x4; custom[4] = 0x4;
            glyph = custom;
        } else if (c == '$') {
            custom[0] = 0x2; custom[1] = 0x7; custom[2] = 0x6; custom[3] = 0x7; custom[4] = 0x2;
            glyph = custom;
        } else if (c == '%') {
            custom[0] = 0x5; custom[1] = 0x1; custom[2] = 0x2; custom[3] = 0x4; custom[4] = 0x5;
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
    tiles_render_sector1_map();
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

    // 2. Pixel-art canonical 32-angle discrete sprite (RotSprite)
    turret_draw_angle(t->x, t->y, t->type, t->current_angle, is_selected);
}

void renderer_draw_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;
        int ex = FROM_FP(g_enemies[i].x);
        int ey = FROM_FP(g_enemies[i].y);
        tiles_draw_xenos(ex, ey, g_enemies[i].dir, g_enemies[i].anim_frame, g_enemies[i].variant);
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
        int sz = g_splatters[i].size;
        int life = g_splatters[i].life;

        // Visual degradation / shrinking as puddle dries and seeps into iron deck
        if (life < 30) {
            // Fading single dot
            renderer_draw_pixel(sx, sy, COLOR_BLOOD_DARK);
            continue;
        }

        if (sz == 0) {
            // Small droplet (1-2 pixels)
            renderer_draw_pixel(sx, sy, col);
            if (life > 60) {
                renderer_draw_pixel(sx + 1, sy, COLOR_BLOOD_DARK);
            }
        } else if (sz == 1) {
            // Medium splatter (2x2 cluster + shadow rim)
            renderer_draw_pixel(sx, sy, col);
            renderer_draw_pixel(sx + 1, sy, col);
            renderer_draw_pixel(sx, sy + 1, COLOR_BLOOD_DARK);
            renderer_draw_pixel(sx + 1, sy + 1, COLOR_BLOOD_DARK);
            if (life > 90) {
                renderer_draw_pixel(sx - 1, sy, col);
                renderer_draw_pixel(sx, sy - 1, col);
            }
        } else {
            // Large bio-pool / gore chunk (3x3 organic blob)
            renderer_draw_pixel(sx, sy, col);
            renderer_draw_pixel(sx + 1, sy, col);
            renderer_draw_pixel(sx - 1, sy, col);
            renderer_draw_pixel(sx, sy - 1, col);
            renderer_draw_pixel(sx, sy + 1, COLOR_BLOOD_DARK);
            renderer_draw_pixel(sx + 1, sy + 1, COLOR_BLOOD_DARK);
            renderer_draw_pixel(sx - 1, sy + 1, COLOR_BLOOD_DARK);
            if (life > 80) {
                renderer_draw_pixel(sx + 2, sy, col);
                renderer_draw_pixel(sx, sy + 2, COLOR_BLOOD_DARK);
            }
        }
    }
}

void renderer_draw_death_particles(void) {
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;

        int ground_x = FROM_FP(g_death_particles[i].x);
        int ground_y = FROM_FP(g_death_particles[i].y);
        int height_z = FROM_FP(g_death_particles[i].z);

        if (ground_x < 0 || ground_x >= SCREEN_W || ground_y < 0 || ground_y >= SCREEN_H) continue;

        // 1. Draw pseudo-3D ground shadow underneath airborne particle
        if (height_z > 2 && ground_y + 1 < SCREEN_H) {
            renderer_draw_pixel(ground_x, ground_y, COLOR_HAZARD_BLACK);
        }

        // 2. Projected airborne position
        int air_y = ground_y - height_z;
        if (air_y >= 0 && air_y < SCREEN_H) {
            uint16_t col = g_death_particles[i].color;
            if (g_death_particles[i].size == 0) {
                // 1x1 fast projectile drop
                renderer_draw_pixel(ground_x, air_y, col);
            } else {
                // 2x2 heavy organ/carapace chunk
                renderer_draw_pixel(ground_x, air_y, col);
                if (ground_x + 1 < SCREEN_W) {
                    renderer_draw_pixel(ground_x + 1, air_y, col);
                    renderer_draw_pixel(ground_x + 1, air_y + 1, COLOR_BLOOD_DARK);
                }
                if (air_y + 1 < SCREEN_H) {
                    renderer_draw_pixel(ground_x, air_y + 1, COLOR_BLOOD_DARK);
                }
            }
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

    // Sanctuary Core Hull Integrity Bar: (x: 6..54, y: 3..14)
    renderer_draw_text(6, 3, "HULL", COLOR_WHITE);
    renderer_fill_rect(24, 3, 26, 6, COLOR_HAZARD_BLACK);
    renderer_draw_rect(24, 3, 26, 6, COLOR_IRON_BORDER);
    int hp_width = (g_game.core_hp * 24) / g_game.core_max_hp;
    if (hp_width < 0) hp_width = 0;
    if (hp_width > 24) hp_width = 24;
    if (hp_width > 0) {
        uint16_t hp_col = (g_game.core_hp > 6) ? COLOR_LED_GREEN : COLOR_LED_RED;
        renderer_fill_rect(25, 4, hp_width, 4, hp_col);
    }
    char hp_txt[8];
    sprintf(hp_txt, "%d", g_game.core_hp);
    renderer_draw_text(6, 10, hp_txt, (g_game.core_hp > 6) ? COLOR_LED_GREEN : COLOR_LED_RED);

    // If turret selected and in prep, show [RECALL] button: (x: 54..98, y: 2..15)
    if (g_game.mode == MODE_PREPARATION && g_game.selected_turret >= 0 && g_game.selected_turret < MAX_TURRETS && g_turrets[g_game.selected_turret].placed) {
        renderer_fill_rect(54, 2, 44, 14, COLOR_HAZARD_BLACK);
        renderer_draw_rect(54, 2, 44, 14, COLOR_RED);
        renderer_fill_rect(56, 5, 3, 8, COLOR_LED_RED);
        renderer_draw_text(62, 6, "RECALL", COLOR_WHITE);
    }

    // Fast-Forward [2X] Button: (x: 102..150, y: 2..15)
    uint16_t ff_led = (g_game.fast_forward == 2) ? COLOR_AMBER : COLOR_DARK_GRAY;
    renderer_fill_rect(102, 2, 48, 14, COLOR_HAZARD_BLACK);
    renderer_draw_rect(102, 2, 48, 14, COLOR_IRON_LIGHT);
    renderer_fill_rect(105, 5, 4, 8, ff_led);
    renderer_draw_text(113, 6, "COG 2X", (g_game.fast_forward == 2) ? COLOR_AMBER : COLOR_IRON_LIGHT);

    // [PAUSE] Button: (x: 154..190, y: 2..15)
    renderer_fill_rect(154, 2, 36, 14, COLOR_HAZARD_BLACK);
    renderer_draw_rect(154, 2, 36, 14, COLOR_IRON_LIGHT);
    renderer_fill_rect(157, 5, 3, 8, (g_game.mode == MODE_PAUSED) ? COLOR_AMBER : COLOR_DARK_GRAY);
    renderer_draw_text(163, 6, "PAUS", (g_game.mode == MODE_PAUSED) ? COLOR_AMBER : COLOR_WHITE);

    // [PURGE / START] Button: (x: 194..252, y: 2..15)
    if (g_game.mode == MODE_PREPARATION) {
        renderer_fill_rect(194, 2, 58, 14, COLOR_HAZARD_BLACK);
        renderer_draw_rect(194, 2, 58, 14, COLOR_HAZARD_YELLOW);
        renderer_fill_rect(197, 5, 3, 8, COLOR_LED_GREEN);
        renderer_draw_text(204, 6, "PURGA", COLOR_HAZARD_YELLOW);
    } else {
        // In wave mode, indicate purging wave
        renderer_fill_rect(194, 2, 58, 14, COLOR_HAZARD_BLACK);
        renderer_draw_rect(194, 2, 58, 14, COLOR_LED_RED);
        renderer_fill_rect(197, 5, 3, 8, COLOR_LED_RED);
        renderer_draw_text(204, 6, "ACTIVA", COLOR_WHITE);
    }

    // 2. Bottom Armory Dock (y: 168..191) - only in prep mode
    if (g_game.mode == MODE_PREPARATION) {
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

        // Dock button for CALIBRAR (x: 84..160, y: 171..188)
        renderer_fill_rect(84, 171, 76, 17, COLOR_HAZARD_BLACK);
        renderer_draw_rect(84, 171, 76, 17, COLOR_AMBER);
        renderer_fill_rect(87, 174, 4, 11, COLOR_HAZARD_YELLOW);
        renderer_draw_text(94, 177, "CALIBRAR", COLOR_AMBER);

        // Dock button for FORGE STC (Branching Skill Tree): (x: 164..248, y: 171..188)
        renderer_fill_rect(164, 171, 84, 17, COLOR_HAZARD_BLACK);
        renderer_draw_rect(164, 171, 84, 17, COLOR_BRASS);
        renderer_fill_rect(167, 174, 4, 11, COLOR_AMBER);
        renderer_draw_text(176, 177, "FORGE STC", COLOR_BRASS);

        // Dragging ghost preview
        if (g_game.is_dragging_new) {
            int gx = g_game.drag_x;
            int gy = g_game.drag_y;

            int valid = game_is_pos_valid(gx, gy);
            uint16_t ghost_col = valid ? COLOR_HAZARD_YELLOW : COLOR_LED_RED;
            renderer_draw_circle(gx, gy, 7, ghost_col, 0);
            renderer_draw_circle(gx, gy, 45, ghost_col, 0);
        }
    } else if (g_game.mode == MODE_WAVE || g_game.mode == MODE_PAUSED) {
        // Bottom combat strip: wave info and instructions
        renderer_fill_rect(0, 174, SCREEN_W, 18, COLOR_IRON_PANEL);
        renderer_draw_line(0, 174, SCREEN_W - 1, 174, COLOR_IRON_BORDER);
        if (g_game.selected_turret >= 0 && g_game.selected_turret < MAX_TURRETS && g_turrets[g_game.selected_turret].placed) {
            char tinfo[48];
            Turret *st = &g_turrets[g_game.selected_turret];
            int deg = (st->center_angle * 360) / 256;
            int cdeg = (st->sweep_amplitude * 360) / 256;
            snprintf(tinfo, sizeof(tinfo), "BATERIA #%d SEL | EJE:%03d | CONO:+-%02d", g_game.selected_turret + 1, deg, cdeg);
            renderer_draw_text(8, 179, tinfo, COLOR_HAZARD_YELLOW);
        } else {
            renderer_draw_text(8, 179, "TOCA TORRETA PARA SELECCIONAR / INSPECCIONAR", COLOR_IRON_LIGHT);
        }
    }
}

void renderer_draw_ui_pause(void) {
    // Semi-transparent / dithered or solid modal frame in center
    int mx = 24, my = 50, mw = 208, mh = 90;
    renderer_fill_rect(mx, my, mw, mh, COLOR_HAZARD_BLACK);
    renderer_draw_rect(mx, my, mw, mh, COLOR_BRASS);
    renderer_draw_rect(mx + 2, my + 2, mw - 4, mh - 4, COLOR_IRON_BORDER);

    // Hazard header in pause box
    renderer_fill_rect(mx + 3, my + 3, mw - 6, 14, COLOR_IRON_PANEL);
    renderer_draw_text(mx + 26, my + 6, "++ LITURGIA EN SUSPENSION ++", COLOR_AMBER);

    renderer_draw_text(mx + 20, my + 26, "SIMULACION BALISTICA PAUSADA", COLOR_WHITE);
    renderer_draw_text(mx + 16, my + 38, "AUSPEX COGITATOR EN ESPERA", COLOR_IRON_LIGHT);

    // Resume button
    renderer_fill_rect(mx + 24, my + 54, 160, 22, COLOR_IRON_PANEL);
    renderer_draw_rect(mx + 24, my + 54, 160, 22, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(mx + 28, my + 59, 4, 12, COLOR_LED_GREEN);
    renderer_draw_text(mx + 40, my + 61, "PULSA [START] O TOCA AQUI", COLOR_HAZARD_YELLOW);
}

void renderer_draw_ui_game_over(void) {
    // Game over modal overlay
    int mx = 20, my = 40, mw = 216, mh = 112;
    renderer_fill_rect(mx, my, mw, mh, COLOR_BLACK);
    renderer_draw_rect(mx, my, mw, mh, COLOR_RED);
    renderer_draw_rect(mx + 2, my + 2, mw - 4, mh - 4, COLOR_HAZARD_BLACK);

    renderer_fill_rect(mx + 3, my + 3, mw - 6, 16, COLOR_RED);
    renderer_draw_text(mx + 24, my + 7, "++ BRECHA FATAL EN EL SANCTUM ++", COLOR_WHITE);

    char buf[48];
    snprintf(buf, sizeof(buf), "XENOS PURGADOS : %d BAJAS", g_game.enemies_killed);
    renderer_draw_text(mx + 16, my + 30, buf, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "ENEMIGOS INFILTRADOS: %d", g_game.enemies_breached);
    renderer_draw_text(mx + 16, my + 44, buf, COLOR_LED_RED);

    snprintf(buf, sizeof(buf), "DIEZMO RECOLECTADO : %d SC", g_game.scrap);
    renderer_draw_text(mx + 16, my + 58, buf, COLOR_HAZARD_YELLOW);

    // Retry / Rebuild button
    renderer_fill_rect(mx + 20, my + 76, 176, 24, COLOR_IRON_PANEL);
    renderer_draw_rect(mx + 20, my + 76, 176, 24, COLOR_BRASS);
    renderer_fill_rect(mx + 24, my + 82, 4, 12, COLOR_LED_GREEN);
    renderer_draw_text(mx + 36, my + 84, "RESTAURAR SANCTUM [TOCAR/A]", COLOR_WHITE);
}

void renderer_draw_ui_workshop(void) {
    skills_draw_tree();
}

void renderer_draw_ui_calibration(void) {
    renderer_clear(COLOR_DECK_FLOOR);

    // Header (y: 0..17)
    renderer_fill_rect(0, 0, SCREEN_W, 18, COLOR_IRON_PANEL);
    renderer_draw_line(0, 18, SCREEN_W - 1, 18, COLOR_IRON_BORDER);
    for (int x = 0; x < SCREEN_W; x += 4) {
        renderer_draw_pixel(x, 17, COLOR_HAZARD_YELLOW);
        renderer_draw_pixel(x + 1, 17, COLOR_HAZARD_YELLOW);
    }
    renderer_draw_text(6, 6, "TUNING DECK - CALIBRACION M1", COLOR_HAZARD_YELLOW);

    // [VOLVER] button (x: 202..252, y: 2..15)
    renderer_fill_rect(202, 2, 50, 14, COLOR_HAZARD_BLACK);
    renderer_draw_rect(202, 2, 50, 14, COLOR_IRON_LIGHT);
    renderer_draw_text(208, 6, "VOLVER", COLOR_WHITE);

    static const char *labels[CALIBRATION_ROWS] = {
        "ACTIVE MAP",
        "ENEMY COUNT",
        "ENEMY HP",
        "ENEMY SPEED",
        "T0 LARVA DELAY",
        "T1 RIPPER DELAY",
        "T2 GAUNT DELAY",
        "T3 RAVENER DELAY",
        "T4 CARNIFEX DLY",
        "T5 TITAN DELAY",
        "EXPLOSION FORCE",
        "TURRET DAMAGE",
        "TURRET CADENCE",
        "CONE SPREAD",
        "SWEEP SPEED",
        "TURRET RANGE",
        "STARTING SCRAP",
        "BASE LIVES"
    };

    static const char *units[CALIBRATION_ROWS] = {
        "M1",
        "XENOS",
        "HP",
        "PX/F",
        "FRAMES",
        "FRAMES",
        "FRAMES",
        "FRAMES",
        "FRAMES",
        "FRAMES",
        "X",
        "DMG",
        "FRAMES",
        "DEG",
        "DEG/F",
        "PX",
        "$",
        "HP"
    };

    // Calculate scroll offset to keep selected_row in view
    // 12 visible rows on screen (y = 22 to 154)
    int scroll_top = g_calibration.selected_row - 6;
    if (scroll_top < 0) scroll_top = 0;
    if (scroll_top > CALIBRATION_ROWS - 12) scroll_top = CALIBRATION_ROWS - 12;

    char val_buf[16];
    for (int v = 0; v < 12; v++) {
        int i = scroll_top + v;
        int y = 22 + v * 11;
        int is_sel = (g_calibration.selected_row == i);

        if (is_sel) {
            renderer_fill_rect(4, y - 1, 240, 10, COLOR_IRON_BORDER);
            renderer_draw_text(6, y + 1, ">", COLOR_HAZARD_YELLOW);
        }

        uint16_t txt_col = is_sel ? COLOR_WHITE : COLOR_IRON_LIGHT;
        renderer_draw_text(14, y + 1, labels[i], txt_col);

        switch (i) {
            case 0:
                if (g_calibration.selected_map == 0) sprintf(val_buf, "TRINCHERA");
                else if (g_calibration.selected_map == 1) sprintf(val_buf, "DOBLE S");
                else sprintf(val_buf, "ROTONDA");
                break;
            case 1:
                if (g_calibration.enemy_count == 0) sprintf(val_buf, "INF");
                else sprintf(val_buf, "%d", g_calibration.enemy_count);
                break;
            case 2: sprintf(val_buf, "%d", g_calibration.enemy_hp); break;
            case 3: sprintf(val_buf, "%d.%d", g_calibration.enemy_speed_int / 10, g_calibration.enemy_speed_int % 10); break;

            // Per-variant spawn delays (0 = OFF)
            case 4: case 5: case 6: case 7: case 8: case 9:
                {
                    int var_idx = i - 4;
                    int dly = g_calibration.spawn_delay[var_idx];
                    if (dly == 0) sprintf(val_buf, "OFF");
                    else sprintf(val_buf, "%d", dly);
                }
                break;

            case 10: sprintf(val_buf, "%d", g_calibration.explosion_force); break;
            case 11: sprintf(val_buf, "%d", g_calibration.turret_damage); break;
            case 12: sprintf(val_buf, "%d", g_calibration.turret_fire_rate); break;
            case 13: sprintf(val_buf, "%d", g_calibration.cone_spread); break;
            case 14: sprintf(val_buf, "%d", g_calibration.sweep_speed); break;
            case 15: sprintf(val_buf, "%d", g_calibration.turret_range); break;
            case 16: sprintf(val_buf, "%d", g_calibration.starting_scrap); break;
            case 17: sprintf(val_buf, "%d", g_calibration.core_lives); break;
            default: sprintf(val_buf, "0"); break;
        }

        // [-] touch button
        renderer_fill_rect(108, y, 10, 8, COLOR_HAZARD_BLACK);
        renderer_draw_rect(108, y, 10, 8, is_sel ? COLOR_AMBER : COLOR_DARK_GRAY);
        renderer_draw_text(111, y + 1, "-", COLOR_WHITE);

        // Value text
        renderer_draw_text(122, y + 1, val_buf, is_sel ? COLOR_HAZARD_YELLOW : COLOR_WHITE);

        // [+] touch button
        renderer_fill_rect(190, y, 10, 8, COLOR_HAZARD_BLACK);
        renderer_draw_rect(190, y, 10, 8, is_sel ? COLOR_AMBER : COLOR_DARK_GRAY);
        renderer_draw_text(193, y + 1, "+", COLOR_WHITE);

        // Unit
        renderer_draw_text(204, y + 1, units[i], COLOR_DARK_GRAY);
    }

    // Vertical Scroll Bar Indicator (x: 248, y: 22..154)
    renderer_fill_rect(248, 22, 3, 132, COLOR_HAZARD_BLACK);
    renderer_draw_rect(248, 22, 3, 132, COLOR_IRON_BORDER);
    int thumb_h = (12 * 132) / CALIBRATION_ROWS;
    int thumb_y = 22 + (scroll_top * (132 - thumb_h)) / (CALIBRATION_ROWS - 12);
    renderer_fill_rect(248, thumb_y, 3, thumb_h, COLOR_HAZARD_YELLOW);

    // Bottom Controls Bar (y: 158..191)
    renderer_fill_rect(0, 158, SCREEN_W, 34, COLOR_IRON_PANEL);
    renderer_draw_line(0, 158, SCREEN_W - 1, 158, COLOR_IRON_BORDER);

    renderer_draw_text(6, 160, "CRUCETA: +/-1  L/R: +/-5  Y: RESET", COLOR_IRON_LIGHT);

    // [DEFAULT (Y)] Button
    renderer_fill_rect(6, 170, 76, 19, COLOR_HAZARD_BLACK);
    renderer_draw_rect(6, 170, 76, 19, COLOR_IRON_LIGHT);
    renderer_draw_text(10, 176, "RESET [Y]", COLOR_WHITE);

    // [PROBAR PARTIDA (A)] Button
    renderer_fill_rect(86, 170, 164, 19, COLOR_HAZARD_BLACK);
    renderer_draw_rect(86, 170, 164, 19, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(90, 174, 6, 11, COLOR_LED_GREEN);
    renderer_draw_text(102, 176, "PROBAR PARTIDA [A]", COLOR_HAZARD_YELLOW);
}

void renderer_present(void) {
    swiWaitForVBlank();
    dmaCopyWords(3, g_backbuffer, VRAM_A, sizeof(g_backbuffer));
}
