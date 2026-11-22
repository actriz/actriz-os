#!/bin/bash
# Actriz OS - Control de audio universal
# Funciona en cualquier PC/laptop con PipeWire, PulseAudio o ALSA puro.
# Uso: control.sh <subir|bajar|cambiar|mute|get> [valor]

ACCION="${1:-}"
VALOR="${2:-}"

# ------------------------------------------------------------------
# 1. DETECTAR BACKEND (con auto-arranque si esta instalado pero dormido)
# ------------------------------------------------------------------
# Prioridad: PipeWire > PulseAudio > ALSA
# Motivo: si existen dos, PipeWire es el que realmente controla hoy,
#         PulseAudio solo actua como compatibilidad encima de el.
detectar_backend() {
    # PipeWire
    if command -v wpctl >/dev/null 2>&1; then
        if wpctl status >/dev/null 2>&1; then
            echo "pipewire"; return
        fi
        # Instalado pero no corriendo: intentar arrancarlo
        systemctl --user start pipewire wireplumber pipewire-pulse 2>/dev/null
        sleep 0.3
        if wpctl status >/dev/null 2>&1; then
            echo "pipewire"; return
        fi
    fi
    # PulseAudio
    if command -v pactl >/dev/null 2>&1; then
        if pactl info >/dev/null 2>&1; then
            echo "pulse"; return
        fi
        # Instalado pero no corriendo
        pulseaudio --start >/dev/null 2>&1
        sleep 0.3
        if pactl info >/dev/null 2>&1; then
            echo "pulse"; return
        fi
    fi
    # ALSA puro (ultimo recurso)
    if command -v amixer >/dev/null 2>&1; then
        echo "alsa"; return
    fi
    echo "none"
}

BACKEND=$(detectar_backend)

if [ "$1" = "get" ]; then
    case "$BACKEND" in
        pipewire) wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null | awk '{printf "%d\n", $2*100}' ;;
        pulse)    pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null | grep -o '[0-9]*%' | head -1 | tr -d '%' ;;
        alsa)     CTRL_G=$(amixer scontrols 2>/dev/null | head -1 | sed -n "s/.*'\([^']*\)'.*/\1/p");
                  amixer sget "$CTRL_G" 2>/dev/null | grep -o '[0-9]*%' | head -1 | tr -d '%' ;;
    esac
    exit 0
fi

if [ "$BACKEND" = "none" ]; then
    echo -e "\033[1;31m[ABC]:\033[0m No hay backend de audio (wpctl / pactl / amixer)."
    echo -e "\033[1;33m[ABC]:\033[0m Instala uno: sudo apt install pipewire-audio"
    exit 1
fi

# ------------------------------------------------------------------
# 2. ALSA: elegir un control que ACEPTE volumen (no todos lo hacen)
# ------------------------------------------------------------------
# IEC958/S/PDIF es solo salida digital fija: no acepta subir/bajar.
# Master/PCM/Speaker/Headphone/Front/Playback si. Por eso probamos
# varios nombres antes de rendirnos. Ademas miramos si el control
# tiene "Playback" (volumen) y no solo "Capture" (microfono).
elegir_ctrl_alsa() {
    for c in Master PCM Speaker Headphone Front Playback "Digital" "Surround" Center "Analog" "hw:0"; do
        if amixer sget "$c" 2>/dev/null | grep -q "Playback"; then
            # Comprobar que tiene pasos de volumen (no es solo un switch on/off)
            if amixer sget "$c" 2>/dev/null | grep -q "Limits: Playback"; then
                echo "$c"; return 0
            fi
        fi
    done
    # Nada con nombre conocido: tomar el primero que tenga "Limits: Playback"
    for c in $(amixer scontrols 2>/dev/null | sed -n "s/.*'\([^']*\)'.*/\1/p"); do
        if amixer sget "$c" 2>/dev/null | grep -q "Limits: Playback"; then
            echo "$c"; return 0
        fi
    done
    return 1
}

if [ "$BACKEND" = "alsa" ]; then
    CTRL=$(elegir_ctrl_alsa)
    if [ -z "$CTRL" ]; then
        echo -e "\033[1;31m[ABC]:\033[0m Tu tarjeta ALSA no expone un control con volumen."
        echo -e "\033[1;33m[ABC]:\033[0m Instala PipeWire: sudo apt install pipewire-audio"
        exit 1
    fi
fi

# ------------------------------------------------------------------
# 3. APLICAR ACCION
# ------------------------------------------------------------------
aplicar() {
    case "$BACKEND" in
        pipewire)
            case "$1" in
                subir)    wpctl set-volume @DEFAULT_AUDIO_SINK@ "${2:-5}%+" ;;
                bajar)    wpctl set-volume @DEFAULT_AUDIO_SINK@ "${2:-5}%-" ;;
                cambiar)  wpctl set-volume @DEFAULT_AUDIO_SINK@ "${2}%" ;;
                desmute)  wpctl set-mute   @DEFAULT_AUDIO_SINK@ 0 ;;
      mute)     wpctl set-mute   @DEFAULT_AUDIO_SINK@ toggle ;;
                get)      wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null | awk '{printf "%d\n", $2*100}' ;;
            esac ;;
        pulse)
            case "$1" in
                subir)    pactl set-sink-volume @DEFAULT_SINK@ "+${2:-5}%" ;;
                bajar)    pactl set-sink-volume @DEFAULT_SINK@ "-${2:-5}%" ;;
                cambiar)  pactl set-sink-volume @DEFAULT_SINK@ "${2}%" ;;
                desmute)  pactl set-sink-mute   @DEFAULT_SINK@ 0 ;;
      mute)     pactl set-sink-mute   @DEFAULT_SINK@ toggle ;;
                get)      pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null | grep -o '[0-9]*%' | head -1 | tr -d '%' ;;
            esac ;;
        alsa)
            case "$1" in
                subir)    amixer -q sset "$CTRL" "${2:-5}%+" ;;
                bajar)    amixer -q sset "$CTRL" "${2:-5}%-" ;;
                cambiar)  amixer -q sset "$CTRL" "${2}%" ;;
                desmute)  amixer -q sset "$CTRL" unmute ;;
      mute)     amixer -q sset "$CTRL" toggle ;;
                get)      amixer sget "$CTRL" 2>/dev/null | grep -o '[0-9]*%' | head -1 | tr -d '%' ;;
            esac ;;
    esac
}

# `get` es solo lectura, no aplica
if [ "$ACCION" = "get" ]; then
    aplicar get
    exit 0
fi

if [ -z "$VALOR" ] && [ "$ACCION" != "mute" ]; then
    VALOR=5
fi

aplicar "$ACCION" "$VALOR"
RC=$?

if [ $RC -ne 0 ]; then
    echo -e "\033[1;31m[ABC]:\033[0m Fallo al aplicar $ACCION (backend: $BACKEND)."
    exit 1
fi

# ------------------------------------------------------------------
# 4. MENSAJE
# ------------------------------------------------------------------
case "$ACCION" in
    subir)    echo -e "\033[1;35m[ABC]:\033[0m Volumen subido." ;;
    bajar)    echo -e "\033[1;35m[ABC]:\033[0m Volumen bajado." ;;
    cambiar)  echo -e "\033[1;35m[ABC]:\033[0m Volumen ajustado." ;;
    mute)     echo -e "\033[1;35m[ABC]:\033[0m Silencio alternado." ;;
    desmute)  echo -e "\033[1;35m[ABC]:\033[0m Silencio desactivado." ;;
esac

# ------------------------------------------------------------------
# 5. GUARDAR ESTADO (opcional, portable)
# ------------------------------------------------------------------
# Usamos XDG_STATE_HOME si existe, si no ~/.local/state
# (funciona en cualquier distro; no depende de ~/.config)
ESTADO_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/actriz"
mkdir -p "$ESTADO_DIR" 2>/dev/null
VOL=$(aplicar get)
if [ -n "$VOL" ]; then
    echo "$VOL" > "$ESTADO_DIR/volumen.txt" 2>/dev/null
fi
