#!/bin/sh

prefix="$HOME/.local"

shscripts="dmenu-wallpaper-selection"
cscripts="dmenu-audio-source-select dmenu-equalizer-select dmenu-wifi-prompt"

uninstall() {
        echo "Deleting dmenu scripts."

	for script in $cscripts; do
		make --directory=c-src/$script PREFIX=$prefix uninstall
	done

	for script in $shscripts; do
		rm -f $prefix/bin/$script
	done
}

install() {
	echo "Copying dmenu scripts."

	mkdir -p $prefix/bin

	for script in $shscripts; do
		cp sh-src/$script $prefix/bin/$script
	done

	for script in $cscripts; do
		make --directory=c-src/$script PREFIX=$prefix install
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
