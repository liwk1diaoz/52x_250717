/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__FREERTOS)
#include "comm/tse.h"
#else
#include "tse.h"
#endif
#define TSHW_INTERFACE          1
#define TSHW_INIT				tse_init
#define TSHW_UNINIT				tse_uninit
#define TSHW_OPEN				tse_open
#define TSHW_CLOSE				tse_close
#define TSHW_START(wait,bSI)	tse_start(wait,TSE_MODE_TSMUX)
#define TSHW_SET_CONFIG			tse_setConfig
#define TSHW_GET_CONFIG			tse_getConfig

// INCLUDE PROTECTED
#include "bsmux_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
static UINT32    g_debug_ts = BSMUX_DEBUG_TS;
#define BSMUX_TS_DUMP(fmtstr, args...) do { if(g_debug_ts) DBG_DUMP(fmtstr, ##args); } while(0)
extern BOOL      g_bsmux_debug_msg;
#define BSMUX_MSG(fmtstr, args...) do { if(g_bsmux_debug_msg) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

typedef struct {
	UINT64                 pcrValue;
	UINT32                 vidContinuityCnt;
	UINT32                 audContinuityCnt;
	UINT32                 patContinuityCnt;
	UINT32                 pmtContinuityCnt;
	UINT32                 pcrContinuityCnt;
	UINT32                 gpsContinuityCnt;
	UINT32                 thumbContinuityCnt;
	UINT32                 vidContCntAfterPSI;
	UINT32                 vidFrameCount;
	UINT32                 recordSec;
	BOOL                   bReConfigMuxer;
} BSMUX_TS_RECINFO;

//use: clean(while start&close)/MakePesHeader/MakePAT/MakePMT/MakePCR
//use: putall/rollback/copy
static BSMUX_TS_RECINFO g_ts_recinfo[BSMUX_MAX_CTRL_NUM] = {0};

static UINT32 gCrc32table[256] = {
0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

static UINT32 bsmux_ts_get_crc32(const UINT8 *data, UINT32 len)
{
	UINT32 i;
	UINT32 crc = 0xFFFFFFFF;
	for (i = 0; i < len; i++) {
		crc = (crc << 8) ^ gCrc32table[((crc >> 24) ^ *data++) & 0xFF];
	}
	return crc;
}

UINT64 bsmux_ts_get_pts(UINT32 pesHeaderLen, UINT32 ptsLen, UINT32 bsAddr)
{
	UINT8* data = (UINT8*)bsAddr;
	UINT64 pts = 0;
	data += (pesHeaderLen - ptsLen);
	pts = (UINT64)(*data & 0x0e) << 29 |
		((*(data+1) << 8 | *(data+2)) >> 1) << 15 |
		(*(data+3) << 8 | *(data+4)) >> 1;
	return pts;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER bsmux_ts_dbg(UINT32 value)
{
	g_debug_ts = value;
	return E_OK;
}

ER bsmux_ts_tskobj_init(UINT32 id, UINT32 *action)
{
	UINT32 value = 0;
	UINT32 emr_on, emrloop;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	//common
	if (emr_on) {
		value |= BSMUX_ACTION_WAITING_HIT;
		if (emrloop) {
			value |= BSMUX_ACTION_WAITING_RUN; //can loop
		}
	} else {
		value |= BSMUX_ACTION_WAITING_RUN;
		value |= BSMUX_ACTION_WAITING_HIT; //can pause
	}
	value |= BSMUX_ACTION_CUTFILE | BSMUX_ACTION_WAITING_IDLE;
	value |= BSMUX_ACTION_MAKE_HEADER | BSMUX_ACTION_SAVE_ENTRY;
	value |= BSMUX_ACTION_ADDLAST;
	value |= BSMUX_ACTION_PUT_LAST;
	//specific
	value |= BSMUX_ACTION_MAKE_PES;
	value |= BSMUX_ACTION_MAKE_PAT | BSMUX_ACTION_MAKE_PMT | BSMUX_ACTION_MAKE_PCR;
	//debug
	BSMUX_TS_DUMP("BSM[%d] ts action 0x%X\r\n", id, value);

	*action= value;
	return E_OK;
}

ER bsmux_ts_open(void)
{
#if TSHW_INTERFACE
	TSHW_INIT();
	TSHW_OPEN();
#endif
	return E_OK;
}

ER bsmux_ts_close(void)
{
#if TSHW_INTERFACE
	TSHW_CLOSE();
	TSHW_UNINIT();
#endif
	return E_OK;
}

UINT32 bsmux_ts_mux(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method)
{
#if TSHW_INTERFACE
	BSMUX_MUX_INFO *p_dst = (BSMUX_MUX_INFO *)uiDst;
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)uiSrc;
	TSE_BUF_INFO in_buf = {0};
	TSE_BUF_INFO out_buf = {0};
	UINT32 pid, con_cnt, id, uiSrcAddr, uiSrcSize, BufNowAddr, BufEnd, BufBlkSize;
	UINT32 result, preCalLen;
	VOS_TICK tt1, tt2, tt3, tt4;

	vos_perf_mark(&tt1);

	con_cnt = 0; //need get from ts rec info & bsq type
	pid = 0;//need get from ts rec info & bsq type
	id = pSrc->uiPortID;

	if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
		pid = MOVREC_TS_VIDEO_PID;
		if (g_ts_recinfo[id].vidContCntAfterPSI != 0xFFFFFFFF) {
			con_cnt = g_ts_recinfo[id].vidContCntAfterPSI;
			g_ts_recinfo[id].vidContCntAfterPSI = 0xFFFFFFFF;
		} else {
			con_cnt = g_ts_recinfo[id].vidContinuityCnt;
		}
	} else if (pSrc->uiType == BSMUX_TYPE_AUDIO) {
		pid = MOVREC_TS_AUDIO_PID;
		con_cnt = g_ts_recinfo[id].audContinuityCnt;
	} else if (pSrc->uiType == BSMUX_TYPE_GPSIN) {
		pid = MOVREC_TS_GPS_PID;
		con_cnt = g_ts_recinfo[id].gpsContinuityCnt;
	} else if (pSrc->uiType == BSMUX_TYPE_THUMB) {
		pid = MOVREC_TS_THUMBNAIL_PID;
		con_cnt = g_ts_recinfo[id].thumbContinuityCnt;
	}
	BSMUX_TS_DUMP("[TSMUX] PID %d CNT %d", (int)pid, (int)con_cnt);

	uiSrcAddr = pSrc->uiBSMemAddr;
	if (uiSrcAddr & 0x3) {
		DBG_WRN("addr (0x%X) not word aligned\n", uiSrcAddr);
		return 0;
	}
	uiSrcSize = pSrc->uiBSSize;
	BSMUX_TS_DUMP("[TSMUX] SRC_INFO %d %d", (int)uiSrcAddr, (int)uiSrcSize);
	#if 1
	in_buf.addr = pSrc->uiBSVirAddr;
	#else
	in_buf.addr = bsmux_util_get_buf_va(pSrc->uiPortID, uiSrcAddr);
	#endif
	in_buf.size = uiSrcSize;

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &BufNowAddr);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &BufEnd);
	/* use pre-calc-len as dst bufsize to reduce duration of drv cache_sync and mux */
	BufBlkSize = (ALIGN_CEIL_4(uiSize) + 183) / 184 * MOVREC_TS_PACKET_SIZE + MOVREC_TS_PACKET_SIZE;
	out_buf.addr = bsmux_util_get_buf_va(pSrc->uiPortID, p_dst->phy_addr);
	out_buf.size = ((BufEnd - BufNowAddr) < BufBlkSize) ? (BufEnd - BufNowAddr) : BufBlkSize;

	TSHW_SET_CONFIG(TSMUX_CFG_ID_PAYLOADSIZE, MOVREC_TS_PACKET_SIZE - 4);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_SRC_INFO, (UINT32)&in_buf);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_DST_INFO, (UINT32)&out_buf);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_SYNC_BYTE, 0x47);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_CONTINUITY_CNT, con_cnt);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_PID, pid);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_TEI, 0);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_TP, 0);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_SCRAMBLECTRL, 0);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_START_INDICTOR, 1);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_STUFF_VAL, 0xFF);
	TSHW_SET_CONFIG(TSMUX_CFG_ID_ADAPT_FLAGS, 0x0);
	vos_perf_mark(&tt2);
	result = TSHW_START(TRUE, TSE_MODE_TSMUX);
	vos_perf_mark(&tt3);
	if (result != E_OK) {
		DBG_ERR("TSHW err %d\r\n", result);
		return 0;
	}

	result = TSHW_GET_CONFIG(TSMUX_CFG_ID_MUXING_LEN);
	preCalLen = (ALIGN_CEIL_4(uiSize) + 183) / 184 * MOVREC_TS_PACKET_SIZE;
	if (result != preCalLen) {
		if (result != (preCalLen + MOVREC_TS_PACKET_SIZE)) {
			DBG_WRN("TS err %d %d\r\n", result, preCalLen);
		}
	}
	vos_perf_mark(&tt4);

	BSMUX_TS_DUMP("[%d]TSMUX S=%d T=%d\r\n", id, vos_perf_duration(tt2, tt3), vos_perf_duration(tt1, tt4));

	BSMUX_TS_DUMP("[TSMUX] MUXING_LEN %d", (int)result);
	return result;
#else
	return uiSize;
#endif
}

ER bsmux_ts_get_need_size(UINT32 id, UINT32 *p_size)
{
	UINT32 size = 0, size1;
	UINT32 gps_on = 0;
	UINT32 vfr = 0, sub_vfr = 0;
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_ON, &gps_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	// ==============================
	// =     Desc                   =
	// ==============================
	if (vfr) {
		size1 = BSMUX_VIDEO_DESC_SIZE;
		size += size1;
		BSMUX_TS_DUMP("BSM:[%d] desc size 0x%X\r\n", id, size1);
	}
	#if BSMUX_TEST_NALU
	if (sub_vfr) {
		size1 = BSMUX_VIDEO_DESC_SIZE;
		size += size1;
		BSMUX_TS_DUMP("BSM:[%d] sub desc size 0x%X\r\n", id, size1);
	}
	#endif
	// ==============================
	// =     GPS                    =
	// ==============================
	if (gps_on) {
		size1 = BSMUX_GPS_MIN;
		size += size1;
		BSMUX_TS_DUMP("BSM:[%d] gps buf size 0x%X\r\n", id, size1);
	}
	*p_size = size ;
	return E_OK;
}

ER bsmux_ts_set_need_size(UINT32 id, UINT32 addr, UINT32 *p_size)
{
	UINT32 size = 0;
	UINT32 size1, alloc_addr, need_size;
	UINT32 gps_on = 0;
	UINT32 vfr = 0, sub_vfr = 0;
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_ON, &gps_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	// get need size
	bsmux_ts_get_need_size(id, &size);
	size1 = size;
	BSMUX_TS_DUMP("BSM:[%d] total ts size = 0x%X\r\n", id, size);
	// ==============================
	// =     Desc                   =
	// ==============================
	if (vfr) {
		need_size = BSMUX_VIDEO_DESC_SIZE;
		if (size < need_size) {
			DBG_ERR("[%d] set Desc range error (%d < %d)\r\n", id, size, need_size);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCADDR, alloc_addr);
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCSIZE, need_size);
		addr += need_size;
		size -= need_size;
		BSMUX_TS_DUMP("BSM:[%d] Desc addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	} else {
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCADDR, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCSIZE, 0);
	}
	#if BSMUX_TEST_NALU
	if (sub_vfr) {
		need_size = BSMUX_VIDEO_DESC_SIZE;
		if (size < need_size) {
			DBG_ERR("[%d] set sub Desc range error (%d < %d)\r\n", id, size, need_size);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, alloc_addr);
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, need_size);
		addr += need_size;
		size -= need_size;
		BSMUX_TS_DUMP("BSM:[%d] Sub Desc addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	} else {
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, 0);
	}
	#endif
	// ==============================
	// =     GPS                    =
	// ==============================
	if (gps_on) {
		need_size = BSMUX_GPS_MIN;
		if (size < need_size) {
			DBG_ERR("[%d] set GPS range error (%d < %d)\r\n", id, size, need_size);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_GSP_ADDR, alloc_addr);
		addr += need_size;
		size -= need_size;
		BSMUX_TS_DUMP("BSM:[%d] GPS addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	} else {
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_GSP_ADDR, 0);
	}
	*p_size = (size1 - size);
	//init
	{
		g_ts_recinfo[id].pcrValue = 0;
		g_ts_recinfo[id].vidContinuityCnt = 0;
		g_ts_recinfo[id].audContinuityCnt = 0;
		g_ts_recinfo[id].patContinuityCnt = 0;
		g_ts_recinfo[id].pmtContinuityCnt = 0;
		g_ts_recinfo[id].pcrContinuityCnt = 0;
		g_ts_recinfo[id].gpsContinuityCnt = 0;
		g_ts_recinfo[id].thumbContinuityCnt = 0;
		g_ts_recinfo[id].vidContCntAfterPSI = 0xFFFFFFFF;
		g_ts_recinfo[id].vidFrameCount = 0;
		g_ts_recinfo[id].bReConfigMuxer = TRUE;
		g_ts_recinfo[id].recordSec = 0;
	}
	return E_OK;
}

ER bsmux_ts_clean(UINT32 id)
{
	{
		//step 1
		g_ts_recinfo[id].vidContinuityCnt = 0;
		g_ts_recinfo[id].audContinuityCnt = 0;
		g_ts_recinfo[id].patContinuityCnt = 0;
		g_ts_recinfo[id].pmtContinuityCnt = 0;
		g_ts_recinfo[id].pcrContinuityCnt = 0;
		g_ts_recinfo[id].gpsContinuityCnt = 0;
		g_ts_recinfo[id].thumbContinuityCnt = 0;
		g_ts_recinfo[id].vidFrameCount = 0;
		g_ts_recinfo[id].recordSec = 0;
		//step 2
		g_ts_recinfo[id].pcrValue = 0;
		g_ts_recinfo[id].vidContCntAfterPSI = 0xFFFFFFFF;
		g_ts_recinfo[id].bReConfigMuxer = TRUE;
	}
	return E_OK;
}

ER bsmux_ts_update_vidinfo(UINT32 id, UINT32 addr, void *p_bsq)
{
	return E_OK;
}

ER bsmux_ts_release_buf(void *pBuf)
{
	BSMUXER_FILE_BUF *p_buf = (BSMUXER_FILE_BUF *)pBuf;
	UINT32 op = p_buf->fileop;
	UINT32 id = p_buf->pathID;
	UINT32 addr = 0;
	UINT32 cut_blk = 0;
	UINT32 end2card = 0;
	static UINT32 real2card = 0;

	if (bsmux_util_is_null_obj((void *)p_buf)) {
		DBG_ERR("[%d] buf null\r\n", id);
		return E_PAR;
	}

	if (p_buf->addr == 0) {
		goto label_release_buf_close;
	}

	addr = p_buf->addr + (UINT32)p_buf->size;

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_END2CARD, &end2card);
	if (end2card == addr) {
		/*
		 *  new flow: need to wait last close blk,
		 *  so temporarily set end2card abnormal.
		 *  BEFORE set real2card to avoid race condition.
		 */
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, 0);
	}

	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_REAL2CARD, addr);
	real2card = addr;
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, addr);

	// critical section: lock
	bsmux_buffer_lock();
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_USEDBLOCK, &cut_blk);
	cut_blk--;
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_USEDBLOCK, cut_blk);
	bsmux_buffer_unlock();
	// critical section: unlock
	DBG_IND("%s: id(%d) USEDBLOCK[%d]\r\n", __func__, id, cut_blk);

label_release_buf_close:
	if (op & BSMUXER_FOP_CLOSE) {
		#if 0
		/* for wait file end */
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_REAL2CARD, real2card);
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, real2card);
		real2card = 0;
		#endif

		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_CUTBLK, &cut_blk);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_CUTBLK, (cut_blk-1));

		/* for wait file end */
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_CUTBLK, &cut_blk);
		if (cut_blk == 0) {
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_REAL2CARD, real2card);
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, real2card);
		real2card = 0;
		}
	}

	DBG_IND(">>> bsmux_util_release_buf 0x%X\r\n", p_buf);
	if (g_bsmux_debug_msg) {
		bsmux_ctrl_mem_det_range(id);
	}
	return E_OK;
}

ER bsmux_ts_add_gps_data(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO bsq = {0};
	UINT32 outputAddr, outputMem;
	UINT32 addr, size;
	UINT32 gps_queue = 0;
	UINT8* data;

	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_QUEUE, &gps_queue);
	if (gps_queue) {
		//use user buffer
		outputAddr = pBsq->uiBSVirAddr; //va
		outputMem = pBsq->uiBSMemAddr; //pa
	} else {
		//use internal buffer
		bsmux_util_get_param(id, BSMUXER_PARAM_HDR_GSP_ADDR, &outputAddr); //va
		if (outputAddr == 0) {
			DBG_ERR("[%d] TS outputAddr is ZERO!!!!\r\n", id);
			return E_SYS;
		}
		outputMem = bsmux_util_get_buf_pa(id, outputAddr); //pa
	}
	addr = pBsq->uiBSMemAddr; //pa
	size = pBsq->uiBSSize;
	hd_gfx_memcpy(outputMem + 6, addr, size); //pa
	data = (UINT8*)outputAddr; //va
    *data++ = 0x00;
    *data++ = 0x00;
    *data++ = 0x01;
    *data++ = 0xBF;
    *data++ = size >> 8;
    *data++ = size & 0xFF;

	bsq.uiBSMemAddr = outputMem; //pa
	bsq.uiBSVirAddr = outputAddr; //va
	bsq.uiBSSize    = size + 6;
	bsq.uiType      = BSMUX_TYPE_GPSIN;
	bsq.uiTimeStamp = hwclock_get_longcounter();
	bsq.uiPortID    = id;

	bsmux_ctrl_bs_enqueue(id, (void *)&bsq);
	bsmux_tsk_bs_in();

	return E_OK;
}

ER bsmux_ts_add_thumb(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO bsq = {0};
	UINT8* data;
	UINT32 addr = pBsq->uiBSMemAddr;
	UINT32 size = pBsq->uiBSSize;
	UINT32 va;

#if TSHW_INTERFACE
	UINT32 pesSize, i;
	pesSize = addr - ALIGN_FLOOR_4(addr - 6); //for addr align (round down)
	va = pBsq->uiBSVirAddr - pesSize;
	data = (UINT8*)va; //va
	*data++ = 0x00;
	*data++ = 0x00;
	*data++ = 0x01;
	*data++ = 0xBF;
	*data++ = size >> 8;
	*data++ = size & 0xFF;
	for (i = 0; i < (pesSize - 6); i++) {
		*data++ = 0xFF; //stuffing
	}

	bsq.uiBSMemAddr = addr - pesSize; //pa
	bsq.uiBSSize    = size + pesSize;
	bsq.uiType      = BSMUX_TYPE_THUMB;
	bsq.uiTimeStamp = hwclock_get_longcounter();
	bsq.uiPortID    = id;

	if (bsq.uiBSMemAddr & 0x3) {
		DBG_WRN("thumb addr (0x%X) not word aligned\n", bsq.uiBSMemAddr);
		return 0;
	}
	bsq.uiBSVirAddr = va;
#else
	hd_gfx_memcpy(addr + 6, addr, size);
	va = bsmux_util_get_buf_va(id, addr);
	data = (UINT8*)va; //va
    *data++ = 0x00;
    *data++ = 0x00;
    *data++ = 0x01;
    *data++ = 0xBF;
    *data++ = size >> 8;
    *data++ = size & 0xFF;

	bsq.uiBSMemAddr = addr; //pa
	bsq.uiBSSize    = size + 6;
	bsq.uiType      = BSMUX_TYPE_THUMB;
#endif

	DBG_DUMP("[%d] add thumbnail (pa=0x%x va=0x%x size=0x%x)\r\n", id,
		pBsq->uiBSMemAddr, pBsq->uiBSVirAddr, pBsq->uiBSSize);

	bsmux_ctrl_bs_enqueue(id, (void *)&bsq);
	bsmux_tsk_bs_in();

	return E_OK;
}

ER bsmux_ts_add_last(UINT32 id)
{
	UINT32 now_addr = 0, last2card_addr = 0, vfr = 0, vfn = 0;
	UINT8* data = NULL;

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &now_addr);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card_addr);

	//Add PAT/PMT/PCR packet in the end of file if not exist
	if ((now_addr - last2card_addr) >= MOVREC_TS_PACKET_SIZE) {
		data = (UINT8*)(now_addr - MOVREC_TS_PACKET_SIZE);
	}

	//if last TS packet is not PCR packet
	if ((!data) || (*(data+1) != 0x41) || (*(data+2) != 0x00)) {
		if (vfr > 0) {
			UINT64 endPCRValue = g_ts_recinfo[id].pcrValue +(90000/vfr+((90000%vfr)?1:0)) * 300 * (vfn % 3);
			UINT32 pat_addr = 0, pmt_addr = 0, pcr_addr = 0;

			pat_addr = now_addr;
			pmt_addr = pat_addr + MOVREC_TS_PACKET_SIZE;
			pcr_addr = pmt_addr + MOVREC_TS_PACKET_SIZE;

			bsmux_ts_make_pat(id, pat_addr);
			bsmux_ts_make_pmt(id, pmt_addr);
			bsmux_ts_make_pcr(id, pcr_addr, endPCRValue);

			now_addr += MOVREC_TS_PACKET_SIZE * 3;
			bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_NOWADDR, now_addr);
		}
	} else {
		DBG_IND("[%d] not add last\r\n", id);
	}

	g_ts_recinfo[id].bReConfigMuxer = TRUE;
	//#NT#2020/06/01#Willy Su -begin
	//#NT# Not to reset pts for HLS m3u8
	//#NT# If need to reset pts, set before muxer start
	{
		UINT32 pts_reset = 0;
		bsmux_util_get_param(id, BSMUXER_PARAM_PTS_RESET, &pts_reset);
		if (pts_reset) {
			bsmux_util_set_minfo(MEDIAWRITE_SETINFO_RESETPTS, 0, 0, id);
		}
	}
	//#NT#2020/06/01#Willy Su -end

	return E_OK;
}

ER bsmux_ts_put_last(UINT32 id)
{
	UINT32 last2card = 0;
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, last2card);
	bsmux_util_prepare_buf(id, BSMUX_BS2CARD_LAST, 0, 0, 0);

	return E_OK;
}

ER bsmux_ts_add_meta(UINT32 id, void *p_bsq)
{
	DBG_IND("not supported\r\n");
	return E_OK;
}

ER bsmux_ts_save_entry(UINT32 id, void *p_bsq)
{
#if TSHW_INTERFACE
	BSMUX_SAVEQ_BS_INFO *pPesQ = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 vfr = 0;

	if (bsmux_util_is_null_obj((void *)pPesQ)) {
		DBG_DUMP("[%d] pPesQ zero\r\n", id);
		return E_PAR;
	}

	if (pPesQ->uiBSSize == 0) {
		DBG_DUMP("[%d] BSSize zero\r\n", id);
		return E_PAR;
	}

	if (pPesQ->uiType == BSMUX_TYPE_VIDEO) {
		UINT32 vidCopyCount;
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vidCopyCount);
		vidCopyCount += 1;
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALVF, vidCopyCount);
		g_ts_recinfo[id].vidContinuityCnt = TSHW_GET_CONFIG(TSMUX_CFG_ID_CON_CURR_CNT);

		// ready to add PAT/PMT/PCR per 100 ms
		if ((vidCopyCount % 3) == 0) {
			UINT32 pat_addr = 0, pmt_addr = 0, pcr_addr = 0;
			UINT32 now_addr = 0, vfr = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &now_addr);
			g_ts_recinfo[id].vidContCntAfterPSI = TSHW_GET_CONFIG(TSMUX_CFG_ID_CON_CURR_CNT);
			pat_addr = now_addr;
			pmt_addr = pat_addr + MOVREC_TS_PACKET_SIZE;
			pcr_addr = pmt_addr + MOVREC_TS_PACKET_SIZE;
			bsmux_ts_make_pat(id, pat_addr);
			bsmux_ts_make_pmt(id, pmt_addr);
			g_ts_recinfo[id].pcrValue += (90000/vfr + ((90000%vfr)?1:0)) * 300 * 3;//pcr per 3 frames
			bsmux_ts_make_pcr(id, pcr_addr, g_ts_recinfo[id].pcrValue);
			now_addr += MOVREC_TS_PACKET_SIZE * 3;
			bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_NOWADDR, now_addr);
		}

		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		if ((vidCopyCount % vfr) == 0) {
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_NOWSEC, (vidCopyCount/vfr));
			if (id == 0) {
				BSMUX_TS_DUMP("sec=%d\r\n", (vidCopyCount/vfr));
			}
		}
	}
	else if (pPesQ->uiType == BSMUX_TYPE_AUDIO) {
		UINT32 audCopyCount;
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &audCopyCount);
		audCopyCount += 1;
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALAF, audCopyCount);
		g_ts_recinfo[id].audContinuityCnt = TSHW_GET_CONFIG(TSMUX_CFG_ID_CON_CURR_CNT);
	}
	else if (pPesQ->uiType == BSMUX_TYPE_GPSIN) {
		g_ts_recinfo[id].gpsContinuityCnt = TSHW_GET_CONFIG(TSMUX_CFG_ID_CON_CURR_CNT);
	}
	else if (pPesQ->uiType == BSMUX_TYPE_THUMB) {
		g_ts_recinfo[id].thumbContinuityCnt = TSHW_GET_CONFIG(TSMUX_CFG_ID_CON_CURR_CNT);
	}
#endif
	return E_OK;
}

// make first PSI
ER bsmux_ts_make_header(UINT32 id, void *p_maker)
{
	UINT32 pat_addr = 0, pmt_addr = 0, pcr_addr = 0;
	UINT32 now_addr = 0, last2card_addr = 0;

	if (g_ts_recinfo[id].bReConfigMuxer) {

		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &now_addr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card_addr);

		// demux address need 8-byte aligned, 4 + 188*3 = 568 %8 == 0
		if ((now_addr % 8) == 0) {
			now_addr += 4;
			last2card_addr += 4;
		}

		//#NT#2020/05/29#Willy Su -begin
		//#NT# Set PCR to zero only when reset PTS is set
		//#NT# If need to reset PCR, set before muxer start
		{
			UINT32 pts_reset = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_PTS_RESET, &pts_reset);
			if (pts_reset) {
				g_ts_recinfo[id].pcrValue = 0;
			}
		}
		//#NT#2020/05/29#Willy Su -end

		pat_addr = now_addr;
		pmt_addr = pat_addr + MOVREC_TS_PACKET_SIZE;
		pcr_addr = pmt_addr + MOVREC_TS_PACKET_SIZE;

		bsmux_ts_make_pat(id, pat_addr);
		bsmux_ts_make_pmt(id, pmt_addr);
		bsmux_ts_make_pcr(id, pcr_addr, g_ts_recinfo[id].pcrValue);
		now_addr += MOVREC_TS_PACKET_SIZE * 3;

		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_NOWADDR, now_addr);
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_LAST2CARD, last2card_addr);

		g_ts_recinfo[id].bReConfigMuxer = FALSE;
	} else {
		DBG_WRN("[%d] already make first PSI\r\n", id);
	}

	return E_OK;
}

// need to add padding bytes in order to take care of tse word aligned
ER bsmux_ts_make_pes(UINT32 id, void *p_src)
{
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_src;
	UINT32 pesAddr = 0, pesSize = 0, naluSize = 0;
	UINT32 idtype = 0;
	UINT32 codectype = 0, adts_bytes = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &codectype);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCSIZE, &naluSize);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_EN_ADTS, &adts_bytes);

	if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
		if (pSrc->uiIsKey) {
			if (naluSize && (codectype == MEDIAVIDENC_H264)) {
				pSrc->uiBSMemAddr = pSrc->uiBSMemAddr - naluSize;
				#if 1
				pSrc->uiBSVirAddr = pSrc->uiBSVirAddr - naluSize;
				#endif
				pSrc->uiBSSize = pSrc->uiBSSize + naluSize;
			}
		}
		idtype = TS_IDTYPE_VIDPATHID;
		if (codectype == MEDIAVIDENC_H264) {
			pesSize = TS_VIDEOPES_HEADERLENGTH + TS_VIDEO_264_NAL_LENGTH; //14 + 6
		} else {
			pesSize = TS_VIDEOPES_HEADERLENGTH + TS_VIDEO_265_NAL_LENGTH; //14 + 7
		}
#if TSHW_INTERFACE
		pesSize = pSrc->uiBSMemAddr - ALIGN_FLOOR_4(pSrc->uiBSMemAddr - pesSize); //for pes addr align
#endif
	} else if (pSrc->uiType == BSMUX_TYPE_AUDIO) {
		pSrc->uiBSMemAddr = pSrc->uiBSMemAddr - adts_bytes;
		#if 1
		pSrc->uiBSVirAddr = pSrc->uiBSVirAddr - adts_bytes;
		#endif
		pSrc->uiBSSize = pSrc->uiBSSize + adts_bytes;
		idtype = TS_IDTYPE_AUDPATHID;
		pesSize = TS_AUDIOPES_HEADERLENGTH + TS_AUDIO_ADTS_LENGTH; //14
#if TSHW_INTERFACE
		pesSize = pSrc->uiBSMemAddr - ALIGN_FLOOR_4(pSrc->uiBSMemAddr - pesSize); //for pes addr align
#endif
	}

	#if 1
	pesAddr = (pSrc->uiBSVirAddr - pesSize);
	#else
	pesAddr = bsmux_util_get_buf_va(id, (pSrc->uiBSMemAddr - pesSize));
	#endif
	BSMUX_TS_DUMP("TSM[%d]: make pes (0x%X)", id, pesAddr);

	//add PES header before BS
#if TSHW_INTERFACE
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_PESHEADERSIZE, pesSize, TRUE, id);
#endif
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_PESHEADERADDR, pesAddr, pSrc->uiBSSize, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MAKEPESHEADER, idtype, pSrc->uiIsKey, id);

	//shift uiBSMemAddr for add PES header
	pSrc->uiBSMemAddr = bsmux_util_get_buf_pa(id, pesAddr);
	#if 1
	pSrc->uiBSVirAddr = pesAddr;
	#endif
	pSrc->uiBSSize = pSrc->uiBSSize + pesSize;

	return E_OK;
}

ER bsmux_ts_make_pat(UINT32 id, UINT32 patAddr)
{
	UINT8 *q = (UINT8 *)patAddr;
    UINT32 crc;
    UINT32 val = 0;
    UINT32 udtaLen;
    MOV_USER_MAKERINFO *makerInfo = NULL;
    UINT32 makerInfoAddr = 0;
	UINT32 pmtPid = MOVREC_TS_PMT_PID;

	if (patAddr == 0) {
		DBG_ERR("PAT address is NULL\r\n");
		return E_PAR;
	}

    bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_USERDATA, &makerInfoAddr, 0, 0);
    makerInfo = (MOV_USER_MAKERINFO *)makerInfoAddr;

    //store maker/model name in PAT
    if ((makerInfo != NULL) && (makerInfo->pMaker != NULL) && (makerInfo->pModel != NULL))
    {
        *q++ = 0x47;//sync byte
        *q++ = 0x40;
        *q++ = 0x00;
        val =  0x30 | (g_ts_recinfo[id].patContinuityCnt & 0x0F);

        *q++ = val;

        *q++ = makerInfo->makerLen + makerInfo->modelLen + 2; // adaptation_field_length
        *q++ = 0x02;
        *q++ = makerInfo->makerLen + makerInfo->modelLen; // transport_private_data_length

        *q++ = makerInfo->makerLen - 1;
        memcpy(q, makerInfo->pMaker, makerInfo->makerLen - 1);
        q += makerInfo->makerLen - 1;

        *q++ = makerInfo->modelLen - 1;
        memcpy(q, makerInfo->pModel, makerInfo->modelLen - 1);
        q += makerInfo->modelLen - 1;

        *q++ = 0x00;//pointer_field //0x04
        *q++ = 0x00;//table_id		//0x05
        *q++ = 0xB0;				//0x06

        *q++ = 0x0D;//section_length //0x07

        *q++ = 0x30;				//0x08
        *q++ = 0x34;//transport stream id //0x09

        *q++ = 0xC3;//version num & current next //0x0a

        *q++ = 0x00;//section number		//0x0b
        *q++ = 0x00;//last section number	//0x0c

        *q++ = 0x00;						//0x0d
        *q++ = 0x01;//program num			//0x0e

        val = (0x0E << 4) | (pmtPid >> 8);	//0x0f

        *q++ = val;
        *q++ = pmtPid & 0xFF;// program map PID

        udtaLen = makerInfo->makerLen + makerInfo->modelLen + 3;

        crc = bsmux_ts_get_crc32((UINT8 *)(patAddr + 5 + udtaLen), 12);

        *q++ = (crc >> 24) & 0xFF;
        *q++ = (crc >> 16) & 0xFF;
        *q++ = (crc >> 8) & 0xFF;
        *q++ = (crc) & 0xFF;//CRC

        memset((UINT8 *)(patAddr + 21 + udtaLen), 0xFF, MOVREC_TS_PACKET_SIZE - 21 - udtaLen);//stuffing
    }
    else
    {
        *q++ = 0x47;//sync byte
        *q++ = 0x40;
        *q++ = 0x00;
        val =  0x10 | (g_ts_recinfo[id].patContinuityCnt & 0x0F);

        *q++ = val;

        *q++ = 0x00;//pointer_field
        *q++ = 0x00;//table_id
        *q++ = 0xB0;

        *q++ = 0x0D;//section_length

        *q++ = 0x30;
        *q++ = 0x34;//transport stream id

        *q++ = 0xC3;//version num & current next

        *q++ = 0x00;//section number
        *q++ = 0x00;//last section number

        *q++ = 0x00;
        *q++ = 0x01;//program num

        val = (0x0E << 4) | (pmtPid >> 8);
        *q++ = val;
        *q++ = pmtPid & 0xFF;// program map PID

        crc = bsmux_ts_get_crc32((UINT8 *)(patAddr + 5), 12);

        *q++ = (crc >> 24) & 0xFF;
        *q++ = (crc >> 16) & 0xFF;
        *q++ = (crc >> 8) & 0xFF;
        *q++ = (crc) & 0xFF;//CRC
        memset((UINT8 *)(patAddr + 21), 0xFF, MOVREC_TS_PACKET_SIZE - 21);//stuffing
    }
    g_ts_recinfo[id].patContinuityCnt = (g_ts_recinfo[id].patContinuityCnt + 1) % 0x10;

	return E_OK;
}

ER bsmux_ts_make_pmt(UINT32 id, UINT32 pmtAddr)
{
	UINT32 vidPid = MOVREC_TS_VIDEO_PID;
	UINT32 audPid = MOVREC_TS_AUDIO_PID;
	UINT32 pcrPid = MOVREC_TS_PCR_PID;
	UINT8* q = (UINT8*)pmtAddr;
    UINT32 val = 0, crc;
    UINT32 uiRecFormat, vidcodec, aud_en = 0;

	if (pmtAddr == 0) {
		DBG_ERR("PMT address is NULL\r\n");
		return E_PAR;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_RECFORMAT, &uiRecFormat);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &vidcodec);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_ON, &aud_en);

    if (uiRecFormat != MEDIAREC_TIMELAPSE && aud_en != 0)
    {
        *q++ = 0x47;//sync byte
        val = MOVREC_TS_PMT_PID;
        *q++ = 0x40 | val >> 8;
        *q++ = val;
        val = (0x01 << 4) | (g_ts_recinfo[id].pmtContinuityCnt & 0x0F);
        *q++ = val;

        *q++ = 0x00;//pointer_field
        *q++ = 0x02;//table_id
        *q++ = 0xB0;

        *q++ = 0x17;//section_length

        *q++ = 0x00;
        *q++ = 0x01;//program num

        *q++ = 0xC3;//version num & current next

        *q++ = 0x00;
        *q++ = 0x00;

        *q++ = 0xE0 | (pcrPid >> 8); //PCR PID
        *q++ = pcrPid & 0xFF;

        *q++ = 0xF0;
        *q++ = 0x00;//program description

		if(vidcodec == MEDIAVIDENC_H264)
	        *q++ = 0x1B;//stream type H264
	    else
	        *q++ = 0x24;//stream type H265

        val = (0x0E << 4) | (vidPid >> 8);
        *q++ = val;
        *q++ = vidPid & 0xFF;//video PID

        *q++ = 0xF0;
        *q++ = 0x00;//description info

        *q++ = 0x0F;//stream type AAC

        val = (0x0E << 4) | (audPid >> 8);
        *q++ = val;
        *q++ = audPid & 0xFF;//audio PID

        *q++ = 0xF0;
        *q++ = 0x00;//description info

        crc = bsmux_ts_get_crc32((UINT8*)(pmtAddr + 5), 22);

        *q++ = (crc >> 24) & 0xFF;
        *q++ = (crc >> 16) & 0xFF;
        *q++ = (crc >> 8) & 0xFF;
        *q++ = (crc) & 0xFF;//CRC

        memset((UINT8*)(pmtAddr + 31), 0xFF, MOVREC_TS_PACKET_SIZE - 31);//stuffing
    }
    else
    {
        *q++ = 0x47;//sync byte
        val = MOVREC_TS_PMT_PID;
        *q++ = 0x40 | val >> 8;
        *q++ = val;
        val = (0x01 << 4) | (g_ts_recinfo[id].pmtContinuityCnt & 0x0F);

        *q++ = val;

        *q++ = 0x00;//pointer_field
        *q++ = 0x02;//table_id
        *q++ = 0xB0;

        *q++ = 0x12;//section_length

        *q++ = 0x00;
        *q++ = 0x01;//program num

        *q++ = 0xC3;//version num & current next

        *q++ = 0x00;
        *q++ = 0x00;

        *q++ = 0xE0 | (pcrPid >> 8); //PCR PID
        *q++ = pcrPid & 0xFF;

        *q++ = 0xF0;
        *q++ = 0x00;//program description

		if(vidcodec == MEDIAVIDENC_H264)
	        *q++ = 0x1B;//stream type H264
	    else
	        *q++ = 0x24;//stream type H265

        val = (0x0E << 4) | (vidPid >> 8);
        *q++ = val;
        *q++ = vidPid & 0xFF;//video PID

        *q++ = 0xF0;
        *q++ = 0x00;//description info

        //no audio
        crc = bsmux_ts_get_crc32((UINT8*)(pmtAddr + 5), 17);

        *q++ = (crc >> 24) & 0xFF;
        *q++ = (crc >> 16) & 0xFF;
        *q++ = (crc >> 8) & 0xFF;
        *q++ = (crc) & 0xFF;//CRC

        memset((UINT8*)(pmtAddr + 26), 0xFF, MOVREC_TS_PACKET_SIZE - 26);//stuffing
    }
    g_ts_recinfo[id].pmtContinuityCnt = (g_ts_recinfo[id].pmtContinuityCnt + 1) % 0x10;

	return E_OK;
}

ER bsmux_ts_make_pcr(UINT32 id, UINT32 pcrAddr, UINT64 pcrValue)
{
	UINT8* q = (UINT8*)pcrAddr;
    UINT32 val = 0, pcrExt;
    UINT64 pcrBase;

	if (pcrAddr == 0) {
		DBG_ERR("PCR address is NULL\r\n");
		return E_SYS;
	}

    *q++ = 0x47;//sync byte
    val = MOVREC_TS_PCR_PID;
    *q++ = 0x40 | val >> 8;
    *q++ = val & 0xFF;
    val = (0x02 << 4) | (g_ts_recinfo[id].pcrContinuityCnt & 0x0F);

    *q++ = val;

    *q++ = 0xB7;//adaptation field length=183
    *q++ = 0x10;//PCR flag

    pcrBase = pcrValue / 300;
    pcrExt  = pcrValue % 300;

    *q++ = (pcrBase >> 25) & 0xFF;
    *q++ = (pcrBase >> 17) & 0xFF;
    *q++ = (pcrBase >> 9) & 0xFF;
    *q++ = (pcrBase >> 1) & 0xFF;
    *q++ = ((pcrBase << 7) & 0x80) | 0x7E | ((pcrExt >> 8) & 0x01);
    *q++ = pcrExt & 0xFF;
    memset((UINT8*)(pcrAddr + 12), 0xFF, MOVREC_TS_PACKET_SIZE - 12);//stuffing
    //g_ts_recinfo[id].pcrContinuityCnt = (g_ts_recinfo[id].pcrContinuityCnt + 1) % 0x10;

	return E_OK;
}

