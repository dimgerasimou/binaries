#!/bin/sh

/usr/bin/picom -b

/usr/local/bin/dwmblocks &

setxkbmap -layout us,gr -option grp:win_space_toggle

(sleep 0.2 && (xrandr --output HDMI-0 --rate 75 --right-of eDP-1-1 --primary --output eDP-1-1 --rate 144)) &

(sleep 0.5 && (nohup easyeffects --gapplication-service &)) &

(sleep 0.2 && (feh --bg-fill $HOME/.local/state/dwm/Wallpaper.jpg)) &

(sleep 0.2 && (/usr/bin/wpctl set-volume @DEFAULT_AUDIO_SINK@ 0.8)) &
(sleep 0.2 && (/usr/bin/wpctl set-mute @DEFAULT_AUDIO_SOURCE@ 1)) &
(sleep 0.6 && (pkill -RTMIN+10 dwmblocks))&

/usr/lib/mate-polkit/polkit-mate-authentication-agent-1 &

/usr/bin/dunst &