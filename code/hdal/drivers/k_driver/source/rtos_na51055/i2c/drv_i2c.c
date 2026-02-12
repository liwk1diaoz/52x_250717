/*
    I2C2 module driver

    I2C2 module driver.

    @file       i2c2.c
    @ingroup    mIDrvIO_I2C
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#define __MODULE__    rtos_i2c
#include <kwrap/debug.h>
extern unsigned int rtos_i2c_debug_level;

#include "i2c_int.h"
#include "rtosfdt.h"
#include "libfdt.h"
#include "compiler.h"

// flg and sem
static ID FLG_ID_KDRV_I2C;
#define MODULE_FLAG_ID          FLG_ID_KDRV_I2C
#define I2C_FLAG_ID_OFS 0
#define I2C_FLAG_SESSION_OFS 8
#define I2C_FLAG_TIMEOUT_MS 1000

// i2c ctrl
static UINT32 i2c_id_lock_cnt[I2C_FW_MAX_SUPT_ID] = {0};
static I2C_SESSION_TBL session_tbl[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION] = {0};
static struct i2c_adapter i2c_adap[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION];
static struct i2c_client i2c_clint[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION];
static struct i2c_board_info i2c_board[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION];
static struct device_driver i2c_device_driver[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION];
static const char dev_name_tbl[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION][I2C_NAME_SIZE] = {
	{"I2C_DEV_0_0", "I2C_DEV_0_1", "I2C_DEV_0_2", "I2C_DEV_0_3", "I2C_DEV_0_4", "I2C_DEV_0_5",},
	{"I2C_DEV_1_0", "I2C_DEV_1_1", "I2C_DEV_1_2", "I2C_DEV_1_3", "I2C_DEV_1_4", "I2C_DEV_1_5",},
	{"I2C_DEV_2_0", "I2C_DEV_2_1", "I2C_DEV_2_2", "I2C_DEV_2_3", "I2C_DEV_2_4", "I2C_DEV_2_5",},
	{"I2C_DEV_3_0", "I2C_DEV_3_1", "I2C_DEV_3_2", "I2C_DEV_3_3", "I2C_DEV_3_4", "I2C_DEV_3_5",},
	{"I2C_DEV_4_0", "I2C_DEV_4_1", "I2C_DEV_4_2", "I2C_DEV_4_3", "I2C_DEV_4_4", "I2C_DEV_4_5",},
};

// buffer ctrl
static UINT8 *i2c_dma_buf[I2C_FW_MAX_SUPT_ID][I2C_TOTAL_SESSION] = {NULL};


static UINT32 kdrv_i2c_timeout[I2C_FW_MAX_SUPT_ID] = {0};

#if 0
- internal api
#endif
/*
    flag and semphore ctrl
*/
_INLINE FLGPTN i2c_module_get_id_flag(I2C_ID id)
{
	return FLGPTN_BIT(id + I2C_FLAG_ID_OFS);
}

_INLINE FLGPTN i2c_module_get_session_flag(I2C_ID id, I2C_SESSION session)
{
	return FLGPTN_BIT(id + session + I2C_FLAG_SESSION_OFS);
}

_INLINE void i2c_module_lock(I2C_ID id, FLGPTN flag)
{
	FLGPTN ui_flag;

	if (vos_flag_wait_timeout(&ui_flag, MODULE_FLAG_ID, flag, TWF_CLR | TWF_ANDW, vos_util_msec_to_tick(kdrv_i2c_timeout[id])) != E_OK) {
		DBG_ERR("flag 0x%.8x timeout %d ms\r\n", (unsigned int)flag, vos_util_msec_to_tick(kdrv_i2c_timeout[id]));
	}
}

_INLINE void i2c_module_unlock(FLGPTN flag)
{
	set_flg(MODULE_FLAG_ID, flag);
}

_INLINE void i2c_module_lock_id_cnt(INT i2c_id)
{
	i2c_id_lock_cnt[i2c_id]--;
	if (i2c_id_lock_cnt[i2c_id] == 0) {
		i2c_module_lock(i2c_id, i2c_module_get_id_flag(i2c_id));
	}
}

_INLINE void i2c_module_unlock_id_cnt(INT i2c_id)
{
	if (i2c_id_lock_cnt[i2c_id] == 0) {
		i2c_module_unlock(i2c_module_get_id_flag(i2c_id));
	}
	i2c_id_lock_cnt[i2c_id]++;
}

static void i2c_update_pad(struct i2c_adapter *adap)
{
	int rt;
	PIN_GROUP_CONFIG pinmux_cfg[1];

	if (adap == NULL) {
		return;
	}

	pinmux_cfg[0].pin_function = PIN_FUNC_I2C;
	pinmux_cfg[0].config = 0;
	rt = nvt_pinmux_capture(pinmux_cfg, 1);
	if (rt) {
		DBG_ERR("get pinmux fail %d\n", rt);
		return;
	}

	if (pinmux_cfg[0].config == PIN_I2C_CFG_NONE) {
		return;
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH1) {
		pad_set_pull_updown(PAD_PIN_SGPIO7, PAD_NONE);
		pad_set_pull_updown(PAD_PIN_SGPIO8, PAD_NONE);
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH1_2ND_PINMUX) {
		if (nvt_get_chip_id() == CHIP_NA51055) {
			// 520 only
			pad_set_pull_updown(PAD_PIN_HSIGPIO10, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_HSIGPIO11, PAD_NONE);
		}
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH2) {
		pad_set_pull_updown(PAD_PIN_SGPIO10, PAD_NONE);
		pad_set_pull_updown(PAD_PIN_SGPIO11, PAD_NONE);
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH2_2ND_PINMUX) {
		pad_set_pull_updown(PAD_PIN_CGPIO17, PAD_NONE);
		pad_set_pull_updown(PAD_PIN_CGPIO18, PAD_NONE);
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH3) {
		pad_set_pull_updown(PAD_PIN_PGPIO22, PAD_NONE);
		pad_set_pull_updown(PAD_PIN_PGPIO21, PAD_NONE);
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH3_2ND_PINMUX) {
		pad_set_pull_updown(PAD_PIN_CGPIO11, PAD_NONE);
		pad_set_pull_updown(PAD_PIN_CGPIO12, PAD_NONE);
	}

	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH4) {
		if (nvt_get_chip_id() == CHIP_NA51084) {
			// 528 only
			pad_set_pull_updown(PAD_PIN_PGPIO11, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_PGPIO12, PAD_NONE);
		}
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH4_2ND_PINMUX) {
		if (nvt_get_chip_id() == CHIP_NA51084) {
			// 528 only
			pad_set_pull_updown(PAD_PIN_CGPIO6, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_CGPIO7, PAD_NONE);
		}
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH5) {
		if (nvt_get_chip_id() == CHIP_NA51084) {
			// 528 only
			pad_set_pull_updown(PAD_PIN_PGPIO2, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_PGPIO3, PAD_NONE);
		}
	}
	if (pinmux_cfg[0].config & PIN_I2C_CFG_CH5_2ND_PINMUX) {
		if (nvt_get_chip_id() == CHIP_NA51084) {
			// 528 only
			DBG_ERR("pad drv not ready\r\n");
//			pad_set_pull_updown(PAD_PIN_DGPIO17, PAD_NONE);
//			pad_set_pull_updown(PAD_PIN_DGPIO18, PAD_NONE);
		}
	}

}

static INT32 i2c_buf_new(INT i2c_id, I2C_SESSION session, UINT32 size)
{
	if (size == 0) {
		DBG_ERR("size zero\r\n");
		return E_PAR;
	}

	if (i2c_dma_buf[0][0] != NULL) {
		DBG_ERR("i2c_dma_buf already allocate\r\n");
		return E_SYS;
	}

	i2c_dma_buf[i2c_id][session] = malloc(size);
//	i2c_dma_buf = kzalloc(size, GFP_KERNEL);
	if (i2c_dma_buf[i2c_id][session] == NULL) {
		DBG_ERR("malloc buf fail (size %d)\r\n", (int)size);
		return E_PAR;
	}

//	DBG_DUMP("i2c_buf_new: addr 0x%.2x size 0x%.2x\r\n", (int)i2c_dma_buf[i2c_id][session], (int)size);
	return E_OK;
}

static INT32 i2c_buf_del(INT i2c_id, I2C_SESSION session)
{
	free(i2c_dma_buf[i2c_id][session]);
	i2c_dma_buf[i2c_id][session] = NULL;

	return E_OK;
}

static const char *get_dev_name(INT i2c_id, UINT32 session_idx)
{
	if ((i2c_id < I2C_ID_I2C) || (i2c_id >= (INT)i2c_get_module_num()) || (session_idx >= I2C_TOTAL_SESSION)) {
		DBG_ERR("param id %d session %d error\r\n", (int)i2c_id, (int)session_idx);
		return NULL;
	}
	return &dev_name_tbl[i2c_id][session_idx][0];
}

/*
    set config
*/
static INT32 i2c_set_config(I2C_SESSION session, struct i2c_adapter *adap, struct i2c_msg msgs[], INT num)
{
	INT i2c_id = adap->nr;
	UINT32 tran_cnt = num;
	UINT8 config_val = msgs[0].buf[0];

	if (msgs[0].flags & NVT_I2C_VD_SRC) {
		i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_VD_SRC, config_val);
	} else if (msgs[0].flags & NVT_I2C_BUSFREE_VAL) {
		i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_BUSFREE_TIME, config_val);
	} else {
		DBG_ERR("flgs 0x%x N.S.\r\n", msgs[0].flags);
		tran_cnt = 0;
	}

	return (INT32)tran_cnt;
}

/*
    DMA mode (vd sync) - re-order dma buffer and transmit via dma
*/
static INT32 i2c_transfer_vd_sync_send(INT i2c_id, I2C_SESSION session, struct i2c_msg msgs[], INT num)
{
	UINT32 i, idx, buf_size = 0;
	I2C_STS i2c_sts;
	INT32 rt = E_OK;

	/*
	    prepare data
	*/
	// calculate total transfer size
	for (i = 0; i < (UINT32)num; i++) {
		buf_size++; // for addr
		if (msgs[i].len != msgs[0].len) {
			DBG_ERR("failed to send different size cmd for vd sync %d %d\r\n", msgs[i].len, msgs[0].len);
			return E_PAR;
		}
		buf_size += msgs[i].len;
	}

	// allocate buffer
	if (i2c_buf_new(i2c_id, session, buf_size * sizeof(UINT8)) != E_OK) {
		DBG_ERR("new buffer fail\r\n");
		return E_PAR;
	}

	// reorder dma buffer
	idx = 0;
	for (i = 0; i < (UINT32)num; i++) {
		*(i2c_dma_buf[i2c_id][session] + idx) = (UINT8)(msgs[i].addr << 1);
		idx++;
		memcpy(i2c_dma_buf[i2c_id][session] + idx, msgs[i].buf, msgs[i].len);
		idx += msgs[i].len;
	}


	/*
	    sen data via dma
	*/
	i2c_sts = i2c_getDrvObject(i2c_id)->transmitDMA((UINT32)i2c_dma_buf[i2c_id][session], idx);
	if (i2c_sts != I2C_STS_OK) {
		DBG_ERR("dma transmit fail (id %d, sts %d)\r\n", (int)i2c_id, (int)i2c_sts);
		rt = E_SYS;
	}


	// free buffer
	if (i2c_buf_del(i2c_id, session) != E_OK) {
		DBG_ERR("new buffer fail\r\n");
		return E_PAR;
	}

	return rt;
}

/*
    DMA mode (vd sync) - transmit
*/
static INT32 i2c_transfer_vd_sync(I2C_SESSION session, struct i2c_adapter *adap, struct i2c_msg *msgs, INT num)
{
	INT i2c_id = adap->nr;
	UINT32 tran_cnt = 0;
	INT32 rt;

	/* set config */
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_TRANS_DB1, (msgs[0].len + 1)); // need to check all msgs same lenght
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_VD_NUMBER, 1);
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_VD_DELAY, 0);
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_VD, TRUE);
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_RCWrite, TRUE);

	/* lock i2c session */
	if (i2c_getDrvObject(i2c_id)->lock(session) != 0) {
		tran_cnt = 0;
		goto FINISH;
	}

	/* transmit dma data (vd sync) */
	rt = i2c_transfer_vd_sync_send(i2c_id, session, msgs, num);
	if (rt < 0) {
		tran_cnt = 0;
	} else {
		tran_cnt = num;
	}

	/* unlock i2c session */
	if (i2c_getDrvObject(i2c_id)->unlock(session) != 0) {
		tran_cnt = 0;
		goto FINISH;
	}

FINISH:
	/* restore config */
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_VD, FALSE);
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_RCWrite, FALSE);

	return (INT32)tran_cnt;
}

/*
    DMA mode (descriptor) - re-order dma buffer and transmit via dma
*/
static INT32 i2c_transfer_desc_send(INT i2c_id, I2C_SESSION session, struct i2c_msg msgs[], INT num)
{
	UINT32 i, buf_size = 0, idx;
	INT32 rt = E_OK;
	I2C_STS i2c_sts;

	/*
	    prepare data
	*/
	// calculate total dma size (include DONE:0x00)
	for (i = 0; i < (UINT32)num; i++) {
		if (msgs[i].len % NVT_I2C_DESC_ALIGN) {
			DBG_ERR("msg %d, len %d must be align %d\r\n", (int)i, (int)msgs[i].len, NVT_I2C_DESC_ALIGN);
			return E_PAR;
		}
		if ((i != (UINT32)(num - 1)) && ((msgs[i].flags & NVT_I2C_DESC_MASK) == 0)) {
			DBG_ERR("flags 0x%.8x error (NOT DESC)\r\n", msgs[i].flags);
		}
		buf_size += msgs[i].len;
	}

	// allocate buffer
	if (i2c_buf_new(i2c_id, session, buf_size) != E_OK) {
		DBG_ERR("new buffer fail\r\n");
		return E_PAR;
	}

	// reorder dma buffer
	idx = 0;
	for (i = 0; i < (UINT32)num; i++) {
		memcpy(i2c_dma_buf[i2c_id][session] + idx, msgs[i].buf, msgs[i].len);
		idx += msgs[i].len;
	}

	/*
	    sen data via dma
	*/
	i2c_sts = i2c_getDrvObject(i2c_id)->transmitDMA((UINT32)i2c_dma_buf[i2c_id][session], idx);
	if (i2c_sts != I2C_STS_OK) {
		DBG_ERR("dma transmit fail (id %d, sts %d)\r\n", (int)i2c_id, (int)i2c_sts);
		rt = E_SYS;
	}

	// free buffer
	if (i2c_buf_del(i2c_id, session) != E_OK) {
		DBG_ERR("new buffer fail\r\n");
		return E_PAR;
	}

	return rt;
}

/*
    DMA mode (descriptor) - transmit
*/
static INT32 i2c_transfer_decs(I2C_SESSION session, struct i2c_adapter *adap, struct i2c_msg *msgs, INT num)
{
	INT i2c_id = adap->nr;
	UINT32 tran_cnt = 0;
	INT32 rt;

	/* set config */
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_DESC, TRUE);

	/* lock i2c session */
	if (i2c_getDrvObject(i2c_id)->lock(session) != 0) {
		tran_cnt = 0;
		goto FINISH;
	}

	/* transmit dma data (vd sync) */
	rt = i2c_transfer_desc_send(i2c_id, session, msgs, num);
	if (rt < 0) {
		tran_cnt = 0;
	} else {
#if defined(_BSP_NA51000_)
		tran_cnt = num;
#else
		tran_cnt = i2c_getDrvObject(i2c_id)->getConfig(session, I2C_CONFIG_ID_DMA_DESC_CNT);
#endif
	}

	/* unlock i2c session */
	if (i2c_getDrvObject(i2c_id)->unlock(session) != 0) {
		tran_cnt = 0;
		goto FINISH;
	}

FINISH:
	/* restore config */
	i2c_getDrvObject(i2c_id)->setConfig(session, I2C_CONFIG_ID_DMA_DESC, FALSE);

	return (INT32)tran_cnt;
}

/*
    PIO mode - transmit or receive
*/
static INT32 i2c_transfer_pio(I2C_SESSION session, struct i2c_adapter *adap, struct i2c_msg *msgs, INT num)
{
	INT i2c_id = adap->nr;
	UINT32 i, j, tran_cnt = 0;
	I2C_DATA data = {0};
	I2C_BYTE byte[I2C_BYTE_CNT_8] = {0};
	I2C_STS sts;
	struct i2c_msg *cur_msgs = NULL, *nxt_msgs = NULL;
	data.pByte = byte;

	/* lock i2c session */
	if (i2c_getDrvObject(i2c_id)->lock(session) != 0) {
		DBG_ERR("lock fail (id %d)\r\n", i2c_id);
		return 0;
	}
	/* transmit or receive i2c data */
	data.VersionInfo = DRV_VER_96680;
	for (i = 0; i < (UINT32)num; i++) {
		cur_msgs = &msgs[i];
		if (i != ((UINT32)num - 1)) {
			nxt_msgs = &msgs[i + 1];
		} else {
			nxt_msgs = NULL;
		}
		if (cur_msgs == NULL) {
			DBG_ERR("num %d msg error\r\n", (INT)i);
			continue;
		} else if (cur_msgs->buf == NULL) {
			DBG_ERR("num %d msg buf error\r\n", (INT)i);
			continue;
		}
		if (cur_msgs->flags == I2C_W_FLG) {
			/* transmit data */
			if (nxt_msgs == NULL) {
				data.ByteCount = (I2C_BYTE_CNT)(cur_msgs->len + 1);
			} else {
				data.ByteCount = (I2C_BYTE_CNT)(cur_msgs->len + 2);
			}
			for (j = 0; j < data.ByteCount; j++) {
				if (j == 0) {
					// first data, send slave address and START
					data.pByte[j].uiValue = ((cur_msgs->addr) << 1);
					data.pByte[j].Param = I2C_BYTE_PARAM_START;
					// 1 byte with START and STOP
					if (j == (I2C_BYTE_CNT)(data.ByteCount - 1)) {
						data.pByte[j].Param |= I2C_BYTE_PARAM_STOP;
					}
				} else if (j == (I2C_BYTE_CNT)(data.ByteCount - 1)) {
					// last data, send STOP to finish or send RE-START for next msgs read
					data.pByte[j].uiValue = cur_msgs->buf[j - 1];
					data.pByte[j].Param = I2C_BYTE_PARAM_STOP;
					if (nxt_msgs != NULL) {
						if (nxt_msgs->flags == I2C_R_FLG) {
							data.pByte[j].uiValue = ((nxt_msgs->addr) << 1) + I2C_R_FLG;
							data.pByte[j].Param = I2C_BYTE_PARAM_START;
						}
					}
				} else {
					data.pByte[j].uiValue = cur_msgs->buf[j - 1];
					data.pByte[j].Param = I2C_BYTE_PARAM_NONE;
				}
			}
			sts = i2c_getDrvObject(i2c_id)->transmit(&data);
			if (i2c_getBaseStatus(sts) == I2C_STS_OK) {
				tran_cnt++;
			} else {
				DBG_ERR("cmd %d W fail 0x%x\r\n", (INT)i, (INT)i2c_getBaseStatus(sts));
				break;
			}

		} else if (cur_msgs->flags == I2C_R_FLG) {
			/* receive data */
			data.ByteCount = (I2C_BYTE_CNT)(cur_msgs->len);
			for (j = 0; j < data.ByteCount; j++) {
				data.pByte[j].uiValue = 0;
				if (j == (I2C_BYTE_CNT)(data.ByteCount - 1)) {
					// last data, send STOP to finish or RE-START next msgs transfer
					data.pByte[j].Param = I2C_BYTE_PARAM_NACK | I2C_BYTE_PARAM_STOP;
					/*if (nxt_msgs != NULL) {
					    if (nxt_msgs->flags == I2C_R_FLG) {
					        data.pByte[j].Param = I2C_BYTE_PARAM_NACK | I2C_BYTE_PARAM_START;
					    }
					}*/
				} else {
					data.pByte[j].Param = I2C_BYTE_PARAM_NONE;
				}
			}
			sts = i2c_getDrvObject(i2c_id)->receive(&data);
			for (j = 0; j < cur_msgs->len; j++) {
				cur_msgs->buf[j] = data.pByte[j].uiValue;
			}
			if (i2c_getBaseStatus(sts) == I2C_STS_OK) {
				tran_cnt++;
			} else {
				DBG_ERR("cmd %d R fail 0x%x\r\n", (INT)i, (INT)i2c_getBaseStatus(sts));
				break;
			}
		} else {
			DBG_ERR("num %d msg flg 0x%x error\r\n", (INT)i, cur_msgs->flags);
		}
	}

	/* unlock i2c session */
	if (i2c_getDrvObject(i2c_id)->unlock(session) != 0) {
		return 0;
	}

	return (INT32)tran_cnt;
}

static void i2c_update_dtsi_info(INT i2c_id, UINT32 session_idx)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	uint32_t *cell = NULL;
	char path[20] = {0};
	int addr[I2C_FW_MAX_SUPT_ID] = {IOADDR_I2C_REG_BASE, IOADDR_I2C2_REG_BASE, IOADDR_I2C3_REG_BASE, IOADDR_I2C4_REG_BASE, IOADDR_I2C5_REG_BASE};
	int nodeoffset, len;

	if (i2c_id == 0) {
		sprintf(path,"/i2c@%x", addr[i2c_id]);
	} else {
		sprintf(path,"/i2c%d@%x", i2c_id + 1, addr[i2c_id]);
	}

	nodeoffset = fdt_path_offset((const void*)fdt_addr, path);
	if (nodeoffset <= 0) {
		// no dtsi info
		return;
	}

	/* set I2C_CONFIG_ID_GSR */
	/* GSR must set before BUSCLOCK , because BUSCLOCK will reference GSR */
	cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gsr", (int *)&len);
	if (cell != NULL) {
		i2c_getDrvObject(i2c_id)->setConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_GSR, (UINT32)be32_to_cpu(cell[0]));
	}

	/* set I2C_CONFIG_ID_BUSCLOCK */
	cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "clock-frequency", (int *)&len);
	if (cell != NULL) {
		i2c_getDrvObject(i2c_id)->setConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_BUSCLOCK, (UINT32)be32_to_cpu(cell[0]));
	}

	/* set I2C_CONFIG_ID_TSR */
	/* TSR must set after BUSCLOCK , because BUSCLOCK will set TSR to 1 */
	cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "tsr", (int *)&len);
	if (cell != NULL) {
		i2c_getDrvObject(i2c_id)->setConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_TSR, (UINT32)be32_to_cpu(cell[0]));
	}

	/* set I2C_CONFIG_ID_TIMEOUT_MS */
	cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "timeout", (int *)&len);
	if (cell != NULL) {
		i2c_getDrvObject(i2c_id)->setConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_TIMEOUT_MS, (UINT32)be32_to_cpu(cell[0]));
		kdrv_i2c_timeout[i2c_id] = (UINT32)be32_to_cpu(cell[0]);
	}

	/* Set I2C_CONFIG_ID_BUS_CLEAR */
	cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "auto_busclear", (int *)&len);
	if (cell != NULL) {
		i2c_getDrvObject(i2c_id)->setConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_BUS_CLEAR, (UINT32)be32_to_cpu(cell[0]));
	}

	/* Set I2C_CONFIG_ID_AUTO_RST */
	cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "auto_rst", (int *)&len);
	if (cell != NULL) {
		i2c_getDrvObject(i2c_id)->setConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_AUTO_RST, (UINT32)be32_to_cpu(cell[0]));
	}
}

#if 0
- extern api
#endif

void i2c_bus_clear(INT i2c_id, I2C_SESSION i2c_session)
{
	if (i2c_getDrvObject(i2c_id)->lock(i2c_session) != 0) {
		DBG_ERR("bus clear lock fail (id %d)\r\n", i2c_id);
		return;
	}
	i2c_getDrvObject(i2c_id)->clearBus_En();
	i2c_getDrvObject(i2c_id)->unlock(i2c_session);
}

void i2c_auto_rst(INT i2c_id, I2C_SESSION *i2c_session)
{
	i2c_getDrvObject(i2c_id)->autoRst_En(i2c_session);
}

void i2c_recover(struct i2c_client *client)
{
	INT i2c_id = 0;
	I2C_SESSION session_idx;

	if (client == NULL) {
		DBG_ERR("input structure error\r\n");
		return;
	} else if (client->adapter == NULL) {
		DBG_ERR("input sub-structure error\r\n");
		return;
	}

	i2c_id = client->adapter->nr;

	if ((i2c_id >= (INT)i2c_get_module_num()) || (i2c_id < I2C_ID_I2C)) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return;
	} else if (i2c_getDrvObject(i2c_id) == NULL) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return;
	}

	for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
		if (strcmp(session_tbl[i2c_id][session_idx].name, client->adapter->dev.driver->name) == 0) {
			goto FIND;
		}
	}
FIND:
	if (session_idx == I2C_TOTAL_SESSION) {
		DBG_ERR("session not find\r\n");
		return;
	}

	if (i2c_getDrvObject(i2c_id)->getConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_BUS_CLEAR)) {
		i2c_bus_clear(i2c_id, session_tbl[i2c_id][session_idx].session);
	}

	if (i2c_getDrvObject(i2c_id)->getConfig(session_tbl[i2c_id][session_idx].session, I2C_CONFIG_ID_AUTO_RST)) {
		i2c_auto_rst(i2c_id, &session_tbl[i2c_id][session_idx].session);
	}
}

struct i2c_adapter *i2c_get_adapter(INT i2c_id)
{
	static BOOL init = FALSE;
	struct i2c_adapter *adap = NULL;
	UINT32 i, j, session_idx;

	if ((i2c_id >= (INT)i2c_get_module_num()) || (i2c_id < I2C_ID_I2C)) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return NULL;
	}

	if (!init) {
		init = TRUE;
		cre_flg(&FLG_ID_KDRV_I2C, NULL, "FLG_ID_KDRV_I2C");
		for (i = I2C_ID_I2C; i < i2c_get_module_num(); i++) {
			for (j = 0; j < I2C_TOTAL_SESSION; j++) {
				session_tbl[i][j].name = NULL;
				session_tbl[i][j].session = I2C_TOTAL_SESSION;
			}
		}

		/* initial timeout value */
		for (i = 0; i < I2C_FW_MAX_SUPT_ID;i++) {
			kdrv_i2c_timeout[i] = I2C_FLAG_TIMEOUT_MS;
		}

	}

	for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
		if (session_tbl[i2c_id][session_idx].name == NULL) {

			adap = &i2c_adap[i2c_id][session_idx];
			adap->nr = i2c_id;
			adap->dev.driver->name = get_dev_name(i2c_id, session_idx);
			if (adap->dev.driver->name == NULL) {
				return NULL;
			}
			session_tbl[i2c_id][session_idx].name = adap->dev.driver->name;
			goto FIND;
		}
	}

FIND:

	if (session_idx == I2C_TOTAL_SESSION) {
		DBG_ERR("session overflow\r\n");
		return NULL;
	}

	return adap;
}

struct i2c_client *i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
	ER rt = E_OK;
	INT i2c_id;
	UINT32 session_idx;

	if (adap == NULL) {
		DBG_ERR("input structure error\r\n");
		return NULL;
	}

	i2c_id = adap->nr;

	if ((i2c_id >= (INT)i2c_get_module_num()) || (i2c_id < I2C_ID_I2C)) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return NULL;
	} else if (i2c_getDrvObject(i2c_id) == NULL) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return NULL;
	}


	// get i2c session
	for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
		if (strcmp(session_tbl[i2c_id][session_idx].name, adap->dev.driver->name) == 0) {
			goto FIND;
		}
	}

FIND:
	if (session_idx == I2C_TOTAL_SESSION) {
		DBG_ERR("session not find\r\n");
		return NULL;
	}

	i2c_adap[i2c_id][session_idx] = *adap;
	i2c_board[i2c_id][session_idx] = *info;
	i2c_clint[i2c_id][session_idx].adapter = &i2c_adap[i2c_id][session_idx];
	i2c_clint[i2c_id][session_idx].adapter->dev.driver = &i2c_device_driver[i2c_id][session_idx];
	i2c_clint[i2c_id][session_idx].adapter->dev.driver->name = get_dev_name(i2c_id, session_idx);

	rt = i2c_getDrvObject(i2c_id)->open(&session_tbl[i2c_id][session_idx].session);
	if (rt != E_OK) {
		DBG_ERR("i2c open fail\r\n");
		return NULL;
	}

	/* parse dtsi info */
	i2c_update_dtsi_info(i2c_id, session_idx);

	/* unlock kdrv */
	i2c_module_unlock_id_cnt(i2c_id);
	i2c_module_unlock(i2c_module_get_session_flag(i2c_id, session_idx));

	i2c_recover(&i2c_clint[i2c_id][session_idx]);

	return &i2c_clint[i2c_id][session_idx];
}

INT i2c_add_driver(struct i2c_driver *driver)
{
	UINT32 i, cur_i2c_id = i2c_get_module_num(), session_idx;

	if (driver == NULL) {
		DBG_ERR("input structure error\r\n");
		return E_SYS;
	} else if ((driver->probe == NULL) || (driver->id_table == NULL)) {
		DBG_ERR("input structure error (probe 0x%.8x, id_table 0x%.8x)\r\n", (INT)driver->probe, (INT)driver->id_table);
		return E_SYS;
	}

	// get i2c id
	for (i = I2C_ID_I2C; i < i2c_get_module_num(); i++) {
		for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
			if (strcmp(i2c_board[i][session_idx].type, driver->id_table->name) == 0) {
				cur_i2c_id = i;
				goto FIND;
			}
		}
	}

FIND:
	if (cur_i2c_id == i2c_get_module_num()) {
		DBG_ERR("fail (name:%s) \r\n", driver->id_table->name);
		return E_SYS;
	}

	driver->probe(&i2c_clint[cur_i2c_id][session_idx], driver->id_table);

//	i2c_module_unlock_id_cnt(cur_i2c_id);
//	i2c_module_unlock(i2c_module_get_session_flag(cur_i2c_id, session_idx));

	return 0;
}

void i2c_set_clientdata(struct i2c_client *dev, void *data)
{
}

void i2c_unregister_device(struct i2c_client *client)
{
	ER rt = E_OK;
	INT i2c_id;
	UINT32 session_idx;

	if (client == NULL) {
		DBG_ERR("input structure error\r\n");
		return;
	} else if (client->adapter == NULL) {
		DBG_ERR("input sub-structure error\r\n");
		return;
	}

	i2c_id = client->adapter->nr;
	if ((i2c_id >= (INT)i2c_get_module_num()) || (i2c_id < I2C_ID_I2C)) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return;
	} else if (i2c_getDrvObject(i2c_id) == NULL) {
		DBG_ERR("input id error %d\r\n", i2c_id);
		return;
	}

	for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
		if (strcmp(session_tbl[i2c_id][session_idx].name, client->adapter->dev.driver->name) == 0) {
			goto FIND;
		}
	}
FIND:
	if (session_idx == I2C_TOTAL_SESSION) {
		DBG_ERR("session not find\r\n");
		return;
	}

	rt = i2c_getDrvObject(i2c_id)->close(session_tbl[i2c_id][session_idx].session);
	if (rt != E_OK) {
		DBG_ERR("i2c close fail\r\n");
	}

	session_tbl[i2c_id][session_idx].name = NULL;
	session_tbl[i2c_id][session_idx].session = I2C_TOTAL_SESSION;

	i2c_module_lock(i2c_id, i2c_module_get_session_flag(i2c_id, session_idx));
	i2c_module_lock_id_cnt(i2c_id);

}

void i2c_del_driver(struct i2c_driver *driver)
{
	UINT32 i, cur_i2c_id = i2c_get_module_num(), session_idx;

	if (driver == NULL) {
		DBG_ERR("input structure error\r\n");
		return;
	} else if ((driver->remove == NULL) || (driver->id_table == NULL)) {
		DBG_ERR("input structure error (remove 0x%.8x, id_table 0x%.8x)\r\n", (INT)driver->remove, (INT)driver->id_table);
		return;
	}

	// get i2c id
	for (i = I2C_ID_I2C; i < i2c_get_module_num(); i++) {
		for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
			if (strcmp(i2c_board[i][session_idx].type, driver->id_table->name) == 0) {
				cur_i2c_id = i;
				goto FIND;
			}
		}
	}

FIND:
	if (cur_i2c_id == i2c_get_module_num()) {
		DBG_ERR("fail (name:%s) \r\n", driver->id_table->name);
		return;
	}
//	i2c_module_lock(i2c_module_get_session_flag(cur_i2c_id, session_idx));
//	i2c_module_lock_id_cnt(cur_i2c_id);

	driver->remove(&i2c_clint[cur_i2c_id][session_idx]);
}

INT32 i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, INT num)
{
	UINT32 tran_cnt = 0, session_idx;;
	INT i2c_id;

	if (adap == NULL) {
		DBG_ERR("input structure error\r\n");
		return E_SYS;
	}

	// chk input param
	if (adap->nr >= (int)i2c_get_module_num()) {
		DBG_ERR("input id error %d\r\n", adap->nr);
		return E_PAR;
	} else if (i2c_getDrvObject(adap->nr) == NULL) {
		DBG_ERR("input id error %d\r\n", adap->nr);
		return E_PAR;
	}

	// update pad to pull-none
	i2c_update_pad(adap);

	// get i2c id
	i2c_id = adap->nr;
	i2c_module_lock_id_cnt(adap->nr);

	// get i2c session
	for (session_idx = 0; session_idx < I2C_TOTAL_SESSION; session_idx++) {
		if (strcmp(session_tbl[i2c_id][session_idx].name, adap->dev.driver->name) == 0) {
			goto FIND;
		}
	}
FIND:
	if (session_idx == I2C_TOTAL_SESSION) {
		DBG_ERR("session not find\r\n");
		return E_PAR;
	}

	i2c_module_lock(adap->nr, i2c_module_get_session_flag(adap->nr, session_idx));


	if ((num == 1) && (msgs[0].flags & NVT_I2C_CONFIG_MASK)) {
		tran_cnt = i2c_set_config(session_tbl[i2c_id][session_idx].session, adap, &msgs[0], num);
	} else if (msgs[0].flags & NVT_I2C_VD_SEND) {
		tran_cnt = i2c_transfer_vd_sync(session_tbl[i2c_id][session_idx].session, adap, msgs, num);
	} else if (msgs[0].flags & NVT_I2C_DESC_MASK) {
		tran_cnt = i2c_transfer_decs(session_tbl[i2c_id][session_idx].session, adap, msgs, num);
	} else {
		tran_cnt = i2c_transfer_pio(session_tbl[i2c_id][session_idx].session, adap, msgs, num);
	}

	i2c_module_unlock(i2c_module_get_session_flag(adap->nr, session_idx));
	i2c_module_unlock_id_cnt(adap->nr);


	return tran_cnt;
}

