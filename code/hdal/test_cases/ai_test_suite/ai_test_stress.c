/**
	@brief Source file of vendor ai net test code.

	@file ai_test.c

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_stress.h"
#include "vendor_gfx.h"

/*-----------------------------------------------------------------------------*/
/* Global Data                                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/
void* cpu_stress_thread(void* args) {

	UINT32 i;
	CPU_STRESS_DATA *data = (CPU_STRESS_DATA*)args;

	if(!data){
		printf("cpu_stress_thread(): data is null\n");
		return NULL;
	}

	data->sleep_us = 10000;

	//printf("cpu stress level = %d\r\n", data->stress_level);

	switch (data->stress_level) {
		case 1:
			data->sleep_us = 30000;
			data->sleep_loop_count = 40000000;
		case 2:
			//45%
			data->sleep_loop_count = 30000000;
			break;
		case 3:
			//41%
			data->sleep_loop_count = 20000000;
			break;
		case 4:
			//35%
			data->sleep_loop_count = 10000000;
			break;
		case 5:
			//25%
			data->sleep_loop_count = 5000000;
			break;
		case 6:
			//15%
			data->sleep_loop_count = 3500000;
			break;
		default:
			//45%
			data->sleep_loop_count = 30000000;
			break;
	}

	while(1)
	{
		for (i = 0;i<data->sleep_loop_count;i++);
		usleep(data->sleep_us);
	}

	//printf("cpu thread exit\r\n");

	return NULL;
}

int start_cpu_stress_thread(CPU_STRESS_DATA *data)
{
	int ret;

	if(!data){
		printf("start_cpu_stress_thread(): data is null\n");
		return -1;
	}

	data->exit = 0;

	ret = pthread_create(&(data->thread_id), NULL, cpu_stress_thread, data);
	if (ret < 0) {
		printf("fail to create cpu stress thread\n");
		return -1;
	}

	return 0;
}

int stop_cpu_stress_thread(CPU_STRESS_DATA *data)
{
	if(!data){
		printf("stop_cpu_stress_thread(): data is null\n");
		return -1;
	}

	data->exit = 1;

	if(data->thread_id)
		pthread_cancel(data->thread_id);

	return 0;
}

void* dma_stress_thread(void* arg) {
	void                 *va1, *va2;
	UINT32               pa1, pa2;
	UINT32               size = 0x400000;
	HD_RESULT            ret;
	UINT32               ts, te;
	UINT32               total_size = 0;
	HD_GFX_COPY          param = {0};
	BOOL                 is_print = FALSE;
	DMA_STRESS_DATA*     data = (DMA_STRESS_DATA *)arg;

	data->copy_size = 0x500000;

	//printf("dma stress level = %d\r\n", data->stress_level);

	switch (data->stress_level) {
		case 1:
			//36%
			data->sleep_us = 10000;
			data->copy_times = 9;
			break;
		case 2:
			//30%
			data->sleep_us = 10000;
			data->copy_times = 5;
			break;
		case 3:
			//20%
			data->sleep_us = 20000;
			data->copy_times = 4;
			break;
		case 4:
			//10%
			data->sleep_us = 10000;
			data->copy_times = 1;
			break;
		default:
			//36%
			data->sleep_us = 10000;
			data->copy_times = 9;
			break;
	}

	if (NULL != data) {
		size = data->copy_size;
	}
	ret = hd_common_mem_alloc("test1", &pa1, (void **)&va1, size, data->ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, (int)data->ddr_id+1);
		return 0;
	}
	ret = hd_common_mem_alloc("test2", &pa2, (void **)&va2, size, data->ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, (int)data->ddr_id+1);
		return 0;
	}

	param.src_img.dim.w            = 4096;
	param.src_img.dim.h            = size/4096;
	param.src_img.format           = HD_VIDEO_PXLFMT_Y8;
	param.src_img.p_phy_addr[0]    = pa1;
	param.src_img.lineoffset[0]    = param.src_img.dim.w;
	param.dst_img.dim.w            = 4096;
	param.dst_img.dim.h            = param.src_img.dim.h;
	param.dst_img.format           = HD_VIDEO_PXLFMT_Y8;
	param.dst_img.p_phy_addr[0]    = pa2;
	param.dst_img.lineoffset[0]    = param.src_img.dim.w;
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = param.src_img.dim.w;
	param.src_region.h             = param.src_img.dim.h;
	param.dst_pos.x                = 0;
	param.dst_pos.y                = 0;
	param.colorkey                 = 0;
	param.alpha                    = 255;

	ts = hd_gettime_ms();
	//printf("src w = %d ,h = %d, linoff = %d\r\n", param.src_img.dim.w, param.src_img.dim.h, param.src_img.lineoffset[0]);
	//printf("dst w = %d ,h = %d, linoff = %d\r\n", param.dst_img.dim.w, param.dst_img.dim.h, param.dst_img.lineoffset[0]);
	//frame_count = 0;
	while (!data->exit) {
		for (UINT8 i = 0; i < data->copy_times; i++) {
			vendor_gfx_copy_no_flush(&param);

			total_size += size;
		}

		if (data->sleep_us) {
			usleep(data->sleep_us);
		}

		te = hd_gettime_ms();
		if (!is_print && te - ts > 1000) {
			//printf("total_size = 0x%x, time = %d ms, perf = %f MB/s\r\n", (unsigned int)total_size, te - ts, (float)total_size/(te - ts)/1024);
			//is_print = TRUE;
			ts = hd_gettime_ms();
			total_size = 0;
		}
	}
	ret = hd_common_mem_free(pa1, va1);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa1, (int)va1);
		return 0;
	}
	ret = hd_common_mem_free(pa2, va2);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa2, (int)va2);
		return 0;
	}
	return 0;
}

int start_dma_stress_thread(DMA_STRESS_DATA *data)
{
	int ret;

	if(!data){
		printf("start_dma_stress_thread(): data is null\n");
		return -1;
	}

	data->exit = 0;

	ret = pthread_create(&(data->thread_id), NULL, dma_stress_thread, data);
	if (ret < 0) {
		printf("fail to create dma stress thread\n");
		return -1;
	}

	return 0;
}

int stop_dma_stress_thread(DMA_STRESS_DATA *data)
{
	if(!data){
		printf("stop_dma_stress_thread(): data is null\n");
		return -1;
	}

	data->exit = 1;

	if(data->thread_id)
		pthread_join(data->thread_id, NULL);

	return 0;
}
