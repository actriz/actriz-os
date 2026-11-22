#!/bin/bash
echo "=================================================="
echo "   DETECTOR DE TECLAS - KEU OS / ACTRIZ OS"
echo "=================================================="
echo "1. Se abrirá una ventana pequeña."
echo "2. Pon el ratón DENTRO de esa ventana."
echo "3. Presiona la tecla que quieres configurar."
echo "4. Mira esta terminal para ver el nombre de la tecla."
echo "=================================================="
sleep 2
xev -event keyboard | grep --line-buffered "keysym" | while read -r line; do
    KEY=$(echo "$line" | grep -oP 'keysym \w+, \K\w+')
    if [ ! -z "$KEY" ]; then
        echo -e "\n👉 Tecla detectada: \033[1;32m$KEY\033[0m"
        echo -e "Añade esto a tu i3 config:"
        echo -e "\033[1;36mbindsym $KEY exec TU_COMANDO_AQUI\033[0m\n"
    fi
done
