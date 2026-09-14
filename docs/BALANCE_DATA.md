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
