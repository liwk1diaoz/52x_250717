#ifndef SYS_MEMPOOL_H
#define SYS_MEMPOOL_H

#include "PrjCfg.h"
#include "umsd.h"
#include "msdcnvt/MsdcNvtApi.h"
#include "LogFile.h"

#ifdef __FREERTOS
#define POOL_SIZE_STORAGE_SDIO  ALIGN_CEIL_64(512)
#define POOL_SIZE_STORAGE_NAND  ALIGN_CEIL_32(_EMBMEM_BLK_SIZE_+(_EMBMEM_BLK_SIZE_>>2))
// R/W buf = 0xEC000 (FileSysInfo=32k, OpenFile=2K*N, BitMap=512k, Sectbuf1=128K, SectBuf2=128k, ScanBuf=128k, ResvBuf=8k, Total 944k = 0xEC000)
// FAT buf = 0x80020 (FatBuff=512k + 32bytes reserved = 0x80020)
#else
#define POOL_SIZE_STORAGE_SDIO  (0)
#endif
#ifdef __FREERTOS
#define POOL_SIZE_FILESYS      (ALIGN_CEIL_64(0xEC000)+ALIGN_CEIL_64(0x80020))
#else
#define POOL_SIZE_FILESYS      (ALIGN_CEIL_64(0x4000))  //for linux fs cmd (FSLINUX_STRUCT_xxx)
#endif
#define POOL_SIZE_GFX_TEMP      ALIGN_CEIL_64(4096) //at least 4k for Gxlib
#define POOL_SIZE_FILEDB        ALIGN_CEIL_64(0xAA000)//0x150000)

#if defined(_UI_STYLE_LVGL_)

#if LV_USE_GPU_NVT_DMA2D

#include "lvgl/src/lv_gpu/lv_gpu_nvt_dma2d.h"

	#if LV_USER_CFG_USE_TWO_BUFFER
		#define POOL_SIZE_OSD           (ALIGN_CEIL_64(OSD_SCREEN_SIZE) * 2 + ALIGN_CEIL_64(UI_LAYOUT_SIZE))
	#else
		#define POOL_SIZE_OSD           (ALIGN_CEIL_64(OSD_SCREEN_SIZE) + ALIGN_CEIL_64(UI_LAYOUT_SIZE))
	#endif

	#define POOL_SIZE_LV_TEMP			LV_GPU_NVT_WORKING_BUFFER_SIZE

#else
	#define POOL_SIZE_OSD           ALIGN_CEIL_64(OSD_SCREEN_SIZE)
#endif

#endif

#ifdef __FREERTOS
#define POOL_SIZE_PS_BUFFER    (ALIGN_CEIL_64(0x20000)+ALIGN_CEIL_64(0x7000))  //128k for buf not align,28k for header
#else
#define POOL_SIZE_PS_BUFFER    (0)
#endif
#define POOL_SIZE_GXSOUND      (ALIGN_CEIL_64(0xD000))
#define POOL_SIZE_FDTAPP       (ALIGN_CEIL_64(ALIGN_CEIL(0x40000, _EMBMEM_BLK_SIZE_)))
#define POOL_SIZE_MSDCNVT      (ALIGN_CEIL_64(MSDCNVT_REQUIRE_MIN_SIZE))
#define POOL_SIZE_LOGFILE      (ALIGN_CEIL_64(LOGFILE_BUFFER_SIZE))
#define POOL_SIZE_WAV_PLAY_HEADER	ALIGN_CEIL_32(8*1024)	// 8KB
#define POOL_SIZE_WAV_PLAY_DATA		ALIGN_CEIL_64(380*1024)	// 380KB (300KB for PCM, 80KB for AAC)

#define POOL_SIZE_XML_TEMP_BUF		ALIGN_CEIL_64(0x40100)

#define POOL_SIZE_DISP_TMP			ALIGN_CEIL_64(ALIGN_CEIL_16(1280)*ALIGN_CEIL_16(320)*3/2) // please refer to display size
#define POOL_SIZE_THUMB_FRAME		ALIGN_CEIL_64(640*480*3/2)
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
#define POOL_SIZE_USBCMD       (ALIGN_CEIL_64(64*1024))
#endif
#define POOL_SIZE_DCF_BUFFER    ALIGN_CEIL_32(70*1024)

#define POOL_SIZE_NVTMPP_TEMP       (ALIGN_CEIL_64(LOGO_YUV_BLK_SIZE))

//===========================================================================
//  User defined Mempool IDs
//===========================================================================
extern UINT32 mempool_storage_sdio;
extern UINT32 mempool_storage_nand;
extern UINT32 mempool_filesys;
extern UINT32 mempool_gxgfx_temp;
extern UINT32 mempool_disp_osd1;

#if defined(_UI_STYLE_LVGL_)
extern UINT32 mempool_disp_osd1_pa;
#endif

extern UINT32 mempool_filedb;
extern UINT32 mempool_pstore;
extern UINT32 mempool_gxsound_pa;
extern UINT32 mempool_gxsound_va;
extern UINT32 mempool_fdtapp;
extern UINT32 mempool_msdcnvt;
extern UINT32 mempool_logfile;
extern UINT32 mempool_wavplay_header;
extern UINT32 mempool_wavplay_data;
extern UINT32 mempool_xml_temp;
extern UINT32 mempool_disp_tmp;
#if 1//(MSDCVENDOR_NVT == ENABLE)
extern UINT32 mempool_msdcnvt_pa;
#endif
extern UINT32 mempool_thumb_frame[];
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
extern UINT32 mempool_usbcmd_pa;
extern UINT32 mempool_usbcmd_va;
#endif
extern UINT32 mempool_dcf;

extern UINT32 mempool_nvtmpp_temp_pa;
extern UINT32 mempool_nvtmpp_temp_va;

extern void mempool_init(void);
#endif
