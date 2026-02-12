#include "MsdcNvtInt.h"
#include <string.h>

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_BI_CALL Message
//------------------------------------------------------------------------------
#define CDB_11_BI_CALL_UNKNOWN                  0x00 ///< Invalid Cmd
#define CDB_11_BI_CALL_GET_PROC_STEP0           0x01 ///< Get Function Proc Address (PC Send Function String To F.W)
#define CDB_11_BI_CALL_GET_PROC_STEP1           0x02 ///< Get Function Proc Address (PC Query, Then F.W Return Function Point)
#define CDB_11_BI_CALL_CALL                     0x03 ///< Call Function,
#define CDB_11_BI_CALL_GET_DATA                 0x04 ///< Get  Result Data after Finish Function

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_BI_CALL Handler
//------------------------------------------------------------------------------
static void xMsdcNvt_BiCall_GetProc_Step0(void *pInfo); ///< PC Query Function Name
static void xMsdcNvt_BiCall_GetProc_Step1(void *pInfo); ///< Return Function Point Address to PC
static void xMsdcNvt_BiCall_Call(void *pInfo);          ///< Recevice PC Data & Call Function (Bi-Direction Step 1)
static void xMsdcNvt_BiCall_GetData(void *pInfo);       ///< Send Result Data to PC (Bi-Direction Step 2)
static void xMsdcNvt_BiCall_RunCmd(void);               ///< Callback for Background Thread Run Command

//Local Variables
static MSDCNVT_FUNCTION_CMD_HANDLE CallMap[] = {
	{CDB_11_BI_CALL_UNKNOWN, NULL},
	{CDB_11_BI_CALL_GET_PROC_STEP0, xMsdcNvt_BiCall_GetProc_Step0},
	{CDB_11_BI_CALL_GET_PROC_STEP1, xMsdcNvt_BiCall_GetProc_Step1},
	{CDB_11_BI_CALL_CALL, xMsdcNvt_BiCall_Call},
	{CDB_11_BI_CALL_GET_DATA, xMsdcNvt_BiCall_GetData},
};

void xMsdcNvt_Function_BiCall(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;

	UINT8 iCmd = pHost->pCBW->CBWCB[11];
	void *pInfo = pHost->pPoolMem;

	if (iCmd < sizeof(CallMap) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& iCmd == CallMap[iCmd].uiCall
		&& CallMap[iCmd].fpCall != NULL) {
		CallMap[iCmd].fpCall(pInfo);
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}

static void xMsdcNvt_BiCall_GetProc_Step0(void *pInfo)
{
	xMsdcNvt_MemPushHostToBk(); //Push Name to Bk
}

static void xMsdcNvt_BiCall_GetProc_Step1(void *pInfo)
{
	BOOL     bFind = FALSE;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_BI_CALL_CTRL *pBiCtrl = &pCtrl->tBiCall;
	MSDCNVT_BACKGROUND_CTRL *pBk = &pCtrl->tBkCtrl;
	char    *szName = (char *)pBk->pPoolMem;
	UINT32  *pProcAddress = (UINT32 *)pHost->pPoolMem;
	MSDCNVT_BI_CALL_UNIT *pCurr = pBiCtrl->pHead;

	while (pCurr != NULL && pCurr->fp != NULL) {
		if (strcmp(szName, pCurr->szName) == 0) {
			bFind = TRUE;
			*pProcAddress = (UINT32)pCurr->fp;
			break;
		}

		pCurr++;
		if (pCurr->szName == NULL
			&& pCurr->fp != NULL) {
			pCurr = (MSDCNVT_BI_CALL_UNIT *)pCurr->fp;
		}
	}

	if (!bFind) {
		DBG_ERR("No Found: %s\r\n", szName);
		*pProcAddress = 0;
	}
	return;
}

static void xMsdcNvt_BiCall_Call(void *pInfo)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_BI_CALL_CTRL *pBiCtrl = &pCtrl->tBiCall;

	pBiCtrl->fpCall = (void (*)(void *))pHost->uiLB_Address;

	if (pBiCtrl->fpCall) {
		xMsdcNvt_MemPushHostToBk();
		xMsdcNvt_Bk_RunCmd(xMsdcNvt_BiCall_RunCmd);
	} else {
		DBG_ERR("gCtrl.fpCall==%d\r\n", (int)(pBiCtrl->fpCall));
	}
}

static void xMsdcNvt_BiCall_GetData(void *pInfo)
{
	MSDCNVT_BI_CALL_CTRL *pBiCtrl = &xMsdcNvt_GetCtrl()->tBiCall;

	if (pBiCtrl->fpCall == NULL) {
		DBG_ERR("gCtrl.fpCall==%d\r\n", (int)(pBiCtrl->fpCall));
	} else {
		xMsdcNvt_MemPopBkToHost();
	}
	return;
}

static void xMsdcNvt_BiCall_RunCmd(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_BACKGROUND_CTRL *pBk = &pCtrl->tBkCtrl;
	MSDCNVT_BI_CALL_CTRL *pBiCtrl = &pCtrl->tBiCall;
	pBiCtrl->fpCall(pBk->pPoolMem);
}