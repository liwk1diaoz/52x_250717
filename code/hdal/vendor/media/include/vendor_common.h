
#ifndef __VENDOR_COMMON_H__
#define __VENDOR_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif
/********************************************************************
    INCLUDE FILES
 ********************************************************************/
#include "hdal.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
 ********************************************************************/
#define VENDOR_YUV420SP_TYPE_NV12           0
#define VENDOR_YUV420SP_TYPE_NV21           1
#define VENDOR_COMMON_PXLFMT_YUV420_TYPE    VENDOR_YUV420SP_TYPE_NV12   // it means 'HD_VIDEO_PXLFMT_YUV420' type in this platform

#define DDR_ID_AUTO                         0xF


typedef INT32         VENDOR_COMMON_MEM_VB_POOL;
#define VENDOR_COMMON_MEM_VB_INVALID_POOL           -1

/********************************************************************
    TYPE DEFINITION
 ********************************************************************/
typedef struct _VENDOR_LINUX_MEM_HOT_PLUG {
	UINT32                   start_addr;                           ///< the physical starting address
	UINT32                   size;                                 ///< the memory size
} VENDOR_LINUX_MEM_HOT_PLUG;

typedef struct _VENDOR_COMM_BRIDGE_MEM {
	UINT32                   phys_addr;                            ///< the physical starting address
	UINT32                   size;                                 ///< the memory size
} VENDOR_COMM_BRIDGE_MEM;

typedef struct _VENDOR_COMM_MAX_FREE_BLOCK {
	HD_COMMON_MEM_DDR_ID     ddr;                                  ///< [in]  the ddr id
	UINT32                   size;                                 ///< [out] the maximum block size
} VENDOR_COMM_MAX_FREE_BLOCK;


typedef struct _VENDOR_COMM_POOL_RANGE {
	HD_COMMON_MEM_DDR_ID     ddr;                                  ///< [in]  the ddr id
	UINT32                   phys_addr;                            ///< [out] the physical addr of pool start
	UINT32                   size;                                 ///< [out] the total pool size
} VENDOR_COMM_POOL_RANGE;

typedef struct _VENDOR_COMM_DDR_MONITOR_ID {
	HD_COMMON_MEM_DDR_ID     ddr;                                  ///< [in]  the ddr id
} VENDOR_COMM_DDR_MONITOR_ID;

typedef struct _VENDOR_COMM_DDR_MONITOR_DATA {
	HD_COMMON_MEM_DDR_ID     ddr;                                  ///< [in]  the ddr id
	UINT64                   cnt;                                  ///< [out]: the total ddr data count
	UINT64                   byte;                                 ///< [out]: the total ddr data bytes
} VENDOR_COMM_DDR_MONITOR_DATA;

typedef struct _VENDOR_COMM_FREE_SIZE {
	HD_COMMON_MEM_DDR_ID     ddr;                                  ///< [in]  the ddr id
	UINT32                   size;                                 ///< [out] the free size
} VENDOR_COMM_FREE_SIZE;

typedef struct _VENDOR_COMM_PA_TO_BLK {
	UINT32                   pa;                                   ///< [in]  the physical starting address
	HD_COMMON_MEM_VB_BLK     blk;                                  ///< [out] the blk handle
} VENDOR_COMM_PA_TO_BLK;

typedef enum _VENDOR_COMMON_MEM_ITEM {
	VENDOR_COMMON_MEM_ITEM_LINUX_HOT_PLUG,           ///< hot plug memory to linux high memory zone, the parameter please refer VENDOR_LINUX_MEM_HOT_PLUG.
	VENDOR_COMMON_MEM_ITEM_BRIDGE_MEM,               ///< get bridge memory region described on fdt, the parameter please refer VENDOR_COMM_BRIDGE_MEM.
	VENDOR_COMMON_MEM_ITEM_MAX_FREE_BLOCK_SIZE,      ///< get current maximum free block size of hdal, the parameter please refer VENDOR_COMM_MAX_FREE_BLOCK.
	VENDOR_COMMON_MEM_ITEM_COMM_POOL_RANGE,          ///< get common pool range, the parameter please refer VENDOR_COMM_POOL_RANGE.
	VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_START,        ///< start ddr monitor, the parameter please refer VENDOR_COMM_DDR_MONITOR_ID.
	VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_STOP,         ///< stop ddr monitor, the parameter please refer VENDOR_COMM_DDR_MONITOR_ID.
	VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_RESET,        ///< reset ddr monitor, the parameter please refer VENDOR_COMM_DDR_MONITOR_ID.
	VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA,         ///< get ddr monitor data count, the parameter please refer VENDOR_COMM_DDR_MONITOR_DATA.
	VENDOR_COMMON_MEM_ITEM_FREE_SIZE,                ///< get current free size of hdal, the parameter please refer VENDOR_COMM_FREE_SIZE.
	VENDOR_COMMON_MEM_ITEM_PA_TO_BLK,                ///< get block by pa
   	ENUM_DUMMY4WORD(VENDOR_COMMON_MEM_ITEM)
} VENDOR_COMMON_MEM_ITEM;


typedef enum _VENDOR_COMMON_MEM_DMA_DIR {
	VENDOR_COMMON_MEM_DMA_BIDIRECTIONAL,             ///< it means flush operation.
	VENDOR_COMMON_MEM_DMA_TO_DEVICE,                 ///< it means clean operation.
	VENDOR_COMMON_MEM_DMA_FROM_DEVICE,               ///< it means invalidate operation.
	ENUM_DUMMY4WORD(VENDOR_COMMON_MEM_DMA_DIR)
} VENDOR_COMMON_MEM_DMA_DIR;



/**
	Set some config settings in the user state.

	@param item:  the parameter ID.
	@param p_param: the parameters.

    @return
    	- @b  HD_OK: Success.
        - @b  < 0:   Some error happened.

	Example:
	@code
	{
		VENDOR_LINUX_MEM_HOT_PLUG  mem_hotplug = {0};

		mem_hotplug.start_addr = 0x40000100;
		mem_hotplug.size = 0x20000000;
		vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_LINUX_HOT_PLUG, (VOID *)&mem_hotplug);
	}
	@endcode
*/
HD_RESULT vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM item, VOID *p_param);

/**
	Get some config settings in the user state.

	@param item:  the parameter ID.
	@param p_param: the parameters.

    @return
		- @b  HD_OK: Success.
        - @b  < 0:   Some error happened.

	Example:
	@code
	{
		VENDOR_COMM_BRIDGE_MEM  bridge_mem = {0};
		vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_BRIDGE_MEM, &bridge_mem);
	}
	@endcode
*/
HD_RESULT vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM item, VOID *p_param);


/**
	Allocates the fixed memory in the user state and the allocated memory can not be freed.
	The API should be called before hd_common_mem_init();

	@note the allocated memory is continuous and cacheable.

	@param name:      [in]  the name to describe this memory.
	@param phy_addr:  [out] the physical address returned.
	@param virt_addr: [out] the virtual address returned.
	@param size:      [in]  the continuous memory size want to alloc.
	@param ddr:       [in]  ddr id.

    @return
    	- @b  HD_OK: Success.
        - @b  < 0:   Some error happened.

	Example:
	@code
	{
		void                 *va;
		UINT32               pa;
		HD_RESULT            ret;

		ret = vendor_common_mem_alloc_fixed_pool("sdio", &pa, (void **)&va, 0x100000, DDR_ID0);
		if (ret != HD_OK) {
			return;
		}
	}
	@endcode
*/
HD_RESULT vendor_common_mem_alloc_fixed_pool(CHAR* name, UINT32 *phy_addr, void **virt_addr, UINT32 size, HD_COMMON_MEM_DDR_ID ddr);

/**
    Re-layout video buffer common pools.

    @param p_mem_config: The video buffer common pools layout.

    @return HD_OK for success, < 0 when some error happened.

    Example:
    @code
    {
	 	HD_RESULT                 ret;
		HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

		mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[0].blk_size = 0x200000;
		mem_cfg.pool_info[0].blk_cnt = 3;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
		mem_cfg.pool_info[1].type = HD_COMMON_MEM_OSG_POOL;
		mem_cfg.pool_info[1].blk_size = 0x200000;
		mem_cfg.pool_info[1].blk_cnt = 3;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;

		ret = vendor_common_mem_relayout(&mem_cfg);
		if (HD_OK != ret) {
			printf("vendor_common_mem_relayout err: %d\r\n", ret);
		}
    }
    @endcode
*/
HD_RESULT vendor_common_mem_relayout(HD_COMMON_MEM_INIT_CONFIG *p_mem_config);

/**
    Do the cache memory data synchronization.

    @param virt_addr: the cache memory address.
    @param size: the memory size want to sync.
    @param dir: the dma direction.

    @return HD_OK for success, < 0 when some error happened.
*/
HD_RESULT vendor_common_mem_cache_sync(void* virt_addr, unsigned int size, VENDOR_COMMON_MEM_DMA_DIR dir);


/**
    Do the cache memory data synchronization of the whole cache.

    @note User must make sure the cache data is only touched by one specified cpu_id.
          This API will get better performance on SMP when flush large size.
          If the task was bouned to one cpu, it will call cache_flush_all() API to fine tune performace.
          If the task was not bouned to one cpu, it will call cache_flush_range() API.

    @param virt_addr: the cache memory address.
    @param size: the memory size want to sync.
    @param dir: the dma direction.

    @return HD_OK for success, < 0 when some error happened.
*/
HD_RESULT vendor_common_mem_cache_sync_all(void* virt_addr, unsigned int size, VENDOR_COMMON_MEM_DMA_DIR dir);
/**
    Create a user private pool.

    @param pool_name: The pool name.
    @param blk_size: the block size.
    @param blk_cnt: the block count.
    @param ddr: DDR1 or DDR2 ...

    @return the pool handle, VENDOR_COMMON_MEM_VB_INVALID_POOL means create pool fail.

    Example:
    @code
    {
		HD_COMMON_MEM_VB_BLK      blk;
		UINT32                    blk_size = 0x200000, blk_cnt = 2;
		HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
		VENDOR_COMMON_MEM_VB_POOL pool;

		pool = vendor_common_mem_create_pool("test_c", blk_size, blk_cnt, ddr_id);
		if (pool == VENDOR_COMMON_MEM_VB_INVALID_POOL) {
			printf("create pool fail\r\n");
		}
		blk = hd_common_mem_get_block(pool, blk_size, ddr_id);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail\r\n");
			return HD_ERR_NG;
		}
    }
    @endcode
*/
VENDOR_COMMON_MEM_VB_POOL vendor_common_mem_create_pool(CHAR *pool_name, UINT32 blk_size, UINT32 blk_cnt, HD_COMMON_MEM_DDR_ID ddr);

/**
    Destroy a user private pool.

    @param pool: the pool handle that returned from create.

    @return HD_OK for success, < 0 when some error happened.

    Example:
    @code
    {
		vendor_common_mem_destroy_pool(pool);
    }
    @endcode
*/
HD_RESULT vendor_common_mem_destroy_pool(VENDOR_COMMON_MEM_VB_POOL pool);


/**
    Lock the video buffer block.

    This API will cause video buffer block reference count+1.

    @param blk: the block handle.

    @return HD_OK for success, < 0 when some error happened.

    Example:
    @code
    {
		HD_RESULT          ret;

		ret = vendor_common_mem_lock_block(g_blk);
		if (HD_OK != ret) {
			printf("lock blk fail %d\r\n", ret);
			return FALSE;
		}
    }
    @endcode
*/
HD_RESULT vendor_common_mem_lock_block(HD_COMMON_MEM_VB_BLK blk);
#ifdef __cplusplus
}
#endif

#endif // __VENDOR_VIDEOOUT_H__

