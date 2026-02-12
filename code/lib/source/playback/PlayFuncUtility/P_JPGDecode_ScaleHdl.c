#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Check if need to scale when decoded data out
    This is internal API.

    @param[in] jdcfg_p:
    @param[in,out] pScaleLvl: 0->No scale; 1->(1/2); 2->(1/4); 3->(1/8)
    @return E_OK: Done
*/
INT32 PB_JPGScalarHandler(void)
{
	return E_OK;
}
