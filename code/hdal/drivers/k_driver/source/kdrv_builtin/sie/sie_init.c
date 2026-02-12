/**
    SIE module fast boot driver

    @file       sie_init.c
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

// include files for FW
#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "sie_init.h"
#include "sie_lib.h"
#include "sie_int.h"
#include "sie_platform.h"
#include "nvtmpp_init.h"
#include "sie_init_int.h"

SIE_FB_DBG_LVL sie_fb_dbg_lvl = SIE_FB_DBG_LVL_WRN;
#define SIE_FB_MAX_SIE_NUM      5           //fastboot maximum support sie number
#define SIE_FB_RING_BUF_NUM     2           //fastboot sie ouptut ring buffer number
#define SIE_ENG_REG_NUM         0x900

//offset 0x0 ctrl
#define SIE_FB_CTL_OFS          0x0         //ctl reg ofs
#define SIE_FB_CTL_RST_BIT      0x01        //reset bit
#define SIE_FB_CTL_LOAD_BIT     0x02        //load bit
#define SIE_FB_CTL_ACT_EN_BIT   0x04        //active enable bit

//offset 0x4    function
#define SIE_FB_FUNC_OFS         0x4         //function reg ofs
#define SIE_FB_FUNC_IQ_OFF_BIT 0x08808      //function disable bit(OB_AVG_EN bit3, DPC bit11, ECS bit15)

//offset 0x8
#define SIE_FB_INTE_OFS     0x08            //interrupt enable reg ofs
#define SIE_FB_INTE_DFT_BIT SIE_INT_VD|SIE_INT_CROPEND|SIE_INT_DRAM_OUT1_END|SIE_INT_DRAM_OUT2_END  //interrupt enable bit, VD and Crop End, OUT1_END, OUT2_END

#define SIE_SIM_BUF_CB 0

static  VK_DEFINE_SPINLOCK(my_lock);
#define sie_header_loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define sie_header_unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

static volatile UINT32 sie_fb_rdy_addr[SIE_FB_MAX_SIE_NUM][SIE_FB_OUT_CH_MAX][SIE_FB_BUF_IDX_MAX] = {0};
static volatile UINT32 sie_fb_ppb[SIE_FB_MAX_SIE_NUM][SIE_FB_OUT_CH_MAX][SIE_FB_RING_BUF_NUM] = {0};
static SIE_DRAM_SINGLE_OUT sie_fb_sin_out[SIE_FB_MAX_SIE_NUM] = {0};
volatile BOOL ctl_sie_fb_de_flg[SIE_FB_MAX_SIE_NUM] = {TRUE, TRUE};
volatile static UINT32 sie_fb_fc[SIE_FB_MAX_SIE_NUM] = {0};
static VOS_TICK sie_fb_vd_ts[SIE_FB_MAX_SIE_NUM] = {0};
void *sie_reg_base[SIE_FB_MAX_SIE_NUM];
static SIE_BUILTIN_HEADER_INFO sie_fb_header_info[SIE_FB_MAX_SIE_NUM] = {0};
static UINT32 sie_fb_out_dest[SIE_FB_MAX_SIE_NUM] = {0};

typedef struct {
	UINT32 addr;
	UINT32 size;
} SIE_BUILTIN_BLK;

static void (*sie_fb_int_cb)(SIE_ENGINE_ID id, UINT32 status);
static void (*sie_fb_buf_out_cb)(SIE_ENGINE_ID id, SIE_BUILTIN_HEADER_INFO *info);
typedef void (*SIE_FB_INT_ISR_CB)(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info);

#if SIE_SIM_BUF_CB
static void sie_fb_buf_out_cb_sim_common(SIE_ENGINE_ID id, SIE_BUILTIN_HEADER_INFO *info)
{
	if (info != NULL) {
		if (info->buf_ctrl == SIE_BUILTIN_HEADER_CTL_PUSH) {
			nvtmpp_unlock_fastboot_blk(info->buf_addr);
		}
	}

}
#endif

static void sie_fb_dump_init_info(SIE_ENGINE_ID id)
{
	SIE_DRAM_OUT0_INFO out_info;
	SIE_ACT_WIN_INFO act_win;
	SIE_CRP_WIN_INFO crp_win;
	UINT32 func;

	sie_fb_dbg_ind("\r\n------------------------- sie id %d builtin init dump begin ------------------------------\r\n", id);
	sie_getDramOut0(id, &out_info);
	sie_fb_dbg_ind("ch0_addr va: 0x%x,pa: 0x%x, lofs: %d, bitdepth: %d \r\n",
		out_info.uiAddr, (out_info.uiAddr == 0)?0:vos_cpu_get_phy_addr(out_info.uiAddr),	out_info.uiLofs, out_info.PackBus);


	sie_fb_dbg_ind("ring_buf_en: %d, ring_bif_len: %d, H/V flip (%d, %d)\r\n",
		out_info.bRingBufEn, out_info.uiRingBufLen, out_info.bHFlip, out_info.bVFlip);

	sie_getActiveWindow(id, &act_win);
	sie_fb_dbg_ind("act_win: (%d, %d, %d, %d), cfapat: %d\r\n",
		act_win.uiStX,act_win.uiStY,act_win.uiSzX,act_win.uiSzY,act_win.CfaPat);

	sie_getCropWindow(id, &crp_win);
	sie_fb_dbg_ind("crp_win: (%d, %d, %d, %d), cfapat: %d\r\n",
		crp_win.uiStX,crp_win.uiStY,crp_win.uiSzX,crp_win.uiSzY,crp_win.CfaPat);

	func = sie_getFunction(id);
	sie_fb_dbg_ind("function: 0x%x\r\n", func);
	sie_fb_dbg_ind("-------------------------- sie id %d builtin init dump end -------------------------------\r\n\r\n", id);
}

static void sie_fb_set_reg(UINT32 id, UINT32 ofs, UINT32 val)
{
	*(UINT32 *)(sie_reg_base[id] + ofs) = val;
}
#if 0
static void sie_fb_dump_reg(UINT32 id, UINT32 ofs)
{
	DBG_ERR("id: %d,ofs %x, val %x \r\n", id, ofs, *(UINT32 *)(sie_reg_base[id] + ofs));
}
#endif
void sie_fb_upd_timestp(UINT32 id)
{
	unsigned long flags;

	sie_header_loc_cpu(flags);
	vos_perf_mark(&sie_fb_vd_ts[id]);
	sie_header_unl_cpu(flags);
}

ER sie_fb_set_ppb(SIE_ENGINE_ID id, SIE_BUILTIN_INIT_INFO *init_info)
{
	ER rt = E_OK;
	UINT32 vcap_out_addr1 = 0, vcap_out_addr2 = 0;
	UINT32 ch0_size = 0, ch1_size = 0, ch2_size = 0, total_buf_size = 0;
	UINT32 ring_buf_sz = 0, ring_buf_en = 0, out_dest = 0;
	CHAR *dtsi_sie_ctrl_tag[SIE_FB_MAX_SIE_NUM] = {
		"/fastboot/sie/sie1_ctrl",
		"/fastboot/sie/sie2_ctrl",
		"/fastboot/sie/sie3_ctrl",
		"/fastboot/sie/sie4_ctrl",
		"/fastboot/sie/sie5_ctrl",
	};

	if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "out0_sz", &ch0_size, 1) != E_OK) {
		DBG_ERR("Failed to get out_0 size %d\r\n", ch0_size);
		return E_SYS;
	}

	if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "out1_sz", &ch1_size, 1) != E_OK) {
		DBG_ERR("Failed to get out_1 size %d\r\n", ch1_size);
		return E_SYS;
	}

	if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "out2_sz", &ch2_size, 1) != E_OK) {
		DBG_ERR("Failed to get out_2 size\r\n");
		return E_SYS;
	}

	if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "out_dest", &out_dest, 1) != E_OK) {
		DBG_ERR("Failed to get out dest\r\n");
		return E_SYS;
	} else {
		sie_fb_out_dest[id] = out_dest;
	}

	if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "ring_buf_en", &ring_buf_en, 1) != E_OK) {
		DBG_ERR("Failed to get ring buf en\r\n");
		return E_SYS;
	}

	if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "ring_buf_sz", &ring_buf_sz, 1) != E_OK) {
		DBG_ERR("Failed to get ring buf size\r\n");
		return E_SYS;
	}

	total_buf_size = ch0_size + ch1_size + ch2_size;
	if (total_buf_size) {
		vcap_out_addr1 = nvtmpp_get_fastboot_blk(total_buf_size);
		vcap_out_addr2 = nvtmpp_get_fastboot_blk(total_buf_size);
		if ((vcap_out_addr1 == 0) || (vcap_out_addr2 == 0)) {
			DBG_ERR("id=%d,alloc buf sz 0x%x fail\r\n", id, total_buf_size);
			return E_NOMEM;
		} else {
			DBG_DUMP("id=%d,alloc buf sz 0x%x ok (0x%x,0x%x)\r\n", id, total_buf_size, (unsigned int)vcap_out_addr1, (unsigned int)vcap_out_addr2);
		}

		sie_fb_ppb[id][SIE_FB_OUT_BASE][0] = vcap_out_addr1;
		sie_fb_ppb[id][SIE_FB_OUT_BASE][1] = vcap_out_addr2;

		if (ch0_size != 0) {    // raw (dram mode only)
			sie_fb_sin_out[id].SingleOut0En = TRUE;
			sie_fb_ppb[id][SIE_FB_OUT_CH0][0] = vcap_out_addr1;
			sie_fb_ppb[id][SIE_FB_OUT_CH0][1] = vcap_out_addr2;
		}

		if (ch1_size != 0) {    // CA
			sie_fb_sin_out[id].SingleOut1En = TRUE;
			sie_fb_ppb[id][SIE_FB_OUT_CH1][0] = vcap_out_addr1 + ch0_size;
			sie_fb_ppb[id][SIE_FB_OUT_CH1][1] = vcap_out_addr2 + ch0_size;
		}

		if (ch2_size != 0) {    // LA
			sie_fb_sin_out[id].SingleOut2En = TRUE;
			sie_fb_ppb[id][SIE_FB_OUT_CH2][0] = vcap_out_addr1 + ch0_size + ch1_size;
			sie_fb_ppb[id][SIE_FB_OUT_CH2][1] = vcap_out_addr2 + ch0_size + ch1_size;
		}
	}

	//SIE2 set ring buffer
	if (id == SIE_ENGINE_ID_2 && out_dest == 0 && ring_buf_en == 1) {
		if (init_info->ring_buf_blk_size >= ring_buf_sz) {
			sie_fb_ppb[id][SIE_FB_OUT_CH0][0] = init_info->ring_buf_blk_addr;
			sie_fb_ppb[id][SIE_FB_OUT_CH0][1] = init_info->ring_buf_blk_addr;
			sie_fb_sin_out[id].SingleOut0En = TRUE;
		} else {
			DBG_ERR("init ring buf fail, need %x > input %x\r\n", (unsigned int)ring_buf_sz, (unsigned int)init_info->ring_buf_blk_size);
			rt = E_NOMEM;
		}
	}
	return rt;
}

void sie_fb_get_rdy_addr(SIE_ENGINE_ID id, UINT32 *addr_ch0, UINT32 *addr_ch1, UINT32 *addr_ch2)
{
	if (addr_ch0 != NULL) {
		*addr_ch0 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_RDY];
	}

	if (addr_ch1 != NULL) {
		*addr_ch1 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_RDY];
	}

	if (addr_ch2 != NULL) {
		*addr_ch2 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH2][SIE_FB_BUF_IDX_RDY];
	}
}

void sie_fb_buf_release(UINT32 id, BOOL cb_trig, UINT32 status, SIE_FB_BUF_IDX rls_buf_idx)
{
	unsigned long flags;
	SIE_DRAM_OUT0_INFO out_info;
	SIE_CRP_WIN_INFO crp_win;
	UINT32 func, sts;
	VOS_TICK ts;
	if (cb_trig && sie_fb_int_cb) {
		sie_fb_int_cb(id, status);
	}

	if (sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][rls_buf_idx] != 0) {
		if (sie_fb_out_dest[id] == 0 || sie_fb_buf_out_cb == NULL) {
			// direct or NULL buf_cb, sie unlock buffer
			nvtmpp_unlock_fastboot_blk(sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][rls_buf_idx]);
		} else if (sie_fb_out_dest[id] == 1) {
			// dram mode, ipp unlock buffer (raw/ca/la)
			if (sie_fb_buf_out_cb) {
				sie_header_loc_cpu(flags);
				sie_fb_header_info[id].buf_ctrl = SIE_BUILTIN_HEADER_CTL_PUSH;
				sie_fb_header_info[id].buf_addr = sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][rls_buf_idx];
				sie_fb_header_info[id].addr_ch0 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][rls_buf_idx];
				sie_fb_header_info[id].addr_ch1 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][rls_buf_idx];
				sie_fb_header_info[id].timestamp = sie_fb_vd_ts[id];

				sie_getDramOut0(id, &out_info);
				sie_getCropWindow(id, &crp_win);
				func = sie_getFunction(id);
				sts = sie_getIntrStatus(id);
				vos_perf_mark(&ts);
				sie_fb_dbg_ind("[SIE FB] buf_rls id: %d, out_addr va:0x%x(pa:0x%x), func: 0x%x, int_sts: 0x%x, crp_win(%d, %d, %d, %d),lofs: %d, buf_ctl: %d, ts: %d\r\n",
					id,	sie_fb_header_info[id].addr_ch0, (sie_fb_header_info[id].addr_ch0 == 0)?0:vos_cpu_get_phy_addr(sie_fb_header_info[id].addr_ch0),
					func, sts,
					crp_win.uiStX, crp_win.uiStY, crp_win.uiSzX, crp_win.uiSzY,
					out_info.uiLofs, sie_fb_header_info[id].buf_ctrl, ts);

				sie_fb_buf_out_cb(id, &sie_fb_header_info[id]);
				sie_header_unl_cpu(flags);
			}
		}
		sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][rls_buf_idx] = 0;
	}
}

void sie_fb_ring_buf_release(UINT32 id)
{
	if (sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_CUR_OUT] != 0) {
		nvtmpp_unlock_fastboot_blk(sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_CUR_OUT]);
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_CUR_OUT] = 0;
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_RDY] = 0;
	}
}

void sie_fb_common_isr(SIE_ENGINE_ID id, UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	SIE_CHG_OUT_ADDR_INFO addr = {0};
	unsigned long flags;
	SIE_DRAM_OUT0_INFO out_info;
	SIE_CRP_WIN_INFO crp_win;
	UINT32 func, sts;
	SIE_DRAM_SINGLE_OUT sin_out;
	VOS_TICK ts;

	sie_header_loc_cpu(flags);

	//update ping pong buffer
	if (status & SIE_INT_DRAM_OUT1_END) {   //set ready adderss
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_RDY] = sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_CUR_OUT];
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_CH1][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];    //for hdal buf release
	}

	if (status & SIE_INT_DRAM_OUT2_END) {   //set ready adderss
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH2][SIE_FB_BUF_IDX_RDY] = sie_fb_rdy_addr[id][SIE_FB_OUT_CH2][SIE_FB_BUF_IDX_CUR_OUT];
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH2][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_CH2][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];    //for hdal buf release
	}

	if (status & SIE_INT_CROPEND) { //set ready adderss
		ctl_sie_fb_de_flg[id] = TRUE;
		sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_RDY] = sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_CUR_OUT];
		sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_BASE][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];    //for hdal buf release

		sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_RDY] = sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_CUR_OUT];
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_CH0][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];    //for hdal buf release

		sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_RDY] = sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_CUR_OUT];

		sie_fb_header_info[id].buf_addr = sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_RDY];
		sie_fb_header_info[id].addr_ch0 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_RDY];
		sie_fb_header_info[id].addr_ch1 = sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_RDY];
		sie_fb_header_info[id].count = sie_fb_fc[id];
		sie_fb_header_info[id].timestamp = sie_fb_vd_ts[id];
	}

	if (status & SIE_INT_VD) {
		if (ctl_sie_fb_de_flg[id] == FALSE) {
			sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_RDY] = sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_CUR_OUT];
		}
		ctl_sie_fb_de_flg[id] = FALSE;
		sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_BASE][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH0][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_CH0][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH1][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_CH1][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];
		sie_fb_rdy_addr[id][SIE_FB_OUT_CH2][SIE_FB_BUF_IDX_CUR_OUT] = sie_fb_ppb[id][SIE_FB_OUT_CH2][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];
		sie_fb_fc[id]++;
		addr.uiOut0Addr = sie_fb_ppb[id][SIE_FB_OUT_CH0][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];
		addr.uiOut1Addr = sie_fb_ppb[id][SIE_FB_OUT_CH1][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];
		addr.uiOut2Addr = sie_fb_ppb[id][SIE_FB_OUT_CH2][sie_fb_fc[id] % SIE_FB_RING_BUF_NUM];

		sie_chgParam(id, (void *)&sie_fb_sin_out[id], SIE_CHG_SINGLEOUT);   //set single out enable

		vos_perf_mark(&ts);
		sie_getDramOut0(id, &out_info);
		sie_getDramSingleOut(id, &sin_out);
		sie_getCropWindow(id, &crp_win);
		func = sie_getFunction(id);
		sie_fb_dbg_func("[SIE FB] vd: id: %d, next_pa: 0x%x, cur_addr va:0x%x(pa:0x%x), func: 0x%x,crp_win(%d, %d, %d, %d) lofs: %d,sin_out: %d, ts: %d\r\n",
			id, (addr.uiOut0Addr == 0)?0:vos_cpu_get_phy_addr(addr.uiOut0Addr),
			out_info.uiAddr, (out_info.uiAddr == 0)?0:vos_cpu_get_phy_addr(out_info.uiAddr), func,
			crp_win.uiStX, crp_win.uiStY, crp_win.uiSzX, crp_win.uiSzY,
			out_info.uiLofs, sin_out.SingleOut0En, ts);

		sie_chgParam(id, (void *)&addr, SIE_CHG_OUT_ADDR);  //update output address
		if (sie_fb_buf_out_cb) {
			vos_perf_mark(&sie_fb_vd_ts[id]);
		}

		if (info != NULL) {
			if (info->bDramIn1Udfl || info->bDramIn2Udfl || info->bDramOut0Ovfl || info->bDramOut1Ovfl || info->bDramOut2Ovfl) {
				DBG_ERR("id %d in_udfl(%d,%d) out_ovfl(%d,%d,%d)\r\n", id
					, info->bDramIn1Udfl, info->bDramIn2Udfl, info->bDramOut0Ovfl, info->bDramOut1Ovfl, info->bDramOut2Ovfl);
			}
		}
	}

	sie_header_unl_cpu(flags);

	//callback to interrupt cb_fp(isp)
	if (sie_fb_int_cb) {
		sie_fb_int_cb(id, status);
	}

	//release buffer
	if (status & SIE_INT_CROPEND) {
		if (sie_fb_out_dest[id] == 0 || sie_fb_buf_out_cb == NULL) {
			// direct mode, sie unlock buffer (ca/la)
			if (status & FLGPTN_SIE_CRPEND_VDLATISR) {
				nvtmpp_unlock_fastboot_blk(sie_fb_rdy_addr[id][SIE_FB_OUT_BASE][SIE_FB_BUF_IDX_RDY]);
			}
		} else if (sie_fb_out_dest[id] == 1) {
			if (sie_fb_buf_out_cb) {
				if (status & FLGPTN_SIE_CRPEND_VDLATISR) {
					sie_fb_header_info[id].buf_ctrl = SIE_BUILTIN_HEADER_CTL_PUSH;
				} else {
					sie_fb_header_info[id].buf_ctrl = SIE_BUILTIN_HEADER_CTL_LOCK;
				}

				vos_perf_mark(&ts);
				sie_getDramOut0(id, &out_info);
				sie_getCropWindow(id, &crp_win);
				func = sie_getFunction(id);
				sts = sie_getIntrStatus(id);
				if (sie_fb_header_info[id].buf_ctrl == SIE_BUILTIN_HEADER_CTL_PUSH) {
					sie_fb_dbg_ind("[SIE FB] ce: id: %d, out_addr va: 0x%x(pa: 0x%x),func_en: 0x%x, int_sts: 0x%x, crp_win(%d, %d, %d, %d), lofs: %d, buf_ctl: %d, fc: %d, ts: %d\r\n",
						id,	sie_fb_header_info[id].addr_ch0, (sie_fb_header_info[id].addr_ch0 == 0)?0:vos_cpu_get_phy_addr(sie_fb_header_info[id].addr_ch0),
						func, sts,
						crp_win.uiStX, crp_win.uiStY, crp_win.uiSzX, crp_win.uiSzY,
						out_info.uiLofs,
						sie_fb_header_info[id].buf_ctrl, sie_fb_header_info[id].count, ts);
				} else {
					sie_fb_dbg_func("[SIE FB] ce: id: %d, out_addr va: 0x%x(pa: 0x%x),func_en: 0x%x, int_sts: 0x%x, crp_win(%d, %d, %d, %d), lofs: %d, buf_ctl: %d, fc: %d, ts: %d\r\n",
						id,	sie_fb_header_info[id].addr_ch0, (sie_fb_header_info[id].addr_ch0 == 0)?0:vos_cpu_get_phy_addr(sie_fb_header_info[id].addr_ch0),
						func, sts,
						crp_win.uiStX, crp_win.uiStY, crp_win.uiSzX, crp_win.uiSzY,
						out_info.uiLofs,
						sie_fb_header_info[id].buf_ctrl, sie_fb_header_info[id].count, ts);
				}

				sie_fb_buf_out_cb(id, &sie_fb_header_info[id]);
			}
		}
	}
}

static void sie1_fb_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	sie_fb_common_isr(SIE_ENGINE_ID_1, status, info);
}

static void sie2_fb_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	sie_fb_common_isr(SIE_ENGINE_ID_2, status, info);
}

static void sie3_fb_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	sie_fb_common_isr(SIE_ENGINE_ID_3, status, info);
}

static void sie4_fb_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	sie_fb_common_isr(SIE_ENGINE_ID_4, status, info);
}

static void sie5_fb_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	sie_fb_common_isr(SIE_ENGINE_ID_5, status, info);
}

static UINT32 sie_fb_id_bit = 0;
static BOOL sie_fb_id_chk(UINT32 id)
{
	if ((sie_fb_id_bit) & (1 << id)) {
		return TRUE; // is fastboot id
	}
	return FALSE; // not fastboot id
}

ER sie_builtin_init(SIE_BUILTIN_INIT_INFO *info)
{
	SIE_CHG_OUT_ADDR_INFO addr = {0};
	UINT32 sie_reg_num = 0;
	UINT32 mclk_id = 0, mclk_src = 0, mclk_en = 0, mclk_rate = 0, id = 0, i = 0;
	UINT32 *reg_value;
	UINT32 eng_ctl_reg[SIE_FB_MAX_SIE_NUM] = {0}; //0x00
	UINT32 dupl_src_id[SIE_FB_MAX_SIE_NUM] = {0};
	SIE_OPENOBJ sie_op_obj = {0};
	const SIE_FB_INT_ISR_CB sie_isr_fp[] = {
		sie1_fb_isr,
		sie2_fb_isr,
		sie3_fb_isr,
		sie4_fb_isr,
		sie5_fb_isr,
	};
	SIE_BUILTIN_BLK sie_reg_base_info[SIE_FB_MAX_SIE_NUM] = {
		{0xF0C00000, SIE_ENG_REG_NUM},
		{0xF0D20000, SIE_ENG_REG_NUM},
		{0xF0D30000, SIE_ENG_REG_NUM},
		{0xF0D40000, SIE_ENG_REG_NUM},
		{0xF0D80000, SIE_ENG_REG_NUM},
	};
	CHAR *dtsi_sie_dbg_tag = "/fastboot/sie/dbg";
	CHAR *dtsi_sie_clk_tag[SIE_FB_MAX_SIE_NUM] = {
		"/fastboot/sie/sie1_clk",
		"/fastboot/sie/sie2_clk",
		"/fastboot/sie/sie3_clk",
		"/fastboot/sie/sie4_clk",
		"/fastboot/sie/sie5_clk",
	};
	CHAR *dtsi_sie_reg_tag[SIE_FB_MAX_SIE_NUM] = {
		"/fastboot/sie/sie1_reg",
		"/fastboot/sie/sie2_reg",
		"/fastboot/sie/sie3_reg",
		"/fastboot/sie/sie4_reg",
		"/fastboot/sie/sie5_reg",
	};

	CHAR *dtsi_sie_ctrl_tag[SIE_FB_MAX_SIE_NUM] = {
		"/fastboot/sie/sie1_ctrl",
		"/fastboot/sie/sie2_ctrl",
		"/fastboot/sie/sie3_ctrl",
		"/fastboot/sie/sie4_ctrl",
		"/fastboot/sie/sie5_ctrl",
	};

#if SIE_SIM_BUF_CB
	if (sie_fb_reg_buf_out_cb(sie_fb_buf_out_cb_sim_common) != E_OK) {
		DBG_ERR("sie_fb_reg_buf_out_cb fail\r\n");
	}
#endif

	//get debug ctl
	if (sie_init_plat_chk_node(dtsi_sie_dbg_tag) == E_OK) {
		if (sie_init_plat_read_dtsi_array(dtsi_sie_dbg_tag, "msg_en", &sie_fb_dbg_lvl, 1) != E_OK) {
			sie_fb_dbg_lvl = SIE_FB_DBG_LVL_WRN;
		}
	}

	// update sie_fb_id_bit
	sie_fb_id_bit |= (info->sie_id_bit); // main sie id
	for (id = 0; id < SIE_FB_MAX_SIE_NUM; id++) {
		if (sie_init_plat_chk_node(dtsi_sie_clk_tag[id]) == E_OK) {
			if (sie_init_plat_read_dtsi_array(dtsi_sie_ctrl_tag[id], "dupl_src_id", &dupl_src_id[id], 1) != E_OK) {
				DBG_ERR("Failed to read SIE %d dupl_src_id\r\n", (int)id + 1);
				return E_SYS;
			} else if ((info->sie_id_bit) & (1 << dupl_src_id[id])) {
				sie_fb_id_bit |= (1 << id); // duplicate sie id
			}
		}
	}
	sie_fb_dbg_ind("sie_fb_id_bit = 0x%x\r\n", (unsigned int)sie_fb_id_bit);

	for (id = 0; id < SIE_FB_MAX_SIE_NUM; id++) {
		if ((sie_init_plat_chk_node(dtsi_sie_clk_tag[id]) == E_OK) && sie_fb_id_chk(id))  {
			sie_reg_isr_cb(id, sie_isr_fp[id]);

			if (sie_fb_set_ppb(id, info) != E_OK) {
				return E_SYS;
			}

			//set clock
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "pxclk_src", &sie_op_obj.PxClkSel, 1) != E_OK) {
				DBG_ERR("Failed to read SIE %d pxclk sel\r\n", (int)id + 1);
				return E_SYS;
			}
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "clk_src", &sie_op_obj.SieClkSel, 1) != E_OK) {
				DBG_ERR("Failed to read sie clk sel\r\n");
				return E_SYS;
			}
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "clk_rate", &sie_op_obj.uiSieClockRate, 1) != E_OK) {
				DBG_ERR("Failed to read sie clk rate\r\n");
				return E_SYS;
			}
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "mclk_id", &mclk_id, 1) != E_OK) {
				DBG_ERR("Failed to read mclk id\r\n");
				return E_SYS;
			}
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "mclk_en", &mclk_en, 1) != E_OK) {
				DBG_ERR("Failed to read mclk en\r\n");
				return E_SYS;
			}
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "mclk_src", &mclk_src, 1) != E_OK) {
				DBG_ERR("Failed to read mclk src\r\n");
				return E_SYS;
			}
			if (sie_init_plat_read_dtsi_array(dtsi_sie_clk_tag[id], "mclk_rate", &mclk_rate, 1) != E_OK) {
				DBG_ERR("Failed to read mclk rate\r\n");
				return E_SYS;
			}

			//set sie mclk/clk/pxclk
			sie_platform_enable_clk(id);    //enable clock/mclk/pxclk
			sie_platform_set_clk_rate(id, &sie_op_obj);//set clock/pxclk_src
			//set mclk
			if (mclk_en) {
				if (mclk_id == 1) {
					sie_setmclock(mclk_src, mclk_rate, mclk_en);
				} else if (mclk_id == 2) {
					sie_setmclock2(mclk_src, mclk_rate, mclk_en);
				} else {
					sie_setmclock3(mclk_src, mclk_rate, mclk_en);
				}
			}
			//set sram shutdown
			sie_platform_disable_sram_shutdown(id);
			//set sie register (parsing from dtsi)
			sie_reg_base[id] = ioremap_nocache(sie_reg_base_info[id].addr, sie_reg_base_info[id].size);

			if (sie_init_plat_read_dtsi_array(dtsi_sie_reg_tag[id], "reg_num", &sie_reg_num, 1) != E_OK) {
				DBG_ERR("Failed to read sie reg num\r\n");
				return E_SYS;
			}

			if (sie_fb_ppb[id][SIE_FB_OUT_BASE][0] == 0) {
				reg_value = (UINT32 *)kdrv_sie_builtin_plat_malloc(sie_reg_num * 2 * sizeof(UINT32)); // 2 : addr & value
			} else {
				reg_value = (UINT32 *)sie_fb_ppb[id][SIE_FB_OUT_BASE][0];
			}

			if (reg_value == NULL) {
				DBG_ERR("NULL for reg_value\r\n");
				return E_SYS;
			}

			if (sie_init_plat_read_dtsi_array(dtsi_sie_reg_tag[id], "reg_start", &reg_value[0], sie_reg_num * 2) != E_OK) {
				DBG_ERR("Failed to read sie reg\r\n");
				if (sie_fb_ppb[id][SIE_FB_OUT_BASE][0] == 0) {
					kdrv_sie_builtin_plat_free((void *)reg_value);
				}
				return E_SYS;
			}

			for (i = 0; i < sie_reg_num; i++) {
				if (reg_value[i * 2] == SIE_FB_CTL_OFS) {
					reg_value[i * 2 + 1] &= ~SIE_FB_CTL_ACT_EN_BIT;
					reg_value[i * 2 + 1] |= SIE_FB_CTL_RST_BIT;
					eng_ctl_reg[id] = reg_value[i * 2 + 1];
				}
				//enable VD/CROP_END interrupt enable for flow ctrl
				if (reg_value[i * 2] == SIE_FB_INTE_OFS) {
					reg_value[i * 2 + 1] |= SIE_FB_INTE_DFT_BIT;
				}
				//disable iq function
				if (reg_value[i * 2] == SIE_FB_FUNC_OFS) {
					reg_value[i * 2 + 1] &= ~SIE_FB_FUNC_IQ_OFF_BIT;
				}
				sie_fb_set_reg(id, reg_value[i * 2], reg_value[i * 2 + 1]);
			}
			//set first frame output address
			addr.uiOut0Addr = sie_fb_ppb[id][SIE_FB_OUT_CH0][0];
			addr.uiOut1Addr = sie_fb_ppb[id][SIE_FB_OUT_CH1][0];
			addr.uiOut2Addr = sie_fb_ppb[id][SIE_FB_OUT_CH2][0];
			sie_chgParam(id, (void *)&addr, SIE_CHG_OUT_ADDR);  //first frame output address
			sie_chgParam(id, (void *)&sie_fb_sin_out[id], SIE_CHG_SINGLEOUT);

			if (sie_fb_ppb[id][SIE_FB_OUT_BASE][0] == 0) {
				kdrv_sie_builtin_plat_free((void *)reg_value);
			}

			//reset flg for isp
			if (sie_fb_int_cb) {
				sie_fb_int_cb(id, SIE_INT_ALL);
			}

			sie_fb_dump_init_info(id);
		}
	}

	for (id = SIE_FB_MAX_SIE_NUM; id > 0; id--) {
		if ((sie_init_plat_chk_node(dtsi_sie_clk_tag[id - 1]) == E_OK) && sie_fb_id_chk(id - 1)) {
			//trigger sie register
			eng_ctl_reg[id - 1] &= ~SIE_FB_CTL_RST_BIT;
			sie_fb_set_reg(id - 1, SIE_FB_CTL_OFS, eng_ctl_reg[id - 1]); //reset release
			eng_ctl_reg[id - 1] |= SIE_FB_CTL_ACT_EN_BIT | SIE_FB_CTL_LOAD_BIT;
			sie_fb_set_reg(id - 1, SIE_FB_CTL_OFS, eng_ctl_reg[id - 1]); //active enable and load
		}
	}
	return E_OK;
}

ER sie_fb_reg_isr_Cb(SIE_FB_ISR_FP fp)
{
	if (fp) {
		sie_fb_int_cb = fp;
		return E_OK;
	}
	return E_NOEXS;
}

ER sie_fb_reg_buf_out_cb(SIE_FB_BUF_OUT_FP fp)
{
	if (fp) {
		sie_fb_buf_out_cb = fp;
		return E_OK;
	}
	return E_NOEXS;
}
#endif

//@}
