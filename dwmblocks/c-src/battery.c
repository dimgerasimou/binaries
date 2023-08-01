#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "colorscheme.h"
#include "common.h"

#define OPTIMUS

char *baticons[] = { CLR_1" "NRM, CLR_3" "NRM, CLR_2" "NRM, CLR_2" "NRM, CLR_2" "NRM };

#if defined OPTIMUS
void getmode(char *mode, char *icon) {
	FILE *ep;
	char buffer[64];

	if ((ep = popen("optimus-manager --status", "r")) == NULL) {
		perror("Failed to execute optimus-manager");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, 64, ep) != NULL)
		if (strstr(buffer, "Current") != NULL)
			break;

	if (strstr(buffer, "integrated") != NULL) {
		strcpy(mode, "Optimus Manager: Integrated");
		strcpy(icon, "intel");
	} else if (strstr(buffer, "hybrid") != NULL) {
		strcpy(mode, "Optimus Manager: Hybrid");
		strcpy(icon, "deepin-graphics-driver-manager");
	} else if (strstr(buffer, "nvidia") != NULL) {
		strcpy(mode, "Optimus Manager: Nvidia");
		strcpy(icon, "nvidia");
	} else {
		strcpy(mode, "Optimus Manager: Unmanaged");
	}
	pclose(ep);
}
#elif defined ENVY
void getmode(char *mode, char *icon) {
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
		strcpy(icon, "intel");
	} else if (strstr(buffer, "hybrid") != NULL) {
		strcpy(mode, "Envy Control: Hybrid");
		strcpy(icon, "deepin-graphics-driver-manager");
	} else if (strstr(buffer, "nvidia") != NULL) {
		strcpy(mode, "Envy Control: Nvidia");
		strcpy(icon, "nvidia");
	} else {
		strcpy(mode, "Envy Control: Unmanaged");
	}
}
#else
void getmode(char *mode, char *icon) {
	strcpy(mode, "Optimus: Not managed");
}
#endif

void modenotify(int capacity, char *status) {
	char body[256];
	char icon[64];
	char mode[64]; 
	
	strcpy(icon, "");
	sprintf(body, "Battery capacity: %d%%\nBattery status: %s\n", capacity, status);

	getmode(mode, icon);
	strcat(body, mode);
	notify("Power", body, icon, NOTIFY_URGENCY_LOW, 0);
}

void executebutton(int capacity, char *status) {
	char *env = getenv("BLOCK_BUTTON");

	if (env != NULL && strcmp(env, "1") == 0)
		modenotify(capacity, status);
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

	if ((fp = fopen("/sys/class/power_supply/BAT1/status", "r")) == NULL) {
		perror("Failed to read \"/sys/class/power_supply/BAT1/status\"");
		return EXIT_FAILURE;
        }
        fgets(status, 64, fp);
	fclose(fp);
	for (int i = 0; i < strlen(status); i++) {
		if (status[i] == '\n')
			status[i] = '\0';
	}
		
	executebutton(capacity, status);

	if(strcmp(status, "Charging") == 0) {
		printf(CLR_3" "NRM"\n");
		return EXIT_SUCCESS;
	}
	
	printf(BG_1" %s\n", baticons[lround(capacity/25.0)]);
	return 0;
}
