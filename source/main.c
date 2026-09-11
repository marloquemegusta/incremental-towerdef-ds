#include "game.h"
#include "telemetry_gfx.h"

int main(void) {
    // Top Screen: Auspex Cogitator OS (16-bit Bitmap Direct Engine)
    telemetry_gfx_init();

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

        // Process Top Screen Auspex navigation and calibration
        telemetry_gfx_update(keys_down, keys_held);

        // Bottom Screen Input processing
        if (keys_down & KEY_SELECT) {
            if (g_game.mode == MODE_CALIBRATION) {
                calibration_apply_settings();
                g_game.mode = MODE_PREPARATION;
            } else {
                g_game.mode = MODE_CALIBRATION;
            }
        } else if (g_game.mode == MODE_CALIBRATION) {
            game_handle_input_calibration(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_PREPARATION) {
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

        // Visual render (Bottom Screen)
        if (g_game.mode == MODE_PREPARATION || g_game.mode == MODE_WAVE) {
            renderer_draw_trench_path();
            renderer_draw_splatters();
            renderer_draw_enemies();
            renderer_draw_bullets();
            renderer_draw_death_particles();

            // Render all placed turrets
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    int is_sel = (g_game.selected_turret == t);
                    renderer_draw_turret(&g_turrets[t], is_sel, is_sel || (g_game.mode == MODE_PREPARATION));
                }
            }

            renderer_draw_ui_prep();
        } else if (g_game.mode == MODE_CALIBRATION) {
            renderer_draw_ui_calibration();
        } else {
            renderer_draw_ui_workshop();
        }

        // Visual render (Top Screen 16-bit Telemetry)
        telemetry_gfx_render();

        // Single Synchronized VBlank Presentation for Both Screens
        renderer_present();
        telemetry_gfx_present();
    }

    return 0;
}
