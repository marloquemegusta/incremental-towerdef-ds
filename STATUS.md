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

