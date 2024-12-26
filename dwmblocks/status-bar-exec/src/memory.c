#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *HTOP_PATH[] = { "usr", "local", "bin", "st", NULL };
const char *HTOP_ARGS[] = { "st", "-e", "sh", "-c", "htop", NULL };

static long
get_used_memory(void)
{
	char  buffer[128];
	FILE *fp   = NULL;
	char *ptr  = NULL;
	long  used = 0;
	long  temp = 0;

	if (!(fp = fopen("/proc/meminfo", "r")))
		logwrite("fopen() failed for", "/proc/meminfo", LOG_FATAL, "dwmblocks-memory");

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
exec_block_button(void)
{
	char *env = NULL;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch(atoi(env)) {
	case 2:
	{
		char *path = get_path((char**) HTOP_PATH, 1);

		forkexecv(path, (char**) HTOP_ARGS, "dwmblocks-memory");
		free(path);
		break;
	}
	
	default:
		break;
	}
}

int
main(void)
{
	exec_block_button();
	printf(CLR_3"   %.1lfGiB"NRM"\n", ((get_used_memory())/1024.0)/1024.0);
	return 0;
}
