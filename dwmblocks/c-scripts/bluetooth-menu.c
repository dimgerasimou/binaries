#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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
			
			execl("/usr/local/bin/xmenu", "xmenu", NULL);
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
}

void freearray(char **array, int size) {
	for (int i = 0; i < size; i++) {
		if (array[i] != NULL)
			free(array[i]);
	}
	if (array != NULL)
		free(array);
}

void connection(int state) {
	char **info;	
	FILE *ep;
	char buffer[256];
	int size = 0;
	char *temp;
	char cmd[64];
	char *pipestring;
	int opt;

	if (state) {
		strcpy(cmd, "bluetoothctl devices Connected");
	} else {
		strcpy(cmd, "bluetoothctl devices");
	}
	
	if ((ep = popen(cmd, "r")) == NULL) {
		perror("Failed to execute bluetoothctl cmd");
		exit(EXIT_FAILURE);
	}
	if ((info = malloc(1 * sizeof(char*))) == NULL)
		freearray(info, 1);
	while(fgets(buffer, sizeof(buffer), ep) != NULL) {
		size++;
		info = realloc(info, size * sizeof(char*));
		info[size - 1] = malloc(274 * sizeof(char));
		if (info[size - 1] == NULL)
			freearray(info, size);
		temp = buffer;
		temp += 7;
		strcpy(info[size - 1], temp);
		for (int i = strlen(info[size - 1]) - 1; i >= 0; i++) {
			if (info[size - 1][i] == '\n') {
				info[size - 1][i] = '\0';
				break;
			}
		}
	}
	pclose(ep);

	if (!size) {
		execl("/bin/dunstify", "dunstify", "          Bluetooth", "No available Bluetooth connections.", NULL);
		return;
	}
	pipestring = (char*) malloc(size * 160 * sizeof(char));
	pipestring[0] = '\0';

	for (int i = 0; i < size; i++) {
		temp = info[i];
		temp += 18;
		sprintf(buffer, "%s\t%d\n", temp, i);
		strcat(pipestring, buffer);
	}

	opt = printmenu(pipestring, strlen(pipestring));
	if (opt >= 0) {
		strncpy(buffer, info[opt], 17);
		buffer[17] = '\0';
		if (state) 
			strcpy(cmd, "bluetoothctl disconnect ");
		else 
			strcpy(cmd, "bluetoothctl connect ");
		strcat(cmd, buffer);
		ep = popen(cmd, "r");
		pclose(ep);
	}
	if (pipestring != NULL)	free(pipestring);
	freearray(info, size);	
}

int main(void) {
	char btmenu[] = "󰂳 Toggle power\t0\n󰂱 Connect\t1\n󰂲 Disconnect\t2";
	int btsize = sizeof(btmenu);
	switch (printmenu(btmenu, btsize)) {
		case 0:
			togglebt();
			break;
		case 1:
			connection(0);
			break;
		case 2:
			connection(1);
			break;
		default:
			break;
	}
	return 0;
}


