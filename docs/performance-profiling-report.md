# Informe de profiling de rendimiento

Fecha: 2026-09-18  
ROM auditado: `game.nds` generado desde el worktree actual  
Entorno: BlocksDS + DeSmuME 0.9.14 headless  
Profiler: sandbox diagnóstico integrado en `main.c`, `renderer.c` y `simulation.c`

## Resumen ejecutivo

El peor caso reproducible es una carga de 384 enemigos. El perfil móvil mantiene las 384 entidades activas y evita que la oleada termine, por lo que no mezcla el coste de render con una pantalla de victoria.

Resultados principales:

| Perfil | Separación | `S` simulación | Estado |
|---|---:|---:|---|
| 384 enemigos fijos | OFF | ~85 ticks | `ALIVE:384 TOTAL:384` |
| 384 enemigos fijos | ON | ~175 ticks | `ALIVE:384 TOTAL:384` |
| 384 enemigos móviles | OFF | ~192 ticks | `ALIVE:384 TOTAL:384` |
| 384 enemigos móviles | ON | ~192 ticks | `ALIVE:384 TOTAL:384`, `Q` activo |

Los ticks son lecturas del timer del profiler, no milisegundos directos. El presupuesto documentado por el proyecto es de 545 ticks por frame.

Conclusiones:

1. El movimiento real añade aproximadamente 107 ticks frente al perfil fijo.
2. La separación física añade aproximadamente 90 ticks en el perfil fijo.
3. En el perfil móvil el frame queda saturado y el HUD redondea ambos casos a `S~192`; el contador `Q` confirma que la separación sí se ejecuta, pero no permite aislar su coste con suficiente resolución en ese régimen.
4. El render inferior (`B~192`) y la presentación framebuffer siguen siendo costes estructurales importantes.
5. La UI del profiler (`U`) es cara y contamina la medición; no debe usarse como representación del coste del juego normal.

## Qué mide el profiler

`main.c` separa el frame en:

- `S`: actualización de simulación.
- `T`: render superior.
- `B`: render inferior.
- `P`: presentación/copia de framebuffers.
- `R`: restauración de fondo y dirty rectangles del inferior.
- `E`: sprites de enemigos.
- `F`: pared, balas y efectos.
- `U`: HUD y profiler.
- `Q`: consultas de separación, targeting y colisiones.

`P` incluye las copias DMA de ambos framebuffers y puede contener overhead adicional del emulador. Por eso no se interpreta como coste puro del código de juego sin confirmación en hardware.

## Matriz de pruebas ejecutada

### 1. Carga progresiva

Escenario: `scenarios/performance_sandbox_load.json`  
Evidencia: [audit-full-load](../artifacts/audit-full-load)

Se intentaron capturas de 100, 200, 300 y 384 entidades. El manifest pasa, pero las capturas intermedias de 100/200/300 se producen antes de que el estado visual se estabilice. No se usan como evidencia cuantitativa. La prueba sirvió para detectar y corregir el defecto del escenario.

### 2. Stress original

Escenario: `scenarios/performance_sandbox_stress_run.json`  
Evidencia inicial: [audit-full-stress](../artifacts/audit-full-stress)

El escenario original llegaba a 384 entidades, pero después los enemigos morían y la pantalla acababa en victoria. Esa ejecución se conserva como evidencia histórica, pero no se usa para promedios.

### 3. Stress fijo estable

Evidencia: [audit-fixed-stress-r4](../artifacts/audit-fixed-stress-r4)

El sandbox de stress fue endurecido para:

- desactivar torretas y proyectiles;
- mantener `ALIVE:384 TOTAL:384`;
- evitar transición a victoria;
- mantener la población durante toda la ventana.

Este perfil aísla bien render y coste algorítmico con entidades quietas, pero no representa una oleada real.

### 4. A/B de separación en carga fija

Evidencia:

- [audit-sep-on-r4](../artifacts/audit-sep-on-r4)
- [audit-sep-off-r4](../artifacts/audit-sep-off-r4)

Ambos escenarios mantienen 384 entidades y la misma composición visual. El HUD muestra `Q` activo con separación y `Q:0/0/0` sin separación.

Resultado observado:

| Caso | `S` |
|---|---:|
| Separación OFF | ~85 |
| Separación ON | ~175 |
| Diferencia | ~90 ticks / ~106% sobre la base |

La diferencia fue reproducida en dos parejas de ejecución y las capturas mantuvieron `ALIVE:384 TOTAL:384`.

### 5. Stress móvil

Evidencia: [audit-moving-stress-r1](../artifacts/audit-moving-stress-r1)

Se restauró la velocidad de los enemigos y se añadió wrap controlado al alcanzar la muralla. Así se conserva una carga móvil sin permitir que la oleada termine.

Resultado frente al perfil fijo sin separación:

| Caso | `S` |
|---|---:|
| Fijo, separación OFF | ~85 |
| Móvil, separación OFF | ~192 |
| Coste adicional del movimiento | ~107 ticks |

Este es el perfil más representativo de una oleada masiva continua.

### 6. A/B de separación en carga móvil

Evidencia:

- [audit-moving-sep-on-r1](../artifacts/audit-moving-sep-on-r1)
- [audit-moving-sep-off-r1](../artifacts/audit-moving-sep-off-r1)

Ambas ejecuciones mantienen 384 enemigos móviles. La variante ON muestra consultas `Q` de separación; la variante OFF muestra `Q:0/0/0`. Sin embargo, ambas lecturas agregadas aparecen como `S~192`, porque el perfil móvil ya está cerca del límite de resolución/tiempo del profiler. No se debe convertir esta pareja en una cifra porcentual precisa; demuestra activación funcional, no una diferencia aislada con resolución suficiente.

### 7. Regresión de gameplay

Escenario: `scenarios/combat_fast_kill.json`  
Evidencia: [audit-fixed-combat](../artifacts/audit-fixed-combat)

Resultado: `DSM_SCENARIO_RESULT=PASS`.

## Hotspots de código

### Render inferior y restauración

- [renderer.c:523](../source/renderer.c:523): `renderer_draw_battlefield_bottom`.
- [renderer.c:676](../source/renderer.c:676): `renderer_draw_enemies_bottom`.
- [renderer.c:297](../source/renderer.c:297): ordenación/buckets de enemigos.

El frame recorre `MAX_ENEMIES`, restaura rectángulos anteriores y dibuja cada enemigo visible, barra de vida y bounds. Este es el primer objetivo de optimización.

### Movimiento y separación

- [simulation.c:989](../source/simulation.c:989): bucle principal de enemigos.
- [simulation.c:1047](../source/simulation.c:1047): separación en celdas vecinas.
- [simulation.c:921](../source/simulation.c:921), [simulation.c:986](../source/simulation.c:986) y [simulation.c:1199](../source/simulation.c:1199): reconstrucciones de la rejilla.

El movimiento real es el salto de coste más grande al pasar del perfil fijo al móvil. La separación es muy cara cuando se la aísla con entidades quietas.

### Queries de targeting y colisión

- [simulation.c:1253](../source/simulation.c:1253): targeting de torretas.
- [simulation.c:1291](../source/simulation.c:1291): targeting del sandbox.
- [simulation.c:1406](../source/simulation.c:1406): colisión de proyectiles.

Estas rutas ya usan `enemy_grid_collect`; no son el primer candidato a una reescritura hasta medirlas con torretas activas y una carga controlada.

## Prioridad de mejoras

| Prioridad | Mejora | Beneficio | Riesgo | Impacto jugable |
|---:|---|---|---|---|
| 1 | Fusionar/restaurar dirty rectangles y evitar restauraciones redundantes | Muy alto | Bajo | Ninguno visible |
| 2 | Omitir sprites completamente fuera de pantalla u ocultos por el parapeto | Alto | Bajo | Ninguno si se mantienen colisiones |
| 3 | Ejecutar separación cada 2 ticks o con presupuesto adaptativo | Alto | Bajo-medio | Enjambres algo menos compactos |
| 4 | Evitar reconstrucciones redundantes de la enemy grid | Medio-alto | Medio | Ninguno si se conserva snapshot válido |
| 5 | Reducir sorting y health bars bajo carga alta | Medio | Medio | Menor detalle visual/HUD |

## Limitaciones y calidad de la evidencia

- Las lecturas proceden de un HUD de baja resolución; no son trazas por frame.
- `S~192` en carga móvil no permite detectar diferencias pequeñas entre separación ON/OFF.
- DeSmuME usa CPU Interpreter y rasterizador multihilo; `P` no es equivalente a hardware DS.
- La carga fija es deliberadamente sintética: sirve para aislar render y separación, no para predecir por sí sola el comportamiento de una oleada.
- La carga móvil con wrap es la referencia principal para el rendimiento de gameplay masivo.

## Veredicto

El cuello de botella práctico es la combinación de movimiento de enemigos y render inferior masivo. La separación puede duplicar el coste de simulación en un perfil aislado, pero en una oleada móvil el frame ya está saturado y el HUD actual no ofrece resolución para atribuirle una cifra exacta adicional.

Antes de optimizar targeting o DMA, conviene atacar restauración/render y reducir la frecuencia de separación. Toda mejora debe repetir como mínimo el stress móvil, las dos variantes de separación y `combat_fast_kill`.


## Iteración de optimización 1+2: dirty rectangles y oclusión

### Baseline

Evidencia: [opt12-baseline-stress](../artifacts/opt12-baseline-stress) y [opt12-baseline-combat](../artifacts/opt12-baseline-combat).

La baseline mantuvo 384 enemigos y produjo aproximadamente: `ALIVE/TOTAL=384/384`, `B~192`, `S~192`, `FPS~13` en la ventana agregada visible.

### Cambios aplicados

En [renderer.c:545](../source/renderer.c:545) se evita restaurar el dirty rectangle anterior si el enemigo no ha cambiado de posición.

En [renderer.c:676](../source/renderer.c:676) se omite el dibujado de un enemigo cuyo sprite completo queda por debajo del borde de la muralla. La simulación y las colisiones continúan ejecutándose; solo se elimina trabajo visual invisible.

### Después

Evidencia: [opt1-after-stress](../artifacts/opt1-after-stress) y [opt1-after-combat](../artifacts/opt1-after-combat).

El build y `combat_fast_kill` pasan. En stress móvil:

| Métrica | Antes | Después | Diferencia |
|---|---:|---:|---:|
| `ALIVE/TOTAL` | 384/384 | 384/384 | 0 |
| `B` | ~192 | ~192 | 0 ticks HUD |
| `S` | ~192 | ~192 | 0 ticks HUD |
| `FPS` | ~13 | ~13 | 0 |

### Interpretación

No hay una ganancia medible en el stress móvil actual. Los 384 enemigos están moviéndose, por lo que la omisión de rectángulos estáticos casi nunca se activa, y la oclusión completa apenas aparece. La modificación es de bajo riesgo y conserva la regresión, pero no debe venderse como una optimización demostrada todavía. Para medir su beneficio hace falta un escenario específico con enemigos detenidos en la línea de parapeto y telemetría de mayor resolución que el HUD.


### Punto 1: dirty rectangles

Antes: `B~192`, `S~192`, `FPS~13`. Después: mismos valores en el stress móvil. La prueba no activa suficientemente el caso optimizado porque prácticamente todos los enemigos cambian de posición en cada frame. El cambio queda validado funcionalmente por build y regresión, pero con beneficio no demostrado en este workload.

### Punto 2: descarte por oclusión

Antes: todos los enemigos candidatos al bottom render llegan al camino de dibujo salvo el filtrado de pantalla existente. Después: se descarta cualquier sprite completamente por debajo de `g_wall.screen_y`. El stress móvil mantiene `B~192` antes y después, lo que indica que el workload no contiene suficientes sprites completamente ocultos para mover el contador. El cambio es seguro porque no elimina simulación ni colisiones.



## Optimización 1: separación física, antes/después

### Cambio aplicado

La separación distribuía los 384 enemigos en dos grupos alternos y cada enemigo se revisaba aproximadamente cada 2 ticks. Se cambió a cuatro grupos alternos: (i & 3) == (sim_ticks_elapsed & 3). Cada enemigo se revisa aproximadamente cada 4 ticks. La separación sigue activa y usa la misma rejilla espacial; solo baja su frecuencia temporal.

### Benchmark visual

Escenario: performance_swarm_600.json

- 384 enemigos.
- Posiciones iniciales deterministas pero irregulares: jitter por carril y profundidad.
- 600 frames de simulación.
- 60 capturas, una cada 10 frames.
- GIF a 6 fps, 10 segundos de duración.

Antes: [swarm-before.gif](../artifacts/swarm-before-r2/swarm-before.gif)  
Después: [swarm-after.gif](../artifacts/swarm-after-r2/swarm-after.gif)

Las imágenes PNG originales permanecen en [swarm-before-r2](../artifacts/swarm-before-r2) y [swarm-after-r2](../artifacts/swarm-after-r2).

### Resultado de performance

| Métrica visible | Antes | Después | Cambio |
|---|---:|---:|---:|
| Entidades | 384 | 384 | 0 |
| S simulación | ~195 | ~187 | -8 ticks / ~4% |
| FPS agregado | ~12 | ~13 | +1 FPS visible |
| Q separación | activo | activo | conserva funcionalidad |
| Duración | 600 frames | 600 frames | misma prueba |

La cifra es la lectura del HUD de una ejecución completa y no un promedio por frame exportado; sirve como comparación práctica, no como precisión de microbenchmark. La mejora visual es un enjambre algo más compacto y con menos corrección lateral frecuente, sin cambios de población ni transición a victoria.

### Validación de gameplay

La regresión pasó después del cambio:

- [swarm-after-combat](../artifacts/swarm-after-combat)
- DSM_SCENARIO_RESULT=PASS
- Build BlocksDS: DS_BUILD=PASS

### Evaluación

El cambio ofrece una mejora modesta pero medible en la carga móvil y conserva la identidad de la marea. El coste jugable es una separación menos inmediata: los enemigos pueden solaparse más durante hasta cuatro ticks. En el GIF posterior no se observa una ruptura visual grave, pero sí una compactación mayor. Recomiendo mantener esta versión como candidata y repetir la prueba sobre hardware DS antes de reducirla más.


## Optimización 3: reconstrucción redundante de enemy grid, antes/después

### Cambio aplicado

Antes de mover enemigos se construía la rejilla espacial dos veces sin que las posiciones hubieran cambiado: una vez antes de wall_update y otra inmediatamente antes del bucle de movimiento. Se eliminó la segunda construcción. La construcción posterior al movimiento se conserva porque targeting, proyectiles y colisiones necesitan las posiciones nuevas.

### Benchmark visual completo

Escenario: performance_swarm_600_full.json

- 384 enemigos.
- Marea caótica determinista.
- 600 capturas, una por frame, sin submuestreo.
- GIF a 60 FPS, 10 segundos.

Antes, 600 frames completos: [swarm-before-600f.gif](../artifacts/grid-before-full/swarm-before-600f.gif)  
Después, 600 frames completos: [swarm-after-600f.gif](../artifacts/grid-after-full/swarm-after-600f.gif)

PNG fuente antes: [grid-before-full](../artifacts/grid-before-full)  
PNG fuente después: [grid-after-full](../artifacts/grid-after-full)

### Resultado

| Métrica HUD | Antes | Después | Lectura |
|---|---:|---:|---|
| Entidades | 384 | 384 | idéntico |
| Render inferior B | ~192 | ~192 | sin cambio visible |
| Simulación S | ~125 | ~125 | sin cambio visible en HUD |
| FPS agregado | ~6 | ~6 | sin cambio visible |
| Capturas | 600 | 600 | idéntico |

La eliminación es correcta por análisis de dependencias: la primera rejilla sigue
siendo válida para wall_update y la siguiente se construye después de mover. Sin
embargo, el coste de esa reconstrucción es pequeño frente a sprites, restauración
y presentación, por lo que no produce una ganancia práctica apreciable en el
stress completo.

### Validación

- Build BlocksDS: DS_BUILD=PASS.
- Gameplay: [grid-after-combat](../artifacts/grid-after-combat), DSM_SCENARIO_RESULT=PASS.
- La comparación visual no muestra cambios de trayectoria, población ni layout.

### Compromiso

Prácticamente ninguno en gameplay. El riesgo era consultar una rejilla antigua
entre movimiento y targeting; se evita conservando la reconstrucción posterior
al movimiento. El beneficio real es pequeño: mantener el cambio puede ahorrar
trabajo en frames con cargas ligeras, pero no debe considerarse el siguiente gran
salto de rendimiento.


## Optimización de render: limitar recorrido de buckets, antes/después

### Cambio aplicado

El ordenamiento vertical de enemigos inicializaba y recorría los 192 buckets de altura completos. Ahora registra el bucket mínimo y máximo ocupado y solo recorre ese rango. No cambia el orden relativo ni el contenido dibujado.

### Benchmark

Se usó la misma marea caótica de 384 enemigos y 600 capturas, una por frame, a 60 FPS.

Antes: [swarm-before-buckets-600f.gif](../artifacts/bucket-before-full/swarm-before-buckets-600f.gif)  
Después: [swarm-after-buckets-600f.gif](../artifacts/bucket-after-full/swarm-after-buckets-600f.gif)

PNG originales: [bucket-before-full](../artifacts/bucket-before-full) y [bucket-after-full](../artifacts/bucket-after-full).

### Resultado

| Métrica | Antes | Después |
|---|---:|---:|
| Entidades | 384 | 384 |
| Frames | 600 | 600 |
| B | ~192 | ~192 |
| S | ~108 | ~108 |
| FPS agregado | ~5 | ~5 |
| Regresión combate | PASS | PASS |

No hay una mejora perceptible en el stress completo. El cambio reduce recorrido auxiliar cuando la población ocupa una franja vertical pequeña, pero con 384 enemigos repartidos por ambas pantallas el rango de buckets sigue siendo amplio. El cuello de botella permanece en restauración y blit de sprites, no en el recorrido de buckets.

### Compromiso

Prácticamente nulo: la profundidad y la imagen se conservan. Se mantiene porque es una optimización segura y barata, pero no debe priorizarse frente a una optimización directa del blit/restauración.


## Optimización de blit: fast path para sprites interiores

### Cambio aplicado

En enemy_data.c se añadió un camino rápido para sprites que no están espejados y están completamente dentro de pantalla. Ese camino evita clipping y comprobaciones de límites por fila y píxel, pero conserva exactamente el test de transparencia y los píxeles escritos.

### Benchmark

Se ejecutaron 600 capturas reales, una por frame, a 60 FPS.

Antes: [swarm-before-blit-600f.gif](../artifacts/blit-before-full/swarm-before-blit-600f.gif)  
Después: [swarm-after-blit-600f.gif](../artifacts/blit-after-full/swarm-after-blit-600f.gif)

PNG originales: [blit-before-full](../artifacts/blit-before-full) y [blit-after-full](../artifacts/blit-after-full).

### Resultado

| Métrica | Antes | Después |
|---|---:|---:|
| Entidades | 384 | 384 |
| Frames | 600 | 600 |
| B | ~192 | ~192 |
| S | ~108 | ~107 |
| FPS agregado | ~5 | ~5 |
| Regresión combate | PASS | PASS |

La imagen resultante es equivalente y el cambio no introduce diferencias visibles. La mejora de ~1 tick en S no cambia el FPS. Esto descarta el clipping como cuello principal: el coste dominante está en escribir/restaurar grandes cantidades de píxeles, no en las condiciones de borde del blit.

### Compromiso

Prácticamente nulo y el fast path es seguro, pero el beneficio es demasiado pequeño para priorizarlo. El siguiente intento debería medir batching o reducción de restauraciones de fondo, no más microoptimización del blit.


## Experimento descartado: restauración por tiles sucios

Se probó sustituir los rectángulos ajustados de enemigos por una máscara de tiles
para restaurar cada tile una sola vez y fusionar solapes.

Resultado en 600 frames:

| Métrica | Rectángulos | Tiles |
|---|---:|---:|
| B | ~192 | ~192 |
| R | ~23 | ~40 |
| S | ~107 | ~107 |
| Imagen | correcta | correcta |

El área de cada tile de 16x16 era demasiado grande frente a los rectángulos
ajustados. Aunque reducía llamadas solapadas, escribía muchos más píxeles y
empeoraba la restauración. Se descartó y se revirtió.

Evidencia del intento: [tile-before-full](../artifacts/tile-before-full) y
[tile-after-full](../artifacts/tile-after-full).

El estado actual vuelve a rectángulos ajustados y fue recompilado y validado:
[tile-reverted-stress](../artifacts/tile-reverted-stress) y
[tile-reverted-combat](../artifacts/tile-reverted-combat), ambos PASS.


## Experimento descartado: cache de spans opacos de sprites

Se probó cachear por frame de sprite los límites opacos de cada fila y usar fast paths para filas totalmente opacas. Aunque el escenario y la regresión pasaron, las capturas mostraron pérdida visible de partes de sprites en la marea y el GIF posterior creció de ~487 KB a ~653 KB. Se revirtió.

Evidencia: [span-before-full](../artifacts/span-before-full) y [span-after-full](../artifacts/span-after-full).

Conclusión: el cache necesitaba una clave estable y una validación pixel-a-pixel más estricta. No se conserva ningún cambio de spans ni se cuenta como mejora.


## Experimento descartado: batching de restauración por filas

Se probó fusionar los dirty rectangles de enemigos en una franja horizontal por
cada fila de pantalla. La imagen y la regresión de combate se conservaron, pero
la marea compacta hace que esas franjas cubran demasiado ancho.

Resultado de 600 frames completos:

| Métrica | Antes | Batching por filas |
|---|---:|---:|
| B | ~192 | ~192 |
| R | ~23 | ~25 |
| S | ~107 | ~107 |
| FPS | ~6 | ~6 |

El cambio fue revertido. Evidencia visual:
[batch-before-batch-600f.gif](../artifacts/batch-before-full/swarm-before-batch-600f.gif) y
[batch-after-batch-600f.gif](../artifacts/batch-after-full/swarm-after-batch-600f.gif).
La causa es que fusionar llamadas reduce overhead, pero amplía el área de píxeles
restaurados; en este workload gana el coste de memoria y escritura.


## Experimento descartado: LOD de multitud

Se probó no dibujar 1 de cada 4 enemigos cuando había más de 320 activos,
manteniendo simulación, targeting y colisiones completos.

Resultado:

| Métrica | Completo | LOD 75% |
|---|---:|---:|
| B | ~192 | ~192 |
| S | ~107 | ~110 |
| FPS | ~6 | ~6 |
| Calidad visual | completa | huecos evidentes |

El GIF muestra una marea notablemente más rala sin mejora de FPS:
[LOD antes](../artifacts/batch-before-full/swarm-before-lod-600f.gif) y
[LOD después](../artifacts/lod-after-full/swarm-after-lod-600f.gif).

Se revirtió porque el compromiso visual es alto y el coste dominante no es el
número de llamadas de sprite aisladas, sino restauración/presentación del
framebuffer.

