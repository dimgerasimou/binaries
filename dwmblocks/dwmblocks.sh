#!/bin/sh

path="$HOME/.local/bin/dwmblocks"
cscripts="date time kernel battery internet power memory volume spacer"
xscripts="keyboard"
shscripts=""

if [ "$1" == "uninstall" ]; then
	echo "Deleting dwmblocks scripts."

	for script in $cscripts; do
		rm -f $path/$script
	done

	for script in $shscripts; do
		rm -f $path/$script
	done
	
	for script in $xscripts; do
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

for script in $xscripts; do
	gcc -I/usr/include -o $script $script.c -Os -Wall -lX11 -lxkbfile
	mv $script $path/
done

cd ..

for script in $shscripts; do
	cp shell-scripts/$script $path/
done
