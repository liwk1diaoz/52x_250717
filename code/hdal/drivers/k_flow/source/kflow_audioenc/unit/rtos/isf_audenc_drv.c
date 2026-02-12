#include "kflow_audioenc/isf_audenc.h"
#include "isf_audenc_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_audioenc_init(void)
{
	isf_reg_unit(ISF_UNIT(AUDENC,0), &isf_audenc);

	return 0;
}
