/*
 * Script that loads Xresources term colors or default colors to colorsheme.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XRESOURCES_PATH "/.Xresources"
#define COLOR_COUNT 16

char *defaultcolors[] = { "#45475A", "#F38BA8", "#A6E3A1", "#F9E2AF",
                          "#89B4FA", "#F5C2E7", "#94E2D5", "#BAC2DE",
                          "#585B70", "#F38BA8", "#A6E3A1", "#F9E2AF",
                          "#89B4FA", "#F5C2E7", "#94E2D5", "#A6ADC8" };
char **colors;

int allocatecolors() {
	colors = (char**)malloc(COLOR_COUNT * sizeof(char*));
	
	if (colors == NULL)
		return 0;

	for (int i=0; i < COLOR_COUNT; i++) {
		colors[i] = (char*)calloc(8, sizeof(char));
		if (colors[i] == NULL)
			return 0;
	}
	return 1;
}

void freecolors() {
	if (colors == NULL)
		return;

	for (int i = 0; i < COLOR_COUNT; i++) {
		if (colors[i] == NULL)
			continue;
		free(colors[i]);
	}
	free(colors);
} 

/* Return values:
 * -1: Did not find string.
 *  0: Did not find color.
 *  1: All nominal.
 */
int parsestring(char *input) {
	int index;
	int colorindex = -1;
	char col[8];

	if (strstr(input, "*color") == NULL)
		return -1;
	
	for (int i=0; i < strlen(input); i++)
		if(input[i] == '#')
			colorindex = i;

	if (colorindex == -1)
		return 0;

	index = input[6] - '0';
	if (index == 1 && ((input[7] >= '0'  && (input[7] <= '9')))) {
		index *= 10;
		index += input[7] - '0';
	}
	if (index >= COLOR_COUNT)
		return 0;

	for(int i=0; i < 7; i++) 
		col[i] = input[i + colorindex];

	col[7] = '\0';
	strncpy(colors[index], col, 8);
	return 1;
}

int checkcolors() {
	for (int i=0; i < COLOR_COUNT; i++) {
		if (strlen(colors[i]) != 7)
			return 0;
		
		if (colors[i][0] != '#')
			return 0;
		
		for (int j=1; j<7; j++)
			if (!((colors[i][j] >= 'A' && colors[i][j] <= 'F') || (colors[i][j] >= '0' && colors[i][j] <= '9')))
			return 0;
		if (colors[i][7] != '\0')
			return 0;
	}
	return 1;
}

int parsefile() {
	FILE *fp;
	char *path;
	char input[512];

	if (strcmp(path = getenv("HOME"), "") == 0) {
		perror("Could not get \"HOME\" enviroment");
		return EXIT_FAILURE;
	}
	strcat(path, XRESOURCES_PATH);

	if ((fp = fopen(path, "r")) == NULL) {
		puts("Could not find ~/.Xresources file.");
		return 0;
	}
	if (!allocatecolors()) {
		perror("Could not allocate memory properly for array \"colors\"");
		freecolors();
		return EXIT_FAILURE;
	}

	while (fgets(input, 512, fp) != NULL) {
		if (!parsestring(input)) {
			freecolors();
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

	if (!checkcolors()) {
		freecolors();
		puts("Wrong color format and/or data");
		return 0;
	}
	return 1;
}

void printfile(char** clrsource) {
	FILE *fp = fopen("colorscheme.h", "w");
	fprintf(fp, "/* Autogenerated file */\n#ifndef COLORSCHEME\n#define COLORSCHEME\n\n");
	for (int i=0; i < COLOR_COUNT; i++)
		fprintf(fp, "#define CLR_%d \"^c%s^\"\n", i, clrsource[i]);

	fprintf(fp, "#define NRM \"^d^\"\n\n#endif   /* COLORSCHEME */\n");
}

int main(void) {
	if (parsefile()) {
		printfile(colors);
		freecolors();
	} else {
		puts("Applying default colorscheme");
		printfile(defaultcolors);
	}
	return EXIT_SUCCESS;
}