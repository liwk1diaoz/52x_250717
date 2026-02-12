#ifndef _EMMC_INT_H
#define _EMMC_INT_H

#include <sys/types.h>

typedef struct _EMMC_BOUNDARY{
	loff_t top;
	loff_t bottom;
} EMMC_BOUNDARY;

typedef struct _EMMC_RW {
	unsigned char *p_buf;
	unsigned int blk_ofs;
	unsigned int blk_num;
	EMMC_BOUNDARY boundary;
} EMMC_RW;

int emmc_open(void);
int emmc_close(void);
ER emmc_lock(void);
ER emmc_unlock(void);
int emmc_get_block_size(void);
loff_t emmc_get_phy_size(void);
int emmc_read(EMMC_RW *p_rw);
int emmc_write(EMMC_RW *p_rw);

#endif