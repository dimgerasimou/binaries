#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "input.h"
#include "config.h"

static char **
makemenuargv(const char *base[], size_t basec, int argc, char *argv[])
{
	char **v;
	size_t i;

	v = malloc((basec + (size_t)argc + 1) * sizeof(*v));
	if (!v) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < basec; i++)
		v[i] = (char *)base[i];
	for (i = 0; i < (size_t)argc; i++)
		v[basec + i] = argv[i];
	v[basec + i] = NULL;

	return v;
}

int
get_ap_input(GString *string, int argc, char *argv[])
{
	const char *base[] = { menucmd, "-p", "Select wifi access point:" };
	int  option = -1;
	int  writepipe[2], readpipe[2];
	char buffer[512] = "";
	char *ptr;
	char **menuargv;

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}

	menuargv = makemenuargv(base, sizeof(base) / sizeof(*base), argc, argv);

	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - dmenu */
			close(writepipe[1]);
			close(readpipe[0]);
			dup2(writepipe[0], STDIN_FILENO);
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);

			execvp(menuargv[0], menuargv);
			exit(EXIT_FAILURE);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			write(writepipe[1], string->str, string->len);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
			free(menuargv);
	}

	ptr = strchr(buffer, '\t');
	if (ptr != NULL)
		sscanf(ptr, "%d", &option);

	return option;
}

char*
get_password(int argc, char *argv[])
{
	const char *base[] = { menucmd, "-P", "-p", "Enter the AP's password:" };
	int  writepipe[2], readpipe[2];
	char buffer[512] = "";
	char **menuargv;

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}

	menuargv = makemenuargv(base, sizeof(base) / sizeof(*base), argc, argv);

	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - dmenu */
			close(writepipe[1]);
			close(readpipe[0]);
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);

			execvp(menuargv[0], menuargv);
			exit(EXIT_FAILURE);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
			free(menuargv);
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