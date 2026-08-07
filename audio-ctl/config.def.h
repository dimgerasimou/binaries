#define DEFAULT_STEP    0.05
#define DEFAULT_DEV     AUDIO_SINK
#define DEFAULT_MAX_VOL 1.5

/* argv for post run hook (using execvp)
 * example:
 * static const char *hookargv[] = { "pkill", "-RTMIN+5", "dwmblocks", NULL };
 */

static const char *hookargv[] = { NULL };
