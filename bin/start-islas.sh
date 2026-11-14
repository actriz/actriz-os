#!/bin/bash

# 1. Limpiar instancias viejas
pkill -x isla
pkill -x isla_inferior
pkill -x bote_reloj
pkill -x bote_wifi_bat
pkill -x borde
sleep 0.2

# 2. Asegurarnos de que Picom esté corriendo
if ! pgrep -x picom > /dev/null; then
    picom --config ~/.config/picom/picom.conf &
    sleep 0.5 # Darle medio segundo a Picom para estar listo
fi

# 3. Lanzar las Islas y los Botes
/opt/actriz/entornografico/navegacion/ISLA_SUPERIOR/isla &
/opt/actriz/entornografico/navegacion/ISLA_INFERIOR/isla_inferior &
/opt/actriz/entornografico/navegacion/ISLA_SUPERIOR/bote_reloj &
/opt/actriz/entornografico/navegacion/ISLA_SUPERIOR/bote_wifi_bat &
/opt/actriz/entornografico/navegacion/BORDE/borde &
