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
// Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved
// LIVE555 Media Server
// main program

#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "DynamicRTSPServer.hh"
#include "version.hh"
#include "PatternFrm.h"
#include "NvtStreamQueue.hh"
#if defined(__LINUX) || defined(__UBUNTU)
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

#define CFG_SCHEDULE_DELAY 1000000 // 1 sec, (unit: microseconds)
//#define CFG_SCHEDULE_DELAY 1000 // 1 sec, (unit: microseconds)

#define FAKE_INTERFACE_TYPE_NONE 0
#define FAKE_INTERFACE_TYPE_STATIC_FRM 1
#define FAKE_INTERFACE_TYPE_FILE_FRM 2
#if (CFG_FAKE_INTERFACE)
#define CFG_FAKE_INTERFACE_TYPE FAKE_INTERFACE_TYPE_STATIC_FRM
#else
#define CFG_FAKE_INTERFACE_TYPE FAKE_INTERFACE_TYPE_NONE
#endif

#if defined(__WIN32__) || defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#elif defined(__LINUX) && !defined(_NO_NVT_INFO_)
#include <nvtinfo.h>
#endif

static char g_fWatchVariable;
#if defined(__ECOS)
static u_int32_t g_Stack[8192 / sizeof(u_int32_t)];
static void Thread_Start(void);
#elif  defined(__FREERTOS)
static void Thread_Start(void);
#endif
int  Live555_Main(int argc, char** argv);

#if defined(__ECOS)
static THREAD_DECLARE(ThreadProc, lpParam);
static THREAD_HANDLE m_hThread = 0;
static THREAD_OBJ  m_hThread_Obj;
static void Thread_Start(void)
{
    u_int32_t uiStackSize = sizeof(g_Stack);
    THREAD_CREATE("RTSP_MAIN",m_hThread,ThreadProc,NULL,g_Stack,uiStackSize,&m_hThread_Obj);
    THREAD_RESUME(m_hThread);
}

static THREAD_DECLARE(ThreadProc,lpParam)
{
    Live555_Main(0, NULL);
    THREAD_EXIT();
}
#elif defined(__FREERTOS)
static THREAD_DECLARE(ThreadProc, lpParam);
static THREAD_HANDLE m_hThread;
static void Thread_Start(void)
{
	THREAD_CREATE("RTSP_MAIN", m_hThread, ThreadProc, NULL, NULL, 0, NULL);
	THREAD_RESUME(m_hThread);
}

static THREAD_DECLARE(ThreadProc, lpParam)
{
	Live555_Main(0, NULL);
	THREAD_EXIT();
}
#endif

int nvtlive555_open(NVTLIVE555_INIT *p_init)
{
	int er;

	if (NvtMgr_CreateMgr() != 0) {
		printf("live555 created failed.\r\n");
		return -1;
	}

	NvtMgr* pNvtMgr = NvtMgr_GetHandle();
	if (p_init->version != NVTLIVE555_INTERFACE_VER_VERSION) {
		printf("nvtlive555_open: invalid version %08X(exe) != %08X(lib)\r\n", p_init->version, NVTLIVE555_INTERFACE_VER_VERSION);
		NvtMgr_DestroyMgr();
		return -1;
	}

	g_fWatchVariable = 0;
	pNvtMgr->SetQuitVariable(&g_fWatchVariable);

	er = pNvtMgr->Set_UserCb(&p_init->require_cb);
	if (er < 0) {
		printf("nvtlive555_open: Set_UserCb failed\r\n");
		NvtMgr_DestroyMgr();
		return er;
	}

	er = pNvtMgr->Set_HdalCb(&p_init->hdal_cb);
	if (er < 0) {
		printf("nvtlive555_open: Set_HdalCb failed\r\n");
		NvtMgr_DestroyMgr();
		return er;
	}

	er = pNvtMgr->RunCmdThread();
	if (er < 0) {
		printf("nvtlive555_open: RunCmdThread failed\r\n");
		NvtMgr_DestroyMgr();
		return er;
	}

#if defined(__ECOS) || defined(__FREERTOS)
	Thread_Start();
#else
	Live555_Main(0, NULL);
#endif
	return er;
}

int nvtlive555_close()
{
	if (NvtMgr_GetHandle()) {
		g_fWatchVariable = 1;
		while (g_fWatchVariable) {
			SLEEP_MS(100);
		}
	}

#if defined(__ECOS)
	if (m_hThread) {
		THREAD_JOIN(m_hThread);
		THREAD_DESTROY(m_hThread);
		m_hThread = 0;
	}
#elif defined(__FREERTOS)
	if (m_hThread) {
		THREAD_JOIN(m_hThread);
		m_hThread = 0;
	}
#endif
	return 0;
}

int nvtlive555_dbgcmd(int argc, char* argv[])
{
#if defined(__LINUX) || defined(__UBUNTU)
	if (argc > CFG_MSG_ARGV_MAX_COUNT) {
		fprintf(stderr, "live555: argc cannot larger than %d\n", CFG_MSG_ARGV_MAX_COUNT);
		return -1;
	}
	key_t key = ftok("/bin/nvtrtspd", 0);
	if (key == -1) {
		fprintf(stderr, "live555: /bin/nvtrtspd is not existing.\n");
		return -1;
	}
	int msqid = msgget(key, 0666);
	if (msqid < 0) {
		fprintf(stderr, "live555: nvtrtspd may not start.\n");
		return -1;
	}

	NvtMgr::MSG_BUF msg = { 0 };
	msg.mtype = 1;
	for (int i=0; i<argc ;i++)	{
		strncpy(msg.cmd.argv[i], argv[i], sizeof(msg.cmd.argv[i]) - 1);
	}
	int er = msgsnd(msqid, &msg, sizeof(msg.cmd), IPC_NOWAIT);
	if (er < 0) {
		fprintf(stderr, "live555: failed to send command, er=%d.\n", er);
		return -1;
	}
	return 0;
#else
	printf("live555: not support nvtlive555_dbgcmd.\r\n");
	return 0;
#endif
}

static void PullJobHandler(void*, int /*mask*/)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();

	pNvtMgr->FlushJobSocket();
	NvtMgr::TASK task = { 0 };
	while (pNvtMgr->PullJob(&task) == 0) {
		(*task.proc)(task.client_data);
	}
}

int  Live555_Main(int argc, char** argv)
{
#if defined (__LINUX) && !defined(_NO_NVT_INFO_)
    nvtbootts_set((char*)"live555", true);
#endif
#if defined(WIN32)
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
    printf("live555 start.\r\n");

 	NvtMgr* pNvtMgr = NvtMgr_GetHandle(); //NVT_MODIFIED

#if (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_NONE)	
	ISTRM_CB InterfaceCb = *HdalFrmGetStrmCb();
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_STATIC_FRM)
	ISTRM_CB InterfaceCb = *StaticFrmGetStrmCb();
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_FILE_FRM)
	ISTRM_CB InterfaceCb = *FileFrmGetStrmCb();
#endif

	pNvtMgr->Set_StrmCb(&InterfaceCb);

	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew(CFG_SCHEDULE_DELAY);
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	UserAuthenticationDatabase* authDB = NULL;

#if (CFG_TEST_AUTH)
	// To implement client access control to the RTSP server, do the following:
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("admin", "admin"); // replace these with real strings
	// Repeat the above with each <username>, <password> that you wish to allow
	// access to the server.
#endif

	// Create the RTSP server.  Try first with the default port number (554),
	// and then with the alternative port number (8554):
	//NVT_MODIFIED
	RTSPServer* rtspServer;
	rtspServer = DynamicRTSPServer::createNew(*env, authDB, CFG_SERVER_TIMEOUT_SEC);
	/*
	if (rtspServer == NULL) {
		rtspServerPortNum = 8554;
		rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
	}*/
	if (rtspServer == NULL) {
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		NvtMgr_DestroyMgr();
		return -1;
	}

#if 0 //disable rtsp over http #NT#IVOT_N00017_CO-241
	// Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
	// Try first with the default HTTP port (80), and then with the alternative HTTP
	// port numbers (8000 and 8080).
	//rtspServer->setUpTunnelingOverHTTP(80) || //because of HFS
	if (rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
#if defined(__WIN32__)
		*env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
#endif
	} else {
		*env << "(RTSP-over-HTTP tunneling is not available.)\n";
	}
#endif

#if defined (__LINUX) && !defined(_NO_NVT_INFO_)
     nvtbootts_set((char*)"live555", true);
#endif


	int pull_socket = pNvtMgr->SetupJobSocket();

	if (pull_socket < 0) {
		printf("live555: failed to SetupJobSocket.\r\n");
		Medium::close(rtspServer);
		env->reclaim(); env = NULL;
		delete scheduler; scheduler = NULL;
		NvtMgr_DestroyMgr();
		return -1;
	}

	env->taskScheduler().setBackgroundHandling(pull_socket, SOCKET_READABLE | SOCKET_EXCEPTION,
		(TaskScheduler::BackgroundHandlerProc*)&PullJobHandler, NULL);

	//*pLive555QuitAddr = 0; //user may quit before process reached here
	env->taskScheduler().doEventLoop(&g_fWatchVariable); // does not return

	Medium::close(rtspServer);
	env->reclaim(); env = NULL;
	delete scheduler;
	//coverity[assigned_pointer]
	scheduler = NULL;

#if 0
	if(authDB) {
		delete authDB;
	}
#endif

	if (NvtMgr_DestroyMgr() != 0) {
		printf("live555 destroy failed.\r\n");
		g_fWatchVariable = 0;
		return -1;
	}

	g_fWatchVariable = 0;
	printf("live555 exit.\r\n");
	return 0; // only to prevent compiler warning
}

extern "C" int FrmSeek(int channel, double request, double *response)
{
#if (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_NONE)
	printf("live555: not support FrmSeek\r\n");
	return -1;
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_STATIC_FRM)
	*response = 0;
	return 0;
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_FILE_FRM)
	return FileFrmSeek(channel, request, response);
#endif
	return -1;
}

extern "C" int FrmSetState(int channel, FILEFRM_STATE state)
{
#if (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_NONE)
	printf("live555: not support FrmSetState\r\n");
	return -1;
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_STATIC_FRM)
	return 0;
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_FILE_FRM)
	return FileFrmSetState(channel, state);
#endif
	return 0;
}

extern "C" int FrmGetTotalTime(int channel, unsigned int *total_ms)
{
#if (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_NONE)
	printf("live555: not support FrmGetTotalTime\r\n");
	return -1;
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_STATIC_FRM)
	*total_ms = 0;
	return 0;
#elif (CFG_FAKE_INTERFACE_TYPE == FAKE_INTERFACE_TYPE_FILE_FRM)
	return FileFrmGetTotalTime(channel, total_ms);
#endif
	return -1;
}
