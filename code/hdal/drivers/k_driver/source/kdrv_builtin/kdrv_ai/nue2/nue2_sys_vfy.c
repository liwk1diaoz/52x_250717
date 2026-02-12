#include "nue2_platform.h"

#if (NUE2_SYS_VFY_EN == ENABLE)

#define EMUF_ENABLE 0

#include "nue2_sys_vfy.h"
#include "ai_drv.h"
#if defined(__FREERTOS)
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "kwrap/type.h"
#include "kwrap/platform.h"
//#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "comm/hwclock.h"

#else
#include "kdrv_ai_dbg.h"
#include "mach/fmem.h"
#include <kwrap/file.h>
#endif

#include "kwrap/task.h"
#include "nue2_dbg.h"

#include "comm/ddr_arb.h"
#include "../nue/nue_platform.h"
//#include "nue2_platform.h"
#include "kwrap/mem.h"
#include "nue_reg.h"
#include "nue2_reg.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "cnn_lib.h"
#include "nue_lib.h"
#include "nue_ll_cmd.h"
#include "nue2_lib.h"
#include "nue2_ll_cmd.h"
#include "nue2_int.h"
#include "kdrv_ai.h"
#include "nn_net.h"

#if defined(__FREERTOS)
#include "FileSysTsk.h"
#else
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#endif

#if (EMUF_ENABLE == 1)
#include "emu_framework/emuf_com_vfy.h"
#include "emu_framework/ai_basic_vfy.h"
#endif

#define SDK_TEST_IO_BASE    0x40000000
#define SDK_TEST_MODEL_BASE 0x20000000
#define SDK_DUMP_REG        0
#define SDK_DUMP_IO         0
#define NUE_TEST_RST        0
#define SDK_DUMP_INFO       0

#define TMP_DBG             0
#define NUE2_MAX_RESERVE_BUF_NUM 3
#define EMU_AI ENABLE
#define NUE2_OLD_PATTERN  1

#if defined(__FREERTOS)
#define emu_msg(msg)       printf msg
extern ER uart_getString(CHAR *pcString, UINT32 *pcBufferLen);
extern ER uart_getChar(CHAR *pcData);
#else
#endif

//#if defined(__FREERTOS)
//#define snprintf(A, B, C, D, E) sprintf(A, C, D, E)
//#else
//#endif

#define NUE2_CEILING(a, n)  (((a) + ((n)-1)) & (~((n)-1)))
#define NUE2_FLOOR(a,n)     ((a) & (~((n)-1)))
#define NUE2_MOD(a,b)       ((a)%(b))

#define NUE2_8_BYTE_ALIGN_CEILING(a)  NUE2_CEILING(a,8)
#define NUE2_64_BYTE_ALIGN_CEILING(a)  NUE2_CEILING(a,64)
#define NUE2_8_BYTE_ALIGN_FLOOR(a) NUE2_FLOOR(a,8)
#define NUE2_4_BYTE_ALIGN_CEILING(a)  NUE2_CEILING(a,4)

#define NUE2_256_CEILING(a) NUE2_CEILING(a,256)
#define NUE2_256_FLOOR(a) NUE2_FLOOR(a,256)

#define NUE2_WIDTH_OPT(OFS, WID) (((OFS) == 0) ? (WID) : (OFS))
#define NUE2_BIG_OPT(a, b) (((a) >= (b)) ? (a) : (b))
#define NUE2_MEMSET_VAL 0x00

extern UINT8 g_nue2_dram_mode;
extern UINT8 g_nue2_dram_outrange;
extern UINT32 g_mem_base, g_mem_base_2;
extern UINT32 g_mem_size, g_mem_size_2;
#define NUE2_SWITCH_DRAM(a) NUE2_4_BYTE_ALIGN_CEILING((((a) > 0x40000000) ? (((a) - g_mem_base_2) + g_mem_base) : (((a) - g_mem_base) + g_mem_base_2)))
#define NUE2_SWITCH_DRAM_1(a) NUE2_4_BYTE_ALIGN_CEILING((((a) > 0x40000000) ? (((a) - g_mem_base_2) + g_mem_base) : (a)))
#define NUE2_SWITCH_DRAM_2(a) NUE2_4_BYTE_ALIGN_CEILING((((a) > 0x40000000) ? (a) : (((a) - g_mem_base) + g_mem_base_2)))

#if defined(__FREERTOS)
#define NUE2_RESERVE_BUF_W ((4095+8)*2)
#define NUE2_RESERVE_BUF_H ((4095+8)*2)
#else
#define NUE2_RESERVE_BUF_W ((1920+8)*2)
#define NUE2_RESERVE_BUF_H ((1920+8)*2)
#endif

#if defined(__FREERTOS)
#define POOL_ID_APP_ARBIT		0
#define POOL_ID_APP_ARBIT2		1
extern UINT32 OS_GetMempoolAddr(UINT32 id);
extern UINT32 OS_GetMempoolSize(UINT32 id);
#endif

UINT32 nue2_dram_in_addr0 = 0,nue2_dram_in_addr1 = 0, nue2_dram_in_addr2 = 0;
UINT32 nue2_dram_in_size0 = 0, nue2_dram_in_size1 = 0, nue2_dram_in_size2 = 0;
UINT32 in_width_ofs_0 = 0, in_width_ofs_1 = 0, in_width_ofs_2 = 0;
UINT32 nue2_dram_out_addr0 = 0, nue2_dram_out_addr1 = 0, nue2_dram_out_addr2 = 0;
UINT32 nue2_dram_out_size0=0,nue2_dram_out_size1 = 0,nue2_dram_out_size2 = 0;
UINT32 nue2_dram_gld_addr0 = 0, nue2_dram_gld_addr1 = 0, nue2_dram_gld_addr2 = 0;
UINT32 nue2_dram_gld_size0 = 0, nue2_dram_gld_size1 = 0, nue2_dram_gld_size2 = 0;
UINT32 nue2_dram_addr_res[NUE2_MAX_RESERVE_BUF_NUM];
UINT32 nue2_dram_size_res[NUE2_MAX_RESERVE_BUF_NUM];


ER nue2_init_strip_parm(NUE2_TEST_PARM *p_parm);
ER nue2_check_strip_parm(NUE2_TEST_PARM *p_parm);
ER nue2_set_strip_parm(NUE2_STRIP_PARM *strip_parm, UINT64 **ll_buf, UINT8 is_bit60, UINT32 ll_base_addr);
VOID nue2_engine_loop_frameend(VOID *p_parm);
VOID nue2_engine_loop_llend(VOID *p_parm);
ER nue2_engine_setmode(NUE2_OPMODE mode, VOID *p_parm_in);
ER nue2_engine_flow(NUE2_TEST_PARM *p_parm);
VOID nue2_engine_isr(UINT32 int_status);
VOID nue2_debug_hook(VOID *p_parm);
VOID nue2_engine_debug_hook(VOID *p_parm);
VOID nue2_engine_debug_hook1(VOID *p_parm);
VOID nue2_engine_debug_hook2(VOID *p_parm);
VOID nue2_write_protect_test(UINT8 wp_en, UINT8 sel_id, NUE2_PARM *p_param);
int nue2_cmp_data(unsigned int rlt_addr, unsigned int size, unsigned int gld_addr, unsigned int is_check_four, unsigned int index);
#if defined(__FREERTOS)
extern VOID nue2_pt_interrupt_en(UINT8 is_en);
#endif
#define NUE2_ALL_CASES  18585 //30384 //18585
static unsigned int nue2_bringup_table[4] = {1, 10, 20, 30};
static unsigned int nue2_bringup_table_all[NUE2_ALL_CASES];
static UINT8 g_bringup_en=0;
static UINT8 g_is_heavy=0;
static UINT8 g_ll_mode_en=0;
static UINT8 g_cnt_is_hw_only=0;
static UINT32 g_s_num=1;
#if defined(__FREERTOS)
static UINT8 g_wp_en=0;
#endif
static UINT8 g_hw_rst_en=0;

static UINT8 g_rand_ch_en=0;
#if defined(__FREERTOS)
static UINT8 g_interrupt_en=1;
#else
static UINT8 g_interrupt_en=0; //Linux
#endif
static UINT8 g_fill_reg_only=0;


static VOID nue2_set_heavy_func(UINT8 heavy_mode, UINT8 dram_mode);

#if (EMUF_ENABLE == 1)
extern void emu_emufMain(UINT32 emu_id);
#endif

VOID nue2_int_en(UINT8 mode)
{
	g_interrupt_en = mode;
}

VOID nue2_rand_ch_en(UINT8 is_en)
{
	g_rand_ch_en = is_en;	
}

VOID nue2_rst_func(UINT8 mode) 
{
	g_hw_rst_en = mode;
}

VOID nue2_set_strip_num(UINT32 s_num)
{
	g_s_num = s_num;
}

VOID nue2_set_ll_mode_en(UINT8 is_en)
{
	g_ll_mode_en = is_en;
	if (g_ll_mode_en == 1) {
		g_cnt_is_hw_only = 1;
	} else {
		g_cnt_is_hw_only = 0;
	}
	DBG_ERR("NUE2:ll_mode_en(%d)\r\n", g_ll_mode_en);
}

VOID nue2_set_heavy_mode(UINT8 mode) //0: heavy, 1: channel disable/enable
{
	g_is_heavy = mode;
	DBG_ERR("NUE2:heavy_mode(%d)\r\n", g_is_heavy);
	nue2_set_heavy_func(g_is_heavy, g_nue2_dram_mode);
	return;
}

VOID nue2_set_dram_mode(UINT8 mode) //2: dram_2, 1: dram_1
{
	g_nue2_dram_mode = mode;
	DBG_ERR("NUE2:dram_mode(%d)\r\n", g_nue2_dram_mode);
	return;
}

unsigned int nue2_get_index(unsigned int idx)
{
	unsigned int index;

	if (g_bringup_en >= 1) {
		if (g_bringup_en == 2) {
			index = nue2_bringup_table_all[idx];
		} else {
			index = nue2_bringup_table[idx];
		}
	} else {
		index = idx;
	}

	return index;
}

void nue2_check_reg_rw(void)
{
    volatile UINT32 *reg_data = (UINT32 *) 0xf0d50000; //nue2

    NUE2_CHECK_REG_RW_BY_OFS(0x4, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x8, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0xc, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0x10, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0x14, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0x18, 0x5A5A5A5A, 0xA5A5A5A5);

    NUE2_CHECK_REG_RW_BY_OFS(0x1c, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0x20, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0x24, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x28, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x2c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x30, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x34, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x38, 0xFFFFFFFF, 0x0);

    NUE2_CHECK_REG_RW_BY_OFS(0x3c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x40, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x44, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x48, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x4c, 0x5A5A5A5A, 0xA5A5A5A5);
    NUE2_CHECK_REG_RW_BY_OFS(0x50, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x54, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x58, 0xFFFFFFFF, 0x0);

    NUE2_CHECK_REG_RW_BY_OFS(0x5c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x60, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x64, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x68, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x6c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x70, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x74, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x78, 0xFFFFFFFF, 0x0);

    NUE2_CHECK_REG_RW_BY_OFS(0x7c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x80, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x84, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x88, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x8c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0x90, 0xFFFFFFFF, 0x0);
	NUE2_CHECK_REG_RW_BY_OFS(0x94, 0xFFFFFFFF, 0x0);
	NUE2_CHECK_REG_RW_BY_OFS(0x98, 0xFFFFFFFF, 0x0);
	NUE2_CHECK_REG_RW_BY_OFS(0x9c, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0xf0, 0xFFFFFFFF, 0x0);
    NUE2_CHECK_REG_RW_BY_OFS(0xf4, 0xFFFFFFFF, 0x0);

    return;
}

/**
    NUE2 Reset

    Enable/disable NUE2 HW reset.

    @param[in] bReset.
        \n-@b TRUE: enable reset.
        \n-@b FALSE: disable reset.

    @return None.
*/
#if defined(__FREERTOS)
VOID nue2_engine_rst(UINT8 mode)
{
	UINT32 ch_idle=0;
	UINT32 count;
	UINT32 reg_data=0;

	//Module Reset Control Register 0
	//AXI
#if defined(_BSP_NA51089_)
	T_NUE2_DMA_DISABLE_REGISTER0_528 reg0_528;
#else
	T_DMA_DISABLE_REGISTER0 reg0;
	T_DMA_DISABLE_REGISTER0_528 reg0_528;
#endif
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
#if defined(_BSP_NA51089_)
	UINT32 ofs_528 = NUE2_DMA_DISABLE_REGISTER0_OFS_528;
#else
	UINT32 ofs = DMA_DISABLE_REGISTER0_OFS;
	UINT32 ofs_528 = DMA_DISABLE_REGISTER0_OFS_528;
#endif
	
	volatile UINT32 *reg_sys_rst = (UINT32 *) 0xf0020088;
	volatile UINT32 *reg_sw_rst = (UINT32 *) 0xf0d50000;

#if defined(_BSP_NA51089_)
		reg0_528.reg = NUE2_GETDATA(ofs_528, base_addr);
		reg0_528.bit.DMA_DISABLE = 1;
		NUE2_SETDATA(ofs_528, reg0_528.reg, base_addr);
#else
		if(nvt_get_chip_id() == CHIP_NA51055) {
			reg0.reg = NUE2_GETDATA(ofs, base_addr);
			reg0.bit.DMA_DISABLE = 1;
			NUE2_SETDATA(ofs, reg0.reg, base_addr);
		} else {
			reg0_528.reg = NUE2_GETDATA(ofs_528, base_addr);
			reg0_528.bit.DMA_DISABLE = 1;
			NUE2_SETDATA(ofs_528, reg0_528.reg, base_addr);
		}
#endif

	ch_idle = 0;
	count = 0;
	while (ch_idle == 0) {
#if defined(_BSP_NA51089_)
		reg0_528.reg = NUE2_GETDATA(ofs_528, base_addr);
		ch_idle = reg0_528.bit.NUE2_IDLE;
		DBG_EMU( "HW_RST(528): reg=0x%x\r\n", (UINT32) reg0_528.reg);
#else
		if(nvt_get_chip_id() == CHIP_NA51055) {
			reg0.reg = NUE2_GETDATA(ofs, base_addr);
			ch_idle = reg0.bit.NUE2_IDLE;
			DBG_EMU( "HW_RST: reg=0x%x\r\n", (UINT32) reg0.reg);
		} else {
			reg0_528.reg = NUE2_GETDATA(ofs_528, base_addr);
			ch_idle = reg0_528.bit.NUE2_IDLE;
			DBG_EMU( "HW_RST(528): reg=0x%x\r\n", (UINT32) reg0_528.reg);
		}
#endif
		vos_task_delay_ms(10);
		if (count++ > 200) {
			DBG_EMU( "HW_RST_1: nue2 ch is still not in idle.\r\n");
			break;
		}
	}

	if (mode == 1) {
		//reset
		if (ch_idle == 1) {
			reg_data  = *reg_sys_rst;
			reg_data &= ~(1 << (15));
			*reg_sys_rst = reg_data;
			//Released
			vos_task_delay_ms(100);
			reg_data |= 1 << (15);
			*reg_sys_rst = reg_data;
			DBG_EMU( "HW_RST(Done)..\r\n");
		} else {
			DBG_EMU( "HW_RST: Error not idle state.\r\n");
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
			DBG_EMU( "SW_RST(Done)..\r\n");
		} else {
			DBG_EMU( "HW_RST: Error not idle state.\r\n");
		}
	} else if (mode == 3) {
		reg_data  = *reg_sys_rst;
		reg_data &= ~(1 << (15));
		*reg_sys_rst = reg_data;
	} else if (mode == 4) {
		reg_data = *reg_sw_rst;
		reg_data |= 1;
		*reg_sw_rst = reg_data;
	} else if (mode == 5) {
		//Released
		vos_task_delay_ms(100);
		reg_data |= 1 << (15);
		*reg_sys_rst = reg_data;

		vos_task_delay_ms(100);
		reg_data &= (~(1));
		*reg_sw_rst = reg_data;
	}
}
#endif

VOID nue2_engine_setdata_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value, UINT32 ll_idx, UINT8 is_bit60, UINT32 ll_fill_reg_num, UINT8 is_switch_dram, UINT8 is_err_cmd, 
							UINT32 ll_base_addr)
{
	UINT32 fill_num;
	UINT64 *ll_buf_tmp;
	//UINT64 *ll_buf_tmp_start;
	UINT8 is_switch_buff;
	static UINT8 is_first=1;
	static UINT8 dram_mode=2;
	INT32 r_value_tmp;

	if (is_first == 1) {
		dram_mode = g_nue2_dram_mode;
		is_first = 0;
	}

	//DBG_EMU( "LL_SWITCH_SETL: is_switch(%d) is_bit60(%d)\r\n", is_switch_dram, is_bit60);

	if ((is_switch_dram == 1) && (is_bit60 == 0)) {
		if (dram_mode == 2) {
			dram_mode = 1;
			ll_buf_tmp = (UINT64 *) NUE2_SWITCH_DRAM_1((UINT32) *llbuf);
			DBG_EMU( "LL_dram1: 0x%x <- 0x%x\r\n", (UINT32)ll_buf_tmp, (UINT32)*llbuf);
		} else {
			dram_mode = 2;
			ll_buf_tmp = (UINT64 *) NUE2_SWITCH_DRAM_2((UINT32) *llbuf);
			DBG_EMU( "LL_dram2: 0x%x <- 0x%x\r\n", (UINT32)ll_buf_tmp, (UINT32)*llbuf);
		}
		is_switch_buff = 1;
	} else {
		ll_buf_tmp = (UINT64 *) *llbuf;
		is_switch_buff = 0;
	}
	//ll_buf_tmp_start = ll_buf_tmp;

	//DBG_EMU( "LL_SWITCH_SETL: is_switch_buff(%d)\r\n", is_switch_buff);

	if (is_switch_buff == 0) { 
	} else {
		(ll_buf_tmp)++;
		**llbuf = nue2_ll_nextupd_cmd(nue2_pt_va2pa((UINT32) ll_buf_tmp));
		//DBG_EMU( "LL_SWITCH: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (*llbuf), (UINT64) **llbuf);
		(*llbuf)++;
	}

	for (fill_num = 0; fill_num < ll_fill_reg_num; fill_num++) {
		if (ll_base_addr != 0) {
			r_value_tmp = (INT32) (nue2_pt_va2pa(r_value) - ll_base_addr);
			nue2_setdata_ll(&ll_buf_tmp, r_ofs, (UINT32)r_value_tmp, 2); //bit59
		} else {
			nue2_setdata_ll(&ll_buf_tmp, r_ofs, nue2_pt_va2pa(r_value), 0);
		}
		//DBG_EMU( "LL_SWITCH_SET_LL: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
		#if 0
		if (is_err_cmd == 2) {
			if (fill_num == (ll_fill_reg_num / 2)) {
				*ll_buf_tmp = 0xffff000000000000;
				ll_buf_tmp++;
			}
		}
		#endif
	}

	if (is_switch_buff == 1) {//is_bit60==0
		nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, 0);
		if (g_nue2_dram_mode == 2) {
			*llbuf = (UINT64 *) NUE2_SWITCH_DRAM_2((UINT32) ll_buf_tmp);
			DBG_EMU( "LL_g_dram2_exit: 0x%x <- 0x%x\r\n", (UINT32) *llbuf, (UINT32) ll_buf_tmp);
		} else {
			*llbuf = (UINT64 *) NUE2_SWITCH_DRAM_1((UINT32) ll_buf_tmp);
			DBG_EMU( "LL_g_dram1_exit: 0x%x <- 0x%x\r\n", (UINT32) *llbuf, (UINT32) ll_buf_tmp);
		}
        *ll_buf_tmp = nue2_ll_nextupd_cmd(nue2_pt_va2pa((UINT32) *llbuf));
		ll_buf_tmp++;
		//nue2_pt_dma_flush((UINT32) ll_buf_tmp_start, (UINT32) (ll_buf_tmp - ll_buf_tmp_start));
		//DBG_EMU( "LL_SWITCH: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp), (UINT64) *ll_buf_tmp);
	} else { //is_bit60==1
		if (is_bit60 == 0) {
			if (is_err_cmd == 1) {
				nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, is_err_cmd);
			} else if (is_err_cmd == 2) {
				nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, is_err_cmd);
			} else {
				nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, 0);
			}
		} else {
			if (is_err_cmd == 2) {
				nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, is_err_cmd);
			}
			nue2_setdata_ll(&ll_buf_tmp, r_ofs, nue2_pt_va2pa(r_value), 1); //for trigger only (no bit59)
		}
		//DBG_EMU( "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
		//nue2_pt_dma_flush((UINT32) ll_buf_tmp_start, (UINT32) (ll_buf_tmp - ll_buf_tmp_start));
		*llbuf = ll_buf_tmp;
		DBG_EMU( "LL_fill_exit: llbuf(0x%x), ll_buf_tmp(0x%x)\r\n", (UINT32) *llbuf, (UINT32) ll_buf_tmp);
	}
	
}

VOID nue2_prepare_ll_buffer(UINT64 **llbuf, UINT8 is_bit60, UINT32 ll_fill_reg_num, UINT32 ll_fill_num, UINT8 is_switch_dram, UINT8 is_err_cmd,
							UINT32 in_reg_addr, UINT32 out_reg_addr, UINT32 *out_buff_addr, UINT32 *out_buff_size, UINT32 out_buff_num, UINT32 ll_base_addr)
{
	UINT64 *ll_buf_tmp= (UINT64 *) *llbuf;
	UINT32 backup_addr;
	UINT32 ll_idx;
	UINT32 fill_num;
	UINT32 idx;

	backup_addr = nue2_dram_in_addr0;
	out_buff_addr[out_buff_num - 1] = nue2_dram_out_addr0;
	out_buff_size[out_buff_num - 1] = nue2_dram_out_size0;
	for (idx=0; idx < out_buff_num; idx++) { //output_num: NUE2_MAX_RESERVE_BUF_NUM
		if (g_nue2_dram_outrange == 1) {
			continue;
		}
		if (out_buff_size[idx] == 0) nvt_dbg(ERR, "Error, the reserved size is 0. at %s\r\n", __FUNCTION__);
		out_buff_size[idx] = nue2_dram_out_size0; //for 0x18A
		memset((void *) out_buff_addr[idx], 0, out_buff_size[idx]);
		nue2_pt_dma_flush(out_buff_addr[idx], out_buff_size[idx]);
	}
	ll_idx = 0;

	for (fill_num=0; fill_num < ll_fill_num; fill_num++) {
		nue2_engine_setdata_ll(&ll_buf_tmp, 0x18, out_buff_addr[(fill_num % out_buff_num)], ll_idx++, is_bit60, ll_fill_reg_num, is_switch_dram, is_err_cmd, 
								ll_base_addr);
		//bit59 is only for address others 0
	}

	nue2_engine_setdata_ll(&ll_buf_tmp, 0x18, nue2_dram_out_addr0, ll_idx++, is_bit60, ll_fill_reg_num, 0, 0, ll_base_addr); //bit59 is only for address others 0
	//DBG_EMU( "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
	nue2_engine_setdata_ll(&ll_buf_tmp, 0x08, backup_addr, ll_idx++, is_bit60, ll_fill_reg_num, 0, 0, ll_base_addr); //bit59 is only for address others 0
	//DBG_EMU( "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
	nue2_setdata_exit_ll(&ll_buf_tmp, 0);
	//DBG_EMU( "LL_SWITCH_0: addr=(0x%x), ll_buf=0x%llx \r\n", (UINT32) (ll_buf_tmp-1), (UINT64) *(ll_buf_tmp-1));
	*llbuf = ll_buf_tmp;

	return;
}

void nue2_reg_clr_all(void)
{
    volatile UINT32 *reg_data = (UINT32 *) 0xf0d50000; //nue2

    NUE2_REG_SET(0x4, 0x0);
    NUE2_REG_SET(0x8, 0x0);
    NUE2_REG_SET(0xc, 0x0);
    NUE2_REG_SET(0x10, 0x0);
    //NUE2_REG_SET(0x14, 0x0);
    NUE2_REG_SET(0x18, 0x0);


    NUE2_REG_SET(0x1c, 0x0);
    NUE2_REG_SET(0x20, 0x0);
    NUE2_REG_SET(0x24, 0x0);
    NUE2_REG_SET(0x28, 0x0);
    NUE2_REG_SET(0x2c, 0x0);
    NUE2_REG_SET(0x30, 0x0);
    NUE2_REG_SET(0x34, 0x0);
    NUE2_REG_SET(0x38, 0x0);



    //NUE2_REG_SET(0x3c, 0x0);
    //NUE2_REG_SET(0x40, 0x0);
    NUE2_REG_SET(0x44, 0x0);
    NUE2_REG_SET(0x48, 0x0);
    NUE2_REG_SET(0x4c, 0x0);
    NUE2_REG_SET(0x50, 0x0);
    NUE2_REG_SET(0x54, 0x0);
    NUE2_REG_SET(0x58, 0x0);

    NUE2_REG_SET(0x5c, 0x0);
    NUE2_REG_SET(0x60, 0x0);
    NUE2_REG_SET(0x64, 0x0);
    NUE2_REG_SET(0x68, 0x0);
    NUE2_REG_SET(0x6c, 0x0);
    NUE2_REG_SET(0x70, 0x0);
    NUE2_REG_SET(0x74, 0x0);
    NUE2_REG_SET(0x78, 0x0);


    NUE2_REG_SET(0x7c, 0x0);
    //NUE2_REG_SET(0x80, 0x0);
    //NUE2_REG_SET(0x84, 0x0);
	//NUE2_REG_SET(0x88, 0x0);
    //NUE2_REG_SET(0x8c, 0x0);
    //NUE2_REG_SET(0x90, 0x0);
	//NUE2_REG_SET(0x94, 0x0);
	//NUE2_REG_SET(0x98, 0x0);
	//NUE2_REG_SET(0x9c, 0x0);
    //NUE2_REG_SET(0xf0, 0x0);
    //NUE2_REG_SET(0xf4, 0x0);

    return;
}

void nue2_reg_set_all(void)
{
    volatile UINT32 *reg_data = (UINT32 *) 0xf0d50000; //nue2

    NUE2_REG_SET(0x4, 0xffffffff);
    NUE2_REG_SET(0x8, 0xffffffff);
    NUE2_REG_SET(0xc, 0xffffffff);
    NUE2_REG_SET(0x10, 0xffffffff);
    //NUE2_REG_SET(0x14, 0xffffffff);
    NUE2_REG_SET(0x18, 0xffffffff);


    NUE2_REG_SET(0x1c, 0xffffffff);
    NUE2_REG_SET(0x20, 0xffffffff);
    NUE2_REG_SET(0x24, 0xffffffff);
    NUE2_REG_SET(0x28, 0xffffffff);
    NUE2_REG_SET(0x2c, 0xffffffff);
    NUE2_REG_SET(0x30, 0xffffffff);
    NUE2_REG_SET(0x34, 0xffffffff);
    NUE2_REG_SET(0x38, 0xffffffff);



    //NUE2_REG_SET(0x3c, 0xffffffff);
    //NUE2_REG_SET(0x40, 0xffffffff);
    NUE2_REG_SET(0x44, 0xffffffff);
    NUE2_REG_SET(0x48, 0xffffffff);
    NUE2_REG_SET(0x4c, 0xffffffff);
    NUE2_REG_SET(0x50, 0xffffffff);
    NUE2_REG_SET(0x54, 0xffffffff);
    NUE2_REG_SET(0x58, 0xffffffff);

    NUE2_REG_SET(0x5c, 0xffffffff);
    NUE2_REG_SET(0x60, 0xffffffff);
    NUE2_REG_SET(0x64, 0xffffffff);
    NUE2_REG_SET(0x68, 0xffffffff);
    NUE2_REG_SET(0x6c, 0xffffffff);
    NUE2_REG_SET(0x70, 0xffffffff);
    NUE2_REG_SET(0x74, 0xffffffff);
    NUE2_REG_SET(0x78, 0xffffffff);


    NUE2_REG_SET(0x7c, 0xffffffff);
    //NUE2_REG_SET(0x80, 0xffffffff);
    //NUE2_REG_SET(0x84, 0xffffffff);
	//NUE2_REG_SET(0x88, 0xffffffff);
    //NUE2_REG_SET(0x8c, 0xffffffff);
    //NUE2_REG_SET(0x90, 0xffffffff);
	//NUE2_REG_SET(0x94, 0xffffffff);
	//NUE2_REG_SET(0x98, 0xffffffff);
	//NUE2_REG_SET(0x9c, 0xffffffff);
    //NUE2_REG_SET(0xf0, 0xffffffff);
    //NUE2_REG_SET(0xf4, 0xffffffff);

    return;
}

void nue2_reg_dump_print(void)
{
    volatile UINT32 *reg_data = (UINT32 *) 0xf0d50000; //nue2

    NUE2_REG_DUMP(0x4);
    NUE2_REG_DUMP(0x8);
    NUE2_REG_DUMP(0xc);
    NUE2_REG_DUMP(0x10);
    NUE2_REG_DUMP(0x14);
    NUE2_REG_DUMP(0x18);

    NUE2_REG_DUMP(0x1c);
    NUE2_REG_DUMP(0x20);
    NUE2_REG_DUMP(0x24);
    NUE2_REG_DUMP(0x28);
    NUE2_REG_DUMP(0x2c);
    NUE2_REG_DUMP(0x30);
    NUE2_REG_DUMP(0x34);
    NUE2_REG_DUMP(0x38);

    NUE2_REG_DUMP(0x3c);
    NUE2_REG_DUMP(0x40);
    NUE2_REG_DUMP(0x44);
    NUE2_REG_DUMP(0x48);
    NUE2_REG_DUMP(0x4c);
    NUE2_REG_DUMP(0x50);
    NUE2_REG_DUMP(0x54);
    NUE2_REG_DUMP(0x58);

    NUE2_REG_DUMP(0x5c);
    NUE2_REG_DUMP(0x60);
    NUE2_REG_DUMP(0x64);
    NUE2_REG_DUMP(0x68);
    NUE2_REG_DUMP(0x6c);
    NUE2_REG_DUMP(0x70);
    NUE2_REG_DUMP(0x74);
    NUE2_REG_DUMP(0x78);

    NUE2_REG_DUMP(0x7c);
    NUE2_REG_DUMP(0x80);
    NUE2_REG_DUMP(0x84);
	NUE2_REG_DUMP(0x88);
    NUE2_REG_DUMP(0x8c);
    NUE2_REG_DUMP(0x90);
	NUE2_REG_DUMP(0x94);
	NUE2_REG_DUMP(0x98);
	NUE2_REG_DUMP(0x9c);
    NUE2_REG_DUMP(0xf0);
    NUE2_REG_DUMP(0xf4);

    return;
}

UINT32 ai_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
    return dma_getPhyAddr(addr);
#elif defined (__FREERTOS)
    return vos_cpu_get_phy_addr(addr);
#else
    return fmem_lookup_pa(addr);
#endif
}

int nue2_cmp_data(unsigned int rlt_addr, unsigned int size, unsigned int gld_addr, unsigned int is_check_four, unsigned int index)
{
    int len=0;
    int c_idx;
    unsigned char *rlt_ptr = (unsigned char *) (rlt_addr);
    unsigned char *gld_ptr = (unsigned char *) (gld_addr);
	unsigned char check_value = NUE2_MEMSET_VAL;
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

	if (is_check_four) {
		//first four bytes
		rlt_ptr = (unsigned char *) (rlt_addr - 4);
		for(c_idx = 0; (unsigned int)c_idx < 4; c_idx++) {
			if (rlt_ptr[c_idx] != check_value) {
				nvt_dbg(ERR, "Error, first four byte compare fail at c_idx(0x%x) rlf_addr(0x%p) value=0x%x vs (check:%d)\r\n",
								c_idx, &rlt_ptr[c_idx], rlt_ptr[c_idx], check_value);
				len = -1;
				//goto retend;
			}
		}
		if (len >= 0) {
			nvt_dbg(ERR, "###first four byte is ok. idx(%d)\r\n", index);
		}
		//last four bytes
		rlt_ptr = (unsigned char *) (rlt_addr + size);
		for(c_idx = 0; (unsigned int)c_idx < 4; c_idx++) {
			if (rlt_ptr[c_idx] != check_value) {
				nvt_dbg(ERR, "Error, last four byte compare fail at c_idx(0x%x) rlf_addr(0x%p) value=0x%x vs (check:%d)\r\n",
								c_idx, &rlt_ptr[c_idx], rlt_ptr[c_idx], check_value);
				len = -1;
				//goto retend;
			}
		}
		if (len >= 0) {
			nvt_dbg(ERR, "###last four byte is ok. idx(%d)\r\n", index);
		}
	}


	rlt_ptr = (unsigned char *) (rlt_addr);
	for(c_idx = 0; (unsigned int)c_idx < size; c_idx++) {

		if (rlt_ptr[c_idx] != gld_ptr[c_idx]) {
			nvt_dbg(ERR, "Error, compare fail at c_idx(0x%x) rlf_addr(0x%p) gld_addr(0x%p) value=0x%x vs 0x%x\r\n",
							c_idx, &rlt_ptr[c_idx], &gld_ptr[c_idx], rlt_ptr[c_idx], gld_ptr[c_idx]);
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

#if 1
static int find_data_size(char* path)
{
#if defined(__FREERTOS)
	unsigned int file_buf_size;
#else
	mm_segment_t old_fs;
	struct file *fp;
	struct kstat statbuf = {0};
	int ret = 0;
#endif
	int len = 0;

#if defined(__FREERTOS)

    file_buf_size = FileSys_GetFileLen(path);
	len = file_buf_size;
	if (len == 0) {
		nvt_dbg(ERR, "Error to get file size. at %s\r\n", path);
	}
#else
	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	ret = vfs_stat(path, &statbuf);
	if (0 == ret) {
		len = (unsigned int)statbuf.size;
	} else {
		len = 0;
		nvt_dbg(ERR, "Error to get file size. at %s\r\n", path);
	}
	filp_close(fp, NULL);
	set_fs(old_fs);
#endif

	return len;
}
#endif

static int load_data(char* path, unsigned int addr, unsigned int size)
{
#if defined(__FREERTOS)
	FST_FILE file;
	INT32 fstatus;
#else
	//mm_segment_t old_fs;
	//struct file *fp;
	int fd;
#endif
	int len = 0;

#if defined(__FREERTOS)

	DBG_EMU( "WW###: path=%s addr=0x%x size=%d\n", path, addr, size);

	file = FileSys_OpenFile(path, FST_OPEN_READ);
    if (file == 0) {
        nvt_dbg(ERR, "Invalid file: %s\r\n", path);
        return -1;
    }
	fstatus = FileSys_ReadFile(file, (UINT8 *)addr, (UINT32 *) &size, 0, NULL);
	if (fstatus != 0) {
		nvt_dbg(ERR, "%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
        return -1;
    }
	FileSys_CloseFile(file);
	len = size;
#else
	/*fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len = vfs_read(fp, (void*)addr, size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);*/
	fd = vos_file_open(path, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", path);
		return -1;
	}
		
	len = vos_file_read(fd, (void *)addr, size);
	vos_file_close(fd);
#endif

	return len;
}

static int dump_data(char* path, unsigned int addr, unsigned int size)
{
#if defined(__FREERTOS)
	FST_FILE file;
	INT32 fstatus;
#else
	//mm_segment_t old_fs;
	//struct file *fp;
	int fd;
#endif
	int len = 0;

#if defined(__FREERTOS)
	file = FileSys_OpenFile(path, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
    if (file == 0) {
        nvt_dbg(ERR, "Invalid file: %s\r\n", path);
        return -1;
    }
	len = (int) size;
    fstatus = FileSys_WriteFile(file, (UINT8 *)addr, (UINT32 *) &size, 0, NULL);
	if (fstatus < 0) {
		nvt_dbg(ERR, "%s:%dError to FileSys_WriteFile at %s\n", __FILE__, __LINE__, path);
		FileSys_CloseFile(file);
		return -1;
	}
	FileSys_CloseFile(file);
#else
	/*
	fp = filp_open(path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len = vfs_write(fp, (const char __user *)addr, size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
	*/
	fd = vos_file_open(path, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "open %s failure\r\n", path);
		return -1;
	}
	
	len = vos_file_write(fd, (void *)addr, size);
	vos_file_close(fd);
	
	
#endif

	return len;
}



static int nue_calc_output_size(NT98520_NUE_REG_STRUCT* p_nue_reg, unsigned int *out_size)
{

	if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 0) {
		/*SVM*/
		*out_size = p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * p_nue_reg->NUE_Register_0048.bit.OBJ_NUM;
		if (p_nue_reg->NUE_Register_0008.bit.SVM_DMAOEN == 0) {
			*out_size = 0;
		} else if (p_nue_reg->NUE_Register_0008.bit.SVM_DMAO_PATH > 1
				 || (p_nue_reg->NUE_Register_0008.bit.SVM_DMAO_PATH == 1
				 && p_nue_reg->NUE_Register_0008.bit.SVM_SCIENTIFIC_NOTATION_EN == 0) ) {
			*out_size *= sizeof(INT32);
		} else if (p_nue_reg->NUE_Register_0004.bit.OUT_BIT_DEPTH == 1
				 || (p_nue_reg->NUE_Register_0008.bit.SVM_DMAO_PATH == 1
				 && p_nue_reg->NUE_Register_0008.bit.SVM_SCIENTIFIC_NOTATION_EN == 1)) {
			*out_size *= sizeof(INT16);
		} else {
			*out_size *= sizeof(INT8);
		}

	} else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 1) {
		/*ROI Pooling*/
		//*out_size = (p_nue_reg->NUE_Register_0034.bit.DRAM_OFSO << 2)*p_nue_reg->NUE_Register_0048.bit.ROI_NUM;

	} else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 3 || p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 4
			|| p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 5 || p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 6) {
		/* Reorganization & Permute & Anchor transform & softmax */
		*out_size = (p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH * p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL);
		if(p_nue_reg->NUE_Register_0004.bit.OUT_BIT_DEPTH == 1) {
			*out_size *= sizeof(INT16);
		} else {
			*out_size *= sizeof(INT8);
		}
	}

	return 0;
}

static unsigned int sdk_addr_update(unsigned int input_addr, unsigned int buf_base, unsigned int vsgen_base)
{
	unsigned int rst_addr = 0;

	if (input_addr > 0 && input_addr >= vsgen_base) {
		rst_addr = input_addr - vsgen_base + buf_base;
	}

	return rst_addr;
}

static unsigned int sdk_dump_output(NN_IOMEM* out_info, unsigned int layer, unsigned int io_base)
{
#if 0
	char path[100];
	int i = 0;
	unsigned int remap_va;
	int len = 0;

	for (i = 0; i < NN_OMEM_NUM; i++) {
		if (out_info->SAO[i].size > 0 && out_info->SAO[i].va > 0) {
			remap_va = sdk_addr_update(out_info->SAO[i].va, io_base, SDK_TEST_IO_BASE);
			#if SDK_DUMP_INFO
			nvt_dbg(ERR, "layer %d sao %d addr(origin) = 0x%08X\n", layer, i, out_info->SAO[i].va);
			nvt_dbg(ERR, "layer %d sao %d addr(re-map) = 0x%08X size = %d\n", layer, i, remap_va, out_info->SAO[i].size);
			#endif
#if defined(__FREERTOS)
			snprintf(path, 64, "A:\\sdk_output\\out_%d_%d.bin", layer, i);
#else
			snprintf(path, 64, "//mnt//sd//sdk_output//out_%d_%d.bin", layer, i);
#endif
			if (remap_va > 0) {
				len = dump_data(path, remap_va, out_info->SAO[i].size);
				if (len == 0) {
					nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
				}
			}
		}
	}
#endif
	return 0;
}

static unsigned int sdk_update_ioaddr(unsigned int is_first_layer, unsigned int addr, unsigned int io_base, unsigned int model_base,
									unsigned int mem_base, unsigned int mem_size, unsigned int ctrl_num, NN_GEN_MODE_CTRL* mode_ctrl)
{
	char io_path[100];
	unsigned int len = 0;
	UINT32 parm_ofs = 0;

	if (mode_ctrl->mode == NN_CONV || mode_ctrl->mode == NN_DECONV || mode_ctrl->mode == NN_ELTWISE
	|| mode_ctrl->mode == NN_POOL || mode_ctrl->mode == NN_BNSCALE || mode_ctrl->mode == NN_SCALEUP) {
		KDRV_AI_NEURAL_PARM* p_parm = (KDRV_AI_NEURAL_PARM*)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out0      = 0x%08X\r\n", p_parm->out0_addr);
		nvt_dbg(IND, "out1      = 0x%08X\r\n", p_parm->out1_addr);
		nvt_dbg(IND, "in_inter  = 0x%08X\r\n", p_parm->in_interm_addr);
		nvt_dbg(IND, "out_inter = 0x%08X\r\n", p_parm->out_interm_addr);
		nvt_dbg(IND, "tmp_buf   = 0x%08X\r\n", p_parm->tmp_buf_addr);
		nvt_dbg(IND, "weight    = 0x%08X\r\n", p_parm->conv.weight_addr);
		nvt_dbg(IND, "bias      = 0x%08X\r\n", p_parm->conv.bias_addr);
		nvt_dbg(IND, "fcd       = 0x%08X\r\n", p_parm->conv.fcd.quanti_kmeans_addr);
		nvt_dbg(IND, "bn        = 0x%08X\r\n", p_parm->norm.bn_scl.bn_scale_addr);
		nvt_dbg(IND, "elt       = 0x%08X\r\n", p_parm->elt.addr);

		nvt_dbg(IND, "bitlen      = 0x%08X\r\n", p_parm->conv.fcd.enc_bit_length);
		nvt_dbg(IND, "out_en      = %d %d\r\n", p_parm->out0_dma_en, p_parm->out1_dma_en);
		nvt_dbg(IND, "conv ker    = %d %d\r\n", p_parm->conv.ker_w, p_parm->conv.ker_h);
		nvt_dbg(IND, "act1 relu leaky = %d %d\r\n", p_parm->act1.relu.leaky_val, p_parm->act1.relu.leaky_shf);
		nvt_dbg(IND, "======chk scale / shift======\r\n");

		nvt_dbg(IND, "elt  in  scl/shift  = %d %d\r\n", p_parm->elt.sclshf.in_scale, p_parm->elt.sclshf.in_shift);
		nvt_dbg(IND, "conv out scl/shift  = %d %d\r\n", p_parm->conv.sclshf.out_scale, p_parm->conv.sclshf.out_shift);
		nvt_dbg(IND, "elt  out scl/shift  = %d %d\r\n", p_parm->elt.sclshf.out_scale, p_parm->elt.sclshf.out_shift);
		nvt_dbg(IND, "act0 out scl/shift  = %d %d\r\n", p_parm->act.sclshf.out_scale, p_parm->act.sclshf.out_shift);
		nvt_dbg(IND, "act1 out scl/shift  = %d %d\r\n", p_parm->act1.sclshf.out_scale, p_parm->act1.sclshf.out_shift);
		nvt_dbg(IND, "pool out scl/shift  = %d %d\r\n", p_parm->pool.sclshf.out_scale, p_parm->pool.sclshf.out_shift);
		nvt_dbg(IND, "======chk pool======\r\n");
		nvt_dbg(IND, "pool div_type  = %d \r\n", (int)p_parm->pool.local.pool_div_type);
		nvt_dbg(IND, "pool cal_type  = %d \r\n", (int)p_parm->pool.local.pool_cal_type);
		#endif
		p_parm->in_addr 		= sdk_addr_update(p_parm->in_addr, 		   io_base, SDK_TEST_IO_BASE);
		p_parm->out0_addr 		= sdk_addr_update(p_parm->out0_addr, 	   io_base, SDK_TEST_IO_BASE);
		p_parm->out1_addr 		= sdk_addr_update(p_parm->out1_addr, 	   io_base, SDK_TEST_IO_BASE);
		p_parm->in_interm_addr  = sdk_addr_update(p_parm->in_interm_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_interm_addr = sdk_addr_update(p_parm->out_interm_addr, io_base, SDK_TEST_IO_BASE);
		p_parm->tmp_buf_addr    = sdk_addr_update(p_parm->tmp_buf_addr,    io_base, SDK_TEST_IO_BASE);
		p_parm->elt.addr        = sdk_addr_update(p_parm->elt.addr,        io_base, SDK_TEST_IO_BASE);

		p_parm->conv.weight_addr            = sdk_addr_update(p_parm->conv.weight_addr,            model_base, SDK_TEST_MODEL_BASE);
		p_parm->conv.bias_addr              = sdk_addr_update(p_parm->conv.bias_addr,              model_base, SDK_TEST_MODEL_BASE);
		p_parm->conv.fcd.quanti_kmeans_addr = sdk_addr_update(p_parm->conv.fcd.quanti_kmeans_addr, model_base, SDK_TEST_MODEL_BASE);
		p_parm->norm.bn_scl.bn_scale_addr   = sdk_addr_update(p_parm->norm.bn_scl.bn_scale_addr,   model_base, SDK_TEST_MODEL_BASE);

		parm_ofs = mode_ctrl->addr;
		p_parm->conv.fcd.p_vlc_code    = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_code + parm_ofs);
		p_parm->conv.fcd.p_vlc_valid   = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_valid + parm_ofs);
		p_parm->conv.fcd.p_vlc_ofs     = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_ofs + parm_ofs);
	} else if (mode_ctrl->mode == NN_ROIPOOLING) {
		KDRV_AI_ROIPOOL_PARM* p_parm = (KDRV_AI_ROIPOOL_PARM*)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		nvt_dbg(IND, "roi       = 0x%08X\r\n", p_parm->roi_ker.roi_addr);
		#endif
		p_parm->in_addr  = sdk_addr_update(p_parm->in_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr = sdk_addr_update(p_parm->out_addr, io_base, SDK_TEST_IO_BASE);
		p_parm->roi_ker.roi_addr = sdk_addr_update(p_parm->roi_ker.roi_addr, model_base, SDK_TEST_MODEL_BASE);
	} else if (mode_ctrl->mode == NN_SVM) {
		KDRV_AI_SVM_PARM* p_parm = (KDRV_AI_SVM_PARM*)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		nvt_dbg(IND, "sv        = 0x%08X\r\n", p_parm->svm_ker.sv_addr);
		nvt_dbg(IND, "alpha     = 0x%08X\r\n", p_parm->svm_ker.alpha_addr);
		nvt_dbg(IND, "fcd       = 0x%08X\r\n", p_parm->fcd.quanti_kmeans_addr);
		#endif
		p_parm->in_addr  = sdk_addr_update(p_parm->in_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr = sdk_addr_update(p_parm->out_addr, io_base, SDK_TEST_IO_BASE);
		p_parm->svm_ker.sv_addr        = sdk_addr_update(p_parm->svm_ker.sv_addr,        model_base, SDK_TEST_MODEL_BASE);
		p_parm->svm_ker.alpha_addr     = sdk_addr_update(p_parm->svm_ker.alpha_addr,     model_base, SDK_TEST_MODEL_BASE);
		p_parm->fcd.quanti_kmeans_addr = sdk_addr_update(p_parm->fcd.quanti_kmeans_addr, model_base, SDK_TEST_MODEL_BASE);

		parm_ofs = mode_ctrl->addr;
		p_parm->fcd.p_vlc_code     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code  + parm_ofs);
		p_parm->fcd.p_vlc_valid    = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
		p_parm->fcd.p_vlc_ofs      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs   + parm_ofs);
	} else if (mode_ctrl->mode == NN_FC) {
		KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		nvt_dbg(IND, "in_inter  = 0x%08X\r\n", p_parm->in_interm_addr);
		nvt_dbg(IND, "out_inter = 0x%08X\r\n", p_parm->out_interm_addr);
		nvt_dbg(IND, "weight    = 0x%08X\r\n", p_parm->fc_ker.weight_addr);
		nvt_dbg(IND, "bias      = 0x%08X\r\n", p_parm->fc_ker.bias_addr);
		nvt_dbg(IND, "kmeans    = 0x%08X\r\n", p_parm->fcd.quanti_kmeans_addr);
		nvt_dbg(IND, "in scl/shift = %d %d\r\n", p_parm->fc_ker.sclshf.in_scale, p_parm->fc_ker.sclshf.in_shift);
		nvt_dbg(IND, "rho fmt      = %d\r\n", p_parm->fc_ker.norm_shf);
		#endif
		p_parm->in_addr 		= sdk_addr_update(p_parm->in_addr, 		   io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr 		= sdk_addr_update(p_parm->out_addr, 	   io_base, SDK_TEST_IO_BASE);
		p_parm->in_interm_addr  = sdk_addr_update(p_parm->in_interm_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_interm_addr = sdk_addr_update(p_parm->out_interm_addr, io_base, SDK_TEST_IO_BASE);
		p_parm->fc_ker.weight_addr     = sdk_addr_update(p_parm->fc_ker.weight_addr,     model_base, SDK_TEST_MODEL_BASE);
		p_parm->fc_ker.bias_addr       = sdk_addr_update(p_parm->fc_ker.bias_addr,       model_base, SDK_TEST_MODEL_BASE);
		p_parm->fcd.quanti_kmeans_addr = sdk_addr_update(p_parm->fcd.quanti_kmeans_addr, model_base, SDK_TEST_MODEL_BASE);

		parm_ofs = mode_ctrl->addr;
		p_parm->fcd.p_vlc_code     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
		p_parm->fcd.p_vlc_valid    = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
		p_parm->fcd.p_vlc_ofs      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
	} else if (mode_ctrl->mode == NN_RESHAPE) {
		KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		#endif
		p_parm->in_addr 		= sdk_addr_update(p_parm->in_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr 		= sdk_addr_update(p_parm->out_addr, io_base, SDK_TEST_IO_BASE);
	} else if (mode_ctrl->mode == NN_REORGANIZATION) {
		KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		#endif
		p_parm->in_addr 		= sdk_addr_update(p_parm->in_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr 		= sdk_addr_update(p_parm->out_addr, io_base, SDK_TEST_IO_BASE);
	} else if (mode_ctrl->mode == NN_ANCHOR) {
		KDRV_AI_ANCHOR_PARM *p_parm = (KDRV_AI_ANCHOR_PARM *)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		nvt_dbg(IND, "w         = 0x%08X\r\n", p_parm->w_addr );
		nvt_dbg(IND, "b         = 0x%08X\r\n", p_parm->b_addr);
		nvt_dbg(IND, "tbl       = 0x%08X\r\n", p_parm->tbl_addr);
		#endif
		p_parm->in_addr 		= sdk_addr_update(p_parm->in_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr 		= sdk_addr_update(p_parm->out_addr, io_base, SDK_TEST_IO_BASE);
		p_parm->w_addr          = sdk_addr_update(p_parm->w_addr,     model_base, SDK_TEST_MODEL_BASE);
		p_parm->b_addr          = sdk_addr_update(p_parm->b_addr,     model_base, SDK_TEST_MODEL_BASE);
		p_parm->tbl_addr        = sdk_addr_update(p_parm->tbl_addr,   model_base, SDK_TEST_MODEL_BASE);
	} else if (mode_ctrl->mode == NN_SOFTMAX) {
		KDRV_AI_SOFTMAX_PARM *p_parm = (KDRV_AI_SOFTMAX_PARM *)addr;
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		#endif
		p_parm->in_addr 		= sdk_addr_update(p_parm->in_addr,  io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr 		= sdk_addr_update(p_parm->out_addr, io_base, SDK_TEST_IO_BASE);
	} else if (mode_ctrl->mode == NN_PREPROC) {
		KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)addr;
		UINT32 preproc_idx = 0, preproc_sub_en = 0;
		for (preproc_idx = 0; preproc_idx < KDRV_AI_PREPROC_FUNC_CNT; preproc_idx++) {
			if (p_parm->func_list[preproc_idx] == KDRV_AI_PREPROC_SUB_EN) {
				preproc_sub_en = 1;
				break;
			}
		}
		#if SDK_DUMP_INFO
		nvt_dbg(IND, "in0       = 0x%08X\r\n", p_parm->in_addr[0]);
		nvt_dbg(IND, "in1       = 0x%08X\r\n", p_parm->in_addr[1]);
		nvt_dbg(IND, "in2       = 0x%08X\r\n", p_parm->in_addr[2]);
		nvt_dbg(IND, "out0      = 0x%08X\r\n", p_parm->out_addr[0]);
		nvt_dbg(IND, "out1      = 0x%08X\r\n", p_parm->out_addr[1]);
		nvt_dbg(IND, "out2      = 0x%08X\r\n", p_parm->out_addr[2]);
		#endif

		//TODO by NUE2
#if 0
		p_parm->flow_ct.rand_ch_en = 0;
		p_parm->flow_ct.interrupt_en = 1;
		p_parm->flow_ct.s_num = 1;
		p_parm->flow_ct.rst_en = 0;
		p_parm->flow_ct.ll_buf = 0;
		p_parm->flow_ct.is_terminate = 0;
		p_parm->flow_ct.is_dma_test = 0;
    	p_parm->flow_ct.is_fill_reg_only = 0;
    	p_parm->flow_ct.is_ll_next_update = 0;
#endif

		p_parm->in_addr[0] = sdk_addr_update(p_parm->in_addr[0], io_base, SDK_TEST_IO_BASE);
		p_parm->in_addr[1] = sdk_addr_update(p_parm->in_addr[1], io_base, SDK_TEST_IO_BASE);
		if (preproc_sub_en) {
			p_parm->in_addr[2] = sdk_addr_update(p_parm->in_addr[2], model_base, SDK_TEST_MODEL_BASE);
		} else {
			p_parm->in_addr[2] = sdk_addr_update(p_parm->in_addr[2], io_base, SDK_TEST_IO_BASE);
		}

		p_parm->out_addr[0] = sdk_addr_update(p_parm->out_addr[0], io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr[1] = sdk_addr_update(p_parm->out_addr[1], io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr[2] = sdk_addr_update(p_parm->out_addr[2], io_base, SDK_TEST_IO_BASE);


		if (is_first_layer) {
			//NN_IOMEM *out_info = (NN_IOMEM*)(mem_base + ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD)) + ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * ctrl_num));
			UINT32 out_end_addr = 0;

#if 0
			UINT32 out_start_addr = 0xFFFFFFFF;
			UINT32 tmp_idx = 0;
			for (tmp_idx = 0; tmp_idx < NN_OMEM_NUM; tmp_idx++) {
				if (out_info->SAO[tmp_idx].size > 0 && out_info->SAO[tmp_idx].va > 0) {
					if (out_start_addr > out_info->SAO[tmp_idx].va) {
						out_start_addr = out_info->SAO[tmp_idx].va;
					}
					if (out_info->SAO[tmp_idx].va + out_info->SAO[tmp_idx].size > out_end_addr) {
						out_end_addr = out_info->SAO[tmp_idx].va + out_info->SAO[tmp_idx].size;
					}
				}
			}
#endif

			p_parm->in_addr[0] = sdk_addr_update(out_end_addr, io_base, SDK_TEST_IO_BASE);
			p_parm->in_addr[1] = sdk_addr_update(out_end_addr + 192512, io_base, SDK_TEST_IO_BASE);
			p_parm->in_addr[2] = 0;


			// read input
			#if SDK_DUMP_INFO
			nvt_dbg(ERR, "start load data\n");
			#endif
			if (p_parm->in_addr[0] != 0) {
				#if SDK_DUMP_INFO
				nvt_dbg(ERR, "load data 0\n");
				#endif
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\sdk\\1\\YVU420_SP_Y.bin");
#else
				snprintf(io_path, 64, "//mnt//sd//sdk//1//YVU420_SP_Y.bin");
#endif
				len = load_data(io_path, p_parm->in_addr[0], mem_size);
				if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
				#if SDK_DUMP_INFO
				nvt_dbg(ERR, "load data 0 done\n");
				#endif
			}
			if (p_parm->in_addr[1] != 0) {
				#if SDK_DUMP_INFO
				nvt_dbg(ERR, "load data 1\n");
				#endif
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\sdk\\1\\YVU420_SP_UV.bin");
#else
				snprintf(io_path, 64, "//mnt//sd//sdk//1//YVU420_SP_UV.bin");
#endif
				len = load_data(io_path, p_parm->in_addr[1], mem_size);
				if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
				#if SDK_DUMP_INFO
				nvt_dbg(ERR, "load data 1 done\n");
				#endif
			}
			#if SDK_DUMP_INFO
			nvt_dbg(ERR, "load data done\n");
			#endif
		}

	} else {
		nvt_dbg(IND, "unsupport mode!\r\n");
		return 1;
	}
	return 0;
}

static unsigned int sdk_update_ll_ioaddr(unsigned int is_first_layer, unsigned int addr, unsigned int io_base, unsigned int model_base,
									unsigned int mem_base, unsigned int mem_size, unsigned int ctrl_num, NN_GEN_MODE_CTRL* mode_ctrl, unsigned int layer_num)
{
	UINT64* cmd_list = NULL;
	UINT32 i = 0;
	UINT32 tmp_addr = 0;
	UINT32 tmp_ofs = 0;
	UINT32 tmp_layer_num = 0;
	#if SDK_DUMP_INFO
	nvt_dbg(IND, "cmd addr: 0x%08X, test layer num = %d\r\n", addr, layer_num);
	#endif
	cmd_list = (UINT64*)addr;
	if (mode_ctrl->mode == NN_CONV || mode_ctrl->mode == NN_DECONV || mode_ctrl->mode == NN_ELTWISE
		|| mode_ctrl->mode == NN_POOL || mode_ctrl->mode == NN_BNSCALE || mode_ctrl->mode == NN_SCALEUP) {
		i = 0;
		while (1) {
			#if SDK_DUMP_INFO
			nvt_dbg(IND, "cmd offset %02d: 0x%08X %08X\r\n", i, (UINT32)(cmd_list[i] >> 32),(UINT32)(cmd_list[i] & 0xFFFFFFFF));
			#endif
			if ((cmd_list[i] >> 60) == 8) {
				tmp_ofs = (UINT32)((cmd_list[i] >> 32) & 0xFFF);
				if (tmp_ofs == 0x10 || tmp_ofs == 0x3C || tmp_ofs == 0x44 || tmp_ofs == 0x48) {
					// io addr
					tmp_addr = (UINT32)(cmd_list[i] & 0xFFFFFFFF);
					tmp_addr = (tmp_addr > 0)?ai_platform_va2pa(sdk_addr_update(tmp_addr, io_base, SDK_TEST_IO_BASE)):0;
					cmd_list[i] = (((cmd_list[i] >> 32) << 32) | (UINT64)tmp_addr);
				} else if (tmp_ofs == 0x30 || tmp_ofs == 0x34 || tmp_ofs == 0x38) {
					// model addr
					tmp_addr = (UINT32)(cmd_list[i] & 0xFFFFFFFF);
					if (tmp_addr > 0) {
						tmp_addr = ai_platform_va2pa(sdk_addr_update(tmp_addr, model_base, SDK_TEST_MODEL_BASE));
					}
					cmd_list[i] = (((cmd_list[i] >> 32) << 32) | (UINT64)tmp_addr);
				}
				i++;
			} else if ((cmd_list[i] >> 60) == 4) {
				cmd_list[i] = cmd_list[i-1];
				i++;
			} else if ((cmd_list[i] >> 60) == 2) {
				tmp_layer_num++;
				if (tmp_layer_num >= layer_num) {
					cmd_list[i] = cmd_list[i] & 0xFF;
					break;
				} else {
					tmp_addr = (UINT32)((cmd_list[i] >> 8) & 0xFFFFFFFF) + mem_base;
					cmd_list[i] = 0 | (cmd_list[i] & 0xFF) | ((UINT64)2 << 60) | ((UINT64)(ai_platform_va2pa(tmp_addr)) << 8);
					cmd_list = (UINT64*)(tmp_addr);
					i = 0;
				}
			} else if ((cmd_list[i] >> 60) == 0) {
				break;
			}
		}
		/*nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out0      = 0x%08X\r\n", p_parm->out0_addr);
		nvt_dbg(IND, "out1      = 0x%08X\r\n", p_parm->out1_addr);
		nvt_dbg(IND, "in_inter  = 0x%08X\r\n", p_parm->in_interm_addr);
		nvt_dbg(IND, "out_inter = 0x%08X\r\n", p_parm->out_interm_addr);
		nvt_dbg(IND, "tmp_buf   = 0x%08X\r\n", p_parm->tmp_buf_addr);
		nvt_dbg(IND, "weight    = 0x%08X\r\n", p_parm->conv.weight_addr);
		nvt_dbg(IND, "bias      = 0x%08X\r\n", p_parm->conv.bias_addr);
		nvt_dbg(IND, "fcd       = 0x%08X\r\n", p_parm->conv.fcd.quanti_kmeans_addr);
		nvt_dbg(IND, "bn        = 0x%08X\r\n", p_parm->norm.bn_scl.bn_scale_addr);
		nvt_dbg(IND, "elt       = 0x%08X\r\n", p_parm->elt.addr);*/

	} else if (mode_ctrl->mode == NN_ROIPOOLING || mode_ctrl->mode == NN_SVM || mode_ctrl->mode == NN_FC
			|| mode_ctrl->mode == NN_RESHAPE    || mode_ctrl->mode == NN_REORGANIZATION
			|| mode_ctrl->mode == NN_ANCHOR     || mode_ctrl->mode == NN_SOFTMAX) {
		i = 0;
		while (1) {
			#if SDK_DUMP_INFO
			nvt_dbg(IND, "cmd offset %02d: 0x%08X %08X\r\n", i, (UINT32)(cmd_list[i] >> 32),(UINT32)(cmd_list[i] & 0xFFFFFFFF));
			#endif
			if ((cmd_list[i] >> 60) == 8) {
				tmp_ofs = (UINT32)((cmd_list[i] >> 32) & 0xFFF);
				if (tmp_ofs == 0x0c || tmp_ofs == 0x10 || tmp_ofs == 0x2c) {
					/* io addr */
					tmp_addr = (UINT32)(cmd_list[i] & 0xFFFFFFFF);
					if (tmp_addr > 0) {
						tmp_addr = ai_platform_va2pa(sdk_addr_update(tmp_addr, io_base, SDK_TEST_IO_BASE));
					}
					cmd_list[i] = (((cmd_list[i] >> 32) << 32) | (UINT64)tmp_addr);
				} else if (tmp_ofs == 0x14 || tmp_ofs == 0x18 || tmp_ofs == 0x1c
						|| tmp_ofs == 0x20 || tmp_ofs == 0x28) {
					/* model addr */
					tmp_addr = (UINT32)(cmd_list[i] & 0xFFFFFFFF);
					if (tmp_addr > 0) {
						tmp_addr = ai_platform_va2pa(sdk_addr_update(tmp_addr, model_base, SDK_TEST_MODEL_BASE));
					}
					cmd_list[i] = (((cmd_list[i] >> 32) << 32) | (UINT64)tmp_addr);
				}
				i++;
			} else if ((cmd_list[i] >> 60) == 4) {
				cmd_list[i] = cmd_list[i-1];
				i++;
			} else if ((cmd_list[i] >> 60) == 2) {
				tmp_addr = (UINT32)((cmd_list[i] >> 8) & 0xFFFFFFFF) + mem_base;
				cmd_list[i] = 0 | (cmd_list[i] & 0xFF) | ((UINT64)2 << 60) | ((UINT64)(ai_platform_va2pa(tmp_addr)) << 8);
				cmd_list = (UINT64*)(tmp_addr);
				i = 0;
			} else if ((cmd_list[i] >> 60) == 0) {
				break;
			}
		}

		/*nvt_dbg(IND, "in        = 0x%08X\r\n", p_parm->in_addr);
		nvt_dbg(IND, "out       = 0x%08X\r\n", p_parm->out_addr);
		nvt_dbg(IND, "roi       = 0x%08X\r\n", p_parm->roi_ker.roi_addr);

		nvt_dbg(IND, "sv        = 0x%08X\r\n", p_parm->svm_ker.sv_addr);
		nvt_dbg(IND, "alpha     = 0x%08X\r\n", p_parm->svm_ker.alpha_addr);
		nvt_dbg(IND, "fcd       = 0x%08X\r\n", p_parm->fcd.quanti_kmeans_addr);

		nvt_dbg(IND, "sv        = 0x%08X\r\n", p_parm->svm_ker.sv_addr);
		nvt_dbg(IND, "alpha     = 0x%08X\r\n", p_parm->svm_ker.alpha_addr);
		nvt_dbg(IND, "fcd       = 0x%08X\r\n", p_parm->fcd.quanti_kmeans_addr);

		nvt_dbg(IND, "in_inter  = 0x%08X\r\n", p_parm->in_interm_addr);
		nvt_dbg(IND, "out_inter = 0x%08X\r\n", p_parm->out_interm_addr);
		nvt_dbg(IND, "weight    = 0x%08X\r\n", p_parm->fc_ker.weight_addr);
		nvt_dbg(IND, "bias      = 0x%08X\r\n", p_parm->fc_ker.bias_addr);
		nvt_dbg(IND, "kmeans    = 0x%08X\r\n", p_parm->fcd.quanti_kmeans_addr);

		nvt_dbg(IND, "w         = 0x%08X\r\n", p_parm->w_addr );
		nvt_dbg(IND, "b         = 0x%08X\r\n", p_parm->b_addr);
		nvt_dbg(IND, "tbl       = 0x%08X\r\n", p_parm->tbl_addr);*/

	}  else if (mode_ctrl->mode == NN_PREPROC) {
		i = 0;
		while (1) {
			//nvt_dbg(IND, "cmd offset %02d: 0x%08X %08X\r\n", i, (UINT32)(cmd_list[i] >> 32),(UINT32)(cmd_list[i] & 0xFFFFFFFF));
			if ((cmd_list[i] >> 60) == 8) {
				tmp_ofs = (UINT32)((cmd_list[i] >> 32) & 0xFFF);
				if (tmp_ofs == 0x08 || tmp_ofs == 0x0C || tmp_ofs == 0x10 ||
				    tmp_ofs == 0x18 || tmp_ofs == 0x1c || tmp_ofs == 0x20) {
					// io addr
					tmp_addr = (UINT32)(cmd_list[i] & 0xFFFFFFFF);
					if (tmp_addr > 0)
						tmp_addr = ai_platform_va2pa(sdk_addr_update(tmp_addr, io_base, SDK_TEST_IO_BASE));
					cmd_list[i] = (((cmd_list[i] >> 32) << 32) | (UINT64)tmp_addr);
				} else if (0) {
					// model addr
					tmp_addr = (UINT32)(cmd_list[i] & 0xFFFFFFFF);
					if (tmp_addr > 0)
						tmp_addr = ai_platform_va2pa(sdk_addr_update(tmp_addr, model_base, SDK_TEST_MODEL_BASE));
					cmd_list[i] = (((cmd_list[i] >> 32) << 32) | (UINT64)tmp_addr);
				}
				i++;
			} else if ((cmd_list[i] >> 60) == 4) {
				cmd_list[i] = cmd_list[i-1];
				i++;
			} else if ((cmd_list[i] >> 60) == 2) {
				tmp_addr = (UINT32)((cmd_list[i] >> 8) & 0xFFFFFFFF) + mem_base;
				cmd_list[i] = 0 | (cmd_list[i] & 0xFF) | ((UINT64)2 << 60) | ((UINT64)(ai_platform_va2pa(tmp_addr)) << 8);
				cmd_list = (UINT64*)(tmp_addr);
				i = 0;
			} else if ((cmd_list[i] >> 60) == 0) {
				break;
			}
		}
		/*KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)addr;
		UINT32 preproc_idx = 0, preproc_sub_en = 0;
		for (preproc_idx = 0; preproc_idx < KDRV_AI_PREPROC_FUNC_CNT; preproc_idx++) {
			if (p_parm->func_list[preproc_idx] == KDRV_AI_PREPROC_SUB_EN) {
				preproc_sub_en = 1;
				break;
			}
		}
		nvt_dbg(IND, "in0       = 0x%08X\r\n", p_parm->in_addr[0]);
		nvt_dbg(IND, "in1       = 0x%08X\r\n", p_parm->in_addr[1]);
		nvt_dbg(IND, "in2       = 0x%08X\r\n", p_parm->in_addr[2]);
		nvt_dbg(IND, "out0      = 0x%08X\r\n", p_parm->out_addr[0]);
		nvt_dbg(IND, "out1      = 0x%08X\r\n", p_parm->out_addr[1]);
		nvt_dbg(IND, "out2      = 0x%08X\r\n", p_parm->out_addr[2]);


		p_parm->in_addr[0] = sdk_addr_update(p_parm->in_addr[0], io_base, SDK_TEST_IO_BASE);
		p_parm->in_addr[1] = sdk_addr_update(p_parm->in_addr[1], io_base, SDK_TEST_IO_BASE);
		if (preproc_sub_en)
			p_parm->in_addr[2] = sdk_addr_update(p_parm->in_addr[2], model_base, SDK_TEST_MODEL_BASE);
		else
			p_parm->in_addr[2] = sdk_addr_update(p_parm->in_addr[2], io_base, SDK_TEST_IO_BASE);

		p_parm->out_addr[0] = sdk_addr_update(p_parm->out_addr[0], io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr[1] = sdk_addr_update(p_parm->out_addr[1], io_base, SDK_TEST_IO_BASE);
		p_parm->out_addr[2] = sdk_addr_update(p_parm->out_addr[2], io_base, SDK_TEST_IO_BASE);


		if (is_first_layer) {
			NN_IOMEM *out_info = (NN_IOMEM*)(mem_base + ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD)) + ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * ctrl_num));
			UINT32 out_start_addr = 0xFFFFFFFF;
			UINT32 out_end_addr = 0;
			UINT32 tmp_idx = 0;

			for (tmp_idx = 0; tmp_idx < NN_OMEM_NUM; tmp_idx++) {
				if (out_info->SAO[tmp_idx].size > 0 && out_info->SAO[tmp_idx].va > 0) {
					if (out_start_addr > out_info->SAO[tmp_idx].va)
						out_start_addr = out_info->SAO[tmp_idx].va;
					if (out_info->SAO[tmp_idx].va + out_info->SAO[tmp_idx].size > out_end_addr)
						out_end_addr = out_info->SAO[tmp_idx].va + out_info->SAO[tmp_idx].size;
				}
			}

			p_parm->in_addr[0] = sdk_addr_update(out_end_addr, io_base, SDK_TEST_IO_BASE);
			p_parm->in_addr[1] = sdk_addr_update(out_end_addr + 192512, io_base, SDK_TEST_IO_BASE);
			p_parm->in_addr[2] = 0;


			// read input
			nvt_dbg(ERR, "start load data\n");
			if (p_parm->in_addr[0] != 0) {
				nvt_dbg(ERR, "load data 0\n");
				snprintf(io_path, 64, "//mnt//sd//sdk//1//YVU420_SP_Y.bin");
				len = load_data(io_path, p_parm->in_addr[0], mem_size);
				if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
				nvt_dbg(ERR, "load data 0 done\n");
			}
			if (p_parm->in_addr[1] != 0) {
				nvt_dbg(ERR, "load data 1\n");
				snprintf(io_path, 64, "//mnt//sd//sdk//1//YVU420_SP_UV.bin");
				len = load_data(io_path, p_parm->in_addr[1], mem_size);
				if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
				nvt_dbg(ERR, "load data 1 done\n");
			}
			nvt_dbg(ERR, "load data done\n");
		}*/

	} else {
		nvt_dbg(IND, "unsupport mode!\r\n");
		return 1;
	}

	return 0;
}

static unsigned int emu_set_nue_ll_addr(NT98520_NUE_REG_STRUCT* p_nue_reg, unsigned int *ll_addr, unsigned int ll_idx, unsigned int is_end)
{
	unsigned int i = 0;
	unsigned int cmd_num = 0;
	unsigned int* reg_table = (unsigned int*)p_nue_reg;
	UINT32 tmp_ll_addr = *ll_addr;
	UINT64 *pLL = (UINT64*)tmp_ll_addr;

	for(i = 0; i < 0x1c4/4 + 2; i++){
		if (i == 0) {
			//pass
			continue;
		}
		else if (i == 14 || i == 15) {
			pLL[cmd_num] = nue_ll_upd_cmd(0xF, 4*i, 0xffffffff);
			cmd_num++;
		}
		else if (i > 113) {
			if (is_end) {
				//if (nue_ll_loop_en)
				//	pLL[cmd_num] = nue_ll_nextll_cmd(dma_getPhyAddr((UINT32)(&pLL[0])), ll_idx);
				//else
					pLL[cmd_num] = nue_ll_null_cmd((UINT8)ll_idx);
				cmd_num++;
			}
			else {
				pLL[cmd_num] = nue_ll_nextll_cmd(ai_platform_va2pa((UINT32)(&pLL[cmd_num+1])), (UINT8)ll_idx);
				cmd_num++;
			}
		}
		else {
			if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 0) { //svm
				if ((i >= 8 && i <= 10) || (i >= 12 && i <= 13) || (i >= 23 && i <= 59) || (i >= 97 && i <= 100)
					|| (i >= 103))
					continue;
				//if ((CNN_ENGINE_HOUSE[1] & 0xf0000) == 0 && (i >= 54 && i <= 88))
				//	continue;
			}
			else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 1) { //roi pooling
				if ((i >= 4 && i <= 7) || (i >= 9 && i <= 10) || (i == 12) || (i >= 19 && i <= 22)
					 || (i >= 24 && i <= 59) || (i >= 61 && i <= 100) || (i >= 103))
					continue;

			}
			else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 3) { //reorg
				if ((i == 2) || (i >= 4 && i <= 10) || (i >= 12 && i <= 13) || (i >= 19 && i <= 23)
					 || (i >= 25 && i <= 59) || (i >= 61 && i <= 100) || (i >= 103))
					continue;

			}
			else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 4) { //permute
				if ((i >= 4 && i <= 10) || (i >= 12 && i <= 13) || (i >= 19 && i <= 59) || (i >= 61 && i <= 100)
					|| (i >= 103 && i <= 112))
					continue;

			}
			else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 5) { //anchor
				if ((i == 2)  || (i == 4) || (i >= 8 && i <= 10) || (i >= 12 && i <= 13) || (i >= 19 && i <= 57)
					|| (i == 59) || (i >= 61 && i <= 100) || (i >= 103))
					continue;

			}
			else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 6) { //softmax
				if ((i == 2) || (i >= 4 && i <= 10) || (i >= 12 && i <= 13) || (i >= 19 && i <= 58)
					 || (i >= 61 && i <= 100) || (i >= 103))
					continue;

			}
			else {
				// reserved
			}

			pLL[cmd_num] = nue_ll_upd_cmd(0xF, 4*i, reg_table[i]);
			cmd_num++;
		}

		//SET_32BitsValue((_CNN_REG_BASE_ADDR + i*4), CNN_ENGINE_HOUSE[i]);
	}

	*ll_addr = *ll_addr + 4*2*cmd_num;

	return 0;
}



//unsigned int NUE_ENGINE_HOUSE[0x1cc / 4 + 1] = {0x0};
//NUE_PARM nue_param = {0};
#define VERIFY_WORDALIGN 1
#define CACHE_DBG 0
#define LL_ADDR_SIZE (0x1c4*2*48)
unsigned int NUE_ENGINE_HOUSE[0x1cc / 4 + 1] = {0x0};
NUE_PARM nue_param = {0};
unsigned int emu_nue(unsigned int index, unsigned int index_start, unsigned int index_end, unsigned int mem_base,
					unsigned int mem_size, unsigned int mem_end, unsigned int ll_en, unsigned int* ll_out_addr, unsigned int* ll_out_size)
{
#if defined(__FREERTOS)
	FST_FILE file;
	unsigned int size;
	unsigned long tmp_long;
	INT32 fstatus;
#else
	//mm_segment_t old_fs;
	//struct file *fp;
	int fd;
#endif
	char io_path[100];
	char tmp[100];
	int len = 0;
#if 0
	unsigned int tmp_val0 = 0, tmp_val1 = 0, i = 0;
#else
	unsigned int tmp_val1 = 0, i = 0;
#endif
	unsigned int mem_tmp_addr = 0;
	NT98520_NUE_REG_STRUCT *p_nue_reg = NULL;
	unsigned int ll_base = 0, ll_addr = 0;
	/* NUE */
	UINT32 nue_dram_in_addr0 = 0, nue_dram_in_addrSv = 0, nue_dram_in_addrAlpha = 0, nue_dram_in_addrRho = 0;
	UINT32 nue_dram_in_addr1 = 0, nue_dram_in_addrRoi = 0, nue_dram_in_addrKQ = 0;
	UINT32 nue_dram_out_addr = 0, nue_out_size = 0, nue_in_size = 0;
#if CACHE_DBG
	UINT32* cache_tmp_buf = NULL;
	UINT32 cache_i = 0;
#endif

	if (ll_en == 1) {
		ll_base = mem_base;
		ll_addr = ll_base;
		mem_base += LL_ADDR_SIZE;
		mem_tmp_addr = mem_base;
	}
	// NUE process
	//===================================================================================================
	// load register
	nvt_dbg(IND, "===============================================================\n");
	nvt_dbg(IND, "emu func = NUE, pattern id %d\n", index);
#if defined(__FREERTOS)
	snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\nueg%d.dat", index, index);
#else
	snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//nueg%d.dat", index, index);
#endif

#if defined(__FREERTOS)
	file = FileSys_OpenFile(io_path, FST_OPEN_READ);
    if (file == 0) {
        nvt_dbg(ERR, "Invalid file: %s\r\n", io_path);
        return -1;
    }
#else
	/*
	fp = filp_open(io_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", io_path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	*/
	fd = vos_file_open(io_path, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", io_path);
		return -1;
	}
#endif
	// parsing registers
	for (i=0;i<0x1cc/4;i++){
		memset(tmp, 0, 14*sizeof(char));
#if defined(__FREERTOS)
		size = 4*sizeof(char);
		fstatus = FileSys_ReadFile(file, (UINT8 *)tmp, (UINT32 *) &size, 0, NULL);
		if (fstatus != 0) {
			nvt_dbg(ERR, "%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
			return -1;
		}

		size = 8*sizeof(char);
		fstatus = FileSys_ReadFile(file, (UINT8 *)tmp, (UINT32 *) &size, 0, NULL);
		if (fstatus != 0) {
            nvt_dbg(ERR, "%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
            return -1;
        }
		tmp_long = strtoul(tmp, NULL, 16);
		tmp_val1 = (unsigned int) tmp_long;
		size = 2*sizeof(char);
		fstatus = FileSys_ReadFile(file, (UINT8 *)tmp, (UINT32 *) &size, 0, NULL);
		if (fstatus != 0) {
            nvt_dbg(ERR, "%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
            return -1;
        }
#else
		//len = vfs_read(fp, (void*)tmp, 4*sizeof(char), &fp->f_pos);
		len = vos_file_read(fd, (void *)tmp, 4*sizeof(char));
		if (len == 0)
			break;
#if 0
		kstrtouint(tmp, 16, &tmp_val0);
#endif
		//len = vfs_read(fp, (void*)tmp, 8*sizeof(char), &fp->f_pos);
		len = vos_file_read(fd, (void *)tmp, 8*sizeof(char));
		kstrtouint(tmp, 16, &tmp_val1);
		//len = vfs_read(fp, (void*)tmp, 2*sizeof(char), &fp->f_pos);
		len = vos_file_read(fd, (void *)tmp, 2*sizeof(char));
#if 0
		nvt_dbg(IND, "read reg file:ofs=%04X val=%08X\n", tmp_val0, tmp_val1);
#endif
#endif
		NUE_ENGINE_HOUSE[i] = tmp_val1;
	}
#if defined(__FREERTOS)
	FileSys_CloseFile(file);
#else
	//filp_close(fp, NULL);
	//set_fs(old_fs);
	vos_file_close(fd);
#endif
	p_nue_reg = (NT98520_NUE_REG_STRUCT*)((unsigned int)NUE_ENGINE_HOUSE);
	//===================================================================================================
	// load inputs
	nue_dram_in_addr0 = 0; nue_dram_in_addrSv = 0; nue_dram_in_addrAlpha = 0; nue_dram_in_addrRho = 0;
	nue_dram_in_addr1 = 0, nue_dram_in_addrRoi = 0, nue_dram_in_addrKQ = 0;   nue_dram_out_addr = 0;
	nue_in_size = 0; nue_out_size = 0;
	if (ll_en == 0)
		mem_tmp_addr = mem_base;

	if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 0) {
		//SVM
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DI0_%d.bin", index, index);
#else
		snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DI0_%d.bin", index, index);
#endif
		#if VERIFY_WORDALIGN
		if ((mem_tmp_addr & 0x3) > 0)
			mem_tmp_addr += (4 - (mem_tmp_addr%4));
		#endif
		nue_in_size = p_nue_reg->NUE_Register_0044.bit.NUE_SVM_INSIZE * p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL
					* p_nue_reg->NUE_Register_0048.bit.OBJ_NUM * (p_nue_reg->NUE_Register_0004.bit.IN_BIT_DEPTH+1);
		if (mem_tmp_addr + nue_in_size > mem_end){
			nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
			return -1;
		}
		len = load_data(io_path, mem_tmp_addr, mem_size);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return -1;
		} else {
			nue_dram_in_addr0 = mem_tmp_addr;
			mem_tmp_addr += len;
		}

#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\ENC_%d.bin", index, index);
#else
		snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//ENC_%d.bin", index, index);
#endif
		#if VERIFY_WORDALIGN
		if ((mem_tmp_addr & 0x3) > 0)
			mem_tmp_addr += (4 - (mem_tmp_addr%4));
		#endif
		nue_in_size = (p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH * p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * 12 + 4) / 8;
		if (mem_tmp_addr + nue_in_size > mem_end){
			nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
			return -1;
		}
		len = load_data(io_path, mem_tmp_addr, mem_size);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return -1;
		} else {
			nue_dram_in_addrSv = mem_tmp_addr;
			mem_tmp_addr += len;
		}

		if (p_nue_reg->NUE_Register_0008.bit.SVM_CAL_EN) {
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DIALPHA%d.bin", index, index);
#else
			snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DIALPHA%d.bin", index, index);
#endif
			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			nue_in_size = (p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * 12 + 4) / 8;
			if (mem_tmp_addr + nue_in_size > mem_end){
				nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
				return -1;
			}
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addrAlpha = mem_tmp_addr;
				mem_tmp_addr += len;
			}
		}
		if (p_nue_reg->NUE_Register_0008.bit.SVM_RESULT_MODE > 0) {
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DIRHO%d.bin", index, index);
#else
			snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DIRHO%d.bin", index, index);
#endif
			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			nue_in_size = (p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * 12 + 4) / 8;
			if (mem_tmp_addr + nue_in_size > mem_end){
				nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
				return -1;
			}
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addrRho = mem_tmp_addr;
				mem_tmp_addr += len;
			}
		}
		if (p_nue_reg->NUE_Register_0008.bit.SVM_INTERMEDIATE_IN_EN) {
			if (p_nue_reg->NUE_Register_0008.bit.SVM_SCIENTIFIC_NOTATION_EN) {
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DIIMD_Sci%d.bin", index, index);
#else
                snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DIIMD_Sci%d.bin", index, index);
#endif
			} else {
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DIIMD%d.bin", index, index);
#else
				snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DIIMD%d.bin", index, index);
#endif
			}

			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			nue_in_size = p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * p_nue_reg->NUE_Register_0048.bit.OBJ_NUM
							* 2 * ((p_nue_reg->NUE_Register_0008.bit.SVM_SCIENTIFIC_NOTATION_EN)?1:2);
			if (mem_tmp_addr + nue_in_size > mem_end){
				nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
				return -1;
			}
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addr1 = mem_tmp_addr;
				mem_tmp_addr += len;
			}
		}
		if (p_nue_reg->NUE_Register_0008.bit.FCD_VLC_EN) {
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\TKMEAN_%d.bin", index, index);
#else
			snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//TKMEAN_%d.bin", index, index);
#endif
			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addrKQ = mem_tmp_addr;
				mem_tmp_addr += len;
			}
		}
	} else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 1) {
		//ROI Pooling
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DI0_%d.bin", index, index);
#else
		snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DI0_%d.bin", index, index);
#endif
		#if VERIFY_WORDALIGN
		if ((mem_tmp_addr & 0x3) > 0)
			mem_tmp_addr += (4 - (mem_tmp_addr%4));
		#endif
		nue_in_size = p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH * p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL
					* p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * (p_nue_reg->NUE_Register_0004.bit.IN_BIT_DEPTH+1);
		if (mem_tmp_addr + nue_in_size > mem_end){
			nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
			return -1;
		}
		len = load_data(io_path, mem_tmp_addr, mem_size);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return -1;
		} else {
			nue_dram_in_addr0 = mem_tmp_addr;
			mem_tmp_addr += len;
		}
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DIROI%d.bin", index, index);
#else
		snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DIROI%d.bin", index, index);
#endif
		#if VERIFY_WORDALIGN
		if ((mem_tmp_addr & 0x3) > 0)
			mem_tmp_addr += (4 - (mem_tmp_addr%4));
		#endif
		//nue_in_size = p_nue_reg->NUE_Register_0048.bit.ROI_NUM * 4 * sizeof(INT16) + 8;
		if (mem_tmp_addr + nue_in_size > mem_end){
			nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
			return -1;
		}
		len = load_data(io_path, mem_tmp_addr, mem_size);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return -1;
		} else {
			nue_dram_in_addrRoi = mem_tmp_addr;
			mem_tmp_addr += len;
		}
	} else if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 3 || p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 4
			|| p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 5 || p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 6) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DI0_%d.bin", index, index);
#else
		snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DI0_%d.bin", index, index);
#endif
		#if VERIFY_WORDALIGN
		if ((mem_tmp_addr & 0x3) > 0)
			mem_tmp_addr += (4 - (mem_tmp_addr%4));
		#endif
		nue_in_size = p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH * p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL
					* p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * (p_nue_reg->NUE_Register_0004.bit.IN_BIT_DEPTH+1);
		if (mem_tmp_addr + nue_in_size > mem_end){
			nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern! in0 size = 0x%08X\n", nue_in_size);
			return -1;
		}
		len = load_data(io_path, mem_tmp_addr, mem_size);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return -1;
		} else {
			nue_dram_in_addr0 = mem_tmp_addr;
			mem_tmp_addr += len;
		}

		if (p_nue_reg->NUE_Register_0004.bit.NUE_MODE == 5) {
			//Anchor transform
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DI_Weight%d.bin", index, index);
#else
			snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DI_Weight%d.bin", index, index);
#endif
			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			nue_in_size = p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH * p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT * 12; // 12 bits per pixel
			if (nue_in_size % 32 > 0)
				nue_in_size += (32 - (nue_in_size % 32));
			nue_in_size = nue_in_size * p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL / 16;
			if (mem_tmp_addr + nue_in_size > mem_end){
				nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern! weight size = 0x%08X\n", nue_in_size);
				return -1;
			}
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addrSv = mem_tmp_addr;
				mem_tmp_addr += len;
			}
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DI_Bias%d.bin", index, index);
#else
			snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DI_Bias%d.bin", index, index);
#endif
			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			if (mem_tmp_addr + nue_in_size > mem_end){
				nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern! bias size = 0x%08X\n", nue_in_size);
				return -1;
			}
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addrRho = mem_tmp_addr;
				mem_tmp_addr += len;
			}
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUEP\\nueg%d\\DI_EXP%d.bin", index, index);
#else
			snprintf(io_path, 64, "//mnt//sd//NUEP//nueg%d//DI_EXP%d.bin", index, index);
#endif
			#if VERIFY_WORDALIGN
			if ((mem_tmp_addr & 0x3) > 0)
				mem_tmp_addr += (4 - (mem_tmp_addr%4));
			#endif
			nue_in_size = 256*2;
			if (mem_tmp_addr + nue_in_size > mem_end){
				nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern!\n");
				return -1;
			}
			len = load_data(io_path, mem_tmp_addr, mem_size);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			} else {
				nue_dram_in_addrAlpha = mem_tmp_addr;
				mem_tmp_addr += len;
			}
		}
	}
	// set output addr
	#if VERIFY_WORDALIGN
	if ((mem_tmp_addr & 0x3) > 0)
		mem_tmp_addr += (4 - (mem_tmp_addr%4));
	#endif
	nue_dram_out_addr = mem_tmp_addr;
	nue_calc_output_size(p_nue_reg, (unsigned int *) &nue_out_size);
	if (mem_tmp_addr + nue_out_size > mem_end){
		nvt_dbg(ERR, "This pattern cost too many memory! skip this pattern! out size = 0x%08X\n", nue_out_size);
		return -1;
	}
	// clear output
	memset((void*)nue_dram_out_addr, 0x0, nue_out_size);
	if (ll_en == 1) {
		//mem_tmp_addr += nue_out_size; //TODO add by Wayne
		// set buf info
		p_nue_reg->NUE_Register_000c.bit.DRAM_SAI0     = nue_dram_in_addr0;
		p_nue_reg->NUE_Register_0010.bit.DRAM_SAI1     = nue_dram_in_addr1     >> 2;
		p_nue_reg->NUE_Register_0014.bit.DRAM_SAISV    = nue_dram_in_addrSv    >> 2;
		p_nue_reg->NUE_Register_0018.bit.DRAM_SAIALPHA = nue_dram_in_addrAlpha >> 2;
		p_nue_reg->NUE_Register_001c.bit.DRAM_SAIRHO   = nue_dram_in_addrRho   >> 2;
		//p_nue_reg->NUE_Register_0020.bit.DRAM_SAIROI   = nue_dram_in_addrRoi   >> 2;
		p_nue_reg->NUE_Register_0028.bit.DRAM_SAIKQ    = nue_dram_in_addrKQ    >> 2;
		p_nue_reg->NUE_Register_002c.bit.DRAM_SAOR     = nue_dram_out_addr;
		nvt_dbg(IND, "addrin0      = 0x%08X\n", nue_dram_in_addr0);
		nvt_dbg(IND, "addrin1      = 0x%08X\n", nue_dram_in_addr1);
		nvt_dbg(IND, "addrin_sv    = 0x%08X\n", nue_dram_in_addrSv);
		nvt_dbg(IND, "addrin_alpha = 0x%08X\n", nue_dram_in_addrAlpha);
		nvt_dbg(IND, "addrin_rho   = 0x%08X\n", nue_dram_in_addrRho);
		nvt_dbg(IND, "addrin_roi   = 0x%08X\n", nue_dram_in_addrRoi);
		nvt_dbg(IND, "addrin_kq    = 0x%08X\n", nue_dram_in_addrKQ);
		nvt_dbg(IND, "addr_out     = 0x%08X\n\n", nue_dram_out_addr);
		ll_out_addr[index-index_start] = nue_dram_out_addr;
		ll_out_size[index-index_start] = nue_out_size;
		// set register info into ll addr
		emu_set_nue_ll_addr(p_nue_reg, &ll_addr, index-index_start, (index == index_end)?1:0);
	} else {
		//===================================================================================================
		// load register setting
		// set in/out type
		nue_param.intype  = (p_nue_reg->NUE_Register_0004.bit.IN_BIT_DEPTH << 1)  + (1 - p_nue_reg->NUE_Register_0004.bit.IN_SIGNEDNESS);
		nue_param.outtype = (p_nue_reg->NUE_Register_0004.bit.OUT_BIT_DEPTH << 1) + (1 - p_nue_reg->NUE_Register_0004.bit.OUT_SIGNEDNESS);

		// set user define functions
		nue_param.userdef.eng_mode    = p_nue_reg->NUE_Register_0004.bit.NUE_MODE;
		nue_param.userdef.func_en     = ((p_nue_reg->NUE_Register_0008.bit.SVM_KERNEL2_EN) ? NUE_SVM_KER2_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.SVM_CAL_EN) ? NUE_SVM_CAL_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.SVM_INTERLACE_EN) ? NUE_SVM_INTERLACE_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.FCD_VLC_EN) ? NUE_FCD_VLC_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.FCD_QUANTIZATION_EN) ? NUE_FCD_QUANTI_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.FCD_SPARSE_EN) ? NUE_FCD_SPARSE_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.FCD_QUANTIZATION_KMEANS) ? NUE_FCD_QUANTI_KMEANS_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.RELU_EN) ? NUE_RELU_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.SVM_INTERMEDIATE_IN_EN) ? NUE_SVM_INTERMEDIATE_IN_EN : 0) |
									   ((p_nue_reg->NUE_Register_0008.bit.SVM_SCIENTIFIC_NOTATION_EN) ? NUE_SVM_SCI_NOTATION_EN : 0);

		nue_param.userdef.svm_userdef.kerl1type = p_nue_reg->NUE_Register_0008.bit.SVM_KERNEL1_MODE;
		nue_param.userdef.svm_userdef.kerl2type = p_nue_reg->NUE_Register_0008.bit.SVM_KERNEL2_MODE;
		nue_param.userdef.svm_userdef.rslttype  = p_nue_reg->NUE_Register_0008.bit.SVM_RESULT_MODE;
		nue_param.userdef.svm_userdef.dmao_en   = p_nue_reg->NUE_Register_0008.bit.SVM_DMAOEN;
		nue_param.userdef.svm_userdef.dmao_path = p_nue_reg->NUE_Register_0008.bit.SVM_DMAO_PATH;

		// set input scale/shift parameters
		nue_param.in_parm.in_shf   = p_nue_reg->NUE_Register_00f0.bit.IN_SHIFT * ((p_nue_reg->NUE_Register_00f0.bit.IN_SHIFT_DIR)? -1 : 1);
		nue_param.in_parm.in_scale = p_nue_reg->NUE_Register_00f0.bit.IN_SCALE;

		// set clamp parameters
		//nue_param.clamp_parm.clamp_th_0 = p_nue_reg->NUE_Register_0194.bit.CLAMP_THRES0;
		//nue_param.clamp_parm.clamp_th_1 = p_nue_reg->NUE_Register_0198.bit.CLAMP_THRES1;

		// set input size
		nue_param.insize.width   = p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH;
		nue_param.insize.height  = p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT;
		nue_param.insize.channel = p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL;

		// set svm input size
		nue_param.insvm_size.insize  = p_nue_reg->NUE_Register_0044.bit.NUE_SVM_INSIZE;
		nue_param.insvm_size.channel = p_nue_reg->NUE_Register_0044.bit.NUE_CHANNEL;
		nue_param.insvm_size.objnum  = p_nue_reg->NUE_Register_0048.bit.OBJ_NUM;
		nue_param.insvm_size.sv_w    = p_nue_reg->NUE_Register_0040.bit.NUE_WIDTH;
		nue_param.insvm_size.sv_h    = p_nue_reg->NUE_Register_0040.bit.NUE_HEIGHT;

		// set svm parameters
		//nue_param.svm_parm.svmker_type
		nue_param.svm_parm.in_rfh                 = p_nue_reg->NUE_Register_0048.bit.IN_RFH;
		nue_param.svm_parm.svmker_parms.ft_shf    = p_nue_reg->NUE_Register_0050.bit.KER1_FT_SHIFT;
		nue_param.svm_parm.svmker_parms.gv        = p_nue_reg->NUE_Register_004c.bit.KER1_GV;
		nue_param.svm_parm.svmker_parms.gv_shf    = p_nue_reg->NUE_Register_004c.bit.KER1_GV_SHIFT;
		nue_param.svm_parm.svmker_parms.coef      = p_nue_reg->NUE_Register_0050.bit.KER1_COEF;
		nue_param.svm_parm.svmker_parms.degree    = p_nue_reg->NUE_Register_004c.bit.KER2_DEGREE;
		if ((p_nue_reg->NUE_Register_0058.bit.RHO_REG >> 11) == 1)
			nue_param.svm_parm.svmker_parms.rho   = (-1) * (INT32)(((~p_nue_reg->NUE_Register_0058.bit.RHO_REG) & 0x7ff) + 1);
		else
			nue_param.svm_parm.svmker_parms.rho   = p_nue_reg->NUE_Register_0058.bit.RHO_REG;
		nue_param.svm_parm.svmker_parms.rho_fmt   = p_nue_reg->NUE_Register_0058.bit.RHO_FMT + 3;
		nue_param.svm_parm.svmker_parms.alpha_shf = p_nue_reg->NUE_Register_0058.bit.ALPHA_SHIFT;
		nue_param.svm_parm.fcd_parm.fcd_vlc_en    = p_nue_reg->NUE_Register_0008.bit.FCD_VLC_EN;
		nue_param.svm_parm.fcd_parm.fcd_quanti_en       = p_nue_reg->NUE_Register_0008.bit.FCD_QUANTIZATION_EN;
		nue_param.svm_parm.fcd_parm.fcd_sparse_en       = p_nue_reg->NUE_Register_0008.bit.FCD_SPARSE_EN;
		nue_param.svm_parm.fcd_parm.fcd_quanti_kmean_en = p_nue_reg->NUE_Register_0008.bit.FCD_QUANTIZATION_KMEANS;
		nue_param.svm_parm.fcd_parm.fcd_enc_bit_length  = p_nue_reg->NUE_Register_00f4.bit.FCD_ENC_BIT_LENGTH;

		for (i = 0; i < 23; i++) {
			nue_param.svm_parm.fcd_parm.fcd_vlc_code[i]  = (NUE_ENGINE_HOUSE[62+i] >> 8) & 0x7FFFFF;
			nue_param.svm_parm.fcd_parm.fcd_vlc_valid[i] = NUE_ENGINE_HOUSE[62+i] >> 31;
			if (i % 2 == 0) {
				nue_param.svm_parm.fcd_parm.fcd_vlc_ofs[i]   = NUE_ENGINE_HOUSE[85 + i/2] & 0xFFFF;
				if (i < 22)
					nue_param.svm_parm.fcd_parm.fcd_vlc_ofs[i+1] = NUE_ENGINE_HOUSE[85 + i/2] >> 16;
			}
		}
		nue_param.svm_parm.ilofs = p_nue_reg->NUE_Register_0030.bit.DRAM_OFSI << 2;

		// set relu parameters
		//nue_param.relu_parm.relu_type
		if ((p_nue_reg->NUE_Register_0054.bit.RELU_LEAKY_VAL >> 10) == 1)
			nue_param.relu_parm.leaky_val = (-1) * (INT32)(((~p_nue_reg->NUE_Register_0054.bit.RELU_LEAKY_VAL) & 0x3ff) + 1);
		else
			nue_param.relu_parm.leaky_val = p_nue_reg->NUE_Register_0054.bit.RELU_LEAKY_VAL;
		nue_param.relu_parm.leaky_shf = p_nue_reg->NUE_Register_0054.bit.RELU_LEAKY_SHIFT + 7;
		nue_param.relu_parm.out_shf   = p_nue_reg->NUE_Register_0054.bit.RELU_SHIFT * ((p_nue_reg->NUE_Register_0054.bit.RELU_SHIFT_SIGNEDNESS)? -1 : 1);

		// set roipooling parameters
		//nue_param.roipool_parm.roi_num   = p_nue_reg->NUE_Register_0048.bit.ROI_NUM;
		nue_param.roipool_parm.olofs     = p_nue_reg->NUE_Register_0034.bit.DRAM_OFSO << 2;
		//nue_param.roipool_parm.size      = (p_nue_reg->NUE_Register_0008.bit.ROIPOOLING_MODE) ? (3+p_nue_reg->NUE_Register_005c.bit.ROIPOOL_PSROI_WH): p_nue_reg->NUE_Register_005c.bit.ROIPOOL_STANDARD_WH;
		//nue_param.roipool_parm.ratio_mul = p_nue_reg->NUE_Register_005c.bit.ROIPOOL_RATIO_MUL;
		//nue_param.roipool_parm.ratio_shf = p_nue_reg->NUE_Register_005c.bit.ROIPOOL_RATIO_SHIFT;
		//nue_param.roipool_parm.out_shf   = p_nue_reg->NUE_Register_005c.bit.ROIPOOL_SHIFT * ((p_nue_reg->NUE_Register_005c.bit.ROIPOOL_SHIFT_SIGNEDNESS)? -1 : 1);
		//nue_param.roipool_parm.mode      = p_nue_reg->NUE_Register_0008.bit.ROIPOOLING_MODE;

		// set reorgnize parameters
		nue_param.reorg_parm.out_shf     = p_nue_reg->NUE_Register_0060.bit.REORGANIZE_SHIFT * ((p_nue_reg->NUE_Register_0060.bit.REORGANIZE_SHIFT_SIGNEDNESS)? -1 : 1);

		// set permute parameters
		nue_param.permute_parm.mode      = p_nue_reg->NUE_Register_0008.bit.PERMUTE_MODE;
		nue_param.permute_parm.out_shf   = p_nue_reg->NUE_Register_01c4.bit.PERMUTE_SHIFT * ((p_nue_reg->NUE_Register_01c4.bit.PERMUTE_SHIFT_SIGNEDNESS)? -1 : 1);

		// set anchor parameters
		nue_param.anchor_parm.at_table_update = p_nue_reg->NUE_Register_00e8.bit.AT_TABLE_UPDATE;
		nue_param.anchor_parm.at_w_shf        = p_nue_reg->NUE_Register_00e8.bit.AT_W_SHIFT;

		// set softmax parameters
		nue_param.softmax_parm.in_shf    = p_nue_reg->NUE_Register_00ec.bit.SF_IN_SHIFT * ((p_nue_reg->NUE_Register_00ec.bit.SF_IN_SHIFT_SIGNEDNESS)? -1 : 1);
		nue_param.softmax_parm.out_shf   = p_nue_reg->NUE_Register_00ec.bit.SF_OUT_SHIFT * ((p_nue_reg->NUE_Register_00ec.bit.SF_OUT_SHIFT_SIGNEDNESS)? -1 : 1);
		//nue_param.softmax_parm.group_num = p_nue_reg->NUE_Register_00ec.bit.SF_GROUP_NUM;
		nue_param.softmax_parm.set_num   = p_nue_reg->NUE_Register_00ec.bit.SF_SET_NUM;

		// set dma info
		nue_param.dmaio_addr.addrin0         = nue_dram_in_addr0;
		nue_param.dmaio_addr.addrin1         = nue_dram_in_addr1;
		nue_param.dmaio_addr.addrin_sv       = nue_dram_in_addrSv;
		nue_param.dmaio_addr.addrin_alpha    = nue_dram_in_addrAlpha;
		nue_param.dmaio_addr.addrin_rho      = nue_dram_in_addrRho;
		nue_param.dmaio_addr.addrin_roi      = nue_dram_in_addrRoi;
		//nue_param.dmaio_addr.addrin_ll       = 0;
		nue_param.dmaio_addr.addrin_kq       = nue_dram_in_addrKQ;
		nue_param.dmaio_addr.addr_out        = nue_dram_out_addr;
		nvt_dbg(IND, "addrin0      = 0x%08X\n", nue_param.dmaio_addr.addrin0);
		nvt_dbg(IND, "addrin1      = 0x%08X\n", nue_param.dmaio_addr.addrin1);
		nvt_dbg(IND, "addrin_sv    = 0x%08X\n", nue_param.dmaio_addr.addrin_sv);
		nvt_dbg(IND, "addrin_alpha = 0x%08X\n", nue_param.dmaio_addr.addrin_alpha);
		nvt_dbg(IND, "addrin_rho   = 0x%08X\n", nue_param.dmaio_addr.addrin_rho);
		nvt_dbg(IND, "addrin_roi   = 0x%08X\n", nue_param.dmaio_addr.addrin_roi);
		nvt_dbg(IND, "addrin_kq    = 0x%08X\n", nue_param.dmaio_addr.addrin_kq);
		nvt_dbg(IND, "addr_out     = 0x%08X\n", nue_param.dmaio_addr.addr_out);

#if (NUE_TEST_INTERRUPT_EN == ENABLE)
		if (nue_setmode(NUE_OPMODE_USERDEFINE, &nue_param) != E_OK) {
			nvt_dbg(IND, "nue_setNUEMode error ..\n");
		} else {
			nvt_dbg(IND, "Set to NUE Driver ... done\n");
		}
#else
		if (nue_setmode(NUE_OPMODE_USERDEFINE_NO_INT, &nue_param) != E_OK) {
			nvt_dbg(IND, "nue_setNUEMode error ..\n");
		} else {
			nvt_dbg(IND, "Set to NUE Driver ... done\n");
		}
#endif
		//===================================================================================================
		// fire engine
#if (NUE_TEST_INTERRUPT_EN == ENABLE)
#else
		nue_clr_intr_status(NUE_INT_ALL);
		//nue_change_interrupt(NUE_INT_ALL);
#endif
		nue_pause();
		DBG_EMU( "[NUE DRV] start engine\n");
		nue_start();
		DBG_EMU( "[NUE DRV] wait frmend\n");
		#if NUE_TEST_RST
		DBG_EMU( "[NUE DRV] sw reset\n");
		nue_reset();
		nue_pause();
		nue_start();
		#endif

#if (NUE_AI_FLOW == ENABLE)
		nvt_dbg(ERR, "Error, Please disable AI_FLOW in nue_platform.h......(NOW: ENABLE)\r\n");
#endif

#if (NUE_TEST_INTERRUPT_EN == ENABLE)
		DBG_EMU( "[NUE_DRV] int mode\r\n");
		nue_wait_frameend(FALSE);
#else
		nue_loop_frameend();
		DBG_EMU( "[NUE_DRV] loop mode\r\n");
#endif
		DBG_EMU( "[NUE DRV] wait frmend done\n");
		//===================================================================================================
		// dump results
		//fmem_dcache_sync((void *)nue_dram_out_addr, nue_out_size, DMA_BIDIRECTIONAL);
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout\\NUE\\nueg%d\\nueR%d.bin", index, index);

#else
		snprintf(io_path, 64, "//mnt//sd//realout//NUE//nueg%d//nueR%d.bin", index, index);
#endif
		len = dump_data(io_path, nue_dram_out_addr, nue_out_size);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
		#if CACHE_DBG
		cache_tmp_buf = (UINT32*)nue_dram_out_addr;
		for (cache_i = 0; cache_i < nue_out_size/4;cache_i++) {
			if (cache_tmp_buf[cache_i] == 0x77777777) {
				cache_i = 0xFFFFFFFF;
				break;
			}
		}
		if (cache_i == 0xFFFFFFFF) {
			nvt_dbg(ERR, "[NUE DRV] chk cache buf fail!\n");
			break;
		}
		#endif
		//===================================================================================================
	}

	return 0;
}

void dump_whole_data(UINT32 in_addr0, UINT32 in_size0, UINT32 in_addr1, UINT32 in_size1,
						UINT32 in_addr2, UINT32 in_size2, unsigned int index,
						UINT32 gld_addr0, UINT32 gld_size0, UINT32 gld_addr1, UINT32 gld_size1,
						UINT32 gld_addr2, UINT32 gld_size2)
{
	char io_path[100];
	int len = 0;

	if (in_size0 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\DI0.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//DI0.bin", index);
#endif
		len = dump_data(io_path, in_addr0, in_size0);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
	}

	if (in_size1 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\DI1.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//DI1.bin", index);
#endif
		len = dump_data(io_path, in_addr1, in_size1);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
	}

	if (in_size2 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\DI2.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//DI2.bin", index);
#endif
		len = dump_data(io_path, in_addr2, in_size2);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
	}
	//golden

	if (gld_size0 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\GI0.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//GI0.bin", index);
#endif
		len = dump_data(io_path, gld_addr0, gld_size0);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
	}

	if (gld_size1 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\GI1.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//GI1.bin", index);
#endif
		len = dump_data(io_path, gld_addr1, gld_size1);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
	}

	if (gld_size2 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\GI2.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//GI2.bin", index);
#endif
		len = dump_data(io_path, gld_addr2, gld_size2);
		if (len == 0) {
			nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
		}
	}

	return;
}

int load_golden_data(UINT32 gld_addr0, UINT32 out_size0, UINT32 gld_addr1, UINT32 out_size1,
						UINT32 gld_addr2, UINT32 out_size2,
						UINT32 *gld_size0, UINT32 *gld_size1, UINT32 *gld_size2, unsigned int index)
{
	char io_path[100];
	int len = 0;


	if (out_size0 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout\\NUE2\\nue2g%d\\DO0.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout//NUE2//nue2g%d//DO0.bin", index);
#endif
		*gld_size0 = find_data_size(io_path);
		len = load_data(io_path, gld_addr0, *gld_size0);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return len;
		}
	}

	if (out_size1 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout\\NUE2\\nue2g%d\\DO1.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout//NUE2//nue2g%d//DO1.bin", index);
#endif

		*gld_size1 = find_data_size(io_path);
		len = load_data(io_path, gld_addr1, *gld_size1);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return len;
		}
	}

	if (out_size2 != 0) {
#if defined(__FREERTOS)
		snprintf(io_path, 64, "A:\\realout\\NUE2\\nue2g%d\\DO2.bin", index);
#else
		snprintf(io_path, 64, "//mnt//sd//realout//NUE2//nue2g%d//DO2.bin", index);
#endif
		*gld_size2 = find_data_size(io_path);
		len = load_data(io_path, gld_addr2, *gld_size2);
		if (len <= 0) {
			nvt_dbg(ERR, "failed in file read:%s\n", io_path);
			return len;
		}
	}

	return len;
}

/**
    NUE2 Initiate strip parameter

    Initiate NUE2 strip parameter.

    @param[in] strip parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_init_strip_parm(NUE2_TEST_PARM *p_parm)
{
	ER er_return=E_OK;

	p_parm->strip_parm.in_width = p_parm->insize.in_width;
	p_parm->strip_parm.in_height = p_parm->insize.in_height;
	
	p_parm->strip_parm.scl_width = p_parm->scale_parm.h_scl_size;
	p_parm->strip_parm.scl_height = p_parm->scale_parm.v_scl_size;
	p_parm->strip_parm.flip_mode = (UINT32) p_parm->flip_parm.flip_mode;
	p_parm->strip_parm.sub_en = (UINT8) p_parm->func_en.sub_en;
	p_parm->strip_parm.sub_mode = (UINT8) p_parm->sub_parm.sub_mode;
	p_parm->strip_parm.sub_in_width = p_parm->sub_parm.sub_in_width;
	p_parm->strip_parm.hsv_en = (UINT8) p_parm->func_en.hsv_en;
	p_parm->strip_parm.hsv_out_mode = (UINT8) p_parm->hsv_parm.hsv_out_mode;
	p_parm->strip_parm.in_fmt = p_parm->infmt;
	p_parm->strip_parm.hsv_en_mode = NUE2_NO_HSV;

	if (p_parm->strip_parm.sub_en == 1 && p_parm->strip_parm.sub_mode == NUE2_PLANER_MODE) {
		p_parm->strip_parm.sub_planer_en = 1;
		p_parm->strip_parm.sub_dup_mode = p_parm->sub_parm.sub_dup;
	} else {
		p_parm->strip_parm.sub_planer_en = 0;
		p_parm->strip_parm.sub_dup_mode = 0;
	}

	er_return = nue2_check_strip_parm(p_parm);
	if (er_return != E_OK) {
		p_parm->strip_parm.s_num = 1;
		//return er_return; //Don't need to return, just set p_parm->strip_parm.s_num=1
		p_parm->strip_parm.is_strip = 0;
		p_parm->strip_parm.s_step = p_parm->strip_parm.scl_width;
		er_return = E_PAR;
		goto ret_end;
	} else {
		p_parm->strip_parm.s_num = (UINT32) p_parm->flow_ct.s_num;
		if (p_parm->strip_parm.s_num == 1) {
			p_parm->strip_parm.is_strip = 0;
		} else {
			p_parm->strip_parm.is_strip = 1;
		}

		if (p_parm->strip_parm.hsv_en == 1 && p_parm->strip_parm.hsv_out_mode == 0) { //8bit
			p_parm->strip_parm.hsv_en_mode = (UINT8) NUE2_HSV_8BIT;
		} else if (p_parm->strip_parm.hsv_en == 1 && p_parm->strip_parm.hsv_out_mode == 1) { //9bit
			p_parm->strip_parm.hsv_en_mode = (UINT8) NUE2_HSV_9BIT;
		} else {
			p_parm->strip_parm.hsv_en_mode = (UINT8) NUE2_NO_HSV;
		}

		p_parm->strip_parm.s_step = NUE2_8_BYTE_ALIGN_CEILING(p_parm->strip_parm.scl_width / p_parm->strip_parm.s_num);
		if (p_parm->strip_parm.s_step == 0) {
			nvt_dbg(ERR, "Error, scl_width(%d) / s_num(%d) is smaller than 8-byte can't be done stripe.\r\n", p_parm->strip_parm.scl_width, p_parm->strip_parm.s_num);
			p_parm->strip_parm.s_num = 1;
			p_parm->strip_parm.is_strip = 0;
			p_parm->strip_parm.s_step = p_parm->strip_parm.scl_width;
		}
	}

ret_end:

	return er_return;
}


/**
    NUE2 Check strip parameter

    Check NUE2 strip parameter.

    @param[in] strip parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_check_strip_parm(NUE2_TEST_PARM *p_parm)
{
	ER er_return=E_OK;

	if (p_parm->func_en.pad_en || p_parm->func_en.rotate_en || p_parm->func_en.hsv_en) {
		nvt_dbg(ERR, "nue2: padding/rotate/hsv case can't support stripe now.\r\n");
		er_return = E_PAR;
		goto ret_end;
	}

	if (p_parm->strip_parm.in_fmt == NUE2_YUV_420) {
		if (p_parm->dmaio_lofs.in0_lofs == 0 || p_parm->dmaio_lofs.in1_lofs == 0) {
			nvt_dbg(ERR, "nue2: in lineoffset=0 case can't support stripe now.\r\n");
			er_return = E_PAR;
			goto ret_end;
		}
	} else if (p_parm->strip_parm.in_fmt == NUE2_RGB_PLANNER) {
		if (p_parm->dmaio_lofs.in0_lofs == 0 || p_parm->dmaio_lofs.in1_lofs == 0 || p_parm->dmaio_lofs.in2_lofs == 0) {
			nvt_dbg(ERR, "nue2: in lineoffset=0 case can't support stripe now.\r\n");
			er_return = E_PAR;
			goto ret_end;
		}
	} else { // Y / UV
		if (p_parm->dmaio_lofs.in0_lofs == 0) {
			nvt_dbg(ERR, "nue2: in lineoffset=0 case can't support stripe now.\r\n");
			er_return = E_PAR;
			goto ret_end;
		}
	}

	if (p_parm->strip_parm.sub_planer_en == 1) {
		if (p_parm->dmaio_lofs.in2_lofs == 0) {
			nvt_dbg(ERR, "nue2: in lineoffset=0 case can't support stripe now.\r\n");
			er_return = E_PAR;
			goto ret_end;
		}
	}


	if ((p_parm->strip_parm.in_fmt == NUE2_YUV_420) || (p_parm->strip_parm.in_fmt == NUE2_RGB_PLANNER)) {
		if (p_parm->dmaio_lofs.out0_lofs == 0 || p_parm->dmaio_lofs.out1_lofs == 0 || p_parm->dmaio_lofs.out2_lofs == 0) {
			nvt_dbg(ERR, "nue2: out lineoffset=0 case can't support stripe now.\r\n");
			er_return = E_PAR;
			goto ret_end;
		}
	} else {
		if (p_parm->dmaio_lofs.out0_lofs == 0) {
			nvt_dbg(ERR, "nue2: out lineoffset=0 case can't support stripe now.\r\n");
			er_return = E_PAR;
			goto ret_end;
		}
	}

	if ((p_parm->scale_parm.scale_h_mode != 0) || (p_parm->scale_parm.scale_v_mode != 0)) {
		nvt_dbg(ERR, "nue2: scaling-up(h or v) case can't support stripe now.\r\n");
		er_return = E_PAR;
		goto ret_end;
	}


ret_end:

	return er_return;
}


/**
    NUE2 Set strip parameter

    Set NUE2 strip parameter.

    @param[in] strip parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_strip_parm(NUE2_STRIP_PARM *strip_parm, UINT64 **ll_buf, UINT8 is_bit60, UINT32 ll_base_addr)
{
	INT32 is_first;
    UINT32 ofs, ofs1;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    ER erReturn = E_OK;
	static UINT32 in_width_byte = 0;
	static UINT32 in_width_byte_sai1 = 0;
	static UINT32 in_width_shift = 0;
	static UINT32 in_width_shift_sai1 = 0;
	static UINT32 in_width_s = 0;
	static UINT32 sai0 = 0;
	UINT32 sai0_s;
	static UINT32 sai1 = 0;
	UINT32 sai1_s;
	static UINT32 sai2 = 0;
	UINT32 sai2_s;
	static UINT32 v_scl_size = 0;
	static UINT32 h_scl_size_off_byte = 0;
	static UINT32 h_scl_size_s = 0;
	static UINT32 total_h_scl_size_byte = 0;
	static UINT32 strip_out_size_byte = 0;

	static UINT32 sao0 = 0;
	UINT32 sao0_s;
	static UINT32 sao1 = 0;
	UINT32 sao1_s;
	static UINT32 sao2 = 0;
	UINT32 sao2_s;

	static UINT32 final_dnrate = 0;
	static UINT32 final_sfact = 0;

	UINT32 in_fmt;

	static UINT32 strip_out_size = 0;
	static UINT32 last_strip_out_size = 0;
	static UINT32 strip_in_size = 0;
	static UINT32 last_strip_in_size = 0;
	UINT32 h_sfact;
	UINT32 h_dnrate;

#if defined(__FREERTOS)
	float tmp_cal;
#else
	UINT32 tmp_cal;
	UINT32 tmp_cal1;
	UINT32 tmp_cal2;
#endif

	static UINT8 sub_dup_div = 1;
	
	static UINT32 sub_remain_width_div = 0;
	static UINT32 sub_width_s_byte_div = 0;
	static UINT32 last_sub_width_byte = 0;
	static UINT32 last_strip_out_size_byte = 0;
	static UINT32 sub_remain_width = 0;
	static UINT32 sub_width_s_byte = 0;
	static NUE2_HSV_MODE hsv_mode = NUE2_NO_HSV;

	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg_en;
	T_NUE2_INPUT_SIZE_REGISTER0 reg_in_size;
	T_SCALING_OUTPUT_SIZE_REGISTER0 reg_out_size;
	T_MEAN_SUBTRACTION_REGISTER0 reg_sub_0;
	T_DMA_TO_NUE2_REGISTER0 reg_in_addr_0;
	T_DMA_TO_NUE2_REGISTER1 reg_in_addr_1;
	T_DMA_TO_NUE2_REGISTER2 reg_in_addr_2;
	T_NUE2_TO_DMA_RESULT_REGISTER0 reg_out_addr_0;
	T_NUE2_TO_DMA_RESULT_REGISTER1 reg_out_addr_1;
	T_NUE2_TO_DMA_RESULT_REGISTER2 reg_out_addr_2;
	T_SCALING_OUTPUT_SIZE_REGISTER0 reg_out_v_size;
#if (NUE2_DBG_STRIPE == ENABLE)
	UINT32 final_dnrate_ck = 0;
	UINT32 final_sfact_ck = 0;
	T_SCALE_DOWN_RATE_REGISTER0 reg_scale;
	T_SCALE_DOWN_RATE_REGISTER1 reg_scale_1;
	T_SCALE_DOWN_RATE_REGISTER3 reg_scale_3;
#endif
	T_SCALE_DOWN_RATE_REGISTER2 reg_scale_2;

	
	//volatile UINT32 *reg_dump=0;

	//reg_dump = (UINT32 *)0xfe000050;

	//DBG_EMU("fe00005 = 0x%x\r\n", *reg_dump);

	if ((UINT32)strip_parm->s_posi == strip_parm->s_step) {
		is_first = NUE2_STRIP_BEGIN; //begin
	} else if ((UINT32)strip_parm->s_posi == strip_parm->scl_width) {
		is_first = NUE2_STRIP_END; //end
	} else {
		is_first = NUE2_STRIP_NORM;
	}
	
	if (is_first == NUE2_STRIP_BEGIN) {
		in_width_byte = 0;
		in_width_byte_sai1 = 0;
		in_width_shift = 0;
		in_width_shift_sai1 = 0;
		in_width_s = 0;
		sai0 = 0;
		sai1 = 0;
		sai2 = 0;
		v_scl_size = 0;
		h_scl_size_off_byte = 0;
		h_scl_size_s = 0;
		total_h_scl_size_byte = 0;
		strip_out_size_byte = 0;
		
		sao0 = 0;
		sao1 = 0;
		sao2 = 0;
		
		final_dnrate = 0;
		final_sfact = 0;

		strip_out_size = 0;
		last_strip_out_size = 0;
	
		strip_in_size = 0;
		last_strip_in_size = 0;

		if (strip_parm->sub_planer_en == 1) {
			switch (strip_parm->sub_dup_mode) {
				case 0:
					sub_dup_div = 1;
					break;
				case 1:
					sub_dup_div = 2;
					break;
				case 2:
					sub_dup_div = 4;
					break;
				case 3:
					sub_dup_div = 8;
					break;
				default:
					sub_dup_div = 1;
			}
		}
		sub_remain_width_div = strip_parm->sub_in_width;
		sub_width_s_byte_div = 0;
		last_sub_width_byte = 0;
		last_strip_out_size_byte = 0;
		sub_remain_width = 0;
    	sub_width_s_byte = 0;
		sub_remain_width = strip_parm->sub_in_width * sub_dup_div;
		hsv_mode = (NUE2_HSV_MODE) strip_parm->hsv_en_mode;
	}

	if (is_first != NUE2_STRIP_END) {
		strip_out_size = strip_parm->s_posi;
	} else { //end
		strip_out_size = strip_parm->scl_width;
	}

#if defined(__FREERTOS)
	tmp_cal = ((float)(strip_out_size - 1) * (float) (strip_parm->in_width - 1)/(float) (strip_parm->scl_width - 1)) + 1; 
	if ((tmp_cal - (float)((UINT32)tmp_cal)) > 0) {
		tmp_cal += 1;
	}
#else
	tmp_cal1 = (strip_out_size - 1) * (strip_parm->in_width - 1);
	tmp_cal2 = (strip_parm->scl_width - 1);
	tmp_cal = (tmp_cal1 / tmp_cal2) + 1;
    if ((tmp_cal1 % tmp_cal2) != 0) {
        tmp_cal += 1;
    }
#endif
	strip_in_size = (UINT32) tmp_cal;



	//T_NUE2_FUNCTION_ENABLE_REGISTER0 reg_en;
	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg_en.reg = NUE2_GETDATA(ofs, base_addr);
	in_fmt = reg_en.bit.NUE2_IN_FMT;
	

	//to consider the last strip width
	//get in_width
	//set in_width by strip num
	//T_NUE2_INPUT_SIZE_REGISTER0 reg_in_size;
	ofs = NUE2_INPUT_SIZE_REGISTER0_OFS;
	reg_in_size.reg = NUE2_GETDATA(ofs, base_addr);

	DBG_EMU( "WW### strip_in_size=0x%x\r\n", strip_in_size);
	if (in_fmt == 0) {
		in_width_byte = last_strip_in_size;
		in_width_byte_sai1 = last_strip_in_size;
		in_width_shift = 1;
		in_width_shift_sai1 = 1;
	} else if (in_fmt == 1) {
		in_width_byte = last_strip_in_size;
		in_width_byte_sai1 = 0;
		in_width_shift = 1;
		in_width_shift_sai1 = 0;
	} else if (in_fmt == 2) {
		in_width_byte = last_strip_in_size * 2;
		in_width_byte_sai1 = 0;
		in_width_shift = 2;
		in_width_shift_sai1 = 0;
	} else if (in_fmt == 3) {
		in_width_byte = last_strip_in_size;
		in_width_byte_sai1 = last_strip_in_size;
		in_width_shift = 1;
		in_width_shift_sai1 = 1;
	}

	if (strip_parm->in_width == strip_parm->scl_width) {
		in_width_shift = 0;
		in_width_shift_sai1 = 0;
	}

	in_width_s = strip_in_size - last_strip_in_size;
	last_strip_in_size = strip_in_size;
		

	//h_scl_size
	//get NUE2_H_SCL_SIZE
	//set NUE2_H_SCL_SIZE by strip number
	//T_SCALING_OUTPUT_SIZE_REGISTER0 reg_out_size;
	ofs1 = SCALING_OUTPUT_SIZE_REGISTER0_OFS;
	reg_out_size.reg = NUE2_GETDATA(ofs1, base_addr);

	h_scl_size_s = strip_out_size - last_strip_out_size; 

	DBG_EMU( "WW### strip_out_size=0x%x\r\n", strip_out_size);
	if (in_fmt == 0) {
		//for both 420/422
		h_scl_size_off_byte = last_strip_out_size;
		total_h_scl_size_byte = strip_parm->scl_width;  	//flip
		strip_out_size_byte = strip_out_size;	//flip
		last_sub_width_byte = strip_parm->sub_in_width * 3 - 3; //sub
		last_strip_out_size_byte = last_strip_out_size * 3; //sub
	} else if (in_fmt == 1) {
		h_scl_size_off_byte = last_strip_out_size;
		total_h_scl_size_byte = strip_parm->scl_width;			//flip
		strip_out_size_byte = strip_out_size;		//flip
		last_sub_width_byte = strip_parm->sub_in_width - 1; 	//sub
		last_strip_out_size_byte = last_strip_out_size; //sub
	} else if (in_fmt == 2) {
		h_scl_size_off_byte = last_strip_out_size * 2;
		total_h_scl_size_byte = strip_parm->scl_width * 2;		//flip
		strip_out_size_byte = strip_out_size * 2;	//flip
		last_sub_width_byte = strip_parm->sub_in_width * 2 - 2; //sub
		last_strip_out_size_byte = last_strip_out_size * 2; //sub
	} else if (in_fmt == 3) {
		h_scl_size_off_byte = last_strip_out_size;
		total_h_scl_size_byte = strip_parm->scl_width;		//flip
		strip_out_size_byte = strip_out_size; 	//flip
		last_sub_width_byte = 0; //sub
		last_strip_out_size_byte = last_strip_out_size; //sub
		if (strip_parm->sub_planer_en == 1) {
			nvt_dbg(ERR, "Error, sub planner mode can't support RGB format.\r\n");
		}
	}//byte calculate

	if (hsv_mode == NUE2_HSV_8BIT) {
		h_scl_size_off_byte = h_scl_size_off_byte * 3;
	} else if (hsv_mode == NUE2_HSV_9BIT) {
		h_scl_size_off_byte = h_scl_size_off_byte * 4;
	} else {
		//no action
	}
	
	//sub_dup handle
	if (sub_remain_width >= h_scl_size_s) {
		sub_width_s_byte = last_strip_out_size_byte;
		sub_remain_width_div = sub_remain_width / sub_dup_div;
		sub_width_s_byte_div = sub_width_s_byte / sub_dup_div;
		sub_remain_width = sub_remain_width - h_scl_size_s;
	} else {
		if (sub_remain_width == 0) {
			sub_remain_width_div = 1;
			sub_width_s_byte_div = last_sub_width_byte;
		} else {
			sub_width_s_byte = last_strip_out_size_byte;
			sub_remain_width_div = sub_remain_width / sub_dup_div;
			sub_width_s_byte_div = sub_width_s_byte / sub_dup_div;
			sub_remain_width = 0;
		}
	}
	
	last_strip_out_size = strip_out_size; //it must be behind byte calculate and sub handle
    
	//recalculation
	if (strip_parm->in_width == strip_parm->scl_width) {
		in_width_shift = 0;
	}

	if (is_first != NUE2_STRIP_BEGIN) {
		in_width_s += in_width_shift;
	}

	//set in_width_s and h_scl_size_s to register
	DBG_EMU( "WW### in_width_s=0x%x\r\n", in_width_s);
	reg_in_size.bit.NUE2_IN_WIDTH = in_width_s;	
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_in_size.reg, base_addr);
	} else {
		nue2_setdata_ll(ll_buf, ofs, reg_in_size.reg, 0);
	}

	DBG_EMU( "WW### h_scl_size_s=0x%x\r\n", h_scl_size_s);
	reg_out_size.bit.NUE2_H_SCL_SIZE = h_scl_size_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs1, reg_out_size.reg, base_addr);
	} else {
		nue2_setdata_ll(ll_buf, ofs1, reg_out_size.reg, 0);
	}

	//set SUB
    //T_MEAN_SUBTRACTION_REGISTER0 reg_sub_0;
	ofs = MEAN_SUBTRACTION_REGISTER0_OFS;
	reg_sub_0.reg = NUE2_GETDATA(ofs, base_addr);

	DBG_EMU( "WW### sub_remain_width_div=0x%x\r\n", sub_remain_width_div);

	reg_sub_0.bit.NUE2_SUB_IN_WIDTH = sub_remain_width_div; //sub after scalingA
	
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_sub_0.reg, base_addr);
	} else {
		nue2_setdata_ll(ll_buf, ofs, reg_sub_0.reg, 0);
	}


	//set SAI0 by strip num and strip order
    //T_DMA_TO_NUE2_REGISTER0 reg_in_addr_0;
	ofs = DMA_TO_NUE2_REGISTER0_OFS;
	reg_in_addr_0.reg = NUE2_GETDATA(ofs, base_addr);
	if (sai0 == 0) {
		sai0 = reg_in_addr_0.bit.DRAM_SAI0;
		DBG_EMU( "WW### sai0=0x%x\r\n", sai0);
	}
	sai0_s = sai0 + in_width_byte;
	if (is_first != NUE2_STRIP_BEGIN) {
		sai0_s -= in_width_shift;
	}
	DBG_EMU( "WW### sai0_s=0x%x\r\n", sai0_s);
	reg_in_addr_0.bit.DRAM_SAI0 = sai0_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_in_addr_0.reg, base_addr);
	} else {
		if (ll_base_addr) {
			reg_in_addr_0.bit.DRAM_SAI0 = reg_in_addr_0.bit.DRAM_SAI0 - ll_base_addr;
			nue2_setdata_ll(ll_buf, ofs, reg_in_addr_0.reg, 2);
		} else {
			nue2_setdata_ll(ll_buf, ofs, reg_in_addr_0.reg, 0);
		}
	}

	//set SAI1 by strip num and strip order
	//T_DMA_TO_NUE2_REGISTER1 reg_in_addr_1;
	ofs = DMA_TO_NUE2_REGISTER1_OFS;
	reg_in_addr_1.reg = NUE2_GETDATA(ofs, base_addr);
	if (sai1 == 0) {
		sai1 = reg_in_addr_1.bit.DRAM_SAI1;
		DBG_EMU( "WW### sai1=0x%x\r\n", sai1);
	}
	sai1_s = sai1 + in_width_byte_sai1;
	if (is_first != NUE2_STRIP_BEGIN) {
		sai1_s -= in_width_shift_sai1;
	}
	DBG_EMU( "WW### sai1_s=0x%x\r\n", sai1_s);
	reg_in_addr_1.bit.DRAM_SAI1 = sai1_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_in_addr_1.reg, base_addr);
	} else {
		if (ll_base_addr) {
			reg_in_addr_1.bit.DRAM_SAI1 = reg_in_addr_1.bit.DRAM_SAI1 - ll_base_addr;
			nue2_setdata_ll(ll_buf, ofs, reg_in_addr_1.reg, 2);
		} else {
			nue2_setdata_ll(ll_buf, ofs, reg_in_addr_1.reg, 0);
		}
	}

	//set SAI2 by strip num and strip order
	//T_DMA_TO_NUE2_REGISTER2 reg_in_addr_2;
	ofs = DMA_TO_NUE2_REGISTER2_OFS;
	reg_in_addr_2.reg = NUE2_GETDATA(ofs, base_addr);
	if (sai2 == 0) {
		sai2 = reg_in_addr_2.bit.DRAM_SAI2;
		DBG_EMU( "WW### sai2=0x%x\r\n", sai2);
	}

	if (strip_parm->sub_planer_en == 0) { //substraction don't need to do shift because sub after scaling.
		sai2_s = sai2 + in_width_byte;
		if (is_first != NUE2_STRIP_BEGIN) {
			sai2_s -= in_width_shift;
		}
	} else {
		DBG_EMU( "WW### sub_width_s_byte_div=0x%x\r\n", sub_width_s_byte_div);
		sai2_s = sai2 + sub_width_s_byte_div;
	}
	DBG_EMU( "WW### sai2_s=0x%x\r\n", sai2_s);
	reg_in_addr_2.bit.DRAM_SAI2 = sai2_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_in_addr_2.reg, base_addr);
	} else {
		if (ll_base_addr) {
			reg_in_addr_2.bit.DRAM_SAI2 = reg_in_addr_2.bit.DRAM_SAI2 - ll_base_addr;
			nue2_setdata_ll(ll_buf, ofs, reg_in_addr_2.reg, 2);
		} else {
			nue2_setdata_ll(ll_buf, ofs, reg_in_addr_2.reg, 0);
		}
	}

	

	//set SAO0 by strip num abd strip order
    //T_NUE2_TO_DMA_RESULT_REGISTER0 reg_out_addr_0;
	ofs = NUE2_TO_DMA_RESULT_REGISTER0_OFS;
	reg_out_addr_0.reg = NUE2_GETDATA(ofs, base_addr);
	if (sao0 == 0) {
		sao0 = reg_out_addr_0.bit.DRAM_SAO0;
		DBG_EMU( "WW### sao0=0x%x\r\n", sao0);
	}
	if (strip_parm->flip_mode == (UINT32) NUE2_Y_FLIP || strip_parm->flip_mode == (UINT32) NUE2_NO_FLIP) {
		sao0_s = sao0 + h_scl_size_off_byte;
	} else {
		sao0_s = sao0 + (total_h_scl_size_byte - strip_out_size_byte);
	}
	DBG_EMU( "WW### sao0_s=0x%x\r\n", sao0_s);
	reg_out_addr_0.bit.DRAM_SAO0 = sao0_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_out_addr_0.reg, base_addr);
	} else {
		if (ll_base_addr) {
			reg_out_addr_0.bit.DRAM_SAO0 = reg_out_addr_0.bit.DRAM_SAO0 - ll_base_addr;
			nue2_setdata_ll(ll_buf, ofs, reg_out_addr_0.reg, 2);
		} else {
			nue2_setdata_ll(ll_buf, ofs, reg_out_addr_0.reg, 0);
		}
	}

	//set SAO1 by strip num abd strip order
    //T_NUE2_TO_DMA_RESULT_REGISTER1 reg_out_addr_1;
	ofs = NUE2_TO_DMA_RESULT_REGISTER1_OFS;
	reg_out_addr_1.reg = NUE2_GETDATA(ofs, base_addr);
	if (sao1 == 0) {
		sao1 = reg_out_addr_1.bit.DRAM_SAO1;
		DBG_EMU( "WW### sao1=0x%x\r\n", sao1);
	}
	if (strip_parm->flip_mode == (UINT32) NUE2_Y_FLIP || strip_parm->flip_mode == (UINT32) NUE2_NO_FLIP) {
		sao1_s = sao1 + h_scl_size_off_byte;
	} else {
		sao1_s = sao1 + (total_h_scl_size_byte - strip_out_size_byte);
	}
	DBG_EMU( "WW### sao1_s=0x%x\r\n", sao1_s);
	reg_out_addr_1.bit.DRAM_SAO1 = sao1_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_out_addr_1.reg, base_addr);
	} else {
		if (ll_base_addr) {
			reg_out_addr_1.bit.DRAM_SAO1 = reg_out_addr_1.bit.DRAM_SAO1 - ll_base_addr;
			nue2_setdata_ll(ll_buf, ofs, reg_out_addr_1.reg, 2);
		} else {
			nue2_setdata_ll(ll_buf, ofs, reg_out_addr_1.reg, 0);
		}
	}

	//set SAO2 by strip num abd strip order
    //T_NUE2_TO_DMA_RESULT_REGISTER2 reg_out_addr_2;
	ofs = NUE2_TO_DMA_RESULT_REGISTER2_OFS;
	reg_out_addr_2.reg = NUE2_GETDATA(ofs, base_addr);
	if (sao2 == 0) {
		sao2 = reg_out_addr_2.bit.DRAM_SAO2;
		DBG_EMU( "WW### sao2=0x%x\r\n", sao2);
	}
	if (strip_parm->flip_mode == (UINT32) NUE2_Y_FLIP || strip_parm->flip_mode == (UINT32) NUE2_NO_FLIP) {
		sao2_s = sao2 + h_scl_size_off_byte;
	} else {
		sao2_s = sao2 + (total_h_scl_size_byte - strip_out_size_byte);
	}
	DBG_EMU( "WW### sao2_s=0x%x\r\n", sao2_s);
	reg_out_addr_2.bit.DRAM_SAO2 = sao2_s;
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_out_addr_2.reg, base_addr);
	} else {
		if (ll_base_addr) {
			reg_out_addr_2.bit.DRAM_SAO2 = reg_out_addr_2.bit.DRAM_SAO2 - ll_base_addr;
			nue2_setdata_ll(ll_buf, ofs, reg_out_addr_2.reg, 2);
		} else {
			nue2_setdata_ll(ll_buf, ofs, reg_out_addr_2.reg, 0);
		}
	}

	//get NUE2_V_SCL_SIZE
	if (v_scl_size == 0) {
		//T_SCALING_OUTPUT_SIZE_REGISTER0 reg_out_v_size;
		ofs = SCALING_OUTPUT_SIZE_REGISTER0_OFS;
		reg_out_v_size.reg = NUE2_GETDATA(ofs, base_addr);
		v_scl_size = reg_out_v_size.bit.NUE2_V_SCL_SIZE;
		DBG_EMU( "WW### v_scl_size=0x%x\r\n", v_scl_size);
		//NUE2_SETDATA(ofs, reg_out_v_size.reg, base_addr);
	}

	//get in_height
	DBG_EMU( "WW### in_height=0x%x\r\n", strip_parm->in_height);


#if 1
	//set NUE2_H_DNRATE / NUE2_V_DNRATE
	//T_SCALE_DOWN_RATE_REGISTER0 reg_scale;
	ofs =  SCALE_DOWN_RATE_REGISTER0_OFS;
#if (NUE2_DBG_STRIPE == ENABLE)
	reg_scale.reg = NUE2_GETDATA(ofs, base_addr);
#endif
	//reg_scale.bit.NUE2_H_DNRATE = (((in_width_s - 1)/(h_scl_size_s - 1)) - 1);
	DBG_EMU( "WW### H_DNRATE=0x%x\r\n", reg_scale.bit.NUE2_H_DNRATE);
	//reg_scale.bit.NUE2_V_DNRATE = (((strip_parm->in_height - 1)/(v_scl_size - 1)) - 1);
	DBG_EMU( "WW### V_DNRATE=0x%x\r\n", reg_scale.bit.NUE2_V_DNRATE);
	//NUE2_SETDATA(ofs, reg_scale.reg, base_addr);

	//set NUE2_H_SFACT / NUE2_V_SFACT
	//T_SCALE_DOWN_RATE_REGISTER1 reg_scale_1;
	ofs = SCALE_DOWN_RATE_REGISTER1_OFS;
#if (NUE2_DBG_STRIPE == ENABLE)
	reg_scale_1.reg = NUE2_GETDATA(ofs, base_addr);
#endif
	//reg_scale_1.bit.NUE2_H_SFACT = (UINT16) ( (float) ((float)(in_width_s - 1)/(float)(h_scl_size_s - 1))  * 65536);
	DBG_EMU( "WW### H_SFACT=0x%x\r\n", reg_scale_1.bit.NUE2_H_SFACT);
	//reg_scale_1.bit.NUE2_V_SFACT = (UINT16) ( (float) ((float)(strip_parm->in_height - 1)/(float)(v_scl_size - 1)) * 65536);
	DBG_EMU( "WW### V_SFACT=0x%x\r\n", reg_scale_1.bit.NUE2_V_SFACT);
	//NUE2_SETDATA(ofs, reg_scale_1.reg, base_addr);
#endif

#if 1 //get final_dnrate and final_sfact
	//get NUE2_FINAL_H_DNRATE / NUE2_FINAL_H_SFACT
	//T_SCALE_DOWN_RATE_REGISTER3 reg_scale_3;
	ofs = SCALE_DOWN_RATE_REGISTER3_OFS;
#if (NUE2_DBG_STRIPE == ENABLE)
	reg_scale_3.reg = NUE2_GETDATA(ofs, base_addr);
	final_dnrate_ck = reg_scale_3.bit.NUE2_FINAL_H_DNRATE;
#endif
	DBG_EMU( "WW### final_dnrate_ck=0x%x\r\n", final_dnrate_ck);
#if (NUE2_DBG_STRIPE == ENABLE)
	final_sfact_ck = reg_scale_3.bit.NUE2_FINAL_H_SFACT;
#endif
	DBG_EMU( "WW### final_sfact_ck=0x%x\r\n", final_sfact_ck);
	//NUE2_SETDATA(ofs, reg_scale_3.reg, base_addr);
#endif
	

	//set NUE2_INI_H_DNRATE /  NUE2_INI_H_SFACT  by last NUE2_FINAL_H_DNRATE / NUE2_FINAL_H_SFACT
	//T_SCALE_DOWN_RATE_REGISTER2 reg_scale_2;
	ofs = SCALE_DOWN_RATE_REGISTER2_OFS;
	reg_scale_2.reg = NUE2_GETDATA(ofs, base_addr);
	if (is_first == NUE2_STRIP_BEGIN) {
		reg_scale_2.bit.NUE2_INI_H_DNRATE = 0;
		reg_scale_2.bit.NUE2_INI_H_SFACT = 0;
	} else {
		reg_scale_2.bit.NUE2_INI_H_DNRATE = final_dnrate;
		reg_scale_2.bit.NUE2_INI_H_SFACT = final_sfact;
	}
	DBG_EMU( "WW### NUE2_INI_H_DNRATE=0x%x\r\n", reg_scale_2.bit.NUE2_INI_H_DNRATE);
	DBG_EMU( "WW### NUE2_INI_H_SFACT=0x%x\r\n", reg_scale_2.bit.NUE2_INI_H_SFACT);
	if (*ll_buf == NULL) {
		NUE2_SETDATA(ofs, reg_scale_2.reg, base_addr);
	} else {
		nue2_setdata_ll(ll_buf, ofs, reg_scale_2.reg, 0);
		nue2_setdata_ll(ll_buf, ofs, reg_scale_2.reg, is_bit60);
	}

	//DBG_EMU("end fe00005 = 0x%x\r\n", *reg_dump);

	//calculate final_dnrate and final_sfact
#if defined(__FREERTOS)
	h_dnrate = ((UINT32) ((float)(strip_parm->in_width - 1) / (float)(strip_parm->scl_width - 1))) - 1;
	h_sfact = (UINT16) (((float)(strip_parm->in_width-1)/(float)(strip_parm->scl_width-1) - h_dnrate - 1) * (1 << 16));
	final_dnrate = NUE2_MOD((strip_in_size - NUE2_FLOOR((strip_out_size - 1) * h_sfact / 65536, 1) - 1), (h_dnrate + 1));
	final_sfact = NUE2_MOD((strip_out_size-1) * h_sfact, 65536) + (1- final_dnrate) * h_sfact;
#else
	h_dnrate = ((UINT32) ((strip_parm->in_width - 1) / (strip_parm->scl_width - 1))) - 1;
    h_sfact = (UINT16) (((strip_parm->in_width - 1) / (strip_parm->scl_width - 1) - h_dnrate - 1) * (1 << 16));
    final_dnrate = NUE2_MOD((strip_in_size - NUE2_FLOOR((strip_out_size - 1) * h_sfact / 65536, 1) - 1), (h_dnrate + 1));
    final_sfact = NUE2_MOD((strip_out_size-1) * h_sfact, 65536) + (1- final_dnrate) * h_sfact;
#endif

	DBG_EMU( "strip_out_size=0x%x, strip_in_size=0x%x, h_dnrate=0x%x, h_sfact=0x%x, \
					final_dnrate=0x%x, final_sfact=0x%x\r\n", 
				strip_out_size, strip_in_size, h_dnrate, h_sfact, final_dnrate, final_sfact);



    return erReturn;
}

/*
    NUE2 while loop routine

    NUE2 while loop routine

    @return None.
*/
void nue2_loop_llend(NUE2_TEST_PARM *p_parm)
{
	UINT32 nue2_int_status = 0;
	//UINT32 s_time, e_time;
    UINT32 count = 0;
#if defined(__FREERTOS)
	UINT32 timeout = 3000;
#else
    const INT32 time_limit = 3000000;
    struct timeval s_time, e_time;
    INT32 dur_time = 0; // (us)
#endif

	nue2_int_status = nue2_get_intr_status();
	//DBG_EMU( "A WW### int status=%x\r\n", nue2_int_status);

	count = 0;

#if defined(__FREERTOS)
#else
	do_gettimeofday(&s_time);
#endif


#if defined(__FREERTOS)
	while ((nue2_int_status & NUE2_INT_LLEND) != NUE2_INT_LLEND) {
#else
	while (((nue2_int_status & NUE2_INT_LLEND) != NUE2_INT_LLEND)  && (dur_time < time_limit) ) {
#endif
#if defined(__FREERTOS)

		if (p_parm->flow_ct.rand_ch_en == ENABLE) {
			nue2_pt_ch_enable();
		}

        vos_task_delay_ms(10);
        count++;
		if ((count%100) == 0) {
			DBG_EMU( "NUE2 frame end wait count(%d).\r\n", count);
			if (p_parm->flow_ct.is_dma_test == 1) {
                nue2_pt_dma_test(ENABLE);
                DBG_EMU( "dma is enable.\r\n");
            }
		}
        if (count > timeout) {
            DBG_EMU( "NUE2 frame end wait too long(timeout).\r\n");
			//error handling
			nue2_ll_terminate(ENABLE);
			nue2_pause();
			if (g_fill_reg_only == 0) {
            	nue2_close();
				nue2_open(NULL);
			}
            break;
        }

		nue2_int_status = nue2_get_intr_status();
		//DBG_EMU( "wait frameend...\r\n");
#else
        do_gettimeofday(&e_time);
        dur_time = (e_time.tv_sec - s_time.tv_sec) * 1000000 + (e_time.tv_usec - s_time.tv_usec);
		nue2_int_status = nue2_get_intr_status();
#endif

	}
	
	//DBG_EMU( "frameend end\r\n");

}

/*
    NUE2 while loop routine

    NUE2 while loop routine

    @return None.
*/
VOID nue2_engine_isr(UINT32 int_status)
{
	//pseudo isr function for FPGA validation only
	return;
}

/*
    NUE2 while loop routine

    NUE2 while loop routine

    @return None.
*/
VOID nue2_engine_loop_llend(VOID *p_parm)
{
    nue2_loop_llend((NUE2_TEST_PARM *) p_parm);
}

/*
    NUE2 debug routine

    NUE2 debug routine

    @return None.
*/
VOID nue2_engine_debug_hook(VOID *p_parm)
{
	volatile UINT32 *clk_auto = (UINT32 *) 0xf00200b0;
	volatile UINT32 *apb_clk_auto = (UINT32 *) 0xf00200c0;
	volatile UINT32 *reg_sram_down = (UINT32 *) 0xf0011000;

#if defined(__FREERTOS)
#else
	volatile UINT32 *count_reg_pa = (UINT32 *) 0xf0d50080; //b12:switch, b13: only HW no LL command
#endif
	volatile UINT32 *count_reg = (UINT32 *) 0xf0d50080;

	NUE2_TEST_PARM *p_parm_test = (NUE2_TEST_PARM *) p_parm;
	UINT64 *tmp_buf;
	UINT32 tmp_idx;
	UINT32 clk_auto_val;
	UINT32 reg_val;
	UINT32 tmp_val;

#if defined(__FREERTOS)
#else
	count_reg = (UINT32 *) nue2_pt_pa2va_remap((UINT32) count_reg_pa, 0x100, 2);
#endif

	if (p_parm_test->flow_ct.is_dump_ll_buf == 1) {
		if (p_parm_test->flow_ct.ll_buf) {
			tmp_buf = (UINT64 *) p_parm_test->flow_ct.ll_buf;

			tmp_idx = 0;
			while(((*(tmp_buf + tmp_idx)) & ((UINT64)(~0xFF))) != 0) {
				while (((*(tmp_buf + tmp_idx) & ((UINT64)0xF<<60)) >> 60) == 0x4) { //next_upd
					DBG_ERR( "LL_BUF_TXT(0x%x): 0x%llx\r\n", ((UINT32) tmp_buf + (tmp_idx*8)), *(tmp_buf + tmp_idx));
					tmp_buf = (UINT64 *) ((UINT32) (*(tmp_buf + tmp_idx) >> 8));
					tmp_idx=0;
					if (((*(tmp_buf + tmp_idx)) & ((UINT64)(~0xFF))) == 0) {
						break;
					}
				}
				DBG_ERR( "LL_BUF_TXT(0x%x): 0x%llx\r\n", ((UINT32) tmp_buf + (tmp_idx*8)), *(tmp_buf + tmp_idx));
				if (((*(tmp_buf + tmp_idx)) & ((UINT64)(~0xFF))) != 0) {
					tmp_idx++;
				}
			}
			DBG_ERR( "LL_BUF_TXT(0x%x): 0x%llx\r\n", ((UINT32) tmp_buf + (tmp_idx*8)), *(tmp_buf + tmp_idx));
			DBG_ERR( "exit: LL_BUF_TXT\r\n");
		}
	}

	if (p_parm_test->flow_ct.auto_clk) {
		clk_auto_val = *clk_auto;
		clk_auto_val = clk_auto_val | (1<<12);
		*clk_auto = clk_auto_val;

		clk_auto_val = *apb_clk_auto;
		clk_auto_val = clk_auto_val | (1<<12);
		*apb_clk_auto = clk_auto_val;

		DBG_EMU( "NUE2_AUTO: clk_auto(0x%x) apb_clk_auto(0x%x)\r\n", *clk_auto, *apb_clk_auto);
	}

	if (p_parm_test->flow_ct.ll_buf && p_parm_test->flow_ct.ll_base_addr) {
		nue2_set_ll_base_addr(nue2_pt_va2pa(p_parm_test->flow_ct.ll_base_addr));
	}

	if (p_parm_test->flow_ct.is_reg_dump == 1) {
		nue2_reg_dump_print();
	}

	if (p_parm_test->flow_ct.sram_down > 0) {
		reg_val = *reg_sram_down;
		reg_val = reg_val | (1<<1);
		*reg_sram_down = reg_val;
		DBG_EMU( "NUE2_SRAM_DOWN(ON): reg(0x%x)\r\n", *reg_sram_down);
		if (p_parm_test->flow_ct.sram_down == 2) {
			reg_val = *reg_sram_down;
			reg_val = reg_val & (~(1<<1));
			*reg_sram_down = reg_val;
			DBG_EMU( "NUE2_SRAM_DOWN(OFF): reg(0x%x)\r\n", *reg_sram_down);
		}
	}

#if defined(_BSP_NA51089_)
	if (p_parm_test->flow_ct.cnt_is_hw_only == 0) {
		tmp_val = *count_reg;
		tmp_val = tmp_val | (1<<12);
		tmp_val = tmp_val & (~(1<<13));
		*count_reg = tmp_val;
	} else {
		tmp_val = *count_reg;
		tmp_val = tmp_val | (1<<12);
		tmp_val = tmp_val | (1<<13);
		*count_reg = tmp_val;
	}
#else		
	if(nvt_get_chip_id() == CHIP_NA51055) {
    } else {
        if (p_parm_test->flow_ct.cnt_is_hw_only == 0) {
            tmp_val = *count_reg;
            tmp_val = tmp_val | (1<<12);
            tmp_val = tmp_val & (~(1<<13));
            *count_reg = tmp_val;
        } else {
            tmp_val = *count_reg;
            tmp_val = tmp_val | (1<<12);
            tmp_val = tmp_val | (1<<13);
            *count_reg = tmp_val;
        }
    }
#endif	

#if defined(__FREERTOS)
#else
	nue2_pt_pa2va_unmap((UINT32)count_reg, (UINT32)count_reg_pa);
#endif

}

/*
    NUE2 debug routine

    NUE2 debug routine

    @return None.
*/
VOID nue2_engine_debug_hook1(VOID *p_parm)
{
	NUE2_TEST_PARM *p_parm_test = (NUE2_TEST_PARM *) p_parm;
	volatile UINT32 *reg_int = (UINT32 *) 0xf0d50040;
	volatile UINT32 *reg_reb = (UINT32 *) 0xf0d500d0;
	volatile UINT32 *reg_check = (UINT32 *) 0xf0d500c8;
#if defined(__FREERTOS)
	volatile UINT32 *reg_clk_en = (UINT32 *) 0xf002007c;
	UINT32 reg_value;
#endif
	UINT32 count;
	UINT32 start_count=0x1FFFFFF;

	if ((p_parm_test->flow_ct.ll_test_2 == 1) && (p_parm_test->strip_parm.is_strip == 1)) {
		count = start_count;
nvt_dbg(ERR, "NUE2: %s_%d\r\n", __FUNCTION__, __LINE__);
		while(count > 0) {
#if 1
			if ((*reg_int & (1<<8)) != 0) {
				DBG_ERR( "LLEND: happend. the ll_test_2 has been closed.(count=0x%x)\r\n", (start_count - count));
				break;
			}
#endif
//			nvt_dbg(ERR, "NUE2: %s_%d\r\n", __FUNCTION__, __LINE__);
			*reg_reb = 0xFFFFFFFF;
#if 1
			if ((*reg_check & (1<<29)) != 0) {
				DBG_ERR( "ll_test_2: successful.(count=0x%x)\r\n", (start_count - count));
				break;
			} else {
				//DBG_EMU( "ll_test_2: fail.\r\n");
			}
#endif
			count--;
		}
nvt_dbg(ERR, "NUE2: %s_%d\r\n", __FUNCTION__, __LINE__);
	}

	if (p_parm_test->flow_ct.ll_test_2 == 2) {
#if defined(__FREERTOS)
		dma_disableChannel(DMA_CH_NUE2_0);
		dma_disableChannel(DMA_CH_NUE2_1);
		dma_disableChannel(DMA_CH_NUE2_2);
		dma_disableChannel(DMA_CH_NUE2_3);
		dma_disableChannel(DMA_CH_NUE2_4);
		dma_disableChannel(DMA_CH_NUE2_5);
		dma_disableChannel(DMA_CH_NUE2_6);
		DBG_EMU( "APP(channdel_disable): int_sts(0x%x) \r\n", *reg_int);
		nue2_reg_set_all();
		dma_enableChannel(DMA_CH_NUE2_0);
		dma_enableChannel(DMA_CH_NUE2_1);
		dma_enableChannel(DMA_CH_NUE2_2);
		dma_enableChannel(DMA_CH_NUE2_3);
		dma_enableChannel(DMA_CH_NUE2_4);
		dma_enableChannel(DMA_CH_NUE2_5);
		dma_enableChannel(DMA_CH_NUE2_6);
#else
		nvt_dbg(ERR, "Error..channel_disable..TODO\r\n");
#endif
	}

	if (p_parm_test->flow_ct.clk_en > 0) {
#if defined(__FREERTOS)
		if (p_parm_test->flow_ct.clk_en == 1) {
			reg_value = *reg_clk_en;
			reg_value = reg_value & (~(1<<21));
			*reg_clk_en = reg_value;
			DBG_EMU( "NUE2_CLK_EN:begin to testing reg_clk-0x%x\r\n", *reg_clk_en);
			vos_task_delay_ms(10000);
			reg_value = *reg_clk_en;
			reg_value = reg_value | (1<<21);
			*reg_clk_en = reg_value;
			DBG_EMU( "NUE2_CLK_EN it should be delay to 10 sec. reg_clk=0x%x\r\n", *reg_clk_en);
		} else if (p_parm_test->flow_ct.clk_en == 2) {
			reg_value = *reg_clk_en;
			reg_value = reg_value & (~(1<<21));
			*reg_clk_en = reg_value;
			DBG_EMU( "NUE2_CLK_EN(it should be fail):begin to testing reg_clk-0x%x\r\n", *reg_clk_en);
		} else if (p_parm_test->flow_ct.clk_en == 3) {
			reg_value = *reg_clk_en;
			reg_value = reg_value | (1<<21);
			*reg_clk_en = reg_value;
			DBG_EMU( "NUE2_CLK_EN(enable, ok):begin to testing reg_clk-0x%x\r\n", *reg_clk_en);
		}
#else
		nvt_dbg(ERR, "Error..clock enable..TODO\r\n");
#endif
	}

	
	return;
}

/*
    NUE2 debug routine

    NUE2 debug routine

    @return None.
*/
VOID nue2_engine_debug_hook2(VOID *p_parm)
{
	UINT32 cmp_idx;
	int len;
	NUE2_TEST_PARM *p_parm_test = (NUE2_TEST_PARM *) p_parm;
	char *tmp_buf;
	char *tmp_buf1;
	volatile UINT32 *trigger_count = (UINT32 *) 0xf0d500c0;
	volatile UINT32 *fire_count = (UINT32 *) 0xf0d500c4;
	volatile UINT32 *count_reg = (UINT32 *) 0xf0d50080;
#if defined(__FREERTOS)
#else
	volatile UINT32 *trigger_count_pa = (UINT32 *) 0xf0d500c0;
	volatile UINT32 *fire_count_pa = (UINT32 *) 0xf0d500c4;
	volatile UINT32 *count_reg_pa = (UINT32 *) 0xf0d50080; //b12:switch, b13: only HW no LL command
#endif
	UINT32 tmp_val;

#if defined(__FREERTOS)
#if (NUE2_EMU_DBG_EN == ENABLE)
	volatile UINT32 *reg_520_80 = (UINT32 *) 0xf0d50080;
	volatile UINT32 *reg_520_84 = (UINT32 *) 0xf0d50084;
	volatile UINT32 *reg_520_88 = (UINT32 *) 0xf0d50088;
	volatile UINT32 *reg_520_8c = (UINT32 *) 0xf0d5008c;
	volatile UINT32 *reg_528_84 = (UINT32 *) 0xf0d50084;
	volatile UINT32 *reg_528_88 = (UINT32 *) 0xf0d50088;
	volatile UINT32 *reg_528_8c = (UINT32 *) 0xf0d5008c;
	volatile UINT32 *reg_528_90 = (UINT32 *) 0xf0d50090;
#endif
	volatile UINT32 *ll_err_addr = (UINT32 *) 0xf0d500d0;
	volatile UINT32 *ll_err_cnt = (UINT32 *) 0xf0d500cc;
	
	
	if (p_parm_test->flow_ct.is_terminate >= 1) {
#if defined(_BSP_NA51089_)
		DBG_EMU( "LL_TERMINATE(table info): 0x%x, 0x%x, 0x%x, 0x%x\r\n",
		*reg_528_84,
		*reg_528_88,
		*reg_528_8c,
		*reg_528_90);
#else
		if(nvt_get_chip_id() == CHIP_NA51055) {
			DBG_EMU( "LL_TERMINATE(table info): 0x%x, 0x%x, 0x%x, 0x%x\r\n", 
			*reg_520_80,
			*reg_520_84,
			*reg_520_88,
			*reg_520_8c);
		} else {
			DBG_EMU( "LL_TERMINATE(table info): 0x%x, 0x%x, 0x%x, 0x%x\r\n", 
			*reg_528_84,
			*reg_528_88,
			*reg_528_8c,
			*reg_528_90);
		}
#endif
	}
#endif

#if defined(__FREERTOS)
#else
	trigger_count = (UINT32 *) nue2_pt_pa2va_remap((UINT32) trigger_count_pa, 0x100, 2);
	fire_count = (UINT32 *) nue2_pt_pa2va_remap((UINT32) fire_count_pa, 0x100, 2);
	count_reg = (UINT32 *) nue2_pt_pa2va_remap((UINT32) count_reg_pa, 0x100, 2);
#endif

#if defined(_BSP_NA51089_)
	if (p_parm_test->flow_ct.cnt_is_hw_only == 0) {
		tmp_val = *count_reg;
		if ((tmp_val & (1<<13)) != 0) {
			nvt_dbg(ERR, "NUE2(APP): Error, count_reg(0x%x) bit13 was not 0.\r\n", *count_reg);
		}
		p_parm_test->flow_ct.cnt_hw_ll = *fire_count;
		p_parm_test->flow_ct.cnt_hw_no_ll = 0;
		p_parm_test->flow_ct.cnt_single_hw = *trigger_count;
	} else {
		tmp_val = *count_reg;
		if ((tmp_val & (1<<13)) == 0) {
			nvt_dbg(ERR, "NUE2(LL): Error, count_reg(0x%x) bit13 was 0.\r\n", *count_reg);
		}
		p_parm_test->flow_ct.cnt_hw_ll = *fire_count;
		p_parm_test->flow_ct.cnt_hw_no_ll = *trigger_count;
		p_parm_test->flow_ct.cnt_single_hw = 0;
	}

	tmp_val = *count_reg;
	tmp_val &= (~(1<<12));
	tmp_val &= (~(1<<13));
	*count_reg = tmp_val;
#else	
	if(nvt_get_chip_id() == CHIP_NA51055) {
    } else {
        if (p_parm_test->flow_ct.cnt_is_hw_only == 0) {
            tmp_val = *count_reg;
			if ((tmp_val & (1<<13)) != 0) {
				nvt_dbg(ERR, "NUE2(APP): Error, count_reg(0x%x) bit13 was not 0.\r\n", *count_reg);
			}
            p_parm_test->flow_ct.cnt_hw_ll = *fire_count;
            p_parm_test->flow_ct.cnt_hw_no_ll = 0;
            p_parm_test->flow_ct.cnt_single_hw = *trigger_count;
        } else {
            tmp_val = *count_reg;
			if ((tmp_val & (1<<13)) == 0) {
				nvt_dbg(ERR, "NUE2(LL): Error, count_reg(0x%x) bit13 was 0.\r\n", *count_reg);
			}
            p_parm_test->flow_ct.cnt_hw_ll = *fire_count;
            p_parm_test->flow_ct.cnt_hw_no_ll = *trigger_count;
            p_parm_test->flow_ct.cnt_single_hw = 0;
        }

		tmp_val = *count_reg;
		tmp_val &= (~(1<<12));
		tmp_val &= (~(1<<13));
		*count_reg = tmp_val;
    }
#endif

	if ((p_parm_test->flow_ct.ll_big_buf != 0) && (p_parm_test->flow_ct.ll_buf != 0)) {
		for (cmp_idx = 0; cmp_idx < (NUE2_MAX_RESERVE_BUF_NUM - 1); cmp_idx++) {
			if (g_nue2_dram_outrange == 1) {
				continue;
			}
			nue2_pt_dma_flush_dev2mem(nue2_dram_addr_res[cmp_idx], nue2_dram_size_res[cmp_idx]);
			tmp_buf = (char *) nue2_dram_addr_res[cmp_idx];
			tmp_buf1 = (char *) nue2_dram_addr_res[NUE2_MAX_RESERVE_BUF_NUM - 1];
			DBG_ERR( "WW##...begine to compare ll_golden: cmp_idx(%d) addr(0x%x) (%d %d %d %d) (%d %d %d %d)\r\n", 
						cmp_idx, nue2_dram_addr_res[cmp_idx], tmp_buf[0], tmp_buf[1], tmp_buf[2], tmp_buf[3],
						tmp_buf1[0], tmp_buf1[1], tmp_buf1[2], tmp_buf1[3]);
			len = nue2_cmp_data(nue2_dram_addr_res[cmp_idx], nue2_dram_size_res[NUE2_MAX_RESERVE_BUF_NUM - 1], nue2_dram_addr_res[NUE2_MAX_RESERVE_BUF_NUM - 1], 0, 0);
			if (len < 0) {
				nvt_dbg(ERR, "%s_%d:Error, nue2_cmp_data(): ret(%d)\r\n", __FUNCTION__, __LINE__, len);
			}
		}
	}

	if (p_parm_test->flow_ct.ll_test_2 == 2) {
		nue2_reg_clr_all();
	}

#if defined(__FREERTOS)
	if (p_parm_test->flow_ct.is_ll_next_update == 2) {
		nvt_dbg(ERR, "LL_ERR_ADDR(0x%x) LL_ERR_CNT(0x%x)\r\n", *ll_err_addr, *ll_err_cnt);
	}
#endif

#if defined(__FREERTOS)
#else
	nue2_pt_pa2va_unmap((UINT32)trigger_count, (UINT32)trigger_count_pa);
	nue2_pt_pa2va_unmap((UINT32)fire_count, (UINT32)fire_count_pa);
	nue2_pt_pa2va_unmap((UINT32)count_reg, (UINT32)count_reg_pa);
#endif

}

/*
    NUE2 while loop routine

    NUE2 while loop routine

    @return None.
*/
VOID nue2_engine_loop_frameend(VOID *p_parm)
{
	UINT32 nue2_int_status = 0;
#if defined(__FREERTOS)
    unsigned int count = 0;
#else
    const INT32 time_limit = 3000000;
    struct timeval s_time, e_time;
    INT32 dur_time = 0; // (us)

	do_gettimeofday(&s_time);
#endif

	nue2_int_status = nue2_get_intr_status();
	
#if defined(__FREERTOS)
	while ((nue2_int_status & NUE2_INT_FRMEND) != NUE2_INT_FRMEND) {
#else
	while (((nue2_int_status & NUE2_INT_FRMEND) != NUE2_INT_FRMEND)  && (dur_time < time_limit) ) {
#endif
		nue2_int_status = nue2_get_intr_status();

#if defined(__FREERTOS)
        vos_task_delay_ms(10);
        count++;
		if ((count % 100) == 0) {
			nvt_dbg(WRN, "NUE2 loop delay count=%d\r\n", count);
		}
        if (count > 6000) {
            nvt_dbg(WRN, "NUE2 frame end wait too long(timeout).\r\n");
			nue2_pause();	
			if (g_fill_reg_only == 0) {
            	nue2_close();
            	nue2_open(NULL);
			}
            break;
        }
#else
        do_gettimeofday(&e_time);
        dur_time = (e_time.tv_sec - s_time.tv_sec) * 1000000 + (e_time.tv_usec - s_time.tv_usec);
#endif

	}
}

ER nue2_engine_setmode(NUE2_OPMODE mode, VOID *p_parm_in)
{
	ER er_return=E_OK;
	NUE2_TEST_PARM *p_parm_test = (NUE2_TEST_PARM *) p_parm_in;
	NUE2_PARM *p_parm = (NUE2_PARM *) p_parm_in;

	er_return = nue2_setmode(mode, p_parm);
	if (er_return != E_OK) {
		goto rt_end;
	}

	er_return = nue2_set_burst(p_parm_test->dbg_parm.in_burst_mode, p_parm_test->dbg_parm.out_burst_mode);
	if (er_return != E_OK) {
		goto rt_end;
	}

rt_end:
		
	return er_return;
}

/**
    NUE2 Engine flow

    Start NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_engine_flow(NUE2_TEST_PARM *p_parm)
{
	ER er_return=E_OK;
	UINT64 *ll_buf_tmp=0;
	UINT32 ll_idx;
	UINT32 rst_en;
	UINT64 *ll_buf;
	UINT8 is_trigger;
	UINT8 is_terminate;
	UINT8 is_dma_test;
	UINT8 is_retry_test;
	UINT8 is_fill_reg_only;
	UINT8 is_ll_next_update;
	UINT8 is_bit60;
	UINT32 ll_fill_reg_num;
	UINT32 ll_fill_num;
	UINT8 is_switch_dram;
	volatile UINT32 *reg_clk_en = (UINT32 *) 0xf002007c;
	UINT32 tmp_val;
	UINT32 loop_mode;
	UINT32 loop_time;

	rst_en = (UINT32) p_parm->flow_ct.rst_en;
	is_terminate = (UINT8) p_parm->flow_ct.is_terminate;
	is_dma_test = (UINT8) p_parm->flow_ct.is_dma_test;
	is_fill_reg_only = (UINT8) p_parm->flow_ct.is_fill_reg_only;
	is_ll_next_update = (UINT8) p_parm->flow_ct.is_ll_next_update;
	is_bit60 = (UINT8) p_parm->flow_ct.is_bit60;
	ll_fill_reg_num = (UINT32) p_parm->flow_ct.ll_fill_reg_num;
	ll_fill_num = (UINT32) p_parm->flow_ct.ll_fill_num;
	is_switch_dram = (UINT32) p_parm->flow_ct.is_switch_dram;
	loop_time = (UINT32) p_parm->flow_ct.loop_time;
	loop_mode = (UINT32) p_parm->flow_ct.loop_mode;

	is_retry_test = 0;

RETRY_TEST:
	
	ll_buf = (UINT64 *) p_parm->flow_ct.ll_buf;
	ll_buf_tmp = (UINT64 *) ll_buf;

#if (NUE2_DEBUG_LL==ENABLE)
	DBG_EMU( "WW###ll_buf = 0x%x\r\n", (UINT32)(ll_buf));
#endif

	p_parm->strip_parm.is_strip = 0;
    p_parm->strip_parm.s_step = 0;
	p_parm->strip_parm.s_posi = 0;
	//initiate strip parm
	er_return = nue2_init_strip_parm(p_parm);
	if (er_return != E_OK) {
		nvt_dbg(ERR, "Error! fail to initiate the stripe parameter.\r\n");
		//don't need to retern fail, just do s_num=1
	}

	ll_idx = 0;
	is_trigger = 0;
	p_parm->strip_parm.s_posi = 0;
	//s_num: upper layer require (the strip number)
	//s_posi: strip position
	while(p_parm->strip_parm.s_posi < p_parm->strip_parm.scl_width) {


		p_parm->strip_parm.s_posi += p_parm->strip_parm.s_step;

		if (p_parm->strip_parm.s_posi > p_parm->strip_parm.scl_width) {
			p_parm->strip_parm.s_posi = p_parm->strip_parm.scl_width;
		}
		
#if (NUE2_DEBUG_LL==ENABLE)
		DBG_EMU( "WW### ll_mode: is_strip=%d\r\n", p_parm->strip_parm.is_strip);
#endif

		if (p_parm->strip_parm.scl_width == p_parm->strip_parm.s_posi) {
			is_trigger = 1;
		}

		if (p_parm->strip_parm.is_strip == 1) {
			nue2_set_strip_parm(&p_parm->strip_parm, &ll_buf_tmp, is_bit60, p_parm->flow_ct.ll_base_addr);
		}

		if (ll_buf != 0) {
			nue2_set_dmain_lladdr(nue2_pt_va2pa((UINT32) ll_buf));
		}

		// Clear engine interrupt status
		//nue2_enable_int(DISABLE, NUE2_INTE_ALL_528); //dummy
#if defined(_BSP_NA51089_)
		nue2_clr_intr_status(NUE2_INT_ALL_528);
#else
		if(nvt_get_chip_id() == CHIP_NA51055) {
			nue2_clr_intr_status(NUE2_INT_ALL);
		} else {
			nue2_clr_intr_status(NUE2_INT_ALL_528);
		}
#endif

		if (p_parm->flow_ct.interrupt_en == 1) {
#if defined(_BSP_NA51089_)
			nue2_enable_int(ENABLE, NUE2_INTE_ALL_528);
#else
			if(nvt_get_chip_id() == CHIP_NA51055) {
				nue2_enable_int(ENABLE, NUE2_INTE_ALL);
			} else {
				nue2_enable_int(ENABLE, NUE2_INTE_ALL_528);
			}
#endif
			if (is_dma_test == 1) {
				nvt_dbg(ERR, "ERROR, dma test can't be at interrupt_en=1.\r\n");
				is_dma_test = 0;
			}
		} else {
#if defined(_BSP_NA51089_)
			nue2_enable_int(DISABLE, NUE2_INTE_ALL_528);
#else
			if(nvt_get_chip_id() == CHIP_NA51055) {
				nue2_enable_int(DISABLE, NUE2_INTE_ALL);
			} else {
				nue2_enable_int(DISABLE, NUE2_INTE_ALL_528);
			}
#endif
			if (is_dma_test == 1) {
				nue2_pt_dma_test(DISABLE);
				DBG_EMU( "dma is disable.\r\n");
			}
		}

#if (NUE2_DEBUG_STRIP == ENABLE)
			nue2_reg_dump_print();
#endif

		if (p_parm->flow_ct.rand_ch_en == ENABLE) {
			nue2_pt_ch_disable();
		}

		if (loop_mode == 2) {
            tmp_val = *reg_clk_en;
            tmp_val |= (1<<21);
            *reg_clk_en = tmp_val;
        }

LOOP_TEST:

        if (loop_mode == 1) {
            tmp_val = *reg_clk_en;
            tmp_val |= (1<<21);
            *reg_clk_en = tmp_val;
        }


		if (ll_buf == 0) {
			p_parm->reg_func.nue2_engine_debug_hook((VOID *) p_parm);

			if (is_fill_reg_only == 1) {
				DBG_EMU( "nue2_start()-exit, only fill register, no trigger engine.(no ll)\r\n");
				return er_return;
			}


			p_parm->reg_func.nue2_start();
			p_parm->reg_func.nue2_engine_debug_hook1((VOID *) p_parm);

#if (NUE2_DEBUG_LL==ENABLE)
			DBG_EMU( "ll_mode:disable..\r\n");
#endif
		} else {
#if (NUE2_DEBUG_LL==ENABLE)
			DBG_EMU( "ll_mode: enable..\r\n");
#endif


			if ((UINT32)p_parm->strip_parm.s_posi == p_parm->strip_parm.scl_width) { //last one
				if (p_parm->strip_parm.s_num == 1) { //only one
#if (NUE2_DEBUG_LL==ENABLE)
					DBG_EMU( "WW### ll_mode: only one, ll_idx=%d B\r\n", ll_idx);
#endif
					if (p_parm->flow_ct.ll_big_buf == 0) {
						if (is_bit60 == 0) {
							nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, is_ll_next_update);
						} else {
							nue2_setdata_ll(&ll_buf_tmp, 0xc0, 0, is_bit60);
						}
						nue2_setdata_exit_ll(&ll_buf_tmp, ll_idx);
						nue2_flush_buf_ll(ll_buf, ll_buf_tmp);
						p_parm->flow_ct.ll_buf_end = ll_buf_tmp;
					} else {
						//DBG_EMU( "LL_SWITCH_B: ll_buf(addr: 0x%x)\r\n", ll_buf_tmp);
						nue2_prepare_ll_buffer(&ll_buf_tmp, is_bit60, ll_fill_reg_num, ll_fill_num, is_switch_dram, p_parm->flow_ct.is_ll_next_update,
												0xf0d50008, 0xf0d50018, nue2_dram_addr_res, nue2_dram_size_res, NUE2_MAX_RESERVE_BUF_NUM, 
												p_parm->flow_ct.ll_base_addr);
					
						nue2_flush_buf_ll(ll_buf, ll_buf_tmp);
						nue2_flush_buf_ll((UINT64 *)NUE2_SWITCH_DRAM_1((UINT32)ll_buf), (UINT64 *)NUE2_SWITCH_DRAM_1((UINT32)ll_buf_tmp));
						nue2_flush_buf_ll((UINT64 *)NUE2_SWITCH_DRAM_2((UINT32)ll_buf), (UINT64 *)NUE2_SWITCH_DRAM_2((UINT32)ll_buf_tmp));
						p_parm->flow_ct.ll_buf_end = ll_buf_tmp;
						
						//DBG_EMU( "LL_SWITCH: ll_buf(0x%x) ll_buf_end(0x%x)\r\n", (unsigned int) ll_buf, (unsigned int) p_parm->flow_ct.ll_buf_end);
					}

#if (NUE2_DEBUG_LL==ENABLE)
					DBG_EMU( "WW### ll_mode: only one, ll_idx=%d A\r\n", ll_idx);
#endif

				} else {
#if (NUE2_DEBUG_LL==ENABLE)
					DBG_EMU( "WW### ll_mode: last_one, ll_idx=%d\r\n", ll_idx);
#endif
					if (is_bit60 == 0) {
						nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, is_ll_next_update);
					}
					nue2_setdata_exit_ll(&ll_buf_tmp, ll_idx);
					nue2_flush_buf_ll(ll_buf, ll_buf_tmp);
					p_parm->flow_ct.ll_buf_end = ll_buf_tmp;
				}

				if (is_fill_reg_only == 1) {
					DBG_EMU( "nue2_start()-exit, only fill register, no trigger engine.(ll)\r\n");
					return er_return;
				}

			
				p_parm->reg_func.nue2_engine_debug_hook((VOID *) p_parm);
				p_parm->reg_func.nue2_ll_start();
				p_parm->reg_func.nue2_engine_debug_hook1((VOID *) p_parm);

				ll_idx++;
			} else {
#if (NUE2_DEBUG_LL==ENABLE)
					DBG_EMU( "WW### ll_mode: ll_idx=%d\r\n", ll_idx);
#endif
				if (is_bit60 == 0) {
					nue2_setdata_end_ll(&ll_buf_tmp, ll_idx, is_ll_next_update);
				}
				ll_idx++;
				//don't need to trigger when it is not the last one
				continue;
			}
		}

		if (is_terminate >= 1 && is_trigger == 1) {
			nue2_ll_terminate(ENABLE);
			if (is_terminate == 1) {
				rst_en = 0; //can't enable hw_rst.
				is_terminate = 0;
				is_retry_test = 1;
				DBG_ERR( "ll_mode: terminate() was triggerred.\r\n");
				//initialization
				if (p_parm->flow_ct.interrupt_en == 1) {
					if (nue2_engine_setmode(NUE2_OPMODE_USERDEFINE, (VOID *) p_parm) != E_OK) {
						nvt_dbg(ERR, "nue2_setmode error ..\r\n");
					} else {
						nvt_dbg(IND, "Set to NUE2 Driver ... done\r\n");
					}
				} else {
					if (nue2_engine_setmode(NUE2_OPMODE_USERDEFINE_NO_INT, (VOID *) p_parm) != E_OK) {
						nvt_dbg(ERR, "nue2_setmode error ..\r\n");
					} else {
						nvt_dbg(IND, "Set to NUE2 Driver ... done\r\n");
					}
				}

				ll_buf_tmp = (UINT64 *) ll_buf; //marked it ok for terminate
				p_parm->strip_parm.is_strip = 0;
				p_parm->strip_parm.s_step = 0;
				p_parm->strip_parm.s_posi = 0;
				//initiate strip parm
				er_return = nue2_init_strip_parm(p_parm);
				if (er_return != E_OK) {
					nvt_dbg(ERR, "Error! fail to initiate the stripe parameter.\r\n");
					//don't need to retern fail, just do s_num=1
				}

				ll_idx = 0;
				is_trigger = 0;
				p_parm->strip_parm.s_posi = 0;
				//end initilization
			} else {
				is_terminate = 0;
				DBG_EMU( "ll_mode: terminate() was triggerred.\r\n");
			}
		}

		if (rst_en > 0 && is_trigger == 1) {
#if (NUE2_DEBUG_LL==ENABLE)
			nvt_dbg(ERR, "Error , ll_mode: enter into rst_en=%d..\r\n", rst_en);
#endif
#if defined(__FREERTOS)
			if (rst_en == 6) {
				nue2_engine_rst(1 + (rand() % 2));
			} else {
				nue2_engine_rst(rst_en);
			}
#else
			nvt_dbg(ERR, "Error..rst_en..TODO\r\n");
#endif
			rst_en = 0; //can't enable hw_rst.
			is_terminate = 0;
			DBG_EMU( "hw_rst was triggerred.\r\n");
			//initialization
			if (p_parm->flow_ct.interrupt_en == 1) {
				if (nue2_engine_setmode(NUE2_OPMODE_USERDEFINE, (VOID *) p_parm) != E_OK) {
					nvt_dbg(ERR, "nue2_setmode error ..\r\n");
				} else {
					nvt_dbg(IND, "Set to NUE2 Driver ... done\r\n");
				}
			} else {
				if (nue2_engine_setmode(NUE2_OPMODE_USERDEFINE_NO_INT, (VOID *) p_parm) != E_OK) {
					nvt_dbg(ERR, "nue2_setmode error ..\r\n");
				} else {
					nvt_dbg(IND, "Set to NUE2 Driver ... done\r\n");
				}
			}

			ll_buf_tmp = (UINT64 *) ll_buf; //marked it ok for terminate
			p_parm->strip_parm.is_strip = 0;
			p_parm->strip_parm.s_step = 0;
			p_parm->strip_parm.s_posi = 0;
			//initiate strip parm
			er_return = nue2_init_strip_parm(p_parm);
			if (er_return != E_OK) {
				nvt_dbg(ERR, "Error! fail to initiate the stripe parameter.\r\n");
				//don't need to retern fail, just do s_num=1
			}

			ll_idx = 0;
			is_trigger = 0;
			p_parm->strip_parm.s_posi = 0;

			is_retry_test = 0;
			goto RETRY_TEST;
		}

#if (NUE2_DEBUG_LL==ENABLE)
		DBG_EMU( "WW### ll_mode: wait interrupt or polling.\r\n");
#endif

		if (p_parm->flow_ct.interrupt_en == 1) {
			if (ll_buf == 0) {
				p_parm->reg_func.nue2_wait_frameend(FALSE);
			} else {
#if (NUE2_DEBUG_LL==ENABLE)
		DBG_EMU( "WW### ll_mode: wait interrupt B\r\n");
#endif
				p_parm->reg_func.nue2_wait_ll_frameend(FALSE);
#if (NUE2_DEBUG_LL==ENABLE)
		DBG_EMU( "WW### ll_mode: wait interrupt A\r\n");
#endif
			}
		} else {
			if (ll_buf == 0) {
				p_parm->reg_func.nue2_engine_loop_frameend((VOID *) p_parm);

			} else {
				p_parm->reg_func.nue2_engine_loop_llend((VOID *) p_parm);
			}
			DBG_EMU( "WW### loop frameend after...\n");
		}

		p_parm->reg_func.nue2_engine_debug_hook2((VOID *) p_parm);

		if (loop_mode == 1) {            
			tmp_val = *reg_clk_en;
            tmp_val &= (~(1<<21));
            *reg_clk_en = tmp_val;
            //dbg_emu("NUE2_CLOSE: nue2_clk_reg=0x%x\r\n", *reg_set);
        }

        if (loop_mode != 0) {
            vos_task_delay_ms(50);
            loop_time--;
            if (loop_time > 0) {
                goto LOOP_TEST;
            }
        }

        if (loop_mode == 2) {
            tmp_val = *reg_clk_en;
            tmp_val &= (~(1<<21));
            *reg_clk_en = tmp_val;
            //dbg_emu("NUE2_CLOSE: nue2_clk_reg=0x%x\r\n", *reg_set);
        }

		if (is_retry_test >= 1) {
			//is_retry_test = 0;
			if (is_retry_test >= 3) {
				is_retry_test = 0;
			} else {
				is_retry_test++;
				goto RETRY_TEST;
			}
		}

#if (NUE2_FPGA == ENABLE)
		//nue2_set_dmaio_addr(p_parm->dmaio_addr, NUE2_OUT_ADDR); //TODO by WW
		er_return = nue2_set_dmaio_addr(p_parm->dmaio_addr);
		if (er_return != E_OK) {
			nvt_dbg(ERR, "Error nue2_set_dmaio_addr(), ret=%d\r\n", (UINT32) er_return);
		}
#endif
	}

	return er_return;
}


/* NUE2 */
extern char end[];
UINT32 NUE2_ENGINE_HOUSE[0x94 / 4 + 1] = {0x0};
NUE2_TEST_PARM nue2_param = {0};

#if defined(__FREERTOS)
#define NUE2_MAX_STR_LEN 256
extern void ive_set_bringup(UINT8 enable);
extern int nvt_kdrv_ipp_api_ive_test(void);
extern VOID ive_set_dram_mode(UINT32 dram_mode); //2:dram_2, 1:dram_1
extern VOID ive_set_heavy_mode(UINT8 mode); //0:heavy, 1:channel disable/enable
extern VOID ive_set_ll_mode_en(UINT8 is_en);
extern VOID ive_set_heavy_func(UINT8 heavy_mode, UINT8 dram_mode);
extern VOID ive_rst_func(UINT8 mode);

UINT32 cStrLen = NUE2_MAX_STR_LEN;
CHAR cStartStr[NUE2_MAX_STR_LEN], cEndStr[NUE2_MAX_STR_LEN];//, cRegStr[StringMaxLen];
INT32 iPatSNum = 0, iPatENum = 0;
#endif

static UINT8 g_gld_cmp=1;
static UINT8 g_is_strip=0;
static UINT32 g_s_step=0;
static UINT32 g_s_posi=0;
static UINT8 g_ll_terminate=0;
static UINT8 g_dma_test=0;
static UINT8 g_is_ll_next_update=0;
static UINT8 g_rand_burst=0;
static UINT8 g_is_reg_dump=0;
static UINT8 g_ll_test_2=0;
static UINT8 g_no_stop=0;
static UINT8 g_dump_ll_buf=1;
static UINT8 g_ll_big_buf=1;
static UINT8 g_auto_clk=0;
static UINT8 g_is_bit60=0;
static UINT32 g_ll_base_addr=0;
static UINT8 g_wp_sel_id=0;
static UINT32 g_pri_mode=2;
static UINT8 g_clock=1;
static const UINT32 g_clock_val[4]={600, 480, 320, 240};
static UINT32 g_ll_fill_reg_num=1;
static UINT32 g_ll_fill_num=5; //fill one
static UINT32 g_is_switch_dram=0;
static UINT8 g_rand_comb=0;
static UINT8 g_is_four_check=1;
static UINT8 g_clk_en=0;
static UINT8 g_sram_down=0;
static UINT8 g_reg_rw=0;
static UINT32 g_loop_time=500;
static UINT32 g_loop_mode=0;


UINT8 g_nue2_dram_outrange=0;
UINT8 g_nue2_dram_mode=2;
UINT32 g_mem_base, g_mem_base_2;
UINT32 g_mem_size, g_mem_size_2;

UINT32 nue2_in_width, nue2_in_height;
UINT32 nue2_h_scl_size, nue2_v_scl_size;
UINT32 nue2_sub_in_width, nue2_sub_in_height;

const unsigned int g_mem_size_check = 0x8000000;

#if defined(__FREERTOS)
int nvt_kdrv_ai_module_test(PAI_INFO pmodule_info, unsigned char argc, char pargv[][10])
#else
int nvt_kdrv_ai_module_test(PAI_INFO pmodule_info, unsigned char argc, char **pargv)
#endif
{
#if defined(__FREERTOS)
	FST_FILE file;
	unsigned int size;
	unsigned long tmp_long;
	INT32 fstatus;
	//unsigned int addr;
#else
	//mm_segment_t old_fs;
	//struct file *fp;
	int fd = 0;
#endif
	int len = 0;
	//unsigned char *pbuffer;
#if defined(__FREERTOS)
#else
	struct  vos_mem_cma_info_t buf_info = {0};
	VOS_MEM_CMA_HDL vos_mem_id;
#endif
	int ret = 0;
	unsigned int index = 0, index_start, index_end;
	int emu_func = 0;
	unsigned int mem_base = 0;
	unsigned int mem_end = 0;
#if defined(__FREERTOS)
	unsigned int mem_size;
#endif
	unsigned int mem_tmp_addr = 0;
	char io_path[100];
	char tmp[100];
#if 0
	unsigned int tmp_val0=0, tmp_val1=0, i;
#else
	unsigned int tmp_val1=0, i;
#endif
	unsigned int total_layer_num = 0;
	unsigned int ll_en = 0, ll_base = 0;//, ll_addr = 0;
	unsigned int ll_out_addr[48] = {0};
	unsigned int ll_out_size[48] = {0};
	//NT98520_NUE_REG_STRUCT *p_nue_reg = NULL;
	NT98520_NUE2_REG_STRUCT *Nue2Reg = NULL;
	NT98528_NUE2_REG_STRUCT *Nue2Reg_528 = NULL;
	UINT8 is_fail_case = 0;

	/* NUE */
	/*UINT32 nue_dram_in_addr0 = 0, nue_dram_in_addrSv = 0, nue_dram_in_addrAlpha = 0, nue_dram_in_addrRho = 0;
	UINT32 nue_dram_in_addr1 = 0, nue_dram_in_addrRoi = 0, nue_dram_in_addrKQ = 0;
	UINT32 nue_dram_out_addr = 0, nue_out_size = 0, nue_in_size = 0;*/
	/* NUE2 */
	
#if 0
	UINT32 nue2_Total_Output_Size;
#endif
    UINT32 word_addr0 = 0, word_addr1 = 0, word_addr2 = 0;
    UINT32 byte_0 = 0,byte_1 = 0,byte_2 = 0;
    //UINT32 AddrDiff = 0x0;

#if CACHE_DBG
	UINT32* cache_tmp_buf = NULL;
	UINT32 cache_i = 0;
#endif
	UINT64 *ll_buf;
	NUE2_OPENOBJ reg_isr_callback;
	UINT32 buf_idx;
	UINT32 buf_start;

#if defined(__FREERTOS)
	if (g_bringup_en >= 1) {
		iPatSNum = 0;
		iPatENum = 0;
	} else {
		srand((unsigned) hwclock_get_counter());

		DBG_EMU( "******* CNN / NUE / NUE2 Verification*******\r\n");
		nvt_dbg(ERR, "Please enter the start No.: ");
		cStrLen = NUE2_MAX_STR_LEN;
		uart_getString(cStartStr, &cStrLen);
		iPatSNum = atoi(cStartStr);
		nvt_dbg(ERR, "\r\n");

		nvt_dbg(ERR, "Please enter the end No.: ");
		cStrLen = NUE2_MAX_STR_LEN;
		uart_getString(cEndStr, &cStrLen);
		iPatENum = atoi(cEndStr);
		nvt_dbg(ERR, "\r\n");
	}
#else
	nvt_dbg(ERR, "Error, TOTO here ... 1\r\n");
#endif

	nue2_dram_in_addr0 = 0;nue2_dram_in_addr1 = 0; nue2_dram_in_addr2 = 0;
	nue2_dram_in_size0 = 0; nue2_dram_in_size1 = 0; nue2_dram_in_size2 = 0;
	in_width_ofs_0 = 0; in_width_ofs_1 = 0; in_width_ofs_2 = 0;
	nue2_dram_out_addr0 = 0; nue2_dram_out_addr1 = 0; nue2_dram_out_addr2 = 0;
	nue2_dram_out_size0=0;nue2_dram_out_size1 = 0;nue2_dram_out_size2 = 0;
	nue2_dram_gld_addr0 = 0; nue2_dram_gld_addr1 = 0; nue2_dram_gld_addr2 = 0;
	nue2_dram_gld_size0 = 0; nue2_dram_gld_size1 = 0; nue2_dram_gld_size2 = 0;
	
	//UINT32 nue_dram_in_addrLL = 0

	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -1;
	}

	if (pargv == NULL) {
		nvt_dbg(ERR, "invalid reg = null\n");
		return -1;
	}

	nvt_dbg(IND, "ideal mem size = 0x%08X\n", g_mem_size_check);
	// Allocate memory

#if defined(__FREERTOS)
	nue2_set_base_addr(0xF0D50000);
    nue2_create_resource(NULL, 0); //before nue2_open()

	nue_set_base_addr(0xF0C60000);
    nue_create_resource(NULL, 0); //before nue_open()
#endif

	// set emulation related info
	if (strcmp(pargv[0], "cnn") == 0) {
		emu_func = 0;
		// insert your open here
	} else if (strcmp(pargv[0], "nue") == 0) {
		emu_func = 1;
		nue_open(NULL);
	} else if (strcmp(pargv[0], "nue2") == 0) {
		emu_func = 2;
		if (g_interrupt_en == 1) {
			//nue2_open((NUE2_OPENOBJ *) nue2_engine_isr);
			reg_isr_callback.clk_sel = 480;
			reg_isr_callback.fp_isr_cb = nue2_engine_isr;
			#if defined(__FREERTOS)
			nue2_pt_interrupt_en(1);
			#endif
			nue2_open((NUE2_OPENOBJ *) &reg_isr_callback);
		} else {
			#if defined(__FREERTOS)
			nue2_pt_interrupt_en(0);
			#endif
			nue2_open((NUE2_OPENOBJ *) NULL);
		}
		if (g_reg_rw == 1) {
			nue2_check_reg_rw();
		}
	} else if (strcmp(pargv[0], "sdk") == 0) {
		emu_func = 3;
	} else if (strcmp(pargv[0], "nuell") == 0) {
		emu_func = 1;
		ll_en = 1;
		nue_open(NULL);
	} else  {
		nvt_dbg(ERR, "unknown input engine type %s\n", pargv[0]);
		return -1;
	}

	nvt_dbg(ERR, "WW###:input emu_func=%d\n", emu_func);

#if defined(__FREERTOS)
	//#define POOL_ID_APP_ARBIT       0
	//#define POOL_ID_APP_ARBIT2      1

	if (g_nue2_dram_mode == 2) {
		mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
		mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
	} else {
		mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT);
		mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT);
	}

	g_mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT);
	g_mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT);
	g_mem_base_2 = OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
	g_mem_size_2 = OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
	
	if (mem_size < g_mem_size_check) {
		nvt_dbg(ERR, "Error, the memory size is not enough for NUE2 validation.(0x%x vs 0x%x)\r\n",
						mem_size, g_mem_size_check);
		return -1;
	}

	DBG_EMU( "WW##, non-mem_base=0x%x\r\n", mem_base);

	if (emu_func == 1) { //NUE
		mem_base = dma_getNonCacheAddr(mem_base);
	} else {
		mem_base = dma_getCacheAddr(mem_base);
	}

	DBG_EMU( "WW##, mem_base=0x%x\r\n", mem_base);
	DBG_EMU( "WW##, mem_size=0x%x\r\n", mem_size);

	mem_end = mem_base + g_mem_size_check;

	if (g_is_four_check) {
		//memset((void *)(mem_base), NUE2_MEMSET_VAL, 4);
		mem_base = mem_base + 4;
	}
#else
	if (emu_func == 1) { //NUE
		DBG_EMU( "WW##,non-cachable address\r\n");
		if (0 != vos_mem_init_cma_info(&buf_info, VOS_MEM_CMA_TYPE_NONCACHE, g_mem_size_check)) { //VOS_MEM_CMA_TYPE_NONCACHE, VOS_MEM_CMA_TYPE_CACHE
			nvt_dbg(ERR, "vos_mem_init_cma_info: init buffer fail. \r\n");
			return -1;
		} else {
			vos_mem_id = vos_mem_alloc_from_cma(&buf_info);
			if (NULL == vos_mem_id) {
				nvt_dbg(ERR, "get buffer fail\n");
				return -1;
			}
		}
	} else {
		DBG_EMU( "WW##,cachable address\r\n");
		if (0 != vos_mem_init_cma_info(&buf_info, VOS_MEM_CMA_TYPE_CACHE, g_mem_size_check)) { //VOS_MEM_CMA_TYPE_NONCACHE, VOS_MEM_CMA_TYPE_CACHE
			nvt_dbg(ERR, "vos_mem_init_cma_info: init buffer fail. \r\n");
			return -1;
		} else {
			vos_mem_id = vos_mem_alloc_from_cma(&buf_info);
			if (NULL == vos_mem_id) {
				nvt_dbg(ERR, "get buffer fail\n");
				return -1;
			}
		}
	}
	mem_base = (UINT32)buf_info.vaddr;  //cache and non-cachable decided by VOS_MEM_CMA_TYPE_NONCACHE/VOS_MEM_CMA_TYPE_CACHE
	mem_end = mem_base + g_mem_size_check;
#endif


#if defined(__FREERTOS)
	index_start = (unsigned int) iPatSNum;
	index_end = (unsigned int) iPatENum;
#else
	if (kstrtouint (pargv[1], 10, &index_start)) {
        nvt_dbg(ERR, "invalid debug_layer value:%s\n", pargv[1]);
        return -1;
    }
	if (kstrtouint (pargv[2], 10, &index_end)) {
        nvt_dbg(ERR, "invalid debug_layer value:%s\n", pargv[2]);
        return -1;
    }
	nvt_dbg(ERR, "index_start=%d, index_end=%d\r\n", index_start, index_end);

	kstrtoint(pargv[1], 0, &index_start);
	kstrtoint(pargv[2], 0, &index_end);
#endif

	nvt_dbg(ERR, "WW###:input s=%d, e=%d\n", index_start, index_end);

	if (ll_en == 1) {
		ll_base = mem_base;
		//ll_addr = ll_base;
		//mem_base += LL_ADDR_SIZE;
		//mem_tmp_addr = mem_base;
	}


	if (emu_func == 3) {
		total_layer_num = index_end;
		index_end = index_start;
		DBG_EMU( "start sdk emulation, test model = %d, test layer num = %d, \n", index_start, total_layer_num);
	}
	else
		DBG_EMU( "start emu, start = %d, end = %d, emu_func = %d\n", index_start, index_end, emu_func);
	// start emulation

	if (g_bringup_en >= 1) {
		index_start = 0;
		if (g_bringup_en == 2) {
			index_end = (sizeof(nue2_bringup_table_all)/sizeof(UINT32)) - 1;
        } else {
			index_end = (sizeof(nue2_bringup_table)/sizeof(UINT32)) - 1;
		}
	}

	for (index = index_start; index <= index_end; index++) {
		is_fail_case = 0;

		if (g_bringup_en >= 1) {
			if (g_bringup_en == 2) {
				if (index >= (sizeof(nue2_bringup_table_all)/sizeof(UINT32))) {
					break;
				}
			} else {
				if (index >= (sizeof(nue2_bringup_table)/sizeof(UINT32))) {
					break;
				}
			}
		}

		if (g_no_stop == 1) {
			if (index == index_end) {
				index = index_start;
			}
		}

#if defined(_BSP_NA51089_)
		if (index == 5569) {
			index = 13833;
			nvt_dbg(ERR, "WW# by-pass cases from 5569 ~ 13832 for CHIP:528.\r\n");
		}
#else
		if(nvt_get_chip_id() == CHIP_NA51055) {
		} else {
			if (index == 5569) {
				index = 13833;
				nvt_dbg(ERR, "WW# by-pass cases from 5569 ~ 13832 for CHIP:528.\r\n");
			}
		}
#endif


		if (emu_func == 0) {
			// CNN process
		} else if (emu_func == 1) {
			if (emu_nue(index, index_start, index_end, mem_base, g_mem_size_check, mem_end, ll_en, ll_out_addr, ll_out_size) != 0) {
				nvt_dbg(ERR, "[NUE] pattern %d has problem! skip this pattern\n", index);
				continue;
			}
		} else if (emu_func == 2) {
			// NUE2 process
			nvt_dbg(IND, "emu func = NUE2, pattern id %d\n", nue2_get_index(index));
			//===================================================================================================
			// load register
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\nue2g%d.dat", nue2_get_index(index), nue2_get_index(index));

			file = FileSys_OpenFile(io_path, FST_OPEN_READ);
			if (file == 0) {
				nvt_dbg(ERR, "Invalid file: %s\r\n", io_path);
				continue;
			}
#else
			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//nue2g%d.dat", nue2_get_index(index), nue2_get_index(index));
			/*
			fp = filp_open(io_path, O_RDONLY, 0);
			if (IS_ERR_OR_NULL(fp)) {
				nvt_dbg(ERR, "failed in file open:%s\n", io_path);
				return -1;
			}
			old_fs = get_fs();
			set_fs(get_ds());
			*/
			fd = vos_file_open(io_path, O_RDONLY, 0);
			if ((VOS_FILE)(-1) == fd) {
				nvt_dbg(ERR, "failed in file open:%s\r\n", io_path);
				continue;
			}
#endif

			// parsing registers
			for (i=0;i<0x8c/4;i++){    //???! reg_num
				memset(tmp, 0, 14*sizeof(char));
#if defined(__FREERTOS)
				size = 4*sizeof(char);
				fstatus = FileSys_ReadFile(file, (UINT8 *)tmp, (UINT32 *) &size, 0, NULL);
				if (fstatus != 0) {
					nvt_dbg(ERR, "%s_%d:FileSys_ReadFile. size=%d sts=%d\r\n", __FILE__, __LINE__, size, fstatus);
					//return -1;
				}
				//tmp_long = strtoul(tmp, NULL, 16);  //4-byte offset
				//nvt_dbg(ERR, "WW###: i=%d, tmp_long=%d tmp=%s\n", i, tmp_long, tmp);

				size = 8*sizeof(char);
				fstatus = FileSys_ReadFile(file, (UINT8 *)tmp, (UINT32 *) &size, 0, NULL);
				if (fstatus != 0) {
					nvt_dbg(ERR, "%s_%d:FileSys_ReadFile. size=%d sts=%d\r\n", __FILE__, __LINE__, size, fstatus);
                    //return -1;
                }
				tmp_long = strtoul(tmp, NULL, 16);  //8-byte value

				tmp_val1 = (unsigned int) tmp_long;
				size = 2*sizeof(char);
				fstatus = FileSys_ReadFile(file, (UINT8 *)tmp, (UINT32 *) &size, 0, NULL);
				if (fstatus != 0) {
					nvt_dbg(ERR, "%s_%d:FileSys_ReadFile. size=%d sts=%d\r\n", __FILE__, __LINE__, size, fstatus);	
                    //return -1;
                }
#else
				//len = vfs_read(fp, (void*)tmp, 4*sizeof(char), &fp->f_pos);
				len = vos_file_read(fd, (void *)tmp, 4*sizeof(char));
				if (len == 0)
					break;
#if 0
				kstrtouint(tmp, 16, &tmp_val0);  //4-byte offset
#endif
				//len = vfs_read(fp, (void*)tmp, 8*sizeof(char), &fp->f_pos);
				len = vos_file_read(fd, (void *)tmp, 8*sizeof(char));
				kstrtouint(tmp, 16, &tmp_val1);  //8-byte value
				//len = vfs_read(fp, (void*)tmp, 2*sizeof(char), &fp->f_pos);
				len = vos_file_read(fd, (void *)tmp, 2*sizeof(char));
#if 0
				nvt_dbg(IND, "read reg file:ofs=%04X val=%08X\n", tmp_val0, tmp_val1);
#endif
#endif
				NUE2_ENGINE_HOUSE[i] = tmp_val1;
			}
#if defined(__FREERTOS)
			FileSys_CloseFile(file);
#else
			//filp_close(fp, NULL);
			//set_fs(old_fs);
			vos_file_close(fd);
#endif
			Nue2Reg = (NT98520_NUE2_REG_STRUCT*)((unsigned int)NUE2_ENGINE_HOUSE);
			Nue2Reg_528 = (NT98528_NUE2_REG_STRUCT*)((unsigned int)NUE2_ENGINE_HOUSE);


			DBG_EMU( "WW###: 1 here\n");

			//===================================================================================================
			// Load input
            // calculate input address
            mem_tmp_addr = mem_base;

#if defined(_BSP_NA51089_)
			nue2_in_width = Nue2Reg_528->NUE2_Register_0044.bit.NUE2_IN_WIDTH;
			nue2_in_height = Nue2Reg_528->NUE2_Register_0044.bit.NUE2_IN_HEIGHT;
			nue2_h_scl_size = Nue2Reg_528->NUE2_Register_0058.bit.NUE2_H_SCL_SIZE;
			nue2_v_scl_size = Nue2Reg_528->NUE2_Register_0058.bit.NUE2_V_SCL_SIZE;
			nue2_sub_in_width = Nue2Reg_528->NUE2_Register_005c.bit.NUE2_SUB_IN_WIDTH;
			nue2_sub_in_height = Nue2Reg_528->NUE2_Register_005c.bit.NUE2_SUB_IN_HEIGHT;
#else
			if(nvt_get_chip_id() == CHIP_NA51055) {
				nue2_in_width = Nue2Reg->NUE2_Register_0044.bit.NUE2_IN_WIDTH;
				nue2_in_height = Nue2Reg->NUE2_Register_0044.bit.NUE2_IN_HEIGHT;
				nue2_h_scl_size = Nue2Reg->NUE2_Register_0058.bit.NUE2_H_SCL_SIZE;
				nue2_v_scl_size = Nue2Reg->NUE2_Register_0058.bit.NUE2_V_SCL_SIZE;
				nue2_sub_in_width = Nue2Reg->NUE2_Register_005c.bit.NUE2_SUB_IN_WIDTH;
				nue2_sub_in_height = Nue2Reg->NUE2_Register_005c.bit.NUE2_SUB_IN_HEIGHT;
			} else {
				nue2_in_width = Nue2Reg_528->NUE2_Register_0044.bit.NUE2_IN_WIDTH;
				nue2_in_height = Nue2Reg_528->NUE2_Register_0044.bit.NUE2_IN_HEIGHT;
				nue2_h_scl_size = Nue2Reg_528->NUE2_Register_0058.bit.NUE2_H_SCL_SIZE;
				nue2_v_scl_size = Nue2Reg_528->NUE2_Register_0058.bit.NUE2_V_SCL_SIZE;
				nue2_sub_in_width = Nue2Reg_528->NUE2_Register_005c.bit.NUE2_SUB_IN_WIDTH;
				nue2_sub_in_height = Nue2Reg_528->NUE2_Register_005c.bit.NUE2_SUB_IN_HEIGHT;
			}
#endif

            //output
             if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 0) { // YUV420
                 if(Nue2Reg->NUE2_Register_0004.bit.NUE2_HSV_EN == 0){  // main path
                    if(Nue2Reg->NUE2_Register_0004.bit.NUE2_PAD_EN == 0){
                        nue2_dram_out_size0 = nue2_v_scl_size*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, nue2_h_scl_size);	
                        nue2_dram_out_size1 = nue2_v_scl_size*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1, nue2_h_scl_size);
                        nue2_dram_out_size2 = nue2_v_scl_size*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2, nue2_h_scl_size);
                    }
                    else{ //PADDING
                        nue2_dram_out_size0 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                        nue2_dram_out_size1 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                        nue2_dram_out_size2 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                    }
                 }
                 else{ //HSV
                     nue2_dram_out_size0 = nue2_v_scl_size*Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0;
                     nue2_dram_out_size1 = 0;
                     nue2_dram_out_size2 = 0;
                 }

				 if ((Nue2Reg->NUE2_Register_000c.bit.DRAM_SAI1 % 2) != 0) {
					nvt_dbg(ERR, "Error, yuv420-channel2 can't be supported by-align, it must be 2-bytes align. nue2_dram_in_addr1=0x%x\r\n",
									 Nue2Reg->NUE2_Register_000c.bit.DRAM_SAI1);
					continue;
				 }
				
             }
             else if(Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 1){ // Y only
                if(Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_EN == 0){  // main path
                    if(Nue2Reg->NUE2_Register_0004.bit.NUE2_PAD_EN == 0){
                        nue2_dram_out_size0 = nue2_v_scl_size*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, nue2_h_scl_size);
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                    }
                    else{ //PADDING
                        nue2_dram_out_size0 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                    }
                 }
                 else{ //rotate
                     if(Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_MODE ==2){
                        nue2_dram_out_size0 = nue2_in_height*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, nue2_in_width);
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                     }
                     else{
                        nue2_dram_out_size0 = nue2_in_width*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, nue2_in_height);
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                     }
                 }

             }
             else if(Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 2){ // UV only
                if(Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_EN == 0){  // main path
                    if(Nue2Reg->NUE2_Register_0004.bit.NUE2_PAD_EN == 0){
                        nue2_dram_out_size0 = nue2_v_scl_size*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, (nue2_h_scl_size*2));
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                    }
                    else{ //PADDING
                        nue2_dram_out_size0 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, (Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH*2));
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                    }
                 }
                 else{ //rotate
                     if(Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_MODE ==2){
                        nue2_dram_out_size0 = nue2_in_height*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, (nue2_in_width*2));
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                     }
                     else{
                        nue2_dram_out_size0 = nue2_in_width*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, (nue2_in_height*2));
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;
                     }
                 }

				 if ((Nue2Reg->NUE2_Register_0008.bit.DRAM_SAI0 % 2) != 0) { //2-byte align
                    nvt_dbg(ERR, "Error, UV mode(0x2) can't be supported byte-align, it must be 2-bytes align. nue2_dram_in_addr0=0x%x\r\n",
									Nue2Reg->NUE2_Register_0008.bit.DRAM_SAI0);
                    continue;
                 }

                 //SUB MODE
                 if(Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){
                     if ((Nue2Reg->NUE2_Register_0010.bit.DRAM_SAI2 % 2) != 0) { //2-byte align
                        nvt_dbg(ERR, "Error, UV mode(0x2) can't be supported byte-align (SUB MODE=1), \
                                        it must be 2-bytes align. nue2_dram_in_addr2=0x%x\r\n",
										Nue2Reg->NUE2_Register_0010.bit.DRAM_SAI2);
                        continue;
                     }
                 }
             }
             else{ // RGB

                if(Nue2Reg->NUE2_Register_0004.bit.NUE2_HSV_EN == 0 && Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_EN ==0 ){  // main path
                    if(Nue2Reg->NUE2_Register_0004.bit.NUE2_PAD_EN == 0){
                        nue2_dram_out_size0 = nue2_v_scl_size*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, nue2_h_scl_size);
                        nue2_dram_out_size1 = nue2_v_scl_size*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1, nue2_h_scl_size);
                        nue2_dram_out_size2 = nue2_v_scl_size*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2, nue2_h_scl_size);
                    }
                    else{ //PADDING
                        nue2_dram_out_size0 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                        nue2_dram_out_size1 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                        nue2_dram_out_size2 = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT*
													NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2, Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH);
                    }
                }
                else if(Nue2Reg->NUE2_Register_0004.bit.NUE2_HSV_EN == 1){
                        nue2_dram_out_size0 = nue2_v_scl_size*Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0;
                        nue2_dram_out_size1 = 0;
                        nue2_dram_out_size2 = 0;

                }
                else{ //ROTATE
					 nvt_dbg(ERR, "Error, RGB can't be supported the rotate mode. (to use Y/UV instead of it.)\r\n");
                     if(Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_MODE ==2){
                        nue2_dram_out_size0 = nue2_in_height*Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0;
                        nue2_dram_out_size1 = nue2_in_height*Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1;
                        nue2_dram_out_size2 = nue2_in_height*Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2;
                     }
                     else{
                        nue2_dram_out_size0 = nue2_in_width*Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0;
                        nue2_dram_out_size1 = nue2_in_width*Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1;
                        nue2_dram_out_size2 = nue2_in_width*Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2;
                     }
                 }

             }


#if 0
            nue2_Total_Output_Size = nue2_dram_out_size0 + nue2_dram_out_size1 +nue2_dram_out_size2;
#endif

			nue2_dram_in_size0 = 0;
			nue2_dram_in_size1 = 0;
			nue2_dram_in_size2 = 0;

			//in_width_ofs
			if(Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 0){ // YUV420
				in_width_ofs_0 = nue2_in_width;
				in_width_ofs_1 = nue2_in_width / 2;
				in_width_ofs_2 = (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 0)?0:nue2_sub_in_width*3;
			}
			else if(Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 1){
				in_width_ofs_0 = nue2_in_width;
				in_width_ofs_1 = 0;
				in_width_ofs_2 = (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 0)?0:nue2_sub_in_width;
			}
			else if(Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 2){
				in_width_ofs_0 = nue2_in_width*2;
				in_width_ofs_1 = 0;
				in_width_ofs_2 = (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 0)?
																0:NUE2_BIG_OPT(nue2_sub_in_width*2, nue2_in_width*2);
			}
			else{ //RGB
				in_width_ofs_0 = nue2_in_width;
				in_width_ofs_1 = nue2_in_width;
				in_width_ofs_2 = nue2_in_width;
			}



			//in_size
			if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 0) {  //YUV420
				 nue2_dram_in_size0 = nue2_in_height*
										NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0024.bit.DRAM_OFSI0, in_width_ofs_0);
				 nue2_dram_in_size1 = nue2_in_height*
										NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0028.bit.DRAM_OFSI1, in_width_ofs_1);
				 if (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_EN == 1) {
					 if (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){
							nue2_dram_in_size2 = nue2_sub_in_height*
										NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_002c.bit.DRAM_OFSI2, in_width_ofs_2);
					 }
					 else{
							nue2_dram_in_size2 = 0;
					 }
				 } else {
					 nue2_dram_in_size2 = 0;
				 }
			}
			else if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 1) { //Y only
				 nue2_dram_in_size0 = nue2_in_height*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0024.bit.DRAM_OFSI0, in_width_ofs_0);
				 nue2_dram_in_size1 = 0;
				 if (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){
						nue2_dram_in_size2 = NUE2_BIG_OPT(nue2_v_scl_size,
														  nue2_sub_in_height)*
														  NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_002c.bit.DRAM_OFSI2, in_width_ofs_2);
				 }
				 else{
						nue2_dram_in_size2 = 0;
				 }
			}
			else if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 2){ //UV pack
				 nue2_dram_in_size0 = nue2_in_height*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0024.bit.DRAM_OFSI0, in_width_ofs_0);
				 nue2_dram_in_size1 = 0;
				 if (Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){
						nue2_dram_in_size2 = NUE2_BIG_OPT(nue2_v_scl_size,
														  nue2_sub_in_height)*
														  NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_002c.bit.DRAM_OFSI2, in_width_ofs_2);
				}
				 else{
						nue2_dram_in_size2 = 0;
				 }
			}
			else{
				 nue2_dram_in_size0 = nue2_in_height*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0024.bit.DRAM_OFSI0, in_width_ofs_0);
				 nue2_dram_in_size1 = nue2_in_height*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_0028.bit.DRAM_OFSI1, in_width_ofs_1);
				 nue2_dram_in_size2 = nue2_in_height*
												NUE2_WIDTH_OPT(Nue2Reg->NUE2_Register_002c.bit.DRAM_OFSI2, in_width_ofs_2);
			}


        	// set in/out address
			nue2_dram_in_addr0 = mem_tmp_addr + (Nue2Reg->NUE2_Register_0008.bit.DRAM_SAI0 % 4);
			nue2_dram_in_addr1 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_in_addr0 + nue2_dram_in_size0 + 8) + (Nue2Reg->NUE2_Register_000c.bit.DRAM_SAI1 % 4);
			nue2_dram_in_addr2 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_in_addr1 + nue2_dram_in_size1 + 8) + (Nue2Reg->NUE2_Register_0010.bit.DRAM_SAI2 % 4);
			nue2_dram_out_addr0 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_in_addr2 + nue2_dram_in_size2 + 8) + (Nue2Reg->NUE2_Register_0018.bit.DRAM_SAO0 % 4);
			nue2_dram_out_addr1 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_out_addr0 + nue2_dram_out_size0 + 8) + (Nue2Reg->NUE2_Register_001c.bit.DRAM_SAO1 % 4);
			nue2_dram_out_addr2 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_out_addr1 + nue2_dram_out_size1 + 8) + (Nue2Reg->NUE2_Register_0020.bit.DRAM_SAO2 % 4);
			
			nue2_dram_gld_addr0 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_out_addr2 + nue2_dram_out_size2 + 8);
			nue2_dram_gld_size0 = nue2_dram_out_size0;
			nue2_dram_gld_addr1 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_gld_addr0 + nue2_dram_gld_size0 + 8);
			nue2_dram_gld_size1 = nue2_dram_out_size1;
			nue2_dram_gld_addr2 = NUE2_8_BYTE_ALIGN_CEILING(nue2_dram_gld_addr1 + nue2_dram_gld_size1 + 8);
			nue2_dram_gld_size2 = nue2_dram_out_size2;

			buf_start = NUE2_4_BYTE_ALIGN_CEILING(nue2_dram_gld_addr2 + nue2_dram_gld_size2 + 8);

			for (buf_idx = 0; buf_idx < NUE2_MAX_RESERVE_BUF_NUM; buf_idx++) {
				if (g_is_switch_dram == 0) {
					nue2_dram_addr_res[buf_idx] = buf_start;
					if (g_nue2_dram_outrange == 1 && g_nue2_dram_mode == 2 && buf_idx == 0) {
						nue2_dram_addr_res[buf_idx] = 0x80000000;
					} else if (g_nue2_dram_outrange == 1 && g_nue2_dram_mode == 1 && buf_idx == 0) {
						nue2_dram_addr_res[buf_idx] = 0x30000000;
					} else if (g_nue2_dram_outrange == 2 && buf_idx == 0) {
						if (g_nue2_dram_mode == 2) {
							nue2_dram_addr_res[buf_idx] = 0x80000000 - nue2_dram_gld_size0 - 8;
						} else if (g_nue2_dram_mode == 1) {	
							nue2_dram_addr_res[buf_idx] = 0x20000000 - nue2_dram_gld_size0 - 8;
						}
					}
				} else {
					if ((buf_idx % 2) == 0) {
						nue2_dram_addr_res[buf_idx] = NUE2_SWITCH_DRAM_1(buf_start);
					} else {
						nue2_dram_addr_res[buf_idx] = NUE2_SWITCH_DRAM_2(buf_start);
					}
				}
				DBG_EMU( "NUE2_RESERVE_BUF: idx(%d) addr(0x%x)\r\n", buf_idx, nue2_dram_addr_res[buf_idx]);
				nue2_dram_size_res[buf_idx] = NUE2_RESERVE_BUF_W*NUE2_RESERVE_BUF_H;
				buf_start = NUE2_4_BYTE_ALIGN_CEILING(buf_start + NUE2_RESERVE_BUF_W*NUE2_RESERVE_BUF_H + 8);

				memset((void *) nue2_dram_addr_res[buf_idx], 0, nue2_dram_size_res[buf_idx]);
            	nue2_pt_dma_flush(nue2_dram_addr_res[buf_idx], nue2_dram_size_res[buf_idx]);	
			}

			ll_buf = (UINT64 *) NUE2_8_BYTE_ALIGN_CEILING(buf_start);



			len = load_golden_data(nue2_dram_gld_addr0, nue2_dram_out_size0,
						     	nue2_dram_gld_addr1, nue2_dram_out_size1,
							 	nue2_dram_gld_addr2, nue2_dram_out_size2,
							 	&nue2_dram_gld_size0, &nue2_dram_gld_size1, &nue2_dram_gld_size2, nue2_get_index(index));
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read and by pass this case(%d)\n", nue2_get_index(index));
				continue;
			}
			
			
			if (nue2_dram_in_size0 == 0) nue2_dram_in_addr0 = 0;
			if (nue2_dram_in_size1 == 0) nue2_dram_in_addr1 = 0;
			if (nue2_dram_in_size2 == 0) nue2_dram_in_addr2 = 0;

			if (nue2_dram_out_size0 == 0) nue2_dram_out_addr0 = 0;
			if (nue2_dram_out_size1 == 0) nue2_dram_out_addr1 = 0;
			if (nue2_dram_out_size2 == 0) nue2_dram_out_addr2 = 0;

#if 1
            DBG_ERR( "nue2_dram_in_addr0  : %08x \r\n", nue2_dram_in_addr0);
            DBG_ERR( "nue2_dram_in_addr1  : %08x \r\n", nue2_dram_in_addr1);
            DBG_ERR( "nue2_dram_in_addr2  : %08x \r\n", nue2_dram_in_addr2);
            DBG_ERR( "nue2_dram_out_addr0 : %08x \r\n", nue2_dram_out_addr0);
            DBG_ERR( "nue2_dram_out_addr1 : %08x \r\n", nue2_dram_out_addr1);
            DBG_ERR( "nue2_dram_out_addr2 : %08x \r\n", nue2_dram_out_addr2);
			DBG_ERR( "nue2_dram_gld_addr0 : %08x \r\n", nue2_dram_gld_addr0);
            DBG_ERR( "nue2_dram_gld_addr1 : %08x \r\n", nue2_dram_gld_addr1);
            DBG_ERR( "nue2_dram_gld_addr2 : %08x \r\n", nue2_dram_gld_addr2);

			DBG_EMU( "nue2_dram_in_size0  : %08x \r\n", nue2_dram_in_size0);
            DBG_EMU( "nue2_dram_in_size1  : %08x \r\n", nue2_dram_in_size1);
            DBG_EMU( "nue2_dram_in_size2  : %08x \r\n", nue2_dram_in_size2);
            DBG_EMU( "nue2_dram_out_size0 : %08x \r\n", nue2_dram_out_size0);
            DBG_EMU( "nue2_dram_out_size1 : %08x \r\n", nue2_dram_out_size1);
            DBG_EMU( "nue2_dram_out_size2 : %08x \r\n", nue2_dram_out_size2);
			DBG_EMU( "nue2_dram_gld_size0 : %08x \r\n", nue2_dram_gld_size0);
            DBG_EMU( "nue2_dram_gld_size1 : %08x \r\n", nue2_dram_gld_size1);
            DBG_EMU( "nue2_dram_gld_size2 : %08x \r\n", nue2_dram_gld_size2);
#endif


    		// load image data
    		if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 0) {  //YUV420
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI0.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI0.bin", nue2_get_index(index));
#endif
                len = load_data(io_path, nue2_dram_in_addr0, nue2_dram_in_size0);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI1.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI1.bin", nue2_get_index(index));
#endif
                len = load_data(io_path, nue2_dram_in_addr1, nue2_dram_in_size1);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
    			if(Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){    //sub img
#if defined(__FREERTOS)
					snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI2.bin", nue2_get_index(index));
#else
                    snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI2.bin", nue2_get_index(index));
#endif
                    len = load_data(io_path, nue2_dram_in_addr2, nue2_dram_in_size2);
                    if (len <= 0) {
    					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
    					return -1;
    				}
    			}

            } else if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 1) { //Y only
    			//ROI Pooling
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI0.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI0.bin", nue2_get_index(index));
#endif
                len = load_data(io_path, nue2_dram_in_addr0, nue2_dram_in_size0);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
    			if(Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){
#if defined(__FREERTOS)
					snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI2.bin", nue2_get_index(index));
#else
                    snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI2.bin", nue2_get_index(index));
#endif
                    len = load_data(io_path, nue2_dram_in_addr2, nue2_dram_in_size2);
                    if (len <= 0) {
    					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
    					return -1;
    				}
    			}
            }
            else if (Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 2) { //UV only
    			//ROI Pooling
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI0.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI0.bin", nue2_get_index(index));
#endif
    			len = load_data(io_path, nue2_dram_in_addr0, nue2_dram_in_size0);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
    			if(Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE == 1){
#if defined(__FREERTOS)
					snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI2.bin", nue2_get_index(index));
#else
                    snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI2.bin", nue2_get_index(index));
#endif
                    len = load_data(io_path, nue2_dram_in_addr2, nue2_dram_in_size2);
                    if (len <= 0) {
    					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
    					return -1;
    				}
    			}
            }
            else{ //RGB only
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI0.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI0.bin", nue2_get_index(index));
#endif
    			len = load_data(io_path, nue2_dram_in_addr0, nue2_dram_in_size0);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI1.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI1.bin", nue2_get_index(index));
#endif
    			len = load_data(io_path, nue2_dram_in_addr1, nue2_dram_in_size1);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\NUE2P\\nue2g%d\\DI2.bin", nue2_get_index(index));
#else
    			snprintf(io_path, 64, "//mnt//sd//NUE2P//nue2g%d//DI2.bin", nue2_get_index(index));
#endif
    			len = load_data(io_path, nue2_dram_in_addr2, nue2_dram_in_size2);
                if (len <= 0) {
					nvt_dbg(ERR, "failed in file read:%s\n", io_path);
					return -1;
				}
            }


            // 	in case for byte align,
            word_addr0 = nue2_dram_out_addr0;
            word_addr1 = nue2_dram_out_addr1;
            word_addr2 = nue2_dram_out_addr2;
			byte_0 = 0;
			byte_1 = 0;
			byte_2 = 0;

#if (NUE2_OLD_PATTERN == 1)
#if defined(_BSP_NA51089_)
#else
			if(nvt_get_chip_id() == CHIP_NA51055) {
				if(Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT == 2){
					if ((nue2_dram_out_addr0 % 8) != 0){
						if(nue2_dram_out_addr0%8 == 4){ //bytealign = 4
							byte_0 = nue2_dram_out_addr0%8;
							word_addr0 = ((nue2_dram_out_addr0 / 8) ) * 8;
							byte_1 = nue2_dram_out_addr1%8;
							word_addr1 = ((nue2_dram_out_addr1 / 8) ) * 8;
							byte_2 = nue2_dram_out_addr2%8;
							word_addr2 = ((nue2_dram_out_addr2 / 8) ) * 8;
						}
						else{  // bytealign = 2
							byte_0 = nue2_dram_out_addr0%4;
							word_addr0 = nue2_dram_out_addr0 -  byte_0;
							byte_1 = nue2_dram_out_addr1%4;
							word_addr1 = nue2_dram_out_addr1 -  byte_1;
							byte_2 = nue2_dram_out_addr2%4;
							word_addr2 =  nue2_dram_out_addr2 -  byte_2;
						}
					}
				}
				else{
					if ((nue2_dram_out_addr0 % 4) != 0){
						byte_0 = nue2_dram_out_addr0%4;
						word_addr0 = ((nue2_dram_out_addr0 / 4) ) * 4;
					}
					if ((nue2_dram_out_addr1 % 4) != 0){
						byte_1 = nue2_dram_out_addr1%4;
						word_addr1 = ((nue2_dram_out_addr1 / 4) ) * 4;
					}
					if ((nue2_dram_out_addr2 % 4) != 0){
						byte_2 = nue2_dram_out_addr2%4;
						word_addr2 = ((nue2_dram_out_addr2 / 4) ) * 4;
					}
				}
			}
#endif
#endif

			
            // clear output
			if (word_addr0) {
				if (g_is_four_check) {
					memset((void*)(word_addr0 - 4), NUE2_MEMSET_VAL, 4);
				}
				memset((void*)word_addr0, NUE2_MEMSET_VAL, ((nue2_dram_out_size0/4+1)*4)+4);
			}
			if (word_addr1) {
				if (g_is_four_check) {
					memset((void*)(word_addr1 - 4), NUE2_MEMSET_VAL, 4);
				}
            	memset((void*)word_addr1, NUE2_MEMSET_VAL, ((nue2_dram_out_size1/4+1)*4)+4);
			}
			if (word_addr2) {
				if (g_is_four_check) {
					memset((void*)(word_addr2 - 4), NUE2_MEMSET_VAL, 4);
				}
            	memset((void*)word_addr2, NUE2_MEMSET_VAL, ((nue2_dram_out_size2/4+1)*4)+4);
			}

			//===================================================================================================
			// load register setting
        	nvt_dbg(IND, "Set to NUE2 Driver ... start\r\n");
            //DMA IN/OUT address
        	nue2_param.dmaio_addr.addr_in0         = nue2_dram_in_addr0;
            nue2_param.dmaio_addr.addr_in1         = nue2_dram_in_addr1;
            nue2_param.dmaio_addr.addr_in2         = nue2_dram_in_addr2;
            nue2_param.dmaio_addr.addr_out0        = nue2_dram_out_addr0;
            nue2_param.dmaio_addr.addr_out1        = nue2_dram_out_addr1;
            nue2_param.dmaio_addr.addr_out2        = nue2_dram_out_addr2;
            //Function
            nue2_param.func_en.yuv2rgb_en = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_YUV2RGB_EN;
            nue2_param.func_en.sub_en = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_EN;
            nue2_param.func_en.pad_en = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_PAD_EN;
            nue2_param.func_en.hsv_en = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_HSV_EN;
            nue2_param.func_en.rotate_en = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_EN;
            //Format
            nue2_param.infmt = (NUE2_IN_FMT)Nue2Reg->NUE2_Register_0004.bit.NUE2_IN_FMT;
            nue2_param.outfmt.out_signedness = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_OUT_SIGNEDNESS;
            //Line offset
            nue2_param.dmaio_lofs.in0_lofs = Nue2Reg->NUE2_Register_0024.bit.DRAM_OFSI0;
            nue2_param.dmaio_lofs.in1_lofs = Nue2Reg->NUE2_Register_0028.bit.DRAM_OFSI1;
            nue2_param.dmaio_lofs.in2_lofs = Nue2Reg->NUE2_Register_002c.bit.DRAM_OFSI2;
            nue2_param.dmaio_lofs.out0_lofs = Nue2Reg->NUE2_Register_0030.bit.DRAM_OFSO0;
            nue2_param.dmaio_lofs.out1_lofs = Nue2Reg->NUE2_Register_0034.bit.DRAM_OFSO1;
            nue2_param.dmaio_lofs.out2_lofs = Nue2Reg->NUE2_Register_0038.bit.DRAM_OFSO2;
            //Scaling
            nue2_param.scale_parm.fact_update = 0;  //MST scaling uses
            nue2_param.scale_parm.h_filtmode = (BOOL)Nue2Reg->NUE2_Register_0048.bit.NUE2_H_FILTMODE;
            nue2_param.scale_parm.v_filtmode = (BOOL)Nue2Reg->NUE2_Register_0048.bit.NUE2_V_FILTMODE;
            nue2_param.scale_parm.h_filtcoef = Nue2Reg->NUE2_Register_0048.bit.NUE2_H_FILTCOEF;
            nue2_param.scale_parm.v_filtcoef = Nue2Reg->NUE2_Register_0048.bit.NUE2_V_FILTCOEF;
            
            nue2_param.scale_parm.h_dnrate = Nue2Reg->NUE2_Register_0048.bit.NUE2_H_DNRATE;
            nue2_param.scale_parm.v_dnrate = Nue2Reg->NUE2_Register_0048.bit.NUE2_V_DNRATE;
            nue2_param.scale_parm.h_sfact = Nue2Reg->NUE2_Register_004c.bit.NUE2_H_SFACT;
            nue2_param.scale_parm.v_sfact = Nue2Reg->NUE2_Register_004c.bit.NUE2_V_SFACT;
            nue2_param.scale_parm.ini_h_dnrate = Nue2Reg->NUE2_Register_0050.bit.NUE2_INI_H_DNRATE;
            nue2_param.scale_parm.ini_h_sfact = Nue2Reg->NUE2_Register_0050.bit.NUE2_INI_H_SFACT;
            //Mean subtraction
            nue2_param.sub_parm.sub_mode = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_SUB_MODE;

			//Input size
            nue2_param.insize.in_width = nue2_in_width;
            nue2_param.insize.in_height = nue2_in_height;

			nue2_param.scale_parm.h_scl_size = nue2_h_scl_size;
            nue2_param.scale_parm.v_scl_size = nue2_v_scl_size;

			nue2_param.sub_parm.sub_in_width = nue2_sub_in_width;
            nue2_param.sub_parm.sub_in_height = nue2_sub_in_height;

#if defined(_BSP_NA51089_)
			//Mean subtraction
			nue2_param.sub_parm.sub_coef0 = Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_COEF_0;
			nue2_param.sub_parm.sub_coef1 = Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_COEF_1;
			nue2_param.sub_parm.sub_coef2 = Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_COEF_2;
			nue2_param.sub_parm.sub_dup = (NUE2_SUBDUP_RATE)Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_DUP;

			//padding
			nue2_param.pad_parm.pad_crop_x = Nue2Reg_528->NUE2_Register_0064.bit.NUE2_PAD_CROP_X;
			nue2_param.pad_parm.pad_crop_y = Nue2Reg_528->NUE2_Register_0064.bit.NUE2_PAD_CROP_Y;
			nue2_param.pad_parm.pad_crop_width = Nue2Reg_528->NUE2_Register_0068.bit.NUE2_PAD_CROP_WIDTH;
			nue2_param.pad_parm.pad_crop_height = Nue2Reg_528->NUE2_Register_0068.bit.NUE2_PAD_CROP_HEIGHT;

			//528 only
			nue2_param.mean_shift_parm.mean_shift_dir = Nue2Reg_528->NUE2_Register_007c.bit.MEAN_SHIFT_DIR;
			nue2_param.mean_shift_parm.mean_shift = Nue2Reg_528->NUE2_Register_007c.bit.MEAN_SHIFT;
			nue2_param.mean_shift_parm.mean_scale = Nue2Reg_528->NUE2_Register_007c.bit.MEAN_SCALE;

			nue2_param.scale_parm.scale_h_mode = Nue2Reg_528->NUE2_Register_0004.bit.NUE2_SCALE_H_MODE;
			nue2_param.scale_parm.scale_v_mode = Nue2Reg_528->NUE2_Register_0004.bit.NUE2_SCALE_V_MODE;
			nue2_param.flip_parm.flip_mode = Nue2Reg_528->NUE2_Register_0004.bit.NUE2_FLIP_MODE;
#else		
			if(nvt_get_chip_id() == CHIP_NA51055) {
            	nue2_param.sub_parm.sub_shift = Nue2Reg->NUE2_Register_0060.bit.NUE2_SUB_SHF;
				//Mean subtraction
				nue2_param.sub_parm.sub_coef0 = Nue2Reg->NUE2_Register_0060.bit.NUE2_SUB_COEF_0;
            	nue2_param.sub_parm.sub_coef1 = Nue2Reg->NUE2_Register_0060.bit.NUE2_SUB_COEF_1;
            	nue2_param.sub_parm.sub_coef2 = Nue2Reg->NUE2_Register_0060.bit.NUE2_SUB_COEF_2;
            	nue2_param.sub_parm.sub_dup = (NUE2_SUBDUP_RATE)Nue2Reg->NUE2_Register_0060.bit.NUE2_SUB_DUP;

				//padding
				nue2_param.pad_parm.pad_crop_x = Nue2Reg->NUE2_Register_0064.bit.NUE2_PAD_CROP_X;
            	nue2_param.pad_parm.pad_crop_y = Nue2Reg->NUE2_Register_0064.bit.NUE2_PAD_CROP_Y;
				nue2_param.pad_parm.pad_crop_width = Nue2Reg->NUE2_Register_0068.bit.NUE2_PAD_CROP_WIDTH;
            	nue2_param.pad_parm.pad_crop_height = Nue2Reg->NUE2_Register_0068.bit.NUE2_PAD_CROP_HEIGHT;

			} else {
				//Mean subtraction
				nue2_param.sub_parm.sub_coef0 = Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_COEF_0;
            	nue2_param.sub_parm.sub_coef1 = Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_COEF_1;
            	nue2_param.sub_parm.sub_coef2 = Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_COEF_2;
            	nue2_param.sub_parm.sub_dup = (NUE2_SUBDUP_RATE)Nue2Reg_528->NUE2_Register_0060.bit.NUE2_SUB_DUP;

				//padding
				nue2_param.pad_parm.pad_crop_x = Nue2Reg_528->NUE2_Register_0064.bit.NUE2_PAD_CROP_X;
            	nue2_param.pad_parm.pad_crop_y = Nue2Reg_528->NUE2_Register_0064.bit.NUE2_PAD_CROP_Y;
				nue2_param.pad_parm.pad_crop_width = Nue2Reg_528->NUE2_Register_0068.bit.NUE2_PAD_CROP_WIDTH;
            	nue2_param.pad_parm.pad_crop_height = Nue2Reg_528->NUE2_Register_0068.bit.NUE2_PAD_CROP_HEIGHT;

				//528 only
				nue2_param.mean_shift_parm.mean_shift_dir = Nue2Reg_528->NUE2_Register_007c.bit.MEAN_SHIFT_DIR;
				nue2_param.mean_shift_parm.mean_shift = Nue2Reg_528->NUE2_Register_007c.bit.MEAN_SHIFT;
				nue2_param.mean_shift_parm.mean_scale = Nue2Reg_528->NUE2_Register_007c.bit.MEAN_SCALE;

				nue2_param.scale_parm.scale_h_mode = Nue2Reg_528->NUE2_Register_0004.bit.NUE2_SCALE_H_MODE;
				nue2_param.scale_parm.scale_v_mode = Nue2Reg_528->NUE2_Register_0004.bit.NUE2_SCALE_V_MODE;
				nue2_param.flip_parm.flip_mode = Nue2Reg_528->NUE2_Register_0004.bit.NUE2_FLIP_MODE;

			}
#endif

            //Padding
            nue2_param.pad_parm.pad_crop_out_x = Nue2Reg->NUE2_Register_006c.bit.NUE2_PAD_OUT_X;
            nue2_param.pad_parm.pad_crop_out_y = Nue2Reg->NUE2_Register_006c.bit.NUE2_PAD_OUT_Y;
            nue2_param.pad_parm.pad_crop_out_width = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_WIDTH;
            nue2_param.pad_parm.pad_crop_out_height = Nue2Reg->NUE2_Register_0070.bit.NUE2_PAD_OUT_HEIGHT;
            nue2_param.pad_parm.pad_val0 = Nue2Reg->NUE2_Register_0074.bit.NUE2_PAD_VAL_0;
            nue2_param.pad_parm.pad_val1 = Nue2Reg->NUE2_Register_0074.bit.NUE2_PAD_VAL_1;
            nue2_param.pad_parm.pad_val2 = Nue2Reg->NUE2_Register_0074.bit.NUE2_PAD_VAL_2;
            //HSV
            nue2_param.hsv_parm.hsv_out_mode = (BOOL)Nue2Reg->NUE2_Register_0004.bit.NUE2_HSV_OUT_MODE;
            nue2_param.hsv_parm.hsv_hue_shift = Nue2Reg->NUE2_Register_0078.bit.NUE2_HUE_SFT;
            //Rotate
            nue2_param.rotate_parm.rotate_mode = (NUE2_ROT_DEG)Nue2Reg->NUE2_Register_0004.bit.NUE2_ROTATE_MODE;

			//debug parameter
			if (g_rand_burst == 0) {
                nue2_param.dbg_parm.in_burst_mode = 0;
                nue2_param.dbg_parm.out_burst_mode = 0;
			} else if (g_rand_burst == 1) {
				nue2_param.dbg_parm.in_burst_mode = 1;
                nue2_param.dbg_parm.out_burst_mode = 1;
			} else if (g_rand_burst == 2) {
				nue2_param.dbg_parm.in_burst_mode = 2;
                nue2_param.dbg_parm.out_burst_mode = 2;
			} else if (g_rand_burst == 3) {
				nue2_param.dbg_parm.in_burst_mode = 3;
                nue2_param.dbg_parm.out_burst_mode = 3;
            } else {
#if defined(__FREERTOS)
                nue2_param.dbg_parm.in_burst_mode = rand() % 4;
                nue2_param.dbg_parm.out_burst_mode = rand() % 4;
#else
                nvt_dbg(ERR, "Error,nue2_param.dbg_parm.in_burst_mode = rand() mod 4, TODO.... \r\n");
                return -1;
#endif
            }

			//flow control
			nue2_param.flow_ct.rand_ch_en = g_rand_ch_en;
            nue2_param.flow_ct.interrupt_en = g_interrupt_en;

			nue2_param.strip_parm.is_strip = g_is_strip;
            nue2_param.strip_parm.s_step = g_s_step;
            nue2_param.strip_parm.s_posi = g_s_posi;	


			if (nue2_param.flow_ct.interrupt_en == 1) {
				if (nue2_engine_setmode(NUE2_OPMODE_USERDEFINE, (VOID *) &nue2_param) != E_OK) {
					nvt_dbg(ERR, "nue2_setmode error ..\r\n");
					continue;
				} else {
					nvt_dbg(IND, "Set to NUE2 Driver ... done\r\n");
				}
			} else {
				if (nue2_engine_setmode(NUE2_OPMODE_USERDEFINE_NO_INT, (VOID *) &nue2_param) != E_OK) {
					nvt_dbg(ERR, "nue2_setmode error ..\r\n");
					continue;
				} else {
					nvt_dbg(IND, "Set to NUE2 Driver ... done\r\n");
				}
			}


            //===================================================================================================
			// fire engine
			if (nue2_param.flow_ct.interrupt_en == 1) {
				//nue2_change_interrupt(NUE2_INT_ALL);A
			} else {
#if defined(_BSP_NA51089_)
				nue2_clr_intr_status(NUE2_INT_ALL_528);
#else
				if(nvt_get_chip_id() == CHIP_NA51055) {
					nue2_clr_intr_status(NUE2_INT_ALL);
				} else {
					nue2_clr_intr_status(NUE2_INT_ALL_528);
				}
#endif
			}


			//stripe handle
			if (g_ll_mode_en == 0) {
				nue2_param.flow_ct.ll_buf = 0;
			} else {
				nue2_param.flow_ct.ll_buf = ll_buf;
			}

			//nue2_param.flow_ct.rand_ch_en = ?; //before decision
			//nue2_param.flow_ct.interrupt_en = ?; //before decision
			nue2_param.flow_ct.s_num = g_s_num;
			nue2_param.flow_ct.rst_en = g_hw_rst_en;
			//nue2_param.flow_ct.ll_buf = ?; //before decision
			nue2_param.flow_ct.is_terminate = g_ll_terminate;
			nue2_param.flow_ct.is_dma_test = g_dma_test;
			nue2_param.flow_ct.is_fill_reg_only = g_fill_reg_only;
			nue2_param.flow_ct.is_ll_next_update = g_is_ll_next_update;
			nue2_param.flow_ct.cnt_is_hw_only = g_cnt_is_hw_only;
			nue2_param.flow_ct.is_reg_dump = g_is_reg_dump;
			nue2_param.flow_ct.ll_test_2 = g_ll_test_2;
			nue2_param.flow_ct.is_dump_ll_buf = g_dump_ll_buf;
			nue2_param.flow_ct.ll_big_buf = g_ll_big_buf;
			nue2_param.flow_ct.auto_clk = g_auto_clk;
			nue2_param.flow_ct.is_bit60 = g_is_bit60;
			nue2_param.flow_ct.ll_base_addr = g_ll_base_addr;
			nue2_param.flow_ct.ll_fill_reg_num = g_ll_fill_reg_num;
			nue2_param.flow_ct.ll_fill_num = g_ll_fill_num;
			nue2_param.flow_ct.is_switch_dram = g_is_switch_dram;
			nue2_param.flow_ct.clk_en = g_clk_en;
			nue2_param.flow_ct.sram_down = g_sram_down;
			nue2_param.flow_ct.loop_time = g_loop_time;
			nue2_param.flow_ct.loop_mode = g_loop_mode;
	
			nue2_param.reg_func.nue2_start = nue2_start;
			nue2_param.reg_func.nue2_ll_start =  nue2_ll_start;
			nue2_param.reg_func.nue2_wait_frameend = nue2_wait_frameend;
			nue2_param.reg_func.nue2_wait_ll_frameend = nue2_wait_ll_frameend;
			nue2_param.reg_func.nue2_engine_loop_frameend = nue2_engine_loop_frameend;
			nue2_param.reg_func.nue2_engine_loop_llend = nue2_engine_loop_llend;
			nue2_param.reg_func.nue2_engine_debug_hook = nue2_engine_debug_hook;
			nue2_param.reg_func.nue2_engine_debug_hook1 = nue2_engine_debug_hook1;
			nue2_param.reg_func.nue2_engine_debug_hook2 = nue2_engine_debug_hook2;

			nue2_pause();
			nvt_dbg(IND, "[NUE2 DRV] start engine\n");
			DBG_EMU( "WW### before to start NUE2\n");
#if defined(__FREERTOS)
			nue2_write_protect_test(g_wp_en, g_wp_sel_id, (NUE2_PARM *) &nue2_param);
			if (g_pri_mode == 4) {
				dma_setChannelPriority(DMA_CH_NUE2_0, rand() % 4);
				dma_setChannelPriority(DMA_CH_NUE2_1, rand() % 4);
				dma_setChannelPriority(DMA_CH_NUE2_2, rand() % 4);
				dma_setChannelPriority(DMA_CH_NUE2_3, rand() % 4);
				dma_setChannelPriority(DMA_CH_NUE2_4, rand() % 4);
				dma_setChannelPriority(DMA_CH_NUE2_5, rand() % 4);
				dma_setChannelPriority(DMA_CH_NUE2_6, rand() % 4);
			} else {
				dma_setChannelPriority(DMA_CH_NUE2_0, g_pri_mode);
				dma_setChannelPriority(DMA_CH_NUE2_1, g_pri_mode);
				dma_setChannelPriority(DMA_CH_NUE2_2, g_pri_mode);
				dma_setChannelPriority(DMA_CH_NUE2_3, g_pri_mode);
				dma_setChannelPriority(DMA_CH_NUE2_4, g_pri_mode);
				dma_setChannelPriority(DMA_CH_NUE2_5, g_pri_mode);
				dma_setChannelPriority(DMA_CH_NUE2_6, g_pri_mode);
			}
			if (g_clock == 4) {
				nue2_pt_set_clock_rate(g_clock_val[rand() % 4]);
			} else {
				nue2_pt_set_clock_rate(g_clock_val[g_clock]);
			}
	
			if (g_rand_comb == 1) {
				g_is_switch_dram = rand() % 2;
				g_ll_fill_reg_num = 1 + (rand() % 10);
				g_ll_fill_num = 5 + (rand() % 10);
				g_nue2_dram_mode = 1 + rand() % 2;
			}

#endif

			nue2_engine_flow(&nue2_param);
			nvt_dbg(IND, "[NUE2 DRV] wait frmend\n");
			DBG_EMU( "WW### after to start NUE2\n");


#if (NUE2_AI_FLOW == ENABLE)
			nvt_dbg(ERR, "Error, Please disable AI_FLOW in nue2_platform.h......(NOW: ENABLE)\r\n");
#endif
			DBG_ERR( "is_hw_only(%d): hw_ll(0x%x) hw_no_ll(0x%x) cnt_single(0x%x)\r\n", 
							nue2_param.flow_ct.cnt_is_hw_only, nue2_param.flow_ct.cnt_hw_ll, nue2_param.flow_ct.cnt_hw_no_ll, nue2_param.flow_ct.cnt_single_hw);

			if (nue2_dram_out_addr0) {
				nue2_pt_dma_flush_dev2mem(nue2_dram_out_addr0, nue2_dram_out_size0);
			}
			if (nue2_dram_out_addr1) {
				nue2_pt_dma_flush_dev2mem(nue2_dram_out_addr1, nue2_dram_out_size1);
			}
			if (nue2_dram_out_addr2) {
				nue2_pt_dma_flush_dev2mem(nue2_dram_out_addr2, nue2_dram_out_size2);
			}

			nvt_dbg(IND, "[NUE2 DRV] wait frmend done\n");
			DBG_EMU( "WW### wait for frameend NUE2\n");


			//===================================================================================================
            // dump results
            nvt_dbg(IND, "nue2_dram_out_size0 = %d\r\n", nue2_dram_out_size0);
            nvt_dbg(IND, "byte_0 = %d\r\n", byte_0);
            nvt_dbg(IND, "word_addr0 = %08x\r\n", word_addr0); //

			//nue2_pt_dma_flush(nue2_dram_gld_addr0, nue2_dram_gld_size0);
			//nue2_pt_dma_flush(nue2_dram_gld_addr1, nue2_dram_gld_size1);
			//nue2_pt_dma_flush(nue2_dram_gld_addr2, nue2_dram_gld_size2);

#if 0
			if (nue2_dram_gld_addr0) {
				nue2_pt_dma_flush_dev2mem(nue2_dram_gld_addr0, nue2_dram_gld_size0);
			}
			if (nue2_dram_gld_addr1) {
				nue2_pt_dma_flush_dev2mem(nue2_dram_gld_addr1, nue2_dram_gld_size1);
			}
			if (nue2_dram_gld_addr2) {
				nue2_pt_dma_flush_dev2mem(nue2_dram_gld_addr2, nue2_dram_gld_size2);
			}
#endif

            if (nue2_dram_out_size0 > 0) { //(sdio2_getCardExist())
				if (g_gld_cmp == 1) {
					len = nue2_cmp_data(word_addr0, nue2_dram_gld_size0, nue2_dram_gld_addr0, g_is_four_check, nue2_get_index(index));
					if (len < 0) {
#if defined(__FREERTOS)
						snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\DO0.bin", nue2_get_index(index));
#else
						snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//DO0.bin", nue2_get_index(index));
#endif
						nvt_dbg(ERR, "Error(DO0), compare fail at rlt_addr(0x%x) gld(0x%x) size(0x%x) nue2_get_index(index)(%d)\r\n",
											word_addr0, nue2_dram_gld_addr0, nue2_dram_gld_size0, nue2_get_index(index));
						nvt_dbg(IND,"dump size = %d\r\n", nue2_dram_out_size0);
						len = dump_data(io_path, word_addr0, nue2_dram_out_size0 + byte_0);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
						nvt_dbg(IND,"dump address = %08x\r\n", nue2_dram_out_addr0);
						is_fail_case = 1;
					} else {
						nvt_dbg(ERR, "DO0: compare ok len(0x%x) at nue2_get_index(index)=%d\r\n", len, nue2_get_index(index));
					}
				} else {
#if defined(__FREERTOS)
                        snprintf(io_path, 64, "A:\\realout\\NUE2\\nue2g%d\\DO0.bin", nue2_get_index(index));
#else
                        snprintf(io_path, 64, "//mnt//sd//realout//NUE2//nue2g%d//DO0.bin", nue2_get_index(index));
#endif
                        nvt_dbg(IND,"dump size = %d\r\n", nue2_dram_out_size0);
                        len = dump_data(io_path, word_addr0, nue2_dram_out_size0 + byte_0);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
                        nvt_dbg(IND,"dump address = %08x\r\n", nue2_dram_out_addr0);
				}
    		}

            nvt_dbg(IND, "nue2_dram_out_size1 = %d\r\n", nue2_dram_out_size1);
            nvt_dbg(IND, "byte_1 = %d\r\n", byte_1);
            nvt_dbg(IND, "word_addr1 = %08x\r\n", word_addr1);
    		if (nue2_dram_out_size1 > 0) {
				if (g_gld_cmp == 1) {
					len = nue2_cmp_data(word_addr1, nue2_dram_gld_size1, nue2_dram_gld_addr1, g_is_four_check, nue2_get_index(index));
					if (len < 0) {
#if defined(__FREERTOS)
						snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\DO1.bin", nue2_get_index(index));
#else
						snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//DO1.bin", nue2_get_index(index));
#endif
						nvt_dbg(ERR, "Error(DO1), compare fail at rlt_addr(0x%x) gld(0x%x) size(0x%x) nue2_get_index(index)(%d)\r\n",
											word_addr1, nue2_dram_gld_addr1, nue2_dram_gld_size1, nue2_get_index(index));
						nvt_dbg(IND, "dump size = %d\r\n", nue2_dram_out_size1);
						len = dump_data(io_path, word_addr1, nue2_dram_out_size1 + byte_1);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
						nvt_dbg(IND, "dump address = %08x\r\n", word_addr1);
						is_fail_case = 1;
					} else {
						nvt_dbg(ERR, "DO1: compare ok len(0x%x) at nue2_get_index(index)=%d\r\n", len, nue2_get_index(index));
					}
				} else {
#if defined(__FREERTOS)
                        snprintf(io_path, 64, "A:\\realout\\NUE2\\nue2g%d\\DO1.bin", nue2_get_index(index));
#else
                        snprintf(io_path, 64, "//mnt//sd//realout//NUE2//nue2g%d//DO1.bin", nue2_get_index(index));
#endif
                        nvt_dbg(IND, "dump size = %d\r\n", nue2_dram_out_size1);
                        len = dump_data(io_path, word_addr1, nue2_dram_out_size1 + byte_1);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
                        nvt_dbg(IND, "dump address = %08x\r\n", word_addr1);
				}
    		}

            nvt_dbg(IND, "nue2_dram_out_size2 = %d\r\n", nue2_dram_out_size2);
            nvt_dbg(IND, "byte_2 = %d\r\n", byte_2);
            nvt_dbg(IND, "word_addr2 = %08x\r\n", word_addr2);
            if (nue2_dram_out_size2 > 0) {
				if (g_gld_cmp == 1) {
					len = nue2_cmp_data(word_addr2, nue2_dram_gld_size2, nue2_dram_gld_addr2, g_is_four_check, nue2_get_index(index));
					if (len < 0) {
#if defined(__FREERTOS)
						snprintf(io_path, 64, "A:\\realout_err\\NUE2\\nue2g%d\\DO2.bin", nue2_get_index(index));
#else
						snprintf(io_path, 64, "//mnt//sd//realout_err//NUE2//nue2g%d//DO2.bin", nue2_get_index(index));
#endif
						nvt_dbg(ERR, "Error(DO2), compare fail at rlt_addr(0x%x) gld(0x%x) size(0x%x) nue2_get_index(index)(%d)\r\n",
											word_addr2, nue2_dram_gld_addr2, nue2_dram_gld_size2, nue2_get_index(index));
						nvt_dbg(IND, "dump size = %d\r\n", nue2_dram_out_size2);
						len = dump_data(io_path, word_addr2, nue2_dram_out_size2 + byte_2);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
						nvt_dbg(IND, "dump address = %08x\r\n", word_addr2);
						is_fail_case = 1;
					} else {
						nvt_dbg(ERR, "DO2: compare ok len(0x%x) at nue2_get_index(index)=%d\r\n", len, nue2_get_index(index));
					}
				} else {
#if defined(__FREERTOS)
						snprintf(io_path, 64, "A:\\realout\\NUE2\\nue2g%d\\DO2.bin", nue2_get_index(index));
#else
						snprintf(io_path, 64, "//mnt//sd//realout//NUE2//nue2g%d//DO2.bin", nue2_get_index(index));
#endif
						nvt_dbg(IND, "dump size = %d\r\n", nue2_dram_out_size2);
						len = dump_data(io_path, word_addr2, nue2_dram_out_size2 + byte_2);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
						nvt_dbg(IND, "dump address = %08x\r\n", word_addr2);
				}
    		}

			if (is_fail_case == 1) {
				dump_whole_data(nue2_dram_in_addr0, nue2_dram_in_size0,
								nue2_dram_in_addr1, nue2_dram_in_size1,
                                nue2_dram_in_addr2, nue2_dram_in_size2, nue2_get_index(index),
                                nue2_dram_gld_addr0, nue2_dram_gld_size0,
								nue2_dram_gld_addr1, nue2_dram_gld_size1,
                                nue2_dram_gld_addr2, nue2_dram_gld_size2);
			}


    		nvt_dbg(IND, "#### Test pattern is done ####\r\n\r\n");
		} else if (emu_func == 3) {
			// test sdk flow
			NN_GEN_MODEL_HEAD* model_head = NULL;
			NN_GEN_MODE_CTRL* mode_ctrl = NULL;
#if defined(__FREERTOS)
			DBG_EMU( "TODO here %s %d\r\n", __FUNCTION__, __LINE__);
#else
			INT32 engine = 0;
			UINT32 id = 0;
#endif
			UINT32 io_base = 0, model_base = 0;

			UINT32 layer_idx = 0;

			//====================================================================
			// load model
#if defined(__FREERTOS)
			snprintf(io_path, 64, "A:\\test_%d.bin", index);
#else
			snprintf(io_path, 64, "//mnt//sd//test_%d.bin", index);
#endif
			mem_tmp_addr = mem_base;
			len = load_data(io_path, mem_tmp_addr, g_mem_size_check);
			if (len <= 0) {
				nvt_dbg(ERR, "failed in file read:%s\n", io_path);
				return -1;
			}

			//====================================================================
			// parsing model

			// model head
			model_head = (NN_GEN_MODEL_HEAD*)mem_tmp_addr;
#if SDK_DUMP_INFO
			nvt_dbg(IND, "model ctrl num  = %d\r\n", model_head->mode_ctrl_num);
			nvt_dbg(IND, "model layer num = %d\r\n", model_head->layer_num);
#endif
			mem_tmp_addr += sizeof(NN_GEN_MODEL_HEAD);

			io_base = ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD)) + ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * model_head->mode_ctrl_num)
					+ ALIGN_CEIL_4(sizeof(NN_IOMEM) * model_head->layer_num) + model_head->model_size + model_head->parm_size
					+ mem_base;
			model_base = io_base - model_head->model_size;
#if SDK_DUMP_INFO
			nvt_dbg(IND, "io base    = 0x%08X\r\n", io_base);
			nvt_dbg(IND, "model base = 0x%08X\r\n", model_base);
#endif
			// mode ctrl
			if (total_layer_num > model_head->mode_ctrl_num)
				total_layer_num = model_head->mode_ctrl_num;
			for (i=0; i < total_layer_num; i++) {
			//for (i=0; i<3; i++) {
				UINT32 j = 0;
				KDRV_AI_LL_HEAD *p_parm_ll_head = NULL;
				KDRV_AI_APP_HEAD* p_parm_head = NULL;
#if SDK_DUMP_INFO
				nvt_dbg(IND, "=============================================================================\r\n");
				nvt_dbg(IND, "layer(mode-ctrl): %d\r\n", i);
				nvt_dbg(IND, "=============================================================================\r\n");
#endif
				mode_ctrl = (NN_GEN_MODE_CTRL*)mem_tmp_addr;
				mem_tmp_addr += sizeof(NN_GEN_MODE_CTRL);
				// chk trig src
				if (mode_ctrl->trig_src != NN_GEN_TRIG_APP_AI_DRV && mode_ctrl->trig_src != NN_GEN_TRIG_LL_AI_DRV) {
					nvt_dbg(IND, "not support now!\r\n");
					continue;
				}

#if defined(__FREERTOS)
			    nvt_dbg(ERR, "TODO here %s %d\r\n", __FUNCTION__, __LINE__);
#else
				// select engine
				if (mode_ctrl->eng == NN_GEN_ENG_NUE2) {
					engine = KDRV_AI_ENGINE_NUE2;
					#if SDK_DUMP_INFO
					nvt_dbg(IND, "engine type: nue2\r\n");
					#endif
				} else if (mode_ctrl->eng == NN_GEN_ENG_CNN) {
					engine = KDRV_AI_ENGINE_CNN2;
					#if SDK_DUMP_INFO
					nvt_dbg(IND, "engine type: cnn\r\n");
					#endif
				} else if (mode_ctrl->eng == NN_GEN_ENG_CNN2) {
					engine = KDRV_AI_ENGINE_CNN2;
					#if SDK_DUMP_INFO
					nvt_dbg(IND, "engine type: cnn2\r\n");
					#endif
				}else if (mode_ctrl->eng == NN_GEN_ENG_NUE) {
					engine = KDRV_AI_ENGINE_NUE;
					#if SDK_DUMP_INFO
					nvt_dbg(IND, "engine type: nue\r\n");
					#endif
				} else {
					nvt_dbg(IND, "unsupport engine type!\r\n");
					continue;
				}

				id = KDRV_DEV_ID(0, engine, 0);
#endif
				#if SDK_DUMP_INFO
				nvt_dbg(IND, "mode ctrl addr = 0x%08X\n", mode_ctrl->addr);
				#endif
				mode_ctrl->addr = mode_ctrl->addr + mem_base;
				#if SDK_DUMP_INFO
				nvt_dbg(IND, "mode = %d, trigger time = %d\r\n", mode_ctrl->mode, mode_ctrl->tot_trig_eng_times);
				#endif
				for (j=0; j<mode_ctrl->tot_trig_eng_times; j++) {
					if (mode_ctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {
						if (j == 0)
							p_parm_head = (KDRV_AI_APP_HEAD*)mode_ctrl->addr;
						else
							p_parm_head = (KDRV_AI_APP_HEAD*)p_parm_head->stripe_head_addr;
						p_parm_head->parm_addr += mem_base;
						if (p_parm_head->stripe_head_addr > 0) {
							#if SDK_DUMP_INFO
							nvt_dbg(IND, "stripe head addr = 0x%08X\r\n", p_parm_head->stripe_head_addr);
							#endif
							p_parm_head->stripe_head_addr += mem_base;
						}
						// update io addr
						if (sdk_update_ioaddr((i==0 && j==0)?1:0, p_parm_head->parm_addr, io_base, model_base, mem_base, g_mem_size_check, model_head->mode_ctrl_num, mode_ctrl) != 0)
							continue;
					} else if (mode_ctrl->trig_src == NN_GEN_TRIG_LL_AI_DRV) {

						p_parm_ll_head = (KDRV_AI_LL_HEAD*)mode_ctrl->addr;
						p_parm_ll_head->parm_addr += mem_base;
						if (sdk_update_ll_ioaddr((i==0 && j==0)?1:0, p_parm_ll_head->parm_addr, io_base, model_base, mem_base, g_mem_size_check, model_head->mode_ctrl_num, mode_ctrl, total_layer_num))
							continue;

						break;
					}
				}
				//if (layer_idx > 0)
				//	layer_idx += mode_ctrl->tot_trig_eng_times;
				if (mode_ctrl->trig_src == NN_GEN_TRIG_LL_AI_DRV && mode_ctrl->tot_trig_eng_times > 0) {
					if (mode_ctrl->eng == NN_GEN_ENG_CNN) {
						layer_idx += (mode_ctrl->tot_trig_eng_times-3);
					} else {
						layer_idx += mode_ctrl->tot_trig_eng_times;
					}
				}
				if (mode_ctrl->tot_trig_eng_times) {
#if (SDK_DUMP_REG == 1)
					unsigned int engine_reg_num = 0;
					unsigned int* engine_reg = NULL;
#endif
					NN_IOMEM *out_info = (NN_IOMEM*)(mem_base + ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD)) + ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * model_head->mode_ctrl_num) + layer_idx*sizeof(NN_IOMEM));

#if defined(__FREERTOS)
					nvt_dbg(ERR, "TODO here %s %d\r\n", __FUNCTION__, __LINE__);
#else

					KDRV_AI_TRIG_MODE mode = (mode_ctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV)?AI_TRIG_MODE_APP:AI_TRIG_MODE_LL;
					#if SDK_DUMP_INFO
					nvt_dbg(IND, "total trigger time = %d\r\n", mode_ctrl->tot_trig_eng_times);
					#endif
					kdrv_ai_open(0, engine);
					kdrv_ai_set(id, KDRV_AI_PARAM_MODE_INFO, &mode);
					if (mode_ctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {
						KDRV_AI_APP_INFO info = { (KDRV_AI_APP_HEAD *)mode_ctrl->addr, mode_ctrl->tot_trig_eng_times};
						kdrv_ai_set(id, KDRV_AI_PARAM_APP_INFO, &info);
						layer_idx++;
					}
					else {
						KDRV_AI_LL_INFO info = { (KDRV_AI_LL_HEAD *)mode_ctrl->addr, mode_ctrl->tot_trig_eng_times};
						kdrv_ai_set(id, KDRV_AI_PARAM_LL_INFO, &info);
					}
					kdrv_ai_trigger(id, NULL, NULL, NULL);
					kdrv_ai_close(0, engine);
#endif

					#if SDK_DUMP_INFO
					nvt_dbg(IND, "[test] layer %d out io info addr = 0x%08X\r\n", (UINT32)layer_idx, (UINT32)out_info);
					#endif

					sdk_dump_output(out_info, layer_idx, io_base);

#if (SDK_DUMP_REG == 1)
					if (mode_ctrl->eng == NN_GEN_ENG_NUE2) {
						engine_reg = (unsigned int*)(nue2_get_base_addr());
						engine_reg_num = 0x94;
					} else if (mode_ctrl->eng == NN_GEN_ENG_CNN) {
						engine_reg = (unsigned int*)(cnn_get_base_addr(1));
						engine_reg_num = 0x200;
					} else if (mode_ctrl->eng == NN_GEN_ENG_CNN2) {
						engine_reg = (unsigned int*)(cnn_get_base_addr(1));
						engine_reg_num = 0x200;
					}else if (mode_ctrl->eng == NN_GEN_ENG_NUE) {
						engine_reg = (unsigned int*)(nue_get_base_addr());
						engine_reg_num = 0x1c4;
					}

					nvt_dbg(IND, "=========layer index %d: reg============\r\n", layer_idx);
					for (j = 0; j < engine_reg_num/4; j++)
						nvt_dbg(IND, "%04X: %08X\r\n", 4*j, engine_reg[j]);
					nvt_dbg(IND, "========================================\r\n");
#endif
#if (TMP_DBG == 1)
					if (i == 2) {
#if defined(__FREERTOS)
						len = dump_data("A:\\dbg.bin", 0x80000000 + engine_reg[4], 0x31000);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
#else
						len = dump_data("//mnt//sd//dbg.bin", 0x80000000 + engine_reg[4], 0x31000);
						if (len == 0) {
							nvt_dbg(ERR, "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
						}
#endif
					}
#endif
				}
				if (i == total_layer_num - 1) {
					NN_IOMEM *out_info = (NN_IOMEM*)(mem_base + ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD)) + ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * model_head->mode_ctrl_num) + mode_ctrl->layer_index*sizeof(NN_IOMEM));
					#if SDK_DUMP_INFO
					nvt_dbg(IND, "***ready to dump last layer %d --> indicate to mode ctrl layer index %d***\r\n", i, mode_ctrl->layer_index);
					#endif
					sdk_dump_output(out_info, 999, io_base);
				}
			}

#if (SDK_DUMP_IO == 1)

			DBG_EMU( "==============dump layer io info==============\n");
			for (i = 0; i < model_head->layer_num; i++) {
				NN_IOMEM *out_info = (NN_IOMEM*)(mem_base + ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD)) + ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * model_head->mode_ctrl_num) + i*sizeof(NN_IOMEM));
				unsigned int remap_va = 0;
				unsigned int j = 0;

				for (j = 0; j < NN_OMEM_NUM; j++) {
					if (out_info->SAO[j].size > 0 && out_info->SAO[j].va > 0) {
						DBG_EMU( "layer %d sao %d addr(origin) = 0x%08X\n", i, j, out_info->SAO[j].va);
						remap_va = sdk_addr_update(out_info->SAO[j].va, io_base, SDK_TEST_IO_BASE);
						DBG_EMU( "layer %d sao %d addr(re-map) = 0x%08X size = %d\n", i, j, remap_va, out_info->SAO[j].size);
					}
				}
				DBG_EMU( "============================\n");
			}
#endif
		}
	}

	if (emu_func == 0) {
		// CNN close
	} else if (emu_func == 1) {
		if (ll_en == 1) {
			nue_ll_pause();
			nvt_dbg(IND, "set nue ll addr\r\n");
			nue_set_dmain_lladdr(ll_base);
			nvt_dbg(IND, "start nue ll\r\n");
			nue_ll_start();
			nvt_dbg(IND, "wait nue ll frmend\r\n");
			nue_wait_ll_frameend(FALSE);
			nvt_dbg(IND, "wait nue ll frmend done\r\n");
			nue_ll_pause();

			//===================================================================================================
			// dump ll results
			//fmem_dcache_sync((void *)nue_dram_out_addr, nue_out_size, DMA_BIDIRECTIONAL);
			for (i = index_start;i <= index_end;i++) {
#if defined(__FREERTOS)
				snprintf(io_path, 64, "A:\\realout\\NUE\\nueg%d\\nueR%d.bin", i, i);
#else
				snprintf(io_path, 64, "//mnt//sd//realout//NUE//nueg%d//nueR%d.bin", i, i);
#endif
				len = dump_data(io_path, ll_out_addr[i - index_start], ll_out_size[i - index_start]);
				if (len == 0) {
					DBG_EMU( "%s:%dthe len from dump_data is 0\r\n", __FUNCTION__, __LINE__);
				}
			}
			//===================================================================================================
		} else {
			// NUE close
			nue_pause();
		}
		nue_close();
	} else if (emu_func == 2) {
		// NUE2 close
		nue2_pause();
		if (g_fill_reg_only == 0) {
			nue2_close();
		}
	} else if (emu_func == 3) {
		// sdk
		nvt_dbg(IND, "sdk flow done\r\n");
	}

#if defined(__FREERTOS)
#else
	ret = vos_mem_release_from_cma(vos_mem_id);
	if (ret != 0) {
		nvt_dbg(ERR, "failed in release buffer\n");
		return -1;
	}
#endif


	return ret;
}

#define NUE2_WRITEPROT_NUM  1
VOID nue2_write_protect_test(UINT8 wp_en, UINT8 sel_id, NUE2_PARM *p_param)
{
	DMA_WRITEPROT_ATTR attr = {0};
	UINT32 dram_mode;

	if (g_nue2_dram_mode == 2) {
		dram_mode = 1; //dram_2
	} else {
		dram_mode = 0; //dram_1
	}

	if (wp_en == 1) { //in range protect
		//input
		DBG_EMU( "WP: in range protect (DMA_RPLEL_UNREAD/DMA_PROT_IN)\r\n");
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RPLEL_UNREAD;
		attr.protect_mode = DMA_PROT_IN;

		DBG_EMU( "WP_in_range_p:sel_id=%d\r\n", sel_id);

		if (sel_id == 0 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_0 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in0));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_in1 - p_param->dmaio_addr.addr_in0 - 4);
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_0): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 1 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_1 = 1;
			attr.protect_rgn_attr[1].en = 1;
			attr.protect_rgn_attr[1].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in1));  //physical addressA
			attr.protect_rgn_attr[1].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_in2 - p_param->dmaio_addr.addr_in1 - 4);
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_1): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 2 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_2 = 1;
			attr.protect_rgn_attr[2].en = 1;
			attr.protect_rgn_attr[2].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in2));  //physical addressA
			attr.protect_rgn_attr[2].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out0 - p_param->dmaio_addr.addr_in2 - 4);
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_2): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}


		DBG_EMU( "WP: in range protect (DMA_WPLEL_UNWRITE/DMA_PROT_IN)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_UNWRITE;
        attr.protect_mode = DMA_PROT_IN;

		if (sel_id == 3 || sel_id == 5) {
			attr.mask.NUE2_3 = 1;	
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out1 - p_param->dmaio_addr.addr_out0 - 4);
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 4 || sel_id == 5) {
			attr.mask.NUE2_4 = 1;
			attr.protect_rgn_attr[1].en = 1;
			attr.protect_rgn_attr[1].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out1));  //physical addressA
			attr.protect_rgn_attr[1].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out2 - p_param->dmaio_addr.addr_out1 - 4);
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_4): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 7 || sel_id == 5) {
			attr.mask.NUE2_5 = 1;
			attr.protect_rgn_attr[2].en = 1;
			attr.protect_rgn_attr[2].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out2));  //physical addressA
			attr.protect_rgn_attr[2].size = NUE2_256_FLOOR(nue2_dram_out_size2);
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_5): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

	}

	if (wp_en == 2) { //out range protect read
		DBG_EMU( "WP: out range protect read (DMA_RPLEL_UNREAD/DMA_PROT_OUT)\r\n");
		//input
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.mask.NUE2_0 = 1;
		attr.level = DMA_RPLEL_UNREAD;
        attr.protect_mode = DMA_PROT_OUT;
	
		attr.protect_rgn_attr[0].en = 1;	
		attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in0 - 0x1000));  //physical addressA
		attr.protect_rgn_attr[0].size = 0x100;
		arb_enable_wp(dram_mode, WPSET_0, &attr);
		DBG_EMU( "wp_out_range_p(OUT_WP): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
	}

	if (wp_en == 3) { //out range protect write
		DBG_EMU( "WP: out range protect write (DMA_WPLEL_UNWRITE/DMA_PROT_OUT) \r\n");
		//Output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_UNWRITE;
        attr.protect_mode = DMA_PROT_OUT;

		if (sel_id == 0) {
			attr.mask.NUE2_3 = 1;
			attr.protect_rgn_attr[0].en = 1;	
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0 - 0x1000));  //physical addressA out range
			attr.protect_rgn_attr[0].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_0, &attr);
		} else if (sel_id == 1) {
			attr.mask.NUE2_4 = 1;
			attr.protect_rgn_attr[0].en = 1;	
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0 - 0x1000));  //physical addressA out range
			attr.protect_rgn_attr[0].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_0, &attr);
		}
		DBG_EMU( "wp_out_range_p(OUT_WP): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
	
	}

	if (wp_en == 4) { //in range read/write denial access
		DBG_EMU( "WP: in range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_IN)\r\n");

		if (sel_id == 0) {
			//input
			memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
			attr.level = DMA_RWPLEL_UNRW;
			attr.protect_mode = DMA_PROT_IN;

			attr.mask.NUE2_0 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in0));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_in1 - p_param->dmaio_addr.addr_in0 - 4);
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_0): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);

			attr.mask.NUE2_1 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in1));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_in2 - p_param->dmaio_addr.addr_in1 - 4);
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_1): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);

			attr.mask.NUE2_2 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in2));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out0 - p_param->dmaio_addr.addr_in2 - 4);
			arb_enable_wp(dram_mode, WPSET_2, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_2): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);


			DBG_EMU( "WP: in range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_IN)\r\n");
		} if (sel_id == 1) {
			//output
			memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
			attr.level = DMA_RWPLEL_UNRW;
			attr.protect_mode = DMA_PROT_IN;

			attr.mask.NUE2_3 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out1 - p_param->dmaio_addr.addr_out0 - 4);
			arb_enable_wp(dram_mode, WPSET_3, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);

			attr.mask.NUE2_4 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out1));  //physical addressA
			attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out2 - p_param->dmaio_addr.addr_out1 - 4);
			arb_enable_wp(dram_mode, WPSET_4, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_4): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

	}

	if (wp_en == 5) { //out range read/write denial access
		DBG_EMU( "WP: out range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_OUT)\r\n");
		//input
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RWPLEL_UNRW;
        attr.protect_mode = DMA_PROT_OUT;

		if (sel_id == 0 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_0 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in0 - 0x1000));  //physical addressA
			attr.protect_rgn_attr[0].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_0): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 1 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_1 = 1;
			attr.protect_rgn_attr[1].en = 1;
			attr.protect_rgn_attr[1].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in1 - 0x1000));  //physical addressA
			attr.protect_rgn_attr[1].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_1): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 2 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_2 = 1;
			attr.protect_rgn_attr[2].en = 1;
			attr.protect_rgn_attr[2].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_in2 - 0x1000));  //physical addressA
			attr.protect_rgn_attr[2].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_0, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_2): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		DBG_EMU( "WP: out range read/write denial access (DMA_RWPLEL_UNRW/DMA_PROT_OUT)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_RWPLEL_UNRW;
        attr.protect_mode = DMA_PROT_OUT;

		if (sel_id == 3 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_3 = 1;
			attr.protect_rgn_attr[0].en = 1;
			attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0 - 0x1000));  //physical addressA
			attr.protect_rgn_attr[0].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 4 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_4 = 1;
			attr.protect_rgn_attr[1].en = 1;
			attr.protect_rgn_attr[1].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out1 - 0x1000));  //physical addressA
			attr.protect_rgn_attr[1].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_4): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

		if (sel_id == 7 || sel_id == 5 || sel_id == 6) {
			attr.mask.NUE2_5 = 1;
			attr.protect_rgn_attr[1].en = 1;
			attr.protect_rgn_attr[1].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out2 - 0x1000));  //physical addressA
			attr.protect_rgn_attr[1].size = 0x100;
			arb_enable_wp(dram_mode, WPSET_1, &attr);
			DBG_EMU( "wp_in_range_p(WPSET_4): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
		}

	}

	if (wp_en == 6) { //in range write detect
		DBG_EMU( "WP: in range write detect (DMA_WPLEL_DETECT/DMA_PROT_IN)\r\n");
		//output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_DETECT;
        attr.protect_mode = DMA_PROT_IN;

		attr.mask.NUE2_3 = 1;
		attr.protect_rgn_attr[0].en = 1;
		attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0));  //physical addressA
		attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out1 - p_param->dmaio_addr.addr_out0 - 4);
		arb_enable_wp(dram_mode, WPSET_0, &attr);
		DBG_EMU( "wp_in_range_p(WPSET_3): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);

		attr.mask.NUE2_4 = 1;
		attr.protect_rgn_attr[0].en = 1;
		attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out1));  //physical addressA
		attr.protect_rgn_attr[0].size = NUE2_256_FLOOR(p_param->dmaio_addr.addr_out2 - p_param->dmaio_addr.addr_out1 - 4);
		arb_enable_wp(dram_mode, WPSET_1, &attr);
		DBG_EMU( "wp_in_range_p(WPSET_4): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);

	}

	if (wp_en == 7) { //out range write detect
		DBG_EMU( "WP: out range write detect (DMA_WPLEL_DETECT/DMA_PROT_OUT)\r\n");
		//Output
		memset((void *)&attr.mask, 0x0, sizeof(DMA_CH_MSK));
		attr.level = DMA_WPLEL_DETECT;
        attr.protect_mode = DMA_PROT_OUT;

		attr.mask.NUE2_3 = 1;	
		attr.protect_rgn_attr[0].en = 1;	
		attr.protect_rgn_attr[0].starting_addr = nue2_pt_va2pa(NUE2_256_CEILING(p_param->dmaio_addr.addr_out0 - 0x1000));  //physical addressA
		attr.protect_rgn_attr[0].size = 0x100;
		arb_enable_wp(dram_mode, WPSET_0, &attr);
		DBG_EMU( "wp_out_range_p(OUT_WP): addr=0x%x size=0x%x\r\n", attr.protect_rgn_attr[0].starting_addr, attr.protect_rgn_attr[0].size);
	}
}

static VOID nue2_set_heavy_func(UINT8 heavy_mode, UINT8 dram_mode)
{
#if defined(__FREERTOS)
	DRAM_CONSUME_ATTR attr;
	UINT32 heavy_load_size = 0x1000000;
	UINT32 mem_base;
	UINT32 mem_size;
	
	memset((void *) &attr, 0x0, sizeof(DRAM_CONSUME_ATTR));

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

	if (mem_size > (heavy_load_size + g_mem_size_check)) {
		attr.size = heavy_load_size;
	} else {
		nvt_dbg(ERR, "Error, The memory size is not enough for heavy load.\r\n");
		return;
	}
	attr.addr = (mem_base + mem_size - heavy_load_size) & (~(0xFF));
	memset((void *)&attr.dma_channel, 0x0, sizeof(attr.dma_channel));

	if (heavy_mode == 1) {
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_0, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_1, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_2, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_3, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_4, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_5, attr.dma_channel);
		DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(DMA_CH_NUE2_6, attr.dma_channel);
	}

	if (dram_mode == 2) {
#if 0
		if (dram2_consume_cfg(&attr) != 0) {
			nvt_dbg(ERR, "Error to do dram2_consume_cfg(&attr).\r\n");
		}
		if (dram2_consume_start() != 0) {
			nvt_dbg(ERR, "Error to do dram2_consume_start().\r\n");
		}
#endif
	} else {
		if (dram_consume_cfg(&attr) != 0) {
			nvt_dbg(ERR, "Error to do dram_consume_cfg(&attr).\r\n");
		}
		if (dram_consume_start() != 0) {
			nvt_dbg(ERR, "Error to do dram_consume_start().\r\n");
		}
	}

	if (heavy_mode == 0) {
		nvt_dbg(ERR, "Start (DRAM_CONSUME_HEAVY_LOADING) at dram (0x%x)\r\n", attr.addr);
	} else {
		nvt_dbg(ERR, "Start (DRAM_CONSUME_CH_DISABLE) at dram (0x%x)\r\n", attr.addr);
	}
#endif

}

#if (EMU_AI == ENABLE)
#define POOL_ID_APP_ARBIT       0
#define POOL_ID_APP_ARBIT2      1
#define EMU_AFFINE_MEM_ID       (POOL_ID_APP_ARBIT)

#define Perf_Open()
#define Perf_Mark()
#define Perf_Close()
#define Perf_GetDuration()  (0)
#endif

#if defined(__FREERTOS)
void emu_nue2Main(char nue2_cmd[][10])
{
	CHAR ch=0;
	
    while (1) {
        nvt_dbg(ERR,"NUE2 test menu\r\n\r\n");
        nvt_dbg(ERR,"1. test patterns(NUE2)\r\n");
		nvt_dbg(ERR,"2. bring up enable(NUE2)\r\n");
		nvt_dbg(ERR,"3. Emulation flow main\r\n");
		nvt_dbg(ERR,"v. loop time(NUE2)\r\n");
		nvt_dbg(ERR,"x. loop mode(NUE2)\r\n");
        nvt_dbg(ERR,"z. back\r\n");
        nvt_dbg(ERR,"Input test item: ");

		uart_getChar(&ch);
		switch (ch) {
			case '1':
				if (g_bringup_en >= 1) {
					if (g_bringup_en == 2) {
						ive_set_bringup(2);
					} else {
						ive_set_bringup(1);
					}

					nue2_set_dram_mode(1);
					nue2_set_ll_mode_en(1);
					nvt_kdrv_ai_module_test(0, 3, nue2_cmd);
					
					ive_set_dram_mode(1);
					ive_set_ll_mode_en(1);
					nvt_kdrv_ipp_api_ive_test();

					nue2_set_dram_mode(2);
					nue2_set_ll_mode_en(1);
					nue2_set_heavy_mode(0);
					nvt_kdrv_ai_module_test(0, 3, nue2_cmd);
					
					ive_set_dram_mode(2);
					ive_set_ll_mode_en(1);
					ive_set_heavy_mode(0);
					nvt_kdrv_ipp_api_ive_test();
										
				} else {
					ive_set_bringup(0);
					nvt_kdrv_ai_module_test(0, 3, nue2_cmd);
				}
				break;
			case '2':
				g_bringup_en++;

				if (g_bringup_en > 2) {
					g_bringup_en = 0;
				}

				DBG_ERR("NUE2: bringup now(%d)\r\n", g_bringup_en);
				break;
  			case '3':
#if (EMUF_ENABLE == 1)
                emu_emufMain(0);
#endif
                break;
			case 'v':
#if defined(__FREERTOS)
				nvt_dbg(ERR, "Please enter loop number: ");
				cStrLen = NUE2_MAX_STR_LEN;
				uart_getString(cEndStr, &cStrLen);
				g_loop_time = atoi(cEndStr);
				nvt_dbg(ERR, "strip number test:%d\r\n", g_s_num);
#else
				g_loop_time = 1;
				nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
            	break;
			case 'x':
				g_loop_mode++;
				if (g_loop_mode >= 3) {
					g_loop_mode = 0;
				}
				nvt_dbg(ERR, "loop_mode: now(%d), 0:no loop, 1:while+CLK_switch  2:while_CLK_always_on\r\n", g_loop_mode);

				break;
			case 'z':
				goto exit;
			default:
				break;
		}
	}

exit:
	return;
}
#endif


void emu_aiMain(UINT32 emu_id)
{
#if (EMU_AI == ENABLE)
	CHAR ch=0;
#if 0
	UINT32 uiPoolAddr;
#endif

#if defined(__FREERTOS)
	char nue2_cmd[3][10]={"nue2","1","1"};
	char nue_cmd[3][10]={"nue","1","1"};
#endif

	UINT32 idx;


#if defined(__FREERTOS)
	DBG_EMU( "WW###:input emu_id=%d\n", emu_id);

    DBG_EMU( "WW###:addr=%p\n", nue2_cmd);
	DBG_EMU( "WW###:addr1=%p\n", nue2_cmd[0]);
	DBG_EMU( "WW###:addr2=%p\n", nue2_cmd[1]);
	DBG_EMU( "WW###:addr3=%p\n", nue2_cmd[2]);

	DBG_EMU( "WW###:addr=%p\n", nue_cmd);
	DBG_EMU( "WW###:addr1=%p\n", nue_cmd[0]);
	DBG_EMU( "WW###:addr2=%p\n", nue_cmd[1]);
	DBG_EMU( "WW###:addr3=%p\n", nue_cmd[2]);
#endif



//	clk_open();
//	clk_change_cpu_ahb(480, 480);
//	clk_close();

	DBG_EMU( "WW###:1\n");

	/*
	    pinmux_select_debugport(PINMUX_DEBUGPORT_GROUP2|PINMUX_DEBUGPORT_AFFINE);
	    {
	        volatile UINT32* pAffDbgReg = (UINT32*)0xB0CA00FC;

	        *pAffDbgReg = 1;

	        // DBG[13] will be chafn_in_req
	    }

	    gpio_setDir(AFF_TRIG_GPIO, GPIO_DIR_OUTPUT);
	    gpio_clearPin(AFF_TRIG_GPIO);
	*/
	DBG_EMU("AI Test Program\r\n");

	DBG_EMU( "WW###:6\n");

	for (idx = 0; idx < NUE2_ALL_CASES; idx++) {
		nue2_bringup_table_all[idx] = idx + 1;
	}

	while (1) {
		nvt_dbg(ERR,"NUE2 test menu\r\n\r\n");
		nvt_dbg(ERR,"1. test patterns(NUE2)\r\n");
		nvt_dbg(ERR,"2. test patterns(NUE)\r\n");
		nvt_dbg(ERR,"3. fill register onlyne\r\n");
		nvt_dbg(ERR,"4. cycle count: hw only(no ll cmd) test\r\n");
		nvt_dbg(ERR,"5. dump register test\r\n");
		nvt_dbg(ERR,"6. ll_test_2:\r\n");
		nvt_dbg(ERR,"7. ll_test_4 (1: normal 2: error cmd):\r\n");
		nvt_dbg(ERR,"8. dump_ll_buf:\r\n");
		nvt_dbg(ERR,"9. ll_big_buf:\r\n");
		nvt_dbg(ERR,"a. auto_clk:\r\n");
		nvt_dbg(ERR,"b. LL(is_bit60):\r\n");
		nvt_dbg(ERR,"c. LL(base address):\r\n");
		nvt_dbg(ERR,"d. dram_mode:\r\n");
		nvt_dbg(ERR,"e. priority_mode:\r\n");
		nvt_dbg(ERR,"f. clock enable:\r\n");
		nvt_dbg(ERR,"g. golden compare test(1:Enable, 0:disable)\r\n");
		nvt_dbg(ERR,"h. heavy load enable\r\n");
		nvt_dbg(ERR,"i. interrupt enable\r\n");
		nvt_dbg(ERR,"j. ll_fill_mreg_num(rigster num)\r\n");
		nvt_dbg(ERR,"k. ll_fill_num(how many linked-list)\r\n");
		nvt_dbg(ERR,"l. Linked-list mode test\r\n");
		nvt_dbg(ERR,"m. sram shutdown\r\n");
		nvt_dbg(ERR,"n. no stop\r\n");
		nvt_dbg(ERR,"o. random clock\r\n");
		nvt_dbg(ERR,"p. write protect\r\n");
		nvt_dbg(ERR,"q. switch dram:\r\n");
		nvt_dbg(ERR,"r. burst length\r\n");
		nvt_dbg(ERR,"s. stripe number test\r\n");
		nvt_dbg(ERR,"t. ll_terminate test(1: retry, 2:no retry)\r\n");
		nvt_dbg(ERR,"u. sw_hw_rst_test\r\n");
		nvt_dbg(ERR,"v. dram_outrange\r\n");
		nvt_dbg(ERR,"w. rand combind mode\r\n");
		nvt_dbg(ERR,"x. register R/W\r\n");
		nvt_dbg(ERR,"y. heavy mode\r\n");
		nvt_dbg(ERR,"z. back\r\n");
		nvt_dbg(ERR,"Input test item: ");

#if defined(__FREERTOS)
		uart_getChar(&ch);
#else
		DBG_EMU( "Error,TODO....2\r\n");
#endif
		switch (ch) {
		case '1':
#if defined(__FREERTOS)
			emu_nue2Main(nue2_cmd);
#endif
			break;
		case '2':
#if defined(__FREERTOS)
			nvt_kdrv_ai_module_test(0, 3, nue_cmd);
#else
			DBG_EMU( "Error,TODO....3\r\n");
#endif
			break;
		case '3':
			if (g_fill_reg_only == 0) {
				g_fill_reg_only = 1;
			} else {
				g_fill_reg_only = 0;
			}
			nvt_dbg(ERR, "g_fill_reg_only(NOW:%d) \r\n", g_fill_reg_only);
			break;
		case '4':
			if (g_cnt_is_hw_only == 0) {
				g_cnt_is_hw_only = 1;
			} else {
				g_cnt_is_hw_only = 0;
			}
			nvt_dbg(ERR, "cycle count: hw only (no ll command)(NOW:%d) \r\n", g_cnt_is_hw_only);
			break;
		case '5':
			if (g_is_reg_dump == 0) {
				g_is_reg_dump = 1;
			} else {
				g_is_reg_dump = 0;
			}
			nvt_dbg(ERR, "dump reg (NOW:%d) \r\n", g_is_reg_dump);
			break;
		case '6':
			g_ll_test_2++;
			if (g_ll_test_2 >= 3) {
				g_ll_test_2 = 0;
			}
			nvt_dbg(ERR, "ll_test_2:(NOW:%d) \r\n", g_ll_test_2);
			break;
		case '7':
			g_is_ll_next_update++;
			if (g_is_ll_next_update >= 3) {
				g_is_ll_next_update = 0;
			}
			nvt_dbg(ERR, "ll_test_4:(NOW:%d) \r\n", g_is_ll_next_update);
			break;
		case '8':
			if (g_dump_ll_buf == 0) {
				g_dump_ll_buf = 1;
			} else {
				g_dump_ll_buf = 0;
			}
			nvt_dbg(ERR, "dump_ll_buf. NOW(%d)\r\n", g_dump_ll_buf);
			break;
		case '9':
			if (g_ll_big_buf == 0) {
				g_ll_big_buf = 1;
			} else {
				g_ll_big_buf = 0;
			}
			nvt_dbg(ERR, "ll_big_buf: NOW(%d)\r\n", g_ll_big_buf);
			break;
		case 'a':
			if (g_auto_clk == 0) {
				g_auto_clk = 1;
			} else {
				g_auto_clk = 0;
			}
			nvt_dbg(ERR, "auto clock gating: NOW(%d)\r\n", g_auto_clk);
			break;
		case 'b':
			if (g_is_bit60 == 0) {
				g_is_bit60 = 1;
			} else {
				g_is_bit60 = 0;
			}
			nvt_dbg(ERR, "is_bit60: NOW(%d)\r\n", g_is_bit60);
			break;
		case 'c':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the ll base address: ");
            cStrLen = NUE2_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_ll_base_addr = atoi(cEndStr);
			nvt_dbg(ERR, "ll base address (now=%d)\r\n", g_ll_base_addr);
#else
            g_ll_base_addr = 0x0;
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif

			break;
		case 'd':
			if (g_nue2_dram_mode == 2) {
				g_nue2_dram_mode = 1;
			} else {
				g_nue2_dram_mode = 2;
			}
			nvt_dbg(ERR, "dram_mode, NOW(%d)\r\n", g_nue2_dram_mode);
			break;
		case 'e':
			//DMA_PRIORITY_LOW,           // Low priority (Default value)
    		//DMA_PRIORITY_MIDDLE,        // Middle priority
    		//DMA_PRIORITY_HIGH,          // High priority
    		//DMA_PRIORITY_SUPER_HIGH,    // Super high priority (Only DMA_CH_SIE_XX or DMA_CH_SIE2_XX are allowed)
			g_pri_mode++;
			if (g_pri_mode >= 5) {
				g_pri_mode = 0;
			}
			nvt_dbg(ERR, "priority mode: NOW(%d) 0:LOW 1:MIDD 2: HIGH 3: SUPER 4:rand\r\n", g_pri_mode);
			break;
		case 'f':
			g_clk_en++;
			if (g_clk_en >= 4) {
				g_clk_en = 0;
			}
			nvt_dbg(ERR, "clock enable: NOW(%d) 1:delay(enable/disable) 2:fail 3:ok test\r\n", g_clk_en);
			break;
		case 'g':
			if (g_gld_cmp == 0) {
				g_gld_cmp = 1;
			} else {
				g_gld_cmp = 0;
			}
			nvt_dbg(ERR, "golden compare, NOW(%d)\r\n", g_gld_cmp);
			break;
		case 'h':
			nue2_set_heavy_func(g_is_heavy, g_nue2_dram_mode);
			break;
		case 'i':
			if (g_interrupt_en == 0) {
				g_interrupt_en = 1;
			} else {
				g_interrupt_en = 0;
			}
			nvt_dbg(ERR, "interrupt enable, NOW(%d)", g_interrupt_en);
			break;
		case 'j':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the ll register number: ");
            cStrLen = NUE2_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_ll_fill_reg_num = atoi(cEndStr);
			nvt_dbg(ERR, "ll register num (now=%d)\r\n", g_ll_fill_reg_num);
#else
            g_ll_fill_reg_num = 0;
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
			break;
		case 'k':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the ll fill number: ");
            cStrLen = NUE2_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_ll_fill_num = atoi(cEndStr);
			nvt_dbg(ERR, "ll_fill_num (now=%d)\r\n", g_ll_fill_num);
#else
            g_ll_fill_num = 0;
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
			break;
		case 'l':
			if (g_ll_mode_en == 0) {
				g_ll_mode_en = 1;
			} else {
				g_ll_mode_en = 0;
			}
			nvt_dbg(ERR, "Linked-list mode enable (now=%d)\r\n", g_ll_mode_en);
			break;
		case 'n':
			if (g_no_stop == 0) {
				g_no_stop = 1;
			} else {
				g_no_stop = 0;
			}
			nvt_dbg(ERR, "no stop, NOW(%d)\r\n", g_no_stop);
			break;
		case 'm':
			g_sram_down++;
			if (g_sram_down >= 3) {
				g_sram_down = 0;
			}
			nvt_dbg(ERR, "sram_down: NOW(%d), 1:ON 2:OFF\r\n", g_sram_down);
			break;
		case 'o':
			g_clock++;
			if (g_clock >= 5) {
				g_clock = 0;
			}
			nvt_dbg(ERR, "g_clock: NOW(%d) 0:600, 1:480, 2:320, 3:240 4:rand\r\n", g_clock);
			break;
		case 'p':
#if defined(__FREERTOS)
			nvt_dbg(ERR, "Please enter the wp_en number: ");
            cStrLen = NUE2_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_wp_en = atoi(cEndStr);
			nvt_dbg(ERR, "wp_en (now=%d)\r\n", g_wp_en);


            nvt_dbg(ERR, "Please enter the wp_sel_id number: ");
            cStrLen = NUE2_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_wp_sel_id = atoi(cEndStr);
			nvt_dbg(ERR, "wp_sel_id (now=%d)\r\n", g_wp_sel_id);
#else
            g_wp_sel_id = 0;
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif

			break;
        case 'q':
			if (g_is_switch_dram == 1) {
				g_is_switch_dram = 0;
			} else {
				g_is_switch_dram = 1;
			}
			nvt_dbg(ERR, "switch_dram: NOW(%d)\r\n", g_is_switch_dram);
			break;
		case 'r':
			g_rand_burst++;
			if (g_rand_burst >= 5) {
				g_rand_burst = 0;
			}
			nvt_dbg(ERR, "burst_length: NOW(%d), 0~3:normal, 4:rand\r\n", g_rand_burst);
			break;
		case 's':
#if defined(__FREERTOS)
            nvt_dbg(ERR, "Please enter the strip number: ");
            cStrLen = NUE2_MAX_STR_LEN;
            uart_getString(cEndStr, &cStrLen);
            g_s_num = atoi(cEndStr);
			nvt_dbg(ERR, "stripe number (now=%d)\r\n", g_s_num);
#else
            g_s_num = 3;
            nvt_dbg(ERR, "Error, Please enter the strip number: TODO...\r\n");
#endif
            break;
		case 't':
			g_ll_terminate++;
			if (g_ll_terminate >= 3) {
				g_ll_terminate = 0;
			} 
			nvt_dbg(ERR, "LL_terminate: NOW(%d)\r\n", g_ll_terminate);
			break;
		case 'u':
			g_hw_rst_en++;
			if (g_hw_rst_en  >= 7) {
				g_hw_rst_en = 0;
			}
			nvt_dbg(ERR, "hw_rst_en: NOW(%d) 1:hw, 2:sw 3:fail 4:fail 5:off 6:rand\r\n", g_hw_rst_en);
			break;
		case 'v':
			g_nue2_dram_outrange++;
			if (g_nue2_dram_outrange >= 3) {
				g_nue2_dram_outrange = 0;
			}
			nvt_dbg(ERR, "dram_outrange: NOW(%d)\r\n", g_nue2_dram_outrange);
			break;
		case 'w':
			if (g_rand_comb == 1) {
				g_rand_comb = 0;
			} else {
				g_rand_comb = 1;
			}
			nvt_dbg(ERR, "rand_combind: NOW(%d)\r\n", g_rand_comb);
			break;
		case 'x':
			if (g_reg_rw == 1) {
				g_reg_rw = 0;
			} else {
				g_reg_rw = 1;
			}
			nvt_dbg(ERR, "NUE2_REG_RW: NOW(%d)\r\n", g_reg_rw);
			break;
		case 'y':
			g_is_heavy++;
			if (g_is_heavy >= 2) {
				g_is_heavy = 0;
			}
			nvt_dbg(ERR, "heavy mode: NOW(%d)\r\n", g_is_heavy);
			break;
		case 'z':
			return;
		default:
			break;
		}
	}

#else
#endif
}
#else
void emu_aiMain(UINT32 emu_id)
{
	nvt_dbg(ERR, "NUE2: Error, please set #define NUE2_AI_FLOW DISABLE in nue2_platform.h.\r\n");
	return;	
}
#endif //#if (NUE2_SYS_VFY_EN == ENABLE)

