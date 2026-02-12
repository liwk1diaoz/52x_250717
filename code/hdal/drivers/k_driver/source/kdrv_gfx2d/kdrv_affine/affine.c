/*
    Affine module driver

    @file       affine.c
    @ingroup    mIDrvIPP_Affine
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
#include "affine_reg.h"
#include "llc.h"

typedef enum {
	FLOAT_ADD,
	FLOAT_SUB,
	FLOAT_MUL,
	FLOAT_MAX,
	FLOAT_MIN,

	ENUM_DUMMY4WORD(FLOAT_OPERATION_TYPE)
} FLOAT_OPERATION_TYPE;

typedef enum {
	FLOAT_EQUAL,
	FLOAT_GREATER,
	FLOAT_LESS,

	ENUM_DUMMY4WORD(FLOAT_COMPARISON_TYPE)
} FLOAT_COMPARISON_TYPE;
#else
#include "affine_reg.h"
#include <asm/neon.h>
#include "affine_neon/affine_neon.h"
#include "llc.h"
#endif

#include "kwrap/error_no.h"
#include "affine_platform.h"
#include "affine_dbg.h"

#define AFFINE_FLT_MAX          (1E+37)

//
//  Internal Definitions
//
#define FIXP_SCALE_FACT         (15)        // Q13 for 1920, Q15 for 16382 (Q15 is proposed)

#define DST_SUBBLK_WIDTH        (32)//(16)        // unit: byte
#define DST_SUBBLK_HEIGHT       (16)//(8)         // unit: line
#define DST_1ST_OUT_HEIGHT	(8)

#define SRC_SUBBLK_WIDTH        (48)//(6*4)       // unit: byte
#define SRC_SUBBLK_HEIGHT       (26)//(13)
#define SRC_1ST_IN_HEIGHT	(7)

#define SRC_BURST_WIDTH_WORDS   (12)         // burst width is fixed at 6 words
#define SRC_BURST_HEIGHT_MIN    (18)        // min burst height is 18 lines
#define SRC_BURST_HEIGHT_MAX    (26)        // min burst height is 26 lines

typedef enum {
	CORNER_LT,                              //< Left Top Corner
	CORNER_RT,                              //< Right Top Corner
	CORNER_LB,                              //< Left Bottom Corner
	CORNER_RB,                              //< Right Bottom Corner

	ENUM_DUMMY4WORD(CORNER_ENUM)
} CORNER_ENUM;

static UINT32 sub_blk_height;               // stores subblock burst height calculated by chk_subblock()
static UINT32 sub_blk_vector_x;              // stores subblock vector X calculated by chk_subblock()
static UINT32 sub_blk_vector_y;              // stores subblock vector Y calculated by chk_subblock()

static BOOL coeff_direct_path = FALSE;

static UINT32 direct_coeff_a = 0;
static UINT32 direct_coeff_b = 0;
static UINT32 direct_coeff_c = 0;
static UINT32 direct_coeff_d = 0;
static UINT32 direct_coeff_e = 0;
static UINT32 direct_coeff_f = 0;
static UINT32 sub_blk_1 = 0;
static UINT32 sub_blk_2 = 0;

//static BOOL bAffineOpend = FALSE;       // TRUE: driver is opend, FALSE: driver is closed
static const FLGPTN v_isr_flag[] = {FLGPTN_AFFINE};

static UINT32 affine_open_counter = 0;
static UINT32 affine_clk_sel[] = {240};

static AFFINE_REG_DOMAIN eAffine_reg_domain = AFFINE_REG_DOMAIN_LL;		// Affine reg domain
static UINT64 *p_affine_llc_cmd;

// memory buffer to store link list descriptors
//static _ALIGNED(32) UINT64 affine_llc_cmd[AFINE_LL_BUF_SIZE];

/**
    @addtogroup mIDrvIPP_Affine
*/
//@{

/*
    Get affine controller register.

    @param[in] id           affine controller ID
    @param[in] offset       register offset in affine controller (word alignment)

    @return register value
*/
static REGVALUE affine_getreg(AFFINE_ID id, UINT32 offset)
{
	if (id == AFFINE_ID_1)
		return AFFINE_GETREG(offset);
	else
		return 0;
}

/*
    Set affine controller register.

    @param[in] id           affine controller ID
    @param[in] offset       register offset in affine controller (word alignment)
    @param[in] value        register value

    @return void
*/
static void affine_setreg(AFFINE_ID id, UINT32 offset, REGVALUE value)
{
	if (id == AFFINE_ID_1)
		AFFINE_SETREG(offset, value);
}

void affine_isr_bottom(AFFINE_ID id, UINT32 events)
{
	AFFINE_REQ_LIST_NODE *p_node;
	BOOL is_list_empty;
	UINT32 spin_flags;

	p_node = affine_platform_get_head(id);
	if (p_node == NULL) {
	DBG_ERR("%s: get head fail\r\n", __func__);
 	return;
	}

 	if (p_node->callback.callback == NULL) {
		//affine_platform_flg_set(id, v_isr_flag[id]);
	} else {
		p_node->callback.callback(&p_node->cb_info, NULL);
	}

	// Enter critical section
 	spin_flags = affine_platform_spin_lock(id);

 	affine_platform_del_list(id);
	is_list_empty = affine_platform_list_empty(id);
	if (is_list_empty == FALSE) {
		// Not empty => process next node
		affine_trigger(id);
	}

	// Leave critical section
	affine_platform_spin_unlock(id, spin_flags);

	if (p_node->callback.callback == NULL) {
		affine_platform_flg_set(id, v_isr_flag[id]);
	}

#if 0
	if (is_list_empty == FALSE) {
		// Not empty => process next node
		affine_trigger(id);
	} else {
#if 0	// debug
		DBG_ERR("%s %d: last Queue, cmd 0x%x\r\n",
			__func__, id, (unsigned int)p_node->trig_param.command);
#endif
	}
#endif

}


/*
    Affine interrupt handler

    Disable status interrupt and set flag
    This is internal API.

    @return void
*/
void affine_isr(void)
{
	T_AFFINE_INTSTS_REG int_sts;
	BOOL set_flg = FALSE;

	int_sts.reg = affine_getreg(AFFINE_ID_1, AFFINE_INTSTS_REG_OFS);
	if (int_sts.reg == 0) {
		return;
	}

	// Check interrupt status
	if (eAffine_reg_domain == AFFINE_REG_DOMAIN_LL) {
		if (int_sts.bit.LLEND_INT_STS == 1)
			set_flg = TRUE;
		if (int_sts.bit.LLERROR_INT_STS == 1) {
			// Todo..
		}
	}
	else {
		if (int_sts.bit.INT_STS == 1)
			set_flg = TRUE;
	}

	// Clear interrupt status
	affine_setreg(AFFINE_ID_1, AFFINE_INTSTS_REG_OFS, int_sts.reg);

	if (set_flg)
		affine_platform_set_ist_event(AFFINE_ID_1);
	//	affine_platform_flg_set(AFFINE_ID_1, (FLGPTN)FLGPTN_AFFINE);

}

/*
    Lock Affine

    This function lock affine module
    This is internal API.

    @param[in] id               affine controller identifier

    @return
        - @b E_OK: Done with no error.
        - @b Others: Error occured.
*/
static ER affine_lock(AFFINE_ID id)
{
	ER ret;

	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	ret = affine_platform_sem_wait(id);

	return ret;
}

/*
    Un-Lock Affine

    This function unlock Affine module
    This is internal API.

    @param[in] id               affine controller identifier

    @return
        - @b E_OK: Done with no error.
        - @b Others: Error occured.
*/
static ER affine_unlock(AFFINE_ID id)
{
	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	return affine_platform_sem_signal(id);
}

static FLOAT float_operation(FLOAT_OPERATION_TYPE type, FLOAT param_1, FLOAT param_2)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	FLOAT result = 0;

	switch (type) {
	case FLOAT_ADD:
		result = param_1 + param_2;
		break;
	case FLOAT_SUB:
		result = param_1 - param_2;
		break;
	case FLOAT_MUL:
		result = param_1 * param_2;
		break;
	case FLOAT_MAX:
		result = (param_1 > param_2) ? param_1 : param_2;
		break;
	case FLOAT_MIN:
		result = (param_1 > param_2) ? param_2 : param_1;
		break;

	default:
		break;
	}

	return result;
#else
	FLOAT value_1[4] = {0, 0, 0, 0};
	FLOAT value_2[4] = {0, 0, 0, 0};
	FLOAT result[4] = {0, 0, 0, 0};

	value_1[0] = param_1;
	value_2[0] = param_2;

	kernel_neon_begin();
	affine_neon_array_operation(type, value_1, value_2, result);
	kernel_neon_end();

	return result[0];
#endif
}

static BOOL float_comparison(FLOAT_COMPARISON_TYPE type, FLOAT param_1, FLOAT param_2)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	BOOL result = FALSE;

	switch (type) {
	case FLOAT_EQUAL:
		result = (param_1 == param_2) ? TRUE : FALSE;
		break;
	case FLOAT_GREATER:
		result = (param_1 > param_2) ? TRUE : FALSE;
		break;
	case FLOAT_LESS:
		result = (param_1 < param_2) ? TRUE : FALSE;
		break;

	default:
		break;
	}

	return result;
#else
	FLOAT value_1[4] = {0, 0, 0, 0};
	FLOAT value_2[4] = {0, 0, 0, 0};
	UINT32 result[4] = {0, 0, 0, 0};

	value_1[0] = param_1;
	value_2[0] = param_2;

	kernel_neon_begin();
	affine_neon_array_comparison(type, value_1, value_2, result);
	kernel_neon_end();

	return (BOOL)result[0];
#endif
}

static INT32 float_to_int(FLOAT param_1)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	return (INT32)param_1;
#else
	FLOAT value_1[4] = {0, 0, 0, 0};
	INT32 result[4] = {0, 0, 0, 0};

	value_1[0] = param_1;

	kernel_neon_begin();
	affine_neon_float_to_int(value_1, result);
	kernel_neon_end();

	return result[0];
#endif
}

static FLOAT int_to_float(INT32 param_1)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	return (FLOAT)param_1;
#else
	INT32 value_1[4] = {0, 0, 0, 0};
	FLOAT result[4] = {0, 0, 0, 0};

	value_1[0] = param_1;

	kernel_neon_begin();
	affine_neon_int_to_float(value_1, result);
	kernel_neon_end();

	return result[0];
#endif
}

// Substitute  #include<math.h> ceilf(x)
//#define ceilf(x)  (x==(int)x ? x : (int)x+1)
static INT32 float_ceil(FLOAT param_1)
{
	INT32 i_param_1 = float_to_int(param_1);

	if (float_comparison(FLOAT_EQUAL, param_1, int_to_float(i_param_1)) == TRUE)
		return i_param_1;
	else
		return i_param_1 + 1;

	return i_param_1;
}

static INT32 float_floor(FLOAT param_1)
{
	INT32 i_param_1 = float_to_int(param_1);

	return i_param_1;
}

/*
    Float to Q16.15

    @param[in] f    floating point value

    @return Q16.15 stored in UINT32
*/
static UINT32 affine_float_to_q16p15(FLOAT f)
{
	//return (INT32)(f * (INT32)(1 << FIXP_SCALE_FACT));
	return float_to_int(float_operation(FLOAT_MUL, f, 1 << FIXP_SCALE_FACT));
}

/*
    Q16.15 to Q1.15

    @param[in] input  Unsigned integer stores Q16.15

    @return Q1.15 stored in UINT32
*/
static UINT32 affine_q16p15_to_q1p15(UINT32 input)
{
	UINT32 fraction, integer, sign;
	UINT32 value;

	fraction = input & ((1 << FIXP_SCALE_FACT) - 1);
	integer = input & (1 << FIXP_SCALE_FACT);
	sign = ((input >> 31) & 0x01) << (FIXP_SCALE_FACT + 1);

	value = sign | integer | fraction;

	return value;
}

/*
    Q16.15 to Q14.15

    @param[in] input  Unsigned integer stores Q16.15

    @return Q14.15 stored in UINT32
*/
static UINT32 affine_q16p15_to_q14p15(UINT32 input)
{
	UINT32 fraction, integer, sign;
	UINT32 value;

	fraction = input & ((1 << FIXP_SCALE_FACT) - 1);
	integer = input & (0xFFFF << FIXP_SCALE_FACT);
	sign = ((input >> 31) & 0x01) << (FIXP_SCALE_FACT + 16);

	value = sign | integer | fraction;

	return value;
}

/*
    Flush cache for affine operations

    @param[in] p_request     Description of request

    @return void
*/
static void affine_flush_cache(PAFFINE_REQUEST p_request)
{
	do {
		// Source image should be cleaned first.
		// Else flush destination image may invalidate overlapped region on source image
		if (affine_platform_dma_is_cacheable(p_request->p_src_img->uiImageAddr)) {
			if (affine_platform_dma_flush_mem2dev(p_request->p_src_img->uiImageAddr,
												  p_request->p_src_img->uiLineOffset * p_request->height)) {
				break;
			}
		}

		if (affine_platform_dma_is_cacheable(p_request->p_dst_img->uiImageAddr)) {
			// Output image width <--> lineoffset may contain data not allowed to modify by graphic
			// Use clean and invalidate to ensure this region will not be invalidated
			if (p_request->width != p_request->p_dst_img->uiLineOffset) {
				affine_platform_dma_flush_dev2mem_width_neq_loff(p_request->p_dst_img->uiImageAddr,
													 p_request->p_dst_img->uiLineOffset * p_request->height);
			} else {
				affine_platform_dma_flush_dev2mem(p_request->p_dst_img->uiImageAddr,
								   p_request->p_dst_img->uiLineOffset * p_request->height);
			}
		}
	} while (0);
	__asm__ __volatile__("dsb\n\t");

}

/*
    Check Parameter validation

    @param[in] p_request     request descriptor to be checked
    @param[in] b_msg         message output enable
                            - @b TRUE: output message
                            - @b FALSE: disable output message

    @return
        - @b E_OK: check OK
        - @b Else: something wrong
*/
static ER affine_chk_parameter(PAFFINE_REQUEST p_request, BOOL b_msg)
{
	if (p_request == NULL) {
		if (b_msg) {
			DBG_ERR("NULL request\r\n");
		}
		return E_PAR;
	}

	if (p_request->p_coefficient == NULL) {
		if (b_msg) {
			DBG_ERR("NULL coefficient\r\n");
		}
		return E_PAR;
	}

	if (p_request->p_src_img == NULL) {
		if (b_msg) {
			DBG_ERR("NULL src image\r\n");
		}
		return E_PAR;
	}
	if (p_request->p_src_img->uiImageAddr & 0x03) {
		if (b_msg) {
			DBG_ERR("src img must be word alignment: 0x%x\r\n", (unsigned int)p_request->p_src_img->uiImageAddr);
		}
		return E_PAR;
	}
	if (p_request->p_src_img->uiLineOffset & 0x03) {
		if (b_msg) {
			DBG_ERR("src lineoffset must be word alignment: 0x%x\r\n", (unsigned int)p_request->p_src_img->uiLineOffset);
		}
		return E_PAR;
	}

	if (p_request->p_dst_img == NULL) {
		if (b_msg) {
			DBG_ERR("NULL dst image\r\n");
		}
		return E_PAR;
	}
	if (p_request->p_dst_img->uiImageAddr & 0x03) {
		if (b_msg) {
			DBG_ERR("dst img must be word alignment: 0x%x\r\n", (unsigned int)p_request->p_dst_img->uiImageAddr);
		}
		return E_PAR;
	}
	if (p_request->p_dst_img->uiLineOffset & 0x03) {
		if (b_msg) {
			DBG_ERR("dst lineoffset must be word alignment: 0x%1x\r\n", (unsigned int)p_request->p_dst_img->uiLineOffset);
		}
		return E_PAR;
	}

	if (p_request->width & 0x0F) {
		if (b_msg) {
			DBG_WRN("width should be 16 bytes alignment: 0x%x\r\n", (unsigned int)p_request->width);
		}
	}
	if (p_request->height & 0x07) {
		if (b_msg) {
			DBG_WRN("height should be 8 lines alignment: 0x%x\r\n", (unsigned int)p_request->height);
		}
	}

	return E_OK;
}

/*
    Check Subblock

    Affine controller divide destination image to several 16x8 destination sub block.
    Each destination sub block is mapped to source image with affine transform
    and controller uses a 24x13 SRAM to fetch the "Rectangle" where destination
    sub block can be fit in.

    This function is used to check if source sub block can contain all pixels
    required by destination sub block with input affine coefficients.

    @param[in] p_request     request descriptor
    @param[in] b_msg         message output enable
                            - @b TRUE: output message
                            - @b FALSE: disable output message

    @return
        - @b E_OK: check OK
        - @b Else: something wrong
*/
static ER affine_chk_subblock(PAFFINE_REQUEST p_request, BOOL b_msg)
{
	FLOAT vX[4], vY[4];
	FLOAT min_x = AFFINE_FLT_MAX, max_x = -AFFINE_FLT_MAX;
	FLOAT min_y = AFFINE_FLT_MAX, max_y = -AFFINE_FLT_MAX;
	UINT32 i;
	UINT32 width, height;

	// Calculate corner coordinate
	if (p_request->format == AFFINE_FORMAT_FORMAT_8BITS) {
		// When format is 8 bits, use (16pixel x 8 line) to get source coordinate.

		// Left/Top
		vX[CORNER_LT] = 0;
		vY[CORNER_LT] = 0;

		// Right/Top
		//vX[CORNER_RT] = p_request->p_coefficient->fCoeffA * (DST_SUBBLK_WIDTH - 1);
		vX[CORNER_RT] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffA, DST_SUBBLK_WIDTH - 1);
		//vY[CORNER_RT] = p_request->p_coefficient->fCoeffD * (DST_SUBBLK_WIDTH - 1);
		vY[CORNER_RT] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffD, DST_SUBBLK_WIDTH - 1);
		DBG_IND("P1 x, y = %f, %f\r\n", vX[CORNER_RT], vY[CORNER_RT]);

		// Left/Bottom
		//vX[CORNER_LB] = p_request->p_coefficient->fCoeffB * (DST_SUBBLK_HEIGHT - 1);
		vX[CORNER_LB] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffB, DST_SUBBLK_HEIGHT - 1);
		//vY[CORNER_LB = p_request->p_coefficient->fCoeffE * (DST_SUBBLK_HEIGHT - 1);
		vY[CORNER_LB] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_SUBBLK_HEIGHT - 1);
		DBG_IND("P2 x, y = %f, %f\r\n", vX[CORNER_LB], vY[CORNER_LB]);

		// Right/Bottom
		//vX[CORNER_RB] = p_request->p_coefficient->fCoeffA * (DST_SUBBLK_WIDTH - 1) +
		//				p_request->p_coefficient->fCoeffB * (DST_SUBBLK_HEIGHT - 1);
		vX[CORNER_RB] = float_operation(FLOAT_ADD, float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffA, DST_SUBBLK_WIDTH - 1),
													float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffB, DST_SUBBLK_HEIGHT - 1));

		//vY[CORNER_RB] = p_request->p_coefficient->fCoeffD * (DST_SUBBLK_WIDTH - 1) +
		//				p_request->p_coefficient->fCoeffE * (DST_SUBBLK_HEIGHT - 1);
		vY[CORNER_RB] = float_operation(FLOAT_ADD, float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffD, DST_SUBBLK_WIDTH - 1),
													float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_SUBBLK_HEIGHT - 1));
		DBG_IND("P3 x, y = %f, %f\r\n", vX[CORNER_RB], vY[CORNER_RB]);

	} else {
		// When format is 16 bits (UV packed), use (8 pixel x 8 line) to get source coordinate.

		// Left/Top
		vX[CORNER_LT] = 0;
		vY[CORNER_LT] = 0;

		// Right/Top
		//vX[CORNER_RT] = p_request->p_coefficient->fCoeffA * (DST_SUBBLK_WIDTH / 2 - 1);
		vX[CORNER_RT] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffA, DST_SUBBLK_WIDTH / 2 - 1);
		//vY[CORNER_RT] = p_request->p_coefficient->fCoeffD * (DST_SUBBLK_WIDTH / 2 - 1);
		vY[CORNER_RT] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffD, DST_SUBBLK_WIDTH / 2 - 1);

		// Left/Bottom
		//vX[CORNER_LB] = p_request->p_coefficient->fCoeffB * (DST_SUBBLK_HEIGHT - 1);
		vX[CORNER_LB] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffB, DST_SUBBLK_HEIGHT - 1);
		//vY[CORNER_LB] = p_request->p_coefficient->fCoeffE * (DST_SUBBLK_HEIGHT - 1);
		vY[CORNER_LB] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_SUBBLK_HEIGHT - 1);

		// Right/Bottom
		//vX[CORNER_RB] = p_request->p_coefficient->fCoeffA * (DST_SUBBLK_WIDTH / 2 - 1) +
		//				p_request->p_coefficient->fCoeffB * (DST_SUBBLK_HEIGHT - 1);
		vX[CORNER_RB] = float_operation(FLOAT_ADD, float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffA, DST_SUBBLK_WIDTH / 2 - 1),
													float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffB, DST_SUBBLK_HEIGHT - 1));

		//vY[CORNER_RB] = p_request->p_coefficient->fCoeffD * (DST_SUBBLK_WIDTH / 2 - 1) +
		//				p_request->p_coefficient->fCoeffE * (DST_SUBBLK_HEIGHT - 1);
		vY[CORNER_RB] = float_operation(FLOAT_ADD, float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffD, DST_SUBBLK_WIDTH / 2 - 1),
													float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_SUBBLK_HEIGHT - 1));
	}

	// Use 4 source corner coordinate to get min/max x/y coordinate
	for (i = 0; i < sizeof(vX) / sizeof(vX[0]); i++) {
		if (float_comparison(FLOAT_LESS, vX[i], min_x)/*vX[i] < min_x*/) {
			min_x = vX[i];
		}
		if (float_comparison(FLOAT_GREATER, vX[i], max_x)/*vX[i] > max_x*/) {
			max_x = vX[i];
		}

		if (float_comparison(FLOAT_LESS, vY[i], min_y)/*vY[i] < min_y*/) {
			min_y = vY[i];
		}
		if (float_comparison(FLOAT_GREATER, vY[i], max_y)/*vY[i] > max_y*/) {
			max_y = vY[i];
		}
	}

	// Calculate "byte" width required for input affine coefficents
	if (p_request->format == AFFINE_FORMAT_FORMAT_8BITS) {
		// width + 1 byte of left + 1 byte of right
		width = float_ceil(float_operation(FLOAT_SUB, max_x, min_x)/*max_x - min_x*/) + 2;
	} else {
		// width + 1 byte of left + 1 byte of right
		// But 16 bits use (8 pixel x 8 line) to get source mapping
		// so (max_x-min_x) need multiply 2 to get byte width
		width = float_ceil(float_operation(FLOAT_SUB, max_x, min_x)/*max_x - min_x*/) * 2 + 2;
	}
	height = float_ceil(float_operation(FLOAT_SUB, max_y, min_y)/*max_y - min_y*/) + 2;

	if (width > SRC_SUBBLK_WIDTH) {
		if (b_msg) {
			DBG_ERR("transformed sub block width %d should <= %d\r\n", (int)width, SRC_SUBBLK_WIDTH);
			//DBG_ERR("coefficients are %f %f %f %f %f %f\r\n",
			//		p_request->p_coefficient->fCoeffA,
			//		p_request->p_coefficient->fCoeffB,
			//		p_request->p_coefficient->fCoeffC,
			//		p_request->p_coefficient->fCoeffD,
			//		p_request->p_coefficient->fCoeffE,
			//		p_request->p_coefficient->fCoeffF);
		}
		return E_NOSPT;
	}
	if (height > SRC_SUBBLK_HEIGHT) {
		if (b_msg) {
			DBG_ERR("transformed sub block height %d should <= %d\r\n", (int)height, SRC_SUBBLK_HEIGHT);
			//DBG_ERR("coefficients are %f %f %f %f %f %f\r\n",
			//		p_request->p_coefficient->fCoeffA,
			//		p_request->p_coefficient->fCoeffB,
			//		p_request->p_coefficient->fCoeffC,
			//		p_request->p_coefficient->fCoeffD,
			//		p_request->p_coefficient->fCoeffE,
			//		p_request->p_coefficient->fCoeffF);
		}
		return E_NOSPT;
	}

	sub_blk_height = height;

	// Calculate sub block vector for H/W requirement
	if (float_comparison(FLOAT_EQUAL, min_y, vY[CORNER_LT])/*min_y == vY[CORNER_LT]*/) {
		//sub_blk_vector_x = vX[CORNER_LT] - min_x + 1;
		sub_blk_vector_x = float_to_int(float_operation(FLOAT_SUB, vX[CORNER_LT], min_x)) + 1;
		sub_blk_vector_y = 0;
	} else if (float_comparison(FLOAT_EQUAL, min_x, vX[CORNER_LT])/*min_x == vX[CORNER_LT]*/) {
		sub_blk_vector_x = 0;
		//sub_blk_vector_y = vY[CORNER_LT] - min_y + 1;
		sub_blk_vector_y = float_to_int(float_operation(FLOAT_SUB, vY[CORNER_LT] , min_y)) + 1;
	} else {
		//sub_blk_vector_x = vX[CORNER_LT] - min_x + 1;
		sub_blk_vector_x = float_to_int(float_operation(FLOAT_SUB, vX[CORNER_LT], min_x)) + 1;
		//sub_blk_vector_y = vY[CORNER_LT] - min_y + 1;
		sub_blk_vector_y = float_to_int(float_operation(FLOAT_SUB, vY[CORNER_LT] , min_y)) + 1;
	}

	return E_OK;
}

static ER affine_chk_vertical_scale(PAFFINE_REQUEST p_request, BOOL b_msg)
{
	FLOAT vY[2];
	FLOAT min_y = AFFINE_FLT_MAX, max_y = -AFFINE_FLT_MAX;
	UINT32 int_min_y, int_max_y;
	UINT32 i;

	// Calculate corner coordinate
	if (p_request->format == AFFINE_FORMAT_FORMAT_8BITS) {
		// Left
		vY[0] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_1ST_OUT_HEIGHT);
		DBG_IND("P2 y = %f\r\n", vY[0]);

		// Right
		vY[1] = float_operation(FLOAT_ADD, float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffD, DST_SUBBLK_WIDTH - 1),
													float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_1ST_OUT_HEIGHT));
		DBG_IND("P3 y = %f\r\n", vY[1]);
	} else {
		// Left
		vY[0] = float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_1ST_OUT_HEIGHT);
		DBG_IND("16-P2 y = %f\r\n", vY[0]);

		// Right
		vY[1] = float_operation(FLOAT_ADD, float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffD, DST_SUBBLK_WIDTH / 2 - 1),
													float_operation(FLOAT_MUL, p_request->p_coefficient->fCoeffE, DST_1ST_OUT_HEIGHT));
		DBG_IND("16-P3 y = %f\r\n", vY[1]);
	}

	// Get min/max y coordinate
	for (i = 0; i < sizeof(vY) / sizeof(vY[0]); i++) {
		if (float_comparison(FLOAT_LESS, vY[i], min_y)/*vY[i] < min_y*/) {
			min_y = vY[i];
		}
		if (float_comparison(FLOAT_GREATER, vY[i], max_y)/*vY[i] > max_y*/) {
			max_y = vY[i];
		}
	}

	int_min_y = float_floor(min_y);
	int_max_y = float_floor(max_y);

	// Both two point should NOT located in 1st ping-pong buffer (because it is flushed out)
	if (int_min_y < SRC_1ST_IN_HEIGHT) {
		if (b_msg) {
			DBG_ERR("Not support vertical scale rate > 1.14\r\n");
		}
		return E_NOSPT;
	}
	if (int_max_y < SRC_1ST_IN_HEIGHT) {
		if (b_msg) {
			DBG_ERR("Not support vertical scale rate > 1.14\r\n");
		}
		return E_NOSPT;
	}

	return E_OK;
}


/**
    @addtogroup mIDrvIPP_Affine
*/
//@{

/*
    Affine attach.

    Attach affine.

    @param[in] id				affine controller identifier

    @return
        - @b E_OK: Done with no error.
        - @b Others: Error occured.
*/
static ER affine_attach(AFFINE_ID id)
{
	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	// Enable Affine Clock
	affine_platform_clk_enable(id);

	return E_OK;
}

/*
    Affine detach.

    detach affine.

    @return
        - @b E_OK: Done with no error.
        - @b Others: Error occured.
*/
static ER affine_detach(AFFINE_ID id)
{
	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	// Disable Graphic Clock
	affine_platform_clk_disable(id);

	return E_OK;
}


/**
    Affine open

    @return
        - @b E_OK: open success
        - @b Else: open fail
*/
ER affine_open(AFFINE_ID id)
{
	T_AFFINE_INTSTS_REG int_sts = {0};
	T_AFFINE_INTEN_REG int_en = {0};
	ER ret;

	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	// Enter critical section
	ret = affine_platform_oc_sem_wait(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d lock fail\r\n", __func__, id);
		return ret;
	}

	if (++affine_open_counter > 1) {
		// Leave critical section
		ret = affine_platform_oc_sem_signal(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
			return ret;
		}
		return E_OK;
	}

	// Select clock source
	affine_platform_clk_set_freq(id, affine_clk_sel[id]);

	// Attach driver
	affine_attach(id);

	// Clear Interrupt Status
	if (eAffine_reg_domain == AFFINE_REG_DOMAIN_LL) {
		int_sts.bit.INT_STS = 1;
		int_sts.bit.LLEND_INT_STS = 1;
		int_sts.bit.LLERROR_INT_STS = 1;
		affine_setreg(id, AFFINE_INTSTS_REG_OFS, int_sts.reg);
	}
	else {
		int_sts.bit.INT_STS = 1;
		affine_setreg(id, AFFINE_INTSTS_REG_OFS, int_sts.reg);
	}

	// Enable Interrupt
	if (eAffine_reg_domain == AFFINE_REG_DOMAIN_LL) {
		int_en.bit.INT_EN = 1;
		int_en.bit.LLEND_INT_EN = 1;
		int_en.bit.LLERROR_INT_EN = 1;
		affine_setreg(id, AFFINE_INTEN_REG_OFS, int_en.reg);
	}
	else {
		int_en.bit.INT_EN = 1;
		affine_setreg(id, AFFINE_INTEN_REG_OFS, int_en.reg);
	}

	// Enable interrupt
	affine_platform_int_enable(id);

	// Leave critical section
	ret = affine_platform_oc_sem_signal(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
		return ret;
	}


	return E_OK;
}
EXPORT_SYMBOL(affine_open);

/**
    Affine close

    @return
        - @b E_OK: close success
        - @b E_OACV: driver already closed
        - @b Else: close fail
*/
ER affine_close(AFFINE_ID id)
{
	ER ret;

	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	// Enter critical section
	ret = affine_platform_oc_sem_wait(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d lock fail\r\n", __func__, id);
		return ret;
	}

	if (affine_open_counter == 0) {
		// Leave critical section
		ret = affine_platform_oc_sem_signal(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
			return ret;
		}
		return E_OK;
	} else if (--affine_open_counter != 0) {
		// Leave critical section
		ret = affine_platform_oc_sem_signal(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
			return ret;
		}
		return E_OK;
	}

	// Release interrupt
	affine_platform_int_disable(id);

	affine_detach(id);

	// Leave critical section
	ret = affine_platform_oc_sem_signal(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
		return ret;
	}

	return E_OK;
}
EXPORT_SYMBOL(affine_close);

/**
    Set affine configuration

    @param[in] config_id         configuration identifier
    @param[in] config_context    configuration context for configID

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: config_id is not supported
*/
ER affine_setconfig(AFFINE_ID id, AFFINE_CONFIG_ID config_id, UINT32 config_context)
{
	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	switch (config_id) {
	case AFFINE_CONFIG_ID_FREQ:
		if (config_context < 240) {
			DBG_WRN("input frequency %d round to 240MHz\r\n", (int)config_context);
			config_context = 240;
		} else if (config_context == 240) {
			config_context = 240;
		} else if (config_context < 320) {
			DBG_WRN("input frequency %d round to 240MHz\r\n",  (int)config_context);
			config_context = 240;
		} else if (config_context == 320) {
			config_context = 320;
		} else if (config_context < 360) {
			DBG_WRN("input frequency %d round to 320MHz\r\n", (int)config_context);
			config_context = 320;
		} else if (config_context == 360) {
			config_context = 360;
		} else if (config_context < 480) {
			DBG_WRN("input frequency %d round to 360MHz(PLL13)\r\n", (int)config_context);
			config_context = 360;
		} else {
			DBG_WRN("input frequency %d round to 480MHz\r\n", (int)config_context);
			config_context = 480;
		}
		affine_clk_sel[id] = config_context;
		break;
	case AFFINE_CONFIG_ID_COEFF_A:
		direct_coeff_a = config_context;
		break;
	case AFFINE_CONFIG_ID_COEFF_B:
		direct_coeff_b = config_context;
		break;
	case AFFINE_CONFIG_ID_COEFF_C:
		direct_coeff_c = config_context;
		break;
	case AFFINE_CONFIG_ID_COEFF_D:
		direct_coeff_d = config_context;
		break;
	case AFFINE_CONFIG_ID_COEFF_E:
		direct_coeff_e = config_context;
		break;
	case AFFINE_CONFIG_ID_COEFF_F:
		direct_coeff_f = config_context;
		break;
	case AFFINE_CONFIG_ID_SUBBLK1:
		if (config_context < SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MIN) { //6*10)
			config_context = SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MIN; //6*10;
		}
		if (config_context > SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MAX) { //12*26 0x138
			config_context = SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MAX;
		}
		sub_blk_1 = config_context;
		break;
	case AFFINE_CONFIG_ID_SUBBLK2:
		sub_blk_2 = config_context;
		break;
	case AFFINE_CONFIG_ID_DIRECT_COEFF:
		if (config_context == TRUE) {
			coeff_direct_path = TRUE;
		} else {
			coeff_direct_path = FALSE;
		}
		break;
	default:
		return E_NOSPT;
	}

	return E_OK;
}
EXPORT_SYMBOL(affine_setconfig);

/**
    Get affine configuration

    @param[in] config_id         configuration identifier
    @param[out] pconfig_context  configuration context for config_id

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: config_id is not supported
*/
ER affine_getconfig(AFFINE_ID id, AFFINE_CONFIG_ID config_id, UINT32 *p_config_context)
{
	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	affine_platform_clk_get_freq(id, p_config_context);

	return E_OK;
}
EXPORT_SYMBOL(affine_getconfig);

/**
    Affine trial

    Trial if affine request structure is valid.

    @param[in] pRequest         affine request descriptor

    @return
        - @b E_OK: pRequest is valid
        - @b E_NOSPT: pRequest is not valid, pass it to affine_request() will NOP
*/
ER affine_trial(AFFINE_ID id, PAFFINE_REQUEST p_request)
{
	ER ret;

	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	ret = affine_lock(id);
	if (ret != E_OK) {
		return ret;
	}

	ret = affine_chk_parameter(p_request, FALSE);
	if (ret != E_OK) {
		affine_unlock(id);
		return E_NOSPT;
	}

	ret = affine_chk_vertical_scale(p_request, FALSE);
	if (ret != E_OK) {
		affine_unlock(id);
		return E_NOSPT;
	}

	ret = affine_chk_subblock(p_request, FALSE);
	if (ret != E_OK) {
		affine_unlock(id);
		return E_NOSPT;
	}

	ret = affine_unlock(id);
	if (ret != E_OK) {
		affine_unlock(id);
		return ret;
	}

	return E_OK;
}

/**
    Affine request

    @param[in] p_request         affine request descriptor

    @return
        - @b E_OK: success
        - @b E_OACV: driver is NOT opened
        - @b Else: fail
*/
ER affine_request(AFFINE_ID id, PAFFINE_REQUEST p_request)
{
	ER ret;
	UINT32 temp;
	UINT32 coeff_a, coeff_b, coeff_c, coeff_d, coeff_e, coeff_f;
	UINT32 reg_count = 16;
	UINT32 job_idx = 0;
	PAFFINE_REQUEST p_reqt_ptr;
	T_AFFINE_CTRL_REG ctrl = {0};
	T_AFFINE_CONFIG_REG config_reg = {0};
	T_AFFINE_IMG_SRC_ADDR_REG src_addr_reg = {0};
	T_AFFINE_IMG_SRC_LOFF_REG src_lof_reg = {0};
	T_AFFINE_LCNT_REG height_reg = {0};
	T_AFFINE_XRGN_REG width_reg = {0};
	T_AFFINE_IMG_DST_ADDR_REG dst_addr_reg = {0};
	T_AFFINE_IMG_DST_LOFF_REG dst_lof_reg = {0};
	T_AFFINE_SUBBLOCK_REG sub_blk_reg = {0};
	T_AFFINE_SUBBLOCK2_REG sub_blk_2_reg = {0};

	//Set p_affine_llc_cmd
	p_affine_llc_cmd = (UINT64*)affine_ll_buffer;

	// Set p_reqt_ptr
	p_reqt_ptr = p_request;

	if (id > AFFINE_ID_1) {
		return E_NOSPT;
	}

	if (affine_open_counter == 0) {
		DBG_ERR("driver NOT opened\r\n");
		return E_OACV;
	}

	//ret = affine_lock(id);
	while (p_reqt_ptr != NULL) {
		if (job_idx * reg_count * 64 >= AFFINE_LL_BUF_SIZE - reg_count * 64) {
			DBG_ERR("LLC buffer overflow\r\n");
			return E_PAR;
		}

		ret = affine_chk_parameter(p_reqt_ptr, TRUE);
		if (ret != E_OK) {
			return ret;
		}

		if (coeff_direct_path == FALSE) {
			ret = affine_chk_vertical_scale(p_reqt_ptr, TRUE);
			if (ret != E_OK)
				return ret;

			ret = affine_chk_subblock(p_reqt_ptr, TRUE);
			if (ret != E_OK)
				return ret;
		}

		// Transform coefficients
		if (coeff_direct_path == TRUE) {
			coeff_a = direct_coeff_a;
			coeff_b = direct_coeff_b;
			coeff_c = direct_coeff_c;
			coeff_d = direct_coeff_d;
			coeff_e = direct_coeff_e;
			coeff_f = direct_coeff_f;

			sub_blk_height = sub_blk_1 / SRC_BURST_WIDTH_WORDS;
			sub_blk_vector_x = sub_blk_2 & 0x3F;
			sub_blk_vector_y = (sub_blk_2 >> 16) & 0x3F;
		} else {
			temp = affine_float_to_q16p15(p_reqt_ptr->p_coefficient->fCoeffA);
			coeff_a = affine_q16p15_to_q1p15(temp);
			temp = affine_float_to_q16p15(p_reqt_ptr->p_coefficient->fCoeffB);
			coeff_b = affine_q16p15_to_q1p15(temp);
			temp = affine_float_to_q16p15(p_reqt_ptr->p_coefficient->fCoeffC);
			coeff_c = affine_q16p15_to_q14p15(temp);
			temp = affine_float_to_q16p15(p_reqt_ptr->p_coefficient->fCoeffD);
			coeff_d = affine_q16p15_to_q1p15(temp);
			temp = affine_float_to_q16p15(p_reqt_ptr->p_coefficient->fCoeffE);
			coeff_e = affine_q16p15_to_q1p15(temp);
			temp = affine_float_to_q16p15(p_reqt_ptr->p_coefficient->fCoeffF);
			coeff_f = affine_q16p15_to_q14p15(temp);
		}

		if (p_reqt_ptr->format == AFFINE_FORMAT_FORMAT_8BITS) {
			config_reg.bit.OP_PRECISION = OP_PRECISION_8BITS;
		} else {
			config_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		}

		src_addr_reg.bit.IMG_SRC_ADDR = affine_platform_va2pa(p_reqt_ptr->p_src_img->uiImageAddr);
		src_lof_reg.bit.IMAGE_SRC_LOFF = p_reqt_ptr->p_src_img->uiLineOffset;
		width_reg.bit.ACT_WIDTH = (p_reqt_ptr->width / /*DST_SUBBLK_WIDTH*/16) - 1;
		height_reg.bit.ACT_HEIGHT = (p_reqt_ptr->height / /*DST_SUBBLK_HEIGHT*/8) - 1;
		dst_addr_reg.bit.IMG_DST_ADDR = affine_platform_va2pa(p_reqt_ptr->p_dst_img->uiImageAddr);
		dst_lof_reg.bit.IMG_DST_LOFF = p_reqt_ptr->p_dst_img->uiLineOffset;
		// Ensure sub block height >= min source block burst height
		// Else affine source block mechanism may NOT start to read from DRAM to source block FIFO
		if (sub_blk_1 < SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MIN) { //if (sub_blk_height < SRC_BURST_HEIGHT_MIN)
			sub_blk_1 = SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MIN; //sub_blk_height = SRC_BURST_HEIGHT_MIN;
		}
		if (sub_blk_1 > SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MAX) { //12*26 0x138
			sub_blk_1 = SRC_BURST_WIDTH_WORDS * SRC_BURST_HEIGHT_MAX;
		}
		sub_blk_reg.bit.BURST_LENGTH = sub_blk_1;//SRC_BURST_WIDTH_WORDS * sub_blk_height;
		sub_blk_2_reg.bit.VECTOR_X = sub_blk_vector_x;
		sub_blk_2_reg.bit.VECTOR_Y = sub_blk_vector_y;


		// Flush CPU cache
		affine_flush_cache(p_reqt_ptr);

		// Fill affine registers
		p_affine_llc_cmd[job_idx * reg_count + 0] = llc_update(0xf, AFFINE_CONFIG_REG_OFS, config_reg.reg);

		p_affine_llc_cmd[job_idx * reg_count + 1] = llc_update(0xf, AFFINE_COEFF_REG_OFS, coeff_a);
		p_affine_llc_cmd[job_idx * reg_count + 2] = llc_update(0xf, AFFINE_COEFF2_REG_OFS, coeff_b);
		p_affine_llc_cmd[job_idx * reg_count + 3] = llc_update(0xf, AFFINE_COEFF3_REG_OFS, coeff_c);
		p_affine_llc_cmd[job_idx * reg_count + 4] = llc_update(0xf, AFFINE_COEFF4_REG_OFS, coeff_d);
		p_affine_llc_cmd[job_idx * reg_count + 5] = llc_update(0xf, AFFINE_COEFF5_REG_OFS, coeff_e);
		p_affine_llc_cmd[job_idx * reg_count + 6] = llc_update(0xf, AFFINE_COEFF6_REG_OFS, coeff_f);

		p_affine_llc_cmd[job_idx * reg_count + 7] = llc_update(0xf, AFFINE_IMG_SRC_ADDR_REG_OFS, src_addr_reg.reg);
		p_affine_llc_cmd[job_idx * reg_count + 8] = llc_update(0xf, AFFINE_IMG_SRC_LOFF_REG_OFS, src_lof_reg.reg);
		p_affine_llc_cmd[job_idx * reg_count + 9] = llc_update(0xf, AFFINE_LCNT_REG_OFS, height_reg.reg);
		p_affine_llc_cmd[job_idx * reg_count + 10] = llc_update(0xf, AFFINE_XRGN_REG_OFS, width_reg.reg);
		p_affine_llc_cmd[job_idx * reg_count + 11] = llc_update(0xf, AFFINE_IMG_DST_ADDR_REG_OFS, dst_addr_reg.reg);
		p_affine_llc_cmd[job_idx * reg_count + 12] = llc_update(0xf, AFFINE_IMG_DST_LOFF_REG_OFS, dst_lof_reg.reg);

		p_affine_llc_cmd[job_idx * reg_count + 13] = llc_update(0xf, AFFINE_SUBBLOCK_REG_OFS, sub_blk_reg.reg);
		p_affine_llc_cmd[job_idx * reg_count + 14] = llc_update(0xf, AFFINE_SUBBLOCK2_REG_OFS, sub_blk_2_reg.reg);

		// Jump to next p_request buffer
		p_reqt_ptr = (PAFFINE_REQUEST)p_reqt_ptr->p_next;

		if (p_reqt_ptr != NULL) {
			// Fill NextLL cmd for Jumping to next LL job address and trigger
			p_affine_llc_cmd[job_idx * reg_count + 15] = llc_next_ll((UINT32)&p_affine_llc_cmd[(job_idx + 1) * reg_count], 0);
		}
		else {
			// Fill Null cmd for trigger fire and assert ll_interrupt after frame done
			p_affine_llc_cmd[job_idx * reg_count + 15] = llc_null(0);
		}

		// Update index
		job_idx++;

	}

	// Flash cache
	affine_platform_dma_flush_mem2dev((UINT32)p_affine_llc_cmd, AFFINE_LL_BUF_SIZE/*sizeof(affine_llc_cmd)*/);

	// Set LLC buffer
	affine_setreg(id, LL_DMA_ADDR_REG_OFS, (REGVALUE)affine_platform_va2pa((UINT32)p_affine_llc_cmd));

	// Trigger Link List
	ctrl.bit.LL_FIRE = TRUE;
	affine_setreg(id, AFFINE_CTRL_REG_OFS, ctrl.reg);

	// Wait operation done
	//affine_platform_flg_wait(id, v_isr_flag[id]);

	//ret = affine_unlock(id);

	return E_OK;
}
//EXPORT_SYMBOL(affine_request);


/**
    Affine internal trigger

    Affine driver internal API.
    This function is shared by affine_enqueue() and affine isr.

    @param[in] id           affine controller identifier
    @param[in] p_param      Description of request

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
        - @b E_NOEXS: Engine not in the ready state.
*/
ER affine_trigger(AFFINE_ID id)
{
	ER ret = E_OK;
	PAFFINE_REQUEST p_request = NULL;
	AFFINE_REQ_LIST_NODE *p_node = NULL;

        if (id > AFFINE_ID_1) {
                DBG_ERR("invalid controller ID: %d\r\n", id);
                return E_NOSPT;
        }

	p_node = affine_platform_get_head(id);

	if (p_node == NULL) return E_NOEXS;

	p_request = (PAFFINE_REQUEST)(&p_node->trig_param);

	if (p_node == NULL || p_request == NULL) return E_NOSPT;

//	DBG_ERR("%s: req src img 0x%x, dst img 0x%x\r\n", __func__, p_request->p_src_img, p_request->p_dst_img);

	do {
			ret = affine_request(id, p_request);
			if (ret != E_OK) {
				DBG_ERR("operation with error %d\r\n", ret);
				break;
			}
	} while (0);

	return ret;
}


/**
    Affine request enqueue

    Affine driver internal API.
    This function is shared by affine_request() and kdrv_affine_trigger().

    @param[in] id           affine controller identifier
    @param[in] p_param      Description of request
    @param[in] p_cb_func    request complete callback

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
        - @b E_NOEXS: Engine not in the ready state.
*/
ER affine_enqueue(AFFINE_ID id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
                KDRV_CALLBACK_FUNC *p_cb_func)
{
	ER ret = E_OK;
	ER ret_unlock = E_OK;
	PAFFINE_REQUEST p_request = NULL;
	UINT32 spin_flags;

        if (id > AFFINE_ID_1) {
                DBG_ERR("invalid controller ID: %d\r\n", id);
                return E_NOSPT;
        }

	if (p_param == NULL) {
                DBG_ERR("NULL p_param\r\n");
                return E_NOEXS;
	}

	if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
		ret = affine_lock(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d lock fail\r\n", __func__, id);
			return ret;
		}
	}

	//
        // enqueue p_param (enter critical section)
        //
        spin_flags = affine_platform_spin_lock(id);

	if (affine_platform_list_empty(id)) {
                // if queue is empty, immediately fed to hw and trigger
                p_request = (PAFFINE_REQUEST)p_param;
        }

	if (affine_platform_add_list(id, p_param, p_cb_func) != E_OK) {
                affine_platform_spin_unlock(id, spin_flags);
                if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
                        ret =  affine_unlock(id);
                        if (ret != E_OK) {
				DBG_ERR("%s: %d lock fail2\r\n", __func__, id);
                                return ret;
                        }
                }
                DBG_ERR("%s: affine %d add list fail\r\n", __func__, id);
                return E_SYS;
        }

#if 0
	if (p_request != NULL) {
		ret = affine_trigger(id);
	}
#endif
	//
        // enqueue p_param (exit critical section)
        //
        affine_platform_spin_unlock(id, spin_flags);

#if 1
	// If queue is empty before we assert, trigger hw now
	if (p_request != NULL) {
		ret = affine_trigger(id);
		if (ret != E_OK) {
			spin_flags = affine_platform_spin_lock(id);
			affine_platform_del_list(id);
			affine_platform_spin_unlock(id, spin_flags);

			affine_unlock(id);
			return ret;
		}
	} else {
	}
#endif

	// Unlock affine module
	if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
		affine_platform_flg_wait(id, v_isr_flag[id]);

		ret_unlock = affine_unlock(id);
		if (ret_unlock != E_OK) {
			return ret_unlock;
		}
	}

	return ret;
}
EXPORT_SYMBOL(affine_enqueue);

//@}

