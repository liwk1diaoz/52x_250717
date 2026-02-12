
#ifndef CARDV_UIAPP_BLUEZ_DEVICE_MANAGER_H_
#define CARDV_UIAPP_BLUEZ_DEVICE_MANAGER_H_

#include "comm.h"

typedef struct _BlueZDeviceManager{

	struct _BlueZDBusConnection* connection;
	struct _BlueZAdapter*	adapter;
	struct _BlueZGattDB*	gatt_db;
	struct _BlueZDBusProxy*	object_manager_proxy;
	char*			adapter_object_path;
	GMainContext*	worker_context;
	GMainLoop*		event_loop;
	THREAD_HANDLE	vos_event_task_id;
	GList*			devices;
	pthread_mutex_t	device_list_lock;
	int				ref_count;

} BlueZDeviceManager;


BlueZDeviceManager* bluez_device_manager_create(void);

BlueZDeviceManager* bluez_device_manager_ref(BlueZDeviceManager* bluez_device_manager);

void 				bluez_device_manager_unref(BlueZDeviceManager* bluez_device_manager);

void 				bluez_device_manager_destroy(BlueZDeviceManager* bluez_device_manager);

struct _BlueZAdapter* 		bluez_device_manager_get_adapter(BlueZDeviceManager* bluez_device_manager);

struct _BlueZDevice*		bluez_device_manager_get_device(
						BlueZDeviceManager* bluez_device_manager,
						const char* pattern
					);

GList*				bluez_device_manager_get_discovered_devices(BlueZDeviceManager* bluez_device_manager);

#endif
