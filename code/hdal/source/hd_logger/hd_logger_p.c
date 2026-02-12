/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_logger_p.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_type.h"
#include "hd_logger.h"
#include "hd_logger_p.h"
#include "comm/log.h"
#include <stdarg.h>
#include <string.h>

#if !defined(__FREERTOS)
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <pthread.h>
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

#if defined(__FREERTOS)
#define LOG_IOCTL log_ioctl
#else
#define LOG_IOCTL ioctl
#endif

#define HD_MODULE_NAME HD_DEBUG
#include "../hd_int.h"

/* gmlib_proc_bind, need to sync the value in vg_log.c */
#define SETTING_MSG_SIZE    (2 * 1024)
#define FLOW_MSG_SIZE       (1 * 1024)
#define ERR_MSG_SIZE        (1 * 1024)
#define HDAL_SETTING_MSG_SIZE (2 * 1024)
#define HDAL_FLOW_MSG_SIZE (128 * 1024)
#define MSG_LENGTH_SIZE     8
#define MSG_OFFSET_SIZE     8
#define BUFFER_SIZE         (SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE + HDAL_SETTING_MSG_SIZE + HDAL_FLOW_MSG_SIZE)

#define HLOG_FOURCC MAKEFOURCC('H','L','O','G')

#define PROC_MSG_SIZE      4096
char *buffer_mapping = 0;
int log_fd = 0;
char *proc_msg = 0;

static unsigned int m_cfg_cpu = (unsigned int)-1;
static pthread_mutex_t m_mutex_log = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *mp_mutex_log = NULL; // from share memory or m_mutex_log

void hdal_flow_log_p(const char *fmt, ...)
{
	va_list ap;
	unsigned int *msg_len, *msg_offset;
	int len = 0;
	unsigned int msg_size = HDAL_FLOW_MSG_SIZE-(MSG_LENGTH_SIZE+MSG_OFFSET_SIZE);
	char *msg, *start_addr;

	if (buffer_mapping == 0 || mp_mutex_log == NULL) {
		return;
	}

	pthread_mutex_lock(mp_mutex_log);

	memset(proc_msg, 0x0, PROC_MSG_SIZE);
	va_start(ap, fmt);
	len = vsnprintf(proc_msg + len, PROC_MSG_SIZE, fmt, ap);
	va_end(ap);

	if (len < 0) {
		DBG_ERR("hdal_flow_log_p got too long msg\n");
		pthread_mutex_unlock(mp_mutex_log);
		return;
	}

	start_addr = buffer_mapping + SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE + HDAL_SETTING_MSG_SIZE;
	msg_len = (unsigned int *)(start_addr);
	msg_offset = (unsigned int *)(start_addr + MSG_LENGTH_SIZE);
	msg = start_addr + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE;

	if (*msg_offset == 0) {
		if ((*msg_len + (unsigned int)len + 1) < msg_size) { // +1 for end of '\0'
			//*msg_len += sprintf(msg + *msg_len, "%s", proc_msg);
			memcpy(msg + *msg_len, proc_msg, len+1); //with end of'\0'
			*msg_len += (unsigned int)len;
		} else {
			//*msg_offset = sprintf(msg, "%s", proc_msg);
			memcpy(msg, proc_msg, len+1);
			*msg_offset = (unsigned int)len;
		}
	} else {
		if ((*msg_offset + len + 1) < msg_size) {
			//*msg_offset += sprintf(msg + *msg_offset, "%s", proc_msg);
			memcpy(msg + *msg_offset, proc_msg, len+1); //with end of'\0'
			*msg_offset += (unsigned int)len;
		} else {
			*msg_len = *msg_offset;
			//*msg_offset = sprintf(msg, "%s", proc_msg);
			memcpy(msg, proc_msg, len+1);
			*msg_offset = (unsigned int)len;
		}
	}

	pthread_mutex_unlock(mp_mutex_log);

	return;
}

void vprintm_p(const char *fmt, va_list ap)
{
	char buf[256] = {0};
	vsnprintf(buf, sizeof(buf)-1, fmt, ap);
	if (LOG_IOCTL(log_fd, IOCTL_PRINTM, (void *)buf) < 0) {
		DBG_ERR("IOCTL_PRINTM failed.\n");
	}
}

void gmlib_proc_init(void)
{
#if !defined(__FREERTOS)
	if (log_fd == 0) {
		log_fd = open("/dev/log_vg", O_RDWR);
	}
	if (!log_fd) {
		DBG_ERR("Error to open /dev/log_vg\n");
	}

	if (!buffer_mapping) {
		buffer_mapping = (char *)mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, log_fd, 0);
	}
	if (!buffer_mapping) {
		DBG_ERR("Error to mmap /dev/log_vg\n");
	}
#else
	if (!buffer_mapping) {
		buffer_mapping = (char *)log_mmap(0, 0, 0, 0, log_fd, 0);
		if (buffer_mapping == NULL) {
			DBG_ERR("Error to log_mmap.\n");
		}
	}
#endif
	if ((proc_msg = (char *)malloc(PROC_MSG_SIZE)) == NULL) {
		DBG_ERR("Error to allcate gmlib proc message buffer.\n");
		return;
	}
}

void hd_logger_init_p(unsigned int cpu_cfg)
{
	m_cfg_cpu = cpu_cfg;
	mp_mutex_log = &m_mutex_log; //default
#if !defined(__FREERTOS)
	// for linux, we always use shared mutex
	int shmid;
	if (m_cfg_cpu == 0) {
		// parent
		pthread_mutexattr_t mat;
		if ((shmid = shmget(HLOG_FOURCC, sizeof(pthread_mutex_t), 0666 | IPC_CREAT)) < 0) {
			perror("hd_logger_init_p: shmget");
			mp_mutex_log = NULL;
			return;
		}
		mp_mutex_log = (pthread_mutex_t *)shmat(shmid, NULL, 0);
		if (mp_mutex_log == NULL) {
			perror("hd_logger_init_p: shmat");
			return;
		}
		pthread_mutexattr_init(&mat);
		if(pthread_mutexattr_setpshared(&mat, PTHREAD_PROCESS_SHARED) != 0){
			perror("hd_logger_init_p: pthread_mutexattr_setpshared");
			return;
		}
		pthread_mutex_init(mp_mutex_log, &mat);
	} else {
		// child
		if ((shmid = shmget(HLOG_FOURCC, sizeof(pthread_mutex_t), 0666)) < 0) {
			perror("hd_logger_init_p(child): shmget");
			mp_mutex_log = NULL;
			return;
		}
		mp_mutex_log = (pthread_mutex_t *)shmat(shmid, NULL, 0);
		if (mp_mutex_log == NULL) {
			perror("hd_logger_init_p: shmat");
			return;
		}
	}
#endif
	gmlib_proc_init();
}

void gmlib_proc_close(void)
{
	if (log_fd) {
		close(log_fd);
		log_fd = 0;
	}
	if (buffer_mapping) {
#if !defined(__FREERTOS)
		munmap(buffer_mapping, BUFFER_SIZE);
#endif
		buffer_mapping = 0;
	}
	if (proc_msg) {
		free(proc_msg);
		proc_msg = 0;
	}
}

void hd_logger_uninit_p(void)
{
	gmlib_proc_close();
}
