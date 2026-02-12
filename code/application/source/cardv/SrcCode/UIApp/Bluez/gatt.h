
#ifndef CARDV_UIAPP_BLUEZ_GATT_H_
#define CARDV_UIAPP_BLUEZ_GATT_H_

#include "comm.h"

struct _BlueZGattChar;
struct _BlueZDeviceManager;

typedef struct _BlueZGattCharValue{

	uint8_t bytes[1024];
	size_t len;
	bool is_string;

} BlueZGattCharValue;

typedef void (*bluez_gatt_char_value_changed_callback)(const struct _BlueZGattChar* bluez_gatt_char, const struct _BlueZGattCharValue* value, void* user_data);

typedef struct _BlueZGattChar {

	struct _BlueZDBusPropProxy* prop_proxy;
	struct _BlueZDBusProxy* proxy;
	char* object_path;
	char* uuid;
	char* service;
	bool read_only;
	bool notifying;
	uint16_t handle;
	int ref_count;
	struct _BlueZGattCharValue value;
	bluez_gatt_char_value_changed_callback value_changed_callback;
	void* value_changed_user_data;

	struct {
		unsigned char	read:1;
		unsigned char	write:1;
		unsigned char	notify:1;
		unsigned char	indicate:1;
	} flags;

} BlueZGattChar;

typedef struct _BlueZGattDB{

	GList* 	char_list;
	GList* 	service_list;
	int		ref_count;

} BlueZGattDB;

typedef struct {
	struct _BlueZDBusPropProxy* prop_proxy;
	struct _BlueZDBusProxy* proxy;
	char *path;
	uint16_t handle;
	char *uuid;
	bool primary;
	GList *chrcs;
	GList *inc;
} BlueZGattLocalService;

BlueZGattDB* bluez_gatt_db_create(void);

void bluez_gatt_db_destroy(BlueZGattDB* bluez_gatt_db);

BlueZGattDB* bluez_gatt_db_ref(BlueZGattDB* bluez_gatt_db);

void bluez_gatt_db_unref(BlueZGattDB* bluez_gatt_db);

bool bluez_gatt_db_add_char(BlueZGattDB* gatt_db, BlueZGattChar* bluez_gatt_char);

bool bluez_gatt_db_remove_char(BlueZGattDB* gatt_db, char* pattern);

bool bluez_gatt_db_char_start_notify(BlueZGattDB* gatt_db, char* pattern);

bool bluez_gatt_db_char_stop_notify(BlueZGattDB* gatt_db, char* pattern);

bool bluez_gatt_db_char_read_value(BlueZGattDB* gatt_db, char* pattern, uint32_t offset);

void bluez_gatt_db_char_on_prop_changed(BlueZGattDB* gatt_db, const char* pattern, GVariant* map);

BlueZGattChar* bluez_gatt_db_find_char(BlueZGattDB* gatt_db, const char* pattern);

/* characteristic */

BlueZGattChar* bluez_gatt_char_create(const char* object_path);

void bluez_gatt_char_destroy(BlueZGattChar* bluez_gatt_char);

BlueZGattChar* bluez_gatt_char_ref(BlueZGattChar* bluez_gatt_char);

void bluez_gatt_char_unref(BlueZGattChar* bluez_gatt_char);

bool bluez_gatt_char_start_notify(BlueZGattChar* bluez_gatt_char);

bool bluez_gatt_char_stop_notify(BlueZGattChar* bluez_gatt_char);

bool bluez_gatt_char_register_value_changed_callback(BlueZGattChar* bluez_gatt_char, bluez_gatt_char_value_changed_callback cb, void* user_data);

bool bluez_gatt_char_read_value(BlueZGattChar* gatt_db, uint32_t offset);

BlueZGattLocalService* bluez_gatt_local_service_register(struct _BlueZDeviceManager *manager, char* uuid, uint16_t handle);

#endif
