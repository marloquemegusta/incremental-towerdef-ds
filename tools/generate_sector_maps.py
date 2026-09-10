#!/usr/bin/env python3
"""
generate_sector1_maps_v7.py (Alineación Estricta a Retícula de Suelo)
1. BORDES Y CORNISAS ALINEADOS A LA RETÍCULA (0 y 31):
   - El bordillo se sitúa exactamente en la frontera de los tiles (coordenada 0 y 31 del bloque 32x32).
   - Ningún tile de suelo es recortado ni invadido por franjas arbitrarias de hormigón.
   - En los codos interiores, los bordillos se encuentran en los vértices exactos de la rejilla (0, 31), (31, 0), etc.
   - La retícula vertical y horizontal de las baldosas queda 100% limpia y continua.
2. TRIDIMENSIONALIDAD PROFUNDA:
   - Sombra arrojada de 4 px bajo el labio del bordillo.
   - Escalinatas de 3 peldaños (curb_stairs) bajando a la carretera.
   - Sumideros de tormenta (curb_drain) y tubos colectores (curb_pipe).
   - Parapetos de sacos terreros (sandbags).
   - Variedad de suelo con hierba en juntas, grietas y manchas sutiles, respetando la retícula.
3. MAPAS AMPLIADOS:
   - Mapa 1-1: La Trinchera con Chicana (~380 px).
   - Mapa 1-2: La Doble S Retorcida (~650 px).
   - Mapa 1-3: La Rotonda del Sanctum con Isla Central 360°.
"""

import os
import math
import random
from PIL import Image, ImageDraw

BRAIN_DIR = r"C:\Users\malfonso\.gemini\antigravity\brain\5afe97fc-2267-40e1-bf34-7201f418bd2d"
TURRETS_DIR = r"c:\codexlocal\towerds\assets\sprites\turrets"
ENEMIES_DIR = r"c:\codexlocal\towerds\assets\sprites\enemies"

# Colores BGR555 estricto
C_BLACK = (10, 11, 14, 255)
C_SHADOW_MID = (18, 20, 26, 255)
C_SHADOW_SOFT = (28, 30, 38, 255)

C_ASP_BASE = (38, 41, 48, 255)
C_ASP_DARK = (28, 30, 36, 255)
C_ASP_LIGHT = (48, 52, 60, 255)
C_ASP_CRACK = (18, 20, 25, 255)
C_ROAD_LINE = (235, 185, 25, 255)

C_CONC_BASE = (65, 70, 82, 255)
C_CONC_LIGHT = (82, 88, 102, 255)
C_CONC_DARK = (45, 48, 58, 255)
C_CONC_BEVEL = (110, 118, 135, 255)
C_CURB_TOP = (135, 142, 160, 255)
C_JOINT = (35, 38, 46, 255)

# Hierba y vegetación
C_GRASS_DEEP = (38, 62, 32, 255)
C_GRASS_MID = (65, 98, 48, 255)
C_GRASS_TALL = (95, 138, 62, 255)

# Manchas de aceite
C_OIL_DARK = (22, 20, 24, 255)
C_OIL_MID = (35, 32, 38, 255)

# Sacos terreros
C_BAG_DARK = (58, 48, 32, 255)
C_BAG_MID = (115, 96, 64, 255)
C_BAG_HI = (165, 142, 98, 255)

def hash_noise(x, y, seed=1337):
    n = (x * 374761393 + y * 668265263 + seed) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    return (n ^ (n >> 16)) & 0xFF

# ------------------------------------------------------------------------------
# SUELO CON RETÍCULA GLOBAL EXACTA (Grandes baldosas 16x16 px nativas DS)
# ------------------------------------------------------------------------------
def draw_ground_pixel(gx, gy, seed=77):
    rx = gx % 16
    ry = gy % 16
    if rx == 15 or ry == 15:
        return C_JOINT
    elif rx == 0 or ry == 0:
        return C_CONC_BEVEL
    else:
        v = hash_noise(gx, gy, seed)
        return C_CONC_BASE if v < 210 else (C_CONC_LIGHT if v > 240 else C_CONC_DARK)

def make_ground_tile(col, row, seed=101):
    im = Image.new("RGBA", (16, 16), C_CONC_BASE)
    p = im.putpixel
    x0 = col * 16
    y0 = row * 16
    for y in range(16):
        for x in range(16):
            p((x, y), draw_ground_pixel(x0 + x, y0 + y, seed))
            
    # Variedad sutil sobre la baldosa grande de 16x16
    t_hash = hash_noise(col, row, 999)
    if t_hash > 215: # Hierba en juntas perimetrales (15%)
        p((15, 7), C_GRASS_MID); p((15, 6), C_GRASS_TALL); p((15, 8), C_GRASS_DEEP)
        p((14, 7), C_GRASS_MID); p((15, 5), C_GRASS_DEEP)
        p((7, 15), C_GRASS_MID); p((6, 15), C_GRASS_TALL); p((8, 15), C_GRASS_DEEP)
    elif t_hash < 25: # Grieta estructural fina (10%)
        crack = [(2,3),(3,4),(4,5),(5,5),(6,6),(7,6),(8,7),(9,8),(10,9),(11,10),(12,11)]
        for cx, cy in crack:
            p((cx, cy), C_ASP_CRACK)
            if cx + 1 < 15: p((cx + 1, cy), C_CONC_DARK)
    elif 50 < t_hash < 62: # Mancha sutil de aceite/grasa (4%)
        for dy in range(4):
            for dx in range(4):
                d = (dx - 1.5)**2 + (dy - 1.5)**2
                if d <= 3.5: p((6 + dx, 6 + dy), C_OIL_MID)
        p((7, 7), C_OIL_DARK); p((8, 7), C_OIL_DARK)
        
    return im

def make_sandbags():
    im = Image.new("RGBA", (16, 16), C_CONC_BASE)
    p = im.putpixel
    for y in range(16):
        for x in range(16):
            p((x, y), draw_ground_pixel(x, y, 808))
    for y in range(11, 14):
        for x in range(16): p((x, y), C_BLACK)
    for seg in range(3):
        x0 = seg * 5
        for sx in range(x0, min(x0 + 5, 16)):
            p((sx, 8), C_BAG_HI); p((sx, 9), C_BAG_MID); p((sx, 10), C_BAG_DARK)
        p((x0, 9), C_BLACK)
    for seg in range(3):
        x0 = seg * 5 + 2
        for sx in range(x0, min(x0 + 5, 16)):
            p((sx, 5), C_BAG_HI); p((sx, 6), C_BAG_MID); p((sx, 7), C_BAG_DARK)
    return im

# ------------------------------------------------------------------------------
# BLOQUES DE CARRETERA 32x32 ALINEADOS A LA RETÍCULA (Bordillos en 0 y 31)
# ------------------------------------------------------------------------------

def make_block_h_aligned(has_dash=True, feat=None):
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    # Bordillo norte y sombra (y=0..4)
    for x in range(32):
        p((x, 0), C_CURB_TOP)
        p((x, 1), C_CONC_DARK)
        p((x, 2), C_BLACK)
        p((x, 3), C_SHADOW_MID)
        p((x, 4), C_SHADOW_SOFT)
    # Asfalto (y=5..29)
    for y in range(5, 30):
        for x in range(32):
            v = hash_noise(x, y, 2)
            p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
    if has_dash:
        for x in range(4, 28):
            if (x // 8) % 2 == 0:
                p((x, 15), C_ROAD_LINE); p((x, 16), C_ROAD_LINE)
    # Bordillo sur (y=30..31)
    for x in range(32):
        p((x, 30), C_CONC_DARK)
        p((x, 31), C_CURB_TOP)
        
    if feat == "stairs":
        # Escalera en bordillo norte (x: 10..22, y: 0..5)
        for y in range(0, 6):
            p((9, y), C_CONC_DARK); p((22, y), C_CONC_DARK)
            p((8, y), C_CONC_LIGHT); p((23, y), C_CONC_LIGHT)
        for x in range(10, 22):
            p((x, 0), C_CONC_BEVEL); p((x, 1), C_CONC_LIGHT)
            p((x, 2), C_CONC_BEVEL); p((x, 3), C_CONC_LIGHT)
            p((x, 4), C_CONC_BEVEL); p((x, 5), C_CONC_LIGHT)
    elif feat == "drain":
        for x in range(12, 20):
            p((x, 0), C_CONC_BEVEL); p((x, 1), C_BLACK); p((x, 2), C_BLACK)
        for x in range(13, 19, 2): p((x, 1), (80, 85, 95, 255))
    elif feat == "pipe":
        for x in range(13, 19):
            p((x, 0), C_CONC_DARK); p((x, 1), C_BLACK)
        p((15, 2), (20, 50, 60, 255)); p((16, 2), (20, 50, 60, 255))
    return im

def make_block_v_aligned(has_dash=True):
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    # Bordillo oeste y sombra (x=0..4)
    for y in range(32):
        p((0, y), C_CURB_TOP)
        p((1, y), C_CONC_DARK)
        p((2, y), C_BLACK)
        p((3, y), C_SHADOW_MID)
        p((4, y), C_SHADOW_SOFT)
    # Asfalto (x=5..29)
    for x in range(5, 30):
        for y in range(32):
            v = hash_noise(x, y, 12)
            p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
    if has_dash:
        for y in range(4, 28):
            if (y // 8) % 2 == 0:
                p((15, y), C_ROAD_LINE); p((16, y), C_ROAD_LINE)
    # Bordillo este (x=30..31)
    for y in range(32):
        p((30, y), C_CONC_DARK)
        p((31, y), C_CURB_TOP)
    return im

def make_turn_ws_aligned(gx0, gy0):
    """
    Giro Oeste a Sur:
    - Inner corner BL en (0, 31) exacto.
    - Outer corner TR: arco R=31 desde (0,0) hasta (31,31) centrado en (0, 31).
    - Acera exterior TR continúa la retícula global de losas.
    """
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    cx, cy = 0, 31
    r_curb = 31.0
    for y in range(32):
        for x in range(32):
            dist = math.sqrt((x - cx)**2 + (y - cy)**2)
            if dist > r_curb:
                p((x, y), draw_ground_pixel(gx0 + x, gy0 + y))
            else:
                v = hash_noise(x, y, 23)
                p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
                
    for deg in range(0, 91):
        rad = math.radians(deg)
        bx = int(round(cx + r_curb * math.cos(rad)))
        by = int(round(cy - r_curb * math.sin(rad)))
        if 0 <= bx < 32 and 0 <= by < 32: p((bx, by), C_CURB_TOP)
        fx = int(round(cx + 30.0 * math.cos(rad)))
        fy = int(round(cy - 30.0 * math.sin(rad)))
        if 0 <= fx < 32 and 0 <= fy < 32: p((fx, fy), C_CONC_DARK)
        sx1 = int(round(cx + 29.0 * math.cos(rad)))
        sy1 = int(round(cy - 29.0 * math.sin(rad)))
        if 0 <= sx1 < 32 and 0 <= sy1 < 32: p((sx1, sy1), C_BLACK)
        sx2 = int(round(cx + 28.0 * math.cos(rad)))
        sy2 = int(round(cy - 28.0 * math.sin(rad)))
        if 0 <= sx2 < 32 and 0 <= sy2 < 32: p((sx2, sy2), C_SHADOW_MID)
        
    p((0, 30), C_CONC_DARK); p((0, 31), C_CURB_TOP)
    p((0, 31), C_CURB_TOP); p((1, 31), C_CONC_DARK); p((2, 31), C_BLACK)
    return im

def make_turn_se_aligned(gx0, gy0):
    """
    Giro Norte a Este:
    - Inner corner TR en (31, 0) exacto.
    - Outer corner BL: arco R=31 centrado en (31, 0).
    - Acera exterior BL continúa la retícula global de losas.
    """
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    cx, cy = 31, 0
    r_curb = 31.0
    for y in range(32):
        for x in range(32):
            dist = math.sqrt((x - cx)**2 + (y - cy)**2)
            if dist > r_curb:
                p((x, y), draw_ground_pixel(gx0 + x, gy0 + y))
            else:
                v = hash_noise(x, y, 33)
                p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
                
    for deg in range(90, 181):
        rad = math.radians(deg)
        bx = int(round(cx - r_curb * math.sin(rad - math.pi/2)))
        by = int(round(cy + r_curb * math.cos(rad - math.pi/2)))
        if 0 <= bx < 32 and 0 <= by < 32: p((bx, by), C_CURB_TOP)
        fx = int(round(cx - 30.0 * math.sin(rad - math.pi/2)))
        fy = int(round(cy + 30.0 * math.cos(rad - math.pi/2)))
        if 0 <= fx < 32 and 0 <= fy < 32: p((fx, fy), C_CONC_DARK)
        
    p((31, 0), C_CURB_TOP); p((31, 1), C_CONC_DARK); p((31, 2), C_BLACK)
    p((30, 0), C_CONC_DARK); p((31, 0), C_CURB_TOP)
    return im

def make_turn_sw_aligned(gx0, gy0):
    """
    Giro Norte a Oeste:
    - Inner corner TL en (0, 0) exacto.
    - Outer corner BR: arco R=31 centrado en (0, 0).
    - Acera exterior BR continúa la retícula global de losas.
    """
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    cx, cy = 0, 0
    r_curb = 31.0
    for y in range(32):
        for x in range(32):
            dist = math.sqrt((x - cx)**2 + (y - cy)**2)
            if dist > r_curb:
                p((x, y), draw_ground_pixel(gx0 + x, gy0 + y))
            else:
                v = hash_noise(x, y, 43)
                p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
                
    for deg in range(0, 91):
        rad = math.radians(deg)
        bx = int(round(cx + r_curb * math.cos(rad)))
        by = int(round(cy + r_curb * math.sin(rad)))
        if 0 <= bx < 32 and 0 <= by < 32: p((bx, by), C_CURB_TOP)
        fx = int(round(cx + 30.0 * math.cos(rad)))
        fy = int(round(cy + 30.0 * math.sin(rad)))
        if 0 <= fx < 32 and 0 <= fy < 32: p((fx, fy), C_CONC_DARK)
        
    p((0, 0), C_CURB_TOP); p((1, 0), C_CONC_DARK); p((2, 0), C_BLACK)
    p((0, 0), C_CURB_TOP); p((0, 1), C_CONC_DARK); p((0, 2), C_BLACK)
    return im

def make_turn_es_aligned(gx0, gy0):
    """
    Giro Este a Sur:
    - Inner corner BR en (31, 31) exacto.
    - Outer corner TL: arco R=31 centrado en (31, 31).
    - Acera exterior TL continúa la retícula global de losas.
    """
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    cx, cy = 31, 31
    r_curb = 31.0
    for y in range(32):
        for x in range(32):
            dist = math.sqrt((x - cx)**2 + (y - cy)**2)
            if dist > r_curb:
                p((x, y), draw_ground_pixel(gx0 + x, gy0 + y))
            else:
                v = hash_noise(x, y, 53)
                p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
                
    for deg in range(0, 91):
        rad = math.radians(deg)
        bx = int(round(cx - r_curb * math.cos(rad)))
        by = int(round(cy - r_curb * math.sin(rad)))
        if 0 <= bx < 32 and 0 <= by < 32: p((bx, by), C_CURB_TOP)
        fx = int(round(cx - 30.0 * math.cos(rad)))
        fy = int(round(cy - 30.0 * math.sin(rad)))
        if 0 <= fx < 32 and 0 <= fy < 32: p((fx, fy), C_CONC_DARK)
        sx1 = int(round(cx - 29.0 * math.cos(rad)))
        sy1 = int(round(cy - 29.0 * math.sin(rad)))
        if 0 <= sx1 < 32 and 0 <= sy1 < 32: p((sx1, sy1), C_BLACK)
        sx2 = int(round(cx - 28.0 * math.cos(rad)))
        sy2 = int(round(cy - 28.0 * math.sin(rad)))
        if 0 <= sx2 < 32 and 0 <= sy2 < 32: p((sx2, sy2), C_SHADOW_MID)

    p((30, 31), C_CONC_DARK); p((31, 31), C_CURB_TOP)
    p((31, 30), C_CONC_DARK); p((31, 31), C_CURB_TOP)
    return im

def make_t_junction_south_aligned():
    im = Image.new("RGBA", (32, 32), C_ASP_BASE)
    p = im.putpixel
    for y in range(32):
        for x in range(32):
            v = hash_noise(x, y, 62)
            p((x, y), C_ASP_BASE if v < 210 else (C_ASP_DARK if v > 240 else C_ASP_LIGHT))
    # Esquina interior NW en (0, 0)
    p((0, 0), C_CURB_TOP); p((1, 0), C_CONC_DARK); p((2, 0), C_BLACK)
    p((0, 0), C_CURB_TOP); p((0, 1), C_CONC_DARK); p((0, 2), C_BLACK)
    # Esquina interior NE en (31, 0)
    p((31, 0), C_CURB_TOP); p((30, 0), C_CONC_DARK)
    p((31, 0), C_CURB_TOP); p((31, 1), C_CONC_DARK); p((31, 2), C_BLACK)
    # Bordillo sur continuo en y=30..31
    for x in range(32): p((x, 30), C_CONC_DARK); p((x, 31), C_CURB_TOP)
    return im

def slice_2x2(im):
    return [
        [im.crop((0, 0, 16, 16)), im.crop((16, 0, 32, 16))],
        [im.crop((0, 16, 16, 32)), im.crop((16, 16, 32, 32))]
    ]

def place_2x2(grid, r, c, im_32):
    tiles = slice_2x2(im_32)
    for dr in range(2):
        for dc in range(2):
            grid[r + dr][c + dc] = tiles[dr][dc]

# ------------------------------------------------------------------------------
# CONSTRUCCIÓN DE LOS 3 MAPAS
# ------------------------------------------------------------------------------

def build_map_1_1(rand_gen):
    grid = [[make_ground_tile(c, r, seed=100 + r*16 + c) for c in range(16)] for r in range(12)]
    
    # Tramo 1 Oeste: Filas 4 y 5 (Cols 0..5)
    place_2x2(grid, 4, 0, make_block_h_aligned(feat="stairs"))
    place_2x2(grid, 4, 2, make_block_h_aligned(feat=None))
    place_2x2(grid, 4, 4, make_block_h_aligned(feat=None))
    
    # Chicana: Cols 6..7 baja de fila 4 a fila 6
    place_2x2(grid, 4, 6, make_turn_ws_aligned(6 * 16, 4 * 16))
    place_2x2(grid, 6, 6, make_turn_se_aligned(6 * 16, 6 * 16))
    
    # Tramo 2 Este: Filas 6 y 7 (Cols 8..15)
    place_2x2(grid, 6, 8, make_block_h_aligned(feat="drain"))
    place_2x2(grid, 6, 10, make_block_h_aligned(feat=None))
    place_2x2(grid, 6, 12, make_block_h_aligned(feat="pipe"))
    place_2x2(grid, 6, 14, make_block_h_aligned(feat=None))
    
    # Parapetos militares tácticos
    grid[3][3] = make_sandbags()
    grid[8][11] = make_sandbags()
    return grid

def build_map_1_2(rand_gen):
    grid = [[make_ground_tile(c, r, seed=200 + r*16 + c) for c in range(16)] for r in range(12)]
    
    # Fila 1 superior: Filas 2 y 3 (Cols 0..11)
    place_2x2(grid, 2, 0, make_block_h_aligned(feat=None))
    place_2x2(grid, 2, 2, make_block_h_aligned(feat="stairs"))
    place_2x2(grid, 2, 4, make_block_h_aligned(feat=None))
    place_2x2(grid, 2, 6, make_block_h_aligned(feat=None))
    place_2x2(grid, 2, 8, make_block_h_aligned(feat="drain"))
    place_2x2(grid, 2, 10, make_block_h_aligned(feat=None))
    
    # Giro 1: Oeste a Sur en Cols 12..13
    place_2x2(grid, 2, 12, make_turn_ws_aligned(12 * 16, 2 * 16))
    # Bajada 1: Filas 4..5, Cols 12..13
    place_2x2(grid, 4, 12, make_block_v_aligned())
    # Giro 2: Norte a Oeste en Cols 12..13, Filas 6..7
    place_2x2(grid, 6, 12, make_turn_sw_aligned(12 * 16, 6 * 16))
    
    # Fila 2 media hacia Oeste: Filas 6 y 7 (Cols 10..4)
    place_2x2(grid, 6, 10, make_block_h_aligned(feat=None))
    place_2x2(grid, 6, 8, make_block_h_aligned(feat="pipe"))
    place_2x2(grid, 6, 6, make_block_h_aligned(feat=None))
    place_2x2(grid, 6, 4, make_block_h_aligned(feat=None))
    
    # Giro 3: Este a Sur en Cols 2..3, Filas 6..7
    place_2x2(grid, 6, 2, make_turn_es_aligned(2 * 16, 6 * 16))
    # Bajada 2: Filas 8..9, Cols 2..3
    place_2x2(grid, 8, 2, make_block_v_aligned())
    # Giro 4: Norte a Este en Cols 2..3, Filas 10..11
    place_2x2(grid, 10, 2, make_turn_se_aligned(2 * 16, 10 * 16))
    
    # Fila 3 inferior hacia Este: Filas 10 y 11 (Cols 4..15)
    place_2x2(grid, 10, 4, make_block_h_aligned(feat=None))
    place_2x2(grid, 10, 6, make_block_h_aligned(feat="stairs"))
    place_2x2(grid, 10, 8, make_block_h_aligned(feat=None))
    place_2x2(grid, 10, 10, make_block_h_aligned(feat="drain"))
    place_2x2(grid, 10, 12, make_block_h_aligned(feat=None))
    place_2x2(grid, 10, 14, make_block_h_aligned(feat=None))
    
    # Sacos terreros en recodos
    grid[1][7] = make_sandbags()
    grid[5][8] = make_sandbags()
    grid[9][7] = make_sandbags()
    return grid

def build_map_1_3(rand_gen):
    grid = [[make_ground_tile(c, r, seed=300 + r*16 + c) for c in range(16)] for r in range(12)]
    
    # Entrada Norte: Cols 4..5 (Filas 0..3)
    place_2x2(grid, 0, 4, make_block_v_aligned())
    place_2x2(grid, 2, 4, make_block_v_aligned())
    
    # Giro Norte a Este en Cols 4..5, Filas 4..5
    place_2x2(grid, 4, 4, make_turn_se_aligned(4 * 16, 4 * 16))
    
    # Anillo Superior (Filas 4 y 5, Cols 6..11)
    place_2x2(grid, 4, 6, make_block_h_aligned(feat=None))
    place_2x2(grid, 4, 8, make_block_h_aligned(feat="stairs"))
    place_2x2(grid, 4, 10, make_block_h_aligned(feat=None))
    
    # Confluencia Este: Anillo superior gira al Sur en Cols 12..13, Filas 4..5
    place_2x2(grid, 4, 12, make_turn_ws_aligned(12 * 16, 4 * 16))
    # Bajada este: Filas 6..7, Cols 12..13
    place_2x2(grid, 6, 12, make_block_v_aligned())
    
    # Anillo Inferior / Entrada Oeste (Filas 8 y 9, Cols 0..11)
    place_2x2(grid, 8, 0, make_block_h_aligned(feat=None))
    place_2x2(grid, 8, 2, make_block_h_aligned(feat="drain"))
    place_2x2(grid, 8, 4, make_block_h_aligned(feat=None))
    place_2x2(grid, 8, 6, make_block_h_aligned(feat=None))
    place_2x2(grid, 8, 8, make_block_h_aligned(feat="pipe"))
    place_2x2(grid, 8, 10, make_block_h_aligned(feat=None))
    
    # Confluencia en T al Este (Filas 8 y 9, Cols 12..13)
    place_2x2(grid, 8, 12, make_t_junction_south_aligned())
    
    # Salida Este hacia compuerta (Filas 8 y 9, Cols 14..15)
    place_2x2(grid, 8, 14, make_block_h_aligned(feat=None))
    
    # Isla Central Fortificada (Filas 6..7, Cols 6..11)
    grid[6][6] = make_sandbags()
    grid[6][9] = make_sandbags()
    grid[7][7] = make_sandbags()
    grid[7][10] = make_sandbags()
    return grid

# ------------------------------------------------------------------------------
# RENDERIZADO Y GENERACIÓN DE IMÁGENES
# ------------------------------------------------------------------------------
def render_grid(grid):
    screen = Image.new("RGBA", (256, 192), C_CONC_BASE)
    for r in range(12):
        for c in range(16): screen.paste(grid[r][c], (c * 16, r * 16))
    return screen

bolter_strip = Image.open(os.path.join(TURRETS_DIR, "heavy_bolter_strip_master_1x.png")).convert("RGBA")
bolter_frame_32 = bolter_strip.crop((0, 0, 32, 32))

lascannon_strip = Image.open(os.path.join(TURRETS_DIR, "lascannon_strip_master_1x.png")).convert("RGBA")
lascannon_frame_32 = lascannon_strip.crop((0, 0, 32, 32))

larva_strip = Image.open(os.path.join(ENEMIES_DIR, "t0_larva_strip_master_1x.png")).convert("RGBA")
larva_f0 = larva_strip.crop((0, 0, 4, 4))
larva_f1 = larva_strip.crop((4, 0, 8, 4))

ripper_strip = Image.open(os.path.join(ENEMIES_DIR, "t1_ripper_strip_master_1x.png")).convert("RGBA")
ripper_f0 = ripper_strip.crop((0, 0, 6, 5))
ripper_f1 = ripper_strip.crop((6, 0, 12, 5))

gaunt_strip = Image.open(os.path.join(ENEMIES_DIR, "t2_hormagaunt_strip_master_1x.png")).convert("RGBA")
gaunt_f0 = gaunt_strip.crop((0, 0, 9, 9))
gaunt_f1 = gaunt_strip.crop((9, 0, 18, 9))

ravener_strip = Image.open(os.path.join(ENEMIES_DIR, "t3_ravener_strip_master_1x.png")).convert("RGBA")
ravener_f0 = ravener_strip.crop((0, 0, 15, 11))

carnifex_strip = Image.open(os.path.join(ENEMIES_DIR, "t4_carnifex_strip_master_1x.png")).convert("RGBA")
carnifex_f0 = carnifex_strip.crop((0, 0, 21, 21))

def orient_enemy(sprite, direction):
    if direction == 0: return sprite
    elif direction == 1: return sprite.rotate(-90, expand=True)
    elif direction == 2: return sprite.transpose(Image.FLIP_LEFT_RIGHT)
    elif direction == 3: return sprite.rotate(90, expand=True)
    return sprite

def draw_cone(img, cx, cy, angle_deg, spread_deg, length):
    draw = ImageDraw.Draw(img)
    rad_l = math.radians(angle_deg - spread_deg / 2)
    rad_r = math.radians(angle_deg + spread_deg / 2)
    lx = int(cx + math.cos(rad_l) * length)
    ly = int(cy + math.sin(rad_l) * length)
    rx = int(cx + math.cos(rad_r) * length)
    ry = int(cy + math.sin(rad_r) * length)
    draw.line([(cx, cy), (lx, ly)], fill=(235, 185, 25, 220), width=1)
    draw.line([(cx, cy), (rx, ry)], fill=(235, 185, 25, 220), width=1)
    bbox = [cx - length, cy - length, cx + length, cy + length]
    draw.arc(bbox, start=angle_deg - spread_deg/2, end=angle_deg + spread_deg/2, fill=(235, 185, 25, 160), width=1)

def draw_hud(img, title, wave):
    draw = ImageDraw.Draw(img)
    draw.rectangle([0, 0, 256, 11], fill=(12, 14, 18, 255), outline=(35, 40, 50, 255))
    draw.text((4, 1), title, fill=(245, 195, 35, 255))
    draw.text((125, 1), wave, fill=(90, 220, 120, 255))
    draw.text((215, 1), "$ 140", fill=(240, 240, 240, 255))

rng = random.Random(777)

maps_data = [
    # MAPA 1-1: LA TRINCHERA CON CHICANA SUAVE EXTENDIDA
    ("map_1_1_straight", "M1: LA TRINCHERA", "OLA 1/3 (CHICANA)", build_map_1_1(rng), [
        ("bolter", 60, 32, 70, 40),
        ("bolter", 150, 145, 180, 35),
    ], [
        (larva_f0, 20, 80, 0), (larva_f1, 40, 78, 0), (ripper_f0, 65, 80, 0),
        (ripper_f1, 105, 90, 1), (gaunt_f0, 110, 105, 1),
        (larva_f0, 145, 112, 0), (larva_f1, 175, 110, 0), (larva_f0, 205, 114, 0)
    ]),
    
    # MAPA 1-2: LA DOBLE S RETORCIDA
    ("map_1_2_s_curve", "M2: DOBLE S", "OLA 2/4 (ZIG-ZAG)", build_map_1_2(rng), [
        ("bolter", 115, 75, 180, 35),
        ("lascannon", 110, 170, 345, 25),
    ], [
        (ripper_f0, 30, 48, 0), (gaunt_f0, 80, 48, 0),
        (gaunt_f1, 200, 75, 1), (ravener_f0, 150, 112, 2), (ripper_f1, 100, 112, 2),
        (gaunt_f0, 48, 140, 1), (larva_f0, 110, 176, 0), (larva_f1, 150, 176, 0)
    ]),
    
    # MAPA 1-3: ROTONDA DEL SANCTUM CON ISLA CENTRAL
    ("map_1_3_crossroads", "M3: ROTONDA SANCTUM", "OLA 5/5 (ISLA 360)", build_map_1_3(rng), [
        ("bolter", 120, 112, 280, 40),
        ("bolter", 144, 112, 60, 40),
    ], [
        (larva_f0, 72, 25, 1), (ripper_f0, 72, 45, 1),
        (gaunt_f0, 110, 80, 0), (ravener_f0, 145, 80, 0),
        (carnifex_f0, 50, 144, 0), (gaunt_f1, 110, 144, 0),
        (larva_f1, 205, 110, 1), (ripper_f1, 235, 144, 0)
    ])
]

for base_name, title, wave_txt, grid, turrets, enemies in maps_data:
    clean_img = render_grid(grid)
    clean_path_1x = os.path.join(BRAIN_DIR, f"{base_name}_v7_clean_1x.png")
    clean_path_3x = os.path.join(BRAIN_DIR, f"{base_name}_v7_clean_3x.png")
    clean_img.save(clean_path_1x)
    clean_img.resize((768, 576), Image.Resampling.NEAREST).save(clean_path_3x)
    
    action_img = clean_img.copy()
    
    # Conos de apuntado
    for t_type, tx, ty, angle, spread in turrets:
        draw_cone(action_img, tx, ty, angle, spread, length=65)
        
    # Enemigos
    for sp, ex, ey, d in enemies:
        oriented = orient_enemy(sp, d)
        ew, eh = oriented.size
        action_img.paste(oriented, (ex - ew // 2, ey - eh // 2), oriented)
        
    # Torretas
    for t_type, tx, ty, angle, spread in turrets:
        strip = bolter_frame_32 if t_type == "bolter" else lascannon_frame_32
        rot_sprite = strip.rotate(-angle, resample=Image.Resampling.NEAREST)
        action_img.paste(rot_sprite, (tx - 16, ty - 16), rot_sprite)
        
    draw_hud(action_img, title, wave_txt)
    
    action_path_1x = os.path.join(BRAIN_DIR, f"{base_name}_v7_action_1x.png")
    action_path_3x = os.path.join(BRAIN_DIR, f"{base_name}_v7_action_3x.png")
    action_img.save(action_path_1x)
    action_img.resize((768, 576), Image.Resampling.NEAREST).save(action_path_3x)
    print(f"[OK] Generado {base_name} con alineacion estricta a reticula!")

print("Proceso completado exitosamente!")
