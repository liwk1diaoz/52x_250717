#ifndef _PS_CMD_H
#define _PS_CMD_H


typedef enum
{
    DUMP_DESC0,
    DUMP_DESC1,
    DUMP_HEAD1,
    DUMP_HEAD2,
    ENUM_DUMMY4WORD(PS_DUMP_TYPE)
} PS_DUMP_TYPE;

extern UINT32 ps_getSize(void);
extern UINT32 ps_getBlkSize(void);
extern ER     ps_initDrv(STORAGE_OBJ *pStrg,PSTORE_INIT_PARAM* pInitParam);
extern ER     ps_uninitDrv(STORAGE_OBJ * pStrg);
extern ER     ps_readByBytes(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum, UINT16 usSectionID);
extern ER     ps_writeByBytes(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum, UINT16 usSectionID);
extern ER     ps_erase(STORAGE_OBJ *pStrgObj);
extern UINT32 ps_getFreeSpace(void);
extern UINT32 ps_dumpHeader(UINT32 uiHead, UINT8 *pBuf,UINT32 *uiBufSize);
extern ER     ps_getSectionHead(UINT16 usSectionID, UINT8 *pcHeadBuf);
extern ER     ps_setSectionHead(UINT16 usSectionID, UINT8 *pcHeadBuf);
extern ER     ps_getPStoreStruct(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum);
extern ER     ps_setPStoreStruct(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum);
extern ER     ps_getHeadDesc(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum);
extern ER     ps_setHeadDesc(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum);
extern ER     ps_initBlockTable(void);
extern ER     ps_getSecTable(UINT16 usSectionID);
extern ER     ps_setSecTable(UINT16 usSectionID,BOOL bCheck);
extern ER     ps_releaseSpace(UINT16 usSectionID, UINT32 ulStart, UINT32 ulEnd);
extern ER     ps_enableUpdateHeader(PS_HEAD_UPDATE Header);
extern ER     ps_restoreHeader(UINT8 * pBuf , UINT32 ReStoreType);
extern ER     ps_checkSingleSection(UINT8 * pcHeadBuf,UINT32 SectionIndex,UINT32  * ErrorSection);
extern ER     ps_clearAllSecTable(UINT8 *pBuf,PSFMT *pFmtStruct);


#endif
