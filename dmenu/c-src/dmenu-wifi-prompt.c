#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libnotify/notification.h>

typedef struct {
	char      BSSID[18];
	char      SSID[128];
	char      security[64];
	short int signal;
} wifiprop;

wifiprop **list = NULL;
int      listSize = 0;

void notifyFailed(NotifyNotification *notification) {
	notify_notification_update(notification, "  Network Manager", "Wifi probing failed!", "x");
	g_object_unref(G_OBJECT(notification));
        notify_uninit();
}

void getProp() {
	FILE *ep;
	char buffer[512];
	char temp[512];
	char *ptr;
	int offset[4];
	NotifyNotification *notification;

	notify_init("dmenu_wifi_prompt");
	notification = notify_notification_new("       Network Manager", "Probing wifi access points...", "wifi-radar");
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_LOW);
        notify_notification_show(notification, NULL);

	if ((ep = popen("nmcli -c no -f \"BSSID, SSID, SIGNAL, SECURITY\" device wifi list", "r")) == NULL) {
		notifyFailed(notification);
		return;
	}

	// Parse the starting index of each field.
	for (int i = 0; i < 4; i++) {
		offset[i] = -1;
	}

	fgets(buffer, sizeof(buffer), ep);
	offset[0] = strstr(buffer, "BSSID") - buffer;
	offset[1] = strstr(buffer, " SSID") - buffer;
	offset[2] = strstr(buffer, "SIGNAL") - buffer;
	offset[3] = strstr(buffer, "SECURITY") - buffer;

	for (int i = 0; i < 4; i++) {
		if (offset[i] < 0 || offset[i] > 510) {
			notifyFailed(notification);
			return;
		}
	}

	notify_notification_close(notification, NULL);
	g_object_unref(G_OBJECT(notification));
        notify_uninit();

	while (fgets(buffer, sizeof(buffer), ep) != NULL) {
		listSize++;
		list = (wifiprop**) realloc(list, listSize * sizeof(wifiprop*));
		list[listSize - 1] = (wifiprop*) malloc(sizeof(wifiprop));

		ptr = buffer + offset[0];
		strncpy(temp, ptr, offset[1] - offset[0]);
		sscanf(temp, "%s", list[listSize - 1]->BSSID);

		ptr = buffer + offset[1];
		strncpy(temp, ptr, offset[2] - offset[1]);
		sscanf(temp, "%[^\n]s", list[listSize-1]->SSID);

		ptr = buffer + offset[2];
		strncpy(temp, ptr, offset[3] - offset[2]);
		sscanf(temp, "%d", &list[listSize-1]->signal);

		ptr = buffer + offset[3];
		sscanf(ptr, "%[^\n]s", list[listSize-1]->security);
	}
	
	pclose(ep);
}

int main(void) {
	getProp();
	for (int i = 0; i < listSize; i++) {
		printf("%s %s %d %s\n", list[i]->BSSID, list[i]->SSID, list[i]->signal, list[i]->security);
	}
	return 0;
}
