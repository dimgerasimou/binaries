#include <stdio.h>
#include <time.h>

#include "../include/colorscheme.h"

int
main(void)
{
	time_t     ct = time(NULL);
	struct tm* lt = localtime(&ct);

	printf(CLR_7"   %.2d:%.2d"NRM"\n", lt->tm_hour, lt->tm_min);

	return 0;
}
