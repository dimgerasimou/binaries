#include <libnotify/notify.h>
#include <unistd.h>
#include <stdio.h>

#include "output.h"

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