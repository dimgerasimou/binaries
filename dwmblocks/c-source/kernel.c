#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "colorscheme.h"

int main() {
	struct utsname buffer;
	char* release;

	if (uname(&buffer)) {
		puts("Kern err.");
		return 1;
	}

	release = strtok(buffer.release, "-");

	printf(SFG SBG" 🐧 "NFG NBG" %s ^d^\n", release);
	
	return 0;
}
