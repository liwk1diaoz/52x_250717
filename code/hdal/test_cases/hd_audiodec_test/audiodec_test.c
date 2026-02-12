/**
	@brief Sample code of audio decode only.\n

	@file audio_decode_only.c

	@author Adam SY Su

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

#define DEBUG_MENU 1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define BITSTREAM_SIZE      12800
#define FRAME_SAMPLES       1024
#define AUD_BUFFER_CNT      2
#define ADDITION_DELAY      1

#define TIME_DIFF(new_val, old_val)     ((int)(new_val) - (int)(old_val))

static int usr_sampling_rate=0, usr_mode=0;
//int buf_cnt=0;
static char usr_file_path_main[64]={0}, usr_output_path[64]={0};
//static pthread_mutex_t     lock;
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (main)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[0].blk_size = 0x100000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_dec_cfg(HD_PATH_ID audio_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIODEC_PATH_CONFIG audio_path_cfg = {0};

	// set audiodec path config
	audio_path_cfg.max_mem.codec_type = dec_type;
	audio_path_cfg.max_mem.sample_rate = usr_sampling_rate;
	audio_path_cfg.max_mem.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_path_cfg.max_mem.mode = usr_mode;
	ret = hd_audiodec_set(audio_dec_path, HD_AUDIODEC_PARAM_PATH_CONFIG, &audio_path_cfg);

	return ret;
}

static HD_RESULT set_dec_param(HD_PATH_ID audio_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIODEC_IN audio_in_param = {0};

	audio_in_param.codec_type = dec_type;
	audio_in_param.sample_rate = usr_sampling_rate;
	audio_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_in_param.mode = usr_mode;
	ret = hd_audiodec_set(audio_dec_path, HD_AUDIODEC_PARAM_IN, &audio_in_param);
	if (ret != HD_OK) {
		printf("set_dec_param_in = %d\r\n", ret);
		return ret;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_DECODE {

	// (1) audio dec
	HD_PATH_ID dec_path;
	UINT32 dec_type;
	UINT32 flow_start;

	// (2) user push
	pthread_t  feed_thread_id;
	UINT32     feed_exit;

	// (3) user pull
	pthread_t  dec_thread_id;
	UINT32     dec_exit;

} AUDIO_DECODE;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiodec_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(AUDIO_DECODE *p_stream)
{
    HD_RESULT ret;
    if((ret = hd_audiodec_open(HD_AUDIODEC_0_IN_0, HD_AUDIODEC_0_OUT_0, &p_stream->dec_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT close_module(AUDIO_DECODE *p_stream)
{
    HD_RESULT ret;
    if((ret = hd_audiodec_close(p_stream->dec_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT exit_module(void)
{
    HD_RESULT ret;
    if((ret = hd_audiodec_uninit()) != HD_OK)
        return ret;
    return HD_OK;
}

static void *feed_bs_thread(void *arg)
{
	AUDIO_DECODE* p_stream0 = (AUDIO_DECODE *)arg;
	INT ret, bs_size, result, timestamp;
	CHAR filename[64], codec_name[8];
	FILE *bs_fd, *len_fd;
	HD_AUDIO_BS  bs_in_buf = {0};
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 pa, va;
	UINT32 blk_size = 0x100000;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 bs_buf_start, bs_buf_curr, bs_buf_end;
	INT au_frame_ms, elapse_time, au_buf_time;
	UINT start_time, data_time;

	// wait flow_start
	while (p_stream0->flow_start == 0) {
		sleep(1);
	}

	// assign audio info
	HD_AUDIO_SR sample_rate = usr_sampling_rate;
	HD_AUDIO_BIT_WIDTH sample_bit = HD_AUDIO_BIT_WIDTH_16;
	HD_AUDIO_SOUND_MODE mode = usr_mode;

	switch (p_stream0->dec_type) {
		case HD_AUDIO_CODEC_AAC:
			snprintf(codec_name, sizeof(codec_name), "aac");
			break;
		case HD_AUDIO_CODEC_ULAW:
			snprintf(codec_name, sizeof(codec_name), "g711u");
			break;
		case HD_AUDIO_CODEC_ALAW:
			snprintf(codec_name, sizeof(codec_name), "g711a");
			break;
		case HD_AUDIO_CODEC_PCM:
			snprintf(codec_name, sizeof(codec_name), "pcm");
			break;
		default:
			printf("invalid audio codec(%d)\n", p_stream0->dec_type);
			break;
	}

	// config test pattern
	snprintf(filename, sizeof(filename), "%s", usr_file_path_main);

	bs_fd = fopen(filename, "rb");
	if (bs_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		return 0;
	}
	printf("input file: [%s], mode: [%d], sample_bit:[%d], sample_rate: [%d]\n", filename, mode, sample_bit, sample_rate);


	snprintf(filename, sizeof(filename), "%s.len", usr_file_path_main);

	len_fd = fopen(filename, "rb");
	if (len_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		goto play_fclose;
	}
	printf("len file: [%s]\n", filename);

	au_frame_ms = FRAME_SAMPLES * 1000 / sample_rate; // the time(in ms) of each audio frame
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

	while (0 == p_stream0->feed_exit) {
retry:
		if (p_stream0->feed_exit == 1) {
			break;
		}

		elapse_time = TIME_DIFF(hd_gettime_ms(), start_time);
		au_buf_time = data_time - elapse_time;
		if (au_buf_time > AUD_BUFFER_CNT * au_frame_ms) {
			usleep(10000);
			goto retry;
		}

		/* get bs size */
		if (fscanf(len_fd, "%d %d\n", &bs_size, &timestamp) == EOF) {
			printf("decode file(%s) finished, EOF!!!\r\n", usr_file_path_main);
			p_stream0->dec_exit = 1;
			usleep(100000);
			p_stream0->feed_exit = 1;

			continue;

			// reach EOF, read from the beginning
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
		bs_in_buf.sign = MAKEFOURCC('A','S','T','M');
		bs_in_buf.acodec_format = p_stream0->dec_type;
		bs_in_buf.phy_addr = pa + (bs_buf_curr - bs_buf_start); // needs to add offset
		bs_in_buf.size = bs_size;
		bs_in_buf.ddr_id = ddr_id;
		bs_in_buf.timestamp = hd_gettime_us();

		/* push in buffer */
		data_time += au_frame_ms;
		ret = hd_audiodec_push_in_buf(p_stream0->dec_path, &bs_in_buf, NULL, 0); // only support non-blocking mode now
		if (ret != HD_OK) {
			printf("hd_audiodec_push_in_buf fail, ret(%d)\n", ret);
		}
/*		printf("push before buf_cnt = %d\r\n", buf_cnt);
 		pthread_mutex_lock(&lock);
		if (buf_cnt >= 0)
			buf_cnt++;
		pthread_mutex_unlock(&lock);

		printf("push after buf_cnt = %d\r\n", buf_cnt);*/
		bs_buf_curr += ALIGN_CEIL_4(bs_size); // shift to next

		#if (ADDITION_DELAY == 1)
		usleep(30000);
		#endif
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

static void *decode_thread(void *arg)
{
	AUDIO_DECODE* p_stream0 = (AUDIO_DECODE *)arg;
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main;
	HD_AUDIODEC_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	// wait flow_start
	while (p_stream0->flow_start == 0) {
		sleep(1);
	}

	// assign audio info
	HD_AUDIO_SR sample_rate = usr_sampling_rate;
	HD_AUDIO_BIT_WIDTH sample_bit = HD_AUDIO_BIT_WIDTH_16;
	HD_AUDIO_SOUND_MODE mode = usr_mode;

	// config pattern name
	snprintf(file_path_main, sizeof(usr_output_path), "%s", usr_output_path);
	snprintf(file_path_len, sizeof(usr_output_path), "%s.len", usr_output_path);

	// query physical address of bs buffer (this can ONLY query after hd_audiodec_start() is called !!)
	hd_audiodec_get(p_stream0->dec_path, HD_AUDIODEC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	// open output files
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail...., mode: [%d], sample_bit:[%d], sample_rate: [%d]\r\n", file_path_len, mode, sample_bit, sample_rate);
	}

	//printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	// pull frame data
	while (p_stream0->dec_exit == 0) {
		// pull data
/* 		printf("pull before buf_cnt = %d\r\n", buf_cnt);
		if ( 0 >= buf_cnt )
			continue; */

		ret = hd_audiodec_pull_out_buf(p_stream0->dec_path, &data_pull, 500); // >1 = timeout mode

/* 		pthread_mutex_lock(&lock);
		buf_cnt--;
		pthread_mutex_unlock(&lock);
		//printf("pull after buf_cnt = %d\r\n", buf_cnt);*/
		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			// write frm
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write frm len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);

			// release data
			ret = hd_audiodec_release_out_buf(p_stream0->dec_path, &data_pull);
			if (ret != HD_OK) {
				printf("release_ouf_buf fail, ret(%d)\r\n", ret);
			}
		}
	}

	// mummap for bs buffer
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	// close output file
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);

	return 0;
}

EXAMFUNC_ENTRY(hd_audiodec_test, argc, argv)
{
	HD_RESULT ret;
    //INT key;
	AUDIO_DECODE stream[1] = {0}; //0: main stream
	UINT32 dec_type = 0, sr=0;

	// query program options
	if (argc == 6) {
		dec_type = atoi(argv[1]);
		switch (dec_type){
			case 0:
				printf("dec_type: AAC\r\n");
				break;
			case 1:
				printf("dec_type: G711U\r\n");
				break;
			case 2:
				printf("dec_type: G711A\r\n");
				break;
			case 3:
				printf("dec_type: PCM\r\n");
				break;
			default:
				printf("not support type(%u), use 0~3\r\n", dec_type);
				return 0;
				break;
		}

		usr_mode = atoi(argv[2]);
		switch (usr_mode) {
			case 1:
				printf("user_mode: mono\r\n");
				break;
			case 2:
				printf("user_mode: stereo\r\n");
				break;
			default:
				printf("not support mode option=%d, 1~2\r\n", usr_mode);
				return 0;
				break;
		}

		sr = atoi(argv[3]);
		switch (sr) {
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
				printf("not support sampling_rate option=%d, use 0~8\r\n", sr);
				return 0;
				break;
		}


		snprintf(usr_file_path_main, sizeof(usr_file_path_main), "%s", argv[4]);
		printf("input file:%s \r\n", usr_file_path_main);

		snprintf(usr_output_path, sizeof(usr_output_path), "%s", argv[5]);
		printf("output file:%s \r\n", usr_output_path);

	}else{
		printf("usage: %s <decode_type> <mode> <sampling_reate> <input_file_path> <output_file_path> \r\n",argv[0]);
		printf("decode_type: 0->AAC , 1->G711u , 2->G711a , 3->PCM \r\n");
		printf("mode: 1->mono , 2->stereo \r\n");
		printf("sampling_reate:  0->8k , 1->11.025k , 2->12k , 3->16k , 4->22.05k , 5->24k , 6->32k , 7->44.1k, 8->48k \r\n");
		return 0;
	}

	//pthread_mutex_init(&lock, NULL);

	// init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
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
	ret = init_module();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

    // open all modules
	ret = open_module(&stream[0]);
    if(ret != HD_OK) {
        printf("open fail=%d\n", ret);
        goto exit;
    }

	// query decode type
	if (dec_type == 0) {
		stream[0].dec_type = HD_AUDIO_CODEC_AAC;
	} else if (dec_type == 1) {
		stream[0].dec_type = HD_AUDIO_CODEC_ULAW;
	} else if (dec_type == 2) {
		stream[0].dec_type = HD_AUDIO_CODEC_ALAW;
	} else {
		stream[0].dec_type = HD_AUDIO_CODEC_PCM;
	}

	// set audiodec config
	ret = set_dec_cfg(stream[0].dec_path, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec-cfg fail=%d\n", ret);
		goto exit;
	}

	// set audiodec paramter
	ret = set_dec_param(stream[0].dec_path, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec fail=%d\n", ret);
		goto exit;
	}

	// create decode_thread (pull_out frame)
	ret = pthread_create(&stream[0].dec_thread_id, NULL, decode_thread, (void *)stream);
	if (ret < 0) {
		printf("create decode_thread failed\n");
		goto exit;
	}

	// create feed_bs_thread (push_in bitstream)
	ret = pthread_create(&stream[0].feed_thread_id, NULL, feed_bs_thread, (void *)stream);
	if (ret < 0) {
		printf("create feed_bs_thread failed\n");
		goto exit;
	}

	// start audio_decode_only modules
	hd_audiodec_start(stream[0].dec_path);

	// let thread start to work
	stream[0].flow_start = 1;

	// destroy all thread
	pthread_join(stream[0].dec_thread_id, NULL);
	pthread_join(stream[0].feed_thread_id, NULL);

	// stop all modules
	hd_audiodec_stop(stream[0].dec_path);

exit:
	// close audio_record modules
	ret = close_module(&stream[0]);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
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

	//pthread_mutex_destroy(&lock);

	return 0;
}
