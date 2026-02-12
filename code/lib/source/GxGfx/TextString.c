#include "GxGfx/GxGfx.h"
#include "TextString.h"

//-----------------------------------------------------------------------------
// String OP (support both TCHAR or WCHAR)
//-----------------------------------------------------------------------------

//#NT#2016/10/20#KCHong -begin
//#NT#For unicode display
static INT8 enc_get_utf8_size(const UINT8 c);
static INT8 enc_utf8_to_unicode_one(UINT8* pInput, UINT32 *Unic);

static INT8 enc_get_utf8_size(const UINT8 c)
{
    // 0xxxxxxx return 0
    // 10xxxxxx return -1 (invalid)
    // 110xxxxx return 2
    // 1110xxxx return 3
    // 11110xxx return 4
    // 111110xx return 5
    // 1111110x return 6
    if (c < 0x80) return 0;
    if (c >= 0x80 && c<0xC0) return -1;
    if (c >= 0xC0 && c<0xE0) return 2;
    if (c >= 0xE0 && c<0xF0) return 3;
    if (c >= 0xF0 && c<0xF8) return 4;
    if (c >= 0xF8 && c<0xFC) return 5;
    else return 6;  //(c >= 0xFC)

}

static INT8 enc_utf8_to_unicode_one(UINT8* pInput, UINT32 *Unic)
{
    UINT8 b1, b2, b3, b4, b5, b6;

    *Unic = 0x0;
    INT8 utfbytes = enc_get_utf8_size(*pInput);
    UINT8 *pOutput = (UINT8 *)Unic;

    switch (utfbytes)
    {
    case 0:
        *pOutput = *pInput;
        utfbytes += 1;
        break;
    case 2:
        b1 = *pInput;
        b2 = *(pInput + 1);
        if ((b2 & 0xE0) != 0x80)
            return 0;
        *pOutput = (b1 << 6) + (b2 & 0x3F);
        *(pOutput + 1) = (b1 >> 2) & 0x07;
        break;
    case 3:
        b1 = *pInput;
        b2 = *(pInput + 1);
        b3 = *(pInput + 2);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
            return 0;
        *pOutput = (b2 << 6) + (b3 & 0x3F);
        *(pOutput + 1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
        break;
    case 4:
        b1 = *pInput;
        b2 = *(pInput + 1);
        b3 = *(pInput + 2);
        b4 = *(pInput + 3);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
            || ((b4 & 0xC0) != 0x80))
            return 0;
        *pOutput = (b3 << 6) + (b4 & 0x3F);
        *(pOutput + 1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
        *(pOutput + 2) = ((b1 << 2) & 0x1C) + ((b2 >> 4) & 0x03);
        break;
    case 5:
        b1 = *pInput;
        b2 = *(pInput + 1);
        b3 = *(pInput + 2);
        b4 = *(pInput + 3);
        b5 = *(pInput + 4);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
            || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80))
            return 0;
        *pOutput = (b4 << 6) + (b5 & 0x3F);
        *(pOutput + 1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
        *(pOutput + 2) = (b2 << 2) + ((b3 >> 4) & 0x03);
        *(pOutput + 3) = (b1 << 6);
        break;
    case 6:
        b1 = *pInput;
        b2 = *(pInput + 1);
        b3 = *(pInput + 2);
        b4 = *(pInput + 3);
        b5 = *(pInput + 4);
        b6 = *(pInput + 5);
        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
            || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)
            || ((b6 & 0xC0) != 0x80))
            return 0;
        *pOutput = (b5 << 6) + (b6 & 0x3F);
        *(pOutput + 1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
        *(pOutput + 2) = (b3 << 2) + ((b4 >> 4) & 0x03);
        *(pOutput + 3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
        break;
    default:
        return 0;
        break;
    }
    return utfbytes;
}
//#NT#2016/10/20#KCHong -end

//convert output string from TCHAR to WCHAR
//ex: UTF8 "Novatek\u884c\u8f66\u8bb0\u5f55\u4eea"
INT16  _TString_N_ConvertToWString(WCHAR* pszDest, const TCHAR* pszSrc, INT16 n)
{
//#NT#2016/10/20#KCHong -begin
//#NT#For unicode display
#if 0
    INT16 iCount = 0;
    if(!pszDest) return -1;
    if(!pszSrc) return -1;

    while (pszSrc[iCount] && (iCount<n))
    {
        pszDest[iCount] = pszSrc[iCount];
        iCount++;
    }
    pszDest[iCount] = 0;

    return iCount;
#else
    INT16 src_cnt = 0, dest_cnt = 0;
    INT8 char_size = 0;
    UINT32 char_ucode = 0;
    UINT8 *src_ptr = NULL;

    if(!pszDest) return -1;
    if(!pszSrc) return -1;

    while (pszSrc[src_cnt] && (src_cnt < n)) {
        src_ptr = (UINT8*)&(pszSrc[src_cnt]);
        char_size = enc_utf8_to_unicode_one((UINT8*)src_ptr, &char_ucode);
        if (char_size > 0) {
            pszDest[dest_cnt] = (WCHAR)char_ucode;
            src_cnt += char_size;
            dest_cnt++;
        } else {
            break;
        }
    }
    pszDest[dest_cnt] = 0;
    return dest_cnt;
#endif
//#NT#2016/10/20#KCHong -end
}

//convert output string from WCHAR to TCHAR
INT16  _WString_N_ConvertToTString(TCHAR *pszDest, const WCHAR *pszSrc, INT16 n)
{
	INT16 iCount = 0;
	if (!pszDest) {
		return -1;
	}
	if (!pszSrc) {
		return -1;
	}

	while (pszSrc[iCount] && (iCount < n)) {
		pszDest[iCount] = (TCHAR)(pszSrc[iCount]);
		iCount++;
	}
	pszDest[iCount] = 0;

	return iCount;
}

//-----------------------------------------------------------------------------
// String support function
//-----------------------------------------------------------------------------
//strlen
INT16 _String_GetLength(const TCHAR *pszSrc)
{
	INT16 iCount = 0;
	if (!pszSrc) {
		return -1;
	}

//    while (pszSrc[iCount])
	while (_String_GetChar(pszSrc, iCount)) {
		iCount++;
	}

	return iCount;
}

//strncpy
INT16 _String_N_Copy(TCHAR *pszDest, const TCHAR *pszSrc, INT16 n)
{
	INT16 iCount = 0;
	if (!pszDest) {
		return -1;
	}
	if (!pszSrc) {
		return -1;
	}

//    while (pszSrc[iCount] && (iCount<n))
	while (_String_GetChar(pszSrc, iCount) && (iCount < n)) {
//        pszDest[iCount] = pszSrc[iCount];
		//coverity[unchecked_value]
		//coverity[check_return]
		_String_SetChar(pszDest, iCount, _String_GetChar(pszSrc, iCount));
		iCount++;
	}
//    pszDest[iCount] = 0;
	_String_Resize(pszDest, iCount);

	return iCount;
}

//strncmp
INT16 _String_N_Compare(const TCHAR *pszDest, const TCHAR *pszSrc, INT16 n)
{
	INT16 iDiff = 0;
	INT16 iCount = 0;
	BOOL bEndDest = 0;
	BOOL bEndSrc = 0;
	if (!pszDest) {
		return -1;
	}
	if (!pszSrc) {
		return -1;
	}

//    while (pszDest[iCount] && pszSrc[iCount] && (iCount<n))
//    while (_String_GetChar(pszDest, iCount) && _String_GetChar(pszSrc, iCount) && (iCount<n))
	while ((iCount < n) && (!bEndDest || !bEndSrc)) {
		INT16 srcChar = 0, destChar = 0;
		/*        if(pszDest[iCount] > pszSrc[iCount])
		            iDiff += (pszDest[iCount] - pszSrc[iCount]);
		        else
		            iDiff += (pszSrc[iCount] - pszDest[iCount]);*/
		if (!bEndDest) {
			destChar = (INT16)_String_GetChar(pszDest, iCount);
			if (destChar == 0) {
				bEndDest = 1;
			}
		}
		if (!bEndSrc) {
			srcChar = (INT16)_String_GetChar(pszSrc, iCount);
			if (srcChar == 0) {
				bEndSrc = 1;
			}
		}

		if (destChar > srcChar) {
			iDiff += (destChar - srcChar);
		} else {
			iDiff += (srcChar - destChar);
		}
		iCount++;
	}

	return iDiff;
}

//strchr
const TCHAR *_String_FindChar(const TCHAR *pszSrc, TCHAR_VALUE c)
{
	INT16 iCount = 0;
	const TCHAR *pFind = 0;
	if (!pszSrc) {
		return 0;
	}

//    while (pszSrc[0] && !pFind)
	while (_String_GetChar(pszSrc, iCount) && !pFind) {
//        if(c == pszSrc[0])
		if (c == _String_GetChar(pszSrc, iCount)) {
			pFind = _String_GetConstCharPtr(pszSrc, iCount);
		}
		iCount++;
	}

	return pFind;
}

//memset
INT16 _String_N_Fill(TCHAR *pszDest, TCHAR_VALUE c, INT16 n)
{
	INT16 iCount = 0;
	if (!pszDest) {
		return -1;
	}

//    while (pszDest[iCount] && (iCount<n))
	while (_String_GetChar(pszDest, iCount) && (iCount < n)) {
//        pszDest[iCount] = c;
		_String_SetChar(pszDest, iCount, c);
		iCount++;
	}

	return iCount;
}

INT16 _String_Empty(TCHAR *pszDest)
{
	if (!pszDest) {
		return -1;
	}

	_String_Resize(pszDest, 0);

	return 0;
}

INT16 _String_N_Concat(TCHAR *pszDest, const TCHAR *pszSrc, INT16 n)
{
	INT16 iCount = 0;
	INT16 len;
	if (!pszDest) {
		return -1;
	}
	if (!pszSrc) {
		return -1;
	}

	len = _String_GetLength(pszDest);
	iCount = _String_N_Copy(_String_GetCharPtr(pszDest, len), pszSrc, n - len);
	return iCount + len;
}

INT16 _String_N_AddTail(TCHAR *pszDest, TCHAR_VALUE iChar, INT16 n)
{
	INT16 len;
	if (!pszDest) {
		return -1;
	}

	len = _String_GetLength(pszDest);
	if (len == n) {
		return len;
	}
	_String_SetChar(pszDest, len, iChar);
	_String_Resize(pszDest, len + 1);

	return len + 1;
}

INT16 _String_MakeReverse(TCHAR *pszDest)
{
	INT16 iCountHead = 0;
	INT16 iCountTail = 0;
	INT16 len;
	if (!pszDest) {
		return -1;
	}

	len = _String_GetLength(pszDest);
	if (len <= 1) {
		return len;
	}
	iCountHead = 0;
	iCountTail = len - 1; //tail char

	while (iCountHead < iCountTail) { //head < tail
		INT16 headChar = 0, tailChar = 0, tmpChar = 0;
		//read
		//coverity[check_return]
		headChar = (INT16)_String_GetChar(pszDest, iCountHead);
		tailChar = (INT16)_String_GetChar(pszDest, iCountTail);
		//swap char
		tmpChar = headChar;
		headChar = tailChar;
		tailChar = tmpChar;
		//write
		//coverity[unchecked_value]
		//coverity[check_return]
		_String_SetChar(pszDest, iCountHead, headChar);
		_String_SetChar(pszDest, iCountTail, tailChar);
		iCountHead++;
		iCountTail--;
	}

	return len;
}



//convert output string from TCHAR/WCHAR to TCHAR (check WCHAR by hint char)
INT16 _XString_N_ConvertToWString(WCHAR *pszDest, const void *pszSrc, INT16 n)

{
	BOOL bWideStr = IsWString((const WCHAR *)pszSrc);

	if (bWideStr) {
		INT16 iLen2 = n - FMT_WCHAR_SIZE;
		pszSrc = SeekConstWString((const WCHAR *)pszSrc, FMT_WCHAR_SIZE);
		return _String_N_Copy((TCHAR *)pszDest, (const TCHAR *)pszSrc, iLen2);
	} else {
		return _TString_N_ConvertToWString(pszDest, (const TCHAR *)pszSrc, n);
	}

}

//convert output string from TCHAR/WCHAR to TCHAR (check WCHAR by hint char)
INT16 _XString_N_ConvertToHeaderWString(WCHAR *pszDest, const void *pszSrc, INT16 n)

{
	BOOL bWideStr = IsWString((const WCHAR *)pszSrc);

	if (bWideStr) {
		return _String_N_Copy((TCHAR *)pszDest, (TCHAR *)pszSrc, n);
	} else {
		//add WString header
		INT16 iLen2 = n - FMT_WCHAR_SIZE;
		AddWStringHeader(pszDest);
		pszDest = SeekWString(pszDest, FMT_WCHAR_SIZE);
		return _TString_N_ConvertToWString(pszDest, (const TCHAR *)pszSrc, iLen2);
	}
}

