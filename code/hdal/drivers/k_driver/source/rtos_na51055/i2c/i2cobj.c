/*
    I2C module driver Object File

    @file       i2cobj.c
    @ingroup    mIDrvIO_I2C
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#define __MODULE__    rtos_i2c
#include <kwrap/debug.h>
unsigned int rtos_i2c_debug_level = NVT_DBG_WRN;

#include "i2c_int.h"

static I2C_OBJ gpI2CObject[I2C_FW_MAX_SUPT_ID] = {
	{
		i2c_open,
		i2c_close,
		i2c_isOpened,
		i2c_lock,
		i2c_unlock,
		i2c_islocked,
		i2c_setConfig,
		i2c_getConfig,
		i2c_transmit,
		i2c_receive,
		i2c_getRWBit,
		i2cTest_compareRegDefaultValue,
		i2cTest_getClockPeriod,
		i2cTest_setVDDelay,
		i2c_transmitDMA,
		i2c_receiveDMA,
		i2c_waitSARMatch,
		i2c_transmitDMA_En,
		i2c_transmitDMA_Wait,
		i2c_clearBus_En,
		i2c_autoRst_En
	},

	{
		i2c2_open,
		i2c2_close,
		i2c2_isOpened,
		i2c2_lock,
		i2c2_unlock,
		i2c2_islocked,
		i2c2_setConfig,
		i2c2_getConfig,
		i2c2_transmit,
		i2c2_receive,
		i2c2_getRWBit,
		i2c2Test_compareRegDefaultValue,
		i2c2Test_getClockPeriod,
		i2c2Test_setVDDelay,
		i2c2_transmitDMA,
		i2c2_receiveDMA,
		i2c2_waitSARMatch,
		i2c2_transmitDMA_En,
		i2c2_transmitDMA_Wait,
		i2c2_clearBus_En,
		i2c2_autoRst_En
	},

	{
		i2c3_open,
		i2c3_close,
		i2c3_isOpened,
		i2c3_lock,
		i2c3_unlock,
		i2c3_islocked,
		i2c3_setConfig,
		i2c3_getConfig,
		i2c3_transmit,
		i2c3_receive,
		i2c3_getRWBit,
		i2c3Test_compareRegDefaultValue,
		i2c3Test_getClockPeriod,
		i2c3Test_setVDDelay,
		i2c3_transmitDMA,
		i2c3_receiveDMA,
		i2c3_waitSARMatch,
		i2c3_transmitDMA_En,
		i2c3_transmitDMA_Wait,
		i2c3_clearBus_En,
		i2c3_autoRst_En
	},

	{
		i2c4_open,
		i2c4_close,
		i2c4_isOpened,
		i2c4_lock,
		i2c4_unlock,
		i2c4_islocked,
		i2c4_setConfig,
		i2c4_getConfig,
		i2c4_transmit,
		i2c4_receive,
		i2c4_getRWBit,
		i2c4Test_compareRegDefaultValue,
		i2c4Test_getClockPeriod,
		i2c4Test_setVDDelay,
		i2c4_transmitDMA,
		i2c4_receiveDMA,
		i2c4_waitSARMatch,
		i2c4_transmitDMA_En,
		i2c4_transmitDMA_Wait,
		i2c4_clearBus_En,
		i2c4_autoRst_En
	},

	{
		i2c5_open,
		i2c5_close,
		i2c5_isOpened,
		i2c5_lock,
		i2c5_unlock,
		i2c5_islocked,
		i2c5_setConfig,
		i2c5_getConfig,
		i2c5_transmit,
		i2c5_receive,
		i2c5_getRWBit,
		i2c5Test_compareRegDefaultValue,
		i2c5Test_getClockPeriod,
		i2c5Test_setVDDelay,
		i2c5_transmitDMA,
		i2c5_receiveDMA,
		i2c5_waitSARMatch,
		i2c5_transmitDMA_En,
		i2c5_transmitDMA_Wait,
		i2c5_clearBus_En,
		i2c5_autoRst_En
	},

};

/**
    @addtogroup mIDrvIO_I2C
*/
//@{
UINT32 i2c_get_module_num(void)
{
	// return value must <= I2C_FW_MAX_SUPT_ID
	if (nvt_get_chip_id() == CHIP_NA51055) {
		return 3; // 520
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		return 5; // 528
	} else {
		DBG_ERR("get chip id %d out of range\r\n", nvt_get_chip_id());
		return 1;
	}
}

PI2C_OBJ i2c_getDrvObject(I2C_ID I2cID)
{
	if (I2cID >= i2c_get_module_num()) {
		DBG_ERR("No such module.%d\r\n", I2cID);
		return NULL;
	}

	return &gpI2CObject[I2cID];
}

BOOL i2c_descpack_start(UINT32 *current_index)
{
	if (current_index != NULL)
		*current_index = 0;
	else
		DBG_ERR("index NULL\r\n");

	return FALSE;
}

BOOL i2c_descpack_write(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT64 data)
{
	UINT8 *pDesc = (UINT8 *)(desc_buf_addr+*current_index);
	UINT8  length, i;

	if((*current_index+8) > max_buf_size) {
		DBG_ERR("Desc Buf Size not enough %d %d\r\n", (INT)(*current_index), (INT)max_buf_size);
		return TRUE;
	}

	length = (UINT8)((data >> 32) & 0xFF);

	if((length > 127) || (length == 0)) {
		DBG_ERR("length error %d\r\n", length);
		return TRUE;
	}

	pDesc[0] = length;
	pDesc[1] = (UINT8)((data >> 40) & 0x7F);
	pDesc[2] = 0;
	pDesc[3] = 0;
	pDesc[6] = 0;
	pDesc[7] = 0;

	for(i=0; i < length; i++) {
		pDesc[4+i] = (UINT8)((data >> (i<<3))&0xFF);
	}

	*current_index += 8;
	return FALSE;
}

BOOL i2c_descpack_delay_us(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT32 delay_us)
{
	UINT8 *pDesc = (UINT8 *)(desc_buf_addr+*current_index);
	UINT64 tickcount;

	if((*current_index+4) > max_buf_size) {
		DBG_ERR("Desc Buf Size not enough %d %d\r\n", (INT)(*current_index), (INT)max_buf_size);
		return TRUE;
	}

	pDesc[0] = 0x80;

	tickcount = ((UINT64)delay_us*I2C_SOURCE_CLOCK/1000000)+1;
	if(tickcount > 0xffffff) {
		tickcount = 0xffffff;
	}

	pDesc[1] = (UINT8) ((tickcount >>  0) & 0xFF);
	pDesc[2] = (UINT8) ((tickcount >>  8) & 0xFF);
	pDesc[3] = (UINT8) ((tickcount >> 16) & 0xFF);

	*current_index += 4;
	return FALSE;
}

BOOL i2c_descpack_set(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT64 data)
{
	UINT8 *pDesc = (UINT8 *)(desc_buf_addr+*current_index);
	UINT8  type;

	if((*current_index+8) > max_buf_size) {
		DBG_ERR("Desc Buf Size not enough %d %d\r\n", (INT)(*current_index), (INT)max_buf_size);
		return TRUE;
	}

	type = (UINT8)((data >> 32) & 0xFF);

	pDesc[0] = type;
	pDesc[1] = (UINT8)((data >> 40) & 0x7F);
	pDesc[2] = (UINT8)((data >>  0) & 0xFF);
	pDesc[3] = (UINT8)((data >>  8) & 0xFF);
	pDesc[4] = (UINT8)((data >> 48) & 0xFF);
	pDesc[5] = (UINT8)((data >> 56) & 0xFF);
	pDesc[6] = (UINT8)((data >> 16) & 0xFF);
	pDesc[7] = (UINT8)((data >> 24) & 0xFF);

	*current_index += 8;
	return FALSE;
}

BOOL i2c_descpack_poll(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT64 data, UINT32 delay_us, UINT32 retry)
{
	UINT8 *pDesc = (UINT8 *)(desc_buf_addr+*current_index);
	UINT8  type;
	UINT64 tickcount;

	if((*current_index+12) > max_buf_size) {
		DBG_ERR("Desc Buf Size not enough %d %d\r\n", (INT)(*current_index), (INT)max_buf_size);
		return TRUE;
	}

	type = (UINT8)((data >> 32) & 0xFF);

	pDesc[0] = type;
	pDesc[1] = (UINT8)((data >> 40) & 0x7F);
	pDesc[2] = (UINT8)((data >>  0) & 0xFF);
	pDesc[3] = (UINT8)((data >>  8) & 0xFF);
	pDesc[4] = (UINT8)((data >> 48) & 0xFF);
	pDesc[5] = (UINT8)((data >> 56) & 0xFF);
	pDesc[6] = (UINT8)((data >> 16) & 0xFF);
	pDesc[7] = (UINT8)((data >> 24) & 0xFF);

	pDesc[8] = (UINT8)(retry & 0xFF);

	tickcount = ((UINT64)delay_us*I2C_SOURCE_CLOCK/1000000)+1;
	if(tickcount > 0xffffff) {
		tickcount = 0xffffff;
	}

	pDesc[9]  = (UINT8) ((tickcount >>  0) & 0xFF);
	pDesc[10] = (UINT8) ((tickcount >>  8) & 0xFF);
	pDesc[11] = (UINT8) ((tickcount >> 16) & 0xFF);

	*current_index += 12;
	return FALSE;
}

BOOL i2c_descpack_end(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size)
{
	UINT8 *pDesc = (UINT8 *)(desc_buf_addr+*current_index);

	if((*current_index+4) > max_buf_size) {
		DBG_ERR("Desc Buf Size not enough %d %d\r\n", (INT)(*current_index), (INT)max_buf_size);
		return TRUE;
	}

	pDesc[0] = 0;
	pDesc[1] = 0;
	pDesc[2] = 0;
	pDesc[3] = 0;

	*current_index += 4;

//	dma_flushWriteCache(desc_buf_addr, *current_index);

	return FALSE;
}


//@}
