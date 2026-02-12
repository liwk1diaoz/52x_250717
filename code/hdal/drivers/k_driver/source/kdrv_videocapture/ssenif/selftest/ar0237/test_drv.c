#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "test_drv.h"
#include "test_dbg.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/




int nvt_test_drv_init(TEST_MODULE_INFO *pmodule_info)
{
	/* Add HW Module initialization here when driver loaded */

	return 0;
}

int nvt_test_drv_remove(TEST_MODULE_INFO *pmodule_info)
{
	/* Add HW Moduel release operation here*/

	return 0;
}

