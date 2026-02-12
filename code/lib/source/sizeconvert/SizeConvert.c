/*
    Size convert calculation.

    @file       SizeConvert.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include <kwrap/nvt_type.h>
#include <kwrap/verinfo.h>
#include "SizeConvert.h"

#if defined __UITRON || defined __ECOS || defined __FREERTOS
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          sizeconvert
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int sizeconvert_debug_level = NVT_DBG_WRN;


/**
    Size convert.

    Input/output size calculation accroding to desired width/height ratio.

    @param[in] pScinfo      the size convert input information

    @return void
*/
void DisplaySizeConvert(SIZECONVERT_INFO *pScinfo)
{
	float ratioSrc, ratioOut, x, out_x;
	UINT32 wid, hei;
	UINT32 outw, outh;
	INT32 temp1, temp2;
	SIZECONVERT_ALIGH_TYPE alignType = SIZECONVERT_ALIGN_FLOOR_32;

	if (!(pScinfo->alignType & 0x80000000)) {
		DBG_ERR("[SizeConvert] invalid alignType = 0x%lx!!!\r\n", pScinfo->alignType);
		DBG_ERR("[SizeConvert] using defalut, aligh to 32!\r\n");
		alignType = SIZECONVERT_ALIGN_FLOOR_32;
	} else {
		alignType = pScinfo->alignType;
	}

	pScinfo->uiOutX = 0;
	pScinfo->uiOutY = 0;

	wid = pScinfo->uiDstWidth;
	hei = pScinfo->uiDstHeight;

	ratioSrc = (float)pScinfo->uiSrcWidth / (float)pScinfo->uiSrcHeight;
	ratioOut = (float)pScinfo->uiDstWRatio / (float)pScinfo->uiDstHRatio;
	x = (wid / ratioOut) / hei;
	if (ratioSrc == ratioOut) {
		//do nothing
		outw = wid;
		outh = hei;
	} else if (ratioSrc > ratioOut) { //16:9 => 4:3//black up/down
		temp1 = (INT32)(ratioSrc * 100);
		temp2 = (INT32)(ratioOut * 100);
		if ((temp1 - temp2) < 5) { // <5%, ignore
			//do nothing
			outw = wid;
			outh = hei;
		} else {
			out_x = (wid / ratioSrc) / x;
			//640*9/16*(448/480)
			outh = (UINT32)out_x;
			if ((hei - outh) % 2) {
				outh -= 1;
			}
			outw = wid;

			pScinfo->uiOutY = (pScinfo->uiDstHeight - outh) / 2;
			pScinfo->uiOutX = 0;
		}
	} else { //4:3 =>16:9 black left/right
		temp1 = (INT32)(ratioOut * 100);
		temp2 = (INT32)(ratioSrc * 100);
		if ((temp1 - temp2) < 5) { // <5%, ignore
			outw = wid;
			outh = hei;
		} else {
			out_x = (wid / ratioOut) * ratioSrc;
			outw = (UINT32)out_x;
			//out_w += 0x10;
			outw &= alignType;
			outh = hei;

			pScinfo->uiOutX = (wid - outw) / 2;
			pScinfo->uiOutY = 0;
		}
	}

	pScinfo->uiOutWidth = outw;
	pScinfo->uiOutHeight = outh;
}

VOS_MODULE_VERSION(SizeConvert, 1, 00, 000, 00)

