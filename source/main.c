#include "game.h"

void telemetry_init_palette(void) {
    // Deep CRT phosphor dark background
    BG_PALETTE_SUB[0] = RGB15(1, 2, 2) | BIT(15);

    // Standard console text colors (ANSI 30..37)
    BG_PALETTE_SUB[1] = RGB15(31, 4, 3)   | BIT(15); // [31m Red Alert
    BG_PALETTE_SUB[2] = RGB15(5, 31, 10)  | BIT(15); // [32m Phosphor Green
    BG_PALETTE_SUB[3] = RGB15(31, 25, 4)  | BIT(15); // [33m Amber Gold
    BG_PALETTE_SUB[4] = RGB15(6, 12, 24)  | BIT(15); // [34m Dark Steel Blue
    BG_PALETTE_SUB[5] = RGB15(26, 8, 20)  | BIT(15); // [35m Xenos Magenta
    BG_PALETTE_SUB[6] = RGB15(6, 28, 31)  | BIT(15); // [36m Plasma Cyan
    BG_PALETTE_SUB[7] = RGB15(31, 31, 30) | BIT(15); // [37m Bone White
}

static void make_bar(char *buf, int val, int max_val, int len, char fill_ch, char empty_ch) {
    int filled = (max_val > 0) ? (val * len) / max_val : 0;
    if (filled < 0) filled = 0;
    if (filled > len) filled = len;
    for (int i = 0; i < len; i++) {
        buf[i] = (i < filled) ? fill_ch : empty_ch;
    }
    buf[len] = '\0';
}

void telemetry_render_top(void) {
    printf("\x1b[0;0H");

    // Tactical calculations
    int shots = g_turret.shots_fired;
    int hits = g_turret.hits_confirmed;
    int acc_int = (shots > 0) ? (hits * 100) / shots : 0;
    int acc_dec = (shots > 0) ? ((hits * 1000) / shots) % 10 : 0;
    int wasted_pct = (shots > 0) ? (g_turret.wasted_shots * 100) / shots : 0;

    int dps_int = 0, dps_dec = 0;
    if (g_game.sim_ticks_elapsed > 10) {
        int dps_scaled = (g_turret.damage_dealt * 600) / g_game.sim_ticks_elapsed;
        dps_int = dps_scaled / 10;
        dps_dec = dps_scaled % 10;
    }

    char hull_bar[12];
    make_bar(hull_bar, g_game.core_hp, g_game.core_max_hp, 10, '#', '-');

    char swarm_bar[12];
    make_bar(swarm_bar, g_game.enemies_killed, TOTAL_WAVE_ENEMIES, 10, '#', '-');
    int swarm_pct = (TOTAL_WAVE_ENEMIES > 0) ? (g_game.enemies_killed * 100) / TOTAL_WAVE_ENEMIES : 0;

    const char *doctrine = skills_get_active_doctrine_name();
    int rps = (g_turret.fire_interval > 0) ? (60 / g_turret.fire_interval) : 10;

    // Header Status
    const char *st_short = (g_game.core_hp <= 0) ? "BREACH" :
                          (g_game.mode == MODE_WAVE) ? "PURGE " :
                          (g_game.mode == MODE_WORKSHOP) ? "FORGE " : "READY ";

    printf("\x1b[1;2H\x1b[33m+----------------------------+");
    printf("\x1b[2;2H| ++ AUSPEX COGITATOR M41 ++ |");
    printf("\x1b[3;2H| BASTION:THETA-7 WAVE:%02d/09 |", g_game.wave_number);
    printf("\x1b[4;2H\x1b[32m| HULL: [%s] %02d/%02d |", hull_bar, g_game.core_hp, g_game.core_max_hp);
    printf("\x1b[5;2H| TITHE:%-5dSC  STAT:\x1b[37m%-6s\x1b[32m|", g_game.scrap, st_short);
    printf("\x1b[6;2H\x1b[33m+----------------------------+");
    printf("\x1b[7;2H| >> XENOS SWARM AUSPEX <<   |\x1b[32m");
    printf("\x1b[8;2H| PURGED: [%s] %3d%%  |", swarm_bar, swarm_pct);
    printf("\x1b[9;2H| ACTIVE:%-3d   KILLED:%-3d   |", g_game.enemies_alive, g_game.enemies_killed);
    printf("\x1b[10;2H| BREACH:%-3d   THREAT:T0..T2 |", g_game.enemies_breached);
    printf("\x1b[11;2H\x1b[33m+----------------------------+");
    printf("\x1b[12;2H| >> BATTERY DOCTRINE <<     |\x1b[32m");
    printf("\x1b[13;2H| RITE:\x1b[37m%-22s\x1b[32m|", doctrine);
    printf("\x1b[14;2H| PURGE RATE: %3d.%d DPS     |", dps_int, dps_dec);
    printf("\x1b[15;2H| ACC:%3d.%d%%  HITS:%-4d    |", acc_int, acc_dec, hits);
    printf("\x1b[16;2H| SHOTS:%-5d WASTED:%2d%%     |", shots, wasted_pct);
    printf("\x1b[17;2H| CADENCE:%2d/s DMG:+%d AP:+%d |", rps, g_skill_tree.bonus_damage, g_skill_tree.bonus_ap);
    printf("\x1b[18;2H\x1b[33m+----------------------------+");
    printf("\x1b[19;2H| >> BASTION ORDERS <<       |\x1b[32m");

    if (g_game.mode == MODE_PREPARATION) {
        printf("\x1b[20;2H| [PURGE] = ENGAGE RITES     |");
        printf("\x1b[21;2H| Stylus: Emplace & Aim Arc  |");
    } else if (g_game.mode == MODE_WAVE) {
        printf("\x1b[20;2H| Destruction Rites Active   |");
        printf("\x1b[21;2H| [COG 2X] / [R] Fast Compute|");
    } else {
        printf("\x1b[20;2H| OMNISSIAH ARSENAL FORGE    |");
        printf("\x1b[21;2H| Sanctify Rites & Re-engage |");
    }

    const char *spd_str = (g_game.fast_forward == 2) ? "[COG 2X VELOZ]" : "[1X ESTANDAR] ";
    printf("\x1b[22;2H| COMPUTE: \x1b[33m%-18s\x1b[32m|", spd_str);
    printf("\x1b[23;2H\x1b[33m+----------------------------+\x1b[32m");
}

int main(void) {
    // Top Screen: Cogitator Green Phosphor CRT Console
    consoleDemoInit();
    consoleClear();
    telemetry_init_palette();

    // Bottom Screen: Framebuffer Graphics
    renderer_init();

    // Internal Math and Systems
    math_init();
    game_init();

    while (1) {
        scanKeys();
        int keys_down = keysDown();
        int keys_held = keysHeld();
        touchPosition touch;
        touchRead(&touch);

        // Input processing
        if (g_game.mode == MODE_PREPARATION) {
            game_handle_input_prep(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_WAVE) {
            game_handle_input_wave(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_WORKSHOP || g_game.mode == MODE_GAME_OVER) {
            game_handle_input_workshop(touch, keys_down, keys_held);
        }

        // Simulation update
        if (g_game.mode == MODE_WAVE) {
            game_update_simulation();
            if (g_game.fast_forward == 2 && g_game.mode == MODE_WAVE) {
                game_update_simulation();
            }
        }

        // Visual render
        if (g_game.mode == MODE_PREPARATION || g_game.mode == MODE_WAVE) {
            renderer_clear(COLOR_DECK_FLOOR);
            renderer_draw_trench_path();
            renderer_draw_splatters();
            renderer_draw_enemies();
            renderer_draw_bullets();
            renderer_draw_turret(&g_turret, (g_game.selected_turret >= 0), (g_game.mode == MODE_PREPARATION));
            renderer_draw_ui_prep();
        } else {
            renderer_draw_ui_workshop();
        }
        renderer_present();

        // Telemetry update
        telemetry_render_top();
    }

    return 0;
}
