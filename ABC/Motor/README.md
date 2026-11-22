# ABC/Motor — planificación futura

Estos directorios están **vacíos por ahora**. Son la estructura planificada
para modularizar `ABC/abc.cpp` (hoy monolítico) en el futuro:

| Directorio | Contenido futuro |
|---|---|
| `lenguaje/` | `normalizar_fonetico`, `levenshtein`, `bolsa_parecida` |
| `interpretacion/` | diccionarios (`D_SUBIR`, `D_BAJAR`, etc.), parser de tokens |
| `contexto/` | historial persistente (CTX-V66B), `accion_heredada` |
| `ejecucion/` | `system()` a `drivers/*/control.sh`, `set-wallpaper` |
| `include/` | cabeceras compartidas (.h) |
| `src/` | implementaciones (.cpp) |

Cuando se aborde la modularización, mover el código de `abc.cpp` aquí
y dejar `abc.cpp` como simple `main()` que orquesta los módulos.
