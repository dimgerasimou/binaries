#!/bin/bash

prefix="$HOME/.local"

cscripts="dwm-audio-control dwm-media-control dwm-screenshot"
shscripts="dwm-xkbswitch"

uninstall() {
	echo "Deleting dwm scripts."

	for script in $cscripts; do
		make --directory=c-src/$script PREFIX=$prefix uninstall
	done

	for script in $shscripts; do
		rm -f $prefix/bin/$script
	done
}

install() {
	echo "Copying dwm scripts."

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
