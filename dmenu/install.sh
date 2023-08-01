#!/bin/sh

path="$HOME/.local/bin/dmenu"
scripts="dmenu-wallpaper-selection dmenu-wifiprompt"

uninstall() {
        echo "Deleting dmenu scripts."

	for script in $scripts; do
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
	cp shell-scripts/$script $path/
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
