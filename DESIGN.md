# DESIGN.md - Especificación de Diseño: Tower Defense Incremental DS

## 1. Visión General
Tower Defense minimalista e incremental para Nintendo DS centrado en la geometría de tiro, la simulación pasiva masiva y la telemetría en tiempo real estilo *Factorio* / *The Tower*.

## 2. Pantallas
- **Pantalla Superior (Sub-Engine / Telemetría):**
  - Consola de texto densa (32x24 caracteres).
  - Métricas en tiempo real:
    - Estado de Oleada: Vivos / Totales, Bajas acumuladas, Fugas / Daño sufrido.
    - Salud del Núcleo y Chatarra disponible.
    - Telemetría de Torretas: Disparos totales, Impactos confirmados, Precisión %, Balas perdidas (*wasted shots* %), Daño infligido y DPS en tiempo real.
- **Pantalla Inferior (Main-Engine / Simulación Táctil 256x192 px):**
  - Fondo estilo *radar/blueprint* militar oscuro.
  - Trazado de camino predefinido con curvas en "S" y Núcleo en la meta.
  - Enjambre de enemigos renderizado como partículas brillantes de 2x2 píxeles en modo Bitmap.
  - Torretas geométricas (8x8 px) con cañón móvil visible.
  - Trazadores de balas rápidas (líneas de 2 px).

## 3. Mecánica de Torretas y Barrido Angular
- **Sin auto-apuntado:** Cada torreta barre continuamente un sector angular ($\theta \pm \alpha/2$).
- **Torreta Vulcan (Inicial):**
  - Apertura $\alpha = 45^\circ$.
  - Cañón oscilante con velocidad angular $\omega$.
  - Disparo de trazadores a cadencia fija.
  - Toda bala disparada hacia un ángulo sin enemigos suma al contador de *Balas Perdidas*.

## 4. Controles Táctiles (Fase de Preparación - Sin Puntos Muertos)
- **Colocación:** Arrastrar desde el dock inferior al mapa. Si se suelta en zona inválida (camino u obstáculo), vuelve automáticamente al dock sin penalización.
- **Orientación:** Con la torreta seleccionada, arrastrar con el stylus alrededor de ella orienta el ángulo central $\theta$ y proyecta el cono de 45°.
- **Reubicación:** Arrastrar una torreta ya colocada a otra posición válida. Si se suelta en zona inválida, regresa a su posición previa.
- **Desmantelar:** Botón `[QUITAR]` en la interfaz devuelve la torreta al dock al 100% de reembolso.
- **Botón `[START WAVE]`:** Pasa de la Fase de Preparación a la Fase de Simulación.
- **Hipervelocidad (Fast Forward):** Mantener pulsado el botón `R` o activar botón táctil `[2X]` para acelerar la simulación.

## 5. Ciclo Incremental (Metaprogresión)
- El enjambre otorga Chatarra por cada baja.
- Al terminar la oleada (victoria) o caer el Núcleo (derrota), se accede al **Taller**:
  - Mejoras iniciales comprables:
    1. *Cadencia Vulcan:* Reduce el cooldown de disparo.
    2. *Servomotores:* Aumenta la velocidad de oscilación angular.
    3. *Reciclador de Chatarra:* Multiplicador de chatarra por baja.
  - Botón `[REINTENTAR / SIGUIENTE OLEADA]`.
