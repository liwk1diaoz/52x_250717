#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <asm/io.h>

#include <linux/bits.h>

#include "comm/gyro_spi.h"
#include "gyro_spi_drv.h"
#include "gyro_spi_dbg.h"

#if (NVT_GYRO_528_PINMUX == ENABLE || NVT_GYRO_530_PINMUX == ENABLE)
#include <rcw_macro.h>
#endif


/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
irqreturn_t nvt_gyro_spi_drv_isr(int irq, void *devid);
void nvt_gyro_spi_linux_act_handler(SPI_LINUX_ACT spiAct, void *param);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
struct clk *nvt_gyro_spi_clk = NULL;
static SPI_ID gTestSpiID = SPI_ID_3;
static volatile BOOL bSpiGyroDtRdy = FALSE;
static volatile UINT32 uiSpiGyroRunState = 0;   // 0: idle, 1: run, 2: stop, 0x8000: buffer over run, 0x8001: seq err
static UINT32 vRecvBuf[2][32 * 2];


/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int nvt_gyro_spi_drv_open(GYRO_MODULE_INFO *pmodule_info, unsigned char ucIF)
{
	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_gyro_spi_drv_release(GYRO_MODULE_INFO *pmodule_info, unsigned char ucIF)
{
	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

#if (NVT_GYRO_528_PINMUX == ENABLE)
void nvt_gyro_spi_drv_pinmux_setting(void)
{
    void __iomem *reg_addr;
    UINT32 tmp_reg;

    // set TOP reg
    reg_addr = ioremap_nocache(TOP_REG_ADDR, 0x200);

    // disable SDIO
    tmp_reg = INW(reg_addr + 0x04);
    tmp_reg &= ~(BIT(14));
    OUTW(reg_addr + 0x04, tmp_reg);

    // SPI3_1: 0x14[13:12] = 0x1, SPI3_2: 0x14[13:12] = 0x2
    // 1-bit full duplex: 0x14[18] = 1
    tmp_reg = INW(reg_addr + 0x14);
    tmp_reg &= ~(BIT(13) | BIT(12));
    tmp_reg |= (NVT_GYRO_528_PINMUX_SEL << 12);
    tmp_reg |= BIT(18);
    OUTW(reg_addr + 0x14, tmp_reg);

    #if (NVT_GYRO_528_PINMUX_SEL == 1)
    // set P_GPIO17 ~ P_GPIO20 to normal functionality
    tmp_reg = INW(reg_addr + 0xA8);
    tmp_reg &= ~(BIT(20) | BIT(19) | BIT(18) | BIT(17));
    OUTW(reg_addr + 0xA8, tmp_reg);
    #elif (NVT_GYRO_528_PINMUX_SEL == 2)
    // set C_GPIO11 ~ C_GPIO14 to normal functionality
    tmp_reg = INW(reg_addr + 0xA0);
    tmp_reg &= ~(BIT(14) | BIT(13) | BIT(12) | BIT(11));
    OUTW(reg_addr + 0xA0, tmp_reg);
    #else
    #error, NVT_GYRO_528_PINMUX_SEL unexcepted value
    #endif

    DBG_IND("top 0x04: 0x%08X, 0x14: 0x%08X, 0xA0: 0x%08X, 0xA8: 0x%08X\n",
            INW(reg_addr + 0x04), INW(reg_addr + 0x14),
            INW(reg_addr + 0xA0), INW(reg_addr + 0xA8));

    iounmap(reg_addr);

    #if (NVT_GYRO_528_PINMUX_SEL == 2)
    // set PAD reg
    reg_addr = ioremap_nocache(PAD_REG_ADDR, 0x200);

    // PAD_MC_1_REGULATOR_EN = 1
    tmp_reg = INW(reg_addr + 0x74);
    tmp_reg |= BIT(4);
    OUTW(reg_addr + 0x74, tmp_reg);

    iounmap(reg_addr);
    #endif
}
#elif (NVT_GYRO_530_PINMUX == ENABLE)
void nvt_gyro_spi_drv_pinmux_setting(void)
{
    void __iomem *reg_addr;
    UINT32 tmp_reg;

    // set TOP reg
    reg_addr = ioremap_nocache(TOP_REG_ADDR, 0x200);

    // check pinmux conflict
    #if (NVT_GYRO_530_PINMUX_SEL == 1)
    tmp_reg = INW(reg_addr + 0x34);
    if(((tmp_reg >> 12) & 0xF) == 0x1) { 
        DBG_ERR("Conflict: Make sure disable UART4_1\r\n");
    }

    tmp_reg = INW(reg_addr + 0x10);
    if(((tmp_reg >> 24) & 0xF) == 0x1) { 
        DBG_ERR("Conflict: Make sure disable I2C7_1\r\n");
    }

    tmp_reg = INW(reg_addr + 0x10);
    if(((tmp_reg >> 28) & 0xF) == 0x1) {
        DBG_ERR("Conflict: Make sure disable I2C8_1\r\n");
    }

    tmp_reg = INW(reg_addr + 0x48);
    if(((tmp_reg >> 8) & 0xF) == 0x1) {
        DBG_ERR("Conflict: Make sure disable SIF2_1\r\n");
    }

    tmp_reg = INW(reg_addr + 0x1C);
    if(((tmp_reg >> 16) & 0xF) == 0x1) {
        DBG_ERR("Conflict: Make sure disable PICNT\r\n");
    }
    
    tmp_reg = INW(reg_addr + 0x1C);
    if(((tmp_reg >> 20) & 0xF) == 0x1) {
        DBG_ERR("Conflict: Make sure disable PICNT2\r\n");
    }
    
    tmp_reg = INW(reg_addr + 0x1C);
    if(((tmp_reg >> 24) & 0xF) == 0x1) {
        DBG_ERR("Conflict: Make sure disable PICNT3\r\n");
    }

    tmp_reg = INW(reg_addr + 0x28);
    if(((tmp_reg >> 16) & 0x3) == 0x2) {
        DBG_ERR("Conflict: Make sure disable SN5_MCLK=2\r\n");
    }

    tmp_reg = INW(reg_addr + 0x50);
    if(((tmp_reg >> 19) & 0x1) == 0x1) {
        DBG_ERR("Conflict: Make sure disable RTC_DIV_OUT\r\n");
    }

    tmp_reg = INW(reg_addr + 0x50);
    if(((tmp_reg >> 18) & 0x1) == 0x1) {
        DBG_ERR("Conflict: Make sure disable RTC_EXT_CLK\r\n");
    }

    #elif (NVT_GYRO_530_PINMUX_SEL == 2)
    tmp_reg = INW(reg_addr + 0x04);
    if(((tmp_reg >> 8) & 0x1) == 0x1) {
        DBG_ERR("Conflict: Make sure disable SDIO2\r\n");
    }

    tmp_reg = INW(reg_addr + 0x34);
    if(((tmp_reg >> 28) & 0xF) == 0x2) {
        DBG_ERR("Conflict: Make sure disable UART8_2\r\n");
    }

    tmp_reg = INW(reg_addr + 0x30);
    if(((tmp_reg >> 0) & 0x3) == 0x2) {
        DBG_ERR("Conflict: Make sure disable I2S=2\r\n");
    }

    tmp_reg = INW(reg_addr + 0x20);
    if(((tmp_reg >> 8) & 0x1) == 0x1) {
        DBG_ERR("Conflict: Make sure disable DSP_EJTAG\r\n");
    }

    tmp_reg = INW(reg_addr + 0x40);
    if(((tmp_reg >> 16) & 0x3) == 0x2) {
        DBG_ERR("Conflict: Make sure disable SDP=2\r\n");
    }
    #endif

    DBG_IND("top 0x44: 0x%08X, 0xA0: 0x%08X, 0xA8: 0x%08X\n",
            INW(reg_addr + 0x44), INW(reg_addr + 0xA0), INW(reg_addr + 0xA8));
    iounmap(reg_addr);

}
#endif

int nvt_gyro_spi_drv_init(GYRO_MODULE_INFO *pmodule_info)
{
	int iRet = 0;

	init_waitqueue_head(&pmodule_info->gyro_wait_queue);

	sema_init(&pmodule_info->gyro_sem, 1);

	/* initial clock here */
	clk_prepare(pmodule_info->pclk[0]);
	clk_enable(pmodule_info->pclk[0]);
    
	nvt_gyro_spi_clk = pmodule_info->pclk[0];
    //for test usage: decrease clock rate for 687 gpio emulate gyro device
    //clk_set_rate(pmodule_info->pclk[0], 375000);

	/* register IRQ here*/
	if(request_irq(pmodule_info->intr_id[0], nvt_gyro_spi_drv_isr, IRQF_TRIGGER_HIGH, "GYRO_INT", pmodule_info)) {
		DBG_ERR("failed to register an IRQ Int:%d\n", pmodule_info->intr_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
    }

	/* Add HW Module initialization here when driver loaded */
    spi_platform_init(nvt_gyro_spi_linux_act_handler);


    spi_setRegAddr(SPI_ID_3, pmodule_info->io_addr[0]);

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->intr_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}

int nvt_gyro_spi_drv_remove(GYRO_MODULE_INFO *pmodule_info)
{
    spi_platform_uninit();

	//Free IRQ
	free_irq(pmodule_info->intr_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	clk_disable(pmodule_info->pclk[0]);
	clk_unprepare(pmodule_info->pclk[0]);

    // TODO: implement gyro.c to reset SPI3 register
    //gyro_reset();

	return 0;
}

int nvt_gyro_spi_drv_suspend(GYRO_MODULE_INFO *pmodule_info)
{
	/* Add suspend operation here*/

	return 0;
}

int nvt_gyro_spi_drv_resume(GYRO_MODULE_INFO *pmodule_info)
{
	/* Add resume operation here*/

	return 0;
}

irqreturn_t nvt_gyro_spi_drv_isr(int irq, void *devid)
{
    spi3_isr();
    return IRQ_HANDLED;
}

void nvt_gyro_spi_linux_act_handler(SPI_LINUX_ACT spi_act, void *param)
{
    switch (spi_act) {
        case SPI_LINUX_ACT_CLK_SET:
            {
                UINT32 clk_rate = *(UINT32 *)param;
                if (nvt_gyro_spi_clk) {
                    clk_set_rate(nvt_gyro_spi_clk, clk_rate);
                }
            }
            break;
        default:
            break;
    }
}


static void emu_gyroHdl(SPI_GYRO_INT uiIntSts)
{
	if (uiIntSts & SPI_GYRO_INT_SYNC_END) {
		bSpiGyroDtRdy = TRUE;
	}

	if (uiIntSts & SPI_GYRO_INT_CHANGE_END) {
		bSpiGyroDtRdy = TRUE;
	}


	if (uiIntSts & SPI_GYRO_INT_SEQ_ERR) {
		uiSpiGyroRunState = 0x8001;
		bSpiGyroDtRdy = FALSE; //2022/06/17
		//DBG_IND("Seq ERR\n");
	}

	if (uiIntSts & SPI_GYRO_INT_OVERRUN) {
		bSpiGyroDtRdy = FALSE; //2022/06/17
		uiSpiGyroRunState = 0x8002;
		//DBG_IND("FIFO overrun\n");
	}

	if (uiIntSts & SPI_GYRO_INT_TRS_TIMEOUT) {
		uiSpiGyroRunState = 0x8003;
		bSpiGyroDtRdy = FALSE; //2022/06/17
		//DBG_IND("Transfer TIMEOUT\n");
	}

	if (uiIntSts & SPI_GYRO_INT_QUEUE_THRESHOLD) {
        //DBG_IND("Queue exceed threshold\n");
		uiSpiGyroRunState = 0x8004;
		bSpiGyroDtRdy = FALSE; //2022/06/17
	}

    if (uiIntSts & SPI_GYRO_INT_QUEUE_OVERRUN) {
        //DBG_IND("Queue overrun\n");
		uiSpiGyroRunState = 0x8005;
		bSpiGyroDtRdy = FALSE; //2022/06/17
	}
}

static int selection = 1;
void emu_gyroAuto(UINT32 cur_clk_rate)
{
	UINT32 i;
	UINT32 uiIntMask = 0;
	UINT32 uiStartIdx = 0;
	SPI_GYRO_INFO gyroInfo = {0};
    DBG_ERR("START GYRO AUTO!!!! ensure that you have connect to slave device\n");

    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_BUSMODE, SPI_MODE_0);
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_MSB_LSB, SPI_MSB);
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_WIDE_BUS_ORDER, SPI_WIDE_BUS_ORDER_NORMAL);
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_CS_CK_DLY, 1);
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_ENG_GYRO_UNIT, TRUE);   // enable engineer emulation mode
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_CS_ACT_LEVEL, SPI_CS_ACT_LEVEL_LOW);
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET, 0);
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_PKT_DLY, 0);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_VD_SRC, SPI_VD_SRC_SIE1); //528 only has SIE1/2/4/5

    // just notify clock rate to gyro host layer
    // gyro host layer calculate timing parameter (e.x. op_interval, transfer_interval)
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_FREQ, cur_clk_rate);

	if(selection == 0) {
		gyroInfo.gyroMode = SPI_GYRO_MODE_ONE_SHOT;
	} else if(selection == 1) {
		gyroInfo.gyroMode = SPI_GYRO_MODE_FREE_RUN;
	} else if(selection == 2) {
		gyroInfo.gyroMode = SPI_GYRO_MODE_SIE_SYNC;
	}

	gyroInfo.uiTransferCount = 32;
	gyroInfo.uiTransferLen = 4;         // 4 OP per transfer

	gyroInfo.uiOpInterval = 0x8;
	gyroInfo.uiTransferInterval = 0x800;

	gyroInfo.uiOp0Length = 8;
	gyroInfo.vOp0OutData[0] = 0x9F;
	for (i = 1; i < gyroInfo.uiOp0Length; i++) {
		gyroInfo.vOp0OutData[i] = 0;
	}

	gyroInfo.uiOp1Length = 8;
	gyroInfo.vOp1OutData[0] = 0x03;
	gyroInfo.vOp1OutData[1] = 0x00;
	gyroInfo.vOp1OutData[2] = 0x00;
	gyroInfo.vOp1OutData[3] = 0x00;
	for (i = 4; i < gyroInfo.uiOp1Length; i++) {
		gyroInfo.vOp1OutData[i] = 0;
	}

	gyroInfo.uiOp2Length = 8;
	gyroInfo.vOp2OutData[0] = 0x04;
	gyroInfo.vOp2OutData[1] = 0x00;
	gyroInfo.vOp2OutData[2] = 0x00;
	gyroInfo.vOp2OutData[3] = 0x00;
	for (i = 4; i < gyroInfo.uiOp2Length; i++) {
		gyroInfo.vOp2OutData[i] = 0;
	}

	gyroInfo.uiOp3Length = 8;
	gyroInfo.vOp3OutData[0] = 0x05;
	gyroInfo.vOp3OutData[1] = 0x00;
	gyroInfo.vOp3OutData[2] = 0x00;
	gyroInfo.vOp3OutData[3] = 0x00;
	for (i = 4; i < gyroInfo.uiOp3Length; i++) {
		gyroInfo.vOp3OutData[i] = 0;
	}

	for (i = 0; i < gyroInfo.uiTransferCount; i++) {
		uiIntMask |= 1 << i;
	}
    DBG_IND("OP intEn will be 0x%x\n", uiIntMask);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_ENG_GYRO_INTMSK, uiIntMask);

	gyroInfo.pEventHandler = emu_gyroHdl;

	spi_open(gTestSpiID);

	uiSpiGyroRunState = 1;
	bSpiGyroDtRdy = FALSE;
	spi_startGyro(gTestSpiID, &gyroInfo);
	while (1) {
		if ((uiSpiGyroRunState == 2) && (bSpiGyroDtRdy == FALSE)) {
			uiSpiGyroRunState = 1;
		}

		if (uiSpiGyroRunState == 0x8001) {
            DBG_IND("Seq ERR\n");
			break;
		}

		if (uiSpiGyroRunState == 0x8002) {
            DBG_IND("FIFO overrun ERR\n");
			break;
		}
		
		if (uiSpiGyroRunState == 0x8003) {
            DBG_IND("Transfer timeout ERR\n");
			break;
		}
		
		if (uiSpiGyroRunState == 0x8004) {
            DBG_IND("Queue execeed threshold ERR\n");
			break;
		}

		if (uiSpiGyroRunState == 0x8005) {
            DBG_IND("Queue overrun ERR\n");
			break;
		}

		if (bSpiGyroDtRdy == TRUE) {
			const UINT32 uiQueueDepth = spi_getGyroQueueCount(gTestSpiID);

			bSpiGyroDtRdy = FALSE;

			for (i = 0; i < uiQueueDepth; i++)
			{
				GYRO_BUF_QUEUE gyroData;
				UINT32 uiNextWord;

				spi_getGyroData(gTestSpiID, &gyroData);
				uiNextWord = (uiStartIdx & 0xFF) |
							 (((uiStartIdx + 1) << 8) & 0xFF00) |
							 (((uiStartIdx + 2) << 16) & 0xFF0000) |
							 (((uiStartIdx + 3) << 24) & 0xFF000000UL);
				vRecvBuf[0][i * 2] = gyroData.vRecvWord[0];
				if (uiNextWord != vRecvBuf[0][i * 2]) {
                    DBG_IND("Expect 0x%x, but 0x%x\n", uiNextWord, vRecvBuf[0][i * 2]);
				}
				uiStartIdx += 4;

				uiNextWord = (uiStartIdx & 0xFF) |
							 (((uiStartIdx + 1) << 8) & 0xFF00) |
							 (((uiStartIdx + 2) << 16) & 0xFF0000) |
							 (((uiStartIdx + 3) << 24) & 0xFF000000UL);
				vRecvBuf[0][i * 2 + 1] = gyroData.vRecvWord[1];
				if (uiNextWord != vRecvBuf[0][i * 2 + 1]) {
                    DBG_IND("Expect 0x%x, but 0x%x\n", uiNextWord, vRecvBuf[0][i * 2 + 1]);
				}
				uiStartIdx += 4;
			}

            DBG_IND(".");
		}
	}

	spi_stopGyro(gTestSpiID);
	if (uiSpiGyroRunState == 0x8001) {
        DBG_IND("Seq ERR\n");
	}
    DBG_IND("%s: exit\n", __func__);
	uiSpiGyroRunState = 0;

	spi_close(gTestSpiID);
}

//test
#if 0
void emu_spi3Wire(UINT32 cur_clk_rate)
{
	UINT32 uiOutLen;
	UINT32 uiInLen;
	UINT8 *pOutBuf;
	UINT8 *pInBuf;
	UINT32 i;
	UINT32 uiStartInIdx;
	UINT32 uiPktDelay;
	BOOL bDmaMode;
	const SPI_MODE spiMode = SPI_MODE_0;
	const UINT32 uiOutBufSize = 4 * 1024;   // 4 KB
	const UINT32 uiInBufSize = 4 * 1024;    // 4 KB

	pOutBuf = kmalloc(uiOutBufSize, GFP_KERNEL);
	pInBuf = kmalloc(uiInBufSize, GFP_KERNEL);

	for (i = 0; i < uiOutBufSize; i++) {
		pOutBuf[i] = i;
	}
	memset(pInBuf, 0, uiInBufSize);

    uiOutLen = 256;
    uiInLen = 256;
	bDmaMode = TRUE;
	uiPktDelay = 0;

	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_DO_HZ_EN, TRUE);

	DBG_IND("\nStart test...\n");

    // just notify clock rate to gyro host layer
    // gyro host layer calculate timing parameter (e.x. op_interval, transfer_interval)
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_FREQ, cur_clk_rate);

	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_BUSMODE, spiMode);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_MSB_LSB, SPI_MSB);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_CS_ACT_LEVEL, SPI_CS_ACT_LEVEL_LOW);

	spi_open(gTestSpiID);

	spi_setBusWidth(gTestSpiID, SPI_BUS_WIDTH_HD_1BIT);

	// 1. Test PIO first
	spi_setCSActive(gTestSpiID, TRUE);

	spi_setTransferLen(gTestSpiID, SPI_TRANSFER_LEN_2BYTES);
	spi_writeSingle(gTestSpiID, uiOutLen);
	spi_setTransferLen(gTestSpiID, SPI_TRANSFER_LEN_1BYTE);

	spiTest_setPktDelay(gTestSpiID, uiPktDelay);
	if (uiOutLen / 4) {
		spi_writeReadData(gTestSpiID, uiOutLen / 4, (UINT32 *)pOutBuf, NULL, bDmaMode);
	}
	if (uiOutLen % 4) {
		UINT32 uiOffset = (uiOutLen / 4) * 4;

		for (i = 0; i < (uiOutLen % 4); i++) {
			spi_writeSingle(gTestSpiID, pOutBuf[uiOffset + i]);
		}
	}

	if (uiInLen / 4) {
		spi_writeReadData(gTestSpiID, uiInLen / 4, NULL, (UINT32 *)pInBuf, bDmaMode);
	}
	if (uiInLen % 4) {
		UINT32 uiOffset = (uiInLen / 4) * 4;

		for (i = 0; i < (uiInLen % 4); i++) {
			UINT32 uiRecvWord;

			spi_readSingle(gTestSpiID, &uiRecvWord);
			pInBuf[uiOffset + i] = uiRecvWord;
		}
	}

	spi_setCSActive(gTestSpiID, FALSE);

	uiStartInIdx = 0;
	for (i = 0; i < uiInLen; i++, uiStartInIdx++) {
		if (pInBuf[i] != (uiStartInIdx & 0xFF)) {
			DBG_IND("Index 0x%x fail, expect 0x%x, but 0x%hhx\n", i, uiStartInIdx & 0xFF, pInBuf[i]);
			DBG_IND("Input buffer base 0x%p, fail addr 0x%p\n", pInBuf, &pInBuf[i]);
			break;
		}
	}

	spi_setBusWidth(gTestSpiID, SPI_BUS_WIDTH_1_BIT);
	spi_close(gTestSpiID);

    DBG_IND("test finish\n");
}

//test
void pmc_read_id_new(UINT32 cur_clk_rate)
{
	UINT32 uiId1 = 0, uiId2 = 0;
	UINT32  uiManufactureID, uiDeviceID1, uiDeviceID2;

    // just notify clock rate to gyro host layer
    // gyro host layer calculate timing parameter (e.x. op_interval, transfer_interval)
    spi_setConfig(gTestSpiID, SPI_CONFIG_ID_FREQ, cur_clk_rate);

	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_BUSMODE, SPI_MODE_0);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_MSB_LSB, SPI_MSB);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_WIDE_BUS_ORDER, SPI_WIDE_BUS_ORDER_NORMAL);
	spi_setConfig(gTestSpiID, SPI_CONFIG_ID_CS_ACT_LEVEL, SPI_CS_ACT_LEVEL_LOW);

	spi_open(gTestSpiID);

	spi_setCSActive(gTestSpiID, TRUE);
	spi_setTransferLen(gTestSpiID, SPI_TRANSFER_LEN_2BYTES);
	spi_writeReadSingle(gTestSpiID, 0x9FFF, &uiId1);
	spi_writeReadSingle(gTestSpiID, 0xFFFF, &uiId2);
	spi_setCSActive(gTestSpiID, FALSE);

	spi_close(gTestSpiID);

	uiManufactureID = uiId1 & 0xFF;
	uiDeviceID1 = (uiId2 >> 8) & 0xFF;
	uiDeviceID2 = uiId2 & 0xFF;

	DBG_IND("PMC> ManufactureID = 0x%02X\n", (unsigned int)uiManufactureID);
	DBG_IND("PMC> DeviceID1 = 0x%02X\n", (unsigned int)uiDeviceID1);
	DBG_IND("PMC> DeviceID2 = 0x%02X\n", (unsigned int)uiDeviceID2);
}
#endif
