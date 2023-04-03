#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "colorscheme.h"

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

void sendmessage(int aur, int pacman) {
	char message[64];
	
	sprintf(message, "󰏖 Pacman Updates: %d\n AUR Updates: %d", pacman, aur);
	switch (fork()) {
		case -1:
			perror("Failed to fork");
			exit(EXIT_FAILURE);

		case 0:	execl("/bin/dunstify", "dunstify", "     Packages", message, NULL);
			exit(EXIT_SUCCESS);
		
		default:
	}
}

void executebutton(int aur, int pacman) {
	char *env = getenv("BLOCK_BUTTON");

	if (env == NULL)
		return;
	switch (env[0] - '0') {
		case 1:	sendmessage(aur, pacman);
			break;
		
		default:
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
		printf(CLR_4"󰏖 %d "NRM, aurupdates + pacmanupdates);
	printf(CLR_4" %s"NRM"\n", release);
        return 0;
}
