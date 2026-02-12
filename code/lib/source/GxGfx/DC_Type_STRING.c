
#include "GxGfx/GxGfx.h"
#include "GxGfx_int.h"
#include "Chunk.h"
#include "TextString.h"
#include "DC.h"

INT16 STRING_GetLength(const TCHAR *pszSrc)
{
	BOOL bWideStr;
	INT16 iLen = 0;
	if (!pszSrc) {
		return iLen;
	}
	bWideStr = IsWString((const WCHAR *)pszSrc);
	if (bWideStr) {
		INT16 iCount = 0;
		while (_String_GetChar(pszSrc, iCount)) {
			iCount++;
		}
		iLen = iCount - 1;  //remove BOM mark
	} else {
		INT16 iCount = 0;
		while (pszSrc[iCount]) {
			iCount++;
		}
		iLen = iCount;
	}
	return iLen;
}

//condition : pTable != null
//condition : id is in the range of table
RESULT _STR_GetLen(const STRING_TABLE *_pTable, IVALUE id, INT16 *pStringLen)
{
	const GX_STRINGTABLE *pTable = (const GX_STRINGTABLE *)_pTable;
	const GX_TABLE *pChunkTable;
	const CHUNK *pChunk;
	IVALUE sid;
	INT16 iLen;

	if (!pTable) {
		DBG_ERR("_STR_GetLen - ERROR! String table is null.\r\n");
		return GX_NO_STRINGTABLE;
	}
	if (!pStringLen) {
		DBG_ERR("_STR_GetLen - ERROR! Output Len pointer is null.\r\n");
		return GX_NULL_POINTER;
	}

	pChunkTable = (const GX_TABLE *)(pTable + 1);
	if ((id < pChunkTable->start) || (id >= pChunkTable->start + pChunkTable->count)) {
		DBG_ERR("_STR_GetLen - ERROR! ID is out of range.\r\n");
		return GX_OUTOF_STRID;
	}

	sid = id - pChunkTable->start;
	pChunk = ((const CHUNK *)(pChunkTable + 1)) + sid;
	if ((pChunk->offset == 0xFFFFFFFF) || (pChunk->size == 0)) {
		DBG_ERR("_STR_GetLen - ERROR! ID is invalid.\r\n");
		return GX_INVALID_STRID;
	}
	iLen = pChunk->size;
	if (pTable->bpc > 0) {
		iLen >>= 1;
	}
	*pStringLen = iLen;
	return GX_OK;
}

//condition : pTable != null
//condition : id is in the range of table
RESULT _STR_MakeStr(const STRING_TABLE *_pTable, IVALUE id, const TCHAR **ppStr)
{
	const GX_STRINGTABLE *pTable = (const GX_STRINGTABLE *)_pTable;
	const GX_TABLE *pChunkTable;
	const CHUNK *pChunk;
	IVALUE sid;

	if (!pTable) {
		DBG_ERR("_STR_MakeStr - ERROR! String table is null.\r\n");
		return GX_NO_STRINGTABLE;
	}
	if (!ppStr) {
		DBG_ERR("_STR_MakeStr - ERROR! Output Str pointer is null.\r\n");
		return GX_NULL_POINTER;
	}

	pChunkTable = (const GX_TABLE *)(pTable + 1);
	if ((id < pChunkTable->start) || (id >= pChunkTable->start + pChunkTable->count)) {
		DBG_ERR("_STR_MakeStr - ERROR! ID 0x%X is of of range.\r\n", id);
		return GX_OUTOF_STRID;
	}

	sid = id - pChunkTable->start;
	pChunk = ((const CHUNK *)(pChunkTable + 1)) + sid;
	if ((pChunk->offset == 0xFFFFFFFF) || (pChunk->size == 0)) {
		DBG_ERR("_STR_MakeStr - ERROR! ID is not valid.\r\n");
		return GX_INVALID_STRID;
	}
	*ppStr = (const TCHAR *)(((const UINT8 *)pChunkTable) + (pChunk->offset));
	return GX_OK;
}


