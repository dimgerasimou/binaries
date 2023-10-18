#!/bin/bash

path="$HOME/.local/bin/dwm"

cscripts="audiocontrol mediacontrol takescreenshot xrandr-set"
shscripts="autostart.sh layoutmenu"

CFLAGS="-Os -Wall -lX11 -lXrandr"
LFLAGS="`pkg-config --cflags --libs libnotify`"

uninstall() {
	echo "Deleting dwm scripts."
    
	sudo rm -f /usr/share/xsessions/dwm.desktop
    
	for script in $shscripts; do
		rm -f $path/$script
	done

	for script in $cscripts; do
		rm -f $path/$script
	done

	if [ -z "$(ls -A $path)" ]; then
		rm -rf $path
	fi
}

install() {
	echo "Copying dwm scripts."

	sudo mkdir -p /usr/share/xsessions
	sudo cp misc-src/dwm.desktop /usr/share/xsessions/dwm.desktop

        mkdir -p $path

	for script in $shscripts; do
		cp sh-src/$script $path/$script
	done

	cd c-src
	for script in $cscripts; do
		gcc -o $script $script.c $CFLAGS $LFLAGS
		mv $script $path/$script
	done
	cd ..
}

case $1 in
	-r|--uninstall)
		uninstall;;

	-h|--help)
		echo "Usage: install.sh [-r | --uninstall]"
		echo "default: install";;
	
	-*|--*)
		echo "Unknown option $1"
		exit 1;;
	*)
		install;;
esac
