#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <nvt_type.h>
#include <sys/mman.h>

#define CHKPNT    printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__) ///< Show a color sting of line count and function name in your insert codes
#define DBG_WRN(fmtstr, args...) printf("\033[33m%s(): \033[0m" fmtstr,__func__, ##args)
#define DBG_ERR(fmtstr, args...) printf("\033[31m%s(): \033[0m" fmtstr,__func__, ##args)

#if 1
#define DBG_IND(fmtstr, args...)
#else
#define DBG_IND(fmtstr, args...) printf("%s(): " fmtstr, \
    __func__, ##args)
#endif

#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)



static int fd = 0;
CHAR    str_dumpmem[64];

void* mem_mmap(int fd, unsigned int mapped_size, off_t phy_addr)
{
	void *map_base = NULL;
	unsigned page_size = 0;

	page_size = getpagesize();
	map_base = mmap(NULL,
			mapped_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			phy_addr & ~(off_t)(page_size - 1));

	if (map_base == MAP_FAILED)
		return NULL;

	return map_base;
}

int mem_munmap(void* map_base, unsigned int mapped_size)
{
	if (munmap(map_base, mapped_size) == -1)
		return -1;

	return 0;
}

void debug_dumpmem(UINT32 vir_addr, UINT32 phy_addr, UINT32 length)
{
	UINT32  i,j,k,str_len, val;
	CHAR    out_ch;
	UINT32	out_ch_arr[4];

	DBG_DUMP("dump phy_addr=%08x , vir_addr=%08x, length=%08x to console:\r\n", phy_addr, vir_addr, length);

	for (i=0;i<length;)
	{
		str_len=snprintf(str_dumpmem, 64, "%08lX : %08lX %08lX %08lX %08lX  ", phy_addr+i, *((UINT32 *)vir_addr),
				*((UINT32 *)(vir_addr+4)),*((UINT32 *)(vir_addr+8)), *((UINT32 *)(vir_addr+12)));
		#if 1
		for (j=0;j<16;j+=4)
		{
			val = *(UINT32 *)(vir_addr+j);
			out_ch_arr[3] = (val & 0xff000000) >> 24;
			out_ch_arr[2] = (val & 0xff0000) >> 16;
			out_ch_arr[1] = (val & 0xff00) >> 8;
			out_ch_arr[0] = (val & 0xff);
			k = 0;
			while (k < 4)
			{
				#if 1
				out_ch = (char)out_ch_arr[k];
				if (((UINT32)out_ch<0x20) || ((UINT32)out_ch>=0x80))
					str_len+=snprintf(str_dumpmem+str_len, 64-str_len, ".");
				else
					str_len+=snprintf(str_dumpmem+str_len, 64-str_len, "%c",out_ch);

				#endif
				k++;
			}

		}
		#else
		for (j=0;j<16;j++)
                {
                       out_ch = *((char *)(vir_addr+j));
                       if (((UINT32)out_ch<0x20) || ((UINT32)out_ch>=0x80))
                               str_len+=snprintf(str_dumpmem+str_len, 64-str_len, ".");
                       else
                               str_len+=snprintf(str_dumpmem+str_len, 64-str_len, "%c",out_ch);
		}
		#endif
		DBG_DUMP("%s", str_dumpmem);
		DBG_DUMP("\r\n");
		i+=16;
		vir_addr+=16;
	}

	DBG_DUMP("\r\n\r\n");
}

void debug_dumpmem2file(UINT32 addr, UINT32 length, CHAR* filename)
{
	int fd;
	int flags = O_RDWR|O_CREAT|O_TRUNC;
	fd = open(filename, flags, 0777);
	if (fd < 0)
	{
		DBG_ERR("create file %s fail errno=%d\r\n",filename,errno);
		return;
	}
	if (length == 0) {
		close(fd);
		return;
	}

	if(write(fd,(void*)addr,length) < 0){
		DBG_ERR("fail to write %d bytes\r\n", length);
	}
	close(fd);
}

void cmd_mem_w(UINT32  addr,UINT32 data)
{
	void*  map_addr;
	UINT32 virtual_addr, length = 0x100;
	UINT32 page_align_addr, map_size, map_offset;

	addr &= 0xFFFFFFFC;
	page_align_addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = addr - page_align_addr;
	map_size = map_offset + length;

	map_addr = mem_mmap(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("mmap error for addr 0x%x\r\n", addr);
		return;
	}
	virtual_addr = (UINT32)(map_addr + map_offset);
	*((UINT32*) virtual_addr)=data;
	DBG_DUMP("addr = 0x%08X, data = 0x%08X\r\n", addr, data);
	mem_munmap(map_addr, map_size);
}

void cmd_bit_w(UINT32  addr,UINT32 bit_num, UINT32 val)
{
	void*  map_addr;
	UINT32 virtual_addr, length = 0x100;
	UINT32 page_align_addr, map_size, map_offset;

	addr &= 0xFFFFFFFC;
	page_align_addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = addr - page_align_addr;
	map_size = map_offset + length;

	map_addr = mem_mmap(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("mmap error for addr 0x%x\r\n", addr);
		return;
	}
	virtual_addr = (UINT32)(map_addr + map_offset);
	DBG_DUMP("\r\taddr = 0x%08X, data = 0x%08X\r\n", addr, *((UINT32*) virtual_addr));
	*((UINT32*) virtual_addr)&= ~(1<<bit_num);
	*((UINT32*) virtual_addr)|= (val<<bit_num);
	if (val == 0)
		DBG_DUMP("\rAfter clear Bit#%u:\n", bit_num);
	else
		DBG_DUMP("\rAfter set Bit#%u:\n", bit_num);
		
	DBG_DUMP("\r\taddr = 0x%08X, data = 0x%08X\r\n", addr, *((UINT32*) virtual_addr));
	mem_munmap(map_addr, map_size);
}

void cmd_mem_r(UINT32  addr,UINT32 length)
{
	UINT32 virtual_addr;
	void*  map_addr;
	UINT32 page_align_addr, map_size, map_offset;

	addr &= 0xFFFFFFFC;
	page_align_addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = addr - page_align_addr;
	map_size = map_offset + length;

	map_addr = mem_mmap(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("mmap error for addr 0x%x\r\n", addr);
		return;
	}
	DBG_DUMP("map_addr = 0x%08X, map_size = 0x%x\r\n", map_addr, map_size);
	virtual_addr = (UINT32)(map_addr + map_offset);
	debug_dumpmem(virtual_addr, addr, length);
	mem_munmap(map_addr, map_size);
}
void cmd_mem_fill(UINT32  addr, UINT32 length, UINT32 data)
{
	UINT32 virtual_addr;
	void*  map_addr;
	UINT32 page_align_addr, map_size, map_offset;

	addr &= 0xFFFFFFFC;
	page_align_addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = addr - page_align_addr;
	map_size = map_offset + length;

	map_addr = mem_mmap(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("mmap error for addr 0x%x\r\n", addr);
		return;
	}
	virtual_addr = (UINT32)(map_addr + map_offset);
	DBG_DUMP("addr = 0x%08X, length = 0x%08X, data = 0x%02X\r\n", addr, length, data);
	memset((void*)virtual_addr, (int)data, (size_t)length);
	mem_munmap(map_addr, map_size);

}

int cmd_mem_fillfile(UINT32  addr, UINT32 length, char* filename)
{
	UINT32 virtual_addr;
	void*  map_addr;
	UINT32 page_align_addr, map_size, map_offset;
	size_t size = 0;
	FILE *file = NULL;
	struct stat st;
	int ret = 0;

	file = fopen(filename, "rb");
	if (!file) {
		DBG_ERR("create file %s fail errno=%d\r\n", filename, errno);
		return -1;
	}
	ret = stat(filename, &st);
	if (ret < 0) {
		fclose(file);
		return -1;
	} else {
		if (length == 0)
			length = st.st_size;
	}
	printf("filename %s size=%u\n", filename, length);
	addr &= 0xFFFFFFFC;
	page_align_addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = addr - page_align_addr;
	map_size = map_offset + length;
	map_addr = mem_mmap(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("mmap error for addr 0x%x\r\n", addr);
		fclose(file);
		return -1;
	}
	virtual_addr = (UINT32)(map_addr + map_offset);
	size = fread((void*)virtual_addr, 1, length, file);
	if (size != length) {
		printf("error read with size %u\n", size);
	}
	fclose(file);
	mem_munmap(map_addr, map_size);
	return 0;
}

void cmd_mem_dump(UINT32 addr, UINT32 length, CHAR* filename)
{
	UINT32 virtual_addr;
	void*  map_addr;
	UINT32 page_align_addr, map_size, map_offset;

	addr &= 0xFFFFFFFC;
	if (length == 0) {
		return;
	}
	DBG_DUMP("dump addr = 0x%08X, length = 0x%08X, filename = %s\r\n", addr, length, filename);
	page_align_addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = addr - page_align_addr;
	map_size = map_offset + length;

	map_addr = mem_mmap(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("mmap error for addr 0x%x\r\n", addr);
		return;
	}
	virtual_addr = (UINT32)(map_addr + map_offset);
	debug_dumpmem2file(virtual_addr, length, filename);
	mem_munmap(map_addr, map_size);
}

void show_help(void)
{
	DBG_DUMP("Help:\r\n");
	DBG_DUMP("\tw    [addr] [data]                  Write a word into memory\r\n");
	DBG_DUMP("\tr    [addr] (length)                Read a region of memory\r\n");
	DBG_DUMP("\tfill [addr] [length] (data)         Fill a region of memory with date value\r\n");
	DBG_DUMP("\tfillfile [addr] [length] (filename) Read file data and fill them into memory\r\n");
	DBG_DUMP("\tdump [addr] (length) (filename)     Dump a region of memory to a file\r\n");
	DBG_DUMP("\tbit [addr] (bit Num)(val)           To do the specific address bit operation with value 0(clear) or 1(set)\r\n");
	DBG_DUMP("\te.g.\r\n");
	DBG_DUMP("\t\tmem r 0x2000 0x200\r\n");
	DBG_DUMP("\t\tmem w 0x2000 0x2c00\r\n");
	DBG_DUMP("\t\tmem fillfile 0x17000000 0 image.bin  --- Read image.bin into memory address 0x17000000\r\n");
	DBG_DUMP("\t\tmem bit 0x17000000 20 1              --- To set(1)/clear(0) bit20\r\n");
}

int init_mem_dev(void)
{
	int fd = 0;
	fd = open("/dev/mem", O_RDWR | O_SYNC);
	return fd;
}


void uninit_mem_dev(int fd)
{
	close(fd);
	return;
}

int main(int argc, char *argv[])
{
	int ret = 0, i = 1;

	DBG_IND("argc=%d\r\n",argc);

	fd = init_mem_dev();
	if (fd < 0) {
		printf("Open /dev/mem device failed\n");
		return -1;
	}

	if (argc <= 1) {
		show_help();
	} else {
		while (i < argc) {
			if (!strcmp(argv[i], "?")) {
				show_help();
				break;
			}
			if (!strcmp(argv[i], "r")) {
				UINT32  addr = 0, length = 256;
				if (argc<3)
				{
					DBG_DUMP("syntex: mem r [addr] (length)\r\n");
					break;
				}
				i++;
				if (i < argc)
					sscanf(argv[i++], "%lx", &addr);
				if (i < argc)
					sscanf(argv[i++], "%lx", &length);
				cmd_mem_r(addr,length);
				break;
			}
			if (!strcmp(argv[i], "w")) {
				UINT32  addr = 0, data = 0;

				if (argc!=4)
				{
					DBG_DUMP("syntex: mem w [addr] [data]\r\n");
					break;
				}
				i++;
				if (i < argc)
					sscanf(argv[i++], "%lx", &addr);
				if (i < argc)
					sscanf(argv[i++], "%lx", &data);
				cmd_mem_w(addr,data);
				break;
			}
			if (!strcmp(argv[i], "fill")) {
				UINT32  addr = 0, length = 0, data = 0;

				if (argc<4)
				{
					DBG_DUMP("syntex: mem fill [addr] [length] (data)\r\n");
					break;
				}
				i++;
				if (i < argc)
					sscanf(argv[i++], "%lx", &addr);
				if (i < argc)
					sscanf(argv[i++], "%lx", &length);
				if (i < argc)
					sscanf(argv[i++], "%lx", &data);
				cmd_mem_fill(addr,length,data);
				break;
			}
			if (!strcmp(argv[i], "fillfile")) {
				UINT32  addr = 0, length = 0;
				CHAR    filename[256]="/mnt/sd/fill.bin";

				if (argc < 3)
				{
					DBG_DUMP("syntex: mem fillfile [addr] [length] (filename)\r\n");
					break;
				}
				i++;
				if (i < argc)
					sscanf(argv[i++], "%lx", &addr);
				if (i < argc)
					sscanf(argv[i++], "%lx", &length);
				if (i < argc)
					sscanf(argv[i++], "%s", filename);
				ret = cmd_mem_fillfile(addr,length,filename);
				if (ret < 0)
					return ret;
				break;
			}
			if (!strcmp(argv[i], "dump")) {
				UINT32  addr = 0, length = 256;
				CHAR    filename[256]="/mnt/sd/dump.bin";

				if (argc < 3)
				{
					DBG_DUMP("syntex: mem dump [addr] (length) (filename)\r\n");
					break;
				}
				i++;
				if (i < argc)
					sscanf(argv[i++], "%lx", &addr);
				if (i < argc)
					sscanf(argv[i++], "%lx", &length);
				if (i < argc)
					sscanf(argv[i++], "%s", filename);
				if (length == 0)
					return -1;
				cmd_mem_dump(addr,length,filename);
				break;
			}
			if (!strcmp(argv[i], "bit")) {
				UINT32  addr = 0, bit_num = 0, val = 0;

				if (argc!=5)
				{
					DBG_DUMP("syntex: mem bit [addr] [bit#num] [1/0] \r\n");
					break;
				}
				i++;
				if (i < argc)
					sscanf(argv[i++], "%lx", &addr);
				if (i < argc)
					sscanf(argv[i++], "%lu", &bit_num);
				if (i < argc)
					sscanf(argv[i++], "%lu", &val);
				if (val > 1) {
					DBG_DUMP("syntex: mem bit [addr] [bit#num] [1/0] \r\n");
					break;
				}
				cmd_bit_w(addr,bit_num, val);
				break;
			}
			show_help();
			break;
		}
	}

	uninit_mem_dev(fd);
	return 0;
}


