#include "game.h"
#include "enemy_data.h"
#include "tiles.h"
#include "turret_data.h"

GameContext g_game;
Turret g_turrets[MAX_TURRETS];
Enemy g_enemies[MAX_ENEMIES];
Bullet g_bullets[MAX_BULLETS];
DeathParticle g_death_particles[MAX_DEATH_PARTICLES];

#include <stdio.h>
#include <fat.h>

GameBalanceConfig g_balance;

// A 16 px grid is wider than the separation radius. It turns the old
// all-pairs search into a query of the enemy's own cell and eight neighbours.
#define ENEMY_GRID_CELL_SIZE 16
#define ENEMY_GRID_W (SCREEN_W / ENEMY_GRID_CELL_SIZE)
#define ENEMY_GRID_H ((FIELD_H + ENEMY_GRID_CELL_SIZE - 1) / ENEMY_GRID_CELL_SIZE)
static int s_enemy_grid_heads[ENEMY_GRID_W * ENEMY_GRID_H];
static int s_enemy_grid_next[MAX_ENEMIES];

static void enemy_grid_build(void) {
    for (int cell = 0; cell < ENEMY_GRID_W * ENEMY_GRID_H; cell++) {
        s_enemy_grid_heads[cell] = -1;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) continue;
        int x = FROM_FP(g_enemies[i].x);
        int y = FROM_FP(g_enemies[i].y);
        int cell_x = x / ENEMY_GRID_CELL_SIZE;
        int cell_y = y / ENEMY_GRID_CELL_SIZE;
        if (cell_x < 0) cell_x = 0;
        if (cell_x >= ENEMY_GRID_W) cell_x = ENEMY_GRID_W - 1;
        if (cell_y < 0) cell_y = 0;
        if (cell_y >= ENEMY_GRID_H) cell_y = ENEMY_GRID_H - 1;
        int cell = cell_y * ENEMY_GRID_W + cell_x;
        s_enemy_grid_next[i] = s_enemy_grid_heads[cell];
        s_enemy_grid_heads[cell] = i;
    }
}

static const GameBalanceConfig s_default_balance = {
    .stages = {
        // Etapa 1 (Min 0:00 - 2:00): Zergling base=180f (~3s), peak=30f (0.5s), no scourges, no hydras, reward=300
        { .zergling_delay_base = 180, .scourge_delay_base = 0,   .hydralisk_delay_base = 0,
          .zergling_delay_peak = 30,  .scourge_delay_peak = 0,   .hydralisk_delay_peak = 0,
          .stage_reward_scrap = 300 },

        // Etapa 2 (Min 2:00 - 4:00): Zerglings base=90f + Scourge base=180f; peak: Zerg=20f, Scourge=45f, reward=600
        { .zergling_delay_base = 90,  .scourge_delay_base = 180, .hydralisk_delay_base = 0,
          .zergling_delay_peak = 20,  .scourge_delay_peak = 45,  .hydralisk_delay_peak = 0,
          .stage_reward_scrap = 600 },

        // Etapa 3 (Min 4:00 - 6:00): Zerglings base=45f + Scourge base=90f; peak: Zerg=12f, Scourge=25f, reward=1200
        { .zergling_delay_base = 45,  .scourge_delay_base = 90,  .hydralisk_delay_base = 0,
          .zergling_delay_peak = 12,  .scourge_delay_peak = 25,  .hydralisk_delay_peak = 0,
          .stage_reward_scrap = 1200 },

        // Etapa 4 (Min 6:00 - 8:00): Zerglings base=25f + Scourge base=50f + Hydra base=300f; peak: Zerg=8f, Scourge=15f, Hydra=60f, reward=2500
        { .zergling_delay_base = 25,  .scourge_delay_base = 50,  .hydralisk_delay_base = 300,
          .zergling_delay_peak = 8,   .scourge_delay_peak = 15,  .hydralisk_delay_peak = 60,
          .stage_reward_scrap = 2500 },

        // Etapa 5 (Min 8:00 - 10:00): Marea final! Zerg base=12f + Scourge base=25f + Hydra base=120f; peak: Zerg=3f, Scourge=8f, Hydra=25f, reward=5000
        { .zergling_delay_base = 12,  .scourge_delay_base = 25,  .hydralisk_delay_base = 120,
          .zergling_delay_peak = 3,   .scourge_delay_peak = 8,   .hydralisk_delay_peak = 25,
          .stage_reward_scrap = 5000 },
    },
    // Enemy stats (Constant across all stages):
    // 0: Scourge, 1: Zergling, 2: Hydralisk, 3: Mutalisk, 4: Defiler, 5: Lurker, 6: Guardian, 7: Ultralisk
    .enemy_hp = { 1, 3, 15, 40, 80, 120, 300, 800 },
    .enemy_speed = { 70, 36, 24, 45, 22, 26, 18, 16 },
    .enemy_scrap = { 2, 1, 8, 20, 45, 75, 180, 500 },
    .enemy_bite_damage = { 12, 6, 10, 10, 15, 20, 30, 50 },
    .enemy_bite_interval = { 1, 25, 30, 25, 35, 30, 45, 40 },

    .upgrade_costs = {
        { 60, 200, 450, 1000, 0 },  // 0: Caliber Lv1..4
        { 90, 250, 600, 1400, 0 },  // 1: Cadence Lv1..4
        { 40, 120, 300, 700, 0 },   // 2: Mag Size Lv1..4
        { 150, 600, 0, 0, 0 },      // 3: Bio Harvest Lv1..2
        { 120, 350, 800, 1800, 0 }, // 4: Supply Conveyor Lv1..4
        { 350, 0, 0, 0, 0 },        // 5: Auto Target
        { 600, 1800, 0, 0, 0 },     // 6: Extra Turrets (Socket 2, Socket 3)
    },
    .turret_damage = { 1, 2, 3, 5, 8 },
    .turret_fire_interval = { 12, 10, 8, 5, 3 },
    .turret_range = { 64, 48, 32, 16, 0 },
    .turret_magazine = { 10, 16, 25, 40, 60 },
    .bunker_start_hp = 100,
    .conveyor_reload_interval = { 9999, 60, 25, 12, 6 },
    .range_upgrade_costs = { 40, 100, 250, 600, 0 },
    .magic = 0x544F5732 // "TOW2"
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
            if (loaded.magic == 0x544F5732) {
                memcpy(&g_balance, &loaded, sizeof(g_balance));
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
    // Blood is persistent in the ground cache; duration is retained for API compatibility.
    (void)duration;
    tiles_stamp_splatter(x, y, size, color);
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

void wall_apply_balance_and_upgrades(void) {
    int act = 1 + g_game.upgrades.extra_turrets;
    if (act < 1) act = 1;
    if (act > 3) act = 3;
    g_wall.active_turrets = act;

    int dmg_lvl = g_game.upgrades.caliber_lvl;
    if (dmg_lvl < 0) dmg_lvl = 0;
    if (dmg_lvl > 4) dmg_lvl = 4;
    g_wall.damage = g_balance.turret_damage[dmg_lvl];

    int rof_lvl = g_game.upgrades.firerate_lvl;
    if (rof_lvl < 0) rof_lvl = 0;
    if (rof_lvl > 4) rof_lvl = 4;
    g_wall.fire_interval = g_balance.turret_fire_interval[rof_lvl];

    int rng_lvl = g_game.upgrades.range_lvl;
    if (rng_lvl < 0) rng_lvl = 0;
    if (rng_lvl > 4) rng_lvl = 4;
    g_wall.range_line_y = g_balance.turret_range[rng_lvl];
    g_wall.range = g_wall.screen_y - g_wall.range_line_y;

    int mag_lvl = g_game.upgrades.mag_size_lvl;
    if (mag_lvl < 0) mag_lvl = 0;
    if (mag_lvl > 4) mag_lvl = 4;
    int mag_cap = g_balance.turret_magazine[mag_lvl];
    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        g_wall.max_ammo[s] = mag_cap;
    }
}

void wall_init(void) {
    memset(&g_wall, 0, sizeof(g_wall));
    memset(g_casings, 0, sizeof(g_casings));
    memset(g_bullet_darts, 0, sizeof(g_bullet_darts));

    g_wall.screen_y = WALL_DEFAULT_Y; // 144
    g_wall.hp = 100;
    g_wall.max_hp = 100;
    g_wall.turret_angles[0] = 0; // NW
    g_wall.turret_angles[1] = 2; // N (forward facing)
    g_wall.turret_angles[2] = 2; // N
    g_wall.turret_angles[3] = 4; // NE
    g_wall.reload_time = 90;
    g_wall.fire_cooldown = 0;
    g_wall.battery_fire_step = 0;
    g_wall.locked_enemy_idx = -1;
    g_wall.conveyor_timer = 0;

    wall_apply_balance_and_upgrades();

    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        g_wall.target_angles[s] = g_wall.turret_angles[s];
        g_wall.target_enemy_idx[s] = -1;
        g_wall.turret_cooldown[s] = 0;
        g_wall.traverse_timer[s] = 0;
        g_wall.ammo[s] = g_wall.max_ammo[s];
        g_wall.reload_timer[s] = 0;
        g_wall.is_reloading[s] = 0;
        g_wall.barrel_alt[s] = 0;
    }
}

void wall_reload_socket(int s) {
    if (s < 0 || s >= WALL_SOCKET_COUNT) return;
    g_wall.ammo[s] = g_wall.max_ammo[s];
    g_wall.is_reloading[s] = 0;
    g_wall.reload_timer[s] = 0;
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
    int slot = -1;
    for (int i = 0; i < MAX_CASINGS; i++) {
        if (!g_casings[i].active) {
            slot = i;
            break;
        }
    }
    // Ring-buffer eviction: if pool is full, evict the oldest casing so EVERY bullet gets its casing!
    if (slot < 0) {
        int min_life = 99999;
        for (int i = 0; i < MAX_CASINGS; i++) {
            if (g_casings[i].life < min_life) {
                min_life = g_casings[i].life;
                slot = i;
            }
        }
    }
    if (slot < 0) return;

    g_casings[slot].active = 1;
    g_casings[slot].x = TO_FP(x);
    g_casings[slot].y = TO_FP(y);
    g_casings[slot].z = TO_FP(4);
    int base_vx = TO_FP(1) + (rand() % (FP_ONE * 3 / 4));
    g_casings[slot].vx = dir_sign * base_vx;
    g_casings[slot].vy = TO_FP(1) + (rand() % (FP_ONE / 2)); // Eject toward bunker floor
    g_casings[slot].vz = TO_FP(2) + (rand() % TO_FP(2));     // Eject upward
    g_casings[slot].angle = rand() % 360;
    g_casings[slot].spin_speed = dir_sign * (30 + (rand() % 20));
    g_casings[slot].bounces = 0;
    g_casings[slot].life = 60;
}

int wall_spawn_bullet_dart(int start_x, int start_y, int target_x, int target_y, int target_enemy_idx) {
    int slot = -1;
    for (int i = 0; i < MAX_BULLET_DARTS; i++) {
        if (!g_bullet_darts[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        int min_dist = 999999;
        for (int i = 0; i < MAX_BULLET_DARTS; i++) {
            if (g_bullet_darts[i].dist_remaining < min_dist) {
                min_dist = g_bullet_darts[i].dist_remaining;
                slot = i;
            }
        }
    }
    if (slot < 0) return 0;

    g_bullet_darts[slot].active = 1;
    g_bullet_darts[slot].x = TO_FP(start_x);
    g_bullet_darts[slot].y = TO_FP(start_y);
    g_bullet_darts[slot].target_x = target_x;
    g_bullet_darts[slot].target_y = target_y;
    g_bullet_darts[slot].damage = g_wall.damage;
    g_bullet_darts[slot].target_enemy_idx = target_enemy_idx;

    int dx = target_x - start_x;
    int dy = target_y - start_y;
    int ax = (dx < 0) ? -dx : dx;
    int ay = (dy < 0) ? -dy : dy;
    int dist = (ax > ay) ? (ax + (ay >> 1)) : (ay + (ax >> 1));
    if (dist < 1) dist = 1;
    g_bullet_darts[slot].dist_remaining = TO_FP(dist);

    int speed = TO_FP(16); // 16 px/frame
    g_bullet_darts[slot].vx = (dx * speed) / dist;
    g_bullet_darts[slot].vy = (dy * speed) / dist;
    return 1;
}

void wall_fire_at_target(int target_x, int target_y, int enemy_idx) {
    if (g_wall.fire_cooldown > 0) return;

    int num_act = g_wall.active_turrets;
    if (num_act < 1) num_act = 1;
    if (num_act > 4) num_act = 4;

    static const int s_active_sockets[4][4] = {
        { 1, -1, -1, -1 }, // 1 turret: Socket 1
        { 1,  2, -1, -1 }, // 2 turrets: Sockets 1 & 2
        { 0,  1,  2, -1 }, // 3 turrets: Sockets 0, 1 & 2
        { 0,  1,  2,  3 }  // 4 turrets: All sockets
    };

    // Round-robin metronome alternating active turrets and barrels:
    // e.g. For 2 turrets: S1(L) -> S2(L) -> S1(R) -> S2(R) -> S1(L)...
    int k = g_wall.battery_fire_step % (num_act * 2);
    int s = s_active_sockets[num_act - 1][k % num_act];
    int barrel = (k / num_act) % 2;

    if (s < 0 || s >= WALL_SOCKET_COUNT) return;

    // If turret is out of ammo, lock it in RELOAD state and cease firing
    if (g_wall.ammo[s] <= 0) {
        g_wall.is_reloading[s] = 1;
        return;
    }

    int sx = c_wall_sockets[s].x;
    int sy = g_wall.screen_y + c_wall_sockets[s].y;

    int angle = wall_angle_from_target(sx, sy, target_x, target_y);
    g_wall.turret_angles[s] = angle;
    g_wall.target_angles[s] = angle;

    const TurretCalibratedPoints *pts = &c_turret_points[angle];
    int tx = sx - TURRET_PIVOT_X;
    int ty = sy - TURRET_PIVOT_Y;

    int mx = tx + (barrel == 0 ? pts->ml_x : pts->mr_x);
    int my = ty + (barrel == 0 ? pts->ml_y : pts->mr_y);
    int dx = tx + (barrel == 0 ? pts->dl_x : pts->dr_x);
    int dy = ty + (barrel == 0 ? pts->dl_y : pts->dr_y);

    int spawned = wall_spawn_bullet_dart(mx, my, target_x, target_y, enemy_idx);
    if (spawned) {
        g_wall.ammo[s]--;
        g_wall.battery_fire_step++;
        wall_spawn_casing(dx, dy, (barrel == 0 ? -1 : 1));
        g_wall.muzzle_flash_timer[s] = 2;
        g_wall.muzzle_flash_barrel[s] = barrel;

        if (enemy_idx >= 0 && enemy_idx < MAX_ENEMIES) {
            g_enemies[enemy_idx].incoming_damage += g_wall.damage;
        }

        if (g_wall.ammo[s] <= 0) {
            g_wall.is_reloading[s] = 1;
        }
    }
    g_wall.fire_cooldown = g_wall.fire_interval;
}

void wall_fire_socket(int s, int target_x, int target_y) {
    wall_fire_at_target(target_x, target_y, -1);
}

void wall_fire_at(int target_x, int target_y) {
    wall_fire_at_target(target_x, target_y, -1);
}

void wall_update(void) {
    if (g_wall.fire_cooldown > 0) g_wall.fire_cooldown--;
    if (g_wall.damage_flash_timer > 0) g_wall.damage_flash_timer--;

    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        // Active reload progress
        if (g_wall.is_reloading[s]) {
            g_wall.reload_timer[s]--;
            if (g_wall.reload_timer[s] <= 0) {
                g_wall.ammo[s] = g_wall.max_ammo[s];
                g_wall.is_reloading[s] = 0;
            }
        }
        if (g_wall.turret_cooldown[s] > 0) g_wall.turret_cooldown[s]--;
        if (g_wall.muzzle_flash_timer[s] > 0) {
            g_wall.muzzle_flash_timer[s]--;
        }
    }

    // Auto-supply conveyor (passive ammo feeding per frame interval)
    int c_lvl = g_game.upgrades.conveyor_lvl;
    if (c_lvl > 0 && c_lvl <= 4) {
        int interval = g_balance.conveyor_reload_interval[c_lvl];
        if (interval > 0 && interval < 9000) {
            g_wall.conveyor_timer++;
            if (g_wall.conveyor_timer >= interval) {
                g_wall.conveyor_timer = 0;
                int num_act = g_wall.active_turrets;
                static const int s_active_sockets[4][4] = {
                    { 1, -1, -1, -1 }, { 1, 2, -1, -1 }, { 0, 1, 2, -1 }, { 0, 1, 2, 3 }
                };
                for (int i = 0; i < num_act; i++) {
                    int s = s_active_sockets[num_act - 1][i];
                    if (s >= 0 && g_wall.ammo[s] < g_wall.max_ammo[s]) {
                        g_wall.ammo[s]++;
                        if (g_wall.is_reloading[s] && g_wall.ammo[s] > 0) {
                            g_wall.is_reloading[s] = 0;
                            g_wall.reload_timer[s] = 0;
                        }
                        break;
                    }
                }
            }
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

    int global_auto_target_e = -1;
    if (g_wall.locked_enemy_idx >= 0) {
        global_auto_target_e = g_wall.locked_enemy_idx;
    } else if (auto_fire) {
        int best_score = -99999;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gx = FROM_FP(g_enemies[e].x);
            int gy = FROM_FP(g_enemies[e].y);
            if (gy > 192 + g_wall.screen_y + 10) continue; // Behind wall

            int h_dist = (gx > 128) ? (gx - 128) : (128 - gx);
            int v_score = (gy >= 192) ? (gy * 3) : gy;
            int score = v_score - h_dist;
            if (score > best_score) {
                best_score = score;
                global_auto_target_e = e;
            }
        }
    }

    // Aiming, tracking, and firing for each active socket
    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        if (!(active_mask & (1 << s))) continue;
        int sx = c_wall_sockets[s].x;
        int sy = g_wall.screen_y + c_wall_sockets[s].y;

        int target_e = global_auto_target_e;
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

        // Auto-firing: respects straight horizontal range line (local Y >= range_line_y) and virtual health
        if (auto_fire && g_wall.fire_cooldown == 0 && target_e >= 0) {
            int gy = FROM_FP(g_enemies[target_e].y);
            if (gy >= 192 && gy < 192 + g_wall.screen_y) {
                int gx = FROM_FP(g_enemies[target_e].x);
                int local_gy = gy - 192;
                if (local_gy >= g_wall.range_line_y && g_enemies[target_e].hp > g_enemies[target_e].incoming_damage) {
                    wall_fire_at_target(gx, local_gy, target_e);
                }
            }
        }
    }

    // Update Bullet Darts with sub-stepping and coordinated virtual health impact
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

            int target_e = g_bullet_darts[i].target_enemy_idx;
            if (target_e >= 0 && target_e < MAX_ENEMIES && g_enemies[target_e].active) {
                int ex = FROM_FP(g_enemies[target_e].x);
                int ey = FROM_FP(g_enemies[target_e].y);
                int ddx = cur_x - ex;
                int ddy = gy - ey;
                if (ddx * ddx + ddy * ddy <= 14 * 14 || g_bullet_darts[i].dist_remaining <= 0) {
                    hit_enemy = 1;
                    hit_e_idx = target_e;
                    hit_x = ex;
                    hit_y = ey;
                    break;
                }
            } else {
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (!g_enemies[e].active) continue;
                    int ex = FROM_FP(g_enemies[e].x);
                    int ey = FROM_FP(g_enemies[e].y);
                    int ddx = cur_x - ex;
                    int ddy = gy - ey;
                    if (ddx * ddx + ddy * ddy <= 12 * 12) {
                        hit_enemy = 1;
                        hit_e_idx = e;
                        hit_x = ex;
                        hit_y = ey;
                        break;
                    }
                }
                if (hit_enemy) break;
            }
        }

        if (hit_enemy == 1 && hit_e_idx >= 0) {
            // Kinetic impact sparks
            for (int k = 0; k < 3; k++) {
                game_add_splatter_ex(hit_x, hit_y, COLOR_BOLTER_TRACER, 0, 8);
            }
            if (g_enemies[hit_e_idx].incoming_damage >= (uint64_t)g_bullet_darts[i].damage) {
                g_enemies[hit_e_idx].incoming_damage -= g_bullet_darts[i].damage;
            } else {
                g_enemies[hit_e_idx].incoming_damage = 0;
            }

            if (g_enemies[hit_e_idx].hp > (uint64_t)g_bullet_darts[i].damage) {
                g_enemies[hit_e_idx].hp -= g_bullet_darts[i].damage;
            } else {
                g_enemies[hit_e_idx].hp = 0;
                g_enemies[hit_e_idx].active = 0;
                g_game.enemies_killed++;
                int v = g_enemies[hit_e_idx].variant;
                if (v < 0) v = 0;
                if (v >= ENEMY_VARIANT_COUNT) v = ENEMY_VARIANT_COUNT - 1;
                uint64_t base_scrap = g_balance.enemy_scrap[v];
                uint64_t reward = base_scrap * (1 + g_game.upgrades.bio_harvest_lvl);
                g_game.scrap += reward;
                game_spawn_death_gore(hit_x, hit_y, g_bullet_darts[i].vx, g_bullet_darts[i].vy, g_enemies[hit_e_idx].variant);
            }
            g_bullet_darts[i].active = 0;
        } else if (hit_enemy == 2 || g_bullet_darts[i].dist_remaining <= 0) {
            int target_e = g_bullet_darts[i].target_enemy_idx;
            if (target_e >= 0 && target_e < MAX_ENEMIES && g_enemies[target_e].active) {
                if (g_enemies[target_e].incoming_damage >= (uint64_t)g_bullet_darts[i].damage) {
                    g_enemies[target_e].incoming_damage -= g_bullet_darts[i].damage;
                } else {
                    g_enemies[target_e].incoming_damage = 0;
                }
            }
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
    memset(g_death_particles, 0, sizeof(g_death_particles));

    wall_init();
    g_game.mode = MODE_PAUSED;
    g_game.wave_number = 1;
    g_game.total_waves = STAGE_COUNT;
    g_game.bunker_hp = g_balance.bunker_start_hp;
    g_game.bunker_max_hp = g_balance.bunker_start_hp;
    g_game.scrap = 10;
    g_game.fast_forward = 1;

    // Upgrades initial state
    g_game.upgrades.caliber_lvl = 0;
    g_game.upgrades.firerate_lvl = 0;
    g_game.upgrades.range_lvl = 0;
    g_game.upgrades.mag_size_lvl = 0;
    g_game.upgrades.auto_target = 0; // Starts requiring manual touch-targeting until upgrade purchased!
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

    // Rebuild the immutable ground layer so persistent blood is cleared only
    // when a new run starts, never as part of ordinary frame updates.
    tiles_init();
}

void game_start_wave(void) {
    g_game.mode = MODE_WAVE;
    g_game.wave_timer = STAGE_DURATION_FRAMES;
    g_game.enemies_spawned = 0;
    g_game.enemies_alive = 0;
    g_game.spawn_timer = 0;
    g_game.spawn_timer_zergling = 0;
    g_game.spawn_timer_scourge = 0;
    g_game.spawn_timer_hydra = 0;

    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_death_particles, 0, sizeof(g_death_particles));

    // Reset locked targets
    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].locked_enemy_idx = -1;
    }
}

void game_reset_to_prep(void) {
    wall_init();
    g_game.bunker_hp = g_wall.hp;
    g_game.bunker_max_hp = g_wall.max_hp;
    g_game.mode = MODE_PAUSED;
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_enemies, 0, sizeof(g_enemies));
    memset(g_death_particles, 0, sizeof(g_death_particles));

    // Repair turrets back to full between waves
    for (int t = 0; t < MAX_TURRETS; t++) {
        g_turrets[t].hp = g_turrets[t].max_hp;
        g_turrets[t].ammo = g_turrets[t].max_ammo;
        g_turrets[t].fire_cooldown = 0;
        g_turrets[t].flash_timer = 0;
        g_turrets[t].barrel_recoil_l = 0;
        g_turrets[t].barrel_recoil_r = 0;
        g_turrets[t].locked_enemy_idx = -1;
    }
}

void game_update_simulation(void) {
    if (g_game.mode != MODE_WAVE && g_game.mode != MODE_DEBUG_SANDBOX) return;

    g_game.sim_ticks_elapsed++;
    wall_update();
    if (g_game.wave_timer > 0) g_game.wave_timer--;

    int stage_idx = g_game.wave_number - 1;
    if (stage_idx < 0) stage_idx = 0;
    if (stage_idx >= STAGE_COUNT) stage_idx = STAGE_COUNT - 1;
    const StageConfig *st = &g_balance.stages[stage_idx];

    // Check if in Base phase (> 1800f, first 90s) or Peak phase (<= 1800f, last 30s)
    int is_base = (g_game.wave_timer > 1800);
    int z_delay = is_base ? st->zergling_delay_base : st->zergling_delay_peak;
    int s_delay = is_base ? st->scourge_delay_base : st->scourge_delay_peak;
    int h_delay = is_base ? st->hydralisk_delay_base : st->hydralisk_delay_peak;

    // 1. Spawning per species
    // Zergling (Variant 1)
    if (z_delay > 0) {
        g_game.spawn_timer_zergling++;
        if (g_game.spawn_timer_zergling >= z_delay) {
            g_game.spawn_timer_zergling = 0;
            spawn_enemy(1, g_balance.enemy_hp[1], g_balance.enemy_speed[1]);
        }
    }
    // Scourge (Variant 0)
    if (s_delay > 0) {
        g_game.spawn_timer_scourge++;
        if (g_game.spawn_timer_scourge >= s_delay) {
            g_game.spawn_timer_scourge = 0;
            spawn_enemy(0, g_balance.enemy_hp[0], g_balance.enemy_speed[0]);
        }
    }
    // Hydralisk (Variant 2)
    if (h_delay > 0) {
        g_game.spawn_timer_hydra++;
        if (g_game.spawn_timer_hydra >= h_delay) {
            g_game.spawn_timer_hydra = 0;
            spawn_enemy(2, g_balance.enemy_hp[2], g_balance.enemy_speed[2]);
        }
    }

    // 2. Death particles
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

    // Snapshot positions for the local separation broad-phase. New spawns do
    // not need separation until their first movement tick.
    enemy_grid_build();

    // 3. Update Enemies (Descending vertically and converging towards central bunker at x=128, y=360)
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

        // 4b. Soft separation repulsion between nearby marching enemies
        int sep_force_x = 0;
        if (py < 336 && ((i & 1) == (g_game.sim_ticks_elapsed & 1))) {
            int cell_x = px / ENEMY_GRID_CELL_SIZE;
            int cell_y = py / ENEMY_GRID_CELL_SIZE;
            if (cell_x < 0) cell_x = 0;
            if (cell_x >= ENEMY_GRID_W) cell_x = ENEMY_GRID_W - 1;
            if (cell_y < 0) cell_y = 0;
            if (cell_y >= ENEMY_GRID_H) cell_y = ENEMY_GRID_H - 1;
            for (int near_y = cell_y - 1; near_y <= cell_y + 1; near_y++) {
                if (near_y < 0 || near_y >= ENEMY_GRID_H) continue;
                for (int near_x = cell_x - 1; near_x <= cell_x + 1; near_x++) {
                    if (near_x < 0 || near_x >= ENEMY_GRID_W) continue;
                    int cell = near_y * ENEMY_GRID_W + near_x;
                    for (int j = s_enemy_grid_heads[cell]; j >= 0; j = s_enemy_grid_next[j]) {
                        if (i == j || !g_enemies[j].active) continue;
                        int ody = ey - g_enemies[j].y;
                        int aody = (ody < 0) ? -ody : ody;
                        if (aody >= TO_FP(16)) continue;
                        int odx = ex - g_enemies[j].x;
                        int aodx = (odx < 0) ? -odx : odx;
                        if (aodx >= TO_FP(16)) continue;

                        int dist_sq = (aodx >> FP_SHIFT) * (aodx >> FP_SHIFT) + (aody >> FP_SHIFT) * (aody >> FP_SHIFT);
                        if (dist_sq < (16 * 16) && dist_sq > 0) {
                            int push = TO_FP(1) / 2;
                            if (odx > 0) sep_force_x += push;
                            else if (odx < 0) sep_force_x -= push;
                        }
                    }
                }
            }
        }
        ex += sep_force_x;
        // Keep within battlefield bounds [8..248]
        if (ex < TO_FP(8)) ex = TO_FP(8);
        if (ex > TO_FP(248)) ex = TO_FP(248);
        g_enemies[i].x = ex;

        // Check if reaching Wall fortification rim (Y_global >= 336, i.e. Y_local >= 144):
        // 3D Depth: Enemies press against the wall parapet at Y=144; their head/mouth
        // tucks behind the sandbag bulwark, leaving only their rear/hind legs visible on the road.
        if (py >= 336) {
            int b_variant = g_enemies[i].variant;
            if (b_variant < 0) b_variant = 0;
            if (b_variant >= ENEMY_VARIANT_COUNT) b_variant = ENEMY_VARIANT_COUNT - 1;

            if (b_variant == 0) {
                // Scourge (Variant 0): Aerial kamikaze suicide detonation on wall impact!
                int bpx = FROM_FP(g_enemies[i].x);
                int bpy = 138;
                uint64_t kamikaze_dmg = g_balance.enemy_bite_damage[0];
                if (g_game.mode != MODE_DEBUG_SANDBOX) {
                    if (g_wall.hp > kamikaze_dmg) {
                        g_wall.hp -= kamikaze_dmg;
                    } else {
                        g_wall.hp = 0;
                        g_game.bunker_hp = 0;
                        g_game.mode = MODE_GAME_OVER;
                        return;
                    }
                    g_game.bunker_hp = g_wall.hp;
                    g_wall.damage_flash_timer = 8; // Trauma flash on wall
                }
                // Massive splatter & debris explosion
                game_spawn_death_gore(bpx, 336, 0, 0, 0);
                for (int s = 0; s < 4; s++) {
                    game_add_splatter_ex(bpx + ((rand() % 13) - 6), bpy + ((rand() % 7) - 3), COLOR_LED_RED, 1, 12);
                }
                g_enemies[i].active = 0;
                g_game.enemies_alive--;
                continue;
            }

            g_enemies[i].y = TO_FP(336);
            g_enemies[i].vy = 0;
            g_enemies[i].vx = 0;
            g_enemies[i].dir = 4; // Face South against the fortified wall
            g_enemies[i].biting_target = 99; // Attacking the Wall
            g_enemies[i].bite_timer++;

            // Cycle attack animation
            const EnemyTypeDef *type = &g_enemy_types[g_enemies[i].variant];
            if (type->attack_frame_count > 0) {
                g_enemies[i].anim_frame = (g_enemies[i].bite_timer / 6) % type->attack_frame_count;
            }

            // Attack cycle for wall biting
            int b_interval = g_balance.enemy_bite_interval[b_variant];
            if (b_interval < 1) b_interval = 25;
            if (g_enemies[i].bite_timer >= b_interval) {
                g_enemies[i].bite_timer = 0;
                uint64_t bite_dmg = g_balance.enemy_bite_damage[b_variant];
                if (bite_dmg < 1) bite_dmg = 1;
                if (g_game.mode != MODE_DEBUG_SANDBOX) {
                    if (g_wall.hp > bite_dmg) {
                        g_wall.hp -= bite_dmg;
                    } else {
                        g_wall.hp = 0;
                        g_game.bunker_hp = 0;
                        g_game.mode = MODE_GAME_OVER;
                        return;
                    }
                    g_game.bunker_hp = g_wall.hp;
                    g_wall.damage_flash_timer = 6; // Trigger visual cathode trauma feedback

                    // Wall impact sparks & concrete dust at the enemy's exact contact point along the wall
                    int bpx = FROM_FP(g_enemies[i].x);
                    int bpy = 138;
                    game_add_splatter_ex(bpx, bpy, COLOR_BOLTER_TRACER, 1, 8);
                    game_add_splatter_ex(bpx + ((rand() % 9) - 4), bpy + ((rand() % 5) - 2), COLOR_LED_RED, 0, 6);
                }
            }
            continue;
        }

        // Advance: straight vertical downward in parallel lanes across both screens
        g_enemies[i].y += spd;
        g_enemies[i].vy = spd;
        g_enemies[i].vx = sep_force_x;

        int move_dir = enemy_direction_from_delta(g_enemies[i].x - old_x,
                                                   g_enemies[i].y - old_y);
        int move_dx = g_enemies[i].x - old_x;
        int move_dy = g_enemies[i].y - old_y;
        int move_ax = (move_dx < 0) ? -move_dx : move_dx;
        int move_ay = (move_dy < 0) ? -move_dy : move_dy;
        int move_distance = move_ax + move_ay;
        if (move_dir >= 0) g_enemies[i].dir = move_dir;

        // Animation: Scourge flaps continuously in flight; terrestrial walk is distance-synchronized.
        if (g_enemies[i].variant == 0) {
            int fly_frames = g_enemy_types[0].frame_count;
            if (fly_frames < 1) fly_frames = 5;
            g_enemies[i].anim_frame = (g_game.sim_ticks_elapsed / 4) % fly_frames;
        } else if (move_distance > 0) {
            g_enemies[i].anim_distance += move_distance;
            int walk_frames = g_enemy_types[g_enemies[i].variant].frame_count;
            if (walk_frames < 1) walk_frames = 4;
            // Zergling stride: 2.5 px/frame perfectly coordinates 7-frame cycle to 17.5 px stride
            int step_thresh = (g_enemies[i].variant == 1) ? ((5 * FP_ONE) / 2) : (4 * FP_ONE);
            while (g_enemies[i].anim_distance >= step_thresh) {
                g_enemies[i].anim_distance -= step_thresh;
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

    // 7. Stage Completion (survived 2 minutes)
    if (g_game.wave_timer <= 0) {
        int st_idx = g_game.wave_number - 1;
        if (st_idx < 0) st_idx = 0;
        if (st_idx >= STAGE_COUNT) st_idx = STAGE_COUNT - 1;
        uint64_t stage_bonus = g_balance.stages[st_idx].stage_reward_scrap;
        g_game.scrap += stage_bonus;

        if (g_game.wave_number >= g_game.total_waves) {
            g_game.mode = MODE_VICTORY;
            return;
        }
        g_game.wave_number++;
        game_reset_to_prep();
    }
}

void game_toggle_pause(void) {
    if (g_game.mode == MODE_PAUSED) {
        if (g_game.enemies_spawned > 0 && g_game.wave_timer > 0) {
            g_game.mode = MODE_WAVE;
        } else {
            game_start_wave();
        }
        tiles_full_screen_refresh();
    } else if (g_game.mode == MODE_WAVE) {
        g_game.previous_mode = g_game.mode;
        g_game.mode = MODE_PAUSED;
        tiles_full_screen_refresh();
    }
}

void game_handle_input_prep(touchPosition touch, int keys_down, int keys_held) {
    game_handle_input_pause(touch, keys_down, keys_held);
}

void game_handle_input_wave(touchPosition touch, int keys_down, int keys_held) {
    if (keys_down & KEY_START) {
        game_toggle_pause();
        return;
    }

    if (keys_down & KEY_R) {
        g_game.fast_forward = (g_game.fast_forward == 1) ? 2 : 1;
    }

    static int s_wave_touching = 0;
    int is_touch = (keys_held & KEY_TOUCH) || (touch.px > 0 && touch.py > 0);
    int touch_press = ((keys_down & KEY_TOUCH) || (is_touch && !s_wave_touching));
    s_wave_touching = is_touch;

    if (touch_press) {
        // [PAUSA] Button in wave HUD: (215..250, 0..14)
        if (touch.px >= 215 && touch.px <= 250 && touch.py <= 14) {
            game_toggle_pause();
            return;
        }

        // Bunker Ammo Depot touch down: (AMMO_DEPOT_X=128, Y=166, W=24, H=16) -> (112..144, 154..178)
        if (touch.px >= 112 && touch.px <= 144 && touch.py >= 154 && touch.py <= 180) {
            g_game.is_dragging_ammo = 1;
            g_game.drag_x = touch.px;
            g_game.drag_y = touch.py;
            return;
        }
    }

    // Dragging ammo crate following stylus
    if (is_touch && g_game.is_dragging_ammo) {
        g_game.drag_x = touch.px;
        g_game.drag_y = touch.py;
        return;
    }

    // Touch release while dragging ammo: drop onto active turret socket to reload
    if (!is_touch && g_game.is_dragging_ammo) {
        int num_act = g_wall.active_turrets;
        static const int s_active_sockets[4][4] = {
            { 1, -1, -1, -1 }, { 1, 2, -1, -1 }, { 0, 1, 2, -1 }, { 0, 1, 2, 3 }
        };
        for (int i = 0; i < num_act; i++) {
            int s = s_active_sockets[num_act - 1][i];
            if (s < 0) continue;
            int sx = c_wall_sockets[s].x;
            int sy = g_wall.screen_y + c_wall_sockets[s].y;
            int dx = g_game.drag_x - sx;
            int dy = g_game.drag_y - sy;
            if (dx * dx + dy * dy <= 24 * 24) {
                wall_reload_socket(s);
                break;
            }
        }
        g_game.is_dragging_ammo = 0;
        return;
    }

    // Combat interaction (Upper & Mid road: Y >= 14 && Y < 144)
    // STRICT RULE: Only fires when touching a living enemy inside range!
    // Empty asphalt clicks DO NOT FIRE!
    if (is_touch && touch.px > 0 && touch.py >= 14 && touch.py < g_wall.screen_y) {
        int can_trigger = touch_press || g_game.upgrades.continuous_fire;

        // Find enemy touched within tolerance (radius ~22px)
        int hit_enemy = -1;
        int best_dsq = 22 * 22;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!g_enemies[e].active) continue;
            int gy = FROM_FP(g_enemies[e].y);
            if (gy < 192) continue; // Must be on bottom screen
            int local_y = gy - 192;
            if (local_y < g_wall.range_line_y) continue; // Outside range perimeter (Y < 64)

            // Anti-overkill virtual health: must have positive effective health remaining!
            if (g_enemies[e].hp <= g_enemies[e].incoming_damage) continue;

            int gx = FROM_FP(g_enemies[e].x);
            int ddx = touch.px - gx;
            int ddy = touch.py - local_y;
            int dsq = ddx * ddx + ddy * ddy;
            if (dsq <= best_dsq) {
                best_dsq = dsq;
                hit_enemy = e;
            }
        }

        if (hit_enemy >= 0) {
            g_wall.locked_enemy_idx = hit_enemy;
            if (can_trigger && g_wall.fire_cooldown == 0) {
                int ex = FROM_FP(g_enemies[hit_enemy].x);
                int ey = FROM_FP(g_enemies[hit_enemy].y) - 192;
                wall_fire_at_target(ex, ey, hit_enemy);
            }
        }
        // If hit_enemy < 0 (empty asphalt): DO NOTHING. Battery stays silent!
    }
}

void game_handle_input_pause(touchPosition touch, int keys_down, int keys_held) {
    (void)keys_held;
    // Physical button shortcut: START launches/resumes wave
    if (keys_down & KEY_START) {
        game_toggle_pause();
        return;
    }

    if (keys_down & KEY_TOUCH) {
        // [TIENDA] Button: (10, 146, 74, 36) -> hitbox (10..84, 144..188)
        if (touch.px >= 10 && touch.px <= 84 && touch.py >= 144 && touch.py <= 188) {
            g_game.previous_mode = MODE_PAUSED;
            g_game.mode = MODE_UPGRADES;
            return;
        }

        // [CALIBRAR] Button: (90, 146, 74, 36) -> hitbox (90..164, 144..188)
        if (touch.px >= 90 && touch.px <= 164 && touch.py >= 144 && touch.py <= 188) {
            g_game.previous_mode = MODE_PAUSED;
            g_game.mode = MODE_CALIBRATION;
            return;
        }

        // [JUGAR / REANUDAR] Button: (170, 146, 76, 36) -> hitbox (170..248, 144..188)
        if (touch.px >= 170 && touch.px <= 248 && touch.py >= 144 && touch.py <= 188) {
            game_toggle_pause();
            return;
        }
    }
}

void game_handle_input_game_over(touchPosition touch, int keys_down, int keys_held) {
    (void)touch;
    (void)keys_held;
    if (keys_down & (KEY_B | KEY_START | KEY_A | KEY_TOUCH)) {
        game_init();
        return;
    }
}

void game_handle_input_victory(touchPosition touch, int keys_down, int keys_held) {
    (void)touch;
    (void)keys_held;
    if (keys_down & (KEY_B | KEY_START | KEY_A | KEY_TOUCH)) {
        game_init();
        return;
    }
}

uint64_t upgrade_get_cost(int idx) {
    if (idx == 7) {
        int level = g_game.upgrades.range_lvl;
        return (level >= 0 && level < 4) ? g_balance.range_upgrade_costs[level] : 999999;
    }
    if (idx >= 0 && idx < 7) {
        int levels[7] = {
            g_game.upgrades.caliber_lvl,
            g_game.upgrades.firerate_lvl,
            g_game.upgrades.mag_size_lvl,
            g_game.upgrades.bio_harvest_lvl,
            g_game.upgrades.conveyor_lvl,
            g_game.upgrades.auto_target,
            g_game.upgrades.extra_turrets
        };
        int level = levels[idx];
        int max_l = (idx == 3 || idx == 6) ? 2 : (idx == 5 ? 1 : 4);
        if (level >= 0 && level < max_l && g_balance.upgrade_costs[idx][level] > 0)
            return g_balance.upgrade_costs[idx][level];
        return 999999;
    }
    return 999999;
}

int upgrade_can_afford(int idx) {
    if (idx == 6) {
        int lvl = g_game.upgrades.extra_turrets;
        if (lvl == 0 && g_game.upgrades.firerate_lvl < 2) return 0; // Gated by Cadencia Lv2
        if (lvl == 1 && g_game.upgrades.firerate_lvl < 4) return 0; // Gated by Cadencia Lv4
        if (lvl >= 2) return 0;
    }
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
        case 2: g_game.upgrades.mag_size_lvl++; break;
        case 3: g_game.upgrades.bio_harvest_lvl++; break;
        case 4: g_game.upgrades.conveyor_lvl++; break;
        case 5: g_game.upgrades.auto_target = 1; break;
        case 6:
            g_game.upgrades.extra_turrets++;
            if (g_game.upgrades.extra_turrets > 2) g_game.upgrades.extra_turrets = 2;
            break;
        case 7: g_game.upgrades.range_lvl++; break;
    }

    wall_apply_balance_and_upgrades();
    for (int s = 0; s < WALL_SOCKET_COUNT; s++) {
        if (g_wall.ammo[s] > g_wall.max_ammo[s]) g_wall.ammo[s] = g_wall.max_ammo[s];
        if (idx == 6 && s == g_game.upgrades.extra_turrets) {
            g_wall.ammo[s] = g_wall.max_ammo[s];
            g_wall.is_reloading[s] = 0;
        }
    }
}

void game_handle_input_upgrades(touchPosition touch, int keys_down, int keys_held) {
    (void)keys_held;
    if (keys_down & (KEY_B | KEY_START)) {
        g_game.mode = MODE_PAUSED;
        tiles_full_screen_refresh();
        return;
    }

    if (keys_down & KEY_TOUCH) {
        // Return button: (130, 174, 76, 16)
        if (touch.px >= 120 && touch.px <= 215 && touch.py >= 165 && touch.py <= 191) {
            g_game.mode = MODE_PAUSED;
            tiles_full_screen_refresh();
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

static void calib_commit_changes(void) {
    wall_apply_balance_and_upgrades();
    for (int e = 0; e < MAX_ENEMIES; e++) {
        if (g_enemies[e].active) {
            int v = g_enemies[e].variant;
            if (v >= 0 && v < ENEMY_VARIANT_COUNT) {
            g_enemies[e].speed = g_balance.enemy_speed[v];
            }
        }
    }
    g_game.calib_saved_timer = 20;
    balance_config_save();
}

static void calib_modify_val(int delta) {
    if (g_game.calib_page == 0) {
        // ETAPAS (1..5): exactly 7 parameters per stage
        int s = g_game.calib_stage_idx;
        if (s < 0) s = 0;
        if (s >= STAGE_COUNT) s = STAGE_COUNT - 1;
        StageConfig *st = &g_balance.stages[s];

        switch (g_game.calib_row) {
            case 0: st->zergling_delay_base += delta; if (st->zergling_delay_base < 0) st->zergling_delay_base = 0; break;
            case 1: st->scourge_delay_base += delta; if (st->scourge_delay_base < 0) st->scourge_delay_base = 0; break;
            case 2: st->hydralisk_delay_base += delta; if (st->hydralisk_delay_base < 0) st->hydralisk_delay_base = 0; break;
            case 3: st->zergling_delay_peak += delta; if (st->zergling_delay_peak < 0) st->zergling_delay_peak = 0; break;
            case 4: st->scourge_delay_peak += delta; if (st->scourge_delay_peak < 0) st->scourge_delay_peak = 0; break;
            case 5: st->hydralisk_delay_peak += delta; if (st->hydralisk_delay_peak < 0) st->hydralisk_delay_peak = 0; break;
            case 6: st->stage_reward_scrap += delta; if (st->stage_reward_scrap < 0) st->stage_reward_scrap = 0; break;
        }
        calib_commit_changes();
        return;
    }
    if (g_game.calib_page == 1) {
        // ENEMY STATS: Constant across all stages (5 fields per enemy: HP, Speed, Scrap, Bite Dmg, Bite Int)
        int enemy = g_game.calib_row / 5;
        int field = g_game.calib_row % 5;
        if (enemy < 0) enemy = 0;
        if (enemy >= ENEMY_VARIANT_COUNT) enemy = ENEMY_VARIANT_COUNT - 1;
        switch (field) {
            case 0: { int n = (int)g_balance.enemy_hp[enemy] + delta; if (n < 1) n = 1; g_balance.enemy_hp[enemy] = (uint32_t)n; break; }
            case 1: { int n = g_balance.enemy_speed[enemy] + delta; if (n < 5) n = 5; g_balance.enemy_speed[enemy] = n; break; }
            case 2: { int n = (int)g_balance.enemy_scrap[enemy] + delta; if (n < 0) n = 0; g_balance.enemy_scrap[enemy] = (uint32_t)n; break; }
            case 3: { int n = g_balance.enemy_bite_damage[enemy] + delta; if (n < 1) n = 1; g_balance.enemy_bite_damage[enemy] = n; break; }
            case 4: { int n = g_balance.enemy_bite_interval[enemy] + delta; if (n < 1) n = 1; g_balance.enemy_bite_interval[enemy] = n; break; }
        }
        calib_commit_changes();
        return;
    }
    if (g_game.calib_page == 2) {
        int r = g_game.calib_row;
        if (r == 0) {
            g_balance.bunker_start_hp += delta * 10;
            if (g_balance.bunker_start_hp < 10) g_balance.bunker_start_hp = 10;
        } else if (r >= 1 && r <= 5) {
            int i = r - 1;
            g_balance.turret_damage[i] += delta;
            if (g_balance.turret_damage[i] < 1) g_balance.turret_damage[i] = 1;
        } else if (r >= 6 && r <= 10) {
            int i = r - 6;
            g_balance.turret_fire_interval[i] += delta;
            if (g_balance.turret_fire_interval[i] < 1) g_balance.turret_fire_interval[i] = 1;
        } else if (r >= 11 && r <= 15) {
            int i = r - 11;
            g_balance.turret_range[i] += delta * 2;
            if (g_balance.turret_range[i] < 0) g_balance.turret_range[i] = 0;
            if (g_balance.turret_range[i] > 140) g_balance.turret_range[i] = 140;
        } else if (r >= 16 && r <= 20) {
            int i = r - 16;
            g_balance.conveyor_reload_interval[i] += delta;
            if (g_balance.conveyor_reload_interval[i] < 1) g_balance.conveyor_reload_interval[i] = 1;
        } else if (r >= 21 && r <= 25) {
            int i = r - 21;
            g_balance.turret_magazine[i] += delta;
            if (g_balance.turret_magazine[i] < 1) g_balance.turret_magazine[i] = 1;
        }
        calib_commit_changes();
        return;
    }
    if (g_game.calib_page == 3) {
        int r = g_game.calib_row;
        int64_t *val_ptr = NULL;
        if (r >= 0 && r <= 3) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[0][r];
        } else if (r >= 4 && r <= 7) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[1][r - 4];
        } else if (r >= 8 && r <= 11) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[2][r - 8];
        } else if (r >= 12 && r <= 13) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[3][r - 12];
        } else if (r >= 14 && r <= 17) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[4][r - 14];
        } else if (r == 18) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[5][0];
        } else if (r >= 19 && r <= 20) {
            val_ptr = (int64_t *)&g_balance.upgrade_costs[6][r - 19];
        } else if (r >= 21 && r <= 24) {
            val_ptr = (int64_t *)&g_balance.range_upgrade_costs[r - 21];
        }
        if (val_ptr) {
            int64_t n = *val_ptr + delta;
            if (n < 0) n = 0;
            if (n > 999999) n = 999999;
            *val_ptr = n;
        }
        calib_commit_changes();
        return;
    }
}

void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held) {
    // B exits calibration back to paused
    if (keys_down & KEY_B) {
        calib_commit_changes();
        g_game.mode = MODE_PAUSED;
        tiles_full_screen_refresh();
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

    // L / R cycle stage index (0..4) in Page 0
    if (g_game.calib_page == 0) {
        if (keys_down & KEY_L) {
            g_game.calib_stage_idx = (g_game.calib_stage_idx + STAGE_COUNT - 1) % STAGE_COUNT;
        }
        if (keys_down & KEY_R) {
            g_game.calib_stage_idx = (g_game.calib_stage_idx + 1) % STAGE_COUNT;
        }
    }

    // Up / Down: select parameter row. Held buttons repeat, then accelerate.
    int max_rows = (g_game.calib_page == 0) ? 7 : ((g_game.calib_page == 1) ? 40 : ((g_game.calib_page == 2) ? 26 : 25));
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

    // Left / Right step size
    int step = 1;
    if (g_game.calib_page == 0) {
        step = (g_game.calib_row == 6) ? 50 : 5;
    } else if (g_game.calib_page == 1) {
        int f = g_game.calib_row % 5;
        step = (f == 1) ? 2 : 1;
    } else if (g_game.calib_page == 2) {
        step = (g_game.calib_row == 0) ? 10 : ((g_game.calib_row >= 11 && g_game.calib_row <= 15) ? 2 : 1);
    } else if (g_game.calib_page == 3) {
        step = (g_game.calib_row >= 19 && g_game.calib_row <= 20) ? 50 : 10;
    }

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
        // Stage Selector buttons [<] (8..42, 16..34) and [>] (214..248, 16..34)
        if (g_game.calib_page == 0 && touch.py >= 16 && touch.py <= 34) {
            if (touch.px >= 8 && touch.px <= 42) {
                g_game.calib_stage_idx = (g_game.calib_stage_idx + STAGE_COUNT - 1) % STAGE_COUNT;
                return;
            } else if (touch.px >= 214 && touch.px <= 248) {
                g_game.calib_stage_idx = (g_game.calib_stage_idx + 1) % STAGE_COUNT;
                return;
            }
        }

        // Parameter rows touch hitboxes
        if (g_game.calib_page == 0) {
            for (int r = 0; r < 7; r++) {
                int ry = 36 + r * 15;
                if (touch.py >= ry && touch.py <= ry + 13) {
                    g_game.calib_row = r;
                    int rstep = (r == 6) ? 50 : 5;
                    if (touch.px >= 175 && touch.px <= 205) {
                        calib_modify_val(-rstep);
                    } else if (touch.px >= 212 && touch.px <= 242) {
                        calib_modify_val(+rstep);
                    }
                    return;
                }
            }
        } else {
            int visible_first = (g_game.calib_row / 10) * 10;
            int visible_count = (visible_first + 10 < max_rows) ? 10 : (max_rows - visible_first);
            for (int r = 0; r < visible_count; r++) {
                int actual = visible_first + r;
                int ry = 32 + r * 13;
                if (touch.py >= ry && touch.py <= ry + 12) {
                    g_game.calib_row = actual;
                    if (touch.px >= 175 && touch.px <= 205) {
                        calib_modify_val(-1);
                    } else if (touch.px >= 212 && touch.px <= 242) {
                        calib_modify_val(+1);
                    }
                    return;
                }
            }
        }

        // Bottom action buttons:
        // [RESTART W1] (8..85, 158..186)
        if (touch.px >= 8 && touch.px <= 85 && touch.py >= 158 && touch.py <= 186) {
            game_init();
            return;
        }
        // [DEFAULTS] (95..165, 158..186)
        if (touch.px >= 95 && touch.px <= 165 && touch.py >= 158 && touch.py <= 186) {
            balance_config_reset_defaults();
            balance_config_save();
            g_game.calib_saved_timer = 30;
            return;
        }
        // [BACK / RESUME] (175..248, 158..186)
        if (touch.px >= 175 && touch.px <= 248 && touch.py >= 158 && touch.py <= 186) {
            calib_commit_changes();
            g_game.mode = MODE_PAUSED;
            tiles_full_screen_refresh();
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
    // Do not interpret stale touch coordinates while the entry chord is held.
    if ((keys_held & KEY_L) && (keys_held & KEY_SELECT)) return;

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
