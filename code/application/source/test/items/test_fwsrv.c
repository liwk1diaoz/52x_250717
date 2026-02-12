#include <stdio.h>
#include <stdlib.h>
#include <kwrap/examsys.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/cpu.h>
#include <comm/shm_info.h>
#include <strg_def.h>
#include <libfdt.h>
#include <compiler.h>
#include <rtosfdt.h>
#include <FwSrvApi.h>
#include <FileSysTsk.h>
#include <kwrap/util.h>
#include "sys_mempool.h"
#include "sys_fwload.h"
//#include <dma_protected.h>

#if defined(__FREERTOS)
#else
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#define THIS_DBGLVL  2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __MODULE__   fwsrv
#define __DBGLVL__   THIS_DBGLVL
#define __DBGFLT__   "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
unsigned int _DBGLVL_ = (unsigned int) - 1;

//#NT#PARTIAL_COMPRESS, we use last 10MB as working buffer
#define FW_PARTIAL_COMPRESS_WORK_BUF_SIZE 0xA00000

#if defined(_EMBMEM_NONE_)
#define EMB_GETSTRGOBJ(x) NULL //Always NULL
#define MAKE_FDT_PARTITION_PATH(x) "/null/partition_"#x
#elif defined(_EMBMEM_NAND_) || defined(_EMBMEM_SPI_NAND_)
#include "nand.h"
#define EMB_GETSTRGOBJ(x) nand_getStorageObject(x)
#define MAKE_FDT_PARTITION_PATH(x) "/nand/partition_"#x
#elif defined(_EMBMEM_SPI_NOR_)
#include "nand.h"
//#include "RamDisk/RamDisk.h"
#define EMB_GETSTRGOBJ(x) spiflash_getStorageObject(x)
#define MAKE_FDT_PARTITION_PATH(x) "/nor/partition_"#x
#elif defined(_EMBMEM_EMMC_)
#define EMB_GETSTRGOBJ(x) sdio2_getStorageObject(x)
#define MAKE_FDT_PARTITION_PATH(x) "/emmc/partition_"#x
#endif

#define SHMEM_PATH "/nvt_memory_cfg/shmem"
#define PARTITION_PATH_FDT      MAKE_FDT_PARTITION_PATH(fdt)
#define PARTITION_PATH_APP      MAKE_FDT_PARTITION_PATH(fdt.app)
#define PARTITION_PATH_UBOOT    MAKE_FDT_PARTITION_PATH(uboot)
#define PARTITION_PATH_RTOS     MAKE_FDT_PARTITION_PATH(rtos)
#define PARTITION_PATH_KERNEL   MAKE_FDT_PARTITION_PATH(kernel)
#define PARTITION_PATH_ROOTFS   MAKE_FDT_PARTITION_PATH(rootfs)
#define PARTITION_PATH_ROOTFS1  MAKE_FDT_PARTITION_PATH(rootfs1)
#define PARTITION_PATH_APPFS    MAKE_FDT_PARTITION_PATH(app)

#define STRG_OBJ_FW_FDT     STRG_OBJ_FW_RSV1
#define STRG_OBJ_FW_APP     STRG_OBJ_FW_RSV2
#define STRG_OBJ_FW_UBOOT   STRG_OBJ_FW_RSV3
#define STRG_OBJ_FW_RTOS    STRG_OBJ_FW_RSV4
#define STRG_OBJ_FW_KERNEL  STRG_OBJ_FW_RSV5
#define STRG_OBJ_FW_ROOTFS  STRG_OBJ_FW_RSV6
#define STRG_OBJ_FW_ROOTFS1 STRG_OBJ_FW_RSV7
#define STRG_OBJ_FW_APPFS   STRG_OBJ_FW_RSV8

#define IS_CMD(s1, s2) ((strncmp(s1, s2, strlen(s2)) == 0) ? TRUE : FALSE)

static FWSRV_INIT gPL_Init = {0};
static FWSRV_CMD gPL_Cmd = {0};
static FWSRV_PL_LOAD_BURST_IN gPL_In = {0};
//This array is sort by section id
UINT32 UserSection_Load[10] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//This array is sort by loading order
#if 1
UINT32 UserSection_Order_full[] = {
	CODE_SECTION_02, CODE_SECTION_03, CODE_SECTION_04, CODE_SECTION_05, CODE_SECTION_06,
	CODE_SECTION_07, CODE_SECTION_08, CODE_SECTION_09, CODE_SECTION_10, FWSRV_PL_BURST_END_TAG
};
#elif 0
UINT32 UserSection_Order_full[] = {
	CODE_SECTION_10, CODE_SECTION_09, CODE_SECTION_08, CODE_SECTION_07, CODE_SECTION_06,
	CODE_SECTION_05, CODE_SECTION_04, CODE_SECTION_03, CODE_SECTION_02, FWSRV_PL_BURST_END_TAG
};
#else
UINT32 UserSection_Order_full[] = {
	CODE_SECTION_10, CODE_SECTION_02, CODE_SECTION_09, CODE_SECTION_03, CODE_SECTION_08,
	CODE_SECTION_04, CODE_SECTION_07, CODE_SECTION_05, CODE_SECTION_06, FWSRV_PL_BURST_END_TAG
};
#endif

static void UserSection_LoadCb(const UINT32 Idx)
{
	DBG_DUMP("Section-%.2ld: (LOAD)\r\n", Idx + 1);
	UserSection_Load[Idx] = 1; //mark loaded
	fwload_set_done(Idx);
}

static int init_partition(STRG_OBJ_ID strg_id, const char *fdt_path)
{
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();

	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */

	UINT32 blksize;
	unsigned long long partition_ofs, partition_size;

	if (p_fdt == NULL) {
		DBG_ERR("p_fdt is NULL. \n");
		return -1;
	}

	nodeoffset = fdt_path_offset(p_fdt, fdt_path);
	if (nodeoffset < 0) {
		DBG_ERR("failed to offset for  %s = %d \n", fdt_path, nodeoffset);
		return -1;
	} else {
		DBG_DUMP("offset for  %s = %d \n", fdt_path, nodeoffset);
	}
	nodep = fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("failed to access reg.\n");
		return -1;
	} else {
		unsigned long long *p_data = (unsigned long long *)nodep;
		partition_ofs = be64_to_cpu(p_data[0]);
		partition_size = be64_to_cpu(p_data[1]);
		DBG_DUMP("partition reg = <0x%llX 0x%llX> \n", partition_ofs, partition_size);
	}


	STORAGE_OBJ *pStrg = EMB_GETSTRGOBJ(strg_id);
	if (pStrg == NULL) {
		DBG_ERR("pStrg is NULL\n");
		return -1;
	}
	pStrg->Open();
	pStrg->GetParam(STRG_GET_BEST_ACCESS_SIZE, (UINT32)&blksize, 0); //get block size
	if (blksize != _EMBMEM_BLK_SIZE_) {
		DBG_ERR("storage block size is not match. %X(driver) : %X(dts)\n", (int)blksize, (int)_EMBMEM_BLK_SIZE_);
		return -1;
	}
	pStrg->SetParam(STRG_SET_MEMORY_REGION, mempool_storage_nand, POOL_SIZE_STORAGE_NAND);
	pStrg->SetParam(STRG_SET_PARTITION_SECTORS, (UINT32)(partition_ofs / _EMBMEM_BLK_SIZE_), (UINT32)(partition_size / _EMBMEM_BLK_SIZE_));
	pStrg->Close();
	return 0;
}

static int fwsrv_open(void)
{
	SHMINFO *p_shm;
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();

	if (p_fdt == NULL) {
		DBG_ERR("p_fdt is NULL. \n");
		return -1;
	}

	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */

	// read SHMEM_PATH
	nodeoffset = fdt_path_offset(p_fdt, SHMEM_PATH);
	if (nodeoffset < 0) {
		DBG_ERR("failed to offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	} else {
		DBG_DUMP("offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	}
	nodep = fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("failed to access reg.\n");
		return 0;
	} else {
		unsigned int *p_data = (unsigned int *)nodep;
		p_shm = (SHMINFO *)be32_to_cpu(p_data[0]);
		DBG_DUMP("p_shm = 0x%08X\n", (int)p_shm);
	}

#if defined(__FREERTOS)
#if defined(_EMBMEM_SPI_NAND_)
	nand_setConfig(NAND_CONFIG_ID_NAND_TYPE, NANDCTRL_SPI_NAND_TYPE);
#elif defined(_EMBMEM_SPI_NOR_)
	nand_setConfig(NAND_CONFIG_ID_NAND_TYPE, NANDCTRL_SPI_NOR_TYPE);
#endif
#endif


#if defined(__FREERTOS)
	//setting partition info
	init_partition(STRG_OBJ_FW_FDT, PARTITION_PATH_FDT);
	init_partition(STRG_OBJ_FW_APP, PARTITION_PATH_APP);
	init_partition(STRG_OBJ_FW_UBOOT, PARTITION_PATH_UBOOT);
	init_partition(STRG_OBJ_FW_RTOS, PARTITION_PATH_RTOS);
#else
	init_partition(STRG_OBJ_FW_FDT, PARTITION_PATH_FDT);
	init_partition(STRG_OBJ_FW_UBOOT, PARTITION_PATH_UBOOT);
	init_partition(STRG_OBJ_FW_KERNEL, PARTITION_PATH_KERNEL);
	init_partition(STRG_OBJ_FW_ROOTFS, PARTITION_PATH_ROOTFS);
	init_partition(STRG_OBJ_FW_ROOTFS1, PARTITION_PATH_ROOTFS1);
	init_partition(STRG_OBJ_FW_APPFS, PARTITION_PATH_APPFS);
#endif


	gPL_Init.uiApiVer = FWSRV_API_VERSION;
	gPL_Init.StrgMap.pStrgFdt = EMB_GETSTRGOBJ(STRG_OBJ_FW_FDT);
	gPL_Init.StrgMap.pStrgApp = EMB_GETSTRGOBJ(STRG_OBJ_FW_APP);
	gPL_Init.StrgMap.pStrgUboot = EMB_GETSTRGOBJ(STRG_OBJ_FW_UBOOT);
	gPL_Init.StrgMap.pStrgRtos = EMB_GETSTRGOBJ(STRG_OBJ_FW_RTOS);
	gPL_Init.StrgMap.pStrgKernel= EMB_GETSTRGOBJ(STRG_OBJ_FW_KERNEL);
	gPL_Init.StrgMap.pStrgRootfs= EMB_GETSTRGOBJ(STRG_OBJ_FW_ROOTFS);
	gPL_Init.StrgMap.pStrgRootfs1 = EMB_GETSTRGOBJ(STRG_OBJ_FW_ROOTFS1);
	gPL_Init.StrgMap.pStrgAppfs = EMB_GETSTRGOBJ(STRG_OBJ_FW_APPFS);
	gPL_Init.PlInit.uiApiVer = PARTLOAD_API_VERSION;
	gPL_Init.PlInit.pStrg = EMB_GETSTRGOBJ(STRG_OBJ_FW_RTOS);
#if defined(_FW_TYPE_PARTIAL_COMPRESS_)
	gPL_Init.PlInit.DataType = PARTLOAD_DATA_TYPE_COMPRESS_GZ;
	gPL_Init.PlInit.uiWorkingAddr = (UINT32)malloc(FW_PARTIAL_COMPRESS_WORK_BUF_SIZE);
	gPL_Init.PlInit.uiWorkingSize = FW_PARTIAL_COMPRESS_WORK_BUF_SIZE ;
#else
	gPL_Init.PlInit.DataType = PARTLOAD_DATA_TYPE_UNCOMPRESS;
	gPL_Init.PlInit.uiWorkingAddr = (UINT32)malloc(_EMBMEM_BLK_SIZE_);
	gPL_Init.PlInit.uiWorkingSize = _EMBMEM_BLK_SIZE_ ;
#endif

#if defined(__FREERTOS)
	gPL_Init.PlInit.uiAddrBegin = _BOARD_RTOS_ADDR_ + p_shm->boot.LdLoadSize;
#else
	gPL_Init.PlInit.uiAddrBegin = 0;
#endif
	FwSrv_Init(&gPL_Init);
	FwSrv_Open();
	return 0;
}

static int fwsrv_close(void)
{
	FwSrv_Close();
	free((void *)gPL_Init.PlInit.uiWorkingAddr);
	return 0;
}

static int fwsrv_pl(void)
{
	void (*LoadCallback)(const UINT32 Idx) = UserSection_LoadCb;
	UINT32 *SecOrderTable = UserSection_Order_full;

	LoadCallback(CODE_SECTION_01); // 1st part has loaded by loader

	ER er;
	gPL_In.puiIdxSequence = SecOrderTable;
	gPL_In.fpLoadedCb = LoadCallback;
	gPL_Cmd.Idx = FWSRV_CMD_IDX_PL_LOAD_BURST; //continue load
	gPL_Cmd.In.pData = &gPL_In;
	gPL_Cmd.In.uiNumByte = sizeof(gPL_In);
	gPL_Cmd.Prop.bExitCmdFinish = TRUE;

	er = FwSrv_Cmd(&gPL_Cmd);
	if (er != FWSRV_ER_OK) {
		DBG_ERR("Process failed!\r\n");
	}
	return 0;
}

static int fwsrv_updfw(void)
{
	unsigned char *p_Mem = NULL;
	UINT32 uiSize = 0;

#if defined(__FREERTOS)
	INT32 fs_er;
	FST_FILE hFile = NULL;
	uiSize = FileSys_GetFileLen("A:\\FWSRV.bin");
	p_Mem = (unsigned char *)malloc(uiSize);

	hFile = FileSys_OpenFile("A:\\FWSRV.BIN", FST_OPEN_READ);
	fs_er = FileSys_ReadFile(hFile, (UINT8 *)p_Mem, &uiSize, 0, NULL);
	if (fs_er == FST_STA_OK) {
		fs_er = FileSys_CloseFile(hFile);
	}

	if (uiSize == 0 || fs_er != FST_STA_OK) {
		free(p_Mem);
		DBG_ERR("read file fail.\r\n");
		return -1;
	}
#else
	const char path[] = "/mnt/sd/FWSRV.BIN";
	struct stat st;
	if (stat(path, &st) != 0) {
		printf("failed to get %s, err = %d \n", path, errno);
		return -1;
	}

	p_Mem = (unsigned char *)malloc(st.st_size);
	int fd = open(path, O_RDONLY);
	read(fd, p_Mem, st.st_size);
	close(fd);
#endif


	FWSRV_BIN_UPDATE_ALL_IN_ONE Desc = {0};
	Desc.uiSrcBufAddr = (UINT32)p_Mem;
	Desc.uiSrcBufSize = (UINT32)uiSize;

	FWSRV_CMD Cmd = {0};
	Cmd.Idx = FWSRV_CMD_IDX_BIN_UPDATE_ALL_IN_ONE; //continue load
	Cmd.In.pData = &Desc;
	Cmd.In.uiNumByte = sizeof(Desc);
	Cmd.Prop.bExitCmdFinish = TRUE;

	ER er = FwSrv_Cmd(&Cmd);

	if (er != FWSRV_ER_OK) {
		free(p_Mem);
		DBG_ERR("Process failed!\r\n");
		return -1;
	}

	free(p_Mem);
	return 0;
}



static ID flag_compare = 0;
static BOOL gLoop = TRUE;
static unsigned char *mp_tail = NULL;
static unsigned char *mp_head = NULL;
static unsigned char *mp_end = NULL;
THREAD_DECLARE(thread_compare, p_param)
{
	vos_task_enter();

	while(gLoop) {
		FLGPTN flgptn;
		if (mp_head >= mp_tail || mp_head == NULL) {
			vos_flag_wait(&flgptn, flag_compare, 0x1, TWF_ORW | TWF_CLR);
		}
#if defined(__FREERTOS)
		unsigned char *p2 = (unsigned char*)dma_getNonCacheAddr(mp_head);
#else
		unsigned char *p2 = (unsigned char*)mp_head;
#endif
		if (mp_head == NULL) {
			printf("mp_head cannot be NULL.\n");
			break;
		}
		if(memcmp(mp_head, p2, _EMBMEM_BLK_SIZE_)) {
			UINT32 i;
			for (i = 0; i < _EMBMEM_BLK_SIZE_; i++) {
				if (p2[i] != mp_head[i]) {
					printf("%02X / %02X, ", p2[i], mp_head[i]);
				}
			}
			printf("\r\n");
		}
		mp_head += _EMBMEM_BLK_SIZE_;
		if (mp_head == mp_end) {
			mp_head = NULL;
		}
	}

	THREAD_RETURN(0);
}


static int fwsrv_flashcache(void)
{
	int read_size =  ALIGN_CEIL(0x500000, _EMBMEM_BLK_SIZE_);
	int alloc_size = read_size + 64;
	unsigned char *p_buf = (unsigned char *)malloc(alloc_size);
	unsigned char *p_aligned = (unsigned char*)ALIGN_CEIL_64((UINT32)p_buf);

	DBGH(p_aligned);

	mp_end = p_aligned + read_size;

	STORAGE_OBJ* pStrg = EMB_GETSTRGOBJ(STRG_OBJ_FW_RSV7);
	if (pStrg == NULL) {
		DBG_ERR("pStrg is NULL.\n");
		return -1;
	}

	vos_flag_create(&flag_compare, NULL, "flag_compare");
	vos_flag_clr(flag_compare, 0xFFFFFFFF);
 	VK_TASK_HANDLE tsk= vos_task_create(thread_compare,  p_aligned, "thread_compare",   20,	4096);
	vos_task_resume(tsk);

	pStrg->SetParam(STRG_SET_PARTITION_SECTORS, 0, (UINT32)(read_size/_EMBMEM_BLK_SIZE_));
	pStrg->Lock();
	pStrg->Open();

	int cnt = 0;
	while(cnt++ < 1000000000) {
		memset(p_aligned, 0x55, read_size);
		vos_cpu_dcache_sync((VOS_ADDR)p_aligned, read_size, VOS_DMA_BIDIRECTIONAL);

		UINT32 uiBlkCnt = read_size / _EMBMEM_BLK_SIZE_;
		UINT32 i;
		mp_tail = p_aligned;
		mp_head = mp_tail;
		for (i = 0; i < uiBlkCnt; i ++) {
			pStrg->RdSectors((INT8 *)mp_tail, i, 1);
			mp_tail += _EMBMEM_BLK_SIZE_;
			vos_flag_set(flag_compare, 0x1);
		}
		while(mp_head != NULL) {
			vos_util_delay_ms(1);
		}
	}

	gLoop = FALSE;

	pStrg->Close();
	pStrg->Unlock();

	free(p_buf);
	return 0;
}


EXAMFUNC_ENTRY(test_fwsrv, argc, argv)
{
#if 0 //quick test
	fwsrv_open();
	fwsrv_pl();
	vos_util_delay_ms(50000);
	fwsrv_updfw();
	fwsrv_close();
#else
	if (argc < 2) {
		printf("usage:\n");
		printf("\t %s open\n", argv[0]);
		printf("\t %s close\n", argv[0]);
		printf("\t %s pl\n", argv[0]);
		printf("\t %s updfw\n", argv[0]);
		return 0;
	}

	if (IS_CMD(argv[1], "open")) {
		fwsrv_open();
	} else if (IS_CMD(argv[1], "pl")) {
		fwsrv_pl();
	} else if (IS_CMD(argv[1], "close")) {
		fwsrv_close();
	} else if (IS_CMD(argv[1], "updfw")) {
		fwsrv_updfw();
	} else if (IS_CMD(argv[1], "cache")) {
		fwsrv_flashcache();
	} else {
		printf("unknown argument: %s\r\n", argv[1]);
	}
#endif
	return 0;
}
