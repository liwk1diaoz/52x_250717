#ifndef _SDIO_INT_H
#define _SDIO_INT_H

#include <sys/types.h>

typedef struct _SDIO_BOUNDARY{
	loff_t top;
	loff_t bottom;
} SDIO_BOUNDARY;

typedef struct _SDIO_RW {
	unsigned char *p_buf;
	unsigned int blk_ofs;
	unsigned int blk_num;
	SDIO_BOUNDARY boundary;
} SDIO_RW;

int sdio_open(void);
int sdio_close(void);
ER sdio_lock(void);
ER sdio_unlock(void);
int sdio_get_block_size(void);
loff_t sdio_get_phy_size(void);
int sdio_read(SDIO_RW *p_rw);
int sdio_write(SDIO_RW *p_rw);

#endif