/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_file
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/file.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_FILE_INITED_TAG       MAKEFOURCC('R', 'F', 'I', 'L') ///< a key value

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void rtos_file_init(void *param)
{
}

void rtos_file_exit(void)
{
}

VOS_FILE vos_file_open(const char *pathname, int flags, vos_mode_t mode)
{
	int fd;

	fd = open(pathname, flags, mode);

	if(-1 == fd) {
		DBG_ERR("open [%s] failed\r\n", pathname);
		return VOS_FILE_INVALID;
	}

	return (VOS_FILE)fd;
}

int vos_file_read(VOS_FILE vos_file, void *p_buf, vos_size_t count)
{
	int fd = (int)vos_file;
	int read_bytes;

	read_bytes = read(fd, p_buf, (size_t)count);

	return read_bytes;
}

int vos_file_write(VOS_FILE vos_file, const void *p_buf, vos_size_t count)
{
	int fd = (int)vos_file;
	int written_bytes;

	written_bytes = write(fd, p_buf, (size_t)count);

	return written_bytes;
}

int vos_file_close(VOS_FILE vos_file)
{
	int fd = (int)vos_file;
	int ret;

	if (0 != fsync(fd)) {
		DBG_WRN("fsync fail, vos_file 0x%lX\r\n", (ULONG)vos_file);
	}

	ret = close(fd);

	return ret;
}

vos_off_t vos_file_lseek(VOS_FILE vos_file, vos_off_t offset, int whence)
{
	int fd = (int)vos_file;
	off_t ret_offset;

	ret_offset = lseek(fd, (off_t)offset, whence);

	return (vos_off_t)ret_offset;
}

int vos_file_fstat(VOS_FILE vos_file, struct vos_stat *p_stat)
{
	struct stat statbuf = {0};
	int fd = (int)vos_file;
	int ret;

	if (NULL == p_stat) {
		return -1;
	}

	ret = fstat(fd, &statbuf);
	if (0 == ret) {
		//success, fill data
		p_stat->st_mode = (vos_mode_t)statbuf.st_mode;
		p_stat->st_size = (vos_size_t)statbuf.st_size;
	}

	return ret;
}

int vos_file_stat(const char *pathname, struct vos_stat *p_stat)
{
	struct stat statbuf = {0};
	int ret;

	if (NULL == p_stat) {
		return -1;
	}

	ret = stat(pathname, &statbuf);
	if (0 == ret) {
		//success, fill data
		p_stat->st_mode = (vos_mode_t)statbuf.st_mode;
		p_stat->st_size = (vos_size_t)statbuf.st_size;
	}

	return ret;
}

int vos_file_fsync(VOS_FILE vos_file)
{
	int fd = (int)vos_file;
	int ret;

	ret = fsync(fd);

	return ret;
}
