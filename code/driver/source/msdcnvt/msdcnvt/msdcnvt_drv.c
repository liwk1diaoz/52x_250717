#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "msdcnvt_int.h"
#include "msdcnvt_drv.h"
#include "msdcnvt_ioctl.h"
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
	MSDCNVT_INIT init = {0};

	msdcnvt_install_id();

	init.api_ver = MSDCNVT_API_VERSION;
	if (!msdcnvt_init(&init)) {
		DBG_ERR("msdcnvt_init() Failed!\r\n");
		return -ENODEV;
	}

	return er;
}

int msdcnvt_drv_remove(MSDCNVT_INFO *pmodule_info)
{
	msdcnvt_exit();

	msdcnvt_uninstall_id();
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

int msdcnvt_drv_ioctl(unsigned char minor, MSDCNVT_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case MSDCNVT_IOC_CMD: {
			MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
			MSDCNVT_ICMD *p_cmd = &p_ctrl->ipc.p_cfg->cmd;
			((MSDCNVT_IAPI)p_cmd->api_addr)(p_cmd);
		}
		break;
	}

	return er;
}
