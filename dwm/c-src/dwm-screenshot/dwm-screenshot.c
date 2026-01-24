#define _POSIX_C_SOURCE 200809L

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
#include <time.h>

/* options */
#define DEFAULT_BORDERSIZE 3
#define DEFAULT_COLOR      0xFFEEEEEE /* ARGB Value */
#define DEFAULT_FSCR       0

static const char scrdirpath[]   = "~/Pictures/Screenshots/";
static const char imgextention[] = "png";

/* function defs */
static void  die(const char *fmt, ...);
static char *expandpath(const char *dir);
static int   isdir(const char *path);
static int   mkdir_p(const char *path, const mode_t mode);
static void  parseargs(const int argc, char *argv[], unsigned int *bsz, unsigned int *argb, unsigned int *fscr);
static int   parsergb(const char *s, unsigned int *out);
static int   parseuint(const char *s, unsigned int *out, const unsigned int base);
static char *setfilepath(const char *dir);
static void  usage(void);

static void
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
static char *
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

static int
isdir(const char *path)
{
	struct stat st;
	if (stat(path, &st))
		return 0;
	return S_ISDIR(st.st_mode);
}

static int
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

static int
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

static int
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

static void
usage(void)
{
	fputs("usage: dwm-screenshot [-b bordersize] [-c #RRGGBB] [-o 0xAA] [-h]\n", stderr);
}

static void
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
			*fscr = 0;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage();
			exit(0);
		} else {
			die("invalid argument: '%s'", argv[i]);
		}
	}
}

static char *
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
	     strlen(imgextention) + 1;

	ret = malloc(sz);
	if (!ret)
		die("malloc() failed to allocate %zu bytes:", sz);

	int n = snprintf(ret, sz,
	                 "%sScreenshot-%04d-%02d-%02d-%02d%02d%02d.%s",
	                 dir,
	                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	                 tm.tm_hour, tm.tm_min, tm.tm_sec, imgextention);
	if (n < 0 || (size_t)n >= sz)
		die("snprintf:");

	return ret;
}

static char *
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

static void
argvmaim(char *argv[16], const char *path, const unsigned int bsz, const unsigned int argb, const unsigned int fscr)
{
	char c[44];
	char b[32];
	int i = 0;
	int n;

	/* make color str */
	n = snprintf(c, sizeof(c), "--color=%f,%f,%f,%f",
	             ((argb >> 16) & 0xFFu) / 255.0,
	             ((argb >>  8) & 0xFFu) / 255.0,
	             ((argb >>  0) & 0xFFu) / 255.0,
	             ((argb >> 24) & 0xFFu) / 255.0);
	if (n < 0 || (size_t)n >= sizeof(c))
		die("snprintf:");

	/* make border str */
	n = snprintf(b, sizeof(b), "--bordersize=%u", bsz);
	if (n < 0 || (size_t)n >= sizeof(b))
		die("snprintf:");

	argv[i++] = "maim";
	argv[i++] = "--capturebackground";

	if (!fscr) {
		argv[i++] = "--select";
		argv[i++] = b;
		argv[i++] = c;
	}

	argv[i++] = (char*)path;
	argv[i++] = NULL;
}

int
main(int argc, char *argv[])
{
	char *path;
	char *margv[16];

	unsigned int bsz  = DEFAULT_BORDERSIZE;
	unsigned int argb = DEFAULT_COLOR;
	unsigned int fscr = DEFAULT_FSCR;

	parseargs(argc, argv, &bsz, &argb, &fscr);

	printf("%u, %x\n", bsz, argb);

	path = getpath(scrdirpath);
	if (!path)
		die("getpath() failed");

	puts(path);

	argvmaim(margv, path, bsz, argb, fscr);

	for (int i = 0; margv[i] != NULL; i++)
		printf("%d: %s, ", i, margv[i]);
	putc('\n', stdout);

	free(path);

	return 0;
}
