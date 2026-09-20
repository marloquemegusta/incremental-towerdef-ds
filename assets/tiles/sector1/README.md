# Tileset Canónico de Sector 1 (Isométrico 2:1 - 32x32 Píxeles)

Esta carpeta contiene los tiles oficiales de suelo isométrico 2:1 (`dx=2, dy=1`) utilizados en el mapa del Sector 1 del Bastión Imperial (`source/tiles.c`, `source/scrabling_data.c`).

## Assets Maestros Canónicos (`master/`)
- `tile_061_cobblestone_1x.png`: Tile maestro canónico de empedrado uniforme (32x32 px nativos NDS).
- `tile_062_irregular_1x.png`: Tile maestro canónico de losas de piedra irregulares (32x32 px).
- `tile_063_flagstone_1x.png`: Tile maestro canónico de losas lisas de calzada (32x32 px).
- `*_8x.png`: Previsualizaciones ampliadas a 256x256 px mediante escalado Nearest-Neighbor para inspección visual.

## Renderizado en Motor (Modo 5 / Bitmap Paletizado)
- **Topología:** Ensamblado continuo de rombos en proyección isométrica 2:1.
- **Paso vertical:** 8 píxeles entre filas sucesivas (renderizado de norte a sur *back-to-front*).
- **Paso horizontal:** 16 píxeles con desplazamiento alternado por fila.
- **Línea de rango defensivo ($Y=64$):** Franja discontinua de pintura vial amarilla trazada con textura orgánica de brocha sobre la piedra.

## Material Histórico y Descartado (`archive/`)
- `archive/`: Contiene los antiguos tiles ortogonales/cenitales a 90° de 16x16 px (`T00` a `T16`) y propuestas exploratorias previas, archivadas para evitar disonancia de perspectiva con los sprites isométricos de torretas y enemigos.
- `archive/scrabling_source_pack/`: Paquete fuente completo de tiles de Scrabling.
