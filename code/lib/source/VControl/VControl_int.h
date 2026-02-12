
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          VCtrl
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////
#include <kwrap/debug.h>

extern INT32 Ux_SendEventInternal(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);

