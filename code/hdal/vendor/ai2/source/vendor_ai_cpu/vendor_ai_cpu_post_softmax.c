/**
	@brief Source file of SoftMax of vendor net postprocessing sample.

	@file net_post_sample_softmax.c

	@ingroup net_post_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <limits.h>
#include <arm_neon.h>
#include "hd_type.h"

#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][cpu]"
#include "vendor_ai_debug.h"
//=============================================================

#include "../include/vendor_ai_net_util.h" //for debug


#if !USE_NEON
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
// y=exp(x)  x: (16 bits, 9 fraction bits) reduced to (13 bits, 6 fraction bits)  y: 16 bits, 14 fraction bits
// exp((i*16+j)*8/512)*16384==0  for i=-511:-42, j=-15:0
// exp((i*16+j)*8/512)*16384     for i=-41:0, j=-15:0     (the following table)
#define VENDOR_AIS_EXPTBL_OFFSET    (470 * 16)  // 470=512-42; 16: values per row
static INT16 g_vendor_ais_exptbl[] = {
	0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,
	4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,
	7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,
	9,9,9,10,10,10,10,10,10,11,11,11,11,11,11,12,
	12,12,12,12,13,13,13,13,13,14,14,14,14,14,15,15,
	15,15,16,16,16,16,17,17,17,17,18,18,18,19,19,19,
	19,20,20,20,21,21,21,22,22,22,23,23,24,24,24,25,
	25,25,26,26,27,27,27,28,28,29,29,30,30,31,31,32,
	32,33,33,34,34,35,35,36,36,37,38,38,39,39,40,41,
	41,42,43,43,44,45,45,46,47,47,48,49,50,51,51,52,
	53,54,55,56,56,57,58,59,60,61,62,63,64,65,66,67,
	68,69,70,71,72,74,75,76,77,78,80,81,82,83,85,86,
	87,89,90,92,93,94,96,97,99,101,102,104,105,107,109,110,
	112,114,116,118,119,121,123,125,127,129,131,133,135,137,140,142,
	144,146,149,151,153,156,158,161,163,166,168,171,174,176,179,182,
	185,188,191,194,197,200,203,206,209,213,216,220,223,227,230,234,
	237,241,245,249,253,257,261,265,269,273,278,282,286,291,295,300,
	305,310,314,319,324,330,335,340,345,351,356,362,368,373,379,385,
	391,398,404,410,417,423,430,437,443,450,458,465,472,480,487,495,
	503,510,518,527,535,543,552,561,569,578,588,597,606,616,625,635,
	645,655,666,676,687,698,709,720,731,743,754,766,778,791,803,816,
	829,842,855,868,882,896,910,924,939,954,969,984,999,1015,1031,1047,
	1064,1081,1098,1115,1133,1150,1168,1187,1206,1225,1244,1263,1283,1304,1324,1345,
	1366,1388,1409,1432,1454,1477,1500,1524,1548,1572,1597,1622,1648,1674,1700,1727,
	1754,1782,1810,1838,1867,1897,1926,1957,1988,2019,2051,2083,2116,2149,2183,2217,
	2252,2288,2324,2360,2398,2435,2474,2513,2552,2592,2633,2675,2717,2760,2803,2847,
	2892,2937,2984,3031,3078,3127,3176,3226,3277,3329,3381,3434,3488,3543,3599,3656,
	3713,3772,3831,3892,3953,4015,4078,4143,4208,4274,4341,4410,4479,4550,4621,4694,
	4768,4843,4919,4997,5076,5155,5237,5319,5403,5488,5574,5662,5751,5842,5934,6027,
	6122,6219,6317,6416,6517,6620,6724,6830,6937,7047,7158,7270,7385,7501,7619,7739,
	7861,7985,8111,8238,8368,8500,8634,8770,8908,9048,9191,9335,9482,9632,9783,9937,
	10094,10253,10414,10578,10745,10914,11086,11261,11438,11618,11801,11987,12176,12367,12562,12760,
	12961,13165,13372,13583,13797,14014,14235,14459,14687,14918,15153,15391,15634,15880,16130,16384,
};

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
static HD_RESULT vendor_ais_submax_s16(INT16 *p_in, INT16 *p_out, INT32 len, UINT8 in_q)
{
#define NUM_LANE        8
#define NUM_VEC         4
#define BATCH           (NUM_LANE * NUM_VEC)
#define EXPTBL_BITS     13
#define EXPTBL_Q        9
#define EXPTBL_SIZE     (1 << EXPTBL_BITS)
#define IN_BITS         16

	int16_t *p_src, *p_dst, *p_data;
	int16_t *p_lut;
	int32_t size, num;
	int16_t shift = (EXPTBL_BITS - IN_BITS) + (EXPTBL_Q - in_q);
	int16_t maxval;
	int32_t idx;
	int32_t i, v;
	int16x8_t     vdata, vshift, vmax;
	int16x8x4_t   vdataset;

	// scale data and get max
	vshift = vmovq_n_s16(shift);
	vmax = vmovq_n_s16(SHRT_MIN);
	p_src = p_in;
	p_dst = p_out;
	size = len;
	num = ALIGN_FLOOR(size, BATCH);
	for (i = 0; i < num; i += BATCH) {
		vdataset = vld4q_s16(p_src);
		for (v = 0; v < NUM_VEC; v++) {
			vdataset.val[v] = vshlq_s16(vdataset.val[v], vshift);
			vmax = vmaxq_s16(vmax, vdataset.val[v]);
		}
		vst4q_s16(p_dst, vdataset);
		p_src += BATCH;
		p_dst += BATCH;
	}

	size -= num;
	num = ALIGN_FLOOR(size, NUM_LANE);
	for (i = 0; i < num; i += NUM_LANE) {
		vdata = vld1q_s16(p_src);
		vdata = vshlq_s16(vdata, vshift);
		vmax = vmaxq_s16(vmax, vdata);
		vst1q_s16(p_dst, vdata);
		p_src += NUM_LANE;
		p_dst += NUM_LANE;
	}

	maxval = SHRT_MIN;
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 0));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 1));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 2));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 3));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 4));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 5));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 6));
	maxval = MAX(maxval, vgetq_lane_s16(vmax, 7));

	size -= num;
	for (i = 0; i < size; i++) {
		p_dst[i] = p_src[i] >> (-shift);
		maxval = MAX(maxval, p_dst[i]);
	}

	// sub max
	vmax = vmovq_n_s16(maxval);
	p_data = p_out;
	size = len;
	num = ALIGN_FLOOR(size, BATCH);
	for (i = 0; i < num; i += BATCH) {
		vdataset = vld4q_s16(p_data);
		for (v = 0; v < NUM_VEC; v++) {
			vdataset.val[v] = vsubq_s16(vdataset.val[v], vmax);
		}
		vst4q_s16(p_data, vdataset);
		p_data += BATCH;
	}

	size -= num;
	num = ALIGN_FLOOR(size, NUM_LANE);
	for (i = 0; i < num; i += NUM_LANE) {
		vdata = vld1q_s16(p_data);
		vdata = vsubq_s16(vdata, vmax);
		vst1q_s16(p_data, vdata);
		p_data += NUM_LANE;
	}

	size -= num;
	for (i = 0; i < size; i++) {
		p_data[i] -= maxval;
	}

	// look up exp table
	p_lut = g_vendor_ais_exptbl;
	for (i = 0; i < len; i++) {
		idx = (int32_t)p_out[i] + (EXPTBL_SIZE - 1);
		if (idx > EXPTBL_SIZE - 1) {
			DBG_ERR("idx %d over limit %d\r\n", (int)idx, EXPTBL_SIZE - 1);
			return HD_ERR_NG;
		}
		idx -= VENDOR_AIS_EXPTBL_OFFSET;
		p_out[i] = (idx < 0) ? 0 : p_lut[idx];
	}

	return HD_OK;
}

static UINT32 vendor_ais_checkparm(NN_SOFTMAX_PARM *p_parm)
{
	UINT32 num = p_parm->batch_num;
	UINT32 channel = p_parm->channel;

	if (num <= 0) {
		DBG_ERR("batch num should be positive, but input is %ld\r\n", num);
		return 2;
	}
	if (channel <= 0) {
		DBG_ERR("channel should be positive, but input is %ld\r\n", channel);
		return 2;
	}
	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

HD_RESULT vendor_ais_softmax_process(NN_SOFTMAX_PARM *p_parm)
{
#if CNN_25_MATLAB
	INT16 *p_in, *p_out;
	INT32 num, channels;
	UINT8 in_q;
	INT32 i;

	if (vendor_ais_checkparm(p_parm)) {
		return HD_ERR_PARAM;
	}

	p_in     	= (INT16*)p_parm->in_addr;
	p_out    	= (INT16*)p_parm->out_addr;
	num       	= p_parm->batch_num;
	channels	= p_parm->channel;
	in_q      	= p_parm->in_bit_fmt.frac_bits;
	

	for (i = 0; i < num; i++) {
		vendor_ais_submax_s16(p_in, p_out, channels, in_q);

		p_in  += channels;
		p_out += channels;
	}

#else
	INT16 *p_in, *p_out;
	INT32 width, height, channels, num;
	INT8 in_q;
	INT32 i, j, k;

	if (vendor_ais_checkparm(p_parm)) {
		return HD_ERR_PARAM;
	}

	p_in        = (INT16 *)p_parm->in_addr;
	p_out       = (INT16 *)p_parm->out_addr;
	width       = p_parm->width;
	height      = p_parm->height;
	channels    = p_parm->channel;
	num         = p_parm->batch_num;
	in_q        = p_parm->in_bit_fmt.frac_bits;

	if (width == 1 && height == 1) {
		for (i = 0; i < num; i++) {
			vendor_ais_submax_s16(p_in, p_out, channels, in_q);

			p_in  += channels;
			p_out += channels;
		}
	} else {
		for (i = 0; i < num; i++) {
			for (j = 0; j < channels; j++) {
				for (k = 0; k < height; k++) {
					vendor_ais_submax_s16(p_in, p_out, width, in_q);

					p_in  += width;
					p_out += width;
				}
			}
		}
	}
#endif

	return HD_OK;
}

#endif // !USE_NEON

