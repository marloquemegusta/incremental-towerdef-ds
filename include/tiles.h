#ifndef TILES_H
#define TILES_H

#include "game.h"

#define TILE_SIZE 16
#define MAP_COLS 16
#define MAP_ROWS 12

enum {
    TILE_FLOOR_PLATE = 0,
    TILE_FLOOR_VENT,
    TILE_FLOOR_PIPES,

    TILE_TRENCH_H_TOP,
    TILE_TRENCH_H_BOT,
    TILE_TRENCH_V_LEFT,
    TILE_TRENCH_V_RIGHT,
    TILE_TRENCH_OPEN,

    TILE_OUTER_TR,
    TILE_OUTER_BR,
    TILE_OUTER_BL,
    TILE_OUTER_TL,

    TILE_CATWALK_BR,
    TILE_CATWALK_TR,
    TILE_CATWALK_TL,
    TILE_CATWALK_BL,

    TILE_BUNKER_TL,
    TILE_BUNKER_TR,
    TILE_BUNKER_BL,
    TILE_BUNKER_BR,

    TILE_COUNT
};

void tiles_init(void);
void tiles_render_map(void);
void tiles_render_sector1_map(void);
void tiles_draw_turret_base(int cx, int cy, int is_selected);
void tiles_draw_twin_bolters(int cx, int cy, int angle, int flash, int recoil_l, int recoil_r, int last_barrel);
void tiles_draw_xenos(int cx, int cy, int dir, int anim_frame, int variant);

// Sector 1 Data
extern const uint16_t g_s1_tiles[17][256];
extern const uint8_t g_s1_map[12][16];
extern const uint16_t g_hb_frames[11][1024];
extern const uint16_t g_las_frames[6][1024];

#endif // TILES_H
