#!/bin/bash
# Detectar el control de audio real de tu tarjeta
CTRL="Master"
if ! amixer get $CTRL >/dev/null 2>&1; then CTRL="PCM"; fi
if ! amixer get $CTRL >/dev/null 2>&1; then CTRL="Speaker"; fi
if ! amixer get $CTRL >/dev/null 2>&1; then CTRL="Headphone"; fi

if [ "$1" == "mute" ]; then
    amixer -q set $CTRL toggle
    echo -e "\033[1;35m[ABC]:\033[0m Silencio alternado."
elif [ "$1" == "subir" ]; then
    amixer -q set $CTRL ${2:-5}%+
    echo -e "\033[1;35m[ABC]:\033[0m Volumen subido."
elif [ "$1" == "bajar" ]; then
    amixer -q set $CTRL ${2:-5}%-
    echo -e "\033[1;35m[ABC]:\033[0m Volumen bajado."
elif [ "$1" == "cambiar" ]; then
    if [ ! -z "$2" ]; then amixer -q set $CTRL ${2}%; fi
    echo -e "\033[1;35m[ABC]:\033[0m Volumen ajustado."
fi

VOL=$(amixer get $CTRL | grep -o '[0-9]*%' | head -1 | tr -d '%')
if [ ! -z "$VOL" ]; then
    echo "$VOL" > ~/.config/actriz/estado/volumen.txt
fi
