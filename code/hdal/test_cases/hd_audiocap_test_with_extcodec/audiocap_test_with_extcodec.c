/**
	@brief Sample code of audio capture.\n

	@file test_audio_capture_only_with_extcodec.c

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
#include "vendor_audiocapture.h"
#include "vendor_audioout.h"
#include <kwrap/examsys.h>

#if defined(__LINUX)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#endif

#define DEBUG_MENU 1

#define CHKPNT    printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)   printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)   printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

#define FRAME_PER_TEST	1

///////////////////////////////////////////////////////////////////////////////

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	/*dummy buffer, not for audio module*/
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

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
typedef struct _TEST_ITEM_USER_CAPONLY {
	HD_AUDIO_SR				max_sample_rate;
	HD_AUDIO_SR				sample_rate;
	HD_AUDIO_SOUND_MODE		mode;
	UINT32					volume;
	HD_AUDIO_MONO			mono;
} TEST_ITEM_USER_CAPONLY;

typedef struct _AUDIO_CAPONY {
	TEST_ITEM_USER_CAPONLY	item;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	UINT32 cap_exit;
	UINT32 flow_start;
} AUDIO_CAPONLY;

///////////////////////////////////////////////////////////////////////////////
static HD_RESULT set_cap_cfg(HD_PATH_ID *p_audio_cap_ctrl, TEST_ITEM_USER_CAPONLY *p_item)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_AUDIOCAP_VOLUME audio_cap_volume = {0};
	HD_PATH_ID audio_cap_ctrl = 0;
	VENDOR_AUDIOCAP_INIT_CFG vendor_config = {0};

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture maximum parameters*/
	audio_cfg_param.in_max.sample_rate = p_item->max_sample_rate;
	audio_cfg_param.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.in_max.frame_sample = 98304;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.out_max.sample_rate = 0;

	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture driver parameters*/
	audio_driver_cfg_param.mono = p_item->mono;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}
	
	//set hd_audiocapture volume parameters
	audio_cap_volume.volume = p_item->volume;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_cap_volume);
	if (ret != HD_OK){
		return ret;
	}

	//set for external codec input
	snprintf(vendor_config.driver_name, VENDOR_AUDIOCAP_NAME_LEN-1, "nvt_aud_emu");
	vendor_config.aud_init_cfg.pin_cfg.pinmux.audio_pinmux  = 0x12; //PIN_AUDIO_CFG_I2S_2ND_PINMUX | PIN_AUDIO_CFG_MCLK
	vendor_config.aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux = 0;//0x1000; //PIN_I2C_CFG_CH4

	vendor_config.aud_init_cfg.i2s_cfg.bit_clk_ratio = 32;
	vendor_config.aud_init_cfg.i2s_cfg.bit_width     = 16;
	vendor_config.aud_init_cfg.i2s_cfg.tdm_ch        = 2;
	vendor_config.aud_init_cfg.i2s_cfg.op_mode       = 1;

	ret = vendor_audiocap_set(audio_cap_ctrl, VENDOR_AUDIOCAP_ITEM_EXT, (VOID *)&vendor_config);

	*p_audio_cap_ctrl = audio_cap_ctrl;

	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID audio_cap_path, TEST_ITEM_USER_CAPONLY *p_item)
{
	HD_RESULT ret = HD_OK;
	//set hd_audiocapture input parameters
	HD_AUDIOCAP_IN audio_cap_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};

	audio_cap_param.sample_rate = p_item->sample_rate;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = p_item->mode;
	audio_cap_param.frame_sample = 98304;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	ret = set_cap_cfg(&p_caponly->cap_ctrl, &p_caponly->item);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}
static void *capture_thread(void* arg){
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main;
	UINT32 frame_per_test = FRAME_PER_TEST;
	HD_AUDIOCAP_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
	AUDIO_CAPONLY *p_cap_only = (AUDIO_CAPONLY *)arg;

	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_%lu_%d_pcm_extcodec.dat", HD_AUDIO_BIT_WIDTH_16, p_cap_only->item.sample_rate, p_cap_only->item.mode, p_cap_only->item.volume, p_cap_only->item.mono);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_%lu_%d_pcm_extcodec.len", HD_AUDIO_BIT_WIDTH_16, p_cap_only->item.sample_rate, p_cap_only->item.mode, p_cap_only->item.volume, p_cap_only->item.mono);	

	/* wait flow_start */
	while (p_cap_only->flow_start == 0) {
		sleep(1);
	}

	/* query physical address of bs buffer
	  (this can ONLY query after hd_audiocap_start() is called !!) */
	hd_audiocap_get(p_cap_only->cap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main);

	/* mmap for bs buffer
	  (just mmap one time only, calculate offset to virtual address later) */
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	if (vir_addr_main == 0) {
		printf("mmap error\r\n");
		return 0;
	}

	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}

	/* pull data test */
	while (frame_per_test != 0){
		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->cap_path, &data_pull, 200); // >1 = timeout mode

		if (ret == HD_OK) {
			//if (frame_per_test%20 == 0)
			printf("%d frames captured.\n", FRAME_PER_TEST - frame_per_test);
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write bs len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);

			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->cap_path, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
			frame_per_test--;
		}
	}
	printf("%d frames captured.\n", FRAME_PER_TEST - frame_per_test);
	p_cap_only->cap_exit = 1;

	/* mummap for bs buffer */
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	/* close output file */
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);

	return 0;
}

static void DumpParam(TEST_ITEM_USER_CAPONLY *ToPrint){
	printf("=====DUMPING START=====\n");
	switch (ToPrint->sample_rate){
		case HD_AUDIO_SR_8000:
			printf("sample rate = 8000.\n");
			break;
		case HD_AUDIO_SR_11025:
			printf("sample rate = 11025.\n");
			break;
		case HD_AUDIO_SR_12000:
			printf("sample rate = 12000.\n");
			break;
		case HD_AUDIO_SR_16000:
			printf("sample rate = 16000.\n");
			break;
		case HD_AUDIO_SR_22050:
			printf("sample rate = 22050.\n");
			break;
		case HD_AUDIO_SR_24000:
			printf("sample rate = 24000.\n");
			break;
		case HD_AUDIO_SR_32000:
			printf("sample rate = 32000.\n");
			break;
		case HD_AUDIO_SR_44100:
			printf("sample rate = 44100.\n");
			break;
		case HD_AUDIO_SR_48000:
			printf("sample rate = 48000.\n");
			break;
		default:
			printf("Unknown sample rate!\n");
	}
	switch (ToPrint->mode){
		case HD_AUDIO_SOUND_MODE_MONO:
			printf("mode = mono.\n");
			break;
		case HD_AUDIO_SOUND_MODE_STEREO:
			printf("mode = stereo.\n");
			break;
		default:
			printf("Unknown mode!\n");
	}
	if (ToPrint->volume <= 160){
		printf("volume = %u.\n", ToPrint->volume);
	} else {
		printf("Unknown volume!\n");
	}
	switch (ToPrint->mono){
		case HD_AUDIO_MONO_LEFT:
			printf("mono = left.\n");
			break;
		case HD_AUDIO_MONO_RIGHT:
			printf("mono = right.\n");
			break;
		default:
			printf("Unknown mono!\n");
	}
	printf("======DUMPING END======\n");
}

EXAMFUNC_ENTRY(hd_audiocap_test_with_extcodec, argc, argv)
{
	HD_RESULT ret;
	//INT key;
	AUDIO_CAPONLY caponly = {0};
	pthread_t cap_thread_id;
	UINT32 sample_rate_argv = 0;
	UINT32 mode_argv = 0;
	UINT32 volume_argv = 0;
	UINT32 mono_argv = 0;

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
	//audiocap module init
	ret = init_module();
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	//set audiocap parameter
	if (argc > 1){
		sample_rate_argv = strtoul(argv[1], NULL, 10);
        switch (sample_rate_argv){
			case 8000:
				//sample rate = 8000
				caponly.item.sample_rate = HD_AUDIO_SR_8000;
				break;
			case 11025:
				//sample rate = 11025
				caponly.item.sample_rate = HD_AUDIO_SR_11025;
				break;
			case 12000:
				//sample rate = 12000
				caponly.item.sample_rate = HD_AUDIO_SR_12000;
				break;
			case 16000:
				//sample rate = 16000
				caponly.item.sample_rate = HD_AUDIO_SR_16000;
				break;
			case 22050:
				//sample rate = 22050
				caponly.item.sample_rate = HD_AUDIO_SR_22050;
				break;
			case 24000:
				//sample rate = 24000
				caponly.item.sample_rate = HD_AUDIO_SR_24000;
				break;
			case 32000:
				//sample rate = 32000
				caponly.item.sample_rate = HD_AUDIO_SR_32000;
				break;
			case 44100:
				//sample rate = 44100
				caponly.item.sample_rate = HD_AUDIO_SR_44100;
				break;
			case 48000:
			default:
				//sample rate = 48000
				caponly.item.sample_rate = HD_AUDIO_SR_48000;
		}
	}
	if (argc > 2){
		mode_argv = strtoul(argv[2], NULL, 10);
		switch (mode_argv){
			case 1:
				//mode = mono
				caponly.item.mode = HD_AUDIO_SOUND_MODE_MONO;
				break;
			case 2:
			default:
				//mode = stereo
				caponly.item.mode = HD_AUDIO_SOUND_MODE_STEREO;
		}
	}
	if (argc > 3){
		//volume
		volume_argv = atoi(argv[3]);
		if (volume_argv <= 160)
			caponly.item.volume = volume_argv;
		else
			caponly.item.volume = 160;
	}
	if (argc>4 && caponly.item.mode==HD_AUDIO_SOUND_MODE_MONO){
		mono_argv = strtoul(argv[4], NULL, 10);
		switch (mono_argv){
			case 1:
				//left mono
				caponly.item.mono = HD_AUDIO_MONO_LEFT;
				break;
			case 2:
			default:
				//right mono
				caponly.item.mono = HD_AUDIO_MONO_RIGHT;
		}
	}
	//dump item
	DumpParam(&caponly.item);

	//open audiocap module
	caponly.item.max_sample_rate = HD_AUDIO_SR_48000;	//assign by user
	ret = open_module(&caponly);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	//set audiocap parameter
	ret = set_cap_param(caponly.cap_path, &caponly.item);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	//create capture thread
	ret = pthread_create(&cap_thread_id, NULL, capture_thread, (void *)&caponly);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	//start audiocap module
	hd_audiocap_start(caponly.cap_path);

	caponly.flow_start = 1;

	pthread_join(cap_thread_id, NULL);

	//stop audiocap module
	hd_audiocap_stop(caponly.cap_path);

exit:
	//close all module
	ret = close_module(&caponly);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	//uninit all module
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
