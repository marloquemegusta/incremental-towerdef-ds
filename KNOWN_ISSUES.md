# Known issues

## Sandbox visual validation

- The debug sandbox is integrated into `main` and can spawn enemies manually.
- The sandbox scenario has required fixes to its input/capture contract, but
  headless DeSmuME still does not reliably enter the sandbox through the
  automated hotkey/touch sequence. A successful visual capture with spawned
  enemies is still pending.
- Because of that, enemy facing and animation have not yet been confirmed
  visually on the DS with the sandbox workflow.

## Enemy sprites

- The Hydralisk direction sheet is normalized to the canonical order
  `N, NE, E, SE, S, SW, W, NW`.
- The small rotated enemy previously showed occasional vibration or apparent
  sprite corruption. Its rotation canvas is now fixed-size to avoid bounds
  changes and diagonal clipping, but this fix still needs confirmation on
  hardware.
- If the Hydralisk still walks backwards on the DS, the remaining suspect is
  the source sheet's front/back interpretation rather than the renderer's
  direction indexing.

## In-Game Controls & Menus

- **Pausa inaccesible en hardware:** Ningún botón activa el menú de pausa durante el combate/preparación; se requiere revisar la captura de input en hardware real (Start/Select/Lid).

## Artefactos Visuales & Renderizado

- **Estela de puntos marrones tras enemigos:** Aunque se eliminó la estela de sprites enteros mediante dirty rects robustos, los enemigos dejan tras de sí una traza de puntos marrones que no se restaura del todo del fondo original.
- **Limpieza de pantalla en Game Over / Reinicio:** Al morir el muro y reiniciar partida, los búferes y la pantalla no se limpian/redibujan por completo: permanecen salpicaduras de sangre previas y artefactos rojizos/amarillentos en el centro de la pantalla superior.
  - *Revisado (sesión 5):* la sangre vive en el *ground cache* y sólo se reconstruye en `tiles_init()`, que se llama al **empezar partida nueva** (boot / reintento tras game over / reinicio). Por diseño no debería sobrevivir a un reinicio; queda por confirmar en hardware y por separado los "artefactos rojizos/amarillentos" de la pantalla superior.

## Rendimiento en Picos de Oleada

- **Caída a 30 FPS en pico máximo:** Durante la saturación máxima de enemigos en pantalla, el framerate cae a ~30 FPS (a pesar de la optimización de los bucles de blit y el recorte de dirty rects a 32x32). Requiere optimización en ensamblador ARM9 o procesamiento por franjas/DMA.
  - *Revisado (sesión 5):* en combate real (5 enemigos) el HUD da `FPS:60 T:44 B:204 P:53 S:31` (≈332/545 ticks). **No** se ha medido la saturación máxima (384 enemigos), así que el issue sigue abierto: pendiente de un escenario de estrés.
