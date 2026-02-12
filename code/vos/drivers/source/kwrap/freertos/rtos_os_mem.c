/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#define __MODULE__    rtos_mem
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/cpu.h>
#include <kwrap/mem.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

#include "freertos_ext_kdrv.h"

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_mem_debug_level = NVT_DBG_WRN;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void rtos_mem_init(void *param)
{
	STATIC_ASSERT(sizeof(UINT32) == sizeof((void *)0)); //make sure the address size is correct
	STATIC_ASSERT(sizeof(UINT32) == sizeof(size_t)); //make sure the address size is correct
}

void rtos_mem_exit(void)
{
}

int vos_mem_init_cma_info(struct vos_mem_cma_info_t *p_info, VOS_MEM_CMA_TYPE type, UINT32 size)
{
	if (NULL == p_info) {
		DBG_ERR("p_info is NULL\r\n");
		return -1;
	}

	if (type != VOS_MEM_CMA_TYPE_CACHE) {
		DBG_ERR("Only support cache type\r\n");
		return -1;
	}

	memset(p_info, 0, sizeof(struct vos_mem_cma_info_t));

	p_info->size = size;
	p_info->type = type;

	return 0;
}

VOS_MEM_CMA_HDL vos_mem_alloc_from_cma(struct vos_mem_cma_info_t *p_info)
{
	void *p_alloc;

	if (NULL == p_info) {
		DBG_ERR("p_info is NULL\r\n");
		return NULL;
	}

	if (0 == p_info->size) {
		DBG_ERR("size is 0\r\n");
		return NULL;
	}

	if (p_info->type != VOS_MEM_CMA_TYPE_CACHE) {
		DBG_ERR("Only support cache type\r\n");
		return NULL;
	}

	p_alloc = malloc(p_info->size);
	if (NULL == p_alloc) {
		DBG_ERR("malloc %d failed\r\n", p_info->size);
		return NULL;
	}
	p_info->paddr = vos_cpu_get_phy_addr((VOS_ADDR)p_alloc);
	p_info->vaddr = (UINT32)p_alloc;

	return (VOS_MEM_CMA_HDL)p_alloc;
}

int vos_mem_release_from_cma(VOS_MEM_CMA_HDL p_hdl)
{
	if (NULL == p_hdl) {
		DBG_ERR("p_hdl is NULL\r\n");
		return -1;
	}

	free((void *)p_hdl);

	return 0;
}

