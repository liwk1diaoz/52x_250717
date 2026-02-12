
#ifndef CARDV_UIAPP_BLUEZ_DEVICE_H_
#define CARDV_UIAPP_BLUEZ_DEVICE_H_

#include "comm.h"

struct _BlueZDevice;
struct _BlueZDeviceManager;
struct _BlueZDBusProxy;
struct _BlueZDBusPropProxy;

typedef enum {

	BLUEZ_DEVICE_EVENT_CONNECTED,
	BLUEZ_DEVICE_EVENT_DISCONNECTED,
	BLUEZ_DEVICE_EVENT_PAIRED,
	BLUEZ_DEVICE_EVENT_UNPAIRED,
	BLUEZ_DEVICE_EVENT_TRUSTED,
	BLUEZ_DEVICE_EVENT_UNTRUSTED,
	BLUEZ_DEVICE_EVENT_ALIAS_CHANGED,	/* check alias through api bluez_device_get_alias */
} BLUEZ_DEVICE_EVENT;

typedef void (*bluez_device_user_callback) (struct _BlueZDevice* device, BLUEZ_DEVICE_EVENT ev, void* user_data);

typedef struct _BlueZDevice {

	struct _BlueZDBusProxy* device_proxy;
	struct _BlueZDBusPropProxy* device_prop_proxy;
	char* alias;
	char* mac;
	char* object_path;
	int ref_count;
	bool connected;
	bool trusted;
	bool paired;
	bluez_device_user_callback user_callback;
	void* user_data;

} BlueZDevice;

BlueZDevice* bluez_device_create(
		const char* mac,
		const char* object_path,
		struct _BlueZDeviceManager* device_manager
);

void bluez_device_destroy(BlueZDevice* device);

BlueZDevice* bluez_device_ref(BlueZDevice* device);
void bluez_device_unref(BlueZDevice* device);

const char* bluez_device_get_mac(BlueZDevice* device);
const char* bluez_device_get_object_path(BlueZDevice* device);
const char* bluez_device_get_alias(BlueZDevice* device);
bool		bluez_device_set_trusted(BlueZDevice* bluez_device, const bool trusted);
bool		bluez_device_is_connected(BlueZDevice* device);
bool		bluez_device_is_paired(BlueZDevice* device);
bool 		bluez_device_is_trusted(BlueZDevice* device);

bool bluez_device_connect(BlueZDevice* bluez_device);
bool bluez_device_disconnect(BlueZDevice* bluez_device);
bool bluez_device_pair(BlueZDevice* bluez_device);
bool bluez_device_unpair(BlueZDevice* bluez_device);

void bluez_device_register_user_callback(BlueZDevice* bluez_device, bluez_device_user_callback cb, void* user_data);

void bluez_device_on_prop_changed(BlueZDevice* bluez_device, GVariant* map);


#endif
