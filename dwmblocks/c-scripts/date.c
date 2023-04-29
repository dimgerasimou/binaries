#include "colorscheme.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const char *months[] = {"January",    "February", "March",    "April",
                        "May",        "June",     "July",     "August",
                        "Semptember", "October",  "November", "December"};

const int daysinmonth[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int firstdayinmonth(int mday, int wday) {
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

int calculatemonthdays(int month, int year) {
  if (month != 1)
    return daysinmonth[month];
  if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
    return 29;
  return 28;
}

void getcalendar(char *calendar, int mday, int wday, int month, int year) {
  int firstday = firstdayinmonth(mday, wday);
  int monthdays = calculatemonthdays(month, year);
  char day[64];

  strcpy(calendar, "Mo Tu We Th Fr <span color='#F38BA8'>Sa Su</span>\n");
  for (int i = 0; i < firstday; i++) {
    strcat(calendar, "   ");
  }

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

void getheader(char *header, int month, int year) {
  char yearstr[6];
  char temp[64];
  int padding;

  strcpy(header, "");
  strcat(temp, months[month]);
  sprintf(yearstr, " %d", year);
  strcat(temp, yearstr);

  padding = (20 - strlen(temp)) / 2;
  for (int i = 0; i < padding; i++)
    strcat(header, " ");
  strcat(header, temp);
}

void execcalendar(int mday, int wday, int mon, int year) {
  char calendar[256];
  char header[64];

  switch (fork()) {
  case -1:
    perror("Failed to fork");
    exit(EXIT_FAILURE);

  case 0:
    getheader(header, mon, year);
    getcalendar(calendar, mday, wday, mon, year);
    execl("/bin/dunstify", "dunstify", header, calendar, NULL);
    exit(EXIT_SUCCESS);

  default:
    break;
  }
}

void execfirefox() {
  switch (fork()) {
  case -1:
    perror("Failed to fork");
    exit(EXIT_FAILURE);

  case 0:
    setsid();
    execl("/bin/firefox", "firefox", "--new-window",
          "https://calendar.google.com", NULL);
    exit(EXIT_SUCCESS);

  default:
    break;
  }
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
    execfirefox();
    return;

  default:
    break;
  }
}

int main() {
  time_t currentTime = time(NULL);
  struct tm *localTime = localtime(&currentTime);

  executebutton(localTime->tm_mday, localTime->tm_wday, localTime->tm_mon,
                localTime->tm_year + 1900);

  printf(CLR_1 "   %02d/%02d" NRM "\n", localTime->tm_mday,
         ++localTime->tm_mon);
  return 0;
}
