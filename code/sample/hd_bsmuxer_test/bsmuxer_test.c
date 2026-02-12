/**
	@brief Sample code of bitstream muxer.\n

	@file bsmuxer_test.c

	@author Willy Su

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2022.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_type.h"
#include "vendor_videoenc.h"
#include "vendor_audioenc.h"

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_bsmuxer_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

// user space library
#include "hd_bsmux_lib.h"
#include "hd_fileout_lib.h"
#include "FileSysTsk.h"
#include "sdio.h"
#include "comm/hwclock.h"

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

#define SEN_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_RAW12_SHDR2
#define CA_WIN_NUM_W    32
#define CA_WIN_NUM_H    32
#define LA_WIN_NUM_W    32
#define LA_WIN_NUM_H    32
#define VA_WIN_NUM_W    16
#define VA_WIN_NUM_H    16
#define YOUT_WIN_NUM_W  128
#define YOUT_WIN_NUM_H  128
#define ETH_8BIT_SEL    0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL     1 //0: full, 1: subsample 1/2

#define RESOLUTION_SET  0 //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR)
#if ( RESOLUTION_SET == 0)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif (RESOLUTION_SET == 1)
#define VDO_SIZE_W      2592
#define VDO_SIZE_H      1944
#elif ( RESOLUTION_SET == 2)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif ( RESOLUTION_SET == 3)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#endif

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_SHDR | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_DEFOG

#define VDO_FRC         30
#define VDO_GOP         15

#define ADO_SR          HD_AUDIO_SR_32000

#define FS_DRIVE        'A'
#define FS_POP_SIZE     4*1024*1024UL
#define HWCLK_TIME      hwclock_get_time(TIME_ID_CURRENT)
#define INS_TSE_KO      "modprobe nvt_tse"
#define TS_VID_SIZE     21
#define TS_AUD_SIZE     21
#define BSMUXER_MAX     4
#define FILEOUT_MAX     4

// video record module
static UINT32 g_vcap_module = 1; //0:disable, 1:enable
static UINT32 g_vproc_module = 1; //0:disable, 1:enable
static UINT32 g_venc_module = 1; //0:disable, 1:enable

// audio record module
static UINT32 g_acap_module = 1; //0:disable, 1:enable
static UINT32 g_aenc_module = 1; //0:disable, 1:enable

// user space library
static UINT32 g_filesys_module = 1; //0:disable, 1:enable
static UINT32 g_fileout_module = 1; //0:disable, 1:enable
static UINT32 g_bsmuxer_module = 1; //0:disable, 1:enable
static UINT32 g_hwclock_module = 0; //0:disable, 1:enable
static UINT32 g_tse_module = 0; //0:disable, 1:enable

// meta data
static UINT32 g_meta_data = 1; //meta data count
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	if (g_vcap_module) {
		//direct ... NOT require raw
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()
				+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
				+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		mem_cfg.pool_info[0].blk_cnt = 2;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	}

	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	if (g_vproc_module) {
		//low-latency
		mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()
				+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[1].blk_cnt = 3;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	}

	ret = hd_common_mem_init(&mem_cfg);

	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _BSMUXER_STREAM {

	// (1) filesys
	char drive;
	unsigned long fs_pa;
	unsigned long fs_va;
	unsigned long fs_size;

	// (2) fileout
	HD_PATH_ID fileout_path[FILEOUT_MAX];
	HD_FILEOUT_CONFIG fileout_config;
	UINT32 fileout_path_count;

	// (3) bsmuxer
	HD_PATH_ID bsmuxer_path[BSMUXER_MAX];
	HD_BSMUX_FTYPE bsmuxer_type;
	HD_BSMUX_EN_UTIL bsmuxer_util;
	UINT32 bsmuxer_path_count;

	// (4) vcap
	HD_VIDEOCAP_SYSCAPS vcap_syscaps;
	HD_PATH_ID vcap_ctrl;
	HD_PATH_ID vcap_path;
	HD_DIM vcap_dim;

	// (5) vproc
	HD_VIDEOPROC_SYSCAPS vproc_syscaps;
	HD_PATH_ID vproc_ctrl;
	HD_PATH_ID vproc_path;
	HD_DIM vproc_max_dim;

	// (6) venc
	HD_VIDEOENC_SYSCAPS venc_syscaps;
	HD_PATH_ID venc_path;
	HD_DIM venc_max_dim;
	HD_DIM venc_dim;
	HD_VIDEO_CODEC venc_codec;
	HD_VIDEOENC_BUFINFO venc_bufinfo;

	// (7) acap
	HD_PATH_ID acap_ctrl;
	HD_PATH_ID acap_path;

	// (8) aenc
	HD_PATH_ID aenc_path;
	HD_AUDIO_CODEC aenc_codec;
	HD_AUDIOENC_BUFINFO aenc_bufinfo;

	// (9) user pull
	pthread_t  vrec_thread_id;
	UINT32     vrec_enter;
	UINT32     vrec_exit;
	pthread_t  arec_thread_id;
	UINT32     arec_enter;
	UINT32     arec_exit;

	// (10) meta data
	unsigned long meta_pa;
	unsigned long meta_va;
	unsigned long meta_size;
	unsigned long meta_pa_idx[10];
	unsigned long meta_va_idx[10];
	unsigned long meta_size_idx[10];

} BSMUXER_STREAM;

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                             Meta Data Area                              = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////
static HD_RESULT init_meta_data(BSMUXER_STREAM *p_stream)
{
	HD_RESULT hd_ret;
	UINT32 i, num = g_meta_data;
	UINT32 pa, va, size;

	size = 0x1000; //user config

	hd_ret = hd_common_mem_alloc("Meta", (UINT32 *)&pa, (void **)&va, (UINT32)(size * num), DDR_ID0);
	if (HD_OK != hd_ret) {
		printf("%s mem alloc fail\r\n", __func__);
		return hd_ret;
	}
	p_stream->meta_pa = pa;
	p_stream->meta_va = va;
	p_stream->meta_size = size * num;

	for (i = 0; i < num; i++) {
		p_stream->meta_pa_idx[i] = pa;
		p_stream->meta_va_idx[i] = va;
		p_stream->meta_size_idx[i] = size;
		pa += size;
		va += size;
	}

	return HD_OK;
}

static HD_RESULT exit_meta_data(BSMUXER_STREAM *p_stream)
{
	HD_RESULT hd_ret;

	if (p_stream->meta_pa && p_stream->meta_va) {
		hd_ret = hd_common_mem_free((UINT32)p_stream->meta_pa, (void *)p_stream->meta_va);
		if (HD_OK != hd_ret)
			printf("%s mem free fail\r\n", __func__);
		p_stream->meta_pa = 0;
		p_stream->meta_va = 0;
		p_stream->meta_size = 0;
	}

	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                             FileSys Flow                                = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////
static HD_RESULT init_filesys_module(BSMUXER_STREAM *p_stream)
{
	HD_RESULT hd_ret;
	FS_HANDLE strg_hdl;
	FILE_TSK_INIT_PARAM param = {0};

	p_stream->drive = FS_DRIVE; // user config
	p_stream->fs_size = (ALIGN_CEIL_64(0x4000));  //for linux fs cmd
	hd_ret = hd_common_mem_alloc("FsLib", (UINT32 *)&p_stream->fs_pa,
		(void **)&p_stream->fs_va, (UINT32)p_stream->fs_size, DDR_ID0);
	if (HD_OK != hd_ret) {
		printf("%s mem alloc fail\r\n", __func__);
		return hd_ret;
	}

	strg_hdl = (FS_HANDLE)sdio_getStorageObject(STRG_OBJ_FAT1);
	if (strg_hdl == NULL) {
		printf("%s get storage fail\r\n", __func__);
		return HD_ERR_NG;
	}
	FileSys_InstallID(FileSys_GetOPS_Linux());
	if (FST_STA_OK != FileSys_Init(FileSys_GetOPS_Linux())) {
		printf("%s init fail\r\n", __func__);
		return HD_ERR_NG;
	}

	memset((void *)&param, 0, sizeof(FILE_TSK_INIT_PARAM));
	param.FSParam.WorkBuf = p_stream->fs_va;
	param.FSParam.WorkBufSize = (UINT32)p_stream->fs_size;
	strncpy(param.FSParam.szMountPath, "/mnt/sd", sizeof(param.FSParam.szMountPath) - 1); //only used by FsLinux
	param.FSParam.szMountPath[sizeof(param.FSParam.szMountPath) - 1] = '\0';
	param.FSParam.MaxOpenedFileNum = 10; // user config
	if (FST_STA_OK != FileSys_OpenEx(p_stream->drive, strg_hdl, &param)) {
		printf("%s open fail\r\n", __func__);
		return HD_ERR_NG;
	}

	// call the function to wait init finish
	FileSys_WaitFinishEx(p_stream->drive);
	return HD_OK;
}

static HD_RESULT exit_filesys_module(BSMUXER_STREAM *p_stream)
{
	HD_RESULT hd_ret;

	if (FST_STA_OK != FileSys_CloseEx(p_stream->drive, 1000)) {
		printf("%s close fail\r\n", __func__);
	}

	if (p_stream->fs_pa && p_stream->fs_va) {
		hd_ret = hd_common_mem_free((UINT32)p_stream->fs_pa, (void *)p_stream->fs_va);
		if (HD_OK != hd_ret)
			printf("%s mem free fail\r\n", __func__);
		p_stream->fs_pa = 0;
		p_stream->fs_va = 0;
	}

	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                             FileOut Flow                                = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////
INT32 fileout_callback_func(CHAR *p_name, HD_FILEOUT_CBINFO *cbinfo, UINT32 *param)
{
	HD_FILEOUT_CB_EVENT event = cbinfo->cb_event;
	switch (event) {
	case HD_FILEOUT_CB_EVENT_NAMING:
		{
			char c_mp4[4] = "MP4\0";
			char c_mov[4] = "MOV\0";
			char c_ts[3] = "TS\0";
			time_t time_sec = time(0);
			struct tm cur_time;
			localtime_r(&time_sec, &cur_time);
			cbinfo->fpath_size = 128;
			snprintf(cbinfo->p_fpath, cbinfo->fpath_size, "A:\\%04d%02d%02d-%02d%02d%02d-%02d.",
				cur_time.tm_year+1900, cur_time.tm_mon+1, cur_time.tm_mday,
				cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec, (int)cbinfo->iport);
			if (cbinfo->type == HD_FILEOUT_FTYPE_MOV)
				strncat(cbinfo->p_fpath, c_mov, 3);
			else if (cbinfo->type == HD_FILEOUT_FTYPE_TS)
				strncat(cbinfo->p_fpath, c_ts, 3);
			else
				strncat(cbinfo->p_fpath, c_mp4, 3);
		}
		break;
	case HD_FILEOUT_CB_EVENT_OPENED:
		break;
	case HD_FILEOUT_CB_EVENT_CLOSED:
		break;
	case HD_FILEOUT_CB_EVENT_DELETE:
		break;
	case HD_FILEOUT_CB_EVENT_FS_ERR:
		{
			printf("fileout errcode 0x%x\r\n", (int)cbinfo->errcode);
		}
		break;
	default:
		break;
	}
	return 0;
}

static HD_RESULT set_fileout_config(BSMUXER_STREAM *p_stream)
{
	HD_FILEOUT_CONFIG fileout_config = {0};
	HD_PATH_ID fileout_ctrl;
	HD_RESULT hd_ret;

	if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
		printf("%s open ctrl fail\r\n", __func__);
		return hd_ret;
	}

	// (0) CALLBACK
	if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_REG_CALLBACK, (VOID *)fileout_callback_func)) != HD_OK) {
		printf("%s reg callback fail\r\n", __func__);
		return hd_ret;
	}

	if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
		printf("%s close ctrl fail\r\n", __func__);
	}

	// (1) CONFIG
	fileout_config.drive = p_stream->drive;
	if (p_stream->fileout_config.max_pop_size)
		fileout_config.max_pop_size = p_stream->fileout_config.max_pop_size;
	else
		fileout_config.max_pop_size = FS_POP_SIZE;
	fileout_config.format_free = ((p_stream->fileout_config.format_free) ? 1 : 0);
	fileout_config.use_mem_blk = ((p_stream->fileout_config.use_mem_blk) ? 1 : 0);
	fileout_config.wait_ready = ((p_stream->fileout_config.wait_ready) ? 1 : 0);
	//fileout_config.close_on_exec = 1;
	if ((hd_ret = hd_fileout_set(p_stream->fileout_path[0], HD_FILEOUT_PARAM_CONFIG, (VOID *)&fileout_config)) != HD_OK) {
		printf("%s set config0 fail\r\n", __func__);
	}
	if (p_stream->fileout_path_count > 1) {
		if ((hd_ret = hd_fileout_set(p_stream->fileout_path[1], HD_FILEOUT_PARAM_CONFIG, (VOID *)&fileout_config)) != HD_OK) {
			printf("%s set config1 fail\r\n", __func__);
		}
	}
	if (p_stream->fileout_path_count > 2) {
		if ((hd_ret = hd_fileout_set(p_stream->fileout_path[2], HD_FILEOUT_PARAM_CONFIG, (VOID *)&fileout_config)) != HD_OK) {
			printf("%s set config2 fail\r\n", __func__);
		}
	}
	if (p_stream->fileout_path_count > 3) {
		if ((hd_ret = hd_fileout_set(p_stream->fileout_path[3], HD_FILEOUT_PARAM_CONFIG, (VOID *)&fileout_config)) != HD_OK) {
			printf("%s set config3 fail\r\n", __func__);
		}
	}

	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                             BsMuxer Flow                                = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////
INT32 bsmux_callback_func(CHAR *p_name, HD_BSMUX_CBINFO *cbinfo, UINT32 *param)
{
	HD_BSMUX_CB_EVENT event = cbinfo->cb_event;
	switch (event) {
	case HD_BSMUX_CB_EVENT_PUTBSDONE:
		break;
	case HD_BSMUX_CB_EVENT_FOUTREADY: //ready ops buf
		{
			HD_PATH_ID fileout_path = cbinfo->id;
			HD_FILEOUT_BUF *fout_buf = (HD_FILEOUT_BUF *)cbinfo->out_data;
#if 1 // newfeature
			if (cbinfo->out_data->p_user_data)
				printf("p_user_data=0x%lx\r\n", (unsigned long)cbinfo->out_data->p_user_data);
#endif
			if (hd_fileout_push_in_buf(fileout_path, fout_buf, -1) != HD_OK)
				printf("hd_fileout_push_in_buf fail\r\n");
		}
		break;
	case HD_BSMUX_CB_EVENT_CUT_COMPLETE:
		break;
	case HD_BSMUX_CB_EVENT_CLOSE_RESULT:
		{
			printf("muxer errcode 0x%x\r\n", (int)cbinfo->errcode);
		}
		break;
	case HD_BSMUX_CB_EVENT_COPYBSBUF:
		break;
	default:
		break;
	}
	return 0;
}

static HD_RESULT set_bsmuxer_config(BSMUXER_STREAM *p_stream)
{
	HD_BSMUX_VIDEOINFO  video_info = {0};
	HD_BSMUX_AUDIOINFO  audio_info = {0};
	HD_BSMUX_FILEINFO   file_info = {0};
	HD_BSMUX_BUFINFO    buf_info = {0};
	HD_PATH_ID bsmuxer_ctrl;
	HD_RESULT hd_ret;

	if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmuxer_ctrl)) != HD_OK) {
		printf("%s open ctrl fail\r\n", __func__);
		return hd_ret;
	}

	// (0) CALLBACK
	if ((hd_ret = hd_bsmux_set(bsmuxer_ctrl, HD_BSMUX_PARAM_REG_CALLBACK, (VOID *)bsmux_callback_func)) != HD_OK) {
		printf("%s reg callback fail\r\n", __func__);
		return hd_ret;
	}

	if ((hd_ret = hd_bsmux_close(bsmuxer_ctrl)) != HD_OK) {
		printf("%s close ctrl fail\r\n", __func__);
	}

	// (1) VIDEOINFO
	if (p_stream->venc_codec == HD_CODEC_TYPE_H264)
		video_info.vidcodec  = HD_BSMUX_VIDCODEC_H264;
	else
		video_info.vidcodec  = HD_BSMUX_VIDCODEC_H265;
	video_info.vfr           = VDO_FRC;
	video_info.width         = VDO_SIZE_W;
	video_info.height        = VDO_SIZE_H;
	video_info.tbr           = 2*1024*1024; // 2 Mb/s
	video_info.gop           = VDO_GOP;
	if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[0], HD_BSMUX_PARAM_VIDEOINFO, (VOID *)&video_info)) != HD_OK) {
		printf("%s set vidconfig0 fail\r\n", __func__);
	}
	if (p_stream->bsmuxer_path_count > 1) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[1], HD_BSMUX_PARAM_VIDEOINFO, (VOID *)&video_info)) != HD_OK) {
			printf("%s set vidconfig1 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 2) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[2], HD_BSMUX_PARAM_VIDEOINFO, (VOID *)&video_info)) != HD_OK) {
			printf("%s set vidconfig2 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 3) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[3], HD_BSMUX_PARAM_VIDEOINFO, (VOID *)&video_info)) != HD_OK) {
			printf("%s set vidconfig3 fail\r\n", __func__);
		}
	}
	// (2) AUDIOINFO
	if (p_stream->aenc_codec == HD_AUDIO_CODEC_PCM)
		audio_info.codectype = HD_BSMUX_AUDCODEC_PPCM; ///< Packed PCM
	else
		audio_info.codectype = HD_BSMUX_AUDCODEC_AAC;
	audio_info.chs           = 2;
	audio_info.asr           = ADO_SR;
	audio_info.adts_bytes    = ((p_stream->aenc_codec == HD_AUDIO_CODEC_PCM) ? 0 : 7);
	audio_info.aud_en        = ((g_aenc_module) ? 1 : 0); //enable or disable
	if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[0], HD_BSMUX_PARAM_AUDIOINFO, (VOID *)&audio_info)) != HD_OK) {
		printf("%s set audconfig0 fail\r\n", __func__);
	}
	if (p_stream->bsmuxer_path_count > 1) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[1], HD_BSMUX_PARAM_AUDIOINFO, (VOID *)&audio_info)) != HD_OK) {
			printf("%s set audconfig1 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 2) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[2], HD_BSMUX_PARAM_AUDIOINFO, (VOID *)&audio_info)) != HD_OK) {
			printf("%s set audconfig2 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 3) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[3], HD_BSMUX_PARAM_AUDIOINFO, (VOID *)&audio_info)) != HD_OK) {
			printf("%s set audconfig3 fail\r\n", __func__);
		}
	}
	// (3) FILEINFO
	file_info.seamlessSec    = 60;
	file_info.rollbacksec    = 3;
	file_info.keepsec        = 5;
	if (p_stream->bsmuxer_type == HD_BSMUX_FTYPE_TS)
		file_info.filetype   = HD_BSMUX_FTYPE_TS;
	else if (p_stream->aenc_codec == HD_AUDIO_CODEC_PCM)
		file_info.filetype   = HD_BSMUX_FTYPE_MOV;
	else
		file_info.filetype   = HD_BSMUX_FTYPE_MP4;
	file_info.recformat      = ((g_aenc_module) ? HD_BSMUX_RECFORMAT_AUD_VID_BOTH : HD_BSMUX_RECFORMAT_VID_ONLY);
	file_info.revsec         = 5;
	file_info.seamlessSec_ms = HD_BSMUX_SET_MS(60, 0);
	file_info.rollbacksec_ms = HD_BSMUX_SET_MS(3, 0);
	file_info.keepsec_ms     = HD_BSMUX_SET_MS(5, 0);
	file_info.revsec_ms      = HD_BSMUX_SET_MS(5, 0);
	if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[0], HD_BSMUX_PARAM_FILEINFO, (VOID *)&file_info)) != HD_OK) {
		printf("%s set fileconfig0 fail\r\n", __func__);
	}
	if (p_stream->bsmuxer_path_count > 1) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[1], HD_BSMUX_PARAM_FILEINFO, (VOID *)&file_info)) != HD_OK) {
			printf("%s set fileconfig1 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 2) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[2], HD_BSMUX_PARAM_FILEINFO, (VOID *)&file_info)) != HD_OK) {
			printf("%s set fileconfig2 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 3) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[3], HD_BSMUX_PARAM_FILEINFO, (VOID *)&file_info)) != HD_OK) {
			printf("%s set fileconfig3 fail\r\n", __func__);
		}
	}
	// (4) BUFINFO
	buf_info.videnc.phy_addr = p_stream->venc_bufinfo.buf_info.phy_addr;
	buf_info.videnc.buf_size = p_stream->venc_bufinfo.buf_info.buf_size;
	buf_info.audenc.phy_addr = p_stream->aenc_bufinfo.buf_info.phy_addr;
	buf_info.audenc.buf_size = p_stream->aenc_bufinfo.buf_info.buf_size;
	if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[0], HD_BSMUX_PARAM_BUFINFO, (VOID *)&buf_info)) != HD_OK) {
		printf("%s set bufconfig0 fail\r\n", __func__);
	}
	if (p_stream->bsmuxer_path_count > 1) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[1], HD_BSMUX_PARAM_BUFINFO, (VOID *)&buf_info)) != HD_OK) {
			printf("%s set bufconfig1 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 2) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[2], HD_BSMUX_PARAM_BUFINFO, (VOID *)&buf_info)) != HD_OK) {
			printf("%s set bufconfig2 fail\r\n", __func__);
		}
	}
	if (p_stream->bsmuxer_path_count > 3) {
		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[3], HD_BSMUX_PARAM_BUFINFO, (VOID *)&buf_info)) != HD_OK) {
			printf("%s set bufconfig3 fail\r\n", __func__);
		}
	}
	// (5) METADATA
	if (g_meta_data) {
		HD_BSMUX_META_ALLOC meta_alloc[2] = {0};

		meta_alloc[0].sign = MAKEFOURCC('G','Y','R','O');
		meta_alloc[0].data_size = 512;
		meta_alloc[0].data_rate = 30;
		meta_alloc[0].p_next = &meta_alloc[1];
		meta_alloc[1].sign = MAKEFOURCC('2','L','U','T');
		meta_alloc[1].data_size = 256;
		meta_alloc[1].data_rate = 30;
		meta_alloc[1].p_next = NULL;

		if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[0], HD_BSMUX_PARAM_META_ALLOC, (VOID *)&meta_alloc)) != HD_OK) {
			printf("%s set metaconfig0 fail\r\n", __func__);
		}
		if (p_stream->bsmuxer_path_count > 1) {
			if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[1], HD_BSMUX_PARAM_META_ALLOC, (VOID *)&meta_alloc)) != HD_OK) {
				printf("%s set metaconfig1 fail\r\n", __func__);
			}
		}
		if (p_stream->bsmuxer_path_count > 2) {
			if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[2], HD_BSMUX_PARAM_META_ALLOC, (VOID *)&meta_alloc)) != HD_OK) {
				printf("%s set metaconfig2 fail\r\n", __func__);
			}
		}
		if (p_stream->bsmuxer_path_count > 3) {
			if ((hd_ret = hd_bsmux_set(p_stream->bsmuxer_path[3], HD_BSMUX_PARAM_META_ALLOC, (VOID *)&meta_alloc)) != HD_OK) {
				printf("%s set metaconfig3 fail\r\n", __func__);
			}
		}
	}

	return HD_OK;
}

static HD_RESULT set_bsmuxer_utility(BSMUXER_STREAM *p_stream)
{
	HD_BSMUX_EN_UTIL util = {0};
	HD_PATH_ID bsmuxer_ctrl;
	HD_RESULT hd_ret;

	if (p_stream->bsmuxer_util.type & HD_BSMUX_EN_UTIL_FRONTMOOV) {
		util.type = HD_BSMUX_EN_UTIL_FRONTMOOV;
		util.enable = 1;
		util.resv[0] = 5;
		util.resv[1] = 0;

		if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmuxer_ctrl)) != HD_OK) {
			printf("%s open ctrl fail\r\n", __func__);
			return hd_ret;
		}
		if ((hd_ret = hd_bsmux_set(bsmuxer_ctrl, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
			printf("%s set fail\r\n", __func__);
		}
		if ((hd_ret = hd_bsmux_close(bsmuxer_ctrl)) != HD_OK) {
			printf("%s close ctrl fail\r\n", __func__);
		}
		p_stream->bsmuxer_util.type &= ~(HD_BSMUX_EN_UTIL_FRONTMOOV);
	}

	return HD_OK;
}

static HD_RESULT init_bsmuxer_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_gfx_init()) != HD_OK)
		return ret;
	if ((ret = hd_bsmux_init()) != HD_OK)
		return ret;
	g_hwclock_module = ((HWCLK_TIME.tm_year == 0) && (HWCLK_TIME.tm_mon == 0) && (HWCLK_TIME.tm_mday == 0));
	if (g_hwclock_module) hwclock_open(HWCLOCK_MODE_RTC);
	return HD_OK;
}

static HD_RESULT exit_bsmuxer_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_gfx_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_bsmux_uninit()) != HD_OK)
		return ret;
	if (g_hwclock_module) hwclock_close();
	return HD_OK;
}

static HD_RESULT push_video_to_bsmuxer_module(BSMUXER_STREAM *p_stream, HD_VIDEOENC_BS *p_user_bs)
{
	HD_RESULT ret;
#if 1 // newfeature
	HD_BSMUX_BS bsmuxer_data = {0};
	bsmuxer_data.sign = MAKEFOURCC('B','S','M','X');
	bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO | HD_BSMUX_TYPE_USRCB;
	bsmuxer_data.timestamp = hd_gettime_us();
	bsmuxer_data.p_user_bs = (VOID *)p_user_bs;
	bsmuxer_data.p_user_data = (VOID *)p_stream;
	if (g_meta_data) {
		bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
	}
	if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[0], &bsmuxer_data, -1)) != HD_OK)
		return ret;
	if (p_stream->bsmuxer_path_count > 1) {
		if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[1], &bsmuxer_data, -1)) != HD_OK)
			return ret;
	}
	if (p_stream->bsmuxer_path_count > 2) {
		if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[2], &bsmuxer_data, -1)) != HD_OK)
			return ret;
	}
	if (p_stream->bsmuxer_path_count > 3) {
		if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[3], &bsmuxer_data, -1)) != HD_OK)
			return ret;
	}
#else // original
	if ((ret = hd_bsmux_push_in_buf_video(p_stream->bsmuxer_path, p_user_bs, -1)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT push_audio_to_bsmuxer_module(BSMUXER_STREAM *p_stream, HD_AUDIO_BS *p_user_bs)
{
	HD_RESULT ret;
#if 1 // newfeature
	HD_BSMUX_BS bsmuxer_data = {0};
	bsmuxer_data.sign = MAKEFOURCC('B','S','M','X');
	bsmuxer_data.data_type = HD_BSMUX_TYPE_AUDIO | HD_BSMUX_TYPE_USRCB;
	bsmuxer_data.timestamp = hd_gettime_us();
	bsmuxer_data.p_user_bs = (VOID *)p_user_bs;
	bsmuxer_data.p_user_data = (VOID *)p_stream;
	if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[0], &bsmuxer_data, -1)) != HD_OK)
		return ret;
	if (p_stream->bsmuxer_path_count > 1) {
		if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[1], &bsmuxer_data, -1)) != HD_OK)
			return ret;
	}
	if (p_stream->bsmuxer_path_count > 2) {
		if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[2], &bsmuxer_data, -1)) != HD_OK)
			return ret;
	}
	if (p_stream->bsmuxer_path_count > 3) {
		if ((ret = hd_bsmux_push_in_buf_struct(p_stream->bsmuxer_path[3], &bsmuxer_data, -1)) != HD_OK)
			return ret;
	}
#else // original
	if ((ret = hd_bsmux_push_in_buf_audio(p_stream->bsmuxer_path, p_user_bs, -1)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                              Video Flow                                 = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_vcap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT set_vcap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

    #if (RESOLUTION_SET == 0)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	printf("Using nvt_sen_imx290\n");
	#elif (RESOLUTION_SET == 1)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	printf("Using nvt_sen_os05a10\n");
	#elif (RESOLUTION_SET == 2)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	printf("Using nvt_sen_os02k10\n");
	#elif (RESOLUTION_SET == 3)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
	printf("Using nvt_sen_ar0237ir\n");
	#endif


    if(RESOLUTION_SET == 3) {
        cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	    printf("Parallel interface\n");
    }
    if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2) {
	    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
	    printf("MIPI interface\n");
    }
	{
		#if (RESOLUTION_SET == 0)
		printf("Using imx290\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 1)
		printf("Using OS052A\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 2)
		printf("Using OS02K10\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	{
		#if (RESOLUTION_SET == 0)
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 1)
			printf("Using OS052A or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 2)
			printf("Using OS02K10 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, (VOID *)&cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, (VOID *)&iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_vcap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, UINT32 path)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;

		// NOTE: only SHDR with path 1
		{
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		}

		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, (VOID *)&video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, (VOID *)&video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	}
	#else //HD_CROP_ON
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = 0;
		video_crop_param.win.rect.y = 0;
		video_crop_param.win.rect.w = 1920/2;
		video_crop_param.win.rect.h= 1080/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		// NOTE: only SHDR with path 1
		{
			video_out_param.pxlfmt = HD_VIDEO_PXLFMT_RAW12;
		}

		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.depth = 0;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, (VOID *)&video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}
	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		//direct mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, (VOID *)&video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}
	return ret;
}

static HD_RESULT set_vproc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID _out_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_VIDEOPROC_LL_CONFIG video_ll_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, _out_id, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.isp_id = 0;
		} else {
			video_cfg_param.isp_id = 1;
		}

		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		// NOTE: only SHDR with path 1
		video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_RAW12;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, (VOID *)&video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.in_func = 0;
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, (VOID *)&video_path_param);
		//printf("set_proc_param PATH_CONFIG=0x%X\r\n", ret);
	}

	video_ctrl_param.func = 0;
	video_ctrl_param.ref_path_3dnr = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, (VOID *)&video_ctrl_param);

	video_ll_param.delay_trig_lowlatency = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_LL_CONFIG, (VOID *)&video_ll_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_vproc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
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
		video_out_param.depth = 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, (VOID *)&video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, (VOID *)&video_path_param);
	}

	return ret;
}

static HD_RESULT set_venc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};
	HD_VIDEOENC_FUNC_CONFIG video_func_config = {0};

	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H264;
		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		video_path_config.max_mem.enc_buf_ms = 3000;
		video_path_config.max_mem.svc_layer  = HD_SVC_4X;
		video_path_config.max_mem.ltr        = TRUE;
		video_path_config.max_mem.rotate     = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id             = 0;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, (VOID *)&video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}

		video_func_config.in_func = 0;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, (VOID *)&video_func_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}

	return ret;
}

static HD_RESULT set_venc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, (VOID *)&video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}

		printf("enc_type=%d\r\n", enc_type);

		if (enc_type == 0) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, (VOID *)&video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, (VOID *)&rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 1) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, (VOID *)&video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, (VOID *)&rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}

		} else if (enc_type == 2) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, (VOID *)&video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	// set reserved size for TS FLOW
	if (g_tse_module) {
		VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_cfg = {0};
		bs_reserved_cfg.reserved_size = TS_VID_SIZE;
		vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, (VOID *)&bs_reserved_cfg);
	}

	if (g_meta_data) {
		VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_cfg = {0};
		bs_reserved_cfg.reserved_size = TS_VID_SIZE * g_meta_data;
		vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, (VOID *)&bs_reserved_cfg);
	}

	return ret;
}

static HD_RESULT set_venc_meta_param(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	VENDOR_META_ALLOC meta_alloc[2] = {0};

	meta_alloc[0].sign = MAKEFOURCC('G','Y','R','O');
	meta_alloc[0].data_size = 512;
	meta_alloc[0].header_size = HD_VENDOR_META_HEADER_SIZE;
	meta_alloc[0].p_next = &meta_alloc[1];
	meta_alloc[1].sign = MAKEFOURCC('2','L','U','T');
	meta_alloc[1].data_size = 256;
	meta_alloc[1].header_size = HD_VENDOR_META_HEADER_SIZE;
	meta_alloc[1].p_next = NULL;

	ret = vendor_videoenc_set(path_id, VENDOR_VIDEOENC_PARAM_META_ALLC, (VOID *)&meta_alloc);
	return ret;
}

static BOOL _is_data_at_video_pack(HD_VIDEOENC_BS *data_pull, UINT32 pack_idx)
{
	if (data_pull->vcodec_format == HD_CODEC_TYPE_H265) {
		if ((data_pull->video_pack[pack_idx].pack_type.h265_type == H265_NALU_TYPE_VPS) ||
			(data_pull->video_pack[pack_idx].pack_type.h265_type == H265_NALU_TYPE_SPS) ||
			(data_pull->video_pack[pack_idx].pack_type.h265_type == H265_NALU_TYPE_PPS) )
			return TRUE;
	} else if (data_pull->vcodec_format == HD_CODEC_TYPE_H264) {
		if ((data_pull->video_pack[pack_idx].pack_type.h264_type == H264_NALU_TYPE_SPS) ||
			(data_pull->video_pack[pack_idx].pack_type.h264_type == H264_NALU_TYPE_PPS) )
			return TRUE;
	} else if (data_pull->vcodec_format == HD_CODEC_TYPE_JPEG) {
		return FALSE;
	}
	return FALSE;
}

static HD_RESULT pull_out_buf_from_venc_module(BSMUXER_STREAM *p_stream, HD_VIDEOENC_BS *p_video_bs, INT32 wait_ms)
{
	if (g_venc_module)
		return hd_videoenc_pull_out_buf(p_stream->venc_path, p_video_bs, wait_ms);
	return HD_OK;
}

static HD_RESULT release_out_buf_from_venc_module(BSMUXER_STREAM *p_stream, HD_VIDEOENC_BS *p_video_bs)
{
	if (g_venc_module)
		return hd_videoenc_release_out_buf(p_stream->venc_path, p_video_bs);
	return HD_OK;
}

static void *video_record_thread(void *arg)
{
	BSMUXER_STREAM* p_stream0 = (BSMUXER_STREAM *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;

	UINT32 j;
	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	//------ wait flow_start ------
	while (p_stream0->vrec_enter == 0) sleep(1);

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->venc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	//--------- pull data test ---------
	while (p_stream0->vrec_exit == 0) {
		// pull data
		ret = pull_out_buf_from_venc_module(p_stream0, &data_pull, -1); // -1 = blocking mode

		if (ret == HD_OK) {

			if (g_meta_data) {
			for (j=0; j< data_pull.pack_num; j++) {
				if (_is_data_at_video_pack(&data_pull, j) == TRUE) {
					// H265(VPS/SPS/PPS), H264(SPS/PPS)

				} else {
					// H265(IDR/SLICE), H264(IDR/SLICE), JPEG
					UINT32                 nalu_num  = *(UINT32*)PHY2VIRT_MAIN((UINT32)data_pull.p_next);
					//HD_BUFINFO             *nalu_data = (HD_BUFINFO *)(PHY2VIRT_MAIN((UINT32)data_pull.p_next)+sizeof(UINT32));
					VENDOR_COMM_DUMMY      *meta_data = (VENDOR_COMM_DUMMY *)ALIGN_CEIL_64(PHY2VIRT_MAIN((UINT32)data_pull.p_next)+sizeof(UINT32)+(nalu_num * sizeof(HD_BUFINFO)));

					//=> meta_info
					while (meta_data != NULL) {
						//test data
						#if 0
						{
							UINT8 *ptr = (UINT8 *)(meta_data) + meta_data->header_size; //little endian
							//memset(ptr, 0x0, meta_data->buffer_size);
							*(ptr)     = (meta_data->sign) & 0xFF;
							*(ptr + 1) = (meta_data->sign >> 8) & 0xFF;
							*(ptr + 2) = (meta_data->sign >> 16) & 0xFF;
							*(ptr + 3) = (meta_data->sign >> 24) & 0xFF;
							*(ptr + 4)     = (meta_data->sign) & 0xFF;
							*(ptr + 5) = (meta_data->sign >> 8) & 0xFF;
							*(ptr + 6) = (meta_data->sign >> 16) & 0xFF;
							*(ptr + 7) = (meta_data->sign >> 24) & 0xFF;
						}
						#endif

						//changed to use offest
						if (meta_data->p_next == NULL) break; //last one
						meta_data = (VENDOR_COMM_DUMMY *)((UINT32)meta_data + meta_data->header_size + meta_data->buffer_size);
					}
				}
			}
			}

			// push bitstream to bsmuxer
			if (g_bsmuxer_module) push_video_to_bsmuxer_module(p_stream0, &data_pull);

			// release data
			ret = release_out_buf_from_venc_module(p_stream0, &data_pull);
			if (ret != HD_OK) {
				printf("enc_release error=%d !!\r\n", ret);
			}
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                              Audio Flow                                 = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_acap_cfg(HD_PATH_ID *p_audio_cap_ctrl)
{
	HD_RESULT ret;
	HD_PATH_ID audio_cap_ctrl = 0;
	HD_AUDIOCAP_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_drv_cfg = {0};

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap dev parameter
	audio_dev_cfg.in_max.sample_rate = ADO_SR;
	audio_dev_cfg.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_dev_cfg.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_dev_cfg.in_max.frame_sample = 1024;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, (VOID *)&audio_dev_cfg);
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap drv parameter
	audio_drv_cfg.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, (VOID *)&audio_drv_cfg);

	*p_audio_cap_ctrl = audio_cap_ctrl;
	return ret;
}

static HD_RESULT set_acap_param(HD_PATH_ID audio_cap_path)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_param = {0};

	// set audiocap input parameter
	audio_cap_param.sample_rate = ADO_SR;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cap_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, (VOID *)&audio_cap_param);

	return ret;
}

static HD_RESULT set_aenc_cfg(HD_PATH_ID audio_enc_path, UINT32 enc_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_PATH_CONFIG audio_path_cfg = {0};

	// set audioenc path config
	audio_path_cfg.max_mem.codec_type = enc_type;
	audio_path_cfg.max_mem.sample_rate = ADO_SR;
	audio_path_cfg.max_mem.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_path_cfg.max_mem.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_PATH_CONFIG, (VOID *)&audio_path_cfg);

	return ret;
}

static HD_RESULT set_aenc_param(HD_PATH_ID audio_enc_path, UINT32 enc_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_IN audio_in_param = {0};
	HD_AUDIOENC_OUT audio_out_param = {0};

	// set audioenc input parameter
	audio_in_param.sample_rate = ADO_SR;
	audio_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_in_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_IN, (VOID *)&audio_in_param);
	if (ret != HD_OK) {
		printf("set_enc_param_in = %d\r\n", ret);
		return ret;
	}

	// set audioenc output parameter
	audio_out_param.codec_type = enc_type;
	audio_out_param.aac_adts = (enc_type == HD_AUDIO_CODEC_AAC) ? TRUE : FALSE;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_OUT, (VOID *)&audio_out_param);
	if (ret != HD_OK) {
		printf("set_enc_param_out = %d\r\n", ret);
		return ret;
	}

	// set reserved size for TS FLOW
	if (g_tse_module) {
		VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG bs_reserved_cfg = {0};
		bs_reserved_cfg.reserved_size = TS_AUD_SIZE;
		vendor_audioenc_set(audio_enc_path, VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE, (VOID *)&bs_reserved_cfg);
	}

	return ret;
}

static HD_RESULT pull_out_buf_from_aenc_module(BSMUXER_STREAM *p_stream, HD_AUDIO_BS *p_audio_bs, INT32 wait_ms)
{
	if (g_aenc_module)
		return hd_audioenc_pull_out_buf(p_stream->aenc_path, p_audio_bs, wait_ms);
	return HD_OK;
}

static HD_RESULT release_out_buf_from_aenc_module(BSMUXER_STREAM *p_stream, HD_AUDIO_BS *p_audio_bs)
{
	if (g_aenc_module)
		return hd_audioenc_release_out_buf(p_stream->aenc_path, p_audio_bs);
	return HD_OK;
}

static void *audio_record_thread(void *arg)
{
	BSMUXER_STREAM* p_stream0 = (BSMUXER_STREAM *)arg;
	HD_RESULT ret = HD_OK;
	HD_AUDIO_BS  data_pull;

	//------ wait flow_start ------
	while (p_stream0->arec_enter == 0) sleep(1);

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->arec_exit == 0) {
		// pull data
		ret = pull_out_buf_from_aenc_module(p_stream0, &data_pull, -1); // -1 = blocking mode

		if (ret == HD_OK) {

			// push bitstream to bsmuxer
			if (g_bsmuxer_module) push_audio_to_bsmuxer_module(p_stream0, &data_pull);

			// release data
			ret = release_out_buf_from_aenc_module(p_stream0, &data_pull);
			if (ret != HD_OK) {
				printf("release_ouf_buf fail, ret(%d)\r\n", ret);
			}
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/* =========================================================================== */
/* =                               Main Flow                                 = */
/* =========================================================================== */
///////////////////////////////////////////////////////////////////////////////
static HD_RESULT init_module(BSMUXER_STREAM *p_stream)
{
	HD_RESULT ret;
	if (g_meta_data)
		if ((ret = init_meta_data(p_stream)) != HD_OK) return ret;
	if (g_vcap_module)
		if ((ret = hd_videocap_init()) != HD_OK) return ret;
	if (g_vproc_module)
		if ((ret = hd_videoproc_init()) != HD_OK) return ret;
	if (g_venc_module)
		if ((ret = hd_videoenc_init()) != HD_OK) return ret;
	if (g_acap_module)
		if((ret = hd_audiocap_init()) != HD_OK) return ret;
	if (g_aenc_module)
		if((ret = hd_audioenc_init()) != HD_OK) return ret;
	if (g_filesys_module) // hdanle sdio
		if ((ret = init_filesys_module(p_stream)) != HD_OK) return ret;
	if (g_fileout_module)
		if ((ret = hd_fileout_init()) != HD_OK) return ret;
	if (g_bsmuxer_module) // hdanle gfx, hwclock
		if ((ret = init_bsmuxer_module()) != HD_OK) return ret;
	return HD_OK;
}

static HD_RESULT open_module(BSMUXER_STREAM *p_stream)
{
	HD_RESULT ret;
	if (g_vcap_module) {
		// set videocap config
		ret = set_vcap_cfg(&p_stream->vcap_ctrl);
		if (ret != HD_OK) {
			printf("set vcap-cfg fail=%d\n", ret);
			return HD_ERR_NG;
		}
	}
	if (g_vproc_module) {
		// set videoproc config
		ret = set_vproc_cfg(&p_stream->vproc_ctrl, &p_stream->vproc_max_dim, HD_VIDEOPROC_0_CTRL);
		if (ret != HD_OK) {
			printf("set vproc-cfg fail=%d\n", ret);
			return HD_ERR_NG;
		}
	}
	if (g_vcap_module)
		if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->vcap_path)) != HD_OK) return ret;
	if (g_vproc_module)
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->vproc_path)) != HD_OK) return ret;
	if (g_venc_module)
		if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->venc_path)) != HD_OK) return ret;
	if (g_acap_module) {
		// set audiocap config
		ret = set_acap_cfg(&p_stream->acap_ctrl);
		if (ret != HD_OK) {
			printf("set acap-cfg fail\r\n");
			return HD_ERR_NG;
		}
	}
	if (g_acap_module)
		if ((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_stream->acap_path)) != HD_OK) return ret;
	if (g_aenc_module)
		if ((ret = hd_audioenc_open(HD_AUDIOENC_0_IN_0, HD_AUDIOENC_0_OUT_0, &p_stream->aenc_path)) != HD_OK) return ret;
	if (g_fileout_module)
		if ((ret = hd_fileout_open(HD_FILEOUT_IN(0, 0), HD_FILEOUT_OUT(0, 0), &p_stream->fileout_path[0])) != HD_OK) return ret;
	if (g_fileout_module && p_stream->bsmuxer_path_count > 1)
		if ((ret = hd_fileout_open(HD_FILEOUT_IN(0, 1), HD_FILEOUT_OUT(0, 1), &p_stream->fileout_path[1])) != HD_OK) return ret;
	if (g_fileout_module && p_stream->bsmuxer_path_count > 2)
		if ((ret = hd_fileout_open(HD_FILEOUT_IN(0, 2), HD_FILEOUT_OUT(0, 2), &p_stream->fileout_path[2])) != HD_OK) return ret;
	if (g_fileout_module && p_stream->bsmuxer_path_count > 3)
		if ((ret = hd_fileout_open(HD_FILEOUT_IN(0, 3), HD_FILEOUT_OUT(0, 3), &p_stream->fileout_path[3])) != HD_OK) return ret;
	if (g_bsmuxer_module)
		if ((ret = hd_bsmux_open(HD_BSMUX_IN(0, 0), HD_BSMUX_OUT(0, 0), &p_stream->bsmuxer_path[0])) != HD_OK) return ret;
	if (g_bsmuxer_module && p_stream->bsmuxer_path_count > 1)
		if ((ret = hd_bsmux_open(HD_BSMUX_IN(0, 1), HD_BSMUX_OUT(0, 1), &p_stream->bsmuxer_path[1])) != HD_OK) return ret;
	if (g_bsmuxer_module && p_stream->bsmuxer_path_count > 2)
		if ((ret = hd_bsmux_open(HD_BSMUX_IN(0, 2), HD_BSMUX_OUT(0, 2), &p_stream->bsmuxer_path[2])) != HD_OK) return ret;
	if (g_bsmuxer_module && p_stream->bsmuxer_path_count > 3)
		if ((ret = hd_bsmux_open(HD_BSMUX_IN(0, 3), HD_BSMUX_OUT(0, 3), &p_stream->bsmuxer_path[3])) != HD_OK) return ret;
	return HD_OK;
}

static HD_RESULT close_module(BSMUXER_STREAM *p_stream)
{
	HD_RESULT ret;
	if (g_vcap_module)
		if ((ret = hd_videocap_close(p_stream->vcap_path)) != HD_OK) return ret;
	if (g_vproc_module)
		if ((ret = hd_videoproc_close(p_stream->vproc_path)) != HD_OK) return ret;
	if (g_venc_module)
		if ((ret = hd_videoenc_close(p_stream->venc_path)) != HD_OK) return ret;
	if (g_acap_module)
		if ((ret = hd_audiocap_close(p_stream->acap_path)) != HD_OK) return ret;
	if (g_aenc_module)
		if ((ret = hd_audioenc_close(p_stream->aenc_path)) != HD_OK) return ret;
	if (g_bsmuxer_module)
		if ((ret = hd_bsmux_close(p_stream->bsmuxer_path[0])) != HD_OK) return ret;
	if (g_bsmuxer_module && p_stream->bsmuxer_path_count > 1)
		if ((ret = hd_bsmux_close(p_stream->bsmuxer_path[1])) != HD_OK) return ret;
	if (g_bsmuxer_module && p_stream->bsmuxer_path_count > 2)
		if ((ret = hd_bsmux_close(p_stream->bsmuxer_path[2])) != HD_OK) return ret;
	if (g_bsmuxer_module && p_stream->bsmuxer_path_count > 3)
		if ((ret = hd_bsmux_close(p_stream->bsmuxer_path[3])) != HD_OK) return ret;
	if (g_fileout_module)
		if ((ret = hd_fileout_close(p_stream->fileout_path[0])) != HD_OK) return ret;
	if (g_fileout_module && p_stream->bsmuxer_path_count > 1)
		if ((ret = hd_fileout_close(p_stream->fileout_path[1])) != HD_OK) return ret;
	if (g_fileout_module && p_stream->bsmuxer_path_count > 2)
		if ((ret = hd_fileout_close(p_stream->fileout_path[2])) != HD_OK) return ret;
	if (g_fileout_module && p_stream->bsmuxer_path_count > 3)
		if ((ret = hd_fileout_close(p_stream->fileout_path[3])) != HD_OK) return ret;
	return HD_OK;
}

static HD_RESULT exit_module(BSMUXER_STREAM *p_stream)
{
	HD_RESULT ret;
	if (g_vcap_module)
		if ((ret = hd_videocap_uninit()) != HD_OK) return ret;
	if (g_vproc_module)
		if ((ret = hd_videoproc_uninit()) != HD_OK) return ret;
	if (g_venc_module)
		if ((ret = hd_videoenc_uninit()) != HD_OK) return ret;
	if (g_acap_module)
		if((ret = hd_audiocap_uninit()) != HD_OK) return ret;
	if (g_aenc_module)
		if((ret = hd_audioenc_uninit()) != HD_OK) return ret;
	if (g_bsmuxer_module) // hdanle gfx, hwclock
		if ((ret = exit_bsmuxer_module()) != HD_OK) return ret;
	if (g_fileout_module)
		if ((ret = hd_fileout_uninit()) != HD_OK) return ret;
	if (g_filesys_module) // hdanle sdio
		if ((ret = exit_filesys_module(p_stream)) != HD_OK) return ret;
	if (g_meta_data)
		if ((ret = exit_meta_data(p_stream)) != HD_OK) return ret;
	return HD_OK;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	BSMUXER_STREAM stream[1] = {0}; //0: main stream
	UINT32 venc_type = 0;
	UINT32 aenc_type = 0; //default aac
	UINT32 bsmu_type = 0;
	UINT32 bsmu_util = 0;
	UINT32 bsmu_path = 1;

	{
		printf("Usage: <venc_type> <aenc_type>.\r\n");
		printf("Help:\r\n");
		printf("  <venc_type>    : 0(H265), 1(H264)\r\n");
		printf("  <aenc_type>    : 0(AAC), 1(PCM)\r\n");
		printf("  <bsmu_type>    : 0(MOV/MP4), 1(TS)\r\n");
		printf("  <bsmu_util>    : 0(NORMAL), 0x1(FRONT_MOOV)\r\n");
		printf("  <bsmu_path>    : 1(DEFAULT) ~ 4(MAX)\r\n");
	}

	// query program options
	if (argc >= 2) {
		venc_type = atoi(argv[1]);
		printf("venc_type %d\r\n", venc_type);
		if(venc_type > 1) {
			printf("error: not support venc_type!\r\n");
			return 0;
		}
	}
	if (argc >= 3) {
		aenc_type = atoi(argv[2]);
		printf("aenc_type %d\r\n", aenc_type);
		if(aenc_type > 1) {
			printf("error: not support aenc_type!\r\n");
			return 0;
		}
	}
	if (argc >= 4) {
		bsmu_type = atoi(argv[3]);
		printf("bsmu_type %d\r\n", bsmu_type);
		if(bsmu_type > 1) {
			printf("error: not support bsmu_type!\r\n");
			return 0;
		}
	}
	if (argc >= 5) {
		bsmu_util = atoi(argv[4]);
		printf("bsmu_util %d\r\n", bsmu_util);
		if(bsmu_util > 1) {
			printf("error: not support bsmu_util!\r\n");
			return 0;
		}
	}
	if (argc >= 6) {
		bsmu_path = atoi(argv[5]);
		printf("bsmu_path %d\r\n", bsmu_path);
		if(bsmu_path > BSMUXER_MAX || bsmu_path < 1) {
			printf("error: not support bsmu_path!\r\n");
			return 0;
		}
	}
	if (venc_type == 0)
		stream[0].venc_codec = HD_CODEC_TYPE_H265;
	else if (venc_type == 1)
		stream[0].venc_codec = HD_CODEC_TYPE_H264;
	if (aenc_type == 0)
		stream[0].aenc_codec = HD_AUDIO_CODEC_AAC;
	else if (aenc_type == 1)
		stream[0].aenc_codec = HD_AUDIO_CODEC_PCM;
	printf("bsmu_type=%d\r\n", bsmu_type);
	if (bsmu_type == 0)
		stream[0].bsmuxer_type = HD_BSMUX_FTYPE_MP4;
	else if (bsmu_type == 1)
		stream[0].bsmuxer_type = HD_BSMUX_FTYPE_TS;
	printf("bsmuxer_type=%d\r\n", stream[0].bsmuxer_type);
	g_tse_module = (bsmu_type == 1);
	if (bsmu_util & 0x1)
		stream[0].bsmuxer_util.type |= HD_BSMUX_EN_UTIL_FRONTMOOV;
	stream[0].bsmuxer_path_count = bsmu_path;
	stream[0].fileout_path_count = bsmu_path; //1-to-1

#if defined(__LINUX)
	if (g_tse_module) system(INS_TSE_KO);
#endif

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module(&stream[0]);
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	// open all modules
	if (g_vproc_module) {
		stream[0].vproc_max_dim.w = VDO_SIZE_W; //assign by user
		stream[0].vproc_max_dim.h = VDO_SIZE_H; //assign by user
	}
	ret = open_module(&stream[0]);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// get videocap capability
	if (g_vcap_module) {
		ret = get_vcap_caps(stream[0].vcap_ctrl, &stream[0].vcap_syscaps);
		if (ret != HD_OK) {
			printf("get cap-caps fail=%d\n", ret);
			goto exit;
		}
	}

	// set videocap parameter
	if (g_vcap_module) {
		stream[0].vcap_dim.w = VDO_SIZE_W; //assign by user
		stream[0].vcap_dim.h = VDO_SIZE_H; //assign by user
		ret = set_vcap_param(stream[0].vcap_path, &stream[0].vcap_dim, 0);
		if (ret != HD_OK) {
			printf("set cap fail=%d\n", ret);
			goto exit;
		}
	}

	// set videoproc parameter (main)
	if (g_vproc_module) {
		ret = set_vproc_param(stream[0].vproc_path, NULL); //keep vprc's out dim = {0,0}, it means auto sync from venc's in dim
		if (ret != HD_OK) {
			printf("set proc fail=%d\n", ret);
			goto exit;
		}
	}

	// set videoenc config (main)
	if (g_venc_module) {
		stream[0].venc_max_dim.w = VDO_SIZE_W;
		stream[0].venc_max_dim.h = VDO_SIZE_H;
		ret = set_venc_cfg(stream[0].venc_path, &stream[0].venc_max_dim, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc-cfg fail=%d\n", ret);
			goto exit;
		}
	}

	// set videoenc parameter (main)
	if (g_venc_module) {
		stream[0].venc_dim.w = VDO_SIZE_W;
		stream[0].venc_dim.h = VDO_SIZE_H;
		ret = set_venc_param(stream[0].venc_path, &stream[0].venc_dim, venc_type, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
			goto exit;
		}
	}

	if (g_meta_data) {
		ret = set_venc_meta_param(stream[0].venc_path);
		if (ret != HD_OK) {
			printf("set_venc_meta_param error=%d\r\n", ret);
		}
	}

	// set audiocap parameter
	if (g_acap_module) {
		ret = set_acap_param(stream[0].acap_path);
		if (ret != HD_OK) {
			printf("set cap fail=%d\n", ret);
			goto exit;
		}
	}

	// set audioenc config
	if (g_aenc_module) {
		ret = set_aenc_cfg(stream[0].aenc_path, stream[0].aenc_codec);
		if (ret != HD_OK) {
			printf("set enc-cfg fail=%d\n", ret);
			goto exit;
		}
	}

	// set audioenc paramter
	if (g_aenc_module) {
		ret = set_aenc_param(stream[0].aenc_path, stream[0].aenc_codec);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
			goto exit;
		}
	}

	// bind video_record modules (main)
	if (g_vcap_module) hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	// bind video_record modules (main)
	if (g_vproc_module) hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);

	// create video_record_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].vrec_thread_id, NULL, video_record_thread, (void *)stream);
	if (ret < 0) {
		printf("create video_record_thread failed");
		goto exit;
	}

	// bind audio_record modules
	if (g_acap_module) hd_audiocap_bind(HD_AUDIOCAP_0_OUT_0, HD_AUDIOENC_0_IN_0);

	// create audio_record_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].arec_thread_id, NULL, audio_record_thread, (void *)stream);
	if (ret < 0) {
		printf("create audio_record_thread failed\n");
		goto exit;
	}

	// start video_record modules (main)
	//direct NOTE: ensure videocap start after 1st videoproc phy path start
	if (g_vproc_module) hd_videoproc_start(stream[0].vproc_path);
	if (g_vcap_module) hd_videocap_start(stream[0].vcap_path);

	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	if (g_venc_module) hd_videoenc_start(stream[0].venc_path);
	// start audio_record modules
	if (g_acap_module) hd_audioenc_start(stream[0].aenc_path);
	if (g_aenc_module) hd_audiocap_start(stream[0].acap_path);

	// start fileout module
	if (g_fileout_module) {
		// user config
		ret = set_fileout_config(&stream[0]);
		if (ret != HD_OK) {
			printf("set fileout config fail=%d\n", ret);
			goto exit;
		}
		ret = hd_fileout_start(stream[0].fileout_path[0]);
		if (ret != HD_OK) {
			printf("start fileout fail=%d\n", ret);
			goto exit;
		}
		if (stream[0].fileout_path_count > 1)
			if ((ret = hd_fileout_start(stream[0].fileout_path[1])) != HD_OK) return ret;
		if (stream[0].fileout_path_count > 2)
			if ((ret = hd_fileout_start(stream[0].fileout_path[2])) != HD_OK) return ret;
		if (stream[0].fileout_path_count > 3)
			if ((ret = hd_fileout_start(stream[0].fileout_path[3])) != HD_OK) return ret;
	}

	// start bsmuxer module
	if (g_bsmuxer_module) {
		hd_videoenc_get(stream[0].venc_path, HD_VIDEOENC_PARAM_BUFINFO, (VOID *)&(stream[0].venc_bufinfo));
		hd_audioenc_get(stream[0].aenc_path, HD_AUDIOENC_PARAM_BUFINFO, (VOID *)&(stream[0].aenc_bufinfo));
		ret = set_bsmuxer_config(&stream[0]);
		if (ret != HD_OK) {
			printf("set bsmuxer config fail=%d\n", ret);
			goto exit;
		}
		ret = set_bsmuxer_utility(&stream[0]);
		if (ret != HD_OK) {
			printf("set bsmuxer utility fail=%d\n", ret);
			goto exit;
		}
		ret = hd_bsmux_start(stream[0].bsmuxer_path[0]);
		if (ret != HD_OK) {
			printf("start bsmuxer fail=%d\n", ret);
			goto exit;
		}
		if (stream[0].bsmuxer_path_count > 1)
			if ((ret = hd_bsmux_start(stream[0].bsmuxer_path[1])) != HD_OK) return ret;
		if (stream[0].bsmuxer_path_count > 2)
			if ((ret = hd_bsmux_start(stream[0].bsmuxer_path[2])) != HD_OK) return ret;
		if (stream[0].bsmuxer_path_count > 3)
			if ((ret = hd_bsmux_start(stream[0].bsmuxer_path[3])) != HD_OK) return ret;
	}

	// start video_record_thread
	stream[0].vrec_enter = 1;

	// start audio_record_thread
	stream[0].arec_enter = 1;

	// query user key
	printf("Enter q to exit (stream-0x%lx)\n", (unsigned long)&stream[0]);
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// quit program
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	// stop video_record_thread
	stream[0].vrec_exit = 1;

	// start audio_record_thread
	stream[0].arec_exit = 1;

	usleep(50000);

	// stop video_record modules (main)
	//direct NOTE: ensure videocap stop after all videoproc path stop
	if (g_vproc_module) hd_videoproc_stop(stream[0].vproc_path);
	if (g_vcap_module) hd_videocap_stop(stream[0].vcap_path);
	if (g_venc_module) hd_videoenc_stop(stream[0].venc_path);

	// stop audio_record modules
	if (g_acap_module) hd_audiocap_stop(stream[0].acap_path);
	if (g_aenc_module) hd_audioenc_stop(stream[0].aenc_path);

	// stop bsmuxer module
	if (g_bsmuxer_module) hd_bsmux_stop(stream[0].bsmuxer_path[0]);
	if (g_bsmuxer_module && stream[0].bsmuxer_path_count > 1) hd_bsmux_stop(stream[0].bsmuxer_path[1]);
	if (g_bsmuxer_module && stream[0].bsmuxer_path_count > 2) hd_bsmux_stop(stream[0].bsmuxer_path[2]);
	if (g_bsmuxer_module && stream[0].bsmuxer_path_count > 3) hd_bsmux_stop(stream[0].bsmuxer_path[3]);

	// stop fileout module
	if (g_fileout_module) hd_fileout_stop(stream[0].fileout_path[0]);
	if (g_fileout_module && stream[0].fileout_path_count > 1) hd_fileout_stop(stream[0].fileout_path[1]);
	if (g_fileout_module && stream[0].fileout_path_count > 2) hd_fileout_stop(stream[0].fileout_path[2]);
	if (g_fileout_module && stream[0].fileout_path_count > 3) hd_fileout_stop(stream[0].fileout_path[3]);

	// destroy video_record_thread
	pthread_join(stream[0].vrec_thread_id, NULL); //NOTE: before destory, call stop to breaking pull(-1)

	// unbind video_record modules (main)
	if (g_vcap_module) hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);

	// unbind video_record modules (main)
	if (g_vproc_module) hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

	// destroy audio_record_thread
	pthread_join(stream[0].arec_thread_id, NULL);

	// unbind audio_record modules
	if (g_acap_module) hd_audiocap_unbind(HD_AUDIOCAP_0_OUT_0);

exit:
	// close all modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module(&stream[0]);
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return 0;
}
