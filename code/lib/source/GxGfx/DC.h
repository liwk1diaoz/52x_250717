#ifndef _DC_H_
#define _DC_H_

#ifndef DISPLAY_CONTEXT

typedef struct _IMAGE_HEADER {
	UINT32      w;          //width
	UINT32      h;          //height
	UINT32      format;     //GXFM_IMAGETABLE_XXX
	UINT32      bpp;        //bpp
	UINT32      offset;
	UINT32      size;
} IMAGE_HEADER;


#define DC_REV_TEX      0
#define DC_REV_FMT      1
#define DC_REV_TK       2
#define DC_REV_BRUSH    3

typedef struct _PXLBLK {
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiPitch;
	UINT32 uiOffset;
} PXLBLK;

#define DCFLAG_SIGN                 0x00004758 //"GX"
#define DCFLAG_SIGN_MASK            0x0000FFFF
#define DCFLAG_LOCK                 0x00010000  //for image buffer as user direct access memory
#define DCFLAG_DRAW                 0x00020000  //for frame buffer as display drawing target

typedef struct _DISPLAY_CONTEXT {
	UINT32 uiFlag;
	UINT16 uiType;
	UINT16 uiPxlfmt;
	ISIZE size; //real size
	IRECT winRect;
	IPOINT origPt; //shift inside vir coord
	IPOINT lastPt;
	IPOINT thisPt;

	PXLBLK blk[4];
	UINT32 uiReserved[4];
	//uiReserved[0] = Device tex object
	//uiReserved[1] = Lock fmt | Device fmt
	//uiReserved[2] = pointer to current TK
	//uiReserved[3] = pointer to current brush
	ISIZE virSize; //vir size
	//IRECT realRect;
	IPOINT shiftVec; //shift to real coord
	IPOINT scaleVec; //scale to real coord
} DISPLAY_CONTEXT;

#endif

#define _DC(pDC)    ((DISPLAY_CONTEXT*)pDC)

UINT32 GxGfx_GetPixelSize(UINT16 uiPxlfmt, UINT8 iCn);

RESULT _DC_CalcPlaneRect(const DC *_pDC, UINT8 ucPlane, const IRECT *pRect, IRECT *pPlaneRect);
UINT32 _DC_CalcPlaneOffset(const DC *_pDC, UINT8 ucPlane, const IRECT *pPlaneRect);
BOOL _DC_CalcIntersectionRect(IRECT *pSrcRect, IPOINT *pDestPt, ISIZE *pDestSz, IRECT *pDestRect);
IRECT _DC_GetRect(const DC *_pDC); //this is virtual size
IRECT _DC_GetRealRect(const DC *_pDC); //this is real size

//BOOL POINT_IsInsideRect  (const IPOINT* pCheckPt, const IRECT* pRect);

//BOOL POINT_IsInsideRange  (const IPOINT* pCheckPt, const IPOINT* pStartPt, const IPOINT* pEndPt);

//BOOL RECT_IsInsideRect  (const IRECT* pCheckRect, const IRECT* pRect);

void RECT_CalcInterection(const IRECT *pSrcRectA, const IRECT *pSrcRectB, IRECT *pOutRect);

//void RECT_CalcUnion(const IRECT* pSrcRectA, const IRECT* pSrcRectB, IRECT* pOutRect);


#define _DC_IS_INDEX(pDC)   \
	((_DC(pDC)->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX)

#define _DC_MAPPING_COLOR(pDC, c, pTable)    \
	if((pTable) && _DC_IS_INDEX(pDC))  \
	{   \
		(c) = ((MAPPING_ITEM*)(pTable))[(c)];  \
	}   \


	void _DC_XFORM_POINT(const DC *pDC, IPOINT *pPt);
	void _DC_XFORM_RECT(const DC *pDC, IRECT *pRect);
	void _DC_XFORM2_RECT(const DC *pDC, IRECT *pRect);
	void _DC_SCALE_POINT(const DC *pDC, IPOINT *pPt);
	void _DC_SCALE_RECT(const DC *pDC, IRECT *pRect);
	void _DC_INVSCALE_POINT(const DC *pDC, IPOINT *pPt);
	void _DC_INVSCALE_RECT(const DC *pDC, IRECT *pRect);


#define MAKE_ALIGN(s, bytes)    ((((UINT32)(s))+((bytes)-1)) & ~((UINT32)((bytes)-1)))

//  ICON : DC.uiType = TYPE_ICON
//UINT8 _ICON_GetBpp      (const IMAGE_TABLE* pTable);
	RESULT _ICON_GetInfo(const IMAGE_TABLE *pTable, IVALUE id, IMAGE_HEADER *pImageHeader);
	RESULT _ICON_GetSize(const IMAGE_TABLE *pTable, IVALUE id, ISIZE *pSize);
	RESULT _ICON_MakeDC(DC *pIconDC, const IMAGE_TABLE *pTable, IVALUE id);

	RESULT _STR_MakeStr(const STRING_TABLE *pTable, IVALUE id, const TCHAR **ppStr);
	RESULT _STR_GetLen(const STRING_TABLE *pTable, IVALUE id, INT16 *pStringLen);

//  BITMAP : DC.uiType = TYPE_BITMAP
	/*
	RESULT BITMAP_Create    (BITMAP* pBm, UINT16 uiPxlfmt, UINT32 uiWidth, UINT32 uiHeight);
	void   BITMAP_Destroy   (BITMAP* pBm);
	RESULT BITMAP_LoadFile  (BITMAP* pBm, const char* pszFilename, UINT32 uiFilefmt);

	DC* BITMAP_GetDC        (BITMAP* pBm);
	void BITMAP_ReleaseDC   (BITMAP* pBm, DC* pBitmapDC);
	*/

#endif //_DC_H_
