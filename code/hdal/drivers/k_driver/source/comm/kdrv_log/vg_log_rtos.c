#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
#include <kwrap/cmdsys.h>
#include <comm/hwclock.h>
#include <comm/log.h>
#include "vg_log_core.h"

#ifndef EFAULT
#define EFAULT 14
#endif

int log_init(void *p_param)
{
	log_init_mem();
	return 0;
}

int log_exit(void *p_param)
{
	log_uninit_mem();
	return 0;
}

void *log_mmap(void *p_addr, unsigned int len, int prot, int flags, int fd, unsigned int offset)
{
	return mmap_msg;
}

int log_munmap(void *p_addr, unsigned int len)
{
	return 0;
}

int log_ioctl(int fd, unsigned int cmd, void *p_arg)
{
	char log_string[MAX_STRING_LEN];

	switch (cmd) {
	case IOCTL_PRINTM:
		memset(log_string, 0, sizeof(log_string));
		memcpy((void *)&log_string, (void *)p_arg, sizeof(char) * MAX_STRING_LEN);
		printm("UR", "%s", log_string);
		break;
	case IOCTL_SET_HDAL_VERSION:
		memset(&hdal_version, 0, sizeof(unsigned int));
		memcpy((void *)&hdal_version, (void *)p_arg, sizeof(unsigned int));
		break;
	case IOCTL_SET_IMPL_VERSION:
		memset(&impl_version, 0, sizeof(unsigned int));
		memcpy((void *)&impl_version, (void *)p_arg, sizeof(unsigned int));
		break;
	default:
		return -EFAULT;
	}

	return 0;
}

static int cmd_help(int argc, char *argv[])
{
	printf("usage:\n");
	printf("    hdal version\n");
	printf("    hdal flow\n");
	printf("    hdal log_dump\n");
	printf("    hdal log_mode 0/1/2 (0:storage  1:dump console  2:direct print)\n");
	return 0;
}

static int cmd_version(int argc, char *argv[])
{
	printf("HDAL: Version: v%x.%x.%x\n",
		(hdal_version & 0xF00000) >> 20,
		(hdal_version & 0x0FFFF0) >> 4,
		(hdal_version & 0x00000F));
	printf("HDAL: IMPL Version: v%x.%x.%x\n",
		(impl_version & 0xF00000) >> 20,
		(impl_version & 0x0FFFF0) >> 4,
		(impl_version & 0x00000F));
	return 0;
}

static int cmd_flow(int argc, char *argv[])
{
	char *hdal_msg, *start_addr;
	unsigned int hdal_offset;

	start_addr = (char *)(mmap_msg + SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE + HDAL_SETTING_MSG_SIZE);
	hdal_offset = *(unsigned int *)(start_addr + MSG_LENGTH_SIZE);
	hdal_msg = start_addr + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE;
	printf("hdal_msg=%08X\n", (unsigned int)hdal_msg);
	if (hdal_offset == 0) {
		printf(hdal_msg);
	} else {
		printf((char *)(hdal_msg + hdal_offset));
		printf((char *)(hdal_msg));
	}
	return 0;
}

static int cmd_log_dump(int argc, char *argv[])
{
	unsigned int start, size;
	calculate_log(&start, &size);
	start = log_base_start;
	prepare_dump_console(start, size);
	return 0;
}

static int cmd_log_mode(int argc, char *argv[])
{
	if (argc < 3) {
		printf("debug mode %d (0:storage  1:dump console  2:direct print)\n", mode);
	} else {
		int mode_set = atoi(argv[2]);
		if (mode_set > 0 && mode_set < 3) {
			mode = mode_set;
			printf("set to mode %d (-1:disable 0:storage  1:dump console  2:direct print(forever)\n", mode_set);
		} else {
			printf("invalid mode number (%d)\n", mode_set);
		}
	}
	return 0;
}

MAINFUNC_ENTRY(hdal, argc, argv)
{
	if (argc < 2) {
		cmd_help(argc, argv);
		return 0;
	}

	if (strncmp(argv[1], "version", strlen("version")) == 0) {
		cmd_version(argc, argv);
	} else if (strncmp(argv[1], "flow", strlen("flow")) == 0) {
		cmd_flow(argc, argv);
	} else if (strncmp(argv[1], "log_dump", strlen("log_dump")) == 0) {
		cmd_log_dump(argc, argv);
	} else if (strncmp(argv[1], "log_mode", strlen("log_mode")) == 0) {
		cmd_log_mode(argc, argv);
	} else {
		cmd_help(argc, argv);
	}
	return 0;
}
