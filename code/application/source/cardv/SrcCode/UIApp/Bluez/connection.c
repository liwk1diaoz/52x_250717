

#include "connection.h"

typedef struct {

	guint subId;

} SubscribeNode;


BlueZDBusConnection* bluez_dbus_connection_create(void)
{

	BlueZDBusConnection* ret = g_malloc0(sizeof(BlueZDBusConnection));
	GError* error = NULL;
	GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

	if (error) {
		DBG_ERR("connection create failed, %s\n", error->message);
		g_error_free(error);
		g_free(ret);
		return NULL;
	}

	g_dbus_connection_set_exit_on_close(conn, false);

	ret->conn = conn;

	return bluez_dbus_connection_ref(ret);
}

void bluez_dbus_connection_close(BlueZDBusConnection* dbus_conn)
{

    g_dbus_connection_flush_sync(dbus_conn->conn, NULL, NULL);
    g_dbus_connection_close_sync(dbus_conn->conn, NULL, NULL);
    g_object_unref(dbus_conn->conn);
    dbus_conn->conn = NULL;

}

unsigned int bluez_dbus_connection_subscribe_signal(
					BlueZDBusConnection* connection,
					const char* service_name,
					const char* interface_name,
					const char* member,
					const char* first_arg_filter,
					GDBusSignalCallback callback,
					gpointer user_data)
{
	if(!service_name){
		DBG_ERR("service_name can't be null\n");
	}

	if(!interface_name){
		DBG_ERR("interface_name can't be null\n");
	}

	if(!member){
		DBG_ERR("member can't be null\n");
	}

    guint sub_id = g_dbus_connection_signal_subscribe(
					connection->conn,
					service_name,
					interface_name,
					member,
					NULL,
					first_arg_filter,
					G_DBUS_SIGNAL_FLAGS_NONE,
					callback,
					user_data,
					NULL);

    if(!sub_id){
    	DBG_ERR("subscribe signal failed!\n");
    	return 0;
    }

    DBG_DUMP("sub_id = %u\n", sub_id);

    SubscribeNode* node = g_new0(SubscribeNode, 1);
    connection->sub_id_list = g_list_append(connection->sub_id_list, node);
    return sub_id;

}

void bluez_dbus_connection_destroy(BlueZDBusConnection* conn)
{




}

BlueZDBusConnection* bluez_dbus_connection_ref(BlueZDBusConnection* conn)
{
	if (!conn)
		return NULL;

	__sync_fetch_and_add(&conn->ref_count, 1);

	return conn;
}

void bluez_dbus_connection_unref(BlueZDBusConnection* conn)
{
	if (!conn)
		return;

	if (__sync_sub_and_fetch(&conn->ref_count, 1))
		return;

	bluez_dbus_connection_destroy(conn);
}

