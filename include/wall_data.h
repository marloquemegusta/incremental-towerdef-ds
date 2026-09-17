#ifndef WALL_DATA_H
#define WALL_DATA_H

#include <nds.h>
#include <stdint.h>

#define WALL_WIDTH 256
#define WALL_HEIGHT 48
#define WALL_DEFAULT_Y 144

#define WALL_SOCKET_COUNT 4

#define TURRET_SPRITE_W 44
#define TURRET_SPRITE_H 44
#define TURRET_PIVOT_X  22
#define TURRET_PIVOT_Y  38
#define WALL_TURRET_ANGLE_COUNT 5

typedef struct {
    int ml_x, ml_y;
    int mr_x, mr_y;
    int dl_x, dl_y;
    int dr_x, dr_y;
} TurretCalibratedPoints;

typedef struct {
    int x;
    int y;
    int default_angle;
} WallSocketDef;

extern const WallSocketDef c_wall_sockets[WALL_SOCKET_COUNT];
extern const TurretCalibratedPoints c_turret_points[WALL_TURRET_ANGLE_COUNT];
extern const uint16_t c_wall_bitmap[WALL_WIDTH * WALL_HEIGHT] __attribute__((aligned(4)));
extern const uint16_t c_turret_frames[WALL_TURRET_ANGLE_COUNT][TURRET_SPRITE_W * TURRET_SPRITE_H] __attribute__((aligned(4)));

void wall_draw_base(uint16_t *buffer, int wall_screen_y, uint64_t hp, uint64_t max_hp);
void wall_draw_turret_sprite(uint16_t *buffer, int dest_x, int dest_y, int angle_idx);

#define AMMO_CRATE_W 26
#define AMMO_CRATE_H 16
extern const uint16_t c_ammo_crate_sprite[AMMO_CRATE_W * AMMO_CRATE_H];

#endif // WALL_DATA_H
