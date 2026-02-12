#if defined(__FREERTOS)
#include <string.h>
#include <stdlib.h>
#else
#include <linux/slab.h>
#include <linux/vmalloc.h>
#endif

#include "kwrap/type.h"
#include "vpe_api_int.h"
#include "vpe_dbg.h"

//=============================================================================
// extern functions
//=============================================================================
void *vpe_uti_mem_alloc(UINT32 mem_size)
{
	#if defined(__FREERTOS)
	return malloc(mem_size);
	#else
	return vmalloc(mem_size);
	#endif
}

void vpe_uti_mem_free(void *mem_addr)
{
	#if defined(__FREERTOS)
	free(mem_addr);
	#else
	vfree(mem_addr);
	#endif
}

UINT32 vpe_uti_calc_2dlut_tbl_length(UINT32 _2dlut_idx)
{
	UINT32 _2dlut_length;

	switch (_2dlut_idx) {
		case VPE_ISP_2DLUT_SZ_9X9:
			_2dlut_length = 9;
			break;
		case VPE_ISP_2DLUT_SZ_65X65:
			_2dlut_length = 65;
			break;
		case VPE_ISP_2DLUT_SZ_129X129:
			_2dlut_length = 129;
			break;
		default:
		case VPE_ISP_2DLUT_SZ_257X257:
			_2dlut_length = 257;
			break;
	}

	return _2dlut_length;
}

UINT32 vpe_uti_calc_2dlut_tbl_size(UINT32 _2dlut_idx)
{
	UINT32 _2dlut_size;

	switch (_2dlut_idx) {
		case VPE_ISP_2DLUT_SZ_9X9:
			_2dlut_size = 12 * 9 * sizeof(UINT32);
			break;
		case VPE_ISP_2DLUT_SZ_65X65:
			_2dlut_size = 68 * 65 * sizeof(UINT32);
			break;
		case VPE_ISP_2DLUT_SZ_129X129:
			_2dlut_size = 132 * 129 * sizeof(UINT32);
			break;
		default:
		case VPE_ISP_2DLUT_SZ_257X257:
			_2dlut_size = 260 * 257 * sizeof(UINT32);
			break;
	}

	return _2dlut_size;
}

UINT32 vpe_uti_calc_2dlut_ioctl_size(UINT32 _2dlut_idx)
{
	UINT32 _2dlut_idx_tmp;

	if (_2dlut_idx <= VPE_ISP_2DLUT_SZ_65X65) {
		_2dlut_idx_tmp = VPE_ISP_2DLUT_SZ_65X65;
	} else {
		_2dlut_idx_tmp = _2dlut_idx;
	}

	return vpe_uti_calc_2dlut_tbl_size(_2dlut_idx_tmp) + sizeof(UINT32) * 2; // lut + size + id
}

static INT32 vpe_uti_sqrt_taylor(INT32 val)
{
	INT32 ans, pre = 0;

	ans = val;

	while (abs(ans - pre) > 1) {
		pre = ans;
		ans = (ans + val / ans) / 2;
	}


	return ans;
}

static void vpe_uti_unsigned_2s_comp_to_signed_num(UINT32 tbl, INT32 *px, INT32 *py)
{
	UINT32 ux, uy;

	ux = tbl & 0xFFFF;
	uy = (tbl >> 16) & 0xFFFF;

	if ((ux & 0x8000) == 0x0) {
		*px = ux;
	} else {
		*px = -(0x8000 - (ux & 0x7FFF));
	}

	if ((uy & 0x8000) == 0x0) {
		*py = uy;
	} else {
		*py = -(0x8000 - (uy & 0x7FFF));
	}
}

void vpe_uti_calc_2dlut_output_size(USIZE *size, UINT32 lut_sz_sel, UINT32 *lut_tbl)
{
	INT32 lut_sz = (INT32)vpe_uti_calc_2dlut_tbl_length((VPE_ISP_2DLUT_SZ)lut_sz_sel);
	INT32 step; // subsample
	INT32 i, j, dcout_x = 0, dcout_y = 0, max_dcout_x = 0, max_dcout_y = 0;
	INT32 x2d[2], y2d[2];
	INT32 padded_lut_sz = (((lut_sz + 3) >> 2) << 2);

	if (lut_sz == 2) {
		step = 1; //to prevent accessing 2dlut out-of-bounds
	} else {
		step = (lut_sz - 1) >> 3;
	}

	//horizontal
	for (j = 0; j < lut_sz; j += step) {
		dcout_x = 0;
		for (i = 0; i < lut_sz - step; i += step) {
			vpe_uti_unsigned_2s_comp_to_signed_num(lut_tbl[j * padded_lut_sz + i], &x2d[0], &y2d[0]);
			vpe_uti_unsigned_2s_comp_to_signed_num(lut_tbl[j * padded_lut_sz + (i + step)], &x2d[1], &y2d[1]);
			
			x2d[0] /= 4; y2d[0] /= 4;
			x2d[1] /= 4; y2d[1] /= 4;
			dcout_x = dcout_x + vpe_uti_sqrt_taylor((x2d[1] - x2d[0]) * (x2d[1] - x2d[0]) + (y2d[1] - y2d[0]) * (y2d[1] - y2d[0]));
		}
		if (dcout_x > max_dcout_x) {
			max_dcout_x = dcout_x;
		}
	}

	//vertical
	for (i = 0; i < lut_sz; i += step) {
		dcout_y = 0;
		for (j = 0; j < lut_sz - step; j += step) {
			vpe_uti_unsigned_2s_comp_to_signed_num(lut_tbl[j * padded_lut_sz + i], &x2d[0], &y2d[0]);
			vpe_uti_unsigned_2s_comp_to_signed_num(lut_tbl[(j + step) * padded_lut_sz + i], &x2d[1], &y2d[1]);
			x2d[0] /= 4; y2d[0] /= 4;
			x2d[1] /= 4; y2d[1] /= 4;
			dcout_y = dcout_y + vpe_uti_sqrt_taylor((x2d[1] - x2d[0]) * (x2d[1] - x2d[0]) + (y2d[1] - y2d[0]) * (y2d[1] - y2d[0]));
		}
		if (dcout_y > max_dcout_y) {
			max_dcout_y = dcout_y;
		}
	}

	size->w = ALIGN_CEIL(max_dcout_x, 8);
	size->h = ALIGN_CEIL(max_dcout_y, 2);
}
