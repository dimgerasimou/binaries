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

const int  daysinmonth[]  = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char *firefoxpath[] = {"usr", "bin", "firefox", NULL};
const char *firefoxcmd[]  = {"firefox", "--new-window", "https://calendar.google.com", NULL};

static int
first_day_in_month(int month_day, int week_day)
{
	while (month_day > 7)
		month_day -= 7;

	while (month_day > 1) {
		month_day--;
		week_day--;
		if (week_day == -1)
			week_day = 6;
	}

	week_day--;
	if (week_day == -1)
		week_day = 6;

	return week_day;
}

static int
get_month_days(const int month, const int year)
{
	if (month != 1)
		return daysinmonth[month];
	if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
		return 29;

	return 28;
}

static char*
get_calendar(const int month_day, const int week_day, const int month, const int year)
{
	int  firstday;
	int  monthdays;
	char day[64];
	char calendar[512];
	char *ret;

	firstday  = first_day_in_month(month_day, week_day);
	monthdays = get_month_days(month, year);

	strcpy(calendar, "Mo Tu We Th Fr <span color='#F38BA8'>Sa Su</span>\n");

	for (int i = 0; i < firstday; i++)
		strcat(calendar, "   ");

	for (int i = 1; i <= monthdays; i++) {
		if (i == month_day)
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

	ret = malloc((strlen(calendar) + 1) * sizeof(char));
	strcpy(ret, calendar);
	return ret;
}

static char*
get_summary(const int month, const int year)
{
	char *ret;
	char summary[16];
	int  total_size;

	sprintf(summary, "%s %d", months[month], year);
	total_size = (20 + strlen(summary)) / 2;

	ret = malloc((total_size + 1) * sizeof(char));
	sprintf(ret, "%*s", total_size, summary);
	return ret;
}

static void
exec_calendar(const int month_day, const int week_day, const int month, const int year)
{
	char *body;
	char *summary;

	body    = get_calendar(month_day, week_day, month, year);
	summary = get_summary(month, year);

	notify(summary, body, "calendar", NOTIFY_URGENCY_NORMAL, 0);

	free(body);
	free(summary);
}

static void
executebutton(const int month_day, const int week_day, const int month, const int year)
{
	char *env;
	char *path;

	if ((env = getenv("BLOCK_BUTTON")) == NULL)
		return;

	switch (env[0] - '0') {
	case 1:
		exec_calendar(month_day, week_day, month, year);
		return;

	case 3:
		path = get_path((char**) firefoxpath, 1);

		forkexecv(path, (char **) firefoxcmd, "dwmblocks-date");

		free(path);
		return;

	default:
		break;
	}
}

int
main()
{
	time_t    currentTime = time(NULL);
	struct tm *localTime  = localtime(&currentTime);

	executebutton(localTime->tm_mday, localTime->tm_wday, localTime->tm_mon,
	              localTime->tm_year + 1900);

	printf(CLR_1 "   %02d/%02d" NRM "\n", localTime->tm_mday, ++localTime->tm_mon);

	return 0;
}
