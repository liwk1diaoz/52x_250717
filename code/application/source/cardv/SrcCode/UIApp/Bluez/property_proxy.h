

#ifndef CARDV_UIAPP_BLUEZ_PROP_PROXY_H_
#define CARDV_UIAPP_BLUEZ_PROP_PROXY_H_

#include "comm.h"
#include "proxy.h"


typedef struct _BlueZDBusPropProxy{

	BlueZDBusProxy*	prop_proxy;
	int				ref_count;

} BlueZDBusPropProxy;


BlueZDBusPropProxy* bluez_dbus_prop_proxy_create(const char* obj_path);

void bluez_dbus_prop_proxy_destroy(BlueZDBusPropProxy* bluez_dbus_prop_proxy);

BlueZDBusPropProxy* bluez_dbus_prop_proxy_ref(BlueZDBusPropProxy* bluez_dbus_prop_proxy);

void bluez_dbus_prop_proxy_unref(BlueZDBusPropProxy* bluez_dbus_prop_proxy);

const char* bluez_dbus_prop_proxy_get_obj_path(BlueZDBusPropProxy* bluez_dbus_prop_proxy);

bool bluez_dbus_prop_proxy_get_boolean_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	bool* result
);

bool bluez_dbus_prop_proxy_get_string_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	char** result
);


typedef void (*bluez_dbus_prop_proxy_get_array_string_callback)(char* str, void* user_data);

bool bluez_dbus_prop_proxy_get_array_string(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	bluez_dbus_prop_proxy_get_array_string_callback callback,
	void* user_data
);

bool bluez_dbus_prop_proxy_set_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	GVariant*	value
);

bool bluez_dbus_prop_proxy_set_bool_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	const bool	value
);

bool bluez_dbus_prop_proxy_set_string_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	const char* value
);

#endif
