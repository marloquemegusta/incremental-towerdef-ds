# TowerDS - Catálogo de Assets y Sprites

Este directorio contiene los sprites, animaciones y capturas oficiales de `towerds`, diseñados a resolución nativa de Nintendo DS y optimizados para el hardware de la consola.

---

## 1. Estructura de Archivos

```
assets/
├── sprites/
│   ├── turrets/
│   │   ├── heavy_bolter_recoil.gif          # Animación oficial del Twin Heavy Bolter con retroceso y chispas
│   │   ├── heavy_bolter_recoil_strip.png    # Tira de fotogramas (filmstrip) del Heavy Bolter
│   │   ├── lascannon_recoil.gif             # Animación oficial del Lascannon v2 con carga térmica
│   │   ├── lascannon_recoil_strip.png       # Tira de fotogramas (filmstrip) del Lascannon
│   │   ├── ds_turrets_12_proposals_1x.png   # 12 propuestas de torretas a resolución nativa DS 1:1
│   │   └── ds_turrets_12_proposals_8x.png   # 12 propuestas escaladas a 8x con Nearest Neighbor
│   │
│   ├── particles/
│   │   ├── bolter_particle_sparks.gif       # Opción 3 (Seleccionada): Chispas de tungsteno por fricción
│   │   ├── bolter_particle_sparks_strip.png
│   │   ├── bolter_particle_clean.gif        # Opción 1: Puro mecánico sin partículas
│   │   ├── bolter_particle_clean_strip.png
│   │   ├── bolter_particle_smoke.gif        # Opción 2: Bocanadas de gas caliente
│   │   ├── bolter_particle_smoke_strip.png
│   │   ├── bolter_particle_casing_spin.gif  # Opción 4: Casquillo giratorio eyectado
│   │   └── bolter_particle_casing_spin_strip.png
│   │
│   └── enemies/
│       ├── ds_enemies_1x.png                # Micro-sprites direccionales de xenos a resolución nativa DS
│       └── ds_enemies_8x.png                # Micro-sprites direccionales escalados a 8x Nearest Neighbor
│
└── screenshots/
    ├── wide_prep.png                        # Pantalla de preparación con trinchera continua de 32px
    ├── wide_placed.png                      # Despliegue de torreta táctica en pasarela central
    ├── wide_combat.png                      # Combate en trinchera ancha contra la horda
    ├── massive_bolter_combat.png            # Combate con cañones masivos y retroceso balístico
    ├── ds_ingame_mockup_1x.png              # Mockup conceptual a 1x nativo (pantalla doble DS)
    └── ds_ingame_mockup_4x.png              # Mockup conceptual a 4x
```

---

## 2. Especificaciones Técnicas

- **Resolución por pantalla:** 256 x 192 píxeles (pantalla inferior táctil y superior de telemetría).
- **Profundidad de color:** BGR555 (15 bits por píxel, `RGB15(r,g,b)` con 0 <= r,g,b <= 31).
- **Tilesets:** Formato cuadrícula de 16 x 16 píxeles.
- **Trinchera balística:** Canal continuo de 32 píxeles de anchura interior con balizas de peligro perimetrales.
- **Micro-sprites del enjambre:** 5 x 5 píxeles con pata/garra animada y vectores direccionales (Norte, Sur, Este, Oeste).
- **Curva balística del Twin Heavy Bolter:** Impulso explosivo instantáneo en 1 fotograma (-4px), retención de pico de cerrojo (-4px) y retorno amortiguado por muelle (-2px -> -1px -> 0px).
- **Sistema de partículas activo:** Opción 3 (Chispas de tungsteno al abrirse la recámara en cada ciclo de disparo).
