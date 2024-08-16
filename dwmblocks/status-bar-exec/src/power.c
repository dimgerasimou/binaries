#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

/* paths */
const char *slockpath[] = {"usr", "local", "bin", "slock", NULL};
const char *dwmblpath[] = {"usr", "local", "bin", "dwmblocks", NULL};
const char *optimpath[] = {"bin", "optimus-manager", NULL};

/* args */
const char *clipcache    = "clipctl cache-dir";
const char *slockargs[]  = {"slock", NULL};
const char *dwmblsargs[] = {"dwmblocks", NULL};
const char *optintargs[] = {"optimus-manager", "--no-confirm", "--switch", "integrated", NULL};
const char *opthybargs[] = {"optimus-manager", "--no-confirm", "--switch", "hybrid", NULL};
const char *optnviargs[] = {"optimus-manager", "--no-confirm", "--switch", "nvidia", NULL};

/* menu prompts */
const char *powermenu   = " Shutdown\t0\n Reboot\t1\n\n󰗽 Logout\t2\n Lock\t3\n\n Restart DwmBlocks\t4\n󰘚 Optimus Manager\t5\n󰅌 Clipmenu\t6";
const char *optimusmenu = "Integrated\t0\nHybrid\t1\nNvidia\t2";
const char *clipmenu    = "Pause clipmenu for 1 minute\t0\nClear clipboard\t1";
const char *yesnomenu   = "Are you sure?\t-1\nYes\t1\nNo\t0";

/* function definitons */
static void clipboard_delete(void);
static void clipboard_handle(void);
static void clipboard_pause(const unsigned int seconds);
static void directory_delete_files(const char *path);
static void dwmblocks_restart(void);
static void executebutton(void);
static void lock_screen(void);
static void optimus_handle(void);

static void
clipboard_delete(void)
{
	FILE *ep;
	FILE *fp;

	char path[PATH_MAX - 256];

	switch (fork()) {
	case -1:
		log_string("fork() failed", "dwmblocks-power");
		exit(errno);

	case 0:
		if (!(ep = popen(clipcache, "r"))){
			char log[256];

			sprintf(log, "Failed to exec: %s, %s\n", clipcache, strerror(errno));
			log_string(log, "dwmblocks-internet");
			exit(errno);
		}

		fgets(path, sizeof(path), ep);
		pclose(ep);

		trimtonewl(path);

		directory_delete_files(path);

		strcat(path, "/status");
		fp = fopen(path, "w");

		fprintf(fp, "enabled\n");
		
		fclose(fp);
		exit(EXIT_SUCCESS);

	default:
		break;
	}
}

static void
clipboard_handle(void)
{
	switch (get_xmenu_option(clipmenu, "dwmblocks-power")) {
	case 0:
		clipboard_pause(60);
		break;

	case 1:
		clipboard_delete();
		break;

	default:
		break;
	}
}

static void
clipboard_pause(const unsigned int seconds)
{
	switch (fork()) {
	case -1:
		log_string("fork() failed", "dwmblocks-internet");
		exit(ECHILD);

	case 0:
		setsid();
		pid_t pid = get_pid_of("clipmenud", "dmblocks-power");

		if (pid < 0) {
			pid = get_pid_of("bash\0/usr/bin/clipmenud", "dmblocks-power");
			if (pid < 0)
				exit(ESRCH);
		}

		kill(pid, SIGUSR1);
		notify("Clipboard", "clipmenu is now disabled.", "com.github.davidmhewitt.clipped", NOTIFY_URGENCY_NORMAL, 1);

		sleep(seconds);

		kill(pid, SIGUSR2);
		notify("Clipboard", "clipmenu is now enabled.", "com.github.davidmhewitt.clipped", NOTIFY_URGENCY_NORMAL, 1);

		exit(EXIT_SUCCESS);

	default:
		break;
	}
}

static void
directory_delete_files(const char *path)
{
	struct dirent *ent;
	DIR           *dir;
	char          filepath[PATH_MAX];

	dir = opendir(path);

	if (!dir) {
		char log[512];

		sprintf(log, "Failed to open clipmenu cache dir: %s", strerror(errno));
		log_string(log, "dwmblocks-internet");

		exit(errno);
	}

	while ((ent = readdir(dir))) {
		sprintf(filepath, "%s/%s", path, ent->d_name);
		remove(filepath);
	}

	closedir(dir);
}

static void
dwmblocks_restart(void)
{
	char *path;

	path = get_path((char**) dwmblpath, TRUE);
	unsetenv("BLOCK_BUTTON");

	if (killstr("dwmblocks", SIGTERM, "dwmblocks-power") < 0) {
		if (killstr(path, SIGTERM, "dwmblocks-power") < 0) {
			char log[1024];

			sprintf(log, "Could not get the pid of: %s - %s", path, strerror(ESRCH));
			log_string(log, "dwmblocks-power");
			exit(ESRCH);
		}
	}

	forkexecv(path, (char**) dwmblsargs, "dwmblocks-power");
	free(path);
}

static void
executebutton(void)
{
	char *env;

	env = getenv("BLOCK_BUTTON");

	if (!env || strcmp(env, "1"))
		return;

	switch (get_xmenu_option(powermenu, "dwmblocks-power")) {
	case 0:
		if (get_xmenu_option(yesnomenu, "dwmblocks-power") == 1)
			execl("/bin/shutdown", "shutdown", "now", NULL);
		break;

	case 1:
		if (get_xmenu_option(yesnomenu, "dwmblocks-power") == 1)
			execl("/bin/shutdown", "shutdown", "now", "-r", NULL);
		break;

	case 2:
		killstr("/usr/local/bin/dwm", SIGTERM, "dwmblocks-power");
		break;

	case 3:
		lock_screen();
		break;

	case 4:
		dwmblocks_restart();
		break;

	case 5:
		optimus_handle();
		break;

	case 6:
		clipboard_handle();
		break;

	default:
		break;
	}
}

static void
lock_screen(void)
{
	char *path;

	path = get_path((char**) slockpath, TRUE);
	sleep(1);

	forkexecv(path, (char**) slockargs, "dwmblocks-power");
	free(path);
}

static void
optimus_handle(void)
{
	char *path;
	
	path = get_path((char**) optimpath, 1);

	switch (get_xmenu_option(optimusmenu, "dwmblocks-power")) {
		case 0:
			if (get_xmenu_option(yesnomenu, "dwmblocks-power") == 1)
				forkexecv(path, (char**) optintargs, "dwmblocks-power");
			break;

		case 1:
			if (get_xmenu_option(yesnomenu, "dwmblocks-power") == 1)
				forkexecv(path, (char**) opthybargs, "dwmblocks-power");
			break;

		case 2:
			if (get_xmenu_option(yesnomenu, "dwmblocks-power") == 1)
				forkexecv(path, (char**) optnviargs, "dwmblocks-power");
			break;

		default:
			break;
	}

	free(path);
}

int
main(void)
{
	executebutton();
	printf(CLR_1 BG_1" "NRM"\n");

	return 0;
}
