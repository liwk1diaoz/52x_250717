#ifndef SYS_MEMPOOL_H
#define SYS_MEMPOOL_H
#include "umsd.h"
#include "msdcnvt/MsdcNvtApi.h"

#define POOL_SIZE_STORAGE_SDIO  ALIGN_CEIL_64(512)
#define POOL_SIZE_STORAGE_NAND  ALIGN_CEIL_32(_EMBMEM_BLK_SIZE_+(_EMBMEM_BLK_SIZE_>>2))
// R/W buf = 0xEC000 (FileSysInfo=32k, OpenFile=2K*N, BitMap=512k, Sectbuf1=128K, SectBuf2=128k, ScanBuf=128k, ResvBuf=8k, Total 944k = 0xEC000)
// FAT buf = 0x80020 (FatBuff=512k + 32bytes reserved = 0x80020)
#define POOL_SIZE_FILESYS      (ALIGN_CEIL_64(0xEC000)+ALIGN_CEIL_64(0x80020))
#define POOL_SIZE_MSDCNVT      (ALIGN_CEIL_64(MSDCNVT_REQUIRE_MIN_SIZE))
//===========================================================================
//  User defined Mempool IDs
//===========================================================================
extern UINT32 mempool_storage_sdio;
extern UINT32 mempool_storage_nand;
extern UINT32 mempool_filesys;
extern UINT32 mempool_msdcnvt;
extern UINT32 mempool_msdcnvt_pa;


extern void mempool_init(void);
#endif