#include "SysKer.h"
#include "FwSrvInt.h"
#include "SxCmd.h"
#include "PartitionInfo.h"
#include "DxApi.h"
#include "DxStorage.h"
#include "StrgDef.h"
#include "BinInfo.h"
#include "BinaryFormat.h"
#include "FileSysTsk.h"
#include "GxDSP.h"
#include "GxeCosApi.h"

static EMB_PARTITION* xFwSrvSxCmd_FindPartition(unsigned short EmbType)
{
    UINT32 i;
    EMB_PARTITION* pPat = (EMB_PARTITION*)(OS_GetMemAddr(MEM_TOTAL) + EMB_PARTITION_INFO_OFFSET);

    for(i=0; i<EMB_PARTITION_INFO_COUNT; i++)
    {
        if(pPat->EmbType == EmbType)
        {
            return pPat;
        }
		pPat++;
    }
    return NULL;
}

static BOOL xFwSrvSxCmd_Dump_uITRON(CHAR* strCmd)
{
    UINT32 uiBlkIndex,uiBlkCnt,uiBlkSize,uiBlockLd,uiDataSize,uiBinSize;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    DX_HANDLE hDx = Dx_GetObject(DX_CLASS_STORAGE_EXT | pCtrl->Init.DxMap.uITRON);
    STORAGE_OBJ* pStrg = (STORAGE_OBJ*)Dx_Getcaps(hDx, STORAGE_CAPS_HANDLE, 0);

    EMB_PARTITION* pPat = xFwSrvSxCmd_FindPartition(EMBTYPE_UITRON);

    if(pPat == NULL)
    {
        DBG_ERR("cannot find uITRON partition.\r\n");
        return TRUE;
    }

    pStrg->Lock();
    pStrg->GetParam(STRG_GET_SECTOR_SIZE, (UINT32)&uiBlkSize, 0);
    pStrg->GetParam(STRG_GET_LOADER_SECTOR_COUNT, (UINT32)&uiBlockLd, 0);
    pStrg->Unlock();

	//for emmc, partition table size unit is block unit.
	UINT32 partition_scale = (uiBlkSize < 0x1000) ? uiBlkSize : 1;
    uiBlkIndex = uiBlockLd;
    uiDataSize = pPat->PartitionSize*partition_scale - uiBlockLd*uiBlkSize;

    UINT32 uiMem = SxCmd_GetTempMem(uiDataSize);
    if(uiMem == 0)
    {
        DBG_ERR("no temp buffer to dump \r\n");
        return TRUE;
    }

    //read one block to get bin size
    pStrg->Lock();
	pStrg->Open();
    if(pStrg->RdSectors((INT8*)uiMem,uiBlkIndex,1)!=E_OK)
    {
        DBG_ERR("read sector failed.\r\n");
		pStrg->Close();
        pStrg->Unlock();
        return TRUE;
    }
    pStrg->Unlock();

    HEADER_BFC* pBfc = (HEADER_BFC*)uiMem;
    BININFO* pBinInfo = (BININFO*)uiMem;
    if(pBfc->uiFourCC == MAKEFOURCC('B','C','L','1'))
    {
        uiBinSize = UINT32_SWAP(pBfc->uiSizeComp)+sizeof(HEADER_BFC);
    }
    else
    {
        uiBinSize = pBinInfo->head.BinLength;
    }

    if(uiBinSize > uiDataSize)
    {
        DBG_ERR("error BinSize info. cannot dump\r\n");
        return TRUE;
    }

    uiBlkCnt = ALIGN_CEIL(uiBinSize,uiBlkSize)/uiBlkSize;

    //read whole bin
    pStrg->Lock();
	pStrg->Open();
    if(pStrg->RdSectors((INT8*)uiMem,uiBlkIndex,uiBlkCnt)!=E_OK)
    {
        DBG_ERR("read sector failed.\r\n");
        pStrg->Unlock();
        return TRUE;
    }
	pStrg->Close();
    pStrg->Unlock();

    FST_FILE hFile = FileSys_OpenFile("A:\\UITRON.BIN", FST_OPEN_ALWAYS|FST_OPEN_WRITE);
    if(hFile == NULL)
    {
        DBG_ERR("cannot open file.\r\n");
        return TRUE;
    }
    FileSys_WriteFile(hFile, (UINT8*)uiMem,&uiBinSize,0, NULL);
    FileSys_CloseFile(hFile);

    return TRUE;
}

static BOOL xFwSrvSxCmd_Dump_eCos(CHAR* strCmd)
{
    UINT32 uiBlkCnt,uiBlkSize,uiDataSize,uiBinSize;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    DX_HANDLE hDx = Dx_GetObject(DX_CLASS_STORAGE_EXT | pCtrl->Init.DxMap.eCos);
    STORAGE_OBJ* pStrg = (STORAGE_OBJ*)Dx_Getcaps(hDx, STORAGE_CAPS_HANDLE, 0);

    EMB_PARTITION* pPat = xFwSrvSxCmd_FindPartition(EMBTYPE_ECOS);

    if(pPat == NULL)
    {
        DBG_ERR("cannot find eCos partition.\r\n");
        return TRUE;
    }

    pStrg->Lock();
    pStrg->GetParam(STRG_GET_SECTOR_SIZE, (UINT32)&uiBlkSize, 0);
    pStrg->Unlock();

	//for emmc, partition table size unit is block unit.
	UINT32 partition_scale = (uiBlkSize < 0x1000) ? uiBlkSize : 1;
    uiDataSize = pPat->PartitionSize*partition_scale;
    uiBlkCnt = uiDataSize/uiBlkSize;

    UINT32 uiMem = SxCmd_GetTempMem(uiDataSize);
    if(uiMem == 0)
    {
        DBG_ERR("no temp buffer to dump \r\n");
        return TRUE;
    }

    //read one block to get bin size
    UINT32 uiPreLoadBlkCnt = ALIGN_CEIL(GXECOS_PRELOAD_ECOS_SIZE,uiBlkSize)/uiBlkSize;
    pStrg->Lock();
    pStrg->Open();
    if(pStrg->RdSectors((INT8*)uiMem,0,uiPreLoadBlkCnt)!=E_OK)
    {
        DBG_ERR("read sector failed.\r\n");
        pStrg->Close();
        pStrg->Unlock();
        return TRUE;
    }
    pStrg->Close();
    pStrg->Unlock();

    HEADER_BFC* pBfc = (HEADER_BFC*)uiMem;
    BININFO* pBinInfo = (BININFO*)(uiMem+BIN_INFO_OFFSET_ECOS);
    if(pBfc->uiFourCC == MAKEFOURCC('B','C','L','1'))
    {
        uiBinSize = UINT32_SWAP(pBfc->uiSizeComp)+sizeof(HEADER_BFC);
    }
    else
    {
        uiBinSize = pBinInfo->head.BinLength;
    }

    if(uiBinSize > uiDataSize)
    {
        DBG_ERR("error BinSize info. cannot dump\r\n");
        return TRUE;
    }

    uiBlkCnt = ALIGN_CEIL(uiBinSize,uiBlkSize)/uiBlkSize;

    //read whole bin
    pStrg->Lock();
    pStrg->Open();
    if(pStrg->RdSectors((INT8*)uiMem,0,uiBlkCnt)!=E_OK)
    {
        DBG_ERR("read sector failed.\r\n");
        pStrg->Close();
        pStrg->Unlock();
        return TRUE;
    }
    pStrg->Close();
    pStrg->Unlock();

    FST_FILE hFile = FileSys_OpenFile("A:\\ECOS.BIN", FST_OPEN_ALWAYS|FST_OPEN_WRITE);
    if(hFile == NULL)
    {
        DBG_ERR("cannot open file.\r\n");
        return TRUE;
    }
    FileSys_WriteFile(hFile, (UINT8*)uiMem,&uiBinSize,0, NULL);
    FileSys_CloseFile(hFile);
    return TRUE;
}

static BOOL xFwSrvSxCmd_Dump_Dsp(CHAR* strCmd)
{
    UINT32 uiBlkCnt,uiBlkSize,uiDataSize,uiBinSize;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    DX_HANDLE hDx = Dx_GetObject(DX_CLASS_STORAGE_EXT | pCtrl->Init.DxMap.DSP);
    STORAGE_OBJ* pStrg = (STORAGE_OBJ*)Dx_Getcaps(hDx, STORAGE_CAPS_HANDLE, 0);

    EMB_PARTITION* pPat = xFwSrvSxCmd_FindPartition(EMBTYPE_ECOS);

    if(pPat == NULL)
    {
        DBG_ERR("cannot find eCos partition.\r\n");
        return TRUE;
    }

    pStrg->Lock();
    pStrg->GetParam(STRG_GET_SECTOR_SIZE, (UINT32)&uiBlkSize, 0);
    pStrg->Unlock();

	//for emmc, partition table size unit is block unit.
	UINT32 partition_scale = (uiBlkSize < 0x1000) ? uiBlkSize : 1;
    uiDataSize = pPat->PartitionSize*partition_scale;
    uiBlkCnt = uiDataSize/uiBlkSize;

    UINT32 uiMem = SxCmd_GetTempMem(uiDataSize);
    if(uiMem == 0)
    {
        DBG_ERR("no temp buffer to dump \r\n");
        return TRUE;
    }

    //read one block to get bin size
    pStrg->Lock();
    pStrg->Open();
    if(pStrg->RdSectors((INT8*)uiMem,0,1)!=E_OK)
    {
        DBG_ERR("read sector failed.\r\n");
        pStrg->Close();
        pStrg->Unlock();
        return TRUE;
    }
    pStrg->Close();
    pStrg->Unlock();


    DSP_FW_HEADER* pH = (DSP_FW_HEADER*)uiMem;

    //check fourcc
    if(pH->uiFourCC!=MAKEFOURCC('_','D','S','P'))
    {
        DBG_ERR("error fourcc.\r\n");
        return TRUE;
    }
    //check size
    if(pH->uiSize!=sizeof(DSP_FW_HEADER))
    {
        DBG_ERR("dsp header size context(%d),uITRON(%d)\r\n",pH->uiSize,sizeof(DSP_FW_HEADER));;
        return TRUE;
    }

    uiBinSize = pH->TotalSize;
    uiBlkCnt = ALIGN_CEIL(uiBinSize,uiBlkSize)/uiBlkSize;

    //read whole bin
    pStrg->Lock();
    pStrg->Open();
    if(pStrg->RdSectors((INT8*)uiMem,0,uiBlkCnt)!=E_OK)
    {
        DBG_ERR("read sector failed.\r\n");
        pStrg->Close();
        pStrg->Unlock();
        return TRUE;
    }
    pStrg->Close();
    pStrg->Unlock();

    FST_FILE hFile = FileSys_OpenFile("A:\\DSP.BIN", FST_OPEN_ALWAYS|FST_OPEN_WRITE);
    if(hFile == NULL)
    {
        DBG_ERR("cannot open file.\r\n");
        return TRUE;
    }
    FileSys_WriteFile(hFile, (UINT8*)uiMem,&uiBinSize,0, NULL);
    FileSys_CloseFile(hFile);
    return TRUE;
}
static SXCMD_BEGIN(fwsrv,"FwSrv")
SXCMD_ITEM("dumpuitron", xFwSrvSxCmd_Dump_uITRON,"Dump uITRON from EMB")
SXCMD_ITEM("dumpecos", xFwSrvSxCmd_Dump_eCos,"Dump eCos from EMB")
SXCMD_ITEM("dumpdsp", xFwSrvSxCmd_Dump_Dsp,"Dump DSP from EMB")
SXCMD_END()

void xFwSrv_InstallCmd(void)
{
    SxCmd_AddTable(fwsrv);
}

