#include "kflow_audiocapture/isf_audcap.h"
#include "isf_audcap_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_audcap/isf_audcap_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_audiocap_init(void)
{
	wavstudio_install_id();
	isf_reg_unit(ISF_UNIT(AUDCAP,0), &isf_audcap);

	return 0;
}
