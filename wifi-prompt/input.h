#ifndef INPUT_H
#define INPUT_H

#include <glib.h>

int   get_ap_input(GString *string, int argc, char *argv[]);
char* get_password(int argc, char *argv[]);
int   msleep(long msec);

#endif //INPUT_H