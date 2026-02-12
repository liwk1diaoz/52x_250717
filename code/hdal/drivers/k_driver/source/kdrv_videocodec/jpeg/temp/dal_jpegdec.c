/**
    @file       dal_jpegdec.c
    @ingroup	mICodec

    @brief      jpegdec device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "../hal/jpg_int.h" //to do
#include "../../include/jpg_dec.h"


ER dal_jpegdec_init(DAL_VDODEC_ID id, DAL_VDODEC_INIT *pinit) {

	if (jpg_async_dec_open() != E_OK) {
		DBG_ERR("[MJPGDEC] Jpg_ASyncDecOpen fail.\r\n");
		return E_SYS;
	}

	return E_OK;
}

ER dal_jpegdec_getinfo(DAL_VDODEC_ID id, DAL_VDODEC_GET_ITEM item, UINT32 *pvalue) {
	ER resultV = E_PAR;

	switch (item) {
	default:
		//DBG_ERR("[jpegDEC][%d] Get Info error! item=%d\r\n", (int)(id), (int)(item));
		break;
	}

	return resultV;
}

ER dal_jpegdec_setinfo(DAL_VDODEC_ID id, DAL_VDODEC_SET_ITEM item, UINT32 value) {
	switch (item) {

	default:
		//DBG_ERR("[jpegDEC][%d] Set Info error! item=%d\r\n", (int)(id), (int)(item));
		break;
	}

	return E_OK;
}

ER dal_jpegdec_decodeone(DAL_VDODEC_ID id, DAL_VDODEC_PARAM *pparam) {
	UINT16 *pJPGSOI;
	UINT8 *pCheck;
	JPGHEAD_DEC_CFG jdcfg = {0};
	JPG_DEC_INFO MjpgInfo = {0};

	pJPGSOI = (UINT16 *)pparam->uiBsAddr;
	pCheck = (UINT8 *)pparam->uiBsAddr;
	if ((*pCheck++ != 0xFF) || (*pCheck != 0xD8)) {
		DBG_ERR("[MJPGDEC] error JPG header\r\n");
		return E_SYS;
	}

	// check raw addr 4 aligned
	if ((pparam->uiRawYAddr % 4 != 0) || (pparam->uiRawUVAddr % 4 != 0)) {
		DBG_ERR("[MJPGDEC] raw addr should be 4 aligned, y=0x%08x, uv=0x%08x\r\n",
				(unsigned int)pparam->uiRawYAddr, (unsigned int)pparam->uiRawUVAddr);
		return E_SYS;
	}

	jdcfg.inbuf = (UINT8 *)pJPGSOI;
	jdcfg.rstinterval = 0;

	// Parse current frame for getting JPEG marker
	if (jpg_parse_header(&jdcfg, DEC_PRIMARY, NULL) != JPG_HEADER_ER_OK) {
		DBG_ERR("[MJPGDEC] parse header error\r\n");
		return E_SYS;
	}

	//MjpgInfo.pJPGScalarHandler	= NULL;
	MjpgInfo.p_src_addr			= (UINT8 *)pparam->uiBsAddr;
	MjpgInfo.p_dst_addr			= (UINT8 *)pparam->uiRawYAddr;
	MjpgInfo.p_dec_cfg			= &jdcfg;
	MjpgInfo.decode_type			= DEC_PRIMARY;
	MjpgInfo.enable_timeout		= FALSE;
	//MjpgInfo.bEnableCrop		= FALSE;
	MjpgInfo.jpg_file_size		= pparam->uiBsSize;

	if (jpg_async_dec_start(&MjpgInfo) != E_OK) {
		DBG_ERR("[MJPGDEC] dec fail.\r\n");
		return E_SYS;
	}

	pparam->uiWidth = MjpgInfo.p_dec_cfg->imagewidth;
	pparam->uiHeight = MjpgInfo.p_dec_cfg->imageheight;

	if (jpg_async_dec_waitdone() != E_OK) {
		DBG_ERR("[MJPGDEC] wait dec done fail.\r\n");
		return E_SYS;
	}

	return E_OK;
}

ER dal_jpegdec_close(DAL_VDODEC_ID id) {

	jpg_async_dec_close();

	return E_OK;
}

#if defined __KERNEL__
EXPORT_SYMBOL(dal_jpegdec_init);
EXPORT_SYMBOL(dal_jpegdec_getinfo);
EXPORT_SYMBOL(dal_jpegdec_setinfo);
EXPORT_SYMBOL(dal_jpegdec_decodeone);
EXPORT_SYMBOL(dal_jpegdec_close);
#endif

