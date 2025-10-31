#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define BUF_MAX 1024

const char *menuargs[] = { "/usr/local/bin/dmenu", "-c", "-l", "5", "-p", "Select equalizer preset:", "-nn", NULL };
const char *program_name = "dmenu-equalizer-select";

typedef struct {
	char **items;
	size_t count;
	size_t capacity;
} StringArray;

void set_program_name(const char *argv0) {
	if (argv0) {
		const char *slash = strrchr(argv0, '/');
		program_name = slash ? slash + 1 : argv0;
	}
}

void print_error(const char *func, const char *msg, int err) {
	if (err)
		fprintf(stderr, "%s: %s: %s: %s\n", program_name, func, msg, strerror(err));
	else
		fprintf(stderr, "%s: %s: %s\n", program_name, func, msg);
}

void array_init(StringArray *arr) {
	arr->items = NULL;
	arr->count = 0;
	arr->capacity = 0;
}

void array_free(StringArray *arr) {
	if (!arr)
		return;

	for (size_t i = 0; i < arr->count; i++) {
		free(arr->items[i]);
	}

	free(arr->items);
	arr->items = NULL;
	arr->count = 0;
	arr->capacity = 0;
}

int array_append(StringArray *arr, const char *str) {
	if (!str) {
		print_error(__func__, "input string is NULL", 0);
		return -1;
	}

	if (arr->count >= arr->capacity) {
		size_t new_cap = arr->capacity == 0 ? 8 : arr->capacity * 2;
		char **new_items = realloc(arr->items, new_cap * sizeof(char*));

		if (!new_items) {
			print_error(__func__, "realloc() failed", errno);
			return -1;
		}

		arr->items = new_items;
		arr->capacity = new_cap;
	}

	arr->items[arr->count] = strdup(str);
	if (!arr->items[arr->count]) {
		print_error(__func__, "strdup() failed", errno);
		return -1;
	}

	arr->count++;
	return 0;
}

// Trim leading and trailing whitespace
char* trim_whitespace(char *str) {
	if (!str)
		return NULL;

	// Trim leading
	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0)
		return str;

	// Trim trailing
	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	*(end + 1) = '\0';
	return str;
}

int parse_string(char *str, StringArray *arr) {
	char *ptr = strtok(str, ",");

	while (ptr) {
		ptr = trim_whitespace(ptr);
		if (*ptr != '\0') {
			if (array_append(arr, ptr) != 0) {
				return -1;
			}
		}
		ptr = strtok(NULL, ",");  // FIX: was strtok(str, ",")
	}

	return 0;
}

int parse_presets(StringArray *arr) {
	const char *cmd = "easyeffects -p";
	FILE *ep;
	char buf[BUF_MAX];
	int found = 0;

	if (!(ep = popen(cmd, "r"))) {
		print_error(__func__, "popen() failed", errno);
		return -1;
	}

	array_init(arr);

	while (fgets(buf, sizeof(buf), ep)) {
		char *ptr = strstr(buf, "Output Presets: ");

		if (ptr) {
			ptr += strlen("Output Presets: ");

			// Remove newline if present
			size_t len = strlen(ptr);
			if (len > 0 && ptr[len - 1] == '\n')
				ptr[len - 1] = '\0';

			if (parse_string(ptr, arr) != 0) {
				array_free(arr);
				pclose(ep);
				return -1;
			}

			found = 1;
			break;
		}
	}

	if (pclose(ep) == -1) {
		print_error(__func__, "pclose() failed", errno);
		array_free(arr);
		return -1;
	}

	if (!found || arr->count == 0) {
		print_error(__func__, "no output presets detected", 0);
		array_free(arr);
		return 0;  // Not an error, just no presets
	}

	return 0;
}

char* run_dmenu(const StringArray *presets) {
	int pipe_in[2];
	int pipe_out[2];
	pid_t pid;
	char *result = NULL;

	if (pipe(pipe_in) == -1) {
		print_error(__func__, "pipe() failed for stdin", errno);
		return NULL;
	}

	if (pipe(pipe_out) == -1) {
		print_error(__func__, "pipe() failed for stdout", errno);
		close(pipe_in[0]);
		close(pipe_in[1]);
		return NULL;
	}

	pid = fork();
	switch (pid) {
	case -1:
		print_error(__func__, "fork() failed", errno);
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		return NULL;
	
	case 0:
		// Child process - execute dmenu

		// Redirect stdin to read from pipe_in
		close(pipe_in[1]);  // Close write end
		if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
			perror("dup2 stdin");
			exit(EXIT_FAILURE);
		}
		close(pipe_in[0]);

		// Redirect stdout to write to pipe_out
		close(pipe_out[0]);  // Close read end
		if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
			perror("dup2 stdout");
			exit(EXIT_FAILURE);
		}

		close(pipe_out[1]);

		// Execute dmenu
		execvp(menuargs[0], (char *const *)menuargs);
		perror("execvp dmenu");
		exit(EXIT_FAILURE);
	
	default:
		// Parent process

		// Close unused pipe ends
		close(pipe_in[0]);   // We only write to stdin
		close(pipe_out[1]);  // We only read from stdout

		// Write all presets to dmenu's stdin
		FILE *dmenu_in = fdopen(pipe_in[1], "w");
		if (!dmenu_in) {
			print_error(__func__, "fdopen() failed", errno);
			close(pipe_in[1]);
			close(pipe_out[0]);
			waitpid(pid, NULL, 0);
			return NULL;
		}

		for (size_t i = 0; i < presets->count; i++)
			fprintf(dmenu_in, "%s\n", presets->items[i]);

		fclose(dmenu_in);  // Close stdin, signals EOF to dmenu

		// Read dmenu's output
		FILE *dmenu_out = fdopen(pipe_out[0], "r");
		if (!dmenu_out) {
			print_error(__func__, "fdopen() failed", errno);
			close(pipe_out[0]);
			waitpid(pid, NULL, 0);
			return NULL;
		}

		char buf[BUF_MAX];
		if (fgets(buf, sizeof(buf), dmenu_out)) {
			// Remove trailing newline
			size_t len = strlen(buf);
			if (len > 0 && buf[len - 1] == '\n')
				buf[len - 1] = '\0';
			result = strdup(buf);
		}
		fclose(dmenu_out);

		// Wait for child to finish
		int status;
		if (waitpid(pid, &status, 0) == -1) {
			print_error(__func__, "waitpid() failed", errno);
			free(result);
			return NULL;
		}

		// Check if dmenu exited normally (user might have pressed ESC)
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			free(result);
			return NULL;
		}
	}

	return result;
}

int preset_select(char *preset) {
	FILE *ep;
	char *cmd;
	size_t len;

	len = strlen(preset) + strlen("easyeffects -l ") + 1;

	cmd = malloc(len);
	if (!cmd) {
		print_error(__func__, "malloc() failed", errno);
		return -1;
	}

	strncpy(cmd, "easyeffects -l ", len);
	strncat(cmd, preset, len);
	cmd[len-1] = '\0';

	if (!(ep = popen(cmd, "w"))) {
		print_error(__func__, "popen() failed", errno);
		return -1;
	}

	if (pclose(ep) == -1) {
		print_error(__func__, "pclose() failed", errno);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[]) {
	StringArray presets;
	char *selection;

	set_program_name(argv[0]);

	// Suppress unused variable warning
	(void) argc;

	if (parse_presets(&presets) != 0) {
		return EXIT_FAILURE;
	}

	if (presets.count == 0) {
		printf("No presets found\n");
		return EXIT_SUCCESS;
	}

	selection = run_dmenu(&presets);
	array_free(&presets);

	if (!selection) {
		printf("No preset selected\n");
		return EXIT_SUCCESS;
	}

	preset_select(selection);

	return EXIT_SUCCESS;
}
