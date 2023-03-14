#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		perror("dwm-mediacontrol:Wrong argument count");
		exit(EXIT_FAILURE);
	}

	char *args[6];
	args[0] = "dbus-send";
	args[1] = "--print-reply";
	args[2] = "--dest=org.mpris.MediaPlayer2.spotify";
	args[3] = "/org/mpris/MediaPlayer2";
	args[4] = malloc(64 * sizeof(char));
	args[5] = NULL;
	strcpy(args[4], "");

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
		free(args[4]);
		exit(EXIT_FAILURE);
	}
	free(args[4]);
	return 0;
}
