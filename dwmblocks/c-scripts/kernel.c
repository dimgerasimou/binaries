#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "colorscheme.h"

int main() {
	struct utsname buffer;
	char* release;

	if (uname(&buffer)) {
		puts("Failed to allocate utsname struct");
		return 1;
        }
	release = strtok(buffer.release, "-");
	printf(CLR_6" %s"NRM"\n", release);
        return 0;
}
