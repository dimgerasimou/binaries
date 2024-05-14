#include <libnotify/notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *updatecommand[] = {"st", "-e", "sh", "-c", "paru", NULL};
const char *aurupdatescmd   = "/bin/paru -Qua";
const char *pmupdatescmd    = "/bin/checkupdates";


int
checkupdates(const char *command)
{
	FILE *ep;
	char buffer[64];
	int  counter = 0;

	if ((ep = popen(command, "r")) == NULL)
		return counter;

	while(fgets(buffer, 64, ep) != NULL)
		counter++;

	pclose(ep);
	return counter;
}

void
updatecounter(int *aur, int *pacman)
{
	*aur    = checkupdates(aurupdatescmd);
	*pacman = checkupdates(pmupdatescmd);
}

void
executebutton(int aur, int pacman)
{
	char *env = getenv("BLOCK_BUTTON");

	if (env == NULL)
		return;
	if(env[0] == '1') {
		char body[64];

		sprintf(body, "󰏖 Pacman Updates: %d\n AUR Updates: %d", pacman, aur);
		notify("Packages", body, "tux", NOTIFY_URGENCY_LOW, 0);
	}
}

int
main(void)
{
	struct utsname buffer;
	char*          release;
	int            aurupdates;
	int            pacmanupdates;

	updatecounter(&aurupdates, &pacmanupdates);
	executebutton(aurupdates, pacmanupdates);

	if (uname(&buffer)) {
		log_string("Failed to allocate utsname struct", "dwmblocks-kernel");
		return 1;
	}

	release = strtok(buffer.release, "-");
	if ((aurupdates + pacmanupdates) > 0)
		printf(CLR_12"  󰏖 %d  %s"NRM"\n", aurupdates + pacmanupdates, release);
	else
		printf(CLR_12"   %s"NRM"\n", release);

	return 0;
}
