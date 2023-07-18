#!/bin/sh

if (($# != 0 )) && ( [[ "$1" != "uninstall" ]] || (($# > 1)) ); then
	echo ""
        echo "Run with no arguments to install all scripts."
        echo "Options:"
        echo "        uninstall - Uninsltalls all scripts and removes the empty associated directories."
	echo ""
        exit
fi

cd dmenu
./dmenu.sh "$1"
cd ..

cd dwm
./dwm.sh "$1"
cd ..

cd dwmblocks
./dwmblocks.sh "$1"
cd ..
