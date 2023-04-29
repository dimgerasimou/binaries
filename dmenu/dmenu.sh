#!/bin/sh

path="$HOME/.local/bin/dmenu"
shscripts="dmenu-launcher dmenu-wallpaper-selection dmenu-wifiprompt"

if [ "$1" == "uninstall" ]; then
        echo "Deleting dmenu scripts."

	for script in $shscripts; do
		rm -f $path/$script
	done

	if [ -z "$(ls -A $path)" ]; then
		rm -rf $path
	fi
	
	exit
fi

echo "Copying dwmblocks scripts."

mkdir -p $path

for script in $shscripts; do
	cp shell-scripts/$script $path/
done