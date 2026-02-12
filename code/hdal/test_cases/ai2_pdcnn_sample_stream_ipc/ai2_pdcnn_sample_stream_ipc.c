/**
	@brief Sample code of ai network with sensor input.\n

	@file alg_net_app_user_sample_stream.c

	@author Joshua Liu

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include <sys/time.h>
#include <dirent.h>

#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"
#include "pdcnn_lib_ai2.h"
#include "pd_shm.h"

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_liveview, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 			1

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
#define AI_IPC              ENABLE

#define SEN_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W		128
#define YOUT_WIN_NUM_H		128

#define VDO_SIZE_W			1920
#define VDO_SIZE_H			1080
#define SENSOR_4M           ENABLE

// nn
#define DEFAULT_DEVICE		"/dev/" VENDOR_AIS_FLOW_DEV_NAME

//#define WRITE_PD_RESULT		DISABLE
#define DBG_OBJ_INFOR       DISABLE
#define	SENSOR_291			1
#define	SENSOR_S02K			2
#define	SENSOR_CHOICE		SENSOR_291 // SENSOR_S02K

#if (!AI_IPC)
#define AI_PD_BUF_SIZE   	PD_MAX_MEM_SIZE
#else
#define AI_PD_BUF_SIZE      (0x580000)//(NETWORKS_MEM_SIZE + POSTPROC_MEM_SIZE)
#endif
#define VENDOR_AI_CFG  		0x000f0000  //ai project config

#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420

#define NN_PDCNN_PROF		ENABLE

#define HD_VIDEOPROC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

typedef struct _VIDEO_LIVEVIEW {
#if(!AI_IPC)
	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;
#endif
	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

#if(!AI_IPC)
	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
#endif
	// (4) --
	HD_VIDEOPROC_SYSCAPS proc_alg_syscaps;
	HD_PATH_ID proc_alg_ctrl;
	HD_PATH_ID proc_alg_path;

	HD_DIM  proc_alg_max_dim;
	HD_DIM  proc_alg_dim;

	// (5) --
	HD_PATH_ID mask_alg_path;

#if NN_PDCNN_DRAW
	HD_PATH_ID mask_path0;
    HD_PATH_ID mask_path1;
    HD_PATH_ID mask_path2;
    HD_PATH_ID mask_path3;
	HD_PATH_ID mask_path4;
    HD_PATH_ID mask_path5;
    HD_PATH_ID mask_path6;
    HD_PATH_ID mask_path7;
#endif

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;


static CHAR model_name[2][256]	= { {"/mnt/sd/CNNLib/para/pdcnn_v4_ll/nvt_model.bin"},
									{"/mnt/sd/CNNLib/para/pdcnn_v4_ll/nvt_model_CNN2.bin"}
	                              };

static BOOL is_net_proc						= TRUE;
static BOOL is_net_run						= FALSE;
static UINT32 fps_delay = 0;
static INT32 save_results = 0;
static FLOAT sensor_ratio[2] = {0, 0};
MEM_PARM rel_buf = {0};

typedef struct _NET_PROC {
	CHAR model_filename[256];
	INT32 binsize;
	int job_method;
	int job_wait_ms;
	int buf_method;
	//MEM_PARM proc_mem;
	UINT32 proc_id;
	//MEM_PARM io_mem;
} NET_PROC;

typedef struct _VENDOR_AIS_PDCNN_PARM {
	NET_PROC net_info;
	PDCNN_MEM pdcnn_mem;
	VIDEO_LIVEVIEW stream;
} VENDOR_AIS_PDCNN_PARM;

typedef struct _THREAD_DRAW_PARM {
    PDCNN_MEM pd_mem;
    VIDEO_LIVEVIEW stream;
	FLOAT ratio_lcd[2];
} THREAD_DRAW_PARM;
static INT32 final_pdobj_num = 0;

///////////////////////////////////////////////////////////////////////////////

#if(AI_IPC)
static char   *g_shm = NULL;
#endif

///////////////////////////////////////////////////////////////////////////////

#if(AI_IPC)
void init_share_memory(void)
{
	int shmid = 0;
	key_t key;

	// Segment key.
	key = PD_SHM_KEY;
	// Create the segment.
	if( ( shmid = shmget( key, PD_SHMSZ, 0666 ) ) < 0 ) {
		perror( "shmget" );
		exit(1);
	}
	// Attach the segment to the data space.
	if( ( g_shm = shmat( shmid, NULL, 0 ) ) == (char *)-1 ) {
		perror( "shmat" );
		exit(1);
	}
}

void exit_share_memory(void)
{
	shmdt(g_shm);
}
#endif

///////////////////////////////////////////////////////////////////////////////
static HD_RESULT mem_init(UINT32 ddr, MEM_PARM *buf)
{
	HD_RESULT              		ret;
	HD_COMMON_MEM_DDR_ID	  	ddr_id = ddr;
	HD_COMMON_MEM_VB_BLK      	blk;
	UINT32                    	pa, va;

#if(!AI_IPC)

	HD_COMMON_MEM_INIT_CONFIG	mem_cfg = {0};


	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        											 +VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        											 +VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	// nn mem pool ---
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[2].blk_size = AI_PD_BUF_SIZE;
	mem_cfg.pool_info[2].blk_cnt = 1;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	// - end nn mem pool

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}
#else
	ret = hd_common_mem_init(NULL);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}

	/* Allocate parameter buffer */
	if (buf->va != 0) {
		printf("err: mem has already been inited\r\n");
		return -1;
	}
#endif
	// nn get block ---
	blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, AI_PD_BUF_SIZE, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, AI_PD_BUF_SIZE);
	//g_blk_info[0] = blk;

	// Release buffer
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, AI_PD_BUF_SIZE);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}

	buf->pa = pa;
	buf->va = va;
	buf->size = AI_PD_BUF_SIZE;
	buf->blk = blk;

exit:
	// - end nn get block

	return ret;
}

static HD_RESULT mem_exit(VOID)
{
	HD_RESULT ret = HD_OK;
	/* Release in buffer */
	if (rel_buf.va) {
		ret = hd_common_mem_munmap((void *)rel_buf.va, AI_PD_BUF_SIZE);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(rel_buf.blk);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	hd_common_mem_uninit();
	return ret;
}

//////////////////////////////////////////////////////////////////////////////
//model load
static INT32 _getsize_model(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail!!!!!!!!\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}

UINT32 _load_model(CHAR *filename, MEM_PARM *mem_parm)
{
	FILE *fd;
	INT32 size = 0;
	fd = fopen(filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
	};
	//mem_parm->size = size;
	
	if (fd) {
		fclose(fd);
	};
	printf("model buf size: %d\r\n", size);
	return size;
}
//////////////////////////////////////////////////////////////////////////////
static HD_RESULT network_open(NET_PROC *p_net, PDCNN_MEM *pdcnn_mem)
{
	HD_RESULT ret = HD_OK;

	if (strlen(p_net->model_filename) == 0) {
		printf("proc_id(%u) input model is null\r\n", p_net->proc_id);
		return 0;
	}
	// set model
	vendor_ai_net_set(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)&pdcnn_mem->model_mem);
	// open
	vendor_ai_net_open(p_net->proc_id);
	
	return ret;
}
static HD_RESULT network_close(NET_PROC *p_net)
{
	HD_RESULT ret = HD_OK;
	// close
	ret = vendor_ai_net_close(p_net->proc_id);
	
	mem_exit();

	return ret;
}

//////////////////////////////////////////////////////////////////////////////
//get layer info
static HD_RESULT network_get_layer0_info(UINT32 proc_id)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF p_inbuf = {0};
	VENDOR_AI_BUF p_outbuf = {0};
		
	// get layer0 in buf
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_IN(0, 0), &p_inbuf);
	if (HD_OK != ret) {
		printf("proc_id(%u) get layer0 inbuf fail !!\n", proc_id);
		return ret;
	}
	
	// get layer0 in buf
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(0, 0), &p_outbuf);
	if (HD_OK != ret) {
		printf("proc_id(%u) get layer0 outbuf fail !!\n", proc_id);
		return ret;
	}
	
	printf("dump layer0 info:\n");
	printf("   channel(%lu)\n", p_inbuf.channel);
	printf("   fmt(0x%lx)\n", p_inbuf.fmt);
	printf("   width(%lu)\n", p_outbuf.width);
	printf("   height(%lu)\n", p_outbuf.height);
	printf("   channel(%lu)\n", p_outbuf.channel);
	printf("   batch_num(%lu)\n", p_outbuf.batch_num);
	printf("   fmt(0x%lx)\n", p_outbuf.fmt);
	printf("\n");

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
#if(!AI_IPC)

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}
static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
#if SENSOR_CHOICE==SENSOR_291
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#elif SENSOR_CHOICE==SENSOR_S02K
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
#endif
}


static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
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
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.isp_id = 0;
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
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

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = VDO_FRAME_FORMAT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);

		video_out_param.depth = 1;	// set > 0 to allow pull out (nn)

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}


///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type, HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	printf("out_type=%d\r\n", out_type);

	#if 1
	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	if (out_type != 1) {
		printf("520 only support LCD\r\n");
	}
	#else
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
	#endif
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);

	*p_video_out_ctrl=video_out_ctrl ;
	return ret;
}

static HD_RESULT get_out_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##devcount %d\r\n", video_out_dev.max_dev_count);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	video_out_param.dim.w = p_dim->w;
	video_out_param.dim.h = p_dim->h;
	video_out_param.pxlfmt = VDO_FRAME_FORMAT;
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);

	return ret;
}
#endif
///////////////////////////////////////////////////////////////////////////////
static HD_RESULT init_module(void)
{
	HD_RESULT ret;
#if(!AI_IPC)

	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
#else
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;

#endif
	return HD_OK;
}
#if(!AI_IPC)
static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;

	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config for nn
	ret = set_proc_cfg(&p_stream->proc_alg_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg alg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, 1, p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, SOURCE_PATH, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;
#if NN_PDCNN_DRAW
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_0, &p_stream->mask_path0)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_1, &p_stream->mask_path1)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_2, &p_stream->mask_path2)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_3, &p_stream->mask_path3)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_4, &p_stream->mask_path4)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_5, &p_stream->mask_path5)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_6, &p_stream->mask_path6)) != HD_OK)
		return ret;
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_7, &p_stream->mask_path7)) != HD_OK)
		return ret;
#endif

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
#if NN_PDCNN_DRAW
    if ((ret = hd_videoout_close(p_stream->mask_path0)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path1)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path2)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path3)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path4)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path5)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path6)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path7)) != HD_OK)
		return ret;
#endif

	return HD_OK;
}
#if NN_PDCNN_DRAW
static HD_RESULT set_proc_param_extend(HD_PATH_ID video_proc_path, HD_PATH_ID src_path, HD_DIM* p_dim, UINT32 dir)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = VDO_FRAME_FORMAT;
		video_out_param.dir = dir;
		video_out_param.depth = 1; //set 1 to allow pull

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	}

	return ret;
}

static HD_RESULT open_module_extend(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videoout config
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module_extend(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}
#endif
#endif
static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
#if(!AI_IPC)

	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
#else
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

#if NN_PDCNN_DRAW
int init_mask_param(HD_PATH_ID mask_path)
{
	HD_OSG_MASK_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    // ghost target
    attr.position[0].x = 1;
    attr.position[0].y = 1;
    attr.position[1].x = 9;
    attr.position[1].y = 1;
    attr.position[2].x = 9;
    attr.position[2].y = 9;
    attr.position[3].x = 1;
    attr.position[3].y = 9;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 0;
    attr.color         = 0x00FF0000;
    attr.thickness     = 0;

	return hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);
}

int pdcnn_mask_draw(HD_PATH_ID mask_path, PDCNN_RESULT p_obj, BOOL bdraw, UINT32 color, FLOAT* ratio)
{
    HD_OSG_MASK_ATTR attr;
    HD_RESULT ret = HD_OK;

    memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    if (!bdraw) {
        return init_mask_param(mask_path);
    }

    attr.position[0].x = (UINT32)(p_obj.x1 * ratio[0]);
    attr.position[0].y = (UINT32)(p_obj.y1 * ratio[1]);
    attr.position[1].x = (UINT32)(p_obj.x2 * ratio[0]);
    attr.position[1].y = (UINT32)(p_obj.y1 * ratio[1]);
    attr.position[2].x = (UINT32)(p_obj.x2 * ratio[0]);
    attr.position[2].y = (UINT32)(p_obj.y2 * ratio[1]);
    attr.position[3].x = (UINT32)(p_obj.x1 * ratio[0]);
    attr.position[3].y = (UINT32)(p_obj.y2 * ratio[1]);
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 255;
    attr.color         = color;
    attr.thickness     = 2;

    ret = hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);

    return ret;
}

int pdcnn_draw_info(PDCNN_MEM pdcnn_mem, VIDEO_LIVEVIEW *p_stream, FLOAT* ratio)
{
	PDCNN_RESULT* pdcnn_results = (PDCNN_RESULT*)pdcnn_mem.final_result.va;
	#if(!AI_IPC)
    pdcnn_mask_draw(p_stream->mask_path0, pdcnn_results[0], (BOOL)(final_pdobj_num >= 1), 0x0000FF00, ratio);
    pdcnn_mask_draw(p_stream->mask_path1, pdcnn_results[1], (BOOL)(final_pdobj_num >= 2), 0x0000FF00, ratio);
    pdcnn_mask_draw(p_stream->mask_path2, pdcnn_results[2], (BOOL)(final_pdobj_num >= 3), 0x0000FF00, ratio);
    pdcnn_mask_draw(p_stream->mask_path3, pdcnn_results[3], (BOOL)(final_pdobj_num >= 4), 0x0000FF00, ratio);
	pdcnn_mask_draw(p_stream->mask_path4, pdcnn_results[4], (BOOL)(final_pdobj_num >= 5), 0x0000FF00, ratio);
    pdcnn_mask_draw(p_stream->mask_path5, pdcnn_results[5], (BOOL)(final_pdobj_num >= 6), 0x0000FF00, ratio);
    pdcnn_mask_draw(p_stream->mask_path6, pdcnn_results[6], (BOOL)(final_pdobj_num >= 7), 0x0000FF00, ratio);
    pdcnn_mask_draw(p_stream->mask_path7, pdcnn_results[7], (BOOL)(final_pdobj_num >= 8), 0x0000FF00, ratio);
	#else
	{
		PD_SHM_INFO  *p_pd_shm = (PD_SHM_INFO  *)g_shm;
		PD_SHM_RESULT *p_obj;
		UINT32         i;

		if (p_pd_shm->exit) {
			return -1;
		}

		// update pdcnn result to share memory
		p_pd_shm->pd_num = final_pdobj_num;
		if(final_pdobj_num > 10){
			p_pd_shm->pd_num = 10;
		}
		for (i = 0; i < p_pd_shm->pd_num; i++) {
			p_obj = &p_pd_shm->pd_results[i];
			p_obj->category = pdcnn_results[i].category;
			p_obj->score = pdcnn_results[i].score;
			p_obj->x1 = pdcnn_results[i].x1 * sensor_ratio[0];
			p_obj->x2 = pdcnn_results[i].x2 * sensor_ratio[0];
			p_obj->y1 = pdcnn_results[i].y1 * sensor_ratio[1];
			p_obj->y2 = pdcnn_results[i].y2 * sensor_ratio[1];
		}
	}
	#endif
    return HD_OK;
}

VOID *draw_thread(VOID *arg)
{
    THREAD_DRAW_PARM *p_draw_parm = (THREAD_DRAW_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_draw_parm->stream;
    PDCNN_MEM pdcnn_mem = p_draw_parm->pd_mem;
	FLOAT* ratio = p_draw_parm->ratio_lcd;

    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

    // wait fd ro pvd init ready
    sleep(2);

    do {
        if (is_net_run) {
			if(fps_delay > 30000){
				usleep(fps_delay - 30000); //300000, 200000, 100000
			}else{
				usleep(fps_delay);
			}
			ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		    if(ret != HD_OK)
		    {
		        printf("ERR : draw_thread hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
		        break;
		    }

		    if (pdcnn_draw_info(pdcnn_mem, &stream, ratio) < 0) {
				// exit
				is_net_proc = FALSE;
	            is_net_run = FALSE;
		    }

		    ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
		    if(ret != HD_OK)
		    {
		        printf("ERR : draw_thread hd_videoproc_release_out_buf fail (%d)\n\r", ret);
		        break;
		    }
    	}
	} while(is_net_proc);

    return 0;
}
#endif

static VOID *pd_thread_api(VOID *arg)
{
#if NN_PDCNN_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif

	HD_RESULT ret;
	VENDOR_AIS_PDCNN_PARM* pdcnn_parm = (VENDOR_AIS_PDCNN_PARM*)arg;
	BOOL need_input_init = FALSE;
	VENDOR_AI_BUF src_img = {0};

	HD_VIDEO_FRAME video_frame = {0};

	PDCNN_MEM pdcnn_mem = pdcnn_parm->pdcnn_mem;
	VIDEO_LIVEVIEW stream = pdcnn_parm->stream;
	NET_PROC net_info = pdcnn_parm->net_info;

	// start network
	ret = vendor_ai_net_start(net_info.proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%u) nn start fail !!\n", net_info.proc_id);
		goto exit_thread;
	}

	//get nn layer 0 info
	network_get_layer0_info(net_info.proc_id);

	FLOAT score_thr = 0.45, nms_thr = 0.1;
	CHAR para_file[] = "/mnt/sd/CNNLib/para/pdcnn_v4_ll/para.txt";
	PROPOSAL_PARAM proposal_params;
	LIMIT_PARAM limit_param;
	proposal_params.ratiow = (FLOAT)VDO_SIZE_W / (FLOAT)YUV_WIDTH;
	proposal_params.ratioh = (FLOAT)VDO_SIZE_H / (FLOAT)YUV_HEIGHT;
	proposal_params.score_thres = score_thr;
	proposal_params.nms_thres = nms_thr;
	proposal_params.run_id = net_info.proc_id;
#if MAX_DISTANCE_MODE
	limit_param.max_distance = 1;
	limit_param.sm_thr_num = 2;
#else
	limit_param.max_distance = 0;
	limit_param.sm_thr_num = 6;
#endif
#if (SENSOR_4M && DBG_OBJ_INFOR)
	FLOAT ratiow_4m = (FLOAT)2560 / (FLOAT)YUV_WIDTH;
	FLOAT ratioh_4m = (FLOAT)1440 / (FLOAT)YUV_HEIGHT;
#endif

	BACKBONE_OUT* backbone_outputs = (BACKBONE_OUT*)pdcnn_mem.backbone_output.va;

	ret = pdcnn_init(&proposal_params, backbone_outputs, &limit_param, para_file);
	if(ret != HD_OK){
		printf("ERR: pdcnn init fail!\r\n");
		goto exit_thread;
	}

	UINT32 ai_pd_frame = 0;
	do {
        if (is_net_run) {
			ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("ERR : pd_thread hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto exit_thread;
			}
#if NN_PDCNN_PROF
            gettimeofday(&tstart0, NULL);
#endif
			if(!need_input_init){
				src_img.width   	= video_frame.dim.w;
				src_img.height  	= video_frame.dim.h;
				src_img.channel 	= 2;
				src_img.line_ofs	= video_frame.loff[0];
				src_img.fmt 		= video_frame.pxlfmt;
				src_img.pa			= video_frame.phy_addr[0];
				src_img.va			= (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], video_frame.pw[0]*video_frame.ph[0]*3/2);
				src_img.sign        = MAKEFOURCC('A','B','U','F');
				src_img.size        = video_frame.loff[0]*video_frame.dim.h*3/2;

				sensor_ratio[0] = (FLOAT)video_frame.dim.w / (FLOAT)YUV_WIDTH;
				sensor_ratio[1] = (FLOAT)video_frame.dim.h / (FLOAT)YUV_HEIGHT;
				need_input_init = TRUE;
			}

			ret = pdcnn_process(&proposal_params, &pdcnn_mem, &limit_param, backbone_outputs, &src_img, (UINT32)MAX_DISTANCE_MODE);

			if(ret != HD_OK){
				printf("pdcnn_process fail!\r\n");
				goto exit_thread;
			}		

			//person detection output results;
			final_pdobj_num = pdcnn_mem.out_num;
#if NN_PDCNN_PROF
            gettimeofday(&tend0, NULL);
            cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
            sum_time0 += cur_time0;
            mean_time0 = sum_time0 / (++icount);
            printf("[PD] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
#endif
			if(save_results){
				PDCNN_RESULT *final_results = (PDCNN_RESULT*)pdcnn_mem.final_result.va;
				INT32 obj_num = pdcnn_mem.out_num;
				if (obj_num > 0) {
					//gettimeofday(&tstart1, NULL);
					CHAR TXT_FILE[256], YUV_FILE[256];
					FILE *fs, *fb;
					sprintf(TXT_FILE, "/mnt/sd/det_results/PD/txt/%09ld.txt", ai_pd_frame);
					sprintf(YUV_FILE, "/mnt/sd/det_results/PD/yuv/%09ld.bin", ai_pd_frame);
					fs = fopen(TXT_FILE, "w+");
					fb = fopen(YUV_FILE, "wb+");
					fwrite((UINT32 *)src_img.va, sizeof(UINT32), (src_img.width * src_img.height * 3 / 2), fb);
					fclose(fb);
					if(obj_num > 10){
						obj_num = 10;
					}
					for(INT32 num = 0; num < obj_num; num++){
						INT32 xmin = (INT32)(final_results[num].x1 * sensor_ratio[0]);
						INT32 ymin = (INT32)(final_results[num].y1 * sensor_ratio[1]);
						INT32 width = (INT32)(final_results[num].x2 * sensor_ratio[0] - xmin);
						INT32 height = (INT32)(final_results[num].y2 * sensor_ratio[1] - ymin);
						FLOAT score = final_results[num].score;
						fprintf(fs, "%f %d %d %d %d\r\n", score, xmin, ymin, width, height);
					}
					fclose(fs);
					ai_pd_frame++;
				}
			}


#if DBG_OBJ_INFOR
			//if(proposal_params.open_pdout){
	#if SENSOR_4M
			print_pdcnn_results(&pdcnn_mem, ratiow_4m, ratioh_4m);
	#else
			print_pdcnn_results(&pdcnn_mem, proposal_params.ratiow, proposal_params.ratioh);
	#endif
			//}
#endif
            //printf("fps_delay %d cur_time0 %lld \r\n",fps_delay,cur_time0);
            gettimeofday(&tend0, NULL);
			cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
            if ((fps_delay)&&(cur_time0 < fps_delay)) {
                usleep(fps_delay - cur_time0 + (mean_time0*0));
            }

			ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
		    if(ret != HD_OK) {
				printf("ERR : pd_thread hd_videoproc_release_out_buf fail (%d)\n\r", ret);
				goto exit_thread;
			}

        }
	} while(is_net_proc);
	ret = hd_common_mem_munmap((void *)src_img.va, video_frame.pw[0]*video_frame.ph[0]*3/2);
	if (ret != HD_OK) {
		printf("pd_thread_api : (src_img.va)hd_common_mem_munmap fail\r\n");
		goto exit_thread;
	}

exit_thread:

	ret = vendor_ai_net_stop(net_info.proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%u) nn stop fail !!\n", net_info.proc_id);
	}


	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	UINT32 proc_id = 0;
	UINT32 req_mem_size = 0;
	INT key;

	PDCNN_MEM pdcnn_mem;
	VENDOR_AIS_PDCNN_PARM pd_parm;
	VIDEO_LIVEVIEW stream[2] = {0};
#if NN_PDCNN_DRAW
	THREAD_DRAW_PARM drawpd_parm;
	pthread_t draw_thread_id;
#endif
	pthread_t pd_thread_id;

	int    start_run = 0;
	UINT32 cnn_id = 0;

    fps_delay = 0 ;
	save_results = 0;
	if (argc >= 2) {
		UINT32 fps = atoi(argv[1]);
		if(2 == MAX_DISTANCE_MODE){
			fps = 7;
		}else if(3 == MAX_DISTANCE_MODE){
			fps = 4;
		}
        if((fps==0)||(fps>30)) {
            fps = 30;
        }
  		fps_delay = (UINT32)(1000000/fps);
		printf("fps %d fps_delay %d\r\n", fps,fps_delay);
	}
	if (argc >= 3) {
		start_run =  atoi(argv[2]);
	}
	if (argc >= 4) {
		save_results =  atoi(argv[3]);
	}

	// init hdal
#if(!AI_IPC)
	UINT32 out_type = 1;
	ret = hd_common_init(0);
#else
	ret = hd_common_init(2);
#endif
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	//set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine
#if(AI_IPC)
	init_share_memory();
#endif
	//init gfx for img
	ret = hd_gfx_init();
	if (ret != HD_OK) {
		printf("hd_gfx_init fail\r\n");
		goto exit;
	}
	//network params
	NET_PROC net_info;
	net_info.proc_id = proc_id;
	sprintf(net_info.model_filename, model_name[0]);
	net_info.binsize		= _getsize_model(net_info.model_filename);
	net_info.job_method     = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
	//net_info.job_method     = VENDOR_AI_NET_JOB_OPT_GRAPH_O1;
	net_info.job_wait_ms    = 0;
	net_info.buf_method     = VENDOR_AI_NET_BUF_OPT_NONE;
	//net_info.buf_method     = VENDOR_AI_NET_BUF_OPT_SHRINK;
	if (net_info.binsize <= 0) {
		printf("WRN: PDCNN->proc_id(%u) input model is not exist!!!!!!!!!!\r\n", proc_id);
		goto exit;
	}
	printf("proc_id(%u) set net_info: model-file(%s), binsize=%d\r\n", proc_id, net_info.model_filename, net_info.binsize);

	//ai cfg set
	UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
	vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
	vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		goto exit;
	}
	//set buf option

	VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
	cfg_buf_opt.method = net_info.buf_method;
	cfg_buf_opt.ddr_id = DDR_ID0;
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	
	// set job option
	VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
	cfg_job_opt.method = net_info.job_method;
	cfg_job_opt.wait_ms = net_info.job_wait_ms;
	cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);

	//PDCNN mem set
	MEM_PARM allbuf = {0};
	ret = mem_init(proc_id, &allbuf);
	if(ret != HD_OK){
		printf("pdcnn all mem get fail (%d)!!\r\n", ret);
		goto exit;
	}
	rel_buf = allbuf;
	ret = get_pd_mem(&allbuf, &(pdcnn_mem.model_mem), net_info.binsize, 32);
	if(ret != HD_OK){
		printf("get model mem fail (%d)!!\r\n", ret);
		goto exit;
	}

	UINT32 modelsize = _load_model(net_info.model_filename, &(pdcnn_mem.model_mem));
	if (modelsize <= 0) {
		printf("proc_id(%u) input model load fail: %s\r\n", net_info.proc_id, net_info.model_filename);
		return 0;
	}
	pdcnn_get_version();
#if 1
	ret = pdcnn_version_check(&(pdcnn_mem.model_mem));
	if(ret != HD_OK){
		printf("pdcnn version check fail (%d)!!\r\n", ret);
		goto exit;
	}
#endif
	// open network(model, get network mem)
	if ((ret = network_open(&net_info, &pdcnn_mem)) != HD_OK) {
		printf("proc_id(%u  nn open fail !!\n", net_info.proc_id);
		goto exit;
	}
	
	//set work buf and assign pdcnn mem
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		goto exit;
	}
	printf("work buf size: %ld\r\n", wbuf.size);

	ret = get_pd_mem(&allbuf, &(pdcnn_mem.io_mem), wbuf.size, 32);
	if(ret != HD_OK){
		printf("get io_mem fail (%d)!!\r\n", ret);
		goto exit;
	}
#if MAX_DISTANCE_MODE
	/*get roi buf*/
	ret = get_pd_mem(&allbuf, &(pdcnn_mem.scale_buf), (YUV_WIDTH * YUV_HEIGHT * 3 / 2), 32);
	if(ret != HD_OK){
		printf("get scale_buf fail (%d)!!\r\n", ret);
		goto exit;
	}
#endif
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &(pdcnn_mem.io_mem));
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}
	//get pdcnn postproc mem
	ret = get_post_mem(&allbuf, &pdcnn_mem);
	if(ret != HD_OK){
		printf("get pdcnn postprocess mem fail (%d)!!\r\n", ret);
		goto exit;
	}

	req_mem_size = AI_PD_BUF_SIZE - allbuf.size;
	printf("AI turnkey allocate total of buf size: (%ld), use total of buf size: (%ld)!\r\n", AI_PD_BUF_SIZE, req_mem_size);
	
	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
#if(!AI_IPC)
	// open video_liveview modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("stream[0] open fail=%d\n", ret);
		goto exit;
	}

	stream[1].proc_max_dim.w = VDO_SIZE_W;
	stream[1].proc_max_dim.h = VDO_SIZE_H;
	ret = open_module_extend(&stream[1], &stream[1].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("stream[1] open fail=%d\n", ret);
		goto exit;
	}
#if NN_PDCNN_DRAW

	if(init_mask_param(stream[0].mask_path0)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
	if(init_mask_param(stream[0].mask_path1)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path2)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path3)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
	if(init_mask_param(stream[0].mask_path4)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
	if(init_mask_param(stream[0].mask_path5)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path6)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path7)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}

	ret = hd_videoout_start(stream[0].mask_path0);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path1);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path2);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path3);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path4);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path5);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path6);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path7);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
#endif
	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	// set videocap parameter
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (alg)
	stream[0].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[0].proc_alg_max_dim.h = VDO_SIZE_H;
	ret = set_proc_param(stream[0].proc_alg_path, &stream[0].proc_alg_max_dim);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}

	stream[1].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[1].proc_alg_max_dim.h = VDO_SIZE_H;
	ret = set_proc_param_extend(stream[1].proc_alg_path, SOURCE_PATH, &stream[1].proc_alg_max_dim, HD_VIDEO_DIR_NONE);
	if (ret != HD_OK) {
		printf("stream[1] set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (main)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
#else
	if (cnn_id == 0) {
		stream[0].proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(6));
	} else {
		stream[0].proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(7));
	}
	stream[1].proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(6));
#endif
	// nn S ------------------------------------------------
	#if NN_USE_DSP
	ret = dsp_init();
	if (ret != HD_OK) {
		DBG_ERR("dsp_init fail=%d\n", ret);
		goto exit;
	}
	#endif

#if NN_PDCNN_DRAW
	pd_parm.stream = stream[1];
#else
	pd_parm.stream = stream[0];
#endif
	pd_parm.pdcnn_mem = pdcnn_mem;
	pd_parm.net_info = net_info;

#if(!AI_IPC)
	// start video_liveview modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	hd_videoproc_start(stream[0].proc_alg_path);
#if NN_PDCNN_DRAW
	hd_videoproc_start(stream[1].proc_alg_path);
#endif
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(stream[0].out_path);
#endif

	ret = pthread_create(&pd_thread_id, NULL, pd_thread_api, (VOID*)(&pd_parm));
	if (ret < 0) {
		printf("create pd_thread encode thread failed");
		goto exit;
	}
#if NN_PDCNN_DRAW
	drawpd_parm.pd_mem = pd_parm.pdcnn_mem;
	drawpd_parm.stream = stream[0];
	drawpd_parm.ratio_lcd[0] = (FLOAT)OSG_LCD_WIDTH / (FLOAT)YUV_WIDTH;
	drawpd_parm.ratio_lcd[1] = (FLOAT)OSG_LCD_HEIGHT / (FLOAT)YUV_HEIGHT;
#endif
#if NN_PDCNN_DRAW
	ret = pthread_create(&draw_thread_id, NULL, draw_thread, (VOID*)(&drawpd_parm));
	if (ret < 0) {
		printf("create draw_thread encode thread failed");
		goto exit;
	}
#endif

#if(AI_IPC)
	//  run network directly
	is_net_proc = TRUE;
    is_net_run = TRUE;
#endif

	if (!start_run) {
		// query user key
		do {
			printf("usage:\n");
			printf("  enter q: exit\n");
			printf("  enter r: run engine\n");
			key = getchar();
			if (key == 'q' || key == 0x3) {
				is_net_proc = FALSE;
	            is_net_run = FALSE;
				break;
			} else if (key == 'r') {
				//  run network
				is_net_proc = TRUE;
	            is_net_run = TRUE;
				printf("[ai sample] write result done!\n");
				continue;
			}

		} while(1);
	}

	pthread_join(pd_thread_id, NULL);
#if NN_PDCNN_DRAW
	pthread_join(draw_thread_id, NULL);
#endif
#if(!AI_IPC)
	// stop video_liveview modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoproc_stop(stream[0].proc_alg_path);
#if NN_PDCNN_DRAW
	hd_videoproc_stop(stream[1].proc_alg_path);
#endif
	hd_videoout_stop(stream[0].out_path);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
#endif

exit:
	#if NN_USE_DSP
	ret = dsp_uninit();
	if (ret != HD_OK) {
		DBG_ERR("dsp_uninit fail=%d\n", ret);
	}
	#endif

#if(!AI_IPC)
	// close video_liveview modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	ret = close_module_extend(&stream[1]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
#endif
	// unint hd_gfx
	ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("hd_gfx_uninit fail\r\n");
	}
	if(net_info.binsize > 0){
		// close network modules
		if ((ret = network_close(&net_info)) != HD_OK) {
			printf("proc_id(%u) nn close fail !!\n", proc_id);
		}
		// uninit all modules
		ret = exit_module();
		if (ret != HD_OK) {
			printf("exit fail=%d\n", ret);
		}
		// uninit network modules
		ret = vendor_ai_uninit();
		if (ret != HD_OK) {
			printf("vendor_ai_uninit fail=%d\n", ret);
		}
	}
	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	#if(AI_IPC)
	exit_share_memory();
	#endif

	return 0;
}
