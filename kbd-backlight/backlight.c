#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>

#include "backlight.h"
#include "config.h"

static void ensure_parent_dir(const char *path)
{
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *dir = dirname(tmp);

    /* mkdir -p, one component at a time */
    for (char *p = dir + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(dir, 0755);
            *p = '/';
        }
    }
    mkdir(dir, 0755);
}

void state_file_path(char *path, size_t bufsz)
{
    const char *cache = getenv("XDG_CACHE_HOME");
    const char *home = getenv("HOME");

    if (cache && cache[0] != '\0') {
        snprintf(path, bufsz, "%s/%s", cache, STATE_FILE_NAME);
    } else if (home && home[0] != '\0') {
        snprintf(path, bufsz, "%s/.cache/%s", home, STATE_FILE_NAME);
    } else {
        snprintf(path, bufsz, "/tmp/%s", STATE_FILE_NAME);
    }
}

static level_t parse_level(const char *s)
{
    if (strcmp(s, "medium") == 0) return LEVEL_MEDIUM;
    if (strcmp(s, "full") == 0) return LEVEL_FULL;
    if (strcmp(s, "custom") == 0) return LEVEL_CUSTOM;
    return LEVEL_OFF;
}

state_t load_state(void)
{
    state_t st;
    st.level = LEVEL_OFF;
    snprintf(st.color, sizeof(st.color), "%s", DEFAULT_COLOR);
    snprintf(st.brightness, sizeof(st.brightness), "%s", BRIGHTNESS_FULL);

    char path[PATH_MAX];
    state_file_path(path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return st; /* no state file yet - defaults are fine */

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (strncmp(line, "level=", 6) == 0) {
            st.level = parse_level(line + 6);
        } else if (strncmp(line, "color=", 6) == 0) {
            char tmp[7] = {0};
            strncpy(tmp, line + 6, 6);
            snprintf(st.color, sizeof(st.color), "%s", tmp);
        } else if (strncmp(line, "brightness=", 11) == 0) {
            char tmp[4] = {0};
            strncpy(tmp, line + 11, 3);
            snprintf(st.brightness, sizeof(st.brightness), "%s", tmp);
        }
    }

    fclose(f);
    return st;
}

int save_state(const state_t *st)
{
    char path[PATH_MAX];
    state_file_path(path, sizeof(path));
    ensure_parent_dir(path);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "kbd-backlight: could not write state file %s\n", path);
        return -1;
    }

    fprintf(f, "level=%s\ncolor=%s\nbrightness=%s\n",
            level_name(st->level), st->color, st->brightness);
    fclose(f);
    return 0;
}

const char *level_name(level_t level)
{
    switch (level) {
        case LEVEL_MEDIUM: return "medium";
        case LEVEL_FULL:   return "full";
        case LEVEL_CUSTOM: return "custom";
        default:           return "off";
    }
}

level_t next_level(level_t level)
{
    switch (level) {
        case LEVEL_OFF:    return LEVEL_MEDIUM;
        case LEVEL_MEDIUM: return LEVEL_FULL;
        case LEVEL_FULL:   return LEVEL_OFF;
        default:           return LEVEL_OFF; /* LEVEL_CUSTOM -> off */
    }
}

/* Runs openrgb with the given argv (NULL-terminated). Its stdout/stderr are
 * captured through a pipe rather than left connected to our terminal -
 * openrgb's own status spam is discarded on success, and only surfaced if
 * the call actually fails, so it doesn't garble whatever's on screen. */
static int run_openrgb(char *const argv[])
{
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(OPENRGB_BIN, argv);
        fprintf(stderr, "execvp openrgb: %s\n", strerror(errno));
        _exit(127);
    }

    close(pipefd[1]);

    char buf[4096];
    size_t used = 0;
    ssize_t n;
    while (used < sizeof(buf) - 1 &&
           (n = read(pipefd[0], buf + used, sizeof(buf) - 1 - used)) > 0) {
        used += (size_t)n;
    }
    buf[used] = '\0';
    close(pipefd[0]);

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) return 0;

    fprintf(stderr, "kbd-backlight: openrgb failed (status %d)\n",
            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    if (used > 0) fprintf(stderr, "%s", buf);
    return -1;
}

int apply_off(void)
{
    char *argv[] = {
        (char *)OPENRGB_BIN, "--device", (char *)DEVICE_INDEX,
        "--mode", "off", NULL
    };
    return run_openrgb(argv);
}

int apply_static(const char *color, const char *brightness)
{
    char *argv[] = {
        (char *)OPENRGB_BIN, "--device", (char *)DEVICE_INDEX,
        "--mode", "static",
        "--color", (char *)color,
        "--brightness", (char *)brightness,
        NULL
    };
    return run_openrgb(argv);
}
