#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>

#if defined(__FREERTOS)
#include "efuse_protected.h"
#endif

BOOL avl_check_available(const CHAR * name)
{
	UINT32 offset = 0;

	if (name[0] == 'l' && name[1] == 'i' && name[2] == 'b') {
		offset = 3;
	}

#if defined(__FREERTOS)
	return efuse_check_available(&(name[offset]));          // skip first 3 characters "lib"
#else
	BOOL ret = FALSE;
	int h_avl;
	char avl_info[29] = "avl ";

	strncat(avl_info, &name[offset], 24);                   // skip first 3 characters "lib"
	if ((h_avl = open("/proc/nvt_info/drvdump/available", O_RDWR)) < 0) {
		DBG_ERR("Please modprobe drvdump first\r\n");
	} else {
		write(h_avl, avl_info, sizeof(avl_info));
		read(h_avl, avl_info, sizeof(avl_info));
		close(h_avl);
		ret = (avl_info[0] == '1') ? TRUE : FALSE;
	}
	return ret;
#endif
}

UINT64 avl_get_uniqueid(void)
{
#if defined(__FREERTOS)
	UINT64 uid = 0;
	UINT32 uid_h = 0, uid_l = 0;
	if (efuse_get_unique_id(&uid_l, &uid_h)!= E_OK) {
		DBG_ERR("read uid fail\r\n");
	} else {
		uid = ((UINT64)uid_h << 32) + (UINT64)uid_l;
	}
	return uid;
#else
	int h_uid;
	UINT64 uid = 0;
	char uid_info[20];

	if ((h_uid = open("/proc/nvt_otp_op/otp_trim", O_RDWR)) < 0) {
		DBG_ERR("read uid fail\r\n");
	} else {
		read(h_uid, uid_info, sizeof(uid_info));
		close(h_uid);
		sscanf(uid_info, "%llx", &uid);
	}
	return uid;
#endif
}

