#ifndef GAME_H
#define GAME_H

#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "skills.h"

#define SCREEN_W 256
#define SCREEN_H 192

#define MAX_ENEMIES 128
#define TOTAL_WAVE_ENEMIES 100
#define MAX_BULLETS 64
#define MAX_WAYPOINTS 6
#define MAX_SPLATTERS 32
#define MAX_TURRETS 4

#include "telemetry_gfx.h"

// Fixed point math: Q8 (256 = 1.0)
#define FP_SHIFT 8
#define FP_ONE (1 << FP_SHIFT)
#define TO_FP(x) ((x) << FP_SHIFT)
#define FROM_FP(x) ((x) >> FP_SHIFT)

// Warhammer 40k Grimdark Palette (15-bit RGB)
#define COLOR_BLACK          (RGB15(0, 0, 0) | BIT(15))
#define COLOR_DECK_FLOOR     (RGB15(2, 2, 3) | BIT(15))
#define COLOR_DECK_GRID      (RGB15(4, 4, 5) | BIT(15))
#define COLOR_TRENCH_BASE    (RGB15(1, 2, 2) | BIT(15))
#define COLOR_TRENCH_GRATE   (RGB15(3, 4, 5) | BIT(15))
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

#define COLOR_CONE_LINE      (RGB15(18, 14, 2) | BIT(15))
#define COLOR_CONE_DASH      (RGB15(28, 22, 0) | BIT(15))

// Xenos / Tyranid bio-mass
#define COLOR_XENOS_CHITIN   (RGB15(18, 8, 12) | BIT(15))
#define COLOR_XENOS_FLESH    (RGB15(28, 14, 16) | BIT(15))
#define COLOR_XENOS_EYE      (RGB15(31, 28, 2) | BIT(15))
#define COLOR_XENOS_ICHOR    (RGB15(4, 28, 6) | BIT(15))
#define COLOR_BLOOD_DARK     (RGB15(12, 1, 1) | BIT(15))

// UI / Terminal
#define COLOR_PHOSPHOR_GREEN (RGB15(6, 31, 10) | BIT(15))
#define COLOR_AMBER          (RGB15(31, 22, 0) | BIT(15))
#define COLOR_LED_RED        (RGB15(31, 2, 2) | BIT(15))
#define COLOR_LED_GREEN      (RGB15(2, 31, 4) | BIT(15))
#define COLOR_WHITE          (RGB15(31, 31, 31) | BIT(15))
#define COLOR_RED            (RGB15(28, 4, 4) | BIT(15))
#define COLOR_DARK_GRAY      (RGB15(6, 6, 7) | BIT(15))

typedef struct {
    int x, y;
} Waypoint;

typedef struct {
    int x, y;            // Q8 fixed point
    int waypoint_idx;
    int hp;
    int active;
    int lateral_offset;  // Q8 offset across trench width (-5 to +5 px)
    int speed;           // Q8 speed (varying around 1.0)
    int variant;         // 0..2 for slight size/color visual difference
    int dir;             // 0 = East, 1 = South, 2 = West, 3 = North
    int anim_frame;      // 0 or 1 for walk cycle
} Enemy;

typedef struct {
    int x, y;            // Screen coordinates
    int center_angle;    // 0..255
    int current_angle;   // 0..255
    int sweep_amplitude; // 16 (~45 deg total)
    int sweep_speed;     // angle units per frame
    int sweep_dir;       // +1 or -1
    int fire_cooldown;
    int fire_interval;
    int range;
    int active;
    int placed;
    int flash_timer;     // frames to show muzzle flash

    // Recoil & Alternating barrels
    int barrel_recoil_l; // 0..3 px recoil left barrel
    int barrel_recoil_r; // 0..3 px recoil right barrel
    int last_barrel;     // 0 = left, 1 = right

    // Metrics
    int id;
    int shots_fired;
    int hits_confirmed;
    int wasted_shots;
    int damage_dealt;
    int kills;
} Turret;

typedef struct {
    int x, y;   // Q8 fixed point
    int vx, vy; // Q8 fixed point
    int life;
    int active;
    int turret_idx;
} Bullet;

typedef struct {
    int x, y;
    int life;
    uint16_t color;
} Splatter;

typedef enum {
    MODE_PREPARATION = 0,
    MODE_WAVE,
    MODE_GAME_OVER,
    MODE_WORKSHOP,
    MODE_CALIBRATION
} GameMode;

#define CALIBRATION_ROWS 12

typedef struct {
    int enemy_count;          // 5..50, default: 12
    int enemy_hp;             // 5..60, default: 15
    int enemy_speed_int;      // speed in tenths of px/f: 4..20 (0.4..2.0), default: 8
    int spawn_delay;          // 15..90 frames, default: 40
    int turret_damage;        // 2..25, default: 5
    int turret_fire_rate;     // 4..20 frames, default: 8
    int cone_spread;          // 10..90 deg, default: 35
    int sweep_speed;          // 1..4 deg/f, default: 1
    int turret_range;         // 40..90 px, default: 65
    int starting_scrap;       // 50..500 $, default: 150
    int core_lives;           // 1..30 HP, default: 10
    int selected_map;         // 0..2 (0=Trinchera, 1=Doble S, 2=Rotonda)
    int selected_row;         // 0..11
} RunCalibration;

extern RunCalibration g_calibration;

typedef struct {
    int firerate_lvl;
    int sweep_lvl;
    int scrap_lvl;
} Metaprogression;

typedef struct {
    GameMode mode;
    int wave_number;
    int core_hp;
    int core_max_hp;
    int scrap;

    int enemies_spawned;
    int enemies_alive;
    int enemies_killed;
    int enemies_breached;
    int spawn_timer;

    int fast_forward;
    int sim_ticks_elapsed;

    // Dock & selection
    int turret_dock_count;
    int is_dragging_new;
    int drag_x, drag_y;
    int selected_turret;

    Metaprogression upgrades;
} GameContext;

// Global declarations
extern GameContext g_game;
extern Turret g_turrets[MAX_TURRETS];
#define g_turret g_turrets[0]
extern Enemy g_enemies[MAX_ENEMIES];
extern Bullet g_bullets[MAX_BULLETS];
extern Splatter g_splatters[MAX_SPLATTERS];
extern Waypoint g_waypoints[MAX_WAYPOINTS];
extern int g_waypoint_count;
extern const uint16_t *g_current_map_bg;
extern uint16_t g_backbuffer[SCREEN_W * SCREEN_H];

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
void game_handle_input_workshop(touchPosition touch, int keys_down, int keys_held);
void game_handle_input_calibration(touchPosition touch, int keys_down, int keys_held);
void game_reset_to_prep(void);
void map_select(int map_index);
void calibration_init(void);
void calibration_apply_settings(void);
void calibration_apply_and_start(void);
void game_add_splatter(int x, int y, uint16_t color);
int game_is_pos_valid(int x, int y);

// Renderer
void renderer_init(void);
void renderer_clear(uint16_t color);
void renderer_draw_pixel(int x, int y, uint16_t color);
void renderer_draw_rect(int x, int y, int w, int h, uint16_t color);
void renderer_fill_rect(int x, int y, int w, int h, uint16_t color);
void renderer_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void renderer_draw_circle(int cx, int cy, int radius, uint16_t color, int filled);
void renderer_draw_text(int x, int y, const char *str, uint16_t color);

void renderer_draw_trench_path(void);
void renderer_draw_turret(const Turret *t, int is_selected, int show_cone);
void renderer_draw_enemies(void);
void renderer_draw_bullets(void);
void renderer_draw_splatters(void);
void renderer_draw_ui_prep(void);
void renderer_draw_ui_workshop(void);
void renderer_draw_ui_calibration(void);
void renderer_present(void);

// Telemetry (Top Cogitator)
void telemetry_init_palette(void);
void telemetry_render_top(void);

#endif // GAME_H
