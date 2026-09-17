# Bastión Defensivo Canónico (Bunker Wall) y Logística

## 1. Muro del Bastión (Bunker Wall)
- **Archivo Maestro Canónico:** `bunker_wall_master_1x.png`
- **Preview Escalado 4x:** `bunker_wall_preview_4x.png`
- **Resolución nativa:** 256 x 48 px (ancho de pantalla completa de Nintendo DS).
- **Simetría:** 100% simétrico respecto al eje central vertical (X = 127.5).
- **Ubicación en pantalla táctil (inferior):** Base del campo de batalla (`Y = 144..191`).
- **Máscara de Transparencia:**
  - El contorno superior del parapeto (`Y < 12`) utiliza color transparente (`0x0000` / RGBA alpha=0), permitiendo que la calzada y los escombros se visualicen correctamente por detrás del muro sin artefactos negros.
- **Oclusión 3D:**
  - El muro se renderiza por delante de los enemigos en el frente de batalla (`Y = 144`). El parapeto cubre la cabeza y el torso de los xenos atacantes, revelando únicamente sus extremidades posteriores sobre la calzada para transmitir profundidad 3D de trinchera hundida.

---

## 2. Bahías de Anclaje Canónicas (Sockets / Hardpoints)
Las 4 bahías mecánicas sobre la cornisa de la muralla están fijadas en:

| Bahía | Coordenada X | Coordenada Y | Ángulo de Batería por Defecto |
| :---: | :---: | :---: | :---: |
| **Socket 0 (Flanco Izquierdo)** | **X = 28** | Y = 16 | Noroeste (-45°, Frame 0) |
| **Socket 1 (Centro-Izquierda)** | **X = 93** | Y = 16 | Nor-Noroeste (-22.5°, Frame 1) |
| **Socket 2 (Centro-Derecha)** | **X = 162** | Y = 16 | Nor-Noreste (+22.5°, Frame 3) |
| **Socket 3 (Flanco Derecho)** | **X = 227** | Y = 16 | Noreste (+45°, Frame 4) |

*Fórmula de colocación de torretas (`TURRET_SPRITE_W = 44, TURRET_SPRITE_H = 44`, pivote en 22, 38):*
- `pos_x = socket_x - 22`
- `pos_y = wall_screen_y + socket_y - 38`

---

## 3. Cajón de Munición de Suministros (Ammo Crate)
- **Archivo Maestro Canónico:** `ammo_crate_master_1x.png`
- **Preview Escalado 4x:** `ammo_crate_preview_4x.png`
- **Resolución nativa:** 26 x 16 px.
- **Ubicación en Búnker:** Centrado en la plataforma de hormigón entre los dos sockets centrales (`X = 128, Y = 166`).
- **Función en Juego:**
  - Almacén de suministros balísticos para recarga manual táctil.
  - Al vaciarse el tambor (10 disparos base), el jugador arrastra un paquete de munición desde el cajón hasta la torreta para recargar.
