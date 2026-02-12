
#include "ExifTemp.h"
#include "ExifWriter.h"
#include "ExifReader.h"
#include "ExifRW.h"
#include "exif/Exif.h"



static EXIF_CTRL m_ExifCtrl[EXIF_HDL_MAX_NUM] = {0};


UINT32 xExif_GetWorkingBuf(PEXIF_CTRL pCtrl, UINT32 size)
{
	UINT32 uiAvailableAddr = 0;
	if ((pCtrl->AvailableAddr + size) > pCtrl->WorkBufAddr + pCtrl->WorkBufSize) {
		DBG_ERR("Exif working buffer overflow!\r\n");
	} else {
		uiAvailableAddr = pCtrl->AvailableAddr;
		pCtrl->AvailableAddr += size;
	}
	return uiAvailableAddr;
}
UINT32 xExif_GetRestWorkingBufSize(PEXIF_CTRL pCtrl)
{
	return pCtrl->WorkBufAddr + pCtrl->WorkBufSize - pCtrl->AvailableAddr;
}


void xExif_ResetWorkingBuf(PEXIF_CTRL pCtrl)
{
	pCtrl->AvailableAddr = pCtrl->WorkBufAddr;
}
void EXIF_Init(EXIF_HANDLE_ID HandleID, PMEM_RANGE pWorkBuf, FPEXIFCB fpExifCB)
{
	PEXIF_CTRL pCtrl;
	if (0 == SEMID_EXIF[HandleID]) {
		DBG_ERR("ID(%d) not installed!\r\n", HandleID);
		return;
	}

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return;
	}

	pCtrl = xExif_GetCtrl(HandleID);
	if (pWorkBuf->size < MAX_APP1_SIZE) {
		DBG_WRN("Exif working buffer might be insufficient! Recommended size is 64KB.\r\n");
	}
	pCtrl->WorkBufAddr = pWorkBuf->addr;
	pCtrl->WorkBufSize = pWorkBuf->size;
	pCtrl->fpExifCB = fpExifCB;

	xExif_ResetWorkingBuf(pCtrl);
	pCtrl->uiInitKey = CFG_EXIF_INIT_KEY;
}

BOOL xExif_CheckInited(PEXIF_CTRL pCtrl)
{
	if (pCtrl->uiInitKey != CFG_EXIF_INIT_KEY) {
		DBG_ERR("Please run EXIF_Init() first!\r\n");
		return FALSE;
	}
	return TRUE;
}
BOOL xExif_CheckHandleID(EXIF_HANDLE_ID HandleID)
{
	if (HandleID < EXIF_HDL_MAX_NUM) {
		return TRUE;
	} else {
		DBG_ERR("Only support max %d EXIF sets.\r\n", EXIF_HDL_MAX_NUM);
		return FALSE;
	}

}
PEXIF_CTRL xExif_GetCtrl(EXIF_HANDLE_ID HandleID)
{
	return &m_ExifCtrl[HandleID];
}
ER xExif_lock(EXIF_HANDLE_ID HandleID)
{
	return wai_sem(SEMID_EXIF[HandleID]);
}

void xExif_unlock(EXIF_HANDLE_ID HandleID)
{
	return sig_sem(SEMID_EXIF[HandleID]);
}

EXIF_HANDLE_ID  EXIF_GetHandleID(UINT32 ExifDataAddr)
{
	UINT32 i;
	PEXIF_CTRL pCtrl;

	for (i = 0; i < EXIF_HDL_MAX_NUM; i++) {
		pCtrl = xExif_GetCtrl(i);
		if (pCtrl->uiInitKey == CFG_EXIF_INIT_KEY && pCtrl->ExifDataAddr == ExifDataAddr) {
			return i;
		}
	}
	DBG_MSG("^RCan't get handle ID! Using default EXIF_HDL_ID_1.\r\n");
	return EXIF_HDL_ID_1;
}

