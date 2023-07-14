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

void getheader(char *str, char *body, char *output) {
	int lcount = 0;
	int mcount = 0;
	
	strcpy(output, "");
	for (int i = 0; i < strlen(body); i++) {
		if (body[i] == '\n') {
			if (lcount > mcount)
				mcount = lcount;
			lcount = 0;
		}
		lcount++;
	}
	if (lcount > mcount)
		mcount = lcount;
	mcount-=strlen(str);
	mcount-=2;
	mcount/=2;

	for (int i = 0; i < mcount; i++)
		strcat(output, " ");
	strcat(output, str);
}

void sendmessage(int aur, int pacman) {
	char message[64];
	char header[128];
	
	sprintf(message, "󰏖 Pacman Updates: %d\n AUR Updates: %d", pacman, aur);
	getheader("Packages", message, header);

	switch (fork()) {
		case -1:
			perror("Failed to fork");
			exit(EXIT_FAILURE);

		case 0:	execl("/bin/dunstify", "dunstify", header, message, "--icon=tux", NULL);
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
		printf(CLR_12"  󰏖 %d  %s"NRM"\n", aurupdates + pacmanupdates, release);
	else
		printf(CLR_12"   %s"NRM"\n", release);
        return 0;
}
