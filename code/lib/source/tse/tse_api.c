/**
	@brief 		tse library.\n

	@file 		tse_api.c

	@ingroup 	mTSE

	@note 		Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <kwrap/error_no.h>
#include <kwrap/type.h>
#include <comm/tse_ioctl.h>
#include "tse.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define _TSE_ "/dev/nvt_drv_tse0"
#define TSE_STS_OPENED	0x00000001
#define TSE_STS_BUSY 	0x00000002

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _TSE_CTRL {
    int fd;
	int status;
} TSE_CTRL;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static TSE_CTRL g_tse_ctrl = {0};
static TSE_IOC_MUX_OBJ g_tse_mux_obj = {0};
static TSE_IOC_DEMUX_OBJ g_tse_demux_obj = {0};
static TSE_IOC_HWCPY_OBJ g_tse_hwcpy_obj = {0};

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
static int tse_mux_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	int ret;
	switch (CfgID) {
		case TSMUX_CFG_ID_PAYLOADSIZE:
			g_tse_mux_obj.cfg.payload_size = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_SRC_INFO:
			{
				TSE_BUF_INFO *buf = (TSE_BUF_INFO *)uiCfgValue;
				g_tse_mux_obj.cfg.src_info.addr = buf->addr;
				g_tse_mux_obj.cfg.src_info.size = buf->size;
				g_tse_mux_obj.cfg.src_info.pnext = NULL;
			}
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_DST_INFO:
			{
				TSE_BUF_INFO *buf = (TSE_BUF_INFO *)uiCfgValue;
				g_tse_mux_obj.cfg.dst_info.addr = buf->addr;
				g_tse_mux_obj.cfg.dst_info.size = buf->size;
				g_tse_mux_obj.cfg.dst_info.pnext = NULL;
			}
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_SYNC_BYTE:
			g_tse_mux_obj.cfg.sync_byte = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_CONTINUITY_CNT:
			g_tse_mux_obj.cfg.continuity_cnt = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_PID:
			g_tse_mux_obj.cfg.pid = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_TEI:
			g_tse_mux_obj.cfg.tei = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_TP:
			g_tse_mux_obj.cfg.tp = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_SCRAMBLECTRL:
			g_tse_mux_obj.cfg.scramblectrl = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_START_INDICTOR:
			g_tse_mux_obj.cfg.start_indictor = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_STUFF_VAL:
			g_tse_mux_obj.cfg.stuff_val = uiCfgValue;
			ret = E_OK;
			break;
		case TSMUX_CFG_ID_ADAPT_FLAGS:
			g_tse_mux_obj.cfg.adapt_flags = uiCfgValue;
			ret = E_OK;
			break;
		default:
			ret = E_SYS;
			break;
	}
	return ret;
}

static int tse_mux_getConfig(TSE_CFG_ID CfgID)
{
	int ret;
	switch (CfgID) {
		case TSMUX_CFG_ID_MUXING_LEN:
			ret = g_tse_mux_obj.cfg.muxing_len;
			break;
		case TSMUX_CFG_ID_CON_CURR_CNT:
			ret = g_tse_mux_obj.cfg.con_curr_cnt;
			break;
		case TSMUX_CFG_ID_LAST_DATA_MUX_MODE:
			ret = g_tse_mux_obj.cfg.last_data_mux_mode;
			break;
		default:
			ret = E_SYS;
			break;
	}
	return ret;
}

static int tse_demux_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	int ret;
	switch (CfgID) {
		case TSDEMUX_CFG_ID_SYNC_BYTE:
			g_tse_demux_obj.cfg.sync_byte = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_ADAPTATION_FLAG:
			g_tse_demux_obj.cfg.adaptation_flag = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_PID0_ENABLE:
			g_tse_demux_obj.cfg.pid_enable[0] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_PID0_VALUE:
			g_tse_demux_obj.cfg.pid_value[0] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_CONTINUITY0_MODE:
			g_tse_demux_obj.cfg.continuity_mode[0] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_CONTINUITY0_VALUE:
			g_tse_demux_obj.cfg.continuity_value[0] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_PID1_ENABLE:
			g_tse_demux_obj.cfg.pid_enable[1] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_PID1_VALUE:
			g_tse_demux_obj.cfg.pid_value[1] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_CONTINUITY1_MODE:
			g_tse_demux_obj.cfg.continuity_mode[1] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_CONTINUITY1_VALUE:
			g_tse_demux_obj.cfg.continuity_value[1] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_PID2_ENABLE:
			g_tse_demux_obj.cfg.pid_enable[2] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_PID2_VALUE:
			g_tse_demux_obj.cfg.pid_value[2] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_CONTINUITY2_MODE:
			g_tse_demux_obj.cfg.continuity_mode[2] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_CONTINUITY2_VALUE:
			g_tse_demux_obj.cfg.continuity_value[2] = uiCfgValue;
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_IN_INFO:
			{
				TSE_BUF_INFO *buf = (TSE_BUF_INFO *)uiCfgValue;
				g_tse_demux_obj.cfg.in_info.addr = buf->addr;
				g_tse_demux_obj.cfg.in_info.size = buf->size;
				g_tse_demux_obj.cfg.in_info.pnext = NULL;
			}
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_OUT0_INFO:
			{
				TSE_BUF_INFO *buf = (TSE_BUF_INFO *)uiCfgValue;
				g_tse_demux_obj.cfg.out_info[0].addr = buf->addr;
				g_tse_demux_obj.cfg.out_info[0].size = buf->size;
				g_tse_demux_obj.cfg.out_info[0].pnext = NULL;
			}
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_OUT1_INFO:
			{
				TSE_BUF_INFO *buf = (TSE_BUF_INFO *)uiCfgValue;
				g_tse_demux_obj.cfg.out_info[1].addr = buf->addr;
				g_tse_demux_obj.cfg.out_info[1].size = buf->size;
				g_tse_demux_obj.cfg.out_info[1].pnext = NULL;
			}
			ret = E_OK;
			break;
		case TSDEMUX_CFG_ID_OUT2_INFO:
			{
				TSE_BUF_INFO *buf = (TSE_BUF_INFO *)uiCfgValue;
				g_tse_demux_obj.cfg.out_info[2].addr = buf->addr;
				g_tse_demux_obj.cfg.out_info[2].size = buf->size;
				g_tse_demux_obj.cfg.out_info[2].pnext = NULL;
			}
			ret = E_OK;
			break;
		default:
			ret = E_SYS;
			break;
	}
	return ret;
}

static int tse_demux_getConfig(TSE_CFG_ID CfgID)
{
	int ret;
	switch (CfgID) {
		case TSDEMUX_CFG_ID_CONTINUITY0_LAST:
			ret = g_tse_demux_obj.cfg.continuity_last[0];
			break;
		case TSDEMUX_CFG_ID_CONTINUITY1_LAST:
			ret = g_tse_demux_obj.cfg.continuity_last[1];
			break;
		case TSDEMUX_CFG_ID_CONTINUITY2_LAST:
			ret = g_tse_demux_obj.cfg.continuity_last[2];
			break;
		case TSDEMUX_CFG_ID_OUT0_TOTAL_LEN:
			ret = g_tse_demux_obj.cfg.out_total_len[0];
			break;
		case TSDEMUX_CFG_ID_OUT1_TOTAL_LEN:
			ret = g_tse_demux_obj.cfg.out_total_len[1];
			break;
		case TSDEMUX_CFG_ID_OUT2_TOTAL_LEN:
			ret = g_tse_demux_obj.cfg.out_total_len[2];
			break;
		default:
			ret = E_SYS;
			break;
	}
	return ret;
}

static int tse_hwcpy_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	int ret;
	switch (CfgID) {
		case HWCOPY_CFG_ID_CMD:
			g_tse_hwcpy_obj.cfg.id_cmd = uiCfgValue;
			ret = E_OK;
			break;
		case HWCOPY_CFG_ID_CTEX:
			g_tse_hwcpy_obj.cfg.id_ctex = uiCfgValue;
			ret = E_OK;
			break;
		case HWCOPY_CFG_ID_SRC_ADDR:
			g_tse_hwcpy_obj.cfg.src_addr = uiCfgValue;
			ret = E_OK;
			break;
		case HWCOPY_CFG_ID_DST_ADDR:
			g_tse_hwcpy_obj.cfg.dst_addr = uiCfgValue;
			ret = E_OK;
			break;
		case HWCOPY_CFG_ID_SRC_LEN:
			g_tse_hwcpy_obj.cfg.src_len = uiCfgValue;
			ret = E_OK;
			break;
		default:
			ret = E_SYS;
			break;
	}
	return ret;
}

static int tse_hwcpy_getConfig(TSE_CFG_ID CfgID)
{
	int ret;
	switch (CfgID) {
		case HWCOPY_CFG_ID_TOTAL_LEN:
			ret = g_tse_hwcpy_obj.cfg.total_len;
			break;
		default:
			ret = E_SYS;
			break;
	}
	return ret;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER tse_open(void)
{
	int ret;
	ret = ioctl(g_tse_ctrl.fd, TSE_IOC_OPEN, 0);
	if (ret < 0) {
		printf("open err %d\r\n", ret);
		return E_SYS;
	}
	g_tse_ctrl.status |= TSE_STS_OPENED;
	return E_OK;
}

ER tse_close(void)
{
	int ret;
	ret = ioctl(g_tse_ctrl.fd, TSE_IOC_CLOSE, 0);
	if (ret < 0) {
		printf("close err %d\r\n", ret);
		return E_SYS;
	}
	g_tse_ctrl.status &= ~TSE_STS_OPENED;
	return E_OK;
}

BOOL tse_isOpened(void)
{
	BOOL ret = FALSE;
	if ((g_tse_ctrl.status & TSE_STS_OPENED) == TSE_STS_OPENED) {
		ret = TRUE;
	}
	return ret;
}

ER tse_start(BOOL bWait, TSE_MODE_NUM OP_MODE)
{
	int ret = 0;
	if (OP_MODE == TSE_MODE_TSMUX) {
		g_tse_mux_obj.obj.bWait = bWait;
		g_tse_mux_obj.obj.OP_MODE = OP_MODE;
		ret = ioctl(g_tse_ctrl.fd, TSE_IOC_MUX_W_CFG, &g_tse_mux_obj);
	} else if (OP_MODE == TSE_MODE_TSDEMUX) {
		g_tse_demux_obj.obj.bWait = bWait;
		g_tse_demux_obj.obj.OP_MODE = OP_MODE;
		ret = ioctl(g_tse_ctrl.fd, TSE_IOC_DEMUX_W_CFG, &g_tse_demux_obj);
	} else if (OP_MODE == TSE_MODE_HWCOPY) {
		g_tse_hwcpy_obj.obj.bWait = bWait;
		g_tse_hwcpy_obj.obj.OP_MODE = OP_MODE;
		ret = ioctl(g_tse_ctrl.fd, TSE_IOC_HWCPY_W_CFG, &g_tse_hwcpy_obj);
	} else {
		ret = E_SYS;
		printf("not support OP_MODE = 0x%.8x\r\n", OP_MODE);
	}
	if (ret < 0) {
		printf("start err %d\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

ER tse_waitDone(void)
{
	/**
		If tse_start()'s bWait is set to FALSE, this API can be used to check if the
		TSE operation is done.
	*/
	return E_OK;
}

ER tse_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	int ret = 0;
	if (CfgID & TSE_CFG_MASK) {
		//ret = tse_cfg_setConfig(CfgID, uiCfgValue);
	} else if (CfgID & TSE_MUX_MASK) {
		ret = tse_mux_setConfig(CfgID, uiCfgValue);
	} else if (CfgID & TSE_DEMUX_MASK) {
		ret = tse_demux_setConfig(CfgID, uiCfgValue);
	} else if (CfgID & TSE_HWCPY_MASK) {
		ret = tse_hwcpy_setConfig(CfgID, uiCfgValue);
	} else {
		ret = E_SYS;
		printf("not support CfgID = 0x%.8x\r\n", CfgID);
	}
	if (ret < 0) {
		printf("setConfig err %d\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

UINT32 tse_getConfig(TSE_CFG_ID CfgID)
{
	UINT32 rt = 0;
	if (CfgID & TSE_CFG_MASK) {
		//rt = tse_cfg_getConfig(CfgID);
	} else if (CfgID & TSE_MUX_MASK) {
		rt = tse_mux_getConfig(CfgID);
	} else if (CfgID & TSE_DEMUX_MASK) {
		rt = tse_demux_getConfig(CfgID);
	} else if (CfgID & TSE_HWCPY_MASK) {
		rt = tse_hwcpy_getConfig(CfgID);
	} else {
		printf("not support CfgID = 0x%.8x\r\n", CfgID);
	}
	return rt;
}

UINT32 tse_getIntStatus(void)
{
	/**
        unsupported.
	*/
	return 0;
}

UINT32 tse_init(void)
{
	g_tse_ctrl.fd = -1;

	g_tse_ctrl.fd = open(_TSE_, O_RDWR);
	if (-1 == g_tse_ctrl.fd) {
		printf("[IO]open() failed with error\r\n");
		return 1;
	}
	return 0;
}

UINT32 tse_uninit(void)
{
	if (g_tse_ctrl.fd != -1) close(g_tse_ctrl.fd);
	return 0;
}

