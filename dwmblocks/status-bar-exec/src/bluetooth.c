#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *tui_path[]  = { "usr", "local", "bin", "st", NULL};
const char *tui_args[]  = {"st", "-e", "bluetuith", NULL};
const char *menu_path[] = {"$HOME", ".local", "bin", "dwmblocks", "bluetooth-menu"};
const char *menu_args[] = {"bluetooth-menu", NULL};
const char *icon_ls[]   = {"󰂲", "󰂯", "󰥰"};
const char *bt_show     = "bluetoothctl show";
const char *bt_info     = "bluetoothctl info";
const char *bt_con      = "bluetoothctl devices Connected";

static int
is_powered(void)
{
	FILE *ep;
	char buffer[128];

	if (!(ep = popen(bt_show, "r"))) {
		char log[512];

		sprintf(log, "popen() failed for: %s - %s", bt_show, strerror(errno));
		log_string(log, "dwmblocks-bluetooth");
		exit(errno);
	}

	while (fgets(buffer, sizeof(buffer), ep)) {
		if (strstr(buffer, "Powered")) {
			pclose(ep);

			if (strstr(buffer, "yes"))
				return 1;

			return 0;
		}
	}

	pclose(ep);
	return 0;
}

static int
audio_device(char *address)
{
	FILE *ep;
	char cmd[128];
	char buf[128];

	sprintf(cmd, "%s %s", bt_info, address);

	if (!(ep = popen(cmd, "r"))) {
		char log[512];

		sprintf(log, "popen() failed for: %s - %s", cmd, strerror(errno));
		log_string(log, "dwmblocks-bluetooth");
		exit(errno);
	}

	while(fgets(buf, sizeof(buf), ep)) {
		if (strstr(buf, "audio-headset")) {
			pclose(ep);
			return 1;
		}
	}

	pclose(ep);
	return 0;
}

static int
has_audio(void)
{
	FILE *ep;
	char buffer[256];
	char mac[18];
	char *ptr;

	if (!(ep = popen(bt_con, "r"))) {
		char log[512];

		sprintf(log, "popen() failed for: %s - %s", bt_con, strerror(errno));
		log_string(log, "dwmblocks-bluetooth");
		exit(errno);
	}

	while (fgets(buffer, sizeof(buffer), ep)) {
		if (strstr(buffer, "Device")) {
			ptr = buffer;
			ptr += 7;
			sanitate_newline(ptr);

			strncpy(mac, ptr, 17);

			if (audio_device(mac)) {
				pclose(ep);
				return 1;
			}
		}
	}

	pclose(ep);
	return 0;
}

static void
execute_block(void)
{
	char *button;
	char *path;

	if (!(button = getenv("BLOCK_BUTTON")))
		return;

	switch (button[0] - '0') {
	case 1:
		path = get_path((char**) menu_path, 1);
		forkexecv(path, (char**) menu_args, "dwmblocks-bluetooth");
		free(path);
		break;

	case 2:
		path = get_path((char**) tui_path, 1);
		forkexecv(path, (char**) tui_args, "dwmblocks-bluetooth");
		free(path);
		break;

	default:
		break;
	}
}

int
main(void)
{
	int icon_arg;

	execute_block();

	icon_arg = is_powered();
	icon_arg = icon_arg && has_audio() ? 2 : icon_arg;

	printf(CLR_4 BG_1" %s " NRM "\n", icon_ls[icon_arg]);

	return EXIT_SUCCESS;
}
