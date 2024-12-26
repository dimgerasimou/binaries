#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

#define BUFFER_SIZE 64

const char *OPTIMUS_COMMAND = "optimus-manager --status";
const char *CAPACITY_PATH   = "/sys/class/power_supply/BAT1/capacity";
const char *STATUS_PATH     = "/sys/class/power_supply/BAT1/status";
const char *BATICON_LIST[]  = { CLR_1" ", CLR_3" ", CLR_2" ", CLR_2" ", CLR_2" " };
const char *MODE_LIST[]     = { "Unmanaged", "Integrated", "Hybrid", "Nvidia" };
const char *ICON_LIST[]     = { NULL, "intel", "deepin-graphics-driver-manager", "nvidia" };

static char*
uitoa(const unsigned int num)
{
	char    *ret     = NULL;
	size_t  digits   = 0;
	int     sn_check = 0;

	for (unsigned int i = num; i > 0; i = i/10)
		digits++;
	if (!digits)
		digits++;

	if (!(ret = malloc((digits + 1) * sizeof(char)))) {
		logwrite("malloc() failed. Returned NULL pointer", NULL, LOG_ERROR, "dwmblocks-battery");
		return NULL;
	}

	sn_check = snprintf(ret, digits + 1, "%u", num);

	if (sn_check < 0 || sn_check > (int) digits) {
		logwrite("snprintf() failed. Buffer overflow", NULL, LOG_ERROR, "dwmblocks-battery");
		return NULL;
	}

	return ret;
}

static unsigned int
get_mode(void)
{
	char buf[BUFFER_SIZE];
	FILE         *ep  = NULL;
	unsigned int  ret = 0;

	if (!(ep = popen(OPTIMUS_COMMAND, "r"))) {
		logwrite("popen() failed for", OPTIMUS_COMMAND, LOG_WARN, "dwmblocks-battery");
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
exec_block_button(const unsigned int capacity, const char *status)
{
	char *env = NULL;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch (atoi(env)) {
	case 1:
	{
		char         *body = NULL;
		char         *cap  = uitoa(capacity);
		unsigned int  mode = get_mode();

		if (!cap)
			break;

		strapp(&body, "Battery capacity: ");
		strapp(&body, cap);
		strapp(&body, "%\nBattery status:   ");
		strapp(&body, status);
		strapp(&body, "\nOptimus manager:  ");
		strapp(&body, MODE_LIST[mode]);

		notify("Power", body, (char*) ICON_LIST[mode], NOTIFY_URGENCY_NORMAL, 1);

		free(body);
		free(cap);
		break;
	}
	default:
		break;
	}
}

static unsigned int
get_capacity(void)
{
	FILE         *fp  = NULL;
	unsigned int  ret = 0;

	if (!(fp = fopen(CAPACITY_PATH, "r"))) {
		logwrite("fopen() failed for file", CAPACITY_PATH, LOG_ERROR, "dwmblocks-battery");
		exit(errno);
	}

	fscanf(fp, "%u", &ret);
	fclose(fp);

	return ret;
}

static char*
get_status(void)
{
	char buffer[BUFFER_SIZE];
	FILE *fp  = NULL;
	char *ret = NULL;

	if (!(fp = fopen(STATUS_PATH, "r"))) {
		logwrite("fopen() failed for file", STATUS_PATH, LOG_ERROR, "dwmblocks-battery");
		exit(errno);
	}

	fgets(buffer, sizeof(buffer), fp);
	fclose(fp);

	trimtonewl(buffer);

	ret = strdup(buffer);
	
	return ret;
}

int
main(void)
{
	char         *status   = get_status();
	unsigned int  capacity = get_capacity();

	exec_block_button(capacity, status);

	if(!strcmp(status, "Charging")) {
		printf(CLR_3 BG_1" "NRM"\n");
		free(status);
		return EXIT_SUCCESS;
	}

	printf(BG_1"%s"NRM"\n", BATICON_LIST[lround(capacity/25.0)]);

	free(status);
	return 0;
}
