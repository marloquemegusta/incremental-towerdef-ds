# Wasteland / Yermo Balístico Tileset (32x32) — Arena Abierta

Tileset modular y periódico para bioma de tierra árida y yermo balístico en Nintendo DS (256x192).

## Filosofía de Diseño
- **Arena abierta sin caminos fijos:** El mapa es un campo abierto de supervivencia donde los enjambres asedian desde todas las direcciones hacia el búnker.
- **100% periódico y tileable:** Todos los tiles enlazan sin costuras (seamless) perimetrales (ancho 2 px de borde estandarizado con `T00`), permitiendo combinaciones procedurales libres sin saltos de costura.
- **Conectores tectónicos continuos:** Las fallas `T04` (horizontal) y `T05` (vertical) son continuas de borde a borde para formar grietas infinitas o redes cruzadas con `T06`.
- **Alto contraste figura-fondo:** Fondo claro en tonos arena y caliche (`#D4C29E`) para maximizar la visibilidad instantánea del enjambre xenos.
- **Regla cromática canónica:** Cero tonalidades púrpuras/violetas y cero blanco hueso en el entorno.

## Catálogo de Tiles (16 Tiles Modulares)

| ID | Nombre | Descripción |
|---|---|---|
| `T00` | `T00_wasteland_plain` | Tierra árida clara continua (tiling toroidal seamless) con micro-polvo. |
| `T01` | `T01_cracks_light` | Suelo árido claro con finas fracturas térmicas aisladas (tileable). |
| `T02` | `T02_cracks_medium` | Fracturas intermedias ramificadas con estrías oscuras (tileable). |
| `T03` | `T03_cracks_dense` | Cuarteamiento denso poligonal de barro desecado al sol (tileable). |
| `T04` | `T04_fissure_h` | Falla tectónica horizontal continua (seamless en X, conecta borde a borde). |
| `T05` | `T05_fissure_v` | Falla tectónica vertical continua (seamless en Y, conecta borde a borde). |
| `T06` | `T06_fissure_cross` | Cruce tectónico 4 direcciones (conecta con `T04` en X y con `T05` en Y). |
| `T07` | `T07_fissure_abyss` | Fosa tectónica con abismo oscuro central y biseles iluminados (tileable). |
| `T08` | `T08_crater_large` | Gran cráter de impacto de 20 px con eyección basáltica y reborde claro (tileable). |
| `T09` | `T09_craters_cluster` | Grupo de 3 impactos de proyectiles de mortero/metralla (tileable). |
| `T10` | `T10_rocks_scatter` | Gravilla y fragmentos basálticos dispersos sobre arena clara (tileable). |
| `T11` | `T11_boulders_central` | Formación de peñascos monolíticos erosionados por viento árido (tileable). |
| `T12` | `T12_dust_dune` | Duna eólica suave con cresta iluminada (seamless en X). |
| `T13` | `T13_turret_pad_plate` | Plataforma Mechanicus de anclaje de torretas 32x32 en acero y óxido (tileable). |
| `T14` | `T14_conduit_plate` | Placa/rejilla blindada de conducción energética a ras de suelo (tileable). |
| `T15` | `T15_scorched_caliche` | Zona calcinada por deflagración térmica con costra de caliche mineral (tileable). |

## Archivos Generados
- `*_1x.png`: Asset maestro canónico (32x32 px).
- `*_8x.png`: Preview de alta fidelidad pixel-art (256x256 px).
- `wasteland_tileset_catalog_4x.png`: Catálogo completo de los 16 tiles a escala 4x (512x512 px).
- `wasteland_scene_mockup_3x.png`: Mockup de arena abierta DS (256x192 escalado 3x) con torretas Heavy Bolter y asedio xenos multidireccional.
