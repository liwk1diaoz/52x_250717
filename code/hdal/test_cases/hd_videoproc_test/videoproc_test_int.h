/**
	@brief Sample code of video snapshot from proc to frame.\n

	@file video_process_only.c

	@author Jeah Yen

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

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#else
#include <FreeRTOS_POSIX.h>	
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_PROCESS {

	char snap_file[128];

	// (1) user push
	pthread_t  feed_thread_id;
	HD_DIM push_dim;
	HD_VIDEO_PXLFMT push_pxlfmt;
	UINT32	push_start;
	UINT32	push_exit;

	HD_DIM  proc_max_dim;

	// (1)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;
	HD_PATH_ID proc_src_path;

	HD_DIM  proc_dim;

	// (4) user pull
	pthread_t  aquire_thread_id;
	HD_DIM pull_dim;
	HD_VIDEO_PXLFMT pull_pxlfmt;
	UINT32	pull_start;
	UINT32	pull_exit;
	UINT32 	do_snap;
	UINT32 	shot_count;
	INT32   wait_ms;
	
	UINT32 	show_ret;
	
	/////////////////////////////////////

	//state (stage test)
	UINT32 ctrl_flow; // [normal] ... init/open/start/stop/close/uninit
	UINT32 set_flow; // [open config] [ready change] [runtime change]
	UINT32 bind_in_flow;  // [bind] or [push]
	UINT32 bind_out_flow; // [bind] or [pull]

	//device (config test)
    UINT32 dev_id;
	//path (config test)
	UINT32 in_id;
	UINT32 out_id;
	//control param (config test)
	UINT32 pipe; //HD_VIDEOPROC_PIPE_RAWALL or HD_VIDEOPROC_PIPE_SCALE

	//control param (func test)
	UINT32 func; //3DNR/SHDR/WDR/DEFOG/COLORNR
	UINT32 ref_path_3dnr;

	//input param (func test)
	UINT32 input_pxlfmt; //0:RAW, 1:YUV
	UINT32 input_dim;
	UINT32 input_crop;
	UINT32 input_dir;
	UINT32 input_frc;
	//output param (func test)
	UINT32 output_pxlfmt;
	UINT32 output_dim;
	UINT32 output_dir;
	UINT32 output_crop;
	UINT32 output_frc;

	//input speed (performance test)
	UINT32 input_fps;
	//output speed (performance test)
	UINT32 output_fps;

	//input data (data test)
	UINT32 data_sign;
	UINT32 data_pxlfmt;
	UINT32 data_dim;
	UINT32 data_loff;
	UINT32 data_addr;
	UINT32 data_timestamp;
	UINT32 data_blk;

} VIDEO_PROCESS;

///////////////////////////////////////////////////////////////////////////////

INT32 push_query_mem(VIDEO_PROCESS *p_stream, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, INT32 i);
HD_RESULT push_init(void);
HD_RESULT push_set_config(VIDEO_PROCESS *p_stream, void* p_cfg);
HD_RESULT push_open(VIDEO_PROCESS *p_stream);
HD_RESULT push_set_out(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_IN* p_out); //send input condition
HD_RESULT push_start(VIDEO_PROCESS *p_stream);
HD_RESULT push_stop(VIDEO_PROCESS *p_stream);
HD_RESULT push_close(VIDEO_PROCESS *p_stream);
HD_RESULT push_uninit(void);

INT32 pull_query_mem(VIDEO_PROCESS *p_stream, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, INT32 i);
HD_RESULT pull_init(void);
HD_RESULT pull_set_config(VIDEO_PROCESS *p_stream, void* p_cfg);
HD_RESULT pull_open(VIDEO_PROCESS *p_stream);
HD_RESULT pull_set_in(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_OUT* p_in); //verify output result
HD_RESULT pull_start(VIDEO_PROCESS *p_stream);
HD_RESULT pull_stop(VIDEO_PROCESS *p_stream);
HD_RESULT pull_close(VIDEO_PROCESS *p_stream);
HD_RESULT pull_uninit(void);
HD_RESULT pull_snapshot(VIDEO_PROCESS *p_stream,  const char* name); //verify output result

INT32 proc_query_mem(VIDEO_PROCESS *p_stream, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, INT32 i);
HD_RESULT proc_init(void);
HD_RESULT proc_set_config(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_DEV_CONFIG* p_cfg);
HD_RESULT proc_open(VIDEO_PROCESS *p_stream);
HD_RESULT proc_start(VIDEO_PROCESS *p_stream);
HD_RESULT proc_set_in(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_IN* p_in);
HD_RESULT proc_set_out(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_OUT* p_out);
HD_RESULT proc_set_in_crop(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_CROP* p_crop);
HD_RESULT proc_set_out_crop(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_CROP* p_crop);
HD_RESULT proc_set_in_frc(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_FRC* p_frc);
HD_RESULT proc_set_out_frc(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_FRC* p_frc);
HD_RESULT proc_stop(VIDEO_PROCESS *p_stream);
HD_RESULT proc_close(VIDEO_PROCESS *p_stream);
HD_RESULT proc_uninit(void);

HD_RESULT bindsrc_init(void);
HD_RESULT bindsrc_open(VIDEO_PROCESS *p_stream);
HD_RESULT bindsrc_start(VIDEO_PROCESS *p_stream);
HD_RESULT bindsrc_stop(VIDEO_PROCESS *p_stream);
HD_RESULT bindsrc_close(VIDEO_PROCESS *p_stream);
HD_RESULT bindsrc_unint(void);

HD_RESULT binddst_init(void);
HD_RESULT binddst_open(VIDEO_PROCESS *p_stream);
HD_RESULT binddst_start(VIDEO_PROCESS *p_stream);
HD_RESULT binddst_stop(VIDEO_PROCESS *p_stream);
HD_RESULT binddst_close(VIDEO_PROCESS *p_stream);
HD_RESULT binddst_uninit(void);


///////////////////////////////////////////////////////////////////////////////

