#include "nm-core-types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unistd.h>
#include <NetworkManager.h>
#include <libnotify/notification.h>
#include <libnotify/notify.h>

#define NM_FLAGS_ANY(flags, check)  ((((flags) & (check)) != 0) ? TRUE : FALSE)
#define SCAN_TIMEOUT_MSEC 5000

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

typedef struct {
	NMAccessPoint *ap;
	NMDevice      *device;
	NMClient      *client;
	NMConnection  *connection;
	gboolean      terminate;
	GString       *message;
	GString       *error;
	GMainLoop     *loop;
} NMDetails;

/* function declerations */

APDetails*  access_point_to_struct(NMAccessPoint *ap, int index);
static void activate_cb(GObject *client, GAsyncResult *result, gpointer user_data);
static void activate_connection(NMDetails *nm);
static void add_cb(GObject *client, GAsyncResult *result, gpointer user_data);
static void add_connection(NMDetails *nm);
void        ap_list_free(APList *list);
void        ap_list_set_bounds(APList *list);
char*       ap_list_to_string(APList *list);
void        get_access_point(NMDetails *nm);
void        get_ap_connection(NMDetails *nm);
int         get_ap_input(char *menu, char *prompt);
char*       get_password(void);
char*       get_security(NMAccessPoint *ap);
void        get_wifi_device(NMDetails *nm);
static void scan_device_cb(GObject *device_wifi, GAsyncResult *result, gpointer user_data);
int         scan_device(NMDetails *nm);
void        init_nm_client(NMDetails **nm);

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
		ret->ssid = g_strdup("---");

	strength       = nm_access_point_get_strength(ap);
	ret->strength  = g_strdup_printf("%u", strength);
	ret->frequency = g_strdup_printf("%d MHz", freq);
	ret->security  = get_security(ap);

	return ret;
}

static void
activate_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
	NMActiveConnection      *active_connection = NULL;
	NMActiveConnectionState state;
	NMDetails               *nm                = user_data;
	GError                  *error             = NULL;
	GBytes                  *ssid              = NULL;
	char                    *ssid_str          = NULL;
	
	active_connection = nm_client_activate_connection_finish(NM_CLIENT(client), result, &error);
	if (error) {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append_printf(nm->error, "Error activating connection: %s\n", error->message);
		g_error_free(error);
	} else {
		state = nm_active_connection_get_state(active_connection);
		ssid = nm_access_point_get_ssid(nm->ap);

		if (ssid)
			ssid_str = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid));
		else
			ssid_str = g_strdup("---");

		switch (state) {
			case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
			case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
				if(!nm->message)
					nm->message = g_string_new("");
				g_string_append_printf(nm->message, "Successfuly connected to wifi network:\n%s", ssid_str);
				break;

			case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
			case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:

				if(!nm->message)
					nm->message = g_string_new("");
				g_string_append_printf(nm->message, "Failed to connect to wifi network:\n%s\n", ssid_str);
				break;

			default:
				break;
		}
	}
	if (active_connection)
		g_object_unref(active_connection);

	g_main_loop_quit(nm->loop);
}

static void
activate_connection(NMDetails *nm)
{
	nm_client_activate_connection_async(nm->client, nm->connection, nm->device, nm_object_get_path (NM_OBJECT(nm->ap)), NULL, activate_cb, nm);
}

static void
add_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
	NMRemoteConnection *remote_connection = NULL;
	GError             *error = NULL;
	NMDetails          *nm = user_data;

	remote_connection = nm_client_add_connection_finish(NM_CLIENT(client), result, &error);

	if (error) {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append_printf(nm->error, "Error adding connection: %s\n", error->message);
		g_error_free(error);
		nm->terminate = TRUE;
	}

	if (remote_connection)
		g_object_unref(remote_connection);

	g_main_loop_quit(nm->loop);
}

static void
add_connection(NMDetails *nm)
{
	NMSettingConnection       *s_connection = NULL;
	NMSettingWireless         *s_wireless = NULL;
	NMSettingIP4Config        *s_ip4 = NULL;
	NMSettingWirelessSecurity *s_wsecurity = NULL;
	GBytes                    *ssid_struct;
	GString                   *ssid;
	guint32                   ap_wpa_flags, ap_rsn_flags;
	const char                *uuid;
	const char                *password;

	nm->connection = nm_simple_connection_new();
	s_wireless     = (NMSettingWireless *) nm_setting_wireless_new();
	s_connection   = (NMSettingConnection *) nm_setting_connection_new();
	s_wsecurity    = (NMSettingWirelessSecurity*) nm_setting_wireless_security_new();
	ssid_struct    = nm_access_point_get_ssid(nm->ap);
	uuid           = nm_utils_uuid_generate();
	password       = NULL;

	if (ssid_struct)
		ssid = g_string_new(nm_utils_ssid_to_utf8(g_bytes_get_data(ssid_struct, NULL), g_bytes_get_size(ssid_struct)));
	else
		ssid = g_string_new("---");

	g_object_set(G_OBJECT(s_connection),
	             NM_SETTING_CONNECTION_UUID, uuid,
	             NM_SETTING_CONNECTION_ID, ssid->str,
	             NM_SETTING_CONNECTION_TYPE, "802-11-wireless",
	             NULL);
	nm_connection_add_setting(nm->connection, NM_SETTING(s_connection));

	g_object_set(G_OBJECT(s_wireless),
	             NM_SETTING_WIRELESS_SSID, ssid->str,
		     NM_SETTING_WIRELESS_HIDDEN, FALSE,
	             NULL);
	nm_connection_add_setting(nm->connection, NM_SETTING(s_wireless));

	ap_wpa_flags = nm_access_point_get_wpa_flags(nm->ap);
	ap_rsn_flags = nm_access_point_get_rsn_flags(nm->ap);

	if ((ap_wpa_flags != NM_802_11_AP_SEC_NONE
	     && !NM_FLAGS_ANY(ap_wpa_flags,
	                      NM_802_11_AP_SEC_KEY_MGMT_OWE | NM_802_11_AP_SEC_KEY_MGMT_OWE_TM))
	     || (ap_rsn_flags != NM_802_11_AP_SEC_NONE
	     && !NM_FLAGS_ANY(ap_rsn_flags,
	                      NM_802_11_AP_SEC_KEY_MGMT_OWE | NM_802_11_AP_SEC_KEY_MGMT_OWE_TM))) {
		const char *con_password = NULL;

		if (ap_wpa_flags == NM_802_11_AP_SEC_NONE
		    && ap_rsn_flags == NM_802_11_AP_SEC_NONE) {
			con_password = nm_setting_wireless_security_get_wep_key(s_wsecurity, 0);
		} else if ((ap_wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
		         || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
		         || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_SAE)) {
			con_password = nm_setting_wireless_security_get_psk(s_wsecurity);
		}

		if (!password && !con_password) {
			password = get_password();
		}

		if (password) {
			if (ap_wpa_flags == NM_802_11_AP_SEC_NONE && ap_rsn_flags == NM_802_11_AP_SEC_NONE) {
				nm_setting_wireless_security_set_wep_key(s_wsecurity, 0, password);
				g_object_set(G_OBJECT(s_wsecurity),
				             NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE,
				             NM_WEP_KEY_TYPE_KEY,
				             NULL);
			} else if ((ap_wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
			            || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
			            || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_SAE)) {
				g_object_set(G_OBJECT(s_wsecurity),
				NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
				NM_SETTING_WIRELESS_SECURITY_PSK, password, NULL);
			}
		}
	}

	nm_connection_add_setting(nm->connection, NM_SETTING(s_wsecurity));

	s_ip4 = (NMSettingIP4Config *)nm_setting_ip4_config_new();
	g_object_set(G_OBJECT(s_ip4),
	             NM_SETTING_IP_CONFIG_METHOD,
	             NM_SETTING_IP4_CONFIG_METHOD_AUTO,
	             NULL);
	nm_connection_add_setting(nm->connection, NM_SETTING(s_ip4));

	nm_client_add_connection_async(nm->client, nm->connection, TRUE, NULL, add_cb, nm);
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

	sprintf(temp, "%-*s   %-*s   %*s   %-*s\t%*s\n",
	        list->ssid_max, "SSID", list->strength_max, "Strength",
	        list->security_max, "Security", list->frequency_max, "Frequency",
	        list->id_max, "-1");
	strcpy(ret, temp);

	for (int i = 0; i < list->len; i++) {
		sprintf(temp, "%-*s   %-*s   %*s   %-*s\t%*s\n",
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

char*
get_summary(char *text, char *body)
{
	int lcount = 0;
	int mcount = 0;

	char *summary = malloc(sizeof(body) + sizeof(text));
	size_t size;
	summary[0] = '\0';

	for (int i = 0; i < (int) strlen(body); i++) {
		if (body[i] == '\n') {
			if (lcount > mcount)
				mcount = lcount;
			lcount = 0;
			continue;
		}
		lcount++;
	}
	
	if (lcount > mcount)
		mcount = lcount;
	mcount = (mcount - strlen(text)) / 2;

	for (int i = 0; i < mcount; i++)
		strcat(summary, " ");
	strcat(summary, text);
	strcat(summary, "\0");
	printf("%d\n", (int)strlen(summary));
	size = (strlen(summary)+1)*sizeof(char);
	printf("%ul\n", (unsigned int) size);
	summary = (char*) realloc(summary, size);
	puts("?");
	return summary;
}

void
get_access_point(NMDetails *nm)
{
	const GPtrArray *aps;
	NMAccessPoint   *ap;
	APList          list;
	char            *string;
	int             return_index;

	list = (APList) {.ap = NULL, .len = 0, .id_max = 2, .ssid_max = 4, .security_max = 8, .strength_max = 8, .frequency_max=9};
	aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(nm->device));

	if (!aps->len) {
		if (!nm->error)
			nm->error = g_string_new("");
		if (!nm->message)
			nm->message = g_string_new("");
		g_string_append(nm->error, "No access points have been detected\n");
		g_string_append(nm->message, "No access points have been detected\n");
		nm->terminate = TRUE;
		return;
	}

	for (int i = 0; i < (int) aps->len; i++) {
		ap = g_ptr_array_index(aps, i);
		list.ap = realloc(list.ap, ++(list.len) * sizeof(APDetails*));
		list.ap[list.len-1] = access_point_to_struct(ap, i);
	}

	string = ap_list_to_string(&list);
	return_index = get_ap_input(string, "Select the wifi access point:");

	if (return_index != -1) {
		nm->ap = g_ptr_array_index(aps, return_index);
	} else {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append(nm->error, "No access point selected\n");
		nm->terminate = TRUE;
	}

	ap_list_free(&list);
	if (string != NULL) {
		free(string);
		string = NULL;
	}
}

void
get_ap_connection(NMDetails *nm)
{
	const GPtrArray           *connections;
	GBytes                    *ssid, *ssid_temp;
	const char                *ssid_str, *ssid_str_temp;
	NMConnection              *connection;
	NMSettingWireless         *wireless_setting;
	NMSettingWirelessSecurity *wireless_security;
	guint32                    ap_wpa_flags, ap_rsn_flags;

	ssid         = nm_access_point_get_ssid(nm->ap);
	ap_wpa_flags = nm_access_point_get_wpa_flags(nm->ap);
	ap_rsn_flags = nm_access_point_get_rsn_flags(nm->ap);

	if (ssid)
		ssid_str = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid));
	else
		return;

	connections = nm_client_get_connections(nm->client);

	for (int i = 0; i < (int) connections->len; i++) {
		connection        = g_ptr_array_index(connections, i);
		if (!nm_connection_is_type(connection, NM_SETTING_WIRELESS_SETTING_NAME))
			continue;
		wireless_setting  = nm_connection_get_setting_wireless(connection);
		wireless_security = nm_connection_get_setting_wireless_security(connection);		
		ssid_temp         = nm_setting_wireless_get_ssid(wireless_setting);
		
		if (ssid_temp)
			ssid_str_temp = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid_temp, NULL), g_bytes_get_size(ssid_temp));
		else
			continue;

		if(strcmp(ssid_str, ssid_str_temp))
			continue;

		if (ap_wpa_flags == NM_802_11_AP_SEC_NONE && ap_rsn_flags == NM_802_11_AP_SEC_NONE) {
			if (nm_setting_wireless_security_get_wep_key_type(wireless_security) != NM_WEP_KEY_TYPE_KEY)
				continue;
		} else if ((ap_wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
			            || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
			            || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_SAE)) {
			if (strcmp(nm_setting_wireless_security_get_key_mgmt(wireless_security), "wpa-psk"))
				continue;
		}
		nm->connection = connection;
		return;
	}
}

int
get_ap_input(char *menu, char *prompt)
{
	int  option = -1;
	int  menusize = strlen(menu);
	int  writepipe[2], readpipe[2];
	char buffer[512] = "";
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
get_password(void)
{
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
	for (int i = 0; buffer[i] != '\0'; i++) {
		if(buffer[i] == '\n') {
			buffer[i] = '\0';
			break;
		}
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

void
get_wifi_device(NMDetails *nm)
{
	const GPtrArray *devices = nm_client_get_devices(nm->client);
	NMDevice        *device  = NULL;

	for (int i = 0; i < (int) devices->len; i++) {
		device = g_ptr_array_index(devices, i);
		if (NM_IS_DEVICE_WIFI(device)) {
			nm->device = device;
		}
	}
	if (!nm->device) {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append(nm->error, "No wifi device has been detected\n");
		nm->terminate = 1;
	}
}

int
notify(char *summary, char *body, NotifyUrgency urgency, gboolean format_summary) {
	NotifyNotification *notification;
	char *sum;

	notify_init("dmenu-wifi-prompt");
	
	if (format_summary)
		sum = get_summary(summary, body);
	else {
		sum = malloc(strlen(summary) + 1);
		strcpy(sum, summary);
	}

	notification = notify_notification_new(sum, body, "wifi-radar");
	notify_notification_set_urgency(notification, urgency);

	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
	notify_uninit();
	free(sum);
	return 0;
}

static void
scan_device_cb(GObject *device_wifi, GAsyncResult *result, gpointer user_data)
{
	GError    *error = NULL;
	NMDetails *nm    = user_data;
	
	nm_device_wifi_request_scan_finish (NM_DEVICE_WIFI(device_wifi), result, &error);

	if (error) {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append_printf(nm->error, "Error scanning wifi device: %s\n", error->message);
		g_error_free(error);
	}

	g_main_loop_quit(nm->loop);
}

int
scan_device(NMDetails *nm)
{
	if ((nm_utils_get_timestamp_msec() - nm_device_wifi_get_last_scan(NM_DEVICE_WIFI(nm->device))) < SCAN_TIMEOUT_MSEC)
		return 0;
	
	nm_device_wifi_request_scan_options_async(NM_DEVICE_WIFI(nm->device), NULL, NULL, scan_device_cb, nm);
	return 1;
}

void
init_nm_client(NMDetails **nm)
{
	GError *error  = NULL;
	(*nm) = malloc(sizeof(NMDetails));
	(*nm)->ap         = NULL;
	(*nm)->device     = NULL;
	(*nm)->connection = NULL;
	(*nm)->terminate  = FALSE;
	(*nm)->message    = NULL;
	(*nm)->error      = NULL;
	(*nm)->loop       = g_main_loop_new(NULL, FALSE);
	(*nm)->client     = nm_client_new(NULL, &error);

	if (error) {
		if (!(*nm)->error)
			(*nm)->error = g_string_new("");
		g_string_append_printf((*nm)->error, "Error initializing NetworkManager client: %s", error->message);
		(*nm)->terminate = TRUE;
		g_error_free(error);
		return;
	}
}

void
terminate_client(NMDetails *nm)
{
	if (nm->error) {
		fprintf(stderr, "dmenu-wifi-prompt errors:\n%s", nm->error->str);
		g_string_free(nm->error, TRUE);
	}
	if (nm->message) {
		notify("Wifi Prompt", nm->message->str, NOTIFY_URGENCY_NORMAL, FALSE);
		g_string_free(nm->message, TRUE);
	}

	if (nm->loop) {
		    g_main_loop_unref(nm->loop);
	}

	if (nm->client)
		g_object_unref(nm->client);

	if (nm)
		free(nm);
	exit(0);
}

int main(void) {
	NMDetails *nm = NULL;

	init_nm_client(&nm);
	if (nm->terminate)
		terminate_client(nm);

	get_wifi_device(nm);
	if (nm->terminate)
		terminate_client(nm);

	if (scan_device(nm))
		g_main_loop_run(nm->loop);

	get_access_point(nm);
	if (nm->terminate)
		terminate_client(nm);
	get_ap_connection(nm);
	if (!nm->connection) {
		add_connection(nm);

		g_main_loop_run(nm->loop);
		if (nm->terminate)
			terminate_client(nm);
	}
	activate_connection(nm);
	g_main_loop_run(nm->loop);
	terminate_client(nm);

	return 0;
}
