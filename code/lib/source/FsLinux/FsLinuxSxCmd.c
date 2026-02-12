#include "FsLinuxAPI.h"
#include "FsLinuxDebug.h"
#include "FsLinuxSxCmd.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "hdal.h"

static UINT32 temp_pa;

static UINT32 SxCmd_GetTempMem(UINT32 size)
{
	CHAR                 name[]= "FsTmp";
	void                 *va;
	HD_RESULT            ret;
    HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

    if(temp_pa) {
    	DBG_ERR("need release buf\r\n");
        return 0;
    }
	ret = hd_common_mem_alloc(name, &temp_pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id));
		return 0;
	}

	DBG_IND("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(va));

	return (UINT32)va;
}

static UINT32 SxCmd_RelTempMem(UINT32 addr)
{
	HD_RESULT            ret;

	DBG_IND("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(addr));

    if(!temp_pa||!addr){
        DBG_ERR("not allo\r\n");
        return HD_ERR_UNINIT;
    }
	ret = hd_common_mem_free(temp_pa, (void *)addr);
	if (ret != HD_OK) {
		DBG_ERR("err:free temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(addr), (unsigned int)(addr));
		return HD_ERR_NG;
	}else {
        temp_pa =0;
	}
    return HD_OK;
}

static BOOL FsLinux_SxCmd_Format(unsigned char argc, char **argv)
{
	FS_HANDLE   StrgDXH = 0;
	FileSys_GetStrgObj(&StrgDXH);
	if(!StrgDXH)
	{
	    DBG_ERR("No storage to format!\r\n");
	    return TRUE;
	}
	FileSys_FormatAndLabel('A', StrgDXH, FALSE, NULL);

	return TRUE;
}


static BOOL FsLinux_SxCmd_Benchmark(unsigned char argc, char **argv)
{
	UINT32 pBuf, BufSize;

	// benchmark size should over 16MB
	BufSize = 0x1800000;
	pBuf = SxCmd_GetTempMem(BufSize);
	if (!pBuf) {
		DBG_ERR("pBuf is NULL\r\n");
		return TRUE;
	}

	FsLinux_Benchmark('A', 0, (UINT8 *)pBuf, BufSize);
	SxCmd_RelTempMem(pBuf);

	return TRUE;
}

static BOOL FsLinux_SxCmd_ShowInfo(unsigned char argc, char **argv)
{
	DBG_DUMP("DiskReady   = %lld\r\n", FsLinux_GetDiskInfo('A', FST_INFO_DISK_RDY));
	DBG_DUMP("DiskSize    = %lld KB\r\n", FsLinux_GetDiskInfo('A', FST_INFO_DISK_SIZE) / 1024);
	DBG_DUMP("freeSpace   = %lld KB\r\n", FsLinux_GetDiskInfo('A', FST_INFO_FREE_SPACE) / 1024);
	DBG_DUMP("VolumeID    = 0x%08llX\r\n", FsLinux_GetDiskInfo('A', FST_INFO_VOLUME_ID));
	DBG_DUMP("ClusterSize = %lld KB\r\n", FsLinux_GetDiskInfo('A', FST_INFO_CLUSTER_SIZE));

	return TRUE;
}

static SXCMD_BEGIN(fslinux_cmd_tbl, "File System Linux Version")
SXCMD_ITEM("format", FsLinux_SxCmd_Format, "Format the disk")
SXCMD_ITEM("benchmark", FsLinux_SxCmd_Benchmark, "Access speed test")
SXCMD_ITEM("showinfo", FsLinux_SxCmd_ShowInfo, "Show disk info")
SXCMD_END()

static int fslinux_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(fslinux_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "fslinux (FsLinux)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", fslinux_cmd_tbl[loop].p_name, fslinux_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(fslinux, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(fslinux_cmd_tbl);
	UINT32 loop;
	int    ret;
	char   *arg;

	if (argc < 1) {
		return -1;
	}

	#if defined(__LINUX)
	arg = argv[0];
	#else
	arg = argv[1];
	#endif
	if (strncmp(arg, "?", 2) == 0) {
		fslinux_cmd_showhelp(vk_printk);
		return 0;
	}
	DBG_DUMP("argv[0] = %s\r\n", argv[0]);
	DBG_DUMP("argv[1] = %s\r\n", argv[1]);
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(arg, fslinux_cmd_tbl[loop].p_name, strlen(arg)) == 0) {
			ret = fslinux_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		//nvtmpp_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

