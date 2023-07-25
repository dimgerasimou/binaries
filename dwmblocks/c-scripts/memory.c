#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "colorscheme.h"
#include "common.h"

const char *htoppath = "/usr/local/bin/st";
const char *htopargs[] = {"st", "-e", "sh", "-c", "htop", NULL};

long calculateused() {
	FILE *fp;
	char buffer[128];
	char *ptr;
	long used=0, temp;

	if ((fp = fopen("/proc/meminfo", "r")) == NULL)
		return 0;

	while (fgets(buffer, 128, fp) != NULL) {
		if (strstr(buffer, "MemTotal") != 0) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used += temp;
			continue;
		}
		if (strstr(buffer, "MemFree") != 0) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used -= temp;
			continue;
		}
		if (strstr(buffer, "Buffers") != 0) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used -= temp;
			continue;
		}
		if (strstr(buffer, "Cached") != 0) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used -= temp;
			break;
		}
	}
	fclose(fp);
	return used;

}


void executebutton() {
	char *env = getenv("BLOCK_BUTTON");
	if (env != NULL && env[0] == '2') 
		forkexecv((char*) htoppath, (char**)htopargs);
}

int main(void) {
	executebutton();
	printf(CLR_3"   %.1lfGiB"NRM"\n", ((calculateused())/1024.0)/1024.0);
	return 0;
}
