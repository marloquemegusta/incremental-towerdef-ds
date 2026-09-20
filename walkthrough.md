# Walkthrough: Eliminación del Rango — Fuego Total en la Pantalla Inferior

**Sesión:** `feat/no-range` · **ROM:** `0A237779` · **vs `main`:** `767EBBAC` · **Commit:** `e90c6c0`

Se ha eliminado el concepto de rango/alcance del juego. La muralla solo podía batir la franja
`Y` local `64..143` (≈55 % del campo útil) por culpa de una línea de fuego en `Y=64` que el
jugador no percibía como una mejora. Ahora **toda la pantalla inferior** (`Y` local `0..143`) es
targeteable, tanto por auto-apuntado como por stylus.

Este documento se actualiza con cada petición de la sesión.

> **Cómo ver las imágenes:** las rutas son relativas a este archivo
> (`artifacts/walkthrough/...`), así que basta con abrir el `.md` en un visor de markdown desde
> la raíz del repo. Si prefieres un visor único sin dependencias de ruta, usa
> [`walkthrough.html`](walkthrough.html), que lleva toda la media embebida en base64.

### Peticiones de esta sesión

| # | Petición | Resultado |
| :---: | :--- | :--- |
| 1 | Eliminar el concepto de rango para que toda la pantalla inferior sea targeteable | Implementado y verificado (secciones 1-6) |
| 2 | Enseñar capturas que demuestren que funciona | Evidencia visual entregada (sección 1) |
| 3 | Recordar el formato `walkthrough.md` por sesión, actualizado en cada petición | Preferencia guardada + este documento |
| 4 | Que el walkthrough salga pre-renderizado en un visor aparte | Rutas relativas + `walkthrough.html` autocontenido |
| 5 | Commitear a `main`, mergear y limpiar | Merge fast-forward a `main` (`27b8a58`), rama `feat/no-range` borrada, temporales de sesión eliminados |
| 6 | `git push` y subir la ROM a la DS | `origin/main` al día (`bfd877a`) y `towerdefense.nds` subido al FTP de la consola (PASS) |

---

## 1. Demostración Visual (DeSmuME headless, motor ARM9 real)

Todas las imágenes y GIF son capturas reales de DeSmuME ejecutando la ROM
(`scripts/run-scenario.ps1`). Los GIF se ensamblan con ffmpeg a partir de los PNG capturados
dentro del motor; no hay contenido sintético.

### A. Antes y después con la misma secuencia de entrada

Misma partida, mismos inputs, mismos fotogramas. La primera imagen es la ROM de `main`
(crc `767EBBAC`) y la segunda la ROM nueva (crc `0A237779`). Nótese la **franja amarilla
discontinua** en la primera y su ausencia total en la segunda.

**`main` — la línea de rango en Y=64 sigue ahí, con enjambre vivo por debajo:**

![Campo de batalla en main](artifacts/walkthrough/01_baseline_field.png)

**ROM nueva — sin línea, todo el enjambre cae en la mitad superior:**

![Campo de batalla en la ROM nueva](artifacts/walkthrough/02_new_field.png)

### B. Recorte ampliado x4 de la banda Y=64 (zoom real, sin alterar contenido)

**`main`:** la pintura vial amarilla, con tres zerglings detenidos justo encima y salpicaduras
de muerte alrededor. Es literalmente el viejo límite de fuego.

![Primer plano de la banda Y=64 en main](artifacts/walkthrough/03_closeup_baseline_y64.png)

**ROM nueva:** empedrado limpio, sin rastro de demarcación y sin enemigos que lleguen ahí.

![Primer plano de la banda Y=64 en la ROM nueva](artifacts/walkthrough/04_closeup_new_y64.png)

### C. GIF animado: 600 frames de asedio (30 capturas @ 3 fps, velocidad real)

**`main`** — el enjambre atraviesa la línea y muere por debajo de ella:

![Asedio en main](artifacts/walkthrough/baseline_engagement.gif)

**ROM nueva** — el enjambre es batido nada más entrar en la pantalla inferior:

![Asedio en la ROM nueva](artifacts/walkthrough/no_range_engagement.gif)

### D. Tienda: 7 cartas, sin RANGE

![Menú de mejoras con 7 cartas](artifacts/walkthrough/05_shop_7_cards.png)

### E. Calibración renumerada (páginas 2 y 3)

**Página 2, filas 10-19:** `CADENCE LV4 = 3`, luego `CONVEYOR LV0..LV4 = 9999/60/25/12/6` y
`MAGAZINE LV0..LV3 = 10/16/25/40`, cada valor alineado con su etiqueta.

![Calibración página 2](artifacts/walkthrough/06_calibration_page2.png)

**Página 3, últimas filas:** termina en `SOCKET 3 (ROF>=4) = 500`; ya no existen las filas
`RANGE LVn COST`.

![Calibración página 3](artifacts/walkthrough/07_calibration_page3.png)

### F. Medición por píxeles (no solo el `PASS` del runner)

| Métrica (34 fotogramas del asedio) | `main` | ROM nueva |
| :--- | :---: | :---: |
| Píxeles amarillos en `Y` global 256-257 (local `Y=64`) | **186 - 201** | **0** |
| Píxeles xenos por debajo de la local 63 | **73 - 174** | **0** |
| Profundidad máxima alcanzada por un xeno | local **99 - 133** | **ninguno llega** |

---

## 2. Qué se ha conseguido

- **Toda la pantalla inferior es targeteable.** El auto-apuntado y el stylus alcanzan cualquier
  enemigo vivo en `Y` local `0..143`, sin franjas muertas.
- **El concepto de rango no existe en ninguna superficie visible:** ni línea en el empedrado, ni
  carta en la tienda, ni controles de rango en sandbox/calibración.
- **Reversibilidad conservada:** la mecánica sigue siendo data-driven. `g_balance.turret_range[]`
  está a `0`; poner un valor `>0` reintroduce una línea de fuego sin recuperar código.
- **Regresión limpia:** la tríada canónica, la progresión de mejoras, la tienda y la calibración
  pasan al 100 %.

## 3. Resumen de cómo

- **Neutralizar en vez de arrancar:** `turret_range[]` → `{0,0,0,0,0}`. La puerta de auto-fuego
  (`local_gy >= range_line_y`) y el filtro táctil de combate quedan inertes, sin tocar su lógica.
- **Purga de lo visible y de lo muerto:** fuera el horneado de la franja amarilla en `tiles.c`,
  `renderer_draw_range_perimeter()` (nunca se llamaba), `WALL_TURRET_RANGE`, la carta RANGE con sus
  costes y su hitbox, y los controles de rango del sandbox y de calibración.
- **Renumeración de índices** en calibración (páginas 2 y 3): etiquetas, mapeo de valores,
  `max_rows`, `last` del renderer y tamaño de paso. Aquí apareció y se corrigió un resto que daba
  paso `2` a las filas que ahora son `CONVEYOR`.
- **Magia del balance `TOW5` → `TOW6`:** sin esto, una SD real con un save antiguo habría
  restaurado `turret_range[0] = 64` y reintroducido el bloqueo a `Y=64` **de forma invisible**,
  porque la línea visual ya no existe.
- **`g_turrets` / círculos por torreta:** deliberadamente intactos. Es código inerte (nada asigna
  `placed = 1`) y tocarlo habría sido ruido sin beneficio para el objetivo.

## 4. Archivos Modificados

- `include/game.h`: campos de rango marcados como dormidos; retirados `WALL_TURRET_RANGE` y
  `renderer_draw_range_perimeter()`; magia `TOW6`.
- `source/simulation.c`: `turret_range = 0`; carta/coste/switch del slot 7 retirados; renumber de
  calibración; sandbox sin fila de rango.
- `source/renderer.c`: 7 cartas; etiquetas y mapeo de calibración renumerados; círculo ámbar de
  rango del sandbox eliminado.
- `source/tiles.c`: retirado el horneado de la franja amarilla en `Y=64`.
- `config/balance/upgrades.csv` y `tools/compile_balance.py`: 7 mejoras.
- `AGENTS.md` (regla 10 reescrita como *Fuego Total en Pantalla Inferior*), `DESIGN.md`
  (`[OQ-06]`), `STATUS.md` (Sesión 4), `docs/BALANCE_DATA.md`.
- Escenarios nuevos: `scenarios/full_screen_targeting.json` y
  `scenarios/calibration_renumber_check.json`.

## 5. Pruebas y Verificación

- **Build BlocksDS:** `DS_BUILD=PASS` sin warnings.
- **Tests host:** `DS_HOST_TESTS=SKIP reason=no-host-tests` (el contrato declara `hostTests: []`).
- **Escenarios en DeSmuME (todos `PASS`):** `full_screen_targeting`, `calibration_renumber_check`,
  `test_phase1_complete`, `test_wall_frontline_damage`, `test_wall_hp_and_crate`,
  `test_full_upgrade_progression`, `calibration_test`, `rebalance_verification`.
- **Evidencia A/B:** ROM `main` compilada en un worktree temporal y ejecutada con la misma
  secuencia de entrada, para comparar fotograma a fotograma.

## 6. Límites y Hallazgos

- **Game feel no demostrable con capturas:** el enjambre ahora muere nada más entrar en pantalla,
  así que la dificultad efectiva habrá bajado y se ha perdido el sumidero de chatarra de la mejora
  RANGE. Eso solo se juzga jugando.
- **Hallazgo preexistente (no introducido por esta sesión):** el sandbox de diagnóstico (`L+SELECT`)
  no se activa bajo escenarios DeSmuME. `sandbox_verification.json` falla con `screen_unchanged` de
  forma **idéntica** en `main` y en la ROM nueva, así que los escenarios de evidencia usan modo
  oleada con overdrive (`L+R`).
- **Desviación de proceso:** no se pudo usar el worktree físico que exige `AGENTS.md`.
  `enter_worktree` no existe en sesión de escritorio y los worktrees hermanos
  (`C:/codexlocal/towerds-*`) quedan fuera del espacio de trabajo editable. Se trabajó en la rama
  `feat/no-range` en su sitio; `main` permanece intacta.
- **Validación física pendiente:** audio y comportamiento en hardware real siguen sin verificar.
