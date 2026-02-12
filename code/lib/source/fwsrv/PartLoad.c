#include "FwSrvInt.h"
#include "PartLoadID.h"
#include "PartLoad_Protected.h"
#include <kwrap/util.h>

#define CFG_PARTLAD_SECTION_COUNTS  10
#define CFG_PARTLAD_INIT_KEY        MAKEFOURCC('P','A','L','D') ///< a key value 'P','A','R','L' for indicating system initial.
#define CFG_GZ_WORK_SIZE 0x10000 //64KB are enough
#define CFG_GZ_BLK_SIZE 0x8000 //ecah gz uncompress interval size
#define CFG_DEBUG_PIPE_UNCOMPRESS DISABLE
#define CFG_DEBUG_PIPE_UNCOMPRESS2 DISABLE //run with pipe, but the notification occurs after all uncompressed

/**
    Flag Pattern
*/
//@{
//Task Operation
#define FLGPARTLOAD_NEW_LOAD            FLGPTN_BIT(0)   //new block has loaded
#define FLGPARTLOAD_UNCOMPRESS_DONE     FLGPTN_BIT(1)   //PartLoadTsk has finished
//@}


typedef struct _PART_DESC {
	BOOL   bLoaded;         ///< Indicate this section has load to memory
	UINT32 uiAddrExe;       ///< Execution address
	UINT32 uiLength;        ///< Length
	UINT32 uiAddrBegin;     ///< real need to load begin address (due to align, some byte of head maybe be loaded
	UINT32 uiBlkBegin;      ///< Calcuted begin block index
	UINT32 uiBlkCnt;        ///< Calcuted total block in this part
	INT32  iRemainBytes;    ///< Not block alignment bytes
} PART_DESC, *PPART_DESC;

//All member have to be UINT32
typedef struct _PARTLOAD_EXAM {
	UINT32 uiExamKey;        ///< must be CFG_PARTLAD_INIT_KEY
	UINT32 uiDramAddrOffset; ///< normal is 0, for debug, set it to other memory and dump it for compare.
	UINT32 uiForcePartIdx2Size; ///< for test partial load with compress data
	UINT32 uiResetLoaded; ///< reset bLoaded to '0' for repeating test
} PARTLOAD_EXAM, *PPARTLOAD_EXAM;

typedef struct _GZ_MEM {
	MEM_RANGE Mem; //gzip working buffer
	UINT32    Offset; //gzip offset of allocated memory
} GZ_MEM, *PGZ_MEM;

typedef struct _PIPE_UNCOMP {
	UINT32 uiAddrCompress;
	UINT32 uiSizeCompress;
	UINT32 uiAddrUnCompress;
	UINT32 uiSizeUnCompress;
	UINT32 uiLoadedSize; ///< main task loaded size
	PARTLOAD_CB_LOAD_FINISH fpFinish; ///< callback to notify FwSrv of finish part
} PIPE_UNCOMP, *PPIPE_UNCOMP;

typedef struct _PARTLOAD_CTRL {
	UINT32 uiInitKey;
	BOOL   bEnable;
	PARTLOAD_INIT Init;
	UINT32 uiBlockLd;       ///< loader reversed block number
	UINT32 uiBlockSize;     ///< Bytes for each block
	UINT32 uiSectNum;       ///< total section number
	UINT32 uiFwStartAddr;   ///< firmware starting address
	PART_DESC Sect[CFG_PARTLAD_SECTION_COUNTS];
	PARTLOAD_EXAM Exam;     ///< for exam direct path control
	GZ_MEM GzMem;           ///< for gz uncompress memory alloc mechanism
	PIPE_UNCOMP PipeUnComp; ///< for pipe multi-gz uncompress
} PARTLOAD_CTRL, *PPARTLOAD_CTRL;

typedef struct _PARTLOAD_BFC {
	UINT32 uiFourCC;    ///< FourCC = BCL1
	UINT32 uiAlgorithm; ///< algorithm always is 9
	UINT32 uiSizeUnComp;///< big endian uncompressed size
	UINT32 uiSizeComp;  ///< big endian compressed size
} PARTLOAD_BFC, *PPARTLOAD_BFC;

static PARTLOAD_CTRL m_PLCtrl = {0};
extern BININFO bin_info;

//PARTLOAD_DATA_TYPE_UNCOMPRESS
static void     xPartLoad_CalcBlockRange(UINT32 Idx);
static UINT32   xPartLoad_FindBlkIndexTop(UINT32 Idx);
static UINT32   xPartLoad_FindBlkIndexBottom(UINT32 Idx);
static ER       xPartLoad_ReadNandToDRAM(UINT32 Idx);
//PARTLOAD_DATA_TYPE_COMPRESS_LZ
static void     xPartLoad_CalcBlockRange_Lz(UINT32 Idx);
static ER       xPartLoad_ReadNandToDRAM_Lz(UINT32 Idx);
//PARTLOAD_DATA_TYPE_COMPRESS_GZ
static void     xPartLoad_CalcBlockRange_Gz(UINT32 Idx);
static ER       xPartLoad_ReadNandToDRAM_Gz(UINT32 Idx);
//MISC
static BOOL     xPartLoad_LockStrg(void);
static BOOL     xPartLoad_UnLockStrg(void);
THREAD_DECLARE(PartLoadTsk, args);

ER PartLoad_Init(PARTLOAD_INIT *pInit)
{
	UINT32 i;
	UINT32 uiCheckAddr; //check loaded size of that equal to aligned address.
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	STORAGE_OBJ *pStrgFw = pInit->pStrg;
	PART_DESC *pP0 = &pCtrl->Sect[0];

	BININFO* pBinInfo = (BININFO*)&bin_info;

	if (pBinInfo == NULL) {
		DBG_ERR("failed to get bininfo.\r\n");
		return E_PAR;
	}

	if (pInit->uiApiVer != PARTLOAD_API_VERSION) {
		DBG_ERR("invalid version.\r\n");
		return E_PAR;
	}

	if (pCtrl->uiInitKey == CFG_PARTLAD_INIT_KEY) {
		DBG_ERR("init twice.\r\n");
		return E_SYS;
	}

	if (pStrgFw == NULL) {
		DBG_ERR("pStrg is NULL.\r\n");
		return E_SYS;
	}

	PartLoad_InstallID();

	if (pCtrl->Exam.uiExamKey == CFG_PARTLAD_INIT_KEY) {
		//keep exam data
		PARTLOAD_EXAM exam = pCtrl->Exam;
		memset(pCtrl, 0, sizeof(PARTLOAD_CTRL));
		pCtrl->Exam = exam;

	} else {
		memset(pCtrl, 0, sizeof(PARTLOAD_CTRL));
	}

	pCtrl->Init = *pInit;
	pCtrl->uiSectNum = CFG_PARTLAD_SECTION_COUNTS;
	pCtrl->bEnable = (pBinInfo->head.Resv1[HEADINFO_RESV_IDX_BOOT_FLAG] & BOOT_FLAG_PARTLOAD_EN) ? TRUE : FALSE;
	if(pCtrl->Init.uiAddrBegin == pBinInfo->head.CodeEntry+pBinInfo->head.BinLength) {
		pCtrl->bEnable = FALSE; //T bin
	}
	pCtrl->uiFwStartAddr = pBinInfo->head.CodeEntry;
	pStrgFw->Lock();
	pStrgFw->GetParam(STRG_GET_SECTOR_SIZE, (UINT32)&pCtrl->uiBlockSize, 0);
	pStrgFw->Unlock();

	DBG_IND("Loader Loaded End Addr = 0x%x\r\n", pCtrl->Init.uiAddrBegin);

	if (pCtrl->bEnable == FALSE) {
		return E_OK; //full load
	}

	//fill information and check all section is available
	MEM_RANGE *pSectMem = (MEM_RANGE *)(pBinInfo->head.CodeEntry + CODE_SECTION_OFFSET);
	for (i = 0; i < pCtrl->uiSectNum; i++) {
		PART_DESC* pSect = &pCtrl->Sect[i];
		pSect->uiAddrExe = pSectMem[i].addr;
		pSect->uiLength = pSectMem[i].size;

		if(pSect->uiAddrExe==0)
		{
			DBG_ERR("Section[%d] is invalid\r\n", i);
			return E_SYS;
		} else {
			DBG_MSG("Section[%d] 0x%08X - 0x%08X \r\n", i, pSect->uiAddrExe, pSect->uiAddrExe + pSect->uiLength);
		}
	}

	//check loader loaded part-1 must be align by sector size from real part-1 size
	uiCheckAddr = pP0->uiAddrExe + (pP0->uiLength + pCtrl->uiBlockSize - 1) / pCtrl->uiBlockSize * pCtrl->uiBlockSize;
	if (uiCheckAddr != pCtrl->Init.uiAddrBegin) {
		DBG_ERR("Loaded Addr (0x%08X)!= Verified Addr (0x%08X)\r\n", pCtrl->Init.uiAddrBegin, uiCheckAddr);
		return E_SYS;
	}

	//fill part-1 info (bcz part-1 already loaded by loader)
	pP0->bLoaded     = TRUE;
	pP0->uiAddrBegin = pCtrl->uiFwStartAddr;
	pP0->uiBlkBegin  = pCtrl->uiBlockLd;
	pP0->uiBlkCnt    = (pCtrl->Init.uiAddrBegin - pP0->uiAddrBegin) / pCtrl->uiBlockSize;
	pP0->iRemainBytes = (INT32)pCtrl->Sect[1].uiAddrExe - pCtrl->Init.uiAddrBegin;

	pCtrl->uiInitKey = CFG_PARTLAD_INIT_KEY;

	return E_OK;
}

ER PartLoad_Load(UINT32 uiPart, PARTLOAD_CB_LOAD_FINISH fpFinish)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pCurr = &pCtrl->Sect[uiPart];
	PARTLOAD_DATA_TYPE DataType = (pCtrl->Exam.uiForcePartIdx2Size == 0) ? pCtrl->Init.DataType : PARTLOAD_DATA_TYPE_COMPRESS_GZ;

	if (pCtrl->Exam.uiResetLoaded != 0) {
		UINT32 n;
		for (n = 0; n < CFG_PARTLAD_SECTION_COUNTS; n++) {
			pCtrl->Sect[n].bLoaded = FALSE;
		}
		pCtrl->Exam.uiResetLoaded = 0;
	}

	if (pCtrl->uiInitKey != CFG_PARTLAD_INIT_KEY) {
		DBG_ERR("not init.\r\n");
		return E_SYS;
	}

	if (pCtrl->bEnable == FALSE) {
		return E_OK; //full load
	}

	if (uiPart >= pCtrl->uiSectNum) {
		DBG_ERR("uiPart:%d is out of range:%d.\r\n", uiPart, pCtrl->uiSectNum - 1);
		return E_PAR;
	}

	if ((DataType == PARTLOAD_DATA_TYPE_COMPRESS_LZ || DataType == PARTLOAD_DATA_TYPE_COMPRESS_GZ) &&
		uiPart > 1) {
		// already decompressed with part[1], just return ok.
		return E_OK;
	}

	xPartLoad_LockStrg();

	if (pCurr->bLoaded == FALSE) {
		DBG_IND("Begin\r\n");
		switch (DataType) {
		case PARTLOAD_DATA_TYPE_UNCOMPRESS:
			xPartLoad_CalcBlockRange(uiPart);
			if (xPartLoad_ReadNandToDRAM(uiPart) != E_OK) {
				xPartLoad_UnLockStrg();
				return E_SYS;
			}
			break;
		case PARTLOAD_DATA_TYPE_COMPRESS_LZ:
			pCtrl->PipeUnComp.fpFinish = fpFinish;
			xPartLoad_CalcBlockRange_Lz(uiPart);
			if (xPartLoad_ReadNandToDRAM_Lz(uiPart) != E_OK) {
				xPartLoad_UnLockStrg();
				return E_SYS;
			}
			break;
		case PARTLOAD_DATA_TYPE_COMPRESS_GZ:
			pCtrl->PipeUnComp.fpFinish = fpFinish;
			xPartLoad_CalcBlockRange_Gz(uiPart);
			if (xPartLoad_ReadNandToDRAM_Gz(uiPart) != E_OK) {
				xPartLoad_UnLockStrg();
				return E_SYS;
			}
			break;
		default:
			DBG_ERR("E_NOEXS\r\n");
			return E_NOEXS;
		}

		DBG_IND("End\r\n");
	}

	xPartLoad_UnLockStrg();
	return E_OK;
}

ER __attribute__((__no_instrument_function__)) PartLoad_IsLoaded(PARTLOAD_ISLOADED *pOut, UINT32 uiAddr)
{
	UINT32 i;
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;

	if (pCtrl->uiInitKey != CFG_PARTLAD_INIT_KEY) {
		return E_SYS;
	}

	if (pCtrl->bEnable == FALSE) {
		return E_CTX; //full load
	}

	for (i = 0; i < pCtrl->uiSectNum; i++) {
		PART_DESC *pSect = &pCtrl->Sect[i];

		if (uiAddr >= pSect->uiAddrExe && uiAddr < pSect->uiAddrExe + pSect->uiLength) {
			if (pSect->bLoaded == TRUE) {
				pOut->bLoaded = TRUE;
			} else {
				pOut->bLoaded = FALSE;
			}
			pOut->uiPart = i;
			return E_OK;
		}
	}
	return E_PAR;
}


ER PartLoad_GetInit(PARTLOAD_INIT *pInit)
{
	*pInit = m_PLCtrl.Init;
	return E_OK;
}

static void xPartLoad_CalcBlockRange(UINT32 Idx)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pCurr = &pCtrl->Sect[Idx];
	UINT32 uiAddrBegin  = xPartLoad_FindBlkIndexTop(Idx);
	UINT32 uiAddrEnd    = xPartLoad_FindBlkIndexBottom(Idx);

	if (uiAddrBegin <= uiAddrEnd) {
		pCurr->uiAddrBegin  = uiAddrBegin;
		pCurr->uiBlkBegin   = (pCurr->uiAddrBegin - pCtrl->uiFwStartAddr) / pCtrl->uiBlockSize + pCtrl->uiBlockLd;
		pCurr->uiBlkCnt     = (uiAddrEnd - uiAddrBegin) / pCtrl->uiBlockSize;
		pCurr->iRemainBytes = (uiAddrEnd - uiAddrBegin) % pCtrl->uiBlockSize;
	} else {
		//lack last part load
		pCurr->bLoaded = TRUE;
		DBG_ERR("Part[%d]: Begin(%08X)>End(%08X)\r\n", Idx, uiAddrBegin, uiAddrEnd);
	}
}

static void xPartLoad_CalcBlockRange_Lz(UINT32 Idx)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pCurr = &pCtrl->Sect[Idx];
	UINT32 uiLength     = (pCtrl->Exam.uiForcePartIdx2Size == 0) ? pCurr->uiLength : pCtrl->Exam.uiForcePartIdx2Size;
	UINT32 uiAddrBegin  = pCtrl->Init.uiAddrBegin;
	UINT32 uiAddrEnd    = (pCurr->uiAddrExe + uiLength + pCtrl->uiBlockSize - 1) / pCtrl->uiBlockSize * pCtrl->uiBlockSize; //block align to bottom

	pCurr->uiAddrBegin  = uiAddrBegin;
	pCurr->uiBlkBegin   = (pCurr->uiAddrBegin - pCtrl->uiFwStartAddr) / pCtrl->uiBlockSize + pCtrl->uiBlockLd;
	pCurr->uiBlkCnt     = (uiAddrEnd - uiAddrBegin) / pCtrl->uiBlockSize;
	pCurr->iRemainBytes = (uiAddrEnd - uiAddrBegin) % pCtrl->uiBlockSize;
}

static void xPartLoad_CalcBlockRange_Gz(UINT32 Idx)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pCurr = &pCtrl->Sect[Idx];
	UINT32 uiLength     = (pCtrl->Exam.uiForcePartIdx2Size == 0) ? pCurr->uiLength : pCtrl->Exam.uiForcePartIdx2Size;
	UINT32 uiAddrBegin  = pCtrl->Init.uiAddrBegin;
	UINT32 uiAddrEnd    = (pCurr->uiAddrExe + uiLength + pCtrl->uiBlockSize - 1) / pCtrl->uiBlockSize * pCtrl->uiBlockSize; //block align to bottom

	pCurr->uiAddrBegin  = uiAddrBegin;
	pCurr->uiBlkBegin   = (pCurr->uiAddrBegin - pCtrl->uiFwStartAddr) / pCtrl->uiBlockSize + pCtrl->uiBlockLd;
	pCurr->uiBlkCnt     = (uiAddrEnd - uiAddrBegin) / pCtrl->uiBlockSize;
	pCurr->iRemainBytes = (uiAddrEnd - uiAddrBegin) % pCtrl->uiBlockSize;
}

static UINT32 xPartLoad_FindBlkIndexTop(UINT32 Idx)
{
	INT32 i, j;
	PART_DESC *pTmp;
	UINT32 uiAddrBegin = 0;
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	//handle fw addr is not aligned to block size
	UINT32 uiOfsAlign = (pCtrl->uiFwStartAddr % pCtrl->uiBlockSize);
	PART_DESC *pCurr = &pCtrl->Sect[Idx];

	if (((pCurr->uiAddrExe - uiOfsAlign) % pCtrl->uiBlockSize) == 0) {
		//the start address of current part are block alignment
		uiAddrBegin = pCurr->uiAddrExe;
	} else {
		pTmp = NULL;
		for (i = (INT32)Idx - 1; i >= 0; i--) {
			pTmp = &pCtrl->Sect[i];

			if (pTmp->bLoaded == TRUE) {
				//if previous part is loaded, we calculate the block alignment address behind previous part block alignment size
				for (j = i; j >= 0; j--) { //this loop for case of previous block is too small to get uiAddrBegin
					PART_DESC *pTmpj = &pCtrl->Sect[j];
					if (pTmpj->uiBlkCnt != 0) {
						uiAddrBegin = pTmpj->uiAddrBegin + pTmpj->uiBlkCnt * pCtrl->uiBlockSize;
						break;
					}
				}
				break;
			} else {
				//if previous part is not loaded,
				uiAddrBegin = pCurr->uiAddrExe - ((pCurr->uiAddrExe-uiOfsAlign) % pCtrl->uiBlockSize);

				if (pTmp->uiAddrExe > uiAddrBegin) {
					//consider next part is too small.
					uiAddrBegin = 0; //Reset
					pTmp->bLoaded = TRUE; //this temp part will be loaded when load current part
				} else {
					//consider next part can cover current part alignment size
					break;
				}
			}
		}
	}

	return uiAddrBegin;
}

static UINT32 xPartLoad_FindBlkIndexBottom(UINT32 Idx)
{
	INT32 i, j;
	PART_DESC *pTmp;
	UINT32 uiAddrEnd = 0;
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	//handle fw addr is not aligned to block size
	UINT32 uiOfsAlign = (pCtrl->uiFwStartAddr % pCtrl->uiBlockSize);
	PART_DESC *pCurr = &pCtrl->Sect[Idx];
	UINT32 uiCurrAddrEnd = pCurr->uiAddrExe + pCurr->uiLength;

	if (((uiCurrAddrEnd - uiOfsAlign) % pCtrl->uiBlockSize) == 0 //alignment bottom
		|| Idx == pCtrl->uiSectNum - 1) {     //last part
		//the end address of current part are block alignment or last part
		uiAddrEnd = uiCurrAddrEnd;
	} else {
		pTmp = NULL;
		for (i = (INT32)Idx + 1; i < (INT32)pCtrl->uiSectNum; i++) {
			UINT32 uiBottom;

			pTmp = &pCtrl->Sect[i];
			uiBottom = pTmp->uiAddrExe + pTmp->uiLength;

			if (pTmp->bLoaded == TRUE) {
				//if next part is loaded, we calculate the block alignment address ahead next part with block alignment size
				for (j = i; j < (INT32)pCtrl->uiSectNum; j++) { //this loop for case of next block is too small to get uiAddrEnd
					PART_DESC *pTmpj = &pCtrl->Sect[j];
					if (pTmpj->uiAddrBegin != 0) {
						uiAddrEnd = pTmpj->uiAddrBegin;
						break;
					}
				}
				break;
			} else {
				//take ceiling
				uiAddrEnd = ALIGN_CEIL(uiCurrAddrEnd-uiOfsAlign, pCtrl->uiBlockSize) + uiOfsAlign;

				if (uiBottom < uiAddrEnd) {
					uiAddrEnd = 0; //Reset
					pTmp->bLoaded = TRUE; //this temp part will be loaded when load current part
				} else {
					break;
				}
			}
		}

		if (uiAddrEnd == 0 && pTmp != NULL) { //arrival to last part
			uiAddrEnd = pTmp->uiAddrExe + pTmp->uiLength;
		}
	}

	return uiAddrEnd;
}

static ER xPartLoad_ReadNandToDRAM(UINT32 Idx)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pPart = &pCtrl->Sect[Idx];
	STORAGE_OBJ *pStrgFw = pCtrl->Init.pStrg;

	if (pPart->uiBlkCnt != 0) {
		DBG_IND("Copy PART%d %08X - %08X [BLK: %d - %d]\r\n", Idx + 1, pPart->uiAddrBegin, pPart->uiAddrBegin + pPart->uiBlkCnt * pCtrl->uiBlockSize, pPart->uiBlkBegin, pPart->uiBlkBegin + pPart->uiBlkCnt);
		if (pStrgFw->RdSectors((INT8 *)(pPart->uiAddrBegin + pCtrl->Exam.uiDramAddrOffset), pPart->uiBlkBegin, pPart->uiBlkCnt) != E_OK) {
			DBG_ERR("Failed to RdSectors-1\r\n");
			return E_SYS;
		}
	}

	// Because the NAND driver read data by block, we have to handle the remaining
	// data carefully to prevent DRAM data (RW/ZI) is overwriten by NAND data.
	if (pPart->iRemainBytes > 0) {
		UINT32  uiNextAddr = pPart->uiAddrBegin + pPart->uiBlkCnt * pCtrl->uiBlockSize + pCtrl->Exam.uiDramAddrOffset;

		if (pCtrl->Init.uiWorkingSize < pCtrl->uiBlockSize) {
			DBG_ERR("working buf size have to be %d bytes.\r\n", pCtrl->uiBlockSize);
			return E_SYS;
		}


		DBG_IND("Copy PART%d %08X - %08X [BLK: %d]\r\n", Idx + 1, uiNextAddr, uiNextAddr + pPart->iRemainBytes, pPart->uiBlkBegin + pPart->uiBlkCnt);
		if (pStrgFw->RdSectors((INT8 *)pCtrl->Init.uiWorkingAddr, pPart->uiBlkBegin + pPart->uiBlkCnt, 1) != E_OK) {
			DBG_ERR("Failed to RdSectors-2\r\n");
			return E_SYS;
		}
		memcpy((void *)uiNextAddr, (const void *)pCtrl->Init.uiWorkingAddr, pPart->iRemainBytes);
	}

	pPart->bLoaded = TRUE;

	return E_OK;
}

static ER xPartLoad_ReadNandToDRAM_Lz(UINT32 Idx)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pPart = &pCtrl->Sect[Idx];
	PARTLOAD_BFC *pHeader = (PARTLOAD_BFC *)pCtrl->Init.uiWorkingAddr;
	UINT32 MemReq = pPart->uiBlkCnt * pCtrl->uiBlockSize;
	STORAGE_OBJ *pStrgFw = pCtrl->Init.pStrg;

	DBG_IND("MemReq = 0x%X\r\n", MemReq);

	//check working memory
	if (pCtrl->Init.uiWorkingSize < MemReq) {
		DBG_ERR("working buf size have to be larger than %d bytes.\r\n", MemReq);
		return E_SYS;
	}

	pStrgFw->RdSectors((INT8 *)pCtrl->Init.uiWorkingAddr, pPart->uiBlkBegin, 1);

	if (pHeader->uiFourCC == MAKEFOURCC('B', 'C', 'L', '1')) {
		UINT32 uiDecoded;
		UINT32 uiLength = (pCtrl->Exam.uiForcePartIdx2Size == 0) ? UINT32_SWAP(pHeader->uiSizeComp) : pCtrl->Exam.uiForcePartIdx2Size;
		UINT32 uiSizeComp = UINT32_SWAP(pHeader->uiSizeComp);
		UINT32 uiSizeUnComp = UINT32_SWAP(pHeader->uiSizeUnComp);
		UINT32 uiBlkCnt = (uiSizeComp + pCtrl->uiBlockSize - 1) / pCtrl->uiBlockSize;
		UINT32 uiAddrCompress = pCtrl->Init.uiWorkingAddr + sizeof(PARTLOAD_BFC);

		DBG_IND("DeComp PART%d %08X - %08X [BLK: %d - %d]\r\n", Idx + 1, pPart->uiAddrBegin, pPart->uiAddrBegin + pPart->uiBlkCnt * pCtrl->uiBlockSize, pPart->uiBlkBegin, pPart->uiBlkBegin + uiBlkCnt);

		pStrgFw->RdSectors((INT8 *)pCtrl->Init.uiWorkingAddr, pPart->uiBlkBegin, uiBlkCnt);

		uiDecoded = LZ_Uncompress((unsigned char *)(uiAddrCompress)
								  , (unsigned char *)(pCtrl->Init.uiAddrBegin + pCtrl->Exam.uiDramAddrOffset)
								  , uiLength);

		if (uiDecoded + pCtrl->Sect[0].uiLength < uiSizeUnComp) {
			DBG_ERR("Uncompress data size not match real_end=%08X, bfc_info=%08X\r\n", uiDecoded, uiSizeUnComp);
			return E_SYS;
		}

	} else {
		DBG_ERR("ERROR BCF Header\r\n");
		return E_SYS;
	}

	return E_OK;
}

static void xPartLoad_GzMemInit(UINT32 uiAddr, UINT32 uiSize)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	pCtrl->GzMem.Mem.addr = uiAddr;
	pCtrl->GzMem.Mem.size = uiSize;
	pCtrl->GzMem.Offset = 0;
}

static void *xPartLoad_GzAlloc(void *x, unsigned items, unsigned size)
{
	void *p = NULL;
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	size = ALIGN_CEIL_4(size * items);
	if (pCtrl->GzMem.Offset + size > pCtrl->GzMem.Mem.size) {
		DBG_ERR("gzip temp memory not enough, need more: %d bytes\r\n", size);
		return p;
	}

	p = (void *)(pCtrl->GzMem.Mem.addr + pCtrl->GzMem.Offset);
	memset(p, 0, size);
	pCtrl->GzMem.Offset += size;

	return p;
}

static void xPartLoad_GzFree(void *x, void *addr, unsigned nb)
{
	//just skip
}

#if (CFG_DEBUG_PIPE_UNCOMPRESS)
static unsigned int GZ_Uncompress(unsigned char *in, unsigned char *out,
								  unsigned int insize, unsigned int outsize)
{
	int err;
	z_stream stream = {0};
	stream.next_in = (z_const Bytef *)in;
	stream.avail_in = insize;
	stream.next_out = (z_const Bytef *)out;
	stream.avail_out = outsize;
	stream.zalloc = (alloc_func)xPartLoad_GzAlloc;
	stream.zfree = (free_func)xPartLoad_GzFree;
	stream.opaque = (voidpf)0;
	err = inflateInit(&stream);
	if (err != Z_OK) {
		DBG_ERR("Failed to inflateInit, err = %d\r\n", err);
		return E_SYS;
	}

	err = inflate(&stream, Z_NO_FLUSH);

	inflateEnd(&stream);

	if (err == Z_STREAM_END) {
		return stream.total_out;
	}

	return 0;
}
#endif

static BOOL xPartLoad_LockStrg(void)
{
	m_PLCtrl.Init.pStrg->Lock();
	m_PLCtrl.Init.pStrg->Open();
	return TRUE;
}

static BOOL xPartLoad_UnLockStrg(void)
{
	m_PLCtrl.Init.pStrg->Close();
	m_PLCtrl.Init.pStrg->Unlock();
	return TRUE;
}

ER PartLoad_ExamProperty(UINT32 uiItem, UINT32 uiValue)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	UINT32 *pItem = (UINT32 *)&pCtrl->Exam;

	if (uiItem == 0) {
		if (uiValue == 0) { //set zero to reset all
			memset(pItem, 0, sizeof(pCtrl->Exam));
		} else {
			pCtrl->Exam.uiExamKey = uiValue;
		}
		return E_OK;
	} else if (pCtrl->Exam.uiExamKey != CFG_PARTLAD_INIT_KEY) {
		DBG_ERR("key invalid\r\n");
		return E_SYS;
	}

	pItem[uiItem] = uiValue;

	return E_OK;
}

static ER xPartLoad_ReadNandToDRAM_Gz(UINT32 Idx)
{
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PART_DESC *pPart = &pCtrl->Sect[Idx];
	PARTLOAD_BFC *pHeader = (PARTLOAD_BFC *)(pCtrl->Init.uiWorkingAddr + CFG_GZ_WORK_SIZE);
	STORAGE_OBJ *pStrgFw = pCtrl->Init.pStrg;

	xPartLoad_GzMemInit(pCtrl->Init.uiWorkingAddr, CFG_GZ_WORK_SIZE);

	if (pCtrl->Init.uiWorkingSize < CFG_GZ_WORK_SIZE + pCtrl->uiBlockSize) {
		DBG_ERR("working buf size have to be larger than %d bytes.\r\n", CFG_GZ_WORK_SIZE + pCtrl->uiBlockSize);
		return E_SYS;
	}

#if 0 // for debug, to clean the flash reading data
	memset(pHeader, 0xFF, pCtrl->Init.uiWorkingSize - CFG_GZ_WORK_SIZE);
	vos_cpu_dcache_sync((VOS_ADDR)pHeader, pCtrl->Init.uiWorkingSize - CFG_GZ_WORK_SIZE, VOS_DMA_BIDIRECTIONAL);
#endif

	pStrgFw->RdSectors((INT8 *)pHeader, pPart->uiBlkBegin, 1);

	if (pHeader->uiFourCC == MAKEFOURCC('B', 'C', 'L', '1')) {
#if (!CFG_DEBUG_PIPE_UNCOMPRESS)
		UINT32 i;
		PIPE_UNCOMP *pPipe = &pCtrl->PipeUnComp;
		pPipe->uiAddrCompress = ((UINT32)pHeader) + sizeof(PARTLOAD_BFC);
		pPipe->uiSizeCompress = UINT32_SWAP(pHeader->uiSizeComp);
		pPipe->uiAddrUnCompress = pCtrl->Init.uiAddrBegin + pCtrl->Exam.uiDramAddrOffset;
		pPipe->uiSizeUnCompress = UINT32_SWAP(pHeader->uiSizeUnComp);
		pPipe->uiLoadedSize = pCtrl->uiBlockSize - sizeof(PARTLOAD_BFC);
		UINT32 uiBlkCnt = ALIGN_CEIL(pPipe->uiSizeCompress+sizeof(PARTLOAD_BFC), pCtrl->uiBlockSize) / pCtrl->uiBlockSize;
		UINT32 uiBlkMultiply = (pCtrl->uiBlockSize >= CFG_GZ_BLK_SIZE) ? 1 : CFG_GZ_BLK_SIZE / pCtrl->uiBlockSize;
		UINT32 MemReq = uiBlkCnt * pCtrl->uiBlockSize + CFG_GZ_WORK_SIZE;
		DBG_IND("MemReq = 0x%X\r\n", MemReq);
		//check working memory
		if (pCtrl->Init.uiWorkingSize < MemReq) {
			DBG_ERR("working buf size have to be larger than %d bytes.\r\n", MemReq);
			return E_SYS;
		}

		if (uiBlkMultiply > 1 && (CFG_GZ_BLK_SIZE % pCtrl->uiBlockSize) != 0) {
			DBG_ERR("Cannot support block size %X/%X!=0\r\n", CFG_GZ_BLK_SIZE, pCtrl->uiBlockSize);
			return E_SYS;
		}

		clr_flg(PARTLOAD_FLG_ID, 0xFFFFFFFF);
		PARTLOAD_TSK_ID = vos_task_create(PartLoadTsk, NULL, "PartLoadTsk", PRI_PARTLOAD, STKSIZE_PARTLOAD);
		vos_task_resume(PARTLOAD_TSK_ID);

		for (i = 1; i < uiBlkCnt; i += uiBlkMultiply) {
			pStrgFw->RdSectors(((INT8 *)pHeader) + i * pCtrl->uiBlockSize, pPart->uiBlkBegin + i, uiBlkMultiply);
			pPipe->uiLoadedSize += pCtrl->uiBlockSize * uiBlkMultiply;
			set_flg(PARTLOAD_FLG_ID, FLGPARTLOAD_NEW_LOAD);
		}

		FLGPTN uiFlag;
		wai_flg(&uiFlag, PARTLOAD_FLG_ID, FLGPARTLOAD_UNCOMPRESS_DONE, TWF_ORW | TWF_CLR);
#else
		UINT32 uiDecoded;
		UINT32 uiLength = (pCtrl->Exam.uiForcePartIdx2Size == 0) ? UINT32_SWAP(pHeader->uiSizeComp) : pCtrl->Exam.uiForcePartIdx2Size;
		UINT32 uiSizeComp = UINT32_SWAP(pHeader->uiSizeComp);
		UINT32 uiSizeUnComp = UINT32_SWAP(pHeader->uiSizeUnComp);
		UINT32 uiBlkCnt = (uiSizeComp + pCtrl->uiBlockSize - 1) / pCtrl->uiBlockSize;
		UINT32 uiAddrCompress = ((UINT32)pHeader) + sizeof(PARTLOAD_BFC);

		DBG_IND("DeComp PART%d %08X - %08X [BLK: %d - %d]\r\n", Idx + 1, pPart->uiAddrBegin, pPart->uiAddrBegin + pPart->uiBlkCnt * pCtrl->uiBlockSize, pPart->uiBlkBegin, pPart->uiBlkBegin + uiBlkCnt);

		// deduct loaded first compressed-block
		pStrgFw->RdSectors(((INT8 *)pHeader) + pCtrl->uiBlockSize, pPart->uiBlkBegin + 1, uiBlkCnt - 1);
		uiDecoded = GZ_Uncompress((unsigned char *)(uiAddrCompress)
								  , (unsigned char *)(pCtrl->Init.uiAddrBegin + pCtrl->Exam.uiDramAddrOffset)
								  , uiLength
								  , uiSizeUnComp);

		if (uiDecoded + pCtrl->Sect[0].uiLength < uiSizeUnComp) {
			DBG_ERR("Uncompress data size not match real_end=%08X, bfc_info=%08X\r\n", uiDecoded, uiSizeUnComp);
			return E_SYS;
		}

		if (pCtrl->PipeUnComp.fpFinish) {
			UINT32 i;
			for (i = Idx; i < pCtrl->uiSectNum; i++) {
				PART_DESC *pSect = &pCtrl->Sect[i];
				if (pSect->uiAddrExe) {
					pCtrl->PipeUnComp.fpFinish(i);
				}
			}
		}
#endif
	} else {
		DBG_ERR("ERROR BCF Header\r\n");
		return E_SYS;
	}

	return E_OK;
}

THREAD_DECLARE(PartLoadTsk, args)
{
	int err;
	z_stream stream = {0};
	PARTLOAD_CTRL *pCtrl = &m_PLCtrl;
	PIPE_UNCOMP *pPipe = &pCtrl->PipeUnComp;
	//partial
	UINT32 SectIdx = 1; //from part-2 start
	UINT32 MemOffset = pCtrl->Sect[0].uiAddrExe + pCtrl->Sect[0].uiLength;

	THREAD_ENTRY();

	unsigned long prev_avail_in = 0;
	unsigned long stream_avail_in_cnt = 0;
	stream.next_in = (z_const Bytef *)pPipe->uiAddrCompress;
	stream.next_out = (z_const Bytef *)pPipe->uiAddrUnCompress;
	stream.avail_out = pPipe->uiSizeUnCompress;
	stream.zalloc = (alloc_func)xPartLoad_GzAlloc;
	stream.zfree = (free_func)xPartLoad_GzFree;
	stream.opaque = (voidpf)0;
	err = inflateInit(&stream);
	if (err != Z_OK) {
		DBG_ERR("Failed to inflateInit, err = %d\r\n", err);
		THREAD_RETURN(-1);
	}

	do {
		while (prev_avail_in + CFG_GZ_BLK_SIZE > pPipe->uiLoadedSize &&
			   pPipe->uiLoadedSize < pPipe->uiSizeCompress) {
			FLGPTN uiFlag;
			vos_flag_wait_interruptible(&uiFlag, PARTLOAD_FLG_ID, FLGPARTLOAD_NEW_LOAD, TWF_ORW | TWF_CLR);
		}

		if (prev_avail_in + CFG_GZ_BLK_SIZE > pPipe->uiSizeCompress) {
			prev_avail_in = pPipe->uiSizeCompress; //last data
		} else {
			prev_avail_in += CFG_GZ_BLK_SIZE;
		}


		clr_flg(PARTLOAD_FLG_ID, FLGPARTLOAD_NEW_LOAD);
		unsigned long incoming = prev_avail_in - stream_avail_in_cnt;
		stream.avail_in += incoming;
		stream_avail_in_cnt += incoming;
		err = inflate(&stream, Z_NO_FLUSH);

		// check part if finish
		if (SectIdx < pCtrl->uiSectNum) {
			UINT32 i;
			for (i = SectIdx; i < pCtrl->uiSectNum; i++) {
				PART_DESC *pSect = &pCtrl->Sect[i];
				if (pSect->uiAddrExe == 0) {
					break;
				} else if (MemOffset + stream.total_out >= pSect->uiAddrExe + pSect->uiLength) {
					pSect->bLoaded = TRUE;
#if (!CFG_DEBUG_PIPE_UNCOMPRESS2)
					pCtrl->PipeUnComp.fpFinish(SectIdx);
#endif
					SectIdx = i + 1;
				} else {
					break;
				}
			}
		}
	} while (err == Z_OK);

	inflateEnd(&stream);

#if (CFG_DEBUG_PIPE_UNCOMPRESS2)
	if (pCtrl->PipeUnComp.fpFinish) {
		UINT32 i;
		for (i = 1; i < pCtrl->uiSectNum; i++) {
			PART_DESC *pSect = &pCtrl->Sect[i];
			if (pSect->uiAddrExe) {
				pCtrl->PipeUnComp.fpFinish(i);
			}
		}
	}
#endif

	if (err == Z_STREAM_END) {
		//set finish flag
		set_flg(PARTLOAD_FLG_ID, FLGPARTLOAD_UNCOMPRESS_DONE);
	} else {
		DBG_ERR("Failed to finish inflate, err = %d\r\n", err);
	}

	THREAD_RETURN(0);
}
