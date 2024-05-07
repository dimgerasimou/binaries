#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include <stdio.h>
#include <time.h>

#include "input.h"

int
get_ap_input(char *menu, char *prompt)
{
	int  option = -1;
	int  menusize = strlen(menu);
	int  writepipe[2], readpipe[2];
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
			
			execl("/usr/local/bin/dmenu", "dmenu", "-c", "-nn", "-l", "20", "-p", prompt, NULL);
			exit(EXIT_FAILURE);

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
	if (ptr != NULL)
		sscanf(ptr, "%d", &option);

	return option;
}

char*
get_password(void)
{
	int  writepipe[2], readpipe[2];
	char buffer[512] = "";

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
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);
			
			execl("/usr/local/bin/dmenu", "dmenu", "-c", "-nn", "-P", "-p", "Enter the AP's password:", NULL);
			exit(EXIT_FAILURE);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
	}
	for (int i = 0; buffer[i] != '\0'; i++) {
		if(buffer[i] == '\n') {
			buffer[i] = '\0';
			break;
		}
	}
	return g_strdup(buffer);
}

int
msleep(long msec)
{
	struct timespec ts;
	int res;

	if (msec < 0) {
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}