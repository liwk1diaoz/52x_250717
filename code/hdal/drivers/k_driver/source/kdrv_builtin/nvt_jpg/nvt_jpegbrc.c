/**
    Default JPEG bit rate control

    Default JPEG bit rate control via Rho estimation.

    @file       JPEGBrcDefault.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "jpeg_brc.h"

static UINT32   rho_array[7];

//NVTVER_VERSION_ENTRY(JpgBrc, 1, 00, 000, /* branch version */ 00)

/**
    @addtogroup
*/
//@{

/**
    JPEG BRC predict quality

    JPEG BRC predict quality

    @param[in/out] pBrcInfo     bit rate control information

    @return Predicted quality (QF, JPGBRC_QUALITY_MIN ~ JPGBRC_QUALITY_MAX)
*/
UINT32 jpgbrc_predict_quality(PJPGBRC_INFO p_brcinfo)
{
	PJPGBRC_PARAM   pbrc_param;
	UINT32          i;
	UINT32          total_sample;
	INT64           temp_target_rho;
	UINT64          tar_qf;
	UINT64          target_rho;
	UINT64          qf_temp_0;
	UINT64          qf_temp_1;
	UINT64          ro_temp_0;
	UINT64          ro_temp_1;
	UINT64          temp_uint;

	pbrc_param   = p_brcinfo->pbrc_param;

	if ((pbrc_param->target_byte == 0) ||
		(pbrc_param->width == 0) ||
		(pbrc_param->height == 0)) {
		DBG_ERR("TargetByte, Width or Height is 0\r\n");
		return p_brcinfo->current_qf;
	}

	switch (p_brcinfo->input_raw) {
	case JPGBRC_COMBINED_YUV420:
		total_sample = (pbrc_param->width * pbrc_param->height) + ((pbrc_param->width * pbrc_param->height) >> 1);
		break;

	case JPGBRC_COMBINED_YUV422:
		total_sample = (pbrc_param->width * pbrc_param->height) << 1;
		break;

	case JPGBRC_YUV100:
		total_sample = pbrc_param->width * pbrc_param->height;
		break;

	default:
		DBG_ERR("Not supported format\r\n");
		return p_brcinfo->current_qf;
	}

	// Get BRC information (Rho value) from JPEG driver
	jpeg_get_brcinfo((PJPEG_BRC_INFO)rho_array);

	// Use integer operation (<< 16) instead of floating operation to improve performance
#if 1
	// Rounding
	temp_uint = ((UINT64)(pbrc_param->target_byte) << 16) + (p_brcinfo->current_rate >> 1);
#ifdef __KERNEL__
	temp_uint  = (UINT64)div_u64(temp_uint, p_brcinfo->current_rate);
#else
	temp_uint /= p_brcinfo->current_rate;
#endif

#else
	// Round down
	temp_uint = (tar_rate << 16) / cur_rate;
#endif

	// Rc: Current Rate
	// Zc: Current Rho (rho_array[3])
	// Rt: Target Rate
	// Zt: Target Rho
	// T: Total sample
	// Get Target Rho from the equation
	// Rc / (T - Zc) = Rt / (T - Zt)
	// --> Zt = T * (1 - (Rt/Rc)) + Zc * (Rt/Rc)
	temp_target_rho = total_sample * (INT64)(65536 - temp_uint) + (temp_uint * rho_array[3]);

	if (temp_target_rho < 0) {
		p_brcinfo->brc_result = JPGBRC_RESULT_UNDERFLOW;
		return JPGBRC_QUALITY_MIN;
	}

	target_rho = (UINT64)temp_target_rho;

	target_rho >>= 16;

	// [0]  1/8  1/8 QF
	// [1]  2/8  1/4 QF
	// [2]  4/8  1/2 QF
	// [3]  8/8    1 QF
	// [4] 16/8    2 QF
	// [5] 32/8    4 QF
	// [6] 64/8    8 QF
	// Get target QF from QF and Rho
	for (i = 0; i < 7; i++) {
		if (target_rho < rho_array[i]) {
			if (i == 0) {
				// 1/8 QF
				tar_qf = p_brcinfo->current_qf >> 3;

				// Chris Hsu: Need to find out why
				// When 1/8 QF is 16 and Target Rho does not closed to 16, direct to use QF=8
				// (uiTargetRho < ((rho_array[0] << 1) - rho_array[1])))
				if ((tar_qf == 16) &&
					((target_rho + rho_array[1]) < (rho_array[0] << 1))) {
					tar_qf = 8; // For Special Case
				}
			} else {
				ro_temp_0 = rho_array[i - 1];
				ro_temp_1 = rho_array[i];
				qf_temp_0 = (UINT64)((UINT64)p_brcinfo->current_qf << (i - 1)) >> 3;
				qf_temp_1 = (UINT64)((UINT64)p_brcinfo->current_qf << i)       >> 3;

				// Use integer operation (<< 7) instead of floating operation to improve performance
#if 1
				// Rounding
				temp_uint = ((target_rho - ro_temp_0) << 7) + ((ro_temp_1 - ro_temp_0) >> 1);
#ifdef __KERNEL__
	temp_uint =  div_u64(temp_uint, (ro_temp_1 - ro_temp_0));
#else
	temp_uint /= (ro_temp_1 - ro_temp_0);
#endif

#else
				// Round down
				temp_uint = ((target_rho - ro_temp_0) << 7) / (ro_temp_1 - ro_temp_0);
#endif
				tar_qf = (qf_temp_0 << 7) + (temp_uint * (qf_temp_1 - qf_temp_0)) + (1 << 6);
				tar_qf >>= 7;
			}

			break;
		}
	}

	if (i == 7) {
		// 8 QF
		tar_qf = (UINT64)p_brcinfo->current_qf << 8;
	}

	if (tar_qf > JPGBRC_QUALITY_MAX) {
		p_brcinfo->brc_result = JPGBRC_RESULT_OVERFLOW;
		tar_qf = JPGBRC_QUALITY_MAX;
	} else if (tar_qf < JPGBRC_QUALITY_MIN) {
		p_brcinfo->brc_result = JPGBRC_RESULT_UNDERFLOW;
		tar_qf = JPGBRC_QUALITY_MIN;
	} else {
		p_brcinfo->brc_result = JPGBRC_RESULT_OK;
	}

	return (UINT32)tar_qf;
}

/**
    JPEG BRC generate Y quantization table

    JPEG BRC generate Y quantization table

    @param[in] uiQuality        Quality value (0 ~ 512), lower value has better quality
    @param[in] pStdYtbl         standard Y channel quantization table
    @param[out] pYtbl           Y channel quantization table

    @return void
*/
void jpgbrc_gen_qtable_y(UINT32 quality, const UINT8 *p_std_ytbl, UINT8 *p_ytbl)
{
	UINT32  coff_y;
	UINT32  i;

	// Best quality, all entries are 1
	if (quality == 0) {
		memset((void *)p_ytbl, 1, 64);
	} else {
		for (i = 0; i < 64; i++) {
#if 1
			// Rounding
			coff_y = ((*p_std_ytbl++ * quality) + 256) >> 9;
#else
			// Round down
			coff_y = (*p_std_ytbl++ * quality) >> 9;
#endif

			if (coff_y < 1) {
				coff_y = 1;
			} else if (coff_y > 255) {
				coff_y = 255;
			}

			*p_ytbl++  = (UINT8)coff_y;
		}
	}
}

/**
    JPEG BRC generate U/V quantization table

    JPEG BRC generate U/V quantization table

    @param[in] uiQuality        Quality value (0 ~ 512), lower value has better quality
    @param[in] pStdUVtbl        Standard U/V channel quantizationt table
	@note						Q[14] ~ Q[63] must be the same
    @param[out] pUVtbl          U/V channel quantization table of input quality

    @return void
*/
void jpgbrc_gen_qtable_uv(UINT32 quality, const UINT8 *p_std_uvtbl, UINT8 *p_uvtbl)
{
	// For compiler warning message, we have to assign initial value
	// But that's a FALSE warning.
	UINT32  coff_uv = 1;
	UINT32  i;

	// Best quality, all entries are 1
	if (quality == 0) {
		memset((void *)p_uvtbl, 1, 64);
	} else {
		for (i = 0; i < 64; i++) {
			// Upper layer will pass grucStdChrQ as pStdUVtbl to this API.
			// The value of grucStdChrQ[i] are the same when i = 14 ~ 63,
			// so we don't calculate the value when i is 16 ~ 63 to improve performance.
			if (i < 16) {
#if 1
				// Rounding
				coff_uv = ((*p_std_uvtbl++ * quality) + 256) >> 9;
#else
				// Round down
				coff_uv = (*p_std_uvtbl++ * quality) >> 9;
#endif

				if (coff_uv < 1) {
					coff_uv = 1;
				} else if (coff_uv > 255) {
					coff_uv = 255;
				}
			}

			*p_uvtbl++ = (UINT8)coff_uv;
		}
	}
}

//@}
