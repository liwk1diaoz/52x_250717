

#ifndef CARDV_UIAPP_BLUEZ_COMM_H_
#define CARDV_UIAPP_BLUEZ_COMM_H_

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <dbus/dbus.h>
#include <glib.h>
#include <gio.h>
#include <pthread.h>
#include <kwrap/debug.h>
#include <kwrap/task.h>


#define BLUEZ_SERVICE_NAME									"org.bluez"

/* BLUEZ_DEVICE_INTERFACE */
#define BLUEZ_DEVICE_INTERFACE  							"org.bluez.Device1"
#define BLUEZ_DEVICE_INTERFACE_PROP_ALIAS  					"Alias"
#define BLUEZ_DEVICE_INTERFACE_PROP_ADDRESS  				"Address"
#define BLUEZ_DEVICE_INTERFACE_PROP_TRUSTED  				"Trusted"
#define BLUEZ_DEVICE_INTERFACE_PROP_CONNECTED 				"Connected"
#define BLUEZ_DEVICE_INTERFACE_PROP_PAIRED 					 "Paired"


#define BLUEZ_DEVICE_INTERFACE_METHOD_CONNECT  				"Connect"
#define BLUEZ_DEVICE_INTERFACE_METHOD_DISCONNECT  			"Disconnect"
#define BLUEZ_DEVICE_INTERFACE_METHOD_PAIR  				"Pair"
#define BLUEZ_DEVICE_INTERFACE_METHOD_UNPAIR  				"CancelPairing"

/* BLUEZ_ADAPTER_INTERFACE */
#define BLUEZ_ADAPTER_INTERFACE								"org.bluez.Adapter1"
#define BLUEZ_ADAPTER_INTERFACE_METHOD_START_SCAN			"StartDiscovery"
#define BLUEZ_ADAPTER_INTERFACE_METHOD_STOP_SCAN			"StopDiscovery"
#define BLUEZ_ADAPTER_INTERFACE_PROP_ALIAS  				BLUEZ_DEVICE_INTERFACE_PROP_ALIAS
#define BLUEZ_ADAPTER_INTERFACE_PROP_ADDRESS				BLUEZ_DEVICE_INTERFACE_PROP_ADDRESS
#define BLUEZ_ADAPTER_INTERFACE_PROP_POWERED				"Powered"
#define BLUEZ_ADAPTER_INTERFACE_PROP_DISCOVERABLE			"Discoverable"
#define BLUEZ_ADAPTER_INTERFACE_PROP_NAME					"Name"

/* PROPERTIES_INTERFACE */
#define PROPERTIES_INTERFACE 								"org.freedesktop.DBus.Properties"
#define PROPERTIES_INTERFACE_EVENT_PROP_CHANGED				"PropertiesChanged"

/* OBJECT_MANAGER_INTERFACE */
#define OBJECT_MANAGER_INTERFACE							"org.freedesktop.DBus.ObjectManager"
#define OBJECT_MANAGER_INTERFACE_METHOD_GET_MANAGED_OBJS	"GetManagedObjects"
#define OBJECT_MANAGER_INTERFACE_EVENT_INTERFACE_ADDED		"InterfacesAdded"
#define OBJECT_MANAGER_INTERFACE_EVENT_INTERFACE_REMOVED	"InterfacesRemoved"

/* GATT_CHAR_INTERFACE */
#define GATT_CHAR_INTERFACE									"org.bluez.GattCharacteristic1"
#define GATT_CHAR_INTERFACE_METHOD_START_NOTIFY				"StartNotify"
#define GATT_CHAR_INTERFACE_METHOD_STOP_NOTIFY				"StopNotify"
#define GATT_CHAR_INTERFACE_METHOD_READ_VALUE				"ReadValue"

/* GATT_SERVICE_INTERFACE */
#define GATT_SERVICE_INTERFACE 								"org.bluez.GattService1"
#define GATT_SERVICE_INTERFACE_PROP_UUID					"UUID"
#define GATT_SERVICE_INTERFACE_PROP_PRIMARY					"Primary"
#define GATT_SERVICE_INTERFACE_PROP_DEVICE					"Device"
#define GATT_SERVICE_INTERFACE_PROP_HANDLE					"Handle"


#endif
