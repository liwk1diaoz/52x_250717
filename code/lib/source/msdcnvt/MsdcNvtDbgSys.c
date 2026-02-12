#include "MsdcNvtInt.h"

#define CFG_MAX_COMMAND_LINE_BYTES              64   //!< Command Task Line Max Lens

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_DBGSYS Message
//------------------------------------------------------------------------------
#define CDB_11_DBGSYS_UNKNOWN                   0x00 //!< Invalid Cmd
#define CDB_11_DBGSYS_OPEN                      0x01 //!< Stopping Command Tsk and Hooking debug_msg
#define CDB_11_DBGSYS_CLOSE                     0x02 //!< Stopping DbgSys
#define CDB_11_DBGSYS_QUERY_MSG                 0x03 //!< Query Msg is Existing in Buffer Queue
#define CDB_11_DBGSYS_GET_MSG                   0x04 //!< Getting Msg from Buffer Queue
#define CDB_11_DBGSYS_SEND_CMD                  0x05 //!< Sending Command from PC -> RTOS
#define CDB_11_DBGSYS_PROPERTY_SET              0x06 //!< Set Some Property from PC
#define CDB_11_DBGSYS_PROPERTY_GET              0x07 //!< Get Some Property from PC

//------------------------------------------------------------------------------
// CDB_11_DBGSYS_SET_PROPERTY / CDB_11_DBGSYS_GET_PROPERTY Message
//------------------------------------------------------------------------------
#define CDB_12_DBGSYS_PROPERTY_UNKNOWN          0x00 //!< Invalid Type
#define CDB_12_DBGSYS_ALSO_OUTPUT_UART          0x01 //!< Also Output Uart Msg (ENABLE / DISABLE)

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_DBGSYS Data Type
//------------------------------------------------------------------------------
//Parent for Host <- Device
typedef struct _DBGSYS_RESPONSE {
	UINT32 biSize;      //!< Structure Size
	UINT32 bHandled;    //!< Indicate Device Handle this function (Don't Use BOOL, for 433 Compatiable)
	UINT32 bOK;         //!< Indicate Function Action is OK or Not (Don't Use BOOL, for 433 Compatiable)
	UINT8  Reversed[2]; //!< Reversed Value for Structure DWORD Alignment
} tDBGSYS_RESPONSE;

//Parent for Host -> PC
typedef struct _DBGSYS_DELIVER {
	UINT32 biSize;      //!< Structure Size
} tDBGSYS_DELIVER;

typedef struct _DBGSYS_MSG_QUERY {
	tDBGSYS_RESPONSE tResponse;//!< Parent
	UINT32           nByteMsg; //!< nBytes Msg Need to be get
} tDBGSYS_MSG_QUERY;

typedef struct _DBGSYS_MSG_GET {
	tDBGSYS_RESPONSE tResponse;//!< Parent
	UINT32           nByteMsg; //!< nBytes Msg in following buffer
} tDBGSYS_MSG_GET;

typedef struct _DBGSYS_CMD_SEND {
	tDBGSYS_DELIVER  tDeliver; //!< Parent
	UINT32           nBytesCmd;//!< Following Data Size
} tDBGSYS_CMD_SEND;

typedef struct _DBGSYS_PROPERTY_GET {
	tDBGSYS_RESPONSE tResponse;//!< Parent
	UINT32           Type;     //!< Property Type
	UINT32           Value;    //!< Property Value
} tDBGSYS_PROPERTY_GET;

typedef struct _DBGSYS_PROPERTY_SET {
	tDBGSYS_DELIVER  tDeliver; //!< Parent
	UINT32           Type;     //!< Property Type
	UINT32           Value;    //!< Property Value
} tDBGSYS_PROPERTY_SET;


//------------------------------------------------------------------------------
// CDB_10_FUNCTION_DBGSYS Handler
//------------------------------------------------------------------------------
static ER   DbgSys_Lock(void);
static ER   DbgSys_UnLock(void);
static void DbgSys_Open(void *pInfo);
static void DbgSys_Close(void *pInfo);
static void DbgSys_MsgQuery(void *pInfo);       //!< Query Is There Existing Msg Needs to Flush
static void DbgSys_MsgGet(void *pInfo);         //!< Get Buffer Address and Length
static void DbgSys_CmdSend(void *pInfo);        //!< Send Command Line To CommandTsk
static void DbgSys_PropertySet(void *pInfo);    //!< Set Some DbgSys Property
static void DbgSys_PropertyGet(void *pInfo);    //!< Get Some DbgSys Property

//Provide Callback functions
static ER   DbgSys_MsgReceive(CHAR *pMsg);              //!< Callback for receive message

static void DbgSys_RunCmd(void);                //!< Callback for Background Thread Run Command

static void DbgSys_Hook(void);
static void DbgSys_Unhook(void);
static void DbgSys_Puts(const char *s);
static int  DbgSys_Gets(char *s, int len);

static MSDCNVT_FUNCTION_CMD_HANDLE CallMap[] = {
	{CDB_11_DBGSYS_UNKNOWN, NULL},
	{CDB_11_DBGSYS_OPEN, DbgSys_Open},
	{CDB_11_DBGSYS_CLOSE, DbgSys_Close},
	{CDB_11_DBGSYS_QUERY_MSG, DbgSys_MsgQuery},
	{CDB_11_DBGSYS_GET_MSG, DbgSys_MsgGet},
	{CDB_11_DBGSYS_SEND_CMD, DbgSys_CmdSend},
	{CDB_11_DBGSYS_PROPERTY_SET, DbgSys_PropertySet},
	{CDB_11_DBGSYS_PROPERTY_GET, DbgSys_PropertyGet},
};

static CONSOLE CON_USB = {DbgSys_Hook, DbgSys_Unhook, DbgSys_Puts, DbgSys_Gets};

// stub functions for linux
#if (!CFG_DBGSYS)
int console_set_curr(CONSOLE *p_console)
{
	return 0;
}

int console_get_curr(CONSOLE *p_console)
{
	return 0;
}
#endif
//Debug System Vendor Command
void xMsdcNvt_Function_DbgSys(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	UINT8 iCmd = pHost->pCBW->CBWCB[11];
	void *pInfo = pHost->pPoolMem;

	if (!pCtrl->tDbgSys.bInit) {
		DBG_ERR("has not been initial\r\n");
		return;
	}

	if (iCmd < sizeof(CallMap) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& iCmd == CallMap[iCmd].uiCall
		&& CallMap[iCmd].fpCall != NULL) {
		CallMap[iCmd].fpCall(pInfo);
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}
static ER DbgSys_Lock(void)
{
	return vos_sem_wait(xMsdcNvt_GetCtrl()->tDbgSys.SemID);
}

static ER DbgSys_UnLock(void)
{
	vos_sem_sig(xMsdcNvt_GetCtrl()->tDbgSys.SemID);
	return 0;
}

static void DbgSys_Hook(void)
{
	clr_flg(xMsdcNvt_GetCtrl()->FlagID, FLGDBGSYS_CMD_ABORT);
}

static void DbgSys_Unhook(void)
{
	set_flg(xMsdcNvt_GetCtrl()->FlagID, FLGDBGSYS_CMD_ABORT);
}

static void DbgSys_Puts(const char *s)
{
	DbgSys_MsgReceive((CHAR *)s);
}

int DbgSys_Gets(char *s, int len)
{
	FLGPTN FlgPtn = 0;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_BACKGROUND_CTRL *pBk = &pCtrl->tBkCtrl;

	set_flg(pCtrl->FlagID, FLGDBGSYS_CMD_FINISH);
	wai_flg(&FlgPtn, pCtrl->FlagID, FLGDBGSYS_CMD_ABORT | FLGDBGSYS_CMD_ARRIVAL, TWF_ORW | TWF_CLR);

	if (FlgPtn & FLGDBGSYS_CMD_ABORT) {
		//we have to keep ABORT Status for blocking in DbgSys_Gets(). Only DbgSys_Unhook can clean it
		set_flg(pCtrl->FlagID, FLGDBGSYS_CMD_ABORT);
		s[0] = 0;
		return 0;
	} else if (FlgPtn & FLGDBGSYS_CMD_ARRIVAL) {
		if (*(char *)pBk->pPoolMem == 0) {
			s[0] = 0;
			printf("\n"); //for telent change line
		} else {
			strncpy(s, (char *)pBk->pPoolMem, len - 1);
		}
		return strlen(s);
	} else

	return 0;
}

static void DbgSys_Open(void *pInfo)
{
	UINT32 i;
	tDBGSYS_RESPONSE *pResponse = (tDBGSYS_RESPONSE *)pInfo;
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &xMsdcNvt_GetCtrl()->tDbgSys;

	memset(pResponse, 0, sizeof(tDBGSYS_RESPONSE));
	pResponse->biSize = sizeof(tDBGSYS_RESPONSE);
	pResponse->bHandled = TRUE;

	DbgSys_Lock();

	pDbgSys->uiMsgIn = pDbgSys->uiMsgOut = 0;

	for (i = 0; i < pDbgSys->uiPayloadNum; i++) {
		pDbgSys->Queue[i].uiUsedBytes = 0;
	}

	console_get_curr(&pDbgSys->DefaultConsole);
	console_set_curr(&CON_USB);

	DbgSys_UnLock();

	if (xMsdcNvt_GetCtrl()->fpEvent) {
		xMsdcNvt_GetCtrl()->fpEvent(MSDCNVT_EVENT_HOOK_DBG_MSG_ON);
	}

	pResponse->bOK = TRUE;
}

static void DbgSys_Close(void *pInfo)
{
	tDBGSYS_RESPONSE *pResponse = (tDBGSYS_RESPONSE *)pInfo;
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &xMsdcNvt_GetCtrl()->tDbgSys;

	memset(pResponse, 0, sizeof(tDBGSYS_RESPONSE));
	pResponse->biSize = sizeof(tDBGSYS_RESPONSE);
	pResponse->bHandled = TRUE;

	DbgSys_Lock();

	console_set_curr(&pDbgSys->DefaultConsole);

	DbgSys_UnLock();

	if (xMsdcNvt_GetCtrl()->fpEvent) {
		xMsdcNvt_GetCtrl()->fpEvent(MSDCNVT_EVENT_HOOK_DBG_MSG_OFF);
	}

	pResponse->bOK = TRUE;
}

static void DbgSys_MsgQuery(void *pInfo)
{
	tDBGSYS_MSG_QUERY *pMsgQuery = (tDBGSYS_MSG_QUERY *)pInfo;
	tDBGSYS_RESPONSE  *pResponse = (tDBGSYS_RESPONSE *)pInfo;
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &xMsdcNvt_GetCtrl()->tDbgSys;

	memset(pMsgQuery, 0, sizeof(tDBGSYS_MSG_QUERY));
	pResponse->biSize = sizeof(tDBGSYS_MSG_QUERY);
	pResponse->bHandled = TRUE;

	pMsgQuery->nByteMsg = pDbgSys->Queue[pDbgSys->uiMsgOut].uiUsedBytes;

	pResponse->bOK = TRUE;
}

static void DbgSys_MsgGet(void *pInfo)
{
	tDBGSYS_MSG_GET *pMsgGet = (tDBGSYS_MSG_GET *)pInfo;
	UINT8 *pDst = (UINT8 *)pMsgGet + sizeof(tDBGSYS_MSG_GET);
	tDBGSYS_RESPONSE *pResponse = &pMsgGet->tResponse;
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &xMsdcNvt_GetCtrl()->tDbgSys;

	memset(pMsgGet, 0, sizeof(tDBGSYS_MSG_GET));
	pResponse->biSize = sizeof(tDBGSYS_MSG_GET);
	pResponse->bHandled = TRUE;

	if (pDbgSys->Queue[pDbgSys->uiMsgOut].uiUsedBytes == 0) {
		*pDst = 0;
		pMsgGet->nByteMsg = 0;
		pResponse->bOK = FALSE;
		return;
	}

	pMsgGet->nByteMsg = pDbgSys->Queue[pDbgSys->uiMsgOut].uiUsedBytes;
	memcpy(pDst, pDbgSys->Queue[pDbgSys->uiMsgOut].pMsg, pMsgGet->nByteMsg);

	DbgSys_Lock();
	pDbgSys->Queue[pDbgSys->uiMsgOut].uiUsedBytes = 0;
	pDbgSys->uiMsgOut = (pDbgSys->uiMsgOut + 1)&pDbgSys->uiMsgCntMask;
	DbgSys_UnLock();

	pResponse->bOK = TRUE;
}

static void DbgSys_CmdSend(void *pInfo)
{
	MSDCNVT_BACKGROUND_CTRL *pBk = &xMsdcNvt_GetCtrl()->tBkCtrl;
	tDBGSYS_CMD_SEND *pCmdSend = (tDBGSYS_CMD_SEND *)pInfo;

	UINT32 i;
	UINT8 *pCmdLine = (UINT8 *)pCmdSend + sizeof(tDBGSYS_CMD_SEND);

	if (pCmdSend->tDeliver.biSize != sizeof(tDBGSYS_CMD_SEND)) {
		DBG_ERR("Not Handled\r\n");
		return;
	}

	pBk->uiTransmitSize = CFG_MAX_COMMAND_LINE_BYTES;
	memcpy(pBk->pPoolMem, pCmdLine, pBk->uiTransmitSize);

	for (i = 0; i < CFG_MAX_COMMAND_LINE_BYTES; i++) {
		pBk->pPoolMem[i] = pCmdLine[i];
		if (pBk->pPoolMem[i] == 0x0D) {
			pBk->pPoolMem[i] = 0;
			break;
		}
	}

	xMsdcNvt_Bk_RunCmd(DbgSys_RunCmd);
}

static void DbgSys_RunCmd(void)
{
	FLGPTN FlgPtn;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();

	clr_flg(pCtrl->FlagID, FLGDBGSYS_CMD_FINISH);
	set_flg(pCtrl->FlagID, FLGDBGSYS_CMD_ARRIVAL);
	//wait DoCommand finish
	wai_flg(&FlgPtn, pCtrl->FlagID, FLGDBGSYS_CMD_FINISH, TWF_ORW | TWF_CLR);
}

static void DbgSys_PropertySet(void *pInfo)
{
	tDBGSYS_PROPERTY_SET *pProperty = (tDBGSYS_PROPERTY_SET *)pInfo;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &pCtrl->tDbgSys;
	UINT32 iType = pHost->pCBW->CBWCB[12];

	if (pProperty->tDeliver.biSize != sizeof(tDBGSYS_PROPERTY_SET)) {
		DBG_ERR("Not Handled\r\n");
		return;
	}

	switch (iType) {
	case CDB_12_DBGSYS_ALSO_OUTPUT_UART:
		pDbgSys->bNoOutputUart = (pProperty->Value) ? FALSE : TRUE;
		break;
	}
}

static void DbgSys_PropertyGet(void *pInfo)
{
	tDBGSYS_PROPERTY_GET *pProperty = (tDBGSYS_PROPERTY_GET *)pInfo;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &pCtrl->tDbgSys;
	UINT32 iType = pHost->pCBW->CBWCB[12];

	if (pProperty->tResponse.biSize != sizeof(tDBGSYS_PROPERTY_GET)) {
		DBG_ERR("Not Handled\r\n");
		return;
	}

	switch (iType) {
	case CDB_12_DBGSYS_ALSO_OUTPUT_UART:
		pProperty->Type = CDB_12_DBGSYS_ALSO_OUTPUT_UART;
		pProperty->Value = (pDbgSys->bNoOutputUart) ? FALSE : TRUE;
		break;
	}
}

static ER DbgSys_MsgReceive(CHAR *pMsg)
{
	UINT32 nCnt;
	CHAR *pDst;
	CHAR *pSrc = pMsg;
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_DBGSYS_CTRL *pDbgSys = &pCtrl->tDbgSys;
	UINT32  nMaxLen = pDbgSys->uiPayloadSize - 1;

	pDst = (CHAR *)pDbgSys->Queue[pDbgSys->uiMsgIn].pMsg;

	nCnt = 0;
	while ((*pMsg) != 0 && nMaxLen--) {
		*(pDst++) = *(pMsg++);
		nCnt++;
	}

	*pDst = 0;
	nCnt++;

	DbgSys_Lock();
	pDbgSys->Queue[pDbgSys->uiMsgIn].uiUsedBytes = nCnt;
	pDbgSys->uiMsgIn = (pDbgSys->uiMsgIn + 1)&pDbgSys->uiMsgCntMask;
	DbgSys_UnLock();

	if (!pDbgSys->bNoOutputUart) {
		pDbgSys->fpUartPutString(pSrc);
	}

	//Check Next Payload is Empty
	nCnt = pDbgSys->Queue[pDbgSys->uiMsgIn].uiUsedBytes;
	if (pDbgSys->uiMsgIn == pDbgSys->uiMsgOut && nCnt != 0) {
		INT32 nRetry = 500; //Retry 5 sec for Crash Detect
		//Wait to Free Payload by PC Getting
		while (pDbgSys->uiMsgIn == pDbgSys->uiMsgOut && nRetry--) {
			vos_util_delay_ms(10);
		}

		if (nRetry < 0) {
			if (kchk_flg(pCtrl->FlagID, FLGDBGSYS_CMD_ABORT) != 0) {
				//Fix Bug for both Running telnet message and update fw then update is failed!
				console_set_curr(&pDbgSys->DefaultConsole);
				if (xMsdcNvt_GetCtrl()->fpEvent) {
					xMsdcNvt_GetCtrl()->fpEvent(MSDCNVT_EVENT_HOOK_DBG_MSG_OFF);
				}
				DBG_ERR("Closed, due to buffer not empty,pDbgSys->uiMsgIn=%d\r\n", (int)pDbgSys->uiMsgIn);
				return E_OK;
			}
		}
	}

	return E_OK;
}

