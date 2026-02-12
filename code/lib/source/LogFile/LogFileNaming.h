#ifndef _LOGFILE_NAMING_H
#define _LOGFILE_NAMING_H
#include "LogFile.h"

#define MAX_FILE_NAME_LEN     20

typedef struct {
	CHAR    PathRoot[LOGFILE_ROOT_DIR_MAX_LEN + 1];
	CHAR    FileNameTbl[LOGFILE_MAX_FILENUM][MAX_FILE_NAME_LEN+1];
	UINT32  FileIdx;
	UINT32  MaxFileCount;
} LOGFILE_NAME_OBJ;

typedef enum {
	LOGFILE_NAME_TYPE_NORMAL        =   0, ///< normal log naming
	LOGFILE_NAME_TYPE_NORMAL2       =   1, ///< normal log naming
	LOGFILE_NAME_TYPE_SYSERR        =   2, ///< system error log naming
	LOGFILE_NAME_TYPE_MAX           =   3, ///< max value
	ENUM_DUMMY4WORD(LOGFILE_NAME_TYPE)
} LOGFILE_NAME_TYPE;



extern LOGFILE_NAME_OBJ*	LogFileNaming_GetObj(LOGFILE_NAME_TYPE name_type);
extern UINT32  				LogFileNaming_Init(LOGFILE_NAME_OBJ *pObj, UINT32 MaxFileCount, CHAR *rootDir);
extern INT32   				LogFileNaming_PreAllocFiles(LOGFILE_NAME_OBJ *pObj, UINT32 MaxFileCount, UINT32 maxFileSize);
extern BOOL    				LogFileNaming_GetNewPath(LOGFILE_NAME_OBJ *pObj, CHAR *path, UINT32 pathlen);

#endif //_LOGFILE_NAMING_H
