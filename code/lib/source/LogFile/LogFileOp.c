/*
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    @file       LogFile.c
    @author     Lincy Lin
    @date       2016/2/1
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "kwrap/perf.h"
#include "FileSysTsk.h"
#include "LogFileID.h"
#include "LogFileNaming.h"
#include "LogFileInt.h"
#include "LogFileOp.h"
#include "LogFile.h"

#define __MODULE__          LogFileOp
#define __DBGLVL__          2
#include "kwrap/debug.h"
extern unsigned int LogFileOp_debug_level;

#if defined(_BSP_NA51000_) || defined(_BSP_NA51023_)
#define DBGLVL_NORMAL       2
#else
#define DBGLVL_NORMAL       1
#endif


CHAR   *logfileBeginTag = "\r\n\r\n\r\n======== LogFile Begin =========\r\n\r\n\r\n";
CHAR   *logfileEndTag   = "\r\n\r\n\r\n======== LogFile End   =========\r\n\r\n\r\n";

void LogFile_LockWrite(void)
{
	vos_sem_wait(LOGFILE_WRITE_SEM_ID);
}

void LogFile_UnLockWrite(void)
{
	vos_sem_sig(LOGFILE_WRITE_SEM_ID);
}

static BOOL LogFile_WriteTag(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl, CHAR *Tag)
{
	UINT32  Wsize;
	INT32   result;

	Wsize = strlen(Tag);
	if (Wsize + p_rw_ctrl->CurrFileSize > p_rw_ctrl->maxFileSize) {
		DBG_WRN("Wsize %d, CurrFileSize %d\r\n", Wsize, p_rw_ctrl->CurrFileSize);
		return FALSE;
	}
	p_rw_ctrl->CurrFileSize += Wsize;
	result = FileSys_WriteFile(filehdl, (UINT8 *)Tag, &Wsize, 0, NULL);
	if (result != FST_STA_OK) {
		DBG_ERR("Write Fail, need to stop\r\n");
		p_rw_ctrl->hasFileErr = TRUE;
		return FALSE;
	}
	return TRUE;
}

static BOOL LogFile_WriteBeginTag(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl)
{
	return LogFile_WriteTag(p_rw_ctrl, filehdl, logfileBeginTag);
}

BOOL LogFile_WriteEndTag(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl)
{
	return LogFile_WriteTag(p_rw_ctrl, filehdl, logfileEndTag);
}

void LogFile_ZeroFileEnd(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl, UINT32 zerobuf, UINT32 zerobufSize)
{
	UINT32  Wsize;
	INT32   result;

	while (1) {
		Wsize = zerobufSize - (p_rw_ctrl->CurrFileSize % zerobufSize);
		DBG_IND("Wsize %d, CurrFileSize %d\r\n", Wsize, p_rw_ctrl->CurrFileSize);
		if (Wsize) {
			if (Wsize + p_rw_ctrl->CurrFileSize > p_rw_ctrl->maxFileSize) {
				DBG_WRN("Wsize %d, CurrFileSize %d\r\n", Wsize, p_rw_ctrl->CurrFileSize);
				return;
			}
			p_rw_ctrl->CurrFileSize += Wsize;
			result = FileSys_WriteFile(filehdl, (UINT8 *)zerobuf, &Wsize, 0, NULL);
			if (result != FST_STA_OK) {
				return;
			}
		}
		if (p_rw_ctrl->CurrFileSize >= p_rw_ctrl->maxFileSize) {
			return;
		}
	}
}

FST_FILE LogFile_OpenNewFile(LOGFILE_RW_CTRL *p_rw_ctrl)
{
	CHAR      filename[LOGFILE_ROOT_DIR_MAX_LEN + MAX_FILE_NAME_LEN + 1];
	FST_FILE  filehdl = NULL;
	UINT32    flag;
	BOOL      ret;

	ret = LogFileNaming_GetNewPath(p_rw_ctrl->p_nameobj, filename, sizeof(filename));
	if (ret == FALSE) {
		return NULL;
	}
	// 1. create new file
	if (p_rw_ctrl->isZeroFile) {
		flag = FST_CREATE_ALWAYS | FST_OPEN_WRITE;
	} else {
		flag = FST_OPEN_ALWAYS | FST_OPEN_WRITE;
	}
#if LOGFILE_USE_CLOEXEC
	/* attach close-on-exec flag to fix umount issue of formatting */
	{
		flag |= FST_CLOSE_ON_EXEC;
	}
#endif
	filehdl = FileSys_OpenFile(filename, flag);
	if (!filehdl) {
		DBG_ERR("Open file %s fail\r\n", filename);
		return NULL;
	}
	DBG_DUMP("LogFile Crefile %s \r\n", filename);
#if LOGFILE_USE_TRUNCATE
	{
		INT32         result;
#if __DBGLVL__ > DBGLVL_NORMAL
		VOS_TICK      t1, t2;
#endif
		if (!p_rw_ctrl->isPreAllocAllFiles || p_rw_ctrl->isZeroFile) {
#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t1);
#endif
			// size > 1MB, need to use alloc instead of truncate
			if (p_rw_ctrl->maxFileSize > 0x100000 && !p_rw_ctrl->isZeroFile) {
				result = FileSys_AllocFile(filehdl, p_rw_ctrl->maxFileSize, 0, 0);
			} else {
				result = FileSys_TruncFile(filehdl, p_rw_ctrl->maxFileSize);
			}
#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t2);
#endif
			DBG_IND("Trunc FileSize %d, time = %d ms\r\n", p_rw_ctrl->maxFileSize, (t2 - t1) / 1000);
			if (result != FST_STA_OK) {
				DBG_ERR("Alloc Fail, need to stop\r\n");
				p_rw_ctrl->hasFileErr = TRUE;
				FileSys_CloseFile(filehdl);
				return NULL;
			}
			FileSys_FlushFile(filehdl);
		}
		FileSys_SeekFile(filehdl, 0, FST_SEEK_SET);
#if 1
		LogFile_WriteBeginTag(p_rw_ctrl, filehdl);
#endif
	}
#else
	FileSys_SeekFile(filehdl, 0, FST_SEEK_SET);
#endif
	return filehdl;
}

void LogFile_CloseFile(FST_FILE  filehdl)
{
	FileSys_CloseFile(filehdl);
}



static BOOL LogFile_CutFile(LOGFILE_RW_CTRL *p_rw_ctrl)
{
	// write end tag
	LogFile_WriteEndTag(p_rw_ctrl, p_rw_ctrl->filehdl);
	FileSys_CloseFile(p_rw_ctrl->filehdl);
	p_rw_ctrl->CurrFileSize = 0;
	p_rw_ctrl->CurrFileClusCnt = 0;
	p_rw_ctrl->filehdl = LogFile_OpenNewFile(p_rw_ctrl);
	if (p_rw_ctrl->filehdl == NULL) {
		p_rw_ctrl->hasFileErr = TRUE;
		return FALSE;
	}
	return TRUE;
}

static void LogFile_FlushFile(LOGFILE_RW_CTRL *p_rw_ctrl)
{
	UINT32    newClusCnt = 0;
#if __DBGLVL__ > DBGLVL_NORMAL
	VOS_TICK  t1, t2;
#endif
	// check if need to flush file
	if (p_rw_ctrl->ClusterSize) {
		newClusCnt = ALIGN_CEIL(p_rw_ctrl->CurrFileSize, p_rw_ctrl->ClusterSize) / p_rw_ctrl->ClusterSize;
		// new cluster added
		if (newClusCnt > p_rw_ctrl->CurrFileClusCnt) {
			DBG_IND("newClusCnt = %d, CurrFileClusCnt =%d, Need flush\r\n", newClusCnt, p_rw_ctrl->CurrFileClusCnt);
#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t1);
#endif
			FileSys_FlushFile(p_rw_ctrl->filehdl);
#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t2);
#endif
			DBG_IND("Flush time = %d ms\r\n", (t2 - t1) / 1000);
			p_rw_ctrl->CurrFileClusCnt = newClusCnt;
		}
	}
}

BOOL LogFile_WriteFile(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl)
{
	UINT32            Wsize = 0, remainFileSize;
	LOGFILE_DUMP_ER   dump_ret;

	if (p_rw_ctrl->CurrFileSize + sizeof(logfileEndTag) > p_rw_ctrl->maxFileSize) {
		if (LogFile_CutFile(p_rw_ctrl) == FALSE)
			return FALSE;
	}
	remainFileSize = p_rw_ctrl->maxFileSize-p_rw_ctrl->CurrFileSize-sizeof(logfileEndTag);
	dump_ret = LogFile_DumpData(p_rw_ctrl,0,remainFileSize,&Wsize);
	if (dump_ret == LOGFILE_DUMP_ER_BUFF2SMALL){
		if (LogFile_CutFile(p_rw_ctrl) == FALSE)
			return FALSE;
		remainFileSize = p_rw_ctrl->maxFileSize-p_rw_ctrl->CurrFileSize-sizeof(logfileEndTag);
		dump_ret = LogFile_DumpData(p_rw_ctrl,0,remainFileSize,&Wsize);
		if (dump_ret < 0)
			return FALSE;
	}
	if (Wsize) {
		p_rw_ctrl->CurrFileSize += Wsize;
		LogFile_FlushFile(p_rw_ctrl);
	}
	return TRUE;
}

LOGFILE_DUMP_ER LogFile_DumpData(LOGFILE_RW_CTRL *p_rw_ctrl, UINT32 bufAddr, UINT32 bufSize, UINT32 *dumpSize)
{
	LOGFILE_DUMP_ER ret = LOGFILE_DUMP_ER_FINISH;
	UINT32          Wsize_total = 0, wsize1, wsize2;
	BOOL            bIsWriteFile = FALSE;
	LOGFILE_RINGBUF_HEAD *pbuf = p_rw_ctrl->pbuf;
#if __DBGLVL__ > DBGLVL_NORMAL
	VOS_TICK  t1, t2;
#endif

	if (p_rw_ctrl->filehdl == NULL && bufAddr == 0) {
		return LOGFILE_DUMP_ER_PARM;
	}
	if (p_rw_ctrl->filehdl != NULL) {
		bIsWriteFile = TRUE;
	}
	if (pbuf->DataIn == pbuf->DataOut) {
		*dumpSize = 0;
		return ret;
	}
	if (pbuf->DataIn > pbuf->DataOut) {
		Wsize_total = pbuf->DataIn - pbuf->DataOut;
	} else {
		Wsize_total = pbuf->BufferSize + pbuf->DataIn - pbuf->DataOut;
	}
	if (Wsize_total > bufSize){
		return LOGFILE_DUMP_ER_BUFF2SMALL;
	}
	if (pbuf->DataIn > pbuf->DataOut) {
		if (bIsWriteFile) {
			#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t1);
			#endif
			if (FileSys_WriteFile(p_rw_ctrl->filehdl, (UINT8 *)(pbuf->DataOut + pbuf->BufferStartAddr), &Wsize_total, 0, NULL) != FST_STA_OK) {
				DBG_ERR("Write Fail, need to stop\r\n");
				return LOGFILE_DUMP_ER_WRITE_FILE;
			}
			#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t2);
			DBG_IND("I=0x%x,O=0x%x,FSize = %u, Wsize_total %u, time = %d ms\r\n", pbuf->DataIn, pbuf->DataOut, p_rw_ctrl->CurrFileSize, Wsize_total, (t2 - t1) / 1000);
			#endif
		} else {
			memcpy((void*)bufAddr, (UINT8 *)(pbuf->DataOut + pbuf->BufferStartAddr), Wsize_total);
		}
		pbuf->DataOut += Wsize_total;
	} else {
		wsize1 = pbuf->BufferSize - pbuf->DataOut;
		if (bIsWriteFile) {
			#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t1);
			#endif
			if (FileSys_WriteFile(p_rw_ctrl->filehdl, (UINT8 *)(pbuf->DataOut + pbuf->BufferStartAddr), &wsize1, 0, NULL) != FST_STA_OK) {
				DBG_ERR("Write Fail, need to stop\r\n");
				return LOGFILE_DUMP_ER_WRITE_FILE;
			}
			wsize2 = Wsize_total - wsize1;
			pbuf->DataOut = 0;
			if (FileSys_WriteFile(p_rw_ctrl->filehdl, (UINT8 *)(pbuf->DataOut + pbuf->BufferStartAddr), &wsize2, 0, NULL) != FST_STA_OK) {
				DBG_ERR("Write Fail, need to stop\r\n");
				return LOGFILE_DUMP_ER_WRITE_FILE;
			}
			#if __DBGLVL__ > DBGLVL_NORMAL
			vos_perf_mark(&t2);
			DBG_IND("I=0x%x,O=0x%x,FSize = %u, Wsize_total %u, time = %d ms\r\n", pbuf->DataIn, pbuf->DataOut, p_rw_ctrl->CurrFileSize, Wsize_total, (t2 - t1) / 1000);
			#endif
			pbuf->DataOut += wsize2;
		} else {
			memcpy((void*)bufAddr, (UINT8 *)(pbuf->DataOut + pbuf->BufferStartAddr), wsize1);
			wsize2 = Wsize_total - wsize1;
			pbuf->DataOut = 0;
			memcpy((void*)bufAddr, (UINT8 *)(pbuf->DataOut + pbuf->BufferStartAddr), wsize2);
			pbuf->DataOut += wsize2;
		}
	}
	*dumpSize = Wsize_total;
	return ret;
}

void LogFile_SaveLastTimeSysErrLog(UINT32 buffAddr, UINT32 buffSize, CHAR  *filename)
{
	FST_FILE  				filehdl;
	CHAR      				temp_str[50];
	UINT32    				wSize;
	LOGFILE_DATA_HEAD	  	*data_head;
	BOOL                    sizeErr = FALSE;

	DBG_FUNC_BEGIN("\r\n");
	data_head = (LOGFILE_DATA_HEAD	 *)buffAddr;
	if (data_head->tag  != CFG_LOGFILE_INIT_KEY){
		DBG_WRN("tag 0x%x != 0x%x\r\n",data_head->tag,CFG_LOGFILE_INIT_KEY);
	}
	if (data_head->size + sizeof(LOGFILE_DATA_HEAD) > buffSize) {
		DBG_WRN("logsize 0x%x + header 0x%x > buffSize 0x%x\r\n",data_head->size,sizeof(LOGFILE_DATA_HEAD),buffSize);
		sizeErr = TRUE;
	}
	if ((UINT32)data_head->data != buffAddr +sizeof(LOGFILE_DATA_HEAD)) {
		DBG_WRN("data Addr 0x%x != buffAddr 0x%x + header 0x%x\r\n", (int)data_head->data,buffAddr, sizeof(LOGFILE_DATA_HEAD));
	}
	filehdl = FileSys_OpenFile(filename, FST_OPEN_ALWAYS | FST_OPEN_WRITE);
	if (!filehdl) {
		DBG_ERR("Open file %s fail\r\n", filename);
		return;
	}
	//debug_dumpmem(buffAddr+buffSize-0x100,0x100);
	wSize = strlen(logfileBeginTag);
	if (FileSys_WriteFile(filehdl, (UINT8 *)logfileBeginTag, &wSize, 0, NULL) != FST_STA_OK) {
		DBG_ERR("Write file %s fail\r\n", filename);
		goto write_fail;
	}
	snprintf(temp_str, sizeof(temp_str), "Tag=0x%lx,Addr=0x%lx,Size=0x%lx\r\n",(unsigned long)data_head->tag, (unsigned long)data_head->data,(unsigned long)data_head->size);
	wSize = strlen(temp_str);
	if (FileSys_WriteFile(filehdl, (UINT8 *)temp_str, &wSize, 0, NULL) != FST_STA_OK) {
		DBG_ERR("Write file %s fail\r\n", filename);
		goto write_fail;
	}
	if (sizeErr == TRUE) {
		wSize = buffSize-sizeof(LOGFILE_DATA_HEAD);
	} else {
		wSize = data_head->size;
	}
	DBG_DUMP("LogFile_SaveLastTimeSysErrLog() %s Addr = 0x%x, len = 0x%x\r\n", filename,buffAddr +sizeof(LOGFILE_DATA_HEAD) ,wSize);
	if (FileSys_WriteFile(filehdl, (UINT8 *)buffAddr +sizeof(LOGFILE_DATA_HEAD), &wSize, 0, NULL) != FST_STA_OK) {
		DBG_ERR("Write file %s fail\r\n", filename);
		goto write_fail;
	}
	wSize = strlen(logfileEndTag);
	if (FileSys_WriteFile(filehdl, (UINT8 *)logfileEndTag, &wSize, 0, NULL) != FST_STA_OK) {
		DBG_ERR("Write file %s fail\r\n", filename);
		goto write_fail;
	}
	// zero file end
	if (buffSize >= LOG_CLR_BUFFER_SIZE) {
		wSize = LOG_CLR_BUFFER_SIZE;
		memset((void*)buffAddr, 0x00, wSize);
		if (FileSys_WriteFile(filehdl, (UINT8 *)buffAddr, &wSize, 0, NULL) != FST_STA_OK) {
			DBG_ERR("Write file %s fail\r\n", filename);
			goto write_fail;
		}
	}
write_fail:
	FileSys_CloseFile(filehdl);
	DBG_FUNC_END("\r\n");
}

