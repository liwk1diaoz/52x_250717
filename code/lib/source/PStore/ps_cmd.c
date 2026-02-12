/**
    @file       NAND_PS.c
    @ingroup    mIPStoreNAND

    @brief      Nand flash / Smartmedia Card PStore driver
                Physical access functions, logical access functions, logical/physical mapping,
                and etc are included in this file.
    @note

    @date       2007/12/04

    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

*/
#include <string.h>
// 520 rtos todo //#include "SysKer.h"
// 520 rtos todo //#include "cache.h"
#include "ps_int.h"
#include <strg_def.h>
#include "ps_cmd.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PSC
#define __DBGLVL__          1 // 0=OFF, 1=ERROR, 2=TRACE
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[RW]"
//#define __DBGFLT__          "[TC]"
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

#define PS_TABLE_NOT_FOUND           0xFFFF   //Block table of the section has not found in memory.
#define PS_EMPTY_BLOCK               0xFFFF   //0xFFFF means no any block address mapped
#define PS_HEADER_NUM                2
#define PS_LEVEL1_SIZE               512         //Support up to 64 * 32 blocks (Default 32MB,if 16K/block)
#define PS_MAX_PS_SIZE               PS_LEVEL1_SIZE*32      //Maximum capacity = PS_MAX_PS_SIZE * Block size

UINT8  *gucSecTable;   //for two section use.
UINT32 PS_DESC_BLOCK =0;

static PSTRG_INFO g_StrgInfo;
static PSTORAGE_OBJ g_pStrgObj;

static UINT16 gusSecTableIndex[PS_NUM_BLOCK_TABLE];    //for two section use.
static UINT32 guiLevelOneMapping[PS_LEVEL1_SIZE];      //Free block mapping  , 0 for free block 1 for otherwise
static UINT8 *gpucPSBlockBuffer=0;
static UINT32 guiPSBlockBufferSize=0;
static UINT32 guiHeadBlock; //Number of reserved block for PStore header. 2K page,header would put in block 1 and block 2
static UINT32 guiDataStarBlock=0;
static UINT32 guiHead1StarBlock=0;
static UINT32 guiHead2StarBlock=0;
static UINT32 guiPStoreUpdateHead = PS_UPDATE_ALLHEAD;  //enable update Pstore Header
static UINT32 guiUsedBlkNum =0;
static ER     ps_readBlockByBytes(UINT8 *pcBuf, UINT16 block, UINT32 usStartByte,UINT32 bytes);
static ER     ps_writeBlockByBytes(UINT8 *pcBuf, UINT16 block, UINT32 usStartByte, UINT32 bytes);
static ER     ps_writeBlockSeq(UINT8 *pcBuf, UINT16 block, UINT16 count);
static ER     ps_readBlockSeq(UINT8 *pcBuf, UINT16 block, UINT16 count);
static BOOL   ps_getBlockStartAdd(UINT16 usSectionID, UINT32 DataAdd, UINT16 *usLogicalAdd);
static void   ps_setBlockAddTbl(UINT16 usSectionID, UINT32 DataAdd, UINT16 usLogicalAdd);
static void   ps_getBlockAddTbl(UINT16 usSectionID, UINT32 DataAdd, UINT16 *usLogicalAdd);
static UINT16 ps_getSecTableID(UINT16 usSectionID);

static BOOL   ps_setFreeLogicalBlock(UINT16 BlockNum);
static ER     ps_getFreeLogicalBlock(UINT16 *usBlockNum);
static ER     ps_checkSingleSectionByHeader(UINT8 * pcHeadBuf,UINT32 Start_Block,UINT32 usSectionID);
static void   ps_printInfo(PSTRG_INFO *StrgInfo);
INT32  ps_getStrgInfo(STORAGE_OBJ *pStrgObj,PSTRG_INFO *StrgInfo);

/**
    Get PStore size

    @Param  void

    @return     UINT32      PStore size in byte unit.
*/
UINT32 ps_getSize(void)
{
    return g_StrgInfo.uiLogicalBlockCount*g_StrgInfo.uiBlockSize;
}

UINT32 ps_getBlkSize(void)
{
    return g_StrgInfo.uiBlockSize;
}

/**
    Initial storage driver

    @return             E_PS_OK or E_PS_SYS
*/
ER ps_initDrv(STORAGE_OBJ *pStrg,PSTORE_INIT_PARAM* pInitParam)
{
    ER erReturn = E_PS_OK;

    DBG_IND("pStrg 0x%x",pStrg);

    g_pStrgObj = pStrg;

    erReturn = pStrg->Close();

    if(erReturn == E_PS_OK)
    {
        erReturn = pStrg->Open();

        if(erReturn == E_PS_OK)
        {
            ps_getStrgInfo(pStrg,&g_StrgInfo);

            if(g_StrgInfo.uiLogicalBlockCount == 0)
            {
                DBG_ERR("PS size is zero\r\n");
                return E_PS_SYS;
            }

            if(g_StrgInfo.uiLogicalBlockCount > PS_MAX_PS_SIZE)
            {
                DBG_ERR("Block table 0x%x is not enough 0x%x blk\r\n",PS_MAX_PS_SIZE,g_StrgInfo.uiLogicalBlockCount);
                return E_PS_SYS;
            }

            //Check PStore parameter
            if(g_StrgInfo.uiBlockSize == 0)
            {
                DBG_ERR("Block size is zero\r\n");
                return E_PS_SYS;
            }

            if((pInitParam->pBuf!=0) && (pInitParam->uiBufSize!=0) && (pInitParam->uiBufSize >= g_StrgInfo.uiBlockSize) )
            {
                gpucPSBlockBuffer = pInitParam->pBuf;       //assign tmp buffer,must word alignment
                guiPSBlockBufferSize = g_StrgInfo.uiBlockSize;  //use block size as tmp buffer
                DBG_IND("set Buffer 0x%x,size 0x%x\r\n",gpucPSBlockBuffer,guiPSBlockBufferSize);
            }
            else
            {
                DBG_ERR("set Buffer fail\r\n");
                return E_PS_SYS;
            }

        }
        else
        {
            DBG_ERR("Open driver fail\r\n");
        }
    }
    else
    {
        DBG_ERR("Close driver fail\r\n");
    }
     return erReturn;
}

/**
    Uninit storage driver

    @return             E_PS_OK or E_PS_SYS
*/
ER ps_uninitDrv(STORAGE_OBJ * pStrg)
{
    ER erReturn = E_PS_OK;

    DBG_IND("pStrg 0x%x", pStrg);

    if(pStrg && pStrg->Close)
        erReturn = pStrg->Close();

    if (erReturn != E_PS_OK)
    {
        DBG_ERR("Close storage %p failed, err %d\r\n", pStrg, erReturn);
        return erReturn;
    }

    g_pStrgObj = NULL;
    return E_PS_OK;
}

/**
    Read data from PStore area by byte unit
    @param pcBuf        [in] The read-out data.
    @param ulByteStart  Where is the data read from.
    @param ulByteNum    How many bytes to read.
    @param usSectionID  Section ID.
    @return             E_PS_OK or E_PS_SYS

    Note : Read data from the header section is prohibited by the function.
*/
ER ps_readByBytes(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum, UINT16 usSectionID)
{
    UINT32 uiStartAdd, uiDataLength, ulTemp,usTemp,  usBlockNum;
    UINT16 usBlockAddress = 0;
    ER erReturn = E_PS_OK;

    DBG_IND("[RW]ulByteStart %d ulByteNum %d usSectionID %d \r\n",ulByteStart,ulByteNum,usSectionID);

    if((ulByteStart + ulByteNum) > (PS_MAX_BLOCK_A_SECTION * g_StrgInfo.uiBlockSize))//PS_MAX_BLOCK_A_SECTION * BlockSize
    {
        DBG_ERR("Out of range %x\r\n", (ulByteStart + ulByteNum));
        return E_PS_SYS;
    }

    uiStartAdd = ulByteStart;
    uiDataLength = ulByteNum;

    //Don't return error directly when error found.
    if(ps_getBlockStartAdd(usSectionID, uiStartAdd, &usBlockAddress) != TRUE)
        erReturn = E_PS_SYS;

    if((usBlockAddress & PS_EMPTY_BLOCK)== PS_EMPTY_BLOCK)
    {
        DBG_ERR("Data is not existence(Section ID:%d Addr:0x%x)\n\r", usSectionID, uiStartAdd);
        erReturn = E_PS_SYS;
    }

    //read the data which are not block-aligned from the beginning
    usTemp = uiStartAdd % g_StrgInfo.uiBlockSize;

    if(((usTemp) != 0) && (erReturn == E_PS_OK))
    {
        //Calculate a correct data length for read operation
        if((usTemp + uiDataLength) > g_StrgInfo.uiBlockSize)
            ulTemp = g_StrgInfo.uiBlockSize - usTemp;
        else
            ulTemp = uiDataLength;

		//coverity[tainted-data]
        if(ps_readBlockByBytes(pcBuf, usBlockAddress, usTemp, ulTemp) != E_PS_OK)
            erReturn = E_PS_SYS;
        else
        {
            uiDataLength -= ulTemp;
            uiStartAdd += ulTemp;
            pcBuf += ulTemp;
        }
    }

    //read the data which are block-aligned
    usBlockNum = uiDataLength / g_StrgInfo.uiBlockSize;
    while((usBlockNum > 0) && (erReturn == E_PS_OK))
    {
        ps_getBlockStartAdd(usSectionID, uiStartAdd, &usBlockAddress);
        if((usBlockAddress & PS_EMPTY_BLOCK)== PS_EMPTY_BLOCK)
        {
            DBG_ERR("Data is not existence(Section ID:%d Addr:0x%x)\n\r", usSectionID, uiStartAdd);
            erReturn = E_PS_SYS;
        }
        else
        {

		//coverity[tainted_data]
            if(ps_readBlockSeq(pcBuf, usBlockAddress, 1) != E_PS_OK)
                erReturn = E_PS_SYS;
            else
            {
                uiDataLength -= g_StrgInfo.uiBlockSize;
                uiStartAdd += g_StrgInfo.uiBlockSize;
                pcBuf += g_StrgInfo.uiBlockSize;
                usBlockNum--;
            }
        }
    }

    //read the rest data which are not block-aligned
    if((uiDataLength > 0) && (erReturn == E_PS_OK))
    {
        ps_getBlockStartAdd(usSectionID, uiStartAdd, &usBlockAddress);
        if((usBlockAddress & PS_EMPTY_BLOCK) == PS_EMPTY_BLOCK)
        {
            DBG_ERR("Data doesn't exist(Section ID:%d Addr:0x%x)\n\r", usSectionID, uiStartAdd);
            erReturn =  E_PS_SYS;
        }
        else
        {
            if(ps_readBlockByBytes(pcBuf, usBlockAddress, 0, uiDataLength) != E_PS_OK)
                erReturn = E_PS_SYS;
        }
    }

    return erReturn;
}


/**
    Write data to PStore area by byte unit.
    @param pcBuf        [in] The write-in data.
    @param ulByteStart  Where is the data be written to.
    @param ulByteNum    How many bytes to written.
    @param usSectionID  Section ID.
    @return             E_PS_OK or E_PS_SYS

    Note : Write data to the header section is prohibited by the function.
*/
ER ps_writeByBytes(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum, UINT16 usSectionID)
{
    UINT32 uiStartAdd, uiDataLength, ulTemp,usTemp,  usBlockNum;
    UINT16 usBlockAddress = 0;
    ER erReturn = E_PS_OK;

    DBG_IND("[RW]ulByteStart %d ulByteNum %d usSectionID %d \r\n",ulByteStart,ulByteNum,usSectionID);

    if((ulByteStart + ulByteNum) > (PS_MAX_BLOCK_A_SECTION * g_StrgInfo.uiBlockSize))//PS_MAX_BLOCK_A_SECTION * BlockSize
    {
        DBG_ERR("Out of range (Addr:0x%x)  0x%x\r\n", (ulByteStart + ulByteNum) , (PS_MAX_BLOCK_A_SECTION * g_StrgInfo.uiBlockSize));
        return E_PS_SYS;
    }

    uiStartAdd = ulByteStart;
    uiDataLength = ulByteNum;

    //write the data which are not block-aligned from the beginning
    usTemp = uiStartAdd % g_StrgInfo.uiBlockSize;

    if(((usTemp) != 0) && (erReturn == E_PS_OK))
    {
        ps_getBlockStartAdd(usSectionID, uiStartAdd, &usBlockAddress);
        if((usBlockAddress & PS_EMPTY_BLOCK) == PS_EMPTY_BLOCK)
        {
            if(ps_getFreeLogicalBlock(&usBlockAddress) != E_PS_OK)
            {
                DBG_ERR("No more free logic block number for allocated\r\n");
                erReturn = E_PS_SYS;
            }
            else
            {
                //Update the Block table of the section.
                ps_setBlockAddTbl(usSectionID, uiStartAdd, usBlockAddress);
            }
        }
        //if((uiStartAdd + uiDataLength) > g_StrgInfo.uiBlockSize)
        if((usTemp + uiDataLength) > g_StrgInfo.uiBlockSize)
            ulTemp = g_StrgInfo.uiBlockSize - usTemp;
        else
            ulTemp = uiDataLength;

        if(erReturn == E_PS_OK)
        {
            if(ps_writeBlockByBytes(pcBuf, usBlockAddress, usTemp, ulTemp) != E_PS_OK)
                erReturn = E_PS_SYS;

            uiDataLength -= ulTemp;
            uiStartAdd += ulTemp;
            pcBuf += ulTemp;
        }
    }

    //write the data which are block-aligned
    usBlockNum = uiDataLength / g_StrgInfo.uiBlockSize;
    while((usBlockNum > 0) && (erReturn == E_PS_OK))
    {
        ps_getBlockStartAdd(usSectionID, uiStartAdd, &usBlockAddress);
        if((usBlockAddress & PS_EMPTY_BLOCK) == PS_EMPTY_BLOCK)
        {
            if(ps_getFreeLogicalBlock(&usBlockAddress) != E_PS_OK)
            {
                erReturn = E_PS_SYS;
                break;
            }
            else
            {
                //Update the block table of the section.
                ps_setBlockAddTbl(usSectionID, uiStartAdd, usBlockAddress);
            }
        }

		//coverity[tainted_data]
        if(ps_writeBlockSeq(pcBuf, usBlockAddress, 1) != E_PS_OK)
        {
            erReturn = E_PS_SYS;
            break;
        }

        uiDataLength -= g_StrgInfo.uiBlockSize;
        uiStartAdd += g_StrgInfo.uiBlockSize;
        pcBuf += g_StrgInfo.uiBlockSize;
        usBlockNum--;
    }

    //write the rest data which are not block-aligned
    if((uiDataLength > 0) && (erReturn == E_PS_OK))
    {
        ps_getBlockStartAdd(usSectionID, uiStartAdd, &usBlockAddress);
        if((usBlockAddress & PS_EMPTY_BLOCK) == PS_EMPTY_BLOCK)
        {
            if(ps_getFreeLogicalBlock(&usBlockAddress) != E_PS_OK)
                erReturn = E_PS_SYS;
            else
            {
                ps_setBlockAddTbl(usSectionID, uiStartAdd, usBlockAddress);
            }
        }

        if(erReturn == E_PS_OK)
        {
            if(ps_writeBlockByBytes(pcBuf, usBlockAddress, 0, uiDataLength) != E_PS_OK)
                erReturn = E_PS_SYS;
        }
    }

    return erReturn;
}

/*
    Erase all data (include PStore header, Mapping table and data area) in PStore area.

*/
ER ps_erase(STORAGE_OBJ *pStrgObj)
{
    ER erReturn = E_PS_OK;
    DBG_IND("ps_erase\r\n");

    if(pStrgObj->ExtIOCtrl)
        erReturn = pStrgObj->ExtIOCtrl(STRG_EXT_CMD_FORMAT,0,0);

    return erReturn;
}


/*

*/
UINT32 ps_getFreeSpace(void)
{
    //DBG_ERR("g_StrgInfo.uiLogicalBlockCount  %d,guiUsedBlkNum %d ,g_StrgInfo.uiBlockSiz %d\r\n",g_StrgInfo.uiLogicalBlockCount,guiUsedBlkNum,g_StrgInfo.uiBlockSize);
    return (g_StrgInfo.uiLogicalBlockCount - guiUsedBlkNum)*g_StrgInfo.uiBlockSize;
}
UINT32 ps_dumpHeader(UINT32 uiHead, UINT8 *pBuf,UINT32 *uiBufSize)
{
    UINT32 fileSize = 0;
    UINT32 blkSize = g_StrgInfo.uiBlockSize;

    UINT32  ucNumBlock;
    ER     erReturn = E_PS_OK;


    if(uiHead==DUMP_DESC0) //dump desc 0
    {
        erReturn = ps_readBlockSeq(pBuf, 0, PS_DESC_BLOCK);
        if(erReturn != E_PS_OK)
        {
            DBG_ERR("Dump Desc fail \r\n");
        }
        fileSize = PS_DESC_BLOCK *blkSize;

    }
    else if(uiHead==DUMP_DESC1) //dump desc 1
    {
        erReturn = ps_readBlockSeq(pBuf, PS_DESC_BLOCK, PS_DESC_BLOCK);
        if(erReturn != E_PS_OK)
        {
            DBG_ERR("Dump Desc fail \r\n");
        }
        fileSize = PS_DESC_BLOCK *blkSize;

    }
    else if(uiHead==DUMP_HEAD1) //dump head1
    {
        for(ucNumBlock = guiHead1StarBlock ;ucNumBlock< guiHead1StarBlock+guiHeadBlock ; ucNumBlock ++)
        {
            erReturn = ps_readBlockByBytes(pBuf+(ucNumBlock-guiHead1StarBlock)*blkSize, ucNumBlock,  0, blkSize);
            if(erReturn != E_PS_OK)
            {
                DBG_ERR("Dump Head1 fail \r\n");
            }
        }
        fileSize =guiHeadBlock*blkSize;

    }
    else if(uiHead==DUMP_HEAD2)//dump head2
    {
        for(ucNumBlock = guiHead2StarBlock ;ucNumBlock< guiHead2StarBlock+guiHeadBlock ; ucNumBlock ++)
        {
            erReturn = ps_readBlockByBytes(pBuf+(ucNumBlock-guiHead2StarBlock)*blkSize, ucNumBlock,  0, blkSize);
            if(erReturn != E_PS_OK)
            {
                DBG_ERR("Dump Head2 fail \r\n");
            }
        }
        fileSize =guiHeadBlock*blkSize;
    }


    if(erReturn ==E_PS_OK)
    {
        if(*uiBufSize>=fileSize)
        {
            *uiBufSize = fileSize;
        }
        else
        {
            erReturn = E_PS_SIZE;
            DBG_ERR("uiBufSize 0x%X too small\r\n",*uiBufSize);
        }
    }
    return erReturn;

}
#if 0
#pragma mark -
#endif

/**
    Read system parameters from NAND with multiple blocks in a sequence.\n

    @param pcBuf Data buffer (data size must be (block size * count))
    @param block Block number to be read (logical block address)
    @param count The number of sequencial block
    @return E_PS_OK or E_PS_SYS
*/
static ER ps_readBlockSeq(UINT8 *pcBuf, UINT16 block, UINT16 count)
{
    UINT32 uiSecNum = 0;
    UINT32 uiSecCnt = g_StrgInfo.uiSectorPerBlock;
    UINT16 i;
    ER ret;

    if ((block + count) > g_StrgInfo.uiLogicalBlockCount)
    {
        DBG_ERR("Invalid logic block number %d\n\r", block);
        return E_PS_SYS;
    }

    for (i = block; i < (block + count); i++)
    {
        uiSecNum = i*g_StrgInfo.uiSectorPerBlock;

        //Avoid word alignment issue
        //ret = nand_readOperation((INT8 *)pcBuf, uiBlockAddresss, g_StrgInfo.uiSectorPerBlock);
        //#NT# Utilied the read action. Read data to storage memory pool first, and then copy to assigned memory.
#if 1
        if((INT32)pcBuf & 0x3)
        {
            DBG_WRN("pcBufr 0x%x not alignment\n\r", pcBuf);
            ret = g_pStrgObj->RdSectors((INT8 *)gpucPSBlockBuffer, uiSecNum, uiSecCnt);

            memcpy((UINT8 *)pcBuf, gpucPSBlockBuffer, g_StrgInfo.uiBlockSize);
        }

        else //Word alignment address
        {
            //# CPU Clean and Invalid cache
            //CPUCleanInvalidateDCacheAll() ;
            //CPUCleanInvalidateDCacheBlock((UINT32)pcBuf, (UINT32)(pcBuf + g_StrgInfo.uiBlockSize));
            //CPUDrainWriteBuffer();
            //ret = pNandRWFunc->nand_readOperation((INT8 *)pcBuf, uiBlockAddresss, g_StrgInfo.uiSectorPerBlock);
            ret = g_pStrgObj->RdSectors((INT8 *)pcBuf, uiSecNum,uiSecCnt);

        }
#else
        ret = g_pStrgObj->RdSectors((INT8 *)gpucPSBlockBuffer, uiSecNum, uiSecCnt);
        memcpy((UINT8 *)pcBuf, gpucPSBlockBuffer, g_StrgInfo.uiBlockSize);
#endif

        DBG_IND("[RW]RdSectors pcBufr 0x%x uiSecNum %d uiSecCnt %d ret %d\r\n",(UINT8 *)pcBuf,uiSecNum,uiSecCnt,ret);

        if (ret != E_PS_OK)
        {
            DBG_ERR("Read logic %ld failed.\n\r", block);
            return E_PS_SYS;
        }
        pcBuf += g_StrgInfo.uiBlockSize;
    }

    return E_PS_OK;
}

/**
    Write system parameters to NAND with multiple blocks in a sequence.

    @param pcBuf Data buffer (data size must be (block size * count))
    @param block Block number to be written (Logical block address)
    @param count The number of sequencial block
    @return E_PS_OK or E_PS_SYS
*/
static ER ps_writeBlockSeq(UINT8 *pcBuf, UINT16 block, UINT16 count)
{
    UINT32 uiSecNum = 0;
    UINT32 uiSecCnt = g_StrgInfo.uiSectorPerBlock;
    UINT16 i;
    ER ret;

    if( guiPStoreUpdateHead == PS_UPDATE_ONEHEAD)
    {
        if( ((block>=guiHead2StarBlock) && (block < guiDataStarBlock)) )
        {
            DBG_WRN("can't update backup header %d \n\r",block);
            return E_PS_OK;
        }

    }
    else if( guiPStoreUpdateHead == PS_UPDATE_ZEROHEAD)
    {
        if( block < guiDataStarBlock )
        {
            DBG_ERR("can't update any header\n\r");
            return E_PS_SYS;
        }
    }

    if ((block + count) > g_StrgInfo.uiLogicalBlockCount)
    {
        DBG_ERR("Out of range %d %d\n\r", block + count, g_StrgInfo.uiLogicalBlockCount);
        return E_PS_SYS;
    }

    for (i = block; i < (block + count); i++)
    {
        uiSecNum = i*g_StrgInfo.uiSectorPerBlock;
        do
        {
            //Avoid word alignment issue
            if((INT32)pcBuf & 0x3)
            {
                DBG_WRN("pcBufr 0x%x not alignment\n\r", pcBuf);

                memcpy(gpucPSBlockBuffer, (UINT8 *)pcBuf, g_StrgInfo.uiBlockSize);

                ret = g_pStrgObj->WrSectors((INT8 *)gpucPSBlockBuffer, uiSecNum, uiSecCnt);
            }
            else //Word alignment address
            {

                //# CPU Invalid cache
                //CPUCleanDCacheBlock((UINT32)pcBuf, (UINT32)(pcBuf + g_StrgInfo.uiBlockSize));
                //CPUCleanDCacheAll();
                //CPUDrainWriteBuffer();
                ret = g_pStrgObj->WrSectors((INT8 *)pcBuf, uiSecNum, uiSecCnt);
            }

            DBG_IND("[RW]WrSectors pcBufr 0x%x uiSecNum %d uiSecCnt %d ret %d\r\n",(UINT8 *)pcBuf,uiSecNum,uiSecCnt,ret);

            if (ret != E_PS_OK)
            {
                DBG_ERR("write data to block %d fail\n\r",i);
                return E_PS_SYS;

            }
            else
            {
                pcBuf += g_StrgInfo.uiBlockSize;
                break;
            }
        }while(1);
    }

    return E_PS_OK;
}



/* ps_eraseBlock*/
/**
    Read system parameters from NAND with bytes.

    @param pcBuf        Data buffer
    @param block        Block number to be read
    @param usStartByte  Where is the data read from.
    @param bytes        How many bytes to be read in bytes
    @return             E_PS_OK or E_PS_SYS
    @note Read bytes can not cross a block
*/
static ER ps_readBlockByBytes(UINT8 *pcBuf, UINT16 block, UINT32 usStartByte,UINT32 bytes)
{
    UINT32 cpbytes =0;
    UINT32 cpblks =0;

    if(bytes == 0)
    {
        DBG_ERR("Data length is zero\n\r");
        return E_PS_SYS;
    }

    if(usStartByte)
    {
        if(ps_readBlockSeq(gpucPSBlockBuffer, block, 1) != E_PS_OK)
        {
            memset(gpucPSBlockBuffer, 0xFF, g_StrgInfo.uiBlockSize);
            DBG_ERR("Read logical %ld failed.\n\r", block);
            return E_PS_SYS;

        }
        if(usStartByte+bytes > g_StrgInfo.uiBlockSize)
            cpbytes = g_StrgInfo.uiBlockSize - usStartByte;
        else
            cpbytes = bytes;

        //memcpy((gpucPSBlockBuffer + usStartByte), pcBuf, cpbytes);
        memcpy(pcBuf, (gpucPSBlockBuffer + usStartByte) , cpbytes);

    }

    if(cpbytes)
    {
        bytes = bytes-cpbytes;
        pcBuf = pcBuf+cpbytes;
        block = block+1;
    }

    if(bytes/g_StrgInfo.uiBlockSize)
    {
        cpblks = bytes/g_StrgInfo.uiBlockSize;
        //write whole block to buffer directly
        if(ps_readBlockSeq(pcBuf, block, bytes/g_StrgInfo.uiBlockSize) != E_PS_OK)
        {
            DBG_ERR("Write logical %ld failed.\n\r", block);
            return E_PS_SYS;
        }
    }

    if(bytes%g_StrgInfo.uiBlockSize)
    {
        bytes = bytes%g_StrgInfo.uiBlockSize;
        pcBuf = pcBuf+cpblks*g_StrgInfo.uiBlockSize;
        block = block+cpblks;

        if(ps_readBlockSeq(gpucPSBlockBuffer, block, 1) != E_PS_OK)
        {
            memset(gpucPSBlockBuffer, 0xFF, g_StrgInfo.uiBlockSize);
            DBG_ERR("Read logical %ld failed.\n\r", block);
            return E_PS_SYS;

        }

        //memcpy(gpucPSBlockBuffer, pcBuf, bytes);
        memcpy(pcBuf, gpucPSBlockBuffer , bytes);
    }
    return E_PS_OK;
}

/**
    Write system parameters to NAND, in bytes unit.

    @param pcBuf        Data buffer
    @param block        Block number to be written (Logical block address)
    @param usStartByte  Where is the data written to.
    @param bytes        How many bytes to be written, in byte unit
    @return             E_PS_OK or E_PS_SYS
    @note Written bytes cannot cross a block
*/
static ER ps_writeBlockByBytes(UINT8 *pcBuf, UINT16 block, UINT32 usStartByte, UINT32 bytes)
{
    //DBG_IND("[RW]block 0x%x usStartByte bytes 0x%x \r\n",block,bytes);
    UINT32 cpbytes =0;
    UINT32 cpblks =0;
    if(bytes == 0)
    {
        DBG_ERR("Data length is zero\n\r");
        return E_PS_SYS;
    }

    if (block > g_StrgInfo.uiLogicalBlockCount)
    {
        DBG_ERR("Invalid logic block address %d\n\r", block);
        return E_PS_SYS;
    }


    if(usStartByte)
    {
        if(ps_readBlockSeq(gpucPSBlockBuffer, block, 1) != E_PS_OK)
        {
            memset(gpucPSBlockBuffer, 0xFF, g_StrgInfo.uiBlockSize);
            DBG_ERR("Read logical %ld failed.\n\r", block);
            return E_PS_SYS;

        }
        if(usStartByte+bytes > g_StrgInfo.uiBlockSize)
            cpbytes = g_StrgInfo.uiBlockSize - usStartByte;
        else
            cpbytes = bytes;

        memcpy((gpucPSBlockBuffer + usStartByte), pcBuf, cpbytes);

        if(ps_writeBlockSeq(gpucPSBlockBuffer, block, 1) != E_PS_OK)
        {
            DBG_ERR("Write logical %ld failed.\n\r", block);
            return E_PS_SYS;
        }
    }
    if(cpbytes)
    {
        bytes = bytes-cpbytes;
        pcBuf = pcBuf+cpbytes;
        block = block+1;
    }

    if(bytes/g_StrgInfo.uiBlockSize)
    {
        cpblks = bytes/g_StrgInfo.uiBlockSize;
        //write whole block to buffer directly
        if(ps_writeBlockSeq(pcBuf, block, bytes/g_StrgInfo.uiBlockSize) != E_PS_OK)
        {
            DBG_ERR("Write logical %ld failed.\n\r", block);
            return E_PS_SYS;
        }
    }

    if(bytes%g_StrgInfo.uiBlockSize)
    {
        bytes = bytes%g_StrgInfo.uiBlockSize;
        pcBuf = pcBuf+cpblks*g_StrgInfo.uiBlockSize;
        block = block+cpblks;

        if(ps_readBlockSeq(gpucPSBlockBuffer, block, 1) != E_PS_OK)
        {
            memset(gpucPSBlockBuffer, 0xFF, g_StrgInfo.uiBlockSize);
            DBG_ERR("Read logical %ld failed.\n\r", block);
            return E_PS_SYS;

        }

        memcpy(gpucPSBlockBuffer, pcBuf, bytes);

        if(ps_writeBlockSeq(gpucPSBlockBuffer, block, 1) != E_PS_OK)
        {
            DBG_ERR("Write logical %ld failed.\n\r", block);
            return E_PS_SYS;
        }
    }
    return E_PS_OK;
}



#if 0
#pragma mark -
#endif

//Block mapping table related function
/**
    create a block mapping table

    @return               E_PS_OK if Success or E_PS_SYS if failure
*/


/**
    Get section data for header area.

    @param usSectionID    Section ID
    @param pcHeadBuf      Data buffer for store the section data

    @return               E_PS_OK if Success or E_PS_SYS if failure
*/
ER ps_getSectionHead(UINT16 usSectionID, UINT8 *pcHeadBuf)//UINT8 usSectionID, INT8 *pcbuf)
{
    UINT32 ucLogicalBlkAdd,ucLogicalBlkAdd1,ucLogicalBlkAdd2;
    UINT32 ulDataAddress;
    ER erReturn;

    if(PS_SECTION_HEADER_SIZE<=g_StrgInfo.uiBlockSize)
    {
        ucLogicalBlkAdd = usSectionID /(g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE);
        ulDataAddress = (usSectionID % (g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE)) * PS_SECTION_HEADER_SIZE;
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + guiHead1StarBlock;
    }
    else
    {
        ucLogicalBlkAdd = (usSectionID*PS_SECTION_HEADER_SIZE)/g_StrgInfo.uiBlockSize;
        ulDataAddress =  (usSectionID*PS_SECTION_HEADER_SIZE)%g_StrgInfo.uiBlockSize;
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + guiHead1StarBlock;
    }
    erReturn = ps_readBlockByBytes(pcHeadBuf, ucLogicalBlkAdd1, ulDataAddress, PS_SECTION_HEADER_SIZE);

    DBG_IND("[RW]usSectionID %d ucLogicalBlkAdd 0x%x \r\n,",usSectionID,ucLogicalBlkAdd1,ulDataAddress);

    //Backup PStore header to another block
    if(erReturn != E_PS_OK)
    {
        ucLogicalBlkAdd2 = ucLogicalBlkAdd + guiHead2StarBlock;
        erReturn = ps_readBlockByBytes(pcHeadBuf, ucLogicalBlkAdd2, ulDataAddress, PS_SECTION_HEADER_SIZE);

        if(erReturn != E_PS_OK)
            DBG_ERR("ps_getSectionHead failed\r\n");

    }
    return erReturn;
}

/**
    Set section data to header area.

    @param usSectionID    Section ID
    @param pcHeadBuf      Data buffer for store the written data

    @return               E_PS_OK if Success or E_PS_SYS if failure
*/
ER ps_setSectionHead(UINT16 usSectionID, UINT8 *pcHeadBuf)//UINT8 usSectionID, INT8 *pcbuf)
{
    UINT32 ucLogicalBlkAdd,ucLogicalBlkAdd1,ucLogicalBlkAdd2;
    UINT32 ulDataAddress;
    if(PS_SECTION_HEADER_SIZE<=g_StrgInfo.uiBlockSize)
    {
        ucLogicalBlkAdd = usSectionID /(g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE);//calculate in which block the ID should be
        ulDataAddress = (usSectionID % (g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE)) * PS_SECTION_HEADER_SIZE;//calculate the address offset of the block
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + guiHead1StarBlock;
    }
    else
    {
        ucLogicalBlkAdd = (usSectionID*PS_SECTION_HEADER_SIZE)/g_StrgInfo.uiBlockSize;

        ulDataAddress =  (usSectionID*PS_SECTION_HEADER_SIZE)%g_StrgInfo.uiBlockSize;

        ucLogicalBlkAdd1 = ucLogicalBlkAdd + guiHead1StarBlock;
    }
    DBG_IND("[RW]usSectionID %d ucLogicalBlkAdd 0x%x\r\n,",usSectionID,ucLogicalBlkAdd1,ulDataAddress);

    if(ps_writeBlockByBytes(pcHeadBuf, ucLogicalBlkAdd1, ulDataAddress, PS_SECTION_HEADER_SIZE) != E_PS_OK)
    {
        DBG_ERR("ps_setSectionHead 0 failed\r\n");
        return E_PS_SYS;
    }

    //Backup PStore header to another block
    ucLogicalBlkAdd2 = ucLogicalBlkAdd + guiHead2StarBlock;
    if(ps_writeBlockByBytes(pcHeadBuf, ucLogicalBlkAdd2, ulDataAddress, PS_SECTION_HEADER_SIZE) != E_PS_OK)
    {
        DBG_ERR("ps_setSectionHead 1 failed\r\n");
        return E_PS_SYS;
    }

    return E_PS_OK;
}

ER ps_getPStoreStruct(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum)
{
    ER erReturn=0;

    DBG_IND("ulByteStart 0x%x  0x%x\r\n",ulByteStart,ulByteNum);

    //read Block0 (header1) first
    erReturn = ps_readBlockByBytes(pcBuf, 0, ulByteStart, ulByteNum);

    //if read Block0 header1 fail, read Block1 (header2)
    if(erReturn != E_PS_OK)
    {
        erReturn = ps_readBlockByBytes(pcBuf, PS_STRUCT_BLOCK, ulByteStart, ulByteNum);

        if(erReturn != E_PS_OK)
            DBG_ERR("ps_getHeadDesc failed\r\n");

    }
    return erReturn;
}

/**
    Set section data to header area.

    @param usSectionID    Section ID
    @param pcHeadBuf      Data buffer for store the written data

    @return               E_PS_OK if Success or E_PS_SYS if failure
*/
ER ps_setPStoreStruct(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum)
{
    DBG_IND("ulByteStart 0x%x  0x%x\r\n",ulByteStart,ulByteNum);

    if(ps_writeBlockByBytes(pcBuf, 0, ulByteStart, ulByteNum) != E_PS_OK)
    {
        DBG_ERR("ps_setHeadDesc 0 failed\r\n");
        return E_PS_SYS;
    }
    //Backup PStore header to another block
    if(ps_writeBlockByBytes(pcBuf, PS_STRUCT_BLOCK, ulByteStart, ulByteNum) != E_PS_OK)
    {
        DBG_ERR("ps_setHeadDesc 1 failed\r\n");
        return E_PS_SYS;
    }
    return E_PS_OK;
}

ER ps_getHeadDesc(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum)
{
    ER erReturn=0;

    DBG_IND("ulByteStart 0x%x  0x%x\r\n",ulByteStart,ulByteNum);
    if(!bSingleStructBlk)
    {
        //read Block0 (header1) first
        erReturn = ps_readBlockByBytes(pcBuf, 0, ulByteStart, ulByteNum);

        //if read Block0 header1 fail, read Block1 (header2)
        if(erReturn != E_PS_OK)
        {
            erReturn = ps_readBlockByBytes(pcBuf, PS_DESC_BLOCK, ulByteStart, ulByteNum);

            if(erReturn != E_PS_OK)
                DBG_ERR("ps_getHeadDesc failed\r\n");

        }
    }
    else
    {
        //read Block0 (header1) first
        erReturn = ps_readBlockByBytes(pcBuf, PS_STRUCT_BLOCK*2, 0, ulByteNum);

        //if read Block0 header1 fail, read Block1 (header2)
        if(erReturn != E_PS_OK)
        {
            erReturn = ps_readBlockByBytes(pcBuf, PS_DESC_BLOCK+PS_STRUCT_BLOCK, 0, ulByteNum);

            if(erReturn != E_PS_OK)
                DBG_ERR("ps_getHeadDesc failed\r\n");

        }

    }
    return erReturn;
}


/**
    Set section data to header area.

    @param usSectionID    Section ID
    @param pcHeadBuf      Data buffer for store the written data

    @return               E_PS_OK if Success or E_PS_SYS if failure
*/
ER ps_setHeadDesc(UINT8 *pcBuf, UINT32 ulByteStart, UINT32 ulByteNum)
{
    DBG_IND("ulByteStart 0x%x  0x%x\r\n",ulByteStart,ulByteNum);

    if(!bSingleStructBlk)
    {
        if(ps_writeBlockByBytes(pcBuf, 0, ulByteStart, ulByteNum) != E_PS_OK)
        {
            DBG_ERR("ps_setHeadDesc 0 failed\r\n");
            return E_PS_SYS;
        }

        //Backup PStore header to another block
        if(ps_writeBlockByBytes(pcBuf, PS_DESC_BLOCK, ulByteStart, ulByteNum) != E_PS_OK)
        {
            DBG_ERR("ps_setHeadDesc 1 failed\r\n");
            return E_PS_SYS;
        }
    }
    else
    {
        if(ps_writeBlockByBytes(pcBuf, PS_STRUCT_BLOCK*2, 0, ulByteNum) != E_PS_OK)
        {
            DBG_ERR("ps_setHeadDesc 0 failed\r\n");
            return E_PS_SYS;
        }

        //Backup PStore header to another block
        if(ps_writeBlockByBytes(pcBuf, PS_DESC_BLOCK+PS_STRUCT_BLOCK, 0, ulByteNum) != E_PS_OK)
        {
            DBG_ERR("ps_setHeadDesc 1 failed\r\n");
            return E_PS_SYS;
        }

    }
    return E_PS_OK;
}


/**
    Retrieve blocks for a useless area.

    @param usSectionID    Section ID
    @param uiStart        starting address of the useless area
    @param uiEnd          Termination address of the useless area.
    @return               TRUE if Success or FALSE if failure

    Note : 1. All the block in specific range will be released, except the block
              that isn't entire located in the range.
           2. The uiEnd will be corrected to maximum of addressing range
              when the uiEnd is exceed the maximum of addressing range of the section.

*/
ER ps_releaseSpace(UINT16 usSectionID, UINT32 uiStart, UINT32 uiEnd)
{
    UINT32 uiAddrIndex = 0, uilimit;
    UINT16 usLogicalBlk = 0;

    DBG_IND("SectionID %d 0x%x  0x%x\r\n",usSectionID,uiStart,uiEnd);

    if(uiStart >= uiEnd)
    {
        DBG_ERR("Wrong range\r\n");
        return E_PS_PAR;
    }

    // max section size
    if(uiEnd >= (PS_MAX_BLOCK_A_SECTION * g_StrgInfo.uiBlockSize) )
    {
        uilimit = (PS_MAX_BLOCK_A_SECTION * g_StrgInfo.uiBlockSize) - 1;
    }
    else
        uilimit = uiEnd;

    //Calculate the proper boundary of useless area.
    if(uiStart % g_StrgInfo.uiBlockSize)
        uiAddrIndex = ((uiStart / g_StrgInfo.uiBlockSize) + 1) * g_StrgInfo.uiBlockSize;
    else
        uiAddrIndex = uiStart;

    if(((uilimit + 1) % g_StrgInfo.uiBlockSize) != 0)
        uilimit = (uilimit / g_StrgInfo.uiBlockSize) * g_StrgInfo.uiBlockSize - 1;

    DBG_IND("uiAddrIndex 0x%x uilimit  0x%x\r\n",uiAddrIndex,uilimit);

    //If the release area is small than a block, then return true and doesn't update block table.
    if((uilimit - uiAddrIndex + 1) < g_StrgInfo.uiBlockSize)
        return E_PS_OK;

    //Add useless block to free block queue.
    for(; uiAddrIndex < uilimit ; uiAddrIndex += g_StrgInfo.uiBlockSize)
    {
        if(ps_getBlockStartAdd(usSectionID, uiAddrIndex, &usLogicalBlk) == FALSE)
             continue;

        if(usLogicalBlk < (UINT16)PS_EMPTY_BLOCK)
        {
            //Set block index to 0xFFFF
            ps_setBlockAddTbl(usSectionID, uiAddrIndex, PS_EMPTY_BLOCK);
            ps_setFreeLogicalBlock(usLogicalBlk);
        }
    }
    //return  ps_setSecTable(usSectionID);
    return E_PS_OK;
}

/**
    Get block mapping number by data address.

    @param usSectionID    Section ID
    @param DataAdd        Start address
    @param usLogicalAdd   Logical block address.
    @return               TRUE if Success or FALSE if failure
*/
static BOOL ps_getBlockStartAdd(UINT16 usSectionID, UINT32 DataAdd, UINT16 *usLogicalAdd)
{
    if(DataAdd > (PS_MAX_BLOCK_A_SECTION*g_StrgInfo.uiBlockSize) ) //addr can not larger than section max size
    {
        DBG_ERR("DataAdd 0x%x Out of range 0x%x\r\n", *usLogicalAdd,(PS_MAX_BLOCK_A_SECTION*g_StrgInfo.uiBlockSize));
        return FALSE;
    }

    ps_getBlockAddTbl(usSectionID,DataAdd,usLogicalAdd);

    DBG_IND("[RW]usLogicalAdd 0x%x  DataAdd  0x%x\r\n",*usLogicalAdd,DataAdd);

    if(*usLogicalAdd < guiDataStarBlock)  //Write data to head block is prohibited
    {
        DBG_ERR("Blk 0x%x is header area\r\n",*usLogicalAdd);
        return FALSE;
    }

    if((*usLogicalAdd > g_StrgInfo.uiLogicalBlockCount) && (*usLogicalAdd != PS_EMPTY_BLOCK))  //Writting data to reserved block is prohibited
    {
        DBG_ERR("Blk 0x%x Out of range 0x%x\r\n", *usLogicalAdd,g_StrgInfo.uiLogicalBlockCount);

        return FALSE;
    }

    return TRUE;
}

/**
    Prepare the block table for the section
    @param usSectionID    Section ID

    @return               E_PS_OK if Success or E_PS_SYS if failure
*/
ER ps_getSecTable(UINT16 usSectionID)
{
    static UINT8 SwapIndex = 0;
    UINT16 usTableID;

    DBG_IND("getSectionHead of SectionID %d\r\n",usSectionID);

    usTableID = ps_getSecTableID(usSectionID);

    if(usTableID == PS_TABLE_NOT_FOUND)  //The section's block table has not been prepared yet.
    {
        if(ps_getSectionHead(usSectionID, (UINT8 *)(gucSecTable+SwapIndex*PS_SECTION_HEADER_SIZE) ) == E_PS_OK)
        {
            gusSecTableIndex[SwapIndex] = usSectionID;
            SwapIndex++;
            if(SwapIndex >= PS_NUM_BLOCK_TABLE)
                SwapIndex = 0;

        }
        else
        {
            DBG_ERR("Get header fail\r\n");
            return E_PS_SYS;
        }
    }
    return E_PS_OK;
}


/**
    Update the block address table that stored in the NAND flash.

    @param usSectionID    Section ID
    @return               E_PS_OK if Success or E_PS_SYS if failure

*/
ER ps_setSecTable(UINT16 usSectionID,BOOL bCheck)
{
    UINT32 ucLogicalBlkAdd,ucLogicalBlkAdd1,ucLogicalBlkAdd2;
    UINT32 ulDataAddress;
    UINT16 usTableID;

    DBG_IND("ps_setSecTable of SectionID %d\r\n",usSectionID);

    usTableID = ps_getSecTableID(usSectionID);

    if(usTableID == PS_TABLE_NOT_FOUND)
    {
        DBG_ERR("Section doesn't exist\r\n");
        return PS_TABLE_NOT_FOUND;
    }

    if(PS_SECTION_HEADER_SIZE<=g_StrgInfo.uiBlockSize)
    {
        ucLogicalBlkAdd = usSectionID /(g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE);
        ulDataAddress = (usSectionID % (g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE)) * PS_SECTION_HEADER_SIZE;
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + guiHead1StarBlock;
    }
    else
    {
        ucLogicalBlkAdd = (usSectionID*PS_SECTION_HEADER_SIZE)/g_StrgInfo.uiBlockSize;
        ulDataAddress =  (usSectionID*PS_SECTION_HEADER_SIZE)%g_StrgInfo.uiBlockSize;
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + guiHead1StarBlock;
    }

    DBG_IND("[RW]usSectionID %d ucLogicalBlkAdd 0x%x\r\n,",usSectionID,ucLogicalBlkAdd1,ulDataAddress);

    if(bCheck)
    {
        DBG_IND("first block 0x%x\r\n,",(*(UINT16 *)(gucSecTable+usTableID*PS_SECTION_HEADER_SIZE)));
        if((*(UINT16 *)(gucSecTable+usTableID*PS_SECTION_HEADER_SIZE))==PS_EMPTY_BLOCK)
        {
            DBG_ERR("first block is 0xFFFF\r\n");
            return E_PS_SYS;
        }
    }

    if(ps_writeBlockByBytes((UINT8 *)(gucSecTable+usTableID*PS_SECTION_HEADER_SIZE), ucLogicalBlkAdd1, ulDataAddress, PS_SECTION_HEADER_SIZE) != E_PS_OK)
    {
        DBG_ERR("ps_setSectionHead 0 failed\r\n");
        return E_PS_SYS;
    }

    //Backup PStore header to another block
    ucLogicalBlkAdd2 = ucLogicalBlkAdd + guiHead2StarBlock;
    if(ps_writeBlockByBytes((UINT8 *)(gucSecTable+usTableID*PS_SECTION_HEADER_SIZE), ucLogicalBlkAdd2, ulDataAddress, PS_SECTION_HEADER_SIZE) != E_PS_OK)
    {
        DBG_ERR("ps_setSectionHead 1 failed\r\n");
        return E_PS_SYS;
    }
    return E_PS_OK;
}
ER ps_clearAllSecTable(UINT8 *pBuf,PSFMT *pFmtStruct)
{
    INT32 erReturn;
    UINT32 headBlk = 0;
    UINT32 i = 0;
    if((pFmtStruct->uiMaxBlkPerSec*pFmtStruct->uiMaxSec)%ps_getBlkSize())
        headBlk= (pFmtStruct->uiMaxBlkPerSec*PS_BYTES_A_BLOCK_ADDR*pFmtStruct->uiMaxSec)/ps_getBlkSize() +1;
    else
        headBlk= (pFmtStruct->uiMaxBlkPerSec*PS_BYTES_A_BLOCK_ADDR*pFmtStruct->uiMaxSec)/ps_getBlkSize();

    //clean the section table
    memset(pBuf, 0xFF,headBlk*ps_getBlkSize() );


    for(i=0;i<PS_NUM_BLOCK_TABLE;i++)
    {
        erReturn = ps_writeBlockSeq(pBuf,PS_DESC_BLOCK*PS_HEADER_NUM+i*headBlk ,headBlk);
        if(erReturn!=E_PS_OK)
            break;
    }
    return erReturn;
}

/**
    Update the block address table that cached in RAM.

    @param usSectionID    Section ID
    @param DataAddr       Start address of the written data
    @param usBlockAddr    Block address of the data written to.

*/
static void ps_setBlockAddTbl(UINT16 usSectionID, UINT32 DataAdd, UINT16 usLogicalAdd)
{
    UINT32 ulBlockAdd;
    UINT16 usTableID;
    UINT8   *pcTable;

    usTableID = ps_getSecTableID(usSectionID);
	//Fix openA;open/close B,C,D;read/wrtie A, no A in preload SecTable
	if(usTableID == PS_TABLE_NOT_FOUND){
        //The section's block table in not in DRAM.load from flash again
        if(ps_getSecTable(usSectionID) == E_PS_OK) {
            usTableID = ps_getSecTableID(usSectionID);
        } else {
            DBG_ERR("no section %d table\r\n",usSectionID);
            return;
        }
    }    
    ulBlockAdd = (DataAdd / g_StrgInfo.uiBlockSize) * PS_BYTES_A_BLOCK_ADDR;

    pcTable = (gucSecTable+usTableID*PS_SECTION_HEADER_SIZE);

    *(pcTable + ulBlockAdd + 1)  = (usLogicalAdd & 0xFF00) >> 8;
    *(pcTable + ulBlockAdd) = (usLogicalAdd & 0xFF);
}

static void ps_getBlockAddTbl(UINT16 usSectionID, UINT32 DataAdd, UINT16 *usLogicalAdd)
{
    UINT32 ulBlockAdd;
    UINT16 usTableID;
    UINT8   *pcTable;

    usTableID = ps_getSecTableID(usSectionID);
	//Fix openA;open/close B,C,D;read/wrtie A, no A in preload SecTable
	if(usTableID == PS_TABLE_NOT_FOUND){
        //The section's block table in not in DRAM.load from flash again
        if(ps_getSecTable(usSectionID) == E_PS_OK) {
            usTableID = ps_getSecTableID(usSectionID);
        } else {
            *usLogicalAdd = PS_EMPTY_BLOCK;
            DBG_ERR("no section %d table\r\n",usSectionID);
            return;
        }
    }
    ulBlockAdd = (DataAdd / g_StrgInfo.uiBlockSize) * PS_BYTES_A_BLOCK_ADDR;

    pcTable = (gucSecTable+usTableID*PS_SECTION_HEADER_SIZE);

    *usLogicalAdd = *(pcTable + ulBlockAdd) | (*(pcTable + ulBlockAdd + 1) << 8);
}
/**
    Get the block table ID by section ID

    @param usSectionID    Section ID

    @return               Table ID, 0xFFFF means table ID not found.
*/
static UINT16 ps_getSecTableID(UINT16 usSectionID)
{
    UINT16 usTableID;
    for(usTableID = 0 ; usTableID < PS_NUM_BLOCK_TABLE ; usTableID++)
    {
        if(usSectionID == gusSecTableIndex[usTableID])
            return usTableID;
    }
    return PS_TABLE_NOT_FOUND;
}

#if 0
#pragma mark -
#endif
/**
    Parse free block list.

    return          TRUE if success, FALSE if no more free block.

    note : 0 for a free block 1 for a used block
*/
static BOOL ps_getLevel1Index(UINT32 uiData, UINT8* ucBlockIndex)
{
    static UINT8 ucLoop = 0;

    for(; ucLoop < 32 ; ucLoop++)
    {
        if((uiData & (1 << ucLoop)) == 0)
        {
            *ucBlockIndex = ucLoop;
            return TRUE;
        }
    }

    for(ucLoop = 0; ucLoop < 32; ucLoop++)
    {
        if((uiData & (1 << ucLoop)) == 0)
        {
            *ucBlockIndex = ucLoop;
            return TRUE;
        }
    }

    return FALSE;
}

/**
    Add a used block to free block list.

    @param usBlockNum           logical block number
    @return                     E_PS_OK if Success or E_PS_SYS if failure
*/
static ER ps_addUsedLogicalBlock(UINT16 BlockNum)
{
    UINT8  ucBlockBit;
    UINT8  ucBlockGroupIndex;

    ucBlockGroupIndex = BlockNum >> 5;
    ucBlockBit  = BlockNum % 32;

    if((guiLevelOneMapping[ucBlockGroupIndex] & (0x1 << ucBlockBit)) == 0x0)
    {
        guiLevelOneMapping[ucBlockGroupIndex] = guiLevelOneMapping[ucBlockGroupIndex] | (0x1 << ucBlockBit);
        guiUsedBlkNum++;
        return E_PS_OK;
    }
    else
    {
        DBG_ERR("blk %03d duplicate used!\n\r", BlockNum);
        return E_PS_SYS;
    }
}

/**
    Add a free block to free block list.

    @param usBlockNum           logical block number
    @return                     E_PS_OK if Success or E_PS_SYS if failure
*/
static BOOL ps_setFreeLogicalBlock(UINT16 BlockNum)
{
    UINT8  ucBlockBit;
    UINT8  ucBlockGroupIndex;

    ucBlockGroupIndex = BlockNum >> 5;
    ucBlockBit  = BlockNum % 32;

    guiLevelOneMapping[ucBlockGroupIndex] = guiLevelOneMapping[ucBlockGroupIndex] &  ~(0x1 << ucBlockBit);
    guiUsedBlkNum--;
    return E_PS_OK;
}

/**
    Get a free block from free block list.

    @param usBlockNum           return logical block number
    @return                     E_PS_OK if Success or E_PS_SYS if failure
*/
static ER ps_getFreeLogicalBlock(UINT16 *usBlockNum)
{
    static UINT8  ucBlockGroupIndex = 0;
    UINT8  ucBlockIndex =0 ;

    //search from the previous position (ucBlockGroupIndex)
    for(; ucBlockGroupIndex < PS_LEVEL1_SIZE ; ucBlockGroupIndex++)
    {
        if(guiLevelOneMapping[ucBlockGroupIndex] != 0xFFFFFFFF)
        {
            ps_getLevel1Index(guiLevelOneMapping[ucBlockGroupIndex], &ucBlockIndex);
            *usBlockNum = (ucBlockGroupIndex << 5) + ucBlockIndex;//ucBlockGroupIndex*32 + ucBlockIndex
            guiLevelOneMapping[ucBlockGroupIndex] = guiLevelOneMapping[ucBlockGroupIndex] | (0x1 << ucBlockIndex);
            guiUsedBlkNum++;

            DBG_IND("^Y getFreeBlock %d\r\n",*usBlockNum);
            return E_PS_OK;
        }
    }

    //search from the head,ucBlockGroupIndex is static variable
    // coverity[result_independent_of_operands]
    for(ucBlockGroupIndex = 0; ucBlockGroupIndex < PS_LEVEL1_SIZE ; ucBlockGroupIndex++)
    {
        if(guiLevelOneMapping[ucBlockGroupIndex] != 0xFFFFFFFF)
        {
            ps_getLevel1Index(guiLevelOneMapping[ucBlockGroupIndex], &ucBlockIndex);
            *usBlockNum = (ucBlockGroupIndex << 5) + ucBlockIndex;
            guiLevelOneMapping[ucBlockGroupIndex] = guiLevelOneMapping[ucBlockGroupIndex] | (0x1 << ucBlockIndex);
            guiUsedBlkNum++;

            DBG_IND("^Y getFreeBlock %d\r\n",*usBlockNum);

            return E_PS_OK;
        }
    }

    DBG_ERR("No more free block\r\n");
    return E_PS_SYS;
}
/**
    Create free blocks table

    @return                     E_PS_OK if Success or E_PS_SYS if failure
*/
ER ps_initBlockTable(void)
{

    UINT32 uiSectionNum =0 ,k;
    UINT16 uiLogicNum_l,uiLogicNum_h,uiLogicNum;
    UINT32 i=0;
    UINT16 usTemp;


#if (PS_PERFORMANCE==ENABLE)
    //Perf_Open();
    Perf_Mark();
#endif

    //Calculate the PStore header(block table) size(in block unit)
    guiHeadBlock = (PS_MAX_SECTION * PS_SECTION_HEADER_SIZE) / g_StrgInfo.uiBlockSize;
    if((PS_MAX_SECTION * PS_SECTION_HEADER_SIZE) % g_StrgInfo.uiBlockSize)
        guiHeadBlock++;

    guiHead1StarBlock = PS_DESC_BLOCK*PS_HEADER_NUM;                //BlockTable1 starting position
    guiHead2StarBlock = guiHead1StarBlock+guiHeadBlock;             //BlockTable2 starting position
    guiDataStarBlock =  (PS_DESC_BLOCK+guiHeadBlock)*PS_HEADER_NUM; //guiHead2StarBlock + guiHeadBlock;

    DBG_IND("guiHeadBlock  %d guiDataStarBlock %d guiHead1StarBlock %d guiHead2StarBlock %d\r\n",guiHeadBlock,guiDataStarBlock,guiHead1StarBlock,guiHead2StarBlock);

    for(usTemp = 0 ; usTemp < PS_NUM_BLOCK_TABLE ; usTemp++)
    {
        gusSecTableIndex[usTemp] = 0xFFFF;
    }

    guiUsedBlkNum = 0;

    memset(&(guiLevelOneMapping[0]), 0x0, sizeof(guiLevelOneMapping));

    //set Header and Block Table as used
    for(i=0;i< guiDataStarBlock ;i++)
    {
        if(ps_addUsedLogicalBlock(i)==E_PS_SYS)
        {
            return E_PS_TBL;
        }
    }

    //for loop to search the whole BlockTable1 by block
    for(uiSectionNum = 0 ;uiSectionNum< PS_MAX_SECTION;uiSectionNum++)
    {
        if(ps_getSectionHead(uiSectionNum, (UINT8 *)gucSecTable) != E_PS_OK)
        {
            DBG_ERR("Read uiSectionNum head %03d error !\n\r", uiSectionNum);
            return E_PS_TBL;
        }
        //search the section block tables of the current block
        {
            //get each block logic number of the current section, and add to used mapping
            for(k=0;k<PS_MAX_BLOCK_A_SECTION;k++)
            {
                uiLogicNum_l = gucSecTable[(k*2)];
                uiLogicNum_h = gucSecTable[(k*2)+1];
                uiLogicNum = (uiLogicNum_l) | (uiLogicNum_h<<8);
                if(uiLogicNum != 0xFFFF )
                {
                    if(ps_addUsedLogicalBlock(uiLogicNum)==E_PS_SYS)
                    {
                        DBG_ERR("section %d addr %d\r\n",uiSectionNum,k);
                        return E_PS_TBL;
                    }
                }
                else
                {
                    break;
                }

            }
        }
    }

    //if the logical block count are smaller than PStore max block count, set unuseable block as 1
    if(g_StrgInfo.uiLogicalBlockCount < (PS_LEVEL1_SIZE * 32))
    {
        if(PS_LEVEL1_SIZE - (g_StrgInfo.uiLogicalBlockCount / 32) > 1)
            // coverity[overrun-local]
            memset(&guiLevelOneMapping[(g_StrgInfo.uiLogicalBlockCount / 32) + 1], 0xFF, (PS_LEVEL1_SIZE - ((g_StrgInfo.uiLogicalBlockCount / 32) + 1)) * 4);

        guiLevelOneMapping[g_StrgInfo.uiLogicalBlockCount / 32] |= 0xFFFFFFFF << (g_StrgInfo.uiLogicalBlockCount % 32);
    }

#if (PS_PERFORMANCE==ENABLE)
    DBG_ERR("time %d \r\n",Perf_GetDuration());
#endif
    return E_PS_OK;

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ER ps_checkSingleSectionByHeader(UINT8 * pcHeadBuf,UINT32 Start_Block,UINT32 usSectionID)
{
    UINT16 BlockNum ;

    UINT32 ucLogicalBlkAdd,ucLogicalBlkAdd1;
    UINT32 ulDataAddress;
    ER erReturn;

    if(PS_SECTION_HEADER_SIZE<=g_StrgInfo.uiBlockSize)
    {
        ucLogicalBlkAdd = usSectionID /(g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE);
        ulDataAddress = (usSectionID % (g_StrgInfo.uiBlockSize / PS_SECTION_HEADER_SIZE)) * PS_SECTION_HEADER_SIZE;
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + Start_Block;//guiHead1StarBlock;
    }
    else
    {
        ucLogicalBlkAdd = (usSectionID*PS_SECTION_HEADER_SIZE)/g_StrgInfo.uiBlockSize;
        ulDataAddress =  (usSectionID*PS_SECTION_HEADER_SIZE)%g_StrgInfo.uiBlockSize;
        ucLogicalBlkAdd1 = ucLogicalBlkAdd + Start_Block;//guiHead1StarBlock;
    }
    erReturn = ps_readBlockByBytes(pcHeadBuf, ucLogicalBlkAdd1, ulDataAddress, PS_SECTION_HEADER_SIZE);

    DBG_IND("[RW]usSectionID %d ucLogicalBlkAdd 0x%x \r\n,",usSectionID,ucLogicalBlkAdd1,ulDataAddress);


    if(erReturn != E_PS_OK)
    {
        DBG_ERR("ps_checkSingleSectionByHeader block %d failed\r\n",usSectionID);
        return erReturn;
    }

    //DBG_ERR("SectionIndex = %d TempSectionIndex = %d BlockNumIndex = %d \r\n",SectionIndex,TempSectionIndex,BlockNumIndex);

    BlockNum = *(UINT16 *) (pcHeadBuf);
    //DBG_ERR("BlockNum = 0x%x Addr 0x%x 0x%x \r\n",BlockNum ,pcHeadBuf,pcHeadBuf + TempSectionIndex*SectBlockArea);

    if( BlockNum == PS_EMPTY_BLOCK || ( BlockNum<guiDataStarBlock ) ) // Empty block
    {
        DBG_ERR("Section[%d] : BlockNum = 0x%X \r\n",usSectionID ,BlockNum);
        return E_PS_SYS ;
    }


    return erReturn;
}

//Pstore check single section logical block number
/**
    check nand Pstore single seciton logical block number is empty or not
    @param SectionIndex        Pstore secton Index
    @param ErrorSection        where return Section Error
    @return             E_PS_OK or E_PS_SYS
*/
ER ps_checkSingleSection(UINT8 * pcHeadBuf,UINT32 SectionIndex,UINT32  * ErrorSection)
{
    ER erReturn1 = E_PS_OK;
    ER erReturn2 = E_PS_OK;

    UINT32 Section_Err = 0;

    erReturn1 = ps_checkSingleSectionByHeader(pcHeadBuf,guiHead1StarBlock,SectionIndex);
    if(erReturn1!=E_PS_OK)
    {
        DBG_ERR(" Check 1st Header Fail \r\n");
        Section_Err |= PS_HEAD1;
    }

    erReturn2 = ps_checkSingleSectionByHeader(pcHeadBuf,guiHead2StarBlock,SectionIndex);
    if(erReturn2!=E_PS_OK)
    {
        DBG_ERR(" Check 2nd Header Fail \r\n");
        Section_Err |= PS_HEAD2;
    }

    *(UINT32  * )ErrorSection = Section_Err;

    if( (erReturn1==E_PS_OK) && (erReturn2==E_PS_OK) )
        return E_PS_OK;
    else
        return E_PS_SYS;
}

/**
    Enable update header
    @param Header       enbale Update Header type,all/one/none
    @return             E_PS_OK
*/
ER ps_enableUpdateHeader(PS_HEAD_UPDATE Header)
{
    guiPStoreUpdateHead = Header;
    return E_PS_OK;
}
/**
    Restore Pstore Header
    @param pBuf         external buffer for read from normal header and write to bad haeader
    @param ReStoreType  ReStore Header1 or Header2
    @return             E_PS_OK or E_PS_SYS
*/
ER ps_restoreHeader(UINT8 * pBuf , UINT32 ReStoreType)
{
    UINT8  ucNumBlock;
    ER     erReturn = E_PS_OK;
    UINT8 * pcBuf = pBuf;
    UINT32 UpdateHeadState = guiPStoreUpdateHead; //keep org stat

    if(ReStoreType > PS_UPDATE_ZEROHEAD || (ReStoreType==0) )
        return E_PS_SYS;

    guiPStoreUpdateHead = PS_UPDATE_ALLHEAD;

    if(!bSingleStructBlk)
    {
    if (ReStoreType == PS_HEAD1)
    {
        //restore descriptor (2=>1)
        {
            erReturn = ps_readBlockByBytes(pcBuf, PS_DESC_BLOCK,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
            erReturn = ps_writeBlockByBytes(pcBuf, 0,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
        }

        //restore block table (2=>1)
        for(ucNumBlock = 0 ;ucNumBlock< guiHeadBlock ; ucNumBlock ++)
        {
            DBG_WRN("Restore PS_HEAD 1  ucNumBlock %d \r\n",ucNumBlock);

            erReturn = ps_readBlockByBytes(pcBuf, guiHead2StarBlock+ucNumBlock,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
            erReturn = ps_writeBlockByBytes(pcBuf, guiHead1StarBlock+ucNumBlock,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
        }
    }
    else
    {
        //restore descriptor (1=>2)
        {
            erReturn = ps_readBlockByBytes(pcBuf, 0,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
            erReturn = ps_writeBlockByBytes(pcBuf, PS_DESC_BLOCK,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
        }

        //restore block table (1=>2)
        for(ucNumBlock = 0 ;ucNumBlock< guiHeadBlock ; ucNumBlock ++)
        {
            DBG_WRN("Restore PS_HEAD 2 ucNumBlock %d \r\n",ucNumBlock);

            erReturn = ps_readBlockByBytes(pcBuf, guiHead1StarBlock+ucNumBlock,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
            erReturn = ps_writeBlockByBytes(pcBuf, guiHead2StarBlock+ucNumBlock,  0, g_StrgInfo.uiBlockSize);
            if(erReturn != E_PS_OK)
            {
                guiPStoreUpdateHead = UpdateHeadState;
                return erReturn;
            }
        }
    }
    }
    //restore org stat
    guiPStoreUpdateHead = UpdateHeadState;
    return erReturn;
}



void ps_printInfo(PSTRG_INFO *StrgInfo)
{

    DBG_IND("uiBlockSize 0x%x\r\n", StrgInfo->uiBlockSize);
    DBG_IND("uiSectorPerBlock 0x%x\r\n", StrgInfo->uiSectorPerBlock);
    DBG_IND("uiLogicalBlockCount 0x%x\r\n", StrgInfo->uiLogicalBlockCount);

}

INT32 ps_getStrgInfo(STORAGE_OBJ *pStrgObj,PSTRG_INFO *StrgInfo)
{
    UINT32 uiBlkSize=0;
    STRG_CAP Cap = {0};

    if(pStrgObj->GetParam(STRG_GET_BEST_ACCESS_SIZE, (UINT32)&uiBlkSize, 0) == E_PS_OK)
    {
        StrgInfo->uiBlockSize = uiBlkSize;
    }
    else
    {
        DBG_ERR("STRG_GET_SECTOR_SIZE fail\r\n");
        return E_PS_SYS;
    }
    if(pStrgObj->GetParam(STRG_GET_CAP_TAB, (UINT32)&Cap, 0) == E_PS_OK)
    {
        StrgInfo->uiLogicalBlockCount = Cap.uiTotalSectors;
        if(Cap.uiBytesPerSector)
        {
            StrgInfo->uiSectorPerBlock = StrgInfo->uiBlockSize/Cap.uiBytesPerSector;
        }
        else
        {
            DBG_ERR("uiBytesPerSector 0\r\n");
            return E_PS_SYS;
        }
    }
    else
    {
        DBG_ERR("STRG_GET_CAP_TAB fail\r\n");
        return E_PS_SYS;
    }

    ps_printInfo(StrgInfo);
    return E_PS_OK;
}


