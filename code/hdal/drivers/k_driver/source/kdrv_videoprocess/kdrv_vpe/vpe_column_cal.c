
#include <kwrap/debug.h>
#include <kwrap/type.h>

#include "kdrv_videoprocess/kdrv_vpe.h"
#include "vpe_config_base.h"

#include "vpe_column_cal.h"
#include "vpe_dbg.h"

#define VPE_SCA_FILTER_COMPATIBLE  DISABLE


int32_t vpe_sca_filter_coefs[17][4] = {
    {0, 0, 0, 64},
    {0, 0, 3, 58},
    {0, 0, 7, 50},
    {0, 0, 11, 42},
    {0, 1, 13, 36},
    {0, 1, 15, 32},
    {0, 2, 15, 30},
    {0, 3, 15, 28},
    {0, 4, 15, 26},
    {1, 4, 15, 24},
    {1, 5, 15, 22},
    {2, 7, 14, 18},
    {3, 8, 13, 16},
    {4, 8, 13, 14},
    {4, 9, 12, 14},
    {6, 9, 11, 12},
    {6, 9, 11, 12}
};


void vpe_sca_get_filter_coefs(uint32_t sca_factor, int32_t *p_get_coefs)
{
    uint8_t i = 0, idx = 0;

#if (VPE_SCA_FILTER_COMPATIBLE == DISABLE)

    if (sca_factor <= 1024) {
        idx = 0;
    } else if (sca_factor <= 1280) {
        idx = 1;
    } else if (sca_factor <= 1536) {
        idx = 2;
    } else if (sca_factor <= 1792) {
        idx = 3;
    } else if (sca_factor <= 2048) {
        idx = 4;
    } else if (sca_factor <= 2304) {
        idx = 5;
    } else if (sca_factor <= 2560) {
        idx = 6;
    } else if (sca_factor <= 2816) {
        idx = 7;
    } else if (sca_factor <= 3072) {
        idx = 8;
    } else if (sca_factor <= 3328) {
        idx = 9;
    } else if (sca_factor <= 3584) {
        idx = 10;
    } else if (sca_factor <= 3840) {
        idx = 11;
    } else if (sca_factor <= 4096) {
        idx = 12;
    } else if (sca_factor <= 5120) {
        idx = 13;
    } else if (sca_factor <= 6144) {
        idx = 14;
    } else if (sca_factor <= 7168) {
        idx = 15;
    } else {
        idx = 16;
    }
#else

    idx = 0;
    
#endif

    for (i = 0; i < 4; i++) {
        p_get_coefs[i] = vpe_sca_filter_coefs[idx][i];
    }
    
}



uint32_t vpe_int_to_2comp(int32_t val, int32_t bits)
{
	uint32_t tmp = 0;
	uint32_t mask = (1 << bits) - 1;

	if (val >= 0) {
		tmp = (((uint32_t)val) & mask);
	} else {
		tmp = ((uint32_t)(~(-1 * val)) & mask) + 1;
	}

	return tmp;
}



bool vpe_stripe_cal(STRIPE_PARAM *get_stp_info, STP_MODE stp_mode, uint32_t width, uint32_t stripe_size, uint32_t stp_overlap_size)
{
	//uint32_t i = 0, j = 0, k = 0;

	uint32_t S_IME_ST_SIZE = stripe_size;
	uint32_t S_IME_HN = 0, S_IME_HM = 0, S_IME_HL = 0, S_IME_HOVLP = 0;
	uint32_t CheckSize = 0;

	//bool stp_flag = false;
	uint32_t rem = 0;

	//uint32_t stp_num = 0, get_stp_size[8] = {0x0};

	if (S_IME_ST_SIZE > 2048) {
		S_IME_ST_SIZE = 2048;
	}


	S_IME_HOVLP = stp_overlap_size;

	get_stp_info->stp_overlap = stp_overlap_size;

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("VPE: Stripe mode: %d\r\n", (int)stp_mode);
	DBG_ERR("VPE: Stripe overlap size: %d\r\n", (int)S_IME_HOVLP);
#endif

	if ((S_IME_ST_SIZE == S_IME_HOVLP)) {
		S_IME_ST_SIZE += S_IME_HOVLP;
	}

	if (S_IME_ST_SIZE > 2048) {
		DBG_ERR("error: stripe size + overlap size > 2048\r\n");
		return false;
	}

	if (stp_mode == STPMODE_SST) {
		if (width > stripe_size) {
			DBG_ERR("VPE: width: %d > StripeSize: %d, can not be operating in SST mode\r\n", (int)width, (int)stripe_size);


			return false;
		} else {
			S_IME_HN = width;
			S_IME_HL = width;
			S_IME_HM = 0;

			S_IME_HM += 1;

			get_stp_info->stp_hn = S_IME_HN;
			get_stp_info->stp_hl = S_IME_HL;
			get_stp_info->stp_hm = S_IME_HM;
		}
	} else if (stp_mode == STPMODE_MST) {
		if ((width <= S_IME_ST_SIZE)) {
			S_IME_HN = width;
			S_IME_HL = width;
			S_IME_HM = 0;

			S_IME_HM += 1;

			get_stp_info->stp_hn = S_IME_HN;
			get_stp_info->stp_hl = S_IME_HL;
			get_stp_info->stp_hm = S_IME_HM;
		} else {
			S_IME_HN = S_IME_ST_SIZE;
			rem = width;
			S_IME_HM = 0;
			while (1) {
				if ((rem > (S_IME_HOVLP + 1))) {
					if ((rem > S_IME_HN)) {
						rem = rem - S_IME_HN;
						rem = rem + S_IME_HOVLP;

						S_IME_HM += 1;
					} else {
						S_IME_HL = rem;
						S_IME_HM += 1;

						break;
					}
				} else {
					DBG_ERR("last stripe error, please decrease stripe size...\r\n");

					return false;
				}
			}

			if (S_IME_HL < (S_IME_HN * 1 / 8)) {
				DBG_ERR("stripe size different is large, S_IME_HN: %d,  S_IME_HL: %d\r\n", (int)S_IME_HN, (int)S_IME_HL);

			}

			get_stp_info->stp_hn = S_IME_HN;
			get_stp_info->stp_hl = S_IME_HL;
			get_stp_info->stp_hm = S_IME_HM;
		}
	}


	CheckSize = ((S_IME_HN - S_IME_HOVLP) * (S_IME_HM - 1)) + S_IME_HL;
	if (CheckSize != width) {
		DBG_ERR("Horizontal stripe check fail...\r\n");
		DBG_ERR("width: %d, check size: %d\r\n", (int)width, (int)CheckSize);

	}


#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("VPE-S_IME_HN: %d\r\n", (int)S_IME_HN);
	DBG_ERR("VPE-S_IME_HL: %d\r\n", (int)S_IME_HL);
	DBG_ERR("VPE-S_IME_HM: %d\r\n", (int)S_IME_HM);
	DBG_ERR("VPE-CheckSize: %d\r\n", (int)CheckSize);
	DBG_ERR("VPE: stripe computation done ...\r\n");
#endif

	return true;
}


int32_t vpe_get_basic_stripe(VPE_IMG_SCALE_PARAM *p_scl_info, uint32_t path_num, uint32_t overlap_size)
{
	uint32_t max_stripe_size = 2048;

	uint32_t dst_max_size_h = 0;
	uint32_t i = 0, r = 0;
	uint32_t get_stp_size = 0, src_size_h = 0; // stp_nums = 0,


	src_size_h = p_scl_info[0].in_size_h;


	dst_max_size_h = p_scl_info[0].in_size_h;
	for (i = 0; i < path_num; i++) {
		if ((p_scl_info[i].sca_en == 1) && (p_scl_info[i].out_size_h > dst_max_size_h)) {
			dst_max_size_h = p_scl_info[i].out_size_h;
		}
	}

	//DBG_ERR("dst_max_size_h: " << dst_max_size_h << endl;

	if (dst_max_size_h <= max_stripe_size) {
		get_stp_size = src_size_h;
	} else {
		r = (dst_max_size_h + 2047) >> 11;  // needed stripe number

		while (1) {
			get_stp_size = VPE_ALIGN_CEIL_32(((src_size_h + ((r - 1) * overlap_size)) + (r >> 1)) / r);

			if (get_stp_size <= max_stripe_size) {
				break;
			} else {
				r += 1;
			}
		}
	}

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("get_stp_size: %d\r\n", (int)get_stp_size);
#endif

	return get_stp_size;

}

#if 0
void vpe_user_to_hw_param_map(VPE_BGL_STRIPE_PARAM *p_in_src, VPE_BGL_STRIPE_PARAM *p_out_dst)
{
	typedef struct _pos_info_ {
		int32_t start_x;
		int32_t end_x;
	} pos_info;

	uint32_t res_idx = 0;
	uint32_t min_x = 8192, max_x = 0;

	pos_info res_proc_crop_pos[4] = {0};

	for (res_idx = 0; res_idx < 4; res_idx++) {
		res_proc_crop_pos[res_idx].start_x = p_in_src->vpe_out_param[res_idx].sca_proc_crop_x_pos;
		res_proc_crop_pos[res_idx].end_x = p_in_src->vpe_out_param[res_idx].sca_proc_crop_x_pos + p_in_src->vpe_out_param[res_idx].sca_proc_crop_width - 1;
	}

	for (res_idx = 0; res_idx < 4; res_idx++) {
		if (p_in_src->vpe_out_param[res_idx].sca_en == 1) {
			if (res_proc_crop_pos[res_idx].start_x <= min_x) {
				min_x = res_proc_crop_pos[res_idx].start_x;
			}

			if (res_proc_crop_pos[res_idx].start_x >= max_x) {
				max_x = res_proc_crop_pos[res_idx].end_x;
			}
		}
	}

	p_out_dst->vpe_in_param.proc_x_pos = min_x;
	p_out_dst->vpe_in_param.proc_width = max_x - min_x + 1;

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("new_proc_x_pos: %d\r\n", p_out_dst->vpe_in_param.proc_x_pos);
	DBG_ERR("new_proc_width: %d\r\n", p_out_dst->vpe_in_param.proc_width);
#endif
}
#endif

#if 0
void vpe_cal_src_in_stripe(VPE_IMG_SCALE_PARAM *p_set_scl_info, STRIPE_PARAM *p_get_stp_info)
{
	uint32_t m_stp_overlap_size = 0;//, m_stp_size = 0;
	uint32_t stp_size = 0, stp_overlap = 0;//, stp_num = 0, stp_cnt = 0;
	uint32_t in_size_h = 0;
	uint32_t get_basic_stp_size = 0;

	STRIPE_PARAM get_stp_info = {0};
	//VPE_IMG_SCALE_PARAM set_scl_info;

	in_size_h = p_set_scl_info[0].in_size_h;

	m_stp_overlap_size = 128;

	get_basic_stp_size = vpe_get_basic_stripe(p_set_scl_info, 4, m_stp_overlap_size);

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("get_basic_stp_size: %d\r\n", (int)get_basic_stp_size);
#endif

	stp_size = get_basic_stp_size;
	//DBG_ERR("stp_size: %d\r\n", stp_size);

	//stp_size = m_stp_size;
	if (in_size_h > stp_size) {
		stp_overlap = m_stp_overlap_size;
	} else {
		stp_size = in_size_h;
		stp_overlap = 0;
	}

	vpe_stripe_cal(&get_stp_info, STPMODE_MST, in_size_h, stp_size, stp_overlap);

	p_get_stp_info->stp_hn = get_stp_info.stp_hn;
	p_get_stp_info->stp_hl = get_stp_info.stp_hl;
	p_get_stp_info->stp_hm = get_stp_info.stp_hm;
	p_get_stp_info->stp_overlap = get_stp_info.stp_overlap;

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("stp_hn: %d\r\n", (int)p_get_stp_info->stp_hn);
	DBG_ERR("stp_hl: %d\r\n", (int)p_get_stp_info->stp_hl);
	DBG_ERR("stp_hm: %d\r\n", (int)p_get_stp_info->stp_hm);
	DBG_ERR("stp_ov: %d\r\n", (int)p_get_stp_info->stp_overlap);
	DBG_ERR("\r\n");
#endif
}
#endif

static uint32_t vpe_cal_dce_max_stripe_size(uint32_t cache_size, uint32_t width, uint32_t height)
{
	uint32_t w = width;
	uint32_t r = height;
	uint32_t d = cache_size;
	uint32_t slot_max = 128;
	uint32_t slot_dmin = d;
	uint32_t slot_dmax = (92682 * cache_size) >> 16;

	uint32_t slot_dst = (slot_dmin + slot_dmax) >> 1;

	uint32_t pw_max = 0;

	if (w <= 1) {
		DBG_WRN("vpe: image width(%d) <= 1...\r\n", (int)w);

		return 768;
	}

	if (r == 0) {
		DBG_WRN("vpe: user lens radius is zero...\r\n");

		r = w >> 1;
	}
	pw_max = ((((w * slot_max * slot_dst) >> 1) / r) * 1304) >> 12;

	return VPE_ALIGN_FLOOR_8(pw_max);
}

POS_INFO stp_sca_out_pos[VPE_MAX_COL_NUM] = {0};
static int vpe_get_scale_out_crop_info(uint32_t res_num, VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info)
{
	uint32_t stp_idx = 0, start_pos = 0, end_pos = 0;
	uint32_t post_sca_crop_start_pos = 0, post_sca_crop_end_pos = 0;
	uint32_t stp_start_idx = 0, stp_end_idx = 0;
	uint32_t post_sca_crop_h_size_cnt = 0, out_h_size_cnt = 0;
	uint32_t out_size = 0, dist = 0, out_pos_cnt = 0;

	memset((void *)stp_sca_out_pos, 0x0, sizeof(POS_INFO) * VPE_MAX_COL_NUM);

	if (p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_width == 0) {
		DBG_ERR("res[%d]: post scale crop width size is 0\r\n", (int)res_num);
		return -1;
	}

	if (p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_height == 0) {
		DBG_ERR("res[%d]: post scale crop height size is 0\r\n", (int)res_num);
		return -1;
	}

	post_sca_crop_start_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_x_pos;
	post_sca_crop_end_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_width + post_sca_crop_start_pos - 1;
	//DBG_DUMP("res[%d]: post_sca_crop_start_pos: %d, post_sca_crop_end_pos: %d\n", res_num, post_sca_crop_start_pos, post_sca_crop_end_pos);

	start_pos = 0;
	for (stp_idx = p_sca_in_stp_info->valid_stp_start_idx; stp_idx <= p_sca_in_stp_info->valid_stp_end_idx; stp_idx++) {
		//DBG_DUMP("\n#### stp_idx: %d,  stp_sca_size_h: %d\n", stp_idx, p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_size_h);

		end_pos = p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_size_h + start_pos - 1;
		stp_sca_out_pos[stp_idx].start_x = start_pos;
		stp_sca_out_pos[stp_idx].end_x   = end_pos;
		//DBG_DUMP("stp[%d]: sca_out_start_pos: %d,  sca_out_end_pos: %d\n", stp_idx, stp_sca_out_pos[stp_idx].start_x, stp_sca_out_pos[stp_idx].end_x);

		if ((post_sca_crop_start_pos >= start_pos) && (post_sca_crop_start_pos <= end_pos)) {
			stp_start_idx = stp_idx;
		}

		if ((post_sca_crop_end_pos >= start_pos) && (post_sca_crop_end_pos <= end_pos)) {
			stp_end_idx = stp_idx;
		}

		start_pos = end_pos + 1;
	}
	//DBG_DUMP("\nstp_start_idx: %d,  stp_end_idx: %d\n\n", stp_start_idx, stp_end_idx);

	if (stp_start_idx > p_sca_in_stp_info->valid_stp_start_idx) {
		for (stp_idx = p_sca_in_stp_info->valid_stp_start_idx; stp_idx < stp_start_idx; stp_idx++) {
			p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
		}
	}

	if (stp_end_idx < p_sca_in_stp_info->valid_stp_end_idx) {
		for (stp_idx = stp_end_idx + 1; stp_idx <= p_sca_in_stp_info->valid_stp_end_idx; stp_idx++) {
			p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
		}
	}
	p_sca_in_stp_info->valid_stp_start_idx = stp_start_idx;
	p_sca_in_stp_info->valid_stp_end_idx   = stp_end_idx;
	//DBG_DUMP("new stp_start_idx: %d,  new stp_end_idx: %d\n", p_sca_in_stp_info->valid_stp_start_idx, p_sca_in_stp_info->valid_stp_end_idx);


	post_sca_crop_h_size_cnt = p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_width;

	post_sca_crop_end_pos = 0;
	for (stp_idx = stp_start_idx; stp_idx <= stp_end_idx; stp_idx++) {
		if (stp_idx == stp_start_idx) {
			post_sca_crop_start_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_x_pos - stp_sca_out_pos[stp_idx].start_x;
			post_sca_crop_end_pos = stp_sca_out_pos[stp_idx].end_x - stp_sca_out_pos[stp_idx].start_x;

			out_size = (post_sca_crop_end_pos - post_sca_crop_start_pos + 1);
			if (post_sca_crop_h_size_cnt <= out_size) {
				out_size = post_sca_crop_h_size_cnt;
			}

			post_sca_crop_h_size_cnt = post_sca_crop_h_size_cnt - out_size;
		} else if (stp_idx == stp_end_idx) {
			post_sca_crop_start_pos = stp_sca_out_pos[stp_idx].start_x - stp_sca_out_pos[stp_idx].start_x;
			post_sca_crop_end_pos = (p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_width + p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_x_pos - 1) - stp_sca_out_pos[stp_idx].start_x;

			out_size = (post_sca_crop_end_pos - post_sca_crop_start_pos + 1);

			if (post_sca_crop_h_size_cnt <= out_size) {
				out_size = post_sca_crop_h_size_cnt;
			}

			post_sca_crop_h_size_cnt = post_sca_crop_h_size_cnt - out_size;
		} else {
			post_sca_crop_start_pos = stp_sca_out_pos[stp_idx].start_x - stp_sca_out_pos[stp_idx].start_x;
			post_sca_crop_end_pos   = stp_sca_out_pos[stp_idx].end_x - stp_sca_out_pos[stp_idx].start_x;

			out_size = (post_sca_crop_end_pos - post_sca_crop_start_pos + 1);

			if (post_sca_crop_h_size_cnt <= out_size) {
				out_size = post_sca_crop_h_size_cnt;
			}

			post_sca_crop_h_size_cnt = post_sca_crop_h_size_cnt - out_size;
		}


		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_pos_x = post_sca_crop_start_pos;
		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_size_h = out_size;

		p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_size_h = p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_size_h;

		//DBG_DUMP("res[%d]col[%d]: \n", res_num, stp_idx);
		//DBG_DUMP("post_sca_crop_start_pos: %d, post_sca_crop_end_pos: %d, crop_width: %d\n", post_sca_crop_start_pos, post_sca_crop_end_pos, p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_size_h);

		//post_sca_crop_h_size_cnt = post_sca_crop_h_size_cnt - out_size;
		if (post_sca_crop_h_size_cnt == 0) {
			break;
		}
	}

	out_pos_cnt = p_stp_gbl_info->vpe_out_param[res_num].out_x_pos;

	out_h_size_cnt = p_stp_gbl_info->vpe_out_param[res_num].out_width;
	if (stp_start_idx == stp_end_idx) {
		p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x = p_stp_gbl_info->vpe_out_param[res_num].rlt_x_pos;

		p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h = out_h_size_cnt;
		p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x = out_pos_cnt;

		//if (out_h_size_cnt >= p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h) {
		//	out_h_size_cnt = out_h_size_cnt - p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h;
		//}
	} else {
		for (stp_idx = stp_start_idx; stp_idx <= stp_end_idx; stp_idx++) {
			if (stp_idx == stp_start_idx) {
				p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x = p_stp_gbl_info->vpe_out_param[res_num].rlt_x_pos;

				dist = p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x - 0;

				p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h = p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_size_h + dist;
				p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x = out_pos_cnt;
				out_pos_cnt = out_pos_cnt + p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h;

				if (out_h_size_cnt >= p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h) {
					out_h_size_cnt = out_h_size_cnt - p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h;
				}
			} else if (stp_idx == stp_end_idx) {
				p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x = 0;

				p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h = out_h_size_cnt;
				p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x = out_pos_cnt;

				if (out_h_size_cnt >= p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h) {
					out_h_size_cnt = out_h_size_cnt - p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h;
				}
			} else {
				p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x = 0;

				p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h = p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_size_h;
				p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x = out_pos_cnt;
				out_pos_cnt = out_pos_cnt + p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h;

				if (out_h_size_cnt >= p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h) {
					out_h_size_cnt = out_h_size_cnt - p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h;
				}
			}

			//printf("rlt_pos_x: %d, rlt_size_h: %d\n", p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x, p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_size_h);
			//printf("out_pos_x: %d, out_size_h: %d\n", p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x, p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h);
		}
	}

	//printf("des_out_size_h: %d\n", p_stp_gbl_info->vpe_out_param[res_num].des_width);

	//printf("\n\n");

	return 0;
}





RES_SCA_INFO res_scale_info[VPE_MAX_RES_NUM] = {0};
int vpe_cal_src_in_stripe_v3(VPE_BGL_STRIPE_PARAM *p_gbl_sca_info, STRIPE_PARAM *p_get_stp_info)
{
	uint32_t res_idx = 0;
	uint32_t stp_size = 0, stp_size_t = 0, stp_overlap = 0, stp_num = 0, stp_cnt = 0, stp_idx = 0;
	//uint32_t stp_boundary_size = 0;
	uint32_t stp_start_pos = 0;  //stp_end_pos = 0;
	uint32_t sca_factor = 0;

	uint32_t max_in_out_size = 0, min_factor = 16384, max_factor = 0;


	uint32_t sca_en_cnt = 0, pass_cnt = 0;
	uint32_t comps_en_cnt = 0;
	uint32_t dce_en = 0;

	uint32_t get_stp_size[VPE_MAX_COL_NUM] = {0}, rem_size = 0;

	uint32_t rate_int = 0, rate_dec = 0;
	uint32_t len = 0, len_limt = 0, dist = 0;
	uint32_t mask_bit = 0, mask_value = 0;
	UINT32 sw_stp_size_min_limit = 0, sw_stp_size_max_limit = 1920;
	BOOL  bstp_size_limit_match = TRUE;

	dce_en = p_gbl_sca_info->vpe_dce_en;
	memset((void *)res_scale_info, 0x0, sizeof(RES_SCA_INFO) * VPE_MAX_RES_NUM);
	memset((void *)get_stp_size, 0x0, sizeof(uint32_t) * VPE_MAX_COL_NUM);

	min_factor = 16384, max_factor = 0;
	for (res_idx = 0; res_idx < VPE_MAX_RES_NUM; res_idx++) {
		if (p_gbl_sca_info->vpe_in_param.proc_width > max_in_out_size) {
			max_in_out_size = p_gbl_sca_info->vpe_in_param.proc_width;
		}

		if (p_gbl_sca_info->vpe_out_param[res_idx].sca_en == 1) {
			sca_en_cnt += 1;

			if (p_gbl_sca_info->vpe_out_param[res_idx].sca_comps_en == 1) {
				comps_en_cnt += 1;
			}


			if (p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_width <= 1) {
				DBG_ERR("vpe: res%d crop width before scale = %d, invalid, please check!!!\r\n", (int)res_idx, (int)p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_width);
				return -1;
			}

			if (p_gbl_sca_info->vpe_out_param[res_idx].sca_width <= 1) {
				DBG_ERR("vpe: res%d scale width = %d, invalid, please check!!!\r\n", (int)res_idx, (int)p_gbl_sca_info->vpe_out_param[res_idx].sca_width);
				return -1;
			}

			if (p_gbl_sca_info->vpe_out_param[res_idx].sca_width > max_in_out_size) {
				max_in_out_size = p_gbl_sca_info->vpe_out_param[res_idx].sca_width;
			}

			sca_factor = ((p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_width - 1) << 10) / (p_gbl_sca_info->vpe_out_param[res_idx].sca_width - 1);
			if (sca_factor < min_factor) {
				min_factor = sca_factor;
			}

			if (sca_factor > max_factor) {
				max_factor = sca_factor;
			}


			res_scale_info[res_idx].sca_en = 1;

			res_scale_info[res_idx].sca_factor_h = ((p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_width - 1) << 10) / (p_gbl_sca_info->vpe_out_param[res_idx].sca_width - 1);

			res_scale_info[res_idx].sca_in_size_h = p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_width;
			res_scale_info[res_idx].sca_in_pos_start_x = p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_x_pos;
			res_scale_info[res_idx].sca_in_pos_end_x = p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_width + p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_x_pos - 1;

			//DBG_DUMP("res-%d  start_x: %d end_x: %d   src_width: %d   sca_width: %d   sca_factor: %d\n", res_idx, res_scale_info[res_idx].sca_in_pos_start_x, res_scale_info[res_idx].sca_in_pos_end_x, res_scale_info[res_idx].sca_in_size_h, p_gbl_sca_info->vpe_out_param[res_idx].sca_width, res_scale_info[res_idx].sca_factor_h);
		}
	}

	if (max_in_out_size > 4096) {
		DBG_ERR("vpe: input crop or scale width > 4096\r\n");
		return -1;
	}

	//DBG_DUMP("max_in_out_size: %d\r\n", max_in_out_size);
	//DBG_DUMP("min_factor: %d\r\n", min_factor);


	stp_size = (2048 * min_factor) >> 10;
	//DBG_DUMP("proc_width: %d\n", p_gbl_sca_info->vpe_in_param.proc_width);
	//DBG_DUMP("stp_size: %d  %d\n", stp_size, VPE_ALIGN_FLOOR_8(stp_size));


	if ((max_in_out_size <= 2048) && (dce_en == 0)) {
		stp_size = p_gbl_sca_info->vpe_in_param.proc_width;
		stp_overlap = 0;
	} else {

		if (comps_en_cnt != 0) {
			stp_overlap = (((max_factor + 1023) >> 10) + 1) * 32;
			if (stp_overlap > 256) {
				stp_overlap = 256;
			}
		} else {
			stp_overlap = ((max_factor + 1023) >> 10) * 8;
			if (stp_overlap > 64) {
				stp_overlap = 64;
			}
		}

		sw_stp_size_min_limit = (2 * 16) + ((stp_overlap) /*>> 1*/);

		stp_size = VPE_ALIGN_FLOOR_8(stp_size);

		if ((dce_en == 1) && (p_gbl_sca_info->vpe_dce_stripe)) {
			stp_size = vpe_cal_dce_max_stripe_size(16, p_gbl_sca_info->vpe_in_param.proc_width, p_gbl_sca_info->vpe_in_param.dce_lens_radius);
		}

		if (stp_size > sw_stp_size_max_limit) {
			stp_size = sw_stp_size_max_limit;
		} else if (stp_size < 512) {
			stp_size = 512;
		}

		stp_size = stp_size - stp_overlap;

		while (1) {

			stp_size_t = stp_size + stp_overlap;

			//DBG_DUMP("stp_size: %d  %d\n", stp_size_t, ALIGN_FLOOR_8(stp_size_t));

			pass_cnt = 0;
			for (res_idx = 0; res_idx < VPE_MAX_RES_NUM; res_idx++) {
				if (p_gbl_sca_info->vpe_out_param[res_idx].sca_en == 1) {
					stp_start_pos = p_gbl_sca_info->vpe_out_param[res_idx].sca_proc_crop_x_pos;

					rate_int = stp_start_pos / stp_size_t;
					rate_dec = stp_start_pos - (rate_int * stp_size_t);

					dist = (stp_size_t - rate_dec);

					len = (dist << 10) / res_scale_info[res_idx].sca_factor_h;

					//DBG_DUMP("res%d - rate_int: %d,  rate_dec: %d, stp_start_pos: %d, dist: %d, len: %d\n", res_idx, rate_int, rate_dec, stp_start_pos, dist, len);

					if (p_gbl_sca_info->vpe_out_param[res_idx].sca_comps_en == 1) {
						mask_bit = 5;
						mask_value = (1 << mask_bit) - 1;
						len_limt = (1 << mask_bit) + 2;
					} else {
						mask_bit = 3;
						mask_value = (1 << mask_bit) - 1;
						len_limt = (1 << mask_bit) + 2;
					}

					//#20220604_1-cliff_modify_start
					rem_size = p_gbl_sca_info->vpe_in_param.proc_width;
					stp_cnt = 0;
					while ((rem_size != 0) && (stp_cnt < VPE_MAX_COL_NUM)) {
						if (rem_size > stp_size_t) {
							get_stp_size[stp_cnt] = stp_size_t;
							rem_size = rem_size - stp_size_t + stp_overlap;
							stp_cnt += 1;
						} else {
							get_stp_size[stp_cnt] = rem_size;
							rem_size = 0;
							stp_cnt += 1;
							
						}
					}
					if (rem_size != 0) {
						stp_cnt += 1;
					}

					if(stp_cnt > VPE_MAX_COL_NUM){
						break;
					}					

					bstp_size_limit_match = TRUE;


					if (((len >> mask_bit) > 0) && (len >= len_limt)) {

						for (stp_idx = 0; stp_idx < (stp_cnt/* + 1*/) ; stp_idx++) {
							if (get_stp_size[stp_idx] < sw_stp_size_min_limit) {
								bstp_size_limit_match = FALSE;
								break;
							}	
						}
						if (bstp_size_limit_match == TRUE) {
							pass_cnt += 1;
						}
					}
					//#20220604_1-cliff_modify_end

				}

			}

			if (pass_cnt == sca_en_cnt) {
				break;
			} else {
				stp_size = stp_size - 8;
				#if 1
				if (stp_size < (sw_stp_size_min_limit - stp_overlap)) {
					    DBG_ERR("vpe(a): stp_size < stp_size_min_limit(%d,%d)\n", (int)(stp_size+stp_overlap), (int)sw_stp_size_min_limit);
						return -1;
				}
				#endif


				
			}

			//DBG_DUMP("========================\n");

		}

		//DBG_DUMP("f stp_size: %d\n", stp_size);
	}


	//DBG_DUMP("rem_size: %d\n", p_gbl_sca_info->vpe_in_param.proc_width);

	rem_size = p_gbl_sca_info->vpe_in_param.proc_width;

	stp_cnt = 0;
	stp_size = stp_size + stp_overlap;
	while (rem_size != 0) {
		if (rem_size > stp_size) {
			get_stp_size[stp_cnt] = stp_size;
			rem_size = rem_size - stp_size + stp_overlap;

			stp_cnt += 1;
		} else {
			get_stp_size[stp_cnt] = rem_size;
			rem_size = 0;
			if (get_stp_size[stp_cnt] < sw_stp_size_min_limit) {
				DBG_ERR("vpe(b): stp_size < stp_size_min_limit(%d,%d)\n", (int)get_stp_size[stp_cnt], (int)sw_stp_size_min_limit);
				return -1;
			}

			
			stp_cnt += 1;
		}
	}

	stp_num = stp_cnt;
	if (stp_num > VPE_MAX_COL_NUM) {
		DBG_ERR("vpe: column number is overflow (%d) > 8\n", (int)stp_num);
		return -1;
	}
	//DBG_DUMP("stp_num: %d\n", stp_num);

#if 0
	len = 0;
	for (stp_idx = 0; stp_idx < stp_num; stp_idx++) {
		//DBG_DUMP("get_stp_size[%d]: %d\n", stp_idx, get_stp_size[stp_idx]);

		len += get_stp_size[stp_idx];
	}
	len = len - (stp_overlap * (stp_num - 1));
#endif

	//DBG_DUMP("check len: %d\n", len);

	p_get_stp_info->stp_hm = stp_num;
	p_get_stp_info->stp_overlap = stp_overlap;
	#if 1

	if (stp_num > 7) {
		stp_num = 7;
	} else if (stp_num < 1) {
		stp_num = 1;
	}
	p_get_stp_info->stp_hn = get_stp_size[0];
	p_get_stp_info->stp_hl = get_stp_size[stp_num - 1];

	#else
	if (stp_num > 1) {
		p_get_stp_info->stp_hn = get_stp_size[0];
		p_get_stp_info->stp_hl = get_stp_size[stp_num - 1];
	} else {
		p_get_stp_info->stp_hn = get_stp_size[0];
		p_get_stp_info->stp_hl = get_stp_size[stp_num - 1];
	}
	#endif

	//DBG_DUMP("\n\n");

	return 0;
}


void vpe_get_in_column_info(VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, STRIPE_PARAM *p_stp_sw_info, VPE_HW_IN_COL_PARAM *p_stp_hw_info)
{
	uint32_t i = 0;
	uint32_t stp_num = p_stp_sw_info->stp_hm;
	uint32_t stp_size_n = p_stp_sw_info->stp_hn;
	uint32_t stp_size_l = p_stp_sw_info->stp_hl;
	uint32_t stp_size_o = p_stp_sw_info->stp_overlap;

	uint32_t stp_proc_x_start_cnt = 0;
	uint32_t stp_col_x_start_cnt = 0;

	stp_proc_x_start_cnt = p_stp_gbl_info->vpe_in_param.proc_x_pos;
	stp_col_x_start_cnt = 0;

	for (i = 0; i < stp_num; i++) {
		if (i == 0) { // first stripe
			//if(p_stp_gbl_info->vpe_dce_en == 0)
			{
				stp_proc_x_start_cnt += 0;
				stp_col_x_start_cnt += 0;
			}

			p_stp_hw_info->in_col_info[i].proc_width = stp_size_n;
			p_stp_hw_info->in_col_info[i].proc_x_start = stp_proc_x_start_cnt;
			p_stp_hw_info->in_col_info[i].col_x_start  = stp_col_x_start_cnt;
		} else if (i == (stp_num - 1)) { // last stripe
			//if(p_stp_gbl_info->vpe_dce_en == 0)
			{
				stp_proc_x_start_cnt += (p_stp_hw_info->in_col_info[i - 1].proc_width - stp_size_o);

				stp_col_x_start_cnt += (p_stp_hw_info->in_col_info[i - 1].proc_width - stp_size_o);
			}

			p_stp_hw_info->in_col_info[i].proc_width = stp_size_l;
			p_stp_hw_info->in_col_info[i].proc_x_start = stp_proc_x_start_cnt;
			p_stp_hw_info->in_col_info[i].col_x_start  = stp_col_x_start_cnt;
		} else { // other stripes
			//if(p_stp_gbl_info->vpe_dce_en == 0)
			{
				stp_proc_x_start_cnt += (p_stp_hw_info->in_col_info[i - 1].proc_width - stp_size_o);

				stp_col_x_start_cnt += (p_stp_hw_info->in_col_info[i - 1].proc_width - stp_size_o);
			}

			p_stp_hw_info->in_col_info[i].proc_width = stp_size_n;
			p_stp_hw_info->in_col_info[i].proc_x_start = stp_proc_x_start_cnt;
			p_stp_hw_info->in_col_info[i].col_x_start  = stp_col_x_start_cnt;
		}

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("in_col_info[%d].proc_width: %d\r\n", (int)i, (int)p_stp_hw_info->in_col_info[i].proc_width);
		DBG_ERR("in_col_info[%d].proc_x_start: %d\r\n", (int)i, (int)p_stp_hw_info->in_col_info[i].proc_x_start);
		DBG_ERR("in_col_info[%d].col_x_start: %d\r\n", (int)i, (int)p_stp_hw_info->in_col_info[i].col_x_start);
		DBG_ERR("\r\n");
#endif
	}

	p_stp_hw_info->col_num = stp_num;

}

void vpe_refine_in_column_info(VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, VPE_HW_IN_COL_PARAM *p_stp_hw_info)
{
	uint32_t col_num = p_stp_hw_info->col_num;
	uint32_t col_idx = 0;

	if (p_stp_gbl_info->vpe_dce_en == 1) {
		for (col_idx = 0; col_idx < col_num; col_idx++) {
			p_stp_hw_info->in_col_info[col_idx].proc_x_start = p_stp_gbl_info->vpe_in_param.proc_x_pos;
		}
	}

}



POS_INFO stp_pos_with_overlap[VPE_MAX_COL_NUM] = {0};
POS_INFO stp_pos_without_overlap[VPE_MAX_COL_NUM] = {0};
int vpe_cal_sca_in_stripe(uint32_t res_num, VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, STRIPE_PARAM *p_src_in_stp_info, VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info)
{
	uint32_t stp_idx = 0;
	//uint32_t size_cnt = 0;

	uint32_t stp_num = p_src_in_stp_info->stp_hm;
	uint32_t stp_size_n = p_src_in_stp_info->stp_hn;
	uint32_t stp_size_l = p_src_in_stp_info->stp_hl;
	uint32_t stp_size_o = p_src_in_stp_info->stp_overlap;

	uint32_t stp_start_pos = 0;

	uint32_t crop_x_start_pos = 0, crop_x_end_pos = 0;
	uint32_t stp_start_idx = 0, stp_end_idx = 0;

	uint32_t crop_rem_size = 0;

	uint32_t in_size = 0, out_size = 0;
	uint32_t get_size_h = 0;//, get_size_h_cnt = 0;

	memset((void *)stp_pos_with_overlap, 0, sizeof(POS_INFO) * VPE_MAX_COL_NUM);
	memset((void *)stp_pos_without_overlap, 0, sizeof(POS_INFO) * VPE_MAX_COL_NUM);

	p_sca_in_stp_info->stp_overlap_size = stp_size_o;

	for (stp_idx = 0; stp_idx < VPE_MAX_COL_NUM; stp_idx++) {
		p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_v = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_height;

		if (p_stp_gbl_info->vpe_out_param[res_num].sca_width <= 1) {
			DBG_ERR("vpe: res%d scale width(%d) <= 1, please check!!!\r\n", (int)res_num, (int)p_stp_gbl_info->vpe_out_param[res_num].sca_width);
			return -1;
		}
		in_size = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width;
		out_size = p_stp_gbl_info->vpe_out_param[res_num].sca_width;
		p_sca_in_stp_info->sca_factor_h = (((in_size - 1) << 10) + ((out_size - 1) >> 1)) / (out_size - 1);

		if (p_stp_gbl_info->vpe_out_param[res_num].sca_height <= 1) {
			DBG_ERR("vpe: res%d scale height(%d) <= 1, please check!!!\r\n", (int)res_num, (int)p_stp_gbl_info->vpe_out_param[res_num].sca_height);
			return -1;
		}
		in_size = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_height;
		out_size = p_stp_gbl_info->vpe_out_param[res_num].sca_height;
		p_sca_in_stp_info->sca_factor_v = (((in_size - 1) << 10) + ((out_size - 1) >> 1)) / (out_size - 1);


		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_pos_x = 0;
		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_size_h = 8;
		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_pos_y  = p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_y_pos;
		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_crop_size_v = p_stp_gbl_info->vpe_out_param[res_num].sca_tag_crop_height;

		p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_x = 0;
		p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_size_h = 8;
		p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_pos_y = p_stp_gbl_info->vpe_out_param[res_num].rlt_y_pos;
		p_sca_in_stp_info->sca_stp_info[stp_idx].rlt_size_v = p_stp_gbl_info->vpe_out_param[res_num].rlt_height;

		p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x = 0;
		p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_h = 8;
		p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_y = p_stp_gbl_info->vpe_out_param[res_num].out_y_pos;
		p_sca_in_stp_info->sca_stp_info[stp_idx].out_size_v = p_stp_gbl_info->vpe_out_param[res_num].out_height;
	}

	//---------------------------------------------------------------
	// define start and end position for each stripe
	if (stp_num == 1) {
		stp_pos_with_overlap[0].start_x = 0;
		stp_pos_with_overlap[0].end_x   = stp_size_l - 1;

		stp_pos_without_overlap[0].start_x = 0;
		stp_pos_without_overlap[0].end_x = stp_size_l - 1;
	} else {
		stp_start_pos = 0;

		for (stp_idx = 0; stp_idx < stp_num; stp_idx++) {
			if (stp_idx == 0) {
				stp_pos_with_overlap[stp_idx].start_x = stp_start_pos;
				stp_pos_with_overlap[stp_idx].end_x   = stp_start_pos + stp_size_n - 1;

				stp_start_pos = stp_start_pos + stp_size_n - stp_size_o;

				stp_pos_without_overlap[stp_idx].start_x = stp_pos_with_overlap[stp_idx].start_x;
				stp_pos_without_overlap[stp_idx].end_x = stp_pos_with_overlap[stp_idx].end_x - (stp_size_o >> 1);
			} else if (stp_idx == (stp_num - 1)) {
				stp_pos_with_overlap[stp_idx].start_x = stp_start_pos;
				stp_pos_with_overlap[stp_idx].end_x   = stp_start_pos + stp_size_l - 1;

				stp_pos_without_overlap[stp_idx].start_x = stp_pos_with_overlap[stp_idx].start_x + (stp_size_o >> 1);
				stp_pos_without_overlap[stp_idx].end_x = stp_pos_with_overlap[stp_idx].end_x;
			} else {
				stp_pos_with_overlap[stp_idx].start_x = stp_start_pos;
				stp_pos_with_overlap[stp_idx].end_x   = stp_start_pos + stp_size_n - 1;

				stp_start_pos = stp_start_pos + stp_size_n - stp_size_o;

				stp_pos_without_overlap[stp_idx].start_x = stp_pos_with_overlap[stp_idx].start_x + (stp_size_o >> 1);
				stp_pos_without_overlap[stp_idx].end_x = stp_pos_with_overlap[stp_idx].end_x - (stp_size_o >> 1);
			}

#if (VPE_COL_MST_DBG == 1)
			DBG_ERR("\r\n\r\n");
			DBG_ERR("stp_pos_with_overlap[%d].start_x: %d\r\n", (int)stp_idx, (int)stp_pos_with_overlap[stp_idx].start_x);
			DBG_ERR("stp_pos_with_overlap[%d].end_x: %d\r\n", (int)stp_idx, (int)stp_pos_with_overlap[stp_idx].end_x);

			DBG_ERR("stp_pos_without_overlap[%d].start_x: %d\r\n", (int)stp_idx, (int)stp_pos_without_overlap[stp_idx].start_x);
			DBG_ERR("stp_pos_without_overlap[%d].end_x: %d\r\n", (int)stp_idx, (int)stp_pos_without_overlap[stp_idx].end_x);
			DBG_ERR("\r\n\r\n");
#endif

		}
	}

	//----------------------------------------------------------------------------

	crop_x_start_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_x_pos;
	crop_x_end_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width + crop_x_start_pos - 1;
	for (stp_idx = 0; stp_idx < stp_num; stp_idx++) {
		if ((crop_x_start_pos >= stp_pos_without_overlap[stp_idx].start_x) && (crop_x_start_pos <= stp_pos_without_overlap[stp_idx].end_x)) {
			stp_start_idx = stp_idx;
		}

		if ((crop_x_end_pos >= stp_pos_without_overlap[stp_idx].start_x) && (crop_x_end_pos <= stp_pos_without_overlap[stp_idx].end_x)) {
			stp_end_idx = stp_idx;
		}
	}

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("crop_x_start_pos: %d\r\n", (int)crop_x_start_pos);
	DBG_ERR("stp_start_idx: %d\r\n", (int)stp_start_idx);
	DBG_ERR("stp_end_idx: d\r\n", (int)stp_end_idx);
#endif

	p_sca_in_stp_info->valid_stp_start_idx = stp_start_idx;
	p_sca_in_stp_info->valid_stp_end_idx = stp_end_idx;
	p_sca_in_stp_info->valid_stp_num = (stp_end_idx - stp_start_idx + 1);


	crop_x_start_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_x_pos;
	//crop_x_end_pos = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width + crop_x_start_pos - 1;
	crop_rem_size = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width + ((stp_end_idx - stp_start_idx) * stp_size_o);

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("crop_rem_size: %d\r\n", (int)crop_rem_size);
#endif

	for (stp_idx = stp_start_idx; stp_idx <= stp_end_idx; stp_idx++) {
		p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 0;

		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_pos_y = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_y_pos;
		if (stp_idx == stp_start_idx) {
			p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_pos_x = crop_x_start_pos - stp_pos_with_overlap[stp_idx].start_x;
		} else {
			p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_pos_x = 0;
		}

		get_size_h = stp_pos_with_overlap[stp_idx].end_x - crop_x_start_pos + 1;
		if (crop_rem_size >= get_size_h) {
			crop_rem_size = crop_rem_size - get_size_h;
		} else {
			get_size_h = crop_rem_size;
			crop_rem_size = 0;
		}
		//DBG_ERR("crop_rem_size: " << crop_rem_size << endl;

		p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = get_size_h;

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("res%d: stp_idx: %d,  get_size_h: %d\r\n", (int)res_num, (int)stp_idx, (int)get_size_h);
#endif

		if (stp_idx != stp_end_idx) {
			crop_x_start_pos = stp_pos_with_overlap[stp_idx + 1].start_x;
		}

		if (crop_rem_size == 0) {
			break;
		}

	}

	for (stp_idx = 0; stp_idx < stp_start_idx; stp_idx++) {
		p_sca_in_stp_info->sca_stp_info[stp_idx] = p_sca_in_stp_info->sca_stp_info[stp_start_idx];
		p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
	}

	for (stp_idx = stp_end_idx + 1; stp_idx < VPE_MAX_COL_NUM; stp_idx++) {
		p_sca_in_stp_info->sca_stp_info[stp_idx] = p_sca_in_stp_info->sca_stp_info[stp_end_idx];
		p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
	}

	return 0;
}


int vpe_get_out_column_info(uint32_t res_num, VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info)
{
	//FILE *pfile = NULL;

	uint32_t i = 0; //x = 0, y = 0, j = 0, ix = 0, iy = 0;
	//uint32_t ldx = 0, pos = 0, ildx = 0, ipos = 0;
	//uint8_t get_a0 = 0, get_a1 = 0, get_y0 = 0, get_y1 = 0, get_u0 = 0, get_u1 = 0, get_v0 = 0, get_v1 = 0;
	//uint32_t get_val = 0, get_t0 = 0, get_t1 = 0;
	//uint32_t cal_val = 0, cal_a = 0, cal_b = 0, cal_c = 0, cal_w = 0;
	//uint32_t val_pack = 0, file_size = 0;

	int32_t stp_overlap = 0, stp_num = 0;    //stp_size = 0, stp_cnt = 0;
	int32_t in_size_h = 0, in_size_v = 0;    //in_lofs = 0;
	int32_t out_size_h = 0, out_size_v = 0;  //out_lofs = 0;
	int32_t stp_size_boundary = 0;  //stp_in_size_cnt = 0;


	int32_t scl_h_ftr = 0, scl_h_ftr_int = 0, scl_h_ftr_dec = 0;
	int32_t scl_v_ftr = 0, scl_v_ftr_int = 0, scl_v_ftr_dec = 0;

	int32_t scl_h_ftr_init = 0, scl_v_ftr_init = 0;
	int32_t vpe_scl_h_ftr_init = 0, vpe_scl_v_ftr_init = 0;


	int32_t next_scl_h_ftr = 0;

	int32_t scl_h_cnt_int = 0, scl_h_cnt_dec = 0;
	int32_t pcnt = 0; //icnt = 0;
	int32_t out_size_h_cnt = 0;
	//int32_t out_size_boundary = 0;
	int32_t out_pos_x_cnt = 0, out_pos_y_cnt = 0;

	//int32_t m_stp_size = 0, m_stp_overlap_size = 0;

	int32_t factor_base_bit = 10;


	//STRIPE_PARAM get_stp_info = {0};
	//RAND_INFO rand_info;

	//bool get_dbg_file_dump_en;

	uint32_t mask_value = 0, mask_bit = 0;

	if (p_stp_gbl_info->vpe_out_param[res_num].sca_comps_en == 1) {
		mask_bit = 5;
		mask_value = (1 << mask_bit) - 1;
	} else {
		mask_bit = 3;
		mask_value = (1 << mask_bit) - 1;
	}
	//DBG_DUMP("mask_bit: %d,  mask_value: %08x\n", mask_bit, mask_value);

	if (p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width <= 1) {
		DBG_ERR("vpe: res%d crop width(%d) before scale  <= 1, please check!!!\r\n", (int)res_num, (int)p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width);
		return -1;
	}

	if (p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_height <= 1) {
		DBG_ERR("vpe: res%d crop height(%d) before scale <= 1, please check!!!\r\n", (int)res_num, (int)p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_height);
		return -1;
	}
	in_size_h = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width;
	in_size_v = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_height;
	//in_lofs = p_stp_gbl_info->vpe_out_param[res_num].sca_proc_crop_width;


	if (p_stp_gbl_info->vpe_out_param[res_num].sca_width <= 1) {
		DBG_ERR("vpe: res%d scale width(%d) <= 1, please check!!!\r\n", (int)res_num, (int)p_stp_gbl_info->vpe_out_param[res_num].sca_width);
		return -1;
	}

	if (p_stp_gbl_info->vpe_out_param[res_num].sca_height <= 1) {
		DBG_ERR("vpe: res%d scale height(%d) <= 1, please check!!!\r\n", (int)res_num, (int)p_stp_gbl_info->vpe_out_param[res_num].sca_height);
		return -1;
	}
	out_size_h = p_stp_gbl_info->vpe_out_param[res_num].sca_width;
	out_size_v = p_stp_gbl_info->vpe_out_param[res_num].sca_height;
	//out_lofs = p_stp_gbl_info->vpe_out_param[res_num].sca_width;

	out_pos_x_cnt = p_stp_gbl_info->vpe_out_param[res_num].out_x_pos;
	out_pos_y_cnt = p_stp_gbl_info->vpe_out_param[res_num].out_y_pos;

	out_size_h_cnt = out_size_h;


	scl_h_ftr = (((in_size_h - 1) * (1 << factor_base_bit)) + ((out_size_h - 1) >> 1)) / (out_size_h - 1);
	scl_v_ftr = (((in_size_v - 1) * (1 << factor_base_bit)) + ((out_size_v - 1) >> 1)) / (out_size_v - 1);

	scl_h_ftr_int = (scl_h_ftr >> factor_base_bit);
	scl_h_ftr_dec = scl_h_ftr - (scl_h_ftr_int << factor_base_bit);
	scl_v_ftr_int = (scl_v_ftr >> factor_base_bit);
	scl_v_ftr_dec = scl_v_ftr - (scl_v_ftr_int << factor_base_bit);


	scl_h_ftr_init = 0;
	scl_v_ftr_init = 0;


#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("in_size_h: %d\r\n", (int)in_size_h);
	DBG_ERR("in_size_v: %d\r\n", (int)in_size_v);

	DBG_ERR("out_size_h: %d\r\n", (int)out_size_h);
	DBG_ERR("out_size_v: %d\r\n", (int)out_size_v);

	DBG_ERR("scl_h_ftr: %d\r\n", (int)scl_h_ftr);
	DBG_ERR("scl_v_ftr: %d\r\n", (int)scl_v_ftr);
#endif

	stp_overlap = p_sca_in_stp_info->stp_overlap_size;


	p_sca_in_stp_info->sca_stp_info[i].scl_v_init_ofs_int = 0;
	p_sca_in_stp_info->sca_stp_info[i].scl_v_init_ofs_dec = 0;
	scl_v_ftr_init = (p_sca_in_stp_info->sca_stp_info[i].scl_v_init_ofs_int << factor_base_bit) + p_sca_in_stp_info->sca_stp_info[i].scl_v_init_ofs_dec;
	vpe_scl_v_ftr_init = scl_v_ftr_init + 512 - (scl_v_ftr >> 1);
	p_sca_in_stp_info->sca_stp_info[i].vpe_scl_v_ftr_init = vpe_scl_v_ftr_init;

#if (VPE_COL_MST_DBG == 1)
	DBG_ERR("**** vpe scl_v_ftr_init: %d\r\n", (int)vpe_scl_v_ftr_init);
#endif

	stp_num = p_sca_in_stp_info->valid_stp_num;
	for (i = p_sca_in_stp_info->valid_stp_start_idx; i <= p_sca_in_stp_info->valid_stp_end_idx; i++) {

		if (i == p_sca_in_stp_info->valid_stp_start_idx) { // 1st stripe

			p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_int = 0;
			p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_dec = 0;

			if (stp_num > 1) {
				//stp_size_boundary = (p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h - (stp_overlap * 2 / 4));
				stp_size_boundary = (p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h - stp_overlap);
			} else {
				stp_size_boundary = p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h;
			}

#if (VPE_COL_MST_DBG == 1)
			DBG_ERR("sca_stp_info[%d].in_size_h: %d, stp_size_boundary: %d\r\r\n", (int)i, (int)p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h, (int)stp_size_boundary);
			//DBG_ERR("sca_stp_info[i].in_size_h: " << sca_stp_info[i].in_size_h << ", stp_size_boundary: " << stp_size_boundary << endl;
#endif
		} else if (i == p_sca_in_stp_info->valid_stp_end_idx) { // last stripe

			stp_size_boundary = p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h;

#if (VPE_COL_MST_DBG == 1)
			DBG_ERR("sca_stp_info[%d].in_size_h: %d, stp_size_boundary: %d\r\r\n", (int)i, (int)p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h, (int)stp_size_boundary);
			//DBG_ERR("*** sca_stp_info[i].in_size_h: " << sca_stp_info[i].in_size_h << ", stp_size_boundary: " << stp_size_boundary << endl;
#endif
		} else { // other stripe

			//stp_size_boundary = (p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h - (stp_overlap * 2 / 4));
			stp_size_boundary = (p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h - stp_overlap);

		}

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("sca_stp_info[%d].in_size_h: %d, stp_size_boundary: %d\r\r\n", i, p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h, stp_size_boundary);
		//DBG_ERR("sca_stp_info[i].in_size_h: " << sca_stp_info[i].in_size_h << ", stp_size_boundary: " << stp_size_boundary << endl;
#endif

		scl_h_ftr_init = (p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_int << factor_base_bit) + p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_dec;
		vpe_scl_h_ftr_init = scl_h_ftr_init + 512 - (scl_h_ftr >> 1);
		p_sca_in_stp_info->sca_stp_info[i].vpe_scl_h_ftr_init = vpe_scl_h_ftr_init;


#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("**** vpe scl_h_ftr_init: %d\r\n", (int)vpe_scl_h_ftr_init);

		//DBG_ERR("sca_stp_info[i].scl_init_ofs_int: %d\r\n", sca_stp_info[i].scl_init_ofs_int);
		//DBG_ERR("sca_stp_info[i].scl_init_ofs_dec: %d\r\n", sca_stp_info[i].scl_init_ofs_dec);

		DBG_ERR("sca_stp_info[%d].scl_init_ofs_int: %d\r\n", (int)i, (int)p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_int);
		DBG_ERR("sca_stp_info[%d].scl_init_ofs_dec: %d\r\n", (int)i, (int)p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_dec);

		//DBG_ERR("sca_stp_info[%d].in_addr: 0x%08x\r\r\n", i, p_sca_in_stp_info->sca_stp_info[i].in_addr);
#endif

		//-------------------------------------------------------------------------------
		// output size emulation

		//int scl_h_tmp_int = 0, scl_h_tmp_dec = 0;
		//int scl_rem_size = 0;

		scl_h_cnt_int = 0;
		scl_h_cnt_dec = 0;

		scl_h_cnt_int = scl_h_cnt_int + p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_int;
		scl_h_cnt_dec = scl_h_cnt_dec + p_sca_in_stp_info->sca_stp_info[i].scl_h_init_ofs_dec;

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("init scl_h_cnt_int: %d\r\n", (int)scl_h_cnt_int);
		DBG_ERR("init scl_h_cnt_dec: %d\r\n", (int)scl_h_cnt_dec);
#endif

		pcnt = 0;

		while (1) {
			//if(((scl_h_cnt_int + 1) <= (stp_size_boundary - 1)))
			{
				pcnt += 1;

				scl_h_cnt_int += scl_h_ftr_int;
				scl_h_cnt_dec += scl_h_ftr_dec;
				if (scl_h_cnt_dec >= (1 << factor_base_bit)) {
					scl_h_cnt_int = scl_h_cnt_int + 1;
					scl_h_cnt_dec = scl_h_cnt_dec - (1 << factor_base_bit);
				}

				if (stp_num > 1) {

					if (i != p_sca_in_stp_info->valid_stp_end_idx) {
						if ((scl_h_cnt_int >= stp_size_boundary) && ((pcnt & mask_value) == 0)) {

							//scl_rem_size = (p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h - scl_h_cnt_int) << factor_base_bit;
							//if ((scl_rem_size / scl_h_ftr) <= 0x0000001f) {
							//    break;
							//}
							break;
						}
					} else {
						if ((scl_h_cnt_int >= stp_size_boundary)) {
							break;
						}
					}
				} else {
					if (pcnt == out_size_h) {
						break;
					}
				}
			}
		}

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("current: %d\r\n", (int)(((scl_h_cnt_int << factor_base_bit) + scl_h_cnt_dec) - scl_h_ftr));
		DBG_ERR("*pcnt: %d\r\n", (int)pcnt);
#endif

		next_scl_h_ftr = ((scl_h_cnt_int << factor_base_bit) + scl_h_cnt_dec);  // next horizontal output position
		//if ((i != p_sca_in_stp_info->valid_stp_end_idx) && ((pcnt & 0x00000007) != 0))
		//if((i != p_sca_in_stp_info->valid_stp_end_idx) && (pcnt & 0x0000001f != 0))
		//if (((pcnt & 0x0000001f) != 0) && (i != p_sca_in_stp_info->valid_stp_end_idx))
		if (((pcnt & mask_value) != 0) && (stp_num > 1)) {
			uint32_t reduce_num = 0;
			uint32_t pcnt_tmp = 0;

			//pcnt_tmp = ((pcnt >> 3) << 3);
			//pcnt_tmp = ((pcnt >> 1) << 1);
			pcnt_tmp = ((pcnt >> mask_bit) << mask_bit);
			//cout << "pcnt_tmp: " << pcnt_tmp << endl;

			reduce_num = pcnt - pcnt_tmp;

			next_scl_h_ftr = next_scl_h_ftr - (scl_h_ftr * reduce_num);

			scl_h_cnt_int = (next_scl_h_ftr >> factor_base_bit);
			scl_h_cnt_dec = (next_scl_h_ftr - (scl_h_cnt_int << factor_base_bit));

			pcnt = pcnt_tmp;
		}

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("next - scl_h_cnt_int: %d, scl_h_cnt_dec: %d\r\n", (int)scl_h_cnt_int, (int)scl_h_cnt_dec);
		DBG_ERR("pcnt: %d\r\n", (int)pcnt);
		DBG_ERR("out_size_h_cnt: %d\r\n", (int)out_size_h_cnt);
#endif

		if (i != p_sca_in_stp_info->valid_stp_end_idx) {
			p_sca_in_stp_info->sca_stp_info[i].sca_out_size_h = pcnt;
			out_size_h_cnt = out_size_h_cnt - pcnt;

			//p_sca_in_stp_info->sca_stp_info[i].out_pos_x = out_pos_x_cnt;
			//out_pos_x_cnt += p_sca_in_stp_info->sca_stp_info[i].sca_out_size_h;

			//p_sca_in_stp_info->sca_stp_info[i].out_pos_y = out_pos_y_cnt;
		} else {
			p_sca_in_stp_info->sca_stp_info[i].sca_out_size_h = out_size_h_cnt;
			out_size_h_cnt = 0;

			//p_sca_in_stp_info->sca_stp_info[i].out_pos_x = out_pos_x_cnt;
			//out_pos_x_cnt += p_sca_in_stp_info->sca_stp_info[i].sca_out_size_h;

			//p_sca_in_stp_info->sca_stp_info[i].out_pos_y = out_pos_y_cnt;
		}

		p_sca_in_stp_info->sca_stp_info[i].sca_out_size_v = out_size_v;
		//p_sca_in_stp_info->sca_stp_info[i].out_addr = out_addr;

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("sca_stp_info[%d].out_size_h: %d\r\n", (int)i, p_sca_in_stp_info->sca_stp_info[i].sca_out_size_h);
#endif


#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("sca_stp_info[%d].out_size_h: %d\r\n", (int)i, (int)p_sca_in_stp_info->sca_stp_info[i].sca_out_size_h);
		DBG_ERR("sca_stp_info[%d].out_size_v: %d\r\n", (int)i, (int)p_sca_in_stp_info->sca_stp_info[i].sca_out_size_v);
		//DBG_ERR("sca_stp_info[%d].out_addr: 0x%08x\r\r\n", i, p_sca_in_stp_info->sca_stp_info[i].out_addr);
#endif

		if ((i + 1) <= p_sca_in_stp_info->valid_stp_end_idx) {
			p_sca_in_stp_info->sca_stp_info[i + 1].scl_h_init_ofs_int = scl_h_cnt_int - (p_sca_in_stp_info->sca_stp_info[i].sca_in_crop_size_h - stp_overlap);
			p_sca_in_stp_info->sca_stp_info[i + 1].scl_h_init_ofs_dec = scl_h_cnt_dec;

#if (VPE_COL_MST_DBG == 1)
			DBG_ERR("next sca_stp_info[%d].scl_init_ofs_int: %d\r\n", (int)(i + 1), (int)p_sca_in_stp_info->sca_stp_info[i + 1].scl_h_init_ofs_int);
			DBG_ERR("next sca_stp_info[%d].scl_init_ofs_dec: %d\r\n", (int)(i + 1), (int)p_sca_in_stp_info->sca_stp_info[i + 1].scl_h_init_ofs_dec);

			if (p_sca_in_stp_info->sca_stp_info[i + 1].scl_h_init_ofs_int < 0) {
				DBG_ERR("next sca_stp_info[%d].scl_init_ofs_int: %d\r\n", (int)(i + 1), (int)p_sca_in_stp_info->sca_stp_info[i + 1].scl_h_init_ofs_int);
			}
#endif

		}

#if (VPE_COL_MST_DBG == 1)
		DBG_ERR("\r\n\r\n");
#endif
	}

	return 0;
}


uint32_t vpe_get_sca_h_div(uint32_t sca_factor)
{
	uint32_t out_div = 0;

	if (sca_factor <= 1024) {
		out_div = 4096;
	} else if (sca_factor <= 2048) {
		out_div = 2048;
	} else if (sca_factor <= 3072) {
		out_div = 1365;
	} else if (sca_factor <= 4096) {
		out_div = 1024;
	} else if (sca_factor <= 5120) {
		out_div = 819;
	} else if (sca_factor <= 6144) {
		out_div = 683;
	} else if (sca_factor <= 7168) {
		out_div = 585;
	} else if (sca_factor <= 8192) {
		out_div = 512;
	}

	return out_div;
}

uint32_t vpe_get_sca_h_avg_pxl_mask(uint32_t sca_factor)
{
	uint32_t avg_msk = 0;

	if (sca_factor <= 1024) {
		avg_msk = 8;
	} else if (sca_factor <= 2048) {
		avg_msk = 24;
	} else if (sca_factor < 3072) {
		avg_msk = 56;
	} else if (sca_factor == 3072) {
		avg_msk = 28;
	} else if (sca_factor <= 4096) {
		avg_msk = 60;
	} else if (sca_factor < 5120) {
		avg_msk = 124;
	} else if (sca_factor == 5120) {
		avg_msk = 62;
	} else if (sca_factor <= 6144) {
		avg_msk = 126;
	} else if (sca_factor < 7168) {
		avg_msk = 254;
	} else if (sca_factor == 7168) {
		avg_msk = 127;
	} else if (sca_factor <= 8192) {
		avg_msk = 255;
	}

	return avg_msk;
}

uint32_t vpe_get_sca_v_div(uint32_t sca_factor)
{
	uint32_t out_div = 0;

	if (sca_factor <= 1024) {
		out_div = 4096;
	} else if (sca_factor <= 1536) {
		out_div = 1365;
	} else if (sca_factor <= 2048) {
		out_div = 1024;
	} else if (sca_factor <= 2560) {
		out_div = 819;
	} else if (sca_factor <= 3072) {
		out_div = 683;
	} else if (sca_factor <= 3584) {
		out_div = 585;
	} else if (sca_factor <= 8192) {
		out_div = 512;
	}

	return out_div;
}

uint32_t vpe_get_sca_v_avg_pxl_mask(uint32_t sca_factor)
{
	uint32_t avg_msk = 0;

	if (sca_factor <= 1024) {
		avg_msk = 8;
	} else if (sca_factor < 1536) {
		avg_msk = 56;
	} else if (sca_factor == 1536) {
		avg_msk = 28;
	} else if (sca_factor <= 2048) {
		avg_msk = 60;
	} else if (sca_factor < 2560) {
		avg_msk = 124;
	} else if (sca_factor == 2560) {
		avg_msk = 62;
	} else if (sca_factor <= 3072) {
		avg_msk = 126;
	} else if (sca_factor < 3584) {
		avg_msk = 254;
	} else if (sca_factor == 3584) {
		avg_msk = 127;
	} else if (sca_factor <= 8192) {
		avg_msk = 255;
	}

	return avg_msk;
}


void vpe_refine_out_column_info(VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info)
{
	uint32_t stp_idx = 0;//, stp_num = 0;
	int32_t ftr_int = 0, ftr = 0;  //ftr_dec = 0,
	int32_t ofs_int = 0;

	//uint32_t out_pos_cnt_x = 0;

#if 0
	if (p_sca_in_stp_info->sca_factor_h == 1024) {
		for (stp_idx = p_sca_in_stp_info->valid_stp_start_idx; stp_idx <= p_sca_in_stp_info->valid_stp_end_idx; stp_idx++) {
			if (p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init != 0) {
				ftr_int = p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init >> 10;
				ftr_dec = p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init - (ftr_int << 10);

				if (ftr_dec == 0) {
					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_pos_x = ftr_int;
					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h - ftr_int;
					p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init = 0;
				}
			}

			p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = VPE_ALIGN_CEIL_8(p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h);
		}
	}
#else
	if (p_sca_in_stp_info->sca_factor_h == 1024) {

		if (p_sca_in_stp_info->valid_stp_end_idx > p_sca_in_stp_info->valid_stp_start_idx) {
			for (stp_idx = p_sca_in_stp_info->valid_stp_start_idx; stp_idx <= p_sca_in_stp_info->valid_stp_end_idx; stp_idx++) {

				if (stp_idx == p_sca_in_stp_info->valid_stp_start_idx) {
					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_size_h;
				} else {
					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_size_h;

					ofs_int = (p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init >> 10);

					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_pos_x += (ofs_int);
					p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init = p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init - (ofs_int << 10);
				}
			}

		}

	} else {
		for (stp_idx = p_sca_in_stp_info->valid_stp_start_idx; stp_idx <= p_sca_in_stp_info->valid_stp_end_idx; stp_idx++) {
			ftr = p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init;

			if ((ftr > 8192)) {

				ftr_int = 0;
				while (ftr > 8192) {
					ftr_int = ftr_int + 8;
					ftr = ftr - 8192;
				}

				if ((int32_t)p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h >= ftr_int) {
					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_pos_x += ftr_int;
					p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = (uint32_t)((int32_t)p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h - ftr_int);
					p_sca_in_stp_info->sca_stp_info[stp_idx].vpe_scl_h_ftr_init = ftr;
				}
			}

			p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h = VPE_ALIGN_CEIL_8(p_sca_in_stp_info->sca_stp_info[stp_idx].sca_in_crop_size_h);
		}
	}

	if (p_sca_in_stp_info->valid_stp_start_idx != 0) {  // 0 is first col. id
		for (stp_idx = 0; stp_idx < p_sca_in_stp_info->valid_stp_start_idx; stp_idx++) {
			p_sca_in_stp_info->sca_stp_info[stp_idx] = p_sca_in_stp_info->sca_stp_info[p_sca_in_stp_info->valid_stp_start_idx];

			p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
		}
	}

	if (p_sca_in_stp_info->valid_stp_end_idx != 7) {  // 7 is last col. id
		for (stp_idx = p_sca_in_stp_info->valid_stp_end_idx + 1; stp_idx < VPE_MAX_COL_NUM; stp_idx++) {
			p_sca_in_stp_info->sca_stp_info[stp_idx] = p_sca_in_stp_info->sca_stp_info[p_sca_in_stp_info->valid_stp_end_idx];

			p_sca_in_stp_info->sca_stp_info[stp_idx].skip_en = 1;
		}
	}


#endif

#if 0
	out_pos_cnt_x = 0;
	for (stp_idx = p_sca_in_stp_info->valid_stp_start_idx; stp_idx <= p_sca_in_stp_info->valid_stp_end_idx; stp_idx++) {
		p_sca_in_stp_info->sca_stp_info[stp_idx].out_pos_x = out_pos_cnt_x;

		out_pos_cnt_x += p_sca_in_stp_info->sca_stp_info[stp_idx].sca_out_size_h;
	}
#endif
	p_sca_in_stp_info->sca_hsca_div = vpe_get_sca_h_div(p_sca_in_stp_info->sca_factor_h);
	p_sca_in_stp_info->sca_havg_pxl_msk = vpe_get_sca_h_avg_pxl_mask(p_sca_in_stp_info->sca_factor_h);

	p_sca_in_stp_info->sca_vsca_div = vpe_get_sca_v_div(p_sca_in_stp_info->sca_factor_v);
	p_sca_in_stp_info->sca_vavg_pxl_msk = vpe_get_sca_v_avg_pxl_mask(p_sca_in_stp_info->sca_factor_v);
}


VPE_BGL_STRIPE_PARAM  vpe_user_in_out_img_param = {0};
VPE_IMG_SCALE_PARAM scl_info[VPE_MAX_RES_NUM] = {0};
STRIPE_PARAM get_src_in_sw_stp_info;
VPE_HW_IN_COL_PARAM in_hw_col_info = {0};
VPE_SCA_SRC_IN_STRIPE_PARAM get_sca_in_stp_info[VPE_MAX_RES_NUM] = {0};
//VPE_RES_EN_PARAM vpe_res_en_info = {0};

int vpe_cal_col_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	uint32_t j = 0, col_idx = 0, res_idx = 0;
	int32_t get_sca_filter_coefs[4] = {0};
	int start_pip_out_x = 0, dest_pip_out_x = 0;
	unsigned int sca_area, sca_out_crop_area;


	//VPE_BGL_STRIPE_PARAM  vpe_in_out_img_param = {0};

	//----------------------------------------------------------
	// get input stripe info
	//uint32_t get_stp_size = 0;

	//VPE_IMG_SCALE_PARAM set_scl_info;


	//----------------------------------------------------------
	// get output stripe info
	//VPE_SW_STRIPE_PARAM get_sw_stp_base_info[4] = {0};
	//VPE_HW_OUT_COL_PARAM out_hw_col_info = {0};

	//----------------------------------------------------------

	memset((void *)&vpe_user_in_out_img_param, 0, sizeof(VPE_BGL_STRIPE_PARAM));
	memset((void *)scl_info, 0, sizeof(VPE_IMG_SCALE_PARAM) * VPE_MAX_RES_NUM);
	memset((void *)&get_src_in_sw_stp_info, 0, sizeof(STRIPE_PARAM));
	memset((void *)&in_hw_col_info, 0, sizeof(VPE_HW_IN_COL_PARAM));
	memset((void *)get_sca_in_stp_info, 0, sizeof(VPE_SCA_SRC_IN_STRIPE_PARAM) * VPE_MAX_RES_NUM);
	//memset((void*)&vpe_res_en_info, 0, sizeof(VPE_RES_EN_PARAM));


	if (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCE_EN) {
		vpe_user_in_out_img_param.vpe_dce_en = ENABLE;
	} else {
		vpe_user_in_out_img_param.vpe_dce_en = DISABLE;
	}

	if((p_vpe_info->dce_param.dc_scenes_sel == KDRV_VPE_DCS_DEFAULT)|| (p_vpe_info->dce_param.dc_scenes_sel == KDRV_VPE_DCS_CEIL_FLOOR)){
		vpe_user_in_out_img_param.vpe_dce_stripe = 1;
	}else{
		vpe_user_in_out_img_param.vpe_dce_stripe = 0;
	}

	vpe_user_in_out_img_param.vpe_in_param.src_width = p_vpe_info->glb_img_info.src_width;
	vpe_user_in_out_img_param.vpe_in_param.src_height = p_vpe_info->glb_img_info.src_height;

	//vpe_user_in_out_img_param.vpe_in_param.dce_width = p_vpe_info->glb_img_info.src_in_w;
	//vpe_user_in_out_img_param.vpe_in_param.dce_height = p_vpe_info->glb_img_info.src_in_h;
	//vpe_user_in_out_img_param.vpe_in_param.dce_lens_radius = p_vpe_info->dce_param.lens_radius;

	vpe_user_in_out_img_param.vpe_in_param.proc_x_pos = p_vpe_info->glb_img_info.src_in_x;
	vpe_user_in_out_img_param.vpe_in_param.proc_y_pos = p_vpe_info->glb_img_info.src_in_y;
	//vpe_user_in_out_img_param.vpe_in_param.proc_width = p_vpe_info->glb_img_info.src_in_w;
	//vpe_user_in_out_img_param.vpe_in_param.proc_height = p_vpe_info->glb_img_info.src_in_h;



	if (vpe_user_in_out_img_param.vpe_dce_en == ENABLE) {
		vpe_user_in_out_img_param.vpe_in_param.dce_width = p_vpe_info->dce_param.dewarp_in_width;
		vpe_user_in_out_img_param.vpe_in_param.dce_height = p_vpe_info->dce_param.dewarp_in_height;
		vpe_user_in_out_img_param.vpe_in_param.dce_lens_radius = p_vpe_info->dce_param.lens_radius;
		vpe_user_in_out_img_param.vpe_in_param.proc_width = p_vpe_info->dce_param.dewarp_in_width;
		vpe_user_in_out_img_param.vpe_in_param.proc_height = p_vpe_info->dce_param.dewarp_in_height;

		if (((p_vpe_info->dce_param.dce_2d_lut_en == 1)&& (!(p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN)))||((p_vpe_info->dce_param.dce_2d_lut_en == 0)&& (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN))){
			if ((p_vpe_info->dce_param.dewarp_out_width != 0) && (p_vpe_info->dce_param.dewarp_out_height != 0)) {
				//set new dce out size
				vpe_user_in_out_img_param.vpe_in_param.proc_width = p_vpe_info->dce_param.dewarp_out_width;
				vpe_user_in_out_img_param.vpe_in_param.proc_height = p_vpe_info->dce_param.dewarp_out_height;
			}
		} else {
			if ((p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90) ||
			   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270) ||
			   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90_H_FLIP) ||
			   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270_H_FLIP)) {
				vpe_user_in_out_img_param.vpe_in_param.proc_width = p_vpe_info->dce_param.dewarp_in_height;
				vpe_user_in_out_img_param.vpe_in_param.proc_height = p_vpe_info->dce_param.dewarp_in_width;
			}
		}

		if(vpe_user_in_out_img_param.vpe_in_param.dce_width > vpe_user_in_out_img_param.vpe_in_param.src_width){
			DBG_ERR("vpe: src_in_width(dewarp_in_width) > src_width !\n");
			return -1;
		}
	}else{
		vpe_user_in_out_img_param.vpe_in_param.dce_width = p_vpe_info->glb_img_info.src_in_w;
		vpe_user_in_out_img_param.vpe_in_param.dce_height = p_vpe_info->glb_img_info.src_in_h;
		vpe_user_in_out_img_param.vpe_in_param.dce_lens_radius = p_vpe_info->dce_param.lens_radius;
		vpe_user_in_out_img_param.vpe_in_param.proc_width = p_vpe_info->glb_img_info.src_in_w;
		vpe_user_in_out_img_param.vpe_in_param.proc_height = p_vpe_info->glb_img_info.src_in_h;

		if(vpe_user_in_out_img_param.vpe_in_param.proc_width > vpe_user_in_out_img_param.vpe_in_param.src_width){
			DBG_ERR("vpe: src_in_width(dewarp_in_width) > src_width !\n");
			return -1;
		}
	}


	for (res_idx = 0; res_idx < VPE_MAX_RES; res_idx++) {
		vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_en = 0;
		if (p_vpe_info->out_img_info[res_idx].out_dup_num) {
			vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_en = 1;

			vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_comps_en = 0;
			if ((res_idx == 0) || (res_idx == 1)) {
				if (p_vpe_info->out_img_info[res_idx].out_dup_info.des_type == KDRV_VPE_DES_TYPE_YCC_YUV420) {
					vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_comps_en = 1;
				}
			}
			if(p_vpe_info->out_img_info[res_idx].res_dim_info.sca_crop_w * p_vpe_info->out_img_info[res_idx].res_dim_info.sca_crop_h > 0){
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_x_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.sca_crop_x;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_y_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.sca_crop_y;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_width  = p_vpe_info->out_img_info[res_idx].res_dim_info.sca_crop_w;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height = p_vpe_info->out_img_info[res_idx].res_dim_info.sca_crop_h;
			}else{
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_x_pos  = 0;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_y_pos  = 0;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_width  = p_vpe_info->glb_img_info.src_in_w;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height = p_vpe_info->glb_img_info.src_in_h;


				if (vpe_user_in_out_img_param.vpe_dce_en) {
					vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_width  = p_vpe_info->dce_param.dewarp_in_width;
					vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height = p_vpe_info->dce_param.dewarp_in_height;

					if (((p_vpe_info->dce_param.dce_2d_lut_en == 1)&& (!(p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN)))||((p_vpe_info->dce_param.dce_2d_lut_en == 0)&& (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN))){
						if ((p_vpe_info->dce_param.dewarp_out_width != 0) && (p_vpe_info->dce_param.dewarp_out_height != 0)) {
							//set new dce out size
							vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_width = p_vpe_info->dce_param.dewarp_out_width;
							vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height = p_vpe_info->dce_param.dewarp_out_height;
						}
					} else {
						if ((p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90) ||
						   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270) ||
						   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90_H_FLIP) ||
						   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270_H_FLIP)) {
							vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_width  = p_vpe_info->dce_param.dewarp_in_height;
							vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height = p_vpe_info->dce_param.dewarp_in_width;
						}
					}
				}
			}



			sca_area = p_vpe_info->out_img_info[res_idx].res_dim_info.sca_width * p_vpe_info->out_img_info[res_idx].res_dim_info.sca_height;
			sca_out_crop_area = p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_w * p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_h;


			if ((sca_area > 0) && (sca_out_crop_area > 0)) {
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_width	= p_vpe_info->out_img_info[res_idx].res_dim_info.sca_width;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_height = p_vpe_info->out_img_info[res_idx].res_dim_info.sca_height;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_x_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_x;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_y_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_y;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_width  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_w;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_height = p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_h;

			} else if ((sca_area == 0) && (sca_out_crop_area == 0)) {
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_width	= p_vpe_info->out_img_info[res_idx].res_dim_info.des_rlt_w;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_height = p_vpe_info->out_img_info[res_idx].res_dim_info.des_rlt_h;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_x_pos  = 0;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_y_pos  = 0;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_width  = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_width;
				vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_height = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_height;

			} else {
		        DBG_ERR("sca(%d %d) sca_out_crop(%d %d %d %d) size=0 err\n", p_vpe_info->out_img_info[res_idx].res_dim_info.sca_width, p_vpe_info->out_img_info[res_idx].res_dim_info.sca_height,
								p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_x, p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_y, p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_w, p_vpe_info->out_img_info[res_idx].res_dim_info.des_crop_out_h);
				return -1;
			}


			vpe_user_in_out_img_param.vpe_out_param[res_idx].rlt_x_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_rlt_x;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].rlt_y_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_rlt_y;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].rlt_width  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_rlt_w;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].rlt_height = p_vpe_info->out_img_info[res_idx].res_dim_info.des_rlt_h;

			vpe_user_in_out_img_param.vpe_out_param[res_idx].out_x_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_out_x;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].out_y_pos  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_out_y;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].out_width  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_out_w;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].out_height = p_vpe_info->out_img_info[res_idx].res_dim_info.des_out_h;

			vpe_user_in_out_img_param.vpe_out_param[res_idx].des_width  = p_vpe_info->out_img_info[res_idx].res_dim_info.des_width;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].des_height = p_vpe_info->out_img_info[res_idx].res_dim_info.des_height;


			vpe_user_in_out_img_param.vpe_out_param[res_idx].pip_x_pos = 0;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].pip_y_pos = 0;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].pip_width = 0;
			vpe_user_in_out_img_param.vpe_out_param[res_idx].pip_height = 0;

			vpe_user_in_out_img_param.vpe_sca_factor_param[res_idx].sca_factor_h = (vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_width - 1) * 65536 / (vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_width - 1);
			vpe_user_in_out_img_param.vpe_sca_factor_param[res_idx].sca_factor_v = (vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height - 1) * 65536 / (vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_height - 1);
		}
	}


	scl_info[0].in_size_h = vpe_user_in_out_img_param.vpe_in_param.proc_width;
	scl_info[0].in_size_v = vpe_user_in_out_img_param.vpe_in_param.proc_height;

	scl_info[3] = scl_info[2] = scl_info[1] = scl_info[0];

	scl_info[0].sca_en = vpe_user_in_out_img_param.vpe_out_param[0].sca_en;
	scl_info[0].out_size_h = vpe_user_in_out_img_param.vpe_out_param[0].sca_width;
	scl_info[0].out_size_v = vpe_user_in_out_img_param.vpe_out_param[0].sca_height;

	scl_info[1].sca_en = vpe_user_in_out_img_param.vpe_out_param[1].sca_en;
	scl_info[1].out_size_h = vpe_user_in_out_img_param.vpe_out_param[1].sca_width;
	scl_info[1].out_size_v = vpe_user_in_out_img_param.vpe_out_param[1].sca_height;

	scl_info[2].sca_en = vpe_user_in_out_img_param.vpe_out_param[2].sca_en;
	scl_info[2].out_size_h = vpe_user_in_out_img_param.vpe_out_param[2].sca_width;
	scl_info[2].out_size_v = vpe_user_in_out_img_param.vpe_out_param[2].sca_height;

	scl_info[3].sca_en = vpe_user_in_out_img_param.vpe_out_param[3].sca_en;
	scl_info[3].out_size_h = vpe_user_in_out_img_param.vpe_out_param[3].sca_width;
	scl_info[3].out_size_v = vpe_user_in_out_img_param.vpe_out_param[3].sca_height;

	//vpe_cal_src_in_stripe(scl_info, &get_src_in_sw_stp_info);

	if (vpe_cal_src_in_stripe_v3(&vpe_user_in_out_img_param, &get_src_in_sw_stp_info) != 0) {
		return -1;
	}

	vpe_get_in_column_info(&vpe_user_in_out_img_param, &get_src_in_sw_stp_info, &in_hw_col_info);

	for (res_idx = 0; res_idx < VPE_MAX_RES; res_idx++) {
		//vpe_res_en_info.res_en[res_idx] = 0;

		if (scl_info[res_idx].sca_en == 1) {
			//vpe_res_en_info.res_en[res_idx] = 1;
			//vpe_res_en_info.res_out_size[res_idx] = (vpe_user_in_out_img_param.vpe_out_param[res_idx].out_width * vpe_user_in_out_img_param.vpe_out_param[res_idx].out_height * 3 / 2);

			if (vpe_cal_sca_in_stripe(res_idx, &vpe_user_in_out_img_param, &get_src_in_sw_stp_info, &get_sca_in_stp_info[res_idx]) != 0) {
				return -1;
			}

			if (vpe_get_out_column_info(res_idx, &vpe_user_in_out_img_param, &get_sca_in_stp_info[res_idx]) != 0) {
				return -1;
			}

			vpe_refine_out_column_info(&get_sca_in_stp_info[res_idx]);

			if (vpe_get_scale_out_crop_info(res_idx, &vpe_user_in_out_img_param, &get_sca_in_stp_info[res_idx]) != 0) {
				return -1;
			}
		}

	}

	vpe_refine_in_column_info(&vpe_user_in_out_img_param, &in_hw_col_info);

	//------------------------------------------------------------------------
	// set to drv interface

	p_drv_cfg->glo_ctl.col_num = in_hw_col_info.col_num - 1;

	/// set to global size parameters
	p_drv_cfg->glo_sz.src_width = p_vpe_info->glb_img_info.src_width;
	p_drv_cfg->glo_sz.src_height = p_vpe_info->glb_img_info.src_height;
	//p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->glb_img_info.src_in_w;
	//p_drv_cfg->glo_sz.proc_height = p_vpe_info->glb_img_info.src_in_h;
	p_drv_cfg->glo_sz.proc_y_start = p_vpe_info->glb_img_info.src_in_y;

	if (p_drv_cfg->glo_ctl.dce_en) {
		p_drv_cfg->glo_sz.dce_width = p_vpe_info->dce_param.dewarp_in_width;
		p_drv_cfg->glo_sz.dce_height = p_vpe_info->dce_param.dewarp_in_height;
		p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->dce_param.dewarp_in_width;
		p_drv_cfg->glo_sz.proc_height = p_vpe_info->dce_param.dewarp_in_height;

		if (((p_vpe_info->dce_param.dce_2d_lut_en == 1)&& (!(p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN)))||((p_vpe_info->dce_param.dce_2d_lut_en == 0)&& (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN))){
			if ((p_vpe_info->dce_param.dewarp_out_width != 0) && (p_vpe_info->dce_param.dewarp_out_height != 0)) {
				//set new dce out size
				p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->dce_param.dewarp_out_width;
				p_drv_cfg->glo_sz.proc_height = p_vpe_info->dce_param.dewarp_out_height;
			}
		} else {
			if ((p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90) ||
			   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270) ||
			   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90_H_FLIP) ||
			   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270_H_FLIP)) {
				p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->glb_img_info.src_in_h;
				p_drv_cfg->glo_sz.proc_height = p_vpe_info->glb_img_info.src_in_w;
			}
		}
	} else{
		p_drv_cfg->glo_sz.dce_width = p_vpe_info->glb_img_info.src_in_w;
		p_drv_cfg->glo_sz.dce_height = p_vpe_info->glb_img_info.src_in_h;
		p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->glb_img_info.src_in_w;
		p_drv_cfg->glo_sz.proc_height = p_vpe_info->glb_img_info.src_in_h;
	}


	// set to in source column parameters
	for (col_idx = 0; col_idx < in_hw_col_info.col_num; col_idx++) {
		p_drv_cfg->col_sz[col_idx].proc_width = in_hw_col_info.in_col_info[col_idx].proc_width;
		p_drv_cfg->col_sz[col_idx].proc_x_start = in_hw_col_info.in_col_info[col_idx].proc_x_start;
		p_drv_cfg->col_sz[col_idx].col_x_start = in_hw_col_info.in_col_info[col_idx].col_x_start;
	}


	for (res_idx = 0; res_idx < VPE_MAX_RES; res_idx++) {

		p_drv_cfg->res[res_idx].res_ctl.sca_en = 0;

		if (vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_en == 1) {
			p_drv_cfg->res[res_idx].res_ctl.sca_en = 1;

			p_drv_cfg->res[res_idx].res_ctl.tc_en = 1;
			p_drv_cfg->res[res_idx].res_ctl.sca_bypass_en = 0;
			p_drv_cfg->res[res_idx].res_ctl.sca_crop_en = 1;
			p_drv_cfg->res[res_idx].res_ctl.sca_chroma_sel = 1;
			p_drv_cfg->res[res_idx].res_ctl.des_yuv420 = 1;
			p_drv_cfg->res[res_idx].res_ctl.out_bg_sel = p_vpe_info->out_img_info[res_idx].out_bg_sel;
			p_drv_cfg->res[res_idx].res_ctl.des_dp0_chrw = 1;
			p_drv_cfg->res[res_idx].res_ctl.des_dp0_format = 0;
			p_drv_cfg->res[res_idx].res_ctl.des_drt = p_vpe_info->out_img_info[res_idx].res_des_drt;
			p_drv_cfg->res[res_idx].res_ctl.des_uv_swap = p_vpe_info->out_img_info[res_idx].res_uv_swap;


			p_drv_cfg->res[res_idx].res_size.sca_height = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_height;
			p_drv_cfg->res[res_idx].res_size.des_width = vpe_user_in_out_img_param.vpe_out_param[res_idx].des_width;
			p_drv_cfg->res[res_idx].res_size.des_height = vpe_user_in_out_img_param.vpe_out_param[res_idx].des_height;
			p_drv_cfg->res[res_idx].res_size.out_y_start = vpe_user_in_out_img_param.vpe_out_param[res_idx].out_y_pos;
			p_drv_cfg->res[res_idx].res_size.out_height = vpe_user_in_out_img_param.vpe_out_param[res_idx].out_height;
			p_drv_cfg->res[res_idx].res_size.rlt_y_start = vpe_user_in_out_img_param.vpe_out_param[res_idx].rlt_y_pos;
			p_drv_cfg->res[res_idx].res_size.rlt_height = vpe_user_in_out_img_param.vpe_out_param[res_idx].rlt_height;
			p_drv_cfg->res[res_idx].res_size.pip_y_start = 0;
			p_drv_cfg->res[res_idx].res_size.pip_height = 0;

			p_drv_cfg->res[res_idx].res_sca_crop.sca_crop_y_start = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_y_pos;
			p_drv_cfg->res[res_idx].res_sca_crop.sca_crop_height = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_proc_crop_height;

			p_drv_cfg->res[res_idx].res_tc.tc_crop_y_start = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_y_pos;
			p_drv_cfg->res[res_idx].res_tc.tc_crop_height  = vpe_user_in_out_img_param.vpe_out_param[res_idx].sca_tag_crop_height;

			p_drv_cfg->res[res_idx].res_sca.sca_hstep = get_sca_in_stp_info[res_idx].sca_factor_h;
			p_drv_cfg->res[res_idx].res_sca.sca_vstep = get_sca_in_stp_info[res_idx].sca_factor_v;

			p_drv_cfg->res[res_idx].res_sca.sca_hsca_divisor = get_sca_in_stp_info[res_idx].sca_hsca_div;
			p_drv_cfg->res[res_idx].res_sca.sca_vsca_divisor = get_sca_in_stp_info[res_idx].sca_vsca_div;
			p_drv_cfg->res[res_idx].res_sca.sca_havg_pxl_msk = get_sca_in_stp_info[res_idx].sca_havg_pxl_msk;
			p_drv_cfg->res[res_idx].res_sca.sca_vavg_pxl_msk = get_sca_in_stp_info[res_idx].sca_vavg_pxl_msk;

			if (p_vpe_info->out_img_info[res_idx].res_sca_info.sca_iq_en) {
				p_drv_cfg->res[res_idx].res_sca.sca_iq_en = 1;
				p_drv_cfg->res[res_idx].res_sca.sca_y_luma_algo_en = p_vpe_info->out_img_info[res_idx].res_sca_info.sca_y_luma_algo_en;
				p_drv_cfg->res[res_idx].res_sca.sca_x_luma_algo_en = p_vpe_info->out_img_info[res_idx].res_sca_info.sca_x_luma_algo_en;
				p_drv_cfg->res[res_idx].res_sca.sca_y_chroma_algo_en = p_vpe_info->out_img_info[res_idx].res_sca_info.sca_y_chroma_algo_en;
				p_drv_cfg->res[res_idx].res_sca.sca_x_chroma_algo_en = p_vpe_info->out_img_info[res_idx].res_sca_info.sca_x_chroma_algo_en;
				p_drv_cfg->res[res_idx].res_sca.sca_map_sel = p_vpe_info->out_img_info[res_idx].res_sca_info.sca_map_sel;

				for (j = 0; j < 4; j++) {
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[j] = vpe_int_to_2comp(p_vpe_info->out_img_info[res_idx].res_sca_info.sca_ceff_h[j], 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[j] = vpe_int_to_2comp(p_vpe_info->out_img_info[res_idx].res_sca_info.sca_ceff_v[j], 10);
				}
			} else {
				p_drv_cfg->res[res_idx].res_sca.sca_iq_en = 0;

				p_drv_cfg->res[res_idx].res_sca.sca_map_sel = 1;
				p_drv_cfg->res[res_idx].res_sca.sca_y_luma_algo_en = 3;
				p_drv_cfg->res[res_idx].res_sca.sca_x_luma_algo_en = 3;
				p_drv_cfg->res[res_idx].res_sca.sca_y_chroma_algo_en = 2;
				p_drv_cfg->res[res_idx].res_sca.sca_x_chroma_algo_en = 2;

                #if 0
				if (p_drv_cfg->res[res_idx].res_sca.sca_hstep <= 1024) {
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(0, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(0, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(0, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(64, 10);
				} else if (p_drv_cfg->res[res_idx].res_sca.sca_hstep <= 2048) {
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(-5, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(-2, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(21, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(36, 10);
				} else if (p_drv_cfg->res[res_idx].res_sca.sca_hstep <= 3072) {
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(-3, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(4, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(18, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(26, 10);
				} else if (p_drv_cfg->res[res_idx].res_sca.sca_hstep <= 4096) {
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(-1, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(7, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(16, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(20, 10);
				} else {
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(4, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(9, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(12, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(14, 10);
				}

				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(0, 10);
				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(0, 10);
				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(0, 10);
				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(64, 10);
				#else
				vpe_sca_get_filter_coefs(p_drv_cfg->res[res_idx].res_sca.sca_hstep, get_sca_filter_coefs);

				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[0] = vpe_int_to_2comp(get_sca_filter_coefs[0], 10);
				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[1] = vpe_int_to_2comp(get_sca_filter_coefs[1], 10);
				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[2] = vpe_int_to_2comp(get_sca_filter_coefs[2], 10);
				p_drv_cfg->res[res_idx].res_sca.coeffHorizontal[3] = vpe_int_to_2comp(get_sca_filter_coefs[3], 10);
				#endif

				#if 0
				if (p_drv_cfg->res[res_idx].res_sca.sca_vstep <= 1024) {
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(0, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(0, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(0, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(64, 10);
				} else if (p_drv_cfg->res[res_idx].res_sca.sca_vstep <= 2048) {
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(-5, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(-2, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(21, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(36, 10);
				} else if (p_drv_cfg->res[res_idx].res_sca.sca_vstep <= 3072) {
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(-3, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(4, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(18, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(26, 10);
				} else if (p_drv_cfg->res[res_idx].res_sca.sca_vstep <= 4096) {
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(-1, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(7, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(16, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(20, 10);
				} else {
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(4, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(9, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(12, 10);
					p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(14, 10);
				}

				p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(0, 10);
    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(0, 10);
    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(0, 10);
    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(64, 10);
    			#else
    			vpe_sca_get_filter_coefs(p_drv_cfg->res[res_idx].res_sca.sca_vstep, get_sca_filter_coefs);

    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[0] = vpe_int_to_2comp(get_sca_filter_coefs[0], 10);
    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[1] = vpe_int_to_2comp(get_sca_filter_coefs[1], 10);
    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[2] = vpe_int_to_2comp(get_sca_filter_coefs[2], 10);
    			p_drv_cfg->res[res_idx].res_sca.coeffVertical[3] = vpe_int_to_2comp(get_sca_filter_coefs[3], 10);
    			#endif
			}
		


			for (col_idx = 0; col_idx < in_hw_col_info.col_num; col_idx++) {
				//DBG_DUMP("**res_idx: %d,  col_idx: %d\r\n", res_idx, col_idx);

				p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_crop_x_start = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].sca_in_crop_pos_x;
				p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_crop_width = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].sca_in_crop_size_h;

				p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_width = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].sca_out_size_h;

				p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_hstep_oft = vpe_int_to_2comp(get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].vpe_scl_h_ftr_init, 15);

				p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_x_start = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].sca_out_crop_pos_x;
				p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_width = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].sca_out_crop_size_h;

				p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_skip = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].skip_en;


				p_drv_cfg->res[res_idx].res_col_size[col_idx].rlt_x_start = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].rlt_pos_x;
				p_drv_cfg->res[res_idx].res_col_size[col_idx].rlt_width = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].rlt_size_h;

				p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].out_pos_x;
				p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width = get_sca_in_stp_info[res_idx].sca_stp_info[col_idx].out_size_h;

				p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_x_start = 0;
				p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_width = 0;

				//DBG_DUMP("sca_crop_x_start: %d,  sca_crop_width: %d,  sca_width: %d\r\n", p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_crop_x_start, p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_crop_width, p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_width);
				//DBG_DUMP("tc_crop_x_start: %d,  tc_crop_width: %d,  tc_crop_skip: %d\r\n", p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_x_start, p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_width, p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_skip);
				//DBG_DUMP("sca_hstep_oft: %d\r\n", p_drv_cfg->res[res_idx].res_col_size[col_idx].sca_hstep_oft);
				//DBG_DUMP("out_x_start: %d,  out_width: %d,  rlt_width: %d\r\n", p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start, p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width, p_drv_cfg->res[res_idx].res_col_size[col_idx].rlt_width);

			}


			//For Pip
            if (p_drv_cfg->glo_ctl.col_num && (p_vpe_info->out_img_info[res_idx].res_dim_info.hole_w && p_vpe_info->out_img_info[res_idx].res_dim_info.hole_h)) {

                //start_pip_out_x = p_drv_cfg->res[res_idx].res_col_size[0].out_x_start + p_vpe_info->out_img_info[res_idx].res_dim_info.hole_x;
                //dest_pip_out_x = p_drv_cfg->res[res_idx].res_col_size[0].out_x_start + p_vpe_info->out_img_info[res_idx].res_dim_info.hole_x + p_vpe_info->out_img_info[res_idx].res_dim_info.hole_w;

				for (col_idx = 0; col_idx < in_hw_col_info.col_num; col_idx++) {
					if (p_drv_cfg->res[res_idx].res_col_size[col_idx].tc_crop_skip ==0) {
						start_pip_out_x = p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start + p_vpe_info->out_img_info[res_idx].res_dim_info.hole_x;
						dest_pip_out_x = p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start + p_vpe_info->out_img_info[res_idx].res_dim_info.hole_x + p_vpe_info->out_img_info[res_idx].res_dim_info.hole_w;
						break;
					}
				}

                for (col_idx = 0; col_idx < in_hw_col_info.col_num; col_idx++) {

                    if ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start <= start_pip_out_x) && ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start + p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width) > start_pip_out_x)) {
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_x_start = start_pip_out_x - p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start;
                        if ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start + p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width) >= dest_pip_out_x) {
                            p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_width = p_vpe_info->out_img_info[res_idx].res_dim_info.hole_w;
                        } else {
                            p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_width = (p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width - p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_x_start);
                        }
                    } else if ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start >= start_pip_out_x) && ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start + p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width) <= dest_pip_out_x)) {
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_x_start = 0;
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_width = p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width;
                    } else if ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start < dest_pip_out_x) && ((p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start + p_drv_cfg->res[res_idx].res_col_size[col_idx].out_width) >= dest_pip_out_x)) {
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_x_start = 0;
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_width = dest_pip_out_x - p_drv_cfg->res[res_idx].res_col_size[col_idx].out_x_start;
                    }
                }
                p_drv_cfg->res[res_idx].res_size.pip_height = p_vpe_info->out_img_info[res_idx].res_dim_info.hole_h;
                p_drv_cfg->res[res_idx].res_size.pip_y_start= p_vpe_info->out_img_info[res_idx].res_dim_info.hole_y;
            } else {
                if (p_vpe_info->out_img_info[res_idx].res_dim_info.hole_w && p_vpe_info->out_img_info[res_idx].res_dim_info.hole_h) {
                    p_drv_cfg->res[res_idx].res_col_size[0].pip_width = p_vpe_info->out_img_info[res_idx].res_dim_info.hole_w;
                    p_drv_cfg->res[res_idx].res_size.pip_height = p_vpe_info->out_img_info[res_idx].res_dim_info.hole_h;
                    p_drv_cfg->res[res_idx].res_col_size[0].pip_x_start= p_vpe_info->out_img_info[res_idx].res_dim_info.hole_x;
                    p_drv_cfg->res[res_idx].res_size.pip_y_start= p_vpe_info->out_img_info[res_idx].res_dim_info.hole_y;
                } else {
                    for (col_idx = 0; col_idx < in_hw_col_info.col_num; col_idx++) {
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_width = 0;
                        p_drv_cfg->res[res_idx].res_col_size[col_idx].pip_x_start= 0;
                    }
                    p_drv_cfg->res[res_idx].res_size.pip_height = 0;
                    p_drv_cfg->res[res_idx].res_size.pip_y_start= 0;
                }
            }


		}
	}

	return 0;
}


