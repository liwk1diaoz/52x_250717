#include "GxGfx/GxGfx.h"
#include "GxGfx_int.h"
#include "DC.h"
#include "TextRender.h"
#include "TextString.h"

///////////////////////////////////////////////////////////////////////////////

#define GX_TEST_NOALIGN         0


ISIZE _gDC_Text_size = {0, 0};
IPOINT _gDC_Text_loc = {0, 0};

IPOINT TextFormat_GetLastLoc(void)
{
	return _gDC_Text_loc;
}
ISIZE TextFormat_GetLastSize(void)
{
	return _gDC_Text_size;
}

_INLINE void TextFormat_MarkLoc(TextFormat *pFormat)
{
	_gDC_Text_loc = pFormat->Pt;
}
_INLINE void TextFormat_ClearAccSize(TextFormat *pFormat)
{
	SIZE_Set(&(pFormat->szAcc), 0, 0);
}
_INLINE ISIZE TextFormat_GetAccSize(TextFormat *pFormat)
{
	return (pFormat->szAcc);
}
_INLINE void TextFormat_IncAccSize(TextFormat *pFormat, LVALUE w, LVALUE h)
{
	pFormat->szAcc.w += w;
	if (pFormat->szAcc.h < h) {
		pFormat->szAcc.h = h;
	}
}
_INLINE void TextFormat_DecAccSize(TextFormat *pFormat, LVALUE w, LVALUE h)
{
	pFormat->szAcc.w -= w;
}
_INLINE void TextFormat_ClearAccHeight(TextFormat *pFormat)
{
	pFormat->szAcc.h = 0;
}
_INLINE void TextFormat_SetBeginAtHere(TextFormat *pFormat)
{
	pFormat->uiBegin = pFormat->uiEnd; //?
	pFormat->outBuf = (TCHAR *)_String_GetCharPtr((TCHAR *)(pFormat->tstrBuf), pFormat->uiBegin);
}
_INLINE void TextFormat_SetBeginAtEndOfWord(TextFormat *pFormat)
{
	pFormat->uiBegin = pFormat->uiEndOfWord;
	pFormat->outBuf = (TCHAR *)_String_GetCharPtr((TCHAR *)(pFormat->tstrBuf), pFormat->uiBegin);
}
_INLINE void TextFormat_MoveBegin(TextFormat *pFormat, INT16 iOffset)
{
	if ((INT32)(pFormat->uiBegin + iOffset) >= (INT32)_gGfxStringBuf_Size) {
		pFormat->uiBegin = _gGfxStringBuf_Size - 1;
	} else {
		pFormat->uiBegin += iOffset;
	}
	pFormat->outBuf = (TCHAR *)_String_GetCharPtr((TCHAR *)(pFormat->tstrBuf), pFormat->uiBegin);
}
_INLINE void TextFormat_MoveEnd(TextFormat *pFormat, INT16 iOffset)
{
	if ((INT32)(pFormat->uiEnd + iOffset) >= (INT32)_gGfxStringBuf_Size) {
		pFormat->uiEnd = _gGfxStringBuf_Size - 1;
	} else {
		pFormat->uiEnd += iOffset;
	}
}
_INLINE void TextFormat_SetEndOfWordAtHere(TextFormat *pFormat)
{
	pFormat->uiEndOfWord = pFormat->uiEnd; //?
}
_INLINE void TextFormat_DelEndOfWord(TextFormat *pFormat)
{
	//TODO : 如果是-1會發生什麼事?
	pFormat->uiEndOfWord = (INT16) - 1;
}
_INLINE void TextFormat_SetEndOfLineAtHere(TextFormat *pFormat)
{
	pFormat->uiEndOfLine = pFormat->uiEnd;
}
_INLINE void TextFormat_SetEndOfLineAtEndOfWord(TextFormat *pFormat)
{
	pFormat->uiEndOfLine = pFormat->uiEndOfWord;
}
_INLINE void TextFormat_DelEndOfLine(TextFormat *pFormat)
{
	//TODO : 如果是-1會發生什麼事?
	pFormat->uiEndOfLine = (INT16) - 1;
}
_INLINE INT16 TextFormat_GetLocateX(TextFormat *pFormat)
{
	return (INT16)pFormat->Pt.x;
}
_INLINE void TextFormat_SetLocateX(TextFormat *pFormat, LVALUE iX)
{
	pFormat->Pt.x = iX;
}
_INLINE void TextFormat_HomeLocateX(TextFormat *pFormat)
{
	//move x posiiton to left
	pFormat->Pt.x = pFormat->pDestRect->x;
}
_INLINE void TextFormat_MoveLocateX(TextFormat *pFormat, INT16 iOffset)
{
	//move x posiiton
	pFormat->Pt.x += iOffset;
}
_INLINE INT16 TextFormat_GetLocateY(TextFormat *pFormat)
{
	return (INT16)pFormat->Pt.y;
}
_INLINE void TextFormat_SetLocateY(TextFormat *pFormat, LVALUE iY)
{
	pFormat->Pt.y = iY;
}
_INLINE void TextFormat_HomeLocateY(TextFormat *pFormat)
{
	//move y posiiton to top
	pFormat->Pt.y = pFormat->pDestRect->y;
}
_INLINE void TextFormat_MoveLocateY(TextFormat *pFormat, INT16 iOffset)
{
	//move y posiiton
	pFormat->Pt.y += iOffset;
}
_INLINE void TextFormat_GetLineSize(TextFormat *pFormat, ISIZE *pSize)
{
	pSize->w = (pFormat->Pt.x - pFormat->pDestRect->x);
	pSize->h = pFormat->szAcc.h;
}
static void TextFormat_ClearBuf(TextFormat *pFormat)
{
	pFormat->uiBegin = 0;
	pFormat->uiEnd = 0;
	_String_Resize(pFormat->tstrBuf, pFormat->uiEnd);
	pFormat->outBuf = _String_GetCharPtr(pFormat->tstrBuf, pFormat->uiBegin);
}
static INT16 TextFormat_GetBufCount(TextFormat *pFormat)
{
	return (INT16)(pFormat->uiEnd - pFormat->uiBegin);
}
void TextFormat_Init(TextFormat *pFormat, TextRender *pRender, UINT32 ScreenObj, IRECT *pDestRect)
{
	//pFormat->pDestDC = pDestDC;
	//pFormat->pStyle = DC_GetStyle();
	pFormat->pDestRect = pDestRect;
	pFormat->nFlag = 1;
	pFormat->tstrBuf = (TCHAR *)tr_str;
	pFormat->pRender = pRender;
	pFormat->ScreenObj = ScreenObj;
}
static void TextFormat_DoBegin(TextFormat *pFormat)
{
	(*pFormat->pRender->pfn_Begin)(pFormat->pRender, pFormat->ScreenObj);

	//initial string buffer
	TextFormat_MarkLoc(pFormat);
	TextFormat_ClearAccSize(pFormat);
	TextFormat_ClearBuf(pFormat);
	TextFormat_DelEndOfLine(pFormat);
	TextFormat_DelEndOfWord(pFormat);

	//initial output location
	TextFormat_HomeLocateX(pFormat);
	TextFormat_HomeLocateY(pFormat);
}
static void TextFormat_DoEnd(TextFormat *pFormat)
{
	(*pFormat->pRender->pfn_End)(pFormat->pRender);
}
void TextFormat_Exit(TextFormat *pFormat)
{
	//pFormat->pDestDC = 0;
	//pFormat->pStyle = 0;
	pFormat->pDestRect = 0;
	pFormat->nFlag = 0;
	pFormat->pRender = 0;
	pFormat->ScreenObj = 0;
}
static RESULT TextFormat_AddChar(TextFormat *pFormat, TCHAR_VALUE c)
{
	RESULT r = GX_OK;
	IVALUE id;
	//IMAGE_TABLE* pFontTable;
	ISIZE szFont;
	LVALUE iCharGap;

	if ((INT32)TextFormat_GetBufCount(pFormat) == (INT32)(_gGfxStringBuf_Size - 1)) {
		return FALSE;
	}
	//pFontTable = (IMAGE_TABLE*)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)

	_String_SetChar(pFormat->tstrBuf, pFormat->uiEnd, c);
	TextFormat_MoveEnd(pFormat, +1);
	_String_Resize(pFormat->tstrBuf, pFormat->uiEnd);

	id = c;
	SIZE_Set(&szFont, 0, 0);
	r = (*pFormat->pRender->pfn_GetCharSize)(id, &szFont, FALSE);
	iCharGap = (LVALUE)GxGfx_Get(BR_TEXTLETTERSPACE);

	if (r == GX_OK) {
		//accmulate size of line
		TextFormat_IncAccSize(pFormat, szFont.w + iCharGap, szFont.h);
	} else {
		//accmulate size of line
		TextFormat_IncAccSize(pFormat, 16, 24); // for draw dummy character by _GxGfx_DummyImage()
	}
	return r;
}
_INLINE TCHAR_VALUE TextFormat_GetCharAtHeader(TextFormat *pFormat)
{
//    return pFormat->tstrBuf[pFormat->uiBegin];
	return _String_GetChar(pFormat->tstrBuf, pFormat->uiBegin);
}
static BOOL TextFormat_DelCharAtHeader(TextFormat *pFormat)
{
	RESULT r = GX_OK;
	IVALUE id;
	//IMAGE_TABLE* pFontTable;
	ISIZE szFont;
	TCHAR_VALUE c;
	LVALUE iCharGap;

	if (TextFormat_GetBufCount(pFormat) == 0) {
		return FALSE;
	}

	//pFontTable = (IMAGE_TABLE*)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)
	//coverity[check_return]
	c = _String_GetChar(pFormat->tstrBuf, pFormat->uiBegin);
	TextFormat_MoveBegin(pFormat, +1);

	id = c;
	SIZE_Set(&szFont, 0, 0);
	r = (*pFormat->pRender->pfn_GetCharSize)(id, &szFont, FALSE);
	iCharGap = (LVALUE)GxGfx_Get(BR_TEXTLETTERSPACE);

	if (r == GX_OK) {
		//update accmulate size of line
		TextFormat_DecAccSize(pFormat, szFont.w + iCharGap, szFont.h);
	} else {
		//update accmulate size of line
		TextFormat_DecAccSize(pFormat, 16, 24); // for draw dummy character by _GxGfx_DummyImage()
	}

	return TRUE;
}
_INLINE TCHAR_VALUE TextFormat_GetCharAtTail(TextFormat *pFormat)
{
//    return pFormat->tstrBuf[pFormat->uiEnd-1];
	INT16 iLen2;
	iLen2 = pFormat->uiEnd - 1;
	return _String_GetChar(pFormat->tstrBuf, iLen2);
}
static BOOL TextFormat_DelCharAtTail(TextFormat *pFormat)
{
	RESULT r = GX_OK;
	IVALUE id;
	//IMAGE_TABLE* pFontTable;
	ISIZE szFont;
	TCHAR_VALUE c;
	INT16 iLen2;
	LVALUE iCharGap;

	if (TextFormat_GetBufCount(pFormat) == 0) {
		return FALSE;
	}

	//pFontTable = (IMAGE_TABLE*)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)

	iLen2 = pFormat->uiEnd - 1;
	//coverity[check_return]
	c = _String_GetChar(pFormat->tstrBuf, iLen2);
	TextFormat_MoveEnd(pFormat, -1);
	_String_Resize(pFormat->tstrBuf, pFormat->uiEnd);

	id = c;
	SIZE_Set(&szFont, 0, 0);
	r = (*pFormat->pRender->pfn_GetCharSize)(id, &szFont, FALSE);
	iCharGap = (LVALUE)GxGfx_Get(BR_TEXTLETTERSPACE);

	if (r == GX_OK) {
		//update accmulate size of line
		TextFormat_DecAccSize(pFormat, szFont.w + iCharGap, szFont.h);
	} else {
		//update accmulate size of line
		TextFormat_DecAccSize(pFormat, 16, 24);  // for draw dummy character by _GxGfx_DummyImage()
	}

	return TRUE;
}


static void TextFormat_DoLine(TextFormat *pFormat, LVALUE iMaxCharHeight, UINT32 uiAlignFlag, ISIZE *pszLine)
{
	RESULT r = GX_OK;
	LVALUE iCharGap;
	//MAPPING_ITEM* pMapping;
	BOOL bDraw;
	const TCHAR *pCurChar;
	//const TCHAR* pCurSrc;
	TCHAR *pszDest;
	INT16 iLen;
	INT16 i;
	IPOINT Move = {0, 0};
	LVALUE iBaseX, iRightX;

	bDraw = (pFormat->ScreenObj != 0);
	iCharGap = (LVALUE)GxGfx_Get(BR_TEXTLETTERSPACE);

	pszDest = (TCHAR *)_gDC_Text_line;
	iLen = pFormat->uiEndOfLine - pFormat->uiBegin;

	pCurChar = _String_GetConstCharPtr(pFormat->tstrBuf, pFormat->uiBegin);
	_String_N_Copy(pszDest, pCurChar, iLen);

	Move.x = TextFormat_GetLocateX(pFormat);
	Move.y = TextFormat_GetLocateY(pFormat);
	//save base X of line
	iBaseX = Move.x;
	if (pszLine) {
		iRightX = iBaseX + pszLine->w;
	} else {
		iRightX = iBaseX;
	}

	//_DC_ParseString(pFormat->pDestDC, pszDest, iLen, iMaxCharHeight, iCharGap, uiFlag, &Move);
	i = 0;
	//pCurSrc = _String_GetConstCharPtr(pszDest, i);

	if ((uiAlignFlag & STREAMALIGN_MASK) && pszLine) { //draw to DC (backward order) -> Ex: Hebrew
		while (i < iLen) {
			ESCCMD cmd;
			i = (*pFormat->pRender->pfn_ParseStringCmd)(pszDest, i, &cmd);

			//bEscIcon
			if (cmd.bDecodeImage) {
				if (!bDraw) { //do not draw
					ISIZE szImage;
					r = (*pFormat->pRender->pfn_GetCharSize)(cmd.id, &szImage, cmd.bEscIcon);
					if (r != GX_OK) {
						SIZE_Set(&szImage, 16, 24);  // for draw dummy character by _GxGfx_DummyImage()
					}

					//move x posiiton to next char (right of current char)
					Move.x += ((LVALUE)szImage.w + iCharGap);
				} else { //draw to DC (backward order) -> Ex: Hebrew
					LVALUE iBaseY;
					//UINT16 lAlign;
					ISIZE szImage;

					LVALUE dX;

					//save base Y of line
					iBaseY = Move.y;

#if !(GX_TEST_NOALIGN)
					r = (*pFormat->pRender->pfn_GetCharSize)(cmd.id, &szImage, cmd.bEscIcon);
					if (r != GX_OK) {
						SIZE_Set(&szImage, 16, 24);  // for draw dummy character by _GxGfx_DummyImage()
					}
					if (cmd.bEscIcon) { //esc icon
						//always align to DSF_LA_CENTER
						Move.y += ((iMaxCharHeight - (LVALUE)szImage.h) >> 1);
					} else { //char icon
						uiAlignFlag = (uiAlignFlag & LETTERALIGN_MASK);
						if (uiAlignFlag == LETTERALIGN_BOTTOM) {
							Move.y += (iMaxCharHeight - (LVALUE)szImage.h);
						} else if (uiAlignFlag == LETTERALIGN_BASELINE) {
							Move.y += (iMaxCharHeight - (LVALUE)szImage.h) - 2;
						}
					}
#endif
					dX = Move.x - iBaseX;
					r = (*pFormat->pRender->pfn_DrawChar)(pFormat->ScreenObj, iRightX - dX - szImage.w, Move.y, cmd.id, cmd.bEscIcon);

					//restore base Y of line
					Move.y = iBaseY;
					//move x posiiton to next char (right of current char)
					Move.x += ((LVALUE)szImage.w + iCharGap);
				}
			}

			//pCurSrc = _String_GetConstCharPtr(pszDest, i);
		}
	} else {
		while (i < iLen) {
			ESCCMD cmd;
			i = (*pFormat->pRender->pfn_ParseStringCmd)(pszDest, i, &cmd);

			//bEscIcon
			if (cmd.bDecodeImage) {
				if (!bDraw) { //do not draw
					ISIZE szImage;
					r = (*pFormat->pRender->pfn_GetCharSize)(cmd.id, &szImage, cmd.bEscIcon);
					if (r != GX_OK) {
						SIZE_Set(&szImage, 16, 24);  // for draw dummy character by _GxGfx_DummyImage()
					}

					//move x posiiton to next char (right of current char)
					Move.x += ((LVALUE)szImage.w + iCharGap);
				} else { //draw to DC (forward order)
					LVALUE iBaseY;
					//UINT16 lAlign;
					ISIZE szImage;

					//save base Y of line
					iBaseY = Move.y;

#if !(GX_TEST_NOALIGN)
					r = (*pFormat->pRender->pfn_GetCharSize)(cmd.id, &szImage, cmd.bEscIcon);
					if (r != GX_OK) {
						SIZE_Set(&szImage, 16, 24);  // for draw dummy character by _GxGfx_DummyImage()
					}
					if (cmd.bEscIcon) { //esc icon
						//always align to DSF_LA_CENTER
						Move.y += ((iMaxCharHeight - (LVALUE)szImage.h) >> 1);
					} else { //char icon
						uiAlignFlag = (uiAlignFlag & LETTERALIGN_MASK);
						if (uiAlignFlag == LETTERALIGN_BOTTOM) {
							Move.y += (iMaxCharHeight - (LVALUE)szImage.h);
						} else if (uiAlignFlag == LETTERALIGN_BASELINE) {
							Move.y += (iMaxCharHeight - (LVALUE)szImage.h) - 2;
						}
					}
#endif
					r = (*pFormat->pRender->pfn_DrawChar)(pFormat->ScreenObj, Move.x, Move.y, cmd.id, cmd.bEscIcon);

					//restore base Y of line
					Move.y = iBaseY;
					//move x posiiton to next char (right of current char)
					Move.x += ((LVALUE)szImage.w + iCharGap);
				}
			}

			//pCurSrc = _String_GetConstCharPtr(pszDest, i);
		}
	}

	TextFormat_SetLocateX(pFormat, Move.x);
	TextFormat_SetLocateY(pFormat, Move.y);
	/*
	    i = 0; pCurSrc = _String_GetCharPtr(pszDest, i);
	    while(i < iLen)
	    {
	        EscCmd.uiFlag = 0;
	        c = _String_GetChar(pCurSrc, 0);

	        if((c == '^') || (c == '$'))
	        {
	            if(c == '^')
	                VAR_SETTYPE(VARTYPE_STATE); //'^'
	            else
	                VAR_SETTYPE(VARTYPE_STACK); //'$'

	            iCount = _String_DecodeCommandWithEscValue(pCurSrc, &EscCmd);

	            i+=iCount;  pCurSrc = _String_GetCharPtr(pszDest, i);
	        }

	        bDecodeImage = FALSE;
	        bEscIcon = FALSE;
	        if(EscCmd.uiFlag == (CTRL_VALUE_CMD))
	        {
	            //kkk
	            BR_Set(EscCmd.uiCommand, (PVALUE)(EscCmd.uiValue));
	        }
	        else if((EscCmd.uiFlag & (CTRL_VALUE_FMT | CTRL_VALUE_IMAGEID)) == (CTRL_VALUE_FMT | CTRL_VALUE_IMAGEID))
	        {
	            //draw esc icon ==>id by pImageTable

	            iCharGap = (LVALUE)GxGfx_Get(BR_TEXTLETTERSPACE);

	            if(EscCmd.uiFlag & CTRL_VALUE_PTR)
	            {
	                id = *(IVALUE*)EscCmd.uiValue; //piid
	            }
	            else
	            {
	                id = (IVALUE)EscCmd.uiValue; //iid
	            }

	            bDecodeImage = TRUE;
	            bEscIcon = TRUE;
	        }
	        else //EscCmd.uiFlag == 0 (normal char)
	        {
	            //draw char icon ==>uiFontID by pFontTable

	            iCharGap = (LVALUE)GxGfx_Get(BR_TEXTLETTERSPACE);

	            id = c;

	            i++; pCurSrc = _String_GetCharPtr(pszDest, i);

	            bDecodeImage = TRUE;
	        }

	        if(bDecodeImage)
	        {
	            if(!bDraw) //do not draw
	            {
	                ISIZE szImage;
	                if(bEscIcon == TRUE)
	                {
	                    DC_GetEscImageSize(pFormat->pDestDC, id, &szImage);
	                }
	                else
	                {
	                    DC_GetCharSize(pFormat->pDestDC, id, &szImage);
	                }

	                //move x posiiton to next char (right of current char)
	                TextFormat_MoveLocateX(pFormat, (LVALUE)szImage.w + iCharGap);
	            }
	            else  //draw to DC
	            {
	                LVALUE iBaseY;
	                UINT16 lAlign;
	                ISIZE szImage;

	                //save base Y of line
	                iBaseY = TextFormat_GetLocateY(pFormat);

	#if !(GX_TEST_NOALIGN)
	                if(bEscIcon == TRUE)
	                {
	                    DC_GetEscImageSize(pFormat->pDestDC, id, &szImage);
	                }
	                else
	                {
	                    DC_GetCharSize(pFormat->pDestDC, id, &szImage);
	                }
	                if(bEscIcon) //esc icon
	                {
	                    //always align to DSF_LA_CENTER
	                    TextFormat_MoveLocateY
	                        (pFormat, (iMaxCharHeight - (LVALUE)szImage.h)>>1);
	                }
	                else //char icon
	                {
	                    lAlign = (uiFlag & LETTERALIGN_MASK);
	                    if(lAlign == LETTERALIGN_BOTTOM)
	                    {
	                        TextFormat_MoveLocateY
	                            (pFormat, iMaxCharHeight - (LVALUE)szImage.h);
	                    }
	                    else if(lAlign == LETTERALIGN_BASELINE)
	                    {
	                        TextFormat_MoveLocateY
	                            (pFormat, (iMaxCharHeight - (LVALUE)szImage.h)-2);
	                    }
	                }
	#endif
	                if(bEscIcon == TRUE)
	                {
	                    if(DC_EscImage(pFormat->pDestDC, pFormat->Pt.x, pFormat->Pt.y, id) != GX_OK)
	                        return;
	                }
	                else //text icon
	                {
	                    if(DC_TextChar(pFormat->pDestDC, pFormat->Pt.x, pFormat->Pt.y, id) != GX_OK)
	                        return;
	                }

	                //restore base Y of line
	                TextFormat_SetLocateY(pFormat, iBaseY);
	                //move x posiiton to next char (right of current char)
	                TextFormat_MoveLocateX(pFormat, (LVALUE)szImage.w + iCharGap);
	            }
	        }
	    }*/
}

#define IsReturn(n)     (((n) == '\n'))
#define IsSpace(n)      (((n) == ' ') || ((n) == '\t') || ((n) == '\r') || ((n) == 0))

//#define IsShowSpace(n)  (((n) == ' '))

static RESULT TextFormat_DoLineByAlign(TextFormat *pFormat, LVALUE iLineHeight, UINT32 uiFlag, ISIZE *pSize)
{
	ISIZE szLine;
	UINT32 hAlign;
	BOOL bDraw;
	//DC* pDestDC;
	UINT32 ScreenObj;

#if (GX_TEST_NOALIGN)
	TextFormat_HomeLocateX(pFormat);
	//draw line
	TextFormat_DoLine(pFormat, 0, 0, 0);

	TextFormat_GetLineSize(pFormat, &szLine);
	//  szLine.w => auto calculate maximal width
	//  szLine.h => auto calculate minimal height

	//subtract output width from accumlate width
	TextFormat_DecAccSize(pFormat, szLine.w, szLine.h);
	//reset accumlate height
	TextFormat_ClearAccHeight(pFormat);

#else
	//pDestDC = pFormat->pDestDC; //save DC
	ScreenObj = pFormat->ScreenObj;

	//pFormat->pDestDC = 0; //not draw
	pFormat->ScreenObj = 0; //not draw
	//move x posiiton to left
	TextFormat_HomeLocateX(pFormat);
	//evaluate line size
	TextFormat_DoLine(pFormat, 0, 0, 0);
	//update line size
	TextFormat_GetLineSize(pFormat, &szLine);
	//  szLine.w => auto calculate maximal width
	//  szLine.h => auto calculate minimal height
	_DC_INVSCALE_POINT((DC *)ScreenObj, (IPOINT *)&szLine);

	//subtract output width from accumlate width
	TextFormat_DecAccSize(pFormat, szLine.w, szLine.h);
	//reset accumlate height
	//TextFormat_ClearAccHeight(pFormat);  //remove this for multi-line layout bug - 2011.3.7

	if (iLineHeight > 0) {
		//force to apply user assign fixed line height
		szLine.h = iLineHeight;
	}
	if (pSize) {
		*pSize = szLine;
	}

	//pFormat->pDestDC = pDestDC; //restore DC
	pFormat->ScreenObj = ScreenObj; //restore DC
	//bDraw = (pFormat->pDestDC!=0);
	bDraw = (pFormat->ScreenObj != 0);
	if (bDraw) { //draw to DC
		IPOINT deltaPt = {0, 0};
		//move x posiiton to alignment
		TextFormat_HomeLocateX(pFormat);
		hAlign = (uiFlag & ALIGN_H_MASK);
		if (hAlign == ALIGN_RIGHT) {
			deltaPt.x = (pFormat->pDestRect->w - szLine.w);
		} else if (hAlign == ALIGN_CENTER) {
			deltaPt.x = ((pFormat->pDestRect->w - szLine.w) >> 1);
		}
		TextFormat_MoveLocateX(pFormat, (INT16)(deltaPt.x));
		//draw line
		TextFormat_DoLine(pFormat, szLine.h, uiFlag, &szLine);
	}
#endif

	//move y posiiton to next line
	TextFormat_MoveLocateY(pFormat, (INT16)szLine.h);

	return GX_OK;
}

static RESULT TextFormat_DelAllSpaceAtHeader(TextFormat *pFormat)
{
	RESULT r = GX_OK;
	BOOL bIsSpace = TRUE;
	TCHAR_VALUE c;
	//remove all space chars at header of line
	while (TextFormat_GetBufCount(pFormat) && bIsSpace) {
		c = TextFormat_GetCharAtHeader(pFormat);
		//check if last char is space char
		bIsSpace = IsSpace(c);
		if (bIsSpace) {
			TextFormat_DelCharAtHeader(pFormat); //remove space char at header
		}
	}
	return r;
}

static RESULT TextFormat_DelAllSpaceAtTail(TextFormat *pFormat)
{
	RESULT r = GX_OK;
	BOOL bIsSpace = TRUE;
	TCHAR_VALUE c;
	//remove all space chars at tail of line
	while (TextFormat_GetBufCount(pFormat) && bIsSpace) {
		c = TextFormat_GetCharAtTail(pFormat);
		//check if last char is space char
		bIsSpace = IsSpace(c);
		if (bIsSpace) {
			TextFormat_DelCharAtTail(pFormat);    //remove space char at tail
		}
	}
	return r;
}

RESULT TextFormat_DoMultiLine(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag, ISIZE *pSize)
{
	RESULT r = GX_OK;
	INT16 iCount;
	TCHAR_VALUE c;
	BOOL bIsSpace, bIsReturn;
	BOOL bLastCharIsNormalChar;
	BOOL bIsNewLineAndSpace;
	BOOL bSkipThisChar, bApplyNewLine;
	ISIZE szLine;
	UINT16 uiBufCount;
	LVALUE iLineHeight;
	ISIZE szMultiLine;

	if (!pFormat) {
		return GX_NULL_POINTER;
	}

	szMultiLine.w = 0;
	szMultiLine.h = 0;
	szLine.w = 0;
	szLine.h = 0;

	TextFormat_SetBeginAtHere(pFormat);
	TextFormat_DelEndOfLine(pFormat); //del end of line
	TextFormat_DelEndOfWord(pFormat); //del end of word
	iLineHeight = (LVALUE)GxGfx_Get(BR_TEXTLINEHEIGHT);

	bIsNewLineAndSpace = TRUE;
	bLastCharIsNormalChar = FALSE;

	iCount = 0;
	c = _String_GetChar(pszSrc, iCount);

	while (c) {
		bSkipThisChar = FALSE;
		bApplyNewLine = FALSE;

		//check if current char is space char
		bIsSpace = IsSpace(c);
		//check if current char is return char
		bIsReturn = IsReturn(c);

		if (bIsSpace) {
			if (bIsNewLineAndSpace) {
				bSkipThisChar = TRUE;    //skip all space char at header of line
			}

			if (bLastCharIsNormalChar) {
				//Last char is end of a word
				TextFormat_SetEndOfWordAtHere(pFormat); //mark end of word
			} else {
				//continue space
			}
		} else if (bIsReturn) {
			//remove all space chars at tail of line
			TextFormat_DelAllSpaceAtTail(pFormat);

			//here is end of a word
			TextFormat_SetEndOfWordAtHere(pFormat); //mark end of word

			bSkipThisChar = TRUE; //skip return char

			//if get a return char, break line here
			bApplyNewLine = TRUE;
		} else {
			bIsNewLineAndSpace = FALSE;
		}

		bLastCharIsNormalChar = (!bIsSpace && !bIsReturn);

		//TODO: bSkipThisChar = TRUE, if it is Esc command

		if (!bSkipThisChar) {
			RESULT tr;
			tr = TextFormat_AddChar(pFormat, c);
			if (tr != GX_OK) {
				r = tr;    //put to r
			}

			//if width is large then rect width, break line here
			szLine = TextFormat_GetAccSize(pFormat);
			if (szLine.w > pFormat->pDestRect->w) {
				bApplyNewLine = TRUE;
			}
		}

		if (bApplyNewLine) {
			TextFormat_SetEndOfLineAtEndOfWord(pFormat);

			//output line [from begin to end of line]
			TextFormat_DoLineByAlign(pFormat, iLineHeight, uiFlag, &szLine);
			if (szMultiLine.w < szLine.w) {
				szMultiLine.w = szLine.w;
			}
			szMultiLine.h += szLine.h;

			//FIXED - 2009/10/19 - begin
			if (!bLastCharIsNormalChar) {
				//try to skip additional space char at header of next line
				bIsNewLineAndSpace = TRUE;
			}
			//FIXED - 2009/10/19 - end

			TextFormat_SetBeginAtEndOfWord(pFormat);

			//remove all space chars at header of line
			TextFormat_DelAllSpaceAtHeader(pFormat);

			TextFormat_DelEndOfLine(pFormat); //del end of line
			TextFormat_DelEndOfWord(pFormat); //del end of word
		}

		iCount++;
		c = _String_GetChar(pszSrc, iCount);
	}

	uiBufCount = TextFormat_GetBufCount(pFormat);
	if (uiBufCount) {
		//remove all space chars at tail of line
		TextFormat_DelAllSpaceAtTail(pFormat);

		TextFormat_SetEndOfLineAtHere(pFormat); //mark end of word

		//output line [from begin to end of line]
		TextFormat_DoLineByAlign(pFormat, iLineHeight, uiFlag, &szLine);
		if (szMultiLine.w < szLine.w) {
			szMultiLine.w = szLine.w;
		}
		szMultiLine.h += szLine.h;

		TextFormat_DelEndOfLine(pFormat); //del end of line
		TextFormat_DelEndOfWord(pFormat); //del end of word
	}

	if (pSize) {
		*pSize = szMultiLine;
	}

	return r;
}

RESULT TextFormat_DoMultiLineByAlign(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag)
{
	RESULT r = GX_OK;
	ISIZE szMultiLine;
	UINT32 vAlign;
	BOOL bDraw;
	//DC* pDestDC;
	UINT32 ScreenObj;

#if (GX_TEST_NOALIGN)

	TextFormat_DoBegin(pFormat);
	//draw multi-line
	r = TextFormat_DoMultiLine(pFormat, pszSrc, 0, 0);
	TextFormat_DoEnd(pFormat);

#else

	//pDestDC = pFormat->pDestDC; //save DC
	ScreenObj = pFormat->ScreenObj; //save DC

	//pFormat->pDestDC = 0; //not draw
	pFormat->ScreenObj = 0; //not draw
	TextFormat_DoBegin(pFormat);
	//evaluate multi-line size
	r = TextFormat_DoMultiLine(pFormat, pszSrc, 0, &szMultiLine);
	_DC_INVSCALE_POINT((DC *)ScreenObj, (IPOINT *)&szMultiLine);
	_gDC_Text_size = szMultiLine;
	TextFormat_DoEnd(pFormat);

	//pFormat->pDestDC = pDestDC; //restore DC
	pFormat->ScreenObj = ScreenObj; //restore DC
	//bDraw = (pFormat->pDestDC!=0);
	bDraw = (pFormat->ScreenObj != 0);
	if (bDraw) {
		IPOINT deltaPt = {0, 0};
		TextFormat_DoBegin(pFormat);
		vAlign = (uiFlag & ALIGN_V_MASK);
		if (vAlign == ALIGN_BOTTOM) {
			deltaPt.y = (pFormat->pDestRect->h - szMultiLine.h);
		} else if (vAlign == ALIGN_MIDDLE) {
			deltaPt.y = ((pFormat->pDestRect->h - szMultiLine.h) >> 1);
		}
		TextFormat_MoveLocateY(pFormat, (INT16)(deltaPt.y));

		//draw multi-line
		r = TextFormat_DoMultiLine(pFormat, pszSrc, uiFlag, 0);
		TextFormat_DoEnd(pFormat);
	}
#endif

	return r;
}

RESULT TextFormat_DoSingleLine(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag, BOOL bDraw)
{
	RESULT r = GX_OK;
	INT16 iCount;
	TCHAR_VALUE c;

	if (!pFormat) {
		return GX_NULL_POINTER;
	}

	TextFormat_SetBeginAtHere(pFormat);
	TextFormat_DelEndOfLine(pFormat); //del end of line
	TextFormat_DelEndOfWord(pFormat); //del end of word

	iCount = 0;
	c = _String_GetChar(pszSrc, iCount);

	while (c) {
		RESULT tr;
		tr = TextFormat_AddChar(pFormat, c);
		if (tr != GX_OK) {
			r = tr;    //put to r;
		}

		iCount++;
		c = _String_GetChar(pszSrc, iCount);
	}

	if (TextFormat_GetBufCount(pFormat)) {
		TextFormat_SetEndOfLineAtHere(pFormat); //mark end of word

		TextFormat_DoLine(pFormat, 0, uiFlag, 0);

		TextFormat_DelEndOfLine(pFormat); //del end of line
		TextFormat_DelEndOfWord(pFormat); //del end of word
	}

	return r;
}

RESULT TextFormat_DoSingleLine_EvalSize
(TextFormat *pFormat, const TCHAR *pszSrc, ISIZE *pSize)
{
	RESULT r = GX_OK;
	//DC* pDestDC;
	UINT32 ScreenObj;
	ISIZE szLine;

	//pDestDC = pFormat->pDestDC; //save DC
	ScreenObj = pFormat->ScreenObj; //save DC
	//pFormat->pDestDC = 0; //not draw
	pFormat->ScreenObj = 0; //not draw
	TextFormat_DoBegin(pFormat);
	//evaluate line size
	r = TextFormat_DoSingleLine(pFormat, pszSrc, 0, 0);
	if (r == GX_OK) {
		szLine = TextFormat_GetAccSize(pFormat);
		_DC_INVSCALE_POINT((DC *)ScreenObj, (IPOINT *)&szLine);
		(*pSize) = szLine;
	}
	TextFormat_DoEnd(pFormat);

//    pFormat->pDestDC = pDestDC; //restore DC
	pFormat->ScreenObj = ScreenObj; //restore DC
	return r;
}

RESULT TextFormat_DoSingleLineByAlign
(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag)
{
	RESULT r = GX_OK;
	ISIZE szLine;
	UINT32 vAlign;
	BOOL bDraw;
	//DC* pDestDC;
	UINT32 ScreenObj;

#if (GX_TEST_NOALIGN)

	TextFormat_DoBegin(pFormat);
	//draw multi-line
	TextFormat_DoSingleLine(pFormat, pszSrc, 0, 0);
	TextFormat_DoEnd(pFormat);

#else

	//pDestDC = pFormat->pDestDC; //save DC
	ScreenObj = pFormat->ScreenObj; //save DC

	//pFormat->pDestDC = 0; //not draw
	pFormat->ScreenObj = 0; //not draw
	TextFormat_DoBegin(pFormat);
	//evaluate line size
	r = TextFormat_DoSingleLine(pFormat, pszSrc, 0, 0);
	szLine = TextFormat_GetAccSize(pFormat);
	_DC_INVSCALE_POINT((DC *)ScreenObj, (IPOINT *)&szLine);
	_gDC_Text_size = szLine;
	TextFormat_DoEnd(pFormat);

//    pFormat->pDestDC = pDestDC; //restore DC
	pFormat->ScreenObj = ScreenObj; //restore DC
//    bDraw = (pFormat->pDestDC!=0);
	bDraw = (pFormat->ScreenObj != 0);
	if (bDraw) {
		TextFormat_DoBegin(pFormat);
		vAlign = (uiFlag & ALIGN_V_MASK);
		if (vAlign == ALIGN_BOTTOM) {
			TextFormat_MoveLocateY(pFormat, (INT16)(pFormat->pDestRect->h - szLine.h));
		} else if (vAlign == ALIGN_MIDDLE) {
			TextFormat_MoveLocateY(pFormat, (INT16)((pFormat->pDestRect->h - szLine.h) >> 1));
		}
		vAlign = (uiFlag & ALIGN_H_MASK);
		if (vAlign == ALIGN_RIGHT) {
			TextFormat_MoveLocateX(pFormat, (INT16)(pFormat->pDestRect->w - szLine.w));
		} else if (vAlign == ALIGN_CENTER) {
			TextFormat_MoveLocateX(pFormat, (INT16)((pFormat->pDestRect->w - szLine.w) >> 1));
		}
		//draw line
		r = TextFormat_DoSingleLine(pFormat, pszSrc, uiFlag, 1);
		TextFormat_DoEnd(pFormat);
	}
#endif
	return r;
}

