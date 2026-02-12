#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/fb.h>
#include<sys/mman.h>
#include<sys/mman.h>
#include "drivers/soc/nvt/nvt_perf/nvt_perf.h"

#include <errno.h>

int misc_fd = 0;

int main(int argc, char* argv[])
{
	printf("Run nvt_perf....\n");

	misc_fd = open("/dev/nvt_perf", O_RDWR);
	if (!misc_fd)	{
		printf("Error: Cannot open nvt_perf...\n");
		exit(1) ;
	}



    if (ioctl(misc_fd, NVT_PERF_CACHE, 0)) {
        printf("Run cache test failed\n");
        exit(3);
    }

	close(misc_fd);

    return 0;
}
