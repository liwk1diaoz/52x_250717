#include "kdrv_ai_int.h"

#if defined(__FREERTOS)
#include "kwrap/debug.h"
#include <string.h>
#else
//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <asm/io.h>
//#include <linux/uaccess.h>
//#include "mach/fmem.h"
//#include <linux/delay.h>
//#include <linux/timer.h>
//#include <linux/spinlock.h>
#include "kdrv_ai_dbg.h"
#include <linux/string.h>
#endif

#include "kwrap/error_no.h"

#define KDRV_AI_RHO_FMT_MIN         3
#define KDRV_AI_RHO_FMT_MAX         10
#define KDRV_AI_FCD_VLC_SIZE       23

NUE_SVM_PARA def_nue_svm = { 0, 1, {0, 0, 0, 0, 1, 0, 3, 0}, {0} };
NUE_RELU_PARM def_nue_relu = { NUE_RELU_LEAKY, 0, 7, 0 };
NUE_ROI_POOL_PARM def_nue_roipool = { 1, 0, NUE_ROI_POOL_SIZE_6, 1, 0, 0 };
//NUE_GLOBAL_POOL_PARM def_nue_globalpool = { NUE_POOL_KER_MAX, 0, 0, 10 };
//NUE_LOCAL_POOL_PARM def_nue_localpool = { NUE_POOL_KER_MAX, 0, 0, 0, FALSE, FALSE, FALSE, FALSE };
NUE_REORG_PARM def_nue_reorg = { 0 };
NUE_ANCHOR_PARM def_nue_anchor = {FALSE, 0};
NUE_SOFTMAX_PARM def_nue_softmax = {0, 0, 0, 0};
//NUE_ELTWISE_PARM def_nue_elt = { 0 };
CNN_CONV_PARM def_cnn_conv = { {0}, {0} };
CNN_BNSCALE_PARM def_cnn_bnscale = { 0 };
//CNN_RELU_PARM def_cnn_act = {{CNN_RELU_LEAKY, 0, 7, 0}, {CNN_RELU_LEAKY, 0, 7, 0}, {CNN_RELU_LEAKY, 0, 7, 0}};
CNN_RELU_IN def_cnn_act = {CNN_RELU_LEAKY, 0, 7, 0};
CNN_LOCAL_POOL_PARM def_cnn_localpool = { 0 };
CNN_GLOBAL_POOL_PARM def_cnn_globalpool = { 0 };
CNN_DECONV_PARM def_cnn_deconv = { 0 };
CNN_ELTWISE_PARM def_cnn_elt = {FALSE, 0, 0, 0, 0, 0, 0, 0};

NUE2_PAD_PARM def_nue2_pad = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

CNN_IO_TYPE kdrv_ai2cnn_io_type(KDRV_AI_IO_TYPE intype);
NUE_IO_TYPE kdrv_ai2nue_io_type(KDRV_AI_IO_TYPE intype);
UINT8 kdrv_ai_align_pad_num(char *p_pad_dir_name, UINT8 *p_pad_dir_num, UINT8 other_pad_num);
ER kdrv_ai_tran_nue_fcd_parm(KDRV_AI_FCD_KERPARM *p_fcd_parm, NUE_PARM *p_eng_parm, UINT32 *p_func_en);

CNN_IO_TYPE kdrv_ai2cnn_io_type(KDRV_AI_IO_TYPE intype)
{
	CNN_IO_TYPE outtype = CNN_INT8;
	switch (intype) {
	case AI_IO_INT8:
		outtype = CNN_INT8;
		break;
	case AI_IO_UINT8:
		outtype = CNN_UINT8;
		break;
	case AI_IO_INT16:
		outtype = CNN_INT16;
		break;
	case AI_IO_UINT16:
		outtype = CNN_UINT16;
		break;
	default:
		nvt_dbg(WRN, "Unknown cnn io type: %d\r\n", intype);
		break;
	}
	return outtype;
}

NUE_IO_TYPE kdrv_ai2nue_io_type(KDRV_AI_IO_TYPE intype)
{
	NUE_IO_TYPE outtype = NUE_INT8;
	switch (intype) {
	case AI_IO_INT8:
		outtype = NUE_INT8;
		break;
	case AI_IO_UINT8:
		outtype = NUE_UINT8;
		break;
	case AI_IO_INT16:
		outtype = NUE_INT16;
		break;
	case AI_IO_UINT16:
		outtype = NUE_UINT16;
		break;
	default:
		nvt_dbg(WRN, "Unknown nue io type: %d\r\n", intype);
		break;
	}
	return outtype;
}

/*
  todo: nue2 may need api for output signedness
*/

UINT8 kdrv_ai_align_pad_num(char *p_pad_dir_name, UINT8 *p_pad_dir_num, UINT8 other_pad_num)
{
	UINT8 align_pad_num = 0;
	if ((p_pad_dir_name == NULL) || (p_pad_dir_num == NULL)) {
		nvt_dbg(ERR, "null input...\r\n");
		return 0;
	}

	if ((*p_pad_dir_num) < 1) {
		//no padding(direction)
		return other_pad_num;
	}

	if ((other_pad_num != 0) && (other_pad_num != (*p_pad_dir_num))) {
		nvt_dbg(WRN, "%s(%d) is not equal with others: %d\r\n", p_pad_dir_name, (int)(*p_pad_dir_num), (int)other_pad_num);
		nvt_dbg(WRN, "%s(%d) is clamp with %d\r\n", p_pad_dir_name, (int)(*p_pad_dir_num), (int)other_pad_num);
		*p_pad_dir_num = other_pad_num;
	}
	align_pad_num = *p_pad_dir_num;

	return align_pad_num;
}

ER kdrv_ai_tran_cnn_parm(KDRV_AI_NEURAL_PARM *p_ai_parm, CNN_PARM *p_eng_parm, UINT32 func_en)
{
	UINT32 io_en = 0, eng_func_en = 0;

	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(CNN_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(CNN_USER_DEF));
#if AI_V4_STRIPE_FUNC
	if (p_ai_parm->in_ofs.h_stripe_en == 1 || p_ai_parm->out0_ofs.h_stripe_en == 1 || p_ai_parm->out1_ofs.h_stripe_en == 1) {
		io_en = io_en | CNN_IN_ISHSTRIPE_EN;
	}
	if (p_ai_parm->in_ofs.v_stripe_en == 1 || p_ai_parm->out0_ofs.v_stripe_en == 1 || p_ai_parm->out1_ofs.v_stripe_en == 1) {
		io_en = io_en | CNN_IN_ISVSTRIPE_EN;
	}
#else
	if ((p_ai_parm->in_ofs.line_ofs * p_ai_parm->size.height) < p_ai_parm->in_ofs.channel_ofs) {
		io_en = io_en | CNN_IN_ISVSTRIPE_EN;
	}
	if (p_ai_parm->size.width * (1+(p_ai_parm->in_type >> 1)) < p_ai_parm->in_ofs.line_ofs) {
		io_en = io_en | CNN_IN_ISHSTRIPE_EN;
	}
#endif

	p_eng_parm->intype      = kdrv_ai2cnn_io_type(p_ai_parm->in_type);
	p_eng_parm->out0type    = kdrv_ai2cnn_io_type(p_ai_parm->out0_type);
	p_eng_parm->out1type    = kdrv_ai2cnn_io_type(p_ai_parm->out1_type);
	p_eng_parm->insize.width        = p_ai_parm->size.width;
	p_eng_parm->insize.height       = p_ai_parm->size.height;
	p_eng_parm->insize.channel      = p_ai_parm->size.channel;
	p_eng_parm->insize.batch        = p_ai_parm->batch_num;
	p_eng_parm->out_ofs.out0_ofs    = p_ai_parm->out0_cropy;

	// set scale/shift
	p_eng_parm->out_scale_parm.conv_shf_dir = (p_ai_parm->conv.sclshf.out_shift < 0)?1:0;
	p_eng_parm->out_scale_parm.conv_shf     = (p_ai_parm->conv.sclshf.out_shift < 0)?(-1*p_ai_parm->conv.sclshf.out_shift):p_ai_parm->conv.sclshf.out_shift;
	p_eng_parm->out_scale_parm.conv_scale   = p_ai_parm->conv.sclshf.out_scale;
	p_eng_parm->out_scale_parm.elt_shf_dir  = (p_ai_parm->elt.sclshf.out_shift < 0)?1:0;
	p_eng_parm->out_scale_parm.elt_shf      = (p_ai_parm->elt.sclshf.out_shift < 0)?(-1*p_ai_parm->elt.sclshf.out_shift):p_ai_parm->elt.sclshf.out_shift;
	p_eng_parm->out_scale_parm.elt_scale    = p_ai_parm->elt.sclshf.out_scale;
	p_eng_parm->out_scale_parm.pool_shf_dir = (p_ai_parm->pool.sclshf.out_shift < 0)?1:0;
	p_eng_parm->out_scale_parm.pool_shf     = (p_ai_parm->pool.sclshf.out_shift < 0)?(-1*p_ai_parm->pool.sclshf.out_shift):p_ai_parm->pool.sclshf.out_shift;
	p_eng_parm->out_scale_parm.pool_scale   = p_ai_parm->pool.sclshf.out_scale;
	if (func_en & KDRV_AI_NEURAL_DECONV_EN) {
		p_eng_parm->out_scale_parm.out0_shf_dir = (p_ai_parm->deconv.sclshf.out_shift < 0)?1:0;
		p_eng_parm->out_scale_parm.out0_shf     = (p_ai_parm->deconv.sclshf.out_shift < 0)?(-1*p_ai_parm->deconv.sclshf.out_shift):p_ai_parm->deconv.sclshf.out_shift;
		p_eng_parm->out_scale_parm.out0_scale   = p_ai_parm->deconv.sclshf.out_scale;
	} else {
		p_eng_parm->out_scale_parm.out0_shf_dir = (p_ai_parm->act.sclshf.out_shift < 0)?1:0;
		p_eng_parm->out_scale_parm.out0_shf     = (p_ai_parm->act.sclshf.out_shift < 0)?(-1*p_ai_parm->act.sclshf.out_shift):p_ai_parm->act.sclshf.out_shift;
		p_eng_parm->out_scale_parm.out0_scale   = p_ai_parm->act.sclshf.out_scale;
	}
	p_eng_parm->out_scale_parm.out1_shf_dir = (p_ai_parm->act1.sclshf.out_shift < 0)?1:0;
	p_eng_parm->out_scale_parm.out1_shf     = (p_ai_parm->act1.sclshf.out_shift < 0)?(-1*p_ai_parm->act1.sclshf.out_shift):p_ai_parm->act1.sclshf.out_shift;
	p_eng_parm->out_scale_parm.out1_scale   = p_ai_parm->act1.sclshf.out_scale;


	// set lineoffset
	if (p_ai_parm->batch_num > 1) {
		p_eng_parm->dmaio_lofs.in0_lofs = p_ai_parm->in_ofs.batch_ofs;
		p_eng_parm->dmaio_lofs.in1_lofs = 0;
		p_eng_parm->dmaio_lofs.in2_lofs = p_ai_parm->elt.in_ofs.batch_ofs;
		p_eng_parm->dmaio_lofs.in3_lofs = 0;
		p_eng_parm->dmaio_lofs.out0_lofs = p_ai_parm->out0_ofs.batch_ofs;
		p_eng_parm->dmaio_lofs.out1_lofs = 0;
		p_eng_parm->dmaio_lofs.out2_lofs = p_ai_parm->out1_ofs.batch_ofs;
		p_eng_parm->dmaio_lofs.out3_lofs = 0;
	} else {
		p_eng_parm->dmaio_lofs.in0_lofs = p_ai_parm->in_ofs.line_ofs;
		p_eng_parm->dmaio_lofs.in1_lofs = p_ai_parm->in_ofs.channel_ofs;
		p_eng_parm->dmaio_lofs.in2_lofs = p_ai_parm->elt.in_ofs.line_ofs;
		p_eng_parm->dmaio_lofs.in3_lofs = p_ai_parm->elt.in_ofs.channel_ofs;
		p_eng_parm->dmaio_lofs.out0_lofs = p_ai_parm->out0_ofs.line_ofs;
		p_eng_parm->dmaio_lofs.out1_lofs = p_ai_parm->out0_ofs.channel_ofs;
		p_eng_parm->dmaio_lofs.out2_lofs = p_ai_parm->out1_ofs.line_ofs;
		p_eng_parm->dmaio_lofs.out3_lofs = p_ai_parm->out1_ofs.channel_ofs;
	}


	// set addr
	if (p_ai_parm->src_fmt == AI_NEURAL_SRC_IMG)
		io_en = io_en | CNN_IN_ISIMAGE_EN;
	p_eng_parm->dmaio_addr.inaddr0  = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.inaddr4  = p_ai_parm->conv.weight_addr;
	p_eng_parm->dmaio_addr.inaddr5  = p_ai_parm->conv.fcd.quanti_kmeans_addr;
	p_eng_parm->dmaio_addr.inaddr6  = p_ai_parm->conv.bias_addr;
	if (p_ai_parm->in_interm_dma_en) {
		io_en = io_en | CNN_IN_ISCHANNELSTRIPE_EN;
		p_eng_parm->dmaio_addr.inaddr7 = p_ai_parm->in_interm_addr;
	} else {
		p_eng_parm->dmaio_addr.inaddr7 = p_ai_parm->elt.addr;
	}

	if (p_ai_parm->out_interm_dma_en) {
		io_en = io_en | CNN_OUT0_EN;
		p_eng_parm->dmaio_addr.outaddr0 = p_ai_parm->out_interm_addr;
		p_eng_parm->dmaio_addr.outaddr1 = p_ai_parm->out1_addr;
	} else {
		if (p_ai_parm->out0_dma_en) {
			io_en = io_en | CNN_OUT0_EN;
		}
		if (p_ai_parm->out1_dma_en) {
			io_en = io_en | CNN_OUT1_EN;
		}
		p_eng_parm->dmaio_addr.outaddr0 = p_ai_parm->out0_addr;
		p_eng_parm->dmaio_addr.outaddr1 = p_ai_parm->out1_addr;
	}

	io_en = io_en & (~CNN_IN_REMAINSRC_EN);


	if (func_en & KDRV_AI_NEURAL_DECONV_EN) {
		CNN_DECONV_KER_STRIDE ker_stride = CNN_DECONV_KER_STRIDE_1;
		UINT32 stride_x = 0/*, stride_y = 0*/;
		stride_x = p_ai_parm->deconv.ker_stridex;
		//stride_y = p_ai_parm->deconv.ker_stridey;

		switch (stride_x) {
		case 1:
			ker_stride = CNN_DECONV_KER_STRIDE_1;
			break;
		case 2:
			ker_stride = CNN_DECONV_KER_STRIDE_2;
			break;
		case 4:
			ker_stride = CNN_DECONV_KER_STRIDE_4;
			break;
		case 8:
			ker_stride = CNN_DECONV_KER_STRIDE_8;
			break;
		default:
			nvt_dbg(WRN, "not support deconv stride: %d, clamp to %d\r\n", (int)stride_x, 1);
			ker_stride = CNN_DECONV_KER_STRIDE_1;
			break;
		}

		if (p_ai_parm->deconv.func_sel == AI_DECONV_SEL) {
			p_eng_parm->deconv_parm.is_top_pad      = p_ai_parm->deconv.pad.top_pad_num;
			p_eng_parm->deconv_parm.is_bot_pad      = p_ai_parm->deconv.pad.bot_pad_num;
			p_eng_parm->deconv_parm.is_left_pad     = p_ai_parm->deconv.pad.left_pad_num;
			p_eng_parm->deconv_parm.is_right_pad    = p_ai_parm->deconv.pad.right_pad_num;
			p_eng_parm->deconv_parm.deconv_padval   = p_ai_parm->deconv.pad.pad_val;
			p_eng_parm->deconv_parm.deconv_stride   = ker_stride;
		} else if (p_ai_parm->deconv.func_sel == AI_UPSAMPLE_SEL) {
			p_eng_parm->scaleup_parm.is_top_pad     = p_ai_parm->deconv.pad.top_pad_num;
			p_eng_parm->scaleup_parm.is_bot_pad     = p_ai_parm->deconv.pad.bot_pad_num;
			p_eng_parm->scaleup_parm.is_left_pad    = p_ai_parm->deconv.pad.left_pad_num;
			p_eng_parm->scaleup_parm.is_right_pad   = p_ai_parm->deconv.pad.right_pad_num;
			p_eng_parm->scaleup_parm.scaleup_padval = p_ai_parm->deconv.pad.pad_val;
			p_eng_parm->scaleup_parm.scaleup_rate   = (CNN_SCALEUP_RATE)ker_stride;
		} else {
			nvt_dbg(WRN, "unknown deconv mode = %d\r\n", (int)p_ai_parm->deconv.func_sel);
		}
	} else {
		memcpy(&p_eng_parm->deconv_parm, &def_cnn_deconv, sizeof(CNN_DECONV_PARM));
	}

	if (func_en & KDRV_AI_NEURAL_CONV_EN) {
		CNN_CONV_KER_SIZE ker_size;
		CNN_CONV_KER_STRIDE stride;
		UINT32 stride_x, /*stride_y,*/ ker_w, ker_h;
		UINT32 i = 0;

		stride_x = p_ai_parm->conv.ker_stridex;
		//stride_y = p_ai_parm->conv.ker_stridey;

		switch (stride_x) {
		case 1:
			stride = CNN_CONV_KER_STRIDE_1;
			break;
		case 2:
			stride = CNN_CONV_KER_STRIDE_2;
			break;
		case 4:
			stride = CNN_CONV_KER_STRIDE_4;
			break;
		default:
			nvt_dbg(WRN, "not support conv stride: %d, clamp to %d\r\n", (int)stride_x, 1);
			stride = CNN_CONV_KER_STRIDE_1;
			break;
		}

		ker_w = p_ai_parm->conv.ker_w;
		ker_h = p_ai_parm->conv.ker_h;

		if (ker_w == 11 && ker_h == 11)
			ker_size = CNN_CONV_KERSZ_11_11;
		else if (ker_w == 7 && ker_h == 7)
			ker_size = CNN_CONV_KERSZ_7_7;
		else if (ker_w == 5 && ker_h == 5)
			ker_size = CNN_CONV_KERSZ_5_5;
		else if (ker_w == 3 && ker_h == 3)
			ker_size = CNN_CONV_KERSZ_3_3;
		else if (ker_w == 1 && ker_h == 1)
			ker_size = CNN_CONV_KERSZ_1_1;
		else if (ker_w == 7 && ker_h == 1)
			ker_size = CNN_CONV_KERSZ_7_1;
		else if (ker_w == 1 && ker_h == 7)
			ker_size = CNN_CONV_KERSZ_1_7;
		else if (ker_w == 5 && ker_h == 1)
			ker_size = CNN_CONV_KERSZ_5_1;
		else if (ker_w == 1 && ker_h == 5)
			ker_size = CNN_CONV_KERSZ_1_5;
		else if (ker_w == 3 && ker_h == 1)
			ker_size = CNN_CONV_KERSZ_3_1;
		else if (ker_w == 1 && ker_h == 3)
			ker_size = CNN_CONV_KERSZ_1_3;
		else if (ker_w == 9 && ker_h == 9)
			ker_size = CNN_CONV_KERSZ_9_9;
		else {
			nvt_dbg(WRN, "not support conv kernel size: %d x %d, clamp to 1 x 1\r\n", (int)ker_w, (int)ker_h);
			ker_size = CNN_CONV_KERSZ_1_1;
		}

		p_eng_parm->conv_parm.convkerl_parms.conv_kersize   = ker_size;
		p_eng_parm->conv_parm.convkerl_parms.conv_stride    = stride;
		p_eng_parm->conv_parm.convkerl_parms.conv_setnum    = p_ai_parm->conv.ker_set_num;
		p_eng_parm->conv_parm.convkerl_parms.conv_shf_acc   = p_ai_parm->conv.acc_shf;
		p_eng_parm->conv_parm.convkerl_parms.is_top_pad     = p_ai_parm->conv.pad.top_pad_num > 0 ? TRUE : FALSE;
		p_eng_parm->conv_parm.convkerl_parms.is_bot_pad     = p_ai_parm->conv.pad.bot_pad_num > 0 ? TRUE : FALSE;
		p_eng_parm->conv_parm.convkerl_parms.is_left_pad    = p_ai_parm->conv.pad.left_pad_num > 0 ? TRUE : FALSE;
		p_eng_parm->conv_parm.convkerl_parms.is_right_pad   = p_ai_parm->conv.pad.right_pad_num > 0 ? TRUE : FALSE;
		p_eng_parm->conv_parm.convkerl_parms.conv_shf_bias  = p_ai_parm->conv.bias_para_shf;

		p_eng_parm->conv_parm.fcd_parm.fcd_vlc_en           = p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_VLC_EN ? TRUE : FALSE;
		p_eng_parm->conv_parm.fcd_parm.fcd_quanti_en        = p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_QUANTI_EN ? TRUE : FALSE;
		p_eng_parm->conv_parm.fcd_parm.fcd_sparse_en        = p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_SPARSE_EN ? TRUE : FALSE;
		p_eng_parm->conv_parm.fcd_parm.fcd_quanti_kmean_en  = p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_QUANTI_KMEANS_EN ? TRUE : FALSE;
		p_eng_parm->conv_parm.fcd_parm.fcd_enc_bit_length   = p_ai_parm->conv.fcd.enc_bit_length;

		if (p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_VLC_EN) {
			eng_func_en = eng_func_en | CNN_FCD_VLC_EN;

			if ((p_ai_parm->conv.fcd.vlc_code_size != KDRV_AI_FCD_VLC_SIZE)
				|| (p_ai_parm->conv.fcd.vlc_valid_size != KDRV_AI_FCD_VLC_SIZE)
				|| (p_ai_parm->conv.fcd.vlc_ofs_size != KDRV_AI_FCD_VLC_SIZE)) {
				nvt_dbg(WRN, "fcd vlc code size %d is clamp %d\r\n", (unsigned int)p_ai_parm->conv.fcd.vlc_code_size, KDRV_AI_FCD_VLC_SIZE);
				nvt_dbg(WRN, "fcd vlc valid size %d is clamp %d\r\n", (unsigned int)p_ai_parm->conv.fcd.vlc_valid_size, KDRV_AI_FCD_VLC_SIZE);
				nvt_dbg(WRN, "fcd vlc offset size %d is clamp %d\r\n", (unsigned int)p_ai_parm->conv.fcd.vlc_ofs_size, KDRV_AI_FCD_VLC_SIZE);
				p_ai_parm->conv.fcd.vlc_code_size   = KDRV_AI_FCD_VLC_SIZE;
				p_ai_parm->conv.fcd.vlc_valid_size  = KDRV_AI_FCD_VLC_SIZE;
				p_ai_parm->conv.fcd.vlc_ofs_size    = KDRV_AI_FCD_VLC_SIZE;
			}

			for (i = 0; i < p_ai_parm->conv.fcd.vlc_code_size; i++) {
				p_eng_parm->conv_parm.fcd_parm.fcd_vlc_code[i] = p_ai_parm->conv.fcd.p_vlc_code[i];
			}
			for (i = 0; i < p_ai_parm->conv.fcd.vlc_valid_size; i++) {
				p_eng_parm->conv_parm.fcd_parm.fcd_vlc_valid[i] = p_ai_parm->conv.fcd.p_vlc_valid[i];
			}
			for (i = 0; i < p_ai_parm->conv.fcd.vlc_ofs_size; i++) {
				p_eng_parm->conv_parm.fcd_parm.fcd_vlc_ofs[i] = p_ai_parm->conv.fcd.p_vlc_ofs[i];
			}
		} else {
			memset(p_eng_parm->conv_parm.fcd_parm.fcd_vlc_code, 0, sizeof(p_eng_parm->conv_parm.fcd_parm.fcd_vlc_code));
			memset(p_eng_parm->conv_parm.fcd_parm.fcd_vlc_valid, 0, sizeof(p_eng_parm->conv_parm.fcd_parm.fcd_vlc_valid));
			memset(p_eng_parm->conv_parm.fcd_parm.fcd_vlc_ofs, 0, sizeof(p_eng_parm->conv_parm.fcd_parm.fcd_vlc_ofs));
		}

		if (p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_QUANTI_EN) {
			eng_func_en = eng_func_en | CNN_FCD_QUANTI_EN;
		}

		if (p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_SPARSE_EN) {
			eng_func_en = eng_func_en | CNN_FCD_SPARSE_EN;
		}

		if (p_ai_parm->conv.fcd.func_en & KDRV_AI_FCD_QUANTI_KMEANS_EN) {
			eng_func_en = eng_func_en | CNN_FCD_QUANTI_KMEANS_EN;
		}
		eng_func_en = eng_func_en | CNN_CONV_KERL_EN;
	} else {
		memcpy(&p_eng_parm->conv_parm, &def_cnn_conv, sizeof(CNN_CONV_PARM));
	}

	if (func_en & KDRV_AI_NEURAL_NORM_EN) {
		if (p_ai_parm->norm.mode != AI_NORM_BN_SCL) {
			nvt_dbg(ERR, "not support normalization mode: %d\r\n", p_ai_parm->norm.mode);
			return E_CTX;
		}
		//p_ai_parm->norm.bn_scl.bn_scale_addr;
		p_eng_parm->bnscale_parm.bn_shf_mean    = p_ai_parm->norm.bn_scl.mean_para_shf;
		p_eng_parm->bnscale_parm.scale_shf_bias = p_ai_parm->norm.bn_scl.bias_para_shf;
		p_eng_parm->bnscale_parm.scale_shf_alpha = p_ai_parm->norm.bn_scl.alpha_para_shf;
		eng_func_en = eng_func_en | CNN_BNSCALE_KERL_EN;
	} else {
		memcpy(&p_eng_parm->bnscale_parm, &def_cnn_bnscale, sizeof(CNN_BNSCALE_PARM));
	}

	if (func_en & KDRV_AI_NEURAL_ELT_EN) {
		p_eng_parm->elt_parm.elt_in_shift_dir = (p_ai_parm->elt.sclshf.in_shift < 0)?1:0;
		p_eng_parm->elt_parm.elt_in_shift     = (p_ai_parm->elt.sclshf.in_shift < 0)?(-1*p_ai_parm->elt.sclshf.in_shift):p_ai_parm->elt.sclshf.in_shift;
		p_eng_parm->elt_parm.elt_in_scale     = p_ai_parm->elt.sclshf.in_scale;
		p_eng_parm->elt_parm.elt_shf0   = p_ai_parm->elt.coef_shf0;
		p_eng_parm->elt_parm.elt_shf1   = p_ai_parm->elt.coef_shf1;
		p_eng_parm->elt_parm.elt_outshf = p_ai_parm->elt.norm_shf;
		p_eng_parm->elt_parm.elt_coef0  = p_ai_parm->elt.coef_scl0;
		p_eng_parm->elt_parm.elt_coef1  = p_ai_parm->elt.coef_scl1;
		p_eng_parm->elttype = kdrv_ai2cnn_io_type(p_ai_parm->elt.type);
		p_eng_parm->userdef.elt_mode  = (CNN_ELT_MODE_TYPE)p_ai_parm->elt.mode;
		if (p_ai_parm->elt.addr > 0)
			eng_func_en = eng_func_en | CNN_ELTWISE_KERL_EN;
	} else {
		memcpy(&p_eng_parm->elt_parm, &def_cnn_elt, sizeof(CNN_ELTWISE_PARM));
	}

	if (func_en & KDRV_AI_NEURAL_PREACT_EN) {
		if (p_ai_parm->preact.mode != AI_ACT_RELU && p_ai_parm->preact.mode != AI_ACT_TANH) {
			nvt_dbg(ERR, "not support activation mode: %d\r\n", p_ai_parm->preact.mode);
			return E_CTX;
		}
		p_eng_parm->relu_parm.pre_relu.relu_type  = (CNN_ACT_MODE_TYPE)p_ai_parm->preact.mode;
		p_eng_parm->relu_parm.pre_relu.leaky_val  = p_ai_parm->preact.relu.leaky_val;
		p_eng_parm->relu_parm.pre_relu.leaky_shf  = p_ai_parm->preact.relu.leaky_shf;
		p_eng_parm->relu_parm.pre_relu.negation   = p_ai_parm->preact.neg_en;
		p_eng_parm->userdef.act_mode = (CNN_ACT_MODE_TYPE)p_ai_parm->act.mode;
		eng_func_en = eng_func_en | CNN_PREACT_KERL_EN;
	} else {
		memcpy(&p_eng_parm->relu_parm.pre_relu, &def_cnn_act, sizeof(CNN_RELU_IN));
	}

	if (func_en & KDRV_AI_NEURAL_ACT_EN) {
		if (p_ai_parm->act.mode != AI_ACT_RELU && p_ai_parm->act.mode != AI_ACT_TANH) {
			nvt_dbg(ERR, "not support activation mode: %d\r\n", p_ai_parm->act.mode);
			return E_CTX;
		}
		if (p_ai_parm->act1.mode != AI_ACT_RELU && p_ai_parm->act1.mode != AI_ACT_TANH) {
			nvt_dbg(ERR, "not support activation mode: %d\r\n", p_ai_parm->act1.mode);
			return E_CTX;
		}
		p_eng_parm->relu_parm.relu0.relu_type = (CNN_ACT_MODE_TYPE)p_ai_parm->act.mode;
		p_eng_parm->relu_parm.relu0.leaky_val = p_ai_parm->act.relu.leaky_val;
		p_eng_parm->relu_parm.relu0.leaky_shf = p_ai_parm->act.relu.leaky_shf;
		p_eng_parm->relu_parm.relu0.negation  = p_ai_parm->act.neg_en;

		p_eng_parm->relu_parm.relu1.relu_type = (CNN_ACT_MODE_TYPE)p_ai_parm->act1.mode;
		p_eng_parm->relu_parm.relu1.leaky_val = p_ai_parm->act1.relu.leaky_val;
		p_eng_parm->relu_parm.relu1.leaky_shf = p_ai_parm->act1.relu.leaky_shf;
		p_eng_parm->relu_parm.relu1.negation  = p_ai_parm->act1.neg_en;

		p_eng_parm->userdef.act_mode = (CNN_ACT_MODE_TYPE)p_ai_parm->act.mode;

		eng_func_en = eng_func_en | CNN_ACT_KERL_EN;
	} else {
		memcpy(&p_eng_parm->relu_parm.relu0, &def_cnn_act, sizeof(CNN_RELU_IN));
		memcpy(&p_eng_parm->relu_parm.relu1, &def_cnn_act, sizeof(CNN_RELU_IN));
	}

	if (func_en & KDRV_AI_NEURAL_POOL_EN) {
		CNN_POOL_KER ker_type;
		if (p_ai_parm->pool.mode == AI_POOL_LOCAL_MAX) {
			ker_type = CNN_POOL_KER_MAX;
			p_eng_parm->userdef.pool_mode = CNN_POOLING_LOCAL;
		} else if (p_ai_parm->pool.mode == AI_POOL_LOCAL_AVG) {
			ker_type = CNN_POOL_KER_AVG;
			p_eng_parm->userdef.pool_mode = CNN_POOLING_LOCAL;
		} else if (p_ai_parm->pool.mode == AI_POOL_GLOBAL_MAX) {
			ker_type = CNN_POOL_KER_MAX;
			p_eng_parm->userdef.pool_mode = CNN_POOLING_GLOBAL;
		} else if (p_ai_parm->pool.mode == AI_POOL_GLOBAL_AVG) {
			ker_type = CNN_POOL_KER_AVG;
			p_eng_parm->userdef.pool_mode = CNN_POOLING_GLOBAL;
		} else {
			ker_type = CNN_POOL_KER_MAX;
			p_eng_parm->userdef.pool_mode = CNN_POOLING_LOCAL;
		}

		if (p_eng_parm->userdef.pool_mode == CNN_POOLING_LOCAL) {
			CNN_POOL_KER_STRIDE stride;
			CNN_POOL_KER_SIZE ker_size;
			UINT32 stride_x = 0, /*stride_y = 0,*/ ker_w = 0/*, ker_h = 0*/;
			stride_x = p_ai_parm->pool.local.ker_stridex;
			//stride_y = p_ai_parm->pool.local.ker_stridey;

			switch (stride_x) {
			case 1:
				stride = CNN_POOL_KER_STRIDE_1;
				break;
			case 2:
				stride = CNN_POOL_KER_STRIDE_2;
				break;
			default:
				nvt_dbg(WRN, "not support pool stride: %d, clamp to %d\r\n", (int)stride_x, 1);
				stride = CNN_POOL_KER_STRIDE_1;
				break;
			}

			ker_w = p_ai_parm->pool.local.ker_w;
			//ker_h = p_ai_parm->pool.local.ker_h;

			switch (ker_w) {
			case 2:
				ker_size = CNN_POOL_KERSZ_2;
				break;
			case 3:
				ker_size = CNN_POOL_KERSZ_3;
				break;
			case 4:
				ker_size = CNN_POOL_KERSZ_4;
				break;
			case 5:
				ker_size = CNN_POOL_KERSZ_5;
				break;
			default:
				nvt_dbg(WRN, "not support pool kernel size: %d, clamp to %d\r\n", (int)ker_w, 2);
				ker_size = CNN_POOL_KERSZ_2;
				break;
			}
			p_eng_parm->local_pool.ker_type     = ker_type;
			p_eng_parm->local_pool.stride       = stride;
			p_eng_parm->local_pool.ker_size     = ker_size;
			p_eng_parm->local_pool.is_top_pad   = p_ai_parm->pool.local.pad.top_pad_num > 0 ? TRUE : FALSE;
			p_eng_parm->local_pool.is_bot_pad   = p_ai_parm->pool.local.pad.bot_pad_num > 0 ? TRUE : FALSE;
			p_eng_parm->local_pool.is_left_pad  = p_ai_parm->pool.local.pad.left_pad_num > 0 ? TRUE : FALSE;
			p_eng_parm->local_pool.is_right_pad = p_ai_parm->pool.local.pad.right_pad_num > 0 ? TRUE : FALSE;
			p_eng_parm->local_pool.out_cal_type = (CNN_POOL_OUT_CAL_TYPE)p_ai_parm->pool.local.pool_cal_type;
			p_eng_parm->local_pool.ave_div_type = (CNN_POOL_AVE_DIV_TYPE)p_ai_parm->pool.local.pool_div_type;
			p_eng_parm->local_pool.pool_shf_dir = (p_ai_parm->pool.pool_shf < 0)?1:0;
			p_eng_parm->local_pool.pool_shf     = (p_ai_parm->pool.pool_shf < 0)?(-1)*p_ai_parm->pool.pool_shf:p_ai_parm->pool.pool_shf;
			p_eng_parm->local_pool.pool_signedness = ((p_eng_parm->out1type == CNN_UINT8) || (p_eng_parm->out1type == CNN_UINT16))?0:1;
		} else {
			memcpy(&p_eng_parm->local_pool, &def_cnn_localpool, sizeof(CNN_LOCAL_POOL_PARM));
		}

		if (p_eng_parm->userdef.pool_mode == CNN_POOLING_GLOBAL) {
			p_eng_parm->global_pool.ker_type      = ker_type;
			p_eng_parm->global_pool.avg_mul       = p_ai_parm->pool.global.avg_mul;
			p_eng_parm->global_pool.avg_shf       = p_ai_parm->pool.global.avg_shf;
			p_eng_parm->global_pool.ave_div_type  = 0; // todo: this parameter is necessary or not?
			p_eng_parm->global_pool.pool_shf_dir  = (p_ai_parm->pool.pool_shf < 0)?1:0;
			p_eng_parm->global_pool.pool_shf      = (p_ai_parm->pool.pool_shf < 0)?(-1)*p_ai_parm->pool.pool_shf:p_ai_parm->pool.pool_shf;
			p_eng_parm->global_pool.pool_signedness = ((p_eng_parm->out1type == CNN_UINT8) || (p_eng_parm->out1type == CNN_UINT16))?0:1;
		} else {
			memcpy(&p_eng_parm->global_pool, &def_cnn_globalpool, sizeof(CNN_GLOBAL_POOL_PARM));
		}

		eng_func_en = eng_func_en | CNN_POOLING_KERL_EN;
	} else {
		memcpy(&p_eng_parm->local_pool, &def_cnn_localpool, sizeof(CNN_LOCAL_POOL_PARM));
		memcpy(&p_eng_parm->global_pool, &def_cnn_globalpool, sizeof(CNN_GLOBAL_POOL_PARM));
	}

	if (func_en & KDRV_AI_NEURAL_DECONV_EN) {
		if (p_ai_parm->deconv.func_sel == AI_DECONV_SEL)
			p_eng_parm->userdef.eng_mode = CNN_DECONV;
		else if (p_ai_parm->deconv.func_sel == AI_UPSAMPLE_SEL)
			p_eng_parm->userdef.eng_mode = CNN_SCALEUP;
		p_eng_parm->userdef.out0_mode   = CNN_OUT0_RELU0;
	} else {
		p_eng_parm->userdef.eng_mode    = CNN_CONV;
		if (p_ai_parm->out_interm_dma_en) {
			p_eng_parm->userdef.out0_mode   = CNN_OUT0_INTERMEDIATE;
		} else {
			p_eng_parm->userdef.out0_mode   = CNN_OUT0_RELU0;
		}
	}
	p_eng_parm->userdef.func_en     = eng_func_en;
	p_eng_parm->userdef.io_enable   = io_en;

	return E_OK;
}

ER kdrv_ai_tran_roipool_parm(KDRV_AI_ROIPOOL_PARM *p_ai_parm, NUE_PARM *p_eng_parm)
{
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);
	p_eng_parm->insize.width    = p_ai_parm->size.width;
	p_eng_parm->insize.height   = p_ai_parm->size.height;
	p_eng_parm->insize.channel  = p_ai_parm->size.channel;
	p_eng_parm->dmaio_addr.addrin0      = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addrin_roi   = p_ai_parm->roi_ker.roi_addr;
	p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_addr;
	p_eng_parm->roipool_parm.mode       = (NUE_ROI_MODE)p_ai_parm->roi_ker.mode;
	p_eng_parm->roipool_parm.roi_num    = p_ai_parm->roi_ker.roi_num;
	p_eng_parm->roipool_parm.olofs      = p_ai_parm->roi_ker.out_ofs.batch_ofs;

	if (p_ai_parm->roi_ker.mode == AI_ROIPOOL_ORIGINAL) {
		if ((p_ai_parm->roi_ker.pool_w == 6) && (p_ai_parm->roi_ker.pool_h == 6)) {
			p_eng_parm->roipool_parm.size = NUE_ROI_POOL_SIZE_6;
		} else if ((p_ai_parm->roi_ker.pool_w == 7) && (p_ai_parm->roi_ker.pool_h == 7)) {
			p_eng_parm->roipool_parm.size = NUE_ROI_POOL_SIZE_7;
		} else if ((p_ai_parm->roi_ker.pool_w == 14) && (p_ai_parm->roi_ker.pool_h == 14)) {
			p_eng_parm->roipool_parm.size = NUE_ROI_POOL_SIZE_14;
		} else {
			nvt_dbg(WRN, "not support roi pool size (%d, %d) in roipooling mode %d\r\n", p_ai_parm->roi_ker.pool_w, p_ai_parm->roi_ker.pool_h, p_ai_parm->roi_ker.mode);
			nvt_dbg(WRN, "auto set roi pool size (%d, %d)\r\n", 6, 6);
			p_eng_parm->roipool_parm.size = NUE_ROI_POOL_SIZE_6;
		}
	} else if (p_ai_parm->roi_ker.mode == AI_PS_ROIPOOL) {
		if ((p_ai_parm->roi_ker.pool_w == 1) && (p_ai_parm->roi_ker.pool_h == 1)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_1;
		} else if ((p_ai_parm->roi_ker.pool_w == 2) && (p_ai_parm->roi_ker.pool_h == 2)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_2;
		} else if ((p_ai_parm->roi_ker.pool_w == 3) && (p_ai_parm->roi_ker.pool_h == 3)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_3;
		} else if ((p_ai_parm->roi_ker.pool_w == 4) && (p_ai_parm->roi_ker.pool_h == 4)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_4;
		} else if ((p_ai_parm->roi_ker.pool_w == 5) && (p_ai_parm->roi_ker.pool_h == 5)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_5;
		} else if ((p_ai_parm->roi_ker.pool_w == 6) && (p_ai_parm->roi_ker.pool_h == 6)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_6;
		} else if ((p_ai_parm->roi_ker.pool_w == 7) && (p_ai_parm->roi_ker.pool_h == 7)) {
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_7;
		} else {
			nvt_dbg(WRN, "not support roi pool size (%d, %d) in roipooling mode %d\r\n", p_ai_parm->roi_ker.pool_w, p_ai_parm->roi_ker.pool_h, p_ai_parm->roi_ker.mode);
			nvt_dbg(WRN, "auto set roi pool size (%d, %d)\r\n", 1, 1);
			p_eng_parm->roipool_parm.size = NUE_PSROI_POOL_SIZE_1;
		}
	} else {
		nvt_dbg(WRN, "not support roi pool mode %d\r\n", p_ai_parm->roi_ker.mode);
		nvt_dbg(WRN, "auto set roi pool mode %d\r\n", 0);
		nvt_dbg(WRN, "auto set roi pool size (%d, %d)\r\n", 6, 6);
		p_eng_parm->roipool_parm.mode = NUE_NORMAL_ROI;
		p_eng_parm->roipool_parm.size = NUE_ROI_POOL_SIZE_6;
	}
	p_eng_parm->in_parm.in_shf    = p_ai_parm->roi_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  = p_ai_parm->roi_ker.sclshf.in_scale;
	p_eng_parm->roipool_parm.ratio_mul  = p_ai_parm->roi_ker.ratio_mul;
	p_eng_parm->roipool_parm.ratio_shf  = p_ai_parm->roi_ker.ratio_shf;
	p_eng_parm->roipool_parm.out_shf    = p_ai_parm->roi_ker.roipool_shf;
	p_eng_parm->userdef.eng_mode = NUE_ROIPOOLING;
	return E_OK;
}

ER kdrv_ai_tran_nue_fcd_parm(KDRV_AI_FCD_KERPARM *p_fcd_parm, NUE_PARM *p_eng_parm, UINT32 *p_func_en)
{
	UINT32 func_en = 0;
	UINT32 i = 0;

	if ((p_fcd_parm == NULL) || (p_eng_parm == NULL) || (p_func_en == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	p_eng_parm->svm_parm.fcd_parm.fcd_vlc_en            = p_fcd_parm->func_en & KDRV_AI_FCD_VLC_EN ? TRUE : FALSE;
	p_eng_parm->svm_parm.fcd_parm.fcd_quanti_en         = p_fcd_parm->func_en & KDRV_AI_FCD_QUANTI_EN ? TRUE : FALSE;
	p_eng_parm->svm_parm.fcd_parm.fcd_sparse_en         = p_fcd_parm->func_en & KDRV_AI_FCD_SPARSE_EN ? TRUE : FALSE;
	p_eng_parm->svm_parm.fcd_parm.fcd_quanti_kmean_en   = p_fcd_parm->func_en & KDRV_AI_FCD_QUANTI_KMEANS_EN ? TRUE : FALSE;
	p_eng_parm->svm_parm.fcd_parm.fcd_enc_bit_length    = p_fcd_parm->enc_bit_length;

	if (p_fcd_parm->func_en & KDRV_AI_FCD_VLC_EN) {
		func_en = func_en | NUE_FCD_VLC_EN;

		if ((p_fcd_parm->vlc_code_size != KDRV_AI_FCD_VLC_SIZE) || (p_fcd_parm->vlc_valid_size != KDRV_AI_FCD_VLC_SIZE)
			|| (p_fcd_parm->vlc_ofs_size != KDRV_AI_FCD_VLC_SIZE)) {
			nvt_dbg(WRN, "fcd vlc code size %d is clamp %d\r\n", (unsigned int)p_fcd_parm->vlc_code_size, KDRV_AI_FCD_VLC_SIZE);
			nvt_dbg(WRN, "fcd vlc valid size %d is clamp %d\r\n", (unsigned int)p_fcd_parm->vlc_valid_size, KDRV_AI_FCD_VLC_SIZE);
			nvt_dbg(WRN, "fcd vlc offset size %d is clamp %d\r\n", (unsigned int)p_fcd_parm->vlc_ofs_size, KDRV_AI_FCD_VLC_SIZE);
			p_fcd_parm->vlc_code_size   = KDRV_AI_FCD_VLC_SIZE;
			p_fcd_parm->vlc_valid_size  = KDRV_AI_FCD_VLC_SIZE;
			p_fcd_parm->vlc_ofs_size    = KDRV_AI_FCD_VLC_SIZE;
		}
		for (i = 0; i < p_fcd_parm->vlc_code_size; i++) {
			p_eng_parm->svm_parm.fcd_parm.fcd_vlc_code[i] = p_fcd_parm->p_vlc_code[i];
		}
		for (i = 0; i < p_fcd_parm->vlc_valid_size; i++) {
			p_eng_parm->svm_parm.fcd_parm.fcd_vlc_valid[i] = p_fcd_parm->p_vlc_valid[i];
		}
		for (i = 0; i < p_fcd_parm->vlc_ofs_size; i++) {
			p_eng_parm->svm_parm.fcd_parm.fcd_vlc_ofs[i] = p_fcd_parm->p_vlc_ofs[i];
		}
	} else {
		memset(p_eng_parm->svm_parm.fcd_parm.fcd_vlc_code, 0, sizeof(p_eng_parm->svm_parm.fcd_parm.fcd_vlc_code));
		memset(p_eng_parm->svm_parm.fcd_parm.fcd_vlc_valid, 0, sizeof(p_eng_parm->svm_parm.fcd_parm.fcd_vlc_valid));
		memset(p_eng_parm->svm_parm.fcd_parm.fcd_vlc_ofs, 0, sizeof(p_eng_parm->svm_parm.fcd_parm.fcd_vlc_ofs));
	}

	if (p_fcd_parm->func_en & KDRV_AI_FCD_QUANTI_EN) {
		func_en = func_en | NUE_FCD_QUANTI_EN;
	}

	if (p_fcd_parm->func_en & KDRV_AI_FCD_SPARSE_EN) {
		func_en = func_en | NUE_FCD_SPARSE_EN;
	}

	if (p_fcd_parm->func_en & KDRV_AI_FCD_QUANTI_KMEANS_EN) {
		func_en = func_en | NUE_FCD_QUANTI_KMEANS_EN;
	}

	*p_func_en = (*p_func_en) | func_en;
	return E_OK;
}

ER kdrv_ai_tran_svm_parm(KDRV_AI_SVM_PARM *p_ai_parm, NUE_PARM *p_eng_parm)
{
	UINT32 func_en = 0;

	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);
	p_eng_parm->insvm_size.insize   = p_ai_parm->ft_size;
	p_eng_parm->insvm_size.channel  = 1;
	p_eng_parm->insvm_size.objnum   = p_ai_parm->obj_num;
	p_eng_parm->insvm_size.sv_w     = p_ai_parm->svm_ker.sv_width;
	p_eng_parm->insvm_size.sv_h     = p_ai_parm->svm_ker.sv_height;

	p_eng_parm->dmaio_addr.addrin0          = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addrin_sv        = p_ai_parm->svm_ker.sv_addr;
	p_eng_parm->dmaio_addr.addrin_alpha     = p_ai_parm->svm_ker.alpha_addr;
	p_eng_parm->dmaio_addr.addrin_kq        = p_ai_parm->fcd.quanti_kmeans_addr;

	p_eng_parm->dmaio_addr.addr_out         = p_ai_parm->out_addr;

	p_eng_parm->svm_parm.in_rfh                 = 1;
	p_eng_parm->svm_parm.svmker_parms.ft_shf    = p_ai_parm->svm_ker.ft_shf;
	p_eng_parm->svm_parm.svmker_parms.gv        = p_ai_parm->svm_ker.gamma;
	p_eng_parm->svm_parm.svmker_parms.gv_shf    = p_ai_parm->svm_ker.gamma_shf;
	p_eng_parm->svm_parm.svmker_parms.coef      = p_ai_parm->svm_ker.coef;
	p_eng_parm->svm_parm.svmker_parms.degree    = p_ai_parm->svm_ker.degree;
	p_eng_parm->svm_parm.svmker_parms.rho       = p_ai_parm->svm_ker.rho;

	p_eng_parm->svm_parm.svmker_parms.rho_fmt   = p_ai_parm->svm_ker.rho_fmt;
	p_eng_parm->svm_parm.svmker_parms.alpha_shf = p_ai_parm->svm_ker.alpha_shf;
	p_eng_parm->in_parm.in_shf    = p_ai_parm->svm_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  = p_ai_parm->svm_ker.sclshf.in_scale;

	//FCD
	if (kdrv_ai_tran_nue_fcd_parm(&p_ai_parm->fcd, p_eng_parm, &func_en) != E_OK) {
		nvt_dbg(ERR, "kdrv_ai_tran_nue_fcd_parm fail\n");
	}


	//p_eng_parm->svm_parm.svmker_type    = p_ai_parm->svm_ker.ker_mode;
	switch (p_ai_parm->svm_ker.ker_mode) {
	case AI_SVMKER_LINEAR:
		func_en = func_en | NUE_SVM_CAL_EN;
		p_eng_parm->userdef.svm_userdef.kerl1type = NUE_SVMKER1_DOT;
		break;
	case AI_SVMKER_POLY:
		func_en = func_en | NUE_SVM_KER2_EN | NUE_SVM_CAL_EN;
		p_eng_parm->userdef.svm_userdef.kerl1type = NUE_SVMKER1_DOT;
		p_eng_parm->userdef.svm_userdef.kerl2type = NUE_SVMKER2_POLY;
		break;
	case AI_SVMKER_RBF:
		func_en = func_en | NUE_SVM_CAL_EN;
		p_eng_parm->userdef.svm_userdef.kerl1type = NUE_SVMKER1_RBF;
		break;
	case AI_SVMKER_SIGMOID:
		func_en = func_en | NUE_SVM_KER2_EN | NUE_SVM_CAL_EN;
		p_eng_parm->userdef.svm_userdef.kerl1type = NUE_SVMKER1_DOT;
		p_eng_parm->userdef.svm_userdef.kerl2type = NUE_SVMKER2_SIGMOID;
		break;
	case AI_SVMKER_INTER:
		func_en = func_en | NUE_SVM_CAL_EN;
		p_eng_parm->userdef.svm_userdef.kerl1type = NUE_SVMKER1_INTER;
		break;
	default:
		nvt_dbg(WRN, "unknown svm kernel type: %d\r\n", p_ai_parm->svm_ker.ker_mode);
		break;
	}
	p_eng_parm->userdef.func_en = func_en;

	p_eng_parm->userdef.svm_userdef.dmao_path   = NUE_OUT_RST;
	p_eng_parm->userdef.svm_userdef.rslttype    = NUE_SVMRST_FULL;
	p_eng_parm->userdef.svm_userdef.dmao_en     = FALSE;
	p_eng_parm->userdef.eng_mode = NUE_SVM;

	//set default value
	memcpy(&p_eng_parm->relu_parm, &def_nue_relu, sizeof(NUE_RELU_PARM));
	memcpy(&p_eng_parm->roipool_parm, &def_nue_roipool, sizeof(NUE_ROI_POOL_PARM));
	//memcpy(&p_eng_parm->globalpool_parm, &def_cnn_globalpool, sizeof(CNN_GLOBAL_POOL_PARM));
	//memcpy(&p_eng_parm->localpool_parm, &def_cnn_localpool, sizeof(CNN_LOCAL_POOL_PARM));
	memcpy(&p_eng_parm->reorg_parm, &def_nue_reorg, sizeof(NUE_REORG_PARM));
	memcpy(&p_eng_parm->anchor_parm, &def_nue_anchor, sizeof(NUE_ANCHOR_PARM));
	memcpy(&p_eng_parm->softmax_parm, &def_nue_softmax, sizeof(NUE_SOFTMAX_PARM));
	//memcpy(&p_eng_parm->elt_parm, &def_nue_elt, sizeof(NUE_ELTWISE_PARM));

	return E_OK;
}

ER kdrv_ai_tran_nue_fc_parm(KDRV_AI_FC_PARM *p_ai_parm, NUE_PARM *p_eng_parm, UINT32 func_en)
{
	UINT32 eng_func_en = 0;
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL) || (func_en == 0)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);

	p_eng_parm->insvm_size.insize   = p_ai_parm->size.width * p_ai_parm->size.height;
	p_eng_parm->insvm_size.channel  = p_ai_parm->size.channel;
	p_eng_parm->insvm_size.objnum   = p_ai_parm->batch_num;
	p_eng_parm->insvm_size.sv_w     = p_ai_parm->fc_ker.weight_w;
	p_eng_parm->insvm_size.sv_h     = p_ai_parm->fc_ker.weight_h;

	p_eng_parm->dmaio_addr.addrin0          = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addrin_sv        = p_ai_parm->fc_ker.weight_addr;
	p_eng_parm->dmaio_addr.addrin_rho       = p_ai_parm->fc_ker.bias_addr;
	p_eng_parm->dmaio_addr.addrin_kq        = p_ai_parm->fcd.quanti_kmeans_addr;
	if (p_ai_parm->in_interm_dma_en) {
		p_eng_parm->dmaio_addr.addrin1      = p_ai_parm->in_interm_addr;
		p_eng_parm->svm_parm.ilofs    		= p_ai_parm->in_interm_ofs.line_ofs;
	} else {
		p_eng_parm->dmaio_addr.addrin1      = 0;
		p_eng_parm->svm_parm.ilofs    		= 0;
	}

	if (p_ai_parm->out_interm_dma_en) {
		p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_interm_addr;
	} else {
		p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_addr;
	}

	p_eng_parm->svm_parm.in_rfh                 = 1;
	p_eng_parm->svm_parm.svmker_parms.ft_shf    = p_ai_parm->fc_ker.weight_shf;
	p_eng_parm->svm_parm.svmker_parms.gv        = p_ai_parm->fc_ker.norm_scl;
	p_eng_parm->svm_parm.svmker_parms.gv_shf    = p_ai_parm->fc_ker.acc_shf;
	p_eng_parm->svm_parm.svmker_parms.coef      = 0;
	p_eng_parm->svm_parm.svmker_parms.degree    = 1;
	p_eng_parm->svm_parm.svmker_parms.rho       = 0;
	p_eng_parm->svm_parm.svmker_parms.rho_fmt   = p_ai_parm->fc_ker.norm_shf;
	p_eng_parm->svm_parm.svmker_parms.alpha_shf = 0;
	p_eng_parm->in_parm.in_shf    = p_ai_parm->fc_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  = p_ai_parm->fc_ker.sclshf.in_scale;

	//FCD
	if (kdrv_ai_tran_nue_fcd_parm(&p_ai_parm->fcd, p_eng_parm, &eng_func_en) != E_OK) {
		nvt_dbg(ERR, "kdrv_ai_tran_nue_fcd_parm fail\n");
	}


	if (func_en & KDRV_AI_FC_ACT_EN) {
		p_eng_parm->relu_parm.relu_type = NUE_RELU_LEAKY;
		p_eng_parm->relu_parm.leaky_val = p_ai_parm->act.relu.leaky_val;
		p_eng_parm->relu_parm.leaky_shf = p_ai_parm->act.relu.leaky_shf;
	} else {
		memcpy(&p_eng_parm->relu_parm, &def_nue_relu, sizeof(NUE_RELU_PARM));
	}
	p_eng_parm->relu_parm.out_shf   = p_ai_parm->act.act_shf0;

	if (p_ai_parm->src_fmt == AI_FC_SRC_BATCH_INTERLACE) {
		eng_func_en = eng_func_en | NUE_SVM_INTERLACE_EN;
	}

	if (func_en & KDRV_AI_FC_ACT_EN) {
		eng_func_en = eng_func_en | NUE_RELU_EN;
	}

	if (p_ai_parm->in_interm_dma_en) {
		eng_func_en = eng_func_en | NUE_SVM_INTERMEDIATE_IN_EN | NUE_SVM_SCI_NOTATION_EN;
	}


	//p_eng_parm->svm_parm.svmker_type    = p_ai_parm->svm_ker.ker_mode;
	p_eng_parm->userdef.svm_userdef.kerl1type = NUE_SVMKER1_DOT;
	p_eng_parm->userdef.func_en = eng_func_en;
	p_eng_parm->userdef.svm_userdef.dmao_path = p_ai_parm->out_interm_dma_en;
	/*
	if (p_ai_parm->out_interm_dma_en) {
		p_eng_parm->userdef.svm_userdef.dmao_path = NUE_OUT_INTERMEDIATE;
	} else {
		p_eng_parm->userdef.svm_userdef.dmao_path = NUE_OUT_RST;
	}
	*/
	p_eng_parm->userdef.svm_userdef.rslttype = NUE_SVMRST_SUB_RHO;
	p_eng_parm->userdef.svm_userdef.dmao_en = TRUE;
	p_eng_parm->userdef.eng_mode = NUE_SVM;

	//set default value
	memcpy(&p_eng_parm->roipool_parm, &def_nue_roipool, sizeof(NUE_ROI_POOL_PARM));
	memcpy(&p_eng_parm->reorg_parm, &def_nue_reorg, sizeof(NUE_REORG_PARM));
	memcpy(&p_eng_parm->anchor_parm, &def_nue_anchor, sizeof(NUE_ANCHOR_PARM));
	memcpy(&p_eng_parm->softmax_parm, &def_nue_softmax, sizeof(NUE_SOFTMAX_PARM));

	return E_OK;
}

ER kdrv_ai_tran_permute_parm(KDRV_AI_PERMUTE_PARM *p_ai_parm, NUE_PARM *p_eng_parm)
{
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init userdef
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);
	p_eng_parm->insize.width    = p_ai_parm->size.width;
	p_eng_parm->insize.height   = p_ai_parm->size.height;
	p_eng_parm->insize.channel  = p_ai_parm->size.channel;
	p_eng_parm->dmaio_addr.addrin0      = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_addr;
	p_eng_parm->permute_parm.mode 		= p_ai_parm->perm_ker.mode;
	p_eng_parm->permute_parm.out_shf	= p_ai_parm->perm_ker.permute_shf;
	p_eng_parm->in_parm.in_shf    		= p_ai_parm->perm_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  		= p_ai_parm->perm_ker.sclshf.in_scale;
	p_eng_parm->permute_parm.olofs        = p_ai_parm->out_ofs.line_ofs;
	p_eng_parm->userdef.permute_stripe_en = p_ai_parm->out_ofs.ch_stripe_en;

	p_eng_parm->userdef.eng_mode = NUE_PERMUTE;

	memcpy(&p_eng_parm->anchor_parm, &def_nue_anchor, sizeof(NUE_ANCHOR_PARM));
	memcpy(&p_eng_parm->softmax_parm, &def_nue_softmax, sizeof(NUE_SOFTMAX_PARM));
	return E_OK;
}

ER kdrv_ai_tran_reorg_parm(KDRV_AI_REORG_PARM *p_ai_parm, NUE_PARM *p_eng_parm)
{
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init userdef
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);
	p_eng_parm->insize.width    = p_ai_parm->size.width;
	p_eng_parm->insize.height   = p_ai_parm->size.height;
	p_eng_parm->insize.channel  = p_ai_parm->size.channel;
	p_eng_parm->dmaio_addr.addrin0      = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_addr;
	p_eng_parm->reorg_parm.out_shf      = p_ai_parm->reorg_ker.reorg_shf;
	p_eng_parm->in_parm.in_shf    		= p_ai_parm->reorg_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  		= p_ai_parm->reorg_ker.sclshf.in_scale;
	p_eng_parm->userdef.eng_mode        = NUE_REORGANIZATION;

	memcpy(&p_eng_parm->anchor_parm, &def_nue_anchor, sizeof(NUE_ANCHOR_PARM));
	memcpy(&p_eng_parm->softmax_parm, &def_nue_softmax, sizeof(NUE_SOFTMAX_PARM));

	return E_OK;
}

ER kdrv_ai_tran_anchor_parm(KDRV_AI_ANCHOR_PARM *p_ai_parm, NUE_PARM *p_eng_parm)
{
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init userdef
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);
	p_eng_parm->insize.width    = p_ai_parm->size.width;
	p_eng_parm->insize.height   = p_ai_parm->size.height;
	p_eng_parm->insize.channel  = p_ai_parm->size.channel;
	p_eng_parm->dmaio_addr.addrin0      = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_addr;
	p_eng_parm->dmaio_addr.addrin_sv    = p_ai_parm->w_addr;
	p_eng_parm->dmaio_addr.addrin_alpha = p_ai_parm->tbl_addr;
	p_eng_parm->dmaio_addr.addrin_rho   = p_ai_parm->b_addr;
	p_eng_parm->anchor_parm.at_table_update = 1;
	p_eng_parm->anchor_parm.at_w_shf    = p_ai_parm->anchor_ker.anchor_w_shf;
	p_eng_parm->in_parm.in_shf    		= p_ai_parm->anchor_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  		= p_ai_parm->anchor_ker.sclshf.in_scale;
	p_eng_parm->userdef.eng_mode = NUE_ANCHOR;

	memcpy(&p_eng_parm->softmax_parm, &def_nue_softmax, sizeof(NUE_SOFTMAX_PARM));
	return E_OK;
}

ER kdrv_ai_tran_softmax_parm(KDRV_AI_SOFTMAX_PARM *p_ai_parm, NUE_PARM *p_eng_parm)
{
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init userdef
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE_DMAIO_ADDR));
	memset(&p_eng_parm->userdef, 0, sizeof(NUE_USER_DEF));

	p_eng_parm->intype  = kdrv_ai2nue_io_type(p_ai_parm->in_type);
	p_eng_parm->outtype = kdrv_ai2nue_io_type(p_ai_parm->out_type);
	p_eng_parm->insize.width    = p_ai_parm->size.width;
	p_eng_parm->insize.height   = p_ai_parm->size.height;
	p_eng_parm->insize.channel  = p_ai_parm->size.channel;
	p_eng_parm->dmaio_addr.addrin0      = p_ai_parm->in_addr;
	p_eng_parm->dmaio_addr.addr_out     = p_ai_parm->out_addr;

	p_eng_parm->softmax_parm.in_shf     = p_ai_parm->softmax_ker.softmax_in_shf;
	p_eng_parm->softmax_parm.out_shf    = p_ai_parm->softmax_ker.softmax_out_shf;
	p_eng_parm->softmax_parm.group_num  = p_ai_parm->size.channel; // same as channel
	p_eng_parm->softmax_parm.set_num    = 1; 					   // channel_num / group_num
	p_eng_parm->in_parm.in_shf    		= p_ai_parm->softmax_ker.sclshf.in_shift;
	p_eng_parm->in_parm.in_scale  		= p_ai_parm->softmax_ker.sclshf.in_scale;
	p_eng_parm->userdef.eng_mode = NUE_SOFTMAX;

	memcpy(&p_eng_parm->anchor_parm, &def_nue_anchor, sizeof(NUE_ANCHOR_PARM));

	return E_OK;
}

ER kdrv_ai_tran_nue2_preproc_parm(KDRV_AI_PREPROC_PARM *p_ai_parm, NUE2_PARM *p_eng_parm, UINT32 func_en)
{
	if ((p_ai_parm == NULL) || (p_eng_parm == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return E_NOEXS;
	}

	//init
	memset(&p_eng_parm->dmaio_addr, 0, sizeof(NUE2_DMAIO_ADDR));
	memset(&p_eng_parm->func_en, 0, sizeof(NUE2_FUNC_EN));

	//DMA IN/OUT address
	p_eng_parm->dmaio_addr.addr_in0  = p_ai_parm->in_addr[0];
	p_eng_parm->dmaio_addr.addr_in1  = p_ai_parm->in_addr[1];
	p_eng_parm->dmaio_addr.addr_in2  = p_ai_parm->in_addr[2];
	p_eng_parm->dmaio_addr.addr_out0 = p_ai_parm->out_addr[0];
	p_eng_parm->dmaio_addr.addr_out1 = p_ai_parm->out_addr[1];
	p_eng_parm->dmaio_addr.addr_out2 = p_ai_parm->out_addr[2];

	//Format
#if AI_SUPPORT_MULTI_FMT
	// in 52x : not support NV21
	if (p_ai_parm->src_fmt == AI_PREPROC_SRC_YUV420_NV21) {
		nvt_dbg(ERR, "not support NV21 format!\n");
		return E_NOEXS;
	}
#endif
	p_eng_parm->infmt = (NUE2_IN_FMT)p_ai_parm->src_fmt;
	p_eng_parm->outfmt.out_signedness = (BOOL)((p_ai_parm->out_type & 0x1)?0:1);

	//Input size
	p_eng_parm->insize.in_width  = p_ai_parm->in_size.width;
	p_eng_parm->insize.in_height = p_ai_parm->in_size.height;

	//Line offset
	p_eng_parm->dmaio_lofs.in0_lofs  = p_ai_parm->in_ofs[0].line_ofs;
	p_eng_parm->dmaio_lofs.in1_lofs  = p_ai_parm->in_ofs[1].line_ofs;
	p_eng_parm->dmaio_lofs.in2_lofs  = p_ai_parm->in_ofs[2].line_ofs;
	p_eng_parm->dmaio_lofs.out0_lofs = p_ai_parm->out_ofs[0].line_ofs;
	p_eng_parm->dmaio_lofs.out1_lofs = p_ai_parm->out_ofs[1].line_ofs;
	p_eng_parm->dmaio_lofs.out2_lofs = p_ai_parm->out_ofs[2].line_ofs;
	
	// Mean scale shift
	p_eng_parm->mean_shift_parm.mean_scale     = 1;
	p_eng_parm->mean_shift_parm.mean_shift 	   = p_ai_parm->sub_ker.sub_shf;
	p_eng_parm->mean_shift_parm.mean_shift_dir = 0;

	//YUV2RGB
	if (func_en & KDRV_AI_PREPROC_YUV2RGB_EN) {
		p_eng_parm->func_en.yuv2rgb_en = 1;
	}

	//Scaling
	p_eng_parm->scale_parm.h_filtmode   = 0;
	p_eng_parm->scale_parm.v_filtmode   = 0;
	p_eng_parm->scale_parm.h_filtcoef   = 0;
	p_eng_parm->scale_parm.v_filtcoef   = 0;
	p_eng_parm->scale_parm.h_scl_size   = p_ai_parm->scale_ker.scl_out_size.width;
	p_eng_parm->scale_parm.v_scl_size   = p_ai_parm->scale_ker.scl_out_size.height;
	if (p_ai_parm->scale_ker.scl_out_size.width == 1 || p_ai_parm->scale_ker.scl_out_size.height == 1) {
		nvt_dbg(ERR, "invalid scaling output size: %d x %d, bypass scaling\r\n", p_ai_parm->scale_ker.scl_out_size.width, p_ai_parm->scale_ker.scl_out_size.height);
		p_eng_parm->scale_parm.h_dnrate     = 0;
		p_eng_parm->scale_parm.v_dnrate     = 0;
		p_eng_parm->scale_parm.h_sfact      = 0;
		p_eng_parm->scale_parm.v_sfact      = 0;
	} else {
		p_eng_parm->scale_parm.h_dnrate     = (p_ai_parm->in_size.width-1)/(p_ai_parm->scale_ker.scl_out_size.width-1) - 1;
		p_eng_parm->scale_parm.v_dnrate     = (p_ai_parm->in_size.height-1)/(p_ai_parm->scale_ker.scl_out_size.height-1) - 1;
		p_eng_parm->scale_parm.h_sfact      = ((p_ai_parm->in_size.width-1)*65536/(p_ai_parm->scale_ker.scl_out_size.width-1)) & 0xffff;
		p_eng_parm->scale_parm.v_sfact      = ((p_ai_parm->in_size.height-1)*65536/(p_ai_parm->scale_ker.scl_out_size.height-1)) & 0xffff;
	}
	p_eng_parm->scale_parm.fact_update = p_ai_parm->scale_ker.fact_update_en;

	//Mean subtraction
	if (func_en & KDRV_AI_PREPROC_SUB_EN) {
		p_eng_parm->func_en.sub_en         = 1;
		p_eng_parm->sub_parm.sub_mode      = (BOOL)p_ai_parm->sub_ker.sub_mode;
		p_eng_parm->sub_parm.sub_in_width  = p_ai_parm->sub_ker.sub_in_w;
		p_eng_parm->sub_parm.sub_in_height = p_ai_parm->sub_ker.sub_in_h;
		p_eng_parm->sub_parm.sub_coef0     = p_ai_parm->sub_ker.sub_dc_coef[0];
		p_eng_parm->sub_parm.sub_coef1     = p_ai_parm->sub_ker.sub_dc_coef[1];
		p_eng_parm->sub_parm.sub_coef2     = p_ai_parm->sub_ker.sub_dc_coef[2];
		p_eng_parm->sub_parm.sub_dup       = (NUE2_SUBDUP_RATE)p_ai_parm->sub_ker.dup_rate;
		p_eng_parm->sub_parm.sub_shift     = p_ai_parm->sub_ker.sub_shf;
	}

	//Padding
	if (func_en & KDRV_AI_PREPROC_PAD_EN) {
		p_eng_parm->func_en.pad_en               = 1;
		p_eng_parm->pad_parm.pad_crop_x          = p_ai_parm->pad_ker.crop_x;
		p_eng_parm->pad_parm.pad_crop_y          = p_ai_parm->pad_ker.crop_y;
		p_eng_parm->pad_parm.pad_crop_width      = p_ai_parm->pad_ker.crop_w;
		p_eng_parm->pad_parm.pad_crop_height     = p_ai_parm->pad_ker.crop_h;
		p_eng_parm->pad_parm.pad_crop_out_x      = p_ai_parm->pad_ker.pad_out_x;
		p_eng_parm->pad_parm.pad_crop_out_y      = p_ai_parm->pad_ker.pad_out_y;
		p_eng_parm->pad_parm.pad_crop_out_width  = p_ai_parm->pad_ker.pad_out_w;
		p_eng_parm->pad_parm.pad_crop_out_height = p_ai_parm->pad_ker.pad_out_h;
		p_eng_parm->pad_parm.pad_val0            = p_ai_parm->pad_ker.pad_val[0];
		p_eng_parm->pad_parm.pad_val1            = p_ai_parm->pad_ker.pad_val[1];
		p_eng_parm->pad_parm.pad_val2            = p_ai_parm->pad_ker.pad_val[2];
	} else {
		memcpy(&p_eng_parm->pad_parm, &def_nue2_pad, sizeof(NUE2_PAD_PARM));
	}

	//HSV not support in AI sdk
	p_eng_parm->func_en.hsv_en         = 0;
	p_eng_parm->hsv_parm.hsv_out_mode  = 0;
	p_eng_parm->hsv_parm.hsv_hue_shift = 0;

	//Rotate
	if (func_en & KDRV_AI_PREPROC_ROT_EN) {
		p_eng_parm->func_en.rotate_en       = 1;
		p_eng_parm->rotate_parm.rotate_mode = (NUE2_ROT_DEG)p_ai_parm->rotate_ker.rot_mode;
	} else {
        p_eng_parm->func_en.rotate_en       = 0;
		p_eng_parm->rotate_parm.rotate_mode = 0;
    }
	
	//flip
	p_eng_parm->flip_parm.flip_mode = 0;

	return E_OK;
}
