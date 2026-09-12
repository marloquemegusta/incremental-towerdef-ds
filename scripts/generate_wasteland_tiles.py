"""
Script de generación del tileset 32x32 de Tierra Árida / Yermo Balístico para Nintendo DS (v3 - Arena Abierta).
Filosofía de juego actual:
1. SIN CAMINOS NI CALZADAS: Mapa tipo arena de supervivencia abierta centrada en el búnker.
2. 100% PERIÓDICO Y TILEABLE: Todos los tiles enlazan sin costuras (seamless) entre sí y con el suelo base T00.
3. CONEXIONES MODULARES TECTÓNICAS: Fisuras continuas H y V con enlace seamless de extremo a extremo.
4. ALTO CONTRASTE: Tonos de suelo claros y luminosos (arena desecada, caliche) para máxima legibilidad del enjambre xenos.
"""

import os
import math
import random
from PIL import Image, ImageDraw

OUTPUT_DIR = "assets/tiles/wasteland"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Paleta luminosa calibrada para arena de supervivencia
# Cero púrpuras/violetas (R y B altos con G bajo) y cero blanco saturado (>230 en todos)
PALETTE = {
    # Cota alta / Suelo árido luminoso (tonos claros predominantes)
    "sand_top": (224, 212, 182),    # Resalte superior suave
    "sand_hi": (212, 194, 158),     # Tono base principal (claro, limpio)
    "sand_mid": (194, 174, 138),    # Variación suave
    "sand_low": (174, 152, 116),    # Matiz de transición
    "clay_light": (152, 128, 94),   # Matiz terroso suave
    
    # Grietas, sombras de fosa y biseles
    "shadow_deep": (44, 26, 13),    # Sombra de fisura
    "shadow_abyss": (25, 15, 8),    # Fondo oscuro
    "void": (11, 6, 3),             # Falla abismal

    # Rocas y grava basáltica
    "rock_hi": (132, 126, 116),     # Roca iluminada
    "rock_mid": (94, 88, 80),       # Cuerpo de roca
    "rock_dark": (60, 54, 46),      # Sombra propia de roca
    
    # Metal oxidado / Mechanicus
    "rust_hi": (148, 82, 45),
    "rust_dark": (98, 50, 24),
    "steel_hi": (125, 130, 135),
    "steel_mid": (85, 90, 95),
    "steel_dark": (48, 52, 56),

    # Caliche / polvo mineral seco (resalte)
    "dust_crust": (228, 218, 194),
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

def create_toroidal_base(rng_seed=42):
    """Crea una base lisa y clara de 32x32 estrictamente toroidal y seamless (tileable en las 4 direcciones)."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_hi"])
    pixels = img.load()
    rng = random.Random(rng_seed)
    
    # Ruido continuo toroidal suave (sin rombos ni ortogonalidades simétricas)
    for y in range(32):
        for x in range(32):
            nx = (x / 32.0) * 2 * math.pi
            ny = (y / 32.0) * 2 * math.pi
            # Mezcla asimétrica suave toroidal
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
                
    # Motas sutiles que no tocan bordes para no romper continuidad
    for _ in range(6):
        rx, ry = rng.randint(2, 29), rng.randint(2, 29)
        pixels[rx, ry] = PALETTE["dust_crust"]
        
    for _ in range(3):
        rx, ry = rng.randint(2, 29), rng.randint(2, 29)
        pixels[rx, ry] = PALETTE["clay_light"]

    return img

def enforce_tileable_edges(target_img, base_img):
    """Asegura que los bordes perimetrales (ancho 2px) coincidan con la base para tiling universal 100% limpio."""
    t_pix = target_img.load()
    b_pix = base_img.load()
    for i in range(32):
        # Esquinas y bordes x=0, x=31, y=0, y=31
        for d in (0, 1):
            t_pix[d, i] = b_pix[d, i]
            t_pix[31 - d, i] = b_pix[31 - d, i]
            t_pix[i, d] = b_pix[i, d]
            t_pix[i, 31 - d] = b_pix[i, 31 - d]

def tile_t00_wasteland_plain():
    """T00: Tierra árida lisa continua (seamless toroidal)."""
    return create_toroidal_base(101)

def tile_t01_cracks_light():
    """T01: Suelo árido con fisuras térmicas finas aisladas (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    cracks = [
        [(7, 8), (11, 10), (16, 12), (19, 16), (23, 18), (25, 22)],
        [(16, 12), (20, 11), (24, 12)],
        [(11, 10), (10, 16), (8, 20)]
    ]
    for path in cracks:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                cx = int(x0 + (x1 - x0) * t)
                cy = int(y0 + (y1 - y0) * t)
                if 2 <= cx < 30 and 2 <= cy < 30:
                    pixels[cx, cy] = PALETTE["shadow_deep"]
                    if cy > 2: pixels[cx, cy - 1] = PALETTE["dust_crust"]
    enforce_tileable_edges(img, base)
    return img

def tile_t02_cracks_medium():
    """T02: Suelo árido con fisuras intermedias ramificadas (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    fissures = [
        [(4, 14), (8, 12), (14, 15), (20, 13), (25, 17), (28, 16)],
        [(14, 15), (15, 21), (13, 26)],
        [(20, 13), (22, 8), (26, 6)]
    ]
    for path in fissures:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                cx = int(x0 + (x1 - x0) * t)
                cy = int(y0 + (y1 - y0) * t)
                if 2 <= cx < 30 and 2 <= cy < 30:
                    pixels[cx, cy] = PALETTE["shadow_deep"]
                    if cy + 1 < 30: pixels[cx, cy + 1] = PALETTE["shadow_abyss"]
                    if cy > 2: pixels[cx, cy - 1] = PALETTE["dust_crust"]
    enforce_tileable_edges(img, base)
    return img

def tile_t03_cracks_dense():
    """T03: Cuarteamiento denso poligonal de barro reseco (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    segments = [
        [(5, 7), (12, 8), (17, 6), (24, 8), (27, 12)],
        [(12, 8), (13, 16), (9, 21), (8, 26)],
        [(17, 6), (18, 14), (23, 18), (27, 22)],
        [(13, 16), (19, 18), (16, 25)],
        [(9, 21), (16, 25), (22, 26)]
    ]
    for seg in segments:
        for i in range(len(seg) - 1):
            x0, y0 = seg[i]
            x1, y1 = seg[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                cx = int(x0 + (x1 - x0) * t)
                cy = int(y0 + (y1 - y0) * t)
                if 2 <= cx < 30 and 2 <= cy < 30:
                    pixels[cx, cy] = PALETTE["shadow_deep"]
                    if cy > 2: pixels[cx, cy - 1] = PALETTE["dust_crust"]
    enforce_tileable_edges(img, base)
    return img

def tile_t04_fissure_h():
    """T04: Gran fractura tectónica horizontal continua (seamless en X: entra en Y=16 y sale en Y=16)."""
    img = create_toroidal_base(101)
    pixels = img.load()
    
    # Puntos de la fisura con entrada en (0, 16) y salida en (31, 16)
    pts = [(0, 16), (5, 14), (11, 17), (16, 15), (22, 18), (27, 15), (31, 16)]
    for i in range(len(pts) - 1):
        x0, y0 = pts[i]
        x1, y1 = pts[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            cx = int(x0 + (x1 - x0) * t)
            cy = int(y0 + (y1 - y0) * t)
            if 0 <= cx < 32 and 0 <= cy < 32:
                pixels[cx, cy] = PALETTE["void"]
                if cy + 1 < 32: pixels[cx, cy + 1] = PALETTE["shadow_abyss"]
                if cy + 2 < 32: pixels[cx, cy + 2] = PALETTE["shadow_deep"]
                if cy > 0: pixels[cx, cy - 1] = PALETTE["dust_crust"]
                if cy > 1: pixels[cx, cy - 2] = PALETTE["sand_top"]
    return img

def tile_t05_fissure_v():
    """T05: Gran fractura tectónica vertical continua (seamless en Y: entra en X=16 y sale en X=16)."""
    img = create_toroidal_base(101)
    pixels = img.load()
    
    pts = [(16, 0), (14, 5), (17, 11), (15, 16), (18, 22), (15, 27), (16, 31)]
    for i in range(len(pts) - 1):
        x0, y0 = pts[i]
        x1, y1 = pts[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            cx = int(x0 + (x1 - x0) * t)
            cy = int(y0 + (y1 - y0) * t)
            if 0 <= cx < 32 and 0 <= cy < 32:
                pixels[cx, cy] = PALETTE["void"]
                if cx + 1 < 32: pixels[cx + 1, cy] = PALETTE["shadow_abyss"]
                if cx + 2 < 32: pixels[cx + 2, cy] = PALETTE["shadow_deep"]
                if cx > 0: pixels[cx - 1, cy] = PALETTE["dust_crust"]
                if cx > 1: pixels[cx - 2, cy] = PALETTE["sand_top"]
    return img

def tile_t06_fissure_cross():
    """T06: Cruce tectónico 4 vías (conecta con T04 en horizontal y con T05 en vertical)."""
    img = create_toroidal_base(101)
    pixels = img.load()
    
    branches = [
        [(0, 16), (8, 15), (16, 16)],
        [(31, 16), (24, 17), (16, 16)],
        [(16, 0), (15, 8), (16, 16)],
        [(16, 31), (17, 24), (16, 16)]
    ]
    for branch in branches:
        for i in range(len(branch) - 1):
            x0, y0 = branch[i]
            x1, y1 = branch[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                cx = int(x0 + (x1 - x0) * t)
                cy = int(y0 + (y1 - y0) * t)
                if 0 <= cx < 32 and 0 <= cy < 32:
                    pixels[cx, cy] = PALETTE["void"]
                    if cy + 1 < 32: pixels[cx, cy + 1] = PALETTE["shadow_abyss"]
                    if cy > 0: pixels[cx, cy - 1] = PALETTE["dust_crust"]
    return img

def tile_t07_fissure_abyss():
    """T07: Falla tectónica con fosa central oscura e insondable (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Abismo central elíptico
    for y in range(8, 24):
        for x in range(6, 26):
            d = ((x - 16) / 8.0)**2 + ((y - 16) / 5.0)**2
            if d <= 1.0:
                if d < 0.4:
                    pixels[x, y] = PALETTE["void"]
                elif d < 0.75:
                    pixels[x, y] = PALETTE["shadow_abyss"]
                else:
                    pixels[x, y] = PALETTE["shadow_deep"]
            elif d <= 1.4:
                if y < 16:
                    pixels[x, y] = PALETTE["dust_crust"]
                else:
                    pixels[x, y] = PALETTE["clay_light"]
    enforce_tileable_edges(img, base)
    return img

def tile_t08_crater_large():
    """T08: Gran cráter de impacto de artillería de 20 px con reborde claro (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 16, 16
    for y in range(32):
        for x in range(32):
            d = math.hypot(x - cx, y - cy)
            if d <= 10.0:
                if d < 4.0:
                    pixels[x, y] = PALETTE["void"]
                elif d < 7.0:
                    pixels[x, y] = PALETTE["shadow_abyss"]
                elif d < 9.0:
                    pixels[x, y] = PALETTE["shadow_deep"]
                else:
                    if y < cy:
                        pixels[x, y] = PALETTE["dust_crust"]
                    else:
                        pixels[x, y] = PALETTE["clay_light"]
            elif 10.0 < d <= 12.5 and y < cy:
                pixels[x, y] = PALETTE["sand_top"]
    enforce_tileable_edges(img, base)
    return img

def tile_t09_craters_cluster():
    """T09: Grupo de tres pequeños impactos de proyectiles de metralla (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    craters = [(9, 10, 4), (22, 13, 5), (14, 23, 4)]
    for cx, cy, rad in craters:
        for y in range(cy - rad, cy + rad + 1):
            for x in range(cx - rad, cx + rad + 1):
                d = math.hypot(x - cx, y - cy)
                if d <= rad and 0 <= x < 32 and 0 <= y < 32:
                    if d < rad * 0.4:
                        pixels[x, y] = PALETTE["void"]
                    elif d < rad * 0.75:
                        pixels[x, y] = PALETTE["shadow_abyss"]
                    else:
                        pixels[x, y] = PALETTE["shadow_deep"]
                elif rad < d <= rad + 1.5 and y < cy and 0 <= x < 32 and 0 <= y < 32:
                    pixels[x, y] = PALETTE["dust_crust"]
    enforce_tileable_edges(img, base)
    return img

def tile_t10_rocks_scatter():
    """T10: Esparcimiento de grava y piedras basálticas sobre arena clara (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    rocks = [
        (8, 8, 4, 3), (21, 10, 5, 4), (13, 19, 4, 3), (24, 22, 3, 3), (7, 23, 4, 3)
    ]
    for rx, ry, rw, rh in rocks:
        # Sombra
        for y in range(ry + 1, ry + rh + 2):
            for x in range(rx + 1, rx + rw + 2):
                if 0 <= x < 32 and 0 <= y < 32:
                    pixels[x, y] = PALETTE["shadow_deep"]
        # Roca
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
    enforce_tileable_edges(img, base)
    return img

def tile_t11_boulders_central():
    """T11: Gran formación de peñascos monolíticos centrales (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    for y in range(9, 27):
        for x in range(12, 28):
            if (x - 19)**2 + (y - 19)**2 < 65:
                pixels[x, y] = PALETTE["shadow_deep"]
                
    for y in range(6, 26):
        for x in range(6, 26):
            dx = (x - 15) / 8.5
            dy = (y - 15) / 8.5
            if dx*dx + dy*dy <= 1.0:
                if dx + dy < -0.3:
                    pixels[x, y] = PALETTE["rock_hi"]
                elif dx + dy < 0.3:
                    pixels[x, y] = PALETTE["rock_mid"]
                else:
                    pixels[x, y] = PALETTE["rock_dark"]
                    
    for y in range(11, 20):
        pixels[15 + (y % 2), y] = PALETTE["shadow_abyss"]
        
    enforce_tileable_edges(img, base)
    return img

def tile_t12_dust_dune():
    """T12: Ondulación suave de duna eólica con cresta iluminada (seamless en X)."""
    img = create_toroidal_base(101)
    pixels = img.load()
    
    # Cresta ondulada suave transversal
    for x in range(32):
        nx = (x / 32.0) * 2 * math.pi
        crest_y = int(15 + math.sin(nx) * 5)
        # Iluminación norte de la duna
        for dy in range(-4, 0):
            cy = crest_y + dy
            if 0 <= cy < 32:
                pixels[x, cy] = PALETTE["sand_top"]
        # Cresta
        pixels[x, crest_y] = PALETTE["dust_crust"]
        # Sombra suave de sotavento al sur
        for dy in range(1, 4):
            cy = crest_y + dy
            if 0 <= cy < 32:
                pixels[x, cy] = PALETTE["sand_low"]
    return img

def tile_t13_turret_pad_plate():
    """T13: Plataforma de anclaje Mechanicus para torretas 32x32 (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Base metálica octogonal contenida (26x26) centrada
    for y in range(3, 29):
        for x in range(3, 29):
            if (x - 3) + (y - 3) < 4: continue
            if (28 - x) + (y - 3) < 4: continue
            if (x - 3) + (28 - y) < 4: continue
            if (28 - x) + (28 - y) < 4: continue
            
            if (x in (3, 28) or y in (3, 28) or 
                (x - 3) + (y - 3) == 4 or (28 - x) + (y - 3) == 4 or
                (x - 3) + (28 - y) == 4 or (28 - x) + (28 - y) == 4):
                pixels[x, y] = PALETTE["steel_hi"] if y <= 16 else PALETTE["steel_dark"]
            else:
                if (x + y) % 3 == 0:
                    pixels[x, y] = PALETTE["rust_hi"]
                elif (x + y) % 5 == 0:
                    pixels[x, y] = PALETTE["rust_dark"]
                else:
                    pixels[x, y] = PALETTE["steel_mid"]
                    
    # Sombra de la plataforma
    for x in range(5, 29):
        pixels[x, 29] = PALETTE["shadow_deep"]
    for y in range(5, 29):
        pixels[29, y] = PALETTE["shadow_deep"]

    # Remaches
    for rx, ry in [(6, 5), (25, 5), (5, 6), (26, 6), (5, 25), (26, 25), (6, 26), (25, 26)]:
        pixels[rx, ry] = PALETTE["steel_hi"]
        pixels[rx + 1, ry + 1] = PALETTE["steel_dark"]

    # Anilla central
    for y in range(11, 21):
        for x in range(11, 21):
            d = math.hypot(x - 15.5, y - 15.5)
            if d < 4.8:
                if d > 3.6:
                    pixels[x, y] = PALETTE["steel_hi"] if y < 16 else PALETTE["steel_dark"]
                elif int(d) % 2 == 0:
                    pixels[x, y] = PALETTE["void"]
                else:
                    pixels[x, y] = PALETTE["steel_mid"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t14_conduit_plate():
    """T14: Conducción/placa blindada Mechanicus a ras de suelo para energía (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Placa rectangular centrada (24x16 en Y: 8..23)
    for y in range(8, 24):
        for x in range(4, 28):
            if x in (4, 27) or y in (8, 23):
                pixels[x, y] = PALETTE["steel_hi"] if y == 8 or x == 4 else PALETTE["steel_dark"]
            else:
                # Rejillas de ventilación
                if (x % 3 == 0):
                    pixels[x, y] = PALETTE["void"]
                else:
                    pixels[x, y] = PALETTE["steel_mid"]
                    
    # Sombra
    for x in range(5, 28):
        pixels[x, 24] = PALETTE["shadow_deep"]
        
    enforce_tileable_edges(img, base)
    return img

def tile_t15_scorched_caliche():
    """T15: Zona calcinada por deflagración con costra de caliche/salitre (tileable)."""
    base = create_toroidal_base(101)
    img = base.copy()
    pixels = img.load()
    
    # Huella de calor irregular en el centro
    cx, cy = 16, 16
    for y in range(4, 28):
        for x in range(4, 28):
            d = math.hypot(x - cx, y - cy)
            if d <= 10.0:
                if d < 4.0:
                    pixels[x, y] = PALETTE["shadow_deep"]
                elif d < 7.0:
                    pixels[x, y] = PALETTE["clay_light"]
                elif d < 9.5:
                    pixels[x, y] = PALETTE["dust_crust"]
                else:
                    pixels[x, y] = PALETTE["sand_top"]
    enforce_tileable_edges(img, base)
    return img


TILES = [
    ("T00_wasteland_plain", tile_t00_wasteland_plain),
    ("T01_cracks_light", tile_t01_cracks_light),
    ("T02_cracks_medium", tile_t02_cracks_medium),
    ("T03_cracks_dense", tile_t03_cracks_dense),
    ("T04_fissure_h", tile_t04_fissure_h),
    ("T05_fissure_v", tile_t05_fissure_v),
    ("T06_fissure_cross", tile_t06_fissure_cross),
    ("T07_fissure_abyss", tile_t07_fissure_abyss),
    ("T08_crater_large", tile_t08_crater_large),
    ("T09_craters_cluster", tile_t09_craters_cluster),
    ("T10_rocks_scatter", tile_t10_rocks_scatter),
    ("T11_boulders_central", tile_t11_boulders_central),
    ("T12_dust_dune", tile_t12_dust_dune),
    ("T13_turret_pad_plate", tile_t13_turret_pad_plate),
    ("T14_conduit_plate", tile_t14_conduit_plate),
    ("T15_scorched_caliche", tile_t15_scorched_caliche),
]

def main():
    generated_images = {}
    
    print("--- Generando 16 tiles modulares 32x32 para Yermo Balístico (v3 - Arena Abierta) ---")
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

    # Mockup escénico canónico de ARENA ABIERTA (256x192) centrada en el búnker / defensa
    # Sin caminos fijos: enjambre asediando desde todas las direcciones en campo abierto
    mockup_map = [
        ["T10_rocks_scatter", "T00_wasteland_plain", "T01_cracks_light", "T05_fissure_v", "T00_wasteland_plain", "T08_crater_large", "T00_wasteland_plain", "T10_rocks_scatter"],
        ["T00_wasteland_plain", "T13_turret_pad_plate", "T00_wasteland_plain", "T05_fissure_v", "T00_wasteland_plain", "T00_wasteland_plain", "T13_turret_pad_plate", "T00_wasteland_plain"],
        ["T04_fissure_h", "T04_fissure_h", "T04_fissure_h", "T06_fissure_cross", "T04_fissure_h", "T04_fissure_h", "T04_fissure_h", "T04_fissure_h"],
        ["T00_wasteland_plain", "T11_boulders_central", "T00_wasteland_plain", "T05_fissure_v", "T14_conduit_plate", "T03_cracks_dense", "T00_wasteland_plain", "T07_fissure_abyss"],
        ["T00_wasteland_plain", "T13_turret_pad_plate", "T02_cracks_medium", "T05_fissure_v", "T00_wasteland_plain", "T13_turret_pad_plate", "T00_wasteland_plain", "T12_dust_dune"],
        ["T09_craters_cluster", "T00_wasteland_plain", "T15_scorched_caliche", "T05_fissure_v", "T00_wasteland_plain", "T00_wasteland_plain", "T10_rocks_scatter", "T00_wasteland_plain"]
    ]

    scene_ds = Image.new("RGBA", (256, 192), (0, 0, 0, 255))
    for r in range(6):
        for c in range(8):
            tile_key = mockup_map[r][c]
            scene_ds.paste(generated_images[tile_key], (c * 32, r * 32))

    # Torretas Heavy Bolter en los pads
    turret_path = "assets/sprites/turrets/heavy_bolter_strip_master_1x.png"
    if os.path.exists(turret_path):
        turret_strip = Image.open(turret_path)
        turret_frame = turret_strip.crop((0, 0, 32, 32))
        scene_ds.paste(turret_frame, (1 * 32, 1 * 32), turret_frame)
        scene_ds.paste(turret_frame, (6 * 32, 1 * 32), turret_frame)
        scene_ds.paste(turret_frame, (1 * 32, 4 * 32), turret_frame)
        scene_ds.paste(turret_frame, (5 * 32, 4 * 32), turret_frame)

    # Enjambre asediando desde múltiples ángulos en arena abierta
    enemy_t1_path = "assets/sprites/enemies/t1_ripper_strip_master_1x.png"
    enemy_t2_path = "assets/sprites/enemies/t2_hormagaunt_strip_master_1x.png"
    
    if os.path.exists(enemy_t1_path):
        t1_strip = Image.open(enemy_t1_path)
        t1_frame = t1_strip.crop((0, 0, 16, 16))
        # Horda T1 desde el norte y este
        scene_ds.paste(t1_frame, (3 * 32 + 8, 0 * 32 + 4), t1_frame)
        scene_ds.paste(t1_frame, (4 * 32 + 4, 1 * 32 + 10), t1_frame)
        scene_ds.paste(t1_frame, (7 * 32 + 6, 1 * 32 + 8), t1_frame)
        scene_ds.paste(t1_frame, (6 * 32 + 8, 3 * 32 + 14), t1_frame)

    if os.path.exists(enemy_t2_path):
        t2_strip = Image.open(enemy_t2_path)
        t2_frame = t2_strip.crop((0, 0, 24, 24))
        # Bestias T2 asaltando campo abierto
        scene_ds.paste(t2_frame, (2 * 32 + 4, 0 * 32 + 4), t2_frame)
        scene_ds.paste(t2_frame, (4 * 32 + 6, 3 * 32 + 4), t2_frame)
        scene_ds.paste(t2_frame, (3 * 32 + 4, 4 * 32 + 8), t2_frame)

    mockup_1x_path = os.path.join(OUTPUT_DIR, "wasteland_scene_mockup_1x.png")
    scene_ds.save(mockup_1x_path)
    mockup_3x_path = os.path.join(OUTPUT_DIR, "wasteland_scene_mockup_3x.png")
    scene_ds.resize((768, 576), Image.NEAREST).save(mockup_3x_path)
    print(f"Mockup escénico canónico de arena abierta guardado en: {mockup_3x_path}")

if __name__ == "__main__":
    main()
