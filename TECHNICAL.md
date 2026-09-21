# TECHNICAL.md - Especificación Técnica y Arquitectura (C / libnds)

Documento canónico de **invariantes de implementación** (cómo está hecho). Qué es el producto y cómo
se ve está en `DESIGN.md`; el proceso de trabajo del agente, en `AGENTS.md`.

---

## 1. Plataforma y Toolchain

- **CPU:** ARM946E-S a 67 MHz (Nintendo DS). **Sin FPU por hardware.**
- **Toolchain:** BlocksDS (`skylyrac/blocksds:slim-latest`) + libnds v2, vía Docker Desktop
  (`scripts/build-project.ps1`).
- **Emulación determinista:** DeSmuME headless (`runtime/bin/libdesmume.so`) sobre WSL2
  (`scripts/run-scenario.ps1`).

## 2. Aritmética

- **Terminantemente prohibido `float`/`double`** en simulación y render.
- **Punto fijo Q8** (`FP_SHIFT 8`, `FP_ONE 256`, `TO_FP()` / `FROM_FP()`): posiciones, velocidades y
  altura (`z`). Aritmética entera de 32 bits.
- Trigonometría por **tabla entera** de 256 divisiones de círculo (`fixed_sin`, `fixed_cos`,
  `fixed_atan2` en `source/math_lut.c`).

## 3. Pipeline de render

### Pantalla inferior (táctil) — Main Engine, framebuffer 16-bit **doble buffer**

- `VRAM_A` y `VRAM_B` en modo LCD; `MODE_FB0` / `MODE_FB1` alternan en VBlank **sin copia**
  (presentación a coste ~0).
- Se dibuja en RAM (`g_backbuffer`) y se vuelca por DMA.
- **Dirty grid 8×8** (32×24 bloques) con **máscara por buffer** (`s_dirty_mask_bot[2]`): sólo se
  restauran los bloques marcados, con `memcpy` desde el *ground cache* (RAM→RAM, coherente con la
  caché; nunca DMA RAM→RAM).
- **Regla dura:** toda entidad dinámica (enemigos, balas, dardos, casquillos, trozos, partículas,
  UI arrastrada) debe marcar su rect con `tiles_dirty_mark_rect(x, y, w, h, is_bottom, buf_idx)`.
- Los **estampados de suelo** (sangre) escriben en el *ground cache* y **deben marcar su propia área
  en ambos buffers** (`mark_ground_stamp`); si no, el siguiente page flip los borra.

### Pantalla superior — Sub Engine, bitmap 8-bit

- `MODE_5_2D` + `BgType_Bmp8` (256×256) en el banco C, paleta de 256 colores en `BG_PALETTE_SUB`.
- Buffer de **48 KB** (la mitad que 16-bit) y una sola máscara de dirty (sin doble buffer).

### Sprites

- Blit **indexado 8-bit → paleta** (`g_enemy_palette`, en **DTCM**), por **quads de 4 píxeles** con
  *skip* de transparentes en 1 ciclo (`qval == 0`). **Sin OAM** para enemigos.
- Rutinas críticas en **ITCM** (`ITCM_CODE`): blitter de enemigos (`source/enemy_data.c`) y
  restauración de dirty (`source/tiles.c`).

## 4. Presupuesto de CPU (60 FPS)

- Timer 0 con divisor 1024 ⇒ ~32.728 Hz ⇒ **545 ticks por frame** a 60 FPS.
- Reparto objetivo: **S (simulación) ≤ 100**, **R (restauración de fondo) ≤ 80**, **E (blit de
  entidades) ≤ 300**, **P (presentación) ≤ 50** y ≥ 15 de margen.
- Telemetría en el HUD superior: `FPS T B P S E` (`T` = render superior, `B` = render inferior,
  `P` = presentación, `S` = simulación, `E` = nº de enemigos activos).
- Patrones y presupuestos detallados en la skill: `references/performance-architecture.md`.

## 5. Simulación

- **Enjambre de hasta 384 enemigos** (`MAX_ENEMIES`) sobre un campo vertical unificado 256×384.
- **Spatial grid** de celdas de 16 px (`ENEMY_GRID_*` en `source/simulation.c`) para separación y
  colisiones: consultas de vecindad 3×3 en O(N).
- **Salud virtual anti-overkill** (`incoming_damage`): la muralla no dispara si
  `hp - incoming_damage <= 0`.
- **Muerte en dos fases:** `dying` (15 frames de animación; no avanza, no colisiona ni es objetivo) y
  retirada.
- Duración de etapa: 120 s (`STAGE_DURATION_FRAMES = 7200`).
- **Sin rango (dormido):** `g_balance.turret_range[]` se conserva a `0` por reversibilidad; un valor
  `> 0` reintroduciría una línea de fuego (prohibida por diseño, `DESIGN.md` `[OQ-06]`).

## 6. Estructuras de datos principales (`include/game.h`)

| Estructura | Notas |
| :--- | :--- |
| `Enemy` | `x,y` Q8 globales; `hp`/`max_hp`; `incoming_damage`; `dying` / `death_timer` / `death_dir_*`; dirty rects independientes por pantalla |
| `GoreChunk` | Trozos sólidos: bloque de índices de paleta (≤ 4×4), `x,y,z` Q8, vida |
| `DeathParticle` | Gotas/partículas 3D con `z`; al aterrizar estampan en el suelo |
| `CasingParticle` | Casquillos con rebote, giro y zumbido de vida |
| `Bullet` / `BulletDart` | Balas de torreta / dardos de muralla (`dist_remaining` + `target_enemy_idx`) |
| `Turret` / `WallPlatform` | Torretas y muralla: sockets, ángulos, munición, `range_line_y` **dormido** en 0 |
| `GameState` / `GameBalanceConfig` | Modos de juego, telemetría y balance serializable |

## 7. Persistencia y configuración

- Balance en `fat:/towerds_balance.bin` con `magic 0x544F5736` (`TOW6`); si el magic no coincide, los
  valores cargados **se ignoran** (evita reintroducir mecánicas retiradas desde un save antiguo).
- **Ground cache:** la sangre acumulada es permanente durante la run; sólo se reconstruye en
  `tiles_init()`, es decir, al empezar partida nueva.

## 8. Verificación

- Escenarios deterministas en `scenarios/` ejecutados con `scripts/run-scenario.ps1` (capturas,
  `events.jsonl`, aserciones de estado y de píxeles).
- Analizador de capturas: `tools/gore_probe.py` (`region` / `count` / `bbox` / `scan`).
- Un `PASS` del runner no es una prueba de comportamiento: exige comparar capturas y medir.
