#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>

_Noreturn static void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno;

	saved_errno = errno;

	fprintf(stderr, "trackpat-toggle: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

static void
warn(const char *fmt, ...)
{
	va_list ap;
	int saved_errno;

	saved_errno = errno;

	fprintf(stderr, "trackpat-toggle: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);
}

int main(void) {
	Display *dpy;
	Atom enabled_prop;
	XIDeviceInfo *devices;
	int ndevices, toggled;

	if (!(dpy = XOpenDisplay(NULL)))
		die("cannot open display");

	if ((enabled_prop = XInternAtom(dpy, "Device Enabled", True)) == None)
		die("XInput 'Device Enabled' property not found");

	if (!(devices = XIQueryDevice(dpy, XIAllDevices, &ndevices)))
		die("XIQueryDevice failed");

	toggled = 0;
	for (int i = 0; i < ndevices; i++) {
		XIDeviceInfo *dev;
		Atom type;
		unsigned char *data = NULL;
		char lname[256];
		unsigned long nitems, bytes_after;
		size_t len;
		int format;

		dev = &devices[i];
		if (dev->use != XISlavePointer && dev->use != XIFloatingSlave)
			continue;

		len = strlen(dev->name);
		if (len >= sizeof(lname))
			len = sizeof(lname) - 1;

		for (size_t j = 0; j < len; j++)
			lname[j] = tolower((unsigned char)dev->name[j]);
		lname[len] = '\0';

		if (!strstr(lname, "touchpad"))
			continue;

		if (XIGetProperty(dpy, dev->deviceid, enabled_prop, 0, 1, False,
		                  XA_INTEGER, &type, &format, &nitems, &bytes_after, &data) != Success) {
			warn("failed to read property for device %d (%s)", dev->deviceid, dev->name);
			continue;
		}

		if (data && nitems > 0) {
			unsigned char current = data[0];
			unsigned char new_val = current ? 0 : 1;

			XIChangeProperty(dpy, dev->deviceid, enabled_prop, XA_INTEGER, 8,
			                 PropModeReplace, &new_val, 1);
			printf("%s (id=%d): %s -> %s\n", dev->name, dev->deviceid,
			       current ? "enabled" : "disabled", new_val ? "enabled" : "disabled");
			toggled++;
		}
		if (data)
			XFree(data);
	}

	XIFreeDeviceInfo(devices);
	XFlush(dpy);
	XCloseDisplay(dpy);

	if (toggled == 0)
		die("no touchpad devices found");

	return 0;
}
