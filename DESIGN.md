# DESIGN.md - Especificación de Diseño: Tower Defense Incremental DS

Consolidación técnica y de diseño para `towerds` en Nintendo DS. Para la visión en profundidad, consultar [`DESIGN_PHILOSOPHY.md`](file:///c:/codexlocal/towerds/DESIGN_PHILOSOPHY.md).

---

## 1. Visión General y Género
- **Género:** Tower Defense Incremental Roguelite.
- **Estructura de la Run:** 3 Sectores $\times$ 3 Niveles = **9 Mapas Procedimentales**.
- **Condición de Derrota:** La ciudad (Sanctum) tiene 20 puntos de integridad acumulados a lo largo de los 9 mapas. Si llega a 0, la run concluye.
- **Identidad Gráfica:** 16-bit pixel art grimdark con temática Adeptus Mechanicus vs Enjambre Xenos (Tiránidos).

---

## 2. Las Dos Pantallas
- **Pantalla Superior (Sub-Engine):**
  - Matriz táctica y telemetría de baterías en tiempo real.
  - Banda superior compacta: Integridad de la ciudad (HP %), Oleada (Progreso %) y Diezmo/Chatarra.
  - Bloque central: Baterías activas agrupadas por tipo (número de emplazamientos, DPS total, barra de % de aporte al daño global de la defensa, bajas).
  - Bloque inferior: Desglose de tipos de amenazas xenos vivas y resumen de doctrinas activas de la forja.
- **Pantalla Inferior (Main-Engine Táctil 256x192 px):**
  - Motor de tilesets de 16x16 px: Suelo de fundición remachado y trinchera balística continua de 32 px de ancho con barandillas de peligro perimetrales.
  - Baterías de torretas con cono de tiro, cañones masivos con retroceso explosivo no lineal y chispas mecánicas de tungsteno.
  - Enjambre xenos en movimiento con micro-sprites direccionales (5x5 px) a 60 FPS estables.
  - Interfaz táctil ergonómica con stylus: colocación, rotación de arco de tiro, desmantelamiento al 100% y acelerador de tiempo `[2X]` (o botón `R`).

---

## 3. Mecánica de Combate y Armadura Plana
$$\text{Daño Recibido} = \max(1, \text{Daño Bala} - \text{Armadura Enemigo})$$

### Familias de Armas Iniciales:
1. **Twin Heavy Bolter (Fuego Rápido Antienjambre):**
   - 2 balas por ráfaga (3 dmg c/u = 6 dmg total).
   - Cadencia: 4 disparos/segundo (8 balas/s).
   - $0$ AP. Excepcional para limpiar hordas sin armadura; ineficiente contra blindajes pesados.
2. **Lascannon (Perforador Anticarro):**
   - Rayo continuo de rieles (35 dmg plano).
   - Cadencia: 1 disparo cada 2.5s.
   - 100% AP (ignora armadura plana) y atraviesa hasta 3 objetivos en línea recta.
3. **Heavy Flamer (Saturación de Zona):**
   - Cono continuo de fuego (12 DPS en área).
   - Ignora 1 punto de armadura por calor y ralentiza un 25% a la horda.
4. **Missile Pod (Artillería AOE):**
   - Salvas de micromisiles balísticos (15 dmg en radio de 16 px).

---

## 4. Jerarquía Oficial de Amenazas Xenos (6 Tiers)
Selección oficial de enemigos para el motor de juego (con todas las 18 variantes preservadas en `assets/sprites/enemies/`):

| Tier | Enemigo Oficial | Tamaño | Rol & Comportamiento | Stats Base |
| :--- | :--- | :---: | :--- | :--- |
| **Tier 0** | **0A. Micro-Larva Rastrera** | $2 \times 2$ px | Horda masiva milimétrica (100+ en pantalla). Se estira y encoge. | 1 HP \| 0 Arm \| 1 Chatarra |
| **Tier 1** | **1A. Ripper Devorador** | $6 \times 4$ px | Parásito carnívoro con ondulación continua en S y mordisco voraz. | 8 HP \| 0 Arm \| 3 Chatarra |
| **Tier 2** | **2A. Gárgola Bio-Scout** | $9 \times 9$ px | Volador ágil con aleteo rítmico membranoso y bio-aguijón venenoso. | 35 HP \| 0 Arm \| 12 Chatarra |
| **Tier 3** | **3A. Ravener Serpiente** | $14 \times 10$ px | Excavador acorazado con onda espinal sinusoidal y 4 guadañas de hueso. | 160 HP \| 2 Arm \| 60 Chatarra |
| **Tier 4** | **4B. Haruspex Fauces Vivas** | $20 \times 20$ px | Bestia de asedio pesada con boca circular dentada y tentáculos prensiles. | 2.8k HP \| 5 Arm \| 850 Chatarra |
| **Tier 5** | **5A. Bio-Titán Hierofante** | $28 \times 28$ px | Coloso arácnido de 4 zancas titánicas, chimeneas de bio-humo y cañones. | 40k HP \| 8 Arm \| 20.000 Chatarra |

---

## 5. Progresión Incremental (De 20 a 20.000.000)
- **Densidad de Biomasa (40-120 sprites en pantalla):** En lugar de dibujar miles de sprites que saturen la pantalla y el ARM9, los enemigos evolucionan en masa de biomasa, vida y valor de chatarra (Sector 1: 2 HP / 1 chatarra; Sector 3: 50.000 HP / 25.000 chatarra).
- **Multiplicadores Compuestos Intra-Run:**
  $$\text{Ganancia Chatarra} = (\text{Base}) \times (\text{Reciclaje Taller}) \times (\text{Combo Racha}) \times (\text{Interés Diezmo})$$
- **Árbol de Habilidades Intra-Run:** Las mejoras de chatarra se conservan a lo largo de los 9 niveles de la run, obligando a elegir entre ramas de especialización excluyentes (*Rama Balística Rápida* vs *Rama Artillería Pesada* vs *Rama Logística/Diezmo*).

---

## 5. Metaprogresión Permanente (Riesgo / Recompensa)
- Al concluir una run, la puntuación se convierte en **Datos STC**.
- Los Datos STC se canjean en la Forja Central por:
  1. Nuevas familias de torretas añadidas al pool de la run.
  2. **Directivas de Asedio (Modificadores activables tipo Heat/Hades):** Aumentan la dificultad (velocidad enemiga, armadura extra, etc.) a cambio de multiplicadores gigantescos de chatarra y puntuación, manteniendo el Sector 1 siempre desafiante e interesante.
