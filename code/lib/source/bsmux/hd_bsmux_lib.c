/**
	@brief 		bsmux library.\n

	@file 		hd_bsmux_lib.c

	@ingroup 	mBsMux

	@note 		Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include "hdal.h"

// INCLUDE PROTECTED
#include "bsmux_init.h"
unsigned int BSMUX_debug_level = NVT_DBG_WRN;
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_BSMUX_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_BSMUX
#define DEV_COUNT       ISF_MAX_BSMUX
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        32
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       16

#define HD_DEV_BASE     0x3200
#define HD_DEV(did)     (HD_DEV_BASE + (did))
#define HD_DEV_MAX      (HD_DEV_BASE + 32 - 1)

#define LIB_BSMUX_PARAM_BEGIN   HD_BSMUX_PARAM_MIN
#define LIB_BSMUX_PARAM_END     HD_BSMUX_PARAM_MAX
#define LIB_BSMUX_PARAM_NUM     (LIB_BSMUX_PARAM_END - LIB_BSMUX_PARAM_BEGIN)

//notice: max path num 16
#define HD_MAX_PATH_NUM	HD_BSMUX_MAX_CTRL

//notice: max meta data num 10
#define HD_BSMUX_META_NUM BSMUX_MAX_META_NUM

// INTERNAL DEBUG
#define DEBUG_COMMON    DISABLE
#define HD_BSMUX_FUNC(fmtstr, args...) do { if(DEBUG_COMMON) DBG_DUMP("%s(): " fmtstr, __func__, ##args); } while(0)
#define HD_BSMUX_DUMP(fmtstr, args...) do { if(DEBUG_COMMON) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
//                                            set/get             set/get/start
//                                              ^ |                  ^ |
//                                              | v                  | v
//  [UNINIT] -- init   --> [INIT] -- open  --> [IDLE]  -- start --> [RUN]
//          <-- uninit --          <-- close --         <-- stop  --
//
typedef enum _HD_BSMUX_STAT {
	HD_BSMUX_STAT_UNINIT = 0,
	HD_BSMUX_STAT_INIT,
	HD_BSMUX_STAT_IDLE,
	HD_BSMUX_STAT_RUN,
} HD_BSMUX_STAT;

typedef struct _BSMUX_INFO_PRIV {
	HD_BSMUX_STAT           status;
	BOOL                    b_maxmem_set;
	BOOL                    b_need_update_param[LIB_BSMUX_PARAM_NUM];
} BSMUX_INFO_PRIV;

typedef struct _BSMUX_INFO_PORT {
	HD_BSMUX_REG_CALLBACK   bsmux_reg_callback;
	HD_BSMUX_VIDEOINFO      bsmux_videoinfo;
	HD_BSMUX_VIDEOINFO      bsmux_sub_videoinfo;
	HD_BSMUX_AUDIOINFO      bsmux_audioinfo;
	HD_BSMUX_FILEINFO       bsmux_fileinfo;
	HD_BSMUX_BUFINFO        bsmux_bufinfo;
	HD_BSMUX_WRINFO         bsmux_wrinfo;
	HD_BSMUX_EXTINFO        bsmux_extinfo;
	HD_BSMUX_GPS_DATA       bsmux_gps_data;
	HD_BSMUX_USER_DATA      bsmux_user_data;
	HD_BSMUX_CUST_DATA      bsmux_cust_data;
	HD_BSMUX_PUT_DATA       bsmux_put_data;
	HD_BSMUX_TRIG_EMR       bsmux_trig_emr;
	HD_BSMUX_TRIG_EVENT     bsmux_trig_event;
	HD_BSMUX_EN_UTIL        bsmux_en_util;
	HD_BSMUX_CALC_SEC       bsmux_calc_sec;

	//private data
	BSMUX_INFO_PRIV         priv;
} BSMUX_INFO_PORT;

typedef struct _BSMUX_INFO {
	BSMUX_INFO_PORT *       port;
} BSMUX_INFO;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
static UINT32 _max_path_count = 0;

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;	\
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) { \
			UINT32 id = (dev_id) - HD_DEV_BASE; \
			if(id < DEV_COUNT) { \
				(dev_id) = DEV_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_OUT_ID(out_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((out_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((out_id) >= HD_OUT_BASE && (out_id) <= HD_OUT_MAX) { \
			UINT32 id = (out_id) - HD_OUT_BASE; \
			if((id < OUT_COUNT) && (id < _max_path_count)) { \
				(out_id) = OUT_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_IN_ID(in_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((in_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((in_id) >= HD_IN_BASE && (in_id) <= HD_IN_MAX) { \
			UINT32 id = (in_id) - HD_IN_BASE; \
			if((id < OUT_COUNT) && (id < _max_path_count)) { \
				(in_id) = IN_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
BSMUX_INFO bsmux_info[DEV_COUNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
void _hd_bsmux_cfg_max(UINT32 maxpath)
{
	if (HD_MAX_PATH_NUM != BSMUX_MAX_CTRL_NUM) {
		DBG_WRN("max num not match (hdal %d lib %d)\n", HD_MAX_PATH_NUM, BSMUX_MAX_CTRL_NUM);
	}
	if (!bsmux_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > HD_MAX_PATH_NUM) _max_path_count = HD_MAX_PATH_NUM;
	}
}

INT _hd_bsmux_check_path_id(HD_PATH_ID path_id)
{
	#if 0
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	#endif
	INT ret = 0;
	#if 0
	//convert self_id
	//_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) { return rv;}

	//convert out_id
	//if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
	//_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { return rv;}

	//convert in_id
	//if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { return HD_ERR_IO;}
	//_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
	#endif
	return ret;
}

INT _hd_bsmux_check_status(HD_PATH_ID path_id, HD_BSMUX_STAT status)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status != status) {
		return -1;
	}

	return ret;
}

INT _hd_bsmux_set_status(HD_PATH_ID path_id, HD_BSMUX_STAT status)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = status;

	return ret;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_VIDEOINFO(HD_BSMUX_VIDEOINFO *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// vidcodec
	if ((p_setting->vidcodec != MEDIAVIDENC_H264) &&
		(p_setting->vidcodec != MEDIAVIDENC_H265) &&
		(p_setting->vidcodec != MEDIAVIDENC_MJPG)) {
		DBG_DUMP("bsmux set vidcodec (%d) error\r\n", p_setting->vidcodec);
		goto exit;
	}
	// vfr
	if (p_setting->vfr <= 0) {
		DBG_DUMP("bsmux set vfr (%d) error\r\n", p_setting->vfr);
		goto exit;
	}
	// width
	if (p_setting->width <= 0) {
		DBG_DUMP("bsmux set width (%d) error\r\n", p_setting->width);
		goto exit;
	}
	// height
	if (p_setting->height <= 0) {
		DBG_DUMP("bsmux set height (%d) error\r\n", p_setting->height);
		goto exit;
	}
	// tbr
	if (p_setting->tbr <= 0) {
		DBG_DUMP("bsmux set tbr (%d) error\r\n", p_setting->tbr);
		goto exit;
	}
	// DAR
	if ((p_setting->DAR != MEDIAREC_DAR_DEFAULT) &&
		(p_setting->DAR != MEDIAREC_DAR_16_9)) {
		DBG_DUMP("bsmux set DAR (%d) error\r\n", p_setting->DAR);
		goto exit;
	}
	#if 0
	// gop
	if ((p_setting->gop <= 0) || (p_setting->vfr % p_setting->gop != 0) || (p_setting->gop > p_setting->vfr)) {
		DBG_DUMP("bsmux set gop (%d) error\r\n", p_setting->gop);
		goto exit;
	}
	#endif

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_AUDIOINFO(HD_BSMUX_AUDIOINFO *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// codectype
	if ((p_setting->codectype < MOVAUDENC_PCM) ||
		(p_setting->codectype > MOVAUDENC_ALAW)) {
		DBG_DUMP("bsmux set audcodec (%d) error\r\n", p_setting->codectype);
		goto exit;
	}
	// chs
	if (p_setting->chs <= 0) {
		DBG_DUMP("bsmux set chs (%d) error\r\n", p_setting->chs);
		goto exit;
	}
	// asr
	if (p_setting->asr <= 0) {
		DBG_DUMP("bsmux set asr (%d) error\r\n", p_setting->asr);
		goto exit;
	}
	// aud_en
	if ((p_setting->aud_en != 0) && (p_setting->aud_en != 1)) {
		DBG_DUMP("bsmux set aud_en (%d) error\r\n", p_setting->aud_en);
		goto exit;
	}
	// adts_bytes
	if ((p_setting->adts_bytes != 0) && (p_setting->adts_bytes != 7)) {
		DBG_DUMP("bsmux set adts (%d) error\r\n", p_setting->adts_bytes);
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_FILEINFO(HD_BSMUX_FILEINFO *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// emron
	if ((p_setting->emron != 0) && (p_setting->emron != 1)) {
		DBG_DUMP("bsmux set emron (%d) error\r\n", p_setting->emron);
		goto exit;
	}
	// emrloop
	if ((p_setting->emrloop != 0) && (p_setting->emrloop != 1)) {
		DBG_DUMP("bsmux set emrloop (%d) error\r\n", p_setting->emrloop);
		goto exit;
	}
	#if 0
	// strgid
	if ((p_setting->strgid != 0) && (p_setting->strgid != 1)) {
		DBG_DUMP("bsmux set strgid (%d) error\r\n", p_setting->strgid);
		goto exit;
	}
	#endif
	#if 0
	// seamlessSec
	if (p_setting->seamlessSec <= 0) {
		DBG_DUMP("bsmux set seamlessSec (%d) error\r\n", p_setting->seamlessSec);
		goto exit;
	}
	// rollbacksec
	if ((p_setting->rollbacksec < BSMUX_ROLLBACKSEC_MIN) ||
		(p_setting->rollbacksec > BSMUX_ROLLBACKSEC_MAX)) {
		DBG_DUMP("bsmux set rollbacksec (%d) error\r\n", p_setting->rollbacksec);
		goto exit;
	}
	// keepsec
	if ((p_setting->keepsec < BSMUX_KEEPSEC_MIN) ||
		(p_setting->keepsec > BSMUX_KEEPSEC_MAX)) {
		DBG_DUMP("bsmux set keepsec (%d) error\r\n", p_setting->keepsec);
		goto exit;
	}
	#endif
	#if 0
	// endtype
	if ((p_setting->endtype < MOVREC_ENDTYPE_NORMAL) ||
		(p_setting->endtype > MOVREC_ENDTYPE_MAX)) {
		DBG_DUMP("bsmux set endtype (%d) error\r\n", p_setting->endtype);
		goto exit;
	}
	#endif
	// filetype
	if ((p_setting->filetype != MEDIA_FILEFORMAT_MOV) &&
		(p_setting->filetype != MEDIA_FILEFORMAT_MP4) &&
		(p_setting->filetype != MEDIA_FILEFORMAT_TS) &&
		(p_setting->filetype != MEDIA_FILEFORMAT_JPG)) {
		DBG_DUMP("bsmux set filetype (%d) error\r\n", p_setting->filetype);
		goto exit;
	}
	// recformat
	if ((p_setting->recformat != MEDIAREC_AUD_VID_BOTH) &&
		(p_setting->recformat != MEDIAREC_VID_ONLY) &&
		(p_setting->recformat != MEDIAREC_TIMELAPSE) &&
		(p_setting->recformat != MEDIAREC_GOLFSHOT)) {
		DBG_DUMP("bsmux set recformat (%d) error\r\n", p_setting->recformat);
		goto exit;
	}
	#if 0
	// playvfr
	if ((p_setting->playvfr != BSMUX_PLAYVFR_30) &&
		(p_setting->playvfr != BSMUX_PLAYVFR_15)) {
		DBG_DUMP("bsmux set playvfr (%d) error\r\n", p_setting->playvfr);
		goto exit;
	}
	#endif
	#if 0
	// revsec
	if ((p_setting->revsec < BSMUX_RESVSEC_MIN) ||
		(p_setting->revsec > BSMUX_RESVSEC_MAX)) {
		DBG_DUMP("bsmux set revsec (%d) error\r\n", p_setting->revsec);
		goto exit;
	}
	#endif
	// overlop_on
	if ((p_setting->overlop_on != 0) && (p_setting->overlop_on != 1)) {
		DBG_DUMP("bsmux set overlop_on (%d) error\r\n", p_setting->overlop_on);
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_BUFINFO(HD_BSMUX_BUFINFO *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// videnc
	if (p_setting->videnc.buf_size) {
		if (p_setting->videnc.phy_addr == 0) {
			DBG_DUMP("bsmux set videnc phy_addr (%d) error\r\n", p_setting->videnc.phy_addr);
			goto exit;
		}
	}
	// audenc
	if (p_setting->audenc.buf_size) {
		if (p_setting->audenc.phy_addr == 0) {
			DBG_DUMP("bsmux set audenc phy_addr (%d) error\r\n", p_setting->audenc.phy_addr);
			goto exit;
		}
	}

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_GPS_DATA(HD_BSMUX_GPS_DATA *p_gps_data, CHAR *p_ret_string, INT max_str_len)
{
	// gpson
	if ((p_gps_data->gpson != 0) && (p_gps_data->gpson != 1)) {
		DBG_DUMP("bsmux set gpson (%d) error\r\n", p_gps_data->gpson);
		goto exit;
	}
	// gpsdataadr
	if (p_gps_data->gpson) {
		if (p_gps_data->gpsdataadr == 0) {
			DBG_DUMP("bsmux set gpsdataadr (%d) error\r\n", p_gps_data->gpsdataadr);
			goto exit;
		}
	}
	// gpsdatasize
	if (p_gps_data->gpson) {
		if ((p_gps_data->gpsdatasize > (BSMUX_GPS_MIN - 16)) || (p_gps_data->gpsdatasize == 0)) {
			DBG_DUMP("bsmux set gpsdatasize (%d) error\r\n", p_gps_data->gpsdatasize);
			goto exit;
		}
	}

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_USER_DATA(HD_BSMUX_USER_DATA *p_user_data, CHAR *p_ret_string, INT max_str_len)
{
	// useron
	if ((p_user_data->useron != 0) && (p_user_data->useron != 1)) {
		DBG_DUMP("bsmux set useron (%d) error\r\n", p_user_data->useron);
		goto exit;
	}
	// userdataadr
	if (p_user_data->useron) {
		if (p_user_data->userdataadr == 0) {
			DBG_DUMP("bsmux set userdataadr (%d) error\r\n", p_user_data->userdataadr);
			goto exit;
		}
	}
	// userdatasize
	if (p_user_data->useron) {
		if (p_user_data->userdatasize == 0) {
			DBG_DUMP("bsmux set userdatasize (%d) error\r\n", p_user_data->userdatasize);
			goto exit;
		}
	}

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_CUST_DATA(HD_BSMUX_CUST_DATA *p_cust_data, CHAR *p_ret_string, INT max_str_len)
{
	// custon
	if ((p_cust_data->custon != 0) && (p_cust_data->custon != 1)) {
		DBG_DUMP("bsmux set custon (%d) error\r\n", p_cust_data->custon);
		goto exit;
	}
	// custaddr
	if (p_cust_data->custon) {
		if (p_cust_data->custaddr == 0) {
			DBG_DUMP("bsmux set custaddr (%d) error\r\n", p_cust_data->custaddr);
			goto exit;
		}
	}
	// custsize
	if (p_cust_data->custon) {
		if (p_cust_data->custsize == 0) {
			DBG_DUMP("bsmux set custsize (%d) error\r\n", p_cust_data->custsize);
			goto exit;
		}
	}

	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_WRINFO(HD_BSMUX_WRINFO *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// flush_freq
	if (p_setting->flush_freq > 10) { //temp limit 10
		DBG_DUMP("bsmux set flush_freq (%d) error\r\n", p_setting->flush_freq);
		goto exit;
	}
	// wrblk_size
	if ((p_setting->wrblk_size < 524288) || (p_setting->wrblk_size > 4194304)) { //temp limit 0.5MB ~ 4MB
		DBG_DUMP("bsmux set wrblk_size (%d) error\r\n", p_setting->wrblk_size);
		goto exit;
	}
	return 0;
exit:
	return -1;
}

static INT _hd_bsmux_verify_param_HD_BSMUX_EXTINFO(HD_BSMUX_EXTINFO *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// unit
	if (p_setting->unit > 30) { //temp limit 30
		DBG_DUMP("bsmux set ext unit (%d) error\r\n", p_setting->unit);
		goto exit;
	}
	// max_num
	if (p_setting->max_num > 2) { //temp limit 2
		DBG_DUMP("bsmux set ext max_num (%d) error\r\n", p_setting->max_num);
		goto exit;
	}
	// enable
	if ((p_setting->enable != 0) && (p_setting->enable != 1)) {
		DBG_DUMP("bsmux set ext enable (%d) error\r\n", p_setting->enable);
		goto exit;
	}
	return 0;
exit:
	return -1;
}

INT _hd_bsmux_check_params_range(HD_BSMUX_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	INT ret = 0;
	switch (id) {
	case HD_BSMUX_PARAM_VIDEOINFO:
		ret = _hd_bsmux_verify_param_HD_BSMUX_VIDEOINFO((HD_BSMUX_VIDEOINFO *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_AUDIOINFO:
		ret = _hd_bsmux_verify_param_HD_BSMUX_AUDIOINFO((HD_BSMUX_AUDIOINFO *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_FILEINFO:
		ret = _hd_bsmux_verify_param_HD_BSMUX_FILEINFO((HD_BSMUX_FILEINFO *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_BUFINFO:
		ret = _hd_bsmux_verify_param_HD_BSMUX_BUFINFO((HD_BSMUX_BUFINFO *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_GPS_DATA:
		ret = _hd_bsmux_verify_param_HD_BSMUX_GPS_DATA((HD_BSMUX_GPS_DATA *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_USER_DATA:
		ret = _hd_bsmux_verify_param_HD_BSMUX_USER_DATA((HD_BSMUX_USER_DATA *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_CUST_DATA:
		ret = _hd_bsmux_verify_param_HD_BSMUX_CUST_DATA((HD_BSMUX_CUST_DATA *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_WRINFO:
		ret = _hd_bsmux_verify_param_HD_BSMUX_WRINFO((HD_BSMUX_WRINFO *)p_param, p_ret_string, max_str_len);
		break;
	case HD_BSMUX_PARAM_EXTINFO:
		ret = _hd_bsmux_verify_param_HD_BSMUX_EXTINFO((HD_BSMUX_EXTINFO *)p_param, p_ret_string, max_str_len);
		break;
	default:
		break;
	}
	return ret;
}

INT _hd_bsmux_check_params(HD_PATH_ID path_id)
{
	INT ret = 0;
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 bsmux_id = (UINT32)out_id;
	HD_BSMUX_VIDEOINFO main_stream = {0};
	HD_BSMUX_VIDEOINFO sub_stream = {0};
	HD_BSMUX_FILEINFO file_info = {0};
	HD_BSMUX_EN_UTIL util = {0};
	// check main stream
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_VID_VFR, &(main_stream.vfr));
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_VID_GOP, &(main_stream.gop));
	if (main_stream.vfr) {
		if (main_stream.vfr < main_stream.gop) { //=> cardv restrictions
			DBG_DUMP("[WRN][%d] mvfr(%d) mgop(%d) invalid\r\n", bsmux_id, main_stream.vfr, main_stream.gop);
			ret = 1;
		}
	} else {
		DBG_DUMP("[WRN][%d] mvfr(%d) invalid\r\n", bsmux_id, main_stream.vfr);
		ret = 1;
	}
	// check sub stream
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_VID_SUB_VFR, &(sub_stream.vfr));
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_VID_SUB_GOP, &(sub_stream.gop));
	if (sub_stream.vfr) {
		if (sub_stream.vfr < sub_stream.gop) { //=> cardv restrictions
			DBG_DUMP("[WRN][%d] svfr(%d) sgop(%d) invalid\r\n", bsmux_id, sub_stream.vfr, sub_stream.gop);
			ret = 1;
		}
	}
	// check 2v1a
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_VID_VFR, &(main_stream.vfr));
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_VID_SUB_VFR, &(sub_stream.vfr));
	if (main_stream.vfr && sub_stream.vfr) {
		if (sub_stream.vfr != main_stream.vfr) { //=> cardv restrictions
			DBG_DUMP("[WRN][%d] mvfr(%d) svfr(%d) invalid\r\n", bsmux_id, main_stream.vfr, sub_stream.vfr);
			ret = 1;
		}
	}
	// check util
	bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_FILE_FILETYPE, &(file_info.filetype));
	if (file_info.filetype == HD_BSMUX_FTYPE_MOV) {
	} else if (file_info.filetype == HD_BSMUX_FTYPE_MP4) {
	} else if (file_info.filetype == HD_BSMUX_FTYPE_TS) {
		bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_FRONT_MOOV, &(util.enable));
		if (util.enable) {
			DBG_DUMP("[WRN][%d] util(0x%x) invalid\r\n", bsmux_id, HD_BSMUX_EN_UTIL_FRONTMOOV);
			ret = 1;
		}
		bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_EN_STRGBUF, &(util.enable));
		if (util.enable) {
			DBG_DUMP("[WRN][%d] util(0x%x) invalid\r\n", bsmux_id, HD_BSMUX_EN_UTIL_STRG_BUF);
			ret = 1;
		}
		bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_EN_FASTPUT, &(util.enable));
		if (util.enable) {
			DBG_DUMP("[WRN][%d] util(0x%x) invalid\r\n", bsmux_id, HD_BSMUX_EN_UTIL_FAST_PUT);
			ret = 1;
		}
		bsmux_util_get_param(bsmux_id, BSMUXER_PARAM_EN_DROP, &(util.enable));
		if (util.enable) {
			DBG_DUMP("[WRN][%d] util(0x%x) invalid\r\n", bsmux_id, HD_BSMUX_EN_UTIL_EN_DROP);
			ret = 1;
		}
	} else {
		DBG_DUMP("[WRN][%d] ftype(%d) invalid\r\n", bsmux_id, file_info.filetype);
		ret = 1;
	}
	return ret;
}

VOID _hd_bsmux_get_param(HD_BSMUX_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	switch (id) {

	case HD_BSMUX_PARAM_VIDEOINFO:
		{
			HD_BSMUX_VIDEOINFO *p_videoinfo = (HD_BSMUX_VIDEOINFO *)p_param;
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_CODECTYPE, &p_videoinfo->vidcodec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_VFR, &p_videoinfo->vfr);
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_WIDTH, &p_videoinfo->width);
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_HEIGHT, &p_videoinfo->height);
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_TBR, &p_videoinfo->tbr);
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_DAR, &p_videoinfo->DAR);
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_GOP, &p_videoinfo->gop);

			HD_BSMUX_DUMP("BSMUX VIDEOINFO: codec %d vfr %d wid %d hei %d tbr %d DAR %d gop %d\r\n",
				p_videoinfo->vidcodec, p_videoinfo->vfr, p_videoinfo->width,
				p_videoinfo->height, p_videoinfo->tbr, p_videoinfo->DAR, p_videoinfo->gop);
		}
		break;

	case HD_BSMUX_PARAM_AUDIOINFO:
		{
			HD_BSMUX_AUDIOINFO *p_audioinfo = (HD_BSMUX_AUDIOINFO *)p_param;
			bsmux_util_get_param(idx, BSMUXER_PARAM_AUD_CODECTYPE, &p_audioinfo->codectype);
			bsmux_util_get_param(idx, BSMUXER_PARAM_AUD_CHS, &p_audioinfo->chs);
			bsmux_util_get_param(idx, BSMUXER_PARAM_AUD_SR, &p_audioinfo->asr);
			bsmux_util_get_param(idx, BSMUXER_PARAM_AUD_ON, &p_audioinfo->aud_en);
			bsmux_util_get_param(idx, BSMUXER_PARAM_AUD_EN_ADTS, &p_audioinfo->adts_bytes);

			HD_BSMUX_DUMP("BSMUX AUDIOINFO: codec %d chs %d asr %d aud_en %d adts_bytes %d\r\n",
				p_audioinfo->codectype, p_audioinfo->chs, p_audioinfo->asr,
				p_audioinfo->aud_en, p_audioinfo->adts_bytes);
		}
		break;

	case HD_BSMUX_PARAM_FILEINFO:
		{
			HD_BSMUX_FILEINFO *p_fileinfo = (HD_BSMUX_FILEINFO *)p_param;
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_EMRON, &p_fileinfo->emron);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_EMRLOOP, &p_fileinfo->emrloop);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_DDR_ID, &p_fileinfo->ddr_id);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_SEAMLESSSEC, &p_fileinfo->seamlessSec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_ROLLBACKSEC, &p_fileinfo->rollbacksec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_KEEPSEC, &p_fileinfo->keepsec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_FILETYPE, &p_fileinfo->filetype);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_RECFORMAT, &p_fileinfo->recformat);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_PLAYFRAMERATE, &p_fileinfo->playvfr);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_BUFRESSEC, &p_fileinfo->revsec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_OVERLAP_ON, &p_fileinfo->overlop_on);
			#if BSMUX_TEST_SET_MS
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, &p_fileinfo->seamlessSec_ms);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_ROLLBACKSEC_MS, &p_fileinfo->rollbacksec_ms);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_KEEPSEC_MS, &p_fileinfo->keepsec_ms);
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_BUFRESSEC_MS, &p_fileinfo->revsec_ms);
			#endif

			HD_BSMUX_DUMP("BSMUX FILEINFO: emron %d emrloop %d filetype %d recformat %d playvfr %d\r\n",
				p_fileinfo->emron, p_fileinfo->emrloop,
				p_fileinfo->filetype, p_fileinfo->recformat, p_fileinfo->playvfr);
			HD_BSMUX_DUMP("BSMUX FILEINFO: seamless %d rollback %d keep %d revsec %d overlop_on %d\r\n",
				p_fileinfo->seamlessSec, p_fileinfo->rollbacksec, p_fileinfo->keepsec,
				p_fileinfo->revsec, p_fileinfo->overlop_on);
		}
		break;

	case HD_BSMUX_PARAM_WRINFO:
		{
			HD_BSMUX_WRINFO *p_wrinfo = (HD_BSMUX_WRINFO *)p_param;
			bsmux_util_get_param(idx, BSMUXER_PARAM_WRINFO_FLUSH_FREQ, &p_wrinfo->flush_freq);
			bsmux_util_get_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_SIZE, &p_wrinfo->wrblk_size);

			HD_BSMUX_DUMP("BSMUX_WRINFO: flush_freq %d wrblk_size %d\r\n",
				p_wrinfo->flush_freq, p_wrinfo->wrblk_size);
		}
		break;

	case HD_BSMUX_PARAM_EXTINFO:
		{
			HD_BSMUX_EXTINFO *p_extinfo = (HD_BSMUX_EXTINFO *)p_param;
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_UNIT, &p_extinfo->unit);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_MAX_NUM, &p_extinfo->max_num);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, &p_extinfo->enable);

			HD_BSMUX_DUMP("BSMUX_EXTINFO: unit %d max_num %d enable %d\r\n",
				p_extinfo->unit, p_extinfo->max_num, p_extinfo->enable);
		}
		break;

	case HD_BSMUX_PARAM_EN_UTIL:
		{
			HD_BSMUX_EN_UTIL *p_utilinfo = (HD_BSMUX_EN_UTIL *)p_param;
			if (p_utilinfo->type == HD_BSMUX_EN_UTIL_EN_DROP)
			{
				bsmux_util_get_param(idx, BSMUXER_PARAM_EN_DROP, &p_utilinfo->enable);
				bsmux_util_get_param(idx, BSMUXER_PARAM_VID_DROP, &p_utilinfo->resv[0]);
				bsmux_util_get_param(idx, BSMUXER_PARAM_SUB_DROP, &p_utilinfo->resv[1]);

				HD_BSMUX_DUMP("BSMUX_EN_UTIL: enable_drop %d main_drop %d sub_drop %d\r\n",
					p_utilinfo->enable, p_utilinfo->resv[0], p_utilinfo->resv[1]);
			}
			else if (p_utilinfo->type == HD_BSMUX_EN_UTIL_BUF_USAGE)
			{
				UINT64 total = 0, free = 0, used = 0;
				bsmux_ctrl_mem_det_range(idx);
				bsmux_ctrl_mem_get_bufinfo(idx, BSMUX_CTRL_USEABLE, &p_utilinfo->enable);
				bsmux_ctrl_mem_get_bufinfo(idx, BSMUX_CTRL_USEDSIZE, &p_utilinfo->resv[0]);
				bsmux_ctrl_mem_get_bufinfo(idx, BSMUX_CTRL_FREESIZE, &p_utilinfo->resv[1]);
				total = (UINT64)p_utilinfo->enable;
				used  = (UINT64)p_utilinfo->resv[0];
				free  = (UINT64)p_utilinfo->resv[1];
				p_utilinfo->resv[0] = (UINT32)((((used * 1000) / total) + 5) / 10);
				p_utilinfo->resv[1] = (UINT32)((((free * 1000) / total) + 5) / 10);
				p_utilinfo->enable = p_utilinfo->resv[0] + p_utilinfo->resv[1];
				if (p_utilinfo->enable != 100) {
					DBG_ERR("buf_usage=%d\r\n", p_utilinfo->enable);
				}

				HD_BSMUX_DUMP("BSMUX_EN_UTIL: used %d free %d total %d\r\n",
					p_utilinfo->resv[0], p_utilinfo->resv[1], p_utilinfo->enable);
			}
		}
		break;

	default:
		break;
	}
	return ;
}

VOID _hd_bsmux_set_param(HD_BSMUX_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	switch (id) {

	case HD_BSMUX_PARAM_VIDEOINFO:
		{
			HD_BSMUX_VIDEOINFO *p_videoinfo = (HD_BSMUX_VIDEOINFO *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_CODECTYPE, p_videoinfo->vidcodec);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_VFR, p_videoinfo->vfr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_VAR_VFR, p_videoinfo->vfr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_WIDTH, p_videoinfo->width);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_HEIGHT, p_videoinfo->height);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_TBR, p_videoinfo->tbr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_DAR, p_videoinfo->DAR);
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_GOP, p_videoinfo->gop);

			HD_BSMUX_DUMP("BSMUX VIDEOINFO: codec %d vfr %d wid %d hei %d tbr %d DAR %d gop %d\r\n",
				p_videoinfo->vidcodec, p_videoinfo->vfr, p_videoinfo->width,
				p_videoinfo->height, p_videoinfo->tbr, p_videoinfo->DAR, p_videoinfo->gop);
		}
		break;

	case HD_BSMUX_PARAM_AUDIOINFO:
		{
			HD_BSMUX_AUDIOINFO *p_audioinfo = (HD_BSMUX_AUDIOINFO *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_AUD_CODECTYPE, p_audioinfo->codectype);
			bsmux_util_set_param(idx, BSMUXER_PARAM_AUD_CHS, p_audioinfo->chs);
			bsmux_util_set_param(idx, BSMUXER_PARAM_AUD_SR, p_audioinfo->asr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_AUD_ON, p_audioinfo->aud_en);
			bsmux_util_set_param(idx, BSMUXER_PARAM_AUD_EN_ADTS, p_audioinfo->adts_bytes);

			HD_BSMUX_DUMP("BSMUX AUDIOINFO: codec %d chs %d asr %d aud_en %d adts_bytes %d\r\n",
				p_audioinfo->codectype, p_audioinfo->chs, p_audioinfo->asr,
				p_audioinfo->aud_en, p_audioinfo->adts_bytes);
		}
		break;

	case HD_BSMUX_PARAM_FILEINFO:
		{
			HD_BSMUX_FILEINFO *p_fileinfo = (HD_BSMUX_FILEINFO *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_EMRON, p_fileinfo->emron);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_EMRLOOP, p_fileinfo->emrloop);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_DDR_ID, p_fileinfo->ddr_id);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_SEAMLESSSEC, p_fileinfo->seamlessSec);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_ROLLBACKSEC, p_fileinfo->rollbacksec);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_KEEPSEC, p_fileinfo->keepsec);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_FILETYPE, p_fileinfo->filetype);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_RECFORMAT, p_fileinfo->recformat);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_PLAYFRAMERATE, p_fileinfo->playvfr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_BUFRESSEC, p_fileinfo->revsec);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_OVERLAP_ON, p_fileinfo->overlop_on);
			#if BSMUX_TEST_SET_MS
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_SEAMLESSSEC_MS, p_fileinfo->seamlessSec_ms);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_ROLLBACKSEC_MS, p_fileinfo->rollbacksec_ms);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_KEEPSEC_MS, p_fileinfo->keepsec_ms);
			bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_BUFRESSEC_MS, p_fileinfo->revsec_ms);
			#endif

			HD_BSMUX_DUMP("BSMUX FILEINFO: emron %d emrloop %d filetype %d recformat %d playvfr %d\r\n",
				p_fileinfo->emron, p_fileinfo->emrloop,
				p_fileinfo->filetype, p_fileinfo->recformat, p_fileinfo->playvfr);
			HD_BSMUX_DUMP("BSMUX FILEINFO: seamless %d rollback %d keep %d revsec %d overlop_on %d\r\n",
				p_fileinfo->seamlessSec, p_fileinfo->rollbacksec, p_fileinfo->keepsec,
				p_fileinfo->revsec, p_fileinfo->overlop_on);
		}
		break;

	case HD_BSMUX_PARAM_BUFINFO:
		{
			HD_BSMUX_BUFINFO *p_bufinfo = (HD_BSMUX_BUFINFO *)p_param;
			if (p_bufinfo->videnc.buf_size) {
				bsmux_util_set_param(idx, BSMUXER_PARAM_BUF_VIDENC_ADDR, p_bufinfo->videnc.phy_addr);
				bsmux_util_set_param(idx, BSMUXER_PARAM_BUF_VIDENC_SIZE, p_bufinfo->videnc.buf_size);
			}
			if (p_bufinfo->audenc.buf_size) {
				bsmux_util_set_param(idx, BSMUXER_PARAM_BUF_AUDENC_ADDR, p_bufinfo->audenc.phy_addr);
				bsmux_util_set_param(idx, BSMUXER_PARAM_BUF_AUDENC_SIZE, p_bufinfo->audenc.buf_size);
			}

			HD_BSMUX_DUMP("BSMUX BUFINFO: videnc phy_addr 0x%X buf_size 0x%X, audenc phy_addr 0x%X buf_size 0x%X\r\n",
				p_bufinfo->videnc.phy_addr, p_bufinfo->videnc.buf_size,
				p_bufinfo->audenc.phy_addr, p_bufinfo->audenc.buf_size);
		}
		break;

	case HD_BSMUX_PARAM_USER_DATA:
		{
			HD_BSMUX_USER_DATA *p_userdata = (HD_BSMUX_USER_DATA *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_ON, p_userdata->useron);
			bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_ADDR, p_userdata->userdataadr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_SIZE, p_userdata->userdatasize);

			HD_BSMUX_DUMP("BSMUX_USER_DATA: useron %d userdataadr %d userdatasize %d\r\n",
				p_userdata->useron, p_userdata->userdataadr, p_userdata->userdatasize);
		}
		break;

	case HD_BSMUX_PARAM_CUST_DATA:
		{
			HD_BSMUX_CUST_DATA *p_custdata = (HD_BSMUX_CUST_DATA *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_ADDR, p_custdata->custaddr);
			bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_SIZE, p_custdata->custsize);
			bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_TAG, p_custdata->custtag);

			HD_BSMUX_DUMP("BSMUX_CUST_DATA: custon %d custaddr %d custsize %d custtag %d\r\n",
				p_custdata->custon, p_custdata->custaddr, p_custdata->custsize, p_custdata->custtag);
		}
		break;

	case HD_BSMUX_PARAM_WRINFO:
		{
			HD_BSMUX_WRINFO *p_wrinfo = (HD_BSMUX_WRINFO *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_FLUSH_FREQ, p_wrinfo->flush_freq);
			bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_SIZE, p_wrinfo->wrblk_size);

			HD_BSMUX_DUMP("BSMUX_WRINFO: flush_freq %d wrblk_size %d\r\n",
				p_wrinfo->flush_freq, p_wrinfo->wrblk_size);
		}
		break;

	case HD_BSMUX_PARAM_EXTINFO:
		{
			HD_BSMUX_EXTINFO *p_extinfo = (HD_BSMUX_EXTINFO *)p_param;
			bsmux_util_set_param(idx, BSMUXER_PARAM_EXTINFO_UNIT, p_extinfo->unit);
			bsmux_util_set_param(idx, BSMUXER_PARAM_EXTINFO_MAX_NUM, p_extinfo->max_num);
			bsmux_util_set_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, p_extinfo->enable);

			HD_BSMUX_DUMP("BSMUX_EXTINFO: unit %d max_num %d enable %d\r\n",
				p_extinfo->unit, p_extinfo->max_num, p_extinfo->enable);
		}
		break;

	default:
		break;
	}
	return ;
}

INT _hd_bsmux_put_data(HD_BSMUX_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	INT rv = 0;
	HD_BSMUX_PUT_DATA *p_data = (HD_BSMUX_PUT_DATA *)p_param;
	switch (p_data->type) {

	case HD_BSMUX_PUT_DATA_TYPE_GPS:
		{
			int r;
			BSMUXER_DATA bsmux_data = {0};
			bsmux_data.type = BSMUX_TYPE_GPSIN;
			bsmux_data.bSMemAddr = p_data->phy_addr; //pa
			bsmux_data.bSSize = p_data->size;
			bsmux_data.bSVirAddr = p_data->vir_addr; //va
			r = bsmux_ctrl_in(idx, (void *) &bsmux_data);
			if (r != E_OK) {
				DBG_ERR("gps fail (%d)\r\n", r);
				rv = HD_ERR_FAIL; ///< already executed and failed
			} else {
				rv = HD_OK;
			}
		}
		break;

	case HD_BSMUX_PUT_DATA_TYPE_USER:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_ADDR, p_data->vir_addr); //va
			bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_SIZE, p_data->size);
			rv = HD_OK;
		}
		break;

	case HD_BSMUX_PUT_DATA_TYPE_CUST:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_ADDR, p_data->vir_addr); //va
			bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_SIZE, p_data->size);
			bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_TAG, p_data->resv);
			rv = HD_OK;
		}
		break;

	case HD_BSMUX_PUT_DATA_TYPE_THUMB:
		{
			int r;
			BSMUXER_DATA bsmux_data = {0};
			bsmux_data.type = BSMUX_TYPE_THUMB;
			bsmux_data.bSMemAddr = p_data->phy_addr; //pa
			bsmux_data.bSSize = p_data->size;
			bsmux_data.bSVirAddr = p_data->vir_addr; //va
			r = bsmux_ctrl_in(idx, (void *) &bsmux_data);
			if (r != E_OK) {
				DBG_ERR("in fail (%d)\r\n", r);
				rv = HD_ERR_FAIL; ///< already executed and failed
			} else {
				rv = HD_OK;
			}
		}
		break;

	default:
		break;
	}
	return rv;
}

INT _hd_bsmux_trig_event(HD_BSMUX_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	INT rv = 0;
	HD_BSMUX_TRIG_EVENT *p_event = (HD_BSMUX_TRIG_EVENT *)p_param;

	switch (p_event->type) {
	case HD_BSMUX_TRIG_EVENT_CUTNOW:
		{
			int r;
			r = bsmux_ctrl_trig_cutnow(idx);
			if (r != E_OK) {
				DBG_DUMP("cutnow ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;
	case HD_BSMUX_TRIG_EVENT_EXTEND:
		{
			int r;
			r = bsmux_ctrl_extend(idx);
			if (r < E_OK) {
				DBG_DUMP("extend ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
				p_event->ret_val = r;
			}
		}
		break;
	case HD_BSMUX_TRIG_EVENT_PAUSE:
		{
			int r;
			r = bsmux_ctrl_pause(idx);
			if (r != E_OK) {
				DBG_DUMP("pause ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;
	case HD_BSMUX_TRIG_EVENT_RESUME:
		{
			int r;
			r = bsmux_ctrl_resume(idx);
			if (r != E_OK) {
				DBG_DUMP("resume ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;
	case HD_BSMUX_TRIG_EVENT_EMERGENCY:
		{
			int r;
			r = bsmux_ctrl_emergency(idx);
			if (r != E_OK) {
				DBG_DUMP("emergency ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;
	default:
		break;
	}
	return rv;
}

INT _hd_bsmux_set_event(HD_BSMUX_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	INT rv = 0;
	switch (id) {

	case HD_BSMUX_PARAM_GPS_DATA:
		{
			HD_BSMUX_GPS_DATA *p_gpsdata = (HD_BSMUX_GPS_DATA *)p_param;
			if (p_gpsdata->gpson) {
				BSMUXER_DATA bsmux_data = {0};
				int r;
				bsmux_data.type = BSMUX_TYPE_GPSIN;
				//bsmux_data.bSMemAddr = p_gpsdata->gpsdataadr;
				bsmux_data.bSVirAddr = p_gpsdata->gpsdataadr;
				bsmux_data.bSSize = p_gpsdata->gpsdatasize;
				r = bsmux_ctrl_in(idx, (void *) &bsmux_data);
				if (r != E_OK) {
					DBG_ERR("in fail (%d)\r\n", r);
					rv = HD_ERR_FAIL; ///< already executed and failed
				} else {
					rv = HD_OK;
				}
			} else {
				DBG_ERR("gps event not on, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			}
		}
		break;

	case HD_BSMUX_PARAM_TRIG_EMR:
		{
			HD_BSMUX_TRIG_EMR *p_emrdata = (HD_BSMUX_TRIG_EMR *)p_param;
			if (p_emrdata->emr_on) {
				int r;
				if (p_emrdata->pause_on) {
					r = bsmux_ctrl_trig_emr(idx, HD_GET_OUT(p_emrdata->pause_id));
				} else {
					r = bsmux_ctrl_trig_emr(idx, idx);
				}
				if (r != E_OK) {
					DBG_DUMP("trig ignored, path_id %d\r\n", (int)idx);
					rv = HD_ERR_ABORT; ///< ignored or skipped
				} else {
					rv = HD_OK;
				}
			} else {
				DBG_ERR("trig emr not on, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			}
		}
		break;

	case HD_BSMUX_PARAM_EXTEND:
		{
			int r;
			r = bsmux_ctrl_extend(idx);
			if (r != E_OK) {
				DBG_DUMP("trig ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;

	case HD_BSMUX_PARAM_PAUSE:
		{
			int r;
			r = bsmux_ctrl_pause(idx);
			if (r != E_OK) {
				DBG_DUMP("trig ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;

	case HD_BSMUX_PARAM_RESUME:
		{
			int r;
			r = bsmux_ctrl_resume(idx);
			if (r != E_OK) {
				DBG_DUMP("trig ignored, path_id %d\r\n", (int)idx);
				rv = HD_ERR_ABORT; ///< ignored or skipped
			} else {
				rv = HD_OK;
			}
		}
		break;

	default:
		break;
	}
	return rv;
}

VOID _hd_bsmux_en_util(HD_BSMUX_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	HD_BSMUX_EN_UTIL *p_util = (HD_BSMUX_EN_UTIL *)p_param;
	switch (p_util->type) {

	case HD_BSMUX_EN_UTIL_FRONTMOOV:
		{
			UINT32 moov_freq = p_util->resv[0];
			UINT32 moov_tune = p_util->resv[1];
			bsmux_util_set_param(idx, BSMUXER_PARAM_FRONT_MOOV, p_util->enable);
			if (moov_freq) { //if not set, default 5 blkcnt (internal)
				bsmux_util_set_param(idx, BSMUXER_PARAM_MOOV_FREQ, moov_freq);
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_MOOV_FREQ, 5);
			}
			if (moov_tune) {
				bsmux_util_set_param(idx, BSMUXER_PARAM_MOOV_TUNE, 1);
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_MOOV_TUNE, 0);
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_STRG_BUF:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_EN_STRGBUF, p_util->enable);
		}
		break;

	case HD_BSMUX_EN_UTIL_RECOVERY:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_NIDX_EN, p_util->enable);
		}
		break;

	case HD_BSMUX_EN_UTIL_EMERGENCY:
		{
			UINT32 emr_on   = p_util->enable;
			UINT32 pause_on = p_util->resv[0];
			UINT32 pause_id = HD_GET_OUT(p_util->resv[1]);
			if (emr_on) {
				if (pause_on) {
					// only set pause_id
					bsmux_util_set_param(idx, BSMUXER_PARAM_FILE_PAUSE_ID, pause_id);
				}
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_MUXALIGN:
		{
			if (p_util->enable) {
				if (p_util->resv[0]) {
					bsmux_util_set_param(idx, BSMUXER_PARAM_MUXALIGN, p_util->resv[0]);
				} else {
					bsmux_util_set_param(idx, BSMUXER_PARAM_MUXALIGN, 4);
				}
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_MUXALIGN, 1);
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_BUFLOCK:
		{
			if (p_util->enable) {
				if (p_util->resv[0]) {
					bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC, p_util->resv[0]);
				} else {
					bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC, 3000); //ms
				}
				if (p_util->resv[1]) {
					bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT, p_util->resv[1]);
				} else {
					bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT, 30000); //ms
				}
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC, 0);
				bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT, 0);
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_FREA_BOX:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_EN_FREABOX, p_util->enable);
		}
		break;

	case HD_BSMUX_EN_UTIL_DUR_LIMIT:
		{
			if (p_util->enable) {
				if (p_util->resv[0]) {
					bsmux_util_set_param(idx, BSMUXER_PARAM_DUR_US_MAX, p_util->resv[0]);
				} else {
					bsmux_util_set_param(idx, BSMUXER_PARAM_DUR_US_MAX, BSMUX_DUR_US_MAX);
				}
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_FAST_PUT:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_EN_FASTPUT, p_util->enable);
		}
		break;

	case HD_BSMUX_EN_UTIL_EN_DROP:
		{
			bsmux_util_set_param(idx, BSMUXER_PARAM_EN_DROP, p_util->enable);
		}
		break;

	case HD_BSMUX_EN_UTIL_GPS_DATA:
		{
			UINT32 gps_rate = p_util->resv[0];
			UINT32 gps_queue = p_util->resv[1];
			bsmux_util_set_param(idx, BSMUXER_PARAM_GPS_ON, p_util->enable);
			if (gps_rate) { //if not set, default 5 blkcnt (internal)
				bsmux_util_set_param(idx, BSMUXER_PARAM_GPS_RATE, gps_rate);
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_GPS_RATE, 1);
			}
			bsmux_util_set_param(idx, BSMUXER_PARAM_GPS_QUEUE, gps_queue);
		}
		break;

	case HD_BSMUX_EN_UTIL_UTC_AUTO:
		{
			INT32 utc_sign = 0, utc_zone = 0;
			if (p_util->enable) {
				utc_sign = 0;
				utc_zone = ATOM_USER;
			} else {
				if (p_util->resv[0] == '+') {
					utc_sign = 0;
				} else if (p_util->resv[0] == '-') {
					utc_sign = 1;
				}
				utc_zone = p_util->resv[1];
			}
			bsmux_util_set_param(idx, BSMUXER_PARAM_UTC_SIGN, utc_sign);
			bsmux_util_set_param(idx, BSMUXER_PARAM_UTC_ZONE, utc_zone);
		}
		break;

	case HD_BSMUX_EN_UTIL_UTC_TIME:
		{
			struct tm utc_time = {0};
			utc_time.tm_year = HD_BSMUX_UTC_YEAR(p_util->resv[0]);
			utc_time.tm_mon  = HD_BSMUX_UTC_MON(p_util->resv[0]);
			utc_time.tm_mday = HD_BSMUX_UTC_DAY(p_util->resv[0]);
			utc_time.tm_hour = HD_BSMUX_UTC_HOUR(p_util->resv[1]);
			utc_time.tm_min  = HD_BSMUX_UTC_MIN(p_util->resv[1]);
			utc_time.tm_sec  = HD_BSMUX_UTC_SEC(p_util->resv[1]);
			bsmux_util_set_param(idx, BSMUXER_PARAM_UTC_TIME, (UINT32)(&utc_time));
		}
		break;

	case HD_BSMUX_EN_UTIL_VAR_RATE:
		{
			UINT32 user_variable_rate = p_util->resv[0];
			UINT32 mux_variable_rate;
			bsmux_util_get_param(idx, BSMUXER_PARAM_VID_VAR_VFR, &mux_variable_rate);
			mux_variable_rate = (user_variable_rate << 16) | mux_variable_rate;
			bsmux_util_set_param(idx, BSMUXER_PARAM_VID_VAR_VFR, mux_variable_rate);
		}
		break;

	case HD_BSMUX_EN_UTIL_BTAG_SIZE:
		{
			UINT32 user_box_tag_size = p_util->resv[0];
			if ((user_box_tag_size == 32) || (user_box_tag_size == 64)) {
				bsmux_util_set_param(idx, BSMUXER_PARAM_BOXTAG_SIZE, user_box_tag_size);
			} else {
				DBG_DUMP("user_box_tag_size(%d) invalid\r\n", (int)user_box_tag_size);
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_SW_METHOD:
		{
			if (p_util->enable) {
				bsmux_util_set_param(idx, BSMUXER_PARAM_MUXMETHOD, 1);
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_MUXMETHOD, 0);
			}
		}
		break;

	case HD_BSMUX_EN_UTIL_PTS_RESET:
		{
			if (p_util->enable) {
				bsmux_util_set_param(idx, BSMUXER_PARAM_PTS_RESET, 1);
			} else {
				bsmux_util_set_param(idx, BSMUXER_PARAM_PTS_RESET, 0);
			}
		}
		break;

	default:
		break;
	}
	return ;
}

HD_RESULT _hd_bsmux_set_all_default_params_to_unit(UINT32 self_id, UINT32 out_id)
{
	UINT32 dev  = self_id - DEV_BASE;
	UINT32 port = out_id- OUT_BASE;
	BSMUX_INFO *p_bsmux_info = &bsmux_info[dev];

	//Project Setting: init values
	_hd_bsmux_set_param(HD_BSMUX_PARAM_VIDEOINFO, out_id, &p_bsmux_info->port[port].bsmux_videoinfo);
	_hd_bsmux_set_param(HD_BSMUX_PARAM_AUDIOINFO, out_id, &p_bsmux_info->port[port].bsmux_audioinfo);
	_hd_bsmux_set_param(HD_BSMUX_PARAM_FILEINFO, out_id, &p_bsmux_info->port[port].bsmux_fileinfo);
	_hd_bsmux_set_param(HD_BSMUX_PARAM_WRINFO, out_id, &p_bsmux_info->port[port].bsmux_wrinfo);
	_hd_bsmux_set_param(HD_BSMUX_PARAM_EXTINFO, out_id, &p_bsmux_info->port[port].bsmux_extinfo);
	//Utility Setting: init values of gps
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_GPS_DATA;
	p_bsmux_info->port[port].bsmux_en_util.enable = TRUE;
	p_bsmux_info->port[port].bsmux_en_util.resv[0] = 1;
	p_bsmux_info->port[port].bsmux_en_util.resv[1] = 1;
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);
	//Utility Setting: init values of recovery
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_RECOVERY;
	p_bsmux_info->port[port].bsmux_en_util.enable = TRUE;
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);
	//Utility Setting: init values of mux align 4
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_MUXALIGN;
	p_bsmux_info->port[port].bsmux_en_util.enable = TRUE;
	p_bsmux_info->port[port].bsmux_en_util.resv[0] = 4;
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);
	//Utility Setting: init values of box tag size
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_BTAG_SIZE;
	p_bsmux_info->port[port].bsmux_en_util.enable = TRUE;
	p_bsmux_info->port[port].bsmux_en_util.resv[0] = 64; //64bits
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);
	//Utility Setting: init values of buf lock
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_BUFLOCK;
	p_bsmux_info->port[port].bsmux_en_util.enable = FALSE;
	p_bsmux_info->port[port].bsmux_en_util.resv[0] = 3000; //ms
	p_bsmux_info->port[port].bsmux_en_util.resv[1] = 30000; //ms
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);
	//Utility Setting: init values of sw method
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_SW_METHOD;
	p_bsmux_info->port[port].bsmux_en_util.enable = TRUE;
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);
	//Utility Setting: init values of sw method
	p_bsmux_info->port[port].bsmux_en_util.type = HD_BSMUX_EN_UTIL_PTS_RESET;
	p_bsmux_info->port[port].bsmux_en_util.enable = FALSE;
	_hd_bsmux_en_util(HD_BSMUX_PARAM_EN_UTIL, out_id, &p_bsmux_info->port[port].bsmux_en_util);

	return HD_OK;
}

VOID _hd_bsmux_fill_param(HD_BSMUX_PARAM_ID id, VOID *p_out_param, VOID *p_in_param)
{
	switch (id) {

	case HD_BSMUX_PARAM_VIDEOINFO:
		{
			HD_BSMUX_VIDEOINFO *p_videoinfo = (HD_BSMUX_VIDEOINFO *)p_out_param;
			if (p_in_param == NULL) {
				p_videoinfo->vidcodec   = MEDIAVIDENC_H264;
				p_videoinfo->vfr        = 30;
				p_videoinfo->width      = 1920;
				p_videoinfo->height     = 1080;
				p_videoinfo->tbr        = 2*1024*1024; // 2 Mb/s
				p_videoinfo->DAR        = MEDIAREC_DAR_DEFAULT;
				p_videoinfo->gop        = 15;
			} else {
				HD_BSMUX_VIDEOINFO *p_setting = (HD_BSMUX_VIDEOINFO *)p_in_param;
				memcpy(p_videoinfo, p_setting, sizeof(HD_BSMUX_VIDEOINFO));
			}
		}
		break;

	case HD_BSMUX_PARAM_AUDIOINFO:
		{
			HD_BSMUX_AUDIOINFO *p_audioinfo = (HD_BSMUX_AUDIOINFO *)p_out_param;
			if (p_in_param == NULL) {
				p_audioinfo->codectype  = MOVAUDENC_AAC;
				p_audioinfo->chs        = 2; //STEREO;
				p_audioinfo->asr        = 32000;
				p_audioinfo->aud_en     = 1;
				p_audioinfo->adts_bytes = 7;
			} else {
				HD_BSMUX_AUDIOINFO *p_setting = (HD_BSMUX_AUDIOINFO *)p_in_param;
				memcpy(p_audioinfo, p_setting, sizeof(HD_BSMUX_AUDIOINFO));
			}
		}
		break;

	case HD_BSMUX_PARAM_FILEINFO:
		{
			HD_BSMUX_FILEINFO *p_fileinfo = (HD_BSMUX_FILEINFO *)p_out_param;
			if (p_in_param == NULL) {
				p_fileinfo->emron       = FALSE;
				p_fileinfo->emrloop     = FALSE;
				p_fileinfo->ddr_id      = DDR_ID0;
				p_fileinfo->seamlessSec = 60;
				p_fileinfo->rollbacksec = 3;
				p_fileinfo->keepsec     = 5;
				p_fileinfo->filetype    = MEDIA_FILEFORMAT_MP4;
				p_fileinfo->recformat   = MEDIAREC_AUD_VID_BOTH;
				p_fileinfo->playvfr     = 30;
				p_fileinfo->revsec      = 10;
				p_fileinfo->overlop_on  = TRUE;
				#if BSMUX_TEST_SET_MS
				p_fileinfo->seamlessSec_ms = HD_BSMUX_SET_MS(60, 0);
				p_fileinfo->rollbacksec_ms = HD_BSMUX_SET_MS(3, 0);
				p_fileinfo->keepsec_ms     = HD_BSMUX_SET_MS(5, 0);
				p_fileinfo->revsec_ms      = HD_BSMUX_SET_MS(10, 0);
				#endif
			} else {
				HD_BSMUX_FILEINFO *p_setting = (HD_BSMUX_FILEINFO *)p_in_param;
				memcpy(p_fileinfo, p_setting, sizeof(HD_BSMUX_FILEINFO));
			}
		}
		break;

	case HD_BSMUX_PARAM_WRINFO:
		{
			HD_BSMUX_WRINFO *p_wrinfo = (HD_BSMUX_WRINFO *)p_out_param;
			if (p_in_param == NULL) {
				p_wrinfo->flush_freq    = 5;
				p_wrinfo->wrblk_size    = 2*1024*1024;
			} else {
				HD_BSMUX_WRINFO *p_setting = (HD_BSMUX_WRINFO *)p_in_param;
				memcpy(p_wrinfo, p_setting, sizeof(HD_BSMUX_WRINFO));
			}
		}
		break;

	case HD_BSMUX_PARAM_EXTINFO:
		{
			HD_BSMUX_EXTINFO *p_extinfo = (HD_BSMUX_EXTINFO *)p_out_param;
			if (p_in_param == NULL) {
				p_extinfo->unit         = 15;
				p_extinfo->max_num      = 2;
				p_extinfo->enable       = FALSE;
			} else {
				HD_BSMUX_EXTINFO *p_setting = (HD_BSMUX_EXTINFO *)p_in_param;
				memcpy(p_extinfo, p_setting, sizeof(HD_BSMUX_EXTINFO));
			}
		}
		break;

	default:
		break;
	}
}

HD_RESULT _hd_bsmux_fill_all_default_params(UINT32 self_id, UINT32 out_id)
{
	UINT32 dev  = self_id - DEV_BASE;
	UINT32 port = out_id- OUT_BASE;
	BSMUX_INFO *p_bsmux_info = &bsmux_info[dev];

	//Project Setting: init values
	_hd_bsmux_fill_param(HD_BSMUX_PARAM_VIDEOINFO, &p_bsmux_info->port[port].bsmux_videoinfo, NULL);
	_hd_bsmux_fill_param(HD_BSMUX_PARAM_AUDIOINFO, &p_bsmux_info->port[port].bsmux_audioinfo, NULL);
	_hd_bsmux_fill_param(HD_BSMUX_PARAM_FILEINFO, &p_bsmux_info->port[port].bsmux_fileinfo, NULL);
	_hd_bsmux_fill_param(HD_BSMUX_PARAM_WRINFO, &p_bsmux_info->port[port].bsmux_wrinfo, NULL);
	_hd_bsmux_fill_param(HD_BSMUX_PARAM_EXTINFO, &p_bsmux_info->port[port].bsmux_extinfo, NULL);

	return HD_OK;
}

HD_RESULT _hd_bsmux_reset_all_default_params_to_unit(UINT32 self_id, UINT32 out_id)
{
	UINT32 dev  = self_id - DEV_BASE;
	UINT32 port = out_id- OUT_BASE;
	BSMUX_INFO *p_bsmux_info = &bsmux_info[dev];

	//Project Setting: init values
	_hd_bsmux_fill_param(HD_BSMUX_PARAM_EXTINFO, &p_bsmux_info->port[port].bsmux_extinfo, NULL);

	//Project Setting: init values
	_hd_bsmux_set_param(HD_BSMUX_PARAM_EXTINFO, out_id, &p_bsmux_info->port[port].bsmux_extinfo);

	return HD_OK;
}

VOID _hd_bsmux_dewrap_video_vps(BSMUXER_DATA *p_data, HD_VIDEOENC_BS *p_video_bs)
{
	p_data->type = BSMUX_TYPE_VIDEO;
	p_data->bufid = p_video_bs->blk; //isf-data-handle
	if (p_video_bs->pack_num == 4) //h265 key
	{
		if (p_video_bs->video_pack[0].pack_type.h265_type != H265_NALU_TYPE_VPS) {
			return;
		}
		p_data->bSMemAddr = p_video_bs->video_pack[0].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[0].size;
	}
}

VOID _hd_bsmux_dewrap_video_sps(BSMUXER_DATA *p_data, HD_VIDEOENC_BS *p_video_bs)
{
	p_data->type = BSMUX_TYPE_VIDEO;
	p_data->bufid = p_video_bs->blk; //isf-data-handle
	if (p_video_bs->pack_num == 4) //h265 key
	{
		if (p_video_bs->video_pack[1].pack_type.h265_type != H265_NALU_TYPE_SPS) {
			return;
		}
		p_data->bSMemAddr = p_video_bs->video_pack[1].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[1].size;
	}
	else if (p_video_bs->pack_num == 3) //h264 key
	{
		if (p_video_bs->video_pack[0].pack_type.h264_type != H264_NALU_TYPE_SPS) {
			return;
		}
		p_data->bSMemAddr = p_video_bs->video_pack[0].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[0].size;
	}
}

VOID _hd_bsmux_dewrap_video_pps(BSMUXER_DATA *p_data, HD_VIDEOENC_BS *p_video_bs)
{
	p_data->type = BSMUX_TYPE_VIDEO;
	p_data->bufid = p_video_bs->blk; //isf-data-handle
	if (p_video_bs->pack_num == 4) //h265 key
	{
		if (p_video_bs->video_pack[2].pack_type.h265_type != H265_NALU_TYPE_PPS) {
			return;
		}
		p_data->bSMemAddr = p_video_bs->video_pack[2].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[2].size;
	}
	else if (p_video_bs->pack_num == 3) //h264 key
	{
		if (p_video_bs->video_pack[1].pack_type.h264_type != H264_NALU_TYPE_PPS) {
			return;
		}
		p_data->bSMemAddr = p_video_bs->video_pack[1].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[1].size;
	}
}

VOID _hd_bsmux_dewrap_video_bs(BSMUXER_DATA *p_data, HD_VIDEOENC_BS *p_video_bs)
{
	p_data->bufid = p_video_bs->blk; //isf-data-handle
	p_data->bSTimeStamp = p_video_bs->timestamp;
	if (p_video_bs->pack_num == 4) //h265 key
	{
		p_data->bSMemAddr = p_video_bs->video_pack[3].phy_addr - (p_video_bs->video_pack[0].size + p_video_bs->video_pack[1].size + p_video_bs->video_pack[2].size); //phy addr & shift
		p_data->bSSize = p_video_bs->video_pack[3].size + (p_video_bs->video_pack[0].size + p_video_bs->video_pack[1].size + p_video_bs->video_pack[2].size); //size shift
		p_data->isKey = 1;
		p_data->naluaddr = p_video_bs->video_pack[0].phy_addr;
		p_data->nalusize = p_video_bs->video_pack[0].size + p_video_bs->video_pack[1].size + p_video_bs->video_pack[2].size;

#if BSMUX_TEST_MULTI_TILE
		p_data->naluVPSSize  = p_video_bs->video_pack[0].size;
		p_data->naluSPSSize  = p_video_bs->video_pack[1].size;
		p_data->naluPPSSize  = p_video_bs->video_pack[2].size;
#endif

	}
	else if (p_video_bs->pack_num == 3) //h264 key
	{
		p_data->bSMemAddr = p_video_bs->video_pack[2].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[2].size;
		p_data->isKey = 1;
		p_data->naluaddr = p_video_bs->video_pack[0].phy_addr;
		p_data->nalusize = p_video_bs->video_pack[0].size + p_video_bs->video_pack[1].size;

#if BSMUX_TEST_MULTI_TILE
		p_data->naluSPSSize  = p_video_bs->video_pack[0].size;
		p_data->naluPPSSize  = p_video_bs->video_pack[1].size;
#endif

	}
	else
	{
		p_data->bSMemAddr = p_video_bs->video_pack[0].phy_addr; //phy addr
		p_data->bSSize = p_video_bs->video_pack[0].size;
		p_data->isKey = 0;
		p_data->naluaddr = 0;
		p_data->nalusize = 0;
	}

#if BSMUX_TEST_MULTI_TILE
	if (p_video_bs->p_next) {
		p_data->naluOftMemAddr = (UINT32)p_video_bs->p_next;
	}
#endif

	HD_BSMUX_FUNC("phy_addr 0x%X, size 0x%X\r\n",
		(unsigned int)p_data->bSMemAddr, (unsigned int)p_data->bSSize);
}

VOID _hd_bsmux_dewrap_audio_bs(BSMUXER_DATA *p_data, HD_AUDIO_BS *p_audio_bs)
{
	p_data->bSMemAddr = p_audio_bs->phy_addr; //phy addr
	p_data->bSSize = p_audio_bs->size;
	p_data->bufid = p_audio_bs->blk;
	p_data->bSTimeStamp = p_audio_bs->timestamp;

	HD_BSMUX_FUNC("phy_addr 0x%X, size 0x%X\r\n",
		(unsigned int)p_data->bSMemAddr, (unsigned int)p_data->bSSize);
}

VOID _hd_bsmux_dewrap_meta_data(BSMUXER_DATA *p_data)
{
	UINT32 meta_data_num = 0;
	HD_BSMUX_COMM_META *meta_data = NULL;
	meta_data = (HD_BSMUX_COMM_META *)ALIGN_CEIL_64(p_data->naluOftVirAddr+sizeof(UINT32)+(p_data->naluNum*sizeof(HD_BUFINFO)));
	while (meta_data != NULL) {
		meta_data_num++;
		if (meta_data->p_next == NULL) break; //last one
		meta_data = (HD_BSMUX_COMM_META *)((UINT32)meta_data + meta_data->header_size + meta_data->buffer_size);
	}
	p_data->meta_data_num = meta_data_num;
	p_data->meta_data_mem_addr = ALIGN_CEIL_64(p_data->naluOftMemAddr+sizeof(UINT32)+(p_data->naluNum*sizeof(HD_BUFINFO)));
	p_data->meta_data_vir_addr = ALIGN_CEIL_64(p_data->naluOftVirAddr+sizeof(UINT32)+(p_data->naluNum*sizeof(HD_BUFINFO)));
	//DBG_DUMP("[DEWRAP] pa-0x%lx\r\n", (unsigned long)p_data->meta_data_mem_addr);
}

VOID _hd_bsmux_dewrap_virtual_address(UINT32 out_id, BSMUXER_DATA *p_data)
{
	if (p_data->bSMemAddr) {
		p_data->bSVirAddr = bsmux_util_get_buf_va(out_id, p_data->bSMemAddr);
	}

#if BSMUX_TEST_MULTI_TILE
	if (p_data->naluaddr) {
		p_data->naluVirAddr = bsmux_util_get_buf_va(out_id, p_data->naluaddr);
	}
	if (p_data->naluOftMemAddr) {
		p_data->naluOftVirAddr = bsmux_util_get_buf_va(out_id, p_data->naluOftMemAddr);
		if (p_data->naluOftVirAddr) {
			p_data->naluNum = *(UINT32*)p_data->naluOftVirAddr;
		}
	}
#endif

}

VOID _hd_bsmux_fill_bsmuxer_data_from_venc(UINT32 in_id, UINT32 out_id, BSMUXER_DATA *p_data, HD_VIDEOENC_BS *p_video_bs)
{
	if (in_id == out_id) { // main-stream
		if (p_video_bs->sign == MAKEFOURCC('T','H','U','M')) {
			p_data->type = BSMUX_TYPE_THUMB;
		} else { // MAKEFOURCC('V','S','T','M')
			p_data->type = BSMUX_TYPE_VIDEO;
		}
	} else { // sub-stream
		p_data->type = BSMUX_TYPE_SUBVD;
	}
	_hd_bsmux_dewrap_video_bs(p_data, p_video_bs);
	_hd_bsmux_dewrap_virtual_address(out_id, p_data);
}

VOID _hd_bsmux_fill_bsmuxer_data_from_aenc(UINT32 in_id, UINT32 out_id, BSMUXER_DATA *p_data, HD_AUDIO_BS *p_audio_bs)
{
	p_data->type = BSMUX_TYPE_AUDIO;
	_hd_bsmux_dewrap_audio_bs(p_data, p_audio_bs);
	_hd_bsmux_dewrap_virtual_address(out_id, p_data);
}

VOID _hd_bsmux_fill_bsmuxer_data_from_user(UINT32 in_id, UINT32 out_id, BSMUXER_DATA *p_data, HD_BSMUX_BS *p_user_bs)
{
	if (p_user_bs->sign == MAKEFOURCC('B','S','M','X')) {
		if (p_user_bs->data_type & HD_BSMUX_TYPE_VIDEO) {
			if (p_user_bs->p_user_bs == NULL) return;
			_hd_bsmux_fill_bsmuxer_data_from_venc(in_id, out_id, p_data, (HD_VIDEOENC_BS *)p_user_bs->p_user_bs);
		} else if (p_user_bs->data_type & HD_BSMUX_TYPE_AUDIO) {
			if (p_user_bs->p_user_bs == NULL) return;
			_hd_bsmux_fill_bsmuxer_data_from_aenc(in_id, out_id, p_data, (HD_AUDIO_BS *)p_user_bs->p_user_bs);
		} else if (p_user_bs->data_type & HD_BSMUX_TYPE_SUBVD) {
			if (p_user_bs->p_user_bs == NULL) return;
			_hd_bsmux_fill_bsmuxer_data_from_venc(in_id, out_id, p_data, (HD_VIDEOENC_BS *)p_user_bs->p_user_bs);
		}
		if (p_user_bs->data_type & HD_BSMUX_TYPE_USRCB) {
			if (p_user_bs->p_user_data == NULL) return;
			p_data->user_data = p_user_bs->p_user_data;
		}
		if (p_user_bs->data_type & HD_BSMUX_TYPE_META) {
			_hd_bsmux_dewrap_meta_data(p_data);
		}
	} else {
		DBG_ERR("invalid sign(0x%x)\r\n", (int)p_user_bs->sign);
	}
}

VOID _hd_bsmux_fill_bsmuxer_data_from_meta(UINT32 in_id, UINT32 out_id, BSMUXER_DATA *p_meta, BSMUXER_DATA *p_data)
{
	HD_BSMUX_COMM_META *meta_data = NULL;
	UINT32 meta_pa = p_data->meta_data_mem_addr;
	UINT32 meta_va = p_data->meta_data_vir_addr;
	UINT32 meta_header_size, meta_buffer_size;

	meta_data = (HD_BSMUX_COMM_META *)meta_va;
	meta_header_size = meta_data->header_size;
	meta_buffer_size = meta_data->buffer_size;

	p_meta->type = BSMUX_TYPE_MTAIN;
	p_meta->bSMemAddr = meta_pa + meta_header_size;
	p_meta->bSSize = meta_buffer_size;
	p_meta->bSVirAddr = meta_va + meta_header_size;

	p_data->meta_data_mem_addr += meta_header_size + meta_buffer_size;
	p_data->meta_data_vir_addr += meta_header_size + meta_buffer_size;
}

VOID _hd_bsmux_test_esdata(UINT32 out_id, HD_VIDEOENC_BS *p_video_bs)
{
	BSMUXER_DATA bsmux_vps = {0};
	BSMUXER_DATA bsmux_sps = {0};
	BSMUXER_DATA bsmux_pps = {0};

	_hd_bsmux_dewrap_video_vps(&bsmux_vps, p_video_bs);
	if (bsmux_vps.bSSize != 0) {
		bsmux_ctrl_in(out_id, (void *) &bsmux_vps);
	}
	_hd_bsmux_dewrap_video_sps(&bsmux_sps, p_video_bs);
	if (bsmux_sps.bSSize != 0) {
		bsmux_ctrl_in(out_id, (void *) &bsmux_sps);
	}
	_hd_bsmux_dewrap_video_pps(&bsmux_pps, p_video_bs);
	if (bsmux_pps.bSSize != 0) {
		bsmux_ctrl_in(out_id, (void *) &bsmux_pps);
	}
}

VOID _hd_bsmux_test_thumbnail(UINT32 out_id, HD_VIDEOENC_BS *p_video_bs)
{
	BSMUXER_DATA bsmux_data = {0};
	static UINT32 video_count = 0;
	if (p_video_bs->sign == MAKEFOURCC('V','S','T','M')) {
		if (video_count == 0) {
			HD_BSMUX_DUMP("video_count = %d, push thumb\r\n", video_count);
			p_video_bs->sign = MAKEFOURCC('T','H','U','M');
			_hd_bsmux_dewrap_video_bs(&bsmux_data, p_video_bs);
			bsmux_data.bSSize = 0x8000;
			bsmux_ctrl_in(out_id, (void *) &bsmux_data);
			p_video_bs->sign = MAKEFOURCC('V','S','T','M');
		}
		video_count++;
		if (video_count == 1800) { // vfr * sec
			video_count = 0;
		}
	}
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_bsmux_init(VOID)
{
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	//notice: max path num 16 / max active port num 4
	_hd_bsmux_cfg_max(HD_MAX_PATH_NUM);

	if (bsmux_info[0].port != NULL) {
		DBG_ERR("already init or not uninit\n");
		rv = HD_ERR_INIT; ///< module is already initialized
		goto l_hd_bsmux_init_end;
	}

	if (_max_path_count == 0) {
		DBG_ERR("check _max_path_count =0?\r\n");
		rv = HD_ERR_NO_CONFIG; ///< module device config is not set
		goto l_hd_bsmux_init_end;
	}

	HD_BSMUX_FUNC("malloc\r\n");
	{
		UINT32 i;
		bsmux_info[0].port = (BSMUX_INFO_PORT*)malloc(sizeof(BSMUX_INFO_PORT)*_max_path_count);
		if (bsmux_info[0].port == NULL) {
			DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
			rv = HD_ERR_HEAP; ///< heap full
			goto l_hd_bsmux_init_end;
		}
		for(i = 0; i < _max_path_count; i++) {
			memset(&(bsmux_info[0].port[i]), 0, sizeof(BSMUX_INFO_PORT));
		}
	}

	HD_BSMUX_FUNC("action\r\n");
	{
		int r;
		r = bsmux_ctrl_init();
		if (r != 0) {
			DBG_ERR("init fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [UNINIT] -> [INIT]
	{
		UINT32 dev, port;
		for (dev = 0; dev < DEV_COUNT; dev++) {
			for (port = 0; port < _max_path_count; port++) {
				// init default params
				_hd_bsmux_fill_all_default_params(dev + DEV_BASE, port + OUT_BASE);

				// set status
				bsmux_info[dev].port[port].priv.status = HD_BSMUX_STAT_INIT;
			}
		}
	}

l_hd_bsmux_init_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	// check input
	//_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) { return rv;}
	//_HD_CONVERT_SELF_ID(in_dev_id, rv); 	if(rv != HD_OK) { return rv;}
	if (in_dev_id != self_id) {
		DBG_ERR("invalid device id\r\n");
		rv = HD_ERR_DEV; ///< invalid device id
		goto l_hd_bsmux_open_end;
	}

	// set path id
	if(!p_path_id) {
		DBG_ERR("p_path_id is null\r\n");
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_bsmux_open_end;
	}
	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		p_path_id[0] = _out_id;
		//do nothing
		return HD_OK;
	} else {
		p_path_id[0] = HD_BSMUX_PATH(self_id, in_id, out_id);
	}

	if (in_id != out_id) {
		//do nothing
		return HD_OK;
	}

	// check status: [INIT]
	if (0 == _hd_bsmux_check_status(p_path_id[0], HD_BSMUX_STAT_UNINIT)) {
		DBG_ERR("open could not be called, please call hd_bsmux_init() first\r\n");
		rv = HD_ERR_UNINIT; ///< module is not initialised yet
		goto l_hd_bsmux_open_end;
	}
	if (0 == _hd_bsmux_check_status(p_path_id[0], HD_BSMUX_STAT_IDLE)) {
		DBG_ERR("open could not be called, bsmux already opened\r\n");
		rv = HD_ERR_ALREADY_OPEN; ///< path is already open
		goto l_hd_bsmux_open_end;
	}
	if (0 == _hd_bsmux_check_status(p_path_id[0], HD_BSMUX_STAT_RUN)) {
		DBG_ERR("open could not be called, bsmux already start\r\n");
		rv = HD_ERR_ALREADY_START; ///< path is already start
		goto l_hd_bsmux_open_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
			(int)p_path_id[0], (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		UINT32 idx = (UINT32)out_id;
		r = bsmux_ctrl_open(idx);
		if (r != 0) {
			DBG_ERR("open fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [INIT] -> [IDLE]
	if (rv == HD_OK) {
		// set default params to unit
		_hd_bsmux_set_all_default_params_to_unit(self_id, out_id);
		if (0 != _hd_bsmux_set_status(p_path_id[0], HD_BSMUX_STAT_IDLE)) {
			DBG_ERR("bsmux set status IDLE fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_bsmux_open_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (in_id != out_id) {
		rv = HD_OK;
		goto l_hd_bsmux_start_end;
	}

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_start_end;
	}

	if (0 != _hd_bsmux_check_params(path_id)) {
		DBG_ERR("check params error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_INV; ///< invalid argument passed
		goto l_hd_bsmux_start_end;
	}

	// check status: [IDLE]
	if (0 == _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_UNINIT)) {
		DBG_ERR("could not be called, please call hd_bsmux_init() first\r\n");
		rv = HD_ERR_UNINIT; ///< module is not initialised yet
		goto l_hd_bsmux_start_end;
	}
	if (0 == _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_INIT)) {
		DBG_ERR("could not be called, please call hd_bsmux_open() first\r\n");
		rv = HD_ERR_NOT_OPEN; ///< path is not open yet
		goto l_hd_bsmux_start_end;
	}
	if (0 == _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_RUN)) {
		DBG_ERR("could not be called, path is already start\r\n");
		rv = HD_ERR_ALREADY_START; ///< path is already start
		goto l_hd_bsmux_start_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		UINT32 idx = (UINT32)out_id;
		r = bsmux_ctrl_start(idx);
		if (r != 0) {
			DBG_ERR("start fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [IDLE] -> [RUN]
	if (rv == HD_OK) {
		if (0 != _hd_bsmux_set_status(path_id, HD_BSMUX_STAT_RUN)) {
			DBG_ERR("bsmux set status RUN fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_bsmux_start_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (in_id != out_id) {
		rv = HD_OK;
		goto l_hd_bsmux_stop_end;
	}

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_stop_end;
	}

	// check status: [RUN]
	if (0 != _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_RUN)) {
		DBG_ERR("could not be called, bsmux is not start\r\n");
		rv = HD_ERR_NOT_START; ///< path is not start yet
		goto l_hd_bsmux_stop_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		UINT32 idx = (UINT32)out_id;
		r = bsmux_ctrl_stop(idx);
		if (r != 0) {
			DBG_ERR("stop fail(%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [RUN] -> [IDLE]
	if (rv == HD_OK) {
		if (0 != _hd_bsmux_set_status(path_id, HD_BSMUX_STAT_IDLE)) {
			DBG_ERR("bsmux set status IDLE fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_bsmux_stop_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (in_id != out_id) {
		rv = HD_OK;
		goto l_hd_bsmux_close_end;
	}

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_close_end;
	}

	if (ctrl_id == HD_CTRL) {
		//do nothing
		return HD_OK;
	}

	// check status: [IDLE]
	if (0 != _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_IDLE)) {
		DBG_ERR("close could not be called, bsmux is not idle\r\n");
		rv = HD_ERR_NOT_OPEN; ///< path is not open yet
		goto l_hd_bsmux_close_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		UINT32 idx = (UINT32)out_id;
		r = bsmux_ctrl_close(idx);
		if (r != 0) {
			DBG_ERR("close fail (%d)\r\n", r);
			rv = HD_ERR_FAIL;
		} else {
			rv = HD_OK;
		}
	}

	// set status: [IDLE] -> [INIT]
	if (rv == HD_OK) {
		// set default params to unit
		_hd_bsmux_reset_all_default_params_to_unit(self_id, out_id);
		if (0 != _hd_bsmux_set_status(path_id, HD_BSMUX_STAT_INIT)) {
			DBG_ERR("bsmux set status INIT fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_bsmux_close_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_get(HD_PATH_ID path_id, HD_BSMUX_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_get_end;
	}

	if (NULL == p_param) {
		DBG_ERR("check p_param is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_bsmux_get_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	if (ctrl_id == HD_CTRL) {
		switch(id) {
		// Project Setting
		case HD_BSMUX_PARAM_EXTINFO:
			{
				HD_BSMUX_EXTINFO *p_user = (HD_BSMUX_EXTINFO *)p_param;
				UINT32 i;
				for (i = 0; i < HD_MAX_PATH_NUM; i++) {
					HD_BSMUX_EXTINFO *p_extinfo = &bsmux_info[self_id-DEV_BASE].port[i-OUT_BASE].bsmux_extinfo;
					_hd_bsmux_get_param(id, i, p_extinfo);
					if (p_extinfo->enable) {
						memcpy(p_user, p_extinfo, sizeof(HD_BSMUX_EXTINFO));
					}
				}
				rv = HD_OK;
			}
			break;
		// Utility
		case HD_BSMUX_PARAM_CALC_SEC:
			{
				HD_BSMUX_CALC_SEC *p_user = (HD_BSMUX_CALC_SEC *)p_param;
				UINT32 calc_sec = 0;
				calc_sec = bsmux_util_calc_sec(p_user);
				p_user->calc_sec = calc_sec;
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	} else {
		switch (id) {
		// Project Setting
		case HD_BSMUX_PARAM_VIDEOINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_VIDEOINFO *p_user = (HD_BSMUX_VIDEOINFO *)p_param;
					HD_BSMUX_VIDEOINFO *p_videoinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_videoinfo;
					_hd_bsmux_get_param(id, out_id, p_videoinfo);
					memcpy(p_user, p_videoinfo, sizeof(HD_BSMUX_VIDEOINFO));
					rv = HD_OK;
				} else {
					HD_BSMUX_VIDEOINFO *p_user = (HD_BSMUX_VIDEOINFO *)p_param;
					HD_BSMUX_VIDEOINFO *p_videoinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_videoinfo;
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &p_videoinfo->vidcodec);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_VFR, &p_videoinfo->vfr);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_WIDTH, &p_videoinfo->width);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_HEIGHT, &p_videoinfo->height);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_TBR, &p_videoinfo->tbr);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_DAR, &p_videoinfo->DAR);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_VID_SUB_GOP, &p_videoinfo->gop);
					memcpy(p_user, p_videoinfo, sizeof(HD_BSMUX_VIDEOINFO));
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_AUDIOINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_AUDIOINFO *p_user = (HD_BSMUX_AUDIOINFO *)p_param;
					HD_BSMUX_AUDIOINFO *p_audioinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_audioinfo;
					_hd_bsmux_get_param(id, out_id, p_audioinfo);
					memcpy(p_user, p_audioinfo, sizeof(HD_BSMUX_AUDIOINFO));
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_FILEINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_FILEINFO *p_user = (HD_BSMUX_FILEINFO *)p_param;
					HD_BSMUX_FILEINFO *p_fileinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_fileinfo;
					_hd_bsmux_get_param(id, out_id, p_fileinfo);
					memcpy(p_user, p_fileinfo, sizeof(HD_BSMUX_FILEINFO));
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_BUFINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_BUFINFO *p_user = (HD_BSMUX_BUFINFO *)p_param;
					HD_BSMUX_BUFINFO *p_bufinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_bufinfo;
					memcpy(p_user, p_bufinfo, sizeof(HD_BSMUX_BUFINFO));
					rv = HD_OK;
				} else {
					HD_BSMUX_BUFINFO *p_user = (HD_BSMUX_BUFINFO *)p_param;
					bsmux_util_get_param(out_id, BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR, &p_user->videnc.phy_addr);
					bsmux_util_get_param(out_id, BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE, &p_user->videnc.buf_size);
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_WRINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_WRINFO *p_user = (HD_BSMUX_WRINFO *)p_param;
					HD_BSMUX_WRINFO *p_wrinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_wrinfo;
					_hd_bsmux_get_param(id, out_id, p_wrinfo);
					memcpy(p_user, p_wrinfo, sizeof(HD_BSMUX_WRINFO));
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_EXTINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_EXTINFO *p_user = (HD_BSMUX_EXTINFO *)p_param;
					HD_BSMUX_EXTINFO *p_extinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_extinfo;
					_hd_bsmux_get_param(id, out_id, p_extinfo);
					memcpy(p_user, p_extinfo, sizeof(HD_BSMUX_EXTINFO));
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		// Utility
		case HD_BSMUX_PARAM_EN_UTIL:
			{
				if (in_id == out_id) {
					HD_BSMUX_EN_UTIL *p_user = (HD_BSMUX_EN_UTIL *)p_param;
					HD_BSMUX_EN_UTIL *p_utilinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_en_util;
					memcpy(p_utilinfo, p_user, sizeof(HD_BSMUX_EN_UTIL));
					_hd_bsmux_get_param(id, out_id, p_utilinfo);
					memcpy(p_user, p_utilinfo, sizeof(HD_BSMUX_EN_UTIL));
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_CALC_SEC:
			{
				HD_BSMUX_CALC_SEC *p_user = (HD_BSMUX_CALC_SEC *)p_param;
				UINT32 calc_sec = 0;
				calc_sec = bsmux_util_calc_sec(p_user);
				p_user->calc_sec = calc_sec;
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	}

l_hd_bsmux_get_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_set(HD_PATH_ID path_id, HD_BSMUX_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_set_end;
	}

	if (NULL == p_param) {
		DBG_ERR("check p_param is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_bsmux_set_end;
	}

	if (0 != _hd_bsmux_check_params_range(id, p_param, NULL, 0)) {
		DBG_ERR("check p_param value range error, path_id %d\n", (int)path_id);
		rv = HD_ERR_INV; ///< invalid argument passed
		goto l_hd_bsmux_set_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	if (ctrl_id == HD_CTRL) {
		switch(id) {
		// Callback
		case HD_BSMUX_PARAM_REG_CALLBACK:
			{
				HD_BSMUX_FUNC("[CTRL%d] callback 0x%X\r\n", ctrl_id, (unsigned int)p_param);
				bsmux_cb_register((HD_BSMUX_CALLBACK)p_param);
				rv = HD_OK;
			}
			break;
		// Event
		case HD_BSMUX_PARAM_TRIG_EVENT:
			{
				HD_BSMUX_TRIG_EVENT *p_setting = (HD_BSMUX_TRIG_EVENT *)p_param;
				HD_BSMUX_FUNC("[CTRL%d] event 0x%X\r\n", ctrl_id, (unsigned int)p_setting->type);
				rv = _hd_bsmux_trig_event(id, BSMUX_CTRL_IDX, (VOID *)p_setting);
			}
			break;
		// Utility
		case HD_BSMUX_PARAM_EN_UTIL:
			{
				HD_BSMUX_EN_UTIL *p_setting = (HD_BSMUX_EN_UTIL *)p_param;
				UINT32 i;
				HD_BSMUX_FUNC("[CTRL%d] utility 0x%X\r\n", ctrl_id, (unsigned int)p_setting->type);
				for (i = 0; i < HD_MAX_PATH_NUM; i++) {
					_hd_bsmux_en_util(id, i, p_setting);
				}
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	} else {
		switch(id) {
		// Callback
		case HD_BSMUX_PARAM_REG_CALLBACK:
			{
				if (in_id == out_id) {
					HD_BSMUX_FUNC("callback 0x%X\r\n", (unsigned int)p_param);
					bsmux_cb_register((HD_BSMUX_CALLBACK)p_param);
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		// Project Setting
		case HD_BSMUX_PARAM_VIDEOINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_VIDEOINFO *p_setting = (HD_BSMUX_VIDEOINFO *)p_param;
					HD_BSMUX_VIDEOINFO *p_videoinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_videoinfo;
					memcpy(p_videoinfo, p_setting, sizeof(HD_BSMUX_VIDEOINFO));
					_hd_bsmux_set_param(id, out_id, p_videoinfo);
					rv = HD_OK;
				} else { // sub-stream
					HD_BSMUX_VIDEOINFO *p_setting = (HD_BSMUX_VIDEOINFO *)p_param;
					HD_BSMUX_VIDEOINFO *p_videoinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_sub_videoinfo;
					memcpy(p_videoinfo, p_setting, sizeof(HD_BSMUX_VIDEOINFO));
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_CODECTYPE, p_videoinfo->vidcodec);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_VFR, p_videoinfo->vfr);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_WIDTH, p_videoinfo->width);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_HEIGHT, p_videoinfo->height);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_TBR, p_videoinfo->tbr);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_DAR, p_videoinfo->DAR);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_VID_SUB_GOP, p_videoinfo->gop);
					HD_BSMUX_DUMP("BSMUX SUB VIDEOINFO: codec %d vfr %d wid %d hei %d tbr %d DAR %d gop %d\r\n",
						p_videoinfo->vidcodec, p_videoinfo->vfr, p_videoinfo->width,
						p_videoinfo->height, p_videoinfo->tbr, p_videoinfo->DAR, p_videoinfo->gop);
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_AUDIOINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_AUDIOINFO *p_setting = (HD_BSMUX_AUDIOINFO *)p_param;
					HD_BSMUX_AUDIOINFO *p_audioinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_audioinfo;
					memcpy(p_audioinfo, p_setting, sizeof(HD_BSMUX_AUDIOINFO));
					_hd_bsmux_set_param(id, out_id, p_audioinfo);
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_FILEINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_FILEINFO *p_setting = (HD_BSMUX_FILEINFO *)p_param;
					HD_BSMUX_FILEINFO *p_fileinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_fileinfo;
					memcpy(p_fileinfo, p_setting, sizeof(HD_BSMUX_FILEINFO));
					_hd_bsmux_set_param(id, out_id, p_fileinfo);
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_BUFINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_BUFINFO *p_setting = (HD_BSMUX_BUFINFO *)p_param;
					HD_BSMUX_BUFINFO *p_bufinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_bufinfo;
					memcpy(p_bufinfo, p_setting, sizeof(HD_BSMUX_BUFINFO));
					_hd_bsmux_set_param(id, out_id, p_bufinfo);
					rv = HD_OK;
				} else { // sub-stream
					HD_BSMUX_BUFINFO *p_setting = (HD_BSMUX_BUFINFO *)p_param;
					if (p_setting->videnc.buf_size) {
						bsmux_util_set_param(out_id, BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR, p_setting->videnc.phy_addr);
						bsmux_util_set_param(out_id, BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE, p_setting->videnc.buf_size);
					}
					HD_BSMUX_DUMP("BSMUX SUB BUFINFO: videnc phy_addr 0x%X buf_size 0x%X\r\n",
						p_setting->videnc.phy_addr, p_setting->videnc.buf_size);
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_WRINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_WRINFO *p_setting = (HD_BSMUX_WRINFO *)p_param;
					HD_BSMUX_WRINFO *p_wrinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_wrinfo;
					memcpy(p_wrinfo, p_setting, sizeof(HD_BSMUX_WRINFO));
					_hd_bsmux_set_param(id, out_id, p_wrinfo);
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_EXTINFO:
			{
				if (in_id == out_id) {
					HD_BSMUX_EXTINFO *p_setting = (HD_BSMUX_EXTINFO *)p_param;
					HD_BSMUX_EXTINFO *p_extinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_extinfo;
					memcpy(p_extinfo, p_setting, sizeof(HD_BSMUX_EXTINFO));
					_hd_bsmux_set_param(id, out_id, p_extinfo);
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		// Data
		case HD_BSMUX_PARAM_GPS_DATA:
			{
				HD_BSMUX_GPS_DATA *p_event = (HD_BSMUX_GPS_DATA *)p_param;
				HD_BSMUX_GPS_DATA *p_gpsdata = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_gps_data;
				memcpy(p_gpsdata, p_event, sizeof(HD_BSMUX_GPS_DATA));
				rv = _hd_bsmux_set_event(id, out_id, (VOID *)p_gpsdata);
			}
			break;
		case HD_BSMUX_PARAM_USER_DATA:
			{
				HD_BSMUX_USER_DATA *p_event = (HD_BSMUX_USER_DATA *)p_param;
				HD_BSMUX_USER_DATA *p_userdata = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_user_data;
				memcpy(p_userdata, p_event, sizeof(HD_BSMUX_USER_DATA));
				_hd_bsmux_set_param(id, out_id, p_userdata);
				rv = HD_OK;
			}
			break;
		case HD_BSMUX_PARAM_CUST_DATA:
			{
				HD_BSMUX_CUST_DATA *p_event = (HD_BSMUX_CUST_DATA *)p_param;
				HD_BSMUX_CUST_DATA *p_custdata = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_cust_data;
				memcpy(p_custdata, p_event, sizeof(HD_BSMUX_CUST_DATA));
				_hd_bsmux_set_param(id, out_id, p_custdata);
				rv = HD_OK;
			}
			break;
		case HD_BSMUX_PARAM_PUT_DATA:
			{
				HD_BSMUX_PUT_DATA *p_setting = (HD_BSMUX_PUT_DATA *)p_param;
				HD_BSMUX_PUT_DATA *p_data = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_put_data;
				memcpy(p_data, p_setting, sizeof(HD_BSMUX_PUT_DATA));
				rv = _hd_bsmux_put_data(id, out_id, (VOID *)p_data);
			}
			break;
		// Event
		case HD_BSMUX_PARAM_TRIG_EMR:
			{
				HD_BSMUX_FILEINFO *p_fileinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_fileinfo;
				HD_BSMUX_TRIG_EMR *p_event = (HD_BSMUX_TRIG_EMR *)p_param;
				HD_BSMUX_TRIG_EMR *p_emrdata = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_trig_emr;
				memcpy(p_emrdata, p_event, sizeof(HD_BSMUX_TRIG_EMR));
				if (p_fileinfo->emron) {
					rv = _hd_bsmux_set_event(id, out_id, (VOID *)p_emrdata);
				} else {
					DBG_ERR("emr not on, path_id %d\r\n", (int)path_id);
					rv = HD_ERR_ABORT; ///< ignored or skipped
				}
			}
			break;
		case HD_BSMUX_PARAM_CUT_NOW:
			{
				int r;
				r = bsmux_ctrl_trig_cutnow(out_id);
				if (r != 0) {
					DBG_ERR("cut now fail, path_id %d\r\n", (int)path_id);
					rv = HD_ERR_ABORT; ///< ignored or skipped
				} else {
					rv = HD_OK;
				}
			}
			break;
		case HD_BSMUX_PARAM_EXTEND:
			{
				HD_BSMUX_EXTINFO *p_extinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_extinfo;
				if (p_extinfo->enable) {
					rv = _hd_bsmux_set_event(id, out_id, p_param);
				} else {
					DBG_ERR("ext not on, path_id %d\r\n", (int)path_id);
					rv = HD_ERR_ABORT; ///< ignored or skipped
				}
			}
			break;
		case HD_BSMUX_PARAM_PAUSE:
			{
				rv = _hd_bsmux_set_event(id, out_id, p_param);
			}
			break;
		case HD_BSMUX_PARAM_RESUME:
			{
				rv = _hd_bsmux_set_event(id, out_id, p_param);
			}
			break;
		case HD_BSMUX_PARAM_TRIG_EVENT:
			{
				HD_BSMUX_TRIG_EVENT *p_setting = (HD_BSMUX_TRIG_EVENT *)p_param;
				HD_BSMUX_TRIG_EVENT *p_event = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_trig_event;
				memcpy(p_event, p_setting, sizeof(HD_BSMUX_TRIG_EVENT));
				rv = _hd_bsmux_trig_event(id, out_id, (VOID *)p_event);
			}
			break;
		case HD_BSMUX_PARAM_EN_UTIL:
			{
				if (in_id == out_id) {
					HD_BSMUX_EN_UTIL *p_setting = (HD_BSMUX_EN_UTIL *)p_param;
					HD_BSMUX_EN_UTIL *p_utilinfo = &bsmux_info[self_id-DEV_BASE].port[out_id-OUT_BASE].bsmux_en_util;
					memcpy(p_utilinfo, p_setting, sizeof(HD_BSMUX_EN_UTIL));
					_hd_bsmux_en_util(id, out_id, p_utilinfo);
					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		// MetaData
		case HD_BSMUX_PARAM_META_ALLOC:
			{
				if (in_id == out_id) {
					HD_BSMUX_META_ALLOC *p_user = (HD_BSMUX_META_ALLOC *)p_param;
					UINT32 meta_idx, meta_num = 0;
					while (p_user != NULL) {
						if (meta_num == HD_BSMUX_META_NUM) {
							DBG_WRN("meta_alloc_max=%d\r\n", HD_BSMUX_META_NUM);
							break;
						}
						meta_num++;
						p_user = p_user->p_next;
					}
					bsmux_util_set_param(out_id, BSMUXER_PARAM_META_ON, TRUE);
					bsmux_util_set_param(out_id, BSMUXER_PARAM_META_NUM, meta_num);

					p_user = (HD_BSMUX_META_ALLOC *)p_param;
					for (meta_idx = 0; meta_idx < meta_num; meta_idx++) {
						BSMUX_REC_METADATA meta = {0};
						meta.meta_sign = p_user->sign;
						meta.meta_rate = p_user->data_rate;
						meta.meta_queue = 1;
						meta.meta_index = meta_idx;
						bsmux_util_set_param(out_id, BSMUXER_PARAM_META_DATA, (UINT32)(&meta));
						p_user = p_user->p_next;
					}

					rv = HD_OK;
				} else {
					rv = HD_OK;
				}
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	}

l_hd_bsmux_set_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_push_in_buf_video(HD_PATH_ID path_id, HD_VIDEOENC_BS * p_user_in_video_bs, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_push_in_buf_video_end;
	}

	if (NULL == p_user_in_video_bs) {
		DBG_ERR("check p_user_in_video_bs is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_bsmux_push_in_buf_video_end;
	}

	// check status: [RUN]
	if (0 != _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_RUN)) {
		DBG_ERR("push_in_buf_video could only be called after START, please call hd_bsmux_start() first\r\n");
		rv = HD_ERR_NOT_START; ///< path is not start yet
		goto l_hd_bsmux_push_in_buf_video_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	HD_BSMUX_DUMP("p_user_in_video_bs 0x%X\r\n", (unsigned int)p_user_in_video_bs);
	{
		BSMUXER_DATA bsmux_data = {0};
		UINT32 idx =  (UINT32) out_id;
		int r;

		#if 0
		_hd_bsmux_test_esdata(idx, p_user_in_video_bs);
		#endif

		#if 0
		_hd_bsmux_test_thumbnail(idx, p_user_in_video_bs);
		#endif

		_hd_bsmux_fill_bsmuxer_data_from_venc(in_id, out_id, &bsmux_data, p_user_in_video_bs);

		r = bsmux_ctrl_in(idx, (void *) &bsmux_data);
		if (r != E_OK) {
			DBG_ERR("in fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

l_hd_bsmux_push_in_buf_video_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_push_in_buf_audio(HD_PATH_ID path_id, HD_AUDIO_BS * p_user_in_audio_bs, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (in_id != out_id) {
		rv = HD_OK;
		goto l_hd_bsmux_push_in_buf_audio_end;
	}

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_push_in_buf_audio_end;
	}

	if (NULL == p_user_in_audio_bs) {
		DBG_ERR("check p_user_in_audio_bs is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_bsmux_push_in_buf_audio_end;
	}

	// check status: [RUN]
	if (0 != _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_RUN)) {
		DBG_ERR("could not be called, please call hd_bsmux_start() first\r\n");
		rv = HD_ERR_NOT_START; ///< path is not start yet
		goto l_hd_bsmux_push_in_buf_audio_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	HD_BSMUX_DUMP("p_user_in_audio_bs 0x%X\r\n", (unsigned int)p_user_in_audio_bs);
	{
		BSMUXER_DATA bsmux_data = {0};
		UINT32 idx =  (UINT32) out_id;
		int r;

		_hd_bsmux_fill_bsmuxer_data_from_aenc(in_id, out_id, &bsmux_data, p_user_in_audio_bs);

		r = bsmux_ctrl_in(idx, (void *) &bsmux_data);
		if (r != E_OK) {
			DBG_ERR("in fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

l_hd_bsmux_push_in_buf_audio_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_push_in_buf_struct(HD_PATH_ID path_id, HD_BSMUX_BS * p_user_in_bsmux_bs, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (in_id != out_id) {
		rv = HD_OK;
		goto l_hd_bsmux_push_in_buf_struct_end;
	}

	if (0 != _hd_bsmux_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_bsmux_push_in_buf_struct_end;
	}

	if (NULL == p_user_in_bsmux_bs) {
		DBG_ERR("check p_user_in_bsmux_bs is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_bsmux_push_in_buf_struct_end;
	}

	// check status: [RUN]
	if (0 != _hd_bsmux_check_status(path_id, HD_BSMUX_STAT_RUN)) {
		DBG_ERR("could not be called, please call hd_bsmux_start() first\r\n");
		rv = HD_ERR_NOT_START; ///< path is not start yet
		goto l_hd_bsmux_push_in_buf_struct_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	HD_BSMUX_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	HD_BSMUX_DUMP("p_user_in_bsmux_bs 0x%X\r\n", (unsigned int)p_user_in_bsmux_bs);
	{
		BSMUXER_DATA bsmux_data = {0};
		UINT32 idx =  (UINT32) out_id;
		int r;

		_hd_bsmux_fill_bsmuxer_data_from_user(in_id, out_id, &bsmux_data, p_user_in_bsmux_bs);

		r = bsmux_ctrl_in(idx, (void *) &bsmux_data);
		if (r != E_OK) {
			DBG_ERR("in fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}

		if (bsmux_data.meta_data_num == 0) {
			goto l_hd_bsmux_push_in_buf_struct_end;
		} else {
			BSMUXER_DATA bsmux_meta_data = {0};
			UINT32 meta_idx, meta_num = bsmux_data.meta_data_num;
			for (meta_idx = 0; meta_idx < meta_num; meta_idx++) {

				_hd_bsmux_fill_bsmuxer_data_from_meta(in_id, out_id, &bsmux_meta_data, &bsmux_data);
				bsmux_meta_data.bufid = meta_idx;

				r = bsmux_ctrl_in(idx, (void *) &bsmux_meta_data);
				if (r != E_OK) {
					DBG_ERR("meta in fail (%d)\r\n", r);
					rv = HD_ERR_FAIL; ///< already executed and failed
				} else {
					rv = HD_OK;
				}
			}
		}
	}

l_hd_bsmux_push_in_buf_struct_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_bsmux_uninit(VOID)
{
	HD_RESULT rv;

	HD_BSMUX_FUNC("begin\r\n");

	if (bsmux_info[0].port == NULL) {
		DBG_ERR("already uninit or not init\n");
		rv = HD_ERR_UNINIT; ///< module is not initialized yet
		goto l_hd_bsmux_uninit_end;
	}

	HD_BSMUX_FUNC("action\r\n");
	{
		int r;
		r = bsmux_ctrl_uninit();
		if (r != 0) {
			DBG_ERR("uninit fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [INIT] -> [UNINIT]
	{
		UINT32 dev, port;
		for (dev = 0; dev < DEV_COUNT; dev++) {
			for (port = 0; port < _max_path_count; port++) {
				bsmux_info[dev].port[port].priv.status = HD_BSMUX_STAT_UNINIT;
			}
		}
	}

	free(bsmux_info[0].port);
	bsmux_info[0].port = NULL;

l_hd_bsmux_uninit_end:
	HD_BSMUX_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

