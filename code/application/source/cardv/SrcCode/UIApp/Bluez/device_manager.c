
#include "adapter.h"
#include "device_manager.h"
#include "device.h"
#include "gatt.h"
#include "proxy.h"
#include "property_proxy.h"
#include "connection.h"

#define OBJECT_PATH_ROOT "/"

#define DEVICE_LIST_LOCK(X)		pthread_mutex_lock(&(X->device_list_lock))
#define DEVICE_LIST_UNLOCK(X)	pthread_mutex_unlock(&(X->device_list_lock))

static void add_device_from_map(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path,
		GVariant* map
);


static bool device_probe(BlueZDeviceManager* bluez_device_manager)
{
	GError* error = NULL;
	GVariantIter iter;
	GVariant* ret = bluez_dbus_proxy_call_method(
							bluez_device_manager->object_manager_proxy,
							OBJECT_MANAGER_INTERFACE_METHOD_GET_MANAGED_OBJS,
							NULL,
							&error);

    if(error){
    	DBG_ERR("GetManagedObjects failed, %s\n", error->message);
    	g_error_free(error);
    	return false;
	}

	ret = g_variant_get_child_value(ret, 0);

    g_variant_iter_init(&iter, ret);

    GVariant* value = NULL;
    GVariant* value2 = NULL;
    char* key = NULL;

    while(g_variant_iter_next(&iter, "{&o*}", &key, &value))
    {
    	DBG_IND("key = %s\n", key);

    	if(!bluez_device_manager->adapter_object_path && g_variant_lookup(value, BLUEZ_ADAPTER_INTERFACE, "*", &value2)){

    	    	bluez_device_manager->adapter_object_path = g_strdup(key);
    	}

    	if(g_variant_lookup(value, BLUEZ_DEVICE_INTERFACE, "*", &value2)){
    		add_device_from_map(bluez_device_manager, key, value);
    	}

    	g_variant_unref(value);
    }

    g_variant_unref(ret);

    return true;
}

static bool adapter_initialize(BlueZDeviceManager* bluez_device_manager)
{

	if(!bluez_device_manager->adapter_object_path){
		DBG_ERR("adapter_object_path can't be null!\n");
		return false;
	}

	BlueZAdapter* adapter = bluez_adapter_create(bluez_device_manager->adapter_object_path);

	if(!adapter){
		DBG_ERR("create adapter failed!\n");
		return false;
	}

	bluez_device_manager->adapter = adapter;

	return true;
}

static gint device_object_path_compare_func(gconstpointer  ele, gconstpointer  find)
{
	BlueZDevice* device = (BlueZDevice*) ele;
	char* object_path =  (char*) find;

	return strcmp(device->object_path, object_path);
}


static void on_interface_removed(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path)
{
	DBG_IND("interface removed %s\n", object_path);

	DEVICE_LIST_LOCK(bluez_device_manager);

	GList *found;
	found = g_list_find_custom(bluez_device_manager->devices, object_path, device_object_path_compare_func);

	if(found){
		BlueZDevice* bluez_device = (BlueZDevice*) found->data;
		bluez_device_manager->devices = g_list_remove(bluez_device_manager->devices, bluez_device);
		bluez_device_unref(bluez_device);
	}
	else{
		DBG_ERR("device(%s) not found\n", object_path);
	}

	DEVICE_LIST_UNLOCK(bluez_device_manager);

}

static bool check_map_contains_device(GVariant* map)
{
    GVariant* value = NULL;
    g_variant_lookup(map, BLUEZ_DEVICE_INTERFACE, "*", &value);

    return value ? true : false;
}

static bool check_map_contains_gatt_char(GVariant* map)
{
    GVariant* value = NULL;
    g_variant_lookup(map, GATT_CHAR_INTERFACE, "*", &value);

    return value ? true : false;
}

static void add_device_from_map(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path,
		GVariant* map
)
{
	GVariant* value = NULL;

	if(g_variant_lookup(map, BLUEZ_DEVICE_INTERFACE, "*", &value) && value){

		char* mac = NULL;

		if(g_variant_lookup(value, BLUEZ_DEVICE_INTERFACE_PROP_ADDRESS, "&s", &mac) && mac){

			BlueZDevice* device = 	bluez_device_create(
											mac,
											object_path,
											bluez_device_manager
									);

			if(device){
				DEVICE_LIST_LOCK(bluez_device_manager);
				bluez_device_manager->devices = g_list_append(bluez_device_manager->devices, device);
				DEVICE_LIST_UNLOCK(bluez_device_manager);
			}
		}
	}
}

static void add_gatt_char_from_map(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path,
		GVariant* map
)
{
	GVariant* value = NULL;

	g_variant_lookup(map, GATT_CHAR_INTERFACE, "*", &value);

	if(value){

    	BlueZGattChar* gatt_chr = 	bluez_gatt_char_create(object_path);

    	if(gatt_chr){

    		DBG_IND("UUID:%s Service:%s\n", gatt_chr->uuid, gatt_chr->service);

    		bluez_gatt_db_add_char(bluez_device_manager->gatt_db, gatt_chr);
    	}
	}
}

static void on_interface_added(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path,
		GVariant* map)
{

	DBG_IND("interface added %s\n", object_path);

	if(check_map_contains_device(map))
		add_device_from_map(bluez_device_manager, object_path, map);

	if(check_map_contains_gatt_char(map))
		add_gatt_char_from_map(bluez_device_manager, object_path, map);
}

static void interfaces_added_callback(
    GDBusConnection* conn,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* parameters,
    gpointer bluez_device_manager)
{
	char* added_object_path = NULL;
    GVariant* value = NULL;

    g_variant_get_child(parameters, 0, "&o", &added_object_path);
    value = g_variant_get_child_value(parameters, 1);

    on_interface_added((BlueZDeviceManager*)bluez_device_manager, added_object_path, value);

    g_variant_unref(value);
}

static void interfaces_removed_callback(
    GDBusConnection* conn,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* variant,
    gpointer bluez_device_manager)
{

    char* removed_obj_path = NULL;

    g_variant_get(variant, "(oas)", &removed_obj_path, NULL);

    on_interface_removed((BlueZDeviceManager*)bluez_device_manager, removed_obj_path);

}

static void on_device_prop_changed(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path,
		GVariant*	map)
{
	DEVICE_LIST_LOCK(bluez_device_manager);

	GList *found;
	found = g_list_find_custom(bluez_device_manager->devices, object_path, device_object_path_compare_func);

	if(found){
		bluez_device_on_prop_changed((BlueZDevice*)found->data, map);
	}
	else{
		DBG_ERR("device(%s) not found\n", object_path);
	}

	DEVICE_LIST_UNLOCK(bluez_device_manager);
}

static void on_gatt_char_prop_changed(
		BlueZDeviceManager* bluez_device_manager,
		const char* object_path,
		GVariant*	map)
{
	bluez_gatt_db_char_on_prop_changed(bluez_device_manager->gatt_db, object_path, map);
}

static void on_prop_changed(
		BlueZDeviceManager* bluez_device_manager,
	    const char* prop_owner,
		const char* object_path,
		GVariant*	map)
{

	if(!strcmp(prop_owner, BLUEZ_DEVICE_INTERFACE)){
		on_device_prop_changed(bluez_device_manager, object_path, map);
	}
	else if(!strcmp(prop_owner, BLUEZ_ADAPTER_INTERFACE)){

	}
	else if(!strcmp(prop_owner, GATT_CHAR_INTERFACE)){

		on_gatt_char_prop_changed(bluez_device_manager, object_path, map);
	}
}

static void prop_changed_callback(
    GDBusConnection* conn,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* variant,
    gpointer bluez_device_manager)
{
	char* prop_owner;
	GVariant* map = NULL;
    g_variant_get_child(variant, 0, "&s", &prop_owner);
    map = g_variant_get_child_value(variant, 1);

    DBG_IND("prop changed %s %s\n", prop_owner, g_variant_get_type_string(map));

    on_prop_changed((BlueZDeviceManager*)bluez_device_manager, prop_owner, object_path, map);

    g_variant_unref(map);
}


static THREAD_RETTYPE event_task(void* arg)
{
	BlueZDeviceManager* bluez_device_manager = (BlueZDeviceManager*) arg;

	g_main_context_push_thread_default(bluez_device_manager->worker_context);

	DBG_IND("subscribe signal\n");

	bluez_dbus_connection_subscribe_signal(
			bluez_device_manager->connection,
			BLUEZ_SERVICE_NAME,
			OBJECT_MANAGER_INTERFACE,
			OBJECT_MANAGER_INTERFACE_EVENT_INTERFACE_ADDED,
			NULL,
			interfaces_added_callback,
			(gpointer) bluez_device_manager
	);

	bluez_dbus_connection_subscribe_signal(
			bluez_device_manager->connection,
			BLUEZ_SERVICE_NAME,
			OBJECT_MANAGER_INTERFACE,
			OBJECT_MANAGER_INTERFACE_EVENT_INTERFACE_REMOVED,
			NULL,
			interfaces_removed_callback,
			(gpointer) bluez_device_manager
	);


	bluez_dbus_connection_subscribe_signal(
			bluez_device_manager->connection,
			BLUEZ_SERVICE_NAME,
			PROPERTIES_INTERFACE,
			PROPERTIES_INTERFACE_EVENT_PROP_CHANGED,
			NULL,
			prop_changed_callback,
			(gpointer) bluez_device_manager
	);

	DBG_IND("event loop is running\n");
	g_main_loop_run(bluez_device_manager->event_loop);

	THREAD_RETURN(0);
}

static bool device_manager_init(BlueZDeviceManager* bluez_device_manager)
{
	if(!device_probe(bluez_device_manager))
		return false;


	if(!adapter_initialize(bluez_device_manager))
		return false;

	bluez_device_manager->vos_event_task_id = vos_task_create(event_task, bluez_device_manager, "bluez_event_task", 10, 8192);

	if(!bluez_device_manager->vos_event_task_id){
		DBG_ERR("create task failed\n");
		return false;
	}

	vos_task_resume(bluez_device_manager->vos_event_task_id);

	return true;
}

BlueZDeviceManager* bluez_device_manager_ref(BlueZDeviceManager* bluez_device_manager)
{
	if(!bluez_device_manager)
		return NULL;

	__sync_fetch_and_add(&bluez_device_manager->ref_count, 1);

	return bluez_device_manager;
}

void bluez_device_manager_unref(BlueZDeviceManager* bluez_device_manager)
{
	if (!bluez_device_manager)
		return;

	if (__sync_sub_and_fetch(&bluez_device_manager->ref_count, 1))
		return;

	bluez_device_manager_destroy(bluez_device_manager);
}

BlueZAdapter* bluez_device_manager_get_adapter(BlueZDeviceManager* bluez_device_manager)
{
	return bluez_device_manager->adapter;
}

static gint bluez_device_compare_pattern_callback(gconstpointer  bluez_device, gconstpointer  pattern)
{
	BlueZDevice* real_bluez_device = (BlueZDevice*) bluez_device;
	char* real_pattern = (char*) pattern;

	if(!strcmp(real_bluez_device->object_path, real_pattern))
		return 0;

	if(!strcmp(real_bluez_device->mac, real_pattern))
		return 0;

	if(!strcmp(real_bluez_device->alias, real_pattern))
		return 0;

	return 1;
}

BlueZDevice* bluez_device_manager_get_device(
			BlueZDeviceManager* bluez_device_manager,
			const char* pattern)
{
	BlueZDevice* ret = NULL;

	DEVICE_LIST_LOCK(bluez_device_manager);

	GList* found = NULL;

	found = g_list_find_custom(bluez_device_manager->devices, pattern, bluez_device_compare_pattern_callback);

	if(found){
		ret = (BlueZDevice*) found->data;
		ret = bluez_device_ref(ret);
	}

	DEVICE_LIST_UNLOCK(bluez_device_manager);

	return ret;
}

GList*	bluez_device_manager_get_discovered_devices(BlueZDeviceManager* bluez_device_manager)
{
	GList* ret = NULL;

	if(!bluez_device_manager){
		DBG_ERR("bluez_device_manager can't be null!\n");
		return NULL;
	}

	DEVICE_LIST_LOCK(bluez_device_manager);
	ret = g_list_copy(bluez_device_manager->devices);
	DEVICE_LIST_UNLOCK(bluez_device_manager);

	return ret;
}

BlueZDeviceManager* bluez_device_manager_create(void)
{
	BlueZDeviceManager* ret = NULL;

	ret = g_malloc0(sizeof(BlueZDeviceManager));

	pthread_mutex_init(&(ret->device_list_lock),NULL);

	ret->gatt_db = bluez_gatt_db_create();
	if(!ret->gatt_db)
		goto FAILED;

	ret->connection = bluez_dbus_connection_create();
	if(!ret->connection)
		goto FAILED;

	ret->object_manager_proxy = bluez_dbus_proxy_create(OBJECT_MANAGER_INTERFACE, OBJECT_PATH_ROOT);

	if(!ret->object_manager_proxy)
		goto FAILED;

	ret->worker_context = g_main_context_new();
    if (!ret->worker_context) {
        DBG_ERR("create worker context failed!\n");
        goto FAILED;
    }

    ret->event_loop = g_main_loop_new(ret->worker_context, false);
    if (!ret->event_loop) {
        DBG_ERR("create event loop failed!\n");
        goto FAILED;
    }

	if(!device_manager_init(ret))
		goto FAILED;

	return bluez_device_manager_ref(ret);

FAILED:

	bluez_device_manager_destroy(ret);
	return NULL;
}

static void free_devices(gpointer data, gpointer user_data)
{
	BlueZDevice* device = (BlueZDevice*)data;

	bluez_device_unref(device);

}

void bluez_device_manager_destroy(BlueZDeviceManager* bluez_device_manager)
{
	if(!bluez_device_manager)
		return;

	DBG_IND("destroy device manager\n");

	if(bluez_device_manager->event_loop){
		g_main_loop_quit(bluez_device_manager->event_loop);
		bluez_device_manager->event_loop = NULL;
	}

	pthread_mutex_destroy(&(bluez_device_manager->device_list_lock));

	if(bluez_device_manager->adapter){
		bluez_adapter_unref(bluez_device_manager->adapter);
		bluez_device_manager->adapter = NULL;
	}

	if(bluez_device_manager->object_manager_proxy){
		bluez_dbus_proxy_unref(bluez_device_manager->object_manager_proxy);
		bluez_device_manager->object_manager_proxy = NULL;
	}


	if(bluez_device_manager->adapter_object_path){
		g_free(bluez_device_manager->adapter_object_path);
		bluez_device_manager->adapter_object_path = NULL;
	}

	if(bluez_device_manager->devices){

		g_list_foreach(bluez_device_manager->devices, free_devices, NULL);
		g_list_free(bluez_device_manager->devices);
		bluez_device_manager->devices = NULL;
	}


	g_free(bluez_device_manager);
	bluez_device_manager = NULL;
}
