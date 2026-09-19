# Balance editable

Los valores de balance viven en `config/balance/` y están pensados para editarse desde Excel y guardarse como CSV UTF-8.

- `waves.csv`: composición, intervalo, velocidad, vida y recompensa base de las 20 oleadas.
- `enemies.csv`: vida y recompensa base constantes de cada tipo de enemigo.
- `upgrades.csv`: costes y nivel máximo de cada mejora.

Validación y generación del archivo que ya entiende la DS:

```powershell
python tools/compile_balance.py --output towerds_balance.bin
```

Servidor local para el futuro cliente Wi-Fi de desarrollo:

```powershell
python tools/serve_balance.py --directory artifacts --port 5000
```

La descarga por Wi-Fi requiere soporte `libnds/dswifi` enlazado al proyecto. El
toolchain actual de esta rama no expone esa biblioteca en sus headers, por lo
que no se debe simular ni declarar completada esa parte hasta añadirla al
toolchain de BlocksDS.

# Balance editable

Los valores de balance viven en `config/balance/` y están pensados para editarse desde Excel y guardarse como CSV UTF-8.

- `waves.csv`: composición, intervalo, velocidad, vida y recompensa base de las 20 oleadas.
- `enemies.csv`: vida y recompensa base constantes de cada tipo de enemigo.
- `upgrades.csv`: costes y nivel máximo de cada mejora.

Validación y generación del archivo que ya entiende la DS:

```powershell
python tools/compile_balance.py --output towerds_balance.bin
```

Servidor local para el futuro cliente Wi-Fi de desarrollo:

```powershell
python tools/serve_balance.py --directory artifacts --port 5000
```

La descarga por Wi-Fi requiere soporte `libnds/dswifi` enlazado al proyecto. El
toolchain actual de esta rama no expone esa biblioteca en sus headers, por lo
que no se debe simular ni declarar completada esa parte hasta añadirla al
toolchain de BlocksDS.

El binario generado se copia a la raíz de la tarjeta SD como `towerds_balance.bin`. Si falta o no supera la validación de magia, el juego conserva sus valores por defecto.

---

# Marco de Balance y Dimensionamiento Biomecánico

Este marco define el estándar matemático y ergonómico para dimensionar cualquier etapa, horda o árbol de mejoras en `towerds`.

---

## 1. El Axioma Biomecánico del Stylus (Nintendo DS)

1. **Techo Fisiológico:** En la pantalla táctil resistiva de la Nintendo DS, un jugador humano medio puede sostener una cadencia de puntería precisa de **2 a 3 pulsaciones por segundo ($2.0 - 3.0\text{ taps/s}$)**.
2. **Zona de Fatiga Extrema:** Puntear a $\ge 4.0\text{ taps/s}$ durante más de 10 segundos genera agotamiento muscular severo, pérdida de puntería e imprecisión de contacto. Puntear a $\ge 5.0\text{ taps/s}$ sostenidos es inviable.
3. **Regla Canónica de Diseño:**
   > **Ninguna oleada o pico de asedio debe exigir más de $2.5\text{ a }3.0\text{ taps/s}$ al jugador**, salvo que ya disponga de mejoras de automatización (*Gatillo Continuo / Hold*, *Auto-Target* o *Torretas adicionales*) o mejoras balísticas que reduzcan los impactos necesarios por baja.

---

## 2. Fórmulas de Dimensionamiento

### A. Presupuesto de Chatarra de una Etapa
Para una etapa con $M$ especies de enemigos y duración $T$:

$$\text{Scrap}_{\text{total}} = \sum_{i=1}^{M} \left( N_i \times V_i \right) \times \left(1 + \text{Nivel}_{\text{BioHarvest}}\right)$$

* $N_i$: Número total de enemigos de la especie $i$ spawneados.
* $V_i$: Valor de Chatarra base por baja de la especie $i$ (`enemy_scrap[i]`).
* **Presupuesto Pre-Pico ($\text{Scrap}_{\text{base}}$):** Chatarra obtenida en la fase tranquila inicial ($0:00 - 1:30$). Es el dinero real con el que el jugador cuenta para preparar sus defensas antes de la horda.

---

### B. Demanda de Daño por Segundo (DPS Requerido)
Para contener una oleada que avanza a la muralla sin que se acumulen enemigos:

$$\text{DPS}_{\text{req}} = \sum_{i=1}^{M} \left( \text{SpawnRate}_i \times \text{HP}_i \right)$$

* $\text{SpawnRate}_i = \frac{60}{\text{DelayFrames}_i}$ (enemigos por segundo).
* $\text{HP}_i$: Puntos de vida del enemigo.

---

### C. Tasa de Pulsaciones Manuales Requeridas (Taps/s)
Si el jugador dispara manualmente sin *Gatillo Continuo*:

$$\text{Balas}_{\text{enemigo}} = \left\lceil \frac{\text{HP}_i}{\text{Daño}_{\text{bala}}} \right\rceil$$

$$\text{Balas/s Requeridas} = \sum_{i=1}^{M} \left( \text{SpawnRate}_i \times \text{Balas}_{\text{enemigo}} \right)$$

Teniendo en cuenta el ciclo de disparo y recarga:
* $C$: Capacidad del cargador (`turret_magazine`).
* $T_{\text{recarga}}$: Tiempo de recarga manual o forzada ($\approx 1.2\text{ a }1.5\text{ s}$).
* $T_{\text{fuego}} = \frac{C}{\text{Balas/s Requeridas}}$: Tiempo que dura el cargador.
* **Tasa de Taps Efectiva durante la ventana de disparo:**

$$\text{Taps/s} = \frac{C}{T_{\text{fuego}}} \times \frac{T_{\text{fuego}} + T_{\text{recarga}}}{T_{\text{fuego}}} = \text{Balas/s Requeridas} \times \left( 1 + \frac{T_{\text{recarga}}}{T_{\text{fuego}}} \right)$$

---

## 3. Estudio de Caso Canónico: Etapa 1 (Fase 1)

### Parámetros Base
* **Bala inicial:** $1$ de daño.
* **Zergling inicial:** $3$ HP $\to$ Requiere **$3$ balas** (sin mejoras).
* **Recompensa Zergling:** **$1$ de Chatarra**.
* **Fase Base ($0:00 - 1:30 = 90\text{ s}$):** Delay $180\text{ f}$ ($3\text{ s}$) $\to$ **$30$ Zerglings** $\to$ **$30$ de Chatarra**.
* **Fase Pico ($1:30 - 2:00 = 30\text{ s}$):** Delay $40\text{ f}$ ($0.66\text{ s}$) $\to$ $1.5\text{ Z/s}$ $\to$ **$45$ Zerglings** $\to$ **$45$ de Chatarra**.
* **Total Chatarra Etapa 1:** $30 + 45 = \mathbf{75\text{ Chatarra}}$.

---

### Análisis Biomecánico del Pico ($1.5\text{ Z/s}$)

| Build / Mejora Adquirida | Daño Bala | Balas / Zerg | Balas/s Netas | Taps/s Requeridos (con recarga) | Estado Biomecánico |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Nivel 0 (Base)** | 1 | 3 | $4.5$ | $\mathbf{\approx 5.5 - 6.0}$ | **Inviable** (fatiga extrema, derrota garantizada). |
| **Calibre Lv1 ($15$ scrap)** | 2 | 2 | $3.0$ | $\mathbf{\approx 3.2 - 3.5}$ | **Difícil pero superable** con buena recarga. |
| **Calibre Lv1 + Cargador Lv1 ($25$ scrap)** | 2 | 2 | $3.0$ | $\mathbf{\approx 2.6 - 2.8}$ | **Óptimo** (dentro del rango cómodo). |
| **Calibre Lv2 ($15+35$ scrap)** | 3 | 1 | $1.5$ | $\mathbf{\approx 1.8 - 2.0}$ | **Puntería Zen** (1 toque = 1 baja instantánea). |
| **Gatillo Continuo ($20$ scrap)** | 1 | 3 | $4.5$ | $\mathbf{0}$ *(Hold)* | **Alivio Total** (fuego continuo manteniendo el stylus). |

---

## 4. Matriz de Costes Calibrada (Etapa 1)

Con un presupuesto pre-pico de **$30$ de Chatarra**, los costes de entrada permiten elegir libremente entre los tres arquetipos:

```
Mejora                   Nivel 1    Nivel 2    Nivel 3    Nivel 4    Efecto Principal
-----------------------------------------------------------------------------------------------------------
0: Calibre (Daño)          15         35         80        180       Daño 1 -> 2 -> 3 -> 5 -> 8
1: Cadencia (Intervalo)    15         30         65        140       Intervalo 12f -> 10f -> 8f -> 5f -> 3f
2: Cargador (Capacidad)    10         20         45         90       Capacidad 10 -> 16 -> 25 -> 40 -> 60
3: Bio-Cosecha (Scrap)     12         35          -          -       Multiplicador de chatarra x2, x3
4: Conveyor (Auto-ammo)    25         50        120        250       Recarga pasiva de munición
5: Auto-Fire               20         80          -          -       Lv1 = Gatillo Continuo (Hold), Lv2 = Auto-Target
6: Torretas Extra         150        400          -          -       Desbloqueo de Sockets laterales
7: Rango                   20         45         90        180       Línea de intercepción avanzada
```

---

## 5. Protocolo para Diseñar Nuevas Etapas

Al crear o rebalancear cualquier etapa futura:
1. **Calcular $\text{Scrap}_{\text{base}}$:** ¿Cuánta chatarra genera la etapa antes del pico?
2. **Definir la Demanda del Pico:** $\text{DPS}_{\text{pico}} = \sum (\text{SpawnRate} \times \text{HP})$.
3. **Test de Viabilidad Biomecánica:**
   - Calcular la tasa de taps con las mejoras asequibles con $\text{Scrap}_{\text{base}}$.
   - Si $\text{Taps/s} > 3.0$ y el jugador aún no puede costear *Hold* ni daño suficiente, **el pico está roto y debe reducirse la tasa de spawn o abaratarse la mejora clave**.
