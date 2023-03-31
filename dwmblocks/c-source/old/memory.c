#include <stdio.h>
#include <string.h>
#include "colorscheme.h"
int main() {
	FILE* fp = popen("/bin/free -m", "r");
	char buffer[256];
	char temp[64];
	int memused;

	if (fp == NULL) {
		fprintf(stderr, "st-memory: Can't exec free.\n");
		return 1;
	}

	fgets(buffer, sizeof(buffer), fp);
	fgets(buffer, sizeof(buffer), fp);
	pclose(fp);
	sscanf(buffer, "%s %d %d", temp, &memused, &memused);
	
	printf(SFG SBG"  "NFG NBG" %.1f GiB ^d^\n", (double)(memused) / 1024.0);

	return 0;
}
