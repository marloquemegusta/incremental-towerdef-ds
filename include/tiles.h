#ifndef TILES_H
#define TILES_H

#include "game.h"

#define TILE_SIZE 16
#define MAP_COLS 16
#define MAP_ROWS 12

#define DIRTY_BLOCK_SIZE 8
#define DIRTY_GRID_W (SCREEN_W / DIRTY_BLOCK_SIZE)
#define DIRTY_GRID_H (SCREEN_H / DIRTY_BLOCK_SIZE)

void tiles_init(void);
void tiles_render_urban_ground(uint16_t *buffer, int y_offset);
void tiles_restore_ground_rect(uint16_t *dst_buffer, int x, int y, int w, int h, int is_bottom);
void tiles_dirty_clear(int is_bottom, int buf_idx);
void tiles_dirty_mark_rect(int x, int y, int w, int h, int is_bottom, int buf_idx);
void tiles_dirty_restore(void *dst_buffer, int is_bottom, int buf_idx);
void tiles_full_screen_refresh(void);
void tiles_stamp_splatter(int x, int y, int size, uint16_t color);
void tiles_stamp_splatter_directional(int x, int y, int size, int bvx, int bvy, int variant);
void tiles_stamp_death_cone(int x, int y, int bvx, int bvy, int length, int half_width);
void tiles_stamp_ground_dot(int x, int y, uint16_t color16);
void tiles_stamp_particle_droplet(int x, int y, int size, uint16_t color);
void tiles_draw_central_bunker(uint16_t *buffer, int cx, int cy, uint64_t hp, uint64_t max_hp);
void tiles_draw_twin_bolters(int cx, int cy, int angle, int flash, int recoil_l, int recoil_r, int last_barrel);
void tiles_draw_xenos_to_buffer(uint16_t *buffer, int cx, int cy, int dir, int anim_frame, int variant);

#endif // TILES_H
