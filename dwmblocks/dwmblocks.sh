#!/bin/sh

path="$HOME/.local/bin/dwmblocks"
cscripts="date time kernel battery"
shscripts="confirm internet keyboard memory power powermenu spacer spacer-0 volume"

if [ "$1" == "uninstall" ]; then
	echo "Deleting dwmblocks scripts."

	for script in $cscripts; do
		rm -f $path/$script
	done

	for script in $shscripts; do
		rm -f $path/$script
	done

	if [ -z "$(ls -A $path)" ]; then
		rm -rf $path
	fi
	
	exit
fi

echo "Copying dwmblocks scripts."


cd c-scripts

gcc -o loadresources loadresources.c

./loadresources

rm -f loadresources

mkdir -p $path

for script in $cscripts; do
	gcc -o $script $script.c -Os -Wall -lm
	mv $script $path/
done

cd ..

for script in $shscripts; do
	cp shell-scripts/$script $path/
done
