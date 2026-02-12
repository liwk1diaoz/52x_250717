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


#include <string.h>
#include <sys/time.h>
#include <kwrap/examsys.h>

#define HD_KEYINPUT_SIZE 256

char cmd[HD_KEYINPUT_SIZE];
const char delimiters[] = {' ', 0x0A, 0x0D, 0x08, '\0'}; //0x0D=\n, 0x0A=\r
char *current_cmd = 0;

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

#else
/*-----------------------------------------------------------------------------*/
/* The code is copy from samples\alg_net_test_linux_cpu_loading                */
/*-----------------------------------------------------------------------------*/
#include <time.h>
#include <sys/types.h>
#include <linux/limits.h>

typedef long long int num;

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

long tickspersec;

FILE *input, *cpu_ptr;

void readone(num *x) { fscanf(input, "%lld ", x); }
void readunsigned(unsigned long long *x) { fscanf(input, "%llu ", x); }
void readstr(char *x) {  fscanf(input, "%s ", x);}
void readchar(char *x) {  fscanf(input, "%c ", x);}

void cal_linux_cpu_loading(int loop_num, double *cpu_user, double *cpu_sys) {
	//printf("1. cal_linux_cpu_loading\r\n");
	pid_t process_id = getpid();
	//printf("2. cal_linux_cpu_loading\r\n");
	tickspersec = sysconf(_SC_CLK_TCK);
	//printf("3. cal_linux_cpu_loading\r\n");
	char filePath[200];
	sprintf(filePath, "/proc/%d/stat", (int)process_id);

	input = NULL;
	cpu_ptr = NULL;

	double pre_cpu_time = 0;
	double pre_utime = 0;
	double pre_stimev = 0;

	double cpu_avg_user = 0.0;
	double cpu_avg_sys = 0.0;
	for(int i = 0; i <= loop_num; ++i){//do num + 1 times
		//printf("read proc/stat\r\n");
		cpu_ptr = fopen("/proc/stat", "r");  

		char cpu_str[20];
		fscanf(cpu_ptr, "%s ", cpu_str);
		double cpu_time = 0;
		long cpu_temp = 0;
		//printf("read proc/stat cpu_tmp\r\n");
		for(int i = 0; i < 9; ++i)
		{
		  fscanf(cpu_ptr, "%lu ", &cpu_temp);
		  cpu_time += cpu_temp;
		}
		fclose(cpu_ptr);

		//printf("read %s\r\n", filePath);
		input = fopen(filePath, "r");
		readone(&pid);
		readstr(tcomm);
		readchar(&state);
		readone(&ppid);
		readone(&pgid);
		readone(&sid);
		readone(&tty_nr);
		readone(&tty_pgrp);
		readone(&flags);
		readone(&min_flt);
		readone(&cmin_flt);
		readone(&maj_flt);
		readone(&cmaj_flt);
		readone(&utime);
		readone(&stimev);
		readone(&cutime);
		readone(&cstime);
		readone(&priority);
		readone(&nicev);
		readone(&num_threads);
		readone(&it_real_value);
		readunsigned(&start_time);
		readone(&vsize);
		readone(&rss);
		readone(&rsslim);
		readone(&start_code);
		readone(&end_code);
		readone(&start_stack);
		readone(&esp);
		readone(&eip);
		readone(&pending);
		readone(&blocked);
		readone(&sigign);
		readone(&sigcatch);
		readone(&wchan);
		readone(&zero1);
		readone(&zero2);
		readone(&exit_signal);
		readone(&cpu);
		readone(&rt_priority);
		readone(&policy);
		fclose(input);

		//printf("cpu time = %f, utime = %f, preutime = %f, sub = %f\r\n", cpu_time - pre_cpu_time, utime, pre_utime, utime - pre_utime);
		double user_util = 100 * (utime - pre_utime) / (cpu_time - pre_cpu_time);
		double sys_util = 100 * (stimev - pre_stimev) / (cpu_time - pre_cpu_time);
		//printf("cpu(user) = %f, cpu(system) = %f\r\n", user_util, sys_util);

		pre_utime = utime;
		pre_stimev = stimev;
		pre_cpu_time = cpu_time;
		sleep(3);
		if(i != 0){
			cpu_avg_user += user_util;
			cpu_avg_sys += sys_util;
		}
	}
	*cpu_user = cpu_avg_user / loop_num;
	*cpu_sys = cpu_avg_sys / loop_num;
}

//---------------------------------------------------------------------------------
#endif
static UINT32 my_read_decimal_key_input(const CHAR *comment)
{
	int     radix = 0;
	char    *ret = NULL;
	char    *symbol = NULL;
	UINT32  value = 0;

	//CHKPNT;

	while (value == 0) {

		//CHKPNT;
		if (current_cmd == 0) {  //buffer is empty?
		
		//CHKPNT;
			fflush(stdin);
			printf("%s: ", comment);
			do {
				ret = NVT_EXAMSYS_FGETS(cmd, sizeof(cmd), stdin);
				
			} while (NULL == ret || cmd[0] == ' ' || cmd[0] == '\n') ;

			//CHKPNT;
			current_cmd = cmd;
			//printf ("get buffer = %s\r\n", current_cmd);
		}
		
		//CHKPNT;
		symbol = strsep(&current_cmd, delimiters); //separate 1 symbol from buffer
		
		if (symbol == NULL) {
			current_cmd = 0; //clear buffer
			//printf ("get symbol = (empty)\r\n");
			//printf ("now buffer = (empty)\r\n");
			value = 0;
			//printf ("value = %d\r\n", value);
		} else {
			//unsigned int i=0;
			//printf ("get symbol = %s\r\n", symbol);
			//printf ("get len = %d\r\n", strlen(symbol));
			//for(i=0;i<strlen(symbol);i++) {
			//	printf (" CHAR[%d] = %d\r\n", i, symbol[i]);
			//}
			//printf ("now buffer = %s\r\n", current_cmd);
			//CHKPNT;
			if (!strncmp(symbol, "0x", 2)) {
				radix = 16;
			} else {
				radix = 10;
			}
			value = (strtoul(symbol, (char **) NULL, radix));
			//printf ("value = %d\r\n", value);
			if (value == 0) {
				//CHKPNT;
				current_cmd = 0; //clear buffer
			} else {
				//value += 256;
			}
		}
	}

	return value;
}


/********************************************************************
	DEBUG MENU IMPLEMENTATION
********************************************************************/
static void my_debug_menu_print_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	printf("\n==============================");
	printf("\n %s", p_title);
	printf("\n------------------------------");

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->b_enable) {
			printf("\n %02d : %s", p_menu->menu_id, p_menu->p_name);
		}
		p_menu++;
	}

	printf("\n------------------------------");
	printf("\n %02d : %s", HD_DEBUG_MENU_ID_QUIT, "Quit");
	printf("\n %02d : %s", HD_DEBUG_MENU_ID_RETURN, "Return");
	printf("\n------------------------------\n");
}

static HD_DBG_MENU *_cur_menu = NULL;

const char* my_debug_get_menu_name(void)
{
	if (_cur_menu == NULL) return "x";
	else return _cur_menu->p_name;
}

static int my_debug_menu_exec_p(int menu_id, HD_DBG_MENU *p_menu)
{
	if (menu_id == HD_DEBUG_MENU_ID_RETURN) {
		return 0; // return 0 for return upper menu
	}

	if (menu_id == HD_DEBUG_MENU_ID_QUIT) {
		return -1; // return -1 to notify upper layer to quit
	}

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->menu_id == menu_id && p_menu->b_enable) {
			printf("Run: %02d : %s\r\n", p_menu->menu_id, p_menu->p_name);
			if (p_menu->p_func) {
				int r;
				_cur_menu = p_menu;
				r = p_menu->p_func();
				_cur_menu = NULL;
				return r;
			} else {
				printf("ERR: null function for menu id = %d\n", menu_id);
				return 0; // just skip
			}
		}
		p_menu++;
	}

	printf("ERR: cannot find menu id = %d\n", menu_id);
	return 0;
}

int my_debug_menu_entry_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	int menu_id = 0;
	//int show_menu = 1;

	do {
		//if (show_menu) {
			my_debug_menu_print_p(p_menu, p_title);
		//}
		menu_id = (int)my_read_decimal_key_input("");
		/*
		if (menu_id > 256) {
			show_menu = 0;
			menu_id -= 256;
		} else {
			show_menu = 1;
		}*/
		
		if (my_debug_menu_exec_p(menu_id, p_menu) == -1) {
			return -1; //quit
		}
	} while (menu_id != HD_DEBUG_MENU_ID_RETURN);

	return 0;
}




/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
HD_RESULT ai_test_mem_get(MEM_PARM *mem_parm, UINT32 size, UINT32 id)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_VB_BLK      blk;

	if (size == 0) {
		printf("mem_alloc fail, size = 0\r\n");
		ret = HD_ERR_NG;
		goto exit;
	}
	mem_parm->size = size;
	printf("ai_test_mem_get: size(%lu), pool_type(%lu)\r\n",size,HD_COMMON_MEM_USER_DEFINIED_POOL + id);
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL + id, mem_parm->size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		ret = HD_ERR_NG;
		goto exit;
	}
	mem_parm->pa = hd_common_mem_blk2pa(blk);
	if (mem_parm->pa == 0) {
		printf("hd_common_mem_blk2pa fail, blk = %#lx\r\n", blk);
		hd_common_mem_release_block(blk);
		ret = HD_ERR_NG;
		goto exit;
	}
	mem_parm->va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, mem_parm->pa, mem_parm->size);
	if (mem_parm->va == 0) {
		ret = HD_ERR_NG;
		goto exit;
	}

	printf("ai_test_mem_get size(%lu) pa(%lx) va(%lx)\n", size, mem_parm->pa, mem_parm->va);
	mem_parm->blk = blk;

exit:
	return ret;
}

HD_RESULT ai_test_mem_rel(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	if (mem_parm->va) {
		ret = hd_common_mem_munmap((void *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("hd_common_mem_munmap fail\n");
		}
	}
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("hd_common_mem_release_block fail\n");
	}

	return ret;
}

#else
HD_RESULT ai_test_mem_get(MEM_PARM *mem_parm, UINT32 size, UINT32 id)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK      blk;
	
	blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap(va, size);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
	}

	mem_parm->pa = pa;
	mem_parm->va = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk = blk;

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
#endif

HD_RESULT ai_test_mem_alloc(MEM_PARM *mem_parm, CHAR* name, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;

	//alloc private pool
	ret = hd_common_mem_alloc(name, &pa, (void**)&va, size, DDR_ID0);
	if (ret!= HD_OK) {
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
	} else {
		printf("write %s with %ld bytes.\r\n", filename, write_size);
	}

	if (fd) {
		fclose(fd);
	}

	return size;
}

INT32 ai_test_mem_save_addr(UINT32 va, UINT32 write_size, const CHAR *filename)
{
	FILE *fd;
	UINT32 size = 0;

	fd = fopen(filename, "wb");

	if (!fd) {
		printf("ERR: cannot open %s for write!\r\n", filename);
		return -1;
	}

	size = (INT32)fwrite((VOID *)va, 1, write_size, fd);
	if (size != write_size) {
		printf("ERR: write %s with size %ld < wanted %ld?\r\n", filename, size, write_size);
	} else {
		printf("write %s with %ld bytes.\r\n", filename, write_size);
	}

	if (fd) {
		fclose(fd);
	}

	return size;
}



VOID ai_test_mem_fill(MEM_PARM *mem_parm, int mode)
{
    UINT32 i = 0;

	if (mode == 0) {
		// clear
		memset((VOID *)mem_parm->va, 1, mem_parm->size); 
	} else {
	    // struct timeval time_temp;
	    // gettimeofday(&time_temp, NULL);
	    // srand((time_temp.tv_sec - time_temp.tv_sec) * 1000000 + (time_temp.tv_usec - time_temp.tv_usec));
	    // for(i = 0; i < mem_parm->size; i++)
	    // {
	    //     ((UINT8 *)mem_parm->va)[i] = rand() & 0xff;
	    // }
	    for(i = 0; i < mem_parm->size; i++) {
	        ((INT8 *)mem_parm->va)[i] = (i & 0x07);
	    }
	}
}

