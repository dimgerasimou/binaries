#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <stddef.h>

typedef enum {
    LEVEL_OFF = 0,
    LEVEL_MEDIUM,
    LEVEL_FULL,
    LEVEL_CUSTOM
} level_t;

typedef struct {
    level_t level;
    char color[7];      /* RRGGBB, no '#', null-terminated */
    char brightness[4]; /* "0".."100", null-terminated */
} state_t;

/* Fills path with the full state file path. bufsz should be >= PATH_MAX. */
void state_file_path(char *path, size_t bufsz);

/* Loads state from disk. On any failure/missing file, returns defaults
 * (LEVEL_OFF, DEFAULT_COLOR) and does not treat that as an error. */
state_t load_state(void);

/* Writes state to disk. Returns 0 on success, -1 on failure. */
int save_state(const state_t *st);

/* Runs `openrgb --device N --mode off`. Returns 0 on success. */
int apply_off(void);

/* Runs `openrgb --device N --mode static --color XX --brightness YY`.
 * brightness is passed through verbatim (e.g. "50", "100"). */
int apply_static(const char *color, const char *brightness);

const char *level_name(level_t level);
/* off -> medium -> full -> off. LEVEL_CUSTOM is not part of the rotation;
 * toggling while in custom mode returns to off. */
level_t next_level(level_t level);

#endif
