#include <libnotify/notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "colorscheme.h"
#include "common.h"

char *updatecommand[] = {"st", "-e", "sh", "-c", "paru", NULL};

int checkupdates(char *command) {
	FILE *ep;
	char buffer[64];
	int counter = 0;
	if ((ep = popen(command, "r")) == NULL)
		return counter;
	while(fgets(buffer, 64, ep) != NULL)
		counter++;
	pclose(ep);
	return counter;
}

void updatecounter(int *aur, int *pacman) {
	*aur = checkupdates("/bin/paru -Qua");
	*pacman = checkupdates("/bin/checkupdates");
}

void executebutton(int aur, int pacman) {
	char *env = getenv("BLOCK_BUTTON");

	if (env == NULL)
		return;
	if(env[0] == '1') {
		char body[64];

		sprintf(body, "󰏖 Pacman Updates: %d\n AUR Updates: %d", pacman, aur);
		notify("Packages", body, "tux", NOTIFY_URGENCY_LOW, 0);
	}
}

int main() {
	struct utsname buffer;
	char* release;
	int aurupdates;
	int pacmanupdates;

	updatecounter(&aurupdates, &pacmanupdates);
	executebutton(aurupdates, pacmanupdates);

	if (uname(&buffer)) {
		puts("Failed to allocate utsname struct");
		return 1;
        }
	release = strtok(buffer.release, "-");
	if ((aurupdates + pacmanupdates) > 0)
		printf(CLR_12"  󰏖 %d  %s"NRM"\n", aurupdates + pacmanupdates, release);
	else
		printf(CLR_12"   %s"NRM"\n", release);
        return 0;
}
