# Bastión Defensivo Canónico (Bunker Wall)

## Especificaciones Técnicas
- **Archivo maestro:** bunker_wall_master_1x.png
- **Resolución nativa:** 256x48 px (ancho de pantalla completa de Nintendo DS).
- **Simetría:** 100% simétrico respecto al eje central vertical (X = 127.5).
- **Ubicación en pantalla táctil (inferior):** Base del campo de batalla (Y = 144..191 o Y = 152..191 según solapamiento de calzada).

## Bahías de Anclaje Canónicas (Sockets / Hardpoints)
Las 4 bahías mecánicas sobre la cornisa de la muralla están fijadas en:

| Bahía | Coordenada X | Coordenada Y | Ángulo de Batería por Defecto |
| :---: | :---: | :---: | :---: |
| **Socket 0 (Flanco Izquierdo)** | **X = 28** | Y = 16 | Noroeste (-45°, Frame 0) |
| **Socket 1 (Centro-Izquierda)** | **X = 93** | Y = 16 | Nor-Noroeste (-22.5°, Frame 1) |
| **Socket 2 (Centro-Derecha)** | **X = 162** | Y = 16 | Nor-Noreste (+22.5°, Frame 3) |
| **Socket 3 (Flanco Derecho)** | **X = 227** | Y = 16 | Noreste (+45°, Frame 4) |

*Nota:* Para fuego en posición única central (Nivel 1), se utiliza el cañón en X = 93 o alternado apuntando a Norte Puro (0°, Frame 2).

## Montaje de Sprites de Torretas (Heavy Bolter Mars Red)
- **Archivo de sprites:** assets/sprites/turrets/heavy_bolter_mars_red_strip_master_1x.png
- **Tamaño de celda:** 38x42 px por ángulo (5 ángulos en total).
- **Punto de anclaje (Pivote):** (X_pivot = 18, Y_pivot = 34) relativo al cuadro del sprite.
- **Fórmula de colocación en pantalla:**
  - pos_x = socket_x - 18
  - pos_y = wall_screen_y + socket_y - 34
