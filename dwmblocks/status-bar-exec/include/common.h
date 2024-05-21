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
char* get_path(char **path_array, const int is_file);

/*
 * Returns the pid of the process that it's cmdline argument
 * matches the process string. Negative pids are the corresponding
 * error values. (If process exists more than one time, it fails)
 */
pid_t get_pid_of(const char *process, const char *argv0);

/*
 * Returns the output of xmenu, after `menu` string is passed as the
 * argument. Negative values are the corresponding error values.
 * Parses only single integer xmenu outputs.
 */
int get_xmenu_option(const char *menu, const char *argv0);

/*
 * Sends the given signal to the process with the given name.
 * Returns the pid of the process or error as in get_pid_of().
 */
int killstr(const char *procname, const int signo, const char *argv0);

/*
 * Logs the string in the file defined by log_path global variable,
 * with a timestamp and the argv0 of the caller.
 */
void log_string(const char *string, const char *argv0);

/*
 * Sends a desktop notification with the given arguments.
 */
void notify(const char *summary, const char *body, const char *icon, NotifyUrgency urgency, const int format_summary);

/*
 * Removes a single '\n' character from the string.
 * Returns 1 if it removes any.
 */
int sanitate_newline(const char *string);

#endif /* COMMON_H */
