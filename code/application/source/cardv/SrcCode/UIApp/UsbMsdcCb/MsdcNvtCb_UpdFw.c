#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umsd.h>
#include <kwrap/task.h>
#include <FileSysTsk.h>
#include <FwSrvApi.h>
#include <msdcnvt/MsdcNvtApi.h>
#include <msdcnvt//MsdcNvtCallback.h>
#include <hwpower.h>
#include <timer.h>
#include "MsdcNvtCb_UpdFw.h"
#include "PrjInc.h"
#include "SysMain.h"

#if defined(__FREERTOS)

#define ALL_IN_ONE_MAX_SIZE 0x1000000
#define CFG_USE_MALLOC DISABLE

static UINT32 uiWorkMem = 0;

//for MSDCVendorNVT_AddCallback_Bi
static void MsdcNvtCb_UpdFwAllInOne_Step1(void *pData);
static void MsdcNvtCb_UpdFwAllInOne_Step2(void *pData);

MSDCNVT_REG_BI_BEGIN(m_MsdcNvtUpdfw)
MSDCNVT_REG_BI_ITEM(MsdcNvtCb_UpdFwAllInOne_Step1)
MSDCNVT_REG_BI_ITEM(MsdcNvtCb_UpdFwAllInOne_Step2)
MSDCNVT_REG_BI_END()

BOOL MsdcNvtRegBi_UpdFw(void)
{
	return MsdcNvt_AddCallback_Bi(m_MsdcNvtUpdfw);
}

static void MsdcNvtCb_UpdFwAllInOne_Step1(void *pData)
{
	tMSDCEXT_UPDFWALLINONE_STEP1 *pDesc = MSDCNVT_CAST(tMSDCEXT_UPDFWALLINONE_STEP1, pData);
	if (pDesc == NULL) {
		return;
	}

	//Delete Firmware in Cards
	DBG_DUMP("\r\nDelete firmware in A:\\ \r\n");
	if (FileSys_DeleteFile("A:\\"_BIN_NAME_T_".BIN") != FST_STA_OK) {
		DBG_IND("Ignore deleting file.\r\n");
	}

	if (FileSys_DeleteFile("A:\\"_BIN_NAME_".BIN") != FST_STA_OK) {
		DBG_IND("Ignore deleting file.\r\n");
	}

#if (CFG_USE_MALLOC)
	uiWorkMem  = (UINT32)malloc(ALL_IN_ONE_MAX_SIZE);
#else
	uiWorkMem  = (UINT32)SysMain_GetTempBuffer(ALL_IN_ONE_MAX_SIZE);
#endif
	if (uiWorkMem) {
		pDesc->uiAddr = uiWorkMem;
		pDesc->uiSize = ALL_IN_ONE_MAX_SIZE;
		pDesc->tParent.bOK = TRUE;
	}
}

static void system_reboot(UINT32 event)
{
	DBG_DUMP("\nsystem_reboot...\n");
	hwpower_set_power_key(POWER_ID_HWRT, 0xff); //h/w reset NOW
}

static void MsdcNvtCb_UpdFwAllInOne_Step2(void *pData)
{
	tMSDCEXT_UPDFWALLINONE_STEP2 *pDesc = MSDCNVT_CAST(tMSDCEXT_UPDFWALLINONE_STEP2, pData);

	if (pDesc == NULL) {
		return;
	}

	if (pDesc->uiKey != MAKEFOURCC('N', 'O', 'V', 'A')) {
		DBG_ERR("MsdcNvtCb_UpdFwSetBlock: Invalid Key\r\n!");
		return;
	}

	FWSRV_BIN_UPDATE_ALL_IN_ONE Desc = {0};
	Desc.uiSrcBufAddr = (UINT32)uiWorkMem;
	Desc.uiSrcBufSize = (UINT32)pDesc->EffectDataSize;

	FWSRV_CMD Cmd = {0};
	Cmd.Idx = FWSRV_CMD_IDX_BIN_UPDATE_ALL_IN_ONE; //continue load
	Cmd.In.pData = &Desc;
	Cmd.In.uiNumByte = sizeof(Desc);
	Cmd.Prop.bExitCmdFinish = TRUE;

	ER er = FwSrv_Cmd(&Cmd);

	if (er != FWSRV_ER_OK) {
		DBG_ERR("Process failed!\r\n");
		pDesc->tParent.bOK = FALSE;
		return;
	}

#if (CFG_USE_MALLOC)
	free((void *)uiWorkMem);
#else
	SysMain_RelTempBuffer(uiWorkMem);
#endif

	TIMER_ID timer_id;
	if (timer_open(&timer_id, system_reboot) == E_OK) {
		timer_cfg(timer_id, 500000, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	}

	pDesc->tParent.bOK = TRUE;
}

#else
BOOL MsdcNvtRegBi_UpdFw(void)
{
	return FALSE;
}
#endif