

#include "gatt.h"
#include "property_proxy.h"
#include "device_manager.h"
#include "connection.h"

static BlueZGattChar* find_char(BlueZGattDB* gatt_db, const char* char_object_path);

BlueZGattDB* bluez_gatt_db_ref(BlueZGattDB* bluez_gatt_db)
{
	if(!bluez_gatt_db)
		return NULL;

	__sync_fetch_and_add(&bluez_gatt_db->ref_count, 1);

	return bluez_gatt_db;
}


void bluez_gatt_db_unref(BlueZGattDB* bluez_gatt_db)
{
	if(!bluez_gatt_db)
		return;

	if(__sync_sub_and_fetch(&bluez_gatt_db->ref_count, 1))
		return;


	bluez_gatt_db_destroy(bluez_gatt_db);
}

BlueZGattDB* bluez_gatt_db_create(void)
{
	BlueZGattDB* ret = g_malloc0(sizeof(BlueZGattDB));

	return ret;
}

void bluez_gatt_db_destroy(BlueZGattDB* bluez_gatt_db)
{

}

static gint gatt_char_compare_pattern_callback(gconstpointer  bluez_gatt_char, gconstpointer  pattern)
{
	BlueZGattChar* real_bluez_gatt_char = (BlueZGattChar*) bluez_gatt_char;
	char* real_pattern = (char*) pattern;

	if(!strcmp(real_bluez_gatt_char->object_path, real_pattern))
		return 0;

	if(!strcmp(real_bluez_gatt_char->uuid, real_pattern))
		return 0;

	return 1;
}

bool bluez_gatt_db_add_char(BlueZGattDB* gatt_db, BlueZGattChar* bluez_gatt_char)
{
	if(!bluez_gatt_char){
		DBG_ERR("bluez_gatt_char can't be null!\n");
		return false;
	}

	gatt_db->char_list = g_list_append(gatt_db->char_list, bluez_gatt_char);

	return true;
}

bool bluez_gatt_db_remove_char(BlueZGattDB* gatt_db, char* pattern)
{
	if(!pattern){
		DBG_ERR("object_path can't be null!\n");
		return false;
	}

	GList *find = g_list_find_custom(gatt_db->char_list, pattern, gatt_char_compare_pattern_callback);

	if(!find){
		DBG_ERR("%s not found\n", pattern);
		return false;
	}

	gatt_db->char_list = g_list_remove(gatt_db->char_list, find->data);
	return true;
}

void gatt_char_get_flags_callback(char* str, void* bluez_gatt_char)
{
	BlueZGattChar* real_bluez_gatt_char = (BlueZGattChar*) bluez_gatt_char;


	if(!strcmp(str, "read"))
		real_bluez_gatt_char->flags.read = 1;
	else if(!strcmp(str, "write"))
		real_bluez_gatt_char->flags.write = 1;
	else if(!strcmp(str, "notify"))
		real_bluez_gatt_char->flags.notify = 1;
	else if(!strcmp(str, "indicate"))
		real_bluez_gatt_char->flags.indicate = 1;
	else{
		/* currently ignnored */
	}
}

BlueZGattChar* bluez_gatt_char_create(const char* object_path)
{
	BlueZGattChar* ret = g_malloc0(sizeof(BlueZGattChar));

	BlueZDBusPropProxy* prop_proxy = bluez_dbus_prop_proxy_create(object_path);

	ret->prop_proxy = prop_proxy;

	BlueZDBusProxy* proxy = bluez_dbus_proxy_create(GATT_CHAR_INTERFACE, object_path);

	ret->proxy = proxy;
	ret->object_path = g_strdup(object_path);

	bluez_dbus_prop_proxy_get_string_prop(
			ret->prop_proxy,
			GATT_CHAR_INTERFACE,
			"UUID",
			&ret->uuid
	);

	bluez_dbus_prop_proxy_get_string_prop(
			ret->prop_proxy,
			GATT_CHAR_INTERFACE,
			"Service",
			&ret->service
	);


	bluez_dbus_prop_proxy_get_array_string(
			ret->prop_proxy,
			GATT_CHAR_INTERFACE,
			"Flags",
			gatt_char_get_flags_callback,
			ret
	);

	return bluez_gatt_char_ref(ret);
}

void bluez_gatt_char_destroy(BlueZGattChar* bluez_gatt_char)
{

}

BlueZGattChar* bluez_gatt_char_ref(BlueZGattChar* bluez_gatt_char)
{
	if(!bluez_gatt_char)
		return NULL;

	__sync_fetch_and_add(&bluez_gatt_char->ref_count, 1);

	return bluez_gatt_char;
}

void bluez_gatt_char_unref(BlueZGattChar* bluez_gatt_char)
{
	if(!bluez_gatt_char)
		return;

	if(__sync_sub_and_fetch(&bluez_gatt_char->ref_count, 1))
		return;


	bluez_gatt_char_destroy(bluez_gatt_char);
}

bool bluez_gatt_char_start_notify(BlueZGattChar* bluez_gatt_char)
{
	GError* error = NULL;

	if(!bluez_gatt_char){
		DBG_ERR("gatt_char can't be null\n");
		return false;
	}

	bluez_dbus_proxy_call_method(
			bluez_gatt_char->proxy,
			GATT_CHAR_INTERFACE_METHOD_START_NOTIFY,
			NULL,
			&error
	);

	if(error){
		DBG_ERR("start notify failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_gatt_char_stop_notify(BlueZGattChar* bluez_gatt_char)
{
	GError* error = NULL;

	if(!bluez_gatt_char){
		DBG_ERR("gatt_char can't be null\n");
		return false;
	}

	bluez_dbus_proxy_call_method(
			bluez_gatt_char->proxy,
			GATT_CHAR_INTERFACE_METHOD_STOP_NOTIFY,
			NULL,
			&error
	);

	if(error){
		DBG_ERR("start notify failed!, %s\n", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool bluez_gatt_char_read_value(BlueZGattChar* bluez_gatt_char, uint32_t offset)
{

	return true;
}

static BlueZGattChar* find_char(BlueZGattDB* gatt_db, const char* pattern)
{
	if(!pattern){
		DBG_ERR("pattern can't be null!\n");
		return NULL;
	}

	GList *find = g_list_find_custom(gatt_db->char_list, pattern, gatt_char_compare_pattern_callback);

	if(!find){
		DBG_ERR("%s not found\n", pattern);
		return NULL;
	}

	return (BlueZGattChar*) find->data;
}

BlueZGattChar* bluez_gatt_db_find_char(BlueZGattDB* gatt_db, const char* pattern)
{
	BlueZGattChar* target = find_char(gatt_db, pattern);

	return bluez_gatt_char_ref(target);
}

bool bluez_gatt_db_char_start_notify(BlueZGattDB* gatt_db, char* pattern)
{
	BlueZGattChar* target = find_char(gatt_db, pattern);

	if(target)
		return bluez_gatt_char_start_notify(target);
	else
		return false;
}

bool bluez_gatt_db_char_stop_notify(BlueZGattDB* gatt_db, char* pattern)
{
	BlueZGattChar* target = find_char(gatt_db, pattern);

	if(target)
		return bluez_gatt_char_stop_notify(target);
	else
		return false;
}

bool bluez_gatt_db_char_read_value(BlueZGattDB* gatt_db, char* pattern, uint32_t offset)
{
	GError* error = NULL;
	BlueZGattChar* target = find_char(gatt_db, pattern);

	GVariant* ret =	bluez_dbus_proxy_call_method(
						target->proxy,
						GATT_CHAR_INTERFACE_METHOD_READ_VALUE,
						NULL,
						&error
					);

	DBG_DUMP("READ_VALUE type string = %s\n", g_variant_get_type_string(ret));

	return true;
}

static BlueZGattCharValue char_read_value(GVariant *value)
{
	GVariantIter iter;
	guchar byte;
	uint16_t len = 0;
	BlueZGattCharValue ret = {0};

	memset(&ret, 0, sizeof(ret));

	g_variant_iter_init (&iter, value);
    while (g_variant_iter_loop (&iter, "y", &byte))
    {
    	ret.bytes[len] = byte;
    	ret.len++;
    }

    if(ret.len && ret.bytes[ret.len - 1] == '\0')
    	ret.is_string = true;

    return ret;
}

void bluez_gatt_db_char_on_prop_changed(BlueZGattDB* gatt_db, const char* char_object_path, GVariant* map)
{
    GVariantIter iter;
    GVariant *value;
    gchar *key;
    BlueZGattChar* target = find_char(gatt_db, char_object_path);

    g_variant_iter_init (&iter, map);
    while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
    {
//        g_print ("Item '%s' has type '%s'\n", key,
//                 g_variant_get_type_string (value));

        if(!strcmp(key, "Notifying")){
        	target->notifying = g_variant_get_boolean(value);

//        	DBG_DUMP("target->notifying = %d\n", target->notifying);
        }
        else if(!strcmp(key, "Value")){
        	BlueZGattCharValue v = char_read_value(value);
        	BlueZGattChar* target = find_char(gatt_db, char_object_path);

        	if(target && target->value_changed_callback){
        		void* data = target->value_changed_user_data;
        		target->value_changed_callback(target, &v, data);
        	}
        }
    }
}

bool bluez_gatt_char_register_value_changed_callback(BlueZGattChar* bluez_gatt_char, bluez_gatt_char_value_changed_callback cb, void* user_data)
{
	if(!bluez_gatt_char || !cb){
		DBG_ERR("bluez_gatt_char and cb can't be null!\n");
		return false;
	}

	bluez_gatt_char->value_changed_callback = cb;
	bluez_gatt_char->value_changed_user_data = user_data;

	return true;
}


/****************************************************************/
#define APP_PATH "/org/bluez/app"

static GList *local_services;

static const char INTROSPECT_XML[] = R"(
<!DOCTYPE node PUBLIC
    "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd" >
<node xmlns:doc="http://www.freedesktop.org/dbus/1.0/doc.dtd">
<interface name='org.bluez.GattService1'>
	<property name='UUID' type="s" access='read'/>
	<property name='Primary' type='b' access='read'/>
	<property name='Handle' type='q' access='readwrite'/>
	<property name='Includes' type='ao' access='readwrite'/>
</interface>
</node>)"
;

static const char INTROSPECT_XML2[] = R"(
<!DOCTYPE node PUBLIC
    "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd" >
<node xmlns:doc="http://www.freedesktop.org/dbus/1.0/doc.dtd">
<interface name='org.bluez.GattProfile1'>
	<method name='Release'/>
	<property name='UUIDs' type='as' access='read'/>
</interface>
</node>)"
;

//static const char INTROSPECT_XML3[] = R"(
//<!DOCTYPE node PUBLIC
//    "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
//    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd" >
//<node xmlns:doc="http://www.freedesktop.org/dbus/1.0/doc.dtd">
//<interface name='org.freedesktop.DBus.ObjectManager'>
//	<method name='GetManagedObjects'>
//		<arg name="objects" type="a{oa{sa{sv}}}" direction="out" />
//	<method/>
//	<signal name="InterfacesAdded">
//		<arg name="object" type="o" />
//		<arg name="interfaces" type="a{sa{sv}}" />
//	</method>
//	<signal name="InterfacesRemoved">
//		<arg name="object" type="o" />
//		<arg name="interfaces" type="as" />
//	</method>
//</interface>
//</node>)"
//;

//static GVariant *get_objects (	GDBusConnection       *connection,
//					const gchar           *sender,
//					const gchar           *object_path,
//					const gchar           *interface_name,
//					const gchar           *property_name,
//					GError               **error,
//					gpointer               user_data)
//{
//
//}

static GVariant *getter (	GDBusConnection       *connection,
					const gchar           *sender,
					const gchar           *object_path,
					const gchar           *interface_name,
					const gchar           *property_name,
					GError               **error,
					gpointer               user_data)
{
	GVariant *ret;
	BlueZGattLocalService* service = (BlueZGattLocalService*) user_data;

	DBG_DUMP("getter called prop name = %s, sender = %s , obj path = %s\n", property_name, sender, object_path);

	if (g_strcmp0 (property_name, "UUID") == 0)
	{
		ret = g_variant_new_string (service->uuid);
	}
	else if (g_strcmp0 (property_name, "Primary") == 0)
	{
		ret = g_variant_new_boolean (service->primary);
	}
	else if (g_strcmp0 (property_name, "Handle") == 0)
	{
		ret = g_variant_new_boolean (service->handle);
	}
	else{
		g_error_new(0, -1, "not support");
		return NULL;
	}

	return ret;
}

static gboolean
setter (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{

	BlueZGattLocalService* service = (BlueZGattLocalService*) user_data;

	if (g_strcmp0 (property_name, "Handle") == 0)
	{
		service->handle = g_variant_get_uint16(value);
	}
	else{
		return false;
	}

	return true;
}

void method (	GDBusConnection       *connection,
				const gchar           *sender,
				const gchar           *object_path,
				const gchar           *interface_name,
				const gchar           *method_name,
				GVariant              *parameters,
				GDBusMethodInvocation *invocation,
				gpointer               user_data)
{

}

GDBusInterfaceVTable vtable =
{
		NULL,
		getter,
		setter
};

GDBusInterfaceVTable vtable2 =
{
		method,
		NULL,
		NULL
};




BlueZGattLocalService* bluez_gatt_local_service_register(
		struct _BlueZDeviceManager *manager,
		char* uuid,
		uint16_t handle)
{
	BlueZDBusConnection *conn = manager->connection;
	BlueZGattLocalService* service = g_malloc0(sizeof(BlueZGattLocalService));
	service->uuid = g_strdup(uuid);
	service->handle = handle;
	service->path = g_strdup_printf("%s/service%u", APP_PATH,
					g_list_length(local_services));
	service->primary = true;
	service->proxy = bluez_dbus_proxy_create("org.bluez.GattManager1", manager->adapter_object_path);

	DBG_DUMP("manager->adapter_object_path = %s\n", manager->adapter_object_path);

	GError* error = NULL;
	GVariantBuilder *b;
	GVariant *dict;
	b = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
	dict = g_variant_builder_end (b);


	{
		GError* error = NULL;
		GDBusNodeInfo* data = g_dbus_node_info_new_for_xml(INTROSPECT_XML2, &error);
		if (error) {
			DBG_ERR("g_dbus_node_info_new_for_xml failed!, %s\n", error->message);
			g_error_free(error);
			return NULL;
		}

		GDBusInterfaceInfo* interfaceInfo = data->interfaces[0];

		g_dbus_connection_register_object(
	    		conn->conn,
				APP_PATH,
				interfaceInfo,
				&vtable2,
				NULL,
				NULL,
				&error
	    );

	    if (error) {
	    	DBG_ERR("g_dbus_connection_register_object failed!, %s\n", error->message);
	    	g_error_free(error);
	        return NULL;
	    }

		DBG_DUMP("register %s ok , name = %s prop[0] = %s\n",
				APP_PATH,
				interfaceInfo->name,
				interfaceInfo->properties[0]->name
		);
	}


	{
		GError* error = NULL;
		GDBusNodeInfo* data = g_dbus_node_info_new_for_xml(INTROSPECT_XML, &error);
		if (error) {
			DBG_ERR("g_dbus_node_info_new_for_xml failed!, %s\n", error->message);
			g_error_free(error);
			return NULL;
		}

		GDBusInterfaceInfo* interfaceInfo = data->interfaces[0];

		DBG_DUMP(
			"%s %s\n",
			interfaceInfo->name,
			interfaceInfo->properties[0]->name
		);

		g_dbus_connection_register_object(
				conn->conn,
				service->path,
				interfaceInfo,
				&vtable,
				service,
				NULL,
				&error
		);

		if (error) {
			DBG_ERR("g_dbus_connection_register_object failed!, %s\n", error->message);
			g_error_free(error);
			return NULL;
		}

		DBG_DUMP("register %s ok\n", service->path);
	}

	DBG_DUMP("call RegisterApplication\n");

	bluez_dbus_proxy_call_method(
			service->proxy,
			"RegisterApplication",
			g_variant_new("(o@a{sv})", "/", dict),
			&error
	);

	return NULL;
}




