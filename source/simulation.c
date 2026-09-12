#include "game.h"
#include "enemy_data.h"
#include "tiles.h"
#include "turret_data.h"

GameContext g_game;
Turret g_turrets[MAX_TURRETS];
Enemy g_enemies[MAX_ENEMIES];
Bullet g_bullets[MAX_BULLETS];
Splatter g_splatters[MAX_SPLATTERS];
DeathParticle g_death_particles[MAX_DEATH_PARTICLES];

// Wave enemy distribution table (20 waves)
typedef struct {
    int total_enemies;
    int spawn_interval; // frames between spawns
    int primary_variant;
    int secondary_variant;
    int secondary_ratio; // 0..10
    uint64_t hp_base;
    uint64_t scrap_base;
} WaveDef;

static const WaveDef s_wave_table[20] = {
    // W1..W5: Early clicker & manual tutorial
    { 14, 75, 0, 0, 0, 6, 1 },              // W1: 14 Larvas (contacto)
    { 20, 60, 0, 0, 0, 8, 1 },              // W2: 20 Larvas
    { 28, 48, 0, 1, 3, 14, 2 },             // W3: Larvas + Rippers
    { 38, 38, 1, 0, 2, 24, 4 },             // W4: Rippers veloces
    { 65, 24, 0, 1, 5, 36, 6 },             // W5: Mini-Horda They Are Billions

    // W6..W10: Hormagaunts & Logistics
    { 50, 32, 1, 2, 4, 60, 10 },            // W6: Entra T2 con 1 Armadura
    { 60, 28, 2, 1, 3, 100, 15 },           // W7: T2 dominante
    { 85, 20, 0, 2, 4, 180, 25 },           // W8: Escala 1.5K
    { 75, 22, 2, 3, 3, 300, 45 },           // W9: Entra T3 Ravener
    { 130, 14, 1, 2, 5, 550, 70 },          // W10: Gran Horda Sectorial

    // W11..W15: Heavy Armor & Factorio Automation
    { 80, 20, 3, 2, 4, 1200, 150 },         // W11: Raveners pesados
    { 110, 15, 2, 3, 5, 2400, 300 },        // W12: Escala 15K
    { 150, 12, 1, 3, 4, 4800, 500 },        // W13: Enjambre masivo
    { 140, 12, 3, 2, 6, 9500, 800 },        // W14: Asedio de choque
    { 180, 10, 2, 4, 3, 18000, 1500 },      // W15: Asedio Carnifex

    // W16..W20: Millions Cataclysm
    { 70, 24, 4, 3, 5, 45000, 4000 },       // W16: Tanques Carnifex
    { 160, 10, 3, 4, 4, 90000, 8000 },      // W17: Horda mixta blindada
    { 220, 8, 2, 4, 5, 180000, 15000 },     // W18: Salto a Millones
    { 200, 8, 4, 5, 2, 400000, 35000 },     // W19: Vanguardia Titánica
    { 320, 5, 3, 5, 4, 1000000, 80000 }     // W20: Cataclismo Final
};

int game_is_pos_valid(int x, int y) {
    // Valid deployment area in bottom screen (local y: 20..150, x: 20..236)
    if (x < 24 || x > 232) return 0;
    if (y < 24 || y > 146) return 0;

    // Do not overlap central bunker area (x: 96..160, y >= 148)
    if (x >= 90 && x <= 166 && y >= 144) return 0;

    // Check distance to other placed turrets (min 32px separation)
    for (int t = 0; t < MAX_TURRETS; t++) {
        if (g_turrets[t].placed) {
            int dx = x - g_turrets[t].x;
            int dy = y - g_turrets[t].y;
            if (dx * dx + dy * dy < (32 * 32)) return 0;
        }
    }

    return 1;
}

void game_add_splatter_ex(int x, int y, uint16_t color, int size, int duration) {
    if (x < 2 || x >= SCREEN_W - 2 || y < 2 || y >= FIELD_H - 2) return;

    int best_slot = -1;
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life <= 0) {
            best_slot = i;
            break;
        }
    }
    if (best_slot < 0) best_slot = rand() % MAX_SPLATTERS;

    g_splatters[best_slot].x = x;
    g_splatters[best_slot].y = y;
    g_splatters[best_slot].color = color;
    g_splatters[best_slot].size = size;
    g_splatters[best_slot].life = duration;
    g_splatters[best_slot].max_life = duration;
}

void game_spawn_death_gore(int x, int y, int bvx, int bvy, int variant) {
    if (variant < 0 || variant >= ENEMY_VARIANT_COUNT) variant = 0;

    int particle_count = 10 + variant * 8;
    uint16_t col_primary = (variant == 0 || variant == 3) ? COLOR_XENOS_ICHOR : COLOR_BLOOD_DARK;
    uint16_t col_secondary = COLOR_XENOS_FLESH;

    // Ground puddles
    game_add_splatter_ex(x, y, col_primary, (variant >= 2 ? 1 : 0), 400 + (rand() % 100));
    if (variant >= 1) {
        game_add_splatter_ex(x - 2, y + 1, col_secondary, 0, 350 + (rand() % 100));
        game_add_splatter_ex(x + 2, y - 1, col_primary, 0, 350 + (rand() % 100));
    }

    // Airborne ballistic particles
    int spawned = 0;
    for (int i = 0; i < MAX_DEATH_PARTICLES && spawned < particle_count; i++) {
        if (!g_death_particles[i].active) {
            g_death_particles[i].active = 1;
            g_death_particles[i].x = TO_FP(x) + ((rand() % 7 - 3) << FP_SHIFT);
            g_death_particles[i].y = TO_FP(y) + ((rand() % 7 - 3) << FP_SHIFT);
            g_death_particles[i].z = TO_FP(3 + variant * 2);

            int ang = rand() % 256;
            int spd = (rand() % TO_FP(3)) + TO_FP(1);
            g_death_particles[i].vx = ((fixed_cos(ang) * spd) >> FP_SHIFT) + (bvx / 10);
            g_death_particles[i].vy = ((fixed_sin(ang) * spd) >> FP_SHIFT) + (bvy / 10);
            g_death_particles[i].vz = TO_FP(1) + (rand() % TO_FP(3));
            g_death_particles[i].life = 35;
            g_death_particles[i].color = (rand() % 2 == 0) ? col_primary : col_secondary;
            g_death_particles[i].size = (variant >= 3 && (rand() % 2 == 0)) ? 1 : 0;
            spawned++;
        }
    }
}

static void spawn_enemy(int variant, uint64_t hp) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            g_enemies[i].active = 1;
            g_enemies[i].variant = variant;
            g_enemies[i].hp = hp;
            g_enemies[i].max_hp = hp;

            // Spawn at top edge (y = 0) with random x spread (16..240)
            int sx = 16 + (rand() % 224);
            g_enemies[i].x = TO_FP(sx);
            g_enemies[i].y = 0;

            // Base speed by caste: Larva (35), Ripper (45), Hormag (55), Ravener (28), Carnifex (18)
            static const int s_speeds[ENEMY_VARIANT_COUNT] = {
                160, 200, 240, 130, 90, 60
            };
            int spd = s_speeds[variant] + ((rand() % 21) - 10);
            g_enemies[i].speed = spd;

            g_enemies[i].dir = 1; // South
            g_enemies[i].anim_frame = 0;
            g_enemies[i].biting_target = -1;
            g_enemies[i].bite_timer = 0;

            g_game.enemies_spawned++;
            g_game.enemies_alive++;
            break;
        }
    }
}

static void spawn_bullet(int x, int y, int angle, int turret_idx, uint64_t dmg) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) {
            g_bullets[i].active = 1;
            g_bullets[i].turret_idx = turret_idx;
            g_bullets[i].x = TO_FP(x);
            g_bullets[i].y = TO_FP(y);
            // 5 px/frame bullet velocity
            g_bullets[i].vx = (fixed_cos(angle) * 5);
            g_bullets[i].vy = (fixed_sin(angle) * 5);
            g_bullets[i].life = 45;
            g_bullets[i].damage = dmg;
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
    memset(g_death_particles, 0, sizeof(g_death_particles));

    g_game.mode = MODE_PREPARATION;
    g_game.wave_number = 1;
    g_game.total_waves = 20;
    g_game.bunker_hp = 100;
    g_game.bunker_max_hp = 100;
    g_game.scrap = 10;
    g_game.fast_forward = 1;

    // Upgrades initial state
    g_game.upgrades.caliber_lvl = 0;
    g_game.upgrades.firerate_lvl = 0;
    g_game.upgrades.range_lvl = 0;
    g_game.upgrades.mag_size_lvl = 0;
    g_game.upgrades.auto_target = 0; // Starts requiring stylus targeting!
    g_game.upgrades.conveyor_lvl = 0; // Starts requiring manual ammo drag!
    g_game.upgrades.extra_turrets = 0;

    // Initial Turret #0: Deployed right in front of the Sanctum
    g_turrets[0].id = 0;
    g_turrets[0].type = TURRET_TYPE_BOLTER;
    g_turrets[0].x = 128;
    g_turrets[0].y = 124; // Local y on bottom screen
    g_turrets[0].current_angle = 192; // Aiming North (towards oncoming swarm)
    g_turrets[0].target_angle = 192;
    g_turrets[0].range = 65;
    g_turrets[0].placed = 1;
    g_turrets[0].active = 1;
    g_turrets[0].hp = 50;
    g_turrets[0].max_hp = 50;
    g_turrets[0].ammo = 20;
    g_turrets[0].max_ammo = 20;
    g_turrets[0].fire_interval = 18; // ~3 shots per second
    g_turrets[0].fire_cooldown = 0;
    g_turrets[0].locked_enemy_idx = -1;

    for (int t = 1; t < MAX_TURRETS; t++) {
        g_turrets[t].id = t;
        g_turrets[t].current_angle = 192;
        g_turrets[t].target_angle = 192;
        g_turrets[t].range = 65;
        g_turrets[t].placed = 0;
        g_turrets[t].active = 0;
        g_turrets[t].hp = 50;
        g_turrets[t].max_hp = 50;
        g_turrets[t].ammo = 20;
        g_turrets[t].max_ammo = 20;
        g_turrets[t].fire_interval = 18;
        g_turrets[t].locked_enemy_idx = -1;
    }
}

void game_start_wave(void) {
    g_game.mode = MODE_WAVE;
    g_game.wave_timer = 1800; // 30 seconds at 60 FPS
    g_game.enemies_spawned = 0;
    g_game.enemies_alive = 0;
    g_game.spawn_timer = 0;

    int w_idx = g_game.wave_number - 1;
    if (w_idx < 0) w_idx = 0;
    if (w_idx >= 20) w_idx = 19;
    g_game.enemies_to_spawn = s_wave_table[w_idx].total_enemies;

    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_death_particles, 0, sizeof(g_death_particles));

    // Reset locked targets
    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].locked_enemy_idx = -1;
    }
}

void game_reset_to_prep(void) {
    g_game.mode = MODE_PREPARATION;
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_death_particles, 0, sizeof(g_death_particles));

    // Repair turrets back to full between waves
    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].hp = g_turrets[t].max_hp;
        g_turrets[t].locked_enemy_idx = -1;
    }
}

void game_update_simulation(void) {
    if (g_game.mode != MODE_WAVE) return;

    g_game.sim_ticks_elapsed++;
    if (g_game.wave_timer > 0) g_game.wave_timer--;

    int w_idx = g_game.wave_number - 1;
    if (w_idx < 0) w_idx = 0;
    if (w_idx >= 20) w_idx = 19;
    const WaveDef *wdef = &s_wave_table[w_idx];

    // 1. Spawning
    if (g_game.enemies_spawned < g_game.enemies_to_spawn) {
        g_game.spawn_timer++;
        if (g_game.spawn_timer >= wdef->spawn_interval) {
            g_game.spawn_timer = 0;
            int v = wdef->primary_variant;
            if (wdef->secondary_ratio > 0 && (rand() % 10) < wdef->secondary_ratio) {
                v = wdef->secondary_variant;
            }
            spawn_enemy(v, wdef->hp_base);
        }
    }

    // 2. Splatters life
    for (int i = 0; i < MAX_SPLATTERS; i++) {
        if (g_splatters[i].life > 0) g_splatters[i].life--;
    }

    // 3. Death particles
    for (int i = 0; i < MAX_DEATH_PARTICLES; i++) {
        if (!g_death_particles[i].active) continue;
        g_death_particles[i].x += g_death_particles[i].vx;
        g_death_particles[i].y += g_death_particles[i].vy;
        g_death_particles[i].z += g_death_particles[i].vz;
        g_death_particles[i].vz -= (FP_ONE / 8); // Gravity
        g_death_particles[i].life--;

        if (g_death_particles[i].z <= 0 || g_death_particles[i].life <= 0) {
            int px = FROM_FP(g_death_particles[i].x);
            int py = FROM_FP(g_death_particles[i].y);
            if (px >= 2 && px < SCREEN_W - 2 && py >= 2 && py < FIELD_H - 2) {
                game_add_splatter_ex(px, py, g_death_particles[i].color, g_death_particles[i].size, 300);
            }
            g_death_particles[i].active = 0;
        }
    }

    // 4. Update Enemies (Descending vertically and converging towards central bunker at x=128, y=360)
    int target_base_x = TO_FP(128);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;

        int ex = g_enemies[i].x;
        int ey = g_enemies[i].y;
        int px = FROM_FP(ex);
        int py = FROM_FP(ey);

        // Animation frame
        g_enemies[i].anim_frame = (g_game.sim_ticks_elapsed / 8) % 4;

        // Check if hitting any placed turret (physical obstruction & biting!)
        int hitting_turret = -1;
        if (py >= 192) {
            int local_y = py - 192;
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    int tdx = px - g_turrets[t].x;
                    int tdy = local_y - g_turrets[t].y;
                    if (tdx * tdx + tdy * tdy < (14 * 14)) {
                        hitting_turret = t;
                        break;
                    }
                }
            }
        }

        if (hitting_turret >= 0) {
            // Bite Turret!
            g_enemies[i].biting_target = hitting_turret;
            g_enemies[i].bite_timer++;
            if (g_enemies[i].bite_timer >= 45) { // Bite every 0.75s
                g_enemies[i].bite_timer = 0;
                int bite_dmg = 1 + g_enemies[i].variant * 2;
                if (g_turrets[hitting_turret].hp > bite_dmg) {
                    g_turrets[hitting_turret].hp -= bite_dmg;
                } else {
                    // Turret destroyed!
                    g_turrets[hitting_turret].hp = 0;
                    g_turrets[hitting_turret].placed = 0;
                    g_turrets[hitting_turret].active = 0;
                    game_spawn_death_gore(g_turrets[hitting_turret].x, 192 + g_turrets[hitting_turret].y, 0, 0, 4);
                }
            }
            continue; // Stop advancing while biting
        }

        // Check if hitting Bunker Sanctum (x: 96..160, global y >= 348)
        if (py >= 348 && px >= 96 && px <= 160) {
            // Bite Bunker!
            g_enemies[i].biting_target = 99;
            g_enemies[i].bite_timer++;
            if (g_enemies[i].bite_timer >= 40) {
                g_enemies[i].bite_timer = 0;
                uint64_t bite_dmg = 1 + g_enemies[i].variant * 2;
                if (g_game.bunker_hp > bite_dmg) {
                    g_game.bunker_hp -= bite_dmg;
                } else {
                    g_game.bunker_hp = 0;
                    g_game.mode = MODE_GAME_OVER;
                    return;
                }
            }
            continue;
        }

        // Advance: vertical downward + funnel convergence to center
        int spd = (g_enemies[i].speed * FP_ONE) / 60;
        if (spd < 1) spd = 1;

        g_enemies[i].y += spd;

        // Converge X towards 128 as it approaches the bottom screen
        if (py > 120) {
            if (ex < target_base_x) ex += (spd / 4);
            else if (ex > target_base_x) ex -= (spd / 4);
            g_enemies[i].x = ex;
        }
    }

    // 5. Update Turrets & Logistics (Factorio conveyors & reloading)
    for (int t = 0; t < MAX_TURRETS; t++) {
        Turret *tur = &g_turrets[t];
        if (!tur->placed || !tur->active) continue;

        if (tur->flash_timer > 0) tur->flash_timer--;
        if (tur->barrel_recoil_l > 0) tur->barrel_recoil_l--;
        if (tur->barrel_recoil_r > 0) tur->barrel_recoil_r--;

        // Conveyor passive reloading (Branch C)
        if (g_game.upgrades.conveyor_lvl > 0 && tur->ammo < tur->max_ammo) {
            int reload_rate = (g_game.upgrades.conveyor_lvl == 1) ? 60 : 20; // 1/s or 3/s
            if (g_game.sim_ticks_elapsed % reload_rate == 0) {
                tur->ammo++;
            }
        }

        // Target selection
        int target_enemy = -1;

        // Auto-targeting or manual target follow
        if (g_game.upgrades.auto_target) {
            // Find closest active enemy in range
            int closest_dist_sq = tur->range * tur->range;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!g_enemies[e].active) continue;
                int gy = FROM_FP(g_enemies[e].y);
                if (gy < 192) continue; // Only shoot enemies on bottom screen
                int local_y = gy - 192;
                int gx = FROM_FP(g_enemies[e].x);

                int dx = gx - tur->x;
                int dy = local_y - tur->y;
                int dsq = dx * dx + dy * dy;
                if (dsq <= closest_dist_sq) {
                    closest_dist_sq = dsq;
                    target_enemy = e;
                }
            }
        } else {
            // Manual locked enemy from stylus click
            if (tur->locked_enemy_idx >= 0 && g_enemies[tur->locked_enemy_idx].active) {
                int gy = FROM_FP(g_enemies[tur->locked_enemy_idx].y);
                if (gy >= 192) {
                    int local_y = gy - 192;
                    int gx = FROM_FP(g_enemies[tur->locked_enemy_idx].x);
                    int dx = gx - tur->x;
                    int dy = local_y - tur->y;
                    if (dx * dx + dy * dy <= tur->range * tur->range) {
                        target_enemy = tur->locked_enemy_idx;
                    }
                }
            }
        }

        // Aim towards target
        if (target_enemy >= 0) {
            int gx = FROM_FP(g_enemies[target_enemy].x);
            int local_y = FROM_FP(g_enemies[target_enemy].y) - 192;
            tur->target_angle = fixed_atan2(local_y - tur->y, gx - tur->x);
            tur->current_angle = tur->target_angle;

            // Firing
            if (tur->ammo > 0) {
                if (tur->fire_cooldown > 0) {
                    tur->fire_cooldown--;
                } else {
                    int interval = tur->fire_interval - (g_game.upgrades.firerate_lvl * 3);
                    if (interval < 4) interval = 4;
                    tur->fire_cooldown = interval;
                    tur->flash_timer = 3;
                    tur->ammo--;

                    // Alternating barrels recoil snap
                    tur->last_barrel = 1 - tur->last_barrel;
                    if (tur->last_barrel == 0) tur->barrel_recoil_l = 4;
                    else tur->barrel_recoil_r = 4;

                    int ang = tur->current_angle & 0xFF;
                    int perp_x = -fixed_sin(ang);
                    int perp_y = fixed_cos(ang);
                    int s = (tur->last_barrel == 0) ? -3 : 3;
                    int bx = tur->x + ((perp_x * s) >> FP_SHIFT);
                    int by = tur->y + ((perp_y * s) >> FP_SHIFT);

                    uint64_t dmg = 4 + (g_game.upgrades.caliber_lvl * 2);
                    spawn_bullet(bx, by, ang, t, dmg);
                    tur->shots_fired++;
                }
            }
        }
    }

    // 6. Update Bullets & Collisions
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
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue; // Only collide in bottom screen
            int local_y = gy - 192;
            int gx = FROM_FP(g_enemies[e].x);

            static const int s_hit_r[ENEMY_VARIANT_COUNT] = { 5, 6, 8, 11, 15, 18 };
            int r = s_hit_r[g_enemies[e].variant];
            if (abs(bx - gx) <= r && abs(by - local_y) <= r) {
                hit = 1;
                uint64_t dmg = g_bullets[b].damage;
                if (g_enemies[e].hp > dmg) {
                    g_enemies[e].hp -= dmg;
                } else {
                    g_enemies[e].hp = 0;
                    g_enemies[e].active = 0;
                    g_game.enemies_alive--;
                    g_game.enemies_killed++;

                    uint64_t reward = wdef->scrap_base * (1 + g_game.upgrades.bio_harvest_lvl);
                    g_game.scrap += reward;

                    game_spawn_death_gore(gx, gy, g_bullets[b].vx, g_bullets[b].vy, g_enemies[e].variant);
                }

                int tid = g_bullets[b].turret_idx;
                if (tid >= 0 && tid < MAX_TURRETS) {
                    g_turrets[tid].hits_confirmed++;
                    g_turrets[tid].damage_dealt += dmg;
                }
                g_bullets[b].active = 0;
                break;
            }
        }

        if (!hit && (g_bullets[b].life <= 0 || bx < 0 || bx >= SCREEN_W || by < 0 || by >= SCREEN_H)) {
            g_bullets[b].active = 0;
        }
    }

    // 7. Wave Completion
    if (g_game.enemies_spawned >= g_game.enemies_to_spawn && g_game.enemies_alive == 0) {
        g_game.wave_number++;
        if (g_game.wave_number > g_game.total_waves) {
            // Victory loop
            g_game.wave_number = g_game.total_waves;
        }
        game_reset_to_prep();
    }
}

void game_toggle_pause(void) {
    if (g_game.mode == MODE_PAUSED) {
        g_game.mode = (g_game.previous_mode == MODE_PAUSED) ? MODE_WAVE : g_game.previous_mode;
    } else if (g_game.mode == MODE_WAVE) {
        g_game.previous_mode = g_game.mode;
        g_game.mode = MODE_PAUSED;
    }
}

void game_handle_input_prep(touchPosition touch, int keys_down, int keys_held) {
    // Physical button shortcuts: A or START launches the wave!
    if (keys_down & (KEY_START | KEY_A)) {
        game_start_wave();
        return;
    }

    // X or SELECT opens upgrades
    if (keys_down & (KEY_X | KEY_SELECT)) {
        g_game.previous_mode = g_game.mode;
        g_game.mode = MODE_UPGRADES;
        return;
    }

    if (keys_down & KEY_R) {
        g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
    }

    if (keys_down & KEY_TOUCH) {
        // [START] Button: drawn at (190, 150, 60, 36) -> generous hitbox (180..255, 144..191)
        if (touch.px >= 180 && touch.px <= 255 && touch.py >= 144 && touch.py <= 191) {
            game_start_wave();
            return;
        }

        // [UPGRADES] Button: drawn at (60, 156, 50, 24) -> generous hitbox (50..120, 144..191)
        if (touch.px >= 50 && touch.px <= 120 && touch.py >= 144 && touch.py <= 191) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_UPGRADES;
            return;
        }

        // [AMMO DEPOT]: drawn at (6, 150, 44, 36) -> tap to fully reload all placed turrets
        if (touch.px <= 50 && touch.py >= 144) {
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    g_turrets[t].ammo = g_turrets[t].max_ammo;
                }
            }
            return;
        }

        // Turret selection in preparation
        for (int t = 0; t < MAX_TURRETS; t++) {
            if (g_turrets[t].placed) {
                int d = abs(touch.px - g_turrets[t].x) + abs(touch.py - g_turrets[t].y);
                if (d <= 16) {
                    g_game.selected_turret = t;
                    return;
                }
            }
        }
    }
}

void game_handle_input_wave(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_R) {
        g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
    }

    // Touch down: Clicker attack on enemies, start ammo drag, or toggle pause
    if (keys_down & KEY_TOUCH) {
        // [PAUSA] Button in wave HUD: (215..250, 0..14)
        if (touch.px >= 215 && touch.px <= 250 && touch.py <= 14) {
            game_toggle_pause();
            return;
        }

        // Clicker Attack on enemies in bottom screen!
        int clicked_enemy = -1;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue;
            int local_y = gy - 192;
            int gx = FROM_FP(g_enemies[e].x);

            if (abs(touch.px - gx) <= 14 && abs(touch.py - local_y) <= 14) {
                clicked_enemy = e;
                break;
            }
        }

        if (clicked_enemy >= 0) {
            // Stylus clicker damage: 4 base damage (ignoring armor)
            uint64_t click_dmg = 4 + (g_game.upgrades.caliber_lvl * 2);
            if (g_enemies[clicked_enemy].hp > click_dmg) {
                g_enemies[clicked_enemy].hp -= click_dmg;
            } else {
                g_enemies[clicked_enemy].hp = 0;
                g_enemies[clicked_enemy].active = 0;
                g_game.enemies_alive--;
                g_game.enemies_killed++;
                g_game.scrap += 2;
                game_spawn_death_gore(FROM_FP(g_enemies[clicked_enemy].x),
                                     FROM_FP(g_enemies[clicked_enemy].y), 0, 0,
                                     g_enemies[clicked_enemy].variant);
            }

            // Designate as target for all turrets that reach it!
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    g_turrets[t].locked_enemy_idx = clicked_enemy;
                }
            }
            return;
        }

        // Ammo Depot drag start (x: 116..140, y: 150..164)
        if (touch.px >= 116 && touch.px <= 140 && touch.py >= 150 && touch.py <= 164) {
            g_game.is_dragging_ammo = 1;
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
            return;
        }
    }

    if (keys_held & KEY_TOUCH) {
        if (g_game.is_dragging_ammo) {
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
        }
    } else {
        // Release touch: if dragging ammo over a turret, reload it!
        if (g_game.is_dragging_ammo) {
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    int d = abs(g_game.drag_x - g_turrets[t].x) + abs(g_game.drag_y - g_turrets[t].y);
                    if (d <= 20) {
                        g_turrets[t].ammo = g_turrets[t].max_ammo; // Reloaded!
                        break;
                    }
                }
            }
            g_game.is_dragging_ammo = 0;
        }
    }
}

void game_handle_input_pause(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & (KEY_START | KEY_A)) {
        game_toggle_pause();
        return;
    }
    if (keys_down & KEY_TOUCH) {
        // Resume button: x: 70..186, y: 96..116
        if (touch.px >= 70 && touch.px <= 186 && touch.py >= 96 && touch.py <= 116) {
            game_toggle_pause();
            return;
        }
    }
}

void game_handle_input_game_over(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & (KEY_START | KEY_A)) {
        game_init();
        return;
    }
    if (keys_down & KEY_TOUCH) {
        if (touch.px >= 60 && touch.px <= 196 && touch.py >= 106 && touch.py <= 128) {
            game_init();
            return;
        }
    }
}

void game_handle_input_upgrades(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & (KEY_B | KEY_START)) {
        g_game.mode = g_game.previous_mode;
        return;
    }

    if (keys_down & KEY_TOUCH) {
        // Back button: y: 166..186
        if (touch.py >= 166 && touch.py <= 186) {
            g_game.mode = g_game.previous_mode;
            return;
        }

        // Row 1: Calibre (+2 Dmg) - Cost: 25$
        if (touch.px >= 180 && touch.px <= 240 && touch.py >= 26 && touch.py <= 40) {
            if (g_game.scrap >= 25) {
                g_game.scrap -= 25;
                g_game.upgrades.caliber_lvl++;
            }
        }
        // Row 2: Cadencia (+30%) - Cost: 35$
        else if (touch.px >= 180 && touch.px <= 240 && touch.py >= 46 && touch.py <= 60) {
            if (g_game.scrap >= 35) {
                g_game.scrap -= 35;
                g_game.upgrades.firerate_lvl++;
            }
        }
        // Row 3: Bio-Cosecha (x2 $) - Cost: 40$
        else if (touch.px >= 180 && touch.px <= 240 && touch.py >= 66 && touch.py <= 80) {
            if (g_game.scrap >= 40) {
                g_game.scrap -= 40;
                g_game.upgrades.bio_harvest_lvl++;
            }
        }
        // Row 4: Auto-Targeting - Cost: 50$
        else if (touch.px >= 180 && touch.px <= 240 && touch.py >= 86 && touch.py <= 100) {
            if (!g_game.upgrades.auto_target && g_game.scrap >= 50) {
                g_game.scrap -= 50;
                g_game.upgrades.auto_target = 1;
            }
        }
        // Row 5: Cinta Transportadora (1B/s) - Cost: 100$
        else if (touch.px >= 180 && touch.px <= 240 && touch.py >= 106 && touch.py <= 120) {
            if (g_game.scrap >= 100) {
                g_game.scrap -= 100;
                g_game.upgrades.conveyor_lvl++;
            }
        }
        // Row 6: Segunda Torreta - Cost: 150$
        else if (touch.px >= 180 && touch.px <= 240 && touch.py >= 126 && touch.py <= 140) {
            if (!g_turrets[1].placed && g_game.scrap >= 150) {
                g_game.scrap -= 150;
                g_turrets[1].x = 64;
                g_turrets[1].y = 110;
                g_turrets[1].placed = 1;
                g_turrets[1].active = 1;
            }
        }
    }
}
