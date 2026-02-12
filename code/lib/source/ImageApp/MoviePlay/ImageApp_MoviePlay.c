#include "stdio.h"
#include <string.h>
#include "ImageApp/ImageApp_MoviePlay.h"
#include "FileSysTsk.h"
#include "avfile/AVFile_ParserMov.h"
#include "avfile/AVFile_ParserTs.h"
#include "nmediaplay_vdodec.h"
#include "hd_common.h"
#include "hd_bsdemux_lib.h"
#include "hd_filein_lib.h"
#include "kflow_common/nvtmpp.h"
//#include <kwrap/util.h>
#include "iamovieplay_tsk.h"
#include "vendor_common.h"
#include "SizeConvert.h"
#include "vendor_audiodec.h"
#include "vendor_videoout.h"
#include "vendor_videodec.h"
#include "GxVideoFile.h"
#include "filein.h"

#define  DECRTPTO_SIZE    256

MOVIEPLAY_IMAGE_STREAM_INFO		*g_pImageStream = {0};
MOVIEPLAY_VIDEO_PLAYBACK        g_tVideoPlaybackStreamInfo[2] = {0};
MOVIEPLAY_AUDIO_PLAYBACK        g_tAudioPlaybackStreamInfo[2] = {0};
MOVIEPLAY_AOUT_INFO_EX			g_audioout_info_ex = {0};
MOVIEPLAY_VDEC_SKIP_SVC_FRAME	g_vdec_skip_svc_frame = {0};
static HD_AUDIOOUT_VOLUME		g_audioout_volume = {100};

#if 0
MOVIEPLAY_DISP_INFO				g_movply_disp_info[4] = {0};
#endif
MOVIEPLAY_DISP_LINK				g_movplay_DispLink[MAX_DISP_PATH] = {0};
UINT32                          g_movply_pool_main_phy_adr = 0;
UINT32                          g_movply_pool_main_va_adr  = 0;
UINT32                          g_movply_pool_header_phy_adr = 0;
UINT32                          g_movply_pool_header_va_adr  = 0;

// file play
FST_FILE                        g_movply_file_hdl = NULL;
CONTAINERPARSER					g_movply_container_obj = {0};
MEDIA_FIRST_INFO                g_movply_1stMedia_info = {0};
UINT32							g_movply_file_format = 0;
UINT64                          g_movply_file_size = 0;
_ALIGNED(32) UINT8			    g_movply_h265_vdo_desc[MOVIEPLAY_VIDDEC_H265_NAL_MAXSIZE] = {0};
MEM_RANGE                       g_movply_mem_map[MOVIEPLAY_MEM_CNT] = {0};
UINT8                           g_movply_config_fileplay_ok = 0;
UINT8                           g_movply_opened = 0;
MOVIEPLAY_FILE_DECRYPT			g_movply_file_decrypt = {0};

// step play
#define MOVIEPLAY_VIDEO_RAW_QUEUE_MAX_SIZE    15  // Should refer to GOP size.
#define MOVIEPLAY_TS_DEMUXBUF_SIZE_DEFAULT		0x100000

typedef struct {
	UINT32           refCnt;
	UINT32           hData;
	MEM_RANGE		 mem;
	UINT32           blk_id;
	void            *pnext_rawdata;
} VDODEC_RAWDATA;

UINT32							g_movply_vdo_present_idx       = 0;
UINT32							g_movply_vdo_max_rawqueue_size = 0;
UINT32                          g_movply_vdo_play_status       = _CFG_PLAY_STS_NORMAL;
UINT32                          g_enter_step_state             = FALSE;
//static VDODEC_RAWDATA		    g_movply_vdo_rawdata[1]		   = {0};
UINT32							g_movply_vdo_rawqueue_idx      = 0;
UINT32							g_movply_vdo_rawqueue_idx_table[MOVIEPLAY_VIDEO_RAW_QUEUE_MAX_SIZE]      = {0xFFFFFFFF};
//static NMI_VDODEC_RAW_INFO      g_movply_vdo_rawdata_queue[MOVIEPLAY_VIDEO_RAW_QUEUE_MAX_SIZE]           = {0};
static HD_COMMON_MEM_INIT_CONFIG *mem_cfg = NULL;
BOOL                            g_movply_ado_out_init          = FALSE;

ID 			   IAMOVIEPLAY_SEMID_DECRYPT = 0;

#if 0
static ISF_RV _ISF_VdoDec_LockCB(UINT64 module, UINT32 hData)
{
	UINT32 i = 0;
	VDODEC_RAWDATA *pVdoDecData = NULL;

	// search VDODEC_RAWDATA link list by hData
	for (i = 0; i < 1; i++) {
		pVdoDecData = &g_movply_vdo_rawdata[i];

		if (pVdoDecData != NULL) {
			pVdoDecData->refCnt++;
			DBG_IND("refCnt++, %s %d\r\n", (CHAR *)&module,pVdoDecData->refCnt);
			//DBG_DUMP("refCnt++, %s %d\r\n", (CHAR *)&module,pVdoDecData->refCnt);
		}
	}
	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_UnLockCB(UINT64 module, UINT32 hData)
{
	UINT32 i = 0;
	VDODEC_RAWDATA *pVdoDecData = NULL;

	// search VDODEC_RAWDATA link list by hData
	for (i = 0; i < 1; i++) {
		pVdoDecData = &g_movply_vdo_rawdata[i];

		if (pVdoDecData != NULL) {
			pVdoDecData->refCnt--;
			DBG_IND("refCnt--, %s %d\r\n", (CHAR *)&module, pVdoDecData->refCnt);
			//DBG_DUMP("refCnt--, %s %d\r\n", (CHAR *)&module, pVdoDecData->refCnt);
		}
	}
	return ISF_OK;
}
#endif

ISF_DATA g_movply_ISF_VdoDecRawData = {
	.sign      = ISF_SIGN_DATA,                 ///< signature, equal to ISF_SIGN_DATA or ISF_SIGN_EVENT
	.h_data     = 0,                             ///< handle of real data, it will be "nvtmpp blk_id", or "custom data handle"
//	.pLockCB   = _ISF_VdoDec_LockCB,            ///< CB to lock "custom data handle"
//	.pUnlockCB = _ISF_VdoDec_UnLockCB,          ///< CB to unlock "custom data handle"
//	.Event     = 0,                             ///< default 0
//	.Serial    = 0,                             ///< serial id
//	.TimeStamp = 0,                             ///< time-stamp
};

//static ISF_VIDEO_STREAM_BUF      g_movply_ISF_VdoStrmBuf = {0};

// TS format
MEM_RANGE                       g_ts_demux_range = {0};

#if 0
ISF_UNIT ISF_VdoOut1; //temp
ISF_UNIT ISF_VdoOut2; //temp
static MOVIEPLAY_VDO_OUT_INFO g_isf_vdoout_info[MOVIEPLAY_VID_OUT_MAX] = {
	{&ISF_VdoOut1},
	{&ISF_VdoOut2},
};
#endif

// debug
//static UINT32                   g_movply_dbglvl = 0;

#if 0
static void _ImageApp_MoviePlay_CB(CHAR *Name, UINT32 event_id, UINT32 value)
{
//	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	MOVIEPLAY_DISP_INFO *p_disp_info = g_pImageStream->p_disp_info;

	if (p_disp_info == NULL) {
		DBG_ERR("disp_info is NULL\r\n");
		return;
	}

	if (strcmp(Name, "BsDemux") == 0) { // from BsDemux
		switch (event_id) {
        #if 0
		case ISF_BSDEMUX_EVENT_REFRESH_VDO: {
			// Use ImageUnit_SendEvent() instead of ImageUnit_SetParam() to avoid semaphore dead lock
			ISF_PORT *pDest = ImageUnit_In(&ISF_VdoDec, ISF_IN1+p_disp_info->disp_id);
			ImageUnit_SendEvent(pDest, VDODEC_PARAM_REFRESH_VDO, 0, 0, 0);
			break;
		}
        #endif
		default:
			DBG_ERR("%s: unknow event_id=0x%x\r\n", Name, event_id);
			break;
		}
	} else if (strcmp(Name, "VdoDec") == 0) { // from VdoDec
		switch (event_id) {
        #if 0
		case ISF_VDODEC_EVENT_ONE_DISPLAYFRAME: {
        	ISF_DATA				*pRawData		= &g_movply_ISF_VdoDecRawData;
        	ISF_PORT             	*pDestPort   	= NULL;
        	PISF_VIDEO_STREAM_BUF	pVdoStrmBuf		= &g_movply_ISF_VdoStrmBuf;
        	UINT32					i 				= 0;

            if(g_movply_vdo_play_status == _CFG_PLAY_STS_STEP) {
    			NMI_VDODEC_RAW_INFO *pRawInfo = (NMI_VDODEC_RAW_INFO *)value;
#if 0           // callback raw info
    			DBG_DUMP("vdodec cb: i=%d, addr=0x%x, size=0x%x\r\n", pRawInfo->uiThisFrmIdx, pRawInfo->uiRawAddr, pRawInfo->uiRawSize);
#endif
                if (pRawInfo) {
            		if (pRawInfo->uiRawAddr == 0) {
            			DBG_ERR("pRawInfo->uiRawAddr = 0\r\n");
            			break;
            		}
                } else {
            			DBG_ERR("pRawInfo = 0\r\n");
            			break;
                }

                g_movply_vdo_rawqueue_idx_table[g_movply_vdo_rawqueue_idx] = pRawInfo->uiThisFrmIdx;
                memcpy(&g_movply_vdo_rawdata_queue[g_movply_vdo_rawqueue_idx], pRawInfo, sizeof(NMI_VDODEC_RAW_INFO));
                g_movply_vdo_rawqueue_idx++;

                // Trigger BSDEMUX_PARAM_STEP_PLAY and the desired callback times is reached.
                // Show the frame with the same idx as g_movply_vdo_present_idx.
                if(g_movply_vdo_rawqueue_idx >= g_movply_vdo_max_rawqueue_size) {
                    MEM_RANGE          mPresentRaw;
                    UINT32             uiRawW;
                    UINT32             uiRawH;

                    // Get width and height of RAW.
                    uiRawW = ImageUnit_GetParam(&ISF_VdoDec, ISF_OUT1, VDODEC_PARAM_WIDTH);
                    uiRawH = ImageUnit_GetParam(&ISF_VdoDec, ISF_OUT1, VDODEC_PARAM_HEIGHT);

                    // Point to the specific RAW in RAW queue with the matched frame idx.
                    pRawInfo = &g_movply_vdo_rawdata_queue[g_movply_vdo_rawqueue_idx-1];
                    mPresentRaw.Addr = pRawInfo->uiRawAddr;
                    mPresentRaw.Size = pRawInfo->uiRawSize;
            		pVdoStrmBuf->flag     = MAKEFOURCC('V', 'R', 'A', 'W');
                    pVdoStrmBuf->Width    = uiRawW;
                    pVdoStrmBuf->Height   = uiRawH;
               		pVdoStrmBuf->DataAddr =  mPresentRaw.Addr;//pRawInfo->uiRawAddr;
            		pVdoStrmBuf->DataSize =  mPresentRaw.Size;//pRawInfo->uiRawSize;
            		pVdoStrmBuf->Resv[0]  = pRawInfo->uiYAddr;       // Resv[1] for Y address
            		pVdoStrmBuf->Resv[1]  = pRawInfo->uiUVAddr;      // Resv[2] for UV address
            		pVdoStrmBuf->Resv[2]  = pRawInfo->uiThisFrmIdx;  // Resv[0] for frame index
            		pVdoStrmBuf->Resv[3]  = pRawInfo->bIsEOF;        // Resv[3] for EOF flag

            		// get VDODEC_RAWDATA
            		#if 1
            		g_movply_vdo_rawdata[0].hData  = mPresentRaw.Addr;
            		g_movply_vdo_rawdata[0].refCnt = 1;
                    #endif

            		// Set Raw ISF_DATA info
            		memcpy(pRawData->Desc, pVdoStrmBuf, sizeof(ISF_VIDEO_STREAM_BUF));
            		//pRawData->TimeStamp  = pRawInfo->TimeStamp;
            		pRawData->hData      = mPresentRaw.Addr;
                    //new pData by user
            		ISF_VdoDec.pBase->New(&ISF_VdoDec, pDestPort, NVTMPP_VB_INVALID_POOL, pRawData->hData, 0, pRawData);
                    //ImageUnit_NewData(img_buf_size, pRawData);

                    //DBGH(mPresentRaw.Addr);
                    //DBGH(mPresentRaw.Size);

                    // Push to UserProc.
                    for (i = 0; i < 1; i++) {
        				pDestPort = ImageUnit_Out(&ISF_VdoDec, (ISF_OUT_BASE + i));
        				if (ImageUnit_IsAllowPush(pDestPort)) {
        					ImageUnit_PushData(pDestPort, pRawData, 0);
                            ImageUnit_ReleaseData(pRawData);
        			    }
                    }

                    // reset vdo raw queue idx.
                    g_movply_vdo_rawqueue_idx = 0;
                } else {
                    DBG_IND("%d)collecting raw...\r\n", g_movply_vdo_rawqueue_idx);
                    break;
                }
            }

			if (p_play_info->event_cb) {
				(p_play_info->event_cb)(MOVIEPLAY_EVENT_ONE_DISPLAYFRAME);
                g_movply_vdo_play_status = _CFG_PLAY_STS_NORMAL;
			} else {
				DBG_ERR("%s: event_cb = NULL\r\n", Name);
			}
			break;
		}
		case ISF_VDODEC_EVENT_CUR_VDOBS: {
			// Use ImageUnit_SendEvent() instead of ImageUnit_SetParam() to avoid semaphore dead lock
			ISF_PORT *pDest = ImageUnit_In(&ISF_FileIn, ISF_IN1+p_disp_info->disp_id);
			ImageUnit_SendEvent(pDest, FILEIN_PARAM_CUR_VDOBS, value, 0, 0);
			break;
		}
        #endif
		default:
			DBG_ERR("%s: unknow event_id=0x%x\r\n", Name, event_id);
			break;
		}
	} else if (strcmp(Name, "FileIn") == 0) { // from FileIn
		switch (event_id) {
#if 0
		case ISF_FILEIN_EVENT_MEM_RANGE:

			break;
		case ISF_FILEIN_EVENT_READ_ONE_BLK: {
			ISF_PORT *pDest = ImageUnit_In(&ISF_BsDemux, ISF_IN_BASE+p_disp_info->disp_id);
			//NMI_TSDEMUX_READBUF_INFO *p_ts_read = (NMI_TSDEMUX_READBUF_INFO *)value;
			ImageUnit_SendEvent(pDest, BSDEMUX_PARAM_TRIG_TSDMX, value, 0, 0);
			break;
		}
#endif
		default:
			DBG_ERR("%s: unknow event_id=0x%x\r\n", Name, event_id);
			break;
		}
	} else {
		DBG_ERR("No matching unit, Name=%s\r\n", Name);
	}
}
#endif
static MOVIEPLAY_IMAGE_STREAM_INFO *_ImageApp_MoviePlay_NEW_STREAM(void)
{
	MOVIEPLAY_IMAGE_STREAM_INFO  *p_isf_strm_info = NULL;
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	ret = hd_common_mem_alloc("IAMovPly_StrmInfo", &pa, (void **)&va, sizeof(MOVIEPLAY_IMAGE_STREAM_INFO), ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("MovPly_StrmInfo alloc fail size 0x%x, ddr %d\r\n", (unsigned int)(sizeof(MOVIEPLAY_IMAGE_STREAM_INFO)), (int)ddr_id);
		return NULL;
	}

	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));

	p_isf_strm_info = (MOVIEPLAY_IMAGE_STREAM_INFO *)va;

	memset(p_isf_strm_info, 0, sizeof(MOVIEPLAY_IMAGE_STREAM_INFO));

	p_isf_strm_info->vb_pool_pa = (UINT32)pa;
	p_isf_strm_info->vb_pool_va = (UINT32)va;

	return p_isf_strm_info;
}

/**
    File Read Callback Function

	param  [in] *path:	not used
	param  [in]  pos:	read file offset. (point to where you want to read)
	param  [in] *pbuf:	read file output buffer address.
    param  [in]  size:	read file size.

    @return ER
*/
static ER _ImageApp_MoviePlay_CBReadFile(UINT8 *type, UINT64 pos, UINT32 size, UINT32 addr)
{
	INT32  ret;
    UINT8 *pbuf = (UINT8 *)addr;
	UINT64 size64 = 0;

	/* decrypt */
	BOOL is_encrypted = (g_movply_file_decrypt.type == MOVIEPLAY_DECRYPT_TYPE_NONE) ? FALSE : TRUE;
	UINT32 decrypt_type = g_movply_file_decrypt.type;

	// check boundary
	if (pos > g_movply_file_size) {
		DBG_ERR("pos(0x%llx) > totalSize(0x%llx)\r\n", pos, g_movply_file_size);
		return E_SYS;
	}

	// check boundary
	size64 = (UINT64)size;
	if ((pos + size64) > g_movply_file_size) {
		UINT64 new_size = 0;
		new_size = ALIGN_FLOOR_32(g_movply_file_size - pos);
		if (new_size > 0xFFFFFFFF) {
			DBG_ERR("new_size(0x%llx) > UINT32 value\r\n", new_size);
			return E_SYS;
		}
		size = (UINT32)new_size;
	}

	/* read file header */
	if (is_encrypted && (pos <= DECRTPTO_SIZE) && (decrypt_type & MOVIEPLAY_DECRYPT_TYPE_CONTAINER)) {

		UINT8 decrypt_buf[DECRTPTO_SIZE] = {0};
		UINT32 decrypt_size = DECRTPTO_SIZE;

		// read decrypt section
		FileSys_SeekFile(g_movply_file_hdl, 0, FST_SEEK_SET);
		ret = FileSys_ReadFile(g_movply_file_hdl, pbuf, &decrypt_size, 0, NULL);

		_iamovieplay_decrypt_hex_dump(MOVIEPLAY_DECRYPT_TYPE_CONTAINER, "CBReadFile before decrypt", (UINT32)pbuf, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);

		_iamovieplay_decrypt((UINT32)pbuf, decrypt_size);

		_iamovieplay_decrypt_hex_dump(MOVIEPLAY_DECRYPT_TYPE_CONTAINER, "CBReadFile after decrypt ", (UINT32)pbuf, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);

		memcpy(decrypt_buf, pbuf, decrypt_size);

		// read file
		FileSys_SeekFile(g_movply_file_hdl, pos, FST_SEEK_SET);
		ret = FileSys_ReadFile(g_movply_file_hdl, pbuf, &size, 0, NULL);

		memcpy(pbuf, decrypt_buf+pos, decrypt_size-pos);
	}
	/* read desc */
	else {

		// read file
		FileSys_SeekFile(g_movply_file_hdl, pos, FST_SEEK_SET);
		ret = FileSys_ReadFile(g_movply_file_hdl, pbuf, &size, 0, NULL);
	}

	if (ret == E_OK) {
		return E_OK;
	} else {
		DBG_ERR("FileSys Read File is FAILS, handle = 0x%x, ret = %d\r\n", g_movply_file_hdl, ret);
		return E_SYS;
	}
}

/**
    Set file handle and get total file size

    @param[in] pFilehdl  opened media file

    @return void
*/
static void _ImageApp_MoviePlay_SetFileHdl(FST_FILE pFilehdl)
{
    INT32  ret;

    if (pFilehdl) {
		g_movply_file_hdl = pFilehdl;
	}

	ret = FileSys_SeekFile(g_movply_file_hdl, 0, FST_SEEK_END);
    if (ret == E_OK) {
    	g_movply_file_size = FileSys_TellFile(g_movply_file_hdl);
        if(g_movply_file_size == (UINT64)FST_STA_ERROR) {
    		DBG_ERR("FileSys Tell File is FAILS\r\n");
        }
    }else {
		DBG_ERR("FileSys Seek File is FAILS, ret = %d\r\n", ret);
	}
    ret = FileSys_SeekFile(g_movply_file_hdl, 0, FST_SEEK_SET); // set to file start
	if (ret != E_OK) {
		DBG_ERR("FileSys Seek File is FAILS, ret = %d\r\n", ret);
	}
}

/**
    Probe Media File Format

    @param[in] addr  file read address
    @param[in] size  file read size

    @return UINT32 file format type
*/
static UINT32 _ImageApp_MoviePlay_ProbeType(UINT32 addr, UINT32 size)
{
	ER              probe;
	UINT32          realtype = 0;
	CONTAINERPARSER *ptr = 0;

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		probe = ptr->Probe(addr, size);

		if (probe == E_OK) {
			realtype = _MOVPLAY_CFG_FILE_FORMAT_MOV;
			g_movply_file_format = _MOVPLAY_CFG_FILE_FORMAT_MOV;
			return realtype;
		}
	}

	// Get TS
	ptr = MP_TsReadLib_GerFormatParser();
	if (ptr) {
		probe = ptr->Probe(addr, TS_PACKET_SIZE * TS_CHECK_COUNT);

		if (probe > 6) { //score
			realtype = _MOVPLAY_CFG_FILE_FORMAT_TS;
			g_movply_file_format = _MOVPLAY_CFG_FILE_FORMAT_TS;
			//tsDemuxMemOffset = GXVIDEO_TSDEMUX_MEM_OFFSET;
		}
	}

	return realtype;
}

/**
    Create container object for playing.

    @param[in] type  file format type

    @return ER
*/
static UINT32 _ImageApp_MoviePlay_CreateContainerObj(UINT32 type)
{
	CONTAINERPARSER *ptr = 0;
   	ER               ret;

    // get parser
	if (type == _MOVPLAY_CFG_FILE_FORMAT_MOV) {
		ptr = MP_MovReadLib_GerFormatParser();
	} else if (type == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		ptr = MP_TsReadLib_GerFormatParser();
	} else {
		DBG_ERR("file type not support\r\n");
		return E_NOSPT;
	}

	if (!ptr) {
		DBG_ERR("file container obj is NULL\r\n");
		return E_SYS;
	}

    // register container obj
	g_movply_container_obj.Probe         = ptr->Probe;
	g_movply_container_obj.Initialize    = ptr->Initialize;
	g_movply_container_obj.SetMemBuf     = ptr->SetMemBuf;
	g_movply_container_obj.ParseHeader   = ptr->ParseHeader;
	g_movply_container_obj.GetInfo       = ptr->GetInfo;
	g_movply_container_obj.SetInfo       = ptr->SetInfo;
	g_movply_container_obj.CustomizeFunc = NULL;
	g_movply_container_obj.checkID       = ptr->checkID;
	//g_movply_container_obj.cbShowErrMsg  = debug_msg;
	g_movply_container_obj.cbReadFile    = _ImageApp_MoviePlay_CBReadFile;

    if (type == _MOVPLAY_CFG_FILE_FORMAT_MOV) {
		ret = MovReadLib_RegisterObjCB(&g_movply_container_obj);
	} else if (type == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		ret = TsReadLib_RegisterObjCB(&g_movply_container_obj);
	}
#if 0 // To eliminate coverity issue.
    else
	{
	    ret = E_NOSPT;
	}
#endif

	return ret;
}

/**
    New buffer for header parsing.

    @param[in] void

    @return ER
*/
static ISF_RV _ImageApp_MoviePlay_New_HeaderBuf(UINT32 *pAddr, UINT32 *pSize)
{
#if 0
	NVTMPP_VB_BLK            mpp_blk         = 0;
	NVTMPP_MODULE            mpp_module      = MAKE_NVTMPP_MODULE('A', 'p', 'p', 'M', 'o', 'v', 'P', 'l');

	g_movply_pool_header = nvtmpp_vb_create_pool("movply_header", *pSize, 1, NVTMPP_DDR_1);
	if (g_movply_pool_header == NVTMPP_VB_INVALID_POOL) {
		DBG_ERR("create mpp_pool failed (size = %d)!\r\n", *pSize);
		return ISF_ERR_FAILED;
	}

	mpp_blk = nvtmpp_vb_get_block(mpp_module, g_movply_pool_header, *pSize, NVTMPP_DDR_1);
	if (mpp_blk == NVTMPP_VB_INVALID_BLK) {
		DBG_ERR("get isf_data block failed!\r\n");
		return ISF_ERR_FAILED;
	}

	*pAddr = nvtmpp_vb_block2addr(mpp_blk);
#else
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	ret = hd_common_mem_alloc("movply_header", &pa, (void **)&va, *pSize, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("FileIn alloc fail size 0x%x, ddr %d\r\n", (unsigned int)(*pSize), (int)ddr_id);
		return E_SYS;
	}

	DBG_DUMP("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));

	g_movply_pool_header_phy_adr = (UINT32)pa;
	g_movply_pool_header_va_adr = (UINT32)va;

    *pAddr = (UINT32)va;
#endif

	return ISF_OK;
}

static ISF_RV _ImageApp_MoviePlay_Free_HeaderBuf(UINT32 *uiPoolPa, UINT32 *uiPoolVa)
{
	HD_RESULT            ret;

	ret = hd_common_mem_free((UINT32)*uiPoolPa, (void *)*uiPoolVa);
	if (ret != HD_OK) {
		DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
		return E_SYS;
	}

	*uiPoolPa = 0;
	*uiPoolVa = 0;

    return ISF_OK;
}

static ISF_RV _ImageApp_MoviePlay_AllocMem(void)
{
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	UINT32 all_need_size = 0, total_sec = 0, aud_total_sec= 0, start_addr = 0;
	UINT32 idx;
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	total_sec = (p_obj->file_dur/1000)+1; //video total second and 1 sec tolerance
	if (p_obj->aud_ttfrm == 0 && p_obj->aud_fr == 0) {
		aud_total_sec = total_sec;
	} else {
		aud_total_sec = (p_obj->aud_ttfrm/p_obj->aud_fr) + 1; //audio total second and 1 sec tolerance
	}

	/* calculate need memory size */
	// video frame entry table
	if (p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		g_movply_mem_map[MOVIEPLAY_MEM_VDOENTTBL].size = sizeof(TS_FRAME_ENTRY) * TSVID_ENTRY_SIZE;
	} else {
		g_movply_mem_map[MOVIEPLAY_MEM_VDOENTTBL].size = total_sec * p_obj->vdo_fr * 0x10; // 0x10: one entry index size
	}
	if (g_movply_mem_map[MOVIEPLAY_MEM_VDOENTTBL].size == 0) {
		DBG_ERR("video entry mem alloc error, total_sec=%d(s), vdo_fr=%d\r\n", total_sec, p_obj->vdo_fr);
	}
	all_need_size += g_movply_mem_map[MOVIEPLAY_MEM_VDOENTTBL].size;

	// audio frame entry table
	if (p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		g_movply_mem_map[MOVIEPLAY_MEM_AUDENTTBL].size = sizeof(TS_FRAME_ENTRY) * TSAUD_ENTRY_SIZE;
	} else {
		g_movply_mem_map[MOVIEPLAY_MEM_AUDENTTBL].size = aud_total_sec * (p_obj->aud_fr+1) * 0x10;
	}
	if (g_movply_mem_map[MOVIEPLAY_MEM_AUDENTTBL].size == 0) {
		DBG_ERR("audio entry mem alloc error, total_sec=%d(s), aud_fr=%d\r\n", aud_total_sec, p_obj->aud_fr);
	}
	all_need_size += g_movply_mem_map[MOVIEPLAY_MEM_AUDENTTBL].size;

	ret = hd_common_mem_alloc("movply_main", &pa, (void **)&va, all_need_size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("FileIn alloc fail size 0x%x, ddr %d\r\n", (unsigned int)(all_need_size), (int)ddr_id);
		return E_SYS;
	}

	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));

	start_addr = (UINT32)va;

	g_movply_pool_main_phy_adr = (UINT32)pa;
	g_movply_pool_main_va_adr = (UINT32)va;

	/* allocate memory */
	DBG_DUMP("-----MoviePlay_MemMap---------------------------------------\r\n");
    for (idx = MOVIEPLAY_MEM_VDOENTTBL; idx < MOVIEPLAY_MEM_CNT; idx++)
    {
        g_movply_mem_map[idx].addr = start_addr;
        start_addr += g_movply_mem_map[idx].size;
		DBG_DUMP("MemIdx          Addr      Size\r\n");
		DBG_DUMP("    %d   0x%08X   0x%08X\r\n", idx, g_movply_mem_map[idx].addr, g_movply_mem_map[idx].size);
    }

	return ISF_OK;
}

static MEM_RANGE _ImageApp_MoviePlay_GetTSDesc(UINT32 vdo_codec)
{
	MEM_RANGE desc = {0};

	if (vdo_codec == _MOVPLAY_CFG_CODEC_H264) {
		if (g_movply_container_obj.GetInfo) {
			(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_H264DESC, &desc.addr, &desc.size, 0);
		}
	} else if (vdo_codec == _MOVPLAY_CFG_CODEC_H265) {
		if (g_movply_container_obj.GetInfo) {
			(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_H265DESC, &desc.addr, &desc.size, 0);
		}
	}

	if (desc.addr == 0 || desc.size == 0) {
		DBG_ERR("get desc error, addr=0x%x, size=0x%x\r\n", desc.addr, desc.size);
	}

	return desc;
}

static MEM_RANGE _ImageApp_MoviePlay_GetH264Desc(void)
{
	MEM_RANGE desc = {0};

	if (g_movply_container_obj.GetInfo) {
		(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_H264DESC, &desc.addr, &desc.size, 0);
	}

	if (desc.addr == 0 || desc.size == 0) {
		DBG_ERR("get desc error, addr=0x%x, size=0x%x\r\n", desc.addr, desc.size);
	}

	return desc;
}

static ER _ImageApp_MoviePlay_GetH265Desc(UINT32 fileOffset, UINT32 DescAddr, UINT32 *pDescSize)
{
	ER     EResult = E_OK;
	UINT32 readSize = MOVIEPLAY_VIDDEC_H265_NAL_MAXSIZE;
	UINT32 startCode = 0;
	UINT16 vpsLen = 0, spsLen = 0, ppsLen = 0;
	UINT8  *ptr8 = 0;
	UINT8 descCnt = 0;

	EResult = _ImageApp_MoviePlay_CBReadFile(0, fileOffset, readSize, DescAddr);
	if (EResult != E_OK) {
		DBG_ERR("H265Desc file read error!!\r\n");
		return EResult;
	}


	ptr8 = (UINT8 *)DescAddr;

	// parse H.265 description and get its length
    while (readSize--) {
		/*** use startcode version ***/
		if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01)) {
            startCode++;
		}
        if (startCode == 4) {
            if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) { // Type: 19 = IDR
                *pDescSize = (UINT32)ptr8 - DescAddr; // addr_dst - addr_src
			}
            else {
                DBG_ERR("Can not find IDR!\r\n");
                *pDescSize = 0;
                EResult = E_SYS;
				break;
            }
            break;
        }

		/* ************************************************
		 * startcode with length version
		 * IVOT_N00028-343 support multi-tile
		 * *************************************************/
        do{

            if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 4) == 0x40) && startCode == 0) { // find vps
    			vpsLen = (*(ptr8 + 2) << 8) + *(ptr8 + 3);
    			*(ptr8 + 2) = 0x00; // update start code
    			*(ptr8 + 3) = 0x01;
    			ptr8 += (4 + vpsLen);

    			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && *(ptr8 + 4) == 0x42) { // find sps
    				spsLen = (*(ptr8 + 2) << 8) + *(ptr8 + 3);
    				*(ptr8 + 2) = 0x00; // update start code
    				*(ptr8 + 3) = 0x01;
    				ptr8 += (4 + spsLen);

    				if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && *(ptr8 + 4) == 0x44) { // find pps
    					ppsLen = (*(ptr8 + 2) << 8) + *(ptr8 + 3);
    					*(ptr8 + 2) = 0x00; // update start code
    					*(ptr8 + 3) = 0x01;
    					ptr8 += (4 + ppsLen);

    					descCnt++;
    				}
    			}
    		}
            else
            	break;

        } while(1);

        if(descCnt > 0){

    		if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) { // Type: 19 = IDR
    			*pDescSize = (UINT32)ptr8 - DescAddr; // addr_dst - addr_src
    			DBG_DUMP("vpsLen=0x%x, spsLen=0x%x, ppsLen=0x%x, DescCnt=%u DescSize=0x%x\r\n", vpsLen, spsLen, ppsLen, descCnt, *pDescSize);
    			EResult = E_OK;
    		}
    		else {
                DBG_ERR("Can not find IDR! %u\r\n", __LINE__);
                *pDescSize = 0;
            }
    		break;
        }

        ptr8++;
    }

	return EResult;
}

static UINT32 _ImageApp_MoviePlay_GetVdoGOP(void)
{
	UINT32 Iframe = 0, uiTotalVidFrm = 0;
	UINT32 uiNextIFrm = 0;
	UINT32 uiSkipI = 0;

	// Since there have no GOP number information in H.264 stream level
	// calculate the difference between the Key Frame index by Sync Sample Atoms to get the GOP number
	if (g_movply_container_obj.GetInfo) {
		// Get Next I frame number
		(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_NEXTIFRAMENUM, &Iframe, &uiNextIFrm, &uiSkipI);
		// Get totoal frame number
		(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_VFRAMENUM, &uiTotalVidFrm, 0, 0);
	}

	return uiNextIFrm;
}

/**
    Create media container by the opened media file, and parse 1st media information.

    @param[in] void

    @return ER
*/
static ISF_RV _ImageApp_MoviePlay_CreateMediaContainer(void)
{
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	MOVIEPLAY_PLAY_OBJ      *play_obj = &g_pImageStream->play_obj;
	UINT32                   fileType;
    ER                       ret;
	MEM_RANGE                ContBuf = {0}, FileHeader = {0}, H26xWorkBuf = {0}, TsDmx = {0};

    // new buffer
    ContBuf.size = MOVIEPLAY_HEADER_READSIZE + MOVIEPLAY_TS_H26X_WORKBUF + p_play_info->ts_demux_buf_size;
	if (_ImageApp_MoviePlay_New_HeaderBuf(&ContBuf.addr, &ContBuf.size) != ISF_OK) {
        DBG_ERR("New header buffer fail\r\n");
        return ISF_ERR_FAILED;
	}
	// allocate buffer
	FileHeader.addr = ContBuf.addr;
	FileHeader.size = MOVIEPLAY_HEADER_READSIZE;
	H26xWorkBuf.addr = FileHeader.addr + FileHeader.size;
	H26xWorkBuf.size = MOVIEPLAY_TS_H26X_WORKBUF;
	TsDmx.addr = H26xWorkBuf.addr + H26xWorkBuf.size;
	TsDmx.size = p_play_info->ts_demux_buf_size;

    // set file handle
    if (p_play_info->file_handle) {
        _ImageApp_MoviePlay_SetFileHdl((FST_FILE) p_play_info->file_handle);
    } else {
        DBG_ERR("file handle is NULL\r\n");
        return ISF_ERR_FAILED;
    }

	// read file
	if (_ImageApp_MoviePlay_CBReadFile(0, 0, FileHeader.size, FileHeader.addr) != E_OK) {
		DBG_ERR("read file error\r\n");
		return ISF_ERR_FAILED;
	}

	// check file type
	fileType = _ImageApp_MoviePlay_ProbeType(FileHeader.addr, FileHeader.size);
	if (!fileType) {
		DBG_ERR("File type get error!!\r\n");
		return ISF_ERR_FAILED;
	}
	play_obj->file_fmt = fileType;

	// create container obj
    if (_ImageApp_MoviePlay_CreateContainerObj(fileType) != E_OK) {
        DBG_ERR("Create container obj fail\r\n");
        return ISF_ERR_FAILED;
    }

    // setup media container
	if (g_movply_container_obj.Initialize) {
		(g_movply_container_obj.Initialize)();
	}

	if (g_movply_container_obj.SetMemBuf) {
		(g_movply_container_obj.SetMemBuf)(FileHeader.addr, FileHeader.size);
	}

    if (g_movply_container_obj.SetInfo)
		(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
										g_movply_file_size >> 32,
										g_movply_file_size,
										0);
	if (fileType == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		if (g_movply_container_obj.SetInfo) {
			(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_TSDEMUXBUF, TsDmx.addr, TsDmx.size, 0);
		}
		// Only TS H.265 need this work buf
		if (g_movply_container_obj.SetInfo) {
			(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_H26X_WORKBUF, (UINT32)&H26xWorkBuf, 0, 0);
		}
	}

    // Get 1st Media Information
	if (fileType == _MOVPLAY_CFG_FILE_FORMAT_MOV) {
		ret = MovReadLib_Parse1stVideo(FileHeader.addr, FileHeader.size, (void *)&g_movply_1stMedia_info);
	} else if (fileType == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		ret = TsReadLib_Parse1stVideo(FileHeader.addr, FileHeader.size, (void *)&g_movply_1stMedia_info);
	}

	if (ret != E_OK) {
		DBG_ERR("Parse header fail, ret=%d\r\n", ret);
		return ISF_ERR_FAILED;
	}

	if (g_movply_1stMedia_info.dur == 0) {
		DBG_ERR("Total duration = 0\r\n");
		return ISF_ERR_FAILED;
	}

	// free header buffer
    if (g_movply_pool_header_va_adr != 0) {
		if (_ImageApp_MoviePlay_Free_HeaderBuf(&g_movply_pool_header_phy_adr, &g_movply_pool_header_va_adr) != ISF_OK) {
			DBG_ERR("free header buf fail\r\n");
	        return ISF_ERR_FAILED;
	    }
    } else {
    	DBG_WRN("no need to free header buf\r\n");
    }

    return ISF_OK;
}

#if 0
static ISF_RV _ImageApp_MoviePlay_ConfigDisp(MOVIEPLAY_DISP_INFO *p_disp_info)
{
	if (p_disp_info == NULL) {
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_disp_info->disp_id >= MOVIEPLAY_DISP_ID_MAX) {
		return ISF_ERR_INVALID_PARAM;
	}

	g_movply_disp_info[p_disp_info->disp_id] = *p_disp_info;

	return ISF_OK;
}
#endif

static ISF_RV _ImageApp_MoviePlay_ParseFullHeader(MEDIA_HEADINFO *p_info)
{
	MEM_RANGE buf = {0};

	// new header buffer
    buf.size = MOVIEPLAY_HEADER_READSIZE;
	if (_ImageApp_MoviePlay_New_HeaderBuf(&buf.addr, &buf.size) != ISF_OK) {
        DBG_ERR("New header buffer fail\r\n");
        return ISF_ERR_FAILED;
	}

	// set buffer to parser
	if (g_movply_container_obj.SetMemBuf) {
		(g_movply_container_obj.SetMemBuf)(buf.addr, buf.size);
	}

	// parse full header
	if (g_movply_container_obj.ParseHeader) {
		(g_movply_container_obj.ParseHeader)(0, 0, p_info);
	}

	// free header buffer
    if (g_movply_pool_header_va_adr != 0) {
		if (_ImageApp_MoviePlay_Free_HeaderBuf(&g_movply_pool_header_phy_adr, &g_movply_pool_header_va_adr) != ISF_OK) {
			DBG_ERR("free header buf fail\r\n");
	        return ISF_ERR_FAILED;
	    }
    } else {
    	DBG_WRN("no need to free header buf\r\n");
    }

	return ISF_OK;
}

static ISF_UNIT ISF_MediaPlay = {0};
static ISF_RV _ImageApp_MoviePlay_ConfigFilePlay(MOVIEPLAY_FILEPLAY_INFO *p_play_info)
{
	MOVIEPLAY_IMAGE_STREAM_INFO *p_isf_stream_info = NULL;
	MOVIEPLAY_PLAY_OBJ          *p_play_obj = NULL;
	MEDIA_FIRST_INFO            *p_1st_info = NULL;
	MEDIA_HEADINFO               header_info;
	MEM_RANGE                    desc_mem = {0};
	UINT32                       desc_size = 0;
	extern const char ImageApp_MoviePlay_LIBRARY_VERSION_INFO[];

	// set play info
	g_pImageStream					= _ImageApp_MoviePlay_NEW_STREAM();
	p_isf_stream_info				= g_pImageStream;
	p_isf_stream_info->id			= p_play_info->fileplay_id;
	p_isf_stream_info->p_mediaplay	= &ISF_MediaPlay;

	if(!p_play_info->ts_demux_buf_size){
		p_play_info->ts_demux_buf_size = MOVIEPLAY_TS_DEMUXBUF_SIZE_DEFAULT;
	}

	DBG_DUMP("ts demux buffer size = %lx\n", p_play_info->ts_demux_buf_size);

	memcpy(&(p_isf_stream_info->play_info), p_play_info, sizeof(MOVIEPLAY_FILEPLAY_INFO));
#if (NMEDIAPLAY_ENABLE)
	// create container
	if (_ImageApp_MoviePlay_CreateMediaContainer() != ISF_OK) {
        return ISF_ERR_FAILED;
	}

	p_1st_info = &g_movply_1stMedia_info;

	// check 1st media info
	if (p_1st_info->vidFR == 0) {
		DBG_ERR("Invalid vdo_fr=%d\r\n", p_1st_info->vidFR);
		return ISF_ERR_FAILED;
	}

	// configure play info (1st media info)
	p_play_obj = &p_isf_stream_info->play_obj;
	p_play_obj->width = p_1st_info->wid;
	p_play_obj->height = p_1st_info->hei;
	p_play_obj->tk_wid = p_1st_info->tkwid;
	p_play_obj->tk_hei = p_1st_info->tkhei;
	p_play_obj->vdo_fr = p_1st_info->vidFR;
	p_play_obj->vdo_codec = p_1st_info->vidtype;
	p_play_obj->vdo_1stfrmsize = p_1st_info->size;
	//p_play_obj->aud_en = (p_1st_info->audSR != 0)? TRUE : FALSE;
	p_play_obj->aud_fr = p_1st_info->audFR;
	p_play_obj->aud_sr = p_1st_info->audSR;
	p_play_obj->aud_chs = p_1st_info->audChs;
	p_play_obj->aud_codec = p_1st_info->audtype;
	p_play_obj->file_dur = p_1st_info->dur;
	p_play_obj->aud_ttfrm = p_1st_info->audFrmNum;

	// allocate memory
	if (_ImageApp_MoviePlay_AllocMem() != ISF_OK) {
		DBG_ERR("alloc mem error\r\n");
		return ISF_ERR_FAILED;
	}

	// Set entry table
	if (g_movply_container_obj.SetInfo) {
		(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_VIDENTRYBUF,
										g_movply_mem_map[MOVIEPLAY_MEM_VDOENTTBL].addr,
										g_movply_mem_map[MOVIEPLAY_MEM_VDOENTTBL].size,
										0);
		(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_AUDENTRYBUF,
										g_movply_mem_map[MOVIEPLAY_MEM_AUDENTTBL].addr,
										g_movply_mem_map[MOVIEPLAY_MEM_AUDENTTBL].size,
										0);
	}
	memset(&header_info, 0, sizeof(header_info));
	header_info.checkID = g_movply_container_obj.checkID;

	// parse full header
	_ImageApp_MoviePlay_ParseFullHeader(&header_info);

	// configure play obj (remaining part)
	if (g_movply_file_format == _MOVPLAY_CFG_FILE_FORMAT_TS) {

		desc_mem = _ImageApp_MoviePlay_GetTSDesc(p_play_obj->vdo_codec);
		p_play_obj->desc_addr = desc_mem.addr;
		p_play_obj->desc_size = desc_mem.size;

	} else {
		if (p_play_obj->vdo_codec == _MOVPLAY_CFG_CODEC_H264) {
			desc_mem = _ImageApp_MoviePlay_GetH264Desc();
			p_play_obj->desc_addr = desc_mem.addr;
			p_play_obj->desc_size = desc_mem.size;
			//DBGD(p_play_obj->desc_addr);
			//DBGD(p_play_obj->desc_size);
		} else if (p_play_obj->vdo_codec == _MOVPLAY_CFG_CODEC_H265) {
			_ImageApp_MoviePlay_GetH265Desc(header_info.uiFirstVideoOffset, (UINT32)&g_movply_h265_vdo_desc[0], &desc_size);
			p_play_obj->desc_addr = (UINT32)&g_movply_h265_vdo_desc[0];
			p_play_obj->desc_size = desc_size;
		}
	}
	p_play_obj->file_handle = p_play_info->file_handle;
	p_play_obj->file_size = g_movply_file_size;
	p_play_obj->gop = _ImageApp_MoviePlay_GetVdoGOP();
	p_play_obj->vdo_en = header_info.bVideoSupport;
	p_play_obj->aud_en = header_info.bAudioSupport;
	p_play_obj->vdo_ttfrm = header_info.uiTotalVideoFrames;
	p_play_obj->aud_ttfrm = header_info.uiTotalAudioFrames;

	if (g_movply_file_format == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		DBG_DUMP("^G NMP:(vid) w=%d, h=%d, tkw=%d, tkh=%d, fr=%d, codec=%d, ttfrm=%d\r\n", p_play_obj->width, p_play_obj->height, p_play_obj->tk_wid, p_play_obj->tk_hei, p_play_obj->vdo_fr, p_play_obj->vdo_codec, p_play_obj->vdo_ttfrm);
	}
	else{
		DBG_DUMP("^G NMP:(vid) w=%d, h=%d, tkw=%d, tkh=%d, fr=%d, codec=%d, ttfrm=%d gop=%u\r\n", p_play_obj->width, p_play_obj->height, p_play_obj->tk_wid, p_play_obj->tk_hei, p_play_obj->vdo_fr, p_play_obj->vdo_codec, p_play_obj->vdo_ttfrm, p_play_obj->gop);
	}

	DBG_DUMP("^G NMP:(aud) sr=%d, fr=%d, chs=%d, codec=%d, ttfrm=%d\r\n", p_play_obj->aud_sr, p_play_obj->aud_fr, p_play_obj->aud_chs, p_play_obj->aud_codec, p_play_obj->aud_ttfrm);
	DBG_DUMP("^G NMP:(file) type=%d, hdl=0x%x, dur=%d ms, size=0x%llx\r\n", g_movply_file_format, (UINT32)p_play_obj->file_handle, p_play_obj->file_dur, p_play_obj->file_size);
	DBG_DUMP("^G NMP: ver: %s\r\n", ImageApp_MoviePlay_LIBRARY_VERSION_INFO);

	// Callback 1st media info to UI
	if (p_play_info->event_cb) {
		(p_play_info->event_cb)(MOVIEPLAY_EVENT_MEDIAINFO_READY, 0, 0, 0);
	} else {
		DBG_ERR("p_play_info->event_cb = NULL\r\n");
		return ISF_ERR_FAILED;
	}
#endif

	return ISF_OK;
}

#if 0
static ISF_RV _ImageApp_MoviePlay_Setup_Parameter(MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream)
{
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &pImageStream->play_info;
	MOVIEPLAY_PLAY_OBJ      *p_play_obj = &pImageStream->play_obj;
	NMI_VDODEC_MEM_RANGE     desc_mem = {0};

	/* common */
	desc_mem.Addr = p_play_obj->desc_addr;
	desc_mem.Size = p_play_obj->desc_size;
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_DECDESC, (UINT32)&desc_mem);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_VDO_CODEC, p_play_obj->vdo_codec);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_GOP, p_play_obj->gop);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_AUD_SR, p_play_obj->aud_sr);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_FILE_FMT, g_movply_file_format);

	/* FileIn */
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_FILEHDL, (UINT32) p_play_info->file_handle);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_FILESIZE, (UINT32) &p_play_obj->file_size);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_FILEDUR, (UINT32) p_play_obj->file_dur);
	// need performance tuning, 2018/06/15
	///ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_BLK_TIME, (UINT32) p_play_info->blk_time);
	//ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_BLK_PLNUM, (UINT32) p_play_info->blk_plnum);
	//ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_BLK_TTNUM, (UINT32) p_play_info->blk_ttnum);

	/* BsDemux */
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_VDO_ENABLE, p_play_obj->vdo_en);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_VDO_FR, p_play_obj->vdo_fr);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_VDO_FIRSTFRMSIZE, p_play_obj->vdo_1stfrmsize);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_VDO_TTFRAME, p_play_obj->vdo_ttfrm);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_AUD_ENABLE, p_play_obj->aud_en);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_AUD_TTFRAME, p_play_obj->aud_ttfrm);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_CONTAINER, (UINT32) &g_movply_container_obj);

	/* VdoDec */
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_WIDTH, p_play_obj->width);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_HEIGHT, p_play_obj->height);

	/* AudDec */
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_AUD_CODEC, p_play_obj->aud_codec);
	ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_AUD_CHS, p_play_obj->aud_chs);

    // In dual display case, MediaPlay unit need to open only one time.
    /*if (p_play_info->pmax_mem_info->NeedBufsize == 0) {
        ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_MAX_MEM_INFO, (UINT32) p_play_info->pmax_mem_info);
    }*/

	return ISF_OK;
}
#endif

static void _ImageApp_MoviePlay_FilePlay_OpenStream(MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream)
{
#if 0
	MOVIEPLAY_DISP_INFO *p_disp_info = pImageStream->p_disp_info;
	MOVIEPLAY_PLAY_OBJ *p_play_obj = &pImageStream->play_obj;

	// Init parameter
	_ImageApp_MoviePlay_Setup_Parameter(pImageStream);
	// Open stream
	ImageStream_Open(&ISF_Stream[p_disp_info->disp_id]);
	// Set root
	ImageStream_Begin(&ISF_Stream[p_disp_info->disp_id], 0);

	// Set dummy root
	ImageUnit_Begin(&ISF_Dummy, 0);
	ImageUnit_SetParam(ISF_IN1+p_disp_info->disp_id, DUMMY_PARAM_UNIT_TYPE_IMM, DUMMY_UNIT_TYPE_ROOT);
	ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(&ISF_FileIn, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
	ImageUnit_End();
	ImageStream_SetRoot(0, ImageUnit_In(&ISF_Dummy, ISF_IN1+p_disp_info->disp_id));

		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_CTRL, FILEIN_PARAM_EVENT_CB, (UINT32) _ImageApp_MoviePlay_CB);
		ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(&ISF_BsDemux, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
		ImageUnit_End();

        ImageUnit_Begin(&ISF_BsDemux, 0);
		// set iPort connect type: ISF_CONNECT_PULL
		//ImageUnit_SetParam(ISF_IN1+p_disp_info->disp_id, BSDEMUX_PARAM_IPORT_TYPE, (UINT32)ISF_CONNECT_PULL);
		// connect BsDemux Out1 to In1
		ImageUnit_SetParam(ISF_OUT1+p_disp_info->disp_id, BSDEMUX_PARAM_CONNECT_IMM, ISF_IN1+p_disp_info->disp_id);
		// connect BsDemux Out2 to In1
		ImageUnit_SetParam(ISF_OUT2+p_disp_info->disp_id, BSDEMUX_PARAM_CONNECT_IMM, ISF_IN1+p_disp_info->disp_id);
		ImageUnit_SetParam(ISF_OUT1+p_disp_info->disp_id, BSDEMUX_PARAM_FILEPLAY_MODE, (UINT32)TRUE);
		ImageUnit_SetParam(ISF_CTRL, BSDEMUX_PARAM_EVENT_CB, (UINT32) _ImageApp_MoviePlay_CB);
    	ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(&ISF_VdoDec, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
        if (p_play_obj->aud_en) {
            ImageUnit_SetOutput(ISF_OUT2+p_disp_info->disp_id, ImageUnit_In(&ISF_AudDec, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
        }
        ImageUnit_End();

		if (p_play_obj->aud_en) {
            ImageUnit_Begin(&ISF_AudDec, 0);
			ImageUnit_SetParam(ISF_OUT1+p_disp_info->disp_id, AUDDEC_PARAM_FILEPLAY_MODE, (UINT32)TRUE);
            ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(&ISF_AudOut, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
            ImageUnit_End();

            ImageUnit_Begin(&ISF_AudOut, 0);
			ImageUnit_SetParam(ISF_CTRL, AUDOUT_PARAM_EN_EVT_CB, TRUE);
			ImageUnit_SetParam(ISF_OUT1, AUDOUT_PARAM_CHANNEL, (p_play_obj->aud_chs == 1)? AUDIO_CH_LEFT : AUDIO_CH_STEREO);
        	ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ISF_PORT_EOS, ISF_PORT_STATE_READY);
        	ImageUnit_End();
        }

        ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_CTRL, VDODEC_PARAM_EVENT_CB, (UINT32) _ImageApp_MoviePlay_CB);
        ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(&ISF_UserProc, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
        ImageUnit_End();

        ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetParam(ISF_OUT1+p_disp_info->disp_id, USERPROC_PARAM_BYPASS_IMM, FALSE);
		ImageUnit_SetVdoImgSize(ISF_IN1+p_disp_info->disp_id, 0, 0);
		ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(&ISF_ImgTrans, ISF_IN1+p_disp_info->disp_id), ISF_PORT_STATE_READY);
		ImageUnit_End();

		ImageUnit_Begin(&ISF_ImgTrans, 0);
		ImageUnit_SetOutput(ISF_OUT1+p_disp_info->disp_id, ImageUnit_In(g_isf_vdoout_info[p_disp_info->vid_out].p_vdoout, ISF_IN1), ISF_PORT_STATE_READY);
		ImageUnit_SetParam(ISF_OUT1+p_disp_info->disp_id, IMGTRANS_PARAM_CONNECT_IMM, ISF_IN1+p_disp_info->disp_id);
		ImageUnit_SetVdoImgSize(ISF_IN1+p_disp_info->disp_id, 0, 0);
		ImageUnit_SetParam(ISF_IN1+p_disp_info->disp_id, IMGTRANS_PARAM_MAX_IMG_WIDTH_IMM, 0);
		ImageUnit_SetParam(ISF_IN1+p_disp_info->disp_id, IMGTRANS_PARAM_MAX_IMG_HEIGHT_IMM, 0);
		ImageUnit_End();

		ImageUnit_Begin(g_isf_vdoout_info[p_disp_info->vid_out].p_vdoout, 0);
		ImageUnit_SetVdoImgSize(ISF_IN1, 0, 0); //buffer size = full device size
		if (((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) ||
			((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->height_ratio, p_disp_info->width_ratio);
		} else {
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio, p_disp_info->height_ratio);
		}
		ImageUnit_SetVdoDirection(ISF_IN1, p_disp_info->rotate_dir);
		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
		ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_READY);
		ImageUnit_End();

	ImageStream_End();
	ImageStream_UpdateAll(&ISF_Stream[p_disp_info->disp_id]);

	// Pre-demux TS
	if (g_movply_file_format == _CFG_FILE_FORMAT_TS) {
		MEM_RANGE read_blk_range = {0};

		// get FileIn Info
		MEM_RANGE *p_mem = (MEM_RANGE *)ImageUnit_GetParam(&ISF_FileIn, p_disp_info->disp_id, FILEIN_PARAM_MEM_RANGE);
		read_blk_range.addr = p_mem->Addr;
		read_blk_range.size = NMI_FILEIN_TS_BLK_SIZE * NMI_FILEIN_TS_PL_BLKNUM;
		if (read_blk_range.size > p_mem->Size) {
			DBG_ERR("read_blk_size(0x%x) > mem_size(0x%x)\r\n", read_blk_range.size, p_mem->Size);
			return;
		}
		// trigger demux with info
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+p_disp_info->disp_id, BSDEMUX_PARAM_TRIG_TSDMX, (UINT32) &read_blk_range);
		ImageUnit_End();
	}
#endif

	if (g_movply_file_format == _MOVPLAY_CFG_FILE_FORMAT_TS) {

		HD_PATH_ID path_id = 0;
		MEM_RANGE mem;

		hd_filein_get(path_id, HD_FILEIN_PARAM_MEM_RANGE, (UINT32 *)&mem);

		mem.size = FILEIN_TS_BLK_SIZE*FILEIN_TS_PL_BLKNUM;

		hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_TRIG_TSDMX, (VOID*)&mem);

		DBG_DUMP("HD_FILEIN_PARAM_MEM_RANGE addr:%lx   size:%lx \r\n", mem.addr, mem.size); /* size = blk_num + blk_size + reserve size*/
	}

}

static void _ImageApp_MoviePlay_FilePlay_Start(MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream)
{
	if(iamovieplay_GetPlayState() == IAMOVIEPLAY_STATE_OPEN)
		return;

    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_OPEN);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);

}

static void _ImageApp_MoviePlay_FilePlay_Pause(MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream)
{
	if(iamovieplay_GetPlayState() == IAMOVIEPLAY_STATE_PAUSE)
		return;

    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_PAUSE);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
}

static void _ImageApp_MoviePlay_FilePlay_Resume(MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream)
{
	if(iamovieplay_GetPlayState() == IAMOVIEPLAY_STATE_RESUME)
		return;

    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_RESUME);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
}

// checked
#if 0
static ER _imageapp_movieplay_common_init(UINT32 type)
{
	HD_RESULT ret;

	if ((ret = hd_common_init(type)) != HD_OK) {
		DBG_ERR("hd_common_init fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

// checked
static ER _imageapp_movieplay_common_uninit(void)
{
	HD_RESULT ret;

	if ((ret = hd_common_uninit()) != HD_OK) {
		DBG_ERR("hd_common_uninit fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}
#endif

// checked
static ER _imageapp_movieplay_mem_init(void)
{
	HD_RESULT ret;

	if (mem_cfg == NULL) {
		DBG_ERR("mem info not config yet\r\n");
		return E_SYS;
	}
	if ((ret = vendor_common_mem_relayout(mem_cfg)) != HD_OK) {
		DBG_ERR("vendor_common_mem_relayout fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

// checked
static ER _imageapp_movieplay_mem_uninit(void)
{
	return E_OK;
}

static HD_RESULT _imageapp_movieplay_set_bsdemux_param(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_BSDEMUX_PATH_CONFIG path_config = {0};
	HD_BSDEMUX_IN in_param = {0};
	HD_BSDEMUX_FILE file_param = {0};
	HD_BSDEMUX_REG_CALLBACK reg_cb = {0};
	HD_FILEIN_BLKINFO   filein_blk = {0};
	MOVIEPLAY_PLAY_OBJ      *p_obj       = &g_pImageStream->play_obj;

#if 0
	// create container
	ret = create_container_obj(file_fmt, &container_obj);
	if (ret != HD_OK) {
		DBG_ERR("create container fail=%d\n", ret);
		return ret;
	}
#endif

	// set path_config
	path_config.file_fmt = (p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_MOV) ? HD_BSDEMUX_FILE_FMT_MP4 : HD_BSDEMUX_FILE_FMT_TS;
	path_config.container = &g_movply_container_obj;
	ret = hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_PATH_CONFIG, &path_config);
	if (ret != HD_OK) {
		DBG_ERR("set path_config fail=%d\n", ret);
		return ret;
	}

	// set in_param
	in_param.video_codec = (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) ? HD_CODEC_TYPE_H264 : HD_CODEC_TYPE_H265;
	in_param.video_width = p_obj->width;   //1920
	in_param.video_height = p_obj->height; //1080
	in_param.video_en = TRUE;
	in_param.decrypt_key = 0;
	in_param.audio_en = TRUE;
	in_param.video_gop = p_obj->gop;		// for H.265
	in_param.desc_size = p_obj->desc_size;	// for H.265
	//DBGD(in_param.video_gop);
	//DBGD(in_param.desc_size);
	ret = hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_IN, &in_param);
	if (ret != HD_OK) {
		DBG_ERR("set in_param fail=%d\n", ret);
		return ret;
	}

	// set file_param (get from hd_filein_lib)
	hd_filein_get(path_id, HD_FILEIN_PARAM_BLK_INFO, (UINT32 *)&filein_blk);
	file_param.buf_start_addr = filein_blk.start_addr;
	file_param.blk_size = filein_blk.blk_size;
	file_param.tt_blknum = filein_blk.tt_blknum;
	ret = hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_FILE, &file_param);
	if (ret != HD_OK) {
		DBG_ERR("set file_param fail=%d\n", ret);
		return ret;
	}

	// set callback function
	reg_cb.callbackfunc = (HD_BSDEMUX_CALLBACK)_imageapp_movieplay_bsdemux_callback;
	ret = hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_REG_CALLBACK, &reg_cb);
	if (ret != HD_OK) {
		DBG_ERR("set reg_cb fail=%d\n", ret);
		return ret;
	}

	hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_FILESIZE, (VOID*)&p_obj->file_size);

	return HD_OK;
}

static HD_RESULT _imageapp_movieplay_set_dec_cfg(HD_PATH_ID video_dec_path, HD_DIM *p_max_dim, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_PATH_CONFIG video_path_cfg = {0};
	HD_VIDEODEC_IN video_in_param = {0};
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

	if (p_max_dim != NULL) {
		HD_H26XDEC_DESC desc_info = {0};
		// set videodec path config
		video_path_cfg.max_mem.codec_type = dec_type;
		video_path_cfg.max_mem.dim.w = p_max_dim->w;
		video_path_cfg.max_mem.dim.h = p_max_dim->h;
		//DBGD(video_dec_path);
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, &video_path_cfg);
		if (ret != HD_OK) {
			DBG_ERR("set desc PATH(%d) !!\r\n\r\n", ret);
			return ret;
		}


		//--- HD_VIDEODEC_PARAM_IN ---
		video_in_param.codec_type = dec_type;
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			DBG_ERR("set HD_VIDEODEC_PARAM_IN(%d) !!\r\n\r\n", ret);
			return ret;
		}

		// set videodec desc
		//memcpy((UINT8 *)bs_buf_curr, desc_buf, read_len);
		desc_info.addr = p_obj->desc_addr;//(UINT32)&desc_buf;
		desc_info.len = p_obj->desc_size;
		#if 0
		DBGH(desc_info.addr);
		DBGD(desc_info.len);
		{
			UINT8 *ptr = (UINT8 *)desc_info.addr;
			UINT32 i;

			for(i = 0; i < desc_info.len; i++) {
				DBG_DUMP("desc[%d]:0x%x\r\n", i, *ptr++);
			}
		}
		#endif
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
		if (ret != HD_OK) {
			DBG_ERR("set desc error(%d) !!\r\n\r\n", ret);
			return ret;
		}


		/* IMPORTANT!! */
		VENDOR_VIDEODEC_YUV_AUTO_DROP auto_drop = { .enable = 1 };
		ret = vendor_videodec_set(video_dec_path, VENDOR_VIDEODEC_PARAM_IN_YUV_AUTO_DROP, &auto_drop);
		if (ret != HD_OK) {
			DBG_ERR("set auto drop error(%d) !!\r\n\r\n", ret);
			return ret;
		}



	} else {
		DBG_ERR("set reg_cb fail=%d\n", ret);
		ret = HD_ERR_NG;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
static HD_RESULT _imageapp_movieplay_set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_SCALE;
		video_cfg_param.isp_id = HD_ISP_DONT_CARE;
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT _imageapp_movieplay_set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}
#endif

///////////////////////////////////////////////////////////////////////////////
#if 0
static HD_RESULT _imageapp_movieplay_set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type, HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	switch(out_type){
	case 0:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_CVBS;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.cvbs= HD_VIDEOOUT_CVBS_NTSC;
	break;
	case 1:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	break;
	case 2:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_HDMI;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.hdmi= hdmi_id;
	break;
	default:
		printf("not support out_type\r\n");
	break;
	}
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);

	*p_video_out_ctrl=video_out_ctrl ;
	return ret;
}
#endif

#if 1
static HD_RESULT _imageapp_movieplay_get_out_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return ret;
	}
	DBG_IND("##devcount %d\r\n", video_out_dev.max_dev_count);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT _imageapp_movieplay_set_out_queue(HD_PATH_ID video_out_ctrl, UINT32 queue_depth)
{
	HD_RESULT ret = HD_OK;
    VENDOR_VIDEOOUT_IN vendor_param={0};

    vendor_param.queue_depth= queue_depth;
    ret = vendor_videoout_set(video_out_ctrl, VENDOR_VIDEOOUT_ITEM_PARAM_IN, &vendor_param);
    if (ret != HD_OK) {
    	DBG_ERR("set VENDOR_VIDEOOUT_ITEM_PARAM_IN failed!\r\n");
            return ret;
    }

	return ret;
}

static HD_RESULT _imageapp_movieplay_set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_param2 = {0};
	SIZECONVERT_INFO     CovtInfo = {0};
	HD_URECT rect = {0};
	UINT32 idx = 0;
	UINT32 disp_rotate;

	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("get HD_VIDEOOUT_PARAM_IN failed!\r\n");
		return ret;
	}

	disp_rotate = video_out_param.dir;

	CovtInfo.uiSrcWidth  = g_tVideoPlaybackStreamInfo[idx].proc_max_dim.w;
	CovtInfo.uiSrcHeight = g_tVideoPlaybackStreamInfo[idx].proc_max_dim.h;
	if (disp_rotate == HD_VIDEO_DIR_ROTATE_90 || disp_rotate == HD_VIDEO_DIR_ROTATE_270) {
		CovtInfo.uiDstWidth  = g_tVideoPlaybackStreamInfo[idx].out_syscaps.output_dim.h;
		CovtInfo.uiDstHeight = g_tVideoPlaybackStreamInfo[idx].out_syscaps.output_dim.w;
	} else {
		CovtInfo.uiDstWidth  = g_tVideoPlaybackStreamInfo[idx].out_syscaps.output_dim.w;
		CovtInfo.uiDstHeight = g_tVideoPlaybackStreamInfo[idx].out_syscaps.output_dim.h;
	}
	CovtInfo.uiDstWRatio = g_movplay_DispLink[idx].vout_ratio.w;
	CovtInfo.uiDstHRatio = g_movplay_DispLink[idx].vout_ratio.h;
	CovtInfo.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&CovtInfo);

	//#NT#20210715#Philex_lin--begin
	// width is 16-align, height is 8-align because gximg rotation constraint
	if (disp_rotate == HD_VIDEO_DIR_ROTATE_90 || disp_rotate == HD_VIDEO_DIR_ROTATE_270) {
		CovtInfo.uiOutWidth = ALIGN_CEIL_16(CovtInfo.uiOutWidth);
		CovtInfo.uiOutHeight = ALIGN_CEIL_8(CovtInfo.uiOutHeight);
	}
	//#NT#20210715#Philex_lin--end

	DBG_DUMP("Vout param: x:%d, y:%d, w:%d, h:%d\r\n", CovtInfo.uiOutX, CovtInfo.uiOutY, p_dim->w, p_dim->h);
	if (CovtInfo.uiOutWidth && CovtInfo.uiOutHeight) {
		rect.x = CovtInfo.uiOutX;
		rect.y = CovtInfo.uiOutY;
		rect.w = CovtInfo.uiOutWidth;
		rect.h = CovtInfo.uiOutHeight;
	} else {
		rect.x = 0;
		rect.y = 0;
		rect.w = g_movplay_DispLink[idx].vout_syscaps.output_dim.w;
		rect.h = g_movplay_DispLink[idx].vout_syscaps.output_dim.h;
	}

	video_out_param.dim.w = rect.w;
	video_out_param.dim.h = rect.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = disp_rotate;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	video_out_param2.visible = TRUE;
	if (disp_rotate == HD_VIDEO_DIR_ROTATE_90 || disp_rotate == HD_VIDEO_DIR_ROTATE_270) {
		video_out_param2.rect.x = rect.y;
		video_out_param2.rect.y = rect.x;
		video_out_param2.rect.w = rect.h;
		video_out_param2.rect.h = rect.w;
	} else {
		video_out_param2.rect.x = rect.x;
		video_out_param2.rect.y = rect.y;
		video_out_param2.rect.w = rect.w;
		video_out_param2.rect.h = rect.h;
	}
	video_out_param2.layer = HD_LAYER1;
	if ((ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param2)) != HD_OK) {
		DBG_ERR("hd_videoout_set fail(%d)\r\n", ret);
	}
	//printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);

	return ret;
}
#endif

static HD_RESULT _imageapp_movieplay_open_video_module(MOVIEPLAY_VIDEO_PLAYBACK *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;

	// set videoproc config
	#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	ret = _imageapp_movieplay_set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		DBG_ERR("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	#endif

	// set videoout config
	#if 0
	ret = _imageapp_movieplay_set_out_cfg(&p_stream->out_ctrl, out_type, p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	#endif

	#if 1
	if ((ret = hd_videodec_open(HD_VIDEODEC_0_IN_0, HD_VIDEODEC_0_OUT_0, &p_stream->dec_path)) != HD_OK) {
		DBG_ERR("hd_videodec_open fail=%d\n", ret);
		return ret;
	}
	#endif

	#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK) {
		DBG_ERR("hd_videoproc_open fail=%d\n", ret);
		return ret;
	}
	#endif

	#if 0 // Already open in PB mode.
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK) {
		printf("hd_videoout_open fail=%d\n", ret);
		return ret;
	}
	#endif

	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT _imageapp_movieplay_set_audio_dec_cfg(HD_PATH_ID audio_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIODEC_PATH_CONFIG audio_path_cfg = {0};
    MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;

	// set audiodec path config
	audio_path_cfg.max_mem.codec_type = dec_type;
	#if 0
	audio_path_cfg.max_mem.sample_rate = HD_AUDIO_SR_32000;
	audio_path_cfg.max_mem.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_path_cfg.max_mem.mode = HD_AUDIO_SOUND_MODE_STEREO;
	#else
	//DBGD(p_play_obj->aud_sr);
	//DBGD(p_play_obj->aud_chs);
	audio_path_cfg.max_mem.sample_rate = p_play_obj->aud_sr;
	audio_path_cfg.max_mem.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_path_cfg.max_mem.mode = p_play_obj->aud_chs;
	#endif
	ret = hd_audiodec_set(audio_dec_path, HD_AUDIODEC_PARAM_PATH_CONFIG, &audio_path_cfg);

	return ret;
}

static HD_RESULT _imageapp_movieplay_set_audio_dec_param(HD_PATH_ID audio_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIODEC_IN audio_in_param = {0};
	UINT32 aac_adts = FALSE;
    MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;

	audio_in_param.codec_type = dec_type;
	audio_in_param.sample_rate = p_play_obj->aud_sr;
	audio_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_in_param.mode = p_play_obj->aud_chs;
	ret = hd_audiodec_set(audio_dec_path, HD_AUDIODEC_PARAM_IN, &audio_in_param);
	if (ret != HD_OK) {
		DBG_ERR("set_dec_param_in = %d\r\n", ret);
		return ret;
	}

	ret = vendor_audiodec_set(audio_dec_path, VENDOR_AUDIODEC_ITEM_CODEC_HEADER, &aac_adts);
	if (ret != HD_OK) {
		DBG_ERR("set_codec_header = %d\r\n", ret);
		return ret;
	}

	return ret;
}

static HD_RESULT _imageapp_movieplay_set_audio_out_cfg(HD_PATH_ID *p_audio_out_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_PATH_ID audio_out_ctrl = 0;
	HD_AUDIOOUT_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_drv_cfg = {0};
    MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;
    static BOOL is_pwr_en_done = FALSE;

	ret = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &audio_out_ctrl); //open this for device control

	if (ret != HD_OK) {
		DBG_ERR("hd_audioout_open fail\r\n");
		return ret;
	}

	/* set audio out maximum parameters */
	audio_dev_cfg.out_max.sample_rate = p_play_obj->aud_sr;
	audio_dev_cfg.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_dev_cfg.out_max.mode = p_play_obj->aud_chs;
	audio_dev_cfg.frame_sample_max = 1024;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_dev_cfg);
	if (ret != HD_OK) {
		DBG_ERR("HD_AUDIOOUT_PARAM_DEV_CONFIG failed(%x)\r\n", ret);
		return ret;
	}

	/* set audio capture maximum parameters */
	audio_drv_cfg.mono = (g_audioout_info_ex.drv_cfg.mono < HD_AUDIO_MONO_MAX_CNT) ? g_audioout_info_ex.drv_cfg.mono : HD_AUDIO_MONO_LEFT;
	audio_drv_cfg.output = (g_audioout_info_ex.drv_cfg.output < HD_AUDIOOUT_OUTPUT_MAX_CNT) ? g_audioout_info_ex.drv_cfg.output : HD_AUDIOOUT_OUTPUT_SPK;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_drv_cfg);
	if (ret != HD_OK) {
		DBG_ERR("HD_AUDIOOUT_PARAM_DRV_CONFIG failed(%x)\r\n", ret);
		return ret;
	}

	if((is_pwr_en_done == FALSE) && (g_audioout_info_ex.pwr_en == TRUE)){

	    ret = vendor_audioout_set(audio_out_ctrl, VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE, (VOID *)&g_audioout_info_ex.pwr_en);
	    if (ret != HD_OK) {
			DBG_ERR("VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE failed(%x)\r\n", ret);
			return ret;
	    }

	    is_pwr_en_done = TRUE;
	}
	else if((is_pwr_en_done == TRUE) && (g_audioout_info_ex.pwr_en == FALSE)){

	    ret = vendor_audioout_set(audio_out_ctrl, VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE, (VOID *)&g_audioout_info_ex.pwr_en);
	    if (ret != HD_OK) {
			DBG_ERR("VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE failed(%x)\r\n", ret);
			return ret;
	    }

	    is_pwr_en_done = FALSE;
	}


	*p_audio_out_ctrl = audio_out_ctrl;
	return ret;
}

static HD_RESULT _imageapp_movieplay_set_audio_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_OUT audio_out_param = {0};
    MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;
	BOOL enable = TRUE;

	// set audioout output parameters
	audio_out_param.sample_rate = p_play_obj->aud_sr;
	audio_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_param.mode = p_play_obj->aud_chs;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_OUT, &audio_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audioout volume
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &g_audioout_volume);

	// TBD ???
	vendor_audioout_set(audio_out_ctrl, VENDOR_AUDIOOUT_ITEM_WAIT_PUSH, (VOID *)&enable);

	return ret;
}

static HD_RESULT _imageapp_movieplay_open_audio_module(MOVIEPLAY_AUDIO_PLAYBACK *p_stream)
{
	HD_RESULT ret;
	// set audioout config
	ret = _imageapp_movieplay_set_audio_out_cfg(&p_stream->out_ctrl);
	if (ret != HD_OK) {
		DBG_ERR("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiodec_open(HD_AUDIODEC_0_IN_0, HD_AUDIODEC_0_OUT_0, &p_stream->dec_path)) != HD_OK) {
		DBG_ERR("hd_audiodec_open fail\n");
        return ret;
	}
	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK) {
		DBG_ERR("hd_audioout_open fail\n");
        return ret;
	}

    return HD_OK;
}

static void filein_cb(CHAR *Name, UINT32 event_id, UINT32 value)
{

	if(event_id == ISF_FILEIN_EVENT_READ_ONE_BLK){

		HD_PATH_ID path_id = 0;

		HD_FILEIN_READBUF_INFO read_info = *((HD_FILEIN_READBUF_INFO*)value);

		MEM_RANGE mem = {read_info.addr, read_info.size};

		hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_TRIG_TSDMX, (VOID*)&mem);
	}

}

static ER _imageapp_movieplay_module_init(void)
{
	ER result = E_OK;
	HD_PATH_ID path_id = 0;
	HD_RESULT ret;
	UINT32 	  out_type = 0;
	HD_DIM    main_dim = {0};
    MOVIEPLAY_PLAY_OBJ      *p_obj       = &g_pImageStream->play_obj;

#if 0 // To do: Need hd_filein_init().
	if ((ret = hd_filein_init()) != HD_OK) {
		DBG_ERR("hd_filein_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
#endif
	hd_filein_set(path_id, HD_FILEIN_PARAM_FILEHDL,  (UINT32)p_obj->file_handle/*Set file handle*/);
	hd_filein_set(path_id, HD_FILEIN_PARAM_FILESIZE, p_obj->file_size);
	hd_filein_set(path_id, HD_FILEIN_PARAM_FILEDUR,  p_obj->file_dur);
	hd_filein_set(path_id, HD_FILEIN_PARAM_FILEFMT,  p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_MOV ? MEDIA_FILEFORMAT_MP4 : MEDIA_FILEFORMAT_TS);
	hd_filein_set(path_id, HD_FILEIN_PARAM_VDO_FR,   p_obj->vdo_fr);
	hd_filein_set(path_id, HD_FILEIN_PARAM_CONTAINER, (UINT32)&g_movply_container_obj);
	hd_filein_set(path_id, HD_FILEIN_PARAM_PL_BLKNUM, 7/*HD_FILEIN_PARAM_PL_BLKNUM*/);
	hd_filein_set(path_id, HD_FILEIN_PARAM_AUD_FR,   p_obj->aud_fr);
	hd_filein_set(path_id, HD_FILEIN_PARAM_VDO_TTFRM,   p_obj->vdo_ttfrm);
	hd_filein_set(path_id, HD_FILEIN_PARAM_AUD_TTFRM,   p_obj->aud_ttfrm);
	//user optional setting
	hd_filein_set(path_id, HD_FILEIN_PARAM_USER_BLK_DIRECTLY, 1);
	hd_filein_set(path_id, HD_FILEIN_PARAM_BLK_SIZE, 0x200000);
	hd_filein_set(path_id, HD_FILEIN_PARAM_EVENT_CB, (UINT32)filein_cb);

	ret = hd_filein_open(path_id);
	if (ret != HD_OK) {
		DBG_ERR("FileIn open fail !\r\n");
	}

	if ((ret = hd_bsdemux_init()) != HD_OK) {
		DBG_ERR("hd_bsdemux_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	// set param
	ret = _imageapp_movieplay_set_bsdemux_param(path_id);
	if (ret != HD_OK) {
		DBG_ERR("set param fail=%d\n", ret);
		return ret;
	}
	// open
	ret = hd_bsdemux_open(path_id);
	if (ret != HD_OK) {
		DBG_ERR("hd_bsdemux_open fail=%d\n", ret);
		return ret;
	}

#if 1
	if ((ret = hd_videodec_init()) != HD_OK) {
		DBG_ERR("hd_videodec_init fail(%d)\r\n", ret);
		return ret;
	}
#endif

	#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	if ((ret = hd_videoproc_init()) != HD_OK) {
		DBG_ERR("hd_videoproc_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	#endif

	if ((ret = hd_audiodec_init()) != HD_OK) {
		DBG_ERR("hd_audiodec_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_audioout_init()) != HD_OK) {
		if (ret != HD_ERR_UNINIT) {
			DBG_ERR("hd_audiodec_init fail(%d)\r\n", ret);
			result = E_SYS;
		}
		g_movply_ado_out_init = FALSE;
	} else {
		g_movply_ado_out_init = TRUE;
	}

	// open video_playback modules (main)
	g_tVideoPlaybackStreamInfo[0].proc_max_dim.w = ALIGN_CEIL_64(p_obj->width); //assign by user
	#if 0
	if (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) {
		g_tVideoPlaybackStreamInfo[0].proc_max_dim.h = ALIGN_CEIL_16(p_obj->height); //assign by user
	} else { // for H.265
		g_tVideoPlaybackStreamInfo[0].proc_max_dim.h = ALIGN_CEIL_64(p_obj->height); //assign by user
	}
	#else // No need to change different aspect ratio for videoproc.
		g_tVideoPlaybackStreamInfo[0].proc_max_dim.h = ALIGN_CEIL_16(p_obj->height); //assign by user
	#endif
	out_type = 1;                          //assign by user, 0:CVBS, 1:LCD, 2:HDMI
	g_tVideoPlaybackStreamInfo[0].hdmi_id = HD_VIDEOOUT_HDMI_1920X1080I60; //default
	ret = _imageapp_movieplay_open_video_module(&g_tVideoPlaybackStreamInfo[0], &g_tVideoPlaybackStreamInfo[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		DBG_ERR("open video module fail=%d\n", ret);
		return E_SYS;
	}
	p_obj->vdec_path = g_tVideoPlaybackStreamInfo[0].dec_path;
	#if 0
	DBGD(g_tVideoPlaybackStreamInfo[0].dec_path);
	DBGD(p_obj->vdec_path);
	#endif

	// get videoout capability
	#if 1
	ret = _imageapp_movieplay_get_out_caps(g_movplay_DispLink[0].vout_p_ctrl, &g_tVideoPlaybackStreamInfo[0].out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get out-caps fail=%d\n", ret);
		//return E_SYS;
	}

	// check display buffer size
	if ((g_tVideoPlaybackStreamInfo[0].out_syscaps.output_dim.w * g_tVideoPlaybackStreamInfo[0].out_syscaps.output_dim.h) > (DISP_SIZE_W * DISP_SIZE_H)) {
		DBG_ERR("display buffer size is not enough!\n");
		DBG_ERR("display buffer size(%dx%d) < panel size(%dx%d)\r\n",
			g_tVideoPlaybackStreamInfo[0].out_syscaps.output_dim.w,
			g_tVideoPlaybackStreamInfo[0].out_syscaps.output_dim.h,
			DISP_SIZE_W,
			DISP_SIZE_H);
		return E_SYS;
	}
	g_tVideoPlaybackStreamInfo[0].out_max_dim = g_tVideoPlaybackStreamInfo[0].out_syscaps.output_dim;
	#endif

	// set videodec config (main)
	// assign parameter by program options
	main_dim.w = ALIGN_CEIL_64(p_obj->width);
	if (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) {
		main_dim.h = ALIGN_CEIL_16(p_obj->height);
	} else {
		main_dim.h = ALIGN_CEIL_64(p_obj->height);
	}
	g_tVideoPlaybackStreamInfo[0].dec_max_dim.w = main_dim.w;
	g_tVideoPlaybackStreamInfo[0].dec_max_dim.h = main_dim.h;
	g_tVideoPlaybackStreamInfo[0].dec_type = (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) ? HD_CODEC_TYPE_H264 : HD_CODEC_TYPE_H265;
	ret = _imageapp_movieplay_set_dec_cfg(g_tVideoPlaybackStreamInfo[0].dec_path, &g_tVideoPlaybackStreamInfo[0].dec_max_dim, g_tVideoPlaybackStreamInfo[0].dec_type);
	if (ret != HD_OK) {
		DBG_ERR("set dec-cfg fail=%d\n", ret);
		return E_SYS;
	}

	// set videodec parameter (main)
	#if 0 // To do ???
	ret = set_dec_param(g_tVideoPlaybackStreamInfo[0].dec_path, g_tVideoPlaybackStreamInfo[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec fail=%d\n", ret);
		return E_SYS;
	}
	#endif

	// set videoproc parameter (main)
	#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	ret = _imageapp_movieplay_set_proc_param(g_tVideoPlaybackStreamInfo[0].proc_path, NULL);
	if (ret != HD_OK) {
		DBG_ERR("set proc fail=%d\n", ret);
		return E_SYS;
	}
	#endif

	// set videoout parameter (main)
	#if 1
	g_tVideoPlaybackStreamInfo[0].out_dim.w = g_tVideoPlaybackStreamInfo[0].out_max_dim.w; //using device max dim.w
	g_tVideoPlaybackStreamInfo[0].out_dim.h = g_tVideoPlaybackStreamInfo[0].out_max_dim.h; //using device max dim.h
	g_tVideoPlaybackStreamInfo[0].out_path = g_movplay_DispLink[0].vout_p[0];
	g_tVideoPlaybackStreamInfo[0].out_queue_depth = g_movplay_DispLink[0].vout_queue_depth;

	ret = _imageapp_movieplay_set_out_param(g_tVideoPlaybackStreamInfo[0].out_path, &g_tVideoPlaybackStreamInfo[0].out_dim);
	if (ret != HD_OK) {
		DBG_ERR("set out fail=%d\n", ret);
		//return E_SYS;
	}

	ret = _imageapp_movieplay_set_out_queue(g_tVideoPlaybackStreamInfo[0].out_path, g_tVideoPlaybackStreamInfo[0].out_queue_depth);
	if (ret != HD_OK) {
		DBG_ERR("set out fail=%d\n", ret);
		//return E_SYS;
	}

	#else
	g_tVideoPlaybackStreamInfo[0].out_path = g_pImageStream->p_cfg_disp_info->vout_path;
	#endif

	// bind video_playback modules (main)
	#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	hd_videodec_bind(HD_VIDEODEC_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
	#endif

	#if 1 /*** Init aduio module(s). ***/
	if(p_obj->aud_en) {
		ret = _imageapp_movieplay_open_audio_module(&g_tAudioPlaybackStreamInfo[0]);
		if (ret != HD_OK) {
			DBG_ERR("open audio module fail=%d\n", ret);
			return E_SYS;
		}

		switch(p_obj->aud_codec)
		{
		case MEDIAAUDIO_CODEC_SOWT:
			g_tAudioPlaybackStreamInfo[0].dec_type = HD_AUDIO_CODEC_PCM;
			break;

		case MEDIAAUDIO_CODEC_ULAW:
			g_tAudioPlaybackStreamInfo[0].dec_type = HD_AUDIO_CODEC_ULAW;
			break;

		case MEDIAAUDIO_CODEC_ALAW:
			g_tAudioPlaybackStreamInfo[0].dec_type = HD_AUDIO_CODEC_ALAW;
			break;

		default:
		case MEDIAAUDIO_CODEC_MP4A:
			g_tAudioPlaybackStreamInfo[0].dec_type = HD_AUDIO_CODEC_AAC;
			break;

		}

//		g_tAudioPlaybackStreamInfo[0].dec_type = (p_obj->aud_codec == MEDIAAUDIO_CODEC_MP4A) ? HD_AUDIO_CODEC_AAC : HD_AUDIO_CODEC_PCM;
//		g_tAudioPlaybackStreamInfo[0].dec_type = HD_AUDIO_CODEC_AAC;
		p_obj->aud_codec = g_tAudioPlaybackStreamInfo[0].dec_type;
		p_obj->adec_path = g_tAudioPlaybackStreamInfo[0].dec_path;
		p_obj->aout_path  = g_tAudioPlaybackStreamInfo[0].out_path;
		p_obj->aout_ctrl  = g_tAudioPlaybackStreamInfo[0].out_ctrl;

		// set audiodec config
		ret = _imageapp_movieplay_set_audio_dec_cfg(g_tAudioPlaybackStreamInfo[0].dec_path, g_tAudioPlaybackStreamInfo[0].dec_type);
		if (ret != HD_OK) {
			DBG_ERR("set dec-cfg fail=%d\n", ret);
		}

		// set audiodec parameter
	    ret = _imageapp_movieplay_set_audio_dec_param(g_tAudioPlaybackStreamInfo[0].dec_path, g_tAudioPlaybackStreamInfo[0].dec_type);
		if (ret != HD_OK) {
			DBG_ERR("set dec fail=%d\n", ret);
		}

		// set audioout parameter
	    ret = _imageapp_movieplay_set_audio_out_param(g_tAudioPlaybackStreamInfo[0].out_ctrl, g_tAudioPlaybackStreamInfo[0].out_path);
		if (ret != HD_OK) {
			DBG_ERR("set out fail=%d\n", ret);
		}

	    // bind audio_playback modules
	    #if 0
	    hd_audiodec_bind(HD_AUDIODEC_0_OUT_0, HD_AUDIOOUT_0_IN_0);
		#endif

		// start audio_playback modules
		{
			HD_RESULT hr;

		    hr = hd_audiodec_start(g_tAudioPlaybackStreamInfo[0].dec_path);
			if(hr != HD_OK) {
				DBG_ERR("audiodec_start failed\r\n");
			}

			hr = hd_audioout_start(g_tAudioPlaybackStreamInfo[0].out_path);
			if(hr != HD_OK) {
				DBG_ERR("audioout_start failed\r\n");
			}
		}
	}
	#endif


	// start video_playback modules (main)
	hd_videodec_start(g_tVideoPlaybackStreamInfo[0].dec_path);
	#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	hd_videoproc_start(g_tVideoPlaybackStreamInfo[0].proc_path);
	hd_videoout_start(g_tVideoPlaybackStreamInfo[0].out_path);
	#endif

	return result;
}

static ER _imageapp_movieplay_module_uninit(void)
{
	ER result = E_OK;
	HD_PATH_ID path_id = 0;
	HD_RESULT ret;
    MOVIEPLAY_PLAY_OBJ *p_obj = &g_pImageStream->play_obj;
	VENDOR_VIDEOOUT_FUNC_CONFIG videoout_cfg ={0};

    /* set vout stop would keep last frame for change mode */
    videoout_cfg.in_func = VENDOR_VIDEOOUT_INFUNC_KEEP_LAST;
	ret = vendor_videoout_set(g_tVideoPlaybackStreamInfo[0].out_path, VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG, &videoout_cfg);
	if(ret != HD_OK){
		DBG_WRN("vendor_videoout_set failed(path=%lx, ret=%d, in_func=VENDOR_VIDEOOUT_INFUNC_KEEP_LAST)\n", g_tVideoPlaybackStreamInfo[0].out_path, ret);
		result = E_SYS;
	}

	// stop video_playback modules (main)
	/*****************************************************************************************************************
	 * if rapidly trigger select key twice(start->stop), bs demux callback with flag FLG_IAMOVIEPLAY_DISP_TRIG may not be invoked
	 * due to task destroyed. therefore post event MOVIEPLAY_EVENT_ONE_DISPLAYFRAME won't be send to start videoout.
	 * skip error code HD_ERR_NOT_START to prevent this case from abnormal stopped.
	 *****************************************************************************************************************/
	if(g_tVideoPlaybackStreamInfo[0].out_path){
		ret = hd_videoout_stop(g_tVideoPlaybackStreamInfo[0].out_path);
		if (ret != HD_OK && ret != HD_ERR_NOT_START) {
			DBG_WRN("hd_videoout_stop failed(path=%lx, ret=%d)\n", g_tVideoPlaybackStreamInfo[0].out_path, ret);
			result = E_SYS;
		}
	}

	if(p_obj->vdec_path){
		ret = hd_videodec_stop(p_obj->vdec_path);
		if (ret != HD_OK && ret != HD_ERR_NOT_START) {
			DBG_WRN("hd_videodec_stop failed(path=%lx, ret=%d)\n", p_obj->vdec_path, ret);
			result = E_SYS;
		}
	}

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	if ((ret = hd_videoproc_stop(g_tVideoPlaybackStreamInfo[0].proc_path)) != HD_OK) {
		DBG_WRN("hd_videodec_stop failed(path=%lx, ret=%d)\n", g_tVideoPlaybackStreamInfo[0].proc_path, ret);
		result = E_SYS;
	}

	hd_videodec_unbind(HD_VIDEODEC_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
#endif

	ret = hd_filein_close(path_id);
    if(ret!= HD_OK) {
		DBG_WRN("hd_filein_close failed(path=%lx, ret=%d)\r\n", path_id, ret);
		result = E_SYS;
    }

    if(p_obj->vdec_path){
		ret = hd_videodec_close(p_obj->vdec_path);
		if(ret != HD_OK && ret != HD_ERR_NOT_OPEN) {
			DBG_WRN("hd_videodec_close failed(path=%lx, ret=%d)\r\n", p_obj->vdec_path, ret);
			result = E_SYS;
		}

		p_obj->vdec_path = 0;
    }

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)

	ret = hd_videoproc_close(g_tVideoPlaybackStreamInfo[0].proc_path);
	if (ret != HD_OK) {
		DBG_WRN("hd_videoproc_close failed(path=%lx, ret=%d)\r\n", g_tVideoPlaybackStreamInfo[0].proc_path, ret);
		result = E_SYS;
	}
#endif

	ret = hd_bsdemux_close(path_id);
	if (ret != HD_OK) {
		DBG_WRN("hd_bsdemux_close failed(path=%lx, ret=%d)\n", path_id, ret);
		result = E_SYS;
	}

	if(p_obj->aud_en) {

		if(g_tAudioPlaybackStreamInfo[0].dec_path){
			ret = hd_audiodec_stop(g_tAudioPlaybackStreamInfo[0].dec_path);
			if(ret != HD_OK && ret != HD_ERR_NOT_START) {
				DBG_WRN("hd_audiodec_stop failed(path=%lx, ret=%d)\n", g_tAudioPlaybackStreamInfo[0].dec_path, ret);
				result = E_SYS;
			}
		}

		if(g_tAudioPlaybackStreamInfo[0].out_path){
			ret = hd_audioout_stop(g_tAudioPlaybackStreamInfo[0].out_path);
			if(ret != HD_OK && ret != HD_ERR_NOT_START) {
				DBG_WRN("hd_audioout_stop failed(path=%lx, ret=%d)\n", g_tAudioPlaybackStreamInfo[0].out_path, ret);
				result = E_SYS;
			}
		}

		if(g_tAudioPlaybackStreamInfo[0].dec_path){
			ret = hd_audiodec_close(g_tAudioPlaybackStreamInfo[0].dec_path);
			if(ret != HD_OK && ret != HD_ERR_NOT_OPEN) {
				DBG_WRN("hd_audiodec_close failed(path=%lx, ret=%d)\n", g_tAudioPlaybackStreamInfo[0].dec_path, ret);
				result = E_SYS;
			}

			g_tAudioPlaybackStreamInfo[0].dec_path = 0;
		}

		if(g_tAudioPlaybackStreamInfo[0].out_path){
			ret = hd_audioout_close(g_tAudioPlaybackStreamInfo[0].out_path);
			if(ret != HD_OK && ret != HD_ERR_NOT_OPEN) {
				DBG_WRN("hd_audioout_close failed(path=%lx, ret=%d)\n", g_tAudioPlaybackStreamInfo[0].out_path, ret);
				result = E_SYS;
			}

			g_tAudioPlaybackStreamInfo[0].out_path = 0;
		}
	}

	if ((ret = hd_filein_uninit()) != HD_OK) {
		DBG_WRN("hd_filein_uninit failed(ret=%d)\r\n", ret);
		result = E_SYS;
	}

	if ((ret = hd_bsdemux_uninit()) != HD_OK) {
		DBG_WRN("hd_bsdemux_uninit failed(ret=%d)\r\n", ret);
		result = E_SYS;
	}
#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == ENABLE)
	if ((ret = hd_videoproc_uninit()) != HD_OK) {
		DBG_WRN("hd_videoproc_uninit failed(ret=%d)\r\n", ret);
		result = E_SYS;
	}
#endif

	if ((ret = hd_videodec_uninit()) != HD_OK) {
		DBG_WRN("hd_videodec_uninit failed(ret=%d)\r\n", ret);
		result = E_SYS;
	}

	if ((ret = hd_audiodec_uninit()) != HD_OK) {
		DBG_WRN("hd_audiodec_uninit failed(ret=%d)\r\n", ret);
		result = E_SYS;
	}

	if (g_movply_ado_out_init) {
		if ((ret = hd_audioout_uninit()) != HD_OK) {
			DBG_WRN("hd_audiodec_uninit failed(ret=%d)\r\n", ret);
			result = E_SYS;
		}
	}

	return result;
}

// checked
static ER _imageapp_movieplay_init(void)
{
#if 0
	// init hdal
	if (_imageapp_movieplay_common_init(0) != HD_OK) {
		return E_SYS;
	}
#endif

#if 0 // move ahead of _ImageApp_MoviePlay_New_HeaderBuf() function was called.
	// init memory
	if (_imageapp_movieplay_mem_init() != HD_OK) {
		//_imageapp_movieplay_common_uninit();
		return E_SYS;
	}
#endif
	// init module
	if (_imageapp_movieplay_module_init() != HD_OK) {
		_imageapp_movieplay_mem_uninit();
		//_imageapp_movieplay_common_uninit();
		return E_SYS;
	}
	return HD_OK;
}

static ER _imageapp_movieplay_uninit(void)
{
	ER result = E_OK;

	if (_imageapp_movieplay_module_uninit() != HD_OK) {
		result = E_SYS;
	}

	return result;
}

ISF_RV ImageApp_MoviePlay_Open(void)
{
	//UINT32						i;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = NULL;

	pImageStream = g_pImageStream;

	if (pImageStream == NULL) {
		DBG_ERR("pImageStream is NULL, please call ImageApp_MoviePlay_Config() first\r\n");
		return ISF_ERR_FAILED;
	}

	if (g_movply_config_fileplay_ok == 0) {
		DBG_ERR("MOVIEPLAY_CONFIG_FILEPLAY_INFO is not configured\r\n");
		return ISF_ERR_FAILED;
	}

	if(_imageapp_movieplay_init() != HD_OK){
		DBG_ERR("_imageapp_movieplay_init failed!\r\n");
		return ISF_ERR_FAILED;
	}

    // Start fineIn and BsDemux task.
    iamovieplay_InstallID();
    iamovieplay_TskStart();

	if (pImageStream->id < _CFG_FILEPLAY_ID_MAX) {
		#if 0
		for (i = MOVIEPLAY_DISP_ID_MIN; i < MOVIEPLAY_DISP_ID_MAX; i++) {
			if (g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].enable) {
				pImageStream->p_disp_info = &g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN];
				_ImageApp_MoviePlay_FilePlay_OpenStream(pImageStream);
			}
		}
		#else
		_ImageApp_MoviePlay_FilePlay_OpenStream(pImageStream);

		MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;
		
		if(p_play_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS){
			p_play_obj->gop = TsReadLib_GetNextIFrame(0, 0);
			DBG_DUMP("^G NMP:(vid) gop=%lu\r\n", p_play_obj->gop);
		}
		
		#endif
	}

	g_movply_opened = 1;

	return ISF_OK;
}

ISF_RV ImageApp_MoviePlay_Close(void)
{
    MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = NULL;
    HD_RESULT		ret;
    BOOL			is_ok = TRUE;

	pImageStream = g_pImageStream;

    // Reset variable for step play.
    g_movply_vdo_play_status = _CFG_PLAY_STS_NORMAL;
    g_enter_step_state       = FALSE;
    memset(g_movply_vdo_rawqueue_idx_table, 0, sizeof(g_movply_vdo_rawqueue_idx_table));
    ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_STEP_PLAY, FALSE);


    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_STOP);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
    iamovieplay_TskDestroy();
    iamovieplay_UninstallID();
	_imageapp_movieplay_uninit();

    if (pImageStream->vb_pool_va != 0) {
        ret = hd_common_mem_free((UINT32)pImageStream->vb_pool_pa, (void *)pImageStream->vb_pool_va);
        if (ret != HD_OK) {
        	DBG_WRN("IAMovPly_StrmInfo free mem failed!(ret=%d)\r\n", ret);
        	is_ok = FALSE;
        }
	}

	// free main memory
    if (g_movply_pool_main_va_adr != 0) {
        ret = hd_common_mem_free((UINT32)g_movply_pool_main_phy_adr, (void *)g_movply_pool_main_va_adr);
        if (ret != HD_OK) {
        	DBG_WRN("movply_main free mem failed!(ret=%d)\r\n", ret);
        	is_ok = FALSE;
        }

        g_movply_pool_main_phy_adr = 0;
        g_movply_pool_main_va_adr = 0;
	}

	// uninit memory
	if ((ret = _imageapp_movieplay_mem_uninit()) != HD_OK) {
		DBG_WRN("movply_main mem_uninit failed! (%d)\r\n", ret);
        is_ok = FALSE;
	}

	g_pImageStream = NULL;
	g_movply_opened = 0;
	g_movply_config_fileplay_ok = 0;

	return is_ok == TRUE? ISF_OK : ISF_ERR_FAILED;
}

ISF_RV ImageApp_MoviePlay_Start(void)
{
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = NULL;

	pImageStream = g_pImageStream;

	if (pImageStream == NULL) {
		DBG_ERR("pImageStream is NULL, please call ImageApp_MoviePlay_Config() first\r\n");
		return ISF_ERR_FAILED;
	}

	if (g_movply_config_fileplay_ok == 0) {
		DBG_ERR("MOVIEPLAY_CONFIG_FILEPLAY_INFO is not configured\r\n");
		return ISF_ERR_FAILED;
	}

	if(g_movply_opened == 0){
		DBG_ERR("No stream are opened, can't play video.\r\n");
		return ISF_ERR_FAILED;
	}

	//DBGD(pImageStream->id);
	if (pImageStream->id < _CFG_FILEPLAY_ID_MAX) {
		#if 0
		for (i = MOVIEPLAY_DISP_ID_MIN; i < MOVIEPLAY_DISP_ID_MAX; i++) {
			if (g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].enable) {
				pImageStream->p_disp_info = &g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN];
				_ImageApp_MoviePlay_FilePlay_Start(pImageStream);
			}
		}
		#else
		_ImageApp_MoviePlay_FilePlay_Start(pImageStream);
		#endif
	}

	return E_OK;
}

ISF_RV ImageApp_MoviePlay_Config(UINT32 config_id, UINT32 value)
{
	ISF_RV ret = ISF_OK;

	switch (config_id) {
	case MOVIEPLAY_CONFIG_DISP_INFO:

		if (value) {
			MOVIEPLAY_CFG_DISP_INFO *ptr = (MOVIEPLAY_CFG_DISP_INFO*) value;

			g_movplay_DispLink[0].vout_p_ctrl = ptr->vout_ctrl;
			g_movplay_DispLink[0].vout_p[0] = ptr->vout_path;
			g_movplay_DispLink[0].vout_ratio.w = ptr->ratio.w;
			g_movplay_DispLink[0].vout_ratio.h = ptr->ratio.h;
			g_movplay_DispLink[0].vout_queue_depth = ptr->queue_depth;
		}

		break;

	case MOVIEPLAY_CONFIG_FILEPLAY_INFO:
		if (value) {
           	// init memory
        	if (_imageapp_movieplay_mem_init() != HD_OK) {
        		//_imageapp_movieplay_common_uninit();
        		return E_SYS;
        	}

			ret = _ImageApp_MoviePlay_ConfigFilePlay((MOVIEPLAY_FILEPLAY_INFO *) value);
			if (ret == ISF_OK) {
				g_movply_config_fileplay_ok = 1;
			} else {
				DBG_ERR("Config file play fail\r\n");
				g_movply_config_fileplay_ok = 0;
			}
		}
		break;

	case MOVIEPLAY_CONFIG_MEM_POOL_INFO: {
			if (value != 0) {
				mem_cfg = (HD_COMMON_MEM_INIT_CONFIG *)value;
				ret = E_OK;
			}
			break;
		}
	case MOVIEPLAY_CONFIG_AUDOUT_INFO:
		if (value) {
			HD_AUDIOOUT_DRV_CONFIG *ptr = (HD_AUDIOOUT_DRV_CONFIG *)value;
			g_audioout_info_ex.drv_cfg.mono	  = ptr->mono;
			g_audioout_info_ex.drv_cfg.output = ptr->output;
		}
		break;

	case MOVIEPLAY_CONFIG_AUDOUT_INFO_EX:
		if (value) {
			memcpy((VOID*)&g_audioout_info_ex, (VOID*)value, sizeof(MOVIEPLAY_AOUT_INFO_EX));
		}
		break;

	case MOVIEPLAY_CONFIG_DECRYPT_INFO:
		if (value) {
			MOVIEPLAY_FILE_DECRYPT *ptr = (MOVIEPLAY_FILE_DECRYPT *)value;
			g_movply_file_decrypt = *ptr;


			if(g_movply_file_decrypt.type == MOVIEPLAY_DECRYPT_TYPE_NONE){

				GxVidFile_SetParam(GXVIDFILE_PARAM_IS_DECRYPT, FALSE);
			}
			else{
				GxVidFile_SetParam(GXVIDFILE_PARAM_IS_DECRYPT, TRUE);
				GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_KEY, (UINT32)g_movply_file_decrypt.key);
				GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_MODE, g_movply_file_decrypt.mode);
				GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_POS, g_movply_file_decrypt.type);
			}
		}
		break;

	case MOVIEPLAY_CONFIG_VDEC_SKIP_SVC_FRAME:
		if (value) {
			memcpy((VOID*)&g_vdec_skip_svc_frame, (VOID*)value, sizeof(MOVIEPLAY_VDEC_SKIP_SVC_FRAME));
		}
		break;

	default:
		DBG_ERR("Config ID 0x%x is not defined!!\r\n", config_id);
		ret = ISF_ERR_INVALID_VALUE;
		break;
	}

	return ret;
}

/**
    Update Speed and Direction.

    @param[in] speed    current play speed
    @param[in] direct   current play direction

    @return ISF_RV
*/
ISF_RV ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(UINT32 speed, UINT32 direct, INT32 disp_idx)
{

	iamovieplay_StartPlay(speed, direct, disp_idx);

	return ISF_OK;
}

ISF_RV ImageApp_MoviePlay_SetParam(UINT32 param, UINT32 value)
{
#if 0
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = NULL;
	UINT32						i;
	MOVIEPLAY_DISP_ID			disp_id = 0;

	pImageStream = g_pImageStream;
	if (pImageStream == NULL) {
		DBG_ERR("MOVIEPLAY_CONFIG_FILEPLAY_INFO or MOVIEPLAY_CONFIG_STRMPLAY_INFO is not configured!!!!\r\n");
		return ISF_ERR_FAILED;
	}

	if (pImageStream->id < _CFG_FILEPLAY_ID_MAX) {
		for (i = MOVIEPLAY_DISP_ID_MIN; i < MOVIEPLAY_DISP_ID_MAX; i++) {
			if (g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].enable) {
				disp_id = g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].disp_id;
			}
		}
	}

	switch (param) {
	/* common */
	case MOVIEPLAY_NM_PARAM_DECDESC:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_DECDESC, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_DECDESC, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_VDO_CODEC:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_CODEC, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_CODEC, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_GOP:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_GOP, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_GOP, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_AUD_SR:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_AUD_SR, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_AudDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, AUDDEC_PARAM_SAMPLERATE, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_FILE_FMT:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_FILE_FMT, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_FILE_FMT, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_WIDTH:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_WID, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_WIDTH, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_HEIGHT:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_HEI, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_HEIGHT, value);
		ImageUnit_End();
		break;
	/* FileIn */
	case MOVIEPLAY_NM_PARAM_FILEHDL:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_FILEHDL, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_FILESIZE:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_FILESIZE, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_FILESIZE, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_FILEDUR:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_FILEDUR, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_BLK_TIME:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_BLK_TIME, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_BLK_PLNUM:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_BLK_PLNUM, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_BLK_TTNUM:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_BLK_TTNUM, value);
		ImageUnit_End();
		break;
	/* BsDemux */
	case MOVIEPLAY_NM_PARAM_VDO_FR:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_VDO_FR, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_FR, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_VDO_FIRSTFRMSIZE:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_FIRSTFRMSIZE, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_VDO_ENABLE:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_ENABLE, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_AUD_ENABLE:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_AUD_ENABLE, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_CONTAINER:
		ImageUnit_Begin(&ISF_FileIn, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, FILEIN_PARAM_CONTAINER, value);
		ImageUnit_End();
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_CONTAINER, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_VDO_TTFRAME:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_VDO_TTFRM, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_AUD_TTFRAME:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_AUD_TTFRM, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_TSDMX_BUFBLK:
		ImageUnit_Begin(&ISF_BsDemux, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, BSDEMUX_PARAM_TSDMX_BUFBLK, value);
		ImageUnit_End();
		break;

	/* VdoDec */
	case MOVIEPLAY_NM_PARAM_RAW_BLK_NUM:
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_RAW_BLK_NUM, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_STEP_PLAY:
		ImageUnit_Begin(&ISF_VdoDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, VDODEC_PARAM_STEP_PLAY, value);
		ImageUnit_End();
		break;

	/* AudDec */
	case MOVIEPLAY_NM_PARAM_AUD_CODEC:
		ImageUnit_Begin(&ISF_AudDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, AUDDEC_PARAM_CODEC, value);
		ImageUnit_End();
		break;
	case MOVIEPLAY_NM_PARAM_AUD_CHS:
		ImageUnit_Begin(&ISF_AudDec, 0);
		ImageUnit_SetParam(ISF_IN1+disp_id, AUDDEC_PARAM_CHANNELS, value);
		ImageUnit_End();
		break;
	default:
		DBG_ERR("invalid param=0x%x, value=%d\r\n", param, value);
		break;
	}
#endif
	return ISF_OK;
}

UINT32 ImageApp_MoviePlay_GetParam(UINT32 param)
{
	switch (param) {
	case MOVIEPLAY_NM_PARAM_MEDIA_INFO:
		return (UINT32)&g_movply_1stMedia_info;
	case MOVIEPLAY_NM_PARAM_VDO_FR:
		return g_movply_1stMedia_info.vidFR;
	case MOVIEPLAY_NM_PARAM_VDO_TTFRAME:
	{
		UINT32 ret = 0;

		if(g_pImageStream){
			MOVIEPLAY_PLAY_OBJ *p_play_obj = NULL;
			p_play_obj = &g_pImageStream->play_obj;
			ret = p_play_obj->vdo_ttfrm;
		}
		else{
			DBG_WRN("get MOVIEPLAY_NM_PARAM_VDO_TTFRAME should be called after MOVIEPLAY_CONFIG_FILEPLAY_INFO\n");
		}

		return ret;
	}
	case MOVIEPLAY_NM_PARAM_GOP:
	{
		UINT32 ret = 0;

		if(g_pImageStream){
			MOVIEPLAY_PLAY_OBJ *p_play_obj = NULL;
			p_play_obj = &g_pImageStream->play_obj;
			ret = p_play_obj->gop;
		}
		else{
			DBG_WRN("get MOVIEPLAY_NM_PARAM_GOP should be called after MOVIEPLAY_CONFIG_FILEPLAY_INFO\n");
		}

		return ret;
	}
	case MOVIEPLAY_NM_PARAM_VDO_CURRFRAME:
	{
		UINT32 ret = 0;
		extern IAMOVIEPLAY_TSK iamovieplay_tsk_obj;
		IAMOVIEPLAY_TSK* tsk_obj = &iamovieplay_tsk_obj;

		if( tsk_obj->state == IAMOVIEPLAY_STATE_PLAYING ||
			tsk_obj->state == IAMOVIEPLAY_STATE_PAUSE ||
			tsk_obj->state == IAMOVIEPLAY_STATE_RESUME
		){
			ret = tsk_obj->vdo_idx;
		}
		else{
			DBG_WRN("get MOVIEPLAY_NM_PARAM_VDO_CURRFRAME should be called while video is playing\n");
			ret = 0;
		}

		return ret;
	}

	default:
		DBG_ERR("Invalid param=0x%x\r\n", param);
		return 0;
	}

	return ISF_OK;
}

/*
    Update Image Stream.

    @param[in] void

    @return ISF_RV
*/
ISF_RV ImageApp_MoviePlay_UpdateAll(void)
{
	ISF_RV ret = ISF_OK;
#if 0
	UINT32 i;

	for (i = MOVIEPLAY_DISP_ID_MIN; i < MOVIEPLAY_DISP_ID_MAX; i++) {
		if (g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].enable) {
			ret = ImageStream_UpdateAll(&ISF_Stream[i - MOVIEPLAY_DISP_ID_MIN]);
			if(ret != ISF_OK){
				//DBG_ERR("[ImageApp_MoviePlay] update stream %d fail ret = %d\r\n", 0, ret);
			}
		}
	}
#endif

	return ret;
}

/**
    Pause Movie Play.

    @param[in] void

    @return ISF_RV
*/
ISF_RV ImageApp_MoviePlay_FilePlay_Pause(void)
{
	//UINT32 i;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = NULL;

	pImageStream = g_pImageStream;

	if (pImageStream == NULL) {
		DBG_ERR("MOVIEPLAY_CONFIG_FILEPLAY_INFO or MOVIEPLAY_CONFIG_STRMPLAY_INFO is not configured\r\n");
		return ISF_ERR_FAILED;
	}

	if (pImageStream->id < _CFG_FILEPLAY_ID_MAX) {
		#if 0
		for (i = MOVIEPLAY_DISP_ID_MIN; i < MOVIEPLAY_DISP_ID_MAX; i++) {
			if (g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].enable) {
				pImageStream->p_disp_info = &g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN];
				_ImageApp_MoviePlay_FilePlay_Pause(pImageStream);
			}
		}
		#else
		_ImageApp_MoviePlay_FilePlay_Pause(pImageStream);
		#endif
	}

    g_movply_vdo_play_status = _CFG_PLAY_STS_PAUSE;
    // Reset variable for step play.
    g_enter_step_state       = FALSE;

	return ISF_OK;
}

/**
    Resume Movie Play.

    @param[in] void

    @return ISF_RV
*/
ISF_RV ImageApp_MoviePlay_FilePlay_Resume(void)
{
	//UINT32 i;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = NULL;

	pImageStream = g_pImageStream;

    // Reset variable for step play.
    g_enter_step_state       = FALSE;
    ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_STEP_PLAY, FALSE);
    memset(g_movply_vdo_rawqueue_idx_table, 0, sizeof(g_movply_vdo_rawqueue_idx_table));

	if (pImageStream == NULL) {
		DBG_ERR("MOVIEPLAY_CONFIG_FILEPLAY_INFO or MOVIEPLAY_CONFIG_STRMPLAY_INFO is not configured\r\n");
		return ISF_ERR_FAILED;
	}

	if (pImageStream->id < _CFG_FILEPLAY_ID_MAX) {
		#if 0
		for (i = MOVIEPLAY_DISP_ID_MIN; i < MOVIEPLAY_DISP_ID_MAX; i++) {
			if (g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN].enable) {
				pImageStream->p_disp_info = &g_movply_disp_info[i - MOVIEPLAY_DISP_ID_MIN];
				_ImageApp_MoviePlay_FilePlay_Resume(pImageStream);
			}
		}
		#else
		_ImageApp_MoviePlay_FilePlay_Resume(pImageStream);
		#endif
	}
	return ISF_OK;
}

/*
    Check The Frame Index Has Been Decoded to RAW Data Buffer

    Check the frame index has been decoded to RAW data buffer

    @param  [in] uifrmidx: The frame index

    @return
        - @b > 0:  Frame idx in RAW idx Queue
        - @b -1: Frame idx is not in RAW idx Queue
*/
#if 0
static UINT32 _ImageApp_MoviePlay_VideoDec_GetVidIdxInRawQue(UINT32 uifrmidx)
{
    UINT32 idx = 0;

    for (idx = 0; idx < g_movply_vdo_max_rawqueue_size; idx++)
    {
        if (g_movply_vdo_rawqueue_idx_table[idx] == uifrmidx)
            return idx;
    }

    return 0xFFFFFFFF;
}

/*
    Check The Frame Index Has Been Decoded to RAW Data Buffer

    Check the frame index has been decoded to RAW data buffer

    @param  [in] uifrmidx: The frame index

    @return
        - @b > 0:  Frame idx in RAW idx Queue
        - @b -1: Frame idx is not in RAW idx Queue
*/
static BOOL _ImageApp_MoviePlay_VideoDec_ChkVidIdxInRawQue(UINT32 uifrmidx)
{
    UINT32 idx = 0;

    //DBGD(uifrmidx);

    for (idx = 0; idx < g_movply_vdo_max_rawqueue_size; idx++)
    {
        //DBGD(g_movply_vdo_rawqueue_idx_table[idx]);
        if (g_movply_vdo_rawqueue_idx_table[idx] == uifrmidx)
            return TRUE;
    }

    return FALSE;
}

/* Get the specific I frame by frame of named index number */
static UINT32 _ImageApp_MoviePlay_GetIfrmByIdx(UINT32 frm_idx)
{
    UINT32 skip_i = 0;
    UINT32 ifrm_prev = 0;
    UINT32 ifrm_next = 0;
    MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;

    if (frm_idx > p_play_obj->vdo_ttfrm) {
        DBG_ERR("Frame of name index number %d is large than totoal frame %d!\r\n", frm_idx, p_play_obj->vdo_ttfrm);
        return frm_idx;
    }

    if (g_movply_container_obj.GetInfo) {
        (g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_PREVIFRAMENUM, &frm_idx, &ifrm_prev, &skip_i);
        (g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_NEXTIFRAMENUM, &ifrm_prev, &ifrm_next, &skip_i);
    }

    if (ifrm_next == frm_idx) {
        return ifrm_next;
    } else {
        return ifrm_prev;
    }
}
#endif
/**
    Trigger Step Play.

    @param[in] direct

    @return ISF_RV
*/
ISF_RV ImageApp_MoviePlay_FilePlay_StepByStep(UINT32 direct, UINT32 disp_idx)
{
#if 0
	MOVIEPLAY_DISP_INFO *p_disp_info = g_pImageStream->p_disp_info;
	NMI_BSDEMUX_PLAY_INFO play_info = {0};

	if (g_movply_file_format == _CFG_FILE_FORMAT_TS) {
		DBG_DUMP("[TSPlay] step play are not supported\r\n");
		return ISF_ERR_NOT_SUPPORT;
	}

    if (g_movply_vdo_play_status == _CFG_PLAY_STS_NORMAL) {
        g_movply_vdo_play_status = _CFG_PLAY_STS_STEP;
        DBG_IND("^Ymovply enter step state.\r\n");
    } else {
		DBG_WRN("Still In step state...\r\n");
		return E_SYS;
    }

	// do it by ImageApp_MoviePlay
	if (direct == NMEDIAPLAY_DIRECT_FORWARD) {
		disp_idx++;
	} else if (direct == NMEDIAPLAY_DIRECT_BACKWARD) {
		if (disp_idx) {
			disp_idx--;
		} else {
			// play finish
			DBG_DUMP("Video Frame is play to first, set to finish...\r\n");
            g_movply_vdo_play_status = _CFG_PLAY_STS_NORMAL;
			//p_obj->vdo_frm_eof = TRUE;
			//p_obj->state = SMEDIA_PLAY_STATUS_WAIT_FINISH; // does it need this ?
			//SMediaPlay_SetPlayFinish(FALSE);
			return E_OK;
		}
	} else {
		DBG_ERR("Invalid direct(%d)\r\n", direct);
		return E_SYS;
	}

    // Check if this frame idx is in RAW idx queue.
    if(!_ImageApp_MoviePlay_VideoDec_ChkVidIdxInRawQue(disp_idx)) {
        // Reset rawqueue index for the table to store frame index.
        // ISF_VDODEC_EVENT_ONE_DISPLAYFRAME callback handler will be called after BSDEMUX_PARAM_STEP_PLAY was triggerred.
        // Enter step play state.
        g_movply_vdo_max_rawqueue_size = disp_idx - _ImageApp_MoviePlay_GetIfrmByIdx(disp_idx) + 1;

        // Should we do every time?
        if(!g_enter_step_state) {
            g_enter_step_state = TRUE;
            ImageApp_MoviePlay_SetParam(MOVIEPLAY_NM_PARAM_STEP_PLAY, TRUE);
            DBG_DUMP("reset VQ\r\n");
            g_movply_vdo_rawqueue_idx = 0;
        } else {
            if (direct == NMEDIAPLAY_DIRECT_FORWARD) // Decode only one frame for forward dir.
                g_movply_vdo_max_rawqueue_size = 1;
        }

        //DBGD(g_movply_vdo_max_rawqueue_size);

    	play_info.direct = direct;
    	play_info.disp_idx = g_movply_vdo_present_idx = disp_idx;
        //DBGD(g_movply_vdo_present_idx);
        //DBGD(g_movply_vdo_rawqueue_idx);
        ImageUnit_Begin(&ISF_BsDemux, 0);
        ImageUnit_SetParam(ISF_OUT1+p_disp_info->disp_id, BSDEMUX_PARAM_STEP_PLAY, (UINT32)&play_info);
        ImageUnit_End();
    } else {  // The frame is already decode. and its frame idx is in Raw frame idx table already
        // Trigger BSDEMUX_PARAM_STEP_PLAY and the desired callback times is reached.
        // Show the frame with the same idx as g_movply_vdo_present_idx.
        NMI_VDODEC_RAW_INFO *pRawInfo;
        MEM_RANGE          mPresentRaw;
        UINT32             uiRawW;
        UINT32             uiRawH;
    	ISF_DATA				*pRawData		= &g_movply_ISF_VdoDecRawData;
    	ISF_PORT             	*pDestPort   	= NULL;
    	PISF_VIDEO_STREAM_BUF	pVdoStrmBuf		= &g_movply_ISF_VdoStrmBuf;
    	UINT32					i 				= 0;
    	MOVIEPLAY_FILEPLAY_INFO *p_play_info =  &g_pImageStream->play_info;


        DBG_IND("^GFrame idx is in Raw frame idx table\r\n");

        // Get width and height of RAW.
        uiRawW = ImageUnit_GetParam(&ISF_VdoDec, ISF_OUT1, VDODEC_PARAM_WIDTH);
        uiRawH = ImageUnit_GetParam(&ISF_VdoDec, ISF_OUT1, VDODEC_PARAM_HEIGHT);

        // Point to the specific RAW in RAW queue with the matched frame idx.
        g_movply_vdo_rawqueue_idx = _ImageApp_MoviePlay_VideoDec_GetVidIdxInRawQue(disp_idx);

        if(g_movply_vdo_rawqueue_idx >= MOVIEPLAY_VIDEO_RAW_QUEUE_MAX_SIZE){
            DBG_ERR("idx out of bound!\r\n");

            g_movply_vdo_play_status = _CFG_PLAY_STS_NORMAL;
            return E_SYS;
        }

        pRawInfo = &g_movply_vdo_rawdata_queue[g_movply_vdo_rawqueue_idx];
        //DBGD(pRawInfo->uiThisFrmIdx);
        mPresentRaw.addr = pRawInfo->uiRawAddr;
        mPresentRaw.size = pRawInfo->uiRawSize;
		pVdoStrmBuf->flag     = MAKEFOURCC('V', 'R', 'A', 'W');
        pVdoStrmBuf->Width    = uiRawW;
        pVdoStrmBuf->Height   = uiRawH;
   		pVdoStrmBuf->DataAddr =  mPresentRaw.addr;//pRawInfo->uiRawAddr;
		pVdoStrmBuf->DataSize =  mPresentRaw.size;//pRawInfo->uiRawSize;
		pVdoStrmBuf->Resv[0]  = pRawInfo->uiYAddr;       // Resv[1] for Y address
		pVdoStrmBuf->Resv[1]  = pRawInfo->uiUVAddr;      // Resv[2] for UV address
		pVdoStrmBuf->Resv[2]  = pRawInfo->uiThisFrmIdx;  // Resv[0] for frame index
		pVdoStrmBuf->Resv[3]  = pRawInfo->bIsEOF;        // Resv[3] for EOF flag

		// get VDODEC_RAWDATA
		#if 1
		g_movply_vdo_rawdata[0].hData  = mPresentRaw.addr;
		g_movply_vdo_rawdata[0].refCnt = 1;
        #endif

		// Set Raw ISF_DATA info
		memcpy(pRawData->Desc, pVdoStrmBuf, sizeof(ISF_VIDEO_STREAM_BUF));
		//pRawData->TimeStamp  = pRawInfo->TimeStamp;
		pRawData->hData      = mPresentRaw.addr;
        //new pData by user
		ISF_VdoDec.pBase->New(&ISF_VdoDec, pDestPort, NVTMPP_VB_INVALID_POOL, pRawData->hData, 0, pRawData);
        //ImageUnit_NewData(img_buf_size, pRawData);

        //DBGH(mPresentRaw.addr);
        //DBGH(mPresentRaw.size);

        // Push to UserProc.
        for (i = 0; i < 1; i++) {
			pDestPort = ImageUnit_Out(&ISF_VdoDec, (ISF_OUT_BASE + i));
			if (ImageUnit_IsAllowPush(pDestPort)) {
				ImageUnit_PushData(pDestPort, pRawData, 0);
                ImageUnit_ReleaseData(pRawData);
		    }
        }

        // reset vdo raw queue idx.
        g_movply_vdo_rawqueue_idx = 0;

    	if (p_play_info->event_cb) {
    		(p_play_info->event_cb)(MOVIEPLAY_EVENT_ONE_DISPLAYFRAME);
            g_movply_vdo_play_status = _CFG_PLAY_STS_NORMAL;
    	} else {
    		DBG_ERR("event_cb = NULL\r\n");
    	}
    }
#endif
	return ISF_OK;
}

INT32 ImageApp_MoviePlay_SetVolume(UINT32 vol)
{
	HD_RESULT ret;

	g_audioout_volume.volume = vol;

	ret = hd_audioout_set(g_tAudioPlaybackStreamInfo[0].out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &g_audioout_volume);
	if (ret == HD_OK) {
		ret = hd_audioout_start(g_tAudioPlaybackStreamInfo[0].out_path);
		if (ret != HD_OK) {
			//DBG_ERR("audioout_start failed, %d\r\n", ret);
		}
	} else {
		//DBG_ERR("audioout_set failed, %d\r\n", ret);
	}

	return (INT32)ret;
}

