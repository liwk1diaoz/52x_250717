#include <stdio.h>
#include "ImageApp/ImageApp_MovieMulti.h"
#include "hdal.h"
#include "PrjInc.h"
#include "UIApp/MovieStamp/MovieStamp.h"
#include "SysSensor.h"
#include "vendor_eis.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UiAppMovie_CommPool
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#if (MOVIE_EIS == ENABLE)
#include "vendor_eis.h"
#endif

//#define ENABLE      1
//#define DISABLE     0

#if defined(_MODEL_580_CARDV_ETHCAM_RX_EVB_) || defined(_MODEL_CARDV_D160_) || defined(_sen_imx415_) || defined(_sen_imx317_) || defined(_sen_imx678_)
#define VDO_SIZE_W                  3840 //raw buffer for D2D mode
#define VDO_SIZE_H                  2160 //raw buffer for D2D mode

#define VDO_MAIN_SIZE_W             3840 //YUV
#define VDO_MAIN_SIZE_H             2160 //YUV

#if defined(_MODEL_580_CARDV_ETHCAM_RX_EVB_)
#define VDO_CLONE_SIZE_W            848
#define VDO_CLONE_SIZE_H            480
#else
#define VDO_CLONE_SIZE_W            1920
#define VDO_CLONE_SIZE_H            1080
#endif

#define VDO2_SIZE_W                  1920 //second sensor. raw buffer for D2D mode
#define VDO2_SIZE_H                  1080 //second sensor. raw buffer for D2D mode

#define VDO2_MAIN_SIZE_W             1920
#define VDO2_MAIN_SIZE_H             1080

#define VDO2_CLONE_SIZE_W            848
#else
    #define VDO_SIZE_W                  3840
    #define VDO_SIZE_H                  2160

    #define VDO_MAIN_SIZE_W             3840
    #define VDO_MAIN_SIZE_H             2160

#define VDO_CLONE_SIZE_W            848
#define VDO_CLONE_SIZE_H            480

    #define VDO2_SIZE_W                  1920 //second sensor. raw buffer for D2D mode
    #define VDO2_SIZE_H                  1080 //second sensor. raw buffer for D2D mode

    #define VDO2_MAIN_SIZE_W             1920
    #define VDO2_MAIN_SIZE_H             1080

    #define VDO2_CLONE_SIZE_W            848
#endif


#define VDO_DISP_SIZE_W             960
#define VDO_DISP_SIZE_H             180

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

static HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

void Movie_CommPoolInit(void)
{
	UINT32 id=0;
	UINT32 vcap_buf_size = 0;
	HD_VIDEO_PXLFMT vcap_fmt = HD_VIDEO_PXLFMT_RAW12;
#if (!defined(_NVT_ETHREARCAM_TX_))
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
    HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
    HD_RESULT hd_ret = HD_OK;
    USIZE DspDevSize  = {0};

	hd_ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (hd_ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
        DspDevSize.w = 960;
        DspDevSize.h = 240;
	} else {
#if (MOVIE_EIS == DISABLE)
        DspDevSize.w = p_video_out_syscaps->output_dim.w;
        DspDevSize.h = p_video_out_syscaps->output_dim.h;
#else
        if (SysGetFlag(FL_MOVIE_EIS) == EIS_ON) {
#if defined(_MODEL_580_CARDV_EVB_)
            DspDevSize.w = 1472;
            DspDevSize.h = 276;
#elif (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_))
            DspDevSize.w = 720;
            DspDevSize.h = 480;
#elif (defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
            DspDevSize.w = 240;
            DspDevSize.h = 240;
#endif
         } else {
        DspDevSize.w = p_video_out_syscaps->output_dim.w;
        DspDevSize.h = p_video_out_syscaps->output_dim.h;
	}
#endif
	}

	// config common pool (cap)
	for (id = 0; id < (SENSOR_CAPS_COUNT& SENSOR_ON_MASK); id++) {
		System_GetSensorInfo(id, SENSOR_CAPOUT_FMT, &vcap_fmt);
#if 1//(MOVIE_DIRECT_FUNC != ENABLE)
		if (HD_VIDEO_PXLFMT_CLASS(vcap_fmt) == HD_VIDEO_PXLFMT_CLASS_YUV) {     // YUV
		    switch (id) {
	            default:
	            case 0: //single sensor
	                vcap_buf_size = VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, vcap_fmt);
	                break;
	            case 1: // dual sensor
	                vcap_buf_size = VDO_YUV_BUFSIZE(VDO2_SIZE_W, VDO2_SIZE_H, vcap_fmt);
	                break;
	            case 2:
	                vcap_buf_size = VDO_YUV_BUFSIZE(VDO2_SIZE_W, VDO2_SIZE_H, vcap_fmt);
	                break;
	            case 3:
	                vcap_buf_size = VDO_YUV_BUFSIZE(VDO2_SIZE_W, VDO2_SIZE_H, vcap_fmt);
	                break;
		    }
		} else {                                                                // RAW
		    switch (id) {
	            default:
	            case 0:
	                vcap_buf_size = VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, vcap_fmt);
	                break;
	            case 1:
	                vcap_buf_size = VDO_RAW_BUFSIZE(VDO2_SIZE_W, VDO2_SIZE_H, vcap_fmt);
	                break;
	            case 2:
	                vcap_buf_size = VDO_RAW_BUFSIZE(VDO2_SIZE_W, VDO2_SIZE_H, vcap_fmt);
	                break;
	            case 3:
	                vcap_buf_size = VDO_RAW_BUFSIZE(VDO2_SIZE_W, VDO2_SIZE_H, vcap_fmt);
	                break;
		    }
		}
#endif
		#if (MOVIE_DIRECT_FUNC == ENABLE)
		if (id == 0) {
			vcap_buf_size = 0;
		}
		#endif
		mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() +
										vcap_buf_size +
										VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H) +
										VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		// if enable EIS
#if (MOVIE_EIS == ENABLE)
        if (SysGetFlag(FL_MOVIE_EIS) == EIS_ON) {
		    mem_cfg.pool_info[id].blk_size += GYRO_DATA_SIZE;
        }
#endif
		mem_cfg.pool_info[id].blk_cnt = 3;
		mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	}
#endif

#if(!defined(_NVT_ETHREARCAM_RX_) && !defined(_NVT_ETHREARCAM_TX_))
	// config common pool (main)
	//id ++;
	#if (SENSOR_CAPS_COUNT >= 2)
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO_MAIN_SIZE_W, VDO_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (main2)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO2_MAIN_SIZE_W, VDO2_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	#if (SENSOR_CAPS_COUNT >= 3)
	// config common pool (main3)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO2_MAIN_SIZE_W, VDO2_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	#endif

	#if (SENSOR_CAPS_COUNT >= 4)
	// config common pool (main4)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO2_MAIN_SIZE_W, VDO2_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	#endif

	// config common pool (clone)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_CLONE_SIZE_W, VDO_CLONE_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (disp) // pipview + rotated lcd
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(DspDevSize.w, DspDevSize.h, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 10;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
    #else
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO_MAIN_SIZE_W, VDO_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
#if (MOVIE_EIS == ENABLE)
	if (SysGetFlag(FL_MOVIE_EIS) == EIS_ON) {
		mem_cfg.pool_info[id].blk_size += (vendor_eis_buf_query(VENDOR_EIS_LUT_65x65) + GYRO_DATA_SIZE + sizeof(VENDOR_EIS_DBG_CTX));
	}
#endif
#if (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_))
	mem_cfg.pool_info[id].blk_cnt = 16;
#elif (defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
	mem_cfg.pool_info[id].blk_cnt = 16;
#else
	if(System_IsBootFromRtos()){
		mem_cfg.pool_info[id].blk_cnt = 10; //3 for 3dnr-off, 4 for 3dnr-on
	}else{
		mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	}
#endif
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (clone)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_CLONE_SIZE_W, VDO_CLONE_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (disp)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(DspDevSize.w, DspDevSize.h, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
    #endif
#elif(defined(_NVT_ETHREARCAM_RX_)) /// for NVT_ETHREARCAM_RX
	#if(SENSOR_CAPS_COUNT & SENSOR_ON_MASK)
	// config common pool (main)
	//id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO_MAIN_SIZE_W, VDO_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (clone)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_CLONE_SIZE_W, VDO_CLONE_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 5;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (disp)
	id ++;
	#endif

	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_16(DspDevSize.w), ALIGN_CEIL_16(DspDevSize.h), HD_VIDEO_PXLFMT_YUV420);
	#if (ETH_REARCAM_CAPS_COUNT >= 2)
	mem_cfg.pool_info[id].blk_cnt = 10;
	#else
	mem_cfg.pool_info[id].blk_cnt = 5;
	#endif
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (decode)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	#if (ETH_REARCAM_CAPS_COUNT >= 2)
		mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(1920), ALIGN_CEIL_64(1080), HD_VIDEO_PXLFMT_YUV420);
		#if (ETHCAM_EIS ==ENABLE)
		mem_cfg.pool_info[id].blk_cnt = 15;
		#else
		mem_cfg.pool_info[id].blk_cnt = 10;
		#endif
	#else
	#if defined(_MODEL_580_CARDV_ETHCAM_RX_EVB_)
	#if (ETHCAM_EIS ==ENABLE)
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(1920), ALIGN_CEIL_64(1080), HD_VIDEO_PXLFMT_YUV420);
	#else
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(848), ALIGN_CEIL_64(480), HD_VIDEO_PXLFMT_YUV420);
	#endif
	#else
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(1280), ALIGN_CEIL_64(720), HD_VIDEO_PXLFMT_YUV420);
	#endif
	#if (ETHCAM_EIS ==ENABLE)
	mem_cfg.pool_info[id].blk_cnt = 15;
	#else
	mem_cfg.pool_info[id].blk_cnt = 5;
	#endif
	#endif
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
#elif(defined(_NVT_ETHREARCAM_TX_)) /// for NVT_ETHREARCAM_TX
	System_GetSensorInfo(id, SENSOR_CAPOUT_FMT, &vcap_fmt);
	vcap_buf_size = VDO_RAW_BUFSIZE(1920, 1080, vcap_fmt);
	#if (MOVIE_DIRECT_FUNC == ENABLE)
	vcap_buf_size = 0;
	#endif // (MOVIE_DIRECT_FUNC == ENABLE)
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() +
									vcap_buf_size +
									VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H) +
									VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[id].blk_cnt = 3;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;


	// config common pool (main)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	//mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(2560, 1440, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_NVX_BUFSIZE(3840, 2160, HD_VIDEO_PXLFMT_YUV420);//yuv compress
	//mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_NVX_BUFSIZE(1920, 1080, HD_VIDEO_PXLFMT_YUV420);//yuv compress
	mem_cfg.pool_info[id].blk_cnt = 4; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;


	// config common pool (disp)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_DISP_SIZE_W, VDO_DISP_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;


	// config common pool (clone)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(1280, 720, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 4; //no alg
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

#endif

	ImageApp_MovieMulti_Config(MOVIE_CONFIG_MEM_POOL_INFO, (UINT32)&mem_cfg);
}

