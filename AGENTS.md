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
