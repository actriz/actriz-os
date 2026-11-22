#!/bin/bash
STATE="/tmp/.abc_sys_state"

ram_t=$(awk '/MemTotal:/{print $2}' /proc/meminfo)
ram_a=$(awk '/MemAvailable:/{print $2}' /proc/meminfo)
ram_f=$(awk '/MemFree:/{print $2}' /proc/meminfo)
ram_u_g=$(awk -v t=$ram_t -v a=$ram_a 'BEGIN{printf "%.2f", (t-a)/1048576}')
ram_t_g=$(awk -v t=$ram_t 'BEGIN{printf "%.1f", t/1048576}')
ram_f_g=$(awk -v f=$ram_f 'BEGIN{printf "%.1f", f/1048576}')
ram_p=$(awk -v t=$ram_t -v a=$ram_a 'BEGIN{printf "%.0f", 100*(t-a)/t}')

cpu_line=$(head -1 /proc/stat)
cpu_p=$(echo "$cpu_line" | awk '{u=$2+$4; t=$2+$3+$4+$5+$6+$7+$8; printf "%.0f", (t>0)?100*(t-u)/t:0}')

l1=$(awk '{print $1}' /proc/loadavg)
l5=$(awk '{print $2}' /proc/loadavg)
l15=$(awk '{print $3}' /proc/loadavg)

temp=""
for z in 0 1 2 3 4; do
  v=$(cat /sys/class/thermal/thermal_zone$z/temp 2>/dev/null)
  if [ -n "$v" ] && [ "$v" -gt 0 ]; then temp=$((v/1000)); break; fi
done
[ -z "$temp" ] && temp="?"

rom_u=$(df -BG / | awk 'NR==2{gsub("G","",$3); print $3}')
rom_t=$(df -BG / | awk 'NR==2{gsub("G","",$2); print $2}')
rom_p=$(awk -v u=$rom_u -v t=$rom_t 'BEGIN{printf "%.0f", (t>0)?100*u/t:0}')

gpu="---"
gpu_vendor=$(cat /sys/class/drm/card0/device/vendor 2>/dev/null)
case "$gpu_vendor" in
  0x1002) gpu="AMD";;
  0x8086) gpu="Intel";;
  0x10de) gpu="NVIDIA";;
esac

bat_cap="?"; bat_short="---"
for b in /sys/class/power_supply/BAT*; do
  [ -d "$b" ] || continue
  bat_cap=$(cat "$b/capacity" 2>/dev/null)
  bs=$(cat "$b/status" 2>/dev/null)
  case "$bs" in
    Charging) bat_short="CHG";;
    Discharging) bat_short="DIS";;
    Full) bat_short="FUL";;
  esac
  break
done

ip_str=$(hostname -I 2>/dev/null | awk '{print $1}')
[ -z "$ip_str" ] && ip_str="0.0.0.0"
ip_hex=$(echo "$ip_str" | awk -F. '{printf "0x%02X%02X%02X%02X", $1, $2, $3, $4}')

now=$(date +%H:%M:%S)
up_s=$(awk '{print int($1)}' /proc/uptime)
up_h=$((up_s / 3600))
up_m=$(( (up_s % 3600) / 60 ))
BUF="/tmp/.abc_sys_buffer"
touch "$BUF"
case $((RANDOM % 22)) in
  0) LINE="<span foreground='#CFA8FF' weight='bold'>CPU</span>  <span foreground='#FFFFFF'>${cpu_p}%</span>" ;;
  1) LINE="<span foreground='#F0E5A8'>    ↳ TEMP</span>  <span foreground='#FFFFFF'>${temp}°C</span>" ;;
  2) LINE="<span foreground='#F0E5A8'>    ↳ LOAD</span>  <span foreground='#FFFFFF'>${l1}</span>" ;;
  3) LINE="<span foreground='#E8E8E8'>        ↳ core0</span>  <span foreground='#FFFFFF'>${cpu_p}%</span>" ;;
  4) LINE="<span foreground='#CFA8FF' weight='bold'>MEM</span>  <span foreground='#FFFFFF'>${ram_p}%</span>" ;;
  5) LINE="<span foreground='#F0E5A8'>    ↳ USED</span>  <span foreground='#FFFFFF'>${ram_u_g}G</span>" ;;
  6) LINE="<span foreground='#F0E5A8'>        ↳ FREE</span>  <span foreground='#FFFFFF'>${ram_f_g}G</span>" ;;
  7) LINE="<span foreground='#CFA8FF' weight='bold'>NET</span>" ;;
  8) LINE="<span foreground='#F0E5A8'>    ↳ IP</span>  <span foreground='#FFFFFF'>${ip_str}</span>" ;;
  9) LINE="<span foreground='#CFA8FF' weight='bold'>SYS</span>" ;;
  10) LINE="<span foreground='#F0E5A8'>    ↳ BAT</span>  <span foreground='#FFFFFF'>${bat_cap}%</span>" ;;
  11) LINE="<span foreground='#F0E5A8'>    ↳ ROM</span>  <span foreground='#FFFFFF'>${rom_p}%</span>" ;;
  12) LINE="<span foreground='#F0E5A8'>    ↳ UP</span>  <span foreground='#FFFFFF'>${up_h}h${up_m}m</span>" ;;
  13) LINE="<span foreground='#E8E8E8'>    ↳ HEX</span>  <span foreground='#FFFFFF'>${ip_hex}</span>" ;;
  14) LINE="<span foreground='#E8E8E8'>    ↳ GPU</span>  <span foreground='#FFFFFF'>${gpu}</span>" ;;
  15) LINE="<span foreground='#E8E8E8'>    ↳ DISK</span>  <span foreground='#FFFFFF'>${rom_u}G</span>" ;;
  16) LINE="<span foreground='#E8E8E8'>    ↳ POLL</span>  <span foreground='#FFFFFF'>${poll_ms}ms</span>" ;;
  17) LINE="<span foreground='#E8E8E8'>    ↳ TICK</span>  <span foreground='#FFFFFF'>${now}</span>" ;;
  18) LINE="<span foreground='#CFA8FF'>────── [end] ──────</span>" ;;
  19) LINE="<span foreground='#F0E5A8'>// </span><span foreground='#E8E8E8'>poll ${poll_ms}ms</span>" ;;
  20) LINE="<span foreground='#E8E8E8'>    ↳ ip</span>  <span foreground='#FFFFFF'>${ip_str}</span>" ;;
  21) LINE="<span foreground='#E8E8E8'>    ↳ df</span>  <span foreground='#FFFFFF'>${rom_u}/${rom_t}G</span>" ;;
esac
printf '%s\n' "$LINE" >> "$BUF"
cnt=$(wc -l < "$BUF")
if [ "$cnt" -gt 500 ]; then
    tail -n 400 "$BUF" > "$BUF.tmp" && mv "$BUF.tmp" "$BUF"
fi
cat "$BUF"
