#!/bin/sh

/usr/bin/picom -b

setxkbmap -layout us,gr -option grp:win_space_toggle

(sleep 0.2 && (xrandr --output HDMI-0 --rate 75 --right-of eDP-1-1 --primary --output eDP-1-1 --rate 144)) &

(sleep 0.2 && (nohup easyeffects --gapplication-service &)) &
(sleep 0.3 && ($HOME/.local/bin/dwm/audiocontrol sink set 0.8)) &
(sleep 0.3 && ($HOME/.local/bin/dwm/audiocontrol source set 1)) &


(sleep 0.2 && (feh --bg-fill $HOME/.local/state/dwm/wallpaper.jpg)) &

/usr/lib/mate-polkit/polkit-mate-authentication-agent-1 &

/usr/bin/emacs --daemon &

/usr/local/bin/dwmblocks &

/usr/bin/xautolock -time 60 -locker "slock" &

/usr/bin/dunst &

(sleep 0.2 && (/usr/bin/systemctl --user import-environment DISPLAY)) &
(sleep 0.3 && (/usr/bin/clipmenud &)) &
