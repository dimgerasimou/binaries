#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

#define BUFFER_SIZE 64

const char *MONTHS[] = {"January",    "February", "March",    "April",
                        "May",        "June",     "July",     "August",
                        "Semptember", "October",  "November", "December"};

const int   DAYS_IN_MONTH[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char *CAL_PATH[]      = {"usr", "bin", "firefox", NULL};
const char *CAL_ARGS[]      = {"firefox", "--new-window", "https://calendar.google.com", NULL};

static int
get_first_day(int mday, int wday)
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

static int
get_month_days(const int m, const int y)
{
	if (m != 1)
		return DAYS_IN_MONTH[m];
	if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)
		return 29;

	return 28;
}

static char*
get_calendar(const int mday, const int wday, const int m, const int y)
{
	char buf[BUFFER_SIZE];
	char *ret     = NULL;
	int  fday     = 0;
	int  daysm    = 0;
	int  sn_check = 0;

	fday  = get_first_day(mday, wday);
	daysm = get_month_days(m, y);
	ret   = strdup("Mo Tu We Th Fr <span color='#F38BA8'>Sa Su</span>\n");

	for (int i = 0; i < fday; i++)
		strapp(&ret, "   ");

	for (int i = 1; i <= daysm; i++) {
		if (fday == 7) {
			fday = 0;
			strapp(&ret, "\n");
		}

		if (i == mday)
			sn_check = snprintf(buf, sizeof(buf), "<span color='black' bgcolor='#F38BA8'>%2d</span> ", i);
		else if (fday == 5 || fday == 6)
			sn_check = snprintf(buf, sizeof(buf), "<span color='#F38BA8'>%2d</span> ", i);
		else
			sn_check = snprintf(buf, sizeof(buf), "%2d ", i);

		if (sn_check > (int) sizeof(buf) - 1) {
			logwrite("snprintf() buffer overflow", NULL, LOG_ERROR, "dwmblocks-calendar");
			if (ret)
				free(ret);

			return NULL;
		}

		strapp(&ret, buf);
		fday++;
	}

	return ret;
}

static char*
get_summary(const int m, const int y)
{
	char buf[BUFFER_SIZE];
	int  size     = 0;
	int  sn_check = 0;

	size = (15 + strlen(MONTHS[m])) / 2;

	sn_check = snprintf(buf, sizeof(buf), "%*s %d", size, MONTHS[m], y);

	if (sn_check > (int) sizeof(buf) - 1) {
		logwrite("snprintf() buffer overflow", NULL, LOG_ERROR, "dwmblocks-date");
		return NULL;
	}

	return strdup(buf);
}

static void
print_calendar(void)
{
	struct tm *lt   = NULL;
	char      *body = NULL;
	char      *sum  = NULL;
	time_t     ct   = 0;

	ct = time(NULL);
	lt = localtime(&ct);

	body = get_calendar(lt->tm_mday, lt->tm_wday, lt->tm_mon, lt->tm_year + 1900);
	sum  = get_summary(lt->tm_mon, lt->tm_year + 1900);

	if (!body || !sum) {
		if (body)
			free(body);
		if (sum)
			free(sum);
		return;
	}

	notify(sum, body, "calendar", NOTIFY_URGENCY_NORMAL, 0);

	free(body);
	free(sum);
}

static void
exec_block_button(void)
{
	char *env = NULL;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch (atoi(env)) {
	case 1:
		print_calendar();
		return;

	case 3:
	{
		char *path;

		path = get_path((char**) CAL_PATH, 1);
		forkexecv(path, (char **) CAL_ARGS, "dwmblocks-date");

		free(path);
		return;
	}

	default:
		break;
	}
}

int
main()
{
	struct tm *lt = NULL;
	time_t     ct = 0;

	exec_block_button();
	
	ct = time(NULL);
	lt = localtime(&ct);

	printf(CLR_1 "   %02d/%02d" NRM "\n", lt->tm_mday, ++(lt->tm_mon));

	return 0;
}
