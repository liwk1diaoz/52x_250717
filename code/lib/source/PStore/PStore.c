/*
    @file       PStore.c
    @ingroup    mISYSPStore

    @brief      PStore function interface

    @note

    @version    V1.00.000
    @author     CL Wang
    @date       2006/10/25

    Copyright   Novatek Microelectronics Corp. 2006.  All rights reserved.

*/

#include <string.h>
#include <stdio.h>
//#include "Type.h"
#include "PStore.h"
//#include "FileSysTsk.h"
#include "ps_int.h"
//#include "DxStorage.h"
//#include "DxApi.h"
#include "ps_op.h"
#include "ps_fs.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PS
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

typedef ER     PS_OP_OPEN (STORAGE_OBJ *pStrgObj,PSTORE_INIT_PARAM* pInitParam);
typedef ER     PS_OP_CLOSE(void);
typedef PSTORE_SECTION_HANDLE* PS_OP_OpenSection(char *pSecName, UINT32 RWOperation);
typedef ER     PS_OP_CloseSection(PSTORE_SECTION_HANDLE* pSectionHandle);
typedef ER     PS_OP_DeleteSection(char *pSecName);
typedef ER     PS_OP_ReadSection( UINT8* pcBuffer,UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle);
typedef ER     PS_OP_WriteSection(UINT8* pcBuffer, UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle);
typedef ER     PS_OP_Format(PSFMT *pFmtStruct);
typedef UINT32 PS_OP_GetInfo(PS_INFO_ID infoId);
typedef BOOL   PS_OP_SearchSectionOpen(PSTORE_SEARCH_HANDLE *pFindData);
typedef BOOL   PS_OP_SearchSection(PSTORE_SEARCH_HANDLE *pFindData);
typedef BOOL   PS_OP_SearchSectionRewind(void);
typedef ER     PS_OP_SearchSectionClose(void);
typedef void   PS_OP_DumpPStoreInfo(void);
typedef void   PS_OP_Dump(UINT8 *pBuf,UINT32 uiBufSize);
typedef ER     PS_OP_CheckSection(char *pSecName,UINT32* ErrorSection);
typedef ER     PS_OP_CheckTotalSection(UINT32* ErrorSection);
typedef ER     PS_OP_EnableUpdateHeader(PS_HEAD_UPDATE Header);
typedef ER     PS_OP_RestoreHeader(UINT8 *pBuf , UINT32 RestoreType);
typedef ER     PS_OP_ReadOnly(char *pSecName, BOOL Enable);
typedef ER     PS_OP_ReadOnlyAll(BOOL Enable);
typedef void   PS_OP_EnableCheckData(BOOL Enable);
typedef void   PS_OP_SetPrjVersion(UINT32 Version);


typedef struct _PSTORE_OPS
{
    PS_OP_OPEN                 *Open;
    PS_OP_CLOSE                *Close;
    PS_OP_OpenSection          *OpenSection;
    PS_OP_CloseSection         *CloseSection;
    PS_OP_DeleteSection        *DeleteSection;
    PS_OP_ReadSection          *ReadSection;
    PS_OP_WriteSection         *WriteSection;
    PS_OP_Format               *Format;
    PS_OP_GetInfo              *GetInfo;
    PS_OP_SearchSectionOpen    *SearchSectionOpen;
    PS_OP_SearchSection        *SearchSection;
    PS_OP_SearchSectionRewind  *SearchSectionRewind;
    PS_OP_SearchSectionClose   *SearchSectionClose;
    PS_OP_DumpPStoreInfo       *DumpPStoreInfo;
    PS_OP_Dump                 *Dump;
    PS_OP_CheckSection         *CheckSection;
    PS_OP_CheckTotalSection    *CheckTotalSection;
    PS_OP_EnableUpdateHeader   *EnableUpdateHeader;
    PS_OP_RestoreHeader        *RestoreHeader;
    PS_OP_ReadOnly             *ReadOnly;
    PS_OP_ReadOnlyAll          *ReadOnlyAll;
    PS_OP_EnableCheckData      *EnableCheckData;
    PS_OP_SetPrjVersion        *SetPrjVersion;
}PSTORE_OPS;

static PSTORE_OPS *g_pCurPS;

static PSTORE_OPS g_PSFilesys =
{
    ps_fs_Open,
    ps_fs_Close,
    ps_fs_OpenSection,
    ps_fs_CloseSection,
    ps_fs_DeleteSection,
    ps_fs_ReadSection,
    ps_fs_WriteSection,
    ps_fs_Format,
    ps_fs_GetInfo,
    NULL,
    NULL,
    NULL,
    NULL,
    ps_fs_DumpPStoreInfo,
    NULL,
    ps_fs_CheckSection,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static UINT32  PS_DRV_VERSION = 1;

ER PStore_Init(PS_TYPE PsType,CHAR Drive)
{
    g_pCurPS = &g_PSFilesys;

    return E_PS_OK;
}


/**
    @addtogroup mISYSPStore
@{
*/

/**
    Enable/Disable check data after write section.

    If enable,write section would read data to temp buffer and compare with write data evey time.

    @note:The performance of write section would be affected.

    @param[in]    Enable enable/disable function.

    @return void
*/
void PStore_EnableCheckData(BOOL Enable)
{
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return;
    }
#if 0
    if(g_pCurPS->EnableCheckData)
        g_pCurPS->EnableCheckData(Enable);
    else
        DBG_WRN("op not sup\r\n");
#else
        DBG_WRN("op not sup\r\n");

#endif

}

/**
    Calculate PStore buffer size

    The fomula as below,
    1.one block for read/write not block alignment access
    2.keep tmp 2 section block table + 1 cTempHeadBuf  = 3 PS_SECTION_HEADER_SIZE
    3.all section descption
    4.buffer check byte:4

    @param[in] pFmtStruct       Format structure for caculate block table size.
    @param[in] blkSize          Storage block size.

    @return    Buffer size.
*/
UINT32 PStore_CalBuffer(PSFMT *pFmtStruct,UINT32 blkSize)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }
#if 0
    if(g_pCurPS->CalBuffer)
        Result = g_pCurPS->CalBuffer(pFmtStruct,blkSize);
    else
        DBG_WRN("op not sup\r\n");
#else
        DBG_WRN("op not sup\r\n");

#endif

    return Result;

}

ER PStore_Open(STORAGE_OBJ *pStrgObj,PSTORE_INIT_PARAM* pInitParam)
{
    ER Result =E_PS_OPFAIL;

    //PST_InstallCmd();

    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not set,use PS_TYPE_EMBEDED as default\r\n");
        g_pCurPS = &g_PSFilesys;
    }

    if(g_pCurPS->SetPrjVersion)
        g_pCurPS->SetPrjVersion(PS_DRV_VERSION);

    if(g_pCurPS->Open)
        Result = g_pCurPS->Open(pStrgObj,pInitParam);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}


/**
    Close PStore.

    This API will check whether there is an opened handle ,so close the section and release handle

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_CHKER if check sum error
*/

ER PStore_Close(void)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }
    if(g_pCurPS->Close)
        Result = g_pCurPS->Close();
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Open a PStore section.

    The API opens the section that is specified by section name.

    @note : A valid section name consist of four characters or less, but can not be NULL or 0xFFFFFFFF.

    @param[in]      pSecName           Section name.
    @param[in]      RWOperation        Read / write operation.
                                                            PS_RDONLY Read only
                                                            PS_WRONLY Write only
                                                            PS_RDWD   Read and write
                                                            PS_CREATE Create a new section when the section not existence.
                                                            PS_UPDATE update exist section data,not change section size and header table

    @return      Section Handle      The pointer of section handle.
                                     - @b E_PS_SECHDLER for open section fail.
                                     (ex:Section not found,operation error,check name error,section full,check sum error.)
                                     If section opened,This API would wait until the section closed.

*/
PSTORE_SECTION_HANDLE* PStore_OpenSection(char *pSecName, UINT32 RWOperation)
{
    PSTORE_SECTION_HANDLE *pSectionHandle = E_PS_SECHDLER;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return pSectionHandle;
    }
    if(g_pCurPS->OpenSection)
        pSectionHandle = g_pCurPS->OpenSection(pSecName,RWOperation);
    else
        DBG_WRN("op not sup\r\n");

    return pSectionHandle;

}

/**
    Close a PStore section.

    The API closes opened handle

    @param[in]      pSectionHandle  The Pointer to section handle.

    @return
        - @b E_PS_OK if success
        - @b E_PS_PAR if wrong parameter
        - @b E_PS_CHKER if check sum error
        - @b E_PS_SYS if write block error
*/
ER PStore_CloseSection(PSTORE_SECTION_HANDLE* pSectionHandle)
{
    ER Result =E_PS_OPFAIL;

    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }
    if(g_pCurPS->CloseSection)
        Result = g_pCurPS->CloseSection(pSectionHandle);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Delete a PStore section.

    Delete a PStore section.

    @param[in]      pSecName           Section name.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if delete fail
        - @b E_PS_SECNOTCLOSE if section opened,please close section first.
        - @b E_PS_SECNOTFOUND if assigned section name not found
        - @b E_PS_PAR if wrong parameter
*/
ER PStore_DeleteSection(char *pSecName)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }
    if(g_pCurPS->DeleteSection)
        Result = g_pCurPS->DeleteSection(pSecName);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Read data from storage of PStore area.

    Read data from storage of PStore area to buffer.

    @param[out] pcBuffer        The read-in data.
    @param[in]  ulStartAddress  Data to read from.
    @param[in]  ulDataLength    How many bytes to read.
    @param[in]  pSectionHandle  The pointer of section handle

    @return
        - @b E_PS_OK if success
        - @b E_PS_PSNOTOPEN if PStore has not opened yet.
        - @b E_PS_PAR if wrong parameter
        - @b E_PS_SECNOTFOUND if section not found.
        - @b E_PS_SECNOTOPEN if the section has not opened yet.
        - @b E_PS_WRONLY if write only
        - @b E_PS_SYS if system error
*/

ER PStore_ReadSection(UINT8 *pcBuffer,UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }
    if(g_pCurPS->ReadSection)
        Result = g_pCurPS->ReadSection(pcBuffer,ulStartAddress,ulDataLength,pSectionHandle);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Write data to storage of PStore area.

    Write data to storage of PStore area.

    @param[in] pcBuffer        The pointer to data to be written.
    @param[in] ulStartAddress  Start position of data.
    @param[in] ulDataLength    How many bytes to write.
    @param[in] pSectionHandle  The pointer of section handle.

    @return
        - @b E_PS_OK if success
        - @b E_PS_PSNOTOPEN if PStore has not opened yet.
        - @b E_PS_PAR if wrong parameter
        - @b E_PS_SECNOTFOUND if section not found.
        - @b E_PS_SECNOTOPEN if the section has not opened yet.
        - @b E_PS_RDONLY if read only
        - @b E_PS_SYS if system error
*/

ER PStore_WriteSection(UINT8 *pcBuffer, UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->WriteSection)
        Result = g_pCurPS->WriteSection(pcBuffer,ulStartAddress,ulDataLength,pSectionHandle);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Dump PSTORE structure and exist PSTORE_SECTION_DESC and PSTORE_SECTION_HANDLE
*/
void PStore_DumpPStoreInfo(void)
{

    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return;
    }

    if(g_pCurPS->DumpPStoreInfo)
        g_pCurPS->DumpPStoreInfo();
    else
        DBG_WRN("op not sup\r\n");

}
/**
    Dump PStore data to file system.

    The dump data including header,block table and section data.

    @param[in] pcBuffer    Working buffer for dump.
    @param[in] uiBufSize   Working buffer size.

    @return void.

*/
void PStore_Dump(UINT8 *pBuf,UINT32 uiBufSize)
{
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return ;
    }

    if(g_pCurPS->Dump)
        g_pCurPS->Dump(pBuf,uiBufSize);
    else
        DBG_WRN("op not sup\r\n");

}


/**
    Format PStore.

    Erase all data in PStore area,then fill PStore header parameter to PStore area.

    @param[in] pFmtStruct    Format structure.Assign block number per section and total block.

    @return
        - @b E_PS_OK if success.
        - @b E_PS_SYS if failure.
        - @b E_PS_PAR if parameter error.
        - @b E_PS_SIZE if buffer size error.


*/
ER PStore_Format(PSFMT *pFmtStruct)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->Format)
        Result = g_pCurPS->Format(pFmtStruct);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}
/**
    Get PStore information.

    Get PStore information.

    @param[in] info_id    To specify which information ID.

    @return               Information data.
*/
UINT32 PStore_GetInfo(PS_INFO_ID info_id)
{
    UINT32 Result =0;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->GetInfo)
        Result = g_pCurPS->GetInfo(info_id);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

BOOL PStore_SearchSectionOpen(PSTORE_SEARCH_HANDLE *pFindData)
{
    BOOL bFound = FALSE;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return FALSE;
    }

    if(g_pCurPS->SearchSectionOpen)
        bFound = g_pCurPS->SearchSectionOpen(pFindData);
    else
        DBG_WRN("op not sup\r\n");

    return bFound;

}

BOOL PStore_SearchSection(PSTORE_SEARCH_HANDLE *pFindData)
{
    BOOL bFound = FALSE;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return FALSE;
    }

    if(g_pCurPS->SearchSection)
        bFound = g_pCurPS->SearchSection(pFindData);
    else
        DBG_WRN("op not sup\r\n");

    return bFound;

}

BOOL PStore_SearchSectionRewind(void)
{
    BOOL Result = FALSE;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return FALSE;
    }

    if(g_pCurPS->SearchSectionRewind)
        Result = g_pCurPS->SearchSectionRewind();
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

ER PStore_SearchSectionClose(void)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->SearchSectionClose)
        Result = g_pCurPS->SearchSectionClose();
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}


/**
    Check assigned section block table.

    Check PStore block table first block number is empty (0xFF) or invalid value

    @param[in] pSecName           Secton name.
    @param[out] ErrorSection       Return Section Error.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_HEADFAIL if check header error
*/

ER PStore_CheckSection(char *pSecName,UINT32 *ErrorSection)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->CheckSection)
        Result = g_pCurPS->CheckSection(pSecName,ErrorSection);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Check total section block table.

    check exist PStore block table first block number is empty (0xFF) or invalid value

    @param[out] ErrorSection        Return Section header error.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_HEADFAIL if check header error
*/

ER PStore_CheckTotalSection(UINT32* ErrorSection)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->CheckTotalSection)
        Result = g_pCurPS->CheckTotalSection(ErrorSection);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Enable update header.

    User can decide update which header
    this API ,PStore_CheckTotalSection and PStore_RestoreHeader can use together.
    ex:
    PStore_EnableUpdateHeader(PS_UPDATE_ONEHEAD); //only update header 1
    PStore_CheckTotalSection(&ErrorSection);
    if(ErrorSection==PS_HEAD1)
        PStore_RestoreHeader((UINT8 *)pBuf,PS_HEAD1);

    @param[in] Header       Update Header type(refer to PS_HEAD_UPDATE).

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
*/
ER PStore_EnableUpdateHeader(PS_HEAD_UPDATE Header)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->EnableUpdateHeader)
        Result = g_pCurPS->EnableUpdateHeader(Header);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}


/**
    Restore PStore header.

    Restore from good header to bad header.

    @param[in] pBuf         Working buffer.
    @param[out] ReStoreType  ReStore header1 or header2

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_PAR if pamameter error
*/
ER PStore_RestoreHeader(UINT8 * pBuf ,UINT32 RestoreType)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->RestoreHeader)
        Result = g_pCurPS->RestoreHeader(pBuf,RestoreType);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Set exist section read only

    Set exist section read only.

    @note
    The API would update PStore header,and write to stoarge.
    The attribute would exist after the reboot.

    @param[in] Enable        TRUE or FALSE

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
*/
ER PStore_ReadOnlyAll(BOOL Enable)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->ReadOnlyAll)
        Result = g_pCurPS->ReadOnlyAll(Enable);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}


/**
    Set single section read only.

    Set single section read only.

    @note
    The API would update PStore header,and write to stoarge.
    The attribute would exist after the reboot.

    @param[in] pSecName         Section name.
    @param[in] Enable            TRUE or FALSE.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_PAR if pamameter error
*/
ER PStore_ReadOnly(char *pSecName,BOOL Enable)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN("PStore_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->ReadOnly)
        Result = g_pCurPS->ReadOnly(pSecName,Enable);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

/**
    Check storage version to determine if format is needed.

    If Version is different from the value stored in storage PStore_Open() will fail and project should then call PStore_Format().

    @param[in]    Version project's specific version.

    @return void
*/
void   PStore_SetPrjVersion(UINT32 Version)
{
    PS_DRV_VERSION = Version;

    if(g_pCurPS && g_pCurPS->SetPrjVersion)
        g_pCurPS->SetPrjVersion(Version);

    return;
}

/**
@}   //addtogroup
*/

ER PStore_SetDevNode(char *pDevNode)
{
    return ps_fs_SetDevNode(pDevNode);
}