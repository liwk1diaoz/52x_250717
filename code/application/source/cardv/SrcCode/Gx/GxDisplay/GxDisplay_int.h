#ifndef _DISPLAY_H_
#define _DISPLAY_H_

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxDisp
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
 #include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////


#if defined(_BSP_NA51000_)
#define DEVICE_COUNT	2
#else
#define DEVICE_COUNT	1
#endif

#define GX_HW_NT962XX   0
#define GX_HW_NT964XX   1
#define GX_HW_NT96630   2
#define GX_HW_NT96450   3
#define GX_HW_NT96220   4
#define GX_HW_NT96650   5

#define GX_HW                       GX_HW_NT96650

#if 0
#define NEW_DISPOBJ_API             1   //use new display object API

#define HW_PALETTE_NUM              0x10    //HW palette count for each OSD
#endif

#define LAYER_BUFFER_NUM            3

//flag for LAYER_STATE_FLAG
#define LRFLAG_SIGN                 0x00004758 //"GX"
#define LRFLAG_SIGN_MASK            0x0000FFFF
#define LRFLAG_INITID               0x00010000
#define LRFLAG_CLEAR                0x00020000
#define LRFLAG_INITDC               0x00040000
#define LRFLAG_SETPAL               0x00080000
#define LRFLAG_SETEN                0x00100000
#define LRFLAG_SETWIN               0x00200000
#define LRFLAG_DRAW                 0x00400000
#define LRFLAG_FLIP                 0x00800000
#define LRFLAG_SETSWAP              0x01000000
#define LRFLAG_SETOSDMIX            0x02000000
#define LRFLAG_SETVDOMIX            0x04000000
#define LRFLAG_SETCTRL              0x08000000
#define LRFLAG_SETKEY               0x10000000

typedef struct _DISPLAY_LAYER {
	//LAYER_STATE_FLAG
	UINT32 uiFlag;

//-input buffer
	//LAYER_STATE_TYPEFMT
	UINT16 uiType;
	UINT16 uiPxlfmt;

	//LAYER_STATE_BUFWIDTH
	UINT32 uiWidth;

	//LAYER_STATE_BUFHEIGHT
	UINT32 uiHeight;

	//LAYER_STATE_BUFSIZE
	UINT32 uiBufSize;

	//LAYER_STATE_BUFADDR0/1/2
	UINT8 *pBufAddr[3];     //memory address

	//LAYER_STATE_BUFATTR
	UINT16 uiSwapEffect;    // 1 = DISCARD, 2 = COPY, 3 = FLIP;
	UINT16 uiBufCount;      // 0 = one buffer, 1 = double buffer, 2 = triple buffer...

//-output window
	//LAYER_STATE_WINX
	//LAYER_STATE_WINY
	//LAYER_STATE_WINW
	//LAYER_STATE_WINH
	IRECT win;

	//LAYER_STATE_WINATTR
	UINT32  uiWinAttr;  // 0x01 = MIRROR X, 0x02 = MIRROR Y

//-status
	//LAYER_STATE_ENABLE
	UINT32 uiEnable;

	//LAYER_STATE_INFO
	UINT8 uiBufShowID;
	UINT8 uiBufDrawID;
	UINT16 uiPalCount;
}
DISPLAY_LAYER;


typedef struct _DISPLAY_MIXER {
	UINT32 uiOSD1KeyOp;
	UINT32 uiOSD1KeyColor;
	UINT32 uiOSD1BlendOp;
	UINT32 uiOSD1ConstAlpha;

	UINT32 uiOSD2KeyOp;
	UINT32 uiOSD2KeyColor;
	UINT32 uiOSD2BlendOp;
	UINT32 uiOSD2ConstAlpha;

	UINT32 uiVDO1KeyOp;
	UINT32 uiVDO1KeyColor;
	UINT32 uiVDO1BlendOp;
	UINT32 uiVDO1ConstAlpha;

	UINT32 uiVDO2KeyOp;
	UINT32 uiVDO2KeyColor;
	UINT32 uiVDO2BlendOp;
	UINT32 uiVDO2ConstAlpha;

	UINT32 uiBackColor;
	UINT32 uiAllCtrl;
	UINT32 uiAllEnable;

	UINT32 uiFlag;

	UINT32 PalStart[2];
	UINT32 PalCount[2];
	PALETTE_ITEM Pal[2][256];
}
DISPLAY_MIXER;


INT32 _DL_DumpBuf(UINT32 LayerID);


#endif //_DISPLAY_H_
