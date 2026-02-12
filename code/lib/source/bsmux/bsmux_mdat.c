/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "bsmux_init.h"
#define _TOFIX_ 0

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _MDAT_NIDX_INFO {
	UINT32 nowVfn;      //now video frame number
	UINT32 thisObjId;   //fstskid to identify
	UINT32 vfr;         //video frame rate
	UINT32 outNidxAddr; //addr to put Nidx
	UINT32 thisV_fn;    //this second video totalfn
	UINT32 v_wid;
	UINT32 v_hei;
	UINT32 v_codec;     //video codec type
	UINT32 a_sr;        //audio sample rate
	UINT32 a_chs;       //channels
	UINT32 a_codec;     //audio codec type
	UINT32 h264DescAddr;//addr of h264 desc
	UINT64 lastNidxPos; //last nidx fileoffset
	UINT64 nowFoft;     //now file offset
	UINT64 lastNidxPos_co64; //last nidx fileoffset
	UINT64 nowFoft_co64;     //now file offset
	UINT32 h264DescSize;//size
	UINT32 nowAfn;      //now audio frame number
	UINT32 thisA_fn;    //this second audio total
	UINT32 lastAfn;     //last nidx audio frame number
	UINT32 sub_vfr;        //video frame rate (sub-streram)
	UINT32 nowSubVfn;      //now video frame number (sub-streram)
	UINT32 thisSubV_fn;    //this second video totalfn (sub-streram)
} MDAT_NIDX_INFO;

typedef struct _BSMUX_GPS_INFO {
	UINT32 outGpsAddr; //addr to put Nidx
	UINT32 GPS_addr;    //gps data addr
	UINT32 GPS_size;    //gps data size
} BSMUX_GPS_INFO;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
//>> sepcific: header related
static MAKEMOOV_INFO      g_bsmux_moov_info[BSMUX_MAX_CTRL_NUM] ={0};
static MOVMJPG_FILEINFO   g_bsmux_nfinfo[BSMUX_MAX_CTRL_NUM] ={0};
static UINT32             g_bsmux_max_entry_sec[BSMUX_MAX_CTRL_NUM] ={0};
static MEDIAREC_HDR_OBJ   g_bsmux_hdr_obj[BSMUX_MAX_CTRL_NUM] = {0};
static BSMUX_HDRMEM_INFO  g_bsmux_hdr_front[BSMUX_MAX_CTRL_NUM][BSMUX_HDRMEM_MAX] = {0};
static BSMUX_HDRMEM_INFO  g_bsmux_hdr_temp[BSMUX_MAX_CTRL_NUM][BSMUX_HDRMEM_MAX] = {0};
static BSMUX_HDRMEM_INFO  g_bsmux_hdr_back[BSMUX_MAX_CTRL_NUM][BSMUX_HDRMEM_MAX] = {0};
static UINT32             g_bsmux_hdr_count = BSMUX_HDRNUM_MAX;
// nidx
static UINT32             g_mdat_nidx_queue[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32             g_mdat_nidx_qnum[BSMUX_MAX_CTRL_NUM] = {0};
static UINT64             g_mdat_nidx_lastpos[BSMUX_MAX_CTRL_NUM] = {0};
// gps
static BSMUX_GPS_INFO     g_mdat_gps_info[BSMUX_MAX_CTRL_NUM] = {0};
// debug
static UINT32             g_debug_mdat = BSMUX_DEBUG_MDAT;
#define MDAT_FUNC(fmtstr, args...) do { if(g_debug_mdat) DBG_DUMP(fmtstr, ##args); } while(0)
extern BOOL               g_bsmux_debug_msg;
#define BSMUX_MSG(fmtstr, args...) do { if(g_bsmux_debug_msg) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
//Note: do not export this function
static MAKEMOOV_INFO *bsmux_get_moov_info(UINT32 id)
{
	g_bsmux_moov_info[id].pfileinfo = &g_bsmux_nfinfo[id];
	return &g_bsmux_moov_info[id];
}
//Note: do not export this function
static MEDIAREC_HDR_OBJ *bsmux_get_hdr_obj(UINT32 id)
{
	return &g_bsmux_hdr_obj[id];
}
//Note: do not export this function
static BSMUX_HDRMEM_INFO *bsmux_get_hdr_mem_info(UINT32 type, UINT32 id)
{
	BSMUX_HDRMEM_INFO *p_hdr_mem_info = NULL;
	switch (type) {
	case BSMUX_HDRMEM_FRONT:
		p_hdr_mem_info = &(g_bsmux_hdr_front[id][0]);
		break;
	case BSMUX_HDRMEM_TEMP:
		p_hdr_mem_info = &(g_bsmux_hdr_temp[id][0]);
		break;
	case BSMUX_HDRMEM_BACK:
		p_hdr_mem_info = &(g_bsmux_hdr_back[id][0]);
		break;
	default:
		DBG_ERR("type error\r\n");
		break;
	}
	return p_hdr_mem_info;
}
//Note: do not export this function
static void bsmux_update_vidsize(UINT32 updateaddr, UINT32 vidsize)
{
	UINT8 *pchar;
	pchar = (UINT8 *)updateaddr;
	*pchar++ = (vidsize >> 24) & 0xFF;
	*pchar++ = (vidsize >> 16) & 0xFF;
	*pchar++ = (vidsize >> 8) & 0xFF;
	*pchar++ = vidsize & 0xFF;
}
//Note: do not export this function
static void bsmux_update_string(UINT32 updateaddr, char *string)
{
	UINT8 *pchar;
	pchar = (UINT8 *)updateaddr;
	*pchar++ = *string++;
	*pchar++ = *string++;
	*pchar++ = *string++;
	*pchar++ = *string++;
}
//Note: do not export this function
static UINT32 bsmux_mdat_nidx_getone(UINT32 id)
{
	MDAT_NIDX_INFO *pnidx = NULL;
	UINT32 num, i;
	pnidx = (MDAT_NIDX_INFO *)g_mdat_nidx_queue[id];
	num = g_mdat_nidx_qnum[id];
	for (i = 0; i < num; i++) {
		if (pnidx->v_hei == 0) {
			return i;
		}
		pnidx++;
	}
	return i;
}
//Note: do not export this function
static INT32 bsmux_mdat_nidx_clearone(UINT32 id, UINT32 nidxid)
{
	MDAT_NIDX_INFO *pnidx = NULL;
	UINT32 num;
	pnidx = (MDAT_NIDX_INFO *)g_mdat_nidx_queue[id];
	num = g_mdat_nidx_qnum[id];
	if (nidxid >= num) {
		DBG_ERR("err id=%d, nidxid=%d\r\n", id, nidxid);
		return -1;
	}
	pnidx += nidxid;
	pnidx->v_hei = 0;//clear
	return 0;
}
//Note: do not export this function
//NMR_BsMuxer_GetNidxQSize
static UINT32 bsmux_mdat_nidx_getsize(UINT32 id)
{
	UINT32 size = 0, rollbacksec;
	UINT32 emrloop = 0, strgbuf = 0;
	UINT32 resvblksec = BSMUX_NIDX_RESVSEC;
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollbacksec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);
	if (emrloop && strgbuf) {
		rollbacksec = 2;//temp suggestion
	}
	size = (sizeof(MDAT_NIDX_INFO)) * (resvblksec+rollbacksec);
	g_mdat_nidx_qnum[id] = resvblksec+rollbacksec;
	return size;
}
//Note: do not export this function
//NMR_BsMuxer_InitNidxQ
static ER bsmux_mdat_nidx_init(UINT32 id, UINT32 addr, UINT32 size)
{
	MDAT_NIDX_INFO *pnidx = NULL;
	UINT32 num, i;
	{
		g_mdat_nidx_queue[id] = addr;
		g_mdat_nidx_qnum[id] = size / sizeof(MDAT_NIDX_INFO);
	}
	pnidx = (MDAT_NIDX_INFO *)g_mdat_nidx_queue[id];
	num = g_mdat_nidx_qnum[id];
	for (i = 0; i < num; i++) {
		pnidx->v_hei = 0;
		memset((void *)pnidx, 0, sizeof(MDAT_NIDX_INFO));
		pnidx++;
	}
	return E_OK;
}
//almost approaching frameinfo size
static UINT32 bsmux_mdat_calc_entry_size(UINT32 id)
{
	UINT32 size, sec, secentry, seamlessSec, rollbackSec;
	UINT32 ext = 0, ext_unit = 0, ext_num = 0;
	UINT32 seamlessSec_ms = 0, rollbackSec_ms = 0;

	//get prj setting
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_ENABLE, &ext);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_UNIT, &ext_unit);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_MAX_NUM, &ext_num);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, &seamlessSec_ms);
	#endif
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollbackSec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC_MS, &rollbackSec_ms);
	#endif
	secentry = bsmux_util_calc_entry_per_sec(id);
	{ //cutoverlap
		if (rollbackSec || rollbackSec_ms) {
			if (rollbackSec_ms) {
				rollbackSec = (rollbackSec_ms / 1000) + ((rollbackSec_ms % 1000) ? 1 : 0);
			}
		} else {
			rollbackSec = 2;
		}
		sec = seamlessSec + rollbackSec;
		if (seamlessSec_ms) {
			sec = (seamlessSec_ms / 1000) + ((seamlessSec_ms % 1000) ? 1 : 0) + rollbackSec;
		}
	}
	if (ext) {
		sec += (ext_unit * ext_num);
	}
	size = secentry * sec * sizeof(MOV_Ientry);
	//size = ALIGN_CEIL_64(size);
	size = bsmux_util_calc_align(size, bsmux_util_calc_reserved_size(id), NMEDIAREC_ALIGN_CEIL);
	DBG_IND(">>> Entry second=%d, size=0x%lx\r\n", sec, size);
	return size;
}
//Note: do not export this function
static UINT32 bsmux_mdat_calc_gps_size(UINT32 id, UINT32 IsTag)
{
	UINT32 size, sec, seamlessSec;
	UINT32 ext = 0, ext_unit = 0, ext_num = 0;
	UINT32 seamlessSec_ms = 0;
	UINT32 gps_rate = 0;

	//get prj setting
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_RATE, &gps_rate);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_ENABLE, &ext);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_UNIT, &ext_unit);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_MAX_NUM, &ext_num);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, &seamlessSec_ms);
	#endif
	{ //cutoverlap
		sec = seamlessSec + 2;
		if (seamlessSec_ms) {
			sec = (seamlessSec_ms / 1000) + ((seamlessSec_ms % 1000) ? 1 : 0) + 2;
		}
	}
	if (ext) {
		sec += (ext_unit * ext_num);
	}
	if (gps_rate == 0) {
		gps_rate = 1;
	}
	size = gps_rate * sec * sizeof(MOV_Ientry);
	if (IsTag) {
		size += 0x30;
	}
	size = ALIGN_CEIL_64(size);
	DBG_IND(">>> Entry second=%d, rate=%d, size=0x%lx\r\n", sec, gps_rate, size);
	return size;
}
//Note: do not export this function
static UINT32 bsmux_mdat_calc_meta_size(UINT32 id, UINT32 IsTag)
{
	UINT32 size = 0, sec, seamlessSec;
	UINT32 ext = 0, ext_unit = 0, ext_num = 0;
	UINT32 seamlessSec_ms = 0;
	UINT32 meta_num = 0, meta_rate = 0, vfr = 0;

	//get prj setting
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	meta_rate = vfr; //temp as vfr
	bsmux_util_get_param(id, BSMUXER_PARAM_META_NUM, &meta_num);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_ENABLE, &ext);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_UNIT, &ext_unit);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_MAX_NUM, &ext_num);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, &seamlessSec_ms);
	#endif
	{ //cutoverlap
		sec = seamlessSec + 2;
		if (seamlessSec_ms) {
			sec = (seamlessSec_ms / 1000) + ((seamlessSec_ms % 1000) ? 1 : 0) + 2;
		}
	}
	if (ext) {
		sec += (ext_unit * ext_num);
	}
	if (meta_rate == 0) {
		meta_rate = 1;
	}
	while (meta_num > 0) {
		size += meta_rate * sec * sizeof(MOV_Ientry);
		meta_num--;
	}
	if (IsTag) {
		size += 0x30;
	}
	size = ALIGN_CEIL_64(size);
	DBG_IND(">>> Entry second=%d, rate=%d, size=0x%lx\r\n", sec, meta_rate, size);
	return size;
}
//Note: do not export this function
static void bsmux_mdat_package_thumb(UINT32 id)
{
	UINT32 frontheader_1 = 0;
	UINT32 frontPut = 0, thumbPut = 0;
	UINT32 last2card_addr = 0;
	UINT32 mux_method = 0;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};

	bsmux_util_get_param(id, BSMUXER_PARAM_MUXMETHOD, &mux_method);
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, &frontheader_1);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_THUMBPUT, &thumbPut);

	if (!thumbPut) {
		MDAT_FUNC("[%d] package_thumb no thumb, return.\r\n", id);
		return ;
	}

	if (!frontPut) {
		MDAT_FUNC("[%d] copy temphdr addr to last2card addr\r\n", id);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card_addr);
		#if _TOFIX_
		if (bsmux_util_is_not_normal()) {
			DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
			return ; ///< Insufficient memory
		}
		#endif
		dst.phy_addr = bsmux_util_get_buf_pa(id, last2card_addr);
		dst.vrt_addr = last2card_addr;
		src.phy_addr = bsmux_util_get_buf_pa(id, frontheader_1);
		src.vrt_addr = frontheader_1;
		bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, bsmux_mdat_calc_header_size(id), mux_method);
	} else {
		MDAT_FUNC("[%d] push temphdr addr to write card\r\n", id);
		bsmux_util_prepare_buf(id, BSMUX_BS2CARD_THUMB, frontheader_1, bsmux_mdat_calc_header_size(id), 0);
	}
}
//Note: do not export this function
static void bsmux_mdat_package_header(UINT32 id)
{
	BSMUX_SAVEQ_BS_INFO hdr_buf = {0};
	UINT32 frontheader_1 = 0;
	UINT32 frontheader_2 = 0;
	UINT32 offset = 0;
	#if 1  //20210827
	UINT32 strgbuf_en = 0, strgbuf_hdr = 0;

	// chk util storage buffer
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf_en);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_HDR, &strgbuf_hdr);
	if (strgbuf_en && strgbuf_hdr) {
		goto label_update_offset;
	}
	#endif //20210827

	//package
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, &frontheader_1);
	hdr_buf.uiBSMemAddr = bsmux_util_get_buf_pa(id, frontheader_1);
	if (!hdr_buf.uiBSMemAddr) {
		DBG_ERR("[%d] get front addr zero\r\n", id);
	}
	hdr_buf.uiBSVirAddr = frontheader_1;
	hdr_buf.uiBSSize = bsmux_mdat_calc_header_size(id);
	bsmux_ctrl_bs_copy2buffer(id, (void *)&hdr_buf);
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, &frontheader_2);
	if (frontheader_2 == 0) {
		DBG_DUMP("BSM:[%d] frontheader_2 zero\r\n", id);
	}

label_update_offset:
	//update fronthdr offset
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_COPYPOS, &offset);
	offset += bsmux_mdat_calc_header_size(id);
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, offset);
}
//Note: do not export this function
static void bsmux_mdat_padding_header(UINT32 id)
{
	UINT32 blksize = 0;
	UINT32 padding = 0;
	UINT32 offset = 0;
	UINT32 freashift = 0, tempaddr, tempsize;
	VOS_TICK tt1, tt2;

	//calculate padding
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEBLOCK, &blksize);
	if (blksize < bsmux_mdat_calc_header_size(id) + bsmux_util_calc_padding_size(id)) {
		DBG_WRN("[%d] blksize(0x%lx) calc_header(0x%lx) calc_padding(0x%lx)\r\n", (int)id, (unsigned long)blksize,
			(unsigned long)bsmux_mdat_calc_header_size(id), (unsigned long)bsmux_util_calc_padding_size(id));
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &offset);
		DBG_WRN("[%d] seamless_sec(%d) exceed limit, changed to 1min\r\n", id, offset);
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, 60);
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, 60 * 1000);
		offset = 0;
		#if 1   //fast put flow ver3
		bsmux_util_pset_padding_size(id, 0);
		DBG_WRN("[%d] changed calc_padding(0x%lx)\r\n", (unsigned long)bsmux_util_calc_padding_size(id));
		#endif	//fast put flow ver3
	}
	#if 1   //fast put flow ver3
	padding = blksize - bsmux_mdat_calc_header_size(id) - bsmux_util_calc_padding_size(id);
	#else
	padding = blksize - bsmux_mdat_calc_header_size(id) - MP_AUDENC_AAC_RAW_BLOCK * 3;
	#endif	//fast put flow ver3
	bsmux_util_get_param(id, BSMUXER_PARAM_FREAENDSIZE, &freashift); //set while make header

	//update buffer current addr
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &offset);
	MDAT_FUNC("[%d] mem org now addr=0x%x, padding=0x%x\r\n", id, offset, padding);
	vos_perf_mark(&tt1);
	memset((void *)(offset), 0x00, padding);
	tempaddr = offset - bsmux_mdat_calc_header_size(id) + freashift + 8;
	tempsize = blksize - freashift - 8;
	MDAT_FUNC("[%d] mem mdat now addr=0x%x, size=0x%x\r\n", id, tempaddr, tempsize);
	bsmux_update_vidsize(tempaddr, tempsize); //update padding blk size
	bsmux_update_string(tempaddr+4, "free"); //update padding blk tag
	vos_perf_mark(&tt2);
	MDAT_FUNC("[%d] clear 0x%x wait %dms\r\n", id, padding, vos_perf_duration(tt1, tt2)/1000);
	offset += padding;
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_NOWADDR, offset);
	MDAT_FUNC("[%d] mem cur now addr=0x%x\n", id, offset);

	//update fronthdr offset
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_COPYPOS, &offset);
	MDAT_FUNC("[%d] file org offset=0x%x, padding=0x%x\r\n", id, offset, padding);
	offset += padding;
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, offset);
	MDAT_FUNC("[%d] file cur offset=0x%x\r\n", id, offset);
}
//Note: do not export this function
static void bsmux_mdat_put_desc(UINT32 id, UINT32 addr, UINT32 size)
{
	UINT32 maker_id = 0, vidcodec = 0;
	bsmux_util_get_mid(id, &maker_id);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &vidcodec);
	if (vidcodec == MEDIAVIDENC_H264) {
		MDAT_FUNC("[H264][%d] put desc addr=0x%X size=0x%X\r\n", id, addr, size);
		bsmux_util_set_minfo(MEDIAWRITE_SETINTO_MOV_264DESC, addr, size, maker_id);
	} else if (vidcodec == MEDIAVIDENC_H265) {
		MDAT_FUNC("[H265][%d] put desc addr=0x%X size=0x%X\r\n", id, addr, size);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_H265DESC, addr, size, maker_id);
	}
}
//Note: do not export this function
static void bsmux_mdat_put_sub_desc(UINT32 id, UINT32 addr, UINT32 size)
{
	UINT32 maker_id = 0, vidcodec = 0;
	bsmux_util_get_mid(id, &maker_id);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &vidcodec);
	if (vidcodec == MEDIAVIDENC_H264) {
		MDAT_FUNC("[H264][%d] sub put desc addr=0x%X size=0x%X\r\n", id, addr, size);
		bsmux_util_set_minfo(MEDIAWRITE_SETINTO_SUB_H264DESC, addr, size, maker_id);
	} else if (vidcodec == MEDIAVIDENC_H265) {
		MDAT_FUNC("[H265][%d] sub put desc addr=0x%X size=0x%X\r\n", id, addr, size);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_H265DESC, addr, size, maker_id);
	}
}
//Note: do not export this function
static UINT32 bsmux_mdat_calc_duration(UINT32 id, UINT32 vfn, UINT32 vfr)
{
	UINT32 timescale = 0, dur;
	// calc
	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_TIMESCALE, &id, &vfr, &timescale);
	dur = vfn * (timescale / vfr);
	BSMUX_MSG("[%d] avg_duration=%d\r\n", id, dur);
	return dur;
}
//Note: do not export this function
static UINT32 bsmux_mdat_calc_entry_duration(UINT32 id, UINT32 vfn, UINT32 vfr)
{
	UINT32 value = vfr;
	// get from movmaker
	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_ENTRY_DURATION, &id, &vfn, &value);
	if ((vfn != vfr) && (value == vfr)) {
		value = bsmux_mdat_calc_duration(id, vfn, vfr);
	}
	BSMUX_MSG("[%d] entry_duration=%d\r\n", id, value);
	return value;
}

/*-----------------------------------------------------------------------------*/
/* Specific Functions                                                          */
/*-----------------------------------------------------------------------------*/
ER bsmux_mdat_set_hdr_mem(UINT32 type, UINT32 id, UINT32 addr, UINT32 size)
{
	BSMUX_HDRMEM_INFO *pStart = 0;
	UINT32 num = bsmux_mdat_get_hdr_count();
	UINT32 i;

	pStart = bsmux_get_hdr_mem_info(type, id);
	if (pStart == NULL) {
		DBG_ERR("get hdr mem info null\r\n");
		return E_SYS;
	}

	for (i = 0; i < num; i++) {
		pStart->hdrtype = type;
		pStart->hdraddr = addr + (i * size);
		pStart->hdrsize = size;
		pStart->hdrused = 0;
		pStart++;
	}

	return E_OK;
}

UINT32 bsmux_mdat_get_hdr_mem(UINT32 type, UINT32 id)
{
	BSMUX_HDRMEM_INFO *pStart = 0;
	UINT32 num = bsmux_mdat_get_hdr_count();
	UINT32 get = 0;
	UINT32 i;

	pStart = bsmux_get_hdr_mem_info(type, id);
	if (pStart == NULL) {
		DBG_ERR("get hdr mem info null\r\n");
		return 0;
	}

	for (i = 0; i < num; i++) {
		if (pStart->hdrtype == type) {
			if (pStart->hdrused == 0) {
				pStart->hdrused = 1;
				get = pStart->hdraddr;
				DBG_DUMP("GetHeader [%d][%d] type=%d, adr=0x%lx\r\n", id, i, pStart->hdrtype, pStart->hdraddr);
				return get;
			} else {
				DBG_DUMP("GetHeader [%d][%d] type=%d, adr=0x%lx USED\r\n", id, i, pStart->hdrtype, pStart->hdraddr);
			}
		}
		pStart++;
	}
	if (get == 0) {
		DBG_DUMP("^R GET ERROR : HDRGet[%d]type=%d fail!\r\n", id, type);
	}

	return get;
}

ER bsmux_mdat_rel_hdr_mem(UINT32 type, UINT32 id, UINT32 addr)
{
	BSMUX_HDRMEM_INFO *pStart = 0;
	UINT32 num = bsmux_mdat_get_hdr_count();
	UINT32 i;

	pStart = bsmux_get_hdr_mem_info(type, id);
	if (pStart == NULL) {
		DBG_ERR("[%d] get hdr mem info null\r\n", id);
		return E_NOEXS;
	}

	for (i = 0; i < num; i++) {
		if (pStart->hdrtype == type) {
			if (pStart->hdrused == 1) {
				if (pStart->hdraddr == addr) {
					pStart->hdrused = 0;
					DBG_DUMP("RelHeader [%d][%d] type=%d, adr=0x%lx\r\n", id, i, pStart->hdrtype, pStart->hdraddr);
				}
			}
		}
		pStart++;
	}
	return E_OK;
}

BOOL bsmux_mdat_chk_hdr_mem(UINT32 type, UINT32 id)
{
	BSMUX_HDRMEM_INFO *pStart = 0;
	UINT32 num = bsmux_mdat_get_hdr_count();
	UINT32 used = 0;
	UINT32 i;
	BOOL result = TRUE;

	pStart = bsmux_get_hdr_mem_info(type, id);
	if (pStart == NULL) {
		DBG_ERR("get hdr mem info null\r\n");
		return FALSE;
	}

	for (i = 0; i < num; i++) {
		if (pStart->hdrtype == type) {
			if (pStart->hdrused == 0) {
				DBG_IND("ChkHeader [%d][%d] type=%d, adr=0x%lx\r\n", id, i, pStart->hdrtype, pStart->hdraddr);
			} else {
				DBG_IND("ChkHeader [%d][%d] type=%d, adr=0x%lx USED\r\n", id, i, pStart->hdrtype, pStart->hdraddr);
				used++;
			}
		}
		pStart++;
	}
	if (used == num) {
		BSMUX_MSG("CHK FULL: HDRChk[%d]type=%d all used! (%d/%d)\r\n", id, type, used, num);
		result = FALSE;
	} else {
		BSMUX_MSG("CHK FREE: HDRChk[%d]type=%d free(%d)!\r\n", id, type, (num-used));
	}

	return result;
}

UINT32 bsmux_mdat_set_hdr_count(UINT32 value)
{
	if (value <= BSMUX_HDRMEM_MAX) {
		g_bsmux_hdr_count = value;
	} else {
		g_bsmux_hdr_count = BSMUX_HDRNUM_MAX;
	}
	return 0;
}

UINT32 bsmux_mdat_get_hdr_count(void)
{
	return g_bsmux_hdr_count;
}

ER bsmux_mdat_set_max_entrytime(UINT32 id, UINT32 idx_size, UINT32 vfr)
{
	UINT32 ratio, min;
	if (vfr % 30) {
		if (vfr > 30) {
			ratio = (vfr * 100) / 30 + 1;
		} else {
			ratio = 100;
		}
	} else {
		ratio = (vfr * 100) / 30;
	}
	min = ((idx_size / 2048) * 3) / 100; //2M = 30min

	g_bsmux_max_entry_sec[id] = (min * 60 * 100) / ratio; //2M = 30fps 60 min
	DBG_IND("[%d] entry time = %d sec\r\n", id, g_bsmux_max_entry_sec[id]);
	return E_OK;
}

ER bsmux_mdat_get_max_entrytime(UINT32 id, UINT32 *value)
{
	*value = g_bsmux_max_entry_sec[id];
	return E_OK;
}

ER bsmux_mdat_nidx_make(UINT32 id, UINT64 offset, UINT32 nidxid)
{
	SM_REC_DUAL_CO64_NIDXINFO nidxInfo = {0}; // if not CO64, use SM_REC_DUAL_NIDXINFO
	MEDIAREC_ENTRY_INFO vidInfo = {0};
	MEDIAREC_ENTRY_INFO audInfo = {0};
	SM_AUD_NIDXINFO audnidx = {0};
	SM_VID_NIDXINFO vidnidx = {0};
	MDAT_NIDX_INFO *ptr;
	UINT32 makerid, nidxmax;

	ptr = (MDAT_NIDX_INFO *)g_mdat_nidx_queue[id];
	nidxmax = g_mdat_nidx_qnum[id];
	if (nidxid >= nidxmax) {
		DBG_ERR("err id=%d, nidxid=%d\r\n", id, nidxid);
		return E_PAR;
	}
	ptr += nidxid;

	if (ptr->outNidxAddr == 0) {
		DBG_ERR("[%d] outNidxAddr is ZERO!!!!\r\n", id);
		return E_NOEXS;
	}
	if (ptr->thisV_fn > 1000) { //actual = -vfr*sec;
		DBG_ERR("[%d] thisV_fn %d too large!!!!\r\n", id, ptr->thisV_fn);
		return E_SYS;
	}

	bsmux_util_get_mid(id, &makerid);
	memset((char *)ptr->outNidxAddr, 0, BSMUX_MAX_NIDX_BLK);
	vidnidx.nowVfn = ptr->nowVfn;
	vidnidx.thisVfn = ptr->thisV_fn;
	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_SM_VIDENTRY_ADDR, &makerid, (UINT32 *)&vidnidx, (UINT32 *)&vidInfo);
	audnidx.nowAfn = ptr->nowAfn;
	audnidx.thisAfn = ptr->thisA_fn;
	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_SM_AUDENTRY_ADDR, &makerid, (UINT32 *)&audnidx, (UINT32 *)&audInfo);

	nidxInfo.dataoutAddr        = ptr->outNidxAddr;
	nidxInfo.thisVideoStartFN   = ptr->nowVfn;
	nidxInfo.thisVideoFN        = ptr->thisV_fn;
	nidxInfo.thisVideoFrameData = vidInfo.entryAddr;
	nidxInfo.thisVideoFrmDataLen = vidInfo.entrySize;
	nidxInfo.thisAudioFrameData = audInfo.entryAddr;
	nidxInfo.thisAudioFrmDataLen = audInfo.entrySize;
	if (nidxInfo.thisVideoStartFN == 0) {
		nidxInfo.lastNidxdataPos64 = 0; // if not CO64, use lastNidxdataPos
	} else {
		nidxInfo.lastNidxdataPos64 = g_mdat_nidx_lastpos[id]; // if not CO64, use lastNidxdataPos
	}
	nidxInfo.thisAudioStartFN   = audnidx.nowAfn;
	nidxInfo.thisAudioFN        = audnidx.thisAfn;
	nidxInfo.audioCodec         = ptr->a_codec;
	nidxInfo.videoVfr = ptr->vfr;
	nidxInfo.videoWid = ptr->v_wid;
	nidxInfo.videoHei = ptr->v_hei;
	nidxInfo.videoCodec = ptr->v_codec;
	nidxInfo.audioSampleRate = ptr->a_sr;
	nidxInfo.audioChannel = ptr->a_chs;
	nidxInfo.h264descLen = (UINT32)ptr->h264DescSize;
	nidxInfo.h264descData = (UINT32)ptr->h264DescAddr;
	nidxInfo.headerSize = bsmux_mdat_calc_header_size(id);
	nidxInfo.versionCode = SM2015_RCVY_VERSION;
	nidxInfo.clusterSize = BSMUX_MAX_NIDX_BLK;

	if (ptr->sub_vfr) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &(nidxInfo.SubvideoVfr));
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_WIDTH, &(nidxInfo.SubvideoWid));
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_HEIGHT, &(nidxInfo.SubvideoHei));
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &(nidxInfo.SubvideoCodec));
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &(nidxInfo.Subh264descData));
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, &(nidxInfo.Subh264descLen));
		vidnidx.nowVfn = ptr->nowSubVfn;
		vidnidx.thisVfn = ptr->thisSubV_fn;
		bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_SM_SUBENTRY_ADDR, &makerid, (UINT32 *)&vidnidx, (UINT32 *)&vidInfo);
		nidxInfo.thisSubVideoStartFN   = ptr->nowSubVfn;
		nidxInfo.thisSubVideoFN        = ptr->thisSubV_fn;
		nidxInfo.thisSubVideoFrameData = vidInfo.entryAddr;
		nidxInfo.thisSubVideoFrmDataLen = vidInfo.entrySize;
	}

	bsmux_mkhdr_lock(); //avoid when maker header
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_DUAL_NIDXDATA, (UINT32)&nidxInfo, 1, id); // if not CO64, use 0 as p2
	bsmux_mkhdr_unlock(); //avoid when maker header

	ptr->nowFoft = offset;
	ptr->nowVfn = ptr->thisV_fn + ptr->vfr;
	if (ptr->sub_vfr) {
		ptr->nowSubVfn = ptr->thisSubV_fn + ptr->sub_vfr;
	}
	g_mdat_nidx_lastpos[id] = offset;
	bsmux_mdat_nidx_clearone(id, nidxid);

	return E_OK;
}

ER bsmux_mdat_gps_make(UINT32 id, UINT64 offset)
{
	MEDIAREC_GPSINFO gpsinfo = {0};
	BSMUX_GPS_INFO *ptr;
	UINT32 gps_blk = BSMUX_GPS_MIN;
	UINT32 gps_queue = 0, gps_shift = 16;

	ptr = &g_mdat_gps_info[id];
	if (ptr->outGpsAddr == 0) {
		DBG_ERR("[%d] outGpsAddr is ZERO!!!!\r\n", id);
		return E_NOEXS;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_QUEUE, &gps_queue);
	if (gps_queue) {
		//use user buffer
		gps_blk = ptr->GPS_size + gps_shift; //shift reserved size for header
		memset((char *)ptr->outGpsAddr, 0, gps_shift); //clear reserved size
	} else {
		//use internal buffer
		memset((char *)ptr->outGpsAddr, 0, gps_blk); //clear internal buffer size
	}
	gpsinfo.bufAddr = ptr->outGpsAddr; //va
	gpsinfo.clusterSize = gps_blk;
	gpsinfo.gpsDataAddr = ptr->GPS_addr; //va
	gpsinfo.gpsDataSize = ptr->GPS_size;

	bsmux_mkhdr_lock(); //avoid when maker header
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_GPSDATA, (UINT32)&gpsinfo, 0, id);
	bsmux_mkhdr_unlock(); //avoid when maker header

	return E_OK;
}

UINT32 bsmux_mdat_calc_header_size(UINT32 id)
{
	UINT32 front_moov = 0;
	UINT32 header_size;

	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);

	if (front_moov) {
		header_size = bsmux_util_calc_reserved_size(id) + bsmux_mdat_calc_entry_size(id); //already align
	} else {
		header_size = bsmux_util_calc_reserved_size(id);
	}
	return header_size;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER bsmux_mdat_dbg(UINT32 value)
{
	g_debug_mdat = value;
	return E_OK;
}

ER bsmux_mdat_tskobj_init(UINT32 id, UINT32 *action)
{
	UINT32 value = 0;
	UINT32 emr_on, overlap, front_moov, emrloop;
	UINT32 sub_vfr;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_OVERLAP_ON, &overlap);
	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
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
	//specific
	value |= BSMUX_ACTION_MAKE_HEADER | BSMUX_ACTION_UPDATE_HEADER | BSMUX_ACTION_SAVE_ENTRY | BSMUX_ACTION_PAD_NIDX;
	//front moov
	if (front_moov) {
		value |= BSMUX_ACTION_MAKE_MOOV;
	}
	//overlap
	if (overlap) {
		value |= BSMUX_ACTION_OVERLAP_1SEC;
	}
	if (sub_vfr) {
		value |= BSMUX_ACTION_REORDER_1SEC;
	}
	//debug
	MDAT_FUNC(">> [%d] mdat action 0x%X\r\n", id, value);

	*action= value;
	return E_OK;
}

ER bsmux_mdat_open(void)
{
	return E_OK;
}

ER bsmux_mdat_close(void)
{
	return E_OK;
}

UINT32 bsmux_mdat_memcpy(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method)
{
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)uiSrc;
	UINT32 uiSrcAddr = pSrc->uiBSMemAddr;
	UINT32 uiSrcSize = uiSize;
	BSMUX_MUX_INFO *p_dst = (BSMUX_MUX_INFO *)uiDst;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};
#if BSMUX_TEST_MULTI_TILE
	UINT32 id = pSrc->uiPortID;
	UINT32 nalu_size = 0, nalu_num = 0;
	UINT32 cntsize = 0;
	UINT32 cnt;

	if (pSrc->uiIsKey) {

		if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCSIZE, &nalu_size);
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_NALUNUM, &nalu_num);
		}
		else if (pSrc->uiType == BSMUX_TYPE_SUBVD) {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, &nalu_size);
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_NALUNUM, &nalu_num);
		}

		if (nalu_num > 1) {

			nalu_size = nalu_size / nalu_num;

			for (cnt = 0; cnt < (nalu_num-1); cnt++)
			{
				dst.phy_addr = (p_dst->phy_addr + cntsize);
				dst.vrt_addr = (p_dst->vrt_addr + cntsize);
				src.phy_addr = uiSrcAddr;
				src.vrt_addr = pSrc->uiBSVirAddr;
				bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, nalu_size, method);
				cntsize += nalu_size;
			}

			if (cntsize >= uiSize) {
				DBG_ERR("[%d] uiDst=0x%x uiSize=0x%x (cntsize=0x%x)\r\n",
					id, p_dst->phy_addr, uiSize, cntsize);
			}

			p_dst->phy_addr += cntsize;
			p_dst->vrt_addr += cntsize;

			dst.phy_addr = p_dst->phy_addr;
			dst.vrt_addr = p_dst->vrt_addr;
			src.phy_addr = uiSrcAddr;
			src.vrt_addr = pSrc->uiBSVirAddr;
			bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, uiSize, method);

		} else {
			dst.phy_addr = p_dst->phy_addr;
			dst.vrt_addr = p_dst->vrt_addr;
			src.phy_addr = uiSrcAddr;
			src.vrt_addr = pSrc->uiBSVirAddr;
			bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, uiSize, method);
		}
	}
	else {
		dst.phy_addr = p_dst->phy_addr;
		dst.vrt_addr = p_dst->vrt_addr;
		src.phy_addr = uiSrcAddr;
		src.vrt_addr = pSrc->uiBSVirAddr;
		bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, uiSize, method);
	}
#else
	dst.phy_addr = p_dst->phy_addr;
	dst.vrt_addr = p_dst->vrt_addr;
	src.phy_addr = uiSrcAddr;
	src.vrt_addr = pSrc->uiBSVirAddr;
	bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, uiSize, method);
#endif
	return uiSrcSize;
}

ER bsmux_mdat_get_need_size(UINT32 id, UINT32 *p_size)
{
	UINT32 hdrcnt;
	UINT32 idxsize = 0, mins = 0, size1;
	UINT32 front_moov = 0;
	UINT32 gps_on = 0;
	UINT32 meta_on = 0;
	#if BSMUX_TEST_NALU
	UINT32 sub_vfr = 0;
	#endif

	MDAT_FUNC("bsmux_mdat_get_need_size():begin\r\n");

	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_ON, &gps_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_META_ON, &meta_on);
	hdrcnt = bsmux_mdat_get_hdr_count();
	#if BSMUX_TEST_NALU
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	#endif

	// calc
	// ==============================
	// =     Desc                   =
	// ==============================
	size1 = BSMUX_VIDEO_DESC_SIZE;
	mins += size1;
	MDAT_FUNC("BSM:[%d] desc size 0x%X\r\n", id, size1);
	#if BSMUX_TEST_NALU
	if (sub_vfr) {
		size1 = BSMUX_VIDEO_DESC_SIZE;
		mins += size1;
		MDAT_FUNC("BSM:[%d] sub desc size 0x%X\r\n", id, size1);
	}
	#endif
	// ==============================
	// =     Thumb Buffer           =
	// ==============================
	size1 = bsmux_util_calc_reserved_size(id);
	mins += size1;
	MDAT_FUNC("BSM:[%d] thumb size 0x%X\r\n", id, size1);
	// ==============================
	// =     Front Header           =
	// ==============================
	#if 0
	size1 = bsmux_mdat_calc_header_size(id) * hdrcnt;
	mins += size1;
	MDAT_FUNC("BSM:[%d] front header size 0x%X\r\n", id, size1);
	#endif
	// ==============================
	// =     Temp  Header           =
	// ==============================
	size1 = bsmux_mdat_calc_header_size(id) * hdrcnt;
	mins += size1;
	MDAT_FUNC("BSM:[%d] temp header size 0x%X\r\n", id, size1);
	// ==============================
	// =     Back  Header           =
	// ==============================
	if (front_moov) {
		//no need
	} else {
		idxsize = bsmux_mdat_calc_entry_size(id);
		size1 = idxsize * hdrcnt;
		mins += size1;
		MDAT_FUNC("BSM:[%d] back header size 0x%X\r\n", id, size1);
	}
	// ==============================
	// =     Frame Info             =
	// ==============================
	size1 = bsmux_mdat_calc_entry_size(id);
	mins += size1;
	MDAT_FUNC("BSM:[%d] frame buf size 0x%X\r\n", id, size1);
	// ==============================
	// =     Nidx                   =
	// ==============================
	size1 = BSMUX_MAX_NIDX_BLK;
	mins += size1;
	MDAT_FUNC("BSM:[%d] nidx blk size 0x%X\r\n", id, size1);
	// ==============================
	// =     GPS                    =
	// ==============================
	if (gps_on) {
		size1 = BSMUX_GPS_MIN;
		mins += size1;
		MDAT_FUNC("BSM:[%d] gps buf size 0x%X\r\n", id, size1);
		size1 = bsmux_mdat_calc_gps_size(id, FALSE);
		mins += size1;
		MDAT_FUNC("BSM:[%d] gps entry size 0x%X\r\n", id, size1);
		size1 = bsmux_mdat_calc_gps_size(id, TRUE);
		mins += size1;
		MDAT_FUNC("BSM:[%d] gps tag size 0x%X\r\n", id, size1);
	}
	// ==============================
	// =     META                   =
	// ==============================
	if (meta_on) {
		size1 = bsmux_mdat_calc_meta_size(id, FALSE);
		mins += size1;
		MDAT_FUNC("BSM:[%d] meta entry size 0x%X\r\n", id, size1);
	}
	// ==============================
	// =     NidxQ  Buf             =
	// ==============================
	size1 = ALIGN_CEIL_16(bsmux_mdat_nidx_getsize(id));
	mins += size1;
	MDAT_FUNC("BSM:[%d] nidxq size 0x%X\r\n", id, size1);

	MDAT_FUNC("BSM:[%d] total mdat size = 0x%X\r\n", id, mins);
	*p_size = mins;
	return E_OK;
}

ER bsmux_mdat_set_need_size(UINT32 id, UINT32 addr, UINT32 *p_size)
{
	UINT32 vfr = 0, asr = 0, audcodec = 0, hdr_count = 0, mid = 0;
	UINT32 sub_vfr = 0;
	UINT32 front_moov = 0;
	UINT32 gps_on = 0;
	UINT32 meta_on = 0, meta_num = 0;
	UINT32 size = 0, size1 = 0;
	UINT32 alloc_addr = 0, need_size = 0;
	ER ret;

	// get need size
	bsmux_mdat_get_need_size(id, &size);
	size1 = size;
	MDAT_FUNC("BSM:[%d] total mdat size = 0x%X\r\n", id, size);

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &asr);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CODECTYPE, &audcodec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_ON, &gps_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_META_ON, &meta_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_META_NUM, &meta_num);
	hdr_count = bsmux_mdat_get_hdr_count();

	// ==============================
	// =     Frame Info             =
	// ==============================
	need_size = bsmux_mdat_calc_entry_size(id);
	if (size < need_size) {
		DBG_ERR("[%d] set FrameBuf range error (%d < %d)\r\n", id, size, need_size);
		return E_NOMEM;
	}
	alloc_addr = addr;
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_FRAMEBUF_ADDR, alloc_addr);
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_FRAMEBUF_SIZE, need_size);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUDIOTYPE, audcodec, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_SAMPLERATE, asr, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_VID_FR, vfr, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_VID_FR, sub_vfr, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_FRAMEINFOBUF, alloc_addr, need_size, id);
	addr += need_size;
	size -= need_size;
	MDAT_FUNC("BSM:[%d] FrameBuf addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);

	// ==============================
	// =     Desc                   =
	// ==============================
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
	MDAT_FUNC("BSM:[%d] Desc addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
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
		MDAT_FUNC("BSM:[%d] Sub Desc addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	}
	#endif

	// ==============================
	// =     Thumb Buffer           =
	// ==============================
	need_size = bsmux_util_calc_reserved_size(id);
	if (size < need_size) {
		DBG_ERR("[%d] set ThumbBuf range error (%d < %d)\r\n", id, size, need_size);
		return E_NOMEM;
	}
	alloc_addr = addr;
	bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_ADDR, alloc_addr);
	bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_SIZE, 0);
	addr += need_size;
	size -= need_size;
	MDAT_FUNC("BSM:[%d] ThumbBuf addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);

	// ==============================
	// =     Front Header           =
	// ==============================
	#if 0
	need_size = bsmux_mdat_calc_header_size(id);
	if (size < need_size * hdr_count) {
		DBG_ERR("[%d] set FrontHDR range error (%d < %d)\r\n", id, size, need_size * hdr_count);
		return E_NOMEM;
	}
	alloc_addr = addr;
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_FRONTHDR_ADDR, alloc_addr);
	bsmux_mdat_set_hdr_mem(BSMUX_BS2CARD_FRONT, id, alloc_addr, need_size);
	addr += need_size * hdr_count;
	size -= need_size * hdr_count;
	MDAT_FUNC("BSM:[%d] FrontHDR addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size * hdr_count);
	#endif

	// ==============================
	// =     Temp  Header           =
	// ==============================
	need_size = bsmux_mdat_calc_header_size(id);
	if (size < need_size * hdr_count) {
		DBG_ERR("[%d] set TempHDR range error (%d < %d)\r\n", id, size, need_size * hdr_count);
		return E_NOMEM;
	}
	alloc_addr = addr;
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, alloc_addr);
	bsmux_mdat_set_hdr_mem(BSMUX_HDRMEM_TEMP, id, alloc_addr, need_size);
	if (front_moov) {
		bsmux_util_set_param(id, BSMUXER_PARAM_MOOV_ADDR, addr);
		bsmux_util_set_param(id, BSMUXER_PARAM_MOOV_SIZE, need_size);
	}
	addr += need_size * hdr_count;
	size -= need_size * hdr_count;
	MDAT_FUNC("BSM:[%d] TempHDR addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size * hdr_count);

	// ==============================
	// =     Back  Header           =
	// ==============================
	if (front_moov) {
		//no need
	} else {
		need_size = bsmux_mdat_calc_entry_size(id);
		if (size < need_size * hdr_count) {
			DBG_ERR("[%d] set BackHDR range error (%d < %d)\r\n", id, size, need_size * hdr_count);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_mdat_set_max_entrytime(id, need_size, vfr);
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_BACKHDR_ADDR, alloc_addr);
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_BACKHDR_SIZE, need_size);
		bsmux_mdat_set_hdr_mem(BSMUX_HDRMEM_BACK, id, alloc_addr, need_size);
		addr += need_size * hdr_count;
		size -= need_size * hdr_count;
		MDAT_FUNC("BSM:[%d] BackHDR addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size * hdr_count);
	}

	// ==============================
	// =     GPS                    =
	// ==============================
	bsmux_util_get_mid(id, &mid);
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
		MDAT_FUNC("BSM:[%d] GPS addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);

		need_size = bsmux_mdat_calc_gps_size(id, FALSE);
		if (size < need_size) {
			DBG_ERR("[%d] set GPS entry range error (%d < %d)\r\n", id, size, need_size);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_GPSBUFFER, alloc_addr, need_size, mid);
		addr += need_size;
		size -= need_size;
		MDAT_FUNC("BSM:[%d] GPS entry addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);

		need_size = bsmux_mdat_calc_gps_size(id, TRUE);
		if (size < need_size) {
			DBG_ERR("[%d] set GPS tag range error (%d < %d)\r\n", id, size, need_size);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_GPSTAGBUFFER, alloc_addr, need_size, mid);
		addr += need_size;
		size -= need_size;
		MDAT_FUNC("BSM:[%d] GPS tag addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	} else {
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_GSP_ADDR, 0);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_GPSTAGBUFFER, 0, 0, mid);
	}

	// ==============================
	// =     META                   =
	// ==============================
	bsmux_util_get_mid(id, &mid);
	if (meta_on) {
		need_size = bsmux_mdat_calc_meta_size(id, FALSE);
		if (size < need_size) {
			DBG_ERR("[%d] set meta entry range error (%d < %d)\r\n", id, size, need_size);
			return E_NOMEM;
		}
		alloc_addr = addr;
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_METATAGBUFFER, meta_num, 0, mid);
		{
			BSMUX_REC_METADATA meta = {0};
			UINT32 meta_idx;
			for (meta_idx = 0; meta_idx < meta_num; meta_idx++) {
				meta.meta_index = meta_idx;
				bsmux_util_get_param(id, BSMUXER_PARAM_META_DATA, (UINT32 *)(&meta));
				bsmux_util_set_minfo(MEDIAWRITE_SETINFO_METASIGN, meta.meta_sign, meta_idx, mid);
			}
		}
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_METABUFFER, alloc_addr, need_size, mid);
		addr += need_size;
		size -= need_size;
		MDAT_FUNC("BSM:[%d] meta entry addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	} else {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_METATAGBUFFER, 0, 0, mid);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_METABUFFER, 0, 0, mid);
	}

	// ==============================
	// =     Nidx                   =
	// ==============================
	need_size = BSMUX_MAX_NIDX_BLK;
	if (size < need_size) {
		DBG_ERR("[%d] set Nidx range error (%d < %d)\r\n", id, size, need_size);
		return E_NOMEM;
	}
	alloc_addr = addr;
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_NIDX_ADDR, alloc_addr);
	addr += need_size;
	size -= need_size;
	MDAT_FUNC("BSM:[%d] Nidx addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);

	// ==============================
	// =     NidxQ  Buf             =
	// ==============================
	need_size = ALIGN_CEIL_16(bsmux_mdat_nidx_getsize(id));
	if (size < need_size) {
		DBG_ERR("[%d] set NidxQBuf range error (%d < %d)\r\n", id, size, need_size);
		return E_NOMEM;
	}
	alloc_addr = addr;
	addr += need_size;
	size -= need_size;
	MDAT_FUNC("BSM:[%d] NidxQBuf addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	//init q
	ret = bsmux_mdat_nidx_init(id, alloc_addr, need_size);
	if (ret != E_OK) {
		DBG_ERR("bsmux[%d] nidx init fail\r\n", id);
		return E_SYS;
	}

	*p_size = (size1 - size);

	return E_OK;
}

ER bsmux_mdat_clean(UINT32 id)
{
	//reset
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, 0);
	return E_OK;
}

#if BSMUX_TEST_MULTI_TILE
ER bsmux_mdat_update_vidsize(UINT32 id, UINT32 addr, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 size = pBsq->uiBSSize;
	UINT32 updateaddr = 0;
	UINT32 updatesize = 0;
	UINT32 type;
	UINT32 nalu_num = 0;
	BOOL bH265DataFound = FALSE;
	UINT32 uiTOftMemAddr = pBsq->uiTOftMemAddr;
	UINT32 uiTOftVirAddr = pBsq->uiTOftVirAddr;

	if (!bsmux_util_get_buf_pa(id, addr)) {
		DBG_ERR("[%d] check addr invalid\r\n", id);
		return E_NOEXS;
	}

	if (!size) {
		DBG_ERR("[%d] check size zero\r\n", id);
		return E_NOEXS;
	}

	if (pBsq->uiType == BSMUX_TYPE_VIDEO) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &type);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_NALUNUM, &nalu_num);
	} else if (pBsq->uiType == BSMUX_TYPE_SUBVD) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &type);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_NALUNUM, &nalu_num);
	}

	//--- handle desc and update info
	if (type == MEDIAVIDENC_H264) {
		// no multi-tile
		updateaddr = addr;
		updatesize = size - 4;
		bH265DataFound = FALSE;

	} else if (type == MEDIAVIDENC_H265) {
		if (pBsq->uiIsKey) { // I-frame: desc
			UINT32 vps = 0, sps = 0, pps = 0;
			UINT32 shift = 0;
			UINT32 i;
			UINT8 *pIn;

			if (pBsq->uiType == BSMUX_TYPE_VIDEO) {
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_VPSSIZE, &vps);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SPSSIZE, &sps);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_PPSSIZE, &pps);
				DBG_IND("[%d] main(%d, %d, %d, %d)\r\n", id, nalu_num, vps, sps, pps);
			}
			else if (pBsq->uiType == BSMUX_TYPE_SUBVD) {
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VPSSIZE, &vps);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_SPSSIZE, &sps);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_PPSSIZE, &pps);
				DBG_IND("[%d] sub(%d, %d, %d, %d)\r\n", id, nalu_num, vps, sps, pps);
			}

			// put desc * tile num
			updateaddr = addr;
			for (i = 0; i < nalu_num; i++) {

				//vps
				pIn = (UINT8 *)updateaddr;
				if ((*(pIn) == 0) && (*(pIn + 1) == 0) && (*(pIn + 2) == 0) && (*(pIn + 3) == 1)) {
					bsmux_update_vidsize(updateaddr, (vps-4));
					updateaddr += vps;
				} else {
					DBG_ERR("[%d][%d] vps error\r\n", id, pBsq->uiType);
					bsmux_util_dump_buffer((char *)updateaddr, 8);
				}
				//sps
				pIn = (UINT8 *)updateaddr;
				if ((*(pIn) == 0) && (*(pIn + 1) == 0) && (*(pIn + 2) == 0) && (*(pIn + 3) == 1)) {
					bsmux_update_vidsize(updateaddr, (sps-4));
					updateaddr += sps;
				} else {
					DBG_ERR("[%d][%d] sps error\r\n", id, pBsq->uiType);
					bsmux_util_dump_buffer((char *)updateaddr, 8);
				}
				//pps
				pIn = (UINT8 *)updateaddr;
				if ((*(pIn) == 0) && (*(pIn + 1) == 0) && (*(pIn + 2) == 0) && (*(pIn + 3) == 1)) {
					bsmux_update_vidsize(updateaddr, (pps-4));
					updateaddr += pps;
				} else {
					DBG_ERR("[%d][%d] pps error\r\n", id, pBsq->uiType);
					bsmux_util_dump_buffer((char *)updateaddr, 8);
				}
			}
			shift = (vps + sps + pps) * nalu_num;
			updateaddr = addr + shift;
			updatesize = size - shift - 4;

			pIn = (UINT8 *)updateaddr;
			if ((*(pIn) == 0) && (*(pIn + 1) == 0) && (*(pIn + 2) == 0) && (*(pIn + 3) == 1)) {

			} else {
				DBG_ERR("[%d][%d] bs error\r\n", id, pBsq->uiType);
				bsmux_util_dump_buffer((char *)updateaddr, 8);
			}

		} else { // P-frame: no desc
			updateaddr = addr;
			updatesize = size - 4;
		}

		bH265DataFound = TRUE;
	}

	if (!updateaddr) {
		DBG_ERR("[%d] updateaddr zero\r\n", id);
		return E_SYS;
	}

	//--- handle bs size
	if (bH265DataFound)
	{ // multi-tile
		HD_BUFINFO *nalu_data;
		UINT32 cnt = 0;
		UINT32 num = 0;
		UINT32 nalu_addr = 0;
		if (uiTOftMemAddr && uiTOftVirAddr) {
			if (uiTOftVirAddr == bsmux_util_get_buf_va(id, uiTOftMemAddr)) {
				num = *(UINT32*)uiTOftVirAddr;
				if (num != nalu_num) {
					DBG_ERR("[%d][%d] nalu(%d)(%d) error\r\n", id, pBsq->uiType, num, nalu_num);
					num = nalu_num;
				}
				if (num > 1) {
					nalu_data = (HD_BUFINFO *)(uiTOftVirAddr+sizeof(UINT32));
					nalu_addr = updateaddr;
					for (cnt = 0; cnt < num; cnt++) {
						if ((nalu_data[cnt].phy_addr >= pBsq->uiBSMemAddr) &&
							((nalu_data[cnt].phy_addr + nalu_data[cnt].buf_size) <= (pBsq->uiBSMemAddr + pBsq->uiBSSize))) {
							UINT8 *pIn = (UINT8 *)nalu_addr;
							if ((*(pIn) == 0) && (*(pIn + 1) == 0) && (*(pIn + 2) == 0) && (*(pIn + 3) == 1)) {
								bsmux_update_vidsize(nalu_addr, (nalu_data[cnt].buf_size - 4));
								nalu_addr += nalu_data[cnt].buf_size;
							} else if ((*(pIn) == 0) && (*(pIn + 1) == 0) && (*(pIn + 2) == 1)) {
								DBG_ERR("[%d][%d] long start code error\r\n", id, pBsq->uiType);
							} else {
								DBG_ERR("[%d][%d] start code error\r\n", id, pBsq->uiType);
							}
						} else {
							DBG_ERR("[%d][%d] tile error\r\n", id, pBsq->uiType);
						}
					}
				} else {
					bsmux_update_vidsize(updateaddr, updatesize);
				}
			} else {
				DBG_ERR("[%d] uiTOftVirAddr not match\r\n", id, uiTOftVirAddr);
			}
		} else {
			bsmux_update_vidsize(updateaddr, updatesize);
		}
	}
	else
	{ // normal
		bsmux_update_vidsize(updateaddr, updatesize);
	}

	return E_OK;
}
#else
ER bsmux_mdat_update_vidsize(UINT32 id, UINT32 addr, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 size = pBsq->uiBSSize;
	UINT32 updateaddr = 0;
	UINT32 updatesize = 0;
	UINT32 i = 0;
	UINT32 type;
	BOOL bH265DataFound = FALSE;
	UINT32 uiSeekSize[8] = {0}, uiSeekCount = 0, shift = 0;
	UINT8 *pSeekPos[8];

#if BSMUX_TEST_MULTI_TILE
	UINT32 uiTOftMemAddr = pBsq->uiTOftMemAddr;
	UINT32 uiTOftVirAddr = pBsq->uiTOftVirAddr;
#endif

	if (!bsmux_util_get_buf_pa(id, addr)) {
		DBG_ERR("[%d] check addr invalid\r\n", id);
		return E_NOEXS;
	}

	if (!size) {
		DBG_ERR("[%d] check size zero\r\n", id);
		return E_NOEXS;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &type);

	if (type == MEDIAVIDENC_H264) {
		updateaddr = addr;
		updatesize = size - 4;
		bH265DataFound = FALSE;

	} else if (type == MEDIAVIDENC_H265) {
		UINT8 *pIn;
		UINT32 len = 0x100;
		pIn = (UINT8 *)addr;
		i = 0;
		while (i < len) {
			// check start code (00, 00, 00, 01)
			if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
				pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
				i += 4;
				uiSeekCount++; // one start code was found
				bH265DataFound = TRUE;
			} else {
				i++;
				if (bH265DataFound) {
					uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
				}
			}
		}
	}

	if (bH265DataFound) {
		for (i = 0; i < (uiSeekCount - 1); i++) {
			if (!pSeekPos[i]) {
				DBG_ERR("[%d] pSeekPos[%d] zero\r\n", id, i);
				continue;
			}
			bsmux_update_vidsize((UINT32)pSeekPos[i], uiSeekSize[i]);
		}
		shift = (UINT32)(pSeekPos[uiSeekCount - 1]) - addr;
		updateaddr = (UINT32)(pSeekPos[uiSeekCount - 1]);
		updatesize = size - shift - 4;
	}

	if (!updateaddr) {
		DBG_ERR("[%d] updateaddr zero\r\n", id);
		return E_SYS;
	}

#if BSMUX_TEST_MULTI_TILE
	{
		HD_BUFINFO *nalu_data;
		UINT32 cnt = 0;
		UINT32 num = 0;
		UINT32 nalu_addr = 0;
		if (uiTOftMemAddr && uiTOftVirAddr) {
			if (uiTOftVirAddr == bsmux_util_get_buf_va(id, uiTOftMemAddr)) {
				num = *(UINT32*)uiTOftVirAddr;
				if (num > 1) {
					nalu_data = (HD_BUFINFO *)(uiTOftVirAddr+sizeof(UINT32));
					nalu_addr = updateaddr;
					for (cnt = 0; cnt < num; cnt++) {
						bsmux_update_vidsize(nalu_addr, (nalu_data[cnt].buf_size - 4));
						nalu_addr += nalu_data[cnt].buf_size;
					}
				} else {
					bsmux_update_vidsize(updateaddr, updatesize);
				}
			} else {
				DBG_ERR("[%d] uiTOftVirAddr not match\r\n", id, uiTOftVirAddr);
			}
		} else {
			bsmux_update_vidsize(updateaddr, updatesize);
		}
	}
#else
	bsmux_update_vidsize(updateaddr, updatesize);
#endif

	return E_OK;
}
#endif

ER bsmux_mdat_release_buf(void *pBuf)
{
	BSMUXER_FILE_BUF *p_buf = (BSMUXER_FILE_BUF *)pBuf;
	UINT32 op = p_buf->fileop;
	UINT32 id = p_buf->pathID;
	UINT32 addr = 0;
	UINT32 cut_blk = 0;

	if (bsmux_util_is_null_obj((void *)p_buf)) {
		DBG_ERR("[%d] buf null\r\n", id);
		return E_PAR;
	}

	if (op & BSMUXER_FOP_CREATE) {
#if 0
		bsmux_mdat_rel_hdr_mem(BSMUX_HDRMEM_FRONT, id, p_buf->addr);
#endif
	}
	if (op & BSMUXER_FOP_SEEK_WRITE) {
		if (op & BSMUXER_FOP_CLOSE) {
			bsmux_mdat_rel_hdr_mem(BSMUX_HDRMEM_TEMP, id, p_buf->addr);
			bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_REAL2CARD, p_buf->addr);
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_CUTBLK, &cut_blk);
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_CUTBLK, (cut_blk-1));
		}
	} else {
		UINT32 buf_addr = 0, buf_end = 0;
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &buf_addr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &buf_end);
		if ((p_buf->addr >= buf_addr) && (p_buf->addr <= (buf_end))) {
			// mdat buffer address
			addr = p_buf->addr + p_buf->size;
			if (addr == buf_end) {
				DBG_IND("BSM:[%d] addr(0x%X) achieve max(0x%X)\r\n", id, addr, buf_end);
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_REAL2CARD, buf_addr);
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, buf_addr);
			} else {
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_REAL2CARD, addr);
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, addr);
			}
			#if 0
			{
				UINT32 nowaddr = 0, last2card = 0, real2card = 0;
				bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &nowaddr);
				bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
				bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
				DBG_DUMP("[%d] release-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id,
					buf_addr, nowaddr, last2card, real2card, buf_end);
			}
			#endif

			// critical section: lock
			bsmux_buffer_lock();
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_USEDBLOCK, &cut_blk);
			cut_blk--;
			bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_USEDBLOCK, cut_blk);
			bsmux_buffer_unlock();
			// critical section: unlock
			DBG_IND("%s: id(%d) USEDBLOCK[%d]\r\n", __func__, id, cut_blk);
		} else {
			// back header address
			bsmux_mdat_rel_hdr_mem(BSMUX_HDRMEM_BACK, id, p_buf->addr);
		}
	}
	DBG_IND(">>> bsmux_util_release_buf 0x%X\r\n", p_buf);
	if (g_bsmux_debug_msg) {
		bsmux_ctrl_mem_det_range(id);
	}
	return E_OK;
}

ER bsmux_mdat_gps_pad(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO bsq = {0};
	BSMUX_GPS_INFO *ptr;
	UINT32 outsize;
	UINT32 outGpsMem, outGpsAddr, GPS_addr, GPS_size;
	UINT32 gps_queue = 0, gps_shift = 16;

	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_QUEUE, &gps_queue);
	if (gps_queue) {
		//use user buffer
		outGpsAddr = pBsq->uiBSVirAddr - gps_shift; //va shift reserved size
		outGpsMem = pBsq->uiBSMemAddr - gps_shift; //pa shift reserved size
		outsize = pBsq->uiBSSize + gps_shift;
	} else {
		//use internal buffer
		bsmux_util_get_param(id, BSMUXER_PARAM_HDR_GSP_ADDR, &outGpsAddr); //va
		if (outGpsAddr == 0) {
			DBG_ERR("[%d] mdat outGpsAddr is ZERO!!!!\r\n", id);
			return E_SYS;
		}
		outGpsMem = bsmux_util_get_buf_pa(id, outGpsAddr); //pa
		outsize = BSMUX_GPS_MIN;
	}
	GPS_addr = pBsq->uiBSVirAddr; //va
	GPS_size = pBsq->uiBSSize;
	if (GPS_size == 0) {
		return E_NOEXS;
	}
	ptr = &g_mdat_gps_info[id];
	ptr->outGpsAddr = outGpsAddr; //va
	ptr->GPS_addr   = GPS_addr; //va
	ptr->GPS_size   = GPS_size;
	bsmux_mdat_gps_make(id, 0);

	bsq.uiBSMemAddr = outGpsMem; //pa
	if (!bsq.uiBSMemAddr) {
		DBG_ERR("[%d] get gps addr zero\r\n", id);
	}
	bsq.uiBSVirAddr = outGpsAddr; //va
	bsq.uiBSSize = outsize;
	bsq.uiType = BSMUX_TYPE_GPSIN;
	bsq.uiIsKey = 0;
	bsq.uiTimeStamp = hwclock_get_longcounter();

	MDAT_FUNC("[%d] add gps (pa=0x%x va=0x%x size=0x%x)\r\n", id,
		pBsq->uiBSMemAddr, pBsq->uiBSVirAddr, pBsq->uiBSSize);

	bsmux_ctrl_bs_enqueue(id, (void *)&bsq);
	bsmux_tsk_bs_in();

	return E_OK;
}

ER bsmux_mdat_add_thumb(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 uiThumbAddr = 0, uiThumbSize = 0;
	UINT32 uiAllocThumbAddr = 0;
	UINT32 mux_method = 0;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};

	uiThumbAddr = pBsq->uiBSVirAddr; //va
	uiThumbSize = pBsq->uiBSSize;

	bsmux_util_get_param(id, BSMUXER_PARAM_MUXMETHOD, &mux_method);
	bsmux_util_get_param(id, BSMUXER_PARAM_THUMB_ADDR, &uiAllocThumbAddr);
	dst.phy_addr = bsmux_util_get_buf_pa(id, uiAllocThumbAddr);
	dst.vrt_addr = uiAllocThumbAddr;
	src.phy_addr = pBsq->uiBSMemAddr;
	src.vrt_addr = pBsq->uiBSVirAddr;
	bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, uiThumbSize, mux_method); //pa
	uiThumbAddr = uiAllocThumbAddr;
	bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_ADDR, uiThumbAddr);
	bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_SIZE, uiThumbSize);
	pBsq->uiTimeStamp = hwclock_get_longcounter();

	DBG_DUMP("[%d] add thumbnail (pa=0x%x va=0x%x size=0x%x)\r\n", id,
		pBsq->uiBSMemAddr, uiThumbAddr, uiThumbSize);

	bsmux_ctrl_bs_enqueue(id, p_bsq);
	bsmux_tsk_bs_in();

	return E_OK;
}

ER bsmux_mdat_add_last(UINT32 id)
{
	DBG_IND("unsupported\r\n");
	return E_OK;
}

ER bsmux_mdat_put_last(UINT32 id)
{
	return E_OK;
}

ER bsmux_mdat_add_meta(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO bsq = {0};
	UINT32 user_size;
	UINT32 user_pa = 0, user_va = 0;
	UINT32 meta_queue = 1, meta_shift = 0;

	//bsmux_util_get_param(id, BSMUXER_PARAM_META_QUEUE, &meta_queue);
	if (meta_queue) {
		//use user buffer
		user_va = pBsq->uiBSVirAddr - meta_shift; //va shift reserved size
		user_pa = pBsq->uiBSMemAddr - meta_shift; //pa shift reserved size
		user_size = pBsq->uiBSSize + meta_shift;
	} else {
		DBG_ERR("should use user buffer\r\n");
		return E_PAR;
	}

	bsq.uiBSMemAddr = user_pa; //pa
	if (!bsq.uiBSMemAddr) {
		DBG_ERR("[%d] get meta addr zero\r\n", id);
	}
	bsq.uiBSVirAddr = user_va; //va
	bsq.uiBSSize = user_size;
	bsq.uiType = BSMUX_TYPE_MTAIN;
	bsq.uiIsKey = 0;
	bsq.uibufid = pBsq->uibufid; //as meta index
	bsq.uiTimeStamp = hwclock_get_longcounter();

	//DBG_DUMP("[%d] add meta (pa=0x%x va=0x%x size=0x%x)\r\n", id,
	//	pBsq->uiBSMemAddr, pBsq->uiBSVirAddr, pBsq->uiBSSize);

	bsmux_ctrl_bs_enqueue(id, (void *)&bsq);
	bsmux_tsk_bs_in();

	return E_OK;
}

ER bsmux_mdat_save_entry(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = NULL;
	MOV_Ientry thisJpg = {0};
	UINT32 nowFN = 0;
	UINT32 filetype = 0;
	UINT32 copyTotalCount = 0, copyPos = 0;
	UINT32 vfr = 0;
	UINT32 uiBSSize, uiType, uiIsKey, uibufid;
	UINT32 front_moov = 0;

	if (bsmux_util_is_null_obj(p_bsq)) {
		DBG_DUMP("[%d] pBsq zero\r\n", id);
		return E_PAR;
	}

	pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	if (bsmux_util_is_null_obj((void *)pBsq)) {
		DBG_DUMP("[%d] pBsq zero\r\n", id);
		return E_PAR;
	}

	if ((pBsq->uiBSMemAddr == 0) || (pBsq->uiBSVirAddr == 0)) {
		DBG_DUMP("[%d] Addr zero\r\n", id);
		return E_PAR;
	}

	if (pBsq->uiBSSize == 0) {
		DBG_DUMP("[%d] BSSize zero\r\n", id);
		return E_PAR;
	}

	uiBSSize = pBsq->uiBSSize;
	uiType = pBsq->uiType;
	uiIsKey = pBsq->uiIsKey;
	uibufid = pBsq->uibufid;

	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTAL, &copyTotalCount);
	copyTotalCount += 1;
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTAL, copyTotalCount);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_COPYPOS, &copyPos);

	if (uiType == BSMUX_TYPE_VIDEO) {
		UINT32 vidCopyCount;
		UINT32 variable_rate;
		UINT32 recformat = 0, playvfr = 0;
		/* GOLFSHOT: need to update duration to config playback speed */
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_RECFORMAT, &recformat);
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PLAYFRAMERATE, &playvfr);
		if (recformat == MEDIAREC_GOLFSHOT) {
			variable_rate = playvfr;
		}
		else
		{
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_VAR_VFR, &variable_rate);
			if (uiIsKey) {
				UINT32 user_variable_rate, mux_variable_rate;
				user_variable_rate = ((variable_rate) >> 16);
				mux_variable_rate = ((variable_rate) & 0xffff);
				if ((user_variable_rate) && (mux_variable_rate != user_variable_rate)) {
					MDAT_FUNC("[%d] user_variable_rate=%d mux_variable_rate=%d\r\n", id, user_variable_rate, mux_variable_rate);
					variable_rate = user_variable_rate;
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_VAR_VFR, variable_rate);
					bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_VAR_RATE, TRUE);
				} else {
					variable_rate = mux_variable_rate;
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_VAR_VFR, variable_rate);
				}
			}
			variable_rate = ((variable_rate) & 0xffff);
		}
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vidCopyCount);

		thisJpg.size    = uiBSSize;
		nowFN           = vidCopyCount;
		if (uiIsKey == 1) {
			thisJpg.key_frame = 1;
		} else {
			thisJpg.key_frame = 0;
		}
		thisJpg.updated = 0;
		thisJpg.pos     = (UINT64)copyPos;
		thisJpg.rev     = (UINT8)variable_rate;  ///< (internal) variable rate usage
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MOVVIDEOENTRY, nowFN, (UINT32)&thisJpg, id);

		vidCopyCount += 1;
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALVF, vidCopyCount);

		copyPos += bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, copyPos);

		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		if ((vidCopyCount % vfr) == 0) {
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_NOWSEC, (vidCopyCount/vfr));
			if (id == 0) {
				MDAT_FUNC("sec=%d\r\n", (vidCopyCount/vfr));
			}
		}
	}
	else if (uiType == BSMUX_TYPE_SUBVD) {
		UINT32 subvidCopyCount;
		UINT32 variable_rate;
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &variable_rate);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &subvidCopyCount);

		thisJpg.size    = uiBSSize;
		nowFN           = subvidCopyCount;
		if (uiIsKey == 1) {
			thisJpg.key_frame = 1;
		} else {
			thisJpg.key_frame = 0;
		}
		thisJpg.updated = 0;
		thisJpg.pos     = (UINT64)copyPos;
		thisJpg.rev     = (UINT8)variable_rate;  ///< (internal) variable rate usage
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_VIDEOENTRY, nowFN, (UINT32)&thisJpg, id);

		subvidCopyCount += 1;
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_SUBVF, subvidCopyCount);

		copyPos += bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, copyPos);
	}
	else if (uiType == BSMUX_TYPE_AUDIO) {
		UINT32 audCopyCount;
		UINT32 variable_rate;
		bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &variable_rate);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &audCopyCount);

		thisJpg.size    = uiBSSize;
		nowFN           = audCopyCount;
		thisJpg.pos     = (UINT64)copyPos;
		thisJpg.rev     = (UINT8)(variable_rate/100);  ///< (internal) variable rate usage
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
		if ((filetype == MEDIA_FILEFORMAT_MOV) || (filetype == MEDIA_FILEFORMAT_MP4)) {
			bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MOVAUDIOENTRY, nowFN, (UINT32)&thisJpg, id);
		}

		audCopyCount += 1;
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALAF, audCopyCount);

		copyPos += bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, copyPos);
	}
	else if (uiType == BSMUX_TYPE_GPSIN) {
		UINT32 gpsCopyCount;
		UINT32 variable_rate;
		bsmux_util_get_param(id, BSMUXER_PARAM_GPS_RATE, &variable_rate);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALGPS, &gpsCopyCount);

		thisJpg.pos     = (UINT64)copyPos;
		thisJpg.size    = uiBSSize;
		nowFN           = gpsCopyCount;
		thisJpg.key_frame = 0;
		thisJpg.updated = 0;
		thisJpg.rev     = (UINT8)variable_rate;  ///< (internal) variable rate usage
		bsmux_util_set_minfo(MEDIAWRITE_SETINTO_MOVGPSENTRY, nowFN, (UINT32)&thisJpg, id);

		gpsCopyCount += 1;
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALGPS, gpsCopyCount);

		copyPos += bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, copyPos);
	}
	else if (uiType == BSMUX_TYPE_NIDXT) {
		bsmux_mdat_nidx_make(id, copyPos, uibufid);
		copyPos += bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, copyPos);
	}
	else if (uiType == BSMUX_TYPE_THUMB) {
		bsmux_util_make_header(id);
		bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
		if (front_moov) {
			BSMUX_MSG("save_entry: [%d] front moov flow\r\n", id);
			bsmux_util_make_moov(id, 1);
		}
		// no need to update offset since the data is placed on the container
	}
	else if (uiType == BSMUX_TYPE_MTAIN) {
		UINT32 vidCopyCount;
		UINT32 meta_copy_count;
		UINT32 variable_rate;
		UINT32 fileinfo_type = BSMUX_FILEINFO_TOTALMETA + uibufid;
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &variable_rate); //temp as vfr
		bsmux_ctrl_bs_get_fileinfo(id, fileinfo_type, &meta_copy_count);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vidCopyCount);

		thisJpg.pos     = (UINT64)copyPos;
		thisJpg.size    = uiBSSize;
		nowFN           = meta_copy_count;
		thisJpg.key_frame = (UINT16)(vidCopyCount - 1);  //todo: use to store video entry index
		thisJpg.updated = (UINT8)uibufid; //todo: use to store meta index
		thisJpg.rev     = (UINT8)variable_rate;  ///< (internal) variable rate usage
		bsmux_util_set_minfo(MEDIAWRITE_SETINTO_METAENTRY, nowFN, (UINT32)&thisJpg, id);

		meta_copy_count += 1;
		bsmux_ctrl_bs_set_fileinfo(id, fileinfo_type, meta_copy_count);

		copyPos += bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, copyPos);
	}

	return E_OK;
}

ER bsmux_mdat_nidx_pad(UINT32 id)
{
	BSMUX_SAVEQ_BS_INFO bsq = {0};
	MDAT_NIDX_INFO *pnidx = NULL;
	UINT32 tmpvalue, nidxok_vfn, nidxok_afn;
	UINT32 outputNidxAddr, update_vinfo = 0, thisV_fn, thisA_fn, output_nidxnum = 0;
	UINT32 outsize;
	UINT32 nidxnum = 0;
	UINT32 nowVfn = 0, nowAfn = 0;
	UINT32 nidxok_sub_vfn = 0, thisSubV_fn = 0, nowSubVfn = 0;

	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &nowVfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &nowAfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &nowSubVfn);
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_NIDX_ADDR, &outputNidxAddr);

	if (nowVfn == 0) {
		bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_VFN, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_AFN, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_SUB_VFN, 0);
		nowAfn = 0;
		nowSubVfn = 0;
		update_vinfo = 1;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_NIDX_VFN, &nidxok_vfn);
	thisV_fn = nowVfn - nidxok_vfn;
	bsmux_util_get_param(id, BSMUXER_PARAM_NIDX_AFN, &nidxok_afn);
	thisA_fn = nowAfn - nidxok_afn;
	bsmux_util_get_param(id, BSMUXER_PARAM_NIDX_SUB_VFN, &nidxok_sub_vfn);
	thisSubV_fn = nowSubVfn - nidxok_sub_vfn;

	//set nidx info
	{
		pnidx = (MDAT_NIDX_INFO *)g_mdat_nidx_queue[id];
		nidxnum = bsmux_mdat_nidx_getone(id);
		pnidx += nidxnum;
		if ((pnidx->vfr == 0) || (update_vinfo == 1)) {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &tmpvalue);
			pnidx->vfr        = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_WIDTH, &tmpvalue);
			pnidx->v_wid      = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_HEIGHT, &tmpvalue);
			pnidx->v_hei      = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &tmpvalue);
			pnidx->v_codec    = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &tmpvalue);
			pnidx->a_sr       = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CHS, &tmpvalue);
			pnidx->a_chs      = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CODECTYPE, &tmpvalue);
			pnidx->a_codec    = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &tmpvalue);
			pnidx->h264DescAddr = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCSIZE, &tmpvalue);
			pnidx->h264DescSize = tmpvalue;
			pnidx->lastAfn    = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &tmpvalue);
			pnidx->sub_vfr    = tmpvalue;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_HEIGHT, &tmpvalue);
		pnidx->v_hei      = tmpvalue;
		if (nidxok_vfn == 0) {
			pnidx->lastNidxPos = 0;
			pnidx->lastNidxPos_co64 = 0;
		} else {
			pnidx->lastNidxPos = pnidx->nowFoft;
			pnidx->lastNidxPos_co64 = pnidx->nowFoft_co64;
		}
		pnidx->nowVfn     = nidxok_vfn;
		pnidx->thisObjId  = id;
		pnidx->outNidxAddr = outputNidxAddr;
		pnidx->thisV_fn   = thisV_fn;
		pnidx->nowAfn     = nidxok_afn;
		pnidx->thisA_fn   = thisA_fn;
		pnidx->nowSubVfn   = nidxok_sub_vfn;
		pnidx->thisSubV_fn = thisSubV_fn;
		output_nidxnum = nidxnum;
		outsize = BSMUX_MAX_NIDX_BLK;
		if (pnidx->h264DescAddr == 0) {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &tmpvalue);
			pnidx->vfr        = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_WIDTH, &tmpvalue);
			pnidx->v_wid      = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_HEIGHT, &tmpvalue);
			pnidx->v_hei      = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &tmpvalue);
			pnidx->v_codec    = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &tmpvalue);
			pnidx->a_sr       = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CHS, &tmpvalue);
			pnidx->a_chs      = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CODECTYPE, &tmpvalue);
			pnidx->a_codec    = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &tmpvalue);
			pnidx->h264DescAddr = tmpvalue;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCSIZE, &tmpvalue);
			pnidx->h264DescSize = tmpvalue;
			pnidx->lastAfn    = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &tmpvalue);
			pnidx->sub_vfr    = tmpvalue;
		}
	}

	bsq.uiBSMemAddr = bsmux_util_get_buf_pa(id, outputNidxAddr);
	if (!bsq.uiBSMemAddr) {
		DBG_ERR("[%d] get nidx addr zero\r\n", id);
		return E_SYS;
	}
	bsq.uiBSVirAddr = outputNidxAddr;
	bsq.uiBSSize = outsize;
	bsq.uiType = BSMUX_TYPE_NIDXT;
	bsq.uiIsKey = 0;
	bsq.uibufid = output_nidxnum;
	bsq.uiTimeStamp = hwclock_get_longcounter();

	if (nowVfn) {
		bsmux_ctrl_bs_enqueue(id, (void *)&bsq);
		bsmux_tsk_bs_in();
	} else {
		//no add , release itself
		bsmux_mdat_nidx_clearone(id, output_nidxnum);
	}
	nidxok_vfn += thisV_fn;
	nidxok_afn += thisA_fn;
	nidxok_sub_vfn += thisSubV_fn;
	bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_VFN, nidxok_vfn);
	bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_AFN, nidxok_afn);
	bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_SUB_VFN, nidxok_sub_vfn);

	return E_OK;
}

ER bsmux_mdat_make_header(UINT32 id, void *p_maker)
{
	CONTAINERMAKER *pCMaker = (CONTAINERMAKER *)p_maker;
	MEDIAREC_HDR_OBJ *pHdrObj = NULL;
	UINT32  uiHeaderSize;
	VOS_TICK temptimeid;
	UINT32 wid, hei, asr, chs, tempfront, aud_en, filetype, makerId, dar;
	UINT32 sub_wid, sub_hei, sub_dar, sub_vfr, sub_vidcodec;
	UINT32 frontheader_2 = 0, vidcodec, vfr;
	UINT32 ver_string_addr = 0;
	UINT32 userDataAddr = 0, userDataSize = 0;
	UINT32 uiThumbAddr = 0, uiThumbSize = 0;
	UINT32 freabox_en;
	UINT32 fastput_en;
	UINT32 utc_sign = 0, utc_zone = 0;
	UINT32 thumbPut = 0;
	UINT32 front_moov = 0;
	ER returnv;

	pHdrObj = bsmux_get_hdr_obj(id);
	if (pHdrObj== NULL) {
		DBG_ERR("get hdr obj null\r\n");
		return E_SYS;
	}

	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("get maker obj null\r\n");
		return E_SYS;
	}
	makerId = pCMaker->id;

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_WIDTH, &wid);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_HEIGHT, &hei);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &asr);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CHS, &chs);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_DAR, &dar);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &vidcodec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, &tempfront);
	if (tempfront) {
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_THUMBPUT, &thumbPut);
		if (thumbPut) {
			MDAT_FUNC("[%d] already make header with thumb, return\r\n", id);
			return E_OK;
		}
		MDAT_FUNC("[%d] no thumb, make header again\r\n", id);
	} else {
		frontheader_2 = bsmux_mdat_get_hdr_mem(BSMUX_HDRMEM_TEMP, id);
		if (frontheader_2 == 0) {
			DBG_ERR("[id=%d]frontheader_2 zero\r\n", id);
			bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
			return E_SYS;
		}
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, frontheader_2);
		tempfront = frontheader_2;
		MDAT_FUNC("[%d] make header for the first time\r\n", id);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_ON, &aud_en);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
	bsmux_util_get_version(&ver_string_addr);

	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_WID_HEI, wid, hei, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_SAMPLERATE, asr, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_BITS, 16, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_CHS, chs, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MEDIAVERSION, ver_string_addr, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MP4_TEMPHDR, (UINT32)tempfront, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_DAR, dar, 0, id);
	if (aud_en == 0) {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_ENABLE, FALSE, 0, id);
		pHdrObj->bAudioEn = FALSE;
	} else {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_ENABLE, TRUE, 0, id);
		pHdrObj->bAudioEn = TRUE;
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_FREABOX, &freabox_en);
	if (freabox_en) {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_EN_FREABOX, TRUE, 0, id);
	} else {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_EN_FREABOX, FALSE, 0, id);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_UTC_SIGN, &utc_sign); //1:negative | 0:positive
	bsmux_util_get_param(id, BSMUXER_PARAM_UTC_ZONE, &utc_zone);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_UTC_TIMEZONE, utc_zone, utc_sign, id);

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	if (sub_vfr) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_WIDTH, &sub_wid);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_HEIGHT, &sub_hei);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &sub_vidcodec);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DAR, &sub_dar);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_WID_HEI, sub_wid, sub_hei, id);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_DAR, sub_dar, 0, id);
	}

	pHdrObj->uiVidCodec = vidcodec;
	pHdrObj->uiVidFrameRate = vfr;
	pHdrObj->uiWidth = wid;
	pHdrObj->uiHeight = hei;
	pHdrObj->uiDAR = 0;
	pHdrObj->uiAudSampleRate = asr;
	pHdrObj->uiAudBits = 16;
	pHdrObj->uiAudChannels = chs;

	uiHeaderSize = bsmux_mdat_calc_header_size(id);
	pHdrObj->uiHeaderSize = uiHeaderSize;
	//thumb data
	bsmux_util_get_param(id, BSMUXER_PARAM_THUMB_ADDR, &uiThumbAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_THUMB_SIZE, &uiThumbSize);
	if (uiThumbSize) {
		pHdrObj->uiThumbAddr = uiThumbAddr;
		pHdrObj->uiThumbSize = uiThumbSize;
	} else {
		pHdrObj->uiThumbAddr = 0;
		pHdrObj->uiThumbSize = 0;
	}
	//user data
	bsmux_util_get_param(id, BSMUXER_PARAM_USERDATA_ADDR, &userDataAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_USERDATA_SIZE, &userDataSize);
	pHdrObj->uiUdataAddr = userDataAddr;
	pHdrObj->uiUdataSize = userDataSize;
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_USERDATA, userDataAddr, userDataSize, id);
	if (filetype == MEDIA_FILEFORMAT_MP4) {
		pHdrObj->uiFileType = MEDIA_FTYP_MP4;
	} else if (filetype == MEDIA_FILEFORMAT_MOV) {
		pHdrObj->uiFileType = MEDIA_FTYP_MOV;
	}
	pHdrObj->uiFre1Addr = 0;
	pHdrObj->uiFre1Size = 0;
	pHdrObj->bCustomUdata = 0;
	pHdrObj->uiMakerID = makerId;
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MOV_ROTATEINFO, 0, 0, id);
	pHdrObj->uiRotationInfo = MOVREC_MOV_ROTATE_0;//NMEDIAREC_MOV_ROTATE_0;
	pHdrObj->uiFreaSize = 0;
	vos_perf_mark(&temptimeid);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_TIMESPECIALID, (UINT32)temptimeid, 0, id);

#if 0
	tempfront = bsmux_mdat_get_hdr_mem(BSMUX_HDRMEM_FRONT, id);
	if (tempfront == 0) {
		DBG_ERR("[id=%d]frontheader_1 zero\r\n", id);
	}
#endif
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, tempfront);

	//makerobj makeheader
	{
		if (pCMaker->MakeHeader == 0) {
			DBG_ERR("MakeHeader( ) NULL!!!\r\n");
			bsmux_mkhdr_unlock();
			return E_SYS;
		}

		returnv = (pCMaker->MakeHeader)(tempfront, &uiHeaderSize, pHdrObj);
		if (returnv != E_OK) {
			DBG_ERR("make header fail\r\n");
			bsmux_mkhdr_unlock();
			return returnv;
		}
	}

	thumbPut = ((pHdrObj->uiThumbSize) ? TRUE : FALSE);
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_THUMBPUT, thumbPut);
	MDAT_FUNC("[%d] uiThumbAddr=0x%X uiThumbSize=0x%X thumbPut=%d\r\n", id, pHdrObj->uiThumbAddr, pHdrObj->uiThumbSize, thumbPut);

	bsmux_util_set_param(id, BSMUXER_PARAM_FREASIZE, pHdrObj->uiFreaSize);
	bsmux_util_set_param(id, BSMUXER_PARAM_FREAENDSIZE, pHdrObj->uiFreaEndSize);
	MDAT_FUNC("[%d] uiFreaSize=0x%X uiFreaEndSize=0x%X\r\n", id, pHdrObj->uiFreaSize, pHdrObj->uiFreaEndSize);

	if (frontheader_2) {
		MDAT_FUNC("[%d] package header\r\n", id);

		// move package and copy header to copy buf here
		bsmux_mdat_package_header(id);

		// if need fast put first ops buf to open file
		bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fastput_en);
		if (fastput_en) {
			bsmux_mdat_padding_header(id);
		}
	} else {
		MDAT_FUNC("[%d] package thumbnail\r\n", id);

		bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
		if (front_moov) {
			BSMUX_MSG("make_header: [%d] front moov flow, thumb with next moov and return\r\n", id);
			return E_OK;
		}

		// package header with thumb and prepare buffer
		bsmux_mdat_package_thumb(id);
	}

	return E_OK;
}

ER bsmux_mdat_update_header(UINT32 id, void *p_maker)
{
	CONTAINERMAKER  MRtempmaker;
	CONTAINERMAKER *pCMaker = (CONTAINERMAKER *)p_maker;
	UINT32 frontheader_2 = 0, backheader = 0;
	UINT32 recformat = 0, width = 0, height = 0, playvfr = 0;
	UINT32 audSize, vfn, vfr, afn;
	UINT32 sub_vfn, sub_vfr, stopVF, sub_newvfn = 0;
	UINT32 vidDescAddr, vidDescSize = 0, vidFrames, dur;//, oldvfn = 0;
	UINT32 filesizeL = 0, filesizeH = 0, real = 0;
	UINT32 audioyes = 0, newvfn = 0;
	UINT32 makerid, asr, chs, backsize = 0, freasize;
	VOS_TICK tt1, tt2;
	UINT32 filetype;
	UINT32 codectype;
	UINT32 userDataAddr = 0, userDataSize = 0;
	CUSTOMDATA custdata = {0};
	UINT32 timescale = 0;
	UINT32 front_moov = 0;
	UINT32 variable_rate = 0;
	ER returnv;

	vos_perf_mark(&tt2);

	vos_perf_mark(&tt1);
	if (vos_perf_duration(tt2, tt1) > 10000) { //10 ms
		DBG_DUMP("[%d]UPDATE wait %dms\r\n", id, vos_perf_duration(tt1, tt2));
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
	if (front_moov) {
		DBG_DUMP("END[%d] fmoov-B\r\n", id);
		bsmux_mdat_make_front_moov(id, p_maker, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_SIZE, 0);
		//reset
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, 0);
		//reset gps total num
		{
			MOV_Ientry thisJpg = {0};
			bsmux_util_set_minfo(MEDIAWRITE_SETINTO_MOVGPSENTRY, 0, (UINT32)&thisJpg, id);
		}
		DBG_DUMP("END[%d] fmoov-E\r\n", id);
		return E_OK;
	}

	//copy temp header
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_COPYTEMPHDR, 0, 0, id);

	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_NOEXS;
	}
	makerid = pCMaker->id;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &codectype);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &asr);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CHS, &chs);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_ON, &audioyes);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_RECFORMAT, &recformat);
	if ((recformat == MEDIAREC_GOLFSHOT) ||
		(recformat == MEDIAREC_TIMELAPSE)) {
		audioyes = 0;
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_WIDTH, &width);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_HEIGHT, &height);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_WID_HEI, width, height, id);

	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	if (vfn == 0 && sub_vfn == 0) {
		DBG_DUMP("NO updateHDR\r\n");
		DBG_DUMP("_upd[%d]-E22\r\n", id);
		bsmux_util_dump_info(id);
		bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_SIZE, 0);
		//reset
		bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, 0);
		//reset gps total num
		{
			MOV_Ientry thisJpg = {0};
			bsmux_util_set_minfo(MEDIAWRITE_SETINTO_MOVGPSENTRY, 0, (UINT32)&thisJpg, id);
		}
		return E_OK;
	}
	MDAT_FUNC(">>> vfn %d\r\n", vfn);
	MDAT_FUNC(">>> sub vfn %d\r\n", sub_vfn);
	//oldvfn = vfn;

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	if ((recformat == MEDIAREC_GOLFSHOT) ||
		(recformat == MEDIAREC_TIMELAPSE)) {
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PLAYFRAMERATE, &playvfr);
		if (playvfr == 0) {
			playvfr = 30;
		}

		/* GOLFSHOT: need to update frame rate to config playback speed */
		if (recformat == MEDIAREC_GOLFSHOT) {
			bsmux_util_set_minfo(MEDIAWRITE_SETINFO_VID_FR, playvfr, 0, id);
		}
	}

	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &afn);
	#if 0 //slowmedia
	if (bsmux_util_is_not_normal()) {
		UINT32 tempdur = 0;
		tempdur = vfn / vfr;
		if (tempdur > 1) {
			tempdur -= 1;
		}
		vfn = tempdur * vfr;
		afn = tempdur * (asr / 1024);
		DBG_DUMP("^GSLow path[%d]oldv=%d, v=%d, a=%d, newdur = %d\r\n", id, oldvfn, vfn, afn, tempdur);
		if (vfn == 0) {
			DBG_DUMP("^C NO updateHDR\r\n");
			DBG_DUMP("^C _upd[%d]-E44\r\n", id);
			DBG_DUMP("^C Push discard\r\n");
			//package
			addInfo.fileop = 0; //clear
			addInfo.fileop |= BSMUXER_FOP_DISCARD;
			addInfo.addr = pOneinfo->hdr.frontheader_2;
			addInfo.type = filetype;//MEDIA_FILEFORMAT_MP4;
			addInfo.pathID = mdatid;
			addInfo.pos  = 0;
			//callback
			(gNMRMdatRecObj[mdatid].callBackFunc)(BSMUXER_CBEVENT_MDATREADY, (UINT32)&addInfo, mdatid, addInfo.addr);
			bsmux_mkhdr_unlock();
			return E_OK;
		}
	}
	#endif
	MDAT_FUNC(">>> afn %d\r\n", afn);

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	if (sub_vfr) {
		// sub-stream
		stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_SUBVD);
		if (sub_vfn > stopVF) {
			DBG_DUMP("[%d] sub_vfn %d > stopVF %d\r\n", id, sub_vfn, stopVF);
			sub_newvfn = stopVF;
		} else {
			sub_newvfn = sub_vfn;
		}
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_VID_FRAME, sub_newvfn, 0, id);
		// main-stream
		stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO);
		if (vfn > stopVF) {
			DBG_DUMP("vfn %d > stopVF %d\r\n", vfn, stopVF);
			newvfn = stopVF;
		} else {
			newvfn = vfn;
		}
	} else {
		newvfn = vfn;
	}

	if (audioyes == 0) { //audio no, but no pading = 120fps to cardfull
		#if 0
		if (bsmux_util_check_if_normal() == 0) {
			newvfn = vfn - (vfn % vfr);
		} else {
			newvfn = vfn;
		}
		#else
		//newvfn = vfn;
		#endif
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_VID_FRAME, newvfn, 0, id);
	} else {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_VID_FRAME, newvfn, 0, id);
	}
	{
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_FRAME, afn, 0, id);
	}

	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_TIMESCALE, &makerid, &vfr, &timescale);
	if (timescale == 0) {
		timescale = MOV_TIMESCALE; //CIM 144979
	}
	MDAT_FUNC("timescale %d\r\n", timescale);
	if (audioyes == 0) {
		vidFrames = newvfn;
		dur = bsmux_mdat_calc_entry_duration(id, vidFrames, vfr);

		/* GOLFSHOT: need to update duration to config playback speed */
		if (recformat == MEDIAREC_GOLFSHOT) {
			dur = bsmux_mdat_calc_entry_duration(id, vidFrames, playvfr);
		}
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_REC_DURATION, dur, 0, id);
	} else {
		vidFrames = newvfn;
		dur = bsmux_mdat_calc_entry_duration(id, vidFrames, vfr);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_REC_DURATION, dur, 0, id);
	}
	MDAT_FUNC(">>> dur %d\r\n", dur / timescale);
	if (sub_vfr) {
		dur = bsmux_mdat_calc_entry_duration(id, sub_newvfn, sub_vfr);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_SUB_DURATION, dur, 0, id);
	}
	MDAT_FUNC(">>> sub dur %d\r\n", dur / timescale);

	filesizeL = 0x10000;
	filesizeH = 0;
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MOVMDATSIZE, filesizeL, filesizeH, id);
	backheader = bsmux_mdat_get_hdr_mem(BSMUX_HDRMEM_BACK, id);
	if (backheader == 0) {
		DBG_ERR("[id=%d]backheader zero\r\n", id);
		bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
		return E_SYS;
	}
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_BACKHDR_ADDR, backheader);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_TEMPMOOVADDR, backheader, 0, id);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_VAR_RATE, &variable_rate);
	if (audioyes == 0) {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUDIOSIZE, 0, 0, id);//set zero audio size
	} else if (variable_rate) {
		BSMUX_MSG("[%d] variable_rate\r\n", id);
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUDIOSIZE, 0, 0, id);//set zero audio size
	} else {
		audSize = afn * MP_AUDENC_AAC_RAW_BLOCK * chs * 2;
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUDIOSIZE, audSize, 0, id);
	}
	if (variable_rate == 0) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VAR_VFR, &variable_rate);
		if (variable_rate != vfr) {
			asr = ((((variable_rate * 100) / vfr) * asr) / 100);
			BSMUX_MSG("[%d] modified_sr=%d\r\n", id);
		}
	}
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_SAMPLERATE, asr, 0, id);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_AUD_CHS, chs, 0, id);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &vidDescAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCSIZE, &vidDescSize);
	if (vidDescSize) {
		bsmux_mdat_put_desc(id, vidDescAddr, vidDescSize);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &vidDescAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, &vidDescSize);
	if (vidDescSize) {
		bsmux_mdat_put_sub_desc(id, vidDescAddr, vidDescSize);
	}
	//user data
	bsmux_util_get_param(id, BSMUXER_PARAM_USERDATA_ADDR, &userDataAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_USERDATA_SIZE, &userDataSize);
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_USERDATA, userDataAddr, userDataSize, id);
	//cust data
	bsmux_util_get_param(id, BSMUXER_PARAM_CUSTDATA_ADDR, &(custdata.addr));
	bsmux_util_get_param(id, BSMUXER_PARAM_CUSTDATA_SIZE, &(custdata.size));
	bsmux_util_get_param(id, BSMUXER_PARAM_CUSTDATA_TAG, &(custdata.tag));
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_CUSTOMDATA, (UINT32)(&(custdata)), 0, id);
	if (id < BSMUX_MAX_CTRL_NUM) {
		MRtempmaker.cbWriteFile = NULL;
	} else {
		DBG_DUMP("^R FSTSKID NOT ENOUGH!! no make file!!\r\n");
		DBG_DUMP("_upd[%d]-E11\r\n", id);
		return E_SYS;
	}
	MovWriteLib_ChangeWriteCB((void *)&MRtempmaker);

	//makerobj updateheader
	{
		if (pCMaker->UpdateHeader == 0) {
			DBG_ERR("UpdateHeader( ) NULL!!!\r\n");
			return E_SYS;
		}

		returnv = (pCMaker->UpdateHeader)(makerid, 0);
		if (E_OK != returnv) {
			DBG_ERR("update header fail\r\n");
			return returnv;
		}
	}

	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_FINALFILESIZE, &makerid, &filesizeL, &filesizeH);
	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_BACKHDR_SIZE, &makerid, &backsize, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_BACKHDR_SIZE, backsize);

	//OUT DATA
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_BACKHDR_ADDR, &backheader);
	bsmux_util_prepare_buf(id, BSMUX_BS2CARD_BACK, backheader, backsize, 0);

	bsmux_util_get_param(id, BSMUXER_PARAM_FREASIZE, &freasize);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, &real);
	MDAT_FUNC(">> bsmux_util_update_header real %d\r\n", real);
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, &frontheader_2);
	bsmux_update_vidsize(frontheader_2 + freasize + 0x1c, real - freasize - 0x1c);

	//update padding blk size and tag
	{
		UINT32 fastput_en = 0, blksize = 0;
		UINT32 tempaddr, tempsize;
		bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fastput_en);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEBLOCK, &blksize);
		if (fastput_en) {
			tempaddr = frontheader_2 + freasize + 0x1c + 8;
			#if 1   //fast put flow ver3
			tempsize = blksize - freasize - 0x1c - 8 - bsmux_util_calc_padding_size(id);
			#else
			tempsize = blksize - freasize - 0x1c - 8 - MP_AUDENC_AAC_RAW_BLOCK * 3;
			#endif	//fast put flow ver3
			MDAT_FUNC("[%d] mem mdat now addr=0x%x, size=0x%x\r\n", id, tempaddr, tempsize);
			bsmux_update_vidsize(tempaddr, tempsize);
			bsmux_update_string(tempaddr+4, "free");
		}
	}

	//OUT DATA
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, &frontheader_2);
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, frontheader_2);
	bsmux_util_prepare_buf(id, BSMUX_BS2CARD_TEMP, frontheader_2, bsmux_mdat_calc_header_size(id), 0);

	bsmux_util_set_param(id, BSMUXER_PARAM_THUMB_SIZE, 0);
	//reset
	bsmux_util_set_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, 0);
	//reset gps total num
	{
		MOV_Ientry thisJpg = {0};
		bsmux_util_set_minfo(MEDIAWRITE_SETINTO_MOVGPSENTRY, 0, (UINT32)&thisJpg, id);
	}
	#if 0
	gNMR_kickthumb[mdatid] = 0;
	#endif

	return E_OK;
}

ER bsmux_mdat_make_front_moov(UINT32 id, void *p_maker, UINT32 minus1sec)
{
	CONTAINERMAKER *pCMaker = (CONTAINERMAKER *)p_maker;
	MAKEMOOV_INFO *p_moovinfo = NULL; //[MakeMoov]
	MOVMJPG_FILEINFO *pnfinfo = NULL; //[MakeMoov]
	UINT32 vfn = 0, vfr = 0, width = 0, height = 0, vidcodec = 0;
	UINT32 afn = 0, asr = 0, chs = 0, audcodec = 0, audioyes = 0;
	UINT32 recformat = 0, playvfr = 0, filetype = 0, vidDescAddr = 0, vidDescSize = 0;
	UINT32 makerId = 0, timescale = 0, dur, filesize = 0, frontheader_2 = 0;
	UINT64 filesize64 = 0;
	UINT32 real = 0, tag_size = 0, header_size = 0, freasize = 0;
	UINT32 type, addr, size, pos;
	UINT32 front_moov = 0, moov_size = 0;
	UINT32 sub_vfn, sub_width, sub_height, sub_vidcodec, sub_vfr, sub_dur, stopVF;
	UINT32 variable_rate = 0;
	UINT32 mux_method = 0;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};
	void *ptemp;
	ER returnv;

	bsmux_util_get_param(id, BSMUXER_PARAM_MUXMETHOD, &mux_method);

	// moov info
	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
	bsmux_util_get_param(id, BSMUXER_PARAM_MOOV_SIZE, &moov_size);

	bsmux_util_get_mid(id, &makerId);
	p_moovinfo = bsmux_get_moov_info(id);
	if (p_moovinfo == NULL) {
		DBG_ERR("[%d] get moov info null\r\n", id);
		return E_SYS;
	}
	if (p_moovinfo->pfileinfo == NULL) {
		DBG_ERR("[%d] get nfinfo null\r\n", id);
		return E_SYS;
	}
	p_moovinfo->id = makerId;
	pnfinfo = p_moovinfo->pfileinfo;

	//copy temp header, moov always with ftyp
	bsmux_util_set_minfo(MEDIAWRITE_SETINFO_COPYTEMPHDR, 0, 0, id);

	// vid info
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	if (minus1sec) {
#if BSMUX_TEST_MOOV_RC
		UINT32 frontPut = 0;
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (frontPut) {
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_PUT_VF, &vfn); //put
		}
#endif
		if (vfn) {
			vfn = vfn - 1; // since CalcAlign round off
			#if 1   //fast put flow ver3
			{
				UINT32 fastput_en = 0, frontPut = 0;
				bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fastput_en);
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
				if (fastput_en && !frontPut) {
					vfn = 1;// already has one vfn
					BSMUX_MSG("[MAKEMOOV] id%d_%d, fput_vfn=%d\r\n", id, p_moovinfo->id, vfn);
				}
			}
			#endif	//fast put flow ver3
		}
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_RECFORMAT, &recformat);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_WIDTH, &width);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_HEIGHT, &height);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &vidcodec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	if (vfr == 0) {
		vfr = 30;
	}
	pnfinfo->ImageWidth = width;
	pnfinfo->ImageHeight = height;
	pnfinfo->encVideoCodec = vidcodec;
	pnfinfo->VideoFrameRate = vfr;
	if ((recformat == MEDIAREC_GOLFSHOT) || (recformat == MEDIAREC_TIMELAPSE)) {
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PLAYFRAMERATE, &playvfr);
		if (playvfr == 0) {
			playvfr = 30;
		}

		/* GOLFSHOT: need to update frame rate to config playback speed */
		if (recformat == MEDIAREC_GOLFSHOT) {
			pnfinfo->VideoFrameRate = playvfr;
		}
	}
	pnfinfo->videoFrameCount = vfn;

	// sub vid info
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
	if (minus1sec) {
#if BSMUX_TEST_MOOV_RC
		UINT32 frontPut = 0;
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (frontPut) {
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_PUT_SUBVF, &sub_vfn); //put
		}
#endif
		if (sub_vfn) {
			sub_vfn = sub_vfn - 1; // since CalcAlign round off
		}
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_WIDTH, &sub_width);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_HEIGHT, &sub_height);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &sub_vidcodec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	if (sub_vfr) {
		pnfinfo->SubVideoTrackEn = 1;
		pnfinfo->SubImageWidth = sub_width;
		pnfinfo->SubImageHeight = sub_height;
		pnfinfo->SubencVideoCodec = sub_vidcodec;
		pnfinfo->SubVideoFrameRate = sub_vfr;
		pnfinfo->SubvideoFrameCount = sub_vfn;
	}

	BSMUX_MSG("[MAKEMOOV] id%d_%d, main=(%d/%d), sub=(%d/%d)\r\n", id, p_moovinfo->id,
		(int)vfr, (int)vfn, (int)sub_vfr, (int)sub_vfn);

	/* change location to avoid divide-by-zero exception on calc_entry_duration */
	#if 1
	if (sub_vfr) {
		if ((vfn == 0) || (sub_vfn == 0)) {
			DBG_IND("make_front_moov: return\r\n");
			return E_OK;
		}
	} else {
		if ((vfn == 0) && (sub_vfn == 0)) {
			DBG_IND("make_front_moov: return\r\n");
			return E_OK;
		}
	}
	#endif

	// aud info
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &afn);
	if (minus1sec) {
#if BSMUX_TEST_MOOV_RC
		UINT32 frontPut = 0;
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (frontPut) {
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_PUT_AF, &afn); //put
		}
#endif
		if (afn) {
			afn = afn - 1; // since CalcAlign round off
		}
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CODECTYPE, &audcodec);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &asr);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_CHS, &chs);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_ON, &audioyes);
	pnfinfo->encAudioCodec = audcodec;
	pnfinfo->AudioSampleRate = asr;
	pnfinfo->AudioChannel = chs;
	if ((recformat == MEDIAREC_GOLFSHOT) || (recformat == MEDIAREC_TIMELAPSE)) {
		audioyes = 0;
	}
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_VAR_RATE, &variable_rate);
	if (audioyes == 0) {
		pnfinfo->audioTotalSize = 0;
	} else if (variable_rate) {
		BSMUX_MSG("[%d] variable_rate\r\n", id);
		pnfinfo->audioTotalSize = 0;
	} else {
		pnfinfo->audioTotalSize = afn * MP_AUDENC_AAC_RAW_BLOCK * chs * 2;
	}
	if (variable_rate == 0) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VAR_VFR, &variable_rate);
		if (variable_rate != vfr) {
			asr = ((((variable_rate * 100) / vfr) * asr) / 100);
			pnfinfo->AudioSampleRate = asr;
			BSMUX_MSG("[%d] modified_sr=%d\r\n", id);
		}
	}
	pnfinfo->audioFrameCount = afn;

	// file info
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
	bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_TIMESCALE, &makerId, &vfr, &timescale);
	if (timescale == 0) {
		timescale = MOV_TIMESCALE; //CIM 144979
	}
	MDAT_FUNC("timescale %d\r\n", timescale);
	if (filetype == MEDIA_FILEFORMAT_MP4) {
		pnfinfo->uiFileType = MEDIA_FTYP_MP4;
	} else {
		pnfinfo->uiFileType = MEDIA_FTYP_MOV;
	}
	if (sub_vfr) {
		// sub-stream
		if (sub_vfn) {
			if (minus1sec) {
				sub_dur = bsmux_mdat_calc_entry_duration(id, sub_vfn, sub_vfr);
				pnfinfo->SubPlayDuration = sub_dur;
			} else {
				stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_SUBVD);
				if (sub_vfn > stopVF) {
					DBG_DUMP("[%d] sub_vfn %d > stopVF %d\r\n", id, sub_vfn, stopVF);
					sub_vfn = stopVF;
				}
				sub_dur = bsmux_mdat_calc_entry_duration(id, sub_vfn, sub_vfr);
				pnfinfo->SubvideoFrameCount = sub_vfn;
				pnfinfo->SubPlayDuration = sub_dur;
			}
		}
		// main-stream
		if (vfn) {
			if (minus1sec) {
				dur = bsmux_mdat_calc_entry_duration(id, vfn, vfr);
				pnfinfo->PlayDuration = dur;
			} else {
				stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO);
				if (vfn > stopVF) {
					DBG_DUMP("[%d] vfn %d > stopVF %d\r\n", id, vfn, stopVF);
					vfn = stopVF;
				}
				dur = bsmux_mdat_calc_entry_duration(id, vfn, vfr);
				pnfinfo->videoFrameCount = vfn;
				pnfinfo->PlayDuration = dur;
			}
		}

		/* GOLFSHOT: need to update duration to config playback speed */
		if (recformat == MEDIAREC_GOLFSHOT) {
			if (sub_vfn) {
				sub_dur = bsmux_mdat_calc_entry_duration(id, sub_vfn, playvfr);
				pnfinfo->SubPlayDuration = sub_dur;
			}
			if (vfn) {
				dur = bsmux_mdat_calc_entry_duration(id, vfn, playvfr);
				pnfinfo->PlayDuration = dur;
			}
		}
	} else {
		dur = bsmux_mdat_calc_entry_duration(id, vfn, vfr);

		/* GOLFSHOT: need to update duration to config playback speed */
		if (recformat == MEDIAREC_GOLFSHOT) {
			dur = bsmux_mdat_calc_entry_duration(id, vfn, playvfr);
		}
		pnfinfo->PlayDuration = dur;
	}
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, &filesize);
	filesize64 = (UINT64)filesize;
	pnfinfo->totalMdatSize = filesize64;
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, &frontheader_2);
	pnfinfo->ptempMoovBufAddr = (UINT8 *)frontheader_2;
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &vidDescAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCSIZE, &vidDescSize);
	if (vidDescSize) {
		pnfinfo->vidDescAdr = vidDescAddr;
		pnfinfo->vidDescSize = vidDescSize;
		bsmux_mdat_put_desc(id, vidDescAddr, vidDescSize);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &vidDescAddr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, &vidDescSize);
	if (vidDescSize) {
		bsmux_mdat_put_sub_desc(id, vidDescAddr, vidDescSize);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_FREAENDSIZE, &freasize); //set while make header
	p_moovinfo->pfileinfo->shiftfrea = freasize;
	p_moovinfo->pfileinfo->outMoovSize = moov_size - freasize;
	MDAT_FUNC("[%d] moov_size=0x%x freasize=0x%x outMoovSize=0x%x\r\n",
		id, moov_size, freasize, p_moovinfo->pfileinfo->outMoovSize);


	// maker
	ptemp = (void *)(p_moovinfo);
	if (pCMaker->CustomizeFunc == 0) {
		DBG_ERR("CustomizeFunc( ) NULL!!!\r\n");
		return E_SYS;
	}

	BSMUX_MSG("[MAKEMOOV] id%d_%d, vfn=%d, afn=%d\r\n", id, p_moovinfo->id,
		p_moovinfo->pfileinfo->videoFrameCount,
		p_moovinfo->pfileinfo->audioFrameCount);

	/* change location to avoid divide-by-zero exception on calc_entry_duration */
	#if 0
	if ((vfn == 0) && (sub_vfn == 0)) {
		DBG_IND("make_front_moov: return\r\n");
		return E_OK;
	}
	#endif

	returnv = (pCMaker->CustomizeFunc)(MEDIAWRITELIB_CUSTOM_FUNC_MAKEMOOV, ptemp);
	if (returnv != E_OK) {
		DBG_ERR("CustomizeFunc( ) ERROR!!!\r\n");
		return E_SYS;
	}

	// offset
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, &real);
	bsmux_util_get_param(id, BSMUXER_PARAM_BOXTAG_SIZE, &tag_size);
	if (tag_size == 64) {
		tag_size = 16;
	} else {
		tag_size = 8;
	}
	header_size = bsmux_mdat_calc_header_size(id);
	bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_ADDR, &frontheader_2);
	bsmux_update_vidsize(frontheader_2 + header_size - tag_size, real - header_size + tag_size);

	if (minus1sec) {
		UINT32 frontPut = 0, last2card_addr = 0;
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			BSMUX_MSG("make_front_moov: [%d] copy temphdr addr to last2card addr\r\n", id);
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card_addr);
			#if _TOFIX_
			if (bsmux_util_is_not_normal()) {
				DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
				return E_NOMEM; ///< Insufficient memory
			}
			#endif
			dst.phy_addr = bsmux_util_get_buf_pa(id, last2card_addr);
			dst.vrt_addr = last2card_addr;
			src.phy_addr = bsmux_util_get_buf_pa(id, frontheader_2);
			src.vrt_addr = frontheader_2;
			bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, bsmux_mdat_calc_header_size(id), mux_method);
			return E_OK;
		}
	}

	//OUT DATA
	if (minus1sec) {
		type = BSMUX_BS2CARD_MOOV;
	} else {
		type = BSMUX_BS2CARD_TEMP;
	}
	addr = (UINT32)p_moovinfo->pfileinfo->ptempMoovBufAddr;
	size = bsmux_mdat_calc_header_size(id);
	pos = 0; // moov always with ftyp
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, addr);
	bsmux_util_prepare_buf(id, type, addr, size, pos);

	return E_OK;
}

