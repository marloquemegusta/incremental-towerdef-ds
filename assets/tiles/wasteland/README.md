# Wasteland / Yermo Balístico Tileset (32x32) — Terreno Plano y Orgánico

Tileset modular, plano y periódico para bioma de tierra árida y yermo balístico en Nintendo DS (256x192).

## Filosofía de Diseño
- **100% plano y transitable:** Sin barricadas verticales, muros rocosos ni abismos que actúen visualmente como barreras. Los enjambres xenos caminan de manera uniforme por todo el mapa.
- **Variedad geológica y botánica natural:**
  - Diversidad de patrones de fractura a ras de suelo: radial/araña, zigzag, paralelas, mosaico arcilloso, fisura ancha plana y estrella expansiva.
  - Vegetación desértica marchita y rastrera: matorral seco, grupo de hierbas desérticas y zarzas espinosas.
  - Charcos de agua salobre y lodo: superficies líquidas reflectantes a nivel de suelo con cerco de salitre y cielo plomizo.
  - Elementos minerales y marcaje Mechanicus: gravilla suelta plana, costras de caliche y cruz táctica de plantilla estarcida plana para torretas.
- **Tiling universal seamless:** Todos los tiles respetan el perímetro toroidal de `T00` para colocarse libremente en cualquier configuración de retícula 8×6 sin costuras.
- **Alto contraste figura-fondo:** Fondo claro en tonos arena desecada (`#D4C29E`) para máxima visibilidad del enjambre xenos.
- **Regla cromática canónica:** Cero tonalidades púrpuras/violetas y cero blanco hueso en el escenario.

## Catálogo de Tiles (16 Tiles Modulares)

| ID | Nombre | Descripción |
|---|---|---|
| `T00` | `T00_wasteland_plain` | Tierra árida clara continua (tiling toroidal seamless) con micro-polvo. |
| `T01` | `T01_cracks_spiderweb` | Fracturas finas radiales / tela de araña térmica a ras de suelo (tileable). |
| `T02` | `T02_cracks_zigzag` | Fractura angulada en zigzag con micro-bifurcaciones superficiales (tileable). |
| `T03` | `T03_cracks_parallel` | Grietas sedimentarias paralelas de resecamiento térmico (tileable). |
| `T04` | `T04_cracks_clay_tiles` | Cuarteamiento en mosaico de barro seco por evaporación solar (tileable). |
| `T05` | `T05_fissure_wide_flat` | Fisura ancha pero llana con lecho de micro-grava en el fondo (tileable). |
| `T06` | `T06_fissure_starburst` | Fractura expansiva en estrella desde punto focal central (tileable). |
| `T07` | `T07_scrub_single` | Matorral seco desértico individual a nivel de suelo (arbusto rastrero). |
| `T08` | `T08_scrub_patch` | Conjunto de matorrales y hierbas secas desérticas dispersas (tileable). |
| `T09` | `T09_dry_thorns` | Raíces secas y zarzas espinosas rastreras a nivel de suelo (tileable). |
| `T10` | `T10_puddle_shallow` | Charco pequeño de agua estancada a ras de suelo con borde salino (tileable). |
| `T11` | `T11_puddle_large` | Charca de agua salobre más ancha con reflejos de cielo plomizo planos (tileable). |
| `T12` | `T12_mud_depression` | Depresión llana de fango húmedo/salobre oscuro a nivel de suelo (tileable). |
| `T13` | `T13_gravel_pebbles` | Esparcimiento de pequeñas piedras planas y gravilla transitable (tileable). |
| `T14` | `T14_mineral_salt_crust` | Costras minerales de salitre / caliche blanco-amarillento (tileable). |
| `T15` | `T15_turret_marking_cross` | Marcaje balístico Mechanicus a ras de suelo (cruz estarcida plana, tileable). |

## Archivos Generados
- `*_1x.png`: Asset maestro canónico (32x32 px).
- `*_8x.png`: Preview de alta fidelidad pixel-art (256x256 px).
- `wasteland_tileset_catalog_4x.png`: Catálogo de los 16 tiles a escala 4x (512x512 px).
- `wasteland_scene_mockup_3x.png`: Mockup de arena plana DS (256x192 escalado 3x) con Heavy Bolters sobre marcas y enjambre atravesando charcos y vegetación.
