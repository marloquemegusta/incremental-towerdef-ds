# AGENTS.md

## Taxonomía documental (qué va dónde)

El repo tiene **cuatro fuentes de verdad** y **no se duplican**: cada una es canónica de su ámbito y las demás la referencian.

| Documento | Contiene | **No** contiene |
| :--- | :--- | :--- |
| `DESIGN.md` | Producto y diseño: visión, mundo, mecánicas, economía, balance, **lenguaje visual** (croma, assets maestros, escenario) y registro de decisiones (`[OQ-*]`). | Cómo está implementado, ni proceso de agente. |
| `TECHNICAL.md` | Invariantes de implementación: plataforma, punto fijo, pipeline de render, presupuestos de CPU y estructuras de datos. | Decisiones de producto, ni proceso de sesión. |
| `AGENTS.md` (este) | Proceso del agente: roles, sesión/ramas/worktrees, evidencias, permisos, entrega y upload. | Diseño ni técnica: sólo los **referencia**. |
| Skill `ds-game-dev` | Workflow genérico de Nintendo DS (toolchain, build, escenarios, evidencias, GDB, upload), válido para cualquier proyecto DS. | Nada específico de `towerds`. |

Además, `STATUS.md` es un **registro de estado por hitos** (no es fuente de verdad: se apoya en la evidencia de `artifacts/` y en los walkthroughs).

**Regla de conflicto:** si algo contradice a `DESIGN.md` o `TECHNICAL.md`, mandan ellos; aquí sólo se apunta. Si una regla de diseño o de técnica aparece escrita en este documento, es un error: se mueve a su fuente.

## Roles & Repositorio
- Proyecto: Tower Defense Incremental para Nintendo DS (`towerds`).
- Propietario del producto: Usuario.
- Agente principal: Antigravity (Google DeepMind).
- Entorno de compilación: Docker Desktop (`skylyrac/blocksds:slim-latest`).
- Entorno de emulación determinista: WSL2 + Python 3 + DeSmuME headless (`libdesmume.so`).

## Reparto de Modelos por Rol (Subagentes)

Los roles se implementan como subagentes de Command Code en `.commandcode/agents/` (ámbito **solo proyecto**: se cargan únicamente dentro de `towerds`, nunca en otros repos). El hilo principal delega en ellos con la herramienta `agent`; pueden correr varios en paralelo y cada uno tiene contexto aislado, su propio juego de herramientas y su modelo fijado. Los nombres `ds-*` no colisionan con los reservados (`explore`, `plan`, `review`, `general`).

| Rol (agente) | Cuándo se usa | Modelo | Frente a `deepseek/deepseek-v4.1-flash` para todo |
| :--- | :--- | :--- | :--- |
| `ds-explore` | Localizar dónde vive un sistema del motor ARM9 antes de editar. | `poolside/laguna-s-2.1-free` | **Mejor: gratis.** $0/$0 frente a $0.15/$0.6 por 1M. Único rol con ganancia clara y sin contrapartida. |
| `ds-log-triage` | Diagnosticar un fallo de build o de escenario (manifest, events, emulator.log). | `Qwen/Qwen3.7-Flash` | **Mejor en su caso de uso.** 5x más barato en input y ~4.6x en output, con 1M de contexto. Su trabajo es de un solo turno (leer un log grande y responder corto), donde manda el input, no la caché. |
| `ds-asset-audit` | Auditar capturas PNG y spritesheets contra el croma y el escenario de `DESIGN.md`. | `deepseek/deepseek-v4.1-flash` | **Igual.** Mismo precio y misma visión. Se deja explícito para fijar ámbito, herramientas y prompt. |
| `ds-scenario` | Compilar y ejecutar escenarios headless de DeSmuME; validar input táctil y de botones. | `deepseek/deepseek-v4.1-flash` | **Igual.** Revertido desde `MiniMaxAI/MiniMax-M3`, que costaba 20x en lectura de caché en un run largo. |
| `ds-coder` | Implementar C/ARM9 en `source/` e `include/`. | `deepseek/deepseek-v4.1-flash` | **Igual.** Revertido desde `zai-org/GLM-5.3` (87x en caché) por falta de evidencia de mejor calidad: los commits de este repo están hechos con v4.1-flash. |
| `ds-reviewer` | Revisar un diff contra los invariantes de `TECHNICAL.md`. | `meta/muse-spark-1.2-contributor` | **Distinto *y* más barato.** $0.1/$0.2 y caché $0.002 frente a $0.15/$0.6 y $0.003 del principal: decorrelación sin pagar peaje, porque revisar un diff es contexto acotado. Descartado `deepseek/deepseek-v4-pro` (4.6x el coste) para no pagar por inteligencia que la tarea no exige. |
| `ds-planner` | Diseñar el plan de una feature contra `DESIGN.md` y `TECHNICAL.md`. | `deepseek/deepseek-v4-pro` | **Distinto *y* mucho más barato.** `xai/grok-4.5` lee `DESIGN.md` + `TECHNICAL.md` enteros y los reenvía a caché $0.5/1M cada turno: ~36x el principal por plan. v4-pro es un modelo de razonamiento a 1/3 de ese precio. Sigue siendo la línea más cara, pero se invoca poco. |
| `ds-walkthrough` | Redactar `walkthroughs/<sesion>/walkthrough.md` y actualizar `STATUS.md`. | `deepseek/deepseek-v4.1-flash` | **Igual.** Revertido desde `z-ai/glm-5.3-flash`: mismo input, pero 10x la caché. |

**Criterio (no obvio):** en un subagente de muchos turnos la factura la domina la **lectura de caché**, no el input. `deepseek-v4.1-flash` tiene una de las cachés más baratas del catálogo ($0.003/1M), así que varios modelos baratos de escaparate (`z-ai/glm-5.3-flash` $0.03, `MiniMaxAI/MiniMax-M3` $0.06) salen más caros en cuanto el agente itera. **Regla: los agentes que más tokens consumen (explorador, triaje de logs, ejecutor de escenarios) van a lo más barato; los de coste acotado o uso esporádico (revisor, planificador) pueden permitirse otro modelo, y ahí el motivo es la decorrelación, no la supuesta inteligencia.** Y si la decorrelación sale más barata que el principal, como en `ds-reviewer`, no hay nada que sopesar: se cambia.

Para cambiar el modelo de un rol, se edita el campo `model:` de su fichero en `.commandcode/agents/`. Los cambios cargan en el turno siguiente, sin reiniciar.

### Tareas internas del bucle principal (ámbito usuario, fuera del repo)

Estas claves viven en `~/.commandcode/config.json`, **no** en este repo: aplican a todos los proyectos.

| Clave | Modelo | Motivo |
| :--- | :--- | :--- |
| `feature-model:vision` | `deepseek/deepseek-v4.1-flash` | **Corrección real.** El modelo principal no tiene visión y el flujo depende de capturas PNG de DeSmuME. |
| `feature-model:titleGeneration` | `Qwen/Qwen3.7-Flash` | 5x más barato que el principal; tarea trivial. |
| `feature-model:toolDescription` | `Qwen/Qwen3.7-Flash` | Idem. |
| `feature-model:tasteLearning` | `deepseek/deepseek-v4-flash` | Alto volumen, bajo riesgo. |
| `feature-model:tasteOnboarding` | `deepseek/deepseek-v4-flash` | Idem. |
| `feature-model:compaction` | `z-ai/glm-5.3-flash` | 1M de contexto. Candidato a bajar a `Qwen/Qwen3.7-Flash` (5x) si la calidad del resumen aguanta. |
| `feature-model:branchSummarization` | `z-ai/glm-5.3-flash` | Idem. |

### Versionado de `.commandcode/`

- `.commandcode/agents/` → **sí se versiona**; es la definición de los roles y es compartible.
- `.commandcode/settings.json` → **no**; contiene rutas absolutas de la máquina y permisos locales.
- `.commandcode/taste/` → aprendizaje personal derivado de las sesiones; a decidir.

## Flujo de Trabajo Autónomo
1. Cualquier cambio de gameplay, UI o lógica de simulación debe acompañarse de pruebas de build y validación visual mediante escenarios DeSmuME.
2. Los cambios visuales se contrastan con capturas en `artifacts/`.
3. No se asume éxito de ejecución sin evidencia determinista en logs y capturas.
4. Las decisiones de producto y diseño son guiadas por `DESIGN.md`.
5. **Prohibición Estricta de Mockups Sintéticos Offline / Obligatoriedad de Ejecución en Motor Real (DeSmuME):**
   - Queda TERMINANTEMENTE PROHIBIDO crear simulaciones de gameplay, animaciones de combate o mockups de pantalla mediante scripts offline externos (PIL, Canvas, Python u otras herramientas sintéticas ajenas al binario).
   - Toda simulación visual, prueba de tiles/escenario, animación y demostración de combate DEBE compilarse directamente en la ROM (`scripts/build-project.ps1`) y ejecutarse dentro del motor real de C en ARM9 mediante escenarios headless de DeSmuME (`scripts/run-scenario.ps1`).
   - Esto garantiza que la física, las partículas balísticas, los casquillos, el retroceso hidráulico, los splatters de sangre con dithering y la lógica determinista real del juego sean los que generen invariablemente las capturas y GIFs de evidencia.
   - Al incorporar o proponer nuevos tiles o sprites, se deben convertir a los arrays C correspondientes, compilar la ROM y capturar el resultado con DeSmuME.
6. **Invariantes de diseño y técnica (viven en su documento canónico; aquí sólo se referencian):**
   - **Filosofía City Defense** — stream continuo por dial del Atraedor, sin Game Over; Generador aditivo de 7 tiers (5 HP/tier) con degradación de automatizaciones por daño; andamio permeable en $Y_{\text{local}}=144$ (los enemigos pasan por debajo si no hay tiers erigidos) → `DESIGN.md` §1, §3 y `[OQ-01]`.
   - **Lenguaje visual y croma** — púrpura reservado al enjambre, **sangre en paleta roja**, assets maestros de sprites (`*_strip_master_1x.png`), sin conos de muerte en pantalla superior → `DESIGN.md` §7, §11 y `[OQ-06]`.
   - **Batería defensiva unificada** — 1 entidad lógica / 4 cúpulas visuales con fuego alternado, indicador común de munición sobre depósito central ($X=128, Y=153$) y recarga diegética por arrastre táctil → `DESIGN.md` `[OQ-06]`.
   - **Escenario y sin rango** — campo abierto de 256 px, suelo isométrico 2:1 (prohibido el cenital), fuego total en pantalla inferior sin franjas de demarcación → `DESIGN.md` §11 y `[OQ-06]`.
   - **Render y rendimiento** — punto fijo Q8, framebuffer con dirty grid, presupuesto de 545 ticks → `TECHNICAL.md` y la skill (`references/performance-architecture.md`).
   - Si algo de esto cambia, se cambia **en su documento**; aquí no se duplica.
7. **Revisión de coherencia documental al cerrar cada tarea (obligatoria):**
   - Al terminar **cualquier** tarea, revisar `DESIGN.md`, `TECHNICAL.md`, `STATUS.md` y este `AGENTS.md` y **corregir lo que el avance haya dejado contradicho**: reglas, invariantes, estructuras de datos, cifras, nombres de ficheros, mecánicas retiradas o añadidas, flags, etc.
   - Nada debe describir algo que el juego ya no hace (ni al revés). Si una mecánica se retira, su regla se retira o se marca como dormida en **su** documento; si un valor cambia (p. ej. `DEATH_CONE_ENABLED`, HP, precios), se actualiza donde vive.
   - Se hace **en el mismo cierre de la tarea**, no después, y se reporta al usuario qué se ha actualizado y qué no se ha tocado por no verse afectado.

## Protocolo de Sesiones Atómicas, Ramas y Worktrees
1. **Un Chat = Una Sesión Atómica (Feature-Scoped):** Cada nueva conversación con el asistente se dedica exclusivamente a una feature, fix o iteración concreta, evitando dispersión de contexto.
2. **Estrategia de Ramas:**
   - `full-incremental`: **Rama principal del repositorio y foco único de desarrollo activo**. Contiene la versión City Defense Incremental (stream continuo, generador de 7 automatizaciones, 1 torreta lógica / 4 visuales, economía de chatarra/vetas). Toda feature o fix se ramifica y se mergea aquí.
   - `feat/<nombre-feature>`: Ramas de trabajo atómicas creadas en worktree para features o fixes antes de mergear en `full-incremental`.
   - `hybrid-clicker-td`: Rama histórica congelada preservando el prototipo híbrido previo (batería balística, recarga diegética por stylus, gore con drag 3D, calibración y render 60 FPS fijos).
   - `classic-towerdefense`: Rama histórica congelada preservando la primera implementación del Tower Defense tradicional.
   - Tag `archive/main-pre-citydefense`: Preservación inmutable del estado del antiguo `main` previo a la transición. (La rama `main` ha sido eliminada para evitar ambigüedades).
3. **Uso Obligatorio de Git Worktrees para Aislamiento:**
   - En cada nueva sesión, se debe instanciar obligatoriamente un worktree físico separado **dentro del propio repo**, bajo `.worktrees/` (ignorado por git), de modo que la sesión nunca salga de la carpeta del proyecto:
     ```bash
     git worktree add .worktrees/<feature> -b feat/<feature>
     ```
   - Todo el trabajo de la sesión se desarrolla dentro de dicho worktree.
   - **Los ficheros ignorados no viajan al worktree:** al ser un checkout limpio, `runtime/bin/` (runtime headless de DeSmuME) e `images/*.tar` (imagen BlocksDS) no existen en el worktree aunque sí en el árbol principal. Antes de correr escenarios, **copiar el runtime desde el árbol principal** (`runtime/bin/`), no re-descargarlo con `scripts/setup.ps1`. La imagen Docker de BlocksDS sí es global del daemon y se comparte.
   - Al concluir, ser aprobado y fusionar a `full-incremental`, el worktree se limpia:
     ```bash
     git worktree remove .worktrees/<feature>
     ```
4. **Cierre y Entrega de Sesión:**
   - Compilación limpia con `scripts/build-project.ps1` (Docker BlocksDS). Todo prototipo o función implementada en `.c` debe declararse debidamente en su cabecera `.h` para evitar fallos de compilación `-Wimplicit-function-declaration`.
   - Validación visual determinista con `scripts/run-scenario.ps1 -RomPath <rom> -ScenarioPath <scenario> -OutputPath <dir>` (DeSmuME headless). Prohibido omitir argumentos mandatorios y usar escenarios con aserciones o hitboxes obsoletas.
   - Subida a la consola mediante `scripts/upload-rom.ps1 -ProjectPath . -RomPath game.nds -ConfirmUpload`:
     - El archivo remoto canónico en la microSD de la DS es invariablemente **`towerdefense.nds`** (configurado en `$RemoteName = 'towerdefense.nds'`). Queda terminantemente prohibido subir con `game.nds` u otro nombre no canónico.
   - **Revisión de coherencia de los `.md`** (`DESIGN.md`, `TECHNICAL.md`, `STATUS.md`, `AGENTS.md`) según la regla 7 del flujo de trabajo, antes de dar la tarea por cerrada.
   - Commit semántico en la rama de la feature.
   - Aprobación explícita del usuario antes de merge a `main` o tagging de versión (`v0.1`, `v0.2`, etc.).
5. **Regla Canónica de Incrustación de Media en Artefactos (`walkthrough.md`, reportes):**
   - Para que la UI de Antigravity renderice sin links rotos imágenes o animaciones GIF en los artefactos de `<appDataDir>\brain\<conversation-id>`, se debe cumplir estrictamente:
     1. Copiar los archivos a la raíz de `<appDataDir>\brain\<conversation-id>\`.
     2. Usar obligatoriamente la sintaxis de ruta absoluta POSIX normalizada: `![caption](/C:/Users/malfonso/.gemini/antigravity/brain/<conv_id>/<file>.gif)` o enlaces directos Markdown con `file:///C:/Users/...`. Queda prohibido usar nombres relativos planos (`showcase.gif`) sin la barra raíz del sistema.
6. **Regla Canónica de Captura y Demostración de Disparo en GIFs:**
   - Para generar animaciones GIF de torretas disparando, se debe configurar invariablemente cargador masivo o infinito (`ammo = 9999`), la mejora de auto-apuntado activa (`auto_target = 1`), cadencia rápida (`fire_interval = 3`) y objetivos de entrenamiento o enemigos pesados en rango dentro del campo de tiro.
   - Esto garantiza que la batería de torretas descargue un torrente ininterrumpido de proyectiles, retroceso hidráulico visible y lluvia continua de casquillos de artillería pesada en el búnker sin detenerse por recarga ni por eliminación prematura de los objetivos.
7. **Convención Canónica de Walkthroughs por Sesión (Resumen Acumulativo):**
   - El resumen de sesión (`walkthrough.md`) **nunca** se deja suelto en la raíz del repo. Se guarda en `walkthroughs/<nombre-sesion>/walkthrough.md`, dentro de una carpeta por sesión, con sus assets en `walkthroughs/<nombre-sesion>/assets/`. El nombre de la carpeta coincide con la rama `feat/<feature>`.
   - Estos archivos **sí se versionan** (a diferencia de `artifacts/`, que está en `.gitignore`); al mergear la rama `feat/<feature>` a `main` los resúmenes se acumulan como histórico de sesiones.
   - Las imágenes y anexos se referencian con **rutas relativas al propio `.md`** (p. ej. `assets/foo.png`): es lo que renderiza la UI para ficheros del repo. El formato absoluto POSIX de la regla 5 es **solo** para la media copiada a `<appDataDir>\brain\<conv_id>\`, no para los `.md` versionados.
   - `walkthroughs/README.md` es el índice acumulativo de sesiones; cada sesión nueva añade su fila.
   - La regla 5 (incrustación para la UI de Antigravity) sigue aplicando al renderizar allí: en ese caso la media se copia a `<appDataDir>\brain\<conv_id>\` con ruta absoluta POSIX.


<!-- lean-ctx -->
## lean-ctx

lean-ctx is active — the MCP tools replace native equivalents.
Full rules: LEAN-CTX.md (open on demand — do not auto-load).
<!-- /lean-ctx -->
