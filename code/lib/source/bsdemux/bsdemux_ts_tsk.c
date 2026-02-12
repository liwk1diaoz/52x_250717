/*
    Novatek Media Play TS (Transport Stream) Demux Task.

    This file is the task of novatek media player TS (Transport Stream) demux.

    @file       bsdemux_ts_tsk.c
    @ingroup    mlib
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "avfile/AVFile_ParserTs.h"
#include "bsdemux_id.h"
#include "bsdemux_tsk.h"
#include "bsdemux_ts_tsk.h"
#include "sxcmd_wrapper.h"
#include "comm/tse.h"
#include "kwrap/util.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          bsdemux_ts_tsk
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

typedef struct {
	UINT32 en;
	UINT32 pid;
	UINT32 cont_mode;
	UINT32 cont_val;
} FILT_INFO;

#define USE_HW_TSE          0

NMP_TSDEMUX_OBJ                     gNMPTsDemuxObj = {0};
NMP_TSDEMUX_BUFINFO                 gNMPTsDemuxBufInfo = {0};

// extern
extern NMP_BSDEMUX_OBJ              gNMPBsDemuxObj;
extern CONTAINERPARSER             *pNMPBsDemuxContainer;

// Debug
static UINT32                       gNMPTsDemuxDebugLevel = 0;
#define TSDMX_DUMP(fmtstr, args...) if (gNMPTsDemuxDebugLevel) DBG_DUMP(fmtstr, ##args)

UINT32 _NMP_TsDemux_GetBufSize(UINT32 pathID)
{
	UINT32 total_buf_size;

	if (gNMPBsDemuxObj.file_fmt != MEDIA_FILEFORMAT_TS) {
		DBG_ERR("file format is not TS\r\n");
		return 0;
	}

	if (gNMPTsDemuxObj.bufblk_num == 0) {
		gNMPTsDemuxObj.bufblk_num = NMP_TSDEMUX_BUFBLK_CNT;
	}

	total_buf_size = NMP_TSDEMUX_BUFBLK_UNIT_SIZE * gNMPTsDemuxObj.bufblk_num;
	return total_buf_size;
}

ER _NMP_TSDemux_ConfigBufInfo(UINT32 buf_addr, UINT32 buf_size)
{
	gNMPTsDemuxBufInfo.start_addr = buf_addr;
	gNMPTsDemuxBufInfo.end_addr = buf_addr + buf_size;
	gNMPTsDemuxBufInfo.cur_addr = gNMPTsDemuxBufInfo.start_addr;
	gNMPTsDemuxBufInfo.read_accum_size = 0;
	gNMPTsDemuxBufInfo.demux_accum_size = 0;
	gNMPTsDemuxBufInfo.use_accum_size = 0;
	gNMPTsDemuxBufInfo.file_remain_size = gNMPTsDemuxBufInfo.file_ttlsize;
	gNMPTsDemuxBufInfo.b_read_finished = FALSE;

	if (gNMPTsDemuxBufInfo.start_addr) {
		if (pNMPBsDemuxContainer->SetInfo) {
			(pNMPBsDemuxContainer->SetInfo)(MEDIAREADLIB_SETINFO_MEMSTARTADDR, gNMPTsDemuxBufInfo.start_addr, 0, 0);
		}
	}

	return E_OK;
}

static void _NMP_TSDemux_Callback(UINT32 demuxSize, UINT32 duplicateSize)
{
    gNMPTsDemuxBufInfo.demux_accum_size += demuxSize;
    gNMPTsDemuxBufInfo.cur_addr += demuxSize;
	//duplicated frame size in recover frame
	if (duplicateSize != 0) {
		gNMPTsDemuxBufInfo.demux_accum_size -= duplicateSize;
	}
	//DBG_DUMP("callback dmx_ac_size=0x%llx, demuxSize=0x%x, dup=0x%x\r\n", gNMPTsDemuxBufInfo.demux_accum_size, demuxSize, duplicateSize);
}

ER _nmp_tsdemux_tse_demux_trig(TSE_BUF_INFO *in_buf, TSE_BUF_INFO *out_buf)
{
	ER ret = 0;
	UINT32 sync_byte, adapt_field_flags;
	FILT_INFO filt = {0};

	sync_byte = 0x47;
	adapt_field_flags = 0;

	/* config filter info */
	filt.en = 1;
	filt.pid = 0x1fff;
	filt.cont_mode = 0;
	filt.cont_val = 0;

	/* trigger demux */
	tse_open();

	tse_setConfig(TSDEMUX_CFG_ID_SYNC_BYTE, sync_byte);
	tse_setConfig(TSDEMUX_CFG_ID_ADAPTATION_FLAG, adapt_field_flags);
	tse_setConfig(TSDEMUX_CFG_ID_IN_INFO, (UINT32)in_buf);

	tse_setConfig(TSDEMUX_CFG_ID_PID0_ENABLE, filt.en);
	if (filt.en) {
		tse_setConfig(TSDEMUX_CFG_ID_PID0_VALUE, filt.pid);
		tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY0_MODE, filt.cont_mode);
		tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY0_VALUE, filt.cont_val);
		tse_setConfig(TSDEMUX_CFG_ID_OUT0_INFO, (UINT32)out_buf);
	}

	ret = tse_start(TRUE, TSE_MODE_TSDEMUX);
	if (ret != E_OK) {
		DBG_ERR("tse_start fail(%d)\r\n", ret);
		goto exit;
	}

	/* update out_buf size */
	out_buf->size = tse_getConfig(TSDEMUX_CFG_ID_OUT0_TOTAL_LEN);

exit:
	tse_close();
	return ret;
}

#if USE_HW_TSE
static ER _nmp_tsdemux_prepare_demux(UINT32 src_addr, UINT32 src_size, UINT32 dst_addr, UINT32 *p_dst_size)
{
	ER ret;
	TSE_BUF_INFO in_buf = {0}, out_buf = {0};

	/* gen in_buf */
	if (src_addr == 0) {
		DBG_ERR("Invalid src_addr = 0x%x\r\n", src_addr);
		return E_SYS;
	}
	if ((src_size) % 188 != 0) {
		DBG_ERR("Invalid src_size = %d\r\n", src_size);
		return E_SYS;
	}
    in_buf.addr = src_addr;
    in_buf.size = src_size;
	in_buf.pnext = NULL;
	DBG_DUMP("(prepare_demux) in: addr = 0x%08x, size = %d\r\n", in_buf.addr, in_buf.size);

	/* gen out_buf */
	if (dst_addr == 0) {
		DBG_ERR("Invalid dst_addr = 0x%x\r\n", dst_addr);
		return E_SYS;
	}
	if (p_dst_size == NULL) {
		DBG_ERR("Invalid p_dst_size = NULL\r\n");
		return E_SYS;
	}
	out_buf.addr = dst_addr;
	out_buf.size = src_size; // use src_size as size of out_buf first
	out_buf.pnext = NULL;
	DBG_DUMP("(prepare_demux) out: addr = 0x%08x, size = %d\r\n", out_buf.addr, out_buf.size);
	
	/* trigger hw demux */
	ret = _nmp_tsdemux_tse_demux_trig(&in_buf, &out_buf);
	if (ret != E_OK) {
		return ret;
	}

	/* update used_size */
	if (out_buf.size == 0) {
		DBG_ERR("out_buf.size = 0\r\n");
		return E_SYS;
	}
	*p_dst_size = out_buf.size;
	DBG_DUMP("(func_trig) used_size = %d\r\n", *p_dst_size);

	return E_OK;
}
#endif

THREAD_DECLARE(_NMP_TsDemux_Tsk, arglist) //void _NMP_TsDemux_Tsk(void)
{
	FLGPTN flag = 0, wait_flag = 0;

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_IDLE);
	wait_flag = FLG_NMP_TSDEMUX_DEMUX | FLG_NMP_TSDEMUX_STOP;

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&flag, NMP_TSDEMUX_FLG_ID, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_IDLE);

		if(flag & FLG_NMP_TSDEMUX_STOP){
			break;
		}

		if (flag & FLG_NMP_TSDEMUX_DEMUX) { // Trigger demux TS packets
#if USE_HW_TSE
			_nmp_tsdemux_prepare_demux(gNMPTsDemuxObj.read_pos, gNMPTsDemuxObj.read_size, gNMPTsDemuxObj.dst_addr, &gNMPTsDemuxObj.used_size);
			DBG_DUMP("(task_trig) used_size = %d\r\n", gNMPTsDemuxObj.used_size);
#else
			UINT32 addr, size, demux_addr;
			addr = gNMPTsDemuxObj.read_pos;
            size = gNMPTsDemuxObj.read_size;
            demux_addr = gNMPTsDemuxObj.dst_addr;
			TSDMX_DUMP("(task) demux TS: pos=0x%x, size=0x%x, dst=0x%x\r\n", addr, size, demux_addr);
            TsReadLib_DemuxTsStream(addr, size, demux_addr, &gNMPTsDemuxObj.used_size);

            //set_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_DONE);
#endif
		}

	} // end of while loop

	set_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_DONE);

	THREAD_RETURN(0); //ext_tsk();
}

ER _NMP_TsDemux_TskStart(void)
{
    THREAD_CREATE(NMP_TSDEMUX_TSK_ID, _NMP_TsDemux_Tsk, NULL, "_NMP_TsDemux_Tsk"); //sta_tsk(NMP_TSDEMUX_TSK_ID, 0);
	if (NMP_TSDEMUX_TSK_ID == 0) {
		DBG_ERR("Invalid NMP_TSDEMUX_TSK_ID\r\n");
		return E_OACV;
	}

	THREAD_RESUME(NMP_TSDEMUX_TSK_ID);
	TSDMX_DUMP("[NMP_BsDmx]TsDemux task start ...\r\n");

	return E_OK;
}

static ER _NMP_TsDemux_TskStop(void)
{
	FLGPTN flag;

	TSDMX_DUMP("[BSDMX]tskstop wait idle ..\r\n");
	wai_flg(&flag, NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_IDLE, TWF_ORW);
	TSDMX_DUMP("[BSDMX]tskstop wait idle OK\r\n");

	set_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_STOP);
	wai_flg(&flag, NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_DONE, TWF_ORW | TWF_CLR);

	return E_OK;
}

void _NMP_TsDemux_Open(void)
{
#if USE_HW_TSE
	tse_init();
#endif

    // start TS demux task
    if (_NMP_TsDemux_TskStart() == E_OK)
    {
        TSDMX_DUMP("start demux tsk ok\r\n");
    }
    TsReadLib_registerTSDemuxCallback(_NMP_TSDemux_Callback);

	if ((gNMPTsDemuxBufInfo.file_remain_size - gNMPTsDemuxBufInfo.read_accum_size) < 0x200000) {
		gNMPTsDemuxBufInfo.b_read_finished = TRUE;
	}
}

ER _NMP_TsDemux_Close(void)
{
	ER ret;

#if USE_HW_TSE
	tse_uninit();
#endif

	// stop task
	ret = _NMP_TsDemux_TskStop();
	if (ret != E_OK) {
		DBG_ERR("tsk stop fail(%d)\r\n", ret);
		return ret;
	}

	return E_OK;
}

#if USE_HW_TSE
static ER _NMP_TSDemux_TrigDemux(BOOL b_wait)
{
    if (b_wait) {
		_nmp_tsdemux_prepare_demux(gNMPTsDemuxObj.read_pos, gNMPTsDemuxObj.read_size, gNMPTsDemuxObj.dst_addr, &gNMPTsDemuxObj.used_size);
		DBG_DUMP("(func_trig) used_size = %d\r\n", gNMPTsDemuxObj.used_size);
    }
    else {
        set_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_DEMUX);
    }

    return E_OK;
}
#else
static ER _NMP_TSDemux_TrigDemux(BOOL b_wait)
{
    UINT32 addr, size, demux_addr;

    if (b_wait) {
        addr = gNMPTsDemuxObj.read_pos;
        size = gNMPTsDemuxObj.read_size;
        demux_addr = gNMPTsDemuxObj.dst_addr;
		TSDMX_DUMP("(func) demux TS: pos=0x%x, size=0x%x, dst=0x%x\r\n", addr, size, demux_addr);
        TsReadLib_DemuxTsStream(addr, size, demux_addr, &gNMPTsDemuxObj.used_size);
    }
    else {
        set_flg(NMP_TSDEMUX_FLG_ID, FLG_NMP_TSDEMUX_DEMUX);
    }

    return E_OK;
}
#endif


ER _NMP_TSDemux_SetInfo(UINT32 type, UINT32 param1, UINT32 param2)
{
    switch (type)
    {
        case NMP_TSDEMUX_SETINFO_SRC_POS_SIZE:
			gNMPTsDemuxObj.read_pos = param1;
			gNMPTsDemuxObj.read_size = param2;

			// rollback checking
			//DBG_DUMP("cur_addr=0x%x + read_size=0x%x > end_addr=0x%x\r\n",
			//		gNMPTsDemuxBufInfo.cur_addr, gNMPTsDemuxObj.read_size, gNMPTsDemuxBufInfo.end_addr);
		    if ((gNMPTsDemuxBufInfo.cur_addr + gNMPTsDemuxObj.read_size) > gNMPTsDemuxBufInfo.end_addr)
		    {
		        //DBG_DUMP("^Y TS rollback...\r\n");
		        gNMPTsDemuxBufInfo.cur_addr = gNMPTsDemuxBufInfo.start_addr;
		    }
			gNMPTsDemuxObj.dst_addr = gNMPTsDemuxBufInfo.cur_addr;

			// trigger demux
#if 0
			if (gNMPTsDemuxBufInfo.read_accum_size <= 0x200000)
			{
				_NMP_TSDemux_TrigDemux(TRUE);
			} else if (gNMPBsDemuxObj.vdo_hei > 1080 && gNMPTsDemuxBufInfo.read_accum_size <= 0x400000) {
				_NMP_TSDemux_TrigDemux(TRUE);
			} else {
				
				TsReadLib_setDemuxFinished(FALSE);
				
				_NMP_TSDemux_TrigDemux(FALSE);

				while(!TsReadLib_DemuxFinished())
				{
					vos_util_delay_ms(20);
				}
			}
#else
			_NMP_TSDemux_TrigDemux(TRUE);
#endif

			gNMPTsDemuxBufInfo.read_accum_size += gNMPTsDemuxObj.read_size;
            break;
        case NMP_TSDEMUX_SETINFO_CLEAR_FRAME_ENTRY:
            TsReadLib_ClearFrameEntry();
            break;
        default:
			DBG_ERR("Invalid type id: %d\r\n", type);
            break;
    }

    return E_OK;

}

/**
    @name  Test commands
*/
BOOL Cmd_TsDemux_SetDebug(CHAR *strCmd)
{
	UINT32 dbg_msg;

	sscanf_s(strCmd, "%d", &dbg_msg);

	gNMPTsDemuxDebugLevel = (dbg_msg > 0);

    DBG_DUMP("[TSDMX] set debug level %d\r\n", gNMPTsDemuxDebugLevel);

	return TRUE;
}

