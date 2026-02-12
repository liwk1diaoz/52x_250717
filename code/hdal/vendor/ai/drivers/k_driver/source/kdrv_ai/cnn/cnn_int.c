/*
CNN module driver

NT98520 CNN driver.

@file       cnn_int.c
@ingroup    mIIPPCNN

Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "cnn_platform.h"
#include "cnn_lib.h"
#include "cnn_reg.h"
#include "cnn_int.h"

//=============================================================
#define __CLASS__ 				"[ai][kdrv][cnn]"
#include "kdrv_ai_debug.h"
//=============================================================

/**
@addtogroup mIIPPCNN
*/
//@{

/**
CNN Get Register Base Address

CNN get register base address.

@return UINT32 register base address.
*/
#define CNN_FCD_VLC_REG_CNT	12
#define CNN_FCD_VLC_TBL_SIZE	23
#define CNN_CLAMP_REG_CNT	6


static UINT32 cnn_get_regbase_addr(BOOL cnn_id)
{
	if (cnn_id == 0) {
		return CNN_IOADDR_REG_BASE1;
	} else {
		return CNN_IOADDR_REG_BASE2;
	}
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
void cnn_cal_sz(BOOL cnn_id, UINT32 *CNN_dram_in_size0, UINT32 *CNN_dram_in_size4, UINT32 *CNN_dram_in_size5, UINT32 *CNN_dram_in_size6, UINT32 *CNN_dram_in_size7, UINT32 *CNN_dram_out_size0, UINT32 *CNN_dram_out_size1)
{
	int rate = 0;
	int out_w = 0, out_h = 0, pad_w = 0, pad_h = 0;
	int deconv_w = 0, deconv_h = 0;
	int ker_w = 1, ker_h = 1;
	int half_ker_w = 0, half_ker_h = 0;
	int conv_out_w = 0, conv_out_h = 0;
	UINT32 output_line_sizeBytes = 0;

	CNN_IN_SIZE insize;
	CNN_DMAIO_LOFS iolofs;
	UINT32 ker_en, io_en;
	CNN_IO_TYPE in_type = cnn_get_intype(cnn_id);
	CNN_IO_TYPE elt_type = cnn_get_elttype(cnn_id);
	CNN_IO_TYPE out0_type = cnn_get_out0type(cnn_id);
	CNN_IO_TYPE out1_type = cnn_get_out1type(cnn_id);
	CNN_CONVKERL_PARM conv_param = cnn_get_convparm(cnn_id);
	CNN_FCD_PARM fcd_parm = cnn_get_fcd_parm(cnn_id);
	CNN_OUT_SIZE out0ofs = cnn_get_out0ofs(cnn_id);
	CNN_DECONV_PARM deconvparm = cnn_get_deconvparm(cnn_id);
	CNN_SCALEUP_PARM scaleupparm = cnn_get_scaleupparm(cnn_id);
	CNN_LOCAL_POOL_PARM local_pool_parm = cnn_get_localpool_parm(cnn_id);


	*CNN_dram_in_size0 = *CNN_dram_in_size4 = *CNN_dram_in_size5 = *CNN_dram_in_size6 = *CNN_dram_in_size7 = *CNN_dram_out_size0= *CNN_dram_out_size1 = 0;

	insize = cnn_get_insize(cnn_id);
	iolofs = cnn_get_dmain_lofs(cnn_id);
	ker_en = cnn_get_kerl_en(cnn_id);
	io_en  = cnn_get_io_enable(cnn_id);

	if (cnn_get_engmode(cnn_id) == CNN_CONV) {

		int stride = 1;

		//input
		//SAI0

		if (io_en & CNN_IN_ISVSTRIPE_EN) { 
			// use ch offset(width*height) to calculate total size
			*CNN_dram_in_size0 = iolofs.in1_lofs*(insize.channel-1) + insize.width*insize.height;
		}
		else if (io_en & CNN_IN_ISHSTRIPE_EN) { 
			// use ch offset(width*height) to calculate total size
			*CNN_dram_in_size0 = iolofs.in0_lofs*(insize.height*insize.channel-1) + insize.width;
		}
		else if (insize.batch > 1 && (io_en & CNN_IN_INTERLACE_EN) == 0) {
			// use lineoffset(batch) to calculate total size
			*CNN_dram_in_size0 = iolofs.in0_lofs*insize.batch;
		}
		else {
			if (insize.batch > 1) {
				*CNN_dram_in_size0 = insize.width*insize.height*insize.channel*insize.batch;
			} else {
				*CNN_dram_in_size0 = insize.width*insize.height*insize.channel;
			}

			if (in_type == CNN_INT16 || in_type == CNN_UINT16) {
				*CNN_dram_in_size0 *= sizeof(INT16);
			} else {
				*CNN_dram_in_size0 *= sizeof(INT8);
			}
		}


		if (conv_param.conv_stride == CNN_CONV_KER_STRIDE_1) {
			stride = 1;
		} else if (conv_param.conv_stride == CNN_CONV_KER_STRIDE_2) {
			stride = 2;
		} else if (conv_param.conv_stride == CNN_CONV_KER_STRIDE_4) {
			stride = 4;
		}

		//SAI4
		if (ker_en & CNN_CONV_KERL_EN) {
			*CNN_dram_in_size4 = fcd_parm.fcd_enc_bit_length / 8;
			if (fcd_parm.fcd_enc_bit_length % 8 > 0) {
				*CNN_dram_in_size4 = *CNN_dram_in_size4 + 1;
			}
		}
		//SAI5
		if (ker_en & CNN_FCD_VLC_EN) {
			if (ker_en & CNN_FCD_QUANTI_EN) {
				*CNN_dram_in_size5 = 4096*2;
			} else {
				*CNN_dram_in_size5 = (256*2)*2;
			}
		}

		//SAI6
		if (ker_en & CNN_CONV_KERL_EN) {
			if (ker_en & CNN_BNSCALE_KERL_EN) {
				*CNN_dram_in_size6 = conv_param.conv_setnum*8;
			} else {
				*CNN_dram_in_size6 = conv_param.conv_setnum*2;
			}
		} else {
			if (ker_en & CNN_BNSCALE_KERL_EN) {
				*CNN_dram_in_size6 = insize.channel*8;
			} else {
				*CNN_dram_in_size6 = insize.channel*2;
			}

			if ((io_en & CNN_IN_ISHSTRIPE_EN) == 0 && (io_en & CNN_IN_ISVSTRIPE_EN) == 0) {
				*CNN_dram_in_size6 = *CNN_dram_in_size6 * insize.batch;
			}
		}

		//output
		if (conv_param.conv_kersize == 0) {
			ker_w = 11;
			ker_h = 11;
		} else if (conv_param.conv_kersize == 1) {
			ker_w = 7;
			ker_h = 7;
		} else if (conv_param.conv_kersize == 2) {
			ker_w = 5;
			ker_h = 5;
		} else if (conv_param.conv_kersize == 3) {
			ker_w = 3;
			ker_h = 3;
		} else if (conv_param.conv_kersize == 4) {
			ker_w = 1;
			ker_h = 1;
		} else if (conv_param.conv_kersize == 5) {
			ker_w = 7;
			ker_h = 1;
		} else if (conv_param.conv_kersize == 6) {
			ker_w = 1;
			ker_h = 7;
		} else if (conv_param.conv_kersize == 7) {
			ker_w = 5;
			ker_h = 1;
		} else if (conv_param.conv_kersize == 8) {
			ker_w = 1;
			ker_h = 5;
		} else if (conv_param.conv_kersize == 9) {
			ker_w = 3;
			ker_h = 1;
		} else if (conv_param.conv_kersize == 10) {
			ker_w = 1;
			ker_h = 3;
		} else if (conv_param.conv_kersize == 11) {
			ker_w = 9;
			ker_h = 9;
		}

		half_ker_w = (ker_w > 1) ? (ker_w >> 1) : 1;
		half_ker_h = (ker_h > 1) ? (ker_h >> 1) : 1;

		if (ker_w == 1) {
			conv_param.is_left_pad  = 0;
			conv_param.is_right_pad = 0;
		}

		if (ker_h == 1) {
			conv_param.is_top_pad = 0;
			conv_param.is_bot_pad = 0;
		}

		if (ker_en & CNN_CONV_KERL_EN) {
			conv_out_w = ((insize.width + (conv_param.is_left_pad+conv_param.is_right_pad)*half_ker_w - ker_w)/stride)+1;
			conv_out_h = ((insize.height + (conv_param.is_top_pad+conv_param.is_bot_pad)*half_ker_h - ker_h)/stride)+1;
		} else {
			conv_out_w = insize.width;
			conv_out_h = insize.height;
		}

		//SAI7
		if (io_en & CNN_IN_ISCHANNELSTRIPE_EN) {
			if (insize.batch > 1) {
				*CNN_dram_in_size7 = conv_out_w*conv_out_h*conv_param.conv_setnum*insize.batch;
			} else {
				*CNN_dram_in_size7 = conv_out_w*conv_out_h*conv_param.conv_setnum;
			}
		} else if (ker_en & CNN_ELTWISE_KERL_EN) {
			if (ker_en & CNN_CONV_KERL_EN) {
				if(insize.batch > 1)
					*CNN_dram_in_size7 = conv_out_w*conv_out_h*conv_param.conv_setnum*insize.batch;
				else
					*CNN_dram_in_size7 = conv_out_w*conv_out_h*conv_param.conv_setnum;
			} else {
				if(insize.batch > 1) {
					*CNN_dram_in_size7 = conv_out_w*conv_out_h*insize.channel*insize.batch;
				} else {
					*CNN_dram_in_size7 = conv_out_w*conv_out_h*insize.channel;
				}
			}


			if ((elt_type == CNN_INT16) || (elt_type == CNN_UINT16)) {
				*CNN_dram_in_size7 *= sizeof(INT16);
			} else {
				*CNN_dram_in_size7 *= sizeof(INT8);
			}
		}

		if ((cnn_get_out0mode(cnn_id) == CNN_OUT0_RELU0) && (io_en & CNN_OUT0_EN)) {
			conv_out_h = conv_out_h - out0ofs.out0_ofs;
		}

		if ((out0_type == CNN_INT16) || (out0_type == CNN_UINT16)) {
			output_line_sizeBytes = conv_out_w*conv_out_h*2;
		} else {
			output_line_sizeBytes = conv_out_w*conv_out_h*1;
		}

		if (io_en & CNN_OUT0_EN) {
			if (cnn_get_out0mode(cnn_id) == CNN_OUT0_RELU0) { // relu

				//If (IN_IS_HSTRIPE =1 || IN_IS_VSTRIPE=1) and out0 is 8 bit (OUT0_BITDEPTH=0), byte aligned.
				//If (IN_IS_HSTRIPE =1 || IN_IS_VSTRIPE=1) and out0 is 16bit (OUT0_BITDEPTH=1), 2-byte aligned.
				//Otherwise, word aligned.

				if ((io_en & CNN_IN_ISHSTRIPE_EN) || (io_en & CNN_IN_ISVSTRIPE_EN)) {
					// refer to OFSO1 (ch offset)
					if (ker_en & CNN_CONV_KERL_EN) {
						*CNN_dram_out_size0 = iolofs.out1_lofs*conv_param.conv_setnum;
					} else {
						*CNN_dram_out_size0 = iolofs.out1_lofs*insize.channel;
					}
				} else {
					if (ker_en & CNN_CONV_KERL_EN) {
						*CNN_dram_out_size0 = output_line_sizeBytes*conv_param.conv_setnum*insize.batch;
					} else {
						if (insize.batch > 1) {
							// refer to OFSO0 (batch offset)
							*CNN_dram_out_size0 =iolofs.out0_lofs*insize.batch;
						} else {
							*CNN_dram_out_size0 = output_line_sizeBytes*insize.channel*insize.batch;
						}
					}
				}
			} else if (cnn_get_engmode(cnn_id) == CNN_CONV && cnn_get_out0mode(cnn_id) == CNN_OUT0_INTERMEDIATE) { // intermediate output
				*CNN_dram_out_size0 = conv_out_w*conv_out_h*2*conv_param.conv_setnum*insize.batch;
			}
		}

		if (cnn_get_io_enable(cnn_id) & CNN_OUT1_EN) {
			if (cnn_get_engmode(cnn_id) == CNN_CONV && (cnn_get_io_enable(cnn_id) & CNN_OUT0_EN)) {
				conv_out_h = conv_out_h + out0ofs.out0_ofs;
			}
			output_line_sizeBytes = conv_out_w*conv_out_h*(((out1_type == CNN_INT16)||(out1_type == CNN_UINT16))?2:1);
			if ((io_en & CNN_IN_ISHSTRIPE_EN) || (io_en & CNN_IN_ISVSTRIPE_EN)) {
				// refer to OFSO3 (ch offset)
				if (ker_en & CNN_CONV_KERL_EN) {
					*CNN_dram_out_size1 = iolofs.out3_lofs*conv_param.conv_setnum;
				} else {
					*CNN_dram_out_size1 = iolofs.out3_lofs*insize.channel;
				}
			} else if (insize.batch > 1 && (ker_en & CNN_CONV_KERL_EN) == 0) {
				*CNN_dram_out_size1 = iolofs.out2_lofs*insize.batch;
			} else {
				if (ker_en & CNN_POOLING_KERL_EN) {
					if ((ker_en & CNN_CONV_KERL_EN) == 0) {
						conv_out_w = insize.width;
						conv_out_h = insize.height;
					}
					//local pooling constraint

					//input min
					if (local_pool_parm.ker_size == CNN_POOL_KERSZ_2) {
						//2x2
						ker_w = 2;
						ker_h = 2;
					} else if(local_pool_parm.ker_size == CNN_POOL_KERSZ_3)  {
						//3x3
						ker_w = 3;
						ker_h = 3;
					} else if (local_pool_parm.ker_size == CNN_POOL_KERSZ_4) {
						//4x4
						ker_w = 4;
						ker_h = 4;
					} else if (local_pool_parm.ker_size == CNN_POOL_KERSZ_5) {
						//5x5
						ker_w = 5;
						ker_h = 5;
					} else {
						ker_w = 0;
						ker_h = 0;
					}

					half_ker_w = ker_w >> 1;
					half_ker_h = ker_h >> 1;
					stride = 1 << local_pool_parm.stride;
					pad_w = local_pool_parm.is_left_pad + local_pool_parm.is_right_pad;
					pad_h = local_pool_parm.is_top_pad + local_pool_parm.is_bot_pad;



					if (local_pool_parm.ave_div_type) {     //floor
						out_w    = (conv_out_w + pad_w*half_ker_w - ker_w) / stride + 1;
						out_h    = (conv_out_h + pad_h*half_ker_h - ker_h) / stride + 1;
					} else {    //ceil
						out_w    = (conv_out_w + pad_w*half_ker_w - ker_w + (stride-1)) / stride + 1;
						out_h    = (conv_out_h + pad_h*half_ker_h - ker_h + (stride-1)) / stride + 1;
					}

					if (cnn_get_poolmode(cnn_id) == CNN_POOLING_GLOBAL) {
						//global pooling
						out_w = 1;
						out_h = 1;
					}

					output_line_sizeBytes = out_w*out_h*(((out1_type == CNN_INT16)||(out1_type == CNN_UINT16))?2:1);
				}

				if (ker_en & CNN_CONV_KERL_EN) {
					*CNN_dram_out_size1 = output_line_sizeBytes*conv_param.conv_setnum*insize.batch;
				} else {
					*CNN_dram_out_size1 = output_line_sizeBytes*insize.channel*insize.batch;
				}
			}

			//debug_msg("dram out_1_size:%d\n", CNN_dram_out_size1);
		} else {
			*CNN_dram_out_size1 = 0;
		}
	} else if (cnn_get_engmode(cnn_id) == CNN_DECONV) {

		int stride =0;
		//input
		if ((io_en & CNN_IN_ISHSTRIPE_EN) || (io_en & CNN_IN_ISVSTRIPE_EN)){
			// use ch offset(width*height) to calculate total size
			*CNN_dram_in_size0 = iolofs.in1_lofs*insize.channel;
		} else if (insize.batch > 1 && (io_en & CNN_IN_INTERLACE_EN) == 0) {
			// use lineoffset(batch) to calculate total size
			*CNN_dram_in_size0 = iolofs.in0_lofs*insize.batch;
		} else {
			if (insize.batch > 1) {
				*CNN_dram_in_size0 = insize.width*insize.height*insize.channel*insize.batch;
			} else {
				*CNN_dram_in_size0 = insize.width*insize.height*insize.channel;
			}

			if ((in_type == CNN_INT16) || (in_type == CNN_UINT16)) {
				*CNN_dram_in_size0 *= sizeof(INT16);
			} else {
				*CNN_dram_in_size0 *= sizeof(INT8);
			}
		}

		if (deconvparm.deconv_stride == 0) {
			stride = 1;
		} else if (deconvparm.deconv_stride == 1) {
			stride = 2;
		} else if (deconvparm.deconv_stride == 2) {
			stride = 4;
		} else if (deconvparm.deconv_stride == 3) {
			stride = 8;
		}

		//output
		out_w   = ((insize.width-1)*stride) + 1;
		out_h   = ((insize.height-1)*stride) + 1;
		deconv_w  = out_w + deconvparm.is_left_pad + deconvparm.is_right_pad;
		deconv_h  = out_h + deconvparm.is_top_pad + deconvparm.is_bot_pad;

		//======0110 add: MST & multi-batch case===============
		if (insize.batch > 1) {
			// refer to OFSO0 (batch offset)
			*CNN_dram_out_size0 = iolofs.out0_lofs*insize.batch;
		} else if ((io_en & CNN_IN_ISHSTRIPE_EN) || (io_en & CNN_IN_ISVSTRIPE_EN)) {
			// refer to OFSO1 (ch offset)
			*CNN_dram_out_size0 = iolofs.out1_lofs*insize.channel;
		} else {
			*CNN_dram_out_size0 = deconv_w*deconv_h*insize.channel;
			if ((out0_type == CNN_INT16) || (out0_type == CNN_UINT16))
				*CNN_dram_out_size0 *= sizeof(INT16);
			else
				*CNN_dram_out_size0 *= sizeof(INT8);
		}
	} else if (cnn_get_engmode(cnn_id) == CNN_SCALEUP) {
		//scale-up
		//input
		*CNN_dram_in_size0 = insize.width*insize.height*insize.channel;
		if ((in_type == CNN_INT16) || (in_type == CNN_UINT16)) {
			*CNN_dram_in_size0 *= sizeof(INT16);
		} else {
			*CNN_dram_in_size0 *= sizeof(INT8);
		}

		//output
		if(scaleupparm.scaleup_rate == 0) {
			rate = 1;
		} else if(scaleupparm.scaleup_rate == 1) {
			rate = 2;
		} else if(scaleupparm.scaleup_rate == 2) {
			rate = 4;
		} else if(scaleupparm.scaleup_rate == 3) {
			rate = 8;
		}

		out_w = insize.width*rate + scaleupparm.is_left_pad + scaleupparm.is_right_pad;
		out_h = insize.height*rate + scaleupparm.is_top_pad + scaleupparm.is_bot_pad;

		//======0110 add: MST & multi-batch case===============
		if (insize.batch > 1) {
			// refer to OFSO0 (batch offset)
			*CNN_dram_out_size0 = iolofs.out0_lofs*insize.batch;
		} else if ((io_en & CNN_IN_ISHSTRIPE_EN) || (io_en & CNN_IN_ISVSTRIPE_EN)) {
			// refer to OFSO1 (ch offset)
			*CNN_dram_out_size0 = iolofs.out1_lofs*insize.channel;
		} else {
			*CNN_dram_out_size0 = out_w * out_h * insize.channel;
			if ((out0_type == CNN_INT16) || (out0_type == CNN_UINT16)) {
				*CNN_dram_out_size0 *= sizeof(INT16);
			} else {
				*CNN_dram_out_size0 *= sizeof(INT8);
			}
		}
	} else if (cnn_get_engmode(cnn_id) == CNN_LRN) {
		//lrn
		//input
		*CNN_dram_in_size0 = insize.width*insize.height*insize.channel;
		if ((in_type == CNN_INT16) || (in_type == CNN_UINT16)) {
			*CNN_dram_in_size0 *= sizeof(INT16);
		} else {
			*CNN_dram_in_size0 *= sizeof(INT8);
		}

		//output

		out_w = insize.width;
		out_h = insize.height;

		//======0110 add: MST & multi-batch case===============
		if (insize.batch > 1) {
			// refer to OFSO0 (batch offset)
			*CNN_dram_out_size0 = iolofs.out0_lofs*insize.batch;
		} else if ((io_en & CNN_IN_ISHSTRIPE_EN) || (io_en & CNN_IN_ISVSTRIPE_EN)) {
			// refer to OFSO1 (ch offset)
			*CNN_dram_out_size0 = iolofs.out1_lofs*insize.channel;
		} else {
			*CNN_dram_out_size0 = out_w * out_h * insize.channel;
			if ((out0_type == CNN_INT16) || (out0_type == CNN_UINT16)) {
				*CNN_dram_out_size0 *= sizeof(INT16);
			} else {
				*CNN_dram_out_size0 *= sizeof(INT8);
			}
		}
	}

}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
CNN Get Cycle

CNN get current cycle.

@return UINT32 cycle.
*/
UINT32 cnn_getcycle(BOOL cnn_id)
{
	if(nvt_get_chip_id() == CHIP_NA51055) {
		return CNN_GETDATA(0xd0, cnn_get_regbase_addr(cnn_id));
	} else {
		return CNN_GETDATA(0xc0, cnn_get_regbase_addr(cnn_id));
	}
}
/**
CNN Get LL Cycle

CNN get current cycle.

@return UINT32 cycle.
*/
UINT32 cnn_get_ll_cycle(BOOL cnn_id)
{
	if(nvt_get_chip_id() == CHIP_NA51055) {
		return 0;
	} else {
		UINT32 ofs = 0xc4;
		return CNN_GETDATA(ofs, cnn_get_regbase_addr(cnn_id));
	}
}
/**
CNN Get DMA Wait Cycle

CNN get current cycle.

@return UINT32 cycle.
*/
UINT32 cnn_get_dma_cycle(BOOL cnn_id)
{
	return 0;
}

/**
Enable/Disable CNN Interrupt

CNN interrupt enable selection.

@param[in] enable Decide selected funtions are to be enabled or disabled.
\n-@b ENABLE: enable selected functions.
\n-@b DISABLE: disable selected functions.
@param[in] intr Indicate the function(s).

@return None.
*/
VOID cnn_enable_int(BOOL cnn_id, BOOL enable, UINT32 intr)
{
	T_CNN_INTERRUPT_ENABLE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_INTERRUPT_ENABLE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	if (enable) {
		reg1.reg |= intr;
	} else {
		reg1.reg &= (~intr);
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Interrupt Enable Status

Get current CNN interrupt Enable status.

@return CNN interrupt Enable status.
*/
UINT32 cnn_get_int_enable(BOOL cnn_id)
{
	T_CNN_INTERRUPT_ENABLE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_INTERRUPT_ENABLE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return reg1.reg;
}

/**
CNN Clear Interrupt Status

Clear CNN interrupt status.

@param[in] uiStatus 1's in this word will clear corresponding interrupt status.

@return None.
*/
VOID cnn_clr_intr_status(BOOL cnn_id, UINT32 status)
{
	T_CNN_INTERRUPT_STATUS_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_INTERRUPT_STATUS_REGISTER_OFS;

	reg1.reg = status;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Interrupt Status

Get current CNN interrupt status.

@return CNN interrupt status readout.
*/
UINT32 cnn_get_intr_status(BOOL cnn_id)
{
	T_CNN_INTERRUPT_STATUS_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_INTERRUPT_STATUS_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return reg1.reg;
}

/**
CNN Reset

Enable/disable CNN SW reset.

@param[in] reset.
\n-@b TRUE: enable reset.
\n-@b FALSE: disable reset.

@return None.
*/
VOID cnn_clr(BOOL cnn_id, BOOL reset)
{
	T_CNN_CONTROL_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_CONTROL_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.CNN_SW_RST = reset;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Enable

Trigger CNN HW start.

@param[in] start
\n-@b TRUE: enable.
\n-@b FALSE: disable.

@return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID cnn_enable(BOOL cnn_id, BOOL start)
{
	T_CNN_CONTROL_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_CONTROL_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.CNN_START = start;
	if (start == 1) {
		reg1.bit.CNN_SRAMCTRL = 0;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
CNN Link-list Enable

Trigger CNN linked list HW start.

@param[in] start
\n-@b TRUE: enable.
\n-@b FALSE: disable.

@return None.
*/
VOID cnn_ll_enable(BOOL cnn_id, BOOL start)
{
	T_CNN_CONTROL_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_CONTROL_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.LL_FIRE = start;
	if (start == 1) {
		reg1.bit.CNN_SRAMCTRL = 0;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Link-list Terminate

Trigger CNN linked list HW terminate.

@param[in] isterminate
\n-@b TRUE: enable.
\n-@b FALSE: disable.

@return None.
*/
VOID cnn_ll_terminate(BOOL cnn_id, BOOL isterminate)
{
	T_LL_TERMINATE_RESISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = LL_TERMINATE_RESISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.LL_TERMINATE = isterminate;
	if (isterminate) {
		reg1.bit.LL_TERMINATE = 0;
		reg1.bit.LL_TERMINATE = isterminate;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Set Engine Mode

Set CNN engine mode.

@param[in] mode input cnn engine mode.

@return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_set_engmode(BOOL cnn_id, CNN_MODE_TYPE mode)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (mode) {
	case CNN_CONV:
		reg1.bit.CNN_MODE = 0;
		break;
	case CNN_DECONV:
		reg1.bit.CNN_MODE = 1;
		break;
	case CNN_SCALEUP:
		reg1.bit.CNN_MODE = 2;
		break;
	case CNN_LRN:
		reg1.bit.CNN_MODE = 3;
		break;
	default:
		DBG_ERR("Unknown engine mode: %d!\r\n", mode);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Engine mode

Get current CNN engine mode.

@return CNN engine mode.
*/
CNN_MODE_TYPE cnn_get_engmode(BOOL cnn_id)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return (CNN_MODE_TYPE)reg1.bit.CNN_MODE;
}

/**
CNN Set Eltwise Mode

Set CNN eltwise mode.

@param[in] mode input cnn eltwise mode.

@return None.
*/
ER cnn_set_eltmode(BOOL cnn_id, CNN_ELT_MODE_TYPE mode)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (mode) {
	case CNN_ELT_ADD:
		reg1.bit.CNN_ELT_MODE = 0;
		break;
	case CNN_ELT_MULTIPLY:
		reg1.bit.CNN_ELT_MODE = 1;
		break;
	case CNN_ELT_MAX:
		reg1.bit.CNN_ELT_MODE = 2;
		break;
	default:
		DBG_ERR("Unknown eltwise mode: %d!\r\n", mode);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Eltwise mode

Get current CNN eltwise mode.

@return CNN eltwise mode.
*/
CNN_ELT_MODE_TYPE cnn_get_eltmode(BOOL cnn_id)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return (CNN_ELT_MODE_TYPE)reg1.bit.CNN_ELT_MODE;
}

/**
CNN Set Act Mode

Set CNN act mode.

@param[in] mode input cnn act mode.

@return None.
*/
ER cnn_set_actmode(BOOL cnn_id, CNN_ACT_MODE_TYPE mode)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (mode) {
	case CNN_ACT_RELU:
		reg1.bit.CNN_ACT_MODE = 0;
		break;
	case CNN_ACT_TANH:
		reg1.bit.CNN_ACT_MODE = 1;
		break;
	case CNN_ACT_LUT:
		reg1.bit.CNN_ACT_MODE = 1;
		break;
	default:
		DBG_ERR("Unknown act mode: %d!\r\n", mode);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Act mode

Get current CNN act mode.

@return CNN act mode.
*/
CNN_ACT_MODE_TYPE cnn_get_actmode(BOOL cnn_id)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return (CNN_ACT_MODE_TYPE)reg1.bit.CNN_ACT_MODE;
}

/**
CNN Set Pool Mode

Set CNN pool mode.

@param[in] mode input cnn pool mode.

@return None.
*/
ER cnn_set_poolmode(BOOL cnn_id, CNN_POOLING_TYPE mode)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (mode) {
	case CNN_POOLING_LOCAL:
		reg1.bit.CNN_POOL_MODE = 0;
		break;
	case CNN_POOLING_GLOBAL:
		reg1.bit.CNN_POOL_MODE = 1;
		break;
	default:
		DBG_ERR("Unknown pool mode: %d!\r\n", mode);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Pool mode

Get current CNN pool mode.

@return CNN pool mode.
*/
CNN_POOLING_TYPE cnn_get_poolmode(BOOL cnn_id)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return (CNN_POOLING_TYPE)reg1.bit.CNN_POOL_MODE;
}

/**
CNN Set Lut Mode

Set CNN lut mode.

@param[in] mode input cnn lut mode.

@return None.
*/
ER cnn_set_lutmode(BOOL cnn_id, CNN_LUT_MODE_TYPE mode)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (mode) {
	case CNN_LUT_TANH:
		reg1.bit.CNN_LUT_MODE = 0;
		break;
	case CNN_LUT_SIGMOID:
		reg1.bit.CNN_LUT_MODE = 1;
		break;
	default:
		DBG_ERR( "Unknown lut mode: %d!\r\n", mode);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Lut mode

Get current CNN lut mode.

@return CNN lut mode.
*/
CNN_LUT_MODE_TYPE cnn_get_lutmode(BOOL cnn_id)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return (CNN_POOLING_TYPE)reg1.bit.CNN_LUT_MODE;
}

/**
CNN Set Input type

Set CNN input type.

@param[in] in_type input type.

@return None.
*/
ER cnn_set_intype(BOOL cnn_id, CNN_IO_TYPE in_type)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN)) {
		if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) && ((in_type == CNN_UINT16) || (in_type == CNN_INT16))) {
			DBG_ERR( "CNN input type must be 8 bits when CNN image mode!\r\n");
			if (in_type == CNN_UINT16) {
				DBG_ERR("CNN input type set CNN_UINT8!\r\n");
			}
			else {
				DBG_ERR("CNN input type set CNN_INT8!\r\n");
			}
			return E_PAR;
		}
	}

	if ((in_type == CNN_UINT8) || (in_type == CNN_INT8)) {
		reg1.bit.IN_BITDEPTH = 0;
		if (in_type == CNN_UINT8) {
			reg1.bit.IN_SIGNEDNESS = 0;
		}
		else {
			reg1.bit.IN_SIGNEDNESS = 1;
		}
	} else {
		reg1.bit.IN_BITDEPTH = 1;
		if (in_type == CNN_UINT16) {
			reg1.bit.IN_SIGNEDNESS = 0;
		}
		else {
			reg1.bit.IN_SIGNEDNESS = 1;
		}
	}

	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Set Eltwise Input type

Set CNN eltwise input type.

@param[in] elttype input type.

@return None.
*/
VOID cnn_set_eltwise_intype(BOOL cnn_id, CNN_IO_TYPE elttype)
{
	T_ELTWISE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = ELTWISE_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	if ((elttype == CNN_UINT8) || (elttype == CNN_INT8)) {
		reg1.bit.ELTWISE_INPUT_BIT_DEPTH = 0;
		if (elttype == CNN_UINT8) {
			reg1.bit.ELTWISE_INPUT_SIGN = 0;
		}
		else {
			reg1.bit.ELTWISE_INPUT_SIGN = 1;
		}
	} else {
		reg1.bit.ELTWISE_INPUT_BIT_DEPTH = 1;
		if (elttype == CNN_UINT16) {
			reg1.bit.ELTWISE_INPUT_SIGN = 0;
		}
		else {
			reg1.bit.ELTWISE_INPUT_SIGN = 1;
		}
	}

	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Set Output 0 type

Set CNN output 0 type.

@param[in] out_type output 0 type.

@return None.
*/
ER cnn_set_out0type(BOOL cnn_id, CNN_IO_TYPE out_type)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs1 = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs1, base_addr);
	if ((cnn_get_engmode(cnn_id) == CNN_DECONV) || (cnn_get_engmode(cnn_id) == CNN_SCALEUP)) {
		CNN_IO_TYPE intype     = cnn_get_intype(cnn_id);
		if (((intype == CNN_UINT8)||(intype == CNN_UINT16)) && ((out_type == CNN_INT8)||(out_type == CNN_INT16))) {
			if (out_type == CNN_INT8) {
				DBG_ERR("CNN for deconv or scale-up, out0_type must the same signedness as the input ,set CNN_UINT8!\r\n");
			}
			else {
				DBG_ERR("CNN for deconv or scale-up, out0_type must the same signedness as the input ,set CNN_UINT16!\r\n");
			}
			return E_PAR;
		}
		else if (((intype == CNN_INT8)||(intype == CNN_INT16)) && ((out_type == CNN_UINT8)||(out_type == CNN_UINT16))) {
			if (out_type == CNN_UINT8) {
				DBG_ERR("CNN for deconv or scale-up, out0_type must the same signedness as the input ,set CNN_INT8!\r\n");
			}
			else {
				DBG_ERR("CNN for deconv or scale-up, out0_type must the same signedness as the input ,set CNN_INT16!\r\n");
			}
			return E_PAR;
		}
	}

	if ((out_type == CNN_UINT8) || (out_type == CNN_INT8)) {
		reg1.bit.OUT0_BITDEPTH = 0;
		if (out_type == CNN_UINT8) {
			reg1.bit.OUT0_SIGNEDNESS = 0;
		}
		else {
			reg1.bit.OUT0_SIGNEDNESS = 1;
		}
	} else {
		reg1.bit.OUT0_BITDEPTH = 1;
		if (out_type == CNN_UINT16) {
			reg1.bit.OUT0_SIGNEDNESS = 0;
		}
		else {
			reg1.bit.OUT0_SIGNEDNESS = 1;
		}
	}

	CNN_SETDATA(ofs1, reg1.reg, base_addr);
	return E_OK;

}

/**
CNN Set Output 1 type

Set CNN output 1 type.

@param[in] out_type input type.

@return None.
*/
ER cnn_set_out1type(BOOL cnn_id, CNN_IO_TYPE out_type)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs1 = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs1, base_addr);

	if ((out_type == CNN_UINT8) || (out_type == CNN_INT8)) {
		reg1.bit.OUT1_BITDEPTH = 0;
		if (out_type == CNN_UINT8) {
			reg1.bit.OUT1_SIGNEDNESS = 0;
		}
		else {
			reg1.bit.OUT1_SIGNEDNESS = 1;
		}
	} else if ((out_type == CNN_UINT16) || (out_type == CNN_INT16)) {
		reg1.bit.OUT1_BITDEPTH = 1;
		if (out_type == CNN_UINT16) {
			reg1.bit.OUT1_SIGNEDNESS = 0;
		}
		else {
			reg1.bit.OUT1_SIGNEDNESS = 1;
		}
	}

	CNN_SETDATA(ofs1, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Input type

Get current CNN input type.

@return CNN input type.
*/
CNN_IO_TYPE cnn_get_intype(BOOL cnn_id)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	if ((reg1.bit.IN_SIGNEDNESS == 1) && (reg1.bit.IN_BITDEPTH == 1)) {
		return CNN_INT16;
	} else if ((reg1.bit.IN_SIGNEDNESS == 1) && (reg1.bit.IN_BITDEPTH == 0)) {
		return CNN_INT8;
	} else if ((reg1.bit.IN_SIGNEDNESS == 0) && (reg1.bit.IN_BITDEPTH == 1)) {
		return CNN_UINT16;
	} else {
		return CNN_UINT8;
	}
}

/**
CNN Get Eltwise type

Get current CNN eltwise type.

@return CNN eltwise type.
*/
CNN_IO_TYPE cnn_get_elttype(BOOL cnn_id)
{
	T_ELTWISE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = ELTWISE_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	if ((reg1.bit.ELTWISE_INPUT_SIGN == 1) && (reg1.bit.ELTWISE_INPUT_BIT_DEPTH == 1)) {
		return CNN_INT16;
	} else if ((reg1.bit.ELTWISE_INPUT_SIGN == 1) && (reg1.bit.ELTWISE_INPUT_BIT_DEPTH == 0)) {
		return CNN_INT8;
	} else if ((reg1.bit.ELTWISE_INPUT_SIGN == 0) && (reg1.bit.ELTWISE_INPUT_BIT_DEPTH == 1)) {
		return CNN_UINT16;
	} else {
		return CNN_UINT8;
	}
}

/**
CNN Get Output 0 type

Get current CNN output 0 type.

@return CNN output 0 type.
*/
CNN_IO_TYPE cnn_get_out0type(BOOL cnn_id)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs1 = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs1, base_addr);
	if (reg1.bit.OUT0_BITDEPTH == 1 && reg1.bit.OUT0_SIGNEDNESS == 1) {
		return CNN_INT16;
	} else if (reg1.bit.OUT0_BITDEPTH == 0 && reg1.bit.OUT0_SIGNEDNESS == 1) {
		return CNN_INT8;
	} else if (reg1.bit.OUT0_BITDEPTH == 1 && reg1.bit.OUT0_SIGNEDNESS == 0) {
		return CNN_UINT16;
	} else {
		return CNN_UINT8;
	}
}

/**
CNN Get Output 1 type

Get current CNN output 1 type.

@return CNN output 1 type.
*/
CNN_IO_TYPE cnn_get_out1type(BOOL cnn_id)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs1 = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs1, base_addr);
	if (reg1.bit.OUT1_BITDEPTH == 1 && reg1.bit.OUT1_SIGNEDNESS == 1) {
		return CNN_INT16;
	} else if (reg1.bit.OUT1_BITDEPTH == 0 && reg1.bit.OUT1_SIGNEDNESS == 1) {
		return CNN_INT8;
	} else if (reg1.bit.OUT1_BITDEPTH == 1 && reg1.bit.OUT1_SIGNEDNESS == 0) {
		return CNN_UINT16;
	} else {
		return CNN_UINT8;
	}
}

/**
CNN Set Output0 Starting Offset parameters

CNN set output0 starting offset parameters.

@param[in] p_parm Output0 Starting Offset parameters.

@return ER Error cnn parameters are out of range.
*/
ER cnn_set_out0ofs(BOOL cnn_id, CNN_OUT_SIZE *p_parm)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_PAR;
	}

	//valid only when OUT0_EN= 1 and OUT0_MODE = ReLU0 or ReLU1
	if ((((cnn_get_io_enable(cnn_id) & CNN_OUT0_EN) != CNN_OUT0_EN) || (cnn_get_out0mode(cnn_id) == CNN_OUT0_INTERMEDIATE))
		&& (p_parm->out0_ofs > 0)) {
			DBG_WRN("CNN: valid only when OUT0_EN= 1 and OUT0_MODE = ReLU0 or ReLU1.\r\n");
	} else if (p_parm->out0_ofs > CNN_CONV_OUT0OFFSET_MAX) {
		DBG_ERR("CNN: For OUT0_MODE equals to relu0 or relu1, OUT0_OFFSET must be smaller than %d.\r\n", CNN_CONV_OUT0OFFSET_MAX);
		return E_PAR;
	}


	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.OUT0_OFFSET = p_parm->out0_ofs;
	CNN_SETDATA(ofs, reg1.reg, base_addr);

	return E_OK;
}

/**
CNN Get Output0 Starting Offset Parameters

CNN get output0 starting offset parameters.

@return output0 offset.
*/
CNN_OUT_SIZE cnn_get_out0ofs(BOOL cnn_id)
{
	CNN_OUT_SIZE out_size;
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	out_size.out0_ofs = reg1.bit.OUT0_OFFSET;
	return out_size;
}

/**
CNN Set Input/Output Enable/Disable

CNN input/output enable selection.

@param[in] enable Decide input/output option to be enabled or disabled.
\n-@b ENABLE: enable output0.
\n-@b DISABLE: disable output0.

@return None.
*/
VOID cnn_set_io_enable(BOOL cnn_id, BOOL enable, UINT32 io)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	if (enable) {
		reg1.reg |= io;
	} else {
		reg1.reg &= (~io);
	}

	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Input/Output Enable Status

Get current CNN Input/Output enable status.

@return CNN Input/Output enable status.
*/
UINT32 cnn_get_io_enable(BOOL cnn_id)
{
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	return (CNN_GETDATA(ofs, base_addr) & CNN_ALL_IO_EN);
}

/**
CNN Set Output0 mode

CNN set output0 mode.

@param[in] output0 mode is selected.

@return None.
*/
ER cnn_set_out0mode(BOOL cnn_id, CNN_OUT0_TYPE mode)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	if ((cnn_get_engmode(cnn_id) == CNN_DECONV)||(cnn_get_engmode(cnn_id) == CNN_SCALEUP)) {
		if (mode != CNN_OUT0_RELU0) {
			DBG_ERR("CNN for CNN_DECONV or CNN_SCALEUP, output 0 mode must set as 0!\r\n");
			return E_PAR;
		}
	}
	switch (mode) {
	case CNN_OUT0_RELU0:
		reg1.bit.OUT0_MODE = 0;
		break;
	case CNN_OUT0_INTERMEDIATE:
		reg1.bit.OUT0_MODE = 2;
		break;
	default:
		DBG_ERR("Unknown output 0 mode: %d!\r\n", mode);
		return E_PAR;
	}

	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Output 0 Mode

Get current output 0 mode.

@return CNN output 0 mode.
*/
CNN_OUT0_TYPE cnn_get_out0mode(BOOL cnn_id)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	return (CNN_OUT0_TYPE)reg1.bit.OUT0_MODE;
}

/**
CNN Set Kernel Enable/Disable

CNN kernel enable selection.

@param[in] enable Decide selected funtions are to be enabled or disabled.
\n-@b ENABLE: enable selected functions.
\n-@b DISABLE: disable selected functions.
@param[in] kerl Indicate the function(s)

@return None.
*/
VOID cnn_set_kerl_en(BOOL cnn_id, BOOL enable, UINT32 kerl)
{
	T_CNN_MODE_REGISTER reg1;
	T_DESIGN_DEBUG_REGISTER0 reg2;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;
	UINT32 ofs2 = DESIGN_DEBUG_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg2.reg = CNN_GETDATA(ofs2, base_addr);

	if (enable) {
		reg1.reg |= kerl;
	} else {
		reg1.reg &= (~kerl);
	}

	if (cnn_get_engmode(cnn_id) == CNN_CONV) {
		if ((kerl & CNN_CONV_KERL_EN) || (kerl & CNN_BNSCALE_KERL_EN)) {
			reg2.bit.BN_DMA_DISABLE = 0;
		} else {
			reg2.bit.BN_DMA_DISABLE = 1;
		}
	}

	CNN_SETDATA(ofs, reg1.reg, base_addr);
	CNN_SETDATA(ofs2, reg2.reg, base_addr);
}

/**
CNN Get Kernel Enable Status

Get current CNN kernel enable status.

@return CNN kernel enable status.
*/
UINT32 cnn_get_kerl_en(BOOL cnn_id)
{
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	return (CNN_GETDATA(ofs, base_addr) & CNN_ALL_KERL_EN);
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
CNN Set Linked List Starting Address

Set linked list DMA input starting address.

@param[in] addr0 DMA input linked list buffer (DMA to CNN) starting address 0.

@return None.
*/
VOID cnn_set_dmain_lladdr(BOOL cnn_id, UINT32 addr0)
{
	T_INPUT_CHANNEL_REGISTER_12 local_reg;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_CHANNEL_REGISTER_12_OFS;
	local_reg.reg = CNN_GETDATA(ofs, base_addr);
	local_reg.bit.DRAM_SAI8 = addr0 >> 2;
	CNN_SETDATA(ofs, local_reg.reg, base_addr);
}

/*
CNN Get Linked List Starting Address

Get linked list DMA input starting address.

@return CNN linked list starting address.
*/
UINT32 cnn_get_dmain_lladdr(BOOL cnn_id)
{
	UINT32 addr = 0, phy_addr = 0;
	T_INPUT_CHANNEL_REGISTER_12 local_reg;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_CHANNEL_REGISTER_12_OFS;
	local_reg.reg = CNN_GETDATA(ofs, base_addr);
	phy_addr = local_reg.bit.DRAM_SAI8 << 2;

#if 0   //(CNN_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}


/**
CNN Set Linked List Address Base

Set linked list DMA address base.

@param[in] addr0 DMA input linked list buffer (DMA to CNN) address base.

@return None.
*/
VOID cnn_set_dmain_lladdr_base(BOOL cnn_id, UINT32 addr0)
{
	T_LL_BASE_ADDRESS_REGISTER0 local_reg;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = LL_BASE_ADDRESS_REGISTER0_OFS;
	local_reg.reg = CNN_GETDATA(ofs, base_addr);
	local_reg.bit.LL_BASE_ADDR = addr0;
	CNN_SETDATA(ofs, local_reg.reg, base_addr);
}

/*
CNN Get Linked List Address Base

Get linked list DMA address base.

@return CNN linked list address base.
*/
UINT32 cnn_get_dmain_lladdr_base(BOOL cnn_id)
{
	UINT32 addr = 0, phy_addr = 0;
	T_LL_BASE_ADDRESS_REGISTER0 local_reg;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = LL_BASE_ADDRESS_REGISTER0_OFS;
	local_reg.reg = CNN_GETDATA(ofs, base_addr);
	phy_addr = local_reg.bit.LL_BASE_ADDR;

#if 0   //(CNN_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}
/**
CNN Set Input Starting Addresses

Set DMA input starting addresses.

@param[in] addr0 DMA input buffer (DMA to CNN) starting address 0.
@param[in] addr4 DMA input buffer (DMA to CNN) starting address 4.
@param[in] addr5 DMA input buffer (DMA to CNN) starting address 5.
@param[in] addr6 DMA input buffer (DMA to CNN) starting address 6.
@param[in] addr7 DMA input buffer (DMA to CNN) starting address 7.
@param[in] addr8 DMA input buffer (DMA to CNN) starting address 8.

@return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_set_dmain_addr(BOOL cnn_id, UINT32 addr0, UINT32 addr1, UINT32 addr2, UINT32 addr3,
	UINT32 addr4)
{
	T_INPUT_CHANNEL_REGISTER_0 local_reg0;
	T_INPUT_CHANNEL_REGISTER_8 local_reg4;
	T_INPUT_CHANNEL_REGISTER_9 local_reg5;
	T_INPUT_CHANNEL_REGISTER_10 local_reg6;
	T_INPUT_CHANNEL_REGISTER_11 local_reg7;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = INPUT_CHANNEL_REGISTER_0_OFS;
	UINT32 ofs4 = INPUT_CHANNEL_REGISTER_8_OFS;
	UINT32 ofs5 = INPUT_CHANNEL_REGISTER_9_OFS;
	UINT32 ofs6 = INPUT_CHANNEL_REGISTER_10_OFS;
	UINT32 ofs7 = INPUT_CHANNEL_REGISTER_11_OFS;


	//================================

	CNN_IO_TYPE in_type = cnn_get_intype(cnn_id);
	CNN_IO_TYPE elt_type = cnn_get_elttype(cnn_id);
	BOOL in_bitdepth = (in_type == CNN_INT16) | (in_type == CNN_UINT16);
	BOOL elt_bitdepth = (elt_type == CNN_INT16) | (elt_type == CNN_UINT16);

	//DRAM_SAI0
	if ((in_bitdepth == 1) && ((addr0 & 0x1) != 0)) {
		DBG_ERR("CNN: For 16-b input, addr0 must be 2-byte aligned.\r\n");
		return E_PAR;
	}
	//DRAM_SAI4
	//CONV: convolution weight
	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) &&  ((addr1 & 0x3) != 0)) {
		DBG_ERR("CNN: For CNN_CONV, addr4 must be word-aligned.\r\n");
		return E_PAR;
	}
	//DRAM_SAI5
	//FCD: kmeans quantization parameters
	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) && (cnn_get_kerl_en(cnn_id) & CNN_FCD_QUANTI_EN) && ((addr2 & 0x3) != 0)) {
		DBG_ERR("CNN: For CNN_CONV and CNN_FCD_QUANTI_EN, addr5 must be word-aligned.\r\n");
		return E_PAR;

	}
	//DRAM_SAI6
	//CONV: convolution bias + batchnorm + scale packed
	//Word-aligned.
	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) && ((addr3 & 0x3) != 0)) {
		DBG_ERR("CNN: For CNN_CONV, addr6 must be word-aligned.\r\n");
		return E_PAR;

	}
	//DRAM_SAI7
	//CONV: intermediate convolution channel stripe sum resultsWord-aligned.
	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISCHANNELSTRIPE_EN) && ((addr4 & 0x3) != 0)) {
		DBG_ERR("CNN: For CNN_CONV and CNN_IN_ISCHANNELSTRIPE_EN, addr7 must be word-aligned.\r\n");
		return E_PAR;

	}
	//ELT: eltwise input
	//If ELTWISE_INPUT_BIT_DEPTH =1 : Byte-aligned, Otherwise : 2-Byte-aligned.
	else if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_io_enable(cnn_id) & CNN_ELTWISE_KERL_EN) && (elt_bitdepth == 0) && ((addr4 & 0x1) != 0)) {
		DBG_ERR("CNN: For ELTWISE 8-b elt input, addr7 must be 2-byte aligned.\r\n");
		return E_PAR;
	}

	local_reg0.reg = CNN_GETDATA(ofs0, base_addr);
	local_reg0.bit.DRAM_SAI0 = addr0;

	local_reg4.reg = CNN_GETDATA(ofs4, base_addr);
	local_reg4.bit.DRAM_SAI4 = addr1 >> 2;

	local_reg5.reg = CNN_GETDATA(ofs5, base_addr);
	local_reg5.bit.DRAM_SAI5 = addr2 >> 2;

	local_reg6.reg = CNN_GETDATA(ofs6, base_addr);
	local_reg6.bit.DRAM_SAI6 = addr3 >> 2;

	local_reg7.reg = CNN_GETDATA(ofs7, base_addr);
	local_reg7.bit.DRAM_SAI7 = addr4;

	CNN_SETDATA(ofs0, local_reg0.reg, base_addr);
	CNN_SETDATA(ofs4, local_reg4.reg, base_addr);
	CNN_SETDATA(ofs5, local_reg5.reg, base_addr);
	CNN_SETDATA(ofs6, local_reg6.reg, base_addr);
	CNN_SETDATA(ofs7, local_reg7.reg, base_addr);
	return E_OK;
}

/**
CNN Get DMA input starting address

Get specifed DMA input buffer starting address.

@param[in] buf_id input ppb ID.

@return specifed DMA input buffer starting address.
*/
UINT32 cnn_get_dmain_addr(BOOL cnn_id, CNN_IN_BUFID buf_id)
{
	T_INPUT_CHANNEL_REGISTER_0 local_reg0;
	T_INPUT_CHANNEL_REGISTER_8 local_reg4;
	T_INPUT_CHANNEL_REGISTER_9 local_reg5;
	T_INPUT_CHANNEL_REGISTER_10 local_reg6;
	T_INPUT_CHANNEL_REGISTER_11 local_reg7;

	UINT32 addr = 0, phy_addr = 0;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = INPUT_CHANNEL_REGISTER_0_OFS;
	UINT32 ofs4 = INPUT_CHANNEL_REGISTER_8_OFS;
	UINT32 ofs5 = INPUT_CHANNEL_REGISTER_9_OFS;
	UINT32 ofs6 = INPUT_CHANNEL_REGISTER_10_OFS;
	UINT32 ofs7 = INPUT_CHANNEL_REGISTER_11_OFS;


	switch (buf_id) {
	case CNN_IN0_BUF:
		local_reg0.reg   = CNN_GETDATA(ofs0, base_addr);
		phy_addr       = local_reg0.bit.DRAM_SAI0; //input ch0
		break;
	case CNN_IN4_BUF:
		local_reg4.reg   = CNN_GETDATA(ofs4, base_addr);
		phy_addr       = local_reg4.bit.DRAM_SAI4 << 2; //FCD weights
		break;
	case CNN_IN5_BUF:
		local_reg5.reg   = CNN_GETDATA(ofs5, base_addr);
		phy_addr       = local_reg5.bit.DRAM_SAI5 << 2; //FCD kmeans table
		break;
	case CNN_IN6_BUF:
		local_reg6.reg   = CNN_GETDATA(ofs6, base_addr);
		phy_addr       = local_reg6.bit.DRAM_SAI6 << 2; //FCD bias
		break;
	case CNN_IN7_BUF:
		local_reg7.reg   = CNN_GETDATA(ofs7, base_addr);
		phy_addr       = local_reg7.bit.DRAM_SAI7 << 2; //intermediate
		break;
		//case CNN_IN8_BUF:
		//local_reg0.reg   = CNN_GETDATA(ofs8, base_addr);
		//phy_addr       = local_reg8.bit.DRAM_SAI8 << 2; //linked list
		//break;
	default:
		DBG_ERR("Error input biffer ID: %d\r\n", buf_id);
		return E_CTX;
	}

#if 0   //(CNN_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}

/**
CNN Set DMA input line offset

Set one DMA input line offset.

@param[in] ilofs DMA input line offset.

@return None.
*/
ER cnn_set_dma_lofs(BOOL cnn_id, CNN_DMAIO_LOFS *p_lofs)
{
	T_INPUT_CHANNEL_REGISTER_4 local_reg0;
	UINT32 ofs = INPUT_CHANNEL_REGISTER_4_OFS;
	T_INPUT_CHANNEL_REGISTER_5 local_reg1;
	UINT32 ofs1 = INPUT_CHANNEL_REGISTER_5_OFS;
	T_INPUT_CHANNEL_REGISTER_6 local_reg2;
	UINT32 ofs2 = INPUT_CHANNEL_REGISTER_6_OFS;
	T_INPUT_CHANNEL_REGISTER_7 local_reg3;
	UINT32 ofs3 = INPUT_CHANNEL_REGISTER_7_OFS;
	T_OUTPUT_CHANNEL_REGISTER_2 local_reg4;
	UINT32 ofs4 = OUTPUT_CHANNEL_REGISTER_2_OFS;
	T_OUTPUT_CHANNEL_REGISTER_3 local_reg5;
	UINT32 ofs5 = OUTPUT_CHANNEL_REGISTER_3_OFS;
	T_OUTPUT_CHANNEL_REGISTER_4 local_reg6;
	UINT32 ofs6 = OUTPUT_CHANNEL_REGISTER_4_OFS;
	T_OUTPUT_CHANNEL_REGISTER_5 local_reg7;
	UINT32 ofs7 = OUTPUT_CHANNEL_REGISTER_5_OFS;

	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);

	CNN_IO_TYPE in_type = cnn_get_intype(cnn_id);
	CNN_IO_TYPE elt_type = cnn_get_elttype(cnn_id);
	CNN_IO_TYPE out0_type = cnn_get_out0type(cnn_id);
	CNN_IO_TYPE out1_type = cnn_get_out1type(cnn_id);
	BOOL in_bitdepth = (in_type == CNN_INT16) | (in_type == CNN_UINT16);
	BOOL elt_bitdepth = (elt_type == CNN_INT16) | (elt_type == CNN_UINT16);
	BOOL out0_bitdepth = (out0_type == CNN_INT16) | (out0_type == CNN_UINT16);
	BOOL out1_bitdepth = (out1_type == CNN_INT16) | (out1_type == CNN_UINT16);
	CNN_IN_SIZE insize = cnn_get_insize(cnn_id);

	//=====================================input========================================
	//	DRAM_OFSI0
	//number of input line/batch offset. (For batch offset, not valid when CONV_EN = 1 & Batch > 1 & IN_INTERLACE_EN = 1)
	if (cnn_get_engmode(cnn_id) == CNN_CONV) {
		if (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) {
			//image / feature line offset:
			if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (in_bitdepth == 1) && ((p_lofs->in0_lofs & 0x1) != 0)) {
				DBG_ERR("CNN: For 16-b input OUT0_BITDEPTH=1 , CNN_DRAMOFSI0 is lineoffset, must be 2-byte aligned.\r\n");
				return E_PAR;
			}
		}
		else {
			//image / feature line offset:
			if (insize.batch <= 1) {
				if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (in_bitdepth == 1) && ((p_lofs->in0_lofs & 0x1) != 0)) {
					DBG_ERR("CNN: For 16-b input OUT0_BITDEPTH=1 , CNN_DRAMOFSI0 is lineoffset, must be 2-byte aligned.\r\n");
					return E_PAR;
				}
			}
			//Batch offset
			else {
				if ((insize.batch > 1) && ((cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) == 0)  && ((p_lofs->in0_lofs & 0x3) != 0)) {
					DBG_ERR("CNN: For multiple batch and CNN_IN_ISIMAGE_EN=0 , CNN_DRAMOFSI0 is batchoffset, word aligned will be more efficient.\r\n");
				}
			}
		}
	}

	//	DRAM_ OFSI1
	//number of input channel offset.
	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) || (cnn_get_io_enable(cnn_id) & CNN_IN_ISVSTRIPE_EN))) {
		if ((in_bitdepth == 1) && ((p_lofs->in1_lofs & 0x1) != 0)) {
			DBG_ERR("CNN: For 16-b input OUT0_BITDEPTH=1 , CNN_DRAMOFSI1 is channel offset, must be 2-byte aligned.\r\n");
			return E_PAR;
		}
	}
	//	DRAM_ OFSI2
	//number of eltwise input line offset.

	if (cnn_get_engmode(cnn_id) == CNN_CONV) {
		if ( cnn_get_kerl_en(cnn_id) & CNN_ELTWISE_KERL_EN) {
			//image / feature line offset:
			if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (elt_bitdepth == 1) && ((p_lofs->in2_lofs & 0x1) != 0)) {
				DBG_ERR("CNN: For 16-b input ELT_BITDEPTH=1 , CNN_DRAMOFSI2 is lineoffset, must be 2-byte aligned.\r\n");
				return E_PAR;
			}
			//Batch offset
			else {
				if ((insize.batch > 1) && ((cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) == 0)  && (cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && ((p_lofs->in2_lofs & 0x3) != 0)) {
					DBG_ERR("CNN: For feat and CNN_IN_ISHSTRIPE_EN=1 , CNN_DRAMOFSI2 is batch offset, word aligned will be more efficient.\r\n");
				}
			}
		}
	}
	//DRAM_ OFSI3
	//number of eltwise input channel offset:
	if (cnn_get_kerl_en(cnn_id) & CNN_ELTWISE_KERL_EN) {
		if ((cnn_get_engmode(cnn_id) == CNN_CONV) && ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) || (cnn_get_io_enable(cnn_id) & CNN_IN_ISVSTRIPE_EN))) {
			if ((elt_bitdepth == 1) && ((p_lofs->in3_lofs & 0x1) != 0)) {
				DBG_ERR("CNN: For 16-b input OUT0_BITDEPTH=1 , CNN_DRAMOFSI3 is channel offset, must be 2-byte aligned.\r\n");
				return E_PAR;
			}
		}
	}
	//=====================================output========================================
	//DRAM_OFSO0
	//number of output line/batch offset of Out0.
	if (cnn_get_engmode(cnn_id) == CNN_CONV && (cnn_get_io_enable(cnn_id) & CNN_OUT0_EN)) {
		if (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) {
			//image / feature line offset:
			if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (out0_bitdepth == 1) && ((p_lofs->out0_lofs & 0x1) != 0)) {
				DBG_ERR("CNN: For 16-b output CNN_IN_ISHSTRIPE_EN=1 , CNN_DRAMOFSO0 is lineoffset, must be 2-byte aligned.\r\n");
				return E_PAR;
			}
		}
		else {
			//image / feature line offset:
			if (insize.batch <= 1) {
				if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (out0_bitdepth == 1) && ((p_lofs->out0_lofs & 0x1) != 0)) {
					DBG_ERR("CNN: For 16-b output CNN_IN_ISHSTRIPE_EN=1 , CNN_DRAMOFSO0 is lineoffset, must be 2-byte aligned.\r\n");
					return E_PAR;
				}
			}
			//Batch offset
			else {
				if ((insize.batch > 1) && ((p_lofs->out0_lofs & 0x3) != 0)) {
					DBG_ERR("CNN: For multiple batch , CNN_DRAMOFSO0 is batch offset, word aligned will be more efficient.\r\n");
				}
			}
		}
	}

	//DRAM_OFSO1
	//number of output channel offset of Out0.
	if ((cnn_get_io_enable(cnn_id) & CNN_OUT0_EN) && ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) || (cnn_get_io_enable(cnn_id) & CNN_IN_ISVSTRIPE_EN))) {
		if ((out0_bitdepth == 1) && ((p_lofs->out1_lofs & 0x1) != 0)) {
			DBG_ERR("CNN: For 16-b out0_bitdepth=1 , CNN_DRAMOFSO1 is channel offset, must be 2-byte aligned.\r\n");
			return E_PAR;
		}
	}
	//DRAM_OFSO2
	//number of output line/batch offset of Out1.
	if (cnn_get_engmode(cnn_id) == CNN_CONV && (cnn_get_io_enable(cnn_id) & CNN_OUT1_EN)) {
		if (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) {
			//image / feature line offset:
			if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (out1_bitdepth == 1) && ((p_lofs->out2_lofs & 0x1) != 0)) {
				DBG_ERR("CNN: For 16-b output CNN_IN_ISHSTRIPE_EN=1 , CNN_DRAMOFSO2 is lineoffset, must be 2-byte aligned.\r\n");
				return E_PAR;
			}
		}
		else {
			//image / feature line offset:
			if (insize.batch <= 1) {
				if ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) && (out1_bitdepth == 1) && ((p_lofs->out2_lofs & 0x1) != 0)) {
					DBG_ERR("CNN: For 16-b output CNN_IN_ISHSTRIPE_EN=1 , CNN_DRAMOFSO2 is lineoffset, must be 2-byte aligned.\r\n");
					return E_PAR;
				}
			}
			//Batch offset
			else {
				if ((insize.batch > 1) && ((p_lofs->out2_lofs & 0x3) != 0)) {
					DBG_ERR("CNN: For multiple batch , CNN_DRAMOFSO0 is batch offset, word aligned will be more efficient.\r\n");
				}
			}
		}
	}
	//DRAM_OFSO3
	//number of output channel offset of Out1.
	if ((cnn_get_io_enable(cnn_id) & CNN_OUT1_EN) && ((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) || (cnn_get_io_enable(cnn_id) & CNN_IN_ISVSTRIPE_EN))) {
		if ((out1_bitdepth == 1) && ((p_lofs->out3_lofs & 0x1) != 0)) {
			DBG_ERR("CNN: For 16-b out1_bitdepth=1 , DRAMOFSO3 is channel offset, must be 2-byte aligned.\r\n");
			return E_PAR;
		}
	}

	//OFSI0
	local_reg0.reg = CNN_GETDATA(ofs, base_addr);
	local_reg0.bit.DRAM_OFSI0 = p_lofs->in0_lofs;
	CNN_SETDATA(ofs, local_reg0.reg, base_addr);
	//OFSI1
	local_reg1.reg = CNN_GETDATA(ofs1, base_addr);
	local_reg1.bit.DRAM_OFSI1 = p_lofs->in1_lofs;
	CNN_SETDATA(ofs1, local_reg1.reg, base_addr);
	//OFSI2
	local_reg2.reg = CNN_GETDATA(ofs2, base_addr);
	local_reg2.bit.DRAM_OFSI2 = p_lofs->in2_lofs;
	CNN_SETDATA(ofs2, local_reg2.reg, base_addr);
	//OFSI3
	local_reg3.reg = CNN_GETDATA(ofs3, base_addr);
	local_reg3.bit.DRAM_OFSI3 = p_lofs->in3_lofs;
	CNN_SETDATA(ofs3, local_reg3.reg, base_addr);

	//OFSO0
	local_reg4.reg = CNN_GETDATA(ofs4, base_addr);
	local_reg4.bit.DRAM_OFSO0 = p_lofs->out0_lofs;
	CNN_SETDATA(ofs4, local_reg4.reg, base_addr);
	//OFSO1
	local_reg5.reg = CNN_GETDATA(ofs5, base_addr);
	local_reg5.bit.DRAM_OFSO1 = p_lofs->out1_lofs;
	CNN_SETDATA(ofs5, local_reg5.reg, base_addr);
	//OFSO2
	local_reg6.reg = CNN_GETDATA(ofs6, base_addr);
	local_reg6.bit.DRAM_OFSO2 = p_lofs->out2_lofs;
	CNN_SETDATA(ofs6, local_reg6.reg, base_addr);
	//OFSO3
	local_reg7.reg = CNN_GETDATA(ofs7, base_addr);
	local_reg7.bit.DRAM_OFSO3 = p_lofs->out3_lofs;
	CNN_SETDATA(ofs7, local_reg7.reg, base_addr);
	return E_OK;
}

/**
CNN Get DMA line offset

Get specifed DMA line offset.

@return specifed DMA line offset.
*/
CNN_DMAIO_LOFS cnn_get_dmain_lofs(BOOL cnn_id)
{
	CNN_DMAIO_LOFS lofs;
	T_INPUT_CHANNEL_REGISTER_4 reg0;
	T_INPUT_CHANNEL_REGISTER_5 reg1;
	T_INPUT_CHANNEL_REGISTER_6 reg2;
	T_INPUT_CHANNEL_REGISTER_7 reg3;

	T_OUTPUT_CHANNEL_REGISTER_2 reg4;
	T_OUTPUT_CHANNEL_REGISTER_3 reg5;
	T_OUTPUT_CHANNEL_REGISTER_4 reg6;
	T_OUTPUT_CHANNEL_REGISTER_5 reg7;

	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = INPUT_CHANNEL_REGISTER_4_OFS;
	UINT32 ofs1 = INPUT_CHANNEL_REGISTER_5_OFS;
	UINT32 ofs2 = INPUT_CHANNEL_REGISTER_6_OFS;
	UINT32 ofs3 = INPUT_CHANNEL_REGISTER_7_OFS;

	UINT32 ofs4 = OUTPUT_CHANNEL_REGISTER_2_OFS;
	UINT32 ofs5 = OUTPUT_CHANNEL_REGISTER_3_OFS;
	UINT32 ofs6 = OUTPUT_CHANNEL_REGISTER_4_OFS;
	UINT32 ofs7 = OUTPUT_CHANNEL_REGISTER_5_OFS;

	reg0.reg = CNN_GETDATA(ofs0, base_addr);
	lofs.in0_lofs = reg0.bit.DRAM_OFSI0;

	reg1.reg = CNN_GETDATA(ofs1, base_addr);
	lofs.in1_lofs = reg1.bit.DRAM_OFSI1;

	reg2.reg = CNN_GETDATA(ofs2, base_addr);
	lofs.in2_lofs = reg2.bit.DRAM_OFSI2;

	reg3.reg = CNN_GETDATA(ofs3, base_addr);
	lofs.in3_lofs = reg3.bit.DRAM_OFSI3;

	reg4.reg = CNN_GETDATA(ofs4, base_addr);
	lofs.out0_lofs = reg4.bit.DRAM_OFSO0;

	reg5.reg = CNN_GETDATA(ofs5, base_addr);
	lofs.out1_lofs = reg5.bit.DRAM_OFSO1;

	reg6.reg = CNN_GETDATA(ofs6, base_addr);
	lofs.out2_lofs = reg6.bit.DRAM_OFSO2;

	reg7.reg = CNN_GETDATA(ofs7, base_addr);
	lofs.out3_lofs = reg7.bit.DRAM_OFSO3;

	return lofs;
}

/**
CNN Set Output 0 Starting Addresses

Set one DMA output 0 starting addresses.

@param[in] addr0 DMA output 0 result (CNN to DMA) starting address.

@return None.
*/
ER cnn_set_dmaout0_addr(BOOL cnn_id, UINT32 addr0)
{
	T_OUTPUT_CHANNEL_REGISTER_0 local_reg0;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = OUTPUT_CHANNEL_REGISTER_0_OFS;
	CNN_IO_TYPE out0_type = cnn_get_out0type(cnn_id);
	BOOL out0_bitdepth = (out0_type == CNN_INT16) | (out0_type == CNN_UINT16);

	//	DRAM_SAO0
	if (cnn_get_io_enable(cnn_id) & CNN_OUT0_EN) {
		if ((out0_bitdepth == 1) && ((addr0 & 0x1) != 0)) {
			DBG_ERR("CNN: For out0_bitdepth = 1, addr0 must be 2-byte aligned.\r\n");
			return E_PAR;
		}
	}

	local_reg0.reg = CNN_GETDATA(ofs, base_addr);
	local_reg0.bit.DRAM_SAO0 = addr0;
	CNN_SETDATA(ofs, local_reg0.reg, base_addr);
	return E_OK;
}

/**
CNN Get DMA output 0 starting address

Get specifed DMA output 0 result (CNN to DMA) starting address

@return specifed DMA output 0 result (CNN to DMA) starting address
*/
UINT32 cnn_get_dmaout0_addr(BOOL cnn_id)
{
	T_OUTPUT_CHANNEL_REGISTER_0 local_reg0;
	UINT32 addr, phy_addr;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = OUTPUT_CHANNEL_REGISTER_0_OFS;

	local_reg0.reg = CNN_GETDATA(ofs, base_addr);
	phy_addr = local_reg0.bit.DRAM_SAO0;

#if 0   //(CNN_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}

/**
CNN Set Output 1 Starting Addresses

Set one DMA output 1 starting addresses.

@param[in] addr0 DMA output 1 result (CNN to DMA) starting address.

@return None.
*/
ER cnn_set_dmaout1_addr(BOOL cnn_id, UINT32 addr0)
{
	T_OUTPUT_CHANNEL_REGISTER_1 local_reg0;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = OUTPUT_CHANNEL_REGISTER_1_OFS;
	CNN_IO_TYPE out1_type = cnn_get_out1type(cnn_id);
	BOOL out1_bitdepth = (out1_type == CNN_INT16) | (out1_type == CNN_UINT16);

	//	DRAM_ SAO1
	if (cnn_get_io_enable(cnn_id) & CNN_OUT1_EN) {
		if ((out1_bitdepth == 1) && ((addr0 & 0x1) != 0)) {
			DBG_ERR("CNN: For out1_bitdepth = 1, addr0 must be 2-byte aligned.\r\n");
			return E_PAR;
		}
	}

	local_reg0.reg = CNN_GETDATA(ofs, base_addr);
	local_reg0.bit.DRAM_SAO1 = addr0;
	CNN_SETDATA(ofs, local_reg0.reg, base_addr);
	return E_OK;
}

/**
CNN Get DMA output 1 starting address

Get specifed DMA output 1 result (CNN to DMA) starting address.

@return specifed DMA output 1 result (CNN to DMA) starting address.
*/
UINT32 cnn_get_dmaout1_addr(BOOL cnn_id)
{
	T_OUTPUT_CHANNEL_REGISTER_1 local_reg0;
	UINT32 addr, phy_addr;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = OUTPUT_CHANNEL_REGISTER_1_OFS;

	local_reg0.reg = CNN_GETDATA(ofs, base_addr);
	phy_addr = local_reg0.bit.DRAM_SAO1;

#if 0   //(CNN_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}

/**
CNN Set Input Size

CNN set input Size.

@param[in] p_size input size.

@return ER Error input size is out of range.
*/
ER cnn_set_insize(BOOL cnn_id, CNN_IN_SIZE *p_size)
{
	CNN_CONVKERL_PARM conv_para;
	CNN_POOL_KER_SIZE local_pool_ker_sz;
	CNN_LOCAL_POOL_PARM local_pool_parm;
	const CNN_MODE_TYPE eng_mode = cnn_get_engmode(cnn_id);
	UINT32 width, height, channel, batch;
	UINT32 pool_ker_w, pool_ker_h;
	T_INPUT_SIZE_REGISTER0 reg1;
	T_INPUT_SIZE_REGISTER1 reg2;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = INPUT_SIZE_REGISTER0_OFS;
	UINT32 ofs1 = INPUT_SIZE_REGISTER1_OFS;

	if (p_size == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	width  	= p_size->width;
	height  = p_size->height;
	channel = p_size->channel;
	batch   = p_size->batch;


	if (eng_mode == CNN_CONV) {
		//channel
		if (channel < CNN_CHANNEL_MIN) {
			DBG_ERR("CNN: For conv mode, Input channel must be >= %d : %d -> %d\r\n", CNN_CHANNEL_MIN, (int)channel, CNN_CHANNEL_MIN);
			return E_PAR;
		} else if (channel > CNN_CHANNEL_MAX) {
			DBG_ERR("CNN: For conv mode, Input channel must be <= %d : %d -> %d\r\n", CNN_CHANNEL_MAX, (int)channel, CNN_CHANNEL_MAX);
			return E_PAR;
		}
		//width & height
		if (width > CNN_WIDTH_MAX) {
			DBG_ERR("CNN: Input width must be <= %d : %d -> %d\r\n", CNN_WIDTH_MAX, (int)width, CNN_WIDTH_MAX);
			return E_PAR;
		} else if (width < CNN_WIDTH_MIN) {
			DBG_ERR("CNN: Input width must be >= %d : %d -> %d\r\n", CNN_WIDTH_MIN, (int)width, CNN_WIDTH_MIN);
			return E_PAR;
		}

		if (height > CNN_HEIGHT_MAX) {
			DBG_ERR("CNN: Input height must be <= %d : %d -> %d\r\n", CNN_HEIGHT_MAX, (int)height, CNN_HEIGHT_MAX);
			return E_PAR;
		} else if (height < CNN_HEIGHT_MIN) {
			DBG_ERR("CNN: Input height must be >= %d : %d -> %d\r\n", CNN_HEIGHT_MIN, (int)height, CNN_HEIGHT_MIN);
			return E_PAR;
		}
		if ((cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN)) {
			conv_para = cnn_get_convparm(cnn_id);
			//width & height
			if (conv_para.conv_kersize == CNN_CONV_KERSZ_11_11) {
				if ((width + 5*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K11_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 11x11 img mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K11_WIDTH_MIN, (int)width, CNN_CONV_K11_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 5*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K11_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 11x11 img mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K11_HEIGHT_MIN, (int)height, CNN_CONV_K11_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_7_7) {
				if ((width + 3*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K7_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 7X7 img mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K7_WIDTH_MIN, (int)width, CNN_CONV_K7_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 3*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K7_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 7X7 img mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K7_HEIGHT_MIN, (int)height, CNN_CONV_K7_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_5_5) {
				if ((width + 2*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K5_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 5X5 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K5_WIDTH_MIN, (int)width, CNN_CONV_K5_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 2*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K5_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 5X5 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K5_HEIGHT_MIN, (int)height, CNN_CONV_K5_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_3_3) {
				if ((width + 1*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K3_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 3X3 img mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K3_WIDTH_MIN, (int)width, CNN_CONV_K3_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 1*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K3_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 3X3 img mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K3_HEIGHT_MIN, (int)height, CNN_CONV_K3_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_1_1) {
				if (width < CNN_CONV_K1_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 1X1 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_WIDTH_MIN, (int)width, CNN_CONV_K1_WIDTH_MIN);
					return E_PAR;
				}
				if (height < CNN_CONV_K1_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 1X1 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_HEIGHT_MIN, (int)height, CNN_CONV_K1_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_7_1) {
				if ((width + 3*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K7_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 7X1 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K7_WIDTH_MIN, (int)width, CNN_CONV_K7_WIDTH_MIN);
					return E_PAR;
				}
				if (height < CNN_CONV_K1_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 7X1 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_HEIGHT_MIN, (int)height, CNN_CONV_K1_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_1_7) {
				if (width < CNN_CONV_K1_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 1X7 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_WIDTH_MIN, (int)width, CNN_CONV_K1_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 3*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K7_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 1X7 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K7_HEIGHT_MIN, (int)height, CNN_CONV_K7_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_5_1) {
				if ((width + 2*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K5_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 5X1 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K5_WIDTH_MIN, (int)width, CNN_CONV_K5_WIDTH_MIN);
					return E_PAR;
				}
				if (height < CNN_CONV_K1_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 5x1 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_HEIGHT_MIN, (int)height, CNN_CONV_K1_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_1_5) {
				if (width < CNN_CONV_K1_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 1X5 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_WIDTH_MIN, (int)width, CNN_CONV_K1_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 2*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K5_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 1X5 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K5_HEIGHT_MIN, (int)height, CNN_CONV_K5_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_3_1) {
				if ((width + 1*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K3_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 3X1 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K3_WIDTH_MIN, (int)width, CNN_CONV_K3_WIDTH_MIN);
					return E_PAR;
				}
				if (height < CNN_CONV_K1_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 3X1 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_HEIGHT_MIN, (int)height, CNN_CONV_K1_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_1_3) {
				if (width < CNN_CONV_K1_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 1X3 feat mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K1_WIDTH_MIN, (int)width, CNN_CONV_K1_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 1*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K3_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 1X3 feat mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K3_HEIGHT_MIN, (int)height, CNN_CONV_K3_HEIGHT_MIN);
					return E_PAR;
				}
			} else if (conv_para.conv_kersize == CNN_CONV_KERSZ_9_9) {
				if ((width + 4*(conv_para.is_left_pad + conv_para.is_right_pad)) < CNN_CONV_K9_WIDTH_MIN) {
					DBG_ERR("CNN: for kernel 9X9 img mode, Input width + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K9_WIDTH_MIN, (int)width, CNN_CONV_K9_WIDTH_MIN);
					return E_PAR;
				}
				if ((height+ 4*(conv_para.is_top_pad + conv_para.is_bot_pad)) < CNN_CONV_K9_HEIGHT_MIN) {
					DBG_ERR("CNN: for kernel 9X9 img mode, Input height + pad must be >= %d : %d -> %d\r\n", CNN_CONV_K9_HEIGHT_MIN, (int)height, CNN_CONV_K9_HEIGHT_MIN);
					return E_PAR;
				}
			}
			//img mode
			if (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) {
				//channel
				if (channel < CNN_CONV_IMAGE_CHANNEL_MIN) {
					DBG_ERR("CNN: For image mode, Input channel must be >= %d : %d -> %d\r\n", CNN_CONV_IMAGE_CHANNEL_MIN, (int)channel, CNN_CONV_IMAGE_CHANNEL_MIN);
					return E_PAR;
				}
				if (channel > CNN_CONV_IMAGE_CHANNEL_MAX) {
					DBG_ERR("CNN: For image mode, Input channel must be <= %d : %d -> %d\r\n", CNN_CONV_IMAGE_CHANNEL_MAX, (int)channel, CNN_CONV_IMAGE_CHANNEL_MAX);
					return E_PAR;
				}
				//batch
				if (batch > CNN_CONV_BATCH_MIN) {
					DBG_ERR("CNN: For image mode, CNN_BATCHNUM must be 1 : %d -> 1\r\n", (int)batch);
					return E_PAR;
				}
			} else {//feature mode.
				//channel
				if (channel < CNN_CHANNEL_MIN) {
					DBG_ERR("CNN: For image mode, Input channel must be >= %d : %d -> %d\r\n", CNN_CHANNEL_MIN, (int)channel, CNN_CHANNEL_MIN);
					return E_PAR;
				}
				if (channel > CNN_CHANNEL_MAX) {
					DBG_ERR("CNN: For feat mode, Input channel must be <= %d : %d -> %d\r\n", CNN_CHANNEL_MAX, (int)channel, CNN_CHANNEL_MAX);
					return E_PAR;
				}
				//batch
				if (((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) || (cnn_get_io_enable(cnn_id) & CNN_IN_ISVSTRIPE_EN)) && (batch > CNN_CONV_BATCH_MIN)) {
					DBG_ERR("CNN: For multiple-stripe(h-streip = 1 or v-stripe = 1), CNN_BATCHNUM must be 1 : %d -> 1\r\n", (int)batch);
					return E_PAR;
				}
				if (((cnn_get_io_enable(cnn_id) & CNN_IN_ISHSTRIPE_EN) || (cnn_get_io_enable(cnn_id) & CNN_IN_ISVSTRIPE_EN)) && (batch > CNN_CONV_BATCH_MIN)) {
					DBG_ERR("CNN: For multiple-stripe(h-streip = 1 or v-stripe = 1), CNN_BATCHNUM must be 1 : %d -> 1\r\n", (int)batch);
					return E_PAR;
				}
				if (batch > CNN_CONV_BATCH_MAX) {
					DBG_ERR("CNN: For convolution, input batch must be <= %d : %d -> %d\r\n", CNN_CONV_BATCH_MAX, (int)batch, CNN_CONV_BATCH_MAX);
					return E_PAR;
				} else if (batch < CNN_CONV_BATCH_MIN) {
					DBG_ERR("CNN: For convolution, input batch must be >= %d : %d -> %d\r\n", CNN_CONV_BATCH_MIN, (int)batch, CNN_CONV_BATCH_MIN);
					return E_PAR;
				}
			}

		}

		if ((cnn_get_kerl_en(cnn_id) & CNN_ELTWISE_KERL_EN)) {
			//channel
			if (channel < CNN_CHANNEL_MIN) {
				DBG_ERR("CNN: For eltwise on, Input channel must be >= %d : %d -> %d\r\n", CNN_CHANNEL_MIN, (int)channel, CNN_CHANNEL_MIN);
				return E_PAR;
			}
			if (channel > CNN_CHANNEL_MAX) {
				DBG_ERR("CNN: For eltwise on, Input channel must be <= %d : %d -> %d\r\n", CNN_CHANNEL_MAX, (int)channel, CNN_CHANNEL_MAX);
				return E_PAR;
			}

			//width & height
			if (width > CNN_ELT_WIDTH_MAX) {
				DBG_ERR("CNN: For eltwise on, Input width must be <= %d : %d -> %d\r\n", CNN_ELT_WIDTH_MAX, (int)width, CNN_ELT_WIDTH_MAX);
				return E_PAR;
			} else if (width < CNN_ELT_WIDTH_MIN) {
				DBG_ERR("CNN: For eltwise on, Input width must be >= %d : %d -> %d\r\n", CNN_ELT_WIDTH_MIN, (int)width, CNN_ELT_WIDTH_MIN);
				return E_PAR;
			}

			if (height > CNN_ELT_HEIGHT_MAX) {
				DBG_ERR("CNN: For eltwise on, Input height must be <= %d : %d -> %d\r\n", CNN_ELT_HEIGHT_MAX, (int)height, CNN_ELT_HEIGHT_MAX);
				return E_PAR;
			} else if (height < CNN_ELT_HEIGHT_MIN) {
				DBG_ERR("CNN: For eltwise on, Input height must be >= %d : %d -> %d\r\n", CNN_ELT_HEIGHT_MIN, (int)height, CNN_ELT_HEIGHT_MIN);
				return E_PAR;
			}
		}

		if ((cnn_get_kerl_en(cnn_id) & CNN_POOLING_KERL_EN)) {
			//channel
			if (channel < CNN_CHANNEL_MIN) {
				DBG_ERR("CNN: For pool on, Input channel must be >= %d : %d -> %d\r\n", CNN_CHANNEL_MIN, (int)channel, CNN_CHANNEL_MIN);
				return E_PAR;
			}
			if (channel > CNN_CHANNEL_MAX) {
				DBG_ERR("CNN: For pool on, Input channel must be <= %d : %d -> %d\r\n", CNN_CHANNEL_MAX, (int)channel, CNN_CHANNEL_MAX);
				return E_PAR;
			}

			//width & height
			if (cnn_get_poolmode(cnn_id) == CNN_POOLING_LOCAL) {
				local_pool_parm = cnn_get_localpool_parm(cnn_id);
				local_pool_ker_sz = cnn_get_localpool_kersize(cnn_id);
				pool_ker_w = local_pool_ker_sz + 2;
				pool_ker_h = local_pool_ker_sz + 2;
				if(local_pool_ker_sz < 2){//pool_ker_sz = 2x2 or 3x3
					if (width > CNN_POOL_WIDTH_MAX) {
						DBG_ERR("CNN: For pool on, Input width must be <= %d : %d -> %d\r\n", CNN_POOL_WIDTH_MAX, (int)width, CNN_POOL_WIDTH_MAX);
						return E_PAR;
					} else if ((width + 1*(local_pool_parm.is_left_pad+local_pool_parm.is_right_pad)) < pool_ker_w) {
						DBG_ERR("CNN: For pool on(local pool), Input width must be >= pool_ker_w %d : %d -> %d\r\n", pool_ker_w, (int)width, pool_ker_w);
						return E_PAR;
					}

					if (height > CNN_POOL_HEIGHT_MAX) {
						DBG_ERR("CNN: For pool on, Input height must be <= %d : %d -> %d\r\n", CNN_POOL_HEIGHT_MAX, (int)height, CNN_POOL_HEIGHT_MAX);
						return E_PAR;
					} else if ((height + 1*(local_pool_parm.is_top_pad+local_pool_parm.is_bot_pad)) < pool_ker_h) {
						DBG_ERR("CNN: For pool on(local pool), Input height must be >= pool_ker_h %d : %d -> %d\r\n", pool_ker_h, (int)height, pool_ker_h);
						return E_PAR;
					}
				}
				else{//pool_ker_sz = 4x4 or 5x5
					if (width > CNN_POOL_WIDTH_MAX) {
						DBG_ERR("CNN: For pool on, Input width must be <= %d : %d -> %d\r\n", CNN_POOL_WIDTH_MAX, (int)width, CNN_POOL_WIDTH_MAX);
						return E_PAR;
					} else if ((width + 2*(local_pool_parm.is_left_pad+local_pool_parm.is_right_pad)) < pool_ker_w) {
						DBG_ERR("CNN: For pool on(local pool), Input width + pad must be >= pool_ker_w %d : %d -> %d\r\n", pool_ker_w, (int)width, pool_ker_w);
						return E_PAR;
					}

					if (height > CNN_POOL_HEIGHT_MAX) {
						DBG_ERR("CNN: For pool on, Input height must be <= %d : %d -> %d\r\n", CNN_POOL_HEIGHT_MAX, (int)height, CNN_POOL_HEIGHT_MAX);
						return E_PAR;
					} else if ((height + 2*(local_pool_parm.is_top_pad+local_pool_parm.is_bot_pad)) < pool_ker_h) {
						DBG_ERR("CNN: For pool on(local pool), Input height + pad must be >= pool_ker_h %d : %d -> %d\r\n", pool_ker_h, (int)height, pool_ker_h);
						return E_PAR;
					}
				}
			} else {
				if (width > CNN_POOL_WIDTH_MAX) {
					DBG_ERR("CNN: For pool on, Input width must be <= %d : %d -> %d\r\n", CNN_POOL_WIDTH_MAX, (int)width, CNN_POOL_WIDTH_MAX);
					return E_PAR;
				} 

				if (height > CNN_POOL_HEIGHT_MAX) {
					DBG_ERR("CNN: For pool on, Input height must be <= %d : %d -> %d\r\n", CNN_POOL_HEIGHT_MAX, (int)height, CNN_POOL_HEIGHT_MAX);
					return E_PAR;
				} 
			}
		}
	} else if (eng_mode == CNN_DECONV) {
		//channel
		if (channel < CNN_CHANNEL_MIN) {
			DBG_ERR("CNN: For deconv, Input channel must be >= %d : %d -> %d\r\n", CNN_CHANNEL_MIN, (int)channel, CNN_CHANNEL_MIN);
			return E_PAR;
		}
		if (channel > CNN_CHANNEL_MAX) {
			DBG_ERR("CNN: For deconv, Input channel must be <= %d : %d -> %d\r\n", CNN_CHANNEL_MAX, (int)channel, CNN_CHANNEL_MAX);
			return E_PAR;
		}
		//width & height
		if (width > CNN_DECONV_WIDTH_MAX) {
			DBG_ERR("CNN: For deconv, Input width must be <= %d : %d -> %d\r\n", CNN_DECONV_WIDTH_MAX, (int)width, CNN_DECONV_WIDTH_MAX);
			return E_PAR;
		} else if (width < CNN_DECONV_WIDTH_MIN) {
			DBG_ERR("CNN: For deconv, Input width must be >= %d : %d -> %d\r\n", CNN_DECONV_WIDTH_MIN, (int)width, CNN_DECONV_WIDTH_MIN);
			return E_PAR;
		}

		if (height > CNN_DECONV_HEIGHT_MAX) {
			DBG_ERR("CNN: For deconv, Input height must be <= %d : %d -> %d\r\n", CNN_DECONV_HEIGHT_MAX, (int)height, CNN_DECONV_HEIGHT_MAX);
			return E_PAR;
		} else if (height < CNN_DECONV_HEIGHT_MIN) {
			DBG_ERR("CNN: For deconv, Input height must be >= %d : %d -> %d\r\n", CNN_DECONV_HEIGHT_MIN, (int)height, CNN_DECONV_HEIGHT_MIN);
			return E_PAR;
		}
		//batch
		if (batch > CNN_CONV_BATCH_MAX) {
			DBG_ERR("CNN: For deconv, input batch must be <= %d : %d -> %d\r\n", CNN_CONV_BATCH_MAX, (int)batch, CNN_CONV_BATCH_MAX);
			return E_PAR;
		} else if (batch < CNN_CONV_BATCH_MIN) {
			DBG_ERR("CNN: For deconv, input batch must be >= %d : %d -> %d\r\n", CNN_CONV_BATCH_MIN, (int)batch, CNN_CONV_BATCH_MIN);
			return E_PAR;
		}
	} else if (eng_mode == CNN_SCALEUP) {
		//channel
		if (channel < CNN_CHANNEL_MIN) {
			DBG_ERR("CNN: For scale-up, Input channel must be >= %d : %d -> %d\r\n", CNN_CHANNEL_MIN, (int)channel, CNN_CHANNEL_MIN);
			return E_PAR;
		}
		if (channel > CNN_CHANNEL_MAX) {
			DBG_ERR("CNN: For scale-up, Input channel must be <= %d : %d -> %d\r\n", CNN_CHANNEL_MAX, (int)channel, CNN_CHANNEL_MAX);
			return E_PAR;
		}
		//width & height
		if (width > CNN_SCALE_WIDTH_MAX) {
			DBG_ERR("CNN: For scale-up, Input width must be <= %d : %d -> %d\r\n", CNN_SCALE_WIDTH_MAX, (int)width, CNN_SCALE_WIDTH_MAX);
			return E_PAR;
		} else if (width < CNN_SCALE_WIDTH_MIN) {
			DBG_ERR("CNN: For scale-up, Input width must be >= %d : %d -> %d\r\n", CNN_SCALE_WIDTH_MIN, (int)width, CNN_SCALE_WIDTH_MIN);
			return E_PAR;
		}

		if (height > CNN_SCALE_HEIGHT_MAX) {
			DBG_ERR("CNN: For scale-up, Input height must be <= %d : %d -> %d\r\n", CNN_SCALE_HEIGHT_MAX, (int)height, CNN_SCALE_HEIGHT_MAX);
			return E_PAR;
		} else if (height < CNN_SCALE_HEIGHT_MIN) {
			DBG_ERR("CNN: For scale-up, Input height must be >= %d : %d -> %d\r\n", CNN_SCALE_HEIGHT_MIN, (int)height, CNN_SCALE_HEIGHT_MIN);
			return E_PAR;
		}
		//batch
		if (batch > CNN_CONV_BATCH_MAX) {
			DBG_ERR("CNN: For scale-up, input batch must be <= %d : %d -> %d\r\n", CNN_CONV_BATCH_MAX, (int)batch, CNN_CONV_BATCH_MAX);
			return E_PAR;
		} else if (batch < CNN_CONV_BATCH_MIN) {
			DBG_ERR("CNN: For scale-up, input batch must be >= %d : %d -> %d\r\n", CNN_CONV_BATCH_MIN, (int)batch, CNN_CONV_BATCH_MIN);
			return E_PAR;
		}
	}

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	reg1.bit.WIDTH  = width;
	reg1.bit.HEIGHT = height;

	reg2.reg = CNN_GETDATA(ofs1, base_addr);
	reg2.bit.CHANNEL      = channel;
	reg2.bit.BATCH_NUM    = batch;

	CNN_SETDATA(ofs0, reg1.reg, base_addr);
	CNN_SETDATA(ofs1, reg2.reg, base_addr);

	return E_OK;
}

/**
CNN Get Input Size

Get current input size.

@return CNN_IN_SIZE input size.
*/
CNN_IN_SIZE cnn_get_insize(BOOL cnn_id)
{
	CNN_IN_SIZE size;
	T_INPUT_SIZE_REGISTER0 reg1;
	T_INPUT_SIZE_REGISTER1 reg2;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = INPUT_SIZE_REGISTER0_OFS;
	UINT32 ofs1 = INPUT_SIZE_REGISTER1_OFS;

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	size.width      = reg1.bit.WIDTH;
	size.height     = reg1.bit.HEIGHT;

	reg2.reg = CNN_GETDATA(ofs1, base_addr);
	size.channel    = reg2.bit.CHANNEL;
	size.batch      = reg2.bit.BATCH_NUM;

	return size;
}


/**
CNN Set Output Scale Shift

CNN set cnn_set_out_scale.

@param[in] p_param out_scale shift.

@return ER Error .
*/
ER cnn_set_out_scale(BOOL cnn_id, CNN_OUT_SCALE_PARM *p_param)
{

	T_SCALESHIFT_REGISTER1 local_reg0;
	UINT32 ofs0 = SCALESHIFT_REGISTER1_OFS;
	T_SCALESHIFT_REGISTER2 local_reg1;
	UINT32 ofs1 = SCALESHIFT_REGISTER2_OFS;
	T_SCALESHIFT_REGISTER3 local_reg2;
	UINT32 ofs2 = SCALESHIFT_REGISTER3_OFS;
	T_SCALESHIFT_REGISTER4 local_reg3;
	UINT32 ofs3 = SCALESHIFT_REGISTER4_OFS;
	T_SCALESHIFT_REGISTER5 local_reg4;
	UINT32 ofs4 = SCALESHIFT_REGISTER5_OFS;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);

	if (p_param->conv_scale > CNN_SCALEOUT_MAX) {
		DBG_ERR("CNN: conv_scale must be <= %d : %d -> %d\r\n", CNN_SCALEOUT_MAX, (int)p_param->conv_scale, CNN_SCALEOUT_MAX);
		return E_PAR;
	}
	if (p_param->conv_shf > CNN_SHFOUT_MAX) {
		DBG_ERR("CNN: conv_shf must be <= %d : %d -> %d\r\n", CNN_SHFOUT_MAX, (int)p_param->conv_shf, CNN_SHFOUT_MAX);
		return E_PAR;
	}

	if (p_param->elt_scale > CNN_SCALEOUT_MAX) {
		DBG_ERR("CNN: elt_scale must be <= %d : %d -> %d\r\n", CNN_SCALEOUT_MAX, (int)p_param->elt_scale, CNN_SCALEOUT_MAX);
		return E_PAR;
	}
	if (p_param->elt_shf > CNN_SHFOUT_MAX) {
		DBG_ERR("CNN: elt_shf must be <= %d : %d -> %d\r\n", CNN_SHFOUT_MAX, (int)p_param->elt_shf, CNN_SHFOUT_MAX);
		return E_PAR;
	}

	if (p_param->out0_scale > CNN_SCALEOUT_MAX) {
		DBG_ERR("CNN: out0_scale must be <= %d : %d -> %d\r\n", CNN_SCALEOUT_MAX, (int)p_param->out0_scale, CNN_SCALEOUT_MAX);
		return E_PAR;
	}
	if (p_param->out0_shf > CNN_SHFOUT_MAX) {
		DBG_ERR("CNN: out0_shf must be <= %d : %d -> %d\r\n", CNN_SHFOUT_MAX, (int)p_param->out0_shf, CNN_SHFOUT_MAX);
		return E_PAR;
	}

	if (p_param->out1_scale > CNN_SCALEOUT_MAX) {
		DBG_ERR("CNN: out1_scale must be <= %d : %d -> %d\r\n", CNN_SCALEOUT_MAX, (int)p_param->out1_scale, CNN_SCALEOUT_MAX);
		return E_PAR;
	}
	if (p_param->out1_shf > CNN_SHFOUT_MAX) {
		DBG_ERR("CNN: out1_shf must be <= %d : %d -> %d\r\n", CNN_SHFOUT_MAX, (int)p_param->out1_shf, CNN_SHFOUT_MAX);
		return E_PAR;
	}

	if (p_param->pool_scale > CNN_SCALEOUT_MAX) {
		DBG_ERR("CNN: pool_scale must be <= %d : %d -> %d\r\n", CNN_SCALEOUT_MAX, (int)p_param->pool_scale, CNN_SCALEOUT_MAX);
		return E_PAR;
	}
	if (p_param->pool_shf > CNN_SHFOUT_MAX) {
		DBG_ERR("CNN: pool_shf must be <= %d : %d -> %d\r\n", CNN_SHFOUT_MAX, (int)p_param->pool_shf, CNN_SHFOUT_MAX);
		return E_PAR;
	}

	local_reg0.reg = CNN_GETDATA(ofs0, base_addr);
	local_reg0.bit.CONV_OUT_SHIFT_DIR = p_param->conv_shf_dir;
	local_reg0.bit.CONV_OUT_SHIFT = p_param->conv_shf;
	local_reg0.bit.CONV_OUT_SCALE = p_param->conv_scale;
	CNN_SETDATA(ofs0, local_reg0.reg, base_addr);

	local_reg1.reg = CNN_GETDATA(ofs1, base_addr);
	local_reg1.bit.ELTWISE_OUT_SHIFT_DIR = p_param->elt_shf_dir;
	local_reg1.bit.ELTWISE_OUT_SHIFT = p_param->elt_shf;
	local_reg1.bit.ELTWISE_OUT_SCALE = p_param->elt_scale;
	CNN_SETDATA(ofs1, local_reg1.reg, base_addr);

	local_reg2.reg = CNN_GETDATA(ofs2, base_addr);
	local_reg2.bit.OUT0_SHIFT_DIR = p_param->out0_shf_dir;
	local_reg2.bit.OUT0_SHIFT = p_param->out0_shf;
	local_reg2.bit.OUT0_SCALE = p_param->out0_scale;
	CNN_SETDATA(ofs2, local_reg2.reg, base_addr);

	local_reg3.reg = CNN_GETDATA(ofs3, base_addr);
	local_reg3.bit.ACT_OUT1_SHIFT_DIR = p_param->out1_shf_dir;
	local_reg3.bit.ACT_OUT1_SHIFT = p_param->out1_shf;
	local_reg3.bit.ACT_OUT1_SCALE = p_param->out1_scale;
	CNN_SETDATA(ofs3, local_reg3.reg, base_addr);

	local_reg4.reg = CNN_GETDATA(ofs4, base_addr);
	local_reg4.bit.POOL_OUT_SHIFT_DIR = p_param->pool_shf_dir;
	local_reg4.bit.POOL_OUT_SHIFT = p_param->pool_shf;
	local_reg4.bit.POOL_OUT_SCALE = p_param->pool_scale;
	CNN_SETDATA(ofs4, local_reg4.reg, base_addr);

	return E_OK;
}

/**
CNN Get Output Scale Shift

Get current out_scale shift.

@return CNN_OUT_SCALE_PARM out_scale shift.
*/
CNN_OUT_SCALE_PARM cnn_get_out_scale(BOOL cnn_id)
{
	T_SCALESHIFT_REGISTER1 local_reg0;
	UINT32 ofs0 = SCALESHIFT_REGISTER1_OFS;
	T_SCALESHIFT_REGISTER2 local_reg1;
	UINT32 ofs1 = SCALESHIFT_REGISTER2_OFS;
	T_SCALESHIFT_REGISTER3 local_reg2;
	UINT32 ofs2 = SCALESHIFT_REGISTER3_OFS;
	T_SCALESHIFT_REGISTER4 local_reg3;
	UINT32 ofs3 = SCALESHIFT_REGISTER4_OFS;
	T_SCALESHIFT_REGISTER5 local_reg4;
	UINT32 ofs4 = SCALESHIFT_REGISTER5_OFS;
	CNN_OUT_SCALE_PARM out_scale_parm;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);



	local_reg0.reg = CNN_GETDATA(ofs0, base_addr);
	out_scale_parm.conv_shf_dir = local_reg0.bit.CONV_OUT_SHIFT_DIR;
	out_scale_parm.conv_shf = local_reg0.bit.CONV_OUT_SHIFT;
	out_scale_parm.conv_scale = local_reg0.bit.CONV_OUT_SCALE;

	local_reg1.reg = CNN_GETDATA(ofs1, base_addr);
	out_scale_parm.elt_shf_dir = local_reg1.bit.ELTWISE_OUT_SHIFT_DIR;
	out_scale_parm.elt_shf = local_reg1.bit.ELTWISE_OUT_SHIFT;
	out_scale_parm.elt_scale = local_reg1.bit.ELTWISE_OUT_SCALE;

	local_reg2.reg = CNN_GETDATA(ofs2, base_addr);
	out_scale_parm.out0_shf_dir = local_reg2.bit.OUT0_SHIFT_DIR;
	out_scale_parm.out0_shf = local_reg2.bit.OUT0_SHIFT;
	out_scale_parm.out0_scale = local_reg2.bit.OUT0_SCALE;

	local_reg3.reg = CNN_GETDATA(ofs3, base_addr);
	out_scale_parm.out1_shf_dir = local_reg3.bit.ACT_OUT1_SHIFT_DIR;
	out_scale_parm.out1_shf = local_reg3.bit.ACT_OUT1_SHIFT;
	out_scale_parm.out1_scale = local_reg3.bit.ACT_OUT1_SCALE;

	local_reg4.reg = CNN_GETDATA(ofs4, base_addr);
	out_scale_parm.pool_shf_dir = local_reg4.bit.POOL_OUT_SHIFT_DIR;
	out_scale_parm.pool_shf = local_reg4.bit.POOL_OUT_SHIFT;
	out_scale_parm.pool_scale = local_reg4.bit.POOL_OUT_SCALE;

	return out_scale_parm;
}
/**
CNN Set Conv Parameteres

set CNN convolution kernel parameters.

@param[in] p_parm cnn kernel parameters.

@return ER Error cnn kernel parameters are out of range.
*/
ER cnn_set_convparm(BOOL cnn_id, CNN_CONVKERL_PARM *p_parm)
{
	T_CONVOLUTION_REGISTER0 reg1;
	T_CONVOLUTION_REGISTER1 reg2;

	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = CONVOLUTION_REGISTER0_OFS;
	UINT32 ofs1 = CONVOLUTION_REGISTER1_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	reg2.reg = CNN_GETDATA(ofs1, base_addr);

	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN)) {
		//11X11
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_11_11)
			&& ((cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) != CNN_IN_ISIMAGE_EN)) {
				DBG_ERR("CNN: IN_ISIMG needs to be 1 for conv kernel 11x11.\r\n");
				return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_11_11) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1) && (p_parm->conv_stride != 2)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 or 2 for conv kernel 11x11.\r\n");
			return E_PAR;
		}

		//7X7
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_7_7)
			&& ((cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) != CNN_IN_ISIMAGE_EN)) {
				DBG_ERR("CNN: IN_ISIMG needs to be 1 for conv kernel 7X7.\r\n");
				return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_7_7) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1) && (p_parm->conv_stride != 2)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 or 2 for conv kernel 7x7.\r\n");
			return E_PAR;
		}

		//5x5
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_5_5) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 5x5.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_5_5) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 5x5.\r\n");
			return E_PAR;
		}

		//3x3

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_3_3) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 3x3.\r\n");
			return E_PAR;
		}

		//1x1
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_1) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 1x1.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_1) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 1x1.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_1) && ((p_parm->is_top_pad + p_parm->is_bot_pad + p_parm->is_left_pad + p_parm->is_right_pad) > 0)) {
			DBG_ERR("CNN: padding for conv kernel 1X1 must be 0.\r\n");
			return E_PAR;
		}

		//7x1
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_7_1) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 7x1.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_7_1) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 7x1.\r\n");
			return E_PAR;
		}
		//1x7
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_7) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 1x7.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_7) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 1x7.\r\n");
			return E_PAR;
		}

		//5x1
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_5_1) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 5x1.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_5_1) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 5x1.\r\n");
			return E_PAR;
		}

		//1x5
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_5) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 1x5.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_5) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 1x5.\r\n");
			return E_PAR;
		}

		//3x1
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_3_1) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 3x1.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_3_1) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 3x1.\r\n");
			return E_PAR;
		}

		//1x3
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_3) && (cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN)) {
			DBG_ERR("CNN: IN_ISIMG needs to be 0 for conv kernel 1x3.\r\n");
			return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_1_3) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 for conv kernel 1x3.\r\n");
			return E_PAR;
		}

		//9x9
		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_9_9)
			&& ((cnn_get_io_enable(cnn_id) & CNN_IN_ISIMAGE_EN) != CNN_IN_ISIMAGE_EN)) {
				DBG_ERR("CNN: IN_ISIMG needs to be 1 for conv kernel 9X9.\r\n");
				return E_PAR;
		}

		if ((p_parm->conv_kersize == CNN_CONV_KERSZ_9_9) && (p_parm->conv_stride != 0) && (p_parm->conv_stride != 1) && (p_parm->conv_stride != 2)) {
			DBG_ERR("CNN: CONV_STRIDE can only be 0 or 1 or 2 for conv kernel 9x9.\r\n");
			return E_PAR;
		}


		if (p_parm->conv_kersize > CNN_CONV_SIZE_MAX) {
			DBG_ERR("CNN: CONV_KERSEL cannot be over %d.\r\n", CNN_CONV_SIZE_MAX);
			return E_PAR;
		}

		if (p_parm->conv_setnum > CNN_CONV_SETNUM_MAX) {
			DBG_ERR("CNN: CONVSETNUM must be <= %d. %d -> %d\r\n", CNN_CONV_SETNUM_MAX, (int)p_parm->conv_setnum, CNN_CONV_SETNUM_MAX);
			return E_PAR;
		} else if (p_parm->conv_setnum < CNN_CONV_SETNUM_MIN) {
			DBG_ERR("CNN: CONVSETNUM must be >= %d. %d -> %d\r\n", CNN_CONV_SETNUM_MIN, (int)p_parm->conv_setnum, CNN_CONV_SETNUM_MIN);
			return E_PAR;
		}

		if (p_parm->conv_shf_bias > CNN_CONV_SHIFTB_MAX) {
			DBG_ERR("CNN: CONV_SHIFTB must be <= %d. %d -> %d\r\n", CNN_CONV_SHIFTB_MAX, (int)p_parm->conv_shf_bias, CNN_CONV_SHIFTB_MAX);
			return E_PAR;
		}
		if (p_parm->conv_shf_acc > CNN_CONV_SHIFTACC_MAX) {
			DBG_ERR("CNN: CONVSHIFTACC must be <= %d. %d -> %d\r\n", CNN_CONV_SHIFTACC_MAX, (int)p_parm->conv_shf_acc, CNN_CONV_SHIFTACC_MAX);
			return E_PAR;
		}
	}

	switch (p_parm->conv_kersize) {
	case CNN_CONV_KERSZ_11_11:
		reg1.bit.CONV_KERSEL = 0;
		break;
	case CNN_CONV_KERSZ_7_7:
		reg1.bit.CONV_KERSEL = 1;
		break;
	case CNN_CONV_KERSZ_5_5:
		reg1.bit.CONV_KERSEL = 2;
		break;
	case CNN_CONV_KERSZ_3_3:
		reg1.bit.CONV_KERSEL = 3;
		break;
	case CNN_CONV_KERSZ_1_1:
		reg1.bit.CONV_KERSEL = 4;
		break;
	case CNN_CONV_KERSZ_7_1:
		reg1.bit.CONV_KERSEL = 5;
		break;
	case CNN_CONV_KERSZ_1_7:
		reg1.bit.CONV_KERSEL = 6;
		break;
	case CNN_CONV_KERSZ_5_1:
		reg1.bit.CONV_KERSEL = 7;
		break;
	case CNN_CONV_KERSZ_1_5:
		reg1.bit.CONV_KERSEL = 8;
		break;
	case CNN_CONV_KERSZ_3_1:
		reg1.bit.CONV_KERSEL = 9;
		break;
	case CNN_CONV_KERSZ_1_3:
		reg1.bit.CONV_KERSEL = 10;
		break;
	case CNN_CONV_KERSZ_9_9:
		reg1.bit.CONV_KERSEL = 11;
		break;
	default:
		DBG_ERR("In convolution, unknown kersel: %d\r\n", (int)p_parm->conv_kersize);
		return E_PAR;
	}

	switch (p_parm->conv_stride) {
	case CNN_CONV_KER_STRIDE_1:
		reg1.bit.CONV_STRIDE = 0;
		break;
	case CNN_CONV_KER_STRIDE_2:
		reg1.bit.CONV_STRIDE = 1;
		break;
	case CNN_CONV_KER_STRIDE_4:
		reg1.bit.CONV_STRIDE = 2;
		break;
	default:
		DBG_ERR("In convolution, unknown stride: %d\r\n", (int)p_parm->conv_stride);
		return E_PAR;
	}

	reg1.bit.CONV_SETNUM        = p_parm->conv_setnum;
	reg1.bit.CONV_SHIFT_ACC     = p_parm->conv_shf_acc;

	reg2.bit.CONV_TOP_PAD       = p_parm->is_top_pad;
	reg2.bit.CONV_BOTTOM_PAD    = p_parm->is_bot_pad;
	reg2.bit.CONV_LEFT_PAD      = p_parm->is_left_pad;
	reg2.bit.CONV_RIGHT_PAD     = p_parm->is_right_pad;
	reg2.bit.CONV_SHIFT_B       = p_parm->conv_shf_bias;

	CNN_SETDATA(ofs0, reg1.reg, base_addr);
	CNN_SETDATA(ofs1, reg2.reg, base_addr);
	return E_OK;
}

/**
CNN Get Conv Parameters

get CNN convolution kernel parameters.

@return CNN convolution kernel parameters.
*/
CNN_CONVKERL_PARM cnn_get_convparm(BOOL cnn_id)
{
	CNN_CONVKERL_PARM parm = { 0 };
	T_CONVOLUTION_REGISTER0 reg1;
	T_CONVOLUTION_REGISTER1 reg2;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = CONVOLUTION_REGISTER0_OFS;
	UINT32 ofs1 = CONVOLUTION_REGISTER1_OFS;

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	reg2.reg = CNN_GETDATA(ofs1, base_addr);

	parm.conv_kersize        = reg1.bit.CONV_KERSEL;
	parm.conv_stride         = reg1.bit.CONV_STRIDE;
	parm.conv_setnum         = reg1.bit.CONV_SETNUM;
	parm.conv_shf_acc        = reg1.bit.CONV_SHIFT_ACC;

	parm.is_top_pad          = reg2.bit.CONV_TOP_PAD;
	parm.is_bot_pad          = reg2.bit.CONV_BOTTOM_PAD;
	parm.is_left_pad         = reg2.bit.CONV_LEFT_PAD;
	parm.is_right_pad        = reg2.bit.CONV_RIGHT_PAD;
	parm.conv_shf_bias       = reg2.bit.CONV_SHIFT_B;
	return parm;
}

/**
CNN set FCD Enable

set CNN FCD function enable.

@param[in] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
ER cnn_set_fcd_en(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	if ((p_parm->fcd_quanti_en == FALSE) && (p_parm->fcd_quanti_kmean_en == TRUE)) {
		DBG_ERR("When QUANTIZATION_EN disable, QUANTIZATION_KMEANS_EN not allow to be enable !\r\n");
		return E_PAR;
	}
	if ((p_parm->fcd_vlc_en == FALSE) && (p_parm->fcd_quanti_kmean_en == TRUE)) {
		DBG_ERR("When VLC_EN disable, QUANTIZATION_KMEANS_EN not allow to be enable !\r\n");
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.FCD_VLC_EN = p_parm->fcd_vlc_en;
	reg1.bit.FCD_QUANTIZATION_EN = p_parm->fcd_quanti_en;
	reg1.bit.FCD_SPARSE_EN = p_parm->fcd_sparse_en;
	reg1.bit.FCD_QUANTIZATION_KMEANS = p_parm->fcd_quanti_kmean_en;

	CNN_SETDATA(ofs, reg1.reg, base_addr);

	return E_OK;
}

/**
CNN Get FCD Enable

get CNN FCD function enable.

@param[out] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
ER cnn_get_fcd_en(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	T_CNN_MODE_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_MODE_REGISTER_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	p_parm->fcd_vlc_en          = reg1.bit.FCD_VLC_EN;
	p_parm->fcd_quanti_en       = reg1.bit.FCD_QUANTIZATION_EN;
	p_parm->fcd_sparse_en       = reg1.bit.FCD_SPARSE_EN;
	p_parm->fcd_quanti_kmean_en = reg1.bit.FCD_QUANTIZATION_KMEANS;
	return E_OK;
}

/**
CNN Set FCD Encode Bit Length

set CNN FCD encode bit length.

@param[in] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
static ER cnn_set_fcd_encbitlength(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	T_COMPRESSION_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = COMPRESSION_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	if (p_parm->fcd_enc_bit_length > CNN_FCD_ENC_BIT_LENGTH_MAX) {
		DBG_ERR("fcd encode bit length is out of range : %d !\r\n", (int)p_parm->fcd_enc_bit_length);
		return E_PAR;
	}
	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.FCD_ENC_BIT_LENGTH = p_parm->fcd_enc_bit_length;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get FCD Encode Bit Length

get CNN FCD encode bit length.

@param[out] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
static ER cnn_get_fcd_encbitlength(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	T_COMPRESSION_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = COMPRESSION_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	p_parm->fcd_enc_bit_length = reg1.bit.FCD_ENC_BIT_LENGTH;
	return E_OK;
}

/**
CNN Set FCD Canonical VLC Table

CNN set FCD canonical VLC table (code).

@param[in] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
static ER cnn_set_fcd_vlc_table_code(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER1 reg[CNN_FCD_VLC_TBL_SIZE];
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs[CNN_FCD_VLC_TBL_SIZE];
	UINT32 ofs_gap = COMPRESSION_REGISTER2_OFS - COMPRESSION_REGISTER1_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER1_OFS;
	for (i = 1; i < CNN_FCD_VLC_TBL_SIZE; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}

	for (i = 0; i < CNN_FCD_VLC_TBL_SIZE; i++) {
		if (p_parm->fcd_vlc_code[i] > CNN_FCD_VLC_CODE_MAX) {
			DBG_ERR("fcd_vlc_code[%d] must be < %d : %d -> %d\r\n", (int)i, CNN_FCD_VLC_CODE_MAX, (int)p_parm->fcd_vlc_code[i], CNN_FCD_VLC_CODE_MAX);
			return E_PAR;
		}
		if (p_parm->fcd_vlc_valid[i] > CNN_FCD_VLC_VALID_MAX) {
			DBG_ERR("fcd_vlc_valid[%d] must be < %d : %d -> %d\r\n", (int)i, CNN_FCD_VLC_VALID_MAX, (int)p_parm->fcd_vlc_valid[i], CNN_FCD_VLC_VALID_MAX);
			return E_PAR;
		}
	}

	for (i = 0; i < CNN_FCD_VLC_TBL_SIZE; i++) {
		reg[i].reg = CNN_GETDATA(ofs[i], base_addr);
		reg[i].bit.FCD_VLC_CODE0 = p_parm->fcd_vlc_code[i];
		reg[i].bit.FCD_VLC_VALID0 = p_parm->fcd_vlc_valid[i];
		CNN_SETDATA(ofs[i], reg[i].reg, base_addr);
	}

	return E_OK;
}


/**
CNN Get FCD Canonical VLC Table

CNN get FCD canonical VLC table (code).

@param[out] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
static ER cnn_get_fcd_vlc_table_code(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER1 reg[CNN_FCD_VLC_TBL_SIZE];
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs[CNN_FCD_VLC_TBL_SIZE];
	UINT32 ofs_gap = COMPRESSION_REGISTER2_OFS - COMPRESSION_REGISTER1_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER1_OFS;
	for (i = 1; i < CNN_FCD_VLC_TBL_SIZE; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}
	for (i = 0; i < CNN_FCD_VLC_TBL_SIZE; i++) {
		reg[i].reg = CNN_GETDATA(ofs[i], base_addr);
		p_parm->fcd_vlc_code[i]     = reg[i].bit.FCD_VLC_CODE0;
		p_parm->fcd_vlc_valid[i]    = reg[i].bit.FCD_VLC_VALID0;
	}
	return E_OK;
}

/**
CNN Set FCD Canonical VLC Table

CNN set FCD canonical VLC table (offset).

@param[in] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
static ER cnn_set_fcd_vlc_table_ofs(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER24 reg[CNN_FCD_VLC_REG_CNT];
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs_gap = COMPRESSION_REGISTER25_OFS - COMPRESSION_REGISTER24_OFS;
	UINT32 ofs[CNN_FCD_VLC_REG_CNT];

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER24_OFS;
	for (i = 1; i < CNN_FCD_VLC_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}

	for (i = 0; i < CNN_FCD_VLC_TBL_SIZE; i++) {
		if (p_parm->fcd_vlc_ofs[i] > CNN_FCD_VLC_OFFSET_MAX) {
			DBG_ERR("fcd_vlc_ofs[%d] must be < %d : %d -> %d\r\n", (int)i, CNN_FCD_VLC_OFFSET_MAX, (int)p_parm->fcd_vlc_ofs[i], CNN_FCD_VLC_OFFSET_MAX);
			return E_PAR;
		}
	}

	for (i = 0; i < CNN_FCD_VLC_REG_CNT; i++) {
		reg[i].reg = CNN_GETDATA(ofs[i], base_addr);
		reg[i].bit.FCD_VLC_INDEX0 = p_parm->fcd_vlc_ofs[2 * i];
		if (i < ((CNN_FCD_VLC_TBL_SIZE - 1) >> 1)) {
			reg[i].bit.FCD_VLC_INDEX1 = p_parm->fcd_vlc_ofs[2 * i + 1];
		}
		CNN_SETDATA(ofs[i], reg[i].reg, base_addr);
	}

	return E_OK;
}

/**
CNN Get FCD Canonical VLC Table

CNN get FCD canonical VLC table (offset).

@param[out] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
static ER cnn_get_fcd_vlc_table_ofs(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER24 reg[CNN_FCD_VLC_REG_CNT];
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs[CNN_FCD_VLC_REG_CNT];
	UINT32 ofs_gap = COMPRESSION_REGISTER25_OFS - COMPRESSION_REGISTER24_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER24_OFS;
	for (i = 1; i < CNN_FCD_VLC_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}

	for (i = 0; i < CNN_FCD_VLC_REG_CNT; i++) {
		reg[i].reg = CNN_GETDATA(ofs[i], base_addr);
		p_parm->fcd_vlc_ofs[2 * i]     = reg[i].bit.FCD_VLC_INDEX0;
		if (i < ((CNN_FCD_VLC_TBL_SIZE - 1) >> 1)) {
			p_parm->fcd_vlc_ofs[2 * i + 1]   = reg[i].bit.FCD_VLC_INDEX1;
		}
	}
	return E_OK;
}

////
/**
CNN Set FCD Parameters

cnn set fcd parameters.

@param[in] p_parm cnn fcd parameters.

@return ER Error cnn fcd parameters are out of range.
*/
ER cnn_set_fcd_parm(BOOL cnn_id, CNN_FCD_PARM *p_parm)
{
	ER er_code = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	er_code = cnn_set_fcd_en(cnn_id, p_parm);
	if (er_code != E_OK) {
		return er_code;
	}
	er_code = cnn_set_fcd_encbitlength(cnn_id, p_parm);
	if (er_code != E_OK) {
		return er_code;
	}
	if ((cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) && (cnn_get_kerl_en(cnn_id) & CNN_FCD_VLC_EN)) {
		er_code = cnn_set_fcd_vlc_table_code(cnn_id, p_parm);
		if (er_code != E_OK) {
			return er_code;
		}
		er_code = cnn_set_fcd_vlc_table_ofs(cnn_id, p_parm);
		if (er_code != E_OK) {
			return er_code;
		}
	}
	return er_code;
}

/**
CNN Get FCD Parameters

cnn get fcd parameters.

@return p_parm cnn fcd parameters.
*/
CNN_FCD_PARM cnn_get_fcd_parm(BOOL cnn_id)
{
	CNN_FCD_PARM parm = {0};
	ER er_code = E_OK;

	er_code = cnn_get_fcd_en(cnn_id, &parm);
	if (er_code != E_OK) {
		DBG_ERR("cnn_get_fcd_en fail...\r\n");
	}

	er_code = cnn_get_fcd_encbitlength(cnn_id, &parm);
	if (er_code != E_OK) {
		DBG_ERR("cnn_get_fcd_encbitlength fail...\r\n");
	}
	if ((cnn_get_engmode(cnn_id) == CNN_CONV) && (cnn_get_kerl_en(cnn_id) & CNN_CONV_KERL_EN) && (cnn_get_kerl_en(cnn_id) & CNN_FCD_VLC_EN)) {
		er_code = cnn_get_fcd_vlc_table_code(cnn_id, &parm);
		if (er_code != E_OK) {
			DBG_ERR("cnn_get_fcd_vlc_table_code fail...\r\n");
		}
		er_code = cnn_get_fcd_vlc_table_ofs(cnn_id, &parm);
		if (er_code != E_OK) {
			DBG_ERR("cnn_get_fcd_vlc_table_ofs fail...\r\n");
		}
	}

	return parm;
}

/**
CNN Set BnScale Parameteres

cnn set bnscale parameters.

@param[in] p_parm bnscale parameters.

@return ER Error cnn bnscale parameters are out of range.
*/
ER cnn_set_bnscale_parm(BOOL cnn_id, CNN_BNSCALE_PARM *p_parm)
{
	T_BNSCALE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = BNSCALE_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	if (p_parm->bn_shf_mean > CNN_BN_SHIFTM_MAX) {
		DBG_ERR("CNN: CNN_BATCHNORM_SHIFT_M must be [0, %d].\r\n", CNN_BN_SHIFTM_MAX);
		return E_PAR;
	}
	if (p_parm->scale_shf_bias > CNN_SCALE_SHIFTB_MAX) {
		DBG_ERR("CNN: CNN_SCALE_SHIFT_B must be [0, %d].\r\n", CNN_SCALE_SHIFTB_MAX);
		return E_PAR;
	}
	if (p_parm->scale_shf_alpha > SCALE_SHIFT_ALPHA_MAX) {
		DBG_ERR("CNN: SCALE_SHIFT_ALPHA must be [0, %d].\r\n", SCALE_SHIFT_ALPHA_MAX);
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs0, base_addr);


	reg1.bit.BATCHNORM_SHIFT_M  = p_parm->bn_shf_mean;
	reg1.bit.SCALE_SHIFT_B      = p_parm->scale_shf_bias;
	reg1.bit.SCALE_SHIFT_ALPHA  = p_parm->scale_shf_alpha;

	CNN_SETDATA(ofs0, reg1.reg, base_addr);

	return E_OK;
}

/**
CNN Get BnScale Parameters

cnn get bnscale parameters.

@return CNN bn scale parameters.
*/
CNN_BNSCALE_PARM cnn_get_bnscale_parm(BOOL cnn_id)
{
	CNN_BNSCALE_PARM parm;
	T_BNSCALE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = BNSCALE_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	parm.bn_shf_mean		   = reg1.bit.BATCHNORM_SHIFT_M;
	parm.scale_shf_bias        = reg1.bit.SCALE_SHIFT_B;
	parm.scale_shf_alpha       = reg1.bit.SCALE_SHIFT_ALPHA;

	return parm;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
CNN Set Eltwise Param

CNN set eltwise coef and shift.

@param[in] eltwise coef and shift.

@return None.
*/
ER cnn_set_elt_parm(BOOL cnn_id, CNN_ELTWISE_PARM *p_parm)
{
	T_SCALESHIFT_REGISTER0 reg0;
	T_ELTWISE_REGISTER0 reg1;
	T_ELTWISE_REGISTER1 reg2;

	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = SCALESHIFT_REGISTER0_OFS;
	UINT32 ofs1 = ELTWISE_REGISTER0_OFS;
	UINT32 ofs2 = ELTWISE_REGISTER1_OFS;
	INT32 elt_coef0 = 0, elt_coef1 = 0;

	//elt_in_scale_shift
	if (p_parm->elt_in_scale > CNN_ELT_IN_SCALE_MAX) {
		DBG_ERR("In Eltwise, in_scale %d is clamp %d!\r\n", (int)p_parm->elt_in_scale, CNN_ELT_IN_SCALE_MAX);
		return E_PAR;
	} 
	if (p_parm->elt_in_shift > CNN_ELT_IN_SHF_MAX) {
		DBG_ERR("In Eltwise, in_shift %d is clamp %d!\r\n", (int)p_parm->elt_in_shift, CNN_ELT_IN_SHF_MAX);
		return E_PAR;
	} 

	//elt_shift
	if (p_parm->elt_shf0 > CNN_ELT_SHF0_MAX) {
		DBG_ERR("In Eltwise, shift 0 %d is clamp %d!\r\n", (int)p_parm->elt_shf0, CNN_ELT_SHF0_MAX);
		return E_PAR;
	} 
	if (p_parm->elt_shf1 > CNN_ELT_SHF1_MAX) {
		DBG_ERR("In Eltwise, shift 1 %d is clamp %d!\r\n", (int)p_parm->elt_shf1, CNN_ELT_SHF1_MAX);
		return E_PAR;
	} 
	if (p_parm->elt_outshf > CNN_ELT_OUTSHF_MAX) {
		DBG_ERR("In Eltwise, outshf %d is clamp %d!\r\n", (int)p_parm->elt_outshf, CNN_ELT_OUTSHF_MAX);
		return E_PAR;
	} 

	//elt_coef
	if (p_parm->elt_coef0 > CNN_ELT_COEF0_MAX)
		elt_coef0 = -(p_parm->elt_coef0-CNN_ELT_COEF0_MAX);
	else
		elt_coef0 = p_parm->elt_coef0;

	if (p_parm->elt_coef1 > CNN_ELT_COEF1_MAX)
		elt_coef1 = -(p_parm->elt_coef1-CNN_ELT_COEF1_MAX);
	else
		elt_coef1 = p_parm->elt_coef1;

	if (elt_coef0 > CNN_ELT_COEF0_MAX) {
		DBG_ERR("In Eltwise, coeff 0 %d is clamp %d!\r\n", (int)elt_coef0, CNN_ELT_COEF0_MAX);
		return E_PAR;
	} else if (elt_coef0 < CNN_ELT_COEF0_MIN) {
		DBG_ERR("In Eltwise, coeff 0 %d is clamp %d!\r\n", (int)elt_coef0, CNN_ELT_COEF0_MIN);
		return E_PAR;
	}
	if (elt_coef1 > CNN_ELT_COEF1_MAX) {
		DBG_ERR("In Eltwise, coeff 1 %d is clamp %d!\r\n", (int)p_parm->elt_coef1, CNN_ELT_COEF1_MAX);
		return E_PAR;
	} else if (elt_coef1 < CNN_ELT_COEF1_MIN) {
		DBG_ERR("In Eltwise, coeff 1 %d is clamp %d!\r\n", (int)p_parm->elt_coef1, CNN_ELT_COEF1_MIN);
		return E_PAR;
	}


	reg0.reg = CNN_GETDATA(ofs1, base_addr);
	reg1.reg = CNN_GETDATA(ofs1, base_addr);
	reg2.reg = CNN_GETDATA(ofs2, base_addr);

	reg0.bit.IN_SHIFT_DIR = p_parm->elt_in_shift_dir;
	reg0.bit.IN_SHIFT = p_parm->elt_in_shift;
	reg0.bit.IN_SCALE = p_parm->elt_in_scale;

	reg1.bit.ELTWISE_SHIFT0 = p_parm->elt_shf0;
	reg1.bit.ELTWISE_SHIFT1 = p_parm->elt_shf1;
	reg1.bit.ELTWISE_OUTSHF = p_parm->elt_outshf;

	reg2.bit.ELTWISE_COEFF0 = p_parm->elt_coef0;
	reg2.bit.ELTWISE_COEFF1 = p_parm->elt_coef1;

	CNN_SETDATA(ofs0, reg0.reg, base_addr);
	CNN_SETDATA(ofs1, reg1.reg, base_addr);
	CNN_SETDATA(ofs2, reg2.reg, base_addr);

	return E_OK;
}

/**
CNN Get Eltwise Param

CNN get eltwise coef and shift.

@return CNN_POOL_KER pooling shift.
*/
CNN_ELTWISE_PARM cnn_get_elt_parm(BOOL cnn_id)
{
	CNN_ELTWISE_PARM p_parm = { 0 };

	T_SCALESHIFT_REGISTER0 reg0;
	T_ELTWISE_REGISTER0 reg1;
	T_ELTWISE_REGISTER1 reg2;

	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = SCALESHIFT_REGISTER0_OFS;
	UINT32 ofs1 = ELTWISE_REGISTER0_OFS;
	UINT32 ofs2 = ELTWISE_REGISTER1_OFS;

	reg0.reg = CNN_GETDATA(ofs0, base_addr);
	reg1.reg = CNN_GETDATA(ofs1, base_addr);
	reg2.reg = CNN_GETDATA(ofs2, base_addr);

	p_parm.elt_in_shift_dir = reg0.bit.IN_SHIFT_DIR;
	p_parm.elt_in_shift = reg0.bit.IN_SHIFT;
	p_parm.elt_in_scale = reg0.bit.IN_SCALE;

	p_parm.elt_shf0 = reg1.bit.ELTWISE_SHIFT0;
	p_parm.elt_shf1 = reg1.bit.ELTWISE_SHIFT1;
	p_parm.elt_outshf = reg1.bit.ELTWISE_OUTSHF;

	p_parm.elt_coef0 = reg2.bit.ELTWISE_COEFF0;
	p_parm.elt_coef1 = reg2.bit.ELTWISE_COEFF1;

	return p_parm;
}

/**
CNN Set PreRelu Parameteres

cnn set prerelu parameters.

@param[in] p_parm prerelu parameters.

@return ER Error cnn prerelu parameters are out of range.
*/
ER cnn_set_prerelu_parm(BOOL cnn_id, CNN_RELU_IN *p_parm)
{
	T_ACTIVATION_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = ACTIVATION_REGISTER0_OFS;
	INT32 leaky_val = 0;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	leaky_val = p_parm->leaky_val;
	if(leaky_val > CNN_RELU_LEAKY_VAL_MAX) {
		leaky_val = (leaky_val - CNN_RELU_LEAKY_VAL_MAX)*(-1);
	}

	if (leaky_val > CNN_RELU_LEAKY_VAL_MAX) {
		DBG_ERR("In ReLU, leaky value %d is clamp %d!\r\n", (int)leaky_val, CNN_RELU_LEAKY_VAL_MAX);
		return E_PAR;
	} else if (leaky_val < CNN_RELU_LEAKY_VAL_MIN) {
		DBG_ERR("In ReLU, leaky value %d is clamp %d!\r\n", (int)leaky_val, CNN_RELU_LEAKY_VAL_MIN);
		return E_PAR;
	}
	if (p_parm->leaky_shf > CNN_RELU_LEAKY_SHF_MAX) {
		DBG_ERR("In ReLU, leaky shift %d is clamp %d!\r\n", (int)p_parm->leaky_shf, CNN_RELU_LEAKY_SHF_MAX);
		return E_PAR;
	} else if (p_parm->leaky_shf < CNN_RELU_LEAKY_SHF_MIN) {
		DBG_ERR("In ReLU, leaky shift %d is clamp %d!\r\n", (int)p_parm->leaky_shf, CNN_RELU_LEAKY_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs0, base_addr);

	reg1.bit.PRE_RELU_LEAKY_VAL   = p_parm->leaky_val;
	reg1.bit.PRE_RELU_LEAKY_SHIFT = p_parm->leaky_shf - CNN_RELU_LEAKY_SHF_MIN;
	reg1.bit.PRE_RELU_NEGATION    = p_parm->negation;

	CNN_SETDATA(ofs0, reg1.reg, base_addr);

	return E_OK;

}

/**
CNN Get PreRelu Parameters

cnn get PreRelu parameters.

@return CNN PreRelu parameters.
*/
CNN_RELU_IN cnn_get_prerelu_parm(BOOL cnn_id)
{
	CNN_RELU_IN parm = { 0 };
	T_ACTIVATION_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = ACTIVATION_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	parm.leaky_val		= reg1.bit.PRE_RELU_LEAKY_VAL;
	parm.leaky_shf      = reg1.bit.PRE_RELU_LEAKY_SHIFT + CNN_RELU_LEAKY_SHF_MIN;
	parm.negation       = reg1.bit.PRE_RELU_NEGATION;

	return parm;
}

/**
CNN Set Relu0 Parameteres

cnn set relu0 parameters.

@param[in] p_parm relu0 parameters.

@return ER Error cnn relu0 parameters are out of range.
*/
ER cnn_set_relu0_parm(BOOL cnn_id, CNN_RELU_IN *p_parm)
{
	T_ACTIVATION_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = ACTIVATION_REGISTER1_OFS;
	INT32 leaky_val = 0;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	leaky_val = p_parm->leaky_val;
	if(leaky_val > CNN_RELU_LEAKY_VAL_MAX) {
		leaky_val = (leaky_val - CNN_RELU_LEAKY_VAL_MAX)*(-1);
	}

	if (leaky_val > CNN_RELU_LEAKY_VAL_MAX) {
		DBG_ERR("In ReLU, leaky value %d is clamp %d!\r\n", (int)leaky_val, CNN_RELU_LEAKY_VAL_MAX);
		return E_PAR;
	} else if (leaky_val < CNN_RELU_LEAKY_VAL_MIN) {
		DBG_ERR("In ReLU, leaky value %d is clamp %d!\r\n", (int)leaky_val, CNN_RELU_LEAKY_VAL_MIN);
		return E_PAR;
	}
	if (p_parm->leaky_shf > CNN_RELU_LEAKY_SHF_MAX) {
		DBG_ERR("In ReLU, leaky shift %d is clamp %d!\r\n", (int)p_parm->leaky_shf, CNN_RELU_LEAKY_SHF_MAX);
		return E_PAR;
	} else if (p_parm->leaky_shf < CNN_RELU_LEAKY_SHF_MIN) {
		DBG_ERR("In ReLU, leaky shift %d is clamp %d!\r\n", (int)p_parm->leaky_shf, CNN_RELU_LEAKY_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs0, base_addr);

	reg1.bit.RELU_LEAKY_VAL0   = p_parm->leaky_val;
	reg1.bit.RELU_LEAKY_SHIFT0 = p_parm->leaky_shf - CNN_RELU_LEAKY_SHF_MIN;
	reg1.bit.RELU_NEGATION0    = p_parm->negation;

	CNN_SETDATA(ofs0, reg1.reg, base_addr);

	return E_OK;

}

/**
CNN Get Relu0 Parameters

cnn get relu0 parameters.

@return CNN relu0 parameters.
*/
CNN_RELU_IN cnn_get_relu0_parm(BOOL cnn_id)
{
	CNN_RELU_IN parm = { 0 };
	T_ACTIVATION_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = ACTIVATION_REGISTER1_OFS;

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	parm.leaky_val	    = reg1.bit.RELU_LEAKY_VAL0;
	parm.leaky_shf      = reg1.bit.RELU_LEAKY_SHIFT0 + CNN_RELU_LEAKY_SHF_MIN;
	parm.negation       = reg1.bit.RELU_NEGATION0;

	return parm;
}


/**
CNN Set Relu1 Parameteres

cnn set relu1 parameters.

@param[in] p_parm relu1 parameters.

@return ER Error cnn relu1 parameters are out of range.
*/
ER cnn_set_relu1_parm(BOOL cnn_id, CNN_RELU_IN *p_parm)
{
	T_ACTIVATION_REGISTER2 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = ACTIVATION_REGISTER2_OFS;
	INT32 leaky_val = 0;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	leaky_val = p_parm->leaky_val;
	if(leaky_val > CNN_RELU_LEAKY_VAL_MAX) {
		leaky_val = (leaky_val - CNN_RELU_LEAKY_VAL_MAX)*(-1);
	}

	if (leaky_val > CNN_RELU_LEAKY_VAL_MAX) {
		DBG_ERR("In ReLU, leaky value %d is clamp %d!\r\n", (int)leaky_val, CNN_RELU_LEAKY_VAL_MAX);
		return E_PAR;
	} else if (leaky_val < CNN_RELU_LEAKY_VAL_MIN) {
		DBG_ERR("In ReLU, leaky value %d is clamp %d!\r\n", (int)leaky_val, CNN_RELU_LEAKY_VAL_MIN);
		return E_PAR;
	}
	if (p_parm->leaky_shf > CNN_RELU_LEAKY_SHF_MAX) {
		DBG_ERR("In ReLU, leaky shift %d is clamp %d!\r\n", (int)p_parm->leaky_shf, CNN_RELU_LEAKY_SHF_MAX);
		return E_PAR;
	} else if (p_parm->leaky_shf < CNN_RELU_LEAKY_SHF_MIN) {
		DBG_ERR("In ReLU, leaky shift %d is clamp %d!\r\n", (int)p_parm->leaky_shf, CNN_RELU_LEAKY_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs0, base_addr);

	reg1.bit.RELU_LEAKY_VAL1   = p_parm->leaky_val;
	reg1.bit.RELU_LEAKY_SHIFT1 = p_parm->leaky_shf - CNN_RELU_LEAKY_SHF_MIN;
	reg1.bit.RELU_NEGATION1    = p_parm->negation;

	CNN_SETDATA(ofs0, reg1.reg, base_addr);

	return E_OK;

}

/**
CNN Get Relu1 Parameters

cnn get relu1 parameters.

@return CNN relu1 parameters.
*/
CNN_RELU_IN cnn_get_relu1_parm(BOOL cnn_id)
{
	CNN_RELU_IN parm = { 0 };
	T_ACTIVATION_REGISTER2 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs0 = ACTIVATION_REGISTER2_OFS;

	reg1.reg = CNN_GETDATA(ofs0, base_addr);
	parm.leaky_val		= reg1.bit.RELU_LEAKY_VAL1;
	parm.leaky_shf      = reg1.bit.RELU_LEAKY_SHIFT1 + CNN_RELU_LEAKY_SHF_MIN;
	parm.negation       = reg1.bit.RELU_NEGATION1;

	return parm;
}

/**
CNN Set Deconv Parameteres

CNN set deconvolution parameteres.

@param[in] p_parm deconvolution parameters.

@return ER Error deconvolution parameters are out of range.
*/
ER cnn_set_deconvparm(BOOL cnn_id, CNN_DECONV_PARM *p_parm)
{
	T_DECONV_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = DECONV_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	if (p_parm->is_top_pad > CNN_DECONV_PADNUM_MAX) {
		DBG_ERR("CNN: CNN_DECONV_PADNUM_MAX must not over %d.\r\n", CNN_DECONV_PADNUM_MAX);
		return E_PAR;
	} 
	if (p_parm->is_bot_pad > CNN_DECONV_PADNUM_MAX) {
		DBG_ERR("CNN: CNN_DECONV_PADNUM_MAX must not over %d.\r\n", CNN_DECONV_PADNUM_MAX);
		return E_PAR;
	} 
	if (p_parm->is_left_pad > CNN_DECONV_PADNUM_MAX) {
		DBG_ERR("CNN: CNN_DECONV_PADNUM_MAX must not over %d.\r\n", CNN_DECONV_PADNUM_MAX);
		return E_PAR;
	} 
	if (p_parm->is_right_pad > CNN_DECONV_PADNUM_MAX) {
		DBG_ERR("CNN: CNN_DECONV_PADNUM_MAX must not over %d.\r\n", CNN_DECONV_PADNUM_MAX);
		return E_PAR;
	} 

	if (p_parm->deconv_padval > CNN_DECONV_PADVAL_MAX) {
		DBG_ERR("CNN: DECONV_PAD_VAL must not over %d.\r\n", CNN_DECONV_PADVAL_MAX);
		return E_PAR;
	} else if (p_parm->deconv_padval < CNN_DECONV_PADVAL_MIN) {
		DBG_ERR("CNN: DECONV_PAD_VAL must not less than %d.\r\n", CNN_DECONV_PADVAL_MIN);
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	reg1.bit.DECONV_TOP_PAD     = p_parm->is_top_pad;
	reg1.bit.DECONV_BOTTOM_PAD  = p_parm->is_bot_pad;
	reg1.bit.DECONV_LEFT_PAD    = p_parm->is_left_pad;
	reg1.bit.DECONV_RIGHT_PAD   = p_parm->is_right_pad;
	reg1.bit.DECONV_PAD_VALUE   = p_parm->deconv_padval;

	switch (p_parm->deconv_stride) {
	case CNN_DECONV_KER_STRIDE_1:
		reg1.bit.DECONV_STRIDE = 0;
		break;
	case CNN_DECONV_KER_STRIDE_2:
		reg1.bit.DECONV_STRIDE = 1;
		break;
	case CNN_DECONV_KER_STRIDE_4:
		reg1.bit.DECONV_STRIDE = 2;
		break;
	case CNN_DECONV_KER_STRIDE_8:
		reg1.bit.DECONV_STRIDE = 3;
		break;
	default:
		DBG_ERR("In deconvolution, unknown stride: %d\r\n", p_parm->deconv_stride);
		return E_CTX;
	}
	reg1.bit.DECONV_STRIDE        = p_parm->deconv_stride;

	CNN_SETDATA(ofs, reg1.reg, base_addr);

	return E_OK;
}

/**
CNN Get Deconv Parameters

CNN get deconvolution parameters.

@return CNN_DECONV_PARM deconvolution parameters.
*/
CNN_DECONV_PARM cnn_get_deconvparm(BOOL cnn_id)
{
	CNN_DECONV_PARM parm = { 0 };
	T_DECONV_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = DECONV_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	parm.is_top_pad         = reg1.bit.DECONV_TOP_PAD;
	parm.is_bot_pad         = reg1.bit.DECONV_BOTTOM_PAD;
	parm.is_left_pad        = reg1.bit.DECONV_LEFT_PAD;
	parm.is_right_pad       = reg1.bit.DECONV_RIGHT_PAD;
	parm.deconv_padval      = reg1.bit.DECONV_PAD_VALUE;
	parm.deconv_stride      = reg1.bit.DECONV_STRIDE;
	return parm;
}


/**
CNN Set ScaleUp Parameteres

CNN set ScaleUp parameteres.

@param[in] p_parm ScaleUp parameters.

@return ER Error ScaleUp parameters are out of range.
*/
ER cnn_set_scaleupparm(BOOL cnn_id, CNN_SCALEUP_PARM *p_parm)
{
	T_SCALEUP_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = SCALEUP_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.SCALE_UP_TOP_PAD     = p_parm->is_top_pad;
	reg1.bit.SCALE_UP_BOTTOM_PAD  = p_parm->is_bot_pad;
	reg1.bit.SCALE_UP_LEFT_PAD    = p_parm->is_left_pad;
	reg1.bit.SCALE_UP_RIGHT_PAD   = p_parm->is_right_pad;
	reg1.bit.SCALE_UP_PAD_VALUE   = p_parm->scaleup_padval;

	switch (p_parm->scaleup_rate) {
	case CNN_SCALEUP_1:
		reg1.bit.SCALE_UP_RATE = 0;
		break;
	case CNN_SCALEUP_2:
		reg1.bit.SCALE_UP_RATE = 1;
		break;
	case CNN_SCALEUP_4:
		reg1.bit.SCALE_UP_RATE = 2;
		break;
	case CNN_SCALEUP_8:
		reg1.bit.SCALE_UP_RATE = 3;
		break;
	default:
		DBG_ERR("In scaleup, unknown stride: %d\r\n", p_parm->scaleup_rate);
		return E_PAR;
	}

	CNN_SETDATA(ofs, reg1.reg, base_addr);

	return E_OK;
}


/**
CNN Get ScaleUp Parameters

CNN get ScaleUp parameters.

@return CNN_SCALEUP_PARM ScaleUp parameters.
*/
CNN_SCALEUP_PARM cnn_get_scaleupparm(BOOL cnn_id)
{
	CNN_SCALEUP_PARM parm = { 0 };
	T_SCALEUP_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = SCALEUP_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	parm.is_top_pad         = reg1.bit.SCALE_UP_TOP_PAD;
	parm.is_bot_pad         = reg1.bit.SCALE_UP_BOTTOM_PAD;
	parm.is_left_pad        = reg1.bit.SCALE_UP_LEFT_PAD;
	parm.is_right_pad       = reg1.bit.SCALE_UP_RIGHT_PAD;
	parm.scaleup_padval     = reg1.bit.SCALE_UP_PAD_VALUE;
	parm.scaleup_rate       = reg1.bit.SCALE_UP_RATE;
	return parm;
}



/**
CNN Set LRN Parameteres

CNN set LRN parameteres.

@param[in] p_parm LRN parameters.

@return ER Error LRN parameters are out of range.
*/
ER cnn_set_lrnparm(BOOL cnn_id, CNN_LRN_PARM *p_parm)
{
	T_LRN_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = LRN_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.LRN_ALPHA     = p_parm->lrn_alpha;
	reg1.bit.LRN_SHIFT  = p_parm->lrn_shift;
	reg1.bit.LRN_BETA    = p_parm->lrn_beta;
	reg1.bit.LRN_N   = p_parm->lrn_n;

	CNN_SETDATA(ofs, reg1.reg, base_addr);

	return E_OK;
}


/**
CNN Get LRN Parameters

CNN get LRN parameters.

@return CNN_LRN_PARM LRN parameters.
*/
CNN_LRN_PARM cnn_get_lrnparm(BOOL cnn_id)
{
	CNN_LRN_PARM parm = { 0 };
	T_LRN_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = LRN_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	parm.lrn_alpha         = reg1.bit.LRN_ALPHA;
	parm.lrn_shift         = reg1.bit.LRN_SHIFT;
	parm.lrn_beta        = reg1.bit.LRN_BETA;
	parm.lrn_n       = reg1.bit.LRN_N;
	return parm;
}

/**
CNN Set Pooling Kernel Type

CNN set global/local pooling kernel type.

@param[in] type global/local pooling kernel type.

@return None.
*/
ER cnn_set_poolkertype(BOOL cnn_id, CNN_POOL_KER type)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (type) {
	case CNN_POOL_KER_MAX:
		reg1.bit.POOL_KERNEL = 0;
		break;
	case CNN_POOL_KER_AVG:
		reg1.bit.POOL_KERNEL = 1;
		break;
	default:
		DBG_ERR("In local/global pooling, unknown kernel type: %d\r\n", type);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Pooling Kernel Type

CNN get global/local pooling kernel type.

@return CNN_POOL_KER pooling kernel type.
*/
CNN_POOL_KER cnn_get_poolkertype(BOOL cnn_id)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (CNN_POOL_KER)reg1.bit.POOL_KERNEL;
}

/**
CNN Set Pooling Shift

CNN set global/local poolingshift.

@param[in] type global/local shift.

@return None.
*/
ER cnn_set_poolshift(BOOL cnn_id, BOOL pool_shf_dir, UINT32 pool_shf)
{
	T_POOL_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER1_OFS;

	if (pool_shf > CNN_POOL_SHF_MAX) {
		DBG_ERR("CNN: CNN_POOLSHF_MAX must not over %d.\r\n", CNN_POOL_SHF_MAX);
		return E_PAR;
	} 

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.POOL_SHIFT_DIR = pool_shf_dir;
	reg1.bit.POOL_SHIFT = pool_shf;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Pooling Shift

CNN get global/local shift.

@return CNN_POOL_KER pooling shift.
*/
ER cnn_get_poolshift(BOOL cnn_id, BOOL *p_pool_shf_dir, UINT32 *p_pool_shf)
{
	T_POOL_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER1_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	*p_pool_shf_dir = reg1.bit.POOL_SHIFT_DIR;
	*p_pool_shf		= reg1.bit.POOL_SHIFT;

	return E_OK;
}

/**
CNN Set Pooling Signedness

CNN set global/local Signedness.

@param[in] type global/local Signedness.

@return None.
*/
VOID cnn_set_poolsignedness(BOOL cnn_id, BOOL p_pool_signedness)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.POOLING_SIGNEDNESS = p_pool_signedness;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Pooling Shift

CNN get global/local shift.

@return CNN_POOL_KER pooling shift.
*/
ER cnn_get_poolsignedness(BOOL cnn_id, BOOL *p_pool_signedness)
{
	T_INPUT_OUTPUT_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = INPUT_OUTPUT_REGISTER_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	*p_pool_signedness = reg1.bit.POOLING_SIGNEDNESS;

	return E_OK;
}


/**
CNN Set Pooling avedivtype

CNN set global/local pooling avedivtype.

@param[in] type global/local pooling ave div type.

@return None.
*/
VOID cnn_set_poolavedivtype(BOOL cnn_id, CNN_POOL_AVE_DIV_TYPE ave_div_type)
{
	T_POOL_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER1_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.POOL_AVE_DIV_TYPE = ave_div_type;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Pooling avedivtype

CNN get global/local pooling avedivtype.

@return global/local pooling ave div type.
*/
CNN_POOL_AVE_DIV_TYPE cnn_get_poolavedivtype(BOOL cnn_id)
{
	T_POOL_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER1_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (CNN_POOL_AVE_DIV_TYPE)reg1.bit.POOL_AVE_DIV_TYPE;
}
/**
CNN Set Pooling output cal type

CNN set local pooling output cal type.

@param[in] type local pooling output cal type.

@return None.
*/
VOID cnn_set_localpool_out_cal_type(BOOL cnn_id, CNN_POOL_OUT_CAL_TYPE out_cal_type)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.POOL_OUT_CAL_TYPE = out_cal_type;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Pooling output cal type

CNN get local pooling output cal type.

@return local pooling output cal type.
*/
CNN_POOL_OUT_CAL_TYPE cnn_get_localpool_out_cal_type(BOOL cnn_id)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (CNN_POOL_OUT_CAL_TYPE)reg1.bit.POOL_OUT_CAL_TYPE;
}

/**
CNN Set Local Pooling Kernel Stride

CNN set local pooling kernel stride.

@param[in] kersize local pooling kernel stride.

@return None.
*/
ER cnn_set_localpool_stride(BOOL cnn_id, CNN_POOL_KER_STRIDE stride)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (stride) {
	case CNN_POOL_KER_STRIDE_1:
		reg1.bit.POOL_STRIDE = 0;
		break;
	case CNN_POOL_KER_STRIDE_2:
		reg1.bit.POOL_STRIDE = 1;
		break;
	default:
		DBG_ERR("In local pooling, unknown stride: %d\r\n", stride);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Local Pooling Kernel Stride

CNN get local pooling kernel stride.

@return CNN_POOL_KER_STRIDE local pooling kernel stride.
*/
CNN_POOL_KER_STRIDE cnn_get_localpool_stride(BOOL cnn_id)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (CNN_POOL_KER_STRIDE)reg1.bit.POOL_STRIDE;
}

/**
CNN Set Local Pooling Kernel Size

CNN set local pooling kernel size.

@param[in] kersize local pooling kernel size.

@return None.
*/
ER cnn_set_localpool_kersize(BOOL cnn_id, CNN_POOL_KER_SIZE kersize)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	switch (kersize) {
	case CNN_POOL_KERSZ_2:
		reg1.bit.POOL_KERSIZE = 0;
		break;
	case CNN_POOL_KERSZ_3:
		reg1.bit.POOL_KERSIZE = 1;
		break;
	case CNN_POOL_KERSZ_4:
		reg1.bit.POOL_KERSIZE = 2;
		break;
	case CNN_POOL_KERSZ_5:
		reg1.bit.POOL_KERSIZE = 3;
		break;
	default:
		DBG_ERR("In local pooling, unknown kernel size: %d\r\n", kersize);
		return E_PAR;
	}
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Local Pooling Kernel Size

CNN get local pooling kernel size.

@return CNN_POOL_KER_SIZE local pooling kernel size.
*/
CNN_POOL_KER_SIZE cnn_get_localpool_kersize(BOOL cnn_id)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (CNN_POOL_KER_SIZE)reg1.bit.POOL_KERSIZE;
}

/**
CNN Set Local Pooling Padding

CNN set local pooling padding.

@param[in] pad_top decide top padding.
@param[in] pad_left decide left padding.
@param[in] pad_bot decide bottom padding.
@param[in] pad_right decide right padding.

@return None.
*/
VOID cnn_set_localpool_pad(BOOL cnn_id, BOOL pad_top, BOOL pad_left, BOOL pad_bot, BOOL pad_right)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.POOL_TOP_PAD       = pad_top;
	reg1.bit.POOL_LEFT_PAD      = pad_left;
	reg1.bit.POOL_BOTTOM_PAD    = pad_bot;
	reg1.bit.POOL_RIGHT_PAD     = pad_right;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get Local Pooling Padding

CNN get local pooling padding.

@param[out] p_pad_top output top padding enable/disable.
@param[out] p_pad_left output left padding enable/disable.
@param[out] p_pad_bot output bottom padding enable/disable.
@param[out] p_pad_right output right padding enable/disable.

@return ER Error local pooling padding parameters are null.
*/
ER cnn_get_localpool_pad(BOOL cnn_id, BOOL *p_pad_top, BOOL *p_pad_left, BOOL *p_pad_bot, BOOL *p_pad_right)
{
	T_POOL_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER0_OFS;

	if ((p_pad_top == NULL) || (p_pad_left == NULL) || (p_pad_bot == NULL) || (p_pad_right == NULL)) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	*p_pad_top    = (BOOL)reg1.bit.POOL_TOP_PAD;
	*p_pad_left   = (BOOL)reg1.bit.POOL_LEFT_PAD;
	*p_pad_bot    = (BOOL)reg1.bit.POOL_BOTTOM_PAD;
	*p_pad_right  = (BOOL)reg1.bit.POOL_RIGHT_PAD;

	return E_OK;
}

/**
CNN Set Local Pooling Parameters

CNN set local pooling parameters.

@param[in] p_parm input cnn local pooling parameters.

@return ER Error local pooling parameters are out of range.
*/
ER cnn_set_localpool_parm(BOOL cnn_id, CNN_LOCAL_POOL_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	//to avoid pooling illegal interrupt, the interrupt is triggered by registers setting not engine start
	er_return = cnn_set_localpool_kersize(cnn_id, CNN_POOL_KERSZ_3);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_localpool_kersize fail...\r\n");
		return E_PAR;
	}

	er_return = cnn_set_poolkertype(cnn_id, p_parm->ker_type);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_poolshift fail...\r\n");
		return E_PAR;
	}
	cnn_set_poolavedivtype(cnn_id, p_parm->ave_div_type);
	er_return = cnn_set_poolshift(cnn_id, p_parm->pool_shf_dir, p_parm->pool_shf);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_poolshift fail...\r\n");
		return E_PAR;
	}
	cnn_set_localpool_out_cal_type(cnn_id, p_parm->out_cal_type);
	er_return = cnn_set_localpool_stride(cnn_id, p_parm->stride);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_localpool_stride fail...\r\n");
		return E_PAR;
	}
	cnn_set_localpool_pad(cnn_id, p_parm->is_top_pad, p_parm->is_left_pad, p_parm->is_bot_pad, p_parm->is_right_pad);
	cnn_set_localpool_kersize(cnn_id, p_parm->ker_size);

	cnn_set_poolsignedness(cnn_id, p_parm->pool_signedness);

	return E_OK;
}

/**
CNN Get Local Pooling Average

CNN get local pooling average parameters.

@return CNN_LOCAL_POOL_PARM cnn local pooling parameters.
*/
CNN_LOCAL_POOL_PARM cnn_get_localpool_parm(BOOL cnn_id)
{
	CNN_LOCAL_POOL_PARM parm = {0};
	parm.ker_type  = cnn_get_poolkertype(cnn_id);
	parm.stride   = cnn_get_localpool_stride(cnn_id);
	parm.ker_size  = cnn_get_localpool_kersize(cnn_id);
	parm.out_cal_type = cnn_get_localpool_out_cal_type(cnn_id);
	parm.ave_div_type = cnn_get_poolavedivtype(cnn_id);

	if (cnn_get_localpool_pad(cnn_id, &parm.is_top_pad, &parm.is_left_pad, &parm.is_bot_pad, &parm.is_right_pad) != E_OK) {
		DBG_ERR("cnn_get_localpool_pad fail...\r\n");
	}
	cnn_get_poolshift(cnn_id, &parm.pool_shf_dir, &parm.pool_shf);
	cnn_get_poolsignedness(cnn_id, &parm.pool_signedness);

	return parm;
}

/**
CNN Set Global Pooling Average

CNN set global pooling average parameters (multiple value and shift).

@param[in] mul global pooling multiple value.
@param[in] shf global pooling shift.

@return None.
*/
ER cnn_set_globalpool_avg(BOOL cnn_id, UINT32 mul, UINT32 shf)
{
	T_POOL_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER1_OFS;

	if (mul > CNN_POOL_AVE_MUL_MAX) {
		DBG_ERR("In global pooling, pool_ave_mul %d is clamp %d!\r\n", (int)mul, CNN_POOL_AVE_MUL_MAX);
		return E_PAR;
	} 

	if (shf > CNN_POOL_AVE_SHF_MAX) {
		DBG_ERR("In global pooling, pool_ave_shift %d is clamp %d!\r\n", (int)shf, CNN_POOL_AVE_SHF_MAX);
		return E_PAR;
	} else if (shf < CNN_POOL_AVE_SHF_MIN) {
		DBG_ERR("In global pooling, pool_ave_shift %d is clamp %d!\r\n", (int)shf, CNN_POOL_AVE_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.POOL_AVE_MUL       = mul;
	reg1.bit.POOL_AVE_SHIFT     = shf - CNN_POOL_AVE_SHF_MIN;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
CNN Get Global Pooling Average

CNN get global pooling average parameters (multiple value and shift).

@param[out] p_mul global pooling multiple value.
@param[out] p_shf global pooling shift.

@return ER Error global pooling average parameters are non-exist.
*/
ER cnn_get_globalpool_avg(BOOL cnn_id, UINT32 *p_mul, UINT32 *p_shf)
{
	T_POOL_REGISTER1 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = POOL_REGISTER1_OFS;
	if ((p_mul == NULL) || (p_shf == NULL)) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	*p_mul = reg1.bit.POOL_AVE_MUL;
	*p_shf = reg1.bit.POOL_AVE_SHIFT + CNN_POOL_AVE_SHF_MIN;

	return E_OK;
}

/**
CNN Set Global Pooling Parameters

CNN set global pooling parameters.

@param[in] p_parm input cnn global pooling parameters.

@return ER Error global pooling parameters are out of range or non-exist.
*/
ER cnn_set_globalpool_parm(BOOL cnn_id, CNN_GLOBAL_POOL_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	/*if (cnn_get_poolmode() != CNN_POOLING_GLOBAL) {
	DBG_ERR("CNN mode should be global pooling mode\r\n");
	return E_CTX;
	}*/
	cnn_set_poolkertype(cnn_id, p_parm->ker_type);
	cnn_set_poolavedivtype(cnn_id, p_parm->ave_div_type);
	er_return = cnn_set_poolshift(cnn_id, p_parm->pool_shf_dir, p_parm->pool_shf);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_poolshift fail...\r\n");
		return E_PAR;
	}

	if (p_parm->ker_type == CNN_POOL_KER_AVG) {
		er_return = cnn_set_globalpool_avg(cnn_id, p_parm->avg_mul, p_parm->avg_shf);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_globalpool_avg fail...\r\n");
			return E_PAR;
		}
	}

	cnn_set_poolsignedness(cnn_id, p_parm->pool_signedness);

	return E_OK;
}

/**
CNN Get Global Pooling Parameters

CNN get global pooling parameters.

@return CNN_GLOBAL_POOL_PARM cnn global pooling parameters.
*/
CNN_GLOBAL_POOL_PARM cnn_get_globalpool_parm(BOOL cnn_id)
{
	CNN_GLOBAL_POOL_PARM parm = {0};
	parm.ker_type   = cnn_get_poolkertype(cnn_id);
	parm.ave_div_type = cnn_get_poolavedivtype(cnn_id);

	if (cnn_get_globalpool_avg(cnn_id, &parm.avg_mul, &parm.avg_shf) != E_OK) {
		DBG_ERR("cnn_get_globalpool_avg fail...\r\n");
	}
	cnn_get_poolshift(cnn_id, &parm.pool_shf_dir, &parm.pool_shf);
	cnn_get_poolsignedness(cnn_id, &parm.pool_signedness);

	return parm;
}

ER cnn_set_clamp(BOOL cnn_id, CNN_CLAMP_PARM *p_parm)
{
	UINT32 i = 0;
	T_CLAMP_THRESHOLD_REGISTER0 reg[CNN_CLAMP_REG_CNT];

	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs[CNN_CLAMP_REG_CNT];
	UINT32 ofs_gap = CLAMP_THRESHOLD_REGISTER1_OFS - CLAMP_THRESHOLD_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = CLAMP_THRESHOLD_REGISTER0_OFS;
	for (i = 1; i < CNN_CLAMP_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}

	for (i = 0; i < CNN_CLAMP_REG_CNT; i++) {
		reg[i].reg = CNN_GETDATA(ofs[i], base_addr);
		reg[i].bit.CLAMP_THRES0 = p_parm->clamp_th[i];
		CNN_SETDATA(ofs[i], reg[i].reg, base_addr);
	}

	return E_OK;
}

/**
CNN Get Clamp parm

CNN get clamp param

@param[out] p_parm cnn clamp parameters.

@return ER Error cnn clamp parameters are out of range.
*/
ER cnn_get_clamp(BOOL cnn_id, CNN_CLAMP_PARM *p_parm){
	UINT32 i = 0;
	T_CLAMP_THRESHOLD_REGISTER0 reg[CNN_CLAMP_REG_CNT];
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs[CNN_CLAMP_REG_CNT];
	UINT32 ofs_gap = CLAMP_THRESHOLD_REGISTER1_OFS - CLAMP_THRESHOLD_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = CLAMP_THRESHOLD_REGISTER0_OFS;
	for (i = 1; i < CNN_CLAMP_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}

	for (i = 0; i < CNN_CLAMP_REG_CNT; i++) {
		reg[i].reg = CNN_GETDATA(ofs[i], base_addr);
		p_parm->clamp_th[i]     = reg[i].bit.CLAMP_THRES0;
	}
	return E_OK;
}



/**
CNN Get clamp rst

CNN get clamp result.

@param[out] p_parm cnn clamp result.

@return ER Error cnn clamp result are out of range.
*/
ER cnn_get_clamp_rst(BOOL cnn_id, CNN_OUT_CLAMP_RST *p_parm){
	UINT32 i = 0;
	T_CLAMP_COUNTER_REGISTER0 reg1[CNN_CLAMP_REG_CNT];
	T_MAX_VALUE_REGISTER0	  reg2[CNN_CLAMP_REG_CNT];
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs1[CNN_CLAMP_REG_CNT];
	UINT32 ofs1_gap = CLAMP_COUNTER_REGISTER1_OFS - CLAMP_COUNTER_REGISTER0_OFS;
	UINT32 ofs2[CNN_CLAMP_REG_CNT];
	UINT32 ofs2_gap = MAX_VALUE_REGISTER1_OFS - MAX_VALUE_REGISTER0_OFS;

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	ofs1[0] = CLAMP_COUNTER_REGISTER0_OFS;
	for (i = 1; i < CNN_CLAMP_REG_CNT; i++) {
		ofs1[i] = ofs1[i - 1] + ofs1_gap;
	}
	ofs2[0] = MAX_VALUE_REGISTER0_OFS;
	for (i = 1; i < CNN_CLAMP_REG_CNT; i++) {
		ofs2[i] = ofs2[i - 1] + ofs2_gap;
	}

	for (i = 0; i < CNN_CLAMP_REG_CNT; i++) {
		reg1[i].reg = CNN_GETDATA(ofs1[i], base_addr);
		p_parm->clamp_count_pos[i]     = reg1[i].bit.CLAMP_COUNT_POS0;
		p_parm->clamp_count_neg[i]     = reg1[i].bit.CLAMP_COUNT_NEG0;
		reg2[i].reg = CNN_GETDATA(ofs2[i], base_addr);
		p_parm->clamp_max_val[i]     = reg2[i].bit.CLAMP_MAX_VAL0;
	}
	return E_OK;
}

/**
CNN Set DMA Disable

CNN set dma disable

@param[in] disable

@return None.
*/
VOID cnn_set_dma_disable(BOOL cnn_id, BOOL disable)
{
	T_DMA_DISABLE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = DMA_DISABLE_REGISTER0_OFS;

	reg1.reg = CNN_GETDATA(ofs, base_addr);
	reg1.bit.DMA_DISABLE = disable;
	CNN_SETDATA(ofs, reg1.reg, base_addr);
}

/**
CNN Get DMA Disable

CNN get dma disable

@return BOOL dma_disable.
*/
BOOL cnn_get_dma_disable(BOOL cnn_id)
{
	T_DMA_DISABLE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = DMA_DISABLE_REGISTER0_OFS;
	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (BOOL)reg1.bit.DMA_DISABLE;
}

/**
CNN Get Engine Idle

CNN get engine idle

@return BOOL cnn_idle.
*/
BOOL cnn_get_engine_idle(BOOL cnn_id)
{
	T_DMA_DISABLE_REGISTER0 reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = DMA_DISABLE_REGISTER0_OFS;
	reg1.reg = CNN_GETDATA(ofs, base_addr);

	return (BOOL)reg1.bit.CNN_IDLE;
}

/**
CNN Set DRAM Input and Output Addresses

CNN set eight DMA input and two output starting addresses.

@param[in] p_addr DMA input and output buffer starting addresses.

@return ER Error address formats are wrong or null.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_set_dmaio_addr(BOOL cnn_id, CNN_DMAIO_ADDR dma_addr)
{
	ER er_return = E_OK;
	CNN_DMAIO_ADDR *p_addr = &dma_addr;
#if (CNN_DMA_CACHE_HANDLE == 1)
	UINT32 addr0, addr4, addr5, addr6, addr7, addr8, addr9;
	UINT32 phy_addr0 = 0, phy_addr4 = 0, phy_addr5 = 0, phy_addr6 = 0, phy_addr7 = 0, phy_addr8 = 0, phy_addr9 = 0;
	UINT32 CNN_dram_in_size0 =0, CNN_dram_in_size4=0, CNN_dram_in_size5=0, CNN_dram_in_size6=0, CNN_dram_in_size7=0, CNN_dram_out_size0=0, CNN_dram_out_size1=0;
	UINT32 kerl_en;


	/* Set to registers*/
	kerl_en = cnn_get_kerl_en(cnn_id);
	cnn_cal_sz(cnn_id, &CNN_dram_in_size0, &CNN_dram_in_size4, &CNN_dram_in_size5, &CNN_dram_in_size6, &CNN_dram_in_size7, &CNN_dram_out_size0, &CNN_dram_out_size1);


	/* Convolution*/
	switch (cnn_get_engmode(cnn_id)) {
	case CNN_CONV:
		// ------ Input Dram DMA/Cache Handle ------
		addr0 = p_addr->inaddr0; //Input0
		addr4 = p_addr->inaddr4; //Input4
		addr5 = p_addr->inaddr5; //Input5
		addr6 = p_addr->inaddr6; //Input6
		addr7 = p_addr->inaddr7; //Input7

		if (p_addr->drv_dma_not_sync == 0) {
			if (addr0 > 0) {
				cnn_pt_dma_flush_mem2dev(addr0, CNN_dram_in_size0);
			}
			if (addr4 > 0) {
				cnn_pt_dma_flush_mem2dev(addr4, CNN_dram_in_size4);
			}
			if ((kerl_en & CNN_FCD_VLC_EN) && (addr5 > 0)) {
				cnn_pt_dma_flush_mem2dev(addr5, CNN_dram_in_size5);
			}
			if (addr6 > 0) {
				cnn_pt_dma_flush_mem2dev(addr6, CNN_dram_in_size6);
			}
			if (addr7 > 0) {
				cnn_pt_dma_flush_mem2dev(addr7, CNN_dram_in_size7);
			}
		}

		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;
		phy_addr4 = (addr4 != 0) ? cnn_pt_va2pa(addr4) : 0;
		phy_addr5 = (addr5 != 0) ? cnn_pt_va2pa(addr5) : 0;
		phy_addr6 = (addr6 != 0) ? cnn_pt_va2pa(addr6) : 0;
		phy_addr7 = (addr7 != 0) ? cnn_pt_va2pa(addr7) : 0;


		er_return = cnn_set_dmain_addr(cnn_id, phy_addr0, phy_addr4, phy_addr5, phy_addr6, phy_addr7);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmain_addr fail...\r\n");
			return E_PAR;
		}

#if 0

		DBG_IND("addr0:0x%08X, phy_addr0:0x%08X\n", addr0, phy_addr0);
		DBG_IND("addr4:0x%08X, phy_addr4:0x%08X\n", addr4, phy_addr4);
		DBG_IND("addr5:0x%08X, phy_addr5:0x%08X\n", addr5, phy_addr5);
		DBG_IND("addr6:0x%08X, phy_addr6:0x%08X\n", addr6, phy_addr6);
		DBG_IND("addr7:0x%08X, phy_addr7:0x%08X\n", addr7, phy_addr7);
#endif


		// ------ Output Dram DMA/Cache Handle 0 ------
		addr8 = p_addr->outaddr0;
		phy_addr8 = (addr8 != 0) ? cnn_pt_va2pa(addr8) : 0;
		er_return = cnn_set_dmaout0_addr(cnn_id, phy_addr8);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmaout0_addr fail...\r\n");
			return E_PAR;
		}
		//DBG_IND("addr0:0x%08X, size0:%d\n", CNN_dram_out_addr0, CNN_dram_out_size0);
		//DBG_IND("addr1:0x%08X, size1:%d\n", CNN_dram_out_addr1, CNN_dram_out_size1);


		// ------ Output Dram DMA/Cache Handle 1 ------
		addr9 = p_addr->outaddr1;
		phy_addr9 = (addr9 != 0) ? cnn_pt_va2pa(addr9) : 0;
		er_return = cnn_set_dmaout1_addr(cnn_id, phy_addr9);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmaout1_addr fail...\r\n");
			return E_PAR;
		}
		//DBG_IND("Out1 addr9:%08lx phy_addr9:%08lx\r\n", addr9, phy_addr9);

#if 0
		DBG_IND("addr8:0x%08X, phy_addr8:0x%08X\n", addr8, phy_addr8);
		DBG_IND("addr9:0x%08X, phy_addr9:0x%08X\n", addr9, phy_addr9);
#endif
		break;

	case CNN_DECONV:
		// ------ Input Dram DMA/Cache Handle ------
		addr0 = p_addr->inaddr0;
		if (p_addr->drv_dma_not_sync == 0) {
			if (addr0 > 0) {
				cnn_pt_dma_flush_mem2dev(addr0, CNN_dram_in_size0);
			}
		}
		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;

		er_return = cnn_set_dmain_addr(cnn_id, phy_addr0, phy_addr4, phy_addr5, phy_addr6, phy_addr7);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmain_addr fail...\r\n");
			return E_PAR;
		}

		// ------ Output Dram DMA/Cache Handle0 ------
		addr0 = p_addr->outaddr0;
		//phy_addr0 = dma_getPhyAddr(addr0);
		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;
		er_return = cnn_set_dmaout0_addr(cnn_id, phy_addr0);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmaout0_addr fail...\r\n");
			return E_PAR;
		}
		break;
	case CNN_SCALEUP:
		// ------ Input Dram DMA/Cache Handle ------
		addr0 = p_addr->inaddr0;
		if (p_addr->drv_dma_not_sync == 0) {
			if (addr0 > 0) {
				cnn_pt_dma_flush_mem2dev(addr0, CNN_dram_in_size0);
			}
		}
		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;
		er_return = cnn_set_dmain_addr(cnn_id, phy_addr0, phy_addr4, phy_addr5, phy_addr6, phy_addr7);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmain_addr fail...\r\n");
			return E_PAR;
		}

		// ------ Output Dram DMA/Cache Handle0 ------
		addr0 = p_addr->outaddr0;
		//phy_addr0 = dma_getPhyAddr(addr0);
		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;
		er_return = cnn_set_dmaout0_addr(cnn_id, phy_addr0);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmaout0_addr fail...\r\n");
			return E_PAR;
		}
		break;
	case CNN_LRN:
		// ------ Input Dram DMA/Cache Handle ------
		addr0 = p_addr->inaddr0;
		if (p_addr->drv_dma_not_sync == 0) {
			if (addr0 > 0) {
				cnn_pt_dma_flush_mem2dev(addr0, CNN_dram_in_size0);
			}
		}
		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;
		er_return = cnn_set_dmain_addr(cnn_id, phy_addr0, phy_addr4, phy_addr5, phy_addr6, phy_addr7);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmain_addr fail...\r\n");
			return E_PAR;
		}

		// ------ Output Dram DMA/Cache Handle0 ------
		addr0 = p_addr->outaddr0;
		//phy_addr0 = dma_getPhyAddr(addr0);
		phy_addr0 = (addr0 != 0) ? cnn_pt_va2pa(addr0) : 0;
		er_return = cnn_set_dmaout0_addr(cnn_id, phy_addr0);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dmaout0_addr fail...\r\n");
			return E_PAR;
		}
		break;
	default:
		DBG_ERR("Error unknown CNN engine mode: %d\r\n", cnn_get_engmode(cnn_id));
		return E_CTX;
	}

#else
	er_return = cnn_set_dmain_addr(cnn_id, p_addr->inaddr0, p_addr->inaddr4, p_addr->inaddr5, p_addr->inaddr6,
		p_addr->inaddr7);

	if (er_return != E_OK) {
		DBG_ERR("cnn_set_dmain_addr fail...\r\n");
		return E_PAR;
	}
	er_return = cnn_set_dmaout0_addr(cnn_id, p_addr->outaddr0);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_dmaout0_addr fail...\r\n");
		return E_PAR;
	}
	er_return = cnn_set_dmaout1_addr(cnn_id, p_addr->outaddr1);

	if (er_return != E_OK) {
		DBG_ERR("cnn_set_dmaout1_addr fail...\r\n");
		return E_PAR;
	}
#endif
	return E_OK;
}

/**
CNN Get DRAM Input and Output Addresses

CNN get four DMA input and one output starting addresses.

@return DMA input and output buffer starting addresses.
*/
CNN_DMAIO_ADDR cnn_get_dmaio_addr(BOOL cnn_id)
{
	CNN_DMAIO_ADDR dma_addr = {0};
	dma_addr.inaddr0    = cnn_get_dmain_addr(cnn_id, CNN_IN0_BUF);
	dma_addr.inaddr4    = cnn_get_dmain_addr(cnn_id, CNN_IN4_BUF);
	dma_addr.inaddr5    = cnn_get_dmain_addr(cnn_id, CNN_IN5_BUF);
	dma_addr.inaddr6    = cnn_get_dmain_addr(cnn_id, CNN_IN6_BUF);
	dma_addr.inaddr7    = cnn_get_dmain_addr(cnn_id, CNN_IN7_BUF);
	dma_addr.outaddr0   = cnn_get_dmaout0_addr(cnn_id);
	dma_addr.outaddr1   = cnn_get_dmaout1_addr(cnn_id);
	return dma_addr;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
CNN Check If Engine Is Running

Check if CNN engine is already started.

@return
\n-@b ENABLE: busy.
\n-@b DISABLE: idle.
*/
BOOL cnn_isenable(BOOL cnn_id)
{
	T_CNN_CONTROL_REGISTER reg1;
	UINT32 base_addr = cnn_get_regbase_addr(cnn_id);
	UINT32 ofs = CNN_CONTROL_REGISTER_OFS;
	reg1.reg = CNN_GETDATA(ofs, base_addr);
	if (reg1.bit.CNN_START == 1 && reg1.bit.CNN_SRAMCTRL == 0) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}

UINT32 cnn_tran_intval(INT32 value, UINT32 bits)
{
	UINT32 out_val;
	if (value < 0) {
		out_val = (UINT32)((INT32)(1 << (bits)) + value);
	} else {
		out_val = (UINT32)value;
	}
	return out_val;
}

/**
CNN Transform Bit Value

CNN transform the small bits value to the 32 bits value.

@param[in] value input the unsigned integer value with finite bit size.
@param[in] bits bit size of the value.

@return 32 bits signed integer value.
*/
INT32 cnn_tran_bitval(UINT32 value, UINT32 bits)
{
	INT32 out_val = 0;
	if (bits < 1) {
		DBG_ERR("bits should higher than 0: %d\r\n", (int)bits);
		return E_PAR;
	}

	if (value < (UINT32)(1 << (bits - 1))) {
		out_val = (INT32)value;
	} else {
		out_val = (INT32)value - (INT32)(1 << bits);
	}
	return out_val;
}


EXPORT_SYMBOL(cnn_get_int_enable);
EXPORT_SYMBOL(cnn_clr_intr_status);
EXPORT_SYMBOL(cnn_get_intr_status);
EXPORT_SYMBOL(cnn_isenable);
EXPORT_SYMBOL(cnn_set_dmain_lladdr);
EXPORT_SYMBOL(cnn_get_dmain_lladdr);
EXPORT_SYMBOL(cnn_set_dmain_lladdr_base);
EXPORT_SYMBOL(cnn_get_dmain_lladdr_base);
EXPORT_SYMBOL(cnn_get_poolkertype);
EXPORT_SYMBOL(cnn_get_localpool_parm);
EXPORT_SYMBOL(cnn_get_globalpool_parm);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
EXPORT_SYMBOL(cnn_get_engmode);
EXPORT_SYMBOL(cnn_get_kerl_en);
EXPORT_SYMBOL(cnn_get_out0type);
EXPORT_SYMBOL(cnn_get_out1type);
EXPORT_SYMBOL(cnn_get_dmaio_addr);
EXPORT_SYMBOL(cnn_get_intype);
EXPORT_SYMBOL(cnn_get_insize);
#endif

//@}
