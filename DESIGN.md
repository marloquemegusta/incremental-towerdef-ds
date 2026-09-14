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

## 4. Plantel Canónico de Amenazas Xenos (8 Especies StarCraft)
Transición del esquema rígido de "Tiers" lineales a un **sistema de roles y amenazas tácticas** que dinamiza la incrementalidad (velocidad, blindaje, vuelo, sombra y asalto pesado):

| ID | Especie | Arquetipo / Amenaza | Tamaño | Comportamiento en Simulación | Stats Base (HP / Scrap) |
| :---: | :--- | :--- | :---: | :--- | :--- |
| **0** | **Scourge** | Volador Kamikaze veloz | $31 \times 27$ px | Hostigador ultrarrápido con sombra dinámica, gran velocidad | 18 HP \| 4 Chatarra |
| **1** | **Zergling** | Vanguardia / Enjambre ágil | $40 \times 39$ px | Corredor rápido en masa, animación de ataque doble con garras | 25 HP \| 5 Chatarra |
| **2** | **Hydralisk** | Asalto medio a distancia | $42 \times 55$ px | Infantería pesada erecta, andanada de bio-espinas | 75 HP \| 15 Chatarra |
| **3** | **Mutalisk** | Cazador alado de flanco | $64 \times 72$ px | Planeador ágil a cota alta, sombra dinámica sobre el terreno | 160 HP \| 35 Chatarra |
| **4** | **Defiler** | Caster biológico / Debilitador | $69 \times 59$ px | Reptante sinuoso de gran resistencia y dispersión de biomasa | 320 HP \| 70 Chatarra |
| **5** | **Lurker** | Ariete acorazado con espinas | $69 \times 64$ px | Cuadrúpedo blindado rompe-líneas de alta absorción de daño | 500 HP \| 120 Chatarra |
| **6** | **Guardian** | Bombardero pesado de asedio | $78 \times 70$ px | Silueta colosal de manta aérea, sombra profunda proyectada | 1.100 HP \| 250 Chatarra |
| **7** | **Ultralisk** | Titán coloso / Boss | $98 \times 105$ px | Apisonadora colosal con hojas Kaiser oscilantes y mordisco demoledor | 2.600 HP \| 600 Chatarra |

### Paleta Cromática Canónica del Enjambre (Regla de Exclusividad)
Para garantizar legibilidad visual inmediata contra el suelo metálico de las trincheras (`RGB 18, 18, 24`), la gama violeta/púrpura queda **estrictamente reservada para las entidades del enjambre**:
- **Blanco Hueso / Exoesqueleto:** `RGB(250, 245, 235)` (brillo primario) y `RGB(215, 205, 190)` (tono medio).
- **Quitina Violeta Radiante (Silueta & Patas):** `RGB(115, 35, 155)` (base sólida) y `RGB(155, 55, 195)` (medio).
- **Bioluminiscencia & Extremidades Menores:** `RGB(205, 85, 240)` y resalte `RGB(240, 150, 255)`.
- **Órganos Sensoriales & Toxinas:** Ojos en Rojo Sangre `RGB(255, 40, 30)` y bio-vapores Verde Neón `RGB(65, 255, 50)`.
*Regla de exclusividad:* Queda terminantemente prohibido utilizar matices púrpuras o magentas en los tiles de suelo, paredes, aceras o estructuras del Adeptus Mechanicus, asegurando un contraste figura-fondo infinito en cualquier pantalla de Nintendo DS.

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
