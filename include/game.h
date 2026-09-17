#ifndef GAME_H
#define GAME_H

#include <nds.h>
#include "wall_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define SCREEN_W 256
#define SCREEN_H 192
#define FIELD_H  384 // Vertical unified battlefield (256x384)

#define MAX_ENEMIES 384
#define MAX_BULLETS 64
#define MAX_SPLATTERS 256
#define MAX_DEATH_PARTICLES 256
#define MAX_TURRETS 4
#define MAX_CASINGS 64
#define MAX_BULLET_DARTS 64
#define WALL_TURRET_RANGE 96

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
    int speed;           // Q8 speed
    int variant;         // 0..5 (Biocaste Tier)
    int dir;             // 8-way compass: N, NE, E, SE, S, SW, W, NW
    int anim_frame;
    int anim_distance;   // Q8 distance accumulated for the next walk pose
    int biting_target;   // -1=None/Marching, 0..3=Turret ID, 99=Bunker Sanctum
    int bite_timer;

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
    int x, y;       // Global coordinates [0..255, 0..383]
    int life;
    int max_life;
    int size;
    uint16_t color;
    int prev_top_active;
    int prev_top_x, prev_top_y, prev_top_r;
    int prev_bot_active;
    int prev_bot_x, prev_bot_y, prev_bot_r;
} Splatter;

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
    int turret_cooldown[4]; // Independent cooldown for each turret socket
    int barrel_alt[4];   // 0 or 1 for left/right muzzle
    int muzzle_flash_timer[4];
    int muzzle_flash_barrel[4];
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
    int range;           // Effective ballistic radius in px (default 80, reach to Y=64)
    int range_line_y;    // Straight horizontal range perimeter line (default 64)
    int locked_enemy_idx;// Player-designated priority target (-1 if none)
    int conveyor_timer;  // Auto-feed cadence counter
} WallPlatform;

#define AMMO_DEPOT_X 128
#define AMMO_DEPOT_Y 166
#define AMMO_DEPOT_W 24
#define AMMO_DEPOT_H 16

void wall_reload_socket(int socket_idx);
void wall_fire_at_target(int target_x, int target_y, int enemy_idx);
void wall_apply_balance_and_upgrades(void);

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
    int enemy_tier;       // 0..5 (Biocaste)
    int enemy_hp;         // 1..99999
    int enemy_speed;      // 0..120 px/s (0 = frozen dummy)
    int turret_firerate;  // 1..30 frames fire interval
    int turret_range;     // 30..200 px radius
    int turret_damage;    // 1..999 damage per bullet
    int turret_infinite_ammo; // 1 = infinite ammo
    int run_sim;          // 0 = paused/step, 1 = live continuous
    int edit_row;         // 0..5 for D-Pad parameter tuning
    int spawn_count;
} DebugSandboxState;

// Editable configuration per enemy tier inside each Wave
#define STAGE_COUNT 5
#define STAGE_DURATION_FRAMES (120 * 60) // 7200 frames = 2 minutes
#define STAGE_PEAK_START_FRAME (90 * 60)  // 5400 frames = 1m 30s (when timer <= 1800)

// Configuration for each 2-minute Stage (7 parameters exposed in calibration)
typedef struct {
    int zergling_delay_base;   // 1. Zergling delay during 0:00 - 1:30 (frames, 0 = disabled)
    int scourge_delay_base;    // 2. Scourge delay during 0:00 - 1:30
    int hydralisk_delay_base;  // 3. Hydralisk delay during 0:00 - 1:30
    int zergling_delay_peak;   // 4. Zergling delay during peak (1:30 - 2:00)
    int scourge_delay_peak;    // 5. Scourge delay during peak
    int hydralisk_delay_peak;  // 6. Hydralisk delay during peak
    int stage_reward_scrap;    // 7. Scrap reward upon completing the stage
} StageConfig;

typedef struct {
    StageConfig stages[STAGE_COUNT]; // 5 stages of 2 minutes each
    uint32_t enemy_hp[8];            // Constant enemy stats across all stages
    int enemy_speed[8];              // Constant enemy speed in px/s
    uint32_t enemy_scrap[8];         // Constant scrap value
    int enemy_bite_damage[8];        // Constant bite / impact damage to wall
    int enemy_bite_interval[8];      // Constant bite cadence
    uint64_t upgrade_costs[7][5];
    int turret_damage[5];
    int turret_fire_interval[5];
    int turret_range[5];
    int turret_magazine[5];
    int bunker_start_hp;
    int conveyor_reload_interval[5];
    uint64_t range_upgrade_costs[5];
    uint32_t magic;                  // 0x544F5732 ("TOW2")
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
    int range_lvl;       // +Range radius (65 -> 80 -> 100 -> 125)
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
extern Splatter g_splatters[MAX_SPLATTERS];
extern DeathParticle g_death_particles[MAX_DEATH_PARTICLES];
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
extern uint16_t g_backbuffer[SCREEN_W * SCREEN_H];
extern uint16_t g_top_backbuffer[SCREEN_W * SCREEN_H];

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
void renderer_draw_range_perimeter(void);
void renderer_draw_battlefield_top(void);
void renderer_draw_turret(const Turret *t, int is_selected);
void renderer_draw_enemies_bottom(void);
void renderer_draw_enemies_top(void);
void renderer_draw_bullets(void);
void renderer_draw_splatters_bottom(void);
void renderer_draw_splatters_top(void);
void renderer_draw_death_particles_bottom(void);
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

// Top screen presentation
void top_screen_present(void);

#endif // GAME_H
