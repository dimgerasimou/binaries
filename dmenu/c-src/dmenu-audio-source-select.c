#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>

typedef struct {
	int id;
	char *name;
} sink;

sink *sinks;
int sinksnum = 0;
const char *configpath = "/.config/dmenu/ignoreaudiosinks";
const char *defaultvol = "0.8";

int parseline(int *id, char *string) {
	char tstr[512];
	char *ptr;

	ptr = string + 7;
	if (sscanf(ptr, "%d %2000[^\n]", id, tstr) < 2)
		return 1;
	if ((ptr = strstr(tstr, "[vol:")) != NULL)
		*ptr = '\0';
	ptr = tstr;
	ptr = ptr + 2;
	strcpy(string, ptr);
	return 0;
}

void cleanstring(char *string) {
	char *ptr;
	char temp[256];

	for (ptr = string; *ptr != '\0'; ptr++) {
		if (*ptr > ' ')
			break;
	}
	strcpy(temp, ptr);
	ptr = temp + strlen(temp) - 1;

	while (ptr >= temp) {
		if (*ptr > ' ') {
			*(ptr+1) = '\0';
			break;
		}
		ptr--;
	}

	strcpy(string, temp);
}

void appendsink(int id, char *name) {
	sinksnum++;
	sinks = (sink*) realloc(sinks, sinksnum * sizeof(sink));
	sinks[sinksnum-1].name = (char*) malloc((strlen(name)+1) * sizeof(char));
	sinks[sinksnum-1].id = id;
	cleanstring(name);
	strcpy(sinks[sinksnum-1].name, name);
}

void freesinks() {
	for (int i = 0; i < sinksnum; i++) {
		free(sinks[i].name);
	}
	free(sinks);
}

void deletesink(int index) {
	free(sinks[index].name);
	for (int i = index; i < sinksnum - 1; i++) {
		sinks[i].name = sinks[i+1].name;
		sinks[i].id = sinks[i+1].id;
	}
	sinksnum--;
	sinks = (sink*) realloc(sinks, sinksnum * sizeof(sink));
}

void removeignoredsinks() {
	FILE *fp = NULL;
	char path[256];
	char *env = NULL;
	char buffer[256];

	env = getenv("HOME");
	if (env == NULL)
		exit(-1);
	strcpy(path, env);
	strcat(path, configpath);

	fp = fopen(path, "r");
	if (fp == NULL)
		return;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		cleanstring(buffer);
		for (int i = 0; i < sinksnum; i++) {
			if (strcmp(buffer, sinks[i].name) == 0) {
				deletesink(i);
				break;
			}
		}
	}
	fclose(fp);
}

int parseinput() {
	FILE *ep;
	char buffer[512];
	int id;
	char *ptr;
	bool writeaudio = false;
	bool writesinks = false;

	if ((ep = popen("wpctl status", "r")) == NULL)
		return 1;
	
	while (fgets(buffer, sizeof(buffer), ep) != NULL) {
		if ((ptr = strchr(buffer, '\n')) != NULL)
			*ptr = '\0';

		if (writeaudio && writesinks) {
			if (!strcmp(buffer, " │  ")) {
				pclose(ep);
				return 0;
			}
			if (parseline(&id, buffer))
				return 2;
			appendsink(id, buffer);
			continue;
		}

		if (writeaudio && !strcmp(buffer, " ├─ Sinks:")) {
			writesinks = true;
			continue;
		}

		if (!writeaudio && !strcmp(buffer, "Audio")) {
			writeaudio = true;
			continue;
		}
	}
	pclose(ep);
	return 2;
}

void sinkconcat(char *out) {
	char temp[8];
	out[0] = '\0';
	for (int i = 0; i < sinksnum; i++) {
		sprintf(temp, "%d", sinks[i].id);
		strcat(out, sinks[i].name);
		strcat(out, "\t");
		strcat(out, temp);
		strcat(out, "\n");
	}
}

int printmenu(char *menu, int menusize) {
	int option=-1;
	int writepipe[2], readpipe[2];
	char buffer[512] = "";
	char *ptr = NULL;

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
			
			execl("/usr/local/bin/dmenu", "dmenu", "-c", "-l", "20", "-nn", "-p", "Select the audio sink:", NULL);
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
	ptr = strchr(buffer, '\t');
	if (ptr == NULL)
		return -1;
	if (ptr[0] != '\0')
		sscanf(ptr, "%d", &option);
	return option;
}

int getsink() {
	int size = sinksnum * 514* sizeof(char);
	char *menu = (char*) malloc(size);
	sinkconcat(menu);
	freesinks();
	int option = printmenu(menu, strlen(menu));
	free(menu);
	return option;
}

void execoption(int option) {
	char opt[8];
	sprintf(opt, "%d", option);
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - xmenu */
			execl("/bin/wpctl", "wpctl", "set-default", opt, NULL);
			exit(EXIT_FAILURE);

		default: /* parent */
			wait(NULL);
			
	}
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - xmenu */
			execl("/bin/wpctl", "wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", defaultvol, NULL);
			exit(EXIT_FAILURE);
		
		default: /* parent */
			wait(NULL);			
	}
	execl("/usr/local/bin/dwmblocksctl", "dwmblocksctl", "-s", "volume", NULL);
}

int main(void) {
	int option;
	if (parseinput())
		return 1;
	removeignoredsinks();

	option = getsink();
	if (option == -1) // So it doesnt segfault at esc
		return -1;
	execoption(option);
	return 0;
}
