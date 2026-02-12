#include "ImageApp_MovieMulti_int.h"

/// ========== Cross file variables ==========
BOOL g_ia_moviemulti_trace_on = TRUE;
/// ========== Noncross file variables ==========

#define DEBUG_SXCMD             ENABLE

static BOOL _ImageApp_MovieMulti_SXCmd_OTrace(unsigned char argc, char **argv)
{
	g_ia_moviemulti_trace_on = TRUE;
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SXCmd_CTrace(unsigned char argc, char **argv)
{
	g_ia_moviemulti_trace_on = FALSE;
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_DumpImgLink(unsigned char argc, char **argv)
{
	UINT32 i = 0, j = MaxLinkInfo.MaxImgLink, k;

	if (argc == 1) {
		sscanf_s(argv[0], "%d", &i);
		j = i + 1;
	}
	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_WRN("id(%d) > MaxLinkInfo.MaxImgLink(%d), force set to 0\r\n", i, MaxLinkInfo.MaxImgLink);
		i = 0;
		j = MaxLinkInfo.MaxImgLink;
	}

	for (; i < j; i++) {
		DBG_DUMP("ImgLink[%d]:\r\n", i);
		// vdocap
		DBG_DUMP(" vcap_p[0]         (0x%08x) = %d\r\n", ImgLink[i].vcap_p[0], ImgLinkStatus[i].vcap_p[0]);
		// vdoprc
		for (k = 0;k < VPROC_PER_LINK; k++) {
			DBG_DUMP(" vproc_p_phy[%d][0] (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p_phy[k][0], ImgLinkStatus[i].vproc_p_phy[k][0]);
			DBG_DUMP(" vproc_p_phy[%d][1] (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p_phy[k][1], ImgLinkStatus[i].vproc_p_phy[k][1]);
			DBG_DUMP(" vproc_p_phy[%d][2] (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p_phy[k][2], ImgLinkStatus[i].vproc_p_phy[k][2]);
			DBG_DUMP(" vproc_p_phy[%d][3] (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p_phy[k][3], ImgLinkStatus[i].vproc_p_phy[k][3]);
			DBG_DUMP(" vproc_p_phy[%d][4] (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p_phy[k][4], ImgLinkStatus[i].vproc_p_phy[k][4]);
			DBG_DUMP(" vproc_p[%d][0]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][0], ImgLinkStatus[i].vproc_p[k][0]);
			DBG_DUMP(" vproc_p[%d][1]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][1], ImgLinkStatus[i].vproc_p[k][1]);
			DBG_DUMP(" vproc_p[%d][2]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][2], ImgLinkStatus[i].vproc_p[k][2]);
			DBG_DUMP(" vproc_p[%d][3]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][3], ImgLinkStatus[i].vproc_p[k][3]);
			DBG_DUMP(" vproc_p[%d][4]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][4], ImgLinkStatus[i].vproc_p[k][4]);
			DBG_DUMP(" vproc_p[%d][5]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][5], ImgLinkStatus[i].vproc_p[k][5]);
			DBG_DUMP(" vproc_p[%d][6]     (0x%08x) = %d\r\n", k, ImgLink[i].vproc_p[k][6], ImgLinkStatus[i].vproc_p[k][6]);
		}
		// vdoenc
		DBG_DUMP(" venc_p[0]         (0x%08x) = %d\r\n", ImgLink[i].venc_p[0], ImgLinkStatus[i].venc_p[0]);
		DBG_DUMP(" venc_p[1]         (0x%08x) = %d\r\n", ImgLink[i].venc_p[1], ImgLinkStatus[i].venc_p[1]);
		// bsmux
		DBG_DUMP(" bsmux_p[0]        (0x%08x) = %d\r\n", ImgLink[i].bsmux_p[0], ImgLinkStatus[i].bsmux_p[0]);
		DBG_DUMP(" bsmux_p[1]        (0x%08x) = %d\r\n", ImgLink[i].bsmux_p[1], ImgLinkStatus[i].bsmux_p[1]);
		DBG_DUMP(" bsmux_p[2]        (0x%08x) = %d\r\n", ImgLink[i].bsmux_p[2], ImgLinkStatus[i].bsmux_p[2]);
		DBG_DUMP(" bsmux_p[3]        (0x%08x) = %d\r\n", ImgLink[i].bsmux_p[3], ImgLinkStatus[i].bsmux_p[3]);
		// file out
		DBG_DUMP(" fout_p[0]         (0x%08x) = %d\r\n", ImgLink[i].fileout_p[0], ImgLinkStatus[i].fileout_p[0]);
		DBG_DUMP(" fout_p[1]         (0x%08x) = %d\r\n", ImgLink[i].fileout_p[1], ImgLinkStatus[i].fileout_p[1]);
		DBG_DUMP(" fout_p[2]         (0x%08x) = %d\r\n", ImgLink[i].fileout_p[2], ImgLinkStatus[i].fileout_p[2]);
		DBG_DUMP(" fout_p[3]         (0x%08x) = %d\r\n", ImgLink[i].fileout_p[3], ImgLinkStatus[i].fileout_p[3]);
	}
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_DumpDispLink(unsigned char argc, char **argv)
{
	UINT32 i = 0, j = MaxLinkInfo.MaxDispLink;

	if (argc == 1) {
		sscanf_s(argv[0], "%d", &i);
		j = i + 1;
	}
	if (i >= MaxLinkInfo.MaxDispLink) {
		DBG_WRN("id(%d) > MaxLinkInfo.MaxDispLink(%d), force set to 0\r\n", i, MaxLinkInfo.MaxDispLink);
		i = 0;
		j = MaxLinkInfo.MaxDispLink;
	}

	for (; i < j; i++) {
		DBG_DUMP("DispLink[%d]:\r\n", i);
		// vdoput
		DBG_DUMP(" vout_p[0]        (0x%08x) = %d\r\n", DispLink[i].vout_p[0], DispLinkStatus[i].vout_p[0]);
	}
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_DumpWifiLink(unsigned char argc, char **argv)
{
	UINT32 i = 0, j = MaxLinkInfo.MaxWifiLink;

	if (argc == 1) {
		sscanf_s(argv[0], "%d", &i);
		j = i + 1;
	}
	if (i >= MaxLinkInfo.MaxWifiLink) {
		DBG_WRN("id(%d) > MaxLinkInfo.MaxWifiLink(%d), force set to 0\r\n", i, MaxLinkInfo.MaxWifiLink);
		i = 0;
		j = MaxLinkInfo.MaxWifiLink;
	}

	for (; i < j; i++) {
		DBG_DUMP("WifiLink[%d]:\r\n", i);
		// vdoenc
		DBG_DUMP(" venc_p[0]        (0x%08x) = %d\r\n", WifiLink[i].venc_p[0], WifiLinkStatus[i].venc_p[0]);
	}
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_DumpAudCapLink(unsigned char argc, char **argv)
{
	UINT32 i = 0, j = MaxLinkInfo.MaxAudCapLink;

	for (; i < j; i++) {
		DBG_DUMP("AudCapLink[%d]:\r\n", i);
		// audcap
		DBG_DUMP(" acap_p[0]        (0x%08x) = %d\r\n", AudCapLink[i].acap_p[0], AudCapLinkStatus[i].acap_p[0]);
		// audenc
		DBG_DUMP(" aenc_p[0]        (0x%08x) = %d\r\n", AudCapLink[i].aenc_p[0], AudCapLinkStatus[i].aenc_p[0]);
	}
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_DumpEthCamLink(unsigned char argc, char **argv)
{
	UINT32 i = 0, j = MaxLinkInfo.MaxEthCamLink;

	if (argc == 1) {
		sscanf_s(argv[0], "%d", &i);
		j = i + 1;
	}
	if (i >= MaxLinkInfo.MaxEthCamLink) {
		DBG_WRN("id(%d) > MaxLinkInfo.MaxEthCamLink(%d), force set to 0\r\n", i, MaxLinkInfo.MaxEthCamLink);
		i = 0;
		j = MaxLinkInfo.MaxEthCamLink;
	}

	for (; i < j; i++) {
		DBG_DUMP("EthCamLink[%d]:\r\n", i);
		// vdoprc
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (ethcam_vproc_vpe_en[i]) {
			DBG_DUMP(" vproc_p_phy[0]    (0x%08x) = %d\r\n", EthCamLink[i].vproc_p_phy[0], EthCamLinkStatus[i].vproc_p_phy[0]);
			DBG_DUMP(" vproc_p[0]        (0x%08x) = %d\r\n", EthCamLink[i].vproc_p[0], EthCamLinkStatus[i].vproc_p[0]);
		}
#endif
		// vdoenc
		DBG_DUMP(" venc_p[0]         (0x%08x) = %d\r\n", EthCamLink[i].venc_p[0], EthCamLinkStatus[i].venc_p[0]);
		// vdodec
		DBG_DUMP(" vdec_p[0]         (0x%08x) = %d\r\n", EthCamLink[i].vdec_p[0], EthCamLinkStatus[i].vdec_p[0]);
		// bsmux
		DBG_DUMP(" bsmux_p[0]        (0x%08x) = %d\r\n", EthCamLink[i].bsmux_p[0], EthCamLinkStatus[i].bsmux_p[0]);
		DBG_DUMP(" bsmux_p[1]        (0x%08x) = %d\r\n", EthCamLink[i].bsmux_p[1], EthCamLinkStatus[i].bsmux_p[1]);
		// file out
		DBG_DUMP(" fout_p[0]         (0x%08x) = %d\r\n", EthCamLink[i].fileout_p[0], EthCamLinkStatus[i].fileout_p[0]);
		DBG_DUMP(" fout_p[1]         (0x%08x) = %d\r\n", EthCamLink[i].fileout_p[1], EthCamLinkStatus[i].fileout_p[1]);
	}
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_DumpVCapStatus(unsigned char argc, char **argv)
{
	UINT32 i = 0, j = MaxLinkInfo.MaxImgLink;
	HD_VIDEOCAP_SYSINFO info;

	if (argc == 1) {
		sscanf_s(argv[0], "%d", &i);
		j = i + 1;
	}
	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_WRN("id(%d) > MaxLinkInfo.MaxImgLink(%d), force set to 0\r\n", i, MaxLinkInfo.MaxImgLink);
		i = 0;
		j = MaxLinkInfo.MaxImgLink;
	}

	for (; i < j; i++) {
		if (ImageApp_MovieMulti_GetVcapSysInfo(i, &info) != E_OK) {
			DBG_ERR("ImageApp_MovieMulti_GetVcapSysInfo(%d) fail.\r\n", i);
		} else {
			DBG_DUMP("VCap id%d:\r\n", i);
			DBG_DUMP("  vd_count = %lld\r\n", info.vd_count);
			DBG_DUMP("  cur_dim  = (%d, %d)\r\n", info.cur_dim.w, info.cur_dim.h);
		}
	}
	return TRUE;
}

#if (DEBUG_SXCMD == ENABLE)
static BOOL _ImageApp_MovieMulti_SxCmd_RecStart(unsigned char argc, char **argv)
{
	ImageApp_MovieMulti_RecStart(_CFG_REC_ID_1);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_RecStop(unsigned char argc, char **argv)
{
	ImageApp_MovieMulti_RecStop(_CFG_REC_ID_1);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_AudRecStart(unsigned char argc, char **argv)
{
	_ImageApp_MovieMulti_AudRecStart(0);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_AudRecStop(unsigned char argc, char **argv)
{
	_ImageApp_MovieMulti_AudRecStop(0);
	_ImageApp_MovieMulti_AudReopenAEnc(0);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_StrmStart(unsigned char argc, char **argv)
{
	ImageApp_MovieMulti_ImgLinkForStreaming(_CFG_REC_ID_1, ENABLE, TRUE);
	ImageApp_MovieMulti_StreamingStart(_CFG_STRM_ID_1);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_StrmStop(unsigned char argc, char **argv)
{
	ImageApp_MovieMulti_StreamingStop(_CFG_STRM_ID_1);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_Snapshot(unsigned char argc, char **argv)
{
	ImageApp_MovieMulti_TriggerSnapshot(_CFG_REC_ID_1);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_SetFmtFree(unsigned char argc, char **argv)
{
	CHAR drive = IAMOVIEMULTI_FM_DRV_FIRST;
	UINT32 is_format_free = TRUE;
	ImageApp_MovieMulti_SetFormatFree(drive, is_format_free);
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	ImageApp_MovieMulti_SetExtStrType(drive, FILEDB_FMT_RAW); //Support set ext str with RAW
#endif
	ImageApp_MovieMulti_SetUseHiddenFirst(drive, TRUE); //Support use hidden first
	ImageApp_MovieMulti_SetSyncTime(drive, TRUE); //Support sync mtime
	ImageApp_MovieMulti_SetSkipReadOnly(drive, TRUE); //Support skip read-only
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_SetFldInfo(unsigned char argc, char **argv)
{
	CHAR   fld_full_path[128] = {0};
	UINT32 fld_ratio = 0;
	UINT64 fixed_size = 0;
	//Set "Movie"
	snprintf(fld_full_path, sizeof(fld_full_path), "A:\\Novatek\\Movie\\");
	fld_ratio = 70;
	fixed_size = 190 * 1024 * 1024ULL;
	ImageApp_MovieMulti_SetFolderInfo(fld_full_path, fld_ratio, fixed_size);
	ImageApp_MovieMulti_SetCyclicRec(fld_full_path, TRUE);
	//Set "EMR"
	snprintf(fld_full_path, sizeof(fld_full_path), "A:\\Novatek\\EMR\\");
	fld_ratio = 20;
	fixed_size = 190 * 1024 * 1024ULL;
	ImageApp_MovieMulti_SetFolderInfo(fld_full_path, fld_ratio, fixed_size);
	ImageApp_MovieMulti_SetCyclicRec(fld_full_path, TRUE);
	//Set "Photo"
	snprintf(fld_full_path, sizeof(fld_full_path), "A:\\Novatek\\Photo\\");
	fld_ratio = 10;
	fixed_size = 30 * 1024 * 1024ULL;
	ImageApp_MovieMulti_SetFolderInfo(fld_full_path, fld_ratio, fixed_size);
	ImageApp_MovieMulti_SetCyclicRec(fld_full_path, TRUE);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_GetFldInfo(unsigned char argc, char **argv)
{
	CHAR   fld_full_path[128] = {0};
	UINT32 fld_ratio = 0;
	UINT64 fixed_size = 0;
	//Set "Movie"
	snprintf(fld_full_path, sizeof(fld_full_path), "A:\\Novatek\\Movie\\");
	ImageApp_MovieMulti_GetFolderInfo(fld_full_path, &fld_ratio, &fixed_size);
	DBG_DUMP("Get %s ratio: %d f_size %lld\r\n", fld_full_path, fld_ratio, fixed_size);
	//Set "EMR"
	snprintf(fld_full_path, sizeof(fld_full_path), "A:\\Novatek\\EMR\\");
	ImageApp_MovieMulti_GetFolderInfo(fld_full_path, &fld_ratio, &fixed_size);
	DBG_DUMP("Get %s ratio: %d f_size %lld\r\n", fld_full_path, fld_ratio, fixed_size);
	//Set "Photo"
	snprintf(fld_full_path, sizeof(fld_full_path), "A:\\Novatek\\Photo\\");
	ImageApp_MovieMulti_GetFolderInfo(fld_full_path, &fld_ratio, &fixed_size);
	DBG_DUMP("Get %s ratio: %d f_size %lld\r\n", fld_full_path, fld_ratio, fixed_size);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_FMAllocate(unsigned char argc, char **argv)
{
	CHAR drive = IAMOVIEMULTI_FM_DRV_FIRST;
	ImageApp_MovieMulti_FMAllocate(drive);
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_FMScan(unsigned char argc, char **argv)
{
	CHAR drive = IAMOVIEMULTI_FM_DRV_FIRST;
	MOVIEMULTI_DISK_INFO disk_info = {0};
	ImageApp_MovieMulti_FMScan(drive, &disk_info);
	//dump
	{
		UINT32 i;
		UINT32 dir_num = disk_info.dir_num;
		DBG_DUMP("Disk [%c] dir num %d max depth %d\r\n", drive, dir_num, disk_info.dir_depth);
		DBG_DUMP(">> total space %lld, free spze %lld, other file size %lld\r\n",
			disk_info.disk_size,
			disk_info.free_size,
			disk_info.other_size);
		for (i = 0; i < dir_num; i++) {
			if (!disk_info.dir_info[i].is_use) {
				continue;
			}
			DBG_DUMP("Dir [%s]\r\n", disk_info.dir_info[i].dir_path);
			DBG_DUMP(">> total size %lld, ratio %d, file num %d, avg size %lld\r\n",
				disk_info.dir_info[i].dir_size,
				disk_info.dir_info[i].dir_ratio_of_disk,
				disk_info.dir_info[i].file_num,
				disk_info.dir_info[i].file_avg_size);
			if (disk_info.dir_info[i].is_falloc) {
				DBG_DUMP(">> falloc size %lld\r\n",
					disk_info.dir_info[i].falloc_size);
			}
		}
		DBG_DUMP("FormatFree State: %d\r\n", disk_info.is_format_free);
	}
	return TRUE;
}

static BOOL _ImageApp_MovieMulti_SxCmd_FMDump(unsigned char argc, char **argv)
{
	CHAR drive = IAMOVIEMULTI_FM_DRV_FIRST;
	ImageApp_MovieMulti_FMDump(drive);
	return TRUE;
}
#endif

static SXCMD_BEGIN(iamovie_cmd_tbl, "iamovie_cmd_tbl")
SXCMD_ITEM("ot", _ImageApp_MovieMulti_SXCmd_OTrace, "Trace code open")
SXCMD_ITEM("ct", _ImageApp_MovieMulti_SXCmd_CTrace, "Trace code cancel")
SXCMD_ITEM("dumpimglink", _ImageApp_MovieMulti_SxCmd_DumpImgLink, "Dump ImgLink Status")
SXCMD_ITEM("dumpdisplink", _ImageApp_MovieMulti_SxCmd_DumpDispLink, "Dump DispLink Status")
SXCMD_ITEM("dumpwifilink", _ImageApp_MovieMulti_SxCmd_DumpWifiLink, "Dump WifiLink Status")
SXCMD_ITEM("dumpaudcaplink", _ImageApp_MovieMulti_SxCmd_DumpAudCapLink, "Dump AudCapLink Status")
SXCMD_ITEM("dumpethcamlink", _ImageApp_MovieMulti_SxCmd_DumpEthCamLink, "Dump EthCamLink Status")
SXCMD_ITEM("vcapstatus", _ImageApp_MovieMulti_SxCmd_DumpVCapStatus, "Dump VCap Status")
#if (DEBUG_SXCMD == ENABLE)
SXCMD_ITEM("recstart", _ImageApp_MovieMulti_SxCmd_RecStart, "Start record")
SXCMD_ITEM("recstop", _ImageApp_MovieMulti_SxCmd_RecStop, "Stop record")
SXCMD_ITEM("arecstart", _ImageApp_MovieMulti_SxCmd_AudRecStart, "Audio start record")
SXCMD_ITEM("arecstop", _ImageApp_MovieMulti_SxCmd_AudRecStop, "Audio stop record")
SXCMD_ITEM("strmstart", _ImageApp_MovieMulti_SxCmd_StrmStart, "Start streaming")
SXCMD_ITEM("strmstop", _ImageApp_MovieMulti_SxCmd_StrmStop, "Stop streaming")
SXCMD_ITEM("snapshot", _ImageApp_MovieMulti_SxCmd_Snapshot, "Snapshot")
SXCMD_ITEM("setfmtfree", _ImageApp_MovieMulti_SxCmd_SetFmtFree, "Set Format Free")
SXCMD_ITEM("setfldinfo", _ImageApp_MovieMulti_SxCmd_SetFldInfo, "Set Folder Info")
SXCMD_ITEM("getfldinfo", _ImageApp_MovieMulti_SxCmd_GetFldInfo, "Get Folder Info")
SXCMD_ITEM("fmallocate", _ImageApp_MovieMulti_SxCmd_FMAllocate, "FM Allocate")
SXCMD_ITEM("fmscan", _ImageApp_MovieMulti_SxCmd_FMScan, "FM Scan")
SXCMD_ITEM("fmdump", _ImageApp_MovieMulti_SxCmd_FMDump, "FM Dump")
#endif
SXCMD_END()

static int iamovie_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(iamovie_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "movie (ImageApp_MovieMulti)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", iamovie_cmd_tbl[loop].p_name, iamovie_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(movie, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(iamovie_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		iamovie_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], iamovie_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = iamovie_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}

