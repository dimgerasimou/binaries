#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "colorscheme.h"


char *baticons[] = { CLR_1" "NRM, CLR_3" "NRM, CLR_2" "NRM, CLR_2" "NRM, CLR_2" "NRM };

void notifymode() {
	FILE *ep;
	char mode[64];
	char buffer[64];
	char *temp;
	int padding;

	if ((ep = popen("optimus-manager --status", "r")) == NULL) {
		perror("Failed to execute optimus-manager");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, 64, ep) != NULL) {
		if (strstr(buffer, "Current") != NULL) {
			mode[0] = '\0';
			if ((temp = strchr(buffer, ':')) == NULL) {
				perror("Could not get format");
				return;
			}

			temp += 2;
			temp[0] = temp[0] - 'a' + 'A';
			for (int i=0; temp[i] != '\0'; i++) {
				if (temp[i] == ' ' || temp[i] == '\n') {
					temp[i] = '\0';
					break;
				}
			}

			padding = (20 - strlen(temp)) / 2;
			for(int i=0; i < padding; i++)
                		strcat(mode, " ");

			strcat(mode, temp);
			break;
		}
	}
	pclose(ep);
	execl("/bin/dunstify", "dunstify", "Optimus Manager mode", mode, NULL);
}
void executebutton() {
	char *env = getenv("BLOCK_BUTTON");
	int pid;

	if (env != NULL && strcmp(env, "1") == 0) {
		pid = fork();
		if (!pid) {
			notifymode();
			exit(EXIT_SUCCESS);
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
