#include "game.h"
#include "enemy_data.h"
#include "tiles.h"

GameContext g_game;
Turret g_turrets[MAX_TURRETS];
Enemy g_enemies[MAX_ENEMIES];
Bullet g_bullets[MAX_BULLETS];
Splatter g_splatters[MAX_SPLATTERS];
RunCalibration g_calibration;

Waypoint g_waypoints[MAX_WAYPOINTS];
int g_waypoint_count = 5;
const uint16_t *g_current_map_bg = NULL;

void map_select(int map_index) {
    if (map_index < 0 || map_index >= 3) map_index = 0;
    g_calibration.selected_map = map_index;
    g_current_map_bg = g_map_backgrounds[map_index];
    g_waypoint_count = g_map_waypoint_counts[map_index];
    for (int i = 0; i < g_waypoint_count && i < MAX_WAYPOINTS; i++) {
        g_waypoints[i] = g_map_waypoints[map_index][i];
    }

    // Set thematic initial position for default Turret 0
    if (map_index == 0) { // La Trinchera con Chicana
        g_turrets[0].x = 150;
        g_turrets[0].y = 145;
        g_turrets[0].center_angle = 192; // Aim North towards highway
        g_turrets[0].current_angle = 192;
    } else if (map_index == 1) { // La Doble S
        g_turrets[0].x = 115;
        g_turrets[0].y = 75;
        g_turrets[0].center_angle = 64;  // Aim South towards highway
        g_turrets[0].current_angle = 64;
    } else if (map_index == 2) { // Rotonda del Sanctum
        g_turrets[0].x = 128;
        g_turrets[0].y = 112; // In center plaza between roads
        g_turrets[0].center_angle = 192;
        g_turrets[0].current_angle = 192;
    }
}

int game_is_pos_valid(int x, int y) {
    if (x < 12 || x > 244) return 0;
    if (y < 24 || y > 160) return 0;

    // Check distance to all active road segments
    for (int wp = 0; wp < g_waypoint_count - 1; wp++) {
        int x0 = g_waypoints[wp].x;
        int y0 = g_waypoints[wp].y;
        int x1 = g_waypoints[wp + 1].x;
        int y1 = g_waypoints[wp + 1].y;

        int dx = x1 - x0;
        int dy = y1 - y0;
        int len_sq = dx * dx + dy * dy;

        int cx = x0;
        int cy = y0;
        if (len_sq > 0) {
            int t = ((x - x0) * dx + (y - y0) * dy);
            if (t < 0) t = 0;
            else if (t > len_sq) t = len_sq;
            cx = x0 + (t * dx) / len_sq;
            cy = y0 + (t * dy) / len_sq;
        }

        int dist_x = x - cx;
        int dist_y = y - cy;
        // Road half-width is 16px + turret radius 8px = 24px clearance (use 22 for slight leeway)
        if (dist_x * dist_x + dist_y * dist_y < (22 * 22)) {
            return 0; // Overlaps road trench
        }
    }

    // Must not overlap another placed turret
    for (int t = 0; t < MAX_TURRETS; t++) {
        if (g_turrets[t].placed) {
            int tdx = x - g_turrets[t].x;
            int tdy = y - g_turrets[t].y;
            if (tdx * tdx + tdy * tdy < (16 * 16)) return 0;
        }
    }

    return 1;
}

void game_add_splatter(int x, int y, uint16_t color) {
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life <= 0) {
            g_splatters[i].x = x;
            g_splatters[i].y = y;
            g_splatters[i].life = 45;
            g_splatters[i].color = color;
            break;
        }
    }
}

void game_init(void) {
    memset(&g_game, 0, sizeof(g_game));
    memset(g_turrets, 0, sizeof(g_turrets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_splatters, 0, sizeof(g_splatters));

    calibration_init();
    map_select(0);

    g_game.mode = MODE_PREPARATION;
    g_game.wave_number = 1;
    g_game.core_hp = g_calibration.core_lives;
    g_game.core_max_hp = g_calibration.core_lives;
    g_game.scrap = g_calibration.starting_scrap;
    g_game.fast_forward = 1;
    g_game.turret_dock_count = 2; // Extra batteries available
    g_game.selected_turret = 0;

    int half_angle_units = (g_calibration.cone_spread * 64) / 90;
    if (half_angle_units < 4) half_angle_units = 4;

    // Heavy Bolter Default Profile for Turret 0
    g_turrets[0].id = 0;
    g_turrets[0].sweep_amplitude = half_angle_units;
    g_turrets[0].sweep_speed = g_calibration.sweep_speed;
    g_turrets[0].sweep_dir = 1;
    g_turrets[0].fire_interval = g_calibration.turret_fire_rate;
    g_turrets[0].range = g_calibration.turret_range;
    g_turrets[0].placed = 1;
    g_turrets[0].active = 1;
    g_turrets[0].flash_timer = 0;

    for (int i = 1; i < MAX_TURRETS; i++) {
        g_turrets[i].id = i;
        g_turrets[i].center_angle = 192;
        g_turrets[i].current_angle = 192;
        g_turrets[i].sweep_amplitude = half_angle_units;
        g_turrets[i].sweep_speed = g_calibration.sweep_speed;
        g_turrets[i].sweep_dir = 1;
        g_turrets[i].fire_interval = g_calibration.turret_fire_rate;
        g_turrets[i].range = g_calibration.turret_range;
        g_turrets[i].placed = 0;
        g_turrets[i].active = 0;
    }

    skills_init();
}

void game_reset_to_prep(void) {
    g_game.mode = MODE_PREPARATION;
    g_game.enemies_spawned = 0;
    g_game.enemies_alive = 0;
    g_game.enemies_killed = 0;
    g_game.enemies_breached = 0;
    g_game.spawn_timer = 0;
    g_game.sim_ticks_elapsed = 0;

    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_splatters, 0, sizeof(g_splatters));

    int base_interval = (g_calibration.turret_fire_rate > 0) ? g_calibration.turret_fire_rate : 8;
    int interval = base_interval - g_skill_tree.bonus_firerate;
    if (interval < 3) interval = 3;

    int half_angle_units = (g_calibration.cone_spread > 0) ? ((g_calibration.cone_spread * 64) / 90) : 16;
    if (half_angle_units < 4) half_angle_units = 4;

    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].shots_fired = 0;
        g_turrets[t].hits_confirmed = 0;
        g_turrets[t].wasted_shots = 0;
        g_turrets[t].damage_dealt = 0;
        g_turrets[t].fire_cooldown = 0;
        g_turrets[t].flash_timer = 0;
        g_turrets[t].fire_interval = interval;
        g_turrets[t].sweep_speed = (g_calibration.sweep_speed > 0) ? g_calibration.sweep_speed : 1;
        g_turrets[t].sweep_amplitude = half_angle_units;
        g_turrets[t].range = (g_calibration.turret_range > 0) ? g_calibration.turret_range : 75;
    }
}

void game_start_wave(void) {
    int any_placed = 0;
    for (int t = 0; t < MAX_TURRETS; t++) {
        if (g_turrets[t].placed) { any_placed = 1; break; }
    }
    if (!any_placed) {
        g_turrets[0].x = 100;
        g_turrets[0].y = 136;
        g_turrets[0].center_angle = 192;
        g_turrets[0].current_angle = 192;
        g_turrets[0].placed = 1;
        g_turrets[0].active = 1;
    }

    game_reset_to_prep();
    g_game.mode = MODE_WAVE;
    g_game.selected_turret = 0;
}

static void spawn_enemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            g_enemies[i].active = 1;
            g_enemies[i].waypoint_idx = 0;

            // Wide Swarm Dispersion across 32px highway: (-10 to +10 px)
            int offset = ((rand() % 21) - 10);
            g_enemies[i].lateral_offset = TO_FP(offset);
            
            // Speed jitter based on calibrated speed (enemy_speed_int is tenths of px/frame)
            int base_spd = (g_calibration.enemy_speed_int > 0) ? ((g_calibration.enemy_speed_int * FP_ONE) / 10) : FP_ONE;
            int jitter = ((rand() % 21) - 10) * (FP_ONE / 100);
            g_enemies[i].variant = rand() % ENEMY_VARIANT_COUNT;
            
            // Speed scaled by biocaste (Swarm runs fast, Colossals tread slowly)
            // T0: 1.2x, T1: 1.3x, T2: 1.5x, T3: 1.1x, T4: 0.6x, T5: 0.35x
            static const int s_tier_speed_mult[ENEMY_VARIANT_COUNT] = {
                120, // T0 Larva
                130, // T1 Ripper
                150, // T2 Hormagaunt
                110, // T3 Ravener
                 60, // T4 Carnifex
                 35  // T5 Hierophant
            };
            int tier_mult = s_tier_speed_mult[g_enemies[i].variant];
            int caste_spd = (base_spd * tier_mult) / 100;
            g_enemies[i].speed = caste_spd + jitter;
            g_enemies[i].hp = (g_calibration.enemy_hp > 0) ? g_calibration.enemy_hp : g_enemy_types[g_enemies[i].variant].default_hp;

            // Initial position (check if first segment is vertical or horizontal)
            if (g_waypoint_count > 1 && g_waypoints[0].x == g_waypoints[1].x) {
                g_enemies[i].x = TO_FP(g_waypoints[0].x) + g_enemies[i].lateral_offset;
                g_enemies[i].y = TO_FP(g_waypoints[0].y);
            } else {
                g_enemies[i].x = TO_FP(g_waypoints[0].x);
                g_enemies[i].y = TO_FP(g_waypoints[0].y) + g_enemies[i].lateral_offset;
            }

            g_game.enemies_spawned++;
            g_game.enemies_alive++;
            break;
        }
    }
}

static void spawn_bullet(int x, int y, int angle, int turret_idx) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) {
            g_bullets[i].active = 1;
            g_bullets[i].turret_idx = turret_idx;
            g_bullets[i].x = TO_FP(x);
            g_bullets[i].y = TO_FP(y);
            // Speed = 4 pixels per frame
            g_bullets[i].vx = (fixed_cos(angle) * 4);
            g_bullets[i].vy = (fixed_sin(angle) * 4);
            int rng = (turret_idx >= 0 && turret_idx < MAX_TURRETS) ? g_turrets[turret_idx].range : 75;
            g_bullets[i].life = (rng / 4) + 2;
            break;
        }
    }
}

void game_update_simulation(void) {
    if (g_game.mode != MODE_WAVE) return;

    g_game.sim_ticks_elapsed++;

    // 1. Spawning
    int max_to_spawn = (g_calibration.enemy_count > 0) ? g_calibration.enemy_count : TOTAL_WAVE_ENEMIES;
    int spawn_delay = (g_calibration.spawn_delay > 0) ? g_calibration.spawn_delay : 10;
    if (g_game.enemies_spawned < max_to_spawn) {
        g_game.spawn_timer++;
        if (g_game.spawn_timer >= spawn_delay) {
            g_game.spawn_timer = 0;
            spawn_enemy();
        }
    }

    // 2. Update Splatters life
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life > 0) g_splatters[i].life--;
    }

    // 3. Update Enemies (Following Waypoints with lateral corridor offset)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;

        int curr_wp = g_enemies[i].waypoint_idx;
        int next_wp = curr_wp + 1;
        if (next_wp >= g_waypoint_count) {
            // Reached Bunker Core!
            g_enemies[i].active = 0;
            g_game.enemies_alive--;
            g_game.enemies_breached++;
            g_game.core_hp--;
            game_add_splatter(FROM_FP(g_enemies[i].x), FROM_FP(g_enemies[i].y), COLOR_BLOOD_DARK);
            if (g_game.core_hp <= 0) {
                g_game.core_hp = 0;
                g_game.mode = MODE_CALIBRATION;
                return;
            }
            continue;
        }

        int x0 = g_waypoints[curr_wp].x, y0 = g_waypoints[curr_wp].y;
        int x1 = g_waypoints[next_wp].x, y1 = g_waypoints[next_wp].y;

        int target_x, target_y;
        if (x0 == x1) {
            // Vertical corridor: lateral offset applies horizontally
            target_x = TO_FP(x1) + g_enemies[i].lateral_offset;
            target_y = TO_FP(y1);
        } else if (y0 == y1) {
            // Horizontal corridor: lateral offset applies vertically
            target_x = TO_FP(x1);
            target_y = TO_FP(y1) + g_enemies[i].lateral_offset;
        } else {
            // Transition / diagonal segment: center line
            target_x = TO_FP(x1);
            target_y = TO_FP(y1);
        }

        int dx = target_x - g_enemies[i].x;
        int dy = target_y - g_enemies[i].y;
        int spd = g_enemies[i].speed;

        if (abs(dy) > abs(dx)) {
            g_enemies[i].dir = (dy > 0) ? 1 : 3; // 1 = South, 3 = North
        } else {
            g_enemies[i].dir = (dx > 0) ? 0 : 2; // 0 = East, 2 = West
        }
        // Animation frequency based on biocaste:
        // Swarm (T0-T2): rapid scuttling (every 3 ticks)
        // Mid (T3): fluid serpentine motion (every 5 ticks)
        // Heavy (T4-T5): heavy, deliberate steps (every 8-10 ticks)
        static const uint8_t s_anim_div[ENEMY_VARIANT_COUNT] = {
            3, // T0 Larva (frenetic crawl)
            3, // T1 Ripper (rapid undulating bite)
            4, // T2 Hormagaunt (galloping scythe strides)
            5, // T3 Ravener (rhythmic burrowing wave)
            8, // T4 Carnifex (heavy pillar stomps)
           10  // T5 Hierophant (colossal titan strides)
        };
        int v = g_enemies[i].variant;
        if (v < 0 || v >= ENEMY_VARIANT_COUNT) v = 0;
        int div = s_anim_div[v];
        int phase_offset = (i * 3 + v * 2);
        g_enemies[i].anim_frame = ((g_game.sim_ticks_elapsed + phase_offset) / div) % 4;

        if (dx > 0) {
            g_enemies[i].x += (dx < spd) ? dx : spd;
        } else if (dx < 0) {
            g_enemies[i].x += (dx > -spd) ? dx : -spd;
        }

        if (dy > 0) {
            g_enemies[i].y += (dy < spd) ? dy : spd;
        } else if (dy < 0) {
            g_enemies[i].y += (dy > -spd) ? dy : -spd;
        }

        // Reached waypoint?
        if (abs(g_enemies[i].x - target_x) < (FP_ONE) && abs(g_enemies[i].y - target_y) < (FP_ONE)) {
            g_enemies[i].waypoint_idx++;
        }
    }

    // 4. Update Heavy Bolter Turrets (All deployed units)
    for (int t = 0; t < MAX_TURRETS; t++) {
        Turret *tur = &g_turrets[t];
        if (!tur->placed || !tur->active) continue;

        if (tur->flash_timer > 0) tur->flash_timer--;
        if (tur->barrel_recoil_l > 0) tur->barrel_recoil_l--;
        if (tur->barrel_recoil_r > 0) tur->barrel_recoil_r--;

        tur->current_angle += tur->sweep_dir * tur->sweep_speed;

        int min_ang = tur->center_angle - tur->sweep_amplitude;
        int max_ang = tur->center_angle + tur->sweep_amplitude;

        if (tur->current_angle >= max_ang) {
            tur->current_angle = max_ang;
            tur->sweep_dir = -1;
        } else if (tur->current_angle <= min_ang) {
            tur->current_angle = min_ang;
            tur->sweep_dir = 1;
        }

        if (tur->fire_cooldown > 0) {
            tur->fire_cooldown--;
        } else {
            tur->fire_cooldown = tur->fire_interval;
            tur->flash_timer = 2;
            tur->last_barrel = 1 - tur->last_barrel;
            // Explosive snap back: set timer to 4
            if (tur->last_barrel == 0) {
                tur->barrel_recoil_l = 4;
            } else {
                tur->barrel_recoil_r = 4;
            }

            int ang = ((tur->current_angle + 2) & ~3) & 0xFF;
            int perp_x = -fixed_sin(ang);
            int perp_y = fixed_cos(ang);
            int s = (tur->last_barrel == 0) ? -3 : 3;
            int bx = tur->x + ((perp_x * s) >> FP_SHIFT);
            int by = tur->y + ((perp_y * s) >> FP_SHIFT);

            spawn_bullet(bx, by, ang, t);
            tur->shots_fired++;
        }
    }

    // 5. Update Bullets & Collisions
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!g_bullets[b].active) continue;

        g_bullets[b].x += g_bullets[b].vx;
        g_bullets[b].y += g_bullets[b].vy;
        g_bullets[b].life--;

        int bx = FROM_FP(g_bullets[b].x);
        int by = FROM_FP(g_bullets[b].y);

        int hit = 0;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int ex = FROM_FP(g_enemies[e].x);
            int ey = FROM_FP(g_enemies[e].y);

            // Radius hit check
            if (abs(bx - ex) <= 4 && abs(by - ey) <= 4) {
                hit = 1;
                int base_dmg = (g_calibration.turret_damage > 0) ? g_calibration.turret_damage : 1;
                int dmg = base_dmg + g_skill_tree.bonus_damage;
                g_enemies[e].hp -= dmg;
                int tid = g_bullets[b].turret_idx;
                if (tid >= 0 && tid < MAX_TURRETS) {
                    g_turrets[tid].damage_dealt += dmg;
                    g_turrets[tid].hits_confirmed++;
                }

                if (g_enemies[e].hp <= 0) {
                    g_enemies[e].active = 0;
                    g_game.enemies_alive--;
                    g_game.enemies_killed++;
                    if (tid >= 0 && tid < MAX_TURRETS) {
                        g_turrets[tid].kills++;
                    }
                    int reward = g_enemy_types[g_enemies[e].variant].scrap_value + (g_skill_tree.bonus_ap > 0 ? 1 : 0);
                    g_game.scrap += reward;

                    // Xenos ichor / blood splatter
                    uint16_t splat_col = (g_enemies[e].variant == 0 || g_enemies[e].variant == 3) ? COLOR_XENOS_ICHOR : COLOR_BLOOD_DARK;
                    game_add_splatter(ex, ey, splat_col);
                }
                g_bullets[b].active = 0;
                break;
            }
        }

        if (!hit) {
            if (g_bullets[b].life <= 0 || bx < 0 || bx >= SCREEN_W || by < 0 || by >= SCREEN_H) {
                int tid = g_bullets[b].turret_idx;
                if (tid >= 0 && tid < MAX_TURRETS) {
                    g_turrets[tid].wasted_shots++;
                }
                g_bullets[b].active = 0;
            }
        }
    }

    // 6. Wave Completion
    if (g_game.enemies_spawned >= max_to_spawn && g_game.enemies_alive == 0) {
        g_game.scrap += 25;
        g_game.wave_number++;
        g_game.mode = MODE_CALIBRATION;
    }
}

void game_handle_input_prep(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_R) {
        g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
    }

    if (keys_down & KEY_TOUCH) {
        // [PURGE / START] Button: (170..252, 2..16)
        if (touch.px >= 170 && touch.px <= 252 && touch.py >= 2 && touch.py <= 16) {
            game_start_wave();
            return;
        }

        // [COG 2X] Button: (110..164, 2..16)
        if (touch.px >= 110 && touch.px <= 164 && touch.py >= 2 && touch.py <= 16) {
            g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
            return;
        }

        // [RECALL] Button: (56..104, 2..16)
        if (g_game.selected_turret >= 0 && g_turrets[g_game.selected_turret].placed) {
            if (touch.px >= 56 && touch.px <= 104 && touch.py >= 2 && touch.py <= 16) {
                g_turrets[g_game.selected_turret].placed = 0;
                g_turrets[g_game.selected_turret].active = 0;
                g_game.turret_dock_count++;
                g_game.selected_turret = -1;
                return;
            }
        }

        // Dock slot touch: drag Heavy Bolter
        if (g_game.turret_dock_count > 0 && touch.px >= 8 && touch.px <= 80 && touch.py >= 168) {
            g_game.is_dragging_new = 1;
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
            return;
        }

        // Dock button touch: open CALIBRAR (x: 84..160, y: 168..191)
        if (touch.px >= 84 && touch.px <= 160 && touch.py >= 168) {
            g_game.mode = MODE_CALIBRATION;
            return;
        }

        // Dock button touch: open FORGE STC (Branching Skill Tree)
        if (touch.px >= 164 && touch.px <= 250 && touch.py >= 168) {
            g_game.mode = MODE_WORKSHOP;
            return;
        }

        // Touch on placed turret: select it
        for (int t = 0; t < MAX_TURRETS; t++) {
            if (g_turrets[t].placed) {
                int d = abs(touch.px - g_turrets[t].x) + abs(touch.py - g_turrets[t].y);
                if (d <= 16) {
                    g_game.selected_turret = t;
                    return;
                }
            }
        }

        // If turret selected and touch is outside: orient angle
        if (g_game.selected_turret >= 0 && g_turrets[g_game.selected_turret].placed && touch.py > 20 && touch.py < 165) {
            Turret *st = &g_turrets[g_game.selected_turret];
            int dy = touch.py - st->y;
            int dx = touch.px - st->x;
            st->center_angle = fixed_atan2(dy, dx);
            st->current_angle = st->center_angle;
            return;
        }

        g_game.selected_turret = -1;
    }

    if (keys_held & KEY_TOUCH) {
        if (g_game.is_dragging_new) {
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
        } else if (g_game.selected_turret >= 0 && g_turrets[g_game.selected_turret].placed) {
            Turret *st = &g_turrets[g_game.selected_turret];
            int dy = touch.py - st->y;
            int dx = touch.px - st->x;
            if (abs(dy) + abs(dx) > 10) {
                st->center_angle = fixed_atan2(dy, dx);
                st->current_angle = st->center_angle;
            }
        }
    } else {
        if (g_game.is_dragging_new) {
            if (game_is_pos_valid(g_game.drag_x, g_game.drag_y)) {
                int slot = -1;
                for (int t = 0; t < MAX_TURRETS; t++) {
                    if (!g_turrets[t].placed) { slot = t; break; }
                }
                if (slot >= 0) {
                    g_turrets[slot].x = g_game.drag_x;
                    g_turrets[slot].y = g_game.drag_y;
                    g_turrets[slot].placed = 1;
                    g_turrets[slot].active = 1;
                    g_turrets[slot].center_angle = 192;
                    g_turrets[slot].current_angle = 192;
                    g_turrets[slot].sweep_amplitude = 16;
                    g_turrets[slot].sweep_speed = 1;
                    g_turrets[slot].sweep_dir = 1;
                    int interval = 8 - g_skill_tree.bonus_firerate;
                    if (interval < 3) interval = 3;
                    g_turrets[slot].fire_interval = interval;
                    g_turrets[slot].range = 75;
                    g_game.turret_dock_count--;
                    g_game.selected_turret = slot;
                }
            }
            g_game.is_dragging_new = 0;
        }
    }
}

void game_handle_input_wave(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_R) {
        g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
    }
    if (keys_down & KEY_TOUCH) {
        if (touch.px >= 110 && touch.px <= 164 && touch.py >= 2 && touch.py <= 16) {
            g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
        }
    }
}

void game_handle_input_workshop(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_TOUCH) {
        if (skills_handle_touch(touch.px, touch.py)) {
            if (g_game.core_hp <= 0) {
                g_game.core_hp = g_game.core_max_hp;
            }
            game_reset_to_prep();
        }
    }
}

void calibration_init(void) {
    int cur_sel = g_calibration.selected_row;
    int cur_map = g_calibration.selected_map;
    if (cur_map < 0 || cur_map >= 3) cur_map = 0;
    g_calibration.selected_map = cur_map;
    g_calibration.enemy_count = 12;
    g_calibration.enemy_hp = 15;
    g_calibration.enemy_speed_int = 8;  // 0.8 px/frame
    g_calibration.spawn_delay = 40;     // 40 frames
    g_calibration.turret_damage = 5;
    g_calibration.turret_fire_rate = 8; // 8 frames
    g_calibration.cone_spread = 35;     // 35 deg
    g_calibration.sweep_speed = 1;      // 1 deg/frame
    g_calibration.turret_range = 65;    // 65 px
    g_calibration.starting_scrap = 150; // 150 $
    g_calibration.core_lives = 10;      // 10 HP
    g_calibration.selected_row = cur_sel;
}

static void modify_param(int row, int delta) {
    switch (row) {
        case 0:
            g_calibration.selected_map = (g_calibration.selected_map + delta + 3) % 3;
            map_select(g_calibration.selected_map);
            break;
        case 1:
            g_calibration.enemy_count += delta * 1;
            if (g_calibration.enemy_count < 1) g_calibration.enemy_count = 1;
            if (g_calibration.enemy_count > 60) g_calibration.enemy_count = 60;
            break;
        case 2:
            g_calibration.enemy_hp += delta * 2;
            if (g_calibration.enemy_hp < 1) g_calibration.enemy_hp = 1;
            if (g_calibration.enemy_hp > 100) g_calibration.enemy_hp = 100;
            break;
        case 3:
            g_calibration.enemy_speed_int += delta * 1;
            if (g_calibration.enemy_speed_int < 3) g_calibration.enemy_speed_int = 3;
            if (g_calibration.enemy_speed_int > 25) g_calibration.enemy_speed_int = 25;
            break;
        case 4:
            g_calibration.spawn_delay += delta * 5;
            if (g_calibration.spawn_delay < 5) g_calibration.spawn_delay = 5;
            if (g_calibration.spawn_delay > 120) g_calibration.spawn_delay = 120;
            break;
        case 5:
            g_calibration.turret_damage += delta * 1;
            if (g_calibration.turret_damage < 1) g_calibration.turret_damage = 1;
            if (g_calibration.turret_damage > 50) g_calibration.turret_damage = 50;
            break;
        case 6:
            g_calibration.turret_fire_rate += delta * 1;
            if (g_calibration.turret_fire_rate < 3) g_calibration.turret_fire_rate = 3;
            if (g_calibration.turret_fire_rate > 30) g_calibration.turret_fire_rate = 30;
            break;
        case 7:
            g_calibration.cone_spread += delta * 5;
            if (g_calibration.cone_spread < 10) g_calibration.cone_spread = 10;
            if (g_calibration.cone_spread > 120) g_calibration.cone_spread = 120;
            break;
        case 8:
            g_calibration.sweep_speed += delta * 1;
            if (g_calibration.sweep_speed < 1) g_calibration.sweep_speed = 1;
            if (g_calibration.sweep_speed > 6) g_calibration.sweep_speed = 6;
            break;
        case 9:
            g_calibration.turret_range += delta * 5;
            if (g_calibration.turret_range < 30) g_calibration.turret_range = 30;
            if (g_calibration.turret_range > 120) g_calibration.turret_range = 120;
            break;
        case 10:
            g_calibration.starting_scrap += delta * 25;
            if (g_calibration.starting_scrap < 0) g_calibration.starting_scrap = 0;
            if (g_calibration.starting_scrap > 999) g_calibration.starting_scrap = 999;
            break;
        case 11:
            g_calibration.core_lives += delta * 1;
            if (g_calibration.core_lives < 1) g_calibration.core_lives = 1;
            if (g_calibration.core_lives > 50) g_calibration.core_lives = 50;
            break;
    }
}

void calibration_apply_settings(void) {
    g_game.core_hp = g_calibration.core_lives;
    g_game.core_max_hp = g_calibration.core_lives;
    g_game.scrap = g_calibration.starting_scrap;

    int base_interval = (g_calibration.turret_fire_rate > 0) ? g_calibration.turret_fire_rate : 8;
    int interval = base_interval - g_skill_tree.bonus_firerate;
    if (interval < 3) interval = 3;

    int half_angle_units = (g_calibration.cone_spread > 0) ? ((g_calibration.cone_spread * 64) / 90) : 16;
    if (half_angle_units < 4) half_angle_units = 4;

    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].fire_interval = interval;
        g_turrets[t].sweep_amplitude = half_angle_units;
        g_turrets[t].sweep_speed = (g_calibration.sweep_speed > 0) ? g_calibration.sweep_speed : 1;
        g_turrets[t].range = (g_calibration.turret_range > 0) ? g_calibration.turret_range : 75;
    }
}

void calibration_apply_and_start(void) {
    calibration_apply_settings();
    game_start_wave();
}

void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_UP) {
        g_calibration.selected_row = (g_calibration.selected_row + CALIBRATION_ROWS - 1) % CALIBRATION_ROWS;
    }
    if (keys_down & KEY_DOWN) {
        g_calibration.selected_row = (g_calibration.selected_row + 1) % CALIBRATION_ROWS;
    }
    if (keys_down & KEY_LEFT) {
        modify_param(g_calibration.selected_row, -1);
    }
    if (keys_down & KEY_RIGHT) {
        modify_param(g_calibration.selected_row, 1);
    }
    if (keys_down & KEY_Y) {
        calibration_init();
    }
    if (keys_down & (KEY_A | KEY_START)) {
        calibration_apply_and_start();
        return;
    }
    if (keys_down & KEY_B) {
        calibration_apply_settings();
        g_game.mode = MODE_PREPARATION;
        return;
    }

    if (keys_down & KEY_TOUCH) {
        // Header [VOLVER]: (x: 202..252, y: 2..15)
        if (touch.px >= 200 && touch.px <= 254 && touch.py >= 2 && touch.py <= 16) {
            calibration_apply_settings();
            g_game.mode = MODE_PREPARATION;
            return;
        }

        // Bottom [RESET (Y)]: (x: 6..82, y: 166..190)
        if (touch.px >= 6 && touch.px <= 82 && touch.py >= 166 && touch.py <= 190) {
            calibration_init();
            return;
        }

        // Bottom [PROBAR PARTIDA (A)]: (x: 86..252, y: 166..190)
        if (touch.px >= 86 && touch.px <= 252 && touch.py >= 166 && touch.py <= 190) {
            calibration_apply_and_start();
            return;
        }

        // Calibration rows: (y: 20 + i * 11)
        for (int i = 0; i < CALIBRATION_ROWS; i++) {
            int ry = 20 + i * 11;
            if (touch.py >= ry - 1 && touch.py <= ry + 10) {
                g_calibration.selected_row = i;
                // [-] button: x: 104..128
                if (touch.px >= 104 && touch.px <= 128) {
                    modify_param(i, -1);
                }
                // [+] button: x: 184..210
                else if (touch.px >= 184 && touch.px <= 210) {
                    modify_param(i, 1);
                }
                break;
            }
        }
    }
}
