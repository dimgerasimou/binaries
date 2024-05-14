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

int
is_powered()
{
	FILE *ep;
	char buffer[128];

	if (!(ep = popen(bt_show, "r"))) {
		char log[512] = "Failed to execute ";

		strcat(log, bt_show);
		log_string(log, "dwmblocks-bluetooth");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, 128, ep)) {
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

int
audio_device(char *address)
{
	FILE *ep;
	char cmd[128];
	char buf[128];

	strcpy(cmd, bt_info);
	strcat(cmd, " ");
	strcat(cmd, address);

	if (!(ep = popen(cmd, "r"))) {
		char log[512] = "Failed to execute ";

		strcat(log, cmd);
		log_string(log, "dwmblocks-bluetooth");
		exit(EXIT_FAILURE);
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

int
has_audio()
{
	FILE *ep;
	char buffer[256];
	char mac[18];
	char *temp;

	if (!(ep = popen(bt_con, "r"))) {
		char log[512] = "Failed to execute ";

		strcat(log, bt_show);
		log_string(log, "dwmblocks-bluetooth");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, sizeof(buffer), ep)) {
		if (strstr(buffer, "Device")) {
			temp = buffer;
			temp += 7;

			strncpy(mac, temp, 17);
			sanitate_newline(temp);

			if (audio_device(mac)) {
				pclose(ep);
				return 1;
			}
		}
	}

	pclose(ep);
	return 0;
}

void
execute_block()
{
	char *button;
	char *path = NULL;

	if ((button = getenv("BLOCK_BUTTON")) == NULL)
		return;

	switch (button[0] - '0') {
		case 1:
			path = get_path((char**) menu_path, 1);
			forkexecv(path, (char**) menu_args, "dwmblocks-bluetooth");
			break;

		case 2:
			path = get_path((char**) tui_path, 1);
			forkexecv(path, (char**) tui_args, "dwmblocks-bluetooth");
			break;

		default:
			break;
	}

	if (path)
		free(path);
}

int
main(void)
{
	int icon_arg = 0;

	execute_block();

	icon_arg += is_powered();
	if (icon_arg)
		icon_arg += has_audio();

	printf(CLR_4 BG_1" %s " NRM "\n", icon_ls[icon_arg]);

	return EXIT_SUCCESS;
}
