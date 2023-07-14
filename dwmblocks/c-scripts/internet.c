#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include "colorscheme.h"

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

char *execnmtui[] = {"st", "-e", "nmtui", NULL};
char dmenuscriptpath[] = "/.local/bin/dmenu/dmenu-wifiprompt";

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

void getpaths(char *epath, char *wpath) {
	char temp[128];
	char edev[8];
	char wdev[8];

	getnames(edev, wdev);
	strcpy(temp, "/sys/class/net/");
	strcat(temp, edev);
	strcat(temp, "/operstate");
	strcpy(epath, temp);
	
	strcpy(temp, "/sys/class/net/");
	strcat(temp, wdev);
	strcat(temp, "/operstate");
	strcpy(wpath, temp);
}

int geticon(char *icon) {
	FILE *fp;
	char epath[128];
	char wpath[128];
	char buffer[128];
	int state = 0;
	
	getpaths(epath, wpath);
	if ((fp = fopen(wpath, "r")) != NULL) {
		fscanf(fp, "%s", buffer);
		if (strcmp(buffer, "up") == 0) {
			strcpy(icon, CLR_6"  󰤨  "NRM);
			state += 2;
		}
		fclose(fp);
	}

	if ((fp = fopen(epath, "r")) != NULL) {
		fscanf(fp, "%s", buffer);
		if (strcmp(buffer, "up") == 0) {
			strcpy(icon, CLR_6"  󰈁  "NRM);
			state += 1;
		}
		fclose(fp);
	}
	
	if (!state) 
		strcpy(icon, CLR_9"    "NRM);
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
	char headermessage[64] = "Network Manager";

	char edev[8];
	char wdev[8];
	struct devprop deviceprop;
	char output[512];
	char tempoutput[256];
	char header[128] = "";
	int maxw = 39;

	getnames(edev, wdev);
	switch (state) {
		case 0:	strcpy(output, "Network Disconnected");
			maxw=20;
			break;

		case 1:	getdeviceattributes(edev, &deviceprop);
			sprintf(output, "%s\n\nIPv4 Address: %s\nIPv6 Address: %s\n\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			break;

		case 2:	getdeviceattributes(wdev, &deviceprop);
			puts(wdev);
			sprintf(output, "%s\nSSID: %s - %s%%\n\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.connection, deviceprop.state, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			if (strlen(deviceprop.connection + 6) > maxw)
				maxw = strlen(deviceprop.connection + 6);
			break;

		case 3:	getdeviceattributes(edev, &deviceprop);
			sprintf(output, "%s\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			
			getdeviceattributes(wdev, &deviceprop);
			sprintf(tempoutput, "\n%s\nSSID: %s - %s%%\nIPv4 Address: %s\nIPv6 Address: %s\nIPv4 Gateway: %s\n",
			        deviceprop.type, deviceprop.connection, deviceprop.state, deviceprop.ip4address, deviceprop.ip6address, deviceprop.ip4gateway);
			if (strlen(deviceprop.connection + 6) > maxw)
				maxw = strlen(deviceprop.connection + 6);
			strcat(output,tempoutput);
			break;
		default: break;
	}

	for (int i=0; i < (maxw - strlen(headermessage)) / 2; i++)
		strcat(header, " ");
	strcat(header, headermessage);	

	execl("/bin/dunstify", "dunstify", header, output, "--icon=tdenetworkmanager", NULL);
}
void notifyproperties(int state) {
	switch(fork()) {
		case -1:perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:	netproperties(state);
			exit(EXIT_SUCCESS);
		default:
	}
}

void execst() {
	switch(fork()) {
		case -1:perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:	setsid();
			execv("/usr/local/bin/st", execnmtui);
			exit(EXIT_SUCCESS);
		default:
	}	
}

void execdmenu() {
	char *env;
	switch(fork()) {
		case -1:perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:	switch(fork()) {
				case -1:perror("Failed in forking");	
					exit(EXIT_FAILURE);
				case 0:	execl("/bin/dunstify", "dunstify", "       Network Manager", "Probing wifi access points...", NULL);
					exit(EXIT_SUCCESS);
				default:
			}
			env = getenv("HOME");
			strcat(env, dmenuscriptpath);
			execl(env, "dmenu-wifiprompt", NULL);
			exit(EXIT_SUCCESS);
		default:break;
	}
}

void checkexec(int state) {
	char *env;

	if ((env = getenv("BLOCK_BUTTON"))== NULL)
		return;

	switch (env[0] - '0') {
		case 1:	notifyproperties(state);
			break;

		case 2: execst();
			break;

		case 3: execdmenu();
			break;
		default:break;
	}
}

int main(void) {
	char icon[32];
	int state = geticon(icon);
	checkexec(state);
	puts(icon);
	return 0;
}
