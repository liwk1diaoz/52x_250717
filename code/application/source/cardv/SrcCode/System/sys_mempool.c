#include <hdal.h>
#include "sys_mempool.h"
#include "UI/UIGraphics.h"
#include "vendor_common.h"
#include "PrjCfg.h"
UINT32 mempool_storage_sdio =0;
#if defined(__FREERTOS)
UINT32 mempool_storage_nand;
UINT32 mempool_pstore;
#endif
UINT32 mempool_filesys;
UINT32 mempool_gxgfx_temp;
UINT32 mempool_disp_osd1;

#if defined(_UI_STYLE_LVGL_)

UINT32 mempool_disp_osd1_pa;
UINT32 mempool_lv_temp;

#endif

UINT32 mempool_filedb;
UINT32 mempool_gxsound_pa;
UINT32 mempool_gxsound_va;
UINT32 mempool_fdtapp;
//#if (MSDCVENDOR_NVT == ENABLE)
UINT32 mempool_msdcnvt;
UINT32 mempool_msdcnvt_pa;
//#endif
#if (LOGFILE_FUNC==ENABLE)
UINT32 mempool_logfile;
#endif
#if (WAV_PLAY_FUNC == ENABLE)
UINT32 mempool_wavplay_header;
UINT32 mempool_wavplay_data;
#endif
#if (PLAY_THUMB_AND_MOVIE == ENABLE)
#include "UIWnd/UIFlow.h"
//#include "UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowPlayFuncs.h"
UINT32 mempool_disp_tmp;
UINT32 mempool_thumb_frame[FLOWPB_THUMB_H_NUM2*FLOWPB_THUMB_V_NUM2];
#endif
UINT32 mempool_xml_temp;
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
UINT32 mempool_usbcmd_pa;
UINT32 mempool_usbcmd_va;
#endif
#if (USE_DCF == ENABLE)
UINT32 mempool_dcf;
#endif


UINT32 mempool_nvtmpp_temp_pa=0;
UINT32 mempool_nvtmpp_temp_va=0;

void mempool_init(void)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret;
#if defined(__FREERTOS)
	ret = vendor_common_mem_alloc_fixed_pool("sdio", &pa, (void **)&va, POOL_SIZE_STORAGE_SDIO, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_storage_sdio = (UINT32)va;
	ret = vendor_common_mem_alloc_fixed_pool("nand", &pa, (void **)&va, POOL_SIZE_STORAGE_NAND, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_storage_nand = (UINT32)va;
#endif
	#if (FS_MULTI_STRG_FUNC == ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("filesys", &pa, (void **)&va, (POOL_SIZE_FILESYS*2), DDR_ID0);
	#else
	ret = vendor_common_mem_alloc_fixed_pool("filesys", &pa, (void **)&va, POOL_SIZE_FILESYS, DDR_ID0);
	#endif
	if (ret != HD_OK) {
		return;
	}
	mempool_filesys = (UINT32)va;
	ret = vendor_common_mem_alloc_fixed_pool("gxgfx_temp", &pa, (void **)&va, POOL_SIZE_GFX_TEMP, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_gxgfx_temp = (UINT32)va;

	#if defined(_UI_STYLE_LVGL_)
	ret = vendor_common_mem_alloc_fixed_pool("disp_osd1", &pa, (void **)&va, POOL_SIZE_OSD, DDR_ID0); //for ping-pong
	if (ret != HD_OK) {
		return;
	}
	mempool_disp_osd1_pa = (UINT32)pa;
	mempool_disp_osd1 = (UINT32)va;

	ret = vendor_common_mem_alloc_fixed_pool("lv_temp", &pa, (void **)&va, POOL_SIZE_LV_TEMP, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_lv_temp = (UINT32)va;

	#else
	ret = vendor_common_mem_alloc_fixed_pool("disp_osd1", &pa, (void **)&va, UI_GetOSDSize()*3, DDR_ID0); //for ping-pong
	if (ret != HD_OK) {
		return;
	}
    mempool_disp_osd1 = (UINT32)va;
    #endif

	ret = vendor_common_mem_alloc_fixed_pool("filedb", &pa, (void **)&va, POOL_SIZE_FILEDB, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
    mempool_filedb = (UINT32)va;
#if defined(__FREERTOS)
	ret = vendor_common_mem_alloc_fixed_pool("pstore", &pa, (void **)&va, POOL_SIZE_PS_BUFFER, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
    mempool_pstore = (UINT32)va;
#endif
	ret = vendor_common_mem_alloc_fixed_pool("gxsound", &pa, (void **)&va, POOL_SIZE_GXSOUND, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_gxsound_va = (UINT32)va;
	mempool_gxsound_pa = (UINT32)pa;
	ret = vendor_common_mem_alloc_fixed_pool("fdtapp", &pa, (void **)&va, POOL_SIZE_FDTAPP, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_fdtapp = (UINT32)va;
#if (MSDCVENDOR_NVT == ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("msdcnvt", &pa, (void **)&va, POOL_SIZE_MSDCNVT, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_msdcnvt = (UINT32)va;
	mempool_msdcnvt_pa = (UINT32)pa;
#endif
#if (LOGFILE_FUNC==ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("logfile", &pa, (void **)&va, POOL_SIZE_LOGFILE, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_logfile = (UINT32)va;
#endif
#if (WAV_PLAY_FUNC == ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("wavp_header", &pa, (void **)&va, POOL_SIZE_WAV_PLAY_HEADER, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_wavplay_header = (UINT32)va;

	ret = vendor_common_mem_alloc_fixed_pool("wavp_data", &pa, (void **)&va, POOL_SIZE_WAV_PLAY_DATA, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_wavplay_data = (UINT32)va;
#endif
	ret = vendor_common_mem_alloc_fixed_pool("xmltmp", &pa, (void **)&va, POOL_SIZE_XML_TEMP_BUF, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_xml_temp = (UINT32)va;
#if (PLAY_THUMB_AND_MOVIE == ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("disptmp", &pa, (void **)&va, POOL_SIZE_DISP_TMP, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_disp_tmp = (UINT32)pa;

	{
		UINT32 i;
		CHAR *name = "thumb00";
		for (i = 0; i < FLOWPB_THUMB_H_NUM2*FLOWPB_THUMB_V_NUM2; i++) {
			snprintf((char *)name, sizeof("thumb00"), "thumb%02d", i);
			ret = vendor_common_mem_alloc_fixed_pool(name, &pa, (void **)&va, POOL_SIZE_THUMB_FRAME, DDR_ID0);
			if (ret != HD_OK) {
				return;
			}
			mempool_thumb_frame[i] = (UINT32)pa;
		}
	}
#endif
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("usbcmd", &pa, (void **)&va, POOL_SIZE_USBCMD, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_usbcmd_pa = (UINT32)pa;
	mempool_usbcmd_va = (UINT32)va;
#endif
#if (USE_DCF == ENABLE)
	ret = vendor_common_mem_alloc_fixed_pool("dcf", &pa, (void **)&va, POOL_SIZE_DCF_BUFFER, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
    mempool_dcf = (UINT32)va;
#endif

	if(1){//System_IsBootFromRtos()==TRUE){
		ret = vendor_common_mem_alloc_fixed_pool("NVTMPP_TEMP", &pa, (void **)&va,  POOL_SIZE_NVTMPP_TEMP, DDR_ID0);
		if (ret != HD_OK) {
			return;
		}
		mempool_nvtmpp_temp_pa = (UINT32)pa;
		mempool_nvtmpp_temp_va = (UINT32)va;
	}
}

