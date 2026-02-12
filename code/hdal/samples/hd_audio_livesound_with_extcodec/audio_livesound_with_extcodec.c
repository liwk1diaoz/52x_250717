/**
	@brief Sample code of audio livesound.\n

	@file audio_livesound_with_extcodec.c

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
#define usleep(x)   vos_task_delay_us(x)
#endif

#define DEBUG_MENU 1

#define CHKPNT    printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)   printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)   printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

#define BITSTREAM_SIZE      12800
#define FRAME_SAMPLES       1024
#define TDM_CH              4
#define TDM_CH_TX           2
#define MIXED_CHANNEL       0
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
	/* user buffer for bs1 pushing in */
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[1].blk_size = 0x100000;
	mem_cfg.pool_info[1].blk_cnt = 5;
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

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_audio_cap_ctrl, HD_AUDIO_SR sample_rate, int op_mode)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_PATH_ID audio_cap_ctrl = 0;
	VENDOR_AUDIOCAP_INIT_CFG vendor_config = {0};

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture maximum parameters*/
	audio_cfg_param.in_max.sample_rate = sample_rate;
	audio_cfg_param.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.in_max.frame_sample = FRAME_SAMPLES;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.out_max.sample_rate = 0;

	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture driver parameters*/
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	snprintf(vendor_config.driver_name, VENDOR_AUDIOCAP_NAME_LEN-1, "nvt_aud_emu");
	vendor_config.aud_init_cfg.pin_cfg.pinmux.audio_pinmux  = 0x12; //PIN_AUDIO_CFG_I2S | PIN_AUDIO_CFG_MCLK
	vendor_config.aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux = 0;//0x1000; //PIN_I2C_CFG_CH4

	vendor_config.aud_init_cfg.i2s_cfg.bit_clk_ratio = 64;
	vendor_config.aud_init_cfg.i2s_cfg.bit_width     = 16;
	vendor_config.aud_init_cfg.i2s_cfg.tdm_ch        = TDM_CH;
	vendor_config.aud_init_cfg.i2s_cfg.op_mode       = op_mode; //1:master, 0:slave

	ret = vendor_audiocap_set(audio_cap_ctrl, VENDOR_AUDIOCAP_ITEM_EXT, (VOID *)&vendor_config);

	*p_audio_cap_ctrl = audio_cap_ctrl;

	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	//set hd_audiocapture input parameters
	HD_AUDIOCAP_IN audio_cap_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};

	audio_cap_param.sample_rate = sample_rate;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cap_param.frame_sample = FRAME_SAMPLES;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_audio_out_ctrl, HD_AUDIO_SR sample_rate, int op_mode)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_PATH_ID audio_out_ctrl = 0;
	VENDOR_AUDIOOUT_INIT_CFG vendor_config = {0};

	ret = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &audio_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio out maximum parameters*/
	audio_cfg_param.out_max.sample_rate = sample_rate;
	audio_cfg_param.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.out_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.frame_sample_max = FRAME_SAMPLES;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.in_max.sample_rate = 0;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio out driver parameters*/
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_LEFT;
	audio_driver_cfg_param.output = HD_AUDIOOUT_OUTPUT_I2S;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	snprintf(vendor_config.driver_name, VENDOR_AUDIOOUT_NAME_LEN-1, "nvt_aud_emu");
	vendor_config.aud_init_cfg.pin_cfg.pinmux.audio_pinmux  = 0x12; //PIN_AUDIO_CFG_I2S | PIN_AUDIO_CFG_MCLK
	vendor_config.aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux = 0;//0x1000; //PIN_I2C_CFG_CH4

	vendor_config.aud_init_cfg.i2s_cfg.bit_clk_ratio = 64;
	vendor_config.aud_init_cfg.i2s_cfg.bit_width     = 16;
	vendor_config.aud_init_cfg.i2s_cfg.tdm_ch        = TDM_CH_TX;
	vendor_config.aud_init_cfg.i2s_cfg.op_mode       = op_mode; // 1 for master, 0  for slave

	vendor_audioout_set(audio_out_ctrl, VENDOR_AUDIOOUT_ITEM_EXT, (VOID *)&vendor_config);

	*p_audio_out_ctrl = audio_out_ctrl;

	return ret;
}


static HD_RESULT set_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	//set hd_audioout output parameters
	HD_AUDIOOUT_OUT audio_out_out_param = {0};
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};
	HD_AUDIOOUT_IN audio_out_in_param = {0};

	audio_out_out_param.sample_rate = sample_rate;
	audio_out_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_out_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_OUT, &audio_out_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audioout volume
	audio_out_vol.volume = 50;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audioout input parameters
	audio_out_in_param.sample_rate = 0;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_IN, &audio_out_in_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_LIVESOUND {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	UINT32 cap_exit;
	UINT32 flow_start;

	UINT32 out_exit;

} AUDIO_LIVESOUND;

#define TIME_DIFF(new_val, old_val)     ((int)(new_val) - (int)(old_val))

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_init()) != HD_OK)
		return ret;
	if((ret = hd_audioout_init()) != HD_OK)
		return ret;
	return HD_OK;
}


static HD_RESULT open_module(AUDIO_LIVESOUND *p_livesound, int op_mode)
{
	HD_RESULT ret;
	ret = set_cap_cfg(&p_livesound->cap_ctrl, p_livesound->sample_rate_max, op_mode);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	ret = set_out_cfg(&p_livesound->out_ctrl, p_livesound->sample_rate_max, op_mode);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_livesound->cap_path)) != HD_OK)
		return ret;

	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_livesound->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(AUDIO_LIVESOUND *p_livesound)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_livesound->cap_path)) != HD_OK)
		return ret;
	if((ret = hd_audioout_close(p_livesound->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	if((ret = hd_audioout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

#if MIXED_CHANNEL
static void audio_pack_and_mix(UINT32 input_addr_mixer,UINT32 input_addr, UINT32 output_addr, UINT32 in_sample_cnt, UINT32 channel)
{
	UINT32 i;
	INT16 temp =0;

	for(i=0; i< in_sample_cnt; i++) {
		temp = ((INT16 *)input_addr)[i] + ((INT16 *)input_addr_mixer)[i];
		if ( (-32768 <= temp)  && (temp <= 32767) )
			((INT16 *)(output_addr))[(i<<(TDM_CH_TX/2))+channel] = temp;
		else if (temp < -32768) //Lowest Saturation
			((INT16 *)(output_addr))[(i<<(TDM_CH_TX/2))+channel] = -32768;
		else if ( 32767 < temp)  //Highest Saturation
			((INT16 *)(output_addr))[(i<<(TDM_CH_TX/2))+channel] =  32767;
	}
}
#endif

static void audio_pack(UINT32 input_addr, UINT32 output_addr, UINT32 in_sample_cnt, UINT32 channel)
{
	UINT32 i;

	for(i=0; i< in_sample_cnt; i++) {
		((INT16 *)(output_addr))[(i<<(TDM_CH_TX/2))+channel] = ((INT16 *)input_addr)[i];
	}
}


static void audio_depack(UINT32 input_addr, UINT32 output_addr, UINT32 in_sample_cnt, UINT32 channel)
{
	UINT32 i;

	for(i=0; i< in_sample_cnt; i++) {
		((INT16 *)(output_addr))[i] = ((INT16 *)(input_addr))[(i<<(TDM_CH/2))+channel];
	}

}

static void *playback_thread(void *arg)
{
	INT ret, bs_size[4], result;
	CHAR filename[4][50];
	FILE *bs_fd[4], *len_fd[4];
	HD_AUDIO_FRAME  bs_in_buf = {0};
	HD_COMMON_MEM_VB_BLK blk[5];
	UINT32 pa[5], va[5];
	UINT32 blk_size = 0x100000;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 bs_buf_start[5], bs_buf_curr[5], bs_buf_end[5];
	INT timestamp[4];
	AUDIO_LIVESOUND *p_out = (AUDIO_LIVESOUND *)arg;
	int i=0;

	/* read test pattern */ //reads 4 files
	for (i=0; i<TDM_CH_TX; i++)
	{
		snprintf(filename[i], sizeof(filename[i]), "/mnt/sd/audio_bs%d_%d_%d_1_pcm",(i+1), p_out->sample_rate, HD_AUDIO_BIT_WIDTH_16);
		bs_fd[i] = fopen(filename[i], "rb");
		if (bs_fd[i] == NULL) {
			printf("[ERROR] Open %s failed!!\n", filename);
			return 0;
		}
		printf("play file: [%s]\n", filename[i]);

		snprintf(filename[i], sizeof(filename[i]), "/mnt/sd/audio_bs%d_%d_%d_1_pcm.len",(i+1), p_out->sample_rate, HD_AUDIO_BIT_WIDTH_16);
		len_fd[i] = fopen(filename[i], "rb");
		if (len_fd[i] == NULL) {
			printf("[ERROR] Open %s failed!!\n", filename[i]);
			goto play_fclose;
		}
		printf("len file: [%s]\n", filename[i]);
	}

	for (i=0; i<5; i++) //allocate buffer
	{
		/* get memory */
		blk[i] = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
		if (blk[i] == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail, blk[%d] = 0x%x\n",i, blk[i]);
			goto play_fclose;
		}

		pa[i] = hd_common_mem_blk2pa(blk[i]); // get physical addr
		if (pa[i] == 0) {
			printf("blk2pa fail, blk[%d]:(0x%x)\n", i, blk[i]);
			goto rel_blk;
		}

		if (pa[i] > 0) {
			va[i] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa[i], blk_size); // Get virtual addr
			if (va[i] == 0) {
				printf("get va fail, va[%d]:(0x%x)\n", i, blk[i]);
				goto rel_blk;
			}
			/* allocate bs buf */
			bs_buf_start[i] = va[i];
			bs_buf_curr[i] = bs_buf_start[i];
			bs_buf_end[i] = bs_buf_start[i] + blk_size;
			printf("alloc bs_buf[%d]: start(0x%x) curr(0x%x) end(0x%x) size(0x%x)\n",i, bs_buf_start[i], bs_buf_curr[i], bs_buf_end[i], blk_size);
		}
	}


	while (1) {
		if (p_out->out_exit == 1) {
			break;
		}

		for(i=0;i<TDM_CH_TX;i++)
		{
			/* get bs size */
			if (fscanf(len_fd[i], "%d %d\n", &bs_size[i], &timestamp[i]) == EOF) {
				// reach EOF, read from the beginning
				fseek(bs_fd[i], 0, SEEK_SET);
				fseek(len_fd[i], 0, SEEK_SET);
				if (fscanf(len_fd[i], "%d %d\n", &bs_size[i], &timestamp[i]) == EOF) {
					printf("[ERROR] fscanf error\n");
					continue;
				}
			}

			if (bs_size[i] == 0 || bs_size[i] > BITSTREAM_SIZE) {
				printf("Invalid bs_size(%d)\n", bs_size);
				continue;
			}
		}

		/* read bs from file and synthesis frames of each channel to 1 audio buffer*/
		for(i=0;i<TDM_CH_TX;i++)
		{
			result = fread((void *)bs_buf_curr[i], 1, bs_size[i], bs_fd[i]);
			if (result != bs_size[i]) {
				printf("reading error\n");
				continue;
			}
			//printf("bs_size= %d\r\n", bs_size[i]);
#if MIXED_CHANNEL
			if (i==0)
				audio_pack(va[i], va[4], FRAME_SAMPLES, i);// keep CH0
			else
				audio_pack_and_mix(va[1],va[i], va[4], FRAME_SAMPLES, i);//mix CH0 to CH1,CH2,CH3
#else
			//pack multi-channel audio
			audio_pack(va[i], va[4], FRAME_SAMPLES, i);

#endif
		}

		bs_in_buf.sign = MAKEFOURCC('A','F','R','M');
		bs_in_buf.phy_addr[0] = pa[4]; // needs to add offset
		bs_in_buf.size = FRAME_SAMPLES * 2 * TDM_CH_TX ;//each sample is 2 bytes(16-bits), for TDM_CH_TX CH
		bs_in_buf.ddr_id = ddr_id;
		bs_in_buf.timestamp = hd_gettime_us();
		bs_in_buf.bit_width = HD_AUDIO_BIT_WIDTH_16;
		bs_in_buf.sound_mode = HD_AUDIO_SOUND_MODE_STEREO;
		bs_in_buf.sample_rate = p_out->sample_rate;

		ret = hd_audioout_push_in_buf(p_out->out_path, &bs_in_buf, -1);
		if (ret != HD_OK) {
			printf("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
		}
	}

	/* release memory */
	for (i=0; i<5; i++)
	{
		hd_common_mem_munmap((void*)va[i], blk_size);
	}

rel_blk:
	for (i=0; i<5; i++)
	{
		ret = hd_common_mem_release_block(blk[i]);
		if (HD_OK != ret) {
			printf("release blk(%d) fail, ret(%d)\n", i,ret);
			goto play_fclose;
		}
	}

play_fclose:
	for (i=0; i<TDM_CH_TX; i++)
	{
		if (bs_fd[i] != NULL) { fclose(bs_fd[i]);}
		if (len_fd[i] != NULL) { fclose(len_fd[i]);}
	}

	return 0;
}

static void *capture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main, temp[4][1024];
	HD_AUDIOCAP_BUFINFO phy_buf_main;
	char file_path_main[4][64], file_path_len[4][64];
	FILE *f_out_main[4], *f_out_len[4];
	AUDIO_LIVESOUND *p_cap = (AUDIO_LIVESOUND *)arg;
	int i =0;

	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	for (i=0; i<4; i++)
	{
		/* config pattern name */
		snprintf(file_path_main[i], sizeof(file_path_main[i]), "/mnt/sd/audio_rbs%d_%d_%d_1_pcm", (i+1),p_cap->sample_rate, HD_AUDIO_BIT_WIDTH_16);
		snprintf(file_path_len[i], sizeof(file_path_len[i]), "/mnt/sd/audio_rbs%d_%d_%d_1_pcm.len", (i+1), p_cap->sample_rate, HD_AUDIO_BIT_WIDTH_16);
	}

	/* wait flow_start */
	while (p_cap->flow_start == 0) {
		sleep(1);
	}

	/* query physical address of bs buffer
	  (this can ONLY query after hd_audiocap_start() is called !!) */
	hd_audiocap_get(p_cap->cap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main);

	/* mmap for bs buffer
	  (just mmap one time only, calculate offset to virtual address later) */
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	if (vir_addr_main == 0) {
		printf("mmap error\r\n");
		return 0;
	}

	for (i=0; i<4; i++)
	{
		/* open output files */
		if ((f_out_main[i] = fopen(file_path_main[i], "wb")) == NULL) {
			printf("open file (%s) fail....\r\n", file_path_main);
		} else {
			printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main[i]);
		}

		if ((f_out_len[i] = fopen(file_path_len[i], "wb")) == NULL) {
			printf("open len file (%s) fail....\r\n", file_path_len[i]);
		}
	}

	/* pull data test */
	while (p_cap->cap_exit == 0) {
		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap->cap_path, &data_pull, 200); // >1 = timeout mode

		if (ret == HD_OK) {

			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);

			/*
			UINT32 size = data_pull.size;// 4CH
			printf("PULL data size: %u\r\n", size);
			*/


			for(i=0; i<4; i++)
			{
				//depack for multi-channel audio
				audio_depack(((UINT32)ptr), (UINT32)temp[i], FRAME_SAMPLES, i);

				// write bs
				if (f_out_main[i]) fwrite( temp[i], 1, FRAME_SAMPLES*2, f_out_main[i]);//2 bytes per sample
				if (f_out_main[i]) fflush(f_out_main[i]);
			}

			// release data
			ret = hd_audiocap_release_out_buf(p_cap->cap_path, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}

	}

	/* mummap for bs buffer */
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	for (i=0; i<4; i++)
	{
		/* close output file */
		if (f_out_main[i]) fclose(f_out_main[i]);
		if (f_out_len[i]) fclose(f_out_len[i]);
	}
	return 0;
}

EXAMFUNC_ENTRY(hd_audio_livesound_with_extcodec, argc, argv)
{
	HD_RESULT ret;
	INT key;
	AUDIO_LIVESOUND livesound = {0};
	pthread_t out_thread_id, cap_thread_id;

	if (argc < 2){
		printf("Please choose I2S mode for argv[1], 0:slave, 1:master\r\n");
		return 0;
	}

	if ( (atoi(argv[1]) < 0) || (atoi(argv[1]) >1)){
		printf("Please choose I2S mode for argv[1], 0:slave, 1:master\r\n");
		return 0;
	}

	printf("opmode = %d\r\n", atoi(argv[1]));

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
	//livesound module init
	ret = init_module();
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	//open livesound module
	livesound.sample_rate_max = HD_AUDIO_SR_16000; //assign by user
	ret = open_module(&livesound, atoi(argv[1]));
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	//set audiocap parameter
	livesound.sample_rate = HD_AUDIO_SR_16000; //assign by user
	ret = set_cap_param(livesound.cap_path, livesound.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}
	//set audioout parameter
	ret = set_out_param(livesound.out_ctrl, livesound.out_path, livesound.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	//create capture thread
	ret = pthread_create(&cap_thread_id, NULL, capture_thread, (void *)&livesound);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}


	//create output thread
	ret = pthread_create(&out_thread_id, NULL, playback_thread, (void *)&livesound);
	if (ret < 0) {
		printf("create playback thread failed");
		goto exit;
	}

	//start livesound module
	hd_audiocap_start(livesound.cap_path);
	hd_audioout_start(livesound.out_path);

	livesound.flow_start = 1;

	printf("\r\nEnter q to exit, Enter d to debug\r\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			livesound.cap_exit=1;
			livesound.out_exit=1;
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	pthread_join(cap_thread_id, NULL);
	pthread_join(out_thread_id, NULL);

	//stop livesound module
	hd_audiocap_stop(livesound.cap_path);

	hd_audioout_stop(livesound.out_path);
	//unbind livesound module

exit:
	//close all module
	ret = close_module(&livesound);
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

	// uninit hdal
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common-uninit fail=%d\n", ret);
	}

	return 0;
}
