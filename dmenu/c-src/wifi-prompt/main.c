#include "nm-core-types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <NetworkManager.h>

/* struct declerations*/

typedef struct {
	char *id;
	char *strength;
	char *ssid;
	char *security;
	char *frequency;
} APDetails;

typedef struct {
	APDetails **ap;
	int len;
	int id_max;
	int strength_max;
	int ssid_max;
	int security_max;
	int frequency_max;
} APList;

/* function declerations */

APDetails*     access_point_to_struct(NMAccessPoint *ap, int index);
static void    add_connection(NMClient *client, GMainLoop *loop, NMAccessPoint *ap);
void           ap_list_free(APList *list);
void           ap_list_set_bounds(APList *list);
char*          ap_list_to_string(APList *list);
static void    callback(GObject *client, GAsyncResult *result, gpointer user_data);
NMAccessPoint* get_access_point(NMClient *client, NMDevice **device);
int            get_ap_input(char *menu, char *prompt);
char*          get_password(NMAccessPoint *ap);
char*          get_security(NMAccessPoint *ap);
NMDevice*      get_wifi_device(NMClient *client);
void           init_nm_client(NMClient **client);

/* code */

APDetails*
access_point_to_struct(NMAccessPoint *ap, int index)
{
	APDetails  *ret;
	GBytes     *ssid;
	guint8     strength;
	guint32    freq;

	ret     = malloc(sizeof(APDetails));
	ret->id = g_strdup_printf("%d", index);
	ssid    = nm_access_point_get_ssid(ap);
	freq    = nm_access_point_get_frequency(ap);
	
	if (ssid)
		ret->ssid = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid));
	else
		ret->ssid = g_strdup("--");

	strength       = nm_access_point_get_strength(ap);
	ret->strength  = g_strdup_printf("%u", strength);
	ret->frequency = g_strdup_printf("%d MHz", freq);
	ret->security  = get_security(ap);
	return ret;
}

static void
add_connection(NMClient *client, GMainLoop *loop, NMAccessPoint *ap)
{
	NMConnection              *connection = NULL;
	NMSettingConnection       *s_con = NULL;
	NMSettingWireless         *s_wireless = NULL;
	NMSettingIP4Config        *s_ip4 = NULL;
	NMSettingWirelessSecurity *s_secure = NULL;
	GBytes                    *ssid_struct = NULL;
	const char                *uuid = NULL;
	const char                *ssid = NULL;
	const char                *password = NULL;

	connection  = nm_simple_connection_new();
	s_wireless  = (NMSettingWireless *) nm_setting_wireless_new();
	s_secure    = (NMSettingWirelessSecurity *) nm_setting_wireless_security_new();
	s_con       = (NMSettingConnection *) nm_setting_connection_new();
	ssid_struct = nm_access_point_get_ssid(ap);
	uuid        = nm_utils_uuid_generate();
	password    = get_password(ap);

	if (ssid_struct)
		ssid = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid_struct, NULL), g_bytes_get_size(ssid_struct));
	else
		ssid = g_strdup("--");

	g_object_set(G_OBJECT(s_con),
	             NM_SETTING_CONNECTION_UUID, uuid,
	             NM_SETTING_CONNECTION_ID, ssid,
	             NM_SETTING_CONNECTION_TYPE, "802-11-wireless",
	             NULL);
	nm_connection_add_setting(connection, NM_SETTING(s_con));

	s_wireless = (NMSettingWireless *)nm_setting_wireless_new();

	g_object_set(G_OBJECT(s_wireless),
	             NM_SETTING_WIRELESS_SSID, ssid,
	             NULL);
	nm_connection_add_setting(connection, NM_SETTING(s_wireless));

	if (password && password[0]!='\0') {
		s_secure = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
		g_object_set(G_OBJECT(s_secure),
		             NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
		             NM_SETTING_WIRELESS_SECURITY_PSK, password,
		             NULL);
		nm_connection_add_setting(connection, NM_SETTING(s_secure));
	}

	s_ip4 = (NMSettingIP4Config *)nm_setting_ip4_config_new();
	g_object_set(G_OBJECT(s_ip4),
	             NM_SETTING_IP_CONFIG_METHOD,
	             NM_SETTING_IP4_CONFIG_METHOD_AUTO,
	             NULL);
	nm_connection_add_setting(connection, NM_SETTING(s_ip4));
	puts("?");

	nm_client_add_connection_async(client, connection, TRUE, NULL, callback, loop);
	puts("?");
	g_object_unref(connection);
}

void
ap_list_free(APList *list)
{
	for (int i = 0; i < list->len; i++) {
		if (!list->ap[i])
			continue;
		if (list->ap[i]->ssid)
			free(list->ap[i]->ssid);
		if (list->ap[i]->strength)
			free(list->ap[i]->strength);
		if (list->ap[i]->security)
			free(list->ap[i]->security);
		if (list->ap[i]->id)
			free(list->ap[i]->id);
		if (list->ap[i]->frequency)
			free(list->ap[i]->frequency);
		free(list->ap[i]);
	}
	list->ap = NULL;
}

void
ap_list_set_bounds(APList *list)
{
	for (int i = 0; i < list->len; i++) {
		if ((int) strlen(list->ap[i]->ssid) > list->ssid_max)
			list->ssid_max = strlen(list->ap[i]->ssid);
		if ((int) strlen(list->ap[i]->security) > list->security_max)
			list->security_max = strlen(list->ap[i]->security);
		if ((int) strlen(list->ap[i]->strength) > list->strength_max)
			list->strength_max = strlen(list->ap[i]->strength);
		if ((int) strlen(list->ap[i]->id) > list->id_max)
			list->id_max = strlen(list->ap[i]->id);
		if ((int) strlen(list->ap[i]->frequency) > list->frequency_max)
			list->frequency_max = strlen(list->ap[i]->frequency);
	}
}

char*
ap_list_to_string(APList *list)
{
	char *ret;
	int  lineSize;
	char temp[512];

	ap_list_set_bounds(list);
	lineSize = list->id_max + list->ssid_max + list->security_max + list->strength_max + list->frequency_max+ 13;
	ret = malloc(lineSize * (list->len+1) * sizeof(char));

	sprintf(temp, "%-*s   %-*s   %*s   %*s\t%*s\n",
	        list->ssid_max, "SSID", list->strength_max, "Strength",
	        list->security_max, "Security", list->frequency_max, "Frequency",
	        list->id_max, "-1");
	strcpy(ret, temp);

	for (int i = 0; i < list->len; i++) {
		sprintf(temp, "%-*s   %-*s   %*s   %*s\t%*s\n",
		        list->ssid_max, list->ap[i]->ssid,
		        list->strength_max, list->ap[i]->strength,
		        list->security_max, list->ap[i]->security,
		        list->frequency_max, list->ap[i]->frequency,
		        list->id_max, list->ap[i]->id);
		strcat(ret, temp);
	}

	if (ret[0] == '\0') {
		free(ret);
		ret = NULL;
	}

	return ret;
}

static void
callback(GObject *client, GAsyncResult *result, gpointer user_data)
{
	NMRemoteConnection *remote;
	GError *error = NULL;

	remote = nm_client_add_connection_finish(NM_CLIENT(client), result, &error);

	if (error) {
		g_print("Error adding connection: %s", error->message);
		g_error_free(error);
	} else {
		g_print("Added: %s\n", nm_connection_get_path(NM_CONNECTION(remote)));
		g_object_unref(remote);
	}

	g_main_loop_quit((GMainLoop *)user_data);
}

NMAccessPoint*
get_access_point(NMClient *client, NMDevice **device)
{
	const GPtrArray *aps;
	NMAccessPoint   *ap;
	APList          list;
	char            *string;
	int             return_index;

	*device = get_wifi_device(client);

	if (*device == NULL) {
		return NULL;
	}

	list = (APList) {.ap = NULL, .len = 0, .id_max = 2, .ssid_max = 4, .security_max = 8, .strength_max = 8, .frequency_max=9};
	aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(*device));

	if (!aps->len) {
		return NULL;
	}

	for (int i = 0; i < (int) aps->len; i++) {
		ap = g_ptr_array_index(aps, i);
		list.ap = realloc(list.ap, ++(list.len) * sizeof(APDetails*));
		list.ap[list.len-1] = access_point_to_struct(ap, i);
	}

	string = ap_list_to_string(&list);
	return_index = get_ap_input(string, "Select the wifi access point:");

	if (return_index != -1) 
		ap = g_ptr_array_index(aps, return_index);
	else
		ap = NULL;

	ap_list_free(&list);
	if (string != NULL) {
		free(string);
		string = NULL;
	}

	return ap;
}

int
get_ap_input(char *menu, char *prompt)
{
	int  option=-1;
	int  menusize = strlen(menu);
	int  writepipe[2], readpipe[2];
	char buffer[64] = "";
	char *ptr;

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}
	
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - xmenu */
			close(writepipe[1]);
			close(readpipe[0]);
			dup2(writepipe[0], STDIN_FILENO);
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);
			
			execl("/usr/local/bin/dmenu", "dmenu", "-c", "-nn", "-l", "20", "-p", prompt, NULL);
			exit(EXIT_FAILURE);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			write(writepipe[1], menu, menusize);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
	}

	ptr = strchr(buffer, '\t');
	if (ptr != NULL)
		sscanf(ptr, "%d", &option);

	return option;
}

char*
get_password(NMAccessPoint *ap)
{
	if (!strcmp(get_security(ap), "---"))
		return NULL;

	int  writepipe[2], readpipe[2];
	char buffer[512] = "";

	if (pipe(writepipe) < 0 || pipe(readpipe) < 0) {
		perror("Failed to initialize pipes");
		exit(EXIT_FAILURE);
	}
	
	switch (fork()) {
		case -1:
			perror("Failed in forking");
			exit(EXIT_FAILURE);

		case 0: /* child - xmenu */
			close(writepipe[1]);
			close(readpipe[0]);
			close(writepipe[0]);
			dup2(readpipe[1], STDOUT_FILENO);
			close(readpipe[1]);
			
			execl("/usr/local/bin/dmenu", "dmenu", "-c", "-nn", "-P", "-p", "Enter the AP's password:", NULL);
			exit(EXIT_FAILURE);

		default: /* parent */
			close(writepipe[0]);
			close(readpipe[1]);
			close(writepipe[1]);
			wait(NULL);
			read(readpipe[0], buffer, sizeof(buffer));
			close(readpipe[0]);
	}
	return g_strdup(buffer);
}

char*
get_security(NMAccessPoint *ap)
{
	guint32 flags, wpa_flags, rsn_flags;
	GString *security_str;
	char    *ret;

	flags        = nm_access_point_get_flags(ap);
	wpa_flags    = nm_access_point_get_wpa_flags(ap);
	rsn_flags    = nm_access_point_get_rsn_flags(ap);
	security_str = g_string_new(NULL);
	ret          = NULL;

	if (!(flags & NM_802_11_AP_FLAGS_PRIVACY) && (wpa_flags != NM_802_11_AP_SEC_NONE)
	    && (rsn_flags != NM_802_11_AP_SEC_NONE))
		g_string_append(security_str, "Encrypted: ");

	if ((flags & NM_802_11_AP_FLAGS_PRIVACY) && (wpa_flags == NM_802_11_AP_SEC_NONE)
	    && (rsn_flags == NM_802_11_AP_SEC_NONE))
		g_string_append(security_str, "WEP ");
	if (wpa_flags != NM_802_11_AP_SEC_NONE)
		g_string_append(security_str, "WPA ");
	if ((rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
	    || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X))
		g_string_append(security_str, "WPA2 ");
	if (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_SAE)
		g_string_append(security_str, "WPA3 ");
	if ((rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_OWE)
	    || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_OWE_TM))
		g_string_append(security_str, "OWE ");
	if ((wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
	    || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X))
		g_string_append(security_str, "802.1X ");

 	if (security_str->len > 0)
		g_string_truncate(security_str, security_str->len - 1); /* Chop off last space */

	if (security_str->str != NULL) {
		ret = g_strdup(security_str->str);
		g_string_free(security_str,TRUE);
	} else {
		ret = g_strdup("---");
	}
	return ret;
}

NMDevice*
get_wifi_device(NMClient *client)
{
	const GPtrArray *devices = nm_client_get_devices(client);
	NMDevice        *device  = NULL;
	NMDevice        *ret     = NULL;

	for (int i = 0; i < (int) devices->len; i++) {
		device = g_ptr_array_index(devices, i);
		if (NM_IS_DEVICE_WIFI(device))
			ret = device;
	}

	return ret;
}

void
init_nm_client(NMClient **client)
{
	GError *error = NULL;

	*client       = nm_client_new(NULL, &error);

	if (error != NULL) {
		g_error("Error initializing NetworkManager client: %s", error->message);
	}
}

int main(void) {
	NMClient      *client = NULL;
	NMDevice      *device = NULL;
	NMAccessPoint *ap     = NULL;
	GMainLoop     *loop   = NULL;

	init_nm_client(&client);
	ap = get_access_point(client, &device);
	
	if (!ap)
		exit(-1);

	add_connection(client, loop, ap);
	
	g_main_loop_run(loop);

	return 0;
}

