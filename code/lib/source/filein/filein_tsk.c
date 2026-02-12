/*
    Novatek Media Play File In Task.

    This file is the task of novatek media player file in.

    @file       NMediaPlayFileInTsk.c
    @ingroup    mIAPPMEDIAPLAY
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined (__UITRON) || defined (__ECOS)
#include "filein.h"
#include "ImageStream.h"
#include "ImageUnit_FileIn.h"
#include "media_def.h"
#include "NvtMediaInterface.h"
#include "NMediaPlayFileIn.h"
#include "NMediaPlayFileInTsk.h"
#include "NMediaPlayBsDemux.h"
#include "NMediaPlayAPI.h"
#include "SxCmd.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          NMediaPlayFileIn
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////
#else
#include "filein_tsk.h"
#include "avfile/media_def.h"

#include "kwrap/task.h"
#include "kwrap/semaphore.h"

#include "kwrap/debug.h"
#include "kwrap/error_no.h"
#include "hd_type.h"
#include "hd_common.h"
#endif

#define FILE_IN_TEST        0

static FILEIN_OBJ               gFileInObj = {0};
static FILEIN_FILEREAD_INFO     gFileInFileReadInfo = {0};
static FILEIN_BLK_INFO      gFileInBlkInfo = {0};
//static FILEIN_JOBQ              gFileInJobQ = {0};
static FILEIN_USERBUF_INFO      gFileInUserBufInfo = {0}; // user customized items

// TS info
//static MEM_RANGE            gFileInAllocMemRange[FILEIN_MAX_PATH] = {0};

static UINT8                        gFileInOpened = FALSE;

// Task, flags, semaphore
THREAD_HANDLE                       FILEIN_TSK_ID = 0;
UINT32                              FILEIN_FLG_ID = 0;
UINT32                              FILEIN_SEM_ID[FILEIN_MAX_PATH] = {0};
#if defined(__LINUX)
sem_t                               FILEIN_COMMON_SEM_ID;
#else
ID                                  FILEIN_COMMON_SEM_ID;
#endif
UINT32                              gFileInMemBlk[FILEIN_MAX_PATH] = {0};
UINT32                              gFileInMemPhyAddr[FILEIN_MAX_PATH] = {0};
UINT32                              gFileInMemVirtAddr[FILEIN_MAX_PATH] = {0};

// Callback event
FileInEventCb                   *gfnFileIn_CB = NULL;

// Debug
static UINT32                       gFileInDebugLevel = 0;
#define FILEIN_DUMP(fmtstr, args...) if (gFileInDebugLevel) DBG_DUMP(fmtstr, ##args)
#if 0
// install task
void _FileIn_install_id(void)
{
	//OS_CONFIG_SEMPHORE(FILEIN_COMMON_SEM_ID, 0, 1, 1);
	SEM_CREATE(FILEIN_COMMON_SEM_ID, 1);
}

void _FileIn_uninstall_id(void)
{
	//OS_CONFIG_SEMPHORE(FILEIN_COMMON_SEM_ID, 0, 1, 1);
	SEM_DESTROY(FILEIN_COMMON_SEM_ID);
}
#endif
static ER _FileIn_AllocMem(UINT32 id)
{
	FILEIN_MEM_RANGE	Mem 		= {0};
	UINT32               pa = 0, va = 0, size = 0;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_COMMON_MEM_VB_BLK blk;

	FileIn_GetParam(id, FILEIN_PARAM_NEED_MEMSIZE, &size);

	if (size == 0) {
		DBG_ERR("Get port %d buffer from FileIn is Zero!\r\n", id);
		return E_PAR;
	}

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("get block fail, blk = 0x%x\n", blk);
		return E_SYS;
	}

	gFileInMemBlk[id] = blk;

	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk(0x%x)\n", blk);
		return E_SYS;
	}
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size); // Get virtual addr
		if (va == 0) {
			DBG_ERR("get va fail, va(0x%x)\n", blk);
			return E_SYS;
		}
	}

	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));

	Mem.Addr = (UINT32)va;
	Mem.Size = size;

	gFileInMemPhyAddr[id] = (UINT32)pa;
	gFileInMemVirtAddr[id] = (UINT32)va;

//	if (gFileInObj.file_fmt == MEDIA_FILEFORMAT_TS) {
//		gFileInAllocMemRange[id].addr = Mem.Addr;
//		gFileInAllocMemRange[id].size = Mem.Size;
//	}

	FileIn_SetParam(id, FILEIN_PARAM_MEM_RANGE, (UINT32) &Mem);

	return E_OK;

}

static ER _FileIn_FreeMem(UINT32 id)
{
	HD_RESULT            ret;
	UINT32               size = 0;

	FileIn_GetParam(id, FILEIN_PARAM_NEED_MEMSIZE, &size);

	// mummap
	ret = hd_common_mem_munmap((void*)gFileInMemVirtAddr[id], size);
	if (ret != HD_OK) {
		printf("FileIn mnumap error(%d) !!\r\n\r\n", ret);
	}

	ret = hd_common_mem_release_block(gFileInMemBlk[id]);
	if (ret != HD_OK) {
		DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
		return E_SYS;
	}

	return E_OK;
}

//#NT#use bitstream size to calculate block size
static UINT64 _NMP_FileIn_GetBlockSize(UINT64 blk_time)
{
	UINT32 vdo_preld_last_idx = 0, aud_preld_last_idx = 0;
	UINT32 vdo_bs_offset = 0, vdo_bs_size = 0;
	UINT32 aud_bs_offset = 0, aud_bs_size = 0;
	UINT64 blk_size = 0, max_blk_size = 0;
	CONTAINERPARSER *file_container = NULL;

	if (gFileInUserBufInfo.user_define_blk) {
		if (gFileInUserBufInfo.blk_size == 0) {
			DBG_ERR("user define filein blk, but not set blk size %d\r\n", gFileInUserBufInfo.blk_size);
			goto error;
		}
		blk_size = (UINT64)gFileInUserBufInfo.blk_size;
	} else {
		if (gFileInObj.file_container == NULL) {
			DBG_ERR("file_container is NULL\r\n");
			goto error;
		}
		file_container = gFileInObj.file_container;
		max_blk_size = NMP_FILEIN_BUFBLK_MAXSIZE;

		vdo_preld_last_idx = gFileInObj.vdo_fr - 1;
		if (gFileInObj.vdo_ttfrm <= gFileInObj.vdo_fr) {
			vdo_preld_last_idx = gFileInObj.vdo_ttfrm - 1;
		}
		(file_container->GetInfo)(MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE, &vdo_preld_last_idx, &vdo_bs_offset, &vdo_bs_size);

		aud_preld_last_idx = gFileInObj.aud_fr - 1;
		if (gFileInObj.aud_ttfrm <= gFileInObj.aud_fr) {
			aud_preld_last_idx = gFileInObj.aud_ttfrm - 1;
		}
		(file_container->GetInfo)(MEDIAREADLIB_GETINFO_GETAUDIOPOSSIZE, &aud_preld_last_idx, &aud_bs_offset, &aud_bs_size);

		if ((vdo_bs_offset+vdo_bs_size) > (aud_bs_offset+aud_bs_size)) {
			blk_size = (vdo_bs_offset+vdo_bs_size)/2;
		} else {
			blk_size = (aud_bs_offset+aud_bs_size)/2;
		}

		if (blk_size == 0) {
			DBG_ERR("blk_size = 0\r\n");
			goto error;
		}

		if (blk_size > max_blk_size) {
			DBG_WRN("block size limit of 0x%llx, but estimate blk_size is 0x%llx. Set block size as 0x%llx\r\n", max_blk_size, blk_size, max_blk_size);
			blk_size = max_blk_size;
		}
	}
	blk_size = ALIGN_CEIL_32(blk_size * blk_time); // buffer size per blk_time
	FILEIN_DUMP("new blk_size = %lld\r\n", blk_size);

	return blk_size;

error:
	return 0;
}

static UINT32 _FileIn_GetBufSize(UINT32 pathID)
{
	UINT64 ttlsize, ttldur, blk_time, ttl_blk_num;
	UINT64 u64blk_size;
	UINT32 u32blk_size, total_need_buf;

	ttlsize = gFileInObj.file_ttlsize;
	ttldur = (UINT64)gFileInObj.file_ttldur; // unit: ms
	if (gFileInUserBufInfo.blk_time == 0) {
		// use default value instead
		gFileInUserBufInfo.blk_time = FILEIN_BLK_TIME;
	}
	blk_time = (UINT64)gFileInUserBufInfo.blk_time;
	if (gFileInUserBufInfo.tt_blknum == 0) {
		// use default value instead
		gFileInUserBufInfo.tt_blknum = FILEIN_BUFBLK_CNT;
	}
	ttl_blk_num = gFileInUserBufInfo.tt_blknum;

	if (ttldur == 0) {
		DBG_ERR("ttldur = %d\r\n", ttldur);
		return 0;
	}

	// calculate block size
	if (gFileInObj.file_fmt == MEDIA_FILEFORMAT_TS) {
		u32blk_size = FILEIN_TS_BLK_SIZE;  // use default
		gFileInFileReadInfo.blk_size = u32blk_size;
	} else {
#if 1
		u64blk_size = _NMP_FileIn_GetBlockSize(blk_time);
#else
		u64blk_size = ALIGN_CEIL_32((ttlsize * blk_time * 1000) / ttldur); // buffer size per blk_time
#endif

		// check UINT32 overflow
		u32blk_size = (UINT32)u64blk_size;
		if ((UINT64)u32blk_size != u64blk_size) {
			DBG_ERR("u32blk_size is overflow\r\n");
			return 0;
		} else {
			gFileInFileReadInfo.blk_size = u32blk_size;
		}
	}

	FILEIN_DUMP("[FileIn] GetBufSize: blk_size=0x%x, ttlsize=0x%llx, ttldur=0x%llx, blk_time=0x%llx, ttl_blk_num=%d\r\n",
			u32blk_size, ttlsize, ttldur, blk_time, ttl_blk_num);

	total_need_buf = u32blk_size * ttl_blk_num + FILEIN_BUFBLK_RESERVESIZE; // total need buffer size

	FILEIN_DUMP("[FileIn] total_need_buf = 0x%x\r\n", total_need_buf);

	return total_need_buf;
}

/* search demux target bs in which block */
static UINT32 _FileIn_SearchWhichBlock(UINT64 offset)
{
	FILEIN_FILEREAD_INFO *p_info = &gFileInFileReadInfo;
	FILEIN_USERBUF_INFO  *p_user_info = &gFileInUserBufInfo;
	UINT32 addrInMem, fbk, blk_idx = 0;

	FILEIN_DUMP("[FileIn] SearchWhichBlock, off=0x%llx\r\n", offset);

	addrInMem = p_info->start_addr + (offset % (p_info->blk_size * p_user_info->tt_blknum));

	for (fbk = p_user_info->tt_blknum; fbk > FILEIN_BUFBLK_0; fbk--) {
		//DBG_DUMP("^C fbk=%d, next_i=%d, a=0x%x, blk_a=0x%x\r\n", fbk, p_info->next_blk_idx, addrInMem, p_info->ReadBlk[fbk-1].addr_in_mem);
		if (addrInMem >= p_info->ReadBlk[fbk-1].addr_in_mem) {
			blk_idx = fbk - 1;
			break;
		}
	}

	return blk_idx;
}

static ER _FileIn_InitBufInfo(void)
{
	UINT32 MemAddr;
	UINT32 i;

	if (gFileInObj.mem.Addr == 0 || gFileInObj.mem.Size == 0) {
		DBG_ERR("Mem addr=0x%x, size=%d\r\n", gFileInObj.mem.Addr, gFileInObj.mem.Size);
		return E_PAR;
	}
	MemAddr = gFileInObj.mem.Addr;

	if (gFileInObj.file_ttldur == 0) {
		DBG_ERR("ttldur = %d\r\n", gFileInObj.file_ttldur);
		return E_PAR;
	}

	// check preload block number
	if (gFileInObj.file_fmt == MEDIA_FILEFORMAT_TS) {
		gFileInUserBufInfo.pl_blknum = FILEIN_TS_PL_BLKNUM; // TS always use default
	} else {
		if (gFileInUserBufInfo.pl_blknum == 0) {
			gFileInUserBufInfo.pl_blknum = FILEIN_PL_BLKNUM; // use default
		}
	}

	// check reserve block number
	if (gFileInUserBufInfo.rsv_blknum == 0) {
		// use default value instead
		gFileInUserBufInfo.rsv_blknum = FILEIN_RSV_BLKNUM;
	}
	if (gFileInFileReadInfo.blk_size == 0) {
		DBG_ERR("blk_size = 0\r\n");
		return E_PAR;
	}

	gFileInFileReadInfo.start_addr = MemAddr;
	gFileInFileReadInfo.end_addr = MemAddr + gFileInFileReadInfo.blk_size * gFileInUserBufInfo.tt_blknum;
	gFileInFileReadInfo.next_blk_idx = FILEIN_BUFBLK_0;
	gFileInFileReadInfo.next_file_pos = 0;
	gFileInFileReadInfo.read_accum_offset = 0;
	gFileInFileReadInfo.demux_offset = 0;
	gFileInFileReadInfo.vdodec_bs.addr = 0;
	gFileInFileReadInfo.vdodec_bs.size = 0;
	gFileInFileReadInfo.read_finish = FALSE;

	gFileInBlkInfo.start_addr = gFileInFileReadInfo.start_addr;
	gFileInBlkInfo.blk_size = gFileInFileReadInfo.blk_size;
	gFileInBlkInfo.tt_blknum = gFileInUserBufInfo.tt_blknum;

	for (i = FILEIN_BUFBLK_0; i < gFileInUserBufInfo.tt_blknum; i++) {
		gFileInFileReadInfo.ReadBlk[i].addr_in_mem = MemAddr;
		gFileInFileReadInfo.ReadBlk[i].file_offset = 0xFFFFFFFFFFFFFFFFULL;
		MemAddr += gFileInFileReadInfo.blk_size;
	}

	return E_OK;
}

static ER _FileIn_TskStart(void)
{
	// start tsk
	//THREAD_CREATE(FILEIN_TSK_ID, _FileIn_Tsk, NULL, "_FileIn_Tsk");
	//if (FILEIN_TSK_ID == 0) {
	//	DBG_ERR("Invalid NMP_BSDEMUX_TSK_ID\r\n");
	//	return E_OACV;
	//}
	//THREAD_RESUME(FILEIN_TSK_ID);
	//BSDMX_DUMP("[bsdemux]bsdemux task start...\r\n");

	return E_OK;
}

/**
    File Read Function

	param  [in]  pos:   read file offset. (point to where you want to read)
    param  [in]  size:  read file size.
    param  [in]  addr:  read file output buffer address.

    @return ER
*/
static ER _FileIn_ReadFile(UINT64 pos, UINT32 size, UINT32 addr)
{
	INT32    ret;
    UINT8   *pbuf        = (UINT8 *)addr;
   	UINT64   totalSize   = gFileInObj.file_ttlsize;
#if FILE_IN_TEST
		UINT32 i = 0;
#else
    FST_FILE file_handle = gFileInObj.file_handle;
#endif

	// check boundary
	if (pos > totalSize) {
		DBG_WRN("pos(0x%llx) > totalSize(0x%llx)\r\n", pos, totalSize);
		gFileInFileReadInfo.read_finish = TRUE;
		return E_SYS;
	}
	if ((pos + (UINT64)size) > totalSize) {
		DBG_IND("pos(0x%llx)+size(0x%x) > totalSize(0x%llx)\r\n", pos, size, totalSize);
		size = ALIGN_CEIL_32(totalSize - pos);
	}

	if (pos == totalSize && size == 0) {
		return E_OK;
	}

#if FILE_IN_TEST
	for (i=0; i<size; i++) {
		*(pbuf+pos+i) = 0xAA;
		ret = E_OK;
		 return ret;
	}
#else
	// read file
#if defined(__LINUX)
	sem_wait(&FILEIN_COMMON_SEM_ID);
#else
	vos_sem_wait(FILEIN_COMMON_SEM_ID);
#endif
	ret = FileSys_SeekFile(file_handle, pos, FST_SEEK_SET);
	if (ret != E_OK) {
		DBG_ERR("FileSys Seek File is FAILS, handle = 0x%x, ret = %d\r\n", file_handle, ret);
#if defined(__LINUX)
		sem_post(&FILEIN_COMMON_SEM_ID);
#else
		vos_sem_sig(FILEIN_COMMON_SEM_ID);
#endif
		return ret;
	}

	ret = FileSys_ReadFile(file_handle, pbuf, &size, 0, NULL);
	if (ret != E_OK) {
		DBG_ERR("FileSys Read File is FAILS, handle = 0x%x, ret = %d\r\n", file_handle, ret);
#if defined(__LINUX)
		sem_post(&FILEIN_COMMON_SEM_ID);
#else
		vos_sem_sig(FILEIN_COMMON_SEM_ID);
#endif
		return ret;
	}
#if defined(__LINUX)
	sem_post(&FILEIN_COMMON_SEM_ID);
#else
	vos_sem_sig(FILEIN_COMMON_SEM_ID);
#endif
#endif
    return E_OK;
}

static ER _FileIn_ReadOneBlkBuf(FILEIN_STATE state)
{
    ER     ret;
	UINT32 blk_size, addr_in_mem, blk_idx;
	UINT64 file_pos;

	if (gFileInFileReadInfo.read_finish) {
		return E_OK;
	}

	blk_idx  = gFileInFileReadInfo.next_blk_idx;
	addr_in_mem = gFileInFileReadInfo.ReadBlk[blk_idx].addr_in_mem;
	blk_size = gFileInFileReadInfo.blk_size;
	file_pos = gFileInFileReadInfo.next_file_pos;

	FILEIN_DUMP("[FileIn]ReadOneBlk: pos=0x%llx, size=0x%x, addr=0x%x, idx=%d\r\n", file_pos, blk_size, addr_in_mem, blk_idx);

    ret = _FileIn_ReadFile(file_pos, blk_size, addr_in_mem); // (offset, size, buf)

    if (ret == E_OK) {
		// update information
		gFileInFileReadInfo.ReadBlk[blk_idx].file_offset = file_pos;

		blk_idx++;
		if (blk_idx == gFileInUserBufInfo.tt_blknum) {
			blk_idx = 0;
		}
		gFileInFileReadInfo.next_blk_idx = blk_idx;

		file_pos += (UINT64)blk_size;
		gFileInFileReadInfo.next_file_pos = file_pos;
		gFileInFileReadInfo.read_accum_offset += (UINT64)blk_size;

		// check read file finished
		if (gFileInFileReadInfo.read_accum_offset > gFileInObj.file_ttlsize) {
			//DBG_DUMP("^C read file finished!\r\n");
			gFileInFileReadInfo.read_finish = TRUE;
		}
		//DBG_DUMP("read_accum=0x%llx\r\n", gFileInFileReadInfo.read_accum_offset);

		// callback read 1 block for TS
		if (gFileInObj.file_fmt == MEDIA_FILEFORMAT_TS && state == FILEIN_STATE_NORMAL) {
			FILEIN_READBUF_INFO read_info = {0};
			read_info.addr = addr_in_mem;
			read_info.size = blk_size;
			read_info.read_state = state;
			if (gfnFileIn_CB) {
				gfnFileIn_CB("FileIn", ISF_FILEIN_EVENT_READ_ONE_BLK, (UINT32)&read_info);
			}
		}
	} else {
		if (!gFileInFileReadInfo.read_finish) {
			DBG_ERR("file read error!!! pos=0x%llx size=0x%x\r\n", file_pos, blk_size);
		}
		return ret;
	}

    return ret;
}

/*static ER _FileIn_PutJob(UINT32 pathID)
{
	wai_sem(FILEIN_SEM_ID[pathID]);

	if ((gFileInJobQ.front == gFileInJobQ.rear) && (gFileInJobQ.bfull == TRUE)) {
		sig_sem(FILEIN_SEM_ID[pathID]);
		static UINT32 sCount;
		if (sCount % 128 == 0) {
			DBG_WRN("[FILEIN] Job Queue is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gFileInJobQ.queue[gFileInJobQ.rear] = pathID;
		gFileInJobQ.rear = (gFileInJobQ.rear + 1) % FILEIN_JOBQ_MAX;
		if (gFileInJobQ.front == gFileInJobQ.rear) { // check queue full
			gFileInJobQ.bfull = TRUE;
		}
		sig_sem(FILEIN_SEM_ID[pathID]);
		return E_OK;
	}
}

static ER _FileIn_GetJob(UINT32 *pPathID)
{
	wai_sem(FILEIN_COMMON_SEM_ID);
	if ((gFileInJobQ.front == gFileInJobQ.rear) && (gFileInJobQ.bfull == FALSE)) {
		sig_sem(FILEIN_COMMON_SEM_ID);
		DBG_ERR("[FILEIN] Job Queue is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gFileInJobQ.queue[gFileInJobQ.front];
		gFileInJobQ.front = (gFileInJobQ.front + 1) % FILEIN_JOBQ_MAX;
		if (gFileInJobQ.front == gFileInJobQ.rear) { // check queue full
			gFileInJobQ.bfull = FALSE;
		}
		sig_sem(FILEIN_COMMON_SEM_ID);
		return E_OK;
	}
}

static UINT32 _FileIn_GetJobCount(void)
{
	UINT32 front, rear, full, sq = 0;

	wai_sem(FILEIN_COMMON_SEM_ID);
	front = gFileInJobQ.front;
	rear = gFileInJobQ.rear;
	full = gFileInJobQ.bfull;
	sig_sem(FILEIN_COMMON_SEM_ID);

	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = FILEIN_JOBQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {     // job queue full
		sq = FILEIN_JOBQ_MAX;
	} else {                                        // job queue empty
		sq = 0;
	}

	return sq;
}*/

static BOOL  _FileIn_ChkTargetFileOffset(UINT64 offset)
{
	FILEIN_FILEREAD_INFO *p_info = &gFileInFileReadInfo;
	FILEIN_USERBUF_INFO  *p_user_info = &gFileInUserBufInfo;
	UINT32 curr_blk_id = _FileIn_SearchWhichBlock(gFileInFileReadInfo.next_file_pos);
	UINT32 curr_ring_cnt;
	UINT32 target_blk_id = _FileIn_SearchWhichBlock(offset);
	UINT32 target_ring_cnt;
	UINT64 buf_size = p_info->blk_size * p_user_info->tt_blknum;

	target_ring_cnt = offset / buf_size;
	curr_ring_cnt = (gFileInFileReadInfo.next_file_pos  / buf_size) - ((gFileInFileReadInfo.next_file_pos % buf_size) ? 0 : 1);

	if(!curr_blk_id)
		curr_blk_id = p_user_info->tt_blknum - 1;
	else
		curr_blk_id = p_info->next_blk_idx - 1;

	FILEIN_DUMP("target bs(ring_cnt = %lu, blk_id = %lu) curr bs(ring_cnt = %lu , blk_id = %lu)\n",
			target_ring_cnt,
			target_blk_id,
			curr_ring_cnt,
			curr_blk_id);

	if(((curr_ring_cnt - target_ring_cnt == 1) && (curr_blk_id >= target_blk_id)) ||
	   (curr_ring_cnt - target_ring_cnt > 1)
	){
		DBG_ERR("offset(%llx) is behind and out of filein ring buffer!\n", offset);
		return FALSE;
	}

	return TRUE;
}


static ER _FileIn_ChkFileRead(UINT64 offset, UINT32 size)
{
	FILEIN_FILEREAD_INFO *p_info = &gFileInFileReadInfo;
	FILEIN_USERBUF_INFO  *p_user_info = &gFileInUserBufInfo;
	UINT64 remain, reserve;
	UINT32 blk_idx, blk_end;
	UINT32 frm_start, frm_end;
	BOOL   b_read_next;

	if(_FileIn_ChkTargetFileOffset(offset) == FALSE)
		return E_SYS;

	// update demux offset
	p_info->demux_offset = offset;

	// condition of read next block
	if (p_info->read_accum_offset >= p_info->demux_offset) {
		remain = p_info->read_accum_offset - p_info->demux_offset;
	} else {
		remain = p_info->demux_offset - p_info->read_accum_offset;
	}
	reserve = (UINT64)p_user_info->rsv_blknum * p_info->blk_size;
#if 0 // debug
	DBG_DUMP("^C ac=0x%llx, dem=0x%llx, remain = 0x%llx ,reserve = 0x%llx\r\n",
			p_info->read_accum_offset, p_info->demux_offset, remain, reserve);
#endif
	if ((remain < reserve) && (p_info->read_finish == FALSE)) {
		b_read_next = TRUE;
	} else {
		b_read_next = FALSE;
	}

	// check buffer corruption by current decode bs
	if (p_info->next_blk_idx == 0) {
		blk_idx = gFileInUserBufInfo.tt_blknum;
	} else {
		blk_idx  = p_info->next_blk_idx - 1;
	}
	blk_end = p_info->ReadBlk[blk_idx].addr_in_mem + p_info->blk_size;
	frm_start = p_info->vdodec_bs.addr;
	frm_end = frm_start + p_info->vdodec_bs.size;
	//DBG_DUMP("f_s=0x%08x, f_e=0x%08x, b_s=0x%08x, b_e=0x%08x, bi=%d\r\n", frm_start, frm_end, p_info->ReadBlk[blk_idx].addr_in_mem, blk_end, blk_idx);
	if (frm_start <= blk_end && blk_end <= frm_end) {
		DBG_WRN("buffer corruption! f_s(0x%08x) <= b_e(0x%08x) <= f_e(0x%08x)\r\n", frm_start, blk_end, frm_end);
		b_read_next = FALSE;
	}

	// check reading one block buffer
	if (b_read_next == TRUE) {
#if 0
		DBG_DUMP("^G read one block %d\r\n", p_info->next_blk_idx);
#endif
		_FileIn_ReadOneBlkBuf(FILEIN_STATE_NORMAL);
	}

	// move this checking to _BsDemux_DemuxVdoBS() / _BsDemux_DemuxAudBS()
#if 0
	//UINT32 addrInMem, vidEnd, firstblkaddr, copysize;

	// Check boundary of buffer address
	addrInMem = p_info->start_addr + (offset % (p_info->blk_size * p_user_info->tt_blknum));
	vidEnd = addrInMem + size;
	if (vidEnd > p_info->end_addr) {
		if (p_info->next_blk_idx == FILEIN_BUFBLK_0) {
			DBG_DUMP("^G over boundary, read one block %d\r\n", p_info->next_blk_idx);
			_FileIn_ReadOneBlkBuf(FILEIN_STATE_NORMAL);
		}
		firstblkaddr = p_info->ReadBlk[FILEIN_BUFBLK_0].addr_in_mem; // use "p_info->start_addr" to instead ?
		copysize = vidEnd - p_info->end_addr;
		memcpy((char *)p_info->end_addr, (char *)firstblkaddr, copysize);
		//DBG_DUMP("copysize = 0x%x\r\n", copysize);
	}
#endif

	return E_OK;
}

static ER _FileIn_ReloadFileReadBlock(UINT64 offset, UINT32 size)
{
	FILEIN_FILEREAD_INFO *p_info = &gFileInFileReadInfo;
	FILEIN_USERBUF_INFO  *p_user_info = &gFileInUserBufInfo;
	UINT32 addrInMem, vidEnd, firstblkaddr, copysize;
	UINT32 i = 0;

	FILEIN_DUMP("[FILEIN] reload block, off=0x%llx, size=0x%x\r\n", offset, size);

	_FileIn_InitBufInfo();

	// update read file info
	p_info->next_blk_idx = _FileIn_SearchWhichBlock(offset);
	p_info->next_file_pos = offset - (offset % (UINT64)p_info->blk_size);
	p_info->read_accum_offset = p_info->next_file_pos;

	// read one block
	for (i=0; i<gFileInUserBufInfo.pl_blknum; i++) {
		_FileIn_ReadOneBlkBuf(FILEIN_STATE_PRELOAD);
	}

	// Check boundary of buffer address
	addrInMem = p_info->start_addr + (offset % (p_info->blk_size * p_user_info->tt_blknum));
	vidEnd = addrInMem + size;
	if (vidEnd > p_info->end_addr) {
		if (p_info->next_blk_idx == FILEIN_BUFBLK_0) {
			DBG_DUMP("^G over boundary, read one block %d\r\n", p_info->next_blk_idx);
			_FileIn_ReadOneBlkBuf(FILEIN_STATE_PRELOAD);
		}
		firstblkaddr = p_info->ReadBlk[FILEIN_BUFBLK_0].addr_in_mem; // use "p_info->start_addr" to instead ?
		copysize = vidEnd - p_info->end_addr;
		memcpy((char *)p_info->end_addr, (char *)firstblkaddr, copysize);
		DBG_DUMP("^C(reload)copysize = 0x%x\r\n", copysize);
	}

	return E_OK;
}

/**
    @name  NMedia Interface
*/
ER FileIn_Open(UINT32 Id)
{
	ER r;
	if (gFileInOpened == TRUE) {
		return E_SYS;
	}

	FILEIN_DUMP("[FileIn][%d] Open\r\n", Id);

	//_FileIn_install_id();

	_FileIn_AllocMem(Id);

	_FileIn_InitBufInfo();

	_FileIn_TskStart();

	// init variable
#if defined(__LINUX)
	sem_init(&FILEIN_COMMON_SEM_ID, 0, 0);
#else
	r = vos_sem_create(&FILEIN_COMMON_SEM_ID, 1, "SEMID_HD_FILEIN");
	if(r == -1) {
		DBG_ERR("bsdemux vos_sem_create error!\r\n");
		return E_SYS;
	}
#endif

    gFileInOpened = TRUE;

	return E_OK;
}

ER FileIn_Close(UINT32 Id)
{
	if (gFileInOpened == FALSE) {
		return E_SYS;
	}

	FILEIN_DUMP("[FileIn][%d] Close\r\n", Id);

	gFileInOpened = FALSE;
	//_FileIn_uninstall_id();
	_FileIn_FreeMem(Id);

	// reset variable
	{
#if defined(__LINUX)
	sem_destroy(&FILEIN_COMMON_SEM_ID);
#else
	vos_sem_destroy(FILEIN_COMMON_SEM_ID);
#endif
	}

	return E_OK;
}

ER FileIn_SetParam(UINT32 Id, UINT32 Param, UINT32 Value)
{
    ER ret = E_OK;

	switch (Param) {
        case FILEIN_PARAM_FILEHDL:
            gFileInObj.file_handle = (FST_FILE)Value;
			FILEIN_DUMP("[FileIn] file_handle = 0x%x\r\n", gFileInObj.file_handle);
            break;
        case FILEIN_PARAM_FILESIZE:
		{
            gFileInObj.file_ttlsize = (UINT64)Value;
			FILEIN_DUMP("[FileIn] file_ttlsize = 0x%llx\r\n", gFileInObj.file_ttlsize);
            break;
		}
        case FILEIN_PARAM_FILEDUR:
            gFileInObj.file_ttldur = Value;
			FILEIN_DUMP("[FileIn] file_ttldur = %d\r\n", gFileInObj.file_ttldur);
            break;
		case FILEIN_PARAM_FILEFMT:
			gFileInObj.file_fmt = Value;
			FILEIN_DUMP("[FileIn] file_fmt = %d\r\n", gFileInObj.file_fmt);
			break;
		case FILEIN_PARAM_MEM_RANGE:
		{
			FILEIN_MEM_RANGE *pMem = (FILEIN_MEM_RANGE *) Value;
			if (pMem->Addr == 0 || pMem->Size == 0) {
				DBG_ERR("[%d] Invalid mem range, addr = 0x%08x, size = %d\r\n", Id, pMem->Addr, pMem->Size);
				ret = E_PAR;
			}
			else {
				FILEIN_DUMP("[FileIn] MemAddr = 0x%x, Size = 0x%x\r\n", pMem->Addr, pMem->Size);
				gFileInObj.mem.Addr = pMem->Addr;
				gFileInObj.mem.Size = pMem->Size;
			}
			break;
		}
		case FILEIN_PARAM_BLK_TIME:
			gFileInUserBufInfo.blk_time = Value;
			FILEIN_DUMP("[FileIn] blk_time = %d\r\n", gFileInUserBufInfo.blk_time);
			break;
		case FILEIN_PARAM_TT_BLKNUM:
			if (Value > FILEIN_BUFBLK_CNT) {
				DBG_WRN("blk_ttnum(%d) > BUFBLK_CNT(%d), use BUFBLK_CNT instead\r\n", Value, FILEIN_BUFBLK_CNT);
				gFileInUserBufInfo.tt_blknum = FILEIN_BUFBLK_CNT;
			} else {
				gFileInUserBufInfo.tt_blknum = Value;
			}
			FILEIN_DUMP("[FileIn] tt_blknum = %d\r\n", gFileInUserBufInfo.tt_blknum);
			break;
		case FILEIN_PARAM_PL_BLKNUM:
			if (gFileInUserBufInfo.pl_blknum > gFileInUserBufInfo.tt_blknum) {
				DBG_WRN("pl_blknum(%d) > tt_blknum(%d), use tt_blknum instead\r\n", gFileInUserBufInfo.pl_blknum, gFileInUserBufInfo.tt_blknum);
				gFileInUserBufInfo.pl_blknum = gFileInUserBufInfo.tt_blknum;
			} else {
				gFileInUserBufInfo.pl_blknum = Value;
			}
			FILEIN_DUMP("[FileIn] pl_blknum = %d\r\n", gFileInUserBufInfo.pl_blknum);
			break;
		case FILEIN_PARAM_RSV_BLKNUM:
			if (gFileInUserBufInfo.rsv_blknum > gFileInUserBufInfo.tt_blknum) {
				DBG_WRN("rsv_blknum(%d) > tt_blknum(%d), use tt_blknum instead\r\n",
					gFileInUserBufInfo.rsv_blknum,
					gFileInUserBufInfo.tt_blknum);
				gFileInUserBufInfo.rsv_blknum = gFileInUserBufInfo.tt_blknum;
			} else {
				gFileInUserBufInfo.rsv_blknum = Value;
			}
			FILEIN_DUMP("[FileIn] rsv_blknum = %d\r\n", gFileInUserBufInfo.rsv_blknum);
			break;
		case FILEIN_PARAM_CUR_VDOBS:
		{
			MEM_RANGE *p_vdobs = (MEM_RANGE *)Value;
			if (p_vdobs) {
				gFileInFileReadInfo.vdodec_bs.addr = p_vdobs->addr;
				gFileInFileReadInfo.vdodec_bs.size = p_vdobs->size;
			} else {
				DBG_ERR("p_vdobs is NULL\r\n");
				ret = E_PAR;
			}
			break;
		}
		case FILEIN_PARAM_EVENT_CB:
			gfnFileIn_CB = (FileInEventCb *)Value;
			break;
		case FILEIN_PARAM_CONTAINER:
			if (Value) {
				gFileInObj.file_container = (CONTAINERPARSER *)Value;
				FILEIN_DUMP("[NMP_FileIn] file_container = 0x%x\r\n", gFileInObj.file_container);
			}
			break;
		case FILEIN_PARAM_VDO_FR:
			gFileInObj.vdo_fr = Value;
			FILEIN_DUMP("[NMP_FileIn] vdo_fr = %d\r\n", gFileInObj.vdo_fr);
			break;
		case FILEIN_PARAM_AUD_FR:
			gFileInObj.aud_fr = Value;
			FILEIN_DUMP("[NMP_FileIn] aud_fr = %d\r\n", gFileInObj.aud_fr);
			break;
		case FILEIN_PARAM_VDO_TTFRM:
			gFileInObj.vdo_ttfrm = Value;
			FILEIN_DUMP("[NMP_FileIn] vdo_ttfrm = %d\r\n", gFileInObj.vdo_ttfrm);
			break;
		case FILEIN_PARAM_AUD_TTFRM:
			gFileInObj.aud_ttfrm = Value;
			FILEIN_DUMP("[NMP_FileIn] aud_ttfrm = %d\r\n", gFileInObj.aud_ttfrm);
			break;
		case FILEIN_PARAM_USER_BLK_DIRECTLY:
			gFileInUserBufInfo.user_define_blk = Value;
			FILEIN_DUMP("[NMP_FileIn] user_define_blk = %d\r\n", gFileInUserBufInfo.user_define_blk);
			break;
		case FILEIN_PARAM_BLK_SIZE:
			gFileInUserBufInfo.blk_size = Value;
			FILEIN_DUMP("[NMP_FileIn] blk_size = %d\r\n", gFileInUserBufInfo.blk_size);
			break;
		default:
			DBG_ERR("[%d] Set invalid param = %d\r\n", Id, Param);
			ret = E_PAR;
            break;
	}
	return ret;
}

ER FileIn_GetParam(UINT32 Id, UINT32 Param, UINT32 *pValue)
{
	ER ret = E_OK;

	switch (Param) {
		case FILEIN_PARAM_NEED_MEMSIZE:
			*pValue = _FileIn_GetBufSize(Id);
			if (*pValue == 0) {
				DBG_ERR("[FILEIN][%d] get buf size = %d\r\n", Id, *pValue);
				ret = E_SYS;
			} else {
				ret = E_OK;
			}
			break;
		case FILEIN_PARAM_MEM_RANGE:
//			*pValue = (UINT32) &gFileInAllocMemRange[Id];
			*pValue = (UINT32) &gFileInObj.mem;
			break;
		case FILEIN_PARAM_BLK_INFO: {
				FILEIN_BLK_INFO *pinfo = (FILEIN_BLK_INFO *)pValue;
				pinfo->start_addr = gFileInBlkInfo.start_addr;
				pinfo->blk_size= gFileInBlkInfo.blk_size;
				pinfo->tt_blknum= gFileInBlkInfo.tt_blknum;
			}
			break;
		default:
			DBG_ERR("[FILEIN][%d] Get invalid param = %d\r\n", Id, Param);
			ret = E_PAR;
			break;
	}

	return ret;
}

ER FileIn_Action(UINT32 Id, UINT32 Action)
{
	ER ret;
	UINT32 i;

	FILEIN_DUMP("[FileIn][%d] Action = %d\r\n", Id, Action);

	switch (Action) {
		case FILEIN_ACTION_PRELOAD:
			for (i=0; i<gFileInUserBufInfo.pl_blknum; i++) {
				// read one block buffer
				FILEIN_DUMP("preload file block %d\r\n", i);
				if (_FileIn_ReadOneBlkBuf(FILEIN_STATE_PRELOAD) != E_OK) {
					DBG_ERR("read one block error, i=%d\r\n", i);
					return E_SYS;
				}
			}
			ret = E_OK;
			break;
		default:
			DBG_ERR("[%d] Invalid action = %d\r\n", Id, Action);
            ret = E_PAR;
			break;
	}

	return ret;
}

ER FileIn_In(UINT32 Id, UINT32 *pBuf)
{
	FILEIN_CB_INFO *p_cb_info;
	ER ret;

	p_cb_info = (FILEIN_CB_INFO *)pBuf;

	FILEIN_DUMP("[FileIn][%d] pull in, bs_offset=0x%llx, bs_size=0x%x, event=%d\r\n", Id, p_cb_info->bs_offset, p_cb_info->bs_size, p_cb_info->event);

	if (p_cb_info->event == FILEIN_IN_TYPE_CHK_FILEBLK) {
		// check is the target in valid block range
		ret = _FileIn_ChkFileRead(p_cb_info->bs_offset, p_cb_info->bs_size);
		if (ret != E_OK) {
			DBG_ERR("check reading file error\r\n");
			return E_SYS;
		}
	} else if (p_cb_info->event == FILEIN_IN_TYPE_READ_ONEFRM) {

		static UINT8 blk_curr = FILEIN_BUFBLK_0;

		UINT32 addr_in_mem = gFileInFileReadInfo.ReadBlk[blk_curr % gFileInUserBufInfo.tt_blknum].addr_in_mem;
		p_cb_info->uiThisAddr = addr_in_mem;
		blk_curr++;
		ret = _FileIn_ReadFile(p_cb_info->bs_offset, p_cb_info->bs_size, addr_in_mem);
		if (ret != E_OK) {
			DBG_ERR("reading file error\r\n");
			return E_SYS;
		}

	} else if (p_cb_info->event == FILEIN_IN_TYPE_RELOAD_BUF) {
		_FileIn_ReloadFileReadBlock(p_cb_info->bs_offset, p_cb_info->bs_size);
	} else if (p_cb_info->event == FILEIN_IN_TYPE_READ_ONEBLK) {
		if (p_cb_info->uiNextFilePos != 0) {
			gFileInFileReadInfo.next_file_pos = p_cb_info->uiNextFilePos;
		}
		_FileIn_ReadOneBlkBuf(FILEIN_STATE_NORMAL);
	} else if (p_cb_info->event == FILEIN_IN_TYPE_READ_TSBLK) {
		p_cb_info->uiThisAddr = gFileInFileReadInfo.ReadBlk[0].addr_in_mem;
		p_cb_info->uiDemuxAddr = gFileInFileReadInfo.ReadBlk[1].addr_in_mem;
	} else if (p_cb_info->event == FILEIN_IN_TYPE_CONFIG_BUFINFO) {
		ret = _FileIn_InitBufInfo();
		if (ret != E_OK) {
			DBG_ERR("Config Buf Info error\r\n");
			return E_SYS;
		}
	} else if (p_cb_info->event == FILEIN_IN_TYPE_GET_BUFINFO) {
		FILEIN_DUMP("[FileIn] FILEIN_IN_TYPE_GET_BUFINFO\r\n");
	} else {
		DBG_ERR("unknow read cb event=%d\r\n", p_cb_info->event);
		return E_SYS;
	}

	// update file read info
	p_cb_info->read_accum_offset = gFileInFileReadInfo.read_accum_offset;
	p_cb_info->last_used_offset = gFileInFileReadInfo.demux_offset;
	p_cb_info->buf_start_addr = gFileInFileReadInfo.start_addr;
	p_cb_info->buf_end_addr = gFileInFileReadInfo.end_addr;
	p_cb_info->blk_size = gFileInFileReadInfo.blk_size;
	p_cb_info->tt_blknum = gFileInUserBufInfo.tt_blknum;

	return E_OK;
}

