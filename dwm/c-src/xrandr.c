#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

/* structures */

typedef struct {
	RRMode       id;
	unsigned int height;
	unsigned int width;
	double       rate;
} mode;

typedef struct {
	char         *name;
	mode         *m;
	unsigned int xOffset;
	unsigned int yOffset;
	Rotation     rotation;
	bool         primary;
} monitor;

/* variables */

static Display            *dpy = NULL;
static Window             root;
static XRRScreenResources *resources = NULL;
static monitor            **screen = NULL;
static unsigned int       screenSize = 0;

const char config_path[] = "/.config/xrandr/displaysetup.conf";


/* function definitions */
char* allocateStringMemory(char *str);
void appendScreen();
void expandScreenSize(bool retract);
void freeApplication();
void freeScreen();
int getConfigArgs();
FILE* getConfigStream();
void getModes();
int getOptions(FILE *fp, long offset);
void setScreen();
void getScreenMaxValues();
long getScreenOffset(FILE *fp);
void initializeX();
char* stripWhitespace(char *string);
mode* XRRGetMonitorMaxMode(XRROutputInfo *output, XRRScreenResources *resources);

char*
allocateStringMemory(char *str)
{
	char *ptr;

	ptr = (char*) malloc((strlen(str) + 1) * sizeof(char));
	
	if (ptr == NULL) {
		fprintf(stderr, "malloc() returned null pointer to allocateStringMemory\n");
		exit(-1);
	}
	strcpy(ptr, str);
	return ptr;
}

void
appendScreen()
{
	screen = (monitor**) realloc(screen, ++screenSize * sizeof(monitor*));
	if (screen == NULL) {
		fprintf(stderr, "malloc() returned null pointer to screen. Line:%d\n", __LINE__);
		exit(-1);
	}
	if ((screen[screenSize-1] = (monitor*) malloc(sizeof(monitor))) == NULL) {
		fprintf(stderr, "malloc() returned null pointer to screen[%d]. Line:%d\n", screenSize - 1, __LINE__);
		exit(-1);
	}
	screen[screenSize-1]->name = NULL;
	screen[screenSize-1]->primary = false;
	screen[screenSize-1]->rotation = RR_Rotate_0;
	screen[screenSize-1]->xOffset = 0;
	screen[screenSize-1]->yOffset = 0;
}

void
expandScreenSize(bool retract)
{
	int width = 0;
	int height = 0;
	int mmWidth;
	int mmHeight;
	int nsizes;
	
	for (int i = 0; i < screenSize; i++) {
		if (screen[i]->yOffset + screen[i]->m->height > height)
			height = screen[i]->yOffset + screen[i]->m->height;
		if (screen[i]->xOffset + screen[i]->m->width > width)
			width = screen[i]->xOffset + screen[i]->m->width;
	}

	XRRScreenSize *s = XRRSizes(dpy, 0, &nsizes);
	
	float dpi = (25.4 * s[0].height) / s[0].mheight;
	
	mmWidth =  (int) ((25.4 * width) / dpi);
  	mmHeight = (int) ((25.4 * height) / dpi);

	if (!retract) {
		if (s[0].width > width) {
			width = s[0].width;
			mmWidth = s[0].mwidth;
		}
		
		if (s[0].height > height) {
			height = s[0].height;
			mmHeight = s[0].mheight;
		}
	}

	XRRSetScreenSize(dpy, root, width, height, mmWidth, mmHeight);
}

void
freeApplication()
{
	XRRFreeScreenResources(resources);
	XCloseDisplay(dpy);
	freeScreen();
}

void
freeScreen()
{
	if(screen == NULL)
		return;
	for (int i = 0; i < screenSize; i++) {
		if (screen[i]->name != NULL)
			free(screen[i]->name);
		if (screen[i]->m != NULL)
			free(screen[i]->m);
		if (screen[i] != NULL)
			free(screen[i]);
	}
	free(screen);
	screenSize = 0;
}

int
getConfigArgs()
{
	FILE *fp;
	long offset;

	if ((fp = getConfigStream()) == NULL)
		return 1;
	
	if ((offset = getScreenOffset(fp)) < 0) {
		fclose(fp);
		return 1;
	}

	if (getOptions(fp, offset)) {
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}

FILE*
getConfigStream()
{
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

void
getModes()
{
	XRROutputInfo *output = NULL;
	XRRModeInfo *minf = NULL;
	mode *m = NULL;

	char str1[16], str2[16];

	for (int i = 0; i < screenSize; i++) {
		m = screen[i]->m;

		for (int j = 0; j < resources->noutput; j++) {
			output = XRRGetOutputInfo(dpy, resources, resources->outputs[j]);
			if(strcmp(screen[i]->name, output->name)) {
				XRRFreeOutputInfo(output);
				continue;
			}
		}

		if (output == NULL)
			continue;

		for (int j = 0; j < output->nmode; j++) {
			for (int k = 0; k < resources->nmode; k++) {
				if (output->modes[j] == resources->modes[k].id) {
					minf = &resources->modes[j];
					if (minf->width == m->width && minf->height == m->height) {
						sprintf(str1, "%.1lf", m->rate);
						sprintf(str2, "%.1lf", (double) minf->dotClock / (double) (minf->hTotal * minf->vTotal));
						if (!strcmp(str1, str2)) {
							m->id = minf->id;
							goto LOOPEND;
						}
					}
					break;
				}
			}
		}
LOOPEND:
		XRRFreeOutputInfo(output);
		continue;
	}
}

int
getOptions(FILE *fp, long offset)
{
	char buffer[512];
	char *ptr;

	int moni = -1;

	if (fseek(fp, offset, SEEK_SET))
		return 1;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		ptr = stripWhitespace(buffer);
		if (ptr[0] == '#')
			continue;

		if (!strcmp(ptr, "EndSection"))
			return 0;
		
		// Get index of the montior, from the connected ones.
		if (strstr(ptr, "Monitor ") == ptr) {
			ptr += 8;
			for (int i = 0; i < screenSize; i++)
				if (!strcmp(screen[i]->name, ptr)) {
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

			if (strstr(ptr, "xOffset ") == ptr) {
				ptr += 8;
				sscanf(ptr, "%u", &screen[moni]->xOffset);
				continue;
			}

			if (strstr(ptr, "yOffset ") == ptr) {
				ptr += 8;
				sscanf(ptr, "%u", &screen[moni]->yOffset);
				continue;
			}

			if (strstr(ptr, "Rotate ") == ptr) {
				ptr += 7;
				sscanf(ptr, "%s", buffer);
				if (!strcmp(buffer, "normal"))
					screen[moni]->rotation = RR_Rotate_0;
				else if (!strcmp(buffer, "inverted"))
					screen[moni]->rotation = RR_Rotate_180;
				else if (!strcmp(buffer, "left"))
					screen[moni]->rotation = RR_Rotate_270;
				else if (!strcmp(buffer, "right"))
					screen[moni]->rotation = RR_Rotate_90;
				continue;
			}

			if (strstr(ptr, "Refresh Rate ") == ptr) {
				ptr += 13;
				sscanf(ptr, "%lf", &screen[moni]->m->rate);
				continue;
			}

			if (strstr(ptr, "Resolution ") == ptr) {
				ptr += 11;
				sscanf(ptr, "%ux%u", &screen[moni]->m->width, &screen[moni]->m->height);
				continue;
			}
		}
	}
	return 1;
}

void
setScreen()
{
	
	for (int i = 0; i < screenSize; i++) {
		if (screen[i]->rotation == RR_Rotate_90 || screen[i]->rotation == RR_Rotate_270) {
			unsigned int temp = screen[i]->m->height;
			screen[i]->m->height = screen[i]->m->width;
			screen[i]->m->width = temp;
		}
	}
	expandScreenSize(false);
	XRROutputInfo *output;
	XRRCrtcInfo *crtc;

	for (int i = 0; i < resources->noutput; i++) {
		output = XRRGetOutputInfo(dpy, resources, resources->outputs[i]);
		for (int j = 0; j < screenSize; j++) {
			if (!strcmp(screen[j]->name, output->name)) {
				crtc = XRRGetCrtcInfo(dpy, resources, output->crtc);
				XRRSetCrtcConfig(dpy, resources, output->crtc,
				                 crtc->timestamp, screen[j]->xOffset, screen[j]->yOffset,
						 screen[j]->m->id, screen[j]->rotation, crtc->outputs,
						 crtc->noutput);
				XRRFreeCrtcInfo(crtc);
				if (screen[j]->primary)
					XRRSetOutputPrimary(dpy, root, resources->outputs[i]);
			}
		}
		XRRFreeOutputInfo(output);
	}
	expandScreenSize(true);
}

void
getScreenMaxValues()
{
	XRROutputInfo *output; 

	for (int i = 0; i < resources->noutput; i++) {
		output = XRRGetOutputInfo(dpy, resources, resources->outputs[i]);
		if (!output->connection) {
			appendScreen();
			screen[screenSize-1]->name = allocateStringMemory(output->name);
			screen[screenSize-1]->m = XRRGetMonitorMaxMode(output, resources);
		}
		XRRFreeOutputInfo(output);
	}
}

long
getScreenOffset(FILE *fp)
{
	char *ptr;
	char buffer[512];
	
	int match_count = 0;
	long offset = -1;
	bool inloop = false;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		ptr = stripWhitespace(buffer);

		// Ignore comments.
		if (ptr[0] == '#')
			continue;

		// Handle section start.
		if (!strcmp(ptr, "Section Screen")) {
			if (inloop) {
				return 1;
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
				if (match_count == screenSize)
					return offset;
				inloop = false;
				continue;
			} else {
				return 1;
			}
		}

		// Check if the monitor id matches with a connected one.
		if (strstr(ptr, "Monitor ") == ptr) {
			ptr += 8;
			for (int i = 0; i < screenSize; i++) 
				if (!strcmp(screen[i]->name, ptr))
					match_count++;
		}
	}
	return 1;
}

void
initializeX()
{
	dpy = XOpenDisplay(NULL);
	
	if (dpy == NULL) {
		fprintf(stderr, "Failed to start X\n");
		exit(-1);
	}

	root = XDefaultRootWindow(dpy);
	resources = XRRGetScreenResources(dpy, root);
}

char*
stripWhitespace(char *string)
{
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

mode*
XRRGetMonitorMaxMode(XRROutputInfo *output, XRRScreenResources *resources)
{
	mode *maxMode;
	XRRModeInfo *m_inf;
	double rate;

	if ((maxMode = (mode*) malloc(sizeof(mode))) == NULL) {
		fprintf(stderr, "malloc() returned null pointer to maxMode. Line:%d\n", __LINE__);
		exit(-1);
	}

	maxMode->id = 0;
	maxMode->height = 0;
	maxMode->width = 0;
	maxMode->rate = -1.0;

	for (int i = 0; i < output->nmode; i++) {
		for (int j = 0; j < resources->nmode; j++) {
			if (output->modes[i] == resources->modes[j].id) {
				m_inf = &resources->modes[j];
				if (m_inf->height >= maxMode->height && m_inf->width >= maxMode->width) {
					rate = (double) m_inf->dotClock / (double) (m_inf->hTotal * m_inf->vTotal);
					if (rate >= maxMode->rate) {
						maxMode->rate = rate;
						maxMode->height = m_inf->height;
						maxMode->width = m_inf->width;
						maxMode->id = m_inf->id;
					}
				}
			}
		}
	}
	return maxMode;
}

int
main(int argc, char *argv[])
{
	initializeX();

	getScreenMaxValues();
	getConfigArgs();

	setScreen();

	freeApplication();
	return 0;
}
