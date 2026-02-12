//#include "stdafx.h"
#include "../include/msdcnvt.h"
#include "msdcnvt_int.h"
#include "msdcnvt_ipc.h"
#include "msdcnvt_uitron.h"

#if defined(WIN32)
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys\timeb.h>
#elif defined(__LINUX)
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#if (CFG_VIA_IPC)
#include <nvtipc.h>
#elif (CFG_VIA_IOCTL)
#include "msdcnvt_ioctl.h"
#endif
#if defined(__LINUX660)
#include <nvtmem.h>
#include <protected/nvtmem_protected.h>
#endif
#elif defined(__ECOS)
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <cyg/nvtipc/NvtIpcAPI.h>
#include <cyg/hal/Hal_cache.h>
#include <cyg/compat/uitron/uit_func.h>
#include <cyg/infra/diag.h>
#endif

//must be the same with driver
#define CFG_NVTMSDC_SIZE_IPC_DATA (sizeof(MSDCNVT_IPC_CFG))
#define CFG_MIN_HOST_MEM_SIZE 0x10000

static MSDCNVT_IPC_CFG *m_p_ipc_cfg = NULL;

#if (CFG_VIA_IPC)
static SEM_HANDLE m_sem_ipc;
static int m_ipc_msgid = -1;
#elif (CFG_VIA_IOCTL)
static int m_fd = -1;
static unsigned char *m_shm_begin = NULL;
static unsigned char *m_shm_end = NULL;
#endif

// only linux needs to handle s3 suspend/resume
#if defined(__LINUX) && (CFG_VIA_IPC)
static THREAD_HANDLE m_thread;
THREAD_DECLARE(thread_s3, lp_param)
{
	NVTIPC_ER er;
	int ipc_sig_ret;
	//coverity[no_escape]
	while (1) {
		ipc_sig_ret = NvtIPC_SigWait();
		switch (ipc_sig_ret) {
		case NVTIPC_SIG_SUSPEND:
			SEM_WAIT(m_sem_ipc);
			er = NvtIPC_MsgRel(m_ipc_msgid);
			if (er < 0) {
				DBG_ERR("s3 failed to NvtIPC_MsgRel: %d\r\n", er);
			}
			m_ipc_msgid = -1;
			break;
		case NVTIPC_SIG_RESUME:
			m_ipc_msgid = NvtIPC_MsgGet(NvtIPC_Ftok("MSDCNVT"));
			if (m_ipc_msgid < 0) {
				DBG_ERR("failed to NvtIPC_MsgGet: %d\r\n", m_ipc_msgid);
			}
			SEM_SIGNAL(m_sem_ipc);
			break;
		default:
			DBG_WRN("not handled ipc signal: %d\r\n", ipc_sig_ret);
			break;
		}
		if (ipc_sig_ret >= 0) {
			NvtIPC_SigAck(ipc_sig_ret);
		}
	}
	THREAD_EXIT();
}
#endif

unsigned short memcheck_calc_checksum_16bit(unsigned short *p, unsigned int len)
{
	unsigned int i, sum = 0;

	for (i = 0; i < len; i++) {
		sum += (*(p + i) + i);
	}

	sum &= 0x0000FFFF;

	return (unsigned short)sum;
}

int msdcnvt_uitron_init(MSDCNVT_IPC_CFG *p_ipc_cfg)
{
#if (CFG_VIA_IPC)
	m_p_ipc_cfg = p_ipc_cfg;
	m_ipc_msgid = NvtIPC_MsgGet(NvtIPC_Ftok("MSDCNVT"));
	if (m_ipc_msgid < 0) {
		DBG_ERR("failed to NvtIPC_MsgGet: %d\r\n", m_ipc_msgid);
	}
	SEM_CREATE(m_sem_ipc, 1);
#if defined(__LINUX)
	if (THREAD_CREATE("MSDCNVT_S3", m_thread, thread_s3, NULL, NULL, 0, NULL) != 0) {
		DBG_ERR("msdcnvt: s3 pthread create failed.\r\n");
	}
#endif
#endif
	return 0;
}

MSDCNVT_IPC_CFG *msdcnvt_uitron_init2(void)
{
#if (CFG_VIA_IOCTL)
	m_fd = open("/dev/msdcnvt0", O_RDWR);
	if (m_fd < 0) {
		DBG_ERR("failed to open /dev/msdcnvt\r\n");
		return NULL;
	}
	m_p_ipc_cfg = (MSDCNVT_IPC_CFG *)mmap(NULL, CFG_NVTMSDC_SIZE_IPC_DATA + CFG_MIN_HOST_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, m_fd, 00);
	m_shm_begin = (unsigned char *)m_p_ipc_cfg;
	m_shm_end = m_shm_begin + CFG_NVTMSDC_SIZE_IPC_DATA + CFG_MIN_HOST_MEM_SIZE;
	return m_p_ipc_cfg;
#else
	return NULL;
#endif

}

int msdcnvt_uitron_exit(void)
{
#if (CFG_VIA_IPC)
	NVTIPC_ER er;

	er = NvtIPC_MsgRel(m_ipc_msgid);
	if (er < 0) {
		DBG_ERR("failed to NvtIPC_MsgRel: %d\r\n", er);
	}
#if defined(__LINUX)
	SEM_WAIT(m_sem_ipc);
	THREAD_DESTROY(m_thread);
	SEM_SIGNAL(m_sem_ipc);
#endif
	SEM_DESTROY(m_sem_ipc);
#elif (CFG_VIA_IOCTL)
	close(m_fd);
#endif

	return 0;
}

unsigned char *mem_phy_to_noncache(unsigned int addr)
{
#if defined(__LINUX660) || defined(__ECOS660)
	return (unsigned char *)dma_getNonCacheAddr(addr);
#elif defined(__LINUX680) || defined(__LINUX510)
	return (unsigned char *)NvtIPC_GetNonCacheAddr((unsigned int)addr);
#elif (CFG_VIA_IOCTL)
	//must be in range
	unsigned char *p = m_shm_begin + addr;

	if (addr == 0) {
		return 0;
	} else if (p < m_shm_begin || p > m_shm_end) {
		DBG_ERR("invalid mapped-addr: %08X, valid(%08X ~ %08X)\r\n",
				(unsigned int)p,
				(unsigned int)m_shm_begin,
				(unsigned int)m_shm_end);
		return 0;
	} else {
		return p;
	}
#else
	return (unsigned char *)addr;
#endif
}

unsigned int mem_to_phy(void *addr)
{
#if defined(__LINUX660) ||  defined(__ECOS660)
	return (unsigned int)dma_getPhyAddr((unsigned int)addr);
#elif defined(__LINUX680) || defined(__LINUX510)
	return (unsigned int)NvtIPC_GetPhyAddr((unsigned int)addr);
#elif (CFG_VIA_IOCTL)
	//must be in range
	unsigned char *p = (unsigned char *)addr;

	if (addr == NULL) {
		return 0;
	} else if (p < m_shm_begin || p > m_shm_end) {
		DBG_ERR("invalid addr: %08X, valid(%08X ~ %08X)\r\n",
				(unsigned int)p,
				(unsigned int)m_shm_begin,
				(unsigned int)m_shm_end);
		return 0;
	} else {
		return (unsigned int)(p - m_shm_begin);
	}
#else
	return (unsigned int)addr;
#endif
}

#if (CFG_VIA_IPC) || (CFG_VIA_IOCTL)
static int msdcnvt_uitron_copy_from(MSDCNVT_ICMD *p_cmd)
{
	unsigned char *p_desc = (unsigned char *)&m_p_ipc_cfg->cmd;
	unsigned char *p_input = (unsigned char *)m_p_ipc_cfg->cmd_data;
	unsigned char *p_output = p_input + ALIGN_CEIL(p_cmd->in_size, 32);

	memcpy(p_desc, p_cmd, sizeof(MSDCNVT_ICMD));
	if (p_cmd->in_addr != 0) {
		if (p_cmd->in_size != 0) {
			memcpy(p_input, (void *)p_cmd->in_addr, p_cmd->in_size);
			((MSDCNVT_ICMD *)p_desc)->in_addr = mem_to_phy(p_input);
			((MSDCNVT_ICMD *)p_desc)->out_addr = mem_to_phy(p_output);
		} else {
			DBG_ERR("p_cmd->in_size=0\r\n");
			return -1;
		}
	}

	if (p_cmd->out_addr != 0) {
		if (p_cmd->out_size != 0) {
			((MSDCNVT_ICMD *)p_desc)->out_addr = mem_to_phy(p_output);
		} else {
			printf("p_cmd->out_size=0\r\n");
			return -1;
		}
	}

	return 0;
}

static int msdcnvt_uitron_copy_to(MSDCNVT_ICMD *p_cmd)
{
	unsigned char *p_desc = (unsigned char *)&m_p_ipc_cfg->cmd;
	unsigned char *p_input = (unsigned char *)m_p_ipc_cfg->cmd_data;
	unsigned char *p_output = p_input + ALIGN_CEIL(p_cmd->in_size, 32);

	p_cmd->err = ((MSDCNVT_ICMD *)p_desc)->err;

	if (p_cmd->out_addr != 0) {
		if (p_cmd->err >= 0) {
			memcpy((void *)p_cmd->out_addr, p_output, p_cmd->out_size);
		}
	}

	return 0;
}
#endif

int msdcnvt_uitron_cmd(MSDCNVT_ICMD *p_cmd)
{
	unsigned req_size = sizeof(MSDCNVT_ICMD)
						+ ALIGN_CEIL(p_cmd->in_size, 32)
						+ ALIGN_CEIL(p_cmd->out_size, 32);

	MSDCNVT_ASSERT(sizeof(m_p_ipc_cfg->cmd_data) >= req_size);
	p_cmd->err = -1;

#if (CFG_VIA_IPC)
	int err;

	SEM_WAIT(m_sem_ipc);

	err = msdcnvt_uitron_copy_from(p_cmd);
	if (err != 0) {
		DBG_ERR("failed to copy from: %d.\r\n", err);
		SEM_SIGNAL(m_sem_ipc);
		return err;
	}

	MSDCNVT_IPC_MSG msg_snd = { 0 };
	MSDCNVT_IPC_MSG msg_rcv = { 0 };

	msg_snd.mtype = 1;
	msg_snd.api_addr = p_cmd->api_addr;
	if (NvtIPC_MsgSnd(m_ipc_msgid, NVTIPC_SENDTO_CORE1, &msg_snd, sizeof(msg_snd)) < 0) {
		DBG_ERR("err:msgsnd:%d\r\n", (int)p_cmd->api_addr);
	}
	if (NvtIPC_MsgRcv(m_ipc_msgid, &msg_rcv, sizeof(msg_rcv)) < 0) {
		DBG_ERR("err:msgrcv\r\n");
	}
	if (msg_rcv.api_addr != msg_snd.api_addr) {
		DBG_ERR("err:msg.uiIPC!=wait %d %d\r\n", (int)msg_snd.api_addr, (int)msg_rcv.api_addr);
	}
	err = msdcnvt_uitron_copy_to(p_cmd);
	if (err != 0) {
		DBG_ERR("failed to copy to: %d.\r\n", err);
		SEM_SIGNAL(m_sem_ipc);
		return err;
	}
	SEM_SIGNAL(m_sem_ipc);
#elif (CFG_VIA_IOCTL)
	int err;

	err = msdcnvt_uitron_copy_from(p_cmd);
	if (err != 0) {
		DBG_ERR("failed to copy from: %d.\r\n", err);
		return err;
	}

	err = ioctl(m_fd, MSDCNVT_IOC_CMD, NULL);
	if (err != 0) {
		DBG_ERR("ioctl failed, er=%d.\r\n", err);
		return err;
	}

	err = msdcnvt_uitron_copy_to(p_cmd);
	if (err != 0) {
		DBG_ERR("failed to copy to: %d.\r\n", err);
		return err;
	}

#else
	((MSDCNVT_IAPI)p_cmd->api_addr)(p_cmd);
#endif
	return 0;
}
