#ifndef TURRET_DATA_H
#define TURRET_DATA_H

#include <nds.h>

#define TURRET_TYPE_BOLTER    0
#define TURRET_TYPE_LASCANNON 1
#define TURRET_TYPE_COUNT     2

#define TURRET_FRAME_SIZE 32

typedef struct {
    uint8_t frame_count;
    uint8_t frame_w;
    uint8_t frame_h;
    const uint16_t *frames;
} TurretSpriteDef;

extern const TurretSpriteDef g_turret_sprites[TURRET_TYPE_COUNT];

void turret_draw_sprite_frame(int cx, int cy, int type, int frame_idx, int is_selected);

#endif // TURRET_DATA_H
