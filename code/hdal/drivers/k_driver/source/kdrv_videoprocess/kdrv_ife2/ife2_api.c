#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <comm/nvtmem.h>


#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>



#include "ife2_api.h"
#include "ife2_drv.h"
#include "ife2_dbg.h"
//#include "ife2_lib.h"

#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ife2.h"


//-------------------------------------------------
#if (__IFE2_DBG_CMD__ == 1)

int nvt_ife2_api_write_reg(PIFE2_INFO pife2_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", (int)argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%x to 0x%x\n", (unsigned int)reg_value, (unsigned int)reg_addr);

	nvt_ife2_drv_write_reg(pife2_info, reg_addr, reg_value);
	return 0;
}


int nvt_ife2_api_write_pattern(PIFE2_INFO pife2_info, unsigned char argc, char **pargv)
{
	return 0;
}

int nvt_ife2_api_read_reg(PIFE2_INFO pife2_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", (int)argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_ife2_drv_read_reg(pife2_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%x = 0x%x\n", (unsigned int)reg_addr, (unsigned int)value);
	return 0;
}
//------------------------------------------------------------
#endif



