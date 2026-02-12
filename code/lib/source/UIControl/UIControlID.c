#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include <string.h>
#include "UIControlInternal.h"
#include "UIControlID_int.h"

ID UICTRL_FLG_ID = 0;
ID UICTRL_WND_SEM_ID = 0;
ID UICTRL_DRW_SEM_ID = 0;

void UIControl_InstallID(void)
{
    #if 1
	OS_CONFIG_FLAG(UICTRL_FLG_ID);
	OS_CONFIG_SEMPHORE(UICTRL_WND_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(UICTRL_DRW_SEM_ID, 0, 1, 1);
    #else
	//T_CSEM csem;
	//T_CFLG cflg;

	//memset(&csem, 0, sizeof(csem));
	//memset(&cflg, 0, sizeof(T_CFLG));
	//csem.isemcnt = 1;
	//csem.maxsem = 1;


	// create semaphore, maxsem = 1
	if (E_OK != vos_sem_create(&UICTRL_WND_SEM_ID, 1, "UICTRL_WND_SEM")) {
		DBG_ERR("UICTRL_WND_SEM fail\r\n");
	}

	if (E_OK != vos_sem_create(&UICTRL_DRW_SEM_ID, 1, "UICTRL_DRW_SEM")) {
		DBG_ERR("UICTRL_DRW_SEM fail\r\n");
	}

	if (E_OK != vos_flag_create(&UICTRL_FLG_ID, NULL, "UICTRL_FLG")) {
		DBG_ERR("UICTRL_FLG fail\r\n");
	}

    #endif
}
void UIControl_UninstallID(void)
{
    #if 1
    SEM_DESTROY(UICTRL_WND_SEM_ID);
    SEM_DESTROY(UICTRL_DRW_SEM_ID);
    rel_flg(UICTRL_FLG_ID);
    #else
    if (UICTRL_WND_SEM_ID) {
        vos_sem_destroy(UICTRL_WND_SEM_ID);
    }
    if (UICTRL_DRW_SEM_ID) {
        vos_sem_destroy(UICTRL_DRW_SEM_ID);
    }
	if (UICTRL_FLG_ID) {
		vos_flag_destroy(UICTRL_FLG_ID);
	}
    #endif
}


