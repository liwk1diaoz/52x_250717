#ifndef _MOVIESTAMP_H
#define _MOVIESTAMP_H
#include "hd_debug.h"

#include "kwrap/error_no.h"
#include "GxVideoFile.h"
#include "hdal.h"

#define MOVIE_ISP_LOG   DISABLE

#if (MOVIE_ISP_LOG == ENABLE)
#undef WATERLOGO_FUNCTION
#define WATERLOGO_FUNCTION              	DISABLE  // Enable/Disable waterlogo function
#undef MOVIE_MULTISTAMP_FUNC
#define MOVIE_MULTISTAMP_FUNC			DISABLE
#endif

// bit [0]
#define STAMP_ON                0x00000000
#define STAMP_OFF               0x00000001
#define STAMP_SWITCH_MASK       0x00000001

// bit [1]
#define STAMP_AUTO              0x00000000
#define STAMP_MANUAL            0x00000002
#define STAMP_OPERATION_MASK    0x00000002

// bit [3:2]
#define STAMP_DATE_TIME         0x00000000
#define STAMP_DATE_TIME_AMPM    0x00000004
#define STAMP_DATE              0x00000008
#define STAMP_TIME              0x0000000C
#define STAMP_DATE_TIME_MASK    0x0000000C

// bit [5:4]
#define STAMP_BOTTOM_LEFT       0x00000000
#define STAMP_BOTTOM_RIGHT      0x00000010
#define STAMP_TOP_LEFT          0x00000020
#define STAMP_TOP_RIGHT         0x00000030
#define STAMP_POSITION_MASK     0x00000030

// bit [7:6]
#define STAMP_YY_MM_DD          0x00000000
#define STAMP_DD_MM_YY          0x00000040
#define STAMP_MM_DD_YY          0x00000080
#define STAMP_DATE_FORMAT_MASK  0x000000C0


// bit [9]
#define STAMP_POS_NORMAL        0x00000000  // default
#define STAMP_POS_END           0x00000200
#define STAMP_POS_END_MASK      0x00000200

#define WATERLOGO_AUTO_POS      0xFFFFFFFF  // auto position for water logo

#define cst_prom0   21
#define cst_prom1   79
#define cst_prom2   29
#define cst_prom3   43
#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * (((INT32)r)-((INT32)g))) >> 8) + ((cst_prom2 * (((INT32)b)-((INT32)g))) >> 8) )
#define RGB_GET_U(r,g,b)    (128 + ((cst_prom3 * (((INT32)g)-((INT32)r))) >> 8) + ((((INT32)b)-((INT32)g)) >> 1) )
#define RGB_GET_V(r,g,b)    (128 + ((cst_prom0 * (((INT32)g)-((INT32)b))) >> 8) + ((((INT32)r)-((INT32)g)) >> 1) )

#define icst_prom0  103
#define icst_prom1  88
#define icst_prom2  183
#define icst_prom3  198
#define YUV_GET_R(y,u,v)    ( ((INT32)y) + ((INT32)(v)-128) + ((icst_prom0*((INT32)(v)-128))>>8) )
#define YUV_GET_G(y,u,v)    ( ((INT32)y) - ((icst_prom1*((INT32)(u)-128))>>8) - ((icst_prom2*((INT32)(v)-128))>>8) )
#define YUV_GET_B(y,u,v)    ( ((INT32)y) + ((INT32)(u)-128) + ((icst_prom3*((INT32)(u)-128))>>8) )
#define CLAMP_TO_BYTE(v)    (((v) < 0)?0:((v>255)?255:(v)))
#define YUV_GET_RGB(y,u,v,r,g,b)    \
	{   \
		r = YUV_GET_R(y,u,v);   r = CLAMP_TO_BYTE(r);   \
		g = YUV_GET_G(y,u,v);   g = CLAMP_TO_BYTE(g);   \
		b = YUV_GET_B(y,u,v);   b = CLAMP_TO_BYTE(b);   \
	}

#define STAMP_HEIGHT_MAX         64
#define MULTISTAMP_WIDTH_MAX        (1920)


typedef struct {
	UINT32    uiX;              ///< the X offset
	UINT32    uiY;              ///< the Y offset
} STAMP_POS;

typedef struct {
	UINT8     ucY;              ///< the Y value
	UINT8     ucU;              ///< the U value
	UINT8     ucV;              ///< the V value
	UINT8     rev;              ///< reserved
} STAMP_COLOR, *PSTAMP_COLOR;

typedef struct {
	UINT32      uiOffset;              ///< offset in database.
	UINT16      uiWidth;               ///< icon width.
	UINT16      uiHeight;              ///< icon height.
} ICON_HEADER, *PICON_HEADER;

typedef struct {
	ICON_HEADER const   *pIconHeader;       ///< point tp icon header
	UINT8       const   *pIconData;         ///< point to icon data
	UINT16              uiNumOfIcon;        ///< how many icon in this icon data base
	UINT8               uiBitPerPixel;      ///< bit perpixel of this icon DB, it can be 1, 2 or 4 bits per pixel
	UINT8               uiDrawStrOffset;    ///< Data base first item offset
} ICON_DB, *PICON_DB;

typedef struct {
	char    *pi8Str;            ///< The string of date.
	ICON_DB const *pDataBase;   ///< The font database for date.
	UINT32  ui32FontWidth;      ///< The max width of font
	UINT32  ui32FontHeight;     ///< The max height of font
	UINT32  ui32DstHeight;      ///< The destination height of font
	STAMP_COLOR Color[3];       ///< The stamp color (0: background, 1: frame, 2: foreground)
} STAMP_INFO, *PSTAMP_INFO;

typedef struct {
	UINT32  uiYAddr;            ///< Stamp buffer Y address
	UINT32  uiUAddr;            ///< Stamp buffer U address
	UINT32  uiVAddr;            ///< Stamp buffer V address
	UINT32  uiYLineOffset;      ///< Stamp buffer Y line offset
	UINT32  uiUVLineOffset;     ///< Stamp buffer UV line offset
	UINT32  uiFormat;           ///< Stamp YUV format
} STAMP_BUFFER, *PSTAMP_BUFFER;

typedef struct {
	UINT32  uiXPos;                     ///< WaterLogo x position
	UINT32  uiYPos;                     ///< WaterLogo y position
	UINT32  uiWidth;                    ///< WaterLogo width
	UINT32  uiHeight;                   ///< WaterLogo height
	UINT32  uiWaterLogoAddr;           ///< WaterLogo buffer address
	UINT32  uilayer;
} WATERLOGO_BUFFER, *PWATERLOGO_BUFFER;
typedef struct {
    UINT32                   pool_va;
    UINT32                   pool_pa;
} STAMP_ADDR_INFO;

extern const WATERLOGO_BUFFER g_WaterLogo_1440;
extern const WATERLOGO_BUFFER g_WaterLogo_640;



// Date stamp for movie
extern void     MovieStamp_Enable(void);
extern void     MovieStamp_Disable(void);
extern void     MovieStamp_Setup(UINT32 uiVidEncId, UINT32 uiFlag, UINT32 uiImageWidth, UINT32 uiImageHeight, WATERLOGO_BUFFER *pWaterLogoBuf);
extern void     MovieStamp_SetColor(UINT32 uiVidEncId, PSTAMP_COLOR pStampColorBg, PSTAMP_COLOR pStampColorFr, PSTAMP_COLOR pStampColorFg);
extern void     MovieStamp_UpdateData(void);
extern UINT32 MovieStamp_TriggerUpdateChk(void);
extern void     MovieStamp_SetBuffer(UINT32 uiVidEncId, UINT32 uiAddr, UINT32 uiSize, UINT32 Width, UINT32 Height);
extern UINT32 MovieStamp_CalcBufSize(UINT32 Width, UINT32 Height,WATERLOGO_BUFFER *pWaterLogoBuf);
extern UINT32 MovieStamp_GetBufAddr(UINT32 uiVidEncId, UINT32 blk_size);
extern void MovieStamp_DestroyBuff(void);
extern ER MovieStamp_VsConfig(UINT32 uiVidEncId, UINT32 uiOSDWidth, UINT32 uiOSDHeight,WATERLOGO_BUFFER *pWaterLogoBuf);
extern INT32 MovieStamp_VsUpdateOsd(HD_PATH_ID path_id, UINT32 enable, UINT32 layer, UINT32 region, UINT32 uiX, UINT32 uiY, UINT32 uiOSDWidth, UINT32 uiOSDHeight, void *picture);
extern void MovieStamp_VsClose(void);
extern void MovieStamp_VsStop(UINT32 uiVEncOutPortId, UINT32 StampPath);
extern void MovieStamp_VsAllocFontBuf(UINT32 uiVidEncId, UINT32 width, UINT32 height);
extern void MovieStamp_VsSwapOsd(UINT32 uiVidEncId, UINT32 StampPath, UINT32 VsHDPathId);
extern void MovieStamp_VsFontConfig(UINT32 uiVidEncId);
extern BOOL MovieStamp_IsEnable(void);
extern UINT32 MovieStamp_OsgQueryBufSize(UINT32 Width, UINT32 Height);
extern void MovieStamp_DestroyMultiStampBuff(void);
extern void MovieStamp_DrawMultiStamp(UINT32 uiVidEncId, UINT32 CntId, UPOINT *Pos, char *pStr, UINT32 bInitStart);
extern ER MovieStamp_VsAllocWaterLogoOsgBuf(UINT32 uiVEncOutPortId, UINT32 WaterLogoWidth, UINT32 WaterLogoHeight);
extern UINT32 MovieStamp_GetRawEncVirtualPort(UINT32 RecId);
extern UINT32 MovieStamp_IsRawEncVirPort(UINT32 PortId);
extern void MovieStamp_DestroyMultiWaterLogoBuff(void);
extern void MovieStamp_DrawMultiWaterLogo(UINT32 uiVEncOutPortId, UINT32 CntId, WATERLOGO_BUFFER *sWaterLogo, UINT32 bInitStart);
extern int MovieStamp_GetMultiWaterLogoSource(UINT32 CntId,WATERLOGO_BUFFER *waterSrc);

extern void UI_GfxInitLite(void);
#endif
