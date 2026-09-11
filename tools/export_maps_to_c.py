import sys
sys.path.append('tools')
import generate_sector_maps as gsm
import random

def to_bgr555(r, g, b, a=255):
    return ((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10) | 0x8000

rng = random.Random(777)
maps = [
    ('g_map_1_1_bg', gsm.render_grid(gsm.build_map_1_1(rng))),
    ('g_map_1_2_bg', gsm.render_grid(gsm.build_map_1_2(rng))),
    ('g_map_1_3_bg', gsm.render_grid(gsm.build_map_1_3(rng)))
]

print('Generating source/sector1_data.c...')
with open('source/sector1_data.c', 'w', encoding='utf-8') as f:
    f.write('// Auto-generated Sector 1 High-Fidelity Maps for Nintendo DS\n')
    f.write('#include game.h\n#include tiles.h\n\n')
    
    for name, img in maps:
        f.write(f'const uint16_t {name}[SCREEN_W * SCREEN_H] __attribute__((aligned(4))) = {{\n')
        pixels = list(img.getdata())
        for row in range(192):
            row_vals = [f'0x{to_bgr555(*pixels[row * 256 + col]):04X}' for col in range(256)]
            comma = ',' if row < 191 else ''
            f.write('  ' + ', '.join(row_vals) + comma + '\n')
        f.write('};\n\n')

    f.write('''static const Waypoint s_map_1_1_wp[4] = {
    {0, 80}, {96, 80}, {112, 112}, {256, 112}
};

static const Waypoint s_map_1_2_wp[6] = {
    {0, 48}, {208, 48}, {208, 112}, {48, 112}, {48, 176}, {256, 176}
};

static const Waypoint s_map_1_3_wp[5] = {
    {72, 0}, {72, 80}, {208, 80}, {208, 144}, {256, 144}
};

const Waypoint *g_map_waypoints[3] = {
    s_map_1_1_wp, s_map_1_2_wp, s_map_1_3_wp
};

const int g_map_waypoint_counts[3] = { 4, 6, 5 };

const uint16_t *g_map_backgrounds[3] = {
    g_map_1_1_bg, g_map_1_2_bg, g_map_1_3_bg
};
''')

print('Done writing source/sector1_data.c')
