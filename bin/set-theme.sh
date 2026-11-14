#!/bin/bash
if [ "$1" == "dark" ]; then
    gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita-dark' 2>/dev/null
    sed -i 's/gtk-theme-name=.*/gtk-theme-name=Adwaita-dark/' ~/.config/gtk-3.0/settings.ini
else
    gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita' 2>/dev/null
    sed -i 's/gtk-theme-name=.*/gtk-theme-name=Adwaita/' ~/.config/gtk-3.0/settings.ini
fi

# Reiniciar Thunar para que aplique el color (se cerrará si estaba abierto)
pkill thunar 2>/dev/null
