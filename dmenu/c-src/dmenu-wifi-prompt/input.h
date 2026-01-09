#ifndef INPUT_H
#define INPUT_H

#include <glib.h>

int   get_ap_input(GString *string);
char* get_password(void);
int   msleep(long msec);

#endif //INPUT_H