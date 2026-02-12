
#include "device.h"
#include "device_manager.h"
#include "proxy.h"
#include "property_proxy.h"

static bool bluez_device_init(BlueZDevice* device)
{

	if(!bluez_dbus_prop_proxy_get_string_prop(
			device->device_prop_proxy,
			BLUEZ_DEVICE_INTERFACE,
			BLUEZ_DEVICE_INTERFACE_PROP_ALIAS,
			&(device->alias)
	)){
		return false;
	}

	return true;

}

BlueZDevice* bluez_device_create(
		const char* mac,
		const char* object_path,
		struct _BlueZDeviceManager* device_manager
)
{
	BlueZDevice* ret = NULL;

	ret = g_malloc0(sizeof(BlueZDevice));

	ret->device_proxy = bluez_dbus_proxy_create(BLUEZ_DEVICE_INTERFACE, object_path);
	if(!ret->device_proxy)
		goto FAILED;

	ret->device_prop_proxy = bluez_dbus_prop_proxy_create(object_path);
	if(!ret->device_prop_proxy)
		goto FAILED;

	ret->object_path = g_strdup(object_path);
	ret->mac = g_strdup(mac);

	if(!bluez_device_init(ret))
		goto FAILED;


	return bluez_device_ref(ret);

FAILED:

	bluez_device_destroy(ret);

	return NULL;

}


void bluez_device_destroy(BlueZDevice* bluez_device)
{
	if(!bluez_device)
		return;
}

BlueZDevice* bluez_device_ref(BlueZDevice* device)
{
	if (!device)
		return NULL;

	__sync_fetch_and_add(&device->ref_count, 1);

	return device;
}

void bluez_device_unref(BlueZDevice* device)
{
	if (!device)
		return;

	if (__sync_sub_and_fetch(&device->ref_count, 1))
		return;

	bluez_device_destroy(device);
}

const char* bluez_device_get_mac(BlueZDevice* device)
{
	if(device && device->mac)
		return device->mac;
	else
		return NULL;
}

const char* bluez_device_get_object_path(BlueZDevice* device)
{
	if(device && device->object_path)
		return device->object_path;
	else
		return NULL;
}

const char* bluez_device_get_alias(BlueZDevice* device)
{
	if(device && device->alias)
		return device->alias;
	else
		return NULL;
}

bool bluez_device_is_connected(BlueZDevice* device)
{
	if(device)
		return device->connected;
	else
		return false;
}

bool bluez_device_is_paired(BlueZDevice* device)
{
	if(device)
		return device->paired;
	else
		return false;
}

bool bluez_device_is_trusted(BlueZDevice* device)
{
	if(device)
		return device->trusted;
	else
		return false;
}

bool bluez_device_connect(BlueZDevice* bluez_device)
{
	GError* error = NULL;

	if(!bluez_device){
		DBG_ERR("bluez_device can't be null!\n");
		return false;
	}

	if(bluez_device_is_connected(bluez_device)){
		DBG_WRN("%s(%s) already connected\n", bluez_device->alias, bluez_device->mac);
		return true;
	}

	DBG_DUMP("connecting %s(%s) ...\n", bluez_device->alias, bluez_device->mac);

	bluez_dbus_proxy_call_method(
			bluez_device->device_proxy,
			BLUEZ_DEVICE_INTERFACE_METHOD_CONNECT,
			NULL,
			&error);

	if(error){
		DBG_ERR("connect failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_device_disconnect(BlueZDevice* bluez_device)
{
	GError* error;

	if(!bluez_device){
		DBG_ERR("bluez_device can't be null!\n");
		return false;
	}

	if(!bluez_device_is_connected(bluez_device)){
		DBG_WRN("%s(%s) already disconnected\n", bluez_device->alias, bluez_device->mac);
		return true;
	}

	DBG_DUMP("disconnecting %s(%s) ...\n", bluez_device->alias, bluez_device->mac);

	bluez_dbus_proxy_call_method(
			bluez_device->device_proxy,
			BLUEZ_DEVICE_INTERFACE_METHOD_DISCONNECT,
			NULL,
			&error);

	if(error){
		DBG_ERR("disconnect failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_device_pair(BlueZDevice* bluez_device)
{
	GError* error = NULL;

	if(!bluez_device){
		DBG_ERR("bluez_device can't be null!\n");
		return false;
	}

	if(bluez_device_is_paired(bluez_device)){
		DBG_WRN("%s(%s) already paired\n", bluez_device->alias, bluez_device->mac);
		return true;
	}

	bluez_dbus_proxy_call_method(
			bluez_device->device_proxy,
			BLUEZ_DEVICE_INTERFACE_METHOD_PAIR,
			NULL,
			&error);

	if(error){
		DBG_ERR("pairing failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_device_unpair(BlueZDevice* bluez_device)
{
	GError* error = NULL;

	if(!bluez_device){
		DBG_ERR("bluez_device can't be null!\n");
		return false;
	}

	if(!bluez_device_is_paired(bluez_device)){
		DBG_WRN("%s(%s) already unpaired\n", bluez_device->alias, bluez_device->mac);
		return true;
	}

	bluez_dbus_proxy_call_method(
			bluez_device->device_proxy,
			BLUEZ_DEVICE_INTERFACE_METHOD_UNPAIR,
			NULL,
			&error);

	if(error){
		DBG_ERR("pairing failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_device_set_trusted(BlueZDevice* bluez_device, const bool trusted)
{
	if(!bluez_device){
		DBG_ERR("bluez_device can't be null!\n");
		return false;
	}

	return bluez_dbus_prop_proxy_set_bool_prop(
			bluez_device->device_prop_proxy,
			BLUEZ_DEVICE_INTERFACE,
			BLUEZ_DEVICE_INTERFACE_PROP_TRUSTED,
			trusted);
}

void bluez_device_register_user_callback(BlueZDevice* bluez_device, bluez_device_user_callback cb, void* user_data)
{
	if(!bluez_device)
		return;

	bluez_device->user_callback = cb;
	bluez_device->user_data = user_data;
}

void bluez_device_on_prop_changed(BlueZDevice* bluez_device, GVariant* map)
{
	gboolean connection_changed, pairing_changed, alias_changed, trust_changed;
	gboolean bool_value = false;
	char*	 string_value = NULL;

	if((connection_changed = g_variant_lookup(map, BLUEZ_DEVICE_INTERFACE_PROP_CONNECTED, "b", &bool_value)))
	{
		bluez_device->connected = bool_value;
		DBG_DUMP("%s %s\n", bluez_device->object_path, bool_value ? "connected" : "disconnected");
	}

	if((pairing_changed = g_variant_lookup(map, BLUEZ_DEVICE_INTERFACE_PROP_PAIRED, "b", &bool_value)) ){
		bluez_device->paired = bool_value;
		DBG_DUMP("%s %s\n", bluez_device->object_path, bool_value ? "paired" : "unpaired");
	}

	if((trust_changed = g_variant_lookup(map, BLUEZ_DEVICE_INTERFACE_PROP_TRUSTED, "b", &bool_value)) ){
		bluez_device->trusted = bool_value;
		DBG_DUMP("%s %s\n", bluez_device->object_path, bool_value ? "trusted" : "untrusted");
	}

	if((alias_changed = g_variant_lookup(map, BLUEZ_DEVICE_INTERFACE_PROP_ALIAS, "&s", &string_value))){

		if(bluez_device->alias)
			g_free(bluez_device->alias);

		bluez_device->alias = g_strdup(string_value);
	}


	if(connection_changed && bluez_device->user_callback){

		bluez_device->user_callback(
				bluez_device,
				bluez_device->connected ? BLUEZ_DEVICE_EVENT_CONNECTED : BLUEZ_DEVICE_EVENT_DISCONNECTED,
				bluez_device->user_data);
	}

	if(pairing_changed && bluez_device->user_callback){

		bluez_device->user_callback(
				bluez_device,
				bluez_device->paired ? BLUEZ_DEVICE_EVENT_PAIRED : BLUEZ_DEVICE_EVENT_UNPAIRED,
				bluez_device->user_data);
	}

	if(trust_changed && bluez_device->user_callback){

		bluez_device->user_callback(
				bluez_device,
				bluez_device->paired ? BLUEZ_DEVICE_EVENT_TRUSTED : BLUEZ_DEVICE_EVENT_UNTRUSTED,
				bluez_device->user_data);
	}

	if(alias_changed && bluez_device->user_callback){

		bluez_device->user_callback(
				bluez_device,
				BLUEZ_DEVICE_EVENT_ALIAS_CHANGED,
				bluez_device->user_data);
	}
}

