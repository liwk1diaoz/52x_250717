/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_videoenc_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

// TODO: temp for [520] build
#ifdef _BSP_NA51000_
#undef _BSP_NA51000_
#endif
#ifndef _BSP_NA51055_
#define _BSP_NA51055_
#endif

#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_videoenc.h"
#include "video_encode.h"
#define HD_MODULE_NAME HD_VIDEOENC
#include "hd_int.h"
#include <string.h>
#include "kflow_videoenc/isf_vdoenc.h"

#define DEV_BASE        ISF_UNIT_VDOENC
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        16
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       16

extern HD_RESULT _hd_videoenc_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_videoenc_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_videoenc_get_state(HD_OUT_ID _out_id, UINT32* p_state);
extern HD_RESULT _hd_videoenc_kflow_param_get_by_vendor(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param, UINT32 *param);

INT _hd_videoenc_menu_cvt_codec(HD_VIDEO_CODEC codec, CHAR *p_ret_string, INT max_str_len)
{
	switch (codec) {
		case HD_CODEC_TYPE_JPEG:  snprintf(p_ret_string, max_str_len, "JPEG");  break;
		case HD_CODEC_TYPE_H264:  snprintf(p_ret_string, max_str_len, "H264");  break;
		case HD_CODEC_TYPE_H265:  snprintf(p_ret_string, max_str_len, "H265");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown codec(%d)\r\n", (int)(codec));
			return (-1);
	}
	return 0;
}

INT _hd_videoenc_menu_cvt_profile(HD_VIDEOENC_PROFILE  profile, CHAR *p_ret_string, INT max_str_len)
{
	switch (profile) {
		case HD_H264E_BASELINE_PROFILE:  snprintf(p_ret_string, max_str_len, "BASE");  break;
		case HD_H264E_MAIN_PROFILE:
		case HD_H265E_MAIN_PROFILE:      snprintf(p_ret_string, max_str_len, "MAIN");  break;
		case HD_H264E_HIGH_PROFILE:      snprintf(p_ret_string, max_str_len, "HIGH");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown profile(%d)\r\n", (int)(profile));
			return (-1);
	}
	return 0;
}

INT _hd_videoenc_menu_cvt_entropy(HD_VIDEOENC_CODING  entropy, CHAR *p_ret_string, INT max_str_len)
{
	switch (entropy) {
		case HD_H264E_CAVLC_CODING:  snprintf(p_ret_string, max_str_len, "CAVLC");  break;
		case HD_H264E_CABAC_CODING:
		case HD_H265E_CABAC_CODING:  snprintf(p_ret_string, max_str_len, "CABAC");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown entropy(%d)\r\n", (int)(entropy));
			return (-1);
	}
	return 0;
}

INT _hd_videoenc_menu_cvt_rc_mode(HD_VIDEOENC_RC_MODE  rc_mode, CHAR *p_ret_string, INT max_str_len)
{
	switch (rc_mode) {
		case HD_RC_MODE_CBR:    snprintf(p_ret_string, max_str_len, "CBR");   break;
		case HD_RC_MODE_VBR:    snprintf(p_ret_string, max_str_len, "VBR");   break;
		case HD_RC_MODE_FIX_QP: snprintf(p_ret_string, max_str_len, "FIX");   break;
		case HD_RC_MODE_EVBR:   snprintf(p_ret_string, max_str_len, "EVBR");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown rc_mode(%d)\r\n", (int)(rc_mode));
			return (-1);
	}
	return 0;
}

INT _hd_videoenc_menu_cvt_qp_mode(HD_VIDEOENC_QPMODE  qp_mode, CHAR *p_ret_string, INT max_str_len)
{
	switch (qp_mode) {
		case HD_VIDEOENC_QPMODE_FIXED_QP:  snprintf(p_ret_string, max_str_len, "FIX");   break;
		case HD_VIDEOENC_QPMODE_DELTA:     snprintf(p_ret_string, max_str_len, "DELTA"); break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown qp_mode(%d)\r\n", (int)(qp_mode));
			return (-1);
	}
	return 0;
}

INT _hd_videoenc_menu_cvt_jpg_rc_mode(UINT32 jpg_vbr_mode, CHAR *p_ret_string, INT max_str_len)
{
	switch (jpg_vbr_mode) {
		case 0:  snprintf(p_ret_string, max_str_len, "CBR");  break;
		case 1:  snprintf(p_ret_string, max_str_len, "VBR");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown jpg_vbr_mode(%d)\r\n", (int)(jpg_vbr_mode));
			return (-1);
	}
	return 0;
}
#if 0
int _hd_videoenc_menu_read_key_input_path(void)
{
	UINT32 path;

	printf("Please enter which path (0~%d)  =>\r\n", (int)(OUT_COUNT-1));
	path = hd_read_decimal_key_input("");

	if (path > OUT_COUNT) {
		printf("error input, path should be (0~%d)\r\n", (int)(OUT_COUNT-1));
		return (-1);
	}
	return (int)path;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _hd_videoenc_dump_info(void)
{
	#define HD_VIDEOENC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

	int i;
	INT32 did = 0;  // videoenc only 1 device
	BOOL b_first = TRUE;
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	UINT32 state[OUT_COUNT] = {0};

	// get each port's state first
	{
		for (i=0; i<16; i++) {
			_hd_videoenc_get_state(HD_VIDEOENC_OUT(0, i), &state[i]);
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d PATH & BIND ------------------------------\r\n", (int)did);
		_hd_dump_printf("in    out   state bind_src              bind_dest\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				CHAR  dest_in[32]="";
				CHAR  src_out[32]="";
				HD_IN_ID video_src_out_id = 0;
				HD_IN_ID video_dest_in_id = 0;

				_hd_videoenc_get_bind_src(HD_VIDEOENC_IN(0, i), &video_src_out_id);
				_hd_src_out_id_str(HD_GET_DEV(video_src_out_id), HD_GET_OUT(video_src_out_id), src_out, 32);

				_hd_videoenc_get_bind_dest(HD_VIDEOENC_OUT(0, i), &video_dest_in_id);
				_hd_dest_in_id_str(HD_GET_DEV(video_dest_in_id), HD_GET_IN(video_dest_in_id), dest_in, 32);
				_hd_dump_printf("%-4d  %-4d  %-5s %-20s  %-20s\r\n",
					(int)i, (int)i, s_str[state[i]], src_out, dest_in);
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d PATH CONFIG ------------------------------\r\n", (int)did);
		_hd_dump_printf("in    out   codec  max_w max_h  svc  ltr  rotate  bitrate   enc_ms  sout  isp_id  min_i  min_p  fit  br  hwp  basec  svcc  ltrc  sscso  ssnojpg  sscs_addr   sscs_size  ttc  seics  lpm  maq  maqs  maqst  maqe  bsring\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_PATH_CONFIG  path_cfg;
				HD_VIDEOENC_BS_RING enc_bs_ring;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				UINT32 min_i_ratio=0, min_p_ratio=0, b_fit_work_mem=0, b_bs_quick_rollback=0, hw_padding_mode=0, b_comm_srcout=0, b_comm_srcout_no_jpg_enc=0;
				UINT32 b_comm_base_recfrm=0, b_comm_svc_recfrm=0, b_comm_ltr_recfrm=0, comm_srcout_addr=0, comm_srcout_size=0;
				UINT32 b_timer_trig_comp=0, b_h26x_sei_chksum=0, b_h26x_low_power=0;
				UINT32 b_h26x_maq_diff_en=0, h26x_maq_diff_str=0, h26x_maq_diff_start_idx=0, h26x_maq_diff_end_idx=0;
				CHAR  codec[8]="";
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &path_cfg);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_BS_RING, &enc_bs_ring);
				_hd_videoenc_menu_cvt_codec(path_cfg.max_mem.codec_type, codec, 8);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_MIN_I_RATIO, &min_i_ratio);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_MIN_P_RATIO, &min_p_ratio);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY, &b_fit_work_mem);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_BS_QUICK_ROLLBACK, &b_bs_quick_rollback);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_HW_PADDING_MODE, &hw_padding_mode);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_BASE_RECFRM, &b_comm_base_recfrm);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_SVC_RECFRM, &b_comm_svc_recfrm);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_LTR_RECFRM, &b_comm_ltr_recfrm);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_SRCOUT_ENABLE, &b_comm_srcout);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC, &b_comm_srcout_no_jpg_enc);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_SRCOUT_ADDR, &comm_srcout_addr);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_COMM_SRCOUT_SIZE, &comm_srcout_size);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE, &b_timer_trig_comp);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_SEI_CHKSUM, &b_h26x_sei_chksum);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_LOW_POWER, &b_h26x_low_power);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE, &b_h26x_maq_diff_en);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_MAQ_DIFF_STR, &h26x_maq_diff_str);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX, &h26x_maq_diff_start_idx);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX, &h26x_maq_diff_end_idx);

				_hd_dump_printf("%-4d  %-4d  %-5s  %-4d  %-4d    %d    %d      %d    %-8d  %-6d  %-4d  %-6d  %-5d  %-5d  %-3d  %-2d  %-3d  %-5d  %-4d  %-4d  %-5d  %-7d  0x%08x  %-9d  %-3d  %-5d  %-3d  %-3d  %-4d  %-5d  %-4d  %-6d\r\n",
					(int)i,
					(int)i,
					codec,
					(int)path_cfg.max_mem.max_dim.w,
					(int)path_cfg.max_mem.max_dim.h,
					(int)path_cfg.max_mem.svc_layer,
					(int)path_cfg.max_mem.ltr,
					(int)path_cfg.max_mem.rotate,
					(int)path_cfg.max_mem.bitrate,
					(int)path_cfg.max_mem.enc_buf_ms,
					(int)path_cfg.max_mem.source_output,
					(int)path_cfg.isp_id,
					(int)min_i_ratio,
					(int)min_p_ratio,
					(int)b_fit_work_mem,
					(int)b_bs_quick_rollback,
					(int)hw_padding_mode,
					(int)b_comm_base_recfrm,
					(int)b_comm_svc_recfrm,
					(int)b_comm_ltr_recfrm,
					(int)b_comm_srcout,
					(int)b_comm_srcout_no_jpg_enc,
					(int)comm_srcout_addr,
					(int)comm_srcout_size,
					(int)b_timer_trig_comp,
					(int)b_h26x_sei_chksum,
					(int)b_h26x_low_power,
					(int)b_h26x_maq_diff_en,
					(int)h26x_maq_diff_str,
					(int)h26x_maq_diff_start_idx,
					(int)h26x_maq_diff_end_idx,
					(int)enc_bs_ring.enable);
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d IN FRAME ---------------------------------\r\n", (int)did);
		_hd_dump_printf("in    w     h     pxlfmt  frc     dir   ddr_id  func\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_IN  enc_in;
				HD_VIDEOENC_FUNC_CONFIG  func_cfg;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_IN, &enc_in);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &func_cfg);
				CHAR  pxlfmt[16]="", dir[8]="";

				_hd_video_pxlfmt_str(enc_in.pxl_fmt, pxlfmt, 16);
				_hd_video_dir_str(enc_in.dir, dir, 8);

				_hd_dump_printf("%-4d  %-4d  %-4d  %-6s  %2u/%-2u   %-4s  %-6d  %08x\r\n", (int)i, (int)enc_in.dim.w, (int)enc_in.dim.h, pxlfmt, (unsigned int)(GET_HI_UINT16(enc_in.frc)), (unsigned int)(GET_LO_UINT16(enc_in.frc)), dir, (int)func_cfg.ddr_id, (unsigned int)func_cfg.in_func);
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d OUT BS -----------------------------------\r\n", (int)did);

		// H264 & H265
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_VIDEOENC_FUNC_CONFIG  func_cfg;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &func_cfg);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					CHAR  codec[8]="", profile[8]="", entropy[8]="";
					MP_VDOENC_FRM_CROP_INFO h26x_crop = {0};
					_hd_videoenc_menu_cvt_codec(enc_out.codec_type, codec, 8);
					_hd_videoenc_menu_cvt_profile(enc_out.h26x.profile, profile, 8);
					_hd_videoenc_menu_cvt_entropy(enc_out.h26x.entropy_mode, entropy, 8);
					_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_CROP, (UINT32 *)&h26x_crop);

					if (b_first == TRUE) {
						b_first = FALSE;
						_hd_dump_printf("--- [H26x] ---\r\n");
						_hd_dump_printf("out   ddr_id  codec  gop  cq_idx  scq_idx  ltr_int  ltr_ref  gray  src_out  profile  level  svc  entropy  crop  dispw  disph\r\n");
					}

					_hd_dump_printf("%-4d  %-6d  %-5s  %-4d  %-6d  %-7d  %-7d  %-7d  %-4d  %-7d  %-7s  %-5d  %-3d  %-7s  %-4d  %-5d  %-5d\r\n",
						(int)i,
						(int)func_cfg.ddr_id,
						codec,
						(int)enc_out.h26x.gop_num,
						(int)enc_out.h26x.chrm_qp_idx,
						(int)enc_out.h26x.sec_chrm_qp_idx,
						(int)enc_out.h26x.ltr_interval,
						(int)enc_out.h26x.ltr_pre_ref,
						(int)enc_out.h26x.gray_en,
						(int)enc_out.h26x.source_output,
						profile,
						(int)enc_out.h26x.level_idc,
						(int)enc_out.h26x.svc_layer,
						entropy,
						(int)h26x_crop.frm_crop_enable,
						(int)h26x_crop.display_w,
						(int)h26x_crop.display_h);
				}
			}
		}

		// JPEG
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_VIDEOENC_FUNC_CONFIG  func_cfg;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				UINT32 jpg_vbr_mode = 0;
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &func_cfg);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_JPG_VBR_MODE, &jpg_vbr_mode);

				if (enc_out.codec_type == HD_CODEC_TYPE_JPEG) {
					CHAR  codec[8]="", rc_mode[8]="";
					UINT32 jpg_yuv_trans_en=0, jpg_jfif_en=0;
					UINT32 jpg_rc_min_q=0;
					UINT32 jpg_rc_max_q=0;
					_hd_videoenc_menu_cvt_codec(enc_out.codec_type, codec, 8);
					_hd_videoenc_menu_cvt_jpg_rc_mode(jpg_vbr_mode, rc_mode, 8);
					_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_JPG_YUV_TRAN_ENABLE, &jpg_yuv_trans_en);
					_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_JPG_JFIF_ENABLE, &jpg_jfif_en);
					_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_JPG_RC_MIN_QUALITY, &jpg_rc_min_q);
					_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_JPG_RC_MAX_QUALITY, &jpg_rc_max_q);

					if (b_first == TRUE) {
						b_first = FALSE;
						_hd_dump_printf("\r\n--- [JPEG] ---\r\n");
						_hd_dump_printf("out   ddr_id  codec  res_int  img_q  bitrate  fr      mode  q(min/max)  yuv_trans  jfif\r\n");
					}
					_hd_dump_printf("%-4d  %-6d  %-5s  %-7d  %-5d  %-8d %-3d/%2d  %-5s  (%3d/%3d)  %-9d  %-4d\r\n",
						(int)i,
						(int)func_cfg.ddr_id,
						codec,
						(int)enc_out.jpeg.retstart_interval,
						(int)enc_out.jpeg.image_quality,
						(int)enc_out.jpeg.bitrate,
						(int)enc_out.jpeg.frame_rate_base,
						(int)enc_out.jpeg.frame_rate_incr,
						(enc_out.jpeg.bitrate>0)? rc_mode:"....",
						(int)jpg_rc_min_q,
						(int)jpg_rc_max_q,
						(int)jpg_yuv_trans_en,
						(int)jpg_jfif_en);
				}
			}
		}

	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d VUI --------------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   vui_en  sar_w  sar_h  mat_c  tran_c  col_prim  vid_fmt  col_rng  time_pre\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_VUI  enc_vui;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &enc_vui);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_vui.vui_en == FALSE) {
						_hd_dump_printf("%-4d  %-6d  .....  .....  .....  ......  ........  .......  .......  ........\r\n", (int)i, (int)enc_vui.vui_en);
					}else{
						_hd_dump_printf("%-4d  %-6d  %-5d  %-5d  %-5d  %-6d  %-8d  %-7d  %-7d  %-8d\r\n",
							(int)i,
							(int)enc_vui.vui_en,
							(int)enc_vui.sar_width,
							(int)enc_vui.sar_height,
							(int)enc_vui.matrix_coef,
							(int)enc_vui.transfer_characteristics,
							(int)enc_vui.colour_primaries,
							(int)enc_vui.video_format,
							(int)enc_vui.color_range,
							(int)enc_vui.timing_present_flag);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d DEBLOCK ----------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   dis_ilf_idc  alpha  beta\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_DEBLOCK  enc_deblock;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &enc_deblock);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_deblock.dis_ilf_idc == 1) { // no filter
						_hd_dump_printf("%-4d  %-11d  .....  ....\r\n", (int)i, (int)enc_deblock.dis_ilf_idc);
					}else{
						_hd_dump_printf("%-4d  %-11d  %-5d  %-4d\r\n",
							(int)i,
							(int)enc_deblock.dis_ilf_idc,
							(int)enc_deblock.db_alpha,
							(int)enc_deblock.db_beta);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d RC ---------------------------------------\r\n", (int)did);

		// H264 & H265 - CBR
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_RATE_CONTROL2  enc_rc;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &enc_rc);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_rc.rc_mode == HD_RC_MODE_CBR) {
						CHAR  rc_mode[8]="";
						UINT32 h26x_rc_gop=0;
						UINT32 h26x_svc_weight_mode=0;
						UINT32 br_tolerance=0;
						_hd_videoenc_menu_cvt_rc_mode(enc_rc.rc_mode, rc_mode, 8);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_RC_GOPNUM, &h26x_rc_gop);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_SVC_WEIGHT_MODE, &h26x_svc_weight_mode);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_BR_TOLERANCE, &br_tolerance);

						if (b_first == TRUE) {
							b_first = FALSE;
							_hd_dump_printf("out   mode  swmode  gop  bitrate  fr     I(int/min/max)  P(int/min/max)  sta  ip_w  kp_p  kp_w  p2_w  p3_w  lt_w  mas  max_size  br_t\r\n");
						}
						_hd_dump_printf("%-4d  %-5s %-7d %-4d %-8d %-3d/%2d  ( %-2d/ %-2d/ %-2d)   ( %-2d/ %-2d/ %-2d)  %-3d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-3d  %-8d  %-4d\r\n",
							(int)i,
							rc_mode,
							(int)h26x_svc_weight_mode,
							(int)h26x_rc_gop,
							(int)enc_rc.cbr.bitrate,
							(int)enc_rc.cbr.frame_rate_base,
							(int)enc_rc.cbr.frame_rate_incr,
							(int)enc_rc.cbr.init_i_qp,
							(int)enc_rc.cbr.min_i_qp,
							(int)enc_rc.cbr.max_i_qp,
							(int)enc_rc.cbr.init_p_qp,
							(int)enc_rc.cbr.min_p_qp,
							(int)enc_rc.cbr.max_p_qp,
							(int)enc_rc.cbr.static_time,
							(int)enc_rc.cbr.ip_weight,
							(int)enc_rc.cbr.key_p_period,
							(int)enc_rc.cbr.kp_weight,
							(int)enc_rc.cbr.p2_weight,
							(int)enc_rc.cbr.p3_weight,
							(int)enc_rc.cbr.lt_weight,
							(int)enc_rc.cbr.motion_aq_str,
							(int)enc_rc.cbr.max_frame_size,
							(int)br_tolerance);
					}
				}
			}
		}

		// H264 & H265 - VBR
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_RATE_CONTROL2  enc_rc;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &enc_rc);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_rc.rc_mode == HD_RC_MODE_VBR) {
						CHAR  rc_mode[8]="";
						UINT32 h26x_vbr_policy=0;
						UINT32 h26x_rc_gop=0;
						UINT32 h26x_svc_weight_mode=0;
						UINT32 br_tolerance=0;
						_hd_videoenc_menu_cvt_rc_mode(enc_rc.rc_mode, rc_mode, 8);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_VBR_POLICY, &h26x_vbr_policy);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_RC_GOPNUM, &h26x_rc_gop);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_SVC_WEIGHT_MODE, &h26x_svc_weight_mode);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_BR_TOLERANCE, &br_tolerance);

						if (b_first == TRUE) {
							b_first = FALSE;
							_hd_dump_printf("out   mode(p)  swmode  gop  bitrate  fr     I(int/min/max)  P(int/min/max)  sta  ip_w  kp_p  kp_w  p2_w  p3_w  lt_w  mas  max_size  pos  br_t\r\n");
						}
						_hd_dump_printf("%-4d  %-4s(%-1d)  %-7d %-4d %-8d %-3d/%2d  ( %-2d/ %-2d/ %-2d)   ( %-2d/ %-2d/ %-2d)  %-3d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-3d  %-8d  %-3d  %-4d\r\n",
							(int)i,
							rc_mode,
							(int)h26x_vbr_policy,
							(int)h26x_svc_weight_mode,
							(int)h26x_rc_gop,
							(int)enc_rc.vbr.bitrate,
							(int)enc_rc.vbr.frame_rate_base,
							(int)enc_rc.vbr.frame_rate_incr,
							(int)enc_rc.vbr.init_i_qp,
							(int)enc_rc.vbr.min_i_qp,
							(int)enc_rc.vbr.max_i_qp,
							(int)enc_rc.vbr.init_p_qp,
							(int)enc_rc.vbr.min_p_qp,
							(int)enc_rc.vbr.max_p_qp,
							(int)enc_rc.vbr.static_time,
							(int)enc_rc.vbr.ip_weight,
							(int)enc_rc.vbr.key_p_period,
							(int)enc_rc.vbr.kp_weight,
							(int)enc_rc.vbr.p2_weight,
							(int)enc_rc.vbr.p3_weight,
							(int)enc_rc.vbr.lt_weight,
							(int)enc_rc.vbr.motion_aq_str,
							(int)enc_rc.vbr.max_frame_size,
							(int)enc_rc.vbr.change_pos,
							(int)br_tolerance);
					}
				}
			}
		}

		// H264 & H265 - FIXQP
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_RATE_CONTROL2  enc_rc;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &enc_rc);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_rc.rc_mode == HD_RC_MODE_FIX_QP) {
						CHAR  rc_mode[8]="";
						_hd_videoenc_menu_cvt_rc_mode(enc_rc.rc_mode, rc_mode, 8);

						if (b_first == TRUE) {
							b_first = FALSE;
							_hd_dump_printf("out   mode  fr     i_qp  p_qp\r\n");
						}
						_hd_dump_printf("%-4d  %-4s  %-3d/%2d %-4d  %-4d\r\n",
							(int)i,
							rc_mode,
							(int)enc_rc.fixqp.frame_rate_base,
							(int)enc_rc.fixqp.frame_rate_incr,
							(int)enc_rc.fixqp.fix_i_qp,
							(int)enc_rc.fixqp.fix_p_qp);
					}
				}
			}
		}

		// H264 & H265 - EVBR
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_RATE_CONTROL2  enc_rc;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &enc_rc);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_rc.rc_mode == HD_RC_MODE_EVBR) {
						CHAR  rc_mode[8]="";
						UINT32 h26x_rc_gop=0;
						UINT32 h26x_svc_weight_mode=0;
						UINT32 br_tolerance=0;
						_hd_videoenc_menu_cvt_rc_mode(enc_rc.rc_mode, rc_mode, 8);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_RC_GOPNUM, &h26x_rc_gop);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_H26X_SVC_WEIGHT_MODE, &h26x_svc_weight_mode);
						_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_BR_TOLERANCE, &br_tolerance);

						if (b_first == TRUE) {
							b_first = FALSE;
							_hd_dump_printf("out   mode  swmode  gop  bitrate  fr     I(int/min/max)  P(int/min/max)  sta  ip_w  kp_p  kp_w  p2_w  p3_w  lt_w  mas  max_size  sfc  mrt  si  sp  skp  br_t\r\n");
						}
						_hd_dump_printf("%-4d  %-4s  %-7d %-4d %-8d %-3d/%2d  ( %-2d/ %-2d/ %-2d)   ( %-2d/ %-2d/ %-2d)  %-3d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-3d  %-8d  %-4d %-3d  %-2d  %-2d  %-3d  %-4d\r\n",
							(int)i,
							rc_mode,
							(int)h26x_svc_weight_mode,
							(int)h26x_rc_gop,
							(int)enc_rc.evbr.bitrate,
							(int)enc_rc.evbr.frame_rate_base,
							(int)enc_rc.evbr.frame_rate_incr,
							(int)enc_rc.evbr.init_i_qp,
							(int)enc_rc.evbr.min_i_qp,
							(int)enc_rc.evbr.max_i_qp,
							(int)enc_rc.evbr.init_p_qp,
							(int)enc_rc.evbr.min_p_qp,
							(int)enc_rc.evbr.max_p_qp,
							(int)enc_rc.evbr.static_time,
							(int)enc_rc.evbr.ip_weight,
							(int)enc_rc.evbr.key_p_period,
							(int)enc_rc.evbr.kp_weight,
							(int)enc_rc.evbr.p2_weight,
							(int)enc_rc.evbr.p3_weight,
							(int)enc_rc.evbr.lt_weight,
							(int)enc_rc.evbr.motion_aq_str,
							(int)enc_rc.evbr.max_frame_size,
							(int)enc_rc.evbr.still_frame_cnd,
							(int)enc_rc.evbr.motion_ratio_thd,
							(int)enc_rc.evbr.still_i_qp,
							(int)enc_rc.evbr.still_p_qp,
							(int)enc_rc.evbr.still_kp_qp,
							(int)br_tolerance);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d USER QP ----------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   en  map_addr\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_USR_QP  enc_usr_qp;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_USR_QP, &enc_usr_qp);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_usr_qp.enable == FALSE) {
						_hd_dump_printf("%-4d  %-2d  ........\r\n", (int)i, (int)enc_usr_qp.enable);
					}else{
						_hd_dump_printf("%-4d  %-2d  %08x\r\n", (int)i, (int)enc_usr_qp.enable, (unsigned int)enc_usr_qp.qp_map_addr);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d SLICE SPLIT ------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   en  row_num\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_SLICE_SPLIT  enc_slice_split;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &enc_slice_split);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_slice_split.enable == FALSE) {
						_hd_dump_printf("%-4d  %-2d  .......\r\n", (int)i, (int)enc_slice_split.enable);
					}else{
						_hd_dump_printf("%-4d  %-2d  %-7d\r\n", (int)i, (int)enc_slice_split.enable, (int)enc_slice_split.slice_row_num);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d GDR --------------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   en  period  row_num  i_en  qp_en  qp\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out = {0};
				HD_H26XENC_GDR  enc_gdr = {0};
				MP_VDOENC_GDR_INFO gdr_info = {0};
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &enc_gdr);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_GDR, (UINT32 *)&gdr_info);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_gdr.enable == FALSE) {
						_hd_dump_printf("%-4d  %-2d  ......  .......\r\n", (int)i, (int)enc_gdr.enable);
					}else{
						_hd_dump_printf("%-4d  %-2d  %-6d  %-7d  %-4d  %-5d  %-2d\r\n", (int)i, (int)enc_gdr.enable, (int)enc_gdr.period, (int)enc_gdr.number, (int)gdr_info.enable_gdr_i_frm, (int)gdr_info.enable_gdr_qp, (int)gdr_info.gdr_qp);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d ROI --------------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   qp_mode  win  qp  rect(x,y,w,h)\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_ROI  enc_roi;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &enc_roi);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					int j;
					CHAR  roi_qp_mode[8]="";

					for (j=0; j<HD_H26XENC_ROI_WIN_COUNT; j++) {
#if defined(_BSP_NA51000_)
						_hd_videoenc_menu_cvt_qp_mode(enc_roi.roi_qp_mode, roi_qp_mode, 8);
#elif defined(_BSP_NA51055_)
						_hd_videoenc_menu_cvt_qp_mode(enc_roi.st_roi[j].mode, roi_qp_mode, 8);
#endif
						if (enc_roi.st_roi[j].enable== TRUE) {
							_hd_dump_printf("%-4d  %-7s  %-3d  %-2d  (%d,%d,%d,%d)\r\n",
								(int)i,
								roi_qp_mode,
								(int)j,
								(int)enc_roi.st_roi[j].qp,
								(int)enc_roi.st_roi[j].rect.x,
								(int)enc_roi.st_roi[j].rect.y,
								(int)enc_roi.st_roi[j].rect.w,
								(int)enc_roi.st_roi[j].rect.h);
						}
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d ROW RC -----------------------------------\r\n", (int)did);

		// H264 & H265
#if defined(_BSP_NA51000_)
		_hd_dump_printf("out   en  qp_rng  qp_step\r\n");
#elif defined(_BSP_NA51055_)
		_hd_dump_printf("out   en  i_qp_rng  i_qp_step  i_qp_min  i_qp_max  p_qp_rng  p_qp_step  p_qp_min  p_qp_max\r\n");
#endif
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_ROW_RC  enc_row_rc;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &enc_row_rc);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_row_rc.enable == FALSE) {
#if defined(_BSP_NA51000_)
						_hd_dump_printf("%-4d  %-2d  ......  .......\r\n", (int)i, (int)enc_row_rc.enable);
#elif defined(_BSP_NA51055_)
						_hd_dump_printf("%-4d  %-2d  ........  .........  ........  ........  ........  .........  ........  ........\r\n", (int)i, (int)enc_row_rc.enable);
#endif
					}else{
#if defined(_BSP_NA51000_)
						_hd_dump_printf("%-4d  %-2d  %-6d  %-7d\r\n", (int)i, (int)enc_row_rc.enable, (int)enc_row_rc.i_qp_range, (int)enc_row_rc.i_qp_step);
#elif defined(_BSP_NA51055_)
						_hd_dump_printf("%-4d  %-2d  %-8d  %-9d  %-8d  %-8d  %-8d  %-9d  %-8d  %-8d\r\n",
						   (int)i,
						   (int)enc_row_rc.enable,
						   (int)enc_row_rc.i_qp_range,
						   (int)enc_row_rc.i_qp_step,
						   (int)enc_row_rc.min_i_qp,
						   (int)enc_row_rc.max_i_qp,
						   (int)enc_row_rc.p_qp_range,
						   (int)enc_row_rc.p_qp_step,
						   (int)enc_row_rc.min_p_qp,
						   (int)enc_row_rc.max_p_qp);
#endif
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d AQ ---------------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   en  i_str  p_str  max_delta  min_delta\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out;
				HD_H26XENC_AQ  enc_aq;
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &enc_aq);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (enc_aq.enable == FALSE) {
						_hd_dump_printf("%-4d  %-2d  .....  .....  .........  .........\r\n", (int)i, (int)enc_aq.enable);
					}else{
						_hd_dump_printf("%-4d  %-2d  %-5d  %-5d  %-9d  %-9d\r\n",
							(int)i,
							(int)enc_aq.enable,
							(int)enc_aq.i_str,
							(int)enc_aq.p_str,
							(int)enc_aq.max_delta_qp,
							(int)enc_aq.min_delta_qp);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d JND ---------------------------------------\r\n", (int)did);

		// H264 & H265
		_hd_dump_printf("out   en  str  level  threshold\r\n");
		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEOENC_OUT2  enc_out = {0};
				MP_VDOENC_JND_INFO jnd_info = {0};
				HD_PATH_ID video_enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(i), HD_OUT(i));
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_JND, (UINT32 *)&jnd_info);

				if ((enc_out.codec_type == HD_CODEC_TYPE_H264) || (enc_out.codec_type == HD_CODEC_TYPE_H265)) {
					if (jnd_info.enable == FALSE) {
						_hd_dump_printf("%-4d  %-2d  ...  .....  .........\r\n", (int)i, (int)jnd_info.enable);
					}else{
						_hd_dump_printf("%-4d  %-2d  %-3d  %-5d  %-9d\r\n",
							(int)i,
							(int)jnd_info.enable,
							(int)jnd_info.str,
							(int)jnd_info.level,
							(int)jnd_info.threshold);
					}
				}
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEOENC %-2d RDO --------------------------------------\r\n", (int)did);

		// H264
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				MP_VDOENC_RDO_INFO rdo_info = {0};
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_RDO, (UINT32 *)&rdo_info);

				if (rdo_info.rdo_codec == VDOENC_RDO_CODEC_264) {
					if (b_first == TRUE) {
						b_first = FALSE;
						_hd_dump_printf("--- [H264] ---\r\n");
						_hd_dump_printf("out   intra4x4  intra8x8  intra16x16  inter_tu4  inter_tu8  inter_skip\r\n");
					}

					_hd_dump_printf("%-4d  %-8d  %-8d  %-10d  %-9d  %-9d  %-10d\r\n",
						(int)i,
						(int)rdo_info.rdo_param.rdo_264.avc_intra_4x4_cost_bias,
						(int)rdo_info.rdo_param.rdo_264.avc_intra_8x8_cost_bias,
						(int)rdo_info.rdo_param.rdo_264.avc_intra_16x16_cost_bias,
						(int)rdo_info.rdo_param.rdo_264.avc_inter_tu4_cost_bias,
						(int)rdo_info.rdo_param.rdo_264.avc_inter_tu8_cost_bias,
						(int)rdo_info.rdo_param.rdo_264.avc_inter_skip_cost_bias);
				}
			}
		}

		// H265
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				MP_VDOENC_RDO_INFO rdo_info = {0};
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_RDO, (UINT32 *)&rdo_info);

				if (rdo_info.rdo_codec == VDOENC_RDO_CODEC_265) {
					if (b_first == TRUE) {
						b_first = FALSE;
						_hd_dump_printf("--- [H265] ---\r\n");
						_hd_dump_printf("out   intra32x32  intra16x16  intra8x8  inter_skip  inter_merge  inter64x64  inter64x32  inter32x32  inter32x16  inter16x16\r\n");
					}

					_hd_dump_printf("%-4d  %-10d  %-10d  %-8d  %-10d  %-11d  %-10d  %-10d  %-10d  %-10d  %-10d\r\n",
						(int)i,
						(int)rdo_info.rdo_param.rdo_265.hevc_intra_32x32_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_intra_16x16_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_intra_8x8_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_skip_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_merge_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_64x64_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_32x32_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias,
						(int)rdo_info.rdo_param.rdo_265.hevc_inter_16x16_cost_bias);
				}
			}
		}

		// not set
		b_first = TRUE;

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				MP_VDOENC_RDO_INFO rdo_info = {0};
				_hd_videoenc_kflow_param_get_by_vendor(0+DEV_BASE, i+OUT_BASE, VDOENC_PARAM_RDO, (UINT32 *)&rdo_info);

				if (rdo_info.rdo_codec > VDOENC_RDO_CODEC_265) {
					if (b_first == TRUE) {
						b_first = FALSE;
						_hd_dump_printf("--- [not set] ---\r\n");
						_hd_dump_printf("out \r\n");
					}

					_hd_dump_printf("%-4d\r\n", (int)i);
				}
			}
		}
	}
	return 0;
}

static int hd_videoenc_show_status_p(void)
{
	system("cat /proc/hdal/venc/info");
	return 0;
}
#if 0
static int hd_videoenc_show_encinfo_p(void)
{
	int path = _hd_videoenc_menu_read_key_input_path();
	char cmd[256];

	if (path < 0) return 0;

	snprintf(cmd, 256, "echo vdoenc encinfo %d > /proc/hdal/venc/cmd", path);
	system(cmd);

	return 0;
}

static int hd_videoenc_show_rcinfo_on_p(void)
{
	int path = _hd_videoenc_menu_read_key_input_path();
	char cmd[256];

	if (path < 0) return 0;

	snprintf(cmd, 256, "echo vdoenc rcinfo %d 1 > /proc/hdal/venc/cmd", path);
	system(cmd);

	return 0;
}

static int hd_videoenc_show_rcinfo_off_p(void)
{
	int path = _hd_videoenc_menu_read_key_input_path();
	char cmd[256];

	if (path < 0) return 0;

	snprintf(cmd, 256, "echo vdoenc rcinfo %d 0 > /proc/hdal/venc/cmd", path);
	system(cmd);

	return 0;
}
#endif
static HD_DBG_MENU videoenc_debug_menu[] = {
	{0x01, "dump status",                       hd_videoenc_show_status_p,            TRUE},
	//{0x02, "enc info",                          hd_videoenc_show_encinfo_p,           TRUE},
	//{0x03, "rc info ON",                        hd_videoenc_show_rcinfo_on_p,         TRUE},
	//{0x04, "rc info OFF",                       hd_videoenc_show_rcinfo_off_p,        TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videoenc_menu(void)
{
	return hd_debug_menu_entry_p(videoenc_debug_menu, "VIDEOENC");
}



