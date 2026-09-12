"""
Script de generación del tileset 32x32 de Tierra Árida / Yermo Balístico para Nintendo DS.
Cumple estrictamente con:
1. Regla Canónica de Geometría de Carreteras y Desnivel 3D (sombra arrojada 3-4px, modulación 32px).
2. Regla Canónica de Exclusividad Cromática Xenos (cero púrpuras, cero blanco hueso).
3. Salida de cada tile en 1x (32x32) y 8x (256x256).
4. Hoja de catálogo global y mockup escénico DS (256x192) usando assets reales canónicos.
"""

import os
import math
import random
from PIL import Image, ImageDraw

OUTPUT_DIR = "assets/tiles/wasteland"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Paleta canónica BGR555 aproximada para Yermo Balístico
# Sin púrpuras (R y B altos con G bajo) ni blanco hueso (RGB > 240)
PALETTE = {
    # Cota alta / Suelo árido
    "sand_hi": (194, 164, 120),    # #C2A478
    "sand_mid": (169, 136, 88),    # #A98858
    "sand_low": (142, 109, 62),    # #8E6D3E
    "clay_hi": (117, 83, 46),      # #75532E
    "clay_mid": (92, 62, 32),      # #5C3E20
    "clay_dark": (70, 44, 22),     # #462C16
    
    # Sombras profundas y grietas
    "shadow_deep": (44, 26, 13),   # #2C1A0D
    "shadow_abyss": (25, 15, 8),   # #190F08
    "void": (11, 6, 3),            # #0B0603

    # Rocas y grava basáltica
    "rock_hi": (102, 97, 88),      # #666158
    "rock_mid": (75, 70, 62),      # #4B463E
    "rock_dark": (50, 46, 40),     # #322E28
    
    # Metal oxidado / Mechanicus
    "rust_hi": (138, 74, 40),      # #8A4A28
    "rust_dark": (94, 47, 22),     # #5E2F16
    "steel_hi": (110, 115, 120),   # #6E7378
    "steel_mid": (75, 80, 85),     # #4B5055
    "steel_dark": (45, 48, 52),    # #2D3034

    # Caliche / polvo mineral seco (resaltes seguros)
    "dust_crust": (212, 191, 148), # #D4BF94
}

def verify_color(color):
    """Audita que ningún color vulnere la exclusividad xenos."""
    r, g, b = color[:3]
    if r >= 105 and b >= 140 and (b > g + 25):
        raise ValueError(f"Color prohibido xenos detectado: {color}")
    if r > 230 and g > 230 and b > 230:
        raise ValueError(f"Blanco excesivo detectado: {color}")
    return color

for k, c in PALETTE.items():
    verify_color(c)

def create_base_ground(rng_seed=42):
    """Crea una textura de base de 32x32 de tierra árida con enlosado orgánico y seamless."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(rng_seed)
    
    for y in range(32):
        for x in range(32):
            nx = (x / 32.0) * 2 * math.pi
            ny = (y / 32.0) * 2 * math.pi
            val = (math.sin(nx * 2) * math.cos(ny * 2) + 
                   math.sin(nx * 4 + 1.2) * math.cos(ny * 4 + 0.8) * 0.5 +
                   rng.uniform(-0.35, 0.35))
            
            if val > 0.45:
                pixels[x, y] = PALETTE["sand_hi"]
            elif val > 0.1:
                pixels[x, y] = PALETTE["sand_mid"]
            elif val > -0.3:
                pixels[x, y] = PALETTE["sand_low"]
            else:
                pixels[x, y] = PALETTE["clay_hi"]
                
    for _ in range(12):
        rx, ry = rng.randint(0, 31), rng.randint(0, 31)
        pixels[rx, ry] = PALETTE["dust_crust"]
        if rx < 31:
            pixels[rx + 1, ry] = PALETTE["sand_hi"]
            
    for _ in range(8):
        rx, ry = rng.randint(0, 31), rng.randint(0, 31)
        pixels[rx, ry] = PALETTE["clay_dark"]
        if ry < 31:
            pixels[rx, ry + 1] = PALETTE["shadow_deep"]

    return img

def tile_t00_wasteland_plain():
    """T00: Tierra árida lisa continua (seamless)."""
    return create_base_ground(101)

def tile_t01_wasteland_cracked():
    """T01: Tierra árida con grietas superficiales por desecación térmica."""
    img = create_base_ground(102)
    pixels = img.load()
    
    crack_paths = [
        [(4, 8), (7, 10), (12, 11), (15, 14), (17, 19), (21, 23), (26, 25)],
        [(15, 14), (19, 12), (24, 10), (28, 11)],
        [(17, 19), (14, 23), (12, 28)],
        [(6, 20), (9, 22), (12, 23)]
    ]
    
    for path in crack_paths:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                cx = int(x0 + (x1 - x0) * t)
                cy = int(y0 + (y1 - y0) * t)
                if 0 <= cx < 32 and 0 <= cy < 32:
                    pixels[cx, cy] = PALETTE["shadow_deep"]
                    if cy > 0 and (cx, cy - 1) not in path:
                        pixels[cx, cy - 1] = PALETTE["dust_crust"]
                    if cx > 0 and (cx - 1, cy) not in path:
                        pixels[cx - 1, cy] = PALETTE["sand_hi"]
    return img

def tile_t02_wasteland_fissure_deep():
    """T02: Fisura tectónica profunda con estrías de sombra y roca expuesta."""
    img = create_base_ground(103)
    pixels = img.load()
    
    fissure = [
        (2, 4), (5, 6), (9, 9), (13, 13), (16, 17), (18, 22), (22, 26), (26, 28), (30, 29)
    ]
    for i in range(len(fissure) - 1):
        x0, y0 = fissure[i]
        x1, y1 = fissure[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            cx = int(x0 + (x1 - x0) * t)
            cy = int(y0 + (y1 - y0) * t)
            if 0 <= cx < 32 and 0 <= cy < 32:
                pixels[cx, cy] = PALETTE["void"]
                if cy + 1 < 32:
                    pixels[cx, cy + 1] = PALETTE["shadow_abyss"]
                if cx + 1 < 32:
                    pixels[cx + 1, cy] = PALETTE["shadow_deep"]
                if cy > 0:
                    pixels[cx, cy - 1] = PALETTE["dust_crust"]
                if cy > 1:
                    pixels[cx, cy - 2] = PALETTE["sand_hi"]
                    
    branch = [(16, 17), (20, 16), (25, 14), (29, 13)]
    for i in range(len(branch) - 1):
        x0, y0 = branch[i]
        x1, y1 = branch[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            cx = int(x0 + (x1 - x0) * t)
            cy = int(y0 + (y1 - y0) * t)
            if 0 <= cx < 32 and 0 <= cy < 32:
                pixels[cx, cy] = PALETTE["shadow_deep"]
                if cy > 0:
                    pixels[cx, cy - 1] = PALETTE["sand_hi"]
    return img

def tile_t03_wasteland_gravel_rocks():
    """T03: Esparcimiento de rocas y gravilla basáltica."""
    img = create_base_ground(104)
    pixels = img.load()
    
    rocks = [
        (8, 7, 5, 4),
        (22, 11, 4, 3),
        (14, 20, 6, 5),
        (25, 24, 3, 3),
        (6, 23, 4, 3)
    ]
    for rx, ry, rw, rh in rocks:
        for y in range(ry + 1, ry + rh + 2):
            for x in range(rx + 1, rx + rw + 2):
                if 0 <= x < 32 and 0 <= y < 32:
                    pixels[x, y] = PALETTE["shadow_deep"]
        for y in range(ry, ry + rh):
            for x in range(rx, rx + rw):
                if 0 <= x < 32 and 0 <= y < 32:
                    pixels[x, y] = PALETTE["rock_mid"]
        for x in range(rx, rx + rw):
            if 0 <= x < 32 and 0 <= ry < 32:
                pixels[x, ry] = PALETTE["rock_hi"]
        for x in range(rx, rx + rw):
            if 0 <= x < 32 and 0 <= ry + rh - 1 < 32:
                pixels[x, ry + rh - 1] = PALETTE["rock_dark"]
                
    return img

def tile_t04_path_straight_v():
    """T04: Trinchera/sendero vertical hundido (32px de ancho).
    Cumple Regla 7: calzada como trinchera balística hundida con sombra arrojada de 3-4px."""
    img = Image.new("RGBA", (32, 32), PALETTE["clay_dark"])
    pixels = img.load()
    
    rng = random.Random(201)
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            if val > 0.65:
                pixels[x, y] = PALETTE["clay_mid"]
            elif val > 0.35:
                pixels[x, y] = PALETTE["clay_dark"]
            else:
                pixels[x, y] = PALETTE["shadow_deep"]
                
    for y in range(32):
        if rng.random() > 0.2:
            pixels[11, y] = PALETTE["shadow_deep"]
            pixels[12, y] = PALETTE["clay_dark"]
        if rng.random() > 0.2:
            pixels[20, y] = PALETTE["shadow_deep"]
            pixels[21, y] = PALETTE["clay_dark"]
            
    for y in range(32):
        pixels[0, y] = PALETTE["sand_mid"]
        pixels[1, y] = PALETTE["sand_low"]
        pixels[2, y] = PALETTE["clay_hi"]
        pixels[3, y] = PALETTE["void"]
        pixels[4, y] = PALETTE["shadow_abyss"]
        pixels[5, y] = PALETTE["shadow_deep"]
        
    for y in range(32):
        pixels[28, y] = PALETTE["clay_mid"]
        pixels[29, y] = PALETTE["sand_low"]
        pixels[30, y] = PALETTE["dust_crust"]
        pixels[31, y] = PALETTE["sand_mid"]

    return img

def tile_t05_path_straight_h():
    """T05: Trinchera/sendero horizontal hundido (32px de ancho).
    Sombra profunda bajo labio superior (coordenadas Y 0 a 5)."""
    img = Image.new("RGBA", (32, 32), PALETTE["clay_dark"])
    pixels = img.load()
    
    rng = random.Random(202)
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            if val > 0.65:
                pixels[x, y] = PALETTE["clay_mid"]
            elif val > 0.35:
                pixels[x, y] = PALETTE["clay_dark"]
            else:
                pixels[x, y] = PALETTE["shadow_deep"]
                
    for x in range(32):
        if rng.random() > 0.2:
            pixels[x, 11] = PALETTE["shadow_deep"]
            pixels[x, 12] = PALETTE["clay_dark"]
        if rng.random() > 0.2:
            pixels[x, 20] = PALETTE["shadow_deep"]
            pixels[x, 21] = PALETTE["clay_dark"]
            
    for x in range(32):
        pixels[x, 0] = PALETTE["sand_mid"]
        pixels[x, 1] = PALETTE["sand_low"]
        pixels[x, 2] = PALETTE["clay_hi"]
        pixels[x, 3] = PALETTE["void"]
        pixels[x, 4] = PALETTE["shadow_abyss"]
        pixels[x, 5] = PALETTE["shadow_deep"]
        
    for x in range(32):
        pixels[x, 28] = PALETTE["clay_mid"]
        pixels[x, 29] = PALETTE["sand_low"]
        pixels[x, 30] = PALETTE["dust_crust"]
        pixels[x, 31] = PALETTE["sand_mid"]

    return img

def tile_t06_path_corner_turn():
    """T06: Curva/codo del camino (conexión Norte -> Este)."""
    img = Image.new("RGBA", (32, 32), PALETTE["clay_dark"])
    pixels = img.load()
    rng = random.Random(203)
    
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["clay_dark"] if val > 0.4 else PALETTE["clay_mid"]
            
    for y in range(32):
        for x in range(32):
            if x <= 5 and y <= 31:
                if x <= 1:
                    pixels[x, y] = PALETTE["sand_mid"]
                elif x == 2:
                    pixels[x, y] = PALETTE["clay_hi"]
                elif x in (3, 4):
                    pixels[x, y] = PALETTE["void"]
                else:
                    pixels[x, y] = PALETTE["shadow_abyss"]
            if y <= 5 and x <= 31:
                if y <= 1:
                    pixels[x, y] = PALETTE["sand_mid"]
                elif y == 2:
                    pixels[x, y] = PALETTE["clay_hi"]
                elif y in (3, 4):
                    pixels[x, y] = PALETTE["void"]
                else:
                    pixels[x, y] = PALETTE["shadow_abyss"]
                    
            if x >= 27 and y >= 27:
                pixels[x, y] = PALETTE["dust_crust"]
                
    return img

def tile_t07_path_junction():
    """T07: Intersección / bifurcación de caminos con marcas de tráfico."""
    img = Image.new("RGBA", (32, 32), PALETTE["clay_dark"])
    pixels = img.load()
    rng = random.Random(204)
    
    for y in range(32):
        for x in range(32):
            dist_center = math.hypot(x - 15.5, y - 15.5)
            val = rng.uniform(0, 1)
            if dist_center < 7:
                pixels[x, y] = PALETTE["shadow_deep"] if val > 0.4 else PALETTE["clay_dark"]
            else:
                pixels[x, y] = PALETTE["clay_mid"] if val > 0.5 else PALETTE["clay_dark"]
                
    for i in range(5, 27, 4):
        pixels[i, 15] = PALETTE["void"]
        pixels[i+1, 15] = PALETTE["shadow_abyss"]
        pixels[15, i] = PALETTE["void"]
        pixels[15, i+1] = PALETTE["shadow_abyss"]
        
    return img

def tile_t08_path_crater():
    """T08: Cráter de artillería en el sendero."""
    img = tile_t04_path_straight_v()
    pixels = img.load()
    
    cx, cy = 16, 16
    radius = 9
    for y in range(32):
        for x in range(32):
            d = math.hypot(x - cx, y - cy)
            if d <= radius:
                if d < 3.5:
                    pixels[x, y] = PALETTE["void"]
                elif d < 6:
                    pixels[x, y] = PALETTE["shadow_abyss"]
                elif d < 8:
                    pixels[x, y] = PALETTE["shadow_deep"]
                else:
                    if y < cy:
                        pixels[x, y] = PALETTE["dust_crust"]
                    else:
                        pixels[x, y] = PALETTE["clay_hi"]
            elif d < radius + 2 and y < cy and abs(x - cx) < 8:
                pixels[x, y] = PALETTE["sand_hi"]
                
    return img

def tile_t09_cliff_edge_s():
    """T09: Farallón rocoso orientado al Sur."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(301)
    
    for y in range(16):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_hi"] if val > 0.6 else PALETTE["sand_mid"]
            
    for x in range(32):
        pixels[x, 15] = PALETTE["dust_crust"]
        
    for y in range(16, 32):
        for x in range(32):
            dy = y - 16
            if dy < 3:
                pixels[x, y] = PALETTE["void"]
            elif dy < 8:
                pixels[x, y] = PALETTE["shadow_abyss"]
            elif dy < 12:
                pixels[x, y] = PALETTE["rock_dark"]
            else:
                pixels[x, y] = PALETTE["clay_dark"] if rng.random() > 0.4 else PALETTE["rock_mid"]
                
    return img

def tile_t10_cliff_edge_n():
    """T10: Farallón orientado al Norte."""
    img = Image.new("RGBA", (32, 32), PALETTE["clay_mid"])
    pixels = img.load()
    rng = random.Random(302)
    
    for y in range(16, 32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["clay_mid"] if val > 0.5 else PALETTE["sand_low"]
            
    for y in range(16):
        for x in range(32):
            if y < 4:
                pixels[x, y] = PALETTE["rock_mid"]
            elif y < 10:
                pixels[x, y] = PALETTE["rock_dark"]
            elif y < 14:
                pixels[x, y] = PALETTE["shadow_deep"]
            else:
                pixels[x, y] = PALETTE["void"]
                
    return img

def tile_t11_boulder_formation():
    """T11: Formación de grandes rocas monolíticas."""
    img = create_base_ground(303)
    pixels = img.load()
    
    for y in range(8, 32):
        for x in range(12, 32):
            if (x - 20)**2 + (y - 22)**2 < 85:
                pixels[x, y] = PALETTE["shadow_deep"]
                
    for y in range(4, 28):
        for x in range(4, 28):
            dx = (x - 16) / 10.0
            dy = (y - 15) / 10.0
            if dx*dx + dy*dy <= 1.0:
                if dx + dy < -0.3:
                    pixels[x, y] = PALETTE["rock_hi"]
                elif dx + dy < 0.3:
                    pixels[x, y] = PALETTE["rock_mid"]
                else:
                    pixels[x, y] = PALETTE["rock_dark"]
                    
    for y in range(9, 21):
        pixels[15 + (y % 2), y] = PALETTE["shadow_abyss"]
        
    return img

def tile_t12_scree_slope():
    """T12: Pedregal / canchal de derrumbe que conecta cotas."""
    img = create_base_ground(304)
    pixels = img.load()
    rng = random.Random(305)
    
    for y in range(32):
        for x in range(32):
            if abs(x - y) < 10:
                val = rng.uniform(0, 1)
                if val > 0.7:
                    pixels[x, y] = PALETTE["rock_hi"]
                elif val > 0.4:
                    pixels[x, y] = PALETTE["rock_mid"]
                elif val > 0.2:
                    pixels[x, y] = PALETTE["rock_dark"]
                else:
                    pixels[x, y] = PALETTE["shadow_deep"]
    return img

def tile_t13_turret_pad_plate():
    """T13: Plataforma de anclaje de torreta Mechanicus (32x32)."""
    img = create_base_ground(401)
    pixels = img.load()
    
    for y in range(2, 30):
        for x in range(2, 30):
            if (x - 2) + (y - 2) < 4: continue
            if (29 - x) + (y - 2) < 4: continue
            if (x - 2) + (29 - y) < 4: continue
            if (29 - x) + (29 - y) < 4: continue
            
            if (x in (2, 29) or y in (2, 29) or 
                (x - 2) + (y - 2) == 4 or (29 - x) + (y - 2) == 4 or
                (x - 2) + (29 - y) == 4 or (29 - x) + (29 - y) == 4):
                if y <= 16:
                    pixels[x, y] = PALETTE["steel_hi"]
                else:
                    pixels[x, y] = PALETTE["steel_dark"]
            else:
                if (x + y) % 3 == 0:
                    pixels[x, y] = PALETTE["rust_hi"]
                elif (x + y) % 5 == 0:
                    pixels[x, y] = PALETTE["rust_dark"]
                else:
                    pixels[x, y] = PALETTE["steel_mid"]
                    
    for x in range(4, 30):
        if pixels[x, 30] != PALETTE["shadow_deep"]:
            pixels[x, 30] = PALETTE["shadow_deep"]
    for y in range(4, 30):
        if pixels[30, y] != PALETTE["shadow_deep"]:
            pixels[30, y] = PALETTE["shadow_deep"]

    rivets = [
        (6, 4), (25, 4), (4, 6), (27, 6),
        (4, 25), (27, 25), (6, 27), (25, 27)
    ]
    for rx, ry in rivets:
        pixels[rx, ry] = PALETTE["steel_hi"]
        pixels[rx + 1, ry + 1] = PALETTE["steel_dark"]

    for y in range(10, 22):
        for x in range(10, 22):
            d = math.hypot(x - 15.5, y - 15.5)
            if d < 5.5:
                if d > 4.2:
                    pixels[x, y] = PALETTE["steel_hi"] if y < 16 else PALETTE["steel_dark"]
                elif int(d) % 2 == 0:
                    pixels[x, y] = PALETTE["void"]
                else:
                    pixels[x, y] = PALETTE["steel_mid"]

    return img

def tile_t14_pipeline_exposed():
    """T14: Conducción industrial de combustible/refrigerante rota."""
    img = create_base_ground(402)
    pixels = img.load()
    
    for y in range(11, 21):
        for x in range(0, 32):
            dy = y - 11
            if dy == 0:
                pixels[x, y] = PALETTE["rust_hi"]
            elif dy in (1, 2):
                pixels[x, y] = PALETTE["steel_hi"]
            elif dy in (3, 4):
                pixels[x, y] = PALETTE["steel_mid"]
            elif dy in (5, 6):
                pixels[x, y] = PALETTE["rust_dark"]
            elif dy in (7, 8):
                pixels[x, y] = PALETTE["steel_dark"]
            elif dy == 9:
                pixels[x, y] = PALETTE["shadow_abyss"]
                
    for y in range(21, 24):
        for x in range(0, 32):
            pixels[x, y] = PALETTE["shadow_deep"]
            
    for bx in (6, 22):
        for by in range(9, 23):
            if by == 9:
                pixels[bx, by] = PALETTE["rust_hi"]
                pixels[bx + 1, by] = PALETTE["rust_hi"]
            elif by < 22:
                pixels[bx, by] = PALETTE["steel_hi"]
                pixels[bx + 1, by] = PALETTE["steel_dark"]
            else:
                pixels[bx, by] = PALETTE["void"]
                pixels[bx + 1, by] = PALETTE["void"]
                
    for fx, fy in [(15, 21), (16, 21), (15, 22), (16, 22), (17, 22), (16, 23)]:
        pixels[fx, fy] = PALETTE["void"]
        
    return img

def tile_t15_sandbag_fortification():
    """T15: Nido defensivo de sacos terreros y alambre de espino."""
    img = create_base_ground(403)
    pixels = img.load()
    
    bags = [
        (3, 18, 8, 4), (12, 18, 8, 4), (21, 18, 8, 4),
        (6, 14, 8, 4), (15, 14, 8, 4),
        (10, 11, 8, 3)
    ]
    
    for bx, by, bw, bh in bags:
        for x in range(bx + 1, bx + bw + 2):
            for y in range(by + bh, by + bh + 3):
                if 0 <= x < 32 and 0 <= y < 32:
                    pixels[x, y] = PALETTE["shadow_deep"]
                    
    for bx, by, bw, bh in bags:
        for y in range(by, by + bh):
            for x in range(bx, bx + bw):
                if 0 <= x < 32 and 0 <= y < 32:
                    if x in (bx, bx + bw - 1) or y == by + bh - 1:
                        pixels[x, y] = PALETTE["clay_dark"]
                    elif y == by:
                        pixels[x, y] = PALETTE["sand_hi"]
                    else:
                        pixels[x, y] = PALETTE["sand_low"]
                        
    for y in range(7, 18):
        pixels[28, y] = PALETTE["steel_mid"]
    pixels[27, 8] = PALETTE["steel_hi"]
    pixels[29, 12] = PALETTE["steel_hi"]
    pixels[27, 15] = PALETTE["steel_hi"]

    return img


TILES = [
    ("T00_wasteland_plain", tile_t00_wasteland_plain),
    ("T01_wasteland_cracked", tile_t01_wasteland_cracked),
    ("T02_wasteland_fissure_deep", tile_t02_wasteland_fissure_deep),
    ("T03_wasteland_gravel_rocks", tile_t03_wasteland_gravel_rocks),
    ("T04_path_straight_v", tile_t04_path_straight_v),
    ("T05_path_straight_h", tile_t05_path_straight_h),
    ("T06_path_corner_turn", tile_t06_path_corner_turn),
    ("T07_path_junction", tile_t07_path_junction),
    ("T08_path_crater", tile_t08_path_crater),
    ("T09_cliff_edge_s", tile_t09_cliff_edge_s),
    ("T10_cliff_edge_n", tile_t10_cliff_edge_n),
    ("T11_boulder_formation", tile_t11_boulder_formation),
    ("T12_scree_slope", tile_t12_scree_slope),
    ("T13_turret_pad_plate", tile_t13_turret_pad_plate),
    ("T14_pipeline_exposed", tile_t14_pipeline_exposed),
    ("T15_sandbag_fortification", tile_t15_sandbag_fortification),
]

def main():
    generated_images = {}
    
    print("--- Generando 16 tiles modulares 32x32 para Yermo Balístico ---")
    for name, func in TILES:
        img_1x = func()
        p1x = os.path.join(OUTPUT_DIR, f"{name}_1x.png")
        img_1x.save(p1x)
        
        img_8x = img_1x.resize((256, 256), Image.NEAREST)
        p8x = os.path.join(OUTPUT_DIR, f"{name}_8x.png")
        img_8x.save(p8x)
        
        generated_images[name] = img_1x
        print(f"  [OK] {name} (1x: 32x32, 8x: 256x256)")
        
    catalog_sheet = Image.new("RGBA", (4 * 128, 4 * 128), (20, 15, 10, 255))
    for idx, (name, _) in enumerate(TILES):
        row = idx // 4
        col = idx % 4
        tile_preview = generated_images[name].resize((128, 128), Image.NEAREST)
        catalog_sheet.paste(tile_preview, (col * 128, row * 128))
    catalog_path = os.path.join(OUTPUT_DIR, "wasteland_tileset_catalog_4x.png")
    catalog_sheet.save(catalog_path)
    print(f"Catálogo completo guardado en: {catalog_path}")

    mockup_map = [
        ["T00_wasteland_plain", "T09_cliff_edge_s", "T11_boulder_formation", "T00_wasteland_plain", "T04_path_straight_v", "T00_wasteland_plain", "T03_wasteland_gravel_rocks", "T00_wasteland_plain"],
        ["T13_turret_pad_plate", "T00_wasteland_plain", "T01_wasteland_cracked", "T06_path_corner_turn", "T07_path_junction", "T05_path_straight_h", "T05_path_straight_h", "T06_path_corner_turn"],
        ["T00_wasteland_plain", "T13_turret_pad_plate", "T00_wasteland_plain", "T04_path_straight_v", "T08_path_crater", "T00_wasteland_plain", "T15_sandbag_fortification", "T04_path_straight_v"],
        ["T02_wasteland_fissure_deep", "T00_wasteland_plain", "T12_scree_slope", "T04_path_straight_v", "T00_wasteland_plain", "T13_turret_pad_plate", "T00_wasteland_plain", "T04_path_straight_v"],
        ["T14_pipeline_exposed", "T01_wasteland_cracked", "T00_wasteland_plain", "T06_path_corner_turn", "T05_path_straight_h", "T05_path_straight_h", "T05_path_straight_h", "T07_path_junction"],
        ["T00_wasteland_plain", "T03_wasteland_gravel_rocks", "T10_cliff_edge_n", "T00_wasteland_plain", "T00_wasteland_plain", "T01_wasteland_cracked", "T13_turret_pad_plate", "T04_path_straight_v"]
    ]

    scene_ds = Image.new("RGBA", (256, 192), (0, 0, 0, 255))
    for r in range(6):
        for c in range(8):
            tile_key = mockup_map[r][c]
            scene_ds.paste(generated_images[tile_key], (c * 32, r * 32))

    turret_path = "assets/sprites/turrets/heavy_bolter_strip_master_1x.png"
    if os.path.exists(turret_path):
        turret_strip = Image.open(turret_path)
        turret_frame = turret_strip.crop((0, 0, 32, 32))
        scene_ds.paste(turret_frame, (0 * 32, 1 * 32), turret_frame)
        scene_ds.paste(turret_frame, (1 * 32, 2 * 32), turret_frame)
        scene_ds.paste(turret_frame, (5 * 32, 3 * 32), turret_frame)
        scene_ds.paste(turret_frame, (6 * 32, 5 * 32), turret_frame)

    enemy_t1_path = "assets/sprites/enemies/t1_ripper_strip_master_1x.png"
    enemy_t2_path = "assets/sprites/enemies/t2_hormagaunt_strip_master_1x.png"
    
    if os.path.exists(enemy_t1_path):
        t1_strip = Image.open(enemy_t1_path)
        t1_frame = t1_strip.crop((0, 0, 16, 16))
        scene_ds.paste(t1_frame, (4 * 32 + 8, 0 * 32 + 8), t1_frame)
        scene_ds.paste(t1_frame, (4 * 32 + 14, 0 * 32 + 18), t1_frame)
        scene_ds.paste(t1_frame, (5 * 32 + 10, 1 * 32 + 8), t1_frame)

    if os.path.exists(enemy_t2_path):
        t2_strip = Image.open(enemy_t2_path)
        t2_frame = t2_strip.crop((0, 0, 24, 24))
        scene_ds.paste(t2_frame, (4 * 32 + 4, 2 * 32 + 4), t2_frame)
        scene_ds.paste(t2_frame, (3 * 32 + 4, 3 * 32 + 4), t2_frame)

    mockup_1x_path = os.path.join(OUTPUT_DIR, "wasteland_scene_mockup_1x.png")
    scene_ds.save(mockup_1x_path)
    mockup_3x_path = os.path.join(OUTPUT_DIR, "wasteland_scene_mockup_3x.png")
    scene_ds.resize((768, 576), Image.NEAREST).save(mockup_3x_path)
    print(f"Mockup escénico canónico guardado en: {mockup_3x_path}")

if __name__ == "__main__":
    main()
