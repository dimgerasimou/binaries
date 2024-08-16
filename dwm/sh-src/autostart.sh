#!/bin/sh

picom -b

setxkbmap -layout us,gr -option

xrandr-setup

(sleep 0.2 && (nohup easyeffects --gapplication-service &)) &
(sleep 1 && ($HOME/.local/bin/dwm/audiocontrol sink set 0.8) && ($HOME/.local/bin/dwm/audiocontrol source set 1)) &

feh --bg-fill $HOME/.local/state/dwm/wallpaper.jpg

lxpolkit &

dwmblocks &

xautolock -time 60 -locker "slock" &

dunst &

solaar --restart-on-wake-up --window hide --tray-icon-size 2 --battery-icons solaar &

(sleep 0.2 && (systemctl --user import-environment DISPLAY)) &
(sleep 0.3 && (clipmenud &)) &
