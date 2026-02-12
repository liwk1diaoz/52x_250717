/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A WAV audio file source
// Implementation

#include "NvtMgr.hh"
#include "GroupsockHelper.hh"
#include "nvtlive555.h"
#include <stdio.h>
#if defined(__LINUX) || defined(__UBUNTU)
#include <sys/ipc.h>
#include <sys/msg.h>
#endif
#if defined(__LINUX) && !defined(_NO_NVT_IPC_)
#include <nvtipc.h>
#endif
#if defined(__ECOS)
#include <netinet/tcp.h>
#endif
#if defined(__ECOS2)
#include <../../ecos-nvtipc/include/cyg/nvtipc/NvtIpcAPI.h>
#endif

#define CFG_JOB_MAX_COUNT 32

static NvtMgr *g_pNvtMgr = NULL;
static NvtMgr::MSG_CMD g_MsgCmd = { 0 }; //must be a global to prevent NvtMgr destroyed

NvtMgr::NvtMgr()
	: m_SocketPush(0)
	, m_SocketPullServer(0)
	, m_SocketPullClient(0)
	, m_bSeekDirty(1)
	, m_msqid(0)
	, m_hCmd(0)
	, m_hCmdObj(0)
	, m_pCmdStack(NULL)
	, m_bCmdLoop(False)
	, m_pWatchVariable(NULL)
	, m_bShowEvent(False)
	, m_bShowLatencyV(False)
	, m_bShowLatencyA(False)
	, m_bSupportAudio(True)
{
	SEM_CREATE(m_sem_job, 1);
	//coverity[uninit_use_in_call]
	SEM_WAIT(m_sem_job);
	m_pJob = new JOB[CFG_JOB_MAX_COUNT];
	memset(m_pJob, 0, sizeof(JOB)*CFG_JOB_MAX_COUNT);
	for (int i = 0; i < CFG_JOB_MAX_COUNT-1; i++) {
		m_pJob[i].next_job = &m_pJob[i + 1];
	}
	m_pJob[CFG_JOB_MAX_COUNT-1].next_job = &m_pJob[0]; //last point to head
	m_pJobHead = &m_pJob[0];
	m_pJobTail = &m_pJob[0];
	SEM_SIGNAL(m_sem_job);

	SEM_CREATE(m_sem_global, 1);

	memset(&m_UserCfg, 0, sizeof(m_UserCfg));
	memset(&m_UserCb, 0, sizeof(m_UserCb));
	memset(&m_HdalCb, 0, sizeof(m_HdalCb));
	memset(&m_StrmCb, 0, sizeof(m_StrmCb));

	m_UserCfg.support_meta = 0;
	m_UserCfg.support_rtcp = 1;
	m_UserCfg.port = 554;
	m_UserCfg.snd_size = 1024 * 1024;
	m_UserCfg.tos = 0;
	m_UserCfg.max_clients = 3;

	xCalibratePtsOfs(&m_PtsOfs);
	//coverity[uninit_member]
}

NvtMgr::~NvtMgr()
{
	SEM_WAIT(m_sem_job);
	delete[] m_pJob;
	m_pJob = NULL;
	SEM_DESTROY(m_sem_job);

	SEM_WAIT(m_sem_global);
	SEM_DESTROY(m_sem_global);

	if (m_SocketPush > 0) {
		closeSocket(m_SocketPush);
	}
	if (m_SocketPullServer > 0) {
		closeSocket(m_SocketPullServer);
	}
	if (m_SocketPullClient > 0) {
		closeSocket(m_SocketPullClient);
	}
#if defined(__LINUX) || defined(__UBUNTU)
	if (m_hCmd) {
		m_bCmdLoop = False;
		// kill and clean private mail box for quit thread
		struct msqid_ds msgds = { 0 };
		if (m_msqid >= 0 && msgctl(m_msqid, IPC_RMID, &msgds) < 0) {
			fprintf(stderr, "live555: message queue could not be deleted.\n");
		}
		THREAD_JOIN(m_hCmd);
		THREAD_DESTROY(m_hCmd);
	}
	if (m_pCmdStack) {
		delete[] m_pCmdStack;
		m_pCmdStack = NULL;
	}
#endif
}

#if defined(__ECOS)
extern "C" unsigned long long HwClock_GetLongCounter(void);
#elif defined(__FREERTOS)
extern "C" unsigned long long hwclock_get_longcounter(void);
#elif defined(__LINUX520)
extern "C" unsigned long long hd_gettime_us(void);
#endif
void NvtMgr::xCalibratePtsOfs(struct timeval *pPtsOfs)
{
	gettimeofday(pPtsOfs, NULL);
#if !defined(_NO_NVT_IPC_) && (defined(__LINUX) || defined(__ECOS2))
	NVTIPC_SYS_MSG ipc_sys_msg;
	NVTIPC_SYS_LONG_COUNTER_MSG long_counter_msg;
	ipc_sys_msg.sysCmdID = NVTIPC_SYSCMD_GET_LONG_COUNTER_REQ;
	ipc_sys_msg.SenderCoreID = NVTIPC_SENDER_CORE2;
	ipc_sys_msg.DataAddr = (uint32_t)&long_counter_msg;
	ipc_sys_msg.DataSize = sizeof(long_counter_msg);
	/* * Send a message. */
	if (NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, NVTIPC_SENDTO_CORE1, &ipc_sys_msg, sizeof(ipc_sys_msg)) < 0) {
		printf("live555: NvtMgr(): failed to msgsnd");
	} else {
		pPtsOfs->tv_sec -= long_counter_msg.sec;
		pPtsOfs->tv_usec -= long_counter_msg.usec;
	}
#elif defined(__ECOS)
	unsigned long long tm64 = HwClock_GetLongCounter();
	pPtsOfs->tv_sec -= (long)(tm64 >> 32);
	pPtsOfs->tv_usec -= (long)(tm64 & 0xFFFFFFFF);
#elif defined(__FREERTOS)
	unsigned long long tm64 = hwclock_get_longcounter();
	pPtsOfs->tv_sec -= (long)(tm64 >> 32);
	pPtsOfs->tv_usec -= (long)(tm64 & 0xFFFFFFFF);
#elif defined(__LINUX520)
	unsigned long long tm64 = hd_gettime_us();
	pPtsOfs->tv_sec -= (long)(tm64 >> 32);
	pPtsOfs->tv_usec -= (long)(tm64 & 0xFFFFFFFF);
#endif
	if (pPtsOfs->tv_usec < 0) {
		pPtsOfs->tv_usec += 1000000;
		pPtsOfs->tv_sec--;
	}
}

u_int32_t NvtMgr::Get_Port()
{
	return m_UserCfg.port;
}

u_int32_t NvtMgr::Get_SendBufSize()
{
	UINT32 uiSendBufSize = m_UserCfg.snd_size;

	if (uiSendBufSize < 1024 * 1024) {
		uiSendBufSize = 1024 * 1024;
	}

	//printf("live555:SO_SNDBUF=%d\r\n",uiSendBufSize);

	return uiSendBufSize;
}

u_int32_t NvtMgr::Get_TypeOfService()
{
	return m_UserCfg.tos;
}

u_int32_t NvtMgr::Get_TCPTimeOutSec()
{
	return 8; //8 sec
}

u_int32_t NvtMgr::Get_MaxConnections()
{
	return m_UserCfg.max_clients;
}

int NvtMgr::Set_UserCb(NVTLIVE555_CB *p_UserCb)
{
	m_UserCb = *p_UserCb;

	if (m_UserCb.get_cfg != NULL) {
		m_UserCb.get_cfg(&m_UserCfg);
	}
	return 0;
}

int NvtMgr::Set_StrmCb(ISTRM_CB *pStrmCb)
{
	m_StrmCb = *pStrmCb;
	return 0;
}

int NvtMgr::Set_HdalCb(NVTLIVE555_HDAL_CB *pHdalCb)
{
	m_HdalCb = *pHdalCb;
	return 0;
}

ISTRM_CB *NvtMgr::Get_StrmCb()
{
	return &m_StrmCb;
}

NVTLIVE555_CB *NvtMgr::Get_UserCb()
{
	return &m_UserCb;
}

NVTLIVE555_HDAL_CB *NvtMgr::Get_HdalCb()
{
	return &m_HdalCb;
}

int NvtMgr::IsSupportRtcp()
{
	return m_UserCfg.support_rtcp;
}
int NvtMgr::IsSupportMeta()
{
	return m_UserCfg.support_meta;
}

int64_t NvtMgr::Get_NextPtsTimeDiff(struct timeval *p_timePTS, u_int32_t frame_time_us)
{
	struct timeval timeNow;
	struct timeval timeNext;

	gettimeofday(&timeNow, NULL);
	timeNext = *p_timePTS;
	unsigned uNextSeconds = timeNext.tv_usec + frame_time_us;
	timeNext.tv_sec += uNextSeconds / 1000000;
	timeNext.tv_usec = uNextSeconds % 1000000;

	int secsDiff = timeNext.tv_sec - timeNow.tv_sec;
	return (int64_t)secsDiff * 1000000 + (timeNext.tv_usec - timeNow.tv_usec);
}

void NvtMgr::Calc_TimeOffset(struct timeval *p_time, u_int32_t offset_time_us)
{
	unsigned uSeconds = p_time->tv_usec + offset_time_us;
	p_time->tv_sec += uSeconds / 1000000;
	p_time->tv_usec = uSeconds % 1000000;
}

void NvtMgr::Set_SeekDirty()
{
	m_bSeekDirty = 1;
}

void NvtMgr::SendEvent(LIVE555_SERVER_EVENT evt)
{
	if (m_bShowEvent) {
		printf("live555: event:%d\r\n", evt);
	}
	if (m_UserCb.notify_rtsp_event != NULL) {
		m_UserCb.notify_rtsp_event(evt);
	}
}

int NvtMgr::GlobalLock()
{
	SEM_WAIT(m_sem_global);
	return 0;
}

int NvtMgr::GlobalUnlock()
{
	SEM_SIGNAL(m_sem_global);
	return 0;
}

u_int32_t NvtMgr::Authenticate(const char *cHTTPHeader, const u_int32_t uiHTTPHeaderLen, char *cRetBuf, u_int32_t uiRetBufLen)
{
	return 200; //200 OK
}

u_int32_t NvtMgr::RequireKeyFrame(u_int32_t m_uiMediaSrcID)
{
	u_int32_t hr = 0;

	if (m_UserCb.require_key_frame != NULL) {
		hr = m_UserCb.require_key_frame(m_uiMediaSrcID);
	}
	return hr;
}

u_int32_t NvtMgr::GetNextMetaDataFrm(char *pTxt, u_int32_t uMaxTxtSize, int b_init)
{
	if (m_UserCb.get_meta != NULL) {
		return m_UserCb.get_meta(pTxt, uMaxTxtSize, b_init);
	}
	return -1;
}

int  NvtMgr::Trans_UrlToInfo(NVTLIVE555_URL_INFO *pInfo, const char *url)
{
	int hr = 0;
	memset(pInfo, 0, sizeof(NVTLIVE555_URL_INFO));
	if (url == NULL) {
		return -1;
	}
	if (m_UserCb.parse_url != NULL) {
		hr = m_UserCb.parse_url(url, pInfo);
	}
	return hr;
}

//Job Control
int NvtMgr::PushJob(TASK *pTask)
{
	SEM_WAIT(m_sem_job);
	if (m_pJobTail->state != JOB_STATE_EMPTY) {
		printf("live555: failed to PushJob, state = %d\r\n", m_pJobTail->state);
		SEM_SIGNAL(m_sem_job);
		return -1;
	}
	m_pJobTail->task = *pTask;
	m_pJobTail->state = JOB_STATE_TODO;
	m_pJobTail = (JOB *)m_pJobTail->next_job;
	if (m_SocketPush > 0) {
		if (send(m_SocketPush, (const char *)&pTask->client_data, sizeof(pTask->client_data), 0) != sizeof(pTask->client_data)) {
			printf("live555: PushJob failed to send.\r\n");
		}
	}
	SEM_SIGNAL(m_sem_job);
	return 0;
}

int NvtMgr::PullJob(TASK *pTask)
{
	SEM_WAIT(m_sem_job);
	while (m_pJobHead->state != JOB_STATE_EMPTY) {
		if (m_pJobHead->state == JOB_STATE_TODO) {
			*pTask = m_pJobHead->task;
			m_pJobHead->state = JOB_STATE_EMPTY;
			m_pJobHead = (JOB *)m_pJobHead->next_job;
			SEM_SIGNAL(m_sem_job);
			return 0;
		}
		if (m_pJobHead->state == JOB_STATE_CANCELED) {
			m_pJobHead->state = JOB_STATE_EMPTY; //scan next job
		}
		m_pJobHead = (JOB *)m_pJobHead->next_job;
	}
	SEM_SIGNAL(m_sem_job);
	return 1; //no job can pull
}

int NvtMgr::CancelJob(TASK *pTask)
{
	SEM_WAIT(m_sem_job);
	JOB *pHead = m_pJobHead;
	while (pHead != m_pJobTail) {
		if (pHead->task.client_data == pTask->client_data) {
			pHead->state = JOB_STATE_CANCELED;
		}
		pHead = (JOB *)pHead->next_job;
	}
	SEM_SIGNAL(m_sem_job);
	return 0;
}

int NvtMgr::SetupJobSocket()
{
	int er = 0;
	int reuseFlag = 1;
	MAKE_SOCKADDR_IN(SockaddrIn, htonl(INADDR_LOOPBACK), htons(559));

	if ((m_SocketPullServer = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("live555: m_SocketPullServer failed to create.\r\n");
		return -1;
	}

	setsockopt(m_SocketPullServer, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseFlag, sizeof(reuseFlag));

	if (bind(m_SocketPullServer, (struct sockaddr *)&SockaddrIn, sizeof(SockaddrIn)) != 0) {
		printf("live555: m_SocketPullServer failed to bind.\r\n");
		return -1;
	}

	if (listen(m_SocketPullServer, 1) != 0) {
		printf("live555: m_SocketPullServer failed to listen.\r\n");
		return -1;
	}

	if ((m_SocketPush = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("live555: m_SocketPush failed to create.\r\n");
		return -1;
	}

	setsockopt(m_SocketPush, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseFlag, sizeof(reuseFlag));

#ifdef SO_REUSEPORT
	if (setsockopt(m_SocketPush, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuseFlag, sizeof(reuseFlag)) == -1) {
		printf("live555: failed to SO_REUSEPORT.\r\n");
	}
#endif

#if defined(__ECOS) || defined(__FREERTOS)
	int tcpNoDelay = 1;
	setsockopt(m_SocketPush, IPPROTO_TCP, TCP_NODELAY, (void *)&tcpNoDelay, sizeof(tcpNoDelay));
#endif

	if ((er = connect(m_SocketPush, (struct sockaddr *)&SockaddrIn, sizeof(SockaddrIn))) != 0) {
		//coverity[dereference]
		printf("live555: m_SocketPush failed to connect. er=%d, errno=%d\r\n", er, errno);
		return -1;
	}

	SOCKLEN_T socksize = sizeof(SockaddrIn);
	if ((m_SocketPullClient = accept(m_SocketPullServer, (struct sockaddr *)&SockaddrIn, &socksize)) < 0) {
		printf("live555: m_SocketPullClient failed to accept.\r\n");
		return -1;
	}

	return m_SocketPullClient;
}

int NvtMgr::FlushJobSocket()
{
	int dummy_buf[64];
	//coverity[check_return]
	recv(m_SocketPullClient, (char *)dummy_buf, sizeof(dummy_buf), 0);
	return 0;
}

int NvtMgr::RunCmdThread()
{
#if defined(__LINUX) || defined(__UBUNTU)
	key_t key = ftok("/bin/nvtrtspd", 0);
	if (key == -1) {
		fprintf(stderr, "live555: /bin/nvtrtspd is not existing.\n");
	}
	// create mailbox
	if ((m_msqid = msgget(key, IPC_CREAT | 0666)) < 0) {
		fprintf(stderr, "live555: msgget failed\n");
	}

	m_pCmdStack = new u_int8_t[1024];
	m_bCmdLoop = True;
	THREAD_CREATE(session_name, m_hCmd, ThreadCmd, this, m_pCmdStack, 1024, &m_hCmdObj);
	THREAD_RESUME(m_hCmd);
#endif
	return 0;
}


int NvtMgr::SetQuitVariable(char* pWatchVariable)
{
	m_pWatchVariable = pWatchVariable;
	return 0;
}

#if defined(__LINUX) || defined(__UBUNTU)
THREAD_DECLARE(NvtMgr::ThreadCmd, pNvtMgr)
{
	NvtMgr *pMgr = (NvtMgr *)pNvtMgr;
	MSG_BUF msg;
	while (pMgr->m_bCmdLoop)
	{
		if (msgrcv(pMgr->m_msqid, &msg, sizeof(msg.cmd), 1, 0) < 0) {
			if (pMgr->m_bCmdLoop) { //avoid quit
				printf("live555: msgrcv failed.\r\n");
			}
			break;
		} else {
			memcpy(&g_MsgCmd, &msg.cmd, sizeof(g_MsgCmd));
			NvtMgr::TASK task = { pMgr->DispatchCmd, &g_MsgCmd };
			pMgr->PushJob(&task);
		}
	}
	THREAD_EXIT();
}
#elif defined(__ECOS) || defined(__FREERTOS)
#if defined(__ECOS)
#include <cyg/infra/mainfunc.h>
#elif defined(__FREERTOS)
#include <kwrap/cmdsys.h>
#endif
MAINFUNC_ENTRY(nvtrtspd, argc, argv)
{
	NvtMgr *pMgr = NvtMgr_GetHandle();
	if (pMgr == NULL) {
		printf("usage: live555 isn't running.\r\n");
		return 0;
	}
	if (argc < 2) {
		printf("usage: nvtlive555 help to get help \r\n");
	} else if (argc > CFG_MSG_ARGV_MAX_COUNT) {
		fprintf(stderr, "live555: argc cannot larger than %d\n", CFG_MSG_ARGV_MAX_COUNT);
		return -1;
	} else {
		g_MsgCmd.argc = argc;
		for (int i = 0; i < argc; i++) {
			strncpy(g_MsgCmd.argv[i], argv[i], sizeof(g_MsgCmd.argv[i]) - 1);
		}
		NvtMgr::TASK task = { pMgr->DispatchCmd, &g_MsgCmd };
		pMgr->PushJob(&task);
	}
	return 0;
}
#endif

NvtMgr *NvtMgr_GetHandle()
{
	return g_pNvtMgr;
}

int NvtMgr_CreateMgr()
{
	if (g_pNvtMgr == NULL) {
		g_pNvtMgr = new NvtMgr;
		return 0;
	}
	return 1; // already has bad obj
}

int NvtMgr_DestroyMgr()
{
	if (g_pNvtMgr) {
		delete g_pNvtMgr;
		g_pNvtMgr = NULL;
		return 0;
	}
	return 1; // already has destroyed
}


void NvtMgr::DispatchCmd(void* firstArg)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	MSG_CMD *pCmd = (MSG_CMD *)firstArg;
	if (strcmp(pCmd->argv[1], "?") == 0) {
		printf("live555 command list:\r\n");
		printf("dump: dump last status\r\n");
		printf("tm: dump time offset difference from uitron\r\n");
		printf("event on/off: turn on/off event message\r\n");
		printf("lv on/off: latency for video (wait time) uitron->wait_done->push_done->pull_done->transfered_done\r\n");
		printf("la on/off: latency for audio\r\n");
		printf("audio on/off: support audio stream at next connection.\r\n");
		printf("fps: evaluate each client's fps.\r\n");
		printf("quit: quit application\r\n");
	} else if (strcmp(pCmd->argv[1], "quit") == 0) {
		if (pNvtMgr->m_pWatchVariable) {
			*pNvtMgr->m_pWatchVariable = 1;
		}
	} else if (strcmp(pCmd->argv[1], "dump") == 0) {
		printf("rtcp: %d\r\n", pNvtMgr->m_UserCfg.support_rtcp);
		printf("meta: %d\r\n", pNvtMgr->m_UserCfg.support_meta);
		printf("port: %d\r\n", pNvtMgr->m_UserCfg.port);
		printf("SO_SNDBUF: %d bytes\r\n", pNvtMgr->m_UserCfg.snd_size);
		printf("tos: %d\r\n", pNvtMgr->m_UserCfg.tos);
		pNvtMgr->Get_StrmCb()->DbgCmd(NVT_DBG_CMD_DUMP);
	} else if (strcmp(pCmd->argv[1], "event") == 0) {
		if (strcmp(pCmd->argv[2], "on") == 0) {
			pNvtMgr->m_bShowEvent = True;
			printf("live555: turn on event msg.\r\n");
		} else {
			pNvtMgr->m_bShowEvent = False;
			printf("live555: turn off event msg.\r\n");
		}
	} else if (strcmp(pCmd->argv[1], "tm") == 0) {
		struct timeval PtsOfs;
		printf("live555: gettimeofday-uitron_time (init) = %ld.%ld\r\n", pNvtMgr->m_PtsOfs.tv_sec, pNvtMgr->m_PtsOfs.tv_usec);
		pNvtMgr->xCalibratePtsOfs(&PtsOfs);
		printf("live555: gettimeofday-uitron_time (now) = %ld.%ld\r\n", PtsOfs.tv_sec, PtsOfs.tv_usec);
	} else if (strcmp(pCmd->argv[1], "lv") == 0) {
		if (strcmp(pCmd->argv[2], "on") == 0) {
			pNvtMgr->m_bShowLatencyV = True;
			printf("live555: turn on lv.\r\n");
		} else {
			pNvtMgr->m_bShowLatencyV = False;
			printf("live555: turn off lv.\r\n");
		}
	} else if (strcmp(pCmd->argv[1], "la") == 0) {
		if (strcmp(pCmd->argv[2], "on") == 0) {
			pNvtMgr->m_bShowLatencyA = True;
			printf("live555: turn on la.\r\n");
		} else {
			pNvtMgr->m_bShowLatencyA = False;
			printf("live555: turn off la.\r\n");
		}
	} else if (strcmp(pCmd->argv[1], "audio") == 0) {
		if (strcmp(pCmd->argv[2], "on") == 0) {
			pNvtMgr->m_bSupportAudio = True;
			printf("live555: turn on audio.\r\n");
		} else {
			pNvtMgr->m_bSupportAudio = False;
			printf("live555: turn off audio.\r\n");
		}
	} else if (strcmp(pCmd->argv[1], "fps") == 0) {
		pNvtMgr->Get_StrmCb()->DbgCmd(NVT_DBG_CMD_FPS);
	} else {
		printf("live555: unknown command.\r\n");
	}
}
