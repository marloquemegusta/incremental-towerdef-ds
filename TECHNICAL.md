# TECHNICAL.md - Especificación Técnica y Arquitectura C/libnds

## 1. Plataforma y Toolchain
- **CPU:** ARM946E-S a 66.7 MHz (Nintendo DS).
- **Toolchain:** BlocksDS (`skylyrac/blocksds:slim-latest`), libnds v2.
- **Emulador de referencia / Testing:** DeSmuME headless x86_64 (`libdesmume.so`) en WSL2.

## 2. Restricciones Técnicas Críticas
- **Sin FPU por hardware:** Terminantemente prohibido el uso de `float` o `double` en bucles de simulación o renderizado.
- **Aritmética de punto fijo (Fixed-Point):**
  - Formato Q12.4 o enteros escalados para posiciones y velocidades.
  - Tabla de senos y cosenos enteros precalculada (256 o 512 divisiones de círculo).
- **Límite de sprites OAM (128 máx.):**
  - Pantalla táctil configurada en modo Framebuffer / Bitmap directo (Modo 5 o Modo 3 16-bit / 8-bit con VRAM directa).
  - Enemigos dibujados como píxeles directos (2x2) en el buffer.
  - Trazadores dibujados directamente como segmentos de 2 px en el buffer.
  - Permite simular cientos y miles de partículas sin sobrecargar la OAM.
- **Pantalla Superior:**
  - Sub-motor 2D inicializado con `consoleDemoInit(NULL)` en modo texto (32x24 caracteres).
  - Vuelco de estadísticas con código ANSI / buffer de texto sin coste gráfico en ARM9.

## 3. Estructuras de Datos Principales
- `Enemy`:
  - `x, y` (punto fijo)
  - `waypoint_idx`
  - `hp`
  - `active`
- `Turret`:
  - `x, y`
  - `center_angle` (0..255)
  - `current_angle` (0..255)
  - `sweep_min, sweep_max`
  - `sweep_dir` (+1 / -1)
  - `cooldown`
  - Estadísticas: `shots_fired, hits_confirmed, wasted_shots, damage_dealt`
- `Bullet`:
  - `x, y, vx, vy` (punto fijo)
  - `life`
  - `active`
- `GameState`:
  - Modo: `PREP, WAVE, GAME_OVER, WORKSHOP`
  - Estadísticas globales: `core_hp, scrap, wave_alive, wave_killed, wave_breached`
  - Upgrades de metaprogresión.
