/* This is an application menu for dmenu.
 * Works by parsing the .desktop files at \usr\share\applications.
 * Arguments are passed through to dmenu, all applications matching the names to
 * $XDG_CONFIG_HOME/dmenu/ignoreapplications will be ignored from being included in the menu
 * plus all hidden and terminal applications. The application writes all entries and crosschecks
 * them every execution at $XDG_CACHE_HOME/dmenu/applicationlist bringing the most recently
 * launched app at the top of the list.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define NAME_ENTRY_LEN 128
#define CMD_ENTRY_LEN 256
#define BUFFER_SIZE 512
#define PATH_SIZE 512

struct appentry {
	char name[NAME_ENTRY_LEN];
	char exec[CMD_ENTRY_LEN];
};

static const char *ignorepath = "/dmenu/ignoreapplications";

static const char *cachepath[] = {"/dmenu", "/applicationlist"};
static const int cachepathsize = 2;

static struct appentry *appentrylist = NULL;
static int  appentrysize = 0;

static char **ignapplist = NULL;
static int  ignappsize = 0;


/* function declerations */
int appentryapp(char *name, char *exec);
int checkdir(char *path);
int checkfilestat(char *path);
int checkhiddenentry(FILE *fp);
int checkignoredentry(char *name);
char *entriesfromfile();
void entriestofile(char *path);
char *entriestostr();
void execcmd(char *cmd);
void freeignapplist();
char *getecommand(char **argv, int argc);
void getentries(char *path);
int getentry(char *path);
int getignapplist();
void getnameexec(char *string, char *ret, int isexec);
void parsestr(char *str);
void prependapp(char *name);
char* printmenu(char *menu, char **argv, int argc);

int
appentryapp(char *name, char *exec)
{
	if (strlen(name) == 0 || strlen(exec) == 0) 
		return -1;
	
	if (strlen(name) > NAME_ENTRY_LEN - 1 || strlen(exec) > CMD_ENTRY_LEN - 1) {
		fprintf(stderr, "String overload at entry with name:%s\n", name);
		exit(EXIT_FAILURE);
	}
	strcpy(appentrylist[appentrysize].name, name);
	strcpy(appentrylist[appentrysize].exec, exec);
	appentrysize++;
	return 1;
}

int
checkdir(char *path)
{
	DIR *dp = opendir(path);
	int ret = -1;

	if (dp) 
		ret = 0;
	else if (errno == ENOENT) 
		ret = 1;

	(void) closedir(dp);
	return ret;
}

int
checkfilestat(char *path)
{
	struct stat path_stat;

	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}

int
checkhiddenentry(FILE *fp)
{
	char buffer[BUFFER_SIZE];

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (strstr(buffer, "NotShowIn")      != NULL ||
		    strstr(buffer, "OnlyShowIn")     != NULL ||
		    strstr(buffer, "NoDisplay=true") != NULL ||
		    strstr(buffer, "Terminal=true")) {
			return 0;
		}
	}
	return 1;
}

int
checkignoredentry(char *name)
{
	for (int i = 0; i < ignappsize; i++) {
		if (strcmp(ignapplist[i], name) == 0)
			return 1;
	}
	return 0;
}

char*
entriesfromfile()
{
	FILE *fp;
	char *cacheenv;
	char *ret;
	char path[PATH_SIZE];
	char buffer[NAME_ENTRY_LEN + 1];

	if ((cacheenv = getenv("XDG_CACHE_HOME")) == NULL) {
		if ((cacheenv = getenv("HOME")) == NULL) {
			perror("Failed to get \"HOME\" environment variable");
			exit(EXIT_FAILURE);
		}
		strcpy(path, cacheenv);
		strcat(path, "/.cache");
	} else {
		strcpy(path, cacheenv);
	}

	for (int i = 0; i < cachepathsize - 1; i++) {
		strcat(path, cachepath[i]);
		switch (checkdir(path)) {
			case -1:
				perror("checkdir error");
				return entriestostr();
			case 1:
				if (mkdir(path, S_IRWXU) == -1) {
					perror("mkdir error");
					return entriestostr();
				}
		}
	}

	strcat(path, cachepath[cachepathsize - 1]);
	entriestofile(path);

	if ((fp = fopen(path, "r")) == NULL) {
		perror("Failed to open applicationlist path");
		exit(EXIT_FAILURE);
	}

	ret = (char*) malloc((NAME_ENTRY_LEN + 1) * appentrysize * sizeof(char));
	ret[0] = '\0';

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		parsestr(buffer);	
		strcat(ret, buffer);
		strcat(ret, "\n");
	}

	fclose(fp);
	return ret;
}

void
entriestofile(char *path)
{
	FILE *fp, *tmp;
	char temppath[PATH_SIZE];
	char buffer[NAME_ENTRY_LEN + 1];
	short int appset[appentrysize];

	if ((fp = fopen(path, "r")) == NULL) {
		fp = fopen(path, "w");
		for (int i = 0; i < appentrysize; i++)
			fprintf(fp, "%s\n", appentrylist[i].name);
		fclose(fp);
		return;
	}

	for (int i = 0; i < appentrysize; i++) {
		appset[i] = 0;
	}

	strcpy(temppath, path);
	strcat(temppath, ".tmp");

	tmp = fopen(temppath, "w");

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (strlen(buffer) == 0)
			continue;
		parsestr(buffer);
		for (int i = 0; i < appentrysize; i++) {
			if (strcmp(appentrylist[i].name, buffer) == 0) {
				fprintf(tmp, "%s\n", appentrylist[i].name);
				appset[i]++;
				continue;
			}
		}
	}

	for (int i = 0; i < appentrysize; i++) {
		if (appset[i] == 0)
			fprintf(tmp, "%s\n", appentrylist[i].name);
	}

	if (appentrylist != NULL)
		free(appentrylist);
	fclose(fp);
	fclose(tmp);

	remove(path);
	rename(temppath, path);
}

char*
entriestostr()
{
	char *ret;
	
	ret = (char*) malloc((NAME_ENTRY_LEN+ 1) * appentrysize * sizeof(char));
	ret[0] = '\0';

	for (int i = 0; i < appentrysize; i++) {
		strcat(ret, appentrylist[i].name);
		strcat(ret, "\n");
	}

	if (appentrylist != NULL)
		free(appentrylist);
	return ret;
}

void
execcmd(char *cmd)
{
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			if (cmd != NULL)
				free(cmd);
			exit(EXIT_FAILURE);

		case 0:
			setsid();
			execl("/bin/sh", "sh", "-c", cmd, NULL);
			if (cmd != NULL)
				free(cmd);
			exit(EXIT_SUCCESS);

		default:
			if (cmd != NULL)
				free(cmd);
	}
}

void
freeignapplist()
{
	if (ignapplist == NULL) {
		ignappsize = 0;
		return;
	}
	for (int i = 0; i < ignappsize; i++) {
		if (ignapplist[i] != NULL)
			free(ignapplist[i]);
	}
	if (ignapplist != NULL)
		free(ignapplist);
	ignappsize = 0;
}

char*
getcommand(char **argv, int argc)
{
	char *menu = entriesfromfile();
	char *name = printmenu(menu, argv, argc);
	char *cmd = NULL;

	if (name == NULL)
		return NULL;
	parsestr(name);

	if (menu != NULL)
		free(menu);

	for (int i = 0; i < appentrysize; i++) {
		if (strcmp(appentrylist[i].name, name) == 0) {
			cmd = (char*) malloc(CMD_ENTRY_LEN * sizeof(char));
			strcpy(cmd, appentrylist[i].exec);
		}
	}
	prependapp(name);
	free(name);
	return cmd;
}

void
getentries(char *path)
{
	DIR *dp;
	struct dirent *ep;
	char filepath[PATH_SIZE];
	int counter = 0;

	if ((dp = opendir(path)) == NULL) {
		fprintf(stderr, "Could not open the '%s' directory", path);
		exit(EXIT_FAILURE);
	}
	while ((ep = readdir(dp)) != NULL)
		counter++;
	rewinddir(dp);

	if (!getignapplist()) {
		perror("Failed to allocate memory or env variables:getignapplist()");
		(void) closedir(dp);
		exit(EXIT_FAILURE);
	}

	appentrylist = (struct appentry*) malloc(counter * sizeof(struct appentry));

	while ((ep = readdir(dp)) != NULL) {
		strcpy(filepath, path);
		strcat(filepath, ep->d_name);

		if (!checkfilestat(filepath))
			continue;
		if (!getentry(filepath)) {
			perror("Failed to allocate memory:getentry()");
			freeignapplist();
			(void) closedir(dp);
			exit(EXIT_FAILURE);
		}
	}

	(void) closedir(dp);
	freeignapplist();
	appentrylist = (struct appentry*) realloc(appentrylist, appentrysize + 1 * sizeof(struct appentry));
}

int
getentry(char *path)
{
	FILE *fp;
	char buffer[BUFFER_SIZE];
	char name[NAME_ENTRY_LEN] = "";
	char exec[CMD_ENTRY_LEN] = "";
	short int found = 0;

	if ((fp = fopen(path, "r")) == NULL)
		return 1;
	if (!checkhiddenentry(fp)) {
		fclose(fp);
		return 1;
	}
	
	fseek(fp, 0, SEEK_SET);
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (strstr(buffer, "Name=") != NULL) {
			if (strstr(buffer, "GenericName=") != NULL)
				continue;
			getnameexec(buffer, name, 0);
			found = 1;
			if (checkignoredentry(name))
				break;
		}
		if (strstr(buffer, "Exec=") != NULL) {
			if (!found) 
				break;
			getnameexec(buffer, exec, 1);
			break;
		}
	}
	fclose(fp);
	return appentryapp(name, exec);
}

int
getignapplist()
{
	FILE *fp;
	char *configenv;
	char path[PATH_SIZE];
	char buffer[BUFFER_SIZE];
	int counter = 0;
	
	if ((configenv = getenv("XDG_CONFIG_HOME")) == NULL) {
		if ((configenv = getenv("HOME")) == NULL) {
			perror("Failed to get \"HOME\" enviroment variable");
			exit(EXIT_FAILURE);
		}
		strcpy(path, configenv);
		strcat(path, "/.config");
	} else {
		strcpy(path, configenv);
	}
	strcat(path, ignorepath);

	if ((fp = fopen(path, "r")) == NULL)
		return -1;

	while (fgets(buffer, sizeof(buffer), fp) != NULL)
		counter++;

	fseek(fp, 0, SEEK_SET);

	if ((ignapplist = (char**) malloc(counter * sizeof(char*))) == NULL)
		return 0;

	for (int i = 0; i < counter; i++) {
		if ((ignapplist[i] = (char*) malloc(NAME_ENTRY_LEN * sizeof(char))) == NULL) {
			freeignapplist();
			return 0;
		}
		ignappsize++;
	}

	counter = 0;
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (counter == ignappsize) {
			perror("More entries found in buffer than expected:getignapplist()");
			break;
		}
		parsestr(buffer);
		if (strlen(buffer) > NAME_ENTRY_LEN - 1) {
			fprintf(stderr, "Name entry '%s' length %ld larger than allowed size, %d bytes\n", buffer, strlen(buffer), NAME_ENTRY_LEN - 1);
			continue;
		}
		strcpy(ignapplist[counter++], buffer);
	}

	if (counter < ignappsize) {
		ignappsize = counter;
		ignapplist = (char**) realloc(ignapplist, ignappsize);
	}

	fclose(fp);
	return 1;
}

void
getnameexec(char *string, char *ret, int isexec)
{
	char *temp;
	char *cptr;

	temp = strchr(string, '=');
	temp ++;
	cptr = strchr(temp, '\n');
	if (cptr != NULL)
		*cptr = '\0';
	if (isexec) {
		cptr = strstr(temp, " %");
		if (cptr != NULL)
			*cptr = '\0';
	}
	strcpy(ret, temp);
}

void
parsestr(char *str)
{
	char *temp = strchr(str, '\n');
	if (temp != NULL)
		*temp='\0';
}

void
prependapp(char *name)
{
	FILE *fp, *tmp;
	char *cacheenv;
	char buffer[NAME_ENTRY_LEN + 1];
	char path[PATH_SIZE];
	char temppath[PATH_SIZE];

	if ((cacheenv = getenv("XDG_CACHE_HOME")) == NULL) {
		if ((cacheenv = getenv("HOME")) == NULL) {
			perror("Failed in getting the \"HOME\" environment variable");
			exit(EXIT_FAILURE);
		}
		strcpy(path, cacheenv);
		strcat(path, "/.cache");
	} else {
		strcpy(path, cacheenv);
	}

	for(int i = 0; i < cachepathsize; i++)
		strcat(path, cachepath[i]);
	strcpy(temppath, path);
	strcat(temppath, ".tmp");

	if((fp = fopen(path, "r")) == NULL) {
		perror("Failed to open applicationlist");
		exit(EXIT_FAILURE);
	}
	tmp = fopen(temppath, "w");
	fprintf(tmp, "%s\n", name);
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		parsestr(buffer);
		if (strcmp(buffer, name) == 0)
			continue;
		fprintf(tmp, "%s\n", buffer);
	}
	fclose(fp);
	fclose(tmp);
	remove(path);
	rename(temppath, path);
}

char*
printmenu(char *menu, char **argv, int argc)
{
	char **cmd;
	char *buffer;
	int writepipe[2];
	int readpipe[2];
	

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}

	buffer = (char*) malloc(NAME_ENTRY_LEN * sizeof(char));
	buffer[0] = '\0';

	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - dmenu */
			cmd = (char**) malloc((argc + 1) * sizeof(char*));
			for (int i = 0; i < argc; i++) {
				cmd[i] = (char*) malloc(128 * sizeof(char));
				if (i > 0 && i != argc + 1)
					strcpy(cmd[i], argv[i]);
			}

			strcpy(cmd[0], "dmenu");
			cmd[argc] = NULL;

			close(writepipe[1]);
			close(readpipe[0]);

			dup2(writepipe[0], STDIN_FILENO);
			close(writepipe[0]);

			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);

			execv("/usr/local/bin/dmenu", cmd);
			exit(EXIT_SUCCESS);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);

			write(writepipe[1], menu, strlen(menu));
			close(writepipe[1]);
			wait(NULL);

			read(readpipe[0], buffer, NAME_ENTRY_LEN * sizeof(char));
			close(readpipe[0]);
	}

	if (buffer[0] == '\0') {
		free(buffer);
		return NULL;
	}

	return buffer;
}

int
main(int argc, char *argv[])
{
	char *cmd;

	getentries("/usr/share/applications/");

	if ((cmd = getcommand(argv, argc)) == NULL)
		return EXIT_SUCCESS;

	execcmd(cmd);	
	return EXIT_SUCCESS;
}
