#ifndef OUTPUT_H
#define OUTPUT_H

#include <libnotify/notification.h>

int   notify(char *summary, char *body, NotifyUrgency urgency, gboolean format_summary);
char* get_summary(char *text, char *body);
void  forkexecv(char *path, char *args[]);
void  signal_dwmblocks();

#endif //OUTPUT_H