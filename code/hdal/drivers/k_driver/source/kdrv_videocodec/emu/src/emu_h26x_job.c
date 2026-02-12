#include "stdlib.h"
#include "string.h"
#include "time.h"

#include "h26x.h"
#include "h26x_def.h"

#include "emu_h26x_job.h"

#include "emu_h264_enc.h"
#include "emu_h265_enc.h"
#include "emu_h264_dec.h"
#include "emu_h265_dec.h"
//#include "h26x_common.h"
#include "kdrv_vdocdc_dbg.h"
#include "rtos_na51055/pll.h"

//#include "pll.h"

//#include  "DRAMConsumeTsk.h"

#include "comm/ddr_arb.h" //heavy laod


//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define SRC_OUT_ONLY 0 //need enable rec_out_dis and src_out_en
int ddrburst = 0;

extern void save_to_reg(UINT32 *reg, INT32 val, UINT32 b_addr, UINT32 bits);
extern BOOL ipp_verf_low_delay_en;
#if 0
static UINT32  uiDegreeTable[7] = {
	DRAM_CONSUME_EASY_LOADING,
	DRAM_CONSUME_NORMAL_LOADING,
	DRAM_CONSUME_HEAVY_LOADING,
	DRAM_CONSUME_HEAVY_LOADING,
	DRAM_CONSUME_HEAVY_LOADING,
	DRAM_CONSUME_HEAVY_LOADING,
	DRAM_CONSUME_DISABLE_DMA_CHANNEL
};
static UINT32   uiPriorityTable[3] = {DMA_PRIORITY_LOW, DMA_PRIORITY_MIDDLE, DMA_PRIORITY_HIGH};
static UINT8   *cDegreeName[7] = {
	(UINT8 *)"easy", (UINT8 *)"normal", (UINT8 *)"heavy", (UINT8 *)"heavy", (UINT8 *)"heavy", (UINT8 *)"heavy", (UINT8 *)"disable"
};
static UINT8   *cPriorityName[3] = {(UINT8 *)"low", (UINT8 *)"middle", (UINT8 *)"high"};


// return IsDisableChannel
BOOL h26XEmuSetTestBandWidth(UINT32 uiAddr, UINT32 uiSize, UINT32 uiSelect, BOOL bTestDisable)
{

	DRAM_CONSUME_ATTR conAttr;
	UINT32  uiDegree, uiPriorityLevel;
	UINT32  uiDegreeCnt = 6;
	BOOL    bIsDisable = 0;

	if (uiSelect < 0) {
		return 0;
	}

#if 1
	conAttr.ubAssignMem = FALSE;
	conAttr.ubDispDMAUtil = TRUE;
	conAttr.uiAddr = 0;
	conAttr.uiSize = 0;
	conAttr.uiTimeingInterval = 3000;
	//conAttr.uiTimeingInterval = 256;
#else
	conAttr.ubAssignMem = TRUE;
	conAttr.ubDispDMAUtil = TRUE;
	conAttr.uiAddr = uiAddr;//(UINT32)pBuf;
	conAttr.uiSize = uiSize;//POOL_SIZE_APP;
	conAttr.uiTimeingInterval = 200;
#endif

	//conAttr.uiLoadDegree = DRAM_CONSUME_HEAVY_LOADING;
	uiSelect = uiSelect % uiDegreeCnt;
	if (bTestDisable) {
		uiSelect = 6;
	}
	uiDegree = uiDegreeTable[uiSelect];

	conAttr.uiLoadDegree = uiDegree;

	emu_msg(("^Y""BW = %s (%d)\r\n", cDegreeName[uiSelect], uiSelect);

	// sdcard priority set high
	dma_setChannelPriority(DMA_CH_SDIO, DMA_PRIORITY_HIGH);
    dma_setChannelPriority(DMA_CH_CPU, DMA_PRIORITY_HIGH);

	//if(uiDegree > 2 && uiDegree < 6)
	//if(uiDegree < 3)
	if (1) {
		UINT32 uiSelect2 = 1;
		uiPriorityLevel = uiPriorityTable[uiSelect2];
		emu_msg(("uiPriorityLevel = %s(%d)\r\n", cPriorityName[uiSelect2], uiSelect2);
		dma_setChannelPriority(DMA_CH_H264_0, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_1, uiPriorityLevel);
		//dma_setChannelPriority(DMA_CH_H264_2, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_3, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_4, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_5, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_6, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_7, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_8, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_9, uiPriorityLevel); //hk
		//dma_setChannelPriority(DMA_CH_H264_A, uiPriorityLevel); //hk
	}

	if (uiDegree == DRAM_CONSUME_DISABLE_DMA_CHANNEL) {
		bIsDisable = 1;
		// Remember initial conAttr.bDMAChannel structure
		memset((void *)&conAttr.bDMAChannel, 0x0, sizeof(conAttr.bDMAChannel));
		if (0) {
			emu_msg(("only set DMA_CH_H264_5\r\n");
			//DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_2, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_5, conAttr.bDMAChannel);
		} else if (0) {
			emu_msg(("only set DMA_CH_H264 1,3,6,7\r\n");
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_1, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_3, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_6, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_7, conAttr.bDMAChannel);
		} else if (0) {

			emu_msg(("only set DMA_CH_H264 3,6\r\n");
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_3, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_6, conAttr.bDMAChannel);

		} else if (1) {
			emu_msg(("set DMA_CH_H264_ 0 ~ 8 + coe 0~1\r\n");
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_0, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_1, conAttr.bDMAChannel);
			//DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_2, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_3, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_4, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_5, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_6, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_7, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_8, conAttr.bDMAChannel);
			DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_9, conAttr.bDMAChannel); // OSD read
			//DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_A, conAttr.bDMAChannel); // OSD read
		}
	}

	if (DRAMConsume_Open(&conAttr) != E_OK) {
		emu_msg(("Open Consume dram BW task fail\r\n");
	} else {
		emu_msg(("Open Consume dram BW task success\r\n");
	}

	return bIsDisable;
}

void h26xEmuCloseTestBandWidth(void)
{
	if (DRAMConsume_Close() != E_OK) {
		emu_msg(("Close Consume dram BW task fail\r\n");
	} else {
		emu_msg(("Close Consume dram BW task success\r\n");
	}
}
#else


static UINT32   uiPriorityTable[3] = {DMA_PRIORITY_LOW, DMA_PRIORITY_MIDDLE, DMA_PRIORITY_HIGH};

static UINT8   *cPriorityName[3] = {(UINT8 *)"low", (UINT8 *)"middle", (UINT8 *)"high"};



BOOL h26XEmuSetTestBandWidth(UINT32 uiAddr, UINT32 uiAddr2, UINT32 uiSize, UINT32 uiSelect)
{
	DRAM_CONSUME_ATTR  attr;
	attr.load_degree = DRAM_CONSUME_HEAVY_LOADING;

	switch (uiSelect) {
		case 0:
			attr.load_degree = DRAM_CONSUME_EASY_LOADING;
			break;
		case 1:
			attr.load_degree = DRAM_CONSUME_NORMAL_LOADING;
			break;
		case 2:
			attr.load_degree = DRAM_CONSUME_HEAVY_LOADING;
			break;
		case 3:
		default:
			attr.load_degree = DRAM_CONSUME_CH_DISABLE;
			break;
	}





	if (1) {
		UINT32 uiSelect2 = 2; // 0 ~ 2
		UINT32 uiPriorityLevel;
		uiPriorityLevel = uiPriorityTable[uiSelect2];
		DBG_INFO("uiPriorityLevel = %s(%d)\r\n", cPriorityName[uiSelect2],(unsigned int) uiSelect2);

		// sdcard priority set high
		dma_setChannelPriority(DMA_CH_SDIO, DMA_PRIORITY_HIGH);
		dma_setChannelPriority(DMA_CH_CPU, DMA_PRIORITY_HIGH);

		#if 1
		dma_setChannelPriority(DMA_CH_HLOAD_0, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_HLOAD_1, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_HLOAD_2, uiPriorityLevel);
		#endif

		dma_setChannelPriority(DMA_CH_H264_0, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_1, uiPriorityLevel);
		//dma_setChannelPriority(DMA_CH_H264_2, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_3, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_4, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_5, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_6, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_7, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_8, uiPriorityLevel);
		dma_setChannelPriority(DMA_CH_H264_9, uiPriorityLevel); //hk
		//dma_setChannelPriority(DMA_CH_H264_A, uiPriorityLevel); //hk
	}


	if(attr.load_degree == DRAM_CONSUME_CH_DISABLE){
		memset((void *)&attr.dma_channel, 0x0, sizeof(attr.dma_channel));

		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_0, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_1, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_3, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_4, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_5, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_6, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_7, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_8, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_H264_9, attr.dma_channel);


	}

	attr.addr = uiAddr;
	attr.size = uiSize;
	dram_consume_cfg(&attr);
	attr.addr = uiAddr2;
	attr.size = uiSize;
	dram2_consume_cfg(&attr);
	dram_consume_start();
	dram2_consume_start();

	DBG_INFO("addr = 0x%08x size = 0x%08x\n",(unsigned int)uiAddr,(unsigned int)uiSize);
	DBG_INFO("addr2 = 0x%08x size = 0x%08x\n",(unsigned int)uiAddr2,(unsigned int)uiSize);
	return 0;
}
#endif

void decide_job_order(h26x_ctrl_t *p_ctrl)
{
    unsigned int i;
    if(p_ctrl->ver_item.job_order){
        int list1[H26X_JOB_Q_MAX];
        int list2[H26X_JOB_Q_MAX];
        int id;
        for (i = 0; i < p_ctrl->job_num; i++)
        {
            list1[i] = p_ctrl->job[i].idx1;
        }
        for (i = 0; i < p_ctrl->job_num; i++)
        {
            unsigned int get;
            do{
                get = rand() % p_ctrl->job_num;
                id = list1[get];
            }while(id == -1);
            list2[i] = id;
            list1[get] = -1;
        }
        for (i = 0; i < p_ctrl->job_num; i++)
        {
            p_ctrl->list_order[i] = list2[i];
            p_ctrl->job[list2[i]].idx2 = i;
        }
    }else{
        for (i = 0; i < p_ctrl->job_num; i++)
        {
            p_ctrl->list_order[i] = p_ctrl->job[i].idx1;
        }
    }
}
static void h26x_job_init(unsigned int idx, unsigned int codec, h26x_job_t *p_job, h26x_mem_t *p_mem)
{
	p_job->idx1 = idx;
	p_job->idx2 = idx;
	p_job->codec = codec;
	p_job->apb_addr = h26x_mem_malloc(p_mem, h26x_getHwRegSize());

	p_job->check1_addr = h26x_mem_malloc(p_mem, H26X_REG_REPORT_SIZE);
	p_job->check2_addr = h26x_mem_malloc(p_mem, H26X_REG_REPORT_SIZE);


    DBG_INFO("[%s][%d], idx = %d %d\r\n", __FUNCTION__, __LINE__,p_job->idx1,p_job->idx2);
}

void h26x_job_ctrl_init(h26x_ctrl_t *p_ctrl, h26x_mem_t *p_mem, h26x_mem_t *p_mem2)
{
	int i;

	memset(p_ctrl, 0, sizeof(h26x_ctrl_t));

	p_ctrl->ver_item.rand_seed = time(NULL);

#if 1
	p_ctrl->ver_item.rec_out_dis = 0;
	p_ctrl->ver_item.src_out_en  = 0;
	p_ctrl->ver_item.rnd_sw_rest = 0;
	p_ctrl->ver_item.rnd_slc_hdr = 0;
	p_ctrl->ver_item.rnd_bs_buf  = 0;
	p_ctrl->ver_item.rnd_bs_buf_32b = 0;//bug
	p_ctrl->ver_item.rotate_en   = 0;//ts
	p_ctrl->ver_item.src_cbcr_iv = 0;
	p_ctrl->ver_item.cmp_bs_en   = 0;
    p_ctrl->ver_item.rnd_sram_en = 0;
	p_ctrl->ver_item.bw_heavy    = 0;
	p_ctrl->ver_item.ddr_brst_en = 0;
	p_ctrl->ver_item.dram_range  = 1;
	p_ctrl->ver_item.dram2       = 0;
	p_ctrl->ver_item.write_prot  = 0; //bug
	p_ctrl->ver_item.gating      = 0; // bug
	p_ctrl->ver_item.clksel      = 0;
	p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.job_order   = 0;
#elif 0
    //source out only // need to set #define SRC_OUT_ONLY 1 //need to emu->info_obj.bSrcOutEn = 1;
    p_ctrl->ver_item.rec_out_dis = 1;
    p_ctrl->ver_item.src_out_en  = 1;
    p_ctrl->ver_item.rnd_sw_rest = 0;
    p_ctrl->ver_item.rnd_slc_hdr = 0;
    p_ctrl->ver_item.rnd_bs_buf  = 0;
    p_ctrl->ver_item.rnd_bs_buf_32b = 0;//bug
    p_ctrl->ver_item.rotate_en   = 1;//ts
    p_ctrl->ver_item.src_cbcr_iv = 1;
    p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 1;
    p_ctrl->ver_item.bw_heavy    = 1;
    p_ctrl->ver_item.ddr_brst_en = 1;
    p_ctrl->ver_item.dram_range  = 1;
    p_ctrl->ver_item.dram2       = 0;
    p_ctrl->ver_item.write_prot  = 0; //bug
    p_ctrl->ver_item.gating      = 1; // bug
    p_ctrl->ver_item.clksel      = 1;
    p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.job_order   = 1;
#elif 0
	p_ctrl->ver_item.rec_out_dis = 0;
	p_ctrl->ver_item.src_out_en  = 1;
	p_ctrl->ver_item.rnd_sw_rest = 0;
	p_ctrl->ver_item.rnd_slc_hdr = 0;
	p_ctrl->ver_item.rnd_bs_buf  = 0;
	p_ctrl->ver_item.rnd_bs_buf_32b = 0;//bug
	p_ctrl->ver_item.rotate_en   = 1;//ts
	p_ctrl->ver_item.src_cbcr_iv = 1;
	p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 0;
    p_ctrl->ver_item.bw_heavy    = 0;
    p_ctrl->ver_item.ddr_brst_en = 0;
    p_ctrl->ver_item.dram_range  = 1;
	p_ctrl->ver_item.dram2       = 0;
	p_ctrl->ver_item.write_prot  = 0; //bug
	p_ctrl->ver_item.gating      = 0; // bug
    p_ctrl->ver_item.clksel      = 0;
    p_ctrl->ver_item.job_order   = 1;
	p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.stable_bs_len = 0;
#elif 0
    //stable bs len
    p_ctrl->ver_item.rec_out_dis = 0;
    p_ctrl->ver_item.src_out_en  = 1;
    p_ctrl->ver_item.rnd_sw_rest = 0;
    p_ctrl->ver_item.rnd_slc_hdr = 0;
    p_ctrl->ver_item.rnd_bs_buf  = 0;
    p_ctrl->ver_item.rnd_bs_buf_32b = 0;
    p_ctrl->ver_item.rotate_en   = 1;
    p_ctrl->ver_item.src_cbcr_iv = 1;
    p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 1;
    p_ctrl->ver_item.bw_heavy    = 0;
    p_ctrl->ver_item.ddr_brst_en = 1;
    p_ctrl->ver_item.dram_range  = 1;
    p_ctrl->ver_item.dram2       = 0;
    p_ctrl->ver_item.write_prot  = 0; //bug
    p_ctrl->ver_item.gating      = 1; // bug
    p_ctrl->ver_item.clksel      = 0;
    p_ctrl->ver_item.job_order   = 1;
    p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.stable_bs_len = 1;
#elif 1
    //ov
    p_ctrl->ver_item.rec_out_dis = 0;
    p_ctrl->ver_item.src_out_en  = 1;
    p_ctrl->ver_item.rnd_sw_rest = 0;
    p_ctrl->ver_item.rnd_slc_hdr = 1;
    p_ctrl->ver_item.rnd_bs_buf  = 1;
    p_ctrl->ver_item.rnd_bs_buf_32b = 1;
    p_ctrl->ver_item.rotate_en   = 1;
    p_ctrl->ver_item.src_cbcr_iv = 1;
    p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 1;
    p_ctrl->ver_item.bw_heavy    = 1;
    p_ctrl->ver_item.ddr_brst_en = 1;
    p_ctrl->ver_item.dram_range  = 1;
    p_ctrl->ver_item.dram2       = 0;
    p_ctrl->ver_item.write_prot  = 0; //bug
    p_ctrl->ver_item.gating      = 1; // bug
    p_ctrl->ver_item.clksel      = 1;
    p_ctrl->ver_item.job_order   = 1;
    p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.stable_bs_len = 0;
#elif 0
    //ae 32
    p_ctrl->ver_item.rec_out_dis = 0;
    p_ctrl->ver_item.src_out_en  = 1;
    p_ctrl->ver_item.rnd_sw_rest = 0;
    p_ctrl->ver_item.rnd_slc_hdr = 0;
    p_ctrl->ver_item.rnd_bs_buf  = 1;
    p_ctrl->ver_item.rnd_bs_buf_32b = 1;
    p_ctrl->ver_item.rotate_en   = 1;
    p_ctrl->ver_item.src_cbcr_iv = 1;
    p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 1;
    p_ctrl->ver_item.bw_heavy    = 1;
    p_ctrl->ver_item.ddr_brst_en = 1;
    p_ctrl->ver_item.dram_range  = 1;
    p_ctrl->ver_item.dram2       = 0;
    p_ctrl->ver_item.write_prot  = 0; //bug
    p_ctrl->ver_item.gating      = 1; // bug
    p_ctrl->ver_item.clksel      = 0;
    p_ctrl->ver_item.job_order   = 1;
    p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.stable_bs_len = 0;
#elif 0
	p_ctrl->ver_item.rec_out_dis = 0;
	p_ctrl->ver_item.src_out_en  = 1;
	p_ctrl->ver_item.rnd_sw_rest = 0;
	p_ctrl->ver_item.rnd_slc_hdr = 0;
	p_ctrl->ver_item.rnd_bs_buf  = 0;
	p_ctrl->ver_item.rnd_bs_buf_32b = 0;
	p_ctrl->ver_item.rotate_en   = 1;
	p_ctrl->ver_item.src_cbcr_iv = 1;
	p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 1;
    p_ctrl->ver_item.bw_heavy    = 1;
    p_ctrl->ver_item.ddr_brst_en = 1;
    p_ctrl->ver_item.dram_range  = 1;
	p_ctrl->ver_item.dram2       = 0;
	p_ctrl->ver_item.write_prot  = 0;
	p_ctrl->ver_item.gating      = 0;
    p_ctrl->ver_item.clksel      = 0;
    p_ctrl->ver_item.job_order   = 1;
	p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.stable_bs_len = 0;
#elif 1
    p_ctrl->ver_item.rec_out_dis = 0;
    p_ctrl->ver_item.src_out_en  = 1;
    p_ctrl->ver_item.rnd_sw_rest = 0;
    p_ctrl->ver_item.rnd_slc_hdr = 0;
    p_ctrl->ver_item.rnd_bs_buf  = 1;
    p_ctrl->ver_item.rnd_bs_buf_32b = 1;//bug
    p_ctrl->ver_item.rotate_en   = 1;//ts
    p_ctrl->ver_item.src_cbcr_iv = 0;
    p_ctrl->ver_item.cmp_bs_en   = 1;
    p_ctrl->ver_item.rnd_sram_en = 1;
    p_ctrl->ver_item.bw_heavy    = 1;
    p_ctrl->ver_item.ddr_brst_en = 1;
    p_ctrl->ver_item.dram_range  = 0;
    p_ctrl->ver_item.dram2       = 0;
    p_ctrl->ver_item.write_prot  = 0; //bug
    p_ctrl->ver_item.gating      = 1; // bug
    p_ctrl->ver_item.clksel      = 0;
    p_ctrl->ver_item.job_order   = 0;
	p_ctrl->ver_item.lofs        = 0;
    p_ctrl->ver_item.stable_bs_len = 0;
#endif

#if 1
    if(p_ctrl->ver_item.dram2)
    {
        p_mem2->start = p_mem->start + 0x40000000;
        p_mem2->addr = p_mem->start + 0x40000000;
        p_mem->size = p_mem2->size =  p_mem->size < p_mem2->size ? p_mem->size : p_mem2->size;
        DBG_INFO("dram1 mem = 0x%08x size = 0x%08x\r\n",(int)p_mem->addr,(int)p_mem->size);
        DBG_INFO("dram2 mem = 0x%08x size = 0x%08x\r\n",(int)p_mem2->addr,(int)p_mem2->size);
    }
#endif


	DBG_INFO("\r\n");
	DBG_INFO("direct mode : %d, %d, %d\r\n", DIRECT_MODE, H26X_JOB_Q_MAX, H26X_JOB_MAX);
	DBG_INFO("rand_seed   : %d\r\n", p_ctrl->ver_item.rand_seed);
	DBG_INFO("rec_out_dis : %d\r\n", p_ctrl->ver_item.rec_out_dis);
	DBG_INFO("src_out_en  : %d\r\n", p_ctrl->ver_item.src_out_en);
	DBG_INFO("rnd_sw_rest : %d\r\n", p_ctrl->ver_item.rnd_sw_rest);
	DBG_INFO("rnd_slc_hdr : %d\r\n", p_ctrl->ver_item.rnd_slc_hdr);
	DBG_INFO("rnd_bs_buf  : %d\r\n", p_ctrl->ver_item.rnd_bs_buf);
	DBG_INFO("rnd_bs_buf_32b  : %d\r\n", p_ctrl->ver_item.rnd_bs_buf_32b);
	DBG_INFO("rotate_en   : %d\r\n", p_ctrl->ver_item.rotate_en);
	DBG_INFO("src_cbcr_iv : %d\r\n", p_ctrl->ver_item.src_cbcr_iv);
	DBG_INFO("cmp_bs_en   : %d\r\n", p_ctrl->ver_item.cmp_bs_en);
	DBG_INFO("rnd_sram_en : %d\r\n", p_ctrl->ver_item.rnd_sram_en);
	DBG_INFO("bw_heavy    : %d\r\n", p_ctrl->ver_item.bw_heavy);
	DBG_INFO("ddr_brst_en : %d\r\n", p_ctrl->ver_item.ddr_brst_en);
	DBG_INFO("dram_range  : %d\r\n", p_ctrl->ver_item.dram_range);
	DBG_INFO("dram2       : %d\r\n", p_ctrl->ver_item.dram2);
	DBG_INFO("write_prot  : %d\r\n", p_ctrl->ver_item.write_prot);
	DBG_INFO("gating      : %d\r\n", p_ctrl->ver_item.gating);
	DBG_INFO("clksel      : %d\r\n", p_ctrl->ver_item.clksel);
	DBG_INFO("lofs        : %d\r\n", p_ctrl->ver_item.lofs);
	DBG_INFO("job_order : %d\r\n", p_ctrl->ver_item.job_order);
    DBG_INFO("stable_bs_len : %d\r\n", p_ctrl->ver_item.stable_bs_len);
	DBG_INFO("\r\n");
	srand(p_ctrl->ver_item.rand_seed);

	if( p_ctrl->ver_item.dram_range && p_ctrl->ver_item.dram2 ){
		p_mem->size = p_mem2->size = ((((p_mem->size+1)/2 + 15)/16)*16) - 0x400;//use half size
		p_mem2->start = p_mem2->addr = p_mem2->start + p_mem->size;
	}

	if(p_ctrl->ver_item.bw_heavy){ //hk TODO
#if 1
		unsigned int uiTestBwSize =  0x1000000;//0x3000;
		unsigned int uiBWDegree = 1; //0:easy 1:normal 2:heavy 3: disable, bw disable only can use in isr mode
		//BOOL bTestDisable = 1;
		unsigned int bw_test_addr =  h26x_mem_malloc(p_mem, uiTestBwSize);
		unsigned int bw_test_addr2 =  h26x_mem_malloc(p_mem2, uiTestBwSize);
        //bTestDisable = 1; //to test bw disable
		h26XEmuSetTestBandWidth(bw_test_addr, bw_test_addr2, uiTestBwSize, uiBWDegree);
#endif
	}

	p_ctrl->llc_size = h26x_getHwRegSize() + 32;

	for (i = 0; i < H26X_JOB_Q_MAX; i++)
		p_ctrl->llc_addr[i] = h26x_mem_malloc(p_mem, p_ctrl->llc_size);

	unsigned int job_mem_size = 0;
	if(p_ctrl->ver_item.dram_range)
		job_mem_size = p_mem->size/((H26X_JOB_MAX+1)/2) - 0x400;
	else
		job_mem_size = p_mem->size/H26X_JOB_MAX - 0x400;

	for (i = 0; i < H26X_JOB_MAX; i++)
	{
		if(p_ctrl->ver_item.dram_range){
			p_ctrl->job[i].mem.start = h26x_mem_malloc(i<H26X_JOB_MAX/2 ? p_mem : p_mem2, job_mem_size);
		}
		else{
			p_ctrl->job[i].mem.start = h26x_mem_malloc(p_mem, job_mem_size);
		}
		p_ctrl->job[i].mem.addr  = p_ctrl->job[i].mem.start;
		p_ctrl->job[i].mem.size  = job_mem_size;
	}

	if(p_ctrl->ver_item.dram2){
		h26x_enable_dram2(); //enable dram2 after malloc job[i].mem
	}
	if(p_ctrl->ver_item.write_prot){
		h26x_enable_wp();
	}

}
void h26x_job_ctrl_close(h26x_ctrl_t *p_ctrl)
{
	if(p_ctrl->ver_item.bw_heavy){
		h26xEmuCloseTestBandWidth();
	}
}

h26x_job_t *h26x_job_add(unsigned int codec, h26x_ctrl_t *p_ctrl)
{
	if (p_ctrl->job_num == H26X_JOB_MAX)	// shall be modify to H26X_JOB_MAX while driver base //
	{
		DBG_INFO("job queeu full, cannot add new job\r\n");
		return NULL;
	}

	h26x_job_init(p_ctrl->job_num, codec, &p_ctrl->job[p_ctrl->job_num], &p_ctrl->job[p_ctrl->job_num].mem);
	p_ctrl->job_num++;

	return &p_ctrl->job[p_ctrl->job_num-1];
}

static int h26x_job_prepare_one_pic(h26x_ctrl_t *p_ctrl, h26x_srcd2d_t *p_src_d2d)
{
	unsigned int idx,i;
	unsigned int ret = 1;

	if (p_ctrl->ver_item.write_prot)
	{
		h26x_disable_wp();
	}

    decide_job_order(p_ctrl);
	for ( i = 0; i < p_ctrl->job_num; i++)
	{
        idx = p_ctrl->list_order[i];
        //DBG_INFO("[%s][%d], i = %d, idx = %d\r\n", __FUNCTION__, __LINE__,i,idx);
		if (p_ctrl->job[idx].codec == 0)
		{
			ret &= emu_h265_prepare_one_pic(&p_ctrl->job[idx], &p_ctrl->ver_item, &p_src_d2d[idx]);
		}else if (p_ctrl->job[idx].codec == 1){
		    ret &= emu_h264_prepare_one_pic(&p_ctrl->job[idx], &p_ctrl->ver_item, &p_src_d2d[idx]);
        }else if (p_ctrl->job[idx].codec == 2){
			ret &= emu_h265d_prepare_one_pic(&p_ctrl->job[idx], &p_ctrl->ver_item);
        }else if (p_ctrl->job[idx].codec == 3){
		    ret &= emu_h264d_prepare_one_pic(&p_ctrl->job[idx], &p_ctrl->ver_item);
		}
	}

	return ret;
}

static UINT32 linklist_cmd(LINKLIST_CMD cmd, int job_id, int offset, int burst)
{
	UINT32 val = 0;

	save_to_reg(&val, burst,   0,  8);

#if 0
	save_to_reg(&val, offset/4,  8, 12); // word align //hk : it's 321 setting
#else
	save_to_reg(&val, offset,  8, 12);  //byte align
#endif
	save_to_reg(&val, job_id, 20,  8);
	save_to_reg(&val, cmd,    28,  4);

	return val;
}

static void set_encode_one_pic_cmd(h26x_job_t *p_job, unsigned int llc_addr, unsigned int nxt_llc_addr, unsigned int llc_size)
{
	unsigned int *addr = (unsigned int *)llc_addr;
	unsigned int i = 0;

#if SRC_OUT_ONLY
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	unsigned int pic_type = pRegSet->PIC_CFG & 0x1;
	unsigned int rec_out_en = (pRegSet->PIC_CFG>>4) & 0x1;
	unsigned int src_out_en = (pRegSet->PIC_CFG>>7) & 0x1;

//	unsigned int lpm = pRegSet->LPM_CFG;
	unsigned int rrc = pRegSet->RRC_CFG[0]&0x3;
	unsigned int mbqp = (pRegSet->FUNC_CFG[0]>>8)&0x1;


	if(pic_type == 1 && rec_out_en ==0 && src_out_en==1)//P
	{
		 save_to_reg(&pRegSet->PIC_CFG, 0, 0, 1);//I
	}
	if( rec_out_en ==0 && src_out_en==1)//P
	{
		//DBG_INFO("disable rrc\r\n");

		// save_to_reg(&pRegSet->LPM_CFG, 0, 0, 32);
	 	save_to_reg(&pRegSet->RRC_CFG[0], 0, 0, 2);
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 8, 1);
	}
#endif

	memset((void *)llc_addr, 0, llc_size);

	addr[i++] = linklist_cmd(Start,  p_job->idx2, 0, 0);
	addr[i++] = linklist_cmd(WR,     p_job->idx2, H26X_REG_BASIC_START_OFFSET, H26X_REG_BASIC_SIZE);
	memcpy(addr + i, (void *)p_job->apb_addr, sizeof(UINT32)*H26X_REG_BASIC_SIZE);
	i += H26X_REG_BASIC_SIZE;
	addr[i++] = linklist_cmd(WR,     p_job->idx2, H26X_REG_PLUG_IN_START_OFFSET, H26X_REG_PLUG_IN_SIZE);
	memcpy(addr + i, (void *)(p_job->apb_addr + (H26X_REG_PLUG_IN_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_SIZE);
	i += H26X_REG_PLUG_IN_SIZE;
	addr[i++] = linklist_cmd(WR,	 p_job->idx2, H26X_REG_PLUG_IN_2_START_OFFSET, H26X_REG_PLUG_IN_2_SIZE);
	memcpy(addr + i, (void *)(p_job->apb_addr + (H26X_REG_PLUG_IN_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_2_SIZE);
	i += H26X_REG_PLUG_IN_2_SIZE;
	addr[i++] = linklist_cmd(WR,	 p_job->idx2, H26X_REG_PLUG_IN_3_START_OFFSET, H26X_REG_PLUG_IN_3_SIZE);
	memcpy(addr + i, (void *)(p_job->apb_addr + (H26X_REG_PLUG_IN_3_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_3_SIZE);
	i += H26X_REG_PLUG_IN_3_SIZE;
	addr[i++] = linklist_cmd(WR,	 p_job->idx2, H26X_REG_PLUG_IN_4_START_OFFSET, H26X_REG_PLUG_IN_4_SIZE);
	memcpy(addr + i, (void *)(p_job->apb_addr + (H26X_REG_PLUG_IN_4_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_4_SIZE);
	i += H26X_REG_PLUG_IN_4_SIZE;


#if 0
    addr[i++] = linklist_cmd(RD,     p_job->idx2, H26X_REG_REPORT_START_OFFSET, H26X_REPORT_NUMBER);
	addr[i++] = h26x_getPhyAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
#else
    addr[i++] = linklist_cmd(RD,     p_job->idx2, H26X_REG_REPORT_START_OFFSET, H26X_REPORT_NUMBER);
    addr[i++] = h26x_getPhyAddr(p_job->check1_addr);
#endif
#if 0
	addr[i++] = linklist_cmd(RD,	 p_job->idx2, H26X_REG_REPORT_2_START_OFFSET, H26X_REPORT_2_NUMBER);
	addr[i++] = h26x_getPhyAddr(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
#else
    addr[i++] = linklist_cmd(RD,     p_job->idx2, H26X_REG_REPORT_2_START_OFFSET, H26X_REPORT_2_NUMBER);
    addr[i++] = h26x_getPhyAddr(p_job->check2_addr);
#endif



	addr[i++] = linklist_cmd(Finish, p_job->idx2, 0, 0);
	addr[i++] = h26x_getPhyAddr(nxt_llc_addr);




/*
		DBG_INFO("RD 0x%08x %d, phy : 0x%08x, 0x%08x\r\n",H26X_REG_REPORT_START_OFFSET,H26X_REPORT_NUMBER,
			h26x_getPhyAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET),
			h26x_getVirAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET));
*/


	p_job->llc_addr = llc_addr;

	h26x_flushCache(llc_addr, llc_size);
	#if 0
    DBG_INFO("apb_addr= 0x%08x\r\n",p_job->apb_addr);
	h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
    DBG_INFO("llc_addr= 0x%08x\r\n",llc_addr);
	h26x_prtMem(llc_addr, llc_size);
	#endif
#if SRC_OUT_ONLY
		if(pic_type == 1 && rec_out_en ==0 && src_out_en==1)//P
		{
			save_to_reg(&pRegSet->PIC_CFG, 1, 0, 1);//P


		}

		if( rec_out_en ==0 && src_out_en==1)//P
		{
		//	save_to_reg(&pRegSet->LPM_CFG, lpm, 0, 32);
		 	save_to_reg(&pRegSet->RRC_CFG[0], rrc, 0, 2);
			save_to_reg(&pRegSet->FUNC_CFG[0], mbqp, 8, 1);
		}
#endif
}

static void set_apb_rec_out_opt(h26x_job_t *p_job, unsigned int rec_out_en)
{
	unsigned int apb_addr = p_job->apb_addr;
	H26XRegSet *pRegSet = (H26XRegSet *)apb_addr;

	if (pRegSet->FUNC_CFG[0] & 0x1) // only h264
	{

		h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
		h264_pat_t *pat = &ctx->pat;
		if(pat->pic_num==0)
		save_to_reg(&pRegSet->PIC_CFG, 0, 5, 1);	// col_mv_out_en
		else
		save_to_reg(&pRegSet->PIC_CFG, rec_out_en, 5, 1);	// col_mv_out_en

		save_to_reg(&pRegSet->PIC_CFG, rec_out_en, 4, 1);	// rec_out_en
		save_to_reg(&pRegSet->PIC_CFG, rec_out_en, 6, 1);	// sideinfo_out_en disable
	}

	if (pRegSet->FUNC_CFG[0] & 0x2) // only decoder
	{
		save_to_reg(&pRegSet->PIC_CFG, 0, 6, 1);	// sideinfo_out_en disable
	}

	if(pRegSet->FUNC_CFG[0] & 0x1) // only h264
	{
        //save_to_reg(&pRegSet->PIC_CFG, rec_out_en, 8, 1);   // smart_rec_out_en
    }

#if	SRC_OUT_ONLY
	unsigned int src_out_en = (pRegSet->PIC_CFG >> 7) & 1;

	if(rec_out_en==0 && src_out_en == 1){
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 4, 1);	// ae_en //
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 15, 1);	// enable dump nalu length //

		//DBG_INFO("FUNC_CFG[0]= 0x%08x\r\n",(int)pRegSet->FUNC_CFG[0]);
	}
	else{
		save_to_reg(&pRegSet->FUNC_CFG[0], 1, 4, 1);	// ae_en //
		save_to_reg(&pRegSet->FUNC_CFG[0], 1, 15, 1);	// enable dump nalu length //
	}
#endif

}

static void h26x_job_encode_one_pic(h26x_ctrl_t *p_ctrl, unsigned int rec_out_en)
{
	static int cnt = 0;

	if (DIRECT_MODE)
	{
		int currect_job_idx = p_ctrl->currect_job_idx;
		if (rec_out_en == 0)
		{
			H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[0].apb_addr;

			//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
			//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
			//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
			memcpy((void *)h26x_getVirAddr(p_ctrl->job[0].tmp_osg_gcac_addr0), (void *)h26x_getVirAddr(pRegSet->OSG_CFG[0x484/4]), 256);
			memcpy((void *)h26x_getVirAddr(p_ctrl->job[0].tmp_osg_gcac_addr1), (void *)h26x_getVirAddr(pRegSet->OSG_CFG[0x480/4]), 256);
            h26x_flushCache(p_ctrl->job[0].tmp_osg_gcac_addr0, 256);
            h26x_flushCache(p_ctrl->job[0].tmp_osg_gcac_addr1, 256);
			H26X_SWAP(p_ctrl->job[0].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
			H26X_SWAP(p_ctrl->job[0].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);
		}
		set_apb_rec_out_opt(&p_ctrl->job[currect_job_idx], rec_out_en);
		h26x_setEncDirectRegSet(p_ctrl->job[currect_job_idx].apb_addr);
	}
	else
	{
		unsigned int job_idx = 0, llc_idx = 0;
		unsigned int i = 0;

		for (i = 0; i < p_ctrl->job_num; i++)
		{
            job_idx = p_ctrl->list_order[i];
			if (p_ctrl->job[job_idx].is_finish == 0)
			{
				if (rec_out_en == 0)
				{
					H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[job_idx].apb_addr;

					//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
					//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
					//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
					memcpy((void *)h26x_getVirAddr(p_ctrl->job[job_idx].tmp_osg_gcac_addr0), (void *)h26x_getVirAddr(pRegSet->OSG_CFG[0x484/4]), 256);
					memcpy((void *)h26x_getVirAddr(p_ctrl->job[job_idx].tmp_osg_gcac_addr1), (void *)h26x_getVirAddr(pRegSet->OSG_CFG[0x480/4]), 256);
                    h26x_flushCache(p_ctrl->job[job_idx].tmp_osg_gcac_addr0, 256);
                    h26x_flushCache(p_ctrl->job[job_idx].tmp_osg_gcac_addr1, 256);
					H26X_SWAP(p_ctrl->job[job_idx].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
					H26X_SWAP(p_ctrl->job[job_idx].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);
				}

				if ((i + 1) < p_ctrl->job_num)
				{
					set_apb_rec_out_opt(&p_ctrl->job[job_idx], rec_out_en);
					set_encode_one_pic_cmd(&p_ctrl->job[job_idx], p_ctrl->llc_addr[llc_idx], p_ctrl->llc_addr[llc_idx+1], p_ctrl->llc_size);
				}
				else
				{
					set_apb_rec_out_opt(&p_ctrl->job[job_idx], rec_out_en);
					set_encode_one_pic_cmd(&p_ctrl->job[job_idx], p_ctrl->llc_addr[llc_idx], 0, p_ctrl->llc_size);
				}
				llc_idx++;
			}
		}
		h26x_setEncLLRegSet(llc_idx, h26x_getPhyAddr(p_ctrl->llc_addr[0]));
	}

	//if (cnt == 2) dump_src_out(&p_ctrl->job[0], 21);
	cnt++;
    if(p_ctrl->ver_item.rnd_sram_en)
    {
        UINT32 uiCycle,uiLightSleepEn,uiSleepDownEn;
        uiCycle = rand() % (1<<17);
        uiLightSleepEn = rand() % 2;
        uiSleepDownEn = rand() % 2;
        h26x_setSRAMMode(uiCycle,uiLightSleepEn,uiSleepDownEn);
    }

#if 1
    {
        int hw_clk = 0;
        int sw_clk = 0;
        int hw_pclk = 0;
        int sw_pclk = 0;

        if (p_ctrl->ver_item.gating){

            hw_clk = rand()%2;
            sw_clk = rand()%2;
            hw_pclk = rand()%2;
            sw_pclk = rand()%2;
            //#if (DIRECT_MODE == 0)
			#if 0
            if(hw_pclk == 0)
            {
                sw_pclk = 1;
            }
            #endif
        }
        DBG_INFO("gating (hw_clk,sw_clk,hw_pclk,sw_pclk) = (%d %d %d %d)\r\n",hw_clk,sw_clk,hw_pclk,sw_pclk);

        if(hw_clk)
        {
            //DBG_INFO("hw clk on\r\n");
            h26x_setCodecClock(TRUE);
        } else {
            //DBG_INFO("hw clk off\r\n");
            h26x_setCodecClock(FALSE);
        }
        if(sw_clk)
        {
            //DBG_INFO("sw clk on\r\n");
            pll_set_clk_auto_gating(H265_M_GCLK);
        } else {
            //DBG_INFO("sw clk off\r\n");
            pll_clear_clk_auto_gating(H265_M_GCLK);
        }
        if(hw_pclk)
        {
            //DBG_INFO("hw pclk on\r\n");
            h26x_setCodecPClock(TRUE);
        } else {
            //DBG_INFO("hw pclk off\r\n");
            h26x_setCodecPClock(FALSE);
        }
        if(sw_pclk)
        {
            //DBG_INFO("sw pclk on\r\n");
            pll_set_pclk_auto_gating(H265_GCLK);
        } else {
            //DBG_INFO("sw pclk off\r\n");
            pll_clear_pclk_auto_gating(H265_GCLK);
        }

    }
#endif


	if (p_ctrl->ver_item.clksel){
#if 0
        UINT32 speed;
        //Min codec clock            DDR 2133   DDR 1866
        //Max burst 64W (burst = 0)   290MHz     254MHz
        //Max burst 32W (burst = 1)   199MHz     174MHz
        // 0 : 240
        // 1 : 320
        // 2 : pll13 (200 + rand() % 120); // 200 ~ 320 //

        if(ddrburst == 0){
            speed = (290 + rand() % 50); // 200 ~ 320 //
            h26xEmuClkSel2( (rand()%2)+1,speed);
        }else{
            speed = (200 + rand() % 120); // 200 ~ 320 //
            h26xEmuClkSel2(rand()%3,speed);
        }
#elif 0
        if(ddrburst == 0){
            speed = 290;
            h26xEmuClkSel2( 2,speed);
        }else{
            speed = 200;
            h26xEmuClkSel2(2,speed);
        }
#elif 0
        UINT32 speed = 340;
/*
        if(pp_see_pat == 0)
            speed = 340;
        else
            speed = 400;
*/
        h26xEmuClkSel2( 2,speed);
#else
		h26xEmuClkSel(rand()%4);
#endif
	}

	//h26x_prtReg();

	//hk:disable_codec_flow
	if (!ipp_verf_low_delay_en) {
		h26x_start();
	}
}

static UINT32 h26x_job_check_interrupt(h26x_ctrl_t *p_ctrl, UINT32 wait_int)
{
	UINT32 interrupt = 0;
	UINT32 tmp = 0;
	while(interrupt == 0)
	{

		//set debug unit
		#if 0
		while(interrupt == 0){

			interrupt = h26x_getIntStatus();

		 	if(interrupt){
				//h26x_setUnitChecksum(0);
				h26x_clearIntStatus(interrupt);
		 	}

		}
		#endif

        if(p_ctrl->ver_item.stable_bs_len)
        {
            UINT32 uiIntStatus = 0;
            while((uiIntStatus & h26x_getIntEn()) ==0){
                unsigned int i = (DIRECT_MODE) ? 0 : h26x_getCurJobNum();
                unsigned int job_idx;
                job_idx = p_ctrl->list_order[i];
                if (p_ctrl->job[job_idx].codec == 0)
                {
                    emu_h265_compare_stable_len(&p_ctrl->job[job_idx]);
                }
				else if (p_ctrl->job[job_idx].codec == 1)
                {
                    emu_h264_compare_stable_len(&p_ctrl->job[job_idx]);
                }
                uiIntStatus = h26x_getIntStatus();
            }
        }

		interrupt = h26x_waitINT();
		//DBG_INFO("get interrupt = 0x%08x \r\n",(unsigned int)interrupt);

		if (p_ctrl->ver_item.rnd_bs_buf)
		{

			if ((interrupt & H26X_SWRST_FINISH_INT) != 0)
			{
				unsigned int i = (DIRECT_MODE) ? 0 : h26x_getCurJobNum();
                unsigned int job_idx;
                job_idx = p_ctrl->list_order[i];
				if (p_ctrl->job[job_idx].codec == 0)
				{
					//emu_h265_reset_bsbuf(&p_ctrl->job[job_idx]);
				}
				else if (p_ctrl->job[job_idx].codec == 1)
				{
					h26x_job_t *p_job = &p_ctrl->job[job_idx];
					h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
					h264_pat_t *pat = &ctx->pat;
					UINT32 bs_size = pat->bs_buf_size;
					emu_h264_reset_bsbuf(&p_ctrl->job[job_idx], bs_size, p_ctrl->ver_item.write_prot );
				}
			}
			else if ((interrupt & H26X_BSOUT_INT) != 0  &&  (wait_int & H26X_SWRST_FINISH_INT) == 0 )
			{
				unsigned int i = (DIRECT_MODE) ? 0 : h26x_getCurJobNum();
                unsigned int job_idx;
                job_idx = p_ctrl->list_order[i];

				if (p_ctrl->job[job_idx].codec == 0)
				{
					emu_h265_set_nxt_bsbuf(&p_ctrl->job[job_idx]);
				}
				else if (p_ctrl->job[job_idx].codec == 1)
				{
					emu_h264_set_nxt_bsbuf(&p_ctrl->job[job_idx], &p_ctrl->ver_item, interrupt);
				}


				interrupt = 0;
			}
			else if ((interrupt & H26X_BSDMA_INT) != 0)
			{
				unsigned int i = (DIRECT_MODE) ? 0 : h26x_getCurJobNum();
                unsigned int job_idx;
                job_idx = p_ctrl->list_order[i];

				if (p_ctrl->job[job_idx].codec == 2)
				{
					emu_h265d_set_nxt_bsbuf(&p_ctrl->job[job_idx]);
				}
				else if (p_ctrl->job[job_idx].codec == 3)
				{
					emu_h264d_set_nxt_bsbuf(&p_ctrl->job[job_idx]);
				}

				interrupt = 0;
			}

		}
		if (p_ctrl->ver_item.rnd_sw_rest)
		{
			if((tmp != 1) && (tmp % 2000000 == 1))
			{
				DBG_INFO("h26x not get soft reset interrupt \r\n");
				if(tmp > 8000000)
				{
					break;
				}
			}
			tmp++;
			if ((interrupt & H26X_SWRST_FINISH_INT) == 0  && (wait_int & H26X_SWRST_FINISH_INT) != 0 )
			{


				#if 1 //hk tmp
				DBG_INFO("[%s][%d] get int but not swrest int \r\n",__FUNCTION__,__LINE__);

				h26x_reset();
				h26x_setLock();
				#endif

				interrupt = 0;


			}
		}
	}

	return interrupt;
}

static void h26x_job_swrset_encode_one_pic(h26x_ctrl_t *p_ctrl)
{
	unsigned int job_idx = 0, llc_idx = 0,i,j;

	if (DIRECT_MODE)
	{
		H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[0].apb_addr;

		H26X_SWAP(p_ctrl->job[0].tmp_rec_y_addr,    pRegSet->REC_Y_ADDR, UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_rec_c_addr,    pRegSet->REC_C_ADDR, UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_colmv_addr,    pRegSet->COL_MVS_WR_ADDR, UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_sideinfo_addr, pRegSet->SIDE_INFO_WR_ADDR_0, UINT32);
        for(i=0;i<H26X_MAX_TILE_NUM-1;i++)
        {
            H26X_SWAP(p_ctrl->job[0].tmp_sideinfo_addr2[i], pRegSet->TILE_CFG[19+i], UINT32);
            H26X_SWAP(p_ctrl->job[0].tmp_rec_y_addr2[i],    pRegSet->TILE_CFG[3+i], UINT32);
            H26X_SWAP(p_ctrl->job[0].tmp_rec_c_addr2[i],    pRegSet->TILE_CFG[7+i], UINT32);
        }

		//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
		//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
		//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);

		h26x_setEncDirectRegSet(p_ctrl->job[0].apb_addr);
	}
	else
	{
		for ( j = 0; j < p_ctrl->job_num; j++)
		{
            job_idx = p_ctrl->list_order[j];
			if (p_ctrl->job[job_idx].is_finish == 0)
			{
				H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[job_idx].apb_addr;


				//DBG_INFO("tmp_rec_y_addr =	0x%08x \n",(unsigned int)p_ctrl->job[job_idx].tmp_rec_y_addr);

				H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_y_addr,    pRegSet->REC_Y_ADDR, UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_c_addr,    pRegSet->REC_C_ADDR, UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_colmv_addr,    pRegSet->COL_MVS_WR_ADDR, UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_sideinfo_addr, pRegSet->SIDE_INFO_WR_ADDR_0, UINT32);


				for(i=0;i<H26X_MAX_TILE_NUM-1;i++)
                {
                    H26X_SWAP(p_ctrl->job[job_idx].tmp_sideinfo_addr2[i],  pRegSet->TILE_CFG[19+i], UINT32);
                    H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_y_addr2[i],    pRegSet->TILE_CFG[3+i], UINT32);
                    H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_c_addr2[i],    pRegSet->TILE_CFG[7+i], UINT32);
                }
				//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
				//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
				//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);

				if ((j + 1) < p_ctrl->job_num)
				{
					set_encode_one_pic_cmd(&p_ctrl->job[job_idx], p_ctrl->llc_addr[llc_idx], p_ctrl->llc_addr[llc_idx+1], p_ctrl->llc_size);
				}
				else
				{
					set_encode_one_pic_cmd(&p_ctrl->job[job_idx], p_ctrl->llc_addr[llc_idx], 0, p_ctrl->llc_size);
				}

				llc_idx++;
			}
		}
		h26x_setEncLLRegSet(llc_idx, h26x_getPhyAddr(p_ctrl->llc_addr[0]));
	}

	h26x_start();

	{
		volatile UINT32 srt_cnt = (rand()%10 * 10 ) +2 /*1600*/, s, ss;
		DBG_INFO("[%s][%d]srt_cnt = %d \r\n",__FUNCTION__,__LINE__,(unsigned int) srt_cnt);
		for (ss=1; ss < srt_cnt; ss=ss+1)
		{
			for (s=0; s < ss; s++)
			#if 1
			{
				volatile int delay = 2000;
				while (delay-- > 0);
				h26x_reset();
				h26x_job_check_interrupt(p_ctrl, H26X_SWRST_FINISH_INT);//wait sw reset interrupt
				h26x_setLock();
			}
			#endif
			//DBG_ERR("[%s][%d]\r\n",__FUNCTION__,__LINE__);

			h26x_setBsDmaEn();
			h26x_setBsOutEn();
			//h26x_setLock();
			h26x_start();
			//DBG_ERR("[%s][%d]\r\n",__FUNCTION__,__LINE__);
		}
		//DBG_ERR("[%s][%d]\r\n",__FUNCTION__,__LINE__);
		volatile int delay = 2000;
		while (delay-- > 0);
		h26x_reset();
		h26x_job_check_interrupt(p_ctrl, H26X_SWRST_FINISH_INT);//wait sw reset interrupt


		//DBG_ERR("[%s][%d]\r\n",__FUNCTION__,__LINE__);
	}

	// recover //
	if (DIRECT_MODE)
	{
		H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[0].apb_addr;

		H26X_SWAP(p_ctrl->job[0].tmp_rec_y_addr,    pRegSet->REC_Y_ADDR, UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_rec_c_addr,    pRegSet->REC_C_ADDR, UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_colmv_addr,    pRegSet->COL_MVS_WR_ADDR, UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_sideinfo_addr, pRegSet->SIDE_INFO_WR_ADDR_0, UINT32);
        for(i=0;i<H26X_MAX_TILE_NUM-1;i++)
        {
            H26X_SWAP(p_ctrl->job[0].tmp_sideinfo_addr2[i], pRegSet->TILE_CFG[19+i], UINT32);
            H26X_SWAP(p_ctrl->job[0].tmp_rec_y_addr2[i], pRegSet->TILE_CFG[3+i], UINT32);
            H26X_SWAP(p_ctrl->job[0].tmp_rec_c_addr2[i], pRegSet->TILE_CFG[7+i], UINT32);
        }

		//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
		//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
		//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
		H26X_SWAP(p_ctrl->job[0].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);

	}
	else
	{
		unsigned int job_idx = 0;

        for ( j = 0; j < p_ctrl->job_num; j++)
		{
            job_idx = p_ctrl->list_order[j];
			if (p_ctrl->job[job_idx].is_finish == 0)
			{
				H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[job_idx].apb_addr;

				H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_y_addr,    pRegSet->REC_Y_ADDR, UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_c_addr,    pRegSet->REC_C_ADDR, UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_colmv_addr,    pRegSet->COL_MVS_WR_ADDR, UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_sideinfo_addr, pRegSet->SIDE_INFO_WR_ADDR_0, UINT32);
                for(i=0;i<H26X_MAX_TILE_NUM-1;i++)
                {
                    H26X_SWAP(p_ctrl->job[job_idx].tmp_sideinfo_addr2[i], pRegSet->TILE_CFG[19+i], UINT32);
                    H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_y_addr2[i], pRegSet->TILE_CFG[3+i], UINT32);
                    H26X_SWAP(p_ctrl->job[job_idx].tmp_rec_c_addr2[i], pRegSet->TILE_CFG[7+i], UINT32);
                }
				//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
				//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
				//H26X_SWAP(p_ctrl->job[job_idx].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
				H26X_SWAP(p_ctrl->job[job_idx].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);
			}
		}
	}



	DBG_ERR("[%s][%d]\r\n",__FUNCTION__,__LINE__);
}

BOOL h26x_job_check_result(h26x_ctrl_t *p_ctrl, UINT32 interrupt, unsigned int rec_out_en)
{
    BOOL ret = TRUE;
	if (DIRECT_MODE)
	{
		int currect_job_idx = p_ctrl->currect_job_idx;
		if (p_ctrl->job[currect_job_idx].codec == 0)
			ret = emu_h265_chk_one_pic(&p_ctrl->job[currect_job_idx], interrupt, p_ctrl->job_num, rec_out_en);
        else if (p_ctrl->job[currect_job_idx].codec == 1)
			emu_h264_chk_one_pic(p_ctrl, &p_ctrl->job[currect_job_idx], interrupt, rec_out_en, p_ctrl->ver_item.cmp_bs_en);
		else if (p_ctrl->job[currect_job_idx].codec == 2)
			emu_h265d_chk_one_pic(&p_ctrl->job[currect_job_idx], interrupt);
        else if (p_ctrl->job[currect_job_idx].codec == 3){
			emu_h264d_chk_one_pic(&p_ctrl->job[currect_job_idx], interrupt, rec_out_en, p_ctrl->ver_item.rnd_bs_buf);
		}

		if (rec_out_en == 0)
		{
			H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[currect_job_idx].apb_addr;

			//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
			//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
			//H26X_SWAP(p_ctrl->job[0].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
			H26X_SWAP(p_ctrl->job[currect_job_idx].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
			H26X_SWAP(p_ctrl->job[currect_job_idx].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);
		}
	}
	else
	{
		unsigned int idx,i;

		for (i = 0; i < p_ctrl->job_num; i++)
		{
            idx = p_ctrl->list_order[i];
			if (p_ctrl->job[idx].is_finish == 0)
			{


				//hk
				{
					UINT32 *report = (UINT32 *)p_ctrl->job[idx].check1_addr;
					UINT32 *report2 = (UINT32 *)p_ctrl->job[idx].check2_addr;
					UINT32 *report3 = (UINT32 *)(p_ctrl->job[idx].apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
					UINT32 *report4 = (UINT32 *)(p_ctrl->job[idx].apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
					int j;
					h26x_flushCacheRead(p_ctrl->job[idx].check1_addr, H26X_REG_REPORT_SIZE);
					h26x_flushCacheRead(p_ctrl->job[idx].check2_addr, H26X_REG_REPORT_SIZE);
					for (j = 0; j < H26X_REPORT_NUMBER; j++) {
						report3[j] = report[j];
						report4[j] = report2[j];
					}
					h26x_flushCache(p_ctrl->job[idx].apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REG_REPORT_SIZE);
					h26x_flushCache(p_ctrl->job[idx].apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REG_REPORT_SIZE);

				}


				if (p_ctrl->job[idx].codec == 0)
				{
					ret = emu_h265_chk_one_pic(&p_ctrl->job[idx], interrupt, p_ctrl->job_num, rec_out_en);
				}else if (p_ctrl->job[idx].codec == 1){
					emu_h264_chk_one_pic(p_ctrl, &p_ctrl->job[idx], interrupt, rec_out_en, p_ctrl->ver_item.cmp_bs_en);
                }else if (p_ctrl->job[idx].codec == 2){
					emu_h265d_chk_one_pic(&p_ctrl->job[idx], interrupt);
				}else if (p_ctrl->job[idx].codec == 3){
                    emu_h264d_chk_one_pic(&p_ctrl->job[idx], interrupt, rec_out_en, p_ctrl->ver_item.rnd_bs_buf);
                }

				if (rec_out_en == 0)
				{
					H26XRegSet *pRegSet = (H26XRegSet *)p_ctrl->job[idx].apb_addr;

					//H26X_SWAP(p_ctrl->job[idx].tmp_tmnr_mtout_addr,  pRegSet->TMNR_CFG[57], UINT32);
					//H26X_SWAP(p_ctrl->job[idx].tmp_tmnr_rec_y_addr,  pRegSet->TMNR_CFG[54], UINT32);
					//H26X_SWAP(p_ctrl->job[idx].tmp_tmnr_rec_c_addr,  pRegSet->TMNR_CFG[55], UINT32);
					H26X_SWAP(p_ctrl->job[idx].tmp_osg_gcac_addr0,    pRegSet->OSG_CFG[0x484/4], UINT32);
					H26X_SWAP(p_ctrl->job[idx].tmp_osg_gcac_addr1,    pRegSet->OSG_CFG[0x480/4], UINT32);
                }
			}
		}
	}
	return ret;

}

BOOL check_recout_flow_all_job(h26x_ctrl_t *p_ctrl)
{
    BOOL ret = TRUE;
    int i,idx = 0;

	if (DIRECT_MODE)
	{
		int currect_job_idx = p_ctrl->currect_job_idx;
		if (p_ctrl->job[currect_job_idx].codec == 1)
        {
            ret = emu_h264_do_recout_flow( p_ctrl,&p_ctrl->job[currect_job_idx]);

        }
    }else{
		for (i = 0; i < (int)p_ctrl->job_num; i++)
		{
            idx = i;
			if (p_ctrl->job[idx].is_finish == 0)
			{
				if (p_ctrl->job[idx].codec == 1)
				{
                    ret = emu_h264_do_recout_flow( p_ctrl,&p_ctrl->job[idx]);
                    if(ret == FALSE)
                    {
                        return ret;
                    }
				}
			}
		}
    }
    return ret;

}

void h26x_job_excute(h26x_ctrl_t *p_ctrl, h26x_srcd2d_t *p_src_d2d)
{
	while(h26x_job_prepare_one_pic(p_ctrl,p_src_d2d) == 0)
	{
		UINT32 interrupt = 0;
		BOOL do_disrecout = 0;
		if (p_ctrl->ver_item.ddr_brst_en)
		{
            ddrburst = rand()%2;
			h26x_setDramBurstLen(ddrburst);
		}else{
            //h26x_setDramBurstLen(0);
            h26x_setDramBurstLen(1);
        }

        do_disrecout = check_recout_flow_all_job( p_ctrl);

		if (p_ctrl->ver_item.rec_out_dis == 1 && do_disrecout)
		{
			h26x_job_encode_one_pic(p_ctrl, 0);
			interrupt = h26x_job_check_interrupt(p_ctrl, 0);
			h26x_job_check_result(p_ctrl, interrupt, 0);

		}

		if (p_ctrl->ver_item.rnd_sw_rest)
		{
			h26x_job_swrset_encode_one_pic(p_ctrl);
		}

		// normal run //
		if (DIRECT_MODE)
        {
            unsigned int i = 0;

            for (i = 0; i < p_ctrl->job_num; i++)
            {
                //p_ctrl->currect_job_idx = i;
                p_ctrl->currect_job_idx = p_ctrl->list_order[i];
				if (p_ctrl->job[p_ctrl->currect_job_idx].is_finish == 0){//hk
                h26x_job_encode_one_pic(p_ctrl, 1);
                interrupt = h26x_job_check_interrupt(p_ctrl, 0);
                h26x_job_check_result(p_ctrl, interrupt, 1);
				}
            }

        }else{
            h26x_job_encode_one_pic(p_ctrl, 1);
            interrupt = h26x_job_check_interrupt(p_ctrl, 0);
            h26x_job_check_result(p_ctrl, interrupt, 1);
        }


	}
}
#if 1 //d2d


h26x_mem_t codec_mem;
h26x_mem_t codec_mem2;
h26x_ctrl_t codec_ctrl;

BOOL emu_codec_open(UINT32 uiVirMemAddr1 , UINT32 uiSize1,UINT32 uiVirMemAddr2 , UINT32 uiSize2, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d, UINT32 job_num, char src_path_d2d[2][128] )
{
    UINT32 i;
    codec_mem.start = uiVirMemAddr1;
    codec_mem.addr  = codec_mem.start;
    codec_mem.size  = uiSize1;

    codec_mem2.start = uiVirMemAddr2;
    codec_mem2.addr  = codec_mem2.start;
    codec_mem2.size  = uiSize2;

    DBG_INFO("dram1 mem = 0x%08x size = 0x%08x\r\n",(unsigned int)codec_mem.addr,(unsigned int)codec_mem.size);
    DBG_INFO("dram2 mem = 0x%08x size = 0x%08x\r\n",(unsigned int)codec_mem2.addr,(unsigned int)codec_mem2.size);

    h26x_job_ctrl_init(&codec_ctrl, &codec_mem, &codec_mem2);

	h26x_request_irq();
    h26x_create_resource();

    h26x_setAPBAddr(0xf0a10000);

    h26x_setRstAddr(0xf0020080);

    h26x_setClk(240);

    h26x_open();

    for(i = 0; i < job_num;i++)
    {
        if(p_src_d2d[i].is_hevc)
        {
            emu_h265_setup(&codec_ctrl,pucDir, &p_src_d2d[i], src_path_d2d);
        }else{
            emu_h264_setup(&codec_ctrl,pucDir, &p_src_d2d[i]);
        }
    }

    return TRUE;

}

void emu_codec_run( h26x_srcd2d_t *p_src_d2d)
{
    DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
    h26x_job_prepare_one_pic(&codec_ctrl, p_src_d2d);
    if (codec_ctrl.ver_item.ddr_brst_en)
    {
        h26x_setDramBurstLen(rand()%2);
    }else{
        h26x_setDramBurstLen(0);
    }
    h26x_job_encode_one_pic(&codec_ctrl, 1);
}

BOOL emu_codec_check(void)
{
    BOOL ret = TRUE;
    UINT32 interrupt = 0;
    DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);

    interrupt = h26x_job_check_interrupt(&codec_ctrl, 0);
    ret = h26x_job_check_result(&codec_ctrl, interrupt, 1);

    return ret;
}



#endif

//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)


