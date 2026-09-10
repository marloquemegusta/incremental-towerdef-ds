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
5. **Uso Estricto de Assets Reales en Mockups y Composiciones:** Al generar mockups, pantallas o composiciones visuales, se deben usar SIEMPRE los assets canónicos reales de sprites y tiles almacenados en `assets/` (abriendo y componiendo directamente los archivos de imagen existentes). Queda ESTRICTAMENTE PROHIBIDO redibujar o simplificar programáticamente por código las torretas, enemigos o elementos de escenario.
6. **Definición de Assets Maestros de Sprites:** El asset maestro canónico de cualquier entidad con animación (torretas, enemigos, etc.) es SIEMPRE la cinta completa de animación (`*_strip_master_1x.png`). Los archivos GIF son exclusivamente recursos ilustrativos de previsualización para el usuario. Las hojas de propuestas exploratorias iniciales (p. ej. hojas de 12 propuestas) se archivan únicamente como referencia histórica.
7. **Regla Canónica de Geometría de Carreteras y Desnivel 3D:**
   - **Sistema Modular 2x2 (32 px):** Las calzadas se construyen en franjas estándar de 2 tiles de ancho (32 px totales, con ~25 px de asfalto útil).
   - **Alineación Estricta a Frontera de Retícula (Coordenadas 0 y 31):** La línea exterior del bordillo se fija invariablemente en la frontera del bloque modular (coordenadas 0 y 31). De este modo, los tiles de suelo contiguos y su retícula de baldosas permanecen 100% íntegros y nunca son intersectados, recortados ni invadidos por franjas arbitrarias de hormigón. En giros de 90°, los codos interiores coinciden con los vértices exactos de la rejilla (0, 31), (31, 0), etc., y los arcos exteriores curvan con radio exacto \(R=31\text{ px}\).
   - **Tridimensionalidad de Trinchera Hundida:** La calzada es una trinchera balística hundida a cota inferior respecto a la acera/plaza. El desnivel se representa obligatoriamente con sombra arrojada profunda (3-4 px) bajo el labio del bordillo, y se acentúa mediante escalinatas de piedra de 3 peldaños (`curb_stairs`), sumideros con rejillas de fundición (`curb_drain`), colectores pluviales (`curb_pipe`) y parapetos de sacos terreros en las cornisas elevadas.

