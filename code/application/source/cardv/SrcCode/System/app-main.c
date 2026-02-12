#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hdal.h>
#include <kwrap/cmdsys.h>
#include <kwrap/examsys.h>
#include <kwrap/perf.h>
#include "startup.h"
#include "sys_mempool.h"
#include "timer.h"
#include "Utility/SwTimer.h"
#include "SxTimer/SxTimer.h"
#include "GxTimer.h"
#include "sys_filesys.h"

extern int NvtMain(void);
extern void System_ChkBootFromRtos(void);
extern UINT32 System_IsBootFromRtos(void);

static void insmod_system(void)
{
	vos_perf_list_mark(__func__, __LINE__, 0);
	nvt_cmdsys_init();      // command system
	nvt_examsys_init();     // exam system
	timer_init();           // hw timer
	SwTimer_Init();         // sw timer
	vos_perf_list_mark(__func__, __LINE__, 1);
}

static void insmod_capture(void)
{
	vos_perf_list_mark(__func__, __LINE__, 0);

	vos_perf_list_mark(__func__, __LINE__, 1);
}

static void insmod_display(void)
{
	vos_perf_list_mark(__func__, __LINE__, 0);

	vos_perf_list_mark(__func__, __LINE__, 1);
}

static void insmod_storage(void)
{
	vos_perf_list_mark(__func__, __LINE__, 0);
	filesys_init();
	vos_perf_list_mark(__func__, __LINE__, 1);
}

static void insmod_encoder(void)
{
	vos_perf_list_mark(__func__, __LINE__, 0);

	vos_perf_list_mark(__func__, __LINE__, 1);
}

static void insmod_others(void)
{
	vos_perf_list_mark(__func__, __LINE__, 0);
	SxTimer_Init();         // detect system
	GxTimer_Init();         // UI timer
	vos_perf_list_mark(__func__, __LINE__, 1);
}

void insmod(void)
{
	insmod_system();
	insmod_capture();
	insmod_display();
	insmod_storage();
	insmod_encoder();
	insmod_others();
}

void app_main(void)
{
	vos_perf_list_reset();
	vos_perf_list_mark(__func__, __LINE__, 0);

	printf("\nHello Linux World! (%s)\n\n", __DATE__ " - " __TIME__);
	System_ChkBootFromRtos();
	printf("is_boot_from_rtos=%d\n\n",System_IsBootFromRtos());
	// allocate fixed memory for module initialzation
	mempool_init();

	// insmod for sw-modules initialzation
	insmod();

	NvtMain();
}
