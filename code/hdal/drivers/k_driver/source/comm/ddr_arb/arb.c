/*
    Arbiter module driver

    This file is the driver of arbiter module

    @file       ddr_arb.c
    @ingroup    miDrvComm_Arb
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/****************************************************************************/
/*                                                                          */
/*  Todo: Modify register accessing code to the macro in RCWMacro.h         */
/*                                                                          */
/****************************************************************************/

#include "ddr_arb_int.h"
#include "ddr_arb_reg.h"


#define HEAVY_LOAD_CTRL_OFS(ch)         (DMA_CHANNEL0_HEAVY_LOAD_CTRL_OFS + ((ch) * 0x10))
#define HEAVY_LOAD_ADDR_OFS(ch)         (DMA_CHANNEL0_HEAVY_LOAD_START_ADDR_OFS + ((ch) * 0x10))
#define HEAVY_LOAD_SIZE_OFS(ch)         (DMA_CHANNEL0_HEAVY_LOAD_DMA_SIZE_OFS + ((ch) * 0x10))
#define HEAVY_LOAD_WAIT_CYCLE_OFS(ch)   (DMA_CHANNEL0_HEAVY_LOAD_WAIT_CYCLE_OFS + ((ch) * 0x10))

#define PROTECT_START_ADDR_OFS(ch)      (DMA_PROTECT_STARTADDR0_REG0_OFS+(ch)*8)
#define PROTECT_END_ADDR_OFS(ch)        (DMA_PROTECT_STOPADDR0_REG0_OFS+(ch)*8)
#define PROTECT_CH_MSK0_OFS(ch)         (DMA_PROTECT_RANGE0_MSK0_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK1_OFS(ch)         (DMA_PROTECT_RANGE0_MSK1_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK2_OFS(ch)         (DMA_PROTECT_RANGE0_MSK2_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK3_OFS(ch)         (DMA_PROTECT_RANGE0_MSK3_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK4_OFS(ch)         (DMA_PROTECT_RANGE0_MSK4_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK5_OFS(ch)         (DMA_PROTECT_RANGE0_MSK5_REG_OFS+(ch)*32)

#if defined __KERNEL__
typedef enum _DMA_CH {
	DMA_CH_RSV = 0x00,
	DMA_CH_FIRST_CHANNEL,
	DMA_CH_CPU = DMA_CH_FIRST_CHANNEL,

	DMA_CH_ISE_a0 = 0x46,          // ISE input
	DMA_CH_ISE_a1 = 0x47,          // ISE output

	DMA_CH_CNN_0 = 0x62,           // CNN input
	DMA_CH_CNN_1 = 0x63,           // CNN input
	DMA_CH_CNN_2 = 0x64,           // CNN input
	DMA_CH_CNN_3 = 0x65,           // CNN input
	DMA_CH_CNN_4 = 0x66,           // CNN output
	DMA_CH_CNN_5 = 0x67,           // CNN output
	DMA_CH_NUE_0 = 0x68,           // NUE input
	DMA_CH_NUE_1 = 0x69,           // NUE input
	DMA_CH_NUE_2 = 0x6a,           // NUE output
	DMA_CH_NUE2_0 = 0x6b,          // NUE input
	DMA_CH_NUE2_1 = 0x6c,          // NUE input
	DMA_CH_NUE2_2 = 0x6d,          // NUE input
	DMA_CH_NUE2_3 = 0x6e,          // NUE output
	DMA_CH_NUE2_4 = 0x6f,			// NUE output

	DMA_CH_NUE2_5 = 0x70,			// NUE output
	DMA_CH_CNN2_0 = 0x7e,
	DMA_CH_CNN2_1 = 0x7f,

	DMA_CH_CNN2_2 = 0x80,
	DMA_CH_CNN2_3 = 0x81,
	DMA_CH_CNN2_4 = 0x82,
	DMA_CH_CNN2_5 = 0x83,

	DMA_CH_CNN_6 = 0x95,
	DMA_CH_CNN2_6 = 0x96,
	DMA_CH_NUE_3 = 0x97,
	DMA_CH_NUE2_6 = 0x98,
	DMA_CH_ISE_2 = 0x99,
	DMA_CH_COUNT,
	DMA_CH_ALL = 0x80000000,

	ENUM_DUMMY4WORD(DMA_CH)
} DMA_CH;
#endif

const static CHAR *dma_wp_engine_name[] = {
	"CPU",
	"USB",
	"USB_1",
	"DCE_0",
	"DCE_1",
	"DCE_2",
	"DCE_3",
	"DCE_4",
	"DCE_5",
	"DCE_6",
	"GRA_0",
	"GRA_1",
	"GRA_2",
	"GRA_3",
	"GRA_4",
	// Ctrl 0
	"GRA2_0",
	"GRA2_1",
	"GRA2_2",
	"JPG0",            ///< JPG IMG
	"JPG1",            ///< JPG BS
	"JPG2",            ///< JPG Encode mode DC output
	"IPE0",
	"IPE1",
	"IPE2",
	"IPE3",
	"IPE4",
	"IPE5",
	"IPE6",
	"SIE_0",
	"SIE_1",
	"SIE_2",
	// Ctrl 1
	"SIE_3",
	"SIE2_0",
	"SIE2_1",
	"SIE2_2",
	"SIE2_3",
	"SIE3_0",
	"SIE3_1",
	"DIS_0",
	"DIS_1",
	"LARB: SIF/I2C/I2C2/I2C3/UART2/UART3/SPI/SDF",
	"DAI",
	"IFE_0",
	"IFE_1",
	"IFE_2",
	"IME_0",
	"IME_1",
	// Ctrl 2
	"IME_2",
	"IME_3",
	"IME_4",
	"IME_5",
	"IME_6",
	"IME_7",
	"IME_8",
	"IME_9",
	"IME_A",
	"IME_B",
	"IME_C",
	"IME_D",
	"IME_E",
	"IME_F",
	"IME_10",
	"IME_11",
	// Ctrl 3
	"IME_12",
	"IME_13",
	"IME_14",
	"IME_15",
	"IME_16",
	"IME_17",
	"ISE_a0",
	"ISE_a1",
	"IDE_a0",
	"IDE_b0",
	"IDE_a1",
	"IDE_b1",
	"IDE_6",
	"IDE_7",
	"SDIO",
	"SDIO2",
	// Ctrl 4
	"SDIO3",
	"NAND",
	"H264_0",
	"H264_1",
	"H264_3",
	"H264_4",
	"H264_5",
	"H264_6",
	"H264_7",
	"H264_8",
	"H264_9(COE)",
	"IFE2_0",
	"IFE2_1",
	"Ethernet",
	"TS Mux",
	"TS Mux 1",
	// Ctrl 5
	"CRYPTO",
	"HASH",
	"CNN_0",
	"CNN_1",
	"CNN_2",
	"CNN_3",
	"CNN_4",
	"CNN_5",
	"NUE_0",
	"NUE_1",
	"NUE_2",
	"NUE2_0",
	"NUE2_1",
	"NUE2_2",
	"NUE2_3",
	"NUE2_4",
	// Ctrl 6
	"NUE2_5",
	"MDBC_0",
	"MDBC_1",
	"MDBC_2",
	"MDBC_3",
	"MDBC_4",
	"MDBC_5",
	"MDBC_6",
	"MDBC_7",
	"MDBC_8",
	"MDBC_9",
	"HLOAD_0",
	"HLOAD_1",
	"HLOAD_2",
	"CNN2_0",
	"CNN2_1",
	// Ctrl 7
	"CNN2_2",
	"CNN2_3",
	"CNN2_4",
	"CNN2_5",
	"DCE_7",
	"AFN_0",
	"AFN_1",
	"IVE_0",
	"IVE_1",
	"SIE4_0", // new channel in 528(137)
	"SIE4_1",
	"SIE4_2",
	"SIE4_3",
	"SIE5_0",
	"SIE5_1",
	"SIE5_2",
	// Ctrl 8
	"SIE5_3",
	"SDE_0",
	"SDE_1",
	"SDE_2",
	"SDE_3",
	"CNN_6",
	"CNN2_6",
	"NUE_3",
	"NUE2_6",
	"ISE_2",
	"LARB_2: UART4/UART5/UART6/I2C4/I2C5/SPI2/SPI3/SPI4/SPI5",
	"VPE_0",
	"VPE_1",
	"VPE_2",
	"VPE_3",
	"VPE_4",
	// Ctrl 9
	"SDE_4",
	"",
	"",
	"",
};

void arb_isr(int is_dprof);
void arb2_isr(int is_dprof);

static UINT64 data_counter[2][8] = {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}};
static int is_used[2][8] = {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}};


// Internal APIs
void arb_isr(int is_dprof)
{
	T_DMA_PROTECT_INTSTS_REG sts_reg;
	T_DMA_PROTECT0_CHSTS_REG ch_reg0;
	T_DMA_PROTECT1_CHSTS_REG ch_reg1;
	T_DMA_PROTECT2_CHSTS_REG ch_reg2;
	T_DMA_PROTECT3_CHSTS_REG ch_reg3;
	T_DMA_PROTECT4_CHSTS_REG ch_reg4;
	T_DMA_PROTECT5_CHSTS_REG ch_reg5;
	T_DMA_PROTECT_CTRL_REG ctrl_reg;
	T_DMA_OUT_RANGE_CHANNEL_REG out_range_ch_reg;
	T_DMA_OUT_RANGE_ADDR_REG out_range_addr_reg;
	UINT32 i = 0;

	sts_reg.reg = ARB_GETREG(DMA_PROTECT_INTSTS_REG_OFS);
	out_range_ch_reg.reg = ARB_GETREG(DMA_OUT_RANGE_CHANNEL_REG_OFS);
	out_range_addr_reg.reg = ARB_GETREG(DMA_OUT_RANGE_ADDR_REG_OFS);

	ARB_SETREG(DMA_PROTECT_INTSTS_REG_OFS, sts_reg.reg);

	if (sts_reg.reg == 0) return;

	//out_range_ch_reg.reg = ARB_GETREG(DMA_OUT_RANGE_CHANNEL_REG_OFS);
	//out_range_addr_reg.reg = ARB_GETREG(DMA_OUT_RANGE_ADDR_REG_OFS);

	if (sts_reg.bit.DMA_OUTRANGE_STS) {
		if ((out_range_ch_reg.bit.DRAM_OUTRANGE_CH == 0) ||
			(out_range_ch_reg.bit.DRAM_OUTRANGE_CH >= DMA_WPCH_COUNT)) {
			MY_PANIC(DBG_COLOR_MAGENTA"%s: Out range addr 0x%x, %x\r\n"DBG_COLOR_END, __func__, (unsigned int)out_range_addr_reg.bit.DRAM_OUTRANGE_ADDR, out_range_ch_reg.bit.DRAM_OUTRANGE_CH);
		} else {
			MY_PANIC(DBG_COLOR_MAGENTA"%s: Out range channel %s, addr 0x%x\r\n"DBG_COLOR_END, __func__, dma_wp_engine_name[out_range_ch_reg.bit.DRAM_OUTRANGE_CH - 1],
					(unsigned int)out_range_addr_reg.bit.DRAM_OUTRANGE_ADDR);
			if((out_range_ch_reg.bit.DRAM_OUTRANGE_CH - 1) == 0) {
				//OS_DumpCurrentBacktraceEx(0);
				//panic();
			}
		}
	}

	ch_reg0.reg = ARB_GETREG(DMA_PROTECT0_CHSTS_REG_OFS);
	ch_reg1.reg = ARB_GETREG(DMA_PROTECT1_CHSTS_REG_OFS);
	ch_reg2.reg = ARB_GETREG(DMA_PROTECT2_CHSTS_REG_OFS);
	ch_reg3.reg = ARB_GETREG(DMA_PROTECT3_CHSTS_REG_OFS);
	ch_reg4.reg = ARB_GETREG(DMA_PROTECT4_CHSTS_REG_OFS);
	ch_reg5.reg = ARB_GETREG(DMA_PROTECT5_CHSTS_REG_OFS);

	ctrl_reg.reg = PROT_GETREG(DMA_PROTECT_CTRL_REG_OFS);

	// set 0
	if (sts_reg.bit.DMA_WPWD0_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}
	if (sts_reg.bit.DMA_WPWD0_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}
	if (sts_reg.bit.DMA_WPWD0_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}
	if (sts_reg.bit.DMA_WPWD0_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}

	// set 1
	if (sts_reg.bit.DMA_WPWD1_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}
	if (sts_reg.bit.DMA_WPWD1_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}
	if (sts_reg.bit.DMA_WPWD1_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}
	if (sts_reg.bit.DMA_WPWD1_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}

	// set 2
	if (sts_reg.bit.DMA_WPWD2_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}
	if (sts_reg.bit.DMA_WPWD2_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}
	if (sts_reg.bit.DMA_WPWD2_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}
	if (sts_reg.bit.DMA_WPWD2_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}

	// set 3
	if (sts_reg.bit.DMA_WPWD3_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}
	if (sts_reg.bit.DMA_WPWD3_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}
	if (sts_reg.bit.DMA_WPWD3_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}
	if (sts_reg.bit.DMA_WPWD3_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}

	// set 4
	if (sts_reg.bit.DMA_WPWD4_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}
	if (sts_reg.bit.DMA_WPWD4_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}
	if (sts_reg.bit.DMA_WPWD4_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}
	if (sts_reg.bit.DMA_WPWD4_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}

	// set 5
	if (sts_reg.bit.DMA_WPWD5_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}
	if (sts_reg.bit.DMA_WPWD5_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}
	if (sts_reg.bit.DMA_WPWD5_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}
	if (sts_reg.bit.DMA_WPWD5_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}

    if (sts_reg.bit.DMA_UPDATE_USAGE_STS) {
#if defined __KERNEL__
		if (is_dprof) {
			dprof_bandwidth_monitor(DDR_ARB_1, 0);
		}
#endif
		for (i = 0; i < 8; i++) {
			data_counter[DDR_ARB_1][i] += ARB_GETREG(DMA_RECORD0_SIZE_REG_OFS + i * 8);
		}

	}
}

void arb2_isr(int is_dprof)
{
	T_DMA_PROTECT_INTSTS_REG sts_reg;
	T_DMA_PROTECT0_CHSTS_REG ch_reg0;
	T_DMA_PROTECT1_CHSTS_REG ch_reg1;
	T_DMA_PROTECT2_CHSTS_REG ch_reg2;
	T_DMA_PROTECT3_CHSTS_REG ch_reg3;
	T_DMA_PROTECT4_CHSTS_REG ch_reg4;
	T_DMA_PROTECT5_CHSTS_REG ch_reg5;
	T_DMA_PROTECT_CTRL_REG ctrl_reg;
	T_DMA_OUT_RANGE_CHANNEL_REG out_range_ch_reg;
	T_DMA_OUT_RANGE_ADDR_REG out_range_addr_reg;
	UINT32 i = 0;

	sts_reg.reg = ARB2_GETREG(DMA_PROTECT_INTSTS_REG_OFS);
	out_range_ch_reg.reg = ARB2_GETREG(DMA_OUT_RANGE_CHANNEL_REG_OFS);
	out_range_addr_reg.reg = ARB2_GETREG(DMA_OUT_RANGE_ADDR_REG_OFS);

	ARB2_SETREG(DMA_PROTECT_INTSTS_REG_OFS, sts_reg.reg);

	if (sts_reg.reg == 0) return;

	//out_range_ch_reg.reg = ARB2_GETREG(DMA_OUT_RANGE_CHANNEL_REG_OFS);
	//out_range_addr_reg.reg = ARB2_GETREG(DMA_OUT_RANGE_ADDR_REG_OFS);

	if (sts_reg.bit.DMA_OUTRANGE_STS) {
		if ((out_range_ch_reg.bit.DRAM_OUTRANGE_CH == 0) ||
			(out_range_ch_reg.bit.DRAM_OUTRANGE_CH >= DMA_WPCH_COUNT)) {
			MY_PANIC(DBG_COLOR_MAGENTA"%s: Out range addr 0x%x\r\n"DBG_COLOR_END, __func__, (unsigned int)out_range_addr_reg.bit.DRAM_OUTRANGE_ADDR);
		} else {
			MY_PANIC(DBG_COLOR_MAGENTA"%s: Out range channel %s, addr 0x%x\r\n"DBG_COLOR_END, __func__, dma_wp_engine_name[out_range_ch_reg.bit.DRAM_OUTRANGE_CH - 1],
					(unsigned int)out_range_addr_reg.bit.DRAM_OUTRANGE_ADDR);
			if((out_range_ch_reg.bit.DRAM_OUTRANGE_CH - 1) == 0) {
				//OS_DumpCurrentBacktraceEx(0);
			}
		}
	}

	ch_reg0.reg = ARB2_GETREG(DMA_PROTECT0_CHSTS_REG_OFS);
	ch_reg1.reg = ARB2_GETREG(DMA_PROTECT1_CHSTS_REG_OFS);
	ch_reg2.reg = ARB2_GETREG(DMA_PROTECT2_CHSTS_REG_OFS);
	ch_reg3.reg = ARB2_GETREG(DMA_PROTECT3_CHSTS_REG_OFS);
	ch_reg4.reg = ARB2_GETREG(DMA_PROTECT4_CHSTS_REG_OFS);
	ch_reg5.reg = ARB2_GETREG(DMA_PROTECT5_CHSTS_REG_OFS);

	ctrl_reg.reg = PROT2_GETREG(DMA_PROTECT_CTRL_REG_OFS);

	// set 0
	if (sts_reg.bit.DMA_WPWD0_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}
	if (sts_reg.bit.DMA_WPWD0_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}
	if (sts_reg.bit.DMA_WPWD0_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}
	if (sts_reg.bit.DMA_WPWD0_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(0, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg0.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD0_MODE, ctrl_reg.bit.DMA_WPWD0_SEL);
	}

	// set 1
	if (sts_reg.bit.DMA_WPWD1_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}
	if (sts_reg.bit.DMA_WPWD1_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}
	if (sts_reg.bit.DMA_WPWD1_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}
	if (sts_reg.bit.DMA_WPWD1_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(1, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg1.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD1_MODE, ctrl_reg.bit.DMA_WPWD1_SEL);
	}

	// set 2
	if (sts_reg.bit.DMA_WPWD2_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}
	if (sts_reg.bit.DMA_WPWD2_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}
	if (sts_reg.bit.DMA_WPWD2_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}
	if (sts_reg.bit.DMA_WPWD2_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(2, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg2.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD2_MODE, ctrl_reg.bit.DMA_WPWD2_SEL);
	}

	// set 3
	if (sts_reg.bit.DMA_WPWD3_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}
	if (sts_reg.bit.DMA_WPWD3_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}
	if (sts_reg.bit.DMA_WPWD3_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}
	if (sts_reg.bit.DMA_WPWD3_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(3, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg3.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD3_MODE, ctrl_reg.bit.DMA_WPWD3_SEL);
	}

	// set 4
	if (sts_reg.bit.DMA_WPWD4_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}
	if (sts_reg.bit.DMA_WPWD4_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}
	if (sts_reg.bit.DMA_WPWD4_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}
	if (sts_reg.bit.DMA_WPWD4_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(4, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg4.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD4_MODE, ctrl_reg.bit.DMA_WPWD4_SEL);
	}

	// set 5
	if (sts_reg.bit.DMA_WPWD5_STS0) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 0): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS0 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}
	if (sts_reg.bit.DMA_WPWD5_STS1) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 1): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS1 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}
	if (sts_reg.bit.DMA_WPWD5_STS2) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 2): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS2 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}
	if (sts_reg.bit.DMA_WPWD5_STS3) {
		MY_PANIC(DBG_COLOR_MAGENTA"%s: DMA PROTECT(5, 3): DMA ch: %s [in/out: %d, mode: %d]\r\n"DBG_COLOR_END, __func__,
			dma_wp_engine_name[ch_reg5.bit.CHSTS3 - 1], ctrl_reg.bit.DMA_WPWD5_MODE, ctrl_reg.bit.DMA_WPWD5_SEL);
	}

    if (sts_reg.bit.DMA_UPDATE_USAGE_STS) {
#if defined __KERNEL__
		if (is_dprof) {
			dprof_bandwidth_monitor(DDR_ARB_2, 0);
		}
#endif
		for (i = 0; i < 8; i++) {
			data_counter[DDR_ARB_2][i] += ARB2_GETREG(DMA_RECORD0_SIZE_REG_OFS + i * 8);
		}
	}
}

static UINT32 arb_get_reg(DDR_ARB id, UINT32 offset)
{
	if (id == DDR_ARB_1) {
		return ARB_GETREG(offset);
	} else {
		return ARB2_GETREG(offset);
	}
}

static void arb_set_reg(DDR_ARB id, UINT32 offset, REGVALUE value)
{
	if (id == DDR_ARB_1) {
		ARB_SETREG(offset, value);
	} else {
		ARB2_SETREG(offset, value);
	}
}

static UINT32 prot_get_reg(DDR_ARB id, UINT32 offset)
{
	if (id == DDR_ARB_1) {
		return PROT_GETREG(offset);
	} else {
		return PROT2_GETREG(offset);
	}
}

static void prot_set_reg(DDR_ARB id, UINT32 offset, REGVALUE value)
{
	if (id == DDR_ARB_1) {
		PROT_SETREG(offset, value);
	} else {
		PROT2_SETREG(offset, value);
	}
}
#if defined __KERNEL__

#endif



ER arb_init(void)
{
	UINT32 i;
	T_DMA_PROTECT_INTCTRL_REG intctrl_reg = {0};
	T_DMA_PROTECT_INTSTS_REG sts_reg;

//	intctrl_reg.bit.
#if defined __FREERTOS
	ddr_arb_platform_create_resource();
#endif

	for (i=0; i<DDR_ARB_COUNT; i++) {
		UINT32 lock;

		lock = ddr_arb_platform_spin_lock();

		sts_reg.reg = arb_get_reg(i, DMA_PROTECT_INTSTS_REG_OFS);
		arb_set_reg(i, DMA_PROTECT_INTSTS_REG_OFS, sts_reg.reg);


		intctrl_reg.bit.DMA_OUTRANGE_INTEN = 1;
		intctrl_reg.bit.DMA_UPDATE_USAGE_INTEN = 1;
		arb_set_reg(i, DMA_PROTECT_INTCTRL_REG_OFS, intctrl_reg.reg);

		ddr_arb_platform_spin_unlock(lock);
	}

	return E_OK;
}

void arb_enable_wp(DDR_ARB id, DMA_WRITEPROT_SET set,
		DMA_WRITEPROT_ATTR *p_attr)
{
	UINT32 i = 0;
	UINT32 lock;
	UINT32 v_mask[DMA_CH_GROUP_CNT];
	T_DMA_PROTECT_INTSTS_REG sts_reg = {0};
	T_DMA_PROTECT_INTCTRL_REG intctrl_reg = {0};
	T_DMA_PROTECT_CTRL_REG wp_ctrl;
	T_DMA_PROTECT_REGION_EN_REG0 region_en0 = {0};
	T_DMA_PROTECT_REGION_EN_REG1 region_en1 = {0};

	if (id > DDR_ARB_2) {
		DBG_ERR("invalid arb ch %d\r\n", id);
		return;
	}

	if (set > WPSET_5) {
		DBG_ERR("invalid protect ch %d\r\n", set);
		return;
	}

	for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
		if (p_attr->protect_rgn_attr[i].en) {
			if (((UINT32)p_attr->protect_rgn_attr[i].starting_addr) & 0xF) {
				DBG_WRN("start addr 0x%x NOT 4 word align\r\n",
					(unsigned int)p_attr->protect_rgn_attr[i].starting_addr);
			}
			if (((UINT32)p_attr->protect_rgn_attr[i].size) & 0xF) {
				DBG_WRN("size 0x%x NOT 4 word align\r\n",
					(unsigned int)p_attr->protect_rgn_attr[i].size);
			}
		}
	}

	memcpy(v_mask, &p_attr->mask, sizeof(DMA_CH_MSK));

	lock = ddr_arb_platform_spin_lock();

	// disable wp before setting
	wp_ctrl.reg = prot_get_reg(id, DMA_PROTECT_CTRL_REG_OFS);
	wp_ctrl.reg &= ~(1<<set);
	prot_set_reg(id, DMA_PROTECT_CTRL_REG_OFS, wp_ctrl.reg);

	// disable ch inten before setting
	intctrl_reg.reg = arb_get_reg(id, DMA_PROTECT_INTCTRL_REG_OFS);
	for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
		if (p_attr->protect_rgn_attr[i].en) {
			intctrl_reg.reg &= ~(1<<(set * 4 + i));
		}
	}
	arb_set_reg(id, DMA_PROTECT_INTCTRL_REG_OFS, intctrl_reg.reg);

	// ensure int sts is cleared
	for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
		if (p_attr->protect_rgn_attr[i].en) {
			sts_reg.reg |= 1<<(set * 4 + i);
		}
	}
	arb_set_reg(id, DMA_PROTECT_INTSTS_REG_OFS, sts_reg.reg);

	// setup protected channels
	prot_set_reg(id, PROTECT_CH_MSK0_OFS(set), v_mask[DMA_CH_GROUP0]);
	prot_set_reg(id, PROTECT_CH_MSK1_OFS(set), v_mask[DMA_CH_GROUP1]);
	prot_set_reg(id, PROTECT_CH_MSK2_OFS(set), v_mask[DMA_CH_GROUP2]);
	prot_set_reg(id, PROTECT_CH_MSK3_OFS(set), v_mask[DMA_CH_GROUP3]);
	prot_set_reg(id, PROTECT_CH_MSK4_OFS(set), v_mask[DMA_CH_GROUP4]);
	prot_set_reg(id, PROTECT_CH_MSK5_OFS(set), v_mask[DMA_CH_GROUP5]);

	// setup range
	for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
		if (p_attr->protect_rgn_attr[i].en) {
			prot_set_reg(id, PROTECT_START_ADDR_OFS(set * 4 + i), p_attr->protect_rgn_attr[i].starting_addr);
			prot_set_reg(id, PROTECT_END_ADDR_OFS(set * 4 + i),
			p_attr->protect_rgn_attr[i].starting_addr + p_attr->protect_rgn_attr[i].size - 1);
		}
	}

	// enable write protect
	wp_ctrl.reg |= 1 << set;

	// set mode
	wp_ctrl.reg &= ~(1 << (set + 8));
	wp_ctrl.reg |= (p_attr->protect_mode << (set + 8));

	// set level
	wp_ctrl.reg &= ~(0x3 << (set * 2 + 16));
	wp_ctrl.reg |= (p_attr->level << (set * 2 + 16));

	prot_set_reg(id, DMA_PROTECT_CTRL_REG_OFS, wp_ctrl.reg);

	// enable region
	if (set > WPSET_3) {
		region_en1.reg = prot_get_reg(id, DMA_PROTECT_REGION_EN_REG1_OFS);
		for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
			if (p_attr->protect_rgn_attr[i].en) {
				region_en1.reg |= (1 << ((set - 4) * 8 + i));
			}
		}
		prot_set_reg(id, DMA_PROTECT_REGION_EN_REG1_OFS, region_en1.reg);
	} else {
		region_en0.reg = prot_get_reg(id, DMA_PROTECT_REGION_EN_REG0_OFS);
		for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
			if (p_attr->protect_rgn_attr[i].en) {
				region_en0.reg |= 1 << (set * 8 + i);
			}
		}
		prot_set_reg(id, DMA_PROTECT_REGION_EN_REG0_OFS, region_en0.reg);
	}

	// enable interrupt
	for (i = 0; i < DMA_PROT_RGN_TOTAL; i++) {
		if (p_attr->protect_rgn_attr[i].en) {
			intctrl_reg.reg |= 1 << (set * 4 + i);
		}
	}
	arb_set_reg(id, DMA_PROTECT_INTCTRL_REG_OFS, intctrl_reg.reg);

	ddr_arb_platform_spin_unlock(lock);
}

void arb_disable_wp(DDR_ARB id, DMA_WRITEPROT_SET set)
{
	UINT32 lock;
	T_DMA_PROTECT_INTSTS_REG sts_reg = {0};
	T_DMA_PROTECT_INTCTRL_REG intctrl_reg = {0};
	T_DMA_PROTECT_CTRL_REG wp_ctrl;

	if (id > DDR_ARB_2) {
                DBG_ERR("invalid arb ch %d\r\n", id);
                return;
        }

        if (set > WPSET_5) {
                DBG_ERR("invalid protect ch %d\r\n", set);
                return;
        }

	lock = ddr_arb_platform_spin_lock();

	// disable wp
        wp_ctrl.reg = prot_get_reg(id, DMA_PROTECT_CTRL_REG_OFS);
        wp_ctrl.reg &= ~(1<<set);
        prot_set_reg(id, DMA_PROTECT_CTRL_REG_OFS, wp_ctrl.reg);

        // disable ch inten
        intctrl_reg.reg = arb_get_reg(id, DMA_PROTECT_INTCTRL_REG_OFS);
        intctrl_reg.reg &= ~(0xf<<(set * 4));
        arb_set_reg(id, DMA_PROTECT_INTCTRL_REG_OFS, intctrl_reg.reg);

        // ensure int sts is cleared
        sts_reg.reg = 0xf<<(set * 4);
        arb_set_reg(id, DMA_PROTECT_INTSTS_REG_OFS, sts_reg.reg);

	ddr_arb_platform_spin_unlock(lock);
}

ER dma_enable_heavyload(DDR_ARB id, DMA_HEAVY_LOAD_CH channel, PDMA_HEAVY_LOAD_PARAM hvy_param)
{
	T_DMA_CHANNEL0_HEAVY_LOAD_CTRL_REG  hvy_load_reg;
	UINT32 temp_value;

	hvy_load_reg.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(channel));



	if (hvy_load_reg.bit.CH0_TEST_START == 1) {
		DBG_WRN("DMA_HVY(ch%d): already enabled register 0x%08x\r\n", channel, (unsigned int)arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(channel)));
		//return E_SYS;
		hvy_load_reg.bit.CH0_TEST_START = 0;
		arb_set_reg(id, HEAVY_LOAD_CTRL_OFS(channel), hvy_load_reg.reg);
	}

	hvy_load_reg.bit.CH0_TEST_METHOD = hvy_param->test_method;
	hvy_load_reg.bit.CH0_BURST_LEN = hvy_param->burst_len - 1;

	if (hvy_param->test_times == 0) {
		//DBG_WRN("Heavy load test time = 0 is not available, auto-adjust it to 1\r\n");
		//hvy_param->test_times++;
	}
	hvy_load_reg.bit.CH0_TEST_TIMES = hvy_param->test_times;

	if ((hvy_param->start_addr & 0x00000003) != 0x00000000) {
		DBG_WRN("DMA_HVY: starting address isn't words alignment!\r\n");
	}
	if ((hvy_param->dma_size & 0x00000003) != 0x00000000) {
		DBG_WRN("DMA_HVY: size isn't words alignment!\r\n");
	}

	if (hvy_param->dma_size < 4) {
		DBG_ERR("DMA_HVY: size less than 1 word\r\n");
		return E_PAR;
	}
	if (!id) {
		vos_cpu_dcache_sync((VOS_ADDR)hvy_param->start_addr,
					hvy_param->dma_size,
					VOS_DMA_BIDIRECTIONAL);
	}

	arb_set_reg(id, HEAVY_LOAD_ADDR_OFS(channel), id ? (hvy_param->start_addr & 0xFFFFFFFC) : ddr_arb_platform_va2pa(hvy_param->start_addr & 0xFFFFFFFC));
	arb_set_reg(id, HEAVY_LOAD_SIZE_OFS(channel), (hvy_param->dma_size & 0xFFFFFFFC));
	arb_set_reg(id, HEAVY_LOAD_CTRL_OFS(channel), hvy_load_reg.reg);

	//DMA_SETREG(HEAVY_LOAD_WAIT_CYCLE_OFS(uiCh), 0x3);

	if (arb_get_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN0_OFS) == 0x00000000) {
		temp_value = 0x69696969;

		arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN0_OFS, temp_value);
		//DBG_WRN("DMA_HVY: Pattern0 not configed, assign random number 0x%08x\r\n", temp_value);
	}
	if (arb_get_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN1_OFS) == 0x00000000) {
		temp_value = 0x5a5a5a5a;

		arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN1_OFS, temp_value);
		//DBG_WRN("DMA_HVY: Pattern1 not configed, assign random number 0x%08x\r\n", temp_value);
	}
	if (arb_get_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN2_OFS) == 0x00000000) {
		temp_value = 0x55aaaa55;

		arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN2_OFS, temp_value);
		//DBG_WRN("DMA_HVY: Pattern2 not configed, assign random number 0x%08x\r\n", temp_value);
	}
	if (arb_get_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN3_OFS) == 0x00000000) {
		temp_value = 0xaa5555aa;

		arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN3_OFS, temp_value);
		//DBG_WRN("DMA_HVY: Pattern3 not configed, assign random number 0x%08x\r\n", temp_value);
	}

	hvy_load_reg.bit.CH0_TEST_START = 1;
	arb_set_reg(id, HEAVY_LOAD_CTRL_OFS(channel), hvy_load_reg.reg);
	//DBG_IND("channel %d\r\n", channel);

	return E_OK;
}

void dma_trig_heavyload(DDR_ARB id, UINT32 channel)
{
	arb_set_reg(id, DMA_HEAVY_LOAD_TRIG_OFS, channel);
}

BOOL dma_wait_heavyload_done_polling(DDR_ARB id, dma_hvyload_callback_func call_back_hdl)
{
	UINT32 cnt;
	T_DMA_HEAVY_LOAD_STS_REG sts_reg;
	T_DMA_CHANNEL0_HEAVY_LOAD_CTRL_REG  hvyload_reg0;
	T_DMA_CHANNEL0_HEAVY_LOAD_CTRL_REG  hvyload_reg1;
	T_DMA_CHANNEL0_HEAVY_LOAD_CTRL_REG  hvyload_reg2;
	T_DMA_HEAVY_LOAD_TRIG_REG			trig_reg;


	cnt = 0;
	if( nvt_get_chip_id() == CHIP_NA51055) {
		hvyload_reg0.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH0));
		hvyload_reg1.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH1));
		hvyload_reg2.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH2));

		while (hvyload_reg0.bit.CH0_TEST_START || hvyload_reg1.bit.CH0_TEST_START || hvyload_reg2.bit.CH0_TEST_START) {
			cnt++;
			hvyload_reg0.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH0));
			hvyload_reg1.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH1));
			hvyload_reg2.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH2));

			if (call_back_hdl) {
				call_back_hdl();
			} else {
				if (cnt % 500 == 0) {
					DBG_DUMP("DMA%d utilization => [%02d]\r\n", (int)id, (int)dma_get_utilization(id));
					DBG_DUMP("DMA%d efficiency => [%02d]\r\n", (int)id, (int)dma_get_efficiency(id));
				}
			}
			ddr_arb_platform_delay_ms(10);
		}
	} else {
		trig_reg.reg = arb_get_reg(id, DMA_HEAVY_LOAD_TRIG_OFS);

		while (trig_reg.bit.CH0_TEST_START || trig_reg.bit.CH1_TEST_START || trig_reg.bit.CH2_TEST_START) {
			cnt++;
			trig_reg.reg = arb_get_reg(id, DMA_HEAVY_LOAD_TRIG_OFS);

			if (call_back_hdl) {
				call_back_hdl();
			} else {
				if (cnt % 500 == 0) {
					DBG_DUMP("DMA%d utilization => [%02d]\r\n", (int)id, (int)dma_get_utilization(id));
					DBG_DUMP("DMA%d efficiency => [%02d]\r\n", (int)id, (int)dma_get_efficiency(id));
				}
			}
			ddr_arb_platform_delay_ms(10);
		}
	}

	sts_reg.reg = arb_get_reg(id, DMA_HEAVY_LOAD_STS_OFS);
	if (sts_reg.bit.HEAVY_LOAD_COMPLETE_STS) {
		DBG_IND("heavy loading test success\r\n");
		arb_set_reg(id, DMA_HEAVY_LOAD_STS_OFS, sts_reg.reg);
		return TRUE;
	} else if (sts_reg.bit.HEAVY_LOAD_ERROR_STS) {
		DBG_ERR("heavy loading test fail @[0x%08x] V[0x%08x] vs X[0x%08x]\r\n", (unsigned int)arb_get_reg(id, DMA_HEAVY_LOAD_CURRENT_ADDR_OFS), (unsigned int)arb_get_reg(id, DMA_HEAVY_LOAD_CORRECT_DATA_OFS), (unsigned int)arb_get_reg(id, DMA_HEAVY_LOAD_ERROR_DATA_OFS));
		arb_set_reg(id, DMA_HEAVY_LOAD_STS_OFS, sts_reg.reg);
		return FALSE;
	} else {
		DBG_ERR("Heavy loading test unknow result sts = [0x%08x] \r\n", (unsigned int)sts_reg.reg);
		arb_set_reg(id, DMA_HEAVY_LOAD_STS_OFS, sts_reg.reg);
		return FALSE;
	}
}

void arb_set_priority(BOOL is_direct)
{
	dma_set_system_priority(is_direct);
}

/*
    Set channel outstanding.

    Set channel outstanding


    @param[in] outstanding channel

    @return Set outstanding
        - @b E_OK   : Set outstanding success
        - @b E_PAR  : Channel is not correct
*/
ER dma_set_channel_outstanding(DMA_CH_OUTSTANDING channel, BOOL enable)
{
	UINT32 reg = 0;
	UINT32 reg2 = 0;

	reg = arb_get_reg(DDR_ARB_1, DMA_CHANNEL_OUTSTANDING_REG_OFS);
	reg2 = arb_get_reg(DDR_ARB_2, DMA_CHANNEL_OUTSTANDING_REG_OFS);
	if (enable) {
		if (channel == DMA_CH_OUTSTANDING_CNN_ALL) {
			reg |= 0x3f;
			reg2 |= 0x3f;
		} else if (channel == DMA_CH_OUTSTANDING_CNN2_ALL) {
			reg |= 0xfc0;
			reg2 |= 0xfc0;
		} else if (channel == DMA_CH_OUTSTANDING_NUE_ALL) {
			reg |= 0x7000;
			reg2 |= 0x7000;
		} else {
			reg |= (1 << channel);
			reg2 |= (1 << channel);
		}
	} else {
		if (channel == DMA_CH_OUTSTANDING_CNN_ALL) {
			reg &= ~0x3f;
			reg2 &= ~0x3f;
		} else if (channel == DMA_CH_OUTSTANDING_CNN2_ALL) {
			reg &= ~0xfc0;
			reg2 &= ~0xfc0;
		} else if (channel == DMA_CH_OUTSTANDING_NUE_ALL) {
			reg &= ~0x7000;
			reg2 &= ~0x7000;
		} else {
			reg &= ~(1 << channel);
			reg2 &= ~(1 << channel);
		}
	}
	arb_set_reg(DDR_ARB_1, DMA_CHANNEL_OUTSTANDING_REG_OFS, reg);
	arb_set_reg(DDR_ARB_2, DMA_CHANNEL_OUTSTANDING_REG_OFS, reg2);

    return E_OK;
}

/*
    Get channel outstanding.

    Get channel outstanding


    @param[in] outstanding channel

    @return Get outstanding
        - @b E_OK   : Set outstanding success
        - @b E_PAR  : Channel is not correct
        - @b E_OBJ  : DMA outstanding in DMA_ID_1 and DMA_ID_2 is not the same
*/
ER dma_get_channel_outstanding(DMA_CH_OUTSTANDING channel, BOOL *enable)
{
	UINT32 reg = 0;
	UINT32 reg2 = 0;

	reg = arb_get_reg(DDR_ARB_1, DMA_CHANNEL_OUTSTANDING_REG_OFS);
	reg2 = arb_get_reg(DDR_ARB_2, DMA_CHANNEL_OUTSTANDING_REG_OFS);
	if (channel >= DMA_CH_OUTSTANDING_CNN_ALL) {
		return E_PAR;
	} else {
		reg = ((reg & (1 << channel)) >> channel);
		reg2 = ((reg2 & (1 << channel)) >> channel);
	}

	if ((reg & 0x1) != (reg2 & 0x1)) {
		return E_OBJ;
	}

	*enable = reg & 0x1;

    return E_OK;
}


unsigned short arb_chksum(DDR_ARB id, unsigned int phy_addr, unsigned int len)
{
    unsigned int  checksum;
	T_DMA_CHANNEL0_HEAVY_LOAD_WAIT_CYCLE waitCycleReg = {0};
	T_DMA_CHANNEL0_HEAVY_LOAD_CTRL_REG  hvyload_reg0;

	if (phy_addr & 0x03) {
		DBG_ERR("input phy_addr not word align: 0x%x\r\n", (unsigned int)phy_addr);
		return 0;
	}
	if (len & 0x03) {
		DBG_ERR("input length not word align: 0x%x\r\n", (unsigned int)len);
		return 0;
	}

	// Configure to skip error when compare not match
	hvyload_reg0.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH0));
	hvyload_reg0.bit.CH0_SKIP_COMPARE = 1;
	hvyload_reg0.bit.CH0_TEST_METHOD = DMA_HEAVY_LOAD_READ_ONLY;
	hvyload_reg0.bit.CH0_BURST_LEN = 127;
	arb_set_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH0), hvyload_reg0.reg);


	arb_set_reg(id, HEAVY_LOAD_ADDR_OFS(DMA_HEAVY_LOAD_CH0), (phy_addr & 0xFFFFFFFC));
	arb_set_reg(id, HEAVY_LOAD_SIZE_OFS(DMA_HEAVY_LOAD_CH0), (len & 0xFFFFFFFC));

	hvyload_reg0.bit.CH0_TEST_START = 1;
	arb_set_reg(id, HEAVY_LOAD_CTRL_OFS((DMA_HEAVY_LOAD_CH0)), hvyload_reg0.reg);
	while (hvyload_reg0.bit.CH0_TEST_START) {
		ddr_arb_platform_delay_ms(1);
		hvyload_reg0.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(DMA_HEAVY_LOAD_CH0));
	}

	// Read register that contains checksum result
	if( nvt_get_chip_id() == CHIP_NA51055) {
		waitCycleReg.reg = arb_get_reg(id, HEAVY_LOAD_WAIT_CYCLE_OFS(DMA_HEAVY_LOAD_CH0));
	} else {
		dma_trig_heavyload(id, 0x1);
	}
	checksum = waitCycleReg.bit.CHECKSUM + (((1+len)*len)>>1);
	return (unsigned short)(checksum & 0xFFFF);
}

static void dma_enable_monitor(DMA_ID id, DMA_MONITOR_CH chMon, DMA_CH chDma,
		DMA_DIRECTION direction)
{
	UINT32 regAddr;
	UINT32 lock;
	T_DMA_RECORD_CONFIG0_REG configReg;

	if (id >= DMA_ID_COUNT) {
		DBG_ERR("DMA id %d invalid\r\n", id);
	}
	if (chMon >= DMA_MONITOR_ALL) {
		DBG_ERR("Monitor ch %d invalid\r\n", chMon);
	}
	if (chDma >= DMA_CH_COUNT) {
		DBG_ERR("DMA ch %d invalid\r\n", chDma);
	}

	regAddr = DMA_RECORD_CONFIG0_REG_OFS + (chMon / 2) * 4;

	// Enter critical section
	lock = ddr_arb_platform_spin_lock();

	configReg.reg = arb_get_reg(id, regAddr);
	if (chMon & 0x01) {
		configReg.bit.MONITOR1_DMA_CH = chDma;
		configReg.bit.MONITOR1_DMA_DIR = direction;
	} else {
		configReg.bit.MONITOR0_DMA_CH = chDma;
		configReg.bit.MONITOR0_DMA_DIR = direction;
	}

	arb_set_reg(id, regAddr, configReg.reg);

	// Leave critical section
	ddr_arb_platform_spin_unlock(lock);

}


int mau_ch_mon_start(int ch, int rw, int dram)
{
	UINT32 lock;
	UINT32 i = 0;
	int true_rw = 0;
	int dma_ch = 0;

	if (is_used[dram][0] & is_used[dram][1] & is_used[dram][2] & is_used[dram][3] & is_used[dram][4] & is_used[dram][5] & is_used[dram][6] & is_used[dram][7]) {
		DBG_ERR("monitor are all in use!\r\n");
		return 1;
	}

	if (rw == 0) {
		true_rw = 1;
	} else if (rw == 1) {
		true_rw = 0;
	} else {
		true_rw = 2;
	}
	if (ch == 0) {
		for (i = 0; i < 8; i++) {
			//DBG_ERR("is_used %d!\r\n", is_used[i]);
			if (is_used[dram][i] == 0) {
				dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_CPU, true_rw);
				lock = ddr_arb_platform_spin_lock();
				is_used[dram][i] = DMA_CH_CPU;
				data_counter[dram][i] = 0;
				ddr_arb_platform_spin_unlock(lock);
				//DBG_ERR("attr.ch %d!\r\n", attr.ch);
				break;
			}
		}
	} else if (ch == 1) {
		for (dma_ch = 0; dma_ch < 6; dma_ch++) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_CNN_0+dma_ch, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_CNN_0+dma_ch;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_CNN_6, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_CNN_6;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
	} else if (ch == 2) {
		for (dma_ch = 0; dma_ch < 6; dma_ch++) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_CNN2_0+dma_ch, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_CNN2_0+dma_ch;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_CNN2_6, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_CNN2_6;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
	} else if (ch == 3) {
		for (dma_ch = 0; dma_ch < 3; dma_ch++) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_NUE_0+dma_ch, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_NUE_0+dma_ch;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_NUE_3, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_NUE_3;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
	} else if (ch == 4) {
		for (dma_ch = 0; dma_ch < 6; dma_ch++) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_NUE2_0+dma_ch, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_NUE2_0+dma_ch;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_NUE2_6, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_NUE2_6;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
	} else if (ch == 5) {
		for (dma_ch = 0; dma_ch < 2; dma_ch++) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_ISE_a0+dma_ch, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_ISE_a0+dma_ch;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				//DBG_ERR("is_used %d!\r\n", is_used[i]);
				if (is_used[dram][i] == 0) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_ISE_2, true_rw);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = DMA_CH_ISE_2;
					data_counter[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					//DBG_ERR("attr.ch %d!\r\n", attr.ch);
					break;
				}
				if (i == 7) {
					DBG_ERR("monitor not enough!\r\n");
					return 1;
				}
			}
		}

	} else {
		DBG_ERR("ch %d not support!\r\n", ch);
		return 1;
	}

	ddr_arb_platform_delay_ms(25);

    return 0;

}

UINT64 mau_ch_mon_stop(int ch, int dram)
{
	UINT32 i = 0;
	UINT64 cnt = 0;
	UINT32 lock;
	int dma_ch = 0;

	ddr_arb_platform_delay_ms(25);

	if (ch == 0) {
		for (i = 0; i < 8; i++) {
			if (is_used[dram][i] == DMA_CH_CPU) {
				dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
				lock = ddr_arb_platform_spin_lock();
				is_used[dram][i] = 0;
				ddr_arb_platform_spin_unlock(lock);
				return data_counter[dram][i]*4;
			}
			if (i == 7) {
				DBG_ERR("ch %d is not in monitor\r\n", ch);
				return 1;
			}
		}
	} else if (ch == 1) {
		for (dma_ch = 0; dma_ch < 6; dma_ch++) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_CNN_0 + dma_ch) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_CNN_6) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		return cnt*4;
	} else if (ch == 2) {
		for (dma_ch = 0; dma_ch < 6; dma_ch++) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_CNN2_0 + dma_ch) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_CNN2_6) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		return cnt*4;
	} else if (ch == 3) {
		for (dma_ch = 0; dma_ch < 3; dma_ch++) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_NUE_0 + dma_ch) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_NUE_3) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		return cnt*4;
	} else if (ch == 4) {
		for (dma_ch = 0; dma_ch < 6; dma_ch++) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_NUE2_0 + dma_ch) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_NUE2_6) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		return cnt*4;
	} else if (ch == 5) {
		for (dma_ch = 0; dma_ch < 2; dma_ch++) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_ISE_a0 + dma_ch) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		if( nvt_get_chip_id() == CHIP_NA51084) {
			for (i = 0; i < 8; i++) {
				if (is_used[dram][i] == DMA_CH_ISE_2) {
					dma_enable_monitor(dram, (DMA_MONITOR_CH)i, DMA_CH_RSV, 0);
					lock = ddr_arb_platform_spin_lock();
					is_used[dram][i] = 0;
					ddr_arb_platform_spin_unlock(lock);
					cnt += data_counter[dram][i];
					break;
				}
				if (i == 7) {
					DBG_ERR("ch %d is not in monitor\r\n", ch);
					return 1;
				}
			}
		}
		return cnt*4;
	} else {
		DBG_ERR("ch %d not support!\r\n", ch);
		return 1;
	}

	return 0;
}


#if defined __FREERTOS
/*

    Flush read (Device to CPU) cache without checking buffer is cacheable or not


    In DMA operation, if buffer is cacheable, we have to flush read buffer before
    DMA operation to make sure CPU will read correct data.



    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return Use clean and invalidate data cache all or not (cpu_cleanInvalidateDCacheAll)
        - @b TRUE:   Use cpu_cleanInvalidateDCacheAll
        - @b FALSE:  Not use cpu_cleanInvalidateDCacheAll
*/
BOOL dma_flushReadCacheWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength)
{
	BOOL   bIsCleanInvalidateDCacheAll;
	const UINT32 uiCacheLineSizeMask = (1 << CACHE_SET_NUMBER_SHIFT) - 1;
	UINT32 uiEndAddr = (uiStartAddr + uiLength);

	// Start address isn't cache line alignment
	// In read operation, we only need to invalidate the buffer.
	// Since invalidate is based on cache line (64 bytes), if the starting address
	// isn't cache line alignment, invalidate the line will invalidate
	// non-buffer address. If these address is in cache and dirty,
	// invalidate operation will cause data inconsistent problem.
	// So we have to clean the data cache to write dirty data back to DRAM.
	if ((uiStartAddr & uiCacheLineSizeMask) != 0) {
		cpu_cleanInvalidateDCache(uiStartAddr);
	}


	// End address isn't cache line alignment
	if (((uiStartAddr + uiLength) & uiCacheLineSizeMask) != 0) {
		cpu_cleanInvalidateDCache(uiStartAddr + uiLength);
	}

	// Invalidate data cache
	// cover the case where address = 0xXXXXXXX0
	// So, need - 1
	uiStartAddr += uiCacheLineSizeMask;
	uiStartAddr &= ~uiCacheLineSizeMask;

	// cahcle line size alignment
	if (uiEndAddr & uiCacheLineSizeMask) {
		uiEndAddr &= ~uiCacheLineSizeMask;
	}


	if ((uiEndAddr - uiStartAddr) > CACHE_FLUSHALL_BOUNDARY) {
		cpu_cleanInvalidateDCacheAll();
		bIsCleanInvalidateDCacheAll = TRUE;
	} else {
		//Use aligned end address
		cpu_invalidateDCacheBlock(uiStartAddr, uiEndAddr);
		bIsCleanInvalidateDCacheAll = FALSE;
	}
	return bIsCleanInvalidateDCacheAll;
}

/*

    Flush read (Device to CPU) cache without checking buffer is cacheable or not


    In DMA operation, if buffer is cacheable, we also have to flush read buffer after
    DMA operation to make sure CPU will read correct data.



    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return void
*/
void dma_flushReadCacheDmaEndWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength)
{
	const UINT32 uiCacheLineSizeMask = (1 << CACHE_SET_NUMBER_SHIFT) - 1;
	UINT32 uiEndAddr = (uiStartAddr + uiLength);

	// Start address isn't cache line alignment
	// In read operation, we only need to invalidate the buffer.
	// Since invalidate is based on cache line (64 bytes), if the starting address
	// isn't cache line alignment, invalidate the line will invalidate
	// non-buffer address. If these address is in cache and dirty,
	// invalidate operation will cause data inconsistent problem.
	// So we have to clean the data cache to write dirty data back to DRAM.
	if ((uiStartAddr & uiCacheLineSizeMask) != 0) {
		cpu_cleanInvalidateDCache(uiStartAddr);
	}


	// End address isn't cache line alignment
	if (((uiStartAddr + uiLength) & uiCacheLineSizeMask) != 0) {
		cpu_cleanInvalidateDCache(uiStartAddr + uiLength);
	}

	// Invalidate data cache
	// cover the case where address = 0xXXXXXXX0
	// So, need - 1
	uiStartAddr += uiCacheLineSizeMask;
	uiStartAddr &= ~uiCacheLineSizeMask;

	// cahcle line size alignment
	if (uiEndAddr & uiCacheLineSizeMask) {
		uiEndAddr &= ~uiCacheLineSizeMask;
	}


	if ((uiEndAddr - uiStartAddr) > CACHE_FLUSHALL_BOUNDARY) {
		cpu_cleanInvalidateDCacheAll();
	} else {
		//Use aligned end address
		cpu_invalidateDCacheBlock(uiStartAddr, uiEndAddr);
	}
}



/*
    Flush write (CPU to Device) cache without checking buffer is cacheable or not

    In DMA operation, if buffer is cacheable, we have to flush write buffer before
    DMA operation to make sure DMA will send correct data.

    @note   Depend on performance measurement
        clean data cache 016K = 00015 us
        clean data cache 032K = 00020 us
        clean data cache 064K = 00031 us
        clean data cache 128K = 00056 us
        clean data cache 256K = 00102 us
        clean data cache 512K = 00198 us

        fatch 16K data cache  = 00024 us

        So, once if (uiEndAddr - uiStartAddr) > 32K
        calling cpu_cleanInvalidateDCacheAll

    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return Use clean and invalidate data cache all or not (cpu_cleanInvalidateDCacheAll)
        - @b TRUE:   Use cpu_cleanInvalidateDCacheAll
        - @b FALSE:  Not use cpu_cleanInvalidateDCacheAll
*/
BOOL dma_flushWriteCacheWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength)
{
	BOOL   bIsCleanInvalidateDCacheAll;
	// line size
	const UINT32 uiCacheLineSizeMask = (1 << CACHE_SET_NUMBER_SHIFT) - 1;
	UINT32 uiEndAddr = (uiStartAddr + uiLength);

	uiStartAddr &= ~uiCacheLineSizeMask;
	// cahcle line size alignment
	if (uiEndAddr & uiCacheLineSizeMask) {
		uiEndAddr &= ~uiCacheLineSizeMask;
		uiEndAddr += (1 << CACHE_SET_NUMBER_SHIFT);
	}

	if ((uiEndAddr - uiStartAddr) > CACHE_FLUSHALL_BOUNDARY) {
		cpu_cleanInvalidateDCacheAll();
		bIsCleanInvalidateDCacheAll = TRUE;
	} else {
		cpu_cleanDCacheBlock(uiStartAddr, uiEndAddr);
		bIsCleanInvalidateDCacheAll = FALSE;
	}
	return bIsCleanInvalidateDCacheAll;
}

/*
    Flush read (Device to CPU) cache where lineoffset not equal to width
    Flush read (Device to CPU) cache where lineoffset not equal to width

    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return Use clean and invalidate data cache all or not (cpu_cleanInvalidateDCacheAll)
        - @b TRUE:   Use cpu_cleanInvalidateDCacheAll
        - @b FALSE:  Not use cpu_cleanInvalidateDCacheAll

*/
BOOL dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength)
{
	BOOL   bIsCleanInvalidateDCacheAll;
	// line size
	const UINT32 uiCacheLineSizeMask = (1 << CACHE_SET_NUMBER_SHIFT) - 1;
	UINT32 uiEndAddr = (uiStartAddr + uiLength);

	uiStartAddr &= ~uiCacheLineSizeMask;
	// cahcle line size alignment
	if (uiEndAddr & uiCacheLineSizeMask) {
		uiEndAddr &= ~uiCacheLineSizeMask;
		uiEndAddr += (1 << CACHE_SET_NUMBER_SHIFT);
	}

	if ((uiEndAddr - uiStartAddr) > CACHE_FLUSHALL_BOUNDARY) {
		cpu_cleanInvalidateDCacheAll();
		bIsCleanInvalidateDCacheAll = TRUE;
	} else {
		cpu_cleanInvalidateDCacheBlock(uiStartAddr, uiEndAddr);
		bIsCleanInvalidateDCacheAll = FALSE;
	}
	return bIsCleanInvalidateDCacheAll;
}

/*
    Set heavy loading test pattern

    Set heavy loading test pattern, total 4 sets

    @param[in] id           DMA controller ID
    @param[in] uiPattern0   Pattern0
    @param[in] uiPattern1   Pattern1
    @param[in] uiPattern2   Pattern2
    @param[in] uiPattern3   Pattern3
*/
void dma_setHeavyLoadPattern(DMA_ID id, UINT32 uiPattern0, UINT32 uiPattern1, UINT32 uiPattern2, UINT32 uiPattern3)
{
	arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN0_OFS, uiPattern0);
	arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN1_OFS, uiPattern1);
	arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN2_OFS, uiPattern2);
	arb_set_reg(id, DMA_HEAVY_LOAD_TEST_PATTERN3_OFS, uiPattern3);
}

/*
    Get heavy load checksum

    Enable heavy load to calculate 16 bit checksum and get result

    @param[in] id           DMA controller ID
    @param[in] uiCh         3 channel are available for configure
    @param[in] uiBurstLen   0 ~ 127
    @param[in] uiAddr       DRAM buffer address (should word align)
    @param[in] uiLength     buffer length (should word align)

    @return checksum
*/
UINT32 dma_getHeavyLoadChkSum(DMA_ID id, DMA_HEAVY_LOAD_CH uiCh, UINT32 uiBurstLen, UINT32 uiAddr, UINT32 uiLength)
{
	T_DMA_CHANNEL0_HEAVY_LOAD_WAIT_CYCLE waitCycleReg;
	T_DMA_CHANNEL0_HEAVY_LOAD_CTRL_REG  uHvyLoadReg;
	DMA_HEAVY_LOAD_PARAM hvyParam = {0};

	if (uiAddr & 0x03) {
		DBG_ERR("input addr not word align: 0x%x\r\n", (unsigned int)uiAddr);
		return 0;
	}
	if (uiLength & 0x03) {
		DBG_ERR("input length not word align: 0x%x\r\n", (unsigned int)uiLength);
		return 0;
	}

	// Configure to skip error when compare not match
	uHvyLoadReg.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(uiCh));
	uHvyLoadReg.bit.CH0_SKIP_COMPARE = 1;
	arb_set_reg(id, HEAVY_LOAD_CTRL_OFS(uiCh), uHvyLoadReg.reg);

	hvyParam.burst_len = uiBurstLen;
	hvyParam.dma_size = uiLength;
	hvyParam.test_method = DMA_HEAVY_LOAD_READ_ONLY;
	hvyParam.start_addr = dma_getPhyAddr(uiAddr);
	hvyParam.test_times = 1;

	// flush cache
	dma_flushWriteCache(uiAddr, uiLength);

	// Enable heavy load to calculate checksum
	if (dma_enableHeavyLoad(id, uiCh, &hvyParam) != E_OK) {
		return 0;
	}
	dma_trig_heavyload(id, 1 << uiCh);
	// Wait heavy load complete
	if (dma_waitHeavyLoadDonePolling(id, NULL) == FALSE) {
		DBG_ERR("heavy load result fail\r\n");
	}

	// Read register that contains checksum result
	waitCycleReg.reg = arb_get_reg(id, HEAVY_LOAD_WAIT_CYCLE_OFS(uiCh));

	// Restore to detect error when compare not match
	uHvyLoadReg.reg = arb_get_reg(id, HEAVY_LOAD_CTRL_OFS(uiCh));
	uHvyLoadReg.bit.CH0_SKIP_COMPARE = 0;
	arb_set_reg(id, HEAVY_LOAD_CTRL_OFS(uiCh), uHvyLoadReg.reg);

	return waitCycleReg.bit.CHECKSUM;
}

#endif

/**
    @addtogroup miDrvComm_Arb
*/
//@{


//@}

