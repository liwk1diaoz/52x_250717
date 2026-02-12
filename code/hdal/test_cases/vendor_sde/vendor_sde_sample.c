
#include <string.h>
#include <stdlib.h>

#include "hdal.h"
#include "hd_debug.h"

#include "vendor_sde.h"

#if defined(__LINUX)
#include <pthread.h> //for pthread API
#define MAIN(argc, argv) int main(int argc, char** argv)
#define GETCHAR() getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h> //for sleep API
#define sleep(x) vos_util_delay_ms(1000*(x))
#define msleep(x) vos_util_delay_ms(x)
#define usleep(x) vos_util_delay_us(x)
#include <kwrap/examsys.h> //for MAIN(), GETCHAR() API
#define MAIN(argc, argv) EXAMFUNC_ENTRY(pq_video_rtsp, argc, argv)
#define GETCHAR() NVT_EXAMSYS_GETCHAR()
#endif

#define MEASURE_TIME 0
#if (MEASURE_TIME)
#include "hd_common.h"
#include "hd_util.h"
#endif

typedef struct _SAMPLE_SDE_BUFFER {
	HD_COMMON_MEM_VB_BLK buf_blk;
	UINT32               buf_size;
	UINT32               buf_pa;
	void                 *buf_va;
} SAMPLE_SDE_BUFFER;

//============================================================================
// global
//============================================================================
pthread_t sde_thread_id;
static UINT32 conti_run = 1;
static SAMPLE_SDE_BUFFER sde_in_0, sde_in_1, sde_cost, sde_out_0, sde_out_1;
static SDET_TRIG_INFO trig = {0};
static SDET_IO_INFO io_info = {0};

static INT32 get_choose_int(void)
{
	char buf[256];
	INT val, rt;

	rt = scanf("%d", &val);

	if (rt != 1) {
		printf("Invalid option. Try again.\n");
		clearerr(stdin);
		fgets(buf, sizeof(buf), stdin);
		val = -1;
	}

	return val;
}

static HD_RESULT mem_init(void)
{
	UINT32 max_width = 1500;
	UINT32 max_height = 720;
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// *************************************************
	// ** register sde's buffer to common memory pool **
	// *************************************************
	// input0
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;
	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = max_width * max_height;                 // io_info.io_info.in0_lofs * io_info.io_info.height;
	mem_cfg.pool_info[0].blk_cnt  = 1;
	// input1
	mem_cfg.pool_info[1].ddr_id   = DDR_ID0;
	mem_cfg.pool_info[1].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = max_width * max_height;                 // io_info.io_info.in1_lofs * io_info.io_info.height;
	mem_cfg.pool_info[1].blk_cnt  = 1;
	// cost
	mem_cfg.pool_info[2].ddr_id   = DDR_ID0;
	mem_cfg.pool_info[2].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = (max_width * 64 * 6 / 8) * max_height; // io_info.io_info.cost_lofs * io_info.io_info.height;
	mem_cfg.pool_info[2].blk_cnt  = 1;
	// output0
	mem_cfg.pool_info[3].ddr_id   = DDR_ID0;
	mem_cfg.pool_info[3].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[3].blk_size = max_width * max_height;                 // io_info.io_info.out0_lofs * io_info.io_info.height;
	mem_cfg.pool_info[3].blk_cnt  = 1;
	// output0
	mem_cfg.pool_info[4].ddr_id   = DDR_ID0;
	mem_cfg.pool_info[4].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[4].blk_size = max_width * max_height;                 // io_info.io_info.out1_lofs * io_info.io_info.height;
	mem_cfg.pool_info[4].blk_cnt  = 1;

	// register sde's buffer to common memory pool
	ret = hd_common_mem_init(&mem_cfg);
	if(ret != HD_OK){
		printf("fail to allocate sde total buffer from common pool\n");
		return ret;
	}

	return HD_OK;
}

static HD_RESULT mem_get(void)
{
	// *************************************************
	// **            get sde's buffer block           **
	// *************************************************
	// get sde's in_0 buffer block
	sde_in_0.buf_size = io_info.io_info.in0_lofs * io_info.io_info.height;
	sde_in_0.buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, sde_in_0.buf_size, DDR_ID0);
	if (sde_in_0.buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get in_0 block (size = %d) fail\r\n", sde_in_0.buf_size);
		return HD_ERR_NOMEM;
	}
	//translate sde's in_0 buffer block to physical address
	sde_in_0.buf_pa = hd_common_mem_blk2pa(sde_in_0.buf_blk);
	if (sde_in_0.buf_pa == 0) {
		printf("blk2pa fail, in_0 buf_blk = 0x%x\r\n", sde_in_0.buf_blk);
		return HD_ERR_NOMEM;
	}

	// get sde's in_1 buffer block
	sde_in_1.buf_size = io_info.io_info.in1_lofs * io_info.io_info.height;
	sde_in_1.buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, sde_in_1.buf_size, DDR_ID0);
	if (sde_in_1.buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get in_1 block (size = %d) fail\r\n", sde_in_1.buf_size);
		return HD_ERR_NOMEM;
	}
	//translate sde's in_1 buffer block to physical address
	sde_in_1.buf_pa = hd_common_mem_blk2pa(sde_in_1.buf_blk);
	if (sde_in_1.buf_pa == 0) {
		printf("blk2pa fail, in_1 buf_blk = 0x%x\r\n", sde_in_1.buf_blk);
		return HD_ERR_NOMEM;
	}

	// get sde's cost buffer block
	sde_cost.buf_size = io_info.io_info.cost_lofs * io_info.io_info.height;
	sde_cost.buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, sde_cost.buf_size, DDR_ID0);
	if (sde_cost.buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get cost block (size = %d) fail\r\n", sde_cost.buf_size);
		return HD_ERR_NOMEM;
	}
	//translate sde's cost buffer block to physical address
	sde_cost.buf_pa = hd_common_mem_blk2pa(sde_cost.buf_blk);
	if (sde_cost.buf_pa == 0) {
		printf("blk2pa fail, cost buf_blk = 0x%x\r\n", sde_cost.buf_blk);
		return HD_ERR_NOMEM;
	}

	// get sde's out_0 buffer block
	sde_out_0.buf_size = io_info.io_info.out0_lofs * io_info.io_info.height;
	sde_out_0.buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, sde_out_0.buf_size, DDR_ID0);
	if (sde_out_0.buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get out_0 block (size = %d) fail\r\n", sde_out_0.buf_size);
		return HD_ERR_NOMEM;
	}
	//translate sde's out_0 buffer block to physical address
	sde_out_0.buf_pa = hd_common_mem_blk2pa(sde_out_0.buf_blk);
	if (sde_out_0.buf_pa == 0) {
		printf("blk2pa fail, out_0 buf_blk = 0x%x\r\n", sde_out_0.buf_blk);
		return HD_ERR_NOMEM;
	}

	// get sde's out_1 buffer block
	sde_out_1.buf_size = io_info.io_info.out1_lofs * io_info.io_info.height;
	sde_out_1.buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, sde_out_1.buf_size, DDR_ID0);
	if (sde_out_1.buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get out_1 block (size = %d) fail\r\n", sde_out_1.buf_size);
		return HD_ERR_NOMEM;
	}
	//translate sde's out_1 buffer block to physical address
	sde_out_1.buf_pa = hd_common_mem_blk2pa(sde_out_1.buf_blk);
	if (sde_out_1.buf_pa == 0) {
		printf("blk2pa fail, out_1 buf_blk = 0x%x\r\n", sde_out_1.buf_blk);
		return HD_ERR_NOMEM;
	}

	return HD_OK;
}

static HD_RESULT load_input_image(void)
{
	int fd;
	int len;
	HD_RESULT ret;

	ret = mem_get();
	if (ret != HD_OK) {
		printf("mem_get fail=%d\n", ret);
		return HD_ERR_NOMEM;
	}

	//map sde's in_0 buffer from physical address to user space
	sde_in_0.buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, sde_in_0.buf_pa, sde_in_0.buf_size);
	if (!sde_in_0.buf_va) {
		printf("hd_common_mem_mmap() in0_va fail\n");
		return -1;
	}

	//load in_0 from sd card
	fd = open("/mnt/sd/sde_test/in_0.yuv", O_RDONLY);
	if (fd == -1){
		printf("fail to open /mnt/sd/sde_test/in_0.yuv\n");
		return HD_ERR_NOT_FOUND;
	}

	len = read(fd, (void*)sde_in_0.buf_va, sde_in_0.buf_size);
	close(fd);
	if (len != (int)sde_in_0.buf_size){
		printf("fail to read /mnt/sd/sde_test/in_0.yuv\n");
		return HD_ERR_SYS;
	}

	//map sde's in_1 buffer from physical address to user space
	sde_in_1.buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, sde_in_1.buf_pa, sde_in_1.buf_size);
	if (!sde_in_1.buf_va) {
		printf("hd_common_mem_mmap() in1_va fail\n");
		return -1;
	}

	//load in_1 from sd card
	fd = open("/mnt/sd/sde_test/in_1.yuv", O_RDONLY);
	if (fd == -1){
		printf("fail to open /mnt/sd/sde_test/in_1.yuv\n");
		return HD_ERR_NOT_FOUND;
	}

	len = read(fd, (void*)sde_in_1.buf_va, sde_in_1.buf_size);
	close(fd);
	if (len != (int)sde_in_1.buf_size){
		printf("fail to read /mnt/sd/sde_test/in_1.yuv\n");
		return HD_ERR_SYS;
	}

	//map sde's cost buffer from physical address to user space
	sde_cost.buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, sde_cost.buf_pa, sde_cost.buf_size);
	if (!sde_cost.buf_va) {
		printf("hd_common_mem_mmap() in1_va fail\n");
		return -1;
	}

	//load cost from sd card
	fd = open("/mnt/sd/sde_test/cost.yuv", O_RDONLY);
	if (fd == -1){
		printf("fail to open /mnt/sd/sde_test/cost.yuv\n");
		return HD_ERR_NOT_FOUND;
	}

	len = read(fd, (void*)sde_cost.buf_va, sde_cost.buf_size);
	close(fd);
	if (len != (int)sde_cost.buf_size){
		printf("fail to read /mnt/sd/sde_test/cost.yuv\n");
		return HD_ERR_SYS;
	}

	return HD_OK;
}

static HD_RESULT write_output_image(void)
{
	int fd;
	int len;

	#if 1
	//map sde's cost buffer from physical address to user space
	sde_cost.buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, sde_cost.buf_pa, sde_cost.buf_size);
	if (!sde_cost.buf_va) {
		printf("hd_common_mem_mmap() out0_va fail\n");
		return -1;
	}

	//flush
	hd_common_mem_flush_cache(sde_cost.buf_va, sde_cost.buf_size);

	//save the result disparity to sd card
	fd = open("/mnt/sd/sde_test/cost.yuv", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/sde_test/cost.yuv\n");
		return HD_ERR_SYS;
	}
	
	len = write(fd, (void*)sde_cost.buf_va, sde_cost.buf_size);
	close(fd);
	if(len != (int)sde_cost.buf_size){
		printf("fail to write /mnt/sd/sde_test/cost.yuv (%d, %d)\n", len, sde_cost.buf_size);
		return HD_ERR_SYS;
	}
	printf("result is /mnt/sd/sde_test/cost.yuv\n");
	#endif

	//map sde's out_0 buffer from physical address to user space
	sde_out_0.buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, sde_out_0.buf_pa, sde_out_0.buf_size);
	if (!sde_out_0.buf_va) {
		printf("hd_common_mem_mmap() out0_va fail\n");
		return -1;
	}

	//flush
	hd_common_mem_flush_cache(sde_out_0.buf_va, sde_out_0.buf_size);

	//save the result disparity to sd card
	fd = open("/mnt/sd/sde_test/out_0.yuv", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/sde_test/out_0.yuv\n");
		return HD_ERR_SYS;
	}
	
	len = write(fd, (void*)sde_out_0.buf_va, sde_out_0.buf_size);
	close(fd);
	if(len != (int)sde_out_0.buf_size){
		printf("fail to write /mnt/sd/sde_test/out_0.yuv\n");
		return HD_ERR_SYS;
	}
	printf("result is /mnt/sd/sde_test/out_0.yuv\n");

	//map sde's out_1 buffer from physical address to user space
	sde_out_1.buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, sde_out_1.buf_pa, sde_out_1.buf_size);
	if (!sde_out_1.buf_va) {
		printf("hd_common_mem_mmap() out1_va fail\n");
		return -1;
	}

	//flush
	hd_common_mem_flush_cache(sde_out_1.buf_va, sde_out_1.buf_size);

	//save the result confidence to sd card
	fd = open("/mnt/sd/sde_test/out_1.yuv", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/sde_test/out_1.yuv\n");
		return HD_ERR_SYS;
	}
	
	len = write(fd, (void*)sde_out_1.buf_va, sde_out_1.buf_size);
	close(fd);
	if(len != (int)sde_out_1.buf_size){
		printf("fail to write /mnt/sd/sde_test/out_1.yuv\n");
		return HD_ERR_SYS;
	}
	printf("result is /mnt/sd/sde_test/out_1.yuv\n");

	return HD_OK;
}

static void *sde_thread(void *arg)
{
	while (conti_run) {
		vendor_sde_set_cmd(SDET_ITEM_TRIGGER, &trig);
	};

	return 0;
}

int main(int argc, char *argv[])
{
	char *chip_name = getenv("NVT_CHIP_ID");
	INT32 option;
	UINT32 run = 1;
	UINT32 version = 0;
	SDET_CTRL_PARAM ctrl = {0};
	int align_byte = 4;
	UINT32 t0, t1;
	HD_RESULT ret;

	if (!((chip_name != NULL) && (strcmp(chip_name, "CHIP_NA51084") == 0))) {
		printf("SDE only support on NT98528!! \n");
		goto exit;
	}

	ret = hd_common_init(0);
	if(ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		goto exit;
	}

	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem_init fail=%d\n", ret);
		goto exit;
	}

	ret = vendor_sde_init();
	if (ret != HD_OK) {
		printf("vendor_sde_init fail=%d\n", ret);
		goto exit;
	}

	while (run) {
			printf("----------------------------------------\n");
			printf("  1. Get version \n");
			printf("  4. Open \n");
			printf("  5. Close \n");
			printf("  6. Trigger \n");
			printf("  7. Continue Trigger \n");
			printf(" 10. Get io_info \n");
			printf(" 11. Get ctrl \n");
			printf(" 60. Set io_info \n");
			printf(" 61. Set ctrl \n");
			printf("  0. Quit\n");
			printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			vendor_sde_get_cmd(SDET_ITEM_VERSION, &version);
			printf("version = 0x%X \n", version);
			break;

		case 4:
			vendor_sde_set_cmd(SDET_ITEM_OPEN, NULL);
			break;

		case 5:
			vendor_sde_set_cmd(SDET_ITEM_CLOSE, NULL);
			break;

		case 6:
			do {
				printf("Set sde id (0, 1)>> \n");
				trig.id = (UINT32)get_choose_int();
			} while (0);
			t0 = hd_gettime_us();
			vendor_sde_set_cmd(SDET_ITEM_TRIGGER, &trig);
			t1 = hd_gettime_us();
			printf("process time (%d) us!! \r\n", t1 - t0);

			write_output_image();
			break;

		case 7:
			do {
				printf("Set sde id (0, 1)>> \n");
				trig.id = (UINT32)get_choose_int();
			} while (0);

			ret = pthread_create(&sde_thread_id, NULL, sde_thread, NULL);
			if (ret < 0) {
				printf("create ir thread failed");
				goto exit;
			}

			do {
				printf("Enter 0 to exit >> \n");
				conti_run = (UINT32)get_choose_int();
			} while (0);

			write_output_image();

			// destroy encode thread
			pthread_join(sde_thread_id, NULL);
			break;

		case 10:
			do {
				printf("Set sde id (0, 1)>> \n");
				io_info.id = (UINT32)get_choose_int();
			} while (0);

			ret = vendor_sde_get_cmd(SDET_ITEM_IO_INFO, &io_info);
			if (ret == HD_OK) {
				printf("io_info id = %d \n", io_info.id);
				printf("io_info width = %d, \n", io_info.io_info.width);
				printf("io_info height = %d, \n", io_info.io_info.height);
				printf("io_info in0_addr = 0x%x, \n", io_info.io_info.in0_addr);
				printf("io_info in1_addr = 0x%x, \n", io_info.io_info.in1_addr);
				printf("io_info cost_addr = 0x%x, \n", io_info.io_info.cost_addr);
				printf("io_info out0_addr = 0x%x, \n", io_info.io_info.out0_addr);
				printf("io_info out1_addr = 0x%x, \n", io_info.io_info.out1_addr);
				printf("io_info in0_lofs = %d, \n", io_info.io_info.in0_lofs);
				printf("io_info in1_lofs = %d, \n", io_info.io_info.in1_lofs);
				printf("io_info cost_lofs = %d, \n", io_info.io_info.cost_lofs);
				printf("io_info out0_lofs = %d, \n", io_info.io_info.out0_lofs);
				printf("io_info out1_lofs = %d, \n", io_info.io_info.out1_lofs);
			}
			break;

		case 11:
			do {
				printf("Set sde id (0, 1)>> \n");
				ctrl.id = (UINT32)get_choose_int();
			} while (0);

			ret = vendor_sde_get_cmd(SDET_ITEM_CTRL_PARAM, &ctrl);
			if (ret == HD_OK) {
				printf("ctrl id = %d \n", ctrl.id);
				printf("ctrl disp_val_mode = %d, \n", ctrl.ctrl.disp_val_mode);
				printf("ctrl disp_op_mode = %d, \n", ctrl.ctrl.disp_op_mode);
				printf("ctrl disp_inv_sel = %d, \n", ctrl.ctrl.disp_inv_sel);
				printf("ctrl conf_en = %d, \n", ctrl.ctrl.conf_en);
			}
			break;

		case 60:
			do {
				printf("Set sde id (0, 1)>> \n");
				io_info.id = (UINT32)get_choose_int();

				printf("Set input width >> \n");
				io_info.io_info.width = (UINT32)get_choose_int();
				printf("Set input height >> \n");
				io_info.io_info.height = (UINT32)get_choose_int();;
				#if 0
				printf("Set input0 line_offset >> \n");
				io_info.io_info.in0_lofs = ALIGN_CEIL((UINT32)get_choose_int(), align_byte);
				printf("Set input1 line_offset >> \n");
				io_info.io_info.in1_lofs = ALIGN_CEIL((UINT32)get_choose_int(), align_byte);
				printf("Set cost line_offset >> \n");
				io_info.io_info.cost_lofs = ALIGN_CEIL((UINT32)get_choose_int(), align_byte);
				printf("Set output0 line_offset >> \n");
				io_info.io_info.out0_lofs = ALIGN_CEIL((UINT32)get_choose_int(), align_byte);
				printf("Set output1 line_offset >> \n");
				io_info.io_info.out1_lofs = ALIGN_CEIL((UINT32)get_choose_int(), align_byte);
				#else
				io_info.io_info.in0_lofs = ALIGN_CEIL(io_info.io_info.width, align_byte);
				io_info.io_info.in1_lofs = ALIGN_CEIL(io_info.io_info.width, align_byte);
				io_info.io_info.cost_lofs = ALIGN_CEIL(io_info.io_info.width * 64 * 6 / 8, align_byte);
				io_info.io_info.out0_lofs = ALIGN_CEIL(io_info.io_info.width, align_byte);
				io_info.io_info.out1_lofs = ALIGN_CEIL(io_info.io_info.width, align_byte);
				#endif
			} while (0);

			load_input_image();

			io_info.io_info.in0_addr = (UINT32)sde_in_0.buf_pa;
			io_info.io_info.in1_addr = (UINT32)sde_in_1.buf_pa;
			io_info.io_info.cost_addr = (UINT32)sde_cost.buf_pa;
			io_info.io_info.out0_addr = (UINT32)sde_out_0.buf_pa;
			io_info.io_info.out1_addr = (UINT32)sde_out_1.buf_pa;

			vendor_sde_set_cmd(SDET_ITEM_IO_INFO, &io_info);

			printf("io_info id = %d \n", io_info.id);
			printf("io_info width = %d, \n", io_info.io_info.width);
			printf("io_info height = %d, \n", io_info.io_info.height);
			printf("io_info in0_addr(pa) = 0x%x\n", io_info.io_info.in0_addr);
			printf("io_info in1_addr(pa) = 0x%x\n", io_info.io_info.in1_addr);
			printf("io_info cost_addr(pa) = 0x%x\n", io_info.io_info.cost_addr);
			printf("io_info out0_addr(pa) = 0x%x\n", io_info.io_info.out0_addr);
			printf("io_info out1_addr(pa) = 0x%x\n", io_info.io_info.out1_addr);
			printf("io_info in0_lofs = %d, \n", io_info.io_info.in0_lofs);
			printf("io_info in1_lofs = %d, \n", io_info.io_info.in1_lofs);
			printf("io_info cost_lofs = %d, \n", io_info.io_info.cost_lofs);
			printf("io_info out0_lofs = %d, \n", io_info.io_info.out0_lofs);
			printf("io_info out1_lofs = %d, \n", io_info.io_info.out1_lofs);
			break;

		case 61:
			do {
				printf("Set sde id (0, 1)>> \n");
				ctrl.id = (UINT32)get_choose_int();
			} while (0);
			
			ctrl.ctrl.disp_val_mode = 0;
			ctrl.ctrl.disp_op_mode = 0;
			ctrl.ctrl.disp_inv_sel = 0;
			ctrl.ctrl.conf_en = 0;
			vendor_sde_set_cmd(SDET_ITEM_CTRL_PARAM, &ctrl);
			break;

		case 0:
		default:
			goto exit;
			break;
		}
	}

exit:
	ret = vendor_sde_uninit();
	if (ret != HD_OK) {
		printf("vendor_sde_uninit fail=%d\n", ret);
	}

	if (sde_in_0.buf_blk) {
		if(HD_OK != hd_common_mem_release_block(sde_in_0.buf_blk)) {
			printf("hd_common_mem_release_block sde_in_0() fail\n");
		}
	}
	if (sde_in_1.buf_blk) {
		if(HD_OK != hd_common_mem_release_block(sde_in_1.buf_blk)) {
			printf("hd_common_mem_release_block sde_in_1() fail\n");
		}
	}
	if (sde_cost.buf_blk) {
		if(HD_OK != hd_common_mem_release_block(sde_cost.buf_blk)) {
			printf("hd_common_mem_release_block sde_cost() fail\n");
		}
	}
	if (sde_out_0.buf_blk) {
		if(HD_OK != hd_common_mem_release_block(sde_out_0.buf_blk)) {
			printf("hd_common_mem_release_block sde_out_0() fail\n");
		}
	}
	if (sde_out_1.buf_blk) {
		if(HD_OK != hd_common_mem_release_block(sde_out_1.buf_blk)) {
			printf("hd_common_mem_release_block sde_out_1() fail\n");
		}
	}

	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("hd_common_mem_uninit fail=%d\n", ret);
	}

	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("hd_common_uninit fail=%d\n", ret);
	}

	return 0;
}

