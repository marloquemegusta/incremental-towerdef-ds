# STATUS.md - Estado de Desarrollo

> Los **hitos 1 y 2** son registro histórico: describen la arquitectura de su momento (telemetría por consola, camino en "S", torreta Vulcan, taller de 3 ramas), ya superada. El **estado vigente** es el hito más reciente y el código de `source/`.

## Fase Actual: Hito 1 (MVD - Mínimo Viable Jugable) - COMPLETADO

- [x] Comprobación de toolchain BlocksDS y DeSmuME (`DS_TOOLCHAIN=PASS`).
- [x] Scaffolding del proyecto inicializado.
- [x] Documentos de arquitectura y diseño redactados (`AGENTS.md`, `DESIGN.md`, `TECHNICAL.md`).
- [x] Implementación del motor gráfico y simulación en C (`source/main.c`, `source/renderer.c`, `source/simulation.c`, `source/math_lut.c`, `include/game.h`).
  - Pantalla superior con consola de telemetría técnica (32x24 caracteres) con DPS en tiempo real, precisión %, balas perdidas y flujo de oleada.
  - Pantalla inferior en modo framebuffer directo (Modo FB0, 256x192 px) con camino en "S", base/núcleo con barra de vida, enjambre de partículas (2x2 px), trazadores y torreta Vulcan con barrido angular oscilante de 45°.
  - Interfaz táctil con stylus sin puntos muertos: arrastrar torreta del dock, colocar en suelo válido, reubicar, desmantelar/guardar al 100%, orientar cono y botón `[START WAVE]`.
  - Acelerador de tiempo (Fast-Forward 2x) mediante botón táctil y botón `R`.
  - Taller de metaprogresión con 3 ramas de mejora (Cadencia, Servomotores, Reciclador) financiado con chatarra.
- [x] Compilación limpia de la ROM `game.nds` con BlocksDS Docker.
- [x] Verificación determinista en DeSmuME headless con escenarios automáticos (`mvd_test.json` y `combat_and_kill_test.json` pasando al 100%).
- [x] Evidencia visual capturada y validada en `artifacts/mvd_run/` y `artifacts/combat_run/`.

## Fase Actual: Hito 2 (Overhaul Visual Pixel-Art y Motor de Tilesets) - COMPLETADO

- [x] Motor de tilesets de 16x16 píxeles para escenario de bastión industrial: placas de blindaje remachadas, rejillas de ventilación y tuberías hidráulicas.
- [x] Trinchera balística de 32 píxeles de anchura interior continua sin solapamientos, delimitada por franjas de peligro (*hazard stripes* amarillas y negras).
- [x] Rediseño de la torreta Twin Heavy Bolter: cúpula de acero Mechanicus con anillo exterior rojo Marte, caja de munición de latón y cañones masivos de 3-4 píxeles de grosor.
- [x] Curva balística de retroceso no lineal explosiva: golpe seco instantáneo a fondo en 1 frame (-4px), retención de pico (-4px) y retorno amortiguado (-2px -> -1px -> 0px).
- [x] Sistema de partículas balísticas: Opción 3 (Chispas de tungsteno por fricción de cerrojo) implementado en C y validado en DeSmuME.
- [x] Micro-sprites del enjambre xenos (5x5 px) con patas animadas y vectores direccionales (Norte, Sur, Este, Oeste).
- [x] Catálogo y versionado de assets gráficos en `assets/` (sprites, propuestas de 12 torretas, animaciones y filmstrips).
- [x] Compilación limpia y validación de escenarios en DeSmuME (`combat_sparks_test`).

## Fase Actual: Hito 3 (Transición a Arcade Incremental: Muralla Modular, Marea Continua y Fricción de Disparo) - EN PROGRESO

- [x] Consolidación de visión canónica en `DESIGN.md` (unificación de documentos, muralla modular en $Y=344$, hoja de ruta de fases y archivo de legacy docs).
- [x] Limpieza de assets canónicos: exclusivamente 8 especies del enjambre StarCraft visibles en `assets/sprites/enemies/`.
- [x] Resolución de `[OQ-01]` a `[OQ-04]` (Muralla unificada, marea continua pautada, escalera de automatización del disparo y economía limpia Chatarra + Núcleos sin energía pasiva).
- [x] **Especificación Maestra y Contrato de Fase 1:** Documento `docs/PHASE_1_SPEC.md` consolidado con el alcance completo del prototipo jugable.
- [x] **Diseño de Escalado Incremental y Automatización Visual:** Documento `docs/SCALING_AND_AUTOMATION_IDEAS.md` consolidado con la visión de logística de drones visibles (Fase 2) y escala volumétrica de enemigos colosales/titanes (Fase 3).
- [ ] **Desarrollo de Fase 1 (Detalle técnico en [docs/PHASE_1_SPEC.md](docs/PHASE_1_SPEC.md)):**
  - [x] **Sesión 1: Muralla, Sockets y Balística Táctil [COMPLETADA - commit `fb6b1bf`]:**
    - Muralla continua en borde inferior con parapeto 3D y descarte de transparencias (`0x0000`).
    - Oclusión 3D: enemigos atacando el muro a $Y=144$ quedan cubiertos por el parapeto (solo asoman extremidades traseras).
    - Metrónomo de muralla: alternancia de fuego entre torretas y cañones ($T_1 L \to T_2 L \to T_1 R \dots$).
    - Salud virtual anti-overkill (`incoming_damage`) con proyectiles balísticos visuales directos a 16 px/f.
    - Línea recta de alcance horizontal a $Y=64$ ($X \in [32..224]$).
    - Filtro estricto de stylus: prohibido disparo sobre asfalto vacío; solo fuego a enemigos vivos en rango.
    - Búnker diegético: indicador de vida con 32 lámparas catódicas verdes + trauma flash dorado y alerta roja al 25% HP.
    - Logística táctil: cajón de munición canónico ($26 \times 16$ px) en $X=128, Y=166$, tambor de 10 balas y recarga manual arrastrando con stylus.
    - Ataque de enemigos distribuido en todo el ancho del frente ($Y=144$), eliminando embudo hacia el socket.
    - Validación determinista al 100% en DeSmuME (`test_phase1_complete`, `test_wall_frontline_damage`, `test_wall_hp_and_crate`).
  - [x] **Sesión 2: Marea Continua y Picos de Alerta [COMPLETADA]:**
    - Spawner de marea incesante de Zerglings terrestres y Scourges aéreos cruzando de pantalla superior ($Y=0$) a inferior ($Y=192$).
    - Temporizador y picos de alerta pautados ("¡ALERTA PICO EN Xs!") con hordas compactas y banner en fila 3.
    - HUD y telemetría superior: contador de bajas, chatarra acumulada, tiempo de supervivencia, aviso de horda y métricas de rendimiento en tiempo real (`FPS`, `T`, `B`, `P`, `S`).
    - Sincronización de daño de mordiscos de enjambre en tiempo real a las 32 lámparas catódicas del muro.
  - [x] **Sesión 3: Árbol Visual de Mejoras, Menús de Calibración y Render Dinámico [COMPLETADA]:**
    - Menús táctiles de calibración/tienda en pantalla inferior con tabs (`STATS`, `TIENDA`, `BALANCE`).
    - Automatización de gatillo (hold) y auto-aim (la batería unificada mantiene 4 cúpulas visuales desde el inicio de la partida).
    - Optimización de renderizado mediante Dirty Rects y caché de fondo en DMA para sostener 60 FPS.
    - Registro de Known Issues en `KNOWN_ISSUES.md` (pausa en hardware, estela residual, limpieza en reset, rendimiento en picos).
  - [x] **Sesión 4: Eliminación del Concepto de Rango (Fuego Total en Pantalla Inferior) [COMPLETADA - rama `feat/no-range`]:**
    - Retirada del horneado de la línea de demarcación de rango en $Y=64$ (franja de pintura vial amarilla); el empedrado queda limpio y sin indicador de alcance.
    - `g_balance.turret_range[]` neutralizado a $0$ como **mecanismo dormido y reversible**: $0$ = campo inferior completo ($Y$ local $0..143$) targeteable, sin línea de fuego. Reintroducir una línea exige un valor $>0$.
    - La puerta de auto-fuego (`local_gy >= range_line_y`) y el filtro táctil quedan inertes, por lo que toda la calzada es batible tanto por auto-aim como por stylus.
    - Retirada de la mejora RANGE: 7 cartas en tienda (antes 8), sin costes (`range_upgrade_costs`) ni hitbox táctil asociados.
    - Purga de código muerto y de controles de diagnóstico: `renderer_draw_range_perimeter()`, `WALL_TURRET_RANGE`, `DebugSandboxState.turret_range` (fila `TURRET RANGE` del sandbox + su círculo ámbar) y filas `RANGE LVn` de calibración en páginas 2 y 3, con renumeración de índices y de `max_rows`.
    - Magia del balance `TOW5` → `TOW6` para descartar saves SD obsoletos que reintroducirían silenciosamente la línea en hardware real.
    - **Evidencia A/B determinista** (ROM nueva crc `0A237779` vs `main` crc `767EBBAC`, misma secuencia de entrada): el baseline conserva 186-201 píxeles amarillos en $Y$ global 256-257 (local $Y=64$) y enemigos xenos vivos hasta la local 133; la ROM nueva tiene **0** píxeles amarillos y **0** píxeles xenos por debajo de la local 63 en 34 fotogramas de la secuencia de asedio. Capturas en `artifacts/baseline/run/` y `artifacts/no-range-full-screen/`.
    - Regresión verde: `test_phase1_complete`, `test_wall_frontline_damage`, `test_wall_hp_and_crate`, `test_full_upgrade_progression`, `calibration_test`, `calibration_renumber_check`, `rebalance_verification`, `full_screen_targeting`.
    - **Hallazgo preexistente (no introducido por esta sesión):** el sandbox de diagnóstico (`L+SELECT`) no se activa bajo escenarios DeSmuME; `sandbox_verification.json` falla con `screen_unchanged` de forma idéntica en `main` y en la ROM nueva. Los escenarios nuevos de evidencia usan modo oleada con la batería en overdrive (`L+R`) en lugar del sandbox.
  - [x] **Sesión 5: Muerte de los enemigos — gore, licuado y trozos [COMPLETADA - commit `aa9af3a`]:**
    - Retirada la salpicadura que se estampaba en **cada impacto** (vector bala nulo → se resolvía como norte): un impacto no letal ya no deja marca.
    - **Muerte animada** (15 frames): el cuerpo cae píxel a píxel por gravedad hasta sus pies y cada píxel queda **permanente** en el suelo (licuado).
    - **Trozos reales del sprite** (bloques de 3-4 px) que salen despedidos, rebotan y **se queman en el suelo** al aterrizar (también permanentes).
    - **Sangre en paleta roja** (regla cromática xenos, `DESIGN.md` §7) para no confundirla con el enjambre.
    - Cono direccional grande **disponible tras `DEATH_CONE_ENABLED`** (por defecto apagado, listo para activar).
    - Fix de doble buffer: los charcos se borraban en el *page flip* (ahora cada estampado marca su área, `mark_ground_stamp`); fix de la normalización Q8 del vector bala.
    - Evidencia y detalle: `walkthroughs/splatter-impact-direction/walkthrough.md` (A/B antes/después, estudio de ablación y coste medido).

## Fase Actual: Hito 4 (City Defense Full Incremental) - EN PROGRESO

> Transición completa desde el modelo TD tradicional / híbrido hacia un City Defense Incremental puro: stream continuo de spawn regulado por diales del Atraedor, eliminación de Game Over, generador de 7 tiers de automatización con degradación por daño, batería unificada (1 lógica / 4 sprites visuales), y economía basada en chatarra directa y vetas minables.
> 
> **Genealogía de Ramas:**
> - `classic-towerdefense`: Preservación histórica del TD inicial.
> - `hybrid-clicker-td`: Preservación del híbrido previo (batería, recarga manual, gore 60 FPS).
> - `full-incremental`: Rama activa de trabajo por defecto.

- [x] **Demo Vertical Slice (Bucle continuo y degradación sin Game Over) [COMPLETADA]:**
  - [x] Presupuesto de spawn continuo en enteros (`budget += rate * dt`).
  - [x] Normalización de tiers de enemigos (T1: 3 HP / 1 scrap, T2: 15 HP / 5 scrap).
  - [x] 1 torreta lógica / 4 sprites visuales con alternancia rotatoria de disparo en pantalla inferior.
  - [x] Andamio + Generador de 7 tiers (5 HP por tier) sustituyendo al muro con Game Over.
  - [x] Validación determinista de 60 FPS en DeSmuME con captura/GIF de evidencia (`walkthroughs/city-defense-prototype/`).

- [x] **Cierre de Fase 1 (Generador Aditivo, Tienda de 7 Nodos, Pantalla Fin de Demo y Calibración Recableada) [COMPLETADA]:**
  - [x] Generador aditivo arranca en 0 tiers construidos (`built_tiers = 0`) con armazón industrial de andamiaje.
  - [x] Compra de A1 (Hold-to-fire, 100 scrap) erige Tier 1 (5 HP).
  - [x] Compra de A2 (Auto-apuntado, 350 scrap, req A1) erige Tier 2 (5 HP) y dispara la pantalla de Fin de Demo (`MODE_VICTORY`).
  - [x] Pantalla de Fin de Demo interactiva: muestra estadísticas, permite continuar en modo infinito (`A`) con auto-apuntado activo o reiniciar (`B`).
  - [x] Menú de calibración (`SELECT`) completamente recableado para el stream continuo: Página 0 (Atraedor & Generador) y Página 3 (Costes de los 14 nodos de la economía).
  - [x] Validación determinista en DeSmuME (`scenarios/phase1_vertical_slice.json`: `DSM_SCENARIO_RESULT=PASS captures=7 events=94`).
  - [x] Documentación y evidencias registradas en `walkthroughs/phase1-vertical-slice/`.

- [x] **Consolidación Estética y Simulación Continua [COMPLETADA]:**
  - [x] Eliminada la fila obsoleta de 32 bombillas de muro en $Y=188$ (incompatible con City Defense).
  - [x] Reutilizadas las bombillas de cátodo diegéticas: 5 micro-bombillas de filamento/fósforo verde por cada una de las 7 bahías del Generador (parpadeo de daño y apagado al recibir impacto).
  - [x] Limpieza del HUD superior retirando la telemetría cruda `GEN:[X0]...` y sustituyéndola por `GENERATOR: X/7 TIERS ACTIVE`.
  - [x] Dial del Atraedor 100% continuo: acumulación fraccionaria Q8 en tasa de spawn y mezcla probabilística suave entre tiers de biocastas ($T1.3 \rightarrow 70\%$ T1 / $30\%$ T2).
  - [x] Validación determinista en DeSmuME (`scenarios/phase1_vertical_slice.json`: `DSM_SCENARIO_RESULT=PASS captures=7 events=94`).

- [x] **Refactor de Batería, Cadencia y Munición Unificada [COMPLETADA]:**
  - [x] Desvinculada la cadencia fija (6 ticks) para respetar `g_balance.turret_fire_interval` (nivel 0 = 12 ticks / ~5 disparos/s). Los cambios en menú de calibración surten efecto inmediato.
  - [x] Indicador unificado de batería: eliminadas las barras individuales bajo las cúpulas; barra de munición compartida sobre el depósito ($X=128, Y=153$).
  - [x] Consumo real de munición (1 bala por tiro) y bloqueo en `RELOAD`. Recarga táctil arrastrando con stylus desde el búnker ($X \in [110..146], Y \in [150..180]$) hacia la línea de batería.
  - [x] ROM compilada con BlocksDS y subida a la consola física (`towerdefense.nds`).

- [x] **Dial de 0.05 con Autorepeat y Compuerta Correcta del Hold [COMPLETADA]:**
  - [x] Dial del Atraedor reescrito en **ticks de 0.05** (tasa `0.00..10.00/s`, tier `T1.00..T4.00`) como fuente de verdad, con vistas Q8 derivadas (`dial_rate_sync`/`dial_tier_sync`); HUD con **dos decimales** (`DIAL:X.XX/s`, `TIER:TX.XX`).
  - [x] **Autorepeat con rampa** al mantener la cruceta (retardo 10 f → cada 4 f → cada 2 f → cada frame).
  - [x] **Acumulador de spawn corregido** (unidades `rate_q8 * frames`, umbral `60 * 256`): elimina el truncado por `/60` que impedía generar por debajo de ~0.23/s.
  - [x] **Bug del Hold:** la compuerta usaba `generator.tiers[0].active`; ahora exige `upgrades.continuous_fire && tiers[0].active`. Erigir/reparar la bahía del Generador sin comprar A1 ya no desbloquea la ráfaga.
  - [x] **Evidencia A/B determinista** en DeSmuME (mismo gesto: reparar bahía + hold 180 f): baseline (`full-incremental` @ `e938e38`) **15 disparos** (munición 40→25) vs ROM nueva **1 disparo** (40→39). Dial: `DIAL:0.50 → 0.55` (tap) `→ 2.20` (hold 60 f); `TIER:T1.00 → T1.10`.
  - [x] Regresión verde: `scenarios/phase1_vertical_slice.json` (`DSM_SCENARIO_RESULT=PASS captures=7 events=94`).
  - [x] Escenarios nuevos: `scenarios/dial_step_accel.json`, `scenarios/hold_gate_shop.json`, `scenarios/hold_gate_repair_repro.json`.

- [x] **Hitbox Táctil Justa y Apuntado Persistente de la Batería [COMPLETADA - rama `feat/tap-hitbox-hold-aim`]:**
  - [x] **Hitbox táctil:** sustituido el círculo fijo de 24 px centrado en el **punto lógico** (los pies) del enemigo por la **caja real del sprite dibujado** (`enemy_get_frame_bounds()`, que replica `offset_x/offset_y` y la cota de vuelo `flight_altitude`) **unida** a un radio de gracia de 24 px alrededor del centro del sprite, con 6 px de margen; gana el más cercano al centro. Helper nuevo declarado en `include/enemy_data.h`.
  - [x] **Apuntado persistente:** nuevo campo `last_aim_angle[4]` en `WallPlatform`. La cúpula **mantiene la marcación de su último disparo** como postura de reposo en vez de resetear al ángulo por defecto (`[OQ-11]`). Unificado además el init hardcodeado `{0,2,2,4}` con la tabla de sockets `{0,1,3,4}` (una sola fuente de verdad).
  - [x] **Disparo libre confirmado:** se mantiene el comportamiento (tocar vacío dispara y consume bala/cadencia) y se **revoca** `DESIGN.md [OQ-06].4` (filtro táctil estricto), que nunca llegó a implementarse, para alinear el canon con el código.
  - [x] **Evidencia A/B determinista** (ROM nueva crc `2AF89A56` vs baseline `full-incremental` @ `45f72b7`, crc `3F2535A3`; mismo gesto de disparo lateral): en el baseline la cúpula vuelve **exactamente** a su postura de reposo (**0 px** de diferencia entre el estado previo al disparo y el final, medido en la región de la torreta) mientras la ROM nueva **mantiene el rumbo** (**771 px** de diferencia); el frame del disparo es idéntico entre ambas ROMs (0 px), lo que descarta cualquier confound de RNG. Medido con `tools/ab_frame_diff.py pair`. Imagen A/B en `walkthroughs/tap-hitbox-hold-aim/assets/aim_ab.png`.
  - [x] **Auditoría geométrica de la hitbox** (`tools/hitbox_geometry.py`): **8/8** variantes tienen píxeles del sprite fuera del círculo antiguo (Scourge variante 0: 7 px, distancia máx. 25.6 > 24; Ultralisk variante 7: 4.015 px, hasta 58.1). La nueva región (unión caja ∪ radio) nunca es más estricta que la anterior.
  - [x] **Límite honesto (sin adornos):** el A/B **funcional** con enemigos tempranos no discrimina, porque para la variante 0 el círculo antiguo ya cubría casi toda la silueta; la evidencia de esta hitbox es **geométrica**, no una captura diferencial. La sensación táctil final requiere validación en **DS física**.

- [x] **Tabla Maestra Única de Enemigos y Calibración por Tier [COMPLETADA - rama `full-incremental`]:**
  - [x] Unificadas las stats de enemigos en `GameBalanceConfig.enemy[]` (`EnemyStatDef` con `tier`): el spawn continuo, la recompensa de scrap al morir, el mordisco, el sandbox de debug y la página `ENEMY STATS` de calibración **leen de la tabla** (antes cada sistema tenía su copia o literales fijos).
  - [x] Tiers **T1..T4** (dos especies por tier): T1 Scourge+Zergling, T2 Hydralisk+Mutalisk, T3 Defiler+Lurker, T4 Guardian+Ultralisk. El **volador del par** = ⅓ de HP y ×3 de velocidad.
  - [x] Eliminados los duplicados muertos: `EnemyTypeDef.default_hp/scrap_value`, `config/balance/*.csv` + `tools/compile_balance.py`, `scripts/export_enemy_data.py` y los campos `sandbox.enemy_hp/enemy_speed`. Magic del balance `TOW6 → TOW7`.
  - [x] Evidencia: build `DS_BUILD=PASS` y `scenarios/master_table_tier_check.json` (`DSM_SCENARIO_RESULT=PASS captures=5 events=61`); detalle y capturas en `walkthroughs/master-enemy-table/`.
  - [ ] **Pendiente:** rebalancear velocidad/daño de mordisco (provisionales) y diferenciar T3/T4; validar la edición por cruceta + guardado en **DS física**.

- [x] **Purga del Sistema Legacy de Torretas (Entidad `Turret`/`Bullet`) [COMPLETADA - rama `feat/purge-legacy-turrets`]:**
  - [x] Eliminada la **entidad de torreta colocable** (código 100% inerte de una versión vieja: nada asignaba `placed = 1`): structs `Turret`/`Bullet`, `g_turrets[]`/`g_bullets[]`, `spawn_bullet`, `BULLET_SPEED`, `MAX_TURRETS`/`MAX_BULLETS`, el bucle de update/aim/fire de torretas, el bucle de colisión de balas, el bloque "enemy bites turret", `game_is_pos_valid` y todos sus resets/`memset`.
  - [x] Eliminado su render muerto: `renderer_draw_turret`, `renderer_draw_bullets` (y su llamada), `tiles_draw_turret_base`, `tiles_draw_twin_bolters`, `C_SPARK_*` y colores `COLOR_TURRET_*`/`COLOR_BARREL_*`.
  - [x] Retirado el módulo generado `source/turret_data.c` / `include/turret_data.h` y el paso `build_turrets()` de `scripts/build_assets.py` (ya roto: buscaba masters archivados); helper huérfano `rotsprite_pil` eliminado.
  - [x] Sandbox limpiado: retirados los knobs inertes `turret_firerate`/`turret_damage` (editor a 1 fila: especie); se conserva `turret_infinite_ammo` (vivo, lo lee la muralla).
  - [x] Escenarios: rótulos obsoletos `turret*` renombrados; `pause_and_turret_select_test.json` → `pause_and_wave_ui_test.json`.
  - [x] **Se conserva intacta la muralla** (`WallPlatform`/`BulletDart`) y su nomenclatura interna `turret_*` (`turret_angles`, `c_turret_points`, `TURRET_PIVOT_X/Y`).
  - [x] Evidencia: `DS_BUILD=PASS` (sin `turret_data.c`, 0 warnings) y regresión verde (`DSM_SCENARIO_RESULT=PASS`) en `city_defense_demo`, `session1_wall_ballistics`, `combat_and_kill_test`, `targeting_and_sprites_test`, `master_table_tier_check`. Detalle en `walkthroughs/purge-legacy-turrets/`.

- [x] **Desmembramiento en Vivo del Enemigo (feedback de daño) [COMPLETADA - rama `feat/live-dismemberment`, merge `7000e24`]:**
  - [x] Cada impacto no letal **muerde una celda del borde de la silueta** (nunca del interior: el xeno se *pela* de fuera hacia dentro) y lanza **ese mismo trozo** como escombro sólido con los índices de paleta reales del sprite.
  - [x] La mordida es **irregular** (patrón determinista sobre bloques de 2×2, compartido entre el dibujo y el escombro, de modo que el trozo que vuela es exactamente el que falta) y la zona mordida muestra **carne roja** de caparazón arrancado (paleta de gore, `DESIGN.md` §7), nunca púrpura.
  - [x] **Presupuesto atado a la salud** (`WOUND_HP_TICKS`): sólo está disponible la fracción de vida ya perdida, así que un xeno casi intacto no se destroza por muchos tiros que reciba y el cupo completo sólo se alcanza al borde de la muerte. La herida se resuelve **después** de aplicar el daño y un **golpe letal no genera ninguna** (de ese cuerpo ya se encarga el licuado).
  - [x] **Dos modos compilables** (`WOUND_AMPUTATE`): por defecto **sin amputación** (el caparazón se enrojece sin romper la silueta); con amputación se arrancan además los apéndices de ≤ 2 px y todo lo que un corte deje desconectado, sin dejar nunca fragmentos flotantes.
  - [x] **Fast path de quads intacto** para enemigos sin heridas. Medido con `scenarios/perf_wound.json` (6 Ultralisks): `B` pasa de 553 a 565 ticks (**+12, ≈+2.2%**); el coste escala por **sprite herido**, no por celda mordida.
  - [x] **Hallazgo colateral:** el sandbox de debug **sí entra** en DeSmuME headless; el fallo registrado en `KNOWN_ISSUES.md` era de **timing de captura**, no de input. `KNOWN_ISSUES.md` corregido.
  - [x] **Nueva pregunta abierta `[OQ-13]`** en `DESIGN.md`: la escalera de HP (1→135) rompe la legibilidad del feedback en los extremos. **El balance del juego NO se ha tocado**; las capturas usan una tabla de HP provisional sólo para la grabación.
  - [x] Evidencia: 8 GIF **A/B (con y sin amputación)** con un solo individuo, más el A/B de rendimiento, en `walkthroughs/live-dismemberment/`.

- [ ] **Bugs y Tareas Pendientes para Siguiente Agente:**
  - [x] **Tasa inicial de spawn del Atraedor:** arranca en $0.50$/s (`dial_rate_ticks = 10`) con paso fino de $0.05$.
  - [ ] **Interacción del Dial táctil (stylus):** implementar el arrastre analógico con stylus (arriba/abajo e izquierda/derecha); el control por cruceta con pasos de $0.05$ y autorepeat ya está resuelto.
  - [ ] **Permeabilidad del Andamio:** verificar que mientras `built_tiers == 0`, los enemigos atraviesen el andamio por debajo sin bloquearse ni acumularse.
  - [ ] **Limpieza de conos en pantalla superior:** certificar que ningún trazador o salpicadura de cono se dibuje en la pantalla superior.
