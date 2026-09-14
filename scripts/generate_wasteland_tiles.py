"""
Script de generación del tileset 32x32 de Tierra Árida / Yermo Balístico para Nintendo DS (v5 - Continuo, Asimétrico y Orgánico).
Optimizaciones basadas en feedback del usuario:
1. ELIMINADA LA MANCHA CENTRAL: Suelo base 100% uniforme, claro e isotrópico. Se acabó la rejilla visible por puntos centrales.
2. ELEMENTOS ASIMÉTRICOS Y DESCENTRADOS: Detalles ubicados en cuadrantes alternos (NO, SE, esquinas, bordes) para romper toda sensación de cuadrícula repetitiva.
3. MICRO-GRIETAS INTEGRADAS EN EL SUSTRATO: Todo el bioma tiene sutiles fisuras naturales de resecamiento solar continuo en la tierra base, evitando la sensación de "grietas aisladas en unos pocos tiles".
4. RED DE FISURAS MODULARES CONECTABLES: Las fracturas mayores cruzan de tile a tile conectando en anclajes fijos (X=16 o Y=16) para formar una telaraña continua en el mapa.
5. VEGETACIÓN Y CHARCOS INTEGRADOS: Los charcos y matas brotan directamente del barro cuarteado de forma creíble.
"""

import os
import math
import random
from PIL import Image, ImageDraw

OUTPUT_DIR = "assets/tiles/wasteland"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Paleta luminosa y natural auditada (Regla 8: Cero púrpuras, cero blanco hueso > 230)
PALETTE = {
    # Suelo árido base luminoso y uniforme
    "sand_top": (224, 212, 184),    # Resalte suave de polvo
    "sand_hi": (212, 194, 160),     # Tono principal de arena desecada
    "sand_mid": (196, 176, 142),    # Matiz suave
    "sand_low": (178, 156, 122),    # Tono cálido de transición
    "clay_light": (156, 132, 98),   # Tierra arcillosa suave
    
    # Grietas y fisuras (desde micro-grietas sutiles hasta fallas oscuras)
    "micro_crack": (160, 138, 106), # Micro-fisura de sustrato (muy sutil)
    "crack_soft": (96, 68, 42),     # Grieta media
    "crack_dark": (48, 30, 16),     # Fisura profunda a ras de suelo
    "crack_lip": (228, 218, 196),   # Borde de caliche iluminado

    # Vegetación árida desértica (matorral seco, verde-oliva marchito y ramas)
    "veg_hi": (124, 134, 80),       # Hoja seca iluminada
    "veg_mid": (92, 100, 56),       # Cuerpo de arbusto
    "veg_dark": (60, 66, 38),       # Base de planta a ras de suelo
    "veg_dry": (152, 138, 90),      # Paja / espino seco
    "veg_branch": (78, 64, 44),     # Ramita seca

    # Charcos de agua salobre / lodo (plano, azul pizarra / gris plomizo)
    "water_hi": (94, 130, 144),     # Brillo / reflejo de cielo plomizo
    "water_mid": (64, 94, 108),     # Cuerpo de agua estancada
    "water_deep": (44, 66, 78),     # Fondo turbio
    "mud_wet": (76, 64, 52),        # Lodo húmedo perimetral
    "salt_rim": (208, 214, 200),    # Cerco de evaporación salina

    # Minerales, gravilla y marcaje Mechanicus
    "pebble_hi": (138, 132, 122),
    "pebble_mid": (100, 94, 86),
    "pebble_dark": (66, 60, 54),
    "stencil_red": (148, 64, 44),   # Pintura desgastada Mechanicus
    "caliche": (228, 220, 200),
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

def create_uniform_substrate(rng_seed=42):
    """Crea una base isotrópica, suave y clara de suelo árido con micro-fisuras toroidales continuas.
    NO TIENE NINGUNA MANCHA CENTRAL. Es perfectamente homogénea y seamless en las 4 direcciones."""
    img = Image.new("RGBA", (32, 32), PALETTE["sand_hi"])
    pixels = img.load()
    rng = random.Random(rng_seed)
    
    # Ruido continuo suave de frecuencias altas distribuido uniformemente (sin senos centrados)
    for y in range(32):
        for x in range(32):
            # Armónicos múltiples desfasados para máxima isotropía
            val = rng.uniform(-0.18, 0.18)
            val += math.sin((x + y) * 0.4) * 0.08
            val += math.cos((x - y) * 0.5) * 0.08
            
            if val > 0.16:
                pixels[x, y] = PALETTE["sand_top"]
            elif val > -0.08:
                pixels[x, y] = PALETTE["sand_hi"]
            elif val > -0.22:
                pixels[x, y] = PALETTE["sand_mid"]
            else:
                pixels[x, y] = PALETTE["sand_low"]
                
    # Red de micro-fisuras de desecación continua en el sustrato base (seamless toroidal)
    # Líneas muy tenues y orgánicas que cruzan de borde a borde para unificar todo el bioma
    micro_lines = [
        # Cruza horizontalmente conectando X=0 con X=31 en Y=10
        [(0, 10), (7, 12), (15, 10), (23, 11), (31, 10)],
        # Rama descendente hacia Y=31 en X=20
        [(15, 10), (18, 18), (20, 25), (20, 31)],
        # Rama que sube a Y=0 en X=7
        [(7, 12), (6, 5), (7, 0)]
    ]
    for path in micro_lines:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t) % 32
                y = int(y0 + (y1 - y0) * t) % 32
                pixels[x, y] = PALETTE["micro_crack"]

    return img

def enforce_tileable_edges(target_img, base_img):
    """Asegura que los bordes perimetrales (1px) coincidan para garantizar un tiling universal sin costuras."""
    t_pix = target_img.load()
    b_pix = base_img.load()
    for i in range(32):
        t_pix[0, i] = b_pix[0, i]
        t_pix[31, i] = b_pix[31, i]
        t_pix[i, 0] = b_pix[i, 0]
        t_pix[i, 31] = b_pix[i, 31]

# --- BLOQUE 1: SUSTRATO BASE Y MICRO-VARIACIONES (4 TILES) ---

def tile_t00_wasteland_plain():
    """T00: Suelo árido base claro con micro-fisuras toroidales continuas uniformes."""
    return create_uniform_substrate(101)

def tile_t01_ground_hairline_cracks():
    """T01: Suelo con fisuras secundarias en abanico asimétrico (cuadrante Este)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    cracks = [
        [(14, 10), (19, 13), (25, 12), (29, 15)],
        [(19, 13), (22, 20), (27, 22)],
        [(19, 13), (17, 18), (14, 23)]
    ]
    for path in cracks:
        for i in range(len(path) - 1):
            x0, y0 = path[i]
            x1, y1 = path[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 1 <= x < 31 and 1 <= y < 31:
                    pixels[x, y] = PALETTE["crack_soft"]
                    if y > 1: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t02_ground_pebble_dust():
    """T02: Salpicado sutil de gravilla basáltica y motas de caliche (disperso asimétrico)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    pebbles = [(5, 7), (8, 22), (23, 8), (26, 25), (12, 17), (28, 14)]
    for px, py in pebbles:
        if 1 <= px < 30 and 1 <= py < 30:
            pixels[px, py] = PALETTE["pebble_mid"]
            pixels[px + 1, py] = PALETTE["pebble_hi"]
            pixels[px, py + 1] = PALETTE["pebble_dark"]
            
    enforce_tileable_edges(img, base)
    return img

def tile_t03_ground_caliche_crust():
    """T03: Costras llanas de salitre / caliche mineral en el cuadrante Noroeste."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    for y in range(4, 15):
        for x in range(4, 16):
            if ((x - 9) / 4.5)**2 + ((y - 9) / 3.5)**2 <= 1.0:
                pixels[x, y] = PALETTE["caliche"] if (x + y) % 2 == 0 else PALETTE["sand_top"]
                
    enforce_tileable_edges(img, base)
    return img

# --- BLOQUE 2: RED DE FISURAS MODULARES CONECTABLES (4 TILES) ---

def tile_t04_fissure_pass_h():
    """T04: Fractura tectónica horizontal continua (conecta en (0, 16) y (31, 16))."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    pts = [(0, 16), (6, 14), (13, 17), (20, 15), (26, 18), (31, 16)]
    for i in range(len(pts) - 1):
        x0, y0 = pts[i]
        x1, y1 = pts[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            x = int(x0 + (x1 - x0) * t)
            y = int(y0 + (y1 - y0) * t)
            if 0 <= x < 32 and 0 <= y < 32:
                pixels[x, y] = PALETTE["crack_dark"]
                if y + 1 < 32: pixels[x, y + 1] = PALETTE["crack_soft"]
                if y > 0: pixels[x, y - 1] = PALETTE["crack_lip"]
                
    return img

def tile_t05_fissure_pass_v():
    """T05: Fractura tectónica vertical continua (conecta en (16, 0) y (16, 31))."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    pts = [(16, 0), (14, 7), (18, 14), (15, 21), (17, 26), (16, 31)]
    for i in range(len(pts) - 1):
        x0, y0 = pts[i]
        x1, y1 = pts[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            x = int(x0 + (x1 - x0) * t)
            y = int(y0 + (y1 - y0) * t)
            if 0 <= x < 32 and 0 <= y < 32:
                pixels[x, y] = PALETTE["crack_dark"]
                if x + 1 < 32: pixels[x + 1, y] = PALETTE["crack_soft"]
                if x > 0: pixels[x - 1, y] = PALETTE["crack_lip"]
                
    return img

def tile_t06_fissure_corner_ne():
    """T06: Fractura en codo modular (conecta en Norte (16, 0) y Este (31, 16))."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    pts = [(16, 0), (15, 6), (17, 11), (21, 14), (26, 15), (31, 16)]
    for i in range(len(pts) - 1):
        x0, y0 = pts[i]
        x1, y1 = pts[i+1]
        dist = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(dist + 1):
            t = s / dist if dist > 0 else 0
            x = int(x0 + (x1 - x0) * t)
            y = int(y0 + (y1 - y0) * t)
            if 0 <= x < 32 and 0 <= y < 32:
                pixels[x, y] = PALETTE["crack_dark"]
                if y + 1 < 32: pixels[x, y + 1] = PALETTE["crack_soft"]
                if y > 0: pixels[x, y - 1] = PALETTE["crack_lip"]
                
    return img

def tile_t07_fissure_branch_t():
    """T07: Bifurcación en T (conecta Oeste (0, 16), Este (31, 16) y Sur (16, 31))."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    branches = [
        [(0, 16), (9, 15), (16, 16)],
        [(31, 16), (23, 17), (16, 16)],
        [(16, 16), (17, 23), (16, 31)]
    ]
    for branch in branches:
        for i in range(len(branch) - 1):
            x0, y0 = branch[i]
            x1, y1 = branch[i+1]
            dist = max(abs(x1 - x0), abs(y1 - y0))
            for s in range(dist + 1):
                t = s / dist if dist > 0 else 0
                x = int(x0 + (x1 - x0) * t)
                y = int(y0 + (y1 - y0) * t)
                if 0 <= x < 32 and 0 <= y < 32:
                    pixels[x, y] = PALETTE["crack_dark"]
                    if y > 0: pixels[x, y - 1] = PALETTE["crack_lip"]
                    
    return img

# --- BLOQUE 3: VEGETACIÓN ÁRIDA ASIMÉTRICA Y DESCENTRADOS (3 TILES) ---

def tile_t08_scrub_offset_nw():
    """T08: Arbusto árido rastrero descentrado en cuadrante Noroeste (9, 10)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 9, 10
    # Grieta local de donde brota la vegetación
    for x in range(4, 16):
        pixels[x, 10] = PALETTE["crack_soft"]
        
    for y in range(cy - 4, cy + 5):
        for x in range(cx - 5, cx + 6):
            d = math.hypot(x - cx, (y - cy) * 1.3)
            if d <= 4.5 and 1 <= x < 31 and 1 <= y < 31:
                if d > 3.5:
                    pixels[x, y] = PALETTE["veg_dry"]
                elif y < cy:
                    pixels[x, y] = PALETTE["veg_hi"] if (x + y) % 2 == 0 else PALETTE["veg_mid"]
                else:
                    pixels[x, y] = PALETTE["veg_mid"] if (x + y) % 2 == 0 else PALETTE["veg_dark"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t09_scrub_cluster_se():
    """T09: Grupo de dos pequeñas matas secas descentradas en cuadrante Sureste (22, 21)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    bushes = [(20, 21, 4), (26, 17, 3)]
    for cx, cy, rad in bushes:
        for y in range(cy - rad, cy + rad + 1):
            for x in range(cx - rad, cx + rad + 1):
                d = math.hypot(x - cx, (y - cy) * 1.2)
                if d <= rad and 1 <= x < 31 and 1 <= y < 31:
                    if d > rad - 1:
                        pixels[x, y] = PALETTE["veg_dry"]
                    elif y < cy:
                        pixels[x, y] = PALETTE["veg_hi"] if (x + y) % 2 == 0 else PALETTE["veg_mid"]
                    else:
                        pixels[x, y] = PALETTE["veg_mid"] if (x + y) % 2 == 0 else PALETTE["veg_dark"]
                        
    enforce_tileable_edges(img, base)
    return img

def tile_t10_dry_thorns_spread():
    """T10: Zarzas espinosas y raíces secas en diagonal SW-NE (descentrado)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    vines = [
        [(5, 25), (11, 22), (18, 16), (24, 11), (28, 7)],
        [(11, 22), (15, 27)],
        [(18, 16), (21, 21)]
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
                if 1 <= x < 31 and 1 <= y < 31:
                    pixels[x, y] = PALETTE["veg_branch"]
                    if s % 3 == 0 and y > 1:
                        pixels[x, y - 1] = PALETTE["veg_dry"]
                        
    enforce_tileable_edges(img, base)
    return img

# --- BLOQUE 4: CHARCOS DE AGUA Y FANGO EN SUELO AGRIETADO (3 TILES) ---

def tile_t11_puddle_cracked_mud():
    """T11: Charco de agua salobre descentrado hacia el Este (21, 15) con barro cuarteado alrededor."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 21, 15
    # Fisuras que alimentan el charco desde el oeste
    for x in range(6, 17):
        pixels[x, 15] = PALETTE["crack_soft"]
        
    for y in range(cy - 6, cy + 7):
        for x in range(cx - 7, cx + 8):
            d = math.hypot((x - cx) * 1.1, (y - cy) * 1.3)
            if d <= 6.5 and 1 <= x < 31 and 1 <= y < 31:
                if d > 5.5:
                    pixels[x, y] = PALETTE["salt_rim"]
                elif d > 4.2:
                    pixels[x, y] = PALETTE["mud_wet"]
                elif d < 2.5 and y < cy:
                    pixels[x, y] = PALETTE["water_hi"]
                elif d < 3.8:
                    pixels[x, y] = PALETTE["water_mid"]
                else:
                    pixels[x, y] = PALETTE["water_deep"]
                    
    enforce_tileable_edges(img, base)
    return img

def tile_t12_puddles_twin():
    """T12: Dos charcos pequeños asimétricos en esquinas opuestas (SO y NE)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    spots = [(9, 23, 4), (24, 8, 3)]
    for cx, cy, rad in spots:
        for y in range(cy - rad - 1, cy + rad + 2):
            for x in range(cx - rad - 1, cx + rad + 2):
                d = math.hypot(x - cx, y - cy)
                if d <= rad + 0.5 and 1 <= x < 31 and 1 <= y < 31:
                    if d > rad - 0.5:
                        pixels[x, y] = PALETTE["salt_rim"]
                    elif d > rad - 1.5:
                        pixels[x, y] = PALETTE["mud_wet"]
                    elif y < cy and d < 1.8:
                        pixels[x, y] = PALETTE["water_hi"]
                    else:
                        pixels[x, y] = PALETTE["water_mid"]
                        
    enforce_tileable_edges(img, base)
    return img

def tile_t13_mud_slick():
    """T13: Mancha plana de barro húmedo con micro-fisuras superficiales (descentrada al Oeste: 11, 16)."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 11, 16
    for y in range(8, 25):
        for x in range(4, 20):
            d = ((x - cx) / 6.0)**2 + ((y - cy) / 6.5)**2
            if d <= 1.0 and 1 <= x < 31 and 1 <= y < 31:
                if d > 0.8:
                    pixels[x, y] = PALETTE["clay_light"]
                elif (x + y) % 3 == 0:
                    pixels[x, y] = PALETTE["crack_soft"] # Grieta en el barro
                else:
                    pixels[x, y] = PALETTE["mud_wet"]
                    
    enforce_tileable_edges(img, base)
    return img

# --- BLOQUE 5: MARCAJES TÁCTICOS Y GRAVA (2 TILES) ---

def tile_t14_gravel_strip():
    """T14: Franja transversal difusa de gravilla plana basáltica."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    # Gravilla a lo largo de una diagonal no centrada
    rng = random.Random(501)
    for t_step in range(18):
        rx = rng.randint(3, 28)
        ry = (rx + rng.randint(-3, 3)) % 28 + 2
        if 1 <= rx < 30 and 1 <= ry < 30:
            pixels[rx, ry] = PALETTE["pebble_mid"]
            pixels[rx + 1, ry] = PALETTE["pebble_hi"]
            pixels[rx, ry + 1] = PALETTE["pebble_dark"]
            
    enforce_tileable_edges(img, base)
    return img

def tile_t15_turret_stencil_flat():
    """T15: Marcaje táctico Mechanicus estarcido plano a ras de suelo para anclaje de torreta."""
    base = create_uniform_substrate(101)
    img = base.copy()
    pixels = img.load()
    
    cx, cy = 16, 16
    # Retícula discontinua de 16x16
    for x in range(9, 24):
        if x % 2 == 0:
            pixels[x, 16] = PALETTE["stencil_red"]
    for y in range(9, 24):
        if y % 2 == 0:
            pixels[16, y] = PALETTE["stencil_red"]
            
    # Micro-remaches planos en 4 esquinas
    for rx, ry in [(9, 9), (23, 9), (9, 23), (23, 23)]:
        pixels[rx, ry] = PALETTE["pebble_hi"]
        pixels[rx + 1, ry + 1] = PALETTE["pebble_dark"]
        
    enforce_tileable_edges(img, base)
    return img


TILES = [
    ("T00_wasteland_plain", tile_t00_wasteland_plain),
    ("T01_ground_hairline_cracks", tile_t01_ground_hairline_cracks),
    ("T02_ground_pebble_dust", tile_t02_ground_pebble_dust),
    ("T03_ground_caliche_crust", tile_t03_ground_caliche_crust),
    ("T04_fissure_pass_h", tile_t04_fissure_pass_h),
    ("T05_fissure_pass_v", tile_t05_fissure_pass_v),
    ("T06_fissure_corner_ne", tile_t06_fissure_corner_ne),
    ("T07_fissure_branch_t", tile_t07_fissure_branch_t),
    ("T08_scrub_offset_nw", tile_t08_scrub_offset_nw),
    ("T09_scrub_cluster_se", tile_t09_scrub_cluster_se),
    ("T10_dry_thorns_spread", tile_t10_dry_thorns_spread),
    ("T11_puddle_cracked_mud", tile_t11_puddle_cracked_mud),
    ("T12_puddles_twin", tile_t12_puddles_twin),
    ("T13_mud_slick", tile_t13_mud_slick),
    ("T14_gravel_strip", tile_t14_gravel_strip),
    ("T15_turret_stencil_flat", tile_t15_turret_stencil_flat),
]

def main():
    generated_images = {}
    
    print("--- Generando 16 tiles modulares 32x32 para Yermo Balístico (v5 - Continuo y Asimétrico) ---")
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

    # Mockup escénico de ARENA CONTINUA ASIMÉTRICA (256x192)
    # Sin patrones de rejilla evidentes: detalles dispersos en cuadrantes alternos y fisuras conectadas
    mockup_map = [
        ["T00_wasteland_plain", "T02_ground_pebble_dust", "T08_scrub_offset_nw", "T05_fissure_pass_v", "T00_wasteland_plain", "T12_puddles_twin", "T00_wasteland_plain", "T03_ground_caliche_crust"],
        ["T01_ground_hairline_cracks", "T15_turret_stencil_flat", "T00_wasteland_plain", "T05_fissure_pass_v", "T00_wasteland_plain", "T00_wasteland_plain", "T15_turret_stencil_flat", "T00_wasteland_plain"],
        ["T04_fissure_pass_h", "T04_fissure_pass_h", "T04_fissure_pass_h", "T07_fissure_branch_t", "T04_fissure_pass_h", "T04_fissure_pass_h", "T06_fissure_corner_ne", "T00_wasteland_plain"],
        ["T00_wasteland_plain", "T13_mud_slick", "T00_wasteland_plain", "T05_fissure_pass_v", "T00_wasteland_plain", "T10_dry_thorns_spread", "T05_fissure_pass_v", "T09_scrub_cluster_se"],
        ["T02_ground_pebble_dust", "T15_turret_stencil_flat", "T00_wasteland_plain", "T05_fissure_pass_v", "T11_puddle_cracked_mud", "T15_turret_stencil_flat", "T05_fissure_pass_v", "T14_gravel_strip"],
        ["T03_ground_caliche_crust", "T00_wasteland_plain", "T08_scrub_offset_nw", "T05_fissure_pass_v", "T00_wasteland_plain", "T01_ground_hairline_cracks", "T07_fissure_branch_t", "T04_fissure_pass_h"]
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
        scene_ds.paste(t1_frame, (3 * 32 + 8, 0 * 32 + 4), t1_frame)
        scene_ds.paste(t1_frame, (4 * 32 + 4, 1 * 32 + 10), t1_frame)
        scene_ds.paste(t1_frame, (7 * 32 + 6, 1 * 32 + 8), t1_frame)
        scene_ds.paste(t1_frame, (4 * 32 + 12, 4 * 32 + 14), t1_frame)
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
    print(f"Mockup escénico continuo guardado en: {mockup_3x_path}")

if __name__ == "__main__":
    main()
