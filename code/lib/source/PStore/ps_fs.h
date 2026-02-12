/**
    PStore driver export variables and function prototypes.

    PStore is novatek original simple file system.
    For access permanent storage in embeded system.

    @file       ps_fs.h
    @ingroup    mIPStore

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/


#ifndef _PS_FS_H
#define _PS_FS_H

#include <kwrap/type.h>
#include <strg_def.h>

void  ps_fs_setDrive(char drive);
ER  ps_fs_Open(STORAGE_OBJ *pStrgObj,PSTORE_INIT_PARAM* pInitParam);
ER  ps_fs_Close(void);
PSTORE_SECTION_HANDLE*  ps_fs_OpenSection(char *pSecName, UINT32 RWOperation);
ER  ps_fs_CloseSection(PSTORE_SECTION_HANDLE* pSectionHandle);
ER  ps_fs_DeleteSection(char *pSecName);
ER  ps_fs_ReadSection(UINT8 *pcBuffer,UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle);
ER  ps_fs_WriteSection(UINT8 *pcBuffer, UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle);
ER  ps_fs_Format(PSFMT *pFmtStruct);
UINT32 ps_fs_GetInfo(PS_INFO_ID info_id);
void ps_fs_DumpPStoreInfo(void);
ER ps_fs_CheckSection(char *pSecName,UINT32 *ErrorSection);
ER ps_fs_SetDevNode(char *pDevNode);

#endif // #define _PS_FS_H


//@}

