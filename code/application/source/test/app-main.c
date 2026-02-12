#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hdal.h>
#include <kwrap/cmdsys.h>
#include <kwrap/examsys.h>
#include "startup/startup.h"
#include "timer.h"
#include "Utility/SwTimer.h"
#include "SxTimer/SxTimer.h"
#include "comm/hwclock.h"
#include "sys_mempool.h"
#include "sys_card.h"
#include "sys_filesys.h"


void insmod(void)
{
	nvt_cmdsys_init();      // command system
	nvt_examsys_init();     // exam system
	timer_init();           // hw timer
	SwTimer_Init();         // sw timer
	SxTimer_Init();         // detect system
	card_init();            // sd card
	filesys_init();         // file system
}

void app_main(void)
{
	printf("\nHello Linux World! (%s)\n\n", __DATE__ " - " __TIME__);

	// allocate fixed memory for module initialzation
	mempool_init();

	// insmod for sw-modules initialzation
	insmod();

	//open hardware clock
	hwclock_open(HWCLOCK_MODE_RTC);
}
