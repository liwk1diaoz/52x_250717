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

#include "NvtMediaCliData.hh"
#include "GroupsockHelper.hh"
#include <stdio.h>

#if defined(__LINUX)
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ioctl.h>
#if defined(__LINUX650) || defined(__LINUX660)
#include <sys/cachectl.h>
#include <nvtmem.h>
#include <protected/nvtmem_protected.h>
#include <nvtauth.h> //_680_TODO
#endif
#include <sys/mman.h>
#include <fcntl.h>
#if !defined(_NO_NVT_IPC_)
#include <nvtipc.h>
#endif
#elif defined(__ECOS660)
#include <cyg/nvtipc/NvtIpcAPI.h>
#include <cyg/hal/Hal_cache.h>
#include <cyg/compat/uitron/uit_func.h>
#define dma_getCacheAddr(addr) (((addr) & 0x1FFFFFFF) | 0x80000000)
#endif


#if defined(__LINUX) || defined(__ECOS660)
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((UINT32)(UINT8)(ch0) | ((UINT32)(UINT8)(ch1) << 8) | ((UINT32)(UINT8)(ch2) << 16) | ((UINT32)(UINT8)(ch3) << 24 ))
#define SIZE_REGS 0xD30000
#define PAGE_SHIFT 0xC
#define CFG_RTSPNVT_INIT_KEY     MAKEFOURCC('R','T','S','C')
#endif

#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

NvtMediaCliData::NvtMediaCliData()
	: m_pBufVideo(NULL)
	, m_uiBufSizeVideo(0)
	, m_pBufAudio(NULL)
	, m_uiBufSizeAudio(0)
	, m_pRTSPClient(NULL)
{
	memset(&m_Config, 0, sizeof(m_Config));
	memset(&m_IPC, 0, sizeof(m_IPC));
	memset(&m_StrmInfo, 0, sizeof(m_StrmInfo));

#if defined(__LINUX) || defined (__ECOS660)
	SEM_CREATE(hSemIPC, 1);
#endif
	//coverity[uninit_member]
}

NvtMediaCliData::~NvtMediaCliData()
{
#if defined(__LINUX650) || defined(__LINUX660) || defined(__ECOS660) || defined(__LINUX680)
	LIVE555CLI_ICMD Cmd = {
		LIVE555CLI_ICMD_IDX_PROCESS_END,
		0,
		0,
		0,
		0,
		0,
	};
	xRun_ICmd(&Cmd);
	SEM_DESTROY(hSemIPC);

	if (m_IPC.use_ipc) {
		NvtIPC_MsgRel(m_IPC.ipc_msgid);
	}
#endif
}

u_int32_t NvtMediaCliData::xCopyICmdToIPC(LIVE555CLI_ICMD *pCmd)
{
	u_int8_t *pDesc = (u_int8_t *)m_IPC.pBufAddr;
	u_int8_t *pInput = pDesc + sizeof(LIVE555CLI_ICMD);
	u_int8_t *pOutput = pInput + ALIGN_CEIL(pCmd->uiInSize, 32);

	memcpy(pDesc, pCmd, sizeof(LIVE555CLI_ICMD));
	if (pCmd->uiInAddr != 0) {
		if (pCmd->uiInSize != 0) {
			memcpy(pInput, (void *)pCmd->uiInAddr, pCmd->uiInSize);
			((LIVE555CLI_ICMD *)pDesc)->uiInAddr = Trans_To_Phy(pInput);
		} else {
			printf("live555: pCmd->uiInSize=0\r\n");
			return (u_int32_t) - 1;
		}
	}

	if (pCmd->uiOutAddr != 0) {
		if (pCmd->uiOutSize != 0) {
			((LIVE555CLI_ICMD *)pDesc)->uiOutAddr = Trans_To_Phy(pOutput);
		} else {
			printf("live555: pCmd->uiOutSize=0\r\n");
			return (u_int32_t) - 1;
		}
	}

	return 0;
}

u_int32_t NvtMediaCliData::xCopyIPCToICmd(LIVE555CLI_ICMD *pCmd)
{
	u_int8_t *pDesc = (u_int8_t *)m_IPC.pBufAddr;
	u_int8_t *pInput = pDesc + sizeof(LIVE555CLI_ICMD);
	u_int8_t *pOutput = pInput + ALIGN_CEIL(pCmd->uiInSize, 32);

	pCmd->uiResult = ((LIVE555CLI_ICMD *)pDesc)->uiResult;

	if (pCmd->uiOutAddr != 0) {
		if (pCmd->uiResult != 0x80000000) {
			memcpy((void *)pCmd->uiOutAddr, pOutput, pCmd->uiOutSize);
		}
	}

	return 0;
}

void NvtMediaCliData::xRun_ICmd(LIVE555CLI_ICMD *pCmd)
{
	pCmd->uiResult = 0x80000000;
#if (CFG_FAKE_INTERFACE)
	m_Config.fpCmd(pCmd);
#elif defined(__ECOS650)
	m_Config.fpCmd(pCmd);
#elif defined(__LINUX650)
	SEM_WAIT(hSemIPC);

	if (xCopyICmdToIPC(pCmd) != 0) {
		return;
	}

	LIVE555CLI_IPC_MSG msg_snd = {0};
	LIVE555CLI_IPC_MSG msg_rcv = {0};
	msg_snd.mtype = 1;
	msg_snd.uiIPC = pCmd->CmdIdx;
	if (msgsnd(m_IPC.ipc_msgid, &msg_snd, sizeof(msg.uiIPC), IPC_NOWAIT) < 0) {
		printf("err:msgsnd:%d\r\n", (int)pCmd->CmdIdx);
	}
	if (msgrcv(m_IPC.ipc_msgid, &msg_rcv, sizeof(msg.uiIPC), 2, 0) < 0) { //2: means rcv mtype=2
		printf("err:msgrcv\r\n");
	}
	if (msg_rcv.uiIPC != msg_snd.uiIPC) {
		printf("err:msg.uiIPC!=wait %d %d\r\n", (int)msg_snd.uiIPC, (int)msg_rcv.uiIPC);
	}

	if (xCopyIPCToICmd(pCmd) != 0) {
		return;
	}

	SEM_SIGNAL(hSemIPC);
#elif defined(__LINUX660) || defined(__ECOS660) || defined(__LINUX680)
	SEM_WAIT(hSemIPC);

	if (xCopyICmdToIPC(pCmd) != 0) {
		SEM_SIGNAL(hSemIPC);
		return;
	}

	LIVE555CLI_IPC_MSG msg_snd = {0};
	LIVE555CLI_IPC_MSG msg_rcv = {0};
	msg_snd.mtype = 1;
	msg_snd.uiIPC = pCmd->CmdIdx;
	if (NvtIPC_MsgSnd(m_IPC.ipc_msgid, NVTIPC_SENDTO_CORE1, &msg_snd, sizeof(msg_snd)) <= 0) {
		printf("err:msgsnd:%d\r\n", (int)pCmd->CmdIdx);
	}
	if (NvtIPC_MsgRcv(m_IPC.ipc_msgid, &msg_rcv, sizeof(msg_rcv)) <= 0) {
		printf("err:msgrcv\r\n");
	}
	if (msg_rcv.uiIPC != msg_snd.uiIPC) {
		printf("err:msg.uiIPC!=wait %d %d\r\n", (int)msg_snd.uiIPC, (int)msg_rcv.uiIPC);
	}

	if (xCopyIPCToICmd(pCmd) != 0) {
		SEM_SIGNAL(hSemIPC);
		return;
	}

	SEM_SIGNAL(hSemIPC);
#endif
}

u_int32_t NvtMediaCliData::Set_Config(LIVE555CLI_CONFIG *pConfig)
{
#if !defined(WIN32)
	if (pConfig->uiVersion != LIVE555CLI_INTERFACE_VER) {
		printf("live555cli: interface version not matched %08X wanted but %08X\r\n", LIVE555CLI_INTERFACE_VER, (unsigned int)pConfig->uiVersion);
		return (u_int32_t) - 1;
	}
#endif
	if (pConfig->uiWorkingSize < LIVE555CLI_TMP_BUF_SIZE_VIDEO + LIVE555CLI_TMP_BUF_SIZE_AUDIO) {
		printf("live555cli: working buffer size too small %08X need %08X \r\n", (unsigned int)pConfig->uiWorkingSize, LIVE555CLI_TMP_BUF_SIZE_VIDEO + LIVE555CLI_TMP_BUF_SIZE_AUDIO);
		return (u_int32_t) - 1;
	}

	m_Config = *pConfig;
	m_Config.uiWorkingAddr = (UINT32)Trans_Phy_To_NonCache((u_int32_t)m_Config.uiWorkingAddr);

	m_pBufVideo = (u_int8_t *)m_Config.uiWorkingAddr;
	m_uiBufSizeVideo = LIVE555CLI_TMP_BUF_SIZE_VIDEO;
	m_pBufAudio = m_pBufVideo + m_uiBufSizeVideo;
	m_uiBufSizeAudio = LIVE555CLI_TMP_BUF_SIZE_AUDIO;
	return 0;
}

u_int32_t NvtMediaCliData::Set_IPC(u_int32_t addr, u_int32_t size)
{
#if defined(__LINUX650) || defined(__LINUX660) || defined(__ECOS660)  || defined(__LINUX680)
	m_IPC.pBufAddr = (void *)Trans_Phy_To_NonCache(addr);
	m_IPC.BufSize = size;

#if defined(__LINUX650)
	m_IPC.ipc_msgid = msgget(CFG_RTSPNVT_INIT_KEY, IPC_CREAT | 0666);
#elif defined(__LINUX660) || defined(__ECOS660) || defined(__LINUX680)
	m_IPC.ipc_msgid = NvtIPC_MsgGet(NvtIPC_Ftok("RTSPCLINVT"));
#endif
	if (m_IPC.ipc_msgid < 0) {
		printf("id_send failed to msgget\r\n");
	}

	m_IPC.use_ipc = 1;
#endif

	return 0;
}

u_int8_t *NvtMediaCliData::Trans_Phy_To_NonCache(u_int32_t addr)
{
#if (CFG_FAKE_INTERFACE) //fake interface may be the Linux / eCos / PC
	return (u_int8_t *)addr;
#elif defined(__LINUX650) || defined(__LINUX660) ||  defined(__ECOS660)
	return (u_int8_t *)NvtMem_GetNonCacheAddr(addr);
#elif defined(__LINUX680)
	return (u_int8_t *)NvtIPC_GetNonCacheAddr(addr);
#else
	return (u_int8_t *)addr;
#endif
}

u_int8_t *NvtMediaCliData::Trans_Phy_To_Cache(u_int32_t addr)
{
#if (CFG_FAKE_INTERFACE) //fake interface may be the Linux / eCos / PC
	return (u_int8_t *)addr;
#elif defined(__LINUX650) || defined(__LINUX660) ||  defined(__ECOS660)
	return (u_int8_t *)dma_getCacheAddr(addr);
#elif defined(__LINUX680)
	return (u_int8_t *)Trans_Phy_To_NonCache(addr); //_680_TODO: use noncache instead of cache for now
#else
	return (u_int8_t *)addr;
#endif
}

u_int32_t NvtMediaCliData::Trans_To_Phy(void *addr)
{
#if (CFG_FAKE_INTERFACE) //fake interface may be the Linux / eCos / PC
	return (u_int32_t)addr;
#elif defined(__LINUX650) || defined(__LINUX660) ||  defined(__ECOS660)
	return (u_int32_t)NvtMem_GetPhyAddr((u_int32_t)addr);
#elif defined(__LINUX680)
	return (u_int32_t)NvtIPC_GetPhyAddr((u_int32_t)addr);
#else
	return (u_int32_t)addr;
#endif
}

u_int32_t NvtMediaCliData::OpenStream()
{
	u_int32_t hr = 0;

	LIVE555CLI_ICMD Cmd = {
		LIVE555CLI_ICMD_IDX_OPEN_SESSION,
		(UINT32) &m_StrmInfo,
		sizeof(m_StrmInfo),
		0,
		0,
		0,
	};
	xRun_ICmd(&Cmd);
	hr = Cmd.uiResult;
	return hr;
}

u_int32_t NvtMediaCliData::CloseStream()
{
	u_int32_t hr = 0;

	LIVE555CLI_ICMD Cmd = {
		LIVE555CLI_ICMD_IDX_CLOSE_SESSION,
		0,
		0,
		0,
		0,
		0,
	};
	xRun_ICmd(&Cmd);
	hr = Cmd.uiResult;

	return hr;
}

u_int32_t NvtMediaCliData::SetNextVideoFrm(LIVE555CLI_FRM *pFrm)
{
	u_int32_t hr = 0;

	pFrm->uiAddr = Trans_To_Phy((void *)pFrm->uiAddr);

	LIVE555CLI_ICMD Cmd = {
		LIVE555CLI_ICMD_IDX_SET_VIDEO,
		(UINT32)pFrm,
		sizeof(*pFrm),
		0,
		0,
		0,
	};

	xRun_ICmd(&Cmd);
	hr = Cmd.uiResult;
	return hr;
}

u_int32_t NvtMediaCliData::SetNextAudioFrm(LIVE555CLI_FRM *pFrm)
{
	u_int32_t hr = 0;

	pFrm->uiAddr = Trans_To_Phy((void *)pFrm->uiAddr);

	LIVE555CLI_ICMD Cmd = {
		LIVE555CLI_ICMD_IDX_SET_AUDIO,
		(UINT32)pFrm,
		sizeof(*pFrm),
		0,
		0,
		0,
	};

	xRun_ICmd(&Cmd);
	hr = Cmd.uiResult;
	return hr;
}

void NvtMediaCliData::SetRTSPClientPointer(void *pRTSPClient)
{
	m_pRTSPClient = pRTSPClient;
}

u_int8_t *NvtMediaCliData::Get_BufVideo(u_int32_t *p_size)
{
	*p_size = m_uiBufSizeVideo;
	return m_pBufVideo;
}

u_int8_t *NvtMediaCliData::Get_BufAudio(u_int32_t *p_size)
{
	*p_size = m_uiBufSizeAudio;
	return m_pBufAudio;
}

void *NvtMediaCliData::GetRTSPClientPointer(void)
{
	return m_pRTSPClient;
}

LIVE555CLI_CONFIG *NvtMediaCliData::Get_Config()
{
	return &m_Config;
}

char *NvtMediaCliData::Get_Url()
{
	return m_Config.szUrl;
}

NVT_MEDIA_TYPE NvtMediaCliData::Get_MediaType(char const *fMediumName)
{
	if (strcmp(fMediumName, "video") == 0) {
		return NVT_MEDIA_TYPE_VIDEO;
	} else if (strcmp(fMediumName, "audio") == 0) {
		return NVT_MEDIA_TYPE_AUDIO;
	} else if (strcmp(fMediumName, "application") == 0) {
		return NVT_MEDIA_TYPE_META;
	}

	printf("unknown media type: %s\r\n", fMediumName);
	return NVT_MEDIA_TYPE_META;
}


LIVE555CLI_CODEC_VIDEO NvtMediaCliData::Trans_VideoCodec(const char *streamId)
{
	if (strcmp(streamId, "H264") == 0) {
		return LIVE555CLI_CODEC_VIDEO_H264;
	} else if (strcmp(streamId, "JPEG") == 0) {
		return LIVE555CLI_CODEC_VIDEO_MJPG;
	} else if (strcmp(streamId, "H265") == 0) {
		return LIVE555CLI_CODEC_VIDEO_H265;
	}

	printf("live555cli: unknown video codec: %s\n", streamId);
	return LIVE555CLI_CODEC_VIDEO_UNKNOWN;
}

LIVE555CLI_CODEC_AUDIO NvtMediaCliData::Trans_AudioCodec(const char *streamId)
{
	if (strcmp(streamId, "L16") == 0) {
		return LIVE555CLI_CODEC_AUDIO_PCM;
	} else if (strcmp(streamId, "MPEG4-GENERIC") == 0) {
		return LIVE555CLI_CODEC_AUDIO_AAC;
	} else if (strcmp(streamId, "PCMU") == 0) {
		return LIVE555CLI_CODEC_AUDIO_G711_ULAW;
	} else if (strcmp(streamId, "PCMA") == 0) {
		return LIVE555CLI_CODEC_AUDIO_G711_ALAW;
	}
	return LIVE555CLI_CODEC_AUDIO_UNKNOWN;
}

u_int32_t NvtMediaCliData::Set_VideoInfo(LIVE555CLI_VIDEO_INFO *pInfo)
{
	m_StrmInfo.vid = *pInfo;
	return 0;
}
u_int32_t NvtMediaCliData::Set_AudioInfo(LIVE555CLI_AUDIO_INFO *pInfo)
{
	m_StrmInfo.aud = *pInfo;
	return 0;
}
