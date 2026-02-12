/**
	@brief Sample code of audio output.\n

	@file audio_output_only.c

	@author HM Tseng

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>

#if defined(__LINUX)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

#define EXT_CODEC 1
#if EXT_CODEC
#include "vendor_audiocapture.h"
#include "vendor_audioout.h"
#endif

#define DEBUG_MENU 1

#if EXT_CODEC
#define CHKPNT    printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)   printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)   printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#endif
///////////////////////////////////////////////////////////////////////////////

#define BITSTREAM_SIZE      12800
#define FRAME_SAMPLES       1024
#define AUD_BUFFER_CNT      5

#define TIME_DIFF(new_val, old_val)     ((int)(new_val) - (int)(old_val))

///////////////////////////////////////////////////////////////////////////////
static int usr_sampling_rate=0, usr_mode=0, vol=80, drv_mono=0, drv_out=0;
static char usr_file_path_main[64]={0};
static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	/* dummy buffer, not for audio module */
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	/* user buffer for bs pushing in */
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[1].blk_size = 0x100000;
	mem_cfg.pool_info[1].blk_cnt = 1;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return -1;
	}
	return 0;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_audio_out_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_PATH_ID audio_out_ctrl = 0;
#if EXT_CODEC
	VENDOR_AUDIOOUT_INIT_CFG vendor_config = {0};
#endif
	ret = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &audio_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/* set audio out maximum parameters */
	audio_cfg_param.out_max.sample_rate = usr_sampling_rate;
	audio_cfg_param.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.out_max.mode = usr_mode;
	audio_cfg_param.frame_sample_max = FRAME_SAMPLES;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.in_max.sample_rate = usr_sampling_rate;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/* set audio out driver parameters */
	audio_driver_cfg_param.mono = drv_mono;
	audio_driver_cfg_param.output = drv_out;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
#if EXT_CODEC
	if (ret != HD_OK) {
		return ret;
	}

	if(drv_out == HD_AUDIOOUT_OUTPUT_I2S){
		printf("322222222\r\n");
		snprintf(vendor_config.driver_name, VENDOR_AUDIOOUT_NAME_LEN-1, "nvt_aud_emu");
		vendor_config.aud_init_cfg.pin_cfg.pinmux.audio_pinmux  = 0x11; //PIN_AUDIO_CFG_I2S | PIN_AUDIO_CFG_MCLK
		vendor_config.aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux = 0;//0x1000; //PIN_I2C_CFG_CH4
		vendor_config.aud_init_cfg.i2s_cfg.bit_clk_ratio = 32;
		vendor_config.aud_init_cfg.i2s_cfg.bit_width     = 16;
		vendor_config.aud_init_cfg.i2s_cfg.tdm_ch        = 2;
		vendor_config.aud_init_cfg.i2s_cfg.op_mode       = 1;
		vendor_audioout_set(audio_out_ctrl, VENDOR_AUDIOOUT_ITEM_EXT, (VOID *)&vendor_config);
	}
#endif
	*p_audio_out_ctrl = audio_out_ctrl;

	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret;
	HD_AUDIOOUT_OUT audio_out_out_param = {0};
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};
	HD_AUDIOOUT_IN audio_out_in_param = {0};

	// set hd_audioout output parameters
	audio_out_out_param.sample_rate = usr_sampling_rate;
	audio_out_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_out_param.mode = usr_mode;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_OUT, &audio_out_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audioout volume
	audio_out_vol.volume = vol;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);
	if (ret != HD_OK) {
		return ret;
	}


	// set hd_audioout input parameters
	audio_out_in_param.sample_rate = usr_sampling_rate;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_IN, &audio_out_in_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_OUTONLY {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	UINT32 out_exit;
	UINT32 out_pause;
} AUDIO_OUTONLY;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audioout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(AUDIO_OUTONLY *p_outonly)
{
	HD_RESULT ret;
	ret = set_out_cfg(&p_outonly->out_ctrl, p_outonly->sample_rate_max);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_outonly->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(AUDIO_OUTONLY *p_outonly)
{
	HD_RESULT ret;
	if((ret = hd_audioout_close(p_outonly->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audioout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static void *playback_thread(void *arg)
{
	INT ret, bs_size, result;
	CHAR filename[64],filename_len[64];
	FILE *bs_fd, *len_fd;
	HD_AUDIO_FRAME  bs_in_buf = {0};
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 pa, va;
	UINT32 blk_size = 0x100000;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 bs_buf_start, bs_buf_curr, bs_buf_end;
	INT au_frame_ms, elapse_time, au_buf_time, timestamp;
	UINT start_time, data_time;
	AUDIO_OUTONLY *p_out_only = (AUDIO_OUTONLY *)arg;

	/* read test pattern */
	snprintf(filename, sizeof(filename), "%s", usr_file_path_main);
	bs_fd = fopen(filename, "rb");
	if (bs_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		return 0;
	}
	printf("play file: [%s]\n", filename);

	snprintf(filename_len, sizeof(filename_len), "%s.len", usr_file_path_main);
	len_fd = fopen(filename_len, "rb");
	if (len_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename_len);
		goto play_fclose;
	}
	printf("len file: [%s]\n", filename_len);

	au_frame_ms = FRAME_SAMPLES * 1000 / p_out_only->sample_rate - 5; // the time(in ms) of each audio frame
	start_time = hd_gettime_ms();
	data_time = 0;

	/* get memory */
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail, blk = 0x%x\n", blk);
		goto play_fclose;
	}
	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		printf("blk2pa fail, blk(0x%x)\n", blk);
		goto rel_blk;
	}
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
		if (va == 0) {
			printf("get va fail, va(0x%x)\n", blk);
			goto rel_blk;
		}
		/* allocate bs buf */
		bs_buf_start = va;
		bs_buf_curr = bs_buf_start;
		bs_buf_end = bs_buf_start + blk_size;
		printf("alloc bs_buf: start(0x%x) curr(0x%x) end(0x%x) size(0x%x)\n", bs_buf_start, bs_buf_curr, bs_buf_end, blk_size);
	}
	system("mem w f0630004 0x00200381");
	while (1) {
retry:
		if (p_out_only->out_exit == 1) {
			break;
		}

		if (p_out_only->out_pause == 1) {
			usleep(10000);
			goto retry;
		}

		elapse_time = TIME_DIFF(hd_gettime_ms(), start_time);
		au_buf_time = data_time - elapse_time;
 		if (au_buf_time > AUD_BUFFER_CNT * au_frame_ms) {
			au_buf_time=0;
			//usleep(au_frame_ms);
			//goto retry;
		}

		/* get bs size */
		if (fscanf(len_fd, "%d %d\n", &bs_size, &timestamp) == EOF) {
			// reach EOF, read from the beginning
			p_out_only->out_exit = 1;
			fseek(bs_fd, 0, SEEK_SET);
			fseek(len_fd, 0, SEEK_SET);
			if (fscanf(len_fd, "%d %d\n", &bs_size, &timestamp) == EOF) {
				printf("[ERROR] fscanf error\n");
				continue;
			}
		}
		if (bs_size == 0 || bs_size > BITSTREAM_SIZE) {
			printf("Invalid bs_size(%d)\n", bs_size);
			continue;
		}

		/* check bs buf rollback */
		if ((bs_buf_curr + bs_size) > bs_buf_end) {
			bs_buf_curr = bs_buf_start;
		}

		/* read bs from file */
		result = fread((void *)bs_buf_curr, 1, bs_size, bs_fd);
		if (result != bs_size) {
			printf("reading error\n");
			continue;
		}
		bs_in_buf.sign = MAKEFOURCC('A','F','R','M');
		bs_in_buf.phy_addr[0] = pa + (bs_buf_curr - bs_buf_start); // needs to add offset
		bs_in_buf.size = bs_size;
		bs_in_buf.ddr_id = ddr_id;
		bs_in_buf.timestamp = hd_gettime_us();
		bs_in_buf.bit_width = HD_AUDIO_BIT_WIDTH_16;
		bs_in_buf.sound_mode = usr_mode;
		bs_in_buf.sample_rate = p_out_only->sample_rate;

		/* push in buffer */
		data_time += au_frame_ms;
resend:
		ret = hd_audioout_push_in_buf(p_out_only->out_path, &bs_in_buf, -1);
		//printf("333333333333\r\n");
		if (ret != HD_OK) {
			printf("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
			//usleep(10000);
			goto resend;
		}

		bs_buf_curr += ALIGN_CEIL_4(bs_size); // shift to next
	}

	usleep(100000);
	/* release memory */
	hd_common_mem_munmap((void*)va, blk_size);
rel_blk:
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		printf("release blk fail, ret(%d)\n", ret);
		goto play_fclose;
	}

play_fclose:
	if (bs_fd != NULL) { fclose(bs_fd);}
	if (len_fd != NULL) { fclose(len_fd);}

	return 0;
}

EXAMFUNC_ENTRY(hd_audioout_test, argc, argv)
{
	HD_RESULT ret;
	INT key;
	pthread_t out_thread_id;
	AUDIO_OUTONLY outonly = {0};
	UINT32 tmp=0;

	if (argc != 7) {
		printf("arg(%d) != 7 , usage:\r\n", argc);
		printf("./hd_audio_out_test <drv_out:1~4> <drv_mono:0~1> <vol:0~160> <usr_mode:1~2> <usr_sampling_rate:0~8>  <file_path> \r\n");
		return 0;
	}

	tmp = atoi(argv[1]);
	switch (tmp) {
		case 1:
			printf("drv_out: SPK\r\n");
			drv_out=HD_AUDIOOUT_OUTPUT_SPK;
			break;
		case 2:
			printf("drv_out: LINE\r\n");
			drv_out=HD_AUDIOOUT_OUTPUT_LINE;
			break;
		case 3:
			printf("drv_out: ALL\r\n");
			drv_out=HD_AUDIOOUT_OUTPUT_ALL;
			break;
		case 4:
			printf("drv_out: I2S\r\n");
			drv_out=HD_AUDIOOUT_OUTPUT_I2S;
			break;
		default:
			printf("not support drv_out option=%d, 1~4\r\n", tmp);
			return 0;
			break;
	}

	tmp = atoi(argv[2]);
	switch (tmp) {
		case 0:
			printf("drv_mono: LEFT\r\n");
			drv_mono = HD_AUDIO_MONO_LEFT;
			break;
		case 1:
			printf("drv_mono: RIGHT\r\n");
			drv_mono = HD_AUDIO_MONO_RIGHT;
			break;
		default:
			printf("not support drv_mono option=%d, 0:LEFT, 1:RIGHT\r\n", tmp);
			return 0;
			break;
	}

	vol=atoi(argv[3]);
	if ( (vol < 0) || (vol > 160)){
		vol = 80;
		printf("vol:%d over limit (0~160), set to 80\r\n", vol);
	}

	if ( (drv_out == HD_AUDIOOUT_OUTPUT_I2S) && (vol > 100) ){
		vol = 80;
		printf("vol:%d over HD_AUDIOOUT_OUTPUT_I2S mode limit (0~100), set to 80\r\n", vol);
	}

	printf("vol:%d \r\n", vol);


	tmp = atoi(argv[4]);
	switch (tmp) {
		case 1:
			printf("user_mode: mono\r\n");
			usr_mode = HD_AUDIO_SOUND_MODE_MONO;
			break;
		case 2:
			printf("user_mode: stereo\r\n");
			usr_mode = HD_AUDIO_SOUND_MODE_STEREO;
			break;
		default:
			printf("not support mode option=%d, 1~2\r\n", tmp);
			return 0;
			break;
	}

	tmp = atoi(argv[5]);
	switch (tmp) {
		case 0:
			usr_sampling_rate = HD_AUDIO_SR_8000;
			printf("sampling rate:8000\r\n");
			break;
		case 1:
			usr_sampling_rate = HD_AUDIO_SR_11025;
			printf("sampling rate:11250\r\n");
			break;
		case 2:
			usr_sampling_rate = HD_AUDIO_SR_12000;
			printf("sampling rate:12000\r\n");
			break;
		case 3:
			usr_sampling_rate = HD_AUDIO_SR_16000;
			printf("sampling rate:16000\r\n");
			break;
		case 4:
			usr_sampling_rate = HD_AUDIO_SR_22050;
			printf("sampling rate:22050\r\n");
			break;
		case 5:
			usr_sampling_rate = HD_AUDIO_SR_24000;
			printf("sampling rate:24000\r\n");
			break;
		case 6:
			usr_sampling_rate = HD_AUDIO_SR_32000;
			printf("sampling rate:32000\r\n");
			break;
		case 7:
			usr_sampling_rate = HD_AUDIO_SR_44100;
			printf("sampling rate:44100\r\n");
			break;
		case 8:
			usr_sampling_rate = HD_AUDIO_SR_48000;
			printf("sampling rate:48000\r\n");
			break;
		default:
			printf("not support sampling_rate option=%d, use 0~8\r\n", tmp);
			return 0;
			break;
	}

	snprintf(usr_file_path_main, sizeof(usr_file_path_main), "%s", argv[6]);
	printf("input file:%s \r\n", usr_file_path_main);


	//init hdal
	ret = hd_common_init(0);
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}
	//output module init
	ret = init_module();
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	//open output module
	outonly.sample_rate_max = usr_sampling_rate; //assign by user
	ret = open_module(&outonly);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	//set audioout parameter
	outonly.sample_rate = usr_sampling_rate; //assign by user
	ret = set_out_param(outonly.out_ctrl, outonly.out_path, outonly.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	//create output thread
	ret = pthread_create(&out_thread_id, NULL, playback_thread, (void *)&outonly);
	if (ret < 0) {
		printf("create playback thread failed");
		goto exit;
	}

	//start output module
	hd_audioout_start(outonly.out_path);

	printf("Enter q to exit\n");
	while (outonly.out_exit == 0) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			outonly.out_exit = 1;
			break;
		}
		if (key == 'w') {
			//outonly.out_pause = 1;
			system("mem w f0630004 0x00200381");
		}
		if (key == 'r') {
			system("mem r f0630000");
			system("mem r f0640000");
			//outonly.out_pause = 0;
		}
		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	pthread_join(out_thread_id, NULL);

	//stop output module
	hd_audioout_stop(outonly.out_path);

exit:
	//close output module
	ret = close_module(&outonly);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	//uninit output module
	ret = exit_module();
	if(ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
	// uninit memory
	ret = mem_exit();
	if(ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}
	//uninit hdal
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common-uninit fail=%d\n", ret);
	}

	return 0;
}
