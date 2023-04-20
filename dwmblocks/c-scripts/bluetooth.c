#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"

struct device {
	char mac[18];
	char name[64];
};

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

void clearnline(char *string) {
	for (int i = 0; i < strlen(string); i++) {
		if (string[i] == '\n') {
			string[i] = '\0';
			return;
		}
	}
}

struct device *getcon(int *size) {
	FILE *ep;
	struct device *list;
	int iter = 0;
	char buf[256];
	char *temp;

	if ((ep = popen("bluetoothctl devices Connected", "r")) == NULL) {
		perror("Failed in exec");
		exit(EXIT_FAILURE);
	}

	list = (struct device*) malloc(sizeof(struct device));
	if (list == NULL) {
		perror("Failed to allocate memory");
		exit(EXIT_FAILURE);
	}
	while  (fgets(buf, sizeof(buf), ep) != NULL) {
		if (strstr(buf, "Device") != NULL) {
			iter++;
			list = (struct device*) realloc(list, iter * sizeof(struct device*));
			if (list == NULL) {
				perror("Failed to allocate memory");
				exit(EXIT_FAILURE);
			}
			temp = buf;
			temp += 7;
			strncpy(list[iter-1].mac, temp, 17);
			temp += 17;
			strcpy(list[iter-1].name, temp);
			clearnline(list[iter-1].mac);
			clearnline(list[iter-1].name);
		}
	}
	pclose(ep);
	*size = iter;
	return list;
}

int getaudio(char *address) {
	FILE *ep;
	char cmd[128];
	char buf[128];

	strcpy(cmd, "bluetoothctl info ");
	strcat(cmd, address);
	if ((ep = popen(cmd, "r")) == NULL) {
		perror("Failed to exec bluetoothctl");
		return 0;
	}
	while(fgets(buf, sizeof(buf), ep) != NULL) {
		if (strstr(buf, "Audio Sink") != NULL) {
			pclose(ep);
			return 1;
		}
	}
	pclose(ep);
	return 0;
}

int audiodevice() {
	int size;
	int isaudio = 0;
	struct device *list = getcon(&size);
	
	for (int i = 0; i < size; i++) {
		printf("Mac:%s, Name:%s\n", list[i].mac, list[i].name);
		if (getaudio(list[i].mac))
			isaudio = 1;
	}
	
	if (list != NULL)
		free(list);
	return isaudio;
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
		if (!audiodevice())
			printf(CLR_4 "󰂯" NRM "\n");
		else
			printf(CLR_4 "󰥰" NRM "\n");
	}
	return EXIT_SUCCESS;
}
