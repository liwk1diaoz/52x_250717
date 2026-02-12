
/*
    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.

    @file       ps_int.h
    @ingroup

    @brief

    @note       Nothing.

*/

#ifndef _PS_INT_H
#define _PS_INT_H

//#include "Utility.h"
#include "PStore.h"

extern ID _SECTION(".kercfg_data") FLG_ID_PSTORE;
extern ID _SECTION(".kercfg_data") SEMID_PS_SEC1;
extern ID _SECTION(".kercfg_data") SEMID_PS_SEC2;


////////////////////////////////////////////
#define PS_HEAD_SECTION              0                  //The location of header section
#define PS_SECTION_START             0
//#define PS_MAX_SECTION               64        //Number of section
#define PS_BYTES_A_BLOCK_ADDR        2
//#define PS_MAX_BLOCK_A_SECTION       512      //How much blocks a section supported
#define PS_NUM_BLOCK_TABLE           3        //Number of block table for table swarp operation. 2K page,header would put in block 0 and block 1
#define PS_SECTION_HEADER_SIZE       (PS_BYTES_A_BLOCK_ADDR*PS_MAX_BLOCK_A_SECTION)
//#define PS_DESC_BLOCK                1          //pstore structure and descriptor reserved one block
#define PS_STRUCT_BLOCK                 1         //pstore structure
#define PS_PERFORMANCE               0


#define PS_BUF_RSV_CHK_BYTE (4)
#define CFG_NVT_PS_TAG      MAKEFOURCC('N','T','P','S')

/**
    Section descriptor structure.
    Each section structure is consist of 8 bytes, and please don't change the structure size(for section structure array).
*/
typedef struct
{
    char pSectionName[SEC_NAME_LEN];            ///<Section name ID
    UINT32 uiSectionSize;          ///<Section size
    UINT32 uiReadOnly;              ///<Section read only
    UINT32 uiDescCheckSum;         ///<check sum of descriptor
} PSTORE_SECTION_DESC, *PPSTORE_SECTION_DESC;


/* Structure storage information*/
typedef struct
{
    UINT32 uiBlockSize;             //< block size in bytes
    UINT32 uiSectorPerBlock;          //< sectors in a block
    UINT32 uiLogicalBlockCount;     //< Logical blocks in a disk
}PSTRG_INFO, *PPSTRG_INFO;

typedef enum _PS_HANDLE_ID_
{
    PS_HANDLE1 = 0,
    PS_HANDLE2,
    ENUM_DUMMY4WORD(_PS_HANDLE_ID_)
} PS_HANDLE_ID;


extern PSTORE gPStoreStruct;
extern UINT8  *gucSecTable;
extern BOOL   bSingleStructBlk;
extern UINT32 PS_DESC_BLOCK;//pstore structure and descriptor reserved total block

#define PS_MAX_SECTION               (gPStoreStruct.uiSectionNum)  //Number of section
#define PS_MAX_BLOCK_A_SECTION       (gPStoreStruct.uiMaxBlkPerSec)       //How much blocks a section supported (Max 8M,if 16K/block)

extern void PST_InstallCmd(void);

#endif
