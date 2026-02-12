/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_util.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_type.h"
#include "hd_logger.h"
#include "hd_util.h"
#define HD_MODULE_NAME HD_UTIL
#include "hd_int.h"
#include <string.h>
#include <sys/time.h>
#include <kwrap/examsys.h>

#define HD_KEYINPUT_SIZE 80

UINT32 hd_read_decimal_key_input(const CHAR* comment)
{
	char    cmd[HD_KEYINPUT_SIZE];
	int     radix = 0;
	char    *ret = NULL;

	DBG_DUMP("%s: ", comment);
	fflush(stdin);
	do {
		ret = NVT_EXAMSYS_FGETS(cmd, sizeof(cmd), stdin);
	} while (NULL == ret || cmd[0] == ' ' || cmd[0] == '\n') ;

	if (!strncmp(cmd, "0x", 2)) {
		radix = 16;
	} else {
		radix = 10;
	}

	return (strtoul(cmd, (char **) NULL, radix));
}

UINT32 hd_gettime_ms(VOID)
{
	ISF_FLOW_IOCTL_GET_TIMESTAMP msg;
	int   ret;

	if (dev_fd < 0) {
		DBG_ERR("hd_common not init\r\n");
		return 0;
	}
	ret = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_TIMESTAMP, &msg);
    if (ret < 0) {
        return 0;
    }
	return (UINT32)(msg.timestamp/1000);
}

UINT64 hd_gettime_us(VOID)
{
	ISF_FLOW_IOCTL_GET_TIMESTAMP msg;
	int   ret;

	if (dev_fd < 0) {
		DBG_ERR("hd_common not init\r\n");
		return 0;
	}
	ret = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_TIMESTAMP, &msg);
    if (ret < 0) {
        return 0;
    }
	return msg.timestamp;
}

