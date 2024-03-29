#!/bin/sh

path="$HOME/.local/bin/dmenu"
scripts="dmenu-wallpaper-selection dmenu-wifiprompt"
cscripts="dmenu-audio-source-select"

uninstall() {
        echo "Deleting dmenu scripts."

	for script in $scripts; do
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
	echo "Copying dmenu scripts."

	mkdir -p $path

	for script in $scripts; do
	cp sh-src/$script $path/
	done

	for script in $cscripts; do
		gcc c-src/$script.c -o c-src/$script
		mv c-src/$script $path/
	done
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
