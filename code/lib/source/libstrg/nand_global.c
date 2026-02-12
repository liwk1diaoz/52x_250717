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
#include <mtd/mtd-user.h>
#include "nand_int.h"

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
	mtd_info_t meminfo;
} STRG_CTRL;


static pthread_mutex_t m_mutex_local = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_mutex_global = PTHREAD_MUTEX_INITIALIZER;
static STRG_CTRL m_ctrl = {0};

ER nand_lock(void)
{
	pthread_mutex_lock(&m_mutex_global);
	return 0;
}

ER nand_unlock(void)
{
	pthread_mutex_unlock(&m_mutex_global);
	return 0;
}

static void _nand_lock(void)
{
	pthread_mutex_lock(&m_mutex_local);
}

static void _nand_unlock(void)
{
	pthread_mutex_unlock(&m_mutex_local);
}

int nand_open(void)
{
	int i, fd;

	_nand_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->ref_cnt < 0) {
		DBG_ERR("assert, ref_cnt=%d\n", p_ctrl->ref_cnt);
		_nand_unlock();
		return -1;
	} else if (p_ctrl->ref_cnt > 0) {
		p_ctrl->ref_cnt++;
		_nand_unlock();
		return 0;
	}

	// find the named 'all' partition
	int loaded;
	int mdt_all_idx = -1;
	char sz_tmp[65] = {0}; // linux report as string

	for (i = 0; i < 256; i++) {
		snprintf(sz_tmp, sizeof(sz_tmp) - 1, "/sys/devices/virtual/mtd/mtd%d/name", i);
		if (access(sz_tmp, R_OK) != 0) {
			break;
		}
		int fd = open(sz_tmp, O_RDONLY);
		memset(sz_tmp, 0, sizeof(sz_tmp));
		if ((loaded = read(fd, sz_tmp, sizeof(sz_tmp))) != 0) {
			if (strncmp(sz_tmp, "all\n", 5) == 0) {
				mdt_all_idx = i;
				close(fd);
				break;
			}
		}
		close(fd);
	}

	// open global fd
	snprintf(sz_tmp, sizeof(sz_tmp) - 1, "/dev/mtd%d", mdt_all_idx);
	p_ctrl->fd = open(sz_tmp, O_RDWR);
	if (p_ctrl->fd < 0) {
		DBG_ERR("failed to open %s\n", sz_tmp);
		_nand_unlock();
		return -1;
	}

	ioctl(p_ctrl->fd, MEMGETINFO, &p_ctrl->meminfo);

#if 0
	DBGH(p_ctrl->meminfo.type);
	DBGH(p_ctrl->meminfo.flags);
	DBGH(p_ctrl->meminfo.size);
	DBGH(p_ctrl->meminfo.erasesize);
	DBGH(p_ctrl->meminfo.writesize);
	DBGH(p_ctrl->meminfo.oobsize);
	DBGH(p_ctrl->meminfo.padding);
#endif

	if (p_ctrl->meminfo.type != MTD_NANDFLASH) {
		DBG_ERR("%s: not a nand flash", sz_tmp);
		_nand_unlock();
		return -1;
	}
	p_ctrl->ref_cnt++;
	_nand_unlock();
	return 0;
}

int nand_close(void)
{
	_nand_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("assert, ref_cnt=%d\n", p_ctrl->ref_cnt);
		_nand_unlock();
		return -1;
	}

	close(p_ctrl->fd);
	p_ctrl->ref_cnt--;

	_nand_unlock();
	return 0;
}

int nand_get_block_size(void)
{
	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->meminfo.erasesize) {
		//nand was opened before.
		return (loff_t)p_ctrl->meminfo.erasesize;
	} else if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("nand not opened.\n");
		return -1;
	}
	return p_ctrl->meminfo.erasesize;
}

loff_t nand_get_phy_size(void)
{
	STRG_CTRL *p_ctrl = &m_ctrl;
	if (p_ctrl->meminfo.size) {
		//nand was opened before.
		return (loff_t)p_ctrl->meminfo.size;
	} else if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("nand not opened.\n");
		return -1;
	}
	return (loff_t)p_ctrl->meminfo.size;
}

static int nand_readblock(unsigned char *p_buf, unsigned int blk_ofs)
{
	_nand_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("nand not opened.\n");
		return -1;
	}

	loff_t offset = blk_ofs * p_ctrl->meminfo.erasesize;

	// check bad block
	if (ioctl(p_ctrl->fd, MEMGETBADBLOCK, &offset) < 0) {
		DBG_WRN("Skipping bad block at 0x%08x", offset);
		_nand_unlock();
		return -2;
	}

	if (lseek(p_ctrl->fd, offset, SEEK_SET) == -1) {
		DBG_ERR("failed to lseek 0x%08X\n", offset);
		_nand_unlock();
		return -1;
	}

	ssize_t loaded;
	if ((loaded = read(p_ctrl->fd, p_buf, p_ctrl->meminfo.erasesize)) != (ssize_t)p_ctrl->meminfo.erasesize) {
		DBG_ERR("read failed, got %d\n", loaded);
		_nand_unlock();
		return -1;
	}

	_nand_unlock();
	return 0;
}

static int nand_writeblock(unsigned char *p_buf, unsigned int blk_ofs)
{
	_nand_lock();

	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("nand not opened.\n");
		return -1;
	}

	loff_t offset = blk_ofs * p_ctrl->meminfo.erasesize;

	// check bad block
	if (ioctl(p_ctrl->fd, MEMGETBADBLOCK, &offset) < 0) {
		DBG_WRN("Skipping bad block at 0x%08x", offset);
		_nand_unlock();
		return -2;
	}

	erase_info_t erase;
	erase.start = offset;
	erase.length = p_ctrl->meminfo.erasesize;
	if (ioctl(p_ctrl->fd, MEMERASE, &erase) < 0) {
		DBG_WRN("erase failed at 0x%08x", offset);
		_nand_unlock();
		return -2;
	}

	if (lseek(p_ctrl->fd, offset, SEEK_SET) == -1) {
		DBG_ERR("failed to lseek 0x%08X\n", offset);
		_nand_unlock();
		return -1;
	}

	ssize_t written;
	if ((written = write(p_ctrl->fd, p_buf, p_ctrl->meminfo.erasesize)) != (ssize_t)p_ctrl->meminfo.erasesize) {
		DBG_ERR("write failed, got %d\n", written);
		_nand_unlock();
		return -1;
	}
	_nand_unlock();
	return 0;
}

int nand_read(NAND_RW *p_rw)
{
	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("nand not opened.\n");
		return -1;
	}

	unsigned char *p_buf = p_rw->p_buf;
	unsigned int n = p_rw->blk_num;
	unsigned int blk_ofs = p_rw->blk_ofs;

	while (n > 0) {
		// check boundary
		loff_t offset = (loff_t)blk_ofs * p_ctrl->meminfo.erasesize;
		if (offset < p_rw->boundary.top || offset > p_rw->boundary.bottom) {
			DBG_ERR("offset 0x%llX out of boundary [0x%llX-0x%llX]\n",
				offset,
				p_rw->boundary.top,
				p_rw->boundary.bottom);
			return -1;
		}

		int er = nand_readblock(p_buf, blk_ofs);
		if ( er == -2) {
			// bad block
			continue;
		} else if (er < 0) {
			return er;
		}

		p_buf += p_ctrl->meminfo.erasesize;
		blk_ofs++;
		n--;
	}
	return 0;
}

int nand_write(NAND_RW *p_rw)
{
	STRG_CTRL *p_ctrl = &m_ctrl;

	if (p_ctrl->ref_cnt <= 0) {
		DBG_ERR("nand not opened.\n");
		return -1;
	}

	unsigned char *p_buf = p_rw->p_buf;
	unsigned int n = p_rw->blk_num;
	unsigned int blk_ofs = (loff_t)p_rw->blk_ofs;

	while (n > 0) {
		// check boundary
		loff_t offset = blk_ofs * p_ctrl->meminfo.erasesize;
		if (offset < p_rw->boundary.top || offset > p_rw->boundary.bottom) {
			DBG_ERR("offset 0x%llX out of boundary [0x%llX-0x%llX]\n",
				offset,
				p_rw->boundary.top,
				p_rw->boundary.bottom);
			return -1;
		}

		int er = nand_writeblock(p_buf, blk_ofs);
		if ( er == -2) {
			// bad block
			continue;
		} else if (er < 0) {
			return er;
		}

		p_buf += p_ctrl->meminfo.erasesize;
		blk_ofs++;
		n--;
	}
	return 0;
}