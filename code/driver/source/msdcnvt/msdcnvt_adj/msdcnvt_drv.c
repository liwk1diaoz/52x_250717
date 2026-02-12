#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "msdcnvt_drv.h"
#include "msdcnvt_int.h"
#include "msdcnvt_adj.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int msdcnvt_drv_open(MSDCNVT_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int msdcnvt_drv_release(MSDCNVT_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int msdcnvt_drv_init(MSDCNVT_INFO *pmodule_info)
{
	int er = 0;

	msdcnvtregbi_adj();

	return er;
}

int msdcnvt_drv_remove(MSDCNVT_INFO *pmodule_info)
{
	return 0;
}

int msdcnvt_drv_suspend(MSDCNVT_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int msdcnvt_drv_resume(MSDCNVT_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}
