/*
 * This is an executable than can raise lower (+-5%) or mute the default source or default sink
 * of wireplumber and then signal dwmblocks to update the corresponding block.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SIGNALNO 10    /* signal number of dwmblocks volume block */
#define MAXVOL "1.2"   /* max volume to raise the sink 1.2 = 120% */

int main(int argc, char *argv[]) {
	// Return if 2 arguments are not given.
	if (argc != 3) {
		perror("dwm-audiocontrol: Wrong argument format: Wrong number of arguments");
		exit(EXIT_FAILURE);
	}
		
	char output[64];
	strcpy(output, "/usr/bin/wpctl ");


	// Check for the validity of the arguments.
	if (strcmp(argv[2], "increase") == 0) {
		strcat(output, "set-volume ");
	} else if (strcmp(argv[2],"decrease") == 0) {
		strcat(output, "set-volume ");
	} else if (strcmp(argv[2], "toggle-mute") == 0) {
		strcat(output, "set-mute ");
	} else {
		perror("dwm-audiocontrol: Wrong argument format: Wrong operation type");
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[1], "source") == 0) {
		strcat(output, "@DEFAULT_AUDIO_SOURCE@ ");
	} else if (strcmp(argv[1], "sink") == 0) {
		strcat(output, "@DEFAULT_AUDIO_SINK@ ");
	} else {
		perror("dwm-audiocontrol: Wrong argument format: Wrong device type");
		exit(EXIT_FAILURE);
	}
 
	if (strcmp(argv[2], "increase") == 0)	
		strcat(output, "5%+ -l "MAXVOL);
	else if (strcmp(argv[2], "decrease") == 0)
		strcat(output, "5%-");
	else
		strcat(output, "toggle");

	FILE* ep;
	ep = popen(output, "w");
	pclose(ep);
	ep = popen("/usr/local/bin/dwmblocksctl -s volume", "w");
	pclose(ep);
	return 0;		
}
