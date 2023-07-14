#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "colorscheme.h"


char *baticons[] = { CLR_1" "NRM, CLR_3" "NRM, CLR_2" "NRM, CLR_2" "NRM, CLR_2" "NRM };

void parsemode(char *input) {
	char mode[64];
	char *temp;

	mode[0] = '\0';
	if ((temp = strchr(input, ':')) == NULL) {
		perror("Could not get format");
		input[0] = '\0';
		return;
	}
	temp += 2;
	temp[0] += ('A' - 'a'); /* capitilize first letter */
	for (int i=0; temp[i] != '\0'; i++) {
		if (temp[i] == ' ' || temp[i] == '\n') {
			temp[i] = '\0';
			break;
		}
	}
	for(int i=0; i < (20 - strlen(temp)) / 2; i++) /* padding for space overfill */
		strcat(mode, " ");
	strcat(mode, temp);
	strcpy(input, mode);
}

void notifymode() {
	FILE *ep;
	char buffer[64];

	if ((ep = popen("optimus-manager --status", "r")) == NULL) {
		perror("Failed to execute optimus-manager");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, 64, ep) != NULL) {
		if (strstr(buffer, "Current") != NULL) {
			parsemode(buffer);
			break;
		}
	}
	if (strstr(buffer, "Integrated") != NULL) 
		execl("/bin/dunstify", "dunstify", "Optimus Manager mode", "Integrated", "--icon=intel", NULL);
	else if (strstr(buffer, "Integrated") != NULL) 
		execl("/bin/dunstify", "dunstify", "Optimus Manager mode", "Nvidia", "--icon=nvidia", NULL);
	else
		execl("/bin/dunstify", "dunstify", "Optimus Manager mode", "Optimus Manager not enabled.", NULL);
	pclose(ep);
}

void executebutton() {
	char *env = getenv("BLOCK_BUTTON");

	if (env != NULL && strcmp(env, "1") == 0) {
		switch (fork()) {
			case -1:perror("Failed in fork");
				exit(EXIT_FAILURE);

			case 0:	notifymode();
				exit(EXIT_SUCCESS);

			default:break;
		}
	}
}

int main(void) {
	FILE *fp;
	int capacity;
	char status[64];
	
	executebutton();

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
	
	if ((fp = fopen("/sys/class/power_supply/BAT1/capacity", "r")) == NULL) {
		perror("Failed to read \"/sys/class/power_supply/BAT1/capacity\"");
		return EXIT_FAILURE;
        }
       	fscanf(fp, "%d", &capacity);
        fclose(fp);

	printf("%s\n", baticons[lround(capacity/25.0)]);
	return 0;
}
