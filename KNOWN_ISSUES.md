# Known issues

## Sandbox visual validation

- **Diagnóstico corregido (sesión 8):** el sandbox de debug **sí entra** en DeSmuME headless. Lo que
  fallaba era el **contrato de captura**, no el input: `sandbox_verification.json` y
  `melt_persistence_debug.json` capturaban **antes del primer render completo del HUD**, así que
  comparaban fotogramas idénticos y devolvían `screen_unchanged`. Con **≥ 90 fotogramas de arranque**
  antes de la primera captura, `L + SELECT` entra en el sandbox, el **D-Pad** cambia de especie y el
  **tap** spawnea en la pantalla inferior.
- **Pendiente real:** corregir esos dos escenarios (retrasar su primera captura). No es un defecto del
  juego.
- Por tanto, el enfrentamiento, la dirección y la animación enemigas **sí** se pueden validar
  visualmente por el flujo del sandbox: así se generó `walkthroughs/live-dismemberment/`.

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
  - *Aclaración (sesión 8):* los **trozos de escombro** (`GoreChunk`) y los casquillos que saltan al impactar son **intencionados** y van en paleta xenos/latón; no son esta estela. Al juzgar el defecto hay que ignorarlos.
- **Limpieza de pantalla en Game Over / Reinicio:** Al morir el muro y reiniciar partida, los búferes y la pantalla no se limpian/redibujan por completo: permanecen salpicaduras de sangre previas y artefactos rojizos/amarillentos en el centro de la pantalla superior.
  - *Revisado (sesión 5):* la sangre vive en el *ground cache* y sólo se reconstruye en `tiles_init()`, que se llama al **empezar partida nueva** (boot / reintento tras game over / reinicio). Por diseño no debería sobrevivir a un reinicio; queda por confirmar en hardware y por separado los "artefactos rojizos/amarillentos" de la pantalla superior.

## Rendimiento en Picos de Oleada

- **Caída a 30 FPS en pico máximo:** Durante la saturación máxima de enemigos en pantalla, el framerate cae a ~30 FPS (a pesar de la optimización de los bucles de blit y el recorte de dirty rects a 32x32). Requiere optimización en ensamblador ARM9 o procesamiento por franjas/DMA.
  - *Revisado (sesión 5):* en combate real (5 enemigos) el HUD da `FPS:60 T:44 B:204 P:53 S:31` (≈332/545 ticks). **No** se ha medido la saturación máxima (384 enemigos), así que el issue sigue abierto: pendiente de un escenario de estrés.
  - *Revisado (sesión 8):* ya existe escenario de estrés (`scenarios/perf_wound.json`). Con **6 Ultralisks** (el sprite mayor) en la zona de fuego el HUD da `B:553` ticks, **por encima** del presupuesto de 545 ⇒ 30 FPS: las siluetas grandes saturan la pantalla inferior **por sí solas**, sin necesidad de llegar a 384 enemigos. El desmembramiento en vivo añade solo **+12 ticks** (≈+2.2%) encima. Sigue sin medirse la saturación máxima de 384 enemigos, que probablemente sea mucho peor.
