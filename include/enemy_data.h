#ifndef ENEMY_DATA_H
#define ENEMY_DATA_H

#include <nds.h>

#define ENEMY_VARIANT_COUNT 8
#define ENEMY_RENDER_DIRECTIONAL 1
#define ENEMY_MAX_DIRECTIONS 5
#define ENEMY_MAX_FRAMES 9
#define ENEMY_MAX_ATTACK_FRAMES 6

typedef struct {
    uint8_t w;
    uint8_t h;
    int8_t offset_x;
    int8_t offset_y;
    int8_t flip_ox;
    const uint16_t *pixels;
} EnemyFrameDef;

typedef struct {
    uint8_t render_mode;
    uint8_t is_flying;
    int8_t flight_altitude;
    uint8_t direction_count;
    uint8_t frame_count;
    uint8_t attack_frame_count;
    uint32_t default_hp;
    uint32_t scrap_value;
    EnemyFrameDef frames[ENEMY_MAX_DIRECTIONS][ENEMY_MAX_FRAMES];
    EnemyFrameDef attack_frames[ENEMY_MAX_DIRECTIONS][ENEMY_MAX_ATTACK_FRAMES];
} EnemyTypeDef;

extern const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT];

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir);
void enemy_draw_sprite_to_buffer(uint16_t *buffer, int cx, int cy, int variant, int frame, int dir, int is_attacking, int *out_bx, int *out_by, int *out_bw, int *out_bh);

#endif // ENEMY_DATA_H
