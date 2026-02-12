#include <kwrap/type.h>
#include "DxStorage.h"
#include "DxApi.h"
#include "GxStrg.h"
#include "GxStrgInt.h"
#include <kwrap/error_no.h>

UINT32 uiStrgCardCurSts = DET_CARD_REMOVED;
UINT32 uiStrgCardStatus = DET_CARD_UNKNOWN;

/*
  Detect SD card is inserted or not

  Detect SD card is inserted or not.
  If SD card is inserted, then detect SD card write protect status and
  store it in bStrgCardLock (static variable)
  [KeyScan internal API]

  @param void
  @return void
*/
void GxStrg_Det(UINT32 DevID, DX_HANDLE StrgDXH)
{
	UINT32 uiDxState = 0;
	static UINT32   uiStrgCardPrevSts = DET_CARD_UNKNOWN;

	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		DBG_WRN("Invalid DevID(%d)\r\n", DevID);
		return;
	}

	if (FST_FS_TYPE_LINUX == g_FsType) {
		uiStrgCardCurSts = GxStrgLnx_Det(DevID);
	} else {
		if (Dx_GetState(StrgDXH, STORAGE_STATE_INSERT, &uiDxState) == DX_OK && uiDxState == TRUE) {
			uiStrgCardCurSts = DET_CARD_INSERTED;
		} else {
			uiStrgCardCurSts = DET_CARD_REMOVED;
		}
	}

	// Debounce
	if ((uiStrgCardCurSts == uiStrgCardPrevSts) &&
		(uiStrgCardCurSts != uiStrgCardStatus)) {
		switch (uiStrgCardCurSts) {
		case DET_CARD_INSERTED:
			if (g_fpStrgCB) {
				(*g_fpStrgCB)(STRG_CB_INSERTED, DevID, 0);
			}
			break;
		case DET_CARD_REMOVED:
			if (g_fpStrgCB) {
				(*g_fpStrgCB)(STRG_CB_REMOVED, DevID, 0);
			}
			break;
		case DET_CARD_UNKNOWN:
			if (g_fpStrgCB) {
				(*g_fpStrgCB)(STRG_CB_UNKNOWN, DevID, 0);
			}
			break;
		}

		uiStrgCardStatus = uiStrgCardCurSts;
	}

	uiStrgCardPrevSts = uiStrgCardCurSts;
}

void GxStrg_SetDeviceCtrl(UINT32 DevID, UINT32 data, UINT32 value)
{
	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		DBG_WRN("Invalid DevID(%d)\r\n", DevID);
		return;
	}

	switch (data) {
	case CARD_POWER:
		Dx_Control(g_pCurStrgDXH[DevID], STORAGE_CTRL_POWER, value, 0);
		break;
	default:
		break;
	}
}

/*
  Get SD card status

  Get SD card status

  @param state
           CARD_INSERT
           CARD_READONLY
  @return BOOL:
           CARD_INSERT
           TRUE -> Card is inserted, FALSE -> Card is removed
           CARD_READONLY
           TRUE -> Card is locked, FALSE -> Card is not locked
*/
UINT32 GxStrg_GetDeviceCtrl(UINT32 DevID, UINT32 data)
{
	UINT32 uiDxState = 0;
	int getv = 0;

	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		DBG_WRN("Invalid DevID(%d)\r\n", DevID);
		return getv;
	}

	switch (data) {
	case CARD_INSERT:
		if (FST_FS_TYPE_LINUX == g_FsType) {
			getv = g_LnxStrgStatus[DevID].IsInserted;
		} else {
			getv = (Dx_GetState(g_pCurStrgDXH[DevID], STORAGE_STATE_INSERT, &uiDxState) == DX_OK && uiDxState == TRUE);
		}
		break;

	case CARD_READONLY:
		if (FST_FS_TYPE_LINUX == g_FsType) {
			getv = g_LnxStrgStatus[DevID].IsReadOnly;
		} else {
			BOOL bLock = FALSE;
			if (Dx_GetState(g_pCurStrgDXH[DevID], STORAGE_STATE_INSERT, &uiDxState) == DX_OK && uiDxState == TRUE) {
				if (DX_OK == Dx_GetState(g_pCurStrgDXH[DevID], STORAGE_STATE_LOCK, &uiDxState)) {
					bLock = (BOOL)uiDxState;
				} else {
					bLock = TRUE;    //if get status failed, return as locked
				}
			}
			getv = bLock;
		}
		break;
	case CARD_MNTPATH:
		getv = (UINT32)&g_FSInitParam[DevID].FSParam.szMountPath[0];
		break;
	default:
		DBG_WRN("Invalid arg %d\r\n", data);
		break;
	}
	return getv;

}

