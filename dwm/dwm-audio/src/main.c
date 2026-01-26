#include <errno.h>
#include <pulse/def.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pa.h"
#include "config.h"

static void die(const char *fmt, ...);
static void exec(const char *cmd, const char *arg, const audio_dev_t dev, const double step);
static int  parseargs(const int argc, char *argv[], audio_dev_t *dev, double *step, double *max_vol, char **cmd, char **arg);
static int  parsedouble(const char *s, double *out);
static void usage(void);

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno = errno;

	fputs("dwm-audio: ", stderr);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

void
exec(const char *cmd, const char *arg, const audio_dev_t dev, const double step)
{
	if (!strcmp(cmd, "up")) {
		if (arg)
			die("'up' takes no arguments");
		if (audio_adjust_volume(dev, step) < 0)
			die("failed to adjust volume");

	} else if (!strcmp(cmd, "down")) {
		if (arg)
			die("'down' takes no arguments");
		if (audio_adjust_volume(dev, -step) < 0)
			die("failed to adjust volume");

	} else if (!strcmp(cmd, "set")) {
		double val;
		if (!arg)
			die("'set' requires a value");
		if (parsedouble(arg, &val))
			die("invalid volume value: '%s'", arg);
		if (audio_set_volume(dev, val) < 0)
			die("failed to set volume");

	} else if (!strcmp(cmd, "mute")) {
		if (!arg) {
			if (audio_toggle_mute(dev) < 0)
				die("failed to toggle mute");
		} else if (!strcmp(arg, "on")) {
			if (audio_set_mute(dev, 1) < 0)
				die("failed to set mute");
		} else if (!strcmp(arg, "off")) {
			if (audio_set_mute(dev, 0) < 0)
				die("failed to set mute");
		} else {
			die("invalid mute argument: '%s' (use 'on' or 'off')", arg);
		}

	} else if (!strcmp(cmd, "get")) {
		double vol;
		if (arg)
			die("'get' takes no arguments");
		vol = audio_get_volume(dev);
		if (vol < 0.0)
			die("failed to get volume");
		printf("%.2f\n", vol);

	} else if (!strcmp(cmd, "status")) {
		double vol;
		int muted;
		if (arg)
			die("'status' takes no arguments");
		vol = audio_get_volume(dev);
		if (vol < 0.0)
			die("failed to get volume");
		muted = audio_get_mute(dev);
		if (muted < 0)
			die("failed to get mute status");
		printf("volume: %.2f (%.0f%%)\n", vol, vol * 100.0);
		printf("muted: %s\n", muted ? "yes" : "no");

	} else {
		die("unknown command: '%s'", cmd);
	}
}

int
parseargs(const int argc, char *argv[], audio_dev_t *dev, double *step, double *max_vol, char **cmd, char **arg)
{
	int i;
	/* parse options */
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		if (!strcmp(argv[i], "-d")) {
			if (++i >= argc)
				die("missing argument for -d");
			if (!strcmp(argv[i], "sink"))
				*dev = AUDIO_SINK;
			else if (!strcmp(argv[i], "source"))
				*dev = AUDIO_SOURCE;
			else
				die("invalid device: '%s'", argv[i]);
		} else if (!strcmp(argv[i], "-s")) {
			if (++i >= argc)
				die("missing argument for -s");
			if (parsedouble(argv[i], step))
				die("invalid step value: '%s'", argv[i]);
		} else if (!strcmp(argv[i], "-m")) {
			if (++i >= argc)
				die("missing argument for -m");
			if (parsedouble(argv[i], max_vol))
				die("invalid max volume: '%s'", argv[i]);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage();
			return 1;
		} else {
			die("invalid option: '%s'", argv[i]);
		}
	}

	/* get command */
	if (i >= argc) {
		usage();
		return 1;
	}
	*cmd = argv[i++];

	/* get optional command argument */
	if (i < argc)
		*arg = argv[i++];

	/* check for extra arguments */
	if (i < argc)
		die("unexpected argument: '%s'", argv[i]);

	return 0;
}

int
parsedouble(const char *s, double *out)
{
	char *end;
	double v;

	if (!s || !*s || !out)
		return -1;

	errno = 0;
	v = strtod(s, &end);

	if (errno != 0 || end == s || *end != '\0' || v < 0.0)
		return -1;

	*out = v;
	return 0;
}

void
usage(void)
{
	fputs("usage: audio-ctl [-d device] [-s step] [-m max] command\n"
	      "\n"
	      "devices:\n"
	      "  sink        output device (default)\n"
	      "  source      input device\n"
	      "\n"
	      "commands:\n"
	      "  up          increase volume\n"
	      "  down        decrease volume\n"
	      "  set VALUE   set volume to VALUE (0.0-max)\n"
	      "  mute        toggle mute\n"
	      "  mute on     enable mute\n"
	      "  mute off    disable mute\n"
	      "  get         get current volume\n"
	      "  status      get volume and mute status\n"
	      "\n"
	      "options:\n"
	      "  -d DEVICE   device type (sink or source)\n"
	      "  -s STEP     volume step for up/down (default: 0.05)\n"
	      "  -m MAX      maximum volume limit (default: 1.5)\n"
	      "  -h          show this help\n",
	      stderr);
}

int
main(int argc, char *argv[])
{
	audio_dev_t dev = DEFAULT_DEV;
	double step     = DEFAULT_STEP;
	double max_vol  = DEFAULT_MAX_VOL;

	char *cmd = NULL;
	char *arg = NULL;

	if (parseargs(argc, argv, &dev, &step, &max_vol, &cmd, &arg))
		return 0;

	if (audio_init(max_vol) < 0)
		die("failed to initialize audio");

	exec(cmd, arg, dev, step);

	audio_cleanup();

	if (*hookargv) {
		execvp(*hookargv, (char**)hookargv);
		_exit(127);
	}

	return 0;
}
