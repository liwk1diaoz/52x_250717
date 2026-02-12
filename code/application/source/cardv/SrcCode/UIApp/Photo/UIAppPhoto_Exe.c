////////////////////////////////////////////////////////////////////////////////
#include "SysCommon.h"
//#include "AppCommon.h"
////////////////////////////////////////////////////////////////////////////////
#include "UIFramework.h"
#include "UIApp/Photo/UIAppPhoto.h"      //include EVENT of Photo App
#include "ImageApp/ImageApp_Photo.h"
#include "vendor_isp.h"
#include "vendor_videocapture.h"
#include "UIAppPhoto_Param.h"
#include "SysSensor.h"
#include <kwrap/type.h>
#include <kwrap/util.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>

#include "GxSound.h"
#include "sys_mempool.h"
#include "comm/hwclock.h"
#include "NamingRule/NameRule_Custom.h"
#include "FileDB.h"
#include "SizeConvert.h"
#include "UIApp/ExifVendor.h"
#include "UIApp/Photo/UIDateImprint.h"
#if(WIFI_AP_FUNC==ENABLE)
#include "UIApp/Network/WifiAppCmd.h"
#include "UIApp/MovieStamp/MovieStamp.h"
#include "UIApp/Network/UIAppWiFiCmd.h"
#endif
#include "UIApp/AppDisp_PipView.h"
#include <vf_gfx.h>
#include "vendor_videocapture.h"

#if (HEIC_FUNC == ENABLE)
#include "UIApp/Movie/UIAppMovie_SampleCode_HEIC.h"
#include "heifnvt/nvt_wrapper.h"
#endif

#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#if 1//_TODO
#define __MODULE__          UiAppPhoto
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
#endif

#define DZOOM_MAX_STEP  6 //Please setting acording the DZoom table

BOOL _g_bFirstPhoto = TRUE;

#if(PHOTO_MODE==ENABLE)

#if (USE_FILEDB==ENABLE)
static PHOTO_CAP_FOLDER_NAMING gPhoto_Folder_Naming = {
	PHOTO_CAP_ID_1,             //cap_id
	0,                 //ipl_id
//	IPL_PATH_1,                 //ipl_id
	"Photo"                    //folder_path
};
#endif


#define PHOTO_ROOT_PATH         "A:\\Novatek\\"
#define FILE_SN_MAX		999999


//static UINT32 gPhotoDzoomStop = 0;
static INT32 g_i32PhotoFileSerialNum = 0;
static BOOL g_bPhotoDzoomStop = TRUE;
static INT32 g_i32PhotoDzoomStep = 0;
static BOOL g_bPhotoOpened = FALSE;
static UIAppPhotoExeInfoType UIAppPhotoExeInfo;
static UIAppPhotoExeInfoType *localInfo = &UIAppPhotoExeInfo;
UIAppPhotoExeInfoType *pPhotoExeInfo = &UIAppPhotoExeInfo;

static USIZE g_photo_ImageRatioSize;

HD_DIM DZoom_5M_Table[DZOOM_MAX_STEP] = {
    {2592, 1944},  //1.0
    {2536, 1768},  //1.1x
    {2160, 1620},  //1.2x
    {1996, 1496},  //1.3x
    {1852, 1388},  //1.4x
    {1728, 1296}   //1.5x
};


HD_DIM DZoom_2M_Table[DZOOM_MAX_STEP] = {
	{1920,1080},
	{1856,1044},
	{1792,1008},
	{1732,976},
	{1672,940},
	{1616,908},
};



static USIZE IMAGERATIO_SIZE[IMAGERATIO_MAX_CNT] = {
	{9, 16}, //IMAGERATIO_9_16
	{2, 3}, //IMAGERATIO_2_3
	{3, 4}, //IMAGERATIO_3_4
	{1, 1}, //IMAGERATIO_1_1
	{4, 3}, //IMAGERATIO_4_3
	{3, 2}, //IMAGERATIO_3_2
	{16, 9}, //IMAGERATIO_16_9
};


PHOTO_STRM_CBR_INFO g_tStrmCbrInfo[SENSOR_MAX_NUM] =  {
    {
        1,
        PHOTO_STRM_ID_1,
        {1, 4, 200 * 1024, 30, 15, 26, 10, 50, 26, 10, 50, 0, 1, 8, 4}
      //{1, 4, 1024* 1024, 30, 15, 26, 10, 40, 26, 10, 40, 0, 1, 8, 4}//for mjpg
    },
    {
        1,
        PHOTO_STRM_ID_2,
        {1, 4, 200 * 1024, 30, 15, 26, 10, 50, 26, 10, 50, 0, 1, 8, 4}
    },
    {
        1,
        PHOTO_STRM_ID_3,
        {1, 4, 200 * 1024, 30, 15, 26, 10, 50, 26, 10, 50, 0, 1, 8, 4}
    },
    {
        1,
        PHOTO_STRM_ID_4,
        {1, 4, 200 * 1024, 30, 15, 26, 10, 50, 26, 10, 50, 0, 1, 8, 4}
    },
};


#if PHOTO_PREVIEW_SLICE_ENC_FUNC

#define PHOTO_SLICE_ENC_DBG_NVTMPP_USAGE		0		/* check common blk usage after opened */
#define PHOTO_SLICE_ENC_DBG_PERF			0
#define PHOTO_SLICE_ENC_DBG_DUMP			0
#define PHOTO_SLICE_ENC_DBG_PRIMARY_YUV			0
#define PHOTO_SLICE_ENC_DBG_SRC_SLICE_YUV		0
#define PHOTO_SLICE_ENC_DBG_DST_SLICE_YUV		0

#define PHOTO_SLICE_ENC_DBG_PRIMARY_JPG			0
#define PHOTO_SLICE_ENC_DBG_SCREENNAIL_JPG		0
#define PHOTO_SLICE_ENC_DBG_THUMBNAIL_JPG		0

#if PHOTO_SLICE_ENC_DBG_PRIMARY_JPG || PHOTO_SLICE_ENC_DBG_SCREENNAIL_JPG || PHOTO_SLICE_ENC_DBG_THUMBNAIL_JPG
	#define PHOTO_SLICE_ENC_DBG_JPG 1
#else
	#define PHOTO_SLICE_ENC_DBG_JPG 0
#endif

#if PHOTO_SLICE_ENC_DBG_DUMP
	#define PHOTO_SLICE_ENC_DUMP(fmtstr, args...)  DBG_DUMP(fmtstr, ##args) /* debug dump macro */
#else
	#define PHOTO_SLICE_ENC_DUMP(fmtstr, args...)
#endif

#include "FileSysTsk.h"
#include "vendor_videoenc.h"
#include "GxImageFile.h"
#include "kwrap/cmdsys.h"

typedef struct
{
	UINT32 width;
	UINT32 height;
	UINT32 slice_num;
	UINT32 slice_height; 		/* slice height except last one */
	UINT32 last_slice_height; 	/* last slice height */
} PhotoExe_SliceSize_Info;

typedef struct
{
	UINT32 va;
	UINT32 pa;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 blk_size;
	UINT32 used_size;

} PhotoExe_MEM_Info;

typedef struct
{
	HD_PATH_ID enc_path_id;
	PhotoExe_MEM_Info hd_enc_internal_buf_mem_info;
	PhotoExe_MEM_Info yuv_buf_mem_info;
	PhotoExe_MEM_Info bs_buf_mem_info;

#if HEIC_FUNC == ENABLE
	HD_PATH_ID enc_path_id_heic;
#endif

} PhotoExe_SliceEncode_Info;

typedef enum {
	SLICE_ENC_VOS_TICK_S,
	SLICE_ENC_VOS_TICK_E,
	SLICE_ENC_VOS_TICK_PRI_SCALE_S,
	SLICE_ENC_VOS_TICK_PRI_SCALE_E,
	SLICE_ENC_VOS_TICK_SCR_SCALE_S,
	SLICE_ENC_VOS_TICK_SCR_SCALE_E,
	SLICE_ENC_VOS_TICK_THUMB_SCALE_S,
	SLICE_ENC_VOS_TICK_THUMB_SCALE_E,
	SLICE_ENC_VOS_TICK_PRI_STAMP_S,
	SLICE_ENC_VOS_TICK_PRI_STAMP_E,
	SLICE_ENC_VOS_TICK_SCR_STAMP_S,
	SLICE_ENC_VOS_TICK_SCR_STAMP_E,
	SLICE_ENC_VOS_TICK_THUMB_STAMP_S,
	SLICE_ENC_VOS_TICK_THUMB_STAMP_E,
	SLICE_ENC_VOS_TICK_PRI_ENC_S,
	SLICE_ENC_VOS_TICK_PRI_ENC_E,
	SLICE_ENC_VOS_TICK_SCR_ENC_S,
	SLICE_ENC_VOS_TICK_SCR_ENC_E,
	SLICE_ENC_VOS_TICK_THUMB_ENC_S,
	SLICE_ENC_VOS_TICK_THUMB_ENC_E,
	SLICE_ENC_VOS_TICK_COMBINE_S,
	SLICE_ENC_VOS_TICK_COMBINE_E,
	SLICE_ENC_VOS_TICK_QVIEW_S,
	SLICE_ENC_VOS_TICK_QVIEW_E,
	SLICE_ENC_VOS_TICK_WR_S,
	SLICE_ENC_VOS_TICK_WR_E,
	SLICE_ENC_VOS_TICK_NUM
} SLICE_ENC_VOS_TICK;

/* static funcitons */
static PhotoExe_SliceEncode_Info* PhotoExe_Preview_SliceEncode_Get_Info(const PHOTO_ENC_JPG_TYPE type);
static INT32 PhotoExe_Preview_SliceEncode_Open(void);
static INT32 PhotoExe_Preview_SliceEncode_Close(void);
static INT32 PhotoExe_Preview_SliceEncode_Get_Dst_Slice_Info(PhotoExe_SliceSize_Info *info, UINT32 cap_size_w, UINT32 cap_size_h, UINT32 slice_num);
static INT32 PhotoExe_Preview_SliceEncode_Get_Curr_Dst_Slice_Info(PhotoExe_SliceSize_Info *info, const HD_VIDEO_FRAME src_frame);
static INT32 PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Info(PhotoExe_SliceSize_Info *info);
static UINT32 PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Buffer_Size(HD_VIDEO_PXLFMT pxl_fmt);
static HD_DIM PhotoExe_Preview_SliceEncode_Get_Encode_Max_Size(void);
static UINT32 PhotoExe_Preview_SliceEncode_Get_Encode_Max_Bitrate(HD_VIDEO_PXLFMT vproc_out_pxlfmt);
static INT32 PhotoExe_Preview_SliceEncode_Encode_Config_Path(HD_PATH_ID enc_path, HD_DIM max_dim, UINT32 bitrate);

#if HEIC_FUNC == ENABLE
static INT32 PhotoExe_Preview_SliceEncode_Encode_Config_Path_HEIC(HD_PATH_ID enc_path, HD_DIM max_dim, UINT32 bitrate);
#endif

static INT32 PhotoExe_Preview_SliceEncode_Get_Comm_Buffer(PhotoExe_MEM_Info* mem_info);
static INT32 PhotoExe_Preview_SliceEncode_Release_Comm_Buffer(PhotoExe_MEM_Info mem_info);
static INT32 PhotoExe_Preview_SliceEncode_Encode_Screennail(HD_VIDEO_FRAME* video_frame_in);
static INT32 PhotoExe_Preview_SliceEncode_Alloc_Buffer(PhotoExe_MEM_Info* info, char* name);
static INT32 PhotoExe_Preview_SliceEncode_Free_Buffer(PhotoExe_MEM_Info* info);
static INT32 PhotoExe_Preview_SliceEncode_Get_Enc_Buffer_Info(const HD_PATH_ID enc_path_id, PhotoExe_MEM_Info* info);
static INT32 PhotoExe_Preview_SliceEncode_Init_VF_GFX_Slice(
		VF_GFX_SCALE* vf_gfx_scale_param,
		const HD_VIDEO_FRAME* video_frame,
		const PhotoExe_MEM_Info dst_buffer_info,
		const PhotoExe_SliceSize_Info src_slice_info,
		const PhotoExe_SliceSize_Info dst_slice_info,
		const UINT8 slice_idx);

static INT32 PhotoExe_Preview_SliceEncode_DateStamp(
		const HD_VIDEO_FRAME* frame,
		IMG_CAP_DATASTAMP_EVENT event);
		
static INT32 PhotoExe_Preview_SliceEncode_Encode_Set_In(
		const HD_PATH_ID enc_path_id,
		const HD_VIDEO_PXLFMT vproc_out_pxlfmt,
		const HD_DIM dim);
		
static INT32 PhotoExe_Preview_SliceEncode_Encode_Set_Out(
		const HD_PATH_ID enc_path_id,
		const UINT32 quality);				

/* static variables */
PHOTO_CAP_CBMSG_FP  PhotoCapMsgCb = (PHOTO_CAP_CBMSG_FP)Photo_CaptureCB;

#if PHOTO_SLICE_ENC_DBG_PERF
	static VOS_TICK slice_enc_vos_tick[SLICE_ENC_VOS_TICK_NUM];
	#define SLICE_ENC_VOS_TICK_ELE(X) slice_enc_vos_tick[X]
	#define SLICE_ENC_VOS_TICK_TRIG(X) vos_perf_mark(&SLICE_ENC_VOS_TICK_ELE(X))
#else
	#define SLICE_ENC_VOS_TICK_ELE(X)
	#define SLICE_ENC_VOS_TICK_TRIG(X)
#endif

#endif

#endif

//---------------------UIAPP_PHOTO Local Functions-------------------------
//return Primary Size or Header+Primary+Screennail Size

void PhotoExe_InitNetHttp(void)
{
#if _TODO
	#if HTTP_LIVEVIEW_FUNC
	// config daemon parameter
	LVIEWNVT_DAEMON_INFO   DaemonInfo = {0};

	DaemonInfo.portNum = 8192;
	// set http live view server thread priority
	DaemonInfo.threadPriority = 6;
	// live view streaming frame rate
	DaemonInfo.frameRate = 30;
	// socket buffer size
	DaemonInfo.sockbufSize = 102400;
	// set time out 10*0.5 = 5 sec
	DaemonInfo.timeoutCnt  = 10;
	// Set type of service as Maximize Throughput
	DaemonInfo.tos = 0x0100;
	// if want to use https
	DaemonInfo.is_ssl = 1;
	DaemonInfo.is_push_mode = 1;
	ImageUnit_Begin(&ISF_NetHTTP, 0);
	ImageUnit_SetParam(ISF_CTRL, NETHTTP_PARAM_DAEMON, (UINT32)&DaemonInfo);
	ImageUnit_End();
	#endif
#endif
}
#if (PHOTO_MODE_CAP_YUV420_FUNC == ENABLE)
static BOOL	  bPhotoCapYUV420En = TRUE;
#else
static BOOL	  bPhotoCapYUV420En = FALSE;
#endif
void PhotoExe_SetCapYUV420En(BOOL Enable)
{
	bPhotoCapYUV420En = Enable;
}
BOOL PhotoExe_GetCapYUV420En(void)
{
	return bPhotoCapYUV420En;
}

#if(PHOTO_MODE==ENABLE)

static UINT32 g_exifbuf_pa = 0, g_exifbuf_va = 0;

static ER PhotoExe_InitExif(void)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 pa, blk_size;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	MEM_RANGE buf;

	EXIF_InstallID();

	//blk_size = 64 * 1024 * 1;
	blk_size = 64 * 1024 * 1 * SENSOR_CAPS_COUNT;

	if(g_exifbuf_va == 0)  {
		if ((hd_ret = hd_common_mem_alloc("exifbuf", &pa, (void **)&va, blk_size, ddr_id)) != HD_OK) {
			DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
			return E_NOMEM;
		}
		g_exifbuf_va = (UINT32)va;
		g_exifbuf_pa = (UINT32)pa;

		buf.addr = g_exifbuf_va;
		buf.size = 0x10000;
		EXIF_Init(EXIF_HDL_ID_1, &buf, ExifCB);
#if !defined(_SENSOR2_sen_off_) && (SENSOR_CAPS_COUNT >=2)
		buf.addr = (g_exifbuf_va + buf.size);
		buf.size = 0x10000;
		EXIF_Init(EXIF_HDL_ID_2, &buf, ExifCB);
#endif
#if !defined(_SENSOR3_sen_off_) && (SENSOR_CAPS_COUNT >=3)
		buf.addr = (g_exifbuf_va + buf.size);
		buf.size = 0x10000;
		EXIF_Init(EXIF_HDL_ID_3, &buf, ExifCB);
#endif
#if !defined(_SENSOR4_sen_off_) && (SENSOR_CAPS_COUNT >=4)
		buf.addr = (g_exifbuf_va + buf.size);
		buf.size = 0x10000;
		EXIF_Init(EXIF_HDL_ID_4, &buf, ExifCB);
#endif
		ret = E_OK;
	}
	return ret;
}

static ER PhotoExe_UninitExif(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if ((hd_ret = hd_common_mem_free(g_exifbuf_pa, (void *)g_exifbuf_va)) != HD_OK) {
		DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	g_exifbuf_pa = 0;
	g_exifbuf_va = 0;

	EXIF_UninstallID();

	return ret;
}
UINT32 _PhotoExe_GetImageAR(UINT32 aspect_ratio_x, UINT32 aspect_ratio_y)
{
	UINT32 ratio = 0;
	float fdiv;
	if (aspect_ratio_y == 0) {
		return 0;
	}

	fdiv = ((float)aspect_ratio_x) / aspect_ratio_y;

	if (fdiv == ((float)9) / 16) {
		ratio = 0x00090010;    //9:16
	} else if (fdiv == ((float)2) / 3) {
		ratio = 0x00020003;    //2:3
	} else if (fdiv == ((float)3) / 4) {
		ratio = 0x00030004;    //3:4
	} else if (fdiv == ((float)1) / 1) {
		ratio = 0x00010001;    //1:1
	} else if (fdiv == ((float)4) / 3) {
		ratio = 0x00040003;    //4:3
	} else if (fdiv == ((float)3) / 2) {
		ratio = 0x00030002;    //3:2
	} else if (fdiv == ((float)16) / 9) {
		ratio = 0x00100009;    //16:9
	} else {
		return 0;
	}

	return ratio;
}

static USIZE PhotoExe_RatioSizeConvert(USIZE *devSize, USIZE *devRatio, USIZE *Imgratio)
{
	USIZE  resultSize = *devSize;

	if ((!devRatio->w) || (!devRatio->h)) {
		DBG_ERR("devRatio w=%d, h=%d\r\n", devRatio->w, devRatio->h);
	} else if ((!Imgratio->w) || (!Imgratio->h)) {
		DBG_ERR("Imgratio w=%d, h=%d\r\n", Imgratio->w, Imgratio->h);
	} else {
		if (((float)Imgratio->w / Imgratio->h) >= ((float)devRatio->w / devRatio->h)) {
			resultSize.w = devSize->w;
			resultSize.h = ALIGN_ROUND_4(devSize->h * Imgratio->h / Imgratio->w * devRatio->w / devRatio->h);
		} else {
			resultSize.h = devSize->h;
			resultSize.w = ALIGN_ROUND_16(devSize->w * Imgratio->w / Imgratio->h * devRatio->h / devRatio->w);
		}
	}
	return resultSize;
}


static UINT32 PhotoExe_GetScreenNailSize(void)
{
	UINT32           uiImageSize, ScreenNailSize;
	UINT32           BitStreamSize;


	ScreenNailSize = CFG_SCREENNAIL_SIZE;
	uiImageSize = UI_GetData(FL_PHOTO_SIZE);
	if (uiImageSize < ScreenNailSize) {
		BitStreamSize = CFG_SCREENNAIL_W * CFG_SCREENNAIL_H / 2;
	} else {
		BitStreamSize = 0;
	}
	//#NT#2016/04/26#Lincy Lin -begin
	//#NT#Support sidebyside capture mode
	if (UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_SIDE) {
		BitStreamSize = BitStreamSize << 1;
	}
	//#NT#2016/04/26#Lincy Lin -end
	DBG_IND("[cap]ScreenNail BitStreamSize =%d K\r\n", BitStreamSize / 1024);
	return BitStreamSize;
}
void PhotoExe_SetScreenNailSize(UINT32 sensor_id)
{
	UINT32           uiImageSize, ScreenNailSize;
	USIZE            devRatio = {4, 3};
	USIZE            ImageRatioSize = {0}, BufferSize = {0};
	USIZE            ImgSize = {0};
	UINT32           ImageRatioIdx, BitStreamSize;
	PHOTO_CAP_INFO *pCapInfo= UIAppPhoto_get_CapConfig(sensor_id);
	if(pCapInfo){
		memcpy(pCapInfo,ImageApp_Photo_GetCapConfig(sensor_id), sizeof(PHOTO_CAP_INFO));
	}else{
		DBG_ERR("pCapInfo null\r\n");
		return;
	}
	ScreenNailSize = CFG_SCREENNAIL_SIZE;
	uiImageSize = UI_GetData(FL_PHOTO_SIZE);
	if (uiImageSize < ScreenNailSize) {
		//BufferSize.w = GetPhotoSizeWidth(ScreenNailSize);
		//BufferSize.h = GetPhotoSizeHeight(ScreenNailSize);
		BufferSize.w = CFG_SCREENNAIL_W;
		BufferSize.h = BufferSize.w * 3 / 4;

		ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
		ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];
		ImgSize = PhotoExe_RatioSizeConvert(&BufferSize, &devRatio, &ImageRatioSize);
		BitStreamSize = PhotoExe_GetScreenNailSize();

		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREEN_IMG, SEL_SCREEN_IMG_ON);
		pCapInfo->screen_img= SEL_SCREEN_IMG_ON;

		//#NT#2016/04/26#Lincy Lin -begin
		//#NT#Support sidebyside capture mode
		if (UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_SIDE) {
			ImgSize.w = ImgSize.w << 1;
		}
		//#NT#2016/04/26#Lincy Lin -end
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREEN_IMG_H_SIZE, ImgSize.w);
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREEN_IMG_V_SIZE, ImgSize.h);
		pCapInfo->screen_img_size.w= ImgSize.w;
		pCapInfo->screen_img_size.h= ImgSize.h;
		if(PhotoExe_GetCapYUV420En()){
			//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREEN_FMT, IPL_IMG_Y_PACK_UV420);
			pCapInfo->screen_fmt= HD_VIDEO_PXLFMT_YUV420;
		}else{
			//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREEN_FMT, IPL_IMG_Y_PACK_UV422);
			pCapInfo->screen_fmt= HD_VIDEO_PXLFMT_YUV422;
		}
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREENBUFSIZE, BitStreamSize);
		pCapInfo->screen_bufsize=BitStreamSize;
		DBG_IND("[cap]ScreenNail w=%d,h=%d, buffSize=%d k\r\n", ImgSize.w, ImgSize.h, BitStreamSize / 1024);

	} else {
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREEN_IMG, SEL_SCREEN_IMG_OFF);
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_SCREENBUFSIZE, 0);
		pCapInfo->screen_img= SEL_SCREEN_IMG_OFF;
		pCapInfo->screen_bufsize=0;
		DBG_IND("[cap]ScreenNail None\r\n");
	}
	ImageApp_Photo_Config(PHOTO_CFG_CAP_INFO, (UINT32)pCapInfo);
}
void PhotoExe_SetQuickViewSize(UINT32 sensor_id)
{
	USIZE            devRatio = {4, 3};
	USIZE            ImageRatioSize = {0}, BufferSize = {0};
	USIZE            ImgSize = {0};
	UINT32           ImageRatioIdx;
	ISIZE            DevSize = {0};
	PHOTO_CAP_INFO *pCapInfo= UIAppPhoto_get_CapConfig(sensor_id);
	if(pCapInfo){
		memcpy(pCapInfo,ImageApp_Photo_GetCapConfig(sensor_id), sizeof(PHOTO_CAP_INFO));
	}else{
		DBG_ERR("pCapInfo null\r\n");
		return;
	}

	DevSize = GxVideo_GetDeviceSize(DOUT1);
#if (_QUICKVIEW_SIZE_ == _QUICKVIEW_SIZE_VGA_)
	{
		// fix bug: w/h size will be wrong if no VGA/3M resolution!
		BufferSize.w = CFG_SCREENNAIL_W;
		BufferSize.h = CFG_SCREENNAIL_H;

		if (BufferSize.w > (UINT32)DevSize.w) {
			BufferSize.w = DevSize.w;
			BufferSize.h = BufferSize.w * devRatio.h / devRatio.w;
		}
	}
#else
	{
		BufferSize.w = DevSize.w;
		BufferSize.h = BufferSize.w * devRatio.h / devRatio.w;
	}
#endif

	ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
	ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];
	ImgSize = PhotoExe_RatioSizeConvert(&BufferSize, &devRatio, &ImageRatioSize);


#if (_QUICKVIEW_SIZE_ != _QUICKVIEW_SIZE_VGA_)
	//#NT#2013/07/19#Lincy Lin -begin
	//#NT#Fine tune TV NTSC/PAL quick view display quality
	{

		SIZECONVERT_INFO     CovtInfo = {0};
		USIZE                dev1Ratio;
		USIZE                tmpImgSize = {0};

		dev1Ratio = GxVideo_GetDeviceAspect(DOUT1);

		CovtInfo.uiSrcWidth  = ImgSize.w;
		CovtInfo.uiSrcHeight = ImgSize.h;
		CovtInfo.uiDstWidth  = DevSize.w;
		CovtInfo.uiDstHeight = DevSize.h;
		CovtInfo.uiDstWRatio = dev1Ratio.w;
		CovtInfo.uiDstHRatio = dev1Ratio.h;
		//#NT#2016/03/29#Lincy Lin -begin
		//#NT#Fix the photo quick view display error when image size is 16:9 with rotate panel
		//CovtInfo.alignType = SIZECONVERT_ALIGN_FLOOR_16;
		CovtInfo.alignType = SIZECONVERT_ALIGN_FLOOR_32;
		//#NT#2016/03/29#Lincy Lin -end
		DisplaySizeConvert(&CovtInfo);

		DBG_IND("[cap]CovtInfo Srcw=%d,h=%d, Dstw=%d ,h=%d, Ratiow=%d ,h=%d, OutW=%d,h=%d\r\n", CovtInfo.uiSrcWidth, CovtInfo.uiSrcHeight, CovtInfo.uiDstWidth, CovtInfo.uiDstHeight, CovtInfo.uiDstWRatio, CovtInfo.uiDstHRatio, CovtInfo.uiOutWidth, CovtInfo.uiOutHeight);
		DBG_IND("[cap]QuickView w=%d,h=%d, DevSize w=%d ,h=%d\r\n", ImgSize.w, ImgSize.h, DevSize.w, DevSize.h);
		//#NT#2016/03/29#Lincy Lin -begin
		//#NT#Fix the photo quick view display error when image size is 16:9 with rotate panel
		if (ImgSize.h != CovtInfo.uiOutHeight) {
			tmpImgSize.h = CovtInfo.uiOutHeight;
			// image size should not exceed video size
			if (tmpImgSize.h > ImgSize.h) {
				tmpImgSize.h = ImgSize.h;
			}
			tmpImgSize.w = tmpImgSize.h * ImgSize.w / ImgSize.h;
			//tmpImgSize.w = ALIGN_CEIL_16(tmpImgSize.w);
			tmpImgSize.w &= SIZECONVERT_ALIGN_FLOOR_32; //reference  DisplaySizeConvert()
			ImgSize = tmpImgSize;
		}
		//#NT#2016/03/29#Lincy Lin -end
	}
#endif
	//#NT#2013/07/19#Lincy Lin -end

	//#NT#2016/04/26#Lincy Lin -begin
	//#NT#Support sidebyside capture mode
	if (UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_SIDE) {
		ImgSize.w = ImgSize.w << 1;
	}
	//#NT#2016/04/26#Lincy Lin -end
	//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_QV_IMG_H_SIZE, ImgSize.w);
	//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_QV_IMG_V_SIZE, ImgSize.h);
	pCapInfo->qv_img_size.w=ImgSize.w;
	pCapInfo->qv_img_size.h=ImgSize.h;

	if(PhotoExe_GetCapYUV420En()){
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_QV_FMT, IPL_IMG_Y_PACK_UV420);
		pCapInfo->qv_img_fmt=HD_VIDEO_PXLFMT_YUV420;
	}else{
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_QV_FMT, IPL_IMG_Y_PACK_UV422);
		pCapInfo->qv_img_fmt=HD_VIDEO_PXLFMT_YUV422;
	}


	if (SysGetFlag(FL_QUICK_REVIEW) == QUICK_REVIEW_0SEC){
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_QV_IMG, SEL_QV_IMG_OFF);
		pCapInfo->qv_img=SEL_QV_IMG_OFF;
	} else {
		//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_QV_IMG, SEL_QV_IMG_ON);
		pCapInfo->qv_img=SEL_QV_IMG_ON;
	}


	ImageApp_Photo_Config(PHOTO_CFG_CAP_INFO, (UINT32)pCapInfo);

	DBG_IND("[cap]QuickView w=%d,h=%d, DevSize w=%d ,h=%d\r\n", ImgSize.w, ImgSize.h, DevSize.w, DevSize.h);
}


//return Primary Size or Header+Primary+Screennail Size
UINT32 PhotoExe_GetExpectSize_RhoBRCrtl(UINT32 ImgIdx, BOOL bPrimaryOnly)
{
	//#NT#2016/04/26#Lincy Lin -begin
	//#NT#Support sidebyside capture mode
	UINT32 BitstreamSize;

	if (UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_SIDE) {
		BitstreamSize = GetPhotoSizeWidth(ImgIdx) * 2 * GetPhotoSizeHeight(ImgIdx);
	} else {
		BitstreamSize = GetPhotoSizeWidth(ImgIdx) * GetPhotoSizeHeight(ImgIdx);
	}
	//#NT#2016/04/26#Lincy Lin -end
	switch (UI_GetData(FL_QUALITY)) {
	case QUALITY_FINE:
		BitstreamSize = (UINT32)(BitstreamSize * 30 / 100);
		break;

	case QUALITY_NORMAL:
		BitstreamSize = (UINT32)(BitstreamSize * 25 / 100);
		break;

	case QUALITY_ECONOMY:
	default:
		BitstreamSize = (UINT32)(BitstreamSize * 20 / 100);
	}

	if (!bPrimaryOnly) {
		BitstreamSize += CFG_JPG_HEADER_SIZE + PhotoExe_GetScreenNailSize();
	}

	return BitstreamSize;

}

UINT32 PhotoExe_GetFreePicNum(void)
{

	UINT64              uiFreeSpace;
	UINT32              uiMaxImageSize;
	UINT32              freeImgNum;
	UINT32              reserveSize = 0x80000; //  500KB
	UINT32              CaptureSize;
	UINT32              size;

#if (FILESIZE_ALIGN_FUNC)
	reserveSize += FS_ALIGN_RESERVED_SIZE;
#endif
	uiFreeSpace = FileSys_GetDiskInfo(FST_INFO_FREE_SPACE);

	DBG_IND("[cap]Free Space = %d KB\r\n", uiFreeSpace / 1024);

	size = UI_GetData(FL_PHOTO_SIZE);

	CaptureSize = PhotoExe_GetExpectSize_RhoBRCrtl(size, TRUE);
	DBG_IND("[cap]CaptureSize = %d K \r\n", CaptureSize / 1024);

	if (uiFreeSpace < reserveSize) {
		freeImgNum = 0;
	} else {
		freeImgNum = (uiFreeSpace - reserveSize) / CaptureSize;
	}

	DBG_IND("[cap]Free ImgNum = %d\r\n", freeImgNum);

	uiMaxImageSize = CaptureSize + reserveSize;
	//uiMaxImageSize = CaptureSize ;
	//uiMaxImageSize = CaptureSize+(CaptureSize*15/100); //+15%

	localInfo->uiMaxImageSize = uiMaxImageSize;
	//NA51055-1333
	localInfo->uiMaxImageSize = ALIGN_CEIL_4(localInfo->uiMaxImageSize);
	DBG_IND("[cap]uiMaxImageSize = %ld KB\r\n", uiMaxImageSize / 1024);

	//#NT#store in info
	localInfo->FreePicNum = freeImgNum;
	if (localInfo->FreePicNum > 0) {
		localInfo->isCardFull = FALSE;
	}
	return freeImgNum;
}


void PhotoExe_GetDispCord(URECT *dispCord)
{
	UINT32 ImageRatioIdx = 0;
	USIZE  ImageRatioSize = {0};
	URECT  DispCoord;
	ISIZE  dev1size;
	USIZE  dev1Ratio;
	USIZE  finalSize = {0};

	ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
	ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];

	//1.get current device size (current mode)
	dev1size = GxVideo_GetDeviceSize(DOUT1);
	//2.get current device aspect Ratio
	dev1Ratio = GxVideo_GetDeviceAspect(DOUT1);
	finalSize = PhotoExe_RatioSizeConvert((USIZE *)&dev1size, &dev1Ratio, &ImageRatioSize);
	DispCoord.w = finalSize.w;
	DispCoord.h = finalSize.h;
	if (finalSize.w == (UINT32)dev1size.w) {
		DispCoord.x = 0;
		DispCoord.y = (dev1size.h - finalSize.h) >> 1;
	} else {
		DispCoord.y = 0;
		DispCoord.x = (dev1size.w - finalSize.w) >> 1;

	}
	*dispCord = DispCoord;
}

#if defined(_UI_STYLE_LVGL_)

static void PhotoExe_CallBackUpdateInfo(UINT32 callBackEvent)
{
	DBG_IND("callBackEvent=0x%x\r\n", callBackEvent);

	localInfo->IsCallBack = TRUE;
	localInfo->CallBackEvent = callBackEvent;
	//NVTEVT_UPDATE_INFO

	LV_USER_EVENT_NVTMSG_DATA* data = gen_nvtmsg_data(NVTEVT_UPDATE_INFO, 1, localInfo->CallBackEvent);
	lv_event_send(lv_plugin_scr_act(), LV_USER_EVENT_NVTMSG, data);

	localInfo->IsCallBack = FALSE;
}

#else

static void PhotoExe_CallBackUpdateInfo(UINT32 callBackEvent)
{
	DBG_IND("callBackEvent=0x%x\r\n", callBackEvent);
	VControl *pCurrnetWnd;

	Ux_GetFocusedWindow(&pCurrnetWnd);
	localInfo->IsCallBack = TRUE;
	localInfo->CallBackEvent = callBackEvent;
	//NVTEVT_UPDATE_INFO
	Ux_SendEvent(pCurrnetWnd, NVTEVT_UPDATE_INFO, 1, localInfo->CallBackEvent);
	localInfo->IsCallBack = FALSE;
}

#endif

void AF_LOCK(BOOL bLock)
{
	// AF just support AF_ID_1
	if (System_GetEnableSensor() & SENSOR_1) {
#if _TODO
		ImageUnit_Begin(ISF_IPL(0), 0);
		//ImageUnit_SetParam(ISF_CTRL, IMAGEPIPE_PARAM_LOCKAF, (bLock));
		ImageUnit_End();
#endif
	}
}

BOOL AF_ISLOCK(void)
{
	// AF just support AF_ID_1
	if (System_GetEnableSensor() & SENSOR_1) {
#if _TODO
		//return ImageUnit_GetParam(ISF_IPL(0), ISF_CTRL, IMAGEPIPE_PARAM_LOCKAF);
#else
		return FALSE;
#endif
	} else {
		return FALSE;
	}
}

void AF_WAIT_IDLE(void)
{
	// AF just support AF_ID_1
	if (System_GetEnableSensor() & SENSOR_1) {
#if _TODO
		//if (ImageUnit_GetParam(ISF_IPL(0), ISF_CTRL, IMAGEPIPE_PARAM_WAITAF) != 0) {
		if(0) {
			DBG_ERR("WAITAF\r\n");
		}
#endif
	}
}

#if SHDR_FUNC
void PhotoExe_SHDR_SetUIInfo(UINT32 index, UINT32 value)
{
	return ;

	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
	{
        DBG_ERR("hd_videoproc_open() failed\r\n");
        return;
	}

	ret = hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
	if (ret != HD_OK) {
        DBG_ERR("hd_videoproc_get() HD_VIDEOPROC_PARAM_CTRL failed\r\n");
		return ;
	}

	if (HD_VIDEOPROC_FUNC_SHDR==index) {
	    if (UI_GetData(FL_SHDR) == SHDR_ON){
	        video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
	    }else{
	        video_cfg_param.ctrl_max.func &= ~(HD_VIDEOPROC_FUNC_SHDR);
	    }
	}

	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
	if (ret != HD_OK) {
        DBG_ERR("hd_videoproc_set() HD_VIDEOPROC_PARAM_CTRL failed\r\n");
		return ;
	}

	ret = hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);
	if (ret != HD_OK) {
        DBG_ERR("hd_videoproc_get() HD_VIDEOPROC_PARAM_CTRL failed\r\n");
		return ;
	}

    if (HD_VIDEOPROC_FUNC_SHDR==index) {
        if (UI_GetData(FL_SHDR) == SHDR_ON)
			video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
        else
            video_ctrl_param.func &= ~(HD_VIDEOPROC_FUNC_SHDR);
	}

	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_cfg_param);
	if (ret != HD_OK) {
        DBG_ERR("hd_videoproc_set() HD_VIDEOPROC_PARAM_CTRL failed\r\n");
		return ;
	}

}
#endif

void PhotoExe_IQ_SetUIInfo(UINT32 index, UINT32 value)
{
	HD_RESULT hd_ret;

	UINT32 isp_ver = 0;
	if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
	if ((hd_ret = vendor_isp_init()) != HD_OK) {
			DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
	}
	}

    switch (index){
       case IQT_ITEM_SHARPNESS_LV: {
        IQT_SHARPNESS_LV SHARPNESS_LV;

            SHARPNESS_LV.lv = value;
    	if (System_GetEnableSensor() & SENSOR_1){
            SHARPNESS_LV.id = IQ_ID_1;
            vendor_isp_set_iq(index, &SHARPNESS_LV);
    	}
		if (System_GetEnableSensor() & SENSOR_2){
            SHARPNESS_LV.id = IQ_ID_2;
            vendor_isp_set_iq(index, &SHARPNESS_LV);
    	}
       break;
       }
       case IQT_ITEM_SATURATION_LV: {
        IQT_SATURATION_LV Saturation_LV;

            Saturation_LV.lv = value;
    	if (System_GetEnableSensor() & SENSOR_1){
            Saturation_LV.id = IQ_ID_1;
            vendor_isp_set_iq(index, &Saturation_LV);
    	}
		if (System_GetEnableSensor() & SENSOR_2){
            Saturation_LV.id = IQ_ID_2;
            vendor_isp_set_iq(index, &Saturation_LV);
    	}
       break;
       }
       case IQT_ITEM_NR_LV: {
        IQT_NR_LV NR_LV;

            NR_LV.lv = value;
    	if (System_GetEnableSensor() & SENSOR_1){
            NR_LV.id = IQ_ID_1;
            vendor_isp_set_iq(index, &NR_LV);
    	}
		if (System_GetEnableSensor() & SENSOR_2){
            NR_LV.id = IQ_ID_2;
            vendor_isp_set_iq(index, &NR_LV);
    	}
       break;
       }
       case IQT_ITEM_WDR_PARAM: {
        IQT_WDR_PARAM WDR_info={0};
       if ((hd_ret = vendor_isp_get_iq(index, &WDR_info)) != HD_OK) {
		DBG_ERR("vendor_isp_get_iq failed(%d)\r\n", hd_ret);
       }
        WDR_info.wdr.enable = value;
    	if (System_GetEnableSensor() & SENSOR_1){
            WDR_info.id = IQ_ID_1;
            vendor_isp_set_iq(index, &WDR_info);
    	}
		if (System_GetEnableSensor() & SENSOR_2){
            WDR_info.id = IQ_ID_2;
            vendor_isp_set_iq(index, &WDR_info);
    	}
       break;
       }

       case IQT_ITEM_DEFOG_PARAM: {
        IQT_DEFOG_PARAM DeFog_info={0};

       if ((hd_ret = vendor_isp_get_iq(index, &DeFog_info)) != HD_OK) {
		DBG_ERR("vendor_isp_get_iq failed(%d)\r\n", hd_ret);
       }
        DeFog_info.defog.enable = value;
    	if (System_GetEnableSensor() & SENSOR_1){
            DeFog_info.id = IQ_ID_1;
            vendor_isp_set_iq(index, &DeFog_info);
    	}
		if (System_GetEnableSensor() & SENSOR_2){
            DeFog_info.id = IQ_ID_2;
            vendor_isp_set_iq(index, &DeFog_info);
    	}
       break;
       }
    }

	if (isp_ver == 0) {
	if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
			DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
		}
	}
}

void PhotoExe_AE_SetUIInfo(UINT32 index, UINT32 value)
{
	HD_RESULT hd_ret;

	UINT32 isp_ver = 0;
	if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
	if ((hd_ret = vendor_isp_init()) != HD_OK) {
			DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
		}
	}
    switch (index){
       case AET_ITEM_FREQUENCY: {
          AET_FREQUENCY_MODE FreMode ={0};
            FreMode.mode = value;
            if (System_GetEnableSensor() & SENSOR_1) {
                FreMode.id  = AE_ID_1;
                vendor_isp_set_ae(index, &FreMode);
            }
            if (System_GetEnableSensor() & SENSOR_2) {
                FreMode.id  = AE_ID_2;
                vendor_isp_set_ae(index, &FreMode);
            }
        break;
       }
       case AET_ITEM_EV: {
          AET_EV_OFFSET EvOffset ={0};

            EvOffset.offset = value;
            if (System_GetEnableSensor() & SENSOR_1) {
                EvOffset.id  = AE_ID_1;
                vendor_isp_set_ae(index, &EvOffset);
            }
            if (System_GetEnableSensor() & SENSOR_2) {
                EvOffset.id  = AE_ID_2;
                vendor_isp_set_ae(index, &EvOffset);
            }
        break;
       }
       case AET_ITEM_ISO: {
          AET_ISO_VALUE ISOValue ={0};

            ISOValue.value = value;
            if (System_GetEnableSensor() & SENSOR_1) {
                ISOValue.id  = AE_ID_1;
                vendor_isp_set_ae(index, &ISOValue);
            }
            if (System_GetEnableSensor() & SENSOR_2) {
                ISOValue.id  = AE_ID_2;
                vendor_isp_set_ae(index, &ISOValue);
            }
        break;
       }
       case AET_ITEM_METER: {
          AET_METER_MODE AEMeterMode ={0};

            AEMeterMode.mode = value;
            if (System_GetEnableSensor() & SENSOR_1) {
                AEMeterMode.id  = AE_ID_1;
                vendor_isp_set_ae(index, &AEMeterMode);
            }
            if (System_GetEnableSensor() & SENSOR_2) {
                AEMeterMode.id  = AE_ID_2;
                vendor_isp_set_ae(index, &AEMeterMode);
            }
        break;
       }
      default:
        printf("AE select:%d not match\r\n",index);
        break;
    }
	if (isp_ver == 0) {
	if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
			DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
		}
	}
}

void PhotoExe_AF_SetUIInfo(UINT32 index, UINT32 value)
{
#if 0
	HD_RESULT hd_ret;

	UINT32 isp_ver = 0;
	if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
	if ((hd_ret = vendor_isp_init()) != HD_OK) {
			DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
		}
	}
    switch (index){
        case AFT_ITEM_RLD_CONFIG: {
            AFT_CFG_INFO AF_Conf;

            strncpy(AF_Conf.path, "/etc/isp/isp_imx291_0.cfg", CFG_NAME_LENGTH);
            if (System_GetEnableSensor() & SENSOR_1) {
                AF_Conf.id  = AF_ID_1;
                vendor_isp_set_af(index, &AF_Conf);
            }
            if (System_GetEnableSensor() & SENSOR_2) {
                AF_Conf.id  = AF_ID_2;
                vendor_isp_set_af(index, &AF_Conf);
            }
            break;
        }
        case AFT_ITEM_RETRIGGER: {
            AFT_RETRIGGER AF_Retrigger;

            AF_Retrigger.retrigger = value;
            if (System_GetEnableSensor() & SENSOR_1) {
                AF_Retrigger.id  = AF_ID_1;
                vendor_isp_set_af(index, &AF_Retrigger);
            }
            if (System_GetEnableSensor() & SENSOR_2) {
                AF_Retrigger.id  = AF_ID_2;
                vendor_isp_set_af(index, &AF_Retrigger);
            }
        }
        default:
            printf("AF select:%d not match\r\n",index);
        break;
    }
	if (isp_ver == 0) {
	if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
			DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
		}
	}
#endif
}

//#NT#2020/04/15#Philex Lin
// set Fluorescent mode of White balance
static void PhotoExe_AWB_SetSCENE_CUSTOMER1(UINT32 id)
{
 AWBT_MWB_GAIN mwb = {0};

    mwb.id = id;
    vendor_isp_get_awb(AWBT_ITEM_MWB_GAIN, &mwb);
    mwb.mwb_gain.r_gain[5] = 398;
    mwb.mwb_gain.g_gain[5] = 256;
    mwb.mwb_gain.b_gain[5] = 520;
    vendor_isp_set_awb(AWBT_ITEM_MWB_GAIN, &mwb);
}
//#NT#2020/04/15#Philex end

void PhotoExe_AWB_SetUIInfo(UINT32 index, UINT32 value)
{
	HD_RESULT hd_ret;

	UINT32 isp_ver = 0;
	if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
	if ((hd_ret = vendor_isp_init()) != HD_OK) {
			DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
		}
	}
    switch (index){
       case  AWBT_ITEM_SCENE: {
         AWBT_SCENE_MODE AwbScene;

         AwbScene.mode = value;
         if (System_GetEnableSensor() & SENSOR_1) {
             AwbScene.id = AWB_ID_1;
             if (AwbScene.mode==AWB_SCENE_CUSTOMER1)
             {
                PhotoExe_AWB_SetSCENE_CUSTOMER1(AwbScene.id);
             }
             vendor_isp_set_awb(index, &AwbScene);
         }
         if (System_GetEnableSensor() & SENSOR_2) {
            AwbScene.id  = AWB_ID_2;
             if (AwbScene.mode==AWB_SCENE_CUSTOMER1)
             {
                PhotoExe_AWB_SetSCENE_CUSTOMER1(AwbScene.id);
             }
             vendor_isp_set_awb(index, &AwbScene);
         }
         break;
       }
       default:
            printf("AWB select:%d not match\r\n",index);
        break;
    }
	if (isp_ver == 0) {
	if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
			DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
		}
	}
}

void PhotoExe_Cap_SetUIInfo(UINT32 sensor_id, UINT32 index, UINT32 value)
{
#if 0
    ImageUnit_Begin(&ISF_Cap, 0);
	ImageUnit_SetParam(IPL_PATH(sensor_id) + ISF_IN1, index, value);
	ImageUnit_End();
#endif
}

void PhotoExe_RSC_SetSwitch(UINT32 index, UINT32 value)
{
#if RSC_FUNC
#if !defined(_IPL1_IPL_FAKE_)
	// RSC just support IPL 1
	if (System_GetEnableSensor() & SENSOR_1) {
		RSC_SetSwitch(IPL_PATH(0), index, value);
	}
	//DBG_IND("RSC index = %d , value %d\r\n", index, value);
#endif
#endif
}



//---------------------UIAPP_PHOTO MISC-------------------------

#if DZOOM_FUNC
static void PhotoExe_DZoomIn(void)
{
	g_bPhotoDzoomStop = FALSE;
	BKG_PostEvent(NVTEVT_BKW_DZOOM_IN);
}

static void PhotoExe_DZoomOut(void)
{
	g_bPhotoDzoomStop = FALSE;
	BKG_PostEvent(NVTEVT_BKW_DZOOM_OUT);
}

static void PhotoExe_DZoomStop(void)
{
	g_bPhotoDzoomStop = TRUE;
}
#endif

void PhotoExe_DZoomInBK(void)
{
	HD_PATH_ID vcap_id = 0;
	HD_PATH_ID vprc_id = 0;
	HD_VIDEOCAP_CROP vcap_crop_param= {0};
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_IN hd_vproc_in = {0};
	HD_VIDEOPROC_OUT vprc_out_param = {0};
	INT32 idx = 0;
	HD_DIM *pDzoomTbl=DZoom_2M_Table;

	if (g_i32PhotoDzoomStep >= (DZOOM_MAX_STEP-1)) {
		return;
	}
	if(System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE){
		vcap_id = ImageApp_MovieMulti_GetVcapPort(_CFG_REC_ID_1);
		vprc_id = ImageApp_MovieMulti_GetVprc3DNRPort(_CFG_REC_ID_1);
	}else{
 		ImageApp_Photo_Get_Hdal_Path(PHOTO_VID_IN_1, PHOTO_HDAL_VCAP_CAP_PATH, (UINT32 *)&vcap_id);
		ImageApp_Photo_Get_Hdal_Path(PHOTO_VID_IN_1, PHOTO_HDAL_VPRC_3DNR_REF_PATH, (UINT32 *)&vprc_id);
	}
	if ((vcap_id == 0) || (vprc_id == 0)) {
		DBG_ERR("vcap_id=%d, vprc_id=%d\r\n", vcap_id, vprc_id);
		return;
	}

	vcap_crop_param.mode = HD_CROP_ON;

	hd_vproc_in.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	hd_vproc_in.frc = HD_VIDEO_FRC_RATIO(1, 1);

	vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	vprc_out_param.frc =  HD_VIDEO_FRC_RATIO(1, 1);

	for (idx = g_i32PhotoDzoomStep + 1; idx < DZOOM_MAX_STEP; idx++) {
		if (g_bPhotoDzoomStop) {
		    break;
		}

		vcap_crop_param.win.rect.x = (pDzoomTbl[0].w - pDzoomTbl[idx].w)/2;
		vcap_crop_param.win.rect.y = (pDzoomTbl[0].h - pDzoomTbl[idx].h)/2;
		vcap_crop_param.win.rect.w = pDzoomTbl[idx].w;
		vcap_crop_param.win.rect.h = pDzoomTbl[idx].h;

		hd_vproc_in.dim.w = pDzoomTbl[idx].w;
		hd_vproc_in.dim.h = pDzoomTbl[idx].h;

		vprc_out_param.dim.w = pDzoomTbl[idx].w;
		vprc_out_param.dim.h = pDzoomTbl[idx].h;
		vprc_out_param.rect.x = 0;
		vprc_out_param.rect.y = 0;
		vprc_out_param.rect.w = pDzoomTbl[idx].w;
		vprc_out_param.rect.h = pDzoomTbl[idx].w;

		//DBG_DUMP("zoomin idx=%d, w=%d, h=%d\r\n", idx, pDzoomTbl[idx].w, pDzoomTbl[idx].h);

		if ((ret = hd_videocap_set(vcap_id, HD_VIDEOCAP_PARAM_OUT_CROP, &vcap_crop_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
		}

		if ((ret = hd_videocap_start(vcap_id)) != HD_OK) {
			DBG_ERR("set hd_videocap_start fail(%d)\r\n", ret);
		}

		if ((ret = hd_videoproc_set(vprc_id, HD_VIDEOPROC_PARAM_IN, &hd_vproc_in)) != HD_OK) {
			DBG_ERR("set hd_videoproc_set fail(%d)\r\n", ret);
		}

		if ((ret = hd_videoproc_set(vprc_id, HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
			DBG_ERR("set hd_videoproc_set fail(%d)\r\n", ret);
		}

		if ((ret = hd_videoproc_start(vprc_id)) != HD_OK) {
			DBG_ERR("set hd_videoproc_start fail(%d)\r\n", ret);
		}

		g_i32PhotoDzoomStep++;
		vos_util_delay_ms(90);
	}
}

void PhotoExe_DZoomOutBK(void)
{
	HD_PATH_ID vcap_id = 0;
	HD_PATH_ID vprc_id = 0;
	HD_VIDEOCAP_CROP vcap_crop_param = {0};
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_IN hd_vproc_in = {0};
	HD_VIDEOPROC_OUT vprc_out_param = {0};
	INT32 idx = 0;
	HD_DIM *pDzoomTbl=DZoom_2M_Table;

	if(System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE){
		vcap_id = ImageApp_MovieMulti_GetVcapPort(_CFG_REC_ID_1);
		vprc_id = ImageApp_MovieMulti_GetVprc3DNRPort(_CFG_REC_ID_1);
	}else{
 		ImageApp_Photo_Get_Hdal_Path(PHOTO_VID_IN_1, PHOTO_HDAL_VCAP_CAP_PATH, (UINT32 *)&vcap_id);
		ImageApp_Photo_Get_Hdal_Path(PHOTO_VID_IN_1, PHOTO_HDAL_VPRC_3DNR_REF_PATH, (UINT32 *)&vprc_id);
	}

	if ((vcap_id == 0) || (vprc_id == 0)) {
		DBG_ERR("vcap_id=%d, vprc_id=%d\r\n", vcap_id, vprc_id);
		return;
	}

	vcap_crop_param.mode = HD_CROP_ON;

	hd_vproc_in.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	hd_vproc_in.frc = HD_VIDEO_FRC_RATIO(1, 1);

	vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	vprc_out_param.frc =  HD_VIDEO_FRC_RATIO(1, 1);


	for (idx = g_i32PhotoDzoomStep - 1; idx >= 0; idx--) {
		if (g_bPhotoDzoomStop) {
		    break;
		}

		vcap_crop_param.win.rect.x = (pDzoomTbl[0].w - pDzoomTbl[idx].w)/2;
		vcap_crop_param.win.rect.y = (pDzoomTbl[0].h - pDzoomTbl[idx].h)/2;
		vcap_crop_param.win.rect.w = pDzoomTbl[idx].w;
		vcap_crop_param.win.rect.h = pDzoomTbl[idx].h;

		hd_vproc_in.dim.w = pDzoomTbl[idx].w;
		hd_vproc_in.dim.h = pDzoomTbl[idx].h;

		vprc_out_param.dim.w = pDzoomTbl[idx].w;
		vprc_out_param.dim.h = pDzoomTbl[idx].h;
		vprc_out_param.rect.x = 0;
		vprc_out_param.rect.y = 0;
		vprc_out_param.rect.w = pDzoomTbl[idx].w;
		vprc_out_param.rect.h = pDzoomTbl[idx].w;

		if ((ret = hd_videocap_set(vcap_id, HD_VIDEOCAP_PARAM_OUT_CROP, &vcap_crop_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
		}

		if ((ret = hd_videocap_start(vcap_id)) != HD_OK) {
			DBG_ERR("set hd_videocap_start fail(%d)\r\n", ret);
		}

		if ((ret = hd_videoproc_set(vprc_id, HD_VIDEOPROC_PARAM_IN, &hd_vproc_in)) != HD_OK) {
			DBG_ERR("set hd_videoproc_set fail(%d)\r\n", ret);
		}

		if ((ret = hd_videoproc_set(vprc_id, HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
			DBG_ERR("set hd_videoproc_set fail(%d)\r\n", ret);
		}

		if ((ret = hd_videoproc_start(vprc_id)) != HD_OK) {
			DBG_ERR("set hd_videoproc_start fail(%d)\r\n", ret);
		}

		if(g_i32PhotoDzoomStep > 1){
			g_i32PhotoDzoomStep--;
		}
		vos_util_delay_ms(90);
	}

}
//12M
//#define CAP_SIZE_W		4032//3840//1920
//#define CAP_SIZE_H		3024//2160//1080
#if (defined(_MODEL_529_CARDV_V70_))
#define VDO_SEN1_SIZE_W             3840
#define VDO_SEN1_SIZE_H             2160
#else
#define VDO_SEN1_SIZE_W            1920// 2560
#define VDO_SEN1_SIZE_H             1080//1440
#endif

#define VDO_SEN2_SIZE_W             1920
#define VDO_SEN2_SIZE_H             1080

#define VDO_WIFI_SIZE_W            864
#define VDO_WIFI_SIZE_H            480

#define VDO_DISP_SIZE_W             960
#define VDO_DISP_SIZE_H             240

#define DBGINFO_BUFSIZE()	(0x200)

#define CA_WIN_NUM_W        32
#define CA_WIN_NUM_H        32
#define LA_WIN_NUM_W        32
#define LA_WIN_NUM_H        32
#define VA_WIN_NUM_W        16
#define VA_WIN_NUM_H        16
#define YOUT_WIN_NUM_W      128
#define YOUT_WIN_NUM_H      128
#define ETH_8BIT_SEL        0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL         1 //0: full, 1: subsample 1/2

#define SEN_OUT_FMT         HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT         HD_VIDEO_PXLFMT_RAW12

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)

///////////////////////////////////////////////////////////////////////////////

static HD_COMMON_MEM_INIT_CONFIG g_photo_mem_cfg = {0};

void PhotoExe_CommPoolInit(void)
{
	UINT32 id;
	HD_VIDEO_PXLFMT pxl_fmt=HD_VIDEO_PXLFMT_YUV420;
	UINT32 MAX_CAP_SIZE_W=GetPhotoSizeWidth(PHOTO_MAX_CAP_SIZE);
	UINT32 MAX_CAP_SIZE_H=GetPhotoSizeHeight(PHOTO_MAX_CAP_SIZE);
	HD_VIDEO_PXLFMT vcap_fmt = HD_VIDEO_PXLFMT_RAW12;
	UINT32 vcap_buf_size = 0;
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	HD_RESULT hd_ret = HD_OK;
	USIZE DispDevSize  = {0};

	hd_ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (hd_ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
		DispDevSize.w = VDO_DISP_SIZE_W;
		DispDevSize.h = VDO_DISP_SIZE_H;
	} else {
		DispDevSize.w = p_video_out_syscaps->output_dim.w;
		DispDevSize.h = p_video_out_syscaps->output_dim.h;
	}

	if(PhotoExe_GetCapYUV420En()){
		pxl_fmt=HD_VIDEO_PXLFMT_YUV420;
	}else{
		pxl_fmt=HD_VIDEO_PXLFMT_YUV422;
	}

	for (id = 0; id < SENSOR_CAPS_COUNT; id++) {
		System_GetSensorInfo(id, SENSOR_CAPOUT_FMT, &vcap_fmt);
		if(id==0){
			vcap_buf_size = (HD_VIDEO_PXLFMT_CLASS(vcap_fmt) == HD_VIDEO_PXLFMT_CLASS_YUV)?  VDO_YUV_BUFSIZE(VDO_SEN1_SIZE_W, VDO_SEN1_SIZE_H, vcap_fmt) : VDO_RAW_BUFSIZE(VDO_SEN1_SIZE_W, VDO_SEN1_SIZE_H, vcap_fmt);
		}else{
			vcap_buf_size = (HD_VIDEO_PXLFMT_CLASS(vcap_fmt) == HD_VIDEO_PXLFMT_CLASS_YUV)?  VDO_YUV_BUFSIZE(VDO_SEN2_SIZE_W, VDO_SEN2_SIZE_H, vcap_fmt) : VDO_RAW_BUFSIZE(VDO_SEN2_SIZE_W, VDO_SEN2_SIZE_H, vcap_fmt);
		}
		DBG_IND("\r\nvcap_buf_size=0x%x ,vcap_fmt=0x%x, %d %d\r\n\r\n",vcap_buf_size,vcap_fmt, MAX_CAP_SIZE_W,MAX_CAP_SIZE_H);

		#if (PHOTO_DIRECT_FUNC == ENABLE)
		if (id == 0) {
			vcap_buf_size = 0;
		}
		#endif // (PHOTO_DIRECT_FUNC == ENABLE)

		g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
		g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() +
										vcap_buf_size +
										VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H) +
										VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		#if (SHDR_FUNC == ENABLE)
		g_photo_mem_cfg.pool_info[id].blk_cnt = 4;
		#else
		g_photo_mem_cfg.pool_info[id].blk_cnt = 2;
		#endif
		g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	}



	// config common pool (disp)
	// config common pool (yuv for liveview: vprc + vout)
	//id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(DispDevSize.w, DispDevSize.h, pxl_fmt);
	#if (SENSOR_CAPS_COUNT==1)
	g_photo_mem_cfg.pool_info[id].blk_cnt = 3;
	#else
	g_photo_mem_cfg.pool_info[id].blk_cnt = 8;
	#endif
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (wifi)
	// config common pool (yuv for liveview: vprc + venc)
	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_WIFI_SIZE_W, VDO_WIFI_SIZE_H, pxl_fmt);
	#if (SENSOR_CAPS_COUNT==1)
	g_photo_mem_cfg.pool_info[id].blk_cnt = 5;
	#else
	g_photo_mem_cfg.pool_info[id].blk_cnt = 8;
	#endif
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (main)
	// config common pool (yuv for capture: vprc + venc)
	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;

#if (PHOTO_PREVIEW_SLICE_ENC_FUNC == ENABLE)
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO_SEN1_SIZE_W, VDO_SEN1_SIZE_H, pxl_fmt);
#else
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, pxl_fmt);
#endif

	g_photo_mem_cfg.pool_info[id].blk_cnt = 1;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

#if (PHOTO_PREVIEW_SLICE_ENC_FUNC == ENABLE)

	(void)MAX_CAP_SIZE_W;
	(void)MAX_CAP_SIZE_H;

	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	PhotoExe_SliceSize_Info dst_slice_info;
	PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Info(&dst_slice_info);
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Buffer_Size(pxl_fmt);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 1;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(CFG_SCREENNAIL_W, CFG_SCREENNAIL_H, pxl_fmt);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 1;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(CFG_THUMBNAIL_W, CFG_THUMBNAIL_H, pxl_fmt);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 1;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

#endif

	// config common pool (yuv vprc)
	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SEN1_SIZE_W, VDO_SEN1_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 3;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;

#if (SENSOR_CAPS_COUNT >=2)
	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SEN2_SIZE_W, VDO_SEN2_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 2;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;
#endif
#if (SENSOR_CAPS_COUNT >=3)
	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SEN2_SIZE_W, VDO_SEN2_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 2;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;
#endif
#if (SENSOR_CAPS_COUNT >=4)
	id ++;
	g_photo_mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	g_photo_mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SEN2_SIZE_W, VDO_SEN2_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	g_photo_mem_cfg.pool_info[id].blk_cnt = 2;
	g_photo_mem_cfg.pool_info[id].ddr_id = DDR_ID0;
#endif

	ImageApp_Photo_Config(PHOTO_CFG_MEM_POOL_INFO, (UINT32)&g_photo_mem_cfg);
}
static UINT32 PhotoExe_InitSensorCount(void)
{
    //
	if (System_GetEnableSensor() == (SENSOR_1)) {
		localInfo->sensorCount = 1;
	} else if (System_GetEnableSensor() == (SENSOR_2)) {
		localInfo->sensorCount = 1;
	} else if (System_GetEnableSensor() == (SENSOR_1 | SENSOR_2)) {
		localInfo->sensorCount = 2;
	} else if (System_GetEnableSensor() == (SENSOR_1 | SENSOR_2 | SENSOR_3)) {
		localInfo->sensorCount = 3;
	} else if (System_GetEnableSensor() == (SENSOR_1 | SENSOR_2 | SENSOR_3 | SENSOR_4)) {
		localInfo->sensorCount = 4;
	}
//for hot plug
#if ((SENSOR_CAPS_COUNT >= 2) && (SENSOR_INSERT_MASK != 0))
	localInfo->sensorCount = SENSOR_CAPS_COUNT;
#endif

	//localInfo->sensorCount  = 1;
	DBG_IND("sensorCount = %d, FL_DUAL_CAM = %d\r\n", localInfo->sensorCount,UI_GetData(FL_DUAL_CAM));

	// load sensor display option
	if (localInfo->sensorCount < 2) {
		if (System_GetEnableSensor() == (SENSOR_1)) {
			localInfo->DualCam = DUALCAM_FRONT;
		} else {
			localInfo->DualCam = DUALCAM_BEHIND;
		}
	} else {
		//localInfo->DualCam = UI_GetData(FL_DUAL_CAM);
		localInfo->DualCam = DUALCAM_BOTH;
	}
#if (PIP_VIEW_FUNC == ENABLE)
	PipView_SetStyle(UI_GetData(FL_DUAL_CAM));
#endif
#if (PHOTO_IME_CROP == DISABLE)
	if(UI_GetData(FL_DUAL_CAM) >= DUALCAM_LR_16_9){
		UI_SetData(FL_DUAL_CAM, localInfo->DualCam);
	}
#endif

    DBG_IND("localInfo->DualCam = %d\r\n", localInfo->DualCam);
    return localInfo->DualCam;
}

void PhotoExe_ResetFileSN(void)
{
	g_i32PhotoFileSerialNum = 0;
}

UINT32 PhotoExe_GetFileSN(void)
{
	return (UINT32)g_i32PhotoFileSerialNum;
}

BOOL PhotoExe_CheckSNFull(void)
{
	return (g_i32PhotoFileSerialNum >= FILE_SN_MAX);
}
#if USE_FILEDB

static void PhotoExe_FileNamingCB(UINT32 id, char *pFileName)
{
	static struct tm   CurDateTime = {0};

	DBG_IND("id=%d\r\n", id);
	g_i32PhotoFileSerialNum++;
	if (g_i32PhotoFileSerialNum > FILE_SN_MAX) {
		g_i32PhotoFileSerialNum = FILE_SN_MAX;
		Ux_PostEvent(NVTEVT_CB_MOVIE_FULL, 0);
	}

	CurDateTime = hwclock_get_time(TIME_ID_CURRENT);
#if(SENSOR_CAPS_COUNT>=2)
	char    NH_endChar='A';
	NH_endChar+=id;

	snprintf(pFileName, NMC_TOTALFILEPATH_MAX_LEN, "%04d%02d%02d%02d%02d%02d_%06d%c",
		CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday,
	         CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec, g_i32PhotoFileSerialNum, NH_endChar);
	//snprintf(pFileName, NMC_TOTALFILEPATH_MAX_LEN, "%04d%02d%02d%02d%02d%02d%c",
        //    CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday,
	//	CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec, NH_endChar);
#else
	snprintf(pFileName, NMC_TOTALFILEPATH_MAX_LEN, "%04d%02d%02d%02d%02d%02d_%06d",
            CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday,
             CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec, g_i32PhotoFileSerialNum);
      //snprintf(pFileName, NMC_TOTALFILEPATH_MAX_LEN, "%04d%02d%02d%02d%02d%02d",
      //      CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday,
      //      CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);

#endif
}
#endif
static void PhotoExe_InitFileNaming(void)
{
#if USE_FILEDB
	UI_SetData(FL_IsUseFileDB, 1);
#else
	UI_SetData(FL_IsUseFileDB, 0);
#endif

#if (USE_FILEDB==ENABLE)
	MEM_RANGE               filedb_Pool = {0};

	//filedb_Pool.Addr = dma_getCacheAddr(OS_GetMempoolAddr(POOL_ID_FILEDB));
	filedb_Pool.addr = (int)mempool_filedb;
	filedb_Pool.size = POOL_SIZE_FILEDB;
	ImageApp_Photo_Config(PHOTO_CFG_FBD_POOL, (UINT32)&filedb_Pool);

	ImageApp_Photo_Config(PHOTO_CFG_ROOT_PATH, (UINT32)PHOTO_ROOT_PATH);

	// Folder Naming
	if (System_GetEnableSensor() & SENSOR_1) {
		gPhoto_Folder_Naming.cap_id = PHOTO_CAP_ID_1;
		gPhoto_Folder_Naming.ipl_id = 0;//IPL_PATH(0);
		ImageApp_Photo_Config(PHOTO_CFG_FOLDER_NAME, (UINT32)&gPhoto_Folder_Naming);
	}

	if ((System_GetEnableSensor() & SENSOR_2)
#if ((SENSOR_CAPS_COUNT >= 2) && (SENSOR_INSERT_MASK != 0))
	|| (System_GetEnableSensor() == SENSOR_1)
#endif
	) {
		gPhoto_Folder_Naming.cap_id = PHOTO_CAP_ID_2;
		gPhoto_Folder_Naming.ipl_id = 1;//IPL_PATH(1);
		ImageApp_Photo_Config(PHOTO_CFG_FOLDER_NAME, (UINT32)&gPhoto_Folder_Naming);
	}
	if ((System_GetEnableSensor() & SENSOR_3)
#if ((SENSOR_CAPS_COUNT >= 2) && (SENSOR_INSERT_MASK != 0))
	|| (System_GetEnableSensor() == SENSOR_1)
#endif
	){
		gPhoto_Folder_Naming.cap_id = PHOTO_CAP_ID_3;
		gPhoto_Folder_Naming.ipl_id = 2;//IPL_PATH(2);
		ImageApp_Photo_Config(PHOTO_CFG_FOLDER_NAME, (UINT32)&gPhoto_Folder_Naming);
	}
	if ((System_GetEnableSensor() & SENSOR_4)
#if ((SENSOR_CAPS_COUNT >= 2) && (SENSOR_INSERT_MASK != 0))
	|| (System_GetEnableSensor() == SENSOR_1)
#endif
	){
		gPhoto_Folder_Naming.cap_id = PHOTO_CAP_ID_4;
		gPhoto_Folder_Naming.ipl_id = 3;//IPL_PATH(2);
		ImageApp_Photo_Config(PHOTO_CFG_FOLDER_NAME, (UINT32)&gPhoto_Folder_Naming);
	}
	ImageApp_Photo_Config(PHOTO_CFG_FILE_NAME_CB, (UINT32)&PhotoExe_FileNamingCB);
#endif
}
static void PhotoExe_WifiStamp(void)
{
	if (UI_GetData(FL_MOVIE_DATEIMPRINT) == MOVIE_DATEIMPRINT_ON)
	{
		PHOTO_STRM_INFO *p_strm_info = (PHOTO_STRM_INFO*)ImageApp_Photo_GetConfig(PHOTO_CFG_STRM_INFO, 0);
		UINT32      uiStampAddr;
		UINT32      uiWidth ;// = p_strm_info->width;
		UINT32      uiHeight ;//= p_strm_info->height;
		STAMP_COLOR StampColorBg = {RGB_GET_Y( 16,  16,  16), RGB_GET_U( 16,  16,  16), RGB_GET_V( 16,  16,  16)}; // date stamp background color
		STAMP_COLOR StampColorFr = {RGB_GET_Y( 16,  16,  16), RGB_GET_U( 16,  16,  16), RGB_GET_V( 16,  16,  16)}; // date stamp frame color
		STAMP_COLOR StampColorFg = {RGB_GET_Y(224, 224, 192), RGB_GET_U(224, 224, 192), RGB_GET_V(224, 224, 192)}; // date stamp foreground color

		HD_PATH_ID  uiVEncOutPortId = HD_GET_OUT(p_strm_info->venc_p[0]) - 1;
	   	UINT32 ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
		PHOTO_STRM_INFO    *p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1);
		g_photo_ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];

		uiHeight =p_strm->height;
		uiWidth = ALIGN_CEIL_16(uiHeight* g_photo_ImageRatioSize.w/g_photo_ImageRatioSize.h);

		DBG_DUMP("PhotoExe_WifiStamp  ,VEncOutPortId=%d, Width=%d, Height=%d\r\n",uiVEncOutPortId, uiWidth, uiHeight);

		uiStampAddr=MovieStamp_GetBufAddr(uiVEncOutPortId, MovieStamp_CalcBufSize(uiWidth, uiHeight, NULL));//POOL_SIZE_DATEIMPRINT/4);
		MovieStamp_SetBuffer(uiVEncOutPortId, uiStampAddr , MovieStamp_CalcBufSize(uiWidth, uiHeight, NULL), 0, 0);//POOL_SIZE_DATEIMPRINT/4);
		MovieStamp_SetColor(uiVEncOutPortId, &StampColorBg, &StampColorFr, &StampColorFg);

		//DBG_IND("w = %d, h = %d\r\n",uiWidth,uiHeight);

		MovieStamp_Setup(
                    uiVEncOutPortId,
                    STAMP_ON |
                    STAMP_AUTO |
                    STAMP_DATE_TIME |
                    STAMP_BOTTOM_RIGHT |
                    STAMP_POS_NORMAL |
                    STAMP_YY_MM_DD,
                    uiWidth,
                    uiHeight,
                    NULL);
		// enable movie stamp
		MovieStamp_Enable();
	}
}

#if (SENSOR_CAPS_COUNT==1)
static void PhotoExe_WifiCB(void)
{
	HD_VIDEO_FRAME video_frame = {0};
	HD_RESULT ret;
	PHOTO_STRM_INFO     *p_strm = NULL;
	p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1);
	if(p_strm->enable != PHOTO_PORT_STATE_EN_RUN){
		vos_util_delay_ms(50);

		return;
	}
	//p_disp->enable = TRUE;
	if ((ret = ImageApp_Photo_WifiPullOut(p_strm, &video_frame, 500)) != HD_OK) {
		if (!(ret == HD_ERR_UNDERRUN || (ret ==HD_ERR_TIMEDOUT && localInfo->isStartCapture))){
			DBG_ERR("wifi pull_out error(%d)\r\n", ret);
		}
		return;
	}
	if ((ret = ImageApp_Photo_WifiPushIn(p_strm, &video_frame, 0)) != HD_OK) {
		if (ret != HD_ERR_OVERRUN){
			DBG_ERR("wifi push_in error(%d)\r\n", ret);
		}
	}
	if ((ret = ImageApp_Photo_WifiReleaseOut(p_strm, &video_frame)) != HD_OK) {
		DBG_ERR("wifi release_out error(%d)\r\n", ret);
	}
}
#else
static void PhotoExe_WifiCB(void)
{
	HD_VIDEO_FRAME src_img[SENSOR_CAPS_COUNT] = {0};
	HD_VIDEO_FRAME dst_img;

	//HD_VIDEO_FRAME video_frame = {0};
	HD_RESULT ret;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 i, mask, sensor_mask, sensor_count;
	UINT32 blk_size, pa;
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	APPDISP_VIEW_DRAW	pip_draw = {0};
	PHOTO_STRM_INFO     *p_strm = NULL;

	if (System_GetState(SYS_STATE_CURRSUBMODE) != SYS_SUBMODE_WIFI) {
		//return;
	}

	mask = 1;
	sensor_count = 0;
	sensor_mask = System_GetEnableSensor();
	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		if (sensor_mask & mask) {
			p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1 +i);
			if ((ret = ImageApp_Photo_WifiPullOut(p_strm, &src_img[i], 500)) == HD_OK) {
				pip_draw.p_src_img[i] = &src_img[i];
				sensor_count++;
			}
		}
		mask <<= 1;
	}


	if (sensor_count == 0) { // cannot pull data!
		return;
	}


	PHOTO_STRM_INFO *p_strm_info = (PHOTO_STRM_INFO*)ImageApp_Photo_GetConfig(PHOTO_CFG_STRM_INFO, 0);
	ISIZE  DevSize = {p_strm_info->width, p_strm_info->height};
	//DBG_DUMP("wifi DevSize=%d, %d\r\n", DevSize.w,DevSize.h);

	//if(pip_draw.p_src_img[1])
	//DBG_DUMP("DevSize=%d, %d %d,%d,%d\r\n", DevSize.w,DevSize.h,pip_draw.p_src_img[1]->pw[0],pip_draw.p_src_img[1]->loff[0],pip_draw.p_src_img[1]->ph[0]);
#if 0
	if ((UI_GetData(FL_DUAL_CAM) == DUALCAM_FRONT) || (sensor_count == 1)) {
		if ((ret = ImageApp_Photo_WifiPushIn( UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1), &src_img[0], 500)) != HD_OK) {
			DBG_ERR("wifi push_in error(%d)\r\n", ret);
		}
		goto video_frame_release;
	} else if (UI_GetData(FL_DUAL_CAM) == DUALCAM_BEHIND) {
		#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
		if(pip_draw.p_src_img[1]  && ((UINT32)DevSize.w == pip_draw.p_src_img[1]->pw[0])){
			if ((ret = ImageApp_Photo_WifiPushIn( UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1), &src_img[1], 500)) != HD_OK) {
				DBG_ERR("wifi push_in error(%d)\r\n", ret);
			}
			goto video_frame_release;
		}
		#endif
	}
#endif
	blk_size = VDO_YUV_BUFSIZE(DevSize.w, DevSize.h, HD_VIDEO_PXLFMT_YUV420);
	if ((blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, DDR_ID0)) == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("hd_common_mem_get_block fail(%d)\r\n", blk);
		goto video_frame_release;
	}

	if ((pa = hd_common_mem_blk2pa(blk)) == 0) {
		DBG_ERR("hd_common_mem_blk2pa fail\r\n");
		if ((ret = hd_common_mem_release_block(blk)) != HD_OK) {
			DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", ret);
		}
		goto video_frame_release;
	}

	// set dest buffer
	pip_draw.p_dst_img = &dst_img;
	addr[0] = pa;
	loff[0] = ALIGN_CEIL_4(DevSize.w);
	addr[1] = pa + loff[0] * DevSize.h;
	loff[1] = ALIGN_CEIL_4(DevSize.w);
	if ((ret = vf_init_ex(pip_draw.p_dst_img, DevSize.w, DevSize.h, HD_VIDEO_PXLFMT_YUV420, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", ret);
	}

	PipView_OnDraw(&pip_draw);

	pip_draw.p_dst_img->count = 0;
	pip_draw.p_dst_img->timestamp = hd_gettime_us();
	pip_draw.p_dst_img->blk = blk;
	if ((ret = ImageApp_Photo_WifiPushIn( UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1), pip_draw.p_dst_img, 500)) != HD_OK) {
		DBG_ERR("wifi push_in error(%d)\r\n", ret);
	}

	if ((ret = hd_common_mem_release_block(blk)) != HD_OK) {
		DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", ret);
	}

video_frame_release:
	for (i = 0; i < (SENSOR_CAPS_COUNT); i++) {
		if (pip_draw.p_src_img[i]) {
			p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1 +i);
			if ((ret = ImageApp_Photo_WifiReleaseOut(p_strm, &src_img[i])) != HD_OK) {
				DBG_ERR("wifi release_out error(%d)\r\n", ret);
			}
		}
	}
}
#endif

static void PhotoExe_InitNetworkStream(UINT32 dualCam, USIZE *pImageRatioSize)
{
	UINT32 i;
	PHOTO_STRM_INFO    *p_strm = NULL;
	//APPDISP_VIEW_INFO   appdisp_info ={0};


	// sensor 1 display
	if (dualCam == DUALCAM_FRONT) {
		p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1);
		ImageApp_Photo_Config(PHOTO_CFG_STRM_INFO, (UINT32)p_strm);
		p_strm->width_ratio  =  pImageRatioSize->w;
		p_strm->height_ratio =  pImageRatioSize->h;
		p_strm->width = ALIGN_CEIL_16(p_strm->height* pImageRatioSize->w/pImageRatioSize->h);
		//p_strm->height = PHOTO_STRM_HEIGHT;
		ImageApp_Photo_Config(PHOTO_CFG_STRM_INFO, (UINT32)p_strm);
		if (p_strm->strm_type== PHOTO_STRM_TYPE_HTTP) {
			PhotoExe_InitNetHttp();
		}
	}
	// sensor 2 display
	else if (dualCam == DUALCAM_BEHIND) {
		p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_2);
		ImageApp_Photo_Config(PHOTO_CFG_STRM_INFO, (UINT32)p_strm);
		p_strm->width_ratio  =  pImageRatioSize->w;
		p_strm->height_ratio =  pImageRatioSize->h;
		p_strm->width = ALIGN_CEIL_16(p_strm->height* pImageRatioSize->w/pImageRatioSize->h);
		//p_strm->height = PHOTO_STRM_HEIGHT;
		ImageApp_Photo_Config(PHOTO_CFG_STRM_INFO, (UINT32)p_strm);
		if (p_strm->strm_type == PHOTO_STRM_TYPE_HTTP) {
			PhotoExe_InitNetHttp();
		}
	}
#if 1
	// PIP view
	else {
		//if (AppDispView_GetInfo(&appdisp_info) != E_OK)
		//	return;
		for (i=0;i<localInfo->sensorCount;i++) {
			p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1+i);
			if (p_strm == NULL)
				return;
			p_strm->enable          = PHOTO_PORT_STATE_EN_RUN;
			#if ((SENSOR_CAPS_COUNT == 2) && (SENSOR_INSERT_MASK != 0))
			if (System_GetEnableSensor() == SENSOR_1 && i==UIAPP_PHOTO_STRM_ID_2){
				p_strm->enable          = PHOTO_PORT_STATE_EN;
			}
			#endif

			#if (SBS_VIEW_FUNC == ENABLE)
            p_strm->multi_view_type = PHOTO_MULTI_VIEW_SBS_LR;
			#else
			p_strm->multi_view_type = PHOTO_MULTI_VIEW_PIP;
			#endif
			p_strm->width_ratio  =  pImageRatioSize->w;
			p_strm->height_ratio =  pImageRatioSize->h;
			p_strm->width = ALIGN_CEIL_16(p_strm->height* pImageRatioSize->w/pImageRatioSize->h);
			#if (SBS_VIEW_FUNC == ENABLE)
            p_strm->width <<= 1;
			#endif
		    //p_strm->height = PHOTO_STRM_HEIGHT;
			ImageApp_Photo_Config(PHOTO_CFG_STRM_INFO, (UINT32)p_strm);
		}
        	//ImageApp_Photo_WiFiConfig(PHOTO_CFG_WIFI_REG_CB, (UINT32)PhotoExe_WifiCB);
		if (p_strm->strm_type == PHOTO_STRM_TYPE_HTTP) {
			PhotoExe_InitNetHttp();
		}
	}
#endif
#if 1
	for (i = 0; i < SENSOR_CAPS_COUNT; i++)	{
        	ImageApp_Photo_WiFiConfig(PHOTO_CFG_CBR_INFO, (UINT32)&(g_tStrmCbrInfo[i]));
	}
#endif
}
#if (PHOTO_IME_CROP == ENABLE)
static PHOTO_IME_CROP_INFO IMECropInfo[SENSOR_CAPS_COUNT]={0};
#endif
void PhotoExe_SetIMECrop(UINT32 id)
{
#if (PHOTO_IME_CROP == ENABLE)
	UINT32 i;
	ISIZE  disp_size = GxVideo_GetDeviceSize(DOUT1);
	USIZE  disp_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);
	UINT32 pip_style = UI_GetData(FL_DUAL_CAM);
	UINT32           ImageRatioIdx = 0;
	USIZE            ImageRatioSize = {0};

	i = id;

	ImageRatioIdx=GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
	if (ImageRatioIdx >= IMAGERATIO_MAX_CNT) {
		DBG_ERR("ImageRatioIdx =%d\r\n", ImageRatioIdx);
		return ;
	}
	ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];
	DBG_DUMP("[%d]SetIMECrop ImageRatioIdx=%d, ImageRatioSize.w =%d, %d\r\n",id,ImageRatioIdx, ImageRatioSize.w,ImageRatioSize.h);

	if ((ImageRatioSize.w * disp_aspect_ratio.h) > (disp_aspect_ratio.w * ImageRatioSize.h)) {
		IMECropInfo[i].IMESize.h = disp_size.h;
		IMECropInfo[i].IMESize.w = ALIGN_CEIL_16((IMECropInfo[i].IMESize.h * ImageRatioSize.w) / ImageRatioSize.h);
		IMECropInfo[i].IMEWin.w = disp_size.w;
		IMECropInfo[i].IMEWin.h = disp_size.h;
		IMECropInfo[i].IMEWin.x = ALIGN_CEIL_4((IMECropInfo[i].IMESize.w - IMECropInfo[i].IMEWin.w) / 2);
		IMECropInfo[i].IMEWin.y = 0;
	} else {
		if (pip_style == DUALCAM_LR_FULL) {

			if ((ImageRatioSize.w * disp_aspect_ratio.h) > (disp_aspect_ratio.w * (ImageRatioSize.h*2))) {
				IMECropInfo[i].IMESize.h = disp_size.h;
				IMECropInfo[i].IMESize.w = ALIGN_CEIL_16((IMECropInfo[i].IMESize.h * ImageRatioSize.w) / (ImageRatioSize.h*2));
				IMECropInfo[i].IMEWin.w = disp_size.w / 2;
				IMECropInfo[i].IMEWin.h = disp_size.h;
				IMECropInfo[i].IMEWin.x = ALIGN_CEIL_4((IMECropInfo[i].IMESize.w - IMECropInfo[i].IMEWin.w) / 2);
				IMECropInfo[i].IMEWin.y = 0;
			} else {
				IMECropInfo[i].IMESize.w = ALIGN_CEIL_16(disp_size.w / 2);
				IMECropInfo[i].IMESize.h = ALIGN_CEIL_4((IMECropInfo[i].IMESize.w * (ImageRatioSize.h*2)) / ImageRatioSize.w);
				IMECropInfo[i].IMEWin.w = disp_size.w / 2;
				IMECropInfo[i].IMEWin.h = disp_size.h;
				IMECropInfo[i].IMEWin.x = 0;
				IMECropInfo[i].IMEWin.y = ALIGN_CEIL_4((IMECropInfo[i].IMESize.h - IMECropInfo[i].IMEWin.h) / 2);
			}
		} else if ((pip_style == DUALCAM_FRONT) || (pip_style == DUALCAM_BEHIND)) {
			IMECropInfo[i].IMESize.h = disp_size.h;
			IMECropInfo[i].IMESize.w = ALIGN_CEIL_16((IMECropInfo[i].IMESize.h * ImageRatioSize.w) /ImageRatioSize.h);
			IMECropInfo[i].IMEWin.w = IMECropInfo[i].IMESize.w;
			IMECropInfo[i].IMEWin.h = IMECropInfo[i].IMESize.h;
			IMECropInfo[i].IMEWin.x = 0;
			IMECropInfo[i].IMEWin.y = 0;
		} else {
			IMECropInfo[i].IMESize.w = ALIGN_CEIL_16(disp_size.w);
			IMECropInfo[i].IMESize.h = ALIGN_CEIL_4((IMECropInfo[i].IMESize.w * ImageRatioSize.h) / ImageRatioSize.w);
			IMECropInfo[i].IMEWin.w = disp_size.w;
			IMECropInfo[i].IMEWin.h = disp_size.h;
			IMECropInfo[i].IMEWin.x = 0;
			IMECropInfo[i].IMEWin.y = ALIGN_CEIL_4((IMECropInfo[i].IMESize.h - IMECropInfo[i].IMEWin.h) / 2);
		}
	}

	//DBG_DUMP("IMECropInfo[%d]: size w %d, h %d, Win x %d, y %d, w %d, h %d\r\n",
	//	i, IMECropInfo[i].IMESize.w, IMECropInfo[i].IMESize.h, IMECropInfo[i].IMEWin.x, IMECropInfo[i].IMEWin.y, IMECropInfo[i].IMEWin.w, IMECropInfo[i].IMEWin.h);

	if (((IMECropInfo[i].IMEWin.x + IMECropInfo[i].IMEWin.w) > IMECropInfo[i].IMESize.w) || ((IMECropInfo[i].IMEWin.y + IMECropInfo[i].IMEWin.h) > IMECropInfo[i].IMESize.h)) {
		DBG_ERR("Window out of range! %d+%d>%d or %d+%d>%d\r\n", IMECropInfo[i].IMEWin.x, IMECropInfo[i].IMEWin.w, IMECropInfo[i].IMESize.w, IMECropInfo[i].IMEWin.y, IMECropInfo[i].IMEWin.h, IMECropInfo[i].IMESize.h);
	}
	IMECropInfo[i].enable=1;
	IMECropInfo[i].vid=i;
	ImageApp_Photo_Config(PHOTO_CFG_DISP_IME_CROP,(UINT32)&IMECropInfo[i]);
#endif
}

#if (SENSOR_CAPS_COUNT==1)
static void PhotoExe_DispCB(void)
{
	HD_VIDEO_FRAME video_frame = {0};
	HD_RESULT ret;
	PHOTO_DISP_INFO     *p_disp = NULL;
	p_disp = UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1);
	//p_disp->enable = TRUE;
	if(0 != System_GetState(SYS_STATE_SLEEPLEVEL)){
		//Enter the sleep Mode and skip the callback
		return;
	}
	if ((ret = ImageApp_Photo_DispPullOut(p_disp, &video_frame, 500)) != HD_OK) {
		if (ret != HD_ERR_UNDERRUN){
			DBG_ERR("disp pull_out error(%d)\r\n", ret);
		}
		return;
	}
	if ((ret = ImageApp_Photo_DispPushIn(p_disp, &video_frame, 0)) != HD_OK) {
		DBG_ERR("disp push_in error(%d)\r\n", ret);
	}
	if ((ret = ImageApp_Photo_DispReleaseOut(p_disp, &video_frame)) != HD_OK) {
		DBG_ERR("disp release_out error(%d)\r\n", ret);
	}
}
#else
static void PhotoExe_DispCB(void)
{
	HD_VIDEO_FRAME src_img[SENSOR_CAPS_COUNT] = {0};
	HD_VIDEO_FRAME dst_img;
	BOOL  bPullData[SENSOR_CAPS_COUNT] = {0};
	HD_RESULT ret;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 i, mask, sensor_mask, sensor_count;
	UINT32 blk_size=0, pa;
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	APPDISP_VIEW_DRAW	pip_draw = {0};
	//ISIZE DevSize =  GxVideo_GetDeviceSize(DOUT1);
	ISIZE disp_buf = {0};
	//UINT32 sen_caps_count = SENSOR_CAPS_COUNT;
	PHOTO_DISP_INFO     *p_disp = NULL;
	if(0 != System_GetState(SYS_STATE_SLEEPLEVEL)){
		//Enter the sleep Mode and skip the callback
		return;
	}
	mask = 1;
	sensor_count = 0;
	sensor_mask = System_GetEnableSensor();
	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		if (sensor_mask & mask) {
			p_disp = UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1+i);
			if ((ret = ImageApp_Photo_DispPullOut(p_disp, &src_img[i], 500)) == HD_OK) {
				pip_draw.p_src_img[i] = &src_img[i];
				bPullData[i] = TRUE;
				sensor_count++;
			}else{
				DBG_ERR("disp pull out error(%d)(%d)\r\n",i, ret);
			}
		}
		mask <<= 1;
	}


	if (sensor_count == 0) { // cannot pull data!
		return;
	}

	#if 0
	if ((UI_GetData(FL_DUAL_CAM) == DUALCAM_FRONT) || (sensor_count == 1)) {
		if ((ret = ImageApp_Photo_DispPushIn(UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1), &src_img[0], 500)) != HD_OK) {
			DBG_ERR("disp push_in error(%d)\r\n", ret);
		}
		goto video_frame_release;
	} else if (UI_GetData(FL_DUAL_CAM) == DUALCAM_BEHIND) {
		#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
		if(pip_draw.p_src_img[1]  && ((UINT32)DevSize.w==pip_draw.p_src_img[1]->pw[0])){
			if ((ret = ImageApp_Photo_DispPushIn(UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1), &src_img[1], 500)) != HD_OK) {
				DBG_ERR("disp push_in error(%d)\r\n", ret);
			}
			goto video_frame_release;
		}
		#endif
	}
	#endif
	//blk_size = VDO_YUV_BUFSIZE(src_img[0].dim.w, src_img[0].dim.h, src_img[0].pxlfmt); // temporarily
	//blk_size = VDO_YUV_BUFSIZE(DevSize.w, DevSize.h, HD_VIDEO_PXLFMT_YUV420);
	if(src_img[0].dim.w){
		disp_buf.w = src_img[0].dim.w;
		disp_buf.h = src_img[0].dim.h;
	}else if(src_img[1].dim.w){
		disp_buf.w = src_img[1].dim.w;
		disp_buf.h = src_img[1].dim.h;
	}
#if (SENSOR_CAPS_COUNT>=3)
	else{
		disp_buf.w = src_img[2].dim.w;
		disp_buf.h = src_img[2].dim.h;
	}
#endif
	if(SysVideo_GetDirbyID(DOUT1) != HD_VIDEO_DIR_NONE){
		blk_size = VDO_YUV_BUFSIZE(ALIGN_CEIL_16(disp_buf.w), ALIGN_CEIL_16(disp_buf.h), HD_VIDEO_PXLFMT_YUV420);
	}else{
		blk_size = VDO_YUV_BUFSIZE(disp_buf.w, disp_buf.h, HD_VIDEO_PXLFMT_YUV420);
	}
	//DBG_DUMP("DevSize blk_size=0x%x, w=%d, h=%d, %d, %d\r\n",blk_size,DevSize.w,DevSize.h,pip_draw.p_src_img[0]->dim.w,pip_draw.p_src_img[0]->dim.h);
	//DBG_DUMP("disp_buf.w=%d, %d, blk_size=%d\r\n",disp_buf.w ,disp_buf.h, blk_size);

	if ((blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, DDR_ID0)) == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("hd_common_mem_get_block fail(%d)\r\n", blk);
		goto video_frame_release;
	}

	if ((pa = hd_common_mem_blk2pa(blk)) == 0) {
		DBG_ERR("hd_common_mem_blk2pa fail\r\n");
		if ((ret = hd_common_mem_release_block(blk)) != HD_OK) {
			DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", ret);
		}
		goto video_frame_release;
	}

	// set dest buffer
	pip_draw.p_dst_img = &dst_img;
	addr[0] = pa;
	#if 0
	loff[0] = ALIGN_CEIL_4(src_img[0].dim.w);
	addr[1] = pa + loff[0] * src_img[0].dim.h;
	loff[1] = ALIGN_CEIL_4(src_img[0].dim.w);
	if ((ret = vf_init_ex(pip_draw.p_dst_img, src_img[0].dim.w, src_img[0].dim.h, HD_VIDEO_PXLFMT_YUV420, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", ret);
	}
	#else
	loff[0] = ALIGN_CEIL_4(disp_buf.w);
	addr[1] = pa + loff[0] * disp_buf.h;
	loff[1] = ALIGN_CEIL_4(disp_buf.w);
	if ((ret = vf_init_ex(pip_draw.p_dst_img, disp_buf.w, disp_buf.h, HD_VIDEO_PXLFMT_YUV420, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", ret);
	}
	#endif
	//DBG_DUMP("sensor_count=%d\r\n", sensor_count);

	PipView_OnDraw(&pip_draw);

	pip_draw.p_dst_img->count = 0;
	pip_draw.p_dst_img->timestamp = hd_gettime_us();
	pip_draw.p_dst_img->blk = blk;
	if ((ret = ImageApp_Photo_DispPushIn(UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1), pip_draw.p_dst_img, 500)) != HD_OK) {
		DBG_ERR("disp push_in error(%d)\r\n", ret);
	}

	if ((ret = hd_common_mem_release_block(blk)) != HD_OK) {
		DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", ret);
	}

video_frame_release:
	for (i = 0; i < (SENSOR_CAPS_COUNT); i++) {
		//if (pip_draw.p_src_img[i]) {
		if (bPullData[i]){
			p_disp = UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1+i);
			if ((ret = ImageApp_Photo_DispReleaseOut(p_disp, &src_img[i])) != HD_OK) {
				DBG_ERR("disp release_out error(%d)\r\n", ret);
			}
		}
	}
}
#endif

static void PhotoExe_InitDisplayStream(UINT32 dualCam, USIZE *pImageRatioSize)
{
	UINT32 i;
	PHOTO_DISP_INFO     *p_disp = NULL;
	//APPDISP_VIEW_INFO    appdisp_info = {0};


	// sensor 1 display
	if (dualCam == DUALCAM_FRONT) {
		p_disp = UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1);
		p_disp->enable = TRUE;
		//p_disp->width  =  0;
		//p_disp->height =  0;
		p_disp->width_ratio  =  pImageRatioSize->w;
		p_disp->height_ratio =  pImageRatioSize->h;
#if (DISPLAY2_FUNC == ENABLE)
		if (g_DualVideo == DISPLAY_1) {
			p_disp->vid_out      =  PHOTO_VID_OUT_1;
		} else if (g_DualVideo == DISPLAY_2) {
			p_disp->vid_out      =  PHOTO_VID_OUT_2;
		} else if (g_DualVideo == (DISPLAY_1|DISPLAY_2)) {
			//p_disp->vid_out      =  PHOTO_VID_OUT_2; //only display video on HDMI(videoout2)
			p_disp->vid_out      =  PHOTO_VID_OUT_1;
		}
#else
		p_disp->vid_out      =  PHOTO_VID_OUT_1;
#endif
		if(p_disp->vid_out == PHOTO_VID_OUT_1)
        		p_disp->rotate_dir   =  SysVideo_GetDirbyID(DOUT1);
		else
        		p_disp->rotate_dir   =  SysVideo_GetDirbyID(DOUT2);
		ImageApp_Photo_Config(PHOTO_CFG_DISP_INFO, (UINT32)p_disp);
	}
	// sensor 2 display
	else if (dualCam == DUALCAM_BEHIND) {
		p_disp = UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_2);
		p_disp->enable = TRUE;
		//p_disp->width  =  0;
		//p_disp->height =  0;
		p_disp->width_ratio  =  pImageRatioSize->w;
		p_disp->height_ratio =  pImageRatioSize->h;
		p_disp->rotate_dir   =  SysVideo_GetDirbyID(DOUT1);
#if (DISPLAY2_FUNC == ENABLE)
		if (g_DualVideo == DISPLAY_1) {
			p_disp->vid_out      =  PHOTO_VID_OUT_1;
		} else if (g_DualVideo == DISPLAY_2) {
			p_disp->vid_out      =  PHOTO_VID_OUT_2;
		} else if (g_DualVideo == (DISPLAY_1|DISPLAY_2)) {
			p_disp->vid_out      =  PHOTO_VID_OUT_2; //only display video on HDMI(videoout2)
		}
#else
		p_disp->vid_out      =  PHOTO_VID_OUT_1;
#endif
		ImageApp_Photo_Config(PHOTO_CFG_DISP_INFO, (UINT32)p_disp);
	}
#if 1
	// PIP view
	else {
		//if (AppDispView_GetInfo(&appdisp_info) != E_OK)
		//	return;
		for (i=0;i<localInfo->sensorCount;i++) {
				p_disp = UIAppPhoto_get_DispConfig(UIAPP_PHOTO_DISP_ID_1+i);
				if (p_disp== NULL){
					return;
				}
				p_disp->enable          = PHOTO_PORT_STATE_EN_RUN;
#if ((SENSOR_CAPS_COUNT == 2) && (SENSOR_INSERT_MASK != 0))
				if (System_GetEnableSensor() == SENSOR_1 && i==UIAPP_PHOTO_DISP_ID_2){
					p_disp->enable          = PHOTO_PORT_STATE_EN;
				}
#endif
				#if (SBS_VIEW_FUNC == ENABLE)
				p_disp->multi_view_type = PHOTO_MULTI_VIEW_SBS_LR;
				#else
				p_disp->multi_view_type = PHOTO_MULTI_VIEW_PIP;
				#endif
				p_disp->width_ratio  =  pImageRatioSize->w;
				p_disp->height_ratio =  pImageRatioSize->h;
				p_disp->rotate_dir   =  SysVideo_GetDirbyID(DOUT1);
#if (DISPLAY2_FUNC == ENABLE)
				if (g_DualVideo == DISPLAY_1) {
					p_disp->vid_out      =  PHOTO_VID_OUT_1;
				} else if (g_DualVideo == DISPLAY_2) {
					p_disp->vid_out      =  PHOTO_VID_OUT_2;
				} else if (g_DualVideo == (DISPLAY_1|DISPLAY_2)) {
					p_disp->vid_out      =  PHOTO_VID_OUT_2; //only display video on HDMI(videoout2)
				}
#else
				p_disp->vid_out      =  PHOTO_VID_OUT_1;
#endif
				ImageApp_Photo_Config(PHOTO_CFG_DISP_INFO, (UINT32)p_disp);
		}
		//ImageApp_Photo_DispConfig(PHOTO_CFG_DISP_REG_CB ,(UINT32)&PhotoExe_DispCB);
	}
#endif
}
static void PhotoExe_InitCapStream(UINT32 dualCam, USIZE *pImageRatioSize)
{
	PHOTO_CAP_INFO *pCapInfo;
	UINT32                  sensor_id;
	UINT32 ImageRatioIdx = 0;
	USIZE  ImageRatioSize = {0};

	for (sensor_id=0;sensor_id<localInfo->sensorCount;sensor_id++) {
		pCapInfo = UIAppPhoto_get_CapConfig(sensor_id);
		if(pCapInfo){
			// Set Capture image ratio
			{
				ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
				ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];
				//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_IMG_RATIO, _PhotoExe_GetImageAR(ImageRatioSize.w, ImageRatioSize.h));
				pCapInfo->img_ratio=_PhotoExe_GetImageAR(ImageRatioSize.w, ImageRatioSize.h);
			}
			pCapInfo->sCapSize.w=GetPhotoSizeWidth(UI_GetData(FL_PHOTO_SIZE));
			pCapInfo->sCapSize.h=GetPhotoSizeHeight(UI_GetData(FL_PHOTO_SIZE));
			pCapInfo->sCapMaxSize.w=GetPhotoSizeWidth(PHOTO_MAX_CAP_SIZE);
			pCapInfo->sCapMaxSize.h=GetPhotoSizeHeight(PHOTO_MAX_CAP_SIZE);
			pCapInfo->jpgfmt=HD_VIDEO_PXLFMT_YUV420;
			ImageApp_Photo_Config(PHOTO_CFG_CAP_INFO, (UINT32)pCapInfo);
		}
    }
}

/**
  Initialize application for Photo mode

  Initialize application for Photo mode.

  @param void
  @return void
*/

INT32 PhotoExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	HD_RESULT           hd_ret;
	PHOTO_VOUT_INFO Disp_Info;
	UINT32              dualCam=0;
	USIZE               ImageRatioSize = {0};
    UINT32              ImageRatioIdx;

    UI_SetData(FL_PHOTO_SIZE, PHOTO_SIZE_12M);

	g_bPhotoOpened = FALSE;
	localInfo->isStartCapture = FALSE;

   	ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
	g_photo_ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];

//    g_photo_ImageRatioSize = IMAGERATIO_SIZE[4];

	PhotoExe_CommPoolInit();
	dualCam =PhotoExe_InitSensorCount();
	PhotoExe_InitFileNaming();
	DBG_IND("PhotoExe_OnOpen ,dualCam=%d\r\n",dualCam);

	Disp_Info.vout_ctrl = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_CTRLPATH);
	Disp_Info.vout_path = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_PATH);
	Disp_Info.vout_ratio= GxVideo_GetDeviceAspect(0);
	#if defined(_MODEL_580_SDV_SJ10_)
    if (SysGetFlag(FL_CURR_DISP) == LCD_CHG_DISP_LCD2) {
        Disp_Info.vout_ratio.w = 240;
        Disp_Info.vout_ratio.h = 240;
    }
	#endif

	ImageApp_Photo_DispConfig(PHOTO_CFG_VOUT_INFO, (UINT32)&Disp_Info);
   	ImageApp_Photo_DispConfig(PHOTO_CFG_DISP_REG_CB ,(UINT32)&PhotoExe_DispCB);
	PhotoExe_InitDisplayStream(dualCam,&g_photo_ImageRatioSize);

	// call vendor_isp_init() in movie/photo mode OnOpen
	if ((hd_ret = vendor_isp_init()) != HD_OK) {
		DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
	}

	PhotoExe_InitNetworkStream(dualCam,&g_photo_ImageRatioSize);
   	ImageApp_Photo_WiFiConfig(PHOTO_CFG_WIFI_REG_CB ,(UINT32)&PhotoExe_WifiCB);


	PHOTO_SENSOR_INFO sen_cfg = {0};
	UIAPP_PHOTO_SENSOR_INFO *pSensorInfo;
#if (SENSOR_SIEPATGEN == ENABLE)
	PHOTO_VCAP_PAT_GEN vcap_pat_gen={0};
#endif

	UINT32 i;
	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		pSensorInfo = UIAppPhoto_get_SensorInfo(i);
		if (pSensorInfo == NULL) {
			DBG_ERR("get pSensorInfo error\r\n");
			return NVTEVT_CONSUME;
		}
		sen_cfg.enable=PHOTO_PORT_STATE_EN_RUN;
		sen_cfg.vid_in = PHOTO_VID_IN_1 + i;
		sen_cfg.sSize[0].w=pSensorInfo->sSize.w;
		sen_cfg.sSize[0].h=pSensorInfo->sSize.h;
		sen_cfg.fps[0] =pSensorInfo->fps;
		sen_cfg.vcap_ctrl.func=(HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB);//HD_VIDEOCAP_FUNC_SHDR);
		//sen_cfg.vproc_func =(HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR);// | HD_VIDEOPROC_FUNC_3DNR);
		sen_cfg.vproc_func =(HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR);

        //#NT#20191014#Philex Lin - begin
        // SHDR/WDR/DEFOG function
#if SHDR_FUNC
	 if (UI_GetData(FL_SHDR) == SHDR_ON){
		sen_cfg.vcap_ctrl.func |=(HD_VIDEOCAP_FUNC_SHDR);
		sen_cfg.vproc_func |=(HD_VIDEOPROC_FUNC_SHDR);// | HD_VIDEOPROC_FUNC_3DNR);
	 }else{
		sen_cfg.vcap_ctrl.func &= ~(HD_VIDEOCAP_FUNC_SHDR);
		sen_cfg.vproc_func &= ~(HD_VIDEOPROC_FUNC_SHDR);// | HD_VIDEOPROC_FUNC_3DNR);
	 }
#endif

		#if WDR_FUNC
        sen_cfg.vproc_func |=(HD_VIDEOPROC_FUNC_WDR);// | HD_VIDEOPROC_FUNC_3DNR);
        #endif

		#if DEFOG_FUNC
        sen_cfg.vproc_func |=(HD_VIDEOPROC_FUNC_DEFOG);// | HD_VIDEOPROC_FUNC_3DNR);
        #endif
        //#NT#20191014#Philex Lin - end


		System_GetSensorInfo(i, SENSOR_DRV_CFG, &(sen_cfg.vcap_cfg));
		System_GetSensorInfo(i, SENSOR_SENOUT_FMT, &(sen_cfg.senout_pxlfmt));
		System_GetSensorInfo(i, SENSOR_CAPOUT_FMT, &(sen_cfg.capout_pxlfmt));
		System_GetSensorInfo(i, SENSOR_DATA_LANE, &(sen_cfg.data_lane));
		System_GetSensorInfo(i, SENSOR_AE_PATH, &(sen_cfg.ae_path));
		System_GetSensorInfo(i, SENSOR_AWB_PATH, &(sen_cfg.awb_path));
		System_GetSensorInfo(i, SENSOR_IQ_PATH, &(sen_cfg.iq_path));
		System_GetSensorInfo(i, SENSOR_IQ_CAP_PATH, &(sen_cfg.iqcap_path));
		System_GetSensorInfo(i, SENSOR_IQ_SHADING_PATH, &(sen_cfg.iq_shading_path));
		System_GetSensorInfo(i, SENSOR_IQ_DPC_PATH, &(sen_cfg.iq_dpc_path));
		System_GetSensorInfo(i, SENSOR_IQ_LDC_PATH, &(sen_cfg.iq_ldc_path));

		#if (SHDR_FUNC == ENABLE)
		 if (UI_GetData(FL_SHDR) == SHDR_OFF){
			sen_cfg.vcap_cfg.sen_cfg.shdr_map = 0;
			if ((HD_VIDEO_PXLFMT_CLASS(sen_cfg.capout_pxlfmt) == HD_VIDEO_PXLFMT_CLASS_RAW) && (HD_VIDEO_PXLFMT_PLANE(sen_cfg.capout_pxlfmt) != 1)) {
				//convert  SHDR  RAW to  RAW format
				sen_cfg.capout_pxlfmt &= ~HD_VIDEO_PXLFMT_PLANE_MASK;
				sen_cfg.capout_pxlfmt |= (1 << 24);
			}
		}
		#endif // (SHDR_FUNC == ENABLE)

		ImageApp_Photo_Config(PHOTO_CFG_SENSOR_INFO, (UINT32)&sen_cfg);
		PhotoExe_SetIMECrop(i);

#if (SENSOR_SIEPATGEN == ENABLE)
		vcap_pat_gen.vid=i;
		vcap_pat_gen.patgen_type=HD_VIDEOCAP_SEN_PAT_COLORBAR;
		ImageApp_Photo_Config(PHOTO_CFG_VCAP_PAT_GEN,(UINT32)&vcap_pat_gen);
#endif
#if (PHOTO_DIRECT_FUNC == ENABLE)
		PHOTO_VCAP_OUTFUNC vcap_func_cfg = {0};
		vcap_func_cfg.vid=PHOTO_VID_IN_1+i ;
		vcap_func_cfg.out_func =HD_VIDEOCAP_OUTFUNC_DIRECT;
		ImageApp_Photo_Config(PHOTO_CFG_VCAP_OUTFUNC, (UINT32)&vcap_func_cfg);
#endif
	}

    #if (DZOOM_FUNC == ENABLE)
    ImageApp_Photo_Config(PHOTO_CFG_3DNR_PATH, PHOTO_3DNR_REF_PATH);
    #else
    ImageApp_Photo_Config(PHOTO_CFG_3DNR_PATH, PHOTO_3DNR_SHARED_PATH);
    #endif

	PhotoExe_InitCapStream(dualCam,&ImageRatioSize);

	// register Callback function
	Photo_RegCB();

	ImageApp_Photo_Open();


#if (SENSOR_INSERT_FUNCTION == ENABLE)
	System_EnableSensorDet();
#endif

	PHOTO_IPL_MIRROR Flip = {0};
	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		memset(&Flip, 0 ,sizeof(Flip));
		Flip.vid=PHOTO_VID_IN_1+i ;
		Flip.mirror_type=(UI_GetData(FL_MOVIE_SENSOR_ROTATE) == SEN_ROTATE_ON) ? HD_VIDEO_DIR_MIRRORX : HD_VIDEO_DIR_NONE;//HD_VIDEO_DIR_MIRRORX;
		ImageApp_Photo_Config(PHOTO_CFG_IPL_MIRROR,(UINT32)&Flip);
	}

#if (USE_FILEDB== ENABLE)

	ImageApp_Photo_Config(PHOTO_CFG_FILEDB_MAX_NUM,5000);

	ImageApp_Photo_FileNaming_SetSortBySN("_", 1, 6);
	ImageApp_Photo_FileNaming_Open();
	{
		// sample code to get file serial number; assume the FileDB handle created by ImageApp is 0
		PFILEDB_FILE_ATTR  pFileAttr;
		if (FileDB_GetTotalFileNum(0)) {
			pFileAttr = FileDB_SearhFile(0, (FileDB_GetTotalFileNum(0)-1));
			if (pFileAttr != NULL) {
				g_i32PhotoFileSerialNum = FileDB_GetSNByName(0, pFileAttr->filename);
				if (g_i32PhotoFileSerialNum < 0){
					g_i32PhotoFileSerialNum = 0;
				}
			}else{
				DBG_ERR("get file failed\r\n");
				g_i32PhotoFileSerialNum = 0;
			}
		} else {
			g_i32PhotoFileSerialNum = 0;
		}
	}
#endif

	PhotoExe_InitExif();

#if (WIFI_AP_FUNC==ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_WIFI) {
		Ux_PostEvent(NVTEVT_WIFI_EXE_MODE_DONE, 1, E_OK);
		PhotoExe_WifiStamp();
	} else
#endif
	{
        	if (UI_GetData(FL_MOVIE_DATEIMPRINT) == MOVIE_DATEIMPRINT_ON){
                	MovieStamp_Disable();
        	}
	}

#if PHOTO_PREVIEW_SLICE_ENC_FUNC

	/* open encode path */
	if(PhotoExe_Preview_SliceEncode_Open() != E_OK){
		return NVTEVT_CONSUME;
	}
	
#endif

	Ux_DefaultEvent(pCtrl, NVTEVT_EXE_OPEN, paramNum, paramArray);

	g_bPhotoOpened = TRUE;

    return NVTEVT_CONSUME;
}


INT32 PhotoExe_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	HD_RESULT hd_ret;

	g_bPhotoOpened = FALSE;

#if (SENSOR_INSERT_FUNCTION == ENABLE)
	System_DisableSensorDet();
#endif

	PhotoExe_UninitExif();
	if (DATEIMPRINT_OFF != UI_GetData(FL_DATE_STAMP)) {
    		UiDateImprint_DestroyBuff();
  	}
	if (UI_GetData(FL_MOVIE_DATEIMPRINT) == MOVIE_DATEIMPRINT_ON){
		MovieStamp_Disable();
		MovieStamp_DestroyBuff();
	}
#if PHOTO_PREVIEW_SLICE_ENC_FUNC

	/* open encode path */
	if(PhotoExe_Preview_SliceEncode_Close() != E_OK){
		return NVTEVT_CONSUME;
	}

#endif

	ImageApp_Photo_Close();

	// call vendor_isp_uninit() in movie/photo mode OnClose
	if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
		DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
	}

	Ux_DefaultEvent(pCtrl, NVTEVT_EXE_CLOSE, paramNum, paramArray);

    return NVTEVT_CONSUME;
}


INT32 PhotoExe_OnSleep(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    ImageApp_Photo_SetScreenSleep();
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnWakeup(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    ImageApp_Photo_SetScreenWakeUp();
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnMacro(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}
	UI_SetData(FL_MACRO, uhSelect);
	PhotoExe_AF_SetUIInfo(AFT_ITEM_RLD_CONFIG, Get_MacroValue(uhSelect));

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnSelftimer(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_SELFTIMER, uhSelect);
	DBG_IND("photo selftimer %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnEV(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_EV, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_EV, uhSelect);
	PhotoExe_AE_SetUIInfo(AET_ITEM_EV, Get_EVValue(uhSelect));
	DBG_IND("photo ev %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnCaptureSize(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);

	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}
	UI_SetData(FL_PHOTO_SIZE, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_SIZE, uhSelect);
	DBG_IND("photo capture size %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnQuality(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}
	UI_SetData(FL_QUALITY, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_QUALITY, uhSelect);
	DBG_IND("photo quality %d\r\n", uhSelect);

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnWB(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);

	uhSelect = paramArray[0];
	UI_SetData(FL_WB, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_WB, uhSelect);
	PhotoExe_AWB_SetUIInfo(AWBT_ITEM_SCENE, Get_WBValue(uhSelect));
	DBG_IND("photo wb %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnColor(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);

	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	if (UI_GetData(FL_ModeIndex) != DSC_MODE_PHOTO_SCENE) {
		UI_SetData(FL_COLOR_EFFECT, uhSelect);
		Photo_SetUserIndex(PHOTO_USR_COLOR, uhSelect);
//		PhotoExe_IQ_SetUIInfo(IQ_UI_IMAGEEFFECT, Get_ColorValue(uhSelect));
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnISO(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);

	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_ISO, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_ISO, uhSelect);
	PhotoExe_AE_SetUIInfo(AET_ITEM_ISO, Get_ISOValue(uhSelect));
	DBG_IND("photo iso %d\r\n", uhSelect);

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnAFWindow(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);

	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_AFWindowIndex, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_AFWINDOW, uhSelect);
	PhotoExe_AF_SetUIInfo(AFT_ITEM_RLD_CONFIG, Get_AFWindowValue(uhSelect));
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnAFBeam(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_AFBeamIndex, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_AFBEAM, uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnContAF(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_ContAFIndex, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_CONTAF, uhSelect);
	if (uhSelect == CONT_AF_ON) {
		localInfo->isCafOn = TRUE;
	} else {
		localInfo->isCafOn = FALSE;
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnMetering(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);

	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_METERING, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_METERING, uhSelect);
	PhotoExe_AE_SetUIInfo(AET_ITEM_METER, Get_MeteringValue(uhSelect));
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnCaptureMode(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_CapModeIndex, uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnDatePrint(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;

	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_DATE_STAMP, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_DATEIMPRINT, uhSelect);
	DBG_IND("photo datestamp %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnPreview(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;

	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_QUICK_REVIEW, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_QREVIEW, uhSelect);
	DBG_IND("photo quick view time %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnDigitalZoom(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_Dzoom, uhSelect);
	Photo_SetUserIndex(PHOTO_USR_DZOOMSTATUS, uhSelect);
	if (uhSelect == DZOOM_OFF) {
		UI_SetData(FL_DzoomIndex, DZOOM_10X);
	}
	return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnFD(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	DBG_IND("%d \r\n", paramArray[0]);

	UI_SetData(FL_FD, uhSelect);
    //#NT#2019/09/18#Philex Lin -begin
	if (uhSelect == FD_ON) {
//		PhotoExe_IPL_AlgSetFD_AEInfo(SEL_FD_ON);
		localInfo->isSdOn = FALSE;
		localInfo->isFdOn = TRUE;
#if _FD_FUNC_
		FD_Lock(FALSE);
#endif
	} else {
//		PhotoExe_IPL_AlgSetFD_AEInfo(SEL_FD_OFF);
		localInfo->isSdOn = FALSE;
		localInfo->isFdOn = FALSE;
		if (!localInfo->isAscnOn) {
#if _FD_FUNC_
			FD_Lock(TRUE);
#endif
		}
	}
    //#NT#2019/09/18#Philex Lin -end
	DBG_IND("photo fd %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnContShot(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;

	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_CONTINUE_SHOT, uhSelect);
	DBG_IND("photo contshot  %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}



INT32 PhotoExe_OnCaptureStart(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if PHOTO_PREVIEW_SLICE_ENC_FUNC

	if (g_bPhotoOpened == FALSE) {
		DBG_ERR("photo mode not open yet!\r\n");
		return NVTEVT_CONSUME;
	}

	if (localInfo->isStartCapture) {
		DBG_ERR("Capture start in Capturing\r\n");
		return NVTEVT_CONSUME;
	}

	if (UI_GetData(FL_DATE_STAMP) != DATEIMPRINT_OFF) {
		//Init Date Imprint buff
		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_INIT_DATE_BUF, 0);
	}

	localInfo->isStartCapture = TRUE;

	/* Start to capture */
	PhotoExe_SetQuickViewSize(0);
	localInfo->IsJPGok = FALSE;
	localInfo->IsCapFSTok = FALSE;
	PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_CAPSTART);

	ImageApp_Photo_Disp_SetPreviewShow(0);

#if PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC == DISABLE

	UINT32 cnt = 0;

	switch (UI_GetData(FL_CONTINUE_SHOT)) {
	case CONTINUE_SHOT_BURST:
		cnt = 5;
		break;
	case CONTINUE_SHOT_BURST_3:
		cnt = 3;
		break;
	case CONTINUE_SHOT_OFF:
	default:
		cnt = 1;
		break;
	}

	for(UINT32 i = 0 ; i < cnt ; i++)
	{
		Ux_PostEvent(NVTEVT_EXE_SLICE_ENCODE, 2, 0, i == (cnt - 1) ? TRUE : FALSE);
	}
#else
	Ux_PostEvent(NVTEVT_EXE_SLICE_ENCODE, 1, 0);
#endif

#else
	#if PHOTO_DIRECT_FUNC
		DBG_ERR("photo mode direct MUST use slice capture, please enable PHOTO_PREVIEW_SLICE_ENC_FUNC!!\r\n");
		return NVTEVT_CONSUME;
	#endif
	UINT32 i, sensor_id, sensor_cnt = 1;
	PHOTO_CAP_INFO *pCapInfo;

	if (g_bPhotoOpened == FALSE) {
		DBG_ERR("photo mode not open yet!\r\n");
		return NVTEVT_CONSUME;
	}

	if (localInfo->isStartCapture) {
		DBG_ERR("Capture start in Capturing\r\n");
		return NVTEVT_CONSUME;
	}

	if (UI_GetData(FL_DATE_STAMP) != DATEIMPRINT_OFF) {
		//Init Date Imprint buff
		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_INIT_DATE_BUF, 0);
	}
	if (UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_SIDE) {
		DBG_IND("[cap]sidebyside\r\n");
	}
	//  dual cam capture
	else if (localInfo->DualCam >= DUALCAM_BOTH) {
		sensor_cnt = localInfo->sensorCount;
	}
	DBG_IND("sensor_cnt=%d\r\n",sensor_cnt);

	for (i = 0; i < sensor_cnt; i++) {
#if (SENSOR_CAPS_COUNT >= 2)
		if (sensor_cnt >= 2) {
			sensor_id = i;
		} else
#endif
		{
			// single sensor case
			if (localInfo->DualCam == DUALCAM_BEHIND) {
				sensor_id = localInfo->DualCam; //get active sensor
			}
			else {
				sensor_id = 0;
			}
			DBG_IND("[cap]single sensor_id=%d \r\n", sensor_id);
		}

		PhotoExe_SetScreenNailSize(sensor_id);
		PhotoExe_SetQuickViewSize(sensor_id);

		pCapInfo = UIAppPhoto_get_CapConfig(sensor_id);
		memcpy(pCapInfo,ImageApp_Photo_GetCapConfig(sensor_id), sizeof(PHOTO_CAP_INFO));
		if(localInfo->uiMaxImageSize==0){
			PhotoExe_GetFreePicNum();
		}
		pCapInfo->filebufsize=localInfo->uiMaxImageSize;
		// BRC setting
		{
			pCapInfo->rho_targetsize=PhotoExe_GetExpectSize_RhoBRCrtl(UI_GetData(FL_PHOTO_SIZE), TRUE);
			pCapInfo->reenctype=SEL_REENCTYPE_RHO;
			pCapInfo->rho_initqf=85;
			pCapInfo->rho_hboundsize= pCapInfo->rho_targetsize + (pCapInfo->rho_targetsize * 15 / 100); //+15%
			pCapInfo->rho_lboundsize= pCapInfo->rho_targetsize - (pCapInfo->rho_targetsize * 15 / 100); //-15%
			pCapInfo->rho_retrycnt = 4;
			DBG_IND("[cap]TargetBytes=%d k, H=%d k, L=%d K\r\n", pCapInfo->rho_targetsize / 1024, pCapInfo->rho_hboundsize / 1024, pCapInfo->rho_lboundsize / 1024);
		}

		if(PhotoExe_GetCapYUV420En()){
			pCapInfo->jpgfmt=HD_VIDEO_PXLFMT_YUV420;
			pCapInfo->thumb_fmt=HD_VIDEO_PXLFMT_YUV420;
			pCapInfo->qv_img_fmt=HD_VIDEO_PXLFMT_YUV420;
			pCapInfo->screen_fmt=HD_VIDEO_PXLFMT_YUV420;

		}else{
			pCapInfo->jpgfmt=HD_VIDEO_PXLFMT_YUV422;
			pCapInfo->thumb_fmt=HD_VIDEO_PXLFMT_YUV422;
			pCapInfo->qv_img_fmt=HD_VIDEO_PXLFMT_YUV422;
			pCapInfo->screen_fmt=HD_VIDEO_PXLFMT_YUV422;
		}

		pCapInfo->datastamp=Get_DatePrintValue(UI_GetData(FL_DATE_STAMP));
		#if ((SENSOR_CAPS_COUNT == 2) && (SENSOR_INSERT_MASK != 0))
		if (System_GetEnableSensor() == SENSOR_1){
			if(i==0){
				pCapInfo->actflag =TRUE;
			}else{
				pCapInfo->actflag =FALSE;
			}
	 	}else{
			pCapInfo->actflag =TRUE;
	 	}
		#else
		pCapInfo->actflag =TRUE;
		#endif

		pCapInfo->quality=Get_QualityValue(UI_GetData(FL_QUALITY));
		pCapInfo->sCapSize.w=GetPhotoSizeWidth(UI_GetData(FL_PHOTO_SIZE));
		pCapInfo->sCapSize.h=GetPhotoSizeHeight(UI_GetData(FL_PHOTO_SIZE));
		pCapInfo->sCapMaxSize.w=GetPhotoSizeWidth(PHOTO_MAX_CAP_SIZE);
		pCapInfo->sCapMaxSize.h=GetPhotoSizeHeight(PHOTO_MAX_CAP_SIZE);
		switch (UI_GetData(FL_CONTINUE_SHOT)) {
		case CONTINUE_SHOT_BURST:
			pCapInfo->picnum=SEL_PICNUM_INF;//9999;
			pCapInfo->raw_buff=1;//2;
			pCapInfo->jpg_buff=2;//3;

			UI_SetData(FL_IsSingleCapture, FALSE);
			localInfo->isDoingContShot = TRUE;
			localInfo->isStopingContShot = FALSE;
			localInfo->uiTakePicNum = 0;
			// set s2 pressed status to true
			Photo_setS2Status(TRUE);
			break;
		case CONTINUE_SHOT_BURST_3:
			pCapInfo->picnum=3;
			pCapInfo->raw_buff=1;//2;
			pCapInfo->jpg_buff=2;

			UI_SetData(FL_IsSingleCapture, FALSE);
			localInfo->isDoingContShot = TRUE;
			localInfo->isStopingContShot = FALSE;
			localInfo->uiTakePicNum = 0;
			break;
		default:
			pCapInfo->picnum=1;
			pCapInfo->raw_buff=1;
			pCapInfo->jpg_buff=1;

			UI_SetData(FL_IsSingleCapture, TRUE);
			localInfo->isDoingContShot = FALSE;
			break;
		}
		// Set Capture image ratio
		{
			UINT32 ImageRatioIdx = 0;
			USIZE  ImageRatioSize = {0};

			ImageRatioIdx = GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE));
			ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];
			//PhotoExe_Cap_SetUIInfo(sensor_id, CAP_SEL_IMG_RATIO, _PhotoExe_GetImageAR(ImageRatioSize.w, ImageRatioSize.h));
			pCapInfo->img_ratio=_PhotoExe_GetImageAR(ImageRatioSize.w, ImageRatioSize.h);
		}

		ImageApp_Photo_Config(PHOTO_CFG_CAP_INFO, (UINT32)pCapInfo);


	}
	localInfo->isStartCapture = TRUE;

	/* Start to capture */
	localInfo->IsJPGok = FALSE;
	localInfo->IsCapFSTok = FALSE;
	PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_CAPSTART);

	ImageApp_Photo_CapStart();
#endif
	return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnCaptureStop(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if (localInfo->isDoingContShot) {
		Photo_setS2Status(FALSE);
		ImageApp_Photo_CapStop();
	}
	return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnCaptureEnd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_IND("[cap]\r\n");
	localInfo->isDoingContShot = FALSE;
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnZoom(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if DZOOM_FUNC
	UINT32 uiZoomCtrl = UI_ZOOM_CTRL_STOP;

	if (paramNum == 1) {
		uiZoomCtrl = paramArray[0];
	} else {
		DBG_ERR("ParamNum %d\r\n", paramNum);
	}

	switch (uiZoomCtrl) {

	case UI_ZOOM_CTRL_STOP:
		PhotoExe_DZoomStop();
		break;

	case UI_ZOOM_CTRL_IN:
		PhotoExe_DZoomIn();
		break;

	case UI_ZOOM_CTRL_OUT:
		PhotoExe_DZoomOut();
		break;

	case UI_ZOOM_CTRL_RESET_DZOOM:
		break;

	case UI_ZOOM_CTRL_RESET_OZOOM:
		break;

	default:
		DBG_ERR("Unknown zoom control 0x%x\r\n", uiZoomCtrl);
		break;
	}
#endif
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnRSC(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uiSelect = 0;

	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum) {
		uiSelect = paramArray[0];
	}

	UI_SetData(FL_RSC, uiSelect);

	if (uiSelect == RSC_ON) {
#if (!defined(_Gyro_None_) && (RSC_FUNC == ENABLE))
		PhotoExe_RSC_SetSwitch(SEL_RSC_RUNTIME, SEL_RSC_ON);
#endif
		if (UI_GetData(FL_SHDR) == SHDR_ON) {
			Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_SHDR, 1, SHDR_OFF);
		}
	} else {
#if (!defined(_Gyro_None_) && (RSC_FUNC == ENABLE))
		PhotoExe_RSC_SetSwitch(SEL_RSC_RUNTIME, SEL_RSC_OFF);
#endif
	}
	DBG_IND("photo rsc %d\r\n", uiSelect);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnStartFunc(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32   func = 0;
	UINT32   waitIdle = FALSE;

	DBG_IND("func=0x%x\r\n", paramArray[0]);
	if (localInfo->isStartCapture) {
		DBG_ERR("in capturing\r\n");
	} else if (paramNum == 2) {
		func = paramArray[0];
		waitIdle = paramArray[1];

#if _FD_FUNC_
		if (func & UIAPP_PHOTO_FD) {
			FD_Lock(FALSE);
		}
#endif
		if (func & UIAPP_PHOTO_AF) {
			AF_LOCK(FALSE);
		}
		if (waitIdle == TRUE) {
		}
	} else {
		DBG_ERR("wrong param 0x%x\r\n", paramNum);
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnStopFunc(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32   func = 0;
	UINT32   waitIdle = FALSE;

	DBG_IND("func=0x%x\r\n", paramArray[0]);
	if (paramNum == 2) {
		func = paramArray[0];
		waitIdle = paramArray[1];

#if _FD_FUNC_
		if (func & UIAPP_PHOTO_FD) {
			FD_Lock(TRUE);
		}
#endif
		if (func & UIAPP_PHOTO_AF) {
			AF_LOCK(TRUE);
		}
		if (waitIdle == TRUE) {
			AF_WAIT_IDLE();
		}

	} else {
		DBG_ERR("wrong param 0x%x\r\n", paramNum);
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnAFProcess(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (LENS_FUNCTION == ENABLE)
	UINT32 isAFBeam;
	//UINT32 isSupportPunchThrough;
	UINT32 currEV, EV_Threshold, LV;

	isAFBeam = paramArray[0];
#if (ASSIST_BEAM_FUNC == DISABLE)
	isAFBeam = FALSE;
#endif
	//isSupportPunchThrough = paramArray[1];
	DBG_FUNC_BEGIN("[af]\r\n");
	//AE,AWB lock will auto control by capture flow or AF flow
	//AE_LOCK(TRUE);
	//AWB_LOCK(TRUE);
	AF_LOCK(TRUE);
#if _FD_FUNC_
	FD_Lock(TRUE);
#endif
	Ux_FlushEventByRange(NVTEVT_ALGMSG_FOCUSEND, NVTEVT_ALGMSG_FOCUSEND);
	Ux_FlushEventByRange(NVTEVT_EXE_FDEND, NVTEVT_EXE_FDEND);

	currEV = CURR_EV();
	LV     =  currEV / 10;
	EV_Threshold = AF_BEAM_EV_THRESHOLD;
	DBG_IND("[af]EV = %d, LV=%d\r\n", currEV, LV);

	if (isAFBeam && (currEV < EV_Threshold)) { //LV 5.6~6.1
		//If AF beam is on ,then turn on the focus LED before AF_Process().
		if (!localInfo->isAFBeam) {
			if (LV > 6) {
				DBG_ERR("[af]AF LV %d\r\n", LV);
				LED_SetFcsLevel(0);
			} else {
				LED_SetFcsLevel(LCSBRT_LVL_06 - LV);
			}
			LED_TurnOnLED(GPIOMAP_LED_FCS);
			localInfo->isAFBeam = TRUE;
		}
		/* AF_Run no wait. */
        PhotoExe_AF_SetUIInfo(AFT_ITEM_RETRIGGER,TRUE)
		localInfo->IsAFProcess = TRUE;
	} else {
		/* AF_Run no wait. */
        PhotoExe_AF_SetUIInfo(AFT_ITEM_RETRIGGER,TRUE)
		localInfo->IsAFProcess = TRUE;
	}

	DBG_FUNC_END("[af]\r\n");
#endif
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnAFRelease(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (LENS_FUNCTION == ENABLE)
	DBG_IND("[af]\r\n");
	AF_Release(AF_ID_1, TRUE);

	if (localInfo->isAFBeam) {
		LED_TurnOffLED(GPIOMAP_LED_FCS);   //If AF beam is on ,then turn off the focus LED after AF_Process().
		LED_SetFcsLevel(LCSBRT_LVL_03);
		localInfo->isAFBeam = FALSE;
	}
	DBG_FUNC_END("[af]\r\n");
#endif

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnAFWaitEnd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (LENS_FUNCTION == ENABLE)
	DBG_IND("[af]\r\n");

	if (localInfo->IsAFProcess) {
		AF_WAIT_IDLE();
		localInfo->IsAFProcess = FALSE;
		if (localInfo->isAFBeam) {
			LED_TurnOffLED(GPIOMAP_LED_FCS);   //If AF beam is on ,then turn off the focus LED after AF_Process().
			LED_SetFcsLevel(LCSBRT_LVL_03);
			localInfo->isAFBeam = FALSE;
		}
	}
	DBG_FUNC_END("[af]\r\n");
#endif

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnImageRatio(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if 1
	UINT32           ImageRatioIdx = 0;
	USIZE            ImageRatioSize = {0};
	//UINT32           ime3dnr_size = 0;
	UINT32		sensor_id;
	//URECT            DispCoord;
	//UIAPP_PHOTO_SENSOR_INFO *pSensorInfo;
	//PHOTO_STRM_INFO    *p_strm = NULL;
	//USIZE sz_img={0};
	//UINT32    rotate_dir=0;
	//UINT32 dst_w ,dst_h;

    if (paramNum > 0) {
		ImageRatioIdx = paramArray[0];
	}
	DBG_IND("ImageRatioIdx =%d\r\n", ImageRatioIdx);
	if (ImageRatioIdx >= IMAGERATIO_MAX_CNT) {
		DBG_ERR("ImageRatioIdx =%d\r\n", ImageRatioIdx);
		return NVTEVT_CONSUME;
	}
	ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];

	if (g_photo_ImageRatioSize.w == ImageRatioSize.w && g_photo_ImageRatioSize.h == ImageRatioSize.h){
		return NVTEVT_CONSUME;
	}

	if (UI_GetData(FL_MOVIE_DATEIMPRINT) == MOVIE_DATEIMPRINT_ON){
		MovieStamp_Disable();
		MovieStamp_DestroyBuff();
	}

	g_photo_ImageRatioSize = ImageRatioSize;

	for (sensor_id=0;sensor_id<localInfo->sensorCount;sensor_id++) {
	    	PhotoExe_SetIMECrop(sensor_id);
	    	ImageApp_Photo_SetVdoImgSize(sensor_id,ImageRatioSize.w,ImageRatioSize.h);

	    	ImageApp_Photo_SetVdoAspectRatio(PHOTO_DISP_ID_1+sensor_id,ImageRatioSize.w,ImageRatioSize.h);
	    	ImageApp_Photo_SetVdoAspectRatio(PHOTO_STRM_ID_1+sensor_id,ImageRatioSize.w,ImageRatioSize.h);

	}
	if (UI_GetData(FL_MOVIE_DATEIMPRINT) == MOVIE_DATEIMPRINT_ON){
		PhotoExe_WifiStamp();
	}

#else
	UINT32           ImageRatioIdx = 0;
	USIZE            ImageRatioSize = {0};
	UINT32           ime3dnr_size = 0, sensor_id;
	URECT            DispCoord;
	UIAPP_PHOTO_SENSOR_INFO *pSensorInfo;
	PHOTO_STRM_INFO    *p_strm = NULL;
	USIZE sz_img={0};
	UINT32    rotate_dir=0;
	UINT32 dst_w ,dst_h;


    if (paramNum > 0) {
		ImageRatioIdx = paramArray[0];
	}
	DBG_IND("ImageRatioIdx =%d\r\n", ImageRatioIdx);
	if (ImageRatioIdx >= IMAGERATIO_MAX_CNT) {
		DBG_ERR("ImageRatioIdx =%d\r\n", ImageRatioIdx);
		return NVTEVT_CONSUME;
	}
	ImageRatioSize = IMAGERATIO_SIZE[ImageRatioIdx];

	if (g_photo_ImageRatioSize.w == ImageRatioSize.w && g_photo_ImageRatioSize.h == ImageRatioSize.h)
		return NVTEVT_CONSUME;
	g_photo_ImageRatioSize = ImageRatioSize;
	// get display coordinate
	PhotoExe_GetDispCord(&DispCoord);

	for (sensor_id=0;sensor_id<localInfo->sensorCount;sensor_id++) {
		pSensorInfo = UIAppPhoto_get_SensorInfo(sensor_id);
		if (pSensorInfo == NULL) {
			DBG_ERR("get pSensorInfo error\r\n");
			return NVTEVT_CONSUME;
		}
	    ImageApp_Photo_SetVdoAspectRatio(PHOTO_IME3DNR_ID_1+sensor_id,ImageRatioSize.w,ImageRatioSize.h);
		if (pSensorInfo->bIME3DNR) {
		    ime3dnr_size = PhotoExe_AutoCal_IME3DNRSize(sensor_id,pSensorInfo,&ImageRatioSize);
			ImageApp_Photo_SetVdoImgSize(PHOTO_IME3DNR_ID_1+sensor_id, ISF_GET_HI(ime3dnr_size), ISF_GET_LO(ime3dnr_size));
			//ImageApp_Photo_ResetPath(PHOTO_IME3DNR_ID_1+sensor_id);
		}
		else {
			ImageUnit_Begin(ISF_IPL(sensor_id), 0);
			ImageUnit_SetVdoAspectRatio(ISF_IN1, ImageRatioSize.w, ImageRatioSize.h);
			ImageUnit_End();
		}

		sz_img.w=ImageRatioSize.w;
		sz_img.h=ImageRatioSize.h;
		ImageApp_Photo_DispGetVdoImgSize(PHOTO_DISP_ID_1+sensor_id, &sz_img.w, &sz_img.h);


		ImageUnit_GetVdoDirection(&ISF_VdoOut1, ISF_IN1, &rotate_dir);

		if (((rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) || \
		((rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
			dst_w=sz_img.h;
			dst_h=sz_img.w;

		}else{
			dst_w=sz_img.w;
			dst_h=sz_img.h;
		}

		ImageApp_Photo_SetVdoImgSize(PHOTO_DISP_ID_1+sensor_id, dst_w, dst_h);
	    ImageApp_Photo_SetVdoAspectRatio(PHOTO_DISP_ID_1+sensor_id,ImageRatioSize.w,ImageRatioSize.h);

		p_strm = UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1+sensor_id);
		p_strm->width = ALIGN_CEIL_16(p_strm->height* ImageRatioSize.w/ImageRatioSize.h);

		#if (SBS_VIEW_FUNC == ENABLE)
		ImageApp_Photo_SetVdoImgSize(PHOTO_STRM_ID_1+sensor_id, (p_strm->width << 1), p_strm->height);
		#else
		ImageApp_Photo_SetVdoImgSize(PHOTO_STRM_ID_1+sensor_id, p_strm->width, p_strm->height);
        #endif
		ImageApp_Photo_SetVdoAspectRatio(PHOTO_STRM_ID_1+sensor_id,ImageRatioSize.w,ImageRatioSize.h);
	}
	// Set Fd image ratio
#if _FD_FUNC_
	{
		localInfo->FdDispCoord = DispCoord;
	}
#endif

#endif
    return NVTEVT_CONSUME;
}
static void PhotoExe_StopContShot(void)
{
	if (localInfo->isStopingContShot == FALSE) {
		localInfo->isStopingContShot = TRUE;
		ImageApp_Photo_CapStop();
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_STOP_CONTSHOT);
	}
}
static BOOL PhotoExe_CheckBD(void)
{
	return FALSE;
}

IMG_CAP_QV_DATA gPhoto_QvData = {0};

INT32 PhotoExe_OnCallback(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	NVTEVT event;

	event = paramArray[0];

	DBG_IND("[cb] event=0x%x\r\n", event);
	switch (event) {
	case NVTEVT_ALGMSG_FLASH:
		DBG_IND("[cb]NVTEVT_ALGMSG_FLASH\r\n");
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_FLASH);
		break;

	case NVTEVT_ALGMSG_QVSTART:

		DBG_IND("[cb]NVTEVT_ALGMSG_QVSTART\r\n");
		//copy current QV data
		memcpy(&gPhoto_QvData, (IMG_CAP_QV_DATA *)(paramArray[1]), sizeof(IMG_CAP_QV_DATA));
		//Charge flash
		if (UI_GetData(FL_FLASH_MODE) != FLASH_OFF) {
			//#NT#2011/04/15#Lincy Lin -begin
			//#NT#Hera14 HW bug , no battery insert can't charge for flash
			if (!UI_GetData(FL_IsStopCharge))
				//#NT#2011/04/15#Lincy Lin -end
			{
#if (FLASHLIGHT_FUNCTION == ENABLE)
				SxTimer_SetFuncActive(SX_TIMER_DET_RECHARGE_ID, TRUE);
				GxFlash_StartCharge();
#endif

			}
		}
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_QVSTART);
		break;

	case NVTEVT_ALGMSG_JPGOK:

		DBG_IND("[cb]NVTEVT_ALGMSG_JPGOK\r\n");

		// add picture count 1
		UI_SetData(FL_TakePictCnt, UI_GetData(FL_TakePictCnt) + 1);
		localInfo->IsJPGok = TRUE;
#if (FS_FUNC == ENABLE)
		localInfo->isFolderFull = UIStorageCheck(STORAGE_CHECK_FOLDER_FULL, NULL);
		localInfo->isCardFull = UIStorageCheck(STORAGE_CHECK_FULL, &localInfo->FreePicNum);
#else
		localInfo->isFolderFull=0;
		localInfo->isCardFull=0;
#endif
		if (localInfo->isDoingContShot) {
			pPhotoExeInfo->uiTakePicNum++;
			if (localInfo->isFolderFull || localInfo->isCardFull) {
				PhotoExe_StopContShot();
			}
		} else {
			localInfo->BDstatus = PhotoExe_CheckBD();
		}
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_JPGOK);
		break;

	case NVTEVT_ALGMSG_CAPFSTOK: {
			INT32  FSsts = paramArray[1];
			DBG_IND("[cb]NVTEVT_ALGMSG_CAPFSTOK\r\n");
			localInfo->IsCapFSTok = TRUE;
#if (FS_FUNC == ENABLE)
			localInfo->isFolderFull = UIStorageCheck(STORAGE_CHECK_FOLDER_FULL, NULL);
			localInfo->isCardFull = UIStorageCheck(STORAGE_CHECK_FULL, &localInfo->FreePicNum);
#else
			localInfo->isFolderFull=0;
			localInfo->isCardFull=0;
#endif
			if (FSsts != FST_STA_OK) {
				System_SetState(SYS_STATE_FS, FS_DISK_ERROR);
			}
			if (localInfo->isDoingContShot == TRUE) {
				if (localInfo->isFolderFull || localInfo->isCardFull) {
					PhotoExe_StopContShot();
				}
			}
			PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_FSTOK);
			#if (WIFI_AP_FUNC==ENABLE)
			WifiCmd_Done(WIFIFLAG_CAPTURE_DONE, E_OK);
			#endif
		}
		break;


	case NVTEVT_ALGMSG_CAPTUREEND:
		DBG_IND("[cb]NVTEVT_ALGMSG_CAPTUREEND \r\n");
		localInfo->isStartCapture = FALSE;
		localInfo->isStopingContShot = FALSE;
		//Disable USB detection
#if (USB_MODE==ENABLE)
		SxTimer_SetFuncActive(SX_TIMER_DET_USB_ID, TRUE);
#endif
#if _TODO
		//Disable USB detection
		SxTimer_SetFuncActive(SX_TIMER_DET_TV_ID, TRUE);
#endif
		//Disable Mode detection
		SxTimer_SetFuncActive(SX_TIMER_DET_MODE_ID, TRUE);
		//clear fd number
#if _FD_FUNC_
		FD_ClrRsltFaceNum();
#endif

		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_CAPTUREEND);
		localInfo->isDoingContShot = FALSE;
		DBG_IND("{OnCaptureEnd}\r\n");
		//exam_msg("{OnCaptureEnd}\r\n");
		//#if (WIFI_AP_FUNC==ENABLE)
		//WifiCmd_Done(WIFIFLAG_CAPTURE_DONE, E_OK);
		//#endif
		break;
	case NVTEVT_ALGMSG_SLOWSHUTTER:
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_SLOWSHUTTER);
		break;

	case NVTEVT_ALGMSG_PREVIEW_STABLE:
		DBG_IND("[cb]NVTEVT_ALGMSG_PREVIEW_STABLE \r\n");
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_PREVIEW_STABLE);
		break;

	default:

		break;
	}
    return NVTEVT_PASS;
}

INT32 PhotoExe_OnInitDateBuf(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	static BOOL bFirstEn=1;
	if (DATEIMPRINT_OFF != UI_GetData(FL_DATE_STAMP)) {
		if(bFirstEn){
			bFirstEn=0;
			UiDateImprint_InstallID();
		}
		UiDateImprint_InitBuff();
	}

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnGenDateStr(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if (DATEIMPRINT_OFF != UI_GetData(FL_DATE_STAMP)) {
		//UiDateImprint_UpdateDate();
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnGenDatePic(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	if (paramNum != 1) {
		//error parameter
		return NVTEVT_CONSUME;
	}
	if (DATEIMPRINT_OFF != UI_GetData(FL_DATE_STAMP)) {
		UiDateImprint_GenData((IMG_CAP_DATASTAMP_INFO *)paramArray[0]);
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnSharpness(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	DBG_IND("%d \r\n", uhSelect);
	UI_SetData(FL_SHARPNESS, uhSelect);
	PhotoExe_IQ_SetUIInfo(IQT_ITEM_SHARPNESS_LV, Get_SharpnessValue(uhSelect));
	Photo_SetUserIndex(PHOTO_USR_SHARPNESS, uhSelect);
	DBG_IND("photo sharpness %d\r\n", uhSelect);

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnSaturation(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	DBG_IND("%d \r\n", uhSelect);
	UI_SetData(FL_SATURATION, uhSelect);
	PhotoExe_IQ_SetUIInfo(IQT_ITEM_SATURATION_LV, Get_SaturationValue(uhSelect));
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnPlayShutterSound(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	GxSound_Stop();
	UISound_Play(DEMOSOUND_SOUND_SHUTTER_TONE);
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnVideoChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (PIP_VIEW_FUNC == ENABLE)
	PipView_SetStyle(UI_GetData(FL_DUAL_CAM));
#endif
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO) {
		Ux_SendEvent(0, NVTEVT_EXE_IMAGE_RATIO, 1, GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE)));
	}
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnDualcam(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (SENSOR_CAPS_COUNT == 2)
	UINT32 uhSelect;
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	} else {
		uhSelect = 0;
	}

	DBG_IND("%d \r\n", uhSelect);


	if (localInfo->sensorCount >= 2) {
		localInfo->DualCam = uhSelect;
	}
	UI_SetData(FL_DUAL_CAM, uhSelect);

	// also change the capture id
	//PhotoExe_OnCaptureID(pCtrl, paramNum, paramArray);
	//if (localInfo->DualCam != uhSelect)
#if (PIP_VIEW_FASTSWITCH==ENABLE)
	{
		Ux_SendEvent(0, NVTEVT_SENSOR_DISPLAY, 1, (SENSOR_1 | SENSOR_2)); //for Always trigger PIP View
		//#NT#2015/11/25#Niven Cho#[87393] -begin
		//Here be invoked at startup without in any mode
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO) {
			Ux_SendEvent(0, NVTEVT_EXE_IMAGE_RATIO, 1, GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE)));
		}
		PipView_SetStyle(UI_GetData(FL_DUAL_CAM));
		//#NT#2015/11/25#Niven Cho -end
	}
#else
	{
		// set display display
		if (uhSelect == DUALCAM_FRONT) {
			Ux_SendEvent(0, NVTEVT_SENSOR_DISPLAY, 1, SENSOR_1);
		}
		if (uhSelect == DUALCAM_BEHIND) {
			Ux_SendEvent(0, NVTEVT_SENSOR_DISPLAY, 1, SENSOR_2);
		}
		if (uhSelect == DUALCAM_BOTH) {
			Ux_SendEvent(0, NVTEVT_SENSOR_DISPLAY, 1, (SENSOR_1 | SENSOR_2));
		}
	}
#endif //(PIP_VIEW_FASTSWITCH==ENABLE)

#endif
    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnFDEnd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_IND("[cb]\r\n");

	if ((!localInfo->isStartCapture) && (UI_GetData(FL_FD) != FD_OFF)) {
		//Flush FD event before draw
		Ux_FlushEventByRange(NVTEVT_EXE_FDEND, NVTEVT_EXE_FDEND);
		PhotoExe_CallBackUpdateInfo(UIAPPPHOTO_CB_FDEND);
	}

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnSHDR(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_SHDR, uhSelect);

#if SHDR_FUNC
	PhotoExe_SHDR_SetUIInfo(HD_VIDEOPROC_FUNC_SHDR, uhSelect);

	if (UI_GetData(FL_SHDR) == SHDR_ON) {
		UI_SetData(FL_WDR, WDR_OFF);
		UI_SetData(FL_RSC, RSC_OFF);
	}
#endif

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnWDR(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_WDR, uhSelect);
#if WDR_FUNC
	if (UI_GetData(FL_WDR) == WDR_ON && UI_GetData(FL_SHDR) == SHDR_ON) {
		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_SHDR, 1, SHDR_OFF);
	} else {
		PhotoExe_IQ_SetUIInfo(IQT_ITEM_WDR_PARAM, uhSelect);
	}
#endif
	DBG_IND("photo wdr %d\r\n", uhSelect);

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnNR(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uiSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum) {
		uiSelect = paramArray[0];
	}

	UI_SetData(FL_NR, uiSelect);
    PhotoExe_IQ_SetUIInfo(IQT_ITEM_NR_LV, Get_NRValue(uiSelect));

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnFocusEnd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{

    return NVTEVT_CONSUME;
}

INT32 PhotoExe_OnDefog(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	DBG_IND("%d \r\n", paramArray[0]);
	if (paramNum > 0) {
		uhSelect = paramArray[0];
	}

	UI_SetData(FL_DEFOG, uhSelect);

#if DEFOG_FUNC
	PhotoExe_IQ_SetUIInfo(IQT_ITEM_DEFOG_PARAM, uhSelect);
#endif
	DBG_IND("photo DEFOG %d\r\n", uhSelect);
    return NVTEVT_CONSUME;
}
#if (SENSOR_INSERT_FUNCTION == ENABLE)
HD_RESULT PhotoExe_DetSensor(BOOL *plug)
{
	HD_RESULT result;
	HD_PATH_ID path_id = 0;

	#if (SENSOR_INSERT_MASK == SENSOR_1)
	path_id = ImageApp_Photo_GetConfig(PHOTO_CFG_VCAP_CTRLPORT, PHOTO_VID_IN_1);
	#elif (SENSOR_INSERT_MASK == SENSOR_2)
	path_id = ImageApp_Photo_GetConfig(PHOTO_CFG_VCAP_CTRLPORT, PHOTO_VID_IN_2);
	#endif
	result = vendor_videocap_get(path_id, VENDOR_VIDEOCAP_PARAM_GET_PLUG, plug);

	return result;
}
#endif


static void PhotoExe_2sensor_HotPlug_Disp(void)
{
	UINT32 u32CurrSensorEn = System_GetEnableSensor();
	UINT32 u32PrevSensorEn = System_GetPrevEnableSensor();
	UINT32 u32Mask;
	UINT32 i;

	DBG_DUMP("^M%s: u32CurrSensorEn = 0x%x\r\n", __func__, u32CurrSensorEn);
	DBG_DUMP("^M%s: u32PrevSensorEn = 0x%x\r\n", __func__, u32PrevSensorEn);

	u32Mask = 1;
	for (i=PHOTO_DISP_ID_MIN; i< (PHOTO_DISP_ID_MIN+SENSOR_CAPS_COUNT); i++){
		if ((u32PrevSensorEn & u32Mask) != (u32CurrSensorEn & u32Mask)){
			if (u32CurrSensorEn & u32Mask){
				ImageApp_Photo_UpdateImgLinkForDisp(i, ENABLE, TRUE);
			}else{
				ImageApp_Photo_UpdateImgLinkForDisp(i, DISABLE, TRUE);
			}
		}

		u32Mask <<= 1;
	}//for (i=0; i<SENSOR_CAPS_COUNT; i++)

}

static void PhotoExe_2sensor_HotPlug_WiFi(void)
{
	UINT32 u32CurrSensorEn = System_GetEnableSensor();
	UINT32 u32PrevSensorEn = System_GetPrevEnableSensor();
	UINT32 u32Mask;
	UINT32 i;

	DBG_DUMP("^M%s: u32CurrSensorEn = 0x%x\r\n", __func__, u32CurrSensorEn);
	DBG_DUMP("^M%s: u32PrevSensorEn = 0x%x\r\n", __func__, u32PrevSensorEn);

	u32Mask = 1;
	for (i=PHOTO_STRM_ID_MIN; i<(PHOTO_STRM_ID_MIN+SENSOR_CAPS_COUNT); i++){
		if ((u32PrevSensorEn & u32Mask) != (u32CurrSensorEn & u32Mask)){
			if (u32CurrSensorEn & u32Mask){
				if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_WIFI){
					ImageApp_Photo_UpdateImgLinkForStrm(i, ENABLE, TRUE);
				}
			}else{
				ImageApp_Photo_UpdateImgLinkForStrm(i, DISABLE, TRUE);
			}
		}

		u32Mask <<= 1;
	}//for (i=0; i<SENSOR_CAPS_COUNT; i++)

}
INT32 PhotoExe_OnSensorHotPlug(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	PhotoExe_2sensor_HotPlug_Disp();

	PhotoExe_2sensor_HotPlug_WiFi();

    return NVTEVT_CONSUME;
}

BOOL FlowPhoto_CheckReOpenItem(void)
{
	BOOL bReOpen = FALSE;

#if SHDR_FUNC
	DBG_DUMP("UI_GetData(FL_SHDR_MENU)=%d,  UI_GetData(FL_SHDR)=%d\r\n",UI_GetData(FL_SHDR_MENU), UI_GetData(FL_SHDR));
	if (UI_GetData(FL_SHDR_MENU) != UI_GetData(FL_SHDR)) {
		if (UI_GetData(FL_SHDR_MENU) == SHDR_ON) {
			UI_SetData(FL_WDR, WDR_OFF);
			UI_SetData(FL_WDR_MENU, WDR_OFF);
			UI_SetData(FL_RSC, RSC_OFF);
			UI_SetData(FL_RSC_MENU, RSC_OFF);
		}
		UI_SetData(FL_SHDR, UI_GetData(FL_SHDR_MENU));
		bReOpen = TRUE;
	}
#endif

#if WDR_FUNC
	if (UI_GetData(FL_WDR_MENU) != UI_GetData(FL_WDR)) {
		if (UI_GetData(FL_WDR_MENU) == WDR_ON) {
			UI_SetData(FL_SHDR, SHDR_OFF);
			UI_SetData(FL_SHDR_MENU, SHDR_OFF);
		}
		UI_SetData(FL_WDR, UI_GetData(FL_WDR_MENU));
		bReOpen = TRUE;
	}
#endif

	if (UI_GetData(FL_RSC_MENU) != UI_GetData(FL_RSC)) {
		if (UI_GetData(FL_RSC_MENU) == RSC_ON) {
			UI_SetData(FL_SHDR, SHDR_OFF);
			UI_SetData(FL_SHDR_MENU, SHDR_OFF);
		}
		UI_SetData(FL_RSC, UI_GetData(FL_RSC_MENU));
		bReOpen = TRUE;
	}
#if DEFOG_FUNC
	if (UI_GetData(FL_DEFOG_MENU) != UI_GetData(FL_DEFOG)) {
		UI_SetData(FL_DEFOG, UI_GetData(FL_DEFOG_MENU));
		bReOpen = TRUE;
	}
#endif

	return bReOpen;
}


#if PHOTO_PREVIEW_SLICE_ENC_FUNC == ENABLE

static UINT32 PhotoExe_PHY2VIRT(UINT32 pa_pos, UINT32 pa_start, UINT32 va)
{
	if(pa_pos < pa_start){

		DBG_ERR("pa_pos(%lx) must greater than pa_start(%lx)!\r\n", pa_pos, pa_start);
		return 0;
	}
	else if(va == 0 || pa_start == 0 || pa_pos == 0){
		DBG_ERR("va & pa & pa_start can't be zero!\r\n");
		return 0;
	}

	return (va + (pa_pos - pa_start));
}

static INT32 PhotoExe_Preview_SliceEncode_Open(void)
{
	HD_RESULT ret = HD_OK;
	PHOTO_VID_IN vid_in = 0;
	PhotoExe_SliceSize_Info dst_slice_info = {0};
	HD_DIM dim = {0};
	UINT32 bitrate = 0;
	HD_PATH_ID vproc_path_id = 0;
	HD_PATH_ID venc_path_id = 0;
	HD_VIDEOPROC_OUT vproc_out = {0};
	HD_VIDEO_PXLFMT vproc_out_pxlfmt = 0;
	UINT8 first_out_port = 2; /* 0 & 1 used by ImageApp_Photo, encode path can be shared by different sensor */
	HD_IN_ID in = HD_VIDEOENC_IN(0, first_out_port);
	HD_OUT_ID out = HD_VIDEOENC_OUT(0, first_out_port);

#if HEIC_FUNC == ENABLE
	HD_PATH_ID venc_path_id_heic = 0;
#endif

	/* calculate dst slice info */
	if(PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Info(&dst_slice_info) != E_OK){
		goto EXIT;
	}

	for (vid_in = 0; vid_in < SENSOR_CAPS_COUNT; vid_in++) {

		/* 3dnr ref path output sensor size frame */
		ImageApp_Photo_Get_Hdal_Path(vid_in, PHOTO_HDAL_VPRC_3DNR_REF_PATH, (UINT32 *)&vproc_path_id);

		/* get vproc out frame format ( = venc in) */
		ret = hd_videoproc_get(vproc_path_id, HD_VIDEOPROC_PARAM_OUT, (VOID*)&vproc_out);
		if(ret != HD_OK){
			DBG_ERR("hd_videoproc_get HD_VIDEOPROC_PARAM_OUT failed(path_id=%lx, ret=%d)!", vproc_path_id, ret);
			goto EXIT;
		}

		vproc_out_pxlfmt = vproc_out.pxlfmt;
	}

	in = HD_VIDEOENC_IN(0, first_out_port);
	out = HD_VIDEOENC_OUT(0, first_out_port);
	ret = hd_videoenc_open(in, out, &venc_path_id);
	if (ret != HD_OK ){
		DBG_ERR("hd_videoenc_open(%lx, %lx) failed(%d)!\r\n", in, out, ret);
		goto EXIT;
	}

#if HEIC_FUNC == ENABLE

	in = HD_VIDEOENC_IN(0, first_out_port + 1);
	out = HD_VIDEOENC_OUT(0, first_out_port + 1);
	ret = hd_videoenc_open(in, out, &venc_path_id_heic);
	if (ret != HD_OK ){
		DBG_ERR("hd_videoenc_open(%lx, %lx) failed(%d)!\r\n", in, out, ret);
		goto EXIT;
	}

#endif

	for(int i=0 ; i<PHOTO_ENC_JPG_TYPE_MAX_ID ; i++)
	{
		PhotoExe_SliceEncode_Info* info = PhotoExe_Preview_SliceEncode_Get_Info(i);

		switch(i)
		{
		case PHOTO_ENC_JPG_PRIMARY:
		{
			PhotoExe_SliceSize_Info max_info;

			info->enc_path_id = venc_path_id;

#if HEIC_FUNC == ENABLE
			info->enc_path_id_heic = venc_path_id_heic;
#endif

			/* calculate max buffer for slice */
			info->yuv_buf_mem_info.blk_size =
					PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Buffer_Size(vproc_out_pxlfmt);

			if(PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Info(&max_info) != E_OK)
				goto EXIT;

			bitrate = PhotoExe_Preview_SliceEncode_Get_Encode_Max_Bitrate(vproc_out_pxlfmt);
			if(bitrate == 0){
				DBG_ERR("Calculate bitrate error!\r\n");
				goto EXIT;
			}

			dim = (HD_DIM){max_info.width, max_info.slice_height};

			PhotoExe_Preview_SliceEncode_Encode_Config_Path(info->enc_path_id, dim, bitrate * 2);

			/* To avoid mem fragment, start venc firtst. (hdal vdoenc.o[2].max / vdoenc.o[2].que / vdoenc.ctrl.bs)*/
			{
				PhotoExe_Preview_SliceEncode_Encode_Set_In(info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {max_info.width, max_info.height});
				PhotoExe_Preview_SliceEncode_Encode_Set_Out(info->enc_path_id, CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_PRIMARY);
				hd_videoenc_start(info->enc_path_id);
				hd_videoenc_stop(info->enc_path_id);
			}

#if HEIC_FUNC == ENABLE

			dim = (HD_DIM){CFG_JPG_PREVIEW_SLICE_ENC_HEIC_WIDTH, CFG_JPG_PREVIEW_SLICE_ENC_HEIC_HEIGHT};

			PhotoExe_Preview_SliceEncode_Encode_Config_Path_HEIC(info->enc_path_id_heic, dim, CFG_JPG_PREVIEW_SLICE_ENC_HEIC_BITRATE*2);
#endif

			break;
		}

		case PHOTO_ENC_JPG_SCREENNAIL:
		{
			info->enc_path_id = venc_path_id;
			info->yuv_buf_mem_info.blk_size = VDO_YUV_BUFSIZE(CFG_SCREENNAIL_W, CFG_SCREENNAIL_H, vproc_out_pxlfmt);

			dim = (HD_DIM){CFG_SCREENNAIL_W, CFG_SCREENNAIL_H};
			bitrate = SCREENNAIL_TARGETBYTERATE * 8;

			break;
		}

		case PHOTO_ENC_JPG_THUMBNAIL:
		{

			info->enc_path_id = venc_path_id;
			info->yuv_buf_mem_info.blk_size = VDO_YUV_BUFSIZE(CFG_THUMBNAIL_W, CFG_THUMBNAIL_H, vproc_out_pxlfmt);

			dim = (HD_DIM){CFG_THUMBNAIL_W, CFG_THUMBNAIL_H};
			bitrate = THUMBNAIL_TARGETBYTERATE * 8;

			break;
		}
		}

		PhotoExe_Preview_SliceEncode_Get_Comm_Buffer(&info->yuv_buf_mem_info);
	}

EXIT:

	return (ret == HD_OK) ? E_OK : E_SYS;
}

static INT32 PhotoExe_Preview_SliceEncode_Close(void)
{
	HD_RESULT ret = HD_OK;
	HD_PATH_ID path_id = 0;

	for(int i=0 ; i<PHOTO_ENC_JPG_TYPE_MAX_ID ; i++)
	{
		PhotoExe_SliceEncode_Info* info = PhotoExe_Preview_SliceEncode_Get_Info(i);

#if HEIC_FUNC == ENABLE
		if((i == PHOTO_ENC_JPG_PRIMARY) && info->enc_path_id_heic){
			ret = hd_videoenc_close(info->enc_path_id_heic);
			if (ret != HD_OK){
				DBG_ERR("hd_videoenc_close failed(%d)!\r\n",ret);
			}
		}
#endif

		if(path_id == 0 || path_id != info->enc_path_id){

			ret = hd_videoenc_close(info->enc_path_id);
			if (ret != HD_OK){
				DBG_ERR("hd_videoenc_close failed(%d)!\r\n",ret);
			}

			path_id = info->enc_path_id;
		}

		if(info->yuv_buf_mem_info.va){
			PhotoExe_Preview_SliceEncode_Release_Comm_Buffer(info->yuv_buf_mem_info);
			memset(&info->yuv_buf_mem_info, 0, sizeof(PhotoExe_MEM_Info));
		}
	}

	return (ret == HD_OK) ? E_OK : E_SYS;
}

static void PhotoExe_Preview_SliceEncode_Get_Src_Slice_Info(
		const PhotoExe_SliceSize_Info *dst_info,
		PhotoExe_SliceSize_Info *src_info,
		const HD_VIDEO_FRAME video_frame)
{
	src_info->slice_num = dst_info->slice_num;
	src_info->width = video_frame.dim.w;
	src_info->height = video_frame.dim.h;

	if(src_info->slice_num > 1){
		src_info->slice_height = ALIGN_CEIL((src_info->height) / src_info->slice_num, 2);
		src_info->last_slice_height = ALIGN_CEIL(src_info->height - src_info->slice_height * (src_info->slice_num - 1), 2);
	}
	else{
		src_info->slice_height =  video_frame.dim.h;
		src_info->last_slice_height = src_info->slice_height;
	}

	if(src_info->slice_height * (src_info->slice_num - 1) >= src_info->height){
		DBG_ERR("aligned total slice height(%lu * %lu + %lu = %lu) exceed photo height(%lu)!\n",
				src_info->slice_height,
				src_info->slice_num - 1,
				src_info->last_slice_height,
				src_info->height);
	}

	PHOTO_SLICE_ENC_DUMP("Src Slice Info: size = {%lu,%lu} slice num = {%lu} slice height = {%lu} last slice height = {%lu}\r\n",
			src_info->width, src_info->height, src_info->slice_num, src_info->slice_height, src_info->last_slice_height);
}

static PhotoExe_SliceEncode_Info* PhotoExe_Preview_SliceEncode_Get_Info(const PHOTO_ENC_JPG_TYPE type)
{
	static PhotoExe_SliceEncode_Info info[PHOTO_ENC_JPG_TYPE_MAX_ID] = {0};

	return &info[type];
}

static INT32 PhotoExe_Preview_SliceEncode_Get_Dst_Slice_Info(PhotoExe_SliceSize_Info *info, UINT32 cap_size_w, UINT32 cap_size_h, UINT32 slice_num)
{
	info->width = cap_size_w;
	info->height = cap_size_h;
	info->slice_num = slice_num;

	if(info->height == 1080 && slice_num == 1){
		info->slice_height = info->height;
	}
	else{
		info->slice_height = ALIGN_CEIL_16(info->height / info->slice_num);
	}

	if(info->slice_height * (info->slice_num - 1) > info->height){

		DBG_ERR("calculate last slice height error! (slice height = %lu , slice num = %lu , photo height = %lu)\n", info->slice_height, info->slice_num, info->height);
		return E_SYS;
	}

	info->last_slice_height = info->height  - (info->slice_height * (info->slice_num - 1));

	PHOTO_SLICE_ENC_DUMP("Dst Slice Info: size = {%lu,%lu} slice num = {%lu} slice height = {%lu} last slice height = {%lu}\r\n",
			info->width, info->height, info->slice_num, info->slice_height, info->last_slice_height);

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Get_Curr_Dst_Slice_Info(PhotoExe_SliceSize_Info *info, const HD_VIDEO_FRAME src_frame)
{
	UINT32 cap_size_w = GetPhotoSizeWidth(SysGetFlag(FL_PHOTO_SIZE));
	UINT32 cap_size_h = GetPhotoSizeHeight(SysGetFlag(FL_PHOTO_SIZE));
	UINT32 max_slice_num;
	UINT32 slice_num;

	unsigned int cap_size = cap_size_w * cap_size_h;
	unsigned int buf_size = CFG_JPG_PREVIEW_SLICE_ENC_PRIMARY_BUF_WIDTH * CFG_JPG_PREVIEW_SLICE_ENC_PRIMARY_BUF_HEIGHT;

	if(buf_size > cap_size){
		max_slice_num = 1;
		slice_num = 1;
	}
	else{
		PHOTO_SLICE_ENC_DUMP("cap_size = %lu buf_size = %lu\n", cap_size, buf_size);

		UINT32 lines = (buf_size / cap_size_w);
		UINT32 slice_height = ALIGN_CEIL(lines, 16);
		max_slice_num = (cap_size_h / slice_height) + (cap_size_h % slice_height ? 1 : 0);

		UINT32 tmp_src_h = src_frame.dim.h / 2;
		UINT32 tmp_dst_h = cap_size_h / 16;
		UINT32 found_common_factor = 0;
		UINT32 i;

	    for (i = 1; i <= (tmp_src_h > tmp_dst_h ? tmp_dst_h : tmp_src_h) ; ++i)
	    {
	        if (tmp_src_h % i == 0 && tmp_dst_h % i == 0)
	        {
	        	PHOTO_SLICE_ENC_DUMP("common factor = %d\n", i);

		        if(max_slice_num < i){
		        	found_common_factor = i;
		        	break;
		        }
	        }
	    }

	    if(found_common_factor){
	    	slice_num = found_common_factor;
	    	PHOTO_SLICE_ENC_DUMP("use common factor %d\n ", slice_num);
	    }
	    else{
	    	slice_num = max_slice_num;
	    	PHOTO_SLICE_ENC_DUMP("use max slice num %d\n ", slice_num);
	    }
	}

	return PhotoExe_Preview_SliceEncode_Get_Dst_Slice_Info(info, cap_size_w, cap_size_h, slice_num);
}

static UINT32 PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Buffer_Size(HD_VIDEO_PXLFMT pxl_fmt)
{
	HD_DIM dim = PhotoExe_Preview_SliceEncode_Get_Encode_Max_Size();
	UINT32 reserved_buffer = 0;
	UINT32 max_buffer_size = VDO_YUV_BUFSIZE(ALIGN_CEIL(dim.w, 16), ALIGN_CEIL(dim.h, 16), pxl_fmt) + reserved_buffer;

	return max_buffer_size;
}

static INT32 PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Info(PhotoExe_SliceSize_Info *info)
{
	static PhotoExe_SliceSize_Info max_slice_info = {0};
	UINT32 max_slice_num;
	UINT32 slice_num;
	INT32 ret = E_OK;
	UIAPP_PHOTO_SENSOR_INFO *sensor_info = UIAppPhoto_get_SensorInfo(0);
	unsigned int buf_size = CFG_JPG_PREVIEW_SLICE_ENC_PRIMARY_BUF_WIDTH * CFG_JPG_PREVIEW_SLICE_ENC_PRIMARY_BUF_HEIGHT;

	/* search max slice (slice height * slice width) */
	if(!max_slice_info.slice_height || !max_slice_info.width){

		UINT8 cap_idx = 0;

		for(cap_idx = PHOTO_MAX_CAP_SIZE ; cap_idx <= PHOTO_MIN_CAP_SIZE ; cap_idx++)
		{
			HD_DIM tmp_cap_size = {0};
			PhotoExe_SliceSize_Info tmp_slice_info = {0};

			tmp_cap_size = (HD_DIM){GetPhotoSizeWidth(cap_idx),  GetPhotoSizeHeight(cap_idx)};

			if(buf_size > (tmp_cap_size.w * tmp_cap_size.h)){
				max_slice_num = 1;
				slice_num = 1;
			}
			else{
				PHOTO_SLICE_ENC_DUMP("cap_size = %lu buf_size = %lu\n", tmp_cap_size.w * tmp_cap_size.h, buf_size);

				UINT32 lines = (buf_size / tmp_cap_size.w);
				UINT32 slice_height = ALIGN_CEIL(lines, 16);
				max_slice_num = (tmp_cap_size.h / slice_height) + (tmp_cap_size.h % slice_height ? 1 : 0);

				UINT32 tmp_src_h = sensor_info->sSize.h / 2;
				UINT32 tmp_dst_h = tmp_cap_size.h / 16;
				UINT32 found_common_factor = 0;
				UINT32 i;

			    for (i = 1; i <= (tmp_src_h > tmp_dst_h ? tmp_dst_h : tmp_src_h) ; ++i)
			    {
			        if (tmp_src_h % i == 0 && tmp_dst_h % i == 0)
			        {
			        	PHOTO_SLICE_ENC_DUMP("common factor = %d\n", i);

				        if(max_slice_num < i){
				        	found_common_factor = i;
				        	break;
				        }
			        }
			    }

			    if(found_common_factor){
			    	slice_num = found_common_factor;
			    	PHOTO_SLICE_ENC_DUMP("use common factor %d\n ", slice_num);
			    }
			    else{
			    	slice_num = max_slice_num;
			    	PHOTO_SLICE_ENC_DUMP("use max slice num %d\n ", slice_num);
			    }
			}

			ret = PhotoExe_Preview_SliceEncode_Get_Dst_Slice_Info(&tmp_slice_info, tmp_cap_size.w, tmp_cap_size.h, slice_num);

			if((tmp_slice_info.slice_height * tmp_slice_info.width) > (max_slice_info.slice_height * max_slice_info.width)){
				max_slice_info = tmp_slice_info;
			}
		}

		PHOTO_SLICE_ENC_DUMP("max dst slice = {%lu, %lu}, cap idx = %lu\n", max_slice_info.width, max_slice_info.slice_height, cap_idx);
	}

	*info = max_slice_info;

	return ret;
}

static HD_DIM PhotoExe_Preview_SliceEncode_Get_Encode_Max_Size()
{
	PhotoExe_SliceSize_Info info;

	if(PhotoExe_Preview_SliceEncode_Get_Max_Dst_Slice_Info(&info) != E_OK)
		return (HD_DIM){0, 0};

	HD_DIM dim_max_slice = {info.width, info.slice_height};

	return dim_max_slice;
}

static UINT32 PhotoExe_Preview_SliceEncode_Get_Encode_Max_Bitrate(HD_VIDEO_PXLFMT vproc_out_pxlfmt)
{
	const UINT32 ratio = CFG_JPG_PREVIEW_SLICE_ENC_BS_BUF_RATIO;
	UINT32 bitrate;
	HD_DIM dim = PhotoExe_Preview_SliceEncode_Get_Encode_Max_Size();

	if(dim.w == 0 || dim.h == 0){
		DBG_ERR("can't get max size\r\n");
		return 0;
	}

	bitrate = (dim.w * dim.h * HD_VIDEO_PXLFMT_BPP(vproc_out_pxlfmt) / ratio);

	PHOTO_SLICE_ENC_DUMP("vproc_out_pxlfmt = {%lx}, dim = {%lu, %lu}, ratio = {%lu}, bitrate = {%lu}\r\n", vproc_out_pxlfmt, dim.w, dim.h, ratio, bitrate);
	return bitrate;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Config_Path(HD_PATH_ID enc_path, HD_DIM max_dim, UINT32 bitrate)
{
	HD_RESULT ret;
	HD_VIDEOENC_PATH_CONFIG video_enc_path_config = {0};

	video_enc_path_config.max_mem.codec_type = HD_CODEC_TYPE_JPEG;
	video_enc_path_config.max_mem.max_dim  = max_dim;
	video_enc_path_config.max_mem.bitrate	 = bitrate;
	video_enc_path_config.max_mem.enc_buf_ms = 1500;
	video_enc_path_config.max_mem.svc_layer  = HD_SVC_DISABLE;
	video_enc_path_config.max_mem.ltr		 = FALSE;
	video_enc_path_config.max_mem.rotate	 = FALSE;
	video_enc_path_config.max_mem.source_output   = FALSE;
	video_enc_path_config.isp_id			 = 0;

	PHOTO_SLICE_ENC_DUMP("hd_videoenc_set HD_VIDEOENC_PARAM_PATH_CONFIG max_dim={%lu, %lu} bitrate={%lu}\r\n", max_dim, bitrate);


	ret = hd_videoenc_set(enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_enc_path_config);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_set HD_VIDEOENC_PARAM_PATH_CONFIG failed(%d)\r\n", ret);
	}

	return (ret == HD_OK) ? E_OK : E_SYS;
}

/**************************************************************************************************
 * HEIC
**************************************************************************************************/
#if HEIC_FUNC == ENABLE
static INT32 PhotoExe_Preview_SliceEncode_Encode_Config_Path_HEIC(HD_PATH_ID enc_path, HD_DIM max_dim, UINT32 bitrate)
{
	HD_RESULT ret;
	HD_VIDEOENC_PATH_CONFIG video_enc_path_config = {0};

	video_enc_path_config.max_mem.codec_type = HD_CODEC_TYPE_H265;
	video_enc_path_config.max_mem.max_dim  = max_dim;
	video_enc_path_config.max_mem.bitrate	 = bitrate;
	video_enc_path_config.max_mem.enc_buf_ms = 1500;
	video_enc_path_config.max_mem.svc_layer  = HD_SVC_DISABLE;
	video_enc_path_config.max_mem.ltr		 = FALSE;
	video_enc_path_config.max_mem.rotate	 = FALSE;
	video_enc_path_config.max_mem.source_output   = FALSE;
	video_enc_path_config.isp_id			 = 0;

	PHOTO_SLICE_ENC_DUMP("hd_videoenc_set HD_VIDEOENC_PARAM_PATH_CONFIG max_dim={%lu, %lu} bitrate={%lu}\r\n", max_dim, bitrate);


	ret = hd_videoenc_set(enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_enc_path_config);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_set HD_VIDEOENC_PARAM_PATH_CONFIG failed(%d)\r\n", ret);
	}

	return (ret == HD_OK) ? E_OK : E_SYS;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Set_Out_HEIC(
		const HD_PATH_ID enc_path_id)
{
	HD_RESULT 		ret;
	HD_VIDEOENC_OUT2 enc_out = {0};
	VENDOR_VIDEOENC_LONG_START_CODE lsc = {0};
	VENDOR_VIDEOENC_QUALITY_BASE_MODE qbm = {0};
	HD_H26XENC_RATE_CONTROL2 rc_param = {0};

    enc_out.codec_type                 = HD_CODEC_TYPE_H265;
    enc_out.h26x.gop_num               = 1;
    enc_out.h26x.source_output         = DISABLE;
    enc_out.h26x.profile               = HD_H265E_MAIN_PROFILE;
    enc_out.h26x.level_idc             = HD_H265E_LEVEL_5;
    enc_out.h26x.svc_layer             = HD_SVC_DISABLE;
    enc_out.h26x.entropy_mode          = HD_H265E_CABAC_CODING;
    PHOTO_SLICE_ENC_DUMP("hd_videoenc_set HD_VIDEOENC_PARAM_OUT_ENC_PARAM path_id={%lx} image_quality={%lu}\r\n", enc_path_id, enc_out.jpeg.image_quality);
    ret = hd_videoenc_set(enc_path_id, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &enc_out);
    if (ret != HD_OK) {
		DBG_ERR("set_enc_param_out = %d\r\n", ret);
		return E_SYS;
    }

	rc_param.rc_mode                     = HD_RC_MODE_CBR;
	rc_param.cbr.bitrate                 = CFG_JPG_PREVIEW_SLICE_ENC_HEIC_BITRATE;
	rc_param.cbr.frame_rate_base         = 1;
	rc_param.cbr.frame_rate_incr         = 1;
	rc_param.cbr.init_i_qp               = CFG_JPG_PREVIEW_SLICE_ENC_HEIC_INIT_QP;
	rc_param.cbr.max_i_qp                = 45;
	rc_param.cbr.min_i_qp                = 10;
	rc_param.cbr.init_p_qp               = CFG_JPG_PREVIEW_SLICE_ENC_HEIC_INIT_QP;
	rc_param.cbr.max_p_qp                = 45;
	rc_param.cbr.min_p_qp                = 10;
	rc_param.cbr.static_time             = 4;
	rc_param.cbr.ip_weight               = 0;
	rc_param.cbr.key_p_period            = 0;
	rc_param.cbr.kp_weight               = 0;
	rc_param.cbr.p2_weight               = 0;
	rc_param.cbr.p3_weight               = 0;
	rc_param.cbr.lt_weight               =  0;
	rc_param.cbr.motion_aq_str           = 0;
	rc_param.cbr.max_frame_size          = 0;
	ret = hd_videoenc_set(enc_path_id, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
	if (ret != HD_OK) {
		DBG_ERR("set HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2 fail(%d)\r\n", ret);
		return E_SYS;
	}

	lsc.long_start_code_en = ENABLE;
	ret = vendor_videoenc_set(enc_path_id, VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE, &lsc);
	if (ret != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE) fail(%d)\n", ret);
		return E_SYS;
	}

	qbm.quality_base_en = TRUE;
	ret = vendor_videoenc_set(enc_path_id, VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE, &qbm);
	if (ret != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE) fail(%d)\n", ret);
		return E_SYS;
	}

	{
		HD_H26XENC_VUI vui_param = {0};

		ret = hd_videoenc_get(enc_path_id, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
		if (ret != HD_OK) {
			DBG_ERR("hd_videoenc_get(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", ret);
			return E_SYS;
		}

		vui_param.vui_en = TRUE;
		vui_param.color_range = 1; /* full range, makes color transform like jpg */

		if ((ret = hd_videoenc_set(enc_path_id, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param)) != HD_OK) {
			DBG_ERR("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", ret);
		}
	}

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Naming_HEIC(char* file_path)
{


#if HEIC_DCF_SUPPORT == ENABLE
	UINT32 nextFolderID = 0, nextFileID = 0;

	DCF_GetNextID(&nextFolderID,&nextFileID);
	DCF_MakeObjPath(nextFolderID, nextFileID, HEIC_DCF_FILE_TYPE, file_path);
	DCF_AddDBfile(file_path);
	DBG_DUMP("%s added to DCF\r\n", file_path);

#else

	char* last_jpg_filepath = ImageApp_Photo_GetLastWriteFilePath();
	char* pos = strstr(last_jpg_filepath, ".");

	if(pos){
		strncpy(file_path, last_jpg_filepath, (pos - last_jpg_filepath) + 1);
	}
	else{
		DBG_ERR("basename of [%s] not found\n", last_jpg_filepath);
		return E_SYS;
	}

	strncat(file_path, HEIC_TMP_EXT_NAME, strlen(HEIC_TMP_EXT_NAME));

#endif

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_WriteFile_HEIC(HD_VIDEOENC_BS *bs_data, char* file_path)
{
	char filename[80];
	UINT32 i, j;

	strncpy(filename, HEIC_STORAGE_ROOT, sizeof(filename)-1);
	i = 3;
	j = strlen(filename);
	while(file_path[i]) {
		filename[j] = (file_path[i] == '\\') ? '/' : file_path[i];
		i++;
		j++;
	}

	if(heif_create_heic(bs_data, filename) != TRUE){
		DBG_ERR("create heic file failed!\n");
		return E_SYS;
	}

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Primary_HEIC(
		HD_VIDEO_FRAME* video_frame,
		PhotoExe_MEM_Info* heic_bs_buf_mem_info,
		HD_VIDEOENC_BS* heic_bs_data)
{
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_PRIMARY);
	HD_PATH_ID enc_path_id_heic = slice_enc_info->enc_path_id_heic;
	HD_VIDEOENC_BS bs_data_pull = {0};
	HD_RESULT ret = HD_OK;

	if ((ret = hd_videoenc_start(enc_path_id_heic)) != HD_OK) {
		DBG_ERR("hd_videoenc_start failed!(%d)\n", ret);
 		goto EXIT;
	}

	ret = hd_videoenc_push_in_buf(enc_path_id_heic, video_frame, NULL, -1); // -1 = blocking mode
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_push_in_buf failed!(%d)\r\n", ret);
		goto EXIT;
	}

	ret = hd_videoenc_pull_out_buf(enc_path_id_heic, &bs_data_pull, -1); // -1 = blocking mode
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_pull_out_buf failed!(%d)\r\n", ret);
		goto EXIT;
	}

	heic_bs_buf_mem_info->blk_size = bs_data_pull.video_pack[0].size; /* I frame */
	heic_bs_buf_mem_info->used_size = bs_data_pull.video_pack[0].size;

	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(heic_bs_buf_mem_info, "slice_enc_heic") != E_OK){
		goto EXIT;
	}

	/* copy heic bs data to another buffer */
	PhotoExe_MEM_Info heci_internal_mem_info = {0};

	*heic_bs_data = bs_data_pull;
	heic_bs_data->video_pack[0].phy_addr = heic_bs_buf_mem_info->pa;

	PhotoExe_Preview_SliceEncode_Get_Enc_Buffer_Info(enc_path_id_heic, &heci_internal_mem_info);

	memcpy((void*)heic_bs_buf_mem_info->va, (void*)heci_internal_mem_info.va, heic_bs_buf_mem_info->blk_size);
	hd_common_mem_munmap((void*)heci_internal_mem_info.va, heci_internal_mem_info.blk_size);

EXIT:

	if(bs_data_pull.pack_num){
		ret = hd_videoenc_release_out_buf(enc_path_id_heic, &bs_data_pull);
		if(ret != HD_OK){
			DBG_ERR("hd_videoenc_release_out_buf failed(%d)\n", ret);
		}
	}

	if ((ret = hd_videoenc_stop(enc_path_id_heic)) != HD_OK) {
		DBG_ERR("hd_videoenc_start failed!(%d)\n", ret);
	}

	return (ret == HD_OK) ? E_OK : E_SYS;
}
#endif
/**************************************************************************************************
 * HEIC End
**************************************************************************************************/

static INT32 PhotoExe_Preview_SliceEncode_Encode_Set_Out(
		const HD_PATH_ID enc_path_id,
		const UINT32 quality)
{
	HD_RESULT 		ret;
    HD_VIDEOENC_OUT enc_out = {0};

	enc_out.codec_type		   = HD_CODEC_TYPE_JPEG;
	enc_out.jpeg.retstart_interval = 0;
	enc_out.jpeg.image_quality = quality;

	PHOTO_SLICE_ENC_DUMP("hd_videoenc_set HD_VIDEOENC_PARAM_OUT_ENC_PARAM path_id={%lx} image_quality={%lu}\r\n", enc_path_id, enc_out.jpeg.image_quality);

	ret = hd_videoenc_set(enc_path_id, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &enc_out);
	if (ret != HD_OK) {
		DBG_ERR("set_enc_param_out = %d\r\n", ret);
		return E_SYS;
	}

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Get_Out(const HD_PATH_ID enc_path_id, UINT32* quality)
{
	HD_RESULT 		ret;
    HD_VIDEOENC_OUT enc_out = {0};

	ret = hd_videoenc_get(enc_path_id, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &enc_out);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_get = %d\r\n", ret);
		return E_SYS;
	}

	*quality = enc_out.jpeg.image_quality;

	PHOTO_SLICE_ENC_DUMP("hd_videoenc_get HD_VIDEOENC_PARAM_OUT_ENC_PARAM path_id={%lx} image_quality={%lu}\r\n", enc_path_id, enc_out.jpeg.image_quality);


	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Set_In(
		const HD_PATH_ID enc_path_id,
		const HD_VIDEO_PXLFMT vproc_out_pxlfmt,
		const HD_DIM dim)
{
	HD_RESULT 		ret;
    HD_VIDEOENC_IN 	enc_in = {0};

	enc_in.dir	   	= HD_VIDEO_DIR_NONE;
	enc_in.pxl_fmt 	= vproc_out_pxlfmt;
	enc_in.dim 		= dim;
	enc_in.frc	   	= HD_VIDEO_FRC_RATIO(1,1);

	PHOTO_SLICE_ENC_DUMP("hd_videoenc_set HD_VIDEOENC_PARAM_IN path_id={%lx} dim={%lu, %lu} pxlfmt={%lx}\r\n", enc_path_id, enc_in.dim, vproc_out_pxlfmt);

	ret = hd_videoenc_set(enc_path_id, HD_VIDEOENC_PARAM_IN, &enc_in);
	if(ret != HD_OK){
		DBG_ERR("hd_videoenc_set HD_VIDEOENC_PARAM_IN failed(path_id=%lx, ret=%d)!", enc_path_id, ret);
		return E_SYS;
	}

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Get_Comm_Buffer(PhotoExe_MEM_Info* mem_info)
{
	mem_info->blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, mem_info->blk_size, DDR_ID0); // Get block from mem pool
	if (mem_info->blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("hd_common_mem_get_block failed!(size=%lx)\n", mem_info->blk_size);
		return E_SYS;
	}

	mem_info->pa = hd_common_mem_blk2pa(mem_info->blk);
	if (mem_info->pa == 0) {
		DBG_ERR("hd_common_mem_blk2pa failed!(blk=0x%x)\n", mem_info->blk);
		return E_SYS;
	}

	mem_info->va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, mem_info->pa, mem_info->blk_size);
	if (mem_info->va == 0) {
		DBG_ERR("hd_common_mem_mmap failed!\r\n");
		return E_SYS;
	}

	PHOTO_SLICE_ENC_DUMP("hd_common_mem_get_block blk_size={%lx} pa={%lx} va={%lx}\r\n", mem_info->blk_size, mem_info->pa, mem_info->va);

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Release_Comm_Buffer(PhotoExe_MEM_Info mem_info)
{

	if(mem_info.blk != HD_COMMON_MEM_VB_INVALID_BLK ){
		hd_common_mem_release_block(mem_info.blk);
	}

	if(mem_info.va){
		hd_common_mem_munmap((void*) mem_info.va, mem_info.blk_size);
	}

	PHOTO_SLICE_ENC_DUMP("hd_common_mem_release_block blk_size={%lx} pa={%lx} va={%lx}\r\n", mem_info.blk_size, mem_info.pa, mem_info.va);

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Alloc_Buffer(PhotoExe_MEM_Info* info, char* name)
{
	HD_RESULT ret;
	void *va_ptr;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	if ((ret = hd_common_mem_alloc(name, &info->pa, (void **)&va_ptr, info->blk_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", ret);
		return E_SYS;
	}

	info->va = (UINT32)va_ptr;

	PHOTO_SLICE_ENC_DUMP("hd_common_mem_alloc name={%s} blk_size={%lx} pa={%lx} va={%lx}\r\n", name, info->blk_size, info->pa, info->va);


	return E_OK;
}

#if PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC == ENABLE

static INT32  PhotoExe_Preview_SliceEncode_Alloc_Buffer_Retry(PhotoExe_MEM_Info* info, char* name, UINT8 timeout, UINT32 delay_ms)
{
	INT32 ret;
	UINT8 cnt = 0;

	do{
		if(cnt > 0){
			DBG_WRN("retrying %u...\n", cnt);
			vos_util_delay_ms(delay_ms);
		}

		ret = PhotoExe_Preview_SliceEncode_Alloc_Buffer(info, name);
		if(ret == E_OK)
			break;

	} while(++cnt < timeout);

	return ret;
}

#endif

static INT32 PhotoExe_Preview_SliceEncode_Free_Buffer(PhotoExe_MEM_Info* info)
{
	HD_RESULT ret;

	PHOTO_SLICE_ENC_DUMP("free bs buffer(va:%lx)\r\n", info->va);

	if ((ret = hd_common_mem_free(info->pa, (void*) info->va)) != HD_OK) {
		DBG_ERR("hd_common_mem_free failed(%d)\r\n", ret);
		return E_SYS;
	}

	info->va = 0;
	info->pa = 0;
	info->blk = HD_COMMON_MEM_VB_INVALID_BLK;

	return E_OK;
}


static INT32 PhotoExe_Preview_SliceEncode_Get_Enc_Buffer_Info(const HD_PATH_ID enc_path_id, PhotoExe_MEM_Info* info)
{
	HD_RESULT ret;
	HD_VIDEOENC_BUFINFO enc_buf_info = {0};

	if ((ret = hd_videoenc_get(enc_path_id, HD_VIDEOENC_PARAM_BUFINFO, &enc_buf_info)) != HD_OK) {
		DBG_ERR("hd_videoenc_get HD_VIDEOENC_PARAM_BUFINFO failed!(%d)\n", ret);
		return E_SYS;
	}

	info->pa = enc_buf_info.buf_info.phy_addr;
	info->blk_size = enc_buf_info.buf_info.buf_size;

	info->va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, info->pa, info->blk_size);
	if (info->va == 0) {
		DBG_ERR("enc_vir_addr mmap error!!\r\n\r\n");
		return E_SYS;
	}

	PHOTO_SLICE_ENC_DUMP("hd_videoenc_get HD_VIDEOENC_PARAM_BUFINFO blk_size={%lx} pa={%lx} va={%lx}\r\n", info->blk_size, info->pa, info->va);


	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Init_VF_GFX_Slice(
		VF_GFX_SCALE* vf_gfx_scale_param,
		const HD_VIDEO_FRAME* video_frame,
		const PhotoExe_MEM_Info dst_buffer_info,
		const PhotoExe_SliceSize_Info src_slice_info,
		const PhotoExe_SliceSize_Info dst_slice_info,
		const UINT8 slice_idx)
{
	HD_RESULT ret;
	UINT32 addr_src[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr_dst[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff_src[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff_dst[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 offset;
	UINT32 scr_slice_height = (slice_idx == (src_slice_info.slice_num - 1)) ? src_slice_info.last_slice_height : src_slice_info.slice_height;
	UINT32 dst_slice_height = (slice_idx == (dst_slice_info.slice_num - 1)) ? dst_slice_info.last_slice_height : dst_slice_info.slice_height;
	UINT32 dst_scale_slice_height = (slice_idx == (dst_slice_info.slice_num - 1)) ? (src_slice_info.last_slice_height *  dst_slice_info.slice_height / src_slice_info.slice_height) : dst_slice_info.slice_height;

	/* dst img */
	addr_dst[0] = dst_buffer_info.pa;
	loff_dst[0] = dst_slice_info.width;
	addr_dst[1] = addr_dst[0] + loff_dst[0] * dst_slice_height;
	loff_dst[1] = dst_slice_info.width;
	ret = vf_init_ex(&vf_gfx_scale_param->dst_img, dst_slice_info.width, dst_slice_height, video_frame->pxlfmt, loff_dst, addr_dst);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", ret);
		return E_SYS;
	}

	vf_gfx_scale_param->engine = 0;
	vf_gfx_scale_param->src_region.x = 0;
	vf_gfx_scale_param->src_region.y = 0;
	vf_gfx_scale_param->src_region.w = src_slice_info.width;
	vf_gfx_scale_param->src_region.h = scr_slice_height;
	vf_gfx_scale_param->dst_region.x = 0;
	vf_gfx_scale_param->dst_region.y = 0;
	vf_gfx_scale_param->dst_region.w = dst_slice_info.width;
	vf_gfx_scale_param->dst_region.h = dst_scale_slice_height;
	vf_gfx_scale_param->dst_img.blk  = dst_buffer_info.blk;
	vf_gfx_scale_param->quality = HD_GFX_SCALE_QUALITY_NULL;

	offset = video_frame->loff[HD_VIDEO_PINDEX_Y] * slice_idx * src_slice_info.slice_height;

	addr_src[0] = video_frame->phy_addr[HD_VIDEO_PINDEX_Y] + offset;
	addr_src[1] = video_frame->phy_addr[HD_VIDEO_PINDEX_UV] + offset/2;
    loff_src[0] = video_frame->loff[HD_VIDEO_PINDEX_Y];
    loff_src[1] = video_frame->loff[HD_VIDEO_PINDEX_UV];
	if ((ret = vf_init_ex(&vf_gfx_scale_param->src_img, src_slice_info.width, scr_slice_height, video_frame->pxlfmt, loff_src, addr_src)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", ret);
		return E_SYS;
	}

	PHOTO_SLICE_ENC_DUMP("[Slice %lu] src dim{%lu, %lu} src region{%lu, %lu, %lu, %lu} addr{y:%lx uv:%lx}\r\n",
			slice_idx,
			vf_gfx_scale_param->src_img.dim.w,
			vf_gfx_scale_param->src_img.dim.h,
			vf_gfx_scale_param->src_region.x,
			vf_gfx_scale_param->src_region.y,
			vf_gfx_scale_param->src_region.w,
			vf_gfx_scale_param->src_region.h,
			addr_src[0], addr_src[1]
	);

	PHOTO_SLICE_ENC_DUMP("[Slice %lu] dst dim{%lu, %lu} dst region{%lu, %lu, %lu, %lu} addr{y:%lx uv:%lx}\r\n",
			slice_idx,
			vf_gfx_scale_param->dst_img.dim.w,
			vf_gfx_scale_param->dst_img.dim.h,
			vf_gfx_scale_param->dst_region.x,
			vf_gfx_scale_param->dst_region.y,
			vf_gfx_scale_param->dst_region.w,
			vf_gfx_scale_param->dst_region.h,
			addr_dst[0], addr_dst[1]
	);

	if(slice_idx == (dst_slice_info.slice_num - 1) &&
	   dst_scale_slice_height < dst_slice_info.last_slice_height
	){

		HD_GFX_DRAW_RECT draw_rect = {0};

		draw_rect.dst_img.dim = vf_gfx_scale_param->dst_img.dim;
		draw_rect.dst_img.ddr_id =  vf_gfx_scale_param->dst_img.ddr_id;
		draw_rect.dst_img.format = vf_gfx_scale_param->dst_img.pxlfmt;
		draw_rect.dst_img.p_phy_addr[0] = vf_gfx_scale_param->dst_img.phy_addr[0];
		draw_rect.dst_img.p_phy_addr[1] = vf_gfx_scale_param->dst_img.phy_addr[1];
		draw_rect.dst_img.lineoffset[0] = vf_gfx_scale_param->dst_img.loff[0];
		draw_rect.dst_img.lineoffset[1] = vf_gfx_scale_param->dst_img.loff[1];
		draw_rect.color = 0xFF000000; /* black (ARGB8888), driver internally converts into target format */
		draw_rect.thickness = 1;
		draw_rect.type = HD_GFX_RECT_SOLID;
		draw_rect.rect = (HD_IRECT){0, 0, vf_gfx_scale_param->dst_img.dim.w, vf_gfx_scale_param->dst_img.dim.h};
		hd_gfx_draw_rect(&draw_rect);
	}


	return E_OK;
}

#if	PHOTO_SLICE_ENC_DBG_PRIMARY_YUV
static INT32 PhotoExe_Preview_SliceEncode_Dump_Frame(const HD_VIDEO_FRAME video_frame)
{
	char fileName[128] = {0};
	FST_FILE fp = NULL;
	UINT32 size;
	UINT32 va;

	size = VDO_YUV_BUFSIZE(video_frame.dim.w, video_frame.dim.h, video_frame.pxlfmt);

	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], size);
	if (va == 0) {
		DBG_ERR("hd_common_mem_mmap error!r\n");
		return E_SYS;
	}

	sprintf(fileName, "A:\\frame_%lux%lu_fmt%lx.dat", video_frame.dim.w, video_frame.dim.h, video_frame.pxlfmt);

	fp = FileSys_OpenFile(fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	FileSys_WriteFile(fp, (UINT8*)va, &size, 0, NULL);
	FileSys_FlushFile(fp);
	FileSys_CloseFile(fp);

	hd_common_mem_munmap((void*) va, size);

	return E_OK;
}
#endif


#if PHOTO_SLICE_ENC_DBG_SRC_SLICE_YUV
static INT32 PhotoExe_Preview_SliceEncode_Dump_Src_Slice(
		const HD_VIDEO_FRAME* video_frame,
		const PhotoExe_SliceSize_Info src_slice_info,
		const UINT8 slice_idx,
		const UINT32 pa_y,
		const UINT32 pa_uv
)
{
	char fileName[128] = {0};
	UINT32 src_slice_height = (slice_idx == (src_slice_info.slice_num - 1)) ? src_slice_info.last_slice_height : src_slice_info.slice_height;
	UINT32 va;
	UINT32 size;
	FST_FILE fp = NULL;

	size = VDO_YUV_BUFSIZE(src_slice_info.width, src_slice_height, video_frame->pxlfmt);

	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa_y, size);
	if (va == 0) {
		DBG_ERR("hd_common_mem_mmap error!r\n");
		return E_SYS;
	}

	sprintf(fileName, "A:\\src_slice%u_%lux%lu_fmt%lx.dat", slice_idx, src_slice_info.width, src_slice_height, video_frame->pxlfmt);

	fp = FileSys_OpenFile(fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);

	size = src_slice_info.width * src_slice_height;
	FileSys_WriteFile(fp, (UINT8*)va, &size, 0, NULL);

	size = (src_slice_info.width * src_slice_height) / 2;
	FileSys_WriteFile(fp, (UINT8*)va + (pa_uv - pa_y), &size, 0, NULL);

	FileSys_FlushFile(fp);
	FileSys_CloseFile(fp);

	hd_common_mem_munmap((void*) va, size);

	return E_OK;
}

#endif

#if PHOTO_SLICE_ENC_DBG_DST_SLICE_YUV

static INT32 PhotoExe_Preview_SliceEncode_Dump_Dst_Slice(
		const HD_VIDEO_FRAME* video_frame,
		const PhotoExe_SliceSize_Info dst_slice_info,
		const UINT8 slice_idx,
		const UINT32 pa)
{
	char fileName[128] = {0};
	UINT32 dst_slice_height = (slice_idx == (dst_slice_info.slice_num - 1)) ? dst_slice_info.last_slice_height : dst_slice_info.slice_height;
	UINT32 va;
	UINT32 size;
	FST_FILE fp = NULL;

	size = VDO_YUV_BUFSIZE(dst_slice_info.width, dst_slice_height, video_frame->pxlfmt);

	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);
	if (va == 0) {
		DBG_ERR("hd_common_mem_mmap error!r\n");
		return E_SYS;
	}

	sprintf(fileName, "A:\\dst_slice%u_%lux%lu_fmt%lx.dat", slice_idx, dst_slice_info.width, dst_slice_height, video_frame->pxlfmt);

	fp = FileSys_OpenFile(fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	FileSys_WriteFile(fp, (UINT8*)va, &size, 0, NULL);
	FileSys_FlushFile(fp);
	FileSys_CloseFile(fp);

	hd_common_mem_munmap((void*) va, size);

	return E_OK;
}

#endif


#if PHOTO_SLICE_ENC_DBG_JPG

static INT32 PhotoExe_Preview_SliceEncode_Dump_JPG(const PhotoExe_MEM_Info* info, char* name)
{
	char fileName[128] = {0};
	FST_FILE fp = NULL;
	UINT32 size = info->used_size;

	sprintf(fileName, "A:\\%s.jpg", name);

	fp = FileSys_OpenFile(fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	FileSys_WriteFile(fp, (UINT8*)info->va, &size, 0, NULL);
	FileSys_FlushFile(fp);
	FileSys_CloseFile(fp);

	return E_OK;
}

#endif

void PhotoExe_Cal_Jpg_Size(USIZE *psrc, USIZE *pdest , URECT *pdestwin)
{
	SIZECONVERT_INFO scale_info = {0};

	scale_info.uiSrcWidth  = psrc->w;
	scale_info.uiSrcHeight = psrc->h;
	scale_info.uiDstWidth  = pdest->w;
	scale_info.uiDstHeight = pdest->h;
	scale_info.uiDstWRatio = pdest->w;
	scale_info.uiDstHRatio = pdest->h;
	scale_info.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&scale_info);
	pdestwin->x = scale_info.uiOutX;
	pdestwin->y = scale_info.uiOutY;
	pdestwin->w = scale_info.uiOutWidth;
	pdestwin->h = scale_info.uiOutHeight;
}

ER PhotoExe_Preview_SliceEncode_Scale_YUV(
		VF_GFX_SCALE *pscale,
		const HD_VIDEO_FRAME *psrc,
		const PhotoExe_MEM_Info mem_info,
		USIZE *pdest_sz,
		URECT *pdestwin,
		HD_VIDEO_PXLFMT pxl_fmt)
{
	UINT32 blk_size;
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	VF_GFX_DRAW_RECT fill_rect = {0};
	HD_RESULT hd_ret;

	blk_size = VDO_YUV_BUFSIZE(pdest_sz->w, pdest_sz->h, pxl_fmt);

	if (blk_size > mem_info.blk_size) {
		DBG_ERR("Request blk_size(%d) > pComBufInfo->blk_size(%d)\r\n", blk_size, mem_info.blk_size);
		return E_SYS;
	}


	memcpy(&pscale->src_img, psrc, sizeof(HD_VIDEO_FRAME));

	// set dest
	addr[0] = mem_info.pa;
	loff[0] = ALIGN_CEIL_4(pdest_sz->w);
	if(pxl_fmt == HD_VIDEO_PXLFMT_YUV420 ){
		addr[1] = addr[0] + loff[0] * pdest_sz->h;
		loff[1] = ALIGN_CEIL_4(pdest_sz->w);
	}else{
		addr[1] = addr[0] + loff[0] * pdest_sz->h;
		loff[1] = ALIGN_CEIL_4(pdest_sz->w);
		addr[2] = addr[0] + loff[0] * pdest_sz->h + (loff[1] * pdest_sz->h)/2;
		loff[2] = ALIGN_CEIL_4(pdest_sz->w);
	}
	if ((hd_ret = vf_init_ex(&(pscale->dst_img), pdest_sz->w, pdest_sz->h, pxl_fmt, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
	}

	if ((pdest_sz->w != pdestwin->w ) || (pdest_sz->h != pdestwin->h)) {
		// clear buffer by black
		//gximg_fill_data((VDO_FRAME *)&(pscale->dst_img), GXIMG_REGION_MATCH_IMG, COLOR_YUV_BLACK);
		memcpy((void *)&(fill_rect.dst_img), (void *)&(pscale->dst_img), sizeof(HD_VIDEO_FRAME));
		fill_rect.color = COLOR_RGB_BLACK;
		fill_rect.rect.x = 0;
		fill_rect.rect.y = 0;
		fill_rect.rect.w = pdest_sz->w;
		fill_rect.rect.h = pdest_sz->h;
		fill_rect.type = HD_GFX_RECT_SOLID;
		fill_rect.thickness = 0;
		fill_rect.engine = 0;
		if ((hd_ret = vf_gfx_draw_rect(&fill_rect)) != HD_OK) {
			DBG_ERR("vf_gfx_draw_rect failed(%d)\r\n", hd_ret);
		}
	}

	// set config
	pscale->engine = 0;
	pscale->src_region.x = 0;
	pscale->src_region.y = 0;
	pscale->src_region.w = psrc->dim.w;
	pscale->src_region.h = psrc->dim.h;
	pscale->dst_region.x = pdestwin->x;
	pscale->dst_region.y = pdestwin->y;
	pscale->dst_region.w = pdestwin->w;
	pscale->dst_region.h = pdestwin->h;
	pscale->dst_img.blk	 = mem_info.blk;
	pscale->quality = HD_GFX_SCALE_QUALITY_NULL;
	vf_gfx_scale(pscale, 1);

	pscale->dst_img.count 	= 0;
	pscale->dst_img.timestamp = hd_gettime_us();

	return E_OK;

}

/***************************************************************************
 * screennail rate control
 ***************************************************************************/
#if CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL
static INT32 PhotoExe_Preview_SliceEncode_Encode_Screennail_RC(HD_VIDEO_FRAME* video_frame_in, UINT8* screennail_quality)
{
	UINT8 quality = *screennail_quality;
	INT8 direction = 0;
	INT32 ret = E_OK;
	PhotoExe_SliceEncode_Info* slice_encode_screennail_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_SCREENNAIL);
	const UINT32 ubount = CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_UBOUND;
	const UINT32 lbount = CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_LBOUND;
	BOOL stop_flag = FALSE;

	do {
		PHOTO_SLICE_ENC_DUMP("screennail quality = %lu\n", quality);

		*screennail_quality = quality;
		ret = PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_screennail_info->enc_path_id, quality);
		if(ret != E_OK){
			goto EXIT;
		}

		ret = PhotoExe_Preview_SliceEncode_Encode_Screennail(video_frame_in);
		if ( (ret != E_OK) && (ret != HD_ERR_NOMEM)){
			goto EXIT;
		}

		if((slice_encode_screennail_info->bs_buf_mem_info.used_size > ubount) || (ret == HD_ERR_NOMEM)){

			if(direction == 1){
				quality -= CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP;
				stop_flag = TRUE; /* current step > ubound and next prev step < lbound , stop rc flow (choose prev step)*/
				continue;
			}
			else{
				direction = -1;

				if(quality == 0){
					DBG_WRN("lowest quality reached, stop rc flow!\n");
					goto EXIT;
				}
				else if(quality > CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP)
					quality -= CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP;
				else{
					quality = 0;
				}
			}
		}
		else if(slice_encode_screennail_info->bs_buf_mem_info.used_size < lbount){

			if(stop_flag){
				break;
			}
			if(direction == -1){
				DBG_WRN("back search due to low resolution of CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP(%lu), stop rc flow\n", CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP);
				break;
			}
			else{
				direction = 1;

				if(quality >= 99){
					quality = 99;
					DBG_WRN("highest quality reached, stop rc flow!\n");
					goto EXIT;
				}
				else if(quality < (99 - CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP))
					quality += CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_QUALITY_STEP;
				else{
					quality = 99;
				}
			}
		}
		else{
			break;
		}

	} while(1);


EXIT:
	return ret;
}
#endif

static INT32 PhotoExe_Preview_SliceEncode_Scale_Thumbnail(
		const HD_VIDEO_FRAME* video_frame_in,
		HD_VIDEO_FRAME* video_frame_out)
{
	INT32 ret;
	VF_GFX_SCALE vf_gfx_scale = {0};
	URECT dest_win = {0};
	USIZE src_size = {0}, dest_size = {0};
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_THUMBNAIL);
	PhotoExe_MEM_Info thumbnail_buffer_info = slice_enc_info->yuv_buf_mem_info;

	src_size.w = video_frame_in->dim.w;
	src_size.h = video_frame_in->dim.h;
	dest_size.w = CFG_THUMBNAIL_W;
	dest_size.h = CFG_THUMBNAIL_H;
	PhotoExe_Cal_Jpg_Size(&src_size, &dest_size , &dest_win);

	ret = PhotoExe_Preview_SliceEncode_Scale_YUV(&vf_gfx_scale, video_frame_in, thumbnail_buffer_info, &dest_size, &dest_win, HD_VIDEO_PXLFMT_YUV420);
	if(ret != E_OK){
		return ret;
	}

	*video_frame_out = vf_gfx_scale.dst_img;

	return E_OK;
}

#if HEIC_FUNC == ENABLE

static INT32 PhotoExe_Preview_SliceEncode_Scale_HEIC(
		const HD_VIDEO_FRAME* video_frame_in,
		HD_VIDEO_FRAME* video_frame_out)
{
	INT32 ret;
	VF_GFX_SCALE vf_gfx_scale = {0};
	URECT dest_win = {0};
	USIZE src_size = {0}, dest_size = {0};
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_PRIMARY);
	PhotoExe_MEM_Info heic_buffer_info = slice_enc_info->yuv_buf_mem_info;

	src_size.w = video_frame_in->dim.w;
	src_size.h = video_frame_in->dim.h;
	dest_size.w = CFG_JPG_PREVIEW_SLICE_ENC_HEIC_WIDTH;
	dest_size.h = CFG_JPG_PREVIEW_SLICE_ENC_HEIC_HEIGHT;
	PhotoExe_Cal_Jpg_Size(&src_size, &dest_size , &dest_win);

	ret = PhotoExe_Preview_SliceEncode_Scale_YUV(&vf_gfx_scale, video_frame_in, heic_buffer_info, &dest_size, &dest_win, HD_VIDEO_PXLFMT_YUV420);
	if(ret != E_OK){
		return ret;
	}

	*video_frame_out = vf_gfx_scale.dst_img;

	return E_OK;
}

#endif

static INT32 PhotoExe_Preview_SliceEncode_Encode_Thumbnail(HD_VIDEO_FRAME* video_frame_in)
{
	HD_RESULT ret;
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_THUMBNAIL);
	HD_PATH_ID enc_path_id = slice_enc_info->enc_path_id;
	UINT32 enc_jpg_va = 0;
	HD_VIDEOENC_BS bs_data_pull = {0};

	if ((ret = hd_videoenc_start(slice_enc_info->enc_path_id)) != HD_OK) {
		DBG_ERR("hd_videoenc_start failed!(%d)\n", ret);
		goto EXIT;
	}

	PhotoExe_Preview_SliceEncode_Get_Enc_Buffer_Info(enc_path_id, &slice_enc_info->hd_enc_internal_buf_mem_info);

	ret = hd_videoenc_push_in_buf(enc_path_id, video_frame_in, NULL, -1); // -1 = blocking mode
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_push_in_buf failed!(%d)\r\n", ret);
		goto EXIT;
	}

	ret = hd_videoenc_pull_out_buf(enc_path_id, &bs_data_pull, -1); // -1 = blocking mode
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_pull_out_buf failed!(%d)\r\n", ret);
		goto EXIT;
	}

	enc_jpg_va = PhotoExe_PHY2VIRT(bs_data_pull.video_pack[0].phy_addr, slice_enc_info->hd_enc_internal_buf_mem_info.pa, slice_enc_info->hd_enc_internal_buf_mem_info.va);
	if(enc_jpg_va == 0){
		ret = HD_ERR_SYS;
		goto EXIT;
	}

	PHOTO_SLICE_ENC_DUMP("copy bs data (%lx -> %lx) , size = %lx\r\n", enc_jpg_va, slice_enc_info->bs_buf_mem_info.va, bs_data_pull.video_pack[0].size);
	if(bs_data_pull.video_pack[0].size > slice_enc_info->bs_buf_mem_info.blk_size){
		DBG_ERR("bs overflow!(data size: %lx buffer size: %lx)\r\n", bs_data_pull.video_pack[0].size, slice_enc_info->bs_buf_mem_info.blk_size);
		ret = HD_ERR_NOMEM;
		goto EXIT;
	}

	memcpy((VOID*)slice_enc_info->bs_buf_mem_info.va, (VOID*)enc_jpg_va, bs_data_pull.video_pack[0].size);
	slice_enc_info->bs_buf_mem_info.used_size = bs_data_pull.video_pack[0].size;

EXIT:

	/* unmap hdal internal buffer */
	if(slice_enc_info->hd_enc_internal_buf_mem_info.va){

		if (hd_common_mem_munmap((void *)slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size) != HD_OK) {
			DBG_ERR("hd_common_mem_munmap error!(va:%lx size:%lx)\r\n", slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size);
		}

		memset(&slice_enc_info->hd_enc_internal_buf_mem_info, 0, sizeof(PhotoExe_MEM_Info));
	}

	if(bs_data_pull.video_pack[0].size){
		hd_videoenc_release_out_buf(enc_path_id, &bs_data_pull);
	}

	if ((hd_videoenc_stop(enc_path_id)) != HD_OK) {
		DBG_ERR("hd_videoenc_stop failed(%d)\r\n", ret);
	}

	return (ret == HD_OK) ? E_OK : E_SYS;
}


static INT32 PhotoExe_Preview_SliceEncode_Scale_Screennail(
		const HD_VIDEO_FRAME* video_frame_in,
		HD_VIDEO_FRAME* video_frame_out)
{
	INT32 ret;
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_SCREENNAIL);
	PhotoExe_MEM_Info screennail_buffer_info = slice_enc_info->yuv_buf_mem_info;
	VF_GFX_SCALE vf_gfx_scale = {0};
	URECT dest_win = {0};
	USIZE src_size = {0}, dest_size = {0};

	src_size.w = video_frame_in->dim.w;
	src_size.h = video_frame_in->dim.h;
	dest_size.w = CFG_SCREENNAIL_W;
	dest_size.h = CFG_SCREENNAIL_H;
	PhotoExe_Cal_Jpg_Size(&src_size, &dest_size , &dest_win);

	ret = PhotoExe_Preview_SliceEncode_Scale_YUV(&vf_gfx_scale, video_frame_in, screennail_buffer_info, &dest_size, &dest_win, HD_VIDEO_PXLFMT_YUV420);
	if(ret != E_OK){
		return ret;
	}

	*video_frame_out = vf_gfx_scale.dst_img;

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Screennail(HD_VIDEO_FRAME* video_frame_in)
{
	HD_RESULT ret;
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_SCREENNAIL);
	HD_PATH_ID enc_path_id = slice_enc_info->enc_path_id;
	UINT32 enc_jpg_va = 0;
	HD_VIDEOENC_BS bs_data_pull = {0};

	if ((ret = hd_videoenc_start(slice_enc_info->enc_path_id)) != HD_OK) {
		DBG_ERR("hd_videoenc_start failed!(%d)\n", ret);
		goto EXIT;
	}

	PhotoExe_Preview_SliceEncode_Get_Enc_Buffer_Info(enc_path_id, &slice_enc_info->hd_enc_internal_buf_mem_info);

	ret = hd_videoenc_push_in_buf(enc_path_id, video_frame_in, NULL, -1); // -1 = blocking mode
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_push_in_buf failed!(%d)\r\n", ret);
		goto EXIT;
	}

	ret = hd_videoenc_pull_out_buf(enc_path_id, &bs_data_pull, -1); // -1 = blocking mode
	if (ret != HD_OK) {
		DBG_ERR("hd_videoenc_pull_out_buf failed!(%d)\r\n", ret);
		goto EXIT;
	}


	enc_jpg_va = PhotoExe_PHY2VIRT(bs_data_pull.video_pack[0].phy_addr, slice_enc_info->hd_enc_internal_buf_mem_info.pa, slice_enc_info->hd_enc_internal_buf_mem_info.va);
	if(enc_jpg_va == 0){
		ret = HD_ERR_SYS;
		goto EXIT;
	}

	PHOTO_SLICE_ENC_DUMP("copy bs data (%lx -> %lx) , size = %lx\r\n", enc_jpg_va, slice_enc_info->bs_buf_mem_info.va, bs_data_pull.video_pack[0].size);
	if(bs_data_pull.video_pack[0].size > slice_enc_info->bs_buf_mem_info.blk_size){
		DBG_ERR("bs overflow!(data size: %lx buffer size: %lx)\r\n", bs_data_pull.video_pack[0].size, slice_enc_info->bs_buf_mem_info.blk_size);
		ret = HD_ERR_NOMEM;
		goto EXIT;
	}

	memcpy((VOID*)slice_enc_info->bs_buf_mem_info.va, (VOID*)enc_jpg_va, bs_data_pull.video_pack[0].size);
	slice_enc_info->bs_buf_mem_info.used_size = bs_data_pull.video_pack[0].size;

EXIT:

	/* unmap hdal internal buffer */
	if(slice_enc_info->hd_enc_internal_buf_mem_info.va){

		if (hd_common_mem_munmap((void *)slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size) != HD_OK) {
			DBG_ERR("hd_common_mem_munmap error!(va:%lx size:%lx)\r\n", slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size);
		}

		memset(&slice_enc_info->hd_enc_internal_buf_mem_info, 0, sizeof(PhotoExe_MEM_Info));
	}

	if(bs_data_pull.video_pack[0].size){
		hd_videoenc_release_out_buf(enc_path_id, &bs_data_pull);
	}

	if (hd_videoenc_stop(enc_path_id) != HD_OK) {
		DBG_ERR("hd_videoenc_stop failed(%d)\r\n", ret);
	}

	return (ret == HD_OK) ? E_OK : ret;

}

static INT32 PhotoExe_Preview_SliceEncode_DateStamp(
		const HD_VIDEO_FRAME* frame,
		IMG_CAP_DATASTAMP_EVENT event
)
{
	if (UI_GetData(FL_DATE_STAMP) != DATEIMPRINT_OFF) {

		IMG_CAP_DATASTAMP_INFO stamp_info = {0};

		stamp_info.ImgInfo.ch[IMG_CAP_YUV_Y].width = frame->pw[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_Y].height = frame->ph[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.h;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_Y].line_ofs = frame->loff[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_U].width = frame->pw[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_U].height = frame->ph[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.h>>1;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_U].line_ofs=  frame->loff[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_V].width = frame->pw[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_V].height = frame->ph[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.h>>1;
		stamp_info.ImgInfo.ch[IMG_CAP_YUV_V].line_ofs = frame->loff[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
		stamp_info.ImgInfo.pixel_addr[0] = frame->phy_addr[0];
		stamp_info.ImgInfo.pixel_addr[1] = frame->phy_addr[1];
		stamp_info.ImgInfo.pixel_addr[2] = frame->phy_addr[2];
		stamp_info.event = event;

		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_GEN_DATE_PIC, 1, (UINT32)&stamp_info);
	}

	return E_OK;
}

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == ENABLE
static INT32 PhotoExe_Preview_SliceEncode_Src_DateStamp(
		const HD_VIDEO_FRAME* video_frame,
		const PhotoExe_SliceSize_Info* src_slice_info,
		const PhotoExe_SliceSize_Info* dst_slice_info)
{

	HD_VIDEO_FRAME video_frame_src_stamp = *video_frame;
	UINT32 dst_last_slice_scale_height = (src_slice_info->last_slice_height *  dst_slice_info->slice_height) / src_slice_info->slice_height;

	if(dst_last_slice_scale_height > dst_slice_info->last_slice_height)
	{
		video_frame_src_stamp.dim.h = video_frame_src_stamp.dim.h - ((src_slice_info->last_slice_height * (dst_last_slice_scale_height - dst_slice_info->last_slice_height)) / dst_last_slice_scale_height);
		video_frame_src_stamp.ph[0] = video_frame_src_stamp.dim.h;
		video_frame_src_stamp.ph[1] = video_frame_src_stamp.dim.h;
	}

	return PhotoExe_Preview_SliceEncode_DateStamp(&video_frame_src_stamp, CAP_DS_EVENT_PRI);
}
#endif

static INT32 PhotoExe_Preview_SliceEncode_QView(HD_VIDEO_FRAME* video_frame)
{
	HD_PATH_ID vout_path = 0;
	IMG_CAP_YCC_IMG_INFO QvImgInfo = {0};
	IMG_CAP_QV_DATA QVInfor = {0};
	UINT8 retry_timeout = 10 , cnt = 0;
	HD_RESULT ret;

	QvImgInfo.ch[IMG_CAP_YUV_Y].width = video_frame->pw[IMG_CAP_YUV_Y];
	QvImgInfo.ch[IMG_CAP_YUV_Y].height = video_frame->ph[IMG_CAP_YUV_Y];
	QvImgInfo.ch[IMG_CAP_YUV_Y].line_ofs = video_frame->loff[IMG_CAP_YUV_Y];
	QvImgInfo.ch[IMG_CAP_YUV_U].width = video_frame->pw[IMG_CAP_YUV_U];
	QvImgInfo.ch[IMG_CAP_YUV_U].height = video_frame->ph[IMG_CAP_YUV_U];
	QvImgInfo.ch[IMG_CAP_YUV_U].line_ofs = video_frame->loff[IMG_CAP_YUV_U];
	QvImgInfo.ch[IMG_CAP_YUV_V].width = video_frame->pw[IMG_CAP_YUV_V];
	QvImgInfo.ch[IMG_CAP_YUV_V].height = video_frame->ph[IMG_CAP_YUV_V];
	QvImgInfo.pixel_addr[0] = video_frame->phy_addr[0];
	QvImgInfo.pixel_addr[1] = video_frame->phy_addr[1];
	QvImgInfo.pixel_addr[2] = video_frame->phy_addr[2];
	QVInfor.ImgInfo = QvImgInfo;

	if(PhotoCapMsgCb){
		PhotoCapMsgCb(IMG_CAP_CBMSG_QUICKVIEW, &QVInfor);
	}

	vout_path = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);

	do{
		if ((ret = hd_videoout_push_in_buf(vout_path, video_frame, NULL, -1)) != HD_OK) {
			DBG_WRN("QV push_in error(%d), retrying ...\n", ret);
			vos_util_delay_ms(1);
			continue;
		}

		break;
	} while(cnt++ < retry_timeout);

	if(ret != HD_OK){
		DBG_ERR("QV push_in error(%d)\n", ret);
		return E_SYS;
	}

	return E_OK;
}

static INT32 PhotoExe_Preview_SliceEncode_Encode_Primary(
		const HD_VIDEO_FRAME* video_frame,
		const PhotoExe_SliceSize_Info src_slice_info,
		const PhotoExe_SliceSize_Info dst_slice_info,
		UINT32* enc_bs_accum_size,
		UINT8* primary_quality
)
{
	PhotoExe_SliceEncode_Info* slice_enc_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_PRIMARY);
	HD_PATH_ID enc_path_id = slice_enc_info->enc_path_id;
	PhotoExe_MEM_Info yuv_buffer_info = slice_enc_info->yuv_buf_mem_info;
	PhotoExe_MEM_Info enc_bs_buf_info = slice_enc_info->bs_buf_mem_info;
	HD_RESULT ret = 0;
	VF_GFX_SCALE vf_gfx_scale_param = {0};
	UINT32 enc_jpg_va = 0;
	UINT32 enc_bs_buf_ptr = 0;
	BOOL restart = FALSE;
	HD_VIDEOENC_BS bs_data_pull;

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == DISABLE
	const UINT8 slice_idx_of_date_stamp = dst_slice_info.slice_num - 1;
#endif

	do {
		restart = FALSE;

		if ((ret = hd_videoenc_start(enc_path_id)) != HD_OK) {
			DBG_ERR("hd_videoenc_start failed!(%d)\n", ret);
	 		goto EXIT;
		}

		if(PhotoExe_Preview_SliceEncode_Get_Enc_Buffer_Info(enc_path_id, &slice_enc_info->hd_enc_internal_buf_mem_info) != E_OK)
			goto EXIT;

		/* reserve space(CFG_JPG_HEADER_SIZE) for header */
		enc_bs_buf_ptr = enc_bs_buf_info.va + CFG_JPG_HEADER_SIZE;
		*enc_bs_accum_size = 0;

		for(UINT8 slice_idx=0 ; slice_idx<dst_slice_info.slice_num ; slice_idx++)
		{
			PHOTO_SLICE_ENC_DUMP("\r\n");

			if(slice_idx == 0){
				SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_PRI_SCALE_S);
			}

			PhotoExe_Preview_SliceEncode_Init_VF_GFX_Slice(
					&vf_gfx_scale_param,
					video_frame,
					yuv_buffer_info,
					src_slice_info,
					dst_slice_info,
					slice_idx);

			if((ret = vf_gfx_scale(&vf_gfx_scale_param, 1)) != HD_OK){
				goto EXIT;
			}

			if(slice_idx == 0){
				SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_PRI_SCALE_E);
			}

			/* attach the date stamp to a slice */
#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == DISABLE
			if(slice_idx == slice_idx_of_date_stamp){

				SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_PRI_STAMP_S);

				HD_VIDEO_FRAME slice_img;

				memcpy(&slice_img, &vf_gfx_scale_param.dst_img, sizeof(HD_VIDEO_FRAME));

				if(slice_idx_of_date_stamp == (dst_slice_info.slice_num - 1)){
					slice_img.dim.h = dst_slice_info.last_slice_height;
					slice_img.ph[0] = dst_slice_info.last_slice_height;
					slice_img.ph[1] = dst_slice_info.last_slice_height;
				}

				PhotoExe_Preview_SliceEncode_DateStamp(&slice_img, CAP_DS_EVENT_PRI);

				SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_PRI_STAMP_E);
			}
#endif

			vf_gfx_scale_param.dst_img.count 	= 0;
			vf_gfx_scale_param.dst_img.timestamp = hd_gettime_us();

			if(dst_slice_info.slice_num > 1){
				vf_gfx_scale_param.dst_img.reserved[1] = MAKEFOURCC('P','A','R','T');
				vf_gfx_scale_param.dst_img.reserved[2] = (dst_slice_info.slice_num << 16) | slice_idx;
			}
			else{
				vf_gfx_scale_param.dst_img.reserved[1] = 0;
				vf_gfx_scale_param.dst_img.reserved[2] = 0;
			}

#if PHOTO_SLICE_ENC_DBG_SRC_SLICE_YUV

			PhotoExe_Preview_SliceEncode_Dump_Src_Slice(
					video_frame,
					src_slice_info,
					slice_idx,
					vf_gfx_scale_param.src_img.phy_addr[0],
					vf_gfx_scale_param.src_img.phy_addr[1]
			);

#endif

#if PHOTO_SLICE_ENC_DBG_DST_SLICE_YUV

			PhotoExe_Preview_SliceEncode_Dump_Dst_Slice(
					video_frame,
					dst_slice_info,
					slice_idx,
					vf_gfx_scale_param.dst_img.phy_addr[0]
					);

#endif

			PHOTO_SLICE_ENC_DUMP("[Slice %lu] reserved[2]:%lx dst_img.dim:{%lu, %lu}\r\n",
					slice_idx,
					vf_gfx_scale_param.dst_img.reserved[2],
					vf_gfx_scale_param.dst_img.dim.w, vf_gfx_scale_param.dst_img.dim.h
			);

			ret = hd_videoenc_push_in_buf(enc_path_id, &vf_gfx_scale_param.dst_img, NULL, -1); // -1 = blocking mode
			if (ret != HD_OK) {
				DBG_ERR("hd_videoenc_push_in_buf failed!(%d)\r\n", ret);
				goto EXIT;
			}

			ret = hd_videoenc_pull_out_buf(enc_path_id, &bs_data_pull, -1); // -1 = blocking mode
			if (ret != HD_OK) {
				DBG_ERR("hd_videoenc_pull_out_buf failed!(%d)\r\n", ret);
				goto EXIT;
			}


			PHOTO_SLICE_ENC_DUMP("[Slice %lu] pack pa:%lx , size:%lx\r\n",
					slice_idx,
					bs_data_pull.video_pack[0].phy_addr,
					bs_data_pull.video_pack[0].size
			);

			enc_jpg_va = PhotoExe_PHY2VIRT(bs_data_pull.video_pack[0].phy_addr, slice_enc_info->hd_enc_internal_buf_mem_info.pa, slice_enc_info->hd_enc_internal_buf_mem_info.va);
			if(enc_jpg_va == 0){
				ret = HD_ERR_SYS;
				goto EXIT;
			}

			if((*enc_bs_accum_size + bs_data_pull.video_pack[0].size + CFG_JPG_HEADER_SIZE + PhotoExe_GetScreenNailSize()) < enc_bs_buf_info.blk_size){

				PHOTO_SLICE_ENC_DUMP("[Slice %u] enc accum size: %lx , slice size:%lx , bs buf size: %lx buffer ptr:%lx , jpg va: %lx\r\n",
						slice_idx, *enc_bs_accum_size,
						bs_data_pull.video_pack[0].size,
						enc_bs_buf_info.blk_size,
						enc_bs_buf_ptr,
						enc_jpg_va
						);

				memcpy((void *)enc_bs_buf_ptr, (void *)enc_jpg_va, bs_data_pull.video_pack[0].size);
				(*enc_bs_accum_size) += bs_data_pull.video_pack[0].size;
				enc_bs_buf_ptr += bs_data_pull.video_pack[0].size;

				hd_videoenc_release_out_buf(enc_path_id, &bs_data_pull);
				memset(&bs_data_pull, 0, sizeof(HD_VIDEOENC_BS)); /* marked as released */

			}
			/* restart process */
			else{

				UINT32 quality_old = 0, quality_new = 0;

				DBG_WRN("[Slice %lu] bs buffer overflow!(primary bs buffer size:%lx bs accum size:%lx)\r\n",
						slice_idx,
						enc_bs_buf_info.blk_size - (CFG_JPG_HEADER_SIZE + PhotoExe_GetScreenNailSize()),
						(*enc_bs_accum_size + bs_data_pull.video_pack[0].size)
				);

				/* check old quality */
				PhotoExe_Preview_SliceEncode_Encode_Get_Out(slice_enc_info->enc_path_id, &quality_old);

				if(quality_old == 1){
					restart = FALSE;
				}
				else if(quality_old <= CFG_JPG_PREVIEW_SLICE_ENC_QUALITY_STEP){
					quality_new = 1;
					restart = TRUE;
				}
				else if(quality_old > CFG_JPG_PREVIEW_SLICE_ENC_QUALITY_STEP){
					quality_new = quality_old - CFG_JPG_PREVIEW_SLICE_ENC_QUALITY_STEP;
					restart = TRUE;
				}
				else{
					restart = FALSE;
				}

				/* check restart */
				if(restart == FALSE){
					ret = HD_ERR_SYS;
					goto EXIT;
				}
				else{

					DBG_WRN("reset quality %lu -> %lu\r\n", quality_old, quality_new);

					if ((ret = hd_videoenc_stop(enc_path_id)) != HD_OK) {
						goto EXIT;
					}

					*primary_quality = quality_new;
					if((ret = PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_enc_info->enc_path_id, quality_new) != HD_OK))
						goto EXIT;

					break;
				}
			}
		}

		if(restart == TRUE)
		{
			hd_videoenc_release_out_buf(enc_path_id, &bs_data_pull);
			memset(&bs_data_pull, 0, sizeof(HD_VIDEOENC_BS)); /* marked as released */

			/* unmap hdal internal buffer */
			if(slice_enc_info->hd_enc_internal_buf_mem_info.va){

				ret = hd_common_mem_munmap((void *)slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size);
				if (ret != HD_OK) {
					DBG_ERR("hd_common_mem_munmap error!(va:%lx size:%lx)\r\n", slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size);
				}

				memset(&slice_enc_info->hd_enc_internal_buf_mem_info, 0, sizeof(PhotoExe_MEM_Info));
			}
		}
		else{
			break;
		}


	} while(1);

EXIT:

	/* unmap hdal internal buffer */
	if(slice_enc_info->hd_enc_internal_buf_mem_info.va){

		if (hd_common_mem_munmap((void *)slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size) != HD_OK) {
			DBG_ERR("hd_common_mem_munmap error!(va:%lx size:%lx)\r\n", slice_enc_info->hd_enc_internal_buf_mem_info.va, slice_enc_info->hd_enc_internal_buf_mem_info.blk_size);
		}

		memset(&slice_enc_info->hd_enc_internal_buf_mem_info, 0, sizeof(PhotoExe_MEM_Info));
	}

	if(bs_data_pull.video_pack[0].size){
		hd_videoenc_release_out_buf(enc_path_id, &bs_data_pull);
		memset(&bs_data_pull, 0, sizeof(HD_VIDEOENC_BS)); /* marked as released */
	}

	if (hd_videoenc_stop(enc_path_id) != HD_OK) {
		DBG_WRN("hd_videoenc_stop failed!(%d)\n", ret);
	}

	return (ret == HD_OK) ? E_OK : E_SYS;
}


static void PhotoExe_Preview_SliceEncode_Perf_Result(const PhotoExe_SliceSize_Info* slice_info)
{
#if PHOTO_SLICE_ENC_DBG_PERF
	VOS_TICK tick;

	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_E));
	DBG_DUMP("Slice Encode = %lu ms\n", tick / 1000);

	/* pri */
	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_PRI_ENC_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_PRI_ENC_E));
	DBG_DUMP("Pri(Scale + Enc + Stamp) = %lu ms\n", tick / 1000);

	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_PRI_SCALE_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_PRI_SCALE_E));
	DBG_DUMP("Pri Scale(One Slice) = %lu ms , slice num = %lu\n", tick / 1000, slice_info->slice_num);

	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_PRI_STAMP_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_PRI_STAMP_E));
	DBG_DUMP("Pri Stamp = %lu ms\n", tick / 1000);

	/* screennail */
	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_SCR_SCALE_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_SCR_SCALE_E));
	DBG_DUMP("Scr Scale = %lu ms\n", tick / 1000);

	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_SCR_STAMP_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_SCR_STAMP_E));
	DBG_DUMP("Scr stamp = %lu ms\n", tick / 1000);

	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_SCR_ENC_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_SCR_ENC_E));
	DBG_DUMP("Scr Enc = %lu ms\n", tick / 1000);

	/* thumb */
	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_THUMB_SCALE_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_THUMB_SCALE_E));
	DBG_DUMP("Thumb Scale = %lu ms\n", tick / 1000);

	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_THUMB_ENC_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_THUMB_ENC_E));
	DBG_DUMP("Thumb Enc = %lu ms\n", tick / 1000);

	/* qview */
	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_QVIEW_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_QVIEW_E));
	DBG_DUMP("Quick View = %lu ms\n", tick / 1000);

	/* combine & exif */
	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_COMBINE_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_COMBINE_E));
	DBG_DUMP("Combine & Exif = %lu ms\n", tick / 1000);

	/* write file */
	tick = vos_perf_duration(SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_WR_S), SLICE_ENC_VOS_TICK_ELE(SLICE_ENC_VOS_TICK_WR_E));
	DBG_DUMP("Write File = %lu ms\n", tick / 1000);
#endif
}

INT32 PhotoExe_Preview_SliceEncode(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	HD_RESULT ret = HD_OK;
	PHOTO_VID_IN vid_in = PHOTO_VID_IN_1;
	PhotoExe_SliceEncode_Info* slice_encode_primary_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_PRIMARY);
	PhotoExe_SliceEncode_Info* slice_encode_screennail_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_SCREENNAIL);
	PhotoExe_SliceEncode_Info* slice_encode_thumbnail_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_THUMBNAIL);
	PhotoExe_SliceSize_Info src_slice_info = {0}, dst_slice_info = {0};
	HD_VIDEO_FRAME video_frame = {0};
	HD_VIDEO_FRAME video_frame_out_screennail = {0};
	HD_VIDEO_FRAME video_frame_out_thumbnail = {0};
	HD_PATH_ID vproc_path_id = 0;
	HD_VIDEOPROC_OUT vproc_out = {0};
	HD_VIDEO_PXLFMT vproc_out_pxlfmt = 0;
	PhotoExe_MEM_Info exif_mem_info = {0};
	BOOL last_shot = TRUE;
	UINT32 enc_accum_size = 0;
	static UINT8 primary_quality = CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_PRIMARY;
	static UINT8 screennail_quality = CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_SCREENNAIL;


	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_S);

	/* fetch parameters */
	if(paramNum >= 1){
		vid_in = (PHOTO_VID_IN) paramArray[0]; /* sensor id */
	}

	if(paramNum >= 2){
		last_shot = (BOOL) paramArray[1]; /* for cont shot */
	}

	DBG_DUMP("slice encode vid_in = %lu last_shot = %lu\r\n", vid_in, last_shot);

	/*******************************************************************
	 * Get vproc info
	 ******************************************************************/

	/* 3dnr ref path output sensor sized frame */
	ImageApp_Photo_Get_Hdal_Path(vid_in, PHOTO_HDAL_VPRC_3DNR_REF_PATH, (UINT32 *)&vproc_path_id);

	/* get vproc out frame format ( = venc in) */
	ret = hd_videoproc_get(vproc_path_id, HD_VIDEOPROC_PARAM_OUT, (VOID*)&vproc_out);
	if(ret != HD_OK){
		DBG_ERR("hd_videoproc_get HD_VIDEOPROC_PARAM_OUT failed(path_id=%lx, ret=%d)!", vproc_path_id, ret);
		goto EXIT;
	}

	vproc_out_pxlfmt = vproc_out.pxlfmt;

	/*******************************************************************
	 * Pull out preview video frame, size should equal to sensor size
	 ******************************************************************/
	ret = hd_videoproc_pull_out_buf(vproc_path_id, &video_frame, -1);
	if(ret != HD_OK){
		DBG_ERR("hd_videoproc_pull_out_buf failed!(path_id=%lx, ret=%d)", vproc_path_id, ret);
		goto EXIT;
	}

	/*******************************************************************
	 * Calculate dst slice info
	 ******************************************************************/
	if(PhotoExe_Preview_SliceEncode_Get_Curr_Dst_Slice_Info(&dst_slice_info, video_frame) != E_OK){
		return NVTEVT_CONSUME;
	}

	/*******************************************************************
	 * Calculate src slice info
	 ******************************************************************/
	PhotoExe_Preview_SliceEncode_Get_Src_Slice_Info(&dst_slice_info, &src_slice_info, video_frame);

	/*******************************************************************
	 * Init Date Buffer
	 ******************************************************************/
	if (UI_GetData(FL_DATE_STAMP) != DATEIMPRINT_OFF) {
		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_INIT_DATE_BUF, 0);
	}

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == ENABLE
	/*******************************************************************
	 * Stamp on the src frame
	 ******************************************************************/
	if(PhotoExe_Preview_SliceEncode_Src_DateStamp(&video_frame, &src_slice_info, &dst_slice_info) != E_OK){
		goto EXIT;
	}
#endif

#if	PHOTO_SLICE_ENC_DBG_PRIMARY_YUV

	PhotoExe_Preview_SliceEncode_Dump_Frame(video_frame);

#endif

	/*******************************************************************
	 * Allocate slice buffer
	 ******************************************************************/
	slice_encode_thumbnail_info->bs_buf_mem_info.blk_size =  (CFG_THUMBNAIL_W * CFG_THUMBNAIL_H) / 2;
	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&slice_encode_thumbnail_info->bs_buf_mem_info, "slice_enc_thumbnail") != E_OK){
		goto EXIT;
	}


#if CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL
	slice_encode_screennail_info->bs_buf_mem_info.blk_size =  CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL_UBOUND;
#else
	slice_encode_screennail_info->bs_buf_mem_info.blk_size =  PhotoExe_GetScreenNailSize();
#endif

	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&slice_encode_screennail_info->bs_buf_mem_info, "slice_enc_screennail") != E_OK){
		goto EXIT;
	}

	slice_encode_primary_info->bs_buf_mem_info.blk_size =  (VDO_YUV_BUFSIZE(dst_slice_info.width, dst_slice_info.height, video_frame.pxlfmt) / (CFG_JPG_PREVIEW_SLICE_ENC_BS_BUF_RATIO)) + CFG_JPG_HEADER_SIZE + PhotoExe_GetScreenNailSize() ;
	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&slice_encode_primary_info->bs_buf_mem_info, "slice_enc_primary") != E_OK){
		goto EXIT;
	}

	/*******************************************************************
	 * Screennail & Thumbnail Scale
	 ******************************************************************/

	/* screennail scale */
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_SCR_SCALE_S);

	if(PhotoExe_Preview_SliceEncode_Scale_Screennail(&video_frame, &video_frame_out_screennail) != E_OK){
		goto EXIT;
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_SCR_SCALE_E);

	/* thumbnail scale, 2pass from screennail */
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_THUMB_SCALE_S);

	if(PhotoExe_Preview_SliceEncode_Scale_Thumbnail(&video_frame_out_screennail, &video_frame_out_thumbnail) != E_OK){
		goto EXIT;
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_THUMB_SCALE_E);

	/* screennail date stamp */
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_THUMB_STAMP_S);

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == DISABLE
	if(PhotoExe_Preview_SliceEncode_DateStamp(&video_frame_out_screennail, CAP_DS_EVENT_SCR) != E_OK){
		goto EXIT;
	}
#endif

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_THUMB_STAMP_E);

	/*******************************************************************
	 * Quick View (screennail)
	 ******************************************************************/
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_QVIEW_S);

	PhotoExe_Preview_SliceEncode_QView(&video_frame_out_screennail);

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_QVIEW_E);

	/*******************************************************************
	 * Primary
	 ******************************************************************/

	/* slice encode */
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_PRI_ENC_S);

	PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_primary_info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {dst_slice_info.width, dst_slice_info.height});
	PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_primary_info->enc_path_id, primary_quality);

	if(PhotoExe_Preview_SliceEncode_Encode_Primary(&video_frame, src_slice_info, dst_slice_info, &enc_accum_size, &primary_quality) != E_OK){
		goto EXIT;
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_PRI_ENC_E);

	slice_encode_primary_info->bs_buf_mem_info.used_size = enc_accum_size;

#if PHOTO_SLICE_ENC_DBG_PRIMARY_JPG

	PhotoExe_MEM_Info bs_buf_mem_info = slice_encode_primary_info->bs_buf_mem_info;

	/* skip header and write accum size */
	bs_buf_mem_info.va += CFG_JPG_HEADER_SIZE;

	/* dump file */
	PhotoExe_Preview_SliceEncode_Dump_JPG(&bs_buf_mem_info, "primary");
#endif

	/*******************************************************************
	 * Screennail & Thumbnail Encode
	 ******************************************************************/

	/* screennail encode */
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_SCR_ENC_S);

	PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_screennail_info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {CFG_SCREENNAIL_W, CFG_SCREENNAIL_H});

#if CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL
	if(PhotoExe_Preview_SliceEncode_Encode_Screennail_RC(&video_frame_out_screennail, &screennail_quality) != E_OK){
		goto EXIT;
	}
#else
	PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_screennail_info->enc_path_id, CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_SCREENNAIL);

	if(PhotoExe_Preview_SliceEncode_Encode_Screennail(&video_frame_out_screennail) != E_OK){
		goto EXIT;
	}
#endif

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_SCR_ENC_E);

#if PHOTO_SLICE_ENC_DBG_SCREENNAIL_JPG
	/* dump file */
	PhotoExe_Preview_SliceEncode_Dump_JPG(&slice_encode_screennail_info->bs_buf_mem_info, "screennail");
#endif

	/* thumbnail encode */
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_THUMB_ENC_S);

	PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_thumbnail_info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {CFG_THUMBNAIL_W, CFG_THUMBNAIL_H});
	PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_thumbnail_info->enc_path_id, CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_THUMBNAIL);

	if(PhotoExe_Preview_SliceEncode_Encode_Thumbnail(&video_frame_out_thumbnail) != E_OK){
		goto EXIT;
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_THUMB_ENC_E);

#if PHOTO_SLICE_ENC_DBG_THUMBNAIL_JPG
	/* dump file */
	PhotoExe_Preview_SliceEncode_Dump_JPG(&slice_encode_thumbnail_info->bs_buf_mem_info, "thumbnail");
#endif

	/*******************************************************************
	 * Combine All Images
	 ******************************************************************/
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_COMBINE_S);

	exif_mem_info.blk_size = CFG_JPG_HEADER_SIZE;
	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&exif_mem_info, "slice_enc_exif") != E_OK)
		goto EXIT;

	MEM_RANGE exif_data = 		{.addr = exif_mem_info.va, .size = exif_mem_info.blk_size};
	MEM_RANGE thumb_jpg = 		{.addr = slice_encode_thumbnail_info->bs_buf_mem_info.va, .size = slice_encode_thumbnail_info->bs_buf_mem_info.used_size};
	MEM_RANGE pri_jpg = 		{.addr = slice_encode_primary_info->bs_buf_mem_info.va + CFG_JPG_HEADER_SIZE, .size = slice_encode_primary_info->bs_buf_mem_info.used_size};
	MEM_RANGE scr_jpg = 		{.addr = slice_encode_screennail_info->bs_buf_mem_info.va, .size = slice_encode_screennail_info->bs_buf_mem_info.used_size};
	MEM_RANGE dst_jpg_file = 	{ 0 };

	/* exif */
	ExifVendor_Write0thIFD(EXIF_HDL_ID_1);
	ExifVendor_WriteExifIFD(EXIF_HDL_ID_1);
	ExifVendor_Write0thIntIFD(EXIF_HDL_ID_1);
	if (EXIF_CreateExif(EXIF_HDL_ID_1, &exif_data, &thumb_jpg) != EXIF_ER_OK) {
		DBG_ERR("Create Exif fail\r\n");
		exif_data.size = 0;
	}

	/* combine jpg */
	GxImgFile_CombineJPG(&exif_data, &pri_jpg, &scr_jpg, &dst_jpg_file);

	if(PhotoCapMsgCb){
		PhotoCapMsgCb(IMG_CAP_CBMSG_JPEG_OK, NULL);
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_COMBINE_E);

	PHOTO_SLICE_ENC_DUMP("exif{%lx, %lx} thumb{%lx, %lx} primary{%lx, %lx} scr{%lx, %lx} dst{%lx, %lx}\r\n",
			exif_data.addr, exif_data.size,
			thumb_jpg.addr, thumb_jpg.size,
			pri_jpg.addr, 	pri_jpg.size,
			exif_data.addr, exif_data.size,
			dst_jpg_file.addr, dst_jpg_file.size
	);

	/*******************************************************************
	 * Output jpg file
	 ******************************************************************/
	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_WR_S);

	extern INT32 ImageApp_Photo_WriteCB(UINT32 Addr, UINT32 Size, UINT32 Fmt, UINT32 uiPathId);
	IMG_CAP_FST_INFO FstStatus = {FST_STA_OK};

	if(dst_jpg_file.addr > (slice_encode_primary_info->bs_buf_mem_info.va + slice_encode_primary_info->bs_buf_mem_info.blk_size)){
		DBG_ERR("primary buffer overflow during combine jpg!\r\n");
	}

	FstStatus.Status = ImageApp_Photo_WriteCB(
			dst_jpg_file.addr,
			dst_jpg_file.size,
			HD_CODEC_TYPE_JPEG, 0);

	if(PhotoCapMsgCb){
		PhotoCapMsgCb(IMG_CAP_CBMSG_FSTOK, &FstStatus);
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_WR_E);


	/*******************************************************************
	 * Output heic file
	 ******************************************************************/
#if HEIC_FUNC == ENABLE

	HD_VIDEO_FRAME video_frame_out_heic = {0};
	PhotoExe_MEM_Info heic_bs_buf_mem_info = {0};
	HD_VIDEOENC_BS heic_bs_data = {0};

	if(	(src_slice_info.width != CFG_JPG_PREVIEW_SLICE_ENC_HEIC_WIDTH) ||
		(src_slice_info.height != CFG_JPG_PREVIEW_SLICE_ENC_HEIC_HEIGHT)){

		if(PhotoExe_Preview_SliceEncode_Scale_HEIC(&video_frame, &video_frame_out_heic) != HD_OK){
			goto EXIT;
		}
	}
	else{
		memcpy(&video_frame_out_heic, &video_frame, sizeof(HD_VIDEO_FRAME));
	}

	PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_primary_info->enc_path_id_heic, vproc_out_pxlfmt, (HD_DIM) {CFG_JPG_PREVIEW_SLICE_ENC_HEIC_WIDTH, CFG_JPG_PREVIEW_SLICE_ENC_HEIC_HEIGHT});
	PhotoExe_Preview_SliceEncode_Encode_Set_Out_HEIC(slice_encode_primary_info->enc_path_id_heic);

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == DISABLE
	if(PhotoExe_Preview_SliceEncode_DateStamp(&video_frame_out_heic, CAP_DS_EVENT_PRI) != E_OK){
		goto EXIT;
	}
#endif

	if(PhotoExe_Preview_SliceEncode_Encode_Primary_HEIC(&video_frame_out_heic, &heic_bs_buf_mem_info, &heic_bs_data) != E_OK){
		goto EXIT;
	}

	if(heic_bs_buf_mem_info.blk_size && heic_bs_buf_mem_info.va){
		char file_path[NMC_TOTALFILEPATH_MAX_LEN] = {'\0'};
		PhotoExe_Preview_SliceEncode_Naming_HEIC(file_path);
		PhotoExe_Preview_SliceEncode_WriteFile_HEIC(&heic_bs_data, file_path);
	}

#endif

EXIT:

#if HEIC_FUNC == ENABLE
	if(heic_bs_buf_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&heic_bs_buf_mem_info);
	}
#endif

	/* check user bs buffer is freed */
	if(slice_encode_primary_info->bs_buf_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&slice_encode_primary_info->bs_buf_mem_info);
	}

	if(slice_encode_screennail_info->bs_buf_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&slice_encode_screennail_info->bs_buf_mem_info);
	}

	if(slice_encode_thumbnail_info->bs_buf_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&slice_encode_thumbnail_info->bs_buf_mem_info);
	}

	/* check exif buffer is freed */
	if(exif_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&exif_mem_info);
	}

	/* check is vproc frame released */
	if(video_frame.phy_addr[0]){

		if ((ret = hd_videoproc_release_out_buf(vproc_path_id, &video_frame))!= HD_OK) {
			DBG_ERR("hd_videoproc_release_out_buf failed(%d)\r\n", ret);
		}
	}

	if(PhotoCapMsgCb && (last_shot == TRUE)){
		PhotoCapMsgCb(IMG_CAP_CBMSG_RET_PRV, NULL);
		PhotoCapMsgCb(IMG_CAP_CBMSG_CAPEND, NULL);
	}

	SLICE_ENC_VOS_TICK_TRIG(SLICE_ENC_VOS_TICK_E);

	PhotoExe_Preview_SliceEncode_Perf_Result(&dst_slice_info);

	return NVTEVT_CONSUME;
}


#if PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC == ENABLE

/* task update */
#define FLG_PHOTO_SLICE_ENC_STA_IDLE        FLGPTN_BIT(0)
#define FLG_PHOTO_SLICE_ENC_STA_STOPPED     FLGPTN_BIT(1)
#define FLG_PHOTO_SLICE_ENC_STA_STARTED     FLGPTN_BIT(2)

/* caller update */
#define FLG_PHOTO_SLICE_ENC_CMD_START       FLGPTN_BIT(3)
#define FLG_PHOTO_SLICE_ENC_CMD_STOP        FLGPTN_BIT(4)

#include "../lfqueue/lfqueue.h"

typedef struct {
	THREAD_HANDLE task_id;
	UINT run;
	UINT is_running;
	ID flag_id;
	void *user_data;
	INT32 (*callback)(void* user_data);
} PhotoExe_Preview_SliceEncode_Task_Param;

THREAD_RETTYPE _PhotoExe_Preview_SliceEncode_Worker(void* arg)
{
	PhotoExe_Preview_SliceEncode_Task_Param* param = (PhotoExe_Preview_SliceEncode_Task_Param*) arg;
	FLGPTN			flag = 0, wait_flag = 0;

	if(param == NULL)
		goto EXIT;

	wait_flag = FLG_PHOTO_SLICE_ENC_CMD_START | FLG_PHOTO_SLICE_ENC_CMD_STOP ;

	clr_flg(param->flag_id, FLG_PHOTO_SLICE_ENC_STA_IDLE);

	while(param->run)
	{
		param->is_running = 1;

		set_flg(param->flag_id, FLG_PHOTO_SLICE_ENC_STA_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&flag, param->flag_id, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(param->flag_id, FLG_PHOTO_SLICE_ENC_STA_IDLE);

		if(flag & FLG_PHOTO_SLICE_ENC_CMD_STOP){
			break;
		}

		if(flag & FLG_PHOTO_SLICE_ENC_CMD_START){
			set_flg(param->flag_id, FLG_PHOTO_SLICE_ENC_STA_STARTED);
			if(param->callback){
				param->callback(param->user_data);
			}
		}
	}

EXIT:
	param->is_running = 0;

	set_flg(param->flag_id, FLG_PHOTO_SLICE_ENC_STA_STOPPED);

	THREAD_RETURN(0);
}

typedef struct {
	UINT8 terminate:1;
} PhotoExe_Preview_SliceEncode_Queue_Comm_Param;

typedef struct {
	HD_VIDEO_FRAME frame;
	PhotoExe_Preview_SliceEncode_Queue_Comm_Param comm;
} PhotoExe_Preview_SliceEncode_Queue12_Param;

typedef struct {
	UINT8* jpg_combined_addr;
	UINT32 jpg_combined_size;
	UINT8* jpg_thumb_addr;
	UINT32 jpg_thumb_size;
	PhotoExe_MEM_Info mem_info_combined;
	PhotoExe_MEM_Info mem_info_thumb;
	PhotoExe_Preview_SliceEncode_Queue_Comm_Param comm;

#if HEIC_FUNC == ENABLE
	PhotoExe_MEM_Info heic_bs_buf_mem_info;
	HD_VIDEOENC_BS heic_bs_data;
#endif
} PhotoExe_Preview_SliceEncode_Queue23_Param;

typedef struct {
	UINT8 max_cnt;
	UINT8 cnt;
	lfqueue_t* queue12;			/* shared with CB2 */
	HD_PATH_ID vproc_path_id;
	UINT32 period;
} PhotoExe_Preview_SliceEncode_CB1_Param;

typedef struct {
	UINT8 max_cnt;
	UINT8 cnt;
	lfqueue_t* queue12;			/* shared with CB1 */
	lfqueue_t* queue23;			/* shared with CB3 */
	HD_PATH_ID vproc_path_id;
} PhotoExe_Preview_SliceEncode_CB2_Param;

typedef struct {
	UINT8 max_cnt;
	UINT8 cnt;
	lfqueue_t* queue23;			/* shared with CB2 */
} PhotoExe_Preview_SliceEncode_CB3_Param;

static VOID PhotoExe_Preview_SliceEncode2_Close(VOID);

INT32 PhotoExe_Preview_SliceEncode_CB1(void* user_data)
{
	INT32 ret = E_OK;
	PhotoExe_Preview_SliceEncode_CB1_Param* param = NULL;
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEO_FRAME video_frame;
	PHOTO_VID_IN vid_in = PHOTO_VID_IN_1;
	HD_PATH_ID vproc_path_id;
	PhotoExe_Preview_SliceEncode_Queue12_Param* queue_ele_out = NULL;
	VOS_TICK tick = 0;
	UINT32 err_cnt = 0, err_timeout = 30;

	if(!user_data){
		DBG_ERR("user_data can't be null!\n");
		ret = E_SYS;
		goto exit;
	}

	param = (PhotoExe_Preview_SliceEncode_CB1_Param*) user_data;
	param->cnt = 0;

	/*******************************************************************
	 * Get vproc info
	 ******************************************************************/

	/* 3dnr ref path output sensor sized frame */
	ImageApp_Photo_Get_Hdal_Path(vid_in, PHOTO_HDAL_VPRC_3DNR_REF_PATH, (UINT32 *)&vproc_path_id);

	do{

		if(param->cnt)
			vos_util_delay_ms(param->period);

		hd_ret = hd_videoproc_pull_out_buf(vproc_path_id, &video_frame, -1);
		if(hd_ret != HD_OK){
			err_cnt++;
			if(err_cnt >= err_timeout){
				DBG_ERR("hd_videoproc_pull_out_buf failed!(path_id=%lx, ret=%d)", param->vproc_path_id, hd_ret);
				goto exit;
			}
			else{
				DBG_WRN("hd_videoproc_pull_out_buf failed!(path_id=%lx, ret=%d), retrying ...", param->vproc_path_id, hd_ret);
				continue;
			}
		}

		vos_perf_mark(&tick);

		DBG_DUMP("pic%lu = %lu\n", param->cnt, tick);

		queue_ele_out = (PhotoExe_Preview_SliceEncode_Queue12_Param*) malloc(sizeof(PhotoExe_Preview_SliceEncode_Queue12_Param));
		memset(queue_ele_out, 0, sizeof(PhotoExe_Preview_SliceEncode_Queue12_Param));
		queue_ele_out->frame = video_frame;

		while (lfqueue_enq(param->queue12, (void*) queue_ele_out) == -1) {
			vos_util_delay_ms(1);
		    DBG_ERR("ENQ Full ?\r\n");
		}

		param->cnt++;
		if(param->cnt >= param->max_cnt)
			break;

	} while(1);

	DBG_DUMP("task1 job finished\n");

exit:
	return ret;
}

INT32 PhotoExe_Preview_SliceEncode_CB2(void* user_data)
{
	INT32 ret = E_OK;
	PhotoExe_Preview_SliceEncode_CB2_Param* param = NULL;
	PhotoExe_Preview_SliceEncode_Queue12_Param* queue_ele_in = NULL;
	PhotoExe_Preview_SliceEncode_Queue23_Param* queue_ele_out = NULL;
	HD_RESULT hd_ret = HD_OK;
	PHOTO_VID_IN vid_in = PHOTO_VID_IN_1;
	PhotoExe_SliceEncode_Info* slice_encode_primary_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_PRIMARY);
	PhotoExe_SliceEncode_Info* slice_encode_screennail_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_SCREENNAIL);
	PhotoExe_SliceEncode_Info* slice_encode_thumbnail_info = PhotoExe_Preview_SliceEncode_Get_Info(PHOTO_ENC_JPG_THUMBNAIL);
	PhotoExe_SliceSize_Info src_slice_info = {0}, dst_slice_info = {0};
	HD_VIDEO_FRAME video_frame_out_screennail = {0};
	HD_VIDEO_FRAME video_frame_out_thumbnail = {0};
	HD_PATH_ID vproc_path_id = 0;
	HD_VIDEOPROC_OUT vproc_out = {0};
	HD_VIDEO_PXLFMT vproc_out_pxlfmt = 0;
	PhotoExe_MEM_Info exif_mem_info = {0};
	UINT32 enc_accum_size = 0;
	static UINT8 primary_quality = CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_PRIMARY;
	static UINT8 screennail_quality = CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_SCREENNAIL;

	if(!user_data){
		DBG_ERR("user_data can't be null!\n");
		ret = E_SYS;
		goto EXIT;
	}

	param = (PhotoExe_Preview_SliceEncode_CB2_Param*) user_data;
	param->cnt = 0;

	/*******************************************************************
	 * Get vproc info
	 ******************************************************************/

	/* 3dnr ref path output sensor sized frame */
	ImageApp_Photo_Get_Hdal_Path(vid_in, PHOTO_HDAL_VPRC_3DNR_REF_PATH, (UINT32 *)&vproc_path_id);

	/* get vproc out frame format ( = venc in) */
	hd_ret = hd_videoproc_get(vproc_path_id, HD_VIDEOPROC_PARAM_OUT, (VOID*)&vproc_out);
	if(hd_ret != HD_OK){
		DBG_ERR("hd_videoproc_get HD_VIDEOPROC_PARAM_OUT failed(path_id=%lx, ret=%d)!", vproc_path_id, ret);
		goto EXIT;
	}

	vproc_out_pxlfmt = vproc_out.pxlfmt;

	/* pull out vprc frame is in the CB1 */

	/*******************************************************************
	 * Init Date Buffer
	 ******************************************************************/
	if (UI_GetData(FL_DATE_STAMP) != DATEIMPRINT_OFF) {
		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_INIT_DATE_BUF, 0);
	}

	/*******************************************************************
	 * Allocate slice buffer
	 ******************************************************************/
	slice_encode_thumbnail_info->bs_buf_mem_info.blk_size =  (CFG_THUMBNAIL_W * CFG_THUMBNAIL_H) / 2;
	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&slice_encode_thumbnail_info->bs_buf_mem_info, "slice_enc_thumbnail") != E_OK){
		goto EXIT;
	}

//	slice_encode_screennail_info->bs_buf_mem_info.blk_size =  PhotoExe_GetScreenNailSize();
//	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&slice_encode_screennail_info->bs_buf_mem_info, "slice_enc_screennail") != E_OK){
//		goto EXIT;
//	}

	exif_mem_info.blk_size = CFG_JPG_HEADER_SIZE;
	if(PhotoExe_Preview_SliceEncode_Alloc_Buffer(&exif_mem_info, "slice_enc_exif") != E_OK)
		goto EXIT;

	do{

		queue_ele_in = (PhotoExe_Preview_SliceEncode_Queue12_Param*)lfqueue_deq(param->queue12);
		if(queue_ele_in == NULL){
			vos_util_delay_ms(1);
			continue;
		}

		if(queue_ele_in->comm.terminate){
			DBG_ERR("abort CB2\n");
			free(queue_ele_in);
			queue_ele_in = NULL;
			goto EXIT;
		}

		/*******************************************************************
		 * Calculate dst slice info
		 ******************************************************************/
		if(PhotoExe_Preview_SliceEncode_Get_Curr_Dst_Slice_Info(&dst_slice_info, queue_ele_in->frame) != E_OK){
			return NVTEVT_CONSUME;
		}

		/*******************************************************************
		 * Calculate src slice info
		 ******************************************************************/
		PhotoExe_Preview_SliceEncode_Get_Src_Slice_Info(&dst_slice_info, &src_slice_info, queue_ele_in->frame);

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == ENABLE
		/*******************************************************************
		 * Stamp on the src frame
		 ******************************************************************/
		if(PhotoExe_Preview_SliceEncode_Src_DateStamp(&queue_ele_in->frame, &src_slice_info, &dst_slice_info) != E_OK){
			goto EXIT;
		}
#endif

#if	PHOTO_SLICE_ENC_DBG_PRIMARY_YUV

	PhotoExe_Preview_SliceEncode_Dump_Frame(queue_ele_in->frame);

#endif

		slice_encode_primary_info->bs_buf_mem_info.blk_size =  (VDO_YUV_BUFSIZE(dst_slice_info.width, dst_slice_info.height, queue_ele_in->frame.pxlfmt) / (CFG_JPG_PREVIEW_SLICE_ENC_BS_BUF_RATIO)) + CFG_JPG_HEADER_SIZE + PhotoExe_GetScreenNailSize() ;
		if(PhotoExe_Preview_SliceEncode_Alloc_Buffer_Retry(
				&slice_encode_primary_info->bs_buf_mem_info,
				"slice_enc_primary",
				10,
				100
		) != E_OK){
			goto EXIT;
		}

		slice_encode_screennail_info->bs_buf_mem_info.blk_size =  PhotoExe_GetScreenNailSize();
		if(PhotoExe_Preview_SliceEncode_Alloc_Buffer_Retry(
				&slice_encode_screennail_info->bs_buf_mem_info,
				"slice_enc_screennail",
				10,
				100
		) != E_OK){
			goto EXIT;
		}

		/*******************************************************************
		 * Screennail & Thumbnail Scale
		 ******************************************************************/

		/* screennail scale */
		if(PhotoExe_Preview_SliceEncode_Scale_Screennail(&queue_ele_in->frame, &video_frame_out_screennail) != E_OK){
			goto EXIT;
		}

		/* thumbnail scale, 2pass from screennail */
		if(PhotoExe_Preview_SliceEncode_Scale_Thumbnail(&video_frame_out_screennail, &video_frame_out_thumbnail) != E_OK){
			goto EXIT;
		}

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == DISABLE
		/* screennail date stamp */
		if(PhotoExe_Preview_SliceEncode_DateStamp(&video_frame_out_screennail, CAP_DS_EVENT_SCR) != E_OK){
			goto EXIT;
		}

		/* thumbnail date stamp */
//		if(PhotoExe_Preview_SliceEncode_DateStamp(&video_frame_out_thumbnail, CAP_DS_EVENT_QV) != E_OK){
//			goto EXIT;
//		}
#endif
		/*******************************************************************
		 * Primary
		 ******************************************************************/

		/* slice encode */
		PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_primary_info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {dst_slice_info.width, dst_slice_info.height});
		PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_primary_info->enc_path_id, primary_quality);

		if(PhotoExe_Preview_SliceEncode_Encode_Primary(&queue_ele_in->frame, src_slice_info, dst_slice_info, &enc_accum_size, &primary_quality) != E_OK){
			goto EXIT;
		}

		slice_encode_primary_info->bs_buf_mem_info.used_size = enc_accum_size;

		/*******************************************************************
		 * Screennail & Thumbnail Encode
		 ******************************************************************/

		/* screennail encode */
		PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_screennail_info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {CFG_SCREENNAIL_W, CFG_SCREENNAIL_H});

#if CFG_JPG_PREVIEW_SLICE_ENC_RC_SCREENNAIL
	if(PhotoExe_Preview_SliceEncode_Encode_Screennail_RC(&video_frame_out_screennail, &screennail_quality) != E_OK){
		goto EXIT;
	}
#else
	PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_screennail_info->enc_path_id, CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_SCREENNAIL);

	if(PhotoExe_Preview_SliceEncode_Encode_Screennail(&video_frame_out_screennail) != E_OK){
		goto EXIT;
	}
#endif

		/* thumbnail encode */
		PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_thumbnail_info->enc_path_id, vproc_out_pxlfmt, (HD_DIM) {CFG_THUMBNAIL_W, CFG_THUMBNAIL_H});
		PhotoExe_Preview_SliceEncode_Encode_Set_Out(slice_encode_thumbnail_info->enc_path_id, CFG_JPG_PREVIEW_SLICE_ENC_INIT_QUALITY_THUMBNAIL);

		if(PhotoExe_Preview_SliceEncode_Encode_Thumbnail(&video_frame_out_thumbnail) != E_OK){
			goto EXIT;
		}

		/*******************************************************************
		 * Combine All Images
		 ******************************************************************/
		MEM_RANGE exif_data = 		{.addr = exif_mem_info.va, .size = exif_mem_info.blk_size};
		MEM_RANGE thumb_jpg = 		{.addr = slice_encode_thumbnail_info->bs_buf_mem_info.va, .size = slice_encode_thumbnail_info->bs_buf_mem_info.used_size};
		MEM_RANGE pri_jpg = 		{.addr = slice_encode_primary_info->bs_buf_mem_info.va + CFG_JPG_HEADER_SIZE, .size = slice_encode_primary_info->bs_buf_mem_info.used_size};
		MEM_RANGE scr_jpg = 		{.addr = slice_encode_screennail_info->bs_buf_mem_info.va, .size = slice_encode_screennail_info->bs_buf_mem_info.used_size};
		MEM_RANGE dst_jpg_file = 	{ 0 };

		/* exif */
		ExifVendor_Write0thIFD(EXIF_HDL_ID_1);
		ExifVendor_WriteExifIFD(EXIF_HDL_ID_1);
		ExifVendor_Write0thIntIFD(EXIF_HDL_ID_1);
		if (EXIF_CreateExif(EXIF_HDL_ID_1, &exif_data, &thumb_jpg) != EXIF_ER_OK) {
			DBG_ERR("Create Exif fail\r\n");
			exif_data.size = 0;
		}

		/* combine jpg */
		GxImgFile_CombineJPG(&exif_data, &pri_jpg, &scr_jpg, &dst_jpg_file);

		if(PhotoCapMsgCb){
			PhotoCapMsgCb(IMG_CAP_CBMSG_JPEG_OK, NULL);
		}

		/* check overflow */
		if(dst_jpg_file.addr > (slice_encode_primary_info->bs_buf_mem_info.va + slice_encode_primary_info->bs_buf_mem_info.blk_size)){
			DBG_ERR("primary buffer overflow during combine jpg!\r\n");
		}

		/*******************************************************************
		 * Encode HEIC
		 ******************************************************************/
#if HEIC_FUNC == ENABLE

		HD_VIDEO_FRAME video_frame_out_heic = {0};
		PhotoExe_MEM_Info heic_bs_buf_mem_info = {0};
		HD_VIDEOENC_BS heic_bs_data = {0};

		if(	(src_slice_info.width != CFG_JPG_PREVIEW_SLICE_ENC_HEIC_WIDTH) ||
			(src_slice_info.height != CFG_JPG_PREVIEW_SLICE_ENC_HEIC_HEIGHT)){

			if(PhotoExe_Preview_SliceEncode_Scale_HEIC(&queue_ele_in->frame, &video_frame_out_heic) != HD_OK){
				goto EXIT;
			}
		}
		else{
			memcpy(&video_frame_out_heic, &queue_ele_in->frame, sizeof(HD_VIDEO_FRAME));
		}

		PhotoExe_Preview_SliceEncode_Encode_Set_In(slice_encode_primary_info->enc_path_id_heic, vproc_out_pxlfmt, (HD_DIM) {CFG_JPG_PREVIEW_SLICE_ENC_HEIC_WIDTH, CFG_JPG_PREVIEW_SLICE_ENC_HEIC_HEIGHT});
		PhotoExe_Preview_SliceEncode_Encode_Set_Out_HEIC(slice_encode_primary_info->enc_path_id_heic);

#if PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP == DISABLE

		if(PhotoExe_Preview_SliceEncode_DateStamp(&video_frame_out_heic, CAP_DS_EVENT_PRI) != E_OK){
			goto EXIT;
		}

#endif

		if(PhotoExe_Preview_SliceEncode_Encode_Primary_HEIC(&video_frame_out_heic, &heic_bs_buf_mem_info, &heic_bs_data) != E_OK){
			goto EXIT;
		}

#endif /* #if HEIC_FUNC == ENABLE */

		/* enqueue jpg buffer info */
		queue_ele_out = (PhotoExe_Preview_SliceEncode_Queue23_Param*) malloc(sizeof(PhotoExe_Preview_SliceEncode_Queue23_Param));
		memset(queue_ele_out, 0, sizeof(PhotoExe_Preview_SliceEncode_Queue23_Param));
		queue_ele_out->jpg_combined_addr = (UINT8*) dst_jpg_file.addr;
		queue_ele_out->jpg_combined_size = dst_jpg_file.size;
		queue_ele_out->jpg_thumb_addr = (UINT8*) scr_jpg.addr;
		queue_ele_out->jpg_thumb_size = scr_jpg.size;
		queue_ele_out->mem_info_combined = slice_encode_primary_info->bs_buf_mem_info;
		queue_ele_out->mem_info_thumb = slice_encode_screennail_info->bs_buf_mem_info;

#if HEIC_FUNC == ENABLE
		queue_ele_out->heic_bs_buf_mem_info = heic_bs_buf_mem_info;
		queue_ele_out->heic_bs_data = heic_bs_data;
#endif

		while (lfqueue_enq(param->queue23, (void*) queue_ele_out) == -1)
		{
		    DBG_ERR("ENQ Full ?\r\n");
		}

		/* check is vproc frame released */
		if(queue_ele_in->frame.phy_addr[0]){

			if ((ret = hd_videoproc_release_out_buf(vproc_path_id, &(queue_ele_in->frame)))!= HD_OK) {
				DBG_ERR("hd_videoproc_release_out_buf failed(%d)\r\n", ret);
			}
		}

		if(queue_ele_in){
			free(queue_ele_in);
			queue_ele_in = NULL;
		}

		param->cnt++;
		if(param->cnt >= param->max_cnt)
			break;

	} while(1);

EXIT:

//	if(slice_encode_screennail_info->bs_buf_mem_info.va){
//		PhotoExe_Preview_SliceEncode_Free_Buffer(&slice_encode_screennail_info->bs_buf_mem_info);
//	}

	if(slice_encode_thumbnail_info->bs_buf_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&slice_encode_thumbnail_info->bs_buf_mem_info);
	}

	/* check exif buffer is freed */
	if(exif_mem_info.va){
		PhotoExe_Preview_SliceEncode_Free_Buffer(&exif_mem_info);
	}

	if(param->cnt < param->max_cnt){
		queue_ele_out = (PhotoExe_Preview_SliceEncode_Queue23_Param*) malloc(sizeof(PhotoExe_Preview_SliceEncode_Queue23_Param));
		memset(queue_ele_out, 0, sizeof(PhotoExe_Preview_SliceEncode_Queue23_Param));
		queue_ele_out->comm.terminate = 1;
		lfqueue_enq(param->queue23, (void*) queue_ele_out);
	}

	DBG_DUMP("task2 job finished\n");

	return ret;
}

INT32 PhotoExe_Preview_SliceEncode_CB3(void* user_data)
{
	INT32 ret = E_OK;
	PhotoExe_Preview_SliceEncode_CB3_Param* param = NULL;
	PhotoExe_Preview_SliceEncode_Queue23_Param* queue_ele_in = NULL;

	if(!user_data){
		DBG_ERR("user_data can't be null!\n");
		ret = E_SYS;
		goto EXIT;
	}

	param = (PhotoExe_Preview_SliceEncode_CB3_Param*) user_data;
	param->cnt = 0;

	do{

		queue_ele_in = (PhotoExe_Preview_SliceEncode_Queue23_Param*)lfqueue_deq(param->queue23);
		if(queue_ele_in == NULL){
			vos_util_delay_ms(1);
			continue;
		}

		if(queue_ele_in->comm.terminate){
			DBG_ERR("abort CB3\n");
			free(queue_ele_in);
			queue_ele_in = NULL;
			goto EXIT;
		}

		/*******************************************************************
		 * Output jpg file
		 ******************************************************************/
		extern INT32 ImageApp_Photo_WriteCB(UINT32 Addr, UINT32 Size, UINT32 Fmt, UINT32 uiPathId);
		IMG_CAP_FST_INFO FstStatus = {FST_STA_OK};

		FstStatus.Status = ImageApp_Photo_WriteCB(
				(UINT32)queue_ele_in->jpg_combined_addr,
				queue_ele_in->jpg_combined_size,
				HD_CODEC_TYPE_JPEG, 0);

		if(PhotoCapMsgCb){
			PhotoCapMsgCb(IMG_CAP_CBMSG_FSTOK, &FstStatus);
		}

#if HEIC_FUNC == ENABLE
		/*******************************************************************
		 * Output heic file
		 ******************************************************************/
		if(queue_ele_in->heic_bs_data.video_pack[0].size){
			char file_path[NMC_TOTALFILEPATH_MAX_LEN] = {'\0'};
			PhotoExe_Preview_SliceEncode_Naming_HEIC(file_path);
			PhotoExe_Preview_SliceEncode_WriteFile_HEIC(&queue_ele_in->heic_bs_data, file_path);
			PhotoExe_Preview_SliceEncode_Free_Buffer(&queue_ele_in->heic_bs_buf_mem_info); /* no need to release hdal bs data, heic_bs_data is a user buffer */
		}
#endif

		/* check user bs buffer is freed */
		if(queue_ele_in->mem_info_combined.va){
			PhotoExe_Preview_SliceEncode_Free_Buffer(&queue_ele_in->mem_info_combined);
		}

		if(queue_ele_in->mem_info_thumb.va){
			PhotoExe_Preview_SliceEncode_Free_Buffer(&queue_ele_in->mem_info_thumb);
		}

		if(queue_ele_in){
			free(queue_ele_in);
			queue_ele_in = NULL;
		}

		param->cnt++;
		if(param->cnt >= param->max_cnt)
			break;

	} while(1);

EXIT:

	if(PhotoCapMsgCb){
		PhotoCapMsgCb(IMG_CAP_CBMSG_RET_PRV, NULL);
		PhotoCapMsgCb(IMG_CAP_CBMSG_CAPEND, NULL);
	}

	DBG_DUMP("task3 job finished\n");

	return ret;
}

static INT32 PhotoExe_Preview_SliceEncode_Init_Flag(ID* flag_id, char* name)
{
	INT32 ret = E_OK;

	if(vos_flag_create(flag_id, NULL, name) != E_OK){
		DBG_ERR("create flag(%s) failed!\n", name);
		ret = E_SYS;
		goto EXIT;
	}

	clr_flg(*flag_id, FLG_PHOTO_SLICE_ENC_STA_IDLE);
	clr_flg(*flag_id, FLG_PHOTO_SLICE_ENC_STA_STOPPED);
	clr_flg(*flag_id, FLG_PHOTO_SLICE_ENC_STA_STARTED);
	clr_flg(*flag_id, FLG_PHOTO_SLICE_ENC_CMD_START);
	clr_flg(*flag_id, FLG_PHOTO_SLICE_ENC_CMD_STOP);

EXIT:

	return ret;
}

static INT32 PhotoExe_Preview_SliceEncode_Uninit_Flag(ID* flag_id)
{
	INT32 ret = E_OK;

	if(vos_flag_destroy(*flag_id) != E_OK){
		DBG_ERR("destroy flag failed!\n");
		ret = E_SYS;
		goto EXIT;
	}

	*flag_id = 0;

EXIT:

	return ret;
}

static PhotoExe_Preview_SliceEncode_Task_Param task1_param = {0};
static PhotoExe_Preview_SliceEncode_Task_Param task2_param = {0};
static PhotoExe_Preview_SliceEncode_Task_Param task3_param = {0};
static PhotoExe_Preview_SliceEncode_CB1_Param cb1_param = {0};
static PhotoExe_Preview_SliceEncode_CB2_Param cb2_param = {0};
static PhotoExe_Preview_SliceEncode_CB3_Param cb3_param = {0};
static lfqueue_t queue12 = {0};
static lfqueue_t queue23 = {0};

static VOID PhotoExe_Preview_SliceEncode2_Close(VOID)
{
	FLGPTN flag = 0;

	/* stop cmd */
	if(task1_param.flag_id)
		set_flg(task1_param.flag_id, FLG_PHOTO_SLICE_ENC_CMD_STOP);

	if(task2_param.flag_id)
		set_flg(task2_param.flag_id, FLG_PHOTO_SLICE_ENC_CMD_STOP);

	if(task3_param.flag_id)
		set_flg(task3_param.flag_id, FLG_PHOTO_SLICE_ENC_CMD_STOP);

	/* check stopped */
	if(task1_param.flag_id)
		wai_flg(&flag, task1_param.flag_id, FLG_PHOTO_SLICE_ENC_STA_STOPPED, TWF_ORW);

	if(task2_param.flag_id)
		wai_flg(&flag, task2_param.flag_id, FLG_PHOTO_SLICE_ENC_STA_STOPPED, TWF_ORW);

	if(task3_param.flag_id)
		wai_flg(&flag, task3_param.flag_id, FLG_PHOTO_SLICE_ENC_STA_STOPPED, TWF_ORW);

	DBG_DUMP("slice encode finished\n");

	if(queue12.head){
		lfqueue_destroy(&queue12);
		memset(&queue12, 0, sizeof(queue12));
	}

	if(queue23.head){
		lfqueue_destroy(&queue23);
		memset(&queue23, 0, sizeof(queue23));
	}

	if(task1_param.flag_id)
		PhotoExe_Preview_SliceEncode_Uninit_Flag(&task1_param.flag_id);

	if(task2_param.flag_id)
		PhotoExe_Preview_SliceEncode_Uninit_Flag(&task2_param.flag_id);

	if(task3_param.flag_id)
		PhotoExe_Preview_SliceEncode_Uninit_Flag(&task3_param.flag_id);
}

INT32 PhotoExe_Preview_SliceEncode2(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{

	UINT8 max_cnt = 0;

	INT32 ret = E_OK;
	FLGPTN flag = 0;

	/* make sure last execution is finished */
	PhotoExe_Preview_SliceEncode2_Close();

	if(lfqueue_init(&queue12)){
		DBG_ERR("init lfqueue12 failed\n");
		goto EXIT;
	}

	if(lfqueue_init(&queue23)){
		DBG_ERR("init lfqueue23 failed\n");
		goto EXIT;
	}

	switch (UI_GetData(FL_CONTINUE_SHOT))
	{
		case CONTINUE_SHOT_BURST:
			max_cnt = 5;
			break;

		case CONTINUE_SHOT_BURST_3:
			max_cnt = 3;
			break;

		case CONTINUE_SHOT_OFF:
		default:
			max_cnt = 1;
			break;
	}

	/***************************************************************************************
	 * task1
	 ***************************************************************************************/

	memset(&task1_param, 0, sizeof(PhotoExe_Preview_SliceEncode_Task_Param));
	memset(&cb1_param, 0, sizeof(PhotoExe_Preview_SliceEncode_CB1_Param));


	if(PhotoExe_Preview_SliceEncode_Init_Flag(&(task1_param.flag_id), "slice_enc_flag1") != E_OK){
		goto EXIT;
	}

	task1_param.callback = PhotoExe_Preview_SliceEncode_CB1;
	task1_param.user_data = &cb1_param;
	task1_param.run = 0;
	task1_param.is_running = 0;
	task1_param.task_id = vos_task_create(_PhotoExe_Preview_SliceEncode_Worker, &task1_param, "slice_enc_tsk1", 9, 2048);
	if(!task1_param.task_id){
		DBG_ERR("create task1 failed!\n");
		ret = E_SYS;
		goto EXIT;
	}

	cb1_param.max_cnt = max_cnt;
	cb1_param.cnt = 0;
	cb1_param.queue12 = &queue12;
	cb1_param.period = 1000 / max_cnt; /* average ms in one second */

	task1_param.run = 1;
	vos_task_resume(task1_param.task_id);

	/***************************************************************************************
	 * task2
	 ***************************************************************************************/
	memset(&task2_param, 0, sizeof(PhotoExe_Preview_SliceEncode_Task_Param));
	memset(&cb2_param, 0, sizeof(PhotoExe_Preview_SliceEncode_CB2_Param));

	if(PhotoExe_Preview_SliceEncode_Init_Flag(&(task2_param.flag_id), "slice_enc_flag2") != E_OK){
		goto EXIT;
	}

	task2_param.callback = PhotoExe_Preview_SliceEncode_CB2;
	task2_param.user_data = &cb2_param;
	task2_param.run = 0;
	task2_param.is_running = 0;
	task2_param.task_id = vos_task_create(_PhotoExe_Preview_SliceEncode_Worker, &task2_param, "slice_enc_tsk2", 12, 8192);
	if(!task2_param.task_id){
		DBG_ERR("create task2 failed!\n");
		ret = E_SYS;
		goto EXIT;
	}

	cb2_param.max_cnt = max_cnt;
	cb2_param.cnt = 0;
	cb2_param.queue12 = &queue12;
	cb2_param.queue23 = &queue23;

	task2_param.run = 1;
	vos_task_resume(task2_param.task_id);


	/***************************************************************************************
	 * task3
	 ***************************************************************************************/
	memset(&task3_param, 0, sizeof(PhotoExe_Preview_SliceEncode_Task_Param));
	memset(&cb3_param, 0, sizeof(PhotoExe_Preview_SliceEncode_CB3_Param));

	if(PhotoExe_Preview_SliceEncode_Init_Flag(&(task3_param.flag_id), "slice_enc_flag3") != E_OK){
		goto EXIT;
	}

	task3_param.callback = PhotoExe_Preview_SliceEncode_CB3;
	task3_param.user_data = &cb3_param;
	task3_param.run = 0;
	task3_param.is_running = 0;
	task3_param.task_id = vos_task_create(_PhotoExe_Preview_SliceEncode_Worker, &task3_param, "slice_enc_tsk3", 12, 4096);
	if(!task3_param.task_id){
		DBG_ERR("create task3 failed!\n");
		ret = E_SYS;
		goto EXIT;
	}

	cb3_param.max_cnt = max_cnt;
	cb3_param.cnt = 0;
	cb3_param.queue23 = &queue23;

	task3_param.run = 1;
	vos_task_resume(task3_param.task_id);

	/* start cmd */
	set_flg(task1_param.flag_id, FLG_PHOTO_SLICE_ENC_CMD_START);
	set_flg(task2_param.flag_id, FLG_PHOTO_SLICE_ENC_CMD_START);
	set_flg(task3_param.flag_id, FLG_PHOTO_SLICE_ENC_CMD_START);

	/* check started */
	wai_flg(&flag, task1_param.flag_id, FLG_PHOTO_SLICE_ENC_STA_STARTED, TWF_ORW);
	wai_flg(&flag, task2_param.flag_id, FLG_PHOTO_SLICE_ENC_STA_STARTED, TWF_ORW);
	wai_flg(&flag, task3_param.flag_id, FLG_PHOTO_SLICE_ENC_STA_STARTED, TWF_ORW);

EXIT:

	if(ret != E_OK)
		DBG_ERR("cb2 filaed!\n");

	return NVTEVT_CONSUME;

}

#endif

#endif


EVENT_ENTRY CustomPhotoObjCmdMap[] = {
	{NVTEVT_EXE_OPEN,               PhotoExe_OnOpen                 },
	{NVTEVT_EXE_CLOSE,              PhotoExe_OnClose                },
	{NVTEVT_EXE_SLEEP,              PhotoExe_OnSleep                },
	{NVTEVT_EXE_WAKEUP,             PhotoExe_OnWakeup               },
	{NVTEVT_EXE_MACRO,              PhotoExe_OnMacro                },
	{NVTEVT_EXE_SELFTIMER,          PhotoExe_OnSelftimer            },
	{NVTEVT_EXE_EV,                 PhotoExe_OnEV                   },
	{NVTEVT_EXE_CAPTURE_SIZE,       PhotoExe_OnCaptureSize          },
	{NVTEVT_EXE_QUALITY,            PhotoExe_OnQuality              },
	{NVTEVT_EXE_WB,                 PhotoExe_OnWB                   },
	{NVTEVT_EXE_COLOR,              PhotoExe_OnColor                },
	{NVTEVT_EXE_ISO,                PhotoExe_OnISO                  },
	{NVTEVT_EXE_AFWINDOW,           PhotoExe_OnAFWindow             },
	{NVTEVT_EXE_AFBEAM,             PhotoExe_OnAFBeam               },
	{NVTEVT_EXE_CONTAF,             PhotoExe_OnContAF               },
	{NVTEVT_EXE_METERING,           PhotoExe_OnMetering             },
	{NVTEVT_EXE_CAPTURE_MODE,       PhotoExe_OnCaptureMode          },
	{NVTEVT_EXE_DATE_PRINT,         PhotoExe_OnDatePrint            },
	{NVTEVT_EXE_PREVIEW,            PhotoExe_OnPreview              },
	{NVTEVT_EXE_DIGITAL_ZOOM,       PhotoExe_OnDigitalZoom          },
	{NVTEVT_EXE_FD,                 PhotoExe_OnFD                   },
	{NVTEVT_EXE_CONTSHOT,           PhotoExe_OnContShot             },
	{NVTEVT_EXE_CAPTURE_START,      PhotoExe_OnCaptureStart         },
	{NVTEVT_EXE_CAPTURE_STOP,       PhotoExe_OnCaptureStop          },
	{NVTEVT_EXE_CAPTURE_END,        PhotoExe_OnCaptureEnd           },
	{NVTEVT_EXE_ZOOM,               PhotoExe_OnZoom                 },
	{NVTEVT_EXE_RSC,                PhotoExe_OnRSC                  },
	{NVTEVT_EXE_START_FUNC,         PhotoExe_OnStartFunc            },
	{NVTEVT_EXE_STOP_FUNC,          PhotoExe_OnStopFunc             },
	{NVTEVT_EXE_AF_PROCESS,         PhotoExe_OnAFProcess            },
	{NVTEVT_EXE_AF_RELEASE,         PhotoExe_OnAFRelease            },
	{NVTEVT_EXE_AF_WAITEND,         PhotoExe_OnAFWaitEnd            },
	{NVTEVT_EXE_IMAGE_RATIO,        PhotoExe_OnImageRatio           },
	{NVTEVT_EXE_INIT_DATE_BUF,      PhotoExe_OnInitDateBuf          },
	{NVTEVT_EXE_GEN_DATE_STR,       PhotoExe_OnGenDateStr           },
	{NVTEVT_EXE_GEN_DATE_PIC,       PhotoExe_OnGenDatePic           },
	{NVTEVT_EXE_SHARPNESS,          PhotoExe_OnSharpness            },
	{NVTEVT_EXE_SATURATION,         PhotoExe_OnSaturation           },
	{NVTEVT_EXE_PLAY_SHUTTER_SOUND, PhotoExe_OnPlayShutterSound     },
	{NVTEVT_VIDEO_CHANGE,           PhotoExe_OnVideoChange          },
	{NVTEVT_EXE_DUALCAM,            PhotoExe_OnDualcam              },
	{NVTEVT_EXE_FDEND,              PhotoExe_OnFDEnd                },
	//{NVTEVT_EXE_SHDR,               PhotoExe_OnSHDR                 },// mark for FlowPhoto_CheckReOpenItem()
	{NVTEVT_EXE_WDR,                PhotoExe_OnWDR                  },
	{NVTEVT_EXE_NR,                 PhotoExe_OnNR                   },
	{NVTEVT_CALLBACK,               PhotoExe_OnCallback             },
	{NVTEVT_ALGMSG_FOCUSEND,        PhotoExe_OnFocusEnd             },
	{NVTEVT_EXE_DEFOG,              PhotoExe_OnDefog},
	{NVTEVT_EXE_SENSORHOTPLUG,      PhotoExe_OnSensorHotPlug       },

#if PHOTO_PREVIEW_SLICE_ENC_FUNC == ENABLE
	#if PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC == ENABLE
		{NVTEVT_EXE_SLICE_ENCODE, 		PhotoExe_Preview_SliceEncode2},
	#else
		{NVTEVT_EXE_SLICE_ENCODE, 		PhotoExe_Preview_SliceEncode},
	#endif
#endif

	{NVTEVT_NULL,                   0},
};

CREATE_APP(CustomPhotoObj, APP_SETUP)
#endif // PHOTO_MODE==ENABLE
