#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/examsys.h"
#include "kwrap/sxcmd.h"
//#include "FileSysTsk.h"
#include "LviewNvt/lviewd.h"
#include "LviewNvt/LviewNvtAPI.h"
#include "hdal.h"


#define __MODULE__          test_lviewd
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define PORT_NUM             8080
#define RANDOM_RANGE(n)     (randomUINT32() % (n))

static UINT32 g_jpgAddr;
static UINT32 g_jpgSize;
static UINT32 g_jpgAddr2;
static UINT32 g_jpgSize2;
static UINT32 gCount = 0;
static UINT32 g_buffer = 0;

static UINT64 xExamLviewNvt_getCurrTime(void)
{
	return hd_gettime_us();
}

// noify some status of HTTP server
static void xExamLviewNvt_notifyStatus(int status)
{
	switch (status) {
	// HTTP client has request coming in
	case CYG_LVIEW_STATUS_CLIENT_REQUEST:
		DBG_DUMP("client request %lld ms\r\n", xExamLviewNvt_getCurrTime() / 1000);
		break;
	// HTTP server send JPG data start
	case CYG_LVIEW_STATUS_SERVER_RESPONSE_START:
		//DBG_DUMP("response start, time= %05d ms\r\n",xExamLviewNvt_getCurrTime()/1000);
		break;
	// HTTP server send JPG data end
	case CYG_LVIEW_STATUS_SERVER_RESPONSE_END:
		//DBG_DUMP("se time= %05d ms\r\n", xExamLviewNvt_getCurrTime() / 1000);
		break;
	case CYG_LVIEW_STATUS_CLIENT_DISCONNECT:
		DBG_DUMP("client disconnect %lld ms\r\n", xExamLviewNvt_getCurrTime() / 1000);
		break;
	}
}

static int xExamLviewNvt_getJpg(int *jpgAddr, int *jpgSize)
{
	int        ret;

	if (gCount % 10 == 0) {
		*jpgAddr = g_jpgAddr;
		*jpgSize = g_jpgSize;
	} else {
		*jpgAddr = g_jpgAddr2;
		*jpgSize = g_jpgSize2;
	}
	gCount++;
	ret = TRUE;
	//DBG_MSG("jpgAddr=0x%x,jpgSize=0x%x\r\n ",*jpgAddr,*jpgSize);
	return ret;
}

// load the jpg image from SD card , the file path is A:\HTTP_SRC.JPG.
static void xExamLviewNvt_loadimg(UINT32 pBuf)
{
	FILE          *p_file;
	UINT32        size = 0x100000;

	p_file = fopen("/mnt/sd/HTTP/HTTP1.JPG", "rb");
	if (NULL == p_file) {
		DBG_ERR("No file /mnt/sd/HTTP/HTTP1.JPG\r\n");
		return;
	} else {
		fread((void*)pBuf, 1, size, p_file);
		fclose(p_file);
	}
	g_jpgAddr = pBuf;
	g_jpgSize = size;
	DBG_MSG("g_jpgAddr=0x%x,g_jpgSize=0x%x\r\n ", g_jpgAddr, g_jpgSize);

	pBuf += 0x100000;
	p_file = fopen("/mnt/sd/HTTP/HTTP2.JPG", "rb");
	if (NULL == p_file) {
		DBG_ERR("No file /mnt/sd/HTTP/HTTP2.JPG\r\n");
		return;
	} else {
		fread((void*)pBuf, 1, size, p_file);
		fclose(p_file);
	}
	g_jpgAddr2 = pBuf;
	g_jpgSize2 = size;
	DBG_MSG("g_jpgAddr2=0x%x,g_jpgSize2=0x%x\r\n ", g_jpgAddr2, g_jpgSize2);
	return;
}


static BOOL xExamLviewNvt_Start(unsigned char argc, char **argv)
{
	LVIEWNVT_INFO      LViewInfo = {0};
	UINT32             frameRate = 30, portNum = PORT_NUM, threadPriority = 6, maxJpgSize = 1024 * 200, sockbufSize = 51200;
	HD_RESULT          ret;


	DBG_IND("\r\n");

	//sscanf_s(strCmd, "%d %d %d %d %d", &portNum, &threadPriority, &maxJpgSize, &frameRate, &sockbufSize);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&portNum);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&threadPriority);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%d", (int *)&maxJpgSize);
	}
	if (argc >= 4) {
		sscanf(argv[3], "%d", (int *)&frameRate);
	}
	if (argc >= 5) {
		sscanf(argv[4], "%d", (int *)&sockbufSize);
	}
	// load image A:\HTTP_SRC.JPG to buffer
	g_buffer = (UINT32)malloc(0x200000);
	xExamLviewNvt_loadimg(g_buffer);

	//init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
		printf("init fail=%d\r\n", ret);
	}
	DBG_DUMP("start\r\n");

	// start Http daemon
	{
		LVIEWNVT_DAEMON_INFO   lviewObj = {0};

		lviewObj.getJpg = (LVIEWD_GETJPGCB)xExamLviewNvt_getJpg;
		lviewObj.serverEvent = (LVIEWD_SERVER_EVENT_CB)xExamLviewNvt_notifyStatus;
		lviewObj.portNum = portNum;
		// set http live view server thread priority
		lviewObj.threadPriority = threadPriority;
		// set the maximum JPG size that http live view server to support
		//lviewObj.maxJpgSize = maxJpgSize;
		// live view streaming frame rate
		lviewObj.frameRate = frameRate;
		// socket buffer size
		lviewObj.sockbufSize = sockbufSize;

		lviewObj.bitstream_mem_range.addr = LViewInfo.workMemAdr;
		lviewObj.bitstream_mem_range.size = LViewInfo.workMemSize;
		// reserved parameter , no use now, just fill NULL.
		lviewObj.arg = NULL;
		// if want to use https
		lviewObj.is_ssl = 0;
		// start http daemon
		LviewNvt_StartDaemon(&lviewObj);
	}
	return 0;
}




// stop Http liveview daemon
static BOOL xExamLviewNvt_Stop(unsigned char argc, char **argv)
{
	HD_RESULT          ret;

	DBG_IND("stop Daemon\r\n");
	LviewNvt_StopDaemon();
	DBG_IND("stop end\r\n");
	free((void *)g_buffer);
	//uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
    }
	return 0;
}



static SXCMD_BEGIN(lviewd_cmd_tbl, "test_lviewd")
SXCMD_ITEM("start", xExamLviewNvt_Start, "")
SXCMD_ITEM("stop", xExamLviewNvt_Stop, "")
SXCMD_END()


static int lviewd_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(lviewd_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "lviewd");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", lviewd_cmd_tbl[loop].p_name, lviewd_cmd_tbl[loop].p_desc);
	}
	return 0;
}

EXAMFUNC_ENTRY(test_lviewd, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(lviewd_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		lviewd_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], lviewd_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = lviewd_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		lviewd_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

