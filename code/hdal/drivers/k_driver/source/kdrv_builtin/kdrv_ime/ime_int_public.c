
#include "ime_platform.h"

/*
#ifdef __KERNEL__

#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

#include "ime_dbg.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "Type.h"

#endif  // end #ifdef __KERNEL__
*/

#include "ime_int_public.h"



UINT32 get_coef_val[17] = {0};

UINT32 bit_mask[33] = {0};


VOID ime_base_reg_init(VOID)
{
	UINT32 i;

	for (i = 0; i <= 31; i++) {
		bit_mask[i] = ((1 << i) - 1);
	}
	bit_mask[32] = 0xFFFFFFFF;
}
//---------------------------------------------------------------------------

UINT32 ime_int_to_2comp(INT32 val, INT32 bits)
{
	UINT32 tmp = 0;
	UINT32 mask = (1 << bits) - 1;

	if (val >= 0) {
		tmp = (((UINT32)val) & mask);
	} else {
		//cout << ((unsigned int)(~(-1*val)) & mask) << endl;
		tmp = ((UINT32)(~(-1 * val)) & mask) + 1;
	}

	return tmp;
}
//----------------------------------------------------------------------------------


UINT32 ime_scaling_isd_init_kernel_number(UINT32 in_size, UINT32 out_size)
{
	UINT32 factor = 0;
	UINT32 ker_cnt = 0;

	factor = ((in_size - 1) * 65536) / (out_size - 1);

	if (factor > 0) {
		ker_cnt += 1;
		factor -= 65536;
	}

	while (factor > 0) {
		if (factor >= 131072) {
			ker_cnt += 2;
			factor -= 131072;
		} else {
			ker_cnt += 2;

			//coverity[result_independent_of_operands]
			factor -= factor;
		}
	}

	return ker_cnt;
}
//---------------------------------------------------------------------------------------------------



