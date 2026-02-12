#include <msdcnvt/MsdcNvtCallback.h>
#include "MsdcNvtInt.h"
#include "MsdcNvtID.h"


//Local Functions
static BOOL xMsdcNvt_Function(void);
static BOOL xMsdcNvt_CalcAccessAddressWithSize(void);
static void xMsdcNvt_RegisterAccess(UINT32 *pDst, UINT32 *pSrc, UINT32 nDWORDs);

//Local Variable
static MSDCNVT_LUN  m_NullLun = {0};

//APIs
BOOL MsdcNvt_Verify_Cb(UINT32 pCmdBuf, UINT32 *pDataBuf) //!< Callback for USB_MSDC_INFO.msdc_check_cb
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;

	vos_sem_wait(MSDCNVT_SEM_CB_ID);

	//Check Internal Status
	if (pCtrl->uiInitKey != CFG_MSDCNVT_INIT_KEY) {
		DBG_ERR("Not Initial\r\n");
		vos_sem_sig(MSDCNVT_SEM_CB_ID);
		return FALSE;
	}

	pHost->pCBW = (MSDCNVT_CBW *)pCmdBuf;
	pHost->uiCmd = pHost->pCBW->CBWCB[0];

	//Check Novatek Tag
	if (pHost->pCBW->CBWCB[1] != CDB_01_NVT_TAG) {
		vos_sem_sig(MSDCNVT_SEM_CB_ID);
		return FALSE;
	} else if (!xMsdcNvt_CalcAccessAddressWithSize()) {
		vos_sem_sig(MSDCNVT_SEM_CB_ID);
		return FALSE;
	} else if (pCtrl->tHostInfo.uiLB_Length  > pCtrl->tHostInfo.uiPoolSize) {
		DBG_ERR("data exceed %d Bytes\r\n", (int)(pCtrl->tHostInfo.uiPoolSize));
		vos_sem_sig(MSDCNVT_SEM_CB_ID);
		return FALSE;
	}

	switch (pHost->uiCmd) {
	case SBC_CMD_MSDCNVT_READ:
	case SBC_CMD_MSDCNVT_WRITE:
		if ((pHost->uiLB_Address & CFG_VIRTUAL_MEM_MAP_MASK) == CFG_VIRTUAL_MEM_MAP_ADDR) {
			if (pCtrl->tHandler.fpVirtualMemMap) {
				pHost->uiLB_Address = pCtrl->tHandler.fpVirtualMemMap((pHost->uiLB_Address & (~CFG_VIRTUAL_MEM_MAP_MASK)), pHost->uiLB_Length); //Customer
				*pDataBuf = pHost->uiLB_Address;
			} else {
				DBG_ERR("MSDCNVT_HANDLER_TYPE_INVALID_MEM_READ called, but no handler registered\r\n");
				*pDataBuf = 0;
			}
		} else if (pHost->uiLB_Address >= CFG_REGISTER_BEGIN_ADDR) {
#if defined(__FREERTOS)
				*pDataBuf = (UINT32)pHost->pPoolMem; //Register
#else
				*pDataBuf = (UINT32)pHost->pPoolMem_pa; //Register
				vos_cpu_dcache_sync((VOS_ADDR)pHost->pPoolMem, pHost->uiLB_Length, VOS_DMA_BIDIRECTIONAL);
#endif
		} else {
			*pDataBuf = pHost->uiLB_Address;     //DRAM
		}
		break;
	case SBC_CMD_MSDCNVT_FUNCTION:
#if defined(__FREERTOS)
		*pDataBuf = (UINT32)pHost->pPoolMem;
#else
		*pDataBuf = (UINT32)pHost->pPoolMem_pa;
		vos_cpu_dcache_sync((VOS_ADDR)pHost->pPoolMem, pHost->uiLB_Length, VOS_DMA_BIDIRECTIONAL);
#endif
		break;
	default:
		vos_sem_sig(MSDCNVT_SEM_CB_ID);
		return FALSE;
	}

	if(pCtrl->pMsdc) {
		pCtrl->pMsdc->SetConfig(USBMSDC_CONFIG_ID_CHGVENINBUGADDR, *pDataBuf);
	}

	//vos_sem_sig(MSDCNVT_SEM_CB_ID); at MsdcNvt_Vendor_Cb when MsdcNvt_Verify_Cb success
	//coverity[missing_unlock] ,the unlock at MsdcNvt_Vendor_Cb

	return TRUE;
}

void MsdcNvt_Vendor_Cb(PMSDCVendorParam pBuf) //!< Callback for USB_MSDC_INFO.msdc_vendor_cb
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;

	pHost->pCBW = (MSDCNVT_CBW *)pBuf->VendorCmdBuf;
	pHost->pCSW = (MSDCNVT_CSW *)pBuf->VendorCSWBuf;

	//At device side, the CSWTag has to be written as the same with dCBWTag
	pHost->pCSW->dCSWTag = pHost->pCBW->dCBWTag;

	switch (pHost->uiCmd) {
	case SBC_CMD_MSDCNVT_READ:
		if ((pHost->uiLB_Address & CFG_VIRTUAL_MEM_MAP_MASK) == CFG_VIRTUAL_MEM_MAP_ADDR) {
			pBuf->InDataBuf = pHost->uiLB_Address;
		} else if (pHost->uiLB_Address >= CFG_REGISTER_BEGIN_ADDR) { //Register
			xMsdcNvt_RegisterAccess((UINT32 *)pHost->pPoolMem, (UINT32 *)pHost->uiLB_Address, pHost->uiLB_Length >> 2);
#if !defined(__FREERTOS)
			vos_cpu_dcache_sync((VOS_ADDR)pHost->pPoolMem, pHost->uiLB_Length, VOS_DMA_BIDIRECTIONAL);
#endif
		} else {//DRAM
			pBuf->InDataBuf = pHost->uiLB_Address;
		}
		break;
	case SBC_CMD_MSDCNVT_WRITE:
		if ((pHost->uiLB_Address & CFG_VIRTUAL_MEM_MAP_MASK) == CFG_VIRTUAL_MEM_MAP_ADDR) {
			pBuf->InDataBuf = pHost->uiLB_Address;
		} else if (pHost->uiLB_Address >= CFG_REGISTER_BEGIN_ADDR) { //Register
			xMsdcNvt_RegisterAccess((UINT32 *)pHost->uiLB_Address, (UINT32 *)pHost->pPoolMem, pHost->uiLB_Length >> 2);
		} else {//DRAM
			pBuf->InDataBuf = pHost->uiLB_Address;
		}
		break;
	case SBC_CMD_MSDCNVT_FUNCTION:
		xMsdcNvt_Function();
#if defined(__FREERTOS)
		pBuf->InDataBuf = (UINT32)pHost->pPoolMem;
#else
		pBuf->InDataBuf = (UINT32)pHost->pPoolMem_pa;
		vos_cpu_dcache_sync((VOS_ADDR)pHost->pPoolMem, pHost->uiLB_Length, VOS_DMA_BIDIRECTIONAL);
#endif
		break;
	default:
		DBG_ERR("unknown command id: %d\r\n", (int)(pHost->uiCmd));
		vos_sem_sig(MSDCNVT_SEM_CB_ID);
		return;
	}

	pBuf->InDataBufLen = pBuf->OutDataBufLen = pHost->uiLB_Length;
	vos_sem_sig(MSDCNVT_SEM_CB_ID);
}

//!< for Register Callback Function
BOOL MsdcNvt_AddCallback_Bi(MSDCNVT_BI_CALL_UNIT *pUnit)
{
	MSDCNVT_BI_CALL_CTRL *pBiCall = &xMsdcNvt_GetCtrl()->tBiCall;

	if (pBiCall->pHead == NULL) {
		pBiCall->pHead = pUnit;
	}

	if (pBiCall->pLast != NULL) {
		pBiCall->pLast->fp = (void *)pUnit;
	}

	while (pUnit->szName != NULL) {
		pUnit++;
	}
	pBiCall->pLast = pUnit;

	return TRUE;
}

BOOL MsdcNvt_AddCallback_Si(FP *fpTblGet, UINT8 nGets, FP *fpTblSet, UINT8 nSets)
{
	MSDCNVT_SI_CALL_CTRL *pSiCtrl = &xMsdcNvt_GetCtrl()->tSiCall;
	pSiCtrl->fpTblGet = fpTblGet;
	pSiCtrl->uiGets = nGets;
	pSiCtrl->fpTblSet = fpTblSet;
	pSiCtrl->uiSets = nSets;

	return TRUE;
}

BOOL MsdcNvt_GetCallback_Si(FP **fpTblGet, UINT8 *nGets, FP **fpTblSet, UINT8 *nSets)
{
	MSDCNVT_SI_CALL_CTRL *pSiCtrl = &xMsdcNvt_GetCtrl()->tSiCall;
	*fpTblGet = pSiCtrl->fpTblGet;
	*nGets = pSiCtrl->uiGets;
	*fpTblSet = pSiCtrl->fpTblSet;
	*nSets = pSiCtrl->uiSets;

	return TRUE;
}

UINT8 *MsdcNvt_GetDataBufferAddress(void)
{
	return xMsdcNvt_GetCtrl()->tBkCtrl.pPoolMem;
}

UINT32 MsdcNvt_GetDataBufferSize(void)
{
	return xMsdcNvt_GetCtrl()->tBkCtrl.uiPoolSize;
}

UINT32 MsdcNvt_GetTransSize(void)
{
	return xMsdcNvt_GetCtrl()->tBkCtrl.uiTransmitSize;
}

BOOL MsdcNvt_Init(MSDCNVT_INIT *pInit)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_BACKGROUND_CTRL *pBk = &pCtrl->tBkCtrl;
	UINT8 *pMemPool = pInit->pMemCache;
	UINT8 *pMemPool_pa = pInit->pMemCache_pa;
	UINT32 nMemSize = pInit->uiSizeCache;

	MsdcNvt_InstallID();

	memset(pCtrl, 0, sizeof(MSDCNVT_CTRL));

	if (pInit->uiApiVer != MSDCNVT_API_VERSION) {
		DBG_ERR("wrong version\r\n");
		return FALSE;
	}

	//Check Valid
	if (pInit->uiSizeCache < CFG_MIN_WORKING_BUF_SIZE) {
		DBG_ERR("uiSizeCache minimum need %d bytes\r\n", (int)(CFG_MIN_WORKING_BUF_SIZE));
		return FALSE;
	}

#if !defined(__FREERTOS)
	if (pMemPool_pa == NULL) {
		DBG_ERR("MSDCNVT_INIT::pMemCache_pa cannot be NULL.\r\n");
	}
#endif

	pCtrl->FlagID = MSDCNVT_FLG_ID;
	pCtrl->fpEvent = pInit->fpEvent;
	pCtrl->pMsdc = pInit->pMsdc;

	//CBW Offset
	pMemPool += CFG_NVTMSDC_SIZE_CBW_DATA;
	pMemPool_pa += CFG_NVTMSDC_SIZE_CBW_DATA;
	nMemSize -= CFG_NVTMSDC_SIZE_CBW_DATA;
	//IPC Data
	pCtrl->tIPC.pCfg = (MSDCNVT_IPC_CFG *)pMemPool;
	pMemPool += CFG_NVTMSDC_SIZE_IPC_DATA;
	pMemPool_pa += CFG_NVTMSDC_SIZE_IPC_DATA;
	nMemSize -= CFG_NVTMSDC_SIZE_IPC_DATA;
	//Set Host Ctrl
	pHost->pPoolMem = pMemPool;
	pHost->pPoolMem_pa = pMemPool_pa;
	pHost->uiPoolSize = CFG_MIN_HOST_MEM_SIZE;
	pMemPool += pHost->uiPoolSize;
	pMemPool_pa += pHost->uiPoolSize;
	nMemSize -= pHost->uiPoolSize;
	//Set Background Ctrl
	pBk->TaskID = MSDCNVT_TSK_ID;
	pBk->pPoolMem = pMemPool;
	pBk->pPoolMem_pa = pMemPool_pa;
	pBk->uiPoolSize = CFG_MIN_BACKGROUND_MEM_SIZE;
	pMemPool += pBk->uiPoolSize;
	pMemPool_pa += pBk->uiPoolSize;
	nMemSize -= pBk->uiPoolSize;

#if 0 //dynamic create
	if (pBk->TaskID == 0) {
		DBG_ERR("ID not installed\r\n");
		return FALSE;
	}
#endif

	//Set IPC
	pCtrl->tIPC.pCfg->ipc_version = MSDCNVT_IPC_VERSION;
	pCtrl->tIPC.pCfg->port = (pInit->uiPort == 0) ? 1968 : pInit->uiPort;
	pCtrl->tIPC.pCfg->tos = pInit->uiTos;
	pCtrl->tIPC.pCfg->snd_buf_size = pInit->uiSndBufSize;
	pCtrl->tIPC.pCfg->rcv_buf_size = pInit->uiRecvBufSize;
	pCtrl->tIPC.pCfg->call_tbl_addr = MSDCNVT_PHY_ADDR((UINT32)xMsdcNvt_GetIpcCallTbl((UINT32 *)&pCtrl->tIPC.pCfg->call_tbl_count));
	pCtrl->tIPC.ipc_msgid = MAKEFOURCC('M','S','D','C');
	if (pCtrl->tIPC.ipc_msgid < 0) {
		DBG_ERR("failed to msgget\r\n");
		return FALSE;
	}


	if (pInit->bHookDbgMsg == TRUE) {
#if (CFG_DBGSYS)
		INT32 i;
		MSDCNVT_DBGSYS_CTRL *pDbgSys = &pCtrl->tDbgSys;
		//Set DbgSys
		pDbgSys->SemID = MSDCNVT_SEM_ID;
		pDbgSys->uiMsgCntMask   = CFG_DBGSYS_MSG_PAYLOAD_NUM - 1;
		pDbgSys->uiPayloadNum   = CFG_DBGSYS_MSG_PAYLOAD_NUM;
		pDbgSys->uiPayloadSize  = nMemSize / CFG_DBGSYS_MSG_PAYLOAD_NUM;
		pDbgSys->bNoOutputUart  = TRUE;
		switch (pInit->uiUartIdx) {
		case 0:
			pDbgSys->fpUartPutString = uart_putString;
			break;
		default:
			pDbgSys->fpUartPutString = NULL;
			break;

		}

		for (i = 0; i < CFG_DBGSYS_MSG_PAYLOAD_NUM; i++) {
			MSDCNVT_DBGSYS_UNIT *pUnit = &pDbgSys->Queue[i];
			pUnit->pMsg = pMemPool;
			pMemPool += pDbgSys->uiPayloadSize;
			pMemPool_pa += pDbgSys->uiPayloadSize;
			nMemSize -= pDbgSys->uiPayloadSize;
		}

		pDbgSys->bInit = TRUE;
#else
		DBG_WRN("not support dbgsys on linux system.\n");
#endif
	}

#if !defined(_NETWORK_ON_CPU1_)
	vos_task_resume(MSDCNVT_TSK_IPC_ID);
#endif

	pCtrl->uiInitKey = CFG_MSDCNVT_INIT_KEY;
	return TRUE;
}

BOOL MsdcNvt_Exit(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &pCtrl->tDbgSys;

	if (pDbgSys->bInit == TRUE) {
#if 0 //MSDCNVT_TODO
		debug_set_console(CON_1);
#endif
		if (xMsdcNvt_GetCtrl()->fpEvent) {
			xMsdcNvt_GetCtrl()->fpEvent(MSDCNVT_EVENT_HOOK_DBG_MSG_OFF);
		}
		if (pCtrl->tIPC.bNetRunning) {
			pCtrl->tIPC.pCfg->stop_server = 1;
		} else {
#if 0 //MSDCNVT_TODO
			if (NvtIPC_MsgRel(pCtrl->tIPC.ipc_msgid) < 0) {
				DBG_ERR("NvtIPC_MsgRel.\r\n");
			}
#endif
			pCtrl->tIPC.ipc_msgid = 0;
		}

#if !defined(_NETWORK_ON_CPU1_)
		FLGPTN Flag;
		wai_flg(&Flag, pCtrl->FlagID, FLGMSDCNVT_IPC_STOPPED, TWF_ORW | TWF_CLR);
#endif
		pDbgSys->bInit = FALSE;
	}

	MsdcNvt_UnInstallID();

	pCtrl->uiInitKey = 0;
	return TRUE;
}

BOOL MsdcNvt_Net(BOOL bTurnOn)
{
	INT32 nRetry = 100;
	MSDCNVT_IPC_CTRL *pIpc = &xMsdcNvt_GetCtrl()->tIPC;
	if (bTurnOn) {
		if (pIpc->bNetRunning) {
			DBG_WRN("cannot turn on twice.\r\n");
			return FALSE;
		}

		pIpc->pCfg->stop_server = 0;
		snprintf(pIpc->CmdLine, sizeof(pIpc->CmdLine) - 1, "msdcnvt 0x%08X 0x%08X &", (unsigned int)MSDCNVT_PHY_ADDR((UINT32)pIpc->pCfg), sizeof(MSDCNVT_IPC_CFG));
#if defined(_NETWORK_ON_CPU1_)
#if 0 //MSDCNVT_TODO
		msdcnvt_ecos_main(pIpc->CmdLine);
#endif
#else
		NVTIPC_SYS_MSG  sysMsg;
		sysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
		sysMsg.DataAddr = (UINT32)&pIpc->CmdLine[0];
		sysMsg.DataSize = strlen(pIpc->CmdLine) + 1;
		if (NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, NVTIPC_SENDTO_CORE2, &sysMsg, sizeof(sysMsg)) < 0) {
			DBG_ERR("Failed to NVTIPC_SYS_QUEUE_ID\r\n");
		}
#endif
		while (!pIpc->bNetRunning && nRetry-- > 0) {
			vos_util_delay_ms(10);
		}
		if (!pIpc->bNetRunning) {
			DBG_ERR("Failed to start MSDC-NET\r\n");
		}

	} else {
		if (pIpc->bNetRunning) {
			pIpc->pCfg->stop_server = 1;
			while (pIpc->bNetRunning && nRetry-- > 0) {
				vos_util_delay_ms(10);
			}
			if (pIpc->bNetRunning) {
				DBG_ERR("Failed to close MSDC-NET\r\n");
			}
		}
	}
	return TRUE;
}

//Check Parameters, return NULL indicate error parameters
void *MsdcNvt_ChkParameters(void *pData, UINT32 uiSize)
{
	tMSDCEXT_PARENT *pDesc = (tMSDCEXT_PARENT *)pData;
	if (pDesc->biSize != uiSize) {
		DBG_ERR("MsdcNvtCallback: Data Size Wrong, %d(Host) != %d(Device)\r\n", (int)(pDesc->biSize), (int)(uiSize));
		pDesc->bHandled = TRUE;
		pDesc->bOK = FALSE;
		return NULL;
	}
	return pData;
}

BOOL MsdcNvt_RegHandler(MSDCNVT_HANDLER_TYPE Type, void *pCb)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	switch (Type) {
	case MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP:
		pCtrl->tHandler.fpVirtualMemMap = pCb;
		break;
	default:
		DBG_ERR("unknown typ:%d\r\n", (int)(Type));
		break;
	}
	return TRUE;
}

BOOL MsdcNvt_SetMsdcObj(MSDC_OBJ *pMsdc)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	pCtrl->pMsdc = pMsdc;
	return TRUE;
}

MSDC_OBJ *MsdcNvt_GetMsdcObj(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	return pCtrl->pMsdc;
}

static BOOL null_lun_DetStrgCard(void)
{
	return FALSE;
}
static BOOL null_lun_DetStrgCardWP(void)
{
	return TRUE;
}

static ER null_lun_lock(void)
{
	return E_OK;
}
static ER null_lun_unlock(void)
{
	return E_OK;
}
static ER null_lun_open(void)
{
	return E_OK;
}
static ER null_lun_writeSectors(INT8 *pcBuf, UINT32 ulLBAddr, UINT32 ulSctCnt)
{
	return E_OK;
}
static ER null_lun_readSectors(INT8 *pcBuf, UINT32 ulLBAddr, UINT32 ulSctCnt)
{
	return E_OK;
}
static ER null_lun_close(void)
{
	return E_OK;
}
static ER null_lun_openMemBus(void)
{
	return E_OK;
}
static ER null_lun_closeMemBus(void)
{
	return E_OK;
}
static ER null_lun_fwSetParam(STRG_SET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	return E_OK;
}
static ER null_lun_fwGetParam(STRG_GET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	return E_OK;
}
static ER null_lun_extIOCtrl(STRG_EXT_CMD uiCmd, UINT32 param1, UINT32 param2)
{
	return E_OK;
}

static STORAGE_OBJ NullLunObj = {0, 0, null_lun_lock, null_lun_unlock, null_lun_open, null_lun_writeSectors, null_lun_readSectors, null_lun_close, null_lun_openMemBus, null_lun_closeMemBus, null_lun_fwSetParam, null_lun_fwGetParam, null_lun_extIOCtrl};


MSDCNVT_LUN *MsdcNvt_GetNullLun(void)
{
	m_NullLun.pStrg = &NullLunObj;
	m_NullLun.Type = MSDC_STRG;
	m_NullLun.fpStrgDetCb = null_lun_DetStrgCard;
	m_NullLun.fpStrgLockCb = null_lun_DetStrgCardWP;
	m_NullLun.uiLUNs = 1;
	return &m_NullLun;
}

static BOOL xMsdcNvt_CalcAccessAddressWithSize(void)
{
	UINT32   CDB_cmdid;
	UINT32   CDB_DataXferLen, CBW_DataXferLen;
	MSDCNVT_HOST_INFO *pHost = &xMsdcNvt_GetCtrl()->tHostInfo;

	CDB_cmdid = pHost->pCBW->CBWCB[0];
	CBW_DataXferLen = pHost->pCBW->dCBWDataTransferLength;

	pHost->uiLB_Address =  pHost->pCBW->CBWCB[2] << 24;
	pHost->uiLB_Address |= pHost->pCBW->CBWCB[3] << 16;
	pHost->uiLB_Address |= pHost->pCBW->CBWCB[4] << 8;
	pHost->uiLB_Address |= pHost->pCBW->CBWCB[5];

	pHost->uiLB_Length =  pHost->pCBW->CBWCB[6] << 24;
	pHost->uiLB_Length |= pHost->pCBW->CBWCB[7] << 16;
	pHost->uiLB_Length |= pHost->pCBW->CBWCB[8] << 8;
	pHost->uiLB_Length |= pHost->pCBW->CBWCB[9];

	CDB_DataXferLen = pHost->uiLB_Length;

	//Check DWORD Alignment with Starting Address
	if (pHost->uiLB_Address & 0x3) {
		return FALSE;
	}
	//Case(1) : Hn = Dn
	else if ((CBW_DataXferLen == 0) && (CDB_DataXferLen == 0)) {
		return TRUE;
	}
	// case(2): Hn < Di , case(3): Hn < Do
	else if ((CBW_DataXferLen == 0) && (CDB_DataXferLen > 0)) {
		return FALSE;
	}
	// case(8): Hi <> Do
	else if ((CDB_cmdid == SBC_CMD_MSDCNVT_WRITE) && ((pHost->pCBW->bmCBWFlags & SBC_CMD_DIR_MASK) != SBC_CMD_DIR_OUT)) {
		return FALSE;
	}
	// case(10): Ho <> Di
	else if ((CDB_cmdid == SBC_CMD_MSDCNVT_READ)  && ((pHost->pCBW->bmCBWFlags & SBC_CMD_DIR_MASK) != SBC_CMD_DIR_IN)) {
		return FALSE;
	}
	// case(4): Hi>Dn, case(5): hi>Di, case(11): Ho>Do
	else if (CBW_DataXferLen > CDB_DataXferLen) {
		return FALSE;
	}
	// case(7): Hi<Di, case(13): Ho < Do
	else if (CBW_DataXferLen < CDB_DataXferLen) {
		return FALSE;
	}

	return TRUE;
}

#if !defined(__FREERTOS)
#include <sys/mman.h>
void* mem_mmap(int fd, unsigned int mapped_size, off_t phy_addr)
{
	void *map_base = NULL;
	unsigned page_size = 0;

	page_size = getpagesize();
	map_base = mmap(NULL,
			mapped_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			phy_addr & ~(off_t)(page_size - 1));

	if (map_base == MAP_FAILED)
		return NULL;

	return map_base;
}

int mem_munmap(void* map_base, unsigned int mapped_size)
{
	if (munmap(map_base, mapped_size) == -1)
		return -1;

	return 0;
}
#endif

static void xMsdcNvt_RegisterAccess(UINT32 *pDst, UINT32 *pSrc, UINT32 nDWORDs)
{
#if defined(__FREERTOS)
	while (nDWORDs--) {
		*(pDst++) = *(pSrc++);
	}
#else
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if ((UINT32)pSrc > CFG_REGISTER_BEGIN_ADDR) {
		UINT32 aligned_pa = ALIGN_FLOOR((UINT32)pSrc, 4096);
		UINT32 aligned_size = ALIGN_CEIL(nDWORDs*sizeof(UINT32), 4096*2); //2 page align for address front and rear align
		UINT32 ofs = (UINT32)pSrc - aligned_pa;
		unsigned char *p_mem = (unsigned char *)mem_mmap(fd, aligned_size, aligned_pa);
		pSrc = (UINT32 *)(p_mem+ofs);
		while (nDWORDs--) {
			*(pDst++) = *(pSrc++);
		}
		mem_munmap(p_mem, aligned_size);
	} else {
		UINT32 aligned_pa = ALIGN_FLOOR((UINT32)pDst, 4096);
		UINT32 aligned_size = ALIGN_CEIL(nDWORDs*sizeof(UINT32), 4096*2); //2 page align for address front and rear align
		UINT32 ofs = (UINT32)pDst - aligned_pa;
		unsigned char *p_mem = (unsigned char *)mem_mmap(fd, aligned_size, aligned_pa);
		pDst = (UINT32 *)(p_mem+ofs);
		while (nDWORDs--) {
			*(pDst++) = *(pSrc++);
		}
		mem_munmap(p_mem, aligned_size);
	}
	close(fd);
#endif
}

static BOOL xMsdcNvt_Function(void)
{
	MSDCNVT_HOST_INFO *pHost = &xMsdcNvt_GetCtrl()->tHostInfo;

	switch (pHost->pCBW->CBWCB[10]) {
	case CDB_10_FUNCTION_DBGSYS: //!< DBGSYS Needs to High Priority
		xMsdcNvt_Function_DbgSys();
		break;
	case CDB_10_FUNCTION_BI_CALL:
		xMsdcNvt_Function_BiCall();
		break;
	case CDB_10_FUNCTION_SI_CALL:
		xMsdcNvt_Function_SiCall();
		break;
	case CDB_10_FUNCTION_MISC:
		xMsdcNvt_Function_Misc();
		break;
	default:
		DBG_ERR("Not Handled ID: %d\r\n", (int)(pHost->pCBW->CBWCB[10]));
	}
	return TRUE;
}

