#include <stdio.h>
#include <time.h>

#include "../include/colorscheme.h"

int
main(void)
{
	time_t     currentTime = time(NULL);
	struct tm* localTime   = localtime(&currentTime);

	printf(CLR_7"   %.2d:%.2d"NRM"\n", localTime->tm_hour, localTime->tm_min);

	return 0;
}
