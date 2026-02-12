#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <linux/fs.h>
#include <errno.h>
#include "sdio_int.h"

#define CFG_DEV_SDIO "/dev/mmcblk0"

# if 0
struct mtd_info_user {
	__u8 type;
	__u32 flags;
	__u32 size;     /* Total size of the MTD */
	__u32 erasesize;
	__u32 writesize;
	__u32 oobsize;  /* Amount of OOB data per block (e.g. 16) */
	__u64 padding;  /* Old obsolete field; do not use */
};
#endif

typedef struct _STRG_CTRL {
	int fd;
	int ref_cnt;
	int blk_size; // always 512
	uint64_t dev_size;
} STRG_CTRL;


static pthread_mutex_t m_mutex_local = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_mutex_global = PTHREAD_MUTEX_INITIALIZER;
static STRG_CTRL m_ctrl = {0};

ER sdio_lock(void)
{
	pthread_mutex_lock(&m_mutex_global);
	return 0;
}

ER sdio_unlock(void)
{
	pthread_mutex_unlock(&m_mutex_global);
	return 0;
}

static void _sdio_lock(void)
{
	pthread_mutex_lock(&m_mutex_local);
}

static void _sdio_unlock(void)
{
	pthread_mutex_unlock(&m_mutex_local);
}

int sdio_open(void)
{
	int i, fd;

	_sdio_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->ref_cnt < 0) {
		DBG_ERR("assert, ref_cnt=%d\n", p_ctrl->ref_cnt);
		_sdio_unlock();
		return -1;
	} else if (p_ctrl->ref_cnt > 0) {
		p_ctrl->ref_cnt++;
		_sdio_unlock();
		return 0;
	}

	p_ctrl->fd = open(CFG_DEV_SDIO, O_RDWR);
	if (p_ctrl->fd < 0) {
		DBG_ERR("failed to open %s\n", CFG_DEV_SDIO);
		_sdio_unlock();
		return -1;
	}

	uint64_t size;
	if (ioctl(p_ctrl->fd,BLKGETSIZE64,&p_ctrl->dev_size)==-1) {
		printf("%s",strerror(errno));
		return -1;
	}

	p_ctrl->blk_size = 512;

	p_ctrl->ref_cnt++;
	_sdio_unlock();
	return 0;
}

int sdio_close(void)
{
	_sdio_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("assert, ref_cnt=%d\n", p_ctrl->ref_cnt);
		_sdio_unlock();
		return -1;
	}

	close(p_ctrl->fd);
	p_ctrl->ref_cnt--;

	_sdio_unlock();
	return 0;
}

int sdio_get_block_size(void)
{
	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("sdio not opened.\n");
		return -1;
	}
	return p_ctrl->blk_size;
}

loff_t sdio_get_phy_size(void)
{
	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("sdio not opened.\n");
		return -1;
	}
	return (loff_t)p_ctrl->dev_size;
}

static int sdio_readblock(unsigned char *p_buf, unsigned int blk_ofs)
{
	_sdio_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("sdio not opened.\n");
		return -1;
	}

	loff_t offset = (loff_t)blk_ofs * (loff_t)p_ctrl->blk_size;

	if (lseek(p_ctrl->fd, offset, SEEK_SET) == -1) {
		DBG_ERR("failed to lseek 0x%llx\n", offset);
		_sdio_unlock();
		return -1;
	}

	ssize_t loaded;
	if ((loaded = read(p_ctrl->fd, p_buf, p_ctrl->blk_size)) != (ssize_t)p_ctrl->blk_size) {
		DBG_ERR("read failed, got %d\n", loaded);
		_sdio_unlock();
		return -1;
	}

	_sdio_unlock();
	return 0;
}

static int sdio_writeblock(unsigned char *p_buf, unsigned int blk_ofs)
{
	_sdio_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("sdio not opened.\n");
		return -1;
	}

	loff_t offset = (loff_t)blk_ofs * (loff_t)p_ctrl->blk_size;

	if (lseek(p_ctrl->fd, offset, SEEK_SET) == -1) {
		DBG_ERR("failed to lseek 0x%llx\n", offset);
		_sdio_unlock();
		return -1;
	}

	ssize_t written;
	if ((written = write(p_ctrl->fd, p_buf, p_ctrl->blk_size)) != (ssize_t)p_ctrl->blk_size) {
		DBG_ERR("write failed, got %d\n", written);
		_sdio_unlock();
		return -1;
	}
	_sdio_unlock();
	return 0;
}

int sdio_read(SDIO_RW *p_rw)
{
	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("sdio not opened.\n");
		return -1;
	}

	unsigned char *p_buf = p_rw->p_buf;
	unsigned int n = p_rw->blk_num;
	unsigned int blk_ofs = p_rw->blk_ofs;

	while (n > 0) {
		// check boundary
		loff_t offset = (loff_t)blk_ofs * p_ctrl->blk_size;
		if (offset < p_rw->boundary.top || offset > p_rw->boundary.bottom) {
			DBG_ERR("offset 0x%llX out of boundary [0x%llX-0x%llX]\n",
				offset,
				p_rw->boundary.top,
				p_rw->boundary.bottom);
			return -1;
		}

		int er = sdio_readblock(p_buf, blk_ofs);
		if ( er == -2) {
			// bad block
			continue;
		} else if (er < 0) {
			return er;
		}

		p_buf += p_ctrl->blk_size;
		blk_ofs++;
		n--;
	}
	return 0;
}

int sdio_write(SDIO_RW *p_rw)
{
	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("sdio not opened.\n");
		return -1;
	}

	unsigned char *p_buf = p_rw->p_buf;
	unsigned int n = p_rw->blk_num;
	unsigned int blk_ofs = (loff_t)p_rw->blk_ofs;

	while (n > 0) {
		// check boundary
		loff_t offset = blk_ofs * p_ctrl->blk_size;
		if (offset < p_rw->boundary.top || offset > p_rw->boundary.bottom) {
			DBG_ERR("offset 0x%llX out of boundary [0x%llX-0x%llX]\n",
				offset,
				p_rw->boundary.top,
				p_rw->boundary.bottom);
			return -1;
		}

		int er = sdio_writeblock(p_buf, blk_ofs);
		if ( er == -2) {
			// bad block
			continue;
		} else if (er < 0) {
			return er;
		}

		p_buf += p_ctrl->blk_size;
		blk_ofs++;
		n--;
	}
	return 0;
}