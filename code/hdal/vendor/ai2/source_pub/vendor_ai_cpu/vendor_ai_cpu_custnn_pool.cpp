/**
	@brief Source file of Custom NN pooling layer operating using CPU.

	@file custnn_cpu_pool.cpp

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <limits.h>
#include "hd_type.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_cpu_custnn_sample.h"
#include "nvtnn/nvtnn_lib.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define POOL_PRINT_PARM     FALSE

#define PROF                FALSE
#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    gettimeofday(&tstart, NULL);
	#define PROF_END(msg)   gettimeofday(&tend, NULL);      \
			printf("%s time (us): %lu\r\n", msg,            \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif

#define POOL_SCA(val, sca, shf, out, type, min, max)                                \
	if (shr) {                                                                      \
		*(type *)out = CLAMP((val * sca + (1 << (shf - 1))) >>   shf,  min, max);   \
	} else {                                                                        \
		*(type *)out = CLAMP((val * sca                   ) << (-shf), min, max);   \
	}

#define POOL_AVG_SCA(sum, size, sca, shf, out, type, min, max)                                          \
	if (shr) {                                                                                          \
		*(type *)out = CLAMP(((sum + size / 2) / size * sca + (1 << (shf - 1))) >>   shf,  min, max);   \
	} else {                                                                                            \
		*(type *)out = CLAMP(((sum + size / 2) / size * sca                   ) << (-shf), min, max);   \
	}

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
template<typename T, INT32 tmin, INT32 tmax, BOOL shr>
static HD_RESULT custnn_local_max_pool(CUSTNN_POOL_PARM *p_parm, UINT32 in_addr, UINT32 out_addr)
{
	int32_t width         = p_parm->size_width;
	int32_t height        = p_parm->size_height;
	int32_t channels      = p_parm->size_channel;
	int32_t batch         = p_parm->batch_num;
	int32_t in_line_ofs   = p_parm->in_ofs_line_ofs;
	int32_t in_ch_ofs     = p_parm->in_ofs_channel_ofs;
	int32_t in_batch_ofs  = p_parm->in_ofs_batch_ofs;
	int32_t out_line_ofs  = p_parm->out_ofs_line_ofs;
	int32_t out_ch_ofs    = p_parm->out_ofs_channel_ofs;
	int32_t out_batch_ofs = p_parm->out_ofs_batch_ofs;
	uint8_t ker_w         = p_parm->pool_local_ker_w;
	uint8_t ker_h         = p_parm->pool_local_ker_h;
	uint8_t ker_stridex   = p_parm->pool_local_ker_stridex;
	uint8_t ker_stridey   = p_parm->pool_local_ker_stridey;
	int32_t sca           = p_parm->norm_scale;
	int32_t shf           = p_parm->norm_shift + p_parm->pool_pool_shf;
	int32_t lastx, lasty;
	int32_t nin, cin, yin, kin;
	int32_t nout, cout, yout, kout;
	int32_t max;
	int32_t n, c, x, y, kx, ky;

	lastx = (width  - 1) / ker_stridex * ker_stridex;
	lasty = (height - 1) / ker_stridey * ker_stridey;

	nin  = in_addr;
	nout = out_addr;
	for (n = 0; n < batch; n++) {
		cin  = nin;
		cout = nout;
		for (c = 0; c < channels; c++) {
			yin  = cin;
			yout = cout;
			for (y = 0; y < lasty; y += ker_stridey) {
				kout = yout;
				for (x = 0; x < lastx; x += ker_stridex) {
					kin = yin;
					max = tmin;
					for (ky = y; ky < y + ker_h; ky++) {
						for (kx = x; kx < x + ker_w; kx++) {
							if (max < *((T *)kin + kx)) {
								max = *((T *)kin + kx);
							}
						}
						kin += in_line_ofs;
					}
					POOL_SCA(max, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				}

				// last h stride
				// {
					kin = yin;
					max = tmin;
					for (ky = y; ky < y + ker_h; ky++) {
						for (kx = x; kx < width; kx++) {
							if (max < *((T *)kin + kx)) {
								max = *((T *)kin + kx);
							}
						}
						kin += in_line_ofs;
					}
					POOL_SCA(max, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }

				yin  +=  in_line_ofs * ker_stridey;
				yout += out_line_ofs;
			}

			// last v stride
			// {
				kout = yout;
				for (x = 0; x < lastx; x += ker_stridex) {
					kin = yin;
					max = tmin;
					for (ky = y; ky < height; ky++) {
						for (kx = x; kx < x + ker_w; kx++) {
							if (max < *((T *)kin + kx)) {
								max = *((T *)kin + kx);
							}
						}
						kin += in_line_ofs;
					}
					POOL_SCA(max, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				}

				// last h stride
				// {
					kin = yin;
					max = tmin;
					for (ky = y; ky < height; ky++) {
						for (kx = x; kx < width; kx++) {
							if (max < *((T *)kin + kx)) {
								max = *((T *)kin + kx);
							}
						}
						kin += in_line_ofs;
					}
					POOL_SCA(max, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }
			// }

			cin  +=  in_ch_ofs;
			cout += out_ch_ofs;
		}
		nin  +=  in_batch_ofs;
		nout += out_batch_ofs;
	}

	return HD_OK;
}

template<typename T, INT32 tmin, INT32 tmax, BOOL shr>
static HD_RESULT custnn_local_avg_pool(CUSTNN_POOL_PARM *p_parm, UINT32 in_addr, UINT32 out_addr)
{
	int32_t width         = p_parm->size_width;
	int32_t height        = p_parm->size_height;
	int32_t channels      = p_parm->size_channel;
	int32_t batch         = p_parm->batch_num;
	int32_t in_line_ofs   = p_parm->in_ofs_line_ofs;
	int32_t in_ch_ofs     = p_parm->in_ofs_channel_ofs;
	int32_t in_batch_ofs  = p_parm->in_ofs_batch_ofs;
	int32_t out_line_ofs  = p_parm->out_ofs_line_ofs;
	int32_t out_ch_ofs    = p_parm->out_ofs_channel_ofs;
	int32_t out_batch_ofs = p_parm->out_ofs_batch_ofs;
	uint8_t ker_w         = p_parm->pool_local_ker_w;
	uint8_t ker_h         = p_parm->pool_local_ker_h;
	uint8_t ker_stridex   = p_parm->pool_local_ker_stridex;
	uint8_t ker_stridey   = p_parm->pool_local_ker_stridey;
	uint8_t tpad          = p_parm->pool_local_pad_top_pad_num;
	uint8_t bpad          = p_parm->pool_local_pad_bot_pad_num;
	uint8_t lpad          = p_parm->pool_local_pad_left_pad_num;
	uint8_t rpad          = p_parm->pool_local_pad_right_pad_num;
	int32_t sca           = p_parm->norm_scale;
	int32_t shf           = p_parm->norm_shift + p_parm->pool_pool_shf;
	int32_t lastx, lasty;
	int32_t nin, cin, yin, kin;
	int32_t nout, cout, yout, kout;
	int32_t sum;
	int32_t pool_size, pool_size_r, pool_size_b, pool_size_br;
	int32_t n, c, x, y, kx, ky;

	lastx = (width  - 1) / ker_stridex * ker_stridex;
	lasty = (height - 1) / ker_stridey * ker_stridey;

	pool_size    = ker_w * ker_h;
	pool_size_r  = (width + rpad - lastx) * ker_h;
	pool_size_b  = ker_w * (height + bpad - lasty);
	pool_size_br = (width + rpad - lastx) * (height + rpad - lasty);

	nin  = in_addr;
	nout = out_addr;
	for (n = 0; n < batch; n++) {
		cin  = nin;
		cout = nout;
		for (c = 0; c < channels; c++) {
			yin  = cin;
			yout = cout;

			// first v stride / top pad points
			// {
				kout = yout;

				// first h stride / left pad points
				// {
					kin = yin;
					sum = 0;
					for (ky = 0; ky < ker_h - tpad; ky++) {
						for (kx = 0; kx < ker_w - lpad; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }

				// middle h stride
				for (x = -lpad + ker_stridex; x < lastx; x += ker_stridex) {
					kin = yin;
					sum = 0;
					for (ky = 0; ky < ker_h - tpad; ky++) {
						for (kx = x; kx < x + ker_w; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				}

				// last h stride / right pad points
				// {
					kin = yin;
					sum = 0;
					for (ky = 0; ky < ker_h - tpad; ky++) {
						for (kx = x; kx < width; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size_r, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }

				yin  +=  in_line_ofs * ker_stridey;
				yout += out_line_ofs;
			// }

			// middle v stride
			for (y = -tpad + ker_stridey; y < lasty; y += ker_stridey) {
				kout = yout;

				// first h stride / left pad points
				// {
					kin = yin;
					sum = 0;
					for (ky = y; ky < y + ker_h; ky++) {
						for (kx = 0; kx < ker_w - lpad; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }

				// middle h stride
				for (x = -lpad + ker_stridex; x < lastx; x += ker_stridex) {
					kin = yin;
					sum = 0;
					for (ky = y; ky < y + ker_h; ky++) {
						for (kx = x; kx < x + ker_w; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				}

				// last h stride / right pad points
				// {
					kin = yin;
					sum = 0;
					for (ky = y; ky < y + ker_h; ky++) {
						for (kx = x; kx < width; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size_r, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }

				yin  +=  in_line_ofs * ker_stridey;
				yout += out_line_ofs;
			}

			// last v stride / bottom pad points
			// {
				kout = yout;

				// first h stride / left pad points
				// {
					kin = yin;
					sum = 0;
					for (ky = y; ky < height; ky++) {
						for (kx = 0; kx < ker_w - lpad; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size_b, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }

				// middle h stride
				for (x = -lpad + ker_stridex; x < lastx; x += ker_stridex) {
					kin = yin;
					sum = 0;
					for (ky = y; ky < height; ky++) {
						for (kx = x; kx < x + ker_w; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size_b, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				}

				// last h stride / right pad points
				// {
					kin = yin;
					sum = 0;
					for (ky = y; ky < height; ky++) {
						for (kx = x; kx < width; kx++) {
							sum += *((T *)kin + kx);
						}
						kin += in_line_ofs;
					}
					POOL_AVG_SCA(sum, pool_size_br, sca, shf, kout, T, tmin, tmax);
					kout += sizeof(T);
				// }
			// }

			cin  +=  in_ch_ofs;
			cout += out_ch_ofs;
		}
		nin  +=  in_batch_ofs;
		nout += out_batch_ofs;
	}

	return HD_OK;
}

template<typename T, INT32 tmin, INT32 tmax, BOOL shr>
static HD_RESULT custnn_global_max_pool(CUSTNN_POOL_PARM *p_parm, UINT32 in_addr, UINT32 out_addr)
{
	int32_t width         = p_parm->size_width;
	int32_t height        = p_parm->size_height;
	int32_t channels      = p_parm->size_channel;
	int32_t batch         = p_parm->batch_num;
	int32_t in_line_ofs   = p_parm->in_ofs_line_ofs;
	int32_t in_ch_ofs     = p_parm->in_ofs_channel_ofs;
	int32_t in_batch_ofs  = p_parm->in_ofs_batch_ofs;
	int32_t out_ch_ofs    = p_parm->out_ofs_channel_ofs;
	int32_t out_batch_ofs = p_parm->out_ofs_batch_ofs;
	int32_t sca           = p_parm->norm_scale;
	int32_t shf           = p_parm->norm_shift + p_parm->pool_pool_shf;
	int32_t nin, cin, yin;
	int32_t nout, cout;
	int32_t max;
	int32_t n, c, x, y;

	nin  = in_addr;
	nout = out_addr;
	for (n = 0; n < batch; n++) {
		cin  = nin;
		cout = nout;
		for (c = 0; c < channels; c++) {
			yin = cin;
			max = tmin;
			for (y = 0; y < height; y++) {
				for (x = 0; x < width; x++) {
					if (max < *((T *)yin + x)) {
						max = *((T *)yin + x);
					}
				}
				yin += in_line_ofs;
			}
			POOL_SCA(max, sca, shf, cout, T, tmin, tmax);

			cin  +=  in_ch_ofs;
			cout += out_ch_ofs;
		}
		nin  +=  in_batch_ofs;
		nout += out_batch_ofs;
	}

	return HD_OK;
}

template<typename T, INT32 tmin, INT32 tmax, BOOL shr>
static HD_RESULT custnn_global_avg_pool(CUSTNN_POOL_PARM *p_parm, UINT32 in_addr, UINT32 out_addr)
{
	int32_t width         = p_parm->size_width;
	int32_t height        = p_parm->size_height;
	int32_t channels      = p_parm->size_channel;
	int32_t batch         = p_parm->batch_num;
	int32_t in_line_ofs   = p_parm->in_ofs_line_ofs;
	int32_t in_ch_ofs     = p_parm->in_ofs_channel_ofs;
	int32_t in_batch_ofs  = p_parm->in_ofs_batch_ofs;
	int32_t out_ch_ofs    = p_parm->out_ofs_channel_ofs;
	int32_t out_batch_ofs = p_parm->out_ofs_batch_ofs;
	int32_t sca           = p_parm->norm_scale;
	int32_t shf           = p_parm->norm_shift + p_parm->pool_pool_shf;
	int32_t nin, cin, yin;
	int32_t nout, cout;
	int32_t sum;
	int32_t pool_size;
	int32_t n, c, x, y;

	pool_size = width * height;

	nin  = in_addr;
	nout = out_addr;
	for (n = 0; n < batch; n++) {
		cin  = nin;
		cout = nout;
		for (c = 0; c < channels; c++) {
			yin = cin;
			sum = 0;
			for (y = 0; y < height; y++) {
				for (x = 0; x < width; x++) {
					sum += *((T *)yin + x);
				}
				yin += in_line_ofs;
			}
			POOL_AVG_SCA(sum, pool_size, sca, shf, cout, T, tmin, tmax);

			cin  +=  in_ch_ofs;
			cout += out_ch_ofs;
		}
		nin  +=  in_batch_ofs;
		nout += out_batch_ofs;
	}

	return HD_OK;
}

template<typename T, INT32 tmin, INT32 tmax, BOOL shr>
static HD_RESULT custnn_pool_mode(CUSTNN_POOL_PARM *p_parm, UINT32 in_addr, UINT32 out_addr)
{
	HD_RESULT ret = HD_OK;

	switch (p_parm->pool_mode) {
	case NN_AI_POOL_LOCAL_MAX:
		ret = custnn_local_max_pool <T, tmin, tmax, shr>(p_parm, in_addr, out_addr);
		break;
	case NN_AI_POOL_LOCAL_AVG:
		ret = custnn_local_avg_pool <T, tmin, tmax, shr>(p_parm, in_addr, out_addr);
		break;
	case NN_AI_POOL_GLOBAL_MAX:
		ret = custnn_global_max_pool<T, tmin, tmax, shr>(p_parm, in_addr, out_addr);
		break;
	case NN_AI_POOL_GLOBAL_AVG:
		ret = custnn_global_avg_pool<T, tmin, tmax, shr>(p_parm, in_addr, out_addr);
		break;
	default:
		DBG_ERR("unsupported pooling mode %ld\r\n", p_parm->pool_mode);
		ret = HD_ERR_NOT_SUPPORT;
		break;
	}

	return ret;
}

template<BOOL shr>
static HD_RESULT custnn_pool_type(CUSTNN_POOL_PARM *p_parm, UINT32 in_addr, UINT32 out_addr)
{
	HD_RESULT ret = HD_OK;

	switch (p_parm->in_type) {
	case NN_AI_IO_INT8:
		ret = custnn_pool_mode<  INT8, CHAR_MIN,  CHAR_MAX, shr>(p_parm, in_addr, out_addr);
		break;
	case NN_AI_IO_UINT8:
		ret = custnn_pool_mode< UINT8,        0, UCHAR_MAX, shr>(p_parm, in_addr, out_addr);
		break;
	case NN_AI_IO_INT16:
		ret = custnn_pool_mode< INT16, SHRT_MIN,  SHRT_MAX, shr>(p_parm, in_addr, out_addr);
		break;
	case NN_AI_IO_UINT16:
		ret = custnn_pool_mode<UINT16,        0, USHRT_MAX, shr>(p_parm, in_addr, out_addr);
		break;
	default:
		DBG_ERR("unsupported input type %ld\r\n", p_parm->in_type);
		ret = HD_ERR_NOT_SUPPORT;
		break;
	}

	return ret;
}

static VOID update_custnn_pool_parm(CUSTNN_POOL_PARM *p_pool_parm, NN_DATA* input_info, NN_DATA* output_info)
{
	INT32 in_bitdepth = input_info->fmt.sign_bits + input_info->fmt.int_bits + input_info->fmt.frac_bits;
	INT32 out_bitdepth = output_info->fmt.sign_bits + output_info->fmt.int_bits + output_info->fmt.frac_bits;
	if (in_bitdepth == 8 && p_pool_parm->in_type < 2 && out_bitdepth == 16){
		p_pool_parm->in_ofs_line_ofs *= 2;
		p_pool_parm->in_ofs_channel_ofs *= 2;
		p_pool_parm->in_ofs_batch_ofs *= 2;
		p_pool_parm->in_type += 2;
	}

	if (out_bitdepth == 16 && p_pool_parm->out_type < 2){
		p_pool_parm->out_ofs_line_ofs *= 2;
		p_pool_parm->out_ofs_channel_ofs *= 2;
		p_pool_parm->out_ofs_batch_ofs *= 2;
		p_pool_parm->out_type += 2;
	}
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT custnn_cpu_pool_process(UINTPTR bin_length, CHAR *p_prototxt_parm, CUSTNN_POOL_PARM *p_parm, UINTPTR temp_addr, UINTPTR out_addr,
 NN_DATA* input_info, NN_CUSTOM_DIM* input_dim, INT32 *p_isf_parm, UINT32 input_num,
 NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, INT32 *p_osf_parm, UINT32 output_num)
{
	// convert and move inbuf to tempbuf, set temp_ofs
	UINT32 temp_ofs[4] = {0};
	NN_AI_IO_TYPE in_fmt = custnn_get_type(input_info[0].fmt);
	NN_AI_IO_TYPE out_fmt = custnn_get_type(output_info[0].fmt);
	if ((in_fmt < 2 && out_fmt < 2) || (in_fmt >= 2 && out_fmt >= 2)) {

		for (UINT32 temp_idx = 0; temp_idx < 4; temp_idx++) {
			temp_ofs[temp_idx] = input_dim[0].ofs[temp_idx];
		}
	} else {

		for (UINT32 temp_idx = 0; temp_idx < 4; temp_idx++) {

			if (in_fmt < out_fmt) {
				temp_ofs[temp_idx] = input_dim[0].ofs[temp_idx] * 2;
			} else {
				temp_ofs[temp_idx] = input_dim[0].ofs[temp_idx] / 2;
			}
		}
	}

	NN_SF dummy_sf = {1, 0};
	UINTPTR input_addr_b, input_addr_c, input_addr_h;
	UINTPTR temp_addr_b, temp_addr_c, temp_addr_h;
	for (UINT32 b_idx = 0; b_idx < input_dim[0].dim[3]; b_idx++) {

		input_addr_b = input_info[0].va + b_idx * input_dim[0].ofs[2];
		temp_addr_b = temp_addr + b_idx * temp_ofs[2];

		for (UINT32 c_idx = 0; c_idx < input_dim[0].dim[2]; c_idx++) {

			input_addr_c = input_addr_b + c_idx * input_dim[0].ofs[1];
			temp_addr_c = temp_addr_b + c_idx * temp_ofs[1];

			for (UINT32 h_idx = 0; h_idx < input_dim[0].dim[1]; h_idx++) {

				input_addr_h = input_addr_c + h_idx * input_dim[0].ofs[0];
				temp_addr_h = temp_addr_c + h_idx * temp_ofs[0];

				nvtnn_convert_fmt_c((VOID *)input_addr_h, (VOID *)temp_addr_h, input_dim[0].dim[0], &input_info[0].fmt, &output_info[0].fmt, &dummy_sf, &dummy_sf);
			}
		}
	}

	if (bin_length == 0) { //prototxt mode, update CUSTNN_POOL_PARM

		CUSTNN_POOL_PROTO_PARM *p_proto_parm = (CUSTNN_POOL_PROTO_PARM *)p_prototxt_parm;

		p_parm->pool_mode = p_proto_parm->pool_type;
		p_parm->pool_local_ker_w = p_proto_parm->kerl_sz;
		p_parm->pool_local_ker_h = p_proto_parm->kerl_sz;
		p_parm->pool_local_ker_stridex = p_proto_parm->stride;
		p_parm->pool_local_ker_stridey = p_proto_parm->stride;
		p_parm->pool_local_pad_top_pad_num = p_proto_parm->pad;
		p_parm->pool_local_pad_bot_pad_num = p_proto_parm->pad;
		p_parm->pool_local_pad_left_pad_num = p_proto_parm->pad;
		p_parm->pool_local_pad_right_pad_num = p_proto_parm->pad;

		p_parm->in_type = custnn_get_type(input_info[0].fmt);
		p_parm->out_type = custnn_get_type(output_info[0].fmt);

		p_parm->size_width = input_dim[0].dim[0];
		p_parm->size_height = input_dim[0].dim[1];
		p_parm->size_channel = input_dim[0].dim[2];
		p_parm->batch_num = input_dim[0].dim[3];

		p_parm->in_ofs_line_ofs = input_dim[0].ofs[0];
		p_parm->in_ofs_channel_ofs = input_dim[0].ofs[1];
		p_parm->in_ofs_batch_ofs = input_dim[0].ofs[2];

		p_parm->out_ofs_line_ofs = output_dim[0].ofs[0];
		p_parm->out_ofs_channel_ofs = output_dim[0].ofs[1];
		p_parm->out_ofs_batch_ofs = output_dim[0].ofs[2];

		p_parm->pool_local_pad_pad_val = 0;
		p_parm->pool_global_avg_mul = 1;
		p_parm->pool_global_avg_shf= 0;
		p_parm->pool_pool_shf = 0;
		p_parm->norm_scale = 1;
		p_parm->norm_shift = 0;
		
	}
	
	update_custnn_pool_parm(p_parm, &input_info[0], &output_info[0]);

	HD_RESULT ret = HD_OK;

#if POOL_PRINT_PARM
	DBGD(p_parm->in_type);
	DBGD(p_parm->out_type);
	DBGD(p_parm->size_width);
	DBGD(p_parm->size_height);
	DBGD(p_parm->size_channel);
	DBGD(p_parm->batch_num);
	DBGD(p_parm->in_ofs_line_ofs);
	DBGD(p_parm->in_ofs_channel_ofs);
	DBGD(p_parm->in_ofs_batch_ofs);
	DBGD(p_parm->out_ofs_line_ofs);
	DBGD(p_parm->out_ofs_channel_ofs);
	DBGD(p_parm->out_ofs_batch_ofs);
	DBGD(p_parm->pool_mode);
	DBGD(p_parm->pool_local_ker_w);
	DBGD(p_parm->pool_local_ker_h);
	DBGD(p_parm->pool_local_ker_stridex);
	DBGD(p_parm->pool_local_ker_stridey);
	DBGD(p_parm->pool_local_pad_top_pad_num);
	DBGD(p_parm->pool_local_pad_bot_pad_num);
	DBGD(p_parm->pool_local_pad_left_pad_num);
	DBGD(p_parm->pool_local_pad_right_pad_num);
	DBGD(p_parm->pool_local_pad_pad_val);
	DBGD(p_parm->pool_global_avg_mul);
	DBGD(p_parm->pool_global_avg_shf);
	DBGD(p_parm->pool_pool_shf);
	DBGD(p_parm->norm_scale);
	DBGD(p_parm->norm_shift);
#endif

	PROF_START();

	if (p_parm->norm_shift + p_parm->pool_pool_shf > 0) { // shift right
		ret = custnn_pool_type<TRUE>(p_parm, temp_addr, out_addr);
	} else {                                              // shift left
		ret = custnn_pool_type<FALSE>(p_parm, temp_addr, out_addr);
	}

	PROF_END("net_pool_sample");

	return ret;
}
