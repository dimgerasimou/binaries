#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#include "../include/colorscheme.h"
#include "../include/common.h"

void
XkbRF_FreeVarDefs(XkbRF_VarDefsRec *var_defs)
{
	if (!var_defs)
		return;

	free(var_defs->model);
	free(var_defs->layout);
	free(var_defs->variant);
	free(var_defs->options);
	free(var_defs->extra_names);
	free(var_defs->extra_values);
}


int
main(void)
{
	Display          *dpy;
	XkbStateRec      state;
	XkbRF_VarDefsRec vd;
	char             *tok;

	if (!(dpy = XOpenDisplay(NULL))) {
		log_string("Cannot open X11 display", "dwmblocks-keyboard");
		return EXIT_FAILURE;
	}

	XkbGetState(dpy, XkbUseCoreKbd, &state);
	XkbRF_GetNamesProp(dpy, NULL, &vd);
	
	tok = strtok(vd.layout, ",");

	for (int i = 0; i < state.group; i++) {
		tok = strtok(NULL, ",");
		if (!tok) {
			log_string("Cannot get vd.layout", "dwmblocks-keyboard");
			return EXIT_FAILURE;
		}
	}

	printf(CLR_5"   %s"NRM"\n", tok);

	XkbRF_FreeVarDefs(&vd);
	XCloseDisplay(dpy);

	return 0;
}
