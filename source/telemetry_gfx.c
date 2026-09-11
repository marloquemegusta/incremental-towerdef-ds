#include "telemetry_gfx.h"
#include "game.h"
#include "skills.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Top screen backbuffer (16-bit direct color, 256x192)
uint16_t g_top_backbuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));

static u16 *s_top_vram = NULL;
static int s_top_bg = 0;
static int s_active_tab = TOP_TAB_BATTERIES;
static int s_selected_turret = 0;
static int s_detail_mode = 0;

// Compact 3x5 font for UI
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

// Primitive Drawing Routines for Top Screen
static void top_clear(uint16_t color) {
    uint32_t color32 = ((uint32_t)color << 16) | color;
    uint32_t *dest = (uint32_t *)g_top_backbuffer;
    int count = (SCREEN_W * SCREEN_H) / 2;
    for (int i = 0; i < count; i++) {
        dest[i] = color32;
    }
}

static inline void top_draw_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        g_top_backbuffer[y * SCREEN_W + x] = color;
    }
}

static void top_draw_rect(int x, int y, int w, int h, uint16_t color) {
    for (int i = x; i < x + w; i++) {
        top_draw_pixel(i, y, color);
        top_draw_pixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        top_draw_pixel(x, j, color);
        top_draw_pixel(x + w - 1, j, color);
    }
}

static void top_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    for (int j = y; j < y + h; j++) {
        uint16_t *row = &g_top_backbuffer[j * SCREEN_W + x];
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

static void top_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        top_draw_pixel(x0, y0, color);
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

static void top_draw_circle(int cx, int cy, int radius, uint16_t color, int filled) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        if (filled) {
            for (int i = cx - x; i <= cx + x; i++) {
                top_draw_pixel(i, cy + y, color);
                top_draw_pixel(i, cy - y, color);
            }
            for (int i = cx - y; i <= cx + y; i++) {
                top_draw_pixel(i, cy + x, color);
                top_draw_pixel(i, cy - x, color);
            }
        } else {
            top_draw_pixel(cx + x, cy + y, color);
            top_draw_pixel(cx + y, cy + x, color);
            top_draw_pixel(cx - y, cy + x, color);
            top_draw_pixel(cx - x, cy + y, color);
            top_draw_pixel(cx - x, cy - y, color);
            top_draw_pixel(cx - y, cy - x, color);
            top_draw_pixel(cx + y, cy - x, color);
            top_draw_pixel(cx + x, cy - y, color);
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

static void top_draw_text(int x, int y, const char *str, uint16_t color) {
    while (*str) {
        char c = *str++;
        const uint8_t *glyph = NULL;
        uint8_t custom[5] = {0};

        if (c >= 'a' && c <= 'z') c -= 32;

        if (c >= 'A' && c <= 'Z') {
            glyph = s_font_letters[c - 'A'];
        } else if (c >= '0' && c <= '9') {
            glyph = s_font_digits[c - '0'];
        } else if (c == '-') {
            custom[2] = 0x7; glyph = custom;
        } else if (c == '+') {
            custom[1] = 0x2; custom[2] = 0x7; custom[3] = 0x2; glyph = custom;
        } else if (c == ':') {
            custom[1] = 0x2; custom[3] = 0x2; glyph = custom;
        } else if (c == '.') {
            custom[4] = 0x2; glyph = custom;
        } else if (c == '/') {
            custom[0] = 0x1; custom[1] = 0x1; custom[2] = 0x2; custom[3] = 0x4; custom[4] = 0x4; glyph = custom;
        } else if (c == '%') {
            custom[0] = 0x5; custom[1] = 0x1; custom[2] = 0x2; custom[3] = 0x4; custom[4] = 0x5; glyph = custom;
        } else if (c == '[') {
            custom[0] = 0x6; custom[1] = 0x4; custom[2] = 0x4; custom[3] = 0x4; custom[4] = 0x6; glyph = custom;
        } else if (c == ']') {
            custom[0] = 0x3; custom[1] = 0x1; custom[2] = 0x1; custom[3] = 0x1; custom[4] = 0x3; glyph = custom;
        } else if (c == '>') {
            custom[0] = 0x4; custom[1] = 0x2; custom[2] = 0x1; custom[3] = 0x2; custom[4] = 0x4; glyph = custom;
        } else if (c == '<') {
            custom[0] = 0x1; custom[1] = 0x2; custom[2] = 0x4; custom[3] = 0x2; custom[4] = 0x1; glyph = custom;
        } else if (c == '#') {
            custom[0] = 0x5; custom[1] = 0x7; custom[2] = 0x5; custom[3] = 0x7; custom[4] = 0x5; glyph = custom;
        } else if (c == '|') {
            custom[0] = 0x2; custom[1] = 0x2; custom[2] = 0x2; custom[3] = 0x2; custom[4] = 0x2; glyph = custom;
        } else if (c == '=') {
            custom[1] = 0x7; custom[3] = 0x7; glyph = custom;
        }

        if (glyph) {
            for (int r = 0; r < 5; r++) {
                uint8_t row = glyph[r];
                for (int col = 0; col < 3; col++) {
                    if (row & (1 << (2 - col))) {
                        top_draw_pixel(x + col, y + r, color);
                    }
                }
            }
        }
        x += 4;
    }
}

static void top_draw_gauge(int x, int y, int w, int h, int val, int max_val, uint16_t fg_col, uint16_t bg_col) {
    top_fill_rect(x, y, w, h, bg_col);
    top_draw_rect(x, y, w, h, COLOR_IRON_BORDER);
    if (max_val > 0 && val > 0) {
        int fill_w = (val * (w - 2)) / max_val;
        if (fill_w > w - 2) fill_w = w - 2;
        if (fill_w > 0) {
            top_fill_rect(x + 1, y + 1, fill_w, h - 2, fg_col);
        }
    }
}

// Public API
void telemetry_gfx_init(void) {
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    s_top_bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    s_top_vram = bgGetGfxPtr(s_top_bg);

    s_active_tab = TOP_TAB_BATTERIES;
    s_selected_turret = 0;
    s_detail_mode = 0;

    top_clear(COLOR_DECK_FLOOR);
    telemetry_gfx_present();
}

int telemetry_gfx_get_tab(void) { return s_active_tab; }
int telemetry_gfx_get_selected_turret(void) { return s_selected_turret; }
int telemetry_gfx_is_detail_mode(void) { return s_detail_mode; }

void telemetry_gfx_update(int keys_down, int keys_held) {
    if (g_game.mode == MODE_CALIBRATION) return;

    // Synchronize s_selected_turret from g_game.selected_turret (touch input writes there)
    // This covers the case where the touch screen selected a turret last frame.
    s_selected_turret = g_game.selected_turret;

    // 1. Tab Switching with L / R
    if (keys_down & KEY_L) {
        s_active_tab = (s_active_tab + 2) % 3;
        s_detail_mode = 0;
    }
    if (keys_down & KEY_R) {
        s_active_tab = (s_active_tab + 1) % 3;
        s_detail_mode = 0;
    }

    // 2. Battery Tab Interaction
    if (s_active_tab == TOP_TAB_BATTERIES) {
        // Toggle detail view
        if (keys_down & KEY_A) {
            s_detail_mode = 1;
        }
        if (keys_down & KEY_B) {
            s_detail_mode = 0;
        }

        // D-Pad Up / Down: Select turret — write directly to g_game.selected_turret
        if (keys_down & KEY_UP) {
            int cur = (s_selected_turret >= 0 && s_selected_turret < MAX_TURRETS) ? s_selected_turret : 0;
            s_selected_turret = (cur + MAX_TURRETS - 1) % MAX_TURRETS;
            g_game.selected_turret = s_selected_turret;
        }
        if (keys_down & KEY_DOWN) {
            int cur = (s_selected_turret >= 0 && s_selected_turret < MAX_TURRETS) ? s_selected_turret : 0;
            s_selected_turret = (cur + 1) % MAX_TURRETS;
            g_game.selected_turret = s_selected_turret;
        }
    }

    // 3. Live Calibration on selected placed turret (available anytime in prep, wave, or pause)
    if (s_selected_turret >= 0 && s_selected_turret < MAX_TURRETS && g_turrets[s_selected_turret].placed) {
        Turret *t = &g_turrets[s_selected_turret];

        // D-Pad Left / Right: Rotate central axis
        if ((keys_down & KEY_LEFT) || ((keys_held & KEY_LEFT) && (g_game.sim_ticks_elapsed % 4 == 0))) {
            t->center_angle = (t->center_angle - 4) & 0xFF;
        }
        if ((keys_down & KEY_RIGHT) || ((keys_held & KEY_RIGHT) && (g_game.sim_ticks_elapsed % 4 == 0))) {
            t->center_angle = (t->center_angle + 4) & 0xFF;
        }

        // X / Y: Narrow or Widen sweep cone (on selected turret!)
        if (keys_down & KEY_X) {
            if (t->sweep_amplitude > 4) t->sweep_amplitude -= 2;
        }
        if (keys_down & KEY_Y) {
            if (t->sweep_amplitude < 48) t->sweep_amplitude += 2;
        }
    }
}

static void render_header(void) {
    // Header Bar: Y: 0..20
    top_fill_rect(0, 0, SCREEN_W, 21, COLOR_IRON_PANEL);
    top_draw_rect(0, 0, SCREEN_W, 21, COLOR_BRASS);

    top_draw_text(6, 4, "AUSPEX COGITATOR M41", COLOR_AMBER);
    char sec_buf[32];
    sprintf(sec_buf, "WAVE %02d/09", g_game.wave_number);
    top_draw_text(110, 4, sec_buf, COLOR_WHITE);

    // Sanctum Hull Bar
    top_draw_text(6, 12, "HULL", COLOR_IRON_LIGHT);
    uint16_t hp_col = (g_game.core_hp > 5) ? COLOR_PHOSPHOR_GREEN : COLOR_LED_RED;
    top_draw_gauge(26, 12, 50, 6, g_game.core_hp, g_game.core_max_hp, hp_col, COLOR_BLACK);

    // Tithe Scrap
    char scrap_buf[24];
    sprintf(scrap_buf, "TITHE: %d SC", g_game.scrap);
    top_draw_text(86, 12, scrap_buf, COLOR_HAZARD_YELLOW);

    // Status mode
    const char *st = (g_game.core_hp <= 0) ? "BREACH" :
                     (g_game.mode == MODE_PAUSED) ? "PAUSE" :
                     (g_game.mode == MODE_WAVE) ? "PURGE" :
                     (g_game.mode == MODE_WORKSHOP) ? "FORGE" : "READY";
    uint16_t st_col = (g_game.mode == MODE_PAUSED) ? COLOR_AMBER :
                      (g_game.mode == MODE_WAVE) ? COLOR_LED_RED : COLOR_PHOSPHOR_GREEN;
    top_draw_text(190, 12, "MODE:", COLOR_IRON_LIGHT);
    top_draw_text(216, 12, st, st_col);
}

static void render_tab_bar(void) {
    // Tab Strip: Y: 22..34
    top_fill_rect(0, 22, SCREEN_W, 13, COLOR_BLACK);

    const char *tabs[3] = {"1:BATERIAS", "2:RADAR SWARM", "3:FORJA STC"};
    int tab_w = 84;

    for (int i = 0; i < 3; i++) {
        int tx = 2 + i * 85;
        int active = (s_active_tab == i);
        uint16_t bg = active ? COLOR_IRON_PANEL : COLOR_BLACK;
        uint16_t border = active ? COLOR_BRASS : COLOR_IRON_BORDER;
        uint16_t txt = active ? COLOR_AMBER : COLOR_IRON_LIGHT;

        top_fill_rect(tx, 22, tab_w, 13, bg);
        top_draw_rect(tx, 22, tab_w, 13, border);
        top_draw_text(tx + 6, 26, tabs[i], txt);
    }
}

static void render_tab_batteries(void) {
    if (s_detail_mode == 0) {
        // --- VISTA LISTA GENERAL ---
        top_fill_rect(4, 38, SCREEN_W - 8, 136, COLOR_BLACK);
        top_draw_rect(4, 38, SCREEN_W - 8, 136, COLOR_IRON_BORDER);

        // Header table
        top_fill_rect(6, 40, SCREEN_W - 12, 10, COLOR_IRON_PANEL);
        top_draw_text(8, 42, "ID TIPO    EJE   CONO   SATURACION ACC  DPS  KIL", COLOR_BRASS);

        int start_y = 52;
        int row_h = 24;

        for (int i = 0; i < MAX_TURRETS; i++) {
            int ry = start_y + i * row_h;
            Turret *t = &g_turrets[i];
            int is_sel = (s_selected_turret == i);

            if (is_sel) {
                top_fill_rect(6, ry, SCREEN_W - 12, row_h - 2, COLOR_IRON_PANEL);
                top_draw_rect(6, ry, SCREEN_W - 12, row_h - 2, COLOR_BRASS);
                top_draw_text(8, ry + 3, ">", COLOR_HAZARD_YELLOW);
            }

            char id_str[16];
            snprintf(id_str, sizeof(id_str), "#%d", i + 1);
            top_draw_text(14, ry + 3, id_str, is_sel ? COLOR_WHITE : COLOR_IRON_LIGHT);

            if (t->placed) {
                top_draw_text(26, ry + 3, "TWIN-B", COLOR_WHITE);

                // Central angle in degrees
                int deg = (t->center_angle * 360) / 256;
                char deg_str[16];
                snprintf(deg_str, sizeof(deg_str), "%03d", deg);
                top_draw_text(54, ry + 3, deg_str, COLOR_PHOSPHOR_GREEN);

                // Cone amplitude in degrees
                int cone_deg = (t->sweep_amplitude * 360) / 256;
                char cone_str[16];
                snprintf(cone_str, sizeof(cone_str), "+-%02d", cone_deg);
                top_draw_text(74, ry + 3, cone_str, COLOR_AMBER);

                // Saturation Gauge: Cadence / ConeDeg
                int total_cone = cone_deg * 2;
                if (total_cone <= 0) total_cone = 1;
                int rps = (t->fire_interval > 0) ? (60 / t->fire_interval) : 10;
                int sat_index = (rps * 100) / total_cone; // e.g. 8 * 100 / 40 = 20 (high)

                uint16_t sat_col = (sat_index >= 12) ? COLOR_PHOSPHOR_GREEN :
                                   (sat_index >= 7) ? COLOR_AMBER : COLOR_LED_RED;
                top_draw_gauge(106, ry + 4, 38, 5, sat_index, 25, sat_col, COLOR_BLACK);

                // Precision %
                int acc = (t->shots_fired > 0) ? (t->hits_confirmed * 100) / t->shots_fired : 0;
                char acc_str[16];
                snprintf(acc_str, sizeof(acc_str), "%2d%%", acc);
                top_draw_text(150, ry + 3, acc_str, COLOR_WHITE);

                // Live DPS
                int dps = 0;
                if (g_game.sim_ticks_elapsed > 10) {
                    dps = (t->damage_dealt * 60) / g_game.sim_ticks_elapsed;
                }
                char dps_str[16];
                snprintf(dps_str, sizeof(dps_str), "%3d", dps);
                top_draw_text(176, ry + 3, dps_str, COLOR_HAZARD_YELLOW);

                // Kills
                char kil_str[16];
                snprintf(kil_str, sizeof(kil_str), "%3d", t->kills);
                top_draw_text(202, ry + 3, kil_str, COLOR_WHITE);

                // Second line telemetry
                char sub_info[64];
                snprintf(sub_info, sizeof(sub_info), "Balas:%-4d Imp:%-4d Desperdicio:%2d%%",
                        t->shots_fired, t->hits_confirmed,
                        (t->shots_fired > 0) ? (t->wasted_shots * 100) / t->shots_fired : 0);
                top_draw_text(26, ry + 12, sub_info, COLOR_IRON_LIGHT);

            } else {
                top_draw_text(26, ry + 5, "[DISPONIBLE EN ARSENAL - DOCK]", COLOR_IRON_LIGHT);
            }
        }

        // Summary footer
        top_draw_line(6, 150, SCREEN_W - 6, 150, COLOR_IRON_BORDER);
        top_draw_text(8, 154, "D-PAD: [^v] Bateria  [<>] Girar  [X/Y] Cono", COLOR_AMBER);
        top_draw_text(8, 163, "BOTON (A): Abrir Gonio-Radar Balistico", COLOR_PHOSPHOR_GREEN);

    } else {
        // --- VISTA DETALLADA / GONIO-RADAR BALÍSTICO ---
        Turret *t = &g_turrets[s_selected_turret];

        top_fill_rect(4, 38, SCREEN_W - 8, 136, COLOR_BLACK);
        top_draw_rect(4, 38, SCREEN_W - 8, 136, COLOR_BRASS);

        // Header
        top_fill_rect(6, 40, SCREEN_W - 12, 11, COLOR_IRON_PANEL);
        char title_buf[64];
        snprintf(title_buf, sizeof(title_buf), "<< [B] VOLVER | BATERIA #%d: TWIN BOLTER", s_selected_turret + 1);
        top_draw_text(8, 42, title_buf, COLOR_AMBER);

        // Left Side: Gonio-Radar (cx = 60, cy = 104, r = 40)
        int cx = 60;
        int cy = 104;
        int r = 38;

        // Dial circles
        top_draw_circle(cx, cy, r, COLOR_IRON_BORDER, 0);
        top_draw_circle(cx, cy, r / 2, COLOR_DECK_GRID, 0);
        top_draw_line(cx - r, cy, cx + r, cy, COLOR_DECK_GRID);
        top_draw_line(cx, cy - r, cx, cy + r, COLOR_DECK_GRID);

        // Cardinal marks
        top_draw_text(cx - 2, cy - r - 6, "N", COLOR_IRON_LIGHT);
        top_draw_text(cx + r + 2, cy - 2, "E", COLOR_IRON_LIGHT);
        top_draw_text(cx - 2, cy + r + 2, "S", COLOR_IRON_LIGHT);
        top_draw_text(cx - r - 6, cy - 2, "W", COLOR_IRON_LIGHT);

        if (t->placed) {
            // Draw Cone Sector lines
            int left_ang = (t->center_angle - t->sweep_amplitude) & 0xFF;
            int right_ang = (t->center_angle + t->sweep_amplitude) & 0xFF;

            int lx = cx + ((fixed_cos(left_ang) * r) >> FP_SHIFT);
            int ly = cy + ((fixed_sin(left_ang) * r) >> FP_SHIFT);
            int rx = cx + ((fixed_cos(right_ang) * r) >> FP_SHIFT);
            int ry = cy + ((fixed_sin(right_ang) * r) >> FP_SHIFT);

            top_draw_line(cx, cy, lx, ly, COLOR_CONE_LINE);
            top_draw_line(cx, cy, rx, ry, COLOR_CONE_LINE);

            // Arc outline
            for (int a = t->center_angle - t->sweep_amplitude; a <= t->center_angle + t->sweep_amplitude; a += 4) {
                int ax = cx + ((fixed_cos(a & 0xFF) * r) >> FP_SHIFT);
                int ay = cy + ((fixed_sin(a & 0xFF) * r) >> FP_SHIFT);
                top_draw_pixel(ax, ay, COLOR_HAZARD_YELLOW);
            }

            // Real-time sweeping barrel needle
            int cur_ang = t->current_angle & 0xFF;
            int nx = cx + ((fixed_cos(cur_ang) * (r - 2)) >> FP_SHIFT);
            int ny = cy + ((fixed_sin(cur_ang) * (r - 2)) >> FP_SHIFT);

            uint16_t needle_col = (t->flash_timer > 0) ? COLOR_MUZZLE_FLASH : COLOR_WHITE;
            top_draw_line(cx, cy, nx, ny, needle_col);
            top_draw_circle(nx, ny, 2, COLOR_AMBER, 1);

            // Center hub
            top_draw_circle(cx, cy, 5, COLOR_BRASS, 1);
            top_draw_circle(cx, cy, 2, COLOR_LED_RED, 1);
        }

        // Right Side: High-Precision Telemetry Panel (X: 112..248)
        top_fill_rect(112, 54, 134, 114, COLOR_IRON_PANEL);
        top_draw_rect(112, 54, 134, 114, COLOR_IRON_BORDER);

        if (t->placed) {
            char buf[64];
            int deg = (t->center_angle * 360) / 256;
            int cone_deg = (t->sweep_amplitude * 360) / 256;
            int rps = (t->fire_interval > 0) ? (60 / t->fire_interval) : 10;
            int total_cone = cone_deg * 2;
            if (total_cone <= 0) total_cone = 1;
            int sat_index = (rps * 100) / total_cone;

            top_draw_text(116, 58, "TELEMETRIA DE BATERIA:", COLOR_BRASS);

            snprintf(buf, sizeof(buf), "EJE CENTRAL  : %03d GRADOS", deg);
            top_draw_text(116, 68, buf, COLOR_WHITE);

            snprintf(buf, sizeof(buf), "CONO BARRIDO : +-%02d (%02d TOT)", cone_deg, total_cone);
            top_draw_text(116, 78, buf, COLOR_AMBER);

            snprintf(buf, sizeof(buf), "CADENCIA     : %d DISP/S", rps);
            top_draw_text(116, 88, buf, COLOR_WHITE);

            top_draw_text(116, 98, "SATURACION   :", COLOR_WHITE);
            uint16_t sat_col = (sat_index >= 12) ? COLOR_PHOSPHOR_GREEN :
                               (sat_index >= 7) ? COLOR_AMBER : COLOR_LED_RED;
            top_draw_gauge(176, 99, 44, 5, sat_index, 25, sat_col, COLOR_BLACK);

            const char *sat_diag = (sat_index >= 12) ? "MURO DE FUEGO (OK)" :
                                   (sat_index >= 7) ? "COBERTURA MEDIA" : "DISPERSA / FILTRA!";
            top_draw_text(116, 107, sat_diag, sat_col);

            int acc = (t->shots_fired > 0) ? (t->hits_confirmed * 100) / t->shots_fired : 0;
            snprintf(buf, sizeof(buf), "PRECISION    : %d%% (%d/%d)", acc, t->hits_confirmed, t->shots_fired);
            top_draw_text(116, 118, buf, COLOR_WHITE);

            int wasted = (t->shots_fired > 0) ? (t->wasted_shots * 100) / t->shots_fired : 0;
            snprintf(buf, sizeof(buf), "BALAS VACIO  : %d%% DESPERD.", wasted);
            top_draw_text(116, 128, buf, COLOR_IRON_LIGHT);

            snprintf(buf, sizeof(buf), "DANIO TOTAL  : %d HP", t->damage_dealt);
            top_draw_text(116, 138, buf, COLOR_HAZARD_YELLOW);

            snprintf(buf, sizeof(buf), "XENOS PURGADOS: %d BAJAS", t->kills);
            top_draw_text(116, 148, buf, COLOR_PHOSPHOR_GREEN);

        } else {
            top_draw_text(120, 90, "BATERIA NO INSTALADA", COLOR_IRON_LIGHT);
            top_draw_text(120, 100, "Arrastra desde el dock", COLOR_AMBER);
        }

        // Footer hint
        top_draw_text(8, 160, "[<>] Rotar Eje   [X/Y] Cono +-5   [B] Volver", COLOR_AMBER);
    }
}

static void render_tab_swarm(void) {
    top_fill_rect(4, 38, SCREEN_W - 8, 136, COLOR_BLACK);
    top_draw_rect(4, 38, SCREEN_W - 8, 136, COLOR_IRON_BORDER);

    // Header
    top_fill_rect(6, 40, SCREEN_W - 12, 10, COLOR_IRON_PANEL);
    top_draw_text(8, 42, "AUSPEX TACTICO: RADAR DEL DESFILADERO Y BIOMASA", COLOR_AMBER);

    // Left: Mini Trench Map (X: 8..120, Y: 54..166)
    top_fill_rect(8, 54, 114, 112, COLOR_IRON_PANEL);
    top_draw_rect(8, 54, 114, 112, COLOR_IRON_BORDER);
    top_draw_text(12, 57, "RADAR SECTOR 1", COLOR_IRON_LIGHT);

    // Draw miniature trench path
    // Scale: screen 256x192 -> radar 100x80 (offset x=12, y=70, scale /2.5)
    for (int wp = 0; wp < g_waypoint_count - 1; wp++) {
        int x0 = 14 + (g_waypoints[wp].x * 100) / SCREEN_W;
        int y0 = 70 + (g_waypoints[wp].y * 80) / SCREEN_H;
        int x1 = 14 + (g_waypoints[wp + 1].x * 100) / SCREEN_W;
        int y1 = 70 + (g_waypoints[wp + 1].y * 80) / SCREEN_H;
        top_draw_line(x0, y0, x1, y1, COLOR_TRENCH_GRATE);
    }

    // Draw living enemies on mini-radar
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].active) {
            int ex = 14 + ((FROM_FP(g_enemies[i].x)) * 100) / SCREEN_W;
            int ey = 70 + ((FROM_FP(g_enemies[i].y)) * 80) / SCREEN_H;
            top_draw_pixel(ex, ey, COLOR_LED_RED);
            top_draw_pixel(ex + 1, ey, COLOR_LED_RED);
        }
    }

    // Draw deployed turrets on mini-radar
    for (int t = 0; t < MAX_TURRETS; t++) {
        if (g_turrets[t].placed) {
            int tx = 14 + (g_turrets[t].x * 100) / SCREEN_W;
            int ty = 70 + (g_turrets[t].y * 80) / SCREEN_H;
            top_draw_circle(tx, ty, 2, COLOR_PHOSPHOR_GREEN, 1);
        }
    }

    // Right: Biological Threat Auspex (X: 126..248, Y: 54..166)
    top_fill_rect(126, 54, 122, 112, COLOR_IRON_PANEL);
    top_draw_rect(126, 54, 122, 112, COLOR_IRON_BORDER);

    top_draw_text(130, 58, "BIOCASTAS XENOS:", COLOR_BRASS);

    top_draw_text(130, 70, "- T0: GAUNT RAPIDO", COLOR_WHITE);
    top_draw_text(130, 80, "- T1: HORMAGAUNT", COLOR_WHITE);
    top_draw_text(130, 90, "- T2: GUERRERO BLIN.", COLOR_WHITE);

    char buf[32];
    sprintf(buf, "ACTIVAS EN CAMPO: %d", g_game.enemies_alive);
    top_draw_text(130, 106, buf, COLOR_HAZARD_YELLOW);

    int total_enemies = (g_calibration.enemy_count > 0) ? g_calibration.enemy_count : TOTAL_WAVE_ENEMIES;
    sprintf(buf, "PURGADOS: %d / %d", g_game.enemies_killed, total_enemies);
    top_draw_text(130, 118, buf, COLOR_PHOSPHOR_GREEN);

    sprintf(buf, "BRECHAS SANCTUM : %d", g_game.enemies_breached);
    top_draw_text(130, 130, buf, (g_game.enemies_breached > 0) ? COLOR_LED_RED : COLOR_IRON_LIGHT);

    // Wave progress gauge
    top_draw_text(130, 142, "PRESION OLEADA:", COLOR_WHITE);
    top_draw_gauge(130, 152, 114, 6, g_game.enemies_killed, total_enemies, COLOR_AMBER, COLOR_BLACK);
}

static void render_tab_forge(void) {
    top_fill_rect(4, 38, SCREEN_W - 8, 136, COLOR_BLACK);
    top_draw_rect(4, 38, SCREEN_W - 8, 136, COLOR_IRON_BORDER);

    // Header
    top_fill_rect(6, 40, SCREEN_W - 12, 10, COLOR_IRON_PANEL);
    top_draw_text(8, 42, "ARSENAL DE LA FORJA: DOCTRINAS STC Y DIEZMO", COLOR_AMBER);

    // Active Doctrines
    top_fill_rect(8, 54, 240, 56, COLOR_IRON_PANEL);
    top_draw_rect(8, 54, 240, 56, COLOR_IRON_BORDER);

    top_draw_text(12, 58, "DOCTRINAS BALISTICAS ACTIVAS (FORGE STC):", COLOR_BRASS);

    char buf[64];
    sprintf(buf, "CADENCIA DE FUEGO : +%d%% VELOCIDAD RECARGA", (g_turret.fire_interval < 8) ? (8 - g_turret.fire_interval) * 20 : 0);
    top_draw_text(14, 70, buf, COLOR_WHITE);

    sprintf(buf, "CALIBRE Y DANIO   : +%d DANIO UNITARIO", g_skill_tree.bonus_damage);
    top_draw_text(14, 80, buf, COLOR_WHITE);

    sprintf(buf, "PERFORACION AP    : +%d PENETRACION DE ARMADURA", g_skill_tree.bonus_ap);
    top_draw_text(14, 90, buf, COLOR_WHITE);

    const char *doc_name = skills_get_active_doctrine_name();
    sprintf(buf, "DOCTRINA SUPREMA  : %s", doc_name);
    top_draw_text(14, 100, buf, COLOR_HAZARD_YELLOW);

    // Tithe Balance
    top_fill_rect(8, 114, 240, 54, COLOR_IRON_PANEL);
    top_draw_rect(8, 114, 240, 54, COLOR_IRON_BORDER);

    top_draw_text(12, 118, "CONTABILIDAD DEL DIEZMO IMPERIAL:", COLOR_BRASS);

    sprintf(buf, "RESERVA DISPONIBLE   : %d SC (CHATARRA)", g_game.scrap);
    top_draw_text(14, 130, buf, COLOR_HAZARD_YELLOW);

    top_draw_text(14, 140, "ESTADO DE MEJORAS    : DISPONIBLE EN PANTALLA INFERIOR", COLOR_PHOSPHOR_GREEN);
    top_draw_text(14, 150, "Pulsa [FORGE STC] en preparacion para consagrar ritos", COLOR_IRON_LIGHT);
}

static void render_footer(void) {
    // Footer: Y: 176..191
    top_fill_rect(0, 176, SCREEN_W, 16, COLOR_IRON_PANEL);
    top_draw_rect(0, 176, SCREEN_W, 16, COLOR_BRASS);

    top_draw_text(6, 180, "[L/R] Pestanas", COLOR_AMBER);
    top_draw_text(74, 180, "[^v] Bateria", COLOR_WHITE);
    top_draw_text(138, 180, "[<>] Girar", COLOR_WHITE);
    top_draw_text(188, 180, "[X/Y] Cono +-5", COLOR_HAZARD_YELLOW);
}

void telemetry_gfx_render(void) {
    top_clear(COLOR_DECK_FLOOR);

    render_header();
    render_tab_bar();

    switch (s_active_tab) {
        case TOP_TAB_BATTERIES: render_tab_batteries(); break;
        case TOP_TAB_SWARM:     render_tab_swarm(); break;
        case TOP_TAB_FORGE:     render_tab_forge(); break;
    }

    render_footer();
}

void telemetry_gfx_present(void) {
    if (s_top_vram) {
        dmaCopyWords(1, g_top_backbuffer, s_top_vram, sizeof(g_top_backbuffer));
    }
}
