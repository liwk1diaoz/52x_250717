#ifndef _SDE_API_H_
#define _SDE_API_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "sde_alg.h"
#endif

//=============================================================================
// struct & enum definition
//=============================================================================


//=============================================================================
// extern functions
//=============================================================================
#if defined(__KERNEL__) || defined(__FREERTOS)
extern ER sde_flow_init(SDE_ID id);
extern ER sde_trigger(SDE_ID id);
#endif

#endif

