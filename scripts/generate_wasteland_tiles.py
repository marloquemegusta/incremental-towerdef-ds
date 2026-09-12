"""
Script de generación del tileset 32x32 de Tierra Árida / Yermo Balístico para Nintendo DS (v4 - Plano y Orgánico).
Filosofía de juego ajustada:
1. TERRENO 100% PLANO Y TRANSITABLE: Sin barricadas, sin muros rocosos verticales, sin agujeros/fosos que parezcan barreras. Los enemigos caminan por encima de forma natural.
2. VARIEDAD NATURAL Y ORGÁNICA:
   - Diversos tipos de fisuras (radiales/tela de araña, zigzag, paralelas, mosaico de barro, expansiva).
   - Vegetación árida a ras de suelo (matorrales secos, hierba desértica, raíces rastreras).
   - Charcos de agua estancada y lodo salobre a nivel de suelo (reflejo plomizo/pizarra, fondo sedimentario).
   - Costras de salitre mineral, gravilla y marcajes de plantilla Mechanicus a ras de suelo.
3. 100% PERIÓDICO Y TILEABLE: Todos los tiles enlazan sin costuras perimetrales con T00 y entre sí.
4. MÁXIMO CONTRASTE: Tonos de suelo claros y luminosos para destacar las siluetas del enjambre xenos.
"""

import os
import math
import random
from PIL import Image, ImageDraw

OUTPUT_DIR = "assets/tiles/wasteland"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Paleta luminosa y natural auditada (Regla 8: Cero púrpuras, cero blanco hueso > 230)
PALETTE = {
    # Suelo árido base luminoso
    "sand_top": (224, 212, 182),    # #E0D4B6
    "sand_hi": (212, 194, 158),     # #D4C29E (Tono principal predominante)
    "sand_mid": (194, 174, 138),    # #C2AE8A
    "sand_low": (174, 152, 116),    # #AE9874
    "clay_light": (152, 128, 94),   # #98805E
    
    # Grietas y fisuras planas
    "crack_dark": (48, 30, 16),     # Línea de fisura
    "crack_soft": (82, 56, 32),     # Degradado de grieta
    "crack_lip": (228, 218, 194),    # Borde mineral iluminado

    # Vegetación árida desértica (matorral seco, verde-oliva marchito y ramas)
    "veg_hi": (122, 132, 78),       # Hoja seca iluminada
    "veg_mid": (90, 98, 54),        # Cuerpo de arbusto
    "veg_dark": (58, 64, 36),       # Base de planta a ras de suelo
    "veg_dry": (150, 136, 88),      # Paja / espino seco
    "veg_branch": (76, 62, 42),     # Rama seca

    # Charcos de agua salobre / lodo (plano, azul pizarra / gris plomizo)
    "water_hi": (92, 128, 142),     # Brillo / reflejo de cielo plomizo
    "water_mid": (62, 92, 106),     # Cuerpo de agua estancada
    "water_deep": (42, 64, 76),     # Fondo turbio / profundidad plana
    "mud_wet": (72, 62, 50),        # Lodo húmedo perimetral
    "salt_rim": (205, 212, 198),    # Cerco de evaporación salina

    # Minerales, gravilla y marcas Mechanicus planas
    "pebble_hi": (136, 130, 120),
    "pebble_mid": (98, 92, 84),
    "pebble_dark": (64, 58, 52),
    "stencil_red": (148, 64, 44),   # Pintura desgastada Mechanicus
    "caliche": (228, 220, 198),
}

def verify_color(color):
    r, g, b = color[:3]
    if r >= 105 and b >= 140 and (b > g + 25):
        raise ValueError(f"Color prohibido xenos detectado: {color}")
    if r > 230 and g > 230 and b > 230:
        raise ValueError(f"Blanco excesivo detectado: {color}")
    return color

for k, c in PALETTE.items():
    verify_color(c)

def create_toroidal_base(rng_seed=42):
    """Crea una base lisa y clara de 32x32 estrictamente toroidal y seamless."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_hi"])
    pixels = img.load()
    rng = random.Random(rng_seed)
    
    for y in range(32):
        for x in range(32):
            nx = (x / 32.0) * 2 * math.pi
            ny = (y / 32.0) * 2 * math.pi
            val = (math.sin(nx + 0.7) * 0.28 + 
                   math.cos(ny + 1.1) * 0.24 + 
                   math.sin(nx + ny + 0.4) * 0.18 + 
                   math.cos(nx - ny + 1.2) * 0.12 + 
                   rng.uniform(-0.08, 0.08))
            
            if val > 0.30:
                pixels[x, y] = PALETTE["sand_top"]
            elif val > -0.12:
                pixels[x, y] = PALETTE["sand_hi"]
            elif val > -0.38:
                pixels[x, y] = PALETTE["sand_mid"]
            else:
                pixels[x, y] = PALETTE["sand_low"]
                
    for _ in range(6):
        rx, ry = rng.randint(2, 29), rng.randint(2, 29)
        pixels[rx, ry] = PALETTE["crack_lip"]
        
    for _ in range(3):
        rx, ry = rng.randint(2, 29), rng.randint(2, 29)
        pixels[rx, ry] = PALETTE["clay_light"]

    return img

def enforce_tileable_edges(target_img, base_img):
    """Asegura que los bordes perimetrales (2px) coincidan con la base para tiling universal 100% limpio."""
    t_pix = target_img.load()
    b_pix = base_img.load()
    for i in range(32):
        for d in (0, 1):
            t_pix[d, i] = b_pix[d, i]
            t_pix[31 - d, i] = b_pix[31 - d, i]
            t_pix[i, d] = b_pix[i, d]
            t_pix[i, 31 - d] = b_pix[i, 31 - d]

# --- 1. FISURAS Y FRACTURAS PLANAS ---

def tile_t00_wasteland_plain():
    """T00: Tierra árida lisa continua (seamless toroidal)."""
    return create_toroidal_base(101)

def tile_t01_cracks_spiderweb():
    """T01: Fisuras finas radiales / tela de araña térmica a ras de suelo."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Centro en (15, 16) con 5 ramas radiales finas
    cx, cy = 15, 16
    branches = [
        [(cx, cy), (11, 12), (7, 10), (4, 9)],
        [(cx, cy), (18, 12), (22, 9), (26, 7)],
        [(cx, cy), (19, 20), (23, 23), (27, 24)],
        [(cx, cy), (13, 21), (10, 24), (7, 26)],
        [(cx, cy), (14, 9), (13, 5)]
    ]
    for path in branches:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["crack_dark"]
                    if y > 2: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t02_cracks_zigzag():
    """T02: Fractura angulada en zigzag con micro-bifurcaciones."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    zigzag = [
        [(5, 6), (11, 10), (9, 16), (17, 18), (15, 24), (22, 26), (26, 25)],
        [(9, 16), (5, 20), (3, 23)],
        [(17, 18), (23, 16), (27, 14)]
    ]
    for path in zigzag:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["crack_dark"]
                    if x + 1 < 30: pixels[x + 1, y] = PALETTE["crack_soft"]
                    if y > 2: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t03_cracks_parallel():
    """T03: Grietas tectónicas paralelas de sedimentación seca."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    tracks = [
        [(4, 10), (10, 9), (16, 11), (22, 10), (27, 9)],
        [(5, 17), (12, 16), (18, 18), (24, 17), (28, 18)],
        [(8, 23), (14, 24), (20, 23), (25, 25)]
    ]
    for path in tracks:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["crack_dark"]
                    if y > 2: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t04_cracks_clay_tiles():
    """T04: Cuarteamiento en mosaico de barro seco por evaporación (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    polygons = [
        [(4, 8), (12, 7), (18, 10), (24, 8), (28, 11)],
        [(12, 7), (11, 15), (7, 21), (5, 27)],
        [(18, 10), (17, 18), (22, 22), (26, 25)],
        [(11, 15), (17, 18)],
        [(7, 21), (14, 23), (17, 18)],
        [(14, 23), (15, 28)]
    ]
    for seg in polygons:
        for i in range(len(seg) - 1):
            x0, y0 = seg[i]
            x1, y1 = seg[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["crack_dark"]
                    if y > 2: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t05_fissure_wide_flat():
    """T05: Fisura ancha pero plana con lecho de micro-grava en el fondo."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Fisura diagonal de (4, 26) a (27, 5)
    for t_step in range(36):
        t = t_step / 35.0
        cx = int(4 + (27 - 4) * t)
        cy = int(26 + (5 - 26) * t)
        for offset in (-1, 0, 1):
            x, y = cx + offset, cy
            if 2 <= x < 30 and 2 <= y < 30:
                if offset == 0:
                    pixels[x, y] = PALETTE["crack_dark"]
                else:
                    pixels[x, y] = PALETTE["crack_soft"] if t_step % 2 == 0 else PALETTE["clay_light"]
        if 2 <= cx < 30 and 2 <= cy - 1 < 30:
            pixels[cx, cy - 1] = PALETTE["crack_lip"]
            
    enforce_tileable_edges(img, base)
    return img

def tile_t06_fissure_starburst():
    """T06: Fractura de descompresión expansiva en estrella central."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 16, 16
    rays = [
        [(cx, cy), (16, 5)],
        [(cx, cy), (26, 9)],
        [(cx, cy), (27, 21)],
        [(cx, cy), (19, 27)],
        [(cx, cy), (8, 25)],
        [(cx, cy), (5, 14)]
    ]
    for path in rays:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["crack_dark"]
                    if y > 2: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    enforce_tileable_edges(img, base)
    return img

# --- 2. VEGETACIÓN ÁRIDA (A RAS DE SUELO, TRANSITABLE) ---

def tile_t07_scrub_single():
    """T07: Matorral seco desértico individual a nivel de suelo (arbusto rastrero)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Arbusto en (15, 16) de radio 5 px
    cx, cy = 15, 16
    for y in range(cy - 5, cy + 6):
        for x in range(cx - 6, cx + 7):
            d = math.hypot(x - cx, (y - cy) * 1.3)
            if d <= 5.2 and 2 <= x < 30 and 2 <= y < 30:
                if d > 4.2:
                    pixels[x, y] = PALETTE["veg_dry"]
                elif y < cy:
                    pixels[x, y] = PALETTE["veg_hi"] if (x + y) % 2 == 0 else PALETTE["veg_mid"]
                else:
                    pixels[x, y] = PALETTE["veg_mid"] if (x + y) % 2 == 0 else PALETTE["veg_dark"]
                    
    # Ramitas secas sobresaliendo
    for rx, ry in [(10, 14), (20, 15), (14, 10), (17, 21)]:
        if 2 <= rx < 30 and 2 <= ry < 30:
            pixels[rx, ry] = PALETTE["veg_branch"]
            
    enforce_tileable_edges(img, base)
    return img

def tile_t08_scrub_patch():
    """T08: Conjunto de matorrales y hierbas secas desérticas dispersas."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    bushes = [(10, 11, 4), (21, 14, 4), (14, 22, 5)]
    for cx, cy, rad in bushes:
        for y in range(cy - rad, cy + rad + 1):
            for x in range(cx - rad, cx + rad + 1):
                d = math.hypot(x - cx, (y - cy) * 1.2)
                if d <= rad and 2 <= x < 30 and 2 <= y < 30:
                    if d > rad - 1:
                        pixels[x, y] = PALETTE["veg_dry"]
                    elif y < cy:
                        pixels[x, y] = PALETTE["veg_hi"] if (x + y) % 2 == 0 else PALETTE["veg_mid"]
                    else:
                        pixels[x, y] = PALETTE["veg_mid"] if (x + y) % 2 == 0 else PALETTE["veg_dark"]
                        
    enforce_tileable_edges(img, base)
    return img

def tile_t09_dry_thorns():
    """T09: Raíces secas y zarzas espinosas rastreras a nivel de suelo."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    vines = [
        [(5, 12), (10, 15), (16, 14), (21, 18), (26, 16)],
        [(10, 15), (12, 22), (17, 25)],
        [(16, 14), (18, 9), (23, 7)]
    ]
    for path in vines:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["veg_branch"]
                    # Espinas
                    if s % 3 == 0 and y > 2:
                        pixels[x, y - 1] = PALETTE["veg_dry"]
                        
    enforce_tileable_edges(img, base)
    return img

# --- 3. CHARCOS DE AGUA Y LODO SALOBRE (PLANOS) ---

def tile_t10_puddle_shallow():
    """T10: Charco pequeño de agua estancada a ras de suelo con borde salino."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 16, 16
    for y in range(8, 24):
        for x in range(8, 24):
            d = math.hypot((x - cx) * 1.2, y - cy)
            if d <= 6.5 and 2 <= x < 30 and 2 <= y < 30:
                if d > 5.5:
                    pixels[x, y] = PALETTE["salt_rim"] # Cerco salino
                elif d > 4.5:
                    pixels[x, y] = PALETTE["mud_wet"] # Lodo
                elif d < 2.5 and y < cy:
                    pixels[x, y] = PALETTE["water_hi"] # Reflejo de cielo
                elif d < 3.5:
                    pixels[x, y] = PALETTE["water_mid"]
                else:
                    pixels[x, y] = PALETTE["water_deep"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t11_puddle_large():
    """T11: Charca de agua salobre más ancha con reflejos plomizos planos."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 16, 16
    for y in range(6, 26):
        for x in range(5, 27):
            d = ((x - cx) / 8.5)**2 + ((y - cy) / 6.0)**2
            if d <= 1.2 and 2 <= x < 30 and 2 <= y < 30:
                if d > 1.0:
                    pixels[x, y] = PALETTE["salt_rim"]
                elif d > 0.8:
                    pixels[x, y] = PALETTE["mud_wet"]
                elif d < 0.4 and (x + y) % 3 != 0:
                    pixels[x, y] = PALETTE["water_hi"]
                elif d < 0.7:
                    pixels[x, y] = PALETTE["water_mid"]
                else:
                    pixels[x, y] = PALETTE["water_deep"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t12_mud_depression():
    """T12: Depresión llana de fango húmedo/salobre oscuro."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 16, 16
    for y in range(7, 25):
        for x in range(7, 25):
            d = ((x - cx) / 7.0)**2 + ((y - cy) / 5.5)**2
            if d <= 1.1 and 2 <= x < 30 and 2 <= y < 30:
                if d > 0.9:
                    pixels[x, y] = PALETTE["clay_light"]
                elif d > 0.5:
                    pixels[x, y] = PALETTE["mud_wet"]
                else:
                    pixels[x, y] = PALETTE["crack_dark"] if (x + y) % 2 == 0 else PALETTE["mud_wet"]
                    
    enforce_tileable_edges(img, base)
    return img

# --- 4. MINERAL, GRAVILLA Y MARCAJE PLANO MECHANICUS ---

def tile_t13_gravel_pebbles():
    """T13: Esparcimiento de pequeñas piedras planas y gravilla transitable."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    pebbles = [
        (8, 8, 3, 2), (21, 10, 3, 2), (14, 18, 3, 2), (24, 22, 2, 2), (8, 23, 3, 2),
        (18, 7, 2, 2), (12, 13, 2, 2), (25, 15, 2, 2)
    ]
    for px, py, pw, ph in pebbles:
        for y in range(py, py + ph):
            for x in range(px, px + pw):
                if 2 <= x < 30 and 2 <= y < 30:
                    pixels[x, y] = PALETTE["pebble_mid"]
        if 2 <= px < 30 and 2 <= py < 30:
            pixels[px, py] = PALETTE["pebble_hi"]
            
    enforce_tileable_edges(img, base)
    return img

def tile_t14_mineral_salt_crust():
    """T14: Costras minerales de salitre / caliche blanco-amarillento de evaporación."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    crust_patches = [(11, 12, 5), (20, 18, 6)]
    for cx, cy, rad in crust_patches:
        for y in range(cy - rad, cy + rad + 1):
            for x in range(cx - rad, cx + rad + 1):
                d = math.hypot(x - cx, y - cy)
                if d <= rad and 2 <= x < 30 and 2 <= y < 30:
                    if d < rad * 0.5:
                        pixels[x, y] = PALETTE["caliche"]
                    elif (x + y) % 2 == 0:
                        pixels[x, y] = PALETTE["sand_top"]
                        
    enforce_tileable_edges(img, base)
    return img

def tile_t15_turret_marking_cross():
    """T15: Marcaje balístico Mechanicus a ras de suelo (plantilla/cruz pintada plana).
    Totalmente transitable, señaliza posición táctica de torreta sin elevar barreras."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Cruz / retícula central de 20x20 pintada al estarcido desgastado
    cx, cy = 16, 16
    # Línea horizontal
    for x in range(7, 26):
        if 2 <= x < 30:
            pixels[x, 15] = PALETTE["stencil_red"] if x % 2 == 0 else PALETTE["clay_light"]
            pixels[x, 16] = PALETTE["stencil_red"]
    # Línea vertical
    for y in range(7, 26):
        if 2 <= y < 30:
            pixels[15, y] = PALETTE["stencil_red"] if y % 2 == 0 else PALETTE["clay_light"]
            pixels[16, y] = PALETTE["stencil_red"]
            
    # Círculo guía discontinuo de radio 8
    for deg in range(0, 360, 15):
        rad = math.radians(deg)
        x = int(cx + math.cos(rad) * 8.5)
        y = int(cy + math.sin(rad) * 8.5)
        if 2 <= x < 30 and 2 <= y < 30 and deg % 30 == 0:
            pixels[x, y] = PALETTE["stencil_red"]
            
    # 4 Remaches planos en esquinas
    for rx, ry in [(8, 8), (23, 8), (8, 23), (23, 23)]:
        pixels[rx, ry] = PALETTE["pebble_hi"]
        pixels[rx + 1, ry + 1] = PALETTE["pebble_dark"]
        
    enforce_tileable_edges(img, base)
    return img


TILES = [
    ("T00_wasteland_plain", tile_t00_wasteland_plain),
    ("T01_cracks_spiderweb", tile_t01_cracks_spiderweb),
    ("T02_cracks_zigzag", tile_t02_cracks_zigzag),
    ("T03_cracks_parallel", tile_t03_cracks_parallel),
    ("T04_cracks_clay_tiles", tile_t04_cracks_clay_tiles),
    ("T05_fissure_wide_flat", tile_t05_fissure_wide_flat),
    ("T06_fissure_starburst", tile_t06_fissure_starburst),
    ("T07_scrub_single", tile_t07_scrub_single),
    ("T08_scrub_patch", tile_t08_scrub_patch),
    ("T09_dry_thorns", tile_t09_dry_thorns),
    ("T10_puddle_shallow", tile_t10_puddle_shallow),
    ("T11_puddle_large", tile_t11_puddle_large),
    ("T12_mud_depression", tile_t12_mud_depression),
    ("T13_gravel_pebbles", tile_t13_gravel_pebbles),
    ("T14_mineral_salt_crust", tile_t14_mineral_salt_crust),
    ("T15_turret_marking_cross", tile_t15_turret_marking_cross),
]

def main():
    generated_images = {}
    
    print("--- Generando 16 tiles modulares 32x32 para Yermo Balístico (v4 - Plano y Orgánico) ---")
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

    # Mockup escénico de ARENA PLANA (256x192) centrada en la defensa
    mockup_map = [
        ["T13_gravel_pebbles", "T00_wasteland_plain", "T01_cracks_spiderweb", "T07_scrub_single", "T00_wasteland_plain", "T10_puddle_shallow", "T00_wasteland_plain", "T14_mineral_salt_crust"],
        ["T00_wasteland_plain", "T15_turret_marking_cross", "T00_wasteland_plain", "T03_cracks_parallel", "T00_wasteland_plain", "T00_wasteland_plain", "T15_turret_marking_cross", "T00_wasteland_plain"],
        ["T02_cracks_zigzag", "T00_wasteland_plain", "T08_scrub_patch", "T00_wasteland_plain", "T11_puddle_large", "T00_wasteland_plain", "T05_fissure_wide_flat", "T00_wasteland_plain"],
        ["T00_wasteland_plain", "T12_mud_depression", "T00_wasteland_plain", "T06_fissure_starburst", "T00_wasteland_plain", "T09_dry_thorns", "T00_wasteland_plain", "T04_cracks_clay_tiles"],
        ["T00_wasteland_plain", "T15_turret_marking_cross", "T00_wasteland_plain", "T00_wasteland_plain", "T10_puddle_shallow", "T15_turret_marking_cross", "T00_wasteland_plain", "T13_gravel_pebbles"],
        ["T14_mineral_salt_crust", "T00_wasteland_plain", "T07_scrub_single", "T00_wasteland_plain", "T00_wasteland_plain", "T00_wasteland_plain", "T08_scrub_patch", "T00_wasteland_plain"]
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
        scene_ds.paste(turret_frame, (1 * 32, 1 * 32), turret_frame)
        scene_ds.paste(turret_frame, (6 * 32, 1 * 32), turret_frame)
        scene_ds.paste(turret_frame, (1 * 32, 4 * 32), turret_frame)
        scene_ds.paste(turret_frame, (5 * 32, 4 * 32), turret_frame)

    enemy_t1_path = "assets/sprites/enemies/t1_ripper_strip_master_1x.png"
    enemy_t2_path = "assets/sprites/enemies/t2_hormagaunt_strip_master_1x.png"
    
    if os.path.exists(enemy_t1_path):
        t1_strip = Image.open(enemy_t1_path)
        t1_frame = t1_strip.crop((0, 0, 16, 16))
        # Enjambre caminando sobre el terreno plano, cruzando charcos y fisuras
        scene_ds.paste(t1_frame, (3 * 32 + 8, 0 * 32 + 4), t1_frame)
        scene_ds.paste(t1_frame, (4 * 32 + 4, 1 * 32 + 10), t1_frame)
        scene_ds.paste(t1_frame, (7 * 32 + 6, 1 * 32 + 8), t1_frame)
        scene_ds.paste(t1_frame, (4 * 32 + 10, 2 * 32 + 12), t1_frame) # Cruzando el charco
        scene_ds.paste(t1_frame, (6 * 32 + 8, 3 * 32 + 14), t1_frame)

    if os.path.exists(enemy_t2_path):
        t2_strip = Image.open(enemy_t2_path)
        t2_frame = t2_strip.crop((0, 0, 24, 24))
        scene_ds.paste(t2_frame, (2 * 32 + 4, 0 * 32 + 4), t2_frame)
        scene_ds.paste(t2_frame, (4 * 32 + 6, 3 * 32 + 4), t2_frame)
        scene_ds.paste(t2_frame, (3 * 32 + 4, 4 * 32 + 8), t2_frame)

    mockup_1x_path = os.path.join(OUTPUT_DIR, "wasteland_scene_mockup_1x.png")
    scene_ds.save(mockup_1x_path)
    mockup_3x_path = os.path.join(OUTPUT_DIR, "wasteland_scene_mockup_3x.png")
    scene_ds.resize((768, 576), Image.NEAREST).save(mockup_3x_path)
    print(f"Mockup escénico canónico de arena plana guardado en: {mockup_3x_path}")

if __name__ == "__main__":
    main()
