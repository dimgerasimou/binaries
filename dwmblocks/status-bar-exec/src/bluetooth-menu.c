#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

struct device {
	char mac[18];
	char name[64];
};

int printmenu(char *menu, int menusize) {
	int option = -1;
	int writepipe[2], readpipe[2];
	char buffer[64] = "";

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - xmenu */
			close(writepipe[1]);
			close(readpipe[0]);
			dup2(writepipe[0], STDIN_FILENO);
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);
			
			execl("/usr/bin/xmenu", "xmenu", NULL);
			exit(EXIT_SUCCESS);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			write(writepipe[1], menu, menusize);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
	}
	if (buffer[0] != '\0')
		sscanf(buffer, "%d", &option);
	return option;
}

int getopstate() {
	FILE *ep;
	char buf[128];
	
	if ((ep = popen("bluetoothctl show", "r")) == NULL) {
		perror("Failed to execute bluetoothctl command");
		exit(EXIT_FAILURE);
	}
	while (fgets(buf, 128, ep) != NULL) {
		if (strstr(buf, "Powered") != NULL) {
			pclose(ep);
		if (strstr(buf, "yes") != NULL)
			return 1;
		return 0;
		}
	}
	pclose(ep);
	return 0;
}

void togglebt() {
	char cmd[64];
	FILE *ep;
	
	strcpy(cmd, "bluetoothctl power ");
	if (getopstate())
		strcat(cmd,"off");
	else
		strcat(cmd,"on");
	ep = popen(cmd, "r");
	pclose(ep);
	ep = popen("/usr/local/bin/dwmblocksctl -s bluetooth", "r");
	pclose(ep);
}

void clearnline(char *string) {
	for (int i = 0; i < (int) strlen(string); i++) {
		if (string[i] == '\n') {
			string[i] = '\0';
			return;
		}
	}
}

char *sttostr(struct device *list, int count) {
	char *string = (char *) malloc(count * 64 * sizeof(char));
	char temp[64];

	if (string == NULL)
		return string;
	string[0] = '\0';
	for (int i = 0; i < count; i++) {
		sprintf(temp, "%s\t%d\n", list[i].name, i);
		strcat(string, temp);
	}
	return string;
}

struct device *getcon(int *size, char *cmd) {
	FILE *ep;
	struct device *list;
	int iter = 0;
	char buf[256];
	char *temp;

	if ((ep = popen(cmd, "r")) == NULL) {
		perror("Failed in exec");
		exit(EXIT_FAILURE);
	}

	list = (struct device*) malloc(sizeof(struct device));
	if (list == NULL) {
		perror("Failed to allocate memory");
		exit(EXIT_FAILURE);
	}
	while  (fgets(buf, sizeof(buf), ep) != NULL) {
		if (strstr(buf, "Device") != NULL) {
			iter++;
			list = (struct device*) realloc(list, iter * sizeof(struct device*));
			if (list == NULL) {
				perror("Failed to allocate memory");
				exit(EXIT_FAILURE);
			}
			temp = buf;
			temp += 7;
			strncpy(list[iter-1].mac, temp, 17);
			temp += 17;
			strcpy(list[iter-1].name, temp);
			clearnline(list[iter-1].mac);
			clearnline(list[iter-1].name);
		}
	}
	pclose(ep);
	*size = iter;
	return list;
}

void menuandexec(struct device *list, int count, char *cmdin) {
	FILE *ep;
	char *pipestr;
	char cmd[64];
	int option;
	
	if((pipestr = sttostr(list, count)) == NULL) {
		perror("Failed in allocating memory");
		return;
	}
	option = printmenu(pipestr, strlen(pipestr));
	if (pipestr != NULL)
		free(pipestr);
	
	strcpy(cmd, cmdin);
	strcat(cmd, list[option].mac);
	ep = popen(cmd, "r");
	pclose(ep);
}

struct device *getinact(int *inasize, int actsize, int allsize, struct device *actlist, struct device *alllist) {
	int iter = 0;
	int maxsize = allsize - actsize;
	int found;
	
	struct device *list = (struct device*) malloc(maxsize * sizeof(struct device));
	if (list == NULL)
		return list;
	
	for (int i = 0; i < allsize; i++) {
		if (iter == maxsize)
			break;
		found = 0;
		for (int j = 0; j < actsize; j++) {
			if (strcmp(alllist[i].mac, actlist[i].mac) == 0)
				found = 1;
		}
		if (!found) {
			strcpy(list[iter].mac, alllist[i].mac);
			strcpy(list[iter].name, alllist[i].name);
			iter++;
		}
	}
	if (iter < maxsize) {
		list = (struct device*) realloc(list, iter * sizeof(struct device));
	}
	*inasize = iter;
	return list;
}

void connect() {
	int actsize;
	int allsize;
	int inasize;
	struct device *actlist = getcon(&actsize, "bluetoothctl devices Connected");
	struct device *alllist = getcon(&allsize, "bluetoothctl devices");
	struct device *inalist;

	if (!allsize) {
		execl("/bin/dunstify", "dunstify", "Bluetooth", "No devices available.", NULL);
		if (actlist != NULL)
			free(actlist);
		if (alllist != NULL)
			free(alllist);
		return;
	}
	inalist = getinact(&inasize, actsize, allsize, actlist, alllist);
	if (actlist != NULL)
		free(actlist);
	if (alllist != NULL)
		free(alllist);
	if (inalist == NULL)
		return;
	menuandexec(inalist, inasize, "bluetoothctl connect ");
	if (inalist != NULL)
		free(inalist);
}

void disconnect() {
	int size;
	struct device *list = getcon(&size, "bluetoothctl devices Connected");
	
	if (!size) {
		execl("/bin/dunstify", "dunstify", "Bluetooth", "No devices available.", NULL);
		if (list != NULL)
			free(list);
		return;
	}
	menuandexec(list, size, "bluetoothctl disconnect ");	
	if (list != NULL)
		free(list);
}

int main(void) {
	char btmenu[] = "󰂳 Toggle power\t0\n󰂱 Connect\t1\n󰂲 Disconnect\t2";
	int btsize = sizeof(btmenu);
	switch (printmenu(btmenu, btsize)) {
		case 0:
			togglebt();
			break;
		case 1:
			connect();
			break;
		case 2:
			disconnect();
			break;
		default:
			break;
	}
	return 0;
}
