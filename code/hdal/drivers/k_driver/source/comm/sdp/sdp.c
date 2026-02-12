/*
    SDP module driver

    This file is the driver of SDP module

    @file       sdp.c
    @ingroup    mIDrvIO_SDP
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "plat/top.h"
#include "pll_protected.h"
#include <plat/nvt-sramctl.h>
#include "kwrap/consts.h"
#include "kwrap/semaphore.h"
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include <mach/fmem.h>
#include <asm/div64.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include "comm/sdp.h"
#include "sdp_reg.h"
#include "sdp_int.h"
#include "sdp_dbg.h"


static DEFINE_SPINLOCK(sdp_spinlock);
static unsigned long sdp_spinlock_flags;
#define loc_cpu() spin_lock_irqsave(&sdp_spinlock, sdp_spinlock_flags)
#define unl_cpu() spin_unlock_irqrestore(&sdp_spinlock, sdp_spinlock_flags)
#define FLG_ID_SDP_END      FLGPTN_BIT(30)

static UINT32 _REGIOBASE = 0;
static bool sdp_ist_en = 0 ;

static ID               FLG_ID_SDP;
static SEM_HANDLE       SEMID_SDP;


static SEM_HANDLE v_semaphore[SDP_ID_COUNT] = {
	0,
};

static BOOL v_isopened[SDP_ID_COUNT] = {
	FALSE,
};

//static BOOL v_isattached[SDP_ID_COUNT] = {
//	FALSE,
//};


static ID       v_lock_status[SDP_ID_COUNT] = {
	NO_TASK_LOCKED,
};

//const static INT_ID v_int_enum[SDP_ID_COUNT] = {
//	INT_ID_SDP,
//};

//const static CG_EN v_clk_en[SDP_ID_COUNT] = {
//	SDP_CLK,
//};

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

	ret        = SEM_WAIT(SEMID_SDP);
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

	SEM_SIGNAL(SEMID_SDP);
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

#if defined(__FREERTOS)
	if (v_isattached[id] == FALSE) {
		// Enable SPI clock
		pll_enableClock(SDP_CLK);
		v_isattached[id] = TRUE;
	}
#endif
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
#if defined(__FREERTOS)
	if (v_isattached[id] == TRUE) {
		pll_disableClock(SDP_CLK);
		v_isattached[id] = FALSE;
	}
#endif
}

/*
    SDP interrupt handler

    @return void
*/
#if defined(__FREERTOS)
static irqreturn_t sdp_isr(int irq, void *devid)
#else
void sdp_isr (void)
#endif
{
    UINT32 sdp_isr_buf;
	RCW_DEF(SDP_STATUS_REG);
	RCW_DEF(SDP_INTEN_REG);

 	sdp_isr_buf = RCW_LD(SDP_STATUS_REG);
	sdp_isr_buf &= RCW_LD(SDP_INTEN_REG);

	if (sdp_isr_buf == 0) {
        return;
	}

	RCW_VAL(SDP_STATUS_REG) = sdp_isr_buf;
	RCW_ST(SDP_STATUS_REG);

#if (DRV_SUPPORT_IST == ENABLE)
    if (v_sdp_event_handler[SDP_ID_1] != NULL) {
        sdp_kick_ist(SDP_ID_1, RCW_VAL(SDP_STATUS_REG));
        //kick_bh(INT_ID_SDP, sts_reg.reg, NULL);
    }
#else
    if (v_sdp_event_handler[SDP_ID_1] != NULL) {
        v_sdp_event_handler[SDP_ID_1](RCW_VAL(SDP_STATUS_REG));
    }
#endif

    //return IRQ_HANDLED;
}

void sdp_init(UINT32 pmodule_info)
{
	_REGIOBASE = (UINT32)pmodule_info;
    //DBG_WRN("sdp_init, reg : 0x%x\n", _REGIOBASE);
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
    RCW_DEF(SDP_CTRL_REG);
    RCW_DEF(SDP_INTEN_REG);

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

	//RCW_OF(SDP_INTEN_REG).fifo_loaded = 1;
    RCW_OF(SDP_INTEN_REG).fifo_underrun = 1;
    RCW_OF(SDP_INTEN_REG).dma_exhausted = 1;
    RCW_OF(SDP_INTEN_REG).dma_end = 1;
    RCW_OF(SDP_INTEN_REG).mrx_fifo_emptry = 1;
    RCW_OF(SDP_INTEN_REG).mtx_fifo_full = 1;
    RCW_OF(SDP_INTEN_REG).dma_cmd_not_align = 1;

    // Enter critical section
    loc_cpu();

#if (DRV_SUPPORT_IST == ENABLE)
//	pfIstCB_SDP = v_sdp_event_handler[id];
#endif

    RCW_LD(SDP_CTRL_REG);
    RCW_OF(SDP_CTRL_REG).dma_en = 0;
    RCW_OF(SDP_CTRL_REG).transfer_mode = 1;
    RCW_ST(SDP_CTRL_REG);
    RCW_ST(SDP_INTEN_REG);

    // Leave critical section
    unl_cpu();

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
//	UINT32 uiSpiFreq = 0;

	if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

	if (!v_isopened[id]) {
		return E_OK;
	}

//	spi_waitFlag(spiID);

    // Enter critical section
    loc_cpu();

#if (DRV_SUPPORT_IST == ENABLE)
//	pfIstCB_SDP = NULL;
#endif

    // Leave critical section
    unl_cpu();

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
			RCW_DEF(SDP_READ_STS_REG);
			RCW_DEF(SDP_MASTER_RX_PORT_REG);

            if (p_param->p_data == NULL) {
                DBG_ERR("p_data NULL, skip\r\n");
                return E_PAR;
            }
            if (((UINT32)p_param->p_data) & 0x03) {
                DBG_ERR("p_data not word aligned: 0x%x\r\n", (UINT32)p_param->p_data);
                return E_PAR;
            }

            loc_cpu();

			RCW_LD(SDP_READ_STS_REG);
			if 	(RCW_OF(SDP_READ_STS_REG).mrx_port_full) {
                unl_cpu();

                DBG_ERR("MRX FIFO already full, skip\r\n");
                return E_QOVR;
            }

			RCW_VAL(SDP_MASTER_RX_PORT_REG) = *p_param->p_data;
			RCW_ST(SDP_MASTER_RX_PORT_REG);

            unl_cpu();

//            DBG_ERR("Write MRX port 0x%lx\r\n", mrx_port.reg);
        }
        break;
    case SDP_CMD_GET_MASTER_TX_PORT:
        {
			RCW_DEF(SDP_READ_STS_REG);
			RCW_DEF(SDP_MASTER_TX_PORT_REG);

            if (p_param->p_data == NULL) {
                DBG_ERR("p_data NULL, skip\r\n");
                return E_PAR;
            }
            if (((UINT32)p_param->p_data) & 0x03) {
                DBG_ERR("p_data not word aligned: 0x%x\r\n", (UINT32)p_param->p_data);
                return E_PAR;
            }
            if (p_param->size < sizeof(UINT32)) {
                DBG_ERR("size below 4 byte: 0x%x\r\n", p_param->size);
                return E_PAR;
            }

            loc_cpu();

			RCW_LD(SDP_READ_STS_REG);
			if (RCW_OF(SDP_READ_STS_REG).mtx_port_full == 0) {
                unl_cpu();

                DBG_ERR("MTX FIFO empty, skip\r\n");
                return E_QOVR;
            }

            //mtx_port.reg = SDP_GETREG(SDP_MASTER_TX_PORT_REG_OFS);
			*p_param->p_data = RCW_LD(SDP_MASTER_TX_PORT_REG);

            unl_cpu();

            //*p_param->p_data = mtx_port.reg;


//            DBG_ERR("Read MTX port 0x%lx\r\n", mtx_port.reg);
        }
        break;
    case SDP_CMD_SET_DMA_BUF:
        {
			RCW_DEF(SDP_CTRL_REG);
			RCW_DEF(SDP_DMA_ADDR_REG) = {0};
			RCW_DEF(SDP_DMA_SIZE_REG) = {0};

            if (p_param->p_data == NULL) {
                DBG_ERR("p_data NULL, skip\r\n");
                return E_PAR;
            }
            if (((UINT32)p_param->p_data) & 0x03) {
                //DBG_ERR("p_data not word aligned: 0x%x\r\n", p_param->p_data);
                return E_PAR;
            }
            if (p_param->size > SDP_DMA_LEN_MAX) {
                //DBG_ERR("input size 0x%x > max DMA size 0x%x\r\n",
                //        p_param->size, SDP_DMA_LEN_MAX);
                return E_PAR;
            }

            DBG_ERR("sync dram, pAddr:0x%x, size: %x \n", (UINT32)p_param->p_data, p_param->size);
            // sync DRAM with cache
            vos_cpu_dcache_sync((VOS_ADDR)p_param->p_data, p_param->size, VOS_DMA_TO_DEVICE_NON_ALIGN);
//            dma_flushWriteCache((UINT32)p_param->p_data, p_param->size);

            loc_cpu();

			RCW_LD(SDP_CTRL_REG);
			if (RCW_OF(SDP_CTRL_REG).dma_en) {
                unl_cpu();

                DBG_ERR("DMA mode still running, skip\r\n");
                return E_QOVR;
            }

            //
            // start dma
            //
			RCW_OF(SDP_DMA_SIZE_REG).buf_size = p_param->size;
			RCW_ST(SDP_DMA_SIZE_REG);
			RCW_OF(SDP_DMA_ADDR_REG).addr = (UINT32)p_param->p_data;
			RCW_ST(SDP_DMA_ADDR_REG);
			RCW_OF(SDP_CTRL_REG).dma_en = 1;
			RCW_ST(SDP_CTRL_REG);

            unl_cpu();
        }
        break;
	case SDP_CMD_ABORT_DMA:
		{
			RCW_DEF(SDP_CTRL_REG);

            loc_cpu();

			RCW_LD(SDP_CTRL_REG);
			RCW_OF(SDP_CTRL_REG).dma_en = 0;
			RCW_ST(SDP_CTRL_REG);

			unl_cpu();
DBG_ERR("%s: before wait DMA\r\n", __func__);
#if 1
			while (1) {
				RCW_LD(SDP_CTRL_REG);
				if (RCW_OF(SDP_CTRL_REG).dma_en == 0) break;
			}
#endif
DBG_ERR("%s: after wait DMA\r\n", __func__);

        }
		break;
    default:
        //DBG_ERR("command 0x%x not supported\r\n", p_param->command);
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
    if (id >= SDP_ID_COUNT) {
		return E_ID;
	}

    switch (param_id) {
    case SDP_PARAM_ID_EVENT_CALLBACK:
		DBG_WRN("%s: SDP CB 0x%x, ref 0x%x\r\n", __func__, (unsigned int)p_param , (unsigned int)(*(DRV_CB*)p_param));
        loc_cpu();
        v_sdp_event_handler[id] = p_param;
        unl_cpu();
        break;
	case SDP_PARAM_ID_TRANSFER_MODE:
		{
			SDP_TRANSFER_MODE *p_mode = (SDP_TRANSFER_MODE*)p_param;
			RCW_DEF(SDP_CTRL_REG);

			loc_cpu();
	        v_sdp_transfer_mode[id] = *p_mode;
			RCW_LD(SDP_CTRL_REG);
			RCW_OF(SDP_CTRL_REG).transfer_mode = v_sdp_transfer_mode[id];
			RCW_ST(SDP_CTRL_REG);
	        unl_cpu();
		}
		break;
    default:
        return E_NOSPT;
    }

    return E_OK;
}

/**
    Reset SDP controller

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER sdp_reset(void)
{
	RCW_DEF(SDP_RSTN_REG);

    _REGIOBASE -= (UINT32)0x370000;
	RCW_LD(SDP_RSTN_REG);
    RCW_OF(SDP_RSTN_REG).rstn = 0;
    RCW_ST(SDP_RSTN_REG);

    RCW_OF(SDP_RSTN_REG).rstn = 1;
    RCW_ST(SDP_RSTN_REG);
    _REGIOBASE += (UINT32)0x370000;
    //pr_info("SDP controller reset.");

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
				/*DBG_ERR("SDP%d: %s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
						ctrl_idx + 1,
						v_spi_reg_default[i].p_name,
						v_spi_reg_default[i].offset,
						INW(v_base_addr[ctrl_idx] + v_spi_reg_default[i].offset),
						v_spi_reg_default[i].value);*/
				b_return = FALSE;
			}
		}
	}

	return b_return;
}


#endif

static void sdp_kick_ist(SDP_ID id, UINT32 event)
{
    loc_cpu();
    v_ist_event[id] |= event;
    unl_cpu();
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


static int sdp_ist(void)
{
	THREAD_ENTRY();

    DBG_WRN("\n");

	sdp_ist_en = 1;
    //coverity[no_escape]
	while (1) {
		FLGPTN ptn = 0;
		UINT32 flg = v_flg_ptn[SDP_ID_1];

		vos_flag_wait(&ptn, FLG_ID_SDP, flg | FLG_ID_SDP_END, TWF_ORW | TWF_CLR);

		if (ptn & v_flg_ptn[SDP_ID_1]) {
			UINT32 event;

			loc_cpu();
			event = v_ist_event[SDP_ID_1];
			v_ist_event[SDP_ID_1] &= ~event;
			unl_cpu();

			sdp_drv_do_tasklet(SDP_ID_1, event);
		}

		if (ptn & FLG_ID_SDP_END) {
			pr_info("ist close.\r\n");
            sdp_ist_en = 0;
            THREAD_RETURN(0);
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
    DBG_WRN("\n");

    cre_flg(&FLG_ID_SDP, NULL, "sdp_flag");
    SEM_CREATE(SEMID_SDP, 1);
    DBG_WRN("SEMID: %d\n", SEMID_SDP);
    v_semaphore[0] = SEMID_SDP;

    v_ist_tsk_id[SDP_ID_1] = vos_task_create(sdp_ist,  0, "SdpIst",   5,  4096);
    vos_task_resume(v_ist_tsk_id[SDP_ID_1]);

//	request_irq(INT_ID_SDP, sdp_isr, IRQF_TRIGGER_HIGH, "SDP", 0);
//	request_irq_bh(INT_ID_SDP, (irq_bh_handler_t) sdp_bh_ist, IRQF_BH_PRI_HIGH);
}

void sdp_platform_uninit(void)
{
    //free_irq(INT_ID_SDP, NULL);
    //vos_task_destroy(v_ist_tsk_id[SDP_ID_1]);
	DBG_WRN("Wait sdp_ist close: \n");
	while (sdp_ist_en){
		iset_flg(FLG_ID_SDP, FLG_ID_SDP_END);
		msleep(10);
	}
    v_ist_tsk_id[SDP_ID_1] = 0;

    rel_flg(FLG_ID_SDP);
    SEM_DESTROY(SEMID_SDP);
}

