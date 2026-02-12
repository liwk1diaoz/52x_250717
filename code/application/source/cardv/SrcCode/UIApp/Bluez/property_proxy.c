
#include "property_proxy.h"

static GVariant* tuple_unbox(GVariant* tuple)
{
    GVariant *child = g_variant_get_child_value(tuple, 0);

    child = g_variant_get_variant(child);

    return child;
}

BlueZDBusPropProxy* bluez_dbus_prop_proxy_create(const char* obj_path)
{
	BlueZDBusPropProxy* ret = NULL;

	ret = g_malloc0(sizeof(BlueZDBusPropProxy));

	ret->prop_proxy = bluez_dbus_proxy_create(PROPERTIES_INTERFACE, obj_path);

	if(!ret->prop_proxy)
		goto FAILED;

    return bluez_dbus_prop_proxy_ref(ret);


FAILED:

	bluez_dbus_prop_proxy_destroy(ret);
	return NULL;
}

void bluez_dbus_prop_proxy_destroy(BlueZDBusPropProxy* bluez_dbus_prop_proxy)
{
	if(!bluez_dbus_prop_proxy)
		return;

	if(bluez_dbus_prop_proxy->prop_proxy){
		bluez_dbus_proxy_unref(bluez_dbus_prop_proxy->prop_proxy);
		bluez_dbus_prop_proxy->prop_proxy = NULL;
	}

	g_free(bluez_dbus_prop_proxy);
}

BlueZDBusPropProxy* bluez_dbus_prop_proxy_ref(BlueZDBusPropProxy* bluez_dbus_prop_proxy)
{
	if (!bluez_dbus_prop_proxy)
		return NULL;

	__sync_fetch_and_add(&bluez_dbus_prop_proxy->ref_count, 1);

	return bluez_dbus_prop_proxy;
}

void bluez_dbus_prop_proxy_unref(BlueZDBusPropProxy* bluez_dbus_prop_proxy)
{
	if (!bluez_dbus_prop_proxy)
		return;

	if (__sync_sub_and_fetch(&bluez_dbus_prop_proxy->ref_count, 1))
		return;

	bluez_dbus_prop_proxy_destroy(bluez_dbus_prop_proxy);
}

const char* bluez_dbus_prop_proxy_get_obj_path(BlueZDBusPropProxy* bluez_dbus_prop_proxy)
{
	if(bluez_dbus_prop_proxy){
		DBG_ERR("bluez_dbus_prop_proxy can't be NULL!\n");
		return NULL;
	}

	return bluez_dbus_proxy_get_obj_path(bluez_dbus_prop_proxy->prop_proxy);
}

bool bluez_dbus_prop_proxy_get_array_string(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	bluez_dbus_prop_proxy_get_array_string_callback callback,
	void* user_data
)
{
	GError* error = NULL;

	if(!bluez_dbus_prop_proxy){
		DBG_ERR("bluez_dbus_prop_proxy can't be NULL!\n");
		return false;
	}

	if(!interface_name | !prop_name){
		DBG_ERR("interface_name or prop_name can't be NULL!\n");
		return false;
	}


	GVariant* ret = bluez_dbus_proxy_call_method(
			bluez_dbus_prop_proxy->prop_proxy,
			"Get",
			g_variant_new("(ss)", interface_name, prop_name),
			&error
	);

	if(error){
    	DBG_ERR("get boolean prop filaed, %s\n", error->message);
    	g_error_free(error);
    	return false;
	}

	ret = tuple_unbox(ret);


//	*result = g_variant_get_boolean(ret);

	GVariantIter *iter;
	gchar *str;

	g_variant_get(ret, "as", &iter);

	while (g_variant_iter_loop(iter, "s", &str))
	{
		callback(str, user_data);
	}

	g_variant_iter_free (iter);

	return true;

}

bool bluez_dbus_prop_proxy_get_boolean_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	bool* result
)
{
	GError* error = NULL;

	if(!bluez_dbus_prop_proxy){
		DBG_ERR("bluez_dbus_prop_proxy can't be NULL!\n");
		return false;
	}

	if(!interface_name | !prop_name){
		DBG_ERR("interface_name or prop_name can't be NULL!\n");
		return false;
	}


	GVariant* ret = bluez_dbus_proxy_call_method(
			bluez_dbus_prop_proxy->prop_proxy,
			"Get",
			g_variant_new("(ss)", interface_name, prop_name),
			&error
	);

	if(error){
    	DBG_ERR("get boolean prop filaed, %s\n", error->message);
    	g_error_free(error);
    	return false;
	}

	ret = tuple_unbox(ret);


	*result = g_variant_get_boolean(ret);

	DBG_IND("result = %s\n", *result == true ? "true" : "false");

	return true;

}


bool bluez_dbus_prop_proxy_get_string_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	char** result
)
{
	GError* error = NULL;

	if(!bluez_dbus_prop_proxy){
		DBG_ERR("proxy can't be NULL!\n");
	}

	if(!interface_name | !prop_name){
		DBG_ERR("interface_name or prop_name can't be NULL!\n");
	}

	GVariant* ret = bluez_dbus_proxy_call_method(
			bluez_dbus_prop_proxy->prop_proxy,
			"Get",
			g_variant_new("(ss)", interface_name, prop_name),
			&error
	);

	if(error){
		DBG_ERR("get string prop failed, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	ret = tuple_unbox(ret);

	gsize length = 0;
	const gchar *string = g_variant_get_string(ret, &length);

	*result = g_strdup(string);

	DBG_IND("result = %s\n", *result);

	return true;
}

bool bluez_dbus_prop_proxy_set_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	GVariant*	value
)
{
	GError* error = NULL;

	bluez_dbus_proxy_call_method(
			bluez_dbus_prop_proxy->prop_proxy,
			"Set",
			g_variant_new("(ssv)", interface_name, prop_name, value),
			&error
	);

	if(error){
		DBG_ERR("set prop(%s,%s) failed!, %s\n", interface_name, prop_name, error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_dbus_prop_proxy_set_bool_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	const bool	value
){
	GVariant* boolean = NULL;

	boolean = g_variant_new_boolean(value);

	return bluez_dbus_prop_proxy_set_prop(
			bluez_dbus_prop_proxy,
			interface_name,
			prop_name,
			boolean
	);
}

bool bluez_dbus_prop_proxy_set_string_prop(
	BlueZDBusPropProxy* bluez_dbus_prop_proxy,
	const char* interface_name,
	const char* prop_name,
	const char* value
){

	GVariant* string = NULL;

	string = g_variant_new_string(value);

	return bluez_dbus_prop_proxy_set_prop(
			bluez_dbus_prop_proxy,
			interface_name,
			prop_name,
			string
	);
}

