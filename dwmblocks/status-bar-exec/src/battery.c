#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

#define BUF_SIZE 64

const char *pmcmd      = "optimus-manager --status";
const char *cappath    = "/sys/class/power_supply/BAT1/capacity";
const char *statuspath = "/sys/class/power_supply/BAT1/status";
const char *baticons[] = { CLR_1" ", CLR_3" ", CLR_2" ", CLR_2" ", CLR_2" " };
const char *models[]   = { "Unmanaged", "Integrated", "Hybrid", "Nvidia"};
const char *iconls[]   = { NULL, "intel", "deepin-graphics-driver-manager", "nvidia"};

static char*
uitoa(const unsigned int num)
{
	size_t digits = 0;
	char   *ret;

	for (unsigned int i = num; i > 0; i = i/10)
		digits++;
	if (!digits)
		digits++;

	if (!(ret = malloc((digits + 1) * sizeof(char))))
		logwrite("malloc() failed", "Returned NULL pointer", LOG_ERROR, "dwmblocks-kernel");

	snprintf(ret, digits + 1, "%u", num);
	return ret;
}

static unsigned int
getmode(void)
{
	FILE *ep;
	char buf[BUF_SIZE];
	int  ret = 0;

	if (!(ep = popen(pmcmd, "r"))) {
		logwrite("popen() failed for", pmcmd, LOG_WARN, "dwmblocks-battery");
		return 0;
	}

	while (fgets(buf, sizeof(buf), ep))
		if (strstr(buf, "Current"))
			break;

	if (strstr(buf, "integrated"))
		ret = 1;
	else if (strstr(buf, "hybrid"))
		ret = 2;
	else if (strstr(buf, "nvidia"))
		ret = 3;

	pclose(ep);
	return ret;
}

static void
execbutton(const unsigned int capacity, const char *status)
{
	char *env;
	char *body = NULL;
	unsigned int mode;

	
	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch (atoi(env)) {
	case 1:
		mode = getmode();
		strapp(&body, "Battery capacity: ");
		strapp(&body, uitoa(capacity));
		strapp(&body, "%\nBattery status: ");
		strapp(&body, status);
		strapp(&body, "\nOptimus manager: ");
		strapp(&body, models[mode]);

		notify("Power", body, (char*) iconls[mode], NOTIFY_URGENCY_NORMAL, 1);
		free(body);
		break;
		
	default:
		break;
	}
}

int
main(void)
{
	FILE *fp;
	char status[BUF_SIZE];
	unsigned int capacity;

	if (!(fp = fopen(cappath, "r")))
		logwrite("fopen() failed for", cappath, LOG_ERROR, "dwmblocks-battery");

	fscanf(fp, "%d", &capacity);
	fclose(fp);

	if (!(fp = fopen(statuspath, "r")))
		logwrite("fopen() failed for", statuspath, LOG_ERROR, "dwmblocks-battery");

	fgets(status, sizeof(status), fp);
	fclose(fp);

	trimtonewl(status);
	execbutton(capacity, status);

	if(!strcmp(status, "Charging")) {
		printf(CLR_3 BG_1" "NRM"\n");
		return EXIT_SUCCESS;
	}

	printf(BG_1"%s"NRM"\n", baticons[lround(capacity/25.0)]);
	return 0;
}
