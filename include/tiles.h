#ifndef TILES_H
#define TILES_H

#include "game.h"

#define TILE_SIZE 16
#define MAP_COLS 16
#define MAP_ROWS 12

void tiles_init(void);
void tiles_render_urban_ground(uint16_t *buffer, int y_offset);
void tiles_draw_central_bunker(uint16_t *buffer, int cx, int cy, uint64_t hp, uint64_t max_hp);
void tiles_draw_twin_bolters(int cx, int cy, int angle, int flash, int recoil_l, int recoil_r, int last_barrel);
void tiles_draw_xenos_to_buffer(uint16_t *buffer, int cx, int cy, int dir, int anim_frame, int variant);

#endif // TILES_H
