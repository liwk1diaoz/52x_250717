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

#include "NvtStreamQueue.hh"
#include "GroupsockHelper.hh"
#include "NvtMgr.hh"
#include <stdio.h>

#define CFG_DEBUG_QUQUE 0

#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define NVT_RTP_HDR_SIZE_REVERSED ALIGN_CEIL(NVT_RTP_HDR_SIZE_MJPG, 16) //use max size to be the reversed size
#define CFG_VIDEO_MEM_SIZE 0x40000 //128K align
#define CFG_AUDIO_MEM_SIZE 0x10000 //8K align

# if(CFG_DEBUG_QUQUE)
static int push_cnt = 0;
static int unlock_cnt = 0;
#endif

NvtStreamQueue::NvtStreamQueue()
	: m_quit(0)
	, m_mem_align_size(CFG_VIDEO_MEM_SIZE)
{
	SEM_CREATE(m_sem_api, 1);
	//coverity[uninit_use_in_call]
	SEM_WAIT(m_sem_api);

	SEM_CREATE(m_sem, 2);
	memset(&m_mem, 0, sizeof(m_mem));
	memset(&m_pts_ofs, 0, sizeof(m_pts_ofs));
	// make ring buffer
	m_mem[0].p_next = &m_mem[1];
	m_mem[1].p_next = &m_mem[0];
	// init frame index
	m_plock = &m_mem[0];
	m_ppull = &m_mem[0];

	SEM_SIGNAL(m_sem_api);
	//coverity[uninit_member]
}

NvtStreamQueue::~NvtStreamQueue()
{
	SEM_WAIT(m_sem_api);

	for (int i = 0; i < 2; i++) {
		if (m_mem[i].p_buf) {
			delete[] m_mem[i].p_buf;
			m_mem[i].p_buf = NULL;
		}
		m_mem[i].total_size = 0;
		m_mem[i].valid_size = 0;
	}

	SEM_DESTROY(m_sem);
	SEM_SIGNAL(m_sem_api);
	SEM_DESTROY(m_sem_api);
}

int NvtStreamQueue::open(NVT_CODEC_TYPE codec_type)
{
	SEM_WAIT(m_sem_api);
	switch (codec_type)
	{
	case NVT_CODEC_TYPE_MJPG:
	case NVT_CODEC_TYPE_H264:
	case NVT_CODEC_TYPE_H265:
		m_mem_align_size = CFG_VIDEO_MEM_SIZE;
		break;
	case NVT_CODEC_TYPE_PCM:
	case NVT_CODEC_TYPE_AAC:
	case NVT_CODEC_TYPE_G711_ALAW:
	case NVT_CODEC_TYPE_G711_ULAW:
	case NVT_CODEC_TYPE_META:
		m_mem_align_size = CFG_AUDIO_MEM_SIZE;
		break;
	default:
		printf("live555: NvtStreamQueue::open cannot handle: %d\r\n", codec_type);
		break;
	}
	SEM_SIGNAL(m_sem_api);
	return 0;
}

int NvtStreamQueue::close()
{
	SEM_WAIT(m_sem_api);
	m_quit = 1;
	SEM_SIGNAL(m_sem);
	SEM_SIGNAL(m_sem_api);
	return 0;
}


int NvtStreamQueue::reset()
{
	SEM_WAIT(m_sem_api);
	for (int i = 0; i < 2; i++) {
		if (m_mem[i].state != MEM_STATE_EMPTY) {
			m_mem[i].state = MEM_STATE_EMPTY;
			SEM_SIGNAL(m_sem);
		}
	}
	// init frame index
	m_plock = &m_mem[0];
	m_ppull = &m_mem[0];
	// reset pts
	m_pts_ofs.tv_sec = 0;
	m_pts_ofs.tv_usec = 0;
	SEM_SIGNAL(m_sem_api);
	return 0;
}

NvtStreamQueue::MEM_HANDLE NvtStreamQueue::lock()
{
	SEM_WAIT(m_sem);
	SEM_WAIT(m_sem_api);
	if (m_quit) {
		SEM_SIGNAL(m_sem_api);
		return NULL;
	}
	// check next is empty state
	MEM *p_next = (MEM *)m_plock->p_next;
	if (p_next->state != MEM_STATE_EMPTY) {
		printf("live555: error lock state = %d\r\n", p_next->state);
		SEM_SIGNAL(m_sem_api);
		return NULL;
	}
	m_plock = p_next;
	m_plock->state = MEM_STATE_LOCKED;
	SEM_SIGNAL(m_sem_api);
	return m_plock;
}

int NvtStreamQueue::push(MEM_HANDLE handle, NVT_STRM_INFO *p_strm)
{
	SEM_WAIT(m_sem_api);
#if (CFG_DEBUG_QUQUE)
	printf("push:%d\n", push_cnt++);
#endif

	MEM *p_mem = (MEM *)handle;
	if (p_mem->state != MEM_STATE_LOCKED) {
		printf("live555: error push state = %d\r\n", p_mem->state);
		SEM_SIGNAL(m_sem_api);
		return -1;
	}

	p_mem->strm_info = *p_strm;
	if (p_mem->total_size < p_strm->size + NVT_RTP_HDR_SIZE_REVERSED) {
		if (p_mem->p_buf) {
			delete[] p_mem->p_buf;
			p_mem->p_buf = NULL;
		}
		p_mem->total_size = ALIGN_CEIL(p_strm->size + NVT_RTP_HDR_SIZE_REVERSED, m_mem_align_size);
		p_mem->p_buf = new u_int8_t[p_mem->total_size];
		if (p_mem->p_buf == NULL) {
			printf("live555/push: alloc mem %d failed.\r\n", (int)p_mem->total_size);
			SEM_SIGNAL(m_sem_api);
			return -1;
		}
	}

	if (m_pts_ofs.tv_sec == 0 && m_pts_ofs.tv_usec == 0) {
		gettimeofday(&m_pts_ofs, NULL);
		m_pts_ofs.tv_sec -= p_strm->tm.tv_sec;
		m_pts_ofs.tv_usec -= p_strm->tm.tv_usec;
	}

	u_int8_t *p = p_mem->p_buf + NVT_RTP_HDR_SIZE_REVERSED;
	memcpy(p, (void *)p_strm->addr, p_strm->size);
	p_mem->valid_size = p_strm->size;
	p_mem->state = MEM_STATE_PUSHED;
	gettimeofday(&p_mem->strm_info.latency.push_done, NULL);
	SEM_SIGNAL(m_sem_api);
	return 0;
}

NvtStreamQueue::MEM_HANDLE NvtStreamQueue::pull()
{
	SEM_WAIT(m_sem_api);
	MEM *p_next = (MEM *)m_ppull->p_next;
	if (p_next->state != MEM_STATE_PUSHED) {
		printf("live555: error pull state = %d\r\n", p_next->state);
		SEM_SIGNAL(m_sem_api);
		return NULL;
	}
	m_ppull = p_next;
	m_ppull->state = MEM_STATE_PULLED;
	gettimeofday(&m_ppull->strm_info.latency.pull_done, NULL);
	SEM_SIGNAL(m_sem_api);
	return m_ppull;
}

int NvtStreamQueue::unlock(MEM_HANDLE handle)
{
	SEM_WAIT(m_sem_api);
#if (CFG_DEBUG_QUQUE)
	printf("unlock:%d\n", unlock_cnt++);
#endif
	MEM *p_unlock = (MEM *)handle;
	if (p_unlock->state != MEM_STATE_PULLED) {
		printf("live555: error unlock state = %d\r\n", p_unlock->state);
		SEM_SIGNAL(m_sem_api);
		return -1;
	}
	p_unlock->state = MEM_STATE_EMPTY;
	gettimeofday(&p_unlock->strm_info.latency.unlock_done, NULL);

	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	if (pNvtMgr != NULL) {
		if ((pNvtMgr->IsShowLatencyV() && p_unlock->strm_info.type == NVT_MEDIA_TYPE_VIDEO) ||
			(pNvtMgr->IsShowLatencyA() && p_unlock->strm_info.type == NVT_MEDIA_TYPE_AUDIO)) {
			struct timeval ofs;
			NVT_DBG_LATENCY latency = p_unlock->strm_info.latency;
			pNvtMgr->GetPtsOfs(&ofs);
			long wait_time_sec = latency.wait_done.tv_sec - latency.begin_wait.tv_sec;
			long wait_time_usec = latency.wait_done.tv_usec - latency.begin_wait.tv_usec;
			latency.wait_done.tv_sec -= (ofs.tv_sec + p_unlock->strm_info.tm.tv_sec);
			latency.wait_done.tv_usec -= (ofs.tv_usec + +p_unlock->strm_info.tm.tv_usec);
			latency.push_done.tv_sec -= p_unlock->strm_info.latency.wait_done.tv_sec;
			latency.push_done.tv_usec -= p_unlock->strm_info.latency.wait_done.tv_usec;
			latency.pull_done.tv_sec -= p_unlock->strm_info.latency.push_done.tv_sec;
			latency.pull_done.tv_usec -= p_unlock->strm_info.latency.push_done.tv_usec;
			latency.unlock_done.tv_sec -= p_unlock->strm_info.latency.pull_done.tv_sec;
			latency.unlock_done.tv_usec -= p_unlock->strm_info.latency.pull_done.tv_usec;
			printf("(%d) %d %d %d %d\r\n",
				(int)((wait_time_sec * 1000000 + wait_time_usec) /1000),
				(int)((latency.wait_done.tv_sec * 1000000 + latency.wait_done.tv_usec) / 1000),
				(int)((latency.push_done.tv_sec * 1000000 + latency.push_done.tv_usec) / 1000),
				(int)((latency.pull_done.tv_sec * 1000000 + latency.pull_done.tv_usec) / 1000),
				(int)((latency.unlock_done.tv_sec * 1000000 + latency.unlock_done.tv_usec) / 1000));
		}
	}
	SEM_SIGNAL(m_sem);
	SEM_SIGNAL(m_sem_api);
	return 0;
}

int NvtStreamQueue::get_buf(MEM_HANDLE handle, BUF_INFO *p_info)
{
	memset(p_info, 0, sizeof(BUF_INFO));
	MEM *p_mem = (MEM *)handle;
	if (p_mem->p_buf == NULL ||
		p_mem->valid_size == 0) {
		return -1;
	}
	p_info->p_buf = p_mem->p_buf + NVT_RTP_HDR_SIZE_REVERSED;
	p_info->size = p_mem->valid_size;
	p_info->strm_info = p_mem->strm_info;
	p_info->pts.tv_sec = p_info->strm_info.tm.tv_sec + m_pts_ofs.tv_sec;
	p_info->pts.tv_usec = p_info->strm_info.tm.tv_usec + m_pts_ofs.tv_usec;
	if (p_info->pts.tv_usec > 1000000) {
		p_info->pts.tv_sec++;
		p_info->pts.tv_usec -= 1000000;
	} else if (p_info->pts.tv_usec < 0) {
		p_info->pts.tv_sec--;
		p_info->pts.tv_usec += 1000000;
	}
	return 0;
}
