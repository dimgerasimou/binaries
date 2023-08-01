#!/bin/sh

installations="dmenu dwm dwmblocks"

case $1 in
	-r|--uninstall)
		;;

	-h|--help)
		echo "Usage: install.sh [-r | --uninstall]"
		echo "default: install"
		exit 0;;
	
	-*|--*)
		echo "Unknown option $1"
		exit 1;;
	*)
		;;
esac


for install in $installations; do
	cd $install
	./install.sh "$1"
	cd ..
done
echo "Done!"
