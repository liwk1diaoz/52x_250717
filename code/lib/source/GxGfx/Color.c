#include "GxGfx/GxGfx.h"

RESULT PALETTE_FILL_DEFAULT(PALETTE_ITEM *pPalette, UINT32 uiPxlfmt, UINT8 bpp)
{
	INT32 di = 0;
	INT32 nCount = 0;
	if (pPalette == 0) {
		return GX_NULL_POINTER;
	}
	switch (bpp) {
	case 1:
		/*
		//Windows System Palette

		//4 Colors Mode
		*/
		//pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[1]  = (COLOR_RGB_WHITE);
		nCount = 2;
		break;
	case 2:
		/*
		//Windows System Palette

		//4 Colors Mode
		*/
		//pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[1]  = (COLOR_RGB_GRAY);
		pPalette[2]  = (COLOR_RGB_LIGHTGRAY);
		pPalette[3]  = (COLOR_RGB_WHITE);
		nCount = 4;
		break;
	case 4:
		/*
		//Windows System Palette

		//16 Colors Mode
		0 $000000 black
		1 $000080 dark red
		2 $008000 dark green
		3 $008080 dark yellow
		4 $800000 dark blue
		5 $800080 dark magenta
		6 $808000 dark cyan
		7 $C0C0C0 light gray
		8 $808080 gray
		9 $0000FF red
		10 $00FF00 green
		11 $00FFFF yellow
		12 $FF0000 blue
		13 $FF00FF magenta
		14 $FFFF00 cyan
		15 $FFFFFF white
		*/
		//pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[1]  = (COLOR_RGB_DARKRED);
		pPalette[2]  = (COLOR_RGB_DARKGREEN);
		pPalette[3]  = (COLOR_RGB_DARKYELLOW);
		pPalette[4]  = (COLOR_RGB_DARKBLUE);
		pPalette[5]  = (COLOR_RGB_DARKMAGENTA);
		pPalette[6]  = (COLOR_RGB_DARKCYAN);
		pPalette[7]  = (COLOR_RGB_LIGHTGRAY);
		pPalette[8]  = (COLOR_RGB_GRAY);
		pPalette[9]  = (COLOR_RGB_RED);
		pPalette[10] = (COLOR_RGB_GREEN);
		pPalette[11] = (COLOR_RGB_YELLOW);
		pPalette[12] = (COLOR_RGB_BLUE);
		pPalette[13] = (COLOR_RGB_MAGENTA);
		pPalette[14] = (COLOR_RGB_CYAN);
		pPalette[15] = (COLOR_RGB_WHITE);
		nCount = 16;
		break;
	case 8:
		/*
		//Windows System Palette

		//256 Colors Mode
		0 $000000 black
		1 $000080 dark red
		2 $008000 dark green
		3 $008080 dark yellow
		4 $800000 dark blue
		5 $800080 dark magenta
		6 $808000 dark cyan
		7 $C0C0C0 light gray

		8 $C0DCC0 pastel green
		9 $F0CAA6 pastel blue

		10~223 : 6x6x6-2=214 (6x6x6 RGB cube colors exclude most dark and bright color)
		224~245 : 24-2=22 (24 gray ramp colors exclude most dark and bright color)

		246 $F0FBFF soft white
		247 $A4A0A0 medium gray

		248 $808080 gray
		249 $0000FF red
		250 $00FF00 green
		251 $00FFFF yellow
		252 $FF0000 blue
		253 $FF00FF magenta
		254 $FFFF00 cyan
		255 $FFFFFF white
		*/
		di = 0;
		//pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[0]  = (COLOR_RGB_BLACK);
		pPalette[1]  = (COLOR_RGB_DARKRED);
		pPalette[2]  = (COLOR_RGB_DARKGREEN);
		pPalette[3]  = (COLOR_RGB_DARKYELLOW);
		pPalette[4]  = (COLOR_RGB_DARKBLUE);
		pPalette[5]  = (COLOR_RGB_DARKMAGENTA);
		pPalette[6]  = (COLOR_RGB_DARKCYAN);
		pPalette[7]  = (COLOR_RGB_LIGHTGRAY);
		pPalette[8]  = (COLOR_RGB_PASTELGREEN);
		pPalette[9]  = (COLOR_RGB_PASTELBLUE);
		di += 10;
		{
			/*
			            //non-uniform ???
			            UINT8 ColorLevel[6] =
			                {0x00, 0x2C, 0x56, 0x87, 0xC0, 0xFF}; //?
			            UINT8 GrayLevel[24] =
			                {0x00, 0x11, 0x18, 0x1E, 0x25, 0x2C, 0x34, 0x3C,
			                0x44, 0x4D, 0x56, 0x5F, 0x69, 0x72, 0x7D, 0x92,
			                0x9D, 0xA8, 0xB4, 0xCC, 0xD8, 0xE5, 0xF2, 0xFF};
			*/
			//uniform (web safe)
			UINT8 ColorLevel[6] =
			{0x00, 0x33, 0x66, 0x99, 0xCC, 0xFF};
			UINT8 GrayLevel[24] = {
				0x00, 0x0B, 0x16, 0x21, 0x2C, 0x37, 0x42, 0x4D,
				0x58, 0x63, 0x6E, 0x79, 0x85, 0x90, 0x9B, 0xA6,
				0xB1, 0xBC, 0xC7, 0xD2, 0xDD, 0xE8, 0xF3, 0xFF
			};
			INT8 ri, gi, bi;
			INT8 grayi;
			for (bi = 0; bi < 6; bi++)
				for (gi = 0; gi < 6; gi++)
					for (ri = 0; ri < 6; ri++)
						if (!((ri == 0) && (gi == 0) && (bi == 0)) && !((ri == 5) && (gi == 5) && (bi == 5))) {
							pPalette[di] =
								(COLOR_MAKE_RGBD(ColorLevel[ri], ColorLevel[gi], ColorLevel[bi]));
							di++;
						}
			for (grayi = 0; grayi < 24; grayi++)
				if ((grayi != 0) && (grayi != 23)) {
					pPalette[di] =
						(COLOR_MAKE_RGBD(GrayLevel[grayi], GrayLevel[grayi], GrayLevel[grayi]));
					di++;
				}
		}
		pPalette[246] = (COLOR_RGB_SOFTWHITE);
		pPalette[247] = (COLOR_RGB_MEDIUMGRAY);
		pPalette[248] = (COLOR_RGB_GRAY);
		pPalette[249] = (COLOR_RGB_RED);
		pPalette[250] = (COLOR_RGB_GREEN);
		pPalette[251] = (COLOR_RGB_YELLOW);
		pPalette[252] = (COLOR_RGB_BLUE);
		pPalette[253] = (COLOR_RGB_MAGENTA);
		pPalette[254] = (COLOR_RGB_CYAN);
		pPalette[255] = (COLOR_RGB_WHITE);
		di += 10;
		nCount = 256;
		break;
	default:
		return GX_ERROR_FORMAT;
	}

    if(uiPxlfmt!=PXLFMT_RGBA8888_PK)
    {
    	//convert palette from RGB to YUV
    	for (di = 0; di < nCount; di++) {
    		pPalette[di] = COLOR_RGBD_2_YUVD(pPalette[di]);
    	}
    }
	return GX_OK;
}

RESULT PALETTE_FILL_ALPHA(PALETTE_ITEM *pal, COLOR_ITEM color_first, COLOR_ITEM color_last, UINT8 alpha)
{
	int i;
	if (pal == 0) {
		return GX_NULL_POINTER;
	}

	//set alpha = 0xff
	for (i = (int)color_first; i <= (int)color_last; i++) {
		pal[i] &= ~0xff000000; //clear
		pal[i] |= (alpha << 24); //fill
	}

	return GX_OK;
}

MAPPING_ITEM COLOR_INDEX8_GRAYLEVEL(CVALUE gray)
{
	MAPPING_ITEM i;
	i = ((gray + 12) / 24);
	if (i == 0) {
		return COLOR_INDEX8_BLACK;
	}
	if (i == 23) {
		return COLOR_INDEX8_WHITE;
	}
	return i - 1 + (256 - 10 - 22);
}

MAPPING_ITEM COLOR_INDEX8_COLORRGB(CVALUE r, CVALUE g, CVALUE b)
{
	MAPPING_ITEM i;
	r = (CVALUE)((r + 2) >> 2);
	g = (CVALUE)((g + 2) >> 2);
	b = (CVALUE)((b + 2) >> 2);
	if ((r == 0) && (g == 0) && (b == 0)) {
		return COLOR_INDEX8_BLACK;
	}
	if ((r == 5) && (g == 5) && (b == 5)) {
		return COLOR_INDEX8_WHITE;
	}
	i = 10 + (r * g * b) - 1;
	return i;
}

COLOR_ITEM INDEX_2_COLOR(UINT8 index, MAPPING_ITEM *pMap, PALETTE_ITEM *pPal)
{
	if (!pPal) {
		return COLOR_TRANSPARENT;
	}

	if (pMap) {
		index = (UINT8)(pMap[index]);
	}

	return PALETTE_GET_COLOR(pPal[index]);
}

RESULT PALETTE_FILL_GRADIENT(PALETTE_ITEM *pal, COLOR_ITEM color_first, COLOR_ITEM color_last, int levels)
{
	int i;
	int y1, u1, v1, a1;
	int y2, u2, v2, a2;
	int n = levels - 1;
	if (pal == 0) {
		return GX_NULL_POINTER;
	}
	if (n == 0) {
		return GX_ERROR_SIZEZERO;
	}

	y1 = COLOR_YUVD_GET_Y(color_first);
	u1 = COLOR_YUVD_GET_U(color_first);
	v1 = COLOR_YUVD_GET_V(color_first);
	a1 = PALETTE_GET_ALPHA(color_first);
	y2 = COLOR_YUVD_GET_Y(color_last);
	u2 = COLOR_YUVD_GET_U(color_last);
	v2 = COLOR_YUVD_GET_V(color_last);
	a2 = PALETTE_GET_ALPHA(color_last);
	for (i = 0; i < levels; i++) {
		int y, u, v, a;
		COLOR_ITEM yuvd;
		if (i == 0) {
			y = y1;
			u = u1;
			v = v1;
			a = a1;
		} else if (i == levels - 1) {
			y = y2;
			u = u2;
			v = v2;
			a = a2;
		} else {
			y = y1 * ((levels - 1) - i) / n + y2 * i / n;
			u = u1 * ((levels - 1) - i) / n + u2 * i / n;
			v = v1 * ((levels - 1) - i) / n + v2 * i / n;
			a = a1 * ((levels - 1) - i) / n + a2 * i / n;
		}
		yuvd = COLOR_MAKE_YUVD(y, u, v);
		pal[i] = PALETTE_YUVA_MAKE(a, yuvd);
	}
	return GX_OK;
}

