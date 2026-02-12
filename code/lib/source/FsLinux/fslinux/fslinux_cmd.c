#define _GNU_SOURCE //define this for O_DIRECT open flag
#define  _LARGEFILE64_SOURCE
#define  _FILE_OFFSET_BITS 64
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/magic.h>
#include <linux/falloc.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "fslinux.h"
#include "fslinux_cmd.h"
#include "fslinux_debug.h"
#if FSLINUX_USE_IPC
#include "fslinux_ipc.h"
#else
#define FsLinux_GetNonCacheAddr(addr)	(addr)
#define FsLinux_GetCacheAddr(addr)		(addr)
#define FSLINUX_IPC_INVALID_ADDR		0xFFFFFFFF
#endif


#define FSLINUX_CMD_FP_MAX  (sizeof(g_CmdFpTbl)/sizeof(FSLINUX_CMDID_SET))
#define FSLINUX_MAXPATH     256
#define FSLINUX_PATH_SEPAR  '/' //separator of linux path

#define FSLINUX_ATTRIB_READONLY     0x01
#define FSLINUX_BLK_WSIZE           (512*1024)

//definition of fs fat ioctl
#define VFAT_IOCTL_READDIR_MSDOS    _IOR('r', 3, struct kernel_dirent[2])
#define FAT_IOCTL_GET_ATTRIBUTES    _IOR('r', 0x10, long)
#define FAT_IOCTL_SET_ATTRIBUTES    _IOW('r', 0x11, long)
#define VFAT_IOCTL_SET_DELAY_SYNC   _IOR('r', 0x18, long)

#define EXFAT_IOCTL_READDIR_DIRECT _IOR('r', 0x13, DIR_ENTRY_T)
#define EXFAT_IOCTL_SET_DELAY_SYNC _IOR('r', 0x14, unsigned int)

#define EXFAT_SUPER_MAGIC       (0x2011BAB0L)

#define KFS_STRCPY(dst, src, dst_size)  do { strncpy(dst, src, dst_size-1); dst[dst_size-1] = '\0'; } while(0)

#define FSLINUX_MAX(a,b) ((a)>(b)?(a):(b))
#define FSLINUX_IOCTL_BUFSIZE	FSLINUX_MAX(sizeof(DIR_ENTRY_T), sizeof(struct kernel_dirent[2]))

//mbr parsing supplement
typedef struct master_boot_record_entry {
	unsigned char record[446];
} MASTER_BOOT_RECORD_T; //446

typedef struct partition_table_entry {
	unsigned char boot_indicator;
	unsigned char starting_head;
	unsigned char starting_sector_cylinder[2];
	unsigned char system_id;
	unsigned char ending_head;
	unsigned char ending_sector_cylinder[2];
	unsigned int relative_sector;
	unsigned int total_sector;
} PARTITION_TABLE_T; //16

typedef struct signature_word_entry {
	unsigned char word1;
	unsigned char word2;
} SIGNATURE_WORD_T; //2

typedef struct __attribute__ ((packed)) mbr_info_entry {
	MASTER_BOOT_RECORD_T master_boot; //446
	PARTITION_TABLE_T partition[4]; //16*4
	SIGNATURE_WORD_T signature; //2
} MBR_INFO_T; //512

//pbr parsing supplement
typedef struct pbr_common_info_entry {
	unsigned char	ignored[3];	/* Boot strap short or near jump */
	unsigned char	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	unsigned char	sector_size[2];	/* bytes per logical sector */
	unsigned char	sec_per_clus;	/* sectors/cluster */
	unsigned short	reserved;	/* reserved sectors */
	unsigned char	fats;		/* number of FATs */
	unsigned char	dir_entries[2];	/* root directory entries */
	unsigned char	sectors[2];	/* number of sectors */
	unsigned char	media;		/* media code */
	unsigned short	fat_length;	/* sectors/FAT */
	unsigned short	secs_track;	/* sectors per track */
	unsigned short	heads;		/* number of heads */
	unsigned int	hidden;		/* hidden sectors (unused) */
	unsigned int	total_sect;	/* number of sectors (if sectors == 0) */
} PBR_COMMON_T; //36

typedef struct pbr_info_entry {
	PBR_COMMON_T pbr_common; //0-35
	union {
		struct {
			/*  Extended BPB Fields for FAT16 */
			unsigned char	drive_number;	/* Physical drive number */
			unsigned char	state;		/* undocumented, but used
						   for mount state. */
			unsigned char	signature;  /* extended boot signature */
			unsigned char	vol_id[4];	/* volume ID */
			unsigned char	vol_label[11];	/* volume label */
			unsigned char	fs_type[8];		/* file system type */
			unsigned char	resv_for_system_use[58];
			/* other fields are not added here */
		} fat16; //36-119
		struct {
			/* only used by FAT32 */
			unsigned int	length;		/* sectors/FAT */
			unsigned short	flags;		/* bit 8: fat mirroring,
						   low 4: active fat */
			unsigned char	version[2];	/* major, minor filesystem
						   version */
			unsigned int	root_cluster;	/* first cluster in
						   root directory */
			unsigned short	info_sector;	/* filesystem info sector */
			unsigned short	backup_boot;	/* backup boot sector */
			unsigned short	reserved2[6];	/* Unused */
			/* Extended BPB Fields for FAT32 */
			unsigned char	drive_number;   /* Physical drive number */
			unsigned char    state;       	/* undocumented, but used
						   for mount state. */
			unsigned char	signature;  /* extended boot signature */
			unsigned char	vol_id[4];	/* volume ID */
			unsigned char	vol_label[11];	/* volume label */
			unsigned char	fs_type[8];		/* file system type */
			unsigned char	resv_for_system_use[30];
			/* other fields are not added here */
		} fat32; //36-119
		struct {
			/* only used by exFAT */
			unsigned char	must_be_zero[28]; //actual 11-63 here 36-63
			unsigned char	vol_offset[8];
			unsigned char	vol_length[8];
			unsigned char	fat_offset[4];
			unsigned char	fat_length[4];
			unsigned char	clu_offset[4];
			unsigned char	clu_count[4];
			unsigned char	root_cluster[4];
			unsigned char	vol_serial[4];
			unsigned char	fs_version[2];
			unsigned char	vol_flags[2];  //has volume dirty bit
			unsigned char	sector_size_bits;
			unsigned char	sectors_per_clu_bits;
			unsigned char	num_fats;
			unsigned char	phy_drv_no;
			unsigned char	perc_in_use;
			unsigned char	reserved2[7];
			/* other fields are not added here */
		} exfat; //36-119
	};
} PBR_INFO_T; //120

//linux kernel fs/fat
struct kernel_dirent {
    long            d_ino;
    long            d_off;
    unsigned short  d_reclen;
    char            d_name[256];
};

//linux kernel supplement fs/exfat ---------- begin
#define MAX_CHARSET_SIZE        3
#define MAX_NAME_LENGTH         256
#define DOS_NAME_LENGTH         11

typedef struct {
	unsigned short      Year;
	unsigned short      Month;
	unsigned short      Day;
	unsigned short      Hour;
	unsigned short      Minute;
	unsigned short      Second;
	unsigned short      MilliSecond;
} DATE_TIME_T;

typedef struct {
	char        Name[MAX_NAME_LENGTH * MAX_CHARSET_SIZE];
	char        ShortName[DOS_NAME_LENGTH + 2];
	unsigned long      Attr;
	unsigned long long Size;
	unsigned long      NumSubdirs;
	DATE_TIME_T CreateTimestamp;
	DATE_TIME_T ModifyTimestamp;
	DATE_TIME_T AccessTimestamp;
} DIR_ENTRY_T;
//linux kernel supplement fs/exfat ---------- end

typedef void (*FSLINUX_CMDID_FP)(unsigned int);

typedef struct{
    FSLINUX_CMDID CmdId;
    FSLINUX_CMDID_FP pFunc;
}FSLINUX_CMDID_SET;

static void FsLinux_CmdId_OpenFile(unsigned int ArgAddr);
static void FsLinux_CmdId_CloseFile(unsigned int ArgAddr);
static void FsLinux_CmdId_ReadFile(unsigned int ArgAddr);
static void FsLinux_CmdId_WriteFile(unsigned int ArgAddr);
static void FsLinux_CmdId_SeekFile(unsigned int ArgAddr);
static void FsLinux_CmdId_TellFile(unsigned int ArgAddr);
static void FsLinux_CmdId_DeleteFile(unsigned int ArgAddr);
static void FsLinux_CmdId_StatFile(unsigned int ArgAddr);
static void FsLinux_CmdId_TruncFile(unsigned int ArgAddr);
static void FsLinux_CmdId_AllocFile(unsigned int ArgAddr);
static void FsLinux_CmdId_GetFileLen(unsigned int ArgAddr);
static void FsLinux_CmdId_GetDateTime(unsigned int ArgAddr);
static void FsLinux_CmdId_SetDateTime(unsigned int ArgAddr);
static void FsLinux_CmdId_FlushFile(unsigned int ArgAddr);
static void FsLinux_CmdId_MakeDir(unsigned int ArgAddr);
static void FsLinux_CmdId_DeleteDir(unsigned int ArgAddr);
static void FsLinux_CmdId_RenameDir(unsigned int ArgAddr);
static void FsLinux_CmdId_RenameFile(unsigned int ArgAddr);
static void FsLinux_CmdId_MoveFile(unsigned int ArgAddr);
static void FsLinux_CmdId_GetAttrib(unsigned int ArgAddr);
static void FsLinux_CmdId_SetAttrib(unsigned int ArgAddr);
static void FsLinux_CmdId_OpenDir(unsigned int ArgAddr);
static void FsLinux_CmdId_ReadDir(unsigned int ArgAddr);
static void FsLinux_CmdId_UnlinkatDir(unsigned int ArgAddr);
static void FsLinux_CmdId_LockDir(unsigned int ArgAddr);
static void FsLinux_CmdId_CloseDir(unsigned int ArgAddr);
static void FsLinux_CmdId_RewindDir(unsigned int ArgAddr);
static void FsLinux_CmdId_CopyToByName(unsigned int ArgAddr);
static void FsLinux_CmdId_GetDiskInfo(unsigned int ArgAddr);
static void FsLinux_CmdId_FormatAndLabel(unsigned int ArgAddr);
static void FsLinux_CmdId_GetLabel(unsigned int ArgAddr);
static void FsLinux_CmdId_SetParam(unsigned int ArgAddr);

//the order shoule be the same as FSLINUX_CMDID structure
static const FSLINUX_CMDID_SET g_CmdFpTbl[] = {
{FSLINUX_CMDID_GET_VER_INFO, NULL},
{FSLINUX_CMDID_GET_BUILD_DATE, NULL},
{FSLINUX_CMDID_OPENFILE, FsLinux_CmdId_OpenFile},
{FSLINUX_CMDID_CLOSEFILE, FsLinux_CmdId_CloseFile},
{FSLINUX_CMDID_READFILE, FsLinux_CmdId_ReadFile},
{FSLINUX_CMDID_WRITEFILE, FsLinux_CmdId_WriteFile},
{FSLINUX_CMDID_SEEKFILE, FsLinux_CmdId_SeekFile},
{FSLINUX_CMDID_TELLFILE, FsLinux_CmdId_TellFile},
{FSLINUX_CMDID_DELETEFILE, FsLinux_CmdId_DeleteFile},
{FSLINUX_CMDID_STATFILE, FsLinux_CmdId_StatFile},
{FSLINUX_CMDID_GETFILELEN, FsLinux_CmdId_GetFileLen},
{FSLINUX_CMDID_GETDATETIME, FsLinux_CmdId_GetDateTime},
{FSLINUX_CMDID_SETDATETIME, FsLinux_CmdId_SetDateTime},
{FSLINUX_CMDID_FLUSHFILE, FsLinux_CmdId_FlushFile},
{FSLINUX_CMDID_MAKEDIR, FsLinux_CmdId_MakeDir},
{FSLINUX_CMDID_DELETEDIR, FsLinux_CmdId_DeleteDir},
{FSLINUX_CMDID_RENAMEDIR, FsLinux_CmdId_RenameDir},
{FSLINUX_CMDID_RENAMEFILE, FsLinux_CmdId_RenameFile},
{FSLINUX_CMDID_MOVEFILE, FsLinux_CmdId_MoveFile},
{FSLINUX_CMDID_GETATTRIB, FsLinux_CmdId_GetAttrib},
{FSLINUX_CMDID_SETATTRIB, FsLinux_CmdId_SetAttrib},
{FSLINUX_CMDID_OPENDIR, FsLinux_CmdId_OpenDir},
{FSLINUX_CMDID_READDIR, FsLinux_CmdId_ReadDir},
{FSLINUX_CMDID_UNLINKATDIR, FsLinux_CmdId_UnlinkatDir},
{FSLINUX_CMDID_LOCKDIR, FsLinux_CmdId_LockDir},
{FSLINUX_CMDID_CLOSEDIR, FsLinux_CmdId_CloseDir},
{FSLINUX_CMDID_REWINDDIR, FsLinux_CmdId_RewindDir},
{FSLINUX_CMDID_COPYTOBYNAME, FsLinux_CmdId_CopyToByName},
{FSLINUX_CMDID_GETDISKINFO, FsLinux_CmdId_GetDiskInfo},
{FSLINUX_CMDID_FORMATANDLABEL, FsLinux_CmdId_FormatAndLabel},
{FSLINUX_CMDID_GETLABEL, FsLinux_CmdId_GetLabel},
{FSLINUX_CMDID_ACKPID, NULL},
{FSLINUX_CMDID_FINISH, NULL},
{FSLINUX_CMDID_TRUNCFILE, FsLinux_CmdId_TruncFile},
{FSLINUX_CMDID_IPC_DOWN, NULL},
{FSLINUX_CMDID_ALLOCFILE, FsLinux_CmdId_AllocFile},
{FSLINUX_CMDID_SETPARAM, FsLinux_CmdId_SetParam},
};

int fslinux_cmd_Init(void)
{
    int CmdId;

    //check the command table to make sure the table is valid
    for(CmdId = 0; CmdId < FSLINUX_CMDID_MAX_ID; CmdId++)
    {
        if(CmdId != (int)g_CmdFpTbl[CmdId].CmdId)
        {
            DBG_ERR("CmdId(%d) and TableCmdId(%d) not equal\r\n", CmdId, g_CmdFpTbl[CmdId].CmdId);
            return -1;
        }
    }
    return 0;
}

int fslinux_cmd_DoCmd(const FSLINUX_IPCMSG *pMsgRcv)
{
    int CmdId;
    unsigned ArgAddr;

    DBG_IND("begin %c %d\r\n", pMsgRcv->Drive, pMsgRcv->CmdId);

    CmdId = pMsgRcv->CmdId;
    if(CmdId >= (int)FSLINUX_CMD_FP_MAX || CmdId < 0)
    {
        DBG_ERR("CmdId(%d) should be 0~%d\r\n", CmdId, FSLINUX_CMD_FP_MAX);
        return -1;
    }

    if(CmdId != (int)(g_CmdFpTbl[CmdId].CmdId))
    {
        DBG_ERR("CmdId(%d) and TableCmdId(%d) not equal\r\n", CmdId, g_CmdFpTbl[CmdId].CmdId);
        return -1;
    }

    ArgAddr = FsLinux_GetNonCacheAddr(pMsgRcv->phy_ArgAddr);
    if(FSLINUX_IPC_INVALID_ADDR == ArgAddr)
    {
        DBG_ERR("Get non-cached addr failed, phy_ArgAddr = 0x%X\r\n", pMsgRcv->phy_ArgAddr);
        return -1;
    }

    if(g_CmdFpTbl[CmdId].pFunc)
         g_CmdFpTbl[CmdId].pFunc(ArgAddr);

    DBG_IND("end %c %d\r\n", pMsgRcv->Drive, pMsgRcv->CmdId);
    return 0;//Always return 0 for success. The return value of API is passed by shared memory.
}

static int fslinux_get_gmtdiff_minute(void)
{
	time_t time_cur = time(NULL);
	struct tm *time_local;

	time_local = localtime(&time_cur);
	if (NULL == time_local) {
		DBG_ERR("get localtime failed\r\n");
		return 0;
	}

	DBG_IND("tm_gmtoff %d (minutes)\r\n", time_local->tm_gmtoff / 60);

	return (time_local->tm_gmtoff / 60);
}

static void fslinux_GetYMDHMS(const time_t *timep, unsigned int YMDHMS[6])
{
    struct tm tm;

    localtime_r(timep, &tm);

    YMDHMS[0] = tm.tm_year+1900;
    YMDHMS[1] = tm.tm_mon+1;
    YMDHMS[2] = tm.tm_mday;
    YMDHMS[3] = tm.tm_hour;
    YMDHMS[4] = tm.tm_min;
    YMDHMS[5] = tm.tm_sec;

    DBG_IND("Date: %d/%d/%d, Time: %d:%d:%d\r\n", YMDHMS[0], YMDHMS[1], YMDHMS[2], YMDHMS[3], YMDHMS[4], YMDHMS[5]);
}

static void fslinux_GetFatDateTime(const time_t *timep, unsigned short *pFatDate, unsigned short *pFatTime, unsigned char *pFatTime10ms)
{
    struct tm tm;
    int year;
    int month;
    int mday;
    int hour;
    int min;
    int sec;

    localtime_r(timep, &tm);

    year = tm.tm_year+1900;
    month = tm.tm_mon+1;
    mday = tm.tm_mday;
    hour = tm.tm_hour;
    min = tm.tm_min;
    sec = tm.tm_sec;

    DBG_IND("Date: %d/%d/%d, Time: %d:%d:%d\r\n", year, month, mday, hour, min, sec);

    //for FAT year, it only has 7 bits (max is 127), max year = 1980+127 = 2107
    if(year < 1980)
        year = 1980;
    else if(year > 2107)
        year = 2107;

    if(pFatDate)
        *pFatDate = (unsigned short)(mday | month<<5 | (year-1980)<<9);

    if(pFatTime)
        *pFatTime = (unsigned short)(sec>>1 | min<<5 | hour<<11);

    if(pFatTime10ms)
        *pFatTime10ms = (unsigned char)(sec&0x1)*100;//tm->tm_sec = 1 (sec) = 100 (10ms)
}

time_t fslinux_GetLinuxSec(unsigned int YMDHMS[6])
{
    struct tm tm = {0};
    time_t ret_mktime;

    tm.tm_year = YMDHMS[0] - 1900;
    tm.tm_mon = YMDHMS[1] - 1;
    tm.tm_mday = YMDHMS[2];
    tm.tm_hour = YMDHMS[3];
    tm.tm_min = YMDHMS[4];
    tm.tm_sec = YMDHMS[5];

    ret_mktime = mktime(&tm);
    return ret_mktime;
}

static unsigned short fslinux_TimeSys2Fat(unsigned short Hour, unsigned short Min, unsigned short Sec)
{
    unsigned short ret;
    ret = ((Sec >> 1) & 0x001F) | ((Min << 5) & 0x07E0) | ((Hour << 11) & 0xF800);
    return ret;
}

static unsigned short fslinux_DateSys2Fat(unsigned short Year, unsigned short Month, unsigned short Day)
{
    unsigned short ret;
    ret = (Day & 0x001F) | ((Month << 5) & 0x01E0) | (((Year - 1980) << 9) & 0xFE00);
    return ret;
}

static unsigned char fslinux_GetFatAttrib(mode_t mode)
{
    unsigned char FatAttrib = 0;

    if (S_ISDIR(mode)) {
		FatAttrib |= FSLINUX_ATTRIB_DIRECTORY;
    } else {
        FatAttrib |= FSLINUX_ATTRIB_ARCHIVE;
    }
	if (!(mode & (S_IWUSR|S_IWGRP|S_IWOTH))) {
		FatAttrib |= FSLINUX_ATTRIB_READONLY;
	}
    return FatAttrib;
}

void fslinux_toupper(char *str, int count)
{
	//Main statement
	while (*str != '\0' && count != 0) {
		if ((*str >= 'a') && (*str <= 'z')) {
			*str -= 'a' - 'A';
		}
		str++;
		count--;
	}
}

static unsigned int g_fslinux_skip_sync = 0;

int fslinux_set_skip_sync(unsigned int state)
{
	if (state) {
		DBG_DUMP("need to handle i/o (f)sync since skip sync ON.\r\n");
		g_fslinux_skip_sync = 1;
	} else {
		DBG_DUMP("auto i/o (f)sync since skip sync OFF.\r\n");
		g_fslinux_skip_sync = 0;
	}
	return 0;
}

unsigned int fslinux_is_skip_sync(void)
{
	if (g_fslinux_skip_sync) {
		return 1;
	} else {
		return 0;
	}
}

//static int fslinux_ConvertToFATName(char *pDstSFN83, const char* pSrc)
static int fslinux_ConvertToFATName(char *pDstMain, char *pDstExt, const char* pSrc)
{
    unsigned int SrcLen;
    unsigned int MainLen, ExtLen;
    const char *pDot;

    //clear the result first
    memset(pDstMain, ' ', 8);
    memset(pDstExt, ' ', 3);

    SrcLen = strlen(pSrc);

    if(0 == strcmp(pSrc, ".") || 0 == strcmp(pSrc, ".."))
    {
        memcpy(pDstMain, pSrc, SrcLen);
        return 0;
    }

    //find dot
    pDot = strrchr(pSrc, '.');

    if(pDot)
    {//have ext name
        MainLen = (unsigned int)pDot - (unsigned int)(pSrc);
        ExtLen = SrcLen - MainLen - 1;//-1 is for '.'
    }
    else
    {
        MainLen = SrcLen;
        ExtLen = 0;
    }

    if(MainLen < 9 && MainLen > 0) {
		memcpy(pDstMain, pSrc, MainLen);
		fslinux_toupper(pDstMain, MainLen);
    } else
        memcpy(pDstMain, "LFNERR~1", 8);

    if(ExtLen < 4 && ExtLen > 0) {
		memcpy(pDstExt, pDot+1, ExtLen);
		fslinux_toupper(pDstMain, ExtLen);
    }

    return 0;
}

static int fslinux_FillFindData(FSLINUX_FIND_DATA *pFindData, const struct stat *pStat, const char *pFileName)
{
    //FindData: the fields that remain unchanged will show by comments
    //fslinux_ConvertToFATName(pFindData->FATMainName, pFileName);
    fslinux_ConvertToFATName(pFindData->FATMainName, pFindData->FATExtName, pFileName);
    pFindData->attrib = fslinux_GetFatAttrib(pStat->st_mode);
    //[Reserved]
    fslinux_GetFatDateTime(&(pStat->st_ctime), &(pFindData->creDate), &(pFindData->creTime), &(pFindData->creSecond));
    fslinux_GetFatDateTime(&(pStat->st_atime), &(pFindData->lastAccessDate), NULL, NULL);
    fslinux_GetFatDateTime(&(pStat->st_mtime), &(pFindData->lastWriteDate), &(pFindData->lastWriteTime), NULL);
    //[FstClusHI]
    //[FstClusLO]
    pFindData->fileSize = (pStat->st_size)&0xFFFFFFFF;
#if 0
    pFindData->fileSizeHI = (pStat->st_size >> 32)&0xFFFFFFFF;
#else
	pFindData->fileSizeHI= 0;
#endif

    KFS_STRCPY(pFindData->filename, pFileName, sizeof(pFindData->filename));
    DBG_IND("fn: %s\r\n", pFindData->filename);
    //[PartitionType]
    //[bNoFatChain]
    //[ClusterCount]
    return 0;
}

// only for reference
void fslinux_sample_readdir(char *pPath)
{
	unsigned long fd;
	DBG_DUMP("pPath=%s\r\n", pPath);

	// opendir
	DIR *ret_opendir = opendir(pPath);
	if(NULL == ret_opendir)
	{
		DBG_IND("ret_opendir: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
		return;
	}
	fd = (unsigned long)ret_opendir;

	// readdir
	int bLastRound = 0;
	int find_num = 0;
	unsigned long find_size = 0;
	{
		struct dirent *entry = NULL;
		DIR *pDir = (DIR *)fd;

		while (bLastRound == 0)
		{
			//Linux man:
			//The readdir_r() function returns 0 on success. On error, it returns a positive error number (listed under ERRORS).
			//If the end of the directory stream is reached, readdir_r() returns 0, and returns NULL in *result.

			entry = readdir(pDir);

			if(NULL == entry)
			{//reach the end
				bLastRound = 1;
				break;
			}

			if (0 == strncmp(entry->d_name, ".", 1) || 0 == strncmp(entry->d_name, "..", 2))
			{
				continue;
			}

			find_num ++;

			/* if need more information */
			{
				struct stat TmpStat = {0};
				int curfd = dirfd(pDir);
				if(0 != fstatat(curfd, entry->d_name, &TmpStat, 0))
				{
					DBG_ERR("fstatat failed, name = %s, errno = %d, errmsg = %s\r\n", entry->d_name, errno, strerror(errno));
					break;
				}

				char            filename[FSLINUX_LONGFILENAME_MAX_LENG + 1] = {0};
				unsigned char   attrib = fslinux_GetFatAttrib(TmpStat.st_mode);
				unsigned long   fileSize = TmpStat.st_size;
				KFS_STRCPY(filename, entry->d_name, sizeof(filename));
				DBG_IND("find:(%s, 0x%x, 0x%x)\r\n", filename, attrib, fileSize);

				if (attrib & 0x20) //file
					find_size += fileSize;
			}
		}

		DBG_DUMP("find_num = %d, find_size = 0x%lx\r\n", (int)find_num, (unsigned long)find_size);
	}

	// closedir
	if(0 != closedir((DIR *)fd))
		DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
}
// only for reference
void fslinux_sample_readdir_vfat(char *pPath)
{
	unsigned long fd;

	// open
	int ret_fd = open(pPath, O_RDONLY);
	if(-1 == ret_fd)
	{
		DBG_IND("open: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
		return;
	}
	fd = (unsigned long)ret_fd;

	// ioctl
	int bLastRound = 0;
	int find_num = 0;
	unsigned long find_size = 0;
	{
		unsigned char ioctl_buf[FSLINUX_IOCTL_BUFSIZE];
		struct kernel_dirent *kdirent = (struct kernel_dirent *)ioctl_buf;
		int ret_ioctl = 0;

		while (bLastRound == 0)
		{
			ret_ioctl = ioctl((int)fd, VFAT_IOCTL_READDIR_MSDOS, (long)kdirent);
			if(-1 == ret_ioctl)
			{
				DBG_ERR("ioctl failed, errno = %d, errmsg = %s\r\n", errno, strerror(errno));
				break;
			}

			if(0 == kdirent[0].d_reclen)
			{
				bLastRound = 1;
				break;
			}

			find_num ++;

			/* if need more information */
			{
				int FullNameLen;
				FSLINUX_FIND_DATA FindData = {0};
				//get the full name
				FullNameLen = strlen(kdirent[0].d_name);
				if(FullNameLen > FSLINUX_LONGFILENAME_MAX_LENG)
				{
					DBG_ERR("FullNameLen(%d) > Max(%d)\r\n", FullNameLen, FSLINUX_LONGFILENAME_MAX_LENG);
				}
				KFS_STRCPY(FindData.filename, kdirent[0].d_name, sizeof(FindData.filename));
				DBG_IND("fn=%s\r\n", FindData.filename);

				//get the 32-byte entry data
				memcpy(&(FindData), kdirent[1].d_name, 32);

				if (FindData.attrib & 0x20) //file
					find_size += FindData.fileSize;
				}
		}

		DBG_DUMP("find_num = %d\r\n", (int)find_num);
	}

	// close
	if(0 != close((int)fd))
		DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
}
// only for reference
void fslinux_sample_readdir_exfat(char *pPath)
{
	unsigned long fd;
	DBG_DUMP("pPath=%s\r\n", pPath);

	// open
	int ret_fd = open(pPath, O_RDONLY);
	if(-1 == ret_fd)
	{
		DBG_IND("open: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
		return;
	}
	fd = (unsigned long)ret_fd;

	// ioctl
	int bLastRound = 0;
	int find_num = 0;
	unsigned long find_size = 0;
	{
		unsigned char ioctl_buf[FSLINUX_IOCTL_BUFSIZE];
		DIR_ENTRY_T *de = (DIR_ENTRY_T *)ioctl_buf;
		int ret_ioctl = 0;

		while (bLastRound == 0)
		{
			ret_ioctl = ioctl((int)fd, EXFAT_IOCTL_READDIR_DIRECT, (long)de);
			if(-1 == ret_ioctl)
			{
				DBG_ERR("ioctl failed, errno = %d, errmsg = %s\r\n", errno, strerror(errno));
				break;
			}

			if(0 == de->Name[0])
			{
				bLastRound = 1;
				break;
			}

			find_num ++;

			/* if need more information */
			{
				char            filename[FSLINUX_LONGFILENAME_MAX_LENG + 1] = {0};
				unsigned char   attrib = de->Attr;
				unsigned long   fileSize = (unsigned long)de->Size;
				KFS_STRCPY(filename, de->Name, sizeof(filename));
				DBG_IND("find:(%s, 0x%x, 0x%x)\r\n", filename, attrib, fileSize);

				if (attrib & 0x20) //file
					find_size += fileSize;
			}
		}

		DBG_DUMP("find_num = %d, find_size = 0x%lx\r\n", (int)find_num, (unsigned long)find_size);
	}

	// close
	if(0 != close((int)fd))
		DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
}

//fslinux_StatNextEntry will return the item number gotten
//the return number should be the same as ItemNum_Req, unless it reaches the folder end
static int fslinux_StatNextEntry(unsigned int fd, FSLINUX_FIND_DATA_ITEM *pItemBuf, unsigned int *pItemNum, unsigned int bUseIoctl)
{
    FSLINUX_FIND_DATA_ITEM *pCurItem;
    unsigned int ItemNum_Req, ItemNum_Got = 0;
    int bLastRound = 0;
    unsigned char ioctl_buf[FSLINUX_IOCTL_BUFSIZE];

    pCurItem = pItemBuf;
    ItemNum_Req = *pItemNum;

    DBG_IND("bUseIoctl(%d)\r\n", bUseIoctl);

    if(FSLINUX_IOCTL_MSDOS_ENTRY == bUseIoctl)
    {
        struct kernel_dirent *kdirent = (struct kernel_dirent *)ioctl_buf;
        int ret_ioctl = 0;
        int FullNameLen;

        for(pCurItem = pItemBuf; ItemNum_Got < ItemNum_Req;  pCurItem++, ItemNum_Got++)
        {
            ret_ioctl = ioctl(fd, VFAT_IOCTL_READDIR_MSDOS, (long)kdirent);
            if(-1 == ret_ioctl)
            {
                DBG_ERR("ioctl failed, errno = %d, errmsg = %s\r\n", errno, strerror(errno));
                break;
            }

            if(0 == kdirent[0].d_reclen)
            {
                bLastRound = 1;
                break;
            }

            //get the full name
            FullNameLen = strlen(kdirent[0].d_name);
            if(FullNameLen > FSLINUX_LONGFILENAME_MAX_LENG)
            {
                DBG_ERR("FullNameLen(%d) > Max(%d)\r\n", FullNameLen, FSLINUX_LONGFILENAME_MAX_LENG);
                return FSLINUX_STA_ERROR;
            }
            //strcpy(pCurItem->FindData.filename, kdirent[0].d_name);
            KFS_STRCPY(pCurItem->FindData.filename, kdirent[0].d_name, sizeof(pCurItem->FindData.filename));
            DBG_IND("fn=%s\r\n", pCurItem->FindData.filename);

            //get the 32-byte entry data
            memcpy(&(pCurItem->FindData), kdirent[1].d_name, 32);

            //set high byte to 0 for FAT32 (VFAT)
            pCurItem->FindData.fileSizeHI = 0;

            //reset the flag
            pCurItem->flag = 0;
        }
    }
    else if (FSLINUX_IOCTL_EXFAT_ENTRY == bUseIoctl)
    {
        DIR_ENTRY_T *de = (DIR_ENTRY_T *)ioctl_buf;
        int ret_ioctl = 0;
        int FullNameLen;

        for(pCurItem = pItemBuf; ItemNum_Got < ItemNum_Req;  pCurItem++, ItemNum_Got++)
        {
            ret_ioctl = ioctl(fd, EXFAT_IOCTL_READDIR_DIRECT, (long)de);
            if(-1 == ret_ioctl)
            {
                DBG_ERR("ioctl failed, errno = %d, errmsg = %s\r\n", errno, strerror(errno));
                break;
            }

            if(0 == de->Name[0])
            {
                bLastRound = 1;
                break;
            }

            //get the full name
            FullNameLen = strlen(de->Name);
            if(FullNameLen > FSLINUX_LONGFILENAME_MAX_LENG)
            {
                DBG_ERR("FullNameLen(%d) > Max(%d)\r\n", FullNameLen, FSLINUX_LONGFILENAME_MAX_LENG);
                return FSLINUX_STA_ERROR;
            }
            //strcpy(pCurItem->FindData.filename, kdirent[0].d_name);
            memset(&pCurItem->FindData, 0, sizeof(pCurItem->FindData));

            fslinux_ConvertToFATName(pCurItem->FindData.FATMainName, pCurItem->FindData.FATExtName, de->Name);
            pCurItem->FindData.attrib = de->Attr;
            //pCurItem->FindData.Reserved
            pCurItem->FindData.creSecond = de->CreateTimestamp.Second;
            pCurItem->FindData.creTime = fslinux_TimeSys2Fat(de->CreateTimestamp.Hour, de->CreateTimestamp.Minute, de->CreateTimestamp.Second);
            pCurItem->FindData.creDate = fslinux_DateSys2Fat(de->CreateTimestamp.Year, de->CreateTimestamp.Month, de->CreateTimestamp.Day);
            pCurItem->FindData.lastAccessDate = fslinux_DateSys2Fat(de->AccessTimestamp.Year, de->AccessTimestamp.Month, de->AccessTimestamp.Day);
            //pCurItem->FindData.FstClusHI
            pCurItem->FindData.lastWriteTime = fslinux_TimeSys2Fat(de->ModifyTimestamp.Hour, de->ModifyTimestamp.Minute, de->ModifyTimestamp.Second);
            pCurItem->FindData.lastWriteDate = fslinux_DateSys2Fat(de->ModifyTimestamp.Year, de->ModifyTimestamp.Month, de->ModifyTimestamp.Day);
            //pCurItem->FindData.FstClusLO
            pCurItem->FindData.fileSize = (unsigned int)de->Size;
            pCurItem->FindData.fileSizeHI = de->Size >> 32;

            KFS_STRCPY(pCurItem->FindData.filename, de->Name, sizeof(pCurItem->FindData.filename));
            DBG_IND("fn2=%s\r\n", pCurItem->FindData.filename);

            //reset the flag
            pCurItem->flag = 0;
        }
    }
    else
    {
#if defined (__LINUX_USER__)
#else
        struct dirent readdir_r_entry;
        struct dirent *readdir_r_result = NULL;
        int ret_readdir_r = 0;
#endif
		struct dirent *entry = NULL;
        struct stat TmpStat = {0};
        DIR *pDir = (DIR *)fd;
        int curfd = dirfd(pDir);

        for(pCurItem = pItemBuf; ItemNum_Got < ItemNum_Req;  pCurItem++, ItemNum_Got++)
        {
            //Linux man:
            //The readdir_r() function returns 0 on success. On error, it returns a positive error number (listed under ERRORS).
            //If the end of the directory stream is reached, readdir_r() returns 0, and returns NULL in *result.
#if defined (__LINUX_USER__)
            entry = readdir(pDir);

            if(NULL == entry)
            {//reach the end
                bLastRound = 1;
                break;
            }
#else
            ret_readdir_r = readdir_r(pDir, &readdir_r_entry, &readdir_r_result);
            if(0 != ret_readdir_r)
            {//readdir_r error
                DBG_ERR("ret_readdir_r = %d\r\n", ret_readdir_r);
                break;
            }

            if(NULL == readdir_r_result)
            {//reach the end
                bLastRound = 1;
                break;
            }
            entry = &readdir_r_entry;
#endif
            if(0 != fstatat(curfd, entry->d_name, &TmpStat, 0))
            {
                DBG_ERR("fstatat failed, name = %s, errno = %d, errmsg = %s\r\n", entry->d_name, errno, strerror(errno));
                break;
            }

            fslinux_FillFindData(&(pCurItem->FindData), &TmpStat, entry->d_name);
            pCurItem->flag = 0;//reset the flag
        }
    }

    //last round + got items (NO MORE FILES)
    //last round + no items (ERR to stop)
    //non last round + got items (OK)
    //non last round + no items (ERR to stop)

    if(bLastRound && ItemNum_Got > 0)
    {
        *pItemNum = ItemNum_Got;
        return FSLINUX_STA_FS_NO_MORE_FILES;
    }
    else if(ItemNum_Got == ItemNum_Req)
    {
        *pItemNum = ItemNum_Got;
        return FSLINUX_STA_OK;
    }
    else
    {
        *pItemNum = 0;
        return FSLINUX_STA_ERROR;
    }
}


static int fslinux_get_parent_dir(const char *path, char *out_parent)
{
	const char *pchar;
	int src_len;

	src_len = strlen(path);
	if (src_len < 1)
	{
		DBG_ERR("Invalid path %s\r\n", path);
		return -1;
	}

    if(FSLINUX_PATH_SEPAR == path[src_len-1])
        pchar = path + src_len - 1;
    else
        pchar = path + src_len;

    while(--pchar > path)
    {
        if (FSLINUX_PATH_SEPAR == *pchar)
        {
            unsigned int par_len = (unsigned int)pchar - (unsigned int)path;//without FSLINUX_PATH_SEPAR
            memcpy(out_parent, path, par_len);//Ex: parentDir = "A:\BC"
            out_parent[par_len] = '\0';
            DBG_IND("out_parent = %s, len = %d\r\n", out_parent, par_len);

			return 0;
        }
    }

	return -1;
}

static int fslinux_sync_path(const char *path)
{
	int fd;
	int ret = 0;

	DBG_IND("[%s]\r\n", path);

	fd = open(path, O_RDONLY);
    if(fd < 0) {
		DBG_ERR("Invalid path %s\r\n", path);
        return -1;
    }

	if (-1 == fsync(fd))
    {
        DBG_ERR("fsync %s: errno = %d, errmsg = %s\r\n", path, errno, strerror(errno));
		ret = -1;
    }

    if (-1 == close(fd))
    {
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", path, errno, strerror(errno));
        ret = -1;
    }

	return ret;
}

static int fslinux_mkdir(const char *pPath)
{
    if(0 == mkdir(pPath, 0777))
    {
		char szParent[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};

		if (0 != fslinux_get_parent_dir(pPath, szParent))
		{
			DBG_ERR("get %s parent dir failed\r\n", pPath);
			return -1;
		}

		if (0 != fslinux_sync_path(szParent))
		{
			DBG_ERR("sync %s failed\r\n", szParent);
			return -1;
		}
        return 0;
    }

    if(EEXIST == errno) {
	    struct stat TmpStat = {0};

		if(0 == stat(pPath, &TmpStat))
		{//if the entry exists
		    if(S_ISDIR(TmpStat.st_mode))
		        return 0;//skip if it is DIR
		    else
				DBG_ERR("FILE: %s already exists\r\n", pPath);
		}
		DBG_ERR("stat: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        return -1;

    } else
        DBG_ERR("mkdir: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

    return -1;
}

int fslinux_mkpath(char *pPath)
{
    char *pChar;
    char *pSlash;
    int ret = 0;

    DBG_IND("pPath = %s\r\n", pPath);

    pChar = pPath+1;//skip the root '/'

    while(0 == ret && NULL != (pSlash = strchr(pChar, FSLINUX_PATH_SEPAR)))
    {
        *pSlash = '\0';
        ret = fslinux_mkdir(pPath);
        *pSlash = FSLINUX_PATH_SEPAR;

        pChar = pSlash + 1;
    }

    if (ret == 0)
        ret = fslinux_mkdir(pPath);//last dir

    return (ret);
}

static unsigned int fslinux_ioctl_set_delay_sync(char *pPath, unsigned long bsync)
{
    unsigned int IsIoctlValid = 0;
    int ret_fd;

    DBG_IND("open(%s, O_RDONLY)\r\n", pPath);
    ret_fd = open(pPath, O_RDONLY);
    if(-1 == ret_fd)
    {
        DBG_IND("pPath = %s, errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return IsIoctlValid;
    }

	if(0 == ioctl(ret_fd, VFAT_IOCTL_SET_DELAY_SYNC, bsync)) {
		DBG_IND("pPath(%s), VFAT_IOCTL_SET_DELAY_SYNC %d\r\n", pPath, bsync);
		IsIoctlValid = 1;
	} else if(0 == ioctl(ret_fd, EXFAT_IOCTL_SET_DELAY_SYNC, bsync)) {
		DBG_IND("pPath(%s), EXFAT_IOCTL_SET_DELAY_SYNC %d\r\n", pPath, bsync);
		IsIoctlValid = 1;
	} else {
        DBG_IND("pPath(%s), SET_DELAY_SYNC failed\r\n", pPath);
    }

    DBG_IND("close(0x%X)\r\n", ret_fd);
    if(0 != close(ret_fd))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));

    return IsIoctlValid;
}

static int fslinux_rmdir_recursive(char *pPath, int bRecursive)
{
    char szCurDir[FSLINUX_MAXPATH] = {0};
#if defined (__LINUX_USER__)
#else
    struct dirent readdir_r_entry;
    struct dirent *readdir_r_result = NULL;
#endif
	struct dirent *entry = NULL;
    DIR *pDir;
    int curfd;
    int ret_tmp = 0;
    int iDepth = 0;//0: the original folder, 1: the first child
    int PathLen;
    int bSetDelaySync = 0;

    if(pPath == NULL)
    {
        DBG_ERR("pPath is NULL\r\n");
        return -1;
    }

    bSetDelaySync = fslinux_ioctl_set_delay_sync(pPath, 1);
    //strcpy(szCurDir, pPath);
    KFS_STRCPY(szCurDir, pPath, sizeof(szCurDir));
    PathLen = strlen(szCurDir);

    if(szCurDir[PathLen-1] == FSLINUX_PATH_SEPAR)
        szCurDir[PathLen-1] = '\0';

    pDir = opendir(szCurDir);
    if(NULL == pDir)
    {
        DBG_ERR("opendir(%s): errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
        goto L_fslinux_rmdir_recursive_FAIL;
    }
    curfd = dirfd(pDir);

    while(0 == ret_tmp)
    {
#if defined (__LINUX_USER__)
        entry = readdir(pDir);

        if(NULL == entry)
        {//reach the current folder end

            //close the current folder
            if(0 != closedir(pDir))
            {
                DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
                goto L_fslinux_rmdir_recursive_FAIL;
            }

            //remove the current folder
            DBG_IND("Remove folder %s\r\n", szCurDir);
            ret_tmp = rmdir(szCurDir);
            if(ret_tmp != 0)
                DBG_ERR("rmdir: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

            if(iDepth > 0)
            {//go back to the parent folder
                char *pChar;

                pChar = strrchr(szCurDir, FSLINUX_PATH_SEPAR);
                if(pChar)
                    *pChar = '\0';

                pDir = opendir(szCurDir);
                if(NULL == pDir)
                {
                    DBG_ERR("opendir(%s): errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
                    goto L_fslinux_rmdir_recursive_FAIL;
                }
                curfd = dirfd(pDir);
                iDepth--;
            }
            else
            {//all done;
                //Note: we do not call fslinux_ioctl_set_delay_sync() here,
                //because the original folder is deleted, and the delay_sync option will be clear by driver
                return 0;
            }
        }
#else
        ret_tmp = readdir_r(pDir, &readdir_r_entry, &readdir_r_result);
        if(ret_tmp != 0)
        {
            DBG_ERR("ret_readdir_r = %d\r\n", ret_tmp);
            break;
        }

        if(NULL != readdir_r_result)
        {
            entry = &readdir_r_entry;
        }

        if(NULL == readdir_r_result)
        {//reach the current folder end

            //close the current folder
            if(0 != closedir(pDir))
            {
                DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
                goto L_fslinux_rmdir_recursive_FAIL;
            }

            //remove the current folder
            DBG_IND("Remove folder %s\r\n", szCurDir);
            ret_tmp = rmdir(szCurDir);
            if(ret_tmp != 0)
                DBG_ERR("rmdir: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

            if(iDepth > 0)
            {//go back to the parent folder
                char *pChar;

                pChar = strrchr(szCurDir, FSLINUX_PATH_SEPAR);
                if(pChar)
                    *pChar = '\0';

                pDir = opendir(szCurDir);
                if(NULL == pDir)
                {
                    DBG_ERR("opendir(%s): errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
                    goto L_fslinux_rmdir_recursive_FAIL;
                }
                curfd = dirfd(pDir);
                iDepth--;
            }
            else
            {//all done;
                //Note: we do not call fslinux_ioctl_set_delay_sync() here,
                //because the original folder is deleted, and the delay_sync option will be clear by driver
                return 0;
            }
        }
#endif
        //else if(strcmp(readdir_r_entry.d_name, ".") && strcmp(readdir_r_entry.d_name, ".."))
        else if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
        {//found a valid entry

            if(DT_REG == entry->d_type)
            {//found a file entry
                DBG_IND("Remove file %s/%s\r\n", szCurDir, entry->d_name);
                ret_tmp = unlinkat(curfd, entry->d_name, 0);
                if(ret_tmp != 0)
                {
                    DBG_ERR("unlinkat: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
                    break;
                }
            }
            else if(bRecursive)
            {//found a dir entry
                //close the current folder
                if(0 != closedir(pDir))
                {
                    DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
                    goto L_fslinux_rmdir_recursive_FAIL;
                }

                //enter the child folder
                strcat(szCurDir, "/");
                strcat(szCurDir, entry->d_name);
                pDir = opendir(szCurDir);
                if(NULL == pDir)
                {
                    DBG_ERR("opendir(%s): errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));
                    goto L_fslinux_rmdir_recursive_FAIL;
                }
                curfd = dirfd(pDir);
                iDepth++;
            }
        }
    }

    if(0 != closedir(pDir))
        DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", szCurDir, errno, strerror(errno));

L_fslinux_rmdir_recursive_FAIL:
	if (bSetDelaySync)
		fslinux_ioctl_set_delay_sync(pPath, 0);
    return -1;
}

// Depth 0 ex: "/", ""
// Depth 1 ex: "/mydir1", "/mydir1/"
// Depth 2 ex: "/mydir1/mydir2", "/mydir1/mydir2/"
static int fslinux_GetPathDepth(char *pPath)
{
    char *pChar;
    int bFoundSlash = 0;
    int Depth = 0;

    for(pChar = pPath; *pChar != 0; pChar++)
    {
        if(FSLINUX_PATH_SEPAR == *pChar)
        {
            bFoundSlash = 1;
        }
        else if(bFoundSlash)
        {
            bFoundSlash = 0;
            Depth++;
        }
    }

    return Depth;
}

static int fslinux_GetDevByMountPath(char *pDevPath, const char* pMountPath, int BufSize)
{
    struct mntent *ent;
    FILE *Linux_pFILE;
    int PathLen;

    PathLen = strlen(pMountPath);
    if('/' == pMountPath[PathLen-1])
        PathLen--;//exclude the last '/' for strncmp

    Linux_pFILE = setmntent("/proc/mounts", "r");
    if (NULL == Linux_pFILE) {
        DBG_ERR("setmntent error\n");
        return -1;
    }

    while (NULL != (ent = getmntent(Linux_pFILE)))
    {
        DBG_IND("%s => %s\n", ent->mnt_fsname, ent->mnt_dir);

        if(0 == strncmp(pMountPath, ent->mnt_dir, PathLen))
        {
            strncpy(pDevPath, ent->mnt_fsname, BufSize-1);
            pDevPath[BufSize-1] = '\0';
            endmntent(Linux_pFILE);
            return 0;
        }
    }

    *pDevPath = 0;//clean the result
    endmntent(Linux_pFILE);
    return -1;
}

static int fslinux_IsPathExist(char *path)
{
    int fd;

    fd = open(path, O_RDONLY);
    if(fd < 0)
        return 0;

    if(0 != close(fd))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", path, errno, strerror(errno));

    return 1;
}

int fslinux_find_entry_by_prefix(const char *root_pathp, const char *prefix_namep, char *out_namep, int out_size)
{
    DIR *pDIR;
    struct dirent *entry = NULL;
#if defined (__LINUX_USER__)
#else
    struct dirent readdir_r_entry;
    struct dirent *readdir_r_result = NULL;
    int ret_readdir_r = 0;
#endif
    int prefix_len;
    int bfound = 0;

    *out_namep = '\0';//set empty first
    prefix_len = strlen(prefix_namep);

    pDIR = opendir(root_pathp);
    if(NULL == pDIR)
    {
        DBG_IND("opendir: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        return -1;
    }

    while(!bfound)
    {
#if defined (__LINUX_USER__)
        entry = readdir(pDIR);

        if(NULL == entry)
        {//reach the end
            break;
        }
#else
        ret_readdir_r = readdir_r(pDIR, &readdir_r_entry, &readdir_r_result);
        if(0 != ret_readdir_r)
        {//readdir_r error
            DBG_ERR("ret_readdir_r = %d\r\n", ret_readdir_r);
            break;
        }

        if(NULL == readdir_r_result)
        {//reach the end
            break;
        }
        entry = &readdir_r_entry;
#endif
        if(0 == strncmp(entry->d_name, prefix_namep, prefix_len))
        {
			KFS_STRCPY(out_namep, entry->d_name, out_size);
            bfound = 1;
        }
    }

    if(0 != closedir(pDIR))
        DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", root_pathp, errno, strerror(errno));

    if(bfound)
    {
        DBG_IND("Found %s in %s\r\n", out_namep, root_pathp);
        return 0;
    }
    else
    {
        DBG_IND("Prefix:%s in %s not found\r\n", prefix_namep, root_pathp);
        return -1;
    }
}

static int fslinux_get_sysinfo_by_path(const char *path, char *out_string, int out_size)
{
	int fd;
	ssize_t ret_read;

	if(out_size < 2)
	{
		DBG_ERR("out_size at least 2\r\n");
		return -1;
	}

	memset(out_string, 0, out_size);

	fd = open(path, O_RDONLY);
    if(fd < 0) {
		DBG_ERR("Invalid path %s\r\n", path);
        return -1;
    }

	ret_read = read(fd, out_string, out_size - 1); //-1 for null-terminated

    if(-1 == ret_read) {
		DBG_ERR("read: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        close(fd);
        return -1;
    }

	//since Coverity can not recognize the above null-terminated, set null again	out_string[out_size-1] = '\0';
	out_string[out_size-1] = '\0';

    close(fd);

    DBG_IND("out_string = %s\r\n", out_string);

	return 0;
}

static int fslinux_is_bus0_emmc(void)
{
	#define MMC_SYS_PATH "/sys/bus/mmc/devices"
	#define MMC0BUS_PREFIX "mmc0"
	#define TYPE_STR_MMC "MMC"
	#define TYPE_STR_SD "SD"

	char find_path[64] = {0};//e.g. /sys/devices/platform/nt96660_mmc.0/mmc_host/mmc0/mmc0:b368/block
	char mmc_type[8] = {0};
	char mmc_bga[8] = {0};

	char mmc_bus_name[16] = {0}; //e.g. mmc0:b368

	if(0 != fslinux_find_entry_by_prefix(MMC_SYS_PATH, MMC0BUS_PREFIX, mmc_bus_name, sizeof(mmc_bus_name)))
	{
		DBG_IND("mmc0 bus not found\r\n");
        return 0;
	}

	snprintf(find_path, sizeof(find_path), "%s/%s/type", MMC_SYS_PATH, mmc_bus_name);
	if(0 != fslinux_get_sysinfo_by_path(find_path, mmc_type, sizeof(mmc_type)))
	{
        DBG_ERR("%s not found\r\n", find_path);
        return 0;
    }
	DBG_IND("mmc_type = [%s]\r\n", mmc_type);

	if (0 == strncmp(mmc_type, TYPE_STR_SD, strlen(TYPE_STR_SD))) {
		return 0;
	} else if (0 == strncmp(mmc_type, TYPE_STR_MMC, strlen(TYPE_STR_MMC))) {
		snprintf(find_path, sizeof(find_path), "%s/%s/bga", MMC_SYS_PATH, mmc_bus_name);
		if(0 != fslinux_get_sysinfo_by_path(find_path, mmc_bga, sizeof(mmc_bga))) {
	        DBG_IND("%s not found\r\n", find_path);
	        return 0;
	    }
		DBG_IND("mmc_bga = [%s]\r\n", mmc_bga);

		if (0 == strncmp(mmc_bga, "1", 1))
			return 1; //emmc storage is found
		else
			return 0;
	} else {
		DBG_ERR("unknown mmc_type [%s]\r\n", mmc_type);
		return 0;
	}

	return 0;
}

static int fslinux_GetSD1DevByBus(char *out_path, int out_size)
{
    #define MMC_SYS_PATH "/sys/bus/mmc/devices"
    #define MMC0BUS_PREFIX "mmc0"
    #define MMC0BUS_PREFIX2 "mmc2"
    #define MMCDEV_PREFIX "mmc"

    char find_path[64] = {0};//e.g. /sys/devices/platform/nt96660_mmc.0/mmc_host/mmc0/mmc0:b368/block
    char dev_path[32] = {0};//e.g. /dev/mmcblk0p1, /dev/mmcblk0, /dev/mmcblk1p1, /dev/mmcblk1
	char mmc_bus_name[16] = {0}; //e.g. mmc0:b368
	char mmc_dev_name[16] = {0};
    int bFound = 0, beMMCFound = 0;

    out_path[0] = '\0';//set empty first

	if(0 == fslinux_find_entry_by_prefix(MMC_SYS_PATH, MMC0BUS_PREFIX2, mmc_bus_name, sizeof(mmc_bus_name)))
	{
		DBG_IND("mmc2 found\r\n");
        beMMCFound = 1;
	}else if(0 != fslinux_find_entry_by_prefix(MMC_SYS_PATH, MMC0BUS_PREFIX, mmc_bus_name, sizeof(mmc_bus_name))) //1. find the bus mmc0 to check the card is inserted or not
	{
		DBG_IND("mmc0 bus not found\r\n");
        return -1;
	}

    //2. get the device name from mmc0 information
	snprintf(find_path, sizeof(find_path), "%s/%s/block", MMC_SYS_PATH, mmc_bus_name);
	if(!fslinux_IsPathExist(find_path))
    {
        DBG_ERR("%s not found\r\n", find_path);
        return -1;
    }

    if(0 != fslinux_find_entry_by_prefix(find_path, MMCDEV_PREFIX, mmc_dev_name, sizeof(mmc_dev_name)))
	{
		DBG_IND("device not found\r\n");
        return -1;
	}

    //3. try the real device name is mmcblk0/mmcblk1 or mmcblk0p1/mmcblk1p1
    //find dev name with p1 (e.g. mmcblk0p1)
    snprintf(dev_path, sizeof(dev_path), (beMMCFound == 0) ? "/dev/%sp1" : "/dev/%sp5", mmc_dev_name);
    if(fslinux_IsPathExist(dev_path))
        bFound = 1;

    if(!bFound)
    {//find dev name without p1. (e.g. mmcblk0)
        snprintf(dev_path, sizeof(dev_path), "/dev/%s", mmc_dev_name);
        if(fslinux_IsPathExist(dev_path))
            bFound = 1;
    }

    if(bFound)
    {
        DBG_IND("SD1Dev = %s\r\n", out_path);

		KFS_STRCPY(out_path, dev_path, out_size);
        return 0;
    }
    else
    {
        DBG_ERR("The dev partition name not found\r\n");
        return -1;
    }
}

static int fslinux_SpawnCmd(char **arg_list)
{
    int status;
    pid_t pid_tmp, pid_child;

#if TRACE_LOG
	{
		char **parg = arg_list;

	    DBG_DUMP("spawn pid %d: [ ", getpid());

		while(NULL != *parg)
			DBG_DUMP("%s ", *parg++);

		DBG_DUMP("]\n");
	}
#endif

    pid_tmp = fork();
    if(pid_tmp < 0)
    {//fork error
        DBG_ERR("fork error\n");
        return -1;
    }
    else if(0 == pid_tmp)
    {//child process
        DBG_IND("pid %d: child process\n", getpid());
        execvp(arg_list[0], arg_list);

        //execvp only return if an error has have occurred
        DBG_ERR("execvp: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        return -1;
    }

    //original process
    DBG_IND("wait child process ...\n");
    pid_child = waitpid(pid_tmp, &status, 0);

    if(pid_child < 0)
    {
        DBG_ERR("wait error\n");
        return -1;
    }

    DBG_IND("return child pid = %d\n", pid_child);
    if(WIFEXITED(status))
    {
        DBG_IND("child exited normally with status = %d\n", WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }
    else
    {
        DBG_ERR("abnormal exit, status = %d\n", status);
        return -1;
    }
}

#if 0
static int fslinux_system(const char* pCommand)
{
    pid_t pid;
    int status;
    int i = 0;

    if(pCommand == NULL)
    {
        return (1);
    }

    if((pid = fork())<0)
    {
        status = -1;
    }
    else if(pid == 0)
    {
        /* close all descriptors in child sysconf(_SC_OPEN_MAX) */
        for (i = 3; i < sysconf(_SC_OPEN_MAX); i++)
        {
            close(i);
        }

        execl("/bin/sh", "sh", "-c", pCommand, (char *)0);
        _exit(127);
    }
    else
    {
        /*当�?程中SIGCHLD?�SIG_IGN?��??��??��?不到status返�??��?XOS_System?��?�?��工�?*/
        while(waitpid(pid, &status, 0) < 0)
        {
            if(errno != EINTR)
            {
                status = -1;
                break;
            }
        }
    }

    return status;
}
#endif

static int fslinux_UmountDisk(const char *pMountPath)
{
    int ret = -1;
    int RetryTimes = 50;

    while(ret != 0 && RetryTimes-- > 0)
    {
        ret = umount2(pMountPath, MNT_EXPIRE);
        DBG_IND("umount2: ret = %d\n", ret);

        if(ret < 0)
        {
            DBG_IND("umount2: errno = %d, errmsg = %s\n", errno, strerror(errno));
            usleep(100000);
        }
    }

    return ret;
}

static unsigned int fslinux_IsIoctlValid(char *pPath, FSLINUX_IOCTL_TYPE IoctlType)
{
    unsigned int IsIoctlValid = 0;
    int ret_fd;
    unsigned int cmd;
    unsigned long arg;
    unsigned char ioctl_buf[FSLINUX_IOCTL_BUFSIZE];

    DBG_IND("open(%s, O_RDONLY)\r\n", pPath);
    ret_fd = open(pPath, O_RDONLY);
    if(-1 == ret_fd)
    {
        DBG_IND("pPath = %s, errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return IsIoctlValid;
    }

    switch(IoctlType) {
    case FSLINUX_IOCTL_MSDOS_ENTRY:
        cmd = VFAT_IOCTL_READDIR_MSDOS;
        arg = (long)ioctl_buf;
        break;
    case FSLINUX_IOCTL_EXFAT_ENTRY:
        cmd = EXFAT_IOCTL_READDIR_DIRECT;
        arg = (long)ioctl_buf;
        break;
    default:
        DBG_IND("Unsupported IoctlType(%d)\r\n", IoctlType);
        DBG_IND("close(0x%X)\r\n", ret_fd);

        if(0 != close(ret_fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));

        return IsIoctlValid;
    };

    DBG_IND("ioctl(0x%X, %d, 0x%X)\r\n", ret_fd, cmd, arg);
    if(-1 == ioctl(ret_fd, cmd, arg))
    {
        DBG_IND("pPath(%s), ioctl(%d) is invalid, errno = %d, errmsg = %s\r\n", pPath, IoctlType, errno, strerror(errno));
    }
    else
    {
        DBG_IND("pPath(%s), ioctl(%d) is valid\r\n", pPath, IoctlType);
        IsIoctlValid = 1;
    }

    DBG_IND("close(0x%X)\r\n", ret_fd);
    if(0 != close(ret_fd))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
    return IsIoctlValid;
}

static int fslinux_IoctlAttrib(char *pPath, long AttribVal, unsigned int bSet)
{
    int ret_fd;
    long IoctlAttrib = 0;

    PERF_VAL(t1);
    PERF_VAL(t2);

    DBG_IND("%s %s attrib(0x%X)\r\n", bSet?"Set":"Clear", pPath, AttribVal);

    PERF_MARK(t1);
    ret_fd = open(pPath, O_RDONLY);
    PERF_MARK(t2);
    PERF_PRINT("open %lu ms\n", PERF_DIFF(t1, t2)/1000);
    if (ret_fd == -1)
    {
        DBG_ERR("open %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return -1;
    }

    //try to use ioctl first
    if (-1 == ioctl(ret_fd, FAT_IOCTL_GET_ATTRIBUTES, &IoctlAttrib))
    {
        //DBG_ERR("get %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        if(0 != close(ret_fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return -1;
    }

    if(bSet)
        IoctlAttrib |= AttribVal;
    else
        IoctlAttrib &= (~AttribVal);

    if (-1 == ioctl(ret_fd, FAT_IOCTL_SET_ATTRIBUTES, &IoctlAttrib))
    {
        DBG_ERR("set %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        if(0 != close(ret_fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return -1;
    }

    if (!fslinux_is_skip_sync()) {
        if (-1 == fsync(ret_fd))
        {
            DBG_ERR("fsync %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
            if(0 != close(ret_fd))
                DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
            return -1;
        }
    }

    if (-1 == close(ret_fd))
    {
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return -1;
    }

    return 0;
}

static int fslinux_SyncWrite(int fd, void* pBuf, size_t wsize)
{
    int blk_idx, blk_idx_max;//the block index and the max block index
    int blk_size;
    int offset_org;//the original offset of the fd
    PERF_VAL(t1);
    PERF_VAL(t2);

    PERF_MARK(t1);
    offset_org = lseek(fd, 0, SEEK_CUR);
    PERF_MARK(t2);
    PERF_PRINT("lseek = %lu ms\n", PERF_DIFF(t1, t2)/1000);

    if((off_t)-1 == offset_org)
    {
        DBG_ERR("lseek: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        return -1;
    }

    blk_idx_max = (wsize + FSLINUX_BLK_WSIZE - 1) / FSLINUX_BLK_WSIZE;
    DBG_IND("wsize = %d, BLK_WSIZE = %d, blk_idx_max = %d\r\n", wsize, FSLINUX_BLK_WSIZE, blk_idx_max);

    //for loop to write each block
    blk_size = FSLINUX_BLK_WSIZE;

    for(blk_idx = 0; blk_idx < blk_idx_max; blk_idx++)
    {
        //write all rest data at the last round
        if(blk_idx == blk_idx_max - 1)
            blk_size = wsize - blk_idx*FSLINUX_BLK_WSIZE;

        PERF_MARK(t1);
        if(-1 == write(fd, pBuf + blk_idx*FSLINUX_BLK_WSIZE, blk_size))
        {
            DBG_ERR("write: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
            if (errno == EIO || errno == EROFS) {
                return FSLINUX_STA_SDIO_ERR;
            }
            return -1;
        }
        PERF_MARK(t2);
        PERF_PRINT("write = %lu ms\n", PERF_DIFF(t1, t2)/1000);

        if (!fslinux_is_skip_sync())
        {
            //start the current index async write
            PERF_MARK(t1);
            sync_file_range(fd, offset_org + blk_idx*FSLINUX_BLK_WSIZE, blk_size, SYNC_FILE_RANGE_WRITE);
            PERF_MARK(t2);
            PERF_PRINT("sync_file_range = %lu ms\n", PERF_DIFF(t1, t2)/1000);

            PERF_MARK(t1);
            if(blk_idx || offset_org >= FSLINUX_BLK_WSIZE)
            {//wait for the previous block done
                sync_file_range(fd, offset_org + (off64_t)(blk_idx-1)*FSLINUX_BLK_WSIZE, blk_size, SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE|SYNC_FILE_RANGE_WAIT_AFTER);
                posix_fadvise64(fd, offset_org + (off64_t)(blk_idx-1)*FSLINUX_BLK_WSIZE, blk_size, POSIX_FADV_DONTNEED);
            }
            PERF_MARK(t2);
            PERF_PRINT("wait_sync = %lu ms\n", PERF_DIFF(t1, t2)/1000);
        }
    }

    return wsize;
}

static int fslinux_AutoMount(char *pDevPath, char *pMountPath)
{
	char mountopt[30] = {0};
	char *arg_list[] = {"mount", "-o", mountopt, pDevPath, pMountPath, NULL};//e.g. mount -o time_offset=480 /dev/mmcblk0p1 /mnt/sd

    if(0 != fslinux_mkpath(pMountPath))
    {
        DBG_ERR("fslinux_mkpath(%s) failed\r\n", pMountPath);
        return -1;
    }

	snprintf(mountopt, sizeof(mountopt), "dirsync,time_offset=%d", fslinux_get_gmtdiff_minute());

    if(0 == fslinux_SpawnCmd(arg_list)) {
        return 0;
    } else {
        DBG_ERR("mount -o dirsync,time_offset=%d -t %s %s : errno = %d, errmsg = %s\r\n",
			fslinux_get_gmtdiff_minute(), pDevPath, pMountPath, errno, strerror(errno));
    }

    return -1;
}

/*
static int fslinux_AutoMount(char *pDevPath, char *pMountPath, long FsType)
{
    typedef struct {
        long FsType;
        char MountType[10];
    } MOUNT_ARG;

    #define FSTYPE_NUM  (sizeof(MountArg)/sizeof(MOUNT_ARG))

    MOUNT_ARG MountArg[] = {
    {MSDOS_SUPER_MAGIC, "vfat"},
    {EXFAT_SUPER_MAGIC, "exfat"},
    {EXT4_SUPER_MAGIC, "ext4"},
    {EXT3_SUPER_MAGIC, "ext3"},
    {EXT2_SUPER_MAGIC, "ext2"},
    {TMPFS_MAGIC, "tmpfs"},
    };

    unsigned int idx;

    //find the specific FsType
    for(idx = 0; idx < FSTYPE_NUM; idx++)
    {
        if(MountArg[idx].FsType == FsType)
            break;
    }

    //if FsType not found, try from index 0
    if(FSTYPE_NUM == idx)
        idx = 0;

    if(fslinux_mkpath(pMountPath) != 0)
    {
        DBG_ERR("fslinux_mkpath(%s) failed\r\n", pMountPath);
        return -1;
    }

    for(; idx < FSTYPE_NUM; idx++)
    {
        if(0 == mount(pDevPath, pMountPath, MountArg[idx].MountType, 0, ""))
        {
            DBG_IND("mount -t %s %s %s : OK\r\n", MountArg[idx].MountType, pDevPath, pMountPath);
            return 0;
        }
        else
            DBG_IND("mount -t %s %s %s : errno = %d, errmsg = %s\r\n", MountArg[idx].MountType, pDevPath, pMountPath, errno, strerror(errno));
    }

    return -1;
}
*/

//return the device size in bytes, return -1 for error
static int fslinux_GetBlkDevSize(char *pPath, long long *pDevSize)
{
    int fd;
    long long size = 0;

    *pDevSize = 0;//set to zero first

    fd = open(pPath, O_RDONLY);
    if(fd < 0)
    {
        DBG_ERR("open %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return -1;
    }

    if(ioctl(fd, BLKGETSIZE64, &size) < 0)
    {
        DBG_ERR("ioctl\r\n");

        if (0 != close(fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));

        return -1;
    }

    if (0 != close(fd))
    {
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return -1;
    }

    DBG_IND("%s size = %lld\r\n", pPath, size);

    *pDevSize = size;
    return 0;
}

static int fslinux_set_mbr_type(char *pPath, unsigned char type)
{
	char path_dev[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};//e.g. /dev/mmcblk0p1
	PARTITION_TABLE_T *p_partition_table = NULL;
	MBR_INFO_T mbr_buf = {0};
	char *path_mbr = NULL;
	int num_pbr = 0;
	int ret_mbr;
	int fd;

	//1. find dev name (e.g. mmcblk0)
	if (NULL == pPath) {
		DBG_ERR("Path error\r\n");
		return -1;
	}
	strncpy(path_dev, pPath, sizeof(path_dev)-1);
	path_dev[sizeof(path_dev)-1] = '\0';
	path_mbr = strtok(path_dev, "p");
	num_pbr = atoi(strtok(NULL, " "));

	//2. get mbr
	if (NULL == path_mbr) {
		DBG_ERR("MBR path error\r\n");
		return -1;
	}
	fd = open(path_mbr, O_RDONLY);
	if(fd < 0) {
		DBG_ERR("open %s error\r\n", path_mbr);
		return -1;
	}
	ret_mbr = read(fd, &mbr_buf, sizeof(MBR_INFO_T));
	if (ret_mbr < 0) {
		DBG_ERR("read %s: errno = %d, errmsg = %s\r\n", path_mbr, errno, strerror(errno));
		close(fd);
		return -1;
	}
	if (!(mbr_buf.signature.word1 == 0x55 && mbr_buf.signature.word2 == 0xaa)) {
		DBG_ERR("%s is not MBR\r\n", path_mbr);
		close(fd);
		return -1;
	}
	close(fd);

	//3. get partition table
	if (num_pbr < 1 || num_pbr > 4) {
		DBG_ERR("PBR number error\r\n");
		return -1;
	}
	p_partition_table = &mbr_buf.partition[num_pbr-1];

	DBG_IND("MBR %s PBR %d original type %02x\r\n",
		path_mbr, num_pbr, p_partition_table->system_id);

	//4. update partition tble
	p_partition_table->system_id = type;
	if (NULL == path_mbr) {
		DBG_ERR("MBR path error\r\n");
		return -1;
	}
	fd = open(path_mbr, O_WRONLY);
	if(fd < 0) {
		DBG_ERR("open %s error\r\n", path_mbr);
		return -1;
	}
	ret_mbr = write(fd, &mbr_buf, 512);
	if (ret_mbr < 0) {
		DBG_ERR("write %s: errno = %d, errmsg = %s\r\n", path_mbr, errno, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	DBG_IND("MBR %s PBR %d new type %02x\r\n",
		path_mbr, num_pbr, p_partition_table->system_id);

	return 0;
}

static int fslinux_set_mount_state(char *pPath, unsigned int state, unsigned int bExfat)
{
	char path_dev[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};//e.g. /dev/mmcblk0p1
	PBR_INFO_T pbr_buf = {0};
	char *path_pbr = NULL;
	int ret;
	int fd;
	int fs_type;
	//char fs_type_str[6] = {0};

	//0. check input
	if (0 != state) {
		DBG_DUMP("value only could be 0\r\n");
		return -1;
	}

	if (NULL == pPath) {
		DBG_ERR("Path error\r\n");
		return -1;
	}

	//1. get the device name from the mount list (e.g. mmcblk0p1)
	if(fslinux_GetDevByMountPath(path_dev, pPath, sizeof(path_dev)) == 0)
	{
		DBG_DUMP("Found %s is mounted on %s\r\n", path_dev, pPath);
	}
	else
	{
		DBG_DUMP("Can not found %s mounted\r\n", pPath);
		return -1;
	}
	path_pbr = path_dev;

	//2. get pbr
	if (NULL == path_pbr) {
		DBG_ERR("PBR path error\r\n");
		return -1;
	}
	fd = open(path_pbr, O_RDONLY);
	if(fd < 0) {
		DBG_ERR("open %s error\r\n", path_pbr);
		return -1;
	}
	ret = read(fd, &pbr_buf, sizeof(PBR_INFO_T));
	if (ret < 0) {
		DBG_ERR("read %s: errno = %d, errmsg = %s\r\n", path_pbr, errno, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	//3. get fs type
	if (!strncmp((char *)(pbr_buf.pbr_common.system_id), "EXFAT", 5)) {
		DBG_DUMP("fs type: exFAT\r\n");
		fs_type = FSLINUX_EXFAT;
	}
	else if (!strncmp((char *)(pbr_buf.fat32.fs_type), "FAT32", 5)) {
		DBG_DUMP("fs type: FAT32\r\n");
		fs_type = FSLINUX_FAT32;
	}
	else if (!strncmp((char *)(pbr_buf.fat16.fs_type), "FAT16", 5)) {
		DBG_DUMP("fs type: FAT16\r\n");
		fs_type = 0x05;
	}
	else if (!strncmp((char *)(pbr_buf.fat16.fs_type), "FAT12", 5)) {
		DBG_DUMP("fs type: FAT12\r\n");
		fs_type = 0x05;
	}
	else {
		DBG_DUMP("fs type unsupported\r\n");
		return -1;
	}

	//4. update pbr mount state
	if (fs_type == FSLINUX_FAT32) {
		DBG_IND("Set %s [%d] -> [%d]\r\n", path_pbr, state, pbr_buf.fat32.state);
		if (state)
			pbr_buf.fat32.state |= FSLINUX_STATE_DIRTY;
		else
			pbr_buf.fat32.state &= ~FSLINUX_STATE_DIRTY;
		DBG_IND("Set %s [%d] -> [%d]\r\n", path_pbr, state, pbr_buf.fat32.state);
	}
	else if (fs_type == 0x05) {
		DBG_IND("Set %s [%d] -> [%d]\r\n", path_pbr, state, pbr_buf.fat16.state);
		if (state)
			pbr_buf.fat16.state |= FSLINUX_STATE_DIRTY;
		else
			pbr_buf.fat16.state &= ~FSLINUX_STATE_DIRTY;
		DBG_IND("Set %s [%d] -> [%d]\r\n", path_pbr, state, pbr_buf.fat16.state);
	}
	else if ((fs_type == FSLINUX_EXFAT) && (bExfat)) {
		DBG_IND("Set %s [%d] -> [%d %d]\r\n", path_pbr, state, pbr_buf.exfat.vol_flags[0], pbr_buf.exfat.vol_flags[1]);
		if (state)
		{
			pbr_buf.exfat.vol_flags[0] = (FSLINUX_VOL_DIRTY & 0x00ff);
			pbr_buf.exfat.vol_flags[1] = (FSLINUX_VOL_DIRTY >> 2);
		}
		else
		{
			pbr_buf.exfat.vol_flags[0] = (FSLINUX_VOL_CLEAN & 0x00ff);
			pbr_buf.exfat.vol_flags[1] = (FSLINUX_VOL_CLEAN >> 2);
		}
		DBG_IND("Set %s [%d] -> [%d %d]\r\n", path_pbr, state, pbr_buf.exfat.vol_flags[0], pbr_buf.exfat.vol_flags[1]);
	}
	else {
		DBG_DUMP("fs type unsupported\r\n");
		return -1;
	}

	//4. write back
	if (NULL == path_pbr) {
		DBG_ERR("PBR path error\r\n");
		return -1;
	}
	fd = open(path_pbr, O_WRONLY);
	if(fd < 0) {
		DBG_ERR("open %s error\r\n", path_pbr);
		return -1;
	}
	ret = write(fd, &pbr_buf, sizeof(PBR_INFO_T));
	if (ret < 0) {
		DBG_ERR("write %s: errno = %d, errmsg = %s\r\n", path_pbr, errno, strerror(errno));
		close(fd);
		return -1;
	}
	ret = fsync(fd);
	if (ret < 0) {
		DBG_ERR("fsync %s: errno = %d, errmsg = %s\r\n", path_pbr, errno, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}

static int fslinux_check_disk(char *pMountPath, unsigned int state, unsigned int bIsSupportExfat)
{
	char *arg_list[7] = {NULL};
		//e.g. fsck.fat -a /dev/mmcblk0p1
	char **parg_cur = arg_list;
	char arg_dev[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};//e.g. /dev/mmcblk0p1
	char arg_bin_vfat[] = "fsck.fat";
	char arg_opt_sect[] = "-a";
	char arg_opt_fix_all[] = "-y";
	char arg_opt_fix_none[] = "-n";

#define FSLINUX_FSCK_FIX_ALL 2
#define FSLINUX_FSCK_FIX_NONE 1
#define FSLINUX_FSCK_DEFAULT 0

	int ret_cmd;
	long long DevSize;

	if( fslinux_GetDevByMountPath(arg_dev, pMountPath, sizeof(arg_dev)) == 0)
	{//get the device name from the mount list
		DBG_IND("Found %s is mounted on %s\r\n", arg_dev, pMountPath);

		if(fslinux_UmountDisk(pMountPath) < 0)
		{
			DBG_ERR("Umount %s failed\r\n", pMountPath);
			return -1;
		}

		DBG_IND("Umount %s to %s\r\n", pMountPath, arg_dev);
	}
	else if (fslinux_GetSD1DevByBus(arg_dev, sizeof(arg_dev)) != 0)
	{//if the device is not mounted, get the dev name from the mmc0 bus
		DBG_ERR("Find the dev name from the mmc0 bus failed\r\n");
		return -1;
	}

	if (fslinux_GetBlkDevSize(arg_dev, &DevSize) < 0)
	{
		DBG_ERR("fslinux_GetBlkDevSize %s failed\r\n", arg_dev);
		return -1;
	}

	DBG_IND("dev[%s], DevSize %lld, bIsSupportExfat %d\r\n", arg_dev, DevSize, bIsSupportExfat);

	//set bin file name
	if (fslinux_is_bus0_emmc())
	{//eMMC storage
		DBG_DUMP("Not support eMMC storage\r\n");
		return -1;
	}
	else
	{//original SD card
		if (bIsSupportExfat && DevSize > 32*1024*1024*1024LL)
		{//exFAT
			DBG_DUMP("Not support exFAT storage\r\n");
			return -1;
		}
		else
		{//VFAT
			*parg_cur++ = arg_bin_vfat;
		}
	}

	if (state == FSLINUX_FSCK_FIX_ALL) {
		*parg_cur++ = arg_opt_fix_all;
	} else if (state == FSLINUX_FSCK_FIX_NONE) {
		*parg_cur++ = arg_opt_fix_none;
	} else {
		*parg_cur++ = arg_opt_sect;
	}

	//set device name
	*parg_cur++ = arg_dev;

	ret_cmd = fslinux_SpawnCmd(arg_list);
	DBG_IND("ret = %d\r\n", ret_cmd);

	if (ret_cmd != 0)
		DBG_ERR("fslinux_SpawnCmd failed\r\n");

	//mount back
	if (fslinux_AutoMount(arg_dev, pMountPath) < 0)
	{
		DBG_ERR("mount %s %s failed\r\n", arg_dev, pMountPath);
		return -1;
	}

	DBG_IND("Mount %s on %s\r\n", arg_dev, pMountPath);

	return 0;
}

static int fslinux_CopyFile(char *pSrcPath, char *pDstPath, int bDeleteSrc)
{
    int fd_in, fd_out;
    struct stat SrcStat;
    int ret_fstat;
    ssize_t ret_written;
    void *map_in;

    DBG_IND("pSrcPath = %s, pDstPath = %s, bDeleteSrc = %d\r\n", pSrcPath, pDstPath, bDeleteSrc);
    fd_in = open(pSrcPath, O_RDONLY);
    if(-1 == fd_in)
    {
        DBG_ERR("open %s, errno = %d, errmsg = %s\r\n", pSrcPath, errno, strerror(errno));
        return -1;
    }

    ret_fstat = fstat(fd_in, &SrcStat);
    if(ret_fstat != 0)
    {
        DBG_ERR("ret_fstat = %d, errno = %d, errmsg = %s\r\n", ret_fstat, errno, strerror(errno));

        if(0 != close(fd_in))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pSrcPath, errno, strerror(errno));

        return -1;
    }

    if(fslinux_GetPathDepth(pDstPath) > 1)
    {
        char *pLastSlash = strrchr(pDstPath, FSLINUX_PATH_SEPAR);
        *pLastSlash = '\0';
		if(0 != fslinux_mkpath(pDstPath))
			DBG_ERR("fslinux_mkpath(%s) failed\r\n", pDstPath);
        *pLastSlash = FSLINUX_PATH_SEPAR;
    }

    fd_out = open(pDstPath, O_RDWR|O_CREAT|O_TRUNC, SrcStat.st_mode);
    if(-1 == fd_out)
    {
        DBG_ERR("open %s: errno = %d, errmsg = %s\r\n", pDstPath, errno, strerror(errno));

        if(0 != close(fd_in))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pSrcPath, errno, strerror(errno));

        return -1;
    }

    map_in = mmap(0, SrcStat.st_size, PROT_READ, MAP_SHARED, fd_in, 0);
    if(MAP_FAILED == map_in)
        DBG_ERR("mmap failed\r\n");

    ret_written = fslinux_SyncWrite(fd_out, map_in, SrcStat.st_size);

    if(0 != munmap(map_in, SrcStat.st_size))
        DBG_ERR("munmap failed\r\n");
    /*
    //Note: Use sendfile will create an empty file. The root cause is unknown.
    DBG_IND("sendfile: %s => %s, %lld bytes\r\n", pSrcPath, pDstPath, SrcStat.st_size);
    ret_sendfile = sendfile(fd_out, fd_in, NULL, SrcStat.st_size);
    DBG_IND("ret_sendfile = %lld\r\n", (unsigned long long)ret_sendfile);
    */

    if(ret_written != SrcStat.st_size)
    {
        DBG_ERR("fslinux_SyncWrite failed\r\n");

        if(0 != close(fd_in))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pSrcPath, errno, strerror(errno));

        if(0 != close(fd_out))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pDstPath, errno, strerror(errno));

        return -1;
    }

    if(-1 == fsync(fd_out))
         DBG_ERR("fsync %s: errno = %d, errmsg = %s\r\n", pDstPath, errno, strerror(errno));

    posix_fadvise64(fd_out, 0, 0, POSIX_FADV_DONTNEED);

    if(0 != close(fd_out))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pDstPath, errno, strerror(errno));

    if(0 != close(fd_in))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pSrcPath, errno, strerror(errno));

    if(bDeleteSrc)
    {
        int ret_unlink;

        DBG_IND("unlink(%s)\r\n", pSrcPath);
        ret_unlink = unlink(pSrcPath);
        DBG_IND("ret_unlink = %d\r\n", ret_unlink);

        if(-1 == ret_unlink)
        {
            DBG_ERR("unlink: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

static void FsLinux_CmdId_OpenFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_OPENFILE *pArg = (FSLINUX_ARG_OPENFILE *)ArgAddr;

    char *pPath = pArg->szPath;
    int uITRONflag = pArg->flag & 0xFF;

    int ret_open;
    mode_t open_flag;

    int bIsWrite = 0;
    int bSyncParent = 0;
    int bIsHidden = 0;
    PERF_VAL(t1);
    PERF_VAL(t2);

    PERF_MARK(t1);

	pArg->Linux_fd = -1; //set the return fd as an error value

    //get the file mode for Linux
    if ((uITRONflag & FSLINUX_CREATE_ALWAYS) && (uITRONflag & FSLINUX_OPEN_WRITE))
    {
        open_flag = O_RDWR|O_CREAT|O_TRUNC;
        bIsWrite = 1;
		bSyncParent = 1;
    }
    else if ((uITRONflag & FSLINUX_OPEN_ALWAYS) && (uITRONflag & FSLINUX_OPEN_WRITE))
    {
        //this mode will not create automatically, recover this later
        open_flag = O_RDWR|O_CREAT;
        bIsWrite = 1;
		bSyncParent = 1;
    }
    else if ((uITRONflag & FSLINUX_OPEN_EXISTING) && (uITRONflag & FSLINUX_OPEN_WRITE))
    {
        open_flag = O_RDWR;
        bIsWrite = 1;
    }
    else if ((uITRONflag & FSLINUX_CREATE_HIDDEN) && (uITRONflag & FSLINUX_OPEN_WRITE))
    {
        open_flag = O_RDWR|O_CREAT|O_TRUNC;
        bIsWrite = 1;
        bSyncParent = 1;
        bIsHidden = 1;
    }
    else if ((uITRONflag == FSLINUX_OPEN_READ) || (uITRONflag == (FSLINUX_OPEN_READ|FSLINUX_OPEN_EXISTING)))
    {
        open_flag = O_RDONLY;
    }
    else
    {
        DBG_ERR("Invalid uITRON flag(0x%X)\r\n", pArg->flag);
        return;
    }

    if (uITRONflag & FSLINUX_CLOSE_ON_EXEC)
    {
        open_flag |= O_CLOEXEC;
    }

    //create the parent folders
    if(bIsWrite)
    {
        if(fslinux_GetPathDepth(pPath) > 1)
        {
            char *pLastSlash = strrchr(pPath, FSLINUX_PATH_SEPAR);
            *pLastSlash = '\0';
            if(0 != fslinux_mkpath(pPath))
	        	DBG_ERR("fslinux_mkpath(%s) failed\r\n", pPath);
            *pLastSlash = FSLINUX_PATH_SEPAR;
        }
    }

    //create the target file
    DBG_IND("open(%s, %d)\r\n", pPath, open_flag);
    ret_open = open(pPath, open_flag, 0666);
    DBG_IND("ret_open = 0x%X\r\n", ret_open);

    //set the file path with hidden attrib
    if(bIsHidden)
    {
        //try to use ioctl
        if(0 != fslinux_IoctlAttrib(pPath, FSLINUX_ATTRIB_HIDDEN, bIsHidden))
        {
            DBG_IND("open %s with hidden attrib not supported\r\n", pPath);
        }
    }

    if(ret_open != -1) {
		if (bSyncParent) {
			char szParent[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};

			if(-1 == fsync(ret_open))
	            DBG_ERR("fsync: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

			if (0 != fslinux_get_parent_dir(pPath, szParent))
				DBG_ERR("get %s parent dir failed\r\n", pPath);

			if (0 != fslinux_sync_path(szParent))
				DBG_ERR("sync %s failed\r\n", szParent);
		}
        pArg->Linux_fd = (unsigned int)ret_open;
    } else {
        DBG_IND("errno = %d, errmsg = %s\r\n", errno, strerror(errno));
    }

    PERF_MARK(t2);
    PERF_PRINT("open = %lu ms\n", PERF_DIFF(t1, t2)/1000);
}
static void FsLinux_CmdId_CloseFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_CLOSEFILE *pArg = (FSLINUX_ARG_CLOSEFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
    int ret_fcntl;
    PERF_VAL(t1);
    PERF_VAL(t2);

    PERF_MARK(t1);
    ret_fcntl = fcntl(Linux_fd, F_GETFL, 0);
    if(-1 == ret_fcntl)
    {
        DBG_ERR("fcntl: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }
    PERF_MARK(t2);
    PERF_PRINT("fcntl %lu ms\n", PERF_DIFF(t1, t2)/1000);
    DBG_IND("ret_fcntl = 0x%X\r\n", ret_fcntl);

    PERF_MARK(t1);
    if (!fslinux_is_skip_sync()) {
        if(ret_fcntl & O_WRONLY || ret_fcntl & O_RDWR)
        {//only call fsync for writing
            DBG_IND("fsync(0x%X)\r\n", Linux_fd);
            if(-1 == fsync(Linux_fd))
            {
                DBG_ERR("fsync: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
                pArg->ret_fslnx = FSLINUX_STA_ERROR;
                if (errno == EIO || errno == EROFS) {
                    pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
                }
                return;
            }
            posix_fadvise64(Linux_fd, 0, 0, POSIX_FADV_DONTNEED);
        }
    }
    PERF_MARK(t2);
    PERF_PRINT("fsync %lu ms\n", PERF_DIFF(t1, t2)/1000);

    DBG_IND("close(0x%X)\r\n", (unsigned int)Linux_fd);
    PERF_MARK(t1);
    if(-1 == close(Linux_fd))
    {
        DBG_ERR("close: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }
    PERF_MARK(t2);
    PERF_PRINT("close %lu ms\n", PERF_DIFF(t1, t2)/1000);

    pArg->ret_fslnx = FSLINUX_STA_OK;

}

static void FsLinux_CmdId_ReadFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_READFILE *pArg = (FSLINUX_ARG_READFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
    void *pBuf = (void *)FsLinux_GetNonCacheAddr((unsigned int)pArg->phy_pBuf);
    size_t rwsize = pArg->rwsize;

    ssize_t ret_read;
    int offset_org;//the original offset of the fd

    offset_org = lseek(Linux_fd, 0, SEEK_CUR);
    if((off_t)-1 == offset_org)
    {
        DBG_ERR("lseek: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->rwsize = 0;
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    posix_fadvise64(Linux_fd, offset_org, rwsize, POSIX_FADV_SEQUENTIAL);

    DBG_IND("read(%d, 0x%X, %d)\r\n", Linux_fd, (unsigned int)pBuf, rwsize);
    ret_read = read(Linux_fd, pBuf, rwsize);
    DBG_IND("ret_read = %d\r\n", ret_read);

    posix_fadvise64(Linux_fd, offset_org, rwsize, POSIX_FADV_DONTNEED);

    if(ret_read != -1)
    {
        pArg->rwsize = ret_read;
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("read: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->rwsize = 0;
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        if (errno == EIO) {
            pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
        }
    }
}

static void FsLinux_CmdId_WriteFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_WRITEFILE *pArg = (FSLINUX_ARG_WRITEFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
	void *pBuf;
    size_t rwsize = pArg->rwsize;
    int ret_written;

#if FSLINUX_USE_IPC
#if !defined(_BSP_NA50902_)
	void *map_addr;
	unsigned int map_size;

	map_size = rwsize;
	map_addr = NvtIPC_mmap(NVTIPC_MEM_TYPE_CACHE, (unsigned int)pArg->phy_pBuf, map_size);
	if (NULL == map_addr) {
        DBG_ERR("NvtIPC_mmap failed\r\n");
		goto L_FsLinux_CmdId_WriteFile_END;
    }
#endif
#endif

	//set to default error
	pArg->ret_fslnx = FSLINUX_STA_ERROR;
	pArg->rwsize = 0;

	pBuf = (void *)FsLinux_GetCacheAddr((unsigned int)pArg->phy_pBuf);
	if(FSLINUX_IPC_INVALID_ADDR == (unsigned int)pBuf)
	{
		DBG_ERR("Convert addr failed\r\n");
		goto L_FsLinux_CmdId_WriteFile_END;
	}

#if FSLINUX_USE_IPC
#if defined(_BSP_NA50902_)
    if(0 != cacheflush(pBuf, rwsize, DCACHE))
#else
    if(0 != NvtIPC_FlushCache((unsigned int)pBuf, rwsize))
#endif
    {
        DBG_ERR("cacheflush failed\r\n");
        goto L_FsLinux_CmdId_WriteFile_END;;
    }
#endif

    ret_written = fslinux_SyncWrite(Linux_fd, pBuf, rwsize);
    if((size_t)ret_written != rwsize)
    {
        DBG_ERR("fslinux_SyncWrite failed\r\n");
        pArg->ret_fslnx = ret_written;
        goto L_FsLinux_CmdId_WriteFile_END;
    }

	//success here
	pArg->ret_fslnx = FSLINUX_STA_OK;
	pArg->rwsize = rwsize;//update the correct written size

L_FsLinux_CmdId_WriteFile_END:
#if FSLINUX_USE_IPC
#if !defined(_BSP_NA50902_)
	if (map_addr) {
		if (0 != NvtIPC_munmap(map_addr, map_size))
			DBG_ERR("munmap failed\r\n");
	}
#endif
#endif
	return;
}

static void FsLinux_CmdId_SeekFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_SEEKFILE *pArg = (FSLINUX_ARG_SEEKFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
    off_t offset = pArg->offset;
    int uITRON_whence = pArg->whence;

    int lseek_whence;
    off_t ret_lseek;

    switch(uITRON_whence){
    case FSLINUX_SEEK_SET:
        lseek_whence = SEEK_SET;
        break;
    case FSLINUX_SEEK_CUR:
        lseek_whence = SEEK_CUR;
        break;
    case FSLINUX_SEEK_END:
        lseek_whence = SEEK_END;
        break;
    default:
        DBG_ERR("unknown uITRON whence(%d)\r\n", uITRON_whence);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    DBG_IND("lseek(0x%X, %lld, %d)\r\n", Linux_fd, offset, lseek_whence);
    ret_lseek = lseek(Linux_fd, offset, lseek_whence);
    DBG_IND("ret_lseek = %lld\r\n", ret_lseek);

    if((off_t)-1 == ret_lseek)
    {
        DBG_ERR("ret_lseek: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    pArg->ret_fslnx = FSLINUX_STA_OK;
}

static void FsLinux_CmdId_TruncFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_TRUNCFILE *pArg = (FSLINUX_ARG_TRUNCFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
    unsigned long long NewSize = pArg->NewSize;

    int ret_ftruncate;

    DBG_IND("ftruncate(0x%X, %lld)\r\n", Linux_fd, NewSize);
    ret_ftruncate = ftruncate(Linux_fd, NewSize);
    DBG_IND("ret_ftruncate = %d\r\n", ret_ftruncate);

    if(0 == ret_ftruncate)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("ftruncate: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        if (errno == EIO || errno == EROFS) {
            pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
        }
    }
}

static void FsLinux_CmdId_AllocFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_ALLOCFILE *pArg = (FSLINUX_ARG_ALLOCFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
    int mode = pArg->mode;
    unsigned long long len = pArg->len;

    int ret_fallocate;

    DBG_IND("fallocate(0x%X, 0, 0, %lld)\r\n", Linux_fd, len);
    ret_fallocate = fallocate(Linux_fd, mode, 0, len); //FALLOC_FL_KEEP_SIZE
    DBG_IND("ret_fallocate = %d\r\n", ret_fallocate);

    if(0 == ret_fallocate)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("fallocate: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        if (errno == EIO || errno == EROFS) {
            pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
        }
    }
}


static void FsLinux_CmdId_TellFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_TELLFILE *pArg = (FSLINUX_ARG_TELLFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;

    off_t ret_lseek;

    DBG_IND("lseek(0x%X, %lld, %d)\r\n", Linux_fd, 0, SEEK_CUR);
    ret_lseek = lseek(Linux_fd, 0, SEEK_CUR);
    DBG_IND("ret_lseek = %lld\r\n", ret_lseek);

    if((off_t)-1 == ret_lseek)
    {
        DBG_ERR("lseek: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = 0;
        return;
    }

    pArg->ret_fslnx = ret_lseek;
}

static void FsLinux_CmdId_DeleteFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_DELETEFILE *pArg = (FSLINUX_ARG_DELETEFILE *)ArgAddr;

    char *pPath = pArg->szPath;;

    PERF_VAL(t1);
    PERF_VAL(t2);

    int ret_unlink;

    DBG_IND("unlink(%s)\r\n", pPath);
    PERF_MARK(t1);
    ret_unlink = unlink(pPath);
    PERF_MARK(t2);
    PERF_PRINT("unlink %lu ms\n", PERF_DIFF(t1, t2)/1000);
    DBG_IND("ret_unlink = %d\r\n", ret_unlink);

    if(0 == ret_unlink)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("%s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == ENOENT) {
			pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
		}
		if (errno == EIO || errno == EROFS) {
			pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
		}
    }
}

static void FsLinux_CmdId_StatFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_STATFILE *pArg = (FSLINUX_ARG_STATFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;
    FSLINUX_FILE_STATUS *pStat = &(pArg->OutStat);

    struct stat TmpStat = {0};
    int ret_fstat;

    DBG_IND("fstat(%d, &TmpStat(0x%X))\r\n", Linux_fd, (unsigned int)pStat);
    ret_fstat = fstat(Linux_fd, &TmpStat);
    DBG_IND("ret_fstat = %d\r\n", ret_fstat);

    if(0 == ret_fstat)
    {
        pStat->uiFileSize = TmpStat.st_size;
        fslinux_GetFatDateTime(&TmpStat.st_atime, &pStat->uiAccessDate, NULL, NULL);
        fslinux_GetFatDateTime(&TmpStat.st_mtime, &pStat->uiModifiedDate, &pStat->uiModifiedTime, NULL);
        fslinux_GetFatDateTime(&TmpStat.st_ctime, &pStat->uiCreatedDate, &pStat->uiCreatedTime, &pStat->uiCreatedTime10ms);
        pStat->uiAttrib = fslinux_GetFatAttrib(TmpStat.st_mode);

        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("ret_fstat = %d, errno = %d, errmsg = %s\r\n", ret_fstat, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == EIO) {
			pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
		}
    }
}

static void FsLinux_CmdId_GetFileLen(unsigned int ArgAddr)
{
    FSLINUX_ARG_GETFILELEN *pArg = (FSLINUX_ARG_GETFILELEN *)ArgAddr;

    char *pPath = pArg->szPath;;

    struct stat TmpStat = {0};
    int ret_stat;

    DBG_IND("stat(%s, &TmpStat)\r\n", pPath);
    ret_stat = stat(pPath, &TmpStat);
    DBG_IND("ret_stat = %d\r\n", ret_stat);

    if(0 == ret_stat)
    {
        pArg->ret_fslnx = TmpStat.st_size;
    }
    else
    {
        DBG_ERR("ret_stat = %d, errno = %d, errmsg = %s\r\n", ret_stat, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == ENOENT) {
			pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
		}
    }
}

static void FsLinux_CmdId_GetDateTime(unsigned int ArgAddr)
{
    FSLINUX_ARG_GETDATETIME *pArg = (FSLINUX_ARG_GETDATETIME *)ArgAddr;

    char *pPath = pArg->szPath;;
    unsigned int *creDateTime = pArg->DT_Created;
    unsigned int *modDateTime = pArg->DT_Modified;

    struct stat TmpStat = {0};
    int ret_stat;

    DBG_IND("stat(%s, &TmpStat)\r\n", pPath);
    ret_stat = stat(pPath, &TmpStat);
    DBG_IND("ret_stat = %d\r\n", ret_stat);

    if(0 == ret_stat)
    {
        fslinux_GetYMDHMS(&TmpStat.st_ctime, creDateTime);
        fslinux_GetYMDHMS(&TmpStat.st_mtime, modDateTime);
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("ret_stat = %d, errno = %d, errmsg = %s\r\n", ret_stat, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == ENOENT) {
			pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
		}
    }
}

static void FsLinux_CmdId_SetDateTime(unsigned int ArgAddr)
{
    FSLINUX_ARG_SETDATETIME *pArg = (FSLINUX_ARG_SETDATETIME *)ArgAddr;

    char *pPath = pArg->szPath;
    //unsigned int *dummyTime = pArg->DT_Created;//not support create time now
    unsigned int *modDateTime = pArg->DT_Modified;

	int Linux_fd;
    struct stat TmpStat = {0};
    int ret_tmp;
	struct tm new_tm = {0};
	struct timespec new_timespec[2] = {0};

	Linux_fd = open(pPath, O_RDWR);
    if (Linux_fd == -1)
    {
        DBG_ERR("open %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    //get the original attrib
    DBG_IND("fstat(%d, &TmpStat(0x%X))\r\n", Linux_fd, (unsigned int)&TmpStat);
    ret_tmp = fstat(Linux_fd, &TmpStat);
    DBG_IND("ret_tmp = %d\r\n", ret_tmp);

    if(0 != ret_tmp)
    {
        DBG_ERR("fstat: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

	    if(0 != close(Linux_fd))
    	    DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));

        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		return;
    }

	new_tm.tm_year = modDateTime[0] - 1900;
    new_tm.tm_mon = modDateTime[1] - 1;
    new_tm.tm_mday = modDateTime[2];
    new_tm.tm_hour = modDateTime[3];
    new_tm.tm_min = modDateTime[4];
    new_tm.tm_sec = modDateTime[5];

	new_timespec[0].tv_sec = TmpStat.st_atime; 	//"last access time"
	new_timespec[1].tv_sec = timegm(&new_tm); 	//"last modification time"

	DBG_IND("futimens(%d)\r\n", Linux_fd);
    ret_tmp = futimens(Linux_fd, new_timespec);
    DBG_IND("ret_tmp = %d\r\n", ret_tmp);

	if(0 != ret_tmp)
    {
        DBG_ERR("futimens: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

	    if(0 != close(Linux_fd))
    	    DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));

        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		return;
    }

    if(0 != close(Linux_fd))
    {
	    DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
		pArg->ret_fslnx = FSLINUX_STA_ERROR;
		return;
    }

	pArg->ret_fslnx = FSLINUX_STA_OK;
}


static void FsLinux_CmdId_FlushFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_FLUSHFILE *pArg = (FSLINUX_ARG_FLUSHFILE *)ArgAddr;

    int Linux_fd = pArg->Linux_fd;

    int ret_fsync = 0;

    PERF_VAL(t1);
    PERF_VAL(t2);

    DBG_IND("fsync(%d)\r\n", Linux_fd);
    PERF_MARK(t1);
    if (!fslinux_is_skip_sync()) {
        ret_fsync = fsync(Linux_fd);
    }
    PERF_MARK(t2);
    PERF_PRINT("fsync %lu ms\n", PERF_DIFF(t1, t2)/1000);
    DBG_IND("ret_fsync = %d\r\n", ret_fsync);

    if(0 == ret_fsync)
    {
        PERF_MARK(t1);
        if (!fslinux_is_skip_sync()) {
            posix_fadvise64(Linux_fd, 0, 0, POSIX_FADV_DONTNEED);
        }
        pArg->ret_fslnx = FSLINUX_STA_OK;
        PERF_MARK(t2);
        PERF_PRINT("fadvise %lu ms\n", PERF_DIFF(t1, t2)/1000);
    }
    else
    {
        DBG_ERR("fsync: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == ENOENT) {
			pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
		}
		if (errno == EIO || errno == EROFS) {
			pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
		}
    }
}

static void FsLinux_CmdId_MakeDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_MAKEDIR *pArg = (FSLINUX_ARG_MAKEDIR *)ArgAddr;

    char *pPath = pArg->szPath;;

    int ret;

    DBG_IND("fslinux_mkpath(%s)\r\n", pPath);
    ret = fslinux_mkpath(pPath);
    DBG_IND("ret = %d\r\n", ret);

    if(0 == ret)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("fslinux_mkpath %s failed\r\n", pPath);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
    }
}

static void FsLinux_CmdId_DeleteDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_DELETEDIR *pArg = (FSLINUX_ARG_DELETEDIR *)ArgAddr;

    char *pPath = pArg->szPath;;

    int ret;

    DBG_IND("fslinux_rmdir_recursive(%s)\r\n", pPath);
    ret = fslinux_rmdir_recursive(pPath, 1);
    DBG_IND("ret = %d\r\n", ret);

    if(0 == ret)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("fslinux_rmdir_recursive %s failed\r\n", pPath);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
    }
}

static void FsLinux_CmdId_RenameDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_RENAMEDIR *pArg = (FSLINUX_ARG_RENAMEDIR *)ArgAddr;

    char *pNewName = pArg->szNewName;
    char *pPath = pArg->szPath;;
    unsigned int bIsOverwrite = pArg->bIsOverwrite;

    char NewPath[FSLINUX_MAXPATH] = {0};
    int ret_rename;

    //strcpy(NewPath, pPath);
    KFS_STRCPY(NewPath, pPath, sizeof(NewPath));
    dirname(NewPath);

    if(NewPath[strlen(NewPath)-1] != FSLINUX_PATH_SEPAR)
        strcat(NewPath, "/");

    strcat(NewPath, pNewName);
    DBG_IND("NewPath = %s\r\n", NewPath);

    if(0 == bIsOverwrite)
    {
        DIR *pDir = opendir(NewPath);
        if(pDir)
        {
            if(0 != closedir(pDir))
                DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", NewPath, errno, strerror(errno));

            DBG_IND("%s exists, skip since bIsOverwrite = FALSE\r\n", NewPath);
            pArg->ret_fslnx = FSLINUX_STA_ERROR;
            return;
        }
    }

    DBG_IND("rename(%s => %s)\r\n", pPath, NewPath);
    ret_rename = rename(pPath, NewPath);
    DBG_IND("ret_rename = %d\r\n", ret_rename);

    if(0 == ret_rename)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("rename %s to %s, errno = %d, errmsg = %s\r\n", pPath, NewPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
    }
}

static void FsLinux_CmdId_RenameFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_RENAMEFILE *pArg = (FSLINUX_ARG_RENAMEFILE *)ArgAddr;

    char *pNewName = pArg->szNewName;
    char *pPath = pArg->szPath;;
    unsigned int bIsOverwrite = pArg->bIsOverwrite;

    char NewPath[FSLINUX_MAXPATH] = {0};
    int ret_rename;

    //strcpy(NewPath, pPath);
    KFS_STRCPY(NewPath, pPath, sizeof(NewPath));
    dirname(NewPath);

    if(NewPath[strlen(NewPath)-1] != FSLINUX_PATH_SEPAR)
        strcat(NewPath, "/");

    strcat(NewPath, pNewName);
    DBG_IND("NewPath = %s\r\n", NewPath);

    if(0 == bIsOverwrite)
    {
        int Linux_fd = open(NewPath, O_RDONLY);

        if(Linux_fd != -1)
        {
            if(0 != close(Linux_fd))
                DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", NewPath, errno, strerror(errno));

            DBG_IND("The NewPath exists, skip since bIsOverwrite = FALSE\r\n");
            pArg->ret_fslnx = FSLINUX_STA_ERROR;
            return;
        }
    }

    DBG_IND("rename(%s => %s)\r\n", pPath, NewPath);
    ret_rename = rename(pPath, NewPath);
    DBG_IND("ret_rename = %d\r\n", ret_rename);

    if(0 == ret_rename)
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("rename %s to %s, errno = %d, errmsg = %s\r\n", pPath, NewPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == ENOENT) {
			pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
		}
    }
}

static void FsLinux_CmdId_MoveFile(unsigned int ArgAddr)
{
    FSLINUX_ARG_MOVEFILE *pArg = (FSLINUX_ARG_MOVEFILE *)ArgAddr;

    char *pSrcPath = pArg->szSrcPath;
    char *pDstPath = pArg->szDstPath;;

    int ret_rename;

    if(fslinux_GetPathDepth(pDstPath) > 1)
    {
        char *pLastSlash = strrchr(pDstPath, FSLINUX_PATH_SEPAR);
        *pLastSlash = '\0';
		if(0 != fslinux_mkpath(pDstPath))
	        DBG_ERR("fslinux_mkpath(%s) failed\r\n", pDstPath);
        *pLastSlash = FSLINUX_PATH_SEPAR;
    }

    DBG_IND("rename(%s => %s)\r\n", pSrcPath, pDstPath);
    ret_rename = rename(pSrcPath, pDstPath);
    DBG_IND("ret_rename = %d\r\n", ret_rename);

    if(0 != ret_rename)
    {
        DBG_IND("rename %s to %s failed, errno = %d, errmsg = %s\r\n", pSrcPath, pDstPath, errno, strerror(errno));
        //try to use copy
        ret_rename = fslinux_CopyFile(pSrcPath, pDstPath, 1);
    }

    if(0 == ret_rename)
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_GetAttrib(unsigned int ArgAddr)
{
    FSLINUX_ARG_GETATTRIB *pArg = (FSLINUX_ARG_GETATTRIB *)ArgAddr;

    char *pPath = pArg->szPath;;

    int Linux_fd;
    long TmpAttrib = 0;

    Linux_fd = open(pPath, O_RDONLY);
    if (Linux_fd == -1)
    {
        DBG_ERR("open %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == ENOENT) {
			pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
		}
        return;
    }


    //get the original attrib
    if (0 == ioctl(Linux_fd, FAT_IOCTL_GET_ATTRIBUTES, &TmpAttrib))
    {
        pArg->Attrib = (unsigned char)TmpAttrib;
        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        struct stat TmpStat = {0};
        int ret_fstat;

        DBG_IND("fstat(%d, &TmpStat(0x%X))\r\n", Linux_fd, (unsigned int)&TmpStat);
        ret_fstat = fstat(Linux_fd, &TmpStat);
        DBG_IND("ret_fstat = %d\r\n", ret_fstat);

        if(0 == ret_fstat)
        {
            pArg->Attrib = fslinux_GetFatAttrib(TmpStat.st_mode);
            pArg->ret_fslnx = FSLINUX_STA_OK;
        }
        else
        {
            DBG_ERR("ret_fstat = %d, errno = %d, errmsg = %s\r\n", ret_fstat, errno, strerror(errno));
            pArg->ret_fslnx = FSLINUX_STA_ERROR;
			if (errno == ENOENT) {
				pArg->ret_fslnx = FSLINUX_STA_FILE_NOTEXIST;
			}
        }
    }
    if(0 != close(Linux_fd))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
}

static void FsLinux_CmdId_SetAttrib(unsigned int ArgAddr)
{
    FSLINUX_ARG_SETATTRIB *pArg = (FSLINUX_ARG_SETATTRIB *)ArgAddr;

    char *pPath = pArg->szPath;;
    long AttribVal = (long)pArg->Attrib;
    int bSet = pArg->bSet;

    struct stat TmpStat = {0};
    mode_t newmode;
    int Linux_fd;

    //try to use ioctl first
    if(0 == fslinux_IoctlAttrib(pPath, AttribVal, bSet))
    {
        pArg->ret_fslnx = FSLINUX_STA_OK;
        return;
    }

    //if ioctl not done, use original Linux API
    Linux_fd = open(pPath, O_RDONLY);
    if (Linux_fd == -1)
    {
        DBG_ERR("open %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    if(AttribVal != FSLINUX_ATTRIB_READONLY)
    {
        DBG_ERR("%s %s attrib(0x%X) failed, only support RO\r\n", bSet?"Set":"Clear", pPath, AttribVal);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;

        if(0 != close(Linux_fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return;
    }

    DBG_IND("fstat(%d, &TmpStat(0x%X))\r\n", Linux_fd, (unsigned int)&TmpStat);
    if(0 != fstat(Linux_fd, &TmpStat))
    {
        DBG_ERR("fstat %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;

        if(0 != close(Linux_fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return;
    }

    if(bSet)
        newmode = TmpStat.st_mode & (~(S_IWUSR|S_IWGRP|S_IWOTH));//set readonly => remove all write permissions
    else
        newmode = TmpStat.st_mode | (S_IWUSR|S_IWGRP|S_IWOTH);//remove readonly => set all write permissions

    DBG_IND("org mode = 0x%X, new mode = 0x%X\r\n", TmpStat.st_mode, newmode);

    DBG_IND("fchmod(%d, 0x%X)\r\n", Linux_fd, newmode);
    if(0 != fchmod(Linux_fd, newmode))
    {
        DBG_ERR("fchmod %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        if(0 != close(Linux_fd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
        return;
    }

    pArg->ret_fslnx = FSLINUX_STA_OK;
    if(0 != close(Linux_fd))
        DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
}

static void FsLinux_CmdId_OpenDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_OPENDIR *pArg = (FSLINUX_ARG_OPENDIR *)ArgAddr;

    char *pPath = pArg->szPath;;
    FSLINUX_FIND_DATA_ITEM *pItemBuf = (FSLINUX_FIND_DATA_ITEM *)FsLinux_GetNonCacheAddr((unsigned int)pArg->phy_pItemBuf);
    int direction = pArg->direction;
    unsigned int TmpItemNum = 1;//fix the number to one, since we should return the opendir handle, not the list result
    unsigned int bUseIoctl = FSLINUX_IOCTL_INVALID;

    //DIR *ret_opendir;
    unsigned int dirfd;
    int ret_tmp;

    //we only handle directions FSLINUX_HANDLEONLY_SEARCH and FSLINUX_FORWARD_SEARCH
    if(direction != FSLINUX_FORWARD_SEARCH && direction != FSLINUX_HANDLEONLY_SEARCH)
    {
        DBG_ERR("unknown direction 0x%X\r\n", direction);
        pArg->Linux_dirfd = 0;
        return;
    }

    if (fslinux_IsIoctlValid(pPath, FSLINUX_IOCTL_MSDOS_ENTRY))
        bUseIoctl = FSLINUX_IOCTL_MSDOS_ENTRY;
    else if (fslinux_IsIoctlValid(pPath, FSLINUX_IOCTL_EXFAT_ENTRY))
        bUseIoctl = FSLINUX_IOCTL_EXFAT_ENTRY;

    DBG_IND("path(%s), bUseIoctl(%d)\r\n", pPath, bUseIoctl);

    if(bUseIoctl > FSLINUX_IOCTL_INVALID)
    {
        int Linux_fd = open(pPath, O_RDONLY);
        if(-1 == Linux_fd)
        {
            DBG_IND("open: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
            pArg->Linux_dirfd = 0;
            return;
        }
        dirfd = (unsigned int)Linux_fd;
    }
    else
    {
        DIR *ret_opendir = opendir(pPath);
        if(NULL == ret_opendir)
        {
            DBG_IND("ret_opendir: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
            pArg->Linux_dirfd = 0;
            return;
        }
        dirfd = (unsigned int)ret_opendir;
    }
    DBG_IND("dirfd = 0x%X\r\n", dirfd);

    //if handle only, return right away
    if(FSLINUX_HANDLEONLY_SEARCH == direction)
    {
        pArg->Linux_dirfd = dirfd;
        pArg->bUseIoctl = bUseIoctl;
        return;
    }

    //readdir N items
    ret_tmp = fslinux_StatNextEntry(dirfd, pItemBuf, &TmpItemNum, bUseIoctl);
    if(FSLINUX_STA_OK == ret_tmp)
    {
        pArg->Linux_dirfd = dirfd;
        pArg->bUseIoctl = bUseIoctl;
        return;
    }

    DBG_ERR("fslinux_StatNextEntry failed(%d)\r\n", ret_tmp);

    if(bUseIoctl > FSLINUX_IOCTL_INVALID)
    {
        if(0 != close((int)dirfd))
            DBG_ERR("close %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
    }
    else
    {
        if(0 != closedir((DIR *)dirfd))
            DBG_ERR("closedir %s: errno = %d, errmsg = %s\r\n", pPath, errno, strerror(errno));
    }

    pArg->Linux_dirfd = 0;
}

static void FsLinux_CmdId_ReadDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_READDIR *pArg = (FSLINUX_ARG_READDIR *)ArgAddr;

    FSLINUX_FIND_DATA_ITEM *pItemBuf = (FSLINUX_FIND_DATA_ITEM *)FsLinux_GetNonCacheAddr((unsigned int)pArg->phy_pItemBuf);
    unsigned int TmpItemNum = pArg->ItemNum;

    int ret_tmp;

    //readdir N items
    ret_tmp = fslinux_StatNextEntry(pArg->Linux_dirfd, pItemBuf, &TmpItemNum, pArg->bUseIoctl);
    if(FSLINUX_STA_OK == ret_tmp || FSLINUX_STA_FS_NO_MORE_FILES == ret_tmp)
    {
        pArg->ItemNum = TmpItemNum;
        pArg->ret_fslnx = ret_tmp;
        return;
    }
    else
    {
        pArg->ItemNum = 0;
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
    }
}

static void FsLinux_CmdId_UnlinkatDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_UNLINKATDIR *pArg = (FSLINUX_ARG_UNLINKATDIR *)ArgAddr;

    FSLINUX_FIND_DATA_ITEM *pItemBuf = (FSLINUX_FIND_DATA_ITEM *)FsLinux_GetNonCacheAddr((unsigned int)pArg->phy_pItemBuf);
    unsigned int ItemLeft = pArg->ItemNum;
    unsigned int bUseIoctl = pArg->bUseIoctl;

    FSLINUX_FIND_DATA_ITEM *pCurItem;
    int fail_count = 0;

    DBG_IND("bUseIoctl(%d)\r\n", bUseIoctl);
    if(bUseIoctl)
    {
        char szFullPath[FSLINUX_LONGNAME_PATH_MAX_LENG];

        for(pCurItem = pItemBuf; ItemLeft--; pCurItem++)
        {
            if(0 == pCurItem->flag || (pCurItem->FindData.attrib & FSLINUX_ATTRIB_DIRECTORY))
                continue;

            DBG_IND("delete %s\r\n", pCurItem->FindData.filename);

            sprintf(szFullPath, "%s/%s", pArg->szPath, pCurItem->FindData.filename);

            if(0 != unlink(szFullPath))
            {
                DBG_WRN("unlink %s: errno = %d, errmsg = %s\r\n", szFullPath, errno, strerror(errno));
                fail_count++;
            }
        }
    }
    else
    {
        int curfd = dirfd((DIR *)pArg->Linux_dirfd);

        for(pCurItem = pItemBuf; ItemLeft--; pCurItem++)
        {
            if(0 == pCurItem->flag || (pCurItem->FindData.attrib & FSLINUX_ATTRIB_DIRECTORY))
                continue;

            DBG_IND("delete %s\r\n", pCurItem->FindData.filename);

            if(0 != unlinkat(curfd, pCurItem->FindData.filename, 0))
            {
                DBG_WRN("unlinkat %s: errno = %d, errmsg = %s\r\n", pCurItem->FindData.filename, errno, strerror(errno));
                fail_count++;
            }
        }
    }

    if(0 == fail_count)
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_LockDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_LOCKDIR *pArg = (FSLINUX_ARG_LOCKDIR *)ArgAddr;

    FSLINUX_FIND_DATA_ITEM *pItemBuf = (FSLINUX_FIND_DATA_ITEM *)FsLinux_GetNonCacheAddr((unsigned int)pArg->phy_pItemBuf);
    unsigned int ItemLeft = pArg->ItemNum;
    int bLock = pArg->bLock;
    unsigned int bUseIoctl = pArg->bUseIoctl;

    FSLINUX_FIND_DATA_ITEM *pCurItem;
    int fail_count = 0;

    DBG_IND("bUseIoctl(%d), bLock = %d\r\n", bUseIoctl, bLock);

    if(bUseIoctl)
    {
        char szFullPath[FSLINUX_LONGNAME_PATH_MAX_LENG];

        for(pCurItem = pItemBuf; ItemLeft--; pCurItem++)
        {
            DBG_IND("%s: bApply = %d\r\n", pCurItem->FindData.filename, pCurItem->flag);

            if(0 == pCurItem->flag)
                continue;//do not apply, skip to the next

            sprintf(szFullPath, "%s/%s", pArg->szPath, pCurItem->FindData.filename);

            if(-1 == fslinux_IoctlAttrib(szFullPath, FSLINUX_ATTRIB_READONLY, bLock))
            {
                fail_count++;
                continue;
            }
        }
    }
    else
    {
        DIR *pDir = (DIR *)pArg->Linux_dirfd;
        int curfd = dirfd(pDir);
        struct stat TmpStat = {0};
        mode_t newmode;

        for(pCurItem = pItemBuf; ItemLeft--; pCurItem++)
        {
            DBG_IND("%s: bApply = %d\r\n", pCurItem->FindData.filename, pCurItem->flag);

            if(0 == pCurItem->flag)
                continue;//do not apply, skip to the next

            if(0 != fstatat(curfd, pCurItem->FindData.filename, &TmpStat, 0))
            {
                DBG_WRN("fstatat %s: errno = %d, errmsg = %s\r\n", pCurItem->FindData.filename, errno, strerror(errno));
                fail_count++;
                continue;
            }

            if(bLock)
                newmode = TmpStat.st_mode & (~(S_IWUSR|S_IWGRP|S_IWOTH));//set readonly => remove all write permissions
            else
                newmode = TmpStat.st_mode | (S_IWUSR|S_IWGRP|S_IWOTH);//remove readonly => set all write permissions

            if(0 != fchmodat(curfd, pCurItem->FindData.filename, newmode, 0))
            {
                DBG_WRN("fchmodat %s: errno = %d, errmsg = %s\r\n", pCurItem->FindData.filename, errno, strerror(errno));
                fail_count++;
            }
        }
    }

    if(0 == fail_count)
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_CloseDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_CLOSEDIR *pArg = (FSLINUX_ARG_CLOSEDIR *)ArgAddr;
    unsigned int bUseIoctl = pArg->bUseIoctl;

    DBG_IND("bUseIoctl(%d)\r\n", bUseIoctl);
    if(bUseIoctl)
    {
        if(0 == close((int)pArg->Linux_dirfd))
        {
            pArg->ret_fslnx = FSLINUX_STA_OK;
            return;
        }
    }
    else
    {
        if(0 == closedir((DIR *)pArg->Linux_dirfd))
        {
            pArg->ret_fslnx = FSLINUX_STA_OK;
            return;
        }
    }

    DBG_ERR("errno = %d, errmsg = %s\r\n", errno, strerror(errno));
    pArg->ret_fslnx = FSLINUX_STA_ERROR;
}
static void FsLinux_CmdId_RewindDir(unsigned int ArgAddr)
{
    FSLINUX_ARG_REWINDDIR *pArg = (FSLINUX_ARG_REWINDDIR *)ArgAddr;
    unsigned int bUseIoctl = pArg->bUseIoctl;

    DBG_IND("bUseIoctl(%d)\r\n", bUseIoctl);
    if(bUseIoctl)
    {
        if(lseek((int)pArg->Linux_dirfd, 0, SEEK_SET) != (off_t)-1)
        {
            pArg->ret_fslnx = FSLINUX_STA_OK;
            return;
        }
    }
    else
    {
        rewinddir((DIR *)pArg->Linux_dirfd);//rewinddir return void
        pArg->ret_fslnx = FSLINUX_STA_OK;
        return;
    }

    DBG_ERR("errno = %d, errmsg = %s\r\n", errno, strerror(errno));
    pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_CopyToByName(unsigned int ArgAddr)
{
    FSLINUX_ARG_COPYTOBYNAME *pArg = (FSLINUX_ARG_COPYTOBYNAME *)ArgAddr;

    if(0 == fslinux_CopyFile(pArg->szSrcPath, pArg->szDstPath, pArg->bDelete))
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_GetDiskInfo(unsigned int ArgAddr)
{
    FSLINUX_ARG_GETDISKINFO *pArg = (FSLINUX_ARG_GETDISKINFO *)ArgAddr;

    char *pPath = pArg->szPath;;

    struct statfs TmpStatFs = {0};
    int ret_statfs;

    DBG_IND("statfs(%s, &TmpStatFs)\r\n", pPath);
    ret_statfs = statfs(pPath, &TmpStatFs);
    DBG_IND("ret_statfs = %d\r\n", ret_statfs);

    if(ret_statfs != 0)
    {
        DBG_ERR("ret_statfs: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
		if (errno == EIO) {
			pArg->ret_fslnx = FSLINUX_STA_SDIO_ERR;
		}
        return;
    }

    if( MSDOS_SUPER_MAGIC == TmpStatFs.f_type ||
        EXFAT_SUPER_MAGIC == TmpStatFs.f_type ||
        TMPFS_MAGIC == TmpStatFs.f_type ||
        EXT4_SUPER_MAGIC == TmpStatFs.f_type ||
        EXT3_SUPER_MAGIC == TmpStatFs.f_type ||
        EXT2_SUPER_MAGIC == TmpStatFs.f_type)
    {
        DBG_IND("f_bsize = %lld, f_blocks = %lld, f_bfree = %lld\r\n",
            (unsigned long long)TmpStatFs.f_bsize,
            (unsigned long long)TmpStatFs.f_blocks,
            (unsigned long long)TmpStatFs.f_bfree);
        pArg->ClusSize = TmpStatFs.f_bsize;
        pArg->TotalClus = TmpStatFs.f_blocks;
        pArg->FreeClus = TmpStatFs.f_bfree;

        if (EXFAT_SUPER_MAGIC == TmpStatFs.f_type ||
			EXT4_SUPER_MAGIC == TmpStatFs.f_type ||
			EXT3_SUPER_MAGIC == TmpStatFs.f_type ||
	        EXT2_SUPER_MAGIC == TmpStatFs.f_type)
            pArg->FsType = FSLINUX_EXFAT; //support file size more than 4GB
        else
            pArg->FsType = FSLINUX_FAT32;

        pArg->ret_fslnx = FSLINUX_STA_OK;
    }
    else
    {
        DBG_ERR("unknown f_type = 0x%lX\r\n", TmpStatFs.f_type);
        DBG_ERR("f_bsize = %lld\r\n", (unsigned long long)TmpStatFs.f_bsize);
        DBG_ERR("f_blocks = %lld\r\n", (unsigned long long)TmpStatFs.f_blocks);
        DBG_ERR("f_bfree = %lld\r\n", (unsigned long long)TmpStatFs.f_bfree);
        pArg->ClusSize = 0;
        pArg->TotalClus = 0;
        pArg->FreeClus = 0;
        pArg->FsType = 0;
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
    }
}

static void FsLinux_CmdId_FormatAndLabel(unsigned int ArgAddr)
{
    FSLINUX_ARG_FORMATANDLABEL *pArg = (FSLINUX_ARG_FORMATANDLABEL *)ArgAddr;

    char *pMountPath = pArg->szMountPath;
    char *pNewLabel = pArg->szNewLabel;
    unsigned int bIsSupportExfat = pArg->bIsSupportExfat;

    char *arg_list[7] = {NULL};
		//e.g. mkfs.vfat /dev/mmcblk0p1 -n NEWLABEL (NULL)
		//e.g. mkexfatfs /dev/mmcblk0p1 -n NEWLABEL -s 128 (NULL)
    char **parg_cur = arg_list;
    char arg_dev[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};//e.g. /dev/mmcblk0p1
    char arg_bin_vfat[] = "mkfs.vfat";
    char arg_bin_exfat[] = "mkexfatfs";
    char arg_bin_ext4[] = "mkfs.ext4";
    char arg_opt_label[] = "-n";
    char arg_opt_sect[] = "-s";
    char arg_val_sectperclus[] = "128";

    int ret_cmd;
    long long DevSize;
	int bexfat = 0;
	int bvfat = 0;
	int bext4 = 0;

	//mbr parsing supplement
	unsigned char type_mbr = 0;

    if(fslinux_GetDevByMountPath(arg_dev, pMountPath, sizeof(arg_dev)) == 0)
    {//get the device name from the mount list
        DBG_IND("Found %s is mounted on %s\r\n", arg_dev, pMountPath);

        if(fslinux_UmountDisk(pMountPath) < 0)
        {
            DBG_ERR("fslinux_UmountDisk %s failed\r\n", pMountPath);
            pArg->ret_fslnx = FSLINUX_STA_ERROR;
            return;
        }
    }
    else if(fslinux_GetSD1DevByBus(arg_dev, sizeof(arg_dev)) != 0)
    {//if the device is not mounted, get the dev name from the mmc0 bus
        DBG_ERR("fslinux_GetSD1DevByBus failed\r\n");
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    if(fslinux_GetBlkDevSize(arg_dev, &DevSize) < 0)
    {
        DBG_ERR("fslinux_GetBlkDevSize %s failed\r\n", arg_dev);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    DBG_IND("dev[%s], DevSize %lld, bIsSupportExfat %d\r\n", arg_dev, DevSize, bIsSupportExfat);

	//set bin file name
	if (fslinux_is_bus0_emmc())
	{//eMMC storage
		*parg_cur++ = arg_bin_ext4;
		bext4 = 1;
	}
	else
	{//original SD card
	    if(bIsSupportExfat && DevSize > 32*1024*1024*1024LL)
	    {//exFAT
			*parg_cur++ = arg_bin_exfat;
			bexfat = 1;
	    }
	    else
	    {//VFAT
			*parg_cur++ = arg_bin_vfat;
			bvfat = 1;
	    }
	}

	//set device name
	*parg_cur++ = arg_dev;

	//set label name
	if (pNewLabel[0]) {
		*parg_cur++ = arg_opt_label;
		*parg_cur++ = pNewLabel;
	}

	//set secter per cluster for exFAT only
	if (bexfat) {
		*parg_cur++ = arg_opt_sect;
		*parg_cur++ = arg_val_sectperclus;
	}

	if (strlen(arg_dev) > 12) { //has mbr

		//get new mbr type
		if (bext4 == 1) {
			type_mbr = 0x83;
		}
		else if (bexfat == 1) {
			type_mbr = 0x07;
		}
		else if (bvfat == 1) {
			if(DevSize < (8032*1024*1024LL + 512*1024LL)) {
				type_mbr = 0x0b;
			}
			else {
				type_mbr = 0x0c;
			}
		}
		else {
			DBG_ERR("format %s type unknown\r\n", arg_dev);
			pArg->ret_fslnx = FSLINUX_STA_ERROR;
			return;
		}

		//set mbr type
		if(fslinux_set_mbr_type(arg_dev, type_mbr) < 0) {
			DBG_ERR("fslinux_set_mbr_type %s failed\r\n", arg_dev);
			//pArg->ret_fslnx = FSLINUX_STA_ERROR;
			//return;
		}
	}

    ret_cmd = fslinux_SpawnCmd(arg_list);
    DBG_IND("ret = %d\r\n", ret_cmd);

    if(ret_cmd != 0)
        DBG_ERR("fslinux_SpawnCmd failed\r\n");

    //mount back
    if(fslinux_AutoMount(arg_dev, pMountPath) < 0)
    {
        DBG_ERR("mount %s %s failed\r\n", arg_dev, pMountPath);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    if(0 == ret_cmd)
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_GetLabel(unsigned int ArgAddr)
{
    FSLINUX_ARG_GETLABEL *pArg = (FSLINUX_ARG_GETLABEL *)ArgAddr;

    char *pMountPath = pArg->szMountPath;
    char *pOutLabel = pArg->szLabel;

    char cmd_buf[FSLINUX_LONGNAME_PATH_MAX_LENG] = "blkid ";
	char *dev_path;
	int dev_len;
    FILE *ret_popen;

	*pOutLabel = '\0'; //set to NULL string first
	dev_path = cmd_buf + strlen(cmd_buf);
	dev_len = sizeof(cmd_buf) - strlen(cmd_buf);

	//get the device name from the mount list
	if (0 != fslinux_GetDevByMountPath(dev_path, pMountPath, dev_len)) {
		//if the device is not mounted, get the dev name from the mmc0 bus
		if(0 != fslinux_GetSD1DevByBus(dev_path, dev_len)) {
	        DBG_ERR("fslinux_GetSD1DevByBus failed\r\n");
	        pArg->ret_fslnx = FSLINUX_STA_ERROR;
	        return;
	    }
	}

    DBG_IND("cmd_buf[%s]\r\n", cmd_buf);
	ret_popen = popen(cmd_buf, "r");
	if (ret_popen != NULL )	{
		//the result of blkid, e.g. /dev/mmcblk0p1: LABEL="MYLABEL" UUID="C631-DCB9"
		#define LABEL_TAG_BEG "LABEL=\""
		#define LABEL_TAG_END "\""

		char buf_blkid[128] = {0};
		char *plabel_beg = NULL, *plabel_end = NULL;

		if (NULL == fgets(buf_blkid, sizeof(buf_blkid), ret_popen))
			DBG_ERR("fgets failed\r\n");

		if (-1 == pclose(ret_popen))
			DBG_ERR("pclose: errno = %d, errmsg = %s\r\n", errno, strerror(errno));

		DBG_IND("blkid result [%s]\r\n", buf_blkid);

		plabel_beg = strstr(buf_blkid, LABEL_TAG_BEG);
		if (NULL != plabel_beg) {
			plabel_beg += strlen(LABEL_TAG_BEG);
			plabel_end = strstr(plabel_beg, LABEL_TAG_END);
			if (plabel_end)
				*plabel_end = '\0';
			else
				DBG_ERR("parsing plabel_end failed\r\n");

			KFS_STRCPY(pOutLabel, plabel_beg, FSLINUX_FILENAME_MAX_LENG);
			DBG_IND("pOutLabel = %s\r\n", pOutLabel);
		}
	}

    if('\0' != *pOutLabel)
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}

static void FsLinux_CmdId_SetParam(unsigned int ArgAddr)
{
	FSLINUX_ARG_SETPARAM *pArg = (FSLINUX_ARG_SETPARAM *)ArgAddr;

	char *pMountPath = pArg->szMountPath;
	unsigned int bIsSupportExfat = pArg->bIsSupportExfat;
	unsigned int param_id = pArg->param_id;
	unsigned int value = pArg->value;

	int ret_cmd = -1;

	switch (param_id) {

	case FSLINUX_PARM_UPDATE_MOUNT_STATE:
		ret_cmd = fslinux_set_mount_state(pMountPath, value, bIsSupportExfat);
		break;

	case FSLINUX_PARM_SKIP_SYNC:
		ret_cmd = fslinux_set_skip_sync(value);
		break;

	case FSLINUX_PARM_CHECK_DISK:
		ret_cmd = fslinux_check_disk(pMountPath, value, bIsSupportExfat);
		break;

	default:

		break;
	}

	if(0 == ret_cmd)
		pArg->ret_fslnx = FSLINUX_STA_OK;
	else
		pArg->ret_fslnx = FSLINUX_STA_ERROR;

	return ;
}

/*
static long fslinux_GetFsType(char *pMountPath, long *pFsType)
{
    struct statfs TmpStatFs = {0};
    int ret_statfs;

    *pFsType = 0;//set to empty first

    ret_statfs = statfs(pMountPath, &TmpStatFs);
    if(ret_statfs != 0)
    {
        DBG_ERR("ret_statfs: errno = %d, errmsg = %s\r\n", errno, strerror(errno));
        return -1;
    }

    if (TmpStatFs.f_type == MSDOS_SUPER_MAGIC ||
        TmpStatFs.f_type == EXFAT_SUPER_MAGIC)
    {
        DBG_IND("f_type = %ld\r\n", TmpStatFs.f_type);
        *pFsType = (long)TmpStatFs.f_type;
        return 0;
    }

    DBG_ERR("unknown f_type = 0x%lX\r\n", TmpStatFs.f_type);
    return -1;
}

static void FsLinux_CmdId_SetDiskLabel(unsigned int ArgAddr)
{
    FSLINUX_ARG_SETDISKLABEL *pArg = (FSLINUX_ARG_SETDISKLABEL *)ArgAddr;

    char *pMountPath = pArg->szMountPath;
    char *pNewLabel = pArg->szNewLabel;
    unsigned int bIsSupportExfat = pArg->bIsSupportExfat;

    char DevPath[FSLINUX_LONGNAME_PATH_MAX_LENG] = {0};
    char *arg_list_vfat[] = {"fatlabel", DevPath, pNewLabel, NULL};//e.g. fatlabel /dev/mmcblk0p1 LABEL123
    char *arg_list_exfat[] = {"exfatlabel", DevPath, pNewLabel, NULL};//e.g. fatlabel /dev/mmcblk0p1 LABEL123
    char **arg_list_cur;
    int ret_label;
    long FsType;

    //The device should be mounted. Get the device name from the mount path
    if(fslinux_GetDevByMountPath(DevPath, pMountPath, sizeof(DevPath)) != 0)
    {
        DBG_ERR("fslinux_GetDevicePath from %s failed\r\n", pMountPath);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    //check the file system type
    if(fslinux_GetFsType(pMountPath, &FsType) != 0)
    {
        DBG_ERR("fslinux_GetFsType from %s failed\r\n", pMountPath);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    if(bIsSupportExfat && EXFAT_SUPER_MAGIC == FsType)
        arg_list_cur = arg_list_exfat;
    else if(MSDOS_SUPER_MAGIC == FsType)
        arg_list_cur = arg_list_vfat;
    else
    {
        DBG_ERR("Unsupported FsType = 0x%lX, bIsSupportExfat = %d\r\n", FsType, bIsSupportExfat);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    //umount before setlabel
    if(fslinux_UmountDisk(pMountPath) < 0)
    {
        DBG_ERR("fslinux_UmountDisk failed\r\n");
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    DBG_IND("fslinux_SpawnCmd(arg_list[] = {%s, %s, %s}\r\n", arg_list_cur[0], arg_list_cur[1], arg_list_cur[2]);
    ret_label = fslinux_SpawnCmd(arg_list_cur);
    DBG_IND("ret_label = %d\r\n", ret_label);

    //only show error messages here. keep on the mount operation
    if(ret_label != 0)
        DBG_ERR("fslinux_spawncmd {%s, %s, %s} failed\r\n", arg_list_cur[0], arg_list_cur[1], arg_list_cur[2]);

    //mount back after setlabel
    if(fslinux_AutoMount(DevPath, pMountPath, FsType) < 0)
    {
        DBG_ERR("mount %s %s failed\r\n", DevPath, pMountPath);
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
        return;
    }

    //return OK only if the setlabel operation is successful
    if(0 == ret_label)
        pArg->ret_fslnx = FSLINUX_STA_OK;
    else
        pArg->ret_fslnx = FSLINUX_STA_ERROR;
}
*/
