#!/bin/bash
if [ "$1" == "apagar" ]; then
    xset dpms force off
    echo -e "\033[1;35m[ABC]:\033[0m Pantalla apagada."
    exit 0
fi

CURR=$(cat /sys/class/backlight/*/brightness)
MAX=$(cat /sys/class/backlight/*/max_brightness)
PCT=$((CURR * 100 / MAX))
VAL=${2:-10}

if [ "$1" == "subir" ]; then PCT=$((PCT + VAL)); fi
if [ "$1" == "bajar" ]; then PCT=$((PCT - VAL)); fi
if [ "$1" == "cambiar" ]; then PCT=$VAL; fi

if [ $PCT -gt 100 ]; then PCT=100; fi
if [ $PCT -lt 1 ]; then PCT=1; fi

NEWVAL=$((MAX * PCT / 100))
echo $NEWVAL > /sys/class/backlight/*/brightness

# Guardar en la memoria persistente
echo "$PCT" > ~/.config/actriz/estado/brillo.txt
case "$1" in
    subir)   echo -e "\033[1;35m[ABC]:\033[0m Brillo subido al \033[1;33m$PCT%\033[0m" ;;
    bajar)   echo -e "\033[1;35m[ABC]:\033[0m Brillo bajado al \033[1;33m$PCT%\033[0m" ;;
    cambiar) echo -e "\033[1;35m[ABC]:\033[0m Brillo ajustado al \033[1;33m$PCT%\033[0m" ;;
    *)       echo -e "\033[1;35m[ABC]:\033[0m Brillo al \033[1;33m$PCT%\033[0m" ;;
esac
