#include "ive_platform.h"
#if (IVE_SYS_VFY_EN == ENABLE)
#if defined(__FREERTOS)
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "kdrv_ive.h"
#include "kwrap/cpu.h"
#include "dma_protected.h"
#else
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include "kdrv_ive.h"
#include <plat-na51055/nvt-sramctl.h>
#include <comm/nvtmem.h>
#include "ive_drv.h"
#endif

#include "kwrap/task.h"
#include "ive_lib.h"
#include "ive_int.h"
#include "ive_sys_vfy.h"
#include "kwrap/type.h"
#include "comm/hwclock.h"
#include "ive_lib.h"
//#include <mach/fmem.h>
#include "kwrap/mem.h"
#include "ive_reg.h"
#include <kwrap/file.h>
#include "ive_platform.h" //chip id
#include "ive_ll_cmd.h"
#include "kdrv_ive.h"
#include "kdrv_ive_int.h"
#include "ive_dbg.h"

#if defined(__FREERTOS)
#else
#define IND                 0x00000001
#define WRN                 0x00000002
#define ERR                 0x00000004
#endif

#define IVE_CHECK_REG_RW_BY_OFS(a, b, c) do { \
    *(reg_data + ((a)/sizeof(UINT32))) = (b);\
    nvt_dbg(ERR, "IVE: 0x%x: 0x%x\r\n", (a), (UINT32) *(reg_data + ((a)/sizeof(UINT32))));\
    *(reg_data + ((a)/sizeof(UINT32))) = (c);\
    nvt_dbg(ERR, "IVE: 0x%x: 0x%x\r\n", (a), (UINT32) *(reg_data + ((a)/sizeof(UINT32))));\
} while(0)\

#define IVE_CEILING(a, n)  (((a) + ((n)-1)) & (~((n)-1)))
#define IVE_FLOOR(a,n)     ((a) & (~((n)-1)))
#define IVE_MOD(a,b)       ((a)%(b))

#define IVE_8_BYTE_ALIGN_CEILING(a)  IVE_CEILING(a,8)
#define IVE_4_BYTE_ALIGN_CEILING(a)  IVE_CEILING(a,4)
#define IVE_4_BYTE_ALIGNF_FLOOR(a)  IVE_FLOOR(a,4)
#define IVE_256_FLOOR(a) IVE_FLOOR(a, 256)
#define IVE_256_CEILING(a) IVE_CEILING(a, 256)


#define IVE_REG_NUMS 		0x64  //It has 0x64 in .dat file
#define IVE_MEM_SIZE       	0x3000000
#define IVE_MEM_SIZE_1     	0x7000000
#define IVE_LL_BUF_SIZE     0x1000000
#define IVE_MAX_RESERVE_BUF_NUM 5

#if defined(__FREERTOS)
extern UINT8 g_ive_dram_mode;
extern UINT32 g_ive_mem_base, g_ive_mem_base_2;
extern UINT32 g_ive_mem_size, g_ive_mem_size_2;
#define IVE_SWITCH_DRAM(a) IVE_4_BYTE_ALIGN_CEILING((((a) > 0x40000000) ? (((a) - g_ive_mem_base_2) + g_ive_mem_base) : (((a) - g_ive_mem_base) + g_ive_mem_base_2)))
#define IVE_SWITCH_DRAM_1(a) IVE_4_BYTE_ALIGN_CEILING((((a) > 0x40000000) ? (((a) - g_ive_mem_base_2) + g_ive_mem_base) : (a)))
#define IVE_SWITCH_DRAM_2(a) IVE_4_BYTE_ALIGN_CEILING((((a) > 0x40000000) ? (a) : (((a) - g_ive_mem_base) + g_ive_mem_base_2)))
#endif

#if defined(__FREERTOS)
#define POOL_ID_APP_ARBIT       0
#define POOL_ID_APP_ARBIT2      1
extern UINT32 OS_GetMempoolAddr(UINT32 id);
extern UINT32 OS_GetMempoolSize(UINT32 id);

#define emu_msg(msg)       printf msg
extern ER uart_getString(CHAR *pcString, UINT32 *pcBufferLen);
extern ER uart_getChar(CHAR *pcData);
#endif

VOID ive_hook(KDRV_IVE_TRIGGER_PARAM *trig_param);  //before trigger
VOID ive_hook1(VOID); //after trigger
VOID ive_hook2(unsigned int pc); //after frameend
VOID ive_ch_en_func(UINT8 mode);

UINT32 g_IVE_ENGINE_HOUSE[IVE_REG_NUMS] = {0x0};
volatile NT98520_IVE_REG_STRUCT *g_pIveReg_drv = (NT98520_IVE_REG_STRUCT *)g_IVE_ENGINE_HOUSE;
volatile NT98528_IVE_REG_STRUCT *g_pIveReg_drv_528 = (NT98528_IVE_REG_STRUCT *)g_IVE_ENGINE_HOUSE;
static UINT32 kdrv_ive_fmd_flag = 0;


extern VOID ive_prepare_ll_buffer(UINT64 **llbuf, UINT8 is_bit60, UINT32 ll_fill_reg_num, UINT32 ll_fill_num, UINT8 is_switch_dram, UINT8 is_err_cmd,
							UINT32 in_addr, UINT32 out_addr, UINT32 *out_buff_addr, UINT32 *out_buff_size, UINT32 out_buff_num,
							UINT32 gld_buf_size);
extern int ive_cmp_data(unsigned int rlt_addr, unsigned int size, unsigned int gld_addr, unsigned int pc);

static UINT8 g_is_gld_cmp=1;
static UINT32 g_gld_addr;
static UINT32 g_gld_size;
#if defined(__FREERTOS)
UINT32 g_ive_mem_base=0, g_ive_mem_base_2=0;
UINT32 g_ive_mem_size=0, g_ive_mem_size_2=0;
UINT32 g_ive_in_addr=0, g_ive_out_addr=0;
UINT32 g_ive_in_size=0, g_ive_out_size=0;
UINT8 g_ive_dram_mode=2;
static UINT32 g_ive_outbuf_size=0;
static UINT8 g_clock=1;
static const UINT32 g_clock_val[4]={600, 480, 320, 240};
static UINT32 ive_dram_addr_res[IVE_MAX_RESERVE_BUF_NUM];
static UINT32 ive_dram_size_res[IVE_MAX_RESERVE_BUF_NUM];
static UINT8 g_pri_mode=0;
static UINT8 g_is_heavy=0;
static UINT8 g_ll_mode_backup=0;
static UINT8 g_ll_dump=1;
static UINT64 *g_ll_buf=0;
static UINT32 g_ll_fill_reg_num = 1;
static UINT32 g_ll_fill_num = IVE_MAX_RESERVE_BUF_NUM;
static UINT8 g_is_switch_dram=0;
static UINT8 g_ive_ll_next_update=0;
static UINT8 g_wp_en=0;
static UINT8 g_wp_sel_id=0;
static UINT8 g_dram_outrange=0;
static UINT8 g_auto_clk=0;
static UINT8 g_sram_down=0;
static UINT8 g_rand_burst=0;
#endif
static UINT8 g_ll_mode=0;
static UINT8 g_ll_terminate=0;
static UINT8 g_interrupt_en=1;
static UINT8 g_clk_en=0;
static UINT8 g_reg_rw=0;
static UINT8 g_no_stop=0;

#define IVE_ALL_CASES 14513
static unsigned int ive_bringup_table[10] = {1, 10, 20, 30, 40, 50, 60, 70, 80, 90};
static unsigned int ive_bringup_table_all[IVE_ALL_CASES];
static UINT8 g_bringup_en=0;


static UINT8 g_hw_rst_en=0;

VOID ive_rst_func(UINT8 mode)
{
	g_hw_rst_en = mode;
}


#if defined(__FREERTOS)
static VOID ive_set_heavy_func(UINT8 heavy_mode, UINT8 dram_mode);
VOID ive_set_ll_mode_en(UINT8 is_en)
{
    g_ll_mode = is_en;
    nvt_dbg(ERR, "IVE:ll_mode_en(%d)\r\n", g_ll_mode);
}

VOID ive_set_dram_mode(UINT32 dram_mode) //2:dram_2, 1:dram_1
{
	g_ive_dram_mode = dram_mode;
	nvt_dbg(ERR, "IVE: dram_mode(%d)\r\n", g_ive_dram_mode);
}

VOID ive_set_heavy_mode(UINT8 mode) //0:heavy, 1:channel disable/enable
{
	g_is_heavy = mode;
	nvt_dbg(ERR, "IVE: heavy_mode(%d)\r\n", g_is_heavy);
	ive_set_heavy_func(g_is_heavy, g_ive_dram_mode);
}
#endif

unsigned int ive_get_index(unsigned int idx)
{
	unsigned int index;

	if (g_bringup_en >= 1) {
		if (g_bringup_en == 2) {
			index = ive_bringup_table_all[idx];
		} else {
			index = ive_bringup_table[idx];
		}
	} else {
		index = idx;
	}

	return index;
}

void ive_check_reg_rw(void)
{
    volatile UINT32 *reg_data = (UINT32 *) 0xf0d70000; //iveA

    IVE_CHECK_REG_RW_BY_OFS(0x4, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x8, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0xc, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x10, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x14, 0x5A5A5A5A, 0xA5A5A5A5);
    IVE_CHECK_REG_RW_BY_OFS(0x18, 0x5A5A5A5A, 0xA5A5A5A5);

    IVE_CHECK_REG_RW_BY_OFS(0x1c, 0x5A5A5A5A, 0xA5A5A5A5);
    IVE_CHECK_REG_RW_BY_OFS(0x20, 0x5A5A5A5A, 0xA5A5A5A5);
    IVE_CHECK_REG_RW_BY_OFS(0x24, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x28, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x2c, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x30, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x34, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x38, 0xFFFFFFFF, 0x0);

    IVE_CHECK_REG_RW_BY_OFS(0x3c, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x40, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x44, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x48, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x4c, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x50, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x54, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x58, 0xFFFFFFFF, 0x0);

    IVE_CHECK_REG_RW_BY_OFS(0x5c, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x60, 0x5A5A5A5A, 0xA5A5A5A5);
    IVE_CHECK_REG_RW_BY_OFS(0x64, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x68, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x6c, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x70, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x74, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x78, 0xFFFFFFFF, 0x0);

    IVE_CHECK_REG_RW_BY_OFS(0x7c, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x80, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x84, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x88, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x8c, 0xFFFFFFFF, 0x0);
    IVE_CHECK_REG_RW_BY_OFS(0x90, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x94, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x98, 0xFFFFFFFF, 0x0);
	IVE_CHECK_REG_RW_BY_OFS(0x9C, 0xFFFFFFFF, 0x0);

    return;
}



VOID ive_engine_terminate(UINT8 mode)
{
	if (mode == 1) {
		ive_ll_terminate(ENABLE);
		nvt_dbg(ERR, "IVE: LL terminate (Begin)\r\n");
	} else {
		ive_ll_terminate(DISABLE);
	}
}

VOID ive_engine_rst(UINT8 mode)
{
	UINT32 ch_idle=0;
	UINT32 count;
	UINT32 reg_data=0;

	//Module Reset Control Register 0
	//AXI
	volatile UINT32 *reg_ive_dma = (UINT32 *) 0xf0d70068;
	volatile UINT32 *reg_sys_rst = (UINT32 *) 0xf0020080;
	volatile UINT32 *reg_sw_rst = (UINT32 *) 0xf0d70000;

	reg_data = *reg_ive_dma;
	reg_data = reg_data | (1<<30); //disable dma
	*reg_ive_dma = reg_data;

	ch_idle = 0;
	count = 0;
	while (ch_idle == 0) {
		reg_data = *reg_ive_dma;
		ch_idle = ((reg_data & (1<<31)) >> 31);
		vos_task_delay_ms(10);
		if (count++ > 200) {
			nvt_dbg(ERR, "HW_RST_1: ive ch is still not in idle.\r\n");
			break;
		}
	}

	if (mode == 1) {
		//reset
		if (ch_idle == 1) {
			reg_data  = *reg_sys_rst;
			reg_data &= ~(1 << (12));
			*reg_sys_rst = reg_data;

			//Released
			vos_task_delay_ms(100);
			reg_data |= 1 << (12);
			*reg_sys_rst = reg_data;
			nvt_dbg(ERR, "HW_RST(Done)..\r\n");
		} else {
			nvt_dbg(ERR, "HW_RST: Error to HW_RST (not idle state).\r\n");
		}
		
	} else if (mode == 2) {
		//reset
		if (ch_idle == 1) {
			reg_data = *reg_sw_rst;
			reg_data |= 1;
			*reg_sw_rst = reg_data;

			//Released
			vos_task_delay_ms(100);
			reg_data &= (~(1));
			*reg_sw_rst = reg_data;

			reg_data = *reg_ive_dma;
			reg_data &=  (~(1<<30));
			*reg_ive_dma = reg_data;

			nvt_dbg(ERR, "SW_RST(Done)..\r\n");
		} else {
			nvt_dbg(ERR, "SW_RST: Error to SW_RST (not idle state).\r\n");
		}
	}
}

VOID kdrv_ive_hook_func(UINT32 hook_mode, UINT32 hook_value)
{
	if (hook_mode == (UINT32) IVE_TERMINATE) {
		ive_engine_terminate((UINT8) hook_value);	
	} else if (hook_mode == (UINT32) IVE_RST) {
		ive_engine_rst((UINT8) hook_value);
	} else if (hook_mode == (UINT32) IVE_CLK_EN) {
		ive_ch_en_func((UINT8) hook_value);
	} else {
		DBG_ERR("IVE(HOOK): Error, the mode is not supported.\r\n");
	}
}

VOID kdrv_ive_null_func(UINT32 hook_mode, UINT32 hook_value)
{
	return;
}

#if defined(__FREERTOS)
VOID ive_write_protect_test(UINT8 wp_en, UINT8 sel_id, UINT32 in_addr, UINT32 in_size, UINT32 out_addr, UINT32 out_size)
{
	DMA_WRITEPROT_ATTR attr = {0};
	UINT32 dram_mode;

	if (g_ive_dram_mode == 2) {
		dram_mode = 1; //dram_2
	} else {
		dram_mode = 0; //dram_1
	}

	if (wp_en == 1) { //in range protect
		//input
		nvt_dbg(ERR, "WP: in range protect (DMA_RPLEL_UNREAD/DMA_PROT_IN)\r\n");
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RPLEL_UNREAD;
		attr.protect_mode = DMA_PROT_IN;

		nvt_dbg(ERR, "WP_in_range_p:sel_id=%d\r\n", sel_id);

		if (sel_id == 0 || sel_id == 5) {
			attr.mask.IVE_0 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(in_addr));  //physical addressAA
			attr.protect_rgn_attr[0].size = IVE_256_FLOOR(in_size);
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			nvt_dbg(ERR, "wp_in_range_p(WPSET_0): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		nvt_dbg(ERR, "WP: in range protect (DMA_WPLEL_UNWRITE/DMA_PROT_IN)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_UNWRITE;
        attr.protect_mode = DMA_PROT_IN;

		if (sel_id == 1 || sel_id == 5) {
			attr.mask.IVE_1 = 1;	
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(out_addr));  //physical addressA
			attr.protect_rgn_attr[0].size = IVE_256_FLOOR(out_size);
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			nvt_dbg(ERR, "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

	}

	if (wp_en == 2) { //out range protect read
		nvt_dbg(ERR, "WP: out range protect read (DMA_RPLEL_UNREAD/DMA_PROT_OUT)\r\n");
		//input
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.mask.IVE_0 = 1;
		attr.level = DMA_RPLEL_UNREAD;
        attr.protect_mode = DMA_PROT_OUT;
	
		attr.protect_rgn_attr[0].en = 1;	
		attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(in_addr - 0x1000));  //physical addressA
		attr.protect_rgn_attr[0].size = 0x100;
		arb_enable_wp(dram_mode, WPSET_0, &attr);
		nvt_dbg(ERR, "wp_out_range_p(OUT_WP): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
	}

	if (wp_en == 3) { //out range protect write
		nvt_dbg(ERR, "WP: out range protect write (DMA_WPLEL_UNWRITE/DMA_PROT_OUT) \r\n");
		//Output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_UNWRITE;
        attr.protect_mode = DMA_PROT_OUT;

		attr.mask.IVE_1 = 1;
		attr.protect_rgn_attr[0].en = 1;	
		attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(out_addr - 0x1000));  //physical addressA out range
		attr.protect_rgn_attr[0].size = 0x100;
		arb_enable_wp(dram_mode, WPSET_0, &attr);

		nvt_dbg(ERR, "wp_out_range_p(OUT_WP): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
	
	}

	if (wp_en == 4) { //in range read/write denial access
		nvt_dbg(ERR, "WP: in range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_IN)\r\n");
		//input
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RWPLEL_UNRW;
        attr.protect_mode = DMA_PROT_IN;
		
		if (sel_id == 0 || sel_id == 2) {
			attr.mask.IVE_0 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(in_addr));  //physical addressA
			attr.protect_rgn_attr[0].size = IVE_256_FLOOR(in_size);
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			nvt_dbg(ERR, "wp_in_range_p(WPSET_0): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		nvt_dbg(ERR, "WP: in range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_IN)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RWPLEL_UNRW;
        attr.protect_mode = DMA_PROT_IN;

		if (sel_id == 1 || sel_id == 2) {
			attr.mask.IVE_1 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(out_addr));  //physical addressA
			attr.protect_rgn_attr[0].size = IVE_256_FLOOR(out_size);
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			nvt_dbg(ERR, "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}
		
	}

	if (wp_en == 5) { //out range read/write denial access
		nvt_dbg(ERR, "WP: out range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_OUT)\r\n");
		//input
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RWPLEL_UNRW;
        attr.protect_mode = DMA_PROT_OUT;

		if (sel_id == 0 || sel_id == 5) {
			attr.mask.IVE_0 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(in_addr - 0x1000));  //physical addressA
			attr.protect_rgn_attr[0].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			nvt_dbg(ERR, "wp_in_range_p(WPSET_0): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		nvt_dbg(ERR, "WP: out range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_OUT)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RWPLEL_UNRW;
        attr.protect_mode = DMA_PROT_OUT;

		if (sel_id == 1 || sel_id == 5) {
			attr.mask.IVE_1 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(out_addr - 0x1000));  //physical addressA
			attr.protect_rgn_attr[0].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			nvt_dbg(ERR, "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

	}

	if (wp_en == 6) { //in range write detect
		nvt_dbg(ERR, "WP: in range write detect (DMA_WPLEL_DETECT/DMA_PROT_IN)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_DETECT;
        attr.protect_mode = DMA_PROT_IN;

		attr.mask.IVE_1 = 1;
		attr.protect_rgn_attr[0].en = 1;
		attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(out_addr));  //physical addressA
		attr.protect_rgn_attr[0].size = IVE_256_FLOOR(out_size);
		arb_enable_wp(dram_mode, WPSET_0, &attr);
		nvt_dbg(ERR, "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);

	}

	if (wp_en == 7) { //out range write detect
		nvt_dbg(ERR, "WP: out range write detect (DMA_WPLEL_DETECT/DMA_PROT_OUT)\r\n");
		//Output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_DETECT;
        attr.protect_mode = DMA_PROT_OUT;

		attr.mask.IVE_1 = 1;	
		attr.protect_rgn_attr[0].en = 1;	
		attr.protect_rgn_attr[0].starting_addr = ive_platform_va2pa(IVE_256_CEILING(out_addr - 0x1000));  //physical addressA
		attr.protect_rgn_attr[0].size = 0x100;
		arb_enable_wp(dram_mode, WPSET_0, &attr);
		nvt_dbg(ERR, "wp_out_range_p(OUT_WP): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
	}
}
#endif

VOID ive_hook(KDRV_IVE_TRIGGER_PARAM *trig_param)
{
#if defined(__FREERTOS)
	ER erReturn = E_OK;
	UINT64 *ll_buf_tmp;
	UINT64 *tmp_buf;
	UINT32 tmp_idx;
	volatile UINT32 *reg_auto_clk= (UINT32 *) 0xf00200b4;
	volatile UINT32 *reg_p_auto_clk= (UINT32 *) 0xf00200c4;
	volatile UINT32 *reg_sram_down = (UINT32 *) 0xf0011000;
	UINT32 auto_clk_val;
	UINT32 reg_val;

#if defined(__FREERTOS)
	srand((unsigned) hwclock_get_counter());
#else
	DBG_ERR("IVE:Error rand TODO..\r\n");
#endif

	//nvt_dbg(ERR, "ive_hook: begin\r\n");

	if (g_pri_mode == 4) {
		dma_setChannelPriority(DMA_CH_IVE_0, rand() % 4);
		dma_setChannelPriority(DMA_CH_IVE_1, rand() % 4);
	} else {
		dma_setChannelPriority(DMA_CH_IVE_0, g_pri_mode);
		dma_setChannelPriority(DMA_CH_IVE_1, g_pri_mode);
	}

	if (g_clock == 4) {
		erReturn = ive_platform_set_clk_rate(g_clock_val[rand() % 4]);
		if (erReturn != E_OK) {
			DBG_ERR("Error to set ive_platform_set_clk_rate().\r\n");
		}
	} else {
		erReturn = ive_platform_set_clk_rate(g_clock_val[g_clock]);
		if (erReturn != E_OK) {
			DBG_ERR("Error to set ive_platform_set_clk_rate().\r\n");
		}
	}

	if (g_ll_mode == 1) {
		trig_param->is_nonblock = 1;
		//nvt_dbg(ERR, "ive_hook: LL_Mode begin\r\n");
		ll_buf_tmp = (UINT64 *) g_ll_buf;
		ive_prepare_ll_buffer(&ll_buf_tmp, 0, g_ll_fill_reg_num, g_ll_fill_num, g_is_switch_dram, g_ive_ll_next_update,
							g_ive_in_addr, g_ive_out_addr, ive_dram_addr_res, ive_dram_size_res, IVE_MAX_RESERVE_BUF_NUM,
							g_gld_size);

		if (g_ll_dump) {
			//nvt_dbg(ERR, "ive_hook: LL_dump begin\r\n");
			tmp_buf = (UINT64 *) g_ll_buf;

			tmp_idx = 0;
			while(((*(tmp_buf + tmp_idx)) & ((UINT64)(~0xFF))) != 0) {
				while (((*(tmp_buf + tmp_idx) & ((UINT64)0xF<<60)) >> 60) == 0x4) { //next_upd
					nvt_dbg(ERR, "LL_BUF_TXT(0x%x): 0x%llx\r\n", ((UINT32) tmp_buf + (tmp_idx*8)), *(tmp_buf + tmp_idx));
					tmp_buf = (UINT64 *) ((UINT32) (*(tmp_buf + tmp_idx) >> 8));
					tmp_idx=0;
					if (((*(tmp_buf + tmp_idx)) & ((UINT64)(~0xFF))) == 0) {
						break;
					}
				}
				nvt_dbg(ERR, "LL_BUF_TXT(0x%x): 0x%llx\r\n", ((UINT32) tmp_buf + (tmp_idx*8)), *(tmp_buf + tmp_idx));
				if (((*(tmp_buf + tmp_idx)) & ((UINT64)(~0xFF))) != 0) {
					tmp_idx++;
				}
			}
			nvt_dbg(ERR, "LL_BUF_TXT(0x%x): 0x%llx\r\n", ((UINT32) tmp_buf + (tmp_idx*8)), *(tmp_buf + tmp_idx));
			nvt_dbg(ERR, "exit: LL_BUF_TXT\r\n");
		}

		ive_flush_buf_ll((UINT64 *)IVE_SWITCH_DRAM_1((UINT32)g_ll_buf), (UINT64 *)IVE_SWITCH_DRAM_1((UINT32)ll_buf_tmp));
		ive_flush_buf_ll((UINT64 *)IVE_SWITCH_DRAM_2((UINT32)g_ll_buf), (UINT64 *)IVE_SWITCH_DRAM_2((UINT32)ll_buf_tmp));

		nvt_dbg(ERR, "ll_buf_size=0x%x\r\n", ((UINT32)ll_buf_tmp - (UINT32)g_ll_buf));
	} else if (g_ll_mode == 2) {
		trig_param->is_nonblock = 1;
	} else {
		trig_param->is_nonblock = 0;
	}

	if (g_wp_en) {
		ive_write_protect_test(g_wp_en, g_wp_sel_id, g_ive_in_addr, g_ive_in_size, g_ive_out_addr, g_ive_out_size);
	}

	if (g_auto_clk >= 1) {
		auto_clk_val = *reg_auto_clk;
		*reg_auto_clk = auto_clk_val | (1 << 28);

		auto_clk_val = *reg_p_auto_clk;
		*reg_p_auto_clk = auto_clk_val | (1 << 28);

		if (g_auto_clk == 2) { //disable
			auto_clk_val = *reg_auto_clk;
        	*reg_auto_clk = auto_clk_val & (~(1 << 28));

        	auto_clk_val = *reg_p_auto_clk;
        	*reg_p_auto_clk = auto_clk_val & (~(1 << 28));
		}
		nvt_dbg(ERR, "IVE_AUTO(bit28): clk_auto(0x%x) apb_clk_auto(0x%x)\r\n", *reg_auto_clk, *reg_p_auto_clk);
	}

	if (g_sram_down > 0) {
		reg_val = *reg_sram_down;
        reg_val = reg_val | (1<<23);
        *reg_sram_down = reg_val;
        nvt_dbg(ERR, "IVE_SRAM_DOWN(b23)(ON): reg(0x%x)\r\n", *reg_sram_down);
        if (g_sram_down == 2) {
            reg_val = *reg_sram_down;
            reg_val = reg_val & (~(1<<23));
            *reg_sram_down = reg_val;
            nvt_dbg(ERR, "IVE_SRAM_DOWN(b23)(OFF): reg(0x%x)\r\n", *reg_sram_down);
        }	
	} 

	if (g_rand_burst < 4) {
		ive_setInBurstLengthReg(g_rand_burst);
		ive_setOutBurstLengthReg(g_rand_burst);
	} else {
		ive_setInBurstLengthReg((rand() % 4));
		ive_setOutBurstLengthReg((rand() % 4));
	}	
#endif
}

VOID ive_ch_en_func(UINT8 mode)
{
#if defined(__FREERTOS)
	volatile UINT32 *reg_clk_en = (UINT32 *) 0xf002007c;
    UINT32 reg_value;

	if (mode > 0) {
		if (mode == 1) {
			reg_value = *reg_clk_en;
			reg_value = reg_value & (~(1<<25));
			*reg_clk_en = reg_value;
			nvt_dbg(ERR, "IVE_CLK_EN:begin to testing reg_clk-0x%x\r\n", *reg_clk_en);
			vos_task_delay_ms(10000);
			reg_value = *reg_clk_en;
			reg_value = reg_value | (1<<25);
			*reg_clk_en = reg_value;
			nvt_dbg(ERR, "IVE_CLK_EN it should be delay to 10 sec. reg_clk=0x%x\r\n", *reg_clk_en);
		} else if (mode == 2) {
			reg_value = *reg_clk_en;
            reg_value = reg_value & (~(1<<25));
            *reg_clk_en = reg_value;
            nvt_dbg(ERR, "IVE_CLK_EN(fail):begin to testing reg_clk-0x%x\r\n", *reg_clk_en);
		} else if (mode == 3) {
			reg_value = *reg_clk_en;
            reg_value = reg_value | (1<<25);
            *reg_clk_en = reg_value;
            nvt_dbg(ERR, "IVE_CLK_EN(OK): it should be delay to 10 sec. reg_clk=0x%x\r\n", *reg_clk_en);
		}
    }
#endif

	return;

}

VOID ive_hook1(VOID)
{
	ive_ch_en_func(g_clk_en);
	return;
}

VOID ive_hook2(unsigned int pc)
{
#if defined(__FREERTOS)
	UINT32 cmp_idx;
	int len;
	volatile UINT32 *ll_err_addr = (UINT32 *) 0xf0d70088;
    volatile UINT32 *ll_err_cnt = (UINT32 *) 0xf0d70084;

	if (g_ll_mode == 1) {
		ive_platform_dma_flush(ive_dram_addr_res[IVE_MAX_RESERVE_BUF_NUM - 1], ive_dram_size_res[IVE_MAX_RESERVE_BUF_NUM - 1]);
		for (cmp_idx = 0; cmp_idx < (IVE_MAX_RESERVE_BUF_NUM - 1); cmp_idx++) {
			if (g_dram_outrange == 1) {
				continue;
			}
			nvt_dbg(ERR, "IVE:...begine to compare ll_golden: cmp_idx(%d)\r\n", cmp_idx);
			ive_platform_dma_flush(ive_dram_addr_res[cmp_idx], ive_dram_size_res[cmp_idx]);
			len = ive_cmp_data(ive_dram_addr_res[cmp_idx], ive_dram_size_res[IVE_MAX_RESERVE_BUF_NUM - 1], ive_dram_addr_res[IVE_MAX_RESERVE_BUF_NUM - 1], pc);
			if (len < 0) {
				nvt_dbg(ERR, "%s_%d:Error, ive_cmp_data(): ret(%d)\r\n", __FUNCTION__, __LINE__, len);
			}
		}
	}

	if (g_ive_ll_next_update == 2) {
		nvt_dbg(ERR, "LL_ERR_ADDR(0x%x) LL_ERR_CNT(0x%x)\r\n", *ll_err_addr, *ll_err_cnt);
	}
#endif


	return;
}

int ive_cmp_data(unsigned int rlt_addr, unsigned int size, unsigned int gld_addr, unsigned int pc)
{
    unsigned int len=0;
    int c_idx;
    unsigned char *rlt_ptr = (unsigned char *) (rlt_addr);
    unsigned char *gld_ptr = (unsigned char *) (gld_addr);
	unsigned int dump_num;

#if defined(__FREERTOS)
	if (g_wp_en > 0) {
		dump_num = 512;
	} else {
		dump_num = 1;
	}
#else
	dump_num = 1;
#endif

    for(c_idx = 0; (unsigned int)c_idx < size; c_idx++) {
        if (rlt_ptr[c_idx] != gld_ptr[c_idx]) {
            DBG_ERR("Error, compare fail at c_idx(0x%x) rlf_addr(0x%p) gld_addr(0x%p) value=0x%x vs 0x%x at index(%d)\r\n",
                            c_idx, &rlt_ptr[c_idx], &gld_ptr[c_idx], rlt_ptr[c_idx], gld_ptr[c_idx], pc);
            len = -1;
			dump_num--;
			if (dump_num == 0) {
            	goto retend;
			}
        }
    }

    len = size;

retend:
    return len;
}

#if defined(__FREERTOS)
VOID ive_engine_setdata_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value, UINT32 ll_idx, UINT8 is_bit60, UINT32 ll_fill_reg_num, UINT8 is_switch_dram, UINT8 is_err_cmd)
{
	UINT32 fill_num;
	UINT64 *ll_buf_tmp;
	//UINT64 *ll_buf_tmp_start;
	UINT8 is_switch_buff;
	static UINT8 is_first=1;
	static UINT8 dram_mode=2;

	if (is_first == 1) {
		dram_mode = g_ive_dram_mode;
		is_first = 0;
	}

	//nvt_dbg(ERR, "LL_SWITCH_SETL: is_switch(%d) is_bit60(%d)\r\n", is_switch_dram, is_bit60);

	if ((is_switch_dram == 1) && (is_bit60 == 0)) {
		if (dram_mode == 2) {
			dram_mode = 1;
			ll_buf_tmp = (UINT64 *) IVE_SWITCH_DRAM_1((UINT32) *llbuf);
			nvt_dbg(ERR, "LL_dram1: 0x%x <- 0x%x\r\n", (UINT32)ll_buf_tmp, (UINT32)*llbuf);
		} else {
			dram_mode = 2;
			ll_buf_tmp = (UINT64 *) IVE_SWITCH_DRAM_2((UINT32) *llbuf);
			nvt_dbg(ERR, "LL_dram2: 0x%x <- 0x%x\r\n", (UINT32)ll_buf_tmp, (UINT32)*llbuf);
		}
		is_switch_buff = 1;
	} else {
		ll_buf_tmp = (UINT64 *) *llbuf;
		is_switch_buff = 0;
	}
	//ll_buf_tmp_start = ll_buf_tmp;

	//nvt_dbg(ERR, "LL_SWITCH_SETL: is_switch_buff(%d)\r\n", is_switch_buff);

	if (is_switch_buff == 0) { 
	} else {
		(ll_buf_tmp)++;
		**llbuf = ive_ll_nextupd_cmd(ive_platform_va2pa((UINT32) ll_buf_tmp));
		//nvt_dbg(ERR, "LL_SWITCH: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (*llbuf), (UINT64) **llbuf);
		(*llbuf)++;
	}

	for (fill_num = 0; fill_num < ll_fill_reg_num; fill_num++) {
		ive_setdata_ll(&ll_buf_tmp, r_ofs, ive_platform_va2pa(r_value), 0);
		//nvt_dbg(ERR, "LL_SWITCH_SET_LL: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
		if (is_err_cmd == 2) {
			if (fill_num == (ll_fill_reg_num / 2)) {
				*ll_buf_tmp = 0xffff000000000000;
				ll_buf_tmp++;
				is_err_cmd = 0;
			}
		}
	}

	if (is_switch_buff == 1) {//is_bit60==0
		ive_setdata_end_ll(&ll_buf_tmp, ll_idx, 0);
		if (g_ive_dram_mode == 2) {
			*llbuf = (UINT64 *) IVE_SWITCH_DRAM_2((UINT32) ll_buf_tmp);
			nvt_dbg(ERR, "LL_g_dram2_exit: 0x%x <- 0x%x\r\n", (UINT32) *llbuf, (UINT32) ll_buf_tmp);
		} else {
			*llbuf = (UINT64 *) IVE_SWITCH_DRAM_1((UINT32) ll_buf_tmp);
			nvt_dbg(ERR, "LL_g_dram1_exit: 0x%x <- 0x%x\r\n", (UINT32) *llbuf, (UINT32) ll_buf_tmp);
		}
        *ll_buf_tmp = ive_ll_nextupd_cmd(ive_platform_va2pa((UINT32) *llbuf));
		ll_buf_tmp++;
		//ive_platform_dma_flush((UINT32) ll_buf_tmp_start, (UINT32) (ll_buf_tmp - ll_buf_tmp_start));
		//nvt_dbg(ERR, "LL_SWITCH: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp), (UINT64) *ll_buf_tmp);
	} else { //is_bit60==1
		if (is_bit60 == 0) {
			if (is_err_cmd == 1) {
				ive_setdata_end_ll(&ll_buf_tmp, ll_idx, is_err_cmd);
			} else {
				ive_setdata_end_ll(&ll_buf_tmp, ll_idx, 0);
			}
		} else {
			ive_setdata_ll(&ll_buf_tmp, r_ofs, ive_platform_va2pa(r_value), 1);
		}
		//nvt_dbg(ERR, "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
		//ive_platform_dma_flush((UINT32) ll_buf_tmp_start, (UINT32) (ll_buf_tmp - ll_buf_tmp_start));
		*llbuf = ll_buf_tmp;
		nvt_dbg(ERR, "LL_fill_exit: llbuf(0x%x), ll_buf_tmp(0x%x)\r\n", (UINT32) *llbuf, (UINT32) ll_buf_tmp);
	}
	
}

VOID ive_prepare_ll_buffer(UINT64 **llbuf, UINT8 is_bit60, UINT32 ll_fill_reg_num, UINT32 ll_fill_num, UINT8 is_switch_dram, UINT8 is_err_cmd,
							UINT32 in_addr, UINT32 out_addr, UINT32 *out_buff_addr, UINT32 *out_buff_size, UINT32 out_buff_num,
							UINT32 gld_buf_size)
{
	UINT64 *ll_buf_tmp= (UINT64 *) *llbuf;
	UINT32 reg_dram_sai0= in_addr; //0xf0d50008; 
	UINT32 reg_dram_sao0= out_addr; //0xf0d50018;
	UINT32 backup_addr=0;
	UINT32 backup_addr_sao0=0;
	UINT32 ll_idx;
	UINT32 fill_num;
	UINT32 idx;

	ive_set_start_addr_ll((UINT32) *llbuf);

	backup_addr = reg_dram_sai0;
	backup_addr_sao0 = reg_dram_sao0;
	out_buff_addr[out_buff_num - 1] = backup_addr_sao0;
	out_buff_size[out_buff_num - 1] = gld_buf_size;
	for (idx=0; idx < out_buff_num; idx++) { //output_num: IVE_MAX_RESERVE_BUF_NUM
		if (g_dram_outrange == 1) {
			continue;
		}
		if (out_buff_size[idx] == 0) nvt_dbg(ERR, "Error, the reserved size is 0. at %s\r\n", __FUNCTION__);
		out_buff_size[idx] = gld_buf_size; //for 0x18
		memset((void *) out_buff_addr[idx], 0, out_buff_size[idx]);
		ive_platform_dma_flush(out_buff_addr[idx], out_buff_size[idx]);
		//nvt_dbg(ERR, "%s_%d: here flush after\r\n", __FUNCTION__, __LINE__);
	}

	ll_idx = 0;

	for (fill_num=0; fill_num < ll_fill_num; fill_num++) {
		ive_engine_setdata_ll(&ll_buf_tmp, 0x1c, out_buff_addr[(fill_num % out_buff_num)], ll_idx++, is_bit60, ll_fill_reg_num, is_switch_dram, is_err_cmd);
	}

	ive_engine_setdata_ll(&ll_buf_tmp, 0x1c, backup_addr_sao0, ll_idx++, is_bit60, ll_fill_reg_num, 0, 0);
	//nvt_dbg(ERR, "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
	ive_engine_setdata_ll(&ll_buf_tmp, 0x14, backup_addr, ll_idx++, is_bit60, ll_fill_reg_num, 0, 0);
	//nvt_dbg(ERR, "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
	ive_setdata_exit_ll(&ll_buf_tmp, 0);
	//nvt_dbg(ERR, "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));

	*llbuf = ll_buf_tmp;

	return;
}
#endif

INT32 KDRV_IVE_ISR_CB(UINT32 handle, UINT32 sts, void* in_data, void* out_data)
{
	if ((sts & IVE_INT_FRM_END)||(sts & IVE_INT_LLEND)) {
		kdrv_ive_fmd_flag = 1;
	}
	return 0;
}

VOID ive_load_golden_data(int pc, UINT32 gld_addr, UINT32 gld_size)
{
	char cFilePath[100];
	int fd;
	int len;
#if 0
	char *dump_addr;
#endif

	// save image //
	sprintf(cFilePath, "/mnt/sd/IVEP/1_All/04_DO_0/ive%05d.bin", pc);
	fd = vos_file_open(cFilePath, O_RDONLY, 0);
	if ((VOS_FILE)(-1)==fd) {
		DBG_ERR("failed in file open:%s\n", cFilePath);
		goto EOP;
	}

	len = vos_file_read(fd, (void *)gld_addr, gld_size);
	if (len == 0) {
		DBG_ERR("%s:%dfail to FileSys_ReadFile. ret(%d) addr(0x%x) size(0x%x) pc(%d)\r\n", __FILE__, __LINE__, len, gld_addr, gld_size, pc);
		//return -1;
	}

#if 0
	dump_addr = (char *) gld_addr;
	DBG_EMU("IVE: golden data=0x%x 0x%x 0x%x 0x%x\r\n", dump_addr[0], dump_addr[1], dump_addr[2], dump_addr[3]);
#endif

	vos_file_close(fd);

EOP:

	return;
}

#if defined(__FREERTOS)
int nvt_kdrv_ipp_api_ive_test(void)
#else
int nvt_kdrv_ipp_api_ive_test(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
#endif
{

    //struct nvt_fmem_mem_info_t yin_buf_info ={0} ;
	//struct nvt_fmem_mem_info_t yout_buf_info ={0} ;
    //void *hdl_in_buf_y = NULL;
	//void *hdl_out_buf_y = NULL;

#if defined(__FREERTOS)
	UINT32 mem_base;
	UINT32 mem_size;
	UINT32 size;
	unsigned long tmp_long;
	UINT32 tmp_val1;
	volatile UINT32 *mod_cnt = (UINT32 *) 0xf0d70054;
	volatile UINT32 *nmod_cnt = (UINT32 *) 0xf0d70058;
	volatile UINT32 *cycle_cnt = (UINT32 *) 0xf0d70090;
	volatile UINT32 *ll_cycle_cnt = (UINT32 *) 0xf0d70094;
#else
	struct  vos_mem_cma_info_t yin_buf_info = {0};
	struct  vos_mem_cma_info_t yout_buf_info = {0};
    VOS_MEM_CMA_HDL yin_vos_mem_id;
	VOS_MEM_CMA_HDL yout_vos_mem_id;
	UINT32 tmp_val0,tmp_val1;
	int ret;
#endif
	
	UINT8 *yin_mem_base;
	UINT8 *yout_mem_base;

#if defined(__FREERTOS)
#else
	UINT32 yin_mem_size;
	UINT32 yout_mem_size;
#endif

	int fd;
	
	int len = 0;
	UINT32 in_w=0, in_h=0;
	char tmp[56];

	UINT64 start_cnt, end_cnt;
	UINT32 timeout;

	KDRV_IVE_OPENCFG ive_open_obj;
	KDRV_IVE_IN_IMG_INFO in_img_info = {0};
	KDRV_IVE_GENERAL_FILTER_PARAM gen_filt = {0};
	KDRV_IVE_MEDIAN_FILTER_PARAM med_filt = {0};
	KDRV_IVE_IMG_IN_DMA_INFO in_addr = {0};
	KDRV_IVE_IMG_OUT_DMA_INFO out_addr = {0};
	KDRV_IVE_EDGE_FILTER_PARAM edge_filt = {0};
	KDRV_IVE_NON_MAX_SUP_PARAM nonmax_filt = {0};
	KDRV_IVE_THRES_LUT_PARAM thr_filt = {0};
	KDRV_IVE_MORPH_FILTER_PARAM morph_filt = {0};
    KDRV_IVE_INTEGRAL_IMG_PARAM integral_img = {0};
	KDRV_IVE_TRIGGER_PARAM trig_param = {0};
	KDRV_IVE_OUTSEL_PARAM outdata_sel = {0};
	KDRV_IVE_IRV_PARAM	irv_param = {0};

	INT32 id = 0; //KDRV_DEV_ID(chip, engine, channel);
    UINT32 outlfs =0 ,inlfs=0;

#if defined(__FREERTOS)
#define IVE_MAX_STR_LEN 256
	UINT32 cStrLen = IVE_MAX_STR_LEN;
	CHAR cStartStr[IVE_MAX_STR_LEN], cEndStr[IVE_MAX_STR_LEN];//, cRegStr[StringMaxLen];
	INT32 iPatSNum = 0, iPatENum = 0;
#endif

    //set run pattern
    int start_index=0 ;
    int end_index=0;
    int i,pc ;
    char cRegFilePath [50],cInputFilePath [50];
    UINT32 in_buf_size_y = 0,out_buf_size_y;
#if defined(__FREERTOS)
	UINT32 buf_idx;
	UINT32 buf_start_addr;
	UINT32 buf_start_size;
#endif
	UINT32 is_retry=0;
	KDRV_IVE_TRIGGER_HOOK_PARAM kdrv_ive_hook_parm;

#if defined(__FREERTOS)
	srand((unsigned) hwclock_get_counter());
#else
	DBG_ERR("IVE:Error rand TODO..\r\n");
#endif

    DBG_IND("start kdrv ....\r\n");
	DBG_EMU("start IVE kdrv ....\r\n");

#if defined(__FREERTOS)
	if (g_bringup_en >= 1) {
		iPatSNum = 0;
		iPatENum = 0;
		start_index = (unsigned int) iPatSNum;
		end_index = (unsigned int) iPatENum;
	} else {
		nvt_dbg(ERR, "******* CNN / NUE / IVE Verification*******\r\n");
		nvt_dbg(ERR, "Please enter the start No.: ");

		uart_getString(cStartStr, &cStrLen);
		iPatSNum = atoi(cStartStr);
		nvt_dbg(ERR, "\r\n");

		nvt_dbg(ERR, "Please enter the end No.: ");
		cStrLen = IVE_MAX_STR_LEN;
		uart_getString(cEndStr, &cStrLen);
		iPatENum = atoi(cEndStr);
		nvt_dbg(ERR, "\r\n");

		start_index = (unsigned int) iPatSNum;
		end_index = (unsigned int) iPatENum;
	}
#else

	DBG_EMU("input data %s \n", pargv[0]);
	if(kstrtouint(pargv[0], 10, &start_index)){
		DBG_ERR("invalid pat_start value:%s\n", pargv[0]);
		return -EINVAL;
	};
	if(kstrtouint(pargv[1], 10, &end_index)){
		DBG_ERR("invalid pat_start value:%s\n", pargv[0]);
		return -EINVAL;
	}

#endif

	DBG_EMU("start_index = %d \r\n",start_index);
    DBG_EMU("end_index = %d \r\n",end_index);

#if 0
    ret = nvt_fmem_mem_info_init(&yin_buf_info, NVT_FMEM_ALLOC_CACHE, IVE_MEM_SIZE, NULL);
    if (ret >= 0) {
        hdl_in_buf_y = nvtmem_alloc_buffer(&yin_buf_info);

    }

	//get Y out buffer//
    ret = nvt_fmem_mem_info_init(&yout_buf_info, NVT_FMEM_ALLOC_CACHE, IVE_MEM_SIZE, NULL);
    if (ret >= 0) {
        hdl_out_buf_y = nvtmem_alloc_buffer(&yout_buf_info);

    }

#endif

#if defined(__FREERTOS)
	ive_set_base_addr((UINT32)0xf0d70000);
#endif

#if defined(__FREERTOS)
    //#define POOL_ID_APP_ARBIT       0
    //#define POOL_ID_APP_ARBIT2      1

	if (g_ive_dram_mode == 2) {
		mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
		mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
	} else {
		mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT);
		mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT);
	}

	g_ive_mem_base_2 = OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
	g_ive_mem_size_2 = OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
	g_ive_mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT);
	g_ive_mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT);

	DBG_EMU("IVE: dram1(0x%x)size(0x%x) dram2(0x%x)size(0x%x)\r\n", 
					ive_platform_va2pa(g_ive_mem_base), g_ive_mem_size,
					ive_platform_va2pa(g_ive_mem_base_2), g_ive_mem_size_2);

    if (mem_size < (IVE_MEM_SIZE + (IVE_MEM_SIZE_1*2) + IVE_LL_BUF_SIZE)) {
        DBG_ERR("Error, the memory size is not enough for IVE validation.(0x%x vs 0x%x)\r\n",
                        mem_size, (IVE_MEM_SIZE + (IVE_MEM_SIZE_1*2) + IVE_LL_BUF_SIZE));
        return -1;
    }

    DBG_EMU("WW##, mem_base=0x%x\r\n", mem_base);

    mem_base = dma_getCacheAddr(mem_base);

    DBG_EMU("WW##, mem_base=0x%x\r\n", mem_base);
    DBG_EMU("WW##, mem_size=0x%x\r\n", mem_size);

	yin_mem_base = (UINT8 *)mem_base;
	//yin_mem_size = IVE_MEM_SIZE;
	yout_mem_base = (UINT8 *) (mem_base + IVE_MEM_SIZE);
	//yout_mem_size = IVE_MEM_SIZE_1;
	
#else
	DBG_EMU("WW##,cachable address\r\n");
	if (0 != vos_mem_init_cma_info(&yin_buf_info, VOS_MEM_CMA_TYPE_CACHE, IVE_MEM_SIZE)) { //VOS_MEM_CMA_TYPE_NONCACHE, VOS_MEM_CMA_TYPE_CACHE
		DBG_ERR("vos_mem_init_cma_info: init buffer fail. \r\n");
		return -1;
	} else {
		yin_vos_mem_id = vos_mem_alloc_from_cma(&yin_buf_info);
		if (NULL == yin_vos_mem_id) {
			DBG_ERR("get buffer fail\n");
			return -1;
		}
	}

	DBG_EMU("WW##,cachable address\r\n");
	if (0 != vos_mem_init_cma_info(&yout_buf_info, VOS_MEM_CMA_TYPE_CACHE, (IVE_MEM_SIZE_1*2))) { //VOS_MEM_CMA_TYPE_NONCACHE, VOS_MEM_CMA_TYPE_CACHE
		DBG_ERR("vos_mem_init_cma_info: init buffer fail. \r\n");
		return -1;
	} else {
		yout_vos_mem_id = vos_mem_alloc_from_cma(&yout_buf_info);
		if (NULL == yout_vos_mem_id) {
			DBG_ERR("get buffer fail\n");
			return -1;
		}
	}

	yin_mem_base = (UINT8 *) yin_buf_info.vaddr;
	yin_mem_size = yin_buf_info.size;
	yout_mem_base = (UINT8 *) yout_buf_info.vaddr;
	yout_mem_size = yout_buf_info.size;

	if ((yin_mem_size < IVE_MEM_SIZE) || (yout_mem_size < IVE_MEM_SIZE_1*2)) {
		DBG_ERR("vos_mem_init_cma_info: size is not enough yin(0x%x) yout(0x%x) allocated(0x%x, 0x%x) \r\n", 
				yin_mem_size, yout_mem_size, IVE_MEM_SIZE, IVE_MEM_SIZE_1);
        return -1;
	}
	
#endif

    //memset((void*)yin_mem_base, 0x0, (UINT32)yin_mem_size);
    //memset((void*)yout_mem_base, 0x0, (UINT32)yout_mem_size);

	if (g_bringup_en >= 1) {
		start_index = 0;
		if (g_bringup_en == 2) {
			end_index = (sizeof(ive_bringup_table_all)/sizeof(UINT32));
		} else {
			end_index = (sizeof(ive_bringup_table)/sizeof(UINT32));
		}
	}

	for(pc = start_index; pc<=end_index; pc++){

		is_retry = 0;

//RETRY_AGAIN_1:


		if (g_no_stop == 1) {
            if (pc == end_index) {
                pc = start_index;
            }
        }

		if (g_bringup_en >= 1) {
			if (g_bringup_en == 2) {
				if ((UINT32) pc >= (sizeof(ive_bringup_table_all)/sizeof(UINT32)) ) {
					break;
				}
			} else {
				if ((UINT32) pc >= (sizeof(ive_bringup_table)/sizeof(UINT32)) ) {
					break;
				}
			}
		}

	   	if ((pc >= 6201) && (pc < 10001)) {
			pc = 10001;
	   	}

	  	DBG_EMU("gen_filt enable = 0x%08x\r\n", (UINT32)gen_filt.enable);

       	sprintf(cInputFilePath, "/mnt/sd/IVEP/1_All/02_di/ive%05d.bin",  ive_get_index(pc));
	   	DBG_EMU("img file in: %s\r\n", cInputFilePath);


       	//set reg to address
       	sprintf(cRegFilePath, "/mnt/sd/IVEP/1_All/01_ri/ive%05d.dat",  ive_get_index(pc));
	   	DBG_EMU("reg file in: %s\r\n", cRegFilePath);

       	fd = vos_file_open(cRegFilePath, O_RDONLY , 0);
       	if ((VOS_FILE)(-1) == fd) {
	  	  	DBG_ERR("failed in file open:%s\n", cRegFilePath);
		  	len = -1;
	  	  	goto EOP;
	   	}
	   	DBG_EMU("start vos_file_open ....\r\n");
	
	          //// parsing registers
       for (i=0;i<(IVE_REG_NUMS/4 + 1 );i++){

			memset(tmp, 0, 14*sizeof(char));

            
#if defined(__FREERTOS)
			size = 4*sizeof(char);
			len = vos_file_read(fd, (void *)tmp, size);
			if (len == 0) {
				DBG_ERR("%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
				//return -1;
			}

			size = 8*sizeof(char);
			len = vos_file_read(fd, (void *)tmp, size);
			if (len == 0) {
				DBG_ERR("%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
				//return -1;
			}
			tmp_long = strtoul(tmp, NULL, 16);
			tmp_val1 = (unsigned int) tmp_long;
			size = 2*sizeof(char);
			len = vos_file_read(fd, (void *)tmp, size);
			if (len == 0) {
				DBG_ERR("%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
				//return -1;
			}
			DBG_EMU("read reg file:ofs=%04X val=%08X\n", (i*4), tmp_val1);
#else
			
            len = vos_file_read(fd, (void*)tmp, 4*sizeof(char));
            if (len == 0){
				DBG_ERR("read register fail ...\r\n");
                break;
			}

            kstrtouint(tmp, 16, &tmp_val0);
            vos_file_read(fd, (void*)tmp, 8*sizeof(char));

            kstrtouint(tmp, 16, &tmp_val1);
            vos_file_read(fd, (void*)tmp, 2*sizeof(char));
			DBG_EMU("read reg file:ofs=%04X val=%08X\n", tmp_val0, tmp_val1);
#endif

            g_IVE_ENGINE_HOUSE[i] = tmp_val1;
       }
       vos_file_close(fd);


	   //if (g_pIveReg_drv->IVE_Register_0010.bit.IRV_EN == 0) {
	   //    continue;
	   //}

	   //===========  //get Y in buffer =
	   in_w = g_pIveReg_drv->IVE_Register_0024.bit.IMG_WIDTH;
       in_h = g_pIveReg_drv->IVE_Register_0024.bit.IMG_HEIGHT;
	   in_buf_size_y = (((in_w * in_h) + 3) / 4) * 4;

	   DBG_EMU("in_w = 0x%08x\r\n", (UINT32)in_w);
	   DBG_EMU("in_h = 0x%08x\r\n", (UINT32)in_h);
	   DBG_EMU("in_buf_size_y = 0x%08x\r\n", (UINT32)in_buf_size_y);



       DBG_EMU("yin_mem_base = 0x%08x\r\n", (UINT32)yin_mem_base);

       out_buf_size_y = ((g_pIveReg_drv->IVE_Register_0020.bit.DRAM_OUT_LOFST) << 2) * in_h;
#if defined(__FREERTOS)
	   g_ive_outbuf_size = out_buf_size_y;
#endif
       DBG_EMU("g_pIveReg_drv->IVE_Register_0020.bit.DRAM_OUT_LOFST) << 2 = %d\r\n", (UINT32)(g_pIveReg_drv->IVE_Register_0020.bit.DRAM_OUT_LOFST ));
       DBG_EMU("out_buf_size_y= %d\r\n", (UINT32)out_buf_size_y);

       memset((void*)yin_mem_base, 0x0, (UINT32)in_buf_size_y*2);
	   DBG_EMU("yin_mem_size = 0x%x\r\n", (UINT32) in_buf_size_y*2);
       memset((void*)yout_mem_base, 0x0, (UINT32)out_buf_size_y*2);
	   DBG_EMU("yout_mem_size = 0x%x\r\n", (UINT32)out_buf_size_y*2);

		g_gld_addr = (UINT32) IVE_4_BYTE_ALIGN_CEILING((UINT32)yout_mem_base + out_buf_size_y + 8);
		g_gld_size = out_buf_size_y;
#if defined(__FREERTOS) //assign LL buffer / LL output buffer

		if (mem_size < (IVE_MEM_SIZE + (IVE_MEM_SIZE_1*2) + IVE_LL_BUF_SIZE + (out_buf_size_y*2*IVE_MAX_RESERVE_BUF_NUM))) {
			nvt_dbg(ERR, "[IVE_LL]Error, the memory size is not enough for IVE validation.(total0x%x, now:0x%x) by pass ive_get_index(pc)(%x)\r\n",
							mem_size,  (IVE_MEM_SIZE + (IVE_MEM_SIZE_1*2) + IVE_LL_BUF_SIZE + (out_buf_size_y*2*IVE_MAX_RESERVE_BUF_NUM)), ive_get_index(pc));
			if (g_ll_mode_backup == 0) {
				g_ll_mode_backup = g_ll_mode;
				g_ll_mode = 0;
			}
			
		} else {
			if (g_ll_mode_backup != 0) {
				g_ll_mode = g_ll_mode_backup;
				g_ll_mode_backup = 0;
			}

			if (g_ll_mode == 1) {
				buf_start_addr = IVE_4_BYTE_ALIGN_CEILING(g_gld_addr + out_buf_size_y + 8);
				buf_start_size = out_buf_size_y;
				for (buf_idx = 0; buf_idx < IVE_MAX_RESERVE_BUF_NUM; buf_idx++) {
					if (g_is_switch_dram == 0) {
						ive_dram_addr_res[buf_idx] = buf_start_addr;
						ive_dram_size_res[buf_idx] = buf_start_size;
						if (g_dram_outrange == 1 && buf_idx == 0 && g_ive_dram_mode == 1) {
							ive_dram_addr_res[buf_idx] = 0x30000000  - out_buf_size_y + 100;
							ive_dram_size_res[buf_idx] = out_buf_size_y;
						} else if (g_dram_outrange == 1 && buf_idx == 0 && g_ive_dram_mode == 2) {
							ive_dram_addr_res[buf_idx] = 0x80000000;
							ive_dram_size_res[buf_idx] = out_buf_size_y;
						} else if (g_dram_outrange == 2 && buf_idx == 0) {
							if (g_ive_dram_mode == 2) {
								ive_dram_addr_res[buf_idx] = 0x80000000 - out_buf_size_y;
							} else if (g_ive_dram_mode == 1) {
								ive_dram_addr_res[buf_idx] = 0x30000000 - out_buf_size_y;
							}
						}
					} else {
						if ((buf_idx % 2) == 0) {
							ive_dram_addr_res[buf_idx] = IVE_SWITCH_DRAM_1(buf_start_addr);
						} else {
							ive_dram_addr_res[buf_idx] = IVE_SWITCH_DRAM_2(buf_start_addr);
						}
					}
					nvt_dbg(ERR, "IVE_RES_BUF: idx(%d) addr(0x%x)\r\n", buf_idx, ive_dram_addr_res[buf_idx]);
					buf_start_addr = IVE_4_BYTE_ALIGN_CEILING(buf_start_addr + out_buf_size_y + 8);
					buf_start_size = out_buf_size_y;
				}
				g_ll_buf = (UINT64 *) IVE_8_BYTE_ALIGN_CEILING(ive_dram_addr_res[buf_idx - 1] + ive_dram_size_res[buf_idx - 1] + 8);
			}
		}
#endif
		ive_load_golden_data(ive_get_index(pc), g_gld_addr, g_gld_size);


	   // load image
	   fd = vos_file_open(cInputFilePath, O_RDONLY , 0);
	   if ((VOS_FILE)(-1) == fd) {
	  	  DBG_ERR("failed in file open:%s\n", cInputFilePath);
		  len = -1;
	  	  goto EOP;
	   }

	   len = vos_file_read(fd, yin_mem_base, in_buf_size_y*2);//va_addr
	   DBG_EMU("Read %d bytes\r\n", len);
	   vos_file_close(fd);


       //========== ive d2d flow ========/
        DBG_IND("ive d2d flow start\n");
        //open//
        ive_open_obj.ive_clock_sel = 480;
        kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_OPENCFG, (void *)&ive_open_obj);
        if(kdrv_ive_open(0, 0) != 0) {
            DBG_ERR("set opencfg fail!\r\n");
            goto EOP;
        }
        DBG_IND("ive opened\r\n");

        //set input img parameters/
        in_img_info.width= g_pIveReg_drv->IVE_Register_0024.bit.IMG_WIDTH;   //in_w;
        in_img_info.height = g_pIveReg_drv->IVE_Register_0024.bit.IMG_HEIGHT; //in_h;
        in_img_info.lineofst = g_pIveReg_drv->IVE_Register_0018.bit.DRAM_IN_LOFST <<2;
        kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IN_IMG, (void *)&in_img_info);
        DBG_IND("KDRV_IVE_PARAM_IPL_IN_IMG\r\n");

        inlfs = g_pIveReg_drv->IVE_Register_0018.bit.DRAM_IN_LOFST ;
        outlfs = g_pIveReg_drv->IVE_Register_0020.bit.DRAM_OUT_LOFST ;

        DBG_EMU("in_img_info.lineofst : %d \r\n", in_img_info.lineofst);
        DBG_EMU("inlfs : %d \r\n", inlfs);
        DBG_EMU("outlfs : %d \r\n", outlfs);

		if (inlfs == 0) inlfs = 0;
		if (outlfs == 0) outlfs = 0;

        //set input address/
        in_addr.addr = (UINT32)yin_mem_base;
#if defined(__FREERTOS)
		g_ive_in_addr = in_addr.addr;
		g_ive_in_size = in_buf_size_y;
#endif
        kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IMG_DMA_IN, (void *)&in_addr);
        DBG_IND("KDRV_IVE_PARAM_IPL_IMG_DMA_IN\r\n");

        //set output address/
        out_addr.addr = (UINT32)yout_mem_base;
#if defined(__FREERTOS)
		g_ive_out_addr = out_addr.addr;
		g_ive_out_size = out_buf_size_y;
#endif
        kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IMG_DMA_OUT, (void *)&out_addr);
        DBG_IND("KDRV_IVE_PARAM_IPL_IMG_DMA_OUT\r\n");

        //set general filter/
        gen_filt.enable = g_pIveReg_drv->IVE_Register_0010.bit.GEN_FILT_EN;
        gen_filt.filt_coeff[0] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF0;
        gen_filt.filt_coeff[1] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF1;
        gen_filt.filt_coeff[2] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF2;
        gen_filt.filt_coeff[3] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF3;
        gen_filt.filt_coeff[4] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF4;
        gen_filt.filt_coeff[5] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF5;
        gen_filt.filt_coeff[6] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF6;
        gen_filt.filt_coeff[7] = g_pIveReg_drv->IVE_Register_0028.bit.GEN_FILT_COEFF7;
        gen_filt.filt_coeff[8] = g_pIveReg_drv->IVE_Register_002c.bit.GEN_FILT_COEFF8;
        gen_filt.filt_coeff[9] = g_pIveReg_drv->IVE_Register_002c.bit.GEN_FILT_COEFF9;



        kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_GENERAL_FILTER, (void *)&gen_filt);
        DBG_IND("KDRV_IVE_PARAM_IQ_GENERAL_FILTER\r\n");
        DBG_EMU("gen_filt.filt_coeff[0] = 0x%08x\r\n", (UINT32)gen_filt.filt_coeff[0]);
        DBG_EMU("gen_filt enable = 0x%08x\r\n", (UINT32)gen_filt.enable);


        //set med_filt filter
        med_filt.enable = g_pIveReg_drv->IVE_Register_0010.bit.MEDN_FILT_EN;
        med_filt.mode = g_pIveReg_drv->IVE_Register_0010.bit.MEDN_FILT_MODE;
		med_filt.medn_inval_th = g_pIveReg_drv_528->IVE_Register_0050.bit.MEDN_INVAL_TH;

		kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_MEDIAN_FILTER, (void *)&med_filt);

        //set edge filter
        edge_filt.enable = g_pIveReg_drv->IVE_Register_0010.bit.EDGE_FILT_EN;
        edge_filt.mode   = g_pIveReg_drv->IVE_Register_0010.bit.EDGE_MODE;
        edge_filt.edge_coeff1[0] = g_pIveReg_drv->IVE_Register_0030.bit.EDGE_FILT1_COEFF0;
        edge_filt.edge_coeff1[1] = g_pIveReg_drv->IVE_Register_0030.bit.EDGE_FILT1_COEFF1;
        edge_filt.edge_coeff1[2] = g_pIveReg_drv->IVE_Register_0030.bit.EDGE_FILT1_COEFF2;
        edge_filt.edge_coeff1[3] = g_pIveReg_drv->IVE_Register_0030.bit.EDGE_FILT1_COEFF3;
        edge_filt.edge_coeff1[4] = g_pIveReg_drv->IVE_Register_0030.bit.EDGE_FILT1_COEFF4;
        edge_filt.edge_coeff1[5] = g_pIveReg_drv->IVE_Register_0030.bit.EDGE_FILT1_COEFF5;
        edge_filt.edge_coeff1[6] = g_pIveReg_drv->IVE_Register_0034.bit.EDGE_FILT1_COEFF6;
        edge_filt.edge_coeff1[7] = g_pIveReg_drv->IVE_Register_0034.bit.EDGE_FILT1_COEFF7;
        edge_filt.edge_coeff1[8] = g_pIveReg_drv->IVE_Register_0034.bit.EDGE_FILT1_COEFF8;

        edge_filt.edge_coeff2[0] = g_pIveReg_drv->IVE_Register_0034.bit.EDGE_FILT2_COEFF0;
        edge_filt.edge_coeff2[1] = g_pIveReg_drv->IVE_Register_0034.bit.EDGE_FILT2_COEFF1;
        edge_filt.edge_coeff2[2] = g_pIveReg_drv->IVE_Register_0034.bit.EDGE_FILT2_COEFF2;
        edge_filt.edge_coeff2[3] = g_pIveReg_drv->IVE_Register_0038.bit.EDGE_FILT2_COEFF3;
        edge_filt.edge_coeff2[4] = g_pIveReg_drv->IVE_Register_0038.bit.EDGE_FILT2_COEFF4;
        edge_filt.edge_coeff2[5] = g_pIveReg_drv->IVE_Register_0038.bit.EDGE_FILT2_COEFF5;
        edge_filt.edge_coeff2[6] = g_pIveReg_drv->IVE_Register_0038.bit.EDGE_FILT2_COEFF6;
        edge_filt.edge_coeff2[7] = g_pIveReg_drv->IVE_Register_0038.bit.EDGE_FILT2_COEFF7;
        edge_filt.edge_coeff2[8] = g_pIveReg_drv->IVE_Register_0038.bit.EDGE_FILT2_COEFF8;

        edge_filt.edge_shift_bit  =  g_pIveReg_drv->IVE_Register_0010.bit.EDGE_SHIFT_BIT;
        edge_filt.AngSlpFact      =  g_pIveReg_drv->IVE_Register_0010.bit.ANGLE_SLP_FACT;
        kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_EDGE_FILTER, (void *)&edge_filt);

		if(nvt_get_chip_id() == CHIP_NA51055) {
		} else {
			irv_param.en = g_pIveReg_drv->IVE_Register_0010.bit.IRV_EN;
			irv_param.hist_mode_sel = g_pIveReg_drv->IVE_Register_0010.bit.IRV_HIST_MODE_SEL;
			irv_param.invalid_val = g_pIveReg_drv->IVE_Register_0010.bit.IRV_INVALID_VAL;

			irv_param.thr_s = g_pIveReg_drv_528->IVE_Register_0050.bit.IRV_THR_S;
			irv_param.thr_h = g_pIveReg_drv_528->IVE_Register_0050.bit.IRV_THR_H;
			
			kdrv_ive_set(id, KDRV_IVE_PARAM_IRV, (void *)&irv_param);
		}

        //non-max sup
        nonmax_filt.enable = g_pIveReg_drv->IVE_Register_0010.bit.NON_MAX_SUP_EN;
        nonmax_filt.mag_thres = g_pIveReg_drv->IVE_Register_0048.bit.EDGE_MAG_TH;
        kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_NON_MAX_SUP, (void *)&nonmax_filt);


	    //thr_filt
        thr_filt.enable  = g_pIveReg_drv->IVE_Register_0010.bit.THRES_LUT_EN;
        thr_filt.thres_lut[0] = g_pIveReg_drv->IVE_Register_003c.bit.EDGE_THRES_LUT0;
        thr_filt.thres_lut[1] = g_pIveReg_drv->IVE_Register_003c.bit.EDGE_THRES_LUT1;
        thr_filt.thres_lut[2] = g_pIveReg_drv->IVE_Register_003c.bit.EDGE_THRES_LUT2;
        thr_filt.thres_lut[3] = g_pIveReg_drv->IVE_Register_003c.bit.EDGE_THRES_LUT3;
        thr_filt.thres_lut[4] = g_pIveReg_drv->IVE_Register_0040.bit.EDGE_THRES_LUT4;
        thr_filt.thres_lut[5] = g_pIveReg_drv->IVE_Register_0040.bit.EDGE_THRES_LUT5;
        thr_filt.thres_lut[6] = g_pIveReg_drv->IVE_Register_0040.bit.EDGE_THRES_LUT6;
        thr_filt.thres_lut[7] = g_pIveReg_drv->IVE_Register_0040.bit.EDGE_THRES_LUT7;
        thr_filt.thres_lut[8] = g_pIveReg_drv->IVE_Register_0044.bit.EDGE_THRES_LUT8;
        thr_filt.thres_lut[9] = g_pIveReg_drv->IVE_Register_0044.bit.EDGE_THRES_LUT9;
        thr_filt.thres_lut[10] = g_pIveReg_drv->IVE_Register_0044.bit.EDGE_THRES_LUT10;
        thr_filt.thres_lut[11] = g_pIveReg_drv->IVE_Register_0044.bit.EDGE_THRES_LUT11;
        thr_filt.thres_lut[12] = g_pIveReg_drv->IVE_Register_0048.bit.EDGE_THRES_LUT12;
        thr_filt.thres_lut[13] = g_pIveReg_drv->IVE_Register_0048.bit.EDGE_THRES_LUT13;
        thr_filt.thres_lut[14] = g_pIveReg_drv->IVE_Register_0048.bit.EDGE_THRES_LUT14;


        kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_THRES_LUT, (void *)&thr_filt);


        //morph_filt
        morph_filt.enable = g_pIveReg_drv->IVE_Register_0010.bit.MORPH_FILT_EN;
        morph_filt.in_sel = g_pIveReg_drv->IVE_Register_0010.bit.MORPH_IN_SEL;
        morph_filt.operation =  g_pIveReg_drv->IVE_Register_0010.bit.MORPH_OP;

        morph_filt.neighbor[0 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH0_EN;
        morph_filt.neighbor[1 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH1_EN;
        morph_filt.neighbor[2 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH2_EN;
        morph_filt.neighbor[3 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH3_EN;
        morph_filt.neighbor[4 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH4_EN;
        morph_filt.neighbor[5 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH5_EN;
        morph_filt.neighbor[6 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH6_EN;
        morph_filt.neighbor[7 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH7_EN;
        morph_filt.neighbor[8 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH8_EN;
        morph_filt.neighbor[9 ] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH9_EN;
        morph_filt.neighbor[10] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH10_EN;
        morph_filt.neighbor[11] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH11_EN;
        morph_filt.neighbor[12] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH12_EN;
        morph_filt.neighbor[13] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH13_EN;
        morph_filt.neighbor[14] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH14_EN;
        morph_filt.neighbor[15] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH15_EN;
        morph_filt.neighbor[16] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH16_EN;
        morph_filt.neighbor[17] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH17_EN;
        morph_filt.neighbor[18] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH18_EN;
        morph_filt.neighbor[19] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH19_EN;
        morph_filt.neighbor[20] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH20_EN;
        morph_filt.neighbor[21] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH21_EN;
        morph_filt.neighbor[22] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH22_EN;
        morph_filt.neighbor[23] = g_pIveReg_drv->IVE_Register_004c.bit.NEIGH23_EN;



        kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_MORPH_FILTER, (void *)&morph_filt);

		//integral image
        integral_img.enable = g_pIveReg_drv->IVE_Register_0010.bit.INTEGRAL_EN;
		kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_INTEGRAL_IMG, (void *)&integral_img);

		//out selection
		outdata_sel.OutDataSel = g_pIveReg_drv->IVE_Register_0010.bit.OUT_DATA_SEL;
		outdata_sel.Outlofs = g_pIveReg_drv->IVE_Register_0020.bit.DRAM_OUT_LOFST<<2 ;
		DBG_EMU("outdata_sel.OutDataSel = %d \r\n", (UINT32)outdata_sel.OutDataSel);
		DBG_EMU("outdata_sel.Outlofs = %d \r\n", (UINT32)outdata_sel.Outlofs);

		kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_OUTSEL, (void *)&outdata_sel);

		//set call back funcio//
		kdrv_ive_fmd_flag = 0;
		kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_ISR_CB, (void *)NULL); //kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_ISR_CB, (void *)&KDRV_IVE_ISR_CB);
		DBG_IND("KDRV_IVE_PARAM_IPL_ISR_CB\r\n");

RETRY_AGAIN:
		ive_hook(&trig_param);
		if (g_reg_rw == 1) {
			ive_check_reg_rw();
			goto IVE_END;
		}

		//trig ive start//
		DBG_IND("kdrv_ive_trigger\r\n");
		start_cnt = hwclock_get_longcounter();
		trig_param.time_out_ms = 0;
#if defined(__FREERTOS)
		if (g_interrupt_en == 0) {
			ive_platform_interrupt_en(0);
			trig_param.wait_end = FALSE;
		} else {
			ive_platform_interrupt_en(1);
			trig_param.wait_end = TRUE;
		}
#else
		trig_param.wait_end = TRUE;
#endif

#if defined(__FREERTOS)
		if (is_retry == 0) {
nvt_dbg(ERR, "IVE: is_retry == 0\r\n");
			if ((g_ll_terminate == 1) && (g_ll_mode == 1)) {
				kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_TERMINATE;
				kdrv_ive_hook_parm.hook_value = 1;
				if (g_interrupt_en == 0) {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				} else {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				}
				kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, kdrv_ive_hook_func, kdrv_ive_hook_parm);
			} else if (g_hw_rst_en > 0) {
#if 1
				kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_RST;
				if (g_hw_rst_en == 3) {
					kdrv_ive_hook_parm.hook_value = 1 + (rand()%2);
				} else {
                	kdrv_ive_hook_parm.hook_value = g_hw_rst_en;
				}
				if (g_interrupt_en == 0) {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				} else {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				}
                kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, kdrv_ive_hook_func, kdrv_ive_hook_parm);
#else
				kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_NO_FUNC;
                kdrv_ive_hook_parm.hook_value = 0;
                if (g_interrupt_en == 0) {
                    kdrv_ive_hook_parm.hook_is_direct_return = 1;
                } else {
                    kdrv_ive_hook_parm.hook_is_direct_return = 0;
                }
                kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, kdrv_ive_null_func, kdrv_ive_hook_parm);
#endif
			} else if (g_clk_en > 0) {
				kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_CLK_EN;
                kdrv_ive_hook_parm.hook_value = g_clk_en;
				if (g_interrupt_en == 0) {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				} else {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				}
                kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, kdrv_ive_hook_func, kdrv_ive_hook_parm);
			} else {
				kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_NO_FUNC;
				kdrv_ive_hook_parm.hook_value = 0;
				if (g_interrupt_en == 0) {
					kdrv_ive_hook_parm.hook_is_direct_return = 1;
				} else {
					kdrv_ive_hook_parm.hook_is_direct_return = 0;
				}
				kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, kdrv_ive_null_func, kdrv_ive_hook_parm);
			}
		} else {
nvt_dbg(ERR, "IVE: is_retry == 1\r\n");
			kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_NO_FUNC;
			kdrv_ive_hook_parm.hook_value = 0;
			if (g_interrupt_en == 0) {
				kdrv_ive_hook_parm.hook_is_direct_return = 1;
			} else {
				kdrv_ive_hook_parm.hook_is_direct_return = 0;
			}
			kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, kdrv_ive_null_func, kdrv_ive_hook_parm);
		}

#else
		kdrv_ive_hook_parm.hook_mode = (UINT32) IVE_NO_FUNC;
		kdrv_ive_trigger_with_hook(id , &trig_param, NULL, NULL, 0, kdrv_ive_hook_parm);
#endif

		ive_hook1();

		end_cnt = hwclock_get_longcounter();
		DBG_IND("trigger end!\r\n");
#if defined(__FREERTOS)
		DBG_IND("trigger time: %llu us\r\n", end_cnt - start_cnt);
		if (end_cnt == 0) end_cnt = 0; //set but not used
		if (start_cnt == 0) start_cnt = 0; //set but not used
#else
		DBG_DUMP("trigger time: %llu us\r\n", end_cnt - start_cnt);
#endif

#if 0
		//wait end//
		if (!trig_param.wait_end) {
			while(!kdrv_ive_fmd_flag) {
				DBG_IND("wait fmd flag\n");
#if defined(__FREERTOS)
				vos_task_delay_ms(10);
#else
				msleep(10);
#endif
			}
		}
#endif
		if (g_interrupt_en == 0) {
			timeout = 10;
			if (trig_param.is_nonblock == 1) { //LL mode
				while(ive_getLLFrmEndRdyReg() == 0) {
					vos_task_delay_ms(200);
					timeout--;
					if ((timeout % 2) == 0) {
						DBG_ERR("IVE: polling at timeout=%d\r\n", timeout);
					}
					if (timeout == 0) {
						break;
					}
				}
			} else {
				while(ive_getFrmEndRdyReg() == 0) {
					vos_task_delay_ms(200);
					timeout--;
					if ((timeout % 2) == 0) {
						DBG_ERR("IVE: polling at timeout=%d\r\n", timeout);
					}
					if (timeout == 0) {
						break;
					}
				}
			}
		}

		ive_hook2(ive_get_index(pc));
		
#if defined(__FREERTOS)
		ive_platform_dma_flush(g_ive_out_addr, g_ive_out_size);
#endif

		if (g_is_gld_cmp == 0) {
			// save image //
			DBG_EMU("out_buf_size_y = %d\r\n", (UINT32)out_buf_size_y);
			DBG_EMU("yout_mem_base = 0x%08x\r\n", (UINT32)yout_mem_base);
			sprintf(cRegFilePath, "/mnt/sd/IVEP/1_All/04_DO_0/ive%05d.bin",  ive_get_index(pc));
			DBG_EMU("reg file in: %s addr=0x%x size=0x%x\r\n", cRegFilePath, (UINT32)yout_mem_base, (UINT32)out_buf_size_y);
			fd = vos_file_open(cRegFilePath, O_CREAT|O_WRONLY|O_SYNC|O_TRUNC , 0);
			if ((VOS_FILE)(-1)==fd) {
				DBG_ERR("failed in file open:%s\n", cRegFilePath);
				len = -1;
				goto EOP;
			}
			
			len = vos_file_write(fd, yout_mem_base, out_buf_size_y);
			vos_file_close(fd);
		} else {
			if (ive_cmp_data((UINT32)yout_mem_base, out_buf_size_y, (UINT32) g_gld_addr, ive_get_index(pc)) <= 0) {
				// save image //
				DBG_EMU("out_buf_size_y = %d\r\n", (UINT32)out_buf_size_y);
				DBG_EMU("yout_mem_base = 0x%08x\r\n", (UINT32)yout_mem_base);
				sprintf(cRegFilePath, "/mnt/sd/IVEP/1_All/04_DO_0_err/ive%05d.bin",  ive_get_index(pc));
				DBG_EMU("reg file in: %s addr=0x%x size=0x%x\r\n", cRegFilePath, (UINT32)yout_mem_base, (UINT32)out_buf_size_y);
				fd = vos_file_open(cRegFilePath, O_CREAT|O_WRONLY|O_SYNC|O_TRUNC , 0);
				if ((VOS_FILE)(-1)==fd) {
					DBG_ERR("failed in file open:%s\n", cRegFilePath);
					len = -1;
					goto EOP;
				}
				
				len = vos_file_write(fd, yout_mem_base, out_buf_size_y);
				vos_file_close(fd);
			} else {
				nvt_dbg(ERR, "IVE: golden compared result ok at case:%d\r\n", ive_get_index(pc));
			}
			
		}

		if ((g_ll_terminate > 0) && (is_retry == 0) && (g_ll_mode == 1)) {
			is_retry = 1;
            goto RETRY_AGAIN;
		} else if ((g_hw_rst_en > 0) && (is_retry == 0)) {
			is_retry = 1;
			goto RETRY_AGAIN;
		} else {
			is_retry = 0;
		}
		
IVE_END:

		//close//
		kdrv_ive_close(0, 0);
		DBG_IND("ive d2d flow end\n");
		//======== ive d2d flow ========/
		
#if defined(__FREERTOS)
		nvt_dbg(ERR, "IVE: cyc(0x%x) ll_cyc(0x%x)\r\n", *cycle_cnt, *ll_cycle_cnt);
#endif

#if defined(__FREERTOS)
		if (irv_param.en == 1) {
			DBG_ERR("IRV_DUMP_%05d: %x\r\n", ive_get_index(pc), *mod_cnt);
			DBG_ERR("IRV_DUMP_%05d: %x\r\n", ive_get_index(pc), *nmod_cnt);
		}
#endif

		DBG_EMU("Saved %d bytes\r\n", len);

   }
   EOP:


   //ret = nvtmem_release_buffer(hdl_in_buf_y);
   //ret = nvtmem_release_buffer(hdl_out_buf_y);

#if defined(__FREERTOS)
#else
    ret = vos_mem_release_from_cma(yin_vos_mem_id);
    if (ret != 0) {
        DBG_ERR("failed in release buffer\n");
        return -1;
    }

	ret = vos_mem_release_from_cma(yout_vos_mem_id);
    if (ret != 0) {
        DBG_ERR("failed in release buffer\n");
        return -1;
    }
#endif


   return len;
}

void ive_set_bringup(UINT8 enable)
{
	UINT32 idx;

	g_bringup_en = enable;

	if (g_bringup_en == 2) {
		for (idx = 0; idx < IVE_ALL_CASES; idx++) {
			ive_bringup_table_all[idx] = idx + 1;
		}
	}

	DBG_EMU("bring up NOW(%d)\r\n", g_bringup_en);
	return;
}

#if defined(__FREERTOS)
static VOID ive_set_heavy_func(UINT8 heavy_mode, UINT8 dram_mode)
{
	UINT32 mem_base, mem_size;
	DRAM_CONSUME_ATTR attr;
	UINT32 heavy_load_size = 0x1000000;

	if (dram_mode == 2) {
		mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
		mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
	} else {
		mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT);
		mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT);
	}

	if (heavy_mode == 0) {
		attr.load_degree = DRAM_CONSUME_HEAVY_LOADING;
	} else {
		attr.load_degree = DRAM_CONSUME_CH_DISABLE;
	}

	if (mem_size > (heavy_load_size + 0xA000000)) {
		attr.size = heavy_load_size;
	} else {
		nvt_dbg(ERR, "Error, The memory size is not enough for heavy load.\r\n");
		return;
	}
	attr.addr = (mem_base + mem_size - heavy_load_size) & (~(0xFF));

	if (heavy_mode == 1) {
		memset((void *)&attr.dma_channel, 0x0, sizeof(attr.dma_channel));
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_IVE_0, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_IVE_1, attr.dma_channel);
	}

	if (dram_mode == 2) {
		if (dram2_consume_cfg(&attr) != 0) {
			nvt_dbg(ERR, "Error to do dram2_consume_cfg(&attr)\r\n");
		}
		if (dram2_consume_start() != 0) {
			nvt_dbg(ERR, "Error to do dram2_consume_start()\r\n");
		}
	} else {
		if (dram_consume_cfg(&attr) != 0) {
			nvt_dbg(ERR, "Error to do dram_consume_cfg(&attr)\r\n");
		}
		if (dram_consume_start() != 0) {
			nvt_dbg(ERR, "Error to do dram_consume_start()\r\n");
		}
	}
	nvt_dbg(ERR, "Start (DRAM_CONSUME_HEAVY_LOADING) at dram(%d) (0x%x)\r\n", attr.addr, dram_mode);

	return;

}
#endif


void emu_iveMain_drv(void)
{
	CHAR ch=0;
#if defined(__FREERTOS)
#define IVE_MAX_STR_LEN 256
	UINT32 cStrLen = IVE_MAX_STR_LEN;
	CHAR cEndStr[IVE_MAX_STR_LEN];//, cRegStr[StringMaxLen];
#endif

	DBG_EMU("start IVE test:\r\n");

	

	while(1) {
		nvt_dbg(ERR,"1: test IVE\r\n");
		nvt_dbg(ERR,"2: bring up enable:\r\n");
		nvt_dbg(ERR,"7. ll_test_4 (1: normal 2: error cmd):\r\n");
		nvt_dbg(ERR,"a: auto clk\r\n");
		nvt_dbg(ERR,"d: dram mode\r\n");
		nvt_dbg(ERR,"e: priority mode\r\n");
		nvt_dbg(ERR,"f: clock enable\r\n");
		nvt_dbg(ERR,"g: golden compare:\r\n");
		nvt_dbg(ERR,"h: heavy load\r\n");
		nvt_dbg(ERR,"j: ll register number\r\n");
		nvt_dbg(ERR,"k: ll filled number\r\n");
		nvt_dbg(ERR,"l. Linked-list mode test\r\n");
		nvt_dbg(ERR,"m. sram down\r\n");
		nvt_dbg(ERR,"o: clock\r\n");
		nvt_dbg(ERR,"p: write protect\r\n");
		nvt_dbg(ERR,"q. switch dram:\r\n");
		nvt_dbg(ERR,"r. burst length:\r\n");
		nvt_dbg(ERR,"t. ll terminate:\r\n");
		nvt_dbg(ERR,"u. sw_hw_rst_test\r\n");
		nvt_dbg(ERR,"v: dram out-range\r\n");
		nvt_dbg(ERR,"x: register R/W\r\n");
		nvt_dbg(ERR,"y: heavy mode(0:heavy, 1:channel disable)\r\n");
		nvt_dbg(ERR,"z: exit\r\n");
		nvt_dbg(ERR,"IVE test menu\r\n");

#if defined(__FREERTOS)
        uart_getChar(&ch);
#else
        nvt_dbg(ERR, "Error,TODO_%s_%dr\n", __FILE__, __LINE__);
#endif
        switch (ch) {
        case '1':
#if defined(__FREERTOS)
			nvt_kdrv_ipp_api_ive_test();
#else
			nvt_dbg(ERR, "%s_%d: TODO here.\r\n", __FUNCTION__, __LINE__);
#endif
			break;
		case '2':
			g_bringup_en++;
			if (g_bringup_en > 2) {
				g_bringup_en = 0;
			}
			nvt_dbg(ERR, "bringup:(NOW:%d) \r\n", g_bringup_en);
			break;
		case '7':
#if defined(__FREERTOS)
            g_ive_ll_next_update++;
            if (g_ive_ll_next_update >= 3) {
                g_ive_ll_next_update = 0;
            }
            nvt_dbg(ERR, "ll_test_4:(NOW:%d) \r\n", g_ive_ll_next_update);
#endif
            break;
		case 'a':
#if defined(__FREERTOS)
			g_auto_clk++;
			if (g_auto_clk >= 3) {
				g_auto_clk = 0;
			}
			nvt_dbg(ERR, "auto_clk_gating: NOW(%d)\r\n", g_auto_clk);
#endif
			break;
		case 'd':
#if defined(__FREERTOS)
			g_ive_dram_mode++;
			if (g_ive_dram_mode >=3) {
				g_ive_dram_mode = 1;
			}
			nvt_dbg(ERR, "dram_mode: NOW(%d)\r\n", g_ive_dram_mode);
#endif
			break;
		case 'e':
            //DMA_PRIORITY_LOW,           // Low priority (Default value)
            //DMA_PRIORITY_MIDDLE,        // Middle priority
            //DMA_PRIORITY_HIGH,          // High priority
            //DMA_PRIORITY_SUPER_HIGH,    // Super high priority (Only DMA_CH_SIE_XX or DMA_CH_SIE2_XX are allowed)
#if defined(__FREERTOS)
            g_pri_mode++;
            if (g_pri_mode >= 5) {
                g_pri_mode = 0;
            }
            nvt_dbg(ERR, "priority mode: NOW(%d) 0:LOW 1:MIDD 2: HIGH 3: SUPER 4:rand\r\n", g_pri_mode);
#endif
            break;
		case 'f':
#if defined(__FREERTOS)
			g_clk_en++;
			if (g_clk_en >= 4) {
				g_clk_en = 0;
			}
			nvt_dbg(ERR, "clock_en: NOW(%d) 1:delay 2:fail 3:ok\r\n", g_clk_en);
#endif
			break;
		case 'g':
			if (g_is_gld_cmp == 0) {
				g_is_gld_cmp = 1;
			} else {
				g_is_gld_cmp = 0;
			}
			nvt_dbg(ERR, "golden compared: NOW(%d)\r\n", g_is_gld_cmp);
			break;
		case 'h':
#if defined(__FREERTOS)
			ive_set_heavy_func(g_is_heavy, g_ive_dram_mode);
#endif
			break;
		case 'i':
#if defined(__FREERTOS)
            if (g_interrupt_en == 0) {
                g_interrupt_en = 1;
            } else {
                g_interrupt_en = 0;
            }
            nvt_dbg(ERR, "interrupt enable, NOW(%d)", g_interrupt_en);
#endif
            break;	
		case 'j':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the ll register number: ");
            cStrLen = IVE_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_ll_fill_reg_num = atoi(cEndStr);
            nvt_dbg(ERR, "ll register num (now=%d)\r\n", g_ll_fill_reg_num);
#else
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
            break;
		case 'k':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the ll fill number: ");
            cStrLen = IVE_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_ll_fill_num = atoi(cEndStr);
            nvt_dbg(ERR, "ll_fill_num (now=%d)\r\n", g_ll_fill_num);
#else
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
            break;
		case 'l':
#if defined(__FREERTOS)
			g_ll_mode++;
			if (g_ll_mode >= 3) {
				g_ll_mode = 0;
			}
			g_ll_mode_backup = 0;
			nvt_dbg(ERR, "LL_MODE: NOW(%d) 1:LL_mode, 2:trigger by LL\r\n", g_ll_mode);
#endif
			break;
		case 'm':
#if defined(__FREERTOS)
			g_sram_down++;
			if (g_sram_down >= 3) {
				g_sram_down = 0;
			}
			nvt_dbg(ERR, "sram_down: NOW(%d), 1:ON 2:OFF\r\n", g_sram_down);
#endif
			break;
		case 'n':
			if (g_no_stop == 0) {
				g_no_stop = 1;
			} else {
				g_no_stop = 0;
			}
			nvt_dbg(ERR, "no_stop NOW(%d)\r\n", g_no_stop);
			break;
		case 'o':
#if defined(__FREERTOS)
			g_clock++;
			if (g_clock >= 5) {
				g_clock = 0;
			}
			nvt_dbg(ERR, "g_clock: NOW(%d)", g_clock);
#endif
			break;
		 case 'p':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the wp_en number: ");
            cStrLen = IVE_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_wp_en = atoi(cEndStr);
            nvt_dbg(ERR, "wp_en (now=%d)\r\n", g_wp_en);


            nvt_dbg(ERR, "Please enter the wp_sel_id number: ");
            cStrLen = IVE_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_wp_sel_id = atoi(cEndStr);
            nvt_dbg(ERR, "wp_sel_id (now=%d)\r\n", g_wp_sel_id);
#else
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
            break;
		case 'q':
#if defined(__FREERTOS)
            if (g_is_switch_dram == 1) {
                g_is_switch_dram = 0;
            } else {
                g_is_switch_dram = 1;
            }
            nvt_dbg(ERR, "switch_dram: NOW(%d)\r\n", g_is_switch_dram);
#endif
            break;
		case 'r':
#if defined(__FREERTOS)
			g_rand_burst++;
			if (g_rand_burst >= 5) {
				g_rand_burst = 0;
			}
			nvt_dbg(ERR, "burst_length: NOW(%d), 0~3:normal, 4:rand\r\n", g_rand_burst);
#endif
			break;
		case 't':
#if defined(__FREERTOS)
			g_ll_terminate++;
			if (g_ll_terminate >= 2) {
				g_ll_terminate = 0;
			}
			nvt_dbg(ERR, "ll_terminate: NOW(%d)\r\n", g_ll_terminate);
#endif
			break;
		case 'u':
#if defined(__FREERTOS)
            g_hw_rst_en++;
            if (g_hw_rst_en >= 4) {
                g_hw_rst_en = 0;
            }
            nvt_dbg(ERR, "hw_rst_en: NOW(%d) 1:hw, 2:sw 3:rand\r\n", g_hw_rst_en);
#endif
            break;
		case 'v':
#if defined(__FREERTOS)
			g_dram_outrange++;
            if (g_dram_outrange >= 3) {
                g_dram_outrange = 0;
            }
            nvt_dbg(ERR, "dram_outrange: NOW(%d)\r\n", g_dram_outrange);
#endif
            break;
		case 'x':
			if (g_reg_rw == 1) {
				g_reg_rw = 0;
			} else {
				g_reg_rw = 1;
			}
			nvt_dbg(ERR, "IVE_REG_RW: NOW(%d)\r\n", g_reg_rw);
			break;
		case 'y':
#if defined(__FREERTOS)
            g_is_heavy++;
            if (g_is_heavy >= 2) {
                g_is_heavy = 0;
            }
            nvt_dbg(ERR, "heavy mode: NOW(%d)\r\n", g_is_heavy);
#endif
            break;
		case 'z':
			return;
		break;
		default:
		break;
		}
	}	
}
#else
void emu_iveMain_drv(void)
{
	nvt_dbg(ERR, "IVE: Error, plaease set #define IVE_WORK_FLOW DISABLE at ive_platform.h.\r\n");
}
#endif //#if (IVE_SYS_VFY_EN == ENABLE)

