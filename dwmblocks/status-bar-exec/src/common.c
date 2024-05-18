#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "../include/common.h"

const char *log_path[] = {"$HOME", "window-manager.log", NULL};

void
sanitate_newline(char *string)
{
	char *ptr;

	if ((ptr = strchr(string, '\n')))
		*ptr = '\0';
}

char*
get_path(char **path_array, int is_file)
{
	char path[4096] = "";
	char temp_path[256];
	char *ret;

	for (int i = 0; path_array[i] != NULL; i++) {
		if (path_array[i][0] == '$') {
			const char *ptr = path_array[i] + 1;
			char *env = getenv(ptr);

			if (!env) {
				fprintf(stderr, "Failed to get env variable:%s\n", path_array[i]);
				exit(EXIT_FAILURE);
			}

			sprintf(temp_path, "%s/", env);
			strcat(path, temp_path);
		} else {
			if (i == 0)
				strcat(path, "/");

			sprintf(temp_path, "%s/", path_array[i]);
			strcat(path, temp_path);
		}
	}
	
	if (is_file && path[0] != '\0') {
		char *ptr = strchr(path, '\0');
		ptr--;
		*ptr = '\0';
	}

	ret = (char*) malloc((strlen(path)+1) * sizeof(char));
	strcpy(ret, path);
	return ret;
}

void
log_string(const char *string, const char *argv0)
{
	if (!string && strlen(string) < 1)
		return;

	FILE      *fp;
	time_t    rawtime;
	struct tm *timeinfo;
	char      *path;

	path = get_path((char**) log_path, 1);

	if (!(fp = fopen(path, "a"))) {
		fprintf(stderr, "Failed to open in append mode, path:%s\n", path);
		exit(EXIT_FAILURE);
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	fprintf(fp, "%d-%d-%d %d:%d:%d %s\n%s\n\n", timeinfo->tm_year+1900,
		timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, argv0, string);
	
	if (fp)
		fclose(fp);

	if (path)
		free(path);
}

void forkexecv(char *path, char *args[], const char *argv0) {
	pid_t pID;

	pID = fork();
	if (pID < 0) {
		log_string("fork() failed", argv0);
		exit(EXIT_FAILURE);
	} else if (pID == 0) {
		setsid();
		execv(path, args);

		char string[512];
		strcpy(string, "forkexecv() failed for:");
		strcat(string, args[0]);
		log_string(string, argv0);
		exit(EXIT_FAILURE);
	}
}

void format_summary(char *text, char *body, char *summary) {
	int lcount = 0;
	int mcount = 0;

	summary[0] = '\0';

	for (int i = 0; i < (int) strlen(body); i++) {
		if (body[i] == '\n') {
			if (lcount > mcount)
				mcount = lcount;
			lcount = 0;
			continue;
		}
		lcount++;
	}
	
	if (lcount > mcount)
		mcount = lcount;
	mcount = (mcount - strlen(text)) / 2;

	for (int i = 0; i < mcount; i++)
		strcat(summary, " ");
	strcat(summary, text);
}

int notify(char *summary, char *body, char *icon, NotifyUrgency urgency, int no_format_summary) {
	notify_init("dwmblocks");

	char temp_summary[512];
	if (!no_format_summary)
		format_summary(summary, body, temp_summary);
	else
		strcpy(temp_summary, summary);

	NotifyNotification *notification = notify_notification_new(temp_summary, body, icon);
	notify_notification_set_urgency(notification, urgency);

	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
	notify_uninit();
	return 0;
}

void freestruct(struct dirent **input, int n) {
	while (n--)
		free(input[n]);
	free(input);
}

int isnumber(char *string) {
	for (int i = 0; i < (int) strlen(string); i++)
		if (string[i] > '9' || string[i] < '0')
			return 0;
	return 1;
}

void killstr(char *procname, int signo, const char *argv0) {
	FILE *fp;
	struct dirent **pidlist;
	int funcret;
	char buffer[128];
	char filename[128];

	pid_t pID = -1;

	if ((funcret  = scandir("/proc", &pidlist, NULL, alphasort)) == -1) {
		log_string("Failed to scan /proc directoy", argv0);
		freestruct(pidlist, funcret);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < funcret; i++) {
		if (isnumber(pidlist[i]->d_name)) {
			strcpy(filename, "/proc/");
			strcat(filename, pidlist[i]->d_name);
			strcat(filename, "/cmdline");
			fp = fopen(filename, "r");
			if (fp == NULL) {
				continue;
			}
			fgets(buffer, sizeof(buffer), fp);
			fclose(fp);
			if (strcmp(buffer, procname) == 0) {
				pID = strtol(pidlist[i]->d_name, NULL, 10);
				break;
			}
		}
	}
	if (pID != -1)
		kill(pID, signo);
	freestruct(pidlist, funcret);
}
