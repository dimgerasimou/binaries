#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "backlight.h"
#include "config.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s {off|medium|full|custom|toggle|status} [OPTIONS]\n"
        "\n"
        "  off|medium|full   set backlight to that level\n"
        "  custom            set an arbitrary brightness (requires --brightness)\n"
        "                    not part of the toggle rotation\n"
        "  toggle            cycle off -> medium -> full -> off\n"
        "                    (skips custom even if currently in it)\n"
        "  status            print current level, color, and brightness\n"
        "\n"
        "  --color RRGGBB     set/save the color used for medium/full/custom\n"
        "                     (can be combined with any command)\n"
        "  --brightness N     0-100, required for 'custom', saved to state\n",
        prog);
}

static int is_valid_hex_color(const char *s)
{
    if (strlen(s) != 6) return 0;
    for (int i = 0; i < 6; i++) {
        if (!isxdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

static int is_valid_brightness(const char *s, int *out)
{
    if (!s[0]) return 0;
    for (const char *p = s; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    long v = strtol(s, NULL, 10);
    if (v < 0 || v > 100) return 0;
    *out = (int)v;
    return 1;
}

static void to_upper_inplace(char *s)
{
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

int main(int argc, char *argv[])
{
    const char *command = NULL;
    const char *color_arg = NULL;
    const char *brightness_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--color") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "kbd-backlight: --color requires an argument\n");
                usage(argv[0]);
                return 1;
            }
            color_arg = argv[++i];
        } else if (strcmp(argv[i], "--brightness") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "kbd-backlight: --brightness requires an argument\n");
                usage(argv[0]);
                return 1;
            }
            brightness_arg = argv[++i];
        } else if (command == NULL) {
            command = argv[i];
        } else {
            fprintf(stderr, "kbd-backlight: unexpected argument '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (!command) {
        usage(argv[0]);
        return 1;
    }

    char color_buf[7];
    if (color_arg) {
        const char *c = color_arg;
        if (c[0] == '#') c++;
        if (!is_valid_hex_color(c)) {
            fprintf(stderr, "kbd-backlight: invalid --color '%s' (expected RRGGBB hex)\n",
                    color_arg);
            return 1;
        }
        snprintf(color_buf, sizeof(color_buf), "%s", c);
        to_upper_inplace(color_buf);
    }

    int brightness_val = 0;
    if (brightness_arg && !is_valid_brightness(brightness_arg, &brightness_val)) {
        fprintf(stderr,
            "kbd-backlight: invalid --brightness '%s' (expected 0-100)\n",
            brightness_arg);
        return 1;
    }

    if (brightness_arg && strcmp(command, "custom") != 0) {
        fprintf(stderr,
            "kbd-backlight: --brightness is only valid with 'custom'\n");
        return 1;
    }

    if (strcmp(command, "custom") == 0 && !brightness_arg) {
        fprintf(stderr, "kbd-backlight: 'custom' requires --brightness\n");
        return 1;
    }

    state_t st = load_state();
    if (color_arg) {
        snprintf(st.color, sizeof(st.color), "%s", color_buf);
    }

    int rc = 0;

    if (strcmp(command, "off") == 0) {
        st.level = LEVEL_OFF;
        rc = apply_off();
    } else if (strcmp(command, "medium") == 0) {
        st.level = LEVEL_MEDIUM;
        snprintf(st.brightness, sizeof(st.brightness), "%s", BRIGHTNESS_MEDIUM);
        rc = apply_static(st.color, st.brightness);
    } else if (strcmp(command, "full") == 0) {
        st.level = LEVEL_FULL;
        snprintf(st.brightness, sizeof(st.brightness), "%s", BRIGHTNESS_FULL);
        rc = apply_static(st.color, st.brightness);
    } else if (strcmp(command, "custom") == 0) {
        st.level = LEVEL_CUSTOM;
        snprintf(st.brightness, sizeof(st.brightness), "%d", brightness_val);
        rc = apply_static(st.color, st.brightness);
    } else if (strcmp(command, "toggle") == 0) {
        st.level = next_level(st.level);
        switch (st.level) {
            case LEVEL_OFF:
                rc = apply_off();
                break;
            case LEVEL_MEDIUM:
                snprintf(st.brightness, sizeof(st.brightness), "%s", BRIGHTNESS_MEDIUM);
                rc = apply_static(st.color, st.brightness);
                break;
            case LEVEL_FULL:
                snprintf(st.brightness, sizeof(st.brightness), "%s", BRIGHTNESS_FULL);
                rc = apply_static(st.color, st.brightness);
                break;
            default:
                break; /* unreachable: next_level() never returns LEVEL_CUSTOM */
        }
    } else if (strcmp(command, "status") == 0) {
        printf("level: %s\ncolor: %s\nbrightness: %s\n",
               level_name(st.level), st.color, st.brightness);
    } else {
        fprintf(stderr, "kbd-backlight: unknown command '%s'\n", command);
        usage(argv[0]);
        return 1;
    }

    if (rc != 0) {
        /* openrgb call failed - don't persist a state that doesn't match reality */
        return 1;
    }

    save_state(&st);
    return 0;
}
