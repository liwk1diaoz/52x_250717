/**
    DIS Process API

    This file contains the control interface of DIS process.

    @file       dis_alg.c
    @ingroup    mISYSAlg
    @note       N/A

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/


/** \addtogroup mISYSAlg */


//@{
//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#include "kdrv_eth_alg.h"
#include "kdrv_dis.h"
#include "dis_alg_eth.h"



static ER               		       g_ret = E_OK;
UINT32                                 g_egmap_size =  (1920 * 1080)>>2;
KDRV_ETH_IN_PARAM        			   g_eth_input_info = {0};
KDRV_ETH_IN_BUFFER_INFO     		   g_eth_in_addr = {0};
UINT32                                 g_pa_buff_save = 0;
UINT32                                 g_va_buff_save = 0;
extern UINT32                          g_dis_sensor_w;
extern UINT32                          g_dis_sensor_h;


ER dis_eth_set_en(void)
{
	/*UINT32 func = 0;

	DBG_ERR("OPEN+++++++++++func = %d+++++++++++++++\n\r", func);
	g_ret = vendor_dis_init(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/


    DBG_IND("eth_in_info: enable = %d, h_out_sel = %d, v_out_sel = %d, out_bit_sel = %d, out_sel = %d,th_high = %d, th_mid = %d, th_low = %d\n\r ",g_eth_input_info.enable, g_eth_input_info.h_out_sel, g_eth_input_info.v_out_sel, g_eth_input_info.out_bit_sel, g_eth_input_info.out_sel, g_eth_input_info.th_high, g_eth_input_info.th_mid, g_eth_input_info.th_low);
	DBG_IND("eth_in_addr: uiInAdd0 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",g_eth_in_addr.ui_inadd, g_eth_in_addr.buf_size, g_eth_in_addr.frame_cnt);

    g_ret = kdrv_dis_eth_set_dma_out(0, &g_eth_in_addr);
	if (g_ret != E_OK) {
		DBG_ERR("err: ETH set input addr error %d\n", g_ret);
		return E_PAR;
	}

    g_ret = kdrv_dis_eth_set_in_param_info(0, &g_eth_input_info);
	if (g_ret != E_OK) {
		DBG_ERR("err: ETH set input info error %d\n", g_ret);
		return E_PAR;
	}

	/*DBG_ERR("CLOSE------------func = %d---------------\n\r", func);
	g_ret = vendor_dis_uninit(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_uninit error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/

	return E_OK;

}

ER dis_eth_set_un(KDRV_ETH_IN_PARAM eth_input_info)
{
	/*UINT32 func = 0;

	DBG_ERR("OPEN+++++++++++func = %d+++++++++++++++\n\r", func);

	g_ret = vendor_dis_init(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/
    g_ret = kdrv_dis_eth_set_in_param_info(0, &eth_input_info);
	if (g_ret != E_OK) {
		DBG_ERR("err: ETH set input info error %d\n", g_ret);
		return E_PAR;
	}
	/*DBG_ERR("CLOSE-------------func = %d--------------\n\r", func);
	g_ret = vendor_dis_uninit(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_uninit error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/
	return E_OK;

}

ER dis_eth_get_input_info(DIS_ETH_IN_PARAM* p_dis_input_param)
{
	KDRV_ETH_IN_PARAM eth_input_info;
	/*UINT32 func = 0;

	DBG_ERR("OPEN++++++++++func = %d++++++++++++++++\n\r", func);
	g_ret = vendor_dis_init(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/
    g_ret = kdrv_dis_eth_get_in_param_info(0, &eth_input_info);
	if (g_ret != E_OK) {
		DBG_ERR("err: ETH get input info error %d\n", g_ret);
		return E_PAR;
	}

	/*DBG_ERR("CLOSE-----------func = %d----------------\n\r", func);
	g_ret = vendor_dis_uninit(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_uninit error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/
	p_dis_input_param->enable      = eth_input_info.enable;
	p_dis_input_param->h_out_sel   = eth_input_info.h_out_sel;
	p_dis_input_param->v_out_sel   = eth_input_info.v_out_sel;
	p_dis_input_param->out_bit_sel = eth_input_info.out_bit_sel;
	p_dis_input_param->out_sel     = eth_input_info.out_sel;
	p_dis_input_param->th_low      = eth_input_info.th_low;
	p_dis_input_param->th_mid      = eth_input_info.th_mid;
	p_dis_input_param->th_high     = eth_input_info.th_high;

	return E_OK;
}
ER dis_eth_get_out_info(DIS_ETH_OUT_PARAM* p_eth_out_info)
{
	KDRV_ETH_OUT_PARAM eth_out_info;
	/*UINT32 func = 0;

	DBG_ERR("OPEN++++++++++func = %d++++++++++++++++\n\r", func);
	g_ret = vendor_dis_init(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/


	g_ret = kdrv_dis_eth_get_out_info(0, &eth_out_info);
	if (g_ret != E_OK) {
		DBG_ERR("err: ETH get output info error %d\n", g_ret);
		return E_PAR;
	}

	/*DBG_ERR("CLOSE-----------func = %d----------------\n\r", func);

	g_ret = vendor_dis_uninit(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_uninit error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/

	p_eth_out_info->w = eth_out_info.out_size.w;
	p_eth_out_info->h = eth_out_info.out_size.h;
	p_eth_out_info->out_lofs = eth_out_info.out_lofs;

	return E_OK;

}
ER dis_eth_get_out_addr(DIS_ETH_BUFFER_INFO* p_eth_out_addr)
{
	KDRV_ETH_IN_BUFFER_INFO eth_out_addr;

	/*UINT32 func = 0;

	DBG_ERR("OPEN++++++++++++func = %d++++++++++++++\n\r", func);
	g_ret = vendor_dis_init(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/

	g_ret = kdrv_dis_eth_get_dma_out(0, &eth_out_addr);
	if (g_ret != E_OK) {
		DBG_ERR("err: ETH get output buff error %d\n", g_ret);
		return E_PAR;
	}

	/*DBG_ERR("CLOSE------------func = %d---------------\n\r", func);
	g_ret = vendor_dis_uninit(func);
	if (g_ret != HD_OK) {
		DBG_ERR("err:vendor_dis_uninit error %d\r\n",g_ret);
		return HD_ERR_FAIL;
	}*/

	//printf("eth_out_addr: uiInAdd0 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",eth_out_addr.ui_inadd, eth_out_addr.buf_size, eth_out_addr.frame_cnt);

	p_eth_out_addr->buf_size  = eth_out_addr.buf_size;
	if ((eth_out_addr.ui_inadd < g_pa_buff_save) && (eth_out_addr.ui_inadd != 0)){
		DBG_ERR("err:ETH get output buff error: pa 0x%x, pa begin = 0x%x\r\n", eth_out_addr.ui_inadd, g_pa_buff_save);
		return E_PAR;
	} else if (eth_out_addr.ui_inadd == 0) {
	    return E_SYS;
	}

	p_eth_out_addr->ui_inadd  = eth_out_addr.ui_inadd - g_pa_buff_save + g_va_buff_save;
	p_eth_out_addr->frame_cnt = eth_out_addr.frame_cnt;
	return E_OK;
}

ER dis_eth_initialize(DIS_IPC_INIT *p_buf, BOOL subsample_sel)
{
	g_eth_input_info.enable	     = 1;
	g_eth_input_info.h_out_sel   = 0;
	g_eth_input_info.v_out_sel   = 0;
	g_eth_input_info.out_bit_sel = 0;
	g_eth_input_info.out_sel	 = subsample_sel;

	g_eth_input_info.th_low	     = 28;
	g_eth_input_info.th_mid	     = 48;
	g_eth_input_info.th_high	 = 128;


	g_eth_in_addr.ui_inadd	     = p_buf->addr;
	g_eth_in_addr.buf_size	     = g_egmap_size;
	g_eth_in_addr.frame_cnt	     = 0;

	g_pa_buff_save               = g_eth_in_addr.ui_inadd;
	g_va_buff_save				 = p_buf->addr;


	DBG_IND("eth_in_info: enable = %d, h_out_sel = %d, v_out_sel = %d, out_bit_sel = %d, out_sel = %d,th_high = %d, th_mid = %d, th_low = %d\n\r ",g_eth_input_info.enable, g_eth_input_info.h_out_sel, g_eth_input_info.v_out_sel, g_eth_input_info.out_bit_sel, g_eth_input_info.out_sel, g_eth_input_info.th_high, g_eth_input_info.th_mid, g_eth_input_info.th_low);
	DBG_IND("eth_in_addr: uiInAdd0 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",g_eth_in_addr.ui_inadd, g_eth_in_addr.buf_size, g_eth_in_addr.frame_cnt);
	DBG_IND("buff_addr_save: g_pa_buff_save = 0x%x, g_va_buff_save = 0x%x\n\r ",g_pa_buff_save, g_va_buff_save);

	return dis_eth_set_en();

}

ER dis_eth_uninitialize(void)
{
	KDRV_ETH_IN_PARAM eth_input_info;

	eth_input_info.enable	   = 0;
	eth_input_info.h_out_sel   = 0;
	eth_input_info.v_out_sel   = 0;
	eth_input_info.out_bit_sel = 0;
	eth_input_info.out_sel	   = 0;

	eth_input_info.th_low	   = 0;
	eth_input_info.th_mid	   = 0;
	eth_input_info.th_high	   = 0;

	return dis_eth_set_un(eth_input_info);
}

UINT32 dis_eth_get_prvmaxBuffer(void)
{
	UINT32 out_bufsz = 0;
    g_egmap_size =  (g_dis_sensor_w * g_dis_sensor_h)>>2;
    out_bufsz = ALIGN_CEIL_4(g_egmap_size*3);
	return out_bufsz;
}

//@}

