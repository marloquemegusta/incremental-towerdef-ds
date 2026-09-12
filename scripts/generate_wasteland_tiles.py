"""
Script de generación del tileset 32x32 de Tierra Árida / Yermo Balístico para Nintendo DS (v2).
Optimizaciones basadas en feedback del usuario:
1. Eliminado entramado de rombos/patrón sinusoidal regular del suelo base.
2. Tonos de suelo mucho más claros y uniformes para maximizar contraste con enemigos xenos oscuros.
3. Reemplazado el tile de sacos de arena (T15) por una red densa de fracturas y barro cuarteado por sequedad extrema (T15_cracked_network).
4. Mantener y potenciar fracturas, grietas tectónicas y formaciones rocosas.
"""

import os
import math
import random
from PIL import Image, ImageDraw

OUTPUT_DIR = "assets/tiles/wasteland"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Paleta luminosa de suelo árido (alto contraste con enjambre xenos oscuro)
# Auditada contra Regla 8: Cero púrpuras, cero blanco hueso saturado (>230 en todos)
PALETTE = {
    # Cota alta / Suelo árido luminoso (tonos claros predominantes)
    "sand_top": (224, 210, 180),    # Resalte superior suave
    "sand_hi": (210, 192, 156),     # Tono base principal (claro, limpio)
    "sand_mid": (192, 172, 136),    # Variación suave
    "sand_low": (172, 150, 114),    # Transición cálida
    "clay_light": (148, 126, 92),   # Matiz terroso suave
    
    # Grietas y sombras profundas (alto contraste local)
    "shadow_deep": (44, 26, 13),    # #2C1A0D
    "shadow_abyss": (25, 15, 8),    # #190F08
    "void": (11, 6, 3),             # #0B0603

    # Rocas y grava basáltica
    "rock_hi": (130, 125, 115),     # Roca iluminada
    "rock_mid": (92, 86, 78),       # Cuerpo de roca
    "rock_dark": (58, 52, 45),      # Base de roca
    
    # Metal oxidado / Mechanicus
    "rust_hi": (148, 82, 45),
    "rust_dark": (98, 50, 24),
    "steel_hi": (125, 130, 135),
    "steel_mid": (85, 90, 95),
    "steel_dark": (48, 52, 56),

    # Caliche / costra de polvo seco (muy claro pero sin saturar)
    "dust_crust": (228, 216, 192),
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
    """Crea una base lisa y clara de suelo árido sin patrones regulares ni rombos."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_hi"])
    pixels = img.load()
    rng = random.Random(rng_seed)
    
    # Textura muy suave y orgánica basada en ruido perlino de gradiente muy bajo
    # Sin senos ni cosenos multiplicados que provoquen rombos o rejillas
    for y in range(32):
        for x in range(32):
            # Gradiente suave toroidal
            nx = (x / 32.0) * 2 * math.pi
            ny = (y / 32.0) * 2 * math.pi
            # Armónicos suaves no alineados en producto ortogonal
            wave = (math.sin(nx + 0.5) * 0.3 + 
                    math.cos(ny * 1.5 + 0.3) * 0.25 + 
                    math.sin(nx * 2 + ny * 1.5) * 0.2 + 
                    rng.uniform(-0.15, 0.15))
            
            if wave > 0.35:
                pixels[x, y] = PALETTE["sand_top"]
            elif wave > -0.15:
                pixels[x, y] = PALETTE["sand_hi"]
            elif wave > -0.45:
                pixels[x, y] = PALETTE["sand_mid"]
            else:
                pixels[x, y] = PALETTE["sand_low"]
                
    # Micro-motas de polvo árido y salitre disperso (muy sutil, sin formar cuadrícula)
    for _ in range(7):
        rx, ry = rng.randint(0, 31), rng.randint(0, 31)
        pixels[rx, ry] = PALETTE["dust_crust"]
        
    for _ in range(4):
        rx, ry = rng.randint(0, 31), rng.randint(0, 31)
        pixels[rx, ry] = PALETTE["clay_light"]

    return img

def tile_t00_wasteland_plain():
    """T00: Tierra árida lisa continua (seamless), limpia y clara."""
    return create_base_ground(101)

def tile_t01_wasteland_cracked():
    """T01: Tierra árida con finas grietas superficiales por sequedad."""
    img = create_base_ground(102)
    pixels = img.load()
    
    crack_paths = [
        [(5, 7), (8, 9), (13, 11), (16, 14), (18, 20), (22, 23), (27, 24)],
        [(16, 14), (20, 12), (25, 11), (29, 12)],
        [(18, 20), (15, 24), (13, 29)],
        [(7, 21), (10, 23), (13, 24)]
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
    return img

def tile_t02_wasteland_fissure_deep():
    """T02: Fisura tectónica profunda con sombra abismal y roca basáltica expuesta."""
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
                    pixels[cx, cy - 2] = PALETTE["sand_top"]
                    
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
                    pixels[cx, cy - 1] = PALETTE["dust_crust"]
    return img

def tile_t03_wasteland_gravel_rocks():
    """T03: Esparcimiento de rocas y peñascos basálticos sobre arena clara."""
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
    Suelo de sendero en tono medio/claro con sombra balística profunda de 4px en el labio oeste."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(201)
    
    # Fondo del sendero compacto y limpio
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_mid"] if val > 0.4 else PALETTE["sand_low"]
            
    # Huellas de rodada sutiles
    for y in range(32):
        if rng.random() > 0.3:
            pixels[11, y] = PALETTE["clay_light"]
        if rng.random() > 0.3:
            pixels[20, y] = PALETTE["clay_light"]
            
    # Borde oeste (coordenadas 0 a 5): Cornisa alta clara y sombra profunda arrojada (4px)
    for y in range(32):
        pixels[0, y] = PALETTE["sand_top"]
        pixels[1, y] = PALETTE["sand_hi"]
        pixels[2, y] = PALETTE["clay_light"]
        pixels[3, y] = PALETTE["void"]
        pixels[4, y] = PALETTE["shadow_abyss"]
        pixels[5, y] = PALETTE["shadow_deep"]
        
    # Borde este (coordenadas 28 a 31): Labio opuesto iluminado
    for y in range(32):
        pixels[28, y] = PALETTE["sand_low"]
        pixels[29, y] = PALETTE["sand_mid"]
        pixels[30, y] = PALETTE["dust_crust"]
        pixels[31, y] = PALETTE["sand_top"]

    return img

def tile_t05_path_straight_h():
    """T05: Trinchera/sendero horizontal hundido (32px de ancho).
    Cornisa norte clara con sombra profunda arrojada de 4px."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(202)
    
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_mid"] if val > 0.4 else PALETTE["sand_low"]
            
    for x in range(32):
        if rng.random() > 0.3:
            pixels[x, 11] = PALETTE["clay_light"]
        if rng.random() > 0.3:
            pixels[x, 20] = PALETTE["clay_light"]
            
    # Borde superior (Y 0..5): Cornisa y sombra profunda arrojada de 4px
    for x in range(32):
        pixels[x, 0] = PALETTE["sand_top"]
        pixels[x, 1] = PALETTE["sand_hi"]
        pixels[x, 2] = PALETTE["clay_light"]
        pixels[x, 3] = PALETTE["void"]
        pixels[x, 4] = PALETTE["shadow_abyss"]
        pixels[x, 5] = PALETTE["shadow_deep"]
        
    # Borde inferior (Y 28..31): Labio iluminado
    for x in range(32):
        pixels[x, 28] = PALETTE["sand_low"]
        pixels[x, 29] = PALETTE["sand_mid"]
        pixels[x, 30] = PALETTE["dust_crust"]
        pixels[x, 31] = PALETTE["sand_top"]

    return img

def tile_t06_path_corner_turn():
    """T06: Curva/codo del sendero conectando Norte con Este."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(203)
    
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_mid"] if val > 0.4 else PALETTE["sand_low"]
            
    for y in range(32):
        for x in range(32):
            # Cornisa alta en ángulo NO
            if x <= 5 and y <= 31:
                if x <= 1: pixels[x, y] = PALETTE["sand_top"]
                elif x == 2: pixels[x, y] = PALETTE["clay_light"]
                elif x in (3, 4): pixels[x, y] = PALETTE["void"]
                else: pixels[x, y] = PALETTE["shadow_abyss"]
            if y <= 5 and x <= 31:
                if y <= 1: pixels[x, y] = PALETTE["sand_top"]
                elif y == 2: pixels[x, y] = PALETTE["clay_light"]
                elif y in (3, 4): pixels[x, y] = PALETTE["void"]
                else: pixels[x, y] = PALETTE["shadow_abyss"]
                    
            # Labio receptor SE
            if x >= 27 and y >= 27:
                pixels[x, y] = PALETTE["dust_crust"]
                
    return img

def tile_t07_path_junction():
    """T07: Intersección / bifurcación limpia de senderos."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(204)
    
    for y in range(32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_mid"] if val > 0.45 else PALETTE["sand_low"]
            
    # Marcas suaves de rodaduras cruzadas
    for i in range(5, 27, 4):
        pixels[i, 15] = PALETTE["clay_light"]
        pixels[i+1, 15] = PALETTE["clay_light"]
        pixels[15, i] = PALETTE["clay_light"]
        pixels[15, i+1] = PALETTE["clay_light"]
        
    return img

def tile_t08_path_crater():
    """T08: Cráter de artillería en el sendero con eyección de escombros basálticos."""
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
                        pixels[x, y] = PALETTE["sand_low"]
            elif d < radius + 2 and y < cy and abs(x - cx) < 8:
                pixels[x, y] = PALETTE["sand_top"]
                
    return img

def tile_t09_cliff_edge_s():
    """T09: Farallón rocoso orientado al Sur."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_hi"])
    pixels = img.load()
    rng = random.Random(301)
    
    # Cota alta clara
    for y in range(16):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_top"] if val > 0.6 else PALETTE["sand_hi"]
            
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
                pixels[x, y] = PALETTE["sand_low"] if rng.random() > 0.4 else PALETTE["rock_mid"]
                
    return img

def tile_t10_cliff_edge_n():
    """T10: Farallón orientado al Norte."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_mid"])
    pixels = img.load()
    rng = random.Random(302)
    
    for y in range(16, 32):
        for x in range(32):
            val = rng.uniform(0, 1)
            pixels[x, y] = PALETTE["sand_hi"] if val > 0.5 else PALETTE["sand_mid"]
            
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
    """T11: Formación de grandes rocas monolíticas erosionadas."""
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

def tile_t15_cracked_network():
    """T15: Red intrincada de fracturas y barro cuarteado por sequedad extrema.
    Reemplaza a los antiguos sacos de arena a petición directa del usuario."""
    img = create_base_ground(403)
    pixels = img.load()
    
    # Red poligonal de cuarteamiento árido (desiccation cracks)
    fissure_segments = [
        [(2, 10), (8, 11), (13, 8), (17, 10), (24, 7), (29, 9)],
        [(8, 11), (9, 18), (6, 24), (4, 30)],
        [(13, 8), (14, 2), (15, 0)],
        [(17, 10), (18, 17), (15, 23), (17, 30)],
        [(18, 17), (23, 20), (28, 18), (31, 21)],
        [(23, 20), (25, 27), (27, 31)],
        [(9, 18), (15, 23)],
        [(6, 24), (11, 27), (15, 28)]
    ]
    
    for seg in fissure_segments:
        for i in range(len(seg) - 1):
            x0, y0 = seg[i]
            x1, y1 = seg[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                cx = int(x0 + (x1 - x0) * t)
                cy = int(y0 + (y1 - y0) * t)
                if 0 <= cx < 32 and 0 <= cy < 32:
                    pixels[cx, cy] = PALETTE["shadow_deep"]
                    # Reborde iluminado
                    if cy > 0:
                        pixels[cx, cy - 1] = PALETTE["dust_crust"]
                        
    # Añadir micro-bloques cuarteados con variación de tono
    polygons_centers = [(10, 6), (22, 13), (13, 16), (20, 25), (8, 22)]
    for px, py in polygons_centers:
        if 0 <= px < 32 and 0 <= py < 32:
            pixels[px, py] = PALETTE["sand_top"]

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
    ("T15_cracked_network", tile_t15_cracked_network),
]

def main():
    generated_images = {}
    
    print("--- Generando 16 tiles modulares 32x32 para Yermo Balístico (v2) ---")
    for name, func in TILES:
        img_1x = func()
        p1x = os.path.join(OUTPUT_DIR, f"{name}_1x.png")
        img_1x.save(p1x)
        
        img_8x = img_1x.resize((256, 256), Image.NEAREST)
        p8x = os.path.join(OUTPUT_DIR, f"{name}_8x.png")
        img_8x.save(p8x)
        
        generated_images[name] = img_1x
        print(f"  [OK] {name} (1x: 32x32, 8x: 256x256)")
        
    catalog_sheet = Image.new("RGBA", (4 * 128, 4 * 128), (24, 20, 16, 255))
    for idx, (name, _) in enumerate(TILES):
        row = idx // 4
        col = idx % 4
        tile_preview = generated_images[name].resize((128, 128), Image.NEAREST)
        catalog_sheet.paste(tile_preview, (col * 128, row * 128))
    catalog_path = os.path.join(OUTPUT_DIR, "wasteland_tileset_catalog_4x.png")
    catalog_sheet.save(catalog_path)
    print(f"Catálogo completo guardado en: {catalog_path}")

    # Mockup escénico actualizado
    mockup_map = [
        ["T00_wasteland_plain", "T09_cliff_edge_s", "T11_boulder_formation", "T00_wasteland_plain", "T04_path_straight_v", "T00_wasteland_plain", "T03_wasteland_gravel_rocks", "T00_wasteland_plain"],
        ["T13_turret_pad_plate", "T00_wasteland_plain", "T01_wasteland_cracked", "T06_path_corner_turn", "T07_path_junction", "T05_path_straight_h", "T05_path_straight_h", "T06_path_corner_turn"],
        ["T00_wasteland_plain", "T13_turret_pad_plate", "T00_wasteland_plain", "T04_path_straight_v", "T08_path_crater", "T00_wasteland_plain", "T15_cracked_network", "T04_path_straight_v"],
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
