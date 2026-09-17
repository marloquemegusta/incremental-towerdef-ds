# STATUS.md - Estado de Desarrollo

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
  - [ ] **Sesión 2: Marea Continua y Picos de Alerta [SIGUIENTE SESIÓN / PRÓXIMO CHAT]:**
    - Spawner de marea incesante de Zerglings terrestres y Scourges aéreos cruzando de pantalla superior ($Y=0$) a inferior ($Y=192$).
    - Temporizador y picos de alerta pautados ("¡BRECHA DETECTADA!") con hordas compactas.
    - HUD y telemetría superior: contador de bajas, chatarra acumulada, tiempo de supervivencia y aviso de horda.
    - Sincronización de daño de mordiscos de enjambre en tiempo real a las 32 lámparas catódicas del muro.
  - [ ] **Sesión 3: Árbol Visual de Mejoras, Pausa Táctica y Balance [PENDIENTE]:**
    - Pantalla de Árbol Tecnológico táctil con nodos interconectados y pistas de cobre.
    - Pausa táctica total al abrir la tienda para pensar y relajar la mano.
    - Escalera de automatización: Gatillo Continuo (`Hold`) y Cogitador de Tiro (`Auto-target` + override manual).
    - Mejoras de daño, cadencia de muro, desbloqueo de 2º socket, reciclaje de chatarra y blindaje/reparación.
    - Bucle de Game Over / Reset instantáneo con estadísticas de run.



