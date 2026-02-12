/*
    Novatek Media Play BitStream Demux Task.

    This file is the task of novatek media player bitstream demux.

    @file       bsdemux_tsk.c
    @ingroup    mlib
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include "avfile/AVFile_ParserTs.h"
#include "avfile/MediaReadLib.h"
#include "nvt_media_interface.h"
#include "bsdemux.h"
#include "bsdemux_id.h"
#include "bsdemux_tsk.h"
#include "bsdemux_ts_tsk.h"
#include "nmediaplay_api.h"
#include "hd_type.h"
#include "hd_common.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          bsdemux
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int bsdemux_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(bsdemux_debug_level, bsdemux_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bsdemux_debug_level, "bsdemux debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif
///////////////////////////////////////////////////////////////////////////////

NMP_BSDEMUX_OBJ                     gNMPBsDemuxObj[NMP_BSDEMUX_MAX_PATH] = {0};
CONTAINERPARSER                    *pNMPBsDemuxContainer[NMP_BSDEMUX_MAX_PATH] = {0};

UINT32                              gNMPBsDemuxPathCount = 0;
UINT8                               gNMPBsDemuxPath[NMP_BSDEMUX_MAX_PATH] = {0};

// TS demux
extern NMP_TSDEMUX_OBJ              gNMPTsDemuxObj;
extern NMP_TSDEMUX_BUFINFO          gNMPTsDemuxBufInfo;
extern UINT32                       gNMPTsDemuxDebugLevel;

// Debug
static UINT32                       gNMPBsDemuxDebugLevel = 0;
#define BSDMX_DUMP(fmtstr, args...) if (gNMPBsDemuxDebugLevel) DBG_DUMP(fmtstr, ##args)
#define NMP_BSDMX_TEST              0
#define NMP_BSDMX_DUMMY_BS          0 // for unit test (please refer to test_bsdemux.c)

static ER _NMP_BsDemux_GetVdoBs(UINT32 path_id, UINT64 *poffset, UINT32 *psize, UINT32 uiFrmIdx)
{
	if (pNMPBsDemuxContainer[path_id]->GetInfo) {
		(pNMPBsDemuxContainer[path_id]->GetInfo)(MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE, &uiFrmIdx, (UINT32 *)poffset, psize);
	}
	DBG_IND("[NMP_BsDmx]get vdo bs, of=0x%llx, s=0x%x\r\n", *poffset, *psize);

	return E_OK;
}

static ER _NMP_BsDemux_GetAudBs(UINT32 path_id, UINT64 *poffset, UINT32 *psize, UINT32 uiFrmIdx)
{
	if (pNMPBsDemuxContainer[path_id]->GetInfo) {
		(pNMPBsDemuxContainer[path_id]->GetInfo)(MEDIAREADLIB_GETINFO_GETAUDIOPOSSIZE, &uiFrmIdx, (UINT32 *)poffset, psize);
	}
	DBG_IND("[NMP_BsDmx]get aud bs, of=0x%llx, s=0x%x\r\n", *poffset, *psize);

	return E_OK;
}

static UINT32 _NMP_BsDemux_HowManyInVQ(UINT32 path_id)
{
	UINT32 front, rear, full, sq = 0;
	NMP_BSDEMUX_IN_VQ *p_vdo_que;

	SEM_WAIT(NMP_BSDEMUX_VDO_SEM_ID[path_id]);
	p_vdo_que = &(gNMPBsDemuxObj[path_id].vdo_que);
	front = p_vdo_que->front;
	rear = p_vdo_que->rear;
	full = p_vdo_que->bfull;
	SEM_SIGNAL(NMP_BSDEMUX_VDO_SEM_ID[path_id]);
	
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = NMP_BSDEMUX_IN_VQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = NMP_BSDEMUX_IN_VQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

static UINT32 _NMP_BsDemux_HowManyInAQ(UINT32 path_id)
{
	UINT32 front, rear, full, sq = 0;
	NMP_BSDEMUX_IN_AQ *p_aud_que;

	SEM_WAIT(NMP_BSDEMUX_AUD_SEM_ID[path_id]);
	p_aud_que = &(gNMPBsDemuxObj[path_id].aud_que);
	front = p_aud_que->front;
	rear = p_aud_que->rear;
	full = p_aud_que->bfull;
	SEM_SIGNAL(NMP_BSDEMUX_AUD_SEM_ID[path_id]);
	
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = NMP_BSDEMUX_IN_AQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = NMP_BSDEMUX_IN_AQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

static ER _NMP_BsDemux_PutVQ(UINT32 path_id, NMP_BSDEMUX_IN_BUF *p_buf)
{
	NMP_BSDEMUX_IN_VQ *p_vdo_que;

	SEM_WAIT(NMP_BSDEMUX_VDO_SEM_ID[path_id]);

    p_vdo_que = &(gNMPBsDemuxObj[path_id].vdo_que);

	if ((p_vdo_que->front == p_vdo_que->rear) && (p_vdo_que->bfull == TRUE)) {
		SEM_SIGNAL(NMP_BSDEMUX_VDO_SEM_ID[path_id]);
		DBG_ERR("[%d] add in queue full!\r\n", path_id);
		return E_SYS;
	} else {
		memcpy(&p_vdo_que->in_buf[p_vdo_que->rear], p_buf, sizeof(NMP_BSDEMUX_IN_BUF));
		p_vdo_que->rear = (p_vdo_que->rear + 1) % NMP_BSDEMUX_IN_VQ_MAX;
		if (p_vdo_que->front == p_vdo_que->rear) { // Check Queue full
			p_vdo_que->bfull = TRUE;
		}
		SEM_SIGNAL(NMP_BSDEMUX_VDO_SEM_ID[path_id]);
		return E_OK;
	}
}

static ER _NMP_BsDemux_GetVQ(UINT32 path_id, NMP_BSDEMUX_IN_BUF *p_buf)
{
	NMP_BSDEMUX_IN_VQ *p_vdo_que;

	SEM_WAIT(NMP_BSDEMUX_VDO_SEM_ID[path_id]);

	p_vdo_que = &(gNMPBsDemuxObj[path_id].vdo_que);

	if ((p_vdo_que->front == p_vdo_que->rear) && (p_vdo_que->bfull == FALSE)) {
		SEM_SIGNAL(NMP_BSDEMUX_VDO_SEM_ID[path_id]);
		DBG_ERR("[%d] get in queue empty!\r\n", path_id);
		return E_SYS;
	} else {
		memcpy(p_buf, &p_vdo_que->in_buf[p_vdo_que->front], sizeof(NMP_BSDEMUX_IN_BUF));
		p_vdo_que->front = (p_vdo_que->front + 1) % NMP_BSDEMUX_IN_VQ_MAX;
		if (p_vdo_que->front == p_vdo_que->rear) { // Check Queue full
			p_vdo_que->bfull = FALSE;
		}
		SEM_SIGNAL(NMP_BSDEMUX_VDO_SEM_ID[path_id]);
		return E_OK;
	}
}

static ER _NMP_BsDemux_PutAQ(UINT32 path_id, NMP_BSDEMUX_IN_BUF *p_buf)
{
	NMP_BSDEMUX_IN_AQ *p_aud_que;

	SEM_WAIT(NMP_BSDEMUX_AUD_SEM_ID[path_id]);

    p_aud_que = &(gNMPBsDemuxObj[path_id].aud_que);

	if ((p_aud_que->front == p_aud_que->rear) && (p_aud_que->bfull == TRUE)) {
		SEM_SIGNAL(NMP_BSDEMUX_AUD_SEM_ID[path_id]);
		DBG_ERR("[%d] add in queue full!\r\n", path_id);
		return E_SYS;
	} else {
		memcpy(&p_aud_que->in_buf[p_aud_que->rear], p_buf, sizeof(NMP_BSDEMUX_IN_BUF));
		p_aud_que->rear = (p_aud_que->rear + 1) % NMP_BSDEMUX_IN_AQ_MAX;
		if (p_aud_que->front == p_aud_que->rear) { // Check Queue full
			p_aud_que->bfull = TRUE;
		}
		SEM_SIGNAL(NMP_BSDEMUX_AUD_SEM_ID[path_id]);
		return E_OK;
	}
}

static ER _NMP_BsDemux_GetAQ(UINT32 path_id, NMP_BSDEMUX_IN_BUF *p_buf)
{
	NMP_BSDEMUX_IN_AQ *p_aud_que;

	SEM_WAIT(NMP_BSDEMUX_AUD_SEM_ID[path_id]);

	p_aud_que = &(gNMPBsDemuxObj[path_id].aud_que);

	if ((p_aud_que->front == p_aud_que->rear) && (p_aud_que->bfull == FALSE)) {
		SEM_SIGNAL(NMP_BSDEMUX_AUD_SEM_ID[path_id]);
		DBG_ERR("[%d] get in queue empty!\r\n", path_id);
		return E_SYS;
	} else {
		memcpy(p_buf, &p_aud_que->in_buf[p_aud_que->front], sizeof(NMP_BSDEMUX_IN_BUF));
		p_aud_que->front = (p_aud_que->front + 1) % NMP_BSDEMUX_IN_AQ_MAX;
		if (p_aud_que->front == p_aud_que->rear) { // Check Queue full
			p_aud_que->bfull = FALSE;
		}
		SEM_SIGNAL(NMP_BSDEMUX_AUD_SEM_ID[path_id]);
		return E_OK;
	}
}

static ER _NMP_BsDemux_ChkSVC(NMP_BSDEMUX_OBJ *p_obj, UINT32 bs_addr, BOOL* p_is_skippable)
{
	if (p_obj == NULL || p_is_skippable == NULL) {
		DBG_ERR("p_obj or p_svc is NULL\n");
		return E_SYS;
	}

	if (bs_addr == 0) {
		DBG_ERR("invalid bs address(0x%lx)\n", bs_addr);
		return E_SYS;
	}

	UINT8* ptr = (UINT8*) bs_addr;
	*p_is_skippable = FALSE;
	ptr += 4; /* skip start code */

	if (p_obj->vdo_codec == NMEDIAPLAY_DEC_H264) {

		/* SVC 1x P frame */
		if(*ptr == 0x01){
			*p_is_skippable = TRUE;
		}
	}
	else if(NMEDIAPLAY_DEC_H265){

		/* SVC 1x P frame */
		if(*ptr == 0x02 && *(ptr + 1) == 0x03){
			*p_is_skippable = TRUE;
		}
	}
	else{
		DBG_ERR("SVC only support H26x!\n");
		return E_SYS;
	}

	return E_OK;
}

static ER _NMP_BsDemux_ChkH265IfrmBS(UINT32 desc_size, UINT32 bs_addr)
{
	UINT8 *ptr8 = NULL;

	// error check
	if (desc_size == 0) {
		DBG_ERR("h265 desc size = 0\r\n");
		return E_PAR;
	}
	if (bs_addr == 0) {
		DBG_ERR("bs_addr = 0\r\n");
		return E_PAR;
	}
	ptr8 = (UINT8 *)bs_addr;

	// check IDR type
	ptr8 = ptr8 + desc_size;
	if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) {

		/* IVOT_N00028-343 support multi-tile, skip update start code here */
//		*ptr8 = 0x00;
//		*(ptr8+1) = 0x00;
//		*(ptr8+2) = 0x00;
//		*(ptr8+3) = 0x01;
	} else {
		DBG_ERR("Get IDR type(0x%08x) error! desc_size=%d\r\n", *(ptr8 + 4), desc_size);
		return E_SYS;
	}

	return E_OK;
}

/* IVOT_N00028-343 support multi-tile */
static ER _NMP_BsDemux_UnwrapBS(NMP_BSDEMUX_OBJ *p_obj, UINT32 vdo_idx, UINT32 *p_addr, UINT32 bs_size)
{
	UINT8 *ptr8 = NULL;
	UINT32 tile_size = 0;
	UINT32 offset = 0;
	UINT32 bs_size_count = 0;
	UINT8 tile_cnt = 0;

	// error check
	if (p_obj == NULL) {
		DBG_ERR("p_obj is NULL\r\n");
		return E_SYS;
	}

	if (*p_addr == 0) {
		DBG_ERR("invalid addr_in_mem(0x%lx)\r\n", *p_addr);
		return E_SYS;
	}

	// unwrap bs
	if (p_obj->vdo_codec == NMEDIAPLAY_DEC_H264) {
		// update start code
		ptr8 = (UINT8 *) *p_addr;
		*ptr8 = 0x00;
		*(ptr8+1) = 0x00;
		*(ptr8+2) = 0x00;
		*(ptr8+3) = 0x01;
	} else if (p_obj->vdo_codec == NMEDIAPLAY_DEC_H265) {
		if (p_obj->vdo_gop == 0) {
			DBG_ERR("h265 demux gop = 0\r\n");
			return E_SYS;
		}

		// skip descriptor
		if (vdo_idx % p_obj->vdo_gop == 0) {
			if (_NMP_BsDemux_ChkH265IfrmBS(p_obj->desc_size, *p_addr) != E_OK) {
				DBG_ERR("check I-frame error, i=%d\r\n", vdo_idx);

				#if NMP_BSDMX_TEST // debug
					DBG_DUMP("ERR!!!!  vdo_bsdmx a=0x%lx\r\n", *p_addr);
					DBG_DUMP("bs = ");
					UINT8 *ptr = (UINT8 *)p_addr;
					DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
						*(ptr), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9), *(ptr+10));
				#endif

				return E_SYS;
			}
			if (p_obj->desc_size == 0) {
				DBG_WRN("h265 desc size = 0\r\n");
			}
			*p_addr += p_obj->desc_size;
			bs_size -= p_obj->desc_size;
		}

		/* update start code, IVOT_N00028-343 support multi-tile */
		ptr8 = (UINT8 *) *p_addr;
		do {

			tile_size = (((*ptr8) << 24) | ((*(ptr8 + 1))<<16) | ((*(ptr8 + 2))<< 8) | (*(ptr8 + 3)));

			offset = (4 + tile_size);
			bs_size_count += offset;

			*ptr8 = 0x00;
			*(ptr8+1) = 0x00;
			*(ptr8+2) = 0x00;
			*(ptr8+3) = 0x01;

			ptr8 += offset;
			tile_cnt++;

		} while(bs_size_count < bs_size);

		DBG_IND("tile_cnt=%u\r\n", tile_cnt);

	} else {
		DBG_ERR("unknown codec(%lu)\r\n", p_obj->vdo_codec);
		return E_PAR;
	}

	return E_OK;
}

static ER _NMP_BsDemux_CopyOutOfBoundsData(UINT64 bs_offset, UINT32 bs_size, NMP_BSDEMUX_FILE_INFO *p_file)
{
	UINT32 addrInMem = 0, vid_end = 0;
	UINT32 first_blk_addr = 0, copy_size = 0;
	UINT32 buf_end_addr = 0;

	// error check
	if (p_file == NULL) {
		DBG_ERR("p_file is NULL\r\n");
		return E_SYS;
	}
    if (!p_file->tt_blknum || !p_file->blk_size) {
		DBG_ERR("Invalid param tt_blknum(%lu), blk_size(%llu)\r\n", p_file->tt_blknum, p_file->blk_size);
		return E_SYS;
    }

	// Check boundary of buffer address
	addrInMem = p_file->buf_start_addr + (bs_offset % (p_file->blk_size * p_file->tt_blknum));
#if 1 // tmp
	buf_end_addr = p_file->buf_start_addr + p_file->blk_size * p_file->tt_blknum;
#endif
	vid_end = addrInMem + bs_size;
	if (vid_end > buf_end_addr) {
		first_blk_addr = p_file->buf_start_addr;
		copy_size = vid_end - buf_end_addr;
		memcpy((char *)buf_end_addr, (char *)first_blk_addr, copy_size);
		DBG_IND("copy_size = 0x%x\r\n", copy_size);
	}

	return E_OK;
}

static ER _NMP_BsDemux_TsDemuxAudBS(UINT32 path_id)
{
	UINT64 bs_offset = 0;
	UINT32 bs_size = 0;
	UINT32 addr_in_mem = 0;
	NMP_BSDEMUX_OBJ *p_obj = &gNMPBsDemuxObj[path_id];
	NMP_BSDEMUX_FILE_INFO *p_file = &gNMPBsDemuxObj[path_id].file_info;

	UINT64 bs_offset2 = 0;
	UINT32 bs_size2 = 0;
	NMP_BSDEMUX_IN_BUF in_buf = {0};
	HD_BSDEMUX_CBINFO cb_info = {0};

	// get in_buf from queue
	if (_NMP_BsDemux_GetAQ(path_id, &in_buf) != E_OK) {
		DBG_ERR("[%d] get in_buf fail\r\n", path_id);
	}

    ER eResult = E_OK;
	NMP_TSDEMUX_BUFINFO *p_ts_buf = &gNMPTsDemuxBufInfo;

	_NMP_BsDemux_GetAudBs(path_id, &bs_offset, &bs_size, in_buf.index);
	if (bs_offset == 0) {
		DBG_ERR("get vdo bs error, i=%d\r\n", in_buf.index);
		return E_SYS;
	}

	if (p_obj->file_fmt == MEDIA_FILEFORMAT_TS) {
		// get vdo bs
		_NMP_BsDemux_GetAudBs(path_id, &bs_offset2, &bs_size2, in_buf.index + 1);
	}

	// offset -> addrInMem
	if (p_obj->file_fmt == MEDIA_FILEFORMAT_TS) {
		p_ts_buf->use_accum_size += bs_size;

		// check ts demux finish
		if (TsReadLib_DemuxFinished()) {

            UINT64 diff = (p_ts_buf->demux_accum_size - p_ts_buf->use_accum_size);
			UINT64 file_diff = p_ts_buf->file_remain_size - p_ts_buf->read_accum_size;
			const UINT64 blk_size = p_obj->file_info.blk_size;

			if ((diff < blk_size || file_diff < blk_size) && !p_ts_buf->b_read_finished) {
				if ((p_ts_buf->file_remain_size - p_ts_buf->read_accum_size) < blk_size) {
					p_ts_buf->b_read_finished = TRUE;
				}

				if (p_obj->callback)
				{
					(p_obj->callback)(HD_BSDEMUX_CB_EVENT_READ_AUDIO_BS, NULL);
				}
			}
		}

		addr_in_mem = bs_offset;
	}

	eResult = _NMP_BsDemux_CopyOutOfBoundsData(bs_offset, bs_size, p_file);
    if(eResult != E_OK) {  // File read again and retry.

		if (p_obj->callback)
		{
			(p_obj->callback)(HD_BSDEMUX_CB_EVENT_READ_AUDIO_BS, NULL);
		}

        _NMP_BsDemux_CopyOutOfBoundsData(bs_offset, bs_size, p_file);
    }

	// prepare cb_info
	cb_info.addr = addr_in_mem;
	cb_info.offset = bs_offset;
	cb_info.size = bs_size;
	cb_info.index = in_buf.index;
	cb_info.eof = (bs_offset2 == 0) ? TRUE : FALSE;

	// debug
#if NMP_BSDMX_TEST
	DBG_DUMP("vdo_bsdmx i=%d, a=0x%x, o=0x%llx, s=%d\r\n", cb_info.index, cb_info.addr, cb_info.offset, cb_info.size);
	DBG_DUMP("bs = ");
	UINT8 *ptr = (UINT8 *)cb_info.addr;
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9), *(ptr+10));
	DBG_DUMP("\r\n...\r\n");
	ptr = (UINT8 *)(cb_info.addr + cb_info.size);
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr-10), *(ptr-9), *(ptr-8), *(ptr-7), *(ptr-6), *(ptr-5), *(ptr-4), *(ptr-3), *(ptr-2));
	DBG_DUMP("\r\n\r\n");
#endif

	// callback to app
	if (p_obj->callback) {
		(p_obj->callback)(HD_BSDEMUX_CB_EVENT_AUDIO_BS, (HD_BSDEMUX_CBINFO *)&cb_info);
	}

	return E_OK;
}

static ER _NMP_BsDemux_TsDemuxVdoBS(UINT32 path_id)
{
	UINT64 bs_offset = 0;
	UINT32 bs_size = 0;
	UINT32 addr_in_mem = 0;
	NMP_BSDEMUX_OBJ *p_obj = &gNMPBsDemuxObj[path_id];
	NMP_BSDEMUX_FILE_INFO *p_file = &gNMPBsDemuxObj[path_id].file_info;

	UINT64 bs_offset2 = 0;
	UINT32 bs_size2 = 0;
	NMP_BSDEMUX_IN_BUF in_buf = {0};
	HD_BSDEMUX_CBINFO cb_info = {0};

	// get in_buf from queue
	if (_NMP_BsDemux_GetVQ(path_id, &in_buf) != E_OK) {
		DBG_ERR("[%d] get in_buf fail\r\n", path_id);
		return E_SYS;
	}

    ER eResult = E_OK;
	NMP_TSDEMUX_BUFINFO *p_ts_buf = &gNMPTsDemuxBufInfo;

	_NMP_BsDemux_GetVdoBs(path_id, &bs_offset, &bs_size, in_buf.index);
	if (bs_offset == 0) {
		DBG_ERR("get vdo bs error, i=%d\r\n", in_buf.index);
		return E_SYS;
	}

	if (p_obj->file_fmt == MEDIA_FILEFORMAT_TS) {
		// get vdo bs
		_NMP_BsDemux_GetVdoBs(path_id, &bs_offset2, &bs_size2, in_buf.index + 1);
	}

	// offset -> addrInMem
	if (p_obj->file_fmt == MEDIA_FILEFORMAT_TS) {
		p_ts_buf->use_accum_size += bs_size;

		// check ts demux finish
		if (TsReadLib_DemuxFinished()) {
#if 1
            UINT64 diff = (p_ts_buf->demux_accum_size - p_ts_buf->use_accum_size);
			UINT64 file_diff = p_ts_buf->file_remain_size - p_ts_buf->read_accum_size;
			const UINT64 blk_size = p_obj->file_info.blk_size;

#endif
			if ((diff < blk_size || file_diff < blk_size) && !p_ts_buf->b_read_finished) {
				if ((p_ts_buf->file_remain_size - p_ts_buf->read_accum_size) < blk_size) {
					p_ts_buf->b_read_finished = TRUE;
				}

				if (p_obj->callback)
				{
					(p_obj->callback)(HD_BSDEMUX_CB_EVENT_READ_VIDEO_BS, NULL);
				}
			}
		}

		addr_in_mem = bs_offset;
	}

	eResult = _NMP_BsDemux_CopyOutOfBoundsData(bs_offset, bs_size, p_file);
    if(eResult != E_OK) {  // File read again and retry.

		if (p_obj->callback)
		{
			(p_obj->callback)(HD_BSDEMUX_CB_EVENT_READ_VIDEO_BS, NULL);
		}

        _NMP_BsDemux_CopyOutOfBoundsData(bs_offset, bs_size, p_file);
    }

	// prepare cb_info
	cb_info.addr = addr_in_mem;
	cb_info.offset = bs_offset;
	cb_info.size = bs_size;
	cb_info.index = in_buf.index;

	// debug
#if NMP_BSDMX_TEST
	DBG_DUMP("vdo_bsdmx i=%d, a=0x%x, o=0x%llx, s=%d\r\n", cb_info.index, cb_info.addr, cb_info.offset, cb_info.size);
	DBG_DUMP("bs = ");
	UINT8 *ptr = (UINT8 *)cb_info.addr;
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9), *(ptr+10));
	DBG_DUMP("\r\n...\r\n");
	ptr = (UINT8 *)(cb_info.addr + cb_info.size);
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr-10), *(ptr-9), *(ptr-8), *(ptr-7), *(ptr-6), *(ptr-5), *(ptr-4), *(ptr-3), *(ptr-2));
	DBG_DUMP("\r\n\r\n");
#endif

	// callback to app
	if (p_obj->callback) {
		(p_obj->callback)(HD_BSDEMUX_CB_EVENT_VIDEO_BS, (HD_BSDEMUX_CBINFO *)&cb_info);
	}

	return E_OK;
}

static ER _NMP_BsDemux_DemuxVdoBS(UINT32 path_id)
{
	ER r = E_OK;
	UINT64 bs_offset = 0;
	UINT32 bs_size = 0, addr_in_mem = 0;
	NMP_BSDEMUX_OBJ *p_obj = &gNMPBsDemuxObj[path_id];
	NMP_BSDEMUX_FILE_INFO *p_file = &gNMPBsDemuxObj[path_id].file_info;
	NMP_BSDEMUX_IN_BUF in_buf = {0};
	HD_BSDEMUX_CBINFO cb_info = {0};

	// get in_buf from queue
	if (_NMP_BsDemux_GetVQ(path_id, &in_buf) != E_OK) {
		DBG_ERR("[%d] get in_buf fail\r\n", path_id);
		r = E_SYS;
		goto demux_vdo_end;
	}

	// get vdo bs offset & size
	if (in_buf.bs_type == NMP_BSDEMUX_BS_TPYE_VIDEO) {
		_NMP_BsDemux_GetVdoBs(path_id, &bs_offset, &bs_size, in_buf.index);
	}

#if NMP_BSDMX_DUMMY_BS
	if (!bs_offset) { bs_offset = 0x1234; }
	if (!bs_size) { bs_size = 0x1000; }
#endif

	if (!bs_size) {
		DBG_ERR("[%d] invalid bs: av_type(%d), size(0x%lx)\r\n", path_id, in_buf.bs_type, bs_size);
		r = E_PAR;
		goto demux_vdo_end;
	}

	// check data out of bounds
	r = _NMP_BsDemux_CopyOutOfBoundsData(bs_offset, bs_size, p_file);
	if (r != E_OK) {
		DBG_ERR("[%d] CopyOutOfBoundsData fail\r\n", path_id);
		goto demux_vdo_end;
	}

	// offset -> addr_in_mem
	addr_in_mem = p_file->buf_start_addr + (bs_offset % (p_file->blk_size * p_file->tt_blknum));

	/* IVOT_N00028-343 support multi-tile */
	r = _NMP_BsDemux_UnwrapBS(p_obj, in_buf.index, &addr_in_mem, bs_size);
	if (r != E_OK) {
		goto demux_vdo_end;
	}

	// prepare cb_info
	cb_info.addr = addr_in_mem;
	cb_info.offset = bs_offset;
	cb_info.size = bs_size;
	cb_info.index = in_buf.index;

	// debug
#if NMP_BSDMX_TEST
	DBG_DUMP("vdo_bsdmx i=%d, a=0x%x, o=0x%llx, s=%d\r\n", cb_info.index, cb_info.addr, cb_info.offset, cb_info.size);
	DBG_DUMP("bs = ");
	UINT8 *ptr = (UINT8 *)cb_info.addr;
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9), *(ptr+10));
	DBG_DUMP("\r\n...\r\n");
	ptr = (UINT8 *)(cb_info.addr + cb_info.size);
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr-10), *(ptr-9), *(ptr-8), *(ptr-7), *(ptr-6), *(ptr-5), *(ptr-4), *(ptr-3), *(ptr-2));
	DBG_DUMP("\r\n\r\n");
#endif


	BOOL is_skippable = FALSE;

	if(_NMP_BsDemux_ChkSVC(p_obj, cb_info.addr, &is_skippable) == E_OK){
		cb_info.skippable = is_skippable;
	}
	else{
		cb_info.skippable = FALSE;
	}

	if (p_obj->callback) {
		(p_obj->callback)(HD_BSDEMUX_CB_EVENT_VIDEO_BS, (HD_BSDEMUX_CBINFO *)&cb_info);
	}

demux_vdo_end:
	return r;
}

static ER _NMP_BsDemux_DemuxAudBS(UINT32 path_id)
{
	ER r = E_OK;
	UINT64 bs_offset = 0;
	UINT32 bs_size = 0, addr_in_mem = 0;
	NMP_BSDEMUX_OBJ *p_obj = &gNMPBsDemuxObj[path_id];
	NMP_BSDEMUX_FILE_INFO *p_file = &gNMPBsDemuxObj[path_id].file_info;
	NMP_BSDEMUX_IN_BUF in_buf = {0};
	HD_BSDEMUX_CBINFO cb_info = {0};
	
	// get in_buf from queue
	if (_NMP_BsDemux_GetAQ(path_id, &in_buf) != E_OK) {
		DBG_ERR("[%d] get in_buf fail\r\n", path_id);
		r = E_SYS;
		goto demux_aud_end;
	}

	// get vdo bs offset & size
	if (in_buf.bs_type == NMP_BSDEMUX_BS_TPYE_AUDIO) {
		_NMP_BsDemux_GetAudBs(path_id, &bs_offset, &bs_size, in_buf.index);
	}

#if NMP_BSDMX_DUMMY_BS
	if (!bs_offset) { bs_offset = 0x5678; }
	if (!bs_size) { bs_size = 0x1000; }
#endif

	if (!bs_size) {
		DBG_ERR("[%d] invalid bs: av_type(%d), size(0x%lx)\r\n", path_id, in_buf.bs_type, bs_size);
		r = E_PAR;
		goto demux_aud_end;
	}

	// check data out of bounds
	r = _NMP_BsDemux_CopyOutOfBoundsData(bs_offset, bs_size, p_file);
	if (r != E_OK) {
		DBG_ERR("[%d] CopyOutOfBoundsData fail\r\n", path_id);
		goto demux_aud_end;
	}

	// offset -> addr_in_mem
	addr_in_mem = p_file->buf_start_addr + (bs_offset % (p_file->blk_size * p_file->tt_blknum));

	// prepare cb_info
	cb_info.addr = addr_in_mem;
	cb_info.offset = bs_offset;
	cb_info.size = bs_size;
	cb_info.index = in_buf.index;

	// debug
#if NMP_BSDMX_TEST
	DBG_DUMP("aud_bsdmx i=%d, a=0x%x, o=0x%llx, s=%d\r\n", cb_info.index, cb_info.addr, cb_info.offset, cb_info.size);
	DBG_DUMP("bs = ");
	UINT8 *ptr = (UINT8 *)cb_info.addr;
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9), *(ptr+10));
	DBG_DUMP("\r\n...\r\n");
	ptr = (UINT8 *)(cb_info.addr + cb_info.size);
	DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
		*(ptr-10), *(ptr-9), *(ptr-8), *(ptr-7), *(ptr-6), *(ptr-5), *(ptr-4), *(ptr-3), *(ptr-2));
	DBG_DUMP("\r\n\r\n");
#endif

	// callback to app
	if (p_obj->callback) {
		(p_obj->callback)(HD_BSDEMUX_CB_EVENT_AUDIO_BS, (HD_BSDEMUX_CBINFO *)&cb_info);
	}

demux_aud_end:
	return r;
}

THREAD_DECLARE(_NMP_BsDemux_Tsk, arglist) //void _NMP_BsDemux_Demux_Tsk(void)
{
	FLGPTN			flag = 0, wait_flag = 0;
	UINT32          i = 0;

	BSDMX_DUMP("_bsdemux_tsk() start\r\n");

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_IDLE);
	
	wait_flag = FLG_NMP_BSDEMUX_START |
				FLG_NMP_BSDEMUX_VDO_DEMUX |
				FLG_NMP_BSDEMUX_AUD_DEMUX |
				FLG_NMP_BSDEMUX_STOP;

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_IDLE);
		
		PROFILE_TASK_IDLE();
		wai_flg(&flag, NMP_BSDEMUX_FLG_ID, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		
		clr_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_IDLE);

		if (flag & FLG_NMP_BSDEMUX_STOP) {
			break;
		}

		if (flag & FLG_NMP_BSDEMUX_VDO_DEMUX) {
			for (i = 0; i < NMP_BSDEMUX_MAX_PATH; i++) { // i as pathID
				if (gNMPBsDemuxPath[i] == 1) {

					if (gNMPBsDemuxObj[i].file_fmt == MEDIA_FILEFORMAT_TS) {
						_NMP_BsDemux_TsDemuxVdoBS(i);
					}
					else{
						_NMP_BsDemux_DemuxVdoBS(i);
					}
				}
			}

			for (i = 0; i < NMP_BSDEMUX_MAX_PATH; i++) { // i as pathID
				if (gNMPBsDemuxPath[i] == 1) {
					if (_NMP_BsDemux_HowManyInVQ(i) > 0) { // continue to demux
						BSDMX_DUMP("trig vdo_dmx again\r\n");
						set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_VDO_DEMUX);
						break;
					}
				}
			}
		}

		if (flag & FLG_NMP_BSDEMUX_AUD_DEMUX) {
			for (i = 0; i < NMP_BSDEMUX_MAX_PATH; i++) { // i as pathID
				if (gNMPBsDemuxPath[i] == 1) {

					if (gNMPBsDemuxObj[i].file_fmt == MEDIA_FILEFORMAT_TS) {
						_NMP_BsDemux_TsDemuxAudBS(i);
					}
					else{
						_NMP_BsDemux_DemuxAudBS(i);
					}
				}
			}

			for (i = 0; i < NMP_BSDEMUX_MAX_PATH; i++) { // i as pathID
				if (gNMPBsDemuxPath[i] == 1) {
					if (_NMP_BsDemux_HowManyInAQ(i) > 0) { // continue to demux
						BSDMX_DUMP("trig aud_dmx again\r\n");
						set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_AUD_DEMUX);
						break;
					}
				}
			}
		}
	} // end of while loop

	set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_STOP_DONE);
	
	THREAD_RETURN(0);
}

static ER _NMP_BsDemux_TskStop(UINT32 pathID)
{
	FLGPTN flag;

	BSDMX_DUMP("[BSDMX]tskstop wait idle ..\r\n");
	wai_flg(&flag, NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_IDLE, TWF_ORW);
	BSDMX_DUMP("[BSDMX]tskstop wait idle OK\r\n");

	set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_STOP);
	wai_flg(&flag, NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_STOP_DONE, TWF_ORW | TWF_CLR);

	return E_OK;
}

static ER _NMP_BsDemux_TskStart(void)
{
	// start tsk
	THREAD_CREATE(NMP_BSDEMUX_TSK_ID, _NMP_BsDemux_Tsk, NULL, "_NMP_BsDemux_Tsk");
	if (NMP_BSDEMUX_TSK_ID == 0) {
		DBG_ERR("Invalid NMP_BSDEMUX_TSK_ID\r\n");
		return E_OACV;
	}
	THREAD_RESUME(NMP_BSDEMUX_TSK_ID);
	BSDMX_DUMP("[bsdemux]bsdemux task start...\r\n");

	return E_OK;
}

static ER _NMP_BsDemux_AllocMem(UINT32 id)
{
	void                 *va;
	UINT32               pa, size;
	HD_RESULT            ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	NMP_BSDEMUX_OBJ      *p_obj = &gNMPBsDemuxObj[id];

	// get size
	size = _NMP_TsDemux_GetBufSize(id);

	// allocate mem
	ret = hd_common_mem_alloc("BsDemux", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("[%d] alloc size(0x%x), ddr(%d)\r\n", id, (unsigned int)(size), (int)ddr_id);
		return E_SYS;
	}
	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));

	// keep in global
	p_obj->alloc_mem.pa = pa;
	p_obj->alloc_mem.va = (UINT32)va;
	p_obj->alloc_mem.size = size;

	// config buf to tsdemx
	_NMP_TSDemux_ConfigBufInfo((UINT32)va, size);

	return E_OK;
}

static ER _NMP_BsDemux_FreeMem(UINT32 id)
{
	HD_RESULT       ret;
	NMP_BSDEMUX_OBJ *p_obj = &gNMPBsDemuxObj[id];

	// reset tsdemux buf
	_NMP_TSDemux_ConfigBufInfo(0, 0);

	if (p_obj->alloc_mem.pa && p_obj->alloc_mem.va) {
		ret = hd_common_mem_free(p_obj->alloc_mem.pa, (void *)p_obj->alloc_mem.va);
		if (ret != HD_OK) {
			DBG_ERR("[%d] free pa(0x%x), va(%d)\r\n", id, p_obj->alloc_mem.pa, p_obj->alloc_mem.va);
			return E_SYS;
		}
	}

	// reset global
	p_obj->alloc_mem.pa = 0;
	p_obj->alloc_mem.va = 0;
	p_obj->alloc_mem.size = 0;

	return E_OK;
}

static ER NMI_BsDemux_Open(UINT32 id, UINT32 *pContext)
{
	ER r = E_OK;
	
	BSDMX_DUMP("[bsdemux][%d] Open\r\n", id);

	// init param
	//_NMP_BsDemux_InitParam(id);

	// check container register
	if (pNMPBsDemuxContainer[id] == NULL) {
		DBG_ERR("[%d] Container is not registered!\r\n", id);
		return E_PAR;
	}

	// install id
	NMP_BsDemux_InstallID();

	// start task
	gNMPBsDemuxPath[id] = 1;
	gNMPBsDemuxPathCount++;
	if (gNMPBsDemuxPathCount == 1) {
		// for ts demux
		if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
			_NMP_TsDemux_Open();
		}
		
		// for bs demux
		r = _NMP_BsDemux_TskStart();
		if (r != E_OK) {
			DBG_ERR("_NMP_BsDemux_TskStart fail(%d)\r\n", r);
			return r;
		}
	}

	// alloc mem
	if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
		r = _NMP_BsDemux_AllocMem(id);
		if (r != E_OK) {
			DBG_ERR("_NMP_BsDemux_AllocMem fail(%d)\r\n", r);
			return r;
		}
	}

	return E_OK;
}

static ER NMI_BsDemux_Close(UINT32 id)
{
	ER r = E_OK;

	BSDMX_DUMP("[bsdemux][%d] Close\r\n", id);

	gNMPBsDemuxPath[id] = 0;
	gNMPBsDemuxPathCount--;
	if (gNMPBsDemuxPathCount == 0) {
		// for ts demux
		if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
			r = _NMP_TsDemux_Close();
			if (r != E_OK) {
				DBG_ERR("_NMP_TsDemux_Close fail(%d)\r\n", r);
				return r;
			}
		}

		// for bs demux
		_NMP_BsDemux_TskStop(id);
	}

	// free mem
	r = _NMP_BsDemux_FreeMem(id);
	if (r != E_OK) {
		DBG_ERR("_NMP_BsDemux_FreeMem fail(%d)\r\n", r);
		return r;
	}

	// uninstall id
	NMP_BsDemux_UninstallID();

	// reset
	memset(&gNMPBsDemuxObj[id], 0, sizeof(NMP_BSDEMUX_OBJ));

	return E_OK;
}

static ER NMI_BsDemux_SetParam(UINT32 id, UINT32 Param, UINT32 Value)
{
    ER ret = E_OK;

	switch (Param) {
		case NMP_BSDEMUX_PARAM_MAX_MEM_INFO:
			{
				NMP_BSDEMUX_MAX_MEM_INFO *p_max_mem = (NMP_BSDEMUX_MAX_MEM_INFO *) Value;

				if (p_max_mem == NULL) {
					DBG_ERR("p_max_mem is NULL\r\n");
					ret = E_PAR;
					break;
				}

				gNMPBsDemuxObj[id].file_fmt = p_max_mem->file_fmt;

				BSDMX_DUMP("[bsdemux][%d] set max mem info: file_fmt(%lu)\r\n", id, gNMPBsDemuxObj[id].file_fmt);
			}
			break;
		case NMP_BSDEMUX_PARAM_VDO_ENABLE:
            gNMPBsDemuxObj[id].vdo_en = (BOOL)Value;
			BSDMX_DUMP("[NMP_BsDmx]vdo_en = %d\r\n", gNMPBsDemuxObj[id].vdo_en);
            break;
		case NMP_BSDEMUX_PARAM_VDO_CODEC:
			gNMPBsDemuxObj[id].vdo_codec = Value;
			BSDMX_DUMP("[NMP_BsDmx]vdo_codec = %d\r\n", gNMPBsDemuxObj[id].vdo_codec);
			break;
		case NMP_BSDEMUX_PARAM_VDO_WID:
			gNMPBsDemuxObj[id].vdo_wid = Value;
			break;
		case NMP_BSDEMUX_PARAM_VDO_HEI:
			gNMPBsDemuxObj[id].vdo_hei = Value;
			break;
		case NMP_BSDEMUX_PARAM_VDO_DECRYPT: 
			{
				UINT8 *key = (UINT8 *)Value;
				TsReadLib_SetDecryptKey(key);
			}
			break;
		case NMP_BSDEMUX_PARAM_VDO_GOP:
			if (Value) {
				gNMPBsDemuxObj[id].vdo_gop = Value;
			} else {
				DBG_ERR("invalid vdo_gop(%lu)\r\n", Value);
				ret = E_PAR;
			}
			break;
		case NMP_BSDEMUX_PARAM_DESC_SIZE:
			if (Value) {
				gNMPBsDemuxObj[id].desc_size = Value;
			} else {
				DBG_ERR("invalid desc_size(%lu)\r\n", Value);
				ret = E_PAR;
			}
			break;
		case NMP_BSDEMUX_PARAM_AUD_ENABLE:
            gNMPBsDemuxObj[id].aud_en = (BOOL)Value;
			BSDMX_DUMP("[NMP_BsDmx][%d] bAudEn = %d\r\n", id, gNMPBsDemuxObj[id].aud_en);
            break;
		case NMP_BSDEMUX_PARAM_CONTAINER:
			pNMPBsDemuxContainer[id] = (CONTAINERPARSER *)Value;
			BSDMX_DUMP("[NMP_BsDmx][%d] container = 0x%x\r\n", id, pNMPBsDemuxContainer[id]);
			break;
		case NMP_BSDEMUX_PARAM_MEM_RANGE: 
			{
				MEM_RANGE *p_mem = (MEM_RANGE *)Value;
				if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
					_NMP_TSDemux_ConfigBufInfo(p_mem->addr, p_mem->size);
				}
			}
			break;
		case NMP_BSDEMUX_PARAM_BUF_STARTADDR:
			gNMPBsDemuxObj[id].file_info.buf_start_addr = Value;
			break;
		case NMP_BSDEMUX_PARAM_BLK_TTNUM:
			gNMPBsDemuxObj[id].file_info.tt_blknum = Value;
			break;
		case NMP_BSDEMUX_PARAM_BLK_SIZE:
			{
				UINT64 *pBlkSize = (UINT64 *)Value;;
				gNMPBsDemuxObj[id].file_info.blk_size = *pBlkSize;
			}
			break;
        case NMP_BSDEMUX_PARAM_REG_CB:
            BSDMX_DUMP("[NMP_BsDmx]Register CB\r\n");
            gNMPBsDemuxObj[id].callback = (HD_BSDEMUX_CALLBACK) Value;
            break;

		case NMP_BSDEMUX_PARAM_TRIG_TSDMX: {
			MEM_RANGE *p_mem = (MEM_RANGE *)Value;
			if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
				_NMP_TSDemux_SetInfo(NMP_TSDEMUX_SETINFO_SRC_POS_SIZE, p_mem->addr, p_mem->size);
			}
			break;
		}
		case NMP_BSDEMUX_PARAM_FILESIZE:
			if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
				UINT64 *p_file_size = (UINT64 *)Value;
				gNMPTsDemuxBufInfo.file_ttlsize = *p_file_size;
				BSDMX_DUMP("[NMP_TsDmx] file_ttlsize = 0x%llx\r\n", gNMPTsDemuxBufInfo.file_ttlsize);
			}
			break;
		case NMP_BSDEMUX_PARAM_TSDMX_BUFBLK:
			if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
				UINT32 *bufblk_num = (UINT32 *)Value;
				gNMPTsDemuxObj.bufblk_num = *bufblk_num;
				BSDMX_DUMP("[NMP_TsDmx] bufblk_num = %d\r\n", gNMPTsDemuxObj.bufblk_num);
			}
			break;
		case NMP_BSDEMUX_PARAM_TSDMX_BUF_RESET:
			if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
				gNMPTsDemuxBufInfo.read_accum_size = 0;
				gNMPTsDemuxBufInfo.demux_accum_size = 0;
				gNMPTsDemuxBufInfo.use_accum_size = 0;
				gNMPTsDemuxBufInfo.file_remain_size = gNMPTsDemuxBufInfo.file_ttlsize;
				gNMPTsDemuxBufInfo.b_read_finished = FALSE;
			}
			break;
		case NMP_BSDEMUX_PARAM_FILE_REMAIN_SIZE:
			if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
				UINT64 *file_remain_size = (UINT64 *)Value;
				gNMPTsDemuxBufInfo.file_remain_size = *file_remain_size;
			}
			break;
		default:
			DBG_ERR("[%d]Set invalid param = %d\r\n", id, Param);
			ret = E_PAR;
            break;
	}
	return ret;
}

static ER NMI_BsDemux_GetParam(UINT32 id, UINT32 Param, UINT32 *pValue)
{
	ER ret = E_OK;

	switch (Param) {

	case NMP_BSDEMUX_PARAM_FILESIZE:
		if (gNMPBsDemuxObj[id].file_fmt == MEDIA_FILEFORMAT_TS) {
			*((UINT64*)pValue) = gNMPTsDemuxBufInfo.file_ttlsize;
		}
		break;

		default:
			DBG_ERR("[BSDEMUX][%d] Get invalid param = %d\r\n", id, Param);
			ret = E_PAR;
			break;
	}

	return ret;
}

static ER NMI_BsDemux_Action(UINT32 id, UINT32 Action)
{
	ER ret = E_OK;

	BSDMX_DUMP("[NMP_BsDmx][%d]Action = %d\r\n", id, Action);

	switch (Action) {
		default:
			DBG_ERR("[%d]Invalid action = %d\r\n", id, Action);
            ret = E_PAR;
			break;
	}

	return ret;
}

static ER NMI_BsDemux_In(UINT32 id, UINT32 *pBuf)
{
    NMP_BSDEMUX_IN_BUF *p_in_buf = (NMP_BSDEMUX_IN_BUF *)pBuf;

	if (gNMPBsDemuxObj[id].file_fmt == NMP_BSDEMUX_FILEFMT_MP4) {
		BSDMX_DUMP("[NMP_BsDmx][%d]start_bs: bs_type=%d, index=%d\r\n",
			id, p_in_buf->bs_type, p_in_buf->index);
	}
	/*BSDMX_DUMP("[NMP_BsDmx][%d]in_buf: addr=0x%lx, offset=0x%llx, size=0x%lx\r\n",
			id, p_in_buf->addr, p_in_buf->offset, p_in_buf->size);*/

	// put to queue & trigger demux
	if (p_in_buf->bs_type == NMP_BSDEMUX_BS_TPYE_VIDEO && gNMPBsDemuxObj[id].vdo_en) {
		_NMP_BsDemux_PutVQ(id, p_in_buf);
		set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_VDO_DEMUX);
	} else if (p_in_buf->bs_type == NMP_BSDEMUX_BS_TPYE_AUDIO && gNMPBsDemuxObj[id].aud_en) {
		_NMP_BsDemux_PutAQ(id, p_in_buf);
		set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_AUD_DEMUX);
	}

	return E_OK;
}

NMI_UNIT NMI_BsDemux = {
	.Name           = "BsDemux",
	.Open           = NMI_BsDemux_Open,
	.SetParam       = NMI_BsDemux_SetParam,
	.GetParam       = NMI_BsDemux_GetParam,
	.Action         = NMI_BsDemux_Action,
	.In             = NMI_BsDemux_In,
	.Close          = NMI_BsDemux_Close,
};

SXCMD_BEGIN(bsdmx, "bsdemux module")
// TsDmx debug
SXCMD_ITEM("tsdbg %d",  Cmd_TsDemux_SetDebug,           "TsDmx set debug level (1->on, 0->off)")
SXCMD_END()

void NMP_BsDemux_AddUnit(void)
{
	NMI_AddUnit(&NMI_BsDemux);
	//SxCmd_AddTable(bsdmx);
}
