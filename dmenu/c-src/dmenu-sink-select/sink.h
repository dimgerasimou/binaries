#ifndef SINK_H
#define SINK_H

#include <stdint.h>

typedef struct {
	char *name;
	char *desc;
	uint32_t idx;
} sink_info_t;

typedef struct {
	sink_info_t *sinks;
	int count;
} sink_list_t;

/* Initialize the sink API. Returns 0 on success, -1 on failure */
int sink_init(void);

/* Get list of all available sinks. Caller must free with sink_free_list() */
sink_list_t *sink_get_list(void);

/* Set the default sink by index. Returns 0 on success, -1 on failure */
int sink_set_default(uint32_t idx);

/* Free a sink list returned by sink_get_list() */
void sink_free_list(sink_list_t *list);

/* Cleanup the sink API */
void sink_cleanup(void);

#endif /* SINK_H */
