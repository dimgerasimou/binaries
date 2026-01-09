#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "output.h"

const char *log_path[] = {"$HOME", "window-manager.log", NULL};

void
log_string(GString *string)
{
	if (!string && string->len < 1)
		return;

	FILE      *fp;
	GString   *path;
	time_t    rawtime;
	struct tm *timeinfo;

	path = g_string_new("");

	for (int i = 0; log_path[i] != NULL; i++) {
		if (log_path[i][0] == '$') {
			const char *ptr = log_path[i] + 1;
			char *env = getenv(ptr);

			if (!env) {
				fprintf(stderr, "Failed to get env variable:%s\n", log_path[i]);
				exit(EXIT_FAILURE);
			}

			g_string_append_printf(path, "%s/", env);
		} else {
			g_string_append_printf(path, "%s/", log_path[i]);
		}
	}
	
	if (path->len > 0)
		g_string_truncate(path, path->len - 1);

	if (!(fp = fopen(path->str, "a"))) {
		fprintf(stderr, "Failed to open in append mode, path:%s\n", path->str);
		exit(EXIT_FAILURE);
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	fprintf(fp, "%d-%d-%d %d:%d:%d %s\n%s\n", timeinfo->tm_year+1900,
	        timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, "dmenu-wifi-prompt", string->str);
	
	if (string)
		g_string_free(string, TRUE);
	if (fp)
		fclose(fp);
}

int
notify(char *summary, char *body, NotifyUrgency urgency, gboolean format_summary)
{
	NotifyNotification *notification;
	char *sum;

	notify_init("dmenu-wifi-prompt");
	
	if (format_summary)
		sum = get_summary(summary, body);
	else {
		sum = malloc(strlen(summary) + 1);
		strcpy(sum, summary);
	}

	notification = notify_notification_new(sum, body, "wifi-radar");
	notify_notification_set_urgency(notification, urgency);

	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
	notify_uninit();
	free(sum);
	return 0;
}

char*
get_summary(char *text, char *body)
{
	int lcount = 0;
	int mcount = 0;

	char *summary = malloc(sizeof(body) + sizeof(text));
	size_t size;
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
	strcat(summary, "\0");
	size = (strlen(summary)+1)*sizeof(char);
	summary = (char*) realloc(summary, size);
	return summary;
}

void
forkexecv(char *path, char *args[])
{
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

void
signal_dwmblocks()
{
	char *args[] = {"dwmblocksctl", "-s", "internet", NULL};
	forkexecv("/usr/local/bin/dwmblocksctl", args);
}