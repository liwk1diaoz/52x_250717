#include "MsdcNvtInt.h"

#define CFG_MSDC_VENDOR_NVT_VERSION  0x12011315 //Year:Month:Day:Hour
#define CFG_MSDC_VENDOR_NVT_TAG      MAKEFOURCC('N','O','V','A')

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_MISC Message
//------------------------------------------------------------------------------
#define CDB_11_MISC_UNKNOWN                     0x00 //!< Invalid Cmd
#define CDB_11_MISC_VERSION                     0x01 //!< Get Version
#define CDB_11_MISC_NOVATAG                     0x02 //!< Get Tag
#define CDB_11_MISC_BK_IS_LOCK                  0x03 //!< Query Background is Lock
#define CDB_11_MISC_BK_IS_FINISH                0x04 //!< Query Background is finish his work
#define CDB_11_MISC_BK_LOCK                     0x05 //!< Lock Background Service by Host
#define CDB_11_MISC_BK_UNLOCK                   0x06 //!< UnLock Background Service by Host
#define CDB_11_MISC_UPD_FW_GET_INFO             0x07 //!< Update Firmware (PC Get Info)
#define CDB_11_MISC_UPD_FW_SET_BLOCK            0x08 //!< Update Firmware (PC Set Block Data)

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_MISC Data Type
//------------------------------------------------------------------------------
//Parent for Host <- Device
typedef struct _MSDCNVT_MISC_RESPONSE {
	UINT32 biSize;      //!< Structure Size
	UINT32 bHandled;    //!< Indicate Device Handle this function (Don't Use BOOL, for 433 Compatiable)
	UINT32 bOK;         //!< Indicate Function Action is OK or Not (Don't Use BOOL, for 433 Compatiable)
	UINT8  Reversed[2]; //!< Reversed Value for Structure DWORD Alignment
} MSDCNVT_MISC_RESPONSE;

//Parent for Host -> PC
typedef struct _MSDCNVT_MISC_DELIVER {
	UINT32 biSize;      //!< Structure Size
} MSDCNVT_MISC_DELIVER;

//PC Get Version or NovaTag
typedef struct _MSDCNVT_MISC_GET_UINT32 {
	MSDCNVT_MISC_RESPONSE tResponse;   //!< Parent
	UINT32         Value;       //!< UINT32 Value
} MSDCNVT_MISC_GET_UINT32;

//PC Get Nand Block Information
typedef struct _MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO {
	MSDCNVT_MISC_RESPONSE tResponse;   //!< Parent
	UINT32 uiBytesPerBlock;      //!< Each Block Size
	UINT32 uiBytesTempBuf;       //!< Transmit Swap Buffer Size (inc structure size)
} MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO;

//PC Send Nand Block Data
typedef struct _MSDCNVT_MISC_UPDFW_SET_BLOCK_INFO {
	MSDCNVT_MISC_DELIVER tDeliver;     //!< Parent
	UINT32 iBlock;              //!< Block Index of this Packet
	UINT32 IsLastBlock;         //!< Indicate Last Block for Reset System
	UINT32 EffectDataSize;      //!< Current Effective Data Size
} MSDCNVT_MISC_UPDFW_SET_BLOCK_INFO;

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_MISC Handler
//------------------------------------------------------------------------------
static void xMsdcNvt_Misc_Get_Version(void *pInfo);          //!< Get MSDC Vendor Nvt Version
static void xMsdcNvt_Misc_Get_NovaTag(void *pInfo);          //!< Get 'N','O','V','A' Tag
static void xMsdcNvt_Misc_Get_Bk_IsLock(void *pInfo);        //!< Host Query Background Service is lock
static void xMsdcNvt_Misc_Get_Bk_IsFinish(void *pInfo);      //!< Host Query Background Service is Finish his work
static void xMsdcNvt_Misc_Get_Bk_Lock(void *pInfo);          //!< Host Lock Background Service
static void xMsdcNvt_Misc_Get_Bk_UnLock(void *pInfo);        //!< Host UnLock Background Service
static void xMsdcNvt_Misc_Set_UpdFw_GetInfo(void *pInfo);    //!< Update Firmware (PC Get Info)
static void xMsdcNvt_Misc_Set_UpdFw_SetBlock(void *pInfo);   //!< Update Firmware (PC Set Block Data)

//Local Variables
static MSDCNVT_FUNCTION_CMD_HANDLE CallMap[] = {
	{CDB_11_MISC_UNKNOWN, NULL},
	{CDB_11_MISC_VERSION,           xMsdcNvt_Misc_Get_Version},
	{CDB_11_MISC_NOVATAG,           xMsdcNvt_Misc_Get_NovaTag},
	{CDB_11_MISC_BK_IS_LOCK,        xMsdcNvt_Misc_Get_Bk_IsLock},
	{CDB_11_MISC_BK_IS_FINISH,      xMsdcNvt_Misc_Get_Bk_IsFinish},
	{CDB_11_MISC_BK_LOCK,           xMsdcNvt_Misc_Get_Bk_Lock},
	{CDB_11_MISC_BK_UNLOCK,         xMsdcNvt_Misc_Get_Bk_UnLock},
	{CDB_11_MISC_UPD_FW_GET_INFO,   xMsdcNvt_Misc_Set_UpdFw_GetInfo},
	{CDB_11_MISC_UPD_FW_SET_BLOCK,  xMsdcNvt_Misc_Set_UpdFw_SetBlock},
};

void xMsdcNvt_Function_Misc(void)
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

static void xMsdcNvt_Misc_Get_Version(void *pInfo)
{
	MSDCNVT_MISC_GET_UINT32 *pValue = (MSDCNVT_MISC_GET_UINT32 *)pInfo;
	pValue->tResponse.biSize = sizeof(MSDCNVT_MISC_GET_UINT32);
	pValue->tResponse.bHandled = TRUE;

	pValue->Value = CFG_MSDC_VENDOR_NVT_VERSION;

	pValue->tResponse.bOK = TRUE;
}

static void xMsdcNvt_Misc_Get_NovaTag(void *pInfo)
{
	UINT32 *pTag = (UINT32 *)pInfo;
	*pTag = CFG_MSDC_VENDOR_NVT_TAG;
}

static void xMsdcNvt_Misc_Get_Bk_IsLock(void *pInfo)
{
	MSDCNVT_MISC_RESPONSE *pResponse = (MSDCNVT_MISC_RESPONSE *)pInfo;

	memset(pResponse, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	pResponse->biSize = sizeof(MSDCNVT_MISC_RESPONSE);
	pResponse->bHandled = TRUE;
	pResponse->bOK = xMsdcNvt_Bk_HostIsLock();
}

static void xMsdcNvt_Misc_Get_Bk_IsFinish(void *pInfo)
{
	MSDCNVT_MISC_RESPONSE *pResponse = (MSDCNVT_MISC_RESPONSE *)pInfo;

	memset(pResponse, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	pResponse->biSize = sizeof(MSDCNVT_MISC_RESPONSE);
	pResponse->bHandled = TRUE;
	pResponse->bOK = xMsdcNvt_Bk_IsFinish();
}

static void xMsdcNvt_Misc_Get_Bk_Lock(void *pInfo)
{
	MSDCNVT_MISC_RESPONSE *pResponse = (MSDCNVT_MISC_RESPONSE *)pInfo;

	memset(pResponse, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	pResponse->biSize = sizeof(MSDCNVT_MISC_RESPONSE);
	pResponse->bHandled = TRUE;
	pResponse->bOK = xMsdcNvt_Bk_HostLock();
}

static void xMsdcNvt_Misc_Get_Bk_UnLock(void *pInfo)
{
	MSDCNVT_MISC_RESPONSE *pResponse = (MSDCNVT_MISC_RESPONSE *)pInfo;

	memset(pResponse, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	pResponse->biSize = sizeof(MSDCNVT_MISC_RESPONSE);
	pResponse->bHandled = TRUE;
	pResponse->bOK = xMsdcNvt_Bk_HostUnLock();
}

static void xMsdcNvt_Misc_Set_UpdFw_GetInfo(void *pInfo)
{
	MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO *pBlkInfo = (MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO *)pInfo;

	DBG_ERR("NvUSB.dll is version too old\r\n");
	pBlkInfo->tResponse.bOK = FALSE; //unsupport after CFG_MSDC_VENDOR_NVT_VERSION
}

static void xMsdcNvt_Misc_Set_UpdFw_SetBlock(void *pInfo)
{
	DBG_ERR("NvUSB.dll is version too old\r\n");
	//unsupport after CFG_MSDC_VENDOR_NVT_VERSION
}
