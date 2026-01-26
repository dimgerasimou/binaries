#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "config.h"

static void die(const char *fmt, ...);
static int  xkbgroupcount(Display *dpy);
static int  xkbnext(void);

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno = errno;

	fputs("dwm-xkbnext: ", stderr);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

int
xkbgroupcount(Display *dpy)
{
	XkbDescPtr xkb = XkbAllocKeyboard();
	if (!xkb)
		return 0;
	xkb->dpy = dpy;

	int groups = 0;

	if (XkbGetNames(dpy, XkbGroupNamesMask, xkb) == Success && xkb->names) {
		for (int i = 0; i < XkbNumKbdGroups; i++) {
			if (xkb->names->groups[i] != None)
				groups++;
		}
	}

	XkbFreeKeyboard(xkb, 0, True);
	return groups;
}

int
xkbnext(void)
{
	Display *dpy;
	XkbStateRec st;
	unsigned int gc;
	unsigned int n;

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("XOpenDisplay() failed");

	if (XkbGetState(dpy, XkbUseCoreKbd, &st) != Success) {
		XCloseDisplay(dpy);
		die("XkbGetState() failed");
	}

	gc = xkbgroupcount(dpy);
	if (gc <= 0)
		gc = 4;

	n = (st.group + 1u) % gc;

	if (!XkbLockGroup(dpy, XkbUseCoreKbd, n)) {
		XCloseDisplay(dpy);
		die("XkbLockGroup() failed");
	}

	XFlush(dpy);
	XCloseDisplay(dpy);
	return 0;
}

int
main(void)
{
	xkbnext();

	if (*hookargv) {
		execvp(*hookargv, (char**)hookargv);
		_exit(127);
	}

	return 0;
}
