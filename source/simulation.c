#include "game.h"
#include "enemy_data.h"
#include "tiles.h"

GameContext g_game;
Turret g_turrets[MAX_TURRETS];
Enemy g_enemies[MAX_ENEMIES];
Bullet g_bullets[MAX_BULLETS];
Splatter g_splatters[MAX_SPLATTERS];
DeathParticle g_death_particles[MAX_DEATH_PARTICLES];
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

void game_add_splatter_ex(int x, int y, uint16_t color, int size, int duration) {
    if (x < 2 || x >= SCREEN_W - 2 || y < 2 || y >= SCREEN_H - 2) return;

    // Find first empty slot or oldest slot
    int best_slot = -1;
    int min_life = 999999;
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life <= 0) {
            best_slot = i;
            break;
        }
        if (g_splatters[i].life < min_life) {
            min_life = g_splatters[i].life;
            best_slot = i;
        }
    }

    if (best_slot >= 0) {
        g_splatters[best_slot].x = x;
        g_splatters[best_slot].y = y;
        g_splatters[best_slot].life = duration;
        g_splatters[best_slot].max_life = duration;
        g_splatters[best_slot].size = size;
        g_splatters[best_slot].color = color;
    }
}

void game_add_splatter(int x, int y, uint16_t color) {
    game_add_splatter_ex(x, y, color, 1, 180);
}

void game_spawn_death_gore(int x, int y, int bvx, int bvy, int variant) {
    // 1. Determine explosion characteristics by enemy tier
    int particle_count = 6;
    int burst_speed = 3;  // base velocity magnitude
    int gore_spread = 6;  // radius for initial puddle
    int puddle_drops = 2;

    switch (variant) {
        case 0: // T0: Larva (4x4) - Tiny pop
            particle_count = 5;
            burst_speed = 3;
            gore_spread = 5;
            puddle_drops = 2;
            break;
        case 1: // T1: Ripper (6x5) - Small spray
            particle_count = 8;
            burst_speed = 4;
            gore_spread = 8;
            puddle_drops = 3;
            break;
        case 2: // T2: Hormagaunt (9x9) - Medium bloody burst
            particle_count = 14;
            burst_speed = 5;
            gore_spread = 12;
            puddle_drops = 5;
            break;
        case 3: // T3: Ravener (15x11) - Large violent rupture
            particle_count = 32;
            burst_speed = 7;
            gore_spread = 20;
            puddle_drops = 10;
            break;
        case 4: // T4: Carnifex (21x21) - Massive heavy explosion
            particle_count = 64;
            burst_speed = 10;
            gore_spread = 32;
            puddle_drops = 22;
            break;
        case 5: // T5: Hierophant (30x30) - Colossal bio-cataclysm
            particle_count = 96;
            burst_speed = 12;
            gore_spread = 45;
            puddle_drops = 32;
            break;
        default:
            particle_count = 14;
            burst_speed = 5;
            gore_spread = 12;
            puddle_drops = 6;
            break;
    }

    // 2. Primary blood colors according to canonical Xenos lore
    uint16_t col_primary = (variant == 0 || variant == 3) ? COLOR_XENOS_ICHOR : COLOR_BLOOD_DARK;
    uint16_t col_secondary = COLOR_XENOS_FLESH;
    uint16_t col_chitin = COLOR_XENOS_CHITIN;

    // 3. Deposit immediate core blood puddles on the ground
    // Center dense puddle
    int puddle_size = (variant >= 4) ? 2 : ((variant >= 2) ? 1 : 0);
    game_add_splatter_ex(x, y, col_primary, puddle_size, 240 + (rand() % 60));
    if (variant >= 4) {
        // Extra dense satellite core pools for colossal bio-titans
        game_add_splatter_ex(x - 5, y - 3, col_secondary, 2, 240 + (rand() % 60));
        game_add_splatter_ex(x + 5, y + 3, col_primary, 2, 240 + (rand() % 60));
        game_add_splatter_ex(x + 2, y - 5, col_chitin, 1, 240 + (rand() % 60));
        game_add_splatter_ex(x - 2, y + 5, col_primary, 1, 240 + (rand() % 60));
    }

    // Satellite splatter drops
    for (int d = 0; d < puddle_drops; d++) {
        int ox = (rand() % (gore_spread * 2 + 1)) - gore_spread;
        int oy = (rand() % (gore_spread * 2 + 1)) - gore_spread;
        uint16_t c = (rand() % 3 == 0) ? col_secondary : col_primary;
        int sz = (rand() % 4 == 0 && variant >= 3) ? 1 : 0;
        int dur = 180 + (rand() % 120);
        game_add_splatter_ex(x + ox, y + oy, c, sz, dur);
    }

    // 4. Spawn airborne pseudo-3D ballistic particles
    int spawned = 0;
    int bullet_dir_bias_x = bvx / 3; // momentum transfer from projectile
    int bullet_dir_bias_y = bvy / 3;

    for (int i = 0; i < MAX_DEATH_PARTICLES && spawned < particle_count; i++) {
        if (!g_death_particles[i].active) {
            g_death_particles[i].active = 1;
            g_death_particles[i].x = TO_FP(x) + ((rand() % 9 - 4) << FP_SHIFT);
            g_death_particles[i].y = TO_FP(y) + ((rand() % 9 - 4) << FP_SHIFT);
            // Starting height Z (Q8) based on creature size
            g_death_particles[i].z = TO_FP(4 + (variant * 3));

            // Radial burst velocities
            int ang = rand() % 256;
            int spd = (rand() % (burst_speed * 180)) + TO_FP(2);
            g_death_particles[i].vx = ((fixed_cos(ang) * spd) >> FP_SHIFT) + bullet_dir_bias_x;
            g_death_particles[i].vy = ((fixed_sin(ang) * spd) >> FP_SHIFT) + bullet_dir_bias_y;
            // Vertical upward ejection velocity
            g_death_particles[i].vz = TO_FP(2) + (rand() % (TO_FP(burst_speed) + TO_FP(2)));

            g_death_particles[i].life = 45;

            // Particle type / color / size
            int roll = rand() % 100;
            if (roll < 45) {
                // Liquid blood / ichor drop
                g_death_particles[i].color = col_primary;
                g_death_particles[i].size = (variant >= 3 && (rand() % 2 == 0)) ? 1 : 0;
            } else if (roll < 75) {
                // Bioluminescent flesh
                g_death_particles[i].color = col_secondary;
                g_death_particles[i].size = (variant >= 3) ? 1 : 0;
            } else {
                // Hard chitin / exoskeleton shrapnel
                g_death_particles[i].color = col_chitin;
                g_death_particles[i].size = (variant >= 2 && (rand() % 2 == 0)) ? 1 : 0;
            }

            spawned++;
        }
    }
}

void game_init(void) {
    memset(&g_game, 0, sizeof(g_game));
    memset(g_turrets, 0, sizeof(g_turrets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_splatters, 0, sizeof(g_splatters));
    memset(g_death_particles, 0, sizeof(g_death_particles));

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
    memset(g_death_particles, 0, sizeof(g_death_particles));

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
            // Scale HP proportionally by caste: Larva (4), Ripper (12), Hormagaunt (25), Ravener (50), Carnifex (100), Hierophant (200)
            static const int s_tier_hp_base[ENEMY_VARIANT_COUNT] = { 4, 10, 22, 45, 90, 180 };
            int calib_mult = (g_calibration.enemy_hp > 0) ? g_calibration.enemy_hp : 15;
            g_enemies[i].hp = (s_tier_hp_base[g_enemies[i].variant] * calib_mult) / 15;

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

    // 2b. Update Airborne Ballistic Death Particles (Pseudo-3D)
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;

        // Apply horizontal velocities and air friction
        g_death_particles[i].x += g_death_particles[i].vx;
        g_death_particles[i].y += g_death_particles[i].vy;
        g_death_particles[i].vx = (g_death_particles[i].vx * 31) / 32;
        g_death_particles[i].vy = (g_death_particles[i].vy * 31) / 32;

        // Vertical pseudo-3D ballistic flight with gravity
        g_death_particles[i].z += g_death_particles[i].vz;
        g_death_particles[i].vz -= (FP_ONE / 8); // Gravity: 0.125 px/frame^2 (floatier, more visible trajectory)

        g_death_particles[i].life--;

        // Ground collision (Z <= 0) or timeout
        if (g_death_particles[i].z <= 0 || g_death_particles[i].life <= 0) {
            int px = FROM_FP(g_death_particles[i].x);
            int py = FROM_FP(g_death_particles[i].y);
            // On ground impact, deposit a lasting blood droplet / giblet on the deck!
            if (px >= 2 && px < SCREEN_W - 2 && py >= 2 && py < SCREEN_H - 2) {
                int dur = 150 + (rand() % 120);
                game_add_splatter_ex(px, py, g_death_particles[i].color, g_death_particles[i].size, dur);
            }
            g_death_particles[i].active = 0;
        }
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
            game_spawn_death_gore(FROM_FP(g_enemies[i].x), FROM_FP(g_enemies[i].y), 0, 0, g_enemies[i].variant);
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

            // Hit check scaled to enemy hitbox
            static const int s_hit_radius[ENEMY_VARIANT_COUNT] = { 4, 5, 7, 9, 13, 16 };
            int hit_r = s_hit_radius[g_enemies[e].variant];
            if (abs(bx - ex) <= hit_r && abs(by - ey) <= hit_r) {
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

                    // Spectacular visceral death gore burst scaled by enemy tier
                    game_spawn_death_gore(ex, ey, g_bullets[b].vx, g_bullets[b].vy, g_enemies[e].variant);
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
