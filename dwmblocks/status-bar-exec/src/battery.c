#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "../include/colorscheme.h"
#include "../include/common.h"

#define OPTIMUS

const char *pmcmd      = "optimus-manager --status";
const char *cappath    = "/sys/class/power_supply/BAT1/capacity";
const char *statuspath = "/sys/class/power_supply/BAT1/status";
const char *baticons[] = { CLR_1" "NRM, CLR_3" "NRM, CLR_2" "NRM, CLR_2" "NRM, CLR_2" "NRM };
const char *models[]   = { "Unmanaged", "Integrated", "Hybrid", "Nvidia"};
const char *iconls[]   = { NULL, "intel", "deepin-graphics-driver-manager", "nvidia"};

int
getmode()
{
	FILE *ep;
	char buffer[64];
	int  ret = 0;

	if (!(ep = popen(pmcmd, "r"))) {
		char log[512] = "Failed to execute ";

		strcat(log, pmcmd);
		log_string(log, "dwmblocks-battery");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, 64, ep))
		if (strstr(buffer, "Current"))
			break;

	if (strstr(buffer, "integrated")) {
		ret = 1;
	} else if (strstr(buffer, "hybrid")) {
		ret = 2;
	} else if (strstr(buffer, "nvidia")) {
		ret = 3;
	}

	pclose(ep);
	return ret;
}

void
modenotify(int capacity, char *status)
{
	char body[256];
	int mode = getmode();

	sprintf(body, "Battery capacity: %d%%\nBattery status: %s\nOptimus manager: ", capacity, status);
	strcat(body, models[mode]);
	notify("Power", body, (char*) iconls[mode], NOTIFY_URGENCY_LOW, 0);
}

void
executebutton(int capacity, char *status)
{
	char *env = getenv("BLOCK_BUTTON");

	if (env && strcmp(env, "1") == 0)
		modenotify(capacity, status);
}

int
main(void)
{
	FILE *fp;
	int  capacity;
	char status[64];

	if (!(fp = fopen(cappath, "r"))) {
		char log[512] = "Failed to open ";

		strcat(log, cappath);
		log_string(log, "dwmblocks-battery");
		exit(EXIT_FAILURE);
	}

	fscanf(fp, "%d", &capacity);
	fclose(fp);

	if (!(fp = fopen(statuspath, "r"))) {
		char log[512] = "Failed to open ";

		strcat(log, status);
		log_string(log, "dwmblocks-battery");
		exit(EXIT_FAILURE);
	}

	fgets(status, 64, fp);
	fclose(fp);

	sanitate_newline(status);

	executebutton(capacity, status);

	if(strcmp(status, "Charging") == 0) {
		printf(CLR_3 BG_1"  "NRM"\n");
		return EXIT_SUCCESS;
	}

	printf(BG_1"%s\n", baticons[lround(capacity/25.0)]);
	return 0;
}
