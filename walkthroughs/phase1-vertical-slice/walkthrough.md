# Walkthrough: Fase 1 City Defense Incremental — Generador Aditivo y Fin de Demo

## 1. Resumen Ejecutivo

En esta sesión se cierra el ciclo de la **Fase 1** (~primeros minutos / vertical slice contenido) del nuevo diseño City Defense Incremental:
- **Generador Aditivo desde Cero**: La partida arranca con **0 tiers construidos** (`built_tiers = 0`). Las bahías del generador muestran un armazón industrial de andamio oscuro. Al comprar mejoras de automatización se erigen los módulos correspondientes con 5 HP cada uno.
- **Árbol de Mejoras & Economía de Fase 1**:
  - Mejoras numéricas de batería (Calibre, Cadencia, Cargador) con 5 niveles y escalado de costes $\times 2$ ($15, 30, 60, 120, 240$).
  - **A1: Disparo Drag / Ráfaga Continua** (Coste: 100 chatarra): Construye el **Tier 1** del Generador (5 HP).
  - **A2: Auto-apuntado Completo** (Coste: 350 chatarra, requiere A1): Construye el **Tier 2** del Generador (5 HP) y dispara la pantalla de **Fin de la Demo**.
- **Pantalla de Fin de Demo (Hito Fase 1)**: Muestra "FIN DE LA DEMO - FASE 1", "AUTO-APUNTADO DESBLOQUEADO", estadísticas de combate y permite continuar en modo sandbox infinito con `A` o reiniciar con `B`.
- **Menú de Calibración Recableado**: Sustitución de las viejas etapas de oleadas por la página canónica **ATRAEDOR & GENERADOR** (diales de cadencia de spawn, tier biocasta, tiers erectos, salud de tiers 1 y 2, chatarra, bunker) y la página de costes para los 14 nodos de la economía incremental.

---

## 2. Evidencia Visual Determinista (DeSmuME)

### 2.1 Arranque con Andamio (0 Tiers Construidos)
El generador comienza completamente vacío. En el HUD superior se lee `GEN: X0 X0 X0 X0 X0 X0 X0` en rojo, y las 7 bahías inferiores se dibujan con vigas de andamiaje de acero.

![Boot con Andamio](assets/01_scaffold_boot.png)

### 2.2 Menú de Calibración: Atraedor & Generador (Página 1/4)
Accesible con `SELECT` en cualquier momento. Permite ajustar en vivo la cadencia del stream, tier biocasta, tiers del generador y economía.

![Menú de Calibración](assets/02_calib_stream_page0.png)

### 2.3 Árbol de Mejoras: Adquisición de A1 y A2
1. **Tienda Inicial**: A1 disponible por 100$, A2 bloqueado requiriendo A1 (`REQ A1 (TIER 1)`).
2. **Tras comprar A1**: Tier 1 queda activo (`TIER 1 (ACTIVO)`), construyendo la bahía 1 del generador y desbloqueando A2 por 350$.

| Tienda Previa | Tras Comprar A1 (Tier 1 Erecto) |
| :---: | :---: |
| ![Tienda Inicial](assets/03_shop_unbought.png) | ![A1 Comprado](assets/04_shop_bought_a1.png) |

### 2.4 Pantalla de Fin de Demo (Fase 1 Completada)
Al comprar A2, el juego entra automáticamente en la pantalla de celebración:

![Fin de la Demo](assets/05_victory_phase1_complete.png)

Se observa en el pie de pantalla que **Tier 1 y Tier 2** lucen sus 5 micro-lámparas verdes encendidas, mientras las bahías 3 a 7 permanecen en andamio.

### 2.5 Modo Infinito con Auto-Apuntado Activo
Al pulsar `A` se reanuda la simulación continua. Los 4 cañones rastrean y destruyen autónomamente a los enemigos a 60 FPS fijos.

![Defensa Continua](assets/07_stream_defense_with_a2.png)

Animación de combate continuo en el nuevo sistema:

![Animación de Combate](assets/phase1_combat.gif)

---

## 3. Verificación de Rendimiento & Invariantes

- **Escenario Determinista**: `scenarios/phase1_vertical_slice.json` ejecutado en DeSmuME headless (`scripts/run-scenario.ps1`). Resultado: `DSM_SCENARIO_RESULT=PASS captures=7 events=94`.
- **Frame Budget**: 60 FPS estables (`FPS:60 T:61 B:227 P:53 S:26 E:8`). Coste de renderizado y simulación muy por debajo del límite de 545 ticks.
- **Sin Game Over**: El generador degrada sus automatizaciones al recibir impactos sin cortar la partida.
