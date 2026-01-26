#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <libnotify/notify.h>

/* options */
#define DEFAULT_BORDERSIZE 3
#define DEFAULT_COLOR      0xFFEEEEEE /* ARGB Value */
#define DEFAULT_FSCR       0

static const char scrdirpath[]   = "~/Pictures/Screenshots/";
static const char imgextension[] = "png";

/* function defs */
static void  apparg(const char *argv[], const char *arg, size_t *i, const size_t argc);
static void  argvmaim(const char *argv[], const size_t argc, const char *path, const unsigned int bsz, const unsigned int argb, const unsigned int fscr);
static void  die(const char *fmt, ...);
static char *expandpath(const char *dir);
static int   fexecvp(const char *argv[]);
static char *getpath(const char *dir);
static int   isdir(const char *path);
static int   mkdir_p(const char *path, const mode_t mode);
static int   notify(void);
static void  parseargs(const int argc, char *argv[], unsigned int *bsz, unsigned int *argb, unsigned int *fscr);
static int   parsergb(const char *s, unsigned int *out);
static int   parseuint(const char *s, unsigned int *out, const unsigned int base);
static char *setfilepath(const char *dir);
static void  usage(void);

void
apparg(const char *argv[], const char *arg, size_t *i, const size_t argc)
{
	if (*i >= argc)
		die("maim argv overflow");
	
	argv[(*i)++] = arg;
}

void
argvmaim(const char *argv[], const size_t argc, const char *path, 
         const unsigned int bsz, const unsigned int argb, const unsigned int fscr)
{
	static char c[44];
	static char b[32];
	size_t i = 0;
	int n;

	n = snprintf(c, sizeof(c), "--color=%.6f,%.6f,%.6f,%.6f",
	             ((argb >> 16) & 0xFFu) / 255.0,
	             ((argb >>  8) & 0xFFu) / 255.0,
	             ((argb >>  0) & 0xFFu) / 255.0,
	             ((argb >> 24) & 0xFFu) / 255.0);
	if (n < 0 || (size_t)n >= sizeof(c))
		die("snprintf:");

	n = snprintf(b, sizeof(b), "--bordersize=%u", bsz);
	if (n < 0 || (size_t)n >= sizeof(b))
		die("snprintf:");

	apparg(argv, "maim", &i, argc);
	apparg(argv, "--quiet", &i, argc);
	apparg(argv, "--capturebackground", &i, argc);

	if (!fscr) {
		apparg(argv, "--select", &i, argc);
		apparg(argv, b, &i, argc);
		apparg(argv, c, &i, argc);
	}

	apparg(argv, path, &i, argc);
	argv[i] = NULL;
}

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno = errno;

	fputs("dwm-screenshot: ", stderr);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

/* expand ~, ~/, $ENV, ${ENV} in a POSIX-like way */
char *
expandpath(const char *dir)
{
	const size_t chunk = 256;
	char *ret;
	size_t sz = 0;
	size_t ir = 0;
	size_t id = 0;

	if (!dir)
		die("NULL ptr");

	/* expand tilde (~ or ~/) */
	if (dir[0] == '~' && (dir[1] == '\0' || dir[1] == '/')) {
		const char *env = getenv("HOME");
		if (!env || !*env)
			die("\"HOME\" env is not set");

		size_t l = strlen(env);

		ret = strdup(env);
		if (!ret)
			die("strdup:");

		sz = l + 1;
		ir = l;
		id++;
	} else {
		sz = chunk;
		ret = malloc(sz);
		if (!ret)
			die("malloc() failed to allocate %zu bytes:", sz);

		ret[0] = '\0';
	}

	for (; dir[id] != '\0'; ) {
		/* normal character */
		if (dir[id] != '$') {
			if (ir + 1 >= sz) {
				sz += chunk;
				char *tmp = realloc(ret, sz);
				if (!tmp)
					die("realloc() failed to allocate %zu bytes:", sz);
				ret = tmp;
			}
			ret[ir++] = dir[id++];
			ret[ir] = '\0';
			continue;
		}

		/* '$' expansion */
		id++;
		if (dir[id] == '\0') {
			/* trailing '$' -> literal */
			if (ir + 1 >= sz) {
				sz += chunk;
				char *tmp = realloc(ret, sz);
				if (!tmp)
					die("realloc() failed to allocate %zu bytes:", sz);
				ret = tmp;
			}
			ret[ir++] = '$';
			ret[ir] = '\0';
			break;
		}

		const char *vstart;
		size_t vsz = 0;

		if (dir[id] == '{') {
			/* ${VAR} */
			id++;
			vstart = dir + id;

			while (dir[id] != '\0' && dir[id] != '}') {
				id++;
				vsz++;
			}

			if (dir[id] != '}')
				die("invalid path format");
			id++; /* consume '}' */
		} else {
			/* $VAR, where VAR is [A-Za-z_][A-Za-z0-9_]* */
			if (!(isalpha((unsigned char)dir[id]) || dir[id] == '_')) {
				/* not a valid var start -> treat '$' literally */
				if (ir + 1 >= sz) {
					sz += chunk;
					char *tmp = realloc(ret, sz);
					if (!tmp)
						die("realloc() failed to allocate %zu bytes:", sz);
					ret = tmp;
				}
				ret[ir++] = '$';
				ret[ir] = '\0';
				continue;
			}

			vstart = dir + id;
			while (dir[id] != '\0' &&
			       (isalnum((unsigned char)dir[id]) || dir[id] == '_')) {
				id++;
				vsz++;
			}
		}

		char *v = malloc(vsz + 1);
		if (!v)
			die("malloc() failed to allocate %zu bytes:", vsz + 1);

		memcpy(v, vstart, vsz);
		v[vsz] = '\0';

		const char *env = getenv(v);
		if (!env || !*env)
			die("\"%s\" env variable is not set", v);

		free(v);

		size_t l = strlen(env);

		if (ir + l + 1 > sz) {
			while (ir + l + 1 > sz)
				sz += chunk;
			char *tmp = realloc(ret, sz);
			if (!tmp)
				die("realloc() failed to allocate %zu bytes:", sz);
			ret = tmp;
		}

		memcpy(ret + ir, env, l);
		ir += l;
		ret[ir] = '\0';
	}

	char *tmp = realloc(ret, ir + 1);
	if (!tmp)
		die("realloc() failed to allocate %zu bytes:", ir + 1);

	return tmp;
}

int
fexecvp(const char *argv[])
{
	pid_t pid;
	int s;

	pid = fork();
	if (pid < 0)
		die("fork:");
	if (!pid) {
		/* child */
		execvp(*argv, (char **)argv);
		perror("execvp");
		_exit(127);
	}

	do {
		if (waitpid(pid, &s, 0) < 0) {
			if (errno == EINTR)
				continue;
			return 1;
		}
		break;
	} while (1);

	if (WIFEXITED(s))
		return WEXITSTATUS(s);
	return 128 + WTERMSIG(s);
}

char *
getpath(const char *dir)
{
	char *dirpath;
	char *path;

	dirpath = expandpath(dir);
	if (!dirpath)
		return NULL;

	if (mkdir_p(dirpath, 0755))
		die("failed to create directory: %s", dirpath);

	path = setfilepath(dirpath);
	free(dirpath);

	return path;
}

int
isdir(const char *path)
{
	struct stat st;
	if (stat(path, &st))
		return 0;
	return S_ISDIR(st.st_mode);
}

int
mkdir_p(const char *path, const mode_t mode)
{
	char *buf = NULL;
	size_t len;

	if (!path || !*path) {
		errno = EINVAL;
		return -1;
	}

	len = strlen(path);

	buf = malloc(len + 1);
	if (!buf)
		return -1;

	memcpy(buf, path, len + 1);

	/* strip trailing slashes */
	while (len > 1 && buf[len - 1] == '/') {
		buf[len - 1] = '\0';
		len--;
	}

	for (char *p = buf + 1; *p; p++) {
		if (*p != '/')
			continue;

		*p = '\0';

		if (buf[0] != '\0') {
			if (mkdir(buf, mode)) {
				if (errno != EEXIST || !isdir(buf)) {
					*p = '/';
					free(buf);
					errno = ENOTDIR;
					return -1;
				}
			}
		}

		*p = '/';
		while (p[1] == '/')
			p++;
	}

	if (mkdir(buf, mode)) {
		if (errno != EEXIST || !isdir(buf)) {
			free(buf);
			errno = ENOTDIR;
			return -1;
		}
	}

	free(buf);
	return 0;
}

int
notify(void)
{
	NotifyNotification *n;

	if (!notify_init("dwm-screenshot"))
		return 1;

	n = notify_notification_new(" dwm-screenshot", "Screenshot taken", "display");
	if (!n) {
		notify_uninit();
		return 1;
	}

	notify_notification_set_urgency(n, NOTIFY_URGENCY_LOW);
	notify_notification_set_timeout(n, 5000);

	if (!notify_notification_show(n, NULL)) {
		g_object_unref(n);
		notify_uninit();
		return 1;
	}

	g_object_unref(n);
	notify_uninit();
	return 0;
}

int
parsergb(const char *s, unsigned int *out)
{
	char *end;
	unsigned long v;

	/* exactly "#RRGGBB" */
	if (!s || *(s++) != '#' || !out || strlen(s) != 6)
		return -1;

	errno = 0;
	v = strtoul(s, &end, 16);

	if (*end != '\0' || errno == ERANGE || v > 0xFFFFFFUL)
		return -1;

	*out = (unsigned int)v;
	return 0;
}

int
parseuint(const char *s, unsigned int *out, const unsigned int base)
{
	char *end;
	unsigned long v;

	if (!s || !*s || !out)
		return -1;

	errno = 0;
	v = strtoul(s, &end, base);

	if (errno != 0 || end == s || *end != '\0' || v > UINT_MAX)
		return -1;

	*out = (unsigned int)v;
	return 0;
}

void
usage(void)
{
	fputs("usage: dwm-screenshot [-b bordersize] [-c #RRGGBB] [-o 0xAA] [-fh]\n", stderr);
}

void
parseargs(const int argc, char *argv[], unsigned int *bsz, unsigned int *argb, unsigned int *fscr)
{
	unsigned int v;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-b")) {
			if (++i >= argc)
				die("missing argument for %s", argv[--i]);
			if (parseuint(argv[i], &v, 10))
				die("invalid argument for %s", argv[--i]);
			*bsz = v;
		} else if (!strcmp(argv[i], "-c")) {
			if (++i >= argc)
				die("missing argument for %s", argv[--i]);
			if (parsergb(argv[i], &v))
				die("invalid argument for %s", argv[--i]);
			*argb = (*argb & 0xFF000000u) | v;
		} else if (!strcmp(argv[i], "-o")) {
			if (++i >= argc)
				die("missing argument for %s", argv[--i]);
			if (parseuint(argv[i], &v, 16))
				die("invalid argument for %s", argv[--i]);
			if (v > 0xFFu)
				die("invalid argument for -o (expected 0x00..0xFF)");
			*argb = (*argb & 0x00FFFFFFu) | (v << 24);
		} else if (!strcmp(argv[i], "-f")) {
			*fscr = 1;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage();
			exit(0);
		} else {
			die("invalid argument: '%s'", argv[i]);
		}
	}
}

char *
setfilepath(const char *dir)
{
	time_t now = time(NULL);
	struct tm tm;
	char *ret;
	size_t sz;

	if (!dir)
		die("NULL ptr");

	if (now == (time_t)-1 || !localtime_r(&now, &tm))
		die("failed to populate struct tm:");

	sz = strlen(dir) + strlen("Screenshot-0000-00-00-000000.") +
	     strlen(imgextension) + 1;

	ret = malloc(sz);
	if (!ret)
		die("malloc() failed to allocate %zu bytes:", sz);

	int n = snprintf(ret, sz,
	                 "%sScreenshot-%04d-%02d-%02d-%02d%02d%02d.%s",
	                 dir,
	                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	                 tm.tm_hour, tm.tm_min, tm.tm_sec, imgextension);
	if (n < 0 || (size_t)n >= sz)
		die("snprintf:");

	return ret;
}

int
main(int argc, char *argv[])
{
	char *path;
	const char *margv[16];
	int n;

	unsigned int bsz  = DEFAULT_BORDERSIZE;
	unsigned int argb = DEFAULT_COLOR;
	unsigned int fscr = DEFAULT_FSCR;

	parseargs(argc, argv, &bsz, &argb, &fscr);

	path = getpath(scrdirpath);
	if (!path)
		die("getpath() failed");

	argvmaim(margv, sizeof(margv) / sizeof(margv[0]), path, bsz, argb, fscr);

	n = fexecvp(margv);
	free(path);

	if (!n) {
		if (notify())
			die("notify failed");
	}

	return n;
}
