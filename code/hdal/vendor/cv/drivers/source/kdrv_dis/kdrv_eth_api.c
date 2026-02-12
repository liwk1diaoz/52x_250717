/**
    @file       kdrv_dis.c
    @ingroup    Predefined_group_name

    @brief      dis device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "dis_platform.h"
#include "kdrv_eth.h"
#include "eth_lib.h"


KDRV_DIS_ETH_PRAM g_kdrv_dis_eth_parm[KDRV_DIS_HANDLE_MAX_NUM];
BOOL              g_kdrv_dis_eth_buff_set_flag = FALSE;
BOOL              g_kdrv_dis_eth_set_reg = FALSE;
#ifdef SIE_ETH
extern CTL_SIE_ISP_ETH_PARAM g_eth_param;
extern CTL_SIE_ISP_ETH_PARAM g_eth_get_param;
#else
extern CTL_IPP_ISP_ETH g_eth_param;
extern CTL_IPP_ISP_ETH g_rdy_eth;
#endif
extern ETH_IN_BUFFER_INFO g_eth_in_buffer;
extern UINT32             g_eth_out_lofs;
extern UINT32             g_eth_frame_cnt;

#if DIS_IOREMAP_IN_KERNEL
	UINT32 g_in_dma_add_pa = 0;
	UINT32 g_in_dma_add_va = 0;
#endif

SEM_HANDLE  g_kdrv_eth_sem;


ER kdrv_dis_eth_set_in_param_info(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	if(g_kdrv_dis_eth_buff_set_flag == FALSE){
		DBG_ERR("KDRV DIS: please set eth dma first!\r\n");
		return E_PAR;
	}

	g_kdrv_dis_eth_parm[id].in_param_info= *(KDRV_ETH_IN_PARAM*)p_data;

	#ifdef SIE_ETH
		g_eth_param.eth_info.enable      = g_kdrv_dis_eth_parm[id].in_param_info.enable;
		g_eth_param.eth_info.h_out_sel   = g_kdrv_dis_eth_parm[id].in_param_info.h_out_sel;
		g_eth_param.eth_info.v_out_sel   = g_kdrv_dis_eth_parm[id].in_param_info.v_out_sel;
		g_eth_param.eth_info.out_bit_sel = g_kdrv_dis_eth_parm[id].in_param_info.out_bit_sel;
		g_eth_param.eth_info.out_sel     = g_kdrv_dis_eth_parm[id].in_param_info.out_sel;
		g_eth_param.eth_info.th_low      = g_kdrv_dis_eth_parm[id].in_param_info.th_low;
		g_eth_param.eth_info.th_mid      = g_kdrv_dis_eth_parm[id].in_param_info.th_mid;
		g_eth_param.eth_info.th_high     = g_kdrv_dis_eth_parm[id].in_param_info.th_high;
		if (g_eth_param.eth_info.enable ) {
			SEM_CREATE(g_kdrv_eth_sem, 1);
			return dis_eth_api_reg();
		}
		else {
			SEM_DESTROY(g_kdrv_eth_sem);
			return dis_eth_api_unreg();
		}
	#else
		g_eth_param.enable      = g_kdrv_dis_eth_parm[id].in_param_info.enable;
		g_eth_param.h_out_sel   = g_kdrv_dis_eth_parm[id].in_param_info.h_out_sel;
		g_eth_param.v_out_sel   = g_kdrv_dis_eth_parm[id].in_param_info.v_out_sel;
		g_eth_param.out_bit_sel = g_kdrv_dis_eth_parm[id].in_param_info.out_bit_sel;
		g_eth_param.out_sel     = g_kdrv_dis_eth_parm[id].in_param_info.out_sel;
		g_eth_param.th_low      = g_kdrv_dis_eth_parm[id].in_param_info.th_low;
		g_eth_param.th_mid      = g_kdrv_dis_eth_parm[id].in_param_info.th_mid;
		g_eth_param.th_high     = g_kdrv_dis_eth_parm[id].in_param_info.th_high;
		if (g_eth_param.enable ) {
            if(g_kdrv_dis_eth_set_reg == FALSE) {
                g_kdrv_dis_eth_set_reg = TRUE;
				SEM_CREATE(g_kdrv_eth_sem, 1);
				return dis_eth_api_reg();
            }
		}
		else{
            if(g_kdrv_dis_eth_set_reg == TRUE) {
                g_kdrv_dis_eth_set_reg = FALSE;
				SEM_DESTROY(g_kdrv_eth_sem);
				dis_eth_api_unreg();
				#if DIS_IOREMAP_IN_KERNEL
					dis_platform_pa2va_unmap(g_in_dma_add_va, g_in_dma_add_pa);
				#endif
				return dis_eth_api_unreg();
            }
		}
	#endif
	
	return E_OK;
}

ER kdrv_dis_eth_set_dma_out(UINT32 id, void* p_data)
{
	UINT32 i = 0;
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	g_kdrv_dis_eth_parm[id].in_dma_info= *(KDRV_ETH_IN_BUFFER_INFO*)p_data;

	if (g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd == 0) {
		DBG_ERR("KDRV ETH:uiInAdd0 should not be zero\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd & 0x3) {
		DBG_ERR("KDRV DIS: uiInAdd0  should be word alignment\r\n");
		return E_PAR;
	}
	
#if DIS_IOREMAP_IN_KERNEL
	g_in_dma_add_pa = g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd;
	g_in_dma_add_va = dis_platform_pa2va_remap(g_in_dma_add_pa, g_kdrv_dis_eth_parm[id].in_dma_info.buf_size*ETH_BUFFER_NUM, 0);
	for(i = 0; i < ETH_BUFFER_NUM; i++){
		g_eth_in_buffer.buf_addr[i]  = g_in_dma_add_va + i*g_kdrv_dis_eth_parm[id].in_dma_info.buf_size;
    }
#else
    for(i = 0; i < ETH_BUFFER_NUM; i++){
		g_eth_in_buffer.buf_addr[i]  = g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd + i*g_kdrv_dis_eth_parm[id].in_dma_info.buf_size;
    }
#endif
	g_eth_in_buffer.buf_size     = g_kdrv_dis_eth_parm[id].in_dma_info.buf_size;
	
	g_kdrv_dis_eth_buff_set_flag = TRUE;

	return E_OK;
}

ER kdrv_dis_eth_get_in_param_info(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	*(KDRV_ETH_IN_PARAM*)p_data = g_kdrv_dis_eth_parm[id].in_param_info;
	return E_OK;
}

ER kdrv_dis_eth_get_dma_out(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	SEM_WAIT(g_kdrv_eth_sem);

	#ifdef SIE_ETH
		g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd = g_eth_get_param.buf_addr;
		g_kdrv_dis_eth_parm[id].in_dma_info.buf_size = g_eth_get_param.buf_size;
	#else
		if (g_eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_ORIGINAL){
			g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd = g_rdy_eth.buf_addr[0];
			g_kdrv_dis_eth_parm[id].in_dma_info.buf_size = g_rdy_eth.buf_size[0];
		}
		if (g_eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE){
			g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd = g_rdy_eth.buf_addr[1];
			g_kdrv_dis_eth_parm[id].in_dma_info.buf_size = g_rdy_eth.buf_size[1];
		}	
	#endif
	
#if DIS_IOREMAP_IN_KERNEL
	if (g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd){
		g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd = dis_platform_va2pa(g_kdrv_dis_eth_parm[id].in_dma_info.ui_inadd);
	}
#endif

	g_kdrv_dis_eth_parm[id].in_dma_info.frame_cnt   = g_eth_frame_cnt;
	
	*(KDRV_ETH_IN_BUFFER_INFO*)p_data = g_kdrv_dis_eth_parm[id].in_dma_info;
	return E_OK;
}

ER kdrv_dis_eth_get_out_info(UINT32 id, void* p_data)
{
	static KDRV_ETH_OUT_PARAM *p_ethresult;
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	p_ethresult = (KDRV_ETH_OUT_PARAM*)p_data;

	#ifdef SIE_ETH
		p_ethresult->out_size.w = g_eth_get_param.crop_roi.w;
		p_ethresult->out_size.h = g_eth_get_param.crop_roi.h;
	#else
		p_ethresult->out_size.w =	g_rdy_eth.out_size.w;
		p_ethresult->out_size.h =	g_rdy_eth.out_size.h;
	#endif

	p_ethresult->out_lofs	 =	 g_eth_out_lofs;

	return E_OK;
}
