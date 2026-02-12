#include "string.h"
#include "emu_h26x_common.h"
#include "emu_h264_enc.h"
#include "emu_h265_enc.h"
#include "emu_h264_dec.h"
#include "emu_h265_dec.h"

#include "../../include/kdrv_videoenc/kdrv_videoenc.h"
#include "../../include/kdrv_videodec/kdrv_videodec.h"
#include "kdrv_vdocdc_dbg.h"


#include <kwrap/mem.h>

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)
#if 0
extern char end[];
#define POOL_ID_APP_ARBIT		0
#define POOL_ID_APP_ARBIT2		1

static UINT32	OS_GetMempoolAddr(UINT32 id)
{
	UINT32 addr;

	addr = (UINT32)&end;

//	emu_msg(("OS_GetMempoolAddr = 0x%08x\r\n", (unsigned int)addr));
	if(id == POOL_ID_APP_ARBIT)
		addr = ((addr + 0x100000)&~0xFFFFF);
	else
		addr = 0xa0000000;
	return addr;
}

static UINT32 OS_GetMempoolSize(UINT32 id)
{
	UINT32 	size;
	UINT32	addr;

	UINT32 dram1_size;
	UINT32 dram2_size;

	addr = (UINT32)&end;

	dram1_size = dma_getDramCapacity(DMA_ID_1);
	dram2_size = dma_getDramCapacity(DMA_ID_2);

//emu_msg(("OS_GetMempoolSize = [1]0x%08x [2]0x%08x\r\n", (unsigned int)dram1_size, (unsigned int)dram2_size));
	if(id == POOL_ID_APP_ARBIT) {
		addr = ((addr + 0x100000)&~0xFFFFF);
		size = (dram1_size - addr);
	}
	else {
		addr = 0xa0000000;
		size = dram2_size;
	}
	return size;
}
#endif



void emu_h26xMain_core(UINT32 ddr_addr1, UINT32 ddr_size1, UINT32 ddr_addr2, UINT32 ddr_size2, UINT32 item)
{

    h26x_mem_t mem;
    h26x_mem_t mem2;
    h26x_ctrl_t ctrl;
    h26x_srcd2d_t src_d2d[H26X_JOB_MAX] = {0};


	DBG_DUMP("%s %d\r\n",__FUNCTION__,__LINE__);

#if 0
    mem.start = 0x7700000;//OS_GetMempoolAddr(POOL_ID_APP);
    mem.addr  = mem.start;
    mem.size  = 0x02000000;//OS_GetMempoolSize(POOL_ID_APP);

    mem2.start = 0x7700000;//OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
    mem2.addr = mem2.start;
    mem2.size = 0x2000000;//OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
#elif 0
    struct vos_mem_cma_info_t cma_info = {0};
    VOS_MEM_CMA_HDL cma_hdl;
    int malloc_size = 0x02000000;

    if (0 != vos_mem_init_cma_info(&cma_info, VOS_MEM_CMA_TYPE_CACHE, malloc_size)) {
        DBG_DUMP("init cma failed\r\n");
    }

    cma_hdl = vos_mem_alloc_from_cma(&cma_info);
    if (NULL == cma_hdl) {
        DBG_DUMP("alloc cma failed\r\n");
    }
    DBG_DUMP("cma_info.paddr 0x%X\r\n", (int)cma_info.paddr);
    DBG_DUMP("cma_info.vaddr 0x%X\r\n", (int)cma_info.vaddr);
    DBG_DUMP("cma_info.size 0x%X\r\n", (int)cma_info.size);

    //if ((unsigned int)vos_cpu_get_phy_addr(cma_info.vaddr) != (unsigned int)cma_info.paddr) {
    //    DBG_DUMP("phy_addr conv failed, 0x%X != 0x%X\r\n", vos_cpu_get_phy_addr(cma_info.vaddr), (int)cma_info.paddr);
    //}

    mem.start = cma_info.vaddr;//OS_GetMempoolAddr(POOL_ID_APP);
    mem.addr  = mem.start;
    mem.size  = malloc_size;//OS_GetMempoolSize(POOL_ID_APP);

    mem2.start = mem.start;//OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
    mem2.addr = mem2.start;
    mem2.size = malloc_size;//OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
#else
    mem.start = ddr_addr1;//OS_GetMempoolAddr(DDR_ID0);
    mem.addr  = mem.start;
    mem.size  = ddr_size1;//OS_GetMempoolSize(DDR_ID0);

    mem2.start = ddr_addr2;
    mem2.addr  = mem2.start;
    mem2.size  = ddr_size2;
#endif

    DBG_DUMP("dram1 mem = 0x%08x size = 0x%08x\r\n",(int)mem.addr,(int)mem.size);
    DBG_DUMP("dram2 mem = 0x%08x size = 0x%08x\r\n",(int)mem2.addr,(int)mem2.size);

    h26x_job_ctrl_init(&ctrl, &mem, &mem2);
	//DBG_DUMP("%s %d\r\n",__FUNCTION__,__LINE__);



	h26x_request_irq();
    h26x_create_resource();

    h26x_setAPBAddr(0xf0a10000);

    h26x_setRstAddr(0xf0020080);

    h26x_setClk(240);

    h26x_open();

	if(item == 1){
		emu_h265_setup(&ctrl,NULL, &src_d2d[0], NULL);
	}else if(item == 2){
        emu_h265d_setup(&ctrl);
	}else if(item == 3){
		emu_h265d_setup(&ctrl);

        emu_h265_setup(&ctrl,NULL, &src_d2d[0], NULL);

	}else if(item == 4){
        emu_h264_setup(&ctrl,NULL, &src_d2d[0]);
		emu_h264d_setup(&ctrl);
		emu_h265_setup(&ctrl,NULL, &src_d2d[0], NULL);
		emu_h265d_setup(&ctrl);
	}else if(item == 5){
        emu_h264_setup(&ctrl,NULL, &src_d2d[0]);
    }else if(item == 6){
        emu_h264d_setup(&ctrl);
    }else if(item == 7){
        emu_h264_setup(&ctrl,NULL, &src_d2d[0]);
        emu_h264d_setup(&ctrl);
    }else if(item == 8){
        emu_h264_setup(&ctrl,NULL, &src_d2d[0]);
        emu_h265_setup(&ctrl,NULL, &src_d2d[0], NULL);
    }else if(item == 9){
        emu_h264d_setup(&ctrl);
        emu_h265d_setup(&ctrl);
    }

	//DBG_DUMP("%s %d\r\n",__FUNCTION__,__LINE__);
    h26x_job_excute(&ctrl, &src_d2d[0]);

	//DBG_DUMP("%s %d\r\n",__FUNCTION__,__LINE__);
}


INT32 kdrv_videodec_set(UINT32 id, KDRV_VDODEC_SET_PARAM_ID parm_id, VOID *param)
{
    return 0;
}
INT32 kdrv_videodec_get(UINT32 id, KDRV_VDODEC_GET_PARAM_ID parm_id, VOID *param)
{
    return 0;
}
INT32 kdrv_videodec_close(UINT32 chip, UINT32 engine)
{
    return 0;

}
INT32 kdrv_videoenc_set(UINT32 id, KDRV_VDOENC_SET_PARAM_ID parm_id, VOID *param)
{
    return 0;

}
INT32 kdrv_videoenc_open(UINT32 chip, UINT32 engine)
{
    return 0;
}
INT32 kdrv_videoenc_close(UINT32 chip, UINT32 engine)
{
    return 0;
}

INT32 kdrv_videodec_trigger(UINT32 id, KDRV_VDODEC_PARAM *p_dec_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
    return 0;
}
INT32 kdrv_videodec_open(UINT32 chip, UINT32 engine)
{
    return 0;
}
INT32 kdrv_videoenc_trigger(UINT32 id, KDRV_VDOENC_PARAM *p_enc_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
    return 0;
}
INT32 kdrv_videoenc_get(UINT32 id, KDRV_VDOENC_GET_PARAM_ID parm_id, VOID *param)
{
    return 0;
}

//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

