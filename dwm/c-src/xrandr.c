#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

const char config_path[] = "/.config/xrandr/displaysetup.conf";

typedef struct {
	char id[32];
	char res[32];
	char conf_res[32];
	char pos_arg[16];
	char pos_id[16];
	double max_f;
	double conf_f;
	short primary;
} monitor;

monitor *screen;
int screen_size;

void initialize_monitor(monitor *mon) {
	mon->id[0] = '\0';
	mon->res[0] = '\0';
	mon->conf_res[0] = '\0';
	mon->pos_arg[0] = '\0';
	mon->pos_id[0] = '\0';
	mon->max_f=-1;
	mon->conf_f=-1;
	mon->primary = 0;
}

void get_xrandr_args() {
	FILE *ep;
	char *ptr;
	char buffer[512];
	char temp_buffer[512];
	double temp_f;
	int moni = 0;
	int index = 0;

	if ((ep = popen("xrandr", "r")) == NULL) {
		perror("Failed in executing xrandr cmd");
		exit(1);
	}

	while(fgets(buffer, sizeof(buffer), ep) != NULL) {
		// If a monitor is connected
		if ((ptr = strstr(buffer, " connected")) != NULL) {
			// Allocate mon in heap.
			screen_size++;
			screen = (monitor*) realloc(screen, screen_size * sizeof(monitor));
			initialize_monitor(&screen[screen_size - 1]);

			// Parse the name.
			strncpy(screen[moni].id, buffer, ptr - buffer);
			screen[moni].id[ptr-buffer] = '\0';
			
			if (fgets(buffer, sizeof(buffer), ep) == NULL) {
				perror("Error in parsing the file");
				exit(1);
			}

			// On the next line the first argument is the max resolution.
			sscanf(buffer, "%s %[^\n]", screen[moni].res, temp_buffer);
			ptr = temp_buffer;

			// Strip non float characters from the input.
			for (int i = 0; i < strlen(ptr); i++) {
				if (ptr[i] != '.' && (ptr[i] < '0' || ptr[i] > '9'))
					ptr[i] = ' ';
			}

			// Recursivly read the float refresh rate and place the max into the struct disp.
			while (sscanf(ptr, "%lf %n", &temp_f, &index) != EOF) {
				if (temp_f > screen[moni].max_f)
					screen[moni].max_f = temp_f;
				ptr += index;					
			}
			moni++;
		}
	}
	pclose(ep);
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

FILE* get_config_stream() {
	char filepath[512];
	char *env;

	if ((env = getenv("HOME")) == NULL)
		return NULL;

	strcpy(filepath, env);
	strcat(filepath, config_path);

	return fopen(filepath, "r");
}

long get_screen_offset(FILE *fp) {
	char buffer[512];
	char *ptr;
	int match_count = 0;
	long offset = -1;
	bool inloop = false;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		ptr = strip_whitespace(buffer);
		if (ptr[0] == '#')
			continue;
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

		if (strstr(ptr, "Monitor ") == ptr) {
			ptr += 8;
			for (int i = 0; i < screen_size; i++)
				if (!strcmp(screen[i].id, ptr))
					match_count++;
		}
	}
	return -2;
}

int get_options(FILE *fp, long offset) {
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

		if (strstr(ptr, "Monitor ") == ptr) {
			ptr += 8;
			for (int i = 0; i < screen_size; i++)
				if (!strcmp(screen[i].id, ptr)) {
					moni = i;
					break;
				}
			continue;
		}

		if (moni > -1) {
			if (!strcmp(ptr, "Primary")) {
				screen[moni].primary = 1;
				continue;
			}

			if (strstr(ptr, "Position ") == ptr) {
				ptr += 9;
				sscanf(ptr, "%s %s", screen[moni].pos_arg, screen[moni].pos_id);
				continue;
			}

			if (strstr(ptr, "Refresh Rate ") == ptr) {
				ptr += 13;
				sscanf(ptr, "%lf", &screen[moni].conf_f);
				continue;
			}

			if (strstr(ptr, "Resolution ") == ptr) {
				ptr += 11;
				sscanf(ptr, "%s", screen[moni].conf_res);
				continue;
			}
		}
	}
	return 1;
}

int check_screen_args() {
	bool has_primary = false;
	for (int i = 0; i < screen_size; i++) {
		if (screen[i].id[0] == '\0')
			return -1;
		if (screen[i].conf_f == screen[i].max_f && screen[i].max_f == -1)
			return -2;
		if (screen[i].conf_res[0] == screen[i].res[0] && screen[i].res[0] == '\0')
			return -3;

		bool arg = false;
		if (!strcmp(screen[i].pos_arg, "right"))
			arg = true;
		else if (!strcmp(screen[i].pos_arg, "left"))
			arg = true;
		else if (!strcmp(screen[i].pos_arg, "above"))
			arg = true;
		else if (!strcmp(screen[i].pos_arg, "below"))
			arg = true;
		else if (!strcmp(screen[i].pos_arg, "same"))
			arg = true;
		else if (screen[i].pos_arg[0] =='\0' && screen[i].primary)
			arg = true;
		if (!arg)
			return -4;

		if (!strcmp(screen[i].id, screen[i].pos_id))
				continue;
		arg = false;
		for (int j = 0; j < screen_size; j++)
			if (!strcmp(screen[j].id, screen[i].pos_id))
				arg = true;
		if (screen[i].pos_id[0] =='\0' && screen[i].primary)
			arg = true;
		if (!arg)
			return -5;

		if (screen[i].primary > 1 || screen[i].primary < 0)
			return -6;
		if (screen[i].primary) {
			if (!has_primary) {
				has_primary = 1;
			} else {
				return -7;
			}
		}
	}
	return 0;
}

int get_config_args() {
	FILE *fp;
	long offset;

	if ((fp = get_config_stream()) == NULL)
		return -1;

	if ((offset = get_screen_offset(fp)) < 0) {
		fclose(fp);
		return -2;
	}
	if (get_options(fp, offset)) {
		fclose(fp);
		return -3;
	}

	fclose(fp);
	return 0;
}

void get_exec_string(char **exec, int row_size) {
	int index = 0;
	char temp[64];
	strcpy(exec[index++], "xrandr");
	for (int i = 0; i < screen_size; i++) {
		strcpy(exec[index++], "--output");
		strcpy(exec[index++], screen[i].id);

		if (screen[i].conf_f < 0) {
			sprintf(temp, "%.2f", screen[i].max_f);
		} else {
			sprintf(temp, "%.2f", screen[i].conf_f);
		}
		strcpy(exec[index++], "--rate");
		strcpy(exec[index++], temp);

		strcpy(exec[index++], "--mode");
		if (screen[i].conf_res[0] == '\0') {
			strcpy(exec[index++], screen[i].res);
		} else {
			strcpy(exec[index++], screen[i].conf_res);
		}
		
		if (screen[i].primary) {
			strcpy(exec[index++], "--primary");
		} else {
			if (!strcmp(screen[i].pos_arg, "right") || !strcmp(screen[i].pos_arg, "left"))
				sprintf(temp, "--%s-of", screen[i].pos_arg);
			else
				sprintf(temp, "--%s", screen[i].pos_arg);
			
			strcpy(exec[index++], temp);
			strcpy(exec[index++], screen[i].pos_id);
		}
	}
	for (int i = index; i < row_size; i++)
		free(exec[i]);
	exec = (char**) realloc(exec, (index + 1) * sizeof (char*));
	exec[index] = NULL;
}

int main(int argc, char *argv[]) {
	get_xrandr_args();
	get_config_args();

	int row_size = 9 * screen_size + 2;
	char **exec;

	if (check_screen_args()) {
		exec = (char**) malloc(3 * sizeof(char*));
		exec[0] = "xrandr";
		exec[1] = "--auto";
		exec[2] = NULL;
	} else {
		exec = (char**) malloc(row_size * sizeof(char*));
		for (int i = 0; i < row_size; i++) {
			exec[i] = (char*) malloc(32 * sizeof(char));
		}
		get_exec_string(exec, row_size);
	}

	free(screen);

	execv("/bin/xrandr", exec);

	return 1;
}
