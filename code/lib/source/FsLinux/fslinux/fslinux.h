#ifndef _FSLINUX_H_
#define _FSLINUX_H_

#ifndef _BSP_NA51023_
#define _BSP_NA51023_
#endif

#ifndef ENUM_DUMMY4WORD
/**
    Macro to generate dummy element for enum type to expand enum size to word (4 bytes)
*/
//@{
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
//@}
#endif

#if defined (__LINUX_USER__)
#define FSLINUX_USE_IPC 0
#elif defined (__UITRON) || defined (__ECOS)
#define FSLINUX_USE_IPC 1
#endif

#define FSLINUX_SETPARAM            ENABLE

#define FSLINUX_INTERFACE_VER       "17082512"

#define FSLINUX_TOKEN_PATH          "fslinux"
#define FSLINUX_DRIVE_NUM           2
#define FSLINUX_DRIVE_NAME_FIRST    'A'
#define FSLINUX_DRIVE_NAME_LAST     (FSLINUX_DRIVE_NAME_FIRST + FSLINUX_DRIVE_NUM - 1)
#define FSLINUX_STATE_DIRTY         0x01
#define FSLINUX_VOL_CLEAN           0x0000
#define FSLINUX_VOL_DIRTY           0x0002

/**
     @name File system return status

     @note Should be identical to FST_STA_
*/
//@{
#define FSLINUX_STA_OK                  (0)     ///<  the status is ok
#define FSLINUX_STA_ERROR               (-1)    ///<  some error happened
#define FSLINUX_STA_BUSY                (-3)    ///<  file system task is busing
#define FSLINUX_STA_PARAM_ERR           (-6)    ///<  the input parameters are wrong
#define FSLINUX_STA_SDIO_ERR            (-15)   ///<  SDIO error is found
#define FSLINUX_STA_FS_NO_MORE_FILES    (-272)  ///<  use in search file, it means no more file in folder/
#define FSLINUX_STA_FILE_NOTEXIST       (-275)  ///<  file is not exist
//@}

/**
    @name FS file name length definition

    @note Should be identical to KFS_XXX_LENG
*/
//@{
#define FSLINUX_FILENAME_MAX_LENG           (12)
#define FSLINUX_LONGFILENAME_MAX_LENG       (128)   ///< the max length of long file name (N characters + 1 null)
#define FSLINUX_LONGNAME_PATH_MAX_LENG      (FSLINUX_LONGFILENAME_MAX_LENG + 4)   ///< the max length of long file name path (A:/ + N characters + 1 null)

/**
    @name FS type

    @note Should be identical to FST_FAT12/FST_FAT16/FST_FAT32/FST_EXFAT
*/
//@{
#define FSLINUX_FAT32                   (0x03)  ///< the storage is formatted as FAT32
#define FSLINUX_EXFAT                   (0x04)  ///< the storage is formatted as EXFAT

/**
    File DateTime

    @note Should be identical to FST_FILE_DATETIME_ID
*/
typedef enum _FSLINUX_FILE_DATETIME_ID {
	FSLINUX_FILE_DATETIME_YEAR = 0x00000000,   ///< the year of date, ex: 2012
	FSLINUX_FILE_DATETIME_MONTH,               ///< the month of date, value is from 1 ~12
	FSLINUX_FILE_DATETIME_DAY,                 ///< the day of date, value is from 1~31
	FSLINUX_FILE_DATETIME_HOUR,                ///< the hour of time, value is from 0~23
	FSLINUX_FILE_DATETIME_MINUTE,              ///< the minute of time, value is from 0~59
	FSLINUX_FILE_DATETIME_SECOND,              ///< the second of time, value is from 0~59
	FSLINUX_FILE_DATETIME_MAX_ID,              ///< the max id of datetime
} FSLINUX_FILE_DATETIME_ID;

/**
    File seek command

    @note Should be identical to FST_SEEK_CMD
*/
typedef enum _FSLINUX_SEEK_CMD {
	FSLINUX_SEEK_SET = 0x00000000,             ///< seek from the start of file
	FSLINUX_SEEK_CUR = 1,                      ///< seek from current position of file
	FSLINUX_SEEK_END = 2,                      ///< seek from the end of file
	FSLINUX_SEEK_MAX_ID,                       ///< the total seek command number
} FSLINUX_SEEK_CMD;

/**
    File system parameter ID

    @note Should be identical to FST_PARM_ID
*/
typedef enum _FSLINUX_PARM_ID {
	FSLINUX_PARM_SEGMENT_WRITE_MODE_EN = 0,    ///< if enable the segment write mode. (RW)
	FSLINUX_PARM_SEGMENT_READ_MODE_EN,         ///< if enable the segment read mode.  (RW)
	FSLINUX_PARM_SEGMENT_WRITE_SIZE,           ///< the segment write size.           (RW)
	FSLINUX_PARM_STRG_CLK_EN,                  ///< if enable the storage clock, enable means open memory bus, disable means close memory bus.  (WO)
	FSLINUX_PARM_TASK_STS,                     ///< check if file system task is busy.         (RO)
	FSLINUX_PARM_TASK_INIT_RDY,                ///< check if file system is initialized ready. (RO)
	FSLINUX_PARM_PERF_WRITE_MODE,              ///< get or set the performance mode (RW) (value 0:disable, 1:enable, 2:enable and dump log)
	FSLINUX_PARM_UPDATE_FSINFO,                ///< if the FS Info (FAT32) update is enabled. (RW)
	FSLINUX_PARM_UPDATE_MOUNT_STATE,           ///< (fslinux) update the mount/umount state dirty bit. (RW)
	FSLINUX_PARM_SKIP_SYNC,                    ///< (fslinux) if skip the (f)sync operation. (RW)
	FSLINUX_PARM_CHECK_DISK,                   ///< (fslinux) run the fsck operation. (RW)
} FSLINUX_PARM_ID;

/**
     @name File system open file flag

     @note Should be identical to FST_OPEN_
*/
//@{
#define FSLINUX_OPEN_READ               0x00000001      ///< open file with read mode
#define FSLINUX_OPEN_WRITE              0x00000002      ///< open file with write mode
#define FSLINUX_OPEN_EXISTING           0x00000004      ///< if exist, open; if not exist, return fail
#define FSLINUX_OPEN_ALWAYS             0x00000008      ///< if exist, open; if not exist, create new
#define FSLINUX_CREATE_HIDDEN           0x00000040      ///< if not exist, create new with hidden attrib
#define FSLINUX_CREATE_ALWAYS           0x00000020      ///< if exist, open and truncate it; if not exist, create new
#define FSLINUX_CLOSE_ON_EXEC           0x00000080      ///< enable the close-on-exec flag for the new file descriptor
//@}

/**
     @name File attribute

     @note Should be identical to FST_ATTRIB_
*/
//@{
#define FSLINUX_ATTRIB_READONLY         0x01    ///<   Indicates the file is read only.
#define FSLINUX_ATTRIB_HIDDEN           0x02    ///<   Indicates the file should not be shown in normal directory listings.
#define FSLINUX_ATTRIB_SYSTEM           0x04    ///<   Indicates the file is an operating system file.
#define FSLINUX_ATTRIB_VOLUME           0x08    ///<   Indicates the file is the label for the volume.
#define FSLINUX_ATTRIB_DIRECTORY        0x10    ///<   Indicates it is a directory
#define FSLINUX_ATTRIB_ARCHIVE          0x20    ///<   Indicates the file has been created or modified since the attribute was clear.
//@}

/**
    @name FS search file direction

    @note Should be identical to KFS_XXX_SEARCH
*/
//@{
#define FSLINUX_FORWARD_SEARCH          (0)     ///<  search file forward
#define FSLINUX_BACKWARD_SEARCH         (1)     ///<  search file backward
#define FSLINUX_HANDLEONLY_SEARCH       (2)     ///<  only get a search handle, used by fs_SearchFileOpen()
//@}

/**
     File status information.

     This structure contains the file status.

     @note Should be identical to FST_FILE_STATUS
*/
typedef struct _FSLINUX_FILE_STATUS {
	unsigned long long  uiFileSize;                     ///< size of the file in bytes
	unsigned short      uiAccessDate;                   ///< last access date
	unsigned short      uiModifiedDate;                 ///< last modified time
	unsigned short      uiModifiedTime;                 ///< last modified time
	unsigned short      uiCreatedDate;                  ///< file created date
	unsigned short      uiCreatedTime;                  ///< file created time
	unsigned char       uiCreatedTime10ms;              ///< file created time (unit: 10 ms, value: 0~199)
	unsigned char       uiAttrib;                       ///< file attribute
} FSLINUX_FILE_STATUS, *PFSLINUX_FILE_STATUS;

/**
    This sturcture contain data for finding files.

    @note Should be identical to FIND_DATA
*/
typedef struct _FSLINUX_FIND_DATA {
	char            FATMainName[8];     ///< Major file name.
	char            FATExtName[3];      ///< Extern file name.
	unsigned char   attrib;             ///< File attribute.
	unsigned char   Reserved;           ///< Reserved.
	unsigned char   creSecond;          ///< File created time.
	unsigned short  creTime;            ///< File created time. bit0~4: seconds/2, bit5~10: minutes, bit11~15: hours.
	unsigned short  creDate;            ///< File created date. bit0~4: day(1-31), bit5~8: month(1-12), bit9~15: year(0-127) add 1980 to convert
	unsigned short  lastAccessDate;     ///< The file latest access date.
	unsigned short  FstClusHI;          ///< The high 2 bytes of file start cluster.
	unsigned short  lastWriteTime;      ///< The latest time of write file.
	unsigned short  lastWriteDate;      ///< The latest date of write file.
	unsigned short  FstClusLO;          ///< The low 2 bytes of file start cluster.
	unsigned int    fileSize;           ///< The low 4 bytes of file size in bytes.
	unsigned int    fileSizeHI;         ///< The high 4 bytes of file size in bytes.
	char            filename[FSLINUX_LONGFILENAME_MAX_LENG + 1];    ///< The full filename.
	unsigned int    PartitionType;      ///< system ID, what kind of file system has been install on partition
	unsigned int    bNoFatChain;        ///< if this file has a fat cluster chain. (only for exFAT)
	unsigned int    ClusterCount;       ///< the total cluster count of this file
} FSLINUX_FIND_DATA, *PFSLINUX_FIND_DATA;

typedef struct {
	FSLINUX_FIND_DATA FindData;
	unsigned int flag;
} FSLINUX_FIND_DATA_ITEM;

/**
     Copy files information.

     Defines the information for copying a file from one storage to another storage.

     @note Should be identical to COPYTO_BYNAME_INFO
*/
typedef unsigned int FSLINUX_DX_HANDLE;
typedef struct _FSLINUX_COPYTO_BYNAME_INFO {
	FSLINUX_DX_HANDLE   pSrcStrg;                     ///< the source storage object
	FSLINUX_DX_HANDLE   pDstStrg;                     ///< the destionation storage object
	char                *pBuf;                        ///< the temp buffer for copy files
	unsigned int        uiBufSize;                    ///< the temp buffer size
	char               *pSrcPath;                     ///< the source file full path
	char               *pDstPath;                     ///< the destionation file full path
	int                 bDelete;                      ///< if delete the source file after copy complete
} FSLINUX_COPYTO_BYNAME_INFO, *PFSLINUX_COPYTO_BYNAME_INFO;

/**
    Command ID list for IPC
*/
typedef enum {
	FSLINUX_CMDID_GET_VER_INFO  =   0x00000000,
	FSLINUX_CMDID_GET_BUILD_DATE,
	FSLINUX_CMDID_OPENFILE,
	FSLINUX_CMDID_CLOSEFILE,
	FSLINUX_CMDID_READFILE,
	FSLINUX_CMDID_WRITEFILE,
	FSLINUX_CMDID_SEEKFILE,
	FSLINUX_CMDID_TELLFILE,
	FSLINUX_CMDID_DELETEFILE,
	FSLINUX_CMDID_STATFILE,
	FSLINUX_CMDID_GETFILELEN,
	FSLINUX_CMDID_GETDATETIME,
	FSLINUX_CMDID_SETDATETIME,
	FSLINUX_CMDID_FLUSHFILE,
	FSLINUX_CMDID_MAKEDIR,
	FSLINUX_CMDID_DELETEDIR,
	FSLINUX_CMDID_RENAMEDIR,
	FSLINUX_CMDID_RENAMEFILE,
	FSLINUX_CMDID_MOVEFILE,
	FSLINUX_CMDID_GETATTRIB,
	FSLINUX_CMDID_SETATTRIB,
	FSLINUX_CMDID_OPENDIR,
	FSLINUX_CMDID_READDIR,
	FSLINUX_CMDID_UNLINKATDIR,
	FSLINUX_CMDID_LOCKDIR,
	FSLINUX_CMDID_CLOSEDIR,
	FSLINUX_CMDID_REWINDDIR,
	FSLINUX_CMDID_COPYTOBYNAME,
	FSLINUX_CMDID_GETDISKINFO,
	FSLINUX_CMDID_FORMATANDLABEL,
	FSLINUX_CMDID_GETLABEL,
	FSLINUX_CMDID_ACKPID, //IPC_UP
	FSLINUX_CMDID_FINISH,
	FSLINUX_CMDID_TRUNCFILE,
	FSLINUX_CMDID_IPC_DOWN,
	FSLINUX_CMDID_ALLOCFILE,
	FSLINUX_CMDID_SETPARAM,
	FSLINUX_CMDID_MAX_ID,
	ENUM_DUMMY4WORD(FSLINUX_CMDID),
} FSLINUX_CMDID;

/**
    Ioctl type for checking
*/
typedef enum {
	FSLINUX_IOCTL_INVALID       =   0x00000000,
	FSLINUX_IOCTL_MSDOS_ENTRY   =   0x00000001,
	FSLINUX_IOCTL_EXFAT_ENTRY   =   0x00000002,
} FSLINUX_IOCTL_TYPE;

typedef struct {
	FSLINUX_CMDID CmdId;
	char Drive;
	unsigned int phy_ArgAddr;
} FSLINUX_IPCMSG;

struct FSLINUX_STRUCT_OPENFILE {
	int Linux_fd;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned int flag;
};

struct FSLINUX_STRUCT_PFILE {
	int ret_fslnx;
	int Linux_fd;
};

struct FSLINUX_STRUCT_RWFILE {
	int ret_fslnx;
	int Linux_fd;
	void *phy_pBuf;
	unsigned int rwsize;//the requested size and the return size
};

struct FSLINUX_STRUCT_SEEKFILE {
	int ret_fslnx;
	int Linux_fd;
	unsigned long long offset;
	int whence;
};

struct FSLINUX_STRUCT_TELLFILE {
	unsigned long long ret_fslnx;
	int Linux_fd;
};

struct FSLINUX_STRUCT_PATH {
	int ret_fslnx;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
};

struct FSLINUX_STRUCT_STATFILE {
	int ret_fslnx;
	int Linux_fd;
	FSLINUX_FILE_STATUS OutStat;
};

struct FSLINUX_STRUCT_TRUNCFILE {
	int ret_fslnx;
	int Linux_fd;
	unsigned long long NewSize;
};

struct FSLINUX_STRUCT_ALLOCFILE{
    int ret_fslnx;
    int Linux_fd;
    int mode;					//not supported, reserved
    unsigned long long offset;	//not supported, reserved
    unsigned long long len;
};

struct FSLINUX_STRUCT_GETFILELEN {
	unsigned long long ret_fslnx;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
};

struct FSLINUX_STRUCT_DATETIME {
	unsigned long long ret_fslnx;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned int DT_Created[FSLINUX_FILE_DATETIME_MAX_ID];
	unsigned int DT_Modified[FSLINUX_FILE_DATETIME_MAX_ID];
};

struct FSLINUX_STRUCT_RENAME {
	int ret_fslnx;
	char szNewName[FSLINUX_LONGNAME_PATH_MAX_LENG];
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned int bIsOverwrite;
};

struct FSLINUX_STRUCT_MOVEFILE {
	int ret_fslnx;
	char szSrcPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	char szDstPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
};

struct FSLINUX_STRUCT_GETATTRIB {
	int ret_fslnx;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned char Attrib;
};

struct FSLINUX_STRUCT_SETATTRIB {
	int ret_fslnx;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned char Attrib;
	int bSet;
};

struct FSLINUX_STRUCT_OPENDIR {
	unsigned int Linux_dirfd;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned int *phy_pItemBuf;
	int direction;
	unsigned int bUseIoctl;
};

struct FSLINUX_STRUCT_READDIR {
	int ret_fslnx;
	unsigned int Linux_dirfd;
	unsigned int *phy_pItemBuf;
	unsigned int ItemNum;
	unsigned int bUseIoctl;
};

struct FSLINUX_STRUCT_UNLINKATDIR {
	int ret_fslnx;
	unsigned int Linux_dirfd;
	unsigned int *phy_pItemBuf;
	unsigned int ItemNum;
	unsigned int bUseIoctl;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
};

struct FSLINUX_STRUCT_LOCKDIR {
	int ret_fslnx;
	unsigned int Linux_dirfd;
	unsigned int *phy_pItemBuf;
	unsigned int ItemNum;
	int bLock;
	unsigned int bUseIoctl;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
};

struct FSLINUX_STRUCT_IOCTL_CHECK {
	int ret_bValid;
	FSLINUX_IOCTL_TYPE IoctlType;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
};

struct FSLINUX_STRUCT_CLOSEDIR {
	int ret_fslnx;
	unsigned int Linux_dirfd;
	unsigned int bUseIoctl;
};

struct FSLINUX_STRUCT_REWINDDIR {
	int ret_fslnx;
	unsigned int Linux_dirfd;
	unsigned int bUseIoctl;
};

struct FSLINUX_STRUCT_COPYTOBYNAME {
	int ret_fslnx;
	char szSrcPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	char szDstPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	int bDelete;
};

struct FSLINUX_STRUCT_GETDISKINFO {
	int ret_fslnx;
	char szPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned long long ClusSize;
	unsigned long long TotalClus;
	unsigned long long FreeClus;
	unsigned int FsType;
};

struct FSLINUX_STRUCT_FORMATANDLABEL {
	int ret_fslnx;
	char szMountPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	char szNewLabel[FSLINUX_FILENAME_MAX_LENG];
	unsigned int bIsSupportExfat;
};

struct FSLINUX_STRUCT_GETLABEL {
	int ret_fslnx;
	char szMountPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	char szLabel[FSLINUX_FILENAME_MAX_LENG];
};

struct FSLINUX_STRUCT_SETPARAM {
	int ret_fslnx;
	char szMountPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
	unsigned int bIsSupportExfat;
	unsigned int param_id;
	unsigned int value;
};

typedef struct FSLINUX_STRUCT_OPENFILE          FSLINUX_ARG_OPENFILE;
typedef struct FSLINUX_STRUCT_PFILE             FSLINUX_ARG_CLOSEFILE;
typedef struct FSLINUX_STRUCT_RWFILE            FSLINUX_ARG_WRITEFILE;
typedef struct FSLINUX_STRUCT_RWFILE            FSLINUX_ARG_READFILE;
typedef struct FSLINUX_STRUCT_SEEKFILE          FSLINUX_ARG_SEEKFILE;
typedef struct FSLINUX_STRUCT_TELLFILE          FSLINUX_ARG_TELLFILE;
typedef struct FSLINUX_STRUCT_PATH              FSLINUX_ARG_DELETEFILE;
typedef struct FSLINUX_STRUCT_STATFILE          FSLINUX_ARG_STATFILE;
typedef struct FSLINUX_STRUCT_TRUNCFILE         FSLINUX_ARG_TRUNCFILE;
typedef struct FSLINUX_STRUCT_ALLOCFILE         FSLINUX_ARG_ALLOCFILE;
typedef struct FSLINUX_STRUCT_GETFILELEN        FSLINUX_ARG_GETFILELEN;
typedef struct FSLINUX_STRUCT_DATETIME          FSLINUX_ARG_GETDATETIME;
typedef struct FSLINUX_STRUCT_DATETIME          FSLINUX_ARG_SETDATETIME;
typedef struct FSLINUX_STRUCT_PFILE             FSLINUX_ARG_FLUSHFILE;
typedef struct FSLINUX_STRUCT_PATH              FSLINUX_ARG_MAKEDIR;
typedef struct FSLINUX_STRUCT_PATH              FSLINUX_ARG_DELETEDIR;
typedef struct FSLINUX_STRUCT_RENAME            FSLINUX_ARG_RENAMEDIR;
typedef struct FSLINUX_STRUCT_RENAME            FSLINUX_ARG_RENAMEFILE;
typedef struct FSLINUX_STRUCT_MOVEFILE          FSLINUX_ARG_MOVEFILE;
typedef struct FSLINUX_STRUCT_GETATTRIB         FSLINUX_ARG_GETATTRIB;
typedef struct FSLINUX_STRUCT_SETATTRIB         FSLINUX_ARG_SETATTRIB;
typedef struct FSLINUX_STRUCT_OPENDIR           FSLINUX_ARG_OPENDIR;
typedef struct FSLINUX_STRUCT_READDIR           FSLINUX_ARG_READDIR;
typedef struct FSLINUX_STRUCT_UNLINKATDIR       FSLINUX_ARG_UNLINKATDIR;
typedef struct FSLINUX_STRUCT_LOCKDIR           FSLINUX_ARG_LOCKDIR;
typedef struct FSLINUX_STRUCT_IOCTL_CHECK       FSLINUX_ARG_IOCTL_CHECK;
typedef struct FSLINUX_STRUCT_CLOSEDIR          FSLINUX_ARG_CLOSEDIR;
typedef struct FSLINUX_STRUCT_REWINDDIR         FSLINUX_ARG_REWINDDIR;
typedef struct FSLINUX_STRUCT_COPYTOBYNAME      FSLINUX_ARG_COPYTOBYNAME;
typedef struct FSLINUX_STRUCT_GETDISKINFO       FSLINUX_ARG_GETDISKINFO;
typedef struct FSLINUX_STRUCT_FORMATANDLABEL    FSLINUX_ARG_FORMATANDLABEL;
typedef struct FSLINUX_STRUCT_GETLABEL          FSLINUX_ARG_GETLABEL;
typedef struct FSLINUX_STRUCT_SETPARAM          FSLINUX_ARG_SETPARAM;

#endif //_FSLINUX_H_
