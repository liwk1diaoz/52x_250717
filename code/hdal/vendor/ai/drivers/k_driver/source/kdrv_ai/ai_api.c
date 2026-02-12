/*#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>*/
//=============================================================
#define __CLASS__ 				"[ai][kdrv][main]"
#include "kdrv_ai_debug.h"
//=============================================================
#include "ai_proc.h"
#include "ai_main.h"
#include "ai_api.h"
#include "nue2_lib.h"
#include "kwrap/mem.h"
#include <kwrap/file.h>
#include "cnn/cnn_platform.h"
#include "nue/nue_platform.h"
#include "nue2/nue2_platform.h"
#include "kdrv_ai_version.h"

#if defined(__FREERTOS)
#include <stdlib.h>
#include <string.h>
#endif

#if KDRV_AI_SINGLE_LAYER_CYCLE
#define MAX_LAYER_CYCLE_NUM 512
UINT32 g_layer_cycle[MAX_LAYER_CYCLE_NUM] = {0};
UINT32 g_layer_eng[MAX_LAYER_CYCLE_NUM] = {0};
UINT32 g_layer_cnt = 0;
UINT32 g_layer_exceed = 0;
UINT32 g_layer_start_idx = 0;
UINT32 g_single_layer_en = 0;
#endif

static int kdrv_ai_dump_layer_cycle(VOID)
{
	UINT32 i = 0;
	UINT32 dump_layer_num = (g_layer_cnt > MAX_LAYER_CYCLE_NUM) ? MAX_LAYER_CYCLE_NUM: g_layer_cnt;
	
	if (g_layer_exceed == 1) {
		DBG_DUMP("Warning: total running layer number is exceed the max number of cycle array size\n");
		DBG_DUMP("Warning: only last %d layer cycles are preserved!\n", MAX_LAYER_CYCLE_NUM);
	}
	
	DBG_DUMP("=====================================\n");
	DBG_DUMP("single layer cycle: (engine-> 0:CNN1, 1:NUE, 2:NUE2, 3:CNN2)\n");
	for (i = 0; i < dump_layer_num; i++) {
		UINT32 tmp_idx = (g_layer_start_idx + i) % MAX_LAYER_CYCLE_NUM;
		DBG_DUMP("layer %3d engine %d cycle = %d\n", (unsigned int)(g_layer_start_idx + i), (unsigned int)g_layer_eng[tmp_idx], (unsigned int)g_layer_cycle[tmp_idx]);
	}
	DBG_DUMP("=====================================\n");
	
	g_layer_exceed = 0;
	g_layer_start_idx = 0;
	g_layer_cnt = 0;

	return 0;
}

#if !defined(CONFIG_NVT_SMALL_HDAL)
int nvt_ai_api_read_reg(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;

	if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -1;
	}

	if (pargv == NULL) {
		DBG_ERR("invalid reg = null\n");
		return -1;
	}
#if defined(__FREERTOS)
	if ((reg_addr = strtoul(pargv[0], NULL, 16)) == 0) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -1;
	}
#else
	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -1;
	}
#endif

	nvt_ai_drv_read_reg(pmodule_info, reg_addr);

	return 0;
}
#endif

int nvt_ai_api_read_cycle(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	UINT64 cnn_cycle = 0;
	UINT64 nue_cycle = 0;
	UINT64 nue2_cycle = 0;
	UINT32 debug_layer = 0;
#if defined(__FREERTOS)
	unsigned long tmp_long;
#else
#endif
    if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -1;
	}
	if (pargv == NULL) {
		DBG_ERR("invalid command = null\n");
		return -1;
	}

#if defined(__FREERTOS)
	tmp_long = strtoul(pargv[0], NULL, 10);
    debug_layer = (UINT32) tmp_long;
#else
	if (kstrtouint (pargv[0], 10, &debug_layer)) {
		DBG_ERR("invalid debug_layer value:%s\n", pargv[0]);
		return -1;
	}
#endif

	//DBG_DUMP("R REG 0x%lx\n", (int)reg_addr);
	cnn_cycle  = cnn_dump_total_cycle(debug_layer);
	nue_cycle  = nue_dump_total_cycle(debug_layer);
    nue2_cycle = nue2_dump_total_cycle(debug_layer);
    DBG_DUMP("cnn_total_cycle  :  %llu\n", cnn_cycle);
    DBG_DUMP("nue_total_cycle  :  %llu\n", nue_cycle);
    DBG_DUMP("nue2_total_cycle :  %llu\n", nue2_cycle);
#if KDRV_AI_SINGLE_LAYER_CYCLE
	if (g_single_layer_en) {
		kdrv_ai_dump_layer_cycle();
	}
#endif

	if (cnn_cycle == 0) cnn_cycle = 0;
	if (nue_cycle == 0) nue_cycle = 0;
	if (nue2_cycle == 0) nue2_cycle = 0;

	//DBG_ERR("REG 0x%lx = 0x%lx\n", (long unsigned)reg_addr, (long unsigned)value);
	return 0;
}

int nvt_ai_api_read_version(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{

	CHAR* kdrv_ai_version;
	kdrv_ai_version = KDRV_AI_IMPL_VERSION;
	DBG_DUMP("kdrv_ai_version:%s\n", kdrv_ai_version);

	return 0;
}

int nvt_kdrv_ai_reset(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBG_DUMP("kdrv_ai reset\n");
	kdrv_ai_engine_reset(AI_ENG_NUE2);
	kdrv_ai_engine_reset(AI_ENG_CNN);
	kdrv_ai_engine_reset(AI_ENG_CNN2);
	kdrv_ai_engine_reset(AI_ENG_NUE);
	

	return 0;
}

#if !defined(CONFIG_NVT_SMALL_HDAL)
int nvt_ai_api_write_reg(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		DBG_ERR("wrong argument:%d", argc);
		return -1;
	}
	if (pargv == NULL) {
		DBG_ERR("invalid reg = null\n");
		return -1;
	}

#if defined(__FREERTOS)
	if ((reg_addr = strtoul(pargv[0], NULL, 16)) == 0) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -1;
	}
	if ((reg_value = strtoul(pargv[1], NULL, 16)) == 0) {
		DBG_ERR("invalid reg value:%s\n", pargv[1]);
		return -1;

	}
#else
	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -1;
	}
	if (kstrtoul(pargv[1], 0, &reg_value)) {
		DBG_ERR("invalid reg value:%s\n", pargv[1]);
		return -1;

	}
#endif

	DBG_IND("W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_ai_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_ai_api_write_pattern(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int fd;
	int len = 0;
	//unsigned char *pbuffer;
	struct vos_mem_cma_info_t buf_info = {0};
	int ret = 0;
	VOS_MEM_CMA_HDL buf_info_id;

	if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -1;
	}

	if (pargv == NULL) {
		DBG_ERR("invalid reg = null\n");
		return -1;
	}
	
	fd = vos_file_open(pargv[0], O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		DBG_ERR("failed in file open:%s\r\n", pargv[0]);
		return -1;
	}

	//Allocate memory
	if (0 != vos_mem_init_cma_info(&buf_info, VOS_MEM_CMA_TYPE_CACHE, 0x600000)) {
        DBG_ERR("vos_mem_init_cma_info: init buffer fail. \r\n");
		vos_file_close(fd);
        return -1;
    } else {
        buf_info_id = vos_mem_alloc_from_cma(&buf_info);
		if (NULL == buf_info_id) {
            DBG_ERR("get buffer fail\n");
			DBG_ERR("get buffer fail\n");
			vos_file_close(fd);
            return -1;
        }
    }

	len = vos_file_read(fd, (void *)buf_info.vaddr, 1152 * 64);
	/* Do something after get data from file */
	ret = vos_mem_release_from_cma(buf_info_id);
    if (ret != 0) {
        DBG_ERR("failed in release buffer\n");
		vos_file_close(fd);
        return -1;
    }

	vos_file_close(fd);

	return len;
}

int nvt_kdrv_ai_api_test(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	if (pargv == NULL) {
		DBG_ERR("invalid reg = null\n");
		return -1;
	}
	return 0;
}

#if (NUE2_SYS_VFY_EN == DISABLE)
int nvt_kdrv_ai_module_test(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBG_ERR("NUE2: Error, please set #define NUE2_AI_FLOW DISABLE in nue2_platform.h\r\n");
	return 0;
}
#endif
#endif

#if KDRV_AI_SINGLE_LAYER_CYCLE
int nvt_kdrv_ai_set_layer_cycle(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
{
	UINT32 debug_layer_en = 0;
#if defined(__FREERTOS)
	unsigned long tmp_long;
#else
#endif
    if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -1;
	}
	if (pargv == NULL) {
		DBG_ERR("invalid command = null\n");
		return -1;
	}

#if defined(__FREERTOS)
	tmp_long = strtoul(pargv[0], NULL, 10);
    debug_layer_en = (UINT32) tmp_long;
#else
	if (kstrtouint (pargv[0], 10, &debug_layer_en)) {
		DBG_ERR("invalid debug_layer_en value:%s\n", pargv[0]);
		return -1;
	}
#endif
	
	g_single_layer_en = debug_layer_en;
	
	return 0;
}

int kdrv_ai_set_layer_cycle(KDRV_AI_ENG eng, UINT32 cycle)
{
	UINT32 tmp_idx = g_layer_cnt % MAX_LAYER_CYCLE_NUM;
	
	if (g_single_layer_en) {
		g_layer_cycle[tmp_idx] = cycle;
		g_layer_eng[tmp_idx]   = eng;
		g_layer_cnt++;
		if (g_layer_cnt > MAX_LAYER_CYCLE_NUM) {
			g_layer_exceed = 1;	
			g_layer_start_idx = tmp_idx + 1;
		}
	}
	
	return 0;
}

#endif

