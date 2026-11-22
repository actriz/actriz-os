#!/bin/bash
# $1 = icono, $2 = titulo, $3 = valor 0-100
ICONO="$1"; TITULO="$2"; VALOR="$3"
notify-send -i "$ICONO" -h int:value:"$VALOR" -h string:x-canonical-private-synchronous:vol "$TITULO" "$VALOR%"
