#include <pulse/pulseaudio.h>
#include <stdlib.h>
#include <string.h>

#include "sink.h"

typedef struct {
	pa_threaded_mainloop *ml;
	pa_context *ctx;
	int ready;
	int failed;
} sink_ctx_t;

typedef struct {
	sink_list_t *list;
	int done;
} list_req_t;

typedef struct {
	int done;
	int success;
} set_req_t;

typedef struct {
	char *name;
	int done;
} get_name_req_t;

static sink_ctx_t g_ctx = {0};

static void
context_state_cb(pa_context *c, void *userdata)
{
	(void)userdata;

	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY:
		g_ctx.ready = 1;
		pa_threaded_mainloop_signal(g_ctx.ml, 0);
		break;

	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		g_ctx.failed = 1;
		pa_threaded_mainloop_signal(g_ctx.ml, 0);
		break;

	default:
		break;
	}
}

static void
sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
	(void)c;

	list_req_t *req;
	sink_info_t *s;

	req = userdata;
	if (eol > 0) {
		req->done = 1;
		pa_threaded_mainloop_signal(g_ctx.ml, 0);
		return;
	}

	if (!i)
		return;

	req->list->sinks = realloc(req->list->sinks, (req->list->count + 1) * sizeof(sink_info_t));

	s = &req->list->sinks[req->list->count];
	s->name = strdup(i->name);
	s->desc = strdup(i->description);
	s->idx = i->index;
	req->list->count++;
}

static void
sink_info_by_index_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
	(void)c;
	get_name_req_t *req = userdata;

	if (eol > 0 || !i) {
		req->done = 1;
		pa_threaded_mainloop_signal(g_ctx.ml, 0);
		return;
	}

	req->name = strdup(i->name);
	req->done = 1;
	pa_threaded_mainloop_signal(g_ctx.ml, 0);
}

static void
set_default_cb(pa_context *c, int success, void *userdata)
{
	(void)c;
	set_req_t *req = userdata;

	req->success = success;
	req->done = 1;
	pa_threaded_mainloop_signal(g_ctx.ml, 0);
}

int
sink_init(void)
{
	pa_mainloop_api *api;

	if (g_ctx.ml)
		return 0;

	g_ctx.ml = pa_threaded_mainloop_new();
	if (!g_ctx.ml)
		return -1;

	api = pa_threaded_mainloop_get_api(g_ctx.ml);
	g_ctx.ctx = pa_context_new(api, "sink-api");
	if (!g_ctx.ctx) {
		pa_threaded_mainloop_free(g_ctx.ml);
		g_ctx.ml = NULL;
		return -1;
	}

	pa_context_set_state_callback(g_ctx.ctx, context_state_cb, NULL);

	pa_threaded_mainloop_lock(g_ctx.ml);
	
	if (pa_context_connect(g_ctx.ctx, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
		pa_threaded_mainloop_unlock(g_ctx.ml);
		pa_context_unref(g_ctx.ctx);
		pa_threaded_mainloop_free(g_ctx.ml);
		g_ctx.ctx = NULL;
		g_ctx.ml = NULL;
		return -1;
	}

	if (pa_threaded_mainloop_start(g_ctx.ml) < 0) {
		pa_threaded_mainloop_unlock(g_ctx.ml);
		pa_context_disconnect(g_ctx.ctx);
		pa_context_unref(g_ctx.ctx);
		pa_threaded_mainloop_free(g_ctx.ml);
		g_ctx.ctx = NULL;
		g_ctx.ml = NULL;
		return -1;
	}

	while (!g_ctx.ready && !g_ctx.failed)
		pa_threaded_mainloop_wait(g_ctx.ml);

	if (g_ctx.failed) {
		pa_threaded_mainloop_unlock(g_ctx.ml);
		sink_cleanup();
		return -1;
	}

	pa_threaded_mainloop_unlock(g_ctx.ml);
	return 0;
}

void
sink_cleanup(void)
{
	if (!g_ctx.ml)
		return;

	pa_threaded_mainloop_stop(g_ctx.ml);

	if (g_ctx.ctx) {
		pa_context_disconnect(g_ctx.ctx);
		pa_context_unref(g_ctx.ctx);
		g_ctx.ctx = NULL;
	}

	pa_threaded_mainloop_free(g_ctx.ml);
	g_ctx.ml = NULL;
	g_ctx.ready = 0;
	g_ctx.failed = 0;
}

sink_list_t *
sink_get_list(void)
{
	pa_operation *op;
	list_req_t req = {0};

	if (!g_ctx.ml || !g_ctx.ready)
		return NULL;

	req.list = calloc(1, sizeof(sink_list_t));
	if (!req.list)
		return NULL;
	
	pa_threaded_mainloop_lock(g_ctx.ml);

	op = pa_context_get_sink_info_list(g_ctx.ctx, sink_info_cb, &req);
	if (!op) {
		pa_threaded_mainloop_unlock(g_ctx.ml);
		free(req.list);
		return NULL;
	}
	pa_operation_unref(op);

	while (!req.done)
		pa_threaded_mainloop_wait(g_ctx.ml);

	pa_threaded_mainloop_unlock(g_ctx.ml);

	return req.list;
}

int
sink_set_default(uint32_t idx)
{
	pa_operation *op;
	set_req_t set_req = {0};
	get_name_req_t name_req = {0};
	int ret = -1;

	if (!g_ctx.ml || !g_ctx.ready)
		return -1;

	pa_threaded_mainloop_lock(g_ctx.ml);

	/* First, get the sink name from the index */
	op = pa_context_get_sink_info_by_index(g_ctx.ctx, idx, sink_info_by_index_cb, &name_req);
	if (!op) {
		pa_threaded_mainloop_unlock(g_ctx.ml);
		return -1;
	}
	pa_operation_unref(op);

	while (!name_req.done)
		pa_threaded_mainloop_wait(g_ctx.ml);

	if (!name_req.name) {
		pa_threaded_mainloop_unlock(g_ctx.ml);
		return -1;
	}

	/* Now set it as default using the name */
	op = pa_context_set_default_sink(g_ctx.ctx, name_req.name, set_default_cb, &set_req);
	if (!op) {
		free(name_req.name);
		pa_threaded_mainloop_unlock(g_ctx.ml);
		return -1;
	}
	pa_operation_unref(op);

	while (!set_req.done)
		pa_threaded_mainloop_wait(g_ctx.ml);

	ret = set_req.success ? 0 : -1;

	free(name_req.name);
	pa_threaded_mainloop_unlock(g_ctx.ml);

	return ret;
}

void
sink_free_list(sink_list_t *list)
{
	if (!list)
		return;

	for (int i = 0; i < list->count; i++) {
		free(list->sinks[i].name);
		free(list->sinks[i].desc);
	}
	free(list->sinks);
	free(list);
}