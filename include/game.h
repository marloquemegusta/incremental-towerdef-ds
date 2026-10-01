#ifndef GAME_H
#define GAME_H

#include <nds.h>
#include "wall_data.h"
#include "enemy_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define SCREEN_W 256
#define SCREEN_H 192
#define FIELD_H  384 // Vertical unified battlefield (256x384)

#define MAX_ENEMIES 384
#define MAX_BULLETS 64
#define MAX_DEATH_PARTICLES 256
#define ENEMY_DEATH_FRAMES 15 // Liquefaction animation length (~0.25 s @ 60 FPS)

// Death ground cone (the big directional blood fan). Disabled by default; the whole
// implementation (tiles_stamp_death_cone) stays in place, so bringing it back is just
// flipping this to 1 and rebuilding.
#define DEATH_CONE_ENABLED 0
#define MAX_TURRETS 4
#define MAX_CASINGS 256
#define MAX_BULLET_DARTS 64

// Live dismemberment (cosmetic). Each impact tears one cell out of the living
// sprite. The cell is a NORMALIZED grid over the sprite bounding box, not an
// absolute pixel block, so the hole stays in place while the walk animation and
// the facing direction change (see DESIGN.md [OQ-12]). Purely visual: it never
// touches hp, damage, speed or collision.
//
// Tuning note: only ~8-12 impacts land inside the battery's firing window before
// the xeno walks past the wall, so the CELL SIZE is what decides legibility.
// 8x8 (64 cells) over a 69x59 Defiler tears ~8px cells and reads as no damage at
// all; 4x4 (16 cells) erases the body. 6x6 with a 60% cap is the balance between
// "visible mutilation" and "still a recognisable xeno".
#define WOUND_GRID 6
#define WOUND_CELLS (WOUND_GRID * WOUND_GRID)
#define WOUND_WORDS ((WOUND_CELLS + 31) / 32)
#define WOUND_MAX_PCT 60 // Cap: never tear more than this % of the grid cells
// The body only pays for damage it has actually taken: the share of the wound
// budget available is the share of health already lost, quantised into this many
// health steps. A nearly intact xeno therefore stays nearly intact no matter how
// many rounds it eats, and it never reaches the wall looking wrecked while it still
// hits hard. WOUND_MAX_PCT is the budget at death's door.
#define WOUND_HP_TICKS 8
// 1 = amputation: what is only 2 px across is severed outright and whatever a bite
//     disconnects falls off, so the xeno visibly loses limbs and never keeps a
//     fragment floating.
// 0 = no amputation: nothing is ever removed. A bitten cell simply reddens (the torn
//     carapace read) and the silhouette never changes. This is the default: it reads
//     as "the shell is being stripped off" without ever breaking the silhouette.
#define WOUND_AMPUTATE 0
#define LIVE_DISMEMBERMENT_ENABLED 1 // A/B switch: 0 restores pristine sprites
// Per-pixel bite threshold (0..255). Within a torn cell only the pixels that pass
// this test are actually gone, so the bite is ragged instead of a perfect square.
// The same seed drives the hole and the flying debris, so the piece that spins
// away is exactly the piece missing from the body.
#define WOUND_DENSITY 176
// 1 = torn carapace: the cell is repainted as exposed dark-red flesh (reads as
//     "a chunk was ripped off", used for the outer shell).
// 0 = see-through hole: the cell is skipped so the ground shows through.
#define WOUND_STYLE 1
// A pixel is severed outright (rather than repainted as exposed flesh) only when it
// belongs to a genuinely skinny feature: its 3x3 neighbourhood is not fully solid,
// i.e. the local limb is at most 2 px across. Anything bulkier keeps its mass and
// just shows the torn carapace.
#define WOUND_THIN_R 1

// Deterministic per-pixel bite pattern, shared by the sprite draw and the debris
// so both agree on the exact shape of the missing piece. The pattern is computed on
// 2x2 blocks: pixels leave in clumps instead of as isolated speckles, so a bite
// never scatters orphan dots around the wound.
static inline int wound_pixel_gone(int cell_bit, int sx, int sy) {
    int hsh = (((sx >> 1) * 73) ^ ((sy >> 1) * 151) ^ (cell_bit * 977)) & 255;
    return hsh < WOUND_DENSITY;
}

// Per-pixel wound state.
#define WOUND_NONE 0
#define WOUND_FLESH 1    // carapace torn off: the dark flesh shows underneath
#define WOUND_SEVERED 2  // gone outright: the ground shows through

// The per-cell mask packs two bit planes in one array: [0, WOUND_WORDS) marks the
// bitten cells, [WOUND_WORDS, WOUND_MASK_WORDS) marks the cells torn off whole
// because a bite disconnected them from the body (no floating fragments).
#define WOUND_MASK_WORDS (WOUND_WORDS * 2)

static inline int wound_bit_get(const uint32_t *mask, int bit) {
    return (mask[bit >> 5] >> (bit & 31)) & 1;
}

static inline void wound_bit_set(uint32_t *mask, int bit) {
    mask[bit >> 5] |= 1u << (bit & 31);
}

static inline int wound_gone_get(const uint32_t *mask, int bit) {
    return (mask[WOUND_WORDS + (bit >> 5)] >> (bit & 31)) & 1;
}

static inline void wound_gone_set(uint32_t *mask, int bit) {
    mask[WOUND_WORDS + (bit >> 5)] |= 1u << (bit & 31);
}

// A pixel belongs to a skinny feature when its (2*WOUND_THIN_R+1)^2 neighbourhood is
// not fully solid. Those are severed outright; everything else keeps mass.
static inline int wound_pixel_is_thin(const uint8_t *src, int w, int h, int sx, int sy) {
    for (int dy = -WOUND_THIN_R; dy <= WOUND_THIN_R; dy++) {
        for (int dx = -WOUND_THIN_R; dx <= WOUND_THIN_R; dx++) {
            int nx = sx + dx;
            int ny = sy + dy;
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) return 1;
            if (!src[ny * w + nx]) return 1;
        }
    }
    return 0;
}

// Fixed point math: Q8 (256 = 1.0)
#define FP_SHIFT 8
#define FP_ONE (1 << FP_SHIFT)
#define TO_FP(x) ((x) << FP_SHIFT)
#define FROM_FP(x) ((x) >> FP_SHIFT)

// Colors (15-bit RGB)
#define COLOR_BLACK          (RGB15(0, 0, 0) | BIT(15))
#define COLOR_DECK_FLOOR     (RGB15(2, 2, 3) | BIT(15))
#define COLOR_DECK_GRID      (RGB15(4, 4, 5) | BIT(15))
#define COLOR_HAZARD_YELLOW  (RGB15(31, 25, 0) | BIT(15))
#define COLOR_HAZARD_BLACK   (RGB15(2, 2, 2) | BIT(15))

#define COLOR_IRON_PANEL     (RGB15(5, 5, 6) | BIT(15))
#define COLOR_IRON_BORDER    (RGB15(10, 10, 12) | BIT(15))
#define COLOR_IRON_LIGHT     (RGB15(16, 16, 18) | BIT(15))
#define COLOR_BRASS          (RGB15(24, 18, 5) | BIT(15))

#define COLOR_BUNKER_DOOR    (RGB15(8, 8, 9) | BIT(15))
#define COLOR_BUNKER_CORE    (RGB15(18, 4, 4) | BIT(15))
#define COLOR_BUNKER_LIGHT   (RGB15(0, 31, 10) | BIT(15))

#define COLOR_TURRET_BASE    (RGB15(7, 7, 8) | BIT(15))
#define COLOR_TURRET_RING    (RGB15(14, 14, 16) | BIT(15))
#define COLOR_TURRET_CUPOLA  (RGB15(11, 11, 13) | BIT(15))
#define COLOR_BARREL_STEEL   (RGB15(20, 20, 22) | BIT(15))
#define COLOR_BARREL_TIP     (RGB15(28, 28, 30) | BIT(15))
#define COLOR_MUZZLE_FLASH   (RGB15(31, 29, 8) | BIT(15))
#define COLOR_BOLTER_TRACER  (RGB15(31, 27, 4) | BIT(15))

// Xenos Palette
#define COLOR_XENOS_CHITIN   (RGB15(14, 4, 19) | BIT(15))
#define COLOR_XENOS_FLESH    (RGB15(25, 10, 30) | BIT(15))
#define COLOR_XENOS_EYE      (RGB15(31, 5, 4) | BIT(15))
#define COLOR_XENOS_ICHOR    (RGB15(8, 31, 6) | BIT(15))
#define COLOR_BLOOD_DARK     (RGB15(16, 2, 8) | BIT(15))
#define COLOR_XENOS_GORE_CORE (RGB15(31, 6, 8) | BIT(15))   // bright arterial blood
#define COLOR_XENOS_GORE_MID  (RGB15(20, 2, 4) | BIT(15))   // blood red
#define COLOR_XENOS_GORE_DARK (RGB15(9, 1, 2) | BIT(15))    // dried crimson

// UI / Phosphor
#define COLOR_PHOSPHOR_GREEN (RGB15(6, 31, 10) | BIT(15))
#define COLOR_AMBER          (RGB15(31, 22, 0) | BIT(15))
#define COLOR_LED_RED        (RGB15(31, 2, 2) | BIT(15))
#define COLOR_LED_GREEN      (RGB15(2, 31, 4) | BIT(15))
#define COLOR_WHITE          (RGB15(31, 31, 31) | BIT(15))
#define COLOR_RED            (RGB15(28, 4, 4) | BIT(15))
#define COLOR_DARK_GRAY      (RGB15(6, 6, 7) | BIT(15))

typedef struct {
    int x, y;            // Q8 fixed point in global space [0..255, 0..383]
    int vx, vy;          // Q8 velocities for lead-target prediction
    uint64_t hp;
    uint64_t max_hp;
    uint64_t incoming_damage; // Anti-overkill virtual health damage in flight
    int active;
    int speed;           // Integer pixels per second; converted to Q8 per simulation tick
    int variant;         // 0..7 (enemy variant / species index)
    int dir;             // 8-way compass: N, NE, E, SE, S, SW, W, NW
    int anim_frame;
    int anim_distance;   // Q8 distance accumulated for the next walk pose
    int biting_target;   // -1=None/Marching, 0..3=Turret ID, 99=Bunker Sanctum
    int bite_timer;

    // Death liquefaction: the sprite melts into a puddle over ENEMY_DEATH_FRAMES
    int dying;
    int death_timer;
    int death_dir_x, death_dir_y; // Q8 unit vector of the killing blow (256 = 1.0)

    // Live dismemberment: bitmask of the cells bitten while the xeno is still alive.
    // Bit (cell_y * WOUND_GRID + cell_x) set => that normalized cell is bitten.
    // See WOUND_MASK_WORDS: the second plane marks cells torn off whole.
    uint32_t wound_bits[WOUND_MASK_WORDS];
    uint8_t wound_count;

    // 60fps Dirty Rects tracking: independent for top and bottom screens
    int prev_top_active;
    int prev_top_x, prev_top_y, prev_top_w, prev_top_h;
    int prev_bot_active;
    int prev_bot_x, prev_bot_y, prev_bot_w, prev_bot_h;
} Enemy;

typedef struct {
    int id;
    int type;            // 0=TURRET_TYPE_BOLTER, 1=TURRET_TYPE_LASCANNON
    int x, y;            // Local screen coordinates in bottom screen [0..255, 0..191]
    int current_angle;   // 0..255 angle
    int target_angle;
    int range;           // Circular omnidirectional radius in px
    int active;
    int placed;
    
    // Integrity
    int hp;
    int max_hp;

    // Ammo & Logistics
    int ammo;
    int max_ammo;
    int fire_cooldown;
    int fire_interval;
    int flash_timer;

    // Recoil animation & sparks
    int anim_frame;
    int barrel_recoil_l;
    int barrel_recoil_r;
    int last_barrel;

    // Manual targeting / locked enemy
    int locked_enemy_idx;

    // Metrics
    uint64_t shots_fired;
    uint64_t hits_confirmed;
    uint64_t damage_dealt;
    uint64_t kills;
} Turret;

typedef struct {
    int x, y;   // Q8 fixed point (local bottom screen)
    int vx, vy; // Q8 fixed point
    int life;
    int active;
    int turret_idx;
    uint64_t damage;

    int prev_x, prev_y;
    int prev_active;
} Bullet;

typedef struct {
    int x, y;       // Q8 global coordinates
    int z;          // Q8 height
    int vx, vy, vz; // Q8 velocities
    int life;
    int size;
    int active;
    uint16_t color;

    int prev_top_active;
    int prev_top_x, prev_top_y;
    int prev_bot_active;
    int prev_bot_x, prev_bot_y;
    int prev_bot_has_shadow, prev_bot_sy;
} DeathParticle;

// Solid xeno debris: small blocks literally cut out of the enemy sprite (same
// palette indices), thrown ballistically and tumbling to the ground.
#define MAX_GORE_CHUNKS 48
typedef struct {
    int x, y, z;       // Q8 global x/y, z = height above ground
    int vx, vy, vz;    // Q8 velocities
    int life;
    int active;
    int w, h;          // block size (max 4x4)
    uint8_t idx[16];   // enemy palette indices (0 = transparent)

    int prev_bot_active;
    int prev_bot_x, prev_bot_y;
    int prev_bot_has_shadow, prev_bot_sy;
} GoreChunk;

typedef struct {
    int x, y, z;       // Q8 local bottom screen coordinates (z = height above ground)
    int vx, vy, vz;    // Q8 velocities
    int angle;         // 0..360 deg
    int spin_speed;    // deg per frame
    int bounces;
    int life;
    int active;

    int prev_cx, prev_cy;
    int prev_active;
} CasingParticle;

typedef struct {
    int x, y;          // Q8 local bottom screen coordinates
    int vx, vy;        // Q8 velocities
    int target_x, target_y;
    int dist_remaining;// Q8 distance remaining
    int damage;
    int range;           // Maximum ballistic reach in px
    int active;
    int target_enemy_idx; // Tracked enemy index (-1 if none)

    int prev_bx, prev_by;
    int prev_active;
} BulletDart;

typedef struct {
    int screen_y;        // Local screen Y (default 144)
    uint64_t hp;
    uint64_t max_hp;
    int active_turrets;  // 1..4 (number of active turrets / sockets)
    int turret_angles[4];// Current display angle (0..4) for each socket
    int target_angles[4];// Target angle (0..4) each turret wants to face
    int last_aim_angle[4]; // Resting stance: last angle this socket fired at (0..4)
    int turret_cooldown[4]; // Independent cooldown for each turret socket
    int barrel_alt[4];   // 0 or 1 for left/right muzzle
    int muzzle_flash_timer[4];
    int muzzle_flash_barrel[4];
    int turret_recoil[4];    // Frames of hydraulic kickback remaining (0..3)
    int target_enemy_idx[4]; // Enemy currently tracked by each socket (-1 if none)
    int traverse_timer[4];   // Sub-frame timer for smooth mechanical rotation

    // Ammo, Magazine & Reload Logistics Schema
    int ammo[4];         // Current rounds in magazine for each socket
    int max_ammo[4];     // Drum capacity per socket (default 10 in Phase 1)
    int reload_timer[4]; // Frames remaining in active reload cycle
    int reload_time;     // Base reload duration (default 90 frames = 1.5s)
    int is_reloading[4]; // 1 if socket is empty / requires manual reload

    int battery_fire_step; // Metronome round-robin counter alternating turrets and barrels
    int damage_flash_timer; // Feedback trauma flash when wall integrity is damaged
    int fire_cooldown;   // Global wall battery cadence throttle
    int fire_interval;   // Fire rate (frames between rounds, default 12 for 1 turret)
    int damage;          // Damage per bullet impact (default 1)
    int range;           // Dormant: derived from range_line_y, unused
    int range_line_y;    // Dormant defensive fire line; 0 = whole bottom battlefield targetable
    int locked_enemy_idx;// Player-designated priority target (-1 if none)
    int conveyor_timer;  // Auto-feed cadence counter
} WallPlatform;

#define AMMO_DEPOT_X 128
#define AMMO_DEPOT_Y 166
#define AMMO_DEPOT_W 24
#define AMMO_DEPOT_H 16

void wall_reload_socket(int socket_idx);
void wall_reload_battery(void);
void wall_fire_at_target(int target_x, int target_y, int enemy_idx);
void wall_apply_balance_and_upgrades(void);

#define GENERATOR_TIER_COUNT 7
#define GENERATOR_TIER_MAX_HP 5

typedef struct {
    int hp;             // 0..5
    int max_hp;         // 5
    int active;         // 1 if intact, 0 if breached/offline
    int damage_flash;   // visual flash timer on hit
} GeneratorTier;

typedef struct {
    GeneratorTier tiers[GENERATOR_TIER_COUNT];
    int built_tiers;    // 0..7 (number of automation tiers erected by upgrades)
    int active_tier;    // index of highest intact tier (0..6)
    int total_hits;
} Generator;

extern Generator g_generator;

void generator_init(void);
void generator_build_tier(int tier_idx);
void generator_take_hit(int enemy_tier);
void generator_repair_tier(int tier_idx, int amount);
void generator_draw_bays(uint16_t *buffer, int y);


typedef enum {
    MODE_PREPARATION = 0,
    MODE_WAVE,
    MODE_PAUSED,
    MODE_GAME_OVER,
    MODE_UPGRADES,
    MODE_CALIBRATION,
    MODE_DEBUG_SANDBOX,
    MODE_VICTORY
} GameMode;

// Debug / Test Sandbox parameters state
typedef struct {
    int enemy_tier;       // 0..7 (variant; hp/speed come from the master table)
    int turret_firerate;  // 1..30 frames fire interval
    int turret_damage;    // 1..999 damage per bullet
    int turret_infinite_ammo; // 1 = infinite ammo
    int run_sim;          // 0 = paused/step, 1 = live continuous
    int separation_enabled;
    int profiler_compact;
    int edit_row;         // 0..2 for D-Pad parameter tuning
    int spawn_count;
    int last_keys_down;   // Diagnostic input trace for the sandbox profiler
    int last_keys_held;
} DebugSandboxState;

// Editable configuration per enemy tier inside each Wave
#define STAGE_COUNT 5
#define STAGE_DURATION_FRAMES (120 * 60) // 7200 frames = 2 minutes
#define STAGE_PEAK_START_FRAME (90 * 60)  // 5400 frames = 1m 30s (when timer <= 1800)

// Configuration for each 2-minute Stage (8 parameters exposed in calibration)
typedef struct {
    int zergling_delay_base;   // 1. Zergling delay during 0:00 - 1:30 (frames, 0 = disabled)
    int scourge_delay_base;    // 2. Scourge delay during 0:00 - 1:30
    int hydralisk_delay_base;  // 3. Hydralisk delay during 0:00 - 1:30
    int zergling_delay_peak;   // 4. Zergling delay during peak (1:30 - 2:00)
    int scourge_delay_peak;    // 5. Scourge delay during peak
    int hydralisk_delay_peak;  // 6. Hydralisk delay during peak
    int ultralisk_delay_peak;  // 7. Ultralisk delay during peak (frames, 0 = disabled)
    int stage_reward_scrap;    // 8. Scrap reward upon completing the stage
} StageConfig;

// Master per-variant enemy stats: the single source of truth for spawning, scrap
// on death, bite damage/cadence, the debug sandbox and the calibration menu.
typedef struct {
    uint8_t  tier;           // 1..4 threat tier (threat level grouping)
    uint32_t hp;
    int      speed;          // px/s
    uint32_t scrap;
    int      bite_damage;
    int      bite_interval;  // frames
} EnemyStatDef;

typedef struct {
    StageConfig stages[STAGE_COUNT]; // 5 stages of 2 minutes each
    EnemyStatDef enemy[ENEMY_VARIANT_COUNT]; // Master stats: tier + hp/speed/scrap/bite per variant
    uint64_t upgrade_costs[7][5];
    int turret_damage[5];
    int turret_fire_interval[5];
    int turret_range[5];             // Dormant defensive fire line (Y); 0 = full bottom battlefield
    int turret_magazine[5];
    int bunker_start_hp;
    int conveyor_reload_interval[5];
    uint64_t range_upgrade_costs[5]; // Dormant: RANGE upgrade retired
    uint32_t magic;                  // 0x544F5737 ("TOW7")
} GameBalanceConfig;

extern GameBalanceConfig g_balance;
void balance_config_init(void);
void balance_config_save(void);
void balance_config_load(void);
void balance_config_reset_defaults(void);

// Incremental Upgrades
typedef struct {
    // Branch A: Battery Stats
    int caliber_lvl;     // +Damage per bullet (Base 2 -> 3 -> 4 -> 6 -> 8)
    int firerate_lvl;    // +Cadence (Interval 18 -> 14 -> 10 -> 6)
    int range_lvl;       // Dormant: RANGE upgrade retired, always 0
    int mag_size_lvl;    // +Max ammo capacity (20 -> 35 -> 50 -> 80)

    // Branch B: Economy
    int bio_harvest_lvl; // Extra scrap multiplier
    int bunker_armor_lvl;// Bunker HP and DR

    // Branch C: Automation
    int continuous_fire; // 0=Click per shot, 1=Continuous hold spray
    int auto_target;     // 0=Manual, 1=Nearest, 2=Strongest
    int conveyor_lvl;    // 0=Manual reload, 1=1/s, 2=3/s, 3=6/s
    int extra_turrets;   // Extra unlocked turrets (0..3)
} UpgradeTree;

typedef struct {
    GameMode mode;
    GameMode previous_mode;
    int wave_number;
    int wave_timer;      // frames remaining in wave (30s = 1800f)
    int total_waves;     // 20 waves

    uint64_t bunker_hp;
    uint64_t bunker_max_hp;
    uint64_t scrap;      // Huge incremental numbers (up to millions/billions)

    int enemies_spawned;
    int enemies_to_spawn;
    int enemies_alive;
    uint64_t enemies_killed;
    int spawn_timer;

    // Stream Spawner (City Defense Incremental)
    int spawn_rate_q8;       // Enemies per second in Q8
    int spawn_budget_q8;     // Fractional accumulator (units: rate_q8 * frames)
    int dial_quantity;       // Compatibility: integer enemies/s
    int dial_max_tier;       // Compatibility: integer max biocaste tier
    int dial_rate_ticks;     // Dial rate in 0.05/s units, source of truth (0..200 => 0.00..10.00/s)
    int dial_tier_ticks;     // Dial threat tier in 0.05 units, source of truth (20..80 => T1.00..T4.00)
    int dial_rate_hold_timer; // D-pad UP/DOWN autorepeat timer for the rate dial
    int dial_tier_hold_timer; // D-pad LEFT/RIGHT autorepeat timer for the threat dial
    int dial_rate_q8;        // Derived from dial_rate_ticks: continuous enemies/s in Q8 (512 = 2.0/s)
    int dial_tier_q8;        // Derived from dial_tier_ticks: biocaste threat in Q8 (256 = T1.0, 333 = T1.3)

    int fast_forward;
    int sim_ticks_elapsed;

    // Stylus drag & reload state
    int is_dragging_ammo;
    int is_dragging_turret;
    int drag_turret_slot;
    int drag_x, drag_y;
    int prev_drag_x, prev_drag_y;
    int prev_drag_active;
    int selected_turret;
    int upgrade_flash_timer;
    int upgrade_flash_idx;

    // Stage spawning timers
    int spawn_timer_zergling;
    int spawn_timer_scourge;
    int spawn_timer_hydra;
    int spawn_timer_ultra;
    int stage_completed_flag;

    // Calibration UI navigation
    int calib_row;               // Selected row within active page
    int calib_stage_idx;         // 0..4 (Etapa 1..5)
    int calib_page;              // 0: Etapas (1..5), 1: Enemy Stats, 2: Base/Turrets, 3: Upgrades
    int calib_hold_timer;        // For autorepeat continuous adjustment
    int calib_saved_timer;       // Feedback notification ("SAVED")

    UpgradeTree upgrades;
    DebugSandboxState sandbox;

    // Hardware Profiler Telemetry (in ticks, 33 ticks = 1ms, budget = 545 ticks/frame)
    int prof_fps;
    int prof_sim_ticks;
    int prof_top_ticks;
    int prof_bot_ticks;
    int prof_pres_ticks;
    int prof_enemies_active;
    int prof_sep_checks;
    int prof_target_candidates;
    int prof_collision_candidates;
    int prof_bot_base_ticks;
    int prof_bot_enemy_ticks;
    int prof_bot_fx_ticks;
    int prof_bot_ui_ticks;
    int prof_top_restore_ticks;
    int prof_top_enemy_ticks;
} GameContext;

// Upgrades helper
uint64_t upgrade_get_cost(int idx);
int upgrade_can_afford(int idx);
void upgrade_purchase(int idx);

// Global declarations
extern GameContext g_game;
extern Turret g_turrets[MAX_TURRETS];
#define g_turret g_turrets[0]
extern Enemy g_enemies[MAX_ENEMIES];
extern Bullet g_bullets[MAX_BULLETS];
extern DeathParticle g_death_particles[MAX_DEATH_PARTICLES];
extern GoreChunk g_gore_chunks[MAX_GORE_CHUNKS];
extern WallPlatform g_wall;
extern CasingParticle g_casings[MAX_CASINGS];
extern BulletDart g_bullet_darts[MAX_BULLET_DARTS];

void wall_init(void);
void wall_update(void);
void wall_fire_at(int target_x, int target_y);
void wall_fire_socket(int socket_idx, int target_x, int target_y);
int wall_angle_from_target(int turret_x, int turret_y, int target_x, int target_y);
void wall_spawn_casing(int x, int y, int dir_sign);
int wall_spawn_bullet_dart(int start_x, int start_y, int target_x, int target_y, int target_enemy_idx);
extern uint16_t *g_backbuffer;
extern uint8_t *g_top_backbuffer;

// Math
void math_init(void);
int fixed_sin(int angle);
int fixed_cos(int angle);
int fixed_atan2(int dy, int dx);

// Simulation
void game_init(void);
void game_start_wave(void);
void game_update_simulation(void);
void game_handle_input_prep(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_wave(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_pause(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_game_over(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_victory(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_upgrades(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_sandbox(touchPosition touch, int keys_down, int keys_held);
void game_sandbox_spawn_enemy(int x, int y);
void game_sandbox_load_stress_profile(void);
void game_toggle_pause(void);
void game_reset_to_prep(void);
void game_add_splatter_ex(int x, int y, uint16_t color, int size, int duration);
void game_spawn_death_gore(int x, int y, int bvx, int bvy, int variant);
int game_is_pos_valid(int x, int y);

// Helpers
void format_number_compact(char *buf, size_t buf_size, uint64_t val);

// Renderer (Bottom and Top Screen Battlefield)
void renderer_init(void);
void renderer_clear(uint16_t color);
void renderer_draw_pixel(int x, int y, uint16_t color);
void renderer_draw_rect(int x, int y, int w, int h, uint16_t color);
void renderer_fill_rect(int x, int y, int w, int h, uint16_t color);
void renderer_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void renderer_draw_circle(int cx, int cy, int radius, uint16_t color, int filled);
void renderer_draw_text(int x, int y, const char *str, uint16_t color);

void renderer_draw_battlefield_bottom(void);
void renderer_draw_wall(void);
void renderer_draw_battlefield_top(void);
void renderer_draw_turret(const Turret *t, int is_selected);
void renderer_draw_enemies_bottom(void);
void renderer_draw_enemies_top(void);
void renderer_draw_bullets(void);
void renderer_draw_splatters_bottom(void);
void renderer_draw_splatters_top(void);
void renderer_draw_death_particles_bottom(void);
void renderer_draw_gore_chunks_bottom(void);
void renderer_draw_death_particles_top(void);
void renderer_draw_ui_prep(void);
void renderer_draw_ui_wave(void);
void renderer_draw_ui_pause(void);
void renderer_draw_ui_game_over(void);
void renderer_draw_ui_victory(void);
void renderer_draw_ui_upgrades(void);
void renderer_draw_ui_calibration(void);
void renderer_draw_ui_sandbox(void);
void renderer_present(void);
void renderer_refresh_top_vram(void);

// Top screen presentation
void top_screen_present(void);

#endif // GAME_H

// Top screen 8-bit indexed palette constants (0..255)
#define TOP_COLOR_TRANSPARENT    0
#define TOP_C_CONC_BASE          131
#define TOP_C_CONC_LIGHT         132
#define TOP_C_CONC_DARK          133
#define TOP_C_CONC_BEVEL         134
#define TOP_C_JOINT              135
#define TOP_C_GRASS_DEEP         136
#define TOP_C_GRASS_MID          137
#define TOP_C_GRASS_TALL         138
#define TOP_C_OIL_DARK           139
#define TOP_C_OIL_MID            140
#define TOP_C_CRACK_LINE         141
#define TOP_C_BAG_DARK           142
#define TOP_C_BAG_MID            143
#define TOP_C_BAG_HI             144
#define TOP_C_DRAIN_GRATE        145
#define TOP_C_DRAIN_HOLE         146

#define TOP_COLOR_BLACK          200
#define TOP_COLOR_WHITE          201
#define TOP_COLOR_AMBER          202
#define TOP_COLOR_LED_GREEN      203
#define TOP_COLOR_LED_RED        204
#define TOP_COLOR_IRON_LIGHT     205
#define TOP_COLOR_IRON_PANEL     206
#define TOP_COLOR_IRON_BORDER    207
#define TOP_COLOR_PHOSPHOR_GREEN 208
#define TOP_COLOR_BLOOD          209
#define TOP_COLOR_DARK_GRAY      210
#define TOP_COLOR_XENOS_GORE_CORE 211
#define TOP_COLOR_XENOS_GORE_MID  212
#define TOP_COLOR_XENOS_GORE_DARK 213
