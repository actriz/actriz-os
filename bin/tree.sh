#!/bin/bash
DIR="${1:-$HOME}"
[ -d "$DIR" ] || DIR="$HOME"
tree -L 2 --noreport "$DIR" 2>/dev/null | head -150 | while IFS= read -r line; do
    esc=$(printf '%s' "$line" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g')
    case "$line" in
        *"── "*) 
            prefix="${line%%── *}── "
            rest="${line#*── }"
            p_esc=$(printf '%s' "$prefix" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g')
            r_esc=$(printf '%s' "$rest" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g')
            echo "<span foreground='#6CB4EE'>${p_esc}</span><span foreground='#D8DCE3'>${r_esc}</span>"
            ;;
        *)
            echo "<span foreground='#D8DCE3'>${esc}</span>"
            ;;
    esac
done
