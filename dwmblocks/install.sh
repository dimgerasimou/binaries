#!/bin/sh

path="$HOME/.local/bin/dwmblocks"
scripts="time spacer mediacontrol bluetooth-menu keyboard battery date kernel bluetooth internet memory power volume"

CFLAGS="-Os -Wall"
LFLAGS="-lX11 -lxkbfile -lm `pkg-config --cflags --libs libnotify`"

uninstall() {
	echo "Deleting dwmblocks scripts."

	for script in $scripts; do
		rm -f $path/$script
	done

	if [ -z "$(ls -A $path)" ]; then
		rm -rf $path
	fi
}

install() {
	echo "Copying dwmblocks scripts."

	mkdir -p $path
	
	make -C status-bar-exec

	cp status-bar-exec/bin/* $path/
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
