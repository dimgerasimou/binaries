#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* menu command; extra args passed on the command line are forwarded to it */
static const char menucmd[] = "dmenu";

#define BUF_SIZE 256

typedef struct {
	char **name;
	size_t len;
} Presets;

_Noreturn static void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno = errno;

	fprintf(stderr, "eq-select: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

static char *
estrdup(const char *s)
{
	char *p;

	if (!(p = strdup(s)))
		die("strdup:");
	return p;
}

static void *
erealloc(void *ptr, const size_t size)
{
	void *p;

	if ((p = realloc(ptr, size)) == NULL)
		die("realloc:");
	return p;
}

static void *
emalloc(const size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		die("malloc:");
	return p;
}

static int
easyeffectsrunning(void)
{
	int status;

	status = system("pgrep -x easyeffects >/dev/null 2>&1");
	if (status == -1)
		die("system:");

	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void
presetsread(Presets *p)
{
	FILE *ep;
	char b[BUF_SIZE];
	int status;

	ep = popen("easyeffects --presets", "r");
	if (!ep)
		die("popen: easyeffects --presets:");

	while (fgets(b, sizeof(b), ep)) {
		char *ptr;

		if ((ptr = strchr(b, '\n')))
			*ptr = '\0';

		if (!strcmp(b, ""))
			break;

		if (!strcmp(b, "Output presets:"))
			continue;

		ptr = strchr(b, '\t');
		if (!ptr)
			break;

		p->len++;
		ptr++;

		p->name = erealloc(p->name, p->len * sizeof(char*));
		p->name[p->len-1] = estrdup(ptr);
	}

	status = pclose(ep);
	if (status == -1)
		die("pclose:");
	if (!WIFEXITED(status) || WEXITSTATUS(status))
		die("easyeffects --presets: exited with an error");
}

static char *
presetsel(Presets *p, int argc, char *argv[])
{
	FILE *fin, *fout;
	int pin[2], pout[2], status;
	pid_t pid;
	char b[BUF_SIZE], *r = NULL;

	const char **args = emalloc((argc + 3) * sizeof(char*));

	args[0] = menucmd;
	args[1] = "-p";
	args[2] = "Select equalizer preset:";
	for (int i = 1; i < argc; i++)
		args[i + 2] = argv[i];
	args[argc+2] = NULL;

	if (pipe(pin) < 0 || pipe(pout) < 0)
		die("pipe:");

	switch (pid = fork()) {
	case -1:
		die("fork:");

	case 0:
		close(pout[0]);
		close(pin[1]);

		if (dup2(pin[0], STDIN_FILENO) < 0 || dup2(pout[1], STDOUT_FILENO) < 0)
			_exit(127);

		close(pout[1]);
		close(pin[0]);

		execvp(args[0], (char**) args);
		_exit(127);

	default:
		break;
	}

	close(pin[0]);
	close(pout[1]);

	if (!(fin = fdopen(pin[1], "w")) ||
	    !(fout = fdopen(pout[0], "r")))
		die("fdopen:");

	for (size_t i = 0; i < p->len; i++) {
		if (fprintf(fin, "%s\n", p->name[i]) < 0)
			break;
	}
	fclose(fin);
	free(args);

	if (fgets(b, sizeof(b), fout)) {
		char *ptr = strchr(b, '\n');
		if (ptr)
			*ptr = '\0';
		r = estrdup(b);
	}

	fclose(fout);

	while (waitpid(pid, &status, 0) < 0 && errno == EINTR);

	if (!r || !WIFEXITED(status) || WEXITSTATUS(status)) {
		if (r)
			free(r);
		return NULL;
	}

	return r;
}

static void
presetload(const char *preset)
{
	pid_t pid;

	switch (pid = fork()) {
	case -1:
		die("fork:");
	
	case 0:
		execlp("easyeffects", "easyeffects", "--load-preset", preset, NULL);
		_exit(127);
	
	default:
		break;
	}
}

int
main(int argc, char *argv[])
{
	Presets p = {0};
	char *sel;

	signal(SIGPIPE, SIG_IGN);

	if (!easyeffectsrunning())
		die("easyeffects is not running, start it first");

	presetsread(&p);

	sel = presetsel(&p, argc, argv);

	for (size_t i = 0; i < p.len; i++)
		free(p.name[i]);
	free(p.name);

	if (!sel)
		return 0;

	presetload(sel);

	free(sel);
	return 0;
}
