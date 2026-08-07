#include <pulse/pulseaudio.h>
#include <pulse/volume.h>
#include <string.h>

#include "audio.h"

typedef enum {
	AUDIO_VOL_SET,
	AUDIO_VOL_ADJUST,
	AUDIO_MUTE_TOGGLE,
	AUDIO_MUTE_SET
} audio_op_t;

typedef struct {
	pa_threaded_mainloop *ml;
	pa_context *ctx;
	int ready;
	int failed;
	double max_volume;
} audio_ctx_t;

typedef struct {
	audio_dev_t dev;
	audio_op_t op;
	double value;
	int bool_val;
	
	int done;
	int success;
	pa_cvolume result_vol;
	int result_mute;
} audio_req_t;

static audio_ctx_t g_audio = {0};

/* ========== Helpers ========== */

static pa_volume_t
pct_to_vol(double pct)
{
	if (pct < 0.0) pct = 0.0;
	if (pct > g_audio.max_volume) pct = g_audio.max_volume;
	return (pa_volume_t)(pct * PA_VOLUME_NORM);
}

static double
vol_to_pct(pa_volume_t v)
{
	return (double)v / PA_VOLUME_NORM;
}

static pa_cvolume
vol_set_pct(const pa_cvolume *cur, double pct)
{
	pa_cvolume out = *cur;
	pa_cvolume_scale(&out, pct_to_vol(pct));
	return out;
}

static pa_cvolume
vol_adjust_pct(const pa_cvolume *cur, double delta)
{
	double current = vol_to_pct(pa_cvolume_avg(cur));
	return vol_set_pct(cur, current + delta);
}

/* ========== Callbacks ========== */

static void
op_success_cb(pa_context *ctx, int success, void *userdata)
{
	(void)ctx;
	audio_req_t *req = userdata;
	req->success = success;
	req->done = 1;
	pa_threaded_mainloop_signal(g_audio.ml, 0);
}

static void
sink_info_cb(pa_context *ctx, const pa_sink_info *i, int eol, void *userdata)
{
	audio_req_t *req = userdata;
	pa_operation *op = NULL;

	if (eol > 0) {
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		return;
	}
	if (!i) {
		req->done = 1;
		req->success = 0;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		return;
	}

	req->result_vol = i->volume;
	req->result_mute = i->mute;

	switch (req->op) {
	case AUDIO_VOL_SET:
		req->result_vol = vol_set_pct(&i->volume, req->value);
		op = pa_context_set_sink_volume_by_index(ctx, i->index, 
			&req->result_vol, op_success_cb, req);
		break;
	case AUDIO_VOL_ADJUST:
		req->result_vol = vol_adjust_pct(&i->volume, req->value);
		op = pa_context_set_sink_volume_by_index(ctx, i->index,
			&req->result_vol, op_success_cb, req);
		break;
	case AUDIO_MUTE_TOGGLE:
		req->result_mute = !i->mute;
		op = pa_context_set_sink_mute_by_index(ctx, i->index,
			req->result_mute, op_success_cb, req);
		break;
	case AUDIO_MUTE_SET:
		req->result_mute = req->bool_val;
		op = pa_context_set_sink_mute_by_index(ctx, i->index,
			req->result_mute, op_success_cb, req);
		break;
	}

	if (!op) {
		req->done = 1;
		req->success = 0;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
	} else {
		pa_operation_unref(op);
	}
}

static void
source_info_cb(pa_context *ctx, const pa_source_info *i, int eol, void *userdata)
{
	audio_req_t *req = userdata;
	pa_operation *op = NULL;

	if (eol > 0) {
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		return;
	}
	if (!i) {
		req->done = 1;
		req->success = 0;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		return;
	}

	req->result_vol = i->volume;
	req->result_mute = i->mute;

	switch (req->op) {
	case AUDIO_VOL_SET:
		req->result_vol = vol_set_pct(&i->volume, req->value);
		op = pa_context_set_source_volume_by_index(ctx, i->index,
			&req->result_vol, op_success_cb, req);
		break;
	case AUDIO_VOL_ADJUST:
		req->result_vol = vol_adjust_pct(&i->volume, req->value);
		op = pa_context_set_source_volume_by_index(ctx, i->index,
			&req->result_vol, op_success_cb, req);
		break;
	case AUDIO_MUTE_TOGGLE:
		req->result_mute = !i->mute;
		op = pa_context_set_source_mute_by_index(ctx, i->index,
			req->result_mute, op_success_cb, req);
		break;
	case AUDIO_MUTE_SET:
		req->result_mute = req->bool_val;
		op = pa_context_set_source_mute_by_index(ctx, i->index,
			req->result_mute, op_success_cb, req);
		break;
	}

	if (!op) {
		req->done = 1;
		req->success = 0;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
	} else {
		pa_operation_unref(op);
	}
}

static void
server_info_cb(pa_context *ctx, const pa_server_info *s, void *userdata)
{
	audio_req_t *req = userdata;
	const char *name;
	pa_operation *op = NULL;

	name = (req->dev == AUDIO_SINK) ? s->default_sink_name : s->default_source_name;
	if (!name || !*name) {
		req->done = 1;
		req->success = 0;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		return;
	}

	if (req->dev == AUDIO_SINK)
		op = pa_context_get_sink_info_by_name(ctx, name, sink_info_cb, req);
	else
		op = pa_context_get_source_info_by_name(ctx, name, source_info_cb, req);

	if (!op) {
		req->done = 1;
		req->success = 0;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
	} else {
		pa_operation_unref(op);
	}
}

static void
context_state_cb(pa_context *ctx, void *userdata)
{
	(void)userdata;
	pa_context_state_t state = pa_context_get_state(ctx);

	switch (state) {
	case PA_CONTEXT_READY:
		g_audio.ready = 1;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		g_audio.failed = 1;
		pa_threaded_mainloop_signal(g_audio.ml, 0);
		break;
	default:
		break;
	}
}

/* ========== Public API ========== */

int
audio_init(double max_volume)
{
	pa_mainloop_api *api;
	int ret = 0;

	if (g_audio.ml)
		return 0;  /* already initialized */

	if (max_volume <= 0.0)
		g_audio.max_volume = 1.5;
	else
		g_audio.max_volume = max_volume;

	g_audio.ml = pa_threaded_mainloop_new();
	if (!g_audio.ml)
		return -1;

	api = pa_threaded_mainloop_get_api(g_audio.ml);
	g_audio.ctx = pa_context_new(api, "audio-ctl");
	if (!g_audio.ctx) {
		pa_threaded_mainloop_free(g_audio.ml);
		g_audio.ml = NULL;
		return -1;
	}

	pa_context_set_state_callback(g_audio.ctx, context_state_cb, NULL);

	pa_threaded_mainloop_lock(g_audio.ml);
	
	if (pa_context_connect(g_audio.ctx, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
		pa_threaded_mainloop_unlock(g_audio.ml);
		pa_context_unref(g_audio.ctx);
		pa_threaded_mainloop_free(g_audio.ml);
		g_audio.ctx = NULL;
		g_audio.ml = NULL;
		return -1;
	}

	if (pa_threaded_mainloop_start(g_audio.ml) < 0) {
		pa_threaded_mainloop_unlock(g_audio.ml);
		pa_context_disconnect(g_audio.ctx);
		pa_context_unref(g_audio.ctx);
		pa_threaded_mainloop_free(g_audio.ml);
		g_audio.ctx = NULL;
		g_audio.ml = NULL;
		return -1;
	}

	while (!g_audio.ready && !g_audio.failed)
		pa_threaded_mainloop_wait(g_audio.ml);

	if (g_audio.failed) {
		ret = -1;
		pa_threaded_mainloop_unlock(g_audio.ml);
		audio_cleanup();
	} else {
		pa_threaded_mainloop_unlock(g_audio.ml);
	}

	return ret;
}

void
audio_cleanup(void)
{
	if (!g_audio.ml)
		return;

	pa_threaded_mainloop_stop(g_audio.ml);
	
	if (g_audio.ctx) {
		pa_context_disconnect(g_audio.ctx);
		pa_context_unref(g_audio.ctx);
		g_audio.ctx = NULL;
	}

	if (g_audio.ml) {
		pa_threaded_mainloop_free(g_audio.ml);
		g_audio.ml = NULL;
	}

	g_audio.ready = 0;
	g_audio.failed = 0;
	g_audio.max_volume = 0.0;
}

int
audio_set_max_volume(double max_volume)
{
	if (max_volume <= 0.0)
		return -1;

	g_audio.max_volume = max_volume;
	return 0;
}

double
audio_get_max_volume(void)
{
	return g_audio.max_volume;
}

static int
audio_do_op(audio_req_t *req)
{
	pa_operation *op;

	if (!g_audio.ml || !g_audio.ready)
		return -1;

	req->done = 0;
	req->success = 0;

	pa_threaded_mainloop_lock(g_audio.ml);

	op = pa_context_get_server_info(g_audio.ctx, server_info_cb, req);
	if (!op) {
		pa_threaded_mainloop_unlock(g_audio.ml);
		return -1;
	}
	pa_operation_unref(op);

	while (!req->done)
		pa_threaded_mainloop_wait(g_audio.ml);

	pa_threaded_mainloop_unlock(g_audio.ml);

	return req->success ? 0 : -1;
}

int
audio_set_volume(audio_dev_t dev, double pct)
{
	audio_req_t req = {
		.dev = dev,
		.op = AUDIO_VOL_SET,
		.value = pct
	};
	return audio_do_op(&req);
}

int
audio_adjust_volume(audio_dev_t dev, double delta)
{
	audio_req_t req = {
		.dev = dev,
		.op = AUDIO_VOL_ADJUST,
		.value = delta
	};
	return audio_do_op(&req);
}

int
audio_toggle_mute(audio_dev_t dev)
{
	audio_req_t req = {
		.dev = dev,
		.op = AUDIO_MUTE_TOGGLE
	};
	return audio_do_op(&req);
}

int
audio_set_mute(audio_dev_t dev, int mute)
{
	audio_req_t req = {
		.dev = dev,
		.op = AUDIO_MUTE_SET,
		.bool_val = mute
	};
	return audio_do_op(&req);
}

double
audio_get_volume(audio_dev_t dev)
{
	audio_req_t req = {
		.dev = dev,
		.op = AUDIO_VOL_ADJUST,
		.value = 0.0
	};
	
	if (audio_do_op(&req) != 0)
		return -1.0;
	
	return vol_to_pct(pa_cvolume_avg(&req.result_vol));
}

int
audio_get_mute(audio_dev_t dev)
{
	audio_req_t req = {
		.dev = dev,
		.op = AUDIO_VOL_ADJUST,
		.value = 0.0
	};
	
	if (audio_do_op(&req) != 0)
		return -1;
	
	return req.result_mute;
}
