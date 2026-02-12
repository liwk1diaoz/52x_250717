/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       MOVRvcy.c
    @ingroup    mIAVMOV

    @brief      MOV file writer
                It's the MOV file writer, support multi/single payload now

    @note       Nothing.
    @version    V1.00.000
    @date       2019/05/05
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include <string.h>

#include "kernel.h"
//#include "FileSysTsk.h"//2012/07/13 Meg
#include "MediaWriteLib.h"
#include "AVFile_MakerMov.h"
#include "MOVLib.h"
#include "movRec.h"
#include "rtc.h"
#include "Debug.h"
#include "MovWriteLib.h"
#include "AudioEncode.h"

#define __MODULE__          MOVWrite
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
#else
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"

#define __MODULE__          MOVRvcy
#define __DBGLVL__          2
#include "kwrap/debug.h"
unsigned int MOVRvcy_debug_level = NVT_DBG_WRN;

#include "avfile/MediaWriteLib.h"
#include "avfile/AVFile_MakerMov.h"
#include "avfile/MOVLib.h"

// INCLUDE PROTECTED
#include "movRec.h"
#include "MovWriteLib.h"

#define MP_AUDENC_AAC_RAW_BLOCK  		1024 	//encode one aac frame needs 1024 samples
#endif
//static UINT32 MOVRcvy_SearchFinalNidxWithTimerID(UINT32 timerID, UINT32 totalfilesize);

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
//for mov recovery -begin
//char    gMov264VidDesc[0x40];
UINT32          gMovRecvy_nidxNum     = 0;
UINT32          gMovRecvy_Back1sthead = 0;
UINT32          gMovRecvy_CluReadAddr = 0;
UINT32          gMovRecvy_LastBlockAddr = 0;
UINT32          gMovRecvy_entryAddr   = 0;
UINT32          gMovRecvy_entrySize   = 0;
UINT32          gMovRecvy_newMoovAddr = 0;
UINT32          gMovRecvy_newMoovSize = 0;
UINT32          gMovRecvyVidEntryMax  = 0;
UINT32          gMovRecvyAudEntryMax  = 0;
UINT32          gMovRecvy_totalVnum   = 0;
UINT32          gMovRecvy_totalAnum   = 0;
UINT32          gMovRecvy_totalSVnum  = 0;
UINT32          gMovRecvy_audTotalSize = 0;
UINT32          gMovRecvy_vfr = 0, gMovRecvy_vidWid = 0;
UINT32          gMovRcvy_Hdrsize = 0;//2012/09/11 Meg Lin
UINT64          gMovRcvy_NewTruncatesize = 0;//2015/12/7
//for mov recovery -end
UINT32          gMovRvcyMsg = 0, gMovSearchMsg = 0;
UINT32          gMovRecvy_audcodec = 0;
ER MOVRcvy_GetNidxTbl(UINT32 id, UINT64 filesize);
ER MOVRcvy_UpdateMdatHDR(UINT32 id);
static UINT32 MOVRcvy_SearchSpecificCC(UINT32 tag, UINT32 startaddr, UINT32 size);
static UINT64 MOVRcvy_SearchFinalNidxWithTimerID_2(UINT32 timerID, UINT64 totalfilesize);
static UINT64 MOVRcvy_2015_CheckNidx_co64(UINT32 id, UINT32 memAddr, UINT64 filepos);
static UINT64 MOVRcvy_SM_CheckNidx_co64(UINT32 id, UINT32 memAddr, UINT64 filepos);

static UINT64 MOVRcvy_DUAL_CheckNidx_co64(UINT32 id, UINT32 memAddr, UINT64 filepos);

#define MOVRVCY_DUMP_MUST(fmtstr, args...) do { if(gMovRvcyMsg) DBG_DUMP(fmtstr, ##args); } while(0)
#define MR_DUMP_SEARCH(fmtstr, args...) do { if(gMovSearchMsg) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
/** \addtogroup mIAVMOV */
//@{
void MOVRcvy_OpenMsg(UINT32 value)//2015/11/09
{
	gMovRvcyMsg = value;
}

/// ========== parameter function area ==========
typedef enum {
	MOV_RCVY_PARAM_SETTING,
	MOV_RCVY_PARAM_MAX_CFG,
} MOV_RCVY_PARAM;


static UINT32 mov_rcvy_get_header_size(UINT32 id, MOV_RCVY_PARAM param)
{
	UINT32 header_size;
	if (param == MOV_RCVY_PARAM_SETTING) {
		if (gMovRecFileInfo[id].hdr_revSize) {
			header_size = gMovRecFileInfo[id].hdr_revSize;
		} else {
			header_size = MOV_MEDIARECVRY_MAXCLUSIZE;
			DBG_DUMP("use maxcfg 0x%x since MEDIAWRITE_SETINFO_MOVHDR_REVSIZE is not setting\r\n", header_size);
		}
	} else if (param == MOV_RCVY_PARAM_MAX_CFG) {
		header_size = MOV_MEDIARECVRY_MAXCLUSIZE;
	} else {
		DBG_ERR("param 0x%x invalid\r\n", param);
	}
	return header_size;
}

static UINT32 mov_rcvy_get_cluster_size(UINT32 id, MOV_RCVY_PARAM param)
{
	UINT32 cluster_size;
	if (param == MOV_RCVY_PARAM_SETTING) {
		if (gMovRecFileInfo[id].clustersize) {
			cluster_size = gMovRecFileInfo[id].clustersize;
		} else {
			cluster_size = MOV_MEDIARECVRY_MAXCLUSIZE;
			DBG_DUMP("use maxcfg 0x%x since MEDIAWRITE_SETINFO_MOVHDR_REVSIZE is not setting\r\n", cluster_size);
		}
	} else if (param == MOV_RCVY_PARAM_MAX_CFG) {
		cluster_size = MOV_MEDIARECVRY_MAXCLUSIZE;
	} else {
		DBG_ERR("param 0x%x invalid\r\n", param);
	}
	return cluster_size;
}

static UINT32 mov_rcvy_get_entry_size(UINT32 id, MOV_RCVY_PARAM param)
{
	UINT32 entry_size;
	UINT32 vfr, asr, dur;
	if (param == MOV_RCVY_PARAM_SETTING) {
		vfr = ((gMovRecFileInfo[id].max_vfr_cfg) ? gMovRecFileInfo[id].max_vfr_cfg : 60);
		asr = (((gMovRecFileInfo[id].max_asr_cfg) ? gMovRecFileInfo[id].max_asr_cfg : 32000) / 1024) + 1;
		dur = ((gMovRecFileInfo[id].max_dur_cfg) ? gMovRecFileInfo[id].max_dur_cfg : 15 * 60);
		entry_size = (dur * (vfr + asr) * MP_MOV_SIZE_CO64_ONEENTRY);
	} else if (param == MOV_RCVY_PARAM_MAX_CFG) {
		entry_size = (MOV_MEDIARECVRY_MAXFRAME * MP_MOV_SIZE_CO64_ONEENTRY);
	} else {
		DBG_ERR("param 0x%x invalid\r\n", param);
	}
	return entry_size;
}

static UINT32 mov_rcvy_get_block_size(UINT32 id, MOV_RCVY_PARAM param)
{
	UINT32 block_size;
	if (param == MOV_RCVY_PARAM_SETTING) {
		if (gMovRecFileInfo[id].rcvy_blksize) {
			block_size = gMovRecFileInfo[id].rcvy_blksize;
		} else {
			block_size = MOV_MEDIARECVRY_MAXBLKSIZE_BIG;
			DBG_DUMP("use maxcfg 0x%x since MEDIAWRITE_SETINFO_RECVRY_BLKSIZE is not setting\r\n", block_size);
		}
	} else if (param == MOV_RCVY_PARAM_MAX_CFG) {
		block_size = MOV_MEDIARECVRY_MAXBLKSIZE_BIG;
	} else {
		DBG_ERR("param 0x%x invalid\r\n", param);
	}
	return block_size;
}

static UINT32 mov_rcvy_get_moov_size(UINT32 id, MOV_RCVY_PARAM param)
{
	UINT32 moov_size;
	if (param == MOV_RCVY_PARAM_SETTING) {
		moov_size = 0x100000;
	} else if (param == MOV_RCVY_PARAM_MAX_CFG) {
		moov_size = MOV_MEDIARECVRY_MAXBLKSIZE;
	} else {
		DBG_ERR("param 0x%x invalid\r\n", param);
	}
	return moov_size;
}

/// ========== config function area ==========
static UINT32 mov_rcvy_calc_buffer_usage(UINT32 id)
{
	UINT32 total_size;
	total_size = mov_rcvy_get_header_size(id, MOV_RCVY_PARAM_SETTING) +
		mov_rcvy_get_cluster_size(id, MOV_RCVY_PARAM_SETTING) +
		mov_rcvy_get_entry_size(id, MOV_RCVY_PARAM_SETTING) +
		mov_rcvy_get_block_size(id, MOV_RCVY_PARAM_SETTING) +
		mov_rcvy_get_moov_size(id, MOV_RCVY_PARAM_SETTING);
	return total_size;
}

static ER mov_rcvy_set_entry_addr(UINT32 id)
{
	UINT32 vfr = 60;
	UINT32 asr = 32000;
	UINT32 entry_addr = gMovRecvy_entryAddr;
	UINT32 entry_size = gMovRecvy_entrySize;
	MOV_Write_SetEntryAddr(entry_addr, entry_size, asr, vfr, id);
	return E_OK;
}

/// ========== searching function area ==========
//only for ver 3.20 and ver 3.21
static UINT64 mov_rcvy_check_data_blk_v320(UINT32 mem_addr, UINT32 co64)
{
	UINT32 data_end_shift = 0;
	UINT32 *ptr32, *ptrh264, h264len, *sub32;
	MOV_2015_NIDXINFO *ptr_nidx_data;
	MOV_2015_CO64_NIDXINFO *ptr_co64_nidx_data;
	UINT32 tmp, size_oneE;
	UINT32 thisvfn, vfnshift;
	UINT32 thisafn, afnshift;
	UINT32 descshift;
	UINT32 subshift;
	UINT32 clursize, vfr;

	//If here, it means at least version code is fine.
	//So, clursize is also fine and can be used to check.
	ptr32 = (UINT32 *)mem_addr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	MR_DUMP_SEARCH("clursize=0x%lx co64=%d\r\n", (unsigned long)clursize, co64);

	if (co64) {
		ptr_co64_nidx_data = (MOV_2015_CO64_NIDXINFO *)mem_addr;
		if (ptr_co64_nidx_data->versionCode != SMDUAL_RCVY_CO64_VERSION) {
			return MOV_NOT_NIDX;
		}

		//1. main-stream, use clursize to check vfnshift is valid or not
		size_oneE = MP_MOV_SIZE_CO64_ONEENTRY;
		thisvfn =  ptr_co64_nidx_data->vidtotalFN;
		vfnshift = thisvfn * size_oneE + 8 + MP_MOV_2015CO64NIDX_FIXEDSIZE;
		if (vfnshift > clursize) {
			DBG_ERR("vfnshift(%d) invalid\r\n", vfnshift);
			goto exit;
		}
		MR_DUMP_SEARCH("vfnshift=%d\r\n", vfnshift);

		//2. audio, use clursize to check vfnshift is valid or not
		ptr32 = (UINT32 *)(mem_addr + vfnshift);
		thisafn = *ptr32;
		afnshift = thisafn * size_oneE + 4;
		if (vfnshift + afnshift > clursize) {
			DBG_ERR("afnshift(%d) invalid\r\n", afnshift);
			goto exit;
		}
		MR_DUMP_SEARCH("afnshift=%d\r\n", afnshift);

		//3. main-stream desc, use clursize to check data_end_shift is valid or not
		ptrh264 = (UINT32 *)(mem_addr + vfnshift + afnshift);
		h264len = *ptrh264++;
		descshift = h264len + 4;
		data_end_shift += vfnshift + afnshift + descshift;
		if (data_end_shift > clursize) {
			DBG_ERR("descshift(%d) invalid\r\n", descshift);
			goto exit;
		}

		//4. sub-stream (optional)
		sub32 = (UINT32 *)(mem_addr + data_end_shift);
		tmp = *sub32++;
		tmp = *sub32++;
		tmp = *sub32++;
		tmp = *sub32++;
		vfr = *sub32++;
		if (vfr) {
			thisvfn = *sub32++;
			subshift = thisvfn * size_oneE + 4 + 4 * 5; //size(4 bytes) + entry*total + subinfo
			if (subshift > clursize) {
				DBG_ERR("subshift(%d) invalid\r\n", subshift);
				goto exit;
			}
			ptrh264 = (UINT32 *)(mem_addr + data_end_shift + subshift);
			h264len = *ptrh264++;
			descshift = h264len + 4;
			data_end_shift += subshift + descshift;
			if (data_end_shift > clursize) {
				DBG_ERR("sub-descshift(%d) invalid\r\n", descshift);
				goto exit;
			}
		}
		MR_DUMP_SEARCH("data_end_shift=%d\r\n", data_end_shift);

	} else {
		ptr_nidx_data = (MOV_2015_NIDXINFO *)mem_addr;
		if (ptr_nidx_data->versionCode != SMDUAL_RCVY_VERSION) {
			return MOV_NOT_NIDX;
		}

		//1. main-stream, use clursize to check vfnshift is valid or not
		size_oneE = MP_MOV_SIZE_CO64_ONEENTRY;
		thisvfn =  ptr_nidx_data->vidtotalFN;
		vfnshift = thisvfn * size_oneE + 8 + MP_MOV_2015NIDX_FIXEDSIZE;
		if (vfnshift > clursize) {
			DBG_ERR("vfnshift(%d) invalid\r\n", vfnshift);
			goto exit;
		}
		MR_DUMP_SEARCH("vfnshift=%d\r\n", vfnshift);

		//2. audio, use clursize to check vfnshift is valid or not
		ptr32 = (UINT32 *)(mem_addr + vfnshift);
		thisafn = *ptr32;
		afnshift = thisafn * size_oneE + 4;
		if (vfnshift + afnshift > clursize) {
			DBG_ERR("afnshift(%d) invalid\r\n", afnshift);
			goto exit;
		}
		MR_DUMP_SEARCH("afnshift=%d\r\n", afnshift);

		//3. main-stream desc, use clursize to check data_end_shift is valid or not
		ptrh264 = (UINT32 *)(mem_addr + vfnshift + afnshift);
		h264len = *ptrh264++;
		descshift = h264len + 4;
		data_end_shift += vfnshift + afnshift + descshift;
		if (data_end_shift > clursize) {
			DBG_ERR("descshift(%d) invalid\r\n", descshift);
			goto exit;
		}

		//4. sub-stream (optional)
		sub32 = (UINT32 *)(mem_addr + data_end_shift);
		tmp = *sub32++;
		tmp = *sub32++;
		tmp = *sub32++;
		tmp = *sub32++;
		vfr = *sub32++;
		if (vfr) {
			thisvfn = *sub32++;
			subshift = thisvfn * size_oneE + 4 + 4 * 5; //size(4 bytes) + entry*total + subinfo
			if (subshift > clursize) {
				DBG_ERR("subshift(%d) invalid\r\n", subshift);
				goto exit;
			}
			ptrh264 = (UINT32 *)(mem_addr + data_end_shift + subshift);
			h264len = *ptrh264++;
			descshift = h264len + 4;
			data_end_shift += subshift + descshift;
			if (data_end_shift > clursize) {
				DBG_ERR("sub-descshift(%d) invalid\r\n", descshift);
				goto exit;
			}
		}
		MR_DUMP_SEARCH("data_end_shift=%d\r\n", data_end_shift);
	}

	//Here is to check CLUR of data_end_shift is valid or not
	ptr32 = (UINT32 *)(mem_addr + data_end_shift);
	if (*ptr32 == MOV_NIDX_CLUR) {
		MR_DUMP_SEARCH("data_end: MOV_NIDX_CLUR\r\n");
	} else if (*ptr32 == 0x00000000) {
		MR_DUMP_SEARCH("data_end: 0x00000000\r\n");
	} else {
		goto exit;
	}
	return MOV_NIDX_CLUR; //valid

exit:
	return MOV_NOT_NIDX; //invalid
}

static UINT64 mov_rcvy_check_data_blk(UINT32 mem_addr)
{
	MOV_2015_NIDXINFO *ptr_nidx_data;
	MOV_2015_CO64_NIDXINFO *ptr_co64_nidx_data;

	ptr_nidx_data = (MOV_2015_NIDXINFO *)mem_addr;
	ptr_co64_nidx_data = (MOV_2015_CO64_NIDXINFO *)mem_addr;

	if (ptr_co64_nidx_data->versionCode == SMDUAL_RCVY_CO64_VERSION) {
		return mov_rcvy_check_data_blk_v320(mem_addr, 1);
	} else if (ptr_nidx_data->versionCode == SMDUAL_RCVY_VERSION) {
		return mov_rcvy_check_data_blk_v320(mem_addr, 0);
	} else {
		DBG_IND("no need to supported\r\n");
	}
	return 0;
}

/// ========== recovery function area ==========
static ER mov_rcvy_init(UINT32 id)
{
	ER result = E_OK;
	// initialization
	gMovRcvy_NewTruncatesize = 0;
	gMovRcvy_Hdrsize = 0;
	gMovRecvy_vidWid = 0;
	gMovRecvy_nidxNum = 0;
	gMovRecvy_totalVnum = 0;
	gMovRecvy_totalAnum = 0;
	gMovRecvy_totalSVnum = 0;
	//enable sub-stream
	gMovRecFileInfo[id].SubVideoFrameRate = 60;
	return result;
}

static ER mov_rcvy_uninit(UINT32 id)
{
	ER result = E_OK;
	//disable sub-stream
	gMovRecFileInfo[id].SubVideoFrameRate = 0;
	return result;
}
static ER mov_rcvy_buffer_arrangement(UINT32 id)
{
	ER result = E_OK;
	UINT32 mem_addr, mem_size;
	UINT32 cal_size;

	// 0. total buffer
	mem_addr = gMovRecFileInfo[id].bufaddr;
	mem_size = gMovRecFileInfo[id].bufsize;
	cal_size = mov_rcvy_calc_buffer_usage(id);
	if (mem_size < cal_size) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("Need more than 0x%x, not 0x%x!!\r\n", cal_size, mem_size);
		}
		return E_NOSPT;
	}
	MOVRVCY_DUMP_MUST("(0)Total addr=0x%x size=0x%x\r\n", mem_addr, mem_size);

	// 1. header buffer (output) -> (0x40000 | 64KB)(setting)
	cal_size = mov_rcvy_get_header_size(id, MOV_RCVY_PARAM_SETTING);
	if (mem_size < cal_size) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("Back1sthead not enough 0x%x, 0x%x\r\n", cal_size, mem_size);
		}
		return E_NOSPT;
	}
	gMovRecvy_Back1sthead = mem_addr;
	mem_addr += cal_size;
	mem_size -= cal_size;
	MOVRVCY_DUMP_MUST("(1)Back1sthead: addr=0x%x size=0x%x\r\n", gMovRecvy_Back1sthead, cal_size);

	// 2. cluster buffer (read) -> (0x40000 | 64KB)(setting)
	cal_size = mov_rcvy_get_cluster_size(id, MOV_RCVY_PARAM_SETTING);
	if (mem_size < cal_size) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("CluRead not enough 0x%x, 0x%x\r\n", cal_size, mem_size);
		}
		return E_NOSPT;
	}
	gMovRecvy_CluReadAddr = mem_addr;
	mem_addr += cal_size;
	mem_size -= cal_size;
	MOVRVCY_DUMP_MUST("(2)CluRead: addr=0x%x size=0x%x\r\n", gMovRecvy_CluReadAddr, cal_size);

	// 3. entry buffer -> (max: 15*60*92*16 = 0x143700 | 1.3M)
	cal_size = mov_rcvy_get_entry_size(id, MOV_RCVY_PARAM_SETTING);
	if (mem_size < cal_size) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("entrySize not enough 0x%x, 0x%x\r\n", cal_size, mem_size);
		}
		return E_NOSPT;
	}
	gMovRecvy_entryAddr = mem_addr;
	gMovRecvy_entrySize = cal_size;
	mem_addr += cal_size;
	mem_size -= cal_size;
	MOVRVCY_DUMP_MUST("(3)entryBuf: addr=0x%x size=0x%x\r\n", gMovRecvy_entryAddr, gMovRecvy_entrySize);

	// 4. block buffer
	cal_size = mov_rcvy_get_block_size(id, MOV_RCVY_PARAM_SETTING);
	if (mem_size < cal_size) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("LastBlock not enough 0x%x, 0x%x\r\n", cal_size, mem_size);
		}
		return E_NOSPT;
	}
	gMovRecvy_LastBlockAddr = mem_addr;
	mem_addr += cal_size;
	mem_size -= cal_size;
	MOVRVCY_DUMP_MUST("(4)LastBlock: addr=0x%x size=0x%x\r\n", gMovRecvy_LastBlockAddr, cal_size);

	// 5. moov buffer (output)
	cal_size = mov_rcvy_get_moov_size(id, MOV_RCVY_PARAM_SETTING);
	if (mem_size < cal_size) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("MoovSize not enough 0x%x, 0x%x\r\n", cal_size, mem_size);
		}
		return E_NOSPT;
	}
	gMovRecvy_newMoovAddr = mem_addr;
	gMovWriteMoovTempAdr = gMovRecvy_newMoovAddr;
	gMovRecvy_newMoovSize = cal_size;
	mem_addr += cal_size;
	mem_size -= cal_size;
	MOVRVCY_DUMP_MUST("(5)WriteMoov: addr=0x%x size=0x%x\r\n", gMovRecvy_newMoovAddr, gMovRecvy_newMoovSize);

	//
	// setting
	//
	mov_rcvy_set_entry_addr(id);

	MOVRVCY_DUMP_MUST("(6)Remain: size=0x%x\r\n", mem_size);

	return result;
}

static ER mov_rcvy_get_nidx_tbl(UINT32 id)
{
	UINT64 filesize;
	ER result = E_OK;
	filesize = gMovRecFileInfo[id].recv_filesize;
	result = MOVRcvy_GetNidxTbl(id, filesize);
	return result;
}

static ER mov_rcvy_moov_recovery(UINT32 id)
{
	ER result = E_OK;
	UINT32 timescale, dur;
	UINT32 chs;

	// setting
	gMovRecFileInfo[id].videoFrameCount = gMovRecvy_totalVnum;
	gMovRecFileInfo[id].audioTotalSize = gMovRecvy_audTotalSize;
	timescale = MOV_Wrtie_GetTimeScale(gMovRecvy_vfr);
	DBGD(timescale);
	dur = gMovRecvy_totalVnum * (timescale / gMovRecvy_vfr);
	gMovRecFileInfo[id].VideoFrameRate = gMovRecvy_vfr;
	gMovRecFileInfo[id].PlayDuration = dur;
	if (gMovRecFileInfo[id].SubVideoTrackEn && gMovRecFileInfo[id].SubVideoFrameRate) {
		gMovRecFileInfo[id].SubvideoFrameCount = gMovRecvy_totalSVnum;
		timescale = MOV_Wrtie_GetTimeScale(gMovRecFileInfo[id].SubVideoFrameRate);
		DBGD(timescale);
		dur = gMovRecFileInfo[id].SubvideoFrameCount * (timescale / (gMovRecFileInfo[id].SubVideoFrameRate));
		gMovRecFileInfo[id].SubPlayDuration = dur;
	}
	chs = gMovRecFileInfo[id].AudioChannel;
	if ((gMovRecFileInfo[id].uinidxversion == SMEDIARECVY_VERSION)
		|| (gMovRecFileInfo[id].uinidxversion == SM2015_RCVY_VERSION)
		|| (gMovRecFileInfo[id].uinidxversion == SMDUAL_RCVY_VERSION)) {
		if (gMovRecvy_audcodec == 0) {
			gMovRecvy_audcodec = MOVAUDENC_AAC;
		} else {
			if (gMovRvcyMsg) {
				DBG_DUMP("^G audiocodec=%d \r\n", gMovRecvy_audcodec);
			}
		}
		gMovRecFileInfo[id].encAudioCodec = gMovRecvy_audcodec;
		gMovRecFileInfo[id].audioFrameCount = gMovRecvy_totalAnum;
		gMovRecFileInfo[id].audioTotalSize = gMovRecvy_totalAnum * MP_AUDENC_AAC_RAW_BLOCK * 2 * chs;
		if (gMovRvcyMsg) {
			DBG_DUMP("Audio %d \r\n", gMovRecvy_totalAnum);
		}
	} else {
		gMovRecFileInfo[id].encAudioCodec = MOVAUDENC_PCM;
		gMovRecFileInfo[id].audioFrameCount = gMovRecvy_nidxNum;
		if (gMovRvcyMsg) {
			DBG_DUMP("PCM %d \r\n", gMovRecvy_nidxNum);
		}
	}
	if (1) { //gMovRecFileInfo[id].recv_filesize >= (UINT64)0x100000000)
		MOVRVCY_DUMP_MUST("^R co64!!!\r\n");
		MovWriteLib_SetInfo(MEDIAWRITE_SETINFO_EN_CO64, (UINT32)1, 0, 0);
	} else {
		MOVRVCY_DUMP_MUST("^G stco!!!\r\n");
		MovWriteLib_SetInfo(MEDIAWRITE_SETINFO_EN_CO64, (UINT32)0, 0, 0);
	}

	gMovRecvy_newMoovSize = MOVRcvy_MakeMoovHeader(&gMovRecFileInfo[id]);
	DBG_DUMP("gMovRecvy_newMoovSize=0x%x\r\n", gMovRecvy_newMoovSize);

	return result;
}

/*
 *  read header data to buffer and update the size of mdat atom
 */
static ER mov_rcvy_header_recovery(UINT32 id)
{
	ER result = E_OK;
	UINT32 read_size;

	// read header data to header buffer
	read_size = mov_rcvy_get_header_size(id, MOV_RCVY_PARAM_SETTING);
	result = (gMovContainerMaker.cbReadFile)(0, 0, read_size, gMovRecvy_Back1sthead);
	if (result != E_OK) {
		DBG_DUMP("header_recovery: cbReadFile error!!\r\n");
		return E_SYS;
	}

	// update mdat header
	result = MOVRcvy_UpdateMdatHDR(id);
	if (result != E_OK) {
		DBG_DUMP("header_recovery: UpdateMdatHDR error!!\r\n");
	}

	return result;
}

static ER mov_rcvy_index_table_recovery(UINT32 id)
{
	ER     result = E_OK;

	// warning msg
	if (id != 0) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("ERROR!! ID 0 only supported!!!\r\n");
		}
		return E_NOSPT;
	}

	if (gMovContainerMaker.cbReadFile == NULL) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("ERROR!! cbReadFile=NULL!!!\r\n");
		}
		return E_NOSPT;
	}

	if (gMovRecFileInfo[id].recv_filesize == 0) { //>> filesize before recovery
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("Need filesize, call SetInfo() firstly!\r\n");
		}
		return E_NOSPT;
	}

	if (gMovRecFileInfo[id].clustersize == 0) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("Need cluster size, call SetInfo() firstly!\r\n");
		}
		return E_NOSPT;
	}

	// initialization
	result = mov_rcvy_init(id);
	if (result != E_OK) {
		DBG_ERR("init failed\r\n");
		return E_SYS;
	}

	// buffer arrangement
	result = mov_rcvy_buffer_arrangement(id);
	if (result != E_OK) {
		DBG_ERR("buffer_arrangement failed\r\n");
		return E_SYS;
	}

	// data from nidx table
	result = mov_rcvy_get_nidx_tbl(id);
	if (result != E_OK) {
		if (result != E_RSATR) {
			DBG_ERR("get_nidx_tbl other error!!\r\n");
			return E_SYS;
		} else {
			return E_RSATR;
		}
	}

	// make moov to (output) buffer
	result = mov_rcvy_moov_recovery(id);
	if (result != E_OK) {
		DBG_ERR("moov_recovery failed\r\n");
		return E_SYS;
	}

	// update header to (output) buffer
	result = mov_rcvy_header_recovery(id);
	if (result != E_OK) {
		DBG_ERR("header_recovery failed\r\n");
		return E_SYS;
	}

	// uninitialization
	result = mov_rcvy_uninit(id);
	if (result != E_OK) {
		DBG_ERR("uninit failed\r\n");
	}

	return result;
}

ER MOVRcvy_IndexTlb_Recovery(UINT32 id)
{
	return mov_rcvy_index_table_recovery(id);
}

static UINT64 MOVRcvy_DUAL_CheckNidx(UINT32 id, UINT32 memAddr, UINT64 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_2015_NIDXINFO      *ptrNidxData;
	MOV_2015_CO64_NIDXINFO      *ptr_co64_NidxData;
	UINT32               startVFN, vfr = 0, nidxlen, size_oneE, tmp, vfnshift = 0;
	UINT32               startAFN, afnshift = 0, thisvfn, thisafn;
	UINT32               *sub32, descshift = 0, subshift = 0;
	UINT8                i;
	MOV_Ientry           *pthisVid, *pthisAud;
	UINT64               lastjunkPos64 = 0;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	MOVRVCY_DUMP_MUST("[RECOVER]NidxDUAL  len = 0x%lx!\r\n", nidxlen);
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER]NidxDUAL MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	MOVRVCY_DUMP_MUST("[RECOVER]NidxDUAL pos = 0x%llx!\r\n", filepos);
	MOVRVCY_DUMP_MUST("[RECOVER]NidxDUAL  len = 0x%lx!\r\n", nidxlen);

	gMovRecvy_nidxNum += 1;
	size_oneE = MP_MOV_SIZE_CO64_ONEENTRY;//MP_MOV_SIZE_ONEENTRY;
	ptrNidxData = (MOV_2015_NIDXINFO *)memAddr;
	if (gMovRecvy_audcodec == 0) {
		gMovRecvy_audcodec = ptrNidxData->audcodec;
		MOVRVCY_DUMP_MUST("^G aud codec = %d\r\n", gMovRecvy_audcodec);
	}
	ptr_co64_NidxData = (MOV_2015_CO64_NIDXINFO *)memAddr;
	lastjunkPos64 = ((UINT64)(ptr_co64_NidxData->lastjunkPos1) << 32) + (ptr_co64_NidxData->lastjunkPos0);
	MOVRVCY_DUMP_MUST("321 pos = 0x%llx\r\n", lastjunkPos64);
	MOVRVCY_DUMP_MUST("320 pos = 0x%lx\r\n", ptrNidxData->lastjunkPos);
	MOVRVCY_DUMP_MUST("321 vs = 0x%lx\r\n", ptr_co64_NidxData->vidstartFN);
	MOVRVCY_DUMP_MUST("320 vs = 0x%lx\r\n", ptrNidxData->vidstartFN);
	MOVRVCY_DUMP_MUST("321 vsc = 0x%lx\r\n", ptr_co64_NidxData->versionCode);
	MOVRVCY_DUMP_MUST("320 vsc = 0x%lx\r\n", ptrNidxData->versionCode);
	if (ptr_co64_NidxData->versionCode == SMDUAL_RCVY_CO64_VERSION) {
		return MOVRcvy_DUAL_CheckNidx_co64(id, memAddr, filepos);
	}
	MOVRVCY_DUMP_MUST("321 ver = 0x%lx\r\n", ptr_co64_NidxData->versionCode);
	if ((ptrNidxData->versionCode != SMDUAL_RCVY_VERSION) && (ptrNidxData->versionCode != SMDUAL_RCVY_CO64_VERSION)) {
		DBG_ERR("cannot recover, version 0x%lx not Ver320!!\r\n", ptrNidxData->versionCode);
		return MOV_NOT_NIDX;
	}

	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		#if 1
		if (MOV_NOT_NIDX == mov_rcvy_check_data_blk(memAddr)) {
			DBG_ERR("[RECOVER]Nidx2015 MOV NO CLUR!!!!!\r\n");
			return MOV_NOT_NIDX;
		}
		#else
		DBG_ERR("[RECOVER]Nidx2015 MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
		#endif
	}

	DBGD(size_oneE);
	startAFN = ptrNidxData->audstartFN;
	vfr = ptrNidxData->videoFR;
	thisvfn =  ptrNidxData->vidtotalFN;
	MOVRVCY_DUMP_MUST("[RECOVER]SMGet FrameRate = %d!! \r\n", vfr);
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;
	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	MOVRVCY_DUMP_MUST("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	if (gMovRecvy_totalVnum == 0) { //only lastest is enough
		gMovRecvy_totalVnum = startVFN + thisvfn;
		MOVRVCY_DUMP_MUST("[RECOVER]SMGet totalVnum = %d!! \r\n", gMovRecvy_totalVnum);
	}

	// Audio Entry
	vfnshift = thisvfn * size_oneE + 8 + MP_MOV_2015NIDX_FIXEDSIZE; //0x40+ vfr*8
	MOVRVCY_DUMP_MUST("audpos=0x%lx!!\r\n", memAddr + vfnshift);
	ptr32 = (UINT32 *)(memAddr + vfnshift);
	thisafn = *ptr32;
	MOVRVCY_DUMP_MUST("startAFN=%d, thisafn = %d!!\r\n", startAFN, thisafn);
	MOVRVCY_DUMP_MUST("audpos=0x%lx!!\r\n", memAddr + vfnshift + 4);
	if (gMovRecvy_totalAnum == 0) { //only lastest is enough
		gMovRecvy_totalAnum = startAFN + thisafn;
	}
	pthisAud = (MOV_Ientry *)(memAddr + vfnshift + 4); // +=4 (audtotalnum)
	for (i = 0; i < thisafn; i++) {
		MOVRVCY_DUMP_MUST(" aud[%d] pos=0x%llx, size=0x%lx!\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
		if (pthisAud->size > 0x100000) { //2016/03/15
			DBG_ERR("aud[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetAudioIentry(id, startAFN + i, pthisAud);
		pthisAud++;
	}
	MOVRVCY_DUMP_MUST("audio entry OK1\r\n");
	//set Hdrsize
	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}

	// Video Entry
	pthisVid = (MOV_Ientry *)(memAddr + 8 + MP_MOV_2015NIDX_FIXEDSIZE); //junk+fixed
	for (i = 0; i < thisvfn; i++) {
		MOVRVCY_DUMP_MUST("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		if ((pthisVid->size > 0x300000) || (pthisVid->size == 0)) { //2016/04/13
			DBG_ERR("vid[%d] pos=0x%lx, size=0x%llx! MAYBE WRONG\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetIentry(id, startVFN + i, pthisVid);
		pthisVid++;
	}
	MOVRVCY_DUMP_MUST("video entry OK2\r\n");

	afnshift = thisafn * size_oneE + 4; //size(4 bytes) + entry*total

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
		MOVRVCY_DUMP_MUST("Rvcy: Pad h264 Desc\r\n");
	}
	MOVRVCY_DUMP_MUST("Rvcy: Next nidx Pos=0x%lx!\r\n", ptrNidxData->lastjunkPos);

	descshift = h264len + 4; //size(4 bytes) + desclen

	// Sub-stream Entry
	sub32 = (UINT32 *)(memAddr + vfnshift + afnshift + descshift);
	if (*sub32 == MOV_NIDX_CLUR) {
		// fake clur while sub vid zero
		descshift += 4;
		sub32 = (UINT32 *)(memAddr + vfnshift + afnshift + descshift);
	}
	startVFN = *sub32++;
	gMovRecFileInfo[id].SubImageWidth = *sub32++;
	gMovRecFileInfo[id].SubImageHeight = *sub32++;
	gMovRecFileInfo[id].SubencVideoCodec = *sub32++;
	vfr = *sub32++;
	if (vfr == 0) { // skip sub-stream
		return (ptrNidxData->lastjunkPos);
	}
	gMovRecFileInfo[id].SubVideoFrameRate = vfr;
	thisvfn = *sub32++;
	if (gMovRecvy_totalSVnum == 0) { //only lastest is enough
		gMovRecvy_totalSVnum = startVFN + thisvfn;
		MOVRVCY_DUMP_MUST("[RECOVER]SMGet totalSVnum = %d!! \r\n", gMovRecvy_totalSVnum);
		gMovRecFileInfo[id].SubVideoTrackEn = 1;
	}

	pthisVid = (MOV_Ientry *)sub32;
	for (i = 0; i < thisvfn; i++) {
		MOVRVCY_DUMP_MUST("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		if ((pthisVid->size > 0x300000) || (pthisVid->size == 0)) {
			DBG_ERR("vid[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			return MOV_NOT_NIDX;
		}
		MOV_Sub_SetIentry(id, startVFN + i, pthisVid);
		pthisVid++;
	}
	MOVRVCY_DUMP_MUST("sub video entry OK\r\n");

	subshift = thisvfn * size_oneE + 4 + 4 * 5; //size(4 bytes) + entry*total + subinfo

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift + descshift + subshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		MOV_Sub_Make264VidDesc(h264addr, h264len, 0);
		MOVRVCY_DUMP_MUST("^G Rvcy: Pad sub h264 Desc\r\n");
	}

	return (ptrNidxData->lastjunkPos);
}

static UINT64 MOVRcvy_DUAL_CheckNidx_co64(UINT32 id, UINT32 memAddr, UINT64 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_2015_CO64_NIDXINFO      *ptrNidxData;
	UINT32               startVFN, vfr = 0, nidxlen, size_oneE, tmp, vfnshift = 0;
	UINT32               startAFN, afnshift = 0, thisvfn, thisafn;
	UINT32               *sub32, descshift = 0, subshift = 0;
	UINT8                i;
	MOV_Ientry           *pthisVid, *pthisAud;
	UINT64               lastjunkPos64 = 0;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER]NidxDUAL_co64 MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	MOVRVCY_DUMP_MUST("[RECOVER]NidxDUAL pos = 0x%llx!\r\n", filepos);
	MOVRVCY_DUMP_MUST("[RECOVER]NidxDUAL  len = 0x%lx!\r\n", nidxlen);
	gMovRecvy_nidxNum += 1;
	ptrNidxData = (MOV_2015_CO64_NIDXINFO *)memAddr;
	lastjunkPos64 = ((UINT64)(ptrNidxData->lastjunkPos1) << 32) + (ptrNidxData->lastjunkPos0);

	if (ptrNidxData->versionCode != SMDUAL_RCVY_CO64_VERSION) {
		DBG_DUMP("^R NOT co64 version!!");
		return MOV_NOT_NIDX;
	}

	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		#if 1
		if (MOV_NOT_NIDX == mov_rcvy_check_data_blk(memAddr)) {
			DBG_ERR("[RECOVER]Nidx2015_co64 MOV NO CLUR!!!!!\r\n");
			return MOV_NOT_NIDX;
		}
		#else
		DBG_ERR("[RECOVER]Nidx2015_co64 MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
		#endif
	}

	{
		size_oneE = MP_MOV_SIZE_CO64_ONEENTRY;
	}
	//DBGD(size_oneE);
	startAFN = ptrNidxData->audstartFN;
	vfr = ptrNidxData->videoFR;
	thisvfn =  ptrNidxData->vidtotalFN;
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;
	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	MOVRVCY_DUMP_MUST("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	if (gMovRecvy_totalVnum == 0) { //only lastest is enough
		gMovRecvy_totalVnum = startVFN + thisvfn;
		MOVRVCY_DUMP_MUST("[RECOVER]SMGet totalVnum = %d!! \r\n", gMovRecvy_totalVnum);
	}

	vfnshift = thisvfn * size_oneE + 8 + MP_MOV_2015CO64NIDX_FIXEDSIZE; //0x44+ vfr*8
	MOVRVCY_DUMP_MUST("audpos=0x%lx!!\r\n", memAddr + vfnshift);

	ptr32 = (UINT32 *)(memAddr + vfnshift);
	thisafn = *ptr32;
	MOVRVCY_DUMP_MUST("startAFN=%d, thisafn = %d!!\r\n", startAFN, thisafn);
	MOVRVCY_DUMP_MUST("audpos=0x%lx!!\r\n", memAddr + vfnshift + 4);
	if (gMovRecvy_totalAnum == 0) { //only lastest is enough
		gMovRecvy_totalAnum = startAFN + thisafn;
	}

	pthisAud = (MOV_Ientry *)(memAddr + vfnshift + 4); // +=4 (audtotalnum)
	for (i = 0; i < thisafn; i++) {
		MOVRVCY_DUMP_MUST("aud[%d] pos=0x%llx, size=0x%lx!\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
		if (pthisAud->size > 0x100000) { //2016/03/15
			DBG_ERR("aud[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetAudioIentry(id, startAFN + i, pthisAud);
		pthisAud++;
	}
	MOVRVCY_DUMP_MUST("audio entry OK1\r\n");
	//set Hdrsize
	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}
	pthisVid = (MOV_Ientry *)(memAddr + 8 + MP_MOV_2015CO64NIDX_FIXEDSIZE); //junk+fixed
	for (i = 0; i < thisvfn; i++) {
		MOVRVCY_DUMP_MUST("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		if ((pthisVid->size > 0x300000) || (pthisVid->size == 0)) { //2016/04/13
			DBG_ERR("vid[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetIentry(id, startVFN + i, pthisVid);
		pthisVid++;
	}
	MOVRVCY_DUMP_MUST("video entry OK2\r\n");

	afnshift = thisafn * size_oneE + 4; //size(4 bytes) + entry*total

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		if (gMovRecFileInfo[id].encVideoCodec == MEDIAVIDENC_H265) {
			g_movh265len[id] = MovWriteLib_Make265VidDesc(h264addr, h264len, 0);
			MOVRVCY_DUMP_MUST("^G Rvcy: Pad h265 Desc\r\n");
		} else {
			MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
			MOVRVCY_DUMP_MUST("^G Rvcy: Pad h264 Desc\r\n");
		}
	}
	MOVRVCY_DUMP_MUST("Rvcy: Next nidx Pos=0x%llx!\r\n", lastjunkPos64);

	descshift = h264len + 4; //size(4 bytes) + desclen

	// sub-stream
	sub32 = (UINT32 *)(memAddr + vfnshift + afnshift + descshift);
	if (*sub32 == MOV_NIDX_CLUR) {
		// fake clur while sub vid zero
		descshift += 4;
		sub32 = (UINT32 *)(memAddr + vfnshift + afnshift + descshift);
	}
	startVFN = *sub32++;
	gMovRecFileInfo[id].SubImageWidth = *sub32++;
	gMovRecFileInfo[id].SubImageHeight = *sub32++;
	gMovRecFileInfo[id].SubencVideoCodec = *sub32++;
	vfr = *sub32++;
	if (vfr == 0) { // skip sub-stream
		return (lastjunkPos64);
	}
	gMovRecFileInfo[id].SubVideoFrameRate = vfr;
	thisvfn = *sub32++;
	if (gMovRecvy_totalSVnum == 0) { //only lastest is enough
		gMovRecvy_totalSVnum = startVFN + thisvfn;
		MOVRVCY_DUMP_MUST("[RECOVER]SMGet totalSVnum = %d!! \r\n", gMovRecvy_totalSVnum);
		gMovRecFileInfo[id].SubVideoTrackEn = 1;
	}

	pthisVid = (MOV_Ientry *)sub32;
	for (i = 0; i < thisvfn; i++) {
		MOVRVCY_DUMP_MUST("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		if ((pthisVid->size > 0x300000) || (pthisVid->size == 0)) { //2016/04/13
			DBG_ERR("vid[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			return MOV_NOT_NIDX;
		}
		MOV_Sub_SetIentry(id, startVFN + i, pthisVid);
		pthisVid++;
	}
	MOVRVCY_DUMP_MUST("sub video entry OK\r\n");

	subshift = thisvfn * size_oneE + 4 + 4 * 5; //size(4 bytes) + entry*total + subinfo

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift + descshift + subshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		if (gMovRecFileInfo[id].SubencVideoCodec == MEDIAVIDENC_H265) {
			g_sub_movh265len[id] = MOV_Sub_Make265VidDesc(h264addr, h264len, 0);
			MOVRVCY_DUMP_MUST("^G Rvcy: Pad sub h265 Desc\r\n");
		} else {
			MOV_Sub_Make264VidDesc(h264addr, h264len, 0);
			MOVRVCY_DUMP_MUST("^G Rvcy: Pad sub h264 Desc\r\n");
		}
	}

	return (lastjunkPos64);
}

//#NT#2012/09/11#Meg Lin -begin
static UINT32 MOVRcvy_CheckNidx(UINT32 id, UINT32 memAddr, UINT32 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_NIDXINFO         *ptrNidxData;
	UINT32               startVFN, vfr = 0, nidxlen, size_oneE, tmp;
	UINT8                i;
	MOV_old32_Ientry           *pthisVid;
	MOV_old32_Ientry           thisAud;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("[RECOVER]Nidx  len = 0x%lx!\r\n", nidxlen);
	}
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER] MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		DBG_ERR("[RECOVER] MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("[RECOVER]Nidx pos = 0x%lx!\r\n", filepos);
		DBG_DUMP("[RECOVER]Nidx  len = 0x%lx!\r\n", nidxlen);
	}
	gMovRecvy_nidxNum += 1;
	size_oneE = MP_MOV_SIZE_ONEENTRY;
	ptrNidxData = (MOV_NIDXINFO *)memAddr;
	vfr = (ptrNidxData->vidPosLen) / size_oneE;
	if (vfr == 0) {
		DBG_ERR("[RECOVER]Get FrameRate = %d error!! \r\n", vfr);
		return MOV_NOT_NIDX;
	}
	{
		DBG_IND("[RECOVER]Get FrameRate = %d!! \r\n", vfr);
	}
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;

	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	}
	if (gMovRecvy_totalVnum == 0) {
		gMovRecvy_totalVnum = startVFN + vfr;
	}

	// Audio Entry
	thisAud.pos = ptrNidxData->audPos;
	thisAud.size = ptrNidxData->audSize;
	thisAud.key_frame = 0;//coverity 69940
	thisAud.updated = 0;//coverity 69940
	thisAud.rev = 0; //coverity 69940
	gMovRecvy_audTotalSize += thisAud.size;
	if (gMovRvcyMsg) {

		DBG_DUMP("     audio[%d] pos=0x%lx, size=0x%lx!\r\n", startVFN / vfr, thisAud.pos, thisAud.size);
	}
	//DBG_DUMP("id=%d %ld!!\r\n", id, g_movAudioEntryMax[id]);

	MOV_SetAudioIentry_old32(0, startVFN / vfr, &thisAud);

	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}
	if (ptrNidxData->versionCode != MEDIARECVY_VERSION) {
		DBG_ERR("cannot recover, version code NOT match 0x%lx!!\r\n", ptrNidxData->versionCode);
		return MOV_NOT_NIDX;
	}

	pthisVid = (MOV_old32_Ientry *)(memAddr + MP_MOV_NIDX_FIXEDSIZE + 4); //junk+fixed
	for (i = 0; i < vfr; i++) {
		if (gMovRvcyMsg) {

			DBG_DUMP("    video[%d] pos=0x%lx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		}
		//if (pthisVid->pos == 0)
		if (0) {
			DBG_DUMP("    video[%d] pos=0x%lx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			DBG_ERR("something wrong! cannot recover\r\n");
			return MOV_NOT_NIDX;
		}
		MOV_SetIentry_old32(id, startVFN + i, pthisVid);

		pthisVid++;
	}
	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)pthisVid;
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
		if (gMovRvcyMsg) {
			DBG_DUMP("Rvcy: Pad h264 Desc\r\n");
		}
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("Rvcy: Next nidx Pos=0x%lx!\r\n", ptrNidxData->lastjunkPos);
	}
	return (ptrNidxData->lastjunkPos);

}
//#NT#2012/09/11#Meg Lin -end

static UINT64 MOVRcvy_SM_CheckNidx(UINT32 id, UINT32 memAddr, UINT64 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_SM_NIDXINFO      *ptrNidxData;
	MOV_SM_CO64_NIDXINFO *ptrNidxData64;
	UINT32               startVFN, vfr = 0, nidxlen, tmp, vfnshift = 0;
	UINT32               startAFN, afnshift = 0, thisvfn, thisafn;
	UINT8                i;
	MOV_Ientry           *pthisVid, *pthisAud;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("[RECOVER]NidxSM  len = 0x%lx!\r\n", nidxlen);
	}
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER]NidxSM MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("[RECOVER]NidxSM pos = 0x%llx!\r\n", filepos);
		DBG_DUMP("[RECOVER]NidxSM  len = 0x%lx!\r\n", nidxlen);
	}
	//DBG_DUMP("id=%d %ld!!\r\n", id,  g_movAudioEntryMax[id]);

	gMovRecvy_nidxNum += 1;
	//size_oneE = MP_MOV_SIZE_ONEENTRY; // Not used #2017/05/26
	ptrNidxData = (MOV_SM_NIDXINFO *)memAddr;
	ptrNidxData64 = (MOV_SM_CO64_NIDXINFO *)memAddr;
	//lastjunkPos64 = ((UINT64)(ptrNidxData64->lastjunkPos1) << 32)+(ptrNidxData64->lastjunkPos0); // Not used #2017/05/26
	//DBG_DUMP("302 pos = 0x%llx\r\n",lastjunkPos64);
	//DBG_DUMP("301 pos = 0x%lx\r\n",ptrNidxData->lastjunkPos);
	//DBG_DUMP("302 vs startFN = 0x%lx\r\n",ptrNidxData64->vidstartFN);
	//DBG_DUMP("301 vs startFN = 0x%lx\r\n",ptrNidxData->vidstartFN);
	//DBG_DUMP("302 version = 0x%lx\r\n",ptrNidxData64->versionCode);
	//DBG_DUMP("301 version = 0x%lx\r\n",ptrNidxData->versionCode);

	if (ptrNidxData64->versionCode == SMEDIARECVY_CO64_VERSION) {
		return MOVRcvy_SM_CheckNidx_co64(id, memAddr, filepos);
	}
	if (ptrNidxData->versionCode != SMEDIARECVY_VERSION) {
		DBG_ERR("cannot recover, version 0x%lx not Ver301!!\r\n", ptrNidxData->versionCode);
		return MOV_NOT_NIDX;
	}

	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		DBG_ERR("[RECOVER]NidxSM MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
	}

	startAFN = ptrNidxData->audstartFN;
	vfr = ptrNidxData->videoFR;
	if (vfr == 0) {
		DBG_ERR("[RECOVER]SMGet FrameRate = %d error!! \r\n", vfr);
		return MOV_NOT_NIDX;
	}
	thisvfn =  ptrNidxData->vidtotalFN;
	if (gMovRvcyMsg) {
		DBG_DUMP("[RECOVER]SMGet FrameRate = %d!! \r\n", vfr);
	}
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;

	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	}
	if (gMovRecvy_totalVnum == 0) { //only lastest is enough
		//gMovRecvy_totalVnum = startVFN + vfr;
		gMovRecvy_totalVnum = startVFN + thisvfn;//2015/11/09
		if (gMovRvcyMsg) {
			DBG_DUMP("[RECOVER]SMGet totalVnum = %d!! \r\n", gMovRecvy_totalVnum);
		}
	}

	// Audio Entry
#if 0
	thisAud.pos = ptrNidxData->audPos;
	thisAud.size = ptrNidxData->audSize;
	gMovRecvy_audTotalSize += thisAud.size;
	if (gMovRvcyMsg) {

		DBG_DUMP("     audio[%d] pos=0x%lx, size=0x%lx!\r\n", startVFN / vfr, thisAud.pos, thisAud.size);
	}
	MOV_SetAudioIentry(0, startVFN / vfr, &thisAud);
#else
	//audfn = ptrNidxData->audtotalFN;
	vfnshift = thisvfn * MP_MOV_SIZE_ONEENTRY + 8 + MP_MOV_NIDX_FIXEDSIZE; //0x40+ vfr*8
	if (gMovRvcyMsg) {
		DBG_DUMP("audpos=0x%lx!!\r\n", memAddr + vfnshift);
	}
	//return MOV_NOT_NIDX;

	ptr32 = (UINT32 *)(memAddr + vfnshift);
	thisafn = *ptr32;
	if (gMovRvcyMsg) {

		DBG_DUMP("startAFN=%d, thisafn = %d!!\r\n", startAFN, thisafn);
		DBG_DUMP("audpos=0x%lx!!\r\n", memAddr + vfnshift + 4);
		//return MOV_NOT_NIDX;
	}
	if (gMovRecvy_totalAnum == 0) { //only lastest is enough
		gMovRecvy_totalAnum = startAFN + thisafn;
	}

	pthisAud = (MOV_Ientry *)(memAddr + vfnshift + 4); // +=4 (audtotalnum)
	for (i = 0; i < thisafn; i++) {
		if (gMovRvcyMsg) {

			DBG_DUMP("aud[%d] pos=0x%llx, size=0x%lx!\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
		}
		MOV_SetAudioIentry(id, startAFN + i, pthisAud);
		if (pthisAud->pos == 0) { //2016/01/08
			DBG_DUMP("    audio[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisAud->pos, pthisAud->size);
			DBG_ERR("something wrong! cannot recover\r\n");
			return MOV_NOT_NIDX;
		}

		pthisAud++;
	}
#endif
	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK1\r\n");
	}
	//set Hdrsize
	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}

	pthisVid = (MOV_Ientry *)(memAddr + 8 + MP_MOV_NIDX_FIXEDSIZE); //junk+fixed
	for (i = 0; i < thisvfn; i++) {
		if (gMovRvcyMsg) {

			DBG_DUMP("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		}
		MOV_SetIentry(id, startVFN + i, pthisVid);
		if (pthisVid->pos == 0) { //2016/01/08
			DBG_DUMP("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			DBG_ERR("something wrong! cannot recover\r\n");
			return MOV_NOT_NIDX;
		}

		pthisVid++;
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK2\r\n");
	}

	afnshift = thisafn * MP_MOV_SIZE_ONEENTRY + 4; //size(4 bytes) + entry*total

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
		if (gMovRvcyMsg) {
			DBG_DUMP("Rvcy: Pad h264 Desc\r\n");
		}
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("Rvcy: Next nidx Pos=0x%lx!\r\n", ptrNidxData->lastjunkPos);
	}
	return (ptrNidxData->lastjunkPos);

}

static UINT64 MOVRcvy_SM_CheckNidx_co64(UINT32 id, UINT32 memAddr, UINT64 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_SM_CO64_NIDXINFO      *ptrNidxData;
	UINT32               startVFN, vfr = 0, nidxlen, size_oneE, tmp, vfnshift = 0;
	UINT32               startAFN, afnshift = 0, thisvfn, thisafn;
	UINT8                i;
	//UINT8                *ptr8;
	MOV_Ientry           *pthisVid, *pthisAud;
	UINT64               lastjunkPos;
	UINT8  show = 1;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	if (0) { //gMovRvcyMsg)
		DBG_DUMP("[RECOVER]NidxSMco64  len = 0x%lx!\r\n", nidxlen);
	}
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER]NidxSM MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("[RECOVER]NidxSMco64 pos = 0x%llx!\r\n", filepos);
		DBG_DUMP("[RECOVER]NidxSMco64  len = 0x%lx!\r\n", nidxlen);
	}

	gMovRecvy_nidxNum += 1;
	size_oneE = MP_MOV_SIZE_CO64_ONEENTRY;

	ptrNidxData = (MOV_SM_CO64_NIDXINFO *)memAddr;
	lastjunkPos = ((UINT64)(ptrNidxData->lastjunkPos1) << 32) + (ptrNidxData->lastjunkPos0);
	if (ptrNidxData->versionCode != SMEDIARECVY_CO64_VERSION) {
		DBG_ERR("cannot recover, version 0x%lx not Ver302!!\r\n", ptrNidxData->versionCode);
		return MOV_NOT_NIDX;
	}

	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		DBG_ERR("[RECOVER]NidxSMco64 MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
	}

	startAFN = ptrNidxData->audstartFN;
	vfr = ptrNidxData->videoFR;
	thisvfn =  ptrNidxData->vidtotalFN;
	if (0) { //gMovRvcyMsg)
		DBG_DUMP("[RECOVER]SMGet FrameRate = %d!! \r\n", vfr);
	}
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;

	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	}
	if (gMovRecvy_totalVnum == 0) { //only lastest is enough
		//gMovRecvy_totalVnum = startVFN + vfr;
		gMovRecvy_totalVnum = startVFN + thisvfn;//2015/11/09
		if (gMovRvcyMsg) {
			DBG_DUMP("[RECOVER]SMGet totalVnum = %d!! \r\n", gMovRecvy_totalVnum);
		}
	}

	//audfn = ptrNidxData->audtotalFN;
	vfnshift = thisvfn * size_oneE + 8 + MP_MOV_NIDXCO64_FIXEDSIZE; //0x48+ vfr*8
	if (gMovRvcyMsg) {
		DBG_DUMP("audpos=0x%lx!!\r\n", vfnshift);
	}
	//return MOV_NOT_NIDX;

	ptr32 = (UINT32 *)(memAddr + vfnshift);
	thisafn = *ptr32;
	if (gMovRvcyMsg) {

		DBG_DUMP("startAFN=%d, thisafn = %d!!\r\n", startAFN, thisafn);
		DBG_DUMP("audpos=0x%lx!!\r\n", vfnshift + 4);
		//return MOV_NOT_NIDX;
	}
	if (gMovRecvy_totalAnum == 0) { //only lastest is enough
		gMovRecvy_totalAnum = startAFN + thisafn;
	}

	pthisAud = (MOV_Ientry *)(memAddr + vfnshift + 4); // +=4 (audtotalnum)
	for (i = 0; i < thisafn; i++) {
		if ((gMovRvcyMsg) && (show)) {
			if (show) {
				show--;
			}
			DBG_DUMP("aud[%d] pos=0x%llx, size=0x%lx!\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
		}
		MOV_SetAudioIentry(id, startAFN + i, pthisAud);
		if (pthisAud->pos == 0) { //2016/01/08
			DBG_DUMP("    audio[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisAud->pos, pthisAud->size);
			DBG_ERR("something wrong! cannot recover\r\n");
			return MOV_NOT_NIDX;
		}

		pthisAud++;
	}

	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK1\r\n");
	}
	//set Hdrsize
	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}
	show = 1;
	pthisVid = (MOV_Ientry *)(memAddr + 8 + MP_MOV_NIDXCO64_FIXEDSIZE); //junk+fixed
	for (i = 0; i < thisvfn; i++) {
		if ((gMovRvcyMsg) && (show)) {
			if (show) {
				show--;
			}
			DBG_DUMP("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		}
		MOV_SetIentry(id, startVFN + i, pthisVid);
		if (pthisVid->pos == 0) { //2016/01/08
			DBG_DUMP("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			DBG_ERR("something wrong! cannot recover\r\n");
			return MOV_NOT_NIDX;
		}

		pthisVid++;
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK2\r\n");
	}

	afnshift = thisafn * size_oneE + 4; //size(4 bytes) + entry*total

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
		if (gMovRvcyMsg) {
			DBG_DUMP("Rvcy: Pad h264 Desc\r\n");
		}
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("Rvcyco64: Next nidx Pos=0x%llx!\r\n", lastjunkPos);
	}
	return (lastjunkPos);

}
static UINT64 MOVRcvy_2015_CheckNidx(UINT32 id, UINT32 memAddr, UINT64 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_2015_NIDXINFO      *ptrNidxData;
	MOV_2015_CO64_NIDXINFO      *ptr_co64_NidxData;
	UINT32               startVFN, vfr = 0, nidxlen, size_oneE, tmp, vfnshift = 0;
	UINT32               startAFN, afnshift = 0, thisvfn, thisafn;
	UINT8                i;
	//UINT8                *ptr8;
	MOV_old32_Ientry           *pthisVid, *pthisAud;
	UINT64               lastjunkPos64 = 0;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("[RECOVER]Nidx2015  len = 0x%lx!\r\n", nidxlen);
	}
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER]Nidx2015 MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("[RECOVER]NidxSM pos = 0x%llx!\r\n", filepos);
		DBG_DUMP("[RECOVER]NidxSM  len = 0x%lx!\r\n", nidxlen);
	}


	gMovRecvy_nidxNum += 1;
	size_oneE = MP_MOV_SIZE_ONEENTRY;
	ptrNidxData = (MOV_2015_NIDXINFO *)memAddr;
	if (gMovRecvy_audcodec == 0) {
		gMovRecvy_audcodec = ptrNidxData->audcodec;
		MOVRVCY_DUMP_MUST("^G aud codec = %d\r\n", gMovRecvy_audcodec);

	}
	ptr_co64_NidxData = (MOV_2015_CO64_NIDXINFO *)memAddr;
	lastjunkPos64 = ((UINT64)(ptr_co64_NidxData->lastjunkPos1) << 32) + (ptr_co64_NidxData->lastjunkPos0);
	MOVRVCY_DUMP_MUST("316 pos = 0x%llx\r\n", lastjunkPos64);
	MOVRVCY_DUMP_MUST("315 pos = 0x%lx\r\n", ptrNidxData->lastjunkPos);
	MOVRVCY_DUMP_MUST("316 vs = 0x%lx\r\n", ptr_co64_NidxData->vidstartFN);
	MOVRVCY_DUMP_MUST("315 vs = 0x%lx\r\n", ptrNidxData->vidstartFN);
	MOVRVCY_DUMP_MUST("316 vsc = 0x%lx\r\n", ptr_co64_NidxData->versionCode);
	MOVRVCY_DUMP_MUST("315 vsc = 0x%lx\r\n", ptrNidxData->versionCode);
	if (ptr_co64_NidxData->versionCode == SM2015_RCVY_CO64_VERSION) {
		return MOVRcvy_2015_CheckNidx_co64(id, memAddr, filepos);
	}
	MOVRVCY_DUMP_MUST("316 ver = 0x%lx\r\n", ptr_co64_NidxData->versionCode);
	if ((ptrNidxData->versionCode != SM2015_RCVY_VERSION) && (ptrNidxData->versionCode != SM2015_RCVY_CO64_VERSION)) {
		DBG_ERR("cannot recover, version 0x%lx not Ver315!!\r\n", ptrNidxData->versionCode);
		return MOV_NOT_NIDX;
	}

	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		DBG_ERR("[RECOVER]Nidx2015 MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
	}

	DBGD(size_oneE);
	startAFN = ptrNidxData->audstartFN;
	vfr = ptrNidxData->videoFR;
	thisvfn =  ptrNidxData->vidtotalFN;
	if (gMovRvcyMsg) {
		DBG_DUMP("[RECOVER]SMGet FrameRate = %d!! \r\n", vfr);
	}
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;

	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	}
	if (gMovRecvy_totalVnum == 0) { //only lastest is enough
		//gMovRecvy_totalVnum = startVFN + vfr;
		gMovRecvy_totalVnum = startVFN + thisvfn;//2015/11/09
		if (gMovRvcyMsg) {
			DBG_DUMP("[RECOVER]SMGet totalVnum = %d!! \r\n", gMovRecvy_totalVnum);
		}
	}

	// Audio Entry
#if 0
	thisAud.pos = ptrNidxData->audPos;
	thisAud.size = ptrNidxData->audSize;
	gMovRecvy_audTotalSize += thisAud.size;
	if (gMovRvcyMsg) {

		DBG_DUMP("     audio[%d] pos=0x%lx, size=0x%lx!\r\n", startVFN / vfr, thisAud.pos, thisAud.size);
	}
	MOV_SetAudioIentry(0, startVFN / vfr, &thisAud);
#else
	//audfn = ptrNidxData->audtotalFN;
	vfnshift = thisvfn * size_oneE + 8 + MP_MOV_2015NIDX_FIXEDSIZE; //0x40+ vfr*8
	if (gMovRvcyMsg) {
		DBG_DUMP("audpos=0x%lx!!\r\n", memAddr + vfnshift);
	}
	//return MOV_NOT_NIDX;

	ptr32 = (UINT32 *)(memAddr + vfnshift);
	thisafn = *ptr32;
	if (gMovRvcyMsg) {

		DBG_DUMP("startAFN=%d, thisafn = %d!!\r\n", startAFN, thisafn);
		DBG_DUMP("audpos=0x%lx!!\r\n", memAddr + vfnshift + 4);
		//return MOV_NOT_NIDX;
	}
	if (gMovRecvy_totalAnum == 0) { //only lastest is enough
		gMovRecvy_totalAnum = startAFN + thisafn;
	}

	pthisAud = (MOV_old32_Ientry *)(memAddr + vfnshift + 4); // +=4 (audtotalnum)
	for (i = 0; i < thisafn; i++) {
		if (gMovRvcyMsg) {

			DBG_DUMP("aud[%d] pos=0x%lx, size=0x%lx!\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
		}
		if (pthisAud->size > 0x100000) { //2016/03/15
			DBG_ERR("aud[%d] pos=0x%lx, size=0x%lx! MAYBE WRONG\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetAudioIentry_old32(id, startAFN + i, pthisAud);

		pthisAud++;
	}
#endif
	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK1\r\n");
	}
	//set Hdrsize
	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}

	pthisVid = (MOV_old32_Ientry *)(memAddr + 8 + MP_MOV_2015NIDX_FIXEDSIZE); //junk+fixed
	for (i = 0; i < thisvfn; i++) {
		if (gMovRvcyMsg) {

			DBG_DUMP("    video[%d] pos=0x%lx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		}
		if ((pthisVid->size > 0x300000) || (pthisVid->size == 0)) { //2016/04/13
			DBG_ERR("vid[%d] pos=0x%lx, size=0x%lx! MAYBE WRONG\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetIentry_old32(id, startVFN + i, pthisVid);

		pthisVid++;
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK2\r\n");
	}

	afnshift = thisafn * size_oneE + 4; //size(4 bytes) + entry*total

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
		if (gMovRvcyMsg) {
			DBG_DUMP("Rvcy: Pad h264 Desc\r\n");
		}
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("Rvcy: Next nidx Pos=0x%lx!\r\n", ptrNidxData->lastjunkPos);
	}
	return (ptrNidxData->lastjunkPos);

}

static UINT64 MOVRcvy_2015_CheckNidx_co64(UINT32 id, UINT32 memAddr, UINT64 filepos)
{
	UINT32               *ptr32, *ptrh264, h264len, h264addr;
	MOV_2015_CO64_NIDXINFO      *ptrNidxData;
	UINT32               startVFN, vfr = 0, nidxlen, size_oneE, tmp, vfnshift = 0;
	UINT32               startAFN, afnshift = 0, thisvfn, thisafn;
	UINT8                i, show = 1;
	//UINT8                *ptr8;
	MOV_Ientry           *pthisVid, *pthisAud;
	UINT64               lastjunkPos64 = 0;
	UINT32               *ptr32end, clursize;

	ptr32 = (UINT32 *)memAddr;
	tmp = *ptr32;
	clursize = Mov_Swap(tmp);
	if (gMovRvcyMsg) {
		DBG_DUMP("^G clustersize=0x%x\r\n", clursize);
	}
	ptr32 += 2;//len + free + len + nidx
	tmp = *ptr32;
	nidxlen = Mov_Swap(tmp);
	if (0) { //gMovRvcyMsg)
		DBG_DUMP("[RECOVER]Nidx2015  len = 0x%lx!\r\n", nidxlen);
	}
	ptr32 += 1;
	if (*ptr32 != MOV_NIDX) {
		DBG_ERR("[RECOVER]Nidx2015_co64 MOV NO NIDX!!!!!\r\n");
		return MOV_NOT_NIDX;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("[RECOVER]NidxSM pos = 0x%llx!\r\n", filepos);
		DBG_DUMP("[RECOVER]NidxSM  len = 0x%lx!\r\n", nidxlen);
	}
	gMovRecvy_nidxNum += 1;
	//coverity size_oneE = MP_MOV_SIZE_ONEENTRY;
	ptrNidxData = (MOV_2015_CO64_NIDXINFO *)memAddr;
	lastjunkPos64 = ((UINT64)(ptrNidxData->lastjunkPos1) << 32) + (ptrNidxData->lastjunkPos0);
	//DBG_DUMP("316 pos = 0x%llx\r\n",lastjunkPos64);

	if (ptrNidxData->versionCode != SM2015_RCVY_CO64_VERSION) {
		DBG_DUMP("^R NOT co64 version!!");
		return MOV_NOT_NIDX;
	}

	ptr32end = (UINT32 *)(memAddr + clursize); //nidx end
	ptr32end -= 1;
	if (*ptr32end != MOV_NIDX_CLUR) {
		DBG_ERR("[RECOVER]Nidx2015_co64 MOV NO CLUR!!!!!\r\n");
		return MOV_NOT_NIDX;
	}

	{
		size_oneE = MP_MOV_SIZE_CO64_ONEENTRY;
	}
	//DBGD(size_oneE);
	startAFN = ptrNidxData->audstartFN;
	vfr = ptrNidxData->videoFR;
	thisvfn =  ptrNidxData->vidtotalFN;
	if (0) { //gMovRvcyMsg)
		DBG_DUMP("[RECOVER]SMGet FrameRate = %d!! \r\n", vfr);
	}
	gMovRecvy_vfr = vfr;
	startVFN = ptrNidxData->vidstartFN;

	if (gMovRecvy_vidWid == 0) { //getting once is enough
		gMovRecvy_vidWid   = ptrNidxData->videoWid;
		gMovRecFileInfo[id].ImageWidth = gMovRecvy_vidWid;
		gMovRecFileInfo[id].ImageHeight = ptrNidxData->videoHei;
		gMovRecFileInfo[id].encVideoCodec = ptrNidxData->videoCodec;
		gMovRecFileInfo[id].AudioSampleRate = ptrNidxData->audSR;
		gMovRecFileInfo[id].AudioChannel = ptrNidxData->audChs;
	}
	if (gMovRvcyMsg) {

		DBG_DUMP("startVFN=%d, vfr = %d!!\r\n", startVFN, vfr);
	}
	if (gMovRecvy_totalVnum == 0) { //only lastest is enough
		//gMovRecvy_totalVnum = startVFN + vfr;
		gMovRecvy_totalVnum = startVFN + thisvfn;//2015/11/09
		if (gMovRvcyMsg) {
			DBG_DUMP("[RECOVER]SMGet totalVnum = %d!! \r\n", gMovRecvy_totalVnum);
		}
	}


	//audfn = ptrNidxData->audtotalFN;
	vfnshift = thisvfn * size_oneE + 8 + MP_MOV_2015CO64NIDX_FIXEDSIZE; //0x44+ vfr*8
	if (gMovRvcyMsg) {
		DBG_DUMP("audpos=0x%lx!!\r\n", memAddr + vfnshift);
	}
	//return MOV_NOT_NIDX;

	ptr32 = (UINT32 *)(memAddr + vfnshift);
	thisafn = *ptr32;
	if (gMovRvcyMsg) {

		DBG_DUMP("startAFN=%d, thisafn = %d!!\r\n", startAFN, thisafn);
		DBG_DUMP("audpos=0x%lx!!\r\n", memAddr + vfnshift + 4);
		//return MOV_NOT_NIDX;
	}
	if (gMovRecvy_totalAnum == 0) { //only lastest is enough
		gMovRecvy_totalAnum = startAFN + thisafn;
	}

	pthisAud = (MOV_Ientry *)(memAddr + vfnshift + 4); // +=4 (audtotalnum)
	for (i = 0; i < thisafn; i++) {
		if ((gMovRvcyMsg) && (show)) {
			if (show) {
				show--;
			}

			DBG_DUMP("aud[%d] pos=0x%llx, size=0x%lx!\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
		}
		if (pthisAud->size > 0x100000) { //2016/03/15
			DBG_ERR("aud[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startAFN + i, pthisAud->pos, pthisAud->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetAudioIentry(id, startAFN + i, pthisAud);

		pthisAud++;
	}

	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK1\r\n");
	}
	//set Hdrsize
	if (gMovRcvy_Hdrsize == 0) {
		gMovRcvy_Hdrsize = ptrNidxData->hdrSize;
	}
	show = 1;
	pthisVid = (MOV_Ientry *)(memAddr + 8 + MP_MOV_2015CO64NIDX_FIXEDSIZE); //junk+fixed
	for (i = 0; i < thisvfn; i++) {
		if ((gMovRvcyMsg) && (show)) {
			if (show) {
				show--;
			}

			DBG_DUMP("    video[%d] pos=0x%llx, size=0x%lx!\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
		}
		if ((pthisVid->size > 0x300000) || (pthisVid->size == 0)) { //2016/04/13
			DBG_ERR("vid[%d] pos=0x%llx, size=0x%lx! MAYBE WRONG\r\n", startVFN + i, pthisVid->pos, pthisVid->size);
			return MOV_NOT_NIDX;
		}
		MOV_SetIentry(id, startVFN + i, pthisVid);

		pthisVid++;
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("audio entry OK2\r\n");
	}

	afnshift = thisafn * size_oneE + 4; //size(4 bytes) + entry*total

	//final pthisVid->pos = h264descLen
	ptrh264 = (UINT32 *)(memAddr + vfnshift + afnshift);
	h264len = *ptrh264++;
	h264addr = (UINT32)ptrh264;
	if (h264len) { //ptrNidxData->h264descLen
		if (h264len > MEDIAWRITE_H265_STSD_MAXLEN) {
			DBG_ERR("h264len =0x%lx! something error!!\r\n", h264len);
			return MOV_NOT_NIDX;
		}
		if (gMovRecFileInfo[id].encVideoCodec == MEDIAVIDENC_H265) {

			g_movh265len[id] = MovWriteLib_Make265VidDesc(h264addr, h264len, 0);
		    if (gMovRvcyMsg) {
			    DBG_DUMP("^G Rvcy: Pad h265 Desc\r\n");
		    }
		} else {
			MovWriteLib_Make264VidDesc(h264addr, h264len, 0);
		    if (gMovRvcyMsg) {
			    DBG_DUMP("^G Rvcy: Pad h264 Desc\r\n");
		    }
		}
	}
	if (gMovRvcyMsg) {
		DBG_DUMP("Rvcy: Next nidx Pos=0x%llx!\r\n", lastjunkPos64);
	}
	return (lastjunkPos64);

}
ER MOVRcvy_GetNidxTbl(UINT32 id, UINT64 filesize64)
{
	UINT32 size, newnidx = 0;
	UINT32 nextNidxPos;
	INT32  ret;
	UINT32 *ptr;
	UINT32 *ptimeID, timeID;
	UINT64 realpos = 0;
	UINT64 pos64 = 0, blksize, newnidxpos = 0;
	UINT64 nextNidxPos64 = 0;
	UINT64 filesize64_align4 = 0, readsize;
	ER     rev = E_NOSPT;
	//DBG_DUMP("^G 22totalfilesize %llx!\r\n", filesize64);

	//read last cluster
	size = gMovRecFileInfo[id].clustersize;
	#if 0
	blksize = size;
	//#NT#2012/09/12#Meg Lin -begin
	//find cluster size when recording
	pos64 =  filesize64 - 8;//last 4 byte
	ret = (gMovContainerMaker.cbReadFile)(0, pos64, 8, gMovRecvy_CluReadAddr);
	if (ret != E_OK) {
		DBG_ERR("[Recover] Read file error 0x%lx!\r\n", ret);
		return E_SYS;
	}

	ptr = (UINT32 *)(gMovRecvy_CluReadAddr + 4);
	if (*ptr == MOV_NIDX_CLUR) {
		ptr = (UINT32 *)gMovRecvy_CluReadAddr;
		size = *ptr;
		DBG_ERR("[Recover] MOV new cluster size = 0x%lx!\r\n", size);
		newnidx = 0x101;//old version
	} else
	#endif
	{
		blksize = MOV_MEDIARECVRY_MAXBLKSIZE;
		DBG_ERR("[Recover] MOV using now cluster size = 0x%llx!\r\n", blksize);
		if (filesize64 < blksize) { //2015/04/02
			blksize = filesize64;
		}

		//check if SM2015_RCVY_VERSION 0x315
		ret = (gMovContainerMaker.cbReadFile)(0, 0, 0x40, gMovRecvy_LastBlockAddr);
		if (ret != E_OK) { //2017/03/31
			DBG_DUMP("cbReadFile error!!\r\n");
			return E_SYS;
		}
		newnidxpos = MOVRcvy_SearchSpecificCC(MOV_TIMA, gMovRecvy_LastBlockAddr, 0x40);
		if (newnidxpos != 0) {
			ptimeID = (UINT32 *)(gMovRecvy_LastBlockAddr + (UINT32)newnidxpos + 4);
			MOVRVCY_DUMP_MUST("^R TimeID = 0x%lx\r\n", *ptimeID);
			if (*ptimeID != 0) {
				newnidx = SM2015_RCVY_VERSION; //0x315;
				timeID = *ptimeID;
				MOVRVCY_DUMP_MUST("^G 11totalfilesize %llx!\r\n", filesize64);

				realpos = MOVRcvy_SearchFinalNidxWithTimerID_2(timeID, filesize64);
				if (realpos == 0) {
					//#NT#2020/07/22#Willy Su -begin
					//#NT# Rescan nidx for timeID not match issue
					#if 1
					DBG_DUMP("^R NO timeID (0x%lx)! go searching nidx\r\n", timeID);
					newnidxpos = MOVRcvy_SearchFinalNidxWithTimerID_2(MOV_NIDX, filesize64);
					if (newnidxpos != 0) {
						MOVRVCY_DUMP_MUST("^G NIDXrealpos = 0x%llx\r\n", newnidxpos);
						realpos = newnidxpos;
					} else {
						DBG_ERR("[Recover] MOV 2015 recovery fail!\r\n");
						return E_RSATR;//2015/04/30
					}
					#else
					DBG_ERR("[Recover] MOV 2015 recovery fail!\r\n");
					return E_RSATR;//2015/04/30
					#endif
					//#NT#2020/07/22#Willy Su -end
				}
				MOVRVCY_DUMP_MUST("^G realpos = 0x%llx\r\n", realpos);
				gMovRcvy_NewTruncatesize = realpos + 0x2000;//SMEDIAREC_MAX_NIDX_BLK;
			}
			//return E_RSATR;
		} else { //no tima!!
			DBG_DUMP("^R NO TIMA! go searching nidx\r\n");
			newnidxpos = MOVRcvy_SearchFinalNidxWithTimerID_2(MOV_NIDX, filesize64);
			if (newnidxpos == 0) {
				DBG_ERR("[Recover] MOV newnidxpos zero!\r\n");
				//return E_RSATR;//2015/04/30
			}
			MOVRVCY_DUMP_MUST("^G NIDXrealpos = 0x%llx\r\n", newnidxpos);
			realpos = newnidxpos;
			newnidx =SM2015_RCVY_VERSION; //0x315;
		}
		filesize64_align4 = ALIGN_FLOOR_4(filesize64);

		//check if 0x301
		if (newnidx != SM2015_RCVY_VERSION) { //0x315
			MR_DUMP_SEARCH("not NIDX3.15 , GO search 3.01 or 3.02 \r\n");
			gMovRcvy_NewTruncatesize = 0;//2016/04/14 if nidx_SM, no need to truncate to new size.
			blksize = mov_rcvy_get_block_size(id, MOV_RCVY_PARAM_SETTING);
			if (blksize == 0) {
				blksize = mov_rcvy_get_block_size(id, MOV_RCVY_PARAM_MAX_CFG);
			}
			MR_DUMP_SEARCH("read oft =0x%llx \r\n", filesize64_align4 - blksize);
			MR_DUMP_SEARCH("read ize =0x%llx \r\n", blksize);
			if (filesize64_align4 > blksize) {
				readsize = blksize;
			} else {
				readsize = filesize64_align4;
			}
			ret = (gMovContainerMaker.cbReadFile)(0, filesize64_align4 - blksize, readsize, gMovRecvy_LastBlockAddr);
			if (ret != E_OK) {
				DBG_ERR("[Recover] MOV SM3.0 readfile fail!\r\n");
				return E_RSATR;
			}
			newnidxpos = MOVRcvy_SearchSpecificCC(0x7864696e, gMovRecvy_LastBlockAddr, readsize);
			// nidx
			#if 0
			if (newnidxpos == 0) {
				DBG_ERR("[Recover] MOV SM3.0 recovery fail!\r\n");
				return E_RSATR;//2015/04/30
			} else {
				newnidx = 0x301;
			}
			#endif
			if (newnidxpos != 0) {
				newnidx = SMEDIARECVY_VERSION; //0x301
			}
		}

		//check if MEDIARECVY_VERSION
		if ((newnidx != SM2015_RCVY_VERSION) && (newnidx != SMEDIARECVY_VERSION)){ //0x315 ,0x301
			//read last cluster
			blksize = size;
			pos64 =  filesize64 - 8;//last 4 byte
			ret = (gMovContainerMaker.cbReadFile)(0, pos64, 8, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				DBG_ERR("[Recover] Read file error 0x%lx!\r\n", ret);
				return E_SYS;
			}

			ptr = (UINT32 *)(gMovRecvy_CluReadAddr + 4);
			if (*ptr == MOV_NIDX_CLUR) {
				ptr = (UINT32 *)gMovRecvy_CluReadAddr;
				size = *ptr;
				DBG_ERR("[Recover] MOV new cluster size = 0x%lx!\r\n", size);
				newnidx = MEDIARECVY_VERSION; //0x101;//old version
			}
		}

		//check if 0x319 (DUAL)
		if (newnidx == SM2015_RCVY_VERSION) {
			MR_DUMP_SEARCH("[Recover] GO search 3.20 or 3.21\r\n");
			pos64 = realpos - 0xc;
			size = 0x10000;
			ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				return E_SYS;
			}
			ptr = (UINT32 *)(gMovRecvy_CluReadAddr + MP_MOV_2015CO64NIDX_FIXEDSIZE);
			if (*ptr == SMDUAL_RCVY_CO64_VERSION) { //versionCode of CO64
				newnidx = SMDUAL_RCVY_VERSION;
			} else {
				ptr = (UINT32 *)(gMovRecvy_CluReadAddr + MP_MOV_2015NIDX_FIXEDSIZE);
				if (*ptr == SMDUAL_RCVY_VERSION) { //versionCode
					newnidx = SMDUAL_RCVY_VERSION;
				}
			}
		}
	}
	//#NT#2012/09/12#Meg Lin -end
	DBG_DUMP("[Recover] Version 0x%X!\r\n", newnidx);
	gMovRecFileInfo[id].uinidxversion = newnidx;
	if (newnidx == MEDIARECVY_VERSION) { //0x101
		MR_DUMP_SEARCH("nidx version=0x101\r\n");
		pos64  = filesize64 - size;
		// read size: cluster size
		ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
		if (ret != E_OK) {
			return E_SYS;
		}

		nextNidxPos = MOVRcvy_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
		while ((nextNidxPos != 0) && (nextNidxPos != MOV_NOT_NIDX)) {
			pos64 = nextNidxPos;

			ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				return E_SYS;
			}

			nextNidxPos = MOVRcvy_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
		}
		if (nextNidxPos == MOV_NOT_NIDX) {
			DBG_ERR("pos = 0x%llx not NIDX tag!!\r\n", pos64);
			return E_NOSPT;
		}
		rev = E_OK;
	} else  if (newnidx == SMEDIARECVY_VERSION) { //0x301
		MOVRVCY_DUMP_MUST("nidx version=0x301\r\n");
		pos64  = filesize64_align4 - blksize + newnidxpos - 0xc; //-0xc can find junkcc
		// read size: cluster size
		ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
		if (ret != E_OK) {
			return E_SYS;
		}

		nextNidxPos64 = MOVRcvy_SM_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
		MR_DUMP_SEARCH("nextNidxPos64 =0x%llx \r\n", nextNidxPos64);

		while ((nextNidxPos64 != 0) && (nextNidxPos64 != MOV_NOT_NIDX)) {
			pos64 = nextNidxPos64;
			MR_DUMP_SEARCH("read =0x%llx size0x%lx \r\n", pos64, size);
			ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				return E_SYS;
			}

			nextNidxPos64 = MOVRcvy_SM_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
			if (nextNidxPos64 > filesize64) { //seameless recording will happens
				nextNidxPos64 = 0;//break searching nidx
			}
		}
		if (nextNidxPos64 == MOV_NOT_NIDX) {
			DBG_ERR("pos = 0x%llx not NIDX tag!!\r\n", pos64);
			return E_NOSPT;
		}
		rev = E_OK;
	} else  if (newnidx == SM2015_RCVY_VERSION) { //0x315
		MOVRVCY_DUMP_MUST("nidx version=0x315\r\n");
		//pos  = filesize-blksize+newnidxpos-0xc;//-0xc can find junkcc
		pos64 = realpos - 0xc;
		MOVRVCY_DUMP_MUST("pos64=0x%llx\r\n", pos64);

		size = 0x10000;

		ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
		if (ret != E_OK) {
			return E_SYS;
		}

		nextNidxPos64 = MOVRcvy_2015_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
		while ((nextNidxPos64 != 0) && (nextNidxPos64 != MOV_NOT_NIDX)) {
			pos64 = nextNidxPos64;

			ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				return E_SYS;
			}

			nextNidxPos64 = MOVRcvy_2015_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
			if (nextNidxPos64 > filesize64) { //seameless recording will happens
				nextNidxPos64 = 0;//break searching nidx
			}
		}
		if (nextNidxPos64 == MOV_NOT_NIDX) {
			DBG_ERR("pos = 0x%llx not NIDX tag!!\r\n", pos64);
			return E_NOSPT;
		}
		rev = E_OK;
	} else  if (newnidx == SMDUAL_RCVY_VERSION) { //0x320
		MOVRVCY_DUMP_MUST("nidx version=0x320\r\n");
		//pos  = filesize-blksize+newnidxpos-0xc;//-0xc can find junkcc
		pos64 = realpos - 0xc;
		MOVRVCY_DUMP_MUST("pos64=0x%llx\r\n", pos64);

		size = 0x10000;

		ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
		if (ret != E_OK) {
			return E_SYS;
		}

		nextNidxPos64 = MOVRcvy_DUAL_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
		while ((nextNidxPos64 != 0) && (nextNidxPos64 != MOV_NOT_NIDX)) {
			pos64 = nextNidxPos64;
			ret = (gMovContainerMaker.cbReadFile)(0, pos64, size, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				return E_SYS;
			}
			nextNidxPos64 = MOVRcvy_DUAL_CheckNidx(id, gMovRecvy_CluReadAddr, pos64);
			if ((nextNidxPos64 > filesize64) && (nextNidxPos64 != MOV_NOT_NIDX)) { //seameless recording will happens
				nextNidxPos64 = 0;//break searching nidx
			}
		}
		if (nextNidxPos64 == MOV_NOT_NIDX) {
			DBG_ERR("pos = 0x%llx not NIDX tag!!\r\n", pos64);
			return E_NOSPT;
		}
		rev = E_OK;
	}
#if 0//coverity
	else  if (newnidx == 0x316) {
		MOVRVCY_DUMP_MUST("nidx version=0x316\r\n");
		//pos  = filesize-blksize+newnidxpos-0xc;//-0xc can find junkcc
		pos = realpos - 0xc;
		size = 0x10000;

		ret = (gMovContainerMaker.cbReadFile)(0, pos, size, gMovRecvy_CluReadAddr);
		if (ret != E_OK) {
			return E_SYS;
		}

		nextNidxPos = MOVRcvy_2015_CheckNidx(id, gMovRecvy_CluReadAddr, pos);
		while ((nextNidxPos != 0) && (nextNidxPos != MOV_NOT_NIDX)) {
			pos = nextNidxPos;

			ret = (gMovContainerMaker.cbReadFile)(0, pos, size, gMovRecvy_CluReadAddr);
			if (ret != E_OK) {
				return E_SYS;
			}

			nextNidxPos = MOVRcvy_2015_CheckNidx(id, gMovRecvy_CluReadAddr, pos);
			if (nextNidxPos > filesize64) { //seameless recording will happens
				nextNidxPos = 0;//break searching nidx
			}
		}
		if (nextNidxPos == MOV_NOT_NIDX) {
			DBG_ERR("pos = 0x%lx not NIDX tag!!\r\n", pos);
			return E_NOSPT;
		}
		return E_OK;
	}
#endif

	return rev;
}

ER MOVRcvy_UpdateMdatHDR(UINT32 id)
{
	UINT32 *ptr32, count, i, max, outAddr, mdatsize32 = 0;
	UINT32 len_l, len_h;
	UINT64  oldFilesize = 0, mdatsize64 = 0; //2012/10/22 Meg
	UINT32 *ptrFrea, freasize, freapos = 0, tima = 0;
	UINT32 timescale = 0;
	UINT32 ftypsize = 0, ftypscan = 0;

	max = mov_rcvy_get_header_size(id, MOV_RCVY_PARAM_SETTING) / 4;
	if (max == 0) {
		max = mov_rcvy_get_header_size(id, MOV_RCVY_PARAM_MAX_CFG) / 4;
	}

	outAddr = gMovRecvy_Back1sthead;//backup 1st cluster addr
	oldFilesize = gMovRecFileInfo[id].recv_filesize;//old filesize
	if (gMovRcvy_NewTruncatesize) {
		DBG_DUMP("^G newfilesize = 0x%llx!\r\n", gMovRcvy_NewTruncatesize);
		oldFilesize = gMovRcvy_NewTruncatesize;
	}
	ptr32 = (UINT32 *)outAddr;
	count = (gMovRecFileInfo[id].hdr_revSize) / 4; //2012/10/22 Meg
	if (count > max) {
		count = max;
	}

//#NT#2020/05/28#Willy Su -begin
//#NT# Fix sreaching frea fail issue
L_FreaReScan:
#if 0
	i = 0;
	ptr32 = (UINT32 *)outAddr;
	while (i < count) {
		if (*ptr32++ == 0x70696b73) { //skip
			break;
		}
		i += 1;
		freapos = (i - 1) * 4;

	}
	if (i == count) {
		DBG_DUMP("searching skip fail!!cnt=0x%lx \r\n", count);
		i = 0;
		ptr32 = (UINT32 *)outAddr;
		while (i < count) {
			if (*ptr32++ == 0x61657266) { //frea
				break;
			}
			i += 1;
			freapos = (i - 1) * 4;

		}
		if (i==count) {
			DBG_DUMP("searching frea fail!!cnt=0x%lx \r\n", count);

			return E_SYS;
		}
	}
#else
	i = 0;
	ptr32 = (UINT32 *)outAddr;
	while (i < count) {
		if (*ptr32 == 0x70696b73 || *ptr32 == 0x61657266) { //skip|frea
			break;
		}
		ptr32++;
		i += 1;
		freapos = (i - 1) * 4;

	}
	if (i == count) {
		DBG_DUMP("searching skip (or frea) fail!!cnt=0x%lx \r\n", count);
		//#NT#2020/07/22#Willy Su -begin
		//#NT# Rescan skip/frea for tag not match issue
		#if 1
		if (ftypscan == 0) {
			ptr32 = (UINT32 *)(outAddr + 0x08);
			if (*ptr32 == 0x3234706D) { //mp42
				ftypsize = MP4_FTYP_SIZE;
			} else {
				ftypsize = MOV_FTYP_SIZE;
			}
			ptr32 = (UINT32 *)(outAddr + ftypsize + 4);
			*ptr32 = MOV_SKIP; //or MOV_FREA
			ftypscan = 1;
			DBG_DUMP("searching skip (or frea) again\r\n");
			goto L_FreaReScan;
		}
		#endif
		//#NT#2020/07/22#Willy Su -end
		return E_SYS;
	}
#endif
//#NT#2020/05/28#Willy Su -end
	if (i) {
		i -= 1;
	}

	MOVRVCY_DUMP_MUST("frea pos 0x%lx\r\n", freapos);
	ptr32 = (UINT32 *)outAddr;
	ptrFrea = (UINT32 *)(outAddr + i * 4);
	freasize = Mov_Swap(*ptrFrea);
	MOVRVCY_DUMP_MUST("frea size 0x%lx\r\n", freasize);

	i = 0;
	while (i < count) {
		if (*ptr32++ == 0x7461646d) { //mdat
			break;
		}
		i += 1;
	}
	if (i == count) {
		DBG_DUMP("searching MDAT fail!!cnt=0x%lx \r\n", count);
		return E_SYS;
	}
	if ((i < count) && ((i - 1) < 0xffffffff / 4)) {
		ptr32 = (UINT32 *)(outAddr + (i - 1) * 4); //get len pos, =mdat pos -4
		if (((i - 1) * 4) != (freasize + freapos)) { //2017/03/24 fix mdatmdat
			DBG_DUMP("maybe wrong mdat!\r\n");
			//i = (freasize + freapos) / 4 + 1;
			//DBG_DUMP("new i=%d!\r\n", i);
			//ptr32 = (UINT32 *)(outAddr + (i - 1) * 4); //get len pos, =mdat pos -4
			DBG_ERR("mdat FAIL!!\r\n"); // disable flow
			return E_SYS;
		}
		if (oldFilesize < 0xFFFFFFFF) {
			mdatsize32 = oldFilesize - (i - 1) * 4; //len = oldfilesize - lenpos
			*ptr32 = Mov_Swap(mdatsize32);
			MOVRVCY_DUMP_MUST("mdat size 0x%lx\r\n", mdatsize32);
			MOVRVCY_DUMP_MUST("mdat pos 0x%lx\r\n", i * 4);
			//update tima 2017/03/20
			i = 0;
			ptr32 = (UINT32 *)outAddr;
			while (i < count) {
				if (*ptr32++ == 0x616d6974) { //tima
					break;
				}
				i += 1;
			}
			if ((i < count) && ((i + 1) < 0xffffffff / 4)) {
				ptr32 = (UINT32 *)(outAddr + (i + 1) * 4); //get len pos, =mdat pos -4
				timescale = MOV_Wrtie_GetTimeScale(gMovRecvy_vfr);
				tima = gMovRecFileInfo[id].PlayDuration / timescale;
				*ptr32 = Mov_Swap(tima);
				MOVRVCY_DUMP_MUST("update tima %d\r\n", tima);
				MOVRVCY_DUMP_MUST("tima pos 0x%lx\r\n", i * 4);
			}
			return E_OK;
		} else {
			*ptr32 = Mov_Swap(1);//if more than 4G, atom size = 1
			//new atom: "00 00 00 01" "mdat" "8 byte length"
			MOVRVCY_DUMP_MUST("short pos 0x%lx\r\n", (i - 1) * 4);

			ptr32 = (UINT32 *)(outAddr + (i + 1) * 4); //get len pos, =mdat pos +4
			mdatsize64 = oldFilesize - (i - 1) * 4; //len = oldfilesize - lenpos
			len_l = (UINT32)mdatsize64;
			len_h = (UINT32)(mdatsize64 >> 32);
			*ptr32++ = Mov_Swap(len_h);
			*ptr32++ = Mov_Swap(len_l);
			MOVRVCY_DUMP_MUST("mdat size 0x%llx\r\n", mdatsize64);
			MOVRVCY_DUMP_MUST("mdat pos 0x%lx\r\n", i * 4);
			MOVRVCY_DUMP_MUST("high 0x%lx\r\n", len_h);
			MOVRVCY_DUMP_MUST("lows 0x%lx\r\n", len_l);
			return E_OK;
		}
	}
	DBG_ERR("mdat FAIL!!\r\n");
	return E_SYS;

}

UINT32 MOVRcvy_MakeMoovHeader(MOVMJPG_FILEINFO *pMOVInfo)
{
	UINT32  headerLen = 0, id = 0;
	UINT32  soundSize, temp, temp2;//2010/01/29 Meg Lin
	UINT32  timescale = 0;
	UINT32  trackID = 0;
	// Write out video BitStream if there is video Bitstream
	//MOVWriteWaitPreCmdFinish();
	DBG_IND("MOV UPDATE HEADER !!\r\n");


	DBG_IND("addr =0x%lx\r\n", pMOVInfo->ptempMoovBufAddr);
	MOV_MakeAtomInit((void *)gMovWriteMoovTempAdr);


	//--------------VIDEO track begin -----------------
	timescale = MOV_Wrtie_GetTimeScale(pMOVInfo->VideoFrameRate);
	g_movCon[id].codec_type = REC_VIDEO_TRACK;
	g_movCon[id].width = pMOVInfo->ImageWidth;
	g_movCon[id].height = pMOVInfo->ImageHeight;
	if (pMOVInfo->encVideoCodec == MEDIAVIDENC_MJPG) {
		g_movCon[id].codec_name = (char *)&g_movMJPGCodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		//g_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
		g_movTrack[id].tag = 0x6A706567;//jpeg (Photo JPEG)
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H264) {
		g_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		g_movTrack[id].tag = 0x61766331;//avc1
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H265) {
		g_movCon[id].codec_name = (char *)&g_movH265CodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		g_movTrack[id].tag = 0x68766331;//hvc1
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
		//DBG_DUMP("^G HVC1\r\n");
	}

	//g_movCon.rc_buffer_size = 0x2EE000;
	//g_movCon.rc_max_rate = 0xA00F00;

	if (pMOVInfo->videoFrameCount < g_movEntryMax[id]) { //MOV_ENTRY_MAX)
		g_movTrack[id].entry = pMOVInfo->videoFrameCount;
	} else {
		g_movTrack[id].entry = g_movEntryMax[id];//MOV_ENTRY_MAX;
	}
	g_movTrack[id].mode = MOVMODE_MOV;
	g_movTrack[id].hasKeyframes = 1;
	g_movTrack[id].ctime = MOVWriteCountTotalSeconds();//base 2004 01 01 0:0:0
	g_movTrack[id].mtime = MOVWriteCountTotalSeconds();

	g_movTrack[id].timescale = timescale;//2009/11/24 Meg Lin
	//DBG_DUMP("trackDuration=%d!!!\r\n", pMOVInfo->PlayDuration);
	g_movTrack[id].trackDuration = pMOVInfo->PlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
	g_movTrack[id].language = 0x00;//english
	trackID++;
	g_movTrack[id].trackID = trackID;
	g_movTrack[id].frameRate = pMOVInfo->VideoFrameRate;
	g_movTrack[id].uiH264GopType = pMOVInfo->uiH264GopType;
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	//g_movTrack[id].uiRotateInfo = pMOVInfo->uiVideoRotate;
	//#NT#2013/04/17#Calvin Chang -end

	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	MOVRVCY_DUMP_MUST("2=0x%lx\r\n", (unsigned long)gp_movEntry);
	if (gp_movEntry) {
		g_movTrack[id].cluster = gp_movEntry;//&g_movEntry[0];
	} else {
		return MOV_STATUS_ENTRYADDRERR;
	}
	//--------------VIDEO track end -----------------
	//--------------VIDEO sub track begin -----------------
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		g_sub_movCon[id].codec_type = REC_VIDEO_TRACK;
		g_sub_movCon[id].width = pMOVInfo->SubImageWidth;
		g_sub_movCon[id].height = pMOVInfo->SubImageHeight;
		if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_MJPG) {
			g_sub_movCon[id].codec_name = (char *)&g_movMJPGCodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			//g_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
			g_sub_movTrack[id].tag = 0x6A706567;//jpeg (Photo JPEG)
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
		} else if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_H264) {
			g_sub_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			g_sub_movTrack[id].tag = 0x61766331;//avc1
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
		} else if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_H265) {
			g_sub_movCon[id].codec_name = (char *)&g_movH265CodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			g_sub_movTrack[id].tag = 0x68766331;//hvc1
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
			//DBG_DUMP("^G HVC1\r\n");
		}

		//g_movCon.rc_buffer_size = 0x2EE000;
		//g_movCon.rc_max_rate = 0xA00F00;

		if (pMOVInfo->SubvideoFrameCount < g_sub_movEntryMax[id]) { //MOV_ENTRY_MAX)
			g_sub_movTrack[id].entry = pMOVInfo->SubvideoFrameCount;
		} else {
			g_sub_movTrack[id].entry = g_sub_movEntryMax[id];//MOV_ENTRY_MAX;
		}
		g_sub_movTrack[id].mode = MOVMODE_MOV;
		g_sub_movTrack[id].hasKeyframes = 1;
		g_sub_movTrack[id].ctime = MOVWriteCountTotalSeconds();//base 2004 01 01 0:0:0
		g_sub_movTrack[id].mtime = MOVWriteCountTotalSeconds();

		g_sub_movTrack[id].timescale = timescale;//2009/11/24 Meg Lin
		//DBG_DUMP("trackDuration=%d!!!\r\n", pMOVInfo->PlayDuration);
		g_sub_movTrack[id].trackDuration = pMOVInfo->SubPlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
		g_sub_movTrack[id].language = 0x00;//english
		trackID++;
		g_sub_movTrack[id].trackID = trackID;
		g_sub_movTrack[id].frameRate = pMOVInfo->SubVideoFrameRate;
		g_sub_movTrack[id].uiH264GopType = pMOVInfo->uiH264GopType;
		//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
		//g_movTrack[id].uiRotateInfo = pMOVInfo->uiVideoRotate;
		//#NT#2013/04/17#Calvin Chang -end

		gp_sub_movEntry = (MOV_Ientry *)g_sub_movEntryAddr[id];
		MOVRVCY_DUMP_MUST("2=0x%lx\r\n", (unsigned long)gp_sub_movEntry);
		if (gp_movEntry) {
			g_sub_movTrack[id].cluster = gp_sub_movEntry;//&g_movEntry[0];
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
	}
#endif//add for sub-stream -end
	//--------------VIDEO sub track end -----------------
	//--------------audio track begin -----------------
#if 1//2007/06/12 meg_nowave, rec_no_wave, mark this
	if (pMOVInfo->audioTotalSize != 0) {

		g_movAudioCon[id].codec_type = REC_AUDIO_TRACK;
		g_movAudioCon[id].width = pMOVInfo->ImageWidth;
		g_movAudioCon[id].height = pMOVInfo->ImageHeight;

		if (pMOVInfo->encAudioCodec == MOVAUDENC_AAC) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_AAC;//mp4a
			g_movAudioCon[id].rc_max_rate = pMOVInfo->AudioBitRate;
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else if (pMOVInfo->encAudioCodec == MOVAUDENC_ULAW) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_ULAW;//ulaw
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else if (pMOVInfo->encAudioCodec == MOVAUDENC_ALAW) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_ALAW;//alaw
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else { // if (pMOVInfo->encAudioCodec== MOVAUDENC_PCM), default
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_PCM;//sowt
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		}

		if (pMOVInfo->encAudioCodec == MOVAUDENC_ULAW ||
			pMOVInfo->encAudioCodec == MOVAUDENC_ALAW) {
			g_movAudioCon[id].wBitsPerSample = 0x08;
		} else {
			g_movAudioCon[id].wBitsPerSample = 0x10;
		}

		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		g_movAudioCon[id].nchannel = pMOVInfo->AudioChannel;
		g_movAudioCon[id].nAvgBytesPerSecond = (pMOVInfo->AudioSampleRate) * 2 * pMOVInfo->AudioChannel;
		//#NT#2009/12/21#Meg Lin -end
		g_movAudioCon[id].nSamplePerSecond = pMOVInfo->AudioSampleRate;
		if (g_movAudioTrack[id].entry > g_movAudioEntryMax[id]) { //MOV_ENTRY_MAX)
			DBG_DUMP("^R TOOMUCH !!!id=%d, %ld > %ld \r\n", id, g_movAudioTrack[id].entry, g_movAudioEntryMax[id]);
			g_movAudioTrack[id].entry = g_movAudioEntryMax[id];//MOV_ENTRY_MAX;
		}
		g_movAudioTrack[id].context = &g_movAudioCon[id];
		//g_movAudioTrack.tag = 0x6D703476;//mp4v
		g_movAudioTrack[id].vosLen = 0x20;
		g_movAudioTrack[id].mode = MOVMODE_MOV;
		g_movAudioTrack[id].hasKeyframes = 1;
		g_movAudioTrack[id].ctime = MOVWriteCountTotalSeconds();//0xBC191380;
		g_movAudioTrack[id].mtime = MOVWriteCountTotalSeconds();//0xBC191380;
		//#NT#2009/11/24#Meg Lin -begin
		g_movAudioTrack[id].timescale = timescale;
		g_movAudioTrack[id].audioSize = pMOVInfo->audioTotalSize;
		//DBG_DUMP(("audiosize=%lx!\r\n", g_movAudioTrack.audioSize));
		//fixbug: countsize over UINT32
		//if (g_movAudioTrack.audioSize > 0x600000)//0xffffffff/600 =6.8M, *1000 will exceed UINT32 size
		//if (g_movAudioTrack[id].audioSize > 0x20000) { //0xffffffff/30000 =143165 (0x22f00), *1000 will exceed UINT32 size
		if (g_movAudioTrack[id].audioSize > 0xB000) { //0xffffffff/92400 =46482 (0xB592), *1000 will exceed UINT32 size
			temp = g_movAudioTrack[id].audioSize / g_movAudioCon[id].nAvgBytesPerSecond;
			temp2 = g_movAudioTrack[id].audioSize % g_movAudioCon[id].nAvgBytesPerSecond;
			soundSize = (temp2 * timescale) / g_movAudioCon[id].nAvgBytesPerSecond + temp * timescale; //2009/04/21 MegLin 60:01 bug fix
			//DBG_DUMP("audsize=0x%lx, soundsize=0x%lx!\r\n", g_movAudioTrack[id].audioSize, soundSize);
		} else {
			soundSize = (g_movAudioTrack[id].audioSize * timescale) / g_movAudioCon[id].nAvgBytesPerSecond;
			//DBG_DUMP("audsize=0x%lx, soundsize=0x%lx!\r\n", g_movAudioTrack[id].audioSize, soundSize);
		}
		//#NT#2009/11/24#Meg Lin -end

		g_movAudioTrack[id].trackDuration = (UINT32)soundSize;
		g_movAudioTrack[id].language = 0x00;//english
		trackID++;
		g_movAudioTrack[id].trackID = trackID; //2010/01/29 Meg Lin, fixbug
		gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];

		MOVRVCY_DUMP_MUST("3=0x%lx\r\n", (unsigned long)gp_movAudioEntry);
		if (gp_movAudioEntry) {
			g_movAudioTrack[id].cluster = gp_movAudioEntry;
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
		//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
		g_movAudioTrack[id].uiRotateInfo = 0;
		//#NT#2013/04/17#Calvin Chang -end
	}
	//--------------audio track end -----------------
#endif//2007/06/12 meg_nowave
	g_movTrack[id].pH264Desc = g_264VidDesc[id];
	g_movTrack[id].pH265Desc = g_265VidDesc[id];
	g_movTrack[id].h265Len   = g_movh265len[id];
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		g_sub_movTrack[id].pH264Desc = g_sub_264VidDesc[id];
		g_sub_movTrack[id].pH265Desc = g_sub_265VidDesc[id];
		g_sub_movTrack[id].h265Len   = g_sub_movh265len[id];
	}
#endif//add for sub-stream -end
	if ((pMOVInfo->audioTotalSize != 0)) { //&&(gMovRecFileInfo.encVideoCodec==MEDIAVIDENC_MJPG)) //FAKE_264
#if 1//add for sub-stream -begin
		if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
			headerLen = MOV_Sub_WriteMOOVTag(0, &g_movTrack[0], &g_sub_movTrack[0], &g_movAudioTrack[0], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
		} else {
			headerLen = MOVWriteMOOVTag(0, &g_movTrack[0], &g_movAudioTrack[0], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
		}
#else
		headerLen = MOVWriteMOOVTag(0, &g_movTrack[0], &g_movAudioTrack[0], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
#endif
	} else {
#if 1//add for sub-stream -begin
		if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
			headerLen = MOV_Sub_WriteMOOVTag(0, &g_movTrack[0], &g_sub_movTrack[0], 0, TRACK_INDEX_VIDEO);
		} else {
			headerLen = MOVWriteMOOVTag(0, &g_movTrack[0], 0, TRACK_INDEX_VIDEO);
		}
#else
		headerLen = MOVWriteMOOVTag(0, &g_movTrack[0], 0, TRACK_INDEX_VIDEO);
#endif
	}
	//rec_no_wave, use this --headerLen = MOVWriteMOOVTag(0, &g_movTrack, TRACK_INDEX_VIDEO);
	if (gMovRvcyMsg) {
		DBG_DUMP("headerLen=0x%lx\r\n", headerLen);
	}
	//if (gMovRecFileInfo.audioTotalSize != 0)//FAKE_264

	return headerLen;


}

MOVRCVYREAD_INFO  gRcvyReadinfo[MOVRVCY_MAX_CNT] = {0};
static UINT64 MOVRcvy_GoSearch(UINT64 startPos, UINT64 blksize, UINT32 buffer2read, UINT32 tag);

static UINT32 MOVRcvy_SearchSpecificCC(UINT32 tag, UINT32 startaddr, UINT32 size)
{
	UINT32 *ptr32;
	UINT32 count = 0, i, last = 0;

	if (gMovRvcyMsg) {
		DBG_DUMP("tag=0x%lx, startaddr=0x%lx, size=0x%lx!\r\n", tag, startaddr, size);
	}
	if (startaddr == 0) {

		return 0;
	}
	ptr32 = (UINT32 *)startaddr;
	count = size / 4;
	for (i = 0; i < count; i++) {
		if (*ptr32 == tag) {
			MR_DUMP_SEARCH("FOUND pos=0x%lx!\r\n", i * 4);
			last = (i * 4);
		}
		ptr32++;
	}
	return last;//2015/04/02
}

static UINT64 MOVRcvyTest_SearchSpecificCC(UINT32 tag, UINT32 startaddr, UINT32 size, UINT64 fileoft);


static UINT64 MOVRcvy_SearchFinalNidxWithTimerID_2(UINT32 timerID, UINT64 totalfilesize)
{
	UINT32  i;//, readsize;
	UINT32    temp1, buffer2read;
	UINT64   cnt = 0, blksize;// = 0x600000; //2017/08/31 6MB for UHD p50//2016/10/26 3MB (old10MB) big enough to find nidx
	UINT64   oldsearchi, searchi, realpos = 0, notend = 1, finalpos = 0, findsearchi = 0;
	//INT32   ret;

	//test
	//getstring = 0;

	blksize = mov_rcvy_get_block_size(0, MOV_RCVY_PARAM_SETTING);
	if (blksize == 0) {
		blksize = mov_rcvy_get_block_size(0, MOV_RCVY_PARAM_MAX_CFG);
	}

	cnt =  totalfilesize / blksize + 1;
	if (cnt > MOVRVCY_MAX_CNT) {
		DBG_DUMP("^R cnt is too big! %lld !\r\n", cnt);
		return 0;
	}
	if (gMovSearchMsg) {
		DBG_DUMP("^G totalfilesize %llx!\r\n", totalfilesize);
		DBG_DUMP("^G blksize is %lld !\r\n", blksize);
		DBG_DUMP("^G cnt is %lld !\r\n", cnt);
	}
	//totalfilesize &=0xFFFFFFFFFFFFFFFC; //warning??
	buffer2read = gMovRecvy_LastBlockAddr;
	for (i = 0; i < cnt; i++) {
		gRcvyReadinfo[i].startAddr = blksize * i;
		gRcvyReadinfo[i].endAddr = blksize * (i + 1);
		if (i == (cnt - 1)) {
			gRcvyReadinfo[i].endAddr = totalfilesize;
		}
		gRcvyReadinfo[i].searchOk = 0;
		MR_DUMP_SEARCH("[%d] start=0x%llx, end=0x%llx!\r\n", i, gRcvyReadinfo[i].startAddr, gRcvyReadinfo[i].endAddr);
	}

	MR_DUMP_SEARCH("buf 0x%lx, filesize=0x%llx, cnt= %lld\r\n", buffer2read, totalfilesize, cnt);
	//for (i=0;i<(UINT32)cnt;i++)
	if (cnt % 2) {
		searchi = (cnt + 1) / 2;
	} else {
		searchi = cnt / 2;
	}
	notend = 1;
	realpos = 0;
	oldsearchi = cnt;
	while (notend) {
		MR_DUMP_SEARCH("cnt = %lld, searchi=%lld \r\n ", cnt, searchi);
		if (searchi > 1) {
			temp1 = searchi - 1;
		} else {
			temp1 = 0;
		}
		if (gRcvyReadinfo[temp1].searchOk == 1) {
			MR_DUMP_SEARCH("end \r\n ");
			break;
		}
		//DBG_DUMP("1=0x%llx, 2=0x%llx, 3=0x%llx\r\n",gRcvyReadinfo[temp1].startAddr,blksize ,gRcvyReadinfo[temp1].endAddr);
		blksize = ((gRcvyReadinfo[temp1].startAddr + blksize) > gRcvyReadinfo[temp1].endAddr) ? (gRcvyReadinfo[temp1].endAddr - gRcvyReadinfo[temp1].startAddr) : blksize;
		realpos = MOVRcvy_GoSearch(gRcvyReadinfo[temp1].startAddr, blksize, buffer2read, timerID);
		gRcvyReadinfo[temp1].searchOk = 1;
		if (realpos != 0) {
			temp1 = (oldsearchi + searchi) / 2;
			MR_DUMP_SEARCH("realpos =0x%llx\r\n", realpos);
			//finalpos= realpos;
			finalpos = (realpos > finalpos) ? realpos : finalpos;
			findsearchi = searchi;
			oldsearchi = (searchi > oldsearchi) ? searchi : oldsearchi;

		} else {
			if (finalpos) { //has found somewhere
				UINT32 kk, newcnt, findpos, s11;
				if (findsearchi > searchi) {
					newcnt = findsearchi - searchi - 1;
					s11 = searchi;
				} else {
					newcnt = searchi - findsearchi - 1;
					s11 = findsearchi;
				}

				MR_DUMP_SEARCH("newcnt = %d\r\n", newcnt);
				for (kk = 0; kk < newcnt; kk++) {
					if ((gRcvyReadinfo[s11 + kk].startAddr + blksize) > gRcvyReadinfo[s11 + kk].endAddr) {
						blksize = gRcvyReadinfo[s11 + kk].endAddr - gRcvyReadinfo[s11 + kk].startAddr;
						MR_DUMP_SEARCH("newblk= %d\r\n", (int)blksize);
					}
					MR_DUMP_SEARCH("[%d]", (int)(searchi + kk));
					findpos = MOVRcvy_GoSearch(gRcvyReadinfo[s11 + kk].startAddr, blksize, buffer2read, timerID);
					if (findpos) {
						MR_DUMP_SEARCH("findpos = 0x%lx\r\n", findpos);
						//finalpos = findpos;
						finalpos = (findpos > finalpos) ? findpos : finalpos;

					}
				}
				break;
			}
			if (searchi % 2) {
				temp1 = (searchi + 1) / 2;
			} else {
				temp1 = searchi / 2;
			}
			oldsearchi = (searchi > oldsearchi) ? oldsearchi : searchi;

		}
		searchi = temp1;
	}
	if (searchi == (cnt - 1)) {
		blksize =  gRcvyReadinfo[searchi].endAddr - gRcvyReadinfo[searchi].startAddr;
		realpos = MOVRcvy_GoSearch(gRcvyReadinfo[searchi].startAddr, blksize, buffer2read, timerID);
		finalpos = (realpos > finalpos) ? realpos : finalpos;
	}

    if ((finalpos == 0)&&(totalfilesize<(UINT64)0x6400000)) {//2018/10/25 fix for EMR (data1 data2 nidx1 nidx2)
		MR_DUMP_SEARCH("^R finalpos zero, search againg..\r\n");
		MR_DUMP_SEARCH("cnt = %lld, searchi=%lld \r\n ", cnt, searchi);
		searchi = cnt;

		while (searchi>0) {
			if (searchi > 1) {
				temp1 = searchi - 1;
			} else {
				temp1 = 0;
			}
			//DBG_DUMP("1=0x%llx, 2=0x%llx, 3=0x%llx\r\n",gRcvyReadinfo[temp1].startAddr,blksize ,gRcvyReadinfo[temp1].endAddr);
			blksize = ((gRcvyReadinfo[temp1].startAddr + blksize) > gRcvyReadinfo[temp1].endAddr) ? (gRcvyReadinfo[temp1].endAddr - gRcvyReadinfo[temp1].startAddr) : blksize;

			realpos = MOVRcvy_GoSearch(gRcvyReadinfo[temp1].startAddr, blksize, buffer2read, timerID);
			if (realpos) {
				finalpos = realpos;//[CID:130871 (realpos > finalpos) always true.](realpos > finalpos) ? realpos : finalpos;
				break;
			}
			searchi -=1;
		}

    }
	MR_DUMP_SEARCH("final nidx pos = 0x%llx\r\n", finalpos);
	//tt2 = Perf_GetCurrent(); // Not used #2017/05/26
	//MOVRVCY_DUMP_MUST("time = %d ms", (tt2-tt1)/1000);
	return finalpos;
}
/*
static UINT32 MOVRcvy_SearchFinalNidxWithTimerID(UINT32 timerID, UINT32 totalfilesize)
{
    //char rcvytestpath[] = "A:\\Novatek\\Movie\\2015_1204_141942_029.MP4\0";
    //FST_FILE  rcvyfile;
    UINT32  size1, revt, i, lasting, readsize;
    UINT32  cnt=0;
    UINT32   tt1, tt2;
    INT32   ret;




    DBG_DUMP("getstring 0x%lx\r\n", timerID);
    if (timerID == 0)
    {
        DBG_DUMP("^R getstring fail..\r\n");
        return 0;
    }

    cnt =  totalfilesize/0x200000 + 1;
    totalfilesize &=0xFFFFFFFC;
    DBG_DUMP("buf 0x%lx, filesize=0x%lx, cnt= %d\r\n",gMovRecvy_LastBlockAddr, totalfilesize, cnt);
    for (i=0;i<(UINT32)cnt;i++)
    {
        size1 = 0x200000;
        if (i != cnt-1)
        {
            lasting = totalfilesize - (i+1)*size1;
            readsize = size1;
        }
        else
        {
            readsize = lasting;
            lasting = 0;
        }
        //DBG_DUMP("pos = 0x%lx, size=0x%lx\r\n", lasting, readsize);
        //FileSys_SeekFile(rcvyfile, lasting, FST_SEEK_SET);

        //ret = FileSys_ReadFile(rcvyfile, (UINT8 *)buffer2read, &readsize, 0, NULL);
        ret = (gMovContainerMaker.cbReadFile)(0, lasting, readsize, gMovRecvy_LastBlockAddr);

        //revt = SMRTest_SearchSpecificCC(0x7864696e, buffer2read, readsize);
        revt = MOVRcvyTest_SearchSpecificCC(timerID, gMovRecvy_LastBlockAddr, readsize, lasting);
        if (revt == 0)
        {
            DBG_DUMP(".");
            //DBG_DUMP("cannot find\r\n");
        }
        else
        {

            DBG_DUMP("find pos 0x%lx\n", revt);


            break;
        }

    }
    tt2 = Perf_GetCurrent();
    DBG_DUMP("time = %d ms", (tt2-tt1)/1000);
    return revt;
}
*/
static UINT64 MOVRcvyTest_SearchSpecificCC(UINT32 tag, UINT32 startaddr, UINT32 size, UINT64 fileoft)
{
	UINT32 *ptr32, *pID;
	UINT32 count = 0, i;
	UINT64 last = 0;

	if (1) {
		//DBG_DUMP("tag=0x%lx, startaddr=0x%lx, size=0x%lx!\r\n", tag, startaddr, size);
		MOVRVCY_DUMP_MUST("tag=0x%lx, fiiloft=0x%llx, size=0x%lx!\r\n", tag, fileoft, size);
	}
	if (startaddr == 0) {
		DBG_DUMP("startaddr ZERO!\r\n");
		return 0;
	}
	ptr32 = (UINT32 *)startaddr;
	count = size / 4;
	for (i = 0; i < count; i++) {
		if (*ptr32 == MOV_NIDX) {
			if (tag != MOV_NIDX) {
				pID = (ptr32 + 1);
				//DBG_DUMP("0x%lx \r\n", *pID);
				if (*pID == tag) {
					MR_DUMP_SEARCH("FOUND pos=0x%llx!\r\n", i * 4 + fileoft);
					MR_DUMP_SEARCH("FOUND adr=0x%lx!\r\n", i * 4 + startaddr);
					if (MOV_NOT_NIDX == mov_rcvy_check_data_blk((i * 4 + startaddr - 0xc))) {
						MR_DUMP_SEARCH("FOUND adr=0x%lx! MOV_NOT_NIDX\r\n", i * 4 + startaddr);
					} else {
						last = (i * 4) + fileoft;
					}
				}
			} else { //2017/03/10
				pID = (ptr32 + 1);
				MR_DUMP_SEARCH("timeid = 0x%lx \r\n", *pID);
				MR_DUMP_SEARCH("FOUND nidxpos=0x%llx!\r\n", i * 4 + fileoft);
				MR_DUMP_SEARCH("FOUND nidxadr=0x%lx!\r\n", i * 4 + startaddr);
				if (MOV_NOT_NIDX == mov_rcvy_check_data_blk((i * 4 + startaddr - 0xc))) {
					MR_DUMP_SEARCH("FOUND adr=0x%lx! MOV_NOT_NIDX\r\n", i * 4 + startaddr);
				} else {
					last = (i * 4) + fileoft;
				}
			}
		}
		ptr32++;
	}
	return last;//2015/04/02
}


static UINT64 MOVRcvy_GoSearch(UINT64 startPos, UINT64 blksize, UINT32 buffer2read, UINT32 tag)
{
	INT ret;
	UINT64 readsize = blksize, revt;
	MR_DUMP_SEARCH("^Mreadpos=0x%llX readsize0x%llx, adr0x%lx\r\n", startPos, blksize, buffer2read);

	ret = (gMovContainerMaker.cbReadFile)(0, startPos, readsize, buffer2read);
	if (ret != E_OK) { //2017/03/31
		DBG_DUMP("cbReadFile error!!\r\n");
		return 0;
	}


	MR_DUMP_SEARCH("aa[%d] videoMax=0x%lX, audioMax=0x%lX\r\n", 0, g_movEntryMax[0], g_movAudioEntryMax[0]);

	//DBG_DUMP("buffer=0x%lx, size=0x%lx\r\n", buffer2read,readsize);
	//FileSys_SeekFile(rcvyfile, startPos, FST_SEEK_SET);
	//ret = FileSys_ReadFile(rcvyfile, (UINT8 *)buffer2read, &readsize, 0, NULL);

	//revt = SMRTest_SearchSpecificCC(0x7864696e, buffer2read, readsize);
	revt = MOVRcvyTest_SearchSpecificCC(tag, buffer2read, readsize, startPos);

	if (revt == 0) {
		DBG_DUMP(".");
		return 0;
		//DBG_DUMP("cannot find\r\n");
	} else {

		MR_DUMP_SEARCH("find pos 0x%llx\n", revt);
		return revt;
	}

}

//@}
