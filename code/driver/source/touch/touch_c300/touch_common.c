
#ifdef __KERNEL__
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#include <kwrap/file.h>
#else
#include <stdio.h>
#endif

#include "kwrap/type.h"
//#include "touch_cfg.h"
#include "touch_dtsi.h"
#include "touch_common.h"



//=============================================================================
// dtsi file
//=============================================================================


#if 1//touch_dtsi_FROM_FILE
static TOUCH_DTSI_FILE gTOUCH_DTSI_FILE[TOUCH_DTSI_ITEM_MAX_NUM] = {
	{ "i2c_cfg",      "id",            	NULL, sizeof(UINT32)              },    // 0
	{ "i2c_cfg",      "addr",           NULL, sizeof(UINT32)              },

	{ "gpio_cfg",     "pwr_pin",        NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "rst_pin",        NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "int_pin",       	NULL, sizeof(UINT32)              },// 4


	{ "rst_cfg",   	  "rst_time",      	NULL, sizeof(UINT32)              },
};

static UINT32 touch_big_little_swap(UINT8 *pvalue)
{
	UINT32 tmp;

	tmp = (pvalue[0] << 24) | (pvalue[1] << 16) | (pvalue[2] << 8) | pvalue[3];

	return tmp;
}

ER touch_common_load_dtsi_file(UINT8 *node_path, UINT8 *file_path, UINT8 *buf_addr, void *param)
{
	CHAR node_name[64];
	UINT8 *pfdt_addr;
	INT32 i, node_ofst = 0, data_size;
	//UINT32 move_spd;
	const void *pfdt_node;
	ER rt = E_OK;

	TOUCH_DRV_INFO *ptouch_info = (TOUCH_DRV_INFO *)param;

#ifdef __KERNEL__
	UINT32 read_size = 2*1024;
	VOS_FILE fp;

	pfdt_addr = kzalloc(read_size, GFP_KERNEL);
	if (pfdt_addr == NULL) {
		DBG_WRN("fail to allocate pfdt_addr!\r\n");

		return E_SYS;
	}

	fp = vos_file_open((CHAR *)file_path, O_RDONLY, 0x777);
	if (fp == (VOS_FILE)(-1)) {
		kfree(pfdt_addr);
		pfdt_addr = NULL;
		DBG_WRN("open %s fail!\r\n", file_path);

		return -E_SYS;
	}

	if (vos_file_read(fp, pfdt_addr, read_size) < 1) {
		vos_file_close(fp);
		kfree(pfdt_addr);
		pfdt_addr = NULL;
		DBG_WRN("read %s fail!\r\n", file_path);

		return -E_SYS;
	}

	vos_file_close(fp);
#else
	pfdt_addr = buf_addr;
	if (pfdt_addr == NULL) {
		DBG_WRN("pfdt_addr is NULL!\r\n");

		return E_SYS;
	}
#endif
	DBG_WRN("*** node_path = %s \r\n",node_path);
	node_ofst = fdt_path_offset(pfdt_addr, (CHAR *)node_path);
	if (node_ofst < 0) {
#ifdef __KERNEL__
		kfree(pfdt_addr);
		pfdt_addr = NULL;
#endif
		DBG_WRN("%s not available!\r\n", node_path);

		return E_SYS;
	}

	for (i = 0; i < TOUCH_DTSI_ITEM_MAX_NUM; i++) {
		//DBG_WRN("***  gTOUCH_DTSI_FILE[%d].node_name = %s \r\n",i, gTOUCH_DTSI_FILE[i].node_name);
		sprintf(node_name, "%s/%s", node_path, gTOUCH_DTSI_FILE[i].node_name);
		node_ofst = fdt_path_offset(pfdt_addr, node_name);

		if (node_ofst >= 0) {
			//DBG_WRN("gTOUCH_DTSI_FILE[%d].data_name = %s \r\n",i,gTOUCH_DTSI_FILE[i].data_name);
			//DBG_WRN("data_size = %d \r\n",data_size);
			//DBG_WRN("gTOUCH_DTSI_FILE[%d].size = %d \r\n",i,gTOUCH_DTSI_FILE[i].size);
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, (CHAR *)gTOUCH_DTSI_FILE[i].data_name, (int *)&data_size);
			if ((pfdt_node != NULL) && (data_size == gTOUCH_DTSI_FILE[i].size)) {
				gTOUCH_DTSI_FILE[i].pdata = (UINT8 *)pfdt_node;
			} else {
				gTOUCH_DTSI_FILE[i].pdata = NULL;
				DBG_WRN("%s size not match (%d) (%d)!\r\n", node_name, data_size, gTOUCH_DTSI_FILE[i].size);
			}
		} else {
			gTOUCH_DTSI_FILE[i].pdata = NULL;
			DBG_WRN("%s not available!\r\n", node_name);
		}
		#if 0//for debug
		if(i < 5)
		{
			if (gTOUCH_DTSI_FILE[0].pdata != NULL)
			{
				r_rslt = touch_big_little_swap(gTOUCH_DTSI_FILE[i].pdata);
				DBG_WRN("r_rslt = %d \r\n",r_rslt);
			}
		}
		rt = (int)r_rslt;
		#endif
	}
	if (gTOUCH_DTSI_FILE[0].pdata != NULL) {
		ptouch_info->tp_hw.i2c_cfg.i2c_id = touch_big_little_swap(gTOUCH_DTSI_FILE[0].pdata);
		DBG_WRN("i2c_id = %d \r\n",ptouch_info->tp_hw.i2c_cfg.i2c_id);
	}
	if (gTOUCH_DTSI_FILE[1].pdata != NULL) {
		ptouch_info->tp_hw.i2c_cfg.i2c_addr = touch_big_little_swap(gTOUCH_DTSI_FILE[1].pdata);
		DBG_WRN("i2c_addr = 0x%x \r\n",ptouch_info->tp_hw.i2c_cfg.i2c_addr);
	}

	if (gTOUCH_DTSI_FILE[2].pdata != NULL) {
		ptouch_info->tp_hw.gpio_cfg.gpio_pwr = touch_big_little_swap(gTOUCH_DTSI_FILE[2].pdata);
		DBG_WRN("gpio_pwr = %d \r\n",ptouch_info->tp_hw.gpio_cfg.gpio_pwr);
	}
	if (gTOUCH_DTSI_FILE[3].pdata != NULL) {
		ptouch_info->tp_hw.gpio_cfg.gpio_rst = touch_big_little_swap(gTOUCH_DTSI_FILE[3].pdata);
		DBG_WRN("gpio_rst = %d \r\n",ptouch_info->tp_hw.gpio_cfg.gpio_rst);
	}
	if (gTOUCH_DTSI_FILE[4].pdata != NULL) {
		ptouch_info->tp_hw.gpio_cfg.gpio_int = touch_big_little_swap(gTOUCH_DTSI_FILE[4].pdata);
		DBG_WRN("gpio_int = %d \r\n",ptouch_info->tp_hw.gpio_cfg.gpio_int);
	}
	if (gTOUCH_DTSI_FILE[5].pdata != NULL) {
		ptouch_info->tp_hw.reset_time.rst_time = touch_big_little_swap(gTOUCH_DTSI_FILE[5].pdata);
	}

#ifdef __KERNEL__
	kfree(pfdt_addr);
	pfdt_addr = NULL;
#endif

	return rt;
}
#endif
