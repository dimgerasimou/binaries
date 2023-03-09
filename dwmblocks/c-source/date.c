#include <stdio.h>
#include <time.h>
#include "colorscheme.h"

int main() {
    time_t currentTime = time(NULL);
    struct tm* localTime = localtime(&currentTime);
	char * monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char * dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

	printf(SFG SBG"  "NFG NBG" %s, %s %d ^d^\n", dayNames[localTime->tm_wday], monthNames[localTime->tm_mon], localTime->tm_mday);

    return 0;
}
