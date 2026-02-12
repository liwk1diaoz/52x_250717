#include <BasicUsageEnvironment.hh>
#include <string.h>
#include "NvtMedia.hh"

#if defined(__WIN32__) || defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

extern int  Live555Cli_Main(int argc, char** argv);

extern "C" int nvtrtspd_open();
extern "C" int nvtrtspd_close();

#if defined(WIN32)
#if (CFG_MAIN_TYPE==2)
static THREAD_HANDLE m_hThread;
static THREAD_OBJ  m_hThread_Obj;
THREAD_DECLARE(Server_Thread,lpParam)
{
	nvtrtspd_open();
    THREAD_EXIT();
}
#endif
int main(int argc, char** argv)
{
#if (CFG_MAIN_TYPE==0)    
    return Live555Cli_Main(argc,argv);
#elif (CFG_MAIN_TYPE==1)
    return nvtrtspd_open();
#elif (CFG_MAIN_TYPE==2)
    u_int32_t uiStackSize = 0;
    THREAD_CREATE("RTSP_SERVER",m_hThread,Server_Thread,NULL,NULL,uiStackSize,&m_hThread_Obj);
    THREAD_RESUME(m_hThread);
    Sleep(10);
    Live555Cli_Main(argc,argv);
	nvtrtspd_close();
    THREAD_JOIN(m_hThread);
	THREAD_DESTROY(m_hThread);
#endif     
}
#endif