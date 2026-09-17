# TowerDS - Catálogo Canónico de Assets y Sprites

Este directorio contiene los sprites, animaciones y tilesets canónicos de `towerds`, diseñados a resolución nativa de Nintendo DS y optimizados para el hardware físico y emulación determinista.

---

## 1. Estructura Canónica de Sprites

```
assets/sprites/
├── turrets/
│   ├── heavy_bolter_mars_red_strip_master_1x.png  # [CANÓNICO] Cinta de 5 ángulos (220x44 px) para muralla
│   ├── heavy_bolter_mars_red_preview_4x.png       # Preview 4x de la torreta canónica
│   ├── wall_turrets_patrol.gif                    # GIF de demostración de los 5 ángulos
│   ├── README.md                                  # Especificaciones de pivote y ángulos
│   └── archive/                                   # Archivo histórico
│       └── classic_td_32x32/                      # Sprites 32x32 de libre colocación (modo clásico)
│
├── wall/
│   ├── bunker_wall_master_1x.png                  # [CANÓNICO] Muralla modular 256x48 px con transparencia
│   ├── bunker_wall_preview_4x.png                 # Preview 4x del muro del bastión
│   ├── ammo_crate_master_1x.png                   # [CANÓNICO] Cajón de munición 26x16 px en X=128, Y=166
│   ├── ammo_crate_preview_4x.png                  # Preview 4x del cajón de suministros
│   └── README.md                                  # Coordenadas de los 4 sockets y especificaciones
│
├── enemies/
│   ├── t1_zergling_walk_strip_master_1x.png       # [CANÓNICO] Zergling enjambre terrestre
│   ├── t1_zergling_attack_strip_master_1x.png
│   ├── t3_scourge_fly_strip_master_1x.png         # [CANÓNICO] Scourge kamikaze aéreo
│   ├── t2_hydralisk_walk_strip_master_1x.png      # [CANÓNICO] Hydralisk infantería pesada
│   ├── t2_hydralisk_attack_strip_master_1x.png
│   ├── sc_mutalisk_fly_strip_master_1x.png        # [CANÓNICO] Mutalisk cazador aéreo
│   ├── sc_defiler_walk_strip_master_1x.png        # [CANÓNICO] Defiler caster de miasma
│   ├── sc_lurker_walk_strip_master_1x.png         # [CANÓNICO] Lurker ariete acorazado
│   ├── sc_guardian_fly_strip_master_1x.png        # [CANÓNICO] Guardian asedio pesado
│   ├── t4_ultralisk_walk_strip_master_1x.png      # [CANÓNICO] Ultralisk coloso
│   ├── t4_ultralisk_attack_strip_master_1x.png
│   ├── README.md                                  # Dimensiones y stats base de las 8 especies
│   └── archive/                                   # Histórico de propuestas previas
│
└── particles/
    ├── bolter_particle_sparks.gif                 # Chispas de tungsteno por impacto/cerrojo
    ├── bolter_particle_casing_spin.gif            # Casquillos dorados eyectados
    └── ...
```

---

## 2. Especificaciones Técnicas y Reglas Canónicas

- **Resolución nativa por pantalla:** 256 x 192 píxeles (doble pantalla DS).
- **Formato de color en hardware:** BGR555 (`RGB15(r,g,b)`).
- **Muralla Modular:** Renderizada en `Y = 144..191` con oclusión 3D sobre los enemigos atacantes y borde superior transparente (`0x0000`).
- **Torretas de Muralla:** `Twin Heavy Bolter Mars Red` montada sobre sockets físicos (`X = 28, 93, 162, 227`) con rotación en 5 ángulos discretos.
- **Exclusividad Cromática Xenos:** La gama púrpura / magenta está reservada exclusivamente para el enjambre biológico. El bastión imperial utiliza tonos acero, latón, rojo Marte y verde oliva.
