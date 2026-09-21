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
    - Desbloqueo de sockets (1 a 4 torretas), automatización de gatillo y auto-aim.
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



