/**
    FwSrv, Service command function implementation

    @file       FwSrvCmd.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "FwSrvInt.h"

#define CFG_ROOT_FS_HDR_SIZE        0x40
#define CFG_BFC_MAX_SIZE            0xA00000
#define CFG_CHKSUM_FOURCC           MAKEFOURCC('C','K','S','M')

static const FWSRV_CMD_DESC m_FwSrvCallTbl[] = {
	// Idx --------------------------- Command Pointer ----------- Input Data Size -------------------- Output Data Size -------
	{FWSRV_CMD_IDX_UNKNOWN,         NULL,                       0U,                                  0U},
	{FWSRV_CMD_IDX_PL_LOAD_BURST,   xFwSrv_CmdPlLoadBurst,      sizeof(FWSRV_PL_LOAD_BURST_IN),      0U},
	{FWSRV_CMD_IDX_BIN_UPDATE_ALL_IN_ONE, xFwSrv_CmdBinUpdateAllInOne, sizeof(FWSRV_BIN_UPDATE_ALL_IN_ONE), 0U},
	{FWSRV_CMD_IDX_FASTLOAD,        xFwSrv_CmdFastLoad,         sizeof(FWSRV_FASTLOAD),              sizeof(MEM_RANGE)},

};

const char* fdt_path_strg[3] = {
	"/nand",
	"/nor",
	"/mmc@f0510000",
};
/******************************************************************************/

/**
    Get command function table

    @param[out] pNum total function number
    @return command function table
*/
const FWSRV_CMD_DESC *xFwSrv_GetCallTbl(UINT32 *pNum)
{
	*pNum = sizeof(m_FwSrvCallTbl) / sizeof(m_FwSrvCallTbl[0]);
	return m_FwSrvCallTbl;
}

/**
    FWSRV_CMD_IDX_PL_LOAD_BURST mapped implemenatation

    @param[in] pCmd command description
    @return error code
*/
FWSRV_ER xFwSrv_CmdPlLoadBurst(const FWSRV_CMD *pCmd)
{
	UINT32 i = 0;
	FWSRV_CTRL *pCtrl = xFwSrv_GetCtrl();
	FWSRV_PL_LOAD_BURST_IN *pData = (FWSRV_PL_LOAD_BURST_IN *)pCmd->In.pData;

	if (pCtrl->Init.PlInit.DataType == PARTLOAD_DATA_TYPE_UNCOMPRESS) {
		while (pData->puiIdxSequence[i] != FWSRV_PL_BURST_END_TAG) {
			if (PartLoad_Load(pData->puiIdxSequence[i], NULL) != E_OK) {
				return xFwSrv_Err(FWSRV_ER_PL_LOAD);
			}
			if (pData->fpLoadedCb != NULL) {
				pData->fpLoadedCb(pData->puiIdxSequence[i]);
			}
			i++;
		}
	} else {
		if (PartLoad_Load(1, pData->fpLoadedCb) != E_OK) { //only part-2 (while compress) on oncompress
			return xFwSrv_Err(FWSRV_ER_PL_LOAD);
		}
	}

	return FWSRV_ER_OK;
}

static const char *xFwSrv_GetFdtStrgBasePath(const void *p_fdt)
{
	int i;
	int nodeoffset;

	for (i = 0; i < (int)(sizeof(fdt_path_strg)/sizeof(fdt_path_strg[0])); i++) {
		const char *fdt_path = fdt_path_strg[i];
		nodeoffset = fdt_path_offset(p_fdt, fdt_path);
		if (nodeoffset >= 0) {
			return fdt_path;
		}
	}

	DBG_ERR("cannot find fdt_path_strg.\n");
	return NULL;
}

static int nvt_get_partition_info(const void *p_fdt, const char *fdt_path, unsigned long long *p_addr, unsigned long long *p_size)
{
	//get fdt node
	int nodeoffset; /* next node offset from libfdt */
	nodeoffset = fdt_path_offset(p_fdt, fdt_path);
	if (nodeoffset < 0) {
		DBG_ERR("cannot find %s in fdt\n", fdt_path);
		return -1;
	}

	//get offset and size
	int len;
	const unsigned long long *nodep;
	nodep = (const unsigned long long *)fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("cannot find reg in fdt %s\n", fdt_path);
		return -1;
	}

	*p_addr = be64_to_cpu(nodep[0]);
	*p_size = be64_to_cpu(nodep[1]);
	return 0;
}

static int nvt_on_partition_enum_sanity(unsigned int id, NVTPACK_MEM* p_mem, void* p_user_data)
{
	char fdt_path[64] = {0};
	const void *p_fdt = (const void *)p_user_data;
	const char *strg_base_path = (const char *)xFwSrv_GetCtrl()->UpdFw.pFdtNvtStrgBasePath;
	unsigned long long partition_ofs, partition_size;

	//make path
	snprintf(fdt_path, sizeof(fdt_path)-1, "%s/nvtpack/index/id%d", strg_base_path, id);

	//get fdt node
	int nodeoffset; /* next node offset from libfdt */
	nodeoffset = fdt_path_offset(p_fdt, fdt_path);
	if (nodeoffset < 0) {
		DBG_ERR("cannot find %s in fdt\n", fdt_path);
		return 0;
	}

	//get partition name
	int len;
	const void *nodep;  /* property node pointer */
	nodep = fdt_getprop(p_fdt, nodeoffset, "partition_name", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("cannot find partition_name in fdt %s\n", fdt_path);
		return 0;
	}

	//only check sanity of known partition
	int er;
	char partition_name[16] = {0};
	strncpy(partition_name, (const char *)nodep, sizeof(partition_name)-1);

	//get partition offset and size
	snprintf(fdt_path, sizeof(fdt_path)-1, "%s/partition_%s", strg_base_path, partition_name);
	if(nvt_get_partition_info(p_fdt, fdt_path, &partition_ofs, &partition_size) != 0) {
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}

	if (strncmp(partition_name, "fdt", 4) == 0) {
		if ((er = fdt_check_header(p_mem->p_data)) != 0) {
			DBG_ERR("invalid fdt header, addr=0x%08X er = %d \n", (unsigned int)p_fdt, er);
			return -1;
		}
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("fdt bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "fdt.app", 8) == 0) {
		if ((er = fdt_check_header(p_mem->p_data)) != 0) {
			DBG_ERR("invalid fdt.app header, addr=0x%08X er = %d \n", (unsigned int)p_fdt, er);
			return -1;
		}
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("fdt.app bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "uboot", 6) == 0) {
		if(MemCheck_CalcCheckSum16Bit((UINT32)p_mem->p_data, (UINT32)p_mem->len) != 0) {
			DBG_ERR("uboot checksum failed.\n");
			return -1;
		}
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("uboot bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "rtos", 5) == 0) {
		if(MemCheck_CalcCheckSum16Bit((UINT32)p_mem->p_data, (UINT32)p_mem->len) != 0) {
			DBG_ERR("rtos checksum failed.\n");
			return -1;
		}
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("rtos bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "kernel", 7) == 0) {
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("kernel bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "rootfs", 7) == 0) {
		if (*(UINT32 *)p_mem->p_data == CFG_CHKSUM_FOURCC) {
			if(MemCheck_CalcCheckSum16Bit((UINT32)p_mem->p_data, (UINT32)p_mem->len) != 0) {
				DBG_ERR("rootfs checksum failed.\n");
				return -1;
			}
		} else {
			DBG_WRN("rootfs skip checksum.\n");
		}
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("rootfs bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "rootfs1", 8) == 0) {
		if (*(UINT32 *)p_mem->p_data == CFG_CHKSUM_FOURCC) {
			if(MemCheck_CalcCheckSum16Bit((UINT32)p_mem->p_data, (UINT32)p_mem->len) != 0) {
				DBG_ERR("rootfs1 checksum failed.\n");
				return -1;
			}
		} else {
			DBG_WRN("rootfs1 skip checksum.\n");
		}
		if ((unsigned long long)p_mem->len > partition_size) {
			DBG_ERR("rootfs1 bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
			return -1;
		}
	} else if (strncmp(partition_name, "app", 4) == 0) {
		if (*(UINT32 *)p_mem->p_data == CFG_CHKSUM_FOURCC) {
			if ((unsigned long long)p_mem->len > partition_size) {
				DBG_ERR("app bin size (%lld) > partition size(%lld) \n", (unsigned long long)p_mem->len, partition_size);
				return -1;
			}
		} else {
			DBG_WRN("app skip checksum.\n");
		}
	} else {
		DBG_DUMP("skip check partition name = %s\n", partition_name);
	}

	return 0;
}


static int nvt_on_partition_enum_update(unsigned int id, NVTPACK_MEM* p_mem, void* p_user_data)
{
	FWSRV_CTRL *pCtrl = xFwSrv_GetCtrl();

	char fdt_path[64] = {0};
	const void *p_fdt = (const void *)p_user_data;
	const char *strg_base_path = (const char *)pCtrl->UpdFw.pFdtNvtStrgBasePath;
	unsigned long long partition_ofs, partition_size;

	//make path
	snprintf(fdt_path, sizeof(fdt_path)-1, "%s/nvtpack/index/id%d", strg_base_path, id);

	//get fdt node
	int nodeoffset; /* next node offset from libfdt */
	nodeoffset = fdt_path_offset(p_fdt, fdt_path);
	if (nodeoffset < 0) {
		DBG_ERR("cannot find %s in fdt\n", fdt_path);
		return 0;
	}

	//get partition name
	int len;
	const void *nodep;  /* property node pointer */
	nodep = fdt_getprop(p_fdt, nodeoffset, "partition_name", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("cannot find partition_name in fdt %s\n", fdt_path);
		return 0;
	}

	//only update of known partition
	char partition_name[16] = {0};
	strncpy(partition_name, (const char *)nodep, sizeof(partition_name)-1);

	//get partition offset and size
	snprintf(fdt_path, sizeof(fdt_path)-1, "%s/partition_%s", strg_base_path, partition_name);
	if(nvt_get_partition_info(p_fdt, fdt_path, &partition_ofs, &partition_size) != 0) {
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}

	STORAGE_OBJ* pStrg = NULL;
	unsigned int ofs = 0;

	if (strncmp(partition_name, "fdt.app", 7) == 0) {
		pStrg = pCtrl->Init.StrgMap.pStrgApp;
	} else if (strncmp(partition_name, "fdt", 3) == 0) {
		pStrg = pCtrl->Init.StrgMap.pStrgFdt;
	} else if (strncmp(partition_name, "uboot", 5) == 0) {
		pStrg = pCtrl->Init.StrgMap.pStrgUboot;
	} else if (strncmp(partition_name, "rtos", 5) == 0) {
		pStrg = pCtrl->Init.StrgMap.pStrgRtos;
	} else if (strncmp(partition_name, "kernel", 7) == 0) {
		pStrg = pCtrl->Init.StrgMap.pStrgKernel;
	} else if (strncmp(partition_name, "rootfs", 7) == 0) {
		if (*(UINT32 *)p_mem->p_data == CFG_CHKSUM_FOURCC) {
			ofs = sizeof(NVTPACK_CHKSUM_HDR);
		}
		pStrg = pCtrl->Init.StrgMap.pStrgRootfs;
	} else if (strncmp(partition_name, "rootfs1", 8) == 0) {
		if (*(UINT32 *)p_mem->p_data == CFG_CHKSUM_FOURCC) {
			ofs = sizeof(NVTPACK_CHKSUM_HDR);
		}
		pStrg = pCtrl->Init.StrgMap.pStrgRootfs1;
	} else if (strncmp(partition_name, "app", 4) == 0) {
		if (*(UINT32 *)p_mem->p_data == CFG_CHKSUM_FOURCC) {
			ofs = sizeof(NVTPACK_CHKSUM_HDR);
		}
		pStrg = pCtrl->Init.StrgMap.pStrgAppfs;
	} else {
		DBG_DUMP("skip update partition: %s\n", partition_name);
		return 0;
	}

	if(pStrg != NULL) {
		DBG_DUMP("\nupdating %s ...\n", partition_name);
		UINT32 blksize;
		pStrg->GetParam(STRG_GET_BEST_ACCESS_SIZE, (UINT32)&blksize, 0); //get block size
		pStrg->SetParam(STRG_SET_PARTITION_SECTORS, (UINT32)(partition_ofs/blksize), (UINT32)(partition_size/blksize));
		pStrg->Lock();
		pStrg->Open();
		pStrg->WrSectors((INT8*)p_mem->p_data+ofs, 0, (UINT32)(ALIGN_CEIL(p_mem->len-ofs, blksize)/blksize));
		pStrg->Close();
		pStrg->Unlock();
	} else {
		DBG_DUMP("skip update partition: %s\n", partition_name);
	}

	return 0;
}

FWSRV_ER xFwSrv_CmdBinUpdateAllInOne(const FWSRV_CMD *pCmd)
{
	FWSRV_BIN_UPDATE_ALL_IN_ONE *pData = (FWSRV_BIN_UPDATE_ALL_IN_ONE *)pCmd->In.pData;
	FWSRV_CTRL *pCtrl = xFwSrv_GetCtrl();

	NVTPACK_VERIFY_OUTPUT np_verify = {0};
	NVTPACK_GET_PARTITION_INPUT np_get_input;
	NVTPACK_ENUM_PARTITION_INPUT np_enum_input;
	NVTPACK_MEM mem_in = {(void*)pData->uiSrcBufAddr, (unsigned int)pData->uiSrcBufSize};
	NVTPACK_MEM mem_out = {0};

	memset(&np_get_input, 0, sizeof(np_get_input));
	memset(&np_enum_input, 0, sizeof(np_enum_input));

#if 0
	if (pData->uiSrcBufAddr != ALIGN_CEIL_16(pData->uiSrcBufAddr)) {
		DBG_WRN("Failed to protect source buffer, because uiSrcBufAddr must be 16 byte alignment.\r\n");
	} else {
		//Protect Memory
		DBGUT_DMA_PROTECT_CFG Cfg = {0};
		DBGUT_CMD_DATA Cmd = {0};
		Cfg.Fw.bEn = FALSE;
		Cfg.User.Base.bEn = TRUE;
		Cfg.User.Base.bDetectOnly = FALSE;
		Cfg.User.bIncludeCpu = TRUE;
		Cfg.User.bIncludeCpu2 = TRUE;
		Cfg.User.bIncludeDsp = TRUE;
		Cfg.User.uiAddr = pData->uiSrcBufAddr;
		Cfg.User.uiSize = ALIGN_CEIL_16(pData->uiSrcBufSize);
		Cmd.In.pData = &Cfg;
		Cmd.In.uiNumByte = sizeof(Cfg);
		DbgUt_CmdDma(DBGUT_CMD_DMA_PROTECT_CFG, &Cmd);
		DbgUt_CmdDma(DBGUT_CMD_DMA_PROTECT_START, NULL);
	}
#endif
	//first check all partition valid that is going to update.
	DBG_DUMP("Checking valid...\r\n");

	if(nvtpack_verify(&mem_in, &np_verify) != NVTPACK_ER_SUCCESS)
	{
		DBG_ERR("verify failed.\r\n");
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}
	if(np_verify.ver != NVTPACK_VER_16072017)
	{
		DBG_ERR("wrong all-in-one bin version\r\n");
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}


	int er;
	const void *p_fdt = fdt_get_base();

	//check if fdt exists, use the new one.
	np_get_input.id = 1; // fdt must always put in partition[1]
	np_get_input.mem = mem_in;
	if(nvtpack_get_partition(&np_get_input,&mem_out) == NVTPACK_ER_SUCCESS)
	{
		p_fdt = mem_out.p_data;
		if ((er = fdt_check_header(p_fdt)) != 0) {
			DBG_ERR("invalid fdt header, addr=0x%08X er = %d \n", (unsigned int)p_fdt, er);
			return FWSRV_ER_INVALID_UPDATED_DATA;
		}
	}

	pCtrl->UpdFw.pFdtNvtStrgBasePath = xFwSrv_GetFdtStrgBasePath(p_fdt);
	if (pCtrl->UpdFw.pFdtNvtStrgBasePath == NULL) {
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}

	//enum known partition to check sanity
	np_enum_input.mem = mem_in;
	np_enum_input.p_user_data = (void *)p_fdt;
	np_enum_input.fp_enum = nvt_on_partition_enum_sanity;
	if(nvtpack_enum_partition(&np_enum_input) != NVTPACK_ER_SUCCESS)
	{
		printf("failed to check sanity.\r\n");
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}

	//enum known partition to update firmware
	np_enum_input.mem = mem_in;
	np_enum_input.p_user_data = (void *)p_fdt;
	np_enum_input.fp_enum = nvt_on_partition_enum_update;
	if(nvtpack_enum_partition(&np_enum_input) != NVTPACK_ER_SUCCESS)
	{
		printf("failed to update.\r\n");
		return FWSRV_ER_INVALID_UPDATED_DATA;
	}

	return FWSRV_ER_OK;
}


static void *gz_alloc(void *x, unsigned items, unsigned size)
{
	void *p = malloc(size * items);
	if (p) {
		memset(p, 0, size * items);
	}
	return p;
}

static void gz_free(void *x, void *addr, unsigned nb)
{
	free(addr);
}

THREAD_DECLARE(FastLoadTsk, args)
{
	int err;
	z_stream stream = {0};
	FWSRV_CTRL *pCtrl = xFwSrv_GetCtrl();
	FWSRV_PIPE_UNCOMP *pPipe = &pCtrl->PipeUnComp;

	THREAD_ENTRY();

	unsigned long prev_avail_in = 0;
	unsigned long stream_avail_in_cnt = 0;
	stream.next_in = (z_const Bytef *)pPipe->uiAddrCompress;
	stream.next_out = (z_const Bytef *)pPipe->uiAddrUnCompress;
	stream.avail_out = pPipe->uiSizeUnCompress;
	stream.zalloc = (alloc_func)gz_alloc;
	stream.zfree = (free_func)gz_free;
	stream.opaque = (voidpf)0;
	err = inflateInit(&stream);
	if (err != Z_OK) {
		DBG_ERR("Failed to inflateInit, err = %d\r\n", err);
		THREAD_RETURN(-1);
	}

	do {
		while (prev_avail_in + CFG_FWSRV_GZ_BLK_SIZE > pPipe->uiLoadedSize &&
			   pPipe->uiLoadedSize < pPipe->uiSizeCompress) {
			FLGPTN uiFlag;
			vos_flag_wait_interruptible(&uiFlag, pCtrl->Init.FlagID, FLGFWSRV_FASTLOAD_NEW, TWF_ORW | TWF_CLR);
		}

		if (prev_avail_in + CFG_FWSRV_GZ_BLK_SIZE > pPipe->uiSizeCompress) {
			prev_avail_in = pPipe->uiSizeCompress; //last data
		} else {
			prev_avail_in += CFG_FWSRV_GZ_BLK_SIZE;
		}

		clr_flg(pCtrl->Init.FlagID, FLGFWSRV_FASTLOAD_NEW);
		unsigned long incoming = prev_avail_in - stream_avail_in_cnt;
		stream.avail_in += incoming;
		stream_avail_in_cnt += incoming;
		err = inflate(&stream, Z_NO_FLUSH);
	} while (err == Z_OK);

	inflateEnd(&stream);

	if (err == Z_STREAM_END) {
		//set finish flag
		set_flg(pCtrl->Init.FlagID, FLGFWSRV_FASTLOAD_DONE);
	} else {
		DBG_ERR("Failed to finish inflate, err = %d\r\n", err);
	}

	THREAD_RETURN(0);
}

FWSRV_ER xFwSrv_CmdFastLoad(const FWSRV_CMD* pCmd)
{
	FWSRV_FASTLOAD *pData = (FWSRV_FASTLOAD *)pCmd->In.pData;
	MEM_RANGE *pMemRange = (MEM_RANGE *)pCmd->Out.pData;
	FWSRV_CTRL *pCtrl = xFwSrv_GetCtrl();

	UINT32 uiBlockSize;
	FWSRV_ER er = FWSRV_ER_OK;
	STORAGE_OBJ *pStrgFw = pData->pStrg;

	pStrgFw->Lock();
	pStrgFw->Open();
	pStrgFw->GetParam(STRG_GET_SECTOR_SIZE, (UINT32)&uiBlockSize, 0);

	// check sanity
	if (pData->MemComp.size < uiBlockSize) {
		DBG_ERR("MemComp.size not enough: %d\n", pData->MemComp.size);
		pStrgFw->Close();
		pStrgFw->Unlock();
		return FWSRV_ER_PARAM;
	}

	pStrgFw->RdSectors((INT8 *)pData->MemComp.addr, 0, 1);

	HEADER_BFC *pHeader = (HEADER_BFC *)pData->MemComp.addr;

	if (pHeader->uiFourCC != MAKEFOURCC('B', 'C', 'L', '1')) {
		DBG_ERR("ERROR BCF Header\r\n");
		er = FWSRV_ER_LINUX;
		goto FAILED;
	}

	FWSRV_PIPE_UNCOMP *pPipe = &pCtrl->PipeUnComp;
	pPipe->uiAddrCompress = ((UINT32)pHeader) + sizeof(HEADER_BFC);
	pPipe->uiSizeCompress = UINT32_SWAP(pHeader->uiSizeComp);
	pPipe->uiAddrUnCompress = pData->MemUnComp.addr;
	pPipe->uiSizeUnCompress = UINT32_SWAP(pHeader->uiSizeUnComp);
	pPipe->uiLoadedSize = uiBlockSize - sizeof(HEADER_BFC);
	UINT32 uiBlkCnt = ALIGN_CEIL(pPipe->uiSizeCompress+sizeof(HEADER_BFC), uiBlockSize) / uiBlockSize;
	UINT32 uiBlkMultiply = (uiBlockSize >= CFG_FWSRV_GZ_BLK_SIZE) ? 1 : CFG_FWSRV_GZ_BLK_SIZE / uiBlockSize;

	// check sanity
	UINT32 req_memcomp_size = ALIGN_CEIL(pPipe->uiSizeCompress+sizeof(HEADER_BFC), uiBlockSize);
	if (pData->MemComp.size < req_memcomp_size) {
		DBG_ERR("MemComp.size not enough: 0x%X, req: 0x%X\n", pData->MemComp.size, req_memcomp_size);
		er = FWSRV_ER_PARAM;
		goto FAILED;
	}
	UINT32 req_memuncomp_size = ALIGN_CEIL(pPipe->uiSizeUnCompress, uiBlockSize);
	if (pData->MemUnComp.size < req_memuncomp_size) {
		DBG_ERR("MemUnComp.size not enough: 0x%X, req: 0x%X\n", pData->MemUnComp.size, req_memuncomp_size);
		er = FWSRV_ER_PARAM;
		goto FAILED;
	}

	clr_flg(pCtrl->Init.FlagID, FLGFWSRV_FASTLOAD_NEW|FLGFWSRV_FASTLOAD_DONE);
	THREAD_HANDLE thread_fastload = vos_task_create(FastLoadTsk, NULL, "FastLoadTsk", 14, 2048);
	vos_task_resume(thread_fastload);

	UINT32 i;
	for (i = 1; i < uiBlkCnt; i += uiBlkMultiply) {
		pStrgFw->RdSectors(((INT8 *)pHeader) + i * uiBlockSize, i, uiBlkMultiply);
		pPipe->uiLoadedSize += uiBlockSize * uiBlkMultiply;
		set_flg(pCtrl->Init.FlagID, FLGFWSRV_FASTLOAD_NEW);
	}

	FLGPTN uiFlag;
	wai_flg(&uiFlag, pCtrl->Init.FlagID, FLGFWSRV_FASTLOAD_DONE, TWF_ORW | TWF_CLR);

	vos_cpu_dcache_sync((VOS_ADDR)pPipe->uiAddrUnCompress, pPipe->uiSizeUnCompress, VOS_DMA_FROM_DEVICE);
	pMemRange->addr = pPipe->uiAddrUnCompress;
	pMemRange->size = pPipe->uiSizeUnCompress;

FAILED:
	pStrgFw->Close();
	pStrgFw->Unlock();

	return er;
}
