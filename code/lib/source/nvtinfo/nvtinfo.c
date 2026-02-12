/**
    nvtinfo related handling functions.

    To support boot time measurement/timer count get/memory hotplug.
    @file nvtinfo.c
    @ingroup
    @note Nothing (or anything need to be mentioned).
    Copyright Novatek Microelectronics Corp. 2017. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/kernel.h>       /* for struct sysinfo */
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include "nvtinfo.h"

static void sysfs_write(char *path, char *s)
{
	char buf[80];
	int len;
	int fd = open(path, O_WRONLY);

	if (fd < 0) {
		strerror_r(errno, buf, sizeof(buf));
		return;
	}
	len = write(fd, s, strlen(s) + 1);
	if (len < 0) {
		strerror_r(errno, buf, sizeof(buf));
	}

	close(fd);
}

static ssize_t sysfs_read(char *path, char *s, int num_bytes)
{
	char buf[80];
	ssize_t count = 0;
	int fd = open(path, O_RDONLY);

	if (fd < 0) {
		strerror_r(errno, buf, sizeof(buf));
		return -1;
	}

	if ((count = read(fd, s, (num_bytes -1))) < 0) {
		strerror_r(errno, buf, sizeof(buf));
	} else {
		if ((count >= 1) && (s[count-1] == '\n')) {
			s[count-1] = '\0';
		} else {
			s[count] = '\0';
		}
	}

	close(fd);

	return count;
}

static long get_uptime(void)
{
	struct sysinfo s_info;
	int error = sysinfo(&s_info);
	if(error != 0)
		printf("Get /proc/uptime error with %d\n", error);

	return s_info.uptime;
}

int nvtbootts_set(char *s, bool boot_only)
{
	if (boot_only) {
		if (get_uptime() < 15)
			sysfs_write("/proc/nvt_info/bootts", s);
	} else {
		sysfs_write("/proc/nvt_info/bootts", s);
	}

	return 0;
}

int nvt_timer_tm0_get(void)
{
	int count = 0;
	char buf[15] = {0};
	count = sysfs_read("/proc/nvt_info/tm0", buf, 15);
	if (count < 0)
		return -1;

	return atoi(buf);
}
