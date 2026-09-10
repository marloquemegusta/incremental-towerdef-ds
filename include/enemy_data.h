#ifndef ENEMY_DATA_H
#define ENEMY_DATA_H

#include <nds.h>

#define ENEMY_VARIANT_COUNT 5

typedef struct {
    uint8_t w;
    uint8_t h;
    const uint16_t *pixels;
} EnemyFrameDef;

typedef struct {
    uint8_t frame_count;
    uint8_t default_hp;
    uint8_t scrap_value;
    EnemyFrameDef frames[4];
} EnemyTypeDef;

extern const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT];

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir);

#endif // ENEMY_DATA_H
