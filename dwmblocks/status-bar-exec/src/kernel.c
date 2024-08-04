#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *updatecmdpath   = "/usr/local/bin/st";
const char *updatecmdargs[] = {"st", "-e", "sh", "-c", "paru", NULL};
const char *aurupdatescmd   = "/bin/paru -Qua";
const char *pmupdatescmd    = "/bin/checkupdates";


static int
check_updates(const char *command)
{
	FILE *ep;
	char buffer[64];
	int  counter = 0;

	if (!(ep = popen(command, "r")))
		return 0;

	while(fgets(buffer, sizeof(buffer), ep))
		counter++;

	pclose(ep);
	return counter;
}

static void
update_counter(int *aur, int *pacman)
{
	*aur    = check_updates(aurupdatescmd);
	*pacman = check_updates(pmupdatescmd);
}

static void
execute_button(const int aur, const int pacman)
{
	char *env;
	char *body;
	
	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch (atoi(env)) {
	case 1:
		body = malloc(64 * sizeof(char));
		sprintf(body, "󰏖 Pacman Updates: %d\n AUR Updates: %d", pacman, aur);
		notify("Packages", body, "tux", NOTIFY_URGENCY_LOW, 1);
		free(body);
		break;

	case 3:
		forkexecv(updatecmdpath, (char**) updatecmdargs, "dwmblocks-kernel");
		break;

	default:
		break;
	}
}

int
main(void)
{
	struct utsname buffer;
	char*          release;
	int            aurupdates;
	int            pacmanupdates;

	update_counter(&aurupdates, &pacmanupdates);
	execute_button(aurupdates, pacmanupdates);

	if (uname(&buffer)) {
		log_string("Failed to allocate utsname struct", "dwmblocks-kernel");
		return errno;
	}

	release = strtok(buffer.release, "-");

	if ((aurupdates + pacmanupdates) > 0)
		printf(CLR_12"  󰏖 %d  %s"NRM"\n", aurupdates + pacmanupdates, release);
	else
		printf(CLR_12"   %s"NRM"\n", release);

	return 0;
}
