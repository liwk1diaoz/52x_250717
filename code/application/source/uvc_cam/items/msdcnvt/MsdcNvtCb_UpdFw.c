#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umsd.h>
#include <kwrap/task.h>
#include <FileSysTsk.h>
#include <FwSrvApi.h>
#include <msdcnvt/MsdcNvtApi.h>
#include <msdcnvt//MsdcNvtCallback.h>
#include "MsdcNvtCb_UpdFw.h"

#include <kwrap/debug.h>

#define ALL_IN_ONE_MAX_SIZE 0x1000000

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

	uiWorkMem  = (UINT32)malloc(ALL_IN_ONE_MAX_SIZE);
	pDesc->uiAddr = uiWorkMem;
	pDesc->uiSize = ALL_IN_ONE_MAX_SIZE;

	pDesc->tParent.bOK = TRUE;
}

static void system_reboot(void)
{
	DBG_DUMP("system_reboot... not implement yet.\r\n");
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

	free((void *)uiWorkMem);

	system_reboot();

	pDesc->tParent.bOK = TRUE;
}
