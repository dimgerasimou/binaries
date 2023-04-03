#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "colorscheme.h"

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

int main() {
	struct utsname buffer;
	char* release;
	int aurupdates;
	int pacmanupdates;

	updatecounter(&aurupdates, &pacmanupdates);

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
