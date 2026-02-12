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

#if defined(__WIN32__) || defined(_WIN32)
#define CFG_DUMP_VIDEO 0
#define CFG_DUMP_AUDIO 0
#else
#define CFG_DUMP_VIDEO 0 //only support WIN32
#define CFG_DUMP_AUDIO 0 //only support WIN32
#endif

#if defined(__WIN32__) || defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#elif defined(__LINUX) && !defined(_NO_NVT_IPC_)
#include <nvtipc.h>
#if defined(__LINUX650) || defined(__LINUX660)
#include <nvtmem.h>
#include <protected/nvtmem_protected.h>
#endif
#elif defined(__ECOS660)
#include <cyg/nvtipc/NvtIpcAPI.h>
#include <cyg/compat/uitron/uit_func.h>
#define dma_getCacheAddr(addr) (((addr) & 0x1FFFFFFF) | 0x80000000)
#endif

#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPClient.hh"
#include "NvtMediaCliData.hh"
#include "version.hh"

typedef struct _MAIN_PARAM{
    int argc;
    char param0[16];
    char param1[16];
    char param2[16];
}MAIN_PARAM;

static char g_fWatchVariable2;

#if (CFG_DUMP_VIDEO)
static FILE* fp_dump_v = NULL;
static int i_frame_v = 0;
#endif
#if (CFG_DUMP_AUDIO)
static FILE* fp_dump_a = NULL;
static int i_frame_a = 0;
#endif

int  Live555Cli_Main(int argc, char** argv);

#if defined (__ECOS660)
static MAIN_PARAM g_MainParam = { 0 };
static u_int32_t g_Stack[8192 / sizeof(u_int32_t)];
static void Thread_Start(void);

static THREAD_DECLARE(ThreadProc,lpParam);

static THREAD_HANDLE m_hThread;
static THREAD_OBJ  m_hThread_Obj;

static void Thread_Start(void)
{
    u_int32_t uiStackSize = sizeof(g_Stack);
    THREAD_CREATE("RTSPCLI_MAIN",m_hThread,ThreadProc,NULL,g_Stack,uiStackSize,&m_hThread_Obj);
    THREAD_RESUME(m_hThread);
}

static THREAD_DECLARE(ThreadProc,lpParam)
{
    char* argv[3];
    argv[0] = g_MainParam.param0;
    argv[1] = g_MainParam.param1;
    argv[2] = g_MainParam.param2;
    Live555Cli_Main(3,argv);
    THREAD_EXIT();
}
#endif

#if (CFG_FAKE_INTERFACE)
static void SetFakeCb(void* pWorkingAddr, u_int32_t uiWorkingSize);
#if !defined(_NO_NVT_IPC_)
static void SetFakeIpc(u_int32_t uiAddr, u_int32_t uiSize);
#endif
#endif

#if defined(__ECOS650)
extern "C" INT32 Live555Cli_Open(void)
{
    return Live555Cli_Main(0,NULL);
}
#elif defined (__ECOS660)
extern "C" INT32 Live555Cli_Open2(char* szCmdStr)
{
    int argc = sscanf(szCmdStr,"%s %s %s",g_MainParam.param0,g_MainParam.param1,g_MainParam.param2);
    if(argc!=3)
    {
        printf("live555 got incorrect parameters.\r\n");
        return -1;
    }

    g_MainParam.argc = argc;
    Thread_Start();

    return 0;
}
#endif


int  Live555Cli_Main(int argc, char** argv)
{
#if defined(WIN32)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
     printf("live555 client start.\r\n");

  	 char* pLive555QuitAddr = &g_fWatchVariable2; //default use &g_fWatchVariable2;
 	 NvtMediaCliData* pNvtData = new NvtMediaCliData; //NVT_MODIFIED

#if defined(__LINUX) || defined(__ECOS660)
    if(argc<3 || argc>4)
    {
        printf("live555 got incorrect parameters.\r\n");
	    delete pNvtData;
	    return 0;
    }
    else
    {
	    u_int32_t uiAddr;
	    u_int32_t uiSize;
	    sscanf(argv[1],"%08X",&uiAddr);
	    sscanf(argv[2],"%08X",&uiSize);
#if defined(__LINUX680)			
		NvtIPC_mmap(NVTIPC_MEM_TYPE_NONCACHE,NVTIPC_MEM_ADDR_DRAM1_ALL_FLAG,0);
#endif	

#if !defined(_NO_NVT_IPC_)
#if (!CFG_FAKE_INTERFACE)
	    pNvtData->Set_IPC(uiAddr,uiSize);	
	    pLive555QuitAddr = (char*)pNvtData->Trans_Phy_To_NonCache(uiAddr + uiSize - sizeof(char));
	    //Set Interface
	    Live555Cli_SetConfig((LIVE555CLI_CONFIG*)pNvtData->Trans_Phy_To_NonCache(uiAddr));
#else
        if(uiAddr!=0 && uiSize!=0)
        {
            SetFakeIpc(uiAddr,uiSize);
            pLive555QuitAddr = (char*)NvtIPC_GetNonCacheAddr(uiAddr + uiSize - sizeof(char));
        }
#endif // (!CFG_FAKE_INTERFACE)
#endif
    }
#endif //defined(__LINUX) || defined(__ECOS660)

#if (CFG_FAKE_INTERFACE)
  u_int32_t uiWorkingSize = 0x180000;
  unsigned char* pWorkingAddr = new unsigned char[uiWorkingSize];
  SetFakeCb(pWorkingAddr, uiWorkingSize);
#endif //(CFG_FAKE_INTERFACE)

  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);


  RTSPClient* rtspClient = DynamicRTSPClient::createNew(*env, pNvtData, 1);
  pNvtData->SetRTSPClientPointer(rtspClient);

  if (rtspClient == NULL)
  {
      *env << "Failed to create a RTSP client for URL \"" << pNvtData->Get_Url() << "\": " << env->getResultMsg() << "\n";
      *pLive555QuitAddr = 1;
  }
  else
  {
      ((DynamicRTSPClient*)rtspClient)->SendDescribeCommand();
  }
  // All subsequent activity takes place within the event loop:
  //*pLive555QuitAddr = 0; //user may quit before process reached here
  env->taskScheduler().doEventLoop(pLive555QuitAddr); // does not return

  if(pNvtData->GetRTSPClientPointer())
  {
      shutdownStream(rtspClient);
  }

  // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
  // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
  // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
  // Medium::close(rtspClient); it has closed via shutdownStream.
  Medium::close(rtspClient);
  env->reclaim(); env = NULL;
  delete scheduler;
  //coverity[assigned_pointer]
  scheduler = NULL;
  delete pNvtData;

#if (CFG_FAKE_INTERFACE)
  delete pWorkingAddr;
#endif //(CFG_FAKE_INTERFACE)

  printf("live555 client exit.\r\n");
  *pLive555QuitAddr = 0;

  return 0; // only to prevent compiler warning
}

INT32 Live555Cli_Close(void)
{
    g_fWatchVariable2 = 1;

    return 0;
}

#if (CFG_FAKE_INTERFACE)
char* live555dll_url = NULL;
void (*m_fpSetOpen)(void* pAddr,unsigned int size) = NULL;
void (*m_fpSetClose)(void* pAddr,unsigned int size) = NULL;
void (*m_fpSetVideo)(void* pAddr,unsigned int size) = NULL;
void (*m_fpSetAudio)(void* pAddr,unsigned int size) = NULL;
#if !defined(_NO_NVT_IPC_)
static NvtMediaCliData::IPC m_IPC = { 0 };
#endif
static LIVE555CLI_STREAM_INFO m_StrmInfo = {0};
static UINT32 xRtspNvtCli_OnOpen(LIVE555CLI_STREAM_INFO* pInfo)
{
    m_StrmInfo = *pInfo;

    if(m_fpSetOpen)
    {
        m_fpSetOpen((void*)pInfo,sizeof(LIVE555CLI_STREAM_INFO));
    }
    return 0;
}

static UINT32 xRtspNvtCli_OnClose(void)
{
    g_fWatchVariable2 = 1;

    if(m_fpSetClose)
    {
        m_fpSetClose(NULL,0);
    }
    return 0;
}

static UINT32 xRtspNvtCli_OnSetVideo(LIVE555CLI_FRM* pFrm)
{
#if (CFG_DUMP_VIDEO)
    const u_int8_t hdr[4]={0x00,0x00,0x00,0x01};
    if(i_frame_v==0) //open file
    {
        fp_dump_v = fopen("dump_v.bin","wb");
    }
    if(m_StrmInfo.vid.uiCodec == LIVE555CLI_CODEC_VIDEO_H264 ||
		m_StrmInfo.vid.uiCodec == LIVE555CLI_CODEC_VIDEO_H265)
    {
        fwrite(hdr,4,1,fp_dump_v);
    }
    fwrite((u_int8_t*)pFrm->uiAddr,pFrm->uiSize,1,fp_dump_v);
    i_frame_v++;

    if(i_frame_v == 30)
    {
        Live555Cli_Close();
    }
#endif
    if(m_fpSetVideo)
    {
        m_fpSetVideo((void*)pFrm->uiAddr,pFrm->uiSize);
    }

    //trans address into linear
    //printf("%d\r\n",pFrm->uiSize);
    /*
    if(m_StrmInfo.vid.uiCodec == LIVE555CLI_CODEC_VIDEO_H264)
    {
        u_int8_t nal_unit_type = (*(u_int8_t*)pFrm->uiAddr)&0x1F;
        if (nal_unit_type == 7 //SPS
            || nal_unit_type == 8) //PPS
        {
            //skip
            return 0;
        }
    }*/


    return 0;
}

static UINT32 xRtspNvtCli_OnSetAudio(LIVE555CLI_FRM* pFrm)
{
#if (CFG_DUMP_AUDIO)
    if(i_frame_a==0) //open file
    {
        fp_dump_a = fopen("dump_a.bin","wb");
    }
    fwrite((u_int8_t*)pFrm->uiAddr,pFrm->uiSize,1,fp_dump_a);
    i_frame_a++;

    if(i_frame_a == 30)
    {
        Live555Cli_Close();
    }
#endif

    if(m_fpSetAudio)
    {
        m_fpSetAudio((void*)pFrm->uiAddr,pFrm->uiSize);
    }
    //trans address into linear
    //printf("%d\r\n",pFrm->uiSize);
    return 0;
}

static UINT32 xRtspNvtCli_OnProcessEnd(void)
{
#if defined(__LINUX650)
#error "unsupported!"
#elif !defined(_NO_NVT_IPC_) && (defined(__LINUX) || defined(__ECOS660))
    if(m_IPC.use_ipc)
    {
        LIVE555CLI_ICMD* pCmd = (LIVE555CLI_ICMD*) m_IPC.pBufAddr;
        memset(pCmd,0,sizeof(*pCmd));
        pCmd->CmdIdx = LIVE555CLI_ICMD_IDX_PROCESS_END;

        LIVE555CLI_IPC_MSG msg_snd={0};
        LIVE555CLI_IPC_MSG msg_rcv={0};
        msg_snd.mtype = 1;
        msg_snd.uiIPC = pCmd->CmdIdx;
        if (NvtIPC_MsgSnd(m_IPC.ipc_msgid,NVTIPC_SENDTO_CORE1, &msg_snd, sizeof(msg_snd)) <= 0)
        {
            printf("err:msgsnd:%d\r\n",(int)pCmd->CmdIdx);
        }
        if (NvtIPC_MsgRcv(m_IPC.ipc_msgid, &msg_rcv, sizeof(msg_rcv))<=0)
        {
            printf("err:msgrcv\r\n");
        }
        if(msg_rcv.uiIPC!=msg_snd.uiIPC)
        {
            printf("err:msg.uiIPC!=wait %d %d\r\n",(int)msg_snd.uiIPC,(int)msg_rcv.uiIPC);
        }

        NvtIPC_MsgRel(m_IPC.ipc_msgid);
    }
#endif

#if (CFG_DUMP_VIDEO)
    fclose(fp_dump_v);
#endif
#if (CFG_DUMP_AUDIO)
    fclose(fp_dump_a);
#endif

    return 0;
}

#define RTSPNVT_SIZE_ASSERT(income_size, name) {if(sizeof(name)!=(income_size)) {printf("live555: %s not matched\r\n",#name);}}

static void xRtspNvtCli_ICmd(LIVE555CLI_ICMD* pCmd)
{
    switch(pCmd->CmdIdx)
    {
    case LIVE555CLI_ICMD_IDX_SET_VIDEO:
        RTSPNVT_SIZE_ASSERT(pCmd->uiInSize,LIVE555CLI_FRM);
        pCmd->uiResult = xRtspNvtCli_OnSetVideo((LIVE555CLI_FRM*)pCmd->uiInAddr);
        break;
    case LIVE555CLI_ICMD_IDX_SET_AUDIO:
        RTSPNVT_SIZE_ASSERT(pCmd->uiInSize,LIVE555CLI_FRM);
        pCmd->uiResult = xRtspNvtCli_OnSetAudio((LIVE555CLI_FRM*)pCmd->uiInAddr);
        break;
    case LIVE555CLI_ICMD_IDX_OPEN_SESSION:
        RTSPNVT_SIZE_ASSERT(pCmd->uiInSize,LIVE555CLI_STREAM_INFO);
        pCmd->uiResult = xRtspNvtCli_OnOpen((LIVE555CLI_STREAM_INFO*)pCmd->uiInAddr);
        break;
    case LIVE555CLI_ICMD_IDX_CLOSE_SESSION:
        pCmd->uiResult = xRtspNvtCli_OnClose();
        break;
    case LIVE555CLI_ICMD_IDX_PROCESS_END:
        pCmd->uiResult = xRtspNvtCli_OnProcessEnd();
        break;
    default:
        printf("unknown pCmd->CmdIdx:%d\r\n",pCmd->CmdIdx);
        break;
    }
}

static LIVE555CLI_CONFIG m_FakeConfig = {
    LIVE555CLI_INTERFACE_VER,
    xRtspNvtCli_ICmd,
    0,
    0,
    0,
    0,
};

static void SetFakeCb(void* pWorkingAddr, u_int32_t uiWorkingSize)
{
	if (live555dll_url != NULL) {
		strncpy(m_FakeConfig.szUrl, live555dll_url, sizeof(m_FakeConfig.szUrl) - 1);
	} else {
		strncpy(m_FakeConfig.szUrl, "rtsp://admin:admin@localhost:554/", sizeof(m_FakeConfig.szUrl) - 1);
		//strncpy(m_FakeConfig.szUrl, "rtsp://192.168.0.3/live/ch00_0", sizeof(m_FakeConfig.szUrl) - 1);
	}

    m_FakeConfig.uiWorkingAddr = (UINT32)pWorkingAddr;
    m_FakeConfig.uiWorkingSize = uiWorkingSize;
    m_FakeConfig.uiDisableVideo = 0;
    m_FakeConfig.uiDisableAudio = 0;
    Live555Cli_SetConfig(&m_FakeConfig);
}

#if !defined(_NO_NVT_IPC_)
static void SetFakeIpc(u_int32_t uiAddr, u_int32_t uiSize)
{
#if defined(__LINUX680) || defined(__ECOS680)
	m_IPC.pBufAddr = (void*)NvtIPC_GetNonCacheAddr(uiAddr);
	m_IPC.BufSize = uiSize;
	m_IPC.ipc_msgid = NvtIPC_MsgGet(NvtIPC_Ftok("RTSPCLINVT"));
	if (m_IPC.ipc_msgid < 0)
	{
		printf("id_send failed to msgget\r\n");
	}
	m_IPC.use_ipc = 1;
#endif
}
#endif //!defined(_NO_NVT_IPC_)
#endif
