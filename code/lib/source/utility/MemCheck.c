#include <string.h>
#include <kwrap/nvt_type.h>
#include "MemCheck.h"

#define THIS_DBGLVL         1 //0=OFF, 1=ERROR, 2=TRACE
#define __MODULE__          MemCheck
// Default debug level
#ifndef __DBGLVL__
    #define __DBGLVL__  2       // Output all message by default. __DBGLVL__ will be set to 1 via make parameter when release code.
#endif

// Default debug filter
#ifndef __DBGFLT__
    #define __DBGFLT__  "*"     // Display everything when debug level is 2
#endif
#include <kwrap/debug.h>

static UINT32 m_uiCRCTable[256];

/**
  Make CRC table

  Generate CRC table

  @param void
  @return void
*/
static void xMakeCRCTable(void)
{
    UINT32 uiValue, i, j;

    for (i=0; i<256; i++)
    {
        uiValue = i;
        for (j=0; j<8; j++)
        {
            uiValue = uiValue & 1 ? 0xEDB88320 ^ (uiValue >> 1) : (uiValue >> 1);
        }
        m_uiCRCTable[i] = uiValue;
    }
}

UINT32 MemCheck_CalcCrcCode(UINT32 uiAddr,UINT32 uiLen)
{
    UINT32 uiValue;
    UINT8 *puiTemp;

    xMakeCRCTable();

    uiValue = 0xFFFFFFFF;
    puiTemp = (UINT8 *)uiAddr;
    while (uiLen--)
    {
        uiValue = m_uiCRCTable[(uiValue ^ (*puiTemp++)) & 0xFF] ^ (uiValue >> 8);
    }

    uiValue^=0xFFFFFFFF;

    return uiValue;
}

UINT32 MemCheck_CalcCheckSum16Bit(UINT32 uiAddr,UINT32 uiLen)
{
    UINT32 i,uiSum = 0;
    UINT16 *puiValue = (UINT16 *)uiAddr;

    for (i=0; i<(uiLen >> 1); i++)
    {
        uiSum += (*(puiValue + i) + i);
    }

    uiSum &= 0x0000FFFF;

    return uiSum;
}

UINT32 MemCheck_AddPseudoStr(const MEMCHECK_PSEUDOSTR* pPseudo)
{
    UINT32 uiTempLen;
    UINT32 uiFileLen = pPseudo->uiLen;
    UINT32 uiStrLen = strlen(pPseudo->pPseudoStr);

    if(pPseudo->uiMemSize<pPseudo->uiLen+uiStrLen)
    {
        DBG_ERR("uiMemSize<uiLen+uiStrLen\r\n");
        return 0;
    }

    if (uiStrLen > 0)
    {
        uiTempLen = ALIGN_CEIL_4(uiStrLen);
        memset((void *)(pPseudo->uiAddr + uiFileLen), 0, uiTempLen);
        memcpy((void *)(pPseudo->uiAddr + uiFileLen), (const void *)pPseudo->pPseudoStr, uiStrLen);
        uiFileLen += uiStrLen;
    }

    return uiFileLen;
}