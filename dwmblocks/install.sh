#!/bin/sh

uninstall() {
	make uninstall -C status-bar-exec
}

install() {
	make install -C status-bar-exec
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
