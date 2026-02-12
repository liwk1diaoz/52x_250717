#ifndef GX_TEXRENDER
#define GX_TEXRENDER

typedef struct _ESCCMD {
	BOOL bDecodeImage; //or bSetCommand
	BOOL bEscIcon; //valid if bDecodeImage is TRUE and it is a ESC icon
	IVALUE id; //valid if bDecodeImage is TRUE
	BVALUE value; //valid if bDecodeImage is FALSE
} ESCCMD;

typedef struct _TextRender {
	void (*pfn_Begin)(struct _TextRender *pRender, UINT32 ScreenObj);
	void (*pfn_End)(struct _TextRender *pRender);
	UINT16(*pfn_ParseStringCmd)(const TCHAR *pszDest, INT16 i, ESCCMD *pCmd);
	RESULT(*pfn_GetCharSize)(IVALUE id, ISIZE *pCharSize, BOOL bEscIcon);
	RESULT(*pfn_DrawChar)(UINT32 ScreenObj, LVALUE x, LVALUE y, IVALUE id, BOOL bEscIcon);
	UINT32 ScreenObj;
} TextRender;


typedef struct _TextFormat {
	UINT32 nFlag;
	//DC* pDestDC;
	//const STYLE* pStyle;
	IRECT *pDestRect;
	ISIZE szAcc;
	IPOINT Pt;

	INT16 uiBegin;
	INT16 uiEnd;
	INT16 uiEndOfLine;
	INT16 uiEndOfWord;
	TCHAR *tstrBuf;
	TCHAR *outBuf;

	UINT32 ScreenObj;
	TextRender *pRender;
} TextFormat;

void TextFormat_Init(TextFormat *pFormat, TextRender *pRender, UINT32 ScreenObj, IRECT *pDestRect);
void TextFormat_Exit(TextFormat *pFormat);
RESULT TextFormat_DoSingleLine_EvalSize(TextFormat *pFormat, const TCHAR *pszSrc, ISIZE *pSize);
RESULT TextFormat_DoSingleLine(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag, BOOL bDraw);
RESULT TextFormat_DoMultiLine(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag, ISIZE *pSize);
RESULT TextFormat_DoSingleLineByAlign(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag);
RESULT TextFormat_DoMultiLineByAlign(TextFormat *pFormat, const TCHAR *pszSrc, UINT32 uiFlag);
IPOINT TextFormat_GetLastLoc(void);
ISIZE TextFormat_GetLastSize(void);

#endif //GX_TEXRENDER


