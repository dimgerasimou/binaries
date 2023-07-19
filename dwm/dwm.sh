#!/bin/bash

compiledscripts="audiocontrol mediacontrol takescreenshot"
shellscripts="autostart.sh layoutmenu swaplanguage"
path="$HOME/.local/bin/dwm"

if [ "$1" == "uninstall" ]; then
    echo "Deleting dwm scripts."
    
    sudo rm -f /usr/share/xsessions/dwm.desktop
    
    for script in $shellscripts; do
        rm -f $path/$script
    done

    for script in $compiledscripts; do
        rm -f $path/$script
    done

    if [ -z "$(ls -A $path)" ]; then
        rm -rf $path
    fi
else
    echo "Copying dwm scripts."
    
    sudo mkdir -p /usr/share/xsessions
    sudo cp desktop-files/dwm.desktop /usr/share/xsessions/dwm.desktop

    mkdir -p $path

    for script in $shellscripts; do
        cp shell-scripts/$script $path/$script
    done

    for script in $compiledscripts; do
        gcc -Wall -Os c-source/$script.c -o c-source/$script
        mv c-source/$script $path/$script
    done
fi
