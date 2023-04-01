#include <stdio.h>
#include <time.h>
#include "colorscheme.h"

int main() {
    time_t currentTime = time(NULL);
    struct tm* localTime = localtime(&currentTime);

	printf(SFG SBG"  "NBG NFG" %.2d:%.2d:%.2d ^d^\n", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    return 0;
}
