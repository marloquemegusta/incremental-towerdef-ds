# Purga del sistema legacy de torretas (entidad `Turret` / `Bullet`)

- **Rama:** `feat/purge-legacy-turrets`
- **Worktree:** `.worktrees/purge-legacy-turrets`
- **Objetivo:** eliminar toda huella del sistema de **torreta colocable** (una entidad separada del muro) y sus restos, sin tocar la muralla viva.

## 1. Hallazgo que motivó la sesión

El repo arrastraba **dos sistemas de proyectiles**:

- **VIVO (producción):** `WallPlatform` + `BulletDart` — la batería de 4 cúpulas, con trazadoras y salud virtual anti-overkill (`DESIGN.md` `[OQ-06]`). Se dispara con tap (`wall_fire_at_target`) y con auto-apuntado (`wall_update`).
- **MUERTO:** `Turret g_turrets[]` + `Bullet g_bullets[]` — torretas colocables de una versión vieja. **Nada asignaba `placed = 1`**, así que el bucle de update hacía `continue` siempre, `spawn_bullet` nunca se llamaba, su render nunca dibujaba y el mordisco enemigo→torreta nunca saltaba.

Conclusión: el sistema "de balas físicas" que se quería conservar era justo el inerte; el vivo es la muralla. La salud virtual **solo** actúa en la puerta de auto-fuego (evita overkill) y es inerte en tap/hold, pero **no** es lo que impide fallar: un tap al suelo vacío ya falla en el sistema vivo.

## 2. Qué se eliminó

- **Entidad y simulación (`include/game.h`, `source/simulation.c`):** structs `Turret` y `Bullet`; `g_turrets[]`, `g_bullet`, `g_bullets[]`; `MAX_TURRETS`, `MAX_BULLETS`, `BULLET_SPEED`; `spawn_bullet`; el bucle de update/aim/fire de torretas; el bucle de update/colisión de balas; el bloque "enemy bites turret"; `game_is_pos_valid` (código muerto); y todos sus resets/`memset` en `game_init`/`game_start_wave`/`game_reset_to_prep`/sandbox.
- **Render (`source/renderer.c`, `source/tiles.c`, `source/main.c`):** `renderer_draw_turret`, `renderer_draw_bullets` y su llamada en `main.c`, `tiles_draw_turret_base`, `tiles_draw_twin_bolters`, macros `C_SPARK_*` y colores `COLOR_TURRET_*`/`COLOR_BARREL_*`.
- **Campos muertos:** `upgrades.extra_turrets`, `sandbox.turret_firerate`, `sandbox.turret_damage`, `is_dragging_turret`, `drag_turret_slot`, `selected_turret`.
- **Módulo generado:** `source/turret_data.c` / `include/turret_data.h` y el paso `build_turrets()` de `scripts/build_assets.py` (que ya estaba roto: buscaba masters movidos a `archive/`). También el helper huérfano `rotsprite_pil`.
- **Sandbox:** el editor pasó de 3 filas (especie / cadencia / daño) a **1 fila** (especie), ya que cadencia y daño solo alimentaban el bucle muerto.
- **Escenarios:** rótulos obsoletos `turret*` renombrados en `mvd_test.json`; `pause_and_turret_select_test.json` → `pause_and_wave_ui_test.json` con rótulos neutrales.

## 3. Qué se conservó (crítico)

- La **muralla viva**: `WallPlatform`/`g_wall`, `BulletDart`/`g_bullet_darts`, `CasingParticle`/`g_casings`, y su nomenclatura interna `turret_*` (`turret_angles`, `turret_recoil`, `c_turret_points`, `TurretCalibratedPoints`, `TURRET_PIVOT_X/Y`, `wall_draw_turret_sprite`).
- El balance vivo de la muralla (`g_balance.turret_damage[]`, `turret_fire_interval[]`, `turret_magazine[]`) y `sandbox.turret_infinite_ammo` (lo lee `wall_update`).
- Los PNG de assets: el de la muralla (`heavy_bolter_mars_red_*`) y el archivo histórico `archive/classic_td_32x32/`.
- `tiles_draw_central_bunker` y `C_TURRET_*` (código muerto pero **no** de la entidad-torreta; fuera de alcance).

El refactor es **behavior-neutral** para gameplay: todo lo eliminado era inerte.

## 4. Verificación (determinista)

- **Build limpio:** `scripts/build-project.ps1` → `DS_BUILD=PASS`. La lista de compilación **ya no incluye** `turret_data.c` y no hay warnings.
- **Regresión (ROM nueva):** `DSM_SCENARIO_RESULT=PASS` en
  `city_defense_demo`, `session1_wall_ballistics`, `combat_and_kill_test`,
  `targeting_and_sprites_test`, `master_table_tier_check`.
- **Fallos preexistentes (no regresión):** `mvd_test.json` y el antiguo
  `pause_and_turret_select_test.json` fallan con `reason=screen_unchanged`
  **idénticamente** en el árbol principal (baseline, ROM `0CEE00FD`) y en la ROM nueva. Eran escenarios obsoletos de la era de colocación de torretas.

## 5. Coherencia documental

- `TECHNICAL.md` §6: fila `Bullet / BulletDart` → `BulletDart`.
- `STATUS.md`: retirada la afirmación obsoleta "Desbloqueo de sockets (1 a 4 torretas)".
- `docs/BALANCE_DATA.md` §4: fila 6 relabelada a "Extra Socket (Fase 2)" (nodo bloqueado, sin efecto).
- `assets/sprites/turrets/README.md`: anotado que el módulo generado y el paso del pipeline se eliminaron.

**Decisión:** no se editaron los walkthroughs históricos (`00-no-range`, `tap-hitbox-hold-aim`) aunque mencionen `g_turrets`; son registro de una sesión pasada y describían el estado de entonces.

## 6. Pendiente

- Aprobación del usuario antes de fusionar `feat/purge-legacy-turrets` en `full-incremental`.
- (Fuera de alcance, detectado) `spawn_enemy_ex` no resetea `incoming_damage` al reutilizar un slot de enemigo; arreglo opcional de 1 línea.
