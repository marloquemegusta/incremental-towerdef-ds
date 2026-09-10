# TowerDS - Filosofía y Especificación de Diseño

Documento canónico de visión, mecánicas, escalado incremental y arquitectura de juego para **TowerDS** (Nintendo DS).

---

## 1. Visión y Pilares del Proyecto
- **Plataforma:** Nintendo DS (Hardware real & DeSmuME headless).
- **Género:** Tower Defense Incremental Roguelite.
- **Ambientación:** Sector Forja del Adeptus Mechanicus bajo asedio de un Enjambre Bio-Xenos (Warhammer 40.000 grimdark industrial).
- **Core Loop:**
  1. *Fase de Preparación:* Análisis táctico del mapa procedimental, despliegue de baterías de torretas y orientación de conos de fuego.
  2. *Fase de Combate:* Simulación balística pura a 60 FPS con acelerador de tiempo `[2X]`.
  3. *Fase de Forja (Intra-Run):* Inversión de chatarra en el árbol de especialización técnica.
  4. *Fase de Muerte / Victoria (Meta):* Conversión de hazañas en Datos STC para activar Directivas de Riesgo-Recompensa.

---

## 2. Estructura de Campaña: La Gran Purga (3x3 = 9 Mapas)
Una run completa consta de **3 Sectores con 3 Mapas Procedimentales cada uno (9 niveles en total)**:

| Sector | Terreno Procedimental | Tipos de Enemigo | Desafío Táctico |
|---|---|---|---|
| **Sector 1: Perímetro Exterior** (Mapas 1-3) | Trincheras abiertas de 32px, curvas simples en "S". | **Micro-Plaga (Ligero):** 0 Armadura, vida baja, gran número. | Establecer economía inicial y baterías antifantería. |
| **Sector 2: Complejo de Fundición** (Mapas 4-6) | Bifurcaciones, pasarelas dobles y cuellos de botella. | **Ligeros + Acorazados:** Armadura plana de 2 a 5 puntos. | Necesidad de armas pesadas con alta penetración (Lascannon). |
| **Sector 3: Sanctum Imperial** (Mapas 7-9) | Desfiladeros estrechos y fosos de magma. | **Ligeros + Acorazados + Sprinters rápidos + Bio-Titán** en mapa 9. | Sinergia extrema de build y control de masas. |

- **Integridad del Bastión (Una Sola Vida):** El Sanctum tiene 20 puntos de integridad. Cada enemigo que llega al núcleo resta 1 punto. Si llega a 0, la run concluye inmediatamente.

---

## 3. Mecánica de Combate y Armadura Plana
El daño se resuelve mediante **mitigación plana de blindaje**:
$$\text{Daño Aplicado} = \max(1, \text{Daño Bala} - \text{Armadura})$$

- **Armas de Alta Cadencia / Bajo Daño (Twin Heavy Bolter):** Letales contra enjambres sin armadura; sufren drásticamente contra blindajes altos.
- **Armas de Baja Cadencia / Alto Daño (Lascannon):** Anulan la armadura plana y atraviesan varios objetivos en línea.
- **Armas de Saturación Térmica (Heavy Flamer):** Ignoran armadura ligera y aplican daño sobre el tiempo en cuellos de botella.

---

## 4. El Escalado Incremental (De 20 a 20.000.000)

### A. Límite Físico del Hardware vs Densidad de Biomasa
- En la pantalla de $256 \times 192$, la población simultánea se fija en **40 a 120 sprites de 5x5 px**. Esto garantiza legibilidad absoluta y 60 FPS estables en el ARM9.
- El escalado incremental no se logra aumentando el número de entidades a infinito, sino escalando la **Densidad de Biomasa y Valor**:
  - Un sprite en Sector 1 tiene 2 HP y otorga 1 de chatarra.
  - Un sprite evolucionado en Sector 3 tiene 50.000 HP y otorga 25.000 de chatarra.

### B. Multiplicadores Compuestos en Cascada
La economía intra-run escala exponencialmente mediante la combinación de factores:
$$\text{Ingresos} = (\text{Valor Base}) \times (\text{Nivel de Reciclaje}) \times (\text{Racha Pura}) \times (\text{Interés del Diezmo})$$

1. **Ramas Excluyentes de Especialización en el Taller:**
   - **Rama Rápida (Saturación & Flujo):** Cadencia brutal, proyectiles expansivos y supresión.
   - **Rama Pesada (Artillería & Perforación):** Daño demoledor por impacto, penetración de blindaje y radio de explosión.
   - **Rama Económica (Diezmo Sagrado):** Multiplicador de chatarra por baja e interés compuesto (+5-10% por mapa sin daños).

---

## 5. Metaprogresión Permanente: Directivas de Riesgo-Recompensa
Al terminar una run (victoria o derrota), las bajas se transforman en **Datos Sagrados STC**. 

En lugar de mejoras planas que vuelvan el Sector 1 trivial, los STC desbloquean:
1. **Planos STC:** Desbloqueo de nuevas familias de torretas (Flamer, Misiles, Tesla) en el pool de compra del run.
2. **Directivas de Herejía (Modificadores activables estilo Pacto de Hades):**
   - *Directiva de Híper-Propulsión:* Enemigos +40% más veloces, pero otorgan +100% de chatarra.
   - *Directiva de Caparazón Reforzado:* Todos los enemigos obtienen +2 de armadura plana; +150% de Datos STC al finalizar.
   - *Directiva de Sacrificio:* Torretas con +50% de daño; el Bastión solo tiene 5 puntos de integridad.
