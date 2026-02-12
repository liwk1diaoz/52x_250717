#include "ipe_platform.h"
#include "ipe_api.h"
#include "ipe_kit.h"
#include "kdrv_videoprocess/kdrv_ipp.h"

#define KDRV_IPE_TEST_CODE 1

#if (__IPE_DBG_CMD__ == 1)

extern int nvt_kdrv_ipe_api_test(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);
extern int nvt_ipe_api_dump(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);
void ipe_bypass_test(UINT32 uiaddr);


int nvt_ipe_api_write_reg(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_ipe_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}
#if 1
int nvt_ipe_api_write_pattern(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	return 0;
}
void ipe_bypass_test(UINT32 uiaddr)
{
}
int nvt_ipe_api_read_reg(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	return 0;
}

#else

int nvt_ipe_api_write_pattern(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	//mm_segment_t old_fs;
	//struct file *fp;
	VOS_FILE fp;
	int len = 0;
	//unsigned char *pbuffer;
	frammap_buf_t      buf_info = {0};

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	fp = filp_open(pargv[0], O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

#if 0
	pbuffer = kmalloc(256, GFP_KERNEL);
	if (pbuffer == NULL) {
		filp_close(fp, NULL);
		return -ENOMEM;
	}
#endif


	//Allocate memory
	buf_info.size = 0x600000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "nvtmpp";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);
	nvt_dbg(IND, "buf_info.va_addr = 0x%08x, buf_info.phy_addr = 0x%08x\r\n", (UINT32)buf_info.va_addr, (UINT32)buf_info.phy_addr);

	len = vos_file_read(fp, buf_info.va_addr, 1152 * 64);

	/* Do something after get data from file */
	ipe_bypass_test((UINT32)buf_info.va_addr);

	//kfree(pbuffer);
	frm_free_buf_ddr(buf_info.va_addr);
	vos_file_close(fp);

	return len;
}

void ipe_bypass_test(UINT32 uiaddr)
{
#if 0
	ER er_return = E_OK;
	IPE_OPENOBJ ipe_openobj;
	IPE_MODEINFO g_modeinfo = {0};

	IPE_EEXT_PARAM eext_par;
	INT16 edge_ker[12] = {380, 42, -80, -30, -35, 4, 10, 10, 4, 1, 10, 6};
	UINT8 edir_tab[8] = {8, 8, 8, 8, 8, 8, 8, 8};
	UINT32 tone_remap[65] = {0};
	/*IPE_DIREDG_EEXT dir_edge_eext;
	IPE_DIREDG_SCORECAL dir_edge_scr_cal;
	IPE_DIREDG_SCORETH dir_edge_scr_th;
	IPE_DIREDG_CONPARAM dir_edge_con;
	IPE_EWKERA EWA;
	IPE_EWKERB EWB;
	IPE_EW_PARAM ew_param;*/
	IPE_EPROC_PARAM eproc_param;
	IPE_EDGEMAP_INFOR edgemap_th;
	IPE_ESMAP_INFOR esmap_th;
	UINT8 edlut[16] = {255};
	UINT8 eslut[16] = {64};

	ipe_openobj.FP_IPEISR_CB = NULL;
	ipe_openobj.ipe_clk_sel = 240;
	nvt_dbg(IND, "ipe open...\r\n");
	er_return = ipe_open(&ipe_openobj);
	if (er_return != E_OK) {
		return;
	}

	/*

	//enable interrupt
	g_modeinfo.uiIntEn = IPE_INTE_FRMEND;

	// input info
	g_modeinfo.InInfo.IpeMode = IPE_OPMODE_D2D;
	g_modeinfo.InInfo.InSizeY.uiHSize = 1152;
	g_modeinfo.InInfo.InSizeY.uiVSize = 64;
	g_modeinfo.InInfo.uiAdr0 = uiaddr;
	g_modeinfo.InInfo.uiAdr1 = uiaddr + 1152 * 64;;
	g_modeinfo.InInfo.InFmt = IPE_YONLY;
	g_modeinfo.InInfo.OutFmt = IPE_YONLY;
	g_modeinfo.InInfo.uiLofsIn0 = 1152;
	g_modeinfo.InInfo.uiLofsIn1 = 1152;
	g_modeinfo.InInfo.InRandEn = B_DISABLE;
	g_modeinfo.InInfo.InRandRst = B_ENABLE;
	g_modeinfo.InInfo.bOutYCToDRAM = 1;
	g_modeinfo.InInfo.DmaOutSel = IPE_ORIGINAL;
	g_modeinfo.InInfo.EthOutInfo.EthOutSel = ETH_OUT_ORIGINAL;
	g_modeinfo.InInfo.EthOutInfo.EthOutFmt = ETH_OUT_2BITS;
	g_modeinfo.InInfo.HovlpOpt = HOLAP_AUTO;

	//output info
	g_modeinfo.OutInfo.bOutYCToIME = 0;
	g_modeinfo.OutInfo.bOutYCToDRAM = 1;
	g_modeinfo.OutInfo.YCFormat = IPE_YONLY;
	g_modeinfo.OutInfo.uiAdrY0 = uiaddr + 1152 * 64 + 1152 * 64;
	g_modeinfo.OutInfo.uiAdrC0 = uiaddr + 1152 * 64 + 1152 * 64 + 1152 * 64;
	g_modeinfo.OutInfo.uiLofsY = 1152;
	g_modeinfo.OutInfo.uiLofsC = 1152;
	g_modeinfo.OutInfo.DmaOutSel = IPE_ORIGINAL;
	g_modeinfo.OutInfo.EthOutInfo.EthOutSel = ETH_OUT_ORIGINAL;
	g_modeinfo.OutInfo.EthOutInfo.EthOutFmt = ETH_OUT_2BITS;

	g_modeinfo.IQInfo.uiFuncSel = 0x0;
	g_modeinfo.IQInfo.FuncUpdateSel = IPE_FUNC_SET;
	g_modeinfo.IQInfo.uiParamUpdateSel = IPE_SET_EDGEEXT | IPE_SET_EDGEPROC;

	eext_par.EextGamSel = EEXT_POST_GAM;
	eext_par.EextCHSel = EEXT_Y_CHANNEL;
	eext_par.piEdgeKerDiv = edge_ker;
	eext_par.uiEDirDiv = 2;
	eext_par.puiEDirTab = edir_tab;
	eext_par.puiToneMapLut = tone_remap;
	dir_edge_eext.YDRange = (IPE_YDRANGE)1;
	dir_edge_eext.uiYDMaxTh = 22;
	dir_edge_eext.PndAdjEn = (IPE_ENABLE)0;
	dir_edge_eext.uiPndShft = 3;
	dir_edge_eext.uiES = 2;
	dir_edge_eext.EdirSel = (IPE_KERSEL)0;
	eext_par.pDirEdgEext = &dir_edge_eext;
	dir_edge_scr_cal.iA0 = (INT8)3;
	dir_edge_scr_cal.iB0 = (INT8) - 1;
	dir_edge_scr_cal.iC0 = (INT8) - 1;
	dir_edge_scr_cal.uiD0 = 0;
	dir_edge_scr_cal.iA1 = (INT8)5;
	dir_edge_scr_cal.iB1 = (INT8)1;
	dir_edge_scr_cal.iC1 = (INT8)1;
	dir_edge_scr_cal.uiD1 = 0;
	dir_edge_scr_cal.iA2 = (INT8)3;
	dir_edge_scr_cal.iB2 = (INT8)0;
	dir_edge_scr_cal.iC2 = (INT8) - 1;
	dir_edge_scr_cal.uiD2 = 0;
	eext_par.pDirEdgScrCal = &dir_edge_scr_cal;
	dir_edge_scr_th.uiScoreTh0 = 7;
	dir_edge_scr_th.uiScoreTh1 = 28;
	dir_edge_scr_th.uiScoreTh2 = 3;
	dir_edge_scr_th.uiScoreTh3 = 21;
	eext_par.pDirEdgScrTh = &dir_edge_scr_th;
	eext_par.PowerSav = (IPE_ENABLE)0; //temp
	dir_edge_con.ScrCenter = (IPE_DECONSCR)0;
	dir_edge_con.ScrEdg = (IPE_DECONSCR)1;
	dir_edge_con.ScrDetail = (IPE_DECONSCR)0;
	dir_edge_con.uiCntNumTh = 2;
	dir_edge_con.uiMax1Th = 3;
	dir_edge_con.uiMaxCntTh = 0;
	dir_edge_con.uiMax2Th = 7;
	dir_edge_con.IdxSel = (IPE_DECON_IDXSEL)0;
	eext_par.pDirEdgCon = &dir_edge_con;
	EWA.EWKer2DSel = (IPE_KERSEL)0;
	EWA.uiAdwt0 = 8;
	EWA.uiAdwt1 = 8;
	EWA.uiAdwt2 = 16;
	EWA.uiAdwt3 = 16;
	EWA.uiAdwt4 = 16;
	EWA.uiAdwt5 = 16;
	EWA.iAds0 = (INT8)3;
	EWA.iAds1 = (INT8)3;
	EWA.iAds2 = (INT8)3;
	EWA.iAds3 = (INT8)3;
	EWA.iAds4 = (INT8)3;
	EWA.iAds5 = (INT8)3;
	eext_par.pEWA = &EWA;
	EWB.EwKerBOutSel = (IPE_EW_KBOUTSEL)1;
	EWB.uiBdwt0 = 1;
	EWB.uiBdwt1 = 1;
	EWB.iBds0 = (INT8)2;
	EWB.iBds1 = (INT8)2;
	eext_par.pEWB = &EWB;
	ew_param.DynEMEn = (IPE_ENABLE)0;
	ew_param.uiEWDir0 = 8;
	ew_param.uiEWDir1 = 8;
	ew_param.uiEWDir2 = 8;
	eext_par.pEWParam = &ew_param;
	g_modeinfo.IQInfo.pEextParam = &eext_par;
	edgemap_th.uiEthrLow = 0;
	edgemap_th.uiEthrHigh = 256;
	edgemap_th.uiEtabLow = 0;
	edgemap_th.uiEtabHigh = 5;
	edgemap_th.EinSel = (IPE_EMAP_IN_ENUM)1;
	eproc_param.pEdgeMapTh = &edgemap_th;
	eproc_param.puiEdgeMapLut = edlut;
	esmap_th.uiEthrLow = 0;
	esmap_th.uiEthrHigh = 512;
	esmap_th.uiEtabLow = 5;
	esmap_th.uiEtabHigh = 5;
	eproc_param.pEsMapTh = &esmap_th;
	eproc_param.puiESMapLut = eslut;
	eproc_param.EOutSel = (IPE_EMAP_OUT_ENUM)1;
	g_modeinfo.IQInfo.pEprocParam = &eproc_param;

	*/

	ipe_set_mode(&g_modeinfo);
	nvt_dbg(IND, "ipe setMode...done\r\n");

	ipe_clr_frame_end();

	er_return = ipe_start();
	if (er_return != E_OK) {
		return;
	}
	nvt_dbg(IND, "ipe start...done\r\n");

	ipe_wait_frame_end(IPE_NOCLRFLG);
	nvt_dbg(IND, "ipe wait frame-end...done\r\n");

	ipe_pause();
	nvt_dbg(IND, "ipe pause...done\r\n");

	ipe_close();
	nvt_dbg(IND, "ipe close...done\r\n");
#endif
}


int nvt_ipe_api_read_reg(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0x0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_ipe_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

#endif


int nvt_ipe_set_dbg_cmd(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	kdrv_api_ipe_set_dbg_cmd(argc, pargv);
	return 0;
}


int nvt_ipe_api_dump(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	kdrv_api_ipe_dump_info(argc, pargv);
	return 0;
}


int nvt_ipe_api_iq_test(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	kdrv_api_ipe_iq_debug(argc, pargv);
	return 0;
}

#if !KDRV_IPE_TEST_CODE
int nvt_kdrv_ipe_api_test(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	return 0;
}
#else
int nvt_kdrv_ipe_api_test(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	return kdrv_api_ipe_test(argc, pargv);
}
#endif

#endif
