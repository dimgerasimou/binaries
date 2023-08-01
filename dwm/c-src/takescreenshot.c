#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libnotify/notify.h>

static char *screenshotdir[] = {"$HOME", "/Pictures", "/Screenshots", NULL};
static char *logpath[] = {"$HOME", "/logs"};
static char *scrotcmd[] = {"scrot", NULL};

void notify_screenshot();
void writelog(char *log);
void makedir(char **fullpath, char *path);
int execvfork(char *path, char *argv[]);

void notify_screenshot() {
	notify_init("dwm");
	NotifyNotification *notification = notify_notification_new("     Scrot", "Screenshot taken", "display");
	notify_notification_set_urgency(notification, NOTIFY_URGENCY_LOW);
	
	if (!notify_notification_show(notification, 0))
		writelog("Warning:Failed to notify the screenshot.");
	notify_uninit();
}

void writelog(char *log) {
	FILE *fp;
	time_t currentTime = time(NULL);
	struct tm* lt = localtime(&currentTime);
	char isotime[64];
	char path[512];

	sprintf(isotime, "%d-%02d-%02d %02d:%02d:%2d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

	makedir(logpath, path);
	strcat(path, "/dwm.log");
	fp = fopen(path, "a");
	fprintf(fp, "dwm:takescreenshot:%s:%s\n", isotime, log);
	fclose(fp);
}

void makedir(char **fullpath, char *path) {
	struct stat sb;
	char *env;
	int i = 0;

	while (fullpath[i] != NULL) {
		if (strcmp(fullpath[i], "$HOME") == 0) {
			if ((env = getenv("HOME")) == NULL) {
				perror("dwm:takescreenshot:Critical Error: Failed to get \"HOME\" env variable");
				exit(EXIT_FAILURE);
			}
			strcpy(path, env);
			if (stat(path, &sb) == -1) {
				perror("dwm:takescreenshot:Critical Error: home directory doesn't exist");
				exit(EXIT_FAILURE);
			}
			if (!(sb.st_mode & S_IFDIR)) {
				perror("dwm:takescreenshot:Critical Error: home path is not a directory");
				exit(EXIT_FAILURE);
			}
			i++;
			continue;
		}
		strcat(path, fullpath[i]);
		if (stat(path, &sb) == -1) {
			if (mkdir(path, 0700) == -1) {
				char log[1024];

				sprintf(log, "Critical Error: mkdir() failed to make path '%s':%s", path, strerror(errno));
				writelog(log);
				exit(EXIT_FAILURE);
			}
		} else if (!(sb.st_mode & S_IFDIR)) {
			char log[1024];

			sprintf(log, "Critical Error: path '%s' exists but is not a directory:%s", path, strerror(errno));
				writelog(log);
			exit(EXIT_FAILURE);
		}
		i++;
	}
}

int execvfork(char *path, char *argv[]) {
	pid_t pID;
	int fd[2];
	int ret = 0;

	if (pipe(fd) == -1) {
		char log[1024];

		sprintf(log, "Critical Error: pipe failed:%s", strerror(errno));
				writelog(log);
			exit(EXIT_FAILURE);
	}

	pID = fork();
	if (pID < 0 ) { // Fork failure
		char log[512];

		sprintf(log, "Critical Error: fork failed:%s", strerror(errno));
				writelog(log);
			exit(EXIT_FAILURE);
	} else if (pID == 0) { //Child
		char buffer[2];

		close(fd[0]);
		ret = execv(path, argv);
		buffer[0] = '1';
		buffer[1] = '\0';
		write(fd[1], buffer, 2);
		close(fd[1]);
		exit(EXIT_SUCCESS);
	} else { //Parent
		char buffer[2] = {'0', '0'};

		close(fd[1]);
		wait(NULL);
		read(fd[0], buffer, 2);
		close(fd[0]);
		if (buffer[0] == '1')
			ret = -1;
	}
	return ret;
}

int main(void) {
	char path[512];

	makedir(screenshotdir, path);
	if (chdir(path) == -1) {
		char log[1024];

		sprintf(log, "Critical Error: chdir() failed in path '%s':%s", path, strerror(errno));
		writelog(log);
		exit(EXIT_FAILURE);
	}
	if (execvfork("/usr/bin/scrot", scrotcmd) == -1) {
		char log[1024];

		sprintf(log, "Critical Error: Failed in executing scrot:%s", strerror(errno));
		writelog(log);
		exit(EXIT_FAILURE);
	}
	notify_screenshot();
	return EXIT_SUCCESS;
}
