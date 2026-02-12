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
#include "ai_test_util.h"
#include "ai_test_config.h"
#include <time.h>
#include <sys/types.h>
#include <linux/limits.h>

#if defined(__LINUX)
#include <pthread.h>                    //for pthread API
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#endif

/*-----------------------------------------------------------------------------*/
/* Global Data                                                                 */
/*-----------------------------------------------------------------------------*/
typedef long long int num;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *input = NULL;
/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/

HD_RESULT ai_test_mem_get(MEM_PARM *mem_parm, UINT32 size, int sequential)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK      blk;
	int   i;

	if(size == 0){
		printf("allocating 0 bytes from nvtmpp is invalid\n");
		return -1;
	}

	for(i = 0 ; i < 5*60 ; ++i){//retry 5 minutes
		blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL + sequential, size, DDR_ID1);
		if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
			printf("hd_common_mem_get_block fail, wait 1 second and retry\n");
			sleep(1);
			continue;
		}else{
			break;
		}
	}
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail for 5 minutes, quit\n");
		return HD_ERR_NG;
	}

	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("hd_common_mem_blk2pa() fail\n");
		ret = hd_common_mem_release_block(blk);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		}
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);

	/* Release buffer */
	if (va == 0) {
		printf("hd_common_mem_mmap() fail\n");
		ret = hd_common_mem_release_block(blk);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		}
		return HD_ERR_NOMEM;
	}

	mem_parm->pa = pa;
	mem_parm->va = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk = blk;
	mem_parm->ddr_id = DDR_ID1;

	return HD_OK;
}

HD_RESULT ai_test_mem_rel(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (mem_parm->va) {
		ret = hd_common_mem_munmap((void *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;
	return HD_OK;
}

HD_RESULT ai_test_mem_alloc(MEM_PARM *mem_parm, CHAR* name, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	int   i;

	if(size == 0){
		printf("allocating 0 bytes from nvtmpp is invalid\n");
		return -1;
	}

	//alloc private pool
	for(i = 0 ; i < 5*60 ; ++i){//retry 5 minutes
		ret = hd_common_mem_alloc(name, &pa, (void**)&va, size, DDR_ID0);
		if (ret!= HD_OK) {
			ret = hd_common_mem_alloc(name, &pa, (void**)&va, size, DDR_ID1);
			if (ret!= HD_OK) {
				printf("hd_common_mem_alloc() fail, wait 1 second and retry\n");
				sleep(1);
				continue;
			}else{
				mem_parm->ddr_id = DDR_ID1;
				break;
			}
		}else{
			mem_parm->ddr_id = DDR_ID0;
			break;
		}
	}

	if (ret!= HD_OK) {
		printf("hd_common_mem_alloc() fail for 5 minutes, quit\n");
		return ret;
	}

	mem_parm->pa   = pa;
	mem_parm->va   = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk  = (UINT32)-1;

	return HD_OK;
}

HD_RESULT ai_test_mem_free(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	//free private pool
	ret =  hd_common_mem_free(mem_parm->pa, (void *)mem_parm->va);
	if (ret!= HD_OK) {
		return ret;
	}

	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;

	return HD_OK;
}

INT32 ai_test_mem_load(MEM_PARM *mem_parm, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;
	INT32 read_size = 0;

	fd = fopen(filename, "rb");

	if (!fd) {
		printf("cannot read %s\r\n", filename);
		read_size = -1;
		goto exit;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
		read_size = -1;
		goto exit;
	} else if ((INT32)mem_parm->size < size) {
		printf("buffer size(%ld) < file size(%ld)\r\n", mem_parm->size, size);
		read_size = -1;
		goto exit;
	} else {
		read_size = (INT32)fread((VOID *)mem_parm->va, 1, size, fd);
	}

	mem_parm->size = read_size;

	// we use cpu to read memory, which needs to deal cache flush.
	if(hd_common_mem_flush_cache((VOID *)mem_parm->va, mem_parm->size) != HD_OK) {
        printf("flush cache failed.\r\n");
    }

exit:
	if (fd) {
		fclose(fd);
	}

	return read_size;
}

INT32 ai_test_mem_save(MEM_PARM *mem_parm, UINT32 write_size, const CHAR *filename)
{
	FILE *fd;
	UINT32 size = 0;

	fd = fopen(filename, "wb");

	if (!fd) {
		printf("ERR: cannot open %s for write!\r\n", filename);
		return -1;
	}

	size = (INT32)fwrite((VOID *)mem_parm->va, 1, write_size, fd);
	if (size != write_size) {
		printf("ERR: write %s with size %ld < wanted %ld?\r\n", filename, size, write_size);
	}

	if (fd) {
		fclose(fd);
	}

	return size;
}


VOID ai_test_mem_fill(MEM_PARM *mem_parm, unsigned char data)
{
	if(!mem_parm)
		printf("mem_parm is null\n");
	else
		memset((void*)mem_parm->va, data, mem_parm->size);
}

VOID ai_test_lock_and_sync(VOID)
{
	pthread_mutex_lock(&lock);
}
VOID ai_test_unlock(VOID)
{
	pthread_mutex_unlock(&lock);
}

int ai_test_padding(char *src, int src_w, int src_h, int src_loff, int dst_w, int dst_h)
{
	int i;

	//printf("src_w(%d) src_h(%d), dst_w(%d) dst_h(%d)\n", src_w, src_h, dst_w, dst_h);

	if(!src || !src_w || !src_h || !src_loff){
		printf("src(%p) or src_w(%d) or src_h(%d) or src_loff(%d) is invalid\n", src, src_w, src_h, src_loff);
		return -1;
	}

	if(dst_w > src_w || dst_h > src_h){
		printf("dst_w(%d) dst_h(%d) should <= src_w(%d) src_h(%d)\n", dst_w, dst_h, src_w, src_h);
		return -1;
	}

	//right edge
	if(src_w > dst_w){
		for(i = 0 ; i < src_h ; i++)
			memset(&(src[i*src_loff + dst_w]), 0, src_w - dst_w);
	}

	//bottom edge
	if(src_h > dst_h){
		for(i = dst_h ; i < src_h ; i++)
			memset(&(src[i*src_loff]), 0, src_w);
	}

	if(hd_common_mem_flush_cache((VOID *)src, src_loff*src_h) != HD_OK) {
		printf("flush cache failed.\r\n");
		return -1;
	}

	return 0;
}

int ai_test_crop(char *src, int src_w, int src_h, int src_loff, char *dst, int dst_w, int dst_h)
{
	int i;

	if(!src || !src_w || !src_h || !src_loff){
		printf("src(%p) or src_w(%d) or src_h(%d) or src_loff(%d) is invalid\n", src, src_w, src_h, src_loff);
		return -1;
	}

	if(dst_w > src_w || dst_h > src_h){
		printf("dst_w(%d) dst_h(%d) should <= src_w(%d) src_h(%d)\n", dst_w, dst_h, src_w, src_h);
		return -1;
	}

	for(i = 0 ; i < dst_h ; i++) {
		memcpy(&(dst[i*dst_w]), &(src[i*src_loff]), dst_w);
	}

	if(hd_common_mem_flush_cache((VOID *)dst, dst_w*dst_h) != HD_OK) {
		printf("flush cache failed.\r\n");
		return -1;
	}

	return 0;
}

int readone(num *x) {
	int ret = 0;
	ret = fscanf(input, "%lld ", x);
	return ret;
}

int readunsigned(unsigned long long *x) {
	int ret = 0;
	ret = fscanf(input, "%llu ", x);
	return ret;
}

int readstr(char *x) {
	int ret = 0;
	ret = fscanf(input, "%s ", x);
	return ret;
}

int readchar(char *x) {
	int ret = 0;
	ret = fscanf(input, "%c ", x);
	return ret;
}

void cal_linux_cpu_loading(int loop_num, double *cpu_user, double *cpu_sys)
{
	pid_t process_id = getpid();
	char filePath[200];
	double pre_cpu_time = 0;
	double pre_utime = 0;
	double pre_stimev = 0;
	double cpu_avg_user = 0.0;
	double cpu_avg_sys = 0.0;
	double cpu_time = 0.0;
	double user_util = 0.0;
	double sys_util = 0.0;
	char cpu_str[20];
	long cpu_temp = 0;
	int i, j;
	num pid;
	char tcomm[PATH_MAX];
	char state;

	num ppid;
	num pgid;
	num sid;
	num tty_nr;
	num tty_pgrp;

	num flags;
	num min_flt;
	num cmin_flt;
	num maj_flt;
	num cmaj_flt;
	num utime;
	num stimev;

	num cutime;
	num cstime;
	num priority;
	num nicev;
	num num_threads;
	num it_real_value;

	unsigned long long start_time;

	num vsize;
	num rss;
	num rsslim;
	num start_code;
	num end_code;
	num start_stack;
	num esp;
	num eip;

	num pending;
	num blocked;
	num sigign;
	num sigcatch;
	num wchan;
	num zero1;
	num zero2;
	num exit_signal;
	num cpu;
	num rt_priority;
	num policy;
	FILE *cpu_ptr = NULL;


	if (loop_num < 0) {
		return;
	}
	if (cpu_user == NULL) {
		return;
	}
	if (cpu_sys == NULL) {
		return;
	}

	if (cpu_ptr == NULL) {
		cpu_ptr = fopen("/proc/stat", "r");
	}
	sprintf(filePath, "/proc/%d/stat", (int)process_id);
	if (input == NULL) {
		input = fopen(filePath, "r");
	}

	for (i = 0; i <= loop_num; ++i){//do num + 1 times
		fseek(cpu_ptr, 0, SEEK_SET);
		if (fscanf(cpu_ptr, "%s ", cpu_str) < 0) { 
			goto input_exit; 
		}
		for (j = 0; j < 9; ++j) {
			if (fscanf(cpu_ptr, "%lu ", &cpu_temp) < 0) { 
				goto input_exit; 
			}
			cpu_time += cpu_temp;
		}
		fseek(input, 0, SEEK_SET);
		if (readone(&pid) < 0) { goto input_exit; }
		if (readstr(tcomm) < 0) { goto input_exit; }
		if (readchar(&state) < 0) { goto input_exit; }
		if (readone(&ppid) < 0) { goto input_exit; }
		if (readone(&pgid) < 0) { goto input_exit; }
		if (readone(&sid) < 0) { goto input_exit; }
		if (readone(&tty_nr) < 0) { goto input_exit; }
		if (readone(&tty_pgrp) < 0) { goto input_exit; }
		if (readone(&flags) < 0) { goto input_exit; }
		if (readone(&min_flt) < 0) { goto input_exit; }
		if (readone(&cmin_flt) < 0) { goto input_exit; }
		if (readone(&maj_flt) < 0) { goto input_exit; }
		if (readone(&cmaj_flt) < 0) { goto input_exit; }
		if (readone(&utime) < 0) { goto input_exit; }
		if (readone(&stimev) < 0) { goto input_exit; }
		if (readone(&cutime) < 0) { goto input_exit; }
		if (readone(&cstime) < 0) { goto input_exit; }
		if (readone(&priority) < 0) { goto input_exit; }
		if (readone(&nicev) < 0) { goto input_exit; }
		if (readone(&num_threads) < 0) { goto input_exit; }
		if (readone(&it_real_value) < 0) { goto input_exit; }
		if (readunsigned(&start_time) < 0) { goto input_exit; }
		if (readone(&vsize) < 0) { goto input_exit; }
		if (readone(&rss) < 0) { goto input_exit; }
		if (readone(&rsslim) < 0) { goto input_exit; }
		if (readone(&start_code) < 0) { goto input_exit; }
		if (readone(&end_code) < 0) { goto input_exit; }
		if (readone(&start_stack) < 0) { goto input_exit; }
		if (readone(&esp) < 0) { goto input_exit; }
		if (readone(&eip) < 0) { goto input_exit; }
		if (readone(&pending) < 0) { goto input_exit; }
		if (readone(&blocked) < 0) { goto input_exit; }
		if (readone(&sigign) < 0) { goto input_exit; }
		if (readone(&sigcatch) < 0) { goto input_exit; }
		if (readone(&wchan) < 0) { goto input_exit; }
		if (readone(&zero1) < 0) { goto input_exit; }
		if (readone(&zero2) < 0) { goto input_exit; }
		if (readone(&exit_signal) < 0) { goto input_exit; }
		if (readone(&cpu) < 0) { goto input_exit; }
		if (readone(&rt_priority) < 0) { goto input_exit; }
		if (readone(&policy) < 0) { goto input_exit; }

		//printf("cpu time = %f, utime = %f, preutime = %f, sub = %f\r\n", cpu_time - pre_cpu_time, utime, pre_utime, utime - pre_utime);
		user_util = 100 * (utime - pre_utime) / (cpu_time - pre_cpu_time);
		sys_util = 100 * (stimev - pre_stimev) / (cpu_time - pre_cpu_time);
		//printf("cpu(user) = %f, cpu(system) = %f\r\n", user_util, sys_util);

		pre_utime = utime;
		pre_stimev = stimev;
		pre_cpu_time = cpu_time;
		cpu_avg_user += user_util;
		cpu_avg_sys += sys_util;
		sleep(3);
	}
	*cpu_user = cpu_avg_user / loop_num;
	*cpu_sys = cpu_avg_sys / loop_num;

input_exit:
	if (cpu_ptr) {
		fclose(cpu_ptr);
		cpu_ptr = NULL;
	}
	if (input) {
		fclose(input);
		input = NULL;
	}
	return;
}

int wait_for_loading_share_model(FIRST_LOOP_DATA *first_loop)
{
	GLOBAL_DATA *global = NULL;
	int i, ready = 0;
	
	global = ai_test_get_global_data();
	if(!global || !first_loop){
		printf("fail to get global data(%lx) or first_loop(%lx)\n", (unsigned long)global, (unsigned long)first_loop);
		return -1;
	}
	
	if(global->share_model == 0){
		return 0;
	}
	
	//tell other threads that I am waiting master to load model
	ai_test_lock_and_sync();
	global->wait_load_share_model_memory_thread_number++;
	ai_test_unlock();

	//wait all threads waiting master to load model
	//wait for 5 minutes
	for(i = 0 ; i < 30000 ; i++){
		
		//check if all threads are waiting master to load model
		ai_test_lock_and_sync();
		if(global->wait_load_share_model_memory_thread_number == global->first_loop_count){
			ready = 1;
			ai_test_unlock();
			break;
		}
		ai_test_unlock();

		usleep(1000*10);
	}
	
	//other threads are dead
	if(ready == 0){
		printf("waiting for loading share model timeout\n");
		return -1;
	}
	
	//if any thread reaches here, it means all threads are waiting master to load model
	ai_test_lock_and_sync();
	global->wait_load_share_model_memory_thread_number_2nd++;
	ai_test_unlock();

	//master should ensure other threads finish waiting
	if(first_loop->share_model_master == NULL){
		//wait for 5 minutes
		for(i = 0 ; i < 30000 ; i++){
			
			//check if all threads are waiting master to load model
			ai_test_lock_and_sync();
			if(global->wait_load_share_model_memory_thread_number_2nd == global->first_loop_count){
				global->wait_load_share_model_memory_thread_number = 0;
				global->wait_load_share_model_memory_thread_number_2nd = 0;
				ai_test_unlock();
				break;
			}
			ai_test_unlock();

			usleep(1000*10);
		}
	}
	
	return 0;
}

int syc_for_freeing_share_model(FIRST_LOOP_DATA *first_loop)
{
	GLOBAL_DATA *global = NULL;
	int i, ready = 0;
	
	global = ai_test_get_global_data();
	if(!global || !first_loop){
		printf("fail to get global data(%lx) or first_loop(%lx)\n", (unsigned long)global, (unsigned long)first_loop);
		return -1;
	}
	
	if(global->share_model == 0){
		return 0;
	}
	
	//tell other threads that I agree to release model memory
	ai_test_lock_and_sync();
	global->wait_free_share_model_memory_thread_number++;
	ai_test_unlock();

	//wait all threads agree to release model memory
	//wait for 5 minutes
	for(i = 0 ; i < 30000 ; i++){
		
		//check if all threads agree to release model memory
		ai_test_lock_and_sync();
		if(global->wait_free_share_model_memory_thread_number == global->first_loop_count){
			ready = 1;
			ai_test_unlock();
			break;
		}
		ai_test_unlock();

		usleep(1000*10);
	}
	
	//other threads are dead
	if(ready == 0){
		printf("waiting for freeing share model timeout\n");
		return -1;
	}
	
	//if any thread reaches here, it means all threads agree to release model memory
	ai_test_lock_and_sync();
	global->wait_free_share_model_memory_thread_number_2nd++;
	ai_test_unlock();

	//master should ensure other threads finish waiting
	if(first_loop->share_model_master == NULL){
		//wait for 5 minutes
		for(i = 0 ; i < 30000 ; i++){
			
			//check if all threads are waiting master to load model
			ai_test_lock_and_sync();
			if(global->wait_free_share_model_memory_thread_number_2nd == global->first_loop_count){
				global->wait_free_share_model_memory_thread_number = 0;
				global->wait_free_share_model_memory_thread_number_2nd = 0;
				ai_test_unlock();
				break;
			}
			ai_test_unlock();

			usleep(1000*10);
		}
	}
	
	return 0;
}
