

#ifndef CARDV_UIAPP_BLUEZ_PROXY_H_
#define CARDV_UIAPP_BLUEZ_PROXY_H_

#include "comm.h"

typedef struct _BlueZDBusProxy{

	GDBusProxy* 	proxy;
	char* 			obj_path;
	int				ref_count;

} BlueZDBusProxy;

BlueZDBusProxy* bluez_dbus_proxy_create(const char* interfaceName, const char* obj_path);

void 			bluez_dbus_proxy_destroy(BlueZDBusProxy* bluez_dbus_proxy);

BlueZDBusProxy* bluez_dbus_proxy_ref(BlueZDBusProxy* bluez_dbus_prop_proxy);

void bluez_dbus_proxy_unref(BlueZDBusProxy* bluez_dbus_prop_proxy);

const char* 	bluez_dbus_proxy_get_obj_path(BlueZDBusProxy* bluez_dbus_proxy);

GVariant* bluez_dbus_proxy_call_method(
		BlueZDBusProxy* dbus_proxy,
		const char* method,
		GVariant* parameters,
		GError** error
);


#endif
