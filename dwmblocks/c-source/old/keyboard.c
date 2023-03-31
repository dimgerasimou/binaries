#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"

int main() {
	FILE* fp = popen("/bin/setxkbmap -query", "r");
	
	if (fp == NULL) {
		fprintf(stderr, "st-keyboard: Failed to get keyboard language.\n");
		return 1;
	}
	
	char input[128];
	char lang[32];
	char found = 0;

	while (fgets(input, sizeof(input), fp) != NULL) {
		if(strstr(input, "layout")) {
			found = 1;
			break;
		}
	}
	
	if (found)
		sscanf(input, "%s %s", lang, lang);
	else
		strcpy(lang, "Error!");
	
	printf(SFG SBG" ⌨ "NFG NBG" %s ^d^\n", lang);
	
	pclose(fp);
	return 0;
}	

