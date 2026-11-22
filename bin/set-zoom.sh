#!/bin/bash
LEVEL="${1:-5}"
[[ "$LEVEL" =~ ^[1-9]$ ]] || LEVEL=5
case "$LEVEL" in
    1) SCALE=0.75; FONT=8 ;;
    2) SCALE=0.85; FONT=9 ;;
    3) SCALE=0.95; FONT=10 ;;
    4) SCALE=1.00; FONT=11 ;;
    5) SCALE=1.10; FONT=12 ;;
    6) SCALE=1.20; FONT=13 ;;
    7) SCALE=1.30; FONT=14 ;;
    8) SCALE=1.40; FONT=16 ;;
    9) SCALE=1.50; FONT=18 ;;
esac

gsettings set org.gnome.desktop.interface text-scaling-factor "$SCALE" 2>/dev/null

TOML="$HOME/.config/alacritty/alacritty.toml"
if [ -f "$TOML" ]; then
    if grep -qE "^size[[:space:]]*=" "$TOML"; then
        sed -i -E "s|^size[[:space:]]*=.*|size = $FONT|" "$TOML"
    elif grep -q "^\[font\]" "$TOML"; then
        sed -i "/^\[font\]/a size = $FONT" "$TOML"
    else
        printf "\n[font]\nsize = %d\n" "$FONT" >> "$TOML"
    fi
    pgrep -x alacritty >/dev/null && pkill -USR1 -x alacritty 2>/dev/null
fi

mkdir -p "$HOME/.local/state/actriz"
echo "$LEVEL" > "$HOME/.local/state/actriz/zoom.txt"
echo "Zoom: $LEVEL/9 (scale=$SCALE font=$FONT)"
