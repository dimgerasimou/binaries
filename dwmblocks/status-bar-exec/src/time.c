#include <stdio.h>
#include <time.h>

#include "../include/colorscheme.h"

int
main(void)
{
	struct tm *lt = NULL;
	time_t     ct = 0;
	
	ct = time(NULL);
	lt = localtime(&ct);

	printf(CLR_7"   %.2d:%.2d"NRM"\n", lt->tm_hour, lt->tm_min);
	return 0;
}
