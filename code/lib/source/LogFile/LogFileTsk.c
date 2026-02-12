/*
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    @file       LogFile.c
    @author     Lincy Lin
    @date       2016/2/1
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if defined (__UITRON) || defined (__ECOS)
#include "dma_protected.h"
#else
#include "kwrap/cpu.h"
#endif
#include "Utility/SwTimer.h"
#include "FileSysTsk.h"
#include "LogFileID.h"
#include "LogFileInt.h"
#include "LogFileNaming.h"
#include "LogFileOp.h"
#include "LogFileCon.h"
#include "LogFile.h"
#if 0  //_CPU2_LINUX_
#include "NvtIpcAPI.h"
#endif //_CPU2_LINUX_
#include <time.h>
#include <comm/hwclock.h>

#define __MODULE__          LogFileTsk
#define __DBGLVL__          2
#include "kwrap/debug.h"
extern unsigned int LogFileTsk_debug_level;

#if defined(_BSP_NA51000_) || defined(_BSP_NA51023_)
#define DBGLVL_NORMAL       2
#else
#define DBGLVL_NORMAL       1
#endif




static  LOGFILE_CTRL m_LogFile_Ctrl= {0};
static  BOOL         gLoopQuit = FALSE;



LOGFILE_CTRL *LogFile_GetCtrl(void)
{
	return &m_LogFile_Ctrl;
}

static void LogFile_Lock(void)
{
	vos_sem_wait(LOGFILE_SEM_ID);
}

static void LogFile_UnLock(void)
{
	vos_sem_sig(LOGFILE_SEM_ID);
}

static LOGFILE_WR LogFile_Wrn(LOGFILE_WR wr)
{
	if (wr == LOGFILE_WR_OK) {
		return wr;
	}
	DBG_WRN("%d\r\n", wr);
	return wr;
}

static void LogFile_ExitDirect(void)
{
	DBG_DUMP("(halt)\r\n");
	// Loop forever
	while (!gLoopQuit) { //fix for CID 43220
		;
	}
}

ER LogFile_Config(LOGFILE_CFG *pCfg)
{
	LOGFILE_CTRL *pCtrl;
	pCtrl = LogFile_GetCtrl();
	LOGFILE_RW_CTRL		*p_rw_ctrl;

	if (!pCfg) {
		DBG_ERR("pCfg is NULL\r\n");
		return E_PAR;
	}

	// init some buffer variables
	memset(pCtrl, 0, sizeof(LOGFILE_CTRL));
	pCtrl->ConType = pCfg->ConType;
	pCtrl->TimeType = pCfg->TimeType;
	pCtrl->state = LOGFILE_STATE_INIT;
	p_rw_ctrl = &pCtrl->rw_ctrl[LOG_CORE1];
	p_rw_ctrl->pbuf = (LOGFILE_RINGBUF_HEAD *)(pCfg->LogBuffAddr);
	p_rw_ctrl->pbuf->BufferStartAddr = pCfg->LogBuffAddr + sizeof(LOGFILE_RINGBUF_HEAD);
	p_rw_ctrl->pbuf->BufferSize = pCfg->LogBuffSize - sizeof(LOGFILE_RINGBUF_HEAD);
	if (p_rw_ctrl->pbuf->InterfaceVer != LOGFILE_INTERFACE_VER) {
		p_rw_ctrl->pbuf->InterfaceVer = LOGFILE_INTERFACE_VER;
		p_rw_ctrl->pbuf->DataIn = 0;
		p_rw_ctrl->pbuf->DataOut = 0;
	} else {
		DBG_IND("[LogFile_Config] InterfaceVer Saved\r\n");
	}
	pCtrl->LogCoreNum = 1;
	if (pCfg->LogBuffSize2) {
		DBG_IND("[LogFile_Config] Setting LogBuffSize2\r\n");
		p_rw_ctrl = &pCtrl->rw_ctrl[LOG_CORE2];
		p_rw_ctrl->pbuf = (LOGFILE_RINGBUF_HEAD *)(pCfg->LogBuffAddr2);
		p_rw_ctrl->pbuf->BufferStartAddr = pCfg->LogBuffAddr2 + sizeof(LOGFILE_RINGBUF_HEAD);
		p_rw_ctrl->pbuf->BufferSize = pCfg->LogBuffSize2 - sizeof(LOGFILE_RINGBUF_HEAD);
		if (p_rw_ctrl->pbuf->InterfaceVer != LOGFILE_INTERFACE_VER) {
			p_rw_ctrl->pbuf->InterfaceVer = LOGFILE_INTERFACE_VER;
			p_rw_ctrl->pbuf->DataIn = 0;
			p_rw_ctrl->pbuf->DataOut = 0;
		} else {
			DBG_IND("[LogFile_Config] InterfaceVer Saved\r\n");
		}
		pCtrl->LogCoreNum ++;
	}
	// register low-level power off CB to exit()
	atexit(LogFile_ExitDirect);
	// register and set console
	LogFile_SetConsole();
	return E_OK;
}


ER LogFile_Open(LOGFILE_OPEN *pOpenParm)
{
	LOGFILE_CTRL *pCtrl;
	UINT32        Dirlen, i;
	LOGFILE_NAME_OBJ *pObj;
	CHAR      	  filename[LOGFILE_ROOT_DIR_MAX_LEN + MAX_FILE_NAME_LEN + 1];
	BOOL          ret;
	LOGFILE_RW_CTRL		*p_rw_ctrl;
	CHAR          *rootDir;


	DBG_FUNC_BEGIN("\r\n");
	LogFile_InstallID();
	if (!LOGFILE_SEM_ID) {
		DBG_ERR("ID not installed\r\n");
		return E_SYS;
	}
	if (!pOpenParm) {
		DBG_ERR("pOpen is NULL\r\n");
		return E_PAR;
	}
	// check if dir last character is backslash
	if (pOpenParm->rootDir[0]) {
		Dirlen = strlen(pOpenParm->rootDir);
		if (pOpenParm->rootDir[Dirlen - 1] != '\\') {
			DBG_ERR("rootDir = %s, string end should be backslash\r\n", pOpenParm->rootDir);
			return E_PAR;
		}
	}
	LogFile_Lock();
	pCtrl = LogFile_GetCtrl();
	if (pCtrl->state == LOGFILE_STATE_UNINIT) {
		DBG_ERR("Need to setconsole\r\n");
		LogFile_UnLock();
		return E_OBJ;
	}
	if (pCtrl->uiInitKey == CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_ALREADY_OPEN);
		LogFile_UnLock();
		return E_OK;
	}
	DBG_IND("maxFileNum =%d, maxFileSize=0x%x\r\n", pOpenParm->maxFileNum, pOpenParm->maxFileSize);
	// init common variable
	pCtrl->OpenParm = *pOpenParm;
	pCtrl->uiInitKey = CFG_LOGFILE_INIT_KEY;
	pCtrl->state = LOGFILE_STATE_NORMAL;
	pCtrl->FlagID = LOGFILE_FLG_ID;
	pCtrl->isAddTimeStr = TRUE;
	// install sx command
	//LogFile_InstallCmd();
	// clear flag
	clr_flg(pCtrl->FlagID, 0xFFFFFFFF);

	// init each core log context
	for (i = 0; i < pCtrl->LogCoreNum; i++) {
		p_rw_ctrl = &pCtrl->rw_ctrl[i];
		if (pOpenParm->maxFileNum > LOGFILE_MAX_FILENUM) {
			DBG_WRN("maxFileNum exceeds limit 0x%x, use max\r\n", LOGFILE_MAX_FILENUM);
			p_rw_ctrl->maxFileNum = LOGFILE_MAX_FILENUM;
		} else if (pOpenParm->maxFileNum == 0) {
			p_rw_ctrl->maxFileNum = LOGFILE_DEFAULT_MAX_FILENUM;
		} else {
			p_rw_ctrl->maxFileNum = pOpenParm->maxFileNum;
		}
		if (pOpenParm->maxFileSize > LOGFILE_MAX_FILESIZE) {
			DBG_WRN("maxFileSize exceeds limit 0x%x, use max\r\n", LOGFILE_MAX_FILESIZE);
			p_rw_ctrl->maxFileSize = LOGFILE_MAX_FILESIZE;
		} else if (pOpenParm->maxFileSize < LOGFILE_MIN_FILESIZE) {
			DBG_WRN("maxFileSize smaller than min limit 0x%x, use min\r\n", LOGFILE_MIN_FILESIZE);
			p_rw_ctrl->maxFileSize = LOGFILE_MIN_FILESIZE;
		} else if (pOpenParm->maxFileSize == 0) {
			p_rw_ctrl->maxFileSize = LOGFILE_DEFAULT_MAX_FILESIZE;
		} else {
			p_rw_ctrl->maxFileSize = pOpenParm->maxFileSize;
		}
		p_rw_ctrl->isPreAllocAllFiles = pCtrl->OpenParm.isPreAllocAllFiles;
		p_rw_ctrl->isZeroFile = pCtrl->OpenParm.isZeroFile;
		DBG_IND("core %d, log buffer =0x%x, size=0x%x\r\n", i, p_rw_ctrl->pbuf->BufferStartAddr, p_rw_ctrl->pbuf->BufferSize);
		// Init FileNaming
		if (i == 0) {
			rootDir = pCtrl->OpenParm.rootDir;
		} else {
			rootDir = pCtrl->OpenParm.rootDir2;
		}
		pObj = LogFileNaming_GetObj(LOGFILE_NAME_TYPE_NORMAL+i);
		if (NULL == pObj) {
			DBG_ERR("GetObj Fail\r\n");
			break;
		}
		p_rw_ctrl->p_nameobj = pObj;
		if ((pCtrl->ConType & LOGFILE_CON_STORE) && LogFileNaming_Init(pObj, p_rw_ctrl->maxFileNum, rootDir) == 0) {
			// current log file count is 0, need to pre-alloc all files
			if (p_rw_ctrl->isPreAllocAllFiles &&  LogFileNaming_PreAllocFiles(pObj,p_rw_ctrl->maxFileNum, p_rw_ctrl->maxFileSize) < 0) {
				p_rw_ctrl->hasFileErr = TRUE;
			}
		}
	}
	if (pOpenParm->isSaveLastTimeSysErrLog) {
		pObj = LogFileNaming_GetObj(LOGFILE_NAME_TYPE_SYSERR);
		if (LogFileNaming_Init(pObj,pCtrl->OpenParm.maxFileNum, pCtrl->OpenParm.sysErrRootDir) == 0 &&
			pCtrl->OpenParm.isPreAllocAllFiles) {
			// set max sys file size to min file size
			LogFileNaming_PreAllocFiles(pObj, LOGFILE_DEFAULT_MAX_FILENUM, LOGFILE_MIN_FILESIZE *2);
		}
		ret = LogFileNaming_GetNewPath(pObj, filename, sizeof(filename));
		if (ret == TRUE) {
			LogFile_SaveLastTimeSysErrLog(pOpenParm->lastTimeSysErrLogBuffAddr,pOpenParm->lastTimeSysErrLogBuffSize, filename);
		}
	}
	// start task
	THREAD_RESUME(LOGFILE_TSK_MAIN_ID);
	THREAD_RESUME(LOGFILE_TSK_EMR_ID);
	LogFile_UnLock();
	DBG_FUNC_END("\r\n");
	return E_OK;
}


ER LogFile_ReOpen(void)
{
	LOGFILE_CTRL *pCtrl;
	LOGFILE_OPEN  OpenParm;

	DBG_FUNC_BEGIN("\r\n");

	pCtrl = LogFile_GetCtrl();
	if (pCtrl->state == LOGFILE_STATE_NORMAL || pCtrl->state == LOGFILE_STATE_SUSPEND) {
		LogFile_Close();
		OpenParm = pCtrl->OpenParm;
		LogFile_Open(&OpenParm);
	}
	DBG_FUNC_END("\r\n");
	return E_OK;
}


ER LogFile_Close(void)
{
	FLGPTN       uiFlag;
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();

	DBG_FUNC_BEGIN("\r\n");
	LogFile_Lock();
	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_NOT_OPEN);
		LogFile_UnLock();
		return E_OK;
	}
	set_flg(pCtrl->FlagID, FLG_LOGFILE_STOP);
	DBG_IND("wait idle\r\n");
	wai_flg(&uiFlag, pCtrl->FlagID, FLG_LOGFILE_IDLE, TWF_ORW);
	set_flg(pCtrl->FlagID, FLG_LOGFILE_STOP_EMR);
	DBG_IND("wait emr idle\r\n");
	wai_flg(&uiFlag, pCtrl->FlagID, FLG_LOGFILE_EMR_IDLE, TWF_ORW);
	vos_task_destroy(LOGFILE_TSK_MAIN_ID);
	vos_task_destroy(LOGFILE_TSK_EMR_ID);
	pCtrl->uiInitKey = 0;
	// set state to init, because already set the console.
	pCtrl->state = LOGFILE_STATE_INIT;
	LogFile_UnLock();
	LogFile_UnInstallID();
	DBG_FUNC_END("\r\n");
	return E_OK;
}

ER LogFile_Suspend(void)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();

	DBG_FUNC_BEGIN("\r\n");
	LogFile_Lock();
	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_NOT_OPEN);
		LogFile_UnLock();
		return E_OBJ;
	}
	pCtrl->state = LOGFILE_STATE_SUSPEND;
	LogFile_UnLock();
	DBG_FUNC_END("\r\n");
	return E_OK;
}

ER LogFile_Complete(void)
{
	FLGPTN       uiFlag;
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();

	if (pCtrl->state != LOGFILE_STATE_SUSPEND) {
		DBG_DUMP("Need to Suspend first\r\n");
		return E_OK;
	}

	DBG_FUNC_BEGIN("\r\n");
	LogFile_Lock();
	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_NOT_OPEN);
		LogFile_UnLock();
		return E_OBJ;
	}
	set_flg(pCtrl->FlagID, FLG_LOGFILE_COMPLETE);
	DBG_IND("wait complete\r\n");
	wai_flg(&uiFlag, pCtrl->FlagID, FLG_LOGFILE_COMPLETED, TWF_ORW);
	LogFile_UnLock();
	DBG_FUNC_END("\r\n");
	return E_OK;
}

ER LogFile_Resume(void)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();

	DBG_FUNC_BEGIN("\r\n");
	LogFile_Lock();
	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_NOT_OPEN);
		LogFile_UnLock();
		return E_OBJ;
	}
	pCtrl->state = LOGFILE_STATE_NORMAL;
	LogFile_UnLock();
	DBG_FUNC_END("\r\n");
	return E_OK;
}

static void LogFile_TimerCb(UINT32 uiEvent)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();

	set_flg(pCtrl->FlagID, FLG_LOGFILE_TIMEHIT);
}

UINT32 LogFile_GetDataSize(LOGFILE_RINGBUF_HEAD *pbuf)
{
	if (pbuf->DataIn >= pbuf->DataOut) {
		return pbuf->DataIn - pbuf->DataOut;
	}
	return pbuf->BufferSize + pbuf->DataIn - pbuf->DataOut;
}


ER LogFile_DumpToMem(UINT32 Addr, UINT32 Size)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	LOGFILE_RINGBUF_HEAD *pbuf;
	UINT32                dumpSize, DataIn;
	LOGFILE_DATA_HEAD	  *data_head;

	DBG_FUNC_BEGIN("\r\n");
	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_NOT_OPEN);
		return E_OBJ;
	}
	if (Addr == 0){
		DBG_ERR("Addr 0\r\n");
		return E_PAR;
	}
	if (Size < LOGFILE_BUFFER_SIZE){
		DBG_ERR("Size %d < Need %d\r\n", Size, LOGFILE_BUFFER_SIZE);
		return E_PAR;
	}
	if (pCtrl->LogCoreNum > 1 && pCtrl->rw_ctrl[LOG_CORE2].pbuf->SysErr == LOGFILE_SYS_ERROR_KEY) {
		pbuf = pCtrl->rw_ctrl[LOG_CORE2].pbuf;
		DBG_IND("core 2 syserr\r\n");
	} else {
		pbuf = pCtrl->rw_ctrl[LOG_CORE1].pbuf;
		DBG_IND("core 1 syserr\r\n");
	}
	data_head = (LOGFILE_DATA_HEAD	  *)Addr;
	data_head->tag  = CFG_LOGFILE_INIT_KEY;
	data_head->data = (CHAR *)(Addr + sizeof(LOGFILE_DATA_HEAD));
	DataIn = pbuf->DataIn;
	if (pbuf->flags & LOG_ROLLBACK){
		data_head->size = pbuf->BufferSize;
		dumpSize = pbuf->BufferSize - DataIn;
		DBG_IND("dump pbuf->DataIn 0x%x, size 0x%x, \r\n",DataIn,dumpSize);
		memcpy((void*)data_head->data,(void*)(DataIn + pbuf->BufferStartAddr), dumpSize);
		DBG_IND("dump BufferStartAddr 0x%x, size 0x%x \r\n",pbuf->BufferStartAddr, DataIn);
		memcpy((void*)(data_head->data+dumpSize),(void*)pbuf->BufferStartAddr, DataIn);
	} else {
		dumpSize = DataIn;
		data_head->size = dumpSize;
		DBG_IND("dump pbuf->DataIn 0x%x, size 0x%x, \r\n",DataIn,dumpSize);
		memcpy((void*)data_head->data,(void*)pbuf->BufferStartAddr,dumpSize);
	}
#if defined (__UITRON) || defined (__ECOS)
	dma_flushWriteCache(Addr, data_head->size+sizeof(LOGFILE_DATA_HEAD));
#else
	vos_cpu_dcache_sync(Addr, data_head->size+sizeof(LOGFILE_DATA_HEAD), VOS_DMA_TO_DEVICE);
#endif
	DBG_FUNC_END("\r\n");
	return E_OK;
}

ER LogFile_DumpToFile(CHAR* filepath)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	LOGFILE_RINGBUF_HEAD *pbuf = pCtrl->rw_ctrl[LOG_CORE1].pbuf;
	UINT32        wSize, DataIn;
	FST_FILE      filehdl = NULL;

	DBG_FUNC_BEGIN("\r\n");
	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		LogFile_Wrn(LOGFILE_WR_NOT_OPEN);
		return E_OBJ;
	}
	filehdl = FileSys_OpenFile(filepath, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (!filehdl) {
		DBG_ERR("Open file %s fail\r\n", filepath);
		return E_SYS;
	}
	DataIn = pbuf->DataIn;
	if (DataIn < pbuf->DataOut){
		wSize = pbuf->BufferSize - DataIn;
		if (FileSys_WriteFile(filehdl, (UINT8 *)DataIn + pbuf->BufferStartAddr, &wSize, 0, NULL) != FST_STA_OK) {
			goto dumpfile_fail;
		}
		wSize = DataIn;
		if (FileSys_WriteFile(filehdl, (UINT8 *)pbuf->BufferStartAddr, &wSize, 0, NULL) != FST_STA_OK) {
			goto dumpfile_fail;
		}
	} else {
		wSize = DataIn;
		if (FileSys_WriteFile(filehdl, (UINT8 *)pbuf->BufferStartAddr, &wSize, 0, NULL) != FST_STA_OK) {
			goto dumpfile_fail;
		}
	}
	DBG_FUNC_END("\r\n");
	return E_OK;
dumpfile_fail:
	DBG_ERR("Write file %s Fail\r\n", filepath);
	FileSys_CloseFile(filehdl);
	DBG_FUNC_END("\r\n");
	return E_SYS;
}

ER LogFile_ChkSysErr(void)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	LOGFILE_RINGBUF_HEAD *pbuf;

	if (pCtrl->uiInitKey != CFG_LOGFILE_INIT_KEY) {
		return E_OK;
	}
	if (pCtrl->LogCoreNum <= 1) {
		return E_OK;
	}
	pbuf = pCtrl->rw_ctrl[LOG_CORE2].pbuf;
	if (pbuf->SysErr == LOGFILE_SYS_ERROR_KEY) {
		return E_SYS;
	}
	return E_OK;
}

THREAD_RETTYPE LogFile_MainTsk(void)
{
	FLGPTN       uiFlag;
	LOGFILE_CTRL *pCtrl;
	BOOL         bContinue = 1;
	SWTIMER_ID   timer_id;
	BOOL         isOpenTimerOK = TRUE;
	BOOL         isFirstOpen = TRUE;
	UINT32       DataSize, i;
	LOGFILE_RW_CTRL *p_rw_ctrl;

	THREAD_ENTRY();
	DBG_FUNC_BEGIN("\r\n");
	pCtrl = LogFile_GetCtrl();
	if (SwTimer_Open(&timer_id, LogFile_TimerCb) != E_OK) {
		DBG_ERR("open timer fail\r\n");
		isOpenTimerOK = FALSE;
	} else {
		SwTimer_Cfg(timer_id, LOG_WRITE_CHECK_TIME, SWTIMER_MODE_FREE_RUN);
		if (m_LogFile_Ctrl.ConType & LOGFILE_CON_STORE) {
			SwTimer_Start(timer_id);
		}
	}
	while (bContinue) {
		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, pCtrl->FlagID, FLG_LOGFILE_STOP | FLG_LOGFILE_TIMEHIT | FLG_LOGFILE_COMPLETE, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		DBG_IND("uiFlag=0x%x\r\n", uiFlag);
		if (uiFlag & FLG_LOGFILE_STOP) {
			// need to write and close file when exit
			if (pCtrl->state == LOGFILE_STATE_NORMAL) {
				// stop puts log to cache buffer
				pCtrl->state = LOGFILE_STATE_CLOSING;
				for (i = 0; i < pCtrl->LogCoreNum; i++) {
					p_rw_ctrl = &pCtrl->rw_ctrl[i];
					if (p_rw_ctrl->filehdl && (!p_rw_ctrl->hasFileErr)) {
						LogFile_LockWrite();
						LogFile_WriteFile(p_rw_ctrl, p_rw_ctrl->filehdl);
						LogFile_WriteEndTag(p_rw_ctrl, p_rw_ctrl->filehdl);
						// memset buffer
						memset((void*)p_rw_ctrl->pbuf->BufferStartAddr, 0x00, LOG_CLR_BUFFER_SIZE);
						// clear file end to zero
						LogFile_ZeroFileEnd(p_rw_ctrl, p_rw_ctrl->filehdl, p_rw_ctrl->pbuf->BufferStartAddr, LOG_CLR_BUFFER_SIZE);
						LogFile_CloseFile(p_rw_ctrl->filehdl);
						LogFile_UnLockWrite();
					}
				}
			}
			bContinue = 0;
			continue;
		}
		if (uiFlag & FLG_LOGFILE_COMPLETE) {
			// need to write and close file when exit
			if (pCtrl->state == LOGFILE_STATE_SUSPEND) {
				// stop puts log to cache buffer
				pCtrl->state = LOGFILE_STATE_CLOSING;
				for (i = 0; i < pCtrl->LogCoreNum; i++) {
					p_rw_ctrl = &pCtrl->rw_ctrl[i];
					if (p_rw_ctrl->filehdl && (!p_rw_ctrl->hasFileErr)) {
						LogFile_LockWrite();
						LogFile_WriteFile(p_rw_ctrl, p_rw_ctrl->filehdl);
						LogFile_WriteEndTag(p_rw_ctrl, p_rw_ctrl->filehdl);
						// memset buffer
						memset((void*)p_rw_ctrl->pbuf->BufferStartAddr, 0x00, LOG_CLR_BUFFER_SIZE);
						// clear file end to zero
						LogFile_ZeroFileEnd(p_rw_ctrl, p_rw_ctrl->filehdl, p_rw_ctrl->pbuf->BufferStartAddr, LOG_CLR_BUFFER_SIZE);
						LogFile_CloseFile(p_rw_ctrl->filehdl);
						LogFile_UnLockWrite();
					}
				}
				pCtrl->state = LOGFILE_STATE_SUSPEND;
			}
			set_flg(pCtrl->FlagID, FLG_LOGFILE_COMPLETED);
		}
		if (uiFlag & FLG_LOGFILE_TIMEHIT) {
			pCtrl->TimerCnt++;
			if (pCtrl->state != LOGFILE_STATE_NORMAL) {
				continue;
			}

			// first time open ,need to init something
			if (isFirstOpen) {
				isFirstOpen = FALSE;
				for (i = 0; i < pCtrl->LogCoreNum; i++) {
					p_rw_ctrl = &pCtrl->rw_ctrl[i];
					p_rw_ctrl->ClusterSize = FileSys_GetDiskInfo(FST_INFO_CLUSTER_SIZE);
					p_rw_ctrl->CurrFileSize = 0;
					p_rw_ctrl->CurrFileClusCnt = 0;
					p_rw_ctrl->filehdl = LogFile_OpenNewFile(p_rw_ctrl);
					if (p_rw_ctrl->filehdl == NULL) {
						p_rw_ctrl->hasFileErr = TRUE;
					}
				}
			}
			for (i = 0; i < pCtrl->LogCoreNum; i++) {
				p_rw_ctrl = &pCtrl->rw_ctrl[i];
				DBG_IND("core %d pbuf->DataIn 0x%x, pbuf->DataOut = 0x%x\r\n", i, p_rw_ctrl->pbuf->DataIn, p_rw_ctrl->pbuf->DataOut);
				DataSize = LogFile_GetDataSize(p_rw_ctrl->pbuf);
				// datasize > 512 or 4 sec need to write data
				if (p_rw_ctrl->filehdl && (DataSize > 512 || pCtrl->TimerCnt % 4 == 1)) {
					LogFile_LockWrite();
					LogFile_WriteFile(p_rw_ctrl, p_rw_ctrl->filehdl);
					LogFile_UnLockWrite();
				}
			}
		}
	}
	if (isOpenTimerOK) {
		SwTimer_Stop(timer_id);
		SwTimer_Close(timer_id);
	}
	set_flg(pCtrl->FlagID, FLG_LOGFILE_IDLE);
	DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}


THREAD_RETTYPE LogFile_EmrTsk(void)
{
	FLGPTN       uiFlag;
	LOGFILE_CTRL *pCtrl;
	BOOL         bContinue = 1;
	LOGFILE_RW_CTRL *p_rw_ctrl;
	UINT32       i;


	THREAD_ENTRY();

	DBG_FUNC_BEGIN("\r\n");
	pCtrl = LogFile_GetCtrl();
	while (bContinue) {
		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, pCtrl->FlagID, FLG_LOGFILE_STOP_EMR | FLG_LOGFILE_BUFFFULL, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		DBG_IND("uiFlag=0x%x\r\n", uiFlag);
		if (uiFlag & FLG_LOGFILE_STOP_EMR) {
			bContinue = 0;
			continue;
		}
		if (uiFlag & FLG_LOGFILE_BUFFFULL) {
			if (pCtrl->state != LOGFILE_STATE_NORMAL) {
				continue;
			}
			for (i = 0; i < pCtrl->LogCoreNum; i++) {
				p_rw_ctrl = &pCtrl->rw_ctrl[i];
				if (p_rw_ctrl->filehdl) {
					DBG_IND("BEGIN\r\n");
					LogFile_LockWrite();
					LogFile_WriteFile(p_rw_ctrl, p_rw_ctrl->filehdl);
					LogFile_UnLockWrite();
					DBG_IND("END\r\n");
				}
			}

		}
	}
	set_flg(pCtrl->FlagID, FLG_LOGFILE_EMR_IDLE);
	DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}




