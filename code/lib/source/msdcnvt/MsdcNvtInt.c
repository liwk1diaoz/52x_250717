#include "MsdcNvtInt.h"
#include "msdcnvt_ipc.h"

#define MSDCNVT_IPC_CAST_IN(type, p_icmd) \
	( (p_icmd->in_size == sizeof(type))?(type *)MSDCNVT_NONCACHE_ADDR(p_icmd->in_addr) : NULL)
#define MSDCNVT_IPC_CAST_OUT(type, p_icmd) \
	( (p_icmd->out_size == sizeof(type))?(type *)MSDCNVT_NONCACHE_ADDR(p_icmd->out_addr) : NULL)


//Local Variable
static MSDCNVT_CTRL m_Ctrl; //do not init to {0} for speed up zi area

//Internal APIs
MSDCNVT_CTRL *xMsdcNvt_GetCtrl(void)
{
	return &m_Ctrl;
}

#if defined(_NETWORK_ON_CPU2_)
static void xMsdcNvt_SendIPC(UINT32 api_addr)
{
	MSDCNVT_IPC_MSG msg = {0};
	msg.mtype = 2;
	msg.api_addr = api_addr;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();

	if (NvtIPC_MsgSnd(pCtrl->tIPC.ipc_msgid, NVTIPC_SENDTO_CORE2, &msg, sizeof(msg)) < 0) {
		DBG_ERR("NvtIPC_MsgSnd\r\n");
	}
}

void xMsdcNvtTsk_Ipc(void)
{
	FLGPTN FlgPtn;
	NVTIPC_I32 ipc_err = 0;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();

	while (1) {
		MSDCNVT_IPC_MSG msg = {0};

		PROFILE_TASK_IDLE();

		ipc_err = NvtIPC_MsgRcv(pCtrl->tIPC.ipc_msgid, &msg, sizeof(msg));
		if (ipc_err == NVTIPC_ER_MSGQUE_RELEASED) {
			break;
		} else if (ipc_err <= 0) {
			DBG_ERR("NvtIPC_MsgRcv\r\n");
		}

		PROFILE_TASK_BUSY();

		MSDCNVT_ICMD *pCmd = &pCtrl->tIPC.pCfg->cmd;
		((MSDCNVT_IAPI)pCmd->api_addr)(pCmd);
		xMsdcNvt_SendIPC(pCmd->api_addr);
	}

	set_flg(pCtrl->FlagID, FLGMSDCNVT_IPC_STOPPED);
	wai_flg(&FlgPtn, pCtrl->FlagID, FLGMSDCNVT_UNKNOWN, TWF_ORW | TWF_CLR);
}
#endif

static void msdcnvt_icmd_cb_open(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	pCtrl->tIPC.bNetRunning = TRUE;
	icmd->err = 0;
}

static void msdcnvt_icmd_cb_close(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	pCtrl->tIPC.bNetRunning = FALSE;
	icmd->err = 0;
}

static void msdcnvt_icmd_cb_verify(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CBW *pCBW = MSDCNVT_IPC_CAST_IN(MSDCNVT_CBW, icmd);
	UINT32 *pAddr = MSDCNVT_IPC_CAST_OUT(UINT32, icmd);

	if (pCBW == NULL) {
		DBG_ERR("invalid pCBW size\r\n");
		icmd->err = -2;
		return;
	} else if (pAddr == NULL) {
		DBG_ERR("invalid MSDCNVT_IPC_CAST_OUT address\r\n");
		icmd->err = -3;
		return;
	}
	icmd->err = (MsdcNvt_Verify_Cb((UINT32)pCBW, pAddr)) ? 0 : -1;

	//writeback cache for receiving data from PC
	MSDCNVT_HOST_INFO *pHost = &xMsdcNvt_GetCtrl()->tHostInfo;

	if (pCBW->bmCBWFlags == SBC_CMD_DIR_IN) {
		//data from device to PC, so writeback to DRAM
		//dma_flushWriteCache(*pAddr, pHost->uiLB_Length);
		vos_cpu_dcache_sync(*pAddr, pHost->uiLB_Length, VOS_DMA_TO_DEVICE);
	} else if (pCBW->bmCBWFlags == SBC_CMD_DIR_OUT) {
		//data from PC to device, so drop cache
		//dma_flushReadCache(*pAddr, pHost->uiLB_Length);
		vos_cpu_dcache_sync(*pAddr, pHost->uiLB_Length, VOS_DMA_FROM_DEVICE);
	}

	//*pAddr = vos_cpu_get_phy_addr(*pAddr);
}

static void msdcnvt_icmd_cb_vendor(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CBW *pCBW = MSDCNVT_IPC_CAST_IN(MSDCNVT_CBW, icmd);
	MSDCNVT_VENDOR_OUTPUT *pOutput = MSDCNVT_IPC_CAST_OUT(MSDCNVT_VENDOR_OUTPUT, icmd);

	if (pCBW == NULL) {
		DBG_ERR("invalid pCBW size\r\n");
		icmd->err = -2;
		return;
	}
	if (pOutput == NULL) {
		DBG_ERR("invalid pOutput size\r\n");
		icmd->err = -3;
		return;
	}

	MSDCVendorParam Param = {0};
	Param.VendorCmdBuf = (UINT32)pCBW;
	Param.VendorCSWBuf = (UINT32)&pOutput->csw;
	MsdcNvt_Vendor_Cb(&Param);
	//pOutput->InDataBuf = vos_cpu_get_phy_addr(Param.InDataBuf);
	pOutput->InDataBuf = Param.InDataBuf;
	pOutput->InDataBufLen = Param.InDataBufLen;
	pOutput->OutDataBufLen = Param.OutDataBufLen;

	//writeback cache for sending data to PC
	if (Param.InDataBuf == 0) {
		//accessing register will get Param.InDataBuf is NULL
		MSDCNVT_HOST_INFO *pHost = &xMsdcNvt_GetCtrl()->tHostInfo;
		//dma_flushWriteCache((UINT32)pHost->pPoolMem, pOutput->InDataBufLen);
		vos_cpu_dcache_sync((UINT32)pHost->pPoolMem, pOutput->InDataBufLen, VOS_DMA_TO_DEVICE);
	} else {
		//dma_flushWriteCache(Param.InDataBuf, pOutput->InDataBufLen);
		vos_cpu_dcache_sync(Param.InDataBuf, pOutput->InDataBufLen, VOS_DMA_TO_DEVICE);
	}

	icmd->err = 0;
}

static MSDCNVT_ICALL_TBL m_calls = {
	msdcnvt_icmd_cb_open,
	msdcnvt_icmd_cb_close,
	msdcnvt_icmd_cb_verify,
	msdcnvt_icmd_cb_vendor,
};

MSDCNVT_ICALL_TBL *xMsdcNvt_GetIpcCallTbl(UINT32 *pCnt)
{
	*pCnt = sizeof(m_calls) / sizeof(MSDCNVT_IAPI);
	return &m_calls;
}
