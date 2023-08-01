#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"
#include <sys/stat.h>
#include <sys/wait.h>

#define MPRIS_PATH "/.local/state/dwm"

int getmpriscount() {
	FILE *ep;
	int counter = 0;
	char buffer[256];

	if ((ep = popen("dbus-send --print-reply --dest=org.freedesktop.DBus  /org/freedesktop/DBus org.freedesktop.DBus.ListNames", "r")) == NULL) {
		perror("Failed to run dbus-send command");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, sizeof(buffer), ep) != NULL) {
		if (strstr(buffer, "mpris") != 0) 
			counter++;
	}
	pclose(ep);
	return counter;
}

void parseclient(char *client, char *input) {
	char *index;
	char name[64];
	char clid[64];
	
	strcpy(client, "");
	if (strstr(input, "mpris") == NULL)
		return;
                
	index = strstr(input, "MediaPlayer2");
        index += 13;

        strcpy(clid, index);
        clid[strlen(clid) - 2] = '\0';

	strcpy(name, clid);
	name[0] += 'A' - 'a';
	for (int i=0; i < strlen(name); i++) {
		if (name[i] == '.' || name[i] == ' ') {
			name[i] = '\0';
			break;
		}
	}
	sprintf(client, "%s\t%s", name, clid);
}

void getmenu(char **clientlist, int clientcount, char *client) {
	int writepipe[2], readpipe[2];
	char menu[clientcount * 256];

	menu[0] = '\0';
	for (int i=0; i < clientcount; i++) {
		strcat(menu, clientlist[i]);
		strcat(menu, "\n");
	}

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
                perror("Failed to initialize pipes");
		for (int i = 0; i < clientcount; i++)
			free(clientlist[i]);
		free(clientlist);
                exit(EXIT_FAILURE);
        }
	switch (fork()) {
                case -1:
                        perror("Failed in forking");
			for (int i = 0; i < clientcount; i++)
				free(clientlist[i]);
			free(clientlist);
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
                        write(writepipe[1], menu, strlen(menu) * sizeof(char));
                        close(writepipe[1]);
                        wait(NULL);
                        read(readpipe[0], client, 256 * sizeof(char));
                        close(readpipe[0]);
        }
	for (int i=0; i < strlen(client); i++)
		if (client[i] == '\n')
			client[i] = '\0';
}

void getclient(int mpriscount, char *client) {
	FILE *ep;
        char buffer[512];
	char **clientlist;
	int iterator = 0;

        if ((ep = popen("dbus-send --print-reply --dest=org.freedesktop.DBus  /org/freedesktop/DBus org.freedesktop.DBus.ListNames", "r")) == NULL) {
		perror("Failed to run dbus-send command");
		exit(EXIT_FAILURE);
	}
	clientlist = (char**) malloc(mpriscount * sizeof(char*));
	for (int i = 0; i < mpriscount; i++)
		clientlist[i] = (char*) malloc(256 * sizeof(char));

	while (fgets(buffer, sizeof(buffer), ep) != NULL) {
                if (strstr(buffer, "mpris") != NULL) {
			if (iterator < mpriscount) {
                        	parseclient(clientlist[iterator], buffer);
				iterator++;
			}
                }
        }
	pclose(ep);
	
	getmenu(clientlist, mpriscount, client);

	for (int i = 0; i < mpriscount; i++)
		free(clientlist[i]);
	free(clientlist);
}

void selectplayer(int mpriscount) {
	FILE *fp;
	struct stat st = {0};
	char path[256];
	char *env;
	char client[256];

	if(strcmp(env = getenv("HOME"), "") == 0) {
		perror("Failed to get HOME env");
		exit(EXIT_FAILURE);
	}

	strcpy(path, env);
	strcat(path, MPRIS_PATH);
	if (stat(path, &st) == -1)
                mkdir(path, 0700);
	strcat(path, "/mprisclient");

	getclient(mpriscount, client);
	
	fp = fopen(path, "w");
	fprintf(fp, "%s\n", client);
	fclose(fp);
}

void execbutton(int mpriscount) {
	char *env = getenv("BLOCK_BUTTON");

	if (env != NULL && env[0] == '1' && mpriscount > 1) {
		switch (fork()) {
			case -1:perror("Failed to fork");
				exit(EXIT_FAILURE);

			case 0: selectplayer(mpriscount);
				exit(EXIT_SUCCESS);

			default: break;
		}
	}
}

int main(void) {
	int count = getmpriscount();
	if (count == 0)
		return 0;

	execbutton(count);
	
	printf(CLR_4"󰎈"NRM"\n");
	return 0;
}
