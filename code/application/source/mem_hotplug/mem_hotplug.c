#include <stdio.h>
#include <stdlib.h>
#include <vendor_common.h>


void print_usage(void){

	printf("===============\n");
	printf("Usage:\n");
	printf("mem_hotplug [addr] [size]\n");
	printf("sample: mem_hotplug 0x1000 0x1000\n");

	return;
}


int main(int argc, char  **argv)
{
	unsigned long addr;
	unsigned long size;
	HD_RESULT ret=0;
	char **ptr =NULL;
	if(argc < 3){
		print_usage();
		return -1;
	}
	addr = strtoul(argv[1], ptr, 16);
	size = strtoul(argv[2], ptr, 16);

	VENDOR_LINUX_MEM_HOT_PLUG desc = {0};
	desc.start_addr = addr;
	desc.size = size;
	ret = vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_LINUX_HOT_PLUG, &desc);
	if(ret != 0){
		printf("nvtmem_hotplug_set fail ret:%d\n");
	}

    return 0;
}
