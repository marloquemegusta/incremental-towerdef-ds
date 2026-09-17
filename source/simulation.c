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
    .enemy_hp = { 18, 25, 75, 160, 320, 500, 1100, 2600 },
    .enemy_scrap = { 4, 5, 15, 35, 70, 120, 250, 600 },
    .upgrade_costs = { {15,25,40,65,100}, {20,30,45,70,110}, {15,25,35,55,85}, {25,40,65,105,170}, {50,90,160,0,0}, {80,0,0,0,0}, {200,400,800,1600,0} },
    .turret_damage = { 2, 3, 4, 6, 8 },
    .turret_fire_interval = { 18, 14, 10, 7, 5 },
    .turret_range = { 65, 80, 100, 125, 150 },
    .turret_magazine = { 20, 35, 50, 70, 100, 150 },
    .bunker_start_hp = 100, .wave_duration_frames = 1800,
    .wave_bonus_base = 10, .wave_bonus_per_wave = 5,
    .enemy_bite_damage = { 8, 2, 4, 6, 10, 14, 20, 35 },
    .enemy_bite_interval = { 30, 40, 45, 40, 50, 45, 60, 50 },
    .conveyor_reload_interval = { 9999, 60, 20, 10 },
    .range_upgrade_costs = { 30, 60, 120, 240, 480 },
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
        uint8_t raw[sizeof(GameBalanceConfig)];
        size_t n = fread(raw, 1, sizeof(raw), f);
        if (n == sizeof(GameBalanceConfig)) {
            GameBalanceConfig loaded;
            memcpy(&loaded, raw, sizeof(loaded));
            if (loaded.magic == 0x544F5744) memcpy(&g_balance, &loaded, sizeof(g_balance));
        } else if (n == 2576) {
            uint32_t old_magic;
            memcpy(&old_magic, raw + 2572, sizeof(old_magic));
            if (old_magic == 0x544F5744) {
                memcpy(&g_balance, raw, 2572);
                memcpy(&g_balance.range_upgrade_costs[0], (uint64_t[5]){30,60,120,240,480}, sizeof(g_balance.range_upgrade_costs));
                g_balance.magic = 0x544F5744;
            }
        } else if (n == 1576) {
            /* Migrate the previous 6-upgrade format without losing user tuning. */
            uint32_t old_magic;
            memcpy(&old_magic, raw + 1572, sizeof(old_magic));
            if (old_magic == 0x544F5744) {
                memcpy(&g_balance.waves[0], raw, 1120);
                memcpy(&g_balance.enemy_hp[0], raw + 1120, 24);
                memcpy(&g_balance.enemy_scrap[0], raw + 1144, 24);
                memcpy(&g_balance.upgrade_costs[0][0], raw + 1168, 240);
                memcpy(&g_balance.turret_damage[0], raw + 1408, 20);
                memcpy(&g_balance.turret_fire_interval[0], raw + 1428, 20);
                memcpy(&g_balance.turret_range[0], raw + 1448, 20);
                memcpy(&g_balance.turret_magazine[0], raw + 1468, 24);
                memcpy(&g_balance.bunker_start_hp, raw + 1492, 16);
                memcpy(&g_balance.enemy_bite_damage[0], raw + 1508, 24);
                memcpy(&g_balance.enemy_bite_interval[0], raw + 1532, 24);
                memcpy(&g_balance.conveyor_reload_interval[0], raw + 1556, 16);
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

    // More particles: 18 base + variant * 4 (up to 46 for colossus)
    int particle_count = 18 + variant * 4;
    if (particle_count > 48) particle_count = 48;

    uint16_t col_primary   = (variant == 0 || variant == 3) ? COLOR_XENOS_ICHOR : COLOR_BLOOD_DARK;
    uint16_t col_secondary = COLOR_XENOS_FLESH;
    uint16_t col_chitin    = COLOR_XENOS_CHITIN;
    uint16_t col_ichor     = COLOR_XENOS_ICHOR;

    // Proportional puddle on ground (lasts 900-1200 frames, ~15-20s):
    // size 1: Small (Scourge, Zergling) ~ 8x5 px
    // size 2: Medium (Hydralisk, Mutalisk, Defiler) ~ 14x7 px
    // size 3: Large (Lurker, Guardian) ~ 20x9 px
    // size 4: Colossus (Ultralisk) ~ 30x13 px
    int pool_size = 1;
    if (variant >= 2 && variant <= 4) pool_size = 2;
    else if (variant >= 5 && variant <= 6) pool_size = 3;
    else if (variant >= 7) pool_size = 4;

    game_add_splatter_ex(x, y, col_primary, pool_size, 900 + (rand() % 300));

    // Satellite splatter droplets around the pool
    int satellite_count = 2 + pool_size * 2;
    for (int d = 0; d < satellite_count; d++) {
        int ang = rand() % 256;
        int dist = (rand() % (pool_size * 3 + 3)) + 3;
        int dx = (fixed_cos(ang) * dist) >> 8;
        int dy = (fixed_sin(ang) * dist) >> 8;
        uint16_t d_col = (rand() % 2 == 0) ? col_secondary : col_ichor;
        game_add_splatter_ex(x + dx, y + dy, d_col, 0, 700 + (rand() % 300));
    }

    // Airborne ballistic particles: LOCAL EXPLOSION with 1 or 2 shrapnel flung far
    int spawned = 0;
    for (int i = 0; i < MAX_DEATH_PARTICLES && spawned < particle_count; i++) {
        if (!g_death_particles[i].active) {
            g_death_particles[i].active = 1;
            g_death_particles[i].x = TO_FP(x) + ((rand() % 7 - 3) << (FP_SHIFT - 1));
            g_death_particles[i].y = TO_FP(y) + ((rand() % 7 - 3) << (FP_SHIFT - 1));
            g_death_particles[i].z = TO_FP(2 + (rand() % 3));

            int ang = rand() % 256;

            // Exactly 1 or 2 particles shoot far; the rest remain strictly local
            int is_distant = (spawned == 0 || (spawned == 1 && (rand() % 3 == 0)));

            if (is_distant) {
                // High velocity shrapnel flung far (2..3.5 px/frame)
                int spd = TO_FP(2) + (rand() % (FP_ONE * 3 / 2));
                g_death_particles[i].vx = ((fixed_cos(ang) * spd) >> FP_SHIFT) + (bvx / 16);
                g_death_particles[i].vy = ((fixed_sin(ang) * spd) >> FP_SHIFT) + (bvy / 16);
                g_death_particles[i].vz = TO_FP(2) + (rand() % TO_FP(2));
                g_death_particles[i].life = 35 + (rand() % 15);
                g_death_particles[i].size = 1; // Chunky fragment
                g_death_particles[i].color = col_chitin;
            } else {
                // LOCAL EXPLOSION: Low velocity (0.15..0.75 px/frame), stays within 3..8 px
                int spd = (rand() % (FP_ONE * 5 / 8)) + (FP_ONE / 8);
                g_death_particles[i].vx = ((fixed_cos(ang) * spd) >> FP_SHIFT) + (bvx / 48);
                g_death_particles[i].vy = ((fixed_sin(ang) * spd) >> FP_SHIFT) + (bvy / 48);
                g_death_particles[i].vz = (FP_ONE / 2) + (rand() % (FP_ONE * 3 / 2)); // Pop up in air
                g_death_particles[i].life = 16 + (rand() % 14);
                g_death_particles[i].size = (rand() % 3 == 0) ? 1 : 0;
                int r_col = rand() % 3;
                if (r_col == 0) g_death_particles[i].color = col_primary;
                else if (r_col == 1) g_death_particles[i].color = col_secondary;
                else g_death_particles[i].color = col_ichor;
            }
            spawned++;
        }
    }
}

static int enemy_direction_from_delta(int dx, int dy) {
    int ax = (dx < 0) ? -dx : dx;
    int ay = (dy < 0) ? -dy : dy;

    if (ax == 0 && ay == 0) return -1;
    if (ay == 0) return (dx > 0) ? 2 : 6;
    if (ax * 2 < ay) return (dy < 0) ? 0 : 4;
    if (ay * 2 < ax) return (dx > 0) ? 2 : 6;
    if (dx > 0) return (dy < 0) ? 1 : 3;
    return (dy < 0) ? 7 : 5;
}

static void spawn_enemy_ex(int variant, uint64_t hp, int base_spd, int initial_y) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            g_enemies[i].active = 1;
            g_enemies[i].variant = variant;
            g_enemies[i].hp = hp;
            g_enemies[i].max_hp = hp;

            // Spawn with random x spread (32..224) within road
            int sx = 32 + (rand() % 192);
            g_enemies[i].x = TO_FP(sx);
            g_enemies[i].y = TO_FP(initial_y);

            int spd = base_spd + ((rand() % 5) - 2);
            if (spd < 10) spd = 10;
            g_enemies[i].speed = spd;
            g_enemies[i].vx = 0;
            g_enemies[i].vy = (spd * FP_ONE) / 60;

            g_enemies[i].dir = 4; // South in the 8-way compass
            g_enemies[i].anim_frame = 0;
            g_enemies[i].anim_distance = 0;
            g_enemies[i].biting_target = -1;
            g_enemies[i].bite_timer = 0;

            g_game.enemies_spawned++;
            g_game.enemies_alive++;
            break;
        }
    }
}

static void spawn_enemy(int variant, uint64_t hp, int base_spd) {
    spawn_enemy_ex(variant, hp, base_spd, 0);
}

#define BULLET_SPEED 12

static void spawn_bullet(int x, int y, int angle, int turret_idx, uint64_t dmg) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) {
            g_bullets[i].active = 1;
            g_bullets[i].turret_idx = turret_idx;
            g_bullets[i].x = TO_FP(x);
            g_bullets[i].y = TO_FP(y);
            // 12 px/frame high-velocity bolter tracers
            g_bullets[i].vx = (fixed_cos(angle) * BULLET_SPEED);
            g_bullets[i].vy = (fixed_sin(angle) * BULLET_SPEED);
            g_bullets[i].life = 25;
            g_bullets[i].damage = dmg;
            break;
        }
    }
}


WallPlatform g_wall;
CasingParticle g_casings[MAX_CASINGS];
BulletDart g_bullet_darts[MAX_BULLET_DARTS];

void wall_init(void) {
    memset(&g_wall, 0, sizeof(g_wall));
    memset(g_casings, 0, sizeof(g_casings));
    memset(g_bullet_darts, 0, sizeof(g_bullet_darts));

    g_wall.screen_y = WALL_DEFAULT_Y; // 144
    g_wall.hp = 1000;
    g_wall.max_hp = 1000;
    g_wall.active_turrets = 2; // Default dual battery: sockets 1 & 2
    g_wall.turret_angles[0] = 0; // NW
    g_wall.turret_angles[1] = 1; // NNW
    g_wall.turret_angles[2] = 3; // NNE
    g_wall.turret_angles[3] = 4; // NE
    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        g_wall.target_angles[s] = g_wall.turret_angles[s];
        g_wall.target_enemy_idx[s] = -1;
        g_wall.turret_cooldown[s] = 0;
        g_wall.traverse_timer[s] = 0;
    }
    g_wall.fire_cooldown = 0;
    g_wall.fire_interval = 6; // ~10 shots/sec per turret
    g_wall.damage = 10;
    g_wall.range = WALL_TURRET_RANGE;
    g_wall.locked_enemy_idx = -1;
}

int wall_angle_from_target(int turret_x, int turret_y, int target_x, int target_y) {
    int dx = target_x - turret_x;
    int dy = turret_y - target_y; // Upward is positive
    if (dy <= 0) dy = 1;

    int slope = (dx * 100) / dy;
    if (slope < -65) return 0;      // NW (-45 deg)
    else if (slope < -18) return 1; // NNW (-22.5 deg)
    else if (slope <= 18) return 2; // N (0 deg)
    else if (slope <= 65) return 3; // NNE (+22.5 deg)
    else return 4;                  // NE (+45 deg)
}

void wall_spawn_casing(int x, int y, int dir_sign) {
    for (int i = 0; i < MAX_CASINGS; i++) {
        if (!g_casings[i].active) {
            g_casings[i].active = 1;
            g_casings[i].x = TO_FP(x);
            g_casings[i].y = TO_FP(y);
            g_casings[i].z = TO_FP(4);
            int base_vx = TO_FP(1) + (rand() % TO_FP(1));
            g_casings[i].vx = dir_sign * base_vx;
            g_casings[i].vy = TO_FP(1) + (rand() % TO_FP(1)); // pops toward bottom screen
            g_casings[i].vz = TO_FP(3) + (rand() % TO_FP(2)); // pops upward
            g_casings[i].angle = rand() % 360;
            g_casings[i].spin_speed = dir_sign * (30 + (rand() % 20));
            g_casings[i].bounces = 0;
            g_casings[i].life = 60;
            break;
        }
    }
}

void wall_spawn_bullet_dart(int start_x, int start_y, int target_x, int target_y) {
    for (int i = 0; i < MAX_BULLET_DARTS; i++) {
        if (!g_bullet_darts[i].active) {
            g_bullet_darts[i].active = 1;
            g_bullet_darts[i].x = TO_FP(start_x);
            g_bullet_darts[i].y = TO_FP(start_y);
            g_bullet_darts[i].target_x = target_x;
            g_bullet_darts[i].target_y = target_y;
            g_bullet_darts[i].damage = g_wall.damage;

            int dx = target_x - start_x;
            int dy = target_y - start_y;
            int ax = (dx < 0) ? -dx : dx;
            int ay = (dy < 0) ? -dy : dy;
            int dist = (ax > ay) ? (ax + (ay >> 1)) : (ay + (ax >> 1));
            if (dist < 1) dist = 1;
            g_bullet_darts[i].dist_remaining = TO_FP(dist);

            int speed = TO_FP(16); // 16 px/frame
            g_bullet_darts[i].vx = (dx * speed) / dist;
            g_bullet_darts[i].vy = (dy * speed) / dist;
            break;
        }
    }
}

void wall_fire_socket(int s, int target_x, int target_y) {
    if (s < 0 || s >= WALL_SOCKET_COUNT) return;

    int sx = c_wall_sockets[s].x;
    int sy = g_wall.screen_y + c_wall_sockets[s].y;

    // Strict range verification: do not fire outside effective range!
    int tdx = target_x - sx;
    int tdy = target_y - sy;
    if (tdx * tdx + tdy * tdy > g_wall.range * g_wall.range) return;

    g_wall.turret_cooldown[s] = g_wall.fire_interval;

    int angle = g_wall.turret_angles[s];
    int alt = g_wall.barrel_alt[s];
    g_wall.barrel_alt[s] = 1 - alt;

    const TurretCalibratedPoints *pts = &c_turret_points[angle];
    int tx = sx - TURRET_PIVOT_X;
    int ty = sy - TURRET_PIVOT_Y;

    int mx = tx + (alt == 0 ? pts->ml_x : pts->mr_x);
    int my = ty + (alt == 0 ? pts->ml_y : pts->mr_y);
    int dx = tx + (alt == 0 ? pts->dl_x : pts->dr_x);
    int dy = ty + (alt == 0 ? pts->dl_y : pts->dr_y);

    g_wall.muzzle_flash_timer[s] = 2;
    g_wall.muzzle_flash_barrel[s] = alt;

    wall_spawn_bullet_dart(mx, my, target_x, target_y);
    wall_spawn_casing(dx, dy, (alt == 0 ? -1 : 1));
}

void wall_fire_at(int target_x, int target_y) {
    if (g_wall.fire_cooldown > 0) return;

    int active_mask = 0;
    if (g_wall.active_turrets == 1) active_mask = (1 << 1);
    else if (g_wall.active_turrets == 2) active_mask = (1 << 1) | (1 << 2);
    else if (g_wall.active_turrets == 3) active_mask = (1 << 0) | (1 << 1) | (1 << 2);
    else active_mask = 0x0F;

    int best_sock = -1;
    int best_dist = 9999;
    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        if (!(active_mask & (1 << s))) continue;
        int sx = c_wall_sockets[s].x;
        int sy = g_wall.screen_y + c_wall_sockets[s].y;
        int dx = target_x - sx;
        int dy = target_y - sy;
        int dsq = dx * dx + dy * dy;
        // Candidate socket must be in range of target!
        if (dsq <= g_wall.range * g_wall.range) {
            int h_dist = (dx < 0) ? -dx : dx;
            if (h_dist < best_dist) {
                best_dist = h_dist;
                best_sock = s;
            }
        }
    }

    // If point is out of range for all active turrets, do not fire!
    if (best_sock < 0) return;

    int sx = c_wall_sockets[best_sock].x;
    int sy = g_wall.screen_y + c_wall_sockets[best_sock].y;
    int angle = wall_angle_from_target(sx, sy, target_x, target_y);
    g_wall.turret_angles[best_sock] = angle;
    g_wall.target_angles[best_sock] = angle;

    wall_fire_socket(best_sock, target_x, target_y);
    g_wall.fire_cooldown = g_wall.fire_interval;
}

void wall_update(void) {
    if (g_wall.fire_cooldown > 0) g_wall.fire_cooldown--;

    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        if (g_wall.turret_cooldown[s] > 0) g_wall.turret_cooldown[s]--;
        if (g_wall.muzzle_flash_timer[s] > 0) {
            g_wall.muzzle_flash_timer[s]--;
        }
    }

    int active_mask = 0;
    if (g_wall.active_turrets == 1) active_mask = (1 << 1);
    else if (g_wall.active_turrets == 2) active_mask = (1 << 1) | (1 << 2);
    else if (g_wall.active_turrets == 3) active_mask = (1 << 0) | (1 << 1) | (1 << 2);
    else active_mask = 0x0F;

    if (g_wall.locked_enemy_idx >= 0) {
        if (!g_enemies[g_wall.locked_enemy_idx].active) {
            g_wall.locked_enemy_idx = -1;
        }
    }

    int auto_fire = (g_game.upgrades.auto_target > 0) || (g_game.mode == MODE_DEBUG_SANDBOX);

    // Aiming, tracking, and firing for each active socket
    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        if (!(active_mask & (1 << s))) continue;
        int sx = c_wall_sockets[s].x;
        int sy = g_wall.screen_y + c_wall_sockets[s].y;

        int target_e = -1;
        if (g_wall.locked_enemy_idx >= 0) {
            target_e = g_wall.locked_enemy_idx;
        } else {
            int best_score = -99999;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!g_enemies[e].active) continue;
                int gx = FROM_FP(g_enemies[e].x);
                int gy = FROM_FP(g_enemies[e].y);
                if (gy > 192 + g_wall.screen_y + 10) continue; // Behind wall

                int h_dist = (gx > sx) ? (gx - sx) : (sx - gx);
                int v_score = (gy >= 192) ? (gy * 3) : gy;
                int score = v_score - h_dist;
                if (score > best_score) {
                    best_score = score;
                    target_e = e;
                }
            }
        }

        g_wall.target_enemy_idx[s] = target_e;

        // Desired angle towards target (or default stance if no target)
        int desired_angle = c_wall_sockets[s].default_angle;
        if (target_e >= 0) {
            int gx = FROM_FP(g_enemies[target_e].x);
            int local_gy = FROM_FP(g_enemies[target_e].y) - 192;
            desired_angle = wall_angle_from_target(sx, sy, gx, local_gy);
        }
        g_wall.target_angles[s] = desired_angle;

        // Smooth motorized traverse (1 step every 2 frames)
        if (g_wall.turret_angles[s] != g_wall.target_angles[s]) {
            g_wall.traverse_timer[s]++;
            if (g_wall.traverse_timer[s] >= 2) {
                g_wall.traverse_timer[s] = 0;
                if (g_wall.turret_angles[s] < g_wall.target_angles[s]) {
                    g_wall.turret_angles[s]++;
                } else {
                    g_wall.turret_angles[s]--;
                }
            }
        } else {
            g_wall.traverse_timer[s] = 0;
        }

        // Auto-firing: only when enemy is physically inside bottom screen AND within battery range!
        if (auto_fire && g_wall.turret_cooldown[s] == 0 && target_e >= 0) {
            int gy = FROM_FP(g_enemies[target_e].y);
            // Enemy must be on bottom screen (gy >= 192) and in front of wall
            if (gy >= 192 && gy < 192 + g_wall.screen_y) {
                int gx = FROM_FP(g_enemies[target_e].x);
                int local_gy = gy - 192;
                int tdx = gx - sx;
                int tdy = local_gy - sy;
                // Strict Euclidean distance within range
                if (tdx * tdx + tdy * tdy <= g_wall.range * g_wall.range) {
                    int angle_diff = g_wall.turret_angles[s] - g_wall.target_angles[s];
                    if (angle_diff >= -1 && angle_diff <= 1) {
                        int evx = FROM_FP(g_enemies[target_e].vx);
                        int evy = FROM_FP(g_enemies[target_e].vy);
                        int pred_x = gx + evx;
                        int pred_y = local_gy + evy;
                        wall_fire_socket(s, pred_x, pred_y);
                    }
                }
            }
        }
    }

    // Update Bullet Darts with sub-stepping (prevents tunneling through fast enemies)
    for (int i = 0; i < MAX_BULLET_DARTS; i++) {
        if (!g_bullet_darts[i].active) continue;

        int hit_enemy = 0;
        int hit_e_idx = -1;
        int hit_x = 0, hit_y = 0;

        // 2 sub-steps of 8 px per frame (16 px/frame bullet velocity)
        for (int step = 0; step < 2; step++) {
            g_bullet_darts[i].x += (g_bullet_darts[i].vx / 2);
            g_bullet_darts[i].y += (g_bullet_darts[i].vy / 2);
            g_bullet_darts[i].dist_remaining -= (TO_FP(16) / 2);

            int cur_x = FROM_FP(g_bullet_darts[i].x);
            int cur_y = FROM_FP(g_bullet_darts[i].y);
            int gy = cur_y + 192;

            if (cur_y < 0 || cur_x < 0 || cur_x >= SCREEN_W) {
                hit_enemy = 2; // Exited battlefield
                break;
            }

            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!g_enemies[e].active) continue;
                int ex = FROM_FP(g_enemies[e].x);
                int ey = FROM_FP(g_enemies[e].y);
                int ddx = cur_x - ex;
                int ddy = gy - ey;
                if (ddx * ddx + ddy * ddy <= 12 * 12) {
                    hit_enemy = 1;
                    hit_e_idx = e;
                    hit_x = cur_x;
                    hit_y = gy;
                    break;
                }
            }
            if (hit_enemy) break;
        }

        if (hit_enemy == 1 && hit_e_idx >= 0) {
            // Kinetic impact spark burst on target
            for (int k = 0; k < 3; k++) {
                game_add_splatter_ex(hit_x, hit_y, COLOR_BOLTER_TRACER, 0, 8);
            }
            if (g_enemies[hit_e_idx].hp > (uint64_t)g_bullet_darts[i].damage) {
                g_enemies[hit_e_idx].hp -= g_bullet_darts[i].damage;
            } else {
                g_enemies[hit_e_idx].hp = 0;
                g_enemies[hit_e_idx].active = 0;
                g_game.enemies_killed++;
                g_game.scrap += (5 * (g_enemies[hit_e_idx].variant + 1));
                game_spawn_death_gore(hit_x, hit_y, g_bullet_darts[i].vx, g_bullet_darts[i].vy, g_enemies[hit_e_idx].variant);
            }
            g_bullet_darts[i].active = 0;
        } else if (hit_enemy == 2 || g_bullet_darts[i].dist_remaining <= 0) {
            // Dissipate cleanly when reaching max range without false ground splatters
            g_bullet_darts[i].active = 0;
        }
    }

    // Update Casings
    for (int i = 0; i < MAX_CASINGS; i++) {
        if (!g_casings[i].active) continue;
        g_casings[i].x += g_casings[i].vx;
        g_casings[i].y += g_casings[i].vy;
        g_casings[i].z += g_casings[i].vz;
        g_casings[i].vz -= 140; // Gravity in Q8
        g_casings[i].angle = (g_casings[i].angle + g_casings[i].spin_speed) % 360;

        if (g_casings[i].z <= 0) {
            g_casings[i].z = 0;
            if (g_casings[i].bounces < 2) {
                g_casings[i].vz = -(g_casings[i].vz * 42) / 100;
                g_casings[i].vx = (g_casings[i].vx * 55) / 100;
                g_casings[i].vy = (g_casings[i].vy * 55) / 100;
                g_casings[i].spin_speed /= 2;
                g_casings[i].bounces++;
            } else {
                g_casings[i].vz = 0;
                g_casings[i].vx = 0;
                g_casings[i].vy = 0;
                g_casings[i].spin_speed = 0;
            }
        }

        g_casings[i].life--;
        if (g_casings[i].life <= 0) {
            g_casings[i].active = 0;
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

    wall_init();
    g_game.mode = MODE_PREPARATION;
    g_game.wave_number = 1;
    g_game.total_waves = 20;
    g_game.bunker_hp = g_balance.bunker_start_hp;
    g_game.bunker_max_hp = g_balance.bunker_start_hp;
    g_game.scrap = 10;
    g_game.fast_forward = 1;

    // Upgrades initial state
    g_game.upgrades.caliber_lvl = 0;
    g_game.upgrades.firerate_lvl = 0;
    g_game.upgrades.range_lvl = 0;
    g_game.upgrades.mag_size_lvl = 0;
    g_game.upgrades.auto_target = 1; // Core automated battery active by default!
    g_game.upgrades.conveyor_lvl = 0; // Starts requiring manual ammo drag!
    g_game.upgrades.extra_turrets = 0;

    // Debug Sandbox test defaults
    g_game.sandbox.enemy_tier = 1;
    g_game.sandbox.enemy_hp = 10;
    g_game.sandbox.enemy_speed = 30;
    g_game.sandbox.turret_firerate = 10;
    g_game.sandbox.turret_range = 80;
    g_game.sandbox.turret_damage = 5;
    g_game.sandbox.turret_infinite_ammo = 1;
    g_game.sandbox.run_sim = 1;
    g_game.sandbox.edit_row = 0;

    // WallPlatform handles all defenses; g_turrets[0].placed = 0;

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
    g_game.wave_timer = g_balance.wave_duration_frames;
    g_game.enemies_spawned = 0;
    g_game.enemies_alive = 0;
    g_game.spawn_timer = 0;

    int w_idx = g_game.wave_number - 1;
    if (w_idx < 0) w_idx = 0;
    if (w_idx >= 20) w_idx = 19;
    const WaveDef *wdef = &g_balance.waves[w_idx];

    g_game.enemies_to_spawn = 0;
    for (int t = 0; t < 8; t++) {
        const WaveTierConfig *tier_cfg = (t < 3) ? &wdef->tiers[t] : &g_balance.advanced_waves[w_idx][t - 3];
        g_game.enemies_to_spawn += tier_cfg->count;
        g_game.wave_spawned_tier[t] = 0;
        g_game.wave_spawn_timer_tier[t] = 0;
    }

    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_death_particles, 0, sizeof(g_death_particles));

    // Showcase swarm: Staggered across battlefield for immediate tactical engagement!
    // Vanguard entering bottom screen (Y = 200..215, approaching hazard line at Y=257)
    spawn_enemy_ex(0, 18, 48, 200);
    spawn_enemy_ex(0, 18, 46, 205);
    spawn_enemy_ex(1, 25, 40, 210);
    spawn_enemy_ex(1, 25, 38, 215);
    // Midguard advancing down top screen (Y = 110..140)
    spawn_enemy_ex(2, 75, 32, 130);
    spawn_enemy_ex(2, 75, 34, 140);
    spawn_enemy_ex(3, 160, 36, 110);
    spawn_enemy_ex(3, 160, 34, 120);
    // Rearguard colossi (Y = 0..70)
    spawn_enemy_ex(4, 320, 26, 60);
    spawn_enemy_ex(4, 320, 25, 70);
    spawn_enemy_ex(5, 500, 28, 30);
    spawn_enemy_ex(5, 500, 30, 40);
    spawn_enemy_ex(6, 1100, 22, 10);
    spawn_enemy_ex(7, 2600, 20, 0);

    // Reset locked targets
    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].locked_enemy_idx = -1;
    }
}

void game_reset_to_prep(void) {
    wall_init();
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
    if (g_game.mode != MODE_WAVE && g_game.mode != MODE_DEBUG_SANDBOX) return;

    g_game.sim_ticks_elapsed++;
    wall_update();
    if (g_game.wave_timer > 0) g_game.wave_timer--;

    int w_idx = g_game.wave_number - 1;
    if (w_idx < 0) w_idx = 0;
    if (w_idx >= 20) w_idx = 19;
    const WaveDef *wdef = &g_balance.waves[w_idx];

    // 1. Spawning per tier (all 8 enemy variants in roster)
    for (int t = 0; t < ENEMY_VARIANT_COUNT; t++) {
        // Map variant to wave config slots
        const WaveTierConfig *tier_cfg = (t < 3) ? &wdef->tiers[t] : &g_balance.advanced_waves[w_idx][t - 3];
        if (g_game.wave_spawned_tier[t] < tier_cfg->count) {
            g_game.wave_spawn_timer_tier[t]++;
            int interval = tier_cfg->delay;
            if (interval < 1) interval = 1;
            if (g_game.wave_spawn_timer_tier[t] >= interval) {
                g_game.wave_spawn_timer_tier[t] = 0;
                g_game.wave_spawned_tier[t]++;
                /* Base enemy HP is constant; waves tune composition and timing. */
                int hp = (int)g_balance.enemy_hp[t];
                if (hp < 1) hp = 1;
                spawn_enemy(t, hp, tier_cfg->speed);
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
        // Aerodynamic viscous drag: rapidly settles particles into local blast radius
        g_death_particles[i].vx = (g_death_particles[i].vx * 7) / 8;
        g_death_particles[i].vy = (g_death_particles[i].vy * 7) / 8;
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
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;

        int ex = g_enemies[i].x;
        int ey = g_enemies[i].y;
        int old_x = ex;
        int old_y = ey;
        int px = FROM_FP(ex);
        int py = FROM_FP(ey);

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
            int b_variant = g_enemies[i].variant;
            if (b_variant < 0) b_variant = 0;
            if (b_variant >= ENEMY_VARIANT_COUNT) b_variant = ENEMY_VARIANT_COUNT - 1;
            if (g_enemies[i].bite_timer >= g_balance.enemy_bite_interval[b_variant]) {
                g_enemies[i].bite_timer = 0;
                int bite_dmg = g_balance.enemy_bite_damage[b_variant];
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

        // Compute base movement speed (0 = stationary frozen dummy in sandbox)
        int spd = 0;
        if (g_enemies[i].speed > 0) {
            spd = (g_enemies[i].speed * FP_ONE) / 60;
            if (spd < 1) spd = 1;
        }

        // 4b. Soft separation repulsion between nearby enemies to prevent stacking/overlap
        int sep_force_x = 0;
        int sep_force_y = 0;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (i == j || !g_enemies[j].active) continue;
            // Only check enemies within a fast bounding box (Manhattan distance < 32 px)
            int odx = ex - g_enemies[j].x;
            int ody = ey - g_enemies[j].y;
            int aodx = (odx < 0) ? -odx : odx;
            int aody = (ody < 0) ? -ody : ody;
            if (aodx < TO_FP(24) && aody < TO_FP(24)) {
                int dist_sq = (aodx >> FP_SHIFT) * (aodx >> FP_SHIFT) + (aody >> FP_SHIFT) * (aody >> FP_SHIFT);
                // Repel if closer than 16 px center-to-center
                if (dist_sq < (16 * 16) && dist_sq > 0) {
                    // Small repulsive impulse in fixed point (~0.25 to 0.5 px)
                    int push = TO_FP(1) / 3;
                    if (odx > 0) sep_force_x += push;
                    else if (odx < 0) sep_force_x -= push;
                    if (ody > 0) sep_force_y += push / 2;
                    else if (ody < 0) sep_force_y -= push / 2;
                }
            }
        }
        ex += sep_force_x;
        // Keep within battlefield bounds [8..248]
        if (ex < TO_FP(8)) ex = TO_FP(8);
        if (ex > TO_FP(248)) ex = TO_FP(248);
        g_enemies[i].x = ex;

        // Check if reaching Bunker Sanctum baseline (py >= 344):
        // Never teleport! Stagger frontline along bunker wall (x: 88..168) using index-based slot
        int slot_x = 88 + ((i * 13) % 80); // Distributed slots across 80 px width
        if (py >= 344) {
            g_enemies[i].y = TO_FP(344);
            g_enemies[i].vy = 0;

            if (px < slot_x - 3) {
                g_enemies[i].x += spd;
                g_enemies[i].vx = spd;
                g_enemies[i].dir = 2; // East (dir 2 = East in 8-way compass)
                g_enemies[i].biting_target = -1;
                continue;
            } else if (px > slot_x + 3) {
                g_enemies[i].x -= spd;
                g_enemies[i].vx = -spd;
                g_enemies[i].dir = 6; // West (dir 6 = West in 8-way compass)
                g_enemies[i].biting_target = -1;
                continue;
            }

            // Arrived at Bunker slot: bite the Sanctum!
            g_enemies[i].vx = 0;
            g_enemies[i].dir = 4; // Face South against the bunker wall
            g_enemies[i].biting_target = 99;
            g_enemies[i].bite_timer++;

            // Cycle attack animation while attacking
            const EnemyTypeDef *type = &g_enemy_types[g_enemies[i].variant];
            if (type->attack_frame_count > 0) {
                g_enemies[i].anim_frame = (g_enemies[i].bite_timer / 6) % type->attack_frame_count;
            }

            if (g_enemies[i].bite_timer >= 40) {
                g_enemies[i].bite_timer = 0;
                int b_variant = g_enemies[i].variant;
                if (b_variant < 0) b_variant = 0;
                if (b_variant >= ENEMY_VARIANT_COUNT) b_variant = ENEMY_VARIANT_COUNT - 1;
                uint64_t bite_dmg = g_balance.enemy_bite_damage[b_variant];
                if (g_game.mode != MODE_DEBUG_SANDBOX) {
                    if (g_game.bunker_hp > bite_dmg) {
                        g_game.bunker_hp -= bite_dmg;
                    } else {
                        g_game.bunker_hp = 0;
                        g_game.mode = MODE_GAME_OVER;
                        return;
                    }
                }
            }
            continue;
        }

        // Advance: vertical downward in parallel lanes across top screen and upper bottom screen
        g_enemies[i].y += spd + sep_force_y;
        g_enemies[i].vy = spd;
        g_enemies[i].vx = sep_force_x;

        // Funnel X towards Sanctum bunker slot in lower bottom screen (py > 255)
        if (spd > 0 && py > 255) {
            int target_x = TO_FP(slot_x);
            int h_spd = spd / 3;
            if (h_spd < 1) h_spd = 1;
            if (ex < target_x - TO_FP(4)) {
                ex += h_spd;
                g_enemies[i].vx = h_spd;
            } else if (ex > target_x + TO_FP(4)) {
                ex -= h_spd;
                g_enemies[i].vx = -h_spd;
            }
            g_enemies[i].x = ex;
        }

        int move_dir = enemy_direction_from_delta(g_enemies[i].x - old_x,
                                                   g_enemies[i].y - old_y);
        int move_dx = g_enemies[i].x - old_x;
        int move_dy = g_enemies[i].y - old_y;
        int move_ax = (move_dx < 0) ? -move_dx : move_dx;
        int move_ay = (move_dy < 0) ? -move_dy : move_dy;
        int move_distance = move_ax + move_ay;
        if (move_dir >= 0) g_enemies[i].dir = move_dir;

        // Walk animation is distance-based: one pose per four pixels moved.
        // This keeps slow and fast enemy tiers visually synchronized.
        if (move_distance > 0) {
            g_enemies[i].anim_distance += move_distance;
            int walk_frames = g_enemy_types[g_enemies[i].variant].frame_count;
            if (walk_frames < 1) walk_frames = 4;
            while (g_enemies[i].anim_distance >= (4 * FP_ONE)) {
                g_enemies[i].anim_distance -= (4 * FP_ONE);
                g_enemies[i].anim_frame++;
                if (g_enemies[i].anim_frame >= walk_frames) {
                    g_enemies[i].anim_frame = 0;
                }
            }
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
            int rate = (g_game.upgrades.conveyor_lvl <= 3) ? g_balance.conveyor_reload_interval[g_game.upgrades.conveyor_lvl] : g_balance.conveyor_reload_interval[3];
            if (g_game.sim_ticks_elapsed % rate == 0) {
                tur->ammo++;
            }
        }

        // Dynamic range based on upgrade (or sandbox override)
        if (g_game.mode == MODE_DEBUG_SANDBOX) {
            tur->range = g_game.sandbox.turret_range;
            if (g_game.sandbox.turret_infinite_ammo) {
                tur->ammo = tur->max_ammo;
            }
        } else {
            int r_lvl = g_game.upgrades.range_lvl;
            tur->range = (r_lvl < 5) ? g_balance.turret_range[r_lvl] : g_balance.turret_range[4];
        }

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

        // In debug sandbox: auto-acquire closest enemy within range if none locked
        if (target_enemy < 0 && g_game.mode == MODE_DEBUG_SANDBOX) {
            int closest_dist_sq = tur->range * tur->range;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!g_enemies[e].active) continue;
                int gy = FROM_FP(g_enemies[e].y);
                if (gy < 192) continue;
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
        }

        // Aim and fire
        if (target_enemy >= 0) {
            int gx = FROM_FP(g_enemies[target_enemy].x);
            int local_y = FROM_FP(g_enemies[target_enemy].y) - 192;

            // Lead target prediction: calculate future intercept point based on bullet flight time
            int ddx = gx - tur->x;
            int ddy = local_y - tur->y;
            int dsq = ddx * ddx + ddy * ddy;
            int dist = 0;
            while ((dist + 1) * (dist + 1) <= dsq) dist++;

            int t_frames = dist / BULLET_SPEED;
            if (t_frames < 1) t_frames = 1;

            int evx_px = FROM_FP(g_enemies[target_enemy].vx);
            int evy_px = FROM_FP(g_enemies[target_enemy].vy);

            int pred_x = gx + (evx_px * t_frames);
            int pred_y = local_y + (evy_px * t_frames);

            // Clamp predicted point inside valid battlefield bounds
            if (pred_x < 8) pred_x = 8;
            if (pred_x > 248) pred_x = 248;
            if (pred_y < 0) pred_y = 0;
            if (pred_y > 185) pred_y = 185;

            tur->target_angle = fixed_atan2(pred_y - tur->y, pred_x - tur->x);
            tur->current_angle = tur->target_angle;

            // Firing strictly consumes ammo!
            if (tur->ammo > 0) {
                if (tur->fire_cooldown > 0) {
                    tur->fire_cooldown--;
                } else {
                    int f_lvl = g_game.upgrades.firerate_lvl;
                    int interval = (g_game.mode == MODE_DEBUG_SANDBOX) ? g_game.sandbox.turret_firerate :
                                   ((f_lvl < 5) ? g_balance.turret_fire_interval[f_lvl] : g_balance.turret_fire_interval[4]);
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

                    // Dual barrel convergence directly towards predicted impact point
                    int fire_angle = fixed_atan2(pred_y - by, pred_x - bx);

                    // Multiplicative damage: Base 2 -> 3 -> 4 -> 6 -> 8
                    int c_lvl = g_game.upgrades.caliber_lvl;
                    uint64_t dmg = (g_game.mode == MODE_DEBUG_SANDBOX) ? (uint64_t)g_game.sandbox.turret_damage :
                                   ((c_lvl < 5) ? g_balance.turret_damage[c_lvl] : g_balance.turret_damage[4]);

                    spawn_bullet(bx, by, fire_angle, t, dmg);
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

    // 6. Update Bullets & Continuous Collisions
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!g_bullets[b].active) continue;
        int prev_bx = FROM_FP(g_bullets[b].x);
        int prev_by = FROM_FP(g_bullets[b].y);

        g_bullets[b].x += g_bullets[b].vx;
        g_bullets[b].y += g_bullets[b].vy;
        g_bullets[b].life--;

        int curr_bx = FROM_FP(g_bullets[b].x);
        int curr_by = FROM_FP(g_bullets[b].y);
        int mid_bx = (prev_bx + curr_bx) / 2;
        int mid_by = (prev_by + curr_by) / 2;

        int hit = 0;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue; // Only collide in bottom screen
            int local_y = gy - 192;
            int gx = FROM_FP(g_enemies[e].x);

            static const int s_hit_r[ENEMY_VARIANT_COUNT] = { 8, 7, 9, 12, 14, 14, 16, 22 };
            int v = g_enemies[e].variant;
            if (v < 0) v = 0;
            if (v >= ENEMY_VARIANT_COUNT) v = ENEMY_VARIANT_COUNT - 1;
            int r = s_hit_r[v];

            // Check collision at both current position and midpoint to prevent tunneling
            int hit_curr = (abs(curr_bx - gx) <= r && abs(curr_by - local_y) <= r);
            int hit_mid = (abs(mid_bx - gx) <= r && abs(mid_by - local_y) <= r);

            if (hit_curr || hit_mid) {
                hit = 1;
                uint64_t dmg = g_bullets[b].damage;
                if (g_enemies[e].hp > dmg) {
                    g_enemies[e].hp -= dmg;
                } else {
                    g_enemies[e].hp = 0;
                    g_enemies[e].active = 0;
                    g_game.enemies_alive--;
                    g_game.enemies_killed++;

                    uint64_t base_scrap = g_balance.enemy_scrap[v];
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

        if (!hit && (g_bullets[b].life <= 0 || curr_bx < 0 || curr_bx >= SCREEN_W || curr_by < 0 || curr_by >= SCREEN_H)) {
            g_bullets[b].active = 0;
        }
    }

    // 7. Wave Completion
    if (g_game.enemies_spawned >= g_game.enemies_to_spawn && g_game.enemies_alive == 0) {
        // Wave clear bonus scrap (+10 in W1, +15 in W2, etc.)
        uint64_t wave_bonus = g_balance.wave_bonus_base + (g_game.wave_number * g_balance.wave_bonus_per_wave);
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

    // X opens upgrades (SELECT handled in main loop)
    if (keys_down & KEY_X) {
        g_game.previous_mode = g_game.mode;
        g_game.mode = MODE_UPGRADES;
        return;
    }

    if (keys_down & KEY_R) {
        g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
    }

    static int s_prep_touching = 0;
    int is_touch = (keys_held & KEY_TOUCH) || (touch.px > 0 && touch.py > 0);
    int touch_press = ((keys_down & KEY_TOUCH) || (is_touch && !s_prep_touching));
    s_prep_touching = is_touch;

    if (touch_press) {
        // [START] Button: drawn at (190, 150, 60, 36) -> hitbox (190..255, 144..191)
        if (touch.px >= 190 && touch.px <= 255 && touch.py >= 144 && touch.py <= 191) {
            game_start_wave();
            return;
        }

        // [UPGRADES] Button: drawn at (54, 156, 48, 24) -> hitbox (52..104, 144..191)
        if (touch.px >= 52 && touch.px <= 104 && touch.py >= 144 && touch.py <= 191) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_UPGRADES;
            return;
        }

        // [CALIB] Button: drawn at (108, 156, 36, 24) -> hitbox (106..145, 144..191)
        if (touch.px >= 106 && touch.px <= 145 && touch.py >= 144 && touch.py <= 191) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_CALIBRATION;
            return;
        }

        // [SANDBOX] Button: drawn at (148, 156, 36, 24) -> hitbox (146..188, 144..191)
        if (touch.px >= 146 && touch.px <= 188 && touch.py >= 144 && touch.py <= 191) {
            g_game.previous_mode = g_game.mode;
            g_game.mode = MODE_DEBUG_SANDBOX;
            // g_turrets deprecated
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

    static int s_wave_touching = 0;
    int is_touch = (keys_held & KEY_TOUCH) || (touch.px > 0 && touch.py > 0);
    int touch_press = ((keys_down & KEY_TOUCH) || (is_touch && !s_wave_touching));
    s_wave_touching = is_touch;

    // Touch down: Stylus manual targeting - only fires if an enemy is in the area or tapped!
    if (touch_press && touch.px > 0 && touch.py > 14 && touch.py < g_wall.screen_y) {
        int clicked_enemy = -1;
        int best_dist_sq = 28 * 28;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue;
            int local_y = gy - 192;
            int gx = FROM_FP(g_enemies[e].x);
            int ddx = touch.px - gx;
            int ddy = touch.py - local_y;
            int dsq = ddx * ddx + ddy * ddy;
            if (dsq <= best_dist_sq) {
                best_dist_sq = dsq;
                clicked_enemy = e;
            }
        }
        if (clicked_enemy >= 0) {
            g_wall.locked_enemy_idx = clicked_enemy;
            int ex = FROM_FP(g_enemies[clicked_enemy].x);
            int ey = FROM_FP(g_enemies[clicked_enemy].y) - 192;
            wall_fire_at(ex, ey);
        }
    }

    if (touch_press) {
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

        // Target designated enemy in bottom screen with stylus (generous closest selection)
        int clicked_enemy = -1;
        int best_dist_sq = 28 * 28;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue;
            int local_y = gy - 192;
            int gx = FROM_FP(g_enemies[e].x);

            int ddx = touch.px - gx;
            int ddy = touch.py - local_y;
            int dsq = ddx * ddx + ddy * ddy;
            if (dsq <= best_dist_sq) {
                best_dist_sq = dsq;
                clicked_enemy = e;
            }
        }

        if (clicked_enemy >= 0) {
            g_wall.locked_enemy_idx = clicked_enemy;
            for (int t = 0; t < MAX_TURRETS; t++) {
                if (g_turrets[t].placed) {
                    g_turrets[t].locked_enemy_idx = clicked_enemy;
                }
            }
            // Tap directly fires at locked enemy!
            int ex = FROM_FP(g_enemies[clicked_enemy].x);
            int ey = FROM_FP(g_enemies[clicked_enemy].y) - 192;
            wall_fire_at(ex, ey);
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
    /* Costs are part of the persisted balance so calibration can tune them. */
    if (idx == 7) {
        int level = g_game.upgrades.range_lvl;
        return (level >= 0 && level < 5) ? g_balance.range_upgrade_costs[level] : 999999;
    }
    if (idx >= 0 && idx < 7) {
        int levels[7] = { g_game.upgrades.caliber_lvl, g_game.upgrades.firerate_lvl,
            g_game.upgrades.mag_size_lvl, g_game.upgrades.bio_harvest_lvl,
            g_game.upgrades.conveyor_lvl, g_game.upgrades.auto_target,
            g_game.upgrades.extra_turrets };
        int level = levels[idx];
        if (level >= 0 && level < 5 && g_balance.upgrade_costs[idx][level] > 0)
            return g_balance.upgrade_costs[idx][level];
        return 999999;
    }
    /* Legacy switch retained only as a defensive fallback for invalid callers. */
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
                int m = (g_game.upgrades.mag_size_lvl < 6) ? g_balance.turret_magazine[g_game.upgrades.mag_size_lvl] : g_balance.turret_magazine[5];
                g_turrets[t].max_ammo = m;
            }
            break;
        }
        case 3: g_game.upgrades.bio_harvest_lvl++; break;
        case 4: g_game.upgrades.conveyor_lvl++; break;
        case 5: g_game.upgrades.auto_target = 1; break;
        case 6:
            g_game.upgrades.extra_turrets++;
            if (g_game.upgrades.extra_turrets > MAX_TURRETS - 1) g_game.upgrades.extra_turrets = MAX_TURRETS - 1;
            for (int t = 1; t <= g_game.upgrades.extra_turrets && t < MAX_TURRETS; t++) {
                if (!g_turrets[t].placed) {
                    static const int sx[3] = { 82, 174, 128 };
                    static const int sy[3] = { 110, 110, 86 };
                    g_turrets[t].id = t; g_turrets[t].type = TURRET_TYPE_BOLTER;
                    g_turrets[t].x = sx[t - 1]; g_turrets[t].y = sy[t - 1];
                    g_turrets[t].current_angle = 192; g_turrets[t].target_angle = 192;
                    g_turrets[t].range = g_balance.turret_range[g_game.upgrades.range_lvl < 5 ? g_game.upgrades.range_lvl : 4];
                    g_turrets[t].placed = 1; g_turrets[t].active = 1; g_turrets[t].hp = 50; g_turrets[t].max_hp = 50;
                    g_turrets[t].ammo = g_balance.turret_magazine[g_game.upgrades.mag_size_lvl < 6 ? g_game.upgrades.mag_size_lvl : 5];
                    g_turrets[t].max_ammo = g_turrets[t].ammo; g_turrets[t].fire_interval = g_balance.turret_fire_interval[0];
                    g_turrets[t].locked_enemy_idx = -1;
                }
            }
            break;
        case 7: g_game.upgrades.range_lvl++; break;
    }
}

void game_handle_input_upgrades(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & (KEY_B | KEY_START)) {
        g_game.mode = g_game.previous_mode;
        return;
    }

    if (keys_down & KEY_TOUCH) {
        // Return button: (90, 150, 76, 28)
        if (touch.px >= 125 && touch.px <= 210 && touch.py >= 170 && touch.py <= 191) {
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
        // Tab 7: Extra turrets (10, 138, 110, 32)
        else if (touch.px >= 10 && touch.px <= 120 && touch.py >= 136 && touch.py <= 172) {
            upgrade_purchase(6);
        }
        else if (touch.px >= 130 && touch.px <= 240 && touch.py >= 136 && touch.py <= 172) {
            upgrade_purchase(7);
        }
    }
}

static void calib_modify_val(int delta) {
    if (g_game.calib_page == 1) {
        int enemy = g_game.calib_row / 4, field = g_game.calib_row % 4;
        if (enemy < 0) enemy = 0;
        if (enemy >= ENEMY_VARIANT_COUNT) enemy = ENEMY_VARIANT_COUNT - 1;
        if (field == 0) { int n = (int)g_balance.enemy_hp[enemy] + delta; if (n < 1) n = 1; g_balance.enemy_hp[enemy] = (uint32_t)n; }
        else if (field == 1) { int n = (int)g_balance.enemy_scrap[enemy] + delta; if (n < 0) n = 0; g_balance.enemy_scrap[enemy] = (uint32_t)n; }
        else if (field == 2) { g_balance.enemy_bite_damage[enemy] += delta; if (g_balance.enemy_bite_damage[enemy] < 1) g_balance.enemy_bite_damage[enemy] = 1; }
        else { g_balance.enemy_bite_interval[enemy] += delta; if (g_balance.enemy_bite_interval[enemy] < 1) g_balance.enemy_bite_interval[enemy] = 1; }
        g_game.calib_saved_timer = 20; balance_config_save(); return;
    }
    if (g_game.calib_page == 2) {
        int r = g_game.calib_row;
        if (r < 4) { int *p[4] = { &g_balance.bunker_start_hp, &g_balance.wave_duration_frames, &g_balance.wave_bonus_base, &g_balance.wave_bonus_per_wave }; *p[r] += delta; if (*p[r] < 0) *p[r] = 0; }
        else if (r < 9) { int i = r - 4; g_balance.turret_damage[i] += delta; if (g_balance.turret_damage[i] < 1) g_balance.turret_damage[i] = 1; }
        else if (r < 14) { int i = r - 9; g_balance.turret_range[i] += delta * 2; if (g_balance.turret_range[i] < 1) g_balance.turret_range[i] = 1; }
        else if (r < 18) { int i = r - 14; g_balance.conveyor_reload_interval[i] += delta * 5; if (g_balance.conveyor_reload_interval[i] < 1) g_balance.conveyor_reload_interval[i] = 1; }
        else { int i = r - 18; g_balance.turret_magazine[i] += delta; if (g_balance.turret_magazine[i] < 1) g_balance.turret_magazine[i] = 1; }
        g_game.calib_saved_timer = 20; balance_config_save(); return;
    }
    if (g_game.calib_page == 3) {
        int *v = 0;
        switch (g_game.calib_row) {
            case 0: v = &g_balance.bunker_start_hp; break;
            case 1: v = &g_balance.wave_duration_frames; break;
            case 2: v = &g_balance.wave_bonus_base; break;
            case 3: v = &g_balance.wave_bonus_per_wave; break;
        }
        if (v) { *v += delta; if (*v < 0) *v = 0; if (*v > 999999) *v = 999999; }
        else if (g_game.calib_row >= 4 && g_game.calib_row < 9) {
            int i = g_game.calib_row - 4;
            g_balance.turret_damage[i] += delta;
            if (g_balance.turret_damage[i] < 1) g_balance.turret_damage[i] = 1;
        }
        else if (g_game.calib_row >= 9 && g_game.calib_row < 14) {
            int i = g_game.calib_row - 9;
            g_balance.turret_range[i] += delta * 2;
            if (g_balance.turret_range[i] < 1) g_balance.turret_range[i] = 1;
        }
        else if (g_game.calib_row >= 14 && g_game.calib_row < 20) {
            int i = g_game.calib_row - 14;
            int n = (int)g_balance.enemy_hp[i] + delta;
            if (n < 1) n = 1;
            if (n > 999999) n = 999999;
            g_balance.enemy_hp[i] = (uint32_t)n;
        }
        else if (g_game.calib_row >= 20 && g_game.calib_row < 26) {
            int i = g_game.calib_row - 20;
            int n = (int)g_balance.enemy_scrap[i] + delta;
            if (n < 0) n = 0;
            if (n > 999999) n = 999999;
            g_balance.enemy_scrap[i] = (uint32_t)n;
        }
        else if (g_game.calib_row >= 26 && g_game.calib_row < 32) {
            int i = g_game.calib_row - 26;
            g_balance.enemy_bite_damage[i] += delta;
            if (g_balance.enemy_bite_damage[i] < 1) g_balance.enemy_bite_damage[i] = 1;
        }
        else if (g_game.calib_row >= 32 && g_game.calib_row < 36) {
            int i = g_game.calib_row - 32;
            g_balance.conveyor_reload_interval[i] += delta * 5;
            if (g_balance.conveyor_reload_interval[i] < 1) g_balance.conveyor_reload_interval[i] = 1;
        }
        else if (g_game.calib_row >= 36 && g_game.calib_row < 42) {
            int i = g_game.calib_row - 36;
            g_balance.enemy_bite_interval[i] += delta;
            if (g_balance.enemy_bite_interval[i] < 1) g_balance.enemy_bite_interval[i] = 1;
        }
        else if (g_game.calib_row >= 42 && g_game.calib_row < 48) {
            int i = g_game.calib_row - 42;
            g_balance.turret_magazine[i] += delta;
            if (g_balance.turret_magazine[i] < 1) g_balance.turret_magazine[i] = 1;
        }
        g_game.calib_saved_timer = 20; balance_config_save(); return;
    }
    if (g_game.calib_page == 3) {
        if (g_game.calib_row >= 35 && g_game.calib_row < 40) {
            int level = g_game.calib_row - 35;
            int64_t n = (int64_t)g_balance.range_upgrade_costs[level] + delta;
            if (n < 0) n = 0;
            if (n > 999999) n = 999999;
            g_balance.range_upgrade_costs[level] = (uint64_t)n;
            g_game.calib_saved_timer = 20; balance_config_save(); return;
        }
        int up = g_game.calib_row / 5, level = g_game.calib_row % 5;
        if (up >= 0 && up < 7) {
            int64_t n = (int64_t)g_balance.upgrade_costs[up][level] + delta;
            if (n < 0) n = 0;
            if (n > 999999) n = 999999;
            g_balance.upgrade_costs[up][level] = (uint64_t)n;
        }
        g_game.calib_saved_timer = 20; balance_config_save(); return;
    }
    int w = g_game.calib_wave_idx;
    if (w < 0) w = 0;
    if (w >= 20) w = 19;
    WaveDef *wd = &g_balance.waves[w];

    int row = g_game.calib_row;
    if (row < 0) row = 0;
    if (row >= 32) row = 31;

    if (row == 11) {
        wd->scrap_base += delta;
        if (wd->scrap_base < 0) wd->scrap_base = 0;
        if (wd->scrap_base > 999999) wd->scrap_base = 999999;
        g_game.calib_saved_timer = 20;
        balance_config_save();
        return;
    }
    if (row >= 12 && row < 32) {
        int tier = 3 + ((row - 12) / 4), param = (row - 12) % 4;
        if (tier >= 8) tier = 7;
        WaveTierConfig *tc = &g_balance.advanced_waves[w][tier - 3];
        if (param == 3) {
            int hp = (int)g_balance.enemy_hp[tier] + delta;
            if (hp < 1) hp = 1;
            if (hp > 999999) hp = 999999;
            g_balance.enemy_hp[tier] = (uint32_t)hp;
        } else {
            int *v = (param == 0) ? &tc->count : (param == 1) ? &tc->delay : &tc->speed;
            *v += delta;
            if (*v < 0) *v = (param == 0) ? 0 : 1;
            if (*v > 999999) *v = 999999;
        }
        g_game.calib_saved_timer = 20; balance_config_save(); return;
    }
    if ((row % 4) == 3) {
        int tier_hp = row / 4;
        g_balance.enemy_hp[tier_hp] += delta;
        if (g_balance.enemy_hp[tier_hp] < 1) g_balance.enemy_hp[tier_hp] = 1;
        if (g_balance.enemy_hp[tier_hp] > 999999) g_balance.enemy_hp[tier_hp] = 999999;
        g_game.calib_saved_timer = 20;
        balance_config_save();
        return;
    }

    int tier = row / 4;
    int param = row % 4;
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
        case 3: // HP (1..999999)
            tc->hp += delta;
            if (tc->hp < 1) tc->hp = 1;
            if (tc->hp > 999999) tc->hp = 999999;
            break;
    }

    g_game.calib_saved_timer = 20;
    balance_config_save();
}

void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held) {
    // B exits calibration back to preparation
    if (keys_down & KEY_B) {
        wall_init();
    g_game.mode = MODE_PREPARATION;
        return;
    }

    // X / Y cycle pages (0..3)
    if (keys_down & KEY_X) {
        g_game.calib_page = (g_game.calib_page + 3) % 4;
        g_game.calib_row = 0;
    }
    if (keys_down & KEY_Y) {
        g_game.calib_page = (g_game.calib_page + 1) % 4;
        g_game.calib_row = 0;
    }

    // L / R cycle wave index (0..19)
    if (keys_down & KEY_L) {
        g_game.calib_wave_idx = (g_game.calib_wave_idx + 19) % 20;
    }
    if (keys_down & KEY_R) {
        g_game.calib_wave_idx = (g_game.calib_wave_idx + 1) % 20;
    }

    // Up / Down: select parameter row. Held buttons repeat, then accelerate.
    int max_rows = (g_game.calib_page == 0) ? 32 : ((g_game.calib_page == 1) ? 32 : ((g_game.calib_page == 2) ? 24 : 40));
    int nav_dir = 0;
    if (keys_down & KEY_UP) {
        g_game.calib_row = (g_game.calib_row + max_rows - 1) % max_rows;
        g_game.calib_hold_timer = 0;
        nav_dir = -1;
    }
    if (keys_down & KEY_DOWN) {
        g_game.calib_row = (g_game.calib_row + 1) % max_rows;
        g_game.calib_hold_timer = 0;
        nav_dir = 1;
    }
    if (!nav_dir && (keys_held & (KEY_UP | KEY_DOWN))) {
        g_game.calib_hold_timer++;
        if (g_game.calib_hold_timer >= 10) {
            int repeat_every = (g_game.calib_hold_timer >= 45) ? 1 : 3;
            if ((g_game.calib_hold_timer % repeat_every) == 0) {
                nav_dir = (keys_held & KEY_UP) ? -1 : 1;
                g_game.calib_row = (g_game.calib_row + max_rows + nav_dir) % max_rows;
            }
        }
    }

    // Left / Right with continuous autorepeat
    int step = 1;
    int param = (g_game.calib_page == 0) ? (g_game.calib_row % 4) : 0;
    if (param == 0) step = 1; // count
    else if (param == 1) step = 5; // delay
    else if (param == 2) step = 2; // speed
    else if (param == 3) step = 1; // hp
    if (g_game.calib_page != 0 || g_game.calib_row == 11) step = 1;

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
        // Visible page navigation fallback: touch the header.
        if (touch.py >= 0 && touch.py <= 27 && touch.px >= 150 && touch.px <= 205) {
            g_game.calib_page = (g_game.calib_page + 3) % 4;
            g_game.calib_row = 0;
            return;
        }
        if (touch.py >= 0 && touch.py <= 27 && touch.px > 205) {
            g_game.calib_page = (g_game.calib_page + 1) % 4;
            g_game.calib_row = 0;
            return;
        }
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

        // Parameter rows touch hitboxes (12 rows)
        int visible_first = (g_game.calib_page == 0) ? (g_game.calib_row / 10) * 10 : 0;
        int visible_count = (g_game.calib_page == 0) ? 10 : 12;
        for (int r = 0; r < visible_count; r++) {
            int actual = visible_first + r;
            int ry = 31 + r * 10;
            if (touch.py >= ry && touch.py <= ry + 9) {
                g_game.calib_row = actual;
                int rparam = actual % 4;
                int rstep = (r == 11 || rparam == 0 || rparam == 3) ? 1 : ((rparam == 1) ? 5 : 2);
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


void game_sandbox_spawn_enemy(int x, int y) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            g_enemies[i].active = 1;
            g_enemies[i].variant = g_game.sandbox.enemy_tier;
            g_enemies[i].hp = g_game.sandbox.enemy_hp;
            g_enemies[i].max_hp = g_game.sandbox.enemy_hp;
            g_enemies[i].x = TO_FP(x);
            g_enemies[i].y = TO_FP(y);
            g_enemies[i].speed = g_game.sandbox.enemy_speed;
            g_enemies[i].dir = 4; // South in the 8-way compass
            g_enemies[i].anim_frame = 0;
            g_enemies[i].biting_target = -1;
            g_enemies[i].bite_timer = 0;
            g_enemies[i].vx = 0;
            g_enemies[i].vy = (g_enemies[i].speed * FP_ONE) / 60;
            g_game.enemies_alive++;
            g_game.sandbox.spawn_count++;
            break;
        }
    }
}

void game_handle_input_sandbox(touchPosition touch, int keys_down, int keys_held) {
    // START toggles live simulation running / paused
    if (keys_down & KEY_START) {
        g_game.sandbox.run_sim = !g_game.sandbox.run_sim;
        return;
    }

    // Y advances exactly 1 frame (single step execution)
    if (keys_down & KEY_Y) {
        game_update_simulation();
        return;
    }

    // X clears all battlefield entities
    if (keys_down & KEY_X) {
        memset(g_enemies, 0, sizeof(g_enemies));
        memset(g_bullets, 0, sizeof(g_bullets));
        g_game.enemies_alive = 0;
        return;
    }

    // D-Pad UP / DOWN selects parameter row (0..5)
    if (keys_down & KEY_UP) {
        g_game.sandbox.edit_row = (g_game.sandbox.edit_row + 5) % 6;
    } else if (keys_down & KEY_DOWN) {
        g_game.sandbox.edit_row = (g_game.sandbox.edit_row + 1) % 6;
    }

    // D-Pad LEFT / RIGHT adjusts selected parameter
    int delta = 0;
    if (keys_down & KEY_LEFT) delta = -1;
    else if (keys_down & KEY_RIGHT) delta = 1;

    if (delta != 0) {
        switch (g_game.sandbox.edit_row) {
            case 0: { // Enemy Tier (0..5)
                int t = g_game.sandbox.enemy_tier + delta;
                if (t < 0) t = 0;
                if (t >= ENEMY_VARIANT_COUNT) t = ENEMY_VARIANT_COUNT - 1;
                g_game.sandbox.enemy_tier = t;
                break;
            }
            case 1: { // Enemy HP
                static const int s_hp_steps[] = { 1, 5, 10, 25, 50, 100, 250, 500, 1000, 5000 };
                int idx = 2;
                for (int j = 0; j < 10; j++) {
                    if (s_hp_steps[j] == g_game.sandbox.enemy_hp) { idx = j; break; }
                }
                idx += delta;
                if (idx < 0) idx = 0;
                if (idx > 9) idx = 9;
                g_game.sandbox.enemy_hp = s_hp_steps[idx];
                break;
            }
            case 2: { // Enemy Speed
                static const int s_spd_steps[] = { 0, 15, 30, 45, 60, 90, 120, 180 };
                int idx = 2;
                for (int j = 0; j < 8; j++) {
                    if (s_spd_steps[j] == g_game.sandbox.enemy_speed) { idx = j; break; }
                }
                idx += delta;
                if (idx < 0) idx = 0;
                if (idx > 7) idx = 7;
                g_game.sandbox.enemy_speed = s_spd_steps[idx];
                break;
            }
            case 3: { // Turret Range
                static const int s_rng_steps[] = { 40, 65, 80, 100, 125, 150, 200 };
                int idx = 2;
                for (int j = 0; j < 7; j++) {
                    if (s_rng_steps[j] == g_game.sandbox.turret_range) { idx = j; break; }
                }
                idx += delta;
                if (idx < 0) idx = 0;
                if (idx > 6) idx = 6;
                g_game.sandbox.turret_range = s_rng_steps[idx];
                break;
            }
            case 4: { // Turret Fire Interval (cadence)
                static const int s_int_steps[] = { 1, 3, 5, 8, 12, 18, 25, 35 };
                int idx = 4;
                for (int j = 0; j < 8; j++) {
                    if (s_int_steps[j] == g_game.sandbox.turret_firerate) { idx = j; break; }
                }
                idx += delta;
                if (idx < 0) idx = 0;
                if (idx > 7) idx = 7;
                g_game.sandbox.turret_firerate = s_int_steps[idx];
                break;
            }
            case 5: { // Turret Damage
                static const int s_dmg_steps[] = { 1, 2, 3, 5, 10, 20, 50, 100 };
                int idx = 3;
                for (int j = 0; j < 8; j++) {
                    if (s_dmg_steps[j] == g_game.sandbox.turret_damage) { idx = j; break; }
                }
                idx += delta;
                if (idx < 0) idx = 0;
                if (idx > 7) idx = 7;
                g_game.sandbox.turret_damage = s_dmg_steps[idx];
                break;
            }
        }
    }

    // Touch handling
    static int s_sb_touching = 0;
    int is_touch = (keys_held & KEY_TOUCH) || (touch.px > 0 && touch.py > 0);
    int touch_press = ((keys_down & KEY_TOUCH) || (is_touch && !s_sb_touching));
    s_sb_touching = is_touch;

    if (touch_press) {
        // Bottom control bar buttons:
        if (touch.py >= 148) {
            // [CLEAR] (6..50)
            if (touch.px >= 6 && touch.px <= 50) {
                memset(g_enemies, 0, sizeof(g_enemies));
                memset(g_bullets, 0, sizeof(g_bullets));
                g_game.enemies_alive = 0;
                return;
            }
            // [RUN/PAUSE] (54..110)
            if (touch.px >= 54 && touch.px <= 110) {
                g_game.sandbox.run_sim = !g_game.sandbox.run_sim;
                return;
            }
            // [STEP 1F] (114..160)
            if (touch.px >= 114 && touch.px <= 160) {
                game_update_simulation();
                return;
            }
            // [INF AMMO] (164..205)
            if (touch.px >= 164 && touch.px <= 205) {
                g_game.sandbox.turret_infinite_ammo = !g_game.sandbox.turret_infinite_ammo;
                return;
            }
            // [EXIT] (210..252)
            if (touch.px >= 210 && touch.px <= 252) {
                g_game.mode = g_game.previous_mode;
                return;
            }
        } else {
            // Touch in battlefield: spawn chosen enemy right at touch coordinates!
            game_sandbox_spawn_enemy(touch.px, touch.py);
            return;
        }
    }
}
