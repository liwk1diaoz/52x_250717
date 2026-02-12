/**
	@brief Source file of sorting top-N classes of accuracy.

	@file vendor_ai_post_accuracy.c

	@ingroup vendor_ai

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <arm_neon.h>
#include <string.h>
#include "hd_type.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu/vendor_ai_cpu_builtin.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][cpu]"
#include "vendor_ai_debug.h"
//=============================================================

#include "../include/vendor_ai_net_util.h" //for debug

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
#if USE_NEON
static VOID vendor_ais_sort_result(FLOAT *p_in, INT32 len, INT32 *p_idx, INT32 top_n, VENDOR_AIS_OUTPUT_CLASS *p_classes)
{
	INT32 tmp;
	INT32 i, j;

	for (i = 0; i < len; i++) {
		p_idx[i] = i;
	}

	// sort top results
	for (i = 0; i < top_n; i++) {
		for (j = len - 1; j > i; j--) {
			if (p_in[p_idx[j]] > p_in[p_idx[j - 1]]) {
				SWAP(p_idx[j], p_idx[j - 1], tmp);
			}
		}
	}

	// copy top results to output buffer
	for (i = 0; i < top_n; i++) {
		p_classes[i].no = p_idx[i];
		p_classes[i].score = p_in[p_idx[i]];

		//DBG_IND("no=%ld, score=%f\r\n", p_classes[i].no, p_classes[i].score);
	}
}

#else // USE_NEON
static INT32 vendor_ais_get_expsum(INT16 *p_in, INT32 len)
{
#define NUM_LANE        8
#define NUM_VEC         4
#define BATCH           (NUM_LANE * NUM_VEC)

	int16_t *p_data;
	int32_t size, num;
	int32_t expsum;
	int32_t i, v;
	int16x8_t     vdata;
	int16x8x4_t   vdataset;
	int32x4_t     vsum;

	// get exp sum
	vsum = vmovq_n_s32(0);
	p_data = p_in;
	size = len;
	num = ALIGN_FLOOR(size, BATCH);
	for (i = 0; i < num; i += BATCH) {
		vdataset = vld4q_s16(p_data);
		for (v = 0; v < NUM_VEC; v++) {
			vsum = vpadalq_s16(vsum, vdataset.val[v]);
		}
		p_data += BATCH;
	}

	size -= num;
	num = ALIGN_FLOOR(size, NUM_LANE);
	for (i = 0; i < num; i += NUM_LANE) {
		vdata = vld1q_s16(p_data);
		vsum = vpadalq_s16(vsum, vdata);
		p_data += NUM_LANE;
	}

	expsum = 0;
	expsum += vgetq_lane_s32(vsum, 0);
	expsum += vgetq_lane_s32(vsum, 1);
	expsum += vgetq_lane_s32(vsum, 2);
	expsum += vgetq_lane_s32(vsum, 3);

	size -= num;
	for (i = 0; i < size; i++) {
		expsum += p_data[i];
	}

	return expsum;
}
#if !CNN_25_MATLAB
static VOID vendor_ais_sort_result(INT16 *p_in, INT32 len, INT32 *p_idx, INT32 top_n, VENDOR_AIS_OUTPUT_CLASS *p_classes)
{
	INT32 expsum;
	INT32 tmp;
	INT32 i, j;

	expsum = vendor_ais_get_expsum(p_in, len);

	for (i = 0; i < len; i++) {
		p_idx[i] = i;
	}

	// sort top results
	for (i = 0; i < top_n; i++) {
		for (j = len - 1; j > i; j--) {
			if (p_in[p_idx[j]] > p_in[p_idx[j - 1]]) {
				SWAP(p_idx[j], p_idx[j - 1], tmp);
			}
		}
	}

	// copy top results to output buffer
	for (i = 0; i < top_n; i++) {
		p_classes[i].no = p_idx[i];
		p_classes[i].score = (FLOAT)p_in[p_idx[i]] / expsum;

		//DBG_IND("no=%ld, score=%f\r\n", p_classes[i].no, p_classes[i].score);
	}
}
#endif
#endif // USE_NEON


HD_RESULT vendor_ais_accuracy_process(VENDOR_AIS_ACCURACY_PARM *p_parm)
{
#if CNN_25_MATLAB
	INT16 *p_in     = p_parm->input;
	VENDOR_AIS_OUTPUT_CLASS *p_classes = p_parm->classes;
	INT32 num       = p_parm->shape.num;
	INT32 channels  = p_parm->shape.channels;
	INT32 top_n     = MIN(channels, p_parm->top_n);
	INT32 *p_idx    = p_parm->class_idx;
	INT32 expsum;
	INT32 tmp;
	INT32 n, i, j;

	for (n = 0; n < num; n++) {
		expsum = vendor_ais_get_expsum(p_in, channels);
		if (expsum == 0) {
			printf("cpu_acc: err, expsum is 0?\n");
			return HD_ERR_ABORT;
		}

		for (i = 0; i < channels; i++) {
			p_idx[i] = i;
		}

		// sort top results
		for (i = 0; i < top_n; i++) {
			for (j = channels - 1; j > i; j--) {
				if (p_in[p_idx[j]] > p_in[p_idx[j - 1]]) {
					SWAP(p_idx[j], p_idx[j - 1], tmp);
				}
			}
		}

		// copy top results to output buffer
		for (i = 0; i < top_n; i++) {
			p_classes[i].no = p_idx[i];
			p_classes[i].score = (FLOAT)p_in[p_idx[i]] / expsum;
		}

		p_in += channels;
		p_classes += top_n;
	}
	p_parm->top_n = top_n;

#else

#if USE_NEON
	FLOAT *p_in     = (FLOAT *)p_parm->in_addr;
#else
	INT16 *p_in     = (INT16 *)p_parm->in_addr;
#endif
	VENDOR_AIS_OUTPUT_CLASS *p_classes = p_parm->classes;
	INT32 num       = p_parm->shape.num;
	INT32 channels  = p_parm->shape.channels;
	INT32 height    = p_parm->shape.height;
	INT32 width     = p_parm->shape.width;
	INT32 top_n     = p_parm->top_n;
	INT32 *p_idx    = p_parm->class_idx;
	INT32 i, j, k;

	if (width == 1 && height == 1) {
		top_n = MIN(channels, top_n);
		for (i = 0; i < num; i++) {
			vendor_ais_sort_result(p_in, channels, p_idx, top_n, p_classes);

			p_in += channels;
			p_classes += top_n;
		}
	} else {
		top_n = MIN(width, top_n);
		for (i = 0; i < num; i++) {
			for (j = 0; j < channels; j++) {
				for (k = 0; k < height; k++) {
					vendor_ais_sort_result(p_in, width, p_idx, top_n, p_classes);

					p_in += width;
					p_classes += top_n;
				}
			}
		}
	}
	p_parm->top_n = top_n;
#endif

	return HD_OK;
}


