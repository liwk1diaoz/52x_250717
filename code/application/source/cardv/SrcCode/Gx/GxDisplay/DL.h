#ifndef _DL_H_
#define _DL_H_

#include "GxCommon.h"


#ifndef LAYER_NUM
#define _DD(id)                     (((id)>>4)&0x0f)
#define _DL(id)                     ((id)&0x0f)
#define _LayerID(dev,lyr)           ((((dev)&0x0f)<<4)|((lyr)&0x0f))

//DISPLAY layer id
#define LAYER_OSD1                  0x00
#define LAYER_OSD2                  0x01
#define LAYER_VDO1                  0x02
#define LAYER_VDO2                  0x03
#define LAYER_OUTPUT                0x04    //output = mix ( osd1, osd2, vdo1, vdo2 )
#define LAYER_FACE                  0x05    //face tracing frame
#define LAYER_ALL                   0x0F

#define LAYER_NUM                   4       //total input layer
#define LAYER_MAX                   (LAYER_NUM+1)
#endif

#ifndef MIRROR_X
//flag for MIRROREFFECT
#define MIRROR_DISCARD              0x00    //no mirror
#define MIRROR_X                    0x01    //mirror in x direction
#define MIRROR_Y                    0x02    //mirror in y direction
#define MIRROR_KEEP                 0xff    //keep current direction
//value for MIRROREFFECT
#define MIRROR_DEFAULT              (MIRROR_DISCARD)
#endif

#define MAKE_ALIGN(s, bytes)    ((((UINT32)(s))+((bytes)-1)) & ~((UINT32)((bytes)-1)))

//define in GxSystem
extern void _sys_SetDisplayDim(int w, int h);
extern UINT32 COLOR_YUVD_2_RGBD(UINT32 yuvd);
extern UINT32 COLOR_RGBD_2_YUVD(UINT32 yuvd);

void _DL_Init(UINT32 DevID, UINT32 uiDispWidth, UINT32 uiDispHeight);
void _DL_Exit(void);

int _DL_DisplayInit(UINT32 uiDispWidth, UINT32 uiDispHeight);
int _DL_DisplayExit(void);

//layer buffer
INT32 _DL_BufferInit(UINT32 LayerID,
					UINT32 w, UINT32 h, UINT32 PxlFmt, UINT32 uiBufAddr, IRECT *pWin, UINT32 uiWinAttr);
void _DL_BufferExit(UINT32 LayerID);
INT32 _DL_BufferSwitch(UINT32 LayerID,
					  UINT32 w, UINT32 h, UINT16 PxlFmt, UINT32 uiBufAddr);

//layer src buffer
void _DL_SetSrcBuf(UINT32 LayerID, UINT32 fmt, UINT32 *addr);
//layer src window
void _DL_SetSrcWin(UINT32 LayerID, int x, int y, int w, int h, UINT32 loff, UINT32 attr);
//layer dest window
void _DL_SetDestWin(UINT32 LayerID, int x, int y, int w, int h);

//update trigger
void _DL_Dirty(UINT32 LayerID, BOOL bDirty);
void _DL_UpdateOutput(UINT32 DevID);

//update wait
void _DL_SetAutoWait(UINT32 DevID, BOOL bAutoWait);
//BOOL _DL_IsLoad(UINT32 DevID);
void _DL_Wait_Load(UINT32 DevID);
void _DL_Wait_VSync(UINT32 DevID);

//layer enable
INT32 _DL_SetEnable(UINT32 LayerID, BOOL bEnable);
BOOL _DL_GetEnable(UINT32 LayerID);

//layer palette
INT32 _DL_SetPalette(UINT32 LayerID, UINT32 nStart, UINT32 nCount, UINT32 *pData);
INT32 _DL_GetPalette(UINT32 LayerID, UINT32 nStart, UINT32 nCount, UINT32 *pData);
INT32 _DL_SetColorKey(UINT32 LayerID, UINT32 PxlFmt,UINT32 KeyOp,UINT32 ColorKey);

//layer blending
UINT32 _DL_GetOSDCk(UINT32 DevID, UINT32 OsdFmt, UINT32 KeyColor);
void _DL_SetOSDMix(UINT32 DevID, UINT32 uiAllCtrl, UINT8 KeyOp, UINT32 KeyIndex, UINT8 BlendOp, UINT8 uiConstAlpha,
				   UINT8 KeyOp2, UINT32 KeyIndex2, UINT8 BlendOp2, UINT8 uiConstAlpha2);
void _DL_SetVDOMix(UINT32 DevID, UINT32 uiAllCtrl, UINT8 KeyOp, UINT32 KeyColor, UINT8 BlendOp, UINT8 uiConstAlpha,
				   UINT8 KeyOp2, UINT32 KeyColor2, UINT8 BlendOp2, UINT8 uiConstAlpha2);
void _DL_SetCtrl(UINT32 DevID, UINT32 uiAllCtrl, UINT32 BackColor);

#endif //_DL_H_
