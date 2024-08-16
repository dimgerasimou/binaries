#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

const char *swpath[] = { "$HOME", ".local", "bin", "dwm", "keyboard.sh", NULL };
const char *swargs[] = { "keyboard.sh", NULL };

static void
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

static void
execbuton(void)
{
	char *env;
	char *path;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch(atoi(env)) {
	case 1:
		path = get_path((char**) swpath, 1);
		unsetenv("BLOCK_BUTTON");
		forkexecv(path, (char**) swargs, "dwmblocks-keyboard");
		free(path);
		break;

	default:
		break;
	}
}

int
main(void)
{
	Display          *dpy;
	XkbStateRec      state;
	XkbRF_VarDefsRec vd;
	char             *tok;

	execbuton();

	if (!(dpy = XOpenDisplay(NULL)))
		logwrite("XOpenDisplay() failed", NULL, LOG_ERROR, "dwmblocks-keyboard");

	XkbGetState(dpy, XkbUseCoreKbd, &state);
	XkbRF_GetNamesProp(dpy, NULL, &vd);
	
	tok = strtok(vd.layout, ",");

	for (int i = 0; i < state.group; i++) {
		tok = strtok(NULL, ",");
		if (!tok)
			logwrite("Invalid layout for given group", NULL, LOG_ERROR, "dwmblocks-keyboard");
	}

	printf(CLR_5"   %s"NRM"\n", tok);

	XkbRF_FreeVarDefs(&vd);
	XCloseDisplay(dpy);

	return 0;
}
