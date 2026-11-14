#!/bin/bash
ESTADO="$HOME/.config/actriz/estado"

# 1. Restaurar Fondo de Pantalla
if [ -f "$ESTADO/wallpaper.txt" ]; then
    IMG=$(cat "$ESTADO/wallpaper.txt")
    if [ -f "$IMG" ]; then
        feh --bg-fill "$IMG"
    fi
fi

# 2. Restaurar Brillo
if [ -f "$ESTADO/brillo.txt" ]; then
    PCT=$(cat "$ESTADO/brillo.txt")
    MAX=$(cat /sys/class/backlight/*/max_brightness)
    NEWVAL=$((MAX * PCT / 100))
    echo $NEWVAL > /sys/class/backlight/*/brightness 2>/dev/null
fi

# 3. Restaurar Volumen
if [ -f "$ESTADO/volumen.txt" ]; then
    VOL=$(cat "$ESTADO/volumen.txt")
    amixer -q set Master ${VOL}% 2>/dev/null
fi
