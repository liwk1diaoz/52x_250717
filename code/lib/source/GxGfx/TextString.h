#ifndef GX_TEXSTRING
#define GX_TEXSTRING

#include "GxGfx/GxGfx.h"

///////////////////////////////////////////////////////////////////////////////

_INLINE BOOL IsWString(const WCHAR *pszSrc)
{
	//wstring must be 2 bytes alligment
	if ((UINT32)pszSrc & 0x1) {
		return FALSE;
	}

	return (pszSrc[0] == FMT_WCHAR);
	//return ((((CHAR*)pszSrc)[0] == FMT_WCHAR_1ST)
	//    && (((CHAR*)pszSrc)[1] == FMT_WCHAR_2ND));
}
_INLINE BOOL AddWStringHeader(WCHAR *pszDest)
{
	pszDest[0] = FMT_WCHAR;
	//((CHAR*)pszDest)[0] = FMT_WCHAR_1ST;
	//((CHAR*)pszDest)[1] = FMT_WCHAR_2ND;
	return TRUE;
}
_INLINE WCHAR *SeekWString(WCHAR *pszSrc, INT16 iOffset)
{
	return pszSrc + iOffset;
}
_INLINE const WCHAR *SeekConstWString(const WCHAR *pszSrc, INT16 iOffset)
{
	return pszSrc + iOffset;
}

/*
BOOL _bIsWideChar = FALSE; //0:TCHAR, 1:WCHAR

__inline void _String_SetCharType(BOOL bIsWideChar)
{
    _bIsWideChar = bIsWideChar;
}
__inline BOOL _String_GetCharType(void)
{
    return _bIsWideChar;
}
*/

#define WORD_GET_LO_BYTE(v) ((char)((((unsigned short int)(v)) && 0x00FF)))
#define WORD_GET_HI_BYTE(v) ((char)((((unsigned short int)(v)) && 0xFF00)>>8))

#define MAKE_WORD_LOHI_BYTE(lo,hi)  \
	(((unsigned short int)(lo))  |   \
	 (((unsigned short int)(hi))<<8))

_INLINE TCHAR *_String_GetCharPtr(TCHAR *pszDest, INT16 n)
{
#if (GX_SUPPORT_WCHAR)
//    if(_bIsWideChar)
	return (TCHAR *)(((WCHAR *)pszDest) + n);
	//CHAR* pszDestByte = (CHAR*)(pszDest);
	//return pszDestByte+(n<<1);
#else
//    else
	return pszDest + n;
#endif
}
_INLINE const TCHAR *_String_GetConstCharPtr(const TCHAR *pszDest, INT16 n)
{
#if (GX_SUPPORT_WCHAR)
//    if(_bIsWideChar)
	return (const TCHAR *)(((const WCHAR *)pszDest) + n);
	//CHAR* pszDestByte = (CHAR*)(pszDest);
	//return pszDestByte+(n<<1);
#else
//    else
	return pszDest + n;
#endif
}
_INLINE void _String_SetChar(TCHAR *pszDest, INT16 n, TCHAR_VALUE c)
{
#if (GX_SUPPORT_WCHAR)
//    if(_bIsWideChar)
	((WCHAR *)pszDest)[n] = (WCHAR)c;
	//CHAR* pszDestByte = (CHAR*)(pszDest);
	//pszDestByte[0] = WORD_GET_LO_BYTE(c);
	//pszDestByte[1] = WORD_GET_HI_BYTE(c);
#else
//    else
	pszDest[n] = c;
#endif
}
_INLINE TCHAR_VALUE _String_GetChar(const TCHAR *pszDest, INT16 n)
{
#if (GX_SUPPORT_WCHAR)
//    if(_bIsWideChar)
	return (TCHAR_VALUE)((const WCHAR *)pszDest)[n];
	//CHAR* pszDestByte = (CHAR*)(pszDest);
	//return (TCHAR_VALUE)MAKE_WORD_LOHI_BYTE(pszDestByte[0], pszDestByte[1]);
#else
//    else
	return (TCHAR_VALUE)(pszDest[n]);
#endif
}

#define _String_Resize(str, n)  _String_SetChar(str, n, 0)


//-----------------------------------------------------------------------------
// String OP (support both TCHAR or WCHAR)
//-----------------------------------------------------------------------------

//convert output string from TCHAR to WCHAR
INT16  _TString_N_ConvertToWString(WCHAR *pszDest, const TCHAR *pszSrc, INT16 n);

//convert output string from WCHAR to TCHAR
INT16  _WString_N_ConvertToTString(TCHAR *pszDest, const WCHAR *pszSrc, INT16 n);

//-----------------------------------------------------------------------------
// String support function
//-----------------------------------------------------------------------------
//strlen
INT16 _String_GetLength(const TCHAR *pszSrc);

//strncpy
INT16 _String_N_Copy(TCHAR *pszDest, const TCHAR *pszSrc, INT16 n);

//strncmp
INT16 _String_N_Compare(const TCHAR *pszDest, const TCHAR *pszSrc, INT16 n);

//strchr
const TCHAR *_String_FindChar(const TCHAR *pszSrc, TCHAR_VALUE c);

//memset
INT16 _String_N_Fill(TCHAR *pszDest, TCHAR_VALUE c, INT16 n);

//empty
INT16 _String_Empty(TCHAR *pszDest);

//concat
INT16 _String_N_Concat(TCHAR *pszDest, const TCHAR *pszSrc, INT16 n);

//add one char to tail
INT16 _String_N_AddTail(TCHAR *pszDest, TCHAR_VALUE iChar, INT16 n);

//make reverse
INT16 _String_MakeReverse(TCHAR *pszDest);

//convert output string from TCHAR/WCHAR to TCHAR (check WCHAR by hint char)
INT16 _XString_N_ConvertToWString(WCHAR *pszDest, const void *pszSrc, INT16 n);

//convert output string from TCHAR/WCHAR to TCHAR (check WCHAR by hint char)
INT16 _XString_N_ConvertToHeaderWString(WCHAR *pszDest, const void *pszSrc, INT16 n);





#endif //GX_TEXSTRING
