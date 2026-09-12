# Wasteland / Yermo Balístico Tileset (32x32)

Tileset modular para bioma de tierra árida, yermo y desierto balístico para Nintendo DS (256x192).

## Especificaciones Técnicas
- **Resolución unitaria:** 32x32 píxeles (4x4 tiles de hardware NDS de 8x8 px).
- **Relación de pantalla:** 8 columnas × 6 filas = 48 tiles por viewport DS completo.
- **Paleta de color:** BGR555 en tonos luminosos de arena desecada, arcilla calcárea, arenisca, roca basáltica y acero oxidado.
- **Regla cromática canónica:** Cero tonalidades púrpuras/violetas y cero blanco hueso (estrictamente reservadas para el enjambre xenos).
- **Regla balística 3D:** Trincheras de paso de 32 px de ancho con 3-4 px de sombra arrojada profunda en voladizos.
- **Contraste:** Fondo de terreno predominantemente claro y suave para maximizar la lectura visual de los enjambres enemigos oscuros.

## Catálogo de Tiles

| ID | Nombre | Descripción |
|---|---|---|
| `T00` | `T00_wasteland_plain` | Tierra árida clara continua (tiling seamless orgánico sin rejillas) con micro-polvo. |
| `T01` | `T01_wasteland_cracked` | Suelo árido claro con patrón de fisuras superficiales por choque térmico. |
| `T02` | `T02_wasteland_fissure_deep` | Fisura profunda transversal con estrías de sombra y roca basáltica expuesta. |
| `T03` | `T03_wasteland_gravel_rocks` | Esparcimiento de gravilla suelta y piedras basálticas con bisel luminoso. |
| `T04` | `T04_path_straight_v` | Trinchera/sendero vertical hundido con rodadas y sombra profunda de 4 px (labio O). |
| `T05` | `T05_path_straight_h` | Trinchera/sendero horizontal hundido con sombra profunda de 4 px (labio N). |
| `T06` | `T06_path_corner_turn` | Codo de sendero conectando Norte con Este respetando la retícula balística. |
| `T07` | `T07_path_junction` | Intersección / bifurcación de senderos clara y transitable. |
| `T08` | `T08_path_crater` | Cráter de impacto de artillería en mitad del sendero con eyección de escombros. |
| `T09` | `T09_cliff_edge_s` | Farallón rocoso orientado al Sur (cota alta arriba, pared vertical y sombra abajo). |
| `T10` | `T10_cliff_edge_n` | Farallón orientado al Norte (cota baja abajo, sombra cenital al pie del talud). |
| `T11` | `T11_boulder_formation` | Formación monolítica erosionada por viento abrasivo; bloquea línea de paso. |
| `T12` | `T12_scree_slope` | Canchal / pedregal inclinado de derrumbe que conecta cotas. |
| `T13` | `T13_turret_pad_plate` | Plataforma Mechanicus de anclaje de torretas 32x32 en acero biselado y remachado. |
| `T14` | `T14_pipeline_exposed` | Conducción industrial de refrigerante/fuel fracturada y semienterrada en arena. |
| `T15` | `T15_cracked_network` | Red intrincada de fracturas y barro cuarteado por sequedad extrema. |

## Archivos Generados
- `*_1x.png`: Asset maestro canónico (32x32 px).
- `*_8x.png`: Preview de alta fidelidad pixel-art (256x256 px).
- `wasteland_tileset_catalog_4x.png`: Vista completa de los 16 tiles a escala 4x (512x512 px).
- `wasteland_scene_mockup_3x.png`: Composición escénica real DS (256x192 escalada 3x a 768x576) integrando torretas Heavy Bolter y enjambre xenos.
