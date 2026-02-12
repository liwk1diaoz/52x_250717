/*
    Gyro Driver dymmy.

    @file       gyro_dummy.c
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

static GYRO_FREE_RUN_PARAM  gyroFreeRunParm={0};

#define GDI2_GB_NUM (32)

INT32 gyro_open(void)
{
    return E_OK;
}


INT32 gyro_close(void)
{
    return E_OK;
}

INT32 gyro_shutdown(void)
{
    return E_OK;
}

INT32 gyro_readReg(UINT32 uiAddr, UINT32 * puiData)
{
    return E_OK;
}

INT32 gyro_writeReg(UINT32 uiAddr, UINT32 uiData)
{
    return E_OK;
}


INT32 gyro_setFreeRunParam(GYRO_FREE_RUN_PARAM *frParam)
{
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
UINT32 gyro_getLSB(void)
{
    return 131;
}

INT32 gyro_getFreeRunData(UINT32 *puiNum, UINT32 *pulFcnt,INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    *puiNum = gyroFreeRunParm.uiDataNum;
    return E_OK;
}
INT32 gyro_getFreeRunGsData(UINT32 *puiNum, INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    *puiNum = gyroFreeRunParm.uiDataNum;
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
            break;
        }
        case GYROCTL_CFG_FRUN_PARAM: {
            GYRO_FREE_RUN_PARAM *frParam = (GYRO_FREE_RUN_PARAM *)value;
            return gyro_setFreeRunParam(frParam);
        }
        case GYROCTL_CFG_SIE_CB:{
            break;
        }
        case GYROCTL_CFG_SIE_SRC:{
            break;
        }
        case GYROCTL_CFG_OFFSET:{
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

INT32 gyro_get_data(UINT32 *puiNum, UINT32 *pulTs,INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
    *puiNum = gyroFreeRunParm.uiDataNum;
#if 0
	if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL && pulTs != NULL) {
		memset((void *)puiDatX, 0, (*puiNum)*sizeof(UINT32));
		memset((void *)puiDatY, 0, (*puiNum)*sizeof(UINT32));
		memset((void *)puiDatZ, 0, (*puiNum)*sizeof(UINT32));
	}
#endif
    return E_OK;
}

INT32 gyro_get_gsdata(INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ)
{
#if 0
    UINT32 puiNum = gyroFreeRunParm.uiDataNum;
	if (puiDatX != NULL && puiDatY != NULL && puiDatZ != NULL) {
		memset((void *)puiDatX, 0, (puiNum)*sizeof(UINT32));
		memset((void *)puiDatY, 0, (puiNum)*sizeof(UINT32));
		memset((void *)puiDatZ, 0, (puiNum)*sizeof(UINT32));
	}
#endif
    return E_OK;
}

static GYRO_OBJ gyro_dummy_obj = { GYRO_1,gyro_open,gyro_close,gyro_set_cfg,gyro_get_cfg,gyro_get_data,gyro_get_gsdata};


//==========================================

extern INT32 nvt_gyro_comm_set_obj(UINT32 id, PGYRO_OBJ p_gyro_obj);

#ifdef __KERNEL__
int __init gyro_init(void)
{
    DBG_DUMP("gyro_dummy\r\n");
    nvt_gyro_comm_set_obj(GYRO_1,&gyro_dummy_obj);

    return 0;
}

void __exit gyro_exit(void)
{
    nvt_gyro_comm_set_obj(GYRO_1,NULL);

}
module_init(gyro_init);
module_exit(gyro_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("gyro dummy");
MODULE_VERSION("1.01.002");
MODULE_LICENSE("Proprietary");


#else
int gyro_init(void)
{
    nvt_gyro_comm_set_obj(GYRO_1,&gyro_dummy_obj);
    return 0;
}

void gyro_exit(void)
{
	nvt_gyro_comm_set_obj(GYRO_1,NULL);
}

#endif

