#include "kflow_audioout/isf_audout.h"
#include "isf_audout_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_audout_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_audioout_init(void)
{
	isf_reg_unit(ISF_UNIT(AUDOUT,0), &isf_audout0);
	isf_reg_unit(ISF_UNIT(AUDOUT,1), &isf_audout1);

	return 0;
}
