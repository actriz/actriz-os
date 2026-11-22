# ABC (Actriz Basic Command) v6.4

Programa ligero en C++ que traduce frases en español a acciones del sistema.
No usa inteligencia artificial: diccionario + reglas + tolerancia a errores de tipeo.

## Uso

    /opt/actriz/ABC/abc                       # muestra ayuda
    /opt/actriz/ABC/abc "ver fondos"          # lista wallpapers
    /opt/actriz/ABC/abc "pon el brillo a la mitad"
    /opt/actriz/ABC/abc "sube el volumen 10"
    /opt/actriz/ABC/abc "apaga la pantalla"
    /opt/actriz/ABC/abc "cambia el fondo a luna.jpg"

## Qué controla

- Brillo   -> /opt/actriz/drivers/brillo/control.sh
- Volumen  -> /opt/actriz/drivers/audio/control.sh
- Fondo    -> /opt/actriz/bin/set-wallpaper

## Recompilar (si algún día modificas abc.cpp)

    g++ -std=c++17 -O2 -o abc abc.cpp

## Contenido

- abc.cpp  : código fuente (~217 líneas)
- abc      : binario compilado
- README.md: este archivo

## Historia

Formaba parte de /opt/actriz/core/actriz_ai/ junto al motor llama.cpp
y el modelo Qwen3-1.7B. Se separó y conservó al eliminar la IA del sistema.
