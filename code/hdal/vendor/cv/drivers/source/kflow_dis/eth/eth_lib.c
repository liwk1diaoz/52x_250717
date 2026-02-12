//=============================================================================
// include
//=============================================================================
#include "dis_alg_platform.h"
#include "eth_alg_lib.h"
#include "kdrv_eth_alg.h"
#include "dis_alg_task.h"


//=============================================================================
// define
//=============================================================================
#define DIS_ETH_REG_NAME             "NVT_DIS_ETH"
#define DIS_ETH_SIE_REG_EVENT        ISP_EVENT_SIE_VD
#define DIS_ETH_IPP_REG_EVENT        ISP_EVENT_IPP_CFGSTART | ISP_EVENT_IPP_PROCEND

extern SEM_HANDLE  g_kdrv_eth_sem;


#ifdef SIE_ETH
CTL_SIE_ISP_ETH_PARAM g_eth_param = {0,0,100,{0,0,100,100},{0, 0, 0, 0, 0, 0,0,0},0};
CTL_SIE_ISP_ETH_PARAM g_eth_get_param = {0};
#else
CTL_IPP_ISP_ETH g_eth_param = {0};
CTL_IPP_ISP_ETH g_rdy_eth = {0};
#endif

ETH_IN_BUFFER_INFO g_eth_in_buffer = {0};
UINT32             g_eth_frame_cnt = 0;
UINT32             g_eth_out_lofs = 0;

//=============================================================================
// function declaration
//=============================================================================
#ifdef SIE_ETH
static INT32 dis_eth_api_cb_sie(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param);
#else
static INT32 dis_eth_api_cb_ipp(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param);
#endif

//=============================================================================
// extern functions
//=============================================================================
int dis_eth_api_reg(void)
{
	ER ret = E_OK;
	#ifdef SIE_ETH
	ret = ctl_sie_isp_evt_fp_reg(DIS_ETH_REG_NAME, &dis_eth_api_cb_sie, DIS_ETH_SIE_REG_EVENT, CTL_SIE_ISP_CB_MSG_NONE);
	#else
	ret = ctl_ipp_isp_evt_fp_reg(DIS_ETH_REG_NAME, &dis_eth_api_cb_ipp, DIS_ETH_IPP_REG_EVENT, CTL_IPP_ISP_CB_MSG_NONE);
	#endif
	if (ret != E_OK){
		DBG_ERR("DIS_ETH: isp_evt_fp_reg error: %d \r\n", ret);
		return ret;
	}
	return ret;
}

int dis_eth_api_unreg(void)
{
	ER ret = E_OK;
	#ifdef SIE_ETH
	ret = ctl_sie_isp_evt_fp_unreg(DIS_ETH_REG_NAME);
	#else
	ret = ctl_ipp_isp_evt_fp_unreg(DIS_ETH_REG_NAME);
	#endif
	if (ret != E_OK){
		DBG_ERR("DIS_ETH: isp_evt_fp_unreg error: %d \r\n", ret);
		return ret;
	}
	return ret;
}

static UINT32 dis_eth_util_ethsize(UINT32 *w, UINT32 *h, BOOL out_bit_sel, BOOL subsample_en, UINT32 *out_lofs)
{
	UINT32 lofs;
	UINT32 eth_size;
	UINT32 width = *w;
	UINT32 height = *h;

	if (width == 0 || height == 0) {
		return 0;
	}

	/* 2/8 bit output */
	if (out_bit_sel == 0) {
		lofs = width >> 2;
	} else {
		lofs = width;
	}

	/* subsample, w/2, h/2 */
	if (subsample_en == 0) {
		eth_size = lofs * height;
	} else {
		lofs = ALIGN_CEIL_4(lofs >> 1);
		eth_size = lofs * (height >> 1);
		width = ALIGN_CEIL_4(width >> 1);
		height = ALIGN_CEIL_4(height >> 1);
	}

	*out_lofs = lofs;
	*w = width;
	*h = height;

	return eth_size;
}

#ifdef SIE_ETH
static INT32 dis_eth_api_cb_sie(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	static UINT32 eth_cnt = 1;
	ER ret = E_OK;

	#ifdef  DUMP_ETH
	static char filename[100];
	struct file *p_fp;
	mm_segment_t old_fs;
	#endif

	switch(evt) {
	case ISP_EVENT_SIE_VD:
		{
			//get sie eth out info
			ret = ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_ETH, (void *)&g_eth_get_param);

			if (ret != E_OK){
				DBG_ERR("DIS_ETH: CTL_SIE get eth error: %d \r\n", ret);
				return ret;
			}

			//set eth out
			g_eth_param.buf_addr = g_eth_in_buffer.buf_addr[eth_cnt % ETH_BUFFER_NUM];
			g_eth_param.buf_size = g_eth_in_buffer.buf_size;
			eth_cnt++;

			if(eth_cnt == ETH_BUFFER_NUM+1){
				eth_cnt = 1;
			}

			ret = ctl_sie_isp_set(id, CTL_SIE_ISP_ITEM_ETH, (void *)&g_eth_param);
			if (ret != E_OK){
				DBG_ERR("DIS_ETH: CTL_SIE set eth error: %d \r\n", ret);
				return ret;
			}
			g_eth_frame_cnt = frame_cnt;
			g_eth_out_lofs = g_eth_get_param.out_lofs;
			g_eth_get_param.buf_size = dis_eth_util_ethsize(g_eth_get_param.crop_roi.w, g_eth_get_param.crop_roi.h, g_eth_get_param.eth_info.out_bit_sel, g_eth_get_param.eth_info.out_sel, &g_eth_get_param.out_lofs);

			#ifdef ETH_DEBUG
			DBG_ERR("\n\r  **************cnt = %d g_frame_cnt = %d*********************\n\r", eth_cnt, frame_cnt);

			DBG_ERR("rdy_eth: en %d, buf(0x%.8x, 0x%.8x), bit_sel %d, out_sel %d, size(%d %d %d), th(%d %d %d)\r\n",
				g_eth_get_param.eth_info.enable, g_eth_get_param.buf_addr, g_eth_get_param.buf_size, g_eth_get_param.eth_info.out_bit_sel, g_eth_get_param.eth_info.out_sel,
				g_eth_get_param.crop_roi.w, g_eth_get_param.crop_roi.h, g_eth_get_param.out_lofs, g_eth_get_param.eth_info.th_low, g_eth_get_param.eth_info.th_mid, g_eth_get_param.eth_info.th_high);

			DBG_ERR("cur_eth: en %d, buf(0x%.8x, 0x%.8x), bit_sel %d, out_sel %d, size(%d %d %d), th(%d %d %d)\r\n",
				g_eth_param.eth_info.enable, g_eth_param.buf_addr, g_eth_param.buf_size, g_eth_param.eth_info.out_bit_sel, g_eth_param.eth_info.out_sel,
				g_eth_param.crop_roi.w, g_eth_param.crop_roi.h, g_eth_param.out_lofs, g_eth_param.eth_info.th_low, g_eth_param.eth_info.th_mid, g_eth_param.eth_info.th_high);
			#endif
			#ifdef  DUMP_ETH
			sprintf(filename, "/mnt/sd/DIS/outBin/SIE/alg_eth_%d_0x%x.raw", frame_cnt,g_eth_get_param.buf_addr);
			p_fp = filp_open(filename, O_CREAT|O_WRONLY|O_SYNC , 0);
			if (IS_ERR_OR_NULL(p_fp)) {
				DBG_ERR("failed in file open:%s\n", filename);
				return -EFAULT;
			}
			old_fs = get_fs();
			set_fs(get_ds());
			vfs_write(p_fp, (char*)g_eth_get_param.buf_addr, g_eth_get_param.buf_size, &p_fp->f_pos);
			filp_close(p_fp, NULL);
			set_fs(old_fs);
			#endif
		}
		break;

	default:
		DBG_ERR("DIS_ETH: ISP_EVENT out of range (%d) \n", evt);
		break;
	}
	return 0;
}
#else

static INT32 dis_eth_api_cb_ipp(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
    static UINT32 eth_cnt = 1;
	ER            ret;

	#ifdef DUMP_ETH
	static char filename[100];
	int fp;
	//mm_segment_t old_fs;
	#endif

	switch(evt) {

	case ISP_EVENT_IPP_PROCEND:
		/* get ready eth and set next eth param */
		{
		    SEM_SIGNAL(g_kdrv_eth_sem);
			ret = ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&g_rdy_eth);
			if (ret != E_OK){
				DBG_ERR("DIS_ETH: CTL_IPE get eth error: %d \r\n", ret);
				return ret;
			}

			if (g_eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_ORIGINAL){
				g_eth_param.buf_addr[0] = g_eth_in_buffer.buf_addr[eth_cnt % ETH_BUFFER_NUM];
				g_eth_param.buf_size[0] = g_eth_in_buffer.buf_size;
			}
			if (g_eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE){
				g_eth_param.buf_addr[1] = g_eth_in_buffer.buf_addr[eth_cnt % ETH_BUFFER_NUM];
				g_eth_param.buf_size[1] = g_eth_in_buffer.buf_size;
			}
			eth_cnt++;

			if(eth_cnt == ETH_BUFFER_NUM+1){
				eth_cnt = 1;
			}
			ret = ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&g_eth_param);
			if (ret != E_OK){
				DBG_ERR("DIS_ETH: CTL_IPE set eth error: %d \r\n", ret);
				return ret;
			}
			g_eth_frame_cnt = frame_cnt;
			dis_eth_util_ethsize(&g_rdy_eth.out_size.w, &g_rdy_eth.out_size.h, g_rdy_eth.out_bit_sel, g_rdy_eth.out_sel, &g_eth_out_lofs);

			#ifdef ETH_DEBUG
			DBG_ERR("\n\r  **************cnt = %d g_frame_cnt = %d*********************\n\r", eth_cnt, frame_cnt);

			DBG_ERR("rdy_eth: en %d, buf0(0x%.8x, 0x%.8x)(0x%.8x, 0x%.8x),bit_sel %d, out_sel %d, size(%d %d), th(%d %d %d)\r\n",
				g_rdy_eth.enable, g_rdy_eth.buf_addr[0], g_rdy_eth.buf_size[0], g_rdy_eth.buf_addr[1], g_rdy_eth.buf_size[1], g_rdy_eth.out_bit_sel, g_rdy_eth.out_sel,
				g_rdy_eth.out_size.w, g_rdy_eth.out_size.h, g_rdy_eth.th_low, g_rdy_eth.th_mid, g_rdy_eth.th_high);

			DBG_ERR("cur_eth: en %d, buf0(0x%.8x, 0x%.8x)(0x%.8x, 0x%.8x), bit_sel %d, out_sel %d, size(%d %d), th(%d %d %d)\r\n",
				g_eth_param.enable, g_eth_param.buf_addr[0], g_eth_param.buf_size[0], g_eth_param.buf_addr[1], g_eth_param.buf_size[1], g_eth_param.out_bit_sel, g_eth_param.out_sel,
				g_eth_param.out_size.w, g_eth_param.out_size.h, g_eth_param.th_low, g_eth_param.th_mid, g_eth_param.th_high);
			#endif

			#ifdef DUMP_ETH
			sprintf(filename, "/mnt/sd/DIS/outBin/IPE/alg_eth_%d_0x%x_%d.raw", frame_cnt,g_rdy_eth.buf_addr[0], g_rdy_eth.buf_size[0]);
            fp = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);
            if ((VOS_FILE)(-1) == fp) {
                nvt_dbg(ERR, "open %s failure\r\n", filename);
                return -1;
            }

            vos_file_write(fp, (void *)g_rdy_eth.buf_addr[0],g_rdy_eth.buf_size[0]);
            vos_file_close(fp);

            /*
            p_fp = filp_open(filename, O_CREAT|O_WRONLY|O_SYNC , 0);
			if (IS_ERR_OR_NULL(p_fp)) {
				DBG_ERR("failed in file open:%s\n", filename);
				return -EFAULT;
			}
			old_fs = get_fs();
			set_fs(get_ds());
			vfs_write(p_fp, (char*)g_rdy_eth.buf_addr[0], g_rdy_eth.buf_size[0], &p_fp->f_pos);
			filp_close(p_fp, NULL);
			set_fs(old_fs);*/
			#endif

            dis_alg_task_trigger();
		}
		break;
    case ISP_EVENT_IPP_CFGSTART:
        break;
	default:
		DBG_ERR("DIS_ETH:ISP_EVENT out of range (%d) \n", evt);
	break;
	}
	return 0;
}
#endif
