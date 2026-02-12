#include "PrjCfg.h"
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
#include "UsbCmdInt.h"
#include "UIApp/WifiCmdParser/WifiCmdParser.h"

#define PRI_USBCMD                 10
#define STKSIZE_USBCMD             4096
#define STKSIZE_USBCMD_DOWNLOAD    2048

#define MAX_USB_CMD_LEN            128

ID FLG_ID_USBCMD = 0;
ID SEMID_USBCMD_1ST_COM = 0;
static BOOL g_bUsbCmdTskOpened = FALSE;
static THREAD_HANDLE USBCMDTSK_ID, USBCMDTSK2_ID, USBCMDDOWNLOADTSK_ID;
static UINT32 usbcmd_tsk_run = 0, is_usbcmd_tsk_running = 0;
static UINT32 usbcmd_tsk2_run = 0, is_usbcmd_tsk2_running = 0;
static UINT32 usbcmddownload_tsk_run = 0, is_usbcmddownload_tsk_running = 0;

//for receving XML cmd
THREAD_RETTYPE UsbCmdTsk(void)
{
	INT32  ret_value;
	UINT32 CmdStrLen;
	INT32 timeout = -1;

	THREAD_ENTRY();
	//DBG_FUNC_BEGIN("\r\n");
	is_usbcmd_tsk_running = TRUE;
	while (usbcmd_tsk_run) {
		CmdStrLen = MAX_USB_CMD_LEN;
		memset((void *)g_RecvBuf[0].addr_va, 0, CmdStrLen);
		hd_common_mem_flush_cache((void *)g_RecvBuf[0].addr_va, CmdStrLen);
		vos_flag_set(FLG_ID_USBCMD, FLGUSBCMD_IDLE);
		ret_value = UVAC_ReadCdcData(USB_CMD_PORT, (UINT8 *)g_RecvBuf[0].addr_pa, CmdStrLen, timeout);
		vos_flag_clr(FLG_ID_USBCMD, FLGUSBCMD_IDLE);

		if (ret_value > 0) {
			hd_common_mem_flush_cache((void *)g_RecvBuf[0].addr_va, ret_value);
			UsbCmdHandler((UINT8 *)g_RecvBuf[0].addr_va, ret_value);
		} else {
			DBG_IND("Aobrt ReadCdc to exit (%d).\r\n", ret_value);
			break;
		}
	}
	is_usbcmd_tsk_running = FALSE;
	//DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}

//for receving raw data ?
THREAD_RETTYPE UsbCmdTsk2(void)
{
	INT32  ret_value;
	UINT32 DataLen;
	INT32 timeout = -1;

	THREAD_ENTRY();
	//DBG_FUNC_BEGIN("\r\n");
	is_usbcmd_tsk2_running = TRUE;
	while (usbcmd_tsk2_run) {
		DataLen = g_RecvBuf[1].size;
		vos_flag_set(FLG_ID_USBCMD, FLGUSBCMD_IDLE2);
		ret_value = UVAC_ReadCdcData(USB_DATA_PORT, (UINT8 *)g_RecvBuf[1].addr_pa, DataLen, timeout);
		vos_flag_clr(FLG_ID_USBCMD, FLGUSBCMD_IDLE2);

		if (ret_value > 0) {
			hd_common_mem_flush_cache((void *)g_RecvBuf[1].addr_va, ret_value);
			UsbRcvDataHandler((UINT32)g_RecvBuf[1].addr_va, DataLen);
		} else {
			DBG_IND("Aobrt ReadCdc to exit. (%d)\r\n", ret_value);
			break;
		}
	}
	is_usbcmd_tsk2_running = FALSE;
	//DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}

//for downloading file
THREAD_RETTYPE UsbCmdDownloadTsk(void)
{
	FLGPTN      uiFlag = 0;

	THREAD_ENTRY();
	//DBG_FUNC_BEGIN("\r\n");
	is_usbcmddownload_tsk_running = TRUE;
	while (usbcmddownload_tsk_run) {
		vos_flag_set(FLG_ID_USBCMD, FLGUSBCMD_IDLE3);
		vos_flag_wait(&uiFlag, FLG_ID_USBCMD, FLGUSBCMD_DOWNLOAD, TWF_ORW | TWF_CLR);
		vos_flag_clr(FLG_ID_USBCMD, FLGUSBCMD_IDLE3);
		UsbCmdDownloadHandler();
	}
	is_usbcmddownload_tsk_running = FALSE;
	//DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}

ER UsbCmdTsk_Open(void)
{
	ER ret = E_OK;
	T_CFLG cflg;

	if (g_bUsbCmdTskOpened) {
		return E_SYS;
	}

	WifiCmd_InstallID();

	memset(&cflg, 0, sizeof(T_CFLG));
	if (vos_flag_create(&FLG_ID_USBCMD, &cflg, "USBCMD") != E_OK) {
		DBG_ERR("FLG_ID_USBCMD fail\r\n");
		ret = E_SYS;
	}

	vos_flag_clr(FLG_ID_USBCMD, FLGUSBCMD_ALL);

	if (vos_sem_create(&SEMID_USBCMD_1ST_COM, 1, "SEMID_USBCMD_1ST_COM") != E_OK) {
		DBG_ERR("Create SEMID_USBCMD_1ST_COM fail\r\n");
		ret = E_SYS;
	}

	if ((USBCMDTSK_ID = vos_task_create(UsbCmdTsk, 0, "UsbCmdTsk", PRI_USBCMD, STKSIZE_USBCMD)) == 0) {
		DBG_ERR("UsbCmdTsk create failed.\r\n");
		ret = E_SYS;
	} else {
		usbcmd_tsk_run = TRUE;
		vos_task_resume(USBCMDTSK_ID);
	}

	if ((USBCMDTSK2_ID = vos_task_create(UsbCmdTsk2, 0, "UsbCmdTsk2", PRI_USBCMD, STKSIZE_USBCMD)) == 0) {
		DBG_ERR("UsbCmdTsk2 create failed.\r\n");
		ret = E_SYS;
	} else {
		usbcmd_tsk2_run = TRUE;
		vos_task_resume(USBCMDTSK2_ID);
	}

	if ((USBCMDDOWNLOADTSK_ID = vos_task_create(UsbCmdDownloadTsk, 0, "UsbCmdDownloadTsk", PRI_USBCMD, STKSIZE_USBCMD_DOWNLOAD)) == 0) {
		DBG_ERR("UsbCmdDownloadTsk create failed.\r\n");
		ret = E_SYS;
	} else {
		usbcmddownload_tsk_run = TRUE;
		vos_task_resume(USBCMDDOWNLOADTSK_ID);
	}
	g_bUsbCmdTskOpened = TRUE;

	return ret;
}

ER UsbCmdTsk_Close(void)
{
	FLGPTN  FlgPtn;
	UINT32 delay_cnt;

	delay_cnt = 50;

	if (!g_bUsbCmdTskOpened) {
		return E_SYS;
	}

	vos_flag_wait(&FlgPtn, FLG_ID_USBCMD, FLGUSBCMD_IDLE, TWF_ORW);
	//use abort to exit task
	UVAC_AbortCdcRead(USB_CMD_PORT);

	vos_flag_wait(&FlgPtn, FLG_ID_USBCMD, FLGUSBCMD_IDLE2, TWF_ORW);
	//use abort to exit task
	UVAC_AbortCdcRead(USB_DATA_PORT);

	vos_flag_wait(&FlgPtn, FLG_ID_USBCMD, FLGUSBCMD_IDLE3, TWF_ORW);

	usbcmd_tsk_run = FALSE;
	usbcmd_tsk2_run = FALSE;
	usbcmddownload_tsk_run = FALSE;
	delay_cnt = 50;

	while ((is_usbcmd_tsk_running || is_usbcmd_tsk2_running || is_usbcmddownload_tsk_running) && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	if (is_usbcmd_tsk_running) {
		DBG_DUMP("Destroy UsbCmdTsk\r\n");
		vos_task_destroy(USBCMDTSK_ID);
	}
	if (is_usbcmd_tsk2_running) {
		DBG_DUMP("Destroy UsbCmdTsk2\r\n");
		vos_task_destroy(USBCMDTSK2_ID);
	}
	if (is_usbcmddownload_tsk_running) {
		DBG_DUMP("Destroy UsbCmdDownloadTsk\r\n");
		vos_task_destroy(USBCMDDOWNLOADTSK_ID);
	}
	if (vos_flag_destroy(FLG_ID_USBCMD) != E_OK) {
		DBG_ERR("FLG_ID_USBCMD destroy failed.\r\n");
	}
	vos_sem_destroy(SEMID_USBCMD_1ST_COM);

	WifiCmd_UninstallID();

	g_bUsbCmdTskOpened = FALSE;

	return E_OK;
}

#endif

