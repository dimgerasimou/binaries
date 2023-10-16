#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

const char config_path[] = "/.config/xrandr/displaysetup.conf";

typedef struct {
	char id[32];
	char max_mode[32];
	char config_mode[32];
	char pos_arg[16];
	char pos_id[16];
	double max_rate;
	double config_rate;
	bool primary;
} monitor;

void initialize_monitor(monitor *mon) {
	mon->id[0] = '\0';
	mon->max_mode[0] = '\0';
	mon->config_mode[0] = '\0';
	mon->pos_arg[0] = '\0';
	mon->pos_id[0] = '\0';
	mon->max_rate=-1;
	mon->config_rate=-1;
	mon->primary = false;
}

void free_screen(monitor **screen, int screen_size) {
	if (screen == NULL)
		return;
	for (int i = 0; i < screen_size; i++) {
		if (screen[i] != NULL)
			free(screen[i]);
	}
	free(screen);
}

char* strip_whitespace(char *string) {
	char *ptr = string + strlen(string);

	// Strip all trailing control characters and spaces.
	while (*ptr < 33 && ptr != string) {
		*ptr = '\0';
		ptr--;
	}

	ptr = string;

	// Strip all leading control characters and spaces.
	while (*ptr < 33 && *ptr != '\0')
		ptr++;

	return ptr;
}

void get_xrandr_args(monitor ***screen, int *screen_size) {
	FILE *ep;
	char *ptr;
	char buffer[512];

	int moni = 0;
	int index = 0;
	double temp;

	if ((ep = popen("xrandr", "r")) == NULL) {
		fprintf(stderr, "popen() failed with argument \"xrandr\".\n");
		exit(1);
	}

	while(fgets(buffer, sizeof(buffer), ep) != NULL) {
		// If a monitor is connected
		if ((ptr = strstr(buffer, " connected")) != NULL) {

			// Allocate screen and the monitors in heap and initialize to 0.
			if ((*screen = (monitor**) realloc(*screen, (*screen_size + 1) * sizeof(monitor*))) == NULL) {
				fprintf(stderr, "realloc() returned null pointer for screen. Line:%d\n", __LINE__);
				free_screen(*screen, *screen_size);
				exit(-1);
			}

			if (((*screen)[*screen_size] = (monitor*) malloc(sizeof(monitor))) == NULL) {
				fprintf(stderr, "malloc() returned null pointer for screen[%d]. Line:%d\n", *screen_size, __LINE__);
				free_screen(*screen, *screen_size);
				exit(-1);
			}

			initialize_monitor((*screen)[(*screen_size)++]);

			// Parse the name.
			strncpy((*screen)[moni]->id, buffer, ptr - buffer);
			(*screen)[moni]->id[ptr-buffer] = '\0';
			
			if (fgets(buffer, sizeof(buffer), ep) == NULL) {
				fprintf(stderr, "Failed parsing the configuration file. Line:%d\n", __LINE__);
				free_screen(*screen, *screen_size);
				exit(1);
			}
			
			// On the next line the first argument is the max resolution.
			sscanf(buffer, "%s %n", (*screen)[moni]->max_mode, &index);
			ptr = buffer + index;
			
			// Strip non float characters from the input.
			for (int i = 0; i < strlen(ptr); i++) {
				if (ptr[i] != '.' && (ptr[i] < '0' || ptr[i] > '9'))
					ptr[i] = ' ';
			}

			// Recursivly read the float refresh rate and place the max into the struct disp.
			while (sscanf(ptr, "%lf %n", &temp, &index) != EOF) {
				if (temp > (*screen)[moni]->max_rate)
					(*screen)[moni]->max_rate = temp;
				ptr += index;					
			}
			
			moni++;
		}
	}
	pclose(ep);
}

FILE* get_config_stream() {
	char *env;
	char filepath[512];

	if ((env = getenv("XDG_CONFIG_HOME")) == NULL) {
		if ((env = getenv("HOME")) == NULL)
			return NULL;
	}	

	strcpy(filepath, env);
	strcat(filepath, config_path);

	return fopen(filepath, "r");
}

long get_screen_offset(monitor **screen, int screen_size, FILE *fp) {
	char *ptr;
	char buffer[512];
	
	int match_count = 0;
	long offset = -1;
	bool inloop = false;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		ptr = strip_whitespace(buffer);

		// Ignore comments.
		if (ptr[0] == '#')
			continue;

		// Handle section start.
		if (!strcmp(ptr, "Section Screen")) {
			if (inloop) {
				return -1;
			} else {
				offset = ftell(fp);
				inloop = true;
				match_count = 0;
				continue;
			}
		}

		// Handle section end.
		if (!strcmp(ptr, "EndSection")) {
			if (inloop) {
				if (match_count == screen_size)
					return offset;
				inloop = false;
				continue;
			} else {
				return -1;
			}
		}

		// Check if the monitor id matches with a connected one.
		if (strstr(ptr, "Monitor ") == ptr) {
			ptr += 8;
			for (int i = 0; i < screen_size; i++) 
				if (!strcmp(screen[i]->id, ptr))
					match_count++;
		}
	}
	return -2;
}

int get_options(monitor **screen, int screen_size, FILE *fp, long offset) {
	char buffer[512];
	char *ptr;

	int moni = -1;

	if (fseek(fp, offset, SEEK_SET))
		return -1;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		ptr = strip_whitespace(buffer);
		if (ptr[0] == '#')
			continue;

		if (!strcmp(ptr, "EndSection"))
			return 0;
		
		// Get index of the montior, from the connected ones.
		if (strstr(ptr, "Monitor ") == ptr) {
			ptr += 8;
			for (int i = 0; i < screen_size; i++)
				if (!strcmp(screen[i]->id, ptr)) {
					moni = i;
					break;
				}
			continue;
		}

		// Parse arguments.
		if (moni > -1) {
			if (!strcmp(ptr, "Primary")) {
				screen[moni]->primary = true;
				continue;
			}

			if (strstr(ptr, "Position ") == ptr) {
				ptr += 9;
				sscanf(ptr, "%s %s", screen[moni]->pos_arg, screen[moni]->pos_id);
				continue;
			}

			if (strstr(ptr, "Refresh Rate ") == ptr) {
				ptr += 13;
				sscanf(ptr, "%lf", &screen[moni]->config_rate);
				continue;
			}

			if (strstr(ptr, "Resolution ") == ptr) {
				ptr += 11;
				sscanf(ptr, "%s", screen[moni]->config_mode);
				continue;
			}
		}
	}
	return 1;
}

int check_screen_args(monitor **screen, int screen_size) {
	bool has_primary = false;
	bool arg;

	for (int i = 0; i < screen_size; i++) {
		// Check if id, refresh rate and mode are defined.
		if (screen[i]->id[0] == '\0')
			return -1;
		if (screen[i]->config_rate == screen[i]->max_rate && screen[i]->max_rate == -1)
			return -2;
		if (screen[i]->config_mode[0] == screen[i]->max_mode[0] && screen[i]->max_mode[0] == '\0')
			return -3;

		// Check if the position argument and id are correct.
		arg = false;
		if (!strcmp(screen[i]->pos_arg, "right"))
			arg = true;
		else if (!strcmp(screen[i]->pos_arg, "left"))
			arg = true;
		else if (!strcmp(screen[i]->pos_arg, "above"))
			arg = true;
		else if (!strcmp(screen[i]->pos_arg, "below"))
			arg = true;
		else if (!strcmp(screen[i]->pos_arg, "same"))
			arg = true;
		else if (screen[i]->pos_arg[0] =='\0' && screen[i]->primary)
			arg = true;

		if (!arg)
			return -4;

		if (!strcmp(screen[i]->id, screen[i]->pos_id))
				continue;

		arg = false;
		for (int j = 0; j < screen_size; j++)
			if (!strcmp(screen[j]->id, screen[i]->pos_id))
				arg = true;
		
		if (screen[i]->pos_id[0] =='\0' && screen[i]->primary)
			arg = true;

		if (!arg)
			return -5;

		// Check if there is only one primary display.
		if (screen[i]->primary) {
			if (!has_primary) {
				has_primary = true;
			} else {
				return -6;
			}
		}
	}
	return 0;
}

int get_config_args(monitor **screen, int screen_size) {
	FILE *fp;
	long offset;

	if ((fp = get_config_stream()) == NULL)
		return -1;
	
	if ((offset = get_screen_offset(screen, screen_size, fp)) < 0) {
		fclose(fp);
		return -2;
	}

	if (get_options(screen, screen_size, fp, offset)) {
		fclose(fp);
		return -3;
	}

	fclose(fp);
	return 0;
}

char** get_exec_string(monitor **screen, int screen_size) {
	char **exec;
	char temp[64];

	int index = 0;
	int row_size = 9 * screen_size + 2;

	// Allocate exec array.
	if ((exec = (char**) malloc(row_size * sizeof(char*))) == NULL) {
		fprintf(stderr, "malloc() returned null pointer for exec. Line: %d\n", __LINE__);
		free_screen(screen, screen_size);
		exit(-1);
	}

	for (int i = 0; i < row_size; i++) {
		if ((exec[i] = (char*) malloc(32 * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc() returned null pointer for exec[%d]. Line: %d\n", i, __LINE__);
			free_screen(screen, screen_size);
			for (int j = 0; j < i; j++) {
				if (exec[j] != NULL)
					free(exec[j]);
			}
			free(exec);
			exit(-1);
		}
	}
	
	strcpy(exec[index++], "xrandr");

	for (int i = 0; i < screen_size; i++) {
		strcpy(exec[index++], "--output");
		strcpy(exec[index++], screen[i]->id);

		if (screen[i]->config_rate < 0) {
			sprintf(temp, "%.2f", screen[i]->max_rate);
		} else {
			sprintf(temp, "%.2f", screen[i]->config_rate);
		}

		strcpy(exec[index++], "--rate");
		strcpy(exec[index++], temp);

		strcpy(exec[index++], "--mode");

		if (screen[i]->config_mode[0] == '\0') {
			strcpy(exec[index++], screen[i]->max_mode);
		} else {
			strcpy(exec[index++], screen[i]->config_mode);
		}
		
		if (screen[i]->primary) {
			strcpy(exec[index++], "--primary");
		} else {
			if (!strcmp(screen[i]->pos_arg, "right") || !strcmp(screen[i]->pos_arg, "left"))
				sprintf(temp, "--%s-of", screen[i]->pos_arg);
			else
				sprintf(temp, "--%s", screen[i]->pos_arg);
			
			strcpy(exec[index++], temp);
			strcpy(exec[index++], screen[i]->pos_id);
		}
	}
	for (int i = index; i < row_size; i++)
		free(exec[i]);

	exec = (char**) realloc(exec, (index + 1) * sizeof (char*));
	exec[index] = NULL;
	return exec;
}

char** get_auto_exec() {
	char **exec;

	if ((exec = (char**) malloc(3 * sizeof(char*))) == NULL) {
		fprintf(stderr, "malloc() returned null pointer for exec. Line: %d\n", __LINE__);
		exit(-1);
	}
	for (int i = 0; i < 2; i++) {
		if ((exec[i] = (char*) malloc(32 * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc() returned null pointer for exec[%d]. Line: %d\n", i, __LINE__);
			exit(-1);
		}
	}

	strcpy(exec[0], "xrandr");
	strcpy(exec[1], "--auto");
	exec[2] = NULL;
	
	return exec;
}

int main(int argc, char *argv[]) {
	monitor **screen = NULL;
	char **exec;

	int screen_size = 0;

	get_xrandr_args(&screen, &screen_size);
	get_config_args(screen, screen_size);

	if (check_screen_args(screen, screen_size)) {
		free_screen(screen, screen_size);
		exec = get_auto_exec();
	} else {
		exec = get_exec_string(screen, screen_size);
		free_screen(screen, screen_size);
	}

	execv("/bin/xrandr", exec);

	fprintf(stderr, "execv() failed for \"xrandr\". Line:%d", __LINE__);
	return 1;
}
