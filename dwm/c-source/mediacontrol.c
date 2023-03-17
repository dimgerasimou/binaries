/* Script that gets the first mpris client, or one selected from a file for playback control
 * with one of the following arguments:
 * 	toggle - toggles play - pause
 * 	stop   - stops playback
 *	next   - moves to next track
 *	prev   - moves to previous track
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CLIENT_PATH "/.local/state/dwm"

int checkclient(char *client) {
	FILE * ep;
	char input[512];
	
	if ((ep = popen("dbus-send --print-reply --dest=org.freedesktop.DBus  /org/freedesktop/DBus org.freedesktop.DBus.ListNames", "r")) == NULL)
		return 0;

	while (fgets(input, 512, ep) != NULL) {
		if (strstr(input, client) != NULL) {
			pclose(ep);
			return 1;
		}
	}

	pclose(ep);
	return 0;
}

void generateclient(char *client, char *filepath) {
	FILE *ep;
	FILE *fp;
	char *index;
	char input[512];
	
	if ((ep = popen("dbus-send --print-reply --dest=org.freedesktop.DBus  /org/freedesktop/DBus org.freedesktop.DBus.ListNames", "r")) == NULL)
		return;

	while (fgets(input, 512, ep) != NULL) {
		if (strstr(input, "mpris") != NULL) {
			index = strstr(input, "MediaPlayer2");
			index += 13;
			
			strcpy(client, index);
			client[strlen(client) - 2] = '\0';	
			
			fp = fopen(filepath, "w");
			fprintf(fp, "%s\n", client);
			
			fclose(fp);
			pclose(ep);
			return;
		}
	}

	pclose(ep);
}

char *getclient() {
	FILE *fp;
	struct stat st = {0};
	char *path   = (char*) malloc(128 * sizeof(char));
	char *client = (char*) malloc(64 * sizeof(char));
	char filepath[128];

	strcpy(client, "");
	
	if(strcmp(path = getenv("HOME"), "") == 0) 
		return client;

	strcat(path, CLIENT_PATH);
	
	if (stat(path, &st) == -1)
		mkdir(path, 0700);

	strcpy(filepath, path);
	strcat(filepath, "/mprisclient");
	if ((fp = fopen(filepath, "r")) == NULL) {
		generateclient(client, filepath);
	} else {
		fscanf(fp, "%s", client);
		fclose(fp);
		if(!checkclient(client)) {
			remove(filepath);
			generateclient(client, filepath);
		}
	}

	return client;
}
	
int main(int argc, char *argv[]) {
	char *args[6];
	char *client;

	if (argc != 2) {
		perror("dwm-mediacontrol:Wrong argument count");
		exit(EXIT_FAILURE);
	}
	
	if (strcmp((client = getclient()), "") == 0) {
		perror("dwm-mediacontrol:No mpris client running");
		free(client);
		exit(EXIT_FAILURE);
	}

	args[0] = "dbus-send";
	args[1] = "--print-reply";
	args[2] = malloc(128 * sizeof(char));
	args[3] = "/org/mpris/MediaPlayer2";
	args[4] = malloc(64 * sizeof(char));
	args[5] = NULL;
	
	strcpy(args[2], "--dest=org.mpris.MediaPlayer2.");
	strcat(args[2], client);
	strcpy(args[4], "");
	free(client);

	if(strcmp(argv[1], "toggle") == 0) {
		strcpy(args[4], "org.mpris.MediaPlayer2.Player.PlayPause");
	} else if(strcmp(argv[1], "stop") == 0) {
		strcpy(args[4], "org.mpris.MediaPlayer2.Player.Pause");
	} else if(strcmp(argv[1], "next") == 0) {
		strcpy(args[4], "org.mpris.MediaPlayer2.Player.Next");
	}else if(strcmp(argv[1], "prev") == 0) {
		strcpy(args[4], "org.mpris.MediaPlayer2.Player.Previous");
	}

	if (!(strcmp(args[4], "") == 0)) {
		execv("/usr/bin/dbus-send", args);
	} else {
		perror("dwm-mediacontrol:Wrong argument values");
		free(args[2]);
		free(args[4]);
		exit(EXIT_FAILURE);
	}
	free(args[2]);
	free(args[4]);
	return 0;
}
