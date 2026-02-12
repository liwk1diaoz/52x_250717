// live555dll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "live555dll.h"
#include "NvtMedia.hh"

#if (CFG_FAKE_INTERFACE)

extern int Live555Cli_Main(int argc, char** argv);
extern "C" INT32 Live555Cli_Close(void);

char url_tmp[1024] = {0};
extern char* live555dll_url;
extern void (*m_fpSetOpen)(void* pAddr,unsigned int size);
extern void (*m_fpSetClose)(void* pAddr,unsigned int size);
extern void (*m_fpSetVideo)(void* pAddr,unsigned int size);
extern void (*m_fpSetAudio)(void* pAddr,unsigned int size);

extern "C" LIVE555DLL_API int live555dll_SetCbOpen(void (*fpSetOpen)(void* pAddr,unsigned int size))
{
    m_fpSetOpen = fpSetOpen;
    return 0;
}

extern "C" LIVE555DLL_API int live555dll_SetCbClose(void (*fpSetClose)(void* pAddr,unsigned int size))
{
    m_fpSetClose = fpSetClose;
    return 0;
}

extern "C" LIVE555DLL_API int live555dll_SetCbVideo(void (*fpSetVideo)(void* pAddr,unsigned int size))
{
    m_fpSetVideo = fpSetVideo;
    return 0;
}

extern "C" LIVE555DLL_API int live555dll_SetCbAudio(void (*fpSetAudio)(void* pAddr,unsigned int size))
{
    m_fpSetAudio = fpSetAudio;
    return 0;
}

extern "C" LIVE555DLL_API int live555dll_Open(const char* szUrl)
{
    strcpy(url_tmp,szUrl);
    live555dll_url = url_tmp;

    Live555Cli_Main(0,NULL);
    return 0;
}

extern "C" LIVE555DLL_API int live555dll_Close(void)
{
    Live555Cli_Close();
    return 0;
}
#else
#pragma message ("!!!!!!>>>>>>  live555 dll must enable CFG_FAKE_INTERFACE  <<<<<<!!!!!!")
#endif