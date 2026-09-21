# Walkthroughs por Sesión

Resúmenes acumulativos de cada sesión atómica de desarrollo de `towerds`.

## Convención

- Una carpeta por sesión: `walkthroughs/<nombre-sesion>/`.
- Dentro, el resumen `walkthrough.md` y sus assets en `walkthroughs/<nombre-sesion>/assets/`.
- Los assets se enlazan con rutas relativas al propio `.md` (por ejemplo `assets/foo.png`).
- Estos archivos **sí se versionan** (a diferencia de `artifacts/`, que está en `.gitignore`),
  así que al mergear la rama `feat/<feature>` a `main` los resúmenes se van acumulando aquí.
- Al renderizar en la UI de Antigravity se aplica además la regla 5 de `AGENTS.md`
  (copiar la media al directorio `brain` de la conversación con ruta absoluta POSIX).

El nombre de la carpeta debe coincidir con la rama de sesión `feat/<feature>`.

## Índice de sesiones

| # | Sesión | Carpeta | Rama | Estado |
| :---: | :--- | :--- | :--- | :--- |
| 0 | Eliminación del rango — fuego total en la pantalla inferior | [`00-no-range/`](00-no-range/walkthrough.md) | `feat/no-range` | Fusionada en `main` |
| 1 | Muerte de los enemigos: gore, licuado y trozos | [`splatter-impact-direction/`](splatter-impact-direction/walkthrough.md) | `feat/splatter-impact-direction` | Documento final: antes vs ahora, cada efecto (licuado + trozos, ambos permanentes), ablación y coste; cono direccional disponible tras `DEATH_CONE_ENABLED` |
