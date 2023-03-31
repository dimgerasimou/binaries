#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"

#define SOURCE "--default-source"

int main() {	
	char input[32];
	char iconmic[6];
	char iconvol[5];
	int volume;

	FILE* fp;
	fp = popen("/usr/bin/pamixer --get-volume", "r");
	if (fp == NULL) {
		perror("dwmstatusbar-volume: Can't execute pamixer command");
		exit(EXIT_FAILURE);
	}
	fscanf(fp, "%d", &volume);
	pclose(fp);
	
    fp = popen("/usr/bin/pamixer --get-mute", "r");
	if (fp == NULL) {
		perror("dwmstatusbar-volume: Can't execute pamixer command");
		exit(EXIT_FAILURE);
	}
	fscanf(fp, "%s", input);
	pclose(fp);
	
	if (strcmp(input, "true") == 0)
		strcpy(iconvol, "🔇");
	else if (volume > 70)
		strcpy(iconvol, "🔊");
	else if (volume < 30)
		strcpy(iconvol, "🔈");
	else
		strcpy(iconvol, "🔉");

	// Microphone section
	fp = popen("/usr/bin/pamixer "SOURCE" --get-mute", "r");
	if (fp == NULL) {
		perror("dwmstatusbar-volume: Can't execute pamixer command");
		exit(EXIT_FAILURE);
	}
	fgets(input, sizeof(input), fp);
	pclose(fp);
	strcpy(iconmic, "");
	if (strcmp(input, "false") == 0)
		strcpy(iconmic, " 🎤");

	printf(SFG SBG"%s %s "NBG NFG" %d%% ^d^\n", iconmic, iconvol, volume);
	return 0;
}
