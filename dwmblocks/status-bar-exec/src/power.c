#include <errno.h>
#include <libnotify/notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

/* paths */
const char *PATH_SLOCK[]     = {"usr", "local", "bin", "slock", NULL};
const char *PATH_DWMBLOCKS[] = {"usr", "local", "bin", "dwmblocks", NULL};
const char *PATH_OPTIMUS[]   = {"bin", "optimus-manager", NULL};
const char *PATH_CLIPDEL[]   = {"bin", "sh", NULL };

/* args */
const char *ARGS_SLOCK[]              = {"slock", NULL};
const char *ARGS_DWMBLOCKS[]          = {"dwmblocks", NULL};
const char *ARGS_CLIPDEL[]            = {"sh", "-c", "clipdel -d \".*\"", NULL};
const char *ARGS_OPTIMUS_INTEGRATED[] = {"optimus-manager", "--no-confirm", "--switch", "integrated", NULL};
const char *ARGS_OPTIMUS_HYBRID[]     = {"optimus-manager", "--no-confirm", "--switch", "hybrid", NULL};
const char *ARGS_OPTIMUS_NVIDIA[]     = {"optimus-manager", "--no-confirm", "--switch", "nvidia", NULL};

/* menu prompts */
const char *MENU_POWER     = " Shutdown\t0\n Reboot\t1\n\n󰗽 Logout\t2\n Lock\t3\n\n Restart DwmBlocks\t4\n󰘚 Optimus Manager\t5\n󰅌 Clipmenu\t6";
const char *MENU_OPTIMUS   = "Integrated\t0\nHybrid\t1\nNvidia\t2";
const char *MENU_CLIPBOARD = "Pause clipmenu for 1 minute\t0\nClear clipboard\t1";
const char *MENU_YES_NO    = "Are you sure?\t-1\nYes\t1\nNo\t0";

static void
clipboard_pause(const unsigned int seconds)
{
	switch (fork()) {
	case -1:
		errno = ECHILD;
		logwrite("fork() failed", NULL, LOG_FATAL, "dwmblocks-power");
		exit(errno);

	case 0:
		setsid();
		pid_t pid = get_pid_of("clipmenud", "dmblocks-power");

		if (pid < 0) {
			pid = get_pid_of("bash\0/usr/bin/clipmenud", "dmblocks-power");
			if (pid < 0) {
				errno = ESRCH;
				logwrite("Couldn't get pID of", "clipmenud", LOG_ERROR, "dwmblocks-power");
				exit(ESRCH);
			}
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
clipboard_menu(void)
{
	switch (get_xmenu_option(MENU_CLIPBOARD, "dwmblocks-power")) {
	case 0:
		clipboard_pause(60);
		break;

	case 1:
	{
		char *path = NULL;

		path = get_path((char**) PATH_CLIPDEL, 1);
		forkexecv(path, (char**) ARGS_CLIPDEL, "dwmblocks-power");

		free(path);
		break;
	}

	default:
		break;
	}
}

static void
restart_dwmblocks(void)
{
	char *path = NULL;
	int pid = 0;

	path = get_path((char**) PATH_DWMBLOCKS, 1);

	if ((pid = get_pid_of("dwmblocks", "dwmblocks-power")) == -ENOENT) {
		pid = get_pid_of(path, "dwmblocks-power");
	}

	if (pid <= 0) {
		logwrite("Failed to get the pID of", path, LOG_ERROR, "dwmblocks-power");
		exit(errno);
	}

	kill(pid, SIGTERM);
	unsetenv("BLOCK_BUTTON");
	forkexecv(path, (char**) ARGS_DWMBLOCKS, "dwmblocks-power");
	free(path);
}

static void
lock_screen(void)
{
	char *path = NULL;

	path = get_path((char**) PATH_SLOCK, TRUE);
	sleep(1);
	forkexecv(path, (char**) ARGS_SLOCK, "dwmblocks-power");
	free(path);
}

static void
optimus_menu(void)
{
	char *path = NULL;
	
	path = get_path((char**) PATH_OPTIMUS, 1);

	switch (get_xmenu_option(MENU_OPTIMUS, "dwmblocks-power")) {
	case 0:
		if (get_xmenu_option(MENU_YES_NO, "dwmblocks-power") == 1)
			forkexecv(path, (char**) ARGS_OPTIMUS_INTEGRATED, "dwmblocks-power");
		break;

	case 1:
		if (get_xmenu_option(MENU_YES_NO, "dwmblocks-power") == 1)
			forkexecv(path, (char**) ARGS_OPTIMUS_HYBRID, "dwmblocks-power");
		break;

	case 2:
		if (get_xmenu_option(MENU_YES_NO, "dwmblocks-power") == 1)
			forkexecv(path, (char**) ARGS_OPTIMUS_NVIDIA, "dwmblocks-power");
		break;

	default:
		break;
	}

	free(path);
}

static void
main_menu(void)
{
	switch (get_xmenu_option(MENU_POWER, "dwmblocks-power")) {
	case 0:
		if (get_xmenu_option(MENU_YES_NO, "dwmblocks-power") == 1)
			execl("/bin/shutdown", "shutdown", "now", NULL);
		break;

	case 1:
		if (get_xmenu_option(MENU_YES_NO, "dwmblocks-power") == 1)
			execl("/bin/shutdown", "shutdown", "now", "-r", NULL);
		break;

	case 2:
		if (get_xmenu_option(MENU_YES_NO, "dwmblocks-power") == 1) {
			int pid = get_pid_of("/usr/local/bin/dwm", "dwmblocks-power");
			if (pid > 0) {
				kill(pid, SIGTERM);
			} else {
				logwrite("Failed to get pid of process: ", "dwm", LOG_ERROR, "dwmblocks-power");
				exit(errno);
			}
		}
		break;

	case 3:
		lock_screen();
		break;

	case 4:
		restart_dwmblocks();
		break;

	case 5:
		optimus_menu();
		break;

	case 6:
		clipboard_menu();
		break;

	default:
		break;
	}

}

static void
exec_block_button(void)
{
	char *env = NULL;

	env = getenv("BLOCK_BUTTON");

	if (!env || strcmp(env, "1"))
		return;

	main_menu();
}

int
main(void)
{
	exec_block_button();
	printf(CLR_1 BG_1" "NRM"\n");

	return 0;
}
