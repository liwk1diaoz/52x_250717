#include "isf_vdoenc_drv.h"
#include "isf_vdoenc_ioctl.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "nmediarec_api.h"

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "isf_vdoenc_dbg.h"

///#include "../isf_vdoenc_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_vdoenc_drv_open(ISF_VDOENC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_vdoenc_drv_release(ISF_VDOENC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_vdoenc_drv_init(ISF_VDOENC_INFO *pmodule_info)
{
	int er = 0;

	//isf_vdoenc_install_id();
	//nmr_vdoenc_install_id();
	nmr_vdocodec_install_id();

    isf_reg_unit(ISF_UNIT_ID(VDOENC,0), &isf_vdoenc);

	return er;
}

int isf_vdoenc_drv_remove(ISF_VDOENC_INFO *pmodule_info)
{
	//isf_vdoenc_uninstall_id();
	//nmr_vdoenc_uninstall_id();
	nmr_vdocodec_uninstall_id();

	return 0;
}

int isf_vdoenc_drv_suspend(ISF_VDOENC_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_vdoenc_drv_resume(ISF_VDOENC_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_vdoenc_drv_ioctl(unsigned char minor, ISF_VDOENC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_VDOENC_IOC_CMD: {
		}
		break;
	}

	return er;
}
