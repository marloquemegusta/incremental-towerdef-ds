# AGENTS.md

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
6. **Definición de Assets Maestros de Sprites:** El asset maestro canónico de cualquier entidad con animación (torretas, enemigos, etc.) es SIEMPRE la cinta completa de animación (`*_strip_master_1x.png`). Los archivos GIF son exclusivamente recursos ilustrativos de previsualización para el usuario. Las hojas de propuestas exploratorias iniciales (p. ej. hojas de 12 propuestas) se archivan únicamente como referencia histórica.
7. **Campo de Batalla Abierto y Muralla Defensiva:**
   - **Superficie Abierta (256 px):** El campo de batalla abarca el ancho completo de 256 px de la pantalla, sin carriles cerrados ni calzadas estrechas de 32 px. Los enemigos avanzan libremente hacia el sur en formación de enjambre sobre la superficie urbana completa.
   - **Muralla Defensiva:** La posición fortificada se sitúa en la cota canónica WALL_DEFAULT_Y (Y=144) de la pantalla inferior, sirviendo de anclaje para las torretas activas y la línea de defensa.
8. **Regla Canónica de Exclusividad Cromática Xenos:**
   - La gama púrpura/violeta/magenta (`RGB 115, 35, 155` a `RGB 240, 150, 255`) y el blanco hueso luminoso quedan **estrictamente reservados para el enjambre xenos**.
   - Queda estrictamente prohibido utilizar matices púrpuras en el escenario (suelo de metal, aceras, muros, conos o maquinaria del Mechanicus), garantizando un contraste visual inmediato y sin ambigüedades entre los enemigos y el entorno balístico.
9. **Regla Canónica de Suelo Isométrico 2:1 y Assets Maestros de Escenario (Sector 1):**
   - El suelo de combate de Sector 1 utiliza la proyección isométrica 2:1 (`dx=2, dy=1`) mediante tiles maestros de 32x32 px (`assets/tiles/sector1/master/tile_061_cobblestone_1x.png`, `tile_062_irregular_1x.png`, `tile_063_flagstone_1x.png`) generados por Scrabling.
   - En el motor de C (`source/tiles.c`, `source/renderer.c`), se renderizan en modo bitmap / modo 5 paletizado con solapamiento *back-to-front* (paso vertical $Y=8$ px y horizontal $X=16$ px alternado).
   - Queda estrictamente prohibido el uso de los antiguos tiles cenitales/ortogonales a 90° (archivados en `assets/tiles/sector1/archive/`).
10. **Regla Canónica de la Línea de Fuego / Rango Defensivo ($Y=64$):**
    - La línea de demarcación del rango balístico en $Y=64$ de la pantalla inferior es una franja discontinua de pintura vial amarilla trazada con textura orgánica de brocha y desgaste irregular directamente sobre el empedrado.
    - No debe incluir respaldos metálicos, pestañas grises ni bases sólidas rectangulares que rompan la continuidad del empedrado.

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

