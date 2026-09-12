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

#include <stdio.h>
#include <fat.h>

GameBalanceConfig g_balance;

static const GameBalanceConfig s_default_balance = {
    .waves = {
        // W1: 12 Larvae (delay 90f, speed 30, hp 6), 0 Rippers, 0 Hormagaunts
        { .tiers = { { 12, 90, 30, 6 }, { 0, 120, 40, 24 }, { 0, 150, 50, 45 } }, .scrap_base = 2 },
        // W2: 25 Larvae (delay 60f, speed 32, hp 6), 0 Rippers, 0 Hormagaunts
        { .tiers = { { 25, 60, 32, 6 }, { 0, 120, 40, 24 }, { 0, 150, 50, 45 } }, .scrap_base = 2 },
        // W3: 40 Larvae (delay 45f, speed 34, hp 7), 2 Rippers (delay 180f, speed 38, hp 24), 0 Hormagaunts
        { .tiers = { { 40, 45, 34, 7 }, { 2, 180, 38, 24 }, { 0, 150, 50, 45 } }, .scrap_base = 2 },
        // W4: 60 Larvae (delay 35f, speed 35, hp 8), 4 Rippers (delay 140f, speed 40, hp 28), 0 Hormagaunts
        { .tiers = { { 60, 35, 35, 8 }, { 4, 140, 40, 28 }, { 0, 150, 50, 45 } }, .scrap_base = 2 },
        // W5: 100 Larvae (delay 24f, speed 35, hp 8), 8 Rippers (delay 100f, speed 42, hp 30), 1 Hormagaunt (delay 300f, speed 50, hp 50)
        { .tiers = { { 100, 24, 35, 8 }, { 8, 100, 42, 30 }, { 1, 300, 50, 50 } }, .scrap_base = 3 },
        // W6: 140 Larvae (delay 20f, speed 36, hp 8), 12 Rippers (delay 80f, speed 42, hp 32), 2 Hormagaunts (delay 240f, speed 50, hp 50)
        { .tiers = { { 140, 20, 36, 8 }, { 12, 80, 42, 32 }, { 2, 240, 50, 50 } }, .scrap_base = 3 },
        // W7: 180 Larvae (delay 16f, speed 38, hp 9), 16 Rippers (delay 70f, speed 44, hp 34), 4 Hormagaunts (delay 200f, speed 52, hp 55)
        { .tiers = { { 180, 16, 38, 9 }, { 16, 70, 44, 34 }, { 4, 200, 52, 55 } }, .scrap_base = 3 },
        // W8: 220 Larvae (delay 14f, speed 38, hp 10), 20 Rippers (delay 60f, speed 44, hp 36), 6 Hormagaunts (delay 160f, speed 52, hp 60)
        { .tiers = { { 220, 14, 38, 10 }, { 20, 60, 44, 36 }, { 6, 160, 52, 60 } }, .scrap_base = 4 },
        // W9: 260 Larvae (delay 12f, speed 40, hp 10), 25 Rippers (delay 50f, speed 45, hp 38), 8 Hormagaunts (delay 140f, speed 55, hp 65)
        { .tiers = { { 260, 12, 40, 10 }, { 25, 50, 45, 38 }, { 8, 140, 55, 65 } }, .scrap_base = 4 },
        // W10: 300 Larvae (delay 10f, speed 40, hp 12), 30 Rippers (delay 40f, speed 46, hp 40), 12 Hormagaunts (delay 120f, speed 55, hp 70)
        { .tiers = { { 300, 10, 40, 12 }, { 30, 40, 46, 40 }, { 12, 120, 55, 70 } }, .scrap_base = 5 },

        // Sector 2 (W11..W20):
        { .tiers = { { 60, 18, 42, 14 }, { 35, 36, 48, 45 }, { 15, 90, 58, 80 } }, .scrap_base = 6 },
        { .tiers = { { 80, 15, 42, 15 }, { 40, 32, 48, 48 }, { 20, 80, 58, 90 } }, .scrap_base = 8 },
        { .tiers = { { 100, 14, 44, 16 }, { 50, 28, 50, 50 }, { 25, 70, 60, 100 } }, .scrap_base = 10 },
        { .tiers = { { 120, 12, 44, 18 }, { 60, 25, 50, 55 }, { 30, 60, 60, 110 } }, .scrap_base = 12 },
        { .tiers = { { 150, 10, 45, 20 }, { 70, 22, 52, 60 }, { 40, 50, 62, 120 } }, .scrap_base = 15 },
        { .tiers = { { 160, 10, 46, 22 }, { 80, 20, 52, 65 }, { 50, 45, 62, 130 } }, .scrap_base = 20 },
        { .tiers = { { 180,  9, 46, 25 }, { 90, 18, 54, 70 }, { 60, 40, 64, 140 } }, .scrap_base = 25 },
        { .tiers = { { 200,  8, 48, 28 }, { 100, 16, 54, 75 }, { 70, 35, 64, 150 } }, .scrap_base = 30 },
        { .tiers = { { 220,  8, 48, 30 }, { 110, 15, 55, 80 }, { 80, 30, 65, 160 } }, .scrap_base = 40 },
        { .tiers = { { 250,  6, 50, 35 }, { 130, 12, 56, 90 }, { 100, 25, 66, 180 } }, .scrap_base = 50 }
    },
    .magic = 0x544F5744 // "TOWD"
};

static int s_fat_available = 0;

void balance_config_reset_defaults(void) {
    memcpy(&g_balance, &s_default_balance, sizeof(GameBalanceConfig));
}

void balance_config_save(void) {
    if (!s_fat_available) return;
    FILE *f = fopen("fat:/towerds_balance.bin", "wb");
    if (!f) f = fopen("towerds_balance.bin", "wb");
    if (f) {
        fwrite(&g_balance, sizeof(GameBalanceConfig), 1, f);
        fclose(f);
    }
}

void balance_config_load(void) {
    if (!s_fat_available) return;
    FILE *f = fopen("fat:/towerds_balance.bin", "rb");
    if (!f) f = fopen("towerds_balance.bin", "rb");
    if (f) {
        GameBalanceConfig loaded;
        if (fread(&loaded, sizeof(GameBalanceConfig), 1, f) == 1) {
            if (loaded.magic == 0x544F5744) {
                memcpy(&g_balance, &loaded, sizeof(GameBalanceConfig));
            }
        }
        fclose(f);
    }
}

void balance_config_init(void) {
    balance_config_reset_defaults();
    s_fat_available = fatInitDefault();
    if (s_fat_available) {
        balance_config_load();
    }
}

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

static void spawn_enemy(int variant, uint64_t hp, int base_spd) {
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

            int spd = base_spd + ((rand() % 5) - 2);
            if (spd < 10) spd = 10;
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
            // 8 px/frame bullet velocity (faster so enemies don't dodge it)
            g_bullets[i].vx = (fixed_cos(angle) * 8);
            g_bullets[i].vy = (fixed_sin(angle) * 8);
            g_bullets[i].life = 35;
            g_bullets[i].damage = dmg;
            break;
        }
    }
}

void game_init(void) {
    static int s_balance_inited = 0;
    if (!s_balance_inited) {
        balance_config_init();
        s_balance_inited = 1;
    }

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
    const WaveDef *wdef = &g_balance.waves[w_idx];

    // Total enemies to spawn across Tier 0, 1, 2
    g_game.enemies_to_spawn = wdef->tiers[0].count + wdef->tiers[1].count + wdef->tiers[2].count;
    for (int t = 0; t < 3; t++) {
        g_game.wave_spawned_tier[t] = 0;
        g_game.wave_spawn_timer_tier[t] = 0;
    }

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
    const WaveDef *wdef = &g_balance.waves[w_idx];

    // 1. Spawning per tier (Tier 0, Tier 1, Tier 2)
    for (int t = 0; t < 3; t++) {
        if (g_game.wave_spawned_tier[t] < wdef->tiers[t].count) {
            g_game.wave_spawn_timer_tier[t]++;
            int interval = wdef->tiers[t].delay;
            if (interval < 1) interval = 1;
            if (g_game.wave_spawn_timer_tier[t] >= interval) {
                g_game.wave_spawn_timer_tier[t] = 0;
                g_game.wave_spawned_tier[t]++;
                int hp = wdef->tiers[t].hp;
                if (hp < 1) hp = 1;
                spawn_enemy(t, hp, wdef->tiers[t].speed);
            }
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

        // Check if hitting Bunker Sanctum:
        // Bunker visual footprint is centered at (128, 172) on bottom screen, global y = 364, height 42 -> top is y=344.
        // Guarantee that any enemy reaching global y >= 344 bites the bunker, never walking off-screen!
        if (py >= 344) {
            // Clamp enemy position at bunker front wall so it doesn't escape out of screen bounds
            g_enemies[i].y = TO_FP(344);
            if (px < 100) g_enemies[i].x = TO_FP(100);
            if (px > 156) g_enemies[i].x = TO_FP(156);

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

        // Advance: vertical downward + funnel convergence to center (128)
        int spd = (g_enemies[i].speed * FP_ONE) / 60;
        if (spd < 1) spd = 1;

        g_enemies[i].y += spd;

        // Converge X towards 128 as it approaches bottom screen
        if (py > 80) {
            if (ex < target_base_x) ex += (spd / 2);
            else if (ex > target_base_x) ex -= (spd / 2);
            g_enemies[i].x = ex;
        }
    }

    // 5. Update Turrets & Logistics (Factorio conveyors & reloading)
    // 5. Update Turrets & Logistics
    for (int t = 0; t < MAX_TURRETS; t++) {
        Turret *tur = &g_turrets[t];
        if (!tur->placed || !tur->active) continue;

        if (tur->flash_timer > 0) tur->flash_timer--;
        if (tur->barrel_recoil_l > 0) tur->barrel_recoil_l--;
        if (tur->barrel_recoil_r > 0) tur->barrel_recoil_r--;

        // Advance canonical firing animation frames (30 FPS visual tick)
        if (tur->anim_frame > 0) {
            if (g_game.sim_ticks_elapsed % 2 == 0) {
                // Alternating barrels sequence: Left barrel is 1..4, Right barrel is 6..9
                if (tur->anim_frame >= 1 && tur->anim_frame <= 4) {
                    tur->anim_frame++;
                    if (tur->anim_frame > 4) tur->anim_frame = 0; // Return to idle
                } else if (tur->anim_frame >= 6 && tur->anim_frame <= 9) {
                    tur->anim_frame++;
                    if (tur->anim_frame > 9) tur->anim_frame = 0; // Return to idle
                } else {
                    tur->anim_frame = 0;
                }
            }
        }

        // Conveyor passive reloading (Branch C): 1/s, 3/s, 6/s
        if (g_game.upgrades.conveyor_lvl > 0 && tur->ammo < tur->max_ammo) {
            static const int s_rates[4] = { 9999, 60, 20, 10 };
            int rate = (g_game.upgrades.conveyor_lvl <= 3) ? s_rates[g_game.upgrades.conveyor_lvl] : 10;
            if (g_game.sim_ticks_elapsed % rate == 0) {
                tur->ammo++;
            }
        }

        // Dynamic range based on upgrade
        static const int s_ranges[5] = { 65, 80, 100, 125, 150 };
        int r_lvl = g_game.upgrades.range_lvl;
        tur->range = (r_lvl < 5) ? s_ranges[r_lvl] : 150;

        // Target selection
        int target_enemy = -1;

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
            // Manual locked enemy from stylus tap
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

        // Aim and fire
        if (target_enemy >= 0) {
            int gx = FROM_FP(g_enemies[target_enemy].x);
            int local_y = FROM_FP(g_enemies[target_enemy].y) - 192;
            tur->target_angle = fixed_atan2(local_y - tur->y, gx - tur->x);
            tur->current_angle = tur->target_angle;

            // Firing strictly consumes ammo!
            if (tur->ammo > 0) {
                if (tur->fire_cooldown > 0) {
                    tur->fire_cooldown--;
                } else {
                    static const int s_intervals[5] = { 18, 14, 10, 7, 5 };
                    int f_lvl = g_game.upgrades.firerate_lvl;
                    int interval = (f_lvl < 5) ? s_intervals[f_lvl] : 5;
                    tur->fire_cooldown = interval;
                    tur->flash_timer = 3;
                    tur->ammo--;

                    // Alternating barrels animation: Frame 1..4 (left) and 6..9 (right)
                    tur->last_barrel = 1 - tur->last_barrel;
                    if (tur->last_barrel == 0) {
                        tur->anim_frame = 1;
                        tur->barrel_recoil_l = 4;
                    } else {
                        tur->anim_frame = 6;
                        tur->barrel_recoil_r = 4;
                    }

                    int ang = tur->current_angle & 0xFF;
                    int perp_x = -fixed_sin(ang);
                    int perp_y = fixed_cos(ang);
                    int s = (tur->last_barrel == 0) ? -3 : 3;
                    int bx = tur->x + ((perp_x * s) >> FP_SHIFT);
                    int by = tur->y + ((perp_y * s) >> FP_SHIFT);

                    // Multiplicative damage: Base 2 -> 3 -> 4 -> 6 -> 8
                    static const uint64_t s_dmg[5] = { 2, 3, 4, 6, 8 };
                    int c_lvl = g_game.upgrades.caliber_lvl;
                    uint64_t dmg = (c_lvl < 5) ? s_dmg[c_lvl] : 8;

                    spawn_bullet(bx, by, ang, t, dmg);
                    tur->shots_fired++;
                }
            } else {
                // Out of ammo: reset firing frame
                tur->anim_frame = 0;
            }
        } else {
            tur->anim_frame = 0;
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

                    uint64_t base_scrap = 1 + g_enemies[e].variant * 2;
                    uint64_t reward = base_scrap * (1 + g_game.upgrades.bio_harvest_lvl);
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
        // Wave clear bonus scrap (+10 in W1, +15 in W2, etc.)
        uint64_t wave_bonus = 10 + (g_game.wave_number * 5);
        g_game.scrap += wave_bonus;

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

        // [UPGRADES] Button: drawn at (55, 156, 50, 24) -> hitbox (50..105, 144..191)
        if (touch.px >= 50 && touch.px <= 105 && touch.py >= 144 && touch.py <= 191) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_UPGRADES;
            return;
        }

        // [CALIB] Button: drawn at (112, 156, 44, 24) -> hitbox (108..160, 144..191)
        if (touch.px >= 108 && touch.px <= 160 && touch.py >= 144 && touch.py <= 191) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_CALIBRATION;
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

        // Ammo Depot drag start (depot is at x: 6..46, y: 150..186)
        if (touch.px >= 6 && touch.px <= 50 && touch.py >= 145 && touch.py <= 190) {
            g_game.is_dragging_ammo = 1;
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
            return;
        }

        // Target designated enemy in bottom screen with stylus
        int clicked_enemy = -1;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue;
            int local_y = gy - 192;
            int gx = FROM_FP(g_enemies[e].x);

            if (abs(touch.px - gx) <= 16 && abs(touch.py - local_y) <= 16) {
                clicked_enemy = e;
                break;
            }
        }

        if (clicked_enemy >= 0) {
            // Lock onto this enemy for all placed turrets!
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    g_turrets[t].locked_enemy_idx = clicked_enemy;
                }
            }
            return;
        }
    }

    if (keys_held & KEY_TOUCH) {
        if (g_game.is_dragging_ammo) {
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
        } else {
            // If continuous fire / sweep is enabled and touching bottom screen battlefield
            if (g_game.upgrades.continuous_fire && touch.py < 150) {
                // Find enemy near touch to track
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (!g_enemies[e].active) continue;
                    int gy = FROM_FP(g_enemies[e].y);
                    if (gy < 192) continue;
                    int local_y = gy - 192;
                    int gx = FROM_FP(g_enemies[e].x);

                    if (abs(touch.px - gx) <= 24 && abs(touch.py - local_y) <= 24) {
                        for (int t = 0; t < MAX_TURRETS; t++) {
                            if (g_turrets[t].placed) {
                                g_turrets[t].locked_enemy_idx = e;
                            }
                        }
                        break;
                    }
                }
            }
        }
    } else {
        // Release touch: if dragging ammo over a turret, reload it!
        if (g_game.is_dragging_ammo) {
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    int d = abs(g_game.drag_x - g_turrets[t].x) + abs(g_game.drag_y - g_turrets[t].y);
                    if (d <= 24) {
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

uint64_t upgrade_get_cost(int idx) {
    switch (idx) {
        case 0: { // Caliber: 15, 25, 40, 65, 100
            static const uint64_t c[6] = { 15, 25, 40, 65, 100, 999999 };
            int lvl = g_game.upgrades.caliber_lvl;
            return (lvl < 5) ? c[lvl] : 999999;
        }
        case 1: { // Fire rate: 20, 30, 45, 70, 110
            static const uint64_t c[6] = { 20, 30, 45, 70, 110, 999999 };
            int lvl = g_game.upgrades.firerate_lvl;
            return (lvl < 5) ? c[lvl] : 999999;
        }
        case 2: { // Mag size: 15, 25, 35, 55, 85
            static const uint64_t c[6] = { 15, 25, 35, 55, 85, 999999 };
            int lvl = g_game.upgrades.mag_size_lvl;
            return (lvl < 5) ? c[lvl] : 999999;
        }
        case 3: { // Bio Harvest: 25, 40, 65, 105, 170
            static const uint64_t c[6] = { 25, 40, 65, 105, 170, 999999 };
            int lvl = g_game.upgrades.bio_harvest_lvl;
            return (lvl < 5) ? c[lvl] : 999999;
        }
        case 4: { // Supply Conveyor: 50, 90, 160
            static const uint64_t c[4] = { 50, 90, 160, 999999 };
            int lvl = g_game.upgrades.conveyor_lvl;
            return (lvl < 3) ? c[lvl] : 999999;
        }
        case 5: { // Auto Target Cogitator: 80
            return g_game.upgrades.auto_target ? 999999 : 80;
        }
        default:
            return 999999;
    }
}

int upgrade_can_afford(int idx) {
    uint64_t cost = upgrade_get_cost(idx);
    return (cost < 999999 && g_game.scrap >= cost);
}

void upgrade_purchase(int idx) {
    if (!upgrade_can_afford(idx)) return;
    uint64_t cost = upgrade_get_cost(idx);
    g_game.scrap -= cost;
    g_game.upgrade_flash_timer = 4;
    g_game.upgrade_flash_idx = idx;

    switch (idx) {
        case 0: g_game.upgrades.caliber_lvl++; break;
        case 1: g_game.upgrades.firerate_lvl++; break;
        case 2: {
            g_game.upgrades.mag_size_lvl++;
            // Update all turrets max ammo
            for (int t = 0; t < MAX_TURRETS; t++) {
                static const int s_mag[6] = { 20, 35, 50, 70, 100, 150 };
                int m = (g_game.upgrades.mag_size_lvl < 6) ? s_mag[g_game.upgrades.mag_size_lvl] : 150;
                g_turrets[t].max_ammo = m;
            }
            break;
        }
        case 3: g_game.upgrades.bio_harvest_lvl++; break;
        case 4: g_game.upgrades.conveyor_lvl++; break;
        case 5: g_game.upgrades.auto_target = 1; break;
    }
}

void game_handle_input_upgrades(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & (KEY_B | KEY_START)) {
        g_game.mode = g_game.previous_mode;
        return;
    }

    if (keys_down & KEY_TOUCH) {
        // Return button: (90, 150, 76, 28)
        if (touch.px >= 85 && touch.px <= 170 && touch.py >= 145 && touch.py <= 180) {
            g_game.mode = g_game.previous_mode;
            return;
        }

        // Tab 1: Caliber (10, 24, 110, 32)
        if (touch.px >= 10 && touch.px <= 120 && touch.py >= 24 && touch.py <= 56) {
            upgrade_purchase(0);
        }
        // Tab 2: Fire Rate (130, 24, 110, 32)
        else if (touch.px >= 130 && touch.px <= 240 && touch.py >= 24 && touch.py <= 56) {
            upgrade_purchase(1);
        }
        // Tab 3: Mag Size (10, 62, 110, 32)
        else if (touch.px >= 10 && touch.px <= 120 && touch.py >= 62 && touch.py <= 94) {
            upgrade_purchase(2);
        }
        // Tab 4: Bio Harvest (130, 62, 110, 32)
        else if (touch.px >= 130 && touch.px <= 240 && touch.py >= 62 && touch.py <= 94) {
            upgrade_purchase(3);
        }
        // Tab 5: Auto Supply Conveyor (10, 100, 110, 32)
        else if (touch.px >= 10 && touch.px <= 120 && touch.py >= 100 && touch.py <= 132) {
            upgrade_purchase(4);
        }
        // Tab 6: Auto Target (130, 100, 110, 32)
        else if (touch.px >= 130 && touch.px <= 240 && touch.py >= 100 && touch.py <= 132) {
            upgrade_purchase(5);
        }
    }
}

static void calib_modify_val(int delta) {
    int w = g_game.calib_wave_idx;
    if (w < 0) w = 0;
    if (w >= 20) w = 19;
    WaveDef *wd = &g_balance.waves[w];

    int row = g_game.calib_row;
    if (row < 0) row = 0;
    if (row > 8) row = 8;

    int tier = row / 3;
    int param = row % 3;
    WaveTierConfig *tc = &wd->tiers[tier];

    switch (param) {
        case 0: // Total enemies count (0..200)
            tc->count += delta;
            if (tc->count < 0) tc->count = 0;
            if (tc->count > 200) tc->count = 200;
            break;
        case 1: // Spawn delay in frames (5..300)
            tc->delay += delta;
            if (tc->delay < 5) tc->delay = 5;
            if (tc->delay > 300) tc->delay = 300;
            break;
        case 2: // Speed in px/s (10..150)
            tc->speed += delta;
            if (tc->speed < 10) tc->speed = 10;
            if (tc->speed > 150) tc->speed = 150;
            break;
    }

    g_game.calib_saved_timer = 20;
    balance_config_save();
}

void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_B) {
        g_game.mode = g_game.previous_mode;
        return;
    }

    // L / R: cycle waves (1..20)
    if (keys_down & KEY_L) {
        g_game.calib_wave_idx = (g_game.calib_wave_idx + 19) % 20;
    }
    if (keys_down & KEY_R) {
        g_game.calib_wave_idx = (g_game.calib_wave_idx + 1) % 20;
    }

    // Up / Down: select parameter row (0..8)
    int max_rows = 9;
    if (keys_down & KEY_UP) {
        g_game.calib_row = (g_game.calib_row + max_rows - 1) % max_rows;
    }
    if (keys_down & KEY_DOWN) {
        g_game.calib_row = (g_game.calib_row + 1) % max_rows;
    }

    // Left / Right with continuous autorepeat
    int step = 1;
    int param = g_game.calib_row % 3;
    if (param == 0) step = 1; // count
    else if (param == 1) step = 5; // delay
    else if (param == 2) step = 2; // speed

    if (keys_down & KEY_LEFT) {
        calib_modify_val(-step);
        g_game.calib_hold_timer = 0;
    } else if (keys_down & KEY_RIGHT) {
        calib_modify_val(+step);
        g_game.calib_hold_timer = 0;
    } else if (keys_held & (KEY_LEFT | KEY_RIGHT)) {
        g_game.calib_hold_timer++;
        if (g_game.calib_hold_timer >= 16 && (g_game.calib_hold_timer % 3 == 0)) {
            int mult = (g_game.calib_hold_timer >= 45) ? 5 : 1;
            calib_modify_val((keys_held & KEY_LEFT) ? -(step * mult) : +(step * mult));
        }
    } else {
        g_game.calib_hold_timer = 0;
    }

    // Touch controls
    if (keys_down & KEY_TOUCH) {
        // Navigation buttons [<] (10..40, 18..34) and [>] (215..245, 18..34) for Wave
        if (touch.py >= 16 && touch.py <= 34) {
            if (touch.px >= 8 && touch.px <= 42) { // [<]
                g_game.calib_wave_idx = (g_game.calib_wave_idx + 19) % 20;
                return;
            } else if (touch.px >= 214 && touch.px <= 248) { // [>]
                g_game.calib_wave_idx = (g_game.calib_wave_idx + 1) % 20;
                return;
            }
        }

        // Parameter rows touch hitboxes (9 rows: y starts at 36, each row height 13)
        for (int r = 0; r < 9; r++) {
            int ry = 36 + r * 13;
            if (touch.py >= ry && touch.py <= ry + 12) {
                g_game.calib_row = r;
                int rparam = r % 3;
                int rstep = (rparam == 0) ? 1 : ((rparam == 1) ? 5 : 2);
                // Tap on [-] box (175..205) or [+] box (212..242)
                if (touch.px >= 175 && touch.px <= 205) {
                    calib_modify_val(-rstep);
                } else if (touch.px >= 212 && touch.px <= 242) {
                    calib_modify_val(+rstep);
                }
                return;
            }
        }

        // Bottom action buttons:
        // [RESTART W1] (8..85, 160..186)
        if (touch.px >= 8 && touch.px <= 85 && touch.py >= 158 && touch.py <= 186) {
            game_init();
            return;
        }
        // [DEFAULTS] (95..165, 160..186)
        if (touch.px >= 95 && touch.px <= 165 && touch.py >= 158 && touch.py <= 186) {
            balance_config_reset_defaults();
            balance_config_save();
            g_game.calib_saved_timer = 30;
            return;
        }
        // [BACK / RESUME] (175..248, 160..186)
        if (touch.px >= 175 && touch.px <= 248 && touch.py >= 158 && touch.py <= 186) {
            g_game.mode = g_game.previous_mode;
            return;
        }
    }
}
