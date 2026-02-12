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
#include "HfsNvt/HfsNvtAPI.h"

#define __MODULE__          test_hfs
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define RANDOM_RANGE(n)     (randomUINT32() % (n))
#define TEMP_BUF_SIZE        0x1000000
#define PORT_NUM             80
#define MOUNT_FS_ROOT        "/mnt/sd"
#define FAT_GET_DAY_FROM_DATE(x)     (x & 0x1F)              ///<  get day from date
#define FAT_GET_MONTH_FROM_DATE(x)   ((x >> 5) & 0x0F)       ///<  get month from date
#define FAT_GET_YEAR_FROM_DATE(x)    ((x >> 9) & 0x7F)+1980  ///<  get year from date
#define FAT_GET_SEC_FROM_TIME(x)     (x & 0x001F)<<1         ///<  seconds(2 seconds / unit)
#define FAT_GET_MIN_FROM_TIME(x)     (x & 0x07E0)>>5         ///<  Minutes
#define FAT_GET_HOUR_FROM_TIME(x)    (x & 0xF800)>>11        ///<  hours

//static UINT32  gloopCount = 500, gRandomMs = 5000, gExit = 0, gRobust_tsk_exit = 0;
//static void    xExamHfs_robustTsk(void);



static UINT32 xExamHfs_getCurrTime(void)
{
	//return timer_getCurrentCount(timer_getSysTimerID());
	return 0;
}

static void xExamHfs_notifyStatus(int status)
{
	switch (status) {
	// HTTP client has request coming in
	case CYG_HFS_STATUS_CLIENT_REQUEST:
		DBG_DUMP("client request %05d ms\r\n", xExamHfs_getCurrTime() / 1000);
		break;
	// HTTP server send data start
	case CYG_HFS_STATUS_SERVER_RESPONSE_START:
		DBG_DUMP("ss, time= %05d ms\r\n", xExamHfs_getCurrTime() / 1000);
		break;
	// HTTP server send data start
	case CYG_HFS_STATUS_SERVER_RESPONSE_END:
		DBG_DUMP("se, time= %05d ms\r\n", xExamHfs_getCurrTime() / 1000);
		break;
	// HTTP client disconnect
	case CYG_HFS_STATUS_CLIENT_DISCONNECT:
		DBG_DUMP("client disconnect, time= %05d ms\r\n", xExamHfs_getCurrTime() / 1000);
		break;

	}
}

static INT32 xExamHfs_CheckPasswd(const char *username, const char *passwd, char *key, char *questionstr)
{
	DBG_IND("username= %s, passwd= %s, key= %s \r\n", username, passwd, key);
	DBG_IND("questionstr= %s\r\n", questionstr);
	if (strncmp(username, "hfsuser", strlen("hfsuser")) == 0) {
		if (strncmp(passwd, "12345", strlen("12345")) == 0) {
			// return 1 indicates user accepted
			return 1;
		}
	}
	// return 0 indicates user rejected
	return 0;
}
#if 0
static void xExamHfs_ecos2NvtPath(const char *inPath, char *outPath, UINT32 outPathlen)
{
	outPath += snprintf(outPath, outPathlen, "A:");
	inPath += strlen(MOUNT_FS_ROOT);
	while (*inPath != 0) {
		if (*inPath == '/') {
			*outPath = '\\';
		} else {
			*outPath = *inPath;
		}
		inPath++;
		outPath++;
	}
	*outPath++ = '\\';
	*outPath = 0;
}

static void xExamHfs_Nvt2ecosPath(const char *inPath, char *outPath, UINT32 outPathlen)
{
	outPath += snprintf(outPath, outPathlen, MOUNT_FS_ROOT);
	inPath += strlen("A:");
	while (*inPath != 0) {
		if (*inPath == '\\') {
			*outPath = '/';
		} else {
			*outPath = *inPath;
		}
		inPath++;
		outPath++;
	}
	*outPath = 0;
}


static INT32 xExamHfs_getCustomData(CHAR *path, CHAR *argument, UINT32 bufAddr, UINT32 *bufSize, CHAR *mimeType, UINT32 segmentCount)
{
	static UINT32     gIndex;

	UINT32            len, fileCount, i, bufflen = *bufSize, remain_bufflen = bufflen, FileType;
	UINT32            bufAddress = bufAddr;
	char             *buf = (char *)bufAddress;
	char              tempPath[128];
	FST_FILE_STATUS   FileStat = {0};
	FST_FILE          filehdl;
	char              dcfFilePath[128];


	// set the data mimetype
	strncpy(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	mimeType[CYG_HFS_MIMETYPE_MAXLEN] = 0;
	DBG_IND("path = %s, argument -> %s, mimeType= %s, buf = 0x%x, bufsize= %d, segmentCount= %d\r\n", path, argument, mimeType, buf, *bufSize, segmentCount);
	xExamHfs_ecos2NvtPath(path, tempPath, sizeof(tempPath));
	DBG_IND("tempPath = %s\r\n", tempPath);
	//return 0;

	if (segmentCount == 0) {
		len = snprintf(buf, remain_bufflen, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n<PATHROOT>%s</PATHROOT>\n<PATH>%s</PATH>\n", MOUNT_FS_ROOT, path);
		buf += len;
		remain_bufflen -= len;
	}
	fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
	if (segmentCount == 0) {
		// reset some global variables
		gIndex = 1;
	}
	DBG_IND("gIndex = %d\r\n", gIndex);
	for (i = gIndex; i <= fileCount; i++) {
		// check buffer length , reserved 512 bytes
		// should not write data over buffer length
		if ((UINT32)buf - bufAddress > bufflen - 512) {
			DBG_IND("totallen=%d >  bufflen(%d)-512\r\n", (UINT32)buf - bufAddress, bufflen);
			*bufSize = (UINT32)(buf) - bufAddress;
			gIndex = i;
			return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
		}
		// get dcf file
		DCF_SetCurIndex(i);
		FileType = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);

		if (FileType & DCF_FILE_TYPE_JPG) {
			DCF_GetObjPath(i, DCF_FILE_TYPE_JPG, dcfFilePath);
		} else if (FileType & DCF_FILE_TYPE_MOV) {
			DCF_GetObjPath(i, DCF_FILE_TYPE_MOV, dcfFilePath);
		} else {
			continue;
		}
		// get file state
		filehdl = FileSys_OpenFile(dcfFilePath, FST_OPEN_READ);
		FileSys_StatFile(filehdl, &FileStat);
		FileSys_CloseFile(filehdl);
		xExamHfs_Nvt2ecosPath(dcfFilePath, tempPath, sizeof(tempPath));
		// this is a dir
		if (M_IsDirectory(FileStat.uiAttrib)) {
			len = snprintf(buf, remain_bufflen, "<Dir>\n<NAME>\n<![CDATA[%s]]></NAME><FPATH>\n<![CDATA[%s]]></FPATH>", &tempPath[15], tempPath);
			buf += len;
			remain_bufflen -= len;
			len = snprintf(buf, remain_bufflen, "<TIMECODE>%ld</TIMECODE><TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n</Dir>\n", (FileStat.uiModifiedDate << 16) + FileStat.uiModifiedTime,
						   FAT_GET_YEAR_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_MONTH_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_DAY_FROM_DATE(FileStat.uiModifiedDate),
						   FAT_GET_HOUR_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_MIN_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_SEC_FROM_TIME(FileStat.uiModifiedTime));
			buf += len;
			remain_bufflen -= len;
		}
		// this is a file
		else {
			len = snprintf(buf, remain_bufflen, "<ALLFile><File>\n<NAME>\n<![CDATA[%s]]></NAME><FPATH>\n<![CDATA[%s]]></FPATH>", &tempPath[15], tempPath);
			buf += len;
			remain_bufflen -= len;
			len = snprintf(buf, remain_bufflen, "<SIZE>%lld</SIZE>\n<TIMECODE>%ld</TIMECODE>\n<TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n</File>\n</ALLFile>\n", FileStat.uiFileSize, (FileStat.uiModifiedDate << 16) + FileStat.uiModifiedTime,
						   FAT_GET_YEAR_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_MONTH_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_DAY_FROM_DATE(FileStat.uiModifiedDate),
						   FAT_GET_HOUR_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_MIN_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_SEC_FROM_TIME(FileStat.uiModifiedTime));
			buf += len;
			remain_bufflen -= len;
		}
	}
	len = snprintf(buf, remain_bufflen, "</LIST>\n");
	buf += len;
	remain_bufflen -= len;
	*bufSize = (UINT32)(buf) - bufAddress;
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

static INT32 xExamHfs_putCustomData(CHAR *path, CHAR *argument, UINT32 bufAddr, UINT32 bufSize, UINT32 segmentCount, HFS_PUT_STATUS putStatus)
{
	char              tempPath[128];
	static            FST_FILE filehdl = NULL;
	UINT32            writeSize = bufSize;

	DBG_IND("path =%s, argument = %s, bufAddr = 0x%x, bufSize =0x%x , segmentCount  =%d , putStatus = %d\r\n", path, argument, bufAddr, bufSize, segmentCount, putStatus);
	xExamHfs_ecos2NvtPath(path, tempPath, sizeof(tempPath));
	DBG_IND("tempPath = %s\r\n", tempPath);
	if (putStatus == HFS_PUT_STATUS_ERR) {
		DBG_ERR("receive data has some error\r\n");
		if (filehdl) {
			FileSys_CloseFile(filehdl);
		}
		return CYG_HFS_UPLOAD_FAIL_RECEIVE_ERROR;
	}
	// first data
	if (segmentCount == 0) {
		if (filehdl) {
			FileSys_CloseFile(filehdl);
		}
		filehdl = FileSys_OpenFile(tempPath, FST_CREATE_ALWAYS | FST_OPEN_WRITE | FST_SPEEDUP_CONTACCESS);
		if (!filehdl) {
			return CYG_HFS_UPLOAD_FAIL_WRITE_ERROR;
		}

	}
	if (putStatus == HFS_PUT_STATUS_FINISH) {
		// last data
		DBG_IND("last data\r\n");
		FileSys_WriteFile(filehdl, (UINT8 *)(bufAddr), &writeSize, 0, NULL);
		FileSys_CloseFile(filehdl);
		filehdl = NULL;
	} else if (putStatus == HFS_PUT_STATUS_CONTINUE) {

		// data
		DBG_IND("data\r\n");
		FileSys_WriteFile(filehdl, (UINT8 *)(bufAddr), &writeSize, 0, NULL);
	}

	return CYG_HFS_UPLOAD_OK;
}
#endif

static INT32 xExamHfs_ClientQueryData(CHAR *path, CHAR *argument, UINT32 bufAddr, UINT32 *bufSize, CHAR *mimeType, UINT32 segmentCount)
{
	UINT32            len;
	UINT32            bufAddress = bufAddr;
	char             *buf = (char *)bufAddress;

	// set the data mimetype
	strncpy(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	mimeType[CYG_HFS_MIMETYPE_MAXLEN] = 0;
	DBG_IND("path = %s, argument -> %s, mimeType= %s, buf = 0x%x, bufsize= %d, segmentCount= %d\r\n", path, argument, mimeType, (int)buf, *bufSize, segmentCount);
	len = snprintf(buf, *bufSize, "test");
	buf += len;
	*bufSize = (UINT32)(buf) - bufAddress;
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

static INT32 xExamHfs_uploadResultCb(int result, UINT32 bufAddr, UINT32 *bufSize, CHAR *mimeType)
{
	UINT32            len;
	char             *buf = (char *)bufAddr;

	// set the data mimetype
	strncpy(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	mimeType[CYG_HFS_MIMETYPE_MAXLEN] = 0;
	if (result == CYG_HFS_UPLOAD_OK) {
		len = snprintf(buf, *bufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<RESULT>Upload OK</RESULT>\n");
	} else {
		len = snprintf(buf, *bufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<RESULT>Upload fail %d</RESULT>\n", result);
	}
	buf += len;
	*bufSize = (HFS_U32)(buf) - bufAddr;
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}


static INT32 xExamHfs_deleteResultCb(int result, UINT32 bufAddr, UINT32 *bufSize, CHAR *mimeType)
{
	UINT32            len;
	char             *buf = (char *)bufAddr;

	// set the data mimetype
	strncpy(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	mimeType[CYG_HFS_MIMETYPE_MAXLEN] = 0;
	if (result == CYG_HFS_UPLOAD_OK) {
		len = snprintf(buf, *bufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<RESULT>Delete OK</RESULT>\n");
	} else {
		len = snprintf(buf, *bufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<RESULT>Delete fail %d</RESULT>\n", result);
	}
	buf += len;
	*bufSize = (HFS_U32)(buf) - bufAddr;
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

static INT32 xExamHfs_headerCb(UINT32 headerAddr, UINT32 headerSize, CHAR *filepath, CHAR *mimeType, void *reserved)
{
	char *headerstr;

	headerstr = (char *)headerAddr;
	DBG_DUMP("http headerstr=%s", headerstr);
	if (!strncmp(headerstr, "GET", strlen("GET"))) {
		DBG_DUMP("http get\r\n");
		if (!strncmp(headerstr, "GET /test.jpg", strlen("GET /test.jpg"))) {
			// set the data mimetype
			strncpy(mimeType, "image/jpeg", CYG_HFS_MIMETYPE_MAXLEN);
			strncpy(filepath, "/sdcard/test.jpg", CYG_HFS_FILE_PATH_MAXLEN);
			DBG_DUMP("filepath = %s\r\n", filepath);
			return CYG_HFS_CB_HEADER_RETURN_DEFAULT;
		} else if (!strncmp(headerstr, "GET /CUSTOM", strlen("GET /CUSTOM"))) {
			DBG_DUMP("http get custom\r\n");
			return CYG_HFS_CB_HEADER_RETURN_CUSTOM;
		} else if (!strncmp(headerstr, "GET /lview", strlen("GET /lview"))) {
			DBG_DUMP("http rediret\r\n");
			strncpy(filepath, "Http://192.168.0.3:8192", CYG_HFS_FILE_PATH_MAXLEN);
			DBG_DUMP("http rediret to %s\r\n", filepath);
			return CYG_HFS_CB_HEADER_RETURN_REDIRECT;
		} else {
			return CYG_HFS_CB_HEADER_RETURN_DEFAULT;
		}
	} else if (!strncmp(headerstr, "PUT", strlen("PUT"))) {
		DBG_DUMP("http put\r\n");
		if (!strncmp(headerstr, "PUT /test.jpg", strlen("PUT /test.jpg"))) {
			// set the data mimetype
			strncpy(filepath, "/sdcard/upload.jpg", CYG_HFS_FILE_PATH_MAXLEN);
			return CYG_HFS_CB_HEADER_RETURN_DEFAULT;
		} else {
			return CYG_HFS_CB_HEADER_RETURN_CUSTOM;
		}
	} else if (!strncmp(headerstr, "POST", strlen("POST"))) {
		DBG_DUMP("http post\r\n");
		if (!strncmp(headerstr, "POST / ", strlen("POST / "))) {
			// set the data mimetype
			strncpy(filepath, "/sdcard/upload.jpg", CYG_HFS_FILE_PATH_MAXLEN);
			DBG_DUMP("filepath = %s\r\n", filepath);
			return CYG_HFS_CB_HEADER_RETURN_DEFAULT;
		} else {
			return CYG_HFS_CB_HEADER_RETURN_CUSTOM;
		}
	}
	DBG_DUMP("Unknown http method\r\n");
	return CYG_HFS_CB_HEADER_RETURN_ERROR;
}

static BOOL xExamHfs_Start(unsigned char argc, char **argv)
{
	HFSNVT_OPEN  hfsObj = {0};
	int          checkpwd = 0;
	int          portNum = PORT_NUM;
	int          httpsPort = 443;
	int          clientQuery = 0;
	char         clientQueryStr[20] = "query_devinfo";
	int          uploadCB = 1;
	int          deleteCB = 1;
	int          headerCB = 0;
	char         customstr[HFS_CUSTOM_STR_MAXLEN + 1] = {0};

	DBG_IND("\r\n");
	//sscanf_s(strCmd, "%d %d %d %d %s %d %d %d %s", &portNum, &httpsPort, &checkpwd, &clientQuery, clientQueryStr, &uploadCB, &deleteCB, &headerCB, customstr, sizeof(customstr));
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&portNum);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&httpsPort);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%d", (int *)&checkpwd);
	}
	if (argc >= 4) {
		sscanf(argv[3], "%d", (int *)&clientQuery);
	}
	if (argc >= 5) {
		strncpy(clientQueryStr, argv[4], sizeof(clientQueryStr)-1);
	}
	if (argc >= 6) {
		sscanf(argv[5], "%d", (int *)&uploadCB);
	}
	if (argc >= 7) {
		sscanf(argv[6], "%d", (int *)&deleteCB);
	}
	if (argc >= 8) {
		sscanf(argv[7], "%d", (int *)&headerCB);
	}
	if (argc >= 9) {
		strncpy(customstr, argv[8], sizeof(customstr)-1);
	}

	// noify some status of HTTP server
	hfsObj.serverEvent = xExamHfs_notifyStatus;
	// register callback function of get custom data
	//hfsObj.getCustomData = xExamHfs_getCustomData;
	// register callback function of put custom data
	//hfsObj.putCustomData = xExamHfs_putCustomData;
	// if need to check user & password
	if (checkpwd) {
		hfsObj.check_pwd = xExamHfs_CheckPasswd;
	}
	if (clientQuery) {
		hfsObj.clientQuery = xExamHfs_ClientQueryData;
		strncpy(hfsObj.clientQueryStr, clientQueryStr, HFS_USER_QUERY_MAXLEN);
		hfsObj.clientQueryStr[HFS_USER_QUERY_MAXLEN] = 0;
	}
	// register callback function of upload result
	if (uploadCB) {
		hfsObj.uploadResultCb = xExamHfs_uploadResultCb;
	}
	// register callback function of delete result
	if (deleteCB) {
		hfsObj.deleteResultCb = xExamHfs_deleteResultCb;
	}
	if (headerCB) {
		hfsObj.headerCb = xExamHfs_headerCb;
	}
	// set port number
	hfsObj.portNum = portNum;
	hfsObj.httpsPortNum = httpsPort;
	// set thread priority
	hfsObj.threadPriority = 6;
	// set HFS root dir path
	strncpy(hfsObj.rootdir, MOUNT_FS_ROOT, HFS_ROOT_DIR_MAXLEN);
	hfsObj.rootdir[HFS_ROOT_DIR_MAXLEN] = 0;
	// set socket buff size
	hfsObj.sockbufSize = 51200;
	// the shared memory should be non-cache address
	hfsObj.sharedMemAddr = 0;
	// the shared memory size should be larger than 70KB
	hfsObj.sharedMemSize = 0;
	strncpy(hfsObj.customStr, customstr, HFS_CUSTOM_STR_MAXLEN);
	HfsNvt_Open(&hfsObj);

	// 1. Example of browsing root dir -> URL: http://192.168.0.2/
	// 2. Example of download file -> URL: http://192.168.0.2/movie.jpg
	// 3. Example of download file with specific offset & size -> URL: http://192.168.0.2/movie.jpg?offset=100&size=2000
	// 4. Example of download file with user & password -> URL: http://192.168.0.2/movie.jpg?usr=hfsuer&pwd=12345
	// 5. Example of get custom data -> URL: http://192.168.0.2/?custom=1
	// 6. Example of delete a file  -> URL: http://192.168.0.2/movie.jpg?del=1
	return 0;
}

static BOOL xExamHfs_Stop(unsigned char argc, char **argv)
{
	HfsNvt_Close();
	DBG_IND("stop\r\n");
	return 0;
}

#if 0
static void xExamHfs_robustTsk(void)
{
	UINT32 i, loopCount = gloopCount, RandomMs = gRandomMs, delayMs;

	for (i = 0; i < loopCount; i++) {
		DBG_DUMP("loop = %d/%d\r\n", i, gloopCount);
		delayMs = RANDOM_RANGE(RandomMs);
		xExamHfs_Start("");
		DBG_DUMP("B delayMs = %d\r\n", delayMs);
		SwTimer_DelayMs(delayMs);
		DBG_DUMP("E delayMs = %d\r\n", delayMs);
		xExamHfs_Stop("");
		if (gExit) {
			break;
		}
	}
	gRobust_tsk_exit = 1;
	ext_tsk();
}


static BOOL xExamHfs_robust(CHAR *strCmd)
{
	UINT32      loopCount = 500, RandomMs = 5000,  start_robust = 1;
	EXAMSRV_CMD ExamCmd = {0};
	EXAMSRV_USERTSK_CFG TaskCfg = {0};

	sscanf_s(strCmd, "%d %d %d", &start_robust, &loopCount, &RandomMs);

	TaskCfg.fpTask = xExamHfs_robustTsk;
	TaskCfg.PlugId = EXAMSRV_USER_ID_0;

	memset(&ExamCmd, 0, sizeof(ExamCmd));
	if (start_robust) {
		gloopCount = loopCount;
		gRandomMs = RandomMs;
		gExit = 0;
		gRobust_tsk_exit = 0;
		// start task
		ExamCmd.Idx = EXAMSRV_CMD_IDX_USERTSK_START;
		DBG_DUMP("start robust loop %d\r\n", loopCount);
	} else {
		// terminate tasks
		gExit = 1;
		while (1) {
			if (gRobust_tsk_exit == 1) {
				break;
			}
			SwTimer_DelayMs(1000);
		}
		ExamCmd.Idx = EXAMSRV_CMD_IDX_USERTSK_TERM;
		DBG_DUMP("stop robust loop %d\r\n", loopCount);
	}
	ExamCmd.In.pData = &TaskCfg.PlugId;
	ExamCmd.In.uiNumByte = sizeof(TaskCfg.PlugId);
	ExamSrv_Cmd(&ExamCmd);

	return TRUE;
}

static BOOL xExamHfs_robust2(CHAR *strCmd)
{
	UINT32 i, loopCount = 500, RandomUs = 5, delayUs;


	sscanf_s(strCmd, "%d %d", &loopCount, &RandomUs);


	for (i = 0; i < loopCount; i++) {
		DBG_DUMP("loop = %d\r\n", i);
		delayUs = RANDOM_RANGE(RandomUs);
		xExamHfs_Start("");
		//DBG_DUMP("B delayUs = %d\r\n",delayUs);
		Delay_DelayUs(delayUs);
		//DBG_DUMP("E delayUs = %d\r\n",delayUs);
		xExamHfs_Stop("");
		//SwTimer_DelayMs(2000);
	}
	return TRUE;
}


//¦CÁ| Item
EXAM_ITEM_BEGIN(m_Items)
EXAM_ITEM("open", xExamHfs_Open, "open environment")
EXAM_ITEM("close", xExamHfs_Close, "close environment")
EXAM_ITEM("start %", xExamHfs_Start, "start service")
EXAM_ITEM("stop", xExamHfs_Stop, "stop service")
EXAM_ITEM("robust2 %", xExamHfs_robust2, "robust test for open/close delay  us")
EXAM_ITEM("robust %", xExamHfs_robust, "robust test for open/close delay ms")
EXAM_ITEM_END()
#endif

static SXCMD_BEGIN(hfs_cmd_tbl, "test_hfs")
SXCMD_ITEM("start", xExamHfs_Start, "")
SXCMD_ITEM("stop", xExamHfs_Stop, "")
SXCMD_END()


static int hfs_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(hfs_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "hfs");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", hfs_cmd_tbl[loop].p_name, hfs_cmd_tbl[loop].p_desc);
	}
	return 0;
}

EXAMFUNC_ENTRY(test_hfs, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(hfs_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		hfs_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], hfs_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = hfs_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		hfs_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

