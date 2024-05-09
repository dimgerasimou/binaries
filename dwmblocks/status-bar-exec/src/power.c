#include <libnotify/notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

int printmenu(char *menu, int menusize) {
	int option=-1;
	int writepipe[2], readpipe[2];
	char buffer[64] = "";

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}
	
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - xmenu */
			close(writepipe[1]);
			close(readpipe[0]);
			dup2(writepipe[0], STDIN_FILENO);
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);
			
			execl("/usr/bin/xmenu", "xmenu", NULL);
			exit(EXIT_SUCCESS);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			write(writepipe[1], menu, menusize);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
	}
	if (buffer[0] != '\0')
		sscanf(buffer, "%d", &option);
	return option;
}

void clippause() {
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);
		case 0:
			setsid();
			system("clipctl disable");
			notify("Clipboard", "clipmenu is now disabled.", "com.github.davidmhewitt.clipped", NOTIFY_URGENCY_NORMAL, 0);
			sleep(60);
			system("clipctl enable");
			notify("Clipboard", "clipmenu is now enabled.", "com.github.davidmhewitt.clipped", NOTIFY_URGENCY_NORMAL, 0);
			exit(EXIT_SUCCESS);
		default:
			break;
	}
}

void emptydir(char *path) {
	struct dirent **dir;
	int ret = scandir(path, &dir, NULL, NULL);
	char filepath[256];

	if (ret == -1) {
		perror("Failed to scan the clipmenu directory");
		freestruct(dir, ret);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < ret; i++) {
		strcpy(filepath, path);
		strcat(filepath, "/");
		strcat(filepath, dir[i]->d_name);
		remove(filepath);
	}
	freestruct(dir, ret);
}

void deleteclipall() {
	FILE *ep;
	char path[1024];
	char *ptr = NULL;

	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);
			break;
		case 0:
			if ((ep = popen("clipctl cache-dir", "r")) == NULL){
				perror("Failed to exec clipctl cmd");
				exit(EXIT_FAILURE);
			}
			fgets(path, 1024, ep);
			pclose(ep);
			if ((ptr = strchr(path, '\n')))
				*ptr = '\0';
			emptydir(path);
			strcat(path, "/status");
			ep = fopen(path, "w");
			fprintf(ep, "enabled\n");
			fclose(ep);
			exit(EXIT_SUCCESS);
		default:
			break;
	}
}

void executebutton() {
	char *env = getenv("BLOCK_BUTTON");
	char powermenu[] = " Shutdown\t0\n Reboot\t1\n\n󰗽 Logout\t2\n Lock\t3\n\n Restart DwmBlocks\t4\n󰘚 Optimus Manager\t5\n󰅌 Clipmenu\t6";
	char yesnoprompt[] = "Yes\t1\nNo\t0";
	char optimusmenu[] = "Integrated\t0\nHybrid\t1\nNvidia\t2";
	char clipmenu[] = "Pause clipmenu for 1 minute\t0\nClear clipboard\t1";
	const char *slockargs[] = {"slock", NULL};
	const char *dwmblocksargs[] = {"dwmblocks", NULL};
	int pmsz = sizeof(powermenu);
	int ynsz = sizeof(yesnoprompt);
	int optimussz = sizeof(optimusmenu);
	int clipsz = sizeof(clipmenu);

	if (env == NULL || env[0] != '1')
		return;

	switch (printmenu(powermenu, pmsz)) {
		case 0:
			if (printmenu(yesnoprompt, ynsz) == 1)
				execl("/bin/shutdown", "shutdown", "now", NULL);
			break;
		case 1:
			if (printmenu(yesnoprompt, ynsz) == 1)
				execl("/bin/shutdown", "shutdown", "now", "-r", NULL);
			break;
		case 2:
			killstr("/usr/local/bin/dwm", SIGTERM);
			break;
		case 3:
			sleep(1);
			forkexecv("/usr/local/bin/slock", (char**) slockargs);
			break;
		case 4:
			killstr("dwmblocks", SIGTERM);
			unsetenv("BLOCK_BUTTON");
			forkexecv("/usr/local/bin/dwmblocks", (char**) dwmblocksargs);
			break;
		case 5:
			switch (printmenu(optimusmenu, optimussz)) {
				case 0:
					if (printmenu(yesnoprompt, ynsz) == 1)
						execl("/bin/optimus-manager", "oprimus-manager", "--no-confirm", "--switch", "integrated", NULL);
					break;
				case 1:
					if (printmenu(yesnoprompt, ynsz) == 1)
						execl("/bin/optimus-manager", "oprimus-manager", "--no-confirm", "--switch", "hybrid", NULL);
					break;
				case 2:
					if (printmenu(yesnoprompt, ynsz) == 1)
						execl("/bin/optimus-manager", "oprimus-manager", "--no-confirm", "--switch", "nvidia", NULL);
					break;
				default:
					break;
			}
			break;
		case 6:
			switch (printmenu(clipmenu, clipsz)) {
				case 0:
					clippause();
					break;
				case 1:
					deleteclipall();
					break;
				default:
					break;
			}
		default:
			break;
	}
}


int main(void) {
	executebutton();
	printf(CLR_1 BG_1"  "NRM"\n");
	return 0;
}
