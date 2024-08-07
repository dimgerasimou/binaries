#include <libnotify/notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/colorscheme.h"
#include "../include/common.h"

int getaudioprop(int *muted) {
	FILE *ep;
	char buffer[64];
	int num;

	if ((ep = popen("wpctl get-volume @DEFAULT_AUDIO_SINK@", "r")) == NULL) {
		perror("Failed to execute wpctl");
		exit(EXIT_FAILURE);
	}

	fgets(buffer, 64, ep);
	if (strstr(buffer, "MUTED") != NULL)
		*muted = 1;
	else
		*muted = 0;
	if (strlen(buffer) > 3)
		num = (buffer[8] - '0') * 100 + (buffer[10] - '0') * 10 + (buffer[11] - '0');
	else
		num = 0;
	pclose(ep);
	return num;
}

void notifyproperties(int volume, int muted) {
	FILE *ep;
	char buffer[64];
	int micvol;
	int micmuted;

	if ((ep = popen("wpctl get-volume @DEFAULT_AUDIO_SOURCE@", "r")) == NULL) {
		perror("Failed to execute wpctl");
		exit(EXIT_FAILURE);
	}

	fgets(buffer, 64, ep);
	if (strstr(buffer, "MUTED") != NULL)
		micmuted = 1;
	else
		micmuted = 0;
	if (strlen(buffer) > 3)
		micvol = (buffer[8] - '0') * 100 + (buffer[10] - '0') * 10 + (buffer[11] - '0');
	else
		micvol = 0;
	pclose(ep);
	
	sprintf(buffer, " Volume: %3d%%, Muted?: %d\n Volume: %3d%%, Muted?: %d\n", volume, muted, micvol, micmuted);
	notify("Wireplumber", buffer, "audio-headphones", NOTIFY_URGENCY_LOW, 1);
}

void executebutton(int volume, int muted) {
	char *env;
	char *path;

	const char *easyeffectsargs[] = {"easyeffects", NULL};
	const char *increasevol[] = {"audiocontrol", "sink", "increase", NULL};
	const char *decreasevol[] = {"audiocontrol", "sink", "decrease", NULL};
	const char *toggmutevol[] = {"audiocontrol", "sink", "mute", NULL};
	const char *audioctrlpath[]= { "$HOME", ".local", "bin", "dwm", "audiocontrol", NULL};

	if ((env = getenv("BLOCK_BUTTON"))== NULL)
		return;

	switch (env[0] - '0') {
		case 1:
			path = get_path((char**) audioctrlpath, 1);
			forkexecv(path, (char**) toggmutevol, "dwmblocks-volume");
			free(path);
			break;
			
		case 2:
			forkexecv("/usr/bin/easyeffects", (char**) easyeffectsargs, "dwmblocks-volume");
			break;
		case 3:
			notifyproperties(volume, muted); 
			break;
		case 4:
			path = get_path((char**) audioctrlpath, 1);
			forkexecv(path, (char**) increasevol, "dwmblocks-volume");
			free(path);
			break;
		case 5:
			path = get_path((char**) audioctrlpath, 1);
			forkexecv(path, (char**) decreasevol, "dwmblocks-volume");
			free(path);
			break;
		default:
			break;
	}
}


int main(void) {
	int volume;
	int muted;
	char icon[8];

	volume = getaudioprop(&muted);
	executebutton(volume, muted);
	
	if (muted) {
		strcpy(icon, "");
	} else if (volume > 66) {
		strcpy(icon, "");
	} else if (volume > 33) {
		strcpy(icon, "");
	} else {
		strcpy(icon, "");
	}

	printf(CLR_2"%s %d%%"NRM"\n", icon, volume);
	return 0;
}
	

