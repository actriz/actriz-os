#!/bin/bash
PCT="${1:-20}"
[[ "$PCT" =~ ^[0-9]+$ ]] || PCT=20
[ "$PCT" -lt 0 ] && PCT=0
[ "$PCT" -gt 100 ] && PCT=100

BG=$(awk -v p="$PCT" 'BEGIN{printf "#%02X%02X%02X", p*255/100, p*255/100, p*255/100}')
[ "$PCT" -lt 50 ] && FG="#E6E6E6" || FG="#1A1A1A"

TOML="$HOME/.config/alacritty/alacritty.toml"
if [ -f "$TOML" ]; then
    sed -i "s|^background = .*|background = \"$BG\"|" "$TOML"
    sed -i "s|^foreground = .*|foreground = \"$FG\"|" "$TOML"
fi
pgrep -x alacritty >/dev/null && pkill -USR1 -x alacritty 2>/dev/null

[ "$PCT" -lt 50 ] && GT="Adwaita-dark" || GT="Adwaita"
gsettings set org.gnome.desktop.interface gtk-theme "$GT" 2>/dev/null
GTK_INI="$HOME/.config/gtk-3.0/settings.ini"
[ -f "$GTK_INI" ] && sed -i "s|gtk-theme-name=.*|gtk-theme-name=$GT|" "$GTK_INI"

mkdir -p "$HOME/.local/state/actriz"
echo "$PCT" > "$HOME/.local/state/actriz/tema.txt"
echo "Tema: $PCT% (bg=$BG fg=$FG gtk=$GT)"
