#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *pmcmd      = "optimus-manager --status";
const char *cappath    = "/sys/class/power_supply/BAT1/capacity";
const char *statuspath = "/sys/class/power_supply/BAT1/status";
const char *baticons[] = { CLR_1" ", CLR_3" ", CLR_2" ", CLR_2" ", CLR_2" " };
const char *models[]   = { "Unmanaged", "Integrated", "Hybrid", "Nvidia"};
const char *iconls[]   = { NULL, "intel", "deepin-graphics-driver-manager", "nvidia"};

static int
get_mode(void)
{
	FILE *ep;
	char buffer[64];
	int  ret = 0;

	if (!(ep = popen(pmcmd, "r"))) {
		char log[512];

		sprintf(log, "popen() failed for: %s - %s", pmcmd, strerror(errno));
		log_string(log, "dwmblocks-battery");
		exit(errno);
	}

	while (fgets(buffer, sizeof(buffer), ep))
		if (strstr(buffer, "Current"))
			break;

	if (strstr(buffer, "integrated"))
		ret = 1;
	else if (strstr(buffer, "hybrid"))
		ret = 2;
	else if (strstr(buffer, "nvidia"))
		ret = 3;

	pclose(ep);
	return ret;
}

static void
execute_button(const int capacity, const char *status)
{
	char *env;
	
	env = getenv("BLOCK_BUTTON");

	if (env && !strcmp(env, "1")) {
		char body[256];
		int  mode = get_mode();

		sprintf(body, "Battery capacity: %d%%\nBattery status: %s\nOptimus manager: %s", capacity, status, models[mode]);
		notify("Power", body, (char*) iconls[mode], NOTIFY_URGENCY_NORMAL, 1);
	}
}

int
main(void)
{
	FILE *fp;
	int  capacity;
	char status[64];

	if (!(fp = fopen(cappath, "r"))) {
		char log[512];

		sprintf(log, "fopen() failed for: %s - %s", cappath, strerror(errno));
		log_string(log, "dwmblocks-battery");
		exit(errno);
	}

	fscanf(fp, "%d", &capacity);
	fclose(fp);

	if (!(fp = fopen(statuspath, "r"))) {
		char log[512];

		sprintf(log, "fopen() failed for: %s - %s", statuspath, strerror(errno));
		log_string(log, "dwmblocks-battery");
		exit(errno);
	}

	fgets(status, 64, fp);
	fclose(fp);

	while(sanitate_newline(status));

	execute_button(capacity, status);

	if(!strcmp(status, "Charging")) {
		printf(CLR_3 BG_1" "NRM"\n");
		return EXIT_SUCCESS;
	}

	printf(BG_1"%s"NRM"\n", baticons[lround(capacity/25.0)]);
	return 0;
}
