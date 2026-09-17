# Assets de Torretas (Adeptus Mechanicus)

## 1. Asset Maestro Canónico Activo (Wall-Mounted Turret)
El **único asset maestro canónico oficial** para las torretas de la muralla en Nintendo DS es la cinta completa de 5 ángulos calibrados:

- **Twin Heavy Bolter Mars Red (Muralla Defensiva):**
  - **Cinta Maestra Canónica:** `heavy_bolter_mars_red_strip_master_1x.png`
  - **Preview Escalado 4x:** `heavy_bolter_mars_red_preview_4x.png`
  - **GIF de Demostración de Ángulos:** `wall_turrets_patrol.gif`
  - **Dimensiones:** 220 x 44 px totales (5 celdas de 44 x 44 px).
  - **Punto de Anclaje (Pivote):** `(X_pivot = 22, Y_pivot = 38)` relativo al marco del sprite.
  - **Ángulos Calibrados (5 Frames):**
    - Frame 0: Noroeste (-45°)
    - Frame 1: Nor-Noroeste (-22.5°)
    - Frame 2: Norte Puro (0°)
    - Frame 3: Nor-Noreste (+22.5°)
    - Frame 4: Noreste (+45°)
  - **Puntos de Fuego de Cañones (Muzzle Offsets):**
    - Alternancia estricta entre cañón izquierdo y derecho para la cadencia continua de muralla.

---

## 2. Archivo Histórico de Exploración y Modo Clásico
Los assets de colocación libre en 16 ángulos (32x32 px) de la versión temprana de Tower Defense han sido trasladados a:
`assets/sprites/turrets/archive/classic_td_32x32/`

- Contiene las tiras de 16 ángulos (`heavy_bolter_16_angles_anim_1x.png`, `lascannon_...`), animaciones de retroceso libre, y la hoja original de 12 propuestas (`ds_turrets_12_proposals_1x.png`).
- Se conservan exclusivamente como histórico y **no deben ser referenciados como assets activos** del modo Muralla/Arcade Incremental.
