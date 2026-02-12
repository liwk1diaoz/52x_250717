/**
    Gyro Driver Header file.

    @file       GyroDrv.h
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _GYRO_DRV_H
#define _GYRO_DRV_H

#include "kwrap/type.h"
#include "comm/gyro_spi.h"
typedef enum
{
    GYRO_OP_MODE_SINGLE_ACCESS,       ///< gyro in single access mode, read/write register one by one
    GYRO_OP_MODE_FREE_RUN,            ///< gyro in free run mode, read register after periodic trigger automatically
    ENUM_DUMMY4WORD(GYRO_OP_MODE)
} GYRO_OP_MODE;

typedef void (*FP_GYRO_CB)(void);     ///< gyor sie sync callback

typedef enum
{
    GYRO_FREERUN_SIE_SYNC,             ///< SIE sync mode
    GYRO_FREERUN_FWTRIGGER,            ///< FW trigger mode
    ENUM_DUMMY4WORD(GYRO_FREERUN_TRIGGER_MODE)
} GYRO_FREERUN_TRIGGER_MODE;


typedef struct _GYRO_FREE_RUN_PARAM{
    UINT32                        uiPeriodUs;    ///< free run period in Us
    UINT32                        uiDataNum;     ///< gyro data number in a period
    UINT32                        uiTriggerIdx;  ///< the trigger index of gyro free run mode
    GYRO_FREERUN_TRIGGER_MODE     triggerMode;   ///< trigger mode
} GYRO_FREE_RUN_PARAM;

typedef struct _GYRO_CFGINFO{
    UINT32            AxisSelec[3];              ///< Gyro data axis
    UINT32            DirSelec[3];               ///< Gyro data direction
}GYRO_CFGINFO;

typedef struct _GYRO_OSF_DATA{
	INT32 gyro_x;
	INT32 gyro_y;
	INT32 gyro_z;
	INT32 ags_x;
	INT32 ags_y;
	INT32 ags_z;
} GYRO_OSF_DATA;

typedef struct _GYRO_OPENOBJ{
	FP_GYRO_CB  fp_cb;                          ///<
} GYRO_OPENOBJ;

typedef struct _GYRO_STARTOBJ {
    SPI_ID SpiID;
    SPI_GYRO_INFO *pSPIGyroInfo;
}GYRO_STARTOBJ;

typedef struct _GYRO_RSCINFOINIT {
    UINT32            gdFormat;       ///< Gyro data format
    UINT32            AxisSelec[3];   ///< Gyro data axis
    UINT32            DirSelec[3];    ///< Gyro data direction
    UINT32            uiDataUnit;     ///< Gyro data unit, in degree/65536sec
    UINT32            uiCalGain[3];   ///< calibration gain, in 1/1024, recommended fixed to 1024
    INT32             iCalOfs[3];     ///< Calibration offset, recommended fixed to 0
}GYRO_RSCINFOINIT;


typedef enum {
    GYRO_SIE_1,                    ///< VD source is SIE2
    GYRO_SIE_2,                    ///< VD source is SIE2
    GYRO_SIE_4,                    ///< VD source is SIE4
    GYRO_SIE_5,                    ///< VD source is SIE5
    ENUM_DUMMY4WORD(GYRO_SIE_ID)
} GYRO_SIE_ID;


typedef enum {
	GYROCTL_CFG_MODE,           ///< [set], using GYRO_OP_MODE enum
    GYROCTL_CFG_FRUN_PARAM,
    GYROCTL_CFG_SIE_CB,
    GYROCTL_CFG_SIE_SRC,
    GYROCTL_CFG_OFFSET,
	ENUM_DUMMY4WORD(GYROCTL_CFG)
} GYROCTL_CFG;

typedef enum {
    GYRO_1,                    ///< GYRO ID1
    GYRO_2,                    ///< GYRO ID2
    ENUM_DUMMY4WORD(GYRO_ID)
} GYRO_ID;

typedef struct {
    /* Object Common items */
    GYRO_ID   id;                                  ///< Gyro Object ID.

    INT32(*open)(void);                            ///< Gyro Object open.
    INT32(*close)(void);                           ///< Gyro Object close.

    INT32(*set_cfg)(GYROCTL_CFG cfg_id, void *value);///< Set Gyro Object configuration.
    INT32(*get_cfg)(GYROCTL_CFG cfg_id, void *value);///< Get Gyro Object configuration.

    INT32(*get_data)(UINT32 *puiNum, UINT32 *timestamp,INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ);///< Get Gyro Object configuration.
    INT32(*get_gsdata)(INT32 *puiDatX, INT32 *puiDatY, INT32 *puiDatZ);///< Get Gyro Object configuration.
} GYRO_OBJ, *PGYRO_OBJ;


#endif// _GYRO_DRV_H

