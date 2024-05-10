#ifndef COMMON_H
#define COMMON_H

#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <glib.h>

void freestruct(struct dirent **input, int n);
int isnumber(char *string);

void log_string(const char *string, const char *argv0);

/*
 * Forks and executes given command.
 */
void forkexecv(char *path, char *args[]);

/*
 * Centers the summary, according to the size of lines in the body, using spaces.
 */
void format_summary(char *text, char *body, char *summary);

/*
 * Using libnotify sends a desktop notification with the given arguments.
 */
int notify(char *summary, char *body, char *icon, NotifyUrgency urgency, int no_format_summary);


/*
 * Sends the given signal to the proccess with the given name.
 */
void killstr(char *procname, int signo);

#endif /* COMMON_H */
