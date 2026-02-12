#ifndef _NAND_INT_H
#define _NAND_INT_H

#include <sys/types.h>

typedef struct _NAND_BOUNDARY{
	loff_t top;
	loff_t bottom;
} NAND_BOUNDARY;

typedef struct _NAND_RW {
	unsigned char *p_buf;
	unsigned int blk_ofs;
	unsigned int blk_num;
	NAND_BOUNDARY boundary;
} NAND_RW;

int nand_open(void);
int nand_close(void);
ER nand_lock(void);
ER nand_unlock(void);
int nand_get_block_size(void);
loff_t nand_get_phy_size(void);
int nand_read(NAND_RW *p_rw);
int nand_write(NAND_RW *p_rw);

#endif