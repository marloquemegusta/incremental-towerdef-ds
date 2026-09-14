# Sprites de Enemigos Tiránidos (Assets Canónicos)

Esta carpeta contiene los sprites maestros oficiales para las 6 biocastas de enjambre de `towerds`.

## Convención Canónica (AGENTS.md Regla 6)
- **Asset Maestro:** Cada entidad animada tiene como asset canónico su **cinta completa de animación 1x** (`*_strip_master_1x.png`).
- **Previsualización HD:** La versión 4x (`*_strip_master_4x.png`) sirve para inspección visual directa.
- **Previsualización Animada:** El archivo GIF (`*_preview.gif`) es exclusivamente ilustrativo para verificar el ciclo de caminata.
- **Archivo Histórico:** Todas las propuestas previas y hojas de exploración se conservan en `archive/`.

## Catálogo de Especies Canónicas (8 Unidades)

| ID | Especie | Rol / Amenaza | Tamaño Frame | Frames Anim | Altitud Vuelo | Asset Maestro (1x) |
| :---: | :--- | :--- | :---: | :---: | :---: | :--- |
| **0** | **Scourge** | Volador Kamikaze ultrarrápido | $31 \times 27$ px | 5 (Vuelo) | 10 px | `t3_scourge_fly_strip_master_1x.png` |
| **1** | **Zergling** | Vanguardia / Enjambre ágil | $40 \times 39$ px | 7 (Paso) + 5 (Ataque) | 0 px | `t1_zergling_walk_strip_master_1x.png` |
| **2** | **Hydralisk** | Asalto medio a distancia | $42 \times 55$ px | 7 (Paso) + 5 (Ataque) | 0 px | `t2_hydralisk_walk_strip_master_1x.png` |
| **3** | **Mutalisk** | Cazador alado de flanco | $64 \times 72$ px | 5 (Vuelo) | 14 px | `sc_mutalisk_fly_strip_master_1x.png` |
| **4** | **Defiler** | Caster biológico / Debilitador | $69 \times 59$ px | 8 (Paso) | 0 px | `sc_defiler_walk_strip_master_1x.png` |
| **5** | **Lurker** | Ariete acorazado con espinas | $69 \times 64$ px | 7 (Paso) | 0 px | `sc_lurker_walk_strip_master_1x.png` |
| **6** | **Guardian** | Bombardero pesado de asedio | $78 \times 70$ px | 7 (Vuelo) | 16 px | `sc_guardian_fly_strip_master_1x.png` |
| **7** | **Ultralisk** | Titán coloso / Boss demoledor | $98 \times 105$ px | 9 (Paso) + 6 (Ataque) | 0 px | `t4_ultralisk_walk_strip_master_1x.png` |

> [!NOTE]
> Para optimizar la memoria EWRAM de Nintendo DS (límite 4 MB), se almacenan únicamente **5 direcciones** (N, NE, E, SE, S) y las direcciones restantes (SW, W, NW) se espejan horizontalmente en tiempo de ejecución (`flip_h`), reduciendo el consumo de sprites a **1.85 MB**. Las hojas maestras de 12 propuestas y los assets de prueba de larvas se archivan en `archive/`.

