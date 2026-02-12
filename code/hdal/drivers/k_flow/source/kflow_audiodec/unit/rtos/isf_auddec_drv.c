#include "kflow_audiodec/isf_auddec.h"
#include "isf_auddec_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_audiodec_init(void)
{
	isf_reg_unit(ISF_UNIT(AUDDEC,0), &isf_auddec);

	return 0;
}
