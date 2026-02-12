#if defined(__LINUX)
#elif defined(__FREERTOS)
#include <string.h>
#endif

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_emode.h"

#include "h26xenc_int.h"
#include "h264enc_api.h"
#include "h265enc_api.h"

#include "vemd_cfg.h"
#include "vemd_api.h"

typedef struct _cfg_line_t_{
	int size;
	char buf[128];
} cfg_line_t;

typedef struct _attr_t_{
	char name[20];
	char num;
	void *target;
	unsigned char size;
} attr_t;

static void read_line(char *start, int size, cfg_line_t *line)
{
	for (line->size = 0; line->size < size; line->size++) {
		if (start[line->size] == '#')
			break;
	}

	if (size > 128)
		DBG_ERR("valid data (%d) > 64\r\n", size);
	else
		memcpy(line->buf, start, line->size);

	//DBG_DUMP("(%d) %s\r\n", line->size, line->buf);
}

static int set_emode_enc_id(cfg_line_t *line, void *pstCfg)
{
	if (!strncmp(line->buf, "id=", strlen("id=")))
		sscanf(line->buf + strlen("id="), "%d", &g_vemd_info.enc_id);

	if (vemd_get_func_info(&g_vemd_info.func) != 0) {
		DBG_DUMP("enc_id(%d) not enable, enable update init mode\r\n", g_vemd_info.enc_id);
	}

	return g_vemd_info.enc_id;
}

static int parse_slice_cfg(cfg_line_t *line, void *pstCfg)
{
	int i;
	H26XEncSliceSplit *p_slice = (H26XEncSliceSplit *)pstCfg;

	attr_t attr[] = {
		{"enable=",	 1, &p_slice->stCfg.bEnable, 4},
		{"row=", 	 1, &p_slice->stCfg.uiSliceRowNum, 4},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_gdr_cfg(cfg_line_t *line, void *pstCfg)
{
	int i;
	H26XEncGdr *p_gdr = (H26XEncGdr *)pstCfg;

	attr_t attr[] = {
		{"enable=",	 1, &p_gdr->stCfg.bEnable, 4},
		{"period=",	 1, &p_gdr->stCfg.usPeriod, 2},
		{"row_num=", 1, &p_gdr->stCfg.usNumber, 2},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_roi_cfg(cfg_line_t *line, void *pstCfg)
{
	H26XEncRoi *p_roi = (H26XEncRoi *)pstCfg;

	if (!strncmp(line->buf, "win=", strlen("win="))) {
		int j = 0, tmp;
		char *string = (char *)(line->buf + strlen("win="));
		char *token;
		H26XEncRoiCfg *p_cfg = NULL;
		for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
			sscanf(token, "%d", &tmp);

			if (j == 0)
				p_cfg = &p_roi->stCfg[tmp];
			else if (j == 1)
				p_cfg->bEnable = tmp;
			else if (j == 2)
				p_cfg->usCoord_X = tmp;
			else if (j == 3)
				p_cfg->usCoord_Y = tmp;
			else if (j == 4)
				p_cfg->usWidth = tmp;
			else if (j == 5)
				p_cfg->usHeight = tmp;
			else if (j == 6)
				p_cfg->cQP = tmp;
			else if (j == 7)
				p_cfg->ucMode = tmp;
			else
				break;

			j++;
		}
	}

	p_roi->ucNumber++;

	return 1;
}

static int parse_rrc_cfg(cfg_line_t *line, void *pstCfg)
{
	int i;
	int tmp;
	H26XEncRowRc *p_rrc = (H26XEncRowRc *)pstCfg;

	attr_t attr[] = {
		{"enable=",	 			1, &p_rrc->stCfg.bEnable, 4},
		{"scale=",				1, &p_rrc->stCfg.ucScale, 1},
		{"init_coeff=",			1, &p_rrc->stCfg.uiInitFrmCoeff, 4},
		{"i_pred_wt=",			1, &p_rrc->stCfg.ucPredWt[0], 1},
		{"i_qp_range=",			1, &p_rrc->stCfg.ucQPRange[0], 1},
		{"i_qp_step=",			1, &p_rrc->stCfg.ucQPStep[0], 1},
		{"i_qp_min=",			1, &p_rrc->stCfg.ucMinQP[0], 1},
		{"i_qp_max=",			1, &p_rrc->stCfg.ucMaxQP[0], 1},
		{"i_prev_bits_bias=",	1, &p_rrc->iPrevPicBitsBias[0], 4},
		{"p_pred_wt=",			1, &p_rrc->stCfg.ucPredWt[1], 1},
		{"p_qp_range=",			1, &p_rrc->stCfg.ucQPRange[1], 1},
		{"p_qp_step=",			1, &p_rrc->stCfg.ucQPStep[1], 1},
		{"p_qp_min=",			1, &p_rrc->stCfg.ucMinQP[1], 1},
		{"p_qp_max=",			1, &p_rrc->stCfg.ucMaxQP[1], 1},
		{"p_prev_bits_bias=",	1, &p_rrc->iPrevPicBitsBias[0], 4},
		{"min_cost_th=",		1, &p_rrc->ucMinCostTh, 1},
		{"init_qp=",			1, &p_rrc->ucInitQp, 1},
		{"plan_stop=",			1, &p_rrc->uiPlannedStop, 4},
		{"plan_top=",			1, &p_rrc->uiPlannedTop, 4},
		{"plan_bot=",			1, &p_rrc->uiPlannedBot, 4},
		{"pred_bits=",			1, &p_rrc->uiPredBits, 4},
	};

	p_rrc->ucInitQp = 0;
	p_rrc->uiPlannedStop = 0;
	p_rrc->uiPlannedTop = 0;
	p_rrc->uiPlannedBot = 0;
	p_rrc->iPrevPicBitsBias[0] = 0;
	p_rrc->iPrevPicBitsBias[1] = 0;
	p_rrc->uiPredBits = 0;

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", &tmp);
			if (attr[i].size == 1)
				*((UINT8 *)attr[i].target) = (tmp & 0xff);
			else
				*((int *)attr[i].target) = tmp;

			break;
		}
	}

	return 1;
}

static int parse_aq_cfg(cfg_line_t *line, void *pstCfg)
{
	int i, tmp;
	H26XEncAq *p_aq = (H26XEncAq *)pstCfg;

	attr_t attr[] = {
		{"enable=",	 1, &p_aq->stCfg.bEnable, 4},
		{"ic2=", 	 1, &p_aq->stCfg.ucIC2, 1},
		{"i_str=",   1, &p_aq->stCfg.ucIStr, 1},
		{"p_str=",   1, &p_aq->stCfg.ucPStr, 1},
		{"max_dqp=", 1, &p_aq->stCfg.cMaxDQp, 1},
		{"min_dqp=", 1, &p_aq->stCfg.cMinDQp, 1},
		{"aslog2=",  1, &p_aq->stCfg.ucAslog2, 1},
		{"depth=",   1, &p_aq->stCfg.ucDepth, 1},
		{"plan_x=",  1, &p_aq->stCfg.ucPlaneX, 1},
		{"plan_y=",  1, &p_aq->stCfg.ucPlaneY, 1},
		{"th=",		30, &p_aq->stCfg.sTh, 2},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			if (!strcmp(attr[i].name, "th=")) {
				int j = 0;
				char *string = (char *)(line->buf + strlen(attr[i].name));
				char *token;
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					p_aq->stCfg.sTh[j++] = tmp;
				}
			}
			else {
				sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			}
			break;
		}
	}

	return 1;
}

static int parse_lpm_cfg(cfg_line_t *line, void *pstCfg)
{
	H26XEncLpm *p_lpm = (H26XEncLpm *)pstCfg;
	int i;

	attr_t attr[] = {
		{"enable=",		 1, &p_lpm->stCfg.bEnable, 4},
		{"rmd_sad_en=",	 1, &p_lpm->stCfg.ucRmdSadEn, 1},
		{"ime_stop_en=", 1, &p_lpm->stCfg.ucIMEStopEn, 1},
		{"ime_stop_th=", 1, &p_lpm->stCfg.ucIMEStopTh, 1},
		{"rdo_stop_en=", 1, &p_lpm->stCfg.ucRdoStopEn, 1},
		{"rdo_stop_th=", 1, &p_lpm->stCfg.ucRdoStopTh, 1},
		{"chrm_dm_en=",	 1, &p_lpm->stCfg.ucChrmDmEn, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_rnd_cfg(cfg_line_t *line, void *pstCfg)
{
	h26XEncRnd *p_rnd = (h26XEncRnd *)pstCfg;
	int i;

	attr_t attr[] = {
		{"enable=",	   1, &p_rnd->stCfg.bEnable, 4},
		{"rnd_seed=",  1, &p_rnd->stCfg.uiSeed, 4},
		{"rnd_range=", 1, &p_rnd->stCfg.ucRange, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_maq_cfg(cfg_line_t *line, void *pstCfg)
{
	H26XEncMotAq *p_maq = (H26XEncMotAq *)pstCfg;
	int i;

	attr_t attr[] = {
		{"mode=",	   		1, &p_maq->stCfg.ucMode, 4},
		{"8x8to16x16_th=",	1, &p_maq->stCfg.uc8x8to16x16Th, 1},
		{"dqp_roi_th=", 	1, &p_maq->stCfg.ucDqpRoiTh, 1},
		{"dqp=",	   		6, &p_maq->stCfg.cDqp, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			if (!strcmp(attr[i].name, "dqp=")) {
				int j = 0, tmp;
				char *string = (char *)(line->buf + strlen(attr[i].name));
				char *token;
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					p_maq->stCfg.cDqp[j++] = tmp;
				}
			}
			else
				sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));

			break;
		}
	}

	return 1;
}

static int parse_jnd_cfg(cfg_line_t *line, void *pstCfg)
{
	H26XEncJnd *p_jnd = (H26XEncJnd *)pstCfg;
	int i;

	attr_t attr[] = {
		{"enable=",	1, &p_jnd->stCfg.bEnable, 4},
		{"str=",  	1, &p_jnd->stCfg.ucStr, 1},
		{"level=", 	1, &p_jnd->stCfg.ucLevel, 1},
		{"th=", 	1, &p_jnd->stCfg.ucTh, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_rqp_cfg(cfg_line_t *line, void *pstCfg)
{
	H26XEncQpRelatedCfg *p_rqp = (H26XEncQpRelatedCfg *)pstCfg;
	int i;

	attr_t attr[] = {
		{"save_dqp=",	1, &p_rqp->ucSaveDeltaQp, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_var_cfg(cfg_line_t *line, void *pstCfg)
{
	H26XEncVar *p_var = (H26XEncVar *)pstCfg;
	int i;

	attr_t attr[] = {
		{"th=",				1, &p_var->stCfg.usThreshold, 2},
		{"avg_min=",		1, &p_var->stCfg.ucAVGMin, 1},
		{"avg_max=",		1, &p_var->stCfg.ucAVGMax, 1},
		{"delta=",			1, &p_var->stCfg.ucDelta, 1},
		{"i_range_delta=",	1, &p_var->stCfg.ucIRangeDelta, 1},
		{"p_range_delta=",	1, &p_var->stCfg.ucPRangeDelta, 1},
		{"merge=",			1, &p_var->stCfg.ucMerage, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

static int parse_fro_264_cfg(cfg_line_t *line, void *pstCfg)
{
	typedef struct _fro_264_attr_t_{
		char name[32];
		unsigned char idx0;
		unsigned char idx1;
	} fro_264_attr_t;

	fro_264_attr_t attr[] = {
		{"i_y4=", 			0, 0},
		{"i_y8=", 			0, 1},
		{"i_y4dc=", 		0, 2},
		{"i_c4=", 			0, 3},
		{"intra_p_y4=", 	1, 0},
		{"intra_p_y8=", 	1, 1},
		{"intra_p_y4dc=",	1, 2},
		{"intra_p_c4=", 	1, 3},
		{"inter_p_y4=", 	2, 0},
		{"inter_p_y8=", 	2, 1},
		{"inter_p_y4dc=", 	2, 2},
		{"inter_p_c4=", 	2, 3},
	};

	H264EncFroCfg *p_fro = (H264EncFroCfg *)pstCfg;

	if (!strncmp(line->buf, "enable=", strlen("enable="))) {
		sscanf(line->buf + strlen("enable="), "%d", &p_fro->bEnable);
	}
	else {
		int i;

		for (i = 0; i < (int)(sizeof(attr)/sizeof(fro_264_attr_t)); i++) {
			if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
				int j = 0, tmp;
				char *string = (char *)(line->buf + strlen(attr[i].name));
				char *token;
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					if (j == 0)
						p_fro->uiDC[attr[i].idx0][attr[i].idx1] = tmp;
					else if (j == 1)
						p_fro->ucAC[attr[i].idx0][attr[i].idx1] = (UINT8)(tmp & 0xff);
					else if (j == 2)
						p_fro->ucST[attr[i].idx0][attr[i].idx1] = (UINT8)(tmp & 0xff);
					else if (j == 3)
						p_fro->ucMX[attr[i].idx0][attr[i].idx1] = (UINT8)(tmp & 0xff);
					else
						break;

					j++;
				}
				break;
			}
		}
	}

	return 1;
}

static int parse_fro_265_cfg(cfg_line_t *line, void *pstCfg)
{
	typedef struct _fro_265_attr_t_{
		char name[32];
		unsigned char idx0;
		unsigned char idx1;
		unsigned char idx2;
	} fro_265_attr_t;

	fro_265_attr_t attr[] = {
		{"i_y32=", 			0, 0, 0},
		{"i_y16=", 			0, 0, 1},
		{"i_y8=", 			0, 0, 2},
		{"i_y4=", 			0, 0, 3},
		{"i_c16=", 			1, 0, 0},
		{"i_c8=", 			1, 0, 1},
		{"i_c4=",			1, 0, 2},
		{"intra_p_y16=",	0, 1, 1},
		{"intra_p_y8=", 	0, 1, 2},
		{"intra_p_y4=", 	0, 1, 3},
		{"intra_p_c8=", 	1, 1, 1},
		{"intra_p_c4=", 	1, 1, 2},
		{"inter_p_y32=", 	0, 2, 0},
		{"inter_p_y16=", 	0, 2, 1},
		{"inter_p_y8=", 	0, 2, 2},
		{"inter_p_c16=", 	1, 2, 0},
		{"inter_p_c8=", 	1, 2, 1},
		{"inter_p_c4=", 	1, 2, 2},
	};

	H265EncFroCfg *p_fro = (H265EncFroCfg *)pstCfg;

	if (!strncmp(line->buf, "enable=", strlen("enable="))) {
		sscanf(line->buf + strlen("enable="), "%d", &p_fro->bEnable);
	}
	else {
		int i;

		for (i = 0; i < (int)(sizeof(attr)/sizeof(fro_265_attr_t)); i++) {
			if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
				int j = 0, tmp;
				char *string = (char *)(line->buf + strlen(attr[i].name));
				char *token;
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					if (j == 0)
						p_fro->uiDC[attr[i].idx0][attr[i].idx1][attr[i].idx2] = tmp;
					else if (j == 1)
						p_fro->ucAC[attr[i].idx0][attr[i].idx1][attr[i].idx2] = (UINT8)(tmp & 0xff);
					else if (j == 2)
						p_fro->ucST[attr[i].idx0][attr[i].idx1][attr[i].idx2] = (UINT8)(tmp & 0xff);
					else if (j == 3)
						p_fro->ucMX[attr[i].idx0][attr[i].idx1][attr[i].idx2] = (UINT8)(tmp & 0xff);
					else
						break;

					j++;
				}
				break;
			}
		}
	}

	return 1;
}

static int parse_rdo_264_cfg(cfg_line_t *line, void *pstCfg)
{
	H264EncRdo *p_rdo = (H264EncRdo *)pstCfg;
	int i;

	attr_t attr[] = {
		{"enable=",		1, &p_rdo->bEnable, 4},
		{"i_slop=", 	1, &p_rdo->ucSlope[0], 1},
		{"p_slop=", 	1, &p_rdo->ucSlope[1], 1},
		{"inter_slop=",	1, &p_rdo->ucSlope[2], 1},
		{"i4_bias=", 	1, &p_rdo->ucI_Y4_CB, 1},
		{"i8_bias=", 	1, &p_rdo->ucI_Y8_CB, 1},
		{"i16_bias=", 	1, &p_rdo->ucI_Y16_CB, 1},
		{"i4_bias_p=", 	1, &p_rdo->stRdoCfg.ucI_Y4_CB_P, 1},
		{"i8_bias_p=", 	1, &p_rdo->stRdoCfg.ucI_Y8_CB_P, 1},
		{"i16_bias_p=",	1, &p_rdo->stRdoCfg.ucI_Y16_CB_P, 1},
		{"i16_tw_dc=", 	1, &p_rdo->ucI16_CT_DC, 1},
		{"i16_tw_h=", 	1, &p_rdo->ucI16_CT_H, 1},
		{"i16_tw_v=", 	1, &p_rdo->ucI16_CT_V, 1},
		{"i16_tw_p=", 	1, &p_rdo->ucI16_CT_P, 1},
		{"p4_bias=", 	1, &p_rdo->stRdoCfg.ucP_Y4_CB, 1},
		{"p8_bias=", 	1, &p_rdo->stRdoCfg.ucP_Y8_CB, 1},
		{"skip_bias=", 	1, &p_rdo->stRdoCfg.ucIP_CB_SKIP, 1},
		{"icm_tw_dc=", 	1, &p_rdo->ucIP_C_CT_DC, 1},
		{"icm_tw_h=", 	1, &p_rdo->ucIP_C_CT_H, 1},
		{"icm_tw_v=", 	1, &p_rdo->ucIP_C_CT_V, 1},
		{"icm_tw_p=", 	1, &p_rdo->ucIP_C_CT_P, 1},
		{"p_y_coeff_cost_th=", 	1, &p_rdo->stRdoCfg.ucP_Y_COEFF_COST_TH, 1},
		{"p_c_coeff_cost_th=", 	1, &p_rdo->stRdoCfg.ucP_C_COEFF_COST_TH, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			if ((i == 1) || (i == 2) || (i == 3)){
				int j = 0, tmp;
				char *string = (char *)(line->buf + strlen(attr[i].name));
				char *token;
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					p_rdo->ucSlope[i-1][j] = (UINT8)(tmp & 0xff);
				}
			}
			else {
				sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			}
			break;
		}
	}

	return 1;
}

static int parse_rdo_265_cfg(cfg_line_t *line, void *pstCfg)
{
	H265EncRdo *p_rdo = (H265EncRdo *)pstCfg;
	int i;

	attr_t attr[] = {
		{"enable=",				1, &p_rdo->bEnable, 4},
		{"rate_bias_i_32l=", 	1, &p_rdo->rate_bias_I_32L, 1},
		{"rate_bias_i_16l=", 	1, &p_rdo->rate_bias_I_16L, 1},
		{"rate_bias_i_8l=", 	1, &p_rdo->rate_bias_I_08L, 1},
		{"rate_bias_i_4l=", 	1, &p_rdo->rate_bias_I_04L, 1},
		{"rate_bias_i_16c=", 	1, &p_rdo->rate_bias_I_16C, 1},
		{"rate_bias_i_4c=", 	1, &p_rdo->rate_bias_I_04C, 1},
		{"rate_bias_ip_16l=", 	1, &p_rdo->rate_bias_IP_16L, 1},
		{"rate_bias_ip_8l=", 	1, &p_rdo->rate_bias_IP_08L, 1},
		{"rate_bias_ip_4l=", 	1, &p_rdo->rate_bias_IP_04L, 1},
		{"rate_bias_ip_8c=", 	1, &p_rdo->rate_bias_IP_08C, 1},
		{"rate_bias_ip_4c=", 	1, &p_rdo->rate_bias_IP_04C, 1},
		{"rate_bias_p_32l=", 	1, &p_rdo->rate_bias_P_32L, 1},
		{"rate_bias_p_16l=", 	1, &p_rdo->rate_bias_P_16L, 1},
		{"rate_bias_p_8l=", 	1, &p_rdo->rate_bias_P_08L, 1},
		{"rate_bias_p_16c=", 	1, &p_rdo->rate_bias_P_16C, 1},
		{"rate_bias_p_8c=", 	1, &p_rdo->rate_bias_P_08C, 1},
		{"rate_bias_p_4c=", 	1, &p_rdo->rate_bias_P_04C, 1},
		{"cost_bias_skip=", 	1, &p_rdo->stRdoCfg.cost_bias_skip, 1},
		{"cost_bias_merge=",	1, &p_rdo->stRdoCfg.cost_bias_merge, 1},
		{"gmp_i32=", 			1, &p_rdo->stRdoCfg.global_motion_penalty_I32, 1},
		{"gmp_i16=", 			1, &p_rdo->stRdoCfg.global_motion_penalty_I16, 1},
		{"gmp_i08=", 			1, &p_rdo->stRdoCfg.global_motion_penalty_I08, 1},
		{"gmp_i32_o=", 			1, &p_rdo->global_motion_penalty_I32O, 1},
		{"gmp_i16_o=", 			1, &p_rdo->global_motion_penalty_I16O, 1},
		{"gmp_i08_o=", 			1, &p_rdo->global_motion_penalty_I08O, 1},
		{"ime_64c=", 			1, &p_rdo->stRdoCfg.ime_scale_64c, 1},
		{"ime_64p=", 			1, &p_rdo->stRdoCfg.ime_scale_64p, 1},
		{"ime_32c=", 			1, &p_rdo->stRdoCfg.ime_scale_32c, 1},
		{"ime_32p=", 			1, &p_rdo->stRdoCfg.ime_scale_32p, 1},
		{"ime_16c=", 			1, &p_rdo->stRdoCfg.ime_scale_16c, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}

	return 1;
}

int vemd_parse_cfg(char *buf, size_t buf_size)
{
	typedef struct _entry_t_{
		char name[32];
		int  *enable;
		void *pstCfg;
		int  (*pfunc)(cfg_line_t *line, void *pstCfg);
	} entry_t;

	char *start = buf;
	entry_t entry[] = {
		{"[INFO]",		NULL, 					NULL, 							NULL},
		{"[ENC]",		&g_vemd_info.enc_id,	NULL, 							set_emode_enc_id},
		{"[SLICE]",		&g_vemd_info.slice_en,	&g_vemd_info.func.stSliceSplit,	parse_slice_cfg},
		{"[GDR]",		&g_vemd_info.gdr_en,	&g_vemd_info.func.stGdr,		parse_gdr_cfg},
		{"[ROI]",		&g_vemd_info.roi_en,	&g_vemd_info.func.stRoi,		parse_roi_cfg},
		{"[RRC]",		&g_vemd_info.rrc_en,	&g_vemd_info.func.stRowRc,		parse_rrc_cfg},
		{"[AQ]",		&g_vemd_info.aq_en,		&g_vemd_info.func.stAq,			parse_aq_cfg},
		{"[LPM]",		&g_vemd_info.lpm_en,	&g_vemd_info.func.stLpm,		parse_lpm_cfg},
		{"[RND]",		&g_vemd_info.rnd_en,	&g_vemd_info.func.stRnd,		parse_rnd_cfg},
		{"[MAQ]",		&g_vemd_info.maq_en,	&g_vemd_info.func.stMAq,		parse_maq_cfg},
		{"[JND]",		&g_vemd_info.jnd_en,	&g_vemd_info.func.stJnd,		parse_jnd_cfg},
		{"[RQP]",		&g_vemd_info.rqp_en,	&g_vemd_info.func.stRqp,		parse_rqp_cfg},
		{"[VAR]",		&g_vemd_info.var_en,	&g_vemd_info.func.stVar,		parse_var_cfg},
		{"[FRO_264]",	&g_vemd_info.fro_en,	&g_vemd_info.func.stFro.st264,	parse_fro_264_cfg},
		{"[FRO_265]",	&g_vemd_info.fro_en,	&g_vemd_info.func.stFro.st265,	parse_fro_265_cfg},
		{"[RDO_264]",	&g_vemd_info.rdo_en,	&g_vemd_info.func.stRdo.st264,	parse_rdo_264_cfg},
		{"[RDO_265]",	&g_vemd_info.rdo_en,	&g_vemd_info.func.stRdo.st265,	parse_rdo_265_cfg},
	};

	char *end;
	int entry_l0_size = sizeof(entry)/sizeof(entry_t);
	int i;
	int (*pfunc)(cfg_line_t *line, void *pstCfg) = NULL;

	memset(&g_vemd_info, 0, sizeof(vemd_info_t));

	do{
		cfg_line_t line = {0};

		end = strchr(start, '\n');
		if (end != start) {
			read_line(start, (end - start), &line);
			if (*start == '[') {
				pfunc = NULL;

				for (i = 0; i < entry_l0_size; i++) {
					if (!strncmp(line.buf, entry[i].name, strlen(entry[i].name))) {
						pfunc = entry[i].pfunc;
						break;
					}
				}
			}
			else {
				if (pfunc != NULL)
					*entry[i].enable = pfunc(&line, entry[i].pstCfg);
			}
		}
		start = end + 1;
	}while((size_t)(start - buf) < buf_size);

	g_vemd_info.enable = TRUE;

	return 0;
}

vemd_func_t g_stfunc;
int vemd_gen_cfg(char *buf, size_t buf_size)
{
	VENC_CH_INFO info;
	vemd_func_t *p_stfunc = &g_stfunc;

	int i;

	if (buf == NULL) {
		DBG_ERR("write buffer not ready\r\n");
		return -1;
	}

	if (vemd_get_func_info(p_stfunc) != 0) {
		DBG_ERR("get function info error\r\n");
		return -1;
	}

	kdrv_vdocdc_emode_get_enc_info(&info);

	memset(buf, 0, buf_size);

	sprintf(buf + strlen(buf), "[INFO]\n");
	sprintf(buf + strlen(buf), "drv_version=0x%08x\n", info.drv_ver);
	sprintf(buf + strlen(buf), "emode_version=0x%08x\n\n", info.emode_ver);

	sprintf(buf + strlen(buf), "[ENC]\n");
	sprintf(buf + strlen(buf), "id=%d\n", g_vemd_info.enc_id);
	sprintf(buf + strlen(buf), "codec=%d\n\n", p_stfunc->eCodecType);

	sprintf(buf + strlen(buf), "[AQ]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stAq.stCfg.bEnable);
	sprintf(buf + strlen(buf), "ic2=%d\n", p_stfunc->stAq.stCfg.ucIC2);
	sprintf(buf + strlen(buf), "i_str=%d\n", p_stfunc->stAq.stCfg.ucIStr);
	sprintf(buf + strlen(buf), "p_str=%d\n", p_stfunc->stAq.stCfg.ucPStr);
	sprintf(buf + strlen(buf), "max_dqp=%d\n", p_stfunc->stAq.stCfg.cMaxDQp);
	sprintf(buf + strlen(buf), "min_dqp=%d\n", p_stfunc->stAq.stCfg.cMinDQp);
	sprintf(buf + strlen(buf), "aslog2=%d\n", p_stfunc->stAq.stCfg.ucAslog2);
	sprintf(buf + strlen(buf), "depth=%d\n", p_stfunc->stAq.stCfg.ucDepth);
	sprintf(buf + strlen(buf), "plan_x=%d\n", p_stfunc->stAq.stCfg.ucPlaneX);
	sprintf(buf + strlen(buf), "plan_y=%d\n", p_stfunc->stAq.stCfg.ucPlaneY);
	sprintf(buf + strlen(buf), "th=");
	for (i = 0; i < 29; i++) {
		sprintf(buf + strlen(buf), "%d,", (int)p_stfunc->stAq.stCfg.sTh[i]);
	}
	sprintf(buf + strlen(buf), "%d\n\n", (int)p_stfunc->stAq.stCfg.sTh[i]);

	sprintf(buf + strlen(buf), "[SLICE]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stSliceSplit.stCfg.bEnable);
	sprintf(buf + strlen(buf), "row=%d\n\n", (int)p_stfunc->stSliceSplit.stCfg.uiSliceRowNum);

	sprintf(buf + strlen(buf), "[GDR]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stGdr.stCfg.bEnable);
	sprintf(buf + strlen(buf), "period=%d\n", p_stfunc->stGdr.stCfg.usPeriod);
	sprintf(buf + strlen(buf), "row_num=%d\n\n", p_stfunc->stGdr.stCfg.usNumber);

	sprintf(buf + strlen(buf), "[ROI]\n");
	for (i = 0; i < H26X_MAX_ROI_W; i++) {
		sprintf(buf + strlen(buf), "win=%d,%d,%d,%d,%d,%d,%d,%d\n",
			i, p_stfunc->stRoi.stCfg[i].bEnable, p_stfunc->stRoi.stCfg[i].usCoord_X, p_stfunc->stRoi.stCfg[i].usCoord_Y,
			p_stfunc->stRoi.stCfg[i].usWidth, p_stfunc->stRoi.stCfg[i].usHeight, p_stfunc->stRoi.stCfg[i].cQP, p_stfunc->stRoi.stCfg[i].ucMode);
	}
	sprintf(buf + strlen(buf), "\n");

	sprintf(buf + strlen(buf), "[RRC]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stRowRc.stCfg.bEnable);
	sprintf(buf + strlen(buf), "scale=%d\n", p_stfunc->stRowRc.stCfg.ucScale);
	sprintf(buf + strlen(buf), "init_coeff=%d\n", (int)p_stfunc->stRowRc.stCfg.uiInitFrmCoeff);
	sprintf(buf + strlen(buf), "i_pred_wt=%d\n", p_stfunc->stRowRc.stCfg.ucPredWt[0]);
	sprintf(buf + strlen(buf), "i_qp_range=%d\n", p_stfunc->stRowRc.stCfg.ucQPRange[0]);
	sprintf(buf + strlen(buf), "i_qp_step=%d\n", p_stfunc->stRowRc.stCfg.ucQPStep[0]);
	sprintf(buf + strlen(buf), "i_qp_min=%d\n", p_stfunc->stRowRc.stCfg.ucMinQP[0]);
	sprintf(buf + strlen(buf), "i_qp_max=%d\n", p_stfunc->stRowRc.stCfg.ucMaxQP[0]);
	sprintf(buf + strlen(buf), "i_prev_bits_bias=%d\n", (int)p_stfunc->stRowRc.iPrevPicBitsBias[0]);
	sprintf(buf + strlen(buf), "p_pred_wt=%d\n", p_stfunc->stRowRc.stCfg.ucPredWt[1]);
	sprintf(buf + strlen(buf), "p_qp_range=%d\n", p_stfunc->stRowRc.stCfg.ucQPRange[1]);
	sprintf(buf + strlen(buf), "p_qp_step=%d\n", p_stfunc->stRowRc.stCfg.ucQPStep[1]);
	sprintf(buf + strlen(buf), "p_qp_min=%d\n", p_stfunc->stRowRc.stCfg.ucMinQP[1]);
	sprintf(buf + strlen(buf), "p_qp_max=%d\n", p_stfunc->stRowRc.stCfg.ucMaxQP[1]);
	sprintf(buf + strlen(buf), "p_prev_bits_bias=%d\n", (int)p_stfunc->stRowRc.iPrevPicBitsBias[1]);
	sprintf(buf + strlen(buf), "min_cost_th=%d\n", p_stfunc->stRowRc.ucMinCostTh);
	sprintf(buf + strlen(buf), "init_qp=%d\n", p_stfunc->stRowRc.ucInitQp);
	sprintf(buf + strlen(buf), "plan_stop=%d\n", (int)p_stfunc->stRowRc.uiPlannedStop);
	sprintf(buf + strlen(buf), "plan_top=%d\n", (int)p_stfunc->stRowRc.uiPlannedTop);
	sprintf(buf + strlen(buf), "plan_bot=%d\n", (int)p_stfunc->stRowRc.uiPlannedBot);
	sprintf(buf + strlen(buf), "pred_bits=%d\n\n", (int)p_stfunc->stRowRc.uiPredBits);

	sprintf(buf + strlen(buf), "[LPM]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stLpm.stCfg.bEnable);
	sprintf(buf + strlen(buf), "rmd_sad_en=%d\n", p_stfunc->stLpm.stCfg.ucRmdSadEn);
	sprintf(buf + strlen(buf), "ime_stop_en=%d\n", p_stfunc->stLpm.stCfg.ucIMEStopEn);
	sprintf(buf + strlen(buf), "ime_stop_th=%d\n", p_stfunc->stLpm.stCfg.ucIMEStopTh);
	sprintf(buf + strlen(buf), "rdo_stop_en=%d\n", p_stfunc->stLpm.stCfg.ucRdoStopEn);
	sprintf(buf + strlen(buf), "rdo_stop_th=%d\n", p_stfunc->stLpm.stCfg.ucRdoStopTh);
	sprintf(buf + strlen(buf), "chrm_dm_en=%d\n\n", p_stfunc->stLpm.stCfg.ucChrmDmEn);

	sprintf(buf + strlen(buf), "[RND]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stRnd.stCfg.bEnable);
	sprintf(buf + strlen(buf), "rnd_seed=%d\n", p_stfunc->stRnd.stCfg.uiSeed);
	sprintf(buf + strlen(buf), "rnd_range=%d\n\n", p_stfunc->stRnd.stCfg.ucRange);

	sprintf(buf + strlen(buf), "[MAQ]\n");
	sprintf(buf + strlen(buf), "mode=%d\n", p_stfunc->stMAq.stCfg.ucMode);
	sprintf(buf + strlen(buf), "8x8to16x16_th=%d\n", p_stfunc->stMAq.stCfg.uc8x8to16x16Th);
	sprintf(buf + strlen(buf), "dqp_roi_th=%d\n", p_stfunc->stMAq.stCfg.ucDqpRoiTh);
	sprintf(buf + strlen(buf), "dqp=");
	for (i = 0; i < 5; i++) {
		sprintf(buf + strlen(buf), "%d,", (int)p_stfunc->stMAq.stCfg.cDqp[i]);
	}
	sprintf(buf + strlen(buf), "%d\n\n", (int)p_stfunc->stMAq.stCfg.cDqp[5]);

	sprintf(buf + strlen(buf), "[JND]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stJnd.stCfg.bEnable);
	sprintf(buf + strlen(buf), "str=%d\n", p_stfunc->stJnd.stCfg.ucStr);
	sprintf(buf + strlen(buf), "level=%d\n", p_stfunc->stJnd.stCfg.ucLevel);
	sprintf(buf + strlen(buf), "th=%d\n\n", p_stfunc->stJnd.stCfg.ucTh);

	sprintf(buf + strlen(buf), "[RQP]\n");
	sprintf(buf + strlen(buf), "save_dqp=%d\n\n", p_stfunc->stRqp.ucSaveDeltaQp);

	sprintf(buf + strlen(buf), "[VAR]\n");
	sprintf(buf + strlen(buf), "th=%d\n", p_stfunc->stVar.stCfg.usThreshold);
	sprintf(buf + strlen(buf), "avg_min=%d\n", p_stfunc->stVar.stCfg.ucAVGMin);
	sprintf(buf + strlen(buf), "avg_max=%d\n", p_stfunc->stVar.stCfg.ucAVGMax);
	sprintf(buf + strlen(buf), "delta=%d\n", p_stfunc->stVar.stCfg.ucDelta);
	sprintf(buf + strlen(buf), "i_range_delta=%d\n", p_stfunc->stVar.stCfg.ucIRangeDelta);
	sprintf(buf + strlen(buf), "p_range_delta=%d\n", p_stfunc->stVar.stCfg.ucPRangeDelta);
	sprintf(buf + strlen(buf), "merge=%d\n\n", p_stfunc->stVar.stCfg.ucMerage);

	if (p_stfunc->eCodecType == VCODEC_H264) {
		sprintf(buf + strlen(buf), "[FRO_264]\n");
		sprintf(buf + strlen(buf), "enable=%d\n", (int)p_stfunc->stFro.st264.bEnable);
		sprintf(buf + strlen(buf), "i_y4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[0][0], (int)p_stfunc->stFro.st264.ucAC[0][0], (int)p_stfunc->stFro.st264.ucST[0][0], (int)p_stfunc->stFro.st264.ucMX[0][0]);
		sprintf(buf + strlen(buf), "i_y8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[0][1], (int)p_stfunc->stFro.st264.ucAC[0][1], (int)p_stfunc->stFro.st264.ucST[0][1], (int)p_stfunc->stFro.st264.ucMX[0][1]);
		sprintf(buf + strlen(buf), "i_y4dc=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[0][2], (int)p_stfunc->stFro.st264.ucAC[0][2], (int)p_stfunc->stFro.st264.ucST[0][2], (int)p_stfunc->stFro.st264.ucMX[0][2]);
		sprintf(buf + strlen(buf), "i_c4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[0][3], (int)p_stfunc->stFro.st264.ucAC[0][3], (int)p_stfunc->stFro.st264.ucST[0][3], (int)p_stfunc->stFro.st264.ucMX[0][3]);
		sprintf(buf + strlen(buf), "intra_p_y4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[1][0], (int)p_stfunc->stFro.st264.ucAC[1][0], (int)p_stfunc->stFro.st264.ucST[1][0], (int)p_stfunc->stFro.st264.ucMX[1][0]);
		sprintf(buf + strlen(buf), "intra_p_y8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[1][1], (int)p_stfunc->stFro.st264.ucAC[1][1], (int)p_stfunc->stFro.st264.ucST[1][1], (int)p_stfunc->stFro.st264.ucMX[1][1]);
		sprintf(buf + strlen(buf), "intra_p_y4dc=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[1][2], (int)p_stfunc->stFro.st264.ucAC[1][2], (int)p_stfunc->stFro.st264.ucST[1][2], (int)p_stfunc->stFro.st264.ucMX[1][2]);
		sprintf(buf + strlen(buf), "intra_p_c4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[1][3], (int)p_stfunc->stFro.st264.ucAC[1][3], (int)p_stfunc->stFro.st264.ucST[1][3], (int)p_stfunc->stFro.st264.ucMX[1][3]);
		sprintf(buf + strlen(buf), "inter_p_y4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[2][0], (int)p_stfunc->stFro.st264.ucAC[2][0], (int)p_stfunc->stFro.st264.ucST[2][0], (int)p_stfunc->stFro.st264.ucMX[2][0]);
		sprintf(buf + strlen(buf), "inter_p_y8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[2][1], (int)p_stfunc->stFro.st264.ucAC[2][1], (int)p_stfunc->stFro.st264.ucST[2][1], (int)p_stfunc->stFro.st264.ucMX[2][1]);
		sprintf(buf + strlen(buf), "inter_p_y4dc=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st264.uiDC[2][2], (int)p_stfunc->stFro.st264.ucAC[2][2], (int)p_stfunc->stFro.st264.ucST[2][2], (int)p_stfunc->stFro.st264.ucMX[2][2]);
		sprintf(buf + strlen(buf), "inter_p_c4=%d,%d,%d,%d\n\n", (int)p_stfunc->stFro.st264.uiDC[2][3], (int)p_stfunc->stFro.st264.ucAC[2][3], (int)p_stfunc->stFro.st264.ucST[2][3], (int)p_stfunc->stFro.st264.ucMX[2][3]);

		sprintf(buf + strlen(buf), "[RDO_264]\n");
		sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stRdo.st264.bEnable);
		sprintf(buf + strlen(buf), "i_slop=%d,%d,%d,%d\n", p_stfunc->stRdo.st264.ucSlope[0][0], p_stfunc->stRdo.st264.ucSlope[0][1], p_stfunc->stRdo.st264.ucSlope[0][2], p_stfunc->stRdo.st264.ucSlope[0][3]);
		sprintf(buf + strlen(buf), "p_slop=%d,%d,%d,%d\n", p_stfunc->stRdo.st264.ucSlope[1][0], p_stfunc->stRdo.st264.ucSlope[1][1], p_stfunc->stRdo.st264.ucSlope[1][2], p_stfunc->stRdo.st264.ucSlope[1][3]);
		sprintf(buf + strlen(buf), "inter_slop=%d,%d,%d\n", p_stfunc->stRdo.st264.ucSlope[2][0], p_stfunc->stRdo.st264.ucSlope[2][1], p_stfunc->stRdo.st264.ucSlope[2][3]);
		sprintf(buf + strlen(buf), "i4_bias=%d\n", p_stfunc->stRdo.st264.ucI_Y4_CB);
		sprintf(buf + strlen(buf), "i8_bias=%d\n", p_stfunc->stRdo.st264.ucI_Y8_CB);
		sprintf(buf + strlen(buf), "i16_bias=%d\n", p_stfunc->stRdo.st264.ucI_Y16_CB);
		sprintf(buf + strlen(buf), "i4_bias_p=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucI_Y4_CB_P);
		sprintf(buf + strlen(buf), "i8_bias_p=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucI_Y8_CB_P);
		sprintf(buf + strlen(buf), "i16_bias_p=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucI_Y16_CB_P);
		sprintf(buf + strlen(buf), "i16_tw_dc=%d\n", p_stfunc->stRdo.st264.ucI16_CT_DC);
		sprintf(buf + strlen(buf), "i16_tw_h=%d\n", p_stfunc->stRdo.st264.ucI16_CT_H);
		sprintf(buf + strlen(buf), "i16_tw_v=%d\n", p_stfunc->stRdo.st264.ucI16_CT_V);
		sprintf(buf + strlen(buf), "i16_tw_p=%d\n", p_stfunc->stRdo.st264.ucI16_CT_P);

		sprintf(buf + strlen(buf), "p4_bias=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucP_Y4_CB);
		sprintf(buf + strlen(buf), "p8_bias=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucP_Y8_CB);
		sprintf(buf + strlen(buf), "skip_bias=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucIP_CB_SKIP);
		sprintf(buf + strlen(buf), "icm_tw_dc=%d\n", p_stfunc->stRdo.st264.ucIP_C_CT_DC);
		sprintf(buf + strlen(buf), "icm_tw_h=%d\n", p_stfunc->stRdo.st264.ucIP_C_CT_H);
		sprintf(buf + strlen(buf), "icm_tw_v=%d\n", p_stfunc->stRdo.st264.ucIP_C_CT_V);
		sprintf(buf + strlen(buf), "icm_tw_p=%d\n", p_stfunc->stRdo.st264.ucIP_C_CT_P);

		sprintf(buf + strlen(buf), "p_y_coeff_cost_th=%d\n", p_stfunc->stRdo.st264.stRdoCfg.ucP_Y_COEFF_COST_TH);
		sprintf(buf + strlen(buf), "p_c_coeff_cost_th=%d\n\n", p_stfunc->stRdo.st264.stRdoCfg.ucP_C_COEFF_COST_TH);
	}

	if (p_stfunc->eCodecType == VCODEC_H265) {
		sprintf(buf + strlen(buf), "[FRO_265]\n");
		sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stFro.st265.bEnable);

		sprintf(buf + strlen(buf), "i_y32=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][0][0], (int)p_stfunc->stFro.st265.ucAC[0][0][0], (int)p_stfunc->stFro.st265.ucST[0][0][0], (int)p_stfunc->stFro.st265.ucMX[0][0][0]);
		sprintf(buf + strlen(buf), "i_y16=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][0][1], (int)p_stfunc->stFro.st265.ucAC[0][0][1], (int)p_stfunc->stFro.st265.ucST[0][0][1], (int)p_stfunc->stFro.st265.ucMX[0][0][1]);
		sprintf(buf + strlen(buf), "i_y8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][0][2], (int)p_stfunc->stFro.st265.ucAC[0][0][2], (int)p_stfunc->stFro.st265.ucST[0][0][2], (int)p_stfunc->stFro.st265.ucMX[0][0][2]);
		sprintf(buf + strlen(buf), "i_y4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][0][3], (int)p_stfunc->stFro.st265.ucAC[0][0][3], (int)p_stfunc->stFro.st265.ucST[0][0][3], (int)p_stfunc->stFro.st265.ucMX[0][0][3]);

		sprintf(buf + strlen(buf), "i_c16=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][0][0], (int)p_stfunc->stFro.st265.ucAC[1][0][0], (int)p_stfunc->stFro.st265.ucST[1][0][0], (int)p_stfunc->stFro.st265.ucMX[1][0][0]);
		sprintf(buf + strlen(buf), "i_c8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][0][1], (int)p_stfunc->stFro.st265.ucAC[1][0][1], (int)p_stfunc->stFro.st265.ucST[1][0][1], (int)p_stfunc->stFro.st265.ucMX[1][0][1]);
		sprintf(buf + strlen(buf), "i_c4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][0][2], (int)p_stfunc->stFro.st265.ucAC[1][0][2], (int)p_stfunc->stFro.st265.ucST[1][0][2], (int)p_stfunc->stFro.st265.ucMX[1][0][2]);

		sprintf(buf + strlen(buf), "intra_p_y16=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][1][1], (int)p_stfunc->stFro.st265.ucAC[0][1][1], (int)p_stfunc->stFro.st265.ucST[0][1][1], (int)p_stfunc->stFro.st265.ucMX[0][1][1]);
		sprintf(buf + strlen(buf), "intra_p_y8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][1][2], (int)p_stfunc->stFro.st265.ucAC[0][1][2], (int)p_stfunc->stFro.st265.ucST[0][1][2], (int)p_stfunc->stFro.st265.ucMX[0][1][2]);
		sprintf(buf + strlen(buf), "intra_p_y4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][1][3], (int)p_stfunc->stFro.st265.ucAC[0][1][3], (int)p_stfunc->stFro.st265.ucST[0][1][3], (int)p_stfunc->stFro.st265.ucMX[0][1][3]);

		sprintf(buf + strlen(buf), "intra_p_c8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][1][1], (int)p_stfunc->stFro.st265.ucAC[1][1][1], (int)p_stfunc->stFro.st265.ucST[1][1][1], (int)p_stfunc->stFro.st265.ucMX[1][1][1]);
		sprintf(buf + strlen(buf), "intra_p_c4=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][1][2], (int)p_stfunc->stFro.st265.ucAC[1][1][2], (int)p_stfunc->stFro.st265.ucST[1][1][2], (int)p_stfunc->stFro.st265.ucMX[1][1][2]);

		sprintf(buf + strlen(buf), "inter_p_y32=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][2][0], (int)p_stfunc->stFro.st265.ucAC[0][2][0], (int)p_stfunc->stFro.st265.ucST[0][2][0], (int)p_stfunc->stFro.st265.ucMX[0][2][0]);
		sprintf(buf + strlen(buf), "inter_p_y16=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][2][1], (int)p_stfunc->stFro.st265.ucAC[0][2][1], (int)p_stfunc->stFro.st265.ucST[0][2][1], (int)p_stfunc->stFro.st265.ucMX[0][2][1]);
		sprintf(buf + strlen(buf), "inter_p_y8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[0][2][2], (int)p_stfunc->stFro.st265.ucAC[0][2][2], (int)p_stfunc->stFro.st265.ucST[0][2][2], (int)p_stfunc->stFro.st265.ucMX[0][2][2]);

		sprintf(buf + strlen(buf), "inter_p_c16=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][2][0], (int)p_stfunc->stFro.st265.ucAC[1][2][0], (int)p_stfunc->stFro.st265.ucST[1][2][0], (int)p_stfunc->stFro.st265.ucMX[1][2][0]);
		sprintf(buf + strlen(buf), "inter_p_c8=%d,%d,%d,%d\n", (int)p_stfunc->stFro.st265.uiDC[1][2][1], (int)p_stfunc->stFro.st265.ucAC[1][2][1], (int)p_stfunc->stFro.st265.ucST[1][2][1], (int)p_stfunc->stFro.st265.ucMX[1][2][1]);
		sprintf(buf + strlen(buf), "inter_p_c4=%d,%d,%d,%d\n\n", (int)p_stfunc->stFro.st265.uiDC[1][2][2], (int)p_stfunc->stFro.st265.ucAC[1][2][2], (int)p_stfunc->stFro.st265.ucST[1][2][2], (int)p_stfunc->stFro.st265.ucMX[1][2][2]);

		sprintf(buf + strlen(buf), "[RDO_265]\n");
		sprintf(buf + strlen(buf), "enable=%d\n", p_stfunc->stRdo.st265.bEnable);
		sprintf(buf + strlen(buf), "rate_bias_i_32l=%d\n", p_stfunc->stRdo.st265.rate_bias_I_32L);
		sprintf(buf + strlen(buf), "rate_bias_i_16l=%d\n", p_stfunc->stRdo.st265.rate_bias_I_16L);
		sprintf(buf + strlen(buf), "rate_bias_i_8l=%d\n", p_stfunc->stRdo.st265.rate_bias_I_08L);
		sprintf(buf + strlen(buf), "rate_bias_i_4l=%d\n", p_stfunc->stRdo.st265.rate_bias_I_04L);
		sprintf(buf + strlen(buf), "rate_bias_i_16c=%d\n", p_stfunc->stRdo.st265.rate_bias_I_16C);
		sprintf(buf + strlen(buf), "rate_bias_i_8c=%d\n", p_stfunc->stRdo.st265.rate_bias_I_08C);
		sprintf(buf + strlen(buf), "rate_bias_i_4c=%d\n", p_stfunc->stRdo.st265.rate_bias_I_04C);

		sprintf(buf + strlen(buf), "rate_bias_ip_16l=%d\n", p_stfunc->stRdo.st265.rate_bias_IP_16L);
		sprintf(buf + strlen(buf), "rate_bias_ip_8l=%d\n", p_stfunc->stRdo.st265.rate_bias_IP_08L);
		sprintf(buf + strlen(buf), "rate_bias_ip_4l=%d\n", p_stfunc->stRdo.st265.rate_bias_IP_04L);
		sprintf(buf + strlen(buf), "rate_bias_ip_8c=%d\n", p_stfunc->stRdo.st265.rate_bias_IP_08C);
		sprintf(buf + strlen(buf), "rate_bias_ip_4c=%d\n", p_stfunc->stRdo.st265.rate_bias_IP_04C);

		sprintf(buf + strlen(buf), "rate_bias_p_32l=%d\n", p_stfunc->stRdo.st265.rate_bias_P_32L);
		sprintf(buf + strlen(buf), "rate_bias_p_16l=%d\n", p_stfunc->stRdo.st265.rate_bias_P_16L);
		sprintf(buf + strlen(buf), "rate_bias_p_8l=%d\n", p_stfunc->stRdo.st265.rate_bias_P_08L);
		sprintf(buf + strlen(buf), "rate_bias_p_16c=%d\n", p_stfunc->stRdo.st265.rate_bias_P_16C);
		sprintf(buf + strlen(buf), "rate_bias_p_8c=%d\n", p_stfunc->stRdo.st265.rate_bias_P_08C);
		sprintf(buf + strlen(buf), "rate_bias_p_4c=%d\n", p_stfunc->stRdo.st265.rate_bias_P_04C);

		sprintf(buf + strlen(buf), "cost_bias_skip=%d\n", p_stfunc->stRdo.st265.stRdoCfg.cost_bias_skip);
		sprintf(buf + strlen(buf), "cost_bias_merge=%d\n", p_stfunc->stRdo.st265.stRdoCfg.cost_bias_merge);

		sprintf(buf + strlen(buf), "gmp_i32=%d\n", p_stfunc->stRdo.st265.stRdoCfg.global_motion_penalty_I32);
		sprintf(buf + strlen(buf), "gmp_i16=%d\n", p_stfunc->stRdo.st265.stRdoCfg.global_motion_penalty_I16);
		sprintf(buf + strlen(buf), "gmp_i08=%d\n", p_stfunc->stRdo.st265.stRdoCfg.global_motion_penalty_I08);
		sprintf(buf + strlen(buf), "gmp_i32_o=%d\n", p_stfunc->stRdo.st265.global_motion_penalty_I32O);
		sprintf(buf + strlen(buf), "gmp_i16_o=%d\n", p_stfunc->stRdo.st265.global_motion_penalty_I16O);
		sprintf(buf + strlen(buf), "gmp_i08_o=%d\n\n", p_stfunc->stRdo.st265.global_motion_penalty_I08O);

		sprintf(buf + strlen(buf), "ime_64c=%d\n", p_stfunc->stRdo.st265.stRdoCfg.ime_scale_64c);
		sprintf(buf + strlen(buf), "ime_64p=%d\n", p_stfunc->stRdo.st265.stRdoCfg.ime_scale_64p);
		sprintf(buf + strlen(buf), "ime_32c=%d\n", p_stfunc->stRdo.st265.stRdoCfg.ime_scale_32c);
		sprintf(buf + strlen(buf), "ime_32p=%d\n", p_stfunc->stRdo.st265.stRdoCfg.ime_scale_32p);
		sprintf(buf + strlen(buf), "ime_16c=%d\n", p_stfunc->stRdo.st265.stRdoCfg.ime_scale_16c);

	}

	return 0;
}



