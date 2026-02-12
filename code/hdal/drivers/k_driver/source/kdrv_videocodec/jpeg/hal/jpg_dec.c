/*
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       jpgdec.c
    @ingroup    mIAVJPEG

    @brief      Jpeg decoder

*/
#if 0
#include <string.h>

#include "SysKer.h"
#include "jpeg.h"
#include "JpgDec.h"
#include "JpgHeader.h"
#include "top.h"
#include "jpgint.h"
#include "jpeg_lmt.h"
#else
#include "jpg_int.h"
#include "../../include/jpg_dec.h"
#include "../../include/jpeg.h"
#include "jpeg_lmt.h"
#endif
#include "../jpeg_platform.h"


#define JPEG_FPGA_TIME_OUT_CNT  200

static JPG_DEC_INFO m_DecJPG;
static BOOL  m_IsASyncDecOpened = FALSE;
/** \addtogroup mIAVJPEG
*/
//@{

void jpg_dec_trans_size(UINT16 *p_width, UINT16 *p_height, UINT16 *p_loftY, UINT16 *p_loftuv, JPG_YUV_FORMAT  fileformat)
{
	DBG_IND("++WxH=0x%X x 0x%X, LineOffset Y=0x%x, UV=0x%x, fileformat=%d\r\n", (unsigned int)(*p_width), (unsigned int)(*p_height), (unsigned int)(*p_loftY), (unsigned int)(*p_loftuv), (int)(fileformat));
	*p_width = ALIGN_CEIL_16(*p_width);
	if (fileformat == JPG_FMT_YUV420) {
		*p_height = ALIGN_CEIL(*p_height, JPEG_H_ALIGN_MCU_411);
	} else {
		*p_height = ALIGN_CEIL(*p_height, JPEG_H_ALIGN_MCU_2H11);
	}


	*p_loftY = *p_width;
	*p_loftuv = *p_width;

	*p_loftY = ALIGN_CEIL(*p_loftY, JPEG_DEC_Y_LINEOFFSET_ALIGN);
	*p_loftuv = ALIGN_CEIL(*p_loftuv, JPEG_DEC_UV_LINEOFFSET_ALIGN);

	DBG_IND("--WxH=0x%X x 0x%X, LineOffset Y=0x%x, UV=0x%x, fileformat=%d\r\n", (unsigned int)(*p_width), (unsigned int)(*p_height), (unsigned int)(*p_loftY), (unsigned int)(*p_loftuv), (int)(fileformat));
}
ER jpg_dec_one(PJPG_DEC_INFO p_dec_info)
{
	ER E_Ret;
	jpg_async_dec_open();
	E_Ret = jpg_async_dec_start(p_dec_info);
	if (E_OK == E_Ret) {
		E_Ret = jpg_async_dec_waitdone();
	}
	jpg_async_dec_close();
	return E_Ret;
}
ER jpg_async_dec_open(void)
{
	if (m_IsASyncDecOpened) {
		DBG_ERR("Jpg_ASyncDec has opened ! \r\n");
		return E_SYS;
	}
	m_IsASyncDecOpened = TRUE;
	jpeg_set_config(JPEG_CONFIG_ID_CHECK_DEC_ERR, TRUE);
	return jpeg_open();
}
ER jpg_async_dec_start(PJPG_DEC_INFO p_dec_info)
{
	UINT32  ErrorMessage, BitstreamStartAddr;
	UINT32  DataYSize, DataUVSize;
	PJPGHEAD_DEC_CFG jdcfg_p;
	UINT32  tmpWidth, tmpHeight;
	UINT32  uiUserJPGDecoded_Y, uiUserJPGDecoded_Cb, uiUserJPGDecoded_Cr;
	if (FALSE == m_IsASyncDecOpened) {
		DBG_ERR("Jpg_ASyncDec NOT opened yet! \r\n");
		return E_SYS;
	}
	if (!p_dec_info->jpg_file_size) {
		DBG_ERR("Input Bitstream NG! \r\n");
		return E_CTX;
	}


	jdcfg_p     = p_dec_info->p_dec_cfg;
	jdcfg_p->inbuf = (UINT8 *)p_dec_info->p_src_addr;
	//#NT#2008/07/02#Scottie -begin
	//#NT#2008/07/02#Scottie -end
	//#NT#2008/12/03#Scottie -begin
	//#NT#Add for new Screennail decoding mechanism
	jdcfg_p->speedup_sn = p_dec_info->speed_up_sn;
	//#NT#2008/12/03#Scottie -end

	// (1).Parse JPG Header  // 1->parse thumb, 0->primary, 2->AVI
	ErrorMessage = (UINT32)jpg_parse_header(jdcfg_p, p_dec_info->decode_type, p_dec_info->parse_exif_cb);

	if (ErrorMessage) {
//#NT#2009/12/12#Scottie -begin
//#NT#Return error code according to jpg_parse_header() result
		//return E_OBJ;
		return ErrorMessage;
//#NT#2009/12/12#Scottie -end
	}
	tmpWidth  = jdcfg_p->imagewidth;
	tmpHeight = jdcfg_p->imageheight;
	jdcfg_p->ori_imagewidth  = tmpWidth;
	jdcfg_p->ori_imageheight = tmpHeight;

	//#NT#2010/09/01#Daniel -begin
	//#Use JPEG_YUV_FORMAT_xxx macro instead of JPG_HW_FMT_xxx for jpeg_set_format()
	if (jdcfg_p->fileformat == JPG_FMT_YUV211) {   // 0x21->YUV211
		tmpWidth = ALIGN_CEIL(tmpWidth, JPEG_W_ALIGN_MCU_2H11);
		tmpHeight = ALIGN_CEIL(tmpHeight, JPEG_H_ALIGN_MCU_2H11);
		jpeg_set_format(tmpWidth, tmpHeight, JPEG_YUV_FORMAT_211);
		DataYSize  = tmpWidth * tmpHeight;
		jdcfg_p->lineoffset_y = tmpWidth;
		jdcfg_p->lineoffset_uv = tmpWidth;


		DataUVSize = (DataYSize >> 1);
	} else if (jdcfg_p->fileformat == JPG_FMT_YUV420) { // 0x22->YUV420
		tmpWidth = ALIGN_CEIL(tmpWidth, JPEG_W_ALIGN_MCU_411);
		tmpHeight = ALIGN_CEIL(tmpHeight, JPEG_H_ALIGN_MCU_411);

		jpeg_set_format(tmpWidth, tmpHeight, JPEG_YUV_FORMAT_411);
		DataYSize = tmpWidth * tmpHeight;
		jdcfg_p->lineoffset_y = tmpWidth;
		jdcfg_p->lineoffset_uv = tmpWidth;
		DataUVSize = (DataYSize >> 2);
	}
	/*
	//#NT#2009/09/22#Daniel -begin
	else if (jdcfg_p->fileformat == JPG_FMT_YUV100) { // 0x11->YUV100
		if ((tmpWidth % 8) != 0) {
			tmpWidth  = ((tmpWidth + 7) & 0xfffffff8);
		}
		if ((tmpHeight % 8) != 0) {
			tmpHeight = ((tmpHeight + 7) & 0xfffffff8);
		}

		jpeg_set_format(tmpWidth, tmpHeight, JPEG_YUV_FORMAT_100);
		DataYSize = tmpWidth * tmpHeight;
		jdcfg_p->lineoffset_y = tmpWidth;
		jdcfg_p->lineoffset_uv = 0;
		DataUVSize = 0;
	}
	//#NT#2009/09/22#Daniel -begin
	// YUV444 NOT support
	else if(jdcfg_p->fileformat==JPG_FMT_YUV444) // 0x11->YUV444
	{
	    if((tmpWidth % 16) != 0)
	        tmpWidth  = ( (tmpWidth + 15) & 0xfffffff0 );
	    if((tmpHeight % 16) != 0)
	        tmpHeight = ( (tmpHeight + 15) & 0xfffffff0 );

	    jpeg_set_formatHW(tmpWidth, tmpHeight, JPEG_YUV_FORMAT_222);
	    DataYSize  = (tmpWidth * tmpHeight);
	    jdcfg_p->lineoffset_y = (tmpWidth);
	    jdcfg_p->lineoffset_uv= (tmpWidth);
	    DataUVSize = DataYSize;
	}*/
	//#NT#2010/09/01#Daniel -end
	else {
		// Reset jdcfg_p object for avoiding getting wrong jpeg info
		memset(jdcfg_p, 0, sizeof(JPGHEAD_DEC_CFG));
		return E_OBJ;
	}

	// maybe changed
	//jdcfg_p->imagewidth = tmpWidth;
	//jdcfg_p->imageheight= tmpHeight;

	jpeg_set_imglineoffset(jdcfg_p->lineoffset_y, jdcfg_p->lineoffset_uv, jdcfg_p->lineoffset_uv);

	jpeg_set_cropdisable();

	jpeg_set_scaledisable();

	// independent global
	//uiUserJPGDecoded_Y  = ((UINT32)p_dec_info->pDstAddr + 31) & 0xFFFFFFE0;
	//uiUserJPGDecoded_Cb = uiUserJPGDecoded_Y  + ((DataYSize +31)&0xFFFFFFE0);
	//uiUserJPGDecoded_Cr = uiUserJPGDecoded_Cb + ((DataUVSize+31)&0xFFFFFFE0);
	uiUserJPGDecoded_Y  = JPGDEC_IMG_ADDR_ALIGN((UINT32)p_dec_info->p_dst_addr);
	uiUserJPGDecoded_Cb = JPGDEC_IMG_ADDR_ALIGN(uiUserJPGDecoded_Y  + DataYSize);
	uiUserJPGDecoded_Cr = JPGDEC_IMG_ADDR_ALIGN(uiUserJPGDecoded_Cb + DataUVSize);


	BitstreamStartAddr = (UINT32)jdcfg_p->inbuf + jdcfg_p->headerlen;
	// set QTab's values to system
	jpeg_set_hwqtable((UINT8 *)jdcfg_p->p_q_tbl_y, (UINT8 *)jdcfg_p->p_q_tbl_uv);
	// set Huffman table's values to system
	jpeg_set_decode_hufftabhw(jdcfg_p->p_huff_dc0th, jdcfg_p->p_huff_dc1th, jdcfg_p->p_huff_ac0th, jdcfg_p->p_huff_ac1th);
	// set Restart Interval
	if (jdcfg_p->rstinterval) {
		jpeg_set_restartinterval(jdcfg_p->rstinterval);
		jpeg_set_restartenable(TRUE);
	} else {
		jpeg_set_restartenable(FALSE);
	}
	// set jpg enable interupt
	jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_DECERR | JPEG_INT_BUFEND);
	// set jpg decoded data addr
	jpeg_set_imgstartaddr((UINT32)uiUserJPGDecoded_Y, (UINT32)uiUserJPGDecoded_Cb, (UINT32)uiUserJPGDecoded_Cr);
	// set jpg bitstream addr
	jpeg_set_bsstartaddr(BitstreamStartAddr, p_dec_info->jpg_file_size);

	//clr_flg(FLG_ID_INT, FLGPTN_JPEG);
//#NT#2009/11/30#Scottie -begin
//#NT#Remove useless code
//    jpeg_get_status();
//#NT#2009/11/30#Scottie -end
	// start jpg decoding
	jpeg_set_startdecode();

	// independent global
	p_dec_info->out_addr_y  = uiUserJPGDecoded_Y;
	p_dec_info->out_addr_uv = uiUserJPGDecoded_Cb;


	memcpy((void *) &m_DecJPG, (const void *) p_dec_info, sizeof(JPG_DEC_INFO));

	return E_OK;
}
#define _FPGA_EMULATION_ 0 //temp, to do
ER jpg_async_dec_waitdone(void)
{
	BOOL    isTimeout = FALSE;
	UINT32  uiFlag;
#if !_FPGA_EMULATION_
	PJPGHEAD_DEC_CFG jdcfg_p;
#endif
	if (FALSE == m_IsASyncDecOpened) {
		DBG_ERR("Jpg_ASyncDec NOT opened yet! \r\n");
		return E_SYS;
	}
#if !_FPGA_EMULATION_
	jdcfg_p = m_DecJPG.p_dec_cfg;
#endif
	if ((m_DecJPG.enable_timeout) /*&& ((jdcfg_p->ori_imageheight != tmpHeight) || jdcfg_p->numcomp == 1)*/) {
		UINT32 i, count;

		isTimeout = TRUE;
//#NT#2009/06/24#Scottie -begin
//#NT#Assign the time of waitting for JPEG done according to JPG clock
//        count = 300;
#if _FPGA_EMULATION_
		count = JPEG_FPGA_TIME_OUT_CNT;
#else
#if 0
		switch (jpeg_get_config(JPEG_CONFIG_ID_FREQ)) {
		case 160:
			count = jdcfg_p->ori_imagewidth * jdcfg_p->ori_imageheight * 2 / 1600000;
			break;

		case 120:
			count = jdcfg_p->ori_imagewidth * jdcfg_p->ori_imageheight * 2 / 1200000;
			break;

		case 192:
			count = jdcfg_p->ori_imagewidth * jdcfg_p->ori_imageheight * 2 / 1920000;
			break;

		case 80:
			count = jdcfg_p->ori_imagewidth * jdcfg_p->ori_imageheight * 2 / 800000;
			break;

		case 240:
			count = jdcfg_p->ori_imagewidth * jdcfg_p->ori_imageheight * 2 / 2400000;
			break;

		default:
			DBG_WRN("Check JPEG timeout setting!\r\n");
			count = 50;
			break;
		}
#else
		count = jdcfg_p->ori_imagewidth * jdcfg_p->ori_imageheight * 2 / (jpeg_get_config(JPEG_CONFIG_ID_FREQ) * 10000);
#endif
#endif
//        count++;
		count += 10;
//#NT#2009/06/24#Scottie -end
		if (m_DecJPG.timer_start_cb) {
			m_DecJPG.timer_start_cb();
		}
		for (i = count; i > 0; i--) {
			if (jpeg_waitdone_polling()) {
				isTimeout = FALSE;
				break;
			}
			if (m_DecJPG.timer_wait_cb) {
				m_DecJPG.timer_wait_cb();
			}
		}

		if (isTimeout == TRUE) {
			DBG_ERR("Decode timeout \r\n");
		}

		uiFlag = jpeg_get_status();
		if (m_DecJPG.timer_pause_cb) {
			m_DecJPG.timer_pause_cb();
		}
	} else {
		uiFlag = jpeg_waitdone();
	}
	jpeg_set_enddecode();
	if (!(uiFlag & JPEG_INT_FRAMEEND) || (uiFlag & JPEG_INT_DECERR)) {
		return E_SYS;
	} else {
		return E_OK;
	}
}
ER jpg_async_dec_close(void)
{
	if (FALSE == m_IsASyncDecOpened) {
		DBG_ERR("Jpg_ASyncDec NOT opened yet! \r\n");
		return E_SYS;
	}
	m_IsASyncDecOpened = FALSE;
	return jpeg_close();
}

#if defined __UITRON || defined __ECOS
#elif defined __KERNEL__
EXPORT_SYMBOL(jpg_dec_trans_size);
EXPORT_SYMBOL(jpg_dec_one);
EXPORT_SYMBOL(jpg_async_dec_open);
EXPORT_SYMBOL(jpg_async_dec_start);
EXPORT_SYMBOL(jpg_async_dec_waitdone);
EXPORT_SYMBOL(jpg_async_dec_close);
#else

#endif
//@}
