#include "game.h"
#include "tiles.h"

int main(void) {
    // Adaptive simulation keeps real-time progression when rendering is saturated.
    // It is intentionally capped at 3 total simulation ticks per displayed frame.
    static int s_adaptive_steps = 1;
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
        g_game.sandbox.last_keys_down = keys_down;
        g_game.sandbox.last_keys_held = keys_held;
        touchPosition touch = {0};
        touchRead(&touch);

        // Global priority: START toggles pause in active wave or paused mode
        int start_toggled = 0;
        if (keys_down & KEY_START) {
            if (g_game.mode == MODE_WAVE || g_game.mode == MODE_PAUSED) {
                game_toggle_pause();
                start_toggled = 1;
            }
        }

        // Invisible Debug Hotkey: L + SELECT toggles Test Sandbox Lab
        static int s_sandbox_hotkey_held = 0;
        int hotkey_active = ((keys_held & KEY_L) != 0 && (keys_held & KEY_SELECT) != 0);
        static int sandbox_b_was_held = 0;
        if (hotkey_active) {
            if (!s_sandbox_hotkey_held) {
                s_sandbox_hotkey_held = 1;
                if (g_game.mode != MODE_DEBUG_SANDBOX && g_game.mode != MODE_GAME_OVER) {
                    g_game.previous_mode = g_game.mode;
                    g_game.mode = MODE_DEBUG_SANDBOX;
                    // Sandbox owns the full battlefield surface. Restore the
                    // canonical ground immediately so stale victory/game-over
                    // UI pixels cannot contaminate the first profiling frame.
                    tiles_full_screen_refresh();
                }
            }
        } else {
            s_sandbox_hotkey_held = 0;
        }

        // Bottom Screen mode-specific input
        if (!start_toggled && g_game.mode == MODE_DEBUG_SANDBOX) {
            game_handle_input_sandbox(touch, keys_down, keys_held);
        } else if (!start_toggled && (g_game.mode == MODE_PAUSED || g_game.mode == MODE_PREPARATION)) {
            game_handle_input_pause(touch, keys_down, keys_held);
        } else if (!start_toggled && g_game.mode == MODE_WAVE) {
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

        // Performance harness: a deliberate 30-frame entry chord requests a
        // full-capacity sandbox load without relying on key-release plumbing.
        static int s_stress_chord_frames = 0;
        if (g_game.mode == MODE_DEBUG_SANDBOX && hotkey_active) {
            if (s_stress_chord_frames < 91) s_stress_chord_frames++;
            if (s_stress_chord_frames == 30 && g_game.sandbox.spawn_count == 0) {
                game_sandbox_load_stress_profile();
            } else if (s_stress_chord_frames == 60 && g_game.sandbox.spawn_count > 0) {
                g_game.sandbox.run_sim = 1;
            } else if (s_stress_chord_frames == 90 && g_game.sandbox.spawn_count > 0) {
                g_game.sandbox.separation_enabled = 0;
            }
        } else if (!hotkey_active) {
            s_stress_chord_frames = 0;
        }
        if (g_game.mode == MODE_DEBUG_SANDBOX) {
            int sandbox_b_held = (keys_held & KEY_B) != 0;
            if (sandbox_b_held && !sandbox_b_was_held) {
                g_game.sandbox.profiler_compact = !g_game.sandbox.profiler_compact;
            }
            sandbox_b_was_held = sandbox_b_held;
        } else {
            sandbox_b_was_held = 0;
        }
        if (g_game.mode == MODE_DEBUG_SANDBOX && g_game.sandbox.run_sim &&
            g_game.sandbox.spawn_count > 0 && g_game.sim_ticks_elapsed == 30) {
            g_game.sandbox.separation_enabled = 0;
        }

        // --- SUB-SYSTEM PROFILER START ---
        timerElapsed(0); // Reset timer 0 baseline

        // 1. Simulation update
        if (g_game.mode == MODE_WAVE) {
            int sim_steps = s_adaptive_steps;
            if (g_game.fast_forward == 2) sim_steps *= 2;
            if (sim_steps > 3) sim_steps = 3;
            for (int step = 0; step < sim_steps && g_game.mode == MODE_WAVE; step++) {
                game_update_simulation();
            }
        } else if (g_game.mode == MODE_DEBUG_SANDBOX && g_game.sandbox.run_sim) {
            game_update_simulation();
        }

        // Finalize the chord transition after simulation side effects and
        // immediately before rendering.
        if (hotkey_active && g_game.mode != MODE_DEBUG_SANDBOX &&
            g_game.mode != MODE_GAME_OVER) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_DEBUG_SANDBOX;
            tiles_full_screen_refresh();
        }
        uint16_t sim_t = timerElapsed(0);

        // 2. Visual render Top Screen
        renderer_draw_battlefield_top();
        uint16_t top_restore_t = timerElapsed(0);
        renderer_draw_splatters_top();
        renderer_draw_enemies_top();
        uint16_t top_enemy_t = timerElapsed(0);
        renderer_draw_death_particles_top();
        uint16_t top_t = top_restore_t + top_enemy_t;
        g_game.prof_top_restore_ticks = top_restore_t;
        g_game.prof_top_enemy_ticks = top_enemy_t;

        // 3. Visual render Bottom Screen
        uint16_t bot_base_t = 0, bot_enemy_t = 0, bot_fx_t = 0, bot_ui_t = 0;
        if (g_game.mode == MODE_UPGRADES) {
            renderer_draw_ui_upgrades();
        } else if (g_game.mode == MODE_CALIBRATION) {
            renderer_draw_ui_calibration();
        } else {
            renderer_draw_battlefield_bottom();
            renderer_draw_splatters_bottom();
            bot_base_t = timerElapsed(0);
            renderer_draw_enemies_bottom();
            bot_enemy_t = timerElapsed(0);
            renderer_draw_wall(); // 3D Depth: Wall parapet occludes enemy heads & front limbs
            renderer_draw_bullets();
            renderer_draw_death_particles_bottom();
            bot_fx_t = timerElapsed(0);

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
            bot_ui_t = timerElapsed(0);
        }
        uint16_t bot_t = bot_base_t + bot_enemy_t + bot_fx_t + bot_ui_t;
        // Keep the phase counters instantaneous: they are deliberately useful
        // for spotting spikes in a single captured frame, while B remains the
        // one-second average shown in the main profiler row.
        g_game.prof_bot_base_ticks = bot_base_t;
        g_game.prof_bot_enemy_ticks = bot_enemy_t;
        g_game.prof_bot_fx_ticks = bot_fx_t;
        g_game.prof_bot_ui_ticks = bot_ui_t;

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
        // React on the very next frame to the measured frame interval instead
        // of waiting for the one-second FPS aggregate to close.
        if (t1_delta > 0) {
            int instant_fps = 32728 / t1_delta;
            if (instant_fps <= 20) s_adaptive_steps = 3;
            else if (instant_fps <= 30) s_adaptive_steps = 2;
            else if (instant_fps >= 45) s_adaptive_steps = 1;
        }
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
            g_game.prof_enemies_active = 0;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (g_enemies[i].active) g_game.prof_enemies_active++;
            }
        }
    }

    return 0;
}
