
#include "proxy.h"


BlueZDBusProxy* bluez_dbus_proxy_create(const char* interfaceName, const char* obj_path)
{
	GError* error = NULL;
	BlueZDBusProxy* ret = NULL;

	ret = g_malloc0(sizeof(BlueZDBusProxy));

	ret->proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
		BLUEZ_SERVICE_NAME,
		obj_path,
		interfaceName,
        NULL,
        &error);

    if(error){
    	DBG_DUMP("proxy create failed, %s\n", error->message);
    	goto FAILED;
    }

    ret->obj_path = g_strdup(obj_path);

    return ret;

FAILED:

	bluez_dbus_proxy_destroy(ret);
	return NULL;
}


void bluez_dbus_proxy_destroy(BlueZDBusProxy* dbus_proxy)
{
	if(!dbus_proxy)
		return;

	if(dbus_proxy->proxy){
		g_object_unref(dbus_proxy->proxy);
		dbus_proxy->proxy = NULL;
	}

	if(dbus_proxy->obj_path){
		g_free(dbus_proxy->obj_path);
		dbus_proxy->obj_path = NULL;
	}

	g_free(dbus_proxy);
	dbus_proxy = NULL;

}

BlueZDBusProxy* bluez_dbus_proxy_ref(BlueZDBusProxy* bluez_dbus_proxy)
{
	if (!bluez_dbus_proxy)
		return NULL;

	__sync_fetch_and_add(&bluez_dbus_proxy->ref_count, 1);

	return bluez_dbus_proxy;
}

void bluez_dbus_proxy_unref(BlueZDBusProxy* bluez_dbus_proxy)
{
	if (!bluez_dbus_proxy)
		return;

	if (__sync_sub_and_fetch(&bluez_dbus_proxy->ref_count, 1))
		return;

	bluez_dbus_proxy_destroy(bluez_dbus_proxy);
}


GVariant* bluez_dbus_proxy_call_method(
		BlueZDBusProxy* dbus_proxy,
		const char* method,
		GVariant* parameters,
		GError** error
)
{
    GVariant* ret = g_dbus_proxy_call_sync(
    		dbus_proxy->proxy,
			method,
			parameters,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			error);



    return ret;
}

const char* bluez_dbus_proxy_get_obj_path(BlueZDBusProxy* dbus_proxy)
{
	if(dbus_proxy){
		DBG_ERR("dbus_proxy can't be null!\n");
		return NULL;
	}

	return dbus_proxy->obj_path;
}

