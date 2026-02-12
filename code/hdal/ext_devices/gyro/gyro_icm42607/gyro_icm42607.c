/*
    Gyro Driver InvenSense icm42607.

    @file       gyro_icm42607.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

#include "kwrap/error_no.h"
#include "kwrap/util.h"
#include "comm/gyro_drv.h"

#if !defined(__LINUX)
#include <stdio.h>
#include "string.h"
#else
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#endif
#include <kwrap/perf.h>
#include "kwrap/flag.h"
#include "comm/hwclock.h"

#define CHK_FRAME    (0)
#define WORKAROUND   (0)

#if CHK_FRAME
static UINT32 b_latency =0;
#endif
#define Delay_DelayMs(ms) vos_util_delay_ms(ms)
#define Delay_DelayUs(us) vos_util_delay_us_polling(us)
INT32 gyro_readReg(UINT32 uiAddr, UINT32 * puiData);
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          gyro_drv
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////
// gyro auto calibate on gyro_open
#define AUTO_GYRO_CALIBRATION   0
#define PERF_OPEN               0
typedef enum
{
    GYRO_IDLE   = 0,        ///< idle
    GYRO_READY  = 1,        ///< ready for read/write
    GYRO_RUN    = 2,        ///< running
    ENUM_DUMMY4WORD(GYRO_STATUS)
} GYRO_STATUS;

#define FLG_ID_GYRO_DATA_IN        FLGPTN_BIT(1)
#define FLG_ID_GYRO_STOP           FLGPTN_BIT(2)
#define FLG_ID_GYRO_ALL            0xFFFFFFFF
static ID FLG_ID_GYRO = 0;
static GYRO_STATUS g_GyroStatus = GYRO_IDLE;
static GYRO_OP_MODE g_GyroOpMode = GYRO_OP_MODE_SINGLE_ACCESS;
static GYRO_FREE_RUN_PARAM  gyroFreeRunParm={0};
static UINT32 interval = 0;
static SPI_ID g_TestSpiID = SPI_ID_3;
const static UINT32 g_uiReadFrequency  = 24000000; // 1 MHz for read
const static UINT32 g_uiWriteFrequency = 24000000; // 1 MHz for write
static INT32  g_xOffset = 0;
static INT32  g_yOffset = 0;
static INT32  g_zOffset = 0;
static INT32  g_axOffset = 0;
static INT32  g_ayOffset = 0;
static INT32  g_azOffset = 0;
//static void (*pGyroCB)(void) = NULL;
static FP_GYRO_CB pGyroCB = NULL;
static GYRO_CFGINFO g_GyroCfgInfo = {{ 0, //RSC_GYRO_AXIS_X;
                                    1, //RSC_GYRO_AXIS_Y;
                                    2}, //RSC_GYRO_AXIS_Z;
                                    {0, //RSC_DIR_POS
                                    0, //RSC_DIR_POS
                                    0}};//RSC_DIR_POS

#define GDI2_GB_NUM (96)
static UINT32 puiGyroBuf[GDI2_GB_NUM][5]={0};
static UINT32 puiAccelBuf[GDI2_GB_NUM][5]={0};
static UINT32 ulTimeStamp[GDI2_GB_NUM]={0};
static UINT32 uiGyroBufInIdx = 0, uiGyroBufOutIdx = 0, uiGyroQueueCount = 0;
static UINT32 uiAccBufOutIdx = 0;
#define UNINITED_1ST_TS  0xFFFFFFFF
static UINT32 timestamp_1st = UNINITED_1ST_TS;

#if PERF_OPEN
VOS_TICK tigger_s,tigger_e=0;
#endif
#define REG_DEV_CFG                 (0x01)
#define REG_ACCEL_XOUT_H            (0x0B)
#define REG_ACCEL_XOUT_L            (0x0C)
#define REG_ACCEL_YOUT_H            (0x0D)
#define REG_ACCEL_YOUT_L            (0x0E)
#define REG_ACCEL_ZOUT_H            (0x0F)
#define REG_ACCEL_ZOUT_L            (0x10)

//#define REG_GYRO_CONFIG             (0x1B)
#define REG_GYRO_XOUT_H             (0x11)
#define REG_GYRO_XOUT_L             (0x12)
#define REG_GYRO_YOUT_H             (0x13)
#define REG_GYRO_YOUT_L             (0x14)
#define REG_GYRO_ZOUT_H             (0x15)
#define REG_GYRO_ZOUT_L             (0x16)

//#define REG_PWR_MGMT_1              (0x6B)
//#define REG_PWR_MGMT_2              (0x6C)
#define REG_WHO_AM_I                (0x75)
#define REG_WRITE                   (0x00)
#define REG_READ                    (0x80)

#define REG_DELAY                   (0xFF)
#define REG_DELAYU                  (0xFE)

#define GYRO_ID1                     (0x68)  //icm42607T
#define GYRO_ID2                     (0x67)
#define GYRO_ID3                     (0x60)
#define GYRO_ID4                     (0x51)

#define INT_MSG_CNT                 (300)

typedef struct
{
    UINT32  uiAddress;                      // Panel Register Address
    UINT32  uiValue;                        // Panel Register config value
}tGYRO_CMD;

/*
MREG1 write access, BLK_SEL_W must be set to 0x00.
MREG2 write access, BLK_SEL_W must be set to 0x28.
MREG3 write access, BLK_SEL_W must be set to 0x50.
*/

/* BLK_SEL_W must be set to 0
�E MADDR_W must be set to 0x14 (address of the MREG1 register being accessed)
�E M_W must be set to the desired value
�E Wait for 10 �gs
*/
tGYRO_CMD cmd_gyro_init[]=
{
    {0x02,    0x10},      // reset (SIGNAL_PATH_RESET,SOFT_RESET_DEVICE_CONFIG)
    {0xFF,    0x0A},      // delay 10 ms
    {0x1F,    0x10},      // PWR_MGMT0 in IDLE
    {0xFF,    0x0A},      // delay 10 ms

    //OTP reload start
    {0x79,    0x00},      // BLK_SEL_W,MREG1
    {0x7A,    0x2B},      // MADDR_W,0x2B is OTP_CONFIG
    {0x7B,    0x06},      // M_W,value is 6 OTP_COPY_MODE(11: Enable copying self-test data from OTP memory to SRAM)
    {0xFE,    0x0A},      // delay 10 us
    {0x79,    0x00},

    {0x79,    0x28},      // BLK_SEL_W,MREG2
    {0x7A,    0x06},      // MADDR_W,0x06 is OTP_CTRL7
    {0x7B,    0x04},      // M_W,value is 4 OTP_RELOAD (Setting this bit to 1 triggers OTP copy operation.)
    {0xFE,    0x0A},      // delay 10 us
    {0x79,    0x00},

    {0xFE,    0xFF},      // delay 300 us

    {0x79,    0x28},      // BLK_SEL_W,MREG2
    {0x7A,    0x06},      // MADDR_W,0x06 is OTP_CTRL7
    {0x7B,    0x0C},      // M_W,value is C (initial reset value is 0x0C, it changes to 0x06 after OTP load completes)
    {0xFE,    0x0A},      // delay 10 us
    {0x79,    0x00},

    {0xFE,    0xFF},      // delay 280 us
    //OTP reload end

    {0x1F,    0x10},       // PWR_MGMT0 in IDLE
    {0xFF,    0x0A},       // delay 10ms
    {0x06,    0x03},       //int1 pulse, pp, active H  (INT_CONFIG,1: Push pull,1: Active high)
    {0x20,    0x65},       //gyro fsr +-1000 dps, odr 1600 Hz (GYRO_CONFIG0,0010: ��1000 dps,0110: 1600 Hz)
    {0x21,    0x65},       //acc fsr +-2 G, odr 1600 Hz (ACCEL_CONFIG0,11: ��2g,0110: 1600 Hz (LN mode))
    {0x23,    0x31},       //gyro bw 180Hz (GYRO_CONFIG1,001: 180 Hz)
    {0x24,    0x01},       //acc avg 2x, bw 180Hz (ACCEL_CONFIG1,000: 2x average,001: 180 Hz)

    {0x2B,    0x08},       //drdy int1 enable
    //{0x39,    0x00},       //bit0, read drdy status
    {0xFE,    0x0A},      // delay 10 us
    {0x1F,    0x00},       //PWR_MGMT0,Turns gyroscope off,Turns accelerometer off
    {0x1F,    0x0F},       //gyro on/acc on (gyroscope in Low Noise (LN) Mode,accelerometer in Low Noise (LN) Mode)
    {0xFF,    0x0A},       //sleep 100ms->10ms
    {0x01,    0x04},       //set spi 4 wire


};

#if AUTO_GYRO_CALIBRATION
static BOOL Gyro_Calibration(void)
{
    INT32    GyroX,GyroY,GyroZ;
    INT32    i, SampleCount = 100, Retry = 3;
    BOOL     result = FALSE;
    UINT32   gyroInitZeroTolerance = 10;
    INT32    tempX,tempY,tempZ;
    INT32    ZeroOffsetMax, ZeroOffsetMin;

    GyroX =0;
    GyroY =0;
    GyroZ =0;
    ZeroOffsetMax = gyro_getLSB() * gyroInitZeroTolerance;
    ZeroOffsetMin = -ZeroOffsetMax;
    DBG_IND("ZeroOffsetMax=%d, ZeroOffsetMin=%d\r\n",ZeroOffsetMax,ZeroOffsetMin);
    gyro_setCalZeroOffset(0,0,0);
    while (Retry)
    {
        for (i=0;i<SampleCount;i++)
        {
            gyro_getSingleData(&tempX, &tempY, &tempZ);
            if (tempX > ZeroOffsetMax || tempX < ZeroOffsetMin || tempY > ZeroOffsetMax || tempY < ZeroOffsetMin || tempZ > ZeroOffsetMax || tempZ < ZeroOffsetMin)
            {
                DBG_ERR("tempX=%5d, tempY=%5d, tempZ=%5d\r\n",tempX,tempY,tempZ);
                Retry--;
                Delay_DelayMs(100);
                break;
            }
            GyroX+=tempX;
            GyroY+=tempY;
            GyroZ+=tempZ;
            DBG_IND("tempX=%5d, tempY=%5d, tempZ=%5d\r\n",tempX,tempY,tempZ);
            Delay_DelayMs(4);
        }
        if (i >= SampleCount)
        {
            result = TRUE;
            Retry = 0;
        }
    }
    GyroX/=SampleCount;
    GyroY/=SampleCount;
    GyroZ/=SampleCount;
    if (result == TRUE)
    {
        gyro_setCalZeroOffset(-GyroX,-GyroY,-GyroZ);
    }
    DBG_IND("GyroX=%5d, GyroY=%5d, GyroZ=%5d\r\n",GyroX,GyroY,GyroZ);
    return TRUE;
}
#endif

void gyro_cfg(GYRO_CFGINFO *cfgInfo)
{
    g_GyroCfgInfo = *cfgInfo;
    DBG_IND("Axis= %d, %d, %d\r\n",g_GyroCfgInfo.AxisSelec[0],g_GyroCfgInfo.AxisSelec[1],g_GyroCfgInfo.AxisSelec[2]);
    DBG_IND("Dir= %d, %d, %d\r\n",g_GyroCfgInfo.DirSelec[0],g_GyroCfgInfo.DirSelec[1],g_GyroCfgInfo.DirSelec[2]);
}
#if 0
INT32 gyro_open(GYRO_OPENOBJ *openObj)
{
    DBG_IND("openObj =0x%x!!\r\n", (unsigned int)openObj);
    // parameter check
    if (openObj && openObj->FP_GYRO_CB)
    {
        pGyroCB = openObj->FP_GYRO_CB;
    }

#else
INT32 gyro_open(void)
{
#endif
    UINT32 uiRecv1 = 0;
    #if PERF_OPEN
    vos_perf_mark(&tigger_s);
    #endif
    // state check: only while idle
    if(g_GyroStatus!=GYRO_IDLE)
    {
        //DBG_WRN("already opened , g_GyroStatus=%d!!\r\n", g_GyroStatus);
        return E_OK;
    }
    uiGyroBufOutIdx = 0;
    uiGyroBufInIdx = 0;
    uiAccBufOutIdx= 0;
	timestamp_1st = UNINITED_1ST_TS;

    // power on gyro
    // always power-on

    // initializa gyro
    {
        UINT32 uiRecv1 = 0;

        spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_ENG_GYRO_UNIT, FALSE);  // normal mode

        spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_FREQ, g_uiReadFrequency);
        DBG_IND("Frequency %d \r\n",g_uiReadFrequency);
        spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_BUSMODE, SPI_MODE_3);
        spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_MSB_LSB, SPI_MSB);
        spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_CS_ACT_LEVEL, SPI_CS_ACT_LEVEL_LOW);
        spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_CS_CK_DLY, 1);

        // Read gyro ID
        spi_open(g_TestSpiID);
        spi_setCSActive(g_TestSpiID, TRUE);
        spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_2BYTES);
        spi_writeReadSingle(g_TestSpiID, (REG_READ|REG_WHO_AM_I)<<8, &uiRecv1);
        spi_setCSActive(g_TestSpiID, FALSE);
        spi_close(g_TestSpiID);

        if (((uiRecv1&0xFF) != GYRO_ID1)&&((uiRecv1&0xFF) != GYRO_ID2) &&
            ((uiRecv1&0xFF) != GYRO_ID3)&&((uiRecv1&0xFF) != GYRO_ID4) )
        {
            DBG_ERR("gyroid = 0x%x\r\n",uiRecv1);
            return E_SYS;
        }
        #if WORKAROUND
        DBG_IND("GYRO_ID 0x%x 4wire\r\n",uiRecv1);
        #else
        DBG_IND("GYRO_ID 0x%x \r\n",uiRecv1);
        #endif
    }
    // set initial value
    {
        UINT32 i;
        spi_open(g_TestSpiID);
        spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_1BYTE);

        for (i=0;i<sizeof(cmd_gyro_init)/sizeof(tGYRO_CMD);i++)
        {
            if (REG_DELAY == cmd_gyro_init[i].uiAddress)
            {
                Delay_DelayMs(cmd_gyro_init[i].uiValue);
                continue;
            }
            if (REG_DELAYU == cmd_gyro_init[i].uiAddress)
            {
                Delay_DelayUs(cmd_gyro_init[i].uiValue);
                continue;
            }
            spi_setCSActive(g_TestSpiID, TRUE);
            spi_writeSingle(g_TestSpiID, REG_WRITE|(cmd_gyro_init[i].uiAddress&0x7f));
            spi_writeSingle(g_TestSpiID, cmd_gyro_init[i].uiValue);
            spi_setCSActive(g_TestSpiID, FALSE);
        }
        spi_close(g_TestSpiID);
    }

    // state change: ready for gyro
    g_GyroStatus = GYRO_READY;

    gyro_readReg(REG_READ|REG_DEV_CFG,&uiRecv1);
    DBG_IND("REG_DEV_CFG 0x%x \r\n",uiRecv1);
    gyro_readReg(REG_READ|0x39,&uiRecv1);
    DBG_IND("INT_STATUS_DRDY 0x%x \r\n",uiRecv1);

    // Do gyro calibration
    #if AUTO_GYRO_CALIBRATION
    Gyro_Calibration();
    #endif
    #if PERF_OPEN
    vos_perf_mark(&tigger_e);
    DBG_DUMP("%d %d \r\n",tigger_e-tigger_s,tigger_e);
    #endif

    OS_CONFIG_FLAG(FLG_ID_GYRO);
    vos_flag_clr(FLG_ID_GYRO, FLG_ID_GYRO_ALL); //clear all flag

    return E_OK;
}


INT32 gyro_close(void)
{

    DBG_IND("\r\n");
    // state check: only while idle
    if(g_GyroOpMode==GYRO_OP_MODE_FREE_RUN)
    {
        // disable free run
        spi_stopGyro(g_TestSpiID);
        spi_close(g_TestSpiID);

        g_GyroOpMode = GYRO_OP_MODE_SINGLE_ACCESS;
        g_GyroStatus = GYRO_READY;
    }
    #if 0
    if(g_GyroStatus==GYRO_RUN)
    {
        DBG_ERR("GYRO ERR: gyro_close, g_GyroStatus=%d!!\r\n", g_GyroStatus);
        return E_SYS;
    }
    #endif

    // stop/pause gyro
    // not-yet

    // power off gyro
    // always power-on

    // state change: ready for gyro
    g_GyroStatus = GYRO_IDLE;
    vos_flag_set(FLG_ID_GYRO, FLG_ID_GYRO_STOP); //send STOP cmd

	rel_flg(FLG_ID_GYRO);

    return E_OK;
}

INT32 gyro_shutdown(void)
{
    return E_OK;
}

INT32 gyro_readReg(UINT32 uiAddr, UINT32 * puiData)
{
    // state check: only while ready
    if(g_GyroStatus!=GYRO_READY)
    {
        DBG_ERR("GYRO ERR: gyro_readReg, g_GyroStatus=%d!!\r\n", g_GyroStatus);
        return E_SYS;
    }

    // state change: ready -> run
    g_GyroStatus = GYRO_RUN;

    // read register
    //normal job
    {
        UINT32 uiRecv1 = 0;//, uiRecv2;
        spi_open(g_TestSpiID);

        spi_setCSActive(g_TestSpiID, TRUE);
        spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_1BYTE);
        spi_writeSingle(g_TestSpiID, (REG_READ)|(uiAddr&0x7f));
        spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_2BYTES);
        spi_writeReadSingle(g_TestSpiID, 0x00, &uiRecv1);
        spi_setCSActive(g_TestSpiID, FALSE);

        spi_close(g_TestSpiID);

        *puiData = (uiRecv1&0xFFFF);
    }

    // state change: run -> ready
    g_GyroStatus = GYRO_READY;

    return E_OK;
}

INT32 gyro_writeReg(UINT32 uiAddr, UINT32 uiData)
{
    // state check: only while ready
    if(g_GyroStatus!=GYRO_READY)
    {
        DBG_ERR("GYRO ERR: gyro_readReg, g_GyroStatus=%d!!\r\n", g_GyroStatus);
        return E_SYS;
    }

    // state change: ready -> run
    g_GyroStatus = GYRO_RUN;

    // write register
    {
    UINT32 uiRecv1;//, uiRecv2;

    spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_FREQ, g_uiWriteFrequency);

    spi_open(g_TestSpiID);

    spi_setCSActive(g_TestSpiID, TRUE);
    spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_1BYTE);
    spi_writeSingle(g_TestSpiID, (REG_WRITE)|(uiAddr&0x7f));
    spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_2BYTES);
    spi_writeReadSingle(g_TestSpiID, uiData, &uiRecv1);
    spi_setCSActive(g_TestSpiID, FALSE);

    spi_close(g_TestSpiID);

    spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_FREQ, g_uiReadFrequency);

    }

    // state change: run -> ready
    g_GyroStatus = GYRO_READY;

    return E_OK;
}


INT32 gyro_setFreeRunParam(GYRO_FREE_RUN_PARAM *frParam)
{
    // state check: only while idle
    //not-yet

    //g_uiGyroFrPeriod = frParam->uiPeriodUs;
    //g_uiGyroFrNumber = frParam->uiDataNum;

    if (!frParam)
    {
        DBG_ERR("frParam is NULL\r\n");
        return E_PAR;
    }
    if (frParam->uiDataNum > GDI2_GB_NUM)
    {
        DBG_ERR("uiDataNum over limit %d\r\n",GDI2_GB_NUM);
        return E_PAR;
    }
    if (frParam->uiTriggerIdx >= frParam->uiDataNum)
    {
        DBG_ERR("uiTriggerIdx %d, uiDataNum  %d\r\n",frParam->uiTriggerIdx,frParam->uiDataNum);
        return E_PAR;
    }
    DBG_IND("uiPeriodUs %d, uiDataNum %d, triggerMode %d, triggerIdx %d\r\n", frParam->uiPeriodUs, frParam->uiDataNum, frParam->triggerMode,frParam->uiTriggerIdx);
    gyroFreeRunParm.uiPeriodUs = frParam->uiPeriodUs;
    gyroFreeRunParm.uiDataNum = frParam->uiDataNum;
    gyroFreeRunParm.uiTriggerIdx = frParam->uiTriggerIdx;
    gyroFreeRunParm.triggerMode = frParam->triggerMode;


    return E_OK;
}

static void gyro_isr(SPI_GYRO_INT gyroInt)
{
    INT Status=0;
    UINT32 i=0, j=0, k=0;
    GYRO_BUF_QUEUE gyroData = {0};
	UINT32 err_cnt = 0;
    UINT64 timestamp = 0;

    if (gyroInt & SPI_GYRO_INT_SYNC_END) {
        timestamp = hwclock_get_longcounter();
        j = uiGyroBufInIdx;
        if (uiGyroBufInIdx == 0)
        {
            //uiGyroQueueCount = spi_getGyroQueueCount(g_TestSpiID);
            uiGyroQueueCount = ((gyroFreeRunParm.uiDataNum - gyroFreeRunParm.uiTriggerIdx) << 1);
        }
        else
        {
            uiGyroQueueCount = (gyroFreeRunParm.uiDataNum << 1);
        }
        for (i=0; i<uiGyroQueueCount; i++)
        {
            Status = spi_getGyroData(g_TestSpiID, &gyroData);
			if(Status!=0) {
				//DBG_ERR("Status %d\r\n",Status);
				err_cnt++;
			}
            k = j%GDI2_GB_NUM;
#if WORKAROUND
            if (i%2 ==1)
#else
            if (i%2 ==0)
#endif
            {
                puiGyroBuf[k][0] = gyroData.uiFrameID;
                puiGyroBuf[k][1] = gyroData.uiDataID;

                //  x =  byte 1 | byte2 (byte 0 is address, we ignore it)
                puiGyroBuf[k][2] = (gyroData.vRecvWord[0]&0xFF00) | ((gyroData.vRecvWord[0]>>16)&0xFF);
                //  y =  byte 3 | byte4
                puiGyroBuf[k][3] = ((gyroData.vRecvWord[0]>>16)&0xFF00) | (gyroData.vRecvWord[1]&0xFF);
                //  z =  byte 5 | byte6
                puiGyroBuf[k][4] = (gyroData.vRecvWord[1]&0xFF00) | ((gyroData.vRecvWord[1]>>16)&0xFF);
                ulTimeStamp[k] = timestamp;

            }
            else
            {
                puiAccelBuf[k][0] = gyroData.uiFrameID;
                puiAccelBuf[k][1] = gyroData.uiDataID;

                //  x =  byte 1 | byte2 (byte 0 is address, we ignore it)
                puiAccelBuf[k][2] = (gyroData.vRecvWord[0]&0xFF00) | ((gyroData.vRecvWord[0]>>16)&0xFF);
                //  y =  byte 3 | byte4
                puiAccelBuf[k][3] = ((gyroData.vRecvWord[0]>>16)&0xFF00) | (gyroData.vRecvWord[1]&0xFF);
                //  z =  byte 5 | byte6
                puiAccelBuf[k][4] = (gyroData.vRecvWord[1]&0xFF00) | ((gyroData.vRecvWord[1]>>16)&0xFF);

                //DBG_IND("k = %d, gsX = %d, gsY = %d, gsZ = %d\r\n",k,puiAccelBuf[k][2],puiAccelBuf[k][2],puiAccelBuf[k][4]);
                j++;
            }

        }
		if (err_cnt) {
			DBG_ERR("que %d,err %d\r\n", uiGyroQueueCount, err_cnt);
		}
        uiGyroBufInIdx += gyroFreeRunParm.uiDataNum;

        #if 0
        DBG_IND("Sync End = %d ms, uiGyroQueueCount = %d\r\n",clock()/1000,uiGyroQueueCount);
        #else
        DBG_IND("Sync End, uiGyroQueueCount = %d\r\n",uiGyroQueueCount);
        #endif
        if(pGyroCB!=NULL)
        {
            pGyroCB();
        }

        vos_flag_iset(FLG_ID_GYRO, FLG_ID_GYRO_DATA_IN);


    } else if (gyroInt & SPI_GYRO_INT_OVERRUN) {
        static UINT32 overrun_err_cnt =0;
        overrun_err_cnt++;
        if(overrun_err_cnt>=INT_MSG_CNT) {
            DBG_ERR("OVERRUN %d !!\r\n",overrun_err_cnt);
            overrun_err_cnt = 0;
        }
    } else if (gyroInt & SPI_GYRO_INT_SEQ_ERR) {
		#if 0
        DBG_ERR("SEQ_ERR!! %d ms, count=%d\r\n",clock()/1000,spi_getGyroQueueCount(g_TestSpiID));
		#else
        //DBG_ERR("SEQ_ERR!! count=%d\r\n",spi_getGyroQueueCount(g_TestSpiID));
		#endif
    } else if (gyroInt & SPI_GYRO_INT_QUEUE_THRESHOLD) {
        DBG_ERR("QUEUE_THRESHOLD!!\r\n");
    } else if (gyroInt & SPI_GYRO_INT_QUEUE_OVERRUN) {
        DBG_ERR("QUEUE_OVERRUN!!\r\n");
    }else if (gyroInt & SPI_GYRO_INT_LAST_TRS) {
        //DBG_IND("LAST_TRS!!\r\n");
    }
    else {
        DBG_ERR("%d\r\n",gyroInt);
    }

}

static INT32 gyro_setFreeRun(void)
{
    UINT32 i;
    SPI_GYRO_INFO gyroInfo= {0};

    //
    // 1. Prepare data buffer to store polled gyroscope data
    //
    //useless in new driver//uiDumpCount = 0;
    //useless in new driver//pNextBuf = (UINT32*)uiDumpBufAddr;

    //
    // 2. Start SPI gyro polling
    //
    //gyroInfo.gyroMode = SPI_GYRO_MODE_SIE_SYNC;

    if (gyroFreeRunParm.triggerMode == GYRO_FREERUN_SIE_SYNC)
        gyroInfo.gyroMode = SPI_GYRO_MODE_SIE_SYNC;
    else
        gyroInfo.gyroMode = SPI_GYRO_MODE_FREE_RUN;

    gyroInfo.uiTransferLen = 2;
    gyroInfo.uiTransferCount = gyroFreeRunParm.uiDataNum;
    gyroInfo.uiOpInterval       = 1;
     // reserve 200 us for last transfer
    gyroInfo.uiTransferInterval = (gyroFreeRunParm.uiPeriodUs-200)/gyroFreeRunParm.uiDataNum;
    interval = gyroInfo.uiTransferInterval;
    gyroInfo.uiOp0Length = 7;
    DBG_IND("TLen = %d TCount= %d  OpInt =%d, TInt= %d Op0Len=%d\r\n",gyroInfo.uiTransferLen,gyroInfo.uiTransferCount,gyroInfo.uiOpInterval,gyroInfo.uiTransferInterval,gyroInfo.uiOp0Length);
    #if WORKAROUND
    gyroInfo.vOp0OutData[0] = REG_WRITE|(REG_DEV_CFG&0x7f);
    gyroInfo.vOp0OutData[1] = REG_WRITE|(0x04&0x7f);  //4wire mode
    for (i=2; i<gyroInfo.uiOp0Length; i++)
    {
        gyroInfo.vOp0OutData[i] = 0;
    }
    gyroInfo.uiOp1Length = 7;
    gyroInfo.vOp1OutData[0] = REG_READ|REG_GYRO_XOUT_H;
    #else
    gyroInfo.vOp0OutData[0] = REG_READ|REG_GYRO_XOUT_H;
    for (i=1; i<gyroInfo.uiOp0Length; i++)
    {
        gyroInfo.vOp0OutData[i] = 0;
    }
    gyroInfo.uiOp1Length = 7;
    gyroInfo.vOp1OutData[0] = REG_READ|REG_ACCEL_XOUT_H;
    #endif
    for (i=1; i<gyroInfo.uiOp1Length; i++)
    {
        gyroInfo.vOp1OutData[i] = 0;
    }
    gyroInfo.pEventHandler = gyro_isr;

    spi_open(g_TestSpiID);

    spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET, gyroFreeRunParm.uiTriggerIdx);

    spi_setTransferLen(g_TestSpiID, SPI_TRANSFER_LEN_1BYTE);

    spi_startGyro(g_TestSpiID, &gyroInfo);

    return E_OK;
}

UINT32 gyro_getLSB(void)
{
    return 131;
}


void gyro_RSCInfoInit(GYRO_RSCINFOINIT *pRSCInfoInit)
{
    // please refer to IPL_Ctrl_RSC_Int.h file for the GDFmt/FDDir definition
    pRSCInfoInit->gdFormat = 0;//_INT32
    pRSCInfoInit->AxisSelec[0] = g_GyroCfgInfo.AxisSelec[0];
    pRSCInfoInit->AxisSelec[1] = g_GyroCfgInfo.AxisSelec[1];
    pRSCInfoInit->AxisSelec[2] = g_GyroCfgInfo.AxisSelec[2];
    pRSCInfoInit->DirSelec[0] = g_GyroCfgInfo.DirSelec[0];
    pRSCInfoInit->DirSelec[1] = g_GyroCfgInfo.DirSelec[1];
    pRSCInfoInit->DirSelec[2] = g_GyroCfgInfo.DirSelec[2];
    pRSCInfoInit->uiDataUnit = (65536/gyro_getLSB());
    pRSCInfoInit->uiCalGain[0] = 1024;
    pRSCInfoInit->uiCalGain[1] = 1024;
    pRSCInfoInit->uiCalGain[2] = 1024;
    pRSCInfoInit->iCalOfs[0] = 0;
    pRSCInfoInit->iCalOfs[1] = 0;
    pRSCInfoInit->iCalOfs[2] = 0;
}


INT32 gyro_setMode(GYRO_OP_MODE opMode)
{
    DBG_IND("opMode %d\r\n", opMode);

    if(g_GyroStatus==GYRO_IDLE)
    {
        //DBG_WRN("Gyro is not opened\r\n");
        return E_SYS;
    }
    // state check: only while idle
    if((g_GyroStatus!=GYRO_READY) && (g_GyroOpMode!=GYRO_OP_MODE_FREE_RUN))
    {
        DBG_ERR("gyro_setMode, g_GyroOpMode = %d, g_GyroStatus=%d!!\r\n", g_GyroOpMode, g_GyroStatus);
        return E_SYS;
    }

    if(opMode==GYRO_OP_MODE_SINGLE_ACCESS)
    {
        if(g_GyroOpMode==GYRO_OP_MODE_SINGLE_ACCESS)
        {
            //DBG_WRN("gyro_setMode, already single access mode!!\r\n");
            return E_OK;
        }
        else if(g_GyroOpMode==GYRO_OP_MODE_FREE_RUN)
        {
            // disable free run
            spi_stopGyro(g_TestSpiID);
            spi_close(g_TestSpiID);

            g_GyroOpMode = GYRO_OP_MODE_SINGLE_ACCESS;
            g_GyroStatus = GYRO_READY;
            uiGyroBufInIdx = 0;
            uiGyroBufOutIdx = 0;
            uiAccBufOutIdx = 0;
			timestamp_1st = UNINITED_1ST_TS;
        }
        else
        {
            DBG_ERR("gyro_setMode, g_GyroOpMode=%d!!\r\n", opMode);
            return E_SYS;
        }
    }
    else if(opMode==GYRO_OP_MODE_FREE_RUN)
    {
        if(g_GyroOpMode==GYRO_OP_MODE_FREE_RUN)
        {
            //DBG_WRN("gyro_setMode, already free run mode!!\r\n");
            return E_OK;
        }
        else if(g_GyroOpMode==GYRO_OP_MODE_SINGLE_ACCESS)
        {
            if(gyroFreeRunParm.uiPeriodUs==0)
            {
                DBG_ERR("gyro_setMode, run mode param is not set!!\r\n");
                return E_SYS;
            }
            // enable free run
            gyro_setFreeRun();

            g_GyroOpMode = GYRO_OP_MODE_FREE_RUN;
            g_GyroStatus = GYRO_RUN;
        }
        else
        {
            DBG_ERR("gyro_setMode, g_GyroOpMode=%d!!\r\n", opMode);
            return E_SYS;
        }
    }
    else
    {
        DBG_ERR("gyro_setMode, opMode=%d!!\r\n", opMode);
        return E_SYS;
    }

    return E_OK;
}
static void gyro_AdjustDataOffset(INT32* gx, INT32* gy, INT32* gz)
{
    INT32  tempX,tempY,tempZ;

    //DBG_IND("gx=%d, gy=%d, g_xOffset = %d, g_yOffset = %d\r\n",*gx,*gy,g_xOffset,g_yOffset);
    tempX = (INT16)*gx;
    if (tempX + g_xOffset > 32767)
        tempX = 32767;
    else if (tempX + g_xOffset < -32768)
        tempX = -32768;
    else
        tempX +=g_xOffset;

    tempY = (INT16)*gy;
    if (tempY + g_yOffset > 32767)
        tempY = 32767;
    else if (tempY + g_yOffset < -32768)
        tempY = -32768;
    else
        tempY +=g_yOffset;

    tempZ = (INT16)*gz;
    if (tempZ + g_zOffset > 32767)
        tempZ = 32767;
    else if (tempZ + g_zOffset < -32768)
        tempZ = -32768;
    else
        tempZ +=g_zOffset;

    *gx = tempX;
    *gy = tempY;
    *gz = tempZ;

    //DBG_IND("gx=%d, gy=%d gz=%d\r\n",*gx,*gy,*gz);
}

static void gyro_AdjustAccDataOffset(INT32* gx, INT32* gy, INT32* gz)
{
    INT32  tempX,tempY,tempZ;

    //DBG_IND("ax=%d, ay=%d, g_axOffset = %d, g_ayOffset = %d\r\n",*gx,*gy,g_axOffset,g_ayOffset);
    tempX = (INT16)*gx;
    if (tempX + g_axOffset > 32767)
        tempX = 32767;
    else if (tempX + g_axOffset < -32768)
        tempX = -32768;
    else
        tempX +=g_axOffset;

    tempY = (INT16)*gy;
    if (tempY + g_ayOffset > 32767)
        tempY = 32767;
    else if (tempY + g_ayOffset < -32768)
        tempY = -32768;
    else
        tempY +=g_ayOffset;

    tempZ = (INT16)*gz;
    if (tempZ + g_azOffset > 32767)
        tempZ = 32767;
    else if (tempZ + g_azOffset < -32768)
        tempZ = -32768;
    else
        tempZ +=g_azOffset;

    *gx = tempX;
    *gy = tempY;
    *gz = tempZ;

    //DBG_IND("ax=%d, ay=%d az=%d\r\n",*gx,*gy,*gz);
}
INT32 gyro_getSingleData(INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    if (!puiDatX)
    {
        DBG_ERR("puiDatX is NULL\r\n");
        return E_PAR;
    }
    if (!puiDatY)
    {
        DBG_ERR("puiDatY is NULL\r\n");
        return E_PAR;
    }
    if (!puiDatZ)
    {
        DBG_ERR("puiDatZ is NULL\r\n");
        return E_PAR;
    }
    gyro_readReg(REG_GYRO_XOUT_H,(UINT32*)puiDatX);
    gyro_readReg(REG_GYRO_YOUT_H,(UINT32*)puiDatY);
    gyro_readReg(REG_GYRO_ZOUT_H,(UINT32*)puiDatZ);
    gyro_AdjustDataOffset(puiDatX, puiDatY, puiDatZ);

    return E_OK;
}

/**
	used to get gyro data.
    sie would call gyro_getFreeRunData in vd crop end,expect get *puiNum data

	@param puiNum: input buffer of gyro number.Output real gyro data number
	               input 0 mean sie get buffer fail,gyro doesn't copy gyro data,but gyro data should drop for next frame sync
	@param pulTs: input frame vd timestamp.output timestamp of each gyro
	@param puiDatX: output gyro x data,should check if be null pointer
	@param puiDatY: output gyro y data,should check if be pointer
	@param puiDatZ: output gyro z data,should check if be pointer
	@return
		- @b  HD_OK: Success.
		- @b  < 0:   Some error happened.
 */
INT32 gyro_getFreeRunData(UINT32 *puiNum, UINT32 *pulTs,INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    UINT32 i, j;
    UINT32 uiDataNum = gyroFreeRunParm.uiDataNum;
	FLGPTN flag = 0;
	ER wai_rt;
    UINT64 timestamp = 0;

    // state check: only while run
    UINT32 timeout=gyroFreeRunParm.uiPeriodUs/2;

    if(pulTs) {
        timestamp = *pulTs;
        //DBG_DUMP("%lld\r\n",timestamp);
    }

    if(g_GyroStatus!=GYRO_RUN)
    {
        DBG_ERR("gyro_getGyroData, g_GyroStatus=%d!!\r\n", g_GyroStatus);
        return E_SYS;
    }
    // op mode check: only while free run mode
    if(g_GyroOpMode!=GYRO_OP_MODE_FREE_RUN)
    {
        DBG_ERR("gyro_getGyroData, not in free run mode %d !!\r\n", g_GyroOpMode);
        return E_SYS;
    }

    if (timestamp_1st == UNINITED_1ST_TS) {
		timestamp_1st = timestamp;
    }
    if (timestamp != timestamp_1st) {   //not check 1st frame for sensor crop,latency 1
        wai_rt = vos_flag_wait_timeout(&flag, FLG_ID_GYRO, FLG_ID_GYRO_DATA_IN | FLG_ID_GYRO_STOP, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout));
        if (wai_rt != E_OK) {
        	DBG_DUMP("wai%lld %d %d %d %d\r\n", timestamp,wai_rt,timeout,uiGyroBufInIdx,uiGyroBufOutIdx);
        }
    }
    #if 0  //for debug
    else {
        //first time, check data in ,wait 1 frame
	    FLGPTN flgptn;
        UINT64 start_ts=0,end_ts = 0;

        flgptn = vos_flag_chk(FLG_ID_GYRO, FLG_ID_GYRO_DATA_IN);
        // not data in ,wait 1 vd
	    if (!(flgptn&FLG_ID_GYRO_DATA_IN)) {
            start_ts = hwclock_get_longcounter();
            wai_rt = vos_flag_wait_timeout(&flag, FLG_ID_GYRO, FLG_ID_GYRO_DATA_IN | FLG_ID_GYRO_STOP, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(gyroFreeRunParm.uiPeriodUs));
            end_ts = hwclock_get_longcounter();
            if (wai_rt != E_OK) {
           	    DBG_DUMP("wai%lld %d %lld %d %d\r\n", timestamp,wai_rt,end_ts-start_ts,uiGyroBufInIdx,uiGyroBufOutIdx);
            } else {
           	    DBG_DUMP("wai%lld %d %lld %d %d\r\n", timestamp,wai_rt,end_ts-start_ts,uiGyroBufInIdx,uiGyroBufOutIdx);
            }
	    }
    }
    #endif

    #if CHK_FRAME
    else {
        b_latency = 0;
    }
    #endif
    // normal job
    if((uiGyroBufOutIdx+uiDataNum)==uiGyroBufInIdx)
    {
        j = uiGyroBufOutIdx;
        for (i=0; i<uiDataNum; i++)
        {
            UINT32 k;
            k = j%GDI2_GB_NUM;
			if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL && pulTs != NULL) {
				*(puiDatX+i) = puiGyroBuf[k][2];
				*(puiDatY+i) = puiGyroBuf[k][3];
				*(puiDatZ+i) = puiGyroBuf[k][4];
				gyro_AdjustDataOffset(puiDatX+i,puiDatY+i,puiDatZ+i);
                *(pulTs+i) = ulTimeStamp[k]-interval*(uiDataNum-i-1);
			}
            j++;
            if (i==0)
            {
                //DBG_DUMP("%d\r\n",*(pulTs+i));
                DBG_IND("Count = %d,gyro x=%5d, y=%5d, z=%5d\r\n",uiGyroQueueCount>>1,*puiDatX,*puiDatY,*puiDatZ);
                DBG_IND("accel x=%5d, y=%5d, z=%5d\r\n",puiAccelBuf[k][2],puiAccelBuf[k][3],puiAccelBuf[k][4]);
            }
        }

        uiGyroBufOutIdx = j;
        *puiNum = (uiGyroQueueCount >> 1);

    }
    else if((uiGyroBufOutIdx+uiDataNum)>uiGyroBufInIdx)
    {
        DBG_DUMP("ft%lld,%d,%d\r\n",timestamp,uiGyroBufInIdx,uiGyroBufOutIdx);
		if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL && pulTs != NULL) {
			memset((void *)puiDatX, 0, uiDataNum*sizeof(UINT32));
			memset((void *)puiDatY, 0, uiDataNum*sizeof(UINT32));
			memset((void *)puiDatZ, 0, uiDataNum*sizeof(UINT32));
		}
        *puiNum = 0;
#if CHK_FRAME
        if(timestamp==timestamp_1st) {
            b_latency = 1;
        }
#endif
        return E_CTX;
    }
    else if((uiGyroBufOutIdx+uiDataNum)<uiGyroBufInIdx)
    {
        //DBG_DUMP("pt%lld,%d,%d\r\n",timestamp,uiGyroBufInIdx,uiGyroBufOutIdx);
        if(uiGyroBufOutIdx > uiGyroBufInIdx-GDI2_GB_NUM ) //past data still in queue
        {
            j = uiGyroBufOutIdx;
            for (i=0; i<uiDataNum; i++)
            {
                UINT32 k;
                k = j%GDI2_GB_NUM;
				if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL && pulTs != NULL) {
					*(puiDatX+i) = puiGyroBuf[k][2];
					*(puiDatY+i) = puiGyroBuf[k][3];
					*(puiDatZ+i) = puiGyroBuf[k][4];
					gyro_AdjustDataOffset(puiDatX+i,puiDatY+i,puiDatZ+i);
                    *(pulTs+i) = ulTimeStamp[k]-interval*(uiDataNum-i-1);
				}
                j++;
            }
            uiGyroBufOutIdx = j;
            *puiNum = (uiGyroQueueCount >> 1);
        } else {
            DBG_DUMP("pt%lld,%d,%d\r\n",timestamp,uiGyroBufInIdx,uiGyroBufOutIdx);
            if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL) {
	            memset((void *)puiDatX, 0, uiDataNum*sizeof(UINT32));
	            memset((void *)puiDatY, 0, uiDataNum*sizeof(UINT32));
	            memset((void *)puiDatZ, 0, uiDataNum*sizeof(UINT32));
			}
            //*puiNum = uiDataNum;
            *puiNum = (uiGyroQueueCount >> 1);
            uiGyroBufOutIdx =  uiGyroBufOutIdx+(*puiNum);
            return E_QOVR;
        }

    }

#if CHK_FRAME
    if((timestamp - timestamp_1st + 1 - b_latency)!=(uiGyroBufOutIdx/uiDataNum)){
        DBG_DUMP("@%lld,%d\r\n",(timestamp - timestamp_1st + 1 - b_latency),(uiGyroBufOutIdx/uiDataNum));
    }
#endif
    //DBG_DUMP("%lld %lld\r\n",*pulTs,timestamp);
    g_GyroStatus = GYRO_RUN;

    return E_OK;
}


/**
	used to get acc data.
    it would be call next to gyro_getFreeRunData, gyro and acc should be the same timestamp

	@param puiNum: Output real gyro add number
	@param puiDatX: output acc x data,should check if be null pointer
	@param puiDatY: output acc y data,should check if be pointer
	@param puiDatZ: output acc z data,should check if be pointer
	@return
		- @b  HD_OK: Success.
		- @b  < 0:   Some error happened.
 */
INT32 gyro_getFreeRunGsData(UINT32 *puiNum, INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    UINT32 i, j;
    UINT32 uiDataNum = gyroFreeRunParm.uiDataNum;
    // state check: only while run
    if(g_GyroStatus!=GYRO_RUN)
    {
        DBG_ERR("gyro_getGyroData, g_GyroStatus=%d!!\r\n", g_GyroStatus);
        return E_SYS;
    }

    // op mode check: only while free run mode
    if(g_GyroOpMode!=GYRO_OP_MODE_FREE_RUN)
    {
        DBG_ERR("gyro_getGyroData, not in free run mode %d !!\r\n", g_GyroOpMode);
        return E_SYS;
    }
    // normal job
    if((uiAccBufOutIdx+uiDataNum)==uiGyroBufInIdx)
    {
        j = uiAccBufOutIdx;
        for (i=0; i<uiDataNum; i++)
        {
            UINT32 k;
            k = j%GDI2_GB_NUM;
			if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL) {
				*(puiDatX+i) = puiAccelBuf[k][2];
				*(puiDatY+i) = puiAccelBuf[k][3];
				*(puiDatZ+i) = puiAccelBuf[k][4];
				gyro_AdjustAccDataOffset(puiDatX+i,puiDatY+i,puiDatZ+i);
			}
            j++;
            if (i==0)
            {
                DBG_IND("Count = %d,gyro x=%5d, y=%5d, z=%5d\r\n",uiGyroQueueCount>>1,puiGyroBuf[k][2],puiGyroBuf[k][3],puiGyroBuf[k][4]);
                DBG_IND("accel x=%5d, y=%5d, z=%5d\r\n",*puiDatX,*puiDatY,*puiDatZ);
            }
        }

        uiAccBufOutIdx = j;
        *puiNum = (uiGyroQueueCount >> 1);

    }
    else if((uiAccBufOutIdx+uiDataNum)>uiGyroBufInIdx)
    {
        //asking for future data
        DBG_ERR("ft %d %d\r\n",uiAccBufOutIdx,uiGyroBufInIdx);
		if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL) {
			memset((void *)puiDatX, 0, uiDataNum*sizeof(UINT32));
			memset((void *)puiDatY, 0, uiDataNum*sizeof(UINT32));
			memset((void *)puiDatZ, 0, uiDataNum*sizeof(UINT32));
		}
        *puiNum = 0;
        return E_CTX;
    }
    else if((uiAccBufOutIdx+uiDataNum)<uiGyroBufInIdx)
    {
        //DBG_ERR("asking for past data!!\r\n");
        if(uiGyroBufOutIdx > uiGyroBufInIdx-GDI2_GB_NUM ) //past data still in queue
        {
            j = uiAccBufOutIdx;
            for (i=0; i<uiDataNum; i++)
            {
                UINT32 k;
                k = j%GDI2_GB_NUM;
				if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL) {
					*(puiDatX+i) = puiAccelBuf[k][2];
					*(puiDatY+i) = puiAccelBuf[k][3];
					*(puiDatZ+i) = puiAccelBuf[k][4];
					gyro_AdjustAccDataOffset(puiDatX+i,puiDatY+i,puiDatZ+i);
				}
                j++;
            }

            uiAccBufOutIdx = j;
            *puiNum = (uiGyroQueueCount >> 1);

        } else {
            DBG_ERR("pt %d %d\r\n",uiAccBufOutIdx,uiGyroBufInIdx);
			if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL) {
				memset((void *)puiDatX, 0, uiDataNum*sizeof(UINT32));
				memset((void *)puiDatY, 0, uiDataNum*sizeof(UINT32));
				memset((void *)puiDatZ, 0, uiDataNum*sizeof(UINT32));
			}
            //*puiNum = uiDataNum;
            *puiNum = (uiGyroQueueCount >> 1);
            uiAccBufOutIdx = uiAccBufOutIdx+(*puiNum);
            return E_QOVR;
        }
    }

    // state change: run -> run
    g_GyroStatus = GYRO_RUN;

    return E_OK;
}
void gyro_setCalZeroOffset(INT32 xOffset, INT32 yOffset, INT32 zOffset)
{
    g_xOffset = xOffset;
    g_yOffset = yOffset;
    g_zOffset = zOffset;
    DBG_IND("xOffset = %d, yOffset = %d, zOffset = %d\r\n",xOffset,yOffset,zOffset);
}
void gyro_setAccCalZeroOffset(INT32 xOffset, INT32 yOffset, INT32 zOffset)
{
    g_axOffset = xOffset;
    g_ayOffset = yOffset;
    g_azOffset = zOffset;
    DBG_IND("axOffset = %d, ayOffset = %d, azOffset = %d\r\n",xOffset,yOffset,zOffset);
}
INT32 gyro_getGsData(INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    if (!puiDatX)
    {
        DBG_ERR("puiDatX is NULL\r\n");
        return E_PAR;
    }
    if (!puiDatY)
    {
        DBG_ERR("puiDatY is NULL\r\n");
        return E_PAR;
    }
    if (!puiDatZ)
    {
        DBG_ERR("puiDatZ is NULL\r\n");
        return E_PAR;
    }
    if(g_GyroOpMode==GYRO_OP_MODE_SINGLE_ACCESS)
    {
        gyro_readReg(REG_ACCEL_XOUT_H,(UINT32*)puiDatX);
        gyro_readReg(REG_ACCEL_YOUT_H,(UINT32*)puiDatY);
        gyro_readReg(REG_ACCEL_ZOUT_H,(UINT32*)puiDatZ);
    }
    else
    {
        UINT32 k;
        if (uiGyroBufInIdx > 0)
            k = (uiGyroBufInIdx-1)%GDI2_GB_NUM;
        else
            k = uiGyroBufInIdx%GDI2_GB_NUM;
        *puiDatX = puiAccelBuf[k][2];
        *puiDatY = puiAccelBuf[k][3];
        *puiDatZ = puiAccelBuf[k][4];
    }

    gyro_AdjustAccDataOffset(puiDatX,puiDatY,puiDatZ);
    return E_OK;
}

INT32 gyro_set_cfg(GYROCTL_CFG cfg_id, void *value)
{
    if(!value){
        DBG_ERR("value is null \r\n");
        return E_PAR;
    }
    DBG_IND("cfg_id %d \r\n",cfg_id);

    switch(cfg_id) {
        case GYROCTL_CFG_MODE: {
            UINT32 *p_mode = (UINT32 *)value;

            return gyro_setMode(*p_mode);
        }
        case GYROCTL_CFG_FRUN_PARAM: {
            GYRO_FREE_RUN_PARAM *frParam = (GYRO_FREE_RUN_PARAM *)value;
            return gyro_setFreeRunParam(frParam);
        }
        case GYROCTL_CFG_SIE_CB:{
            FP_GYRO_CB cb= (FP_GYRO_CB)value;
            pGyroCB = cb;
            break;
        }
        case GYROCTL_CFG_SIE_SRC:{
            UINT32 *sie_src = (UINT32 *)value;
            spi_setConfig(g_TestSpiID, SPI_CONFIG_ID_VD_SRC, *sie_src);
            break;
        }
        case GYROCTL_CFG_OFFSET:{
            GYRO_OSF_DATA *data = (GYRO_OSF_DATA *)value;
            gyro_setCalZeroOffset(data->gyro_x,data->gyro_y,data->gyro_z);
            gyro_setAccCalZeroOffset(data->ags_x,data->ags_y,data->ags_z);
            break;
        }
        default:
            DBG_ERR("not sup %d \r\n",cfg_id);
            return E_NOSPT;
    }
    return E_OK;
}

INT32 gyro_get_cfg(GYROCTL_CFG cfg_id, void *value)
{
    return E_NOSPT;
}

INT32 gyro_get_data(UINT32 *puiNum,UINT32 *timestamp,INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    if(g_GyroOpMode==GYRO_OP_MODE_SINGLE_ACCESS) {
        *puiNum = 1;
        return gyro_getSingleData(puiDatX,puiDatY,puiDatZ);
    } else if(gyroFreeRunParm.triggerMode == GYRO_FREERUN_SIE_SYNC){
        return gyro_getFreeRunData(puiNum,timestamp,puiDatX,puiDatY,puiDatZ);
    } else {
        return gyro_getFreeRunData(puiNum,0,puiDatX,puiDatY,puiDatZ);
    }

    return E_OK;
}

INT32 gyro_get_gsdata(INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{

    if(g_GyroOpMode==GYRO_OP_MODE_SINGLE_ACCESS) {
        return gyro_getGsData(puiDatX,puiDatY,puiDatZ);
    } else {
        UINT32 uiNum=0;
        return gyro_getFreeRunGsData(&uiNum,puiDatX,puiDatY,puiDatZ);
    }

}

static GYRO_OBJ gyro_icm42607_obj = { GYRO_1,gyro_open,gyro_close,gyro_set_cfg,gyro_get_cfg,gyro_get_data,gyro_get_gsdata};


//==========================================

extern INT32 nvt_gyro_comm_set_obj(UINT32 id, PGYRO_OBJ p_gyro_obj);

#ifdef __KERNEL__
int __init gyro_init(void)
{
    nvt_gyro_comm_set_obj(GYRO_1,&gyro_icm42607_obj);

    return 0;
}

void __exit gyro_exit(void)
{
    nvt_gyro_comm_set_obj(GYRO_1,NULL);

}
module_init(gyro_init);
module_exit(gyro_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("gyro icm42607");
MODULE_VERSION("1.01.002");
MODULE_LICENSE("Proprietary");


#else
int gyro_init(void)
{
    DBG_DUMP("gyro_icm42607\r\n");
    nvt_gyro_comm_set_obj(GYRO_1,&gyro_icm42607_obj);
    return 0;
}

void gyro_exit(void)
{
	nvt_gyro_comm_set_obj(GYRO_1,NULL);
}

#endif

