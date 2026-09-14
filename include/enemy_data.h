#ifndef ENEMY_DATA_H
#define ENEMY_DATA_H

#include <nds.h>

#define ENEMY_VARIANT_COUNT 6
#define ENEMY_RENDER_ROTATED 0
#define ENEMY_RENDER_DIRECTIONAL 1
#define ENEMY_MAX_DIRECTIONS 9
#define ENEMY_MAX_FRAMES 7

typedef struct {
    uint8_t w;
    uint8_t h;
    int8_t offset_x;
    int8_t offset_y;
    const uint16_t *pixels;
} EnemyFrameDef;

typedef struct {
    uint8_t render_mode;
    uint8_t direction_count;
    uint8_t frame_count;
    uint32_t default_hp;
    uint32_t scrap_value;
    EnemyFrameDef frames[ENEMY_MAX_DIRECTIONS][ENEMY_MAX_FRAMES];
} EnemyTypeDef;

extern const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT];

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir);
void enemy_draw_sprite_to_buffer(uint16_t *buffer, int cx, int cy, int variant, int frame, int dir);

#endif // ENEMY_DATA_H
