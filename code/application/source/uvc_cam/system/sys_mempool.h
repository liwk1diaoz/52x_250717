#ifndef SYS_MEMPOOL_H
#define SYS_MEMPOOL_H
#include "umsd.h"
#include "msdcnvt/MsdcNvtApi.h"

#define POOL_SIZE_MSDCNVT      (ALIGN_CEIL_64(MSDCNVT_REQUIRE_MIN_SIZE))
//===========================================================================
//  User defined Mempool IDs
//===========================================================================
extern UINT32 mempool_msdcnvt;
extern UINT32 mempool_msdcnvt_pa;


extern void mempool_init(void);
#endif