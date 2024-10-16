#include <math.h>
#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/colorscheme.h"
#include "../include/common.h"

typedef struct {
	unsigned int volume;
	unsigned int mute;
	unsigned int done;
} AudioInfo;

static pa_mainloop *ml = NULL;
static pa_context *ctx = NULL;
static pa_mainloop_api *mlapi = NULL;

const char *eeargs[] = {"easyeffects", NULL};
const char *volinc[] = {"audiocontrol", "sink", "increase", NULL};
const char *voldec[] = {"audiocontrol", "sink", "decrease", NULL};
const char *volmut[] = {"audiocontrol", "sink", "mute", NULL};
const char *acpath[] = { "$HOME", ".local", "bin", "dwm", "audiocontrol", NULL};

static void contextcb(pa_context *c, void *userdata);
static void execbutton(AudioInfo **a);
static void freeaudioinfo(AudioInfo ***a);
static void freepa(void);
static AudioInfo** getaudioinfo(void);
static AudioInfo** initaudioinfo(void);
static void initpa(void);
static void propnotify(AudioInfo **a);
static void servercb(pa_context *c, const pa_server_info *i, void *userdata);
static void sinkcb(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
static void sourcecb(pa_context *c, const pa_source_info *i, int eol, void *userdata);
static void quitml(AudioInfo **a);

static void
contextcb(pa_context *c, void *userdata)
{
	pa_operation *o;

	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY:
		if ((o = pa_context_get_server_info(c, servercb, userdata)))
			pa_operation_unref(o);
		break;
	
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		pa_mainloop_quit(ml, 1);
		break;

	default:
		break;
	}
}

static void
execbutton(AudioInfo **a)
{
	char *env;
	char *path;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch (atoi(env)) {
	case 1:
		propnotify(a);
		break;

	case 2:
		forkexecv("/usr/bin/easyeffects", (char**) eeargs, "dwmblocks-volume");
		break;

	case 3:
		path = get_path((char**) acpath, 1);
		forkexecv(path, (char**) volmut, "dwmblocks-volume");
		free(path);
		break;

	case 4:
		path = get_path((char**) acpath, 1);
		forkexecv(path, (char**) volinc, "dwmblocks-volume");
		free(path);
		break;

	case 5:
		path = get_path((char**) acpath, 1);
		forkexecv(path, (char**) voldec, "dwmblocks-volume");
		free(path);
		break;

	default:
		break;
	}
}

static void
freeaudioinfo(AudioInfo ***a)
{
	if (!*a)
		return;

	for (int i = 0; i < 2; i++)
		if (!*a[i])
			free(*a[i]);
	free(*a);
	*a = NULL;
}

static void
freepa(void)
{
	if (ctx) {
		pa_context_disconnect(ctx);
		pa_context_unref(ctx);
	}
	if (ml) {
		pa_mainloop_free(ml);
	}
}

static AudioInfo**
getaudioinfo(void)
{
	AudioInfo **a = initaudioinfo();

	initpa();

	pa_context_set_state_callback(ctx, contextcb, a);
	pa_context_connect(ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);

	pa_mainloop_run(ml, NULL);

	freepa();
	return a;
}

static AudioInfo**
initaudioinfo(void)
{
	AudioInfo **ret = NULL;
	
	if (!(ret = malloc(2 * sizeof(AudioInfo*))))
		logwrite("malloc() failed", NULL, LOG_ERROR, "dwmblocks-volume");

	for (int i = 0; i < 2; i++) {
		if (!(ret[i] = malloc(sizeof(AudioInfo))))
			logwrite("malloc() failed", NULL, LOG_ERROR, "dwmblocks-volume");

		ret[i]->mute = 0;
		ret[i]->done = 0;
		ret[i]->volume = 0;
	}

	return ret;
}

static void
initpa(void)
{
	if (!(ml = pa_mainloop_new()))
		logwrite("pa_mainloop_new() failed to initialize", NULL, LOG_ERROR, "dwmblocks-volume");
	if (!(mlapi = pa_mainloop_get_api(ml)))
		logwrite("pa_mainloop_get_api() failed to initialize", NULL, LOG_ERROR, "dwmblocks-volume");
	if (!(ctx = pa_context_new(mlapi, "dwmblocks-volume")))
		logwrite("pa_context_new() failed to initialize", NULL, LOG_ERROR, "dwmblocks-volume");
}

static void
propnotify(AudioInfo **a)
{
	char *buffer = NULL;
	char temp[64];
	
	if (a[0]->done == 1) {
		snprintf(temp, sizeof(temp), "  Volume: %3d%%, Muted: %s\n",
		         a[0]->volume, a[0]->mute ? "Yes" : "No");
	} else {
		snprintf(temp, sizeof(temp), "No audio sink detected.\n");
	}
	strapp(&buffer, temp);

	if (a[1]->done == 1) {
		snprintf(temp, sizeof(temp), "  Volume: %3d%%, Muted: %s\n",
		         a[1]->volume, a[1]->mute ? "Yes" : "No");
	} else {
		snprintf(temp, sizeof(temp), "No audio source detected.\n");
	}
	strapp(&buffer, temp);

	notify("Audio Properties", buffer, "audio-headphones", NOTIFY_URGENCY_NORMAL, 1);
	free(buffer);
}

static void
servercb(pa_context *c, const pa_server_info *i, void *userdata)
{
	pa_operation *o;

	if ((o = pa_context_get_sink_info_by_name(c, i->default_sink_name, sinkcb, userdata)))
		pa_operation_unref(o);
	if ((o = pa_context_get_source_info_by_name(c, i->default_source_name, sourcecb, userdata)))
		pa_operation_unref(o);
}

static void
sinkcb(pa_context __attribute__((__unused__)) *c, const pa_sink_info *i, int eol, void *userdata)
{
	AudioInfo **a = (AudioInfo**) userdata;

	if (eol > 0)
		return;
	if (!i) {
		a[0]->done = 2;
		quitml(a);
		return;
	}

	pa_volume_t volume = pa_cvolume_avg(&i->volume);
	a[0]->volume = round((volume * 100.0) / PA_VOLUME_NORM);
	a[0]->mute = i->mute;

	a[0]->done = 1;
	quitml(a);
}

static void
sourcecb(pa_context __attribute__((__unused__)) *c, const pa_source_info *i, int eol, void *userdata)
{
	AudioInfo **a = (AudioInfo**) userdata;

	if (eol > 0)
		return;
	if (!i) {
		a[1]->done = 2;
		quitml(a);
		return;
	}

	pa_volume_t volume = pa_cvolume_avg(&i->volume);
	a[1]->volume = round((volume * 100.0) / PA_VOLUME_NORM);
	a[1]->mute = i->mute;

	a[1]->done = 1;
	quitml(a);
}

static void
quitml(AudioInfo **a)
{
	if (a[0]->done && a[1]->done)
		pa_mainloop_quit(ml, 0);
}

int
main(void)
{
	AudioInfo **a;
	char s[16];

	a = getaudioinfo();
	execbutton(a);
	
	if (a[0]->mute || a[0]->done != 1) {
		strcpy(s, " ");
	} else if (a[0]->volume > 66) {
		snprintf(s, sizeof(s), "  %d%%", a[0]->volume);
	} else if (a[0]->volume > 33) {
		snprintf(s, sizeof(s), " %d%%", a[0]->volume);
	} else {
		snprintf(s, sizeof(s), " %d%%", a[0]->volume);
	}

	freeaudioinfo(&a);

	printf(CLR_2"%s"NRM"\n", s);
	return 0;
}
