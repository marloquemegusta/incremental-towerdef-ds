#ifndef ENEMY_DATA_H
#define ENEMY_DATA_H

#include <nds.h>

#define ENEMY_VARIANT_COUNT 8
#define ENEMY_MAX_DIRECTIONS 5 // South-facing directions: E(2), SE(3), S(4), SW(5), W(6)
#define ENEMY_MAX_FRAMES 10
#define ENEMY_MAX_ATTACK_FRAMES 8

typedef struct {
    int w, h;
    int offset_x, offset_y;
    const uint8_t *pixels; // 8-bit palettized indices (aligned to 4 bytes, padded width)
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

extern uint16_t g_enemy_palette[256];
extern const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT];

void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir);
void enemy_draw_sprite_to_buffer(uint16_t *buffer, int cx, int cy, int variant, int frame, int dir, int is_attacking, int *out_bx, int *out_by, int *out_bw, int *out_bh);

#endif // ENEMY_DATA_H
