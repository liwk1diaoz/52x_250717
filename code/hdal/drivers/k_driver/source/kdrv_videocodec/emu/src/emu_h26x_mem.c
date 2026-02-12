#include "string.h"
#include "h26x_def.h"

//#include "emu_h26xMain.h"
#include "emu_h26x_mem.h"
#include "kdrv_vdocdc_dbg.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "h26x.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#if 0
static DMA_WRITEPROT_ATTR  gH26XProtectAttr = {0};
static UINT8               gH26XProtectSet = 2;

#endif


static UINT8    		   gH26XProtectEn[WP_DDR_NUM][WP_CHN_NUM] = {0};

static BOOL    		   	   gH26XWPEn = 0;
static BOOL				   gH26XDram2En = 0;
void h26x_enable_dram2(void){
	gH26XDram2En = 1;
}
void h26x_enable_wp(void){
	gH26XWPEn = 1;
}
unsigned int h26x_mem_malloc(h26x_mem_t *p_mem, unsigned int size)
{
	unsigned int addr = p_mem->addr;

	if(addr != SIZE_32X(addr)){
		DBG_ERR("h26x_mem_malloc addr (%08x, %08x)\r\n", (unsigned int)addr, (unsigned int)SIZE_32X(addr));
	}

	if(gH26XDram2En){
		BOOL switch_dram = rand()%2;
		addr = switch_dram ?  (addr < 0x80000000 ? (addr + 0x40000000) : (addr - 0x40000000)) : addr;
	}
	size = SIZE_32X(size);


	if(gH26XWPEn){
		int tmp = rand()%30;
		if(!tmp){
			h26x_test_wp(rand()%WP_CHN_NUM, addr + size ,  32, rand()%4 );
			size += 32;
		}
	}


	if (p_mem->size < size)
	{
		DBG_ERR("memory not enough (%08x, %08x)\r\n", (int)p_mem->size, (int)size);
		return 0;
	}
	else
	{
		p_mem->addr += size;
		p_mem->size -= size;
	}
    h26x_flushCache(addr, size);
	return addr;
}

void h26x_test_wp(UINT8 wp_idx, UINT32 start_addr, UINT32 size, UINT32 level){
#if 0
	gH26XProtectAttr.chEnMask.bH264_0 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_1 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_3 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_4 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_5 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_6 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_7 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_8 = TRUE; // bH264_0 ~ bH264_7
	gH26XProtectAttr.chEnMask.bH264_9 = TRUE; // bH264_0 ~ bH264_7
#if 1
	gH26XProtectAttr.uiProtectlel = DMA_WPLEL_DETECT; // DETECT
#elif 0
	gH26XProtectAttr.uiProtectlel = DMA_WPLEL_UNWRITE;
#elif 1
    gH26XProtectAttr.uiProtectlel = DMA_RPLEL_UNREAD;
#endif

    DBG_MSG("uiProtectlel = %d\r\n",gH26XProtectAttr.uiProtectlel);

    gH26XProtectSet = wp_idx;
    gH26XProtectAttr.uiStartingAddr = start_addr;
    gH26XProtectAttr.uiSize       = size;
    gH26XProtectAttr.uiSize      &= 0xFFFFFFF0; // Two Word-alignment
    DBG_MSG(("set_wp ch %d addr 0x%08x, size 0x%08x\r\n", gH26XProtectSet, gH26XProtectAttr.uiStartingAddr, gH26XProtectAttr.uiSize));
    dma_configWPFunc(gH26XProtectSet, &gH26XProtectAttr, NULL);
    dma_enableWPFunc(gH26XProtectSet);
	gH26XProtectEn[gH26XProtectSet] = 1;
#else
	DMA_WRITEPROT_ATTR attrib = {0};


	#if 1
	attrib.mask.H264_0 = 1;
	attrib.mask.H264_1 = 1;
	attrib.mask.H264_3 = 1;
	attrib.mask.H264_4 = 1;
	attrib.mask.H264_5 = 1;
	attrib.mask.H264_6 = 1;
	attrib.mask.H264_7 = 1;
	attrib.mask.H264_8 = 1;
	attrib.mask.H264_9 = 1;

	#if 0
	//attrib.level = DMA_WPLEL_DETECT; // DETECT
	#elif 1
	//attrib.level = DMA_WPLEL_UNWRITE;
	#elif 1
    //attrib.level = DMA_RPLEL_UNREAD;
	#elif 1
	//attrib.level = DMA_RWPLEL_UNRW;
	#endif

	attrib.level = level;
	attrib.protect_mode = DMA_PROT_IN; //DMA_PROT_OUT
	#endif



	#if 0
	attrib.mask.H264_1 = 1;
	attrib.level = DMA_RPLEL_UNREAD;
	attrib.protect_mode = DMA_PROT_IN; //DMA_PROT_OUT
	#endif



	attrib.protect_rgn_attr[0].en = 1;
	attrib.protect_rgn_attr[0].starting_addr = start_addr;
	attrib.protect_rgn_attr[0].size = size;


	if(start_addr>=0 && start_addr<0x20000000){
		arb_enable_wp(DDR_ARB_1, wp_idx, &attrib);
		gH26XProtectEn[0][wp_idx] = 1;


		DBG_INFO("dram1 set_wp ch %d addr 0x%08x, size 0x%08x\r\n", (unsigned int)wp_idx,(unsigned int) start_addr, (unsigned int)size);


	}
	else if(start_addr>=0x40000000 && start_addr<0x60000000){
		arb_enable_wp(DDR_ARB_2, wp_idx, &attrib);
		gH26XProtectEn[1][wp_idx] = 1;

		DBG_INFO("dram2 set_wp ch %d addr 0x%08x, size 0x%08x\r\n", (unsigned int)wp_idx,(unsigned int) start_addr, (unsigned int)size);

	}
	else{
		DBG_ERR("write protect config error!\r\n");
	}
#endif
}
void h26x_test_wp_2(UINT8 wp_idx, UINT32 start_addr, UINT32 size, UINT32 level){

	DMA_WRITEPROT_ATTR attrib = {0};
	attrib.mask.H264_0 = 1;
	attrib.mask.H264_1 = 1;
	attrib.mask.H264_3 = 1;
	attrib.mask.H264_4 = 1;
	attrib.mask.H264_5 = 1;
	attrib.mask.H264_6 = 1;
	attrib.mask.H264_7 = 1;
	attrib.mask.H264_8 = 1;
	attrib.mask.H264_9 = 1;

	#if 0
	attrib.level = DMA_WPLEL_DETECT; // DETECT
	#elif 1
	attrib.level = DMA_WPLEL_UNWRITE;
	#elif 1
    attrib.level = DMA_RPLEL_UNREAD;
	#elif 1
	attrib.level = DMA_RWPLEL_UNRW;
	#endif
	attrib.level = level;

	attrib.protect_mode = DMA_PROT_IN; //DMA_PROT_OUT

	attrib.protect_rgn_attr[0].en = 1;
	attrib.protect_rgn_attr[0].starting_addr = start_addr;
	attrib.protect_rgn_attr[0].size = size;


	if(start_addr>0 && start_addr<0x20000000){
		arb_enable_wp(DDR_ARB_1, wp_idx, &attrib);
		gH26XProtectEn[0][wp_idx] = 1;


		DBG_INFO("dram1 set_wp ch %d , level %d, addr 0x%08x, size 0x%08x\r\n", (unsigned int)wp_idx, (unsigned int)level,(unsigned int) start_addr, (unsigned int)size);


	}
	else if(start_addr>0x40000000 && start_addr<0x60000000){
		arb_enable_wp(DDR_ARB_2, wp_idx, &attrib);
		gH26XProtectEn[1][wp_idx] = 1;

		DBG_INFO("!1!1!1dram2 set_wp ch %d addr 0x%08x, size 0x%08x\r\n", (unsigned int)wp_idx,(unsigned int) start_addr, (unsigned int)size);

	}
	else{
		DBG_ERR("write protect config error!\r\n");
	}
}
void h26x_disable_wp(void){
#if 0
	int i;
	for(i=0;i<WP_CHN_NUM;i++){
		if(gH26XProtectEn[i]){
			dma_disableWPFunc(i);
			gH26XProtectEn[i] = 0;
		}
	}
#else
	int i;
	for(i=0;i<WP_CHN_NUM;i++){
		if(gH26XProtectEn[0][i]){
			arb_disable_wp(DDR_ARB_1,i);
			gH26XProtectEn[0][i] = 0;
		}
		if(gH26XProtectEn[1][i]){
			arb_disable_wp(DDR_ARB_2,i);
			gH26XProtectEn[1][i] = 0;
		}
	}
#endif
}
//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

