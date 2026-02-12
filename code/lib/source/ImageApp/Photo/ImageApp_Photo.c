//#include "SysKer.h"
#include <stdio.h>
#include <string.h>

#include "ImageApp/ImageApp_Photo.h"
#include "ImageApp_Photo_int.h"
#include "ImageApp_Photo_FileNaming.h"
//#include "ImageUnit_VdoIn.h"
//#include "ImageUnit_ImagePipe.h"
//#include "ImageUnit_VdoOut.h"
//#include "ImageUnit_ImgTrans.h"
//#include "ImageUnit_UserProc.h"
//#include "ImageUnit_VdoEnc.h"
//#include "ImageUnit_NetHTTP.h"
//#include "ImageUnit_NetRTSP.h"
//#include "ImageUnit_Dummy.h"
//#include "ImageUnit_StreamSender.h"
//#include "NMediaRecVdoEnc.h"
//#include "NMediaRecImgCap.h"
//#include "VideoEncode.h"
#include "FileDB.h"
#include "kflow_common/nvtmpp.h"
#include "vendor_common.h"
#include "vendor_isp.h"
#include "SizeConvert.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          IA_Photo
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////// //////////////////////////////////////////////////////////
#include "../Common/ImageApp_Common_int.h"

#if 0//defined(_BSP_NA51023_)
#define IME_3DNR_PATH    0//ISF_OUT2
#define IME_DISPLAY_PATH    1//ISF_OUT2
#define IME_STREAMING_PATH  2//ISF_OUT3

#define VPRC_CAP_MAIN_PATH    0//ISF_OUT2
#define VPRC_CAP_SCR_THUMB_PATH    1//ISF_OUT2
#define VPRC_CAP_QV_PATH  2//ISF_OUT3
#endif


/*
typedef struct {
	ISF_UNIT           *p_vdoIn;
	ISF_UNIT           *p_imagepipe;
	ISF_OUT             imagepipe_out;
} PHOTO_VDO_IN_INFO;
*/

//#define                PHOTO_CAP_STRM_ID    PHOTO_IME3DNR_ID_MAX
#define                PHOTO_RTSP_MAX_CLIENT  2

PHOTO_DISP_INFO  photo_disp_info[PHOTO_DISP_ID_MAX] = {0};
PHOTO_STRM_INFO  photo_strm_info[PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN] = {0};
PHOTO_IME3DNR_INFO  photo_ime3dnr_info[(PHOTO_IME3DNR_ID_MAX-PHOTO_IME3DNR_ID_MIN)] = {0};
PHOTO_IPL_MIRROR  photo_ipl_mirror[PHOTO_VID_IN_MAX] = {0};
PHOTO_DUAL_DISP g_photo_dual_disp={0};
UINT32 g_photo_filedb_filter = FILEDB_FMT_ANY;
UINT32 g_photo_filedb_max_num = 10000;
UINT32 g_photo_3ndr_path = IME_3DNR_PATH;
PHOTO_VCAP_SENSOR_MODE_CFG g_photo_cap_sensor_mode_cfg[PHOTO_VID_IN_MAX]={0};

UINT32 ISF_VdoOut1; //temp
UINT32 ISF_VdoOut2; //temp
PHOTO_VDO_OUT_INFO g_isf_vout_info[PHOTO_VID_OUT_MAX] = {
	{&ISF_VdoOut1},
	{&ISF_VdoOut2},
};
UINT32 g_IsImgLinkForStrmEnabled[PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN] = {0};

#if 1
static INT32 g_photo_formatfree_is_on = 0;
#endif

//HD_VIDEOCAP_DRV_CONFIG photo_img_vcap_cfg[PHOTO_VID_IN_MAX] = {0};
//HD_VIDEOCAP_CTRL photo_img_vcap_ctrl[PHOTO_VID_IN_MAX] = {0};
PHOTO_SENSOR_INFO photo_vcap_sensor_info[PHOTO_VID_IN_MAX] = {0};
static HD_COMMON_MEM_INIT_CONFIG *g_photo_mem_cfg = NULL;
PHOTO_CAP_INFO photo_cap_info[PHOTO_VID_IN_MAX] = {0};
HD_VIDEOENC_IN  photo_cap_venc_in_param = {0};
HD_VIDEOENC_OUT2 photo_cap_venc_out_param = {0};
HD_VIDEOCAP_SYSCAPS  photo_vcap_syscaps[PHOTO_VID_IN_MAX]= {0};
HD_URECT photo_vcap_out_crop_rect= {0};
PHOTO_IME_CROP_INFO photo_disp_imecrop_info[PHOTO_VID_IN_MAX] = {0};
UINT32 photo_vcap_patgen[PHOTO_VID_IN_MAX] = {0};
PHOTO_VCAP_OUTFUNC g_photo_vcap_func_cfg[PHOTO_VID_IN_MAX] = {0};
PhotoVdoBsCb  *photo_vdo_bs_cb[PHOTO_VID_IN_MAX] = {0};

INT32 _ImageApp_Photo_ConfigDisp(PHOTO_DISP_INFO *p_disp_info);
INT32 _ImageApp_Photo_ConfigStrm(PHOTO_STRM_INFO *p_strm_info);
INT32 _ImageApp_Photo_ConfigIME3DNR(PHOTO_IME3DNR_INFO *p_ime3dnr_info);
INT32 _ImageApp_Photo_ConfigDualDisp(PHOTO_DUAL_DISP *p_dual_disp);
INT32 _ImageApp_Photo_ConfigIplMirror(PHOTO_IPL_MIRROR *p_ipl_mirror);
void ImageApp_Photo_SetStrmVdoImgSize(UINT32 Path, UINT32 w, UINT32 h);
void ImageApp_Photo_SetDispVdoImgSize(UINT32 Path, UINT32 w, UINT32 h);
void ImageApp_Photo_SetIME3DNRVdoImgSize(UINT32 Path, UINT32 w, UINT32 h);
void ImageApp_Photo_SetDispAspectRatio(UINT32 Path, UINT32 w, UINT32 h);
void ImageApp_Photo_SetStrmAspectRatio(UINT32 Path, UINT32 w, UINT32 h);
void ImageApp_Photo_SetIME3DNRRatio(UINT32 Path, UINT32 w, UINT32 h);
INT32 _ImageApp_Photo_OpenDisp(PHOTO_DISP_INFO *p_disp_info);
void _ImageApp_Photo_FreeImgCapMaxBuf(void);
void _ImageApp_Photo_CloseStrm(UINT32 OutPortIndex);
INT32 _ImageApp_Photo_OpenStrm(PHOTO_STRM_INFO *p_strm_info);
INT32 _ImageApp_Photo_OpenIME3DNR(PHOTO_IME3DNR_INFO *p_3dnr_info);
INT32 _ImageApp_Photo_OpenCap(void);
void _ImageApp_Photo_MergePath(void);



INT32 _ImageApp_Photo_ConfigDisp(PHOTO_DISP_INFO *p_disp_info)
{

	if (p_disp_info == NULL) {
		return E_SYS;
	}

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		return E_SYS;
	}
	photo_disp_info[p_disp_info->disp_id] = *p_disp_info;
	DBG_IND("_ImageApp_Photo_ConfigDisp  disp_id=%d, en=%d\r\n", p_disp_info->disp_id, photo_disp_info[p_disp_info->disp_id].enable);

	return 0;
}
INT32 _ImageApp_Photo_ConfigCap(PHOTO_CAP_INFO *p_cap_info)
{

	if (p_cap_info == NULL) {
		return E_SYS;
	}

	if (p_cap_info->cap_id >= PHOTO_CAP_ID_MAX) {
		return E_SYS;
	}
	photo_cap_info[p_cap_info->cap_id] = *p_cap_info;
	DBG_IND("_ImageApp_Photo_ConfigDisp  cap_id=%d, picnum=%d\r\n", p_cap_info->cap_id, p_cap_info->picnum);

	return 0;
}

INT32 _ImageApp_Photo_ConfigStrm(PHOTO_STRM_INFO *p_strm_info)
{

	if (p_strm_info == NULL) {
		return E_SYS;
	}

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX || p_strm_info->strm_id < PHOTO_STRM_ID_MIN) {
		return E_SYS;
	}
	if(photo_strm_info[p_strm_info->strm_id - PHOTO_STRM_ID_MIN].venc_p[0]){
		DBG_WRN("strm_id(%d) already set!\r\n", p_strm_info->strm_id);
		//return E_SYS;
	}
	photo_strm_info[p_strm_info->strm_id - PHOTO_STRM_ID_MIN] = *p_strm_info;

	return 0;
}
INT32 _ImageApp_Photo_ConfigIME3DNR(PHOTO_IME3DNR_INFO *p_ime3dnr_info)
{
	/*
	if (p_ime3dnr_info == NULL) {
		return ISF_ERR_INVALID_PARAM;
	}
	if (p_ime3dnr_info->ime3dnr_id >= PHOTO_IME3DNR_ID_MAX || p_ime3dnr_info->ime3dnr_id < PHOTO_IME3DNR_ID_MIN) {
		return ISF_ERR_INVALID_PARAM;
	}
	photo_ime3dnr_info[p_ime3dnr_info->ime3dnr_id - PHOTO_IME3DNR_ID_MIN] = *p_ime3dnr_info;
	*/
	return 0;
}
INT32 _ImageApp_Photo_ConfigDualDisp(PHOTO_DUAL_DISP *p_dual_disp)
{
	/*
	if (p_dual_disp == NULL) {
		return ISF_ERR_INVALID_PARAM;
	}
	g_photo_dual_disp = *p_dual_disp;
	*/
	return 0;
}

INT32 _ImageApp_Photo_ConfigIplMirror(PHOTO_IPL_MIRROR *p_ipl_mirror)
{
	/*
	UINT32 id;

	if (p_ipl_mirror != NULL) {
		if (p_ipl_mirror->mirror_type & ~0x70000000) {
			DBG_ERR("MOVIEMULTI_PARAM_IPL_MIRROR only support mirror/flip.\r\n");
		} else if (p_ipl_mirror->vid >= PHOTO_VID_IN_1 && p_ipl_mirror->vid < PHOTO_VID_IN_MAX){
			id = p_ipl_mirror->vid - PHOTO_VID_IN_1;
			ImageUnit_Begin(ISF_IPL(id), 0);
			ImageUnit_SetVdoDirection(ISF_IN1, p_ipl_mirror->mirror_type);
			ImageUnit_End();
			ImageStream_UpdateAll(&ISF_Stream[id]);
			return 0;
		}
	}
	*/
	HD_RESULT ret;
	UINT32 idx, j;

	if (p_ipl_mirror != NULL) {

		if (p_ipl_mirror->mirror_type & ~HD_VIDEO_DIR_MIRRORXY) {
			DBG_ERR("Invalid setting(%x) of PHOTO_CFG_IPL_MIRROR\r\n", p_ipl_mirror->mirror_type);
		} else if (p_ipl_mirror->vid >= PHOTO_VID_IN_1 && p_ipl_mirror->vid < PHOTO_VID_IN_MAX){

			HD_VIDEOPROC_IN vprc_in_param = {0};
			idx = p_ipl_mirror->vid;
			memcpy(&photo_ipl_mirror[idx], p_ipl_mirror, sizeof(PHOTO_IPL_MIRROR));

			HD_PATH_ID video_proc_path= HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0);
			hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_IN, &vprc_in_param);

			vprc_in_param.dir = p_ipl_mirror->mirror_type;
			// vprc
			if ((ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_IN, &vprc_in_param)) != HD_OK) {
				DBG_ERR("hd_videoproc_set fail(%d)\n", ret);
			}
			// resetrt one running path to active setting
			for(j = 0; j < 4; j++) {
				if (photo_vcap_sensor_info[idx].vproc_p_phy[0][j]) {
					if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("hd_videoproc_start fail(%d)\n", ret);
					}
					break;
				}
			}
			return 0;
		}
	}
	return -1;//ISF_ERR_INVALID_PARAM;
}
INT32 _ImageApp_Photo_ConfigIMECrop(PHOTO_IME_CROP_INFO *p_imecrop_info)
{

	if (p_imecrop_info == NULL) {
		return E_SYS;
	}

	if (p_imecrop_info->vid>= PHOTO_VID_IN_MAX) {
		return E_SYS;
	}
	photo_disp_imecrop_info[p_imecrop_info->vid] = *p_imecrop_info;
	DBG_IND("_ImageApp_Photo_ConfigIMECrop  vin=%d\r\n", p_imecrop_info->vid);

	return 0;
}
HD_PATH_ID _ImageApp_Photo_GetVcapCtrlPort(PHOTO_VID_IN vid)
{
	UINT32 i = vid;
	HD_PATH_ID path = 0;


	if (i>= PHOTO_VID_IN_MAX) {
		return 0;
	}
	path=photo_vcap_sensor_info[i].video_cap_ctrl;

	return path;
}

void ImageApp_Photo_Config(UINT32 config_id, UINT32 value)
{
	/*
	switch (config_id) {
	case PHOTO_CFG_DISP_INFO:
		_ImageApp_Photo_ConfigDisp((PHOTO_DISP_INFO *) value);
		break;
	case PHOTO_CFG_STRM_INFO:
		_ImageApp_Photo_ConfigStrm((PHOTO_STRM_INFO *) value);
		break;
	case PHOTO_CFG_IME3DNR_INFO:
		_ImageApp_Photo_ConfigIME3DNR((PHOTO_IME3DNR_INFO *) value);
		break;
	case PHOTO_CFG_FBD_POOL:
		ImageApp_Photo_FileNaming_SetFileDBPool((MEM_RANGE *) value);
		break;
	case PHOTO_CFG_ROOT_PATH:
		ImageApp_Photo_FileNaming_SetRootPath((CHAR *) value);
		break;
	case PHOTO_CFG_FOLDER_NAME:
		ImageApp_Photo_FileNaming_SetFolder((PHOTO_CAP_FOLDER_NAMING *) value);
		break;
	case PHOTO_CFG_FILE_NAME_CB:
		ImageApp_Photo_FileNaming_SetFileNameCB((PHOTO_FILENAME_CB *) value);
		break;
	case PHOTO_CFG_DUAL_DISP:
		_ImageApp_Photo_ConfigDualDisp((PHOTO_DUAL_DISP *) value);
		break;
	case PHOTO_CFG_FILEDB_FILTER:
		g_photo_filedb_filter = value;
		break;
	case PHOTO_CFG_FILEDB_MAX_NUM:
		g_photo_filedb_max_num = value;
		break;
	case PHOTO_CFG_IPL_MIRROR:
		_ImageApp_Photo_ConfigIplMirror((PHOTO_IPL_MIRROR *) value);
		break;

	default:
		DBG_ERR("Unknown config_id=%d\r\n", config_id);
		break;

	}
	*/
	switch (config_id) {
	case PHOTO_CFG_FBD_POOL:
		ImageApp_Photo_FileNaming_SetFileDBPool((MEM_RANGE *) value);
		break;
	case PHOTO_CFG_ROOT_PATH:
		ImageApp_Photo_FileNaming_SetRootPath((CHAR *) value);
		break;
	case PHOTO_CFG_FOLDER_NAME:
		ImageApp_Photo_FileNaming_SetFolder((PHOTO_CAP_FOLDER_NAMING *) value);
		break;
	case PHOTO_CFG_FILE_NAME_CB:
		ImageApp_Photo_FileNaming_SetFileNameCB((PHOTO_FILENAME_CB *) value);
		break;

	case PHOTO_CFG_STRM_INFO:
		_ImageApp_Photo_ConfigStrm((PHOTO_STRM_INFO *) value);
		break;

	case PHOTO_CFG_DISP_INFO:
		_ImageApp_Photo_ConfigDisp((PHOTO_DISP_INFO *) value);
		break;

	case PHOTO_CFG_SENSOR_INFO:{
			PHOTO_SENSOR_INFO *seninfo = (PHOTO_SENSOR_INFO *)value;
			HD_RESULT hd_ret;

            hd_ret = vendor_isp_init();
			if ((hd_ret != HD_OK) && (hd_ret != HD_ERR_INIT)) {
				DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
			}
			memcpy((void *)&(photo_vcap_sensor_info[seninfo->vid_in]), (void *)(seninfo), sizeof(PHOTO_SENSOR_INFO));
			DBG_IND("PHOTO_CFG_SENSOR_INFO ,vid_in=%d\n", seninfo->vid_in);

			char file_path[64]={0};
			if (strlen(seninfo->file_path) != 0) {
				strncpy(file_path, seninfo->file_path, 63);
			} else {
				strncpy(file_path, "/mnt/app/application.dtb", 63);
			}

			if (strlen(seninfo->ae_path.path) != 0) {
				AET_DTSI_INFO ae_dtsi_info;
				ae_dtsi_info.id = seninfo->ae_path.id;
				strncpy(ae_dtsi_info.node_path, seninfo->ae_path.path, 31);
				strncpy(ae_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
				ae_dtsi_info.buf_addr = (UINT8 *)seninfo->ae_path.addr;
				if ((hd_ret = vendor_isp_set_ae(AET_ITEM_RLD_DTSI, &ae_dtsi_info)) != HD_OK) {
					DBG_ERR("vendor_isp_set_ae fail(%d)\r\n", hd_ret);
				}
			}
			if (strlen(seninfo->awb_path.path) != 0) {
				AWBT_DTSI_INFO awb_dtsi_info;
				awb_dtsi_info.id = seninfo->awb_path.id;
				strncpy(awb_dtsi_info.node_path, seninfo->awb_path.path, 31);
				strncpy(awb_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
				awb_dtsi_info.buf_addr = (UINT8 *)seninfo->awb_path.addr;
				if ((hd_ret = vendor_isp_set_awb(AWBT_ITEM_RLD_DTSI, &awb_dtsi_info)) != HD_OK) {
					DBG_ERR("vendor_isp_set_awb fail(%d)\r\n", hd_ret);
				}
			}
			if (strlen(seninfo->iq_path.path) != 0) {
				IQT_DTSI_INFO iq_dtsi_info;
				iq_dtsi_info.id = seninfo->iq_path.id;
				strncpy(iq_dtsi_info.node_path, seninfo->iq_path.path, 31);
				strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
				iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_path.addr;
				if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
					DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
				}
			}
			if (strlen(seninfo->iq_shading_path.path) != 0) {
				IQT_DTSI_INFO iq_dtsi_info;
				strncpy(iq_dtsi_info.node_path, seninfo->iq_shading_path.path, 31);
				strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
				iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_shading_path.addr;
				if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
					DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
				}
			}
			if (strlen(seninfo->iq_dpc_path.path) != 0) {
				IQT_DTSI_INFO iq_dtsi_info;
				strncpy(iq_dtsi_info.node_path, seninfo->iq_dpc_path.path, 31);
				strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
				iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_dpc_path.addr;
				if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
					DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
				}
			}
			if (strlen(seninfo->iq_ldc_path.path) != 0) {
				IQT_DTSI_INFO iq_dtsi_info;
				strncpy(iq_dtsi_info.node_path, seninfo->iq_ldc_path.path, 31);
				strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
				iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_ldc_path.addr;
				if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
					DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
				}
			}
            AET_OPERATION ae_ui_operation = {0};
            ae_ui_operation.id = seninfo->ae_path.id;
            ae_ui_operation.operation = AE_OPERATION_PHOTO;
            if ((hd_ret = vendor_isp_set_ae(AET_ITEM_OPERATION, &ae_ui_operation)) != HD_OK) {
                DBG_ERR("vendor_isp_set_ae(AET_ITEM_OPERATION) fail(%d)\r\n", hd_ret);
            }
            AWBT_OPERATION awb_ui_operation = {0};
            awb_ui_operation.id = seninfo->awb_path.id;
            awb_ui_operation.operation = AWB_OPERATION_MOVIE;
            if ((hd_ret = vendor_isp_set_awb(AWBT_ITEM_OPERATION, &awb_ui_operation)) != HD_OK) {
                DBG_ERR("vendor_isp_set_awb(AWBT_ITEM_OPERATION) fail(%d)\r\n", hd_ret);
            }
            IQT_OPERATION iq_ui_operation = {0};
            iq_ui_operation.id = seninfo->iq_path.id;
            iq_ui_operation.operation = IQ_UI_OPERATION_MOVIE;
            if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_OPERATION, &iq_ui_operation)) != HD_OK) {
                DBG_ERR("vendor_isp_set_iq(IQT_ITEM_OPERATION) fail(%d)\r\n", hd_ret);
            }
			if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
				DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
			}

		}
		break;

	case PHOTO_CFG_MEM_POOL_INFO: {
			if (value != 0) {
				g_photo_mem_cfg = (HD_COMMON_MEM_INIT_CONFIG *)value;
			}
			break;
		}
	case PHOTO_CFG_CAP_INFO:
		_ImageApp_Photo_ConfigCap((PHOTO_CAP_INFO *) value);
		break;

	case PHOTO_CFG_FILEDB_MAX_NUM:
		g_photo_filedb_max_num = value;
		break;

	case PHOTO_CFG_3DNR_PATH:
	    if (value == PHOTO_3DNR_SHARED_PATH) {
	        g_photo_3ndr_path = IME_3DNR_PATH;
	    } else {
	        g_photo_3ndr_path = IME_3DNR_REF_PATH;
	    }
	    break;

	case PHOTO_CFG_IPL_MIRROR:
		_ImageApp_Photo_ConfigIplMirror((PHOTO_IPL_MIRROR *) value);
		break;


	case PHOTO_CFG_DISP_IME_CROP:
		_ImageApp_Photo_ConfigIMECrop((PHOTO_IME_CROP_INFO *) value);
		break;

	case PHOTO_CFG_VCAP_PAT_GEN:
		if (value != 0) {
			PHOTO_VCAP_PAT_GEN * p_pat_gen= (PHOTO_VCAP_PAT_GEN *)value;
			if (p_pat_gen->vid >= PHOTO_VID_IN_MAX) {
				DBG_ERR("PHOTO_CFG_VCAP_PAT_GEN vid over flow %d\r\n", p_pat_gen->vid);
				break;
			}

			photo_vcap_patgen[p_pat_gen->vid]=p_pat_gen->patgen_type;
		}
        break;

	case PHOTO_CFG_VCAP_OUTFUNC:
		if (value != 0) {
			PHOTO_VCAP_OUTFUNC *outfunc= (PHOTO_VCAP_OUTFUNC*)value;
			if(outfunc->out_func  <= HD_VIDEOCAP_OUTFUNC_ONEBUF && outfunc->vid < PHOTO_VID_IN_MAX){
				g_photo_vcap_func_cfg[outfunc->vid].out_func = outfunc->out_func;
			}
		}
		break;

	case PHOTO_CFG_FILEDB_FILTER:
		g_photo_filedb_filter = value;
		break;

	case PHOTO_CFG_VCAP_SENSOR_SIZE:
		if (value != 0) {
			PHOTO_VCAP_SENSOR_MODE_CFG *cap_sensor_size = (PHOTO_VCAP_SENSOR_MODE_CFG *)value;
			if(cap_sensor_size && cap_sensor_size->in_size.w && cap_sensor_size->in_size.h && cap_sensor_size->vid < PHOTO_VID_IN_MAX && cap_sensor_size->vid >=0){
				g_photo_cap_sensor_mode_cfg[cap_sensor_size->vid].in_size = 	cap_sensor_size->in_size;
				g_photo_cap_sensor_mode_cfg[cap_sensor_size->vid].frc  = 	cap_sensor_size->frc;
			}else{
				DBG_ERR("set PHOTO_CFG_VCAP_SENSOR_SIZE fail\r\n");
			}
		}
		break;

	case PHOTO_CFG_VENC_BS_CB:
		if (value != 0) {
			photo_vdo_bs_cb[0] = (PhotoVdoBsCb *)value;
		} else {
			photo_vdo_bs_cb[0] = NULL;
		}
		break;

	default:
		DBG_ERR("Unknown config_id=%d\r\n", config_id);
		break;

	}

}

UINT32 ImageApp_Photo_GetConfig(UINT32 config_id, UINT32 param1)
{
	switch (config_id) {
	case PHOTO_CFG_STRM_TYPE: {
			PHOTO_STRM_INFO *p_strm_info = &photo_strm_info[0];
			return p_strm_info->strm_type;
		}
	case PHOTO_CFG_STRM_INFO: {
			return (UINT32)&photo_strm_info[0];
		}
	case PHOTO_CFG_VCAP_CTRLPORT:{
			return (UINT32)_ImageApp_Photo_GetVcapCtrlPort(param1);
		}
	case PHOTO_CFG_SENSOR_INFO:{
			return (UINT32)&(photo_vcap_sensor_info[param1]);
		}
	default:
		DBG_ERR("Unknown config_id=%d\r\n", config_id);
		return 0;
	}
	return 0;
}


void ImageApp_Photo_SetStrmVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
    	UINT32    id=0 ;

	DBG_IND("path=%d, w = %d, h =%d\r\n", Path,w,h);

    //PHOTO_STRM_INFO *p_strm_info = &photo_strm_info[Path - PHOTO_STRM_ID_MIN];
#if 0
    ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();

    #if 0
	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();
	#endif

	if (Path == PHOTO_STRM_ID_MIN) {
		ImageUnit_Begin(&ISF_VdoEnc, 0);
		ImageUnit_SetVdoImgSize(ISF_IN1, w, h);
		ImageUnit_End();
	}
#else
	ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();


	id = Path - PHOTO_STRM_ID_MIN;

	ImageStream_UpdateAll(&ISF_Stream[id]);

	_ImageApp_Photo_WiFiSetVdoImgSize(Path, w, h);
#endif
	DBG_FUNC_END("\r\n");
*/
}

void ImageApp_Photo_SetDispVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	//PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];

	DBG_IND("path=%d, w = %d, h =%d\r\n", Path,w,h);
#if 0
    ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();

    if (Path == PHOTO_DISP_ID_MIN) {
		ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
		ImageUnit_SetVdoImgSize(ISF_IN1, w, h);
		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
		ImageUnit_End();
        	if (g_photo_dual_disp.enable) {
        		ImageUnit_Begin(&ISF_VdoOut2, 0);
        		ImageUnit_SetVdoImgSize(ISF_IN1, w, h);
        		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
        		ImageUnit_End();
        	}
    }
#else
	#if 1
	ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();
	#endif
	ImageStream_UpdateAll(&ISF_Stream[Path]);

	_ImageApp_Photo_DispSetVdoImgSize(Path, w, h);
#endif
	DBG_FUNC_END("\r\n");
	*/
}

void ImageApp_Photo_SetIME3DNRVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
    	UINT32    id=0 ;

	DBG_IND("path=%d, w = %d, h =%d\r\n", Path,w,h);

    ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	id = Path - PHOTO_IME3DNR_ID_MIN;

	ImageStream_UpdateAll(&ISF_Stream[id]);

	DBG_FUNC_END("\r\n");
	*/
}



void ImageApp_Photo_SetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	DBG_FUNC_BEGIN("path=%d\r\n", Path);

    if (Path < PHOTO_DISP_ID_MAX) {
		ImageApp_Photo_SetDispVdoImgSize(Path, w, h);
	}
	else if (Path >= PHOTO_STRM_ID_MIN &&  Path < PHOTO_STRM_ID_MAX) {
		ImageApp_Photo_SetStrmVdoImgSize(Path, w, h);
	}
	else if (Path >= PHOTO_IME3DNR_ID_MIN &&  Path < PHOTO_IME3DNR_ID_MAX) {
		ImageApp_Photo_SetIME3DNRVdoImgSize(Path, w, h);
	}
	else {
		DBG_ERR("Path =%d\r\n",Path);
	}

	DBG_FUNC_END("\r\n");
	*/

	//set out crop for 4:3 cap size
	HD_URECT rect;
	USIZE ImageRatioSize = {0};
	HD_VIDEOCAP_CROP vcap_crop_param= {0};
	HD_RESULT ret = HD_OK;
	UINT32 idx=Path;
	UINT32 j;

	if (Path >= PHOTO_VID_IN_MAX) {
		DBG_ERR("Path  =%d\r\n",Path);
		return;
	}

	if (photo_vcap_sensor_info[idx].sSize[0].w== 0 || photo_vcap_sensor_info[idx].sSize[0].h == 0) {
		DBG_ERR("sensor size is not config\r\n");
		return;
	}

	ImageRatioSize.w=w;//((photo_cap_info[idx].img_ratio & 0xffff0000) >> 16);
	ImageRatioSize.h=h;//(photo_cap_info[idx].img_ratio & 0x0000ffff);
	//crop width
	if (((float) photo_vcap_sensor_info[idx].sSize[0].w/photo_vcap_sensor_info[idx].sSize[0].h) >= ((float)ImageRatioSize.w/ImageRatioSize.h)){
	    rect.h = photo_vcap_sensor_info[idx].sSize[0].h;
	    rect.w = ALIGN_ROUND(rect.h/ImageRatioSize.h * ImageRatioSize.w, 4);
	}else{
	    rect.w = photo_vcap_sensor_info[idx].sSize[0].w;
	    rect.h = ALIGN_ROUND(rect.w/ImageRatioSize.w * ImageRatioSize.h, 4);
	}
	rect.x=(photo_vcap_sensor_info[idx].sSize[0].w-rect.w)>>1;
	rect.y=(photo_vcap_sensor_info[idx].sSize[0].h-rect.h)>>1;
	DBG_IND("rect %d, %d, %d, %d\r\n", rect.x,rect.y,rect.w,rect.h);
	memset(&vcap_crop_param,0, sizeof(HD_VIDEOCAP_CROP));
	vcap_crop_param.mode = HD_CROP_ON;
	vcap_crop_param.win.rect.x   = rect.x;
	vcap_crop_param.win.rect.y   = rect.y;
	vcap_crop_param.win.rect.w   = rect.w;
	vcap_crop_param.win.rect.h   = rect.h;
	vcap_crop_param.align.w = 4;
	vcap_crop_param.align.h = 4;

	j=g_photo_3ndr_path;
	HD_VIDEOPROC_OUT vprc_out_param = {0};
	vprc_out_param.func = 0;

	vprc_out_param.dim.w = rect.w;//photo_vcap_sensor_info[idx].sSize[j].w;//p_dim->w;
	vprc_out_param.dim.h = rect.h;//photo_vcap_sensor_info[idx].sSize[j].h;//p_dim->h;
	vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	vprc_out_param.dir = HD_VIDEO_DIR_NONE;
	vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
	vprc_out_param.depth = 1; //allow pull

	if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT_CROP, &vcap_crop_param)) != HD_OK) {
		DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
	}
	if (g_photo_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {


		//if (photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
			j=g_photo_3ndr_path;
			if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("3dnr hd_videoproc_stop fail(%d)\n", ret);
			}
		//}
		//DBG_DUMP("disp en=%d, wifi en=%d\n", photo_disp_info[PHOTO_DISP_ID_1].enable,photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable);

		if (photo_disp_info[PHOTO_DISP_ID_1].enable) {
			j=IME_DISPLAY_PATH;
			if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("disp hd_videoproc_stop fail(%d)\n", ret);
			}
		}
		if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
			j=IME_STREAMING_PATH;
			if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("wifi hd_videoproc_stop fail(%d)\n", ret);
			}
		}




		if ((ret = hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
			DBG_ERR("set hd_videocap_stop fail(%d)\r\n", ret);
		}

		//if (photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
			j=g_photo_3ndr_path;
			if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
			}
			if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("3dnr hd_videoproc_start fail(%d)\n", ret);
			}
		//}
		//DBG_DUMP("disp en=%d, wifi en=%d\n", photo_disp_info[PHOTO_DISP_ID_1].enable,photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable);

		if (photo_disp_info[PHOTO_DISP_ID_1].enable) {
			j=IME_DISPLAY_PATH;
			if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("disp hd_videoproc_start fail(%d)\n", ret);
			}
		}
		if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
			j=IME_STREAMING_PATH;
			if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("wifi hd_videoproc_start fail(%d)\n", ret);
			}
		}

		if ((ret = hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
			DBG_ERR("set hd_videocap_start fail(%d)\r\n", ret);
		}

	}else{
		if ((ret = hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
			DBG_ERR("set hd_videocap_stop fail(%d)\r\n", ret);
		}
		if ((ret = hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
			DBG_ERR("set hd_videocap_start fail(%d)\r\n", ret);
		}
		if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
			DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
		}

		if ((ret =hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
			DBG_ERR("hd_videoproc_stop fail(%d)\r\n", ret);
		}

		if ((ret =hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
			DBG_ERR("hd_videoproc_start fail(%d)\r\n", ret);
		}
	}

}

void ImageApp_Photo_SetDispAspectRatio(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	//UINT32                       rotate_dir=0;//coverity 120717
	PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];

	DBG_IND("path=%d, w = %d, h =%d\r\n", Path,w,h);
#if 0
	ImageUnit_GetVdoDirection(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1+ Path, &rotate_dir);

    ImageUnit_Begin(ISF_VIN(p_disp_info->vid_in), 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
	ImageUnit_End();

    ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
	if (((rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) ||
		((rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1+ Path, h << 1, w);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1+ Path, h, w);
    }
	else {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1+ Path, w << 1, h);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1+ Path, w , h);
	}
	ImageUnit_End();
	if (g_photo_dual_disp.enable) {
		ImageUnit_Begin(&ISF_VdoOut2, 0);
		g_photo_dual_disp.disp_ar.w=w;
		g_photo_dual_disp.disp_ar.h=h;
		ImageUnit_SetVdoAspectRatio(ISF_IN1, g_photo_dual_disp.disp_ar.w, g_photo_dual_disp.disp_ar.h);
		ImageUnit_End();
	}
#else
	ImageUnit_Begin(ISF_VIN(p_disp_info->vid_in), 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageStream_UpdateAll(&ISF_Stream[Path]);

	_ImageApp_Photo_DispSetAspectRatio(Path,  w,  h);
#endif
	*/
	DBG_IND("path=%d, w = %d, h =%d\r\n", Path,w,h);

	_ImageApp_Photo_DispSetAspectRatio(Path,  w,  h);

}

void ImageApp_Photo_SetStrmAspectRatio(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
    //PHOTO_STRM_INFO *p_strm_info = &photo_strm_info[Path - PHOTO_STRM_ID_MIN];
    	UINT32    id=0 ;

	DBG_IND("path=%d, w = %d, h =%d\r\n", Path,w,h);
#if 0
    #if 0
	ImageUnit_Begin(g_isf_vin_info[p_strm_info->vid_in].p_imagepipe, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
	ImageUnit_End();
	#endif

    ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

    if (Path == PHOTO_STRM_ID_MIN) {
		ImageUnit_Begin(&ISF_NetHTTP, 0);
		ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
		ImageUnit_End();

		ImageUnit_Begin(&ISF_NetRTSP, 0);
		ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
		ImageUnit_End();
    }
#else

	ImageUnit_Begin(&ISF_UserProc, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	id = Path - PHOTO_STRM_ID_MIN;

	ImageStream_UpdateAll(&ISF_Stream[id]);

	_ImageApp_Photo_WiFiSetAspectRatio(Path, w, h);
#endif
	*/

	_ImageApp_Photo_WiFiSetAspectRatio(Path, w, h);
}


void ImageApp_Photo_SetIME3DNRRatio(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	PHOTO_IME3DNR_INFO *p_3dnr_info = &photo_ime3dnr_info[Path - PHOTO_IME3DNR_ID_MIN];
    	UINT32    id=0 ;

    ImageUnit_Begin(ISF_VIN(p_3dnr_info->vid_in), 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_Dummy, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + Path, w, h);
	ImageUnit_End();

	id = Path - PHOTO_IME3DNR_ID_MIN;

	ImageStream_UpdateAll(&ISF_Stream[id]);

	*/
}

void ImageApp_Photo_SetVdoAspectRatio(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	if (Path < PHOTO_DISP_ID_MAX) {
		ImageApp_Photo_SetDispAspectRatio(Path, w, h);
	}
	else if (Path >= PHOTO_STRM_ID_MIN &&  Path < PHOTO_STRM_ID_MAX) {
		ImageApp_Photo_SetStrmAspectRatio(Path, w, h);
	}
	else if (Path >= PHOTO_IME3DNR_ID_MIN &&  Path < PHOTO_IME3DNR_ID_MAX) {
		ImageApp_Photo_SetIME3DNRRatio(Path, w, h);
	}
	*/
	if (Path < PHOTO_DISP_ID_MAX) {
		ImageApp_Photo_SetDispAspectRatio(Path, w, h);
	}
	else if (Path >= PHOTO_STRM_ID_MIN &&  Path < PHOTO_STRM_ID_MAX) {
		ImageApp_Photo_SetStrmAspectRatio(Path, w, h);
	}

}

void ImageApp_Photo_ResetPath(UINT32 Path)
{
	/*
	DBG_IND("path=%d\r\n", Path);
	ImageStream_UpdateAll(&ISF_Stream[Path]);
	*/
}


INT32 _ImageApp_Photo_OpenDisp(PHOTO_DISP_INFO *p_disp_info)
{
	/*
	UINT32    ime_out_path;
    	USIZE sz={0}; //coverity 132338
	UINT32 port_state=0;

	DBG_FUNC_BEGIN("\r\n");

	if (p_disp_info == NULL) {
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_disp_info->vid_in >= PHOTO_VID_IN_MAX) {
		DBG_ERR("vid_in  =%d\r\n",p_disp_info->vid_in);
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_disp_info->is_merge_3dnr_path == TRUE)
		ime_out_path = ISF_OUT1;
	else
		ime_out_path = IME_DISPLAY_PATH;

	DBG_IND("ime_out_path = %d\r\n",ime_out_path);

	if(p_disp_info->enable == PHOTO_PORT_STATE_EN_RUN){
		port_state=ISF_PORT_STATE_RUN;
	}else if(p_disp_info->enable == PHOTO_PORT_STATE_EN){
		port_state=ISF_PORT_STATE_OFF;
	}else{
		DBG_ERR("enable  =%d\r\n",p_disp_info->enable);
		return ISF_ERR_INVALID_PARAM;
	}

	//open
	ImageStream_Open(&ISF_Stream[p_disp_info->vid_in]);

	ImageStream_Begin(&ISF_Stream[p_disp_info->vid_in], 0);
	ImageStream_SetRoot(0, ImageUnit_In(ISF_VIN(p_disp_info->vid_in), ISF_IN1));

	ImageUnit_Begin(ISF_VIN(p_disp_info->vid_in), 0);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(ISF_IPL(p_disp_info->vid_in), ISF_IN1), port_state);
	ImageUnit_End();

	ImageUnit_Begin(ISF_IPL(p_disp_info->vid_in), 0);
	ImageUnit_SetParam(ISF_CTRL, IMAGEPIPE_PARAM_MODE, IMAGEPIPE_MODE_PREVIEW);
	ImageUnit_SetOutput(ime_out_path, ImageUnit_In(&ISF_UserProc, ISF_IN1 + p_disp_info->disp_id), port_state);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio, p_disp_info->height_ratio);
	ImageUnit_End();
#if 0
	if (p_disp_info->disp_id == PHOTO_DISP_ID_MIN) {
		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, 0, 0);
		ImageUnit_SetOutput(ISF_OUT1 + p_disp_info->disp_id, ImageUnit_In(&ISF_ImgTrans, ISF_IN1 + p_disp_info->disp_id), ISF_PORT_STATE_RUN);
		ImageUnit_SetParam(ISF_OUT1 + p_disp_info->disp_id, USERPROC_PARAM_BYPASS_IMM, (p_disp_info->multi_view_type) ? FALSE : TRUE);
		ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio, p_disp_info->height_ratio);
		ImageUnit_End();

		ImageUnit_Begin(&ISF_ImgTrans, 0);
		ImageUnit_SetOutput(ISF_OUT1 + p_disp_info->disp_id, ImageUnit_In(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1 + p_disp_info->disp_id), ISF_PORT_STATE_RUN);
		ImageUnit_SetParam(ISF_OUT1 + p_disp_info->disp_id, IMGTRANS_PARAM_CONNECT_IMM, ISF_IN1 + p_disp_info->disp_id);
		if (g_photo_dual_disp.enable) {
        		ImageUnit_SetOutput(ISF_OUT2, ImageUnit_In(&ISF_VdoOut2, ISF_IN1), ISF_PORT_STATE_RUN);
        		ImageUnit_SetParam(ISF_OUT2, IMGTRANS_PARAM_CONNECT_IMM, ISF_IN1);
        		ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, g_photo_dual_disp.disp_size.w, g_photo_dual_disp.disp_size.h);
		}else{
        		ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, 0, 0);
		}
		// set image size 0 to auto sync from vdoout
		ImageUnit_SetParam(ISF_IN1 + p_disp_info->disp_id, IMGTRANS_PARAM_MAX_IMG_WIDTH_IMM, p_disp_info->width);
		ImageUnit_SetParam(ISF_IN1 + p_disp_info->disp_id, IMGTRANS_PARAM_MAX_IMG_HEIGHT_IMM, p_disp_info->height);
		ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio, p_disp_info->height_ratio);
		ImageUnit_End();

		ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
		// set image size 0 to auto calculate by image ratio
		ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, 0, 0);
		if (((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) || \
			((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
			if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
				ImageUnit_SetVdoAspectRatio(ISF_IN1+ p_disp_info->disp_id, p_disp_info->height_ratio << 1, p_disp_info->width_ratio);
			else
				ImageUnit_SetVdoAspectRatio(ISF_IN1+ p_disp_info->disp_id, p_disp_info->height_ratio, p_disp_info->width_ratio);
        }
		else {
			if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
				ImageUnit_SetVdoAspectRatio(ISF_IN1+ p_disp_info->disp_id, p_disp_info->width_ratio << 1, p_disp_info->height_ratio);
			else
				ImageUnit_SetVdoAspectRatio(ISF_IN1+ p_disp_info->disp_id, p_disp_info->width_ratio, p_disp_info->height_ratio);
		}
		ImageUnit_SetVdoDirection(ISF_IN1, p_disp_info->rotate_dir);
		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
		ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
		ImageUnit_End();
    		if (g_photo_dual_disp.enable) {
        		ImageUnit_Begin(&ISF_VdoOut2, 0);
        		ImageUnit_SetVdoImgSize(ISF_IN1, 0, 0);
        		ImageUnit_SetVdoAspectRatio(ISF_IN1, g_photo_dual_disp.disp_ar.w, g_photo_dual_disp.disp_ar.h);
        		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
        		ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
        		ImageUnit_End();
    		}
	} else
#endif
	{
		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + p_disp_info->disp_id, ImageUnit_In(&ISF_Dummy, ISF_IN1 + p_disp_info->disp_id), port_state);
		ImageUnit_SetParam(ISF_OUT1 + p_disp_info->disp_id, USERPROC_PARAM_BYPASS_IMM, FALSE);
		ImageUnit_SetParam(ISF_OUT1 + p_disp_info->disp_id, USERPROC_PARAM_ALLOW_PULL_IMM, TRUE);
		ImageUnit_SetParam(ISF_OUT1 + p_disp_info->disp_id, USERPROC_PARAM_SYNC_SIZE_REF_ID, ISF_OUT1+PHOTO_DISP_ID_MIN);

		ImageUnit_GetVdoImgSize(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1 + p_disp_info->vid_out, &(sz.w), &(sz.h));
		//ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, p_disp_info->width, p_disp_info->height);
		//ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, sz.w,sz.h);

		DBG_IND("sz_img.w=%d, sz_img.h=%d\r\n",sz.w, sz.h);

		if (((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) || \
		((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
			ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, sz.h,sz.w);

		}else{
			//ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, p_disp_info->width, p_disp_info->height);
			ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->disp_id, sz.w,sz.h);
		}


		ImageUnit_End();

		ImageUnit_Begin(&ISF_Dummy, 0);
		ImageUnit_SetParam(ISF_IN1+p_disp_info->disp_id, DUMMY_PARAM_UNIT_TYPE_IMM, DUMMY_UNIT_TYPE_LEAF);
		ImageUnit_SetOutput(ISF_OUT1 + p_disp_info->disp_id, ISF_PORT_EOS, port_state);
		ImageUnit_End();
	}
	ImageStream_End();

	ImageStream_UpdateAll(&ISF_Stream[p_disp_info->vid_in]);
*/
	return 0;

}
 UINT32 ImageApp_Photo_UpdateImgLinkForDisp(PHOTO_DISP_ID disp_id, UINT32 action, BOOL allow_pull)
{
	/*
    	UINT32    ime_out_path;
    	UINT32    id ;
    	PHOTO_IME3DNR_INFO *p_3dnr_info;

	if (disp_id < PHOTO_DISP_ID_MAX) {
		id = disp_id - PHOTO_DISP_ID_MIN;
	} else {
		DBG_ERR("ID %d is out of range!\r\n", disp_id);
		return E_SYS;
	}
	DBG_IND("id=%d, disp_id=%d ,%d, %d\r\n", id, disp_id,action,allow_pull);

    	p_3dnr_info = &photo_ime3dnr_info[id];

	if (p_3dnr_info->is_merge_3dnr_path == TRUE)
		ime_out_path = ISF_OUT1;
	else
		ime_out_path = IME_DISPLAY_PATH;

	DBG_IND("ime_out_path = %d\r\n",ime_out_path);

	ImageStream_Begin(&ISF_Stream[id], 0);

	if (action == ENABLE) {
        	ImageUnit_Begin(ISF_VIN(id), 0);
        	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
        	ImageUnit_End();

        	ImageUnit_Begin(ISF_IPL(id), 0);
        	ImageUnit_SetOutput(ime_out_path, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
        	ImageUnit_End();

		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + disp_id, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
		if (allow_pull) {
		    ImageUnit_SetParam(ISF_OUT1 + disp_id, USERPROC_PARAM_ALLOW_PULL_IMM, TRUE);
		}
		ImageUnit_End();

    		ImageUnit_Begin(&ISF_Dummy, 0);
		ImageUnit_SetOutput(ISF_OUT1 + disp_id, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
		ImageUnit_End();

	}else{
        	ImageUnit_Begin(ISF_VIN(id), 0);
        	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
        	ImageUnit_End();

        	ImageUnit_Begin(ISF_IPL(id), 0);
        	ImageUnit_SetOutput(ime_out_path, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
        	ImageUnit_End();

		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + disp_id, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
		if (allow_pull) {
		    ImageUnit_SetParam(ISF_OUT1 + disp_id, USERPROC_PARAM_ALLOW_PULL_IMM, FALSE);
		}
		ImageUnit_End();

    		ImageUnit_Begin(&ISF_Dummy, 0);
		ImageUnit_SetOutput(ISF_OUT1 + disp_id, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
		ImageUnit_End();
	}
	ImageStream_End();

	ImageStream_UpdateAll(&ISF_Stream[id]);
*/
	return 0;
}

void _ImageApp_Photo_FreeVideoMaxBuf(UINT32 OutPortIndex)
{
	/*
	NMI_VDOENC_MAX_MEM_INFO MaxMemInfo = {0};
	MaxMemInfo.bRelease = TRUE;
	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetParam(OutPortIndex, VDOENC_PARAM_MAX_MEM_INFO, (UINT32) &MaxMemInfo);
	ImageUnit_End();
*/
}

void _ImageApp_Photo_FreeImgCapMaxBuf(void)
{
	/*
	NMI_IMGCAP_MAX_MEM_INFO ImgCapMaxInfo = {0};

	ImgCapMaxInfo.bRelease = 1;
	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_IMGCAP_MAX_MEM_INFO, (UINT32)&ImgCapMaxInfo);
	ImageUnit_End();
*/

}



void _ImageApp_Photo_CloseStrm(UINT32 OutPortIndex)
{
	/*
	_ImageApp_Photo_FreeVideoMaxBuf(OutPortIndex);
	_ImageApp_Photo_FreeImgCapMaxBuf();
*/

}

INT32 _ImageApp_Photo_OpenStrm(PHOTO_STRM_INFO *p_strm_info)
{
	/*
	UINT32    ime_out_path;
	//ISF_UNIT  *pStrmUnit;
	UINT32 port_state=0;

    DBG_FUNC_BEGIN("\r\n");

	if (p_strm_info == NULL) {
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX || p_strm_info->strm_id < PHOTO_STRM_ID_MIN) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_strm_info->vid_in >= PHOTO_VID_IN_MAX) {
		DBG_ERR("vid_in  =%d\r\n",p_strm_info->vid_in);
		return ISF_ERR_INVALID_PARAM;
	}

	if(p_strm_info->enable == PHOTO_PORT_STATE_EN_RUN){
		port_state=ISF_PORT_STATE_RUN;
	}else if(p_strm_info->enable == PHOTO_PORT_STATE_EN){
		port_state=ISF_PORT_STATE_OFF;
	}else{
		DBG_ERR("enable  =%d\r\n",p_strm_info->enable);
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_strm_info->is_merge_3dnr_path == TRUE)
		ime_out_path = ISF_OUT1;
	else
		ime_out_path = IME_STREAMING_PATH;

	ImageStream_Open(&ISF_Stream[p_strm_info->vid_in]);

	ImageStream_Begin(&ISF_Stream[p_strm_info->vid_in], 0);
	ImageStream_SetRoot(p_strm_info->vid_in, ImageUnit_In(ISF_VIN(p_strm_info->vid_in), ISF_IN1));

	ImageUnit_Begin(ISF_VIN(p_strm_info->vid_in), 0);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(ISF_IPL(p_strm_info->vid_in), ISF_IN1), port_state);
	ImageUnit_End();

	ImageUnit_Begin(ISF_IPL(p_strm_info->vid_in), 0);
	ImageUnit_SetOutput(ime_out_path, ImageUnit_In(&ISF_UserProc, ISF_IN1 + p_strm_info->strm_id), port_state);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, p_strm_info->width_ratio, p_strm_info->height_ratio);
	ImageUnit_End();

#if 0
	if (p_strm_info->strm_id == PHOTO_STRM_ID_MIN) {

		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + p_strm_info->strm_id, ImageUnit_In(&ISF_VdoEnc, ISF_IN1), ISF_PORT_STATE_RUN);
		ImageUnit_SetParam(ISF_OUT1 + p_strm_info->strm_id, USERPROC_PARAM_BYPASS_IMM, (p_strm_info->multi_view_type) ? FALSE : TRUE);
		ImageUnit_SetVdoAspectRatio(ISF_IN1, p_strm_info->width_ratio, p_strm_info->height_ratio);
		ImageUnit_End();

		if (p_strm_info->strm_type == PHOTO_STRM_TYPE_HTTP)
			pStrmUnit = &ISF_NetHTTP;
		else
			pStrmUnit = &ISF_StreamSender;//&ISF_NetRTSP;

		_ImageApp_Photo_AllocImgCapMaxBuf(p_strm_info);

		ImageUnit_Begin(&ISF_VdoEnc, 0);
		ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(pStrmUnit, ISF_IN1), ISF_PORT_STATE_RUN);
		ImageUnit_SetVdoImgSize(ISF_IN1, p_strm_info->width, p_strm_info->height);
		ImageUnit_SetVdoBufferCount(ISF_IN1, 1);
		_ImageApp_Photo_SetVideoMaxBuf(ISF_OUT1,p_strm_info);
		// set minimum snapshot size
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_IMGCAP_W, 16);
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_IMGCAP_H, 16);

		//ImageUnit_SetVdoFramerate(ISF_IN1, ISF_VDO_FRC(p_strm_info->frame_rate, 1));
		//set FPS for direct trigger mode
		ImageUnit_SetVdoFramerate(ISF_IN1, ISF_VDO_FRC(1, 1));
		if (p_strm_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_strm_info->width_ratio << 1, p_strm_info->height_ratio);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_strm_info->width_ratio, p_strm_info->height_ratio);
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_CODEC, p_strm_info->codec);
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_TARGETRATE, p_strm_info->target_bitrate);

		if(photo_strm_cbr_info[p_strm_info->strm_id-PHOTO_STRM_ID_MIN].enable){
        		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_CBRINFO, (UINT32) &(photo_strm_cbr_info[p_strm_info->strm_id-PHOTO_STRM_ID_MIN].cbr_info));
		}
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_RECFORMAT, MEDIAREC_LIVEVIEW);
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_ENCBUF_MS, 1500);
		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_ENCBUF_RESERVED_MS, 1000);
		ImageUnit_SetParam(ISF_CTRL, VDOENC_PARAM_EVENT_CB, (UINT32) _ImageApp_Photo_NMI_CB);
		//assign SYNCSRC to framerate source = VdoIn output
		ImageUnit_SetParam(ISF_IN1, VDOENC_PARAM_SYNCSRC, (UINT32)ISF_VIN(p_strm_info->vid_in));
		ImageUnit_End();

		ImageUnit_Begin(pStrmUnit, 0);
		ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
		if (p_strm_info->strm_type == PHOTO_STRM_TYPE_HTTP)
			ImageUnit_SetParam(ISF_OUT1, NETHTTP_PARAM_OPEN_CALLBACK_IMM, (UINT32)_ImageApp_Photo_open_http_cb);
		else {
#if 0//ISF_NetRTSP
			ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_MAX_CLIENT, PHOTO_RTSP_MAX_CLIENT);
			ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_AUDIO, 0);
			ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_MEDIA_SOURCE, RTSPNVT_MEDIA_SOURCE_BY_URL);
			ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_TOS, (0x20) << 2);
#else//ISF_StreamSender
			ImageUnit_SetParam(ISF_CTRL, STMSND_PARAM_SLOTNUM_VIDEO_IMM, 30);
#endif
		}
		ImageUnit_End();
	} else
#endif
	{
		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + p_strm_info->strm_id, ImageUnit_In(&ISF_Dummy, ISF_IN1 + p_strm_info->strm_id), port_state);
		ImageUnit_SetParam(ISF_OUT1 + p_strm_info->strm_id, USERPROC_PARAM_BYPASS_IMM, FALSE);
		ImageUnit_SetParam(ISF_OUT1 + p_strm_info->strm_id, USERPROC_PARAM_ALLOW_PULL_IMM, TRUE);

		ImageUnit_SetParam(ISF_OUT1 + p_strm_info->strm_id, USERPROC_PARAM_SYNC_SIZE_REF_ID, ISF_OUT1+PHOTO_STRM_ID_MIN);
		ImageUnit_SetVdoImgSize(ISF_IN1 + p_strm_info->strm_id, p_strm_info->width, p_strm_info->height);
		ImageUnit_End();

		ImageUnit_Begin(&ISF_Dummy, 0);
		ImageUnit_SetParam(ISF_IN1+p_strm_info->strm_id, DUMMY_PARAM_UNIT_TYPE_IMM, DUMMY_UNIT_TYPE_LEAF);
		ImageUnit_SetOutput(ISF_OUT1 + p_strm_info->strm_id, ISF_PORT_EOS, port_state);
		ImageUnit_End();
	}
	ImageStream_End();
	ImageStream_UpdateAll(&ISF_Stream[p_strm_info->vid_in]);
*/

	return 0;
}
 UINT32 ImageApp_Photo_UpdateImgLinkForStrm(PHOTO_STRM_ID strm_id, UINT32 action, BOOL allow_pull)
{
	/*
	UINT32    ime_out_path;
    	UINT32    id ;
    	PHOTO_IME3DNR_INFO *p_3dnr_info;

	if ((strm_id < PHOTO_STRM_ID_MAX) && (strm_id >= PHOTO_STRM_ID_MIN)) {
		id = strm_id - PHOTO_STRM_ID_MIN;
	} else {
		DBG_ERR("ID %d is out of range!\r\n", strm_id);
		return E_SYS;
	}
	#if 1
	if ((g_IsImgLinkForStrmEnabled[id] >= 2) && (action == ENABLE)) {
		DBG_ERR("id %d is already enabled!\r\n", id);
		return E_SYS;
	}
	if ((g_IsImgLinkForStrmEnabled[id] == 0) && (action == DISABLE)) {
		DBG_ERR("id %d is already disabled!\r\n", id);
		return E_SYS;
	}
	#endif
	DBG_IND("id=%d, strm_id=%d ,%d, %d\r\n", id, strm_id,action,allow_pull);

    	p_3dnr_info = &photo_ime3dnr_info[id];

	if (p_3dnr_info->is_merge_3dnr_path == TRUE)
		ime_out_path = ISF_OUT1;
	else
		ime_out_path = IME_STREAMING_PATH;


	DBG_IND("ime_out_path = %d\r\n",ime_out_path);

	ImageStream_Begin(&ISF_Stream[id], 0);

	if (action == ENABLE) {
        	ImageUnit_Begin(ISF_VIN(id), 0);
        	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
        	ImageUnit_End();

        	ImageUnit_Begin(ISF_IPL(id), 0);
        	ImageUnit_SetOutput(ime_out_path, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
        	ImageUnit_End();

		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + strm_id, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
		if (allow_pull) {
		    ImageUnit_SetParam(ISF_OUT1 + strm_id, USERPROC_PARAM_ALLOW_PULL_IMM, TRUE);
		}
		ImageUnit_End();

    		ImageUnit_Begin(&ISF_Dummy, 0);
		ImageUnit_SetOutput(ISF_OUT1 + strm_id, ISF_PORT_KEEP, ISF_PORT_STATE_RUN);
		ImageUnit_End();
		g_IsImgLinkForStrmEnabled[id]++;
	}else{
        	ImageUnit_Begin(ISF_VIN(id), 0);
        	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
        	ImageUnit_End();

        	ImageUnit_Begin(ISF_IPL(id), 0);
        	ImageUnit_SetOutput(ime_out_path, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
        	ImageUnit_End();

		ImageUnit_Begin(&ISF_UserProc, 0);
		ImageUnit_SetOutput(ISF_OUT1 + strm_id, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
		if (allow_pull) {
		    ImageUnit_SetParam(ISF_OUT1 + strm_id, USERPROC_PARAM_ALLOW_PULL_IMM, FALSE);
		}
		ImageUnit_End();

    		ImageUnit_Begin(&ISF_Dummy, 0);
		ImageUnit_SetOutput(ISF_OUT1 + strm_id, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
		ImageUnit_End();
		g_IsImgLinkForStrmEnabled[id]--;
	}
	ImageStream_End();

	ImageStream_UpdateAll(&ISF_Stream[id]);
*/

	return 0;
 }

INT32 _ImageApp_Photo_OpenIME3DNR(PHOTO_IME3DNR_INFO *p_3dnr_info)
{
	/*
	UINT32 pathid;
	UINT32 port_state=0;

	DBG_FUNC_BEGIN("\r\n");

	if (p_3dnr_info == NULL) {
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_3dnr_info->ime3dnr_id >= PHOTO_IME3DNR_ID_MAX || p_3dnr_info->ime3dnr_id < PHOTO_IME3DNR_ID_MIN) {
		DBG_ERR("ime3dnr_id  =%d\r\n",p_3dnr_info->ime3dnr_id);
		return ISF_ERR_INVALID_PARAM;
	}

	if (p_3dnr_info->vid_in >= PHOTO_VID_IN_MAX) {
		DBG_ERR("vid_in  =%d\r\n",p_3dnr_info->vid_in);
		return ISF_ERR_INVALID_PARAM;
	}

	if(p_3dnr_info->enable == PHOTO_PORT_STATE_EN_RUN){
		port_state=ISF_PORT_STATE_RUN;
	}else if(p_3dnr_info->enable == PHOTO_PORT_STATE_EN){
		port_state=ISF_PORT_STATE_OFF;
	}else{
		DBG_ERR("enable  =%d\r\n",p_3dnr_info->enable);
		return ISF_ERR_INVALID_PARAM;
	}

    pathid = p_3dnr_info->ime3dnr_id;
	//open
	ImageStream_Open(&ISF_Stream[p_3dnr_info->vid_in]);//(&ISF_Stream[pathid]);

	ImageStream_Begin(&ISF_Stream[p_3dnr_info->vid_in], 0);//(&ISF_Stream[pathid], 0);
	ImageStream_SetRoot(0, ImageUnit_In(ISF_VIN(p_3dnr_info->vid_in), ISF_IN1));

	ImageUnit_Begin(ISF_VIN(p_3dnr_info->vid_in), 0);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(ISF_IPL(p_3dnr_info->vid_in), ISF_IN1), port_state);
	ImageUnit_End();

    #if 0
	ImageUnit_Begin(g_isf_vin_info[p_3dnr_info->vid_in].p_imagepipe, 0);
	ImageUnit_SetParam(ISF_CTRL, IMAGEPIPE_PARAM_MODE, IMAGEPIPE_MODE_PREVIEW);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(&ISF_Dummy, ISF_IN1 + pathid), ISF_PORT_STATE_RUN);
	//ImageUnit_SetVdoAspectRatio(ISF_IN1, p_3dnr_info->width_ratio, p_3dnr_info->height_ratio);
	ImageUnit_SetParam(ISF_CTRL, IMAGEPIPE_PARAM_FUNC, IMAGEPIPE_FUNC_SQUARE);
	ImageUnit_End();
	#else

	ImageUnit_Begin(ISF_IPL(p_3dnr_info->vid_in), 0);
	ImageUnit_SetParam(ISF_CTRL, IMAGEPIPE_PARAM_MODE, IMAGEPIPE_MODE_PREVIEW);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(&ISF_ImgTrans, ISF_IN1 + pathid), port_state);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, p_3dnr_info->width_ratio, p_3dnr_info->height_ratio);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetOutput(ISF_OUT1 + pathid, ImageUnit_In(&ISF_Dummy, ISF_IN1 + pathid), port_state);
	ImageUnit_SetParam(ISF_OUT1 + pathid, IMGTRANS_PARAM_CONNECT_IMM, ISF_IN1 + pathid);
	ImageUnit_SetParam(ISF_OUT1 + pathid, IMGTRANS_PARAM_BYPASS_IMM, ISF_IN1 + pathid);
	ImageUnit_SetVdoImgSize(ISF_IN1 + pathid, p_3dnr_info->width, p_3dnr_info->height);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + pathid, p_3dnr_info->width_ratio, p_3dnr_info->height_ratio);
	ImageUnit_SetParam(ISF_IN1 + pathid, IMGTRANS_PARAM_MAX_IMG_WIDTH_IMM, p_3dnr_info->max_width);
	ImageUnit_SetParam(ISF_IN1 + pathid, IMGTRANS_PARAM_MAX_IMG_HEIGHT_IMM, p_3dnr_info->max_height);
	ImageUnit_End();
	#endif

	ImageUnit_Begin(&ISF_Dummy, 0);
	ImageUnit_SetParam(ISF_IN1+pathid, DUMMY_PARAM_UNIT_TYPE_IMM, DUMMY_UNIT_TYPE_LEAF);
	ImageUnit_SetVdoImgSize(ISF_IN1 + pathid, p_3dnr_info->width, p_3dnr_info->height);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + pathid, p_3dnr_info->width_ratio, p_3dnr_info->height_ratio);
	ImageUnit_SetOutput(ISF_OUT1 + pathid, ISF_PORT_EOS, port_state);
	ImageUnit_End();

	ImageStream_End();

	ImageStream_UpdateAll(&ISF_Stream[pathid]);
*/

	return 0;

}



INT32 _ImageApp_Photo_OpenCap(void)
{
	/*
	UINT32 vid_in = PHOTO_VID_IN_1;
	DBG_FUNC_BEGIN("\r\n");

	//open
	ImageStream_Open(&ISF_Stream[PHOTO_CAP_STRM_ID]);

	ImageStream_Begin(&ISF_Stream[PHOTO_CAP_STRM_ID], 0);
	ImageStream_SetRoot(0, ImageUnit_In(ISF_VIN(vid_in), ISF_IN1));

	ImageUnit_Begin(ISF_VIN(vid_in), 0);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(ISF_IPL(vid_in), ISF_IN1), ISF_PORT_STATE_RUN);
	ImageUnit_End();

	ImageUnit_Begin(ISF_IPL(vid_in), 0);
	ImageUnit_SetParam(ISF_CTRL, IMAGEPIPE_PARAM_MODE, IMAGEPIPE_MODE_PREVIEW);
	ImageUnit_SetOutput(ISF_OUTC, ImageUnit_In(&ISF_Cap, ISF_IN1), ISF_PORT_STATE_RUN);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_Cap, 0);
	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
	ImageUnit_SetParam(ISF_CTRL, CAP_PARAM_WRITEFILE_CB, (UINT32)ImageApp_Photo_WriteCB);
	ImageUnit_End();

	ImageStream_End();

	ImageStream_UpdateAll(&ISF_Stream[PHOTO_CAP_STRM_ID]);
*/

	return 0;
}

void _ImageApp_Photo_MergePath(void)
{
	/*
	UINT32             i;
	ISIZE              device_size;
	PHOTO_DISP_INFO    *p_disp_info;
	PHOTO_IME3DNR_INFO *p_3dnr_info;
	PHOTO_STRM_INFO    *p_strm_info;

	device_size = GxVideo_GetDeviceSize(DOUT1);
    for (i = 0; i < PHOTO_DISP_ID_MAX- PHOTO_DISP_ID_MIN; i++) {
		p_disp_info = &photo_disp_info[i];
		p_3dnr_info = &photo_ime3dnr_info[i];
		p_strm_info = &photo_strm_info[i];
		if (p_disp_info->enable && p_3dnr_info->enable && (UINT32)device_size.h == p_3dnr_info->height) {
			p_disp_info->is_merge_3dnr_path = TRUE;
			p_3dnr_info->is_merge_3dnr_path = TRUE;
		}
		else if (p_strm_info->enable && p_3dnr_info->enable && p_strm_info->height == p_3dnr_info->height) {
			p_strm_info->is_merge_3dnr_path = TRUE;
			p_3dnr_info->is_merge_3dnr_path = TRUE;
		}
		else {
			p_disp_info->is_merge_3dnr_path = FALSE;
			p_3dnr_info->is_merge_3dnr_path = FALSE;
			p_strm_info->is_merge_3dnr_path = FALSE;
		}
    }
*/

}

static ER _ImageApp_Photo_mem_init(void)
{
	HD_RESULT ret;

	if (g_photo_mem_cfg == NULL) {
		DBG_ERR("mem info not config yet\r\n");
		return E_SYS;
	}
	if ((ret = vendor_common_mem_relayout(g_photo_mem_cfg)) != HD_OK) {
		DBG_ERR("vendor_common_mem_relayout fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}
static ER _ImageApp_Photo_mem_uninit(void)
{
	return E_OK;
}
static ER _ImageApp_Photo_module_init(void)
{
	ER result = E_OK;
	HD_RESULT ret = HD_ERR_NG;

	// ImgLink
	if ((ret = hd_videocap_init()) != HD_OK) {
		DBG_ERR("hd_videocap_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_videoproc_init()) != HD_OK) {
		DBG_ERR("hd_videoproc_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_videoenc_init()) != HD_OK) {
		DBG_ERR("hd_videoenc_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
#if 0
	if ((ret = hd_bsmux_init()) != HD_OK) {
		DBG_ERR("hd_bsmux_init fail(%d)\r\n", ret);
		//result = E_SYS;
	}
	if ((ret = hd_fileout_init()) != HD_OK) {
		DBG_ERR("hd_fileout_init fail(%d)\r\n", ret);
		//result = E_SYS;
	}
#endif
	// DispLink
	#if 0
	// VdoOut flow refine: hd_videoout_init in project
	if ((ret = hd_videoout_init()) != HD_OK) {
		DBG_ERR("hd_videoout_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	#endif
#if 0
	// AudCapLink
	if ((ret = hd_audiocap_init()) != HD_OK) {
		DBG_ERR("hd_audiocap_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_audioenc_init()) != HD_OK) {
		DBG_ERR("hd_audioenc_init fail(%d)\r\n", ret);
		result = E_SYS;
	}
#endif
	return result;
}

static ER _ImageApp_Photo_module_uninit(void)
{
	ER result = E_OK;
	HD_RESULT ret;

	// ImgLink
	if ((ret = hd_videocap_uninit()) != HD_OK) {
		DBG_ERR("hd_videocap_uninit fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_videoproc_uninit()) != HD_OK) {
		DBG_ERR("hd_videoproc_uninit fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_videoenc_uninit()) != HD_OK) {
		DBG_ERR("hd_videoenc_uninit fail(%d)\r\n", ret);
		result = E_SYS;
	}
#if 0
	if ((ret = hd_bsmux_uninit()) != HD_OK) {
		DBG_ERR("hd_bsmux_uninit fail(%d)\r\n", ret);
		//result = E_SYS;
	}
	if ((ret = hd_fileout_uninit()) != HD_OK) {
		DBG_ERR("hd_fileout_uninit fail(%d)\r\n", ret);
		//result = E_SYS;
	}
#endif
	// DispLink
	#if 0
	if ((ret = hd_videoout_uninit()) != HD_OK) {
		DBG_ERR("hd_videoout_uninit fail(%d)\r\n", ret);
		result = E_SYS;
	}
	#endif
#if 0
	// AudCapLink
	if ((ret = hd_audiocap_uninit()) != HD_OK) {
		DBG_ERR("hd_audiocap_uninit fail(%d)\r\n", ret);
		result = E_SYS;
	}
	if ((ret = hd_audioenc_uninit()) != HD_OK) {
		DBG_ERR("hd_audioenc_uninit fail(%d)\r\n", ret);
		result = E_SYS;
	}
#endif
	return result;
}

HD_RESULT _ImageApp_Photo_venc_set_param(HD_PATH_ID venc_path, HD_VIDEOENC_IN *pin_param, HD_VIDEOENC_OUT2 *pout_param)
{
	HD_RESULT ret = HD_OK;

	//--- HD_VIDEOENC_PARAM_IN ---
	ret |= hd_videoenc_set(venc_path, HD_VIDEOENC_PARAM_IN, pin_param);
	ret |= hd_videoenc_set(venc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, pout_param);

	return ret;
}
HD_URECT _ImageApp_Photo_vcap_set_crop_out(UINT32 img_ratio, ISIZE sensor_info_size)
{
	//set out crop for 4:3 cap size
	//HD_URECT VcapOutCropRect;
	USIZE ImageRatioSize = {0};
	HD_URECT vcap_out_crop_rect= {0};

	ImageRatioSize.w=((img_ratio & 0xffff0000) >> 16);
	ImageRatioSize.h=(img_ratio & 0x0000ffff);
	//crop width
	if (((float) sensor_info_size.w/sensor_info_size.h) >= ((float)ImageRatioSize.w/ImageRatioSize.h)){
	    vcap_out_crop_rect.h =sensor_info_size.h;
	    vcap_out_crop_rect.w = ALIGN_ROUND(vcap_out_crop_rect.h/ImageRatioSize.h * ImageRatioSize.w, 4);
	}else{
	    vcap_out_crop_rect.w = sensor_info_size.w;
	    vcap_out_crop_rect.h = ALIGN_ROUND(vcap_out_crop_rect.w/ImageRatioSize.w * ImageRatioSize.h, 4);
	}
	vcap_out_crop_rect.x=(sensor_info_size.w-vcap_out_crop_rect.w)>>1;
	vcap_out_crop_rect.y=(sensor_info_size.h-vcap_out_crop_rect.h)>>1;

	//DBG_DUMP("photo_vcap_sensor_info.w= %d, %d, img_ratio=0x%x, ImageRatioSize.w=%d, %d\r\n", photo_vcap_sensor_info[idx].sSize[0].w,photo_vcap_sensor_info[idx].sSize[0].h,photo_cap_info[idx].img_ratio, ImageRatioSize.w,ImageRatioSize.h);

	//DBG_DUMP("rect %d, %d, %d, %d\r\n", photo_vcap_out_crop_rect.x,photo_vcap_out_crop_rect.y,photo_vcap_out_crop_rect.w,photo_vcap_out_crop_rect.h);
	return vcap_out_crop_rect;
}


#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)

void ImageApp_Photo_Open(void)
{
	HD_RESULT ret = HD_OK;
	HD_PATH_ID video_cap_ctrl[PHOTO_VID_IN_MAX] = {0};
	//HD_PATH_ID           vcap_p[PHOTO_VID_IN_MAX]= {0};
	//HD_VIDEOCAP_SYSCAPS  vcap_syscaps[PHOTO_VID_IN_MAX]= {0};

	HD_VIDEOCAP_IN vcap_in_param[PHOTO_VID_IN_MAX] = {0};
	HD_VIDEOCAP_CROP vcap_crop_param[PHOTO_VID_IN_MAX] = {0};
	HD_VIDEOCAP_OUT vcap_out_param[PHOTO_VID_IN_MAX] = {0};
    HD_VIDEOCAP_FUNC_CONFIG vcap_func_config = {0};  //for direct


	HD_VIDEOPROC_DEV_CONFIG vproc_cfg[PHOTO_VID_IN_MAX] = {0};
	HD_VIDEOPROC_CTRL vproc_ctrl[PHOTO_VID_IN_MAX] = {0};
	HD_PATH_ID video_proc_ctrl[PHOTO_VID_IN_MAX]  = {0};
	//HD_VIDEOENC_IN  venc_in_param = {0};
	//HD_VIDEOENC_OUT2 venc_out_param = {0};
	HD_VIDEOPROC_FUNC_CONFIG vprc_func_config = {0};  //for direct

	HD_VIDEOPROC_OUT vprc_out_param = {0};

	HD_VIDEOENC_PATH_CONFIG venc_cfg = {0};

	//HD_PATH_ID           vproc_p_phy[4];

	//HD_PATH_ID venc_p[PHOTO_VID_IN_MAX]  = {0};

	UINT32 idx;
	UINT32 j=0;
	// init memory
	if (_ImageApp_Photo_mem_init() != HD_OK) {
		// VdoOut flow refine:
		//_imageapp_moviemulti_common_uninit();
		DBG_ERR("_ImageApp_Photo_mem_init fail\r\n");
		return;
	}
	// init module
	if (_ImageApp_Photo_module_init() != HD_OK) {
		_ImageApp_Photo_mem_uninit();
		// VdoOut flow refine:
		DBG_ERR("_ImageApp_Photo_module_init fail\r\n");
		return ;
	}
	ImageApp_Common_Init();

	if (photo_disp_info[PHOTO_DISP_ID_1].enable) { //move here for ImageApp_Photo_DispGetVdoImgSize
		_ImageApp_Photo_DispLinkOpen(&photo_disp_info[PHOTO_DISP_ID_1]);
	}

	for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {

		DBG_IND("ImageApp_Photo_Open enable[%d] =(%d)\n", (idx-PHOTO_VID_IN_1), photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);

		if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
			//if ((ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl[idx])) != HD_OK) {
			if ((ret = hd_videocap_open( 0,  HD_VIDEOCAP_CTRL(idx), &video_cap_ctrl[idx])) != HD_OK) {
				DBG_ERR("hd_videocap_open fail, idx=%d, ret=%d\n",idx, ret);
				return;
			}

			ret |= hd_videocap_set(video_cap_ctrl[idx], HD_VIDEOCAP_PARAM_DRV_CONFIG, &photo_vcap_sensor_info[idx].vcap_cfg);

			if (HD_VIDEO_PXLFMT_CLASS(photo_vcap_sensor_info[idx].capout_pxlfmt) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
				photo_vcap_sensor_info[idx].vcap_ctrl.func=0;
			}

			ret |= hd_videocap_set(video_cap_ctrl[idx], HD_VIDEOCAP_PARAM_CTRL, &photo_vcap_sensor_info[idx].vcap_ctrl);
			if(ret != HD_OK){
				DBG_ERR("vcap_set_cfg fail(%d)\n", ret);
			}
			ret =hd_videocap_open(HD_VIDEOCAP_IN(idx, 0), HD_VIDEOCAP_OUT(idx, 0), &photo_vcap_sensor_info[idx].vcap_p[0]);
			if(ret != HD_OK){
				DBG_ERR("vcap_open_ch fail(%d)\n", ret);
			}
			ret =hd_videocap_get(video_cap_ctrl[idx], HD_VIDEOCAP_PARAM_SYSCAPS, &photo_vcap_syscaps[idx]);
			if(ret != HD_OK){
				DBG_ERR("vcap_get_caps fail(%d)\n", ret);
			}

			vcap_in_param[idx].sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO;  //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
			vcap_in_param[idx].frc = HD_VIDEO_FRC_RATIO(photo_vcap_sensor_info[idx].fps[0], 1);
			vcap_in_param[idx].dim.w = photo_vcap_sensor_info[idx].sSize[0].w;//sz.w;
			vcap_in_param[idx].dim.h = photo_vcap_sensor_info[idx].sSize[0].h;//sz.h;
			vcap_in_param[idx].pxlfmt = photo_vcap_sensor_info[idx].senout_pxlfmt;
			//vcap_in_param[idx].out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
			if (photo_vcap_sensor_info[idx].vcap_ctrl.func & HD_VIDEOCAP_FUNC_SHDR) {
				vcap_in_param[idx].out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
			}else{
				vcap_in_param[idx].out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
			}
			memset(&vcap_crop_param[idx],0, sizeof(HD_VIDEOCAP_CROP));
			vcap_crop_param[idx].mode = HD_CROP_OFF;

			vcap_out_param[idx].pxlfmt = photo_vcap_sensor_info[idx].capout_pxlfmt;
			vcap_out_param[idx].dir = HD_VIDEO_DIR_NONE;
			vcap_out_param[idx].depth = 0;//1; //set 1 to allow pull

            if (g_photo_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
                vcap_func_config.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
                if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_FUNC_CONFIG, &vcap_func_config)) != HD_OK) {
					DBG_ERR("set HD_VIDEOCAP_PARAM_FUNC_CONFIG fail(%d)\r\n", ret);
				}
            }

			if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_IN, &vcap_in_param[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOCAP_PARAM_IN fail(%d)\r\n", ret);
			}
			if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_IN_CROP, &vcap_crop_param[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
			}
			if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT, &vcap_out_param[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
			}
			#if 0
			//set out crop for 4:3 cap size
			//HD_URECT VcapOutCropRect;
			USIZE ImageRatioSize = {0};
			ImageRatioSize.w=((photo_cap_info[idx].img_ratio & 0xffff0000) >> 16);
			ImageRatioSize.h=(photo_cap_info[idx].img_ratio & 0x0000ffff);
			//crop width
                    if (((float) photo_vcap_sensor_info[idx].sSize[0].w/photo_vcap_sensor_info[idx].sSize[0].h) >= ((float)ImageRatioSize.w/ImageRatioSize.h)){
                        photo_vcap_out_crop_rect.h = photo_vcap_sensor_info[idx].sSize[0].h;
                        photo_vcap_out_crop_rect.w = ALIGN_ROUND(photo_vcap_out_crop_rect.h/ImageRatioSize.h * ImageRatioSize.w, 4);
                    }else{
                        photo_vcap_out_crop_rect.w = photo_vcap_sensor_info[idx].sSize[0].w;
                        photo_vcap_out_crop_rect.h = ALIGN_ROUND(photo_vcap_out_crop_rect.w/ImageRatioSize.w * ImageRatioSize.h, 4);
                    }
			photo_vcap_out_crop_rect.x=(photo_vcap_sensor_info[idx].sSize[0].w-photo_vcap_out_crop_rect.w)>>1;
			photo_vcap_out_crop_rect.y=(photo_vcap_sensor_info[idx].sSize[0].h-photo_vcap_out_crop_rect.h)>>1;

			//DBG_DUMP("photo_vcap_sensor_info.w= %d, %d, img_ratio=0x%x, ImageRatioSize.w=%d, %d\r\n", photo_vcap_sensor_info[idx].sSize[0].w,photo_vcap_sensor_info[idx].sSize[0].h,photo_cap_info[idx].img_ratio, ImageRatioSize.w,ImageRatioSize.h);
			#else
			photo_vcap_out_crop_rect =_ImageApp_Photo_vcap_set_crop_out(photo_cap_info[idx].img_ratio, photo_vcap_sensor_info[idx].sSize[0]);

			//DBG_DUMP("rect %d, %d, %d, %d\r\n", photo_vcap_out_crop_rect.x,photo_vcap_out_crop_rect.y,photo_vcap_out_crop_rect.w,photo_vcap_out_crop_rect.h);
			memset(&vcap_crop_param[idx],0, sizeof(HD_VIDEOCAP_CROP));
			vcap_crop_param[idx].mode = HD_CROP_ON;
			vcap_crop_param[idx].win.rect.x   = photo_vcap_out_crop_rect.x;
			vcap_crop_param[idx].win.rect.y   = photo_vcap_out_crop_rect.y;
			vcap_crop_param[idx].win.rect.w   = photo_vcap_out_crop_rect.w;
			vcap_crop_param[idx].win.rect.h   = photo_vcap_out_crop_rect.h;
			vcap_crop_param[idx].align.w = 4;
			vcap_crop_param[idx].align.h = 4;
			#endif
			if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT_CROP, &vcap_crop_param[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
			}

			UINT32 data_lane = photo_vcap_sensor_info[idx].data_lane;
			if ((ret = vendor_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], VENDOR_VIDEOCAP_PARAM_DATA_LANE, &data_lane)) != HD_OK) {
				DBG_ERR("set vendor_videocap_set fail(%d)\r\n", ret);
			}

			//vcap0 bind-> VIDEOPROC_0_IN_0
			//vcap1 bind-> VIDEOPROC_2_IN_0
			//if ((ret = hd_videocap_bind(HD_VIDEOCAP_OUT(idx, 0), HD_VIDEOPROC_IN(0, 0))) != HD_OK) {
			if (g_photo_vcap_func_cfg[idx].out_func != HD_VIDEOCAP_OUTFUNC_DIRECT) {
				if ((ret = hd_videocap_bind(HD_VIDEOCAP_OUT(idx, 0), HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0))) != HD_OK) {   //if direct mode move down after vprc open
					DBG_ERR("set hd_videocap_bind fail idx=%d, ret=%d\r\n", idx, ret);
				}
			}
			photo_vcap_sensor_info[idx].video_cap_ctrl =	video_cap_ctrl[idx];
			//sensor1
			//vprc0  bind_src  VIDEOCAP_0_OUT_0
			//         out 0 3DNR
			//              1 DISP
			//              2 WIFI
			//vprc1 	bind_src null
			//         out 0 main jpg
			//              1 screen nail
			//              2 QV/exif thumb

			//sensor2
			//vprc2  bind_src VIDEOCAP_1_OUT_0
			//         out 0 3DNR
			//              1 DISP
			//              2 WIFI
			//vprc3 	bind_src null
			//         out 0 main jpg
			//              1 screen nail
			//              2 QV/exif thumb

			// set videoproc
			// liveview, bind
			vproc_cfg[idx].isp_id = idx;//0x00000000;
			//DBG_DUMP("[%d]capout_pxlfmt=0x%x, 0x%x, 0x%x\r\n", idx, photo_vcap_sensor_info[idx].capout_pxlfmt, HD_VIDEO_PXLFMT_CLASS(photo_vcap_sensor_info[idx].capout_pxlfmt),HD_VIDEO_PXLFMT_CLASS_YUV);
			if (HD_VIDEO_PXLFMT_CLASS(photo_vcap_sensor_info[idx].capout_pxlfmt) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
				vproc_cfg[idx].pipe= HD_VIDEOPROC_PIPE_YUVALL;
				vproc_cfg[idx].ctrl_max.func = 0;//ImgLinkRecInfo[idx][0].vproc_func;
				vproc_ctrl[idx].func = 0;//ImgLinkRecInfo[idx][0].vproc_func;
			}else{
				vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_RAWALL;
				vproc_cfg[idx].ctrl_max.func = photo_vcap_sensor_info[idx].vproc_func;//ImgLinkRecInfo[idx][0].vproc_func;
				vproc_ctrl[idx].func = photo_vcap_sensor_info[idx].vproc_func;//ImgLinkRecInfo[idx][0].vproc_func;
			}
			vproc_cfg[idx].in_max.func = 0;
			vproc_cfg[idx].in_max.dim.w = photo_vcap_syscaps[idx].max_dim.w;//ImgLink[idx].vcap_syscaps.max_dim.w;
			vproc_cfg[idx].in_max.dim.h = photo_vcap_syscaps[idx].max_dim.h;//ImgLink[idx].vcap_syscaps.max_dim.h;
			vproc_cfg[idx].in_max.pxlfmt = photo_vcap_sensor_info[idx].capout_pxlfmt;//img_vcap_capout_pxlfmt[idx];
			vproc_cfg[idx].in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
			DBG_IND("[%d]vprc in_max,w=%d, %d\r\n", idx, vproc_cfg[idx].in_max.dim.w, vproc_cfg[idx].in_max.dim.h);

			//vproc_ctrl[idx].func = photo_vcap_sensor_info[idx].vproc_func;//ImgLinkRecInfo[idx][0].vproc_func;
			if (vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				//vproc_ctrl[idx].ref_path_3dnr = HD_VIDEOPROC_OUT(0, g_photo_3ndr_path);//photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_3DNR_PATH];//HD_VIDEOPROC_OUT(0, 0);
				vproc_ctrl[idx].ref_path_3dnr = HD_VIDEOPROC_OUT(idx, g_photo_3ndr_path);//photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_3DNR_PATH];//HD_VIDEOPROC_OUT(0, 0);
			}


			//if ((ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl[idx])) != HD_OK) {
			if ((ret = hd_videoproc_open(0, HD_VIDEOPROC_CTRL((idx*PHOTO_VID_IN_MAX+0)), &video_proc_ctrl[idx])) != HD_OK) {
				DBG_ERR("HD_VIDEOPROC_0_CTRL hd_videoproc_open fail(%d)\r\n", ret);
			}

			if (g_photo_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
				vprc_func_config.in_func |= HD_VIDEOPROC_INFUNC_DIRECT;
				if ((ret = hd_videoproc_set(video_proc_ctrl[idx], HD_VIDEOPROC_PARAM_FUNC_CONFIG, &vprc_func_config)) != HD_OK) {
					DBG_ERR("set HD_VIDEOPROC_PARAM_FUNC_CONFIG fail(%d)\r\n", ret);
				}
			}

			if ((ret = hd_videoproc_set(video_proc_ctrl[idx], HD_VIDEOPROC_PARAM_DEV_CONFIG, &vproc_cfg[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOPROC_PARAM_DEV_CONFIG fail(%d)\r\n", ret);
			}
			if ((ret = hd_videoproc_set(video_proc_ctrl[idx], HD_VIDEOPROC_PARAM_CTRL, &vproc_ctrl[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOPROC_PARAM_CTRL fail(%d)\r\n", ret);
			}


			//HD_VIDEOPROC_OUT(idx, j) , j=0~3, vproc_p_phy[j]
			//j=0, for 3DNR, out size = in size, vcap crop to vprc in= vprc out
			//j=1  for Disp
			//j=2  for Wifi
			j=g_photo_3ndr_path;
			//if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(0, 0), HD_VIDEOPROC_OUT(0, j), &photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
			if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0), HD_VIDEOPROC_OUT((idx*PHOTO_VID_IN_MAX+0), j), &photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				DBG_ERR("hd_videoproc_open fail(%d), in=%d, out=%d\r\n", ret, idx, j);
			}
			//HD_VIDEOPROC_OUT vprc_out_param = {0};
			memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));
			vprc_out_param.func = 0;

			vprc_out_param.dim.w = photo_vcap_out_crop_rect.w;//photo_vcap_sensor_info[idx].sSize[j].w;//p_dim->w;
			vprc_out_param.dim.h = photo_vcap_out_crop_rect.h;//photo_vcap_sensor_info[idx].sSize[j].h;//p_dim->h;
			vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
			vprc_out_param.dir = HD_VIDEO_DIR_NONE;
			vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
			vprc_out_param.depth = 1; //allow pull
			if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
			}

			if (photo_disp_info[PHOTO_DISP_ID_1].enable==PHOTO_PORT_STATE_EN_RUN) {
				j=IME_DISPLAY_PATH;
				//if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(0, 0), HD_VIDEOPROC_OUT(0, j), &photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0), HD_VIDEOPROC_OUT((idx*PHOTO_VID_IN_MAX+0), j), &photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
					DBG_ERR("hd_videoproc_open fail(%d), in=%d, out=%d\r\n", ret,idx, j);
				}
				//HD_VIDEOPROC_OUT video_out_param = {0};
				memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));
				vprc_out_param.func = 0;
				//modify for NA51055-1264
				//ImageApp_Photo_DispGetVdoImgSize(idx, (UINT32*)&photo_vcap_sensor_info[idx].sSize[j].w,(UINT32*)&photo_vcap_sensor_info[idx].sSize[j].h);
				if(photo_disp_imecrop_info[idx].enable==0){
					SIZECONVERT_INFO     CovtInfo = {0};
					CovtInfo.uiSrcWidth  = photo_cap_info[idx].sCapSize.w;//src ratio
					CovtInfo.uiSrcHeight = photo_cap_info[idx].sCapSize.h;//src ratio
					CovtInfo.uiDstWidth  = g_PhotoDispLink[0].vout_syscaps.output_dim.w;
					CovtInfo.uiDstHeight = g_PhotoDispLink[0].vout_syscaps.output_dim.h;

					if ((photo_disp_info[PHOTO_DISP_ID_1].rotate_dir != HD_VIDEO_DIR_NONE) && (CovtInfo.uiDstWidth < CovtInfo.uiDstHeight)) {
						CovtInfo.uiDstWidth = g_PhotoDispLink[0].vout_syscaps.output_dim.h;
						CovtInfo.uiDstHeight  = g_PhotoDispLink[0].vout_syscaps.output_dim.w;
					}
					CovtInfo.uiDstWRatio = g_PhotoDispLink[0].vout_ratio.w;
					CovtInfo.uiDstHRatio = g_PhotoDispLink[0].vout_ratio.h;
					CovtInfo.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
					DBG_IND("Src w %d, %d, uiDstWidth=%d, %d ,uiDstWRatio=%d, %d\r\n", CovtInfo.uiSrcHeight,CovtInfo.uiSrcWidth,CovtInfo.uiDstWidth,CovtInfo.uiDstHeight,CovtInfo.uiDstWRatio,CovtInfo.uiDstHRatio);

					DisplaySizeConvert(&CovtInfo);
                    if (photo_disp_info[PHOTO_DISP_ID_1].rotate_dir != HD_VIDEO_DIR_NONE)  {
                        CovtInfo.uiOutWidth = ALIGN_CEIL_8(CovtInfo.uiOutWidth);
                        CovtInfo.uiOutHeight = ALIGN_CEIL_8(CovtInfo.uiOutHeight);
                    }
					//DBG_DUMP("CovtInfo %d, %d, %d, %d\r\n", CovtInfo.uiOutX,CovtInfo.uiOutY,CovtInfo.uiOutWidth,CovtInfo.uiOutHeight);
					//DBG_DUMP("IMESize.w=%d, %d\r\n", photo_disp_imecrop_info[idx].IMESize.w, photo_disp_imecrop_info[idx].IMESize.h);
					//DBG_DUMP("enable=%d, IMEWin.x=%d, %d, %d, %d\r\n", photo_disp_imecrop_info[idx].enable,photo_disp_imecrop_info[idx].IMEWin.x, photo_disp_imecrop_info[idx].IMEWin.y, photo_disp_imecrop_info[idx].IMEWin.w, photo_disp_imecrop_info[idx].IMEWin.h);

					vprc_out_param.dim.w = CovtInfo.uiOutWidth;//photo_vcap_sensor_info[idx].sSize[j].w;//p_dim->w;
					vprc_out_param.dim.h = CovtInfo.uiOutHeight;//photo_vcap_sensor_info[idx].sSize[j].h;//p_dim->h;
					vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
					vprc_out_param.dir = HD_VIDEO_DIR_NONE;
					vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
					vprc_out_param.depth = 1;//allow pull
					if ((ret =hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
						DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT) fail(%d)\r\n", ret);
					}
				}else{
					//DBG_DUMP("IMESize.w=%d, %d\r\n", photo_disp_imecrop_info[idx].IMESize.w, photo_disp_imecrop_info[idx].IMESize.h);
					//DBG_DUMP("enable=%d, IMEWin.x=%d, %d, %d, %d\r\n", photo_disp_imecrop_info[idx].enable,photo_disp_imecrop_info[idx].IMEWin.x, photo_disp_imecrop_info[idx].IMEWin.y, photo_disp_imecrop_info[idx].IMEWin.w, photo_disp_imecrop_info[idx].IMEWin.h);

					vprc_out_param.dim.w = photo_disp_imecrop_info[idx].IMESize.w;//photo_vcap_sensor_info[idx].sSize[j].w;//p_dim->w;
					vprc_out_param.dim.h = photo_disp_imecrop_info[idx].IMESize.h;//photo_vcap_sensor_info[idx].sSize[j].h;//p_dim->h;
					vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
					vprc_out_param.dir = HD_VIDEO_DIR_NONE;
					vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
					vprc_out_param.depth = 1;//allow pull
					if ((ret =hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
						DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT) fail(%d)\r\n", ret);
					}

					HD_VIDEOPROC_CROP video_out_crop_param = {0};
					video_out_crop_param.mode =HD_CROP_ON;
					video_out_crop_param.win.rect.x = photo_disp_imecrop_info[idx].IMEWin.x;
					video_out_crop_param.win.rect.y = photo_disp_imecrop_info[idx].IMEWin.y;
					video_out_crop_param.win.rect.w = photo_disp_imecrop_info[idx].IMEWin.w;
					video_out_crop_param.win.rect.h = photo_disp_imecrop_info[idx].IMEWin.h;
					if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param)) != HD_OK) {
						DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_CROP) fail(%d)\r\n", ret);
					}
				}
			}

			if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable==PHOTO_PORT_STATE_EN_RUN) {

				j=IME_STREAMING_PATH;
				//if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(0, 0), HD_VIDEOPROC_OUT(0, j), &photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
				if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0), HD_VIDEOPROC_OUT((idx*PHOTO_VID_IN_MAX+0), j), &photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
					DBG_ERR("hd_videoproc_open fail(%d), in=%d, out=%d\r\n", ret,idx, j);
				}
				//HD_VIDEOPROC_OUT video_out_param = {0};
				memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));
				vprc_out_param.func = 0;

				//vprc_out_param.dim.w = photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].max_width;
				//vprc_out_param.dim.h = photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].max_height;
				vprc_out_param.dim.w = photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].width;
				vprc_out_param.dim.h = photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].height;
				vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
				vprc_out_param.dir = HD_VIDEO_DIR_NONE;
				vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
				vprc_out_param.depth = 1;//allow pull
				ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param);
			}



			// for cap ,not bind
			// set videoproc
			memset(&vproc_cfg[idx],0 ,sizeof(HD_VIDEOPROC_DEV_CONFIG));
			if (HD_VIDEO_PXLFMT_CLASS(photo_vcap_sensor_info[idx].capout_pxlfmt) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
				vproc_cfg[idx].pipe= HD_VIDEOPROC_PIPE_YUVCAP;
				vproc_cfg[idx].ctrl_max.func = 0;//ImgLinkRecInfo[idx][0].vproc_func;
				vproc_ctrl[idx].func = 0;//ImgLinkRecInfo[idx][0].vproc_func;
			}else{
				vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_RAWCAP;
				vproc_cfg[idx].ctrl_max.func = photo_vcap_sensor_info[idx].vproc_func;//ImgLinkRecInfo[idx][0].vproc_func;
				vproc_ctrl[idx].func = photo_vcap_sensor_info[idx].vproc_func & (~HD_VIDEOPROC_FUNC_3DNR);//ImgLinkRecInfo[idx][0].vproc_func;
			}
			vproc_cfg[idx].isp_id = 0x00010000+ idx;

			vproc_cfg[idx].in_max.func = 0;
			vproc_cfg[idx].in_max.dim.w = photo_cap_info[idx].sCapMaxSize.w;//photo_cap_info[idx].sCapSize.w;//vcap_syscaps[idx].max_dim.w;//ImgLink[idx].vcap_syscaps.max_dim.w;
			vproc_cfg[idx].in_max.dim.h = photo_cap_info[idx].sCapMaxSize.h;//photo_cap_info[idx].sCapSize.h;//vcap_syscaps[idx].max_dim.h;//ImgLink[idx].vcap_syscaps.max_dim.h;
			vproc_cfg[idx].in_max.pxlfmt = photo_vcap_sensor_info[idx].capout_pxlfmt;//img_vcap_capout_pxlfmt[idx];
			vproc_cfg[idx].in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
			DBG_IND("[%d]cap vprc in_max,w=%d, %d\r\n", idx, vproc_cfg[idx].in_max.dim.w, vproc_cfg[idx].in_max.dim.h);

			//memset(&vproc_ctrl[idx],0 ,sizeof(HD_VIDEOPROC_CTRL));
			//vproc_ctrl[idx].func = photo_vcap_sensor_info[idx].vproc_func;//ImgLinkRecInfo[idx][0].vproc_func;
			//if (vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				//vproc_ctrl[idx].ref_path_3dnr = HD_VIDEOPROC_OUT(1, VPRC_CAP_3DNR_PATH);//photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_3DNR_PATH];//HD_VIDEOPROC_OUT(1, 0);
			//}
			//jira :NA51055-1469
			//cap not support 3DNR
			//vproc_ctrl[idx].func = photo_vcap_sensor_info[idx].vproc_func & (~HD_VIDEOPROC_FUNC_3DNR);//ImgLinkRecInfo[idx][0].vproc_func;


			//if ((ret = hd_videoproc_open(0, HD_VIDEOPROC_1_CTRL, &video_proc_ctrl[idx])) != HD_OK) {
			if ((ret = hd_videoproc_open(0, HD_VIDEOPROC_CTRL((idx*PHOTO_VID_IN_MAX+1)), &video_proc_ctrl[idx])) != HD_OK) {
				DBG_ERR("HD_VIDEOPROC_1_CTRL hd_videoproc_open fail(%d)\r\n", ret);
			}
			if ((ret = hd_videoproc_set(video_proc_ctrl[idx], HD_VIDEOPROC_PARAM_DEV_CONFIG, &vproc_cfg[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOPROC_PARAM_DEV_CONFIG fail(%d)\r\n", ret);
			}
			if ((ret = hd_videoproc_set(video_proc_ctrl[idx], HD_VIDEOPROC_PARAM_CTRL, &vproc_ctrl[idx])) != HD_OK) {
				DBG_ERR("set HD_VIDEOPROC_PARAM_CTRL fail(%d)\r\n", ret);
			}


			if (g_photo_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
				if ((ret = hd_videocap_bind(HD_VIDEOCAP_OUT(idx, 0), HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0))) != HD_OK) {   //if direct mode move down after vprc open
					DBG_ERR("set hd_videocap_bind fail idx=%d, ret=%d\r\n", idx, ret);
				}
			}

			#if 0
			// 3DNR
			j=VPRC_CAP_3DNR_PATH;
			if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
				DBG_ERR("cap hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
				return ;
			}
			//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
			memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));
			vprc_out_param.func = 0;
			vprc_out_param.dim.w =  photo_vcap_sensor_info[idx].sSize[j].w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
			vprc_out_param.dim.h =  photo_vcap_sensor_info[idx].sSize[j].h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
			vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
			vprc_out_param.dir = HD_VIDEO_DIR_NONE;
			vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
			vprc_out_param.depth = 1; //allow pull
			if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
				return ;
			}

			// main jpg
			j=VPRC_CAP_MAIN_PATH;
			if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
				DBG_ERR("cap hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
				return ;
			}
			//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
			memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));

			vprc_out_param.func = 0;

			vprc_out_param.dim.w = photo_cap_info[idx].sCapSize.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
			vprc_out_param.dim.h = photo_cap_info[idx].sCapSize.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
			vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
			vprc_out_param.dir = HD_VIDEO_DIR_NONE;
			vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
			vprc_out_param.depth = 1; //allow pull
			if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
				return ;
			}

			// set videoproc
			// screennail
			DBG_ERR("screen_img=%d\r\n", photo_cap_info[idx].screen_img);
			if(photo_cap_info[idx].screen_img){
				j=VPRC_CAP_SCR_THUMB_PATH;
				if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
					DBG_ERR("cap hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
					return ;
				}

				//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
				memset(&vprc_out_param, 0, sizeof(vprc_out_param));
				vprc_out_param.func = 0;

				vprc_out_param.dim.w = photo_cap_info[idx].screen_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
				vprc_out_param.dim.h = photo_cap_info[idx].screen_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
				vprc_out_param.pxlfmt = photo_cap_info[idx].screen_fmt;//HD_VIDEO_PXLFMT_YUV420;
				vprc_out_param.dir = HD_VIDEO_DIR_NONE;
				vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
				vprc_out_param.depth = 1; //allow pull
				if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
					DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
					return ;
				}
			}
			// set videoproc
			// quickview
			DBG_ERR("qv_img=%d\r\n", photo_cap_info[idx].qv_img);
			if(photo_cap_info[idx].qv_img){
				j=VPRC_CAP_QV_PATH;
				if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
					DBG_ERR("cap hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
					return ;
				}

				//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
				memset(&vprc_out_param, 0, sizeof(vprc_out_param));
				vprc_out_param.func = 0;

				vprc_out_param.dim.w = photo_cap_info[idx].qv_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
				vprc_out_param.dim.h = photo_cap_info[idx].qv_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
				vprc_out_param.pxlfmt = photo_cap_info[idx].qv_img_fmt;//HD_VIDEO_PXLFMT_YUV420;
				vprc_out_param.dir = HD_VIDEO_DIR_NONE;
				vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
				vprc_out_param.depth = 1; //allow pull
				if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
					DBG_ERR("hd_videoproc_set fail(%d)\r\n", ret);
					return ;
				}
			}
			#endif
			// set videoenc ONLY ONE
			if(idx==PHOTO_VID_IN_1){
				// venc set cfg
				//if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &photo_cap_info[idx].venc_p)) != HD_OK){
				if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0, idx), HD_VIDEOENC_OUT(0, idx), &photo_cap_info[idx].venc_p)) != HD_OK){
					DBG_ERR("hd_videoenc_open fail(%d)\r\n", ret);
					return ;
				}
				//HD_VIDEOENC_PATH_CONFIG venc_cfg = {0};
				memset(&venc_cfg,0,sizeof(HD_VIDEOENC_PATH_CONFIG));

				//#define CAP_TBR			(((4032 * 3024 * 3/2) /5) * 8) //yuv 420, fix-quality 70, assume compress ratio = 1/5, for max resolution 4032x3024
				//#define CAP_BUF			10000 //ms
				#define CAP_BUF			1600//1500//2000 //ms
				venc_cfg.max_mem.codec_type = HD_CODEC_TYPE_JPEG;
				venc_cfg.max_mem.max_dim.w  = photo_cap_info[idx].sCapMaxSize.w;//photo_cap_info[idx].sCapSize.w;//vcap_syscaps[idx].max_dim.w;
				venc_cfg.max_mem.max_dim.h  = photo_cap_info[idx].sCapMaxSize.h;//photo_cap_info[idx].sCapSize.h;//vcap_syscaps[idx].max_dim.h;
				//venc_cfg.max_mem.bitrate    = (((photo_cap_info[idx].sCapMaxSize.w*photo_cap_info[idx].sCapMaxSize.h* 3/2) /5) * 8);//CAP_TBR;//max_bitrate;
				venc_cfg.max_mem.bitrate    = VDO_YUV_BUFSIZE(photo_cap_info[idx].sCapMaxSize.w, photo_cap_info[idx].sCapMaxSize.h, photo_cap_info[idx].jpgfmt) * 8 / 5;
				venc_cfg.max_mem.enc_buf_ms = CAP_BUF;//buf_ms;
				//venc_cfg.max_mem.enc_buf_ms += ((65536 * 2 * 8 * 1000) / venc_cfg.max_mem.bitrate );

				if(venc_cfg.max_mem.bitrate >= 128 * 1024 * 1024){
						DBG_WRN("bitrate(%lu) is out-of-range(0~128Mbit), fix to 96 Mbit\n", venc_cfg.max_mem.bitrate);
						venc_cfg.max_mem.bitrate = 96 * 1024 * 1024;
				}

				venc_cfg.max_mem.svc_layer  = HD_SVC_DISABLE;//HD_SVC_4X;
				venc_cfg.max_mem.ltr        = FALSE;//TRUE;
				venc_cfg.max_mem.rotate     = FALSE;
				venc_cfg.max_mem.source_output   = FALSE;
				venc_cfg.isp_id             = 0;
				ret = hd_videoenc_set(photo_cap_info[idx].venc_p, HD_VIDEOENC_PARAM_PATH_CONFIG, &venc_cfg);
				if (ret != HD_OK) {
					DBG_ERR("hd_videoenc_set fail(%d)\r\n", ret);
					return ;
				}
			}
#if 0
			// venc set param
			//--- HD_VIDEOENC_PARAM_IN ---
			memset(&photo_cap_venc_in_param,0,sizeof(HD_VIDEOENC_IN));
			photo_cap_venc_in_param.dir           = HD_VIDEO_DIR_NONE;
			photo_cap_venc_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
			if(photo_cap_info[idx].screen_img){
				photo_cap_venc_in_param.dim.w   = photo_cap_info[idx].screen_img_size.w;//p_dim->w;
				photo_cap_venc_in_param.dim.h   = photo_cap_info[idx].screen_img_size.h;//p_dim->h;
			}else{
				photo_cap_venc_in_param.dim.w   = photo_cap_info[idx].sCapSize.w;//p_dim->w;
				photo_cap_venc_in_param.dim.h   = photo_cap_info[idx].sCapSize.h;//p_dim->h;
			}
			photo_cap_venc_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
			ret = hd_videoenc_set(photo_cap_info[idx].venc_p, HD_VIDEOENC_PARAM_IN, &photo_cap_venc_in_param);
			if (ret != HD_OK) {
				DBG_ERR("set_enc_param_in = %d\r\n", ret);
				return;
			}

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			memset(&photo_cap_venc_out_param,0,sizeof(HD_VIDEOENC_OUT2));
			photo_cap_venc_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			photo_cap_venc_out_param.jpeg.retstart_interval = 0;
#if 0//PHOTO_BRC_MODE
			photo_cap_venc_out_param.jpeg.image_quality = 0;
			photo_cap_venc_out_param.jpeg.bitrate       = bitrate;
#else
			photo_cap_venc_out_param.jpeg.image_quality = 70;
			photo_cap_venc_out_param.jpeg.bitrate       = 0;
#endif
			ret = hd_videoenc_set(photo_cap_info[idx].venc_p, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &photo_cap_venc_out_param);
			if (ret != HD_OK) {
				DBG_ERR("set_enc_param_out = %d\r\n", ret);
				return;
			}
#endif
		}
	}


	for (idx= PHOTO_STRM_ID_MIN; idx < PHOTO_STRM_ID_MAX; idx++) {
		if (photo_strm_info[idx - PHOTO_STRM_ID_MIN].enable ==PHOTO_PORT_STATE_EN_RUN) {
			_ImageApp_Photo_WiFiLinkOpen(&photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN]);
			g_IsImgLinkForStrmEnabled[idx - PHOTO_STRM_ID_MIN]=1;
		}
	}

	if (photo_cap_info[PHOTO_CAP_ID_1].enable) {
		_ImageApp_Photo_CapLinkOpen(&photo_cap_info[PHOTO_CAP_ID_1]);
	}

	for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
		DBG_IND("photo_vcap_sensor_info, idx=%d, en=%d\r\n", idx,photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);

		if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
			if (g_photo_vcap_func_cfg[idx].out_func != HD_VIDEOCAP_OUTFUNC_DIRECT) {
				hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0]);
			}
			if (photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
				j=g_photo_3ndr_path;
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j]);
			}
			DBG_IND("photo_disp_info, en=%d\r\n", photo_disp_info[0].enable);
			if (photo_disp_info[PHOTO_DISP_ID_1-PHOTO_DISP_ID_MIN].enable) {
				j=IME_DISPLAY_PATH;
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j]);
				_ImageApp_Photo_DispLinkStart(&photo_disp_info[PHOTO_DISP_ID_1]);
			}

			if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
				j=IME_STREAMING_PATH;
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][j]);
				//ImageApp_Photo_WiFiLinkStart(&photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN]);  //move down after vcap start
			}

			DBG_IND("photo_cap_info, en=%d\r\n", photo_cap_info[PHOTO_CAP_ID_1].enable);
			if (photo_cap_info[PHOTO_CAP_ID_1].enable) {
				_ImageApp_Photo_CapLinkStart(&photo_cap_info[PHOTO_CAP_ID_1]);
			}
			if (g_photo_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
				hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0]);
			}
			if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
				ImageApp_Photo_WiFiLinkStart(&photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN]);
			}

		}
	}
}

UINT32 ImageApp_Photo_CapStart(void)
{
	/*
	ImageUnit_Begin(&ISF_Cap, 0);
	ImageUnit_SetParam(ISF_CTRL, CAP_PARAM_CAP_START_IMM, TRUE);
	ImageUnit_End();
*/
	_ImageApp_Photo_TriggerCap();
	return 0;
}

void ImageApp_Photo_CapStop(void)
{
	/*
	ImageUnit_Begin(&ISF_Cap, 0);
	ImageUnit_SetParam(ISF_CTRL, CAP_PARAM_CAP_STOP_IMM, TRUE);
	ImageUnit_End();
*/

}

void ImageApp_Photo_Close(void)
{
	/*
	UINT32 i;

    //close cap strem first, to release the capture buffer for video out change mode using
	i = PHOTO_CAP_STRM_ID;
	//close
	ImageStream_Close(&ISF_Stream[i]);


	for (i = PHOTO_DISP_ID_MIN; i < PHOTO_DISP_ID_MAX; i++) {
		if (photo_disp_info[i - PHOTO_DISP_ID_MIN].enable) {
			//close
			ImageStream_Close(&ISF_Stream[i]);
			photo_disp_info[i - PHOTO_DISP_ID_MIN].enable = FALSE;
		}
	}
	for (i = PHOTO_STRM_ID_MIN; i < PHOTO_STRM_ID_MAX; i++) {
		if (photo_strm_info[i - PHOTO_STRM_ID_MIN].enable) {
			//close
			ImageStream_Close(&ISF_Stream[i]);
			photo_strm_info[i - PHOTO_STRM_ID_MIN].enable = FALSE;
		}
	}
	for (i = PHOTO_IME3DNR_ID_MIN; i < PHOTO_IME3DNR_ID_MAX; i++) {
		if (photo_ime3dnr_info[i - PHOTO_IME3DNR_ID_MIN].enable) {
			//close
			ImageStream_Close(&ISF_Stream[i]);
			photo_ime3dnr_info[i - PHOTO_IME3DNR_ID_MIN].enable = FALSE;
		}
	}
	_ImageApp_Photo_DispLinkClose();
	_ImageApp_Photo_WiFiLinkClose();
	for (i = PHOTO_STRM_ID_MIN; i < PHOTO_STRM_ID_MAX; i++) {
		g_IsImgLinkForStrmEnabled[i-PHOTO_STRM_ID_MIN]=0;
	}
	ImageApp_Photo_FileNaming_Close();
	_ImageApp_Photo_CloseStrm(ISF_OUT1);

	g_photo_dual_disp.enable=FALSE;
*/
	//vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_FLOW_LINK_CFG);
	//_ImageApp_MovieMulti_Stream_Close();
	//_ImageApp_MovieMulti_UninstallID();
	UINT32 idx, j;
	HD_RESULT ret;

	_ImageApp_Photo_DispLinkClose(&photo_disp_info[PHOTO_DISP_ID_1]);

	for (j = PHOTO_STRM_ID_MIN; j < PHOTO_STRM_ID_MAX; j++) {
		if (photo_strm_info[j - PHOTO_STRM_ID_MIN].enable) {
			//close
			_ImageApp_Photo_WiFiLinkClose(&photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN]);
			//photo_strm_info[j - PHOTO_STRM_ID_MIN].enable = FALSE;
		}
	}

	for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
		if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
			if (g_photo_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
				if (1){//photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
					j=g_photo_3ndr_path;
					if (photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
						if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
							DBG_ERR("3dnr hd_videoproc_stop fail(%d)\n", ret);
						}
					}
					//if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
					//	DBG_ERR("3dnr hd_videoproc_close fail(%d)\n", ret);
					//}
				}
				DBG_DUMP("disp en=%d, wifi en=%d\n", photo_disp_info[PHOTO_DISP_ID_1].enable,photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable);

				if (photo_disp_info[PHOTO_DISP_ID_1].enable) {
					j=IME_DISPLAY_PATH;
					if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("disp hd_videoproc_stop fail(%d)\n", ret);
					}
					//if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
					//	DBG_ERR("disp hd_videoproc_close fail(%d)\n", ret);
					//}
				}
				if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
					j=IME_STREAMING_PATH;
					if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("wifi hd_videoproc_stop fail(%d)\n", ret);
					}
					//if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
					//	DBG_ERR("wifi hd_videoproc_close fail(%d)\n", ret);
					//}
					//photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].enable = FALSE;
				}

				if ((ret = hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
					DBG_ERR("hd_videocap_stop fail(%d)\n", ret);
				}
				if ((ret = hd_videocap_unbind(HD_VIDEOCAP_OUT(idx, 0))) != HD_OK) {
					DBG_ERR("hd_videocap_unbind fail(%d)\n", ret);
				}

				j=g_photo_3ndr_path;
				if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
					DBG_ERR("3dnr hd_videoproc_close fail(%d)\n", ret);
				}
				if (photo_disp_info[PHOTO_DISP_ID_1].enable) {
					j=IME_DISPLAY_PATH;
					if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("disp hd_videoproc_close fail(%d)\n", ret);
					}
				}
				if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
					j=IME_STREAMING_PATH;
					if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("wifi hd_videoproc_close fail(%d)\n", ret);
					}
					photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].enable = FALSE;
				}


				if ((ret = hd_videocap_close(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
					DBG_ERR("hd_videocap_close fail(%d)\n", ret);
				}

			}else{
				if ((ret = hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
					DBG_ERR("hd_videocap_stop fail(%d)\n", ret);
				}
				if ((ret = hd_videocap_unbind(HD_VIDEOCAP_OUT(idx, 0))) != HD_OK) {
					DBG_ERR("hd_videocap_unbind fail(%d)\n", ret);
				}

				if ((ret = hd_videocap_close(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
					DBG_ERR("hd_videocap_close fail(%d)\n", ret);
				}

				if (1){//photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
					j=g_photo_3ndr_path;
					if (photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
						if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
							DBG_ERR("3dnr hd_videoproc_stop fail(%d)\n", ret);
						}
					}
					if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("3dnr hd_videoproc_close fail(%d)\n", ret);
					}
				}
				DBG_DUMP("disp en=%d, wifi en=%d\n", photo_disp_info[PHOTO_DISP_ID_1].enable,photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable);

				if (photo_disp_info[PHOTO_DISP_ID_1].enable) {
					j=IME_DISPLAY_PATH;
					if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("disp hd_videoproc_stop fail(%d)\n", ret);
					}
					if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("disp hd_videoproc_close fail(%d)\n", ret);
					}
				}
				if (photo_strm_info[PHOTO_STRM_ID_1-PHOTO_STRM_ID_MIN].enable) {
					j=IME_STREAMING_PATH;
					if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("wifi hd_videoproc_stop fail(%d)\n", ret);
					}
					if ((ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[0][j])) != HD_OK) {
						DBG_ERR("wifi hd_videoproc_close fail(%d)\n", ret);
					}
					photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].enable = FALSE;
				}
			}
		}
	}
	if (photo_cap_info[PHOTO_CAP_ID_1].enable) {
		_ImageApp_Photo_CapLinkClose(&photo_cap_info[PHOTO_CAP_ID_1]);
	}
	ImageApp_Common_Uninit();
	// uninit module
	if (_ImageApp_Photo_module_uninit() != HD_OK) {
		DBG_ERR("_ImageApp_Photo_module_uninit fail\r\n");
	}
	// uninit memory
	if (_ImageApp_Photo_mem_uninit() != HD_OK) {
		DBG_ERR("_ImageApp_Photo_mem_uninit fail\r\n");
	}
	ImageApp_Photo_FileNaming_Close();
	for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
		g_photo_cap_sensor_mode_cfg[idx].in_size.w=0;
		g_photo_cap_sensor_mode_cfg[idx].in_size.h=0;
		g_photo_cap_sensor_mode_cfg[idx].frc=0;
	}
}


UINT32 ImageApp_Photo_SetScreenSleep(void)
{
/*
	UINT32 i = 0;

	for(i = 0; i< ISF_MAX_STREAM; i++)
	{
		//check if imagestream is already open?
		if (ImageStream_IsOpen(&ISF_Stream[i]) == 0) {
			//DBGD(i);
			// Make Stream sleep.
			ImageStream_Begin(&ISF_Stream[i], 0);
			ImageStream_SetOutputDownstream(ImageUnit_In(ISF_IPL(0), ISF_IN1), ISF_PORT_STATE_SLEEP);
			ImageStream_End();
			ImageStream_UpdateAll(&ISF_Stream[i]);
		}
	}
*/

	return 0;
}

UINT32 ImageApp_Photo_SetScreenWakeUp(void)
{
	/*
	UINT32 i = 0;

	for(i = 0; i< ISF_MAX_STREAM; i++)
	{
		//check if imagestream is already open?
		if (ImageStream_IsOpen(&ISF_Stream[i]) == 0) {
			//DBGD(i);
			// Make Stream run.
			ImageStream_Begin(&ISF_Stream[i], 0);
			ImageStream_SetOutputDownstream(ImageUnit_In(ISF_IPL(0), ISF_IN1), ISF_PORT_STATE_RUN);
			ImageStream_End();
			ImageStream_UpdateAll(&ISF_Stream[i]);
		}
	}
*/

	return 0;
}

INT32 ImageApp_Photo_SetFormatFree(BOOL is_on)
{
#if 1
	g_photo_formatfree_is_on = (INT32)is_on;
	DBG_IND("^G SetFormatFree %d\r\n", is_on);
#endif
	return 0;
}

INT32 ImageApp_Photo_IsFormatFree(void)
{
#if 1
	return g_photo_formatfree_is_on;
#else
	return 0;
#endif
}

void ImageApp_Photo_Get_Hdal_Path(PHOTO_VID_IN VID_IN, PHOTO_HDAL_PATH hd_path, UINT32 *pValue)
{
    if (VID_IN >= PHOTO_VID_IN_MAX) {
        DBG_ERR("Not corrected PHOTO_VID_IN\r\n");
        return;
    }

    if (hd_path == PHOTO_HDAL_VCAP_CAP_PATH) {
        *pValue = photo_vcap_sensor_info[VID_IN].vcap_p[0];
    } else if (hd_path == PHOTO_HDAL_VPRC_3DNR_REF_PATH) {
        *pValue = photo_vcap_sensor_info[VID_IN].vproc_p_phy[0][g_photo_3ndr_path];
    } else {
        DBG_ERR("Not corrected PHOTO_HDAL_PATH\r\n");
    }
}
/////////////////////////////////////////////////////////////////////////////



