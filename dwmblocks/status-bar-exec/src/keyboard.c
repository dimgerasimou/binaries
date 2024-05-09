#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#include "../include/colorscheme.h"

int main(void) {
	Display *dpy;

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		perror("Cannot open display");
		return EXIT_FAILURE;
	}

	XkbStateRec state;
	XkbGetState(dpy, XkbUseCoreKbd, &state);

	XkbRF_VarDefsRec vd;
	XkbRF_GetNamesProp(dpy, NULL, &vd);
	
	char *tok = strtok(vd.layout, ",");

	for (int i = 0; i < state.group; i++) {
		tok = strtok(NULL, ",");
		if (tok == NULL) {
			return EXIT_FAILURE;
		}
	}
	printf(CLR_5"   %s"NRM"\n", tok);
	
	if (vd.model != NULL) free(vd.model);
	if (vd.layout != NULL) free(vd.layout);
	if (vd.variant != NULL) free(vd.variant);
	if (vd.options != NULL) free(vd.options);
	XCloseDisplay(dpy);
	return 0;
}
