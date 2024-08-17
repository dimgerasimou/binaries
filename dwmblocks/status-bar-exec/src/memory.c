#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *htoppath[] = {"usr", "local", "bin", "st", NULL};
const char *htopargs[] = {"st", "-e", "sh", "-c", "htop", NULL};

static long
calculateused(void)
{
	FILE *fp;
	char buffer[128];
	char *ptr;
	long used=0, temp;

	if (!(fp = fopen("/proc/meminfo", "r")))
		logwrite("fopen() failed for", "/proc/meminfo", LOG_ERROR, "dwmblocks-memory");

	while (fgets(buffer, sizeof(buffer), fp)) {
		if (strstr(buffer, "MemTotal")) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used += temp;
			continue;
		}

		if (strstr(buffer, "MemFree")) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used -= temp;
			continue;
		}

		if (strstr(buffer, "Buffers")) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used -= temp;
			continue;
		}

		if (strstr(buffer, "Cached")) {
			ptr = (strchr(buffer, ':')) + 1;
			sscanf(ptr, "%ld", &temp);
			used -= temp;
			break;
		}
	}

	fclose(fp);
	return used;
}

static void
execbutton(void)
{
	char *env;
	char *path;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch(atoi(env)) {
	case 2:
		path = get_path((char**) htoppath, 1);
		forkexecv(path, (char**) htopargs, "dwmblocks-memory");
		free(path);
		break;
	
	default:
		break;
	}
}

int
main(void)
{
	execbutton();
	printf(CLR_3"   %.1lfGiB"NRM"\n", ((calculateused())/1024.0)/1024.0);
	return 0;
}
