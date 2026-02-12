/**
	@brief Source code of bitstream demuxer module.\n
	This file contains the functions which is related to bitstream demuxer in the chip.

	@file hd_bsdemux.c

	@ingroup mlib

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

// TODO: temp for [520] build
#ifdef _BSP_NA51000_
#undef _BSP_NA51000_
#endif
#ifndef _BSP_NA51055_
#define _BSP_NA51055_
#endif

#include <string.h>
#include "hdal.h"
#include "kwrap/debug.h"
#define HD_MODULE_NAME HD_BSDEMUX
#include "hd_bsdemux_lib.h"
#include "nvt_media_interface.h"
#include "bsdemux.h"
#include "nmediaplay_api.h"
#if defined(__LINUX)
#include <semaphore.h>
#else
#include "kwrap/semaphore.h"
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_BSDEMUX_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

// refer to isf_flow_def.h
#define ISF_IN_BASE     128
#define ISF_OUT_BASE  	0

#define ISF_OUT_BASE  	0
#define DEV_BASE        0 //ISF_UNIT_VDODEC
#define DEV_COUNT       1
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        16
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       16

// refer to hd_type.h::videodec module
#define HD_DAL_BSDEMUX_COUNT	16
#define HD_DAL_BSDEMUX_BASE	0x3000
#define HD_DAL_BSDEMUX(did)	(HD_DAL_BSDEMUX_BASE + (did))
#define HD_DAL_BSDEMUX_MAX	(HD_DAL_BSDEMUX_BASE + HD_DAL_BSDEMUX_COUNT - 1)

#define HD_DEV_BASE	HD_DAL_BSDEMUX_BASE
#define HD_DEV_MAX	HD_DAL_BSDEMUX_MAX

#define LIB_BSDEMUX_PARAM_BEGIN     NMP_BSDEMUX_PARAM_START
#define LIB_BSDEMUX_PARAM_END       NMP_BSDEMUX_PARAM_MAX
#define LIB_BSDEMUX_PARAM_NUM      (LIB_BSDEMUX_PARAM_END - LIB_BSDEMUX_PARAM_BEGIN)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef enum _HD_BSDEMUX_STAT {
	HD_BSDEMUX_STATUS_UNINIT = 0,
	HD_BSDEMUX_STATUS_INIT,
	HD_BSDEMUX_STATUS_OPEN,
	//HD_BSDEMUX_STATUS_START,
	//HD_BSDEMUX_STATUS_STOP,
	HD_BSDEMUX_STATUS_CLOSE,
} HD_BSDEMUX_STAT;

typedef struct _BSDEMUX_INFO_PRIV {
	HD_BSDEMUX_STAT        status;                     ///< hd_bsdemux_status
	BOOL                    b_maxmem_set;
	BOOL                    b_need_update_param[LIB_BSDEMUX_PARAM_NUM];
#if defined(__LINUX)
	sem_t                   sem_id;
#else
	ID                      sem_id;
#endif
} BSDEMUX_INFO_PRIV;

typedef struct _BSDEMUX_INFO_PORT {
	HD_BSDEMUX_PATH_CONFIG   path_cfg;
	HD_BSDEMUX_IN            in_param;
	HD_BSDEMUX_FILE          file_param;
	HD_BSDEMUX_REG_CALLBACK  reg_cb;

	//private data
	BSDEMUX_INFO_PRIV          priv;
} BSDEMUX_INFO_PORT;
	
typedef struct _BSDEMUX_INFO {
		BSDEMUX_INFO_PORT *port;
} BSDEMUX_INFO;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
BSDEMUX_INFO bsdemux_info[DEV_COUNT] = {0};
NMI_UNIT     *p_nmi_bsdemux = NULL;
static BOOL bsdemux_debug = FALSE; // debug message open/close
static UINT32 _max_path_count = 2; // default 2, need to use _hd_bsdemux_cfg_max to config [ToDo]
// prevent AddUnit over flow.
static UINT32           gISF_BsDemux_AddUnit = 0;


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void _hd_bsdemux_cfg_max(UINT32 maxpath)
{
	if (!bsdemux_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > NMP_BSDEMUX_MAX_PATH) _max_path_count = NMP_BSDEMUX_MAX_PATH;
	}
}

INT _hd_bsdemux_check_status(HD_PATH_ID path_id, HD_BSDEMUX_STAT status)
{
	INT ret = 0;

	if (bsdemux_info[0].port[path_id].priv.status != status) {
		return -1;
	}

	return ret;
}

UINT32 _hd_bsdemux_file_fmt_hdal2unit(HD_BSDEMUX_FILE_FMT h_file_fmt)
{
	switch (h_file_fmt)
	{
		case HD_BSDEMUX_FILE_FMT_MP4:    return MEDIA_FILEFORMAT_MP4;
		case HD_BSDEMUX_FILE_FMT_TS:     return MEDIA_FILEFORMAT_TS;
		default:
			DBG_ERR("unknown file fmt(%d)\r\n", h_file_fmt);
			return (-1);
	}
}

NMP_BSDEMUX_BS_TPYE _hd_bsdemux_bs_type_hdal2unit(HD_BSDEMUX_BS_TPYE h_bs_type)
{
	switch (h_bs_type)
	{
		case HD_BSDEMUX_BS_TPYE_VIDEO:    return NMP_BSDEMUX_BS_TPYE_VIDEO;
		case HD_BSDEMUX_BS_TPYE_AUDIO:     return NMP_BSDEMUX_BS_TPYE_AUDIO;
		default:
			DBG_ERR("unknown bs type(%d)\r\n", h_bs_type);
			return (-1);
	}
}

HD_VIDEO_CODEC _hd_bsdemux_vcodec_hdal2unit(HD_VIDEO_CODEC h_vcodec)
{
	switch (h_vcodec)
	{
		case HD_CODEC_TYPE_JPEG:    return NMEDIAPLAY_DEC_MJPG;
		case HD_CODEC_TYPE_H264:    return NMEDIAPLAY_DEC_H264;
		case HD_CODEC_TYPE_H265:    return NMEDIAPLAY_DEC_H265;
		default:
			DBG_ERR("unknown video codec(%d)\r\n", h_vcodec);
			return (-1);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

HD_RESULT hd_bsdemux_init(VOID)
{
	UINT32 dev, port;
	UINT32 i;

	DBG_IND("hd_bsdemux_init\n");

	if (bsdemux_info[0].port != NULL) {
		DBG_ERR("already init\n");
		return HD_ERR_UNINIT;
	}

	if (_max_path_count == 0) {
		DBG_ERR("_max_path_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	bsdemux_info[0].port = (BSDEMUX_INFO_PORT*)malloc(sizeof(BSDEMUX_INFO_PORT)*_max_path_count);
	if (bsdemux_info[0].port == NULL) {
		DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
		return HD_ERR_HEAP;
	}
	for(i = 0; i < _max_path_count; i++) {
		memset(&(bsdemux_info[0].port[i]), 0, sizeof(BSDEMUX_INFO_PORT));  // no matter what status now...reset status = [UNINIT] anyway
	}

	// add & get nmi_bsdemux
	if (gISF_BsDemux_AddUnit==0) {
		NMP_BsDemux_AddUnit();
	    gISF_BsDemux_AddUnit = 1;
	}	
	if ((p_nmi_bsdemux = NMI_GetUnit("BsDemux")) == NULL) {
		DBG_ERR("Get BsDemux unit failed\r\n");
		return HD_ERR_NG;
	}

	// init sxcmd
	sxcmd_addtable(bsdemux);

	// set status [UNINT] -> [INIT]
	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			bsdemux_info[dev].port[port].priv.status = HD_BSDEMUX_STATUS_INIT;
		}
	}

	return HD_OK;
}

HD_RESULT hd_bsdemux_open(HD_PATH_ID path_id)
{
	ER r;

	DBG_IND("hd_bsdemux_open:\n");
	DBG_IND("path_id(0x%x)\n", path_id);

	// error checking
	if (bsdemux_info[0].port[path_id].priv.status == HD_BSDEMUX_STATUS_OPEN) {
		DBG_ERR("path %d has already open!\r\n", path_id);
		return HD_ERR_ALREADY_OPEN;
	}
	if (!p_nmi_bsdemux || !p_nmi_bsdemux->Open) {
		DBG_ERR("p_nmi_bsdemux is NULL\r\n");
		return HD_ERR_NG;
	}

	// init variable
#if defined(__LINUX)
	sem_init(&bsdemux_info[0].port[path_id].priv.sem_id, 0, 0);
#else
	r = vos_sem_create(&bsdemux_info[0].port[path_id].priv.sem_id, 1, "SEMID_HD_BSDEMUX");
	if(r == -1) {
		DBG_ERR("bsdemux vos_sem_create error!\r\n");
	}
#endif
	
	// open
	r = p_nmi_bsdemux->Open(path_id, 0);
	if (r != E_OK) {
		DBG_ERR("bsdemux open fail(%d)\r\n", r);
		return HD_ERR_NG;
	}

	// set status [INIT] -> [OPEN]
	bsdemux_info[0].port[path_id].priv.status = HD_BSDEMUX_STATUS_OPEN;

	return HD_OK;
}

HD_RESULT hd_bsdemux_close(HD_PATH_ID path_id)
{
	int r;

	DBG_IND("hd_bsdemux_close:\n");
	DBG_IND("    path_id(0x%x)\n", path_id);

	if(p_nmi_bsdemux && bsdemux_info[0].port)
	{

		// close
		r = p_nmi_bsdemux->Close(path_id);
		if (r != E_OK) {
			DBG_ERR("bsdemux close fail(%d)\r\n", r);
			return HD_ERR_NG;
		}

		// reset HD_BSDEMUX_PARAM_PATH_CONFIG
		{
			HD_BSDEMUX_PATH_CONFIG *p_path_cfg = &bsdemux_info[0].port[path_id].path_cfg;
			NMP_BSDEMUX_MAX_MEM_INFO max_mem = {0};

			p_path_cfg->file_fmt = 0;
			p_path_cfg->container = 0;
			DBG_IND("path_config: file_fmt(%u), container(0x%lx)\n", p_path_cfg->file_fmt, (UINT32)p_path_cfg->container);

			max_mem.file_fmt = p_path_cfg->file_fmt;

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_MAX_MEM_INFO, (UINT32)&max_mem);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_CONTAINER, (UINT32)p_path_cfg->container);
		}

		// reset HD_BSDEMUX_PARAM_IN
		{
			HD_BSDEMUX_IN* p_in_param = &bsdemux_info[0].port[path_id].in_param;

			p_in_param->video_codec = 0;
			p_in_param->video_width = 0;
			p_in_param->video_height = 0;
			p_in_param->video_en = 0;
			p_in_param->audio_en = 0;
			p_in_param->decrypt_key = 0;
			DBG_IND("param_in: v_codec(%u) w(%d) h(%d) v_en(%u) a_en(%u)\n",
				p_in_param->video_codec, p_in_param->video_width, p_in_param->video_height, p_in_param->video_en, p_in_param->audio_en);
			DBG_IND("param_in: decypt_key(0x%x)\n", p_in_param->decrypt_key);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_CODEC, (UINT32)p_in_param->video_codec);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_WID, p_in_param->video_width);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_HEI, p_in_param->video_height);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_ENABLE, p_in_param->video_en);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_AUD_ENABLE, p_in_param->audio_en);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_DECRYPT, p_in_param->decrypt_key);
		}

		// reset HD_BSDEMUX_PARAM_FILE
		{
			HD_BSDEMUX_FILE* p_file_param = &bsdemux_info[0].port[path_id].file_param;

			p_file_param->buf_start_addr = 0;
			p_file_param->blk_size = 0;
			p_file_param->tt_blknum = 0;
			DBG_IND("param_file: buf_start_addr(0x%lx) blk_size(0x%llx) tt_blknum(0x%lx)\n",
					p_file_param->buf_start_addr, p_file_param->blk_size, p_file_param->tt_blknum);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_BUF_STARTADDR, p_file_param->buf_start_addr);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_BLK_SIZE, (UINT32)&p_file_param->blk_size);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_BLK_TTNUM, p_file_param->tt_blknum);
		}

		// reset HD_BSDEMUX_PARAM_REG_CALLBACK
		{
			HD_BSDEMUX_REG_CALLBACK* p_reg_cb = &bsdemux_info[0].port[path_id].reg_cb;

			p_reg_cb->callbackfunc = NULL;
			DBG_IND("reg_callback: cb_func(0x%lx)\n", (UINT32)p_reg_cb->callbackfunc);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_REG_CB, (UINT32)p_reg_cb->callbackfunc);
		}

		if(bsdemux_info[0].port[path_id].priv.sem_id){

	#if defined(__LINUX)
			sem_destroy(&bsdemux_info[0].port[path_id].priv.sem_id);
	#else
			vos_sem_destroy(bsdemux_info[0].port[path_id].priv.sem_id);
	#endif
			bsdemux_info[0].port[path_id].priv.sem_id = 0;
		}

		bsdemux_info[0].port[path_id].priv.status = HD_BSDEMUX_STATUS_CLOSE;
	}

	return HD_OK;
}

HD_RESULT hd_bsdemux_uninit(VOID)
{
	UINT32 dev, port;

	DBG_IND("hd_bsdemux_uninit\n");

	if (bsdemux_info[0].port == NULL) {
		DBG_ERR("NOT init yet\n");
		return HD_ERR_INIT;
	}

	// set status [CLOSE] -> [UNINIT]
	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			bsdemux_info[dev].port[port].priv.status = HD_BSDEMUX_STATUS_UNINIT;
		}
	}

	//uninit lib
	{
		free(bsdemux_info[0].port);
		bsdemux_info[0].port = NULL;
	}

	return HD_OK;
}

HD_RESULT hd_bsdemux_get(HD_PATH_ID path_id, HD_BSDEMUX_PARAM_ID id, VOID* p_param)
{
	HD_RESULT rv;

	DBG_IND("hd_bsdemux_get(%d):\n", id);
	DBG_IND("    path_id(0x%x) ", path_id);
	
	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	switch(id) {
	case HD_BSDEMUX_PARAM_DEVCOUNT:
		{
			HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
			memset(p_user, 0, sizeof(HD_DEVCOUNT));
			p_user->max_dev_count = 1;

			DBG_IND("max_dev_cnt(%u)\n", p_user->max_dev_count);

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_SYSCAPS:
		{
			HD_BSDEMUX_SYSCAPS* p_user = (HD_BSDEMUX_SYSCAPS*)p_param;
			int  i;
			memset(p_user, 0, sizeof(HD_BSDEMUX_SYSCAPS));
			p_user->dev_id        = 0;
			p_user->chip          = 0;
			p_user->max_in_count  = 16;
			p_user->max_out_count = 16;
			p_user->dev_caps      = HD_CAPS_PATHCONFIG;
			for (i=0; i<16; i++) p_user->in_caps[i] =
				HD_BSDEMUX_IN_CAPS_MP4
				 | HD_BSDEMUX_IN_CAPS_TS;
			for (i=0; i<16; i++) p_user->out_caps[i] =
				HD_BSDEMUX_OUT_CAPS_VIDEO
				 | HD_BSDEMUX_OUT_CAPS_AUDIO;

			DBG_IND("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip, p_user->max_in_count, p_user->max_out_count, p_user->dev_caps);
			{
				int i;
				DBG_IND("         in_caps    , out_caps =>\n");
				for (i=0; i<16; i++) {
					DBG_IND("    [%02d] 0x%08x , 0x%08x\n", i, p_user->in_caps[i], p_user->out_caps[i]);
				}
			}

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_IN:
		{
			HD_BSDEMUX_IN *p_user = (HD_BSDEMUX_IN*)p_param;
			HD_BSDEMUX_IN *p_in_param = &bsdemux_info[0].port[path_id].in_param;
			memcpy(p_user, p_in_param, sizeof(HD_BSDEMUX_IN));

			DBG_IND("param_in: v_codec(%u) w(%d) h(%d) gop(%lu) desc_size(%lu) v_en(%u) a_en(%u)\n",
					p_in_param->video_codec, p_in_param->video_width, p_in_param->video_height,
					p_in_param->video_gop, p_in_param->desc_size,
					p_in_param->video_en, p_in_param->audio_en);

			DBG_IND("param_in: decypt_key(0x%x)\n", p_user->decrypt_key);
			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_PATH_CONFIG:
		{
			HD_BSDEMUX_PATH_CONFIG *p_user = (HD_BSDEMUX_PATH_CONFIG*)p_param;
			HD_BSDEMUX_PATH_CONFIG *p_path_cfg = &bsdemux_info[0].port[path_id].path_cfg;
			memcpy(p_user, p_path_cfg, sizeof(HD_BSDEMUX_PATH_CONFIG));

			DBG_IND("file_fmt(%u)\n", p_user->file_fmt);
			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_REG_CALLBACK:
		{
			HD_BSDEMUX_REG_CALLBACK *p_user = (HD_BSDEMUX_REG_CALLBACK*)p_param;
			HD_BSDEMUX_REG_CALLBACK *p_reg_cb = &bsdemux_info[0].port[path_id].reg_cb;
			memcpy(p_user, p_reg_cb, sizeof(HD_BSDEMUX_REG_CALLBACK));

			DBG_IND("reg_cb(0x%lx)\n", (UINT32)p_user->callbackfunc);
			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_FILESIZE:
		{
			p_nmi_bsdemux->GetParam(path_id, NMP_BSDEMUX_PARAM_FILESIZE, (UINT32*)p_param);
			rv = HD_OK;
		}
		break;
	default:
		{
			DBG_ERR("Invalid id = 0x%x\r\n", id);
			rv = HD_ERR_PARAM;
		}
		break;
	}

	return rv;
}

HD_RESULT hd_bsdemux_set(HD_PATH_ID path_id, HD_BSDEMUX_PARAM_ID id, VOID* p_param)
{
	HD_RESULT rv;

	DBG_IND("hd_bsdemux_set(%d):\n", id);
	DBG_IND("    path_id(0x%x) ", path_id);

	rv = HD_OK;
#if 0
	if(p_param == NULL) {
		DBG_ERR("p_param is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
#endif

	if (!p_nmi_bsdemux || !p_nmi_bsdemux->SetParam) {
		DBG_ERR("p_nmi_bsdemux is null\r\n");
		return HD_ERR_NULL_PTR;
	}
	
	switch(id) {
	case HD_BSDEMUX_PARAM_PATH_CONFIG:
		{
			HD_BSDEMUX_PATH_CONFIG* p_user = (HD_BSDEMUX_PATH_CONFIG*)p_param;
			HD_BSDEMUX_PATH_CONFIG *p_path_cfg = &bsdemux_info[0].port[path_id].path_cfg;
			NMP_BSDEMUX_MAX_MEM_INFO max_mem = {0};

			memcpy(p_path_cfg, p_user, sizeof(HD_BSDEMUX_PATH_CONFIG));

			DBG_IND("path_config: file_fmt(%u), container(0x%lx)\n", p_path_cfg->file_fmt, (UINT32)p_path_cfg->container);

			max_mem.file_fmt = _hd_bsdemux_file_fmt_hdal2unit(p_path_cfg->file_fmt);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_MAX_MEM_INFO, (UINT32)&max_mem);
			if (p_path_cfg->container) {
				p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_CONTAINER, (UINT32)p_path_cfg->container);
			}

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_IN:
		{
			HD_BSDEMUX_IN* p_user = (HD_BSDEMUX_IN*)p_param;
			HD_BSDEMUX_IN* p_in_param = &bsdemux_info[0].port[path_id].in_param;

			memcpy(p_in_param, p_user, sizeof(HD_BSDEMUX_IN));

			DBG_IND("param_in: v_codec(%u) w(%d) h(%d) gop(%lu) desc_size(%lu) v_en(%u) a_en(%u)\n",
					p_in_param->video_codec, p_in_param->video_width, p_in_param->video_height,
					p_in_param->video_gop, p_in_param->desc_size,
					p_in_param->video_en, p_in_param->audio_en);

			p_in_param->video_codec = _hd_bsdemux_vcodec_hdal2unit(p_in_param->video_codec);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_CODEC, (UINT32)p_in_param->video_codec);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_WID, p_in_param->video_width);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_HEI, p_in_param->video_height);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_GOP, p_in_param->video_gop);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_DESC_SIZE, p_in_param->desc_size);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_ENABLE, p_in_param->video_en);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_AUD_ENABLE, p_in_param->audio_en);
			if (p_in_param->decrypt_key) {
				DBG_IND("param_in: decypt_key(0x%x)\n", p_in_param->decrypt_key);
				p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_VDO_DECRYPT, p_in_param->decrypt_key);
			}

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_FILE:
		{
			HD_BSDEMUX_FILE* p_user = (HD_BSDEMUX_FILE*)p_param;
			HD_BSDEMUX_FILE* p_file_param = &bsdemux_info[0].port[path_id].file_param;

			memcpy(p_file_param, p_user, sizeof(HD_BSDEMUX_FILE));

			DBG_IND("param_file: buf_start_addr(0x%lx) blk_size(0x%llx) tt_blknum(0x%lx)\n",
					p_file_param->buf_start_addr, p_file_param->blk_size, p_file_param->tt_blknum);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_BUF_STARTADDR, p_file_param->buf_start_addr);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_BLK_SIZE, (UINT32)&p_file_param->blk_size);
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_BLK_TTNUM, p_file_param->tt_blknum);

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_REG_CALLBACK:
		{
			HD_BSDEMUX_REG_CALLBACK* p_user = (HD_BSDEMUX_REG_CALLBACK*)p_param;
			HD_BSDEMUX_REG_CALLBACK* p_reg_cb = &bsdemux_info[0].port[path_id].reg_cb;

			memcpy(p_reg_cb, p_user, sizeof(HD_BSDEMUX_REG_CALLBACK));

			DBG_IND("reg_callback: cb_func(0x%lx)\n", (UINT32)p_reg_cb->callbackfunc);

			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_REG_CB, (UINT32)p_reg_cb->callbackfunc);

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_TRIG_TSDMX:
		{
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_TRIG_TSDMX, (UINT32)p_param);

			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_FILESIZE:
		{
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_FILESIZE, (UINT32)p_param);
			rv = HD_OK;
		}
		break;

	case HD_BSDEMUX_PARAM_TSDMX_BUFBLK:
		{
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_TSDMX_BUFBLK, (UINT32)p_param);
			rv = HD_OK;
		}
		break;
	case HD_BSDEMUX_PARAM_TSDMX_BUF_RESET:
		{
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_TSDMX_BUF_RESET, (UINT32)p_param);
			rv = HD_OK;
		}
		break;
	case HD_BSDEMUX_PARAM_FILE_REMAIN_SIZE:
		{
			p_nmi_bsdemux->SetParam(path_id, NMP_BSDEMUX_PARAM_FILE_REMAIN_SIZE, (UINT32)p_param);
			rv = HD_OK;
		}
		break;
	default:
		{
			DBG_ERR("invalid id=%d\r\n", id);
			rv = HD_ERR_PARAM;
		}
		break;
	}

	return rv;
}

void _hd_bsdemux_dewrap_buf(NMP_BSDEMUX_IN_BUF *p_data, HD_BSDEMUX_BUF* p_in_bsdemux_buf)
{
	p_data->bs_type = _hd_bsdemux_bs_type_hdal2unit(p_in_bsdemux_buf->bs_type);
	p_data->index = p_in_bsdemux_buf->index;
}

HD_RESULT hd_bsdemux_push_in_buf(HD_PATH_ID path_id, HD_BSDEMUX_BUF* p_in_bsdemux_buf, INT32 wait_ms)
{
	NMP_BSDEMUX_IN_BUF data = {0};

	// error checking
	if (p_in_bsdemux_buf == NULL) {
		DBG_ERR("p_in_bsdemux_bs is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	if (bsdemux_info[0].port[path_id].priv.status != HD_BSDEMUX_STATUS_OPEN) {
		DBG_ERR("push_in_buf could only be called after OPEN, please call hd_bsdemux_open() first\r\n");
		return HD_ERR_NOT_START;
	}

#if defined(__LINUX)
	sem_post(&bsdemux_info[0].port[path_id].priv.sem_id); // sem +1
#else
	vos_sem_sig(bsdemux_info[0].port[path_id].priv.sem_id);
#endif

	if (p_in_bsdemux_buf->sign == MAKEFOURCC('B','S','D','X')) {
		_hd_bsdemux_dewrap_buf(&data, p_in_bsdemux_buf);
		p_nmi_bsdemux->In(path_id, (UINT32 *)&data);
	}


#if defined(__LINUX)
	sem_wait(&bsdemux_info[0].port[path_id].priv.sem_id); // sem -1
#else
	vos_sem_wait(bsdemux_info[0].port[path_id].priv.sem_id);
#endif

	return HD_OK;
}

//
// test commands
//
static BOOL cmd_bsdemux_set_debug(CHAR *str_cmd)
{
	UINT32 dbg_msg;

	sscanf_s(str_cmd, "%d", &dbg_msg);

	bsdemux_debug = (dbg_msg > 0);

	return TRUE;
}

static BOOL cmd_bsdemux_startup(CHAR *str_cmd)
{
	return TRUE;
}

SXCMD_BEGIN(bsdemux, "hd_bsdemux_lib module")
SXCMD_ITEM("dbg %d",		cmd_bsdemux_set_debug,		"set debug msg (Param: 1->on, 0->off)")
SXCMD_ITEM("startup %d",	cmd_bsdemux_startup,		"test startup process (path_id: 0~16)")
SXCMD_END()

