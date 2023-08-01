#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "common.h"

void forkexecv(char *path, char *args[]) {
	pid_t pID;

	pID = fork();
	if (pID < 0) {
		perror("Fork failed");
		exit(EXIT_FAILURE);
	} else if (pID == 0) {
		setsid();
		execv(path, args);
		perror("Fork execv failed");
		exit(EXIT_FAILURE);
	}
}

void format_summary(char *text, char *body, char *summary) {
	int lcount = 0;
	int mcount = 0;

	summary[0] = '\0';

	for (int i = 0; i < strlen(body); i++) {
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
	for (int i = 0; i < strlen(string); i++)
		if (string[i] > '9' || string[i] < '0')
			return 0;
	return 1;
}

void killstr(char *procname, int signo) {
	FILE *fp;
	struct dirent **pidlist;
	int funcret;
	char buffer[128];
	char filename[128];

	pid_t pID = -1;

	if ((funcret  = scandir("/proc", &pidlist, NULL, alphasort)) == -1) {
		perror("Failed to scan /proc directoy");
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
				fclose(fp);
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
