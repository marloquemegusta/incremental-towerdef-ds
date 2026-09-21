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
   - **Lenguaje visual y croma** — púrpura reservado al enjambre, **sangre en paleta roja**, assets maestros de sprites (`*_strip_master_1x.png`) → `DESIGN.md` §7 y §11.
   - **Escenario** — campo abierto de 256 px, muralla defensiva en `WALL_DEFAULT_Y`, suelo isométrico 2:1 (prohibido el cenital) → `DESIGN.md` §11 (y §2/§3).
   - **Sin rango** — fuego total en la pantalla inferior, sin franjas de demarcación → `DESIGN.md` `[OQ-06]`.
   - **Render y rendimiento** — punto fijo, framebuffer con dirty grid, presupuesto de 545 ticks → `TECHNICAL.md` y la skill (`references/performance-architecture.md`).
   - Si algo de esto cambia, se cambia **en su documento**; aquí no se duplica.

## Protocolo de Sesiones Atómicas, Ramas y Worktrees
1. **Un Chat = Una Sesión Atómica (Feature-Scoped):** Cada nueva conversación con el asistente se dedica exclusivamente a una feature, fix o iteración concreta, evitando dispersión de contexto.
2. **Estrategia de Ramas:**
   - `main`: Rama de producción/estable. Solo contiene código probado en hardware/emulación y aprobado por el usuario.
   - `feat/<nombre-feature>`: Rama de trabajo creada al inicio de la sesión.
3. **Uso Obligatorio de Git Worktrees para Aislamiento:**
   - En cada nueva sesión, se debe instanciar obligatoriamente un worktree físico separado fuera del árbol principal para mantener el entorno aislado:
     ```bash
     git worktree add ../towerds-<feature> -b feat/<feature>
     ```
   - Todo el trabajo de la sesión se desarrolla dentro de dicho worktree.
   - Al concluir, ser aprobado y fusionar a `main`, el worktree se limpia:
     ```bash
     git worktree remove ../towerds-<feature>
     ```
4. **Cierre y Entrega de Sesión:**
   - Compilación limpia con `scripts/build-project.ps1` (Docker BlocksDS). Todo prototipo o función implementada en `.c` debe declararse debidamente en su cabecera `.h` para evitar fallos de compilación `-Wimplicit-function-declaration`.
   - Validación visual determinista con `scripts/run-scenario.ps1 -RomPath <rom> -ScenarioPath <scenario> -OutputPath <dir>` (DeSmuME headless). Prohibido omitir argumentos mandatorios y usar escenarios con aserciones o hitboxes obsoletas.
   - Subida a la consola mediante `scripts/upload-rom.ps1 -ProjectPath . -RomPath game.nds -ConfirmUpload`:
     - El archivo remoto canónico en la microSD de la DS es invariablemente **`towerdefense.nds`** (configurado en `$RemoteName = 'towerdefense.nds'`). Queda terminantemente prohibido subir con `game.nds` u otro nombre no canónico.
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
