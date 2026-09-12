#ifndef GAME_H
#define GAME_H

#include <nds.h>
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
    uint64_t hp;
    uint64_t max_hp;
    int active;
    int speed;           // Q8 speed
    int variant;         // 0..5 (Biocaste Tier)
    int dir;             // 0=East, 1=South, 2=West, 3=North
    int anim_frame;
    int biting_target;   // -1=None/Marching, 0..3=Turret ID, 99=Bunker Sanctum
    int bite_timer;
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
} Bullet;

typedef struct {
    int x, y;       // Global coordinates [0..255, 0..383]
    int life;
    int max_life;
    int size;
    uint16_t color;
} Splatter;

typedef struct {
    int x, y;       // Q8 global coordinates
    int z;          // Q8 height
    int vx, vy, vz; // Q8 velocities
    int life;
    int size;
    int active;
    uint16_t color;
} DeathParticle;

typedef enum {
    MODE_PREPARATION = 0,
    MODE_WAVE,
    MODE_PAUSED,
    MODE_GAME_OVER,
    MODE_UPGRADES,
    MODE_CALIBRATION
} GameMode;

// Editable configuration per enemy tier inside each Wave
typedef struct {
    int count;       // Total enemies to spawn of this tier in this wave
    int delay;       // Spawn interval (in frames) for this tier
    int speed;       // Speed in px/s (e.g. 15..120)
    int hp;          // Base HP for this tier in this wave
} WaveTierConfig;

// Editable configuration per Wave
typedef struct {
    WaveTierConfig tiers[3]; // Tier 0 (Larva), Tier 1 (Ripper), Tier 2 (Hormagaunt)
    uint64_t scrap_base;
} WaveDef;

typedef struct {
    WaveDef waves[20];
    uint32_t magic;           // 0x544F5744 ("TOWD")
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
    int selected_turret;
    int upgrade_flash_timer;
    int upgrade_flash_idx;

    int wave_spawned_tier[3];    // Number of enemies spawned so far for Tier 0..2 in current wave
    int wave_spawn_timer_tier[3];// Timers for each tier spawn in current wave

    // Calibration UI navigation
    int calib_row;               // 0..8 (3 rows per tier: Count, Delay, Speed)
    int calib_wave_idx;          // 0..19 (Wave 1..20)
    int calib_hold_timer;        // For autorepeat continuous adjustment
    int calib_saved_timer;       // Feedback notification ("SAVED")

    UpgradeTree upgrades;
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
void game_handle_input_upgrades(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held);
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
void renderer_draw_ui_upgrades(void);
void renderer_draw_ui_calibration(void);
void renderer_present(void);

// Top screen presentation
void top_screen_present(void);

#endif // GAME_H
