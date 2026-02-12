// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Google Inc.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/l2cap.h"
#include "lib/uuid.h"
#include "lib/mgmt.h"
#include "lib/sdp.h"
#include "lib/sdp_lib.h"

#include "monitor/bt.h"

#include "src/shared/mainloop.h"
#include "src/shared/util.h"
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/timeout.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-server.h"
#include "src/shared/mgmt.h"
#include "src/shared/hci.h"
#include "src/shared/ad.h"

#include "kwrap/nvt_type.h"
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include <kwrap/error_no.h>
#include <signal.h>
#include <sys/syscall.h>

#include "UIApp/Network/UIAppNetwork.h"

#define UUID_GAP			0x1800
#define UUID_GATT			0x1801
#define UUID_HEART_RATE			0x180d
#define UUID_HEART_RATE_MSRMT		0x2a37
#define UUID_HEART_RATE_BODY		0x2a38
#define UUID_HEART_RATE_CTRL		0x2a39

#define UUID_STRING_NVT_WIFI_INFO_EXCHAGE	"E1453A21-1492-468D-B9CC-7B2239290344"
#define GATT_CHARAC_NVT_WIFI_SSID			"50C12E68-6F2F-472A-831F-D0D8B1917F83"
#define GATT_CHARAC_NVT_WIFI_PASSPHRASE		"BB93A952-C808-49B7-8AB0-26F15AFCA77D"

#define LOCAL_NAME_COMPLETE	"NVT Complete Name"
#define LOCAL_NAME_SHORT	"NVT Short Name"

#define ATT_CID 4
#define MAX_AD_UUID_BYTES 32

#define PRLOG(...) \
	do { \
		printf(__VA_ARGS__); \
		print_prompt(); \
	} while (0)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define COLOR_OFF	"\x1B[0m"
#define COLOR_RED	"\x1B[0;91m"
#define COLOR_GREEN	"\x1B[0;92m"
#define COLOR_YELLOW	"\x1B[0;93m"
#define COLOR_BLUE	"\x1B[0;94m"
#define COLOR_MAGENTA	"\x1B[0;95m"
#define COLOR_BOLDGRAY	"\x1B[1;30m"
#define COLOR_BOLDWHITE	"\x1B[1;37m"

static const char test_device_name[] = "Very Long Test Device Name For Testing "
				"ATT Protocol Operations On GATT Server";
static bool verbose = true;

struct server {
	int fd;
	struct bt_att *att;
	struct gatt_db *db;
	struct bt_gatt_server *gatt;

	uint8_t *device_name;
	size_t name_len;

	uint16_t gatt_svc_chngd_handle;
	bool svc_chngd_enabled;

	uint16_t hr_handle;
	uint16_t hr_msrmt_handle;
	uint16_t hr_energy_expended;
	bool hr_visible;
	bool hr_msrmt_enabled;
	int hr_ee_count;
	unsigned int hr_timeout_id;
};

typedef struct {

	int sec;			/* BT_SECURITY_LOW BT_SECURITY_MEDIUM BT_SECURITY_HIGH */
	char* dev_name; 	/* hci0 */
	uint8_t src_type ;
	uint16_t mtu;

	struct {
			int dev_id;
			int client_fd;
			int server_fd;
			bdaddr_t src_addr;
			struct server *server;
			VK_TASK_HANDLE task_id;
			bool is_running;
	} priv;

} nvt_bluez_gatt;


static nvt_bluez_gatt nvt_bluez_gatt_handle = {
		.sec = BT_SECURITY_LOW,
		.dev_name = "hci0",
		.src_type = BDADDR_LE_PUBLIC,
		.mtu = 0, /* default */

		.priv.dev_id = 0,
		.priv.client_fd = 0,
		.priv.src_addr = {0},
		.priv.is_running = false,
		.priv.task_id = 0,
};

static void print_prompt(void)
{
	printf(COLOR_BLUE "[GATT server]" COLOR_OFF "# ");
	fflush(stdout);
}

static int l2cap_le_att_listen(bdaddr_t *src, int sec,
							uint8_t src_type);

static struct server *server_create(int fd, uint16_t mtu, bool hr_visible);

static void att_disconnect_cb(int err, void *user_data)
{
	printf("Device disconnected: %s\n", strerror(err));

//	mainloop_quit();
}

static void att_debug_cb(const char *str, void *user_data)
{
	const char *prefix = user_data;

	PRLOG(COLOR_BOLDGRAY "%s" COLOR_BOLDWHITE "%s\n" COLOR_OFF, prefix,
									str);
}

static void gatt_debug_cb(const char *str, void *user_data)
{
	const char *prefix = user_data;

	PRLOG(COLOR_GREEN "%s%s\n" COLOR_OFF, prefix, str);
}

static void gap_device_name_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t error = 0;
	size_t len = 0;
	const uint8_t *value = NULL;

	PRLOG("GAP Device Name Read called\n");

	len = server->name_len;

	if (offset > len) {
		error = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	len -= offset;
	value = len ? &server->device_name[offset] : NULL;

done:
	gatt_db_attribute_read_result(attrib, id, error, value, len);
}

static void gap_device_name_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t error = 0;

	PRLOG("GAP Device Name Write called\n");

	/* If the value is being completely truncated, clean up and return */
	if (!(offset + len)) {
		free(server->device_name);
		server->device_name = NULL;
		server->name_len = 0;
		goto done;
	}

	/* Implement this as a variable length attribute value. */
	if (offset > server->name_len) {
		error = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	if (offset + len != server->name_len) {
		uint8_t *name;

		name = realloc(server->device_name, offset + len);
		if (!name) {
			error = BT_ATT_ERROR_INSUFFICIENT_RESOURCES;
			goto done;
		}

		server->device_name = name;
		server->name_len = offset + len;
	}

	if (value)
		memcpy(server->device_name + offset, value, len);

done:
	gatt_db_attribute_write_result(attrib, id, error);
}

static void gap_device_name_ext_prop_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	uint8_t value[2];

	PRLOG("Device Name Extended Properties Read called\n");

	value[0] = BT_GATT_CHRC_EXT_PROP_RELIABLE_WRITE;
	value[1] = 0;

	gatt_db_attribute_read_result(attrib, id, 0, value, sizeof(value));
}

static void gatt_service_changed_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	PRLOG("Service Changed Read called\n");

	gatt_db_attribute_read_result(attrib, id, 0, NULL, 0);
}

static void gatt_svc_chngd_ccc_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t value[2];

	PRLOG("Service Changed CCC Read called\n");

	value[0] = server->svc_chngd_enabled ? 0x02 : 0x00;
	value[1] = 0x00;

	gatt_db_attribute_read_result(attrib, id, 0, value, sizeof(value));
}

static void gatt_svc_chngd_ccc_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t ecode = 0;

	PRLOG("Service Changed CCC Write called\n");

	if (!value || len != 2) {
		ecode = BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN;
		goto done;
	}

	if (offset) {
		ecode = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	if (value[0] == 0x00)
		server->svc_chngd_enabled = false;
	else if (value[0] == 0x02)
		server->svc_chngd_enabled = true;
	else
		ecode = 0x80;

	PRLOG("Service Changed Enabled: %s\n",
				server->svc_chngd_enabled ? "true" : "false");

done:
	gatt_db_attribute_write_result(attrib, id, ecode);
}

static void hr_msrmt_ccc_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t value[2];

	value[0] = server->hr_msrmt_enabled ? 0x01 : 0x00;
	value[1] = 0x00;

	gatt_db_attribute_read_result(attrib, id, 0, value, 2);
}

static bool hr_msrmt_cb(void *user_data)
{
	struct server *server = user_data;
	bool expended_present = !(server->hr_ee_count % 10);
	uint16_t len = 2;
	uint8_t pdu[4];
	uint32_t cur_ee;
	uint32_t val;

	if (util_getrandom(&val, sizeof(val), 0) < 0)
		return false;

	pdu[0] = 0x06;
	pdu[1] = 90 + (val % 40);

	if (expended_present) {
		pdu[0] |= 0x08;
		put_le16(server->hr_energy_expended, pdu + 2);
		len += 2;
	}

	bt_gatt_server_send_notification(server->gatt,
						server->hr_msrmt_handle,
						pdu, len, false);


	cur_ee = server->hr_energy_expended;
	server->hr_energy_expended = MIN(UINT16_MAX, cur_ee + 10);
	server->hr_ee_count++;

	return true;
}

static void update_hr_msrmt_simulation(struct server *server)
{
	if (!server->hr_msrmt_enabled || !server->hr_visible) {
		timeout_remove(server->hr_timeout_id);
		return;
	}

	server->hr_timeout_id = timeout_add(1000, hr_msrmt_cb, server, NULL);
}

static void hr_msrmt_ccc_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t ecode = 0;

	if (!value || len != 2) {
		ecode = BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN;
		goto done;
	}

	if (offset) {
		ecode = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	if (value[0] == 0x00)
		server->hr_msrmt_enabled = false;
	else if (value[0] == 0x01) {
		if (server->hr_msrmt_enabled) {
			PRLOG("HR Measurement Already Enabled\n");
			goto done;
		}

		server->hr_msrmt_enabled = true;
	} else
		ecode = 0x80;

	PRLOG("HR: Measurement Enabled: %s\n",
				server->hr_msrmt_enabled ? "true" : "false");

	update_hr_msrmt_simulation(server);

done:
	gatt_db_attribute_write_result(attrib, id, ecode);
}

static void hr_control_point_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct server *server = user_data;
	uint8_t ecode = 0;

	if (!value || len != 1) {
		ecode = BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN;
		goto done;
	}

	if (offset) {
		ecode = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	if (value[0] == 1) {
		PRLOG("HR: Energy Expended value reset\n");
		server->hr_energy_expended = 0;
	}

done:
	gatt_db_attribute_write_result(attrib, id, ecode);
}

static void confirm_write(struct gatt_db_attribute *attr, int err,
							void *user_data)
{
	if (!err)
		return;

	fprintf(stderr, "Error caching attribute %p - err: %d\n", attr, err);
	exit(1);
}


/*****************************************/

static void gap_device_wifi_ssid_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	uint8_t error = 0;
	const uint8_t *value = NULL;

	PRLOG("GAP Device WiFi SSID Read called\n");

	value = (uint8_t *) UINet_GetSSID();
	gatt_db_attribute_read_result(attrib, id, error, value, strlen((const char*)value));
}

static void gap_device_wifi_passphrase_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	uint8_t error = 0;
	const uint8_t *value = NULL;

	PRLOG("GAP Device WiFi Passphrase Read called\n");

	value = (uint8_t *) UINet_GetPASSPHRASE();
	gatt_db_attribute_read_result(attrib, id, error, value, strlen((const char*)value));
}

static void print_uuid(const bt_uuid_t *uuid);

static void populate_wifi_info_exchange_service(struct server *server)
{
	bt_uuid_t uuid;
	struct gatt_db_attribute *service;

	/* Add the Vendor service */
	bt_string_to_uuid(&uuid, UUID_STRING_NVT_WIFI_INFO_EXCHAGE);
	print_uuid(&uuid);
	service = gatt_db_add_service(server->db, &uuid, true, 6);


	bt_string_to_uuid(&uuid, GATT_CHARAC_NVT_WIFI_SSID);
	print_uuid(&uuid);
	gatt_db_service_add_characteristic(service, &uuid,
					BT_ATT_PERM_READ,
					BT_GATT_CHRC_PROP_READ,
					gap_device_wifi_ssid_read_cb,
					NULL,
					server);

	bt_string_to_uuid(&uuid, GATT_CHARAC_NVT_WIFI_PASSPHRASE);
	print_uuid(&uuid);
	gatt_db_service_add_characteristic(service, &uuid,
					BT_ATT_PERM_READ,
					BT_GATT_CHRC_PROP_READ,
					gap_device_wifi_passphrase_read_cb,
					NULL,
					server);

	gatt_db_service_set_active(service, true);
}


static void populate_gap_service(struct server *server)
{
	bt_uuid_t uuid;
	struct gatt_db_attribute *service, *tmp;
	uint16_t appearance;

	/* Add the GAP service */
	bt_uuid16_create(&uuid, UUID_GAP);
	service = gatt_db_add_service(server->db, &uuid, true, 6);

	/*
	 * Device Name characteristic. Make the value dynamically read and
	 * written via callbacks.
	 */
	bt_uuid16_create(&uuid, GATT_CHARAC_DEVICE_NAME);
	gatt_db_service_add_characteristic(service, &uuid,
					BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
					BT_GATT_CHRC_PROP_READ |
					BT_GATT_CHRC_PROP_EXT_PROP,
					gap_device_name_read_cb,
					gap_device_name_write_cb,
					server);

	bt_uuid16_create(&uuid, GATT_CHARAC_EXT_PROPER_UUID);
	gatt_db_service_add_descriptor(service, &uuid, BT_ATT_PERM_READ,
					gap_device_name_ext_prop_read_cb,
					NULL, server);

	/*
	 * Appearance characteristic. Reads and writes should obtain the value
	 * from the database.
	 */
	bt_uuid16_create(&uuid, GATT_CHARAC_APPEARANCE);
	tmp = gatt_db_service_add_characteristic(service, &uuid,
							BT_ATT_PERM_READ,
							BT_GATT_CHRC_PROP_READ,
							NULL, NULL, server);

	/*
	 * Write the appearance value to the database, since we're not using a
	 * callback.
	 */
	put_le16(128, &appearance);
	gatt_db_attribute_write(tmp, 0, (void *) &appearance,
							sizeof(appearance),
							BT_ATT_OP_WRITE_REQ,
							NULL, confirm_write,
							NULL);

	gatt_db_service_set_active(service, true);
}

static void populate_gatt_service(struct server *server)
{
	bt_uuid_t uuid;
	struct gatt_db_attribute *service, *svc_chngd;

	/* Add the GATT service */
	bt_uuid16_create(&uuid, UUID_GATT);
	service = gatt_db_add_service(server->db, &uuid, true, 4);

	bt_uuid16_create(&uuid, GATT_CHARAC_SERVICE_CHANGED);
	svc_chngd = gatt_db_service_add_characteristic(service, &uuid,
			BT_ATT_PERM_READ,
			BT_GATT_CHRC_PROP_READ | BT_GATT_CHRC_PROP_INDICATE,
			gatt_service_changed_cb,
			NULL, server);
	server->gatt_svc_chngd_handle = gatt_db_attribute_get_handle(svc_chngd);

	bt_uuid16_create(&uuid, GATT_CLIENT_CHARAC_CFG_UUID);
	gatt_db_service_add_descriptor(service, &uuid,
				BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
				gatt_svc_chngd_ccc_read_cb,
				gatt_svc_chngd_ccc_write_cb, server);

	gatt_db_service_set_active(service, true);
}

static void populate_hr_service(struct server *server)
{
	bt_uuid_t uuid;
	struct gatt_db_attribute *service, *hr_msrmt, *body;
	uint8_t body_loc = 1;  /* "Chest" */

	/* Add Heart Rate Service */
	bt_uuid16_create(&uuid, UUID_HEART_RATE);
	service = gatt_db_add_service(server->db, &uuid, true, 8);
	server->hr_handle = gatt_db_attribute_get_handle(service);

	/* HR Measurement Characteristic */
	bt_uuid16_create(&uuid, UUID_HEART_RATE_MSRMT);
	hr_msrmt = gatt_db_service_add_characteristic(service, &uuid,
						BT_ATT_PERM_NONE,
						BT_GATT_CHRC_PROP_NOTIFY,
						NULL, NULL, NULL);
	server->hr_msrmt_handle = gatt_db_attribute_get_handle(hr_msrmt);

	bt_uuid16_create(&uuid, GATT_CLIENT_CHARAC_CFG_UUID);
	gatt_db_service_add_descriptor(service, &uuid,
					BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
					hr_msrmt_ccc_read_cb,
					hr_msrmt_ccc_write_cb, server);

	/*
	 * Body Sensor Location Characteristic. Make reads obtain the value from
	 * the database.
	 */
	bt_uuid16_create(&uuid, UUID_HEART_RATE_BODY);
	body = gatt_db_service_add_characteristic(service, &uuid,
						BT_ATT_PERM_READ,
						BT_GATT_CHRC_PROP_READ,
						NULL, NULL, server);
	gatt_db_attribute_write(body, 0, (void *) &body_loc, sizeof(body_loc),
							BT_ATT_OP_WRITE_REQ,
							NULL, confirm_write,
							NULL);

	/* HR Control Point Characteristic */
	bt_uuid16_create(&uuid, UUID_HEART_RATE_CTRL);
	gatt_db_service_add_characteristic(service, &uuid,
						BT_ATT_PERM_WRITE,
						BT_GATT_CHRC_PROP_WRITE,
						NULL, hr_control_point_write_cb,
						server);

	if (server->hr_visible)
		gatt_db_service_set_active(service, true);
}

static void populate_db(struct server *server)
{
	populate_gap_service(server);
	populate_gatt_service(server);
	populate_hr_service(server);
	populate_wifi_info_exchange_service(server);
}

static struct server *server_create(int fd, uint16_t mtu, bool hr_visible)
{
	struct server *server;
	size_t name_len = strlen(test_device_name);

	server = new0(struct server, 1);
	if (!server) {
		fprintf(stderr, "Failed to allocate memory for server\n");
		return NULL;
	}

	server->att = bt_att_new(fd, false);
	if (!server->att) {
		fprintf(stderr, "Failed to initialze ATT transport layer\n");
		goto fail;
	}

	if (!bt_att_set_close_on_unref(server->att, true)) {
		fprintf(stderr, "Failed to set up ATT transport layer\n");
		goto fail;
	}

	if (!bt_att_register_disconnect(server->att, att_disconnect_cb, (void*) &nvt_bluez_gatt_handle,
									NULL)) {
		fprintf(stderr, "Failed to set ATT disconnect handler\n");
		goto fail;
	}

	server->name_len = name_len + 1;
	server->device_name = malloc(name_len + 1);
	if (!server->device_name) {
		fprintf(stderr, "Failed to allocate memory for device name\n");
		goto fail;
	}

	memcpy(server->device_name, test_device_name, name_len);
	server->device_name[name_len] = '\0';

	server->fd = fd;
	server->db = gatt_db_new();
	if (!server->db) {
		fprintf(stderr, "Failed to create GATT database\n");
		goto fail;
	}

	server->gatt = bt_gatt_server_new(server->db, server->att, mtu, 0);
	if (!server->gatt) {
		fprintf(stderr, "Failed to create GATT server\n");
		goto fail;
	}

	server->hr_visible = hr_visible;

	if (verbose) {
		bt_att_set_debug(server->att, BT_ATT_DEBUG_VERBOSE,
						att_debug_cb, "att: ", NULL);
		bt_gatt_server_set_debug(server->gatt, gatt_debug_cb,
							"server: ", NULL);
	}

	/* Random seed for generating fake Heart Rate measurements */
	srand(time(NULL));

	/* bt_gatt_server already holds a reference */
	populate_db(server);

	return server;

fail:
	gatt_db_unref(server->db);
	free(server->device_name);
	bt_att_unref(server->att);
	free(server);

	return NULL;
}

static void server_destroy(struct server *server)
{
	timeout_remove(server->hr_timeout_id);
	bt_gatt_server_unref(server->gatt);
	gatt_db_unref(server->db);
}

static int l2cap_le_att_listen(bdaddr_t *src, int sec, uint8_t src_type)
{
	int sk;
	struct sockaddr_l2 srcaddr;
	struct bt_security btsec;

	sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (sk < 0) {
		perror("Failed to create L2CAP socket");
		return -1;
	}

	/* Set up source address */
	memset(&srcaddr, 0, sizeof(srcaddr));
	srcaddr.l2_family = AF_BLUETOOTH;
	srcaddr.l2_cid = htobs(ATT_CID);
	srcaddr.l2_bdaddr_type = src_type;
	bacpy(&srcaddr.l2_bdaddr, src);

	if (bind(sk, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0) {
		perror("Failed to bind L2CAP socket");
		goto fail;
	}

	/* Set the security level */
	memset(&btsec, 0, sizeof(btsec));
	btsec.level = sec;
	if (setsockopt(sk, SOL_BLUETOOTH, BT_SECURITY, &btsec,
							sizeof(btsec)) != 0) {
		fprintf(stderr, "Failed to set L2CAP security level\n");
		goto fail;
	}

	if (listen(sk, 2) < 0) {
		perror("Listening on socket failed");
		goto fail;
	}

	printf("Started listening on ATT channel. Waiting for connections\n");

	return sk;
fail:
	close(sk);
	return -1;
}

static void print_uuid(const bt_uuid_t *uuid)
{
	char uuid_str[MAX_LEN_UUID_STR];
	bt_uuid_t uuid128;

	bt_uuid_to_uuid128(uuid, &uuid128);
	bt_uuid_to_string(&uuid128, uuid_str, sizeof(uuid_str));

	printf("%s\n", uuid_str);
}

//static void print_incl(struct gatt_db_attribute *attr, void *user_data)
//{
//	struct server *server = user_data;
//	uint16_t handle, start, end;
//	struct gatt_db_attribute *service;
//	bt_uuid_t uuid;
//
//	if (!gatt_db_attribute_get_incl_data(attr, &handle, &start, &end))
//		return;
//
//	service = gatt_db_get_attribute(server->db, start);
//	if (!service)
//		return;
//
//	gatt_db_attribute_get_service_uuid(service, &uuid);
//
//	printf("\t  " COLOR_GREEN "include" COLOR_OFF " - handle: "
//					"0x%04x, - start: 0x%04x, end: 0x%04x,"
//					"uuid: ", handle, start, end);
//	print_uuid(&uuid);
//}

//static void print_desc(struct gatt_db_attribute *attr, void *user_data)
//{
//	printf("\t\t  " COLOR_MAGENTA "descr" COLOR_OFF
//					" - handle: 0x%04x, uuid: ",
//					gatt_db_attribute_get_handle(attr));
//	print_uuid(gatt_db_attribute_get_type(attr));
//}

//static void print_chrc(struct gatt_db_attribute *attr, void *user_data)
//{
//	uint16_t handle, value_handle;
//	uint8_t properties;
//	uint16_t ext_prop;
//	bt_uuid_t uuid;
//
//	if (!gatt_db_attribute_get_char_data(attr, &handle,
//								&value_handle,
//								&properties,
//								&ext_prop,
//								&uuid))
//		return;
//
//	printf("\t  " COLOR_YELLOW "charac" COLOR_OFF
//				" - start: 0x%04x, value: 0x%04x, "
//				"props: 0x%02x, ext_prop: 0x%04x, uuid: ",
//				handle, value_handle, properties, ext_prop);
//	print_uuid(&uuid);
//
//	gatt_db_service_foreach_desc(attr, print_desc, NULL);
//}

//static void print_service(struct gatt_db_attribute *attr, void *user_data)
//{
//	struct server *server = user_data;
//	uint16_t start, end;
//	bool primary;
//	bt_uuid_t uuid;
//
//	if (!gatt_db_attribute_get_service_data(attr, &start, &end, &primary,
//									&uuid))
//		return;
//
//	printf(COLOR_RED "service" COLOR_OFF " - start: 0x%04x, "
//				"end: 0x%04x, type: %s, uuid: ",
//				start, end, primary ? "primary" : "secondary");
//	print_uuid(&uuid);
//
//	gatt_db_service_foreach_incl(attr, print_incl, server);
//	gatt_db_service_foreach_char(attr, print_chrc, NULL);
//
//	printf("\n");
//}


static void signal_cb(int signum, void *user_data)
{
	printf("received signal %d\n", signum);

	switch (signum) {
	case SIGINT:
	case SIGTERM:
		mainloop_quit();
		break;
	default:
		break;
	}
}

static struct mgmt *mgmt_if = NULL;

/* Wrapper to get the index and opcode to the response callback */
struct command_data {
	uint16_t id;
	uint16_t op;
	void (*callback) (uint16_t id, uint16_t op, uint8_t status,
					uint16_t len, const void *param);
};

static void cmd_rsp(uint8_t status, uint16_t len, const void *param,
							void *user_data)
{
	struct command_data *data = user_data;

	data->callback(data->op, data->id, status, len, param);
}

static unsigned int send_cmd(struct mgmt *mgmt, uint16_t op, uint16_t id,
				uint16_t len, const void *param,
				void (*cb)(uint16_t id, uint16_t op,
						uint8_t status, uint16_t len,
						const void *param))
{
	struct command_data *data;
	unsigned int send_id;

	data = new0(struct command_data, 1);
	if (!data)
		return 0;

	data->id = id;
	data->op = op;
	data->callback = cb;

	send_id = mgmt_send(mgmt, op, id, len, param, cmd_rsp, data, free);
	if (send_id == 0)
		free(data);

	return send_id;
}

static void name_rsp(uint8_t status, uint16_t len,
					const void *param, void *user_data)
{
	if (status != 0)
		printf("Unable to set local name with status 0x%02x (%s)\n",
						status, mgmt_errstr(status));
	else{
		printf("name_rsp success\n");
	}

	return;
}

static const char *settings_str[] = {
				"powered",
				"connectable",
				"fast-connectable",
				"discoverable",
				"bondable",
				"link-security",
				"ssp",
				"br/edr",
				"hs",
				"le",
				"advertising",
				"secure-conn",
				"debug-keys",
				"privacy",
				"configuration",
				"static-addr",
				"phy-configuration",
				"wide-band-speech",
};

static const char *settings2str(uint32_t settings)
{
	static char str[256];
	unsigned i;
	int off;

	off = 0;
	str[0] = '\0';

	for (i = 0; i < NELEM(settings_str); i++) {
		if ((settings & (1 << i)) != 0)
			off += snprintf(str + off, sizeof(str) - off, "%s ",
							settings_str[i]);
	}

	return str;
}

static void setting_rsp(uint16_t op, uint16_t id, uint8_t status, uint16_t len,
							const void *param)
{
	const uint32_t *rp = param;

	if (status != 0) {
		printf("%s for hci%u failed with status 0x%02x (%s)\n",
			mgmt_opstr(op), id, status, mgmt_errstr(status));
		goto done;
	}

	if (len < sizeof(*rp)) {
		printf("Too small %s response (%u bytes)\n",
							mgmt_opstr(op), len);
		goto done;
	}

	printf("hci%u %s complete, settings: %s\n", id, mgmt_opstr(op),
						settings2str(get_le32(rp)));

done:
	return;
}

static void auto_power_enable_rsp(uint8_t status, uint16_t len,
					const void *param, void *user_data)
{
	uint16_t index = PTR_TO_UINT(user_data);

	printf("Successfully enabled controller with index %u\n", index);
}

static void server_accept_callback(int fd, uint32_t events, void *user_data)
{
	nvt_bluez_gatt* handle = user_data;
	char ba[18];
	struct sockaddr_l2 addr;
	socklen_t len;

	if (events & (EPOLLERR | EPOLLHUP)) {
		mainloop_remove_fd(handle->priv.server_fd);
		return;
	}

	memset(&addr, 0, sizeof(addr));
	len = sizeof(addr);

	handle->priv.client_fd = accept(handle->priv.server_fd, (struct sockaddr *) &addr, &len);
	if (handle->priv.client_fd < 0) {
		perror("Failed to accept client socket");
		return;
	}

	ba2str(&addr.l2_bdaddr, ba);
	printf("Connect from %s\n", ba);

	handle->priv.server = server_create(handle->priv.client_fd, handle->mtu, false);
	if (!handle->priv.server) {
		fprintf(stderr, "Failed to create server\n");
		close(handle->priv.client_fd);
		return;
	}
}

struct bt_ad *data = NULL;
struct bt_ad *scan = NULL;
uint8_t instance = 0;

static void add_adv_params_callback(uint8_t status, uint16_t length,
					  const void *param, void *user_data)
{
	const struct mgmt_rp_add_ext_adv_params *rp = param;
	struct mgmt_cp_add_ext_adv_data *cp;
//	uint8_t param_len;
	uint8_t *adv_data = NULL;
	size_t adv_data_len;
	uint8_t *scan_rsp = NULL;
	size_t scan_rsp_len = -1;
	void *buf = NULL;
	int index = 0;
//	unsigned int mgmt_ret;
//	dbus_int16_t tx_power;

	if (status)
		goto fail;

	if (!param || length < sizeof(*rp)) {
		status = MGMT_STATUS_FAILED;
		goto fail;
	}

	instance = rp->instance;

	bt_ad_add_name(scan, "AD Name");
	adv_data = bt_ad_generate(data, &adv_data_len);
	scan_rsp = bt_ad_generate(scan, &scan_rsp_len);

	printf("adv_data_len = %lu\n", adv_data_len);
	printf("adv_data_len = %lu\n", scan_rsp_len);

	buf = malloc(sizeof(*cp) + adv_data_len + scan_rsp_len);
	if (!buf)
		return;

	memset(buf, 0, sizeof(*cp) + adv_data_len + scan_rsp_len);
	cp = buf;
	cp->instance = instance;
	cp->adv_data_len = adv_data_len;
	cp->scan_rsp_len = scan_rsp_len;
	memcpy(cp->data, adv_data, adv_data_len);
	memcpy(cp->data + adv_data_len, scan_rsp, scan_rsp_len);

	free(adv_data);
	free(scan_rsp);
	adv_data = NULL;
	scan_rsp = NULL;

	mgmt_send(mgmt_if, MGMT_OP_ADD_EXT_ADV_DATA, index,
			sizeof(*cp) + adv_data_len + scan_rsp_len, buf, NULL, NULL, NULL);

fail:
	if (adv_data)
		free(adv_data);

	if (scan_rsp)
		free(scan_rsp);

	if	(buf)
		free(buf);
}

static void add_adv_callback(uint8_t status, uint16_t length,
					  const void *param, void *user_data)
{
//	struct btd_adv_client *client = user_data;
	const struct mgmt_rp_add_advertising *rp = param;

//	client->add_adv_id = 0;

	if (status)
		goto done;

	if (!param || length < sizeof(*rp)) {
		status = MGMT_STATUS_FAILED;
		goto done;
	}

	instance = rp->instance;

done:
	return;
}

static int set_adapter_le_cfg(void)
{
	int index = 0;
	uint8_t val;

	/* LE */
	val = 1;
	if (!mgmt_send(mgmt_if, MGMT_OP_SET_LE, index, sizeof(val), &val,
						auto_power_enable_rsp,
						UINT_TO_PTR(index), NULL)) {
		perror("Unable to send set le cmd");
		return 0;
	}


	/* BREDR */
	val = 0;
	if (!mgmt_send(mgmt_if, MGMT_OP_SET_BREDR, index, sizeof(val), &val,
						auto_power_enable_rsp,
						UINT_TO_PTR(index), NULL)) {
		perror("Unable to send set le cmd");
		return 0;
	}

	/*  Powered */
	val = 1;
	if (!mgmt_send(mgmt_if, MGMT_OP_SET_POWERED, index, sizeof(val), &val,
						auto_power_enable_rsp,
						UINT_TO_PTR(index), NULL)) {
		perror("Unable to send set powerd cmd");
		return 0;
	}


	/* Local Name */
	{
		struct mgmt_cp_set_local_name cp;
		memset(&cp, 0, sizeof(cp));

		strncpy((char *) cp.name, LOCAL_NAME_COMPLETE, HCI_MAX_NAME_LENGTH);
		strncpy((char *) cp.short_name, LOCAL_NAME_SHORT, MGMT_MAX_SHORT_NAME_LENGTH - 1);

		if (mgmt_send(mgmt_if, MGMT_OP_SET_LOCAL_NAME, index, sizeof(cp), &cp,
				name_rsp, NULL, NULL) == 0) {
			perror("Unable to send set_name cmd");
			return 0;
		}
	}

	/* ADV data */
	{
		const uint8_t ad[] = { 0x11, 0x15,
				0xd0, 0x00, 0x2d, 0x12, 0x1e, 0x4b, 0x0f, 0xa4,
				0x99, 0x4e, 0xce, 0xb5, 0x31, 0xf4, 0x05, 0x79 };
		struct mgmt_cp_add_advertising *cp;
		void *buf;

		data = bt_ad_new_with_data(sizeof(ad), ad);
		scan = bt_ad_new();
		uint8_t *adv_data;
		uint8_t *scan_rsp;
		uint32_t flags = MGMT_ADV_FLAG_CONNECTABLE | MGMT_ADV_FLAG_DISCOV | MGMT_ADV_FLAG_TX_POWER | MGMT_ADV_FLAG_LOCAL_NAME;
		size_t adv_data_len = 0;
		size_t scan_rsp_len = 0;


		/* These accept NULL and return NULL */
		bt_ad_add_name(scan, "AD Name");
		adv_data = bt_ad_generate(data, &adv_data_len);
		scan_rsp = bt_ad_generate(scan, &scan_rsp_len);

		printf("adv_data_len = %lu\n", adv_data_len);

		buf = malloc(sizeof(*cp) + adv_data_len + scan_rsp_len);
		if (!buf)
			return 0;

		memset(buf, 0, sizeof(*cp) + adv_data_len + scan_rsp_len);
		cp = buf;
		cp->instance = instance;
		cp->flags = htobl(flags);
		cp->duration = cpu_to_le16(0);
		cp->timeout = cpu_to_le16(0);
		cp->adv_data_len = adv_data_len;
		cp->scan_rsp_len = scan_rsp_len;

		(void) ad;
		memcpy(cp->data, adv_data, adv_data_len);
		memcpy(cp->data + adv_data_len, scan_rsp, scan_rsp_len);
//		(void) adv_data;
//		memcpy(cp->data, ad, adv_data_len);

		free(adv_data);
		free(scan_rsp);

		(void ) add_adv_callback;
//		mgmt_send(mgmt_if, MGMT_OP_ADD_ADVERTISING, index,
//				sizeof(*cp) + adv_data_len + scan_rsp_len, buf, add_adv_callback, NULL, NULL);

		mgmt_send(mgmt_if, MGMT_OP_ADD_ADVERTISING, index,
				sizeof(*cp) + adv_data_len + scan_rsp_len, buf, NULL, NULL, NULL);


		free(buf);
	}

	{
//		struct mgmt_cp_add_ext_adv_params cp;
//		uint32_t flags = 0;
//
//		memset(&cp, 0, sizeof(cp));
//		cp.instance = instance;
////		flags |= MGMT_ADV_PARAM_DURATION;
////		flags |= MGMT_ADV_PARAM_INTERVALS;
////		flags |= MGMT_ADV_PARAM_TX_POWER;
//		flags |= MGMT_ADV_PARAM_SCAN_RSP;
//
//		cp.flags = cpu_to_le32(flags);
//
//		 mgmt_send(mgmt_if, MGMT_OP_ADD_EXT_ADV_PARAMS,
//				 index, sizeof(cp), &cp,
//				 add_adv_params_callback, NULL, NULL);

		(void) add_adv_params_callback;
	}


	/* ADV */
	val = 1;
	if (send_cmd(mgmt_if, MGMT_OP_SET_ADVERTISING, index, sizeof(val), &val,
			setting_rsp) == 0) {
		perror("Unable to send set_advertising cmd");
		return 0;
	}

	/* BONDABLE */
	val = 1;
	if (send_cmd(mgmt_if, MGMT_OP_SET_BONDABLE, index, sizeof(val), &val,
			setting_rsp) == 0) {
		perror("Unable to send set_advertising cmd");
		return 0;
	}

	/* CONNECTABLE */
	val = 1;
	if (send_cmd(mgmt_if, MGMT_OP_SET_CONNECTABLE, index, sizeof(val), &val,
			setting_rsp) == 0) {
		perror("Unable to send set_advertising cmd");
		return 0;
	}

//	/* DISCOVERABLE */
//	{
//		struct mgmt_cp_set_discoverable cp;
//		memset(&cp, 0, sizeof(cp));
//		cp.val = 0x1;
//		cp.timeout = cpu_to_le16(0);
//		if (send_cmd(mgmt_if, MGMT_OP_SET_DISCOVERABLE, index, sizeof(cp), &cp,
//				setting_rsp) == 0) {
//			perror("Unable to send set_advertising cmd");
//			return 0;
//		}
//	}


	return 1;
}

static THREAD_DECLARE(nvt_bluez_gatt_server_task, p_param)
{
	int ret = EXIT_SUCCESS ;
	nvt_bluez_gatt* handle = (nvt_bluez_gatt*)p_param;

	mainloop_init();

	mgmt_if = mgmt_new_default();
	if(mgmt_if == NULL){
		perror("mgmt_if is NULL");
		goto EXIT;
	}

	handle->priv.is_running = 1;
	handle->priv.dev_id = hci_devid(handle->dev_name);

	printf("dev_id = %d\n", handle->priv.dev_id);

	if (handle->priv.dev_id == -1)
		bacpy(&handle->priv.src_addr, BDADDR_ANY);
	else if (hci_devba(handle->priv.dev_id, &handle->priv.src_addr) < 0) {
		perror("Adapter not available");
		ret = EXIT_FAILURE;
		goto EXIT;
	}

	if(!set_adapter_le_cfg()){
		ret = EXIT_FAILURE;
		goto EXIT;
	}

	printf("l2cap_le_att_listen\n", handle->priv.dev_id);

	handle->priv.server_fd = l2cap_le_att_listen(&handle->priv.src_addr, handle->sec, handle->src_type);
	if (handle->priv.server_fd < 0) {
		fprintf(stderr, "Failed to create L2CAP socket\n");
		ret = EXIT_FAILURE;
		goto EXIT;
	}


	if (mainloop_add_fd(handle->priv.server_fd, EPOLLIN, server_accept_callback, handle, NULL) < 0) {
		close(handle->priv.server_fd);
		ret = EXIT_FAILURE;
		goto EXIT;
	}

	printf("Running GATT server\n");

	mainloop_run_with_signal(signal_cb, NULL);

	printf("Exit mainloop\n");

EXIT:

	if(handle->priv.server){
		server_destroy(handle->priv.server);
		free(handle->priv.server);
		handle->priv.server = NULL;
	}

	handle->priv.is_running = 0;

	THREAD_RETURN(ret);
}

int nvt_bluez_gatt_server_start(void)
{
	printf("Start gatt server\n");

	if(!nvt_bluez_gatt_handle.priv.is_running && !nvt_bluez_gatt_handle.priv.task_id){
		nvt_bluez_gatt_handle.priv.task_id = vos_task_create(nvt_bluez_gatt_server_task, (void*) &nvt_bluez_gatt_handle, "nvt_bluez_gatt_svr", 2, 16*1024);
		vos_task_resume(nvt_bluez_gatt_handle.priv.task_id);
	}

    return 0;
}

/* TODO: can't stop server through mainloop_quit when connection exists */
int nvt_bluez_gatt_server_stop(void)
{
	printf("Stop gatt server\n");

	if(nvt_bluez_gatt_handle.priv.task_id){

		mainloop_quit();

		uint16_t cnt = 0, timeout = 10, delay_us = 10000;

		printf("wait exit\n");
		while(cnt ++ < timeout)
		{
			if(nvt_bluez_gatt_handle.priv.is_running){
				usleep(delay_us);
			}
			else{
				printf("server stoped\n");
				break;
			}
		}

		if(cnt >= timeout){
			printf("wait timeout, force kill the task\n");
			vos_task_destroy(nvt_bluez_gatt_handle.priv.task_id);
			nvt_bluez_gatt_handle.priv.is_running = 0;
		}

		printf("task id reset\n");
		nvt_bluez_gatt_handle.priv.task_id = 0;
	}

	return 0;
}

static BOOL Cmd_nvt_bluez_gatt_server_start_gatt_server(unsigned char argc, char **argv)
{

	nvt_bluez_gatt_server_start();
	return TRUE;
}

static BOOL Cmd_nvt_bluez_gatt_server_stop_gatt_server(unsigned char argc, char **argv)
{

	nvt_bluez_gatt_server_stop();
	return TRUE;
}

SXCMD_BEGIN(nvt_bluez_gatt_server_cmd_tbl, "nvt bluez gatt server command")
SXCMD_ITEM("start", Cmd_nvt_bluez_gatt_server_start_gatt_server, "start bluez gatt server")
SXCMD_ITEM("stop", Cmd_nvt_bluez_gatt_server_stop_gatt_server, "stop bluez gatt server")
SXCMD_END()

static int nvt_bluez_gatt_server_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(nvt_bluez_gatt_server_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "nvt_bluez_gatt_server");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", nvt_bluez_gatt_server_cmd_tbl[loop].p_name, nvt_bluez_gatt_server_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(nvt_bluez_gatt_server, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(nvt_bluez_gatt_server_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		nvt_bluez_gatt_server_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], nvt_bluez_gatt_server_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = nvt_bluez_gatt_server_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	nvt_bluez_gatt_server_showhelp(vk_printk);
	return 0;
}
