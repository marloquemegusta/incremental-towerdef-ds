# Wasteland / Yermo Balístico Tileset (32x32) — Sustrato Continuo y Asimétrico (v5)

Tileset modular, plano y orgánico para bioma de tierra árida y yermo en Nintendo DS (256x192).

## Mejoras de Diseño (v5)
- **Erradicación de la mancha central:** El sustrato base `T00` es homogéneo, luminoso e isotrópico. Se ha eliminado por completo cualquier patrón o gradiente centrado que delate una cuadrícula periódica.
- **Detalles asimétricos y descentrados:** Elementos distribuidos en cuadrantes alternos (NO, SE, diagonales y bordes) para una ruptura natural del ritmo de repetición.
- **Micro-fisuras integradas en todo el sustrato:** Todo el terreno árido comparte una red sutil de micro-grietas de desecación solar continua, logrando que el suelo se perciba cohesionado y no como "grietas aisladas en unos pocos tiles".
- **Red de fisuras modulares interconectables:** Las fallas principales (`T04` a `T07`) enlazan de borde a borde en anclajes fijos (X=16 o Y=16), componiendo redes y ramificaciones infinitas a lo largo de la pantalla.
- **100% plano y transitable:** Charcos salobres, matorrales y marcajes Mechanicus a nivel de suelo para paso natural del enjambre xenos.

## Catálogo de Tiles (16 Tiles Modulares)

| ID | Nombre | Descripción |
|---|---|---|
| `T00` | `T00_wasteland_plain` | Suelo árido base claro con micro-fisuras toroidales continuas uniformes (sin mancha central). |
| `T01` | `T01_ground_hairline_cracks` | Suelo con fisuras secundarias en abanico asimétrico (cuadrante Este). |
| `T02` | `T02_ground_pebble_dust` | Salpicado sutil de gravilla basáltica y motas de caliche (disperso asimétrico). |
| `T03` | `T03_ground_caliche_crust` | Costras llanas de salitre / caliche mineral en cuadrante Noroeste. |
| `T04` | `T04_fissure_pass_h` | Fractura tectónica horizontal continua (conecta en (0, 16) y (31, 16)). |
| `T05` | `T05_fissure_pass_v` | Fractura tectónica vertical continua (conecta en (16, 0) y (16, 31)). |
| `T06` | `T06_fissure_corner_ne` | Fractura en codo modular (conecta en Norte (16, 0) y Este (31, 16)). |
| `T07` | `T07_fissure_branch_t` | Bifurcación en T (conecta Oeste (0, 16), Este (31, 16) y Sur (16, 31)). |
| `T08` | `T08_scrub_offset_nw` | Arbusto árido rastrero descentrado en cuadrante Noroeste (brota de grieta). |
| `T09` | `T09_scrub_cluster_se` | Grupo de dos pequeñas matas secas descentradas en cuadrante Sureste. |
| `T10` | `T10_dry_thorns_spread` | Zarzas espinosas y raíces secas en diagonal SW-NE (descentrado). |
| `T11` | `T11_puddle_cracked_mud` | Charco de agua salobre descentrado al Este con barro cuarteado circundante. |
| `T12` | `T12_puddles_twin` | Dos charcos pequeños asimétricos en esquinas opuestas (SO y NE). |
| `T13` | `T13_mud_slick` | Mancha plana de barro húmedo con micro-fisuras superficiales (descentrada al Oeste). |
| `T14` | `T14_gravel_strip` | Franja transversal difusa de gravilla plana basáltica. |
| `T15` | `T15_turret_stencil_flat` | Marcaje táctico Mechanicus estarcido plano a ras de suelo para anclaje de torreta. |

## Archivos Generados
- `*_1x.png`: Asset maestro canónico (32x32 px).
- `*_8x.png`: Preview de alta fidelidad pixel-art (256x256 px).
- `wasteland_tileset_catalog_4x.png`: Catálogo completo de los 16 tiles a escala 4x (512x512 px).
- `wasteland_scene_mockup_3x.png`: Mockup de arena continua DS (256x192 escalado 3x) con fisuras enlazadas y enjambre xenos.
