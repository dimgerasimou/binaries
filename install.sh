#1/bin/sh

if [ "$1" == "uninstall" ]; then
        echo "Run with no arguments to install all scripts."
        echo "Options:"
        echo "\t uninstall - Uninsltalls all scripts and removes the empty associated directories."
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