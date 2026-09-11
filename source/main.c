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

        // Global priority: START always toggles pause in wave/prep/paused
        if (keys_down & KEY_START) {
            if (g_game.mode == MODE_WAVE || g_game.mode == MODE_PREPARATION || g_game.mode == MODE_PAUSED) {
                game_toggle_pause();
            }
        }

        // SELECT: enter/exit calibration, preserving previous mode
        if (keys_down & KEY_SELECT) {
            if (g_game.mode == MODE_CALIBRATION) {
                // Exiting calibration: apply settings (without resetting HP/scrap) and restore previous mode
                calibration_apply_settings();
                // Restore to previous mode, defaulting to PREPARATION if not set
                GameMode restore = g_game.previous_mode;
                if (restore == MODE_CALIBRATION || restore == MODE_PAUSED) restore = MODE_PREPARATION;
                g_game.mode = restore;
            } else if (g_game.mode != MODE_GAME_OVER && g_game.mode != MODE_WORKSHOP) {
                // Save current mode and enter calibration
                g_game.previous_mode = g_game.mode;
                g_game.mode = MODE_CALIBRATION;
            }
        }

        // Bottom Screen mode-specific input
        if (g_game.mode == MODE_CALIBRATION) {
            game_handle_input_calibration(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_PAUSED) {
            game_handle_input_pause(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_PREPARATION) {
            game_handle_input_prep(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_WAVE) {
            game_handle_input_wave(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_WORKSHOP) {
            game_handle_input_workshop(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_GAME_OVER) {
            game_handle_input_game_over(touch, keys_down, keys_held);
        }

        // Simulation update (strictly during active WAVE mode)
        if (g_game.mode == MODE_WAVE) {
            game_update_simulation();
            if (g_game.fast_forward == 2 && g_game.mode == MODE_WAVE) {
                game_update_simulation();
            }
        }

        // Visual render (Bottom Screen)
        if (g_game.mode == MODE_PREPARATION || g_game.mode == MODE_WAVE || g_game.mode == MODE_PAUSED || g_game.mode == MODE_GAME_OVER) {
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

            if (g_game.mode == MODE_PAUSED) {
                renderer_draw_ui_pause();
            } else if (g_game.mode == MODE_GAME_OVER) {
                renderer_draw_ui_game_over();
            }
        } else if (g_game.mode == MODE_CALIBRATION) {
            renderer_draw_ui_calibration();
        } else if (g_game.mode == MODE_WORKSHOP) {
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
