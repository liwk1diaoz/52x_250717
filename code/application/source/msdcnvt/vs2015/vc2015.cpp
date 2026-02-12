// msdcnvt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../include/msdcnvt.h"
#include "../src/msdcnvt_ipc.h"
#include "../src/msdcnvt_int.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

static void msdcnvt_icmd_cb_open(MSDCNVT_ICMD *icmd){}
static void msdcnvt_icmd_cb_close(MSDCNVT_ICMD *icmd){}
static void msdcnvt_icmd_cb_verify(MSDCNVT_ICMD *icmd){}
static void msdcnvt_icmd_cb_vendor(MSDCNVT_ICMD *icmd){}

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    int nRetCode = 0;

	AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);

	MSDCNVT_IPC_CFG m_ipc_cfg = { 0 };

	MSDCNVT_ICALL_TBL calls = {
		msdcnvt_icmd_cb_open,
		msdcnvt_icmd_cb_close,
		msdcnvt_icmd_cb_verify,
		msdcnvt_icmd_cb_vendor,
	};

	char string_ipc_addr[16] = { 0 };
	char string_ipc_size[16] = { 0 };

	sprintf_s(string_ipc_addr, "0x%08X", (unsigned int)&m_ipc_cfg);
	sprintf_s(string_ipc_size, "0x%08X", sizeof(m_ipc_cfg));

	char *av[] = {
		"msdcnvt.exe",
		string_ipc_addr,
		string_ipc_size,
	};

	m_ipc_cfg.ipc_version = MSDCNVT_IPC_VERSION;
	m_ipc_cfg.port = 1968;
	m_ipc_cfg.call_tbl_addr = dma_getPhyAddr((unsigned int)&calls);
	m_ipc_cfg.call_tbl_count = sizeof(calls) / sizeof(MSDCNVT_IAPI);
	msdcnvt_main(3, av);

    return nRetCode;
}
