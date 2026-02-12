
#ifndef CARDV_UIAPP_BLUEZ_CONNECTION_H_
#define CARDV_UIAPP_BLUEZ_CONNECTION_H_


#include "comm.h"

typedef struct _BlueZDBusConnection{

	GDBusConnection* conn;
	GList *sub_id_list;
	int ref_count;

} BlueZDBusConnection;


BlueZDBusConnection* 	bluez_dbus_connection_create(void);

void					bluez_dbus_connection_destroy(BlueZDBusConnection* conn);

BlueZDBusConnection* 	bluez_dbus_connection_ref(BlueZDBusConnection* conn);

void		 			bluez_dbus_connection_unref(BlueZDBusConnection* conn);

unsigned int			bluez_dbus_connection_subscribe_signal(
								BlueZDBusConnection* connection,
								const char* service_name,
								const char* interface_name,
								const char* member,
								const char* first_arg_filter,
								GDBusSignalCallback callback,
								gpointer user_data
						);


#endif
