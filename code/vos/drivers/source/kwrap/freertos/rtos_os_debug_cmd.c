#include <stdio.h>
#include <string.h>
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include "kwrap/cpu.h"
#include "kwrap/ker_dump.h"
#include "FreeRTOS.h"

#define __MODULE__    rtos_debug_cmd
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

#define EXCEPTION_TEST  0

typedef void (*KER_FP_CB)(UINT32 State); ///< State callback.


typedef struct _KER_EXP {
	KER_FP_CB  fpStateCb;
} KER_EXP, *PKER_EXP;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_debug_cmd_debug_level = NVT_DBG_WRN;
OSINIT_PARAM g_ker_int = {0};


void ker_dump_os_init(OSINIT_PARAM *init_parm)
{
	if (NULL == init_parm) {
		DBG_ERR("init_parm is NULL\r\n");
		return;
	}
	g_ker_int = *init_parm;
}


void ker_dump_mem_range(void)
{
	DBG_DUMP("\r\n======================== Kernel Memory Summary ========================\r\n");
	DBG_DUMP("Total     Region: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			 g_ker_int.total.addr, g_ker_int.total.addr + g_ker_int.total.size, g_ker_int.total.size);
	DBG_DUMP("Resv      Region: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			 g_ker_int.resv.addr, g_ker_int.resv.addr + g_ker_int.resv.size, g_ker_int.resv.size);
	DBG_DUMP("Code      Region: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			 g_ker_int.code.addr, g_ker_int.code.addr + g_ker_int.code.size, g_ker_int.code.size);
	DBG_DUMP("ZI        Region: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			 g_ker_int.zi.addr, g_ker_int.zi.addr + g_ker_int.zi.size, g_ker_int.zi.size);
	DBG_DUMP("OS Heap   Region: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			 g_ker_int.os_heap.addr, g_ker_int.os_heap.addr + g_ker_int.os_heap.size, g_ker_int.os_heap.size);
	DBG_DUMP("Libc Heap Region: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			 g_ker_int.libc_heap.addr, g_ker_int.libc_heap.addr + g_ker_int.libc_heap.size, g_ker_int.libc_heap.size);
}

static BOOL cmd_ker_dump(unsigned char argc, char **argv)
{
	vos_task_dump(vk_printk);
	vos_flg_dump(vk_printk);
	vos_sem_dump(vk_printk, 0);
	return TRUE;
}

static BOOL cmd_ker_dumpstack(unsigned char argc, char **argv)
{
	UINT32         task_id = 0;
	VK_TASK_HANDLE task_hdl;

	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&task_id);
	}
	task_hdl = vos_task_id2hdl(task_id);
	if (NULL == task_hdl) {
		DBG_DUMP("command format:ker dump stack <taskID>\r\n");
		return TRUE;
	}
	vos_show_stack(task_hdl, 2);
	return TRUE;
}

static BOOL cmd_ker_dumpsem(unsigned char argc, char **argv)
{
	vos_sem_dump(vk_printk, 1);
	return TRUE;
}

static BOOL cmd_ker_dumpmem(unsigned char argc, char **argv)
{
	ker_dump_mem_range();
	return TRUE;
}

static BOOL cmd_ker_dumposheap(unsigned char argc, char **argv)
{
	vPortDumpHeap();
	return TRUE;
}

#if EXCEPTION_TEST

static UINT8 *exp_func2(UINT32 i)
{
	volatile UINT32 r = 0x0;
	r = (*(UINT32 *)i);
	DBG_DUMP("read data =  %08x\r\n", (int)r); //show result to avoid compiler optimize our test code
	return 0;
}

static UINT8 *exp_func1(UINT32 i)
{
	return exp_func2(i + 200);
}

static BOOL cmd_ker_expund(unsigned char argc, char **argv)
{
	UINT32 *p = (UINT32 *)0;

	DBG_DUMP("*p = 0x%x\r\n", (int)*p);
	return TRUE;
}


BOOL cmd_ker_expdab(unsigned char argc, char **argv)
{
	DBG_DUMP("Test data abort exception\r\n");
	exp_func1(0xC0000000);
	return TRUE;
}

BOOL cmd_ker_exppfb(unsigned char argc, char **argv)
{
	KER_EXP   ker_exp = {0};

	DBG_DUMP("Test Prefetch abort exception\r\n");
	ker_exp.fpStateCb(5);
	return TRUE;
}

BOOL cmd_ker_expswi(unsigned char argc, char **argv)
{
	UINT32 swinum = 0x11;

	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&swinum);
	}
	DBG_DUMP("Test software interrupt\r\n");
#if 1
	__asm__ __volatile__("mov r1, %0\n\t"\
						 : \
						 : "r"(swinum) \
						 : "r1");

	__asm__ __volatile__("swi 5");
#endif
	return TRUE;
}
#endif

static SXCMD_BEGIN(ker_cmd_tbl, "ker_cmd_tbl")
SXCMD_ITEM("dump",        cmd_ker_dump,         "dump kernel info")
SXCMD_ITEM("dumpstack",   cmd_ker_dumpstack,    "dump stack content")
SXCMD_ITEM("dumpsem",     cmd_ker_dumpsem,      "dump all semaphores")
SXCMD_ITEM("dumpmem",     cmd_ker_dumpmem,      "dump the memory layout")
SXCMD_ITEM("dumposheap",  cmd_ker_dumposheap,   "dump the freertos heap layout")
#if EXCEPTION_TEST
SXCMD_ITEM("expund",      cmd_ker_expund,       "test undefined instruction exception")
SXCMD_ITEM("expdab",      cmd_ker_expdab,       "test data abort exception")
SXCMD_ITEM("exppfb",      cmd_ker_exppfb,       "test prefetch abort exception")
SXCMD_ITEM("expswi",      cmd_ker_expswi,       "test software interrupt")
#endif
SXCMD_END()


static int ker_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(ker_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ker");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", ker_cmd_tbl[loop].p_name, ker_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(ker, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(ker_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		ker_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ker_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = ker_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	ker_cmd_showhelp(vk_printk);
	return 0;
}


static BOOL cmd_cwp_on(unsigned char argc, char **argv)
{
	UINT32  index = 0, addr = 0 , size = 0;

	if (argc < 3) {
		DBG_ERR("syntex: cwp on [index][addr][size]\r\n");
		return TRUE;
	}
	sscanf(argv[0], "%d", (int *)&index);
	sscanf(argv[1], "%x", (int *)&addr);
	sscanf(argv[2], "%d", (int *)&size);
	vos_cpu_enable_watch(index, addr, size);
	return TRUE;
}

static BOOL cmd_cwp_off(unsigned char argc, char **argv)
{
	UINT32 index = 0;

	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&index);
	}
	vos_cpu_disable_watch(index);
	return TRUE;
}

static SXCMD_BEGIN(cwp_cmd_tbl, "cpu write protection")
SXCMD_ITEM("on %", cmd_cwp_on, "start protecting")
SXCMD_ITEM("off %", cmd_cwp_off, "stop protecting")
SXCMD_END()


static int cwp_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(cwp_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ker");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", cwp_cmd_tbl[loop].p_name, cwp_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(cwp, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(cwp_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		cwp_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], cwp_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = cwp_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	cwp_cmd_showhelp(vk_printk);
	return 0;
}

BOOL cmd_mem_w(unsigned char argc, char **argv)
{
	UINT32  addr, data;

	//arg_num = sscanf(strCmd, "%x %x\n", &addr, &data);
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&addr);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&data);
	}
	if (argc == 2) {
		addr &= 0xFFFFFFFC;
		*((UINT32 *) addr) = data;
		DBG_DUMP("addr = 0x%08X, data = 0x%08X\r\n", addr, data);
	} else {
		DBG_DUMP("syntex: mem w [addr] [data]\r\n");
	}
	return TRUE;
}
BOOL cmd_mem_r(unsigned char argc, char **argv)
{
	UINT32  addr, length;

	//arg_num = sscanf(strCmd, "%x %x\n", &addr, &length);
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&addr);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&length);
	}
	if (argc == 1) {
		length = 256;
	}
	if ((argc == 1) || (argc == 2)) {
		addr &= 0xFFFFFFFC;
		debug_dumpmem(addr, length);
	} else {
		DBG_DUMP("syntex: mem r [addr] (length)\r\n");
	}
	return TRUE;
}

BOOL cmd_mem_fill(unsigned char argc, char **argv)
{
	UINT32  addr, length, data;
	//INT32   arg_num;

	//arg_num = sscanf(strCmd, "%x %x %x\n", &addr, &length, &data);
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&addr);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&length);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%x", (int *)&data);
	}
	if ((argc == 2) || (argc == 3)) {
		if (argc == 2) {
			data = 0;
		}
		data &= 0x000000ff;
		DBG_DUMP("addr = 0x%08X, length = 0x%08X, data = 0x%02X\r\n", addr, length, data);
		memset((void *)addr, (int)data, (size_t)length);
	} else {
		DBG_DUMP("syntex: mem fill [addr] [length] (data)\r\n");
	}
	return TRUE;
}

BOOL cmd_mem_dump(unsigned char argc, char **argv)
{
	UINT32  addr, length;
	#define FILENAME_MAX_LEN 64
	char    filename[FILENAME_MAX_LEN] = "A:\\dump.bin";

	//arg_num = sscanf(strCmd, "%x %x %s\n", &addr, &length, &name);
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&addr);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&length);
	}
	if (argc >= 3) {
		strncpy(filename, argv[2], FILENAME_MAX_LEN - 1);
		filename[FILENAME_MAX_LEN - 1] = 0; //fill string end
	}
	if ((argc == 1) || (argc == 2) || (argc == 3)) {
		addr &= 0xFFFFFFFC;
		if (argc < 2) {
			length = 256;
		}
		debug_dumpmem2file(addr, length, filename);
	} else {
		DBG_DUMP("syntex: mem dump [addr] (length) (filename)\r\n");
	}
	return TRUE;
}

/*
BOOL cmd_mem_dumpcode(CHAR *strCmd)
{
	UINT32  addr, length;
	char    filename[64] = "A:\\dumpcode.bin";

	addr = OS_GetMemAddr(MEM_CODE);
	length = OS_GetMemSize(MEM_CODE);
	debug_dumpmem2file(addr, length, filename);
	return TRUE;
}
*/

static SXCMD_BEGIN(mem_cmd_tbl, "Memory access")
SXCMD_ITEM("w %",      cmd_mem_w, "Write a word into memory")
SXCMD_ITEM("r %",      cmd_mem_r, "Read a region of memory")
SXCMD_ITEM("fill %",   cmd_mem_fill, "Fill a region of memory")
SXCMD_ITEM("dump %",   cmd_mem_dump, "Dump a region of memory to a file")
//SXCMD_ITEM("dumpcode", cmd_mem_dumpcode, "Dump firmware code region to a bin file")
SXCMD_END()


static int mem_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(mem_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "mem");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", mem_cmd_tbl[loop].p_name, mem_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(mem, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(mem_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		mem_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], mem_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = mem_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	mem_cmd_showhelp(vk_printk);
	return 0;
}

