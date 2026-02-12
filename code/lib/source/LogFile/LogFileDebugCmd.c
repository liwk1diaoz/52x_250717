#include <stdio.h>
#include <string.h>
#include <kwrap/type.h>
#include <kwrap/cmdsys.h>
#include <kwrap/sxcmd.h>
#include "LogFileInt.h"
#include "LogFile.h"

#define __MODULE__          LogFile
#define __DBGLVL__          2
#include "kwrap/debug.h"
extern unsigned int LogFile_debug_level;

void LogFile_DumpDebugInfo(void)
{
	LOGFILE_CTRL    *pCtrl = LogFile_GetCtrl();
	UINT32          i;
	LOGFILE_RW_CTRL *p_rw_ctrl;

	DBG_DUMP("[dump]----------------LogFile info begin----------------\r\n");
	DBG_DUMP("[dump]----\t state =%d\r\n", pCtrl->state);
	for (i = 0; i < pCtrl->LogCoreNum; i++) {
		p_rw_ctrl = &pCtrl->rw_ctrl[i];
		DBG_DUMP("[dump]----\t hasFileErr =%d, hasOverflow=%d\r\n", p_rw_ctrl->hasFileErr, p_rw_ctrl->hasOverflow);
		DBG_DUMP("[dump]----\t ClusterSize =0x%x, CurrFileSize =0x%x, maxFileSize=0x%x\r\n", p_rw_ctrl->ClusterSize, p_rw_ctrl->CurrFileSize, p_rw_ctrl->maxFileSize);
		DBG_DUMP("[dump]----\t Ring buffer Head = 0x%x, BuffAddr =0x%x, BuffSize =0x%x, \r\n", (int)p_rw_ctrl->pbuf, p_rw_ctrl->pbuf->BufferStartAddr, p_rw_ctrl->pbuf->BufferSize);
		DBG_DUMP("[dump]----\t BufferMapAddr = 0x%x, SysErr =0x%x\r\n", p_rw_ctrl->pbuf->BufferMapAddr, p_rw_ctrl->pbuf->SysErr);
		DBG_DUMP("[dump]----\t InterfaceVer = 0x%x, DIn =0x%x, DOut =0x%x \r\n", p_rw_ctrl->pbuf->InterfaceVer, p_rw_ctrl->pbuf->DataIn, p_rw_ctrl->pbuf->DataOut);
	}
	DBG_DUMP("[dump]----------------LogFile info end----------------\r\n");
}

static BOOL Cmd_LogFile_suspend(unsigned char argc, char **argv)
{
	LogFile_Suspend();
	return TRUE;
}

static BOOL Cmd_LogFile_resume(unsigned char argc, char **argv)
{
	LogFile_Resume();
	return TRUE;
}

static BOOL Cmd_LogFile_close(unsigned char argc, char **argv)
{
	LogFile_Close();
	return TRUE;
}


static BOOL Cmd_LogFile_dumpinfo(unsigned char argc, char **argv)
{
	LogFile_DumpDebugInfo();
	return TRUE;
}

static BOOL Cmd_LogFile_dumpmem(unsigned char argc, char **argv)
{
	UINT32 addr = 0,size = 0x100000;

	sscanf(argv[0], "%x", (int *)&addr);
	sscanf(argv[1], "%x", (int *)&size);
	LogFile_DumpToMem(addr,size);
	return TRUE;
}

static BOOL Cmd_LogFile_dumpfile(unsigned char argc, char **argv)
{
	CHAR filepath[80]="A:\\log.txt";

//	sscanf_s(argv[0], "%s", &filepath, sizeof(filepath));
	LogFile_DumpToFile(filepath);
	return TRUE;
}

static SXCMD_BEGIN(logfile_cmd_tbl, "logfile_cmd_tbl")
SXCMD_ITEM("suspend",    Cmd_LogFile_suspend, "suspend logfile")
SXCMD_ITEM("resume",     Cmd_LogFile_resume, "resume logfile")
SXCMD_ITEM("close",      Cmd_LogFile_close, "close logfile")
SXCMD_ITEM("dumpinfo",   Cmd_LogFile_dumpinfo, "dump logfile control info")
SXCMD_ITEM("dumpmem %",  Cmd_LogFile_dumpmem,  "dump logfile data to memory")
SXCMD_ITEM("dumpfile %", Cmd_LogFile_dumpfile, "dump logfile data to file")
SXCMD_END()

static int logfile_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(logfile_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "logfile (logfile)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", logfile_cmd_tbl[loop].p_name, logfile_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(logfile, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(logfile_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		logfile_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], logfile_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = logfile_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}
