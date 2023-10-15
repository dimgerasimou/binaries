#!/bin/sh

/usr/bin/picom -b

setxkbmap -layout us,gr -option grp:win_space_toggle

$HOME/.local/bin/dwm/xrandr

(sleep 0.2 && (nohup easyeffects --gapplication-service &)) &
(sleep 1 && ($HOME/.local/bin/dwm/audiocontrol sink set 0.8) && ($HOME/.local/bin/dwm/audiocontrol source set 1)) &

feh --bg-fill $HOME/.local/state/dwm/wallpaper.jpg

/usr/lib/mate-polkit/polkit-mate-authentication-agent-1 &

/usr/local/bin/dwmblocks &

/usr/bin/xautolock -time 60 -locker "slock" &

/usr/bin/dunst &

(sleep 0.2 && (/usr/bin/systemctl --user import-environment DISPLAY)) &
(sleep 0.3 && (/usr/bin/clipmenud &)) &
