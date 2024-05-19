#include <linux/limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdint.h>

#include "../include/common.h"

const char *log_path[] = {"$HOME", "window-manager.log", NULL};

/* file specific functions */

static char*
format_summary(const char *summary, const char *body)
{
	int count = 0;
	int max_count = 0;
	char *ret;

	for (char *ptr = (char*) body; *ptr != '\0'; ptr++) {
		if (*ptr == '\n') {
			max_count = count > max_count ? count : max_count;
			count = 0;
			continue;
		}
		count++;
	}

	max_count = count > max_count ? count : max_count;
	max_count = (max_count - strlen(summary)) / 2;
	count = max_count + strlen(summary);

	ret = malloc((count + 1) * sizeof(ret));

	sprintf(ret, "%*s", count, summary);

	return ret;
}

/* header functions */

void
forkexecv(const char *path, char **args, const char *argv0)
{
	switch (fork()) {
	case -1:
		log_string("fork() failed", argv0);
		exit(errno);

	case 0:
		setsid();
		execv(path, args);

		char log[512];
		sprintf(log, "forkexecv() failed for:%s - %s", args[0], strerror(errno));
		log_string(log, argv0);
		exit(errno);

	default:
		break;
	}
}

char*
get_path(char **path_array, int is_file)
{
	char path[PATH_MAX];
	char name[NAME_MAX];
	char *ret;

	for (int i = 0; path_array[i] != NULL; i++) {
		if (path_array[i][0] == '$') {
			const char *ptr = path_array[i] + 1;
			char *env = getenv(ptr);

			if (!env) {
				fprintf(stderr, "Failed to get env variable:%s - %s\n", path_array[i], strerror(errno));
				exit(errno);
			}

			sprintf(name, "%s/", env);
			strcat(path, name);
		} else {
			if (i == 0)
				strcat(path, "/");

			sprintf(name, "%s/", path_array[i]);
			strcat(path, name);
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
sanitate_newline(const char *string)
{
	char *ptr;

	if ((ptr = strchr(string, '\n')))
		*ptr = '\0';
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

int notify(char *summary, char *body, char *icon, NotifyUrgency urgency, int no_format_summary) {
	notify_init("dwmblocks");

	char *sum;

	if (!no_format_summary) {
		sum = format_summary(summary, body);
	} else {
		sum = malloc((strlen(summary) + 1) * sizeof(char));
		strcpy(sum, summary);
	}

	NotifyNotification *notification = notify_notification_new(sum, body, icon);
	notify_notification_set_urgency(notification, urgency);

	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
	notify_uninit();
	free(sum);
	return 0;
}

int isnumber(char *string) {
	for (int i = 0; i < (int) strlen(string); i++)
		if (string[i] > '9' || string[i] < '0')
			return 0;
	return 1;
}

int str_to_uint64(const char *input, uint64_t *output) {
	char *endptr;
	errno = 0;

	uint64_t val = strtoull(input, &endptr, 10);
	if (errno > 0) {
		return -1;
	}
	if (!endptr || endptr == input || *endptr != 0) {
		return -EINVAL;
	}
	if (val != 0 && input[0] == '-') {
		return -ERANGE;
	}

	*output = val;
	return 0;
}

pid_t
get_pid_of(const char *proccess, const char *argv0)
{
	DIR           *dir;
	pid_t         ret;
	struct dirent *ent;
	FILE          *fp;
	char          buffer[PATH_MAX];
	uint64_t      pid;

	dir = opendir("/proc");
	ret = 0;

	if (!dir) {
		log_string("Failed to get /proc directory", argv0);
		return -ENOENT;
	}

	while ((ent = readdir(dir)) && ret >= 0) {
		 if (str_to_uint64(ent->d_name, &pid) < 0)
			continue;

		snprintf(buffer, sizeof(buffer), "/proc/%s/cmdline", ent->d_name);
		fp = fopen(buffer, "r");
		if (!fp)
			continue;
		if (!fgets(buffer, sizeof(buffer), fp))
			continue;
		if ((strcmp(buffer, proccess) == 0)) {
			ret = ret ? -EEXIST : (pid_t)pid;
		}
	}

	closedir(dir);
	return ret ? ret : -ENOENT;
}

pid_t
killstr(char *procname, int signo, const char *argv0)
{
	pid_t pID = get_pid_of(procname, argv0);

	if (pID > 0) {
		kill(pID, signo);
		return 0;
	}
	return pID;
}

int
get_xmenu_option(const char *menu, const char *argv0)
{
	int  option;
	int  writepipe[2];
	int  readpipe[2];
	char buffer[16];

	option = -EREMOTEIO;
	buffer[0] = '\0';

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		log_string("Failed to initialize pipes", argv0);
		return -ESTRPIPE;
	}
	
	switch (fork()) {
		case -1:
			log_string("fork() failed", argv0);
			return -ECHILD;

		case 0: /* child - xmenu */
			close(writepipe[1]);
			close(readpipe[0]);

			dup2(writepipe[0], STDIN_FILENO);
			close(writepipe[0]);

			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);
			
			execl("/usr/bin/xmenu", "xmenu", NULL);
			exit(EXIT_FAILURE);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);

			write(writepipe[1], menu, strlen(menu) + 1);
			close(writepipe[1]);

			wait(NULL);

			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
	}

	if (buffer[0] != '\0')
		sscanf(buffer, "%d", &option);

	return option;
}