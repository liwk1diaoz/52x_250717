#include "kflow_videoout/isf_vdoout.h"
#include "isf_vdoout_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdoout_dbg.h"
#include "../isf_vdoout_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_videoout_init(void)
{
	isf_reg_unit(ISF_UNIT(VDOOUT, 0), &isf_vdoout0);

	return 0;
}
