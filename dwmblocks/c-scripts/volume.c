#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "colorscheme.h"

char *execpmixer[] = {"st", "-e", "sh", "-c", "pulsemixer", NULL};

int getaudioprop(int *muted) {
	FILE *ep;
	char buffer[64];
	int num;

	if ((ep = popen("wpctl get-volume @DEFAULT_AUDIO_SINK@", "r")) == NULL) {
		perror("Failed to execute wpctl");
		exit(EXIT_FAILURE);
	}

	fgets(buffer, 64, ep);
	if (strstr(buffer, "MUTED") != NULL)
		*muted = 1;
	else
		*muted = 0;
	if (strlen(buffer) > 3)
		num = (buffer[8] - '0') * 100 + (buffer[10] - '0') * 10 + (buffer[11] - '0');
	else
		num = 0;
	pclose(ep);
	return num;
}

void execst() {
        switch(fork()) {
                case -1:perror("Failed in forking");
                        exit(EXIT_FAILURE);
                case 0: setsid();
                        execv("/usr/local/bin/st", execpmixer);
                        exit(EXIT_SUCCESS);
                default:
        }
}

void execeasyeffects() {
        switch(fork()) {
                case -1:perror("Failed in forking");
                        exit(EXIT_FAILURE);
                case 0: setsid();
                        execl("/bin/easyeffects", "easyeffects", NULL);
                        exit(EXIT_SUCCESS);
                default:
        }
}

void notifyproperties(int volume, int muted) {
	FILE *ep;
	char buffer[64];
	int micvol;
	int micmuted;

	if ((ep = popen("wpctl get-volume @DEFAULT_AUDIO_SOURCE@", "r")) == NULL) {
		perror("Failed to execute wpctl");
		exit(EXIT_FAILURE);
	}

	fgets(buffer, 64, ep);
	if (strstr(buffer, "MUTED") != NULL)
		micmuted = 1;
	else
		micmuted = 0;
	if (strlen(buffer) > 3)
		micvol = (buffer[8] - '0') * 100 + (buffer[10] - '0') * 10 + (buffer[11] - '0');
	else
		micvol = 0;
	pclose(ep);
	
	sprintf(buffer, " Volume: %3d%%, Muted?: %d\n Volume: %3d%%, Muted?: %d\n", volume, muted, micvol, micmuted);
	switch(fork()) {
                case -1:perror("Failed in forking");
                        exit(EXIT_FAILURE);
                case 0: execl("/bin/dunstify", "dunstify", "   Wireplumber", buffer, NULL);
                        exit(EXIT_SUCCESS);
                default:
        }
}

void executebutton(int volume, int muted) {
        char *env;

        if ((env = getenv("BLOCK_BUTTON"))== NULL)
                return;

        switch (env[0] - '0') {
                case 1: notifyproperties(volume, muted); 
                        break;

                case 2: execst();
                        break;

                case 3: execeasyeffects();
                        break;
                default:break;
        }
}


int main(void) {
	int volume;
	int muted;
	char icon[8];

	volume = getaudioprop(&muted);
	executebutton(volume, muted);
	
	if (muted) {
		strcpy(icon, "");
	} else if (volume > 66) {
		strcpy(icon, "");
	} else if (volume > 33) {
		strcpy(icon, "");
	} else {
		strcpy(icon, "");
	}

	printf(CLR_2"%s %d%%"NRM"\n", icon, volume);
	return 0;
}
	

