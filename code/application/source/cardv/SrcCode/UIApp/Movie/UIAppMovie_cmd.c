//global debug level: PRJ_DBG_LVL
#include "PrjInc.h"
#include <kwrap/stdio.h>

#include "System/SysCommon.h"
//#include "SysCfg.h"
#include "UIApp/AppLib.h"
#include "UIApp/UIAppCommon.h"
#include "Mode/UIModeMovie.h"
#include "UIApp/Movie/UIAppMovie.h"  // Isiah, implement YUV merge mode of recording func.
//#include "UIAppMovie_Param.h"
//#include "UIMovieMapping.h"

#include "Mode/UIModePhoto.h"
#include "UIApp/Photo/UIAppPhoto.h"
#if (USE_FILEDB==ENABLE)
#include "FileDB.h"
#endif
//#include "ImageApp_MovieCommon.h"
#include "ImageApp/ImageApp_MovieMulti.h"
#include "UIApp/AppDisp_PipView.h"
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
#include "UIApp/Network/EthCamAppCmd.h"
#include "UIApp/Network/EthCamAppNetwork.h"
#include "UIApp/Network/EthCamAppSocket.h"
#endif
#include "System/SysCommon.h"
#include <kwrap/sxcmd.h>
#include <kwrap/stdio.h>
#include "kwrap/cmdsys.h"
#include <string.h>
#include "UIWnd/UIFlow.h"

//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UiAppMovieCmd
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

static BOOL Cmd_movie(unsigned char argc, char **argv)
{
	UINT32 menuIdx, value;

	//sscanf_s(strCmd, "%d %d", &menuIdx, &value);
	sscanf_s(argv[0],"%d", &menuIdx);
	sscanf_s(argv[1],"%d", &value);
	DBG_DUMP("menuIdx=%d, value=%d\r\n", menuIdx,value);

	if (menuIdx == 0) {
#if (MOVIE_REC_YUVMERGE == ENABLE)
		BOOL bEnable;
		UINT32 uiInterval;
		DBG_DUMP("Test merge record\r\n");
		sscanf_s(strCmd, "%d %d %d", &menuIdx, &bEnable, &uiInterval);
		FlowMovie_RecSetYUVMergeRecInterval(uiInterval);
		FlowMovie_RecSetYUVMergeRecCounter(0);
		FlowMovie_RecSetYUVMergeMode(bEnable);
		if (bEnable) {
			debug_msg("Enable YUV merge record mode, interval=%d\r\n", uiInterval);
		} else {
			debug_msg("Disable YUV merge record mode\r\n");
		}
#endif
	} else if (menuIdx == 1) {
#if(MOVIE_MODE==ENABLE)
		DBG_DUMP("ADAS(LDWS+FCW) %d\r\n", value);
		Ux_SendEvent(&CustomMovieObjCtrl,   NVTEVT_EXE_MOVIE_LDWS,          1,  value);
		Ux_SendEvent(&CustomMovieObjCtrl,   NVTEVT_EXE_MOVIE_FCW,           1,  value);
#endif
	} else if (menuIdx == 2) {
#if(MOVIE_MODE==ENABLE)
		DBG_DUMP("Urgent Protect Manual %d\r\n", value);
		Ux_SendEvent(&CustomMovieObjCtrl,   NVTEVT_EXE_MOVIE_PROTECT_MANUAL, 1,  value);
#endif
	} else if (menuIdx == 3) {
#if(MOVIE_MODE==ENABLE)
		DBG_DUMP("Image Rotation %d\r\n", value);
		Ux_SendEvent(&CustomMovieObjCtrl,   NVTEVT_EXE_MOVIE_SENSOR_ROTATE, 1,  value);
#endif
	} else if (menuIdx == 5) {
#if(MOVIE_MODE==ENABLE)
		DBG_DUMP("Movie RSC %d\r\n", value);
		Ux_SendEvent(&CustomMovieObjCtrl,   NVTEVT_EXE_MOVIE_RSC,          1,  value);
#endif
	} else if (menuIdx == 7) {
#if(PHOTO_MODE==ENABLE)
		UI_SetData(FL_DUAL_CAM, value);
#if(defined(_NVT_ETHREARCAM_RX_))
		MovieExe_EthCam_ChgDispCB(UI_GetData(FL_DUAL_CAM));
#endif
		DBG_DUMP("Sensor index %d\r\n", value);
		Ux_SendEvent(&CustomMovieObjCtrl,   NVTEVT_EXE_DUALCAM,      1, SysGetFlag(FL_DUAL_CAM));
#endif
	} else if (menuIdx == 11) {
#if(MOVIE_MODE==ENABLE)
		DBG_DUMP("Movie WDR index %d\r\n", value);
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_WDR, 1, value);
#endif
	} else if (menuIdx == 13) {
		DBG_DUMP("enter value=%d\r\n", value);
		if(value>=DUALCAM_FRONT && value<DUALCAM_SETTING_MAX )
		{
		}
		else
		{
		    value=DUALCAM_FRONT;
		}
#if(MOVIE_MODE==ENABLE)
		DBG_DUMP("out value=%d\r\n", value);
		UI_SetData(FL_DUAL_CAM, value);
		UI_SetData(FL_DUAL_CAM_MENU, value);

		//Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_IMAGE_RATIO, 1, GetMovieSizeRatio(UI_GetData(FL_MOVIE_SIZE)));
		PipView_SetStyle(UI_GetData(FL_DUAL_CAM));
#endif
	}

	return TRUE;
}
static BOOL Cmd_movie_dumpfdb(unsigned char argc, char **argv)
{
#if (USE_FILEDB==ENABLE)
	FileDB_DumpInfo(0);
#if defined(_EMBMEM_EMMC_) && (FS_MULTI_STRG_FUNC==ENABLE)
	FileDB_DumpInfo(1);
#endif
#endif
	return TRUE;
}

#if(defined(_NVT_ETHREARCAM_TX_))
static BOOL Cmd_movie_eth_tx_start(unsigned char argc, char **argv)
{
	UINT32 value;
	sscanf_s(argv[0],"%d", &value);

	if(value==0){
		MovieExe_EthCamTxStart(_CFG_REC_ID_1);
	}else{
		MovieExe_EthCamTxStart(_CFG_CLONE_ID_1);
	}
	return TRUE;
}
static BOOL Cmd_movie_eth_tx_stop(unsigned char argc, char **argv)
{
	UINT32 value;
	sscanf_s(argv[0],"%d", &value);
	if(value==0){
		MovieExe_EthCamTxStop(_CFG_REC_ID_1);
	}else{
		MovieExe_EthCamTxStop(_CFG_CLONE_ID_1);
	}
	return TRUE;
}
static BOOL Cmd_movie_set_tx_link_status(unsigned char argc, char **argv)
{
	EthCamNet_EthLinkStatusNotify("ethlinknotify 201369792 2");
	return TRUE;
}

#endif
static BOOL Cmd_movie_HDR(unsigned char argc, char **argv)
{
	UINT32 value;

	sscanf_s(argv[0],"%d", &value);
	if(value){
		DBG_DUMP("HDR On\r\n");
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_SHDR, 1, MOVIE_HDR_ON);

	}else{
		DBG_DUMP("HDR Off\r\n");
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_SHDR, 1, MOVIE_HDR_OFF);
	}
	Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, System_GetState(SYS_STATE_CURRMODE));

	return TRUE;
}

#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
static BOOL Cmd_movie_get_desc(unsigned char argc, char **argv)
{
#if 0//(defined(_NVT_ETHREARCAM_TX_))
	UINT8 tem_buf[50];
	MEM_RANGE desc;
	MEM_RANGE sps;
	MEM_RANGE pps;
	MEM_RANGE vps;
	UINT32 i;
	UINT8 *pBuf;
	desc.Addr=(UINT32)tem_buf;
	desc.Size=50;
	ImageApp_MovieMulti_GetDesc(_CFG_REC_ID_1, _CFG_CODEC_H265, &desc, &sps, &pps, &vps);
	DBG_DUMP("\r\ndesc[%d]:\r\n",desc.Size);
	pBuf=(UINT8 *)desc.Addr;
	for(i=0;i<desc.Size;i++){
		DBG_DUMP("0x%x, ", pBuf[i]);
	}
	DBG_DUMP("\r\nsps[%d]:\r\n",sps.Size);

	pBuf=(UINT8 *)sps.Addr;
	for(i=0;i<sps.Size;i++){
		DBG_DUMP("0x%x, ", pBuf[i]);
	}
	DBG_DUMP("\r\npps[%d]:\r\n",pps.Size);

	pBuf=(UINT8 *)pps.Addr;
	for(i=0;i<pps.Size;i++){
		DBG_DUMP("0x%x, ", pBuf[i]);
	}
	DBG_DUMP("\r\nvps[%d]:\r\n",vps.Size);
	pBuf=(UINT8 *)vps.Addr;
	for(i=0;i<vps.Size;i++){
		DBG_DUMP("0x%x, ", pBuf[i]);
	}
	DBG_DUMP("\r\n");
#endif
	return TRUE;
}
static BOOL Cmd_movie_trigger_thumb(unsigned char argc, char **argv)
{
#if(defined(_NVT_ETHREARCAM_TX_))
	//ImageApp_MovieMulti_EthCam_TxTrigThumb((0 | ETHCAM_TX_MAGIC_KEY));
#else
	UINT16 i;
	for(i=0;i<ETH_REARCAM_CAPS_COUNT;i++){
		EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_MOVIE_THUMB, 0);
	}

#endif
	return TRUE;
}
static BOOL Cmd_movie_trigger_rawencode(unsigned char argc, char **argv)
{
#if(defined(_NVT_ETHREARCAM_TX_))
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE) {
		DBG_ERR("not movie mode\r\n");
		return TRUE;
	}

	//680,510 media not record,also can raw encode
	if(ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1 | ETHCAM_TX_MAGIC_KEY)){
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_RAWENC, 0);
	} else {
		DBG_ERR("Not in recording state\r\n");

		return TRUE;
	}
#else
	UINT16 i;
	for(i=0;i<ETH_REARCAM_CAPS_COUNT;i++){
		EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_MOVIE_RAW_ENCODE, 0);
	}
#endif
	return TRUE;
}
static BOOL Cmd_movie_disp_crop(unsigned char argc, char **argv)
{
#if _TODO
#if(defined(_NVT_ETHREARCAM_RX_))
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	UINT32 value;
	sscanf_s(argv[0],"%d", &value);
	DBG_DUMP("value=%d\r\n",value);
	MOVIEMULTI_IME_CROP_INFO CropInfo ={0};
	CropInfo.IMESize.w = 1920;
	CropInfo.IMESize.h = 1080;
	CropInfo.IMEWin.x = 0;
	CropInfo.IMEWin.w = 1920;
	CropInfo.IMEWin.h = 384;
	if(value>=0 && ((value + CropInfo.IMEWin.h) < CropInfo.IMESize.h)){
		CropInfo.IMEWin.y = value;
		EthCam_SendXMLCmd(ETHCAM_PATH_ID_1, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_TX_DISP_CROP, 0);
		EthCam_SendXMLData(ETHCAM_PATH_ID_1, (UINT8 *)&CropInfo, (UINT32)sizeof(MOVIEMULTI_IME_CROP_INFO));
	}else{
		DBG_ERR("input out of range\r\n");
	}

#endif
#endif
#endif
	return TRUE;
}
#endif

BOOL Cmd_movie_mov_file_test(unsigned char argc, char **argv)
{
	UINT32 i;
	for (i = 0; i < (SENSOR_CAPS_COUNT & SENSOR_ON_MASK); i++) {
		gMovie_Rec_Info[i].file_format=_CFG_FILE_FORMAT_MOV;
		gMovie_Rec_Info[i].aud_codec=_CFG_AUD_CODEC_PCM;
	}
	#if(defined(_NVT_ETHREARCAM_RX_))
	gEthcamRecInfoAudCodec=_CFG_AUD_CODEC_PCM;
	gEthcamRecInfoRecFormat=_CFG_FILE_FORMAT_MOV;
	#endif
	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);

	return TRUE;
}
BOOL Cmd_movie_ts_file_test(unsigned char argc, char **argv)
{
	UINT32 i;

	for (i = 0; i < (SENSOR_CAPS_COUNT & SENSOR_ON_MASK); i++) {
		gMovie_Rec_Info[i].file_format=_CFG_FILE_FORMAT_TS;
		gMovie_Rec_Info[i].aud_codec=_CFG_AUD_CODEC_AAC;
	}
	#if(defined(_NVT_ETHREARCAM_RX_))
	gEthcamRecInfoAudCodec=_CFG_AUD_CODEC_AAC;
	gEthcamRecInfoRecFormat=_CFG_FILE_FORMAT_TS;
	#endif
	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);

	return TRUE;
}
BOOL Cmd_movie_frontmoov_test(unsigned char argc, char **argv)
{
	UINT32 i;

	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		ImageApp_MovieMulti_SetParam(_CFG_REC_ID_1 + i, MOVIEMULTI_PARAM_FILE_FRONT_MOOV, TRUE);
//		ImageApp_MovieMulti_SetParam(_CFG_REC_ID_1 + i, MOVIEMULTI_PARAM_FILE_FRONT_MOOV_FLUSH_SEC, 5);
	}
#if(defined(_NVT_ETHREARCAM_RX_))
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
			ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1 + i, MOVIEMULTI_PARAM_FILE_FRONT_MOOV, TRUE);
//			ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1 + i, MOVIEMULTI_PARAM_FILE_FRONT_MOOV_FLUSH_SEC, 5);
		}
	}
#endif

	return TRUE;
}

static SXCMD_BEGIN(uimovie_cmd_tbl, "uimovie command")
SXCMD_ITEM("movie %", Cmd_movie, "movie mode test")
SXCMD_ITEM("dumpfdb",   Cmd_movie_dumpfdb,   "dump filedb 0")
SXCMD_ITEM("movfile", Cmd_movie_mov_file_test, "movie mode mov file test")
SXCMD_ITEM("tsfile", Cmd_movie_ts_file_test, "movie mode ts file test")
SXCMD_ITEM("frontmoov", Cmd_movie_frontmoov_test, "movie mode enable front moov")

#if(defined(_NVT_ETHREARCAM_TX_))
SXCMD_ITEM("linkup",   Cmd_movie_set_tx_link_status,   "eth linkup")
SXCMD_ITEM("ethstart %",   Cmd_movie_eth_tx_start,   "eth stream tx start")
SXCMD_ITEM("ethstop %",   Cmd_movie_eth_tx_stop,   "eth stream tx stop")
#endif
SXCMD_ITEM("hdr %",   Cmd_movie_HDR,   "HDR on/off")
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
SXCMD_ITEM("desc",   Cmd_movie_get_desc,   "get current resolution desc")
SXCMD_ITEM("thumb",   Cmd_movie_trigger_thumb,   "trigger thumb")
SXCMD_ITEM("pim",   Cmd_movie_trigger_rawencode,   "trigger raw encode")
#endif
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
SXCMD_ITEM("ethcamcrop %",   Cmd_movie_disp_crop,   "ethcam set tx disp crop")
#endif
SXCMD_END()

static int uimovie_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(uimovie_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "uimovie");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", uimovie_cmd_tbl[loop].p_name, uimovie_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(uimovie, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(uimovie_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		uimovie_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], uimovie_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = uimovie_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}


