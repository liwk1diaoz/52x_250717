#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "ddr_arb_drv.h"
#include "ddr_arb_ioctl.h"
#include "ddr_arb_platform.h"
#include "ddr_arb_int.h"
#include "ddr_arb_reg.h"

#ifdef CONFIG_NVT_DMA_MONITOR
#define DPROF_EN  1
#else
#define DPROF_EN  0
#endif
/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ddr_arb_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_ddr_arb_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ddr_arb_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_ddr_arb_drv_isr(int irq, void *devid);
irqreturn_t nvt_ddr_arb2_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

#if DPROF_EN
///////////////////////////////////


typedef enum _DPROF_STATE {
	DPROF_STATE_IDLE,
	DPROF_STATE_MONITOR_ONGOING,

	ENUM_DUMMY4WORD(DPROF_STATE)
} DPROF_STATE;

typedef struct _DPROF_NODE{
	UINT32 tick;
	UINT32 vSize[DMA_MONITOR_CH_COUNT];
	UINT32 vRequest[DMA_MONITOR_CH_COUNT];
	UINT32 flags;
} DPROF_NODE;
///////////////////////////////////
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
#endif
/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
static int is_dprof = 0;
#if DPROF_EN
static UINT32 idx_head[DDR_ARB_COUNT] = {0, 0};
static UINT32 idx_tail[DDR_ARB_COUNT] = {0, 0};

static DPROF_NODE g_dprof_node_cache[DDR_ARB_COUNT];
static DPROF_NODE *gp_dprof_queue[DDR_ARB_COUNT];
static UINT32 dprof_queue_length[DDR_ARB_COUNT] = {0, 0};

static DPROF_STATE g_dprof_state[DDR_ARB_COUNT] = {0, 0};
static UINT32 g_dprof_time_slice_idx[DDR_ARB_COUNT] = {0, 0};
static UINT32 b_dprof_queue_overflow[DDR_ARB_COUNT] = {0, 0};   // 1: queue overflow occured

static UINT32 g_dprof_pool_addr[DDR_ARB_COUNT] = {0, 0};
static UINT32 g_dprof_pool_size[DDR_ARB_COUNT] = {0, 0};
static UINT32 g_dprof_start_tick[DDR_ARB_COUNT] = {0, 0};
static UINT32 g_dprof_end_tick[DDR_ARB_COUNT] = {0, 0};
#endif
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
#if DPROF_EN
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
static const CHAR *dma_getChannelName(DMA_CH uiCh)
{
	if (uiCh >= DMA_CH_COUNT) {
		printk("Unknow dma channel = [%02d] \r\n", uiCh);
		return NULL;
	} else if (uiCh == DMA_CH_RSV) {
		return "NONE";
	} else {
		return dma_wp_engine_name[uiCh - DMA_CH_FIRST_CHANNEL];
	}
}

ER dma_configMonitor(DMA_ID id, DMA_MONITOR_CH chMon, DMA_CH chDma,
		DMA_DIRECTION direction)
{
	UINT32 regAddr;
	UINT32 lock;
	T_DMA_RECORD_CONFIG0_REG configReg;

	if (id >= DMA_ID_COUNT) {
		printk("DMA id %d invalid\r\n", id);
		return E_PAR;
	}
	if (chMon >= DMA_MONITOR_ALL) {
		printk("Monitor ch %d invalid\r\n", chMon);
		return E_PAR;
	}
	if (chDma >= DMA_CH_COUNT) {
		printk("DMA ch %d invalid\r\n", chDma);
		return E_PAR;
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

	return E_OK;
}

/*
    Get DMA monitor config

    Config DMA monitor

    @param[in] id       DMA controller ID
    @param[in] chMon    select monitor channel
    @param[out] pChDma      return DMA channel to be monitored by chMon
    @param[out] pDirection  return which direction to be monitored
                        - @b DMA_DIRECTION_READ: DRAM -> Module
                        - @b DMA_DIRECTION_WRITE: Module <- DRAM
                        - @b DMA_DIRECTION_BOTH: both direction

    @return
        - @b E_OK: success
        - @b Else: input parameter not valid
*/
ER dma_getMonitorconfig(DMA_ID id, DMA_MONITOR_CH chMon, DMA_CH *pChDma, DMA_DIRECTION *pDirection)
{
	UINT32 regAddr;
	T_DMA_RECORD_CONFIG0_REG configReg;

	if (id >= DMA_ID_COUNT) {
		printk("DMA id %d invalid\r\n", id);
		return E_PAR;
	}
	if (chMon >= DMA_MONITOR_ALL) {
		printk("Monitor ch %d invalid\r\n", chMon);
		return E_PAR;
	}
	if (pChDma == NULL) {
		printk("invalid pChDma\r\n");
		return E_PAR;
	}
	if (pDirection == NULL) {
		printk("invalid pDirection\r\n");
		return E_PAR;
	}

	regAddr = DMA_RECORD_CONFIG0_REG_OFS + (chMon / 2) * 4;

	configReg.reg = arb_get_reg(id, regAddr);
	if (chMon & 0x01) {
		*pChDma = configReg.bit.MONITOR1_DMA_CH;
		*pDirection = configReg.bit.MONITOR1_DMA_DIR;
	} else {
		*pChDma = configReg.bit.MONITOR0_DMA_CH;
		*pDirection = configReg.bit.MONITOR0_DMA_DIR;
	}

	return E_OK;
}
static void *handle[2] = {NULL, NULL};
static ER dprof_init(void) {
	UINT32 lock;
	const UINT32 POOL_SIZE = 0x40000;
	struct vos_mem_cma_info_t dprof_mem[2] = {{0}, {0}};
	int ret;
	
	ret = vos_mem_init_cma_info(&dprof_mem[0], VOS_MEM_CMA_TYPE_CACHE, POOL_SIZE);
	if (ret >= 0) {
		handle[0] = vos_mem_alloc_from_cma(&dprof_mem[0]);
	} else {
		printk("dprof_init allocate mem fail!!\r\n");
		return E_SYS;
	}	
	g_dprof_pool_addr[0] = (UINT32)dprof_mem[0].vaddr;
	if (arb_get_reg(DDR_ARB_2, 0x04) & 0x20) {
		ret = vos_mem_init_cma_info(&dprof_mem[1], VOS_MEM_CMA_TYPE_CACHE, POOL_SIZE);
		if (ret >= 0) {
			handle[1] = vos_mem_alloc_from_cma(&dprof_mem[1]);
		} else {
			printk("dprof_init allocate mem fail!!\r\n");
			return E_SYS;
		}	
		g_dprof_pool_addr[1] = (UINT32)dprof_mem[1].vaddr;
		g_dprof_pool_size[1] = POOL_SIZE;
	}

	g_dprof_pool_size[0] = POOL_SIZE;


	lock = ddr_arb_platform_spin_lock();

	g_dprof_state[0] = DPROF_STATE_IDLE;
	g_dprof_state[1] = DPROF_STATE_IDLE;
	gp_dprof_queue[0] = (DPROF_NODE*)g_dprof_pool_addr[0];
	gp_dprof_queue[1] = (DPROF_NODE*)g_dprof_pool_addr[1];
	dprof_queue_length[0] = g_dprof_pool_size[0] / sizeof(DPROF_NODE);
	dprof_queue_length[1] = g_dprof_pool_size[1] / sizeof(DPROF_NODE);
	memset(&g_dprof_node_cache[0], 0, sizeof(g_dprof_node_cache[0]));
	memset(&g_dprof_node_cache[1], 0, sizeof(g_dprof_node_cache[1]));
	g_dprof_time_slice_idx[0] = 0;
	g_dprof_time_slice_idx[1] = 0;

	ddr_arb_platform_spin_unlock(lock);
	
	is_dprof = 1;

	return E_OK;
}

void dprof_bandwidth_monitor(DDR_ARB id, UINT32 events)
{
	UINT32 i;
	T_DMA_RECORD0_SIZE_REG size_reg;
	T_DMA_RECORD0_COUNT_REG cnt_reg;

	if (g_dprof_state[id] == DPROF_STATE_IDLE) return;

	for (i = 0; i < DMA_MONITOR_ALL; i++) {
		UINT32 size_value, req_value;

		size_reg.reg = arb_get_reg(id, DMA_RECORD0_SIZE_REG_OFS + i * 8);
		cnt_reg.reg = arb_get_reg(id, DMA_RECORD0_COUNT_REG_OFS + i * 8);

		if (size_reg.bit.MONITOR_SIZE_OVF) {
			size_value = 0xFFFFFFF;
		} else {
			size_value = size_reg.bit.MONITOR_ACC_SIZE;
		}

		if (cnt_reg.bit.MONITOR_REQ_OVF) {
			req_value = 0xFFFFFFF;
		} else {
			req_value = cnt_reg.bit.MONITOR_ACC_REQ;
		}

		g_dprof_node_cache[id].vSize[i] += size_value;

		g_dprof_node_cache[id].vRequest[i] += req_value;
	}
	g_dprof_node_cache[id].vSize[DMA_MONITOR_ALL] += dma_get_utilization(id);

	g_dprof_time_slice_idx[id]++;
	if ((g_dprof_time_slice_idx[id]%10) == 0) {

		//printk("g_dprof_time_slice_idx = %d\r\n", g_dprof_time_slice_idx);
		// 1 time slice is 25ms, 10 time slices is 250ms
		// push to queue per 250ms
		memcpy(&gp_dprof_queue[id][idx_tail[id]], &g_dprof_node_cache[id],
			sizeof(g_dprof_node_cache[id]));

		gp_dprof_queue[id][idx_tail[id]].tick =
			timer_get_current_count(timer_get_sys_timer_id());

		// clear cache
		memset(&g_dprof_node_cache[id], 0, sizeof(g_dprof_node_cache[id]));

		// advance queue index
		idx_tail[id] = (idx_tail[id] + 1) % dprof_queue_length[id];
		if (idx_tail[id] == idx_head[id]) {         // Queue overflow
			b_dprof_queue_overflow[id] = 1;     // Mark overflow flag
			// Advance head index:
			//	remove head element to create 1 empty slot
			idx_head[id] = (idx_head[id] + 1) % dprof_queue_length[id];
		}
	}
}

BOOL dprof_start(DDR_ARB id, DPROF_CH ch, DMA_DIRECTION dir)
{
	UINT32 lock;

	if (ch == DPROF_CPU) {
		dma_configMonitor(id, DMA_MONITOR_CH0, DMA_CH_CPU, dir);
		dma_configMonitor(id, DMA_MONITOR_CH1, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH2, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH3, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH4, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH5, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH6, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH7, DMA_CH_RSV, dir);
	} else if (ch == DPROF_CNN) {
		dma_configMonitor(id, DMA_MONITOR_CH0, DMA_CH_CNN_0, dir);
		dma_configMonitor(id, DMA_MONITOR_CH1, DMA_CH_CNN_1, dir);
		dma_configMonitor(id, DMA_MONITOR_CH2, DMA_CH_CNN_2, dir);
		dma_configMonitor(id, DMA_MONITOR_CH3, DMA_CH_CNN_3, dir);
		dma_configMonitor(id, DMA_MONITOR_CH4, DMA_CH_CNN_4, dir);
		dma_configMonitor(id, DMA_MONITOR_CH5, DMA_CH_CNN_5, dir);
		dma_configMonitor(id, DMA_MONITOR_CH6, DMA_CH_CNN_6, dir);
		dma_configMonitor(id, DMA_MONITOR_CH7, DMA_CH_RSV, dir);
	} else if (ch == DPROF_CNN2) {
		dma_configMonitor(id, DMA_MONITOR_CH0, DMA_CH_CNN2_0, dir);
		dma_configMonitor(id, DMA_MONITOR_CH1, DMA_CH_CNN2_1, dir);
		dma_configMonitor(id, DMA_MONITOR_CH2, DMA_CH_CNN2_2, dir);
		dma_configMonitor(id, DMA_MONITOR_CH3, DMA_CH_CNN2_3, dir);
		dma_configMonitor(id, DMA_MONITOR_CH4, DMA_CH_CNN2_4, dir);
		dma_configMonitor(id, DMA_MONITOR_CH5, DMA_CH_CNN2_5, dir);
		dma_configMonitor(id, DMA_MONITOR_CH6, DMA_CH_CNN2_6, dir);
		dma_configMonitor(id, DMA_MONITOR_CH7, DMA_CH_RSV, dir);
	} else if (ch == DPROF_NUE) {
		dma_configMonitor(id, DMA_MONITOR_CH0, DMA_CH_NUE_0, dir);
		dma_configMonitor(id, DMA_MONITOR_CH1, DMA_CH_NUE_1, dir);
		dma_configMonitor(id, DMA_MONITOR_CH2, DMA_CH_NUE_2, dir);
		dma_configMonitor(id, DMA_MONITOR_CH3, DMA_CH_NUE_3, dir);
		dma_configMonitor(id, DMA_MONITOR_CH4, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH5, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH6, DMA_CH_RSV, dir);
		dma_configMonitor(id, DMA_MONITOR_CH7, DMA_CH_RSV, dir);
	} else if (ch == DPROF_NUE2) {
		dma_configMonitor(id, DMA_MONITOR_CH0, DMA_CH_NUE2_0, dir);
		dma_configMonitor(id, DMA_MONITOR_CH1, DMA_CH_NUE2_1, dir);
		dma_configMonitor(id, DMA_MONITOR_CH2, DMA_CH_NUE2_2, dir);
		dma_configMonitor(id, DMA_MONITOR_CH3, DMA_CH_NUE2_3, dir);
		dma_configMonitor(id, DMA_MONITOR_CH4, DMA_CH_NUE2_4, dir);
		dma_configMonitor(id, DMA_MONITOR_CH5, DMA_CH_NUE2_5, dir);
		dma_configMonitor(id, DMA_MONITOR_CH6, DMA_CH_NUE2_6, dir);
		dma_configMonitor(id, DMA_MONITOR_CH7, DMA_CH_RSV, dir);
	} else {
		return FALSE;
	}

	g_dprof_start_tick[id] = timer_get_current_count(timer_get_sys_timer_id());

	lock = ddr_arb_platform_spin_lock();

	idx_head[id] = 0;
	idx_tail[id] = 0;
	b_dprof_queue_overflow[id] = 0;

	memset(&g_dprof_node_cache[id], 0, sizeof(g_dprof_node_cache[id]));
	g_dprof_time_slice_idx[id] = 0;

	g_dprof_state[id] = DPROF_STATE_MONITOR_ONGOING;

	ddr_arb_platform_spin_unlock(lock);

	return TRUE;
}

static int dprof_print_data_length(char *buff, UINT32 buflen, UINT32 uiBytes) {
	int len, outlen;

	len = snprintf(NULL, 0, "%d", uiBytes);
	if (len > 6) {          // MB
		//printk("%7dMB ", (uiBytes+500000)/1000000);
		//outlen = snprintf(buff, buflen, "%7dMB ", (uiBytes+500000)/1048576);
		outlen = snprintf(buff, buflen, "%5d.%01dMB", uiBytes/1048576, ((uiBytes%1048576)*10)/1048576);
	} else if (len > 3) {   // KB
		//printk("%7dKB ", (uiBytes+500)/1000);
		//outlen = snprintf(buff, buflen, "%7dKB ", (uiBytes+500)/1024);
		outlen = snprintf(buff, buflen, "%5d.%01dKB ", uiBytes/1024, ((uiBytes%1024)*10)/1024);
	} else {
		outlen = snprintf(buff, buflen, "%8dB ", uiBytes);
	}
	return outlen;
}


BOOL dprof_dump_and_stop(DDR_ARB id)
{
	UINT32 i, j, count;
	UINT32 lock;
	UINT32 freq = 933;
	char tempstr[128];
	UINT32 total_len, len, remain_len, buffsize;
	UINT32 total_size = 0;
	UINT32 total_size_per_ch[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	if( nvt_get_chip_id() == CHIP_NA51084)
		freq = 1066;

	g_dprof_end_tick[id] = timer_get_current_count(timer_get_sys_timer_id());

	// disable monitor sw
	lock = ddr_arb_platform_spin_lock();
	g_dprof_state[id] = DPROF_STATE_IDLE;
	ddr_arb_platform_spin_unlock(lock);

	printk("------------------------------------------------------------\r\n");
	printk("dprof start tick: %u, end tick: %u\r\n", g_dprof_start_tick[id], g_dprof_end_tick[id]);
	printk("dprof log addr: 0x%x, size: 0x%x, max duration %d ms\r\n",
		g_dprof_pool_addr[id], g_dprof_pool_size[id], dprof_queue_length[id]*250);
	printk("------------------------------------------------------------\r\n");
	if (b_dprof_queue_overflow[id]) {
		printk("DPROF traffic queue overflow, recording time is too long!!\r\n");
		printk("ONLY recent traffic record is preserved\r\n");
	}

	printk("head %d, tail %d\r\n", idx_head[id], idx_tail[id]);
	tempstr[0]= 0x00;
	buffsize = sizeof(tempstr);
	total_len = 0;
	remain_len = buffsize;
	for (i=0; i<DMA_MONITOR_ALL; i++) {
		DMA_CH chDma = 0;
		DMA_DIRECTION direction = DMA_DIRECTION_BOTH;

		dma_getMonitorconfig(id, i, &chDma, &direction);
		//printk("%9.9s ", dma_getChannelName(chDma));
		len = snprintf(&tempstr[total_len], buffsize, "%9.9s ", dma_getChannelName(chDma));
		if (len >= (buffsize - total_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
	}
	//printk("%9.9s\r\n", "Total");
	len = snprintf(&tempstr[total_len], buffsize, "%9.9s\r\n", "Total");
	if (len >= (buffsize - total_len)) {
		goto len_err;
	}
	printk("%s", tempstr);
	tempstr[0]= 0x00;
	buffsize = sizeof(tempstr);
	total_len = 0;
	remain_len = buffsize;
	for (i=0; i<DMA_MONITOR_ALL; i++) {
		DMA_CH chDma = 0;
		DMA_DIRECTION direction = DMA_DIRECTION_BOTH;

		dma_getMonitorconfig(id, i, &chDma, &direction);
		if (direction == DMA_DIRECTION_READ) {
			//printk("%9.9s ", "READ");
			len = snprintf(&tempstr[total_len], buffsize, "%9.9s ", "READ");
		} else if (direction == DMA_DIRECTION_WRITE) {
			//printk("%9.9s ", "WRITE");
			len = snprintf(&tempstr[total_len], buffsize, "%9.9s ", "WRITE");
		} else {
			//printk("%9.9s ", "BOTH");
			len = snprintf(&tempstr[total_len], buffsize, "%9.9s ", "BOTH");
		}
		if (len >= (buffsize - total_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;

	}
	//printk("%9.9s\r\n", "Util");
	len = snprintf(&tempstr[total_len], buffsize, "%9.9s\r\n", "Util");
	if (len >= (buffsize - total_len)) {
		goto len_err;
	}
	printk("%s", tempstr);

	count = 0;
	i = idx_head[id];
	while (i != idx_tail[id]) {
		printk("\r\nQueue idx %d ", i);
		printk("Tick %u us\r\n", gp_dprof_queue[id][i].tick);

		// length
		tempstr[0]= 0x00;
		buffsize = sizeof(tempstr);
		total_len = 0;
		remain_len = buffsize;
		for (j = 0; j < DMA_MONITOR_ALL; j++) {
			//dprof_print_data_length(gp_dprof_queue[i].vSize[j]*4);
			len = dprof_print_data_length(&tempstr[total_len], buffsize, gp_dprof_queue[id][i].vSize[j]*4);
			total_size += gp_dprof_queue[id][i].vSize[j]; // CLIFF
			total_size_per_ch[j] += gp_dprof_queue[id][i].vSize[j]; //CLIFF
			if (len >= (buffsize - total_len)) {
				goto len_err;
			}
			total_len += len;
			remain_len -= len;
		}
		//printk("\r\n");
		len = snprintf(&tempstr[total_len], buffsize, "\r\n");
		if (len >= (buffsize - total_len)) {
			goto len_err;
		}
		printk("%s", tempstr);
		// utilization
		tempstr[0]= 0x00;
		buffsize = sizeof(tempstr);
		total_len = 0;
		remain_len = buffsize;
		for (j = 0; j < DMA_MONITOR_ALL; j++) {
			const UINT32 PEAK_SIZE = freq*2*4000000;
			UINT32 num;
//			UINT32 denom;

			num = gp_dprof_queue[id][i].vSize[j]*4*100/PEAK_SIZE;
//			denom = gp_dprof_queue[i].vSize[j]*4*100/PEAK_SIZE;

			//printk("%8d%% ", num);
			len = snprintf(&tempstr[total_len], buffsize, "%8d%% ", num);
			if (len >= (buffsize - total_len)) {
				goto len_err;
			}
			total_len += len;
			remain_len -= len;
		}
		//printk("%8d%%\t ", gp_dprof_queue[i].vSize[DMA_MONITOR_ALL]/10);
		//printk("\r\n");
		len = snprintf(&tempstr[total_len], buffsize, "%8d%%\t \r\n", gp_dprof_queue[id][i].vSize[DMA_MONITOR_ALL]/(10 * dma_get_monitor_period(id)/100));
		if (len >= (buffsize - total_len)) {
			goto len_err;
		}
		printk("%s", tempstr);
		i = (i + 1) % dprof_queue_length[id];
		count++;
	}
	printk("total size per channel\r\n");
	tempstr[0]= 0x00;
	buffsize = sizeof(tempstr);
	total_len = 0;
	remain_len = buffsize;
	for (j = 0; j < DMA_MONITOR_ALL; j++) {
		len = dprof_print_data_length(&tempstr[total_len], buffsize, total_size_per_ch[j]*4);
		if (len >= (buffsize - total_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
	}
	//printk("\r\n");
	len = snprintf(&tempstr[total_len], buffsize, "\r\n");
	if (len >= (buffsize - total_len)) {
		goto len_err;
	}
	printk("%s", tempstr);

	tempstr[0]= 0x00;
	buffsize = sizeof(tempstr);
	total_len = 0;
	remain_len = buffsize;
	len = snprintf(&tempstr[total_len], buffsize, "total size  ");
	if (len >= (buffsize - total_len)) {
		goto len_err;
	}
	total_len += len;
	remain_len -= len;
	len = dprof_print_data_length(&tempstr[total_len], buffsize, total_size*4);
	total_len += len;
	remain_len -= len;
	len = snprintf(&tempstr[total_len], buffsize, "\r\n");
	if (len >= (buffsize - total_len)) {
		goto len_err;
	}
	printk("%s", tempstr);
	return TRUE;
len_err:
	printk("\r\n");
	return FALSE;
}
#else
void dprof_bandwidth_monitor(DDR_ARB id, UINT32 events)
{
}
#endif
int nvt_ddr_arb_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ddr_arb_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_ddr_arb_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;

	init_waitqueue_head(&pmodule_info->xxx_wait_queue);
	// coverity[side_effect_free]
	spin_lock_init(&pmodule_info->xxx_spinlock);
	sema_init(&pmodule_info->xxx_sem, 1);
	init_completion(&pmodule_info->xxx_completion);
	tasklet_init(&pmodule_info->xxx_tasklet, nvt_ddr_arb_drv_do_tasklet, (unsigned long)pmodule_info);

	/* allocate graphic resource here */
        ddr_arb_platform_create_resource(pmodule_info);

	//printk("%s: resource done\r\n", __func__);


	/* initial clock here */



	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_ddr_arb_drv_isr, IRQF_TRIGGER_HIGH, "ARB_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
	if (request_irq(pmodule_info->iinterrupt_id[1], nvt_ddr_arb2_drv_isr, IRQF_TRIGGER_HIGH, "ARB2_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[1]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ2;
	}

	dma_set_system_priority(TRUE);
	
#if DPROF_EN
	if (dprof_init() != E_OK) {
		return E_SYS;
	}
#endif	

	arb_init();


	/* Add HW Module initialization here when driver loaded */

	return err;

FAIL_FREE_IRQ2:
	free_irq(pmodule_info->iinterrupt_id[1], pmodule_info);
FAIL_FREE_IRQ:
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return err;
}

int nvt_ddr_arb_drv_remove(MODULE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);
	free_irq(pmodule_info->iinterrupt_id[1], pmodule_info);

	/* Add HW Moduel release operation here*/

	/* release OS resources */
        ddr_arb_platform_release_resource();


	return 0;
}

int nvt_ddr_arb_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ddr_arb_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_ddr_arb_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	MON_INFO mon_info = {0};
	int __user *argp = (int __user *)arg;

	//nvt_dbg(IND, "IF-%d cmd:%x\n =>%08X", if_id, cmd,DDR_ARB_IOC_CHKSUM);

	switch (cmd) {
	case DDR_ARB_IOC_CHKSUM: {
			ARB_CHKSUM param = {0};
			if (unlikely(copy_from_user(&param, argp, sizeof(param)))) {
				DBG_ERR("failed to copy_from_user\r\n");
				return -EFAULT;
			}
			if (param.version != DDR_ARB_IOC_VERSION) {
				DBG_ERR("version not matched kernel(%08X), user(%08X)\r\n", DDR_ARB_IOC_VERSION, param.version);
				return -EFAULT;
			}
			param.sum = arb_chksum(param.ddr_id, param.phy_addr, param.len);
			if (unlikely(copy_to_user(argp, &param, sizeof(param)))) {
                                DBG_ERR("failed to copy_to_user\r\n");
                                return -EFAULT;
                        }
		} break;
	case DDR_IOC_MON_START:
		/*call someone to start operation*/
		err = copy_from_user(&mon_info, argp, sizeof(MON_INFO));
        if(!err) {
			if ((mon_info.dram < 2) && (mon_info.dram >= 0)) {
				mau_ch_mon_start(mon_info.ch, mon_info.rw, mon_info.dram);
			} else {
				nvt_dbg(ERR, "dram id error\n");
			}
		} else {
			nvt_dbg(ERR, "copy from user err\n");
		}
		break;
	case DDR_IOC_MON_STOP:
		/*call someone to start operation*/
		err = copy_from_user(&mon_info, argp, sizeof(MON_INFO));
        if(!err) {
			if ((mon_info.dram < 2) && (mon_info.dram >= 0)) {
				mon_info.count = mau_ch_mon_stop(mon_info.ch, mon_info.dram);
			} else {
				nvt_dbg(ERR, "dram id error\n");
			}
    	 	err = copy_to_user(argp, &mon_info, sizeof(MON_INFO));
		} else {
			nvt_dbg(ERR, "copy from user err\n");
		}
		break;	}

	return err;
}

irqreturn_t nvt_ddr_arb_drv_isr(int irq, void *devid)
{
	arb_isr(is_dprof);

	/* simple triggle and response mechanism*/
//	complete(&pmodule_info->xxx_completion);


	/*  Tasklet for bottom half mechanism */
//	tasklet_schedule(&pmodule_info->xxx_tasklet);

	return IRQ_HANDLED;
}

irqreturn_t nvt_ddr_arb2_drv_isr(int irq, void *devid)
{
	arb2_isr(is_dprof);

	/* simple triggle and response mechanism*/
//	complete(&pmodule_info->xxx_completion);


	/*  Tasklet for bottom half mechanism */
//	tasklet_schedule(&pmodule_info->xxx_tasklet);

	return IRQ_HANDLED;
}

int nvt_ddr_arb_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->xxx_completion);
	return 0;
}



void nvt_ddr_arb_drv_do_tasklet(unsigned long data)
{
#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_TASKLET)
	ddr_arb_platform_ist(data);
#endif
}
