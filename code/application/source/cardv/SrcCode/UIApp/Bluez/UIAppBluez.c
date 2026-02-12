// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Google Inc.
 *
 *
 */


#include "device_manager.h"
#include "connection.h"
#include "device.h"
#include "adapter.h"
#include "gatt.h"
#include "UIAppBluez.h"
#include "UIFramework.h"

static BlueZDeviceManager* device_manager = NULL;
static BlueZAdapter* adapter = NULL;

static void device_user_callback (BlueZDevice* device, BLUEZ_DEVICE_EVENT ev, void* user_data);
static void char_value_changed_callback(const BlueZGattChar* bluez_gatt_char, const BlueZGattCharValue* value, void* user_data);

INT32 BlueZExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(!device_manager){
		DBG_DUMP("create bluez device manager\n");

		device_manager = bluez_device_manager_create();
		if(device_manager)
			adapter = bluez_device_manager_get_adapter(device_manager);
	}
	else{
		DBG_WRN("already opened\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(device_manager){
		DBG_DUMP("destroy bluez device manager\n");
		bluez_device_manager_destroy(device_manager);
		device_manager = NULL;
		adapter = NULL;
	}
	else{
		DBG_WRN("already closed\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnConnect(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(device_manager){

		UINT32 connect = 1;

		if(paramNum > 0){

			char* pattern = (char*)paramArray[0];

			if(paramNum > 1){
				connect = (UINT32) paramArray[1];
			}

			BlueZDevice* device = bluez_device_manager_get_device(device_manager, pattern);

			if(device){

				bluez_device_register_user_callback(device, device_user_callback, NULL);

				if(connect)
					bluez_device_connect(device);
				else
					bluez_device_disconnect(device);

				bluez_device_unref(device);
			}
			else{
				DBG_ERR("device(%s) not found!\n", pattern);
			}
		}
	}
	else{
		DBG_ERR("device manager is null!\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnPowered(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(adapter){
		UINT32 powered = 1;

		if(paramNum > 0)
			powered = paramArray[0];

		bluez_adapter_set_powered(adapter, powered ? true : false);
	}
	else{
		DBG_ERR("adapter is null!\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnScan(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(adapter){
		UINT32 scan = 1;

		if(paramNum > 0)
			scan = paramArray[0];

		if(scan)
			bluez_adapter_start_scan(adapter);
		else
			bluez_adapter_stop_scan(adapter);
	}
	else{
		DBG_ERR("adapter is null!\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnPair(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(device_manager){

		BOOL pair = TRUE;

		if(paramNum > 0){

			char* pattern = (char*)paramArray[0];

			if(paramNum > 1){
				pair = (BOOL) paramArray[1];
			}

			BlueZDevice* device = bluez_device_manager_get_device(device_manager, pattern);

			if(device){

				bluez_device_register_user_callback(device, device_user_callback, NULL);

				if(pair == TRUE)
					bluez_device_pair(device);
				else
					bluez_device_unpair(device);

				bluez_device_unref(device);
			}
			else{
				DBG_ERR("device(%s) not found!\n", pattern);
			}
		}
	}
	else{
		DBG_ERR("device manager is null!\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnTrust(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(device_manager){

		UINT32 trust = 1;

		if(paramNum > 0){

			char* pattern = (char*)paramArray[0];

			if(paramNum > 1){
				trust = paramArray[1];
			}

			BlueZDevice* device = bluez_device_manager_get_device(device_manager, pattern);

			if(device){
				bluez_device_register_user_callback(device, device_user_callback, NULL);
				bluez_device_set_trusted(device, trust ? true : false);
				bluez_device_unref(device);
			}
			else{
				DBG_ERR("device(%s) not found!\n", pattern);
			}
		}
	}
	else{
		DBG_ERR("device manager is null!\n");
	}

	return NVTEVT_CONSUME;
}

INT32 BlueZExe_OnGattCharNotify(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if(device_manager){

		char* char_obj_path = (char*)paramArray[0];
		bool enabled = paramArray[1];
		BlueZGattDB* gatt_db = device_manager->gatt_db;
		bool ret;
		BlueZGattChar* bluez_gatt_char = bluez_gatt_db_find_char(gatt_db, char_obj_path);

		if(bluez_gatt_char){
			if(enabled){

				DBG_DUMP("start notify %s\n", char_obj_path);

				ret = bluez_gatt_char_start_notify(bluez_gatt_char);

				if(ret)
					bluez_gatt_char_register_value_changed_callback(bluez_gatt_char, char_value_changed_callback, NULL);
			}
			else{
				bluez_gatt_char_stop_notify(bluez_gatt_char);
			}

			bluez_gatt_char_unref(bluez_gatt_char);
		}
	}

	return TRUE;
}

void print_name_func (gpointer device, gpointer  user_data)
{
	BlueZDevice* dev = device;

	DBG_DUMP(" %s %s %s\n",
			dev->object_path,
			dev->alias,
			dev->mac

	);
}

void connect_func (gpointer device, gpointer  user_data)
{
	BlueZDevice* dev = device;
	char* mac = (char*) user_data;

	if(!g_strcmp0(dev->mac, mac)){
		bluez_device_connect(dev);
	}
}

void device_user_callback (BlueZDevice* device, BLUEZ_DEVICE_EVENT ev, void* user_data)
{

	switch(ev)
	{
	case BLUEZ_DEVICE_EVENT_CONNECTED:
		DBG_DUMP("%s connected\n", device->alias);
		break;

	case BLUEZ_DEVICE_EVENT_DISCONNECTED:
		DBG_DUMP("%s disconnected\n", device->alias);
		break;

	case BLUEZ_DEVICE_EVENT_PAIRED:
		DBG_DUMP("%s paired\n", device->alias);
		break;

	case BLUEZ_DEVICE_EVENT_UNPAIRED:
		DBG_DUMP("%s unpaired\n", device->alias);
		break;

	default:
		break;
	}
}

void char_value_changed_callback(const BlueZGattChar* bluez_gatt_char, const BlueZGattCharValue* value, void* user_data)
{


	DBG_DUMP("is_string = %u len = %u value byte array = ", value->is_string, value->len);

	for(unsigned int i = 0 ; i < value->len ; i++)
	{
		DBG_DUMP("0x%02x ", value->bytes[i]);
	}

	DBG_DUMP("\n");

}

EVENT_ENTRY CustomBlueZObjCmdMap[] = {
	{NVTEVT_EXE_BLUEZ_OPEN,               BlueZExe_OnOpen                 },
	{NVTEVT_EXE_BLUEZ_CLOSE,              BlueZExe_OnClose                },
	{NVTEVT_EXE_BLUEZ_POWERED,            BlueZExe_OnPowered              },
	{NVTEVT_EXE_BLUEZ_SCAN,               BlueZExe_OnScan                 },
	{NVTEVT_EXE_BLUEZ_CONNECT,            BlueZExe_OnConnect              },
	{NVTEVT_EXE_BLUEZ_PAIR,               BlueZExe_OnPair                 },
	{NVTEVT_EXE_BLUEZ_TRUST,              BlueZExe_OnTrust                },
	{NVTEVT_EXE_BLUEZ_GATT_CHAR_NOTIFY,   BlueZExe_OnGattCharNotify       },
};

CREATE_APP(CustomBlueZObj, APP_SETUP)

#include <linux/input.h>
#include <kwrap/sxcmd.h>
#include <kwrap/cmdsys.h>
#include <kwrap/util.h>

static struct input_event ev[64];
static int ev_fd = 0;
static THREAD_HANDLE hogp_task_id = 0;
static bool task_running = false;
static bool is_task_running = false;

THREAD_RETTYPE hogp_task(void)
{
	int n = 0;

	DBG_DUMP("hogp_task created\n");

	is_task_running = true;

	while(task_running)
	{
		if(ev_fd <= 0){
			ev_fd = open("/dev/input/event0",  O_RDWR | O_NOCTTY | O_NDELAY);
			if(ev_fd <= 0){
				vos_util_delay_ms(500);
				continue;
			}
			else{
				DBG_DUMP("found hogp event fd\n");
			}
		}

		n = read(ev_fd, (void*)ev, sizeof(struct input_event)*64);

	      if (n < 0) {
	        DBG_IND("Read failed\n");
	        vos_util_delay_ms(10);
	      }
	      else if(n == 0) {
	        DBG_IND("No data on port\n");
	        vos_util_delay_ms(10);
	      }
		else{
			for(int i=0 ; i<(int)(n / sizeof(struct input_event)) ; i++)
			{
				if(EV_KEY == ev[i].type)
				{

				DBG_DUMP("ev%d time: %ld.%06ld type:%d code:%d value:%d\n",
							i,
							ev[i].time.tv_sec,
							ev[i].time.tv_usec,
							ev[i].type,
							ev[i].code,
							ev[i].value
						);
				}
				else{
					DBG_DUMP("unknown ev type(%d)\n", ev[i].type);
				}
			}
		}
	}

	DBG_DUMP("hogp_task destroyed\n");

	is_task_running = 0;

	THREAD_RETURN(0);
}

static BOOL cmd_uibluez_init(unsigned char argc, char **argv)
{
	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_OPEN, 0);
	return TRUE;
}

static BOOL cmd_uibluez_set_powered(unsigned char argc, char **argv)
{
	UINT32 enabled = strtoul(argv[0], NULL, 10);

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_POWERED, 1, enabled);

	return TRUE;
}

static BOOL cmd_uibluez_show_discovered_devices(unsigned char argc, char **argv)
{
	GList* devices = bluez_device_manager_get_discovered_devices(device_manager);

	g_list_foreach(devices, print_name_func, NULL);

	g_list_free(devices);

	return TRUE;
}

static BOOL cmd_uibluez_connect_device(unsigned char argc, char **argv)
{
	char* pattern = argv[0];

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_CONNECT, 2, pattern, 1);

	return TRUE;
}

static BOOL cmd_uibluez_disconnect_device(unsigned char argc, char **argv)
{
	char* pattern = argv[0];

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_CONNECT, 2, pattern, 0);

	return TRUE;
}

static BOOL cmd_uibluez_pair_device(unsigned char argc, char **argv)
{
	char* pattern = argv[0];

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_PAIR, 2, pattern, 1);

	return TRUE;
}

static BOOL cmd_uibluez_unpair_device(unsigned char argc, char **argv)
{
	char* pattern = argv[0];

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_PAIR, 2, pattern, 0);

	return TRUE;
}

static BOOL cmd_uibluez_trust_device(unsigned char argc, char **argv)
{
	char* pattern = argv[0];

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_TRUST, 2, pattern, 1);

	return TRUE;
}

static BOOL cmd_uibluez_untrust_device(unsigned char argc, char **argv)
{
	char* pattern = argv[0];

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_TRUST, 2, pattern, 0);

	return TRUE;
}

static BOOL cmd_uibluez_scan(unsigned char argc, char **argv)
{
	bool enabled = true;

	if(argc > 0){
		enabled = strtoul(argv[0], NULL, 10);

		Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_SCAN, 1, enabled);
	}

	return TRUE;
}

static BOOL cmd_uibluez_device_status(unsigned char argc, char **argv)
{
	if(argc > 0){
		char* pattern = argv[0];
		BlueZDevice* device = bluez_device_manager_get_device(device_manager, pattern);
		if(device){

			DBG_DUMP("%s: connected:%u paired:%u trusted:%u\n",
					pattern,
					bluez_device_is_connected(device),
					bluez_device_is_paired(device),
					bluez_device_is_trusted(device)
			);
		}
	}

	return TRUE;
}

static BOOL cmd_uibluez_gatt_char_notify(unsigned char argc, char **argv)
{
	char* char_obj_path = argv[0];
	UINT32 enabled = strtoul(argv[1], NULL, 10);

	Ux_BlueZ_SendEvent(NVTEVT_EXE_BLUEZ_GATT_CHAR_NOTIFY, 2, char_obj_path, enabled);

	return TRUE;
}

static BOOL cmd_uibluez_hogp_start(unsigned char argc, char **argv)
{
	if(!hogp_task_id){

		hogp_task_id = vos_task_create(hogp_task, 0, "hogp_task", 9, 2048);

		if(hogp_task_id){
			task_running = true;
			vos_task_resume(hogp_task_id);
		}
	}

	return TRUE;
}

static BOOL cmd_uibluez_hogp_stop(unsigned char argc, char **argv)
{
	if(hogp_task_id && task_running){

		task_running = 0;

		while(is_task_running)
		{
			vos_util_delay_ms(10);
		}

		hogp_task_id = 0;
	}

	return TRUE;
}

static BOOL cmd_uibluez_gatt_local_service_register(unsigned char argc, char **argv)
{
	bluez_gatt_local_service_register(device_manager, "0000-0000-0000", 0);
	return TRUE;
}



SXCMD_BEGIN(uibluez_cmd_tbl, "nvt bluez command")
SXCMD_ITEM("init", cmd_uibluez_init, "init")
SXCMD_ITEM("connect", cmd_uibluez_connect_device, "connect")
SXCMD_ITEM("disconnect", cmd_uibluez_disconnect_device, "disconnect")
SXCMD_ITEM("pair", cmd_uibluez_pair_device, "pair")
SXCMD_ITEM("unpair", cmd_uibluez_unpair_device, "unpair")
SXCMD_ITEM("trust", cmd_uibluez_trust_device, "trust")
SXCMD_ITEM("untrust", cmd_uibluez_untrust_device, "untrust")
SXCMD_ITEM("show", cmd_uibluez_show_discovered_devices, "show")
SXCMD_ITEM("powered", cmd_uibluez_set_powered, "powered")
SXCMD_ITEM("scan", cmd_uibluez_scan, "scan")
SXCMD_ITEM("status", cmd_uibluez_device_status, "dev status")
SXCMD_ITEM("gatt_char_notify", cmd_uibluez_gatt_char_notify, "gatt_char_notify")
SXCMD_ITEM("hogp_test_start", cmd_uibluez_hogp_start, "hogp_test_start")
SXCMD_ITEM("hogp_test_stop", cmd_uibluez_hogp_stop, "hogp_test_stop")
SXCMD_ITEM("gatt_local_service_register", cmd_uibluez_gatt_local_service_register, "gatt_local_service_register")
SXCMD_END()

static int uibluez_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(uibluez_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "uibluez");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", uibluez_cmd_tbl[loop].p_name, uibluez_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(uibluez, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(uibluez_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		uibluez_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], uibluez_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = uibluez_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	uibluez_showhelp(vk_printk);
	return 0;
}
