#ifndef _VPE_API_H_
#define _VPE_API_H_

#include "vpe_alg.h"


#if defined(__KERNEL__) || defined(__FREERTOS)


#endif

//=============================================================================
// struct & enum definition
//=============================================================================


//=============================================================================
// extern functions
//=============================================================================
#if defined(__KERNEL__) || defined(__FREERTOS)
extern USIZE vpe_2dlut_output_size[VPE_ID_MAX_NUM];

#endif

#endif

