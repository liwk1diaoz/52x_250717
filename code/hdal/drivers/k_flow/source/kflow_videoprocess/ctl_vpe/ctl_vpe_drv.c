#if defined(__LINUX)
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#elif defined(__FREERTOS)
#endif
#include "ctl_vpe_drv.h"
#include "ctl_vpe_dbg.h"
#include "ctl_vpe_isp_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ctl_vpe_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	return 0;
}

int nvt_ctl_vpe_drv_init(MODULE_INFO *pmodule_info)
{
	ctl_vpe_isp_drv_init();
	return 0;
}

int nvt_ctl_vpe_drv_remove(MODULE_INFO *pmodule_info)
{
	ctl_vpe_isp_drv_uninit();
	return 0;
}

int nvt_ctl_vpe_drv_suspend(MODULE_INFO *pmodule_info)
{
	/* Add suspend operation here*/

	return 0;
}

int nvt_ctl_vpe_drv_resume(MODULE_INFO *pmodule_info)
{
	/* Add resume operation here*/

	return 0;
}
