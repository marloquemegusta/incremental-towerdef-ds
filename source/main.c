#include "game.h"

int main(void) {
    // Both screens initialized in renderer_init:
    // Bottom Screen: Direct Framebuffer Mode FB0
    // Top Screen: Sub-engine 16-bit Bitmap BG3 Mode 5
    renderer_init();

    // Internal Math and Systems
    math_init();
    game_init();

    // Hardware Telemetry Timers (Timer 0: subsystem profiler, Timer 1: 1-sec FPS interval)
    timerStart(0, ClockDivider_1024, 0, NULL);
    timerStart(1, ClockDivider_1024, 0, NULL);

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

        // Invisible Debug Hotkey: L + SELECT toggles Test Sandbox Lab
        static int s_sandbox_hotkey_held = 0;
        int hotkey_active = ((keys_held & KEY_L) != 0 && (keys_held & KEY_SELECT) != 0);
        if (hotkey_active) {
            if (!s_sandbox_hotkey_held) {
                s_sandbox_hotkey_held = 1;
                if (g_game.mode == MODE_DEBUG_SANDBOX) {
                    g_game.mode = g_game.previous_mode;
                } else if (g_game.mode != MODE_GAME_OVER) {
                    g_game.previous_mode = g_game.mode;
                    g_game.mode = MODE_DEBUG_SANDBOX;
                }
            }
        } else {
            s_sandbox_hotkey_held = 0;
        }

        // Bottom Screen mode-specific input
        if (g_game.mode == MODE_DEBUG_SANDBOX) {
            game_handle_input_sandbox(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_PAUSED || g_game.mode == MODE_PREPARATION) {
            game_handle_input_pause(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_WAVE) {
            game_handle_input_wave(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_UPGRADES) {
            game_handle_input_upgrades(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_CALIBRATION) {
            game_handle_input_calibration(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_GAME_OVER) {
            game_handle_input_game_over(touch, keys_down, keys_held);
        } else if (g_game.mode == MODE_VICTORY) {
            game_handle_input_victory(touch, keys_down, keys_held);
        }

        // --- SUB-SYSTEM PROFILER START ---
        timerElapsed(0); // Reset timer 0 baseline

        // 1. Simulation update
        if (g_game.mode == MODE_WAVE) {
            game_update_simulation();
            if (g_game.fast_forward == 2 && g_game.mode == MODE_WAVE) {
                game_update_simulation();
            }
        } else if (g_game.mode == MODE_DEBUG_SANDBOX && g_game.sandbox.run_sim) {
            game_update_simulation();
        }
        uint16_t sim_t = timerElapsed(0);

        // 2. Visual render Top Screen
        renderer_draw_battlefield_top();
        renderer_draw_splatters_top();
        renderer_draw_enemies_top();
        renderer_draw_death_particles_top();
        uint16_t top_t = timerElapsed(0);

        // 3. Visual render Bottom Screen
        if (g_game.mode == MODE_UPGRADES) {
            renderer_draw_ui_upgrades();
        } else if (g_game.mode == MODE_CALIBRATION) {
            renderer_draw_ui_calibration();
        } else {
            renderer_draw_battlefield_bottom();
            renderer_draw_splatters_bottom();
            renderer_draw_enemies_bottom();
            renderer_draw_wall(); // 3D Depth: Wall parapet occludes enemy heads & front limbs
            renderer_draw_bullets();
            renderer_draw_death_particles_bottom();

            if (g_game.mode == MODE_PREPARATION || g_game.mode == MODE_PAUSED) {
                renderer_draw_ui_pause();
            } else if (g_game.mode == MODE_WAVE) {
                renderer_draw_ui_wave();
            } else if (g_game.mode == MODE_GAME_OVER) {
                renderer_draw_ui_game_over();
            } else if (g_game.mode == MODE_VICTORY) {
                renderer_draw_ui_victory();
            } else if (g_game.mode == MODE_DEBUG_SANDBOX) {
                renderer_draw_ui_sandbox();
            }
        }
        uint16_t bot_t = timerElapsed(0);

        // 4. Synchronized presentation for both screens
        renderer_present();
        top_screen_present();
        uint16_t pres_t = timerElapsed(0);

        // 5. Aggregate metrics
        static int s_prof_frames = 0;
        static uint32_t s_sum_sim = 0, s_sum_top = 0, s_sum_bot = 0, s_sum_pres = 0;
        static uint32_t s_timer1_accum = 0;
        static int s_fps_counter = 0;

        s_sum_sim += sim_t;
        s_sum_top += top_t;
        s_sum_bot += bot_t;
        s_sum_pres += pres_t;
        s_prof_frames++;
        s_fps_counter++;

        uint16_t t1_delta = timerElapsed(1);
        s_timer1_accum += t1_delta;
        if (s_timer1_accum >= 32728) { // 1.0 real second elapsed
            g_game.prof_fps = s_fps_counter;
            s_fps_counter = 0;
            s_timer1_accum -= 32728;
            if (s_prof_frames > 0) {
                g_game.prof_sim_ticks = s_sum_sim / s_prof_frames;
                g_game.prof_top_ticks = s_sum_top / s_prof_frames;
                g_game.prof_bot_ticks = s_sum_bot / s_prof_frames;
                g_game.prof_pres_ticks = s_sum_pres / s_prof_frames;
                s_sum_sim = s_sum_top = s_sum_bot = s_sum_pres = 0;
                s_prof_frames = 0;
            }
        }
    }

    return 0;
}
