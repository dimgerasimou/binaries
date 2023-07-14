#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "colorscheme.h"

char *baticons[] = { CLR_1" "NRM, CLR_3" "NRM, CLR_2" "NRM, CLR_2" "NRM, CLR_2" "NRM };

int fileinpath(char *name) {
	char *env;
	char execpath[256];
	int subpathcount = 1;
	int i = 0, j = 0;

	env = getenv("PATH");

	for (int k = 0; k < strlen(env); k++) {
		if (env[k] == ':')
			subpathcount++;
	}

	char paths[subpathcount][128];
	
	for (int k = 0; k < strlen(env); k++) {
		if (j == 128 || i == subpathcount) {
			perror("fileinpath: indexes out of bounds");
			exit(EXIT_FAILURE);
		}
		if (env[k] == ':') {
			i++;
			j = 0;
			continue;
		}
		paths[i][j] = env[k];
		j++;
	}
	
	for (i = 0; i < subpathcount; i++) {
		strcpy(execpath, paths[i]);
		strcat(execpath, name);
		if (access(execpath, F_OK) == 0)
			return 1;
	}
	return 0;
}

void getoptimusmode(char *mode, char *icon) {
	FILE *ep;
	char buffer[64];

	if ((ep = popen("optimus-manager --status", "r")) == NULL) {
		perror("Failed to execute optimus-manager");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, 64, ep) != NULL) {
		if (strstr(buffer, "Current") != NULL) {
			break;
		}
	}

	if (strstr(buffer, "integrated") != NULL) {
		strcpy(mode, "Optimus Manager: Integrated");
		strcpy(icon, "--icon=intel");
	} else if (strstr(buffer, "hybrid") != NULL) {
		strcpy(mode, "Optimus Manager: Hybrid");
		strcpy(icon, "--icon=deepin-graphics-driver-manager");
	} else if (strstr(buffer, "nvidia") != NULL) {
		strcpy(mode, "Optimus Manager: Nvidia");
		strcpy(icon, "--icon=nvidia");
	} else {
		strcpy(mode, "Optimus Manager: Unmanaged");
	}
	pclose(ep);
}

void getenvymode(char *mode, char *icon) {
	FILE *ep;
	char buffer[64];

	if ((ep = popen("envycontrol -q", "r")) == NULL) {
		perror("Failed to execute optimus-manager");
		exit(EXIT_FAILURE);
	}

	fgets(buffer, 64, ep);
	pclose(ep);

	if (strstr(buffer, "integrated") != NULL) {
		strcpy(mode, "Envy Control: Integrated");
		strcpy(icon, "--icon=intel");
	} else if (strstr(buffer, "hybrid") != NULL) {
		strcpy(mode, "Envy Control: Hybrid");
		strcpy(icon, "--icon=deepin-graphics-driver-manager");
	} else if (strstr(buffer, "nvidia") != NULL) {
		strcpy(mode, "Envy Control: Nvidia");
		strcpy(icon, "--icon=nvidia");
	} else {
		strcpy(mode, "Envy Control: Unmanaged");
	}
}

void notify(int capacity) {
	char stringout[256];
	char mode[64];
	char icon[64];

	strcpy(icon, "");
	sprintf(stringout, "Battery capacity: %d%%\n", capacity);

	if (fileinpath("/optimus-manager"))
		getoptimusmode(mode, icon);
	else if (fileinpath("/envycontrol"))
		getenvymode(mode, icon);
	else
		strcpy(mode, "Optimus: Not managed");

	strcat(stringout, mode);

	execl("/bin/dunstify", "dunstify", "Power", stringout, icon, NULL);
}

void executebutton(int capacity) {
	char *env = getenv("BLOCK_BUTTON");

	if (env != NULL && strcmp(env, "1") == 0) {
		switch (fork()) {
			case -1:perror("Failed in fork");
				exit(EXIT_FAILURE);

			case 0:	notify(capacity);
				exit(EXIT_SUCCESS);

			default:break;
		}
	}
}

int main(void) {
	FILE *fp;
	int capacity;
	char status[64];

	if ((fp = fopen("/sys/class/power_supply/BAT1/capacity", "r")) == NULL) {
		perror("Failed to read \"/sys/class/power_supply/BAT1/capacity\"");
		return EXIT_FAILURE;
        }
       	fscanf(fp, "%d", &capacity);
        fclose(fp);
	
	executebutton(capacity);

	if ((fp = fopen("/sys/class/power_supply/BAT1/status", "r")) == NULL) {
		perror("Failed to read \"/sys/class/power_supply/BAT1/status\"");
		return EXIT_FAILURE;
        }
        fscanf(fp, "%s", status);
	fclose(fp);
	
	if(strcmp(status, "Charging") == 0) {
		printf(CLR_3""NRM"\n");
		return EXIT_SUCCESS;
	}
	
	printf("%s\n", baticons[lround(capacity/25.0)]);
	return 0;
}
