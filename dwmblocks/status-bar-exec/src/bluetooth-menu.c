#include <dbus/dbus.h>

#include "../include/common.h"

const char *BLUETOOTH_MENU = "󰂳 Toggle power\t0\n󰂱 Connect\t1\n󰂲 Disconnect\t2";
const char *DWMBLOCKS_SIGNAL_PATH   = "/usr/local/bin/dwmblocksctl";
const char *DWMBLOCKS_SIGNAL_ARGS[] = { "dwmblocksct", "-s", "bluetooth", NULL };

static void
toggle_bluetooth(void)
{
	DBusConnection  *conn;
	DBusMessage     *msg;
	DBusMessage     *reply;
	DBusMessageIter  args;
	DBusMessageIter  replyArgs;
	DBusMessageIter  valueIter;
	DBusError        err;

	const char  *iface   = "org.bluez.Adapter1";
	const char  *prop    = "Powered";
	dbus_bool_t  powered = FALSE;

	dbus_error_init(&err);

	if (!(conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err))) {
		logwrite("Failed to connect to the DBus system bus.", err.message, LOG_WARN, "dwmblocks-bluetooth-menu");
		dbus_error_free(&err);
		exit(EXIT_FAILURE);
	}

	msg = dbus_message_new_method_call("org.bluez", "/org/bluez/hci0",
	                                   "org.freedesktop.DBus.Properties", "Get");

	if (!msg) {
		logwrite("Failed to create DBus message.", NULL, LOG_WARN, "dwmblocks-bluetooth-menu");
		exit(EXIT_FAILURE);
	}

	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
	dbus_message_unref(msg);

	if (!reply) {
		logwrite("Failed to get Bluetooth Powered State.", err.message, LOG_WARN, "dwmblocks-bluetooth-menu");
		dbus_error_free(&err);
		exit(EXIT_FAILURE);
	}

	if (dbus_message_iter_init(reply, &replyArgs)) {
		DBusMessageIter variant;
		dbus_message_iter_recurse(&replyArgs, &variant);
		dbus_message_iter_get_basic(&variant, &powered);
	}
	dbus_message_unref(reply);

	powered = !powered;

	msg = dbus_message_new_method_call("org.bluez", "/org/bluez/hci0",
	                                   "org.freedesktop.DBus.Properties", "Set");

	if (!msg) {
		logwrite("Failed to create DBus message.", NULL, LOG_WARN, "dwmblocks-bluetooth-menu");
		exit(EXIT_FAILURE);
	}

	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);

	dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "b", &valueIter);
	dbus_message_iter_append_basic(&valueIter, DBUS_TYPE_BOOLEAN, &powered);
	dbus_message_iter_close_container(&args, &valueIter);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
	dbus_message_unref(msg);

	if (!reply) {
		logwrite("Failed to set Bluetooth Powered state.", err.message, LOG_WARN, "dwmblocks-bluetooth-menu");
		dbus_error_free(&err);
		exit(EXIT_FAILURE);
	}

	dbus_message_unref(reply);
}

int
main(void)
{
	switch (get_xmenu_option(BLUETOOTH_MENU, "dwmblocks-bluetooth-menu")) {
	case 0:
		toggle_bluetooth();
		forkexecv(DWMBLOCKS_SIGNAL_PATH, (char**) DWMBLOCKS_SIGNAL_ARGS, "dwmblocks-bluetooth-menu");
		break;
	
	default:
		break;
	}

	return 0;
}
