#ifndef TURRET_DATA_H
#define TURRET_DATA_H

#include <nds.h>

#define TURRET_TYPE_BOLTER    0
#define TURRET_TYPE_LASCANNON 1
#define TURRET_TYPE_COUNT     2

#define TURRET_FRAME_SIZE 32
#define TURRET_ANGLE_COUNT 16

void turret_draw_frame_angle(int cx, int cy, int type, int frame_idx, int angle_16, int is_selected);

#endif // TURRET_DATA_H
