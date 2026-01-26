#ifndef PA_H
#define PA_H

typedef enum {
	AUDIO_SINK,
	AUDIO_SOURCE
} audio_dev_t;

/* 
 * Initialize audio subsystem with maximum volume limit.
 * max_volume: maximum allowed volume as percentage (e.g., 1.0 = 100%, 1.5 = 150%)
 *             Pass 0.0 or negative to use default (1.5)
 * Returns 0 on success, -1 on error.
 */
int audio_init(double max_volume);

/* Clean up audio subsystem. Call at exit. */
void audio_cleanup(void);

/* Set maximum volume limit at runtime. Returns 0 on success, -1 on error. */
int audio_set_max_volume(double max_volume);

/* Get current maximum volume limit. */
double audio_get_max_volume(void);

/* Set volume to absolute percentage (0.0 - max_volume) */
int audio_set_volume(audio_dev_t dev, double pct);

/* Adjust volume by delta (will be clamped to 0.0 - max_volume) */
int audio_adjust_volume(audio_dev_t dev, double delta);

/* Toggle mute state */
int audio_toggle_mute(audio_dev_t dev);

/* Set mute state explicitly */
int audio_set_mute(audio_dev_t dev, int mute);

/* Get current volume (0.0 - max_volume), -1.0 on error */
double audio_get_volume(audio_dev_t dev);

/* Get current mute state, -1 on error */
int audio_get_mute(audio_dev_t dev);

#endif
