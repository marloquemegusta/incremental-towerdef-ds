# DESIGN.md - Especificación de Diseño: Tower Defense Incremental DS

Consolidación técnica y de diseño para `towerds` en Nintendo DS. Para la visión filosófica profunda y balanceo, consultar [`DESIGN_PHILOSOPHY.md`](file:///c:/codexlocal/towerds/DESIGN_PHILOSOPHY.md).

---

## 1. Visión General, Género y Loop Clicker
- **Género:** Tower Defense Incremental Balístico Grimdark / Clicker de Asedio.
- **Plataforma:** Nintendo DS (Hardware real con stylus y emulación determinista DeSmuME headless).
- **Resolución & Topología:** Campo de batalla vertical unificado y continuo de **256 × 384 px** a 60 FPS estables.
  - Pantalla Superior ($Y \in [0..191]$): Aproximación y descenso del enjambre xenos a través de la trinchera balística, HUD superior de telemetría compacta.
  - Pantalla Inferior ($Y \in [192..383]$): Línea defensiva activa táctil, emplazamiento de torretas ($Y=316$), Sanctum del Búnker ($Y=364$), labio infranqueable ($Y=344$), silos de munición y barra de control táctil.
- **Condición de Derrota:** El Sanctum del Búnker cuenta con 100 HP base. Los enemigos que alcanzan el labio del búnker ($Y \ge 344$) se anclan y devoran su integridad. Si los HP llegan a 0, la run concluye.
- **Identidad Temática:** Factoría de munición y trinchera del Adeptus Mechanicus bajo asedio de un Enjambre Bio-Xenos (estética industrial dieselpunk/grimdark).

---

## 2. La Dinámica Clicker / Incremental: Fricción vs. Automatización
El núcleo de la experiencia de juego pivota sobre la **fricción deliberada** inicial y la satisfacción de **automatizar progresivamente** cada tarea repetitiva mediante la inversión de chatarra:

### A. Fricciones del Early Game (Control Manual con Stylus)
1. **Targeteo Manual / Disparo Asistido:**
   - En fases iniciales, las torretas carecen de cogitadores de tiro autónomos avanzados.
   - El jugador debe tocar directamente con el stylus sobre las amenazas prioritarias para asignar el fuego de las torretas o pulsar rítmicamente para concentrar las ráfagas balísticas.
2. **Logística Manual de Munición (Arrastre de Silos):**
   - Las torretas tienen cargadores limitados (20 proyectiles iniciales).
   - Cuando se agota el cargador, el jugador debe arrastrar manualmente con el stylus las cajas de munición desde el Depot central hacia las torretas para recargarlas.
3. **Selección Manual de Amenazas Críticas:**
   - La diversidad de amenazas (rápidos kamikazes aéreos vs. tanques pesados terrestres) exige priorización manual bajo presión temporal.

### B. El Alivio Incremental (Árbol de Automatización Factorio-Style)
Conforme se cosecha chatarra de los enemigos abatidos, el jugador adquiere mejoras en la Forja que eliminan progresivamente las fricciones:
1. **Cintas Transportadoras de Munición (`conveyor_lvl`):**
   - Automatiza la recarga continua de munición hacia los cargadores de las torretas ($1/\text{s} \to 3/\text{s} \to 6/\text{s}$), eliminando la necesidad de arrastrar munición manualmente.
2. **Cogitador de Tiro Autónomo (`auto_target`):**
   - La torreta adquiere automáticamente el objetivo más cercano o peligroso dentro de su cono de tiro, liberando al jugador para supervisar la economía y las mejoras.
3. **Escalado de Calibre y Cadencia:**
   - Convierte el combate de una lucha táctica desesperada punto a punto en una picadora balística industrial capaz de triturar oleadas masivas a 60 FPS.

---

## 3. Las Dos Pantallas: Campo Continuo y HUD

### A. Pantalla Superior ($Y = 0..191$)
- **Geometría de Trinchera:** Asfalto balístico de 32 px con bordillos en coordenadas 0 y 31, sombra arrojada 3D profunda (3-4 px), escalinatas (`curb_stairs`), sumideros (`curb_drain`) y colectores pluviales (`curb_pipe`).
- **Descenso del Enjambre:** Aparición aleatoria en $Y=0$ ($X \in [16..240]$) con embudo hacia el centro ($X=128$) conforme entran a la pantalla inferior.
- **HUD Superior Compacto:**
  - `WAVE X/20`: Oleada actual sobre el total del asedio.
  - `TIME: Xs`: Tiempo restante de la oleada.
  - `SCRAP: [valor]`: Chatarra acumulada para mejoras.

### B. Pantalla Inferior ($Y = 192..383$)
- **Línea de Torretas ($Y_{local}=124$, Global $Y=316$):** Torreta principal Heavy Bolter de doble cañón con animación de retroceso alternado a 30 FPS y destellos de fuego (`flash_timer`).
- **Labio de Colisión Infranqueable ($Y = 344$):** Cota donde los enemigos terrestres detienen su avance, fijan posición e inician el asalto de mordisco contra el Sanctum.
- **Sanctum del Búnker ($Y_{local}=172$, Global $Y=364$):** Corazón de la defensa con barra de vida de 100 HP.
- **Barra de Control Inferior:**
  - Botón de aceleración `[2X]` (o botón físico `R`) para acelerar la simulación balística.
  - Botón `[CALIB]` accesible en fase de preparación para afinar el balance en vivo en hardware.
  - Paneles de investigación de la Forja para aplicar mejoras incrementales al instante.

---

## 4. Plantel Canónico de Amenazas Xenos (8 Especies StarCraft)
Superando el concepto primitivo de tiers lineales, cada especie cumple un **rol de amenaza táctica diferenciado**:

| ID | Especie | Arquetipo / Amenaza | Tamaño | Comportamiento en Simulación | Stats Base (HP / Scrap) |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **0** | **Scourge** | Volador Kamikaze veloz | $31 \times 27$ px | Hostigador ultrarrápido con sombra dinámica, gran velocidad | 18 HP \| 4 Chatarra |
| **1** | **Zergling** | Vanguardia / Enjambre ágil | $40 \times 39$ px | Corredor rápido en masa, animación de ataque doble con garras | 25 HP \| 5 Chatarra |
| **2** | **Hydralisk** | Asalto medio a distancia | $42 \times 55$ px | Infantería pesada erecta, andanada de bio-espinas | 75 HP \| 15 Chatarra |
| **3** | **Mutalisk** | Cazador alado de flanco | $64 \times 72$ px | Planeador ágil a cota alta, sombra dinámica sobre el terreno | 160 HP \| 35 Chatarra |
| **4** | **Defiler** | Caster biológico / Debilitador | $69 \times 59$ px | Reptante sinuoso de gran resistencia y dispersión de biomasa | 320 HP \| 70 Chatarra |
| **5** | **Lurker** | Ariete acorazado con espinas | $69 \times 64$ px | Cuadrúpedo blindado rompe-líneas de alta absorción de daño | 500 HP \| 120 Chatarra |
| **6** | **Guardian** | Bombardero pesado de asedio | $78 \times 70$ px | Silueta colosal de manta aérea, sombra profunda proyectada | 1.100 HP \| 250 Chatarra |
| **7** | **Ultralisk** | Titán coloso / Boss | $98 \times 105$ px | Apisonadora colosal con hojas Kaiser oscilantes y mordisco demoledor | 2.600 HP \| 600 Chatarra |

### Paleta Cromática Canónica del Enjambre (Regla de Exclusividad)
- **Blanco Hueso / Exoesqueleto:** `RGB(250, 245, 235)` (brillo primario) y `RGB(215, 205, 190)` (tono medio).
- **Quitina Violeta Radiante (Silueta & Patas):** `RGB(115, 35, 155)` (base sólida) y `RGB(155, 55, 195)` (medio).
- **Bioluminiscencia & Extremidades Menores:** `RGB(205, 85, 240)` y resalte `RGB(240, 150, 255)`.
- **Órganos Sensoriales & Toxinas:** Ojos en Rojo Sangre `RGB(255, 40, 30)` y bio-vapores Verde Neón `RGB(65, 255, 50)`.
- *Regla de exclusividad:* Queda terminantemente prohibido utilizar matices púrpuras o magentas en los tiles de suelo, paredes, aceras o estructuras del Adeptus Mechanicus, asegurando contraste figura-fondo instantáneo.

---

## 5. Estructura de Oleadas y Calibración en Hardware (`[CALIB]`)
- **20 Oleadas de Asedio:** Progresión de dificultad incremental afinada a 60 FPS.
- **Consola de Calibración Táctica (4 Páginas, 32 Filas):**
  - **Página 0 (Wave Spawns):** 32 filas por oleada para calibrar el spawn rate, HP multiplier, speed y delay de cada una de las 8 especies y la recompensa de chatarra.
  - **Página 1 (Enemy Stats):** 32 filas con los 4 atributos base (HP, Chatarra, Daño, Intervalo de ataque) para las 8 especies.
  - **Página 2 (Base & Turrets):** Calibración de parámetros de la base, búnker y torretas.
  - **Página 3 (Forge Upgrades):** Curvas de costes y multiplicadores de las mejoras de la Forja.
- **Persistencia MicroSD:** Guardado y carga binaria inmediata en `fat:/towerds_balance.bin`. Botón `[DEFAULTS]` para restaurar valores originales y `[RESTART W1]` para reiniciar run con los valores activos.

---

## 6. Escalado Incremental de la Forja (Árbol Intra-Run)
- **Rama Balística (Letalidad):** Calibre aumentado (daño plano por bala), cadencia de fuego (reducción de cooldown), alcance óptico y capacidad de cargador.
- **Rama Logística y Automatización:** Cintas transportadoras pasivas (`conveyor`), adquisición autónoma de blancos (`auto_target`) y baterías adicionales.
- **Rama Economía y Biocosecha:** Multiplicadores de chatarra cosechada (`bio_harvest`) y blindaje de aleación del Sanctum (`bunker_armor`).
