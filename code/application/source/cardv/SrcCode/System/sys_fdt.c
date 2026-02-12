#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libfdt.h>
#include <strg_def.h>
//#include "sys_storage_partition.h"
#include "sys_mempool.h"
#include "PrjCfg.h"
#include "sys_fdt.h"
#include <kwrap/debug.h>

void *fdt_get_app(void)
{
	static BOOL inited = FALSE;
	if (inited == TRUE) {
		return (void *)mempool_fdtapp;
	}

#if defined(__FREERTOS)
	ER er;
	STORAGE_OBJ* pStrg = EMB_GETSTRGOBJ(STRG_OBJ_FW_APP);
	if (pStrg == NULL) {
		DBG_ERR("pStrg is NULL.\n");
		return NULL;
	}

	pStrg->Open();
	pStrg->Lock();
	er = pStrg->RdSectors((INT8*)mempool_fdtapp, 0, 1);
	pStrg->Unlock();
	if (er != 0) {
		return NULL;
	}

	if ((er = fdt_check_header((void*)mempool_fdtapp)) != 0) {
		DBG_ERR("invalid fdt-app header, addr=0x%08X er = %d \n", (unsigned int)mempool_fdtapp, er);
		pStrg->Close();
		return NULL;
	}

	int fdtapp_size = fdt_totalsize((void*)mempool_fdtapp);
	if (fdtapp_size > POOL_SIZE_FDTAPP) {
		DBG_ERR("POOL_SIZE_FDTAPP is too small, require:0x%08X\n", fdtapp_size);
		return NULL;
	}

	if (fdtapp_size > _EMBMEM_BLK_SIZE_) {
		int remain_blocks = ALIGN_CEIL(fdtapp_size - _EMBMEM_BLK_SIZE_, _EMBMEM_BLK_SIZE_) / _EMBMEM_BLK_SIZE_;
		pStrg->Lock();
		er = pStrg->RdSectors((INT8*)(mempool_fdtapp + _EMBMEM_BLK_SIZE_), 1, remain_blocks);
		pStrg->Unlock();
		if (er != 0) {
			pStrg->Close();
			return NULL;
		}
	}

	pStrg->Close();
#else
	char path[] = "/mnt/app/application.dtb";

	struct stat filestat;
	if (stat(path, &filestat) == -1) {
		DBG_ERR("cannot find %s.\n", path);
		return NULL;
	}

	if (filestat.st_size > POOL_SIZE_FDTAPP) {
		DBG_ERR("dtb size %d bytes larger than POOL_SIZE_FDTAPP %d bytes.\n", filestat.st_size, POOL_SIZE_FDTAPP);
		return NULL;
	}

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		DBG_ERR("failed to open %s.\n", path);
		return NULL;
	}
	if (read(fd, (void *)mempool_fdtapp, filestat.st_size) != filestat.st_size) {
		DBG_ERR("failed to read %s.\n", path);
		close(fd);
		return NULL;
	}
	close(fd);
#endif
	inited = TRUE;
	return (void *)mempool_fdtapp;
}

void *fdt_get_sensor(void)
{
	int er;
	extern unsigned char _fdt_sensor[];

	if ((er = fdt_check_header((void*)_fdt_sensor)) != 0) {
		DBG_ERR("invalid fdt-sensor header, addr=0x%08X er = %d \n", (unsigned int)_fdt_sensor, er);
		return NULL;
	}
	return (void *)_fdt_sensor;
}