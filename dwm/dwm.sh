#!/bin/bash

compiledscripts="audiocontrol mediacontrol"
shellscripts="dwm-start swaplanguage"
dir="$HOME/.local/bin/dwm"

if [ "$1" == "uninstall" ]; then
    echo "Deleting dwm scripts"
    
    sudo rm -f /usr/share/xsessions/dwm.desktop
    
    for script in $shellscripts; do
        rm -f $dir/$script
    done

    for script in $compiledscripts; do
        rm -f $dir/$script
    done

    if [ "$(ls -A $dir)" ]; then
        rm -rf $dir
    fi
else
    echo "Copying dwm scripts"
    
    sudo mkdir -p /usr/share/xsessions
    sudo cp desktop-files/dwm.desktop /usr/share/xsessions/dwm.desktop

    mkdir -p $dir

    for script in $shellscripts; do
        cp shell-scripts/$script $dir/$script
    done

    for script in $compiledscripts; do
        gcc -Wall -Os c-source/$script.c -o c-source/$script
        cp c-source/$script $dir/$script
        rm -f c-source/$script
    done
fi
