#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <NetworkManager.h>
#include "nm-dbus-interface.h"

#include "../include/colorscheme.h"
#include "../include/common.h"

/* constants and paths */
const char *iconarray[] = {"x", "tdenetworkmanager", "wifi-radar"};
const char *staticarr[] = {CLR_9"󰤮  "NRM, CLR_6"  "NRM, CLR_6"󰤯  "NRM, CLR_6"󰤟  "NRM, CLR_6"󰤢  "NRM, CLR_6"󰤥  "NRM, CLR_6"󰤨  "NRM, CLR_9"󰤫  "NRM};
const char *nmtuipath[] = {"usr", "local", "bin", "st", NULL};
const char *nmtuiargs[] = {"st", "-e", "nmtui", NULL};
const char *wifipath[]  = {"$HOME", ".local", "bin", "dmenu", "dmenu-wifi-prompt", NULL};
const char *wifiargs[]  = {"dmenu-wifi-prompt", NULL};

/* structs */
struct ip {
	NMIPConfig *cfg;
	GPtrArray  *arr;
	const char *add;
	const char *gat;
};

/* function definitions */
void append_common_info(NMDevice *device, GString *string);
void append_ethernet_info(NMDevice *device, GString *string);
void append_wifi_info(NMDevice *device, GString *string);
void checkexec(NMClient *client, int iconindex);
int  get_active_state(NMClient *client);
void get_devices_info(NMClient *client, const int index);
void init_nm_client(NMClient **client);

void
append_common_info(NMDevice *device, GString *string)
{
	NMActiveConnection *ac;
	struct ip          ip4, ip6;
	const char         *mac;

	ac = nm_device_get_active_connection(device);

	if (!ac)
		return;

	mac = nm_device_get_hw_address(device);

	ip4.cfg = nm_active_connection_get_ip4_config(ac);
	ip4.arr = nm_ip_config_get_addresses(ip4.cfg);
	ip4.add = nm_ip_address_get_address(g_ptr_array_index(ip4.arr, 0));
	ip4.gat = nm_ip_config_get_gateway(ip4.cfg);

	ip6.cfg = nm_active_connection_get_ip6_config(ac);
	ip6.arr = nm_ip_config_get_addresses(ip6.cfg);
	ip6.add = nm_ip_address_get_address(g_ptr_array_index(ip6.arr, 0));

	g_string_append_printf(string, "MAC:  %s\nIPv4: %s\nGate: %s\nIPv6: %s\n\n",
	                       mac, ip4.add, ip4.gat, ip6.add);
}

void
append_ethernet_info(NMDevice *device, GString *string)
{
	NMActiveConnection *ac;

	ac = nm_device_get_active_connection(device);

	if (!ac)
		g_string_append(string, "Ethernet: Disconnected\n\n");
	else
		g_string_append(string, "Ethernet\n");
}

void
append_wifi_info(NMDevice *device, GString *string)
{
	GBytes             *ssid;
	GString            *ssid_str;
	guint32            freq;
	guint8             strength;
	NMActiveConnection *ac;
	NMAccessPoint      *ap;

	ac = nm_device_get_active_connection(device);

	if (!ac) {
		g_string_append(string, "Wifi: Disconnected\n\n");
		return;
	}

	ap       = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device));
	ssid     = nm_access_point_get_ssid(ap);
	freq     = nm_access_point_get_frequency(ap);
	strength = nm_access_point_get_strength(ap);

	if (ssid)
		ssid_str = g_string_new(nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL),
		                        g_bytes_get_size(ssid)));
	else
		ssid_str = g_string_new("*hidden_network*");

	g_string_append_printf(string, "Wifi\nSSID: %s\nStrn: %d%% | Freq: %d GHz\n",
	                       ssid_str->str, strength, freq);

	g_string_free(ssid_str, TRUE);
}

void
checkexec(NMClient *client, int iconindex)
{
	char *env;
	char *path;

	if (!(env = getenv("BLOCK_BUTTON")))
		return;

	switch (env[0] - '0') {
		case 1:
			get_devices_info(client, iconindex);
			break;

		case 2:
			path = get_path((char**) nmtuipath, 1);
			forkexecv(path, (char**) nmtuiargs, "dwmblocks-internet");
			free(path);
			break;

		case 3:
			path = get_path((char**) wifipath, 1);
			forkexecv(path, (char**) wifiargs, "dwmblocks-internet");
			free(path);
			break;

		default:
			break;
	}
}

int
get_active_state(NMClient *client)
{
	NMActiveConnection *ac;
	NMAccessPoint      *ap;
	NMDeviceWifi       *device;
	const GPtrArray    *act_devices;
	const char         *type;
	int                state;

	ac = nm_client_get_primary_connection(client);
	
	if (!ac)
		return 0;
	
	type = nm_active_connection_get_connection_type(ac);

	if (strcmp(type, "802-3-ethernet") == 0)
		return 1;

	if (strcmp(type, "802-11-wireless") == 0) {
		act_devices = nm_active_connection_get_devices(ac);

		if (!act_devices || act_devices->len < 1) {
			log_string("Wifi active devices less than 1", "dwmblocks-internet");
			return 7;
		}

		device = NM_DEVICE_WIFI(g_ptr_array_index(act_devices, 0));

		if (!device) {
			log_string("Could not fetch wifi device", "dwmblocks-internet");
			return 7;
		}

		ap = nm_device_wifi_get_active_access_point(device);

		if (!ap) {
			log_string("Could not fetch active access point", "dwmblocks-internet");
			return 7;
		}

		state = (nm_access_point_get_strength(ap) / 20);
		state = state > 4 ? 4 : state;

		return (2 + state);
	}

	return 0;
}

void
get_devices_info(NMClient *client, const int index)
{
	const GPtrArray *devices;
	NMDevice        *device;
	GString         *string;
	int             iconindex;

	devices   = nm_client_get_devices(client);
	iconindex = index > 2 ? 2 : index;

	if (!devices) {
		notify("Network Devices Info", "No network devices detected", (char*) iconarray[0], NOTIFY_URGENCY_NORMAL, 1);
		return;
	}

	string = g_string_new("");

	for (int i = 0; i < (int) devices->len; i++) {
		device = g_ptr_array_index(devices, i);

		if (!device) {
			log_string("Failed to get device", "dwmblocks-internet");
			return;
		}

		int type = nm_device_get_device_type(device);
		switch (type) {
			case NM_DEVICE_TYPE_ETHERNET:
				append_ethernet_info(device, string);
				goto INFO;

			case NM_DEVICE_TYPE_WIFI:
				append_wifi_info(device, string);
			INFO:
				append_common_info(device, string);

			default:
				break;
		}
	}

	if (string->len <= 1)
		notify("Network Devices Info", "No network devices detected", (char*) iconarray[iconindex], NOTIFY_URGENCY_NORMAL, 1);
	else
		notify("Network Devices Info", string->str, (char*) iconarray[iconindex], NOTIFY_URGENCY_NORMAL, 1);

	g_string_free(string, TRUE);
}

void
init_nm_client(NMClient **client)
{
	GError *error = NULL;
	*client       = nm_client_new(NULL, &error);

	if (error) {
		char log[256];
		sprintf(log, "Error initializing NetworkManager client: %s", error->message);
		log_string(log, "dwmblocks-internet");
	}
}

int
main(void)
{
	NMClient *client;
	int      state;

	state = 0;
	init_nm_client(&client);

	if (client) {
		state = get_active_state(client);
		checkexec(client, state);
		g_object_unref(client);
	}

	printf(" "BG_1" %s\n", staticarr[state]);

	return 0;
}
