
#ifndef _IME_INT_PUBLIC_
#define _IME_INT_PUBLIC_

#ifdef __cplusplus
extern "C" {
#endif



#include "ime_platform.h"
#include "ime_int.h"


extern UINT32 get_coef_val[17];

extern UINT32 bit_mask[33];

extern VOID ime_base_reg_init(VOID);

extern UINT32 ime_int_to_2comp(INT32 val, INT32 bits);

extern UINT32 ime_scaling_isd_init_kernel_number(UINT32 in_size, UINT32 out_size);


#ifdef __cplusplus
}
#endif

#endif // _IME_INT_PUBLIC_
