#include <stdio.h>
#include "FsLinuxAPI.h"
#include "FsLinuxBuf.h"
#include "FsLinuxDebug.h"
#include "FsLinuxUtil.h"
#if defined (__UITRON) || defined (__ECOS)
#include "dma.h"
#include "dma_protected.h"
#endif

#define JD_1_JAN_1970 2440588 // 1 Jan 1970 in Julian day number

typedef struct {
	int Val_uItron;
	int Val_Linux;
} FSLINUX_VAL_SET;

static const FSLINUX_VAL_SET g_FsLinuxValTbl[] = {
	{ FST_STA_OK,                   FSLINUX_STA_OK},
	{ FST_STA_ERROR,                FSLINUX_STA_ERROR},
	{ FST_STA_BUSY,                 FSLINUX_STA_BUSY},
	{ FST_STA_FS_NO_MORE_FILES,     FSLINUX_STA_FS_NO_MORE_FILES},
	{ FST_STA_FILE_NOT_EXIST,       FSLINUX_STA_FILE_NOTEXIST},
	{ FST_STA_SDIO_ERR,             FSLINUX_STA_SDIO_ERR},
	{ FST_STA_PARAM_ERR,            FSLINUX_STA_PARAM_ERR},
	{ KFS_FILENAME_MAX_LENG,        FSLINUX_FILENAME_MAX_LENG},
	{ KFS_LONGFILENAME_MAX_LENG,    FSLINUX_LONGFILENAME_MAX_LENG},
	{ KFS_LONGNAME_PATH_MAX_LENG,   FSLINUX_LONGNAME_PATH_MAX_LENG},
	{ FST_FILE_DATETIME_MAX_ID,     FSLINUX_FILE_DATETIME_MAX_ID},
	{ FST_SEEK_MAX_ID,              FSLINUX_SEEK_MAX_ID},
	{ FST_FAT32,                    FSLINUX_FAT32},
	{ FST_EXFAT,                    FSLINUX_EXFAT},
	{ sizeof(FST_FILE_STATUS),      sizeof(FSLINUX_FILE_STATUS)},
	{ sizeof(FIND_DATA),            sizeof(FSLINUX_FIND_DATA)},
	{ sizeof(COPYTO_BYNAME_INFO),   sizeof(FSLINUX_COPYTO_BYNAME_INFO)},
};

static BOOL g_FsLinux_FoundSdioErr[FSLINUX_DRIVE_NUM] = {0};

//Linux and uItron should use the same struct definition
BOOL FsLinux_IsDefSync(VOID)
{
	UINT32 i;
	UINT32 ItemNum;

	ItemNum = sizeof(g_FsLinuxValTbl) / sizeof(FSLINUX_VAL_SET);

	for (i = 0; i < ItemNum; i++) {
		if (g_FsLinuxValTbl[i].Val_uItron != g_FsLinuxValTbl[i].Val_Linux) {
			DBG_ERR("Val_uItron(%d) != Val_Linux(%d)\r\n",
					g_FsLinuxValTbl[i].Val_uItron, g_FsLinuxValTbl[i].Val_Linux);
			return FALSE;
		}
	}

	return TRUE;
}

//#NT#2018/05/09#Willy Su -begin
static UINT32 FsLinux_GDateToJDays(INT32 Day, INT32 Month, INT32 Year)
{
	UINT32 JDN, tmp1, tmp2, tmp3;
	tmp1 = ((Month == 1) || (Month == 2)) ? 1 : 0;
	tmp2 = Year + 4800 - tmp1;
	tmp3 = Month + 12 * tmp1 - 3;
	JDN = Day + (((153 * tmp3) + 2) / 5) + (365 * tmp2) +
		(tmp2 / 4) - (tmp2 / 100) + (tmp2 / 400) - 32045;
	return JDN;
}

static void FsLinux_JDaysToGDate(UINT32 JDN, INT32 *OutDay, INT32 *OutMonth, INT32 *OutYear)
{
	UINT32 tmp1, tmp2, tmp3, tmp4, tmp5;
	tmp1 = JDN + 1401 + (((((4 * JDN) + 274277) / 146097) * 3) / 4) - 38;
	tmp2 = 4 * tmp1 + 3;
	tmp3 = (tmp2 % 1461) / 4;
	tmp4 = 5 * tmp3 + 2;
	tmp5 = (((tmp4 / 153) + 2) % 12) + 1;
	*OutDay = ((tmp4 % 153) / 5) + 1;
	*OutMonth = tmp5;
	*OutYear = (tmp2 / 1461) - 4716 + ((12 + 2 - tmp5) / 12);
}
//#NT#2018/05/09#Willy Su -end
//#NT#2018/05/10#Willy Su -begin
UINT32 FsLinux_Date_Dos2Unix(UINT16 DosTime, UINT16 DosDate)
{
	UINT32  TimeStamp, JDDate, JDTime;
	INT32   hour = 24, min = 60, sec = 60;
	INT32   Hr, Mn, Sc, Dy, Mo, Yr;
	DBG_IND("DosTime=%d DosDate=%d\r\n", DosTime, DosDate);

	Yr = (((DosDate / 32) / 16) + 1980);
	Mo = ((DosDate / 32) & 0x0f);
	Dy = (DosDate & 0x1f);
	JDDate = FsLinux_GDateToJDays(Dy, Mo, Yr);
	Hr = ((DosTime / 32) / 64);
	Mn = ((DosTime / 32) & 0x3f);
	Sc = ((DosTime & 0x1f) * 2);
	JDTime = (Sc + (Mn * sec) + (Hr * (sec * min)));
	DBG_IND("date=%d:%d:%d %d-%d-%d\r\n", Hr, Mn, Sc, Yr, Mo, Dy);

	TimeStamp = (JDDate - JD_1_JAN_1970) * (hour * min * sec) + JDTime;
	DBG_IND("timestamp=%d\n", TimeStamp);

	return TimeStamp;
}

void FsLinux_Date_Unix2Dos(UINT32 UnixTimeStamp, UINT16 *OutDosTime, UINT16 *OutDosDate)
{
	UINT32  JDN;
	INT32   hour = 24, min = 60, sec = 60;
	INT32   Hr, Mn, Sc, Dy, Mo, Yr;
	UINT16  DosTime, DosDate;

	JDN = ((UnixTimeStamp / (sec * min * hour))) + JD_1_JAN_1970;
	FsLinux_JDaysToGDate(JDN, &Dy, &Mo, &Yr);
	Yr = (Yr < 1980) ? 0 : (Yr - 1980);
	DosDate = Dy + (Mo * 32) + (Yr * 512);
	Sc = (UnixTimeStamp % sec);
	Mn = ((UnixTimeStamp / sec) % min);
	Hr = ((UnixTimeStamp / (sec * min)) % hour);
	DosTime = (Sc / 2) + (Mn * 32) + (Hr * 2048);
	DBG_IND("UnixTimeStamp=%d date=%d:%d:%d %d-%d-%d\r\n", UnixTimeStamp, Hr, Mn, Sc, Yr, Mo, Dy);

	if (NULL != OutDosTime)
		*OutDosTime = DosTime;
	if (NULL != OutDosTime)
		*OutDosTime = DosDate;
	DBG_IND("dos time=%d date=%d\r\n", DosTime, DosDate);
}
//#NT#2018/05/10#Willy Su -end

UINT32 FsLinux_wcslen(const UINT16 *pSrcString)
{
	const UINT16 *pCurChar;

	pCurChar = pSrcString;

	while (*pCurChar) {
		pCurChar++;
	}

	return (UINT32)(pCurChar - pSrcString);
}

BOOL FsLinux_ConvToSingleByte(char *pDstName, UINT16 *pSrcName)
{
	UINT16 *pCurChar = pSrcName;

	while (*pCurChar != 0) {
		*pDstName++ = (CHAR) * pCurChar++;
	}

	DBG_IND("pDstName :%s \r\n", pDstName);
	return TRUE;
}

//ex: pPath_Src = A:\ABC\DEF\GHI.txt, pPath_Mount = /mnt/sdcard
//pPath_Dst = /mnt/sdcard/ABC/DEF/GHI.txt
BOOL FsLinux_Conv2LinuxPath(CHAR *pPath_Dst, UINT32 DstSize, const CHAR *pPath_Src)
{
	FSLINUX_DRIVE_INFO *pDrvInfo;
	CHAR *pCurChar;

	pDrvInfo = FsLinux_Buf_GetDrvInfo(*pPath_Src);
	if (NULL == pDrvInfo) {
		DBG_ERR("Get Drive Info failed\r\n");
		return FALSE;
	}

	if (strlen(pPath_Src) < 3) {
		DBG_ERR("Invalid path: %s (len < 3)\r\n", pPath_Src);
		return FALSE;
	}

	snprintf(pPath_Dst, DstSize, "%s%s", pDrvInfo->FsInitParam.FSParam.szMountPath, pPath_Src + 3); //3 is for skipping "A:\"

	//change all uITRON '\' to Linux '/'
	pCurChar = pPath_Dst;
	while ('\0' != *pCurChar) {
		if ('\\' == *pCurChar) {
			*pCurChar = '/';
		}

		pCurChar++;
	}

	DBG_IND("pPath_Dst = %s, pPath_Src = %s\r\n", pPath_Dst, pPath_Src);

	return TRUE;
}

//check the path is null-terminated
BOOL FsLinux_IsPathNullTerm(CHAR *pPath, UINT32 MaxLen)
{
	CHAR *pChar = pPath + MaxLen - 1;

	while (pChar > pPath) {
		if ('\0' == *pChar) {
			return TRUE;
		}
	}

	return FALSE;
}

BOOL FsLinux_FlushReadCache(UINT32 Addr, UINT32 Size)
{
	//skip the non-cached address
#if defined (__UITRON) || defined (__ECOS)
	if (!dma_isCacheAddr(Addr)) {
		return TRUE;
	}
#endif

	//check the cached address
#if defined (__UITRON) || defined (__ECOS)
	if (Addr & 0x1F) {
		DBG_WRN("Addr(0x%X) not 32-byte aligned\r\n", Addr);
		return FALSE;
	}

	if (Size & 0x1F) {
		DBG_WRN("Size(%d) not 32-byte aligned\r\n", Size);
		return FALSE;
	}
#endif

#if defined (__UITRON) || defined (__ECOS)
	dma_flushReadCache(Addr, Size);
#else
	//vos_cpu_dcache_sync(Addr, Size, VOS_DMA_FROM_DEVICE);
#endif

	return TRUE;
}

BOOL FsLinux_FlushWriteCache(UINT32 Addr, UINT32 Size)
{
	//skip the non-cached address
#if defined (__UITRON) || defined (__ECOS)
	if (!dma_isCacheAddr(Addr)) {
		return TRUE;
	}
#endif

	//#NT#2015/03/24#Nestor Yang -begin
	//#NT# skip the cache limitation check
	//If the cache buffer is for CPU2 reading and CPU2 will not modify this region,
	//we can skip the 16-byte alignment check
	/*
	if(Addr & 0x1F)
	{
	    DBG_WRN("Addr(0x%X) not 32-byte aligned\r\n", Addr);
	    return FALSE;
	}

	if(Size & 0x1F)
	{
	    DBG_WRN("Size(%d) not 32-byte aligned\r\n", Size);
	    return FALSE;
	}
	*/
	//#NT#2015/03/24#Nestor Yang -end
#if defined (__UITRON) || defined (__ECOS)
	dma_flushWriteCache(Addr, Size);
#else
	//vos_cpu_dcache_sync(Addr, Size, VOS_DMA_TO_DEVICE);
#endif

	return TRUE;
}

BOOL FsLinux_Util_ClrSDIOErr(CHAR Drive)
{
	UINT32 DrvIdx;
	if (Drive < FSLINUX_DRIVE_NAME_FIRST || Drive > FSLINUX_DRIVE_NAME_LAST) {
		DBG_ERR("Invalid Drive %c\r\n", Drive);
		return FALSE;
	}
	DrvIdx = Drive - FSLINUX_DRIVE_NAME_FIRST;
	g_FsLinux_FoundSdioErr[DrvIdx] = FALSE;
	return TRUE;
}

BOOL FsLinux_Util_SetSDIOErr(CHAR Drive)
{
	UINT32 DrvIdx;
	if (Drive < FSLINUX_DRIVE_NAME_FIRST || Drive > FSLINUX_DRIVE_NAME_LAST) {
		DBG_ERR("Invalid Drive %c\r\n", Drive);
		return FALSE;
	}
	DrvIdx = Drive - FSLINUX_DRIVE_NAME_FIRST;
	g_FsLinux_FoundSdioErr[DrvIdx] = TRUE;
	return TRUE;
}

BOOL FsLinux_Util_IsSDIOErr(CHAR Drive)
{
	UINT32 DrvIdx;
	if (Drive < FSLINUX_DRIVE_NAME_FIRST || Drive > FSLINUX_DRIVE_NAME_LAST) {
		DBG_ERR("Invalid Drive %c\r\n", Drive);
		return FALSE;
	}
	DrvIdx = Drive - FSLINUX_DRIVE_NAME_FIRST;
	if (g_FsLinux_FoundSdioErr[DrvIdx]) {
		return TRUE;
	} else {
		return FALSE;
	}
}

ER FsLinux_Util_SendMsgDirect(const FSLINUX_IPCMSG *pSendMsg)
{
	FSLINUX_DRIVE_INFO *pDrvInfo;
	INT32 Ret;

	DBG_FUNC_BEGIN("[CMD]\r\n");

	pDrvInfo = FsLinux_Buf_GetDrvInfo(pSendMsg->Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("GetDrvInfo failed\r\n");
		return E_SYS;
	}

	Ret = fslinux_cmd_DoCmd(pSendMsg);
	if (Ret < 0) {
		DBG_ERR("DoCmd = %d\r\n", Ret);
		return E_SYS;
	}

	DBG_FUNC_END("[CMD]\r\n");

	return E_OK;
}

ER FsLinux_Util_ReqInfoDirect(const FSLINUX_IPCMSG *pSendMsg)
{
	INT32 Ret;

	DBG_FUNC_BEGIN("[CMD]\r\n");

	if (pSendMsg->CmdId != FSLINUX_CMDID_GETDISKINFO) {
		DBG_ERR("Invalid CmdId %d\r\n", pSendMsg->CmdId);
		return E_PAR;
	}

	Ret = fslinux_cmd_DoCmd(pSendMsg);
	if (Ret < 0) {
		DBG_ERR("DoCmd = %d\r\n", Ret);
		return E_SYS;
	}

	DBG_FUNC_END("[CMD]\r\n");

	return E_OK;
}