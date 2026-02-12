#include "MsdcNvtInt.h"

//Used CMD Field to be Command Type
#define CDB_IDX_CALL_ID         12   //!< Use CDB[12] to be Function ID

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_SI_CALL Message
//------------------------------------------------------------------------------
#define CDB_11_SI_CALL_UNKNOWN                  0x00 //!< Invalid Cmd
#define CDB_11_SI_CALL_PC_GET_RUN_CMD           0x01 //!< Single Direction,from PC-Get Step 0, Use CDB[12] to be Function ID
#define CDB_11_SI_CALL_PC_GET_GET_RESULT        0x02 //!< Single Direction,from PC-Get Step 1, Use CDB[12] to be Function ID
#define CDB_11_SI_CALL_PC_SET                   0x03 //!< Single Direction,from PC-Set, Use CDB[12] to be Function ID

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_SI_CALL Handler
//------------------------------------------------------------------------------
static void xMsdcNvt_SiCall_Single_Get_RunCmd(void *pInfo);      //!< Send Data to PC (Single Direction)
static void xMsdcNvt_SiCall_Single_Get_GetResult(void *pInfo);   //!< Send Data to PC (Single Direction)
static void xMsdcNvt_SiCall_Single_Set(void *pInfo);             //!< Recevice PC (Single Direction)
static void xMsdcNvt_SiCall_RunCmdGet(void);                     //!< Callback for Background Thread Run Command for Get
static void xMsdcNvt_SiCall_RunCmdSet(void);                     //!< Callback for Background Thread Run Command for Set

//Local Variables
static MSDCNVT_FUNCTION_CMD_HANDLE CallMap[] = {
	{CDB_11_SI_CALL_UNKNOWN, NULL},
	{CDB_11_SI_CALL_PC_GET_RUN_CMD,     xMsdcNvt_SiCall_Single_Get_RunCmd},    //Get Step 0: Run Command
	{CDB_11_SI_CALL_PC_GET_GET_RESULT,  xMsdcNvt_SiCall_Single_Get_GetResult}, //Get Setp 1: Get Result Data
	{CDB_11_SI_CALL_PC_SET,             xMsdcNvt_SiCall_Single_Set},
};

void xMsdcNvt_Function_SiCall(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	UINT8 iCmd = pHost->pCBW->CBWCB[11];

	if (iCmd < sizeof(CallMap) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& iCmd == CallMap[iCmd].uiCall
		&& CallMap[iCmd].fpCall != NULL) {
		CallMap[iCmd].fpCall(NULL); //pInfo is not used
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}

static void xMsdcNvt_SiCall_Single_Get_RunCmd(void *pInfo)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_SI_CALL_CTRL *pSiCtrl = &pCtrl->tSiCall;

	pSiCtrl->uiCall = pHost->pCBW->CBWCB[CDB_IDX_CALL_ID];
	xMsdcNvt_MemPushHostToBk();
	xMsdcNvt_Bk_RunCmd(xMsdcNvt_SiCall_RunCmdGet);
}

static void xMsdcNvt_SiCall_Single_Get_GetResult(void *pInfo)
{
	xMsdcNvt_MemPopBkToHost();
}

static void xMsdcNvt_SiCall_Single_Set(void *pInfo)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_SI_CALL_CTRL *pSiCtrl = &pCtrl->tSiCall;

	pSiCtrl->uiCall = pHost->pCBW->CBWCB[CDB_IDX_CALL_ID];
	xMsdcNvt_MemPushHostToBk();
	xMsdcNvt_Bk_RunCmd(xMsdcNvt_SiCall_RunCmdSet);
	xMsdcNvt_MemPopBkToHost();
}

static void xMsdcNvt_SiCall_RunCmdGet(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_SI_CALL_CTRL *pSiCtrl = &pCtrl->tSiCall;

	UINT32 uiCall = pSiCtrl->uiCall;

	if (uiCall >= pSiCtrl->uiGets) {
		DBG_ERR("function index out of range\r\n");
		return;
	}

	pSiCtrl->fpTblGet[uiCall]();
}

static void xMsdcNvt_SiCall_RunCmdSet(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_SI_CALL_CTRL *pSiCtrl = &pCtrl->tSiCall;

	UINT32 uiCall = pSiCtrl->uiCall;

	if (uiCall >= pSiCtrl->uiSets) {
		DBG_ERR("function index out of range\r\n");
		return;
	}

	pSiCtrl->fpTblSet[uiCall]();
}