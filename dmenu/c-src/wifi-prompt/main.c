#include "nm-core-types.h"
#include "nm-dbus-interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <NetworkManager.h>

#define SCAN_TIMEOUT_MSEC 5000
#define RELOAD_TIMEOUT_MSEC 7500

#include "output.h"
#include "input.h"

/* struct declerations*/

typedef struct {
	gboolean           terminate;
	GMainLoop          *loop;
	NMClient           *client;
	NMDevice           *device;
	NMAccessPoint      *ap;
	NMConnection       *connection;
	GString            *message;
	GString            *error;
	NMActiveConnection *active_connection;
} NMDetails;

/* function declerations */

/* asynchronus */

static void activate_connection_cb(GObject *client, GAsyncResult *result, gpointer user_data);
static void activate_connection(NMDetails *nm);
static void add_connection_cb(GObject *client, GAsyncResult *result, gpointer user_data);
static void add_connection(NMDetails *nm);
static void reload_client_cb(GObject *client, GAsyncResult *result, gpointer user_data);
static void reload_client(NMDetails *nm);
static void remove_active_connection_cb(GObject *remote_connection, GAsyncResult *result, gpointer user_data);
static void remove_active_connection(NMDetails *nm);
static void scan_device_cb(GObject *device_wifi, GAsyncResult *result, gpointer user_data);
static int  scan_device(NMDetails *nm);

/* synchronus */

void     init_nm_client(NMDetails **nm);
void     terminate_client(NMDetails *nm);
GString* aps_to_string(const GPtrArray *aps);
void     get_access_point(NMDetails *nm);
void     get_ap_connection(NMDetails *nm);
void     get_result(NMDetails *nm);
GString* get_security(NMAccessPoint *ap);
GString* get_ssid(NMAccessPoint* ap);
void     get_state(NMDetails *nm);
void     get_wifi_device(NMDetails *nm);


/* code */

/* asynchronus functions */

/* activate connection*/

static void
activate_connection_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
	NMDetails  *nm    = user_data;
	GError     *error = NULL;
		
	nm->active_connection = nm_client_activate_connection_finish(NM_CLIENT(client), result, &error);

	if (error) {
		if (!nm->error)
			nm->error = g_string_new("");
		
		g_string_append_printf(nm->error, "Error activating connection: %s\n", error->message);
		g_error_free(error);
		nm->terminate = TRUE;
	}

	g_main_loop_quit(nm->loop);
}

static void
activate_connection(NMDetails *nm)
{
	nm_client_activate_connection_async(nm->client, NULL, nm->device, nm_object_get_path(NM_OBJECT(nm->ap)), NULL, activate_connection_cb, nm);
}

/* add connection*/

static void
add_connection_cb(GObject *client, GAsyncResult *result, gpointer user_data)
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
	static GString            *ssid;
	guint32                   ap_wpa_flags, ap_rsn_flags;
	const char                *uuid;
	const char                *password;
	const char                *ssid_str;
	int                       ssid_len;

	nm->connection = nm_simple_connection_new();
	s_wireless     = (NMSettingWireless *) nm_setting_wireless_new();
	s_connection   = (NMSettingConnection *) nm_setting_connection_new();
	s_wsecurity    = (NMSettingWirelessSecurity*) nm_setting_wireless_security_new();
	ssid           = get_ssid(nm->ap);
	uuid           = nm_utils_uuid_generate();
	password       = NULL;
	ssid_len       = ssid->len;
	ssid_str       = g_strdup(ssid->str);

	g_object_set(G_OBJECT(s_connection),
	             NM_SETTING_CONNECTION_UUID, uuid,
	             NM_SETTING_CONNECTION_ID, ssid_str,
	             NM_SETTING_CONNECTION_TYPE, "802-11-wireless",
	             NULL);
	nm_connection_add_setting(nm->connection, NM_SETTING(s_connection));
	g_object_set(G_OBJECT(s_wireless),
	             NM_SETTING_WIRELESS_SSID, g_bytes_new(ssid_str, ssid_len),
	             NULL);

	nm_connection_add_setting(nm->connection, NM_SETTING(s_wireless));


	ap_wpa_flags = nm_access_point_get_wpa_flags(nm->ap);
	ap_rsn_flags = nm_access_point_get_rsn_flags(nm->ap);

	if ((ap_wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK) || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)) {
		password = get_password();
		g_object_set(G_OBJECT(s_wsecurity),
			     NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
			     NM_SETTING_WIRELESS_SECURITY_PSK, password, NULL);
		nm_connection_add_setting(nm->connection, NM_SETTING(s_wsecurity));
	} else if ((ap_wpa_flags = NM_802_11_AP_SEC_NONE) && (ap_wpa_flags = NM_802_11_AP_SEC_NONE)) {

	} else {
		if(!nm->message)
			nm->message = g_string_new("");
		g_string_append_printf(nm->message, "Security authentication is not supported for: %s", ssid_str);
		nm->terminate = TRUE;
		return;
	}
	
	s_ip4 = (NMSettingIP4Config *)nm_setting_ip4_config_new();
	g_object_set(G_OBJECT(s_ip4),
	             NM_SETTING_IP_CONFIG_METHOD,
	             NM_SETTING_IP4_CONFIG_METHOD_AUTO,
	             NULL);
	nm_connection_add_setting(nm->connection, NM_SETTING(s_ip4));

	if (ssid)
		g_string_free(ssid, TRUE);

	nm_client_add_connection_async(nm->client, nm->connection, TRUE, NULL, add_connection_cb, nm);
}

/* reload client*/

static void 
reload_client_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
	GError *error = NULL;
	NMDetails *nm = (NMDetails*) user_data;

	nm_client_reload_connections_finish(NM_CLIENT(client), result, &error);
	g_main_loop_quit(nm->loop);
}

static void
reload_client(NMDetails *nm)
{
	nm_client_reload_connections_async(nm->client, NULL, reload_client_cb, nm);
}

/* remove active connection*/

static void 
remove_active_connection_cb(GObject *remote_connection, GAsyncResult *result, gpointer user_data)
{
	GError *error = NULL;
	NMDetails *nm = (NMDetails*) user_data;

	nm_remote_connection_delete_finish(NM_REMOTE_CONNECTION(remote_connection), result, &error);

	if (error) {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append_printf(nm->error, "Error deleating the connection: %s\n", error->message);
		g_error_free(error);
		nm->terminate = TRUE;
	}

	g_main_loop_quit(nm->loop);
}

static void
remove_active_connection(NMDetails *nm)
{
	NMRemoteConnection *remote_connection = NULL;

	remote_connection = nm_active_connection_get_connection(nm->active_connection);

	nm_remote_connection_delete_async(remote_connection, NULL, remove_active_connection_cb, nm);
}

/* scan device */

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

static int
scan_device(NMDetails *nm)
{
	if ((nm_utils_get_timestamp_msec() - nm_device_wifi_get_last_scan(NM_DEVICE_WIFI(nm->device))) < SCAN_TIMEOUT_MSEC)
		return 0;
	
	nm_device_wifi_request_scan_options_async(NM_DEVICE_WIFI(nm->device), NULL, NULL, scan_device_cb, nm);
	return 1;
}


/* synchronus */


/* construct - destruct main context */

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
	(*nm)->active_connection = NULL;

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

	if (nm->active_connection)
		g_object_unref(nm->active_connection);
	if (nm->loop)
		g_main_loop_unref(nm->loop);
	if (nm->client)
		g_object_unref(nm->client);
	if (nm)
		free(nm);
}

/* data manipulation */

GString*
aps_to_string(const GPtrArray *aps)
{
	NMAccessPoint *ap;
	GString       *security, *ssid, *string;
	gsize         ssid_max_size = 4, security_max_size = 8;
	
	string = g_string_new("");

	for (int i = 0; i < (int) aps->len; i++) {
		ap       = g_ptr_array_index(aps, i);
		security = get_security(ap);
		ssid     = get_ssid(ap);

		if (ssid->len > ssid_max_size)
			ssid_max_size = ssid->len;
		
		if (security->len > security_max_size)
			security_max_size = security->len;

		if (security)
			g_string_free(security, TRUE);

		if (ssid)
			g_string_free(ssid, TRUE);
	}

	g_string_append_printf(string, "%-*s   Strength   %*s   Frequency\t-1\n",
	                       (int) ssid_max_size, "SSID", (int) security_max_size, "Security");

	for (int i = 0; i < (int) aps->len; i++) {
		ap        = g_ptr_array_index(aps, i);
		security  = get_security(ap);
		ssid      = get_ssid(ap);

		g_string_append_printf(string, "%-*s   %8u   %*s    %d MHz\t%d\n",
		                       (int) ssid_max_size, ssid->str,
				       nm_access_point_get_strength(ap),
		                       (int) security_max_size, security->str,
				       nm_access_point_get_frequency(ap), i);
		if(ssid)
			g_string_free(ssid, TRUE);
		if(security)
			g_string_free(security, TRUE);
	}

	return string;
}

void
get_access_point(NMDetails *nm)
{
	const GPtrArray *aps;
	GString         *string;
	int             return_index;

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

	string = aps_to_string(aps);
	return_index = get_ap_input(string);

	if (return_index != -1) {
		nm->ap = g_ptr_array_index(aps, return_index);
	} else {
		if (!nm->error)
			nm->error = g_string_new("");
		g_string_append(nm->error, "No access point selected\n");
		nm->terminate = TRUE;
	}

	if (string)
		g_string_free(string, TRUE);
}

void
get_ap_connection(NMDetails *nm)
{
	const GPtrArray           *connections;
	GString                   *ssid, *ssid_temp;
	GBytes                    *ssid_temp_bytes;
	NMConnection              *connection;
	NMSettingWireless         *wireless_setting;
	NMSettingWirelessSecurity *wireless_security;
	guint32                    ap_wpa_flags, ap_rsn_flags;

	ssid         = get_ssid(nm->ap);
	ssid_temp    = NULL;
	ap_wpa_flags = nm_access_point_get_wpa_flags(nm->ap);
	ap_rsn_flags = nm_access_point_get_rsn_flags(nm->ap);

	connections = nm_client_get_connections(nm->client);

	for (int i = 0; i < (int) connections->len; i++) {
		connection        = g_ptr_array_index(connections, i);
		if (!nm_connection_is_type(connection, NM_SETTING_WIRELESS_SETTING_NAME))
			continue;
		wireless_setting  = nm_connection_get_setting_wireless(connection);
		wireless_security = nm_connection_get_setting_wireless_security(connection);		
		ssid_temp_bytes   = nm_setting_wireless_get_ssid(wireless_setting);
		
		if (ssid_temp_bytes)
			ssid_temp = g_string_new(nm_utils_ssid_to_utf8(g_bytes_get_data(ssid_temp_bytes, NULL), g_bytes_get_size(ssid_temp_bytes)));
		else
			continue;

		if(!g_string_equal(ssid, ssid_temp))
			continue;

		if ((ap_wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK) || (ap_rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)) {
			if (strcmp(nm_setting_wireless_security_get_key_mgmt(wireless_security), "wpa-psk"))
				continue;
		} else {
			if (nm_setting_wireless_security_get_key_mgmt(wireless_security) != NULL)
				continue;
		}
		nm->connection = connection;
		break;
	}
	if (ssid_temp)
		g_string_free(ssid_temp, TRUE);
	if (ssid)
		g_string_free(ssid, TRUE);
}

void
get_result(NMDetails *nm)
{
	const int msec_iteration = 500;
	signal_dwmblocks();
	get_state(nm);
	for (int i = 0; i < (RELOAD_TIMEOUT_MSEC/msec_iteration) && !nm->terminate; i++) {
		msleep(msec_iteration);
		reload_client(nm);
		g_main_loop_run(nm->loop);
		signal_dwmblocks();
		get_state(nm);
	}

	if (!nm->terminate) {
		GString *ssid = get_ssid(nm->ap);
		nm->terminate = TRUE;

		if(!nm->message)
			nm->message = g_string_new("");
		g_string_append_printf(nm->message, "Failed to activate connection:\n%s\n", ssid->str);
		remove_active_connection(nm);
		g_main_loop_run(nm->loop);
	}
}

GString*
get_security(NMAccessPoint *ap)
{
	guint32 flags, wpa_flags, rsn_flags;
	GString *security_str;

	flags        = nm_access_point_get_flags(ap);
	wpa_flags    = nm_access_point_get_wpa_flags(ap);
	rsn_flags    = nm_access_point_get_rsn_flags(ap);
	security_str = g_string_new(NULL);

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

	return security_str;
}

GString*
get_ssid(NMAccessPoint* ap)
{
	GBytes  *ssid = NULL;
	GString *string = NULL;

	ssid   = nm_access_point_get_ssid(ap);
	string = g_string_new("");

	if (ssid)
		g_string_append(string, nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid)));
	else
		g_string_append(string, "*hidden network*");

	return string;
}

void
get_state(NMDetails *nm)
{
	NMActiveConnection *active_connection = nm_device_get_active_connection(nm->device);
	if (!active_connection)
		return;
	NMActiveConnectionState state = nm_active_connection_get_state(active_connection);
	GString *ssid;
	nm->message = NULL;

	switch (state) {
		case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
			ssid  = get_ssid(nm->ap);
			nm->terminate = TRUE;
			if(!nm->message)
				nm->message = g_string_new("");
			if (!strcmp(nm_active_connection_get_uuid(active_connection), nm_active_connection_get_uuid(nm->active_connection))) {		
				g_string_append_printf(nm->message, "Successfuly activated connection:\n%s", ssid->str);
			} else {
				g_string_append_printf(nm->message, "Failed to activate connection:\n%s\n", ssid->str);
				remove_active_connection(nm);
				g_main_loop_run(nm->loop);
			}
			break;

		case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
			ssid  = get_ssid(nm->ap);
			if(!nm->message)
				nm->message = g_string_new("");
			g_string_append_printf(nm->message, "Failed to activate connection:\n%s\n", ssid->str);
			nm->terminate = TRUE;
			remove_active_connection(nm);
			g_main_loop_run(nm->loop);

			break;
		default:
			break;
	}
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
main(void)
{
	NMDetails *nm = NULL;

	init_nm_client(&nm);
	if (nm->terminate) {
		terminate_client(nm);
		return 0;
	}

	get_wifi_device(nm);
	if (nm->terminate) {
		terminate_client(nm);
		return 0;
	}

	if (scan_device(nm))
		g_main_loop_run(nm->loop);

	get_access_point(nm);
	if (nm->terminate) {
		terminate_client(nm);
		return 0;
	}

	get_ap_connection(nm);

	if (!nm->connection) {
		add_connection(nm);
		g_main_loop_run(nm->loop);
		if (nm->terminate) {
			terminate_client(nm);
			return 0;
		}
	}

	activate_connection(nm);
	g_main_loop_run(nm->loop);
	if (nm->terminate) {
		terminate_client(nm);
		return 0;
	}

	get_result(nm);
	terminate_client(nm);

	return 0;
}
