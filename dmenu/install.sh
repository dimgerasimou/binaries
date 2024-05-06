#!/bin/sh

path="$HOME/.local/bin/dmenu"
scripts="dmenu-wallpaper-selection"
cscripts="dmenu-audio-source-select"
mkscripts="wifi-prompt"

uninstall() {
        echo "Deleting dmenu scripts."

	for script in $scripts; do
		rm -f $path/$script
	done

	for script in $cscripts; do
		rm -f $path/$script
	done

	for script in $mkscripts; do
		rm -f $path/dmenu-$script
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

	for script in $mkscripts; do
		make -C c-src/$script/
		cp c-src/$script/bin/$script $path/dmenu-$script
	done
}

clean() {
	echo "Cleaning dmenu scripts."

	for script in $mkscripts; do
		make -C c-src/$script/ clean
	done
}

case $1 in
	-r|--uninstall)
		uninstall;;
	
	-c|--clean)
		clean;;

	-h|--help)
		echo "Usage: install.sh [-r | --uninstall] [-c | --clean]"
		echo "default: install";;
	
	-*|--*)
		echo "Unknown option $1"
		exit 1;;
	*)
		install;;
esac
