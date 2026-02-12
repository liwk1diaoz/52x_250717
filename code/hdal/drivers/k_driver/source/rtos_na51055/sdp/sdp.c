/*
    SDP module driver

    This file is the driver of SDP module

    @file       sdp.c
    @ingroup    mIDrvIO_SDP
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <string.h>
#include "interrupt.h"
#include "sdp.h"
#include "sdp_reg.h"
#include "sdp_int.h"
#include "pll.h"
#include "pll_protected.h"
#include <plat/nvt-sramctl.h>
//#include "top.h"
#include "gpio.h"
//#include "timer.h"
#include "dma.h"
//#include "ist.h"
//#include "Utility.h"
#include "pad.h"

#define __MODULE__  freertos_drv_sdp
#define __DBGLVL__    2
#include <kwrap/debug.h>

static  VK_DEFINE_SPINLOCK(sdp_spinlock);

static ID               FLG_ID_SDP;
static SEM_HANDLE       SEMID_SDP;


static SEM_HANDLE v_semaphore[SDP_ID_COUNT] = {
	0,
};

static BOOL v_isopened[SDP_ID_COUNT] = {
	FALSE,
};

static BOOL v_isattached[SDP_ID_COUNT] = {
	FALSE,
};


static ID       v_lock_status[SDP_ID_COUNT] = {
	NO_TASK_LOCKED,
};

//const static INT_ID v_int_enum[SDP_ID_COUNT] = {
//	INT_ID_SDP,
//};

const static CG_EN v_clk_en[SDP_ID_COUNT] = {
	SDP_CLK,
};

static DRV_CB v_sdp_event_handler[SDP_ID_COUNT] = {
	NULL,
};

static SDP_TRANSFER_MODE v_sdp_transfer_mode[SDP_ID_COUNT] = {
	SDP_TRANSFER_MODE_SLICE,
};

static UINT32 v_ist_event[SDP_ID_COUNT] = {
	0,
};

const static FLGPTN v_flg_ptn[SDP_ID_COUNT] = {
	FLGPTN_BIT(0),
};

static THREAD_HANDLE v_ist_tsk_id[SDP_ID_COUNT] = {
	0,
};


//
// Internal function prototype
//
static void sdp_kick_ist(SDP_ID id, UINT32 event);
#if (_EMULATION_ == ENABLE)
extern BOOL sdptest_compare_default_reg(void);
#endif


/*
    Lock SDP module

    @param[in] id       SDP controller ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER sdp_lock(SDP_ID id)
{
	ER ret;

	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	ret        = SEM_WAIT(v_semaphore[id]);
	if (ret != E_OK) {
		DBG_ERR("SDP%d: wait semaphore fail\r\n", id);
		return ret;
	}

	v_lock_status[id]   = TASK_LOCKED;
	return E_OK;
}

/*
    Unlock SDP module

    @param[in] id       SDP controller ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER sdp_unlock(SDP_ID id)
{
	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	SEM_SIGNAL(v_semaphore[id]);
	v_lock_status[id]   = NO_TASK_LOCKED;

	return E_OK;
}

/*
    Attach SDP driver

    @param[in] id       SDP controller ID

    Pre-initialize SDP driver before SDP driver is opened. This function should be the very first function to be invoked.

    @return void
*/
static void sdp_attach(SDP_ID id)
{
	if (id >= SDP_ID_COUNT) {
		return;
	}
	if (v_isattached[id] == FALSE) {
		// Enable SPI clock
		pll_enableClock(v_clk_en[id]);

		v_isattached[id] = TRUE;
	}

}

/*
    Detach SDP driver

    De-initialize SDP driver after SDP driver is closed.

    @param[in] id       SDP controller ID

    @return void
*/
static void sdp_detach(SDP_ID id)
{
	if (id >= SDP_ID_COUNT) {
		return;
	}

	if (v_isattached[id] == TRUE) {
		pll_disableClock(v_clk_en[id]);
		v_isattached[id] = FALSE;
	}

}

/*
    SDP interrupt handler

    @return void
*/
static irqreturn_t sdp_isr(int irq, void *devid)
{
	T_SDP_STATUS_REG sts_reg;
	T_SDP_INTEN_REG inten_reg;

	sts_reg.reg = SDP_GETREG(SDP_STATUS_REG_OFS);
	inten_reg.reg = SDP_GETREG(SDP_INTEN_REG_OFS);
	sts_reg.reg &= inten_reg.reg;
	if (sts_reg.reg == 0) {
		return IRQ_NONE;
	}
	SDP_SETREG(SDP_STATUS_REG_OFS, sts_reg.reg);

#if (DRV_SUPPORT_IST == ENABLE)
	if (v_sdp_event_handler[SDP_ID_1] != NULL) {
		sdp_kick_ist(SDP_ID_1, sts_reg.reg);
//        kick_bh(INT_ID_SDP, sts_reg.reg, NULL);
	}
#else
	if (v_sdp_event_handler[SDP_ID_1] != NULL) {
		v_sdp_event_handler[SDP_ID_1](sts_reg.reg);
	}
#endif

	return IRQ_HANDLED;
}

/**
    Open SDP driver


    @param[in] id       SDP controller ID

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER sdp_open(SDP_ID id)
{
	ER ret;
	UINT32 flags;
	T_SDP_CTRL_REG ctrl_reg = {0};
	T_SDP_INTEN_REG inten_reg = {0};

	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	if (v_isopened[id]) {
		return E_OK;
	}
	ret = sdp_lock(id);
	if (ret != E_OK) {
		DBG_ERR("SDP%d: lock fail\r\n", id);
		return ret;
	}

	if (v_sdp_event_handler[id] == NULL) {
		DBG_WRN("No handler installed by sdp_set.\r\n");
	}

//    inten_reg.bit.fifo_loaded = 1;
	inten_reg.bit.fifo_underrun = 1;
	inten_reg.bit.dma_exhausted = 1;
	inten_reg.bit.dma_end = 1;
	inten_reg.bit.mrx_fifo_emptry = 1;
	inten_reg.bit.mtx_fifo_full = 1;
	inten_reg.bit.dma_cmd_not_align = 1;

	// Enter critical section
	vk_spin_lock_irqsave(&sdp_spinlock, flags);

#if (DRV_SUPPORT_IST == ENABLE)
//	pfIstCB_SDP = v_sdp_event_handler[id];
#endif

	ctrl_reg.reg = SDP_GETREG(SDP_CTRL_REG_OFS);
	ctrl_reg.bit.dma_en = 0;
	ctrl_reg.bit.transfer_mode = v_sdp_transfer_mode[id];
	SDP_SETREG(SDP_CTRL_REG_OFS, ctrl_reg.reg);

	SDP_SETREG(SDP_INTEN_REG_OFS, inten_reg.reg);

	// Leave critical section
	vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

	nvt_disable_sram_shutdown(SDP_SD);

	// Enable SPI interrupt
//	drv_enableInt(v_int_enum[id]);

//	pll_setClockFreq(vSpiPllClkID[id], vSpiInitInfo[id].uiFreq);

	sdp_attach(id);

	v_isopened[id] = TRUE;

	return E_OK;
}

/**
    Close SDP driver

    The last function to called before leaving this module.
    Disable the SDP module interrupt and sdp clock.

    @param[in] id       SDP controller ID

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER sdp_close(SDP_ID id)
{
	UINT32 flags;
//	UINT32 uiSpiFreq = 0;

	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	if (!v_isopened[id]) {
		return E_OK;
	}

//	spi_waitFlag(spiID);

	// Enter critical section
	vk_spin_lock_irqsave(&sdp_spinlock, flags);

#if (DRV_SUPPORT_IST == ENABLE)
//	pfIstCB_SDP = NULL;
#endif

	// Leave critical section
	vk_spin_unlock_irqrestore(&sdp_spinlock, flags);


	// Release interrupt
//	drv_disableInt(v_int_enum[id]);


	sdp_detach(id);

	v_isopened[id] = FALSE;

	return sdp_unlock(id);
}

/**
    trigger SDP hardware


    @param[in] id           SDP controller ID
    @param[in] p_param      the parameter for trigger
    @param[in] p_cb_func    the callback function
    @param[in] p_user_data  the private user data

    @return
        - @b E_OK: success
        - @b E_ID: invalid SDP ID
        - @b Else: fail
*/
ER sdp_trigger(SDP_ID id, SDP_TRIGGER_PARAM *p_param, DRV_CB p_cb_func, void *p_user_data)
{
	UINT32 flags;

	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	if (p_param == NULL) {
		DBG_ERR("p_param NULL\r\n");
		return E_PAR;
	}

	switch (p_param->command) {
	case SDP_CMD_SET_MASTER_RX_PORT:
	{
		T_SDP_READ_STS_REG sts_reg;
		T_SDP_MASTER_RX_PORT_REG mrx_port;

		if (p_param->p_data == NULL) {
			DBG_ERR("p_data NULL, skip\r\n");
			return E_PAR;
		}
		if (((UINT32)p_param->p_data) & 0x03) {
			DBG_ERR("p_data not word aligned: 0x%p\r\n", p_param->p_data);
			return E_PAR;
		}

		vk_spin_lock_irqsave(&sdp_spinlock, flags);

		sts_reg.reg = SDP_GETREG(SDP_READ_STS_REG_OFS);
		if (sts_reg.bit.mrx_port_full) {
			vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

			DBG_ERR("MRX FIFO already full, skip\r\n");
			return E_QOVR;
		}

		mrx_port.reg = *p_param->p_data;
		SDP_SETREG(SDP_MASTER_RX_PORT_REG_OFS, mrx_port.reg);

		vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

		//            DBG_ERR("Write MRX port 0x%lx\r\n", mrx_port.reg);
	}
		break;
	case SDP_CMD_GET_MASTER_TX_PORT:
	{
		T_SDP_READ_STS_REG sts_reg;
		T_SDP_MASTER_TX_PORT_REG mtx_port;

		if (p_param->p_data == NULL) {
			DBG_ERR("p_data NULL, skip\r\n");
			return E_PAR;
		}
		if (((UINT32)p_param->p_data) & 0x03) {
			DBG_ERR("p_data not word aligned: 0x%p\r\n", p_param->p_data);
			return E_PAR;
		}
		if (p_param->size < sizeof(UINT32)) {
			DBG_ERR("size below 4 byte: 0x%x\r\n", p_param->size);
			return E_PAR;
		}

		vk_spin_lock_irqsave(&sdp_spinlock, flags);

		sts_reg.reg = SDP_GETREG(SDP_READ_STS_REG_OFS);
		if (sts_reg.bit.mtx_port_full == 0) {
			vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

			DBG_ERR("MTX FIFO empty, skip\r\n");
			return E_QOVR;
		}

		mtx_port.reg = SDP_GETREG(SDP_MASTER_TX_PORT_REG_OFS);

		vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

		*p_param->p_data = mtx_port.reg;

		//            DBG_ERR("Read MTX port 0x%lx\r\n", mtx_port.reg);
	}
		break;
	case SDP_CMD_SET_DMA_BUF:
	{
		T_SDP_CTRL_REG ctrl_reg;
		T_SDP_DMA_ADDR_REG addr_reg = {0};
		T_SDP_DMA_SIZE_REG size_reg = {0};

		if (p_param->p_data == NULL) {
			DBG_ERR("p_data NULL, skip\r\n");
			return E_PAR;
		}
		if (((UINT32)p_param->p_data) & 0x03) {
			DBG_ERR("p_data not word aligned: 0x%p\r\n", p_param->p_data);
			return E_PAR;
		}
		if (p_param->size > SDP_DMA_LEN_MAX) {
			DBG_ERR("input size 0x%x > max DMA size 0x%x\r\n",
			p_param->size, SDP_DMA_LEN_MAX);
			return E_PAR;
		}

		// sync DRAM with cache
		vos_cpu_dcache_sync((VOS_ADDR)p_param->p_data, p_param->size, VOS_DMA_TO_DEVICE_NON_ALIGN);
		//            dma_flushWriteCache((UINT32)p_param->p_data, p_param->size);

		vk_spin_lock_irqsave(&sdp_spinlock, flags);

		ctrl_reg.reg = SDP_GETREG(SDP_CTRL_REG_OFS);
		if (ctrl_reg.bit.dma_en == 1) {
			vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

			DBG_ERR("DMA mode still running, skip\r\n");
			return E_QOVR;
		}

		//
		// start dma
		//
		size_reg.bit.buf_size = p_param->size;
		SDP_SETREG(SDP_DMA_SIZE_REG_OFS, size_reg.reg);
		addr_reg.bit.addr = (UINT32)p_param->p_data;
		SDP_SETREG(SDP_DMA_ADDR_REG_OFS, addr_reg.reg);
		ctrl_reg.bit.dma_en = 1;
		SDP_SETREG(SDP_CTRL_REG_OFS, ctrl_reg.reg);

		vk_spin_unlock_irqrestore(&sdp_spinlock, flags);
	}
		break;
	case SDP_CMD_ABORT_DMA:
	{
		T_SDP_CTRL_REG ctrl_reg;

		vk_spin_lock_irqsave(&sdp_spinlock, flags);

		ctrl_reg.reg = SDP_GETREG(SDP_CTRL_REG_OFS);
		ctrl_reg.bit.dma_en = 0;
		SDP_SETREG(SDP_CTRL_REG_OFS, ctrl_reg.reg);

		vk_spin_unlock_irqrestore(&sdp_spinlock, flags);
		DBG_ERR("%s: before wait DMA\r\n", __func__);
#if 1
		while (1) {
			ctrl_reg.reg = SDP_GETREG(SDP_CTRL_REG_OFS);
			if (ctrl_reg.bit.dma_en == 0) break;
		}
#endif
		DBG_ERR("%s: after wait DMA\r\n", __func__);

	}
		break;
	default:
		DBG_ERR("command 0x%x not supported\r\n", p_param->command);
		return E_NOSPT;
	}

	return E_OK;
}

/*!
 * @fn INT32 sdp_set(UINT32 handler, SDP_PARAM_ID id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
ER sdp_set(SDP_ID id, SDP_PARAM_ID param_id, VOID *p_param)
{
	UINT32 flags;

	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	switch (param_id) {
	case SDP_PARAM_ID_EVENT_CALLBACK:
		DBG_WRN("%s: SDP CB 0x%x, ref 0x%p\r\n", __func__, (unsigned int)p_param, (*(DRV_CB*)p_param));
		vk_spin_lock_irqsave(&sdp_spinlock, flags);
		v_sdp_event_handler[id] = p_param;
		vk_spin_unlock_irqrestore(&sdp_spinlock, flags);
		break;
	case SDP_PARAM_ID_TRANSFER_MODE:
	{
		SDP_TRANSFER_MODE *p_mode = (SDP_TRANSFER_MODE*)p_param;
		T_SDP_CTRL_REG ctrl_reg = {0};

		vk_spin_lock_irqsave(&sdp_spinlock, flags);
		v_sdp_transfer_mode[id] = *p_mode;
		ctrl_reg.reg = SDP_GETREG(SDP_CTRL_REG_OFS);
		ctrl_reg.bit.transfer_mode = v_sdp_transfer_mode[id];
		SDP_SETREG(SDP_CTRL_REG_OFS, ctrl_reg.reg);
		vk_spin_unlock_irqrestore(&sdp_spinlock, flags);
	}
		break;
	default:
		return E_NOSPT;
	}

	return E_OK;
}

#if (_EMULATION_ == ENABLE)
// Only compare the register that won't update value from RTC macro
const static SDP_REG_DEFAULT     v_spi_reg_default[] = {
	{ 0x00,     0x0000,                    "CtrlReg"     },
	{ 0x08,     0x0000,                    "IntStsReg"     },
	{ 0x0C,     0x0000,                    "IntEnReg"    },
	{ 0x10,     0x0000,                    "ReadStsReg"   },
	{ 0x14,     0x0000,                    "MrxPortReg"   },
	{ 0x18,     0x0000,                    "MtxPortReg"      },
	{ 0x20,     0x0000,                    "DmaAddrReg"  },
	{ 0x24,     0x0000,                    "DmaSizeReg"  },
	{ 0x28,     0x0000,                    "DmaTransferredReg"  },
};

/*
    (Verification code) Compare SDP register default value

    (Verification code) Compare SDP register default value.

    @return Compare status
        - @b FALSE  : Register default value is incorrect.
        - @b TRUE   : Register default value is correct.
*/
BOOL sdptest_compare_default_reg(void)
{
	UINT32  i;
	BOOL    b_return = TRUE;
	SDP_ID  ctrl_idx;
	const static UINT32 v_base_addr[] = {IOADDR_SDP_REG_BASE,
										 };

	// Compare register
	for (ctrl_idx = SDP_ID_1; ctrl_idx < SDP_ID_COUNT; ctrl_idx++) {
		for (i = 0; i < (sizeof(v_spi_reg_default) / sizeof(v_spi_reg_default[0])); i++) {
			if (INW(v_base_addr[ctrl_idx] + v_spi_reg_default[i].offset) != v_spi_reg_default[i].value) {
				DBG_ERR("SDP%d: %s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
						ctrl_idx + 1,
						v_spi_reg_default[i].p_name,
						v_spi_reg_default[i].offset,
						INW(v_base_addr[ctrl_idx] + v_spi_reg_default[i].offset),
						v_spi_reg_default[i].value);
				b_return = FALSE;
			}
		}
	}

	return b_return;
}


#endif

static void sdp_kick_ist(SDP_ID id, UINT32 event)
{
	UINT32 flags;

	vk_spin_lock_irqsave(&sdp_spinlock, flags);
	v_ist_event[id] |= event;
	vk_spin_unlock_irqrestore(&sdp_spinlock, flags);
	iset_flg(FLG_ID_SDP, v_flg_ptn[id]);
}

static void sdp_drv_do_tasklet(SDP_ID id, unsigned long event)
{
	if (event & SDP_EVENTS_FIFO_UNDERRUN) {
		DBG_WRN("%s: FIFO UNDERRUN\r\n", __func__);
	}
	if (event & SDP_EVENTS_DMA_EXHAUSTED) {
		DBG_WRN("%s: FIFO EXHAUSTED\r\n", __func__);
	}
	if (event & SDP_EVENTS_READ_DATA_UNALIGN) {
		DBG_WRN("%s: Master read data (cmd 0x03) with non align length\r\n", __func__);
	}

	if (v_sdp_event_handler[id] != NULL) {
		v_sdp_event_handler[id](event);
	}
}


static void sdp_ist(void)
{
	THREAD_ENTRY();

	//coverity[no_escape]
	while (1) {
		FLGPTN ptn = 0;
		UINT32 flg = v_flg_ptn[SDP_ID_1];

		vos_flag_wait(&ptn, FLG_ID_SDP, flg, TWF_ORW | TWF_CLR);

		if (ptn & v_flg_ptn[SDP_ID_1]) {
			UINT32 event;
			UINT32 flags;

			vk_spin_lock_irqsave(&sdp_spinlock, flags);
			event = v_ist_event[SDP_ID_1];
			v_ist_event[SDP_ID_1] &= ~event;
			vk_spin_unlock_irqrestore(&sdp_spinlock, flags);

			sdp_drv_do_tasklet(SDP_ID_1, event);
		}
	}
}

#if 0
static irq_bh_handler_t sdp_bh_ist(int irq, unsigned long event, void *data)
{
	sdp_drv_do_tasklet(event);

	return (irq_bh_handler_t) IRQ_HANDLED;
}
#endif

void sdp_platform_init(void)
{
	cre_flg(&FLG_ID_SDP, NULL, "sdp_flag");
	SEM_CREATE(SEMID_SDP, 1);

	v_semaphore[0] = SEMID_SDP;

	v_ist_tsk_id[SDP_ID_1] = vos_task_create(sdp_ist,  0, "SdpIst",   5,  4096);
	vos_task_resume(v_ist_tsk_id[SDP_ID_1]);

	request_irq(INT_ID_SDP, sdp_isr, IRQF_TRIGGER_HIGH, "SDP", 0);
//	request_irq_bh(INT_ID_SDP, (irq_bh_handler_t) sdp_bh_ist, IRQF_BH_PRI_HIGH);
}

void sdp_platform_uninit(void)
{
	free_irq(INT_ID_SDP, NULL);
	vos_task_destroy(v_ist_tsk_id[SDP_ID_1]);
	v_ist_tsk_id[SDP_ID_1] = 0;

	rel_flg(FLG_ID_SDP);
	SEM_DESTROY(SEMID_SDP);
}

