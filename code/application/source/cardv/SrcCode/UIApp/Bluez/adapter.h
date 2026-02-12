
#ifndef CARDV_UIAPP_BLUEZ_ADAPTER_H_
#define CARDV_UIAPP_BLUEZ_ADAPTER_H_


#include "comm.h"

typedef struct _BlueZAdapter {

	struct _BlueZDBusPropProxy* 	adapter_prop_proxy;
	struct _BlueZDBusProxy*			adapter_proxy;
	char*					mac;
	char*					alias;
	char*					name;
	int 					ref_count;

} BlueZAdapter;


BlueZAdapter* 	bluez_adapter_create(char* adapter_obj_path);
void			bluez_adapter_destroy(BlueZAdapter* adapter);

BlueZAdapter* 	bluez_adapter_ref(BlueZAdapter* adapter);
void		 	bluez_adapter_unref(BlueZAdapter* adapter);

bool			bluez_adapter_set_powered(BlueZAdapter* adapter, const bool powered);
bool			bluez_adapter_set_discoverable(BlueZAdapter* adapter, const bool discoverable);
bool			bluez_adapter_start_scan(BlueZAdapter* adapter);
bool			bluez_adapter_stop_scan(BlueZAdapter* adapter);
bool			bluez_adapter_set_alias(BlueZAdapter* adapter, const char* alias);
const char* 	bluez_adapter_get_alias(BlueZAdapter* adapter);
const char* 	bluez_adapter_get_name(BlueZAdapter* adapter);
const char* 	bluez_adapter_get_mac(BlueZAdapter* adapter);

#endif
