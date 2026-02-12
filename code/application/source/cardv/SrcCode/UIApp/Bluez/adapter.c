

#include "adapter.h"
#include "property_proxy.h"

static bool invalidate_adapter(BlueZAdapter* adapter)
{
	if(!adapter || !adapter->adapter_proxy || !adapter->adapter_prop_proxy){

		DBG_ERR("invalid adapter!\n");
		return false;
	}

	return true;
}

static bool adapter_init(BlueZAdapter* adapter)
{
	if(!bluez_dbus_prop_proxy_get_string_prop(
			adapter->adapter_prop_proxy,
			BLUEZ_ADAPTER_INTERFACE,
			BLUEZ_ADAPTER_INTERFACE_PROP_ADDRESS,
			&(adapter->mac)))
	{
		DBG_ERR("get mac address failed\n");
		return false;
	}


	DBG_DUMP("MAC: %s\n", adapter->mac);


	if(!bluez_dbus_prop_proxy_get_string_prop(
			adapter->adapter_prop_proxy,
			BLUEZ_ADAPTER_INTERFACE,
			BLUEZ_ADAPTER_INTERFACE_PROP_ALIAS,
			&(adapter->alias)))
	{
		DBG_ERR("get alias failed\n");
		return false;
	}

	DBG_DUMP("Alias: %s\n", adapter->alias);

	if(!bluez_dbus_prop_proxy_get_string_prop(
			adapter->adapter_prop_proxy,
			BLUEZ_ADAPTER_INTERFACE,
			BLUEZ_ADAPTER_INTERFACE_PROP_ALIAS,
			&(adapter->name)))
	{
		DBG_ERR("get name failed\n");
		return false;
	}

	DBG_DUMP("Name: %s\n", adapter->name);

	return true;
}

BlueZAdapter* bluez_adapter_ref(BlueZAdapter* adapter)
{
	if(!adapter)
		return NULL;

	__sync_fetch_and_add(&adapter->ref_count, 1);

	return adapter;
}

void bluez_adapter_unref(BlueZAdapter* adapter)
{
	if (!adapter)
		return;

	if (__sync_sub_and_fetch(&adapter->ref_count, 1))
		return;

	bluez_adapter_destroy(adapter);

}

BlueZAdapter* bluez_adapter_create(char* adapter_obj_path)
{
	BlueZAdapter* ret = NULL;

	ret = g_malloc0(sizeof(BlueZAdapter));

	ret->adapter_proxy = bluez_dbus_proxy_create(BLUEZ_ADAPTER_INTERFACE, adapter_obj_path);
	if(!ret->adapter_proxy)
		goto FAILED;

	ret->adapter_prop_proxy = bluez_dbus_prop_proxy_create(adapter_obj_path);
	if(!ret->adapter_prop_proxy)
		goto FAILED;

	if(!adapter_init(ret))
		goto FAILED;

	return bluez_adapter_ref(ret);

FAILED:

	bluez_adapter_destroy(ret);
	return NULL;
}

void bluez_adapter_destroy(BlueZAdapter* adapter)
{
	if(!adapter)
		return;

	DBG_DUMP("destroy adapter(%s %s)\n", adapter->alias, adapter->mac);

	if(adapter->adapter_prop_proxy){
		bluez_dbus_prop_proxy_destroy(adapter->adapter_prop_proxy);
		adapter->adapter_prop_proxy = NULL;
	}

	if(adapter->adapter_proxy){
		bluez_dbus_proxy_destroy(adapter->adapter_proxy);
		adapter->adapter_proxy = NULL;
	}

	if(adapter->alias){
		g_free(adapter->alias);
		adapter->alias = NULL;
	}

	if(adapter->name){
		g_free(adapter->name);
		adapter->name = NULL;
	}

	if(adapter->mac){
		g_free(adapter->mac);
		adapter->mac = NULL;
	}

	g_free(adapter);
	adapter = NULL;
}

bool bluez_adapter_set_powered(BlueZAdapter* adapter, const bool powered)
{
	if(!invalidate_adapter(adapter))
		return false;

	return bluez_dbus_prop_proxy_set_bool_prop(
			adapter->adapter_prop_proxy,
			BLUEZ_ADAPTER_INTERFACE,
			BLUEZ_ADAPTER_INTERFACE_PROP_POWERED,
			powered);

}

bool bluez_adapter_set_discoverable(BlueZAdapter* adapter, const bool discoverable)
{
	if(!invalidate_adapter(adapter))
		return false;

	return bluez_dbus_prop_proxy_set_bool_prop(
			adapter->adapter_prop_proxy,
			BLUEZ_ADAPTER_INTERFACE,
			BLUEZ_ADAPTER_INTERFACE_PROP_DISCOVERABLE,
			discoverable);
}

bool bluez_adapter_start_scan(BlueZAdapter* adapter)
{
	GError* error = NULL;

	if(!invalidate_adapter(adapter))
		return false;



	bluez_dbus_proxy_call_method(
		adapter->adapter_proxy,
		BLUEZ_ADAPTER_INTERFACE_METHOD_START_SCAN,
		NULL,
		&error);


	if(error){
		DBG_ERR("start scan failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_adapter_stop_scan(BlueZAdapter* adapter)
{
	GError* error = NULL;

	if(!invalidate_adapter(adapter))
		return false;

	bluez_dbus_proxy_call_method(
		adapter->adapter_proxy,
		BLUEZ_ADAPTER_INTERFACE_METHOD_STOP_SCAN,
		NULL,
		&error);


	if(error){
		DBG_ERR("stop scan failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_adapter_set_alias(BlueZAdapter* adapter, const char* alias)
{
	if(!invalidate_adapter(adapter))
		return false;

	return bluez_dbus_prop_proxy_set_string_prop(
			adapter->adapter_prop_proxy,
			BLUEZ_ADAPTER_INTERFACE,
			BLUEZ_ADAPTER_INTERFACE_PROP_ALIAS,
			alias);
}

const char* bluez_adapter_get_alias(BlueZAdapter* adapter)
{
	if(!invalidate_adapter(adapter))
		return false;

	return adapter->alias;
}

const char* bluez_adapter_get_name(BlueZAdapter* adapter)
{
	if(!invalidate_adapter(adapter))
		return false;

	return adapter->name;
}

const char* bluez_adapter_get_mac(BlueZAdapter* adapter)
{
	if(!invalidate_adapter(adapter))
		return false;

	return adapter->mac;
}

