#include "game.h"

int main(void) {
    // Both screens initialized in renderer_init:
    // Bottom Screen: Direct Framebuffer Mode FB0
    // Top Screen: Sub-engine 16-bit Bitmap BG3 Mode 5
    renderer_init();

    // Internal Math and Systems
    math_init();
    game_init();

    while (1) {
        swiWaitForVBlank();

        scanKeys();
        int keys_down = keysDown();
        int keys_held = keysHeld();
        touchPosition touch;
        touchRead(&touch);

        // Global priority: START toggles pause in active wave or paused mode
        if (keys_down & KEY_START) {
            if (g_game.mode == MODE_WAVE || g_game.mode == MODE_PAUSED) {
                game_toggle_pause();
            }
        }

        // SELECT toggles Upgrade Tree directly
        if (keys_down & KEY_SELECT) {
            if (g_game.mode == MODE_UPGRADES) {
                g_game.mode = (g_game.previous_mode == MODE_UPGRADES || g_game.previous_mode == MODE_PAUSED) 
                              ? MODE_PREPARATION : g_game.previous_mode;
            } else if (g_game.mode != MODE_GAME_OVER) {
                g_game.previous_mode = g_game.mode;
                g_game.mode = MODE_UPGRADES;
            }
        }

        // Bottom Screen mode-specific input
        if (g_game.mode == MODE_PAUSED) {
            game_handle_input_pause(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_PREPARATION) {
            game_handle_input_prep(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_WAVE) {
            game_handle_input_wave(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_UPGRADES) {
            game_handle_input_upgrades(touch, keys_down, keys_held);
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

        // Visual render (Top Screen: Continuous Urban Ground + Inbound Swarm)
        renderer_draw_battlefield_top();
        renderer_draw_splatters_top();
        renderer_draw_enemies_top();
        renderer_draw_death_particles_top();

        // Visual render (Bottom Screen: Urban Ground + Bunker + Turrets + Inbound Swarm + UI)
        if (g_game.mode == MODE_UPGRADES) {
            renderer_draw_ui_upgrades();
        } else {
            renderer_draw_battlefield_bottom();
            renderer_draw_splatters_bottom();
            renderer_draw_enemies_bottom();
            renderer_draw_bullets();
            renderer_draw_death_particles_bottom();

            // Render all placed turrets with animated recoil and sparks
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    int is_sel = (g_game.selected_turret == t);
                    renderer_draw_turret(&g_turrets[t], is_sel);
                }
            }

            if (g_game.mode == MODE_PREPARATION) {
                renderer_draw_ui_prep();
            } else if (g_game.mode == MODE_WAVE) {
                renderer_draw_ui_wave();
            } else if (g_game.mode == MODE_PAUSED) {
                renderer_draw_ui_pause();
            } else if (g_game.mode == MODE_GAME_OVER) {
                renderer_draw_ui_game_over();
            }
        }

        // Synchronized presentation for both screens
        renderer_present();
        top_screen_present();
    }

    return 0;
}
