/*
    System Storage Callback

    System Callback for Storage Module.

    @file       SysStrg_Exe.c
    @ingroup    mIPRJSYS

    @note       �o���ɮ׭t�d�@���

                1.����Storage Event������
                  �i�઺event��:

                     STRG_INSERT --- �N���CARD���J
                       ����unmount���e�ݭn�����Ʊ�
                       �I�sFile unmound (NAND),
                       ����mount���e�ݭn�����Ʊ�
                       �I�sFile mount (CARD)

                     STRG_REMOVE --- �N���CARD�ޥX
                       ����unmount���e�ݭn�����Ʊ�
                       �I�sFile unmount (CARD)
                       ����mount���e�ݭn�����Ʊ�
                       �I�sFile_mount (NAND)

                     STRG_ATTACH --- �N��File mount����
                       ����mount����ݭn�����Ʊ�
                       �o��|�ǤJmount�����Gstatus

                     STRG_DETACH --- �N��File unmount����
                       ����unmount����ݭn�����Ʊ�
                       �o��|�ǤJunmount�����Gstatus


    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <strg_def.h>
#include <libfdt.h>
#include <compiler.h>
#include <rtosfdt.h>
#include "PrjInc.h"
#include "PrjCfg.h"
//#include "sys_storage_partition.h"
////////////////////////////////////////////////////////////////////////////////
#include "UIFrameworkExt.h"
#include "UICommon.h"
//#include "dma.h"
#include "PStore.h"
#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif
//#include "ExifVendor.h"
#include "FileSysTsk.h"
#include "SysCommon.h"
#include "sys_mempool.h"
#include "DxType.h"
#include "Dx.h"
#include "DxApi.h"
#include "DxStorage.h"
#include <comm/shm_info.h>
#include "FwSrvApi.h"
#include <sys/stat.h>
#include <sys/mount.h>
#include "GxStrg.h"
#include "sdio.h"
#include <mntent.h>
#include <string.h>
#include "emmc.h"

#if (LOGFILE_FUNC==ENABLE)
#include "LogFile.h"
#endif
#if (USERLOG_FUNC == ENABLE)
#include "userlog.h"
#endif
//#include "wdt.h"

#if defined(_EMBMEM_SPI_NOR_)
#define MAX_BLK_PER_SEC         128
#define MAX_SEC_NUM             8
#else
#define MAX_BLK_PER_SEC         512
#define MAX_SEC_NUM             64
#endif
#define LDC_HEADER_SIZE         16
#define FW_UPD_FW_TMP_MEM_SIZE  0xA00000

#define LOADER_UPD_FW_PATH "A:\\"_BIN_NAME_".BIN"
#define FW_DEL_INDIACTION_PATH "A:\\NVTDELFW"

//#NT#2018/04/02#Niven Cho -begin
//#NT#PARTIAL_COMPRESS, we use last 10MB of APP as working buffer
#define FW_PARTIAL_COMPRESS_WORK_BUF_SIZE 0xA00000
//#NT#2018/04/02#Niven Cho -end

//global debug level: PRJ_DBG_LVL
#include "PrjCfg.h"
//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysStrgExe
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
//Check definition options
#if defined(_CPU2_ECOS_)
#if (LOGFILE_FUNC == ENABLE && ECOS_DBG_MSG_FORCE_UART1_DIRECT == ENABLE)
//#error "LOGFILE_FUNC must turn off ECOS_DBG_MSG_FORCE_UART1_DIRECT"
#endif
#endif

#if 0
#define FW_UPDATE_FILE_LEN 16
static char uiUpdateFWName[FW_UPDATE_FILE_LEN] = FW_UPDATE_NAME;
static BOOL m_bUpdRename = FALSE;
#endif

#if (FS_FUNC == ENABLE)
static FST_FS_TYPE m_GxStrgType = FST_FS_TYPE_UITRON;
#endif
#if (FWS_FUNC == ENABLE)
static void *mp_fwsrv_work_buf = NULL;
#endif
///////////////////////////////////////////////////////////////////////////////
//
//  EMBMEM
//
///////////////////////////////////////////////////////////////////////////////
//Caculate FAT Start Addr
#if defined(_CPU2_LINUX_)
static BOOL xSysStrg_LinuxRun(void);
#endif
#if defined(_CPU2_ECOS_)
static BOOL xSysStrg_eCosRun(void);
#endif
#if (defined(_DSP1_FREERTOS_) || defined(_DSP2_FREERTOS_))
static BOOL xSysStrg_DspRun(DSP_CORE_ID CoreID);
#endif

#if (FWS_FUNC == ENABLE)
UINT32 System_OnStrgInit_EMBMEM_GetGxStrgType(void)
{
	return m_GxStrgType;
}

void System_OnStrgInit_EMBMEM(void)
{
#if defined(_EMBMEM_SPI_NOR_)
	{
		//if stoarge is SPI, use ram disk as internal FAT
		DXSTRG_INIT UserEmbMemInit = {0};
		UserEmbMemInit.prt.uiDxClassType = DX_CLASS_STORAGE_EXT | USER_DX_TYPE_EMBMEM_FAT;
		UserEmbMemInit.buf.Addr = OS_GetMempoolAddr(POOL_ID_STORAGE_RAMDISK);
		UserEmbMemInit.buf.Size = OS_GetMempoolSize(POOL_ID_STORAGE_RAMDISK);
		DX_HANDLE DxNandDev = Dx_GetObject(UserEmbMemInit.prt.uiDxClassType);
		Dx_Init(DxNandDev, &UserEmbMemInit, 0, STORAGE_VER);
	}
#endif
}

void System_OnStrgExit_EMBMEM(void)
{
	//PHASE-1 : Close Drv or DrvExt
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  EXMEM
//
///////////////////////////////////////////////////////////////////////////////
//Drv or DrvExt
#if (FS_FUNC == ENABLE)
DXSTRG_INIT UserSdioInit;
#if(COPYCARD2CARD_FUNCTION == ENABLE)
DXSTRG_INIT UserSdio2Init;
#endif
DX_HANDLE DxCardDev1 = 0;
typedef enum _BOOT_CARD_STATE {
	BOOT_CARD_STATE_UNKNOWN,
	BOOT_CARD_STATE_INSERTED,
	BOOT_CARD_STATE_REMOVED,
	ENUM_DUMMY4WORD(BOOT_CARD_STATE)
} BOOT_CARD_STATE;
static BOOT_CARD_STATE m_BootState_Drive[16] = {BOOT_CARD_STATE_UNKNOWN}; //DriveA, DriveB
static UINT32 m_FsDxTypeMap[2] = {FS_DX_TYPE_DRIVE_A, FS_DX_TYPE_DRIVE_B};

void System_OnStrgInit_EXMEM(void)
{
	static BOOL bStrg_init_EXMEM = FALSE;
	if (bStrg_init_EXMEM) {
		return;
	}
	TM_BOOT_BEGIN("sdio", "init");

#if (FS_DX_TYPE_DRIVE_A >= DX_TYPE_CARD1 && FS_DX_TYPE_DRIVE_A <= DX_TYPE_CARD3)
	DX_HANDLE DxCardDev1 = Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_A);
	UserSdioInit.buf.addr = mempool_storage_sdio;
	UserSdioInit.buf.size = POOL_SIZE_STORAGE_SDIO;
	Dx_Init(DxCardDev1, &UserSdioInit, 0, STORAGE_VER);
#endif
#if (FS_MULTI_STRG_FUNC && FS_DX_TYPE_DRIVE_B >= DX_TYPE_CARD1 && FS_DX_TYPE_DRIVE_B <= DX_TYPE_CARD3)
	DX_HANDLE DxCardDev2 = Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_B);
	UserSdio2Init.buf.Addr = OS_GetMempoolAddr(POOL_ID_STORAGE_SDIO2);
	UserSdio2Init.buf.Size = OS_GetMempoolSize(POOL_ID_STORAGE_SDIO2);
	Dx_Init(DxCardDev2, &UserSdio2Init, 0, STORAGE_VER);
#endif
	bStrg_init_EXMEM = TRUE;
	TM_BOOT_END("sdio", "init");
}

void System_OnStrgExit_EXMEM(void)
{
	//PHASE-1 : Close Drv or DrvExt

}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  FILESYS
//
///////////////////////////////////////////////////////////////////////////////
//Lib or LibExt
#if (FS_FUNC == ENABLE)

extern void Strg_CB(UINT32 event, UINT32 param1, UINT32 param2);
void Card_DetInsert(void);
void Card_DetBusy(void);
#if (LOGFILE_FUNC==ENABLE)
void System_DetErr(void);
#endif
#if (SDINSERT_FUNCTION == ENABLE)
SX_TIMER_ITEM(Card_DetInsert, Card_DetInsert, 2, FALSE)
#endif
SX_TIMER_ITEM(System_DetBusy, Card_DetBusy, 25, FALSE)
#if (LOGFILE_FUNC==ENABLE)
SX_TIMER_ITEM(System_DetErr,  System_DetErr,50, FALSE)
#endif
int SX_TIMER_DET_STRG_ID = -1;
int SX_TIMER_DET_SYSTEM_BUSY_ID = -1;
#if (LOGFILE_FUNC==ENABLE)
int SX_TIMER_DET_SYSTEM_ERROR_ID = -1;
#endif

#if (LOGFILE_FUNC==ENABLE)
#if defined(__FREERTOS)
_ALIGNED(64) static CHAR gLogFile_Buff[LOGFILE_BUFFER_SIZE]= {0};
static UINT32 gLogFile_Buff_Size = sizeof(gLogFile_Buff);
#else //defined(__LINUX_USER__)
#include <sys/ioctl.h>
#include <sys/mman.h>
/* begin/end dma-buf functions used for userspace mmap. */
struct dma_buf_sync {
	unsigned long long flags;
};
#define DMA_BUF_SYNC_READ      (1 << 0)
#define DMA_BUF_SYNC_WRITE     (2 << 0)
#define DMA_BUF_SYNC_RW        (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START     (0 << 2)
#define DMA_BUF_SYNC_END       (1 << 2)
#define DMA_BUF_SYNC_VALID_FLAGS_MASK \
	(DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END)
#define DMA_BUF_BASE		'b'
#define DMA_BUF_IOCTL_SYNC	_IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)
#define LOGFILE_BASE        'l'
#define LOGFILE_IOCTL_FD    _IOR(LOGFILE_BASE, 0, int)

static CHAR *gLogFile_Buff = NULL;
static UINT32 gLogFile_Buff_Size = 0;

static void logfile_init_dma_buff(void)
{
	int fd;

	fd = open("/dev/logfile", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		printf("open /dev/logfile failed, %s\n", strerror(errno));
		return;
	}

	gLogFile_Buff = mmap(NULL, LOGFILE_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (gLogFile_Buff == MAP_FAILED) {
		printf("mmap dmabuf failed: %s\n", strerror(errno));
		return;
	}
	gLogFile_Buff_Size = LOGFILE_BUFFER_SIZE;

	close(fd);
}
#endif
#if defined(_CPU2_LINUX_)
_ALIGNED(64) static CHAR gLogFile_Buff2[LOGFILE_BUFFER_SIZE]= {0};
static UINT32 gLogFile_Buff2_Size = sizeof(gLogFile_Buff2);
#endif
#endif

void System_OnStrgInit_FS(void)
{
	CHAR mount_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	TM_BOOT_BEGIN("sdio", "init_fs");
	{
		MEM_RANGE Pool;
		Pool.addr = mempool_filesys;
#if (FS_MULTI_STRG_FUNC == ENABLE)
		MEM_RANGE Pool2;
		Pool.size = POOL_SIZE_FILESYS;
		GxStrg_SetConfigEx(0, FILE_CFG_BUF, (UINT32)&Pool);
		Pool2.addr = Pool.addr + POOL_SIZE_FILESYS;
		Pool2.size = POOL_SIZE_FILESYS;
		GxStrg_SetConfigEx(1, FILE_CFG_BUF, (UINT32)&Pool2);
#if defined(_CPU2_LINUX_) && defined(_EMBMEM_EMMC_)
		// 3rd is for linux-pstore mounted by filesys
		MEM_RANGE Pool3;
		Pool3.Addr = Pool2.Addr + POOL_SIZE_FS_BUFFER;
		Pool3.Size = POOL_SIZE_FS_BUFFER;
		GxStrg_SetConfigEx(PST_DEV_ID, FILE_CFG_BUF, (UINT32)&Pool3);
#endif
#else
		Pool.size = POOL_SIZE_FILESYS;
		GxStrg_SetConfigEx(0, FILE_CFG_BUF, (UINT32)&Pool);
#endif
	}

	//#NT#2017/06/02#Nestor Yang -begin
	//#NT# Do not link uITRON if not use
	//GxStrg_SetConfigEx(0, FILE_CFG_FS_TYPE, m_GxStrgType);
#if !defined(__FREERTOS)
	GxStrg_SetConfigEx(0, FILE_CFG_FS_TYPE, FileSys_GetOPS_Linux()); //for FILE_CFG_FS_TYPE, DevID is don't care
#else
	GxStrg_SetConfigEx(0, FILE_CFG_FS_TYPE, FileSys_GetOPS_uITRON());
#endif
	//#NT#2017/06/02#Nestor Yang -end
#if 0
#if (LOGFILE_FUNC==ENABLE)
	GxStrg_SetConfigEx(0, FILE_CFG_MAX_OPEN_FILE, 6);
#endif
#if (USERLOG_FUNC == ENABLE)
	GxStrg_SetConfigEx(0, FILE_CFG_MAX_OPEN_FILE, 6);
#endif
#if (CURL_FUNC == ENABLE)
	GxStrg_SetConfigEx(0, FILE_CFG_MAX_OPEN_FILE, 8);
#endif
#if (IPCAM_FUNC == DISABLE)
	GxStrg_SetConfigEx(0, FILE_CFG_MAX_OPEN_FILE, 8);
#endif
#else
	GxStrg_SetConfigEx(0, FILE_CFG_MAX_OPEN_FILE, 10);
#endif

    //set the device node of msdc mode for emmc only
#if (defined(_EMBMEM_EMMC_) && !defined(__FREERTOS))
	emmc_set_dev_node("/dev/mmcblk2p5"); //This devicde node is related to storate-partition, it is last rootfslX logical partition. Using "cat /proc/nvt_info/emmc" to get.
#endif

	// mount path
#if (defined(_EMBMEM_EMMC_) && FS_MULTI_STRG_FUNC==DISABLE && !defined(__FREERTOS))
	strncpy(mount_path, "/mnt/emmc1", sizeof(mount_path) - 1);
#else
	strncpy(mount_path, "/mnt/sd", sizeof(mount_path) - 1);
#endif


	//strncpy(mount_path, "/mnt/sd", sizeof(mount_path) - 1);
	mount_path[sizeof(mount_path) - 1] = '\0';
    DBG_DUMP("FILE_CFG_MOUNT_PATH[0]: %s\r\n", mount_path);
	GxStrg_SetConfigEx(0, FILE_CFG_MOUNT_PATH, (UINT32)mount_path);
	#if (FS_MULTI_STRG_FUNC==ENABLE)
	strncpy(mount_path, "/mnt/emmc1", sizeof(mount_path) - 1);
	mount_path[sizeof(mount_path) - 1] = '\0';
    DBG_DUMP("FILE_CFG_MOUNT_PATH[1]: %s\r\n", mount_path);
	GxStrg_SetConfigEx(1, FILE_CFG_MOUNT_PATH, (UINT32)mount_path);
	#endif
#if !defined(__FREERTOS)
	GxStrg_SetConfigEx(0, FILE_CFG_STRG_OBJECT, (UINT32)Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_A));
	#if (FS_MULTI_STRG_FUNC==ENABLE)
	GxStrg_SetConfigEx(1, FILE_CFG_STRG_OBJECT, (UINT32)Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_B));
	#endif
#endif

  //#NT#2018/12/18#Philex Lin - begin
  // unused now
	#if 0
  // Enable 32MB alignment recording.
	GxStrg_SetConfigEx(0, FILE_CFG_ALIGNED_SIZE, 32 * 1024 * 1024);
  #endif
  //#NT#2018/12/18#Philex Lin - end

#if (LOGFILE_FUNC==ENABLE)
	{
		LOGFILE_CFG   cfg = {0};
		#if 0
		// only store system error log
		cfg.ConType = LOGFILE_CON_UART|LOGFILE_CON_MEM;
		#else
		// store normal log and system error log
		#if defined(_MODEL_CARDV_HS880C_)
		cfg.ConType = LOGFILE_CON_MEM|LOGFILE_CON_STORE;
		#else
		cfg.ConType = LOGFILE_CON_UART|LOGFILE_CON_STORE;
		#endif
		#endif
		cfg.TimeType = LOGFILE_TIME_TYPE_COUNTER;
		#if defined(__FREERTOS)
		#else //defined(__LINUX_USER__)
		logfile_init_dma_buff();
		#endif
		cfg.LogBuffAddr = (UINT32)gLogFile_Buff;
		cfg.LogBuffSize = gLogFile_Buff_Size;
		#if defined(_CPU2_LINUX_)
		cfg.LogBuffAddr2 = (UINT32)gLogFile_Buff2;
		cfg.LogBuffSize2 = gLogFile_Buff2_Size;
		#endif
		LogFile_Config(&cfg);
	}
#endif

	GxStrg_RegCB(Strg_CB);         //Register CB function of GxStorage (NANR or CARD)
	{
		//1.�]�winit��
		//FileSys:
		//2.�]�wCB��,
		//3.���USxJob�A�� ---------> System Job
		//4.���USxTimer�A�� ---------> Detect Job
#if (SDINSERT_FUNCTION == ENABLE)
		////if (m_GxStrgType == FST_FS_TYPE_UITRON) {
			SX_TIMER_DET_STRG_ID = SxTimer_AddItem(&Timer_Card_DetInsert);
		////}
#endif
#if (LOGFILE_FUNC==ENABLE)
		{
			SX_TIMER_DET_SYSTEM_ERROR_ID = SxTimer_AddItem(&Timer_System_DetErr);
		}
#endif
		//SX_TIMER_DET_SYSTEM_BUSY_ID = SxTimer_AddItem(&Timer_System_DetBusy);
		//5.���USxCmd�A�� ---------> Cmd Function
		//System_AddSxCmd(Storage_OnCommand); //GxStorage

		//start scan
		SxTimer_SetFuncActive(SX_TIMER_DET_STRG_ID, TRUE);
		SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID, TRUE);
	}
	TM_BOOT_END("sdio", "init_fs");

	if (m_GxStrgType == FST_FS_TYPE_UITRON) {
#if (FS_MULTI_STRG_FUNC)
		UINT32 uiDxState = 0;
		DX_HANDLE pStrgDev = Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_B);
		if (Dx_GetState((DX_HANDLE)pStrgDev, STORAGE_STATE_INSERT, &uiDxState) != DX_OK || uiDxState == FALSE) {
			Ux_PostEvent(NVTEVT_STRG_REMOVE, 1, 1);
		} else {
			Ux_PostEvent(NVTEVT_STRG_INSERT, 1, 1);
		}
#endif
	}

#if (USE_DCF == ENABLE)
	{
		// init DCF

//		CHAR pFolderName[9] = {0};
//		CHAR pFileName[5] = {0};
//		// init DCF FolderID/FileID with RTC data
//		struct tm tm_cur = hwclock_get_time(TIME_ID_CURRENT);
//		snprintf(pFolderName, sizeof(pFolderName), "%1d%02d%02d", tm_cur.tm_year % 0x0A, tm_cur.tm_mon, tm_cur.tm_mday);
//		snprintf(pFileName, sizeof(pFileName), "%02d%02d", tm_cur.tm_hour, tm_cur.tm_min);
//		//DCF dir-name
//		DCF_SetDirFreeChars(pFolderName);
//		//DCF file-name
//		DCF_SetFileFreeChars(DCF_FILE_TYPE_ANYFORMAT, pFileName);
//
//		//DCF format
//		DCF_SetParm(DCF_PRMID_SET_VALID_FILE_FMT, DCF_SUPPORT_FORMAT);
//		DCF_SetParm(DCF_PRMID_SET_DEP_FILE_FMT, DCF_FILE_TYPE_JPG | DCF_FILE_TYPE_WAV | DCF_FILE_TYPE_MPO);
//		//TODO: [DCF] How to add an new format & its ext?

		DCF_SetParm(DCF_PRMID_REMOVE_DUPLICATE_FOLDER, TRUE);
		DCF_SetParm(DCF_PRMID_REMOVE_DUPLICATE_FILE, TRUE);
		DCF_SetParm(DCF_PRMID_SET_VALID_FILE_FMT, DCF_FILE_TYPE_JPG|DCF_FILE_TYPE_MP4|DCF_FILE_TYPE_MOV);
		DCF_SetParm(DCF_PRMID_SET_DEP_FILE_FMT, DCF_FILE_TYPE_JPG|DCF_FILE_TYPE_WAV|DCF_FILE_TYPE_MPO);
		DCF_SetDirFreeChars("HUNTI");
		DCF_SetFileFreeChars(DCF_FILE_TYPE_ANYFORMAT, "IMAG");
	}
#endif
}
void System_OnStrgInit_FS2(void)
{
	// update card status again
	if (GxStrg_GetDeviceCtrl(0, CARD_READONLY)) {
		System_SetState(SYS_STATE_CARD, CARD_LOCKED);
	} else if (GxStrg_GetDeviceCtrl(0, CARD_INSERT)) {
		System_SetState(SYS_STATE_CARD, CARD_INSERTED);
	} else {
		System_SetState(SYS_STATE_CARD, CARD_REMOVED);
	}
}
void System_OnStrgInit_FS3(void)
{
	static BOOL bFirst = TRUE;
	if (bFirst) {
		TM_BOOT_BEGIN("sdio", "fdb_create");
	}
#if 0
#if(MOVIE_MODE==ENABLE)
	INT32 iCurrMode = System_GetBootFirstMode();
	if (iCurrMode == PRIMARY_MODE_MOVIE) {
		FlowMovie_FileDBCreate_Fast();
	}
#endif
#endif
	if (bFirst) {
		TM_BOOT_END("sdio", "fdb_create");
	}
	bFirst = FALSE;
}

void System_OnStrgExit_FS(void)
{
	//PHASE-2 : Close Lib or LibExt
#if (LOGFILE_FUNC==ENABLE)
	LogFile_Close();
#endif
	// unmount file system
	GxStrg_CloseDevice(0);
#if (FS_MULTI_STRG_FUNC)
	GxStrg_CloseDevice(1);
#endif

}

#if (PWR_FUNC == ENABLE)
static BOOL FileSys_DetBusy(void)
{
	return (BOOL)((INT32)FileSys_GetParam(FST_PARM_TASK_STS, 0) == FST_STA_BUSY);
}
#endif

void Card_DetInsert(void)
{
#if defined(__FREERTOS)
	GxStrg_Det(0, Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_A));
#if (0)//FS_MULTI_STRG_FUNC)
	GxStrg_Det(1, Dx_GetObject(DX_CLASS_STORAGE_EXT | FS_DX_TYPE_DRIVE_B));
#endif
#else
	GxStrg_Det(0, 0);
#endif
}

void Card_DetBusy(void)
{
#if (PWR_FUNC == ENABLE)
	static BOOL bBusyLED = FALSE;

	if (GxPower_GetControl(GXPWR_CTRL_SLEEP_LEVEL) <= 1 && (!GxPower_GetControl(GXPWR_CTRL_BATTERY_CHARGE_EN))) {
		if (FileSys_DetBusy()) {
			if (bBusyLED == FALSE) {
				DBG_IND("System - busy\r\n");
				bBusyLED = TRUE;
#if (OUTPUT_FUNC == ENABLE)
				GxLED_SetCtrl(KEYSCAN_LED_GREEN, SETLED_SPEED, GXLED_1SEC_LED_TOGGLE_CNT / 5);
				GxLED_SetCtrl(KEYSCAN_LED_GREEN, TURNON_LED, FALSE);
				GxLED_SetCtrl(KEYSCAN_LED_GREEN, SET_TOGGLE_LED, TRUE);
#endif
			}
		} else {
			if (bBusyLED == TRUE) {
				DBG_IND("System - not busy\r\n");
				bBusyLED = FALSE;
#if (OUTPUT_FUNC == ENABLE)
				GxLED_SetCtrl(KEYSCAN_LED_GREEN, SET_TOGGLE_LED, FALSE);
				GxLED_SetCtrl(KEYSCAN_LED_GREEN, TURNON_LED, FALSE);
#endif
			}
		}
	}
#endif
}

#if (FSCK_FUNC == ENABLE)
int search_str_in_file(char *path, char *str)
{
	FILE *fp = NULL;
	UINT32 u32ize = 0;
	int found = 0;
	char *pStrSrc = NULL;

	fp = fopen(path, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		u32ize = ftell(fp); // take file size
		fseek(fp, 0, SEEK_SET); // move to position zero
		pStrSrc = (char *)malloc(u32ize * sizeof(char));

		if (pStrSrc) {
			fread(pStrSrc, 1, u32ize, fp);
			if (strstr(pStrSrc, str)) {
				found = 1;
			}
			free(pStrSrc);
		}

		fclose(fp);
		fp = NULL;
		pStrSrc = NULL;
		u32ize = 0;
	}

	return found;
}

int System_check_mmcblk0p1(void)
{
	SysMain_system("ls /dev/mmcblk0p1 > /tmp/lsdev.txt");
    vos_util_delay_ms(100);
	if (search_str_in_file("/tmp/lsdev.txt", "/dev/mmcblk0p1")) {
		return 1;
	} else {
		return 0;
	}
}

int System_mount_storage(char *pMountPath)
{
	int ret = 0;
	time_t t, t_local, t_gmt;
	struct tm tt_local, tt_gmt;
	char opts[20];
	char *pDevSrc;
	char *pFileSysType = "vfat";

    #if (defined(_EMBMEM_EMMC_) && FS_MULTI_STRG_FUNC==DISABLE && !defined(__FREERTOS))
		pDevSrc = "/dev/mmcblk2p5"; //Please cat /proc/nvt_info/emmc to get corrected /dev/mmcblk2pXX
	#else
	if (System_check_mmcblk0p1()) {
		pDevSrc = "/dev/mmcblk0p1";
	} else {
		pDevSrc = "/dev/mmcblk0";
	}
	#endif

	time(&t);
	localtime_r(&t, &tt_local);
	gmtime_r(&t, &tt_gmt);
	t_local = mktime(&tt_local);
	t_gmt = mktime(&tt_gmt);

	snprintf(opts, 19, "time_offset=%d", (t_local-t_gmt)/60);
	DBG_IND("gtime=%d, ltime=%d, diff=%d, m=%d, %s\r\n", t_gmt, t_local, (t_local-t_gmt), (t_local-t_gmt)/60, opts);

	// mount sd card
	ret = mount(pDevSrc, pMountPath, pFileSysType, MS_DIRSYNC, opts);

	if(ret) {
        if (errno == EBUSY) {
            DBG_ERR("%s is existed, don't need to mount again\r\n", pMountPath);
        } else {
            DBG_ERR("mount /mnt/sd failed, errno = %d\r\n",errno);
        }
	}

	return ret;
}

int System_umount_storage(char *pMountPath)
{
    int ret = 0;

    ret = umount(pMountPath);

    if (ret) {
        if (errno == EBUSY) {
            //DBG_ERR("Catn't umount, please use 'lsof' to check the resource\r\n");
        } else if (errno == EINVAL) {
            //DBG_ERR("%s isn't existed, don't need to umount again\r\n", pMountPath);
        }
	}

	return ret;
}
#endif

INT32 System_OnStrgInsert(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 stg_id = paramArray[0];
    #if (FSCK_FUNC == ENABLE)
    int ret_val = 0;
    #if (defined(_EMBMEM_EMMC_) && FS_MULTI_STRG_FUNC==DISABLE && !defined(__FREERTOS))
    char *pMountPath = "/mnt/emmc1";
    #else
    char *pMountPath = "/mnt/sd";
    #endif
#endif

	if (m_BootState_Drive[stg_id] != BOOT_CARD_STATE_UNKNOWN) {
		if (stg_id == 0) {
#if(IPCAM_FUNC==DISABLE && SDHOTPLUG_FUNCTION == DISABLE)
			System_PowerOff(SYS_POWEROFF_NORMAL);
#endif
		}
	} else {
		TM_BOOT_BEGIN("sdio", "mount_fs");
		m_BootState_Drive[stg_id] = BOOT_CARD_STATE_INSERTED;
	}

    #if (FSCK_FUNC == ENABLE)
    //SysMain_system("df"); //only for debug, the "/mnt/sd" path must not be existed.
    System_umount_storage(pMountPath);
    /*Under normal situation, fsck checking doesn't need to print the message via UART port.
      We sotre the message to /tmp/fsck.txt (on DRAM). */
    #if (defined(_EMBMEM_EMMC_) && FS_MULTI_STRG_FUNC==DISABLE && !defined(__FREERTOS))
        SysMain_system("fsck.fat -a /dev/mmcblk2p5 > /tmp/fsck.txt"); //Please cat /proc/nvt_info/emmc to get corrected /dev/mmcblk2pXX
    #else
	if (System_check_mmcblk0p1()) {
		DBG_IND("fsck /dev/mmcblk0p1\r\n");
		SysMain_system("fsck.fat -a /dev/mmcblk0p1 > /tmp/fsck.txt"); //Store to /tmp/fsck.txt and use "cat /tmp/fsck.txt".
		//SysMain_system("fsck.fat -a /dev/mmcblk0p1"); //The fsck checking will print the message directly.
	} else {
		DBG_IND("no /dev/mmcblk0p1, try to fsck /dev/mmcblk0\r\n");
		SysMain_system("fsck.fat -a /dev/mmcblk0 > /tmp/fsck.txt"); //Store to /tmp/fsck.txt and use "cat /tmp/fsck.txt".
		//SysMain_system("fsck.fat -a /dev/mmcblk0"); //The fsck checking will print the message directly.
	}
    #endif

    ret_val = System_mount_storage(pMountPath);
    if (ret_val) {
        GxStrg_SetStrgMountStatus(stg_id, FALSE);
        DBG_ERR("mount /mnt/sd failed, ret_val is %d; errno = %d\r\n", ret_val, errno);
    } else {
        GxStrg_SetStrgMountStatus(stg_id, TRUE);
    }
    //SysMain_system("df"); //only for debug, the "/mnt/sd" path must be existed.
    #endif

	// linux partition as PStore
	if (stg_id == PST_DEV_ID) {
#if defined(_CPU2_LINUX_)
		if (GxStrg_OpenDevice(stg_id, NULL) != TRUE) {
			DBG_ERR("Storage mount pstore fail\r\n");
		}
		return NVTEVT_CONSUME;
#else
		DBG_FATAL("stg_id cannot be %d.\r\n", PST_DEV_ID);
#endif
	}
#if (FS_MULTI_STRG_FUNC == ENABLE)
	DX_HANDLE pStrgDev = NULL;
	switch (stg_id) {
	case 0:
		pStrgDev = (DX_HANDLE)sdio_getStorageObject(STRG_OBJ_FAT1);
		break;

	case 1:
		pStrgDev = (DX_HANDLE)emmc_getStorageObject(STRG_OBJ_FAT1);
		break;
	}
#else
	DX_HANDLE pStrgDev = (DX_HANDLE)sdio_getStorageObject(STRG_OBJ_FAT1);
#endif
	if (GxStrg_OpenDevice(stg_id, pStrgDev) != TRUE) {
		char *pDxName = "unknown";
		Dx_GetInfo(pStrgDev, DX_INFO_NAME, &pDxName);
		DBG_ERR("Storage mount %s fail\r\n", pDxName);
		return NVTEVT_CONSUME;
	}

#if (USE_DCF == ENABLE)
	{
		DCF_OPEN_PARM           dcfParm = {0};
		// Open DCF
		dcfParm.Drive = (stg_id == 0) ? 'A' : 'B';
#if	(FS_MULTI_STRG_FUNC)
		if (POOL_CNT_DCF_BUFFER !=2) {
			DBG_FATAL("POOL_CNT_DCF_BUFFER be 2 for FS_MULTI_STRG_FUNC.\r\n");
		} else {
			switch(stg_id) {
			case 0:
				dcfParm.WorkbuffAddr = dma_getCacheAddr(OS_GetMempoolAddr(POOL_ID_DCF_BUFFER));
				break;
			case 1:
				dcfParm.WorkbuffAddr = dma_getCacheAddr(OS_GetMempoolAddr(POOL_ID_DCF_BUFFER)) + POOL_SIZE_DCF_BUFFER;
				break;
			default:
				DBG_ERR("unknown stg_id=%d\r\n", stg_id);
				dcfParm.WorkbuffAddr = 0;
				break;
			}
		}
#else

		dcfParm.WorkbuffAddr = mempool_dcf;//dma_getCacheAddr(OS_GetMempoolAddr(POOL_ID_DCF_BUFFER));
#endif

		dcfParm.WorkbuffSize = POOL_SIZE_DCF_BUFFER;
		DCF_Open(&dcfParm);
		DCF_ScanObj();
	}
#endif

	if (GxStrg_GetDeviceCtrl(stg_id, CARD_READONLY)) {
		System_SetState(SYS_STATE_CARD, CARD_LOCKED);
		DBG_IND("Card Locked\r\n");
	} else {
		System_SetState(SYS_STATE_CARD, CARD_INSERTED);
		DBG_IND("Card inserted\r\n");
	}

#if (SDHOTPLUG_FUNCTION == ENABLE)
	INT32 mode = System_GetState(SYS_STATE_CURRMODE);
	DBG_IND("card insert: current mode %d\r\n", mode);
	if (mode != SYS_MODE_UNKNOWN) {
		Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, mode);
	}
#endif

	return NVTEVT_CONSUME;
}


INT32 System_OnStrgRemove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 stg_id = paramArray[0];
	if (m_BootState_Drive[stg_id] != BOOT_CARD_STATE_UNKNOWN) {
		if (stg_id == 0) {
#if (LOGFILE_FUNC==ENABLE)
			LogFile_Suspend();
			LogFile_Close();
#endif
#if (USERLOG_FUNC == ENABLE)
			{
				userlog_close();
			}
#endif
#if (USE_DCF == ENABLE)
			//Fix the error "DCF_GetInfoByHandle() Dcf Handle 0 Data may be overwritted" when card plug out/in
			DCF_Close(0);
#endif
			System_SetState(SYS_STATE_CARD, CARD_REMOVED);

			GxStrg_CloseDevice(stg_id);
#if(IPCAM_FUNC==DISABLE && SDHOTPLUG_FUNCTION == DISABLE)
			System_PowerOff(SYS_POWEROFF_NORMAL);
#endif
		}
	} else {
		TM_BOOT_BEGIN("sdio", "mount_fs");
		m_BootState_Drive[stg_id] = BOOT_CARD_STATE_REMOVED;
		#if (FS_SWITCH_STRG_FUNC == ENABLE)
		if (stg_id==0) {
			DX_HANDLE pStrgDev = Dx_GetObject(DX_CLASS_STORAGE_EXT|FS_DX_TYPE_DRIVE_B);
			if (GxStrg_OpenDevice(stg_id, pStrgDev)!= TRUE) {
				char* pDxName="unknown";
				Dx_GetInfo(pStrgDev, DX_INFO_NAME,&pDxName);
				DBG_ERR("Storage mount %s fail\r\n",pDxName);
				return NVTEVT_CONSUME;
			}
			System_SetState(SYS_STATE_CARD, CARD_INSERTED);
			return NVTEVT_CONSUME;
		}
		#else
		// boot without card, send attach to continue UI flow.
		// because of UserWaitEvent(NVTEVT_STRG_ATTACH, &paramNum, paramArray);
		Ux_PostEvent(NVTEVT_STRG_ATTACH, 2, stg_id, 0xFF);
		#endif
	}

	DX_HANDLE pStrgDev = Dx_GetObject(DX_CLASS_STORAGE_EXT | m_FsDxTypeMap[stg_id]);
	if (GxStrg_CloseDevice(stg_id) != TRUE) {
		char *pDxName = "unknown";
		Dx_GetInfo(pStrgDev, DX_INFO_NAME, &pDxName);
		DBG_ERR("Storage mount %s fail\r\n", pDxName);
		return NVTEVT_CONSUME;
	}

#if (SDHOTPLUG_FUNCTION == ENABLE)
	INT32 mode = System_GetState(SYS_STATE_CURRMODE);
	DBG_IND("card remove: current mode %d\r\n", mode);
	if (mode != SYS_MODE_UNKNOWN) {
		Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, mode);
	}
#endif

	return NVTEVT_CONSUME;
}

INT32 System_OnStrgAttach(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    #if defined(__FREERTOS)
	SHMINFO *p_shm;
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();
	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */
	unsigned int *p_data;
    #endif
	UINT32 result = paramArray[1];

#if defined(__FREERTOS)
	if (p_fdt== NULL) {
		DBG_ERR("p_fdt is NULL.\n");
		return NVTEVT_CONSUME;
	}

	// read SHMEM_PATH
	nodeoffset = fdt_path_offset(p_fdt, SHMEM_PATH);
	if (nodeoffset < 0) {
		DBG_ERR("failed to offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	} else {
		DBG_DUMP("offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	}
	nodep = fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("failed to access reg.\n");
		return NVTEVT_CONSUME;
	} else {
		p_data = (unsigned int *)nodep;
		p_shm = (SHMINFO *)be32_to_cpu(p_data[0]);
		DBG_DUMP("p_shm = 0x%08X\n", (int)p_shm);
	}
	//////////////////////////////////////////////////
#endif

	if (m_GxStrgType == FST_FS_TYPE_LINUX) {
		//Do Nothing
	} else { //m_GxStrgType == FST_FS_TYPE_UITRON
		//if spi use RamDisk as inner Memory,need to format RamDisk
#if defined(_EMBMEM_SPI_NOR_)
		FS_HANDLE pStrgDevCur = (FS_HANDLE)GxStrg_GetDevice(0);
		FS_HANDLE pStrgDevRAM = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | USER_DX_TYPE_EMBMEM_FAT);

		if ((pStrgDevCur == pStrgDevRAM) && (result != FST_STA_OK)) {
			result = FileSys_FormatDisk(pStrgDevCur, TRUE);
		}
#endif

#if ( !defined(_EMBMEM_SPI_NOR_) && (FS_MULTI_STRG_FUNC == ENABLE))
		UINT32 stg_id = paramArray[0];

		if (stg_id != 0) { // stg_id=1 is interal storage FAT
			return NVTEVT_CONSUME;
		}
#endif
	}

	switch (result) {
	case FST_STA_OK:
#if (USE_DCF == ENABLE)
		if (!UI_GetData(FL_IsCopyToCarding)) {
			DCF_ScanObj();
		}
#endif
		System_SetState(SYS_STATE_FS, FS_INIT_OK);

#if defined(__FREERTOS)
		if (p_shm && p_shm->boot.LdCtrl2 & LDCF_UPDATE_FW) {
			FST_FILE hFile = FileSys_OpenFile(FW_DEL_INDIACTION_PATH, FST_OPEN_READ | FST_OPEN_EXISTING);
			if (hFile != NULL) {
				DBG_DUMP("Detected %s, delete %s \r\n", FW_DEL_INDIACTION_PATH, LOADER_UPD_FW_PATH);
				FileSys_CloseFile(hFile);
				// Delete FW bin from A:
				if (FileSys_DeleteFile(LOADER_UPD_FW_PATH) != FST_STA_OK) {
					DBG_ERR("delete "_BIN_NAME_".BIN failed .\r\n");
				}
				if (FileSys_DeleteFile(FW_DEL_INDIACTION_PATH) != FST_STA_OK) {
					DBG_ERR("delete %s failed .\r\n", FW_DEL_INDIACTION_PATH);
				}
			}
		}
#endif

#if (LOGFILE_FUNC==ENABLE)
		if (SxTimer_GetFuncActive(SX_TIMER_DET_SYSTEM_ERROR_ID) == 0)
		{
			LOGFILE_OPEN    logOpenParm = {0};
			UINT32          maxFileNum = 32;
			UINT32          maxFileSize = 0x100000; // 1MB
			CHAR            rootDir[LOGFILE_ROOT_DIR_MAX_LEN + 1] = "A:\\Novatek\\LOG\\";
			#if defined(_CPU2_LINUX_)
			CHAR            rootDir2[LOGFILE_ROOT_DIR_MAX_LEN + 1] = "A:\\Novatek\\LOG2\\";
			#endif
			CHAR            sysErrRootDir[LOGFILE_ROOT_DIR_MAX_LEN + 1] = "A:\\Novatek\\SYS\\";

			logOpenParm.maxFileNum = maxFileNum;
			logOpenParm.maxFileSize = maxFileSize;
			logOpenParm.isPreAllocAllFiles = TRUE;
			#if defined(__FREERTOS)
			logOpenParm.isSaveLastTimeSysErrLog  = wdt_getResetNum()>0 ? TRUE : FALSE;
			#else //defined(__LINUX_USER__)
			logOpenParm.isSaveLastTimeSysErrLog = FALSE;
			#endif
			logOpenParm.lastTimeSysErrLogBuffAddr = mempool_logfile;
			logOpenParm.lastTimeSysErrLogBuffSize = POOL_SIZE_LOGFILE;
			strncpy(logOpenParm.rootDir, rootDir, LOGFILE_ROOT_DIR_MAX_LEN);
			#if defined(_CPU2_LINUX_)
			strncpy(logOpenParm.rootDir2, rootDir2, LOGFILE_ROOT_DIR_MAX_LEN);
			#endif
			strncpy(logOpenParm.sysErrRootDir, sysErrRootDir, LOGFILE_ROOT_DIR_MAX_LEN);
			logOpenParm.isZeroFile = TRUE;
			LogFile_Open(&logOpenParm);

			//start scan
			SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_ERROR_ID, TRUE);

		}
#endif
#if (USERLOG_FUNC == ENABLE)
		{
			userlog_open();
		}
#endif
		break;
	case FST_STA_DISK_UNFORMAT:
		System_SetState(SYS_STATE_FS, FS_UNFORMATTED);
		break;
	case FST_STA_DISK_UNKNOWN_FORMAT:
		System_SetState(SYS_STATE_FS, FS_UNKNOWN_FORMAT);
		break;
	case FST_STA_CARD_ERR:
		System_SetState(SYS_STATE_FS, FS_DISK_ERROR);
		break;
	default:
		System_SetState(SYS_STATE_FS, FS_DISK_ERROR);
		break;
	}
#if (POWERON_FAST_BOOT == ENABLE)
	INIT_SETFLAG(FLGINIT_STRGATH);
#endif
	Ux_PostEvent(NVTEVT_STORAGE_INIT, 0, 0);
#if (PWR_FUNC == ENABLE)
	GxPower_SetControl(GXPWR_CTRL_AUTOSLEEP_EN, 0xff); //reset
#endif
	return NVTEVT_CONSUME;
}

INT32 System_OnStrgDetach(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if (m_GxStrgType == FST_FS_TYPE_LINUX) {
		UINT32 stg_id = paramArray[0];

		if (stg_id != 0) { //not sd-1
			return NVTEVT_CONSUME;
		}
	}

	switch (paramArray[1]) {
	case FST_STA_OK:
		DBG_IND("FS: unmount OK\r\n");
		break;

	default:
		DBG_ERR("^RFS: unmount FAIL\r\n");
		break;
	}
	return NVTEVT_CONSUME;
}

BOOL gChkCardPwrOn = FALSE;
BOOL gChkCardChange = FALSE;

void Storage_PowerOn_Start(void)
{
	gChkCardPwrOn = GxStrg_GetDeviceCtrl(0, CARD_INSERT);
	DBG_IND("^BStg Power On = %d\r\n", gChkCardPwrOn);
}
void Storage_UpdateSource(void)
{
	DBG_IND("^Y-------------CARD det\r\n");
	if (gChkCardPwrOn) {
		if (FALSE == GxStrg_GetDeviceCtrl(0, CARD_INSERT)) { //CARD�w�ް�
			gChkCardChange = TRUE;
		}
	} else {
		if (TRUE == GxStrg_GetDeviceCtrl(0, CARD_INSERT)) { //CARD�w���J
			gChkCardChange = TRUE;
		}
	}
}
void Storage_PowerOn_End(void)
{
	Storage_UpdateSource();
	gChkCardPwrOn = GxStrg_GetDeviceCtrl(0, CARD_INSERT);

	if (TRUE == gChkCardChange) { //CARD���g����
		System_PowerOff(SYS_POWEROFF_NORMAL); //����
		return;
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  PSTORE
//
///////////////////////////////////////////////////////////////////////////////
#if (PST_FUNC == ENABLE)
#define PST_IND_FILE      "/mnt/pstore/.psinit"

void System_PS_Format(void)
{
	PSFMT gFmtStruct = {MAX_BLK_PER_SEC, MAX_SEC_NUM};
	PStore_Format(&gFmtStruct);
}
void System_OnStrgInit_PS(void)
{
	struct stat st;
	char ind_file_name[64];

	//PHASE-2 : Init & Open Lib or LibExt
	TM_BOOT_BEGIN("nand", "init_ps");
#if (PST_FUNC == ENABLE)
	// Open PStore
	PSFMT gFmtStruct = {MAX_BLK_PER_SEC, MAX_SEC_NUM};
	PSTORE_INIT_PARAM gPStoreParam;
	UINT32 result = 0;
#if defined(__FREERTOS)
	UINT8 *pBuf;
#endif
#if defined(_CPU2_LINUX_) && defined(_EMBMEM_EMMC_)
	PStore_Init(PS_TYPE_FILESYS, PST_FS_DRIVE[0]);
	UINT32 paramNum;
	UINT32 paramArray[MAX_MESSAGE_PARAM_NUM];
	do {
		UserWaitEvent(NVTEVT_STRG_ATTACH, &paramNum, paramArray);
	} while(paramArray[0] != PST_DEV_ID); //PStore will mount first before dev[0],dev[1]
#else
	PStore_Init(PS_TYPE_EMBEDED, 0);
#endif
#if defined(__FREERTOS)
	pBuf = (UINT8 *)mempool_pstore;
	if(!pBuf){
		DBG_ERR("fail to allocate pstore buffer of %d bytes\n", POOL_SIZE_PS_BUFFER);
		return;
	}

	gPStoreParam.pBuf = pBuf;
	gPStoreParam.uiBufSize = POOL_SIZE_PS_BUFFER;
	DBG_DUMP("PStore uiBufSize=%d\r\n", gPStoreParam.uiBufSize);
#else
	gPStoreParam.pBuf = NULL;
	gPStoreParam.uiBufSize = 0;
#endif

#if (defined(_EMBMEM_EMMC_) && !defined(__FREERTOS))
    //This is to relate with the storage-partition.
    //If the order is  different, the /dev/mmcblk2XX will be different.
    //Please "cat /proc/nvt_info/emmc" to see the information.
    result = PStore_SetDevNode("/dev/mmcblk2p12");
#endif

	result = PStore_Open(NULL, &gPStoreParam);

	if ((result != E_PS_OK) || (stat(PST_IND_FILE, &st) != 0)) {
		DBG_ERR("PStore Open fail %d format \r\n", result);
		// close pstore to umount pstore partition
		PStore_Close();
		// after format, pstore partition will be re-mounted, so don't call open again
		PStore_Format(&gFmtStruct);
		// create indicated file
		strncpy(ind_file_name, "touch ", 10);
		strncat(ind_file_name, PST_IND_FILE, 50);
		system(ind_file_name);
	}

#if defined(_CPU2_LINUX_)
	{
		PSTOREIPC_OPEN   PsopenObj = {0};
		// open pstore ipc
		PstoreIpc_Open(&PsopenObj);
	}
#endif
#endif
#if (POWERON_FAST_BOOT == ENABLE)
	INIT_SETFLAG(FLGINIT_MOUNTPS);
#endif
	TM_BOOT_END("nand", "init_ps");
}
void System_OnStrgExit_PS(void)
{
#if (PST_FUNC == ENABLE)
#if defined(_CPU2_LINUX_)
	PstoreIpc_Close();
#endif
	PStore_Close();
#endif
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  FWSTORE
//
///////////////////////////////////////////////////////////////////////////////
#if (FWS_FUNC == ENABLE)
#include "FileSysTsk.h"
#if (FWS_FUNC == ENABLE)
#include "FwSrvApi.h"
#endif
#include "MemCheck.h"

typedef enum {
	CODE_SECTION_01 = 0,
	CODE_SECTION_02,
	CODE_SECTION_03,
	CODE_SECTION_04,
	CODE_SECTION_05,
	CODE_SECTION_06,
	CODE_SECTION_07,
	CODE_SECTION_08,
	CODE_SECTION_09,
	CODE_SECTION_10,
	CODE_SECTION_11,
	CODE_SECTION_12,
	CODE_SECTION_13,
	CODE_SECTION_14,
	CODE_SECTION_15,
	CODE_SECTION_16,
	ENUM_DUMMY4WORD(_CODE_SECTION_)
}
CODE_SECTION;

UINT32 UserSection_Load[10] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
UINT32 UserSection_Order_full[] = {
	CODE_SECTION_02, CODE_SECTION_03, CODE_SECTION_04, CODE_SECTION_05, CODE_SECTION_06,
	CODE_SECTION_07, CODE_SECTION_08, CODE_SECTION_09, CODE_SECTION_10, FWSRV_PL_BURST_END_TAG
};

int order = 1;
void UserSection_LoadCb(const UINT32 Idx)
{
	DBG_DUMP("Section-%.2ld: (LOAD)\r\n", Idx + 1);

	UserSection_Load[Idx] = 1; //mark loaded
	order++;

	if (Idx == 9) { //Section-10
		if (mp_fwsrv_work_buf) {
			free(mp_fwsrv_work_buf);
			mp_fwsrv_work_buf = NULL;
		}
	}
#if (POWERON_FAST_BOOT == ENABLE)
	if (Idx == 0) { //Section-01
		INIT_SETFLAG(FLGINIT_LOADCODE1);
	}
	if (Idx == 1) { //Section-02
		INIT_SETFLAG(FLGINIT_LOADCODE2);
	}
	if (Idx == 2) { //Section-03
		INIT_SETFLAG(FLGINIT_LOADCODE3);
	}
	if (Idx == 9) { //Section-10
	}
#endif
}

void System_OnStrgInit_FWS(void)
{
	SHMINFO *p_shm;
	FWSRV_INIT Init = {0};
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();


	if (p_fdt == NULL) {
		DBG_ERR("p_fdt is NULL.\n");
		return;
	}

	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */

	// read SHMEM_PATH
	nodeoffset = fdt_path_offset(p_fdt, SHMEM_PATH);
	if (nodeoffset < 0) {
		DBG_ERR("failed to offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	} else {
		DBG_DUMP("offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	}
	nodep = fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("failed to access reg.\n");
		return;
	} else {
		unsigned int *p_data = (unsigned int *)nodep;
		p_shm = (SHMINFO *)be32_to_cpu(p_data[0]);
		DBG_DUMP("p_shm = 0x%08X\n", (int)p_shm);
		if (p_shm->boot.LdCtrl2 & LDCF_BOOT_CARD) {
			DBG_DUMP(DBG_COLOR_HI_YELLOW"\r\nNotice: Boot from T.bin. app.dtb will load from flash!!!\r\n\r\n"DBG_COLOR_END);
		}
	}

	// init fwsrv
	Init.uiApiVer = FWSRV_API_VERSION;
	Init.StrgMap.pStrgFdt = EMB_GETSTRGOBJ(STRG_OBJ_FW_FDT);
	Init.StrgMap.pStrgApp = EMB_GETSTRGOBJ(STRG_OBJ_FW_APP);
	Init.StrgMap.pStrgUboot = EMB_GETSTRGOBJ(STRG_OBJ_FW_UBOOT);
	Init.StrgMap.pStrgRtos = EMB_GETSTRGOBJ(STRG_OBJ_FW_RTOS);
	Init.PlInit.uiApiVer = PARTLOAD_API_VERSION;
	Init.PlInit.pStrg = EMB_GETSTRGOBJ(STRG_OBJ_FW_RTOS);
#if defined(_FW_TYPE_PARTIAL_COMPRESS_)
	mp_fwsrv_work_buf = (void *)malloc(FW_PARTIAL_COMPRESS_WORK_BUF_SIZE);
	Init.PlInit.DataType = PARTLOAD_DATA_TYPE_COMPRESS_GZ;
	Init.PlInit.uiWorkingAddr = (UINT32)mp_fwsrv_work_buf;
	Init.PlInit.uiWorkingSize = FW_PARTIAL_COMPRESS_WORK_BUF_SIZE ;

#elif defined(_FW_TYPE_PARTIAL_)
	mp_fwsrv_work_buf = (void *)malloc(_EMBMEM_BLK_SIZE_);
	Init.PlInit.DataType = PARTLOAD_DATA_TYPE_UNCOMPRESS;
	Init.PlInit.uiWorkingAddr = (UINT32)mp_fwsrv_work_buf;
	Init.PlInit.uiWorkingSize = _EMBMEM_BLK_SIZE_ ;
#endif
	Init.PlInit.uiAddrBegin = _BOARD_RTOS_ADDR_ + p_shm->boot.LdLoadSize;
	FwSrv_Init(&Init);
	FwSrv_Open();
}

void System_OnStrgExit_FWS(void)
{
#if (FWS_FUNC == ENABLE)
	ER er;
	er = FwSrv_Close();
	if (er != FWSRV_ER_OK) {
		DBG_ERR("Close failed!\r\n");
	}
#endif
}

void System_CPU2_Start(void)
{
}

void System_CPU2_Stop(void)
{
}

void System_CPU2_WaitReady(void)
{
}

void System_DSP_Start(void)
{
}

void System_DSP_WaitReady(void)
{
}

void System_OnStrg_DownloadFW(void)
{
#if defined(_FW_TYPE_PARTIAL_) || defined(_FW_TYPE_PARTIAL_COMPRESS_)
	SHMINFO *p_shm;
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();

	if (p_fdt == NULL) {
		DBG_ERR("p_fdt is NULL.\n");
		return;
	}

	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */

	// read SHMEM_PATH
	nodeoffset = fdt_path_offset(p_fdt, SHMEM_PATH);
	if (nodeoffset < 0) {
		DBG_ERR("failed to offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	} else {
		DBG_DUMP("offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	}
	nodep = fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("failed to access reg.\n");
		return;
	} else {
		unsigned int *p_data = (unsigned int *)nodep;
		p_shm = (SHMINFO *)be32_to_cpu(p_data[0]);
		DBG_DUMP("p_shm = 0x%08X\n", (int)p_shm);
	}

	if ((p_shm->boot.LdCtrl2 & LDCF_BOOT_CARD) == 0) {
		FWSRV_CMD Cmd = {0};
		FWSRV_PL_LOAD_BURST_IN pl_in = {0};

		// start partial load
		void (*LoadCallback)(const UINT32 Idx) = UserSection_LoadCb;
		UINT32 *SecOrderTable = UserSection_Order_full;

		LoadCallback(CODE_SECTION_01); // 1st part has loaded by loader

		ER er;
		pl_in.puiIdxSequence = SecOrderTable;
		pl_in.fpLoadedCb = LoadCallback;
		Cmd.Idx = FWSRV_CMD_IDX_PL_LOAD_BURST; //continue load
		Cmd.In.pData = &pl_in;
		Cmd.In.uiNumByte = sizeof(pl_in);
		Cmd.Prop.bExitCmdFinish = TRUE;

		er = FwSrv_Cmd(&Cmd);
		if (er != FWSRV_ER_OK) {
			DBG_ERR("Process failed!\r\n");
			return;
		}
	}
#endif
	return;
}
#endif

#if (IPCAM_FUNC != ENABLE)
/*
static void CheckSumOKCb(void)
{
    //Create a zero file to indicate deleting FW after loader update
    FST_FILE hFile = FileSys_OpenFile(FW_DEL_INDIACTION_PATH,FST_OPEN_WRITE|FST_OPEN_ALWAYS);
    if(hFile != NULL)
    {
        FileSys_CloseFile(hFile);
    }
    else
    {
        DBG_ERR("create indication %s failed.\r\n",FW_DEL_INDIACTION_PATH);
        return;
    }
    //Rename to be safe, that loader can update by name for next updating if FW updating FW failed.
    if(FileSys_RenameFile(_BIN_NAME_".BIN",uiUpdateFWName, TRUE)==FST_STA_OK)
    {
        m_bUpdRename = TRUE;
    }
    else
    {
        DBG_ERR("cannot rename bin file.\r\n");
    }
}
*/
UINT32 System_OnStrg_UploadFW(UINT32 DevID)
{
	INT32 fst_er;
	//ER fws_er;
	unsigned char *p_Mem = NULL;
	FST_FILE hFile = NULL;
	UINT32 uiFwSize = 0;
    DBG_FUNC_BEGIN("\r\n");

	//m_bUpdRename = FALSE;

	uiFwSize = FileSys_GetFileLen(FW_UPDATE_NAME);
	hFile = FileSys_OpenFile(FW_UPDATE_NAME, FST_OPEN_READ);

	if (hFile == 0) {
		DBG_ERR("cannot find %s\r\n", FW_UPDATE_NAME);
		return UPDNAND_STS_FW_INVALID_STG;
	}


	p_Mem = (unsigned char *)SxCmd_GetTempMem(uiFwSize);
    if(NULL == p_Mem){
        DBG_ERR("Alloc Temp Mem is NULL(uiFwSize=%d)\r\n",uiFwSize);
        return UPDNAND_STS_FW_READ2_ERR;
    }

	fst_er = FileSys_ReadFile(hFile, (UINT8 *)p_Mem, &uiFwSize, 0, NULL);
    DBG_IND("Update Filename=%s, File Size=%d\r\n",FW_UPDATE_NAME,uiFwSize);
	FileSys_CloseFile(hFile);
	if (fst_er != FST_STA_OK) {
		DBG_ERR("FW bin read fail\r\n");
		return UPDNAND_STS_FW_READ_ERR;
	}

    /*
	FWSRV_BIN_UPDATE_ALL_IN_ONE Desc = {0};
	Desc.uiSrcBufAddr = (UINT32)p_Mem;
	Desc.uiSrcBufSize = (UINT32)uiFwSize;
	Desc.fpCheckSumCb = CheckSumOKCb; //we rename bin after check sum OK

	FWSRV_CMD Cmd = {0};
	Cmd.Idx = FWSRV_CMD_IDX_BIN_UPDATE_ALL_IN_ONE; //continue load
	Cmd.In.pData = &Desc;
	Cmd.In.uiNumByte = sizeof(Desc);
	Cmd.Prop.bExitCmdFinish = TRUE;

	fws_er = FwSrv_Cmd(&Cmd);
	*/

	SxCmd_RelTempMem((UINT32)p_Mem);

    /*
	if(fws_er == FWSRV_ER_INVALID_UPDATED_DATA || fws_er == FWSRV_ER_WRITE_BLOCK)
    {
        DBG_ERR("update failed, start to use loader update mechanism.\r\n");
        //GxSystem_EnableHWReset(0);
        //GxSystem_PowerOff();
        return UPDNAND_STS_FW_WRITE_CHK_ERR;
    }
    else if(fws_er != FWSRV_ER_OK)
    {
        DBG_ERR("FW bin check failed %d\r\n",fws_er);
        if(m_bUpdRename)
        {
            if(FileSys_RenameFile(&uiUpdateFWName[3],LOADER_UPD_FW_PATH, TRUE)==FST_STA_OK) //[3]: remove "A:\\"
            {
                FileSys_DeleteFile(FW_DEL_INDIACTION_PATH);
            }
        }
        return UPDNAND_STS_FW_READ_CHK_ERR;
    }
    */
    DBG_FUNC_END("\r\n");
	return UPDNAND_STS_FW_OK;
}
#endif

#if (LOGFILE_FUNC==ENABLE)
void System_DetErr(void)
{
	#if 0  //LOGFILE_FUNC TODO
	extern void LogFile_DumpMemAndSwReset(void);

	#if (LOGFILE_FUNC==ENABLE)
	if (LogFile_ChkSysErr() != E_OK) {
		LogFile_DumpMemAndSwReset();
	}
	#endif
	#endif //LOGFILE_FUNC TODO
}
#endif

///////////////////////////////////////////////////////////////////////////////
