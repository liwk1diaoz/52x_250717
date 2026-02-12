/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "bsmux_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
//>> common
static BSMUX_REC_INFO     g_bsmux_rec_info[BSMUX_MAX_CTRL_NUM] = {0};
static CONTAINERMAKER     g_bsmux_maker_obj[BSMUX_MAX_CTRL_NUM] = {0};
static char               g_bsmux_ver_string[0x10] = {0};
static UINT32             g_bsmux_module_status = BSMUXER_RESULT_NORMAL;
static BSMUX_SYNC_INFO    g_bsmux_vbs[BSMUX_MAX_CTRL_NUM] = {0};
static BSMUX_SYNC_INFO    g_bsmux_abs[BSMUX_MAX_CTRL_NUM] = {0};
static BSMUX_OPS          *g_pBsMuxUtil;
//>> plugin
static BSMUX_OPS          g_BsMuxUtilMdat = {
	MEDIA_FILEFORMAT_MP4,
	//common (15)
	bsmux_mdat_dbg,
	bsmux_mdat_tskobj_init,
	bsmux_mdat_open,
	bsmux_mdat_close,
	bsmux_mdat_memcpy,
	bsmux_mdat_get_need_size,
	bsmux_mdat_set_need_size,
	bsmux_mdat_clean,
	bsmux_mdat_update_vidsize,
	bsmux_mdat_release_buf,
	bsmux_mdat_gps_pad,
	bsmux_mdat_add_thumb,
	bsmux_mdat_add_last,
	bsmux_mdat_put_last,
	bsmux_mdat_add_meta,
	//sepcific (7)
	bsmux_mdat_save_entry,
	bsmux_mdat_nidx_pad,
	bsmux_mdat_make_header,
	bsmux_mdat_update_header,
	NULL,
	NULL,
	bsmux_mdat_make_front_moov,
};
static BSMUX_OPS          g_BsMuxUtilTs = {
	MEDIA_FILEFORMAT_TS,
	//common (15)
	bsmux_ts_dbg,
	bsmux_ts_tskobj_init,
	bsmux_ts_open,
	bsmux_ts_close,
	bsmux_ts_mux,
	bsmux_ts_get_need_size,
	bsmux_ts_set_need_size,
	bsmux_ts_clean,
	bsmux_ts_update_vidinfo,
	bsmux_ts_release_buf,
	bsmux_ts_add_gps_data,
	bsmux_ts_add_thumb,
	bsmux_ts_add_last,
	bsmux_ts_put_last,
	bsmux_ts_add_meta,
	//sepcific (7)
	bsmux_ts_save_entry,
	NULL,
	bsmux_ts_make_header,
	NULL,
	bsmux_ts_make_pes,
	bsmux_ts_make_pat,
	NULL,
};
// debug
static UINT32             g_debug_util_mem = BSMUX_DEBUG_MEMBUF;
#define UTIL_MEM_FUNC(fmtstr, args...) do { if(g_debug_util_mem) DBG_DUMP(fmtstr, ##args); } while(0)
extern BOOL               g_bsmux_debug_msg;
#define BSMUX_MSG(fmtstr, args...) do { if(g_bsmux_debug_msg) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
//Note: do not export this function
static BSMUX_REC_INFO *bsmux_get_rec_info(UINT32 id)
{
	return &g_bsmux_rec_info[id];
}
//Note: do not export this function
static CONTAINERMAKER *bsmux_get_maker_obj(UINT32 id)
{
	return &g_bsmux_maker_obj[id];
}
//Note: do not export this function
//@note: modified from NMediaRec_CheckSec
static UINT32 bsmux_check_sec(BSMUX_CALC_SEC_SETTING *pinfo)
{
	UINT32 vbyte = 0, bytesPerSec, BperSec, chunksize, fr, hdrsize, asr;

	if (pinfo->vidfps == 0) {
		return 0;
	}
	if (pinfo->vidTBR == 0) {
		return 0;
	}
	//audio bytes per second
	asr = pinfo->audSampleRate;
	BperSec = pinfo->audChs * 2 * pinfo->audSampleRate;
	chunksize = BperSec / 2;
	if (pinfo->vidTBR) {
		vbyte = pinfo->vidTBR + pinfo->vidTBR / 30 ;
	}
	fr = pinfo->vidfps;
	//header size
	hdrsize = fr * 20 + asr / 1024 * 20; //vid+aud
	bytesPerSec = vbyte + chunksize + hdrsize;
	if (bytesPerSec == 0) {
		DBG_ERR("bytesPerSec zero\r\n");
		return 0;
	}
	if (pinfo->gpson) {
		bytesPerSec += BSMUX_GPS_MIN;
	}
	if (pinfo->nidxon) {
		bytesPerSec += BSMUX_MAX_NIDX_BLK;
	}
	return bytesPerSec;
}
//Note: do not export this function
static void bsmux_init_sync_info(UINT32 id)
{
	memset(&g_bsmux_vbs[id], 0, sizeof(BSMUX_SYNC_INFO));
	memset(&g_bsmux_abs[id], 0, sizeof(BSMUX_SYNC_INFO));
}
//Note: do not export this function
static void bsmux_maker_init_mov(UINT32 id)
{
	CONTAINERMAKER *pCMaker = NULL;
	BSMUX_REC_INFO *pOneinfo = NULL;
	UINT32 vidcodec, maker_id, vfr, filetype;

	pOneinfo = bsmux_get_rec_info(id);
	if (pOneinfo == NULL) {
		DBG_ERR("get prj setting null\r\n");
		return ;
	}

	pCMaker = bsmux_get_maker_obj(id);
	if (pCMaker == NULL) {
		DBG_ERR("get maker obj null\r\n");
		return ;
	}

	maker_id = pCMaker->id;
	vfr = pOneinfo->file.vid.vfr;
	filetype = pOneinfo->file.filetype;
	if (pCMaker->SetInfo == 0) {
		DBG_ERR("[f%d]SetInfo( ) NULL!!!\r\n", id);
		return;
	}
	vidcodec = pOneinfo->file.vid.vidcodec;

	if (vidcodec == MEDIAVIDENC_MJPG) {
		(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_VIDEOTYPE, MEDIAVIDENC_MJPG, 0, maker_id);
	} else if (vidcodec == MEDIAVIDENC_H264) {
		(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_VIDEOTYPE, MEDIAVIDENC_H264, 0, maker_id);
	} else if (vidcodec == MEDIAVIDENC_H265) {
		(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_VIDEOTYPE, MEDIAVIDENC_H265, 0, maker_id);
	}

	if (filetype == MEDIA_FILEFORMAT_MP4) {
		(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_FILETYPE, MEDIA_FTYP_MP4, 0, maker_id);
	} else {
		(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_FILETYPE, filetype, 0, maker_id);
	}

	(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_CLUSTERSIZE, 0x8000, 0, maker_id);
	(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_SAVE_VF, vfr, 0, maker_id);

	if (pOneinfo->file.sub_vid.vfr) {
		vidcodec = pOneinfo->file.sub_vid.vidcodec;
		if (vidcodec == MEDIAVIDENC_MJPG) {
			(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_SUB_VIDEOTYPE, MEDIAVIDENC_MJPG, 0, maker_id);
		} else if (vidcodec == MEDIAVIDENC_H264) {
			(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_SUB_VIDEOTYPE, MEDIAVIDENC_H264, 0, maker_id);
		} else if (vidcodec == MEDIAVIDENC_H265) {
			(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_SUB_VIDEOTYPE, MEDIAVIDENC_H265, 0, maker_id);
		}
		(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_SUB_ENABLE, TRUE, 0, maker_id);
	}

	return ;
}
//Note: do not export this function
static void bsmux_maker_init_ts(UINT32 id)
{
	CONTAINERMAKER *pCMaker = NULL;
	BSMUX_REC_INFO *pOneinfo = NULL;
	UINT32 vidcodec, maker_id, vfr, asr;

	pOneinfo = bsmux_get_rec_info(id);
	if (pOneinfo == NULL) {
		DBG_ERR("get prj setting null\r\n");
		return ;
	}

	pCMaker = bsmux_get_maker_obj(id);
	if (pCMaker == NULL) {
		DBG_ERR("get maker obj null\r\n");
		return ;
	}

	maker_id = pCMaker->id;
	vfr = pOneinfo->file.vid.vfr;
	asr = pOneinfo->file.aud.asr;
	vidcodec = pOneinfo->file.vid.vidcodec;
	//set sample rate,before reset pts
	(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_AUD_SAMPLERATE, asr, 0, maker_id);
	(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_VIDEOFRAMERATE, vfr, vidcodec, maker_id);
	(pCMaker->SetInfo)(MEDIAWRITE_SETINFO_RESETPTS, 0, 0, maker_id);
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER bsmux_util_get_mid(UINT32 id, UINT32 *makerId)
{
	CONTAINERMAKER *pCMaker = NULL;

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_NOEXS;
	}
	*makerId = pCMaker->id;
	return E_OK;
}

ER bsmux_util_set_minfo(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	CONTAINERMAKER *pCMaker = NULL;
	UINT32 makerId;
	UINT32 id = p3;

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_NOEXS;
	}
	makerId = pCMaker->id;

	if (pCMaker->SetInfo == 0) {
		DBG_ERR("id=%d, type=%d, makerId=%d SetInfo( ) NULL!!!\r\n", id, type, makerId);
		return E_NOEXS;
	}

	return (pCMaker->SetInfo)(type, p1, p2, makerId);
}

ER bsmux_util_get_minfo(UINT32 id, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	CONTAINERMAKER *pCMaker = NULL;

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_NOEXS;
	}

	if (pCMaker->GetInfo == 0) {
		DBG_ERR("id=%d, type=%d, GetInfo( ) NULL!!!\r\n", id, type);
		return E_NOEXS;
	}

	return (pCMaker->GetInfo)(type, p1, p2, p3);
}

ER bsmux_util_set_param(UINT32 id, UINT32 param, UINT32 value)
{
	BSMUX_REC_INFO *p_rec_info = NULL;
	p_rec_info = bsmux_get_rec_info(id);
	if (bsmux_util_is_null_obj((void *)p_rec_info)) {
		DBG_ERR("[%d] get rec setting null\r\n", id);
		return E_NOEXS;
	}

	switch (param) {
	//Basic Settings: THUMB
	case BSMUXER_PARAM_THUMB_ON:
		if (value) {
			p_rec_info->thumb.thumbon = TRUE;
		} else {
			p_rec_info->thumb.thumbon = FALSE;
		}
		break;
	case BSMUXER_PARAM_THUMB_ADDR:
		p_rec_info->thumb.thumbadr = value;
		break;
	case BSMUXER_PARAM_THUMB_SIZE:
		p_rec_info->thumb.thumbsize = value;
		break;
	//Basic Settings: VID
	case BSMUXER_PARAM_VID_WIDTH:
		p_rec_info->file.vid.width = value;
		break;
	case BSMUXER_PARAM_VID_HEIGHT:
		p_rec_info->file.vid.height = value;
		break;
	case BSMUXER_PARAM_VID_VFR:
		p_rec_info->file.vid.vfr = value;
		break;
	case BSMUXER_PARAM_VID_VAR_VFR:
		p_rec_info->file.vid.var_vfr = value;
		break;
	case BSMUXER_PARAM_VID_GOP:
		p_rec_info->file.vid.gop = value;
		break;
	case BSMUXER_PARAM_VID_TBR:
		p_rec_info->file.vid.tbr = value;
		break;
	case BSMUXER_PARAM_VID_CODECTYPE:
		p_rec_info->file.vid.vidcodec = value;
		break;
	case BSMUXER_PARAM_VID_DESCADDR:
		p_rec_info->file.vid.descAddr = value;
		break;
	case BSMUXER_PARAM_VID_DESCSIZE:
		p_rec_info->file.vid.descSize = value;
		break;
	case BSMUXER_PARAM_VID_DAR:
		p_rec_info->file.vid.DAR = value;
		break;
	case BSMUXER_PARAM_VID_NALUNUM:
		p_rec_info->file.vid.naluNum = value;
		break;
	case BSMUXER_PARAM_VID_VPSSIZE:
		p_rec_info->file.vid.vpsSize = value;
		break;
	case BSMUXER_PARAM_VID_SPSSIZE:
		p_rec_info->file.vid.spsSize = value;
		break;
	case BSMUXER_PARAM_VID_PPSSIZE:
		p_rec_info->file.vid.ppsSize = value;
		break;
	//Basic Settings: SUB VID
	case BSMUXER_PARAM_VID_SUB_WIDTH:
		p_rec_info->file.sub_vid.width = value;
		break;
	case BSMUXER_PARAM_VID_SUB_HEIGHT:
		p_rec_info->file.sub_vid.height = value;
		break;
	case BSMUXER_PARAM_VID_SUB_VFR:
		p_rec_info->file.sub_vid.vfr = value;
		break;
	case BSMUXER_PARAM_VID_SUB_GOP:
		p_rec_info->file.sub_vid.gop = value;
		break;
	case BSMUXER_PARAM_VID_SUB_TBR:
		p_rec_info->file.sub_vid.tbr = value;
		break;
	case BSMUXER_PARAM_VID_SUB_CODECTYPE:
		p_rec_info->file.sub_vid.vidcodec = value;
		break;
	case BSMUXER_PARAM_VID_SUB_DESCADDR:
		p_rec_info->file.sub_vid.descAddr = value;
		break;
	case BSMUXER_PARAM_VID_SUB_DESCSIZE:
		p_rec_info->file.sub_vid.descSize = value;
		break;
	case BSMUXER_PARAM_VID_SUB_DAR:
		p_rec_info->file.sub_vid.DAR = value;
		break;
	case BSMUXER_PARAM_VID_SUB_NALUNUM:
		p_rec_info->file.sub_vid.naluNum = value;
		break;
	case BSMUXER_PARAM_VID_SUB_VPSSIZE:
		p_rec_info->file.sub_vid.vpsSize = value;
		break;
	case BSMUXER_PARAM_VID_SUB_SPSSIZE:
		p_rec_info->file.sub_vid.spsSize = value;
		break;
	case BSMUXER_PARAM_VID_SUB_PPSSIZE:
		p_rec_info->file.sub_vid.ppsSize = value;
		break;
	//Basic Settings: AUD
	case BSMUXER_PARAM_AUD_CODECTYPE:
		p_rec_info->file.aud.codectype = value;
		break;
	case BSMUXER_PARAM_AUD_SR:
		p_rec_info->file.aud.asr = value;
		break;
	case BSMUXER_PARAM_AUD_CHS:
		p_rec_info->file.aud.chs = value;
		break;
	case BSMUXER_PARAM_AUD_ON:
		p_rec_info->file.aud.aud_en = value;
		break;
	case BSMUXER_PARAM_AUD_EN_ADTS:
		if (value) {
			p_rec_info->file.aud.adts_bytes = value;
		} else {
			p_rec_info->file.aud.adts_bytes = 0;
		}
		break;
	//Basic Settings: FILE
	case BSMUXER_PARAM_FILE_EMRON:
		p_rec_info->file.emron = value;
		break;
	case BSMUXER_PARAM_FILE_EMRLOOP:
		p_rec_info->file.emrloop = value;
		break;
	case BSMUXER_PARAM_FILE_STRGID:
		p_rec_info->file.strgid = value;
		break;
	case BSMUXER_PARAM_FILE_DDR_ID:
		p_rec_info->file.ddr_id = value;
		break;
	case BSMUXER_PARAM_FILE_SEAMLESSSEC:
		p_rec_info->file.seamlessSec = value;
		break;
	case BSMUXER_PARAM_FILE_ROLLBACKSEC:
		p_rec_info->file.rollbacksec = value;
		break;
	case BSMUXER_PARAM_FILE_KEEPSEC:
		p_rec_info->file.keepsec = value;
		break;
	case BSMUXER_PARAM_FILE_ENDTYPE: //MOVREC_ENDTYPE_CUTOVERLAP
		p_rec_info->file.endtype = value;
		break;
	case BSMUXER_PARAM_FILE_FILETYPE: //MEDIA_FILEFORMAT_MP4
		p_rec_info->file.filetype = value;
		break;
	case BSMUXER_PARAM_FILE_RECFORMAT:
		p_rec_info->file.recformat = value;
		if (p_rec_info->file.recformat == MEDIAREC_AUD_VID_BOTH) {
			p_rec_info->file.aud.aud_en = TRUE;
		}
		if (p_rec_info->file.recformat == MEDIAREC_TIMELAPSE) {
			p_rec_info->file.aud.aud_en = FALSE;
		}
		if (p_rec_info->file.recformat == MEDIAREC_GOLFSHOT) {
			p_rec_info->file.aud.aud_en = FALSE;
		}
		break;
	case BSMUXER_PARAM_FILE_PLAYFRAMERATE:
		p_rec_info->file.playvfr = value;
		break;
	case BSMUXER_PARAM_FILE_BUFRESSEC:
		p_rec_info->file.revsec = value;
		break;
	case BSMUXER_PARAM_FILE_OVERLAP_ON:
		if (value) {
			p_rec_info->file.overlop_on = TRUE;
		} else {
			p_rec_info->file.overlop_on = FALSE;
		}
		break;
	case BSMUXER_PARAM_FILE_PAUSE_ON:
		p_rec_info->file.pause_on = value;
		break;
	case BSMUXER_PARAM_FILE_PAUSE_ID:
		p_rec_info->file.pause_id = value;
		break;
	case BSMUXER_PARAM_FILE_PAUSE_CNT:
		p_rec_info->file.pause_cnt = value;
		break;
	case BSMUXER_PARAM_FILE_SEAMLESSSEC_MS:
		p_rec_info->file.seamlessSec_ms = value;
		break;
	case BSMUXER_PARAM_FILE_ROLLBACKSEC_MS:
		p_rec_info->file.rollbacksec_ms = value;
		break;
	case BSMUXER_PARAM_FILE_KEEPSEC_MS:
		p_rec_info->file.keepsec_ms = value;
		break;
	case BSMUXER_PARAM_FILE_BUFRESSEC_MS:
		p_rec_info->file.revsec_ms = value;
		break;
	//Basic Settings: BUF
	case BSMUXER_PARAM_BUF_VIDENC_ADDR:
		p_rec_info->buf.videnc_phy_addr = value;
		break;
	case BSMUXER_PARAM_BUF_VIDENC_VIRT:
		p_rec_info->buf.videnc_virt_addr = value;
		break;
	case BSMUXER_PARAM_BUF_VIDENC_SIZE:
		p_rec_info->buf.videnc_size = value;
		break;
	case BSMUXER_PARAM_BUF_AUDENC_ADDR:
		p_rec_info->buf.audenc_phy_addr = value;
		break;
	case BSMUXER_PARAM_BUF_AUDENC_VIRT:
		p_rec_info->buf.audenc_virt_addr = value;
		break;
	case BSMUXER_PARAM_BUF_AUDENC_SIZE:
		p_rec_info->buf.audenc_size = value;
		break;
	case BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR:
		p_rec_info->buf.sub_videnc_phy_addr = value;
		break;
	case BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT:
		p_rec_info->buf.sub_videnc_virt_addr = value;
		break;
	case BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE:
		p_rec_info->buf.sub_videnc_size = value;
		break;
	//Basic Settings: HDR
	case BSMUXER_PARAM_HDR_FRAMEBUF_ADDR:
		p_rec_info->hdr.framebuf = value;
		break;
	case BSMUXER_PARAM_HDR_FRAMEBUF_SIZE:
		p_rec_info->hdr.framesize = value;
		break;
	case BSMUXER_PARAM_HDR_FRONTHDR_ADDR:
		p_rec_info->hdr.frontheader = value;
		break;
	case BSMUXER_PARAM_HDR_TEMPHDR_ADDR:
		p_rec_info->hdr.frontheader_2 = value;
		break;
	case BSMUXER_PARAM_HDR_BACKHDR_ADDR:
		p_rec_info->hdr.backheader = value;
		break;
	case BSMUXER_PARAM_HDR_BACKHDR_SIZE:
		p_rec_info->hdr.backsize = value;
		break;
	case BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR:
		p_rec_info->hdr.frontheader_1 = value;
		break;
	case BSMUXER_PARAM_HDR_NIDX_ADDR:
		p_rec_info->hdr.nidxaddr = value;
		break;
	case BSMUXER_PARAM_HDR_GSP_ADDR:
		p_rec_info->hdr.gpsaddr = value;
		break;
	//Basic Settings: GPS
	case BSMUXER_PARAM_GPS_ON:
		p_rec_info->gps.gpson = value;
		break;
	case BSMUXER_PARAM_GPS_RATE:
		p_rec_info->gps.gps_rate = value;
		break;
	case BSMUXER_PARAM_GPS_QUEUE:
		p_rec_info->gps.gps_queue = value;
		break;
	//Basic Settings: MEM
	case BSMUXER_PARAM_MEM_ADDR:
		p_rec_info->mem.phy_addr = value;
		break;
	case BSMUXER_PARAM_MEM_VIRT:
		p_rec_info->mem.virt_addr = value;
		break;
	case BSMUXER_PARAM_MEM_SIZE:
		p_rec_info->mem.size = value;
		break;
	case BSMUXER_PARAM_PRECALC_BUFFER:
		p_rec_info->mem.calc_buf = value;
		break;
	//Basic Settings: NIDX
	case BSMUXER_PARAM_NIDX_EN:
		p_rec_info->nidxinfo.nidx_en = value;
		break;
	case BSMUXER_PARAM_NIDX_VFN:
		p_rec_info->nidxinfo.nidxok_vfn = value;
		break;
	case BSMUXER_PARAM_NIDX_SUB_VFN:
		p_rec_info->nidxinfo.nidxok_sub_vfn = value;
		break;
	case BSMUXER_PARAM_NIDX_AFN:
		p_rec_info->nidxinfo.nidxok_afn = value;
		break;
	//Utility: USERDATA
	case BSMUXER_PARAM_USERDATA_ON:
		p_rec_info->userdata.on = value;
		break;
	case BSMUXER_PARAM_USERDATA_ADDR:
		p_rec_info->userdata.addr = value;
		break;
	case BSMUXER_PARAM_USERDATA_SIZE:
		p_rec_info->userdata.size = value;
		break;
	//Utility: CUSTDATA
	case BSMUXER_PARAM_CUSTDATA_ADDR:
		p_rec_info->custdata.addr = value;
		break;
	case BSMUXER_PARAM_CUSTDATA_SIZE:
		p_rec_info->custdata.size = value;
		break;
	case BSMUXER_PARAM_CUSTDATA_TAG:
		p_rec_info->custdata.tag = value;
		break;
	//Utility: WRINFO
	case BSMUXER_PARAM_WRINFO_WRBLK_CNT:
		p_rec_info->wrinfo.wrblk_count = value;
		break;
	case BSMUXER_PARAM_WRINFO_FLUSH_FREQ:
		p_rec_info->wrinfo.flush_freq = value;
		break;
	case BSMUXER_PARAM_WRINFO_WRBLK_SIZE:
		p_rec_info->wrinfo.wrblk_size = value;
		break;
	case BSMUXER_PARAM_WRINFO_CLOSE_FLAG:
		p_rec_info->wrinfo.close_flag = value;
		break;
	case BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC:
		p_rec_info->wrinfo.wrblk_locksec = value;
		break;
	case BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT:
		p_rec_info->wrinfo.wrblk_timeout = value;
		break;
	//Utility: EXTINFO
	case BSMUXER_PARAM_EXTINFO_UNIT:
		p_rec_info->extinfo.unit = value;
		break;
	case BSMUXER_PARAM_EXTINFO_MAX_NUM:
		p_rec_info->extinfo.max_num = value;
		break;
	case BSMUXER_PARAM_EXTINFO_ENABLE:
		p_rec_info->extinfo.enable = value;
		break;
	//Utility: MOOVINFO
	case BSMUXER_PARAM_FRONT_MOOV:
		p_rec_info->moov.fmoov_on = value;
		break;
	case BSMUXER_PARAM_MOOV_ADDR:
		p_rec_info->moov.moov_addr = value;
		break;
	case BSMUXER_PARAM_MOOV_SIZE:
		p_rec_info->moov.moov_size = value;
		break;
	case BSMUXER_PARAM_MOOV_FREQ:
		p_rec_info->moov.moov_freq = value;
		break;
	case BSMUXER_PARAM_MOOV_TUNE:
		p_rec_info->moov.moov_tune = value;
		break;
	//Utility: DROPINFO
	case BSMUXER_PARAM_EN_DROP:
		p_rec_info->drop.enable = value;
		break;
	case BSMUXER_PARAM_VID_DROP:
		p_rec_info->drop.vidcount = value;
		break;
	case BSMUXER_PARAM_AUD_DROP:
		p_rec_info->drop.audcount = value;
		break;
	case BSMUXER_PARAM_SUB_DROP:
		p_rec_info->drop.subcount = value;
		break;
	case BSMUXER_PARAM_DAT_DROP:
		p_rec_info->drop.datcount = value;
		break;
	case BSMUXER_PARAM_VID_SET:
		p_rec_info->drop.vid_drop_set = value;
		break;
	case BSMUXER_PARAM_AUD_SET:
		p_rec_info->drop.aud_drop_set = value;
		break;
	case BSMUXER_PARAM_SUB_SET:
		p_rec_info->drop.sub_drop_set = value;
		break;
	case BSMUXER_PARAM_DAT_SET:
		p_rec_info->drop.dat_drop_set = value;
		break;
	case BSMUXER_PARAM_FULL_SET:
		p_rec_info->drop.full_drop_set = value;
		break;
	//Utility: UTC
	case BSMUXER_PARAM_UTC_SIGN:
		p_rec_info->zone.utc_sign = value;
		break;
	case BSMUXER_PARAM_UTC_ZONE:
		p_rec_info->zone.utc_zone = value;
		break;
	case BSMUXER_PARAM_UTC_TIME:
		// directly set to maker
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_UTC_TIMEINFO, value, 0, id);
		break;
	//Utility: ALIGN
	case BSMUXER_PARAM_MUXALIGN:
		p_rec_info->mux_align = value;
		break;
	case BSMUXER_PARAM_MUXMETHOD:
		p_rec_info->mux_method = value;
		break;
	//Utility: META
	case BSMUXER_PARAM_META_ON:
		p_rec_info->meta.meta_on = value;
		break;
	case BSMUXER_PARAM_META_NUM:
		p_rec_info->meta.meta_num = value;
		break;
	case BSMUXER_PARAM_META_DATA:
		{
			BSMUX_REC_METADATA *p_meta = (BSMUX_REC_METADATA *)value;
			UINT32 index = p_meta->meta_index;
			p_rec_info->meta.meta_data[index].meta_sign = p_meta->meta_sign;
			p_rec_info->meta.meta_data[index].meta_rate = p_meta->meta_rate;
			p_rec_info->meta.meta_data[index].meta_queue = p_meta->meta_queue;
			p_rec_info->meta.meta_data[index].meta_index = p_meta->meta_index;
			DBG_DUMP("set: [%d] sign=0x%x rate=%d queue=%d\r\n",
				p_rec_info->meta.meta_data[index].meta_index,
				p_rec_info->meta.meta_data[index].meta_sign,
				p_rec_info->meta.meta_data[index].meta_rate,
				p_rec_info->meta.meta_data[index].meta_queue);
		}
		break;
	//Others
	case BSMUXER_PARAM_FREASIZE:
		p_rec_info->freasize = value;
		break;
	case BSMUXER_PARAM_FREAENDSIZE:
		p_rec_info->freaendsize = value;
		break;
	case BSMUXER_PARAM_EN_FREABOX:
		p_rec_info->freabox_en = value;
		break;
	case BSMUXER_PARAM_EN_FASTPUT:
		p_rec_info->fastput_en = value;
		break;
	case BSMUXER_PARAM_DUR_US_MAX:
		p_rec_info->dur_us_max = value;
		break;
	case BSMUXER_PARAM_BOXTAG_SIZE:
		p_rec_info->boxtag_size = value;
		break;
	case BSMUXER_PARAM_PTS_RESET:
		p_rec_info->pts_reset = value;
		break;
	case BSMUXER_PARAM_EN_STRGBUF:
		p_rec_info->strgbuf_info.en = value;
		break;
	case BSMUXER_PARAM_STRGBUF_ACT:
		p_rec_info->strgbuf_info.active = value;
		break;
	case BSMUXER_PARAM_STRGBUF_CUT:
		p_rec_info->strgbuf_info.cut = value;
		break;
	case BSMUXER_PARAM_STRGBUF_HDR:
		p_rec_info->strgbuf_info.is_hdr = value;
		break;
	case BSMUXER_PARAM_STRGBUF_VID:
		p_rec_info->strgbuf_info.vid_ok = value;
		break;
	case BSMUXER_PARAM_STRGBUF_VID_NUM:
		p_rec_info->strgbuf_info.vid_entry_num = value;
		break;
	case BSMUXER_PARAM_STRGBUF_AUD_NUM:
		p_rec_info->strgbuf_info.aud_entry_num = value;
		break;
	case BSMUXER_PARAM_STRGBUF_CUR_POS:
		p_rec_info->strgbuf_info.cur_entry_pos = value;
		break;
	case BSMUXER_PARAM_STRGBUF_MAX_NUM:
		p_rec_info->strgbuf_info.max_entry_num = value;
		break;
	case BSMUXER_PARAM_STRGBUF_ALLOC_SIZE:
		p_rec_info->strgbuf_info.alloc_size = value;
		break;
	case BSMUXER_PARAM_STRGBUF_TOTAL_SIZE:
		p_rec_info->strgbuf_info.total_size = value;
		break;
	case BSMUXER_PARAM_STRGBUF_PUT_POS:
		p_rec_info->strgbuf_info.put_pos = value;
		break;
	case BSMUXER_PARAM_STRGBUF_GET_POS:
		p_rec_info->strgbuf_info.get_pos = value;
		break;

	default:
		break;
	}

	return E_OK;
}

ER bsmux_util_get_param(UINT32 id, UINT32 param, UINT32 *p_value)
{
	BSMUX_REC_INFO *p_rec_info = NULL;
	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("[%d] invalid\r\n", id);
		return E_NOEXS;
	}
	if (bsmux_util_is_null_obj((void *)p_value)) {
		DBG_ERR("[%d] get p_value null\r\n", id);
		return E_NOEXS;
	}
	p_rec_info = bsmux_get_rec_info(id);
	if (bsmux_util_is_null_obj((void *)p_rec_info)) {
		DBG_ERR("[%d] get rec setting null\r\n", id);
		return E_NOEXS;
	}

	switch (param) {
	//Basic Settings: THUMB
	case BSMUXER_PARAM_THUMB_ON:
		*p_value = p_rec_info->thumb.thumbon;
		break;
	case BSMUXER_PARAM_THUMB_ADDR:
		*p_value = p_rec_info->thumb.thumbadr;
		break;
	case BSMUXER_PARAM_THUMB_SIZE:
		*p_value = p_rec_info->thumb.thumbsize;
		break;
	//Basic Settings: VID
	case BSMUXER_PARAM_VID_WIDTH:
		*p_value = p_rec_info->file.vid.width;
		break;
	case BSMUXER_PARAM_VID_HEIGHT:
		*p_value = p_rec_info->file.vid.height;
		break;
	case BSMUXER_PARAM_VID_VFR:
		*p_value = p_rec_info->file.vid.vfr;
		break;
	case BSMUXER_PARAM_VID_VAR_VFR:
		*p_value = p_rec_info->file.vid.var_vfr;
		break;
	case BSMUXER_PARAM_VID_GOP:
		*p_value = p_rec_info->file.vid.gop;
		break;
	case BSMUXER_PARAM_VID_TBR:
		*p_value = p_rec_info->file.vid.tbr;
		break;
	case BSMUXER_PARAM_VID_CODECTYPE:
		*p_value = p_rec_info->file.vid.vidcodec;
		break;
	case BSMUXER_PARAM_VID_DESCADDR:
		*p_value = p_rec_info->file.vid.descAddr;
		break;
	case BSMUXER_PARAM_VID_DESCSIZE:
		*p_value = p_rec_info->file.vid.descSize;
		break;
	case BSMUXER_PARAM_VID_DAR:
		*p_value = p_rec_info->file.vid.DAR;
		break;
	case BSMUXER_PARAM_VID_NALUNUM:
		*p_value = p_rec_info->file.vid.naluNum;
		break;
	case BSMUXER_PARAM_VID_VPSSIZE:
		*p_value = p_rec_info->file.vid.vpsSize;
		break;
	case BSMUXER_PARAM_VID_SPSSIZE:
		*p_value = p_rec_info->file.vid.spsSize;
		break;
	case BSMUXER_PARAM_VID_PPSSIZE:
		*p_value = p_rec_info->file.vid.ppsSize;
		break;
	//Basic Settings: SUB VID
	case BSMUXER_PARAM_VID_SUB_WIDTH:
		*p_value = p_rec_info->file.sub_vid.width;
		break;
	case BSMUXER_PARAM_VID_SUB_HEIGHT:
		*p_value = p_rec_info->file.sub_vid.height;
		break;
	case BSMUXER_PARAM_VID_SUB_VFR:
		*p_value = p_rec_info->file.sub_vid.vfr;
		break;
	case BSMUXER_PARAM_VID_SUB_GOP:
		*p_value = p_rec_info->file.sub_vid.gop;
		break;
	case BSMUXER_PARAM_VID_SUB_TBR:
		*p_value = p_rec_info->file.sub_vid.tbr;
		break;
	case BSMUXER_PARAM_VID_SUB_CODECTYPE:
		*p_value = p_rec_info->file.sub_vid.vidcodec;
		break;
	case BSMUXER_PARAM_VID_SUB_DESCADDR:
		*p_value = p_rec_info->file.sub_vid.descAddr;
		break;
	case BSMUXER_PARAM_VID_SUB_DESCSIZE:
		*p_value = p_rec_info->file.sub_vid.descSize;
		break;
	case BSMUXER_PARAM_VID_SUB_DAR:
		*p_value = p_rec_info->file.sub_vid.DAR;
		break;
	case BSMUXER_PARAM_VID_SUB_NALUNUM:
		*p_value = p_rec_info->file.sub_vid.naluNum;
		break;
	case BSMUXER_PARAM_VID_SUB_VPSSIZE:
		*p_value = p_rec_info->file.sub_vid.vpsSize;
		break;
	case BSMUXER_PARAM_VID_SUB_SPSSIZE:
		*p_value = p_rec_info->file.sub_vid.spsSize;
		break;
	case BSMUXER_PARAM_VID_SUB_PPSSIZE:
		*p_value = p_rec_info->file.sub_vid.ppsSize;
		break;
	//Basic Settings: AUD
	case BSMUXER_PARAM_AUD_CODECTYPE:
		*p_value = p_rec_info->file.aud.codectype;
		break;
	case BSMUXER_PARAM_AUD_SR:
		*p_value = p_rec_info->file.aud.asr;
		break;
	case BSMUXER_PARAM_AUD_CHS:
		*p_value = p_rec_info->file.aud.chs;
		break;
	case BSMUXER_PARAM_AUD_ON:
		*p_value = p_rec_info->file.aud.aud_en;
		break;
	case BSMUXER_PARAM_AUD_EN_ADTS:
		*p_value = p_rec_info->file.aud.adts_bytes;
		break;
	//Basic Settings: FILE
	case BSMUXER_PARAM_FILE_EMRON:
		*p_value = p_rec_info->file.emron;
		break;
	case BSMUXER_PARAM_FILE_EMRLOOP:
		*p_value = p_rec_info->file.emrloop;
		break;
	case BSMUXER_PARAM_FILE_STRGID:
		*p_value = p_rec_info->file.strgid;
		break;
	case BSMUXER_PARAM_FILE_DDR_ID:
		*p_value = p_rec_info->file.ddr_id;
		break;
	case BSMUXER_PARAM_FILE_SEAMLESSSEC:
		*p_value = p_rec_info->file.seamlessSec;
		break;
	case BSMUXER_PARAM_FILE_ROLLBACKSEC:
		*p_value = p_rec_info->file.rollbacksec;
		break;
	case BSMUXER_PARAM_FILE_KEEPSEC:
		*p_value = p_rec_info->file.keepsec;
		break;
	case BSMUXER_PARAM_FILE_ENDTYPE:
		*p_value = p_rec_info->file.endtype;
		break;
	case BSMUXER_PARAM_FILE_FILETYPE:
		*p_value = p_rec_info->file.filetype;
		break;
	case BSMUXER_PARAM_FILE_RECFORMAT:
		*p_value = p_rec_info->file.recformat;
		break;
	case BSMUXER_PARAM_FILE_PLAYFRAMERATE:
		*p_value = p_rec_info->file.playvfr;
		break;
	case BSMUXER_PARAM_FILE_BUFRESSEC:
		*p_value = p_rec_info->file.revsec;
		break;
	case BSMUXER_PARAM_FILE_OVERLAP_ON:
		*p_value = p_rec_info->file.overlop_on;
		break;
	case BSMUXER_PARAM_FILE_PAUSE_ON:
		*p_value = p_rec_info->file.pause_on;
		break;
	case BSMUXER_PARAM_FILE_PAUSE_ID:
		*p_value = p_rec_info->file.pause_id;
		break;
	case BSMUXER_PARAM_FILE_PAUSE_CNT:
		*p_value = p_rec_info->file.pause_cnt;
		break;
	case BSMUXER_PARAM_FILE_SEAMLESSSEC_MS:
		*p_value = p_rec_info->file.seamlessSec_ms;
		break;
	case BSMUXER_PARAM_FILE_ROLLBACKSEC_MS:
		*p_value = p_rec_info->file.rollbacksec_ms;
		break;
	case BSMUXER_PARAM_FILE_KEEPSEC_MS:
		*p_value = p_rec_info->file.keepsec_ms;
		break;
	case BSMUXER_PARAM_FILE_BUFRESSEC_MS:
		*p_value = p_rec_info->file.revsec_ms;
		break;
	//Basic Settings: BUF
	case BSMUXER_PARAM_BUF_VIDENC_ADDR:
		*p_value = p_rec_info->buf.videnc_phy_addr;
		break;
	case BSMUXER_PARAM_BUF_VIDENC_VIRT:
		*p_value = p_rec_info->buf.videnc_virt_addr;
		break;
	case BSMUXER_PARAM_BUF_VIDENC_SIZE:
		*p_value = p_rec_info->buf.videnc_size;
		break;
	case BSMUXER_PARAM_BUF_AUDENC_ADDR:
		*p_value = p_rec_info->buf.audenc_phy_addr;
		break;
	case BSMUXER_PARAM_BUF_AUDENC_VIRT:
		*p_value = p_rec_info->buf.audenc_virt_addr;
		break;
	case BSMUXER_PARAM_BUF_AUDENC_SIZE:
		*p_value = p_rec_info->buf.audenc_size;
		break;
	case BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR:
		*p_value = p_rec_info->buf.sub_videnc_phy_addr;
		break;
	case BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT:
		*p_value = p_rec_info->buf.sub_videnc_virt_addr;
		break;
	case BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE:
		*p_value = p_rec_info->buf.sub_videnc_size;
		break;
	//Basic Settings: HDR
	case BSMUXER_PARAM_HDR_TEMPHDR_ADDR:
		*p_value = p_rec_info->hdr.frontheader_2;
		break;
	case BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR:
		*p_value = p_rec_info->hdr.frontheader_1;
		break;
	case BSMUXER_PARAM_HDR_BACKHDR_ADDR:
		*p_value = p_rec_info->hdr.backheader;
		break;
	case BSMUXER_PARAM_HDR_BACKHDR_SIZE:
		*p_value = p_rec_info->hdr.backsize;
		break;
	case BSMUXER_PARAM_HDR_NIDX_ADDR:
		*p_value = p_rec_info->hdr.nidxaddr;
		break;
	case BSMUXER_PARAM_HDR_GSP_ADDR:
		*p_value = p_rec_info->hdr.gpsaddr;
		break;
	//Basic Settings: GPS
	case BSMUXER_PARAM_GPS_ON:
		*p_value = p_rec_info->gps.gpson;
		break;
	case BSMUXER_PARAM_GPS_RATE:
		*p_value = p_rec_info->gps.gps_rate;
		break;
	case BSMUXER_PARAM_GPS_QUEUE:
		*p_value = p_rec_info->gps.gps_queue;
		break;
	//Basic Settings: MEM
	case BSMUXER_PARAM_MEM_ADDR:
		*p_value = p_rec_info->mem.phy_addr;
		break;
	case BSMUXER_PARAM_MEM_VIRT:
		*p_value = p_rec_info->mem.virt_addr;
		break;
	case BSMUXER_PARAM_MEM_SIZE:
		*p_value = p_rec_info->mem.size;
		break;
	case BSMUXER_PARAM_PRECALC_BUFFER:
		*p_value = p_rec_info->mem.calc_buf;
		break;
	//Basic Settings: NIDX
	case BSMUXER_PARAM_NIDX_EN:
		*p_value = p_rec_info->nidxinfo.nidx_en;
		break;
	case BSMUXER_PARAM_NIDX_VFN:
		*p_value = p_rec_info->nidxinfo.nidxok_vfn;
		break;
	case BSMUXER_PARAM_NIDX_SUB_VFN:
		*p_value = p_rec_info->nidxinfo.nidxok_sub_vfn;
		break;
	case BSMUXER_PARAM_NIDX_AFN:
		*p_value = p_rec_info->nidxinfo.nidxok_afn;
		break;
	//Utility: USERDATA
	case BSMUXER_PARAM_USERDATA_ON:
		*p_value = p_rec_info->userdata.on;
		break;
	case BSMUXER_PARAM_USERDATA_ADDR:
		*p_value = p_rec_info->userdata.addr;
		break;
	case BSMUXER_PARAM_USERDATA_SIZE:
		*p_value = p_rec_info->userdata.size;
		break;
	//Utility: CUSTDATA
	case BSMUXER_PARAM_CUSTDATA_ADDR:
		*p_value = p_rec_info->custdata.addr;
		break;
	case BSMUXER_PARAM_CUSTDATA_SIZE:
		*p_value = p_rec_info->custdata.size;
		break;
	case BSMUXER_PARAM_CUSTDATA_TAG:
		*p_value = p_rec_info->custdata.tag;
		break;
	//Utility: WRINFO
	case BSMUXER_PARAM_WRINFO_WRBLK_CNT:
		*p_value = p_rec_info->wrinfo.wrblk_count;
		break;
	case BSMUXER_PARAM_WRINFO_FLUSH_FREQ:
		*p_value = p_rec_info->wrinfo.flush_freq;
		break;
	case BSMUXER_PARAM_WRINFO_WRBLK_SIZE:
		*p_value = p_rec_info->wrinfo.wrblk_size;
		break;
	case BSMUXER_PARAM_WRINFO_CLOSE_FLAG:
		*p_value = p_rec_info->wrinfo.close_flag;
		break;
	case BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC:
		*p_value = p_rec_info->wrinfo.wrblk_locksec;
		break;
	case BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT:
		*p_value = p_rec_info->wrinfo.wrblk_timeout;
		break;
	//Utility: EXTINFO
	case BSMUXER_PARAM_EXTINFO_UNIT:
		*p_value = p_rec_info->extinfo.unit;
		break;
	case BSMUXER_PARAM_EXTINFO_MAX_NUM:
		*p_value = p_rec_info->extinfo.max_num;
		break;
	case BSMUXER_PARAM_EXTINFO_ENABLE:
		*p_value = p_rec_info->extinfo.enable;
		break;
	//Utility: MOOVINFO
	case BSMUXER_PARAM_FRONT_MOOV:
		*p_value = p_rec_info->moov.fmoov_on;
		break;
	case BSMUXER_PARAM_MOOV_ADDR:
		*p_value = p_rec_info->moov.moov_addr;
		break;
	case BSMUXER_PARAM_MOOV_SIZE:
		*p_value = p_rec_info->moov.moov_size;
		break;
	case BSMUXER_PARAM_MOOV_FREQ:
		*p_value = p_rec_info->moov.moov_freq;
		break;
	case BSMUXER_PARAM_MOOV_TUNE:
		*p_value = p_rec_info->moov.moov_tune;
		break;
	//Utility: DROPINFO
	case BSMUXER_PARAM_EN_DROP:
		*p_value = p_rec_info->drop.enable;
		break;
	case BSMUXER_PARAM_VID_DROP:
		*p_value = p_rec_info->drop.vidcount;
		break;
	case BSMUXER_PARAM_AUD_DROP:
		*p_value = p_rec_info->drop.audcount;
		break;
	case BSMUXER_PARAM_SUB_DROP:
		*p_value = p_rec_info->drop.subcount;
		break;
	case BSMUXER_PARAM_DAT_DROP:
		*p_value = p_rec_info->drop.datcount;
		break;
	case BSMUXER_PARAM_VID_SET:
		*p_value = p_rec_info->drop.vid_drop_set;
		break;
	case BSMUXER_PARAM_AUD_SET:
		*p_value = p_rec_info->drop.aud_drop_set;
		break;
	case BSMUXER_PARAM_SUB_SET:
		*p_value = p_rec_info->drop.sub_drop_set;
		break;
	case BSMUXER_PARAM_DAT_SET:
		*p_value = p_rec_info->drop.dat_drop_set;
		break;
	case BSMUXER_PARAM_FULL_SET:
		*p_value = p_rec_info->drop.full_drop_set;
		break;
	//Utility: UTC
	case BSMUXER_PARAM_UTC_SIGN:
		*p_value = p_rec_info->zone.utc_sign;
		break;
	case BSMUXER_PARAM_UTC_ZONE:
		*p_value = p_rec_info->zone.utc_zone;
		break;
	case BSMUXER_PARAM_UTC_TIME:
		// unsupported
		break;
	//Utility: ALIGN
	case BSMUXER_PARAM_MUXALIGN:
		*p_value = p_rec_info->mux_align;
		break;
	case BSMUXER_PARAM_MUXMETHOD:
		*p_value = p_rec_info->mux_method;
		break;
	//Utility: META
	case BSMUXER_PARAM_META_ON:
		*p_value = p_rec_info->meta.meta_on;
		break;
	case BSMUXER_PARAM_META_NUM:
		*p_value = p_rec_info->meta.meta_num;
		break;
	case BSMUXER_PARAM_META_DATA:
		{
			BSMUX_REC_METADATA *p_meta = (BSMUX_REC_METADATA *)p_value;
			UINT32 index = p_meta->meta_index;
			p_meta->meta_sign = p_rec_info->meta.meta_data[index].meta_sign;
			p_meta->meta_rate = p_rec_info->meta.meta_data[index].meta_rate;
			p_meta->meta_queue = p_rec_info->meta.meta_data[index].meta_queue;
			DBG_DUMP("get: [%d] sign=0x%x rate=%d queue=%d\r\n",
				p_meta->meta_index,
				p_meta->meta_sign,
				p_meta->meta_rate,
				p_meta->meta_queue);
		}
		break;
	//Others
	case BSMUXER_PARAM_FREASIZE:
		*p_value = p_rec_info->freasize;
		break;
	case BSMUXER_PARAM_FREAENDSIZE:
		*p_value = p_rec_info->freaendsize;
		break;
	case BSMUXER_PARAM_EN_FREABOX:
		*p_value = p_rec_info->freabox_en;
		break;
	case BSMUXER_PARAM_EN_FASTPUT:
		*p_value = p_rec_info->fastput_en;
		break;
	case BSMUXER_PARAM_DUR_US_MAX:
		*p_value = p_rec_info->dur_us_max;
		break;
	case BSMUXER_PARAM_BOXTAG_SIZE:
		*p_value = p_rec_info->boxtag_size;
		break;
	case BSMUXER_PARAM_PTS_RESET:
		*p_value = p_rec_info->pts_reset;
		break;
	case BSMUXER_PARAM_EN_STRGBUF:
		*p_value = p_rec_info->strgbuf_info.en;
		break;
	case BSMUXER_PARAM_STRGBUF_ACT:
		*p_value = p_rec_info->strgbuf_info.active;
		break;
	case BSMUXER_PARAM_STRGBUF_CUT:
		*p_value = p_rec_info->strgbuf_info.cut;
		break;
	case BSMUXER_PARAM_STRGBUF_HDR:
		*p_value = p_rec_info->strgbuf_info.is_hdr;
		break;
	case BSMUXER_PARAM_STRGBUF_VID:
		*p_value = p_rec_info->strgbuf_info.vid_ok;
		break;
	case BSMUXER_PARAM_STRGBUF_VID_NUM:
		*p_value = p_rec_info->strgbuf_info.vid_entry_num;
		break;
	case BSMUXER_PARAM_STRGBUF_AUD_NUM:
		*p_value = p_rec_info->strgbuf_info.aud_entry_num;
		break;
	case BSMUXER_PARAM_STRGBUF_CUR_POS:
		*p_value = p_rec_info->strgbuf_info.cur_entry_pos;
		break;
	case BSMUXER_PARAM_STRGBUF_MAX_NUM:
		*p_value = p_rec_info->strgbuf_info.max_entry_num;
		break;
	case BSMUXER_PARAM_STRGBUF_ALLOC_SIZE:
		*p_value = p_rec_info->strgbuf_info.alloc_size;
		break;
	case BSMUXER_PARAM_STRGBUF_TOTAL_SIZE:
		*p_value = p_rec_info->strgbuf_info.total_size;
		break;
	case BSMUXER_PARAM_STRGBUF_PUT_POS:
		*p_value = p_rec_info->strgbuf_info.put_pos;
		break;
	case BSMUXER_PARAM_STRGBUF_GET_POS:
		*p_value = p_rec_info->strgbuf_info.get_pos;
		break;

	default:
		break;
	}

	return E_OK;
}

ER bsmux_util_wait_ready(UINT32 id)
{
	UINT32 end2card = 0, real2card = 0;
	UINT32 cnt = 200;
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_END2CARD, &end2card);
	if (end2card == real2card) {
		DBG_DUMP("BSM:[%d] wait ready ok\r\n", id);
		//blkPut
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, FALSE);
		return E_OK;
	}
	while (cnt) {
		//polling file end
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_END2CARD, &end2card);
		if (end2card == real2card) {
			DBG_DUMP("BSM:[%d] wait file end ok\r\n", id);
			return E_OK;
		}
		vos_util_delay_ms(300);
		DBG_DUMP(".");
		cnt -= 1;
	}
	DBG_WRN("[%d] wait ready fail (end 0x%X real 0x%X)\r\n", id, end2card, real2card);
	//blkPut
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, FALSE);
	return E_SYS;
}

ER bsmux_util_prepare_buf(UINT32 id, UINT32 type, UINT32 addr, UINT64 size, UINT64 pos)
{
	BSMUXER_FILE_BUF action_buf = {0};
	UINT32 emron = 0, emrloop = 0, filetype = 0;
	UINT32 event = 0, fileop = 0;
	UINT32 flush_freq = 0, wrblk_count = 0;
	UINT32 cut_blk = 0;
#if BSMUX_TEST_MOOV_RC
	UINT32 put_vfn = 0, put_afn = 0, put_sub = 0;;
#endif

	bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_FLUSH_FREQ, &flush_freq);
	bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_WRBLK_CNT, &wrblk_count);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
	if (emron) {
		if (emrloop == BSMUX_STEP_EMRLOOP) {
			event = BSMUXER_FEVENT_NORMAL;
		} else {
			event = BSMUXER_FEVENT_EMR;
		}
	} else {
		event = BSMUXER_FEVENT_NORMAL;
	}

	switch (type) {
	case BSMUX_BS2CARD_NORMAL:
		fileop = BSMUXER_FOP_CONT_WRITE;
		if (flush_freq) {
			if ((wrblk_count % flush_freq) == 0) {
				fileop |= BSMUXER_FOP_FLUSH;
			}
		} else {
			fileop |= BSMUXER_FOP_FLUSH;
		}
		wrblk_count++;
		break;
	case BSMUX_BS2CARD_FRONT:
		fileop = HD_BSMUX_FOP_CREATE | HD_BSMUX_FOP_CONT_WRITE;
		fileop |= BSMUXER_FOP_FLUSH;
		wrblk_count++;
		break;
	case BSMUX_BS2CARD_BACK:
		fileop = HD_BSMUX_FOP_CONT_WRITE;
		break;
	case BSMUX_BS2CARD_TEMP:
		fileop = BSMUXER_FOP_SEEK_WRITE | BSMUXER_FOP_FLUSH | BSMUXER_FOP_CLOSE;
		wrblk_count = 0;
		break;
	case BSMUX_BS2CARD_LAST:
		fileop = BSMUXER_FOP_FLUSH | BSMUXER_FOP_CLOSE;
		break;
	case BSMUX_BS2CARD_MOOV:
	case BSMUX_BS2CARD_THUMB:
		fileop = BSMUXER_FOP_SEEK_WRITE | BSMUXER_FOP_FLUSH;
		break;
	case BSMUX_BS2STRG_WRITE:
		event = BSMUXER_FEVENT_BSINCARD;
		fileop = HD_BSMUX_FOP_CONT_WRITE;
		break;
	case BSMUX_BS2STRG_SYNC:
		event = BSMUXER_FEVENT_BSINCARD;
		fileop = BSMUXER_FOP_SEEK_WRITE;
		break;
	case BSMUX_BS2STRG_FLUSH:
		event = BSMUXER_FEVENT_BSINCARD;
		fileop = BSMUXER_FOP_FLUSH;
		break;
	case BSMUX_BS2STRG_READ:
		fileop = BSMUXER_FOP_READ;
		break;
	default:
		return E_SYS;
		break;
	}

	bsmux_util_set_param(id, BSMUXER_PARAM_WRINFO_WRBLK_CNT, wrblk_count);

	if (fileop & HD_BSMUX_FOP_CREATE) {
		bsmux_util_set_param(id, BSMUXER_PARAM_WRINFO_CLOSE_FLAG, FALSE);
	}
	if (fileop & BSMUXER_FOP_CLOSE) {
		bsmux_util_set_param(id, BSMUXER_PARAM_WRINFO_CLOSE_FLAG, TRUE);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_CUTBLK, &cut_blk);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_CUTBLK, (cut_blk+1));
	}

	action_buf.pathID  = id;
	action_buf.event   = event;
	action_buf.fileop  = fileop;
	action_buf.addr    = addr;
	action_buf.size    = size;
	action_buf.pos     = pos;
	action_buf.type    = filetype;

#if BSMUX_TEST_MOOV_RC
	if (type == BSMUX_BS2CARD_MOOV) {
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_PUT_VF, &put_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_PUT_AF, &put_afn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_PUT_SUBVF, &put_sub);
		DBG_IND("[%d] prepare moov buffer(0x%lx)(vfn=%d afn=%d sub=%d)\r\n", id, action_buf.addr, put_vfn, put_afn, put_sub);
	}
	if (type == BSMUX_BS2CARD_NORMAL || type == BSMUX_BS2CARD_FRONT) {
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &put_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &put_afn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &put_sub);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_PUT_VF, put_vfn);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_PUT_AF, put_afn);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_PUT_SUBVF, put_sub);
		DBG_IND("[%d] prepare normal buffer(0x%lx)(vfn=%d afn=%d sub=%d)\r\n", id, action_buf.addr, put_vfn, put_afn, put_sub);
	}
#endif

	DBG_IND(">>> bsmux_util_prepare_buf addr 0x%X size 0x%X pos 0x%X\r\n", addr, size, pos);

	if (type == BSMUX_BS2CARD_NORMAL || type == BSMUX_BS2CARD_FRONT) {
		// critical section: lock
		bsmux_buffer_lock();
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_USEDBLOCK, &cut_blk);
		cut_blk++;
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_USEDBLOCK, cut_blk);
		bsmux_buffer_unlock();
		// critical section: unlock
		UTIL_MEM_FUNC("%s: id(%d) USEDBLOCK[%d]\r\n", __func__, id, cut_blk);
	}

	// callback
	bsmux_cb_buf(&action_buf, id);
	bsmux_ctrl_mem_det_range(id);

	return E_OK;
}

ER bsmux_util_release_buf(BSMUXER_FILE_BUF *p_buf)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->ReleaseBuf((void *)p_buf);
	if (E_OK != result) {
		DBG_ERR("ReleaseBuf error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_add_gps(UINT32 id, void * p_bsq)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->AddGPS(id, p_bsq);
	if (E_OK != result) {
		DBG_ERR("AddGPS error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_add_thumb(UINT32 id, void * p_bsq)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->AddThumb(id, p_bsq);
	if (E_OK != result) {
		DBG_ERR("AddThumb error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_add_meta(UINT32 id, void * p_bsq)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->AddMeta(id, p_bsq);
	if (E_OK != result) {
		DBG_ERR("AddMeta error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_mem_dbg(UINT32 value)
{
	g_debug_util_mem = value;
	return E_OK;
}

ER bsmux_util_strgbuf_init(UINT32 id)
{//Init: frame entry/card space
	UINT32 is_set = 0;
	UINT32 vfr = 0, roll_sec = 0, roll_sec_ms = 0;
	UINT32 val = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &is_set);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &roll_sec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC_MS, &roll_sec_ms);
	#endif

	if (is_set) {
		//Init: status
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_CUT, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_HDR, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_VID, 0);
		//Init: frame entry
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_AUD_NUM, 0);
		val = bsmux_mdat_calc_header_size(id); //??
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_CUR_POS, val); //??
		val = roll_sec * vfr;
		if (roll_sec_ms) {
			val = (roll_sec_ms * vfr) / 1000;
		}
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_MAX_NUM, val);
		DBG_IND("[%d] StrgBuf Max Num: 0x%X\r\n", id, val);
		//Init: card space
		val = (120*1024*1024UL);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_ALLOC_SIZE, val);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_PUT_POS, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_GET_POS, 0);
		DBG_DUMP("[%d] Init Done\r\n", id);
		DBG_DUMP("[%d] StrgBuf Space Size: 0x%X\r\n", id, val);
	} else {
		DBG_DUMP("[%d] Init Nothing To Do\r\n", id);
	}

	return E_OK;
}

ER bsmux_util_strgbuf_handle_entry(UINT32 id, void *p_bsq)
{//handle entry-related
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO bsq_info = {0};
	BSMUXER_FILE_BUF add_info = {0};
	UINT32 filetype = 0;
	UINT32 vfr = 0, rollbacksec = 0;
	UINT32 max_vfn = 0, cur_pos = 0;
	UINT32 vid_entry_num = 0, aud_entry_num = 0;
	UINT32 is_cut = 0;

	// ==============================
	// =        Get  Info           =
	// ==============================
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollbacksec);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_MAX_NUM, &max_vfn);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_CUR_POS, &cur_pos);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, &vid_entry_num);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_AUD_NUM, &aud_entry_num);

	// ==============================
	// =        BSQ Info            =
	// ==============================
	bsq_info.uiBSMemAddr = pSrc->uiBSMemAddr;
	bsq_info.uiBSSize    = pSrc->uiBSSize;
	bsq_info.uiType      = pSrc->uiType;
	bsq_info.uiIsKey     = pSrc->uiIsKey;
	bsq_info.uibufid     = pSrc->uibufid;

	// ==============================
	// =      Save Entry Info       =
	// ==============================
	if (bsq_info.uiType == BSMUX_TYPE_VIDEO) {

		// ==============================
		// =          Save Entry        =
		// ==============================
		{
			MOV_Ientry thisJpg = {0};
			UINT32 nowFN = 0;
			UINT32 variable_rate;
			{
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_VAR_VFR, &variable_rate);
				if (bsq_info.uiIsKey) {
					UINT32 user_variable_rate, mux_variable_rate;
					user_variable_rate = ((variable_rate) >> 16);
					mux_variable_rate = ((variable_rate) & 0xffff);
					if ((user_variable_rate) && (mux_variable_rate != user_variable_rate)) {
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
			nowFN = vid_entry_num;
			thisJpg.size = bsq_info.uiBSSize;
			if (bsq_info.uiIsKey) {
				thisJpg.key_frame = 1;
			} else {
				thisJpg.key_frame = 0;
			}
			thisJpg.updated = 0;
			thisJpg.pos = (UINT64)cur_pos;
			thisJpg.rev = (UINT8)variable_rate;  ///< (internal) variable rate usage
			bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MOVVIDEOENTRY, nowFN, (UINT32)&thisJpg, id);
		}
		vid_entry_num += 1;
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM , vid_entry_num);
	}
	else if (bsq_info.uiType == BSMUX_TYPE_AUDIO) {

		// ==============================
		// =          Save Entry        =
		// ==============================
		{
			MOV_Ientry thisJpg = {0};
			UINT32 nowFN = 0;
			UINT32 variable_rate;
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &variable_rate);
			nowFN = aud_entry_num;
			thisJpg.size = bsq_info.uiBSSize;
			thisJpg.pos = (UINT64)cur_pos;
			thisJpg.rev = (UINT8)(variable_rate/100);  ///< (internal) variable rate usage
			bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
			if ((filetype == MEDIA_FILEFORMAT_MOV) || (filetype == MEDIA_FILEFORMAT_MP4)) {
				bsmux_util_set_minfo(MEDIAWRITE_SETINFO_MOVAUDIOENTRY, nowFN, (UINT32)&thisJpg, id);
			}
		}
		aud_entry_num += 1;
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_AUD_NUM, aud_entry_num);
	}

	// ==============================
	// =     Update cur_entry_pos   =
	// ==============================
	cur_pos += bsmux_util_muxalign(id, bsq_info.uiBSSize);
	bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_CUR_POS, cur_pos);

	// ==============================
	// =          Sync Card         =
	// ==============================
	if (bsq_info.uiType == BSMUX_TYPE_VIDEO) {
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, &vid_entry_num);

		//2. use flush to update/sync get_pos
		if (vid_entry_num == max_vfn) {
			bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_CUT , &is_cut);
			if (is_cut) {

			} else {
				UINT32 shift_size = 0;
				UINT32 duration = rollbacksec;
				bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_SYNCPOSFROMHEAD, &id, &duration, &shift_size);
				bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_VIDENTRYNUM, &id, 0, &vid_entry_num);
				bsmux_util_get_minfo(id, MEDIAWRITE_GETINFO_AUDENTRYNUM, &id, 0, &aud_entry_num);
				bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, vid_entry_num);
				bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_AUD_NUM, aud_entry_num);

				add_info.fileop = 0;
				add_info.fileop |= BSMUXER_FOP_FLUSH;
				add_info.size = shift_size;
				DBG_MSG("[%d] SHIFT Get sysinfo size(%d) vfn(%d) afn(%d)\r\n", id, shift_size, vid_entry_num, aud_entry_num);
				bsmux_util_strgbuf_handle_space(id, (void *)&add_info);

				bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_CUR_POS, &cur_pos);
				cur_pos -= shift_size;
				bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_CUR_POS, cur_pos);
			}
		}
	}

	return E_OK;
}

ER bsmux_util_strgbuf_handle_buffer(UINT32 id, void *p_bsq)
{//handle entry-related
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 is_cut = 0, vid_ok = 0; //strgbuf info
	UINT32 vfn = 0, afn = 0, vfr = 0;

	// ==============================
	// =           One Sec          =
	// ==============================
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_CUT, &is_cut);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, &vfn);
	if (is_cut) {
		DBG_MSG("[%d][STRGBUF][CUT]vfn=%d\r\n", id, vfn);
	}
	if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
		if (((vfn + 1) % vfr) == 0) { //one sec
			DBG_MSG("[BSM%d] now_sec=%d\r\n", id, (vfn + 1) / vfr);
			if (is_cut) {
				vid_ok = 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_VID, vid_ok);
				DBG_DUMP("[%d][STRGBUF] CUT(%d) VID(%d)\r\n", id, is_cut, vid_ok);
			}
		}
	}

	// ==============================
	// =      Save StrgBuf          =
	// ==============================
	bsmux_ctrl_bs_copy2strgbuf(id, p_bsq);

	// ==============================
	// =      Save Entry Info       =
	// ==============================
	bsmux_util_strgbuf_handle_entry(id, p_bsq);

	// ==============================
	// =  If Change to normal mode  =
	// ==============================
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID, &vid_ok);
	if (vid_ok) {
		// update fileinfo
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, &vfn);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALVF, vfn);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_AUD_NUM, &afn);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALAF, afn);
		bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_VFN, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_AFN, 0);
		DBG_DUMP("[STRG][%d][vid_ok] vfn=%d afn=%d\r\n", id, vfn, afn);
		// update strgbuf status
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_ACT, 0);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_VID, 0);
	}

	return E_OK;
}

ER bsmux_util_strgbuf_handle_space(UINT32 id, void *p_buf)
{//handle card space: total_size / put_pos / get_pos
	BSMUXER_FILE_BUF *p_add_info = (BSMUXER_FILE_BUF *)p_buf;
	UINT32 fileop = p_add_info->fileop;
	UINT32 size = (UINT32)p_add_info->size;
	UINT32 alloc_size = 0, total_size = 0, put_pos = 0, get_pos = 0;

	DBG_MSG("[BSM%d] fileop %d addr %d size %d pos %lld\r\n",
		id, fileop, p_add_info->addr, size, p_add_info->pos);

	//p_add_info->pathID = id;
	//p_add_info->event = HD_BSMUX_FEVENT_BSINCARD;

	if (fileop & BSMUXER_FOP_CONT_WRITE) {
		bsmux_util_prepare_buf(id, BSMUX_BS2STRG_WRITE, p_add_info->addr, p_add_info->size, p_add_info->pos);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_ALLOC_SIZE, &alloc_size);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, &total_size);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_PUT_POS, &put_pos);
		put_pos += size; //update put_pos with size(align)
		total_size += size; //update total_size
		if (put_pos == alloc_size) {
			put_pos = 0;
			p_add_info->fileop = BSMUXER_FOP_SEEK_WRITE;
			p_add_info->pos = 0;
			bsmux_util_prepare_buf(id, BSMUX_BS2STRG_SYNC, p_add_info->addr, p_add_info->size, p_add_info->pos);
		}
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, total_size);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_PUT_POS, put_pos);
	}
	if (fileop & BSMUXER_FOP_FLUSH) {
		bsmux_util_prepare_buf(id, BSMUX_BS2STRG_FLUSH, p_add_info->addr, p_add_info->size, p_add_info->pos);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_ALLOC_SIZE, &alloc_size);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, &total_size);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_GET_POS, &get_pos);
		if ((get_pos + size) > alloc_size) {
			get_pos -= (alloc_size - size);
		} else {
			get_pos += size; //update get_pos
		}
		total_size -= size; //update total_size
		DBG_IND("[%d] SHIFT size %d\r\n", id, size);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, total_size);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_GET_POS, get_pos);
	}

	return E_OK;
}

ER bsmux_util_strgbuf_handle_header(UINT32 id)
{//make header
	UINT32 put_pos = 0, get_pos = 0, total_size = 0;
	UINT32 vfn = 0, afn = 0, cur_pos = 0;
	UINT32 bs_to_write = 0;

	// ==============================
	// =        Get  Info           =
	// ==============================
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_PUT_POS, &put_pos);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_GET_POS, &get_pos);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, &total_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID_NUM, &vfn);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_AUD_NUM, &afn);
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_CUR_POS, &cur_pos);

	bs_to_write = total_size;
	if (total_size % BSMUX_BS2CARD_1M) {
		DBG_ERR("[%d] still need to align %d???\r\n", id, BSMUX_BS2CARD_1M);
		UINT32 mod = (total_size % BSMUX_BS2CARD_1M);
		bs_to_write += (BSMUX_BS2CARD_1M - mod);
	}
	DBG_DUMP("[%d] size %d %d\r\n", id, bs_to_write, total_size);
	DBG_DUMP("[%d] vid_entry_num %d aud_entry_num %d cur_entry_pos %lld\r\n",
		id, vfn, afn, (UINT64)cur_pos);
	DBG_DUMP("[%d] total_size %d put_pos %d get_pos %d\r\n",
		id, total_size, put_pos, get_pos);

	// ==============================
	// =        Makeheader          =
	// ==============================
	#if 1
	{
		#if 0  //20210827
		UINT32 addr = 0, size = 0, align = 0;
		UINT32 last2card = 0, nowaddr = 0, blksize = 0;
		UINT32 bufaddr = 0, bufend = 0;
		#else
		UINT32 tempfront = 0;
		#endif //20210827
		UINT32 frontPut = 0;

		DBG_MSG("[%d][STRGBUF] MAKE header\r\n", id);
		bsmux_util_make_header(id);
		#if 0  //20210827
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEBLOCK, &blksize);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &bufaddr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &bufend);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &nowaddr);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			DBG_MSG("[%d[STRGBUF] frontPut\r\n", id);
			addr = last2card;
			size = (nowaddr - last2card);
			#if 1
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_FRONT, (UINT32)addr, (UINT64)size, 0);
			#else
			align = bsmux_util_calc_align(size, blksize, NMEDIAREC_ALIGN_CEIL);
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_FRONT, (UINT32)addr, (UINT64)align, 0);
			#endif
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, TRUE);
		} else {
			DBG_ERR("[%d[STRGBUF] already frontPut\r\n", id);
		}
		#else
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			DBG_MSG("[%d[STRGBUF] frontPut\r\n", id);
			bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, &tempfront);
			if (tempfront) {
				bsmux_util_prepare_buf(id, BSMUX_BS2CARD_FRONT, tempfront, bsmux_mdat_calc_header_size(id), 0);
			} else {
				DBG_ERR("[%d] tempfrontnull\r\n", id);
			}
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, TRUE);
		} else {
			DBG_ERR("[%d[STRGBUF] already frontPut\r\n", id);
		}
		#endif //20210827
		//blkPut
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, TRUE);
		#if 0  //20210827
		align = bsmux_util_calc_align(size, blksize, NMEDIAREC_ALIGN_CEIL);
		if (last2card + align > bufend) {
			nowaddr = bufaddr + (align - (bufend - last2card));
		} else {
			nowaddr = (last2card + align);
		}

		last2card = nowaddr;

		if (nowaddr == bufend) {
			DBG_DUMP("BSM:[%d] now(0x%X) achieve max(0x%X)\r\n", id, nowaddr, bufend);
			nowaddr = bufaddr;
		}
		if (last2card == bufend) {
			DBG_DUMP("BSM:[%d] last(0x%X) achieve max(0x%X)\r\n", id, last2card, bufend);
			last2card = bufaddr;
		}

		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_LAST2CARD, last2card);
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_NOWADDR, nowaddr);
		#endif //20210827
	}
	#endif

	// ==============================
	// =        Rollback BS         =
	// ==============================
	bsmux_util_prepare_buf(id, BSMUX_BS2STRG_READ, get_pos, (UINT64)bs_to_write, (UINT64)put_pos);

	// ==============================
	// =        Update              =
	// ==============================
	// update membuf info
	{
		UINT32 offset;
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, &total_size);
		offset = bsmux_mdat_calc_header_size(id) + total_size;
		bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, offset);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_COPYPOS, offset);
		DBG_DUMP("[STRG][%d][HDR] total2card=0x%x\r\n", id, offset);
	}

	bsmux_util_strgbuf_init(id);

	return E_OK;
}

VOID bsmux_util_put_version(UINT32 date, UINT32 ver1, UINT32 ver2)
{
	char *ptr;
	UINT32 last, quo, i, a = 1, j;
	ptr = &g_bsmux_ver_string[0];
	last = date;
	for (i = 0; i < 6; i++) {
		a = 1;
		for (j = 0; j < (5 - i); j++) {
			a *= 10;
		}
		quo = last / (a);
		*ptr++ = (char)('0' + quo);
		last -= quo * a;
	}
	last = ver2;
	for (i = 0; i < 2; i++) {
		a = 1;
		for (j = 0; j < (1 - i); j++) {
			a *= 10;
		}
		quo = last / (a);
		*ptr++ = (char)('0' + quo);
		last -= quo * a;
	}
	DBG_IND("ver.string = %s\r\n", g_bsmux_ver_string);
}

VOID bsmux_util_get_version(UINT32 *p_version)
{
	UINT32 version_addr = (UINT32)g_bsmux_ver_string;
	*p_version = version_addr;
}

VOID bsmux_util_show_setting(UINT32 id)
{
	BSMUX_REC_INFO *pInfo = NULL;

	pInfo = bsmux_get_rec_info(id);
	if (bsmux_util_is_null_obj((void *)pInfo)) {
		DBG_ERR("get prj setting null\r\n");
		return;
	}

	DBG_DUMP("BSM:[%d] vid w=%d, h=%d, vfr=%d, gop=%d, var_vfr=%d\r\n", id,
		pInfo->file.vid.width, pInfo->file.vid.height, pInfo->file.vid.vfr, pInfo->file.vid.gop, pInfo->file.vid.var_vfr);
	DBG_DUMP("BSM:[%d] sub vid w=%d, h=%d, vfr=%d, gop=%d\r\n", id,
		pInfo->file.sub_vid.width, pInfo->file.sub_vid.height, pInfo->file.sub_vid.vfr, pInfo->file.sub_vid.gop);
	DBG_DUMP("BSM:[%d] aud en=%d, sr=%d, tbr=%d KB\r\n", id,
		pInfo->file.aud.aud_en, pInfo->file.aud.asr, pInfo->file.vid.tbr / 1024);
	DBG_DUMP("BSM:[%d] vcodec=%d subvcodec=%d acodec=%d emr=(%d/%d) fmoov=(%d/%d/%d) fput=(%d)\r\n", id,
		pInfo->file.vid.vidcodec, pInfo->file.sub_vid.vidcodec, pInfo->file.aud.codectype, pInfo->file.emron, pInfo->file.emrloop,
		pInfo->moov.fmoov_on, pInfo->moov.moov_freq, pInfo->moov.moov_tune, pInfo->fastput_en);
	DBG_DUMP("BSM:[%d] filetype=%d recformat=%d endtype=%d durlinit=%d\r\n", id,
		pInfo->file.filetype, pInfo->file.recformat, pInfo->file.endtype, pInfo->dur_us_max);
	DBG_DUMP("BSM:[%d] seamless=%d rollback=%d keep=%d buffer=(%d/%d) muxalign=%d method=%d\r\n", id,
		pInfo->file.seamlessSec, pInfo->file.rollbacksec, pInfo->file.keepsec, pInfo->file.revsec, pInfo->wrinfo.wrblk_locksec,pInfo->mux_align,pInfo->mux_method);
	DBG_DUMP("BSM:[%d] adts=%d gpson=%d/%d thumb=%d nidx=%d rpts=%d\r\n", id,
		pInfo->file.aud.adts_bytes, pInfo->gps.gpson, pInfo->gps.gps_rate, pInfo->thumb.thumbon, pInfo->nidxinfo.nidx_en, pInfo->pts_reset);
	DBG_DUMP("BSM:[%d] flush=%d wrblk=%dK\r\n", id,
		pInfo->wrinfo.flush_freq, pInfo->wrinfo.wrblk_size/1024);
	DBG_DUMP("BSM:ver=%d, %d\r\n", 240115, 155);
	bsmux_util_put_version(240115, 0, 155);
}

VOID bsmux_util_set_result(UINT32 result)
{
	g_bsmux_module_status = result;
	switch (g_bsmux_module_status) {
	case BSMUXER_RESULT_SLOWMEDIA:
		DBG_DUMP("BSM: slow media\r\n");
		break;
	case BSMUXER_RESULT_LOOPREC_FULL:
		DBG_DUMP("BSM: loop rec full\r\n");
		break;
	case BSMUXER_RESULT_OVERTIME:
		DBG_DUMP("BSM: overtime\r\n");
		break;
	case BSMUXER_RESULT_MAXSIZE:
		DBG_DUMP("BSM: exceed max size\r\n");
		break;
	default:
		break;
	}
	return;
}

VOID bsmux_util_chk_frm_sync(UINT32 id, void *p_data)
{
	BSMUXER_DATA *pData = (BSMUXER_DATA *)p_data;
	UINT32 emron = 0, seamsec = 0, rollsec = 0, keepsec = 0, recformat = 0;
	UINT32 vfr = 0, asr = 0, cutsec = 0, stopVF = 0;
	UINT32 seamsec_ms = 0, rollsec_ms = 0, keepsec_ms = 0, cutsec_ms = 0;
	UINT64 dur_us = 0, cal_us = 0, ratio = 0; /* timestamp sync */
	UINT32 caln = 0, diff = 0; /* v/a entry sync */
	UINT32 vfn = 0, afn = 0; /* v/a entry sync */
	static UINT32 onesec = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &asr);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_KEEPSEC, &keepsec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, &seamsec_ms);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC_MS, &rollsec_ms);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_KEEPSEC_MS, &keepsec_ms);
	#endif
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_RECFORMAT, &recformat);
	if ((recformat == MEDIAREC_GOLFSHOT) ||
		(recformat == MEDIAREC_TIMELAPSE)) {
		return;
	}
	vfn = g_bsmux_vbs[id].FrmNum;
	afn = g_bsmux_abs[id].FrmNum;

	if (emron == 0) {
		cutsec = seamsec;
		if (seamsec_ms) {
			cutsec_ms = seamsec_ms;
		}
	} else {
		cutsec = rollsec + keepsec;
		if (rollsec_ms && keepsec_ms) {
			cutsec_ms = rollsec_ms + keepsec_ms;
		}
	}
	stopVF = vfr * cutsec;
	if (cutsec_ms) {
		stopVF = ((vfr * cutsec_ms) / 1000);
	}

	if (pData->type == BSMUX_TYPE_VIDEO) {
		if (vfn == stopVF) {
			g_bsmux_vbs[id].FrmNum = vfn = 0;
			g_bsmux_abs[id].FrmNum = 0;
			g_bsmux_vbs[id].BsDuration = 0;
			g_bsmux_abs[id].BsDuration = 0;
		}
		if (vfn) {
			cal_us = (UINT64)((1 * 1000 * 1000) / vfr);
			dur_us = hwclock_diff_longcounter(g_bsmux_vbs[id].BsTimeStamp, pData->bSTimeStamp);
			if (dur_us > cal_us * 10) {
				if (g_bsmux_vbs[id].Tolerance < 1) {
					dur_us = cal_us;
					g_bsmux_vbs[id].Tolerance++;
				}
			} else {
				if (g_bsmux_vbs[id].Tolerance >= 1) {
					g_bsmux_vbs[id].Tolerance = 0;
				}
			}
			g_bsmux_vbs[id].BsDuration += dur_us;
			UTIL_MEM_FUNC("V[%d] dur_us=%llu\r\n", vfn, dur_us);

			if ((vfn % vfr) == 0) { // one sec
				onesec = 1;
				/* timestamp sync */
				cal_us = (UINT64)(((1 * 1000 * 1000) / vfr) * vfn);
				dur_us = g_bsmux_vbs[id].BsDuration;
				ratio = (dur_us * 100) / cal_us;
				if ((ratio >= 103) || (ratio <= 97)) { //3%
					if (g_bsmux_vbs[id].TimeSyncAlm % 5 == 0) {
						DBG_DUMP("V[%d] cal_us=%lld dur_us=%lld ratio=%lld%%\r\n", vfn, cal_us, dur_us, ratio);
						g_bsmux_vbs[id].TimeSyncAlm = 0;
					}
					g_bsmux_vbs[id].TimeSyncAlm++;
				}
			}
		}
		g_bsmux_vbs[id].BsTimeStamp = pData->bSTimeStamp;
		g_bsmux_vbs[id].FrmNum++;
	}
	else if (pData->type == BSMUX_TYPE_AUDIO) {
		if (afn) {
			cal_us = ((UINT64)((1 * 1000 * 1000) * 1024) / asr);
			dur_us = hwclock_diff_longcounter(g_bsmux_abs[id].BsTimeStamp, pData->bSTimeStamp);
			if (dur_us > cal_us * 10) {
				if (g_bsmux_abs[id].Tolerance < 1) {
					dur_us = cal_us;
					g_bsmux_abs[id].Tolerance++;
				}
			} else {
				if (g_bsmux_abs[id].Tolerance >= 1) {
					g_bsmux_abs[id].Tolerance = 0;
				}
			}
			g_bsmux_abs[id].BsDuration += dur_us;
			UTIL_MEM_FUNC("A[%d] dur_us=%llu\r\n", afn, dur_us);

			if (onesec) { // one sec
				/* timestamp sync */
				cal_us = ((UINT64)(((1 * 1000 * 1000) * 1024) / asr) * (UINT64)afn);
				dur_us = g_bsmux_abs[id].BsDuration;
				ratio = (dur_us * 100) / cal_us;
				if ((ratio >= 103) || (ratio <= 97)) { //3%
					if (g_bsmux_abs[id].TimeSyncAlm % 5 == 0) {
						DBG_DUMP("A[%d] cal_us=%lld dur_us=%lld ratio=%lld%%\r\n", afn, cal_us, dur_us, ratio);
						g_bsmux_abs[id].TimeSyncAlm = 0;
					}
					g_bsmux_abs[id].TimeSyncAlm++;
				}
				/* v/a entry sync */
				caln = (((vfn / vfr) * asr) / 1024);
				diff = (afn >= caln) ? (afn - caln) : (caln - afn);
				if ((diff > (asr / 1024)) && (diff > vfr)) {
					if (g_bsmux_abs[id].FrmSyncAlm % 5 == 0) {
						DBG_DUMP("BSM[%d] V[%d] should match A[%d] but A[%d], diff=%d\r\n", id, vfn, caln, afn, diff);
						bsmux_cb_result(BSMUXER_RESULT_VANOTSYNC, id);
						g_bsmux_abs[id].FrmSyncAlm = 0;
					}
					g_bsmux_abs[id].FrmSyncAlm++;
				}
				onesec = 0;
			}
		}
		g_bsmux_abs[id].BsTimeStamp = pData->bSTimeStamp;
		g_bsmux_abs[id].FrmNum++;
	}
	return;
}

VOID bsmux_util_dump_info(UINT32 id)
{
	UINT32 vfn = 0, afn = 0, sub_vfn = 0, drop_vfn = 0, drop_afn = 0, drop_sub_vfn = 0;
	UINT32 buf_addr = 0, buf_end = 0, now_addr = 0, last2card = 0, real2card = 0, buf_offset = 0;
	UINT32 bsqnum = bsmux_ctrl_bs_getnum(id, 0);

	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_ADDR, &buf_addr);
	if (!buf_addr) {
		return;
	}

	// bsq
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &afn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF_DROP, &drop_afn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &drop_sub_vfn);
	DBG_DUMP("[%d] main(%d/%d) sub(%d/%d) aud(%d/%d) remain(%d)\r\n", id,
		drop_vfn, vfn, drop_sub_vfn, sub_vfn, drop_afn, afn, bsqnum);

	// mem
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &buf_addr);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &buf_end);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &now_addr);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, &buf_offset);
	DBG_DUMP("[%d] buf(0x%x - 0x%x) now(0x%x) last(0x%x) real(0x%x) offset(0x%x)\r\n", id,
		buf_addr, buf_end, now_addr, last2card, real2card, buf_offset);

	if (g_bsmux_debug_msg)
		bsmux_util_show_setting(id);
}

VOID bsmux_util_dump_buffer(char *pBuf, UINT32 length)
{
	UINT32 i, j;
	char  *pC = pBuf;
	for (i = 0; i < length; i += 0x10) {
		DBG_DUMP("ad = 0x%03x : ", i);
		for (j = 0; j < 0x10; j++) {
			DBG_DUMP("%02x ", *pC);
			pC++;
		}
		DBG_DUMP("\r\n");
	}
}

UINT32 bsmux_util_is_invalid_id(UINT32 id)
{
	if (id >= BSMUX_MAX_CTRL_NUM) {
		return 1;
	} else {
		return 0;
	}
}

UINT32 bsmux_util_is_null_obj(void *p_obj)
{
	if (NULL == p_obj) {
		return 1;
	} else {
		return 0;
	}
}

UINT32 bsmux_util_is_not_normal(void)
{
	if (g_bsmux_module_status == BSMUXER_RESULT_NORMAL) {
		return 0;
	} else if ((g_bsmux_module_status == BSMUXER_RESULT_VANOTSYNC) ||
				(g_bsmux_module_status == BSMUXER_RESULT_GOPMISMATCH)) {
		return 0;
	} else if ((g_bsmux_module_status == BSMUXER_RESULT_PROCDATAFAIL)) {
		return 0;
	} else {
		return g_bsmux_module_status;
	}
}

UINT32 bsmux_util_set_writeblock_size(UINT32 id)
{
	UINT32 tbr = 0, resvsec = 0, blksize = 0;
	UINT32 value;
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_TBR, &tbr);
	bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_WRBLK_SIZE, &blksize);
	if (tbr > 0x200000) {
		value = BSMUX_BS2CARD_2M;
	} else if (tbr > 0x80000) { //900K
		value = BSMUX_BS2CARD_1M;
	} else {
		value = BSMUX_BS2CARD_500K;
	}
	if (blksize) {
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_BUFRESSEC, &resvsec);
		if (blksize + bsmux_util_calc_reserved_size(id) < (tbr * resvsec)) {
			value = blksize;
		} else {
			DBG_WRN("BSM: [%d] tbr %d not match. set wrblk %d\r\n", id, tbr, value);
		}
	}
	DBG_IND("wrblk size: 0x%X\r\n", value);
	return value;
}

UINT32 bsmux_util_get_buf_pa(UINT32 id, UINT32 va)
{
	UINT32 videnc_pa, videnc_va, videnc_size;
	UINT32 audenc_pa, audenc_va, audenc_size;
	UINT32 sub_videnc_pa, sub_videnc_va, sub_videnc_size;
	UINT32 mem_pa, mem_va, mem_size;
	UINT32 pa;

	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_ADDR, &videnc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_VIRT, &videnc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_SIZE, &videnc_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_ADDR, &audenc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_VIRT, &audenc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_SIZE, &audenc_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR, &sub_videnc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE, &sub_videnc_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT, &sub_videnc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_ADDR, &mem_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_VIRT, &mem_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_SIZE, &mem_size);

	if ((va >= videnc_va) && (va < (videnc_va + videnc_size))) {
		pa = (videnc_pa + (va - videnc_va));
	} else if ((va >= audenc_va) && (va < (audenc_va + audenc_size))) {
		pa = (audenc_pa + (va - audenc_va));
	} else if ((va >= sub_videnc_va) && (va < (sub_videnc_va + sub_videnc_size))) {
		pa = (sub_videnc_pa + (va - sub_videnc_va));
	} else if ((va >= mem_va) && (va < (mem_va + mem_size))) {
		pa = (mem_pa + (va - mem_va));
	} else {
		DBG_ERR("[BSM%d] invalid va 0x%X\r\n", id, va);
		pa = 0;
	}

	DBG_IND("convert va 0x%X to pa 0x%X\r\n", va, pa);

	return pa;
}

UINT32 bsmux_util_get_buf_va(UINT32 id, UINT32 pa)
{
	UINT32 videnc_pa, videnc_va, videnc_size;
	UINT32 audenc_pa, audenc_va, audenc_size;
	UINT32 sub_videnc_pa, sub_videnc_va, sub_videnc_size;
	UINT32 mem_pa, mem_va, mem_size;
	UINT32 va;

	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_ADDR, &videnc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_SIZE, &videnc_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_VIRT, &videnc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_ADDR, &audenc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_SIZE, &audenc_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_VIRT, &audenc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR, &sub_videnc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE, &sub_videnc_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT, &sub_videnc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_ADDR, &mem_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_SIZE, &mem_size);
	bsmux_util_get_param(id, BSMUXER_PARAM_MEM_VIRT, &mem_va);

	if ((pa >= videnc_pa) && (pa < (videnc_pa + videnc_size))) {
		va = (videnc_va + (pa - videnc_pa));
	} else if ((pa >= audenc_pa) && (pa < (audenc_pa + audenc_size))) {
		va = (audenc_va + (pa - audenc_pa));
	} else if ((pa >= sub_videnc_pa) && (pa < (sub_videnc_pa + sub_videnc_size))) {
		va = (sub_videnc_va + (pa - sub_videnc_pa));
	} else if ((pa >= mem_pa) && (pa < (mem_pa + mem_size))) {
		va = (mem_va + (pa - mem_pa));
	} else {
		DBG_WRN("[BSM%d] pa 0x%X not map\r\n", id, pa);
		va = pa;
	}

	DBG_IND("convert pa 0x%X to va 0x%X\r\n", pa, va);

	return va;
}

UINT32 bsmux_util_calc_align(UINT32 input, UINT32 alignsize, UINT32 type)
{
	UINT32 out, mod;

	mod = input % alignsize;
	if (type == NMEDIAREC_ALIGN_ROUND) { //round off
		if (mod) {
			out = input - mod;
		} else {
			out = input;
		}
	} else { //NMEDIAREC_ALIGN_CEIL
		if (mod) {
			out = input - mod + alignsize;
		} else {
			out = input;
		}
	}
	if (out > 0xF0000000) {
		DBG_DUMP("BSM:in=0x%lx, align=0x%lx, out=0x%lx!\r\n", input, alignsize, out);
	}
	return out;
}

UINT32 bsmux_util_calc_sec(BSMUX_CALC_SEC *p_setting)
{
	BSMUX_CALC_SEC *pSetting = (BSMUX_CALC_SEC *)p_setting;
	UINT32 bytesPerSec[BSMUX_MAX_CTRL_NUM] = {0};
	UINT32 sec, i, total = 0;

	for (i = 0; i < BSMUX_MAX_CTRL_NUM; i++) {
		bytesPerSec[i] = bsmux_check_sec(&(pSetting->info[i]));
		total += bytesPerSec[i];
	}

	// givenSpace: use user config
	if (pSetting->givenSpace == BSMUXER_CALCSEC_UNKNOWN_FSSIZE) {
		DBG_ERR("not assign givenSpace\r\n");
		return 0;
	}
	else if (pSetting->givenSpace <= total) {
		return 0;
	}
	if (total == 0) {
		DBG_DUMP("^R bsmux_util_calc_sec but TBR is zero!\r\n");
		return 2000; //temp value for zero bytesPerSec
	}

	sec = (UINT64)pSetting->givenSpace / (UINT64)total;
	if (1) {
		DBG_DUMP("free=0x%llx, tbr=0x%lx, sec=%d!\r\n",  pSetting->givenSpace, total, sec);
	}
	return sec;
}

UINT32 bsmux_util_calc_entry_per_sec(UINT32 id)
{
	UINT32 vfr = 0, asr = 0, aud_on = 0;
	UINT32 sub_vfr = 0;
	UINT32 gps_on = 0, gps_rate = 0, gps_queue = 0;
	UINT32 meta_on = 0, meta_num = 0, meta_rate = 0, meta_queue = 0;
	UINT32 secentry;

	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_ON, &aud_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SR, &asr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_ON, &gps_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_RATE, &gps_rate);
	bsmux_util_get_param(id, BSMUXER_PARAM_GPS_QUEUE, &gps_queue);
	bsmux_util_get_param(id, BSMUXER_PARAM_META_ON, &meta_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_META_NUM, &meta_num);
	meta_rate = vfr; //temp as vfr
	meta_queue = 1; //temp as 1

	if (vfr) {
		#if 1
			secentry = vfr + ((asr / MP_AUDENC_AAC_RAW_BLOCK) + 1);
		#else
		if (aud_on) {
			secentry = vfr + ((asr / MP_AUDENC_AAC_RAW_BLOCK) + 1);
		} else {
			secentry = vfr;
		}
		#endif
		if (sub_vfr) {
			secentry += sub_vfr;
		}
		if (gps_on && gps_rate && gps_queue) {
			secentry += gps_rate;
		}
		while (meta_num > 0) {
			if (meta_on && meta_rate && meta_queue) {
				secentry += meta_rate;
			}
			meta_num--;
		}
	} else {
		DBG_WRN("[%d] vfr zero\r\n", id);
		secentry = BSMUXER_BSQ_MAX_SECENTRY;
	}
	return secentry;
}

UINT32 bsmux_util_calc_frm_num(UINT32 id, UINT32 type)
{
	UINT32 vfr = 0, emr_on = 0, emrloop = 0, strgbuf = 0;
	UINT32 seamlessSec = 0, rollsec = 0, keepsec = 0, extSec = 0;
	UINT32 seamlessSec_ms = 0, rollsec_ms = 0, keepsec_ms = 0;
	UINT32 real_sec_ms = 0;
	UINT32 sec1_ms = 0, sec2_ms = 0;
	UINT32 stopVF;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_ROLLSEC, &rollsec);
	if (rollsec == 0) {
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollsec);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_KEEPSEC, &keepsec);
	#if BSMUX_TEST_SET_MS
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, &seamlessSec_ms);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_ROLLSEC, &rollsec);
	if (rollsec == 0) {
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC_MS, &rollsec_ms);
	}
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_KEEPSEC_MS, &keepsec_ms);
	#endif
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_EXTSEC, &extSec);

	seamlessSec_ms = ((seamlessSec_ms) ? seamlessSec_ms : seamlessSec * 1000);
	rollsec_ms = ((rollsec_ms) ? rollsec_ms : rollsec * 1000);
	keepsec_ms = ((keepsec_ms) ? keepsec_ms : keepsec * 1000);

	if (extSec) {
		sec2_ms = extSec * 1000;
	} else {
		if (emr_on) {
			sec1_ms = rollsec_ms;
			sec2_ms = keepsec_ms;
			if (emrloop) {
				if (emrloop == BSMUX_STEP_EMRLOOP) {
					sec1_ms = 0;
				}
				if (strgbuf) {
					sec1_ms = 0;
				}
				sec2_ms = seamlessSec_ms;
			}
		} else {
			sec2_ms = seamlessSec_ms;
		}
	}
	real_sec_ms = sec1_ms + sec2_ms;

	if (type == BSMUX_TYPE_VIDEO) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	} else {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &vfr);
	}

	if (real_sec_ms) {
		stopVF = ((vfr * real_sec_ms) / 1000);
	} else {
		stopVF = 0;
		DBG_ERR("[%d]stopVF zero\r\n", id);
	}

	return stopVF;
}

UINT32 bsmux_util_pset_padding_size(UINT32 id, UINT32 size)
{
	bsmux_get_rec_info(id)->padding_size = size;
	BSMUX_MSG("[%d] set padding_size=0x%lx\r\n", id, (unsigned long)size);
	return 0;
}

UINT32 bsmux_util_calc_padding_size(UINT32 id)
{
	UINT32 padding_size = 0;
	UINT32 filetype = 0;
	UINT32 front_moov = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
	bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);

	if ((filetype == MEDIA_FILEFORMAT_MOV) || (filetype == MEDIA_FILEFORMAT_MP4)) {
		if (front_moov) {
			/* FIXME: if padding_size large than wrblksize */
			padding_size = bsmux_get_rec_info(id)->padding_size;
			padding_size += MP_AUDENC_AAC_RAW_BLOCK * 3;
		} else {
			padding_size += MP_AUDENC_AAC_RAW_BLOCK * 3;
		}
	}
	return padding_size;
}

UINT32 bsmux_util_calc_reserved_size(UINT32 id)
{
	UINT32 reserved_size = 0;
	UINT32 filetype = 0;
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
	if ((filetype == MEDIA_FILEFORMAT_MOV) || (filetype == MEDIA_FILEFORMAT_MP4)) {
		reserved_size = BSMUX_RESERVED_FILESIZE;
	} else if (filetype == MEDIA_FILEFORMAT_TS) {
		reserved_size = BSMUX_RESERVED_FILESIZE;
	}
	return reserved_size;
}

UINT32 bsmux_util_memcpy(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method)
{
	BSMUX_MUX_INFO *p_dst = (BSMUX_MUX_INFO *)uiDst;
	BSMUX_MUX_INFO *p_src = (BSMUX_MUX_INFO *)uiSrc;
	if (method == 1)
		memcpy((void *)p_dst->vrt_addr, (const void *)p_src->vrt_addr, uiSize);
	else
		hd_gfx_memcpy(p_dst->phy_addr, p_src->phy_addr, (size_t)uiSize);
	return uiSize;
}

UINT32 bsmux_util_muxalign(UINT32 id, UINT32 src)
{
	UINT32 mux_align = 0;
	bsmux_util_get_param(id, BSMUXER_PARAM_MUXALIGN, &mux_align);
	if (mux_align) {
		return ALIGN_CEIL(src, mux_align);
	}
	return ALIGN_CEIL_4(src);
}

BOOL bsmux_util_check_tag(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 codectype;
	UINT32 va;
	UINT8 *p2nd, nal_type;
	BOOL result = TRUE;
	UINT32 drop_en;

	if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &codectype);

		if (codectype == MEDIAVIDENC_H264) {
			#if 1
			va = pSrc->uiBSVirAddr;
			#else
			va = bsmux_util_get_buf_va(id, pSrc->uiBSMemAddr);
			#endif
			DBG_IND("[H264] pa 0x%X va 0x%X\r\n", pSrc->uiBSMemAddr, va);
			p2nd = (UINT8 *)(va);
			#if 1  // SEI support
			nal_type = *(p2nd + 4) & 0x1f;
			if ((nal_type != 0x01) && (nal_type != 0x05) && (nal_type != 0x06)) { // 0x01:P, 0x05:I, 0x06:SEI
				DBG_DUMP("H264 value = 0x%lx\r\n", *(p2nd + 5));
				DBG_DUMP("BSM:vbs broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#else
			p2nd+=5;
			if ((*p2nd != 0x88)&&(*p2nd != 0x9A)) {
				DBG_DUMP("H264 value = 0x%lx\r\n", *p2nd);
				DBG_DUMP("BSM:vbs broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#endif // SEI support
		}
		else if (codectype == MEDIAVIDENC_H265) {
			#if 1
			va = pSrc->uiBSVirAddr;
			#else
			va = bsmux_util_get_buf_va(id, pSrc->uiBSMemAddr);
			#endif
			DBG_IND("[H265] pa 0x%X va 0x%X\r\n", pSrc->uiBSMemAddr, va);
			p2nd = (UINT8 *)(va);
			#if 1  // SEI support
			nal_type = (*(p2nd + 4) >> 1) & 0x3f;
			if ((nal_type != 32) && (nal_type != 1) && (nal_type != 0) && (nal_type != 39)) { // I(19/VPS:32), P(1), SkipP(0), SEI(39)
				DBG_DUMP("H265 value = 0x%lx\r\n", *(p2nd + 4));
				DBG_DUMP("BSM:vbs2 broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#else
			p2nd+=4;
			if ((((*p2nd>>1)&0x3F)!=32)&&(((*p2nd>>1)&0x3F)!=1)&&(((*p2nd>>1)&0x3F)!=0)) { // I(19/VPS:32), P(1), SkipP(0)
				DBG_DUMP("H265 value = 0x%lx\r\n", *p2nd);
				DBG_DUMP("BSM:vbs2 broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#endif // SEI support
		}
	}
	else if (pSrc->uiType == BSMUX_TYPE_SUBVD) {
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &codectype);

		if (codectype == MEDIAVIDENC_H264) {
			#if 1
			va = pSrc->uiBSVirAddr;
			#else
			va = bsmux_util_get_buf_va(id, pSrc->uiBSMemAddr);
			#endif
			DBG_IND("[H264] sub pa 0x%X va 0x%X\r\n", pSrc->uiBSMemAddr, va);
			p2nd = (UINT8 *)(va);
			#if 1  // SEI support
			nal_type = *(p2nd + 4) & 0x1f;
			if ((nal_type != 0x01) && (nal_type != 0x05) && (nal_type != 0x06)) { // 0x01:P, 0x05:I, 0x06:SEI
				DBG_DUMP("H264 value = 0x%lx\r\n", *(p2nd + 5));
				DBG_DUMP("BSM:sub vbs broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#else
			p2nd+=5;
			if ((*p2nd != 0x88)&&(*p2nd != 0x9A)) {
				DBG_DUMP("H264 value = 0x%lx\r\n", *p2nd);
				DBG_DUMP("BSM:sub vbs broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#endif // SEI support
		}
		else if (codectype == MEDIAVIDENC_H265) {
			#if 1
			va = pSrc->uiBSVirAddr;
			#else
			va = bsmux_util_get_buf_va(id, pSrc->uiBSMemAddr);
			#endif
			DBG_IND("[H265] sub pa 0x%X va 0x%X\r\n", pSrc->uiBSMemAddr, va);
			p2nd = (UINT8 *)(va);
			#if 1  // SEI support
			nal_type = (*(p2nd + 4) >> 1) & 0x3f;
			if ((nal_type != 32) && (nal_type != 1) && (nal_type != 0) && (nal_type != 39)) { // I(19/VPS:32), P(1), SkipP(0), SEI(39)
				DBG_DUMP("H265 value = 0x%lx\r\n", *(p2nd + 4));
				DBG_DUMP("BSM:sub vbs2 broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#else
			p2nd+=4;
			if ((((*p2nd>>1)&0x3F)!=32)&&(((*p2nd>>1)&0x3F)!=1)&&(((*p2nd>>1)&0x3F)!=0)) { // I(19/VPS:32), P(1), SkipP(0)
				DBG_DUMP("H265 value = 0x%lx\r\n", *p2nd);
				DBG_DUMP("BSM:sub vbs2 broken[%d] --E\r\n", id);
				result = FALSE;
			}
			#endif // SEI support
		}
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_EN_DROP, &drop_en);

	if (drop_en) {
		UINT32 gop = 0, DropCount = 0;
		UINT32 DropSet = 0;

		if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_GOP, &gop);
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &DropCount);

			if (result == FALSE) { // CopyVWrong
				DropSet = 1;
			} else {
				DropSet = 0;
			}
			if (DropSet) { // handle drop
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, DropCount);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_VID_DROP, DropCount);
			}
		}
		else if (pSrc->uiType == BSMUX_TYPE_SUBVD) {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_GOP, &gop);
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &DropCount);

			if (result == FALSE) { // CopyVWrong
				DropSet = 1;
			} else {
				DropSet = 0;
			}
			if (DropSet) { // handle drop
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, DropCount);
				bsmux_util_get_param(id, BSMUXER_PARAM_SUB_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_SUB_DROP, DropCount);
			}
		}
	}

	return result;
}

UINT64 bsmux_util_calc_timestamp(UINT32 id, UINT64 time_start)
{
	UINT64 time_end = hwclock_get_longcounter();
	return time_end - time_start;
}

BOOL bsmux_util_check_firstI(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 vfn = 0;
	UINT32 sub_vfn = 0;
	UINT32 drop_vfn = 0;
	BOOL result = TRUE;
	#if 1	//fast put flow ver3
	BOOL first = FALSE;
	#endif	//fast put flow ver3
	if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
		if (drop_vfn) {
			vfn += drop_vfn;
		}
		if (vfn == 0) {
			if (!pSrc->uiIsKey) {
				DBG_DUMP("BSM:[%d] first vbs not I\r\n", id);
				result = FALSE;
			}
			#if 1	//fast put flow ver3
			first = TRUE;
			#endif	//fast put flow ver3
		}
	}
	else if (pSrc->uiType == BSMUX_TYPE_SUBVD) {
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &drop_vfn);
		if (drop_vfn) {
			sub_vfn += drop_vfn;
		}
		if (sub_vfn == 0) {
			if (!pSrc->uiIsKey) {
				DBG_DUMP("BSM:[%d] first sub vbs not I\r\n", id);
				result = FALSE;
			}
			#if 1	//fast put flow ver3
			first = TRUE;
			#endif	//fast put flow ver3
		}
	}
	else {
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
		if (drop_vfn) {
			vfn += drop_vfn;
		}
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &drop_vfn);
		if (drop_vfn) {
			sub_vfn += drop_vfn;
		}
		if (vfn == 0 && sub_vfn == 0) {
			result = FALSE;
		}
	}
	#if 1	//fast put flow ver3
	if ((result == TRUE) && (first == TRUE)) { //=> first key-frame
		UINT32 fast_put = 0;
		bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fast_put);
		if (fast_put) {
			bsmux_util_pset_padding_size(id, bsmux_util_muxalign(id, pSrc->uiBSSize));
		}
	}
	#endif	//fast put flow ver3
	return result;
}

BOOL bsmux_util_check_frmidx(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BOOL result = TRUE;
	if (pSrc->uiType == BSMUX_TYPE_VIDEO) {
		if (pSrc->uiCountSametype == BSMUX_INVALID_IDX) {
			DBG_IND("BSM:[%d] (main) frmidx(%d) already used\r\n", id, pSrc->uiCountSametype);
			result = FALSE;
		}
	}
	else if (pSrc->uiType == BSMUX_TYPE_SUBVD) {
		if (pSrc->uiCountSametype == BSMUX_INVALID_IDX) {
			DBG_IND("BSM:[%d] (sub) frmidx(%d) already used\r\n", id, pSrc->uiCountSametype);
			result = FALSE;
		}
	}
	return result;
}

BOOL bsmux_util_check_bufuse(UINT32 id, void * p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pSrc = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT32 buf_addr = 0, buf_end = 0, nowaddr = 0, last2card = 0, real2card = 0; // current buffer usage
	UINT32 uiBSSize = 0, uiType = 0, uiIsKey = 0, step1 = 0, step2 = 0; // current bs
	UINT32 drop_en = 0, gop = 0, DropCount = 0, filetype = 0;
	#if 1
	UINT32 DropSet = 0;
	UINT32 sub_DropSet = 0;
	UINT32 aud_DropSet = 0;
	UINT32 dat_DropSet = 0;
	UINT32 full_DropSet = 0;
	UINT32 lastWrite = 0;
	#else
	static UINT32 DropSet = 0;
	static UINT32 sub_DropSet = 0;
	static UINT32 aud_DropSet = 0;
	static UINT32 dat_DropSet = 0;
	static UINT32 full_DropSet = 0;
	static UINT32 lastWrite = 0;
	#endif
	#if 1  // check front header case
	UINT32 vfn = 0, afn = 0, sub_vfn = 0;
	UINT32 fastput_en = 0, blksize = 0, blkPut = 0, closeFlag = 0;
	#endif // check front header case
	BOOL result = TRUE;

	bsmux_util_get_param(id, BSMUXER_PARAM_EN_DROP, &drop_en);

	if (drop_en) {

		#if 1
		bsmux_util_get_param(id, BSMUXER_PARAM_FULL_SET, &full_DropSet);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SET, &DropSet);
		bsmux_util_get_param(id, BSMUXER_PARAM_AUD_SET, &aud_DropSet);
		bsmux_util_get_param(id, BSMUXER_PARAM_SUB_SET, &sub_DropSet);
		bsmux_util_get_param(id, BSMUXER_PARAM_DAT_SET, &dat_DropSet);
		#endif

		// current buffer usage
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &buf_addr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &buf_end);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &nowaddr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, &lastWrite);
		if ((real2card >= buf_addr) && (real2card <= buf_end)) {
			// mdat buffer area
			if (lastWrite != real2card) {
				DBG_DUMP("[%d] real2card(0x%x) not match last keep(0x%x)\r\n", id, real2card, lastWrite);
			}
			lastWrite = real2card;
		} else {
			// not mdat buffer,  use last keep
			DBG_IND("[%d] real2card not mdat buffer, use last keep(0x%X)\r\n", id, lastWrite);
		}

		// current bs (consider make front header case and nidx data)
		uiIsKey = pSrc->uiIsKey;
		uiType = pSrc->uiType;
		uiBSSize = bsmux_util_muxalign(id, pSrc->uiBSSize);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &afn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		if ((vfn == 0) && (afn == 0) && (sub_vfn == 0)) {
			bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fastput_en);
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEBLOCK, &blksize);
			if (fastput_en) { // if need fast put first ops buf to open file
				#if 1	//fast put flow ver3
				uiBSSize += blksize - bsmux_util_calc_padding_size(id);
				#else
				uiBSSize += blksize - MP_AUDENC_AAC_RAW_BLOCK * 3;
				#endif	//fast put flow ver3
			} else {
				uiBSSize += bsmux_mdat_calc_header_size(id);
			}
		}
		#if 1
		if (uiType != BSMUX_TYPE_NIDXT) {
			uiBSSize += BSMUX_MAX_NIDX_BLK;
		}
		#endif

		// calculate
		if ((nowaddr + uiBSSize) > buf_end) {
			step1 = buf_end - nowaddr;
			step2 = uiBSSize - step1;
		} else {
			step1 = uiBSSize;
			step2 = 0;
		}

		#if 0
		if (step2) {
			DBG_DUMP("[%d] check-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id,
				buf_addr, nowaddr, last2card, lastWrite, buf_end);
			DBG_DUMP("[%d] check-(uiType=%d uiBSSize=%d)\r\n", id, uiType, uiBSSize);
		}
		#endif
		#if 0
		if (nowaddr == lastWrite) {
			UINT32 blkPut = 0;
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_BLKPUT, &blkPut);
			DBG_DUMP("[%d] sync(%d)-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id, blkPut,
				buf_addr, nowaddr, last2card, lastWrite, buf_end);
			DBG_DUMP("[%d] sync(%d)-(uiType=%d uiBSSize=%d)\r\n", id, blkPut, uiType, uiBSSize);
			if ((vfn == 0) && (afn == 0) && (sub_vfn == 0)) {
				DBG_DUMP("[%d] sync(%d)-makeheader\r\n", id, blkPut);
			}
		}
		#endif

		// check drop or not and handle drop
		bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_CLOSE_FLAG, &closeFlag);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_BLKPUT, &blkPut);
		if ((closeFlag) && (nowaddr == lastWrite)) {
			// mdat buffer full
			if (!full_DropSet) {
				DBG_DUMP("[%d]full-BEGIN(MDAT)(%d)(%d)\r\n", id, closeFlag, blkPut);
			}
			full_DropSet = 1;
			DBG_IND("[%d] full(%d)-(uiType=%d uiBSSize=%d)\r\n", id, closeFlag, uiType, uiBSSize);
		} else {
			// consider make front header case
			if ((vfn == 0) && (afn == 0) && (sub_vfn == 0)) {
				bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
				if ((filetype == MEDIA_FILEFORMAT_MOV) || (filetype == MEDIA_FILEFORMAT_MP4)) {
					UINT32 tempfront;
					bsmux_util_get_param(id, BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR, &tempfront);
					if (tempfront) {
						DBG_IND("check_bufuse: (%d) no need to chk temp\r\n", id);
						full_DropSet = 0;
					} else if (bsmux_mdat_chk_hdr_mem(BSMUX_HDRMEM_TEMP, id) == FALSE) {
						DBG_IND("[%d] hdrfull(%d)-(uiType=%d)\r\n", id, closeFlag, uiType);
						if (!full_DropSet) {
							DBG_DUMP("[%d]full-BEGIN(TEMP)\r\n", id);
						}
						full_DropSet = 1;
					} else {
						if (full_DropSet) {
							DBG_IND("[%d]full-END(TEMP)\r\n", id);
						}
						full_DropSet = 0;
					}
				} else {
					full_DropSet = 0;
				}
			} else {
				if (full_DropSet) {
					DBG_DUMP("[%d]full-END(HDR)\r\n", id);
				}
				full_DropSet = 0;
			}
		}
		if (uiType == BSMUX_TYPE_VIDEO) { //video main-stream
			if ((nowaddr < lastWrite) && (nowaddr + step1 > lastWrite)) {
				if (!DropSet) {
					DBG_IND("[%d] DropSet Begin\r\n", id);
					DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, nowaddr, lastWrite, nowaddr + step1);
				}
				DropSet = 1;
			} else if ((step2) && (buf_addr + step2 > lastWrite)) {
				if (!DropSet) {
					DBG_IND("[%d] DropSet Begin(padding)\r\n", id);
					DBG_IND("[%d] buf_addr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, buf_addr, lastWrite, buf_addr + step2);
				}
				DropSet = 1;
			} else {
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_GOP, &gop);
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &DropCount);
				if (full_DropSet) {
					if (!DropSet) {
						DBG_IND("[%d] DropSet Begin(full)\r\n", id);
					}
					DropSet = 1;
				} else if (DropCount % gop) { //not match gop
					DropSet = 1;
				} else {
					if (DropSet) {
						DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x step1=0x%x step2=0x%x\r\n", id, nowaddr, lastWrite, step1, step2);
						DBG_IND("[%d] DropSet End (%d)\r\n", id, DropCount);
					}
					DropSet = 0;
				}
			}
			// handle drop
			if (DropSet) {
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, DropCount);
				result = FALSE;
				if (uiIsKey) {
					DBG_IND("BSM:[%d] drop I (count=%d)\r\n", id, DropCount);
				}
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_VID_DROP, DropCount);
			}
		}
		else if (uiType == BSMUX_TYPE_SUBVD) { //video sub-stream
			if ((nowaddr < lastWrite) && (nowaddr + step1 > lastWrite)) {
				if (!sub_DropSet) {
					DBG_IND("[%d] sub_DropSet Begin\r\n", id);
					DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, nowaddr, lastWrite, nowaddr + step1);
				}
				sub_DropSet = 1;
			} else if ((step2) && (buf_addr + step2 > lastWrite)) {
				if (!sub_DropSet) {
					DBG_IND("[%d] sub_DropSet Begin(padding)\r\n", id);
					DBG_IND("[%d] buf_addr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, buf_addr, lastWrite, buf_addr + step2);
				}
				sub_DropSet = 1;
			} else {
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_GOP, &gop);
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &DropCount);
				if (full_DropSet) {
					if (!sub_DropSet) {
						DBG_IND("[%d] sub_DropSet Begin(full)\r\n", id);
					}
					sub_DropSet = 1;
				} else if (DropCount % gop) { //not match gop
					sub_DropSet = 1;
				} else {
					if (sub_DropSet) {
						DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x step1=0x%x step2=0x%x\r\n", id, nowaddr, lastWrite, step1, step2);
						DBG_IND("[%d] sub_DropSet End (%d)\r\n", id, DropCount);
					}
					sub_DropSet = 0;
				}
			}
			// handle drop
			if (sub_DropSet) {
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, DropCount);
				result = FALSE;
				if (uiIsKey) {
					DBG_IND("BSM:[%d] drop I (count=%d)\r\n", id, DropCount);
				}
				bsmux_util_get_param(id, BSMUXER_PARAM_SUB_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_SUB_DROP, DropCount);
			}
		}
		else if (uiType == BSMUX_TYPE_AUDIO) { //audio
			if ((nowaddr < lastWrite) && (nowaddr + step1 > lastWrite)) {
				if (!aud_DropSet) {
					DBG_IND("[%d] aud_DropSet Begin\r\n", id);
					DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, nowaddr, lastWrite, nowaddr + step1);
				}
				aud_DropSet = 1;
			} else if ((step2) && (buf_addr + step2 > lastWrite)) {
				if (!aud_DropSet) {
					DBG_IND("[%d] aud_DropSet Begin(padding)\r\n", id);
					DBG_IND("[%d] buf_addr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, buf_addr, lastWrite, buf_addr + step2);
				}
				aud_DropSet = 1;
			} else {
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF_DROP, &DropCount);
				if ((DropSet) || (sub_DropSet) || (full_DropSet)) {
					if (!aud_DropSet) {
						DBG_IND("[%d] aud_DropSet Begin(sync)\r\n", id);
						DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x step1=0x%x step2=0x%x\r\n", id, nowaddr, lastWrite, step1, step2);
					}
					aud_DropSet = 1;
				} else {
					if (aud_DropSet) {
						DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x step1=0x%x step2=0x%x\r\n", id, nowaddr, lastWrite, step1, step2);
						DBG_IND("[%d] aud_DropSet End (%d)\r\n", id, DropCount);
					}
					aud_DropSet = 0;
				}
			}
			// handle drop
			if (aud_DropSet) {
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALAF_DROP, DropCount);
				result = FALSE;
				DBG_IND("[%d] drop audio %d\r\n", id, DropCount);
				bsmux_util_get_param(id, BSMUXER_PARAM_AUD_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_AUD_DROP, DropCount);
			}
		}
		else if (uiType == BSMUX_TYPE_NIDXT) { //nidx
			DBG_IND("[%d](nidx) uiBSSize=0x%x\r\n", id, uiBSSize);
			if ((nowaddr < lastWrite) && (nowaddr + step1 > lastWrite)) {
				DBG_ERR("[%d](nidx) nowaddr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, nowaddr, lastWrite, nowaddr + step1);
				result = FALSE;
			} else if ((step2) && (buf_addr + step2 > lastWrite)) {
				DBG_ERR("[%d](nidx)(padding) buf_addr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, buf_addr, lastWrite, buf_addr + step2);
				result = FALSE;
			} else {

			}
		}
		else if (uiType == BSMUX_TYPE_THUMB) { //thumb
			BSMUX_MSG("[%d]does nothing because the thumb data is not copied to the mdat buffer.\r\n", id);
		}
		else { //data
			#if 0
			if ((DropSet) || (sub_DropSet) || (aud_DropSet) || (full_DropSet)) {
				result = FALSE;
				DBG_IND("[%d] drop data(%d)\r\n", id, uiType);
			}
			#else
			//dat_DropSet
			if ((nowaddr < lastWrite) && (nowaddr + step1 > lastWrite)) {
				if (!dat_DropSet) {
					DBG_IND("[%d] dat_DropSet Begin\r\n", id);
					DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, nowaddr, lastWrite, nowaddr + step1);
				}
				dat_DropSet = 1;
			} else if ((step2) && (buf_addr + step2 > lastWrite)) {
				if (!dat_DropSet) {
					DBG_IND("[%d] dat_DropSet Begin(padding)\r\n", id);
					DBG_IND("[%d] buf_addr=0x%x lastWrite=0x%x updateaddr=0x%x\r\n", id, buf_addr, lastWrite, buf_addr + step2);
				}
				dat_DropSet = 1;
			} else {
				if ((DropSet) || (sub_DropSet) || (aud_DropSet) || (full_DropSet)) {
					if (!dat_DropSet) {
						DBG_IND("[%d] dat_DropSet Begin(sync)\r\n", id);
						DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x step1=0x%x step2=0x%x\r\n", id, nowaddr, lastWrite, step1, step2);
					}
					dat_DropSet = 1;
				} else {
					if (dat_DropSet) {
						DBG_IND("[%d] nowaddr=0x%x lastWrite=0x%x step1=0x%x step2=0x%x\r\n", id, nowaddr, lastWrite, step1, step2);
						DBG_IND("[%d] dat_DropSet End\r\n", id);
					}
					dat_DropSet = 0;
				}
			}
			// handle drop
			if (dat_DropSet) {
				result = FALSE;
				DBG_IND("[%d] drop data(%d)\r\n", id, uiType);
				bsmux_util_get_param(id, BSMUXER_PARAM_DAT_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_DAT_DROP, DropCount);
			}
			#endif
		}

		#if 1
		bsmux_util_set_param(id, BSMUXER_PARAM_FULL_SET, full_DropSet);
		bsmux_util_set_param(id, BSMUXER_PARAM_VID_SET, DropSet);
		bsmux_util_set_param(id, BSMUXER_PARAM_AUD_SET, aud_DropSet);
		bsmux_util_set_param(id, BSMUXER_PARAM_SUB_SET, sub_DropSet);
		bsmux_util_set_param(id, BSMUXER_PARAM_DAT_SET, dat_DropSet);
		#endif
	}

	return result;
}

BOOL bsmux_util_check_duration(UINT32 id, void * p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *p_in_buf = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	UINT64 t_stamp;
	UINT32 drop_en = 0;
	UINT32 t_dur_us = 0;
	BOOL result = TRUE;

	bsmux_util_get_param(id, BSMUXER_PARAM_DUR_US_MAX, &t_dur_us);
	t_stamp = bsmux_util_calc_timestamp(id, p_in_buf->uiTimeStamp);
	if (t_stamp > (UINT64)t_dur_us) {
		BSMUX_MSG("bsmux_util_check_duration:[%d][0x%x][0x%x][0x%x]dur=%llu\r\n",
			id, p_in_buf->uiType, p_in_buf->uiBSMemAddr, p_in_buf->uiBSSize, t_stamp);
		result = FALSE; //invalid
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_EN_DROP, &drop_en);

	if (drop_en) {
		UINT32 DropCount = 0;

		if (p_in_buf->uiType == BSMUX_TYPE_VIDEO) {
			if (result == FALSE) { // invalid and handle drop
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, DropCount);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_VID_DROP, DropCount);
				BSMUX_MSG("bsmux_util_check_duration:[%d]drop(main:%d)\r\n", id, DropCount);
				result = TRUE; //drop
			}
		}
		else if (p_in_buf->uiType == BSMUX_TYPE_SUBVD) {
			if (result == FALSE) { // invalid and handle drop
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &DropCount);
				DropCount += 1;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, DropCount);
				bsmux_util_get_param(id, BSMUXER_PARAM_SUB_DROP, &DropCount);
				DropCount += 1;
				bsmux_util_set_param(id, BSMUXER_PARAM_SUB_DROP, DropCount);
				BSMUX_MSG("bsmux_util_check_duration:[%d]drop(sub:%d)\r\n", id, DropCount);
				result = TRUE; //drop
			}
		}
	}

	return result;
}

BOOL bsmux_util_check_buflocksts(UINT32 id, void *p_bsq)
{
	BOOL result = TRUE;
	UINT32 lock_ms = 0, tbr = 0, sub_tbr = 0, step;
	UINT32 lock_num, useable_num = 0, used_num = 0, curr_num = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC, &lock_ms);
	if (lock_ms == 0)
		return result;

	bsmux_util_get_param(id, BSMUXER_PARAM_VID_TBR, &tbr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_TBR, &sub_tbr);
	step = bsmux_util_set_writeblock_size(id);
	lock_num = ALIGN_CEIL(((tbr + sub_tbr) * (lock_ms/1000)), step) / step;

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_USEABLE, &useable_num); //size
	useable_num = useable_num / step; //blk num
	if (lock_num < BSMUX_LOCKNUMMIN) {
		lock_num = BSMUX_LOCKNUMMIN;
	}
	if (lock_num > useable_num) {
		lock_num = useable_num;
	}

	// critical section: lock
	bsmux_buffer_lock();
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_USEDBLOCK, &used_num);
	bsmux_buffer_unlock();
	// critical section: unlock

	// if only remain one blk (current), need to check current buffer usage
	if (used_num == (lock_num-1)) {
		BSMUX_SAVEQ_BS_INFO *p_in_buf = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
		UINT32 buf_addr = 0, buf_end = 0, nowaddr = 0, last2card = 0, real2card = 0; // current buffer usage
		UINT32 lastWrite = 0;
		UINT32 uiBSSize = 0, step1 = 0, step2 = 0; // current bs (use tbr as bssize)
		UINT32 vfn = 0, afn = 0, sub_vfn = 0; // check front header case
		UINT32 fastput_en = 0, blksize = 0, blkPut = 0, closeFlag = 0; // check front header case

		// current buffer usage
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &buf_addr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &buf_end);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &nowaddr);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, &lastWrite);
		if ((real2card >= buf_addr) && (real2card <= buf_end)) {
			// if mdat buffer area, use real2card. or use last keep
			lastWrite = real2card;
		}

		// current bs (consider make front header case and nidx data)
		bsmux_ctrl_bs_predequeue(id, p_bsq);
		if ((p_in_buf) && (p_in_buf->uiBSSize)) {
			uiBSSize = p_in_buf->uiBSSize + BSMUX_MAX_NIDX_BLK;
		} else {
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_TBR, &uiBSSize);
		}
		uiBSSize = bsmux_util_muxalign(id, uiBSSize);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &afn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		if ((vfn == 0) && (afn == 0) && (sub_vfn == 0)) {
			bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fastput_en);
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEBLOCK, &blksize);
			if (fastput_en) { // if need fast put first ops buf to open file
				#if 1	//fast put flow ver3
				uiBSSize += blksize - bsmux_util_calc_padding_size(id);
				#else
				uiBSSize += blksize - MP_AUDENC_AAC_RAW_BLOCK * 3;
				#endif	//fast put flow ver3
			} else {
				uiBSSize += bsmux_mdat_calc_header_size(id);
			}
		}

		// calculate
		if ((nowaddr + uiBSSize) > buf_end) {
			step1 = buf_end - nowaddr;
			step2 = uiBSSize - step1;
		} else {
			step1 = uiBSSize;
			step2 = 0;
		}

		// check drop or not and handle drop
		bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_CLOSE_FLAG, &closeFlag);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_BLKPUT, &blkPut);
		if ((closeFlag) && (nowaddr == lastWrite)) {
			// mdat buffer full
			curr_num = 1;
			goto label_check;
		} // (not care) make front header case
		{ //data
			if ((nowaddr < lastWrite) && (nowaddr + step1 > lastWrite)) {
				curr_num = 1;
				goto label_check;
			} else if ((step2) && (buf_addr + step2 > lastWrite)) {
				curr_num = 1;
				goto label_check;
			}
		}
		DBG_IND("only remain one blk: now(0x%lx) step(0x%lx) lastWrite(0x%lx)\r\n",
			(unsigned long)nowaddr, (unsigned long)step1, (unsigned long)lastWrite);
	}

label_check:
	if (used_num + curr_num >= lock_num) {
		result = FALSE;
	}

	return result;
}

BOOL bsmux_util_check_buflockdur(UINT32 id, void *p_bsq)
{
	BOOL result = TRUE;
	UINT32 t_dur_ms = 0;
	static VOS_TICK t_1_us = 0, t_2_us = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT, &t_dur_ms);
	if (t_dur_ms == 0)
		return result;

	if (t_1_us == 0) {
		vos_perf_mark(&t_1_us);
	}
	vos_perf_mark(&t_2_us);

	if (vos_perf_duration(t_1_us, t_2_us)/1000 > (VOS_TICK)t_dur_ms) {
		DBG_WRN("[%d] buflock duration %dms\r\n", id, vos_perf_duration(t_1_us, t_2_us)/1000);
		t_1_us = 0;
		result = FALSE;
	}

	if (result == TRUE) {
		//reset
		t_1_us = 0;
	}

	return result;
}

/*-----------------------------------------------------------------------------*/
/* Plugin Functions                                                            */
/*-----------------------------------------------------------------------------*/
ER bsmux_util_plugin(UINT32 id)
{
	CONTAINERMAKER *pCMaker = NULL;
	CONTAINERMAKER *ptrCon = NULL;
	UINT32 m0 = 0;
	UINT32 filetype;
	ER ret = E_NOSPT;

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_NOEXS;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);

	switch (filetype) {
	case MEDIA_FILEFORMAT_MOV:
	case MEDIA_FILEFORMAT_MP4:
		// create mdat makerobj
		mov_ResetContainerMaker();
		mov_setConMakerId(id);
		ptrCon = mov_getContainerMaker();
		if (bsmux_util_is_null_obj((void *)ptrCon)) {
			DBG_ERR("[%d] mov_getContainerMaker null\r\n", id);
			return E_NOEXS;
		}
		m0 = ptrCon->id;
		pCMaker->Initialize = ptrCon->Initialize;
		pCMaker->SetMemBuf = ptrCon->SetMemBuf;
		pCMaker->MakeHeader = ptrCon->MakeHeader;
		pCMaker->UpdateHeader = ptrCon->UpdateHeader;
		pCMaker->GetInfo = ptrCon->GetInfo;
		pCMaker->SetInfo = ptrCon->SetInfo;
		pCMaker->CustomizeFunc = ptrCon->CustomizeFunc;
		pCMaker->checkID = ptrCon->checkID;
		//pCMaker->cbShowErrMsg = debug_msg;
		pCMaker->cbWriteFile = NULL;
		pCMaker->cbReadFile = NULL;
		pCMaker->id = m0;
		ret = MovWriteLib_RegisterObjCB(pCMaker);
		bsmux_maker_init_mov(id);
		#if 0
		if (prjSet->hdr.gpstagsize) {
			if (prjSet->hdr.gpsbuffer) {
				bsmux_util_set_minfo(MEDIAWRITE_SETINFO_GPSBUFFER, prjSet->hdr.gpsbuffer, prjSet->hdr.gpsbufsize, m0);
			} else {
				DBG_ERR("setGPS size, but gpsbuffer ZERO!\r\n");
			}
			if (prjSet->hdr.gpstagaddr) {
				bsmux_util_set_minfo(MEDIAWRITE_SETINFO_GPSTAGBUFFER, prjSet->hdr.gpstagaddr, prjSet->hdr.gpstagsize, m0);
			} else {
				DBG_ERR("setGPS size, but gpstagaddr ZERO!\r\n");
			}
		}
		#endif
		// plugin mdat utility
		g_pBsMuxUtil = &g_BsMuxUtilMdat;
		break;
	case MEDIA_FILEFORMAT_TS:
		// create ts makerobj
		ts_ResetContainerMaker();
		ptrCon = ts_getContainerMaker();
		if (bsmux_util_is_null_obj((void *)ptrCon)) {
			DBG_ERR("[%d] ts_getContainerMaker null\r\n", id);
			return E_NOEXS;
		}
		ptrCon->id = id;
		memcpy(pCMaker, ptrCon, sizeof(CONTAINERMAKER));
		bsmux_maker_init_ts(id);
		// plugin ts utility
		g_pBsMuxUtil = &g_BsMuxUtilTs;
		ret = E_OK;
		break;
	default:
		DBG_ERR("id = %d type = %d!!!!!!\r\n", id, filetype);
		DBG_ERR("others UNSUPPORTED!!!!!!\r\n");
		break;
	}

	// init wrblk count
	bsmux_util_set_param(id, BSMUXER_PARAM_WRINFO_WRBLK_CNT, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_WRINFO_CLOSE_FLAG, 0);
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_CUTBLK, 0);

	// init drop count
	bsmux_util_set_param(id, BSMUXER_PARAM_VID_DROP, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_AUD_DROP, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_SUB_DROP, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_DAT_DROP, 0);

	// init sync info
	bsmux_init_sync_info(id);

	// init emrloop
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &filetype);
	bsmux_util_set_param(id, BSMUXER_PARAM_FILE_EMRLOOP, (filetype != 0));
	bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, id);
	bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_CNT, 0);

	// try to init storage buffer
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &m0);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &filetype);
	if (filetype && m0) {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_ENTRYPOS_ENABLE, TRUE, 0, id);
		bsmux_util_strgbuf_init(id);
		bsmux_util_set_param(id, BSMUXER_PARAM_NIDX_EN, 0); //unsupported
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_ACT, 1);
	} else {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_ENTRYPOS_ENABLE, FALSE, 0, id);
		if (filetype) {
			bsmux_util_set_param(id, BSMUXER_PARAM_EN_STRGBUF, 0);
		}
	}

	// set boxtag size
	bsmux_util_get_param(id, BSMUXER_PARAM_BOXTAG_SIZE, &m0);
	if (m0 == 64) {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_EN_CO64, 1, 0, id);
	} else {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_EN_CO64, 0, 0, id);
	}

	// set bs dur limit
	bsmux_util_get_param(id, BSMUXER_PARAM_DUR_US_MAX, &m0);
	if (m0 == 0) {
		bsmux_util_set_param(id, BSMUXER_PARAM_DUR_US_MAX, BSMUX_DUR_US_MAX);
	}

	// reset var rate
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &m0);
	bsmux_util_set_param(id, BSMUXER_PARAM_VID_VAR_VFR, m0);

	return ret;
}

ER bsmux_util_plugin_dbg(UINT32 value)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->Dbg(value);
	if (E_OK != result) {
		DBG_ERR("Dbg error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_tskobj_init(UINT32 id, UINT32 *action)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->TskObjInit(id, action);
	if (E_OK != result) {
		DBG_ERR("TskObjInit error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_engine_open(void)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->EngOpen();
	if (E_OK != result) {
		DBG_ERR("EngOpen error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_engine_close(void)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->EngClose();
	if (E_OK != result) {
		DBG_ERR("EngClose error\r\n");
		return result;
	}

	return result;
}

UINT32 bsmux_util_engcpy(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method)
{
	UINT32 len;
	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return 0;
	}
	len = g_pBsMuxUtil->EngCpy(uiDst, uiSrc, uiSize, method);
	return len;
}

ER bsmux_util_plugin_get_size(UINT32 id, UINT32 *p_size)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->MemGetSize(id, p_size);
	if (E_OK != result) {
		DBG_ERR("MemGetSize error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_plugin_set_size(UINT32 id, UINT32 addr, UINT32 *p_size)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->MemSetSize(id, addr, p_size);
	if (E_OK != result) {
		DBG_ERR("MemGetSize error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_plugin_clean(UINT32 id)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->Clean(id);
	if (E_OK != result) {
		DBG_ERR("Clean error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_update_vidinfo(UINT32 id, UINT32 addr, void *p_bsq)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->UpdateVidInfo(id, addr, p_bsq);
	if (E_OK != result) {
		DBG_ERR("UpdateVidInfo error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_add_lasting(UINT32 id)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	bsmux_mkhdr_lock();
	result = g_pBsMuxUtil->AddLast(id);
	bsmux_mkhdr_unlock();
	if (E_OK != result) {
		DBG_ERR("AddLast error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_put_lasting(UINT32 id)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->PutLast(id);
	if (E_OK != result) {
		DBG_ERR("PutLast error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_save_entry(UINT32 id, void * p_bsq)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->SaveEntry(id, p_bsq);
	if (E_OK != result) {
		DBG_ERR("SaveEntry error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_nidx_pad(UINT32 id)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->NidxPad(id);
	if (E_OK != result) {
		DBG_ERR("NidxPad error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_make_header(UINT32 id)
{
	CONTAINERMAKER *pCMaker = NULL;
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_SYS;
	}

	bsmux_mkhdr_lock();
	result = g_pBsMuxUtil->MakeHeader(id, (void *)pCMaker);
	bsmux_mkhdr_unlock();
	if (E_OK != result) {
		DBG_ERR("MakeHeader error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_update_header(UINT32 id)
{
	CONTAINERMAKER *pCMaker = NULL;
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_SYS;
	}

	bsmux_mkhdr_lock();
	result = g_pBsMuxUtil->UpdateHeader(id, (void *)pCMaker);
	bsmux_mkhdr_unlock();
	if (E_OK != result) {
		DBG_ERR("MakeHeader error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_make_pes(UINT32 id, void * p_bsq)
{
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	result = g_pBsMuxUtil->MakePES(id, p_bsq);
	if (E_OK != result) {
		DBG_ERR("MakePES error\r\n");
		return result;
	}

	return result;
}

ER bsmux_util_make_pat(UINT32 id)
{
	ER result;
	UINT32 patAddr = 0;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &patAddr);

	result = g_pBsMuxUtil->MakePAT(id, patAddr);
	if (E_OK != result) {
		DBG_ERR("MakePAT error\r\n");
		return result;
	}

	patAddr += MOVREC_TS_PACKET_SIZE;
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_NOWADDR, patAddr);

	return result;
}

ER bsmux_util_make_moov(UINT32 id, UINT32 minus1sec)
{
	CONTAINERMAKER *pCMaker = NULL;
	ER result;

	if (bsmux_util_is_null_obj((void *)g_pBsMuxUtil)) {
		DBG_ERR("g_pBsMuxUtil null\r\n");
		return E_NOEXS;
	}

	pCMaker = bsmux_get_maker_obj(id);
	if (bsmux_util_is_null_obj((void *)pCMaker)) {
		DBG_ERR("[%d] get maker obj null\r\n", id);
		return E_SYS;
	}

	bsmux_mkhdr_lock();
	result = g_pBsMuxUtil->MakeMoov(id, (void *)pCMaker, minus1sec);
	bsmux_mkhdr_unlock();
	if (E_OK != result) {
		DBG_ERR("MakeMoov error\r\n");
		return result;
	}

	return result;
}

