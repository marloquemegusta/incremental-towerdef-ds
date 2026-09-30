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
- **Atraedor Continuo (`AttractorState`):** los diales se **escriben en ticks de 0.05** (`dial_rate_ticks`
  0..200 $\to$ 0.00..10.00 /s; `dial_tier_ticks` 20..80 $\to$ T1.00..T4.00) porque Q8 no representa $0.05$
  de forma exacta. `dial_rate_q8` / `dial_tier_q8` son **vistas derivadas** (Q8) sincronizadas por
  `dial_rate_sync()` / `dial_tier_sync()`. El acumulador de spawn corre en unidades de `rate_q8 * frames`
  (`spawn_budget_q8 += dial_rate_q8` por frame, umbral `60 * 256`), de modo que no trunca y tasas de
  hasta $0.05$/s siguen generando. Mezcla probabilística de biocastas entre tiers.
- **Tabla maestra de enemigos (`GameBalanceConfig.enemy[]`, `EnemyStatDef`):** única fuente de verdad
  de las stats por variante (`tier` 1..4, `hp`, `speed`, `scrap`, `bite_damage`, `bite_interval`).
  La consumen el spawn continuo (toma el tier del dial $\to$ 50/50 entre las dos especies de ese tier
  $\to$ `hp`/`speed` de la tabla), la recompensa de scrap al morir, el mordisco a muro/torreta, el
  sandbox de debug y la página `ENEMY STATS` de calibración. **Prohibido** duplicar estas stats fuera
  de la tabla (constantes literales, arrays paralelos o CSV); `EnemyTypeDef` sólo contiene datos de render.
- **Compuerta del Hold-to-Fire (A1):** el disparo continuo exige `upgrades.continuous_fire` **y**
  `g_generator.tiers[0].active` (automatización erigida y viva). Erigir/reparar el Tier 1 por otras vías
  (reparación diegética, calibración) **no** habilita el Hold; si el Tier 1 cae, el Hold se degrada a tap.
- **Generador Aditivo y Andamio (`GeneratorState`):** 7 tiers físicos (5 HP cada uno). Sin Game Over:
  el daño degrada tiers y desactiva temporalmente automatizaciones hasta su reparación. Si
  `built_tiers == 0`, los enemigos cruzan por debajo del andamio sin colisión.
- **Batería Unificada:** 1 entidad lógica metrónomo (`WallPlatform`) coordinando 4 cúpulas visuales.
  Cargador compartido de batería (10 balas base a Lv0). Consumo de 1 bala por disparo y recarga
  táctil arrastrando desde el depósito diegético ($X \in [110..146], Y \in [150..180]$).
- **Salud virtual anti-overkill** (`incoming_damage`): la batería no dispara si
  `hp - incoming_damage <= 0`.
- **Hitbox táctil (anclada al sprite dibujado):** el tap resuelve el objetivo contra la **caja real del
  sprite** — `enemy_get_frame_bounds()` en `source/enemy_data.c`, que replica el `offset_x/offset_y` del frame
  y la cota de vuelo (`flight_altitude`) igual que la ruta de dibujo — **unida** a un radio de gracia
  `TAP_HIT_RADIUS` (24 px) alrededor del centro del sprite y con `TAP_HIT_MARGIN` (6 px) de margen. Gana el
  enemigo más cercano al centro. Al ser una unión, la región nunca es más estricta que el antiguo círculo
  fijo de 24 px sobre el punto lógico.
- **Disparo libre:** no hay filtro de suelo vacío; el tap abre fuego a cualquier punto del campo inferior y
  consume bala y cadencia igual que un impacto (ver `DESIGN.md` `[OQ-06].4`).
- **Apuntado persistente y traverse:** al disparar se fija `turret_angles[s] = target_angles[s] =
  last_aim_angle[s] = angle` (snap instantáneo, necesario para que la boca y el proyectil casen
  geométricamente). Sin objetivo, la postura de reposo es `last_aim_angle[s]`, no un ángulo fijo
  (`DESIGN.md` `[OQ-11]`). El traverse motorizado avanza **1 paso cada 2 frames** cuando el ángulo actual
  difiere del deseado.
- **Muerte limpia:** licuado gravitatorio local (15 frames) y desprendimiento de trozos de sprite.
  **Conos de muerte desactivados** (`DEATH_CONE_ENABLED = 0`); prohibido dibujar conos en pantalla
  superior o sub-bancos.
- **Sin rango (dormido):** `g_balance.turret_range[]` se conserva a `0` por reversibilidad; un valor
  `> 0` reintroduciría una línea de fuego (prohibida por diseño, `DESIGN.md` `[OQ-06]`).

## 6. Estructuras de datos principales (`include/game.h`)

| Estructura | Notas |
| :--- | :--- |
| `Enemy` | `x,y` Q8 globales; `hp`/`max_hp`; `incoming_damage`; `dying` / `death_timer` / `death_dir_*`; dirty rects independientes por pantalla |
| `GeneratorState` | `built_tiers` (0..7), `tier_hp[7]` (5 HP/tier), bombillas catódicas de estado, degradación de A1-A7 |
| `AttractorState` | Diales en ticks de 0.05 (`rate_ticks`, `tier_ticks`) como fuente de verdad; vistas Q8 derivadas (`rate_q8`, `tier_q8`), acumulador fraccionario en unidades de `rate_q8*frames` |
| `GoreChunk` | Trozos sólidos: bloque de índices de paleta (≤ 4×4), `x,y,z` Q8, vida |
| `DeathParticle` | Gotas/partículas 3D con `z`; al aterrizar estampan en el suelo |
| `CasingParticle` | Casquillos con rebote, giro y zumbido de vida |
| `Bullet` / `BulletDart` | Balas de batería (`dist_remaining` + `target_enemy_idx`) |
| `WallPlatform` | Batería unificada: sockets visuales, `turret_angles`/`target_angles`/`last_aim_angle` (0..4 por socket), munición global de batería, recarga diegética |
| `EnemyStatDef` | Stats maestras por variante (`tier`, `hp`, `speed`, `scrap`, `bite_damage`, `bite_interval`); array `g_balance.enemy[ENEMY_VARIANT_COUNT]` |
| `GameState` / `GameBalanceConfig` | Modos de juego, telemetría y balance serializable (tabla maestra de enemigos + Atraedor/Generador/Costes) |

## 7. Persistencia y configuración

- Balance en `fat:/towerds_balance.bin` con `magic 0x544F5737` (`TOW7`); si el magic no coincide, los
  valores cargados **se ignoran** (evita reintroducir mecánicas retiradas desde un save antiguo).
- **Ground cache:** la sangre acumulada es permanente durante la run; sólo se reconstruye en
  `tiles_init()`, es decir, al empezar partida nueva.

## 8. Verificación

- Escenarios deterministas en `scenarios/` ejecutados con `scripts/run-scenario.ps1` (capturas,
  `events.jsonl`, aserciones **de píxeles**: `screen_changed` / `screen_changes`). El runner **no**
  implementa aserciones de estado (hp / ammo / ángulos); éstas se infieren midiendo las capturas.
- Analizadores de capturas: `tools/gore_probe.py` (sangre), `tools/enemy_probe.py` (enjambre púrpura en
  pantalla inferior), `tools/ab_frame_diff.py` (`seq` / `pair`: diff A/B de píxeles con bbox),
  `tools/make_ab_image.py` (recorte + reescalado + composición 2×2) y `tools/hitbox_geometry.py`
  (auditoría geométrica de la cobertura de la hitbox sobre los sprites definidos).
- Un `PASS` del runner no es una prueba de comportamiento: exige comparar capturas y medir.
