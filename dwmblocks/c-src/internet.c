#include <libnotify/notification.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#include "colorscheme.h"
#include "common.h"

struct devprop {
	char name[8];
	char type[16];
	char hwaddress[18];
	char state[32];
	char connection[33];
	char ip4address[16];
	char ip4gateway[16];
	char ip6address[32];
};

const char *iconarray[] = {"x", "tdenetworkmanager", "wifi-radar", "tdenetworkmanager"};
const char *nmtuipath = "/usr/local/bin/st";
const char *nmtuiargs[] = {"st", "-e", "nmtui", NULL};
const char dmenuscriptpath[] = "/.local/bin/dmenu/dmenu-wifiprompt";
const char *dmenuargs[] = {"dmenu-wifiprompt", NULL};

void getnames(char *edev, char *wdev) {
	DIR *dp;
	struct dirent *ep;

	if ((dp = opendir("/sys/class/net/")) == NULL) {
		perror("Failed to list /sys/class/net/ directory");
		exit(EXIT_FAILURE);
	}
	
	while ((ep = readdir (dp)) != NULL) {
		if (strchr(ep->d_name, 'e') != NULL) 	
			strncpy(edev, ep->d_name, 8);
		if (strchr(ep->d_name, 'w') != NULL) 
			strncpy(wdev, ep->d_name, 8);
        }
	(void) closedir (dp);
}

int geticon(char *icon) {
	FILE *ep;
	char edev[32];
	char wdev[32];
	char buffer[128];
	int state = 0;
	
	getnames(edev, wdev);
	if ((ep = popen("nmcli device", "r")) == NULL)
		return 0;
	while (fgets(buffer, 128, ep) != NULL) {
		if (strstr(buffer, edev) != NULL) {
			if(strstr(buffer, "connected") != NULL) {
				if (strstr(buffer, "disconnected") == NULL)
					state += 1;
			}
		}
		if (strstr(buffer, wdev) != NULL) {
			if(strstr(buffer, "connected") != NULL) {
				if (strstr(buffer, "disconnected") == NULL)
					state += 2;
			}
		}

	}
	if (!state) 
		strcpy(icon, CLR_9" "NRM);
	else if (state == 2)
		strcpy(icon, CLR_6"󰤨 "NRM);
	else if (state == 1 || state == 3)
		strcpy(icon, CLR_6"󰈁 "NRM);
	else
	 	strcpy(icon, "NULL");
	return state;
}

void getdeviceattributes(char *name, struct devprop *deviceprop) {
	FILE *ep;
	char buffer[256];
	char *string;
	char *ptr;

	if ((ep = popen("nmcli device show", "r")) == NULL)
		return;

	while (fgets(buffer, 256, ep) != NULL) {
		if (strstr(buffer, name) != 0) {
			strncpy(deviceprop->name, name, 8);
			while (fgets(buffer, 256, ep) != NULL) {
				if (strstr(buffer, "TYPE") != 0) {
					string = buffer + 40;
					if ((ptr = strchr(string, '\n')) != NULL)
						ptr[0] = '\0';
					string[0] = string[0] - 'a' + 'A';
					strncpy(deviceprop->type, string, 16);
				}else if (strstr(buffer, "HWADDR") != 0) {
					string = buffer + 40;
					string[17] = '\0';
					if ((ptr = strchr(string, '\n')) != NULL)
						ptr[0] = '\0';
					strncpy(deviceprop->hwaddress, string, 18);
				} else if (strstr(buffer, "STATE") != 0) {
					string = buffer + 40;
					if ((ptr = strchr(string, ' ')) != NULL || (ptr = strchr(string, '\n')) != NULL)
						ptr[0] = '\0';
					strncpy(deviceprop->state, string, 4);
				} else if (strstr(buffer, "CONNECTION") != 0) {
					string = buffer + 40;
					if ((ptr = strchr(string, '\n')) != NULL)
						ptr[0] = '\0';
					strncpy(deviceprop->connection, string, 33);
				} else if (strstr(buffer, "IP4.ADDRESS") != 0) {
					string = buffer + 40;
					if ((ptr = strchr(string, '/')) != NULL || (ptr = strchr(string, '\n')) != NULL) 
						ptr[0] = '\0';
					strncpy(deviceprop->ip4address, string, 16);
				} else if (strstr(buffer, "IP4.GATEWAY") != 0) {
					string = buffer + 40;
					if ((ptr = strchr(string, '/')) != NULL || (ptr = strchr(string, '\n')) != NULL)
						ptr[0] = '\0';
					strncpy(deviceprop->ip4gateway, string, 16);
				} else if (strstr(buffer, "IP6.ADDRESS") != 0) {
					string = buffer + 40;
					if ((ptr = strchr(string, '/')) != NULL || (ptr = strchr(string, '\n')) != NULL)
						ptr[0] = '\0';
					strncpy(deviceprop->ip6address, string, 32);
				} else if (strstr(buffer, "IP6.GATEWAY") != 0) {
					return;
				}
			}
		}
	}
}

void netproperties(int state) {
	struct devprop deviceprop;
	char edev[8];
	char wdev[8];
	char output[512];
	char tempoutput[256];

	getnames(edev, wdev);
	switch (state) {
		case 0:
			strcpy(output, "Network Disconnected");
			break;
		case 1:
			getdeviceattributes(edev, &deviceprop);
			sprintf(output, "%s\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			break;
		case 2:
			getdeviceattributes(wdev, &deviceprop);
			sprintf(output, "%s\nSSID: %s - %s%%\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.connection, deviceprop.state, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			break;
		case 3:
			getdeviceattributes(edev, &deviceprop);
			sprintf(output, "%s\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			getdeviceattributes(wdev, &deviceprop);
			sprintf(tempoutput, "\n%s\nSSID: %s - %s%%\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.connection, deviceprop.state, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			strcat(output,tempoutput);
			break;
		default:
			break;
	}
	notify("Netowrk Manager", output, (char *) iconarray[state], NOTIFY_URGENCY_LOW, 0);
}

void execdmenu() {
	char *env;
	switch(fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:
			env = getenv("HOME");
			strcat(env, dmenuscriptpath);
			forkexecv(env, (char**) dmenuargs);
			exit(EXIT_SUCCESS);
		default:
			break;
	}
}

void checkexec(int state) {
	char *env;

	if ((env = getenv("BLOCK_BUTTON"))== NULL)
		return;

	switch (env[0] - '0') {
		case 1:	netproperties(state);
			break;

		case 2: forkexecv((char*) nmtuipath, (char**) nmtuiargs);
			break;

		case 3: execdmenu();
			break;
		default:
			break;
	}
}

int main(void) {
	char icon[32];
	int state = geticon(icon);
	checkexec(state);
	printf("  "BG_1" %s\n", icon);
	return 0;
}
