# TowerDS - Filosofía y Especificación de Diseño Canónico

Documento canónico consolidado de visión de juego, arquitectura de simulación balística, balanceo incremental y calibración en hardware real para **TowerDS** (Nintendo DS).

---

## 1. Visión y Pilares del Proyecto
- **Plataforma Objetivo:** Nintendo DS (Hardware físico & DeSmuME headless determinista).
- **Resolución & Pantallas:** 
  - Campo de batalla continuo vertical de **256 × 384 px** repartido en dos pantallas (Superior: $Y \in [0..191]$, Inferior: $Y \in [192..383]$).
  - Tasa de refresco objetivo: **60 FPS estables** en ARM9 mediante renderizado directo a VRAM (`VRAM_A` y sub-VRAM).
- **Género:** Tower Defense Incremental Balístico Grimdark.
- **Ambientación:** Sector Forja del Adeptus Mechanicus bajo asedio de un Enjambre Bio-Xenos (estética industrial dieselpunk/grimdark).
- **Core Loop de la Partida:**
  1. **Fase de Preparación (`MODE_PREPARATION`):** El jugador revisa la situación, repara torretas, adquiere mejoras incrementales en el panel de investigación y puede abrir la pantalla de calibración táctica (`[CALIB]`).
  2. **Fase de Combate (`MODE_WAVE`):** Oleada activa a 60 FPS con acelerador de tiempo `[2X]` o avance normal.
     - Gestión táctil con stylus: arrastre manual de munición desde los silos a las torretas (hasta desbloquear la cinta transportadora automática) y marcado de objetivos prioritarios.
  3. **Fase de Asedio / Muerte (`MODE_GAME_OVER`):** Los enemigos que alcanzan el Sanctum del Búnker en la base ($Y \ge 344$) devoran su integridad. Si los 100 HP del Sanctum caen a 0, la run concluye.

---

## 2. Geometría Espacial del Campo de Batalla y Línea de Defensa

### A. Coordenadas Globales
- **Pantalla Superior ($Y = 0..191$):** Zona de incursión y descenso inicial del enjambre xenos.
- **Pantalla Inferior ($Y = 192..383$):** Zona táctica interactiva con stylus.
  - Baterías de torretas desplegadas en posiciones defensivas (por defecto Torreta #0 en $X=128, Y_{local}=124$, cota global $Y=316$).
  - **Sanctum del Búnker Central:** Emplazamiento sagrado centrado en $(128, 172)$ de la pantalla inferior (cota global $Y=364$, altura 42 px).
  - **Frontera de Colisión Infranqueable ($Y = 344$):** Cota exacta del labio superior del búnker. Cualquier enemigo que llegue a $Y \ge 344$ colisiona físicamente contra el muro del Sanctum, ancla su avance y ataca directamente a la base, impidiendo cualquier escape o caída fuera de pantalla.

### B. Descenso y Embudo Balístico
- Los enemigos aparecen en la parte superior ($Y = 0$) con dispersión horizontal aleatoria ($X \in [16..240]$).
- A medida que descienden y entran en la pantalla inferior ($Y > 80$), la simulación aplica una fuerza de convergencia hacia el centro ($X=128$), creando un embudo natural que los canaliza hacia las líneas de tiro de las defensas y el perímetro del búnker.

---

## 3. Dinámica de Balística y Combate

### A. Proyectiles y Velocidad de Bala
- **Velocidad de Bala Canónica:** **8 px/frame** (con vida de 35 frames).
- **Objetivo de Diseño:** A 8 px/frame, un proyectil disparado recorre el radio de combate (65-150 px) en apenas 3 a 8 frames, eliminando el fallo por esquiva o desfase angular cuando el jugador marca objetivos móviles con el stylus.
- **Micro-Retroceso y Muzzle Flash:** Animación visual alternada de cañones (doble cañón izquierdo/derecho) a 30 FPS con destello de disparo (`flash_timer`).

### B. Consumo y Logística de Munición
- Las torretas no disparan indefinidamente gratis: cada proyectil consume 1 unidad de munición de su cargador interno.
- **Recarga Táctil Inicial:** Arrastre manual de paquetes de munición con el stylus desde el panel logístico hacia la torreta.
- **Automatización Progresiva (Cinta Transportadora):** La mejora del conveyor alimenta automáticamente munición de forma pasiva (1/s, 3/s, 6/s) liberando atención mental del jugador hacia el targeteo o mejoras.

---

## 4. Estructura de Oleadas y Sistema de Calibración en Hardware

### A. Las 20 Oleadas de Asedio
El modo estándar se compone de **20 oleadas incrementales**. Cada oleada define de forma independiente la composición de las tres primeras castas xenos:

1. **Tier 1 (Larva Rastrera - Micro-Swarm):** Horda rápida de choque, bajo impacto por baja, volumen alto.
2. **Tier 2 (Ripper Devorador - Vanguardia):** Tamaño medio, mayor resistencia y valor de biomasa intermedio.
3. **Tier 3 (Hormagaunt - Asalto Biológico):** Unidad de ruptura, mayor vitalidad y cadencia de avance sostenida.

### B. Menú de Calibración Canónico (`[CALIB]`)
- **Acceso Exclusivo:** Disponible únicamente durante la fase de preparación (`MODE_PREPARATION`) pulsando el botón táctil **`[CALIB]`**. Se ha retirado cualquier atajo directo de botón físico (`KEY_Y` queda inactivo) para evitar activaciones accidentales durante el juego.
- **Navegación Unificada por Oleada:**
  - El menú presenta una única pantalla por oleada seleccionada ($1..20$), navegable con los gatillos `L` / `R` o mediante los botones táctiles en pantalla `[<]` y `[>]`.
  - Cada pantalla expone exactamente **9 parámetros editables** (3 parámetros $\times$ 3 Tiers):
    - `T1 LARVA COUNT`: Cantidad total de larvas en la oleada ($0..200$).
    - `T1 LARVA DELAY`: Intervalo entre spawns de larvas en frames ($5..300$ f).
    - `T1 LARVA SPEED`: Velocidad de avance ($10..150$ px/s).
    - `T2 RIPPER COUNT`: Cantidad de rippers en la oleada.
    - `T2 RIPPER DELAY`: Intervalo de rippers en frames.
    - `T2 RIPPER SPEED`: Velocidad de avance.
    - `T3 HORMAG COUNT`: Cantidad de hormagaunts en la oleada.
    - `T3 HORMAG DELAY`: Intervalo de hormagaunts en frames.
    - `T3 HORMAG SPEED`: Velocidad de avance.
- **Control Ergonómico & Autorrepetición:**
  - Selección de fila con Cruceta Arriba/Abajo o toque directo en la fila.
  - Modificación de valor mediante `[-]` y `[+]` táctiles o Cruceta Izquierda/Derecha.
  - **Autorrepetición Dinámica:** Mantener pulsada la cruceta izquierda o derecha incrementa/decrementa continuamente el valor, acelerando exponencialmente para saltos grandes (e.g., de 20 a 100).
- **Persistencia en Almacenamiento MicroSD (`libfat`):**
  - Cualquier ajuste se guarda automáticamente de forma binaria en `fat:/towerds_balance.bin`.
  - Si la consola se reinicia o se apaga, la configuración calibrada se restaura intacta al arrancar.
  - El botón `[DEFAULTS]` permite restaurar instantáneamente el perfil balanceado original de fábrica.
  - El botón `[RESTART W1]` reinicia la simulación en la Oleada 1 aplicando inmediatamente los parámetros modificados.

---

## 5. Escalado Incremental de la Forja (Árbol Intra-Run)

La chatarra obtenida de los enemigos caídos se invierte inmediatamente en tres ramas complementarias:

### Rama A: Balística y Letalidad
- **Calibre Aumentado (`caliber_lvl`):** Multiplica el daño plano por impacto de cada proyectil ($2 \to 3 \to 4 \to 6 \to 8$).
- **Cadencia de Fuego (`firerate_lvl`):** Reduce los frames de cooldown entre disparos ($18 \to 14 \to 10 \to 7 \to 5$).
- **Alcance Óptico (`range_lvl`):** Incrementa el radio de adquisición balística ($65 \to 80 \to 100 \to 125 \to 150$ px).
- **Capacidad de Cargador (`mag_size_lvl`):** Aumenta el almacenamiento interno de proyectiles de las torretas ($20 \to 35 \to 50 \to 80$).

### Rama B: Automatización y Logística (Factorio-Style)
- **Cinta Transportadora de Munición (`conveyor_lvl`):** Suministro automático pasivo directo al cargador ($0 \to 1/\text{s} \to 3/\text{s} \to 6/\text{s}$).
- **Computador de Tiro Automatizado (`auto_target`):** Adquisición automática del objetivo más próximo dentro del arco de tiro, liberando al jugador de pulsar cada objetivo individual con el stylus.
- **Emplazamientos Adicionales (`extra_turrets`):** Desbloqueo y despliegue de baterías secundarias en el frente.

### Rama C: Economía y Biocosecha
- **Cosechador de Biomasa (`bio_harvest_lvl`):** Multiplica la chatarra base obtenida por cada baja xenos ($1 \times, 2 \times, 3 \times, \dots$).
- **Blindaje del Sanctum (`bunker_armor_lvl`):** Expande los puntos de integridad y mitigación del búnker.

---

## 6. Reglas Canónicas de Arte y Presentación Visual

1. **Exclusividad Cromática Xenos:**
   - La gama púrpura/violeta/magenta (`RGB(115, 35, 155)` a `RGB(240, 150, 255)`) y el blanco hueso quedan **estrictamente reservados para el enjambre xenos**.
   - Prohibido utilizar matices púrpuras en el suelo metálico, muros o maquinaria del Mechanicus, garantizando contraste y legibilidad absoluta.
2. **Efectos de Impacto y Gore Balístico:**
   - La destrucción de biomasa genera salpicaduras persistentes (`Splatter`) y partículas de deflagración (`DeathParticle`) con gravedad y dispersión 3D simulada en punto fijo Q8.
3. **Assets Maestros de Sprites:**
   - El asset maestro canónico de cualquier entidad animada es siempre la tira completa (`*_strip_master_1x.png`). Queda prohibido redibujar o simplificar programáticamente los sprites fuera del pipeline canónico de assets.
