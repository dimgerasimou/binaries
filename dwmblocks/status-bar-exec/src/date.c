#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *months[] = {"January",    "February", "March",    "April",
                        "May",        "June",     "July",     "August",
                        "Semptember", "October",  "November", "December"};

const int daysinmonth[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char *firefoxcmd[] = {"firefox", "--new-window", "https://calendar.google.com", NULL};

int
firstdayinmonth(int mday, int wday)
{
	while (mday > 7)
		mday -= 7;

	while (mday > 1) {
		mday--;
		wday--;
		if (wday == -1)
			wday = 6;
	}

	wday--;
	if (wday == -1)
		wday = 6;

	return wday;
}

int
calculatemonthdays(int month, int year)
{
	if (month != 1)
		return daysinmonth[month];
	if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
		return 29;

	return 28;
}

void
getcalendar(char *calendar, int mday, int wday, int month, int year)
{
	int firstday = firstdayinmonth(mday, wday);
	int monthdays = calculatemonthdays(month, year);
	char day[64];

	strcpy(calendar, "Mo Tu We Th Fr <span color='#F38BA8'>Sa Su</span>\n");
	for (int i = 0; i < firstday; i++)
		strcat(calendar, "   ");

	for (int i = 1; i <= monthdays; i++) {
		if (i == mday)
			sprintf(day, "<span color='black' bgcolor='#F38BA8'>%2d</span> ", i);
		else if (firstday == 5 || firstday == 6)
			sprintf(day, "<span color='#F38BA8'>%2d</span> ", i);
		else
			sprintf(day, "%2d ", i);

		if (firstday == 7) {
			firstday = 0;
			strcat(calendar, "\n");
		}

		strcat(calendar, day);
		firstday++;
	}
}

void get_summary(char *summary, int mon, int year) {
	char yearstring[16];
	char temp_summary[64];

	temp_summary[0] = '\0';
	sprintf(yearstring, " %d", year);
	strcpy(summary, months[mon]);
	strcat(summary, yearstring);

	for (int i = 0; i < (20 - (int) strlen(summary)) / 2; i++)
		strcat(temp_summary, " ");

	strcat(temp_summary, summary);
	strcpy(summary, temp_summary);
}

void execcalendar(int mday, int wday, int mon, int year) {
	char body[512];
	char summary[64];

	getcalendar(body, mday, wday, mon, year);
	get_summary(summary, mon, year);
	notify(summary, body, "calendar", NOTIFY_URGENCY_NORMAL, 1);
}


void executebutton(int mday, int wday, int mon, int year) {
	char *env;

	if ((env = getenv("BLOCK_BUTTON")) == NULL)
		return;

	switch (env[0] - '0') {
		case 1:
			execcalendar(mday, wday, mon, year);
			return;

		case 3:
			forkexecv("/usr/bin/firefox", (char **) firefoxcmd, "dwmblocks-date");
			return;

		default:
			break;
	}
}

int main() {
	time_t    currentTime = time(NULL);
	struct tm *localTime  = localtime(&currentTime);

	executebutton(localTime->tm_mday, localTime->tm_wday, localTime->tm_mon,
	              localTime->tm_year + 1900);

	printf(CLR_1 "   %02d/%02d" NRM "\n", localTime->tm_mday, ++localTime->tm_mon);
	return 0;
}
