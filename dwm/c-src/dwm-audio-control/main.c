/*
 * This is an executable than can raise lower (+-5%), set the volume on a given parameter or mute the default source or default sink
 * of wireplumber and then signal dwmblocks to update the corresponding block.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXVOL "1.2"   /* max volume to raise the sink 1.2 = 120% */

void fork_execv(char *path, char *args[]) {
	pid_t pID;

	pID = fork();
	if (pID < 0) {
		perror("Fork failed");
		exit(1);
	} else if (pID == 0) {
		setsid();
		execv(path, args);
		perror("Fork execv failed");
		exit(1);
	}
}

void freenpointers(char **pointer, int index) {
	for (int i = index; i < 7; i++) {
		free(pointer[i]);
		pointer[i] = NULL;
	}
}

void freearray(char **pointer) {
	if(pointer != NULL) {
		for (int i = 0; i < 7; i++)
			if (pointer[i] != NULL)
				free(pointer[i]);
		free(pointer);
	}
}

int parse_arguments(int argc, char *argv[], char **wpctlargs) {
	if (argc < 3) {
		fprintf(stderr, "Argument count is wrong.\n");
		exit(1);
	}

	if (strcmp(argv[1], "source") == 0)
		strcpy(wpctlargs[2], "@DEFAULT_AUDIO_SOURCE@");
	else if (strcmp(argv[1], "sink") == 0)
		strcpy(wpctlargs[2], "@DEFAULT_AUDIO_SINK@");
	else
		return 0;

	if (strcmp(argv[2], "increase") == 0 && argc == 3){
		strcpy(wpctlargs[1], "set-volume");
		strcpy(wpctlargs[3], "5%+");
		strcpy(wpctlargs[4], "-l");
		strcpy(wpctlargs[5], MAXVOL);
		freenpointers(wpctlargs, 6);
	} else if (strcmp(argv[2], "decrease") == 0 && argc == 3) {
		strcpy(wpctlargs[1], "set-volume");
		strcpy(wpctlargs[3], "5%-");
		freenpointers(wpctlargs, 4);
	} else if (strcmp(argv[2], "mute") == 0 && argc == 3) {
		strcpy(wpctlargs[1], "set-mute");
		strcpy(wpctlargs[3], "toggle");
		freenpointers(wpctlargs, 4);
	} else if (strcmp(argv[2], "set") == 0 && argc == 4) {
		strcpy(wpctlargs[1], "set-volume");
		strcpy(wpctlargs[3], argv[3]);
		freenpointers(wpctlargs, 4);
	} else {
		return 0;
	}
	strcpy(wpctlargs[0], "wpctl");
	return 1;
}

int main(int argc, char *argv[]) {
	char **wpctlargs;
	const char *dctlargs[] = {"dwmblocksctl", "-s", "volume", NULL};

	if ((wpctlargs = (char**) malloc(7 * sizeof(char*))) == NULL) {
		perror("malloc() failed");
		exit(1);
	}

	for (int i = 0; i < 7; i++)
		if ((wpctlargs[i] = (char*) malloc(32 * sizeof(char))) == NULL) {
			perror("malloc() failed");
			freearray(wpctlargs);	
			exit(1);
		}
	

	if (!parse_arguments(argc, argv, wpctlargs)) {
		fprintf(stderr, "Wrong arguments\n");
		freearray(wpctlargs);	
		exit(1);
	}
	
	fork_execv("/usr/bin/wpctl", wpctlargs);
	fork_execv("/usr/local/bin/dwmblocksctl", (char **) dctlargs);

	freearray(wpctlargs);	
	return 0;		
}
