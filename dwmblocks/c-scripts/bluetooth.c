#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"

int getopstate() {
	FILE *ep;
	char buf[128];
	
	if ((ep = popen("bluetoothctl show", "r")) == NULL) {
		perror("Failed to execute bluetoothctl command");
		exit(EXIT_FAILURE);
	}

	while (fgets(buf, 128, ep) != NULL) {
		if (strstr(buf, "Powered") != NULL) {
			pclose(ep);
		if (strstr(buf, "yes") != NULL)
			return 1;
		return 0;
		}
	}
	pclose(ep);
	return 0;
}

void execst() {
	char *term = "st";
	char *termpath = "/usr/local/bin/st";

	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:
			setsid();
			execl(termpath, term, "-e", "bluetuith", NULL);
			exit(EXIT_SUCCESS);
		default:
			break;
	}
}

void execmenu() {
	char path[256];
	char *env;
	env = getenv("HOME");
	strcpy(path, env);
	strcat(path, "/.local/bin/dwmblocks/bluetooth-menu");
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:
			execl(path, "bluetooth-menu", NULL);
			exit(EXIT_SUCCESS);
		default:
			break;
	}
}

void execblock() {
	char *button;

	if ((button = getenv("BLOCK_BUTTON")) == NULL)
		return;

	switch (button[0] - '0') {
		case 1:
			execmenu();
			break;
		case 2:
			execst();
			break;
		default:
			break;
	}
}

int main(void) {
	execblock();

	if (!getopstate()) {
		printf(CLR_4 "󰂲" NRM "\n");
	} else {
		printf(CLR_4 "󰂯" NRM "\n");
	}
	return EXIT_SUCCESS;
}
