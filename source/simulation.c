#include "game.h"
#include "enemy_data.h"

GameContext g_game;
Turret g_turret;
Enemy g_enemies[MAX_ENEMIES];
Bullet g_bullets[MAX_BULLETS];
Splatter g_splatters[MAX_SPLATTERS];

const Waypoint g_waypoints[MAX_WAYPOINTS] = {
    {0, 96},
    {64, 96},
    {128, 96},
    {192, 96},
    {240, 96},
    {256, 96}
};

int game_is_pos_valid(int x, int y) {
    // Continuous pixel placement: valid anywhere on screen within usable bounds
    // Screen is 256x192. Turret base has a radius of 8 px.
    if (x < 10 || x > 245) return 0;

    // Must not overlap Top Command Panel (y: 0..18) or Bottom Dock (y: 168..191)
    if (y < 27 || y > 159) return 0;

    // Highway exclusion zone: Road surface spans y: 80..111 where xenos move.
    // Turret base (radius 8px) cannot overlap the road:
    if (y > 71 && y < 120) return 0;

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
    memset(&g_turret, 0, sizeof(g_turret));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_splatters, 0, sizeof(g_splatters));

    g_game.mode = MODE_PREPARATION;
    g_game.wave_number = 1;
    g_game.core_hp = 20;
    g_game.core_max_hp = 20;
    g_game.scrap = 25; // Initial Mechanicus Tithe Grant
    g_game.fast_forward = 1;
    g_game.turret_dock_count = 1;
    g_game.selected_turret = -1;

    // Heavy Bolter Default Profile
    g_turret.center_angle = 64; // Aim straight down into middle trench
    g_turret.current_angle = 64;
    g_turret.sweep_amplitude = 16; // 45 deg total
    g_turret.sweep_speed = 1;
    g_turret.sweep_dir = 1;
    g_turret.fire_interval = 8;
    g_turret.range = 75;
    g_turret.placed = 0;
    g_turret.active = 0;
    g_turret.flash_timer = 0;
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

    g_turret.shots_fired = 0;
    g_turret.hits_confirmed = 0;
    g_turret.wasted_shots = 0;
    g_turret.damage_dealt = 0;
    g_turret.fire_cooldown = 0;
    g_turret.flash_timer = 0;

    // Apply Mechanicus Skills from Branching Tree
    int interval = 8 - g_skill_tree.bonus_firerate;
    if (interval < 3) interval = 3;
    g_turret.fire_interval = interval;
    g_turret.sweep_speed = 1;
}

void game_start_wave(void) {
    if (!g_turret.placed) {
        g_turret.x = 100;
        g_turret.y = 136; // In open plaza south of highway
        g_turret.center_angle = 192; // Aim upwards towards highway
        g_turret.current_angle = 192;
        g_turret.placed = 1;
        g_turret.active = 1;
        g_game.turret_dock_count = 0;
    }

    game_reset_to_prep();
    g_game.mode = MODE_WAVE;
    g_game.selected_turret = -1;
}

static void spawn_enemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            g_enemies[i].active = 1;
            g_enemies[i].waypoint_idx = 0;

            // Wide Swarm Dispersion across 32px highway: (-10 to +10 px)
            int offset = ((rand() % 21) - 10);
            g_enemies[i].lateral_offset = TO_FP(offset);
            
            // Speed jitter (0.85 to 1.15 px/frame)
            g_enemies[i].speed = FP_ONE + ((rand() % 61) - 30);
            g_enemies[i].variant = rand() % ENEMY_VARIANT_COUNT;
            g_enemies[i].hp = g_enemy_types[g_enemies[i].variant].default_hp;

            // Initial position (WP0 is horizontal, so lateral offset is vertical)
            g_enemies[i].x = TO_FP(g_waypoints[0].x);
            g_enemies[i].y = TO_FP(g_waypoints[0].y) + g_enemies[i].lateral_offset;

            g_game.enemies_spawned++;
            g_game.enemies_alive++;
            break;
        }
    }
}

static void spawn_bullet(int x, int y, int angle) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) {
            g_bullets[i].active = 1;
            g_bullets[i].x = TO_FP(x);
            g_bullets[i].y = TO_FP(y);
            // Speed = 4 pixels per frame
            g_bullets[i].vx = (fixed_cos(angle) * 4);
            g_bullets[i].vy = (fixed_sin(angle) * 4);
            g_bullets[i].life = (g_turret.range / 4) + 2;
            break;
        }
    }
}

void game_update_simulation(void) {
    if (g_game.mode != MODE_WAVE) return;

    g_game.sim_ticks_elapsed++;

    // 1. Spawning
    if (g_game.enemies_spawned < TOTAL_WAVE_ENEMIES) {
        g_game.spawn_timer++;
        if (g_game.spawn_timer >= 10) {
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
        if (next_wp >= MAX_WAYPOINTS) {
            // Reached Bunker Core!
            g_enemies[i].active = 0;
            g_game.enemies_alive--;
            g_game.enemies_breached++;
            g_game.core_hp--;
            game_add_splatter(FROM_FP(g_enemies[i].x), FROM_FP(g_enemies[i].y), COLOR_BLOOD_DARK);
            if (g_game.core_hp <= 0) {
                g_game.core_hp = 0;
                g_game.mode = MODE_WORKSHOP;
                return;
            }
            continue;
        }

        int x0 = g_waypoints[curr_wp].x;
        int x1 = g_waypoints[next_wp].x, y1 = g_waypoints[next_wp].y;

        int target_x, target_y;
        if (x0 == x1) {
            // Vertical corridor: lateral offset applies horizontally
            target_x = TO_FP(x1) + g_enemies[i].lateral_offset;
            target_y = TO_FP(y1);
        } else {
            // Horizontal corridor: lateral offset applies vertically
            target_x = TO_FP(x1);
            target_y = TO_FP(y1) + g_enemies[i].lateral_offset;
        }

        int dx = target_x - g_enemies[i].x;
        int dy = target_y - g_enemies[i].y;
        int spd = g_enemies[i].speed;

        if (abs(dy) > abs(dx)) {
            g_enemies[i].dir = (dy > 0) ? 1 : 3; // 1 = South, 3 = North
        } else {
            g_enemies[i].dir = (dx > 0) ? 0 : 2; // 0 = East, 2 = West
        }
        g_enemies[i].anim_frame = (g_game.sim_ticks_elapsed / 6 + i) % 4;

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

    // 4. Update Heavy Bolter Turret
    if (g_turret.placed && g_turret.active) {
        if (g_turret.flash_timer > 0) g_turret.flash_timer--;
        if (g_turret.barrel_recoil_l > 0) g_turret.barrel_recoil_l--;
        if (g_turret.barrel_recoil_r > 0) g_turret.barrel_recoil_r--;

        g_turret.current_angle += g_turret.sweep_dir * g_turret.sweep_speed;

        int min_ang = g_turret.center_angle - g_turret.sweep_amplitude;
        int max_ang = g_turret.center_angle + g_turret.sweep_amplitude;

        if (g_turret.current_angle >= max_ang) {
            g_turret.current_angle = max_ang;
            g_turret.sweep_dir = -1;
        } else if (g_turret.current_angle <= min_ang) {
            g_turret.current_angle = min_ang;
            g_turret.sweep_dir = 1;
        }

        if (g_turret.fire_cooldown > 0) {
            g_turret.fire_cooldown--;
        } else {
            g_turret.fire_cooldown = g_turret.fire_interval;
            g_turret.flash_timer = 2;
            g_turret.last_barrel = 1 - g_turret.last_barrel;
            // Explosive snap back: set timer to 4
            if (g_turret.last_barrel == 0) {
                g_turret.barrel_recoil_l = 4;
            } else {
                g_turret.barrel_recoil_r = 4;
            }

            int ang = g_turret.current_angle & 0xFF;
            int perp_x = -fixed_sin(ang);
            int perp_y = fixed_cos(ang);
            int s = (g_turret.last_barrel == 0) ? -3 : 3;
            int bx = g_turret.x + ((perp_x * s) >> FP_SHIFT);
            int by = g_turret.y + ((perp_y * s) >> FP_SHIFT);

            spawn_bullet(bx, by, ang);
            g_turret.shots_fired++;
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
                int dmg = 1 + g_skill_tree.bonus_damage;
                g_enemies[e].hp -= dmg;
                g_turret.damage_dealt += dmg;
                if (g_enemies[e].hp <= 0) {
                    g_enemies[e].active = 0;
                    g_game.enemies_alive--;
                    g_game.enemies_killed++;
                    int reward = g_enemy_types[g_enemies[e].variant].scrap_value + (g_skill_tree.bonus_ap > 0 ? 1 : 0);
                    g_game.scrap += reward;

                    // Xenos ichor / blood splatter
                    uint16_t splat_col = (g_enemies[e].variant == 0 || g_enemies[e].variant == 3) ? COLOR_XENOS_ICHOR : COLOR_BLOOD_DARK;
                    game_add_splatter(ex, ey, splat_col);
                }
                g_turret.hits_confirmed++;
                g_bullets[b].active = 0;
                break;
            }
        }

        if (!hit) {
            if (g_bullets[b].life <= 0 || bx < 0 || bx >= SCREEN_W || by < 0 || by >= SCREEN_H) {
                g_turret.wasted_shots++;
                g_bullets[b].active = 0;
            }
        }
    }

    // 6. Wave Completion
    if (g_game.enemies_spawned >= TOTAL_WAVE_ENEMIES && g_game.enemies_alive == 0) {
        g_game.scrap += 25;
        g_game.wave_number++;
        g_game.mode = MODE_WORKSHOP;
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
        if (g_game.selected_turret >= 0 && g_turret.placed) {
            if (touch.px >= 56 && touch.px <= 104 && touch.py >= 2 && touch.py <= 16) {
                g_turret.placed = 0;
                g_turret.active = 0;
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

        // Dock button touch: open FORGE STC (Branching Skill Tree)
        if (touch.px >= 160 && touch.px <= 250 && touch.py >= 168) {
            g_game.mode = MODE_WORKSHOP;
            return;
        }

        // Touch on placed turret: select it
        if (g_turret.placed) {
            int d = abs(touch.px - g_turret.x) + abs(touch.py - g_turret.y);
            if (d <= 16) {
                g_game.selected_turret = 0;
                return;
            }
        }

        // If turret selected and touch is outside: orient angle
        if (g_game.selected_turret >= 0 && g_turret.placed && touch.py > 20 && touch.py < 165) {
            int dy = touch.py - g_turret.y;
            int dx = touch.px - g_turret.x;
            g_turret.center_angle = fixed_atan2(dy, dx);
            g_turret.current_angle = g_turret.center_angle;
            return;
        }

        g_game.selected_turret = -1;
    }

    if (keys_held & KEY_TOUCH) {
        if (g_game.is_dragging_new) {
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
        } else if (g_game.selected_turret >= 0 && g_turret.placed) {
            int dy = touch.py - g_turret.y;
            int dx = touch.px - g_turret.x;
            if (abs(dy) + abs(dx) > 10) {
                g_turret.center_angle = fixed_atan2(dy, dx);
                g_turret.current_angle = g_turret.center_angle;
            }
        }
    } else {
        if (g_game.is_dragging_new) {
            if (game_is_pos_valid(g_game.drag_x, g_game.drag_y)) {
                g_turret.x = g_game.drag_x;
                g_turret.y = g_game.drag_y;
                g_turret.placed = 1;
                g_turret.active = 1;
                g_game.turret_dock_count--;
                g_game.selected_turret = 0;
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
