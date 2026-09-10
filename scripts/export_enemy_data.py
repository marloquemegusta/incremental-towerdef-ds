import os
import sys

# Add scratch path where generator lives
sys.path.append(r'C:\Users\malfonso\.gemini\antigravity\brain\5afe97fc-2267-40e1-bf34-7201f418bd2d\scratch')
import generate_enemy_animations as ga

def to_bgr555(r, g, b, a):
    if a < 128:
        return 0
    return ((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10) | 0x8000

enemies = [
    ('t0_larva', ga.anim_t0_a_larva(), 1, 1),
    ('t1_ripper', ga.anim_t1_a_ripper(), 2, 2),
    ('t1_hormagaunt', ga.anim_t2_b_hormagaunt(), 4, 3),
    ('t2_gargoyle', ga.anim_t2_a_gargoyle(), 8, 5),
    ('t3_ravener', ga.anim_t3_a_ravener(), 16, 10),
]

os.makedirs('include', exist_ok=True)
os.makedirs('source', exist_ok=True)

with open('include/enemy_data.h', 'w') as fh:
    fh.write('''#ifndef ENEMY_DATA_H
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
''')

with open('source/enemy_data.c', 'w') as fc:
    fc.write('#include "enemy_data.h"\n#include "game.h"\n\n')
    
    for name, frames, hp, scrap in enemies:
        w, h = frames[0].size
        for f_idx, frame in enumerate(frames):
            fc.write(f'static const uint16_t s_{name}_f{f_idx}[{w * h}] = {{\n    ')
            vals = []
            for y in range(h):
                for x in range(w):
                    r, g, b, a = frame.getpixel((x, y))
                    c16 = to_bgr555(r, g, b, a)
                    vals.append(f'0x{c16:04X}')
            fc.write(', '.join(vals))
            fc.write('\n};\n\n')

    fc.write('const EnemyTypeDef g_enemy_types[ENEMY_VARIANT_COUNT] = {\n')
    for name, frames, hp, scrap in enemies:
        w, h = frames[0].size
        fc.write(f'    {{ // {name}\n')
        fc.write(f'        4, {hp}, {scrap},\n        {{\n')
        for f_idx in range(4):
            fc.write(f'            {{ {w}, {h}, s_{name}_f{f_idx} }},\n')
        fc.write('        }\n    },\n')
    fc.write('};\n\n')

    fc.write('''void enemy_draw_sprite(int cx, int cy, int variant, int frame, int dir) {
    if (variant < 0 || variant >= ENEMY_VARIANT_COUNT) variant = 0;
    frame = frame & 3;

    const EnemyFrameDef *fd = &g_enemy_types[variant].frames[frame];
    int w = fd->w;
    int h = fd->h;
    const uint16_t *src = fd->pixels;

    // Center sprite around (cx, cy)
    int ox = cx - (w / 2);
    int oy = cy - (h / 2);

    for (int y = 0; y < h; y++) {
        int dst_y = oy + y;
        if (dst_y < 0 || dst_y >= SCREEN_H) continue;

        for (int x = 0; x < w; x++) {
            int draw_x;
            if (dir == 2) {
                // West (-X): flip horizontally
                draw_x = ox + (w - 1 - x);
            } else {
                // East (+X, default)
                draw_x = ox + x;
            }

            if (draw_x < 0 || draw_x >= SCREEN_W) continue;

            uint16_t col = src[y * w + x];
            if (col & 0x8000) {
                g_backbuffer[dst_y * SCREEN_W + draw_x] = col;
            }
        }
    }
}
''')

print("Generated enemy_data.h and enemy_data.c successfully!")
