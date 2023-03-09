#include <stdio.h>
#include <string.h>
#include "colorscheme.h"

int main() {
	int capacity;
	char status[15];
	char icon[4];

	FILE * fp;
	fp = fopen("/sys/class/power_supply/BAT1/capacity", "r");
	
	if (fp == NULL) {
		puts("NULL");
		return 1;
	}

	fscanf(fp, "%d", &capacity);
	fclose(fp);

	fp = fopen("/sys/class/power_supply/BAT1/status", "r");

	if (fp == NULL) {
		puts("NULL");
		return 1;
	}

	fscanf(fp, "%s", status);
	fclose(fp);

	if (strcmp(status, "Charging")) {
		if (capacity <= 10)
			strcpy(icon, "");
		else if (capacity <= 20)
			strcpy(icon, "");
		else if (capacity <= 30)
			strcpy(icon, "");
		else if (capacity <= 40)
			strcpy(icon, "");
		else if (capacity <= 50)
			strcpy(icon, "");
		else if (capacity <= 60)
			strcpy(icon, "");
		else if (capacity <= 70)
			strcpy(icon, "");
		else if (capacity <= 80)
			strcpy(icon, "");
		else if (capacity <= 90)
			strcpy(icon, "");
		else
			strcpy(icon, "");
	} else {
		if (capacity <= 10)
			strcpy(icon, "");
		else if (capacity <= 20)
			strcpy(icon, "");
		else if (capacity <= 30)
			strcpy(icon, "");
		else if (capacity <= 40)
			strcpy(icon, "");
		else if (capacity <= 50)
			strcpy(icon, "");
		else if (capacity <= 60)
			strcpy(icon, "");
		else if (capacity <= 70)
			strcpy(icon, "");
		else if (capacity <= 80)
			strcpy(icon, "");
		else if (capacity <= 90)
			strcpy(icon, "");
		else
			strcpy(icon, "");
	}

	printf(SBG SFG" %s "NBG NFG" %d%% ^d^\n", icon, capacity);

	return 0;
}
