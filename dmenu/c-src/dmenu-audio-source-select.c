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

void appendsink(int id, char *name) {
	sinksnum++;
	sinks = (sink*) realloc(sinks, sinksnum * sizeof(sink));
	sinks[sinksnum-1].name = (char*) malloc((strlen(name)+1) * sizeof(char));
	sinks[sinksnum-1].id = id;
	strcpy(sinks[sinksnum-1].name, name);
}

void freesinks() {
	for (int i = 0; i < sinksnum; i++) {
		free(sinks[i].name);
	}
	free(sinks);
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
	char *ptr;

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
			
			execl("/usr/local/bin/dmenu", "dmenu", "-c", "-l", "20", "-p", "Select your audio output:", NULL);
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
	execl("/bin/wpctl", "wpctl", "set-default", opt, NULL);
}

int main(void) {
	int option;
	if (parseinput())
		return 1;

	option = getsink();
	execoption(option);
	return 0;
}
