/*
 * This is an executable than can raise lower (+-5%) or mute the default source or default sink
 * of wireplumber and then signal dwmblocks to update the corresponding block.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>

#define SIGNALNO 10    /* signal number of dwmblocks volume block */
#define MAXVOL "1.2"   /* max volume to raise the sink 1.2 = 120% */

void freestruct(struct dirent** input, int n) {
	while (n--)
		free(input[n]);
	free(input);
}

int isnumber(char* string) {
	for (int i = 0; i < strlen(string); i++)
		if (string[i] > '9' || string[i] < '0')
			return 0;
	return 1;
}

int main(int argc, char *argv[]) {
	// Return if 2 arguments are not given.
	if (argc != 3) {
		perror("dwm-audiocontrol: Wrong argument format: Wrong number of arguments");
		exit(EXIT_FAILURE);
	}
		
	char output[64];
	strcpy(output, "/usr/bin/wpctl ");


	// Check for the validity of the arguments.
	if (strcmp(argv[2], "increase") == 0) {
		strcat(output, "set-volume ");
	} else if (strcmp(argv[2],"decrease") == 0) {
		strcat(output, "set-volume ");
	} else if (strcmp(argv[2], "toggle-mute") == 0) {
		strcat(output, "set-mute ");
	} else {
		perror("dwm-audiocontrol: Wrong argument format: Wrong operation type");
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[1], "source") == 0) {
		strcat(output, "@DEFAULT_AUDIO_SOURCE@ ");
	} else if (strcmp(argv[1], "sink") == 0) {
		strcat(output, "@DEFAULT_AUDIO_SINK@ ");
	} else {
		perror("dwm-audiocontrol: Wrong argument format: Wrong device type");
		exit(EXIT_FAILURE);
	}
 
	if (strcmp(argv[2], "increase") == 0)	
		strcat(output, "5%+ -l "MAXVOL);
	else if (strcmp(argv[2], "decrease") == 0)
		strcat(output, "5%-");
	else
		strcat(output, "toggle");

	FILE* fp;
	fp = popen(output, "r");
	pclose(fp);
	struct dirent** pidlist;

	int funcret = scandir("/proc", &pidlist, NULL, alphasort); 
	if (funcret == -1) {
		perror("dwm-audiocontrol: Failed to scand /proc directory");
		freestruct(pidlist, funcret);
		exit(EXIT_FAILURE);
	}
	
	char buffer[128];
	char filename[128];
	int pid = -1;

	for (int i = 0; i < funcret; i++) {
		if (isnumber(pidlist[i]->d_name)) {
			strcpy(filename, "/proc/");
			strcat(filename, pidlist[i]->d_name);
			strcat(filename, "/cmdline");
			fp = fopen(filename, "r");
			if (fp == NULL) {
				fclose(fp);
				continue;
			}
			fgets(buffer, sizeof(buffer), fp);
			fclose(fp);
			if (strstr(buffer, "dwmblocks") != NULL) { 
				pid = strtol(pidlist[i]->d_name, NULL, 10);
				break;
			}
		}
	}

	kill(pid, SIGRTMIN + SIGNALNO);
	freestruct(pidlist, funcret);
	return 0;
}

