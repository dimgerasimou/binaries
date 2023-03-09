#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"

int main() {	
	char input[32];
	char iconmic[6];
	char iconvol[5];
	char temp[5];

	FILE* fp;
	fp = popen("/usr/bin/wpctl get-volume @DEFAULT_AUDIO_SINK@", "r");
	if (fp == NULL) {
		perror("dwmstatusbar-volume: Can't execute wpctl command");
		exit(EXIT_FAILURE);
	}
	fgets(input, sizeof(input), fp);
	pclose(fp);
	
	strcpy(temp, "");
	int counter = 0;
	for (int i = 0; input[i] != '\0'; i++) {
		if (input[i] >= '0' && input[i] <= '9')
			temp[counter++] = input[i];
	}
	temp[counter] = '\0';

	int volume = atoi(temp);

	if (strstr(input, "MUTED") != 0)
		strcpy(iconvol, "🔇");
	else if (volume > 70)
		strcpy(iconvol, "🔊");
	else if (volume < 30)
		strcpy(iconvol, "🔈");
	else
		strcpy(iconvol, "🔉");

	// Microphone section
	fp = popen("/usr/bin/wpctl get-volume @DEFAULT_AUDIO_SOURCE@", "r");
	if (fp == NULL) {
		perror("dwmstatusbar-volume: Can't execute wpctl command");
		exit(EXIT_FAILURE);
	}
	fgets(input, sizeof(input), fp);
	pclose(fp);
	strcpy(iconmic, "");
	if (strstr(input, "MUTED") == 0)
		strcpy(iconmic, " 🎤");

	printf(SFG SBG"%s %s "NBG NFG" %d%% ^d^\n", iconmic, iconvol, volume);
	return 0;
}
