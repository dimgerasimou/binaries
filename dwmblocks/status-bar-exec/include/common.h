#ifndef COMMON_H
#define COMMON_H

#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <glib.h>

/*
 * Forks and executes given command.
 */
void forkexecv(const char *path, char **args, const char *argv0);

/*
 * Returns the absolute path of the concatenated path_array.
 * If is_file is true, then it doesn't add the last backslash.
 * Works with environment variables too, in the bas format.
 */
char* get_path(char **path_array, int is_file);
pid_t get_pid_of(const char *proccess, const char *argv0);
int get_xmenu_option(const char *menu, const char *argv0);
int isnumber(char *string);

/*
 * Sends the given signal to the proccess with the given name.
 */
int killstr(char *procname, int signo, const char *argv0);
void log_string(const char *string, const char *argv0);

/*
 * Using libnotify sends a desktop notification with the given arguments.
 */
int notify(char *summary, char *body, char *icon, NotifyUrgency urgency, int no_format_summary);
void sanitate_newline(const char *string);

#endif /* COMMON_H */
