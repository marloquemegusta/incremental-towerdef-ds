#include "game.h"

void telemetry_init_palette(void) {
    // Deep CRT phosphor dark background
    BG_PALETTE_SUB[0] = RGB15(0, 2, 1) | BIT(15);

    // Bright phosphor green for standard console text
    for (int i = 1; i < 16; i++) {
        BG_PALETTE_SUB[i] = RGB15(7, 31, 11) | BIT(15);
    }
    // High-contrast accents
    BG_PALETTE_SUB[3] = RGB15(31, 26, 0) | BIT(15); // Amber
    BG_PALETTE_SUB[1] = RGB15(31, 4, 4) | BIT(15);  // Alert Red
}

void telemetry_render_top(void) {
    printf("\x1b[0;0H\x1b[32m");

    const char *st = "STANDBY - READY       ";
    if (g_game.mode == MODE_WAVE) {
        st = (g_game.fast_forward == 2) ? "PURGING XENOS [2X]    " : "PURGING XENOS [1X]    ";
    } else if (g_game.mode == MODE_WORKSHOP) {
        st = (g_game.core_hp <= 0) ? "SANCTUM BREACHED!     " : "SECTOR SANCTIFIED!    ";
    }

    int shots = g_turret.shots_fired;
    int hits = g_turret.hits_confirmed;
    int wasted = g_turret.wasted_shots;
    
    int acc_int = (shots > 0) ? (hits * 100) / shots : 0;
    int acc_dec = (shots > 0) ? ((hits * 1000) / shots) % 10 : 0;
    int wst_int = (shots > 0) ? (wasted * 100) / shots : 0;

    int dps_int = 0;
    int dps_dec = 0;
    if (g_game.sim_ticks_elapsed > 10) {
        int dps_scaled = (g_turret.damage_dealt * 600) / g_game.sim_ticks_elapsed;
        dps_int = dps_scaled / 10;
        dps_dec = dps_scaled % 10;
    }

    printf("\x1b[33m++ AUSPEX COGITATOR - M41.82 ++\x1b[32m\n");
    printf("===============================\n");
    printf(" BASTION SECTOR: THETA-7       \n");
    printf(" HULL INTEGRITY: [%02d/%02d]      \n", g_game.core_hp, g_game.core_max_hp);
    printf(" SACRED TITHE:   \x1b[33m%-5d\x1b[32m         \n", g_game.scrap);
    printf(" STATUS: %s\n", st);
    printf("-------------------------------\n");
    printf(" XENOS BIO-MASS DETECTIONS:    \n");
    printf("  Active Organisms: %3d/%-3d   \n", g_game.enemies_alive, TOTAL_WAVE_ENEMIES);
    printf("  Confirmed Purges: %-3d       \n", g_game.enemies_killed);
    printf("  Bastion Breaches: %-3d       \n", g_game.enemies_breached);
    printf("-------------------------------\n");
    printf(" HEAVY BOLTER SERVO-BATTERY:   \n");
    printf("  Munitions Fired:  %-5d      \n", shots);
    printf("  Confirmed Impacts:%-5d      \n", hits);
    printf("  Target Accuracy:  %3d.%d%%    \n", acc_int, acc_dec);
    printf("  Wasted Rounds:    %-4d (%2d%%)\n", wasted, wst_int);
    printf("  Bio-Mass Purged:  %-5d      \n", g_turret.damage_dealt);
    printf("  Purge Rate:       %3d.%d DPS \n", dps_int, dps_dec);
    printf("===============================\n");

    if (g_game.mode == MODE_PREPARATION) {
        printf(" [PURGE WAVE] = ENGAGE RITES  \n");
        printf(" Stylus: Position & Aim Arc   \n");
    } else if (g_game.mode == MODE_WAVE) {
        printf(" Rites of Destruction active...\n");
        printf(" [R] / [COG 2X] = Fast Compute \n");
    } else {
        printf(" OMNISSIAH REQUISITION FORGE   \n");
        printf(" Sanctify Upgrades & Re-engage \n");
    }
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
