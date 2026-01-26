#!/bin/bash

prefix="$HOME/.local"

targets="dwm-audio dwm-screenshot dwm-xkbnext"

uninstall() {
	echo "Uninstalling dwm binaries..."

	for script in $targets; do
		make --directory=$script PREFIX=$prefix uninstall
	done
}

install() {
	echo "Installing dwm binaries..."

	for script in $targets; do
		make --directory=$script PREFIX=$prefix install
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
