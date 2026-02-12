#include <stdio.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>
#include <strg_def.h>
#include <nand.h>

#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	const char* pathname="/dev/mmcblk0";
	int fd=open(pathname,O_RDONLY);
	if (fd==-1) {
	printf("%s",strerror(errno));
	}

	uint64_t size;
	if (ioctl(fd,BLKGETSIZE64,&size)==-1) {
	printf("%s",strerror(errno));
	}
	printf("==>%llx\n", size);
	close(fd);
#if 0
	UINT32 blksize;
	unsigned long long partition_ofs = 0xC0000;
	unsigned long long partition_size = 0x200000;
	STORAGE_OBJ *pStrg = nand_getStorageObject(STRG_OBJ_FW_RSV1);
	if (pStrg == NULL) {
		DBG_ERR("pStrg is NULL\n");
		return -1;
	}
	pStrg->Open();
	pStrg->GetParam(STRG_GET_BEST_ACCESS_SIZE, (UINT32)&blksize, 0); //get block size
	if (blksize != 0x20000) {
		DBG_ERR("storage block size is not match. %X(driver) : %X(dts)\n", (int)blksize, (int)0x20000);
		return -1;
	}
	pStrg->SetParam(STRG_SET_MEMORY_REGION, 0, 0);
	pStrg->SetParam(STRG_SET_PARTITION_SECTORS, (UINT32)(partition_ofs / 0x20000), (UINT32)(partition_size / 0x20000));
	pStrg->Close();

	pStrg->Open();
	unsigned char buf[0x100000] = {0};

	FILE *fptr = fopen("/mnt/sd/xxx2.tmp", "wb");

	pStrg->RdSectors((INT8*)buf, 0, 2);
	fwrite(buf, 0x40000, 1, fptr);
	fclose(fptr);
#endif
	return 0;
}