#ifndef _LOGFILE_INT_H
#define _LOGFILE_INT_H
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "LogFile.h"
#include "LogFileNaming.h"
#include "FileSysTsk.h"


// flag define
#define FLG_LOGFILE_IDLE             FLGPTN_BIT(0)
#define FLG_LOGFILE_STOP             FLGPTN_BIT(1)
#define FLG_LOGFILE_TIMEHIT          FLGPTN_BIT(2)
#define FLG_LOGFILE_BUFFFULL         FLGPTN_BIT(3)
#define FLG_LOGFILE_STOP_EMR         FLGPTN_BIT(4)
#define FLG_LOGFILE_EMR_IDLE         FLGPTN_BIT(5)
#define FLG_LOGFILE_COMPLETE         FLGPTN_BIT(6)
#define FLG_LOGFILE_COMPLETED        FLGPTN_BIT(7)


// status
#define LOGFILE_STATE_UNINIT          0
#define LOGFILE_STATE_INIT            1
#define LOGFILE_STATE_NORMAL          2
#define LOGFILE_STATE_SUSPEND         3
#define LOGFILE_STATE_CLOSING         4


#define CFG_LOGFILE_INIT_KEY   MAKEFOURCC('L','O','G','F')
#define LOGFILE_INTERFACE_VER  0x19112808
#define LOGFILE_SYS_ERROR_KEY  MAKEFOURCC('S','Y','S','E')

#define LOG_WRITE_CHECK_TIME   1010     //ms
#define LOG_CLR_BUFFER_SIZE    32*1024



//
//
//#define LOGFILE_USE_TRUNCATE          1
//#define LOGFILE_USE_ALLOCATE          0
#define LOG_CORE1                     0
#define LOG_CORE2                     1


#define LOGFILE_USE_TRUNCATE          1
#define CACHE_LINE_ALIGN              64

/*
 *  "attach close-on-exec flag to fix umount issue" (linux platform issue)
 *
 *  If fork child process when file opened by parent process,
 *  it will cause file handle leak because child process won't close it.
 *  So, enable the close-on-exec flag for the new file descriptor.
 */
#define LOGFILE_USE_CLOEXEC             1

enum log_flags {
	LOG_ROLLBACK = 1,	/* log already rollback */
};

typedef struct {
	UINT32          	InterfaceVer;      ///< the Interface version of logfile ring buffer
	UINT32          	BufferStartAddr;   ///< the buffer start address for store log msg
	UINT32          	BufferSize;        ///< the total buffer size for store log msg
	UINT32          	DataIn;            ///< the log msg input offset
	UINT32          	DataOut;           ///< the log msg output offset
	unsigned int        BufferMapAddr;     ///< the ioremap buffer start address for store log msg
	unsigned int        SysErr;            ///< the kernel panic or some system error
	unsigned int        flags;             ///< the log flags
	unsigned int        reserved[8];       ///< reserved
} LOGFILE_RINGBUF_HEAD;

STATIC_ASSERT(sizeof(LOGFILE_DATA_HEAD) <= sizeof(LOGFILE_RINGBUF_HEAD));
STATIC_ASSERT(CACHE_LINE_ALIGN == sizeof(LOGFILE_RINGBUF_HEAD));

typedef struct {
	UINT32          	ClusterSize;       ///< the cluster size of fat
	UINT32          	CurrFileSize;      ///< the file size of current write log file
	UINT32          	CurrFileClusCnt;   ///< the file cluster count of current write log file
	UINT32          	maxFileSize;       ///< the maximum file size of each log file
	UINT32          	maxFileNum;        ///< the maximum file number
	FST_FILE        	filehdl;           ///< the file handle of current write log file
	BOOL            	hasFileErr;        ///< error happened in file operation
	BOOL            	hasOverflow;       ///< has buffer overflow
	BOOL                isPreAllocAllFiles;
	BOOL                isZeroFile;
	LOGFILE_NAME_OBJ     *p_nameobj;
	LOGFILE_RINGBUF_HEAD *pbuf;
} LOGFILE_RW_CTRL;

typedef struct {
	UINT32          	ConType;           ///< the console output type
	LOGFILE_TIME_TYPE  	TimeType;
	UINT32          	state;             ///< the current state of logfile module
	UINT32          	uiInitKey;         ///< indicate module is initail
	LOGFILE_OPEN    	OpenParm;          ///< init open parameter
	UINT32          	FlagID;            ///< the flag ID
	LOGFILE_RW_CTRL		rw_ctrl[2];
	UINT32          	TimerCnt;          ///< Timer count
	BOOL            	isAddTimeStr;	   ///< Add time string in this string head
	BOOL            	isLongCounterReady;///< If long counter is ready
	UINT32              LogCoreNum;        ///< the core number want to log

} LOGFILE_CTRL;




extern UINT32 LogFile_GetDataSize(LOGFILE_RINGBUF_HEAD *pbuf);
extern LOGFILE_CTRL *LogFile_GetCtrl(void);
extern THREAD_RETTYPE LogFile_MainTsk(void);
extern THREAD_RETTYPE LogFile_EmrTsk(void);
//extern void          LogFile_InstallCmd(void);

#endif //_LOGFILE_INT_H
