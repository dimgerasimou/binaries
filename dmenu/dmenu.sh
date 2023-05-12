#!/bin/sh

path="$HOME/.local/bin/dmenu"
shscripts="dmenu-wallpaper-selection dmenu-wifiprompt"
cscripts=""

if [ "$1" == "uninstall" ]; then
        echo "Deleting dmenu scripts."

	for script in $shscripts; do
		rm -f $path/$script
	done

	for script in $cscripts; do
		rm -f $path/$script
	done

	if [ -z "$(ls -A $path)" ]; then
		rm -rf $path
	fi
	
	exit
fi

echo "Copying dmenu scripts."

mkdir -p $path

for script in $shscripts; do
	cp shell-scripts/$script $path/
done

for script in $cscripts; do
	gcc c-scripts/$script.c -Wall -Os -o c-scripts/$script
	mv c-scripts/$script $path/
done
