/**
    PStore driver export variables and function prototypes.

    PStore is novatek original simple file system.
    For access permanent storage in embeded system.

    @file       ps_op.h
    @ingroup    mIPStore

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/


#ifndef _PS_OP_H
#define _PS_OP_H

#include <kwrap/type.h>
#include <strg_def.h>


/**
    Calculate PStore buffer size

    The fomula as below, \n
    1.one block for read/write not block alignment access \n
    2.keep tmp 2 section block table + 1 cTempHeadBuf  = 3 PS_SECTION_HEADER_SIZE \n
    3.all section descption \n
    4.buffer check byte:4 \n

    @param[in] pFmtStruct       Format structure for caculate block table size.
    @param[in] blkSize          Storage block size.

    @return    Buffer size.
*/
extern UINT32  ps_op_CalBuffer(PSFMT *pFmtStruct,UINT32 blkSize);

/**
    Open PStore.

    This API would initialize storage driver,working buffer and global parmater.
    Need to open PStore before access PStore.

	@param[in] pStrgObj    The pointer of storage object.

    @param[in] pInitParam  The pointer of working buffer address and size.

    @return
        - @b E_PS_OK if success
        - @b E_PS_PAR if the input parameters are wrong
        - @b E_PS_SYS if some system settings did not be set, such as PStore_InstallID
        - @b E_PS_CHKER if checksum error
        - @b E_PS_EMPTY if no PStore header.All haeder data is 0xFF
        - @b E_PS_VERFAIL if library or driver version is different from verion in storage
        - @b E_PS_DRVFAIL if init storge driver fail
        - @b E_PS_GETRIGHTFAIL if get access right fail.(last time use  ps_op_GetAccessRight but not release)
        - @b E_PS_HEADFAIL if found PStore header error
        - @b E_PS_SEC_NUM if section number out of range (PS_MIN_SEC_NUM ~ PS_MAX_SEC_NUM)
        - @b E_PS_PARTITION if NAND partition error
*/
extern ER      ps_op_Open(STORAGE_OBJ *pStrgObj,PSTORE_INIT_PARAM* pInitParam);

/**
    Close PStore.

    This API will check whether there is an opened handle ,so close the section and release handle

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_CHKER if check sum error
*/
extern ER      ps_op_Close(void);

/**
    Open a PStore section.

    The API opens the section that is specified by section name.

    @note
    A valid section name consist of four characters or less, but can not be NULL or 0xFFFFFFFF.

    @param[in]      pSecName           Section name.
    @param[in]      RWOperation         Read / write operation.
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
extern PSTORE_SECTION_HANDLE*  ps_op_OpenSection(char *pSecName, UINT32 RWOperation);

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
extern ER      ps_op_CloseSection(PSTORE_SECTION_HANDLE* pSectionHandle);

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
extern ER      ps_op_DeleteSection(char *pSecName);

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


    Example:
    @code
    {
        if ((pSection =  ps_op_OpenSection(PS_SEC_NAME, PS_RDWR)) != E_PS_SECHDLER)
        {
             ps_op_ReadSection((UINT8 *)pBuf, 0, secSize, pSection);
             ps_op_CloseSection(pSection);
        }
    }
    @endcode
*/
extern ER      ps_op_ReadSection( UINT8* pcBuffer,UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle);

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

    Example:
    @code
    {
        if ((pSection =  ps_op_OpenSection(PS_SEC_NAME, PS_RDWR | PS_CREATE)) != E_PS_SECHDLER)
        {
             ps_op_WriteSection((UINT8 *)pBuf, 0, secSize, pSection);
             ps_op_CloseSection(pSection);
        }
    }
    @endcode
*/
extern ER      ps_op_WriteSection(UINT8* pcBuffer, UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle);

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
extern ER      ps_op_Format(PSFMT *pFmtStruct);

/**
    Get PStore information.

    Get PStore information.

    @param[in] infoId    To specify which information ID.

    @return               Information data.
*/
extern UINT32  ps_op_GetInfo(PS_INFO_ID infoId);

/**
    Open for search PStore section.

    Open for search PStore section.

    @param[out] pFindData    return found section handle.

    @return
        - @b TRUE continue to search next by  ps_op_SearchSection()
        - @b FALSE search to end.Use  ps_op_SearchSectionClose() to close search

    Example:
    @code
    {
        UINT32 bNext = FALSE;
        PSTORE_SEARCH_HANDLE SearchHandle;

        bNext =  ps_op_SearchSectionOpen(&SearchHandle);
        while(bNext)
        {
            DBG_DUMP("%15s 0x%08x  %d  0x%08x\r\n", SearchHandle.pSectionName,SearchHandle.uiSectionSize,SearchHandle.uiReadOnly,SearchHandle.uiDescCheckSum);
            bNext =  ps_op_SearchSection(&SearchHandle);
        }

         ps_op_SearchSectionClose();

        return TRUE;
    }
    @endcode
*/
extern BOOL  ps_op_SearchSectionOpen(PSTORE_SEARCH_HANDLE *pFindData);

/**
    Search PStore section.

    Search PStore section.

    @param[out] pFindData    return found section handle.

    @return
        - @b TRUE continue to search next by  ps_op_SearchSection()
        - @b FALSE search to end.Use  ps_op_SearchSectionClose() to close the search.

*/
extern BOOL  ps_op_SearchSection(PSTORE_SEARCH_HANDLE *pFindData);

/**
    Rewind PStore search section.

    Rewind the PStore search section to the beginning.

    @return
        - @b TRUE Rewind operation succeed.
        - @b FALSE Fail to rewind. Use  ps_op_SearchSectionClose() to close the search.
*/
extern BOOL  ps_op_SearchSectionRewind(void);

/**
    Close search PStore section.

    Close search PStore section.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
*/
extern ER  ps_op_SearchSectionClose(void);

/**
    Dump PSTORE structure and exist PSTORE_SECTION_DESC and PSTORE_SECTION_HANDLE
*/
extern void    ps_op_DumpPStoreInfo(void);

/**
    Dump PStore data to file system.

    The dump data including header,block table and section data.

    @param[in] pBuf        Working buffer for dump.
    @param[in] uiBufSize   Working buffer size.

    @return void.

*/
extern void    ps_op_Dump(UINT8 *pBuf,UINT32 uiBufSize);

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
extern ER      ps_op_CheckSection(char *pSecName,UINT32* ErrorSection);

/**
    Check total section block table.

    Check exist PStore block table first block number is empty (0xFF) or invalid value

    @param[out] ErrorSection        Return Section header error.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_HEADFAIL if check header error
*/
extern ER      ps_op_CheckTotalSection(UINT32* ErrorSection);

/**
    Enable update header.

    User can decide update which header.
    this API , ps_op_CheckTotalSection and  ps_op_RestoreHeader can use together.

    @param[in] Header       Update Header type(refer to PS_HEAD_UPDATE).

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail

    Example:
    @code
    {
         ps_op_EnableUpdateHeader(PS_UPDATE_ONEHEAD); //only update header 1,header 2 would not be update
         ps_op_CheckTotalSection(&ErrorSection);
        if(ErrorSection==PS_HEAD1)
             ps_op_RestoreHeader((UINT8 *)pBuf,PS_HEAD1);
    }
    @endcode

*/
extern ER      ps_op_EnableUpdateHeader(PS_HEAD_UPDATE Header);

/**
    Restore PStore header.

    Restore from good header to bad header.There are only 2 header,if choose PS_HEAD1,
    it would read header2 data and write to header1

    @param[in]  pBuf         Working buffer.
    @param[in]  RestoreType  Restore header1 or header2.

    @return
        - @b E_PS_OK if success
        - @b E_PS_SYS if fail
        - @b E_PS_PAR if pamameter error

    Example:
    @code
    {
         ps_op_CheckTotalSection(&ErrorSection);
        if(ErrorSection==PS_HEAD1)
             ps_op_RestoreHeader((UINT8 *)pBuf,PS_HEAD1);
    }
    @endcode

*/
extern ER      ps_op_RestoreHeader(UINT8 *pBuf , UINT32 RestoreType);

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
extern ER      ps_op_ReadOnly(char *pSecName, BOOL Enable);

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
extern ER      ps_op_ReadOnlyAll(BOOL Enable);

/**
    Enable/Disable check data after write section.

    If enable,write section would read data to temp buffer and compare with write data evey time.

    @note
    The performance of write section would be affected.

    @param[in]    Enable enable/disable function.

    @return void
*/
extern void    ps_op_EnableCheckData(BOOL Enable);

/**
    Check storage version to determine if format is needed.

    If Version is different from the value stored in storage PStore_Open() will fail and project should then call PStore_Format().

    @param[in]    Version project's specific version.

    @return void
*/
extern void   ps_op_SetPrjVersion(UINT32 Version);

#endif // _PSTORE_H

//@}
