#ifndef _LOGFILE_OP_H
#define _LOGFILE_OP_H

#define LOG_DUMP_DATA_COMPLETE 0

typedef enum {
	LOGFILE_DUMP_ER_FINISH          =   0, ///< All data dump finish
	LOGFILE_DUMP_ER_CONTINUE        =   1, ///< Still have data need to dump
	LOGFILE_DUMP_ER_SYS             =  -1, ///< Some system error occur
	LOGFILE_DUMP_ER_WRITE_FILE      =  -2, ///< Has some error when write file
	LOGFILE_DUMP_ER_BUFF2SMALL      =  -3, ///< Buffer size is too small
	LOGFILE_DUMP_ER_PARM            =  -4, ///< Input parameter has some error
	ENUM_DUMMY4WORD(LOGFILE_DUMP_ER)
} LOGFILE_DUMP_ER;

extern void     		LogFile_LockWrite(void);
extern void     		LogFile_UnLockWrite(void);
extern FST_FILE 		LogFile_OpenNewFile(LOGFILE_RW_CTRL *p_rw_ctrl);
extern void     		LogFile_CloseFile(FST_FILE  filehdl);
extern BOOL     		LogFile_WriteFile(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl);
extern BOOL				LogFile_WriteEndTag(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl);
extern void				LogFile_ZeroFileEnd(LOGFILE_RW_CTRL *p_rw_ctrl, FST_FILE  filehdl, UINT32 zerobuf, UINT32 zerobufSize);
extern LOGFILE_DUMP_ER	LogFile_DumpData(LOGFILE_RW_CTRL *p_rw_ctrl, UINT32 bufAddr, UINT32 bufSize, UINT32 *dumpSize);
extern void 			LogFile_SaveLastTimeSysErrLog(UINT32 Addr, UINT32 Size, CHAR  *filename);
#endif //_LOGFILE_OP_H