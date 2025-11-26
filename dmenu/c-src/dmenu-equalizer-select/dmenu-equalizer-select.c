/* dmenu-equalizer-select - select easyeffects preset via dmenu
 * depends: dmenu, easyeffects
 * 
 * Copy config.def.h to config.h and edit to customize
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"

static char *argv0;
static char *presets[MAX_PRESETS];
static size_t npresets;

static void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

static char *
estrdup(const char *s)
{
	char *p;

	if (!(p = strdup(s)))
		die("%s: strdup:", argv0);
	return p;
}

static void
cleanup(void)
{
	size_t i;

	for (i = 0; i < npresets; i++)
		free(presets[i]);
	npresets = 0;
}

static void
parsepresets(void)
{
	FILE *fp;
	char buf[BUFSIZE], *p, *tab;
	int inoutput = 0;

	if (!(fp = popen("easyeffects -p", "r")))
		die("%s: popen:", argv0);

	while (fgets(buf, sizeof(buf), fp)) {
		/* trim newline */
		if ((p = strchr(buf, '\n')))
			*p = '\0';

		if (!strcmp(buf, "Output presets:")) {
			inoutput = 1;
			continue;
		}

		if (strstr(buf, "presets:")) {
			inoutput = 0;
			continue;
		}

		if (inoutput && (tab = strchr(buf, '\t'))) {
			if (npresets >= MAX_PRESETS)
				die("%s: too many presets (max %d)", argv0, MAX_PRESETS);
			
			/* skip leading whitespace after tab */
			for (p = tab + 1; *p == ' ' || *p == '\t'; p++)
				;
			
			if (*p)
				presets[npresets++] = estrdup(p);
		}
	}

	pclose(fp);

	if (!npresets)
		die("%s: no output presets found", argv0);
}

static char *
dmenuselect(void)
{
	int pin[2], pout[2], status;
	pid_t pid;
	FILE *fin, *fout;
	char buf[BUFSIZE], *result, *nl;
	size_t i;

	if (pipe(pin) == -1 || pipe(pout) == -1)
		die("%s: pipe:", argv0);

	switch (pid = fork()) {
	case -1:
		die("%s: fork:", argv0);
		break;
	case 0:
		/* child: exec dmenu */
		dup2(pin[0], STDIN_FILENO);
		dup2(pout[1], STDOUT_FILENO);
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		execvp(dmenu[0], (char **)dmenu);
		die("%s: execvp %s:", argv0, dmenu[0]);
		break;
	default:
		/* parent: write presets, read selection */
		close(pin[0]);
		close(pout[1]);

		if (!(fin = fdopen(pin[1], "w")))
			die("%s: fdopen:", argv0);

		for (i = 0; i < npresets; i++)
			fprintf(fin, "%s\n", presets[i]);
		fclose(fin);

		if (!(fout = fdopen(pout[0], "r")))
			die("%s: fdopen:", argv0);

		result = NULL;
		if (fgets(buf, sizeof(buf), fout)) {
			/* trim newline */
			if ((nl = strchr(buf, '\n')))
				*nl = '\0';
			result = estrdup(buf);
		}
		fclose(fout);

		waitpid(pid, &status, 0);

		if (!result || !WIFEXITED(status) || WEXITSTATUS(status)) {
			free(result);
			return NULL;
		}

		return result;
	}

	return result;
}

static void
loadpreset(const char *preset)
{
	char cmd[BUFSIZE];

	snprintf(cmd, sizeof(cmd), "easyeffects -l \"%s\"", preset);
	
	if (system(cmd) == -1)
		die("%s: system:", argv0);
}

static void
usage(void)
{
	die("usage: %s", argv0);
}

int
main(int argc, char *argv[])
{
	char *selection;

	argv0 = argv[0];

	if (argc != 1)
		usage();

	parsepresets();
	
	if (!(selection = dmenuselect())) {
		cleanup();
		return 0;
	}

	loadpreset(selection);
	
	free(selection);
	cleanup();

	return 0;
}
