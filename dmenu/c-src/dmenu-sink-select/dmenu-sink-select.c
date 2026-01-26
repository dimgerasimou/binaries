#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "sink.h"
#include "config.h"

enum { TO_CHILD, FROM_CHILD };
enum { RD, WR };

static const char confpath[] = "dmenu/dmenu-sink-select/ignore-sinks";

static void   die(const char *fmt, ...);
static int    igncheck(FILE *fp, char *s);
static FILE  *ignopen(void);
static char **makeargv(const int argc, char *argv[]);
static int    menu(const char *list, const char *argv[]);
static char  *sinkstostr(sink_list_t *l);
static int    uintlen(unsigned int n);

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno = errno;

	fputs("dmenu-sink-select: ", stderr);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

int
igncheck(FILE *fp, char *s)
{
	char buf[512];

	if (!fp || !s)
		return 0;

	rewind(fp);
	while (fgets(buf, sizeof(buf), fp)) {
		size_t n = strcspn(buf, "\r\n");
		buf[n] = '\0';
		if (!strcmp(buf, s))
			return 1;
	}
	return 0;
}

FILE *
ignopen(void)
{
	FILE *fp;
	const char *env;
	char *path = NULL;


	env = getenv("XDG_CONFIG_HOME");
	if (env) {
		size_t sz;
		int n;

		sz = strlen(env) + 1 + strlen(confpath) + 1;
		path = malloc(sz);
		if (!path)
			die("malloc:");
		
		n = snprintf(path, sz, "%s/%s", env, confpath);

		if (n < 0 || (size_t)n >= sz)
			die("snprintf:");
	} else {
		size_t sz;
		int n;

		env = getenv("HOME");
		if (!env)
			return NULL;

		sz = strlen(env) + strlen("/.config/") + strlen(confpath) + 1;
		path = malloc(sz);
		if (!path)
			die("malloc:");
		
		n = snprintf(path, sz, "%s/.config/%s", env, confpath);

		if (n < 0 || (size_t)n >= sz)
			die("snprintf:");
	}

	fp = fopen(path, "r");
	free(path);
	return fp;
}

char **
makeargv(const int argc, char *argv[])
{
	char **v;
	size_t s = 0;

	/* exec argv: ["dmenu", "-p", <prompt>, <args...>, NULL] */
	v = malloc(((size_t)argc + 3) * sizeof(*v));
	if (!v)
		die("malloc:");

	v[s++] = (char *)menucmd;
	v[s++] = (char *)"-p";
	v[s++] = (char *)"Select the default audio sink:";

	for (int i = 0; i < argc; i++)
		v[s++] = argv[i];
	v[s] = NULL;

	return v;
}

int
menu(const char *list, const char *argv[])
{
	char buf[512];
	int p[2][2];
	pid_t pid;
	ssize_t n;
	int status, ret = -1;

	if (pipe(p[TO_CHILD]) < 0 || pipe(p[FROM_CHILD]) < 0)
		die("pipe:");

	pid = fork();
	if (pid < 0) {
		close(p[TO_CHILD][RD]); close(p[TO_CHILD][WR]);
		close(p[FROM_CHILD][RD]); close(p[FROM_CHILD][WR]);
		die("fork:");
	}

	if (pid == 0) {
		if (dup2(p[TO_CHILD][RD], STDIN_FILENO) < 0)
			_exit(127);
		if (dup2(p[FROM_CHILD][WR], STDOUT_FILENO) < 0)
			_exit(127);

		close(p[TO_CHILD][RD]); close(p[TO_CHILD][WR]);
		close(p[FROM_CHILD][RD]); close(p[FROM_CHILD][WR]);

		execvp(*argv, (char**)argv);
		_exit(127);
	}

	/* parent */
	close(p[TO_CHILD][RD]);
	close(p[FROM_CHILD][WR]);

	(void)write(p[TO_CHILD][WR], list, strlen(list));
	close(p[TO_CHILD][WR]);

	n = read(p[FROM_CHILD][RD], buf, (ssize_t)sizeof(buf) - 1);
	close(p[FROM_CHILD][RD]);

	(void)waitpid(pid, &status, 0);

	if (n <= 0)
		return -1;
	buf[n] = '\0';

	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
		buf[--n] = '\0';

	if (n <= 0)
		return -1;

	char *tab = strchr(buf, '\t');
	if (!tab || tab[1] == '\0')
		return -1;

	tab++;
	char *end;
	errno = 0;
	unsigned long r = strtoul(tab, &end, 10);

	if (errno != 0 || end == tab || *end != '\0' || r > INT_MAX)
		return -1;

	ret = (int)r;
	return ret;
}

char *
sinkstostr(sink_list_t *l)
{
	char *ret;
	size_t sz, pos;
	FILE *fp;

	sz = 1;
	ret = malloc(sz);
	if (!ret)
		die("malloc:");

	ret[0] = '\0';
	pos = 0;

	fp = ignopen();

	for (int i = 0; i < l->count; i++) {
		char *tmp;
		char *s;
		size_t ssz;
		int n;

		if (igncheck(fp, l->sinks[i].desc))
			continue;
		
		ssz = strlen(l->sinks[i].desc) + uintlen(l->sinks[i].idx) + 3;
		
		s = malloc(ssz);
		if (!s)
			die("malloc:");

		n = snprintf(s, ssz, "%s\t%d\n", l->sinks[i].desc, l->sinks[i].idx);

		if (n < 0 || (size_t)n >= ssz)
			die("snprintf:");

		sz += ssz - 1;

		tmp = realloc(ret, sz);
		if (!tmp)
			die("realloc:");
		ret = tmp;

		memcpy(ret + pos, s, (size_t)n);
		pos += (size_t)n;
		ret[pos] = '\0';

		free(s);
	}

	if(fp)
		fclose(fp);
	return ret;
}

int
uintlen(unsigned int n)
{
	int l;

	if (!n)
		return 1;

	for (l = 0; n != 0; l++)
		n /= 10;

	return l;
}

int
main(int argc, char *argv[])
{
	sink_list_t *l;
	char *s;
	char **v;
	int o = -1;

	if (sink_init())
		die("failed to initialize audio");

	l = sink_get_list();
	if (!l)
		die("failed to get sink list");

	s = sinkstostr(l);
	sink_free_list(l);

	v = makeargv(argc - 1, argv + 1);

	o = menu(s, (const char**)v);
	free(s);
	free(v);

	if (o>=0) {
		if (sink_set_default(o))
			die("failed to set default sink");
	}

	sink_cleanup();

	if (*hookargv) {
		pid_t pid = fork();
		
		if (pid < 0)
			die("fork:");

		if (pid == 0) {
			execvp(*hookargv, (char**)hookargv);
			_exit(127);
		}
	}

	return 0;
}