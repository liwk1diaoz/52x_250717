/**
	@brief Source code of image application movie player module.\n
	This file contains the functions which is related to image application movie player in the chip.

	@file iamovieplay_tsk.c

	@ingroup mlib

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include <kwrap/util.h>
#include "iamovieplay_tsk.h"
#include "filein.h"
#include "hd_filein_lib.h"
#include "ImageApp/ImageApp_MoviePlay.h"
#include "nmediaplay_api.h"
#include "kflow_common/nvtmpp.h"
#include "GxVideoFile.h"

#define _TODO_          0
#define DBGINFO_BUFSIZE()	(0x200)
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
#define DECRTPTO_SIZE    256
/**
	Task priority
*/
#define IAMOVIEPLAY_DISP_PRI           7
#define IAMOVIEPLAY_DEMUX_PRI          8
#define IAMOVIEPLAY_READ_PRI           16
#define IAMOVIEPLAY_AUDIO_PB_PRI       9
#define IAMOVIEPLAY_AUDIO_DECODE_PRI   9

/**
	Stack size
*/
#define IAMOVIEPLAY_DISP_STKSIZE         8192//4096//2048
#define IAMOVIEPLAY_DEMUX_STKSIZE        4096//2048
#define IAMOVIEPLAY_READ_STKSIZE         4096//1024
#define IAMOVIEPLAY_AUDIO_PB_STKSIZE     4096
#define IAMOVIEPLAY_AUDIO_DECODE_STKSIZE 4096

/**
 * Decrypt
*/
#define IAMOVIEPLAY_DECRYPT_BS_OFFSET	16
#define IAMOVIEPLAY_DECRYPT_SIZE 256

/**
	Task ID
*/
THREAD_HANDLE  IAMOVIEPLAY_DEMUX_TSK_ID = 0;
THREAD_HANDLE  IAMOVIEPLAY_READ_TSK_ID = 0;
#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
THREAD_HANDLE  IAMOVIEPLAY_DISP_TSK_ID = 0;
#endif
THREAD_HANDLE  IAMOVIEPLAY_AUDIO_PB_TSK_ID = 0;
THREAD_HANDLE  IAMOVIEPLAY_AUDIO_DECODE_TSK_ID = 0;
///// Noncross file variables
static volatile UINT32 movieplay_demux_tsk_run, is_movieplay_demux_tsk_running, movieplay_audio_playback_tsk_run, movieplay_audio_decode_tsk_run;
static volatile UINT32 movieplay_filein_read_tsk_run, is_movieplay_filein_read_tsk_running, is_movieplay_audio_pb_tsk_running, is_movieplay_audio_decode_tsk_running;

static ER _iamovieplay_IFrameInOneShot(void);

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
static UINT32 movieplay_disp_tsk_run, is_movieplay_disp_tsk_running;
#endif

/************************************************************************************
 * Static function
 ************************************************************************************/
static UINT32 _iamovieplay_GetAudFrmIdxByVideo(UINT32 vdo_idx);

/**
	Flag ID
*/
ID             IAMOVIEPLAY_DEMUX_FLG_ID = 0;
ID             IAMOVIEPLAY_READ_FLG_ID = 0;
#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
ID             IAMOVIEPLAY_DISP_FLG_ID = 0;
#endif
ID             IAMOVIEPLAY_AUDIO_PB_FLG_ID     = 0;
ID             IAMOVIEPLAY_AUDIO_DECODE_FLG_ID = 0;

extern ID IAMOVIEPLAY_SEMID_DECRYPT;

///// Cross file variables
//ID IAMOVIE_FLG_ID = 0;

/**
	global object
*/
IAMOVIEPLAY_TSK iamovieplay_tsk_obj = {0};
IAMOVIEPLAY_BSDEMUX_BS_INFO g_vdo_bsdemux_cbinfo = {0};
IAMOVIEPLAY_BSDEMUX_BS_INFO g_aud_bsdemux_cbinfo = {0};

UINT32 g_movply_vdodec_blk_phy_adr = 0;
UINT32 g_movply_vdodec_blk_va_adr  = 0;

UINT32 g_movie_play_preload = 0;
BOOL   g_bForceFirstAudioPB = FALSE;
static BOOL   g_bReloadingFileReadBlock = FALSE;
static BOOL   g_movie_finish_event = FALSE;


/* debug level by function, can be ored */
static UINT32 g_movply_dbglvl = 0;

/* audio debug */
#define IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE 0 /* record two audio files(encoded / decoded) can be played on PC */

/* audio define */
#define IAMMOVIEPLAY_ADTS_HEADER_SIZE 7

#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE
FST_FILE g_movply_dbg_audio_file_encoded = NULL; /* before hdal audio decode */
FST_FILE g_movply_dbg_audio_file_decoded = NULL; /* before hdal audio out */

static void  addADTStoPacket(UINT8* packet, int packetLen, UINT32 chs, UINT32 sr) {
    int profile = 2; // AAC
    int freqIdx = 0x8; // 16 KHz
    int chanCfg = 2;
    int header_size = IAMMOVIEPLAY_ADTS_HEADER_SIZE; /* protection_absent = 1, header len is 7 bytes */

    packetLen += header_size;

    /* header example */
//    static UINT8 header[IAMMOVIEPLAY_ADTS_HEADER_SIZE] = {0xff, 0xf1, 0x60, 0x40, 0x40, 0xff, 0xfc};

    switch(sr)
    {
    case 96000:
    	freqIdx = 0x0;
    	break;

    case 88200:
    	freqIdx = 0x1;
    	break;

    case 64000:
    	freqIdx = 0x2;
    	break;

    case 48000:
    	freqIdx = 0x3;
    	break;

    case 44100:
    	freqIdx = 0x4;
    	break;

    case 32000:
    	freqIdx = 0x5;
    	break;

    case 24000:
    	freqIdx = 0x6;
    	break;

    case 22050:
    	freqIdx = 0x7;
    	break;

    case 16000:
    	freqIdx = 0x8;
    	break;

    case 12000:
    	freqIdx = 0x9;
    	break;

    case 11025:
    	freqIdx = 0xa;
    	break;

    case 8000:
    	freqIdx = 0xb;
    	break;

    case 7350:
    	freqIdx = 0xc;
    	break;

    default:
    	DBG_ERR("Unknown aud sr(%lu), reset index to 0x8(16000) \r\n", sr);
    	freqIdx = 0x8;
    	break;
    }


    switch(chs)
    {
    case 1:
    	chanCfg = 1;
    	break;

    case 2:
    	chanCfg = 2;
    	break;

    default:
    	DBG_ERR("Unknown aud chanel cfg(%lu), reset index to 1 \r\n", chs);
    	chanCfg = 1;
    	break;

    }

    /* fill in ADTS data */
    packet[0] = 0xFF;
    packet[1] = 0xF1; /* protection_absent = 1, header len is 7 bytes */
    packet[2] = (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    packet[3] = (((chanCfg & 3) << 6) + (packetLen >> 11));
    packet[4] = ((packetLen & 0x7FF) >> 3);
    packet[5] = (((packetLen & 7) << 5) + 0x1F);
    packet[6] = 0xFC;

}
#endif

/* calculate decrypt length */
static UINT32 _iamovieplay_cal_decrypt_size(UINT32 bs_size)
{
	UINT32 crypto_size = 0;

	if (bs_size < IAMOVIEPLAY_DECRYPT_BS_OFFSET) {
		crypto_size = 0;
	} else if (bs_size > (IAMOVIEPLAY_DECRYPT_BS_OFFSET + IAMOVIEPLAY_DECRYPT_SIZE)) {
		crypto_size = IAMOVIEPLAY_DECRYPT_SIZE;
	} else {
		crypto_size = ALIGN_FLOOR_16(bs_size - IAMOVIEPLAY_DECRYPT_BS_OFFSET);
	}
	return crypto_size;
}

/* common decrypt function, decrypt size is assigned by user */
void _iamovieplay_decrypt(UINT32 addr, UINT32 decrypt_size)
{
	if(decrypt_size){

		if(IAMOVIEPLAY_SEMID_DECRYPT == 0){

			int ret;

			if ((ret = vos_sem_create(&IAMOVIEPLAY_SEMID_DECRYPT, 1, "IAMPLAY_SEM_DECRYPT")) != E_OK) {
				DBG_ERR("Create IAMOVIEPLAY_SEMID_DECRYPT fail(%d)\r\n", ret);
			}
		}

		vos_sem_wait(IAMOVIEPLAY_SEMID_DECRYPT);
		GxVideoFile_Decrypto(addr, decrypt_size);
		vos_sem_sig(IAMOVIEPLAY_SEMID_DECRYPT);
	}
}

/* decrypt function for bitstream, decrypt size is calculated internally */
void _iamovieplay_bs_decrypt(UINT32 addr, UINT32 bs_size)
{
	UINT32 decrypt_size = _iamovieplay_cal_decrypt_size(bs_size);

	_iamovieplay_decrypt(addr + IAMOVIEPLAY_DECRYPT_BS_OFFSET, decrypt_size);
}

static BOOL _iamovieplay_is_dbg_decrypt(void)
{
	return (g_movply_dbglvl & IAMMOVIEPLAY_DBG_DECRYPT);
}

void _iamovieplay_decrypt_hex_dump(MOVIEPLAY_DECRYPT_TYPE type, char*tag, UINT32 buf, UINT16 len)
{
	if(FALSE == _iamovieplay_is_dbg_decrypt()){
		return;
	}

	if(type == MOVIEPLAY_DECRYPT_TYPE_CONTAINER){
		IAMMOVIEPLAY_DECRYPT_DUMP("[Container:%s] ", tag);
	}
	else if(type == MOVIEPLAY_DECRYPT_TYPE_I_FRAME){
		IAMMOVIEPLAY_DECRYPT_DUMP("[I frame  :%s] ", tag);
	}
	else if(type == MOVIEPLAY_DECRYPT_TYPE_P_FRAME){
		IAMMOVIEPLAY_DECRYPT_DUMP("[P frame  :%s] ", tag);
	}
	else if(type == MOVIEPLAY_DECRYPT_TYPE_AUDIO){
		IAMMOVIEPLAY_DECRYPT_DUMP("[Audio    :%s] ", tag);
	}
	else{
		return;
	}

	buf += + IAMOVIEPLAY_DECRYPT_BS_OFFSET;

	UINT8* ptr = (UINT8*)buf;

	for(int i=0 ; i<len ; i++)
	{
		IAMMOVIEPLAY_DECRYPT_DUMP("%02X", ptr[i]);
	}

	IAMMOVIEPLAY_DECRYPT_DUMP("\r\n");
}

static void _iamovieplay_update_start_code(UINT8* bs_addr, UINT32 bs_size, UINT32 desc_size)
{
	UINT8 *ptr8 = bs_addr;

	ptr8 = ptr8 + desc_size;

	/* update start code, IVOT_N00028-343 support multi-tile */
	UINT32 tile_size = 0;
	UINT32 offset = 0;
	UINT32 bs_size_count = 0;
	UINT8 tile_cnt = 0;

	do {

		// check IDR type
		if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) {
			tile_size = (((*ptr8) << 24) | ((*(ptr8 + 1))<<16) | ((*(ptr8 + 2))<< 8) | (*(ptr8 + 3)));

			offset = (4 + tile_size);
			bs_size_count += offset;

			*ptr8 = 0x00;
			*(ptr8+1) = 0x00;
			*(ptr8+2) = 0x00;
			*(ptr8+3) = 0x01;

			ptr8 += offset;
			tile_cnt++;
		}
		else{
			DBG_ERR("Get IDR type(0x%08x) error! desc_size=%d\r\n", *(ptr8 + 4), desc_size);
			break;
		}

	} while(bs_size_count < (bs_size - desc_size) );


	IAMMOVIEPLAY_COMM_DUMP("tile_cnt=%u\r\n", tile_cnt);
}

//static UINT32 g_first_trigger = 0;
static void _iamovieplay_TimerHdl(void)
{
#if 1//_TODO_
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;

	#if 1
	if (/*!g_first_trigger++ &&*/ tsk_obj->vdo_idx < tsk_obj->vdo_ttfrm && tsk_obj->state == IAMOVIEPLAY_STATE_PLAYING)	{
	#else
	if (/*!g_first_trigger++ &&*/ tsk_obj->vdo_idx <= 215 && tsk_obj->aud_idx < tsk_obj->aud_ttfrm && tsk_obj->state == IAMOVIEPLAY_STATE_PLAYING)	{
	#endif
		if (tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
			// trigger filein
			if (tsk_obj->file_fmt != IAMOVIEPLAY_FILEFMT_TS) {
				set_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_TRIG);
			}

			// trigger bsdemux audio bs
			#if 0
			if(tsk_obj->aud_en && (tsk_obj->aud_idx < tsk_obj->aud_ttfrm)) {
				set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_ABS_DEMUX);
			}
			#endif
			// trigger bsdemux video bs
			set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_VBS_DEMUX);

			// callback play sec to notify UI.
			if (tsk_obj->gop) {

				if (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS) {

					UINT32 mod;

					if(tsk_obj->speed > IAMOVIEPLAY_SPEED_2X){
						mod = tsk_obj->gop;
					}
					else{
						mod = tsk_obj->vdo_fr;
					}

					if (!(tsk_obj->vdo_idx % mod)) {
						// Callback 1st media info to UI
						if (p_play_info->event_cb) {
							(p_play_info->event_cb)(
									MOVIEPLAY_EVENT_ONE_SECOND,
									(tsk_obj->vdo_idx / (tsk_obj->vdo_fr)),
									tsk_obj->vdo_idx,
									tsk_obj->gop);
						}
					}
				}
				else{
					if (!(tsk_obj->vdo_idx % (tsk_obj->vdo_fr))) {
						// Callback 1st media info to UI
						if (p_play_info->event_cb) {
							(p_play_info->event_cb)(
									MOVIEPLAY_EVENT_ONE_SECOND,
									(tsk_obj->vdo_idx / (tsk_obj->vdo_fr)),
									tsk_obj->vdo_idx,
									tsk_obj->gop);
						}
					}
				}
			}
		} else { // for forward/rewind
			// Callback 1st media info to UI
			if (tsk_obj->vdo_frm_eof && p_play_info->event_cb && (g_movie_finish_event == FALSE)) {
				g_movie_finish_event = TRUE;
				(p_play_info->event_cb)(MOVIEPLAY_EVENT_STOP, 0, 0, 0);
			} else {
				set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IFRM_ONESHOT);

				// callback play sec to notify UI.
				if (tsk_obj->gop) {

					if (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS) {

						UINT32 mod;

						if(tsk_obj->speed > IAMOVIEPLAY_SPEED_2X){
							mod = tsk_obj->gop;
						}
						else{
							mod = tsk_obj->vdo_fr;
						}

						if (!(tsk_obj->vdo_idx % mod)) {
							// Callback 1st media info to UI
							if (p_play_info->event_cb) {
								(p_play_info->event_cb)(
										MOVIEPLAY_EVENT_ONE_SECOND,
										(tsk_obj->vdo_idx / (tsk_obj->vdo_fr)),
										tsk_obj->vdo_idx,
										tsk_obj->gop);
							}
						}
					}
					else{

						UINT32 mod;

						if(tsk_obj->speed > IAMOVIEPLAY_SPEED_2X){
							mod = tsk_obj->gop;
						}
						else{
							mod = tsk_obj->vdo_fr;
						}

						if (!(tsk_obj->vdo_idx % mod)) {
							// Callback 1st media info to UI
							if (p_play_info->event_cb) {
								(p_play_info->event_cb)(
										MOVIEPLAY_EVENT_ONE_SECOND,
										(tsk_obj->vdo_idx / (tsk_obj->vdo_fr)),
										tsk_obj->vdo_idx,
										tsk_obj->gop);
							}
						}
					}
				}
			}
		}
	} else if ((tsk_obj->vdo_idx == tsk_obj->vdo_ttfrm) && (g_movie_finish_event == FALSE)) {
		g_movie_finish_event = TRUE;
		// Callback 1st media info to UI
		if (p_play_info->event_cb) {
			(p_play_info->event_cb)(MOVIEPLAY_EVENT_STOP, 0, 0, 0);
		}
	}
#endif
}

static ER _iamovieplay_TimerTriggerStart(void)
{
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;

	IAMMOVIEPLAY_COMM_DUMP("start timer trig\r\n");

    if (tsk_obj->timer_open) {
		DBG_WRN("timer trig already opened\r\n");
		return E_OK;
	}

	if (timer_open((TIMER_ID *)&tsk_obj->timer_id, (DRV_CB)_iamovieplay_TimerHdl) != E_OK) {
		DBG_ERR("timer trig open failed!\r\n");
		return E_SYS;
	}

	if (tsk_obj->timer_intval == 0) {
		tsk_obj->timer_intval = IAMOVIEPLAY_TIMER_INTVAL_DEFAULT;
		DBG_WRN("do not set timer_intval, use default %lu us\r\n", tsk_obj->timer_intval);
	}

	//DBGD(tsk_obj->timer_intval);
	timer_cfg(tsk_obj->timer_id, tsk_obj->timer_intval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	tsk_obj->timer_open = TRUE;

	return E_OK;
}

static void _iamovieplay_TimerTriggerStop(void)
{
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;

	IAMMOVIEPLAY_COMM_DUMP("stop timer trig\r\n");

	if (tsk_obj->timer_open) {
		ER ret;
		ret = timer_close(tsk_obj->timer_id);
		if(ret != E_OK) {
			DBG_ERR("timer_close Fail(%d)\r\n", ret);
		}
		tsk_obj->timer_open = FALSE;
	}
}

static ER _iamovieplay_Pause(void)
{
	FLGPTN flag = 0;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;

	tsk_obj->state = IAMOVIEPLAY_STATE_PAUSE;
    _iamovieplay_TimerTriggerStop();

	DBG_IND("bsdmx idle ...\r\n");
    wai_flg(&flag, IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE, TWF_ORW);
	DBG_IND("bsdmx idle ... ok\r\n");


	DBG_IND("fr idle ...\r\n");
    wai_flg(&flag, IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_IDLE, TWF_ORW);
	DBG_IND("fr idle ... ok\r\n");

	if (tsk_obj->aud_en) {
		// audio Playback system un-initial
		DBG_IND("audio PB pause...\r\n");
		vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_PAUSE);
		wai_flg(&flag, IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_IDLE, TWF_ORW);
		DBG_IND("audio PB pause ... ok\r\n");
	}

	/* reset audio bs demux info */
	g_aud_bsdemux_cbinfo.bs_addr_va = 0;
	g_aud_bsdemux_cbinfo.bs_addr = 0;
	g_aud_bsdemux_cbinfo.bs_size = 0;

//	tsk_obj->state = IAMOVIEPLAY_STATE_PAUSE;

	//#NT#2018/10/26#Adam Su -begin
	//#NT#AUTO_TEST
	exam_msg("pause play\r\n");
	//#NT#2018/10/26#Adam Su -end

    return E_OK;
}

static ER _iamovieplay_Stop(void)
{
	FLGPTN flag = 0;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;

    _iamovieplay_TimerTriggerStop();

	DBG_IND("bsdmx idle ...\r\n");
    wai_flg(&flag, IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE, TWF_ORW);
	DBG_IND("bsdmx idle ... ok\r\n");


	DBG_IND("fr idle ...\r\n");
    wai_flg(&flag, IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_IDLE, TWF_ORW);
	DBG_IND("fr idle ... ok\r\n");

	tsk_obj->state = IAMOVIEPLAY_STATE_STOP;

	//#NT#2018/10/26#Adam Su -begin
	//#NT#AUTO_TEST
	exam_msg("stop play\r\n");
	//#NT#2018/10/26#Adam Su -end

    return E_OK;
}

static ER _iamovieplay_ChangeSpeed(IAMOVIEPLAY_SPEED_TYPE speed, IAMOVIEPLAY_DIRECT_TYPE direct)
{
	UINT32 timer_intval, slow_factor;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

	if (!tsk_obj->vdo_en) {
		DBG_ERR("video is disable\r\n");
		return E_SYS;
	}
	if (p_obj->vdo_codec == IAMOVIEPLAY_DEC_MJPG) {
		DBG_ERR("fast forward/backward is not support on mjpg\r\n");
		return E_SYS;
	}

	// stop timer first
	_iamovieplay_Pause();
	vos_util_delay_ms(5);

	tsk_obj->speed = speed;
	tsk_obj->direct = direct;

	if (tsk_obj->vdo_fr == 0) {
		DBG_ERR("vdo_fr = 0\r\n");
		return E_SYS;
	}
	if (tsk_obj->gop == 0) {
		DBG_ERR("gop = 0\r\n");
		return E_SYS;
	}

	DBGD(tsk_obj->speed);

	// set timer interval by speed level
	switch (tsk_obj->speed) {
	case IAMOVIEPLAY_SPEED_NORMAL:
		if (tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
			timer_intval = IAMOVIEPLAY_US_IN_SECOND / tsk_obj->vdo_fr;
		} else if (tsk_obj->direct == IAMOVIEPLAY_DIRECT_BACKWARD) {
			timer_intval = IAMOVIEPLAY_US_IN_SECOND / (tsk_obj->vdo_fr / tsk_obj->gop);
		} else {
			DBG_ERR("Invalid direct=%d\r\n", tsk_obj->direct);
			return E_PAR;
		}
		break;
	case IAMOVIEPLAY_SPEED_2X:
	case IAMOVIEPLAY_SPEED_4X:
	case IAMOVIEPLAY_SPEED_8X:
	case IAMOVIEPLAY_SPEED_16X:
	case IAMOVIEPLAY_SPEED_32X:
	case IAMOVIEPLAY_SPEED_64X:
		if ((tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_MOV) || (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_MP4)) {
			if (tsk_obj->speed <= IAMOVIEPLAY_SPEED_4X) { // for performance (display), up to 8x to skip some I-frame
				timer_intval = IAMOVIEPLAY_US_IN_SECOND / (tsk_obj->speed * tsk_obj->vdo_fr / tsk_obj->gop);
			} else {
				timer_intval = IAMOVIEPLAY_US_IN_SECOND / (IAMOVIEPLAY_SPEED_4X * tsk_obj->vdo_fr / tsk_obj->gop);
			}
		} else if (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS) {
			timer_intval = IAMOVIEPLAY_US_IN_SECOND / (IAMOVIEPLAY_SPEED_2X * tsk_obj->vdo_fr / tsk_obj->gop);
		} else {
			DBG_ERR("invalid file_fmt = %d\r\n", tsk_obj->file_fmt);
			return E_PAR;
		}
		break;
	case IAMOVIEPLAY_SPEED_1_2X:
	case IAMOVIEPLAY_SPEED_1_4X:
	case IAMOVIEPLAY_SPEED_1_8X:
	case IAMOVIEPLAY_SPEED_1_16X:
		slow_factor = IAMOVIEPLAY_US_IN_SECOND / tsk_obj->speed;
		timer_intval = (IAMOVIEPLAY_US_IN_SECOND * slow_factor) / (tsk_obj->vdo_fr / tsk_obj->gop);
		break;
	default:
		DBG_ERR("invalid speed=%d\r\n", tsk_obj->speed);
		return E_PAR;
	}

	tsk_obj->timer_intval = timer_intval;
	IAMMOVIEPLAY_COMM_DUMP("change speed, timer_intval=%d (us)\r\n", tsk_obj->timer_intval);

	// for pf-autotest
	if (tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
		exam_msg("speed= %d\r\n", tsk_obj->speed);
		DBG_IND("speed= %d\r\n", tsk_obj->speed);
	} else if (tsk_obj->direct == IAMOVIEPLAY_DIRECT_BACKWARD) {
		exam_msg("speed= -%d\r\n", tsk_obj->speed);
		DBG_IND("speed= -%d\r\n", tsk_obj->speed);
	}

	return E_OK;
}

static ER _iamovieplay_Seek(UINT32 sec)
{

#if 1
	HD_PATH_ID path_id = 0;
	UINT32 totalSec = 0, tmp1, tmp2, result;
	UINT64 guessPosition = 0, targetPts = 0, firstPts = 0;
	INT64 seekDiff = 0;
	int step, secPerBlock;
	float percent;
	BOOL seekDone = FALSE;
	UINT64 clusterSize = 0;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	UINT64 guessPosition_behind = 0xFFFFFFFFFFFFFFFF , guessPosition_ahead = 0;
	BOOL update_behind = FALSE, update_ahead = FALSE;


	FILEIN_CB_INFO      cb_info = {0};

	if (tsk_obj->state == IAMOVIEPLAY_STATE_CLOSE) {
		DBG_WRN("Media Player is not opened\r\n");
		return E_SYS;
	}

	if (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS) {

		UINT64      file_ttlsize = 0;
		UINT64      file_remain_size = 0;

		hd_bsdemux_get(path_id, HD_BSDEMUX_PARAM_FILESIZE, (UINT32*)&file_ttlsize);

		if (g_movply_container_obj.GetInfo) {
			(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_TOTALSEC, &totalSec, 0, 0);
		}
		if (g_movply_container_obj.GetInfo) {
			(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_FIRSTPTS, &tmp1, &tmp2, 0);
		}

		if (totalSec == 0 || sec > totalSec) {
			DBG_WRN("File total sec is %d, seek sec is %d, seek failed!\r\n", totalSec, sec);
			return E_SYS;
		}

		_iamovieplay_Pause();

		// wait demux task finished
		while (!TsReadLib_DemuxFinished()) {
			vos_util_delay_ms(20);
		}

		clusterSize = FileSys_GetDiskInfo(FST_INFO_CLUSTER_SIZE);
		if (clusterSize > 0x40000) { //one big cluster align
			clusterSize = 0x40000;
		}
		percent = (float)sec / totalSec;
		guessPosition = (double)file_ttlsize* percent;
		(guessPosition >= 0x100000) ? (guessPosition -= 0x100000) : (guessPosition = 0);  //prevent frame not found if seeking to end
		if (clusterSize > 0) {
			guessPosition = guessPosition - (guessPosition % clusterSize);
		}
		firstPts = ((INT64)tmp1 << 32) | tmp2;
		targetPts = (sec * 90000) + firstPts;
		DBG_IND("[TS Play] seek sec = %d, pos = %llu, cluster size = %lld\r\n", sec, guessPosition, clusterSize);

		tsk_obj->vdo_idx = sec * p_obj->vdo_fr;
		/* vdo_idx start from zero */
		if(tsk_obj->vdo_idx >= tsk_obj->vdo_ttfrm){
			DBG_WRN("seek idx(%lu) is out of idx(total frame:%lu)\r\n", tsk_obj->vdo_idx + 1, tsk_obj->vdo_ttfrm);
			return E_SYS;
		}

		if (p_obj->aud_en) {
			tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);
			tsk_obj->aud_count = tsk_obj->aud_idx;
		}
		DBG_IND("target VidIdx:%d AudIdx:%d, guessPosition 0x%llx, targetPts=%lld\r\n",
				 tsk_obj->vdo_idx, tsk_obj->aud_idx, guessPosition, targetPts);

		TsReadLib_ClearFrameEntry();
		if (g_movply_container_obj.SetInfo)
			(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_PREPARESEEK, targetPts,
											tsk_obj->vdo_idx, tsk_obj->aud_idx);

		UINT32 tmp;
		hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_TSDMX_BUF_RESET, (VOID*)&tmp);
		cb_info.event = FILEIN_IN_TYPE_CONFIG_BUFINFO;
		hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

		file_remain_size = file_ttlsize - guessPosition;
		hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_FILE_REMAIN_SIZE, &file_remain_size);

		if (g_movply_container_obj.SetInfo)
			(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
											file_remain_size >> 32,
											file_remain_size,
											0);

		// Do seek
		cb_info.uiNextFilePos = guessPosition;
		cb_info.event = FILEIN_IN_TYPE_READ_ONEBLK;
		hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

		// for getting cb_info.blk_size
		cb_info.event = FILEIN_IN_TYPE_GET_BUFINFO;
		hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

		while (!seekDone) {
			if (g_movply_container_obj.GetInfo) {
				(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_SEEKRESULT, &tmp1, &tmp2, &result);
			}
			seekDiff = ((INT64)tmp1 << 32) | tmp2;

			// demuxed BS not enough, read one more block
			if (result == SEEKRESULT_ONEMORE) {
				DBG_IND("read more blocks!\r\n");
				if (guessPosition + cb_info.blk_size< file_ttlsize) {
					guessPosition += cb_info.blk_size;
					cb_info.uiNextFilePos = guessPosition;
					cb_info.event = FILEIN_IN_TYPE_READ_ONEBLK;
					hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);
				}

				if (guessPosition + cb_info.blk_size < file_ttlsize) {
					guessPosition += cb_info.blk_size;
					cb_info.uiNextFilePos = guessPosition;
					cb_info.event = FILEIN_IN_TYPE_READ_ONEBLK;
					hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);
				}
			} else if (seekDiff == 0) {
				seekDone = TRUE;
			} else {
				tmp1 = file_ttlsize / cb_info.blk_size;
				secPerBlock = (totalSec / tmp1 > 0) ? (totalSec / tmp1) : 1;
				step = ((seekDiff / 90000) / secPerBlock) + 1;

				hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_TSDMX_BUF_RESET, NULL);
				cb_info.event = FILEIN_IN_TYPE_CONFIG_BUFINFO;
				hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

				if (result == SEEKRESULT_BEHIND) {

					update_behind = TRUE;

					if(guessPosition < guessPosition_behind){
						guessPosition_behind = guessPosition;
					}
					else if(guessPosition >= guessPosition_behind){
						guessPosition_behind -= cb_info.blk_size;
					}

					if(update_ahead)
						guessPosition = guessPosition_ahead;
					else{
						/* check overflow */
						if(guessPosition > cb_info.blk_size * step)
							guessPosition -= cb_info.blk_size * step;
						else
							guessPosition = 0;
					}
				} else {

					update_ahead = TRUE;

					if(guessPosition > guessPosition_ahead){
						guessPosition_ahead = guessPosition;
					}
					else if(guessPosition <= guessPosition_ahead){
						guessPosition_ahead += cb_info.blk_size;
					}

					if(update_behind)
						guessPosition = guessPosition_behind;
					else
						guessPosition += cb_info.blk_size * step;
				}

				file_remain_size = file_ttlsize - guessPosition;
				hd_bsdemux_set(path_id, HD_BSDEMUX_PARAM_FILE_REMAIN_SIZE, &file_remain_size);

				if (g_movply_container_obj.SetInfo)
					(g_movply_container_obj.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
													file_remain_size >> 32,
													file_remain_size,
													0);

				//DBG_IND("diff = %lld, step = %d, guess %llx\r\n", seekDiff, step, guessPosition);
				cb_info.uiNextFilePos = guessPosition;
				cb_info.event = FILEIN_IN_TYPE_READ_ONEBLK;
				hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);
			}
		}

	} else {
		DBG_IND("[Seek] filetype not supported\r\n");
	}
#endif

	/* read one more blk to fix audio can't be demuxed in time */
	cb_info.uiNextFilePos = 0;
	cb_info.event = FILEIN_IN_TYPE_READ_ONEBLK;
	hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);


	return E_OK;
}

/* get audio frame index by video display frame index */
static UINT32 _iamovieplay_GetAudFrmIdxByVideo(UINT32 vdo_idx)
{
	UINT32 aud_idx = 0;
	UINT32 sec, remv, rema;
//	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

	if (p_obj->vdo_fr == 0 || p_obj->aud_sr == 0) {
		DBG_ERR("invalid frame rate, vdo_fr=%d, aud_fr=%d", p_obj->vdo_fr, p_obj->aud_sr);
		return 0;
	}
	sec = vdo_idx / p_obj->vdo_fr;
	remv = vdo_idx % p_obj->vdo_fr;
	rema = (((remv * 1000) / p_obj->vdo_fr) * p_obj->aud_sr) / 1000;

	aud_idx = ((sec * p_obj->aud_sr) + rema) / IAMOVIEPLAY_BS_DECODE_SAMPLES;

	if (aud_idx >= p_obj->aud_ttfrm && p_obj->aud_ttfrm != 0) {
		aud_idx = p_obj->aud_ttfrm - 1;
	}

	return aud_idx;
}

/************************************************************************************
 * insure MP4 audio and video bitstream file offset distance not exceed filein buffer
 ***********************************************************************************/
static BOOL _iamovieplay_av_bs_offset(void)
{
	HD_FILEIN_BLKINFO blkinfo = {0};
	UINT64 diff;
	UINT64 size;
	UINT64 filein_valid_buf_size;
	IAMOVIEPLAY_TSK    *tsk_obj = &iamovieplay_tsk_obj;

	if(tsk_obj->aud_en == FALSE ||
	   !(tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD))
		return TRUE;

	if (g_aud_bsdemux_cbinfo.bs_offset < g_vdo_bsdemux_cbinfo.bs_offset){
		diff = g_vdo_bsdemux_cbinfo.bs_offset - g_aud_bsdemux_cbinfo.bs_offset;
		size = g_vdo_bsdemux_cbinfo.bs_size;
	}
	else{
		diff = g_aud_bsdemux_cbinfo.bs_offset - g_vdo_bsdemux_cbinfo.bs_offset;
		size = g_aud_bsdemux_cbinfo.bs_size;
	}

	hd_filein_get(0, HD_FILEIN_PARAM_BLK_INFO, (UINT32*) &blkinfo);

	filein_valid_buf_size = blkinfo.blk_size * blkinfo.tt_blknum;

//	DBG_DUMP("filein_valid_buf_size = %llx , audio and video file offset diff = %llx\n", filein_valid_buf_size, diff + size);

	if((diff + size) > filein_valid_buf_size){

		MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
		MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

		DBG_ERR("reload buffer failed!, audio and video file offset exceed filein buffer (%llx, need %llx), bitstream may not be loaded completely\n",
				filein_valid_buf_size,
				diff + size
		);

		info.ret = HD_ERR_BAD_DATA;
		info.is_video = (g_aud_bsdemux_cbinfo.bs_offset < g_vdo_bsdemux_cbinfo.bs_offset) ? TRUE : FALSE;
		info.frame_idx = (g_aud_bsdemux_cbinfo.bs_offset < g_vdo_bsdemux_cbinfo.bs_offset) ? tsk_obj->vdo_idx : tsk_obj->aud_idx;

		(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);

		return FALSE;
	}

	return TRUE;
}

static BOOL _iamovieplay_ReloadFileReadBlock(void)
{
	IAMOVIEPLAY_TSK    *tsk_obj = &iamovieplay_tsk_obj;
	IAMOVIEPLAY_BSDEMUX_BS_INFO *pcbinfo = &g_vdo_bsdemux_cbinfo;
	FILEIN_CB_INFO   cb_info = {0};
	HD_PATH_ID       path_id = 0;
	UINT64 bs_offset = 0, aud_bs_offset = 0;
	UINT32 bs_size = 0, aud_bs_size = 0;
	// get vdo bs offset
   	HD_BSDEMUX_BUF push_in_buf = {0};
	HD_RESULT ret;

	g_bReloadingFileReadBlock = TRUE;

	// video bs
	#if 0
	push_in_buf.sign = MAKEFOURCC('B','S','D','X');
	push_in_buf.bs_type = HD_BSDEMUX_BS_TPYE_VIDEO;
	push_in_buf.index = tsk_obj->vdo_idx;
	DBG_IND("FF(FR) Step2\r\n");
	ret = hd_bsdemux_push_in_buf(path_id, &push_in_buf, 0);
	if (ret != HD_OK) {
		DBG_ERR("push_in video demux fail(%d)", ret);
	}
	#else
	{// Get video frame offset and size in file.
		//IAMOVIEPLAY_BSDEMUX_BS_INFO *p_vdo_cbinfo = &g_vdo_bsdemux_cbinfo;
		if (g_movply_container_obj.GetInfo) {
			(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE, &tsk_obj->vdo_idx, (UINT32 *)&pcbinfo->bs_offset, &pcbinfo->bs_size);
		}
	}
	#endif

	if (pcbinfo->bs_offset == 0) {
		DBG_ERR("get vdo bs offset error, i=%d\r\n", tsk_obj->vdo_idx);
		return FALSE;
	}
	bs_offset = pcbinfo->bs_offset;
	bs_size   = pcbinfo->bs_size;
	DBG_DUMP("get vdo i=%d, o=0x%llx, s=%d\r\n", tsk_obj->vdo_idx, bs_offset, bs_size);

	// get aud bs offset
	#if 1//_TODO
	if (tsk_obj->aud_en) {
		tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);
		tsk_obj->aud_count = tsk_obj->aud_idx;
		// audio bs
		#if 1
		push_in_buf.sign = MAKEFOURCC('B','S','D','X');
		push_in_buf.bs_type = HD_BSDEMUX_BS_TPYE_AUDIO;
		push_in_buf.index = tsk_obj->aud_idx;
		ret = hd_bsdemux_push_in_buf(path_id, &push_in_buf, 0);
		if (ret != HD_OK) {
			DBG_ERR("push_in auido dumux fail(%d)", ret);
			return FALSE;
		}
		#else
		{// Get audio frame offset and size in file.
			pcbinfo = &g_aud_bsdemux_cbinfo;

			if (g_movply_container_obj.GetInfo) {
				(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_GETAUDIOPOSSIZE, &tsk_obj->aud_idx, (UINT32 *)&pcbinfo->bs_offset, &pcbinfo->bs_size);
			}
		}
		#endif

		vos_util_delay_ms(5);  // Important!!! Delay to ensure audio decode/demux was done. Fix resume to normal speed error!

		pcbinfo = &g_aud_bsdemux_cbinfo;
		if (pcbinfo->bs_offset == 0) {
			DBG_ERR("get aud bs offset error, i=%d\r\n", tsk_obj->aud_idx);
			return FALSE;
		}
		aud_bs_offset = pcbinfo->bs_offset;
		aud_bs_size   = pcbinfo->bs_size;
		DBG_DUMP("get aud i=%d, o=0x%llx, s=%d\r\n", tsk_obj->aud_idx, aud_bs_offset, aud_bs_size);

		// check if aud bs is further front than vdo bs
		if (aud_bs_offset < bs_offset) {
			bs_offset = aud_bs_offset;
			bs_size = aud_bs_size;
		}

		if(_iamovieplay_av_bs_offset() == FALSE)
			return FALSE;
	}
	#else
		tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);

		// audio bs
		push_in_buf.sign = MAKEFOURCC('B','S','D','X');
		push_in_buf.bs_type = HD_BSDEMUX_BS_TPYE_AUDIO;
		push_in_buf.index = tsk_obj->aud_idx;
		DBGD(tsk_obj->aud_idx);
		ret = hd_bsdemux_push_in_buf(path_id, &push_in_buf, 0);
		if (ret != HD_OK) {
			DBG_ERR("push_in auido dumux fail(%d)", ret);
		}
		vos_util_delay_ms(10);  // Delay to ensure audio demux was done.

		pcbinfo = &g_aud_bsdemux_cbinfo;
		DBGH(pcbinfo->bs_offset);
		DBGD(pcbinfo->bs_size);
		if (pcbinfo->bs_offset == 0) {
			DBG_ERR("get aud bs offset error, i=%d\r\n", tsk_obj->aud_idx);
			return;
		}
	#endif

	// callback FileIn reload buffer
	DBG_DUMP("reload buf, bs_off=0x%llx, bs_size=%d, vi=%d, ai=%d\r\n", bs_offset, bs_size, tsk_obj->vdo_idx, tsk_obj->aud_idx);
	cb_info.bs_offset = bs_offset;
	cb_info.bs_size = bs_size;
	cb_info.event = FILEIN_IN_TYPE_RELOAD_BUF;
	hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

	//vos_util_delay_ms(50); // Should wait for file buffer reload done?

	g_bReloadingFileReadBlock = FALSE;

	return TRUE;
}
#if 0
static void _iamovieplay_ReloadVideo(void)
{
	if (gNMPBsDemuxObj.vdo_en) {
		_NMP_BsDemux_ResetVQ();

		// callback VdoDec refresh queue
		if (gNMPBsDemuxObj.CallBackFunc) {
	        (gNMPBsDemuxObj.CallBackFunc)(NMI_BSDEMUX_EVENT_REFRESH_VDO, 0);
	    }

		_NMP_BsDemux_PreDemuxVdoBS();
	}
}

static void _iamovieplay_ReloadAudio(void)
{
	if (gNMPBsDemuxObj.aud_en) {
		_NMP_BsDemux_ResetAQ();

		// notify AudOut refresh [ToDo]

		_NMP_BsDemux_PreDemuxAudBS();
	}
}
#endif

/* get next/previous I-frame index */
static UINT32 _iamovieplay_GetNPIframe(UINT32 frm_idx, IAMOVIEPLAY_SPEED_TYPE speed, IAMOVIEPLAY_DIRECT_TYPE direct)
{
	UINT32 skip_i = 0;
	UINT32 get_id = MEDIAREADLIB_GETINFO_NEXTIFRAMENUM;
	UINT32 i_frm_idx = 0;
	//IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	HD_BSDEMUX_PATH_CONFIG p_path_cfg = {0};
	CONTAINERPARSER *p_contr;
	HD_PATH_ID path_id = 0;


	HD_RESULT ret = hd_bsdemux_get(path_id, HD_BSDEMUX_PARAM_PATH_CONFIG, &p_path_cfg);
	if (ret != HD_OK) {
		DBG_ERR("set path_config fail=%d\n", ret);
		return ret;
	}

	p_contr = p_path_cfg.container;

	switch (speed) {
	case IAMOVIEPLAY_SPEED_8X:
		skip_i = 1;
		break;
	case IAMOVIEPLAY_SPEED_16X:
		skip_i = 3;
		break;
	case IAMOVIEPLAY_SPEED_32X:
		skip_i = 7;
		break;
	case IAMOVIEPLAY_SPEED_64X:
		skip_i = 15;
		break;
	default:
		skip_i = 0;
		break;
	}

	if (direct == IAMOVIEPLAY_DIRECT_FORWARD) {
		get_id = MEDIAREADLIB_GETINFO_NEXTIFRAMENUM;
	} else if (direct == IAMOVIEPLAY_DIRECT_BACKWARD) {
		get_id = MEDIAREADLIB_GETINFO_PREVIFRAMENUM;
	}

	if (!p_contr) {
		DBG_ERR("Container NULL\n");
		return HD_ERR_SYS;
	}

	if (p_contr->GetInfo) {
		(p_contr->GetInfo)(get_id, &frm_idx, &i_frm_idx, &skip_i);
		#if 0
		DBGD(i_frm_idx);
		DBGD(skip_i);
		#endif
	}

	return i_frm_idx;
}

static ER _iamovieplay_TS_ReadFile(UINT8 *path, UINT64 pos, UINT32 size, UINT32 addr)
{
	FILEIN_CB_INFO      cb_info = {0};
	UINT64      		file_ttlsize;
	HD_PATH_ID path_id = 0;
	ER EResult = E_OK;

	hd_bsdemux_get(path_id, HD_BSDEMUX_PARAM_FILESIZE, (VOID*)&file_ttlsize);

	if (pos > file_ttlsize) {
		DBG_IND("readFilePos > size: pos=0x%llx, size=0x%lx, filesize=0x%lx\r\n", pos, size, p_ts_buf->file_ttlsize);
		return E_PAR;
	}

	if ((pos + size) > file_ttlsize) {
		size = file_ttlsize - pos;
	}

	// callback reading file (one i-frame)
	cb_info.bs_offset = pos;
	cb_info.bs_size = size;
	cb_info.event = FILEIN_IN_TYPE_READ_ONEFRM;
	hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

	return EResult;
}

/* play I-frame in one shot (read I-frame BS -> decode) */
static ER _iamovieplay_IFrameInOneShot(void)
{
	IAMOVIEPLAY_TSK    *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	UINT32 keyFrameOfs = 0;
	UINT32 i_frm_idx = 0;
	BOOL vdo_frm_eof = FALSE;

	if (tsk_obj->state == IAMOVIEPLAY_STATE_CLOSE) {
		DBG_ERR("BsDemux is closed\r\n");
		return E_SYS;
	}

	if (tsk_obj->vdo_frm_eof) {
		DBGD(tsk_obj->vdo_ttfrm);
		DBG_ERR("EOF!\r\n");
		return E_SYS;
	}

	if (p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) {

		HD_PATH_ID 			path_id = 0;
		MOVIEPLAY_PLAY_OBJ *p_play_obj = &g_pImageStream->play_obj;
		UINT32 				ts_gop = p_play_obj->gop;
		UINT32				keyFrameNum = 0, keyFrameDist = 0, keyFrameSize = 0, readFileSize = 0, uiBSSize = 0, direction = 0;
		UINT64				readFileOffset = 0, readFileOffset_tmp = 0;
		UINT64      		file_ttlsize;
		FILEIN_CB_INFO		cb_info = {0};
		ER EResult = E_OK;

		hd_bsdemux_get(path_id, HD_BSDEMUX_PARAM_FILESIZE, (VOID*)&file_ttlsize);

		if (tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
			i_frm_idx = tsk_obj->vdo_idx + ts_gop - (tsk_obj->vdo_idx % ts_gop);
			// skip I-frame for 4x/8x
			i_frm_idx += (tsk_obj->speed / 2 - 1) * ts_gop;
		} else {
			i_frm_idx = tsk_obj->vdo_idx - ts_gop - (tsk_obj->vdo_idx % ts_gop);
			// check fast-rewind to VidIdx 0
			if (((tsk_obj->speed / 2 - 1) * ts_gop) >= i_frm_idx) {
				i_frm_idx = 0;
				vdo_frm_eof = TRUE;
			} else {
				i_frm_idx -= (tsk_obj->speed / 2 - 1) * ts_gop;
			}
		}

		if (i_frm_idx == 0) {
			vdo_frm_eof = TRUE;
			tsk_obj->vdo_frm_eof = TRUE;
		} else if (i_frm_idx+(ts_gop*(tsk_obj->speed >> 1)) >= tsk_obj->vdo_ttfrm) {
			//i_frm_idx  = gNMPBsDemux_VdoIdx; // repeat and set EOF
			vdo_frm_eof = TRUE;
			tsk_obj->vdo_frm_eof = TRUE;
		}
		tsk_obj->vdo_idx = i_frm_idx;

		// wait demux task finished
		while (!TsReadLib_DemuxFinished()) {
			vos_util_delay_ms(20);
		}

		if (ts_gop != 0) {
			keyFrameNum = tsk_obj->vdo_ttfrm / ts_gop;
		}
		if (keyFrameNum != 0) {
			keyFrameDist = file_ttlsize / keyFrameNum;
		}

		keyFrameSize = p_obj->vdo_1stfrmsize;

		if (ts_gop != 0) {
			readFileOffset = (UINT64)(tsk_obj->vdo_idx / ts_gop) * (UINT64) keyFrameDist;
		}

		readFileSize = keyFrameSize * 2;

		// Read BS
		cb_info.event = FILEIN_IN_TYPE_READ_TSBLK;
		hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0);

		if ((UINT64)keyFrameSize > readFileOffset) {
			keyFrameSize = (UINT32)readFileOffset;
		}

		readFileOffset_tmp = readFileOffset - (UINT64)keyFrameSize;
		EResult = _iamovieplay_TS_ReadFile(0, readFileOffset_tmp, readFileSize, cb_info.uiThisAddr);
		if (EResult == E_OK) {
			// search I-frame in TS file
			TsReadLib_SearchIFrame(cb_info.uiThisAddr, readFileSize, cb_info.uiDemuxAddr, &uiBSSize, &direction, &keyFrameOfs);
			DBG_IND("direction=%lu found_offset = %lx\n", direction, found_offset);

		} else {
			DBG_ERR("NMI_BsDemux_CBReadFile Error\r\n");
			return E_SYS;
		}

		/* found key frame ts packet, move offset by keyFrameOfs */
		if (direction == 1) {
			EResult = _iamovieplay_TS_ReadFile(0, readFileOffset_tmp + (UINT64)keyFrameOfs, readFileSize, cb_info.uiThisAddr);
			TsReadLib_SearchIFrame(cb_info.uiThisAddr, readFileSize, cb_info.uiDemuxAddr, &uiBSSize, &direction, &keyFrameOfs);

			/* direction should be 0 */
			if(direction != 0){
				DBG_ERR("search i frame failed!\n");
			}
		}
		/* key frame ts packet not found, adjust offset */
		else if (direction == 2) {

			UINT32 new_offset = keyFrameSize / 2;

			for (UINT32 i = 0; readFileOffset > (new_offset + i * readFileSize); i++)
			{
				keyFrameOfs = 0;
				readFileOffset_tmp = readFileOffset - new_offset - i * readFileSize;
				EResult = _iamovieplay_TS_ReadFile(0, readFileOffset_tmp, readFileSize, cb_info.uiThisAddr);
				TsReadLib_SearchIFrame(cb_info.uiThisAddr, readFileSize, cb_info.uiDemuxAddr, &uiBSSize, &direction, &keyFrameOfs);

				if(direction == 1){
					readFileOffset_tmp += keyFrameOfs;
					break;
				}
				else if(direction == 0){
					break;
				}
			}

			if(direction == 2){
				DBG_ERR("search i frame failed!\n");
			}
			else if(direction == 1){
				EResult = _iamovieplay_TS_ReadFile(0, readFileOffset_tmp, readFileSize, cb_info.uiThisAddr);
				TsReadLib_SearchIFrame(cb_info.uiThisAddr, readFileSize, cb_info.uiDemuxAddr, &uiBSSize, &direction, &keyFrameOfs);
				if(direction != 0){
					DBG_ERR("search i frame failed!\n");
				}
			}
		}

		if (uiBSSize == 0 && !vdo_frm_eof) {
			//skip this I-frame
			DBG_DUMP("^Y skip!\r\n");
			return E_OK;
		}

		IAMOVIEPLAY_BSDEMUX_BS_INFO *p_vdo_cbinfo = &g_vdo_bsdemux_cbinfo;

		p_vdo_cbinfo->bs_addr = cb_info.uiDemuxAddr;
		p_vdo_cbinfo->bs_size = uiBSSize;
		p_vdo_cbinfo->frm_idx = tsk_obj->vdo_idx;

		if (vdo_frm_eof) {
			p_vdo_cbinfo->bIsEOF   = TRUE;
		} else {
			p_vdo_cbinfo->bIsEOF   = FALSE;
		}
		if (p_vdo_cbinfo->bIsEOF) {
			tsk_obj->vdo_frm_eof = TRUE;
			// wait finish ? [ToDo]
		}

		set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_VBS_OUT);

	} else {
		// get next or previous i-frame index
		i_frm_idx = _iamovieplay_GetNPIframe(tsk_obj->vdo_idx, tsk_obj->speed, tsk_obj->direct);

		if (i_frm_idx == 0) {
			vdo_frm_eof = TRUE;
		} else if (i_frm_idx >= tsk_obj->vdo_ttfrm && tsk_obj->vdo_ttfrm != 0) {
			i_frm_idx = tsk_obj->vdo_idx; // repeat and set EOF
			vdo_frm_eof = TRUE;
		}

		// check i-frame index validation
		if (tsk_obj->gop == 0) {
			DBG_ERR("gop = 0\r\n");
			return E_SYS;
		}
		if ((i_frm_idx % tsk_obj->gop) != 0) {
			DBGD(i_frm_idx);
			DBGD(tsk_obj->gop);
			// find previous i-frame to instead
			i_frm_idx = _iamovieplay_GetNPIframe(tsk_obj->vdo_idx, NMEDIAPLAY_SPEED_NORMAL, NMEDIAPLAY_DIRECT_BACKWARD);
			vdo_frm_eof = TRUE;
			DBG_WRN("invalid i-frm idx, use previous i=%d instead\r\n", i_frm_idx);
		}

		tsk_obj->vdo_idx = i_frm_idx;
		IAMMOVIEPLAY_COMM_DUMP("i-frm=%d (one shot)\r\n", tsk_obj->vdo_idx);

		//_NMP_BsDemux_ResetVQ();
#if 1
		// [ToDo]
		// ~~~~~~~~~~~~ reset video raw queue (?) ~~~~~~~~~~
#endif

#if _TODO
		// get vdo bs
		_NMP_BsDemux_GetVdoBs(&bs_offset, &bs_size, gNMPBsDemux_VdoIdx);

		// video bs
		push_in_buf.sign = MAKEFOURCC('B','S','D','X');
		push_in_buf.bs_type = HD_BSDEMUX_BS_TPYE_VIDEO;
		push_in_buf.index = tsk_obj->vdo_idx;
		ret = hd_bsdemux_push_in_buf(path_id, &push_in_buf, 0);
		if (ret != HD_OK) {
			DBG_ERR("push_in video buf fail(%d)", ret);
		}

		if (pcbinfo->bs_offset == 0) {
			DBG_ERR("get vdo bs error, i=%d\r\n", tsk_obj->vdo_idx);
			return E_SYS;
		}

		// callback reading file (one i-frame)
		cb_info.bs_offset = bs_offset;
		cb_info.bs_size = bs_size;
		cb_info.event = FILEIN_IN_TYPE_READ_ONEFRM;
		if (gNMPBsDemuxObj.CallBackFunc) {
			(gNMPBsDemuxObj.CallBackFunc)(NMI_BSDEMUX_EVENT_READ_FILE, (UINT32) &cb_info);
		}

		// assign bs_addr
		if (cb_info.buf_start_addr) {
			bs_addr = cb_info.buf_start_addr;
		} else {
			DBG_ERR("blk_start_addr = 0, reading file error\r\n");
			return E_SYS;
		}

		// check h.265 bitstream
		if (p_obj->vdo_codec == NMEDIAPLAY_DEC_H265) {
			if (gNMPBsDemux_VdoIdx % p_obj->gop == 0) {
				if (_NMP_BsDemux_ChkH265IfrmBS((UINT8*)bs_addr) != E_OK) {
					DBG_ERR("check i-frame error, i=%d, addr=0x%x, size=%d\r\n", gNMPBsDemux_VdoIdx, bs_addr, bs_size);
					return E_SYS;
				}
				bs_addr += gNMPBsDemuxObj.desc_size;
			}
		}

		bs_info.BsAddr = bs_addr;
		bs_info.BsSize = bs_size;
		bs_info.FrmIdx = gNMPBsDemux_VdoIdx;

		if (vdo_frm_eof) {
			bs_info.bIsEOF = TRUE;
		} else {
			bs_info.bIsEOF = FALSE;
		}
		if (bs_info.bIsEOF) {
			p_obj->vdo_frm_eof = TRUE;
			// wait finish ? [ToDo]
		}

		if (p_obj->do_seek) {
			return E_OK;
		}

		// add to queue
		if (_NMP_BsDemux_AddVQ(0, &bs_info) != E_OK) {
			DBG_ERR("Add VQ Fail\r\n");
			return E_SYS;
		}

		// output video bs (trigger decode)
		set_flg(NMP_BSDEMUX_FLG_ID, FLG_NMP_BSDEMUX_VBS_OUT);
#else
		// FF(FR) Step1. trigger bsdemux video bs
		#if 0
		set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_VBS_DEMUX);
		DBG_IND("FF(FR) Step1\r\n");
		#else
		{
			IAMOVIEPLAY_BSDEMUX_BS_INFO *p_vdo_cbinfo = &g_vdo_bsdemux_cbinfo;
			if (g_movply_container_obj.GetInfo) {
				(g_movply_container_obj.GetInfo)(MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE, &tsk_obj->vdo_idx, (UINT32 *)&p_vdo_cbinfo->bs_offset, &p_vdo_cbinfo->bs_size);
			}
		}
		#endif
		#if 1
		// trigger filein
		set_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_TRIG);
		#endif

		if (vdo_frm_eof) {
			tsk_obj->vdo_frm_eof = TRUE;
			// wait finish ? [ToDo]
		}

		if (tsk_obj->do_seek) {
			return E_OK;
		}
#endif
	}
	return E_OK;
}

static ER _iamovieplay_Start(void)
{
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

#if 0
	// setup timer interval.
    iamovieplay_SetVdoTimerIntval(0);
#endif
	g_movie_finish_event = FALSE;

	tsk_obj->vdo_en    = p_obj->vdo_en;
	tsk_obj->vdo_fr    = p_obj->vdo_fr;
	tsk_obj->aud_en    = p_obj->aud_en;
	DBGD(tsk_obj->vdo_en);
	DBGD(tsk_obj->aud_en);
	tsk_obj->vdo_ttfrm = p_obj->vdo_ttfrm;
	tsk_obj->aud_ttfrm = p_obj->aud_ttfrm;
	tsk_obj->gop       = p_obj->gop;
	if(p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_MOV) {
		tsk_obj->file_fmt  = MEDIA_FILEFORMAT_MOV;
	} else if(p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_MP4) {
		tsk_obj->file_fmt  = MEDIA_FILEFORMAT_MP4;
	} else if(p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) {
		tsk_obj->file_fmt  = MEDIA_FILEFORMAT_TS;
	} else {
		DBG_ERR("unknown file format!\r\n");
	}
	tsk_obj->state = IAMOVIEPLAY_STATE_PLAYING;

	// trigger bsdemux audio bs
	if(tsk_obj->aud_en && (tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD)) {

		if(p_obj->file_fmt != _MOVPLAY_CFG_FILE_FORMAT_TS && (tsk_obj->aud_idx >= tsk_obj->aud_ttfrm)){
		}
		else{
			
			// audio Playback system un-initial
			DBG_IND("audio PB Resume...\r\n");
			vos_flag_clr(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_PAUSE);
			vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_RESUME);
			DBG_IND("audio PB Resume ... ok\r\n");
	
			g_bForceFirstAudioPB = TRUE; // Trigger the
			set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_ABS_DEMUX);
		}
	}

    // start video
    if (p_obj->vdo_en) {
    	_iamovieplay_TimerTriggerStart();
    }

	return E_OK;
}

static HD_RESULT get_video_frame_buf(HD_VIDEO_FRAME *p_video_frame)
{
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = 0;
	UINT32 blk_size = 0, pa = 0;
	UINT32 width = 0, height = 0;
	HD_VIDEO_PXLFMT pxlfmt = 0;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

	if (p_video_frame == NULL) {
		printf("config_vdo_frm: p_video_frame is null\n");
		return HD_ERR_SYS;
	}

	// config yuv info
	ddr_id = DDR_ID0;
	width = p_obj->width;//VDO_SIZE_W;
	height = p_obj->height;//VDO_SIZE_H;
	pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	if (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) {
		blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(width), ALIGN_CEIL_16(height), pxlfmt);  // align to 16 for h264 raw buffer
	} else {
		blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(width), ALIGN_CEIL_64(height), pxlfmt);  // align to 64 for h265 raw buffer
	}

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("config_vdo_frm: get blk fail, blk(0x%x)\n", blk);
		return HD_ERR_SYS;
	}

	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		DBG_ERR("config_vdo_frm: blk2pa fail, blk(0x%x)\n", blk);
		return HD_ERR_SYS;
	}

	p_video_frame->sign = MAKEFOURCC('V', 'F', 'R', 'M');
	p_video_frame->ddr_id = ddr_id;
	p_video_frame->pxlfmt = pxlfmt;
	p_video_frame->dim.w = width;
	p_video_frame->dim.h = height;
	p_video_frame->phy_addr[0] = pa;
	p_video_frame->blk = blk;

	return HD_OK;
}

UINT32 imageapp_movieplay_get_hd_phy_addr(void *va)
{
    if (va) {
        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
        vir_meminfo.va = va;
        if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) == HD_OK) {
                return vir_meminfo.pa;
        }
    }

    return 0;
}

// checked
//static UINT8 g_bfisrt_decode = 0;
INT32 _imageapp_movieplay_bsdemux_callback(HD_BSDEMUX_CB_EVENT event_id, HD_BSDEMUX_CBINFO *p_param)
{
    IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	extern MOVIEPLAY_FILE_DECRYPT g_movply_file_decrypt;

	/* decrypt */
	BOOL is_encrypted = (g_movply_file_decrypt.type == MOVIEPLAY_DECRYPT_TYPE_NONE) ? FALSE : TRUE;
	UINT32 decrypt_type = g_movply_file_decrypt.type;

	switch (event_id) {
	case HD_BSDEMUX_CB_EVENT_VIDEO_BS: {
		    IAMOVIEPLAY_BSDEMUX_BS_INFO *pcbinfo = &g_vdo_bsdemux_cbinfo;

    		DBG_IND("vdo_bs: offset(%lld), addr(0x%x), size(%d), index(%d)\r\n",
    			p_param->offset, p_param->addr, p_param->size, p_param->index);

            // Step1. update bs offset and size for FileIn module to do next push in action.
            pcbinfo->bs_offset = p_param->offset;  // for FileIn moudle.
            pcbinfo->bs_addr   = p_param->addr;    // bs addr in DRAM for VdoDec module.
            pcbinfo->bs_size   = p_param->size;
			pcbinfo->bIsAud	   = FALSE;

			if(tsk_obj->file_fmt != IAMOVIEPLAY_FILEFMT_TS && _iamovieplay_av_bs_offset() == FALSE)
				return 0;

			if (is_encrypted) {

			    BOOL need_decrypt = FALSE;
			    BOOL is_i_frame = (tsk_obj->vdo_idx % tsk_obj->gop == 0) ? TRUE : FALSE;

				if(	((TRUE == is_i_frame) && (decrypt_type & MOVIEPLAY_DECRYPT_TYPE_I_FRAME)) ||
					((FALSE == is_i_frame) && (decrypt_type & MOVIEPLAY_DECRYPT_TYPE_P_FRAME))
				){
					need_decrypt = TRUE;
				}

				if(TRUE == need_decrypt){

					_iamovieplay_decrypt_hex_dump((TRUE == is_i_frame)?MOVIEPLAY_DECRYPT_TYPE_I_FRAME: MOVIEPLAY_DECRYPT_TYPE_P_FRAME, "bsdemux_callback before decrypt", pcbinfo->bs_addr, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);

					/* pcbinfo->bs_addr is a real bs start address from bsdemux, it means no need to skip desc size for H265 */
					_iamovieplay_bs_decrypt(pcbinfo->bs_addr, pcbinfo->bs_size);

					_iamovieplay_decrypt_hex_dump((TRUE == is_i_frame)?MOVIEPLAY_DECRYPT_TYPE_I_FRAME: MOVIEPLAY_DECRYPT_TYPE_P_FRAME, "bsdemux_callback after decrypt ", pcbinfo->bs_addr, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);
				}
			}

			if(tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD)
			{
				UINT32 frame_idx = tsk_obj->vdo_idx;
				extern MOVIEPLAY_VDEC_SKIP_SVC_FRAME g_vdec_skip_svc_frame;
				BOOL skip_result = FALSE;

				tsk_obj->vdo_idx++;
				if(p_param->skippable == TRUE && g_vdec_skip_svc_frame.enabled == TRUE){

					if(g_vdec_skip_svc_frame.condition_callback){

						MOVIEPLAY_VDEC_SKIP_SVC_FRAME_BS_INFO bs_info = {
							.frame_idx = frame_idx,
							.width = p_obj->width,
							.height = p_obj->height,
							.fps = p_obj->vdo_fr,
							.bs_addr = p_param->addr
						};

						if(g_vdec_skip_svc_frame.condition_callback(&bs_info, g_vdec_skip_svc_frame.user_data) == TRUE){
							skip_result = TRUE;
						}
					}
					else
						skip_result = TRUE;

					if(skip_result == TRUE){
						IAMMOVIEPLAY_COMM_DUMP("skip svc 1x frame(idx:%lu)\n", frame_idx);
						break;
					}
				}
			}

            // Step 2.
            //if (!g_bfisrt_decode++)
            { // only push I or P frame to videodec.
    			HD_VIDEODEC_BS video_bitstream = {0};
				HD_VIDEO_FRAME video_frame = {0};
				HD_RESULT      ret = HD_OK;
                HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

				//memcpy((void*)g_movply_vdodec_blk_phy_adr, (void*)p_param->addr, pcbinfo->bs_size);
				//--- data is written by CPU, flush CPU cache to PHY memory ---
				hd_common_mem_flush_cache((void *)p_param->addr, p_param->size);

				// break if get video frame failed
				ret = get_video_frame_buf(&video_frame);
				if (ret != HD_OK) {
					DBG_ERR("get video frame error(%d) !!\r\n", ret);
					break;
				}

    			// config video bs
    			video_bitstream.sign          = MAKEFOURCC('V','S','T','M');
    			video_bitstream.p_next        = NULL;
    			video_bitstream.ddr_id        = ddr_id;
    			video_bitstream.vcodec_format = (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) ? HD_CODEC_TYPE_H264 : HD_CODEC_TYPE_H265;
    			video_bitstream.timestamp     = hd_gettime_us();
				// convert bitstream addr to mapping block.
    			//video_bitstream.blk           = tsk_obj->vdec_blk;
    			video_bitstream.count         = 0;
				video_bitstream.phy_addr      = imageapp_movieplay_get_hd_phy_addr((void *)p_param->addr);
    			video_bitstream.size          = p_param->size;
				video_bitstream.blk           = -2;

				// Don't do videodec if doing _iamovieplay_ReloadFileReadBlock() while resume to normal speed play.
				if(!g_bReloadingFileReadBlock) {

					ret = hd_videodec_push_in_buf(p_obj->vdec_path, &video_bitstream, &video_frame, -1);
	    			if (ret != HD_OK) {
						printf("push_in error(%d) !!\r\n", ret);

						if (p_play_info->event_cb) {

							MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

							info.ret = ret;
							info.is_video = TRUE;
							info.frame_idx = 0; /* reserved, hdal currently not support store user data */

							(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);
						}

						// release video frame buf
						ret = hd_videodec_release_out_buf(p_obj->vdec_path, &video_frame);
						if (ret != HD_OK) {
							printf("release video frame error(%d) !!\r\n\r\n", ret);
						}
						break;
					}
				}

				#if 0 // Save decoded YUV RAW file.
				{
					static FST_FILE fp = NULL;
					char sFileName[256];
					UINT32 uiRawYSize = p_obj->width * p_obj->height;
					UINT32 uiRawUVSize = uiRawYSize / 2;
					DBGD(uiRawYSize);
					DBG_DUMP("[VDODEC][%d] dump decoded yuv frame, w = %d, h = %d, raw addr = 0x%08x, raw size = %d\r\n", 0, p_obj->width, p_obj->height, video_frame.phy_addr[HD_VIDEO_PINDEX_Y], uiRawYSize);
					snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.y", p_obj->width, p_obj->height);
					fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
					if (fp != NULL) {
						if (FileSys_WriteFile(fp, (UINT8 *)&video_frame.phy_addr[HD_VIDEO_PINDEX_Y], &uiRawYSize, 0, NULL) != FST_STA_OK) {
							DBG_ERR("[VDODEC][%d] dump decoded y frame err, write file fail\r\n", 0);
						} else {
							DBG_DUMP("[VDODEC][%d] write file ok\r\n", 0);
						}
						FileSys_CloseFile(fp);
					} else {
						DBG_ERR("[VDODEC][%d] dump decoded y frame err, invalid file handle\r\n", 0);
					}

					sFileName[0] = '\0';
					snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.uv", p_obj->width, p_obj->height);
					fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
					if (fp != NULL) {
						if (FileSys_WriteFile(fp, (UINT8 *)&video_frame.phy_addr[HD_VIDEO_PINDEX_UV], &uiRawUVSize, 0, NULL) != FST_STA_OK) {
							DBG_ERR("[VDODEC][%d] dump decoded uv frame err, write file fail\r\n", 0);
						} else {
							DBG_DUMP("[VDODEC][%d] write file ok\r\n", 0);
						}
						FileSys_CloseFile(fp);
					} else {
						DBG_ERR("[VDODEC][%d] dump decoded uv frame err, invalid file handle\r\n", 0);
					}
				}
				#endif

				// release video frame buf
				ret = hd_videodec_release_out_buf(p_obj->vdec_path, &video_frame);
				if (ret != HD_OK) {
					printf("release video frame error(%d) !!\r\n\r\n", ret);
				}

				#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
				DBG_IND("FLG_IAMOVIEPLAY_DISP_TRIG\r\n");
				set_flg(IAMOVIEPLAY_DISP_FLG_ID, FLG_IAMOVIEPLAY_DISP_TRIG);
				#endif
            }
	    }
		break;
	case HD_BSDEMUX_CB_EVENT_AUDIO_BS: {

			IAMOVIEPLAY_BSDEMUX_BS_INFO *pcbinfo = &g_aud_bsdemux_cbinfo;
			BOOL bIsFirstAudioDemux = !(p_param->index);

	        // Step1. update bs offset and size for FileIn module to do next push in action.
	        pcbinfo->bs_offset = p_param->offset;  // for FileIn moudle.
			pcbinfo->bs_addr = imageapp_movieplay_get_hd_phy_addr((void *)p_param->addr);
			pcbinfo->bs_addr_va = p_param->addr;
	        pcbinfo->bs_size   = p_param->size;
			pcbinfo->bIsAud	   = TRUE;
			pcbinfo->bIsEOF	   = p_param->eof;
			pcbinfo->frm_idx   = p_param->index;

			if(tsk_obj->file_fmt != IAMOVIEPLAY_FILEFMT_TS && _iamovieplay_av_bs_offset() == FALSE)
				return 0;

			tsk_obj->aud_frm_eof = p_param->eof;
			tsk_obj->aud_idx++; // inc audio frame index.

			if(bIsFirstAudioDemux || g_bForceFirstAudioPB) {
				g_bForceFirstAudioPB = FALSE;
				vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC);
			}
		}
		break;
	case HD_BSDEMUX_CB_EVENT_READ_VIDEO_BS:
	case HD_BSDEMUX_CB_EVENT_READ_AUDIO_BS:
	{
		set_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_TRIG);
		break;
	}
	default:
		DBG_ERR("invalid event_id = %d\r\n", event_id);
		break;
	}

	return 0;
}

THREAD_RETTYPE _iamovieplay_Read_Tsk(void)
{
	DBG_FUNC("start\r\n");

	FLGPTN			flag = 0, wait_flag = 0;

	THREAD_ENTRY(); //kent_tsk();
	is_movieplay_filein_read_tsk_running = TRUE;

	clr_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_IDLE);

	wait_flag = FLG_IAMOVIEPLAY_READ_STOP | FLG_IAMOVIEPLAY_READ_TRIG;

	// coverity[no_escape]
	while (movieplay_filein_read_tsk_run) {
		set_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&flag, IAMOVIEPLAY_READ_FLG_ID, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_IDLE);

		if (flag & FLG_IAMOVIEPLAY_READ_STOP) {
			break;
		}

		if (flag & FLG_IAMOVIEPLAY_READ_TRIG) {
#if 1//_TODO_
            IAMOVIEPLAY_BSDEMUX_BS_INFO *p_vdo_cbinfo = &g_vdo_bsdemux_cbinfo;
            IAMOVIEPLAY_BSDEMUX_BS_INFO *p_aud_cbinfo = &g_aud_bsdemux_cbinfo;
			IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
			// trigger filein read file check
        	FILEIN_CB_INFO cb_info = {0};
            HD_PATH_ID     path_id = 0;

			// video filein
			if(tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {

	    		/* ts */
				if(tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS){
					cb_info.event = FILEIN_IN_TYPE_READ_ONEBLK;
				}
				else{
					cb_info.event = FILEIN_IN_TYPE_CHK_FILEBLK;
				}

			} else {
	    		cb_info.event = FILEIN_IN_TYPE_READ_ONEFRM;
			}
        	cb_info.bs_offset = p_vdo_cbinfo->bs_offset;
        	cb_info.bs_size   = p_vdo_cbinfo->bs_size;

            //DBG_DUMP("bs_of:%ld\r\n", cb_info.bs_offset);
            //DBG_DUMP("bs_sz:%d\r\n", cb_info.bs_size);
            #if 0
			DBGH(p_vdo_cbinfo->bs_addr);
            DBGH(cb_info.bs_offset);
			DBGH(cb_info.bs_size);
			#endif
			// callback reading file
			UINT32 bs_size = cb_info.bs_size;

			if(hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0) != HD_OK){

				MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
				MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

				DBG_ERR("hd_filein_push_in_buf failed!(video bs offset = %llx , size = %lx)\n",
						p_vdo_cbinfo->bs_offset,
						p_vdo_cbinfo->bs_size
				);

				info.ret = HD_ERR_BAD_DATA;
				info.is_video = TRUE;
				info.frame_idx = tsk_obj->vdo_idx;

				(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);

				continue;
			}

			if(bs_size != p_vdo_cbinfo->bs_size){
				DBG_WRN("Filein overrun, skip frame\n");
				continue;
			}

			// assign I-frame bs_addr.
			if(cb_info.event == FILEIN_IN_TYPE_READ_ONEFRM) {
				MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
				MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

				p_vdo_cbinfo->bs_addr = cb_info.uiThisAddr;

				if (tsk_obj->file_fmt != IAMOVIEPLAY_FILEFMT_TS && p_obj->vdo_codec == NMEDIAPLAY_DEC_H265) {

					if (tsk_obj->vdo_idx % tsk_obj->gop == 0) {

						_iamovieplay_update_start_code(
								(UINT8 *)p_vdo_cbinfo->bs_addr,
								p_vdo_cbinfo->bs_size,
								p_obj->desc_size
						);

						/* skip desc before pushing into hdal videodec */
						p_vdo_cbinfo->bs_addr += p_obj->desc_size;
					}
					else{
						IAMMOVIEPLAY_COMM_DUMP("skip p frame(idx:%lu) in FF(FR) mode\r\n", tsk_obj->vdo_idx);
						continue;
					}

				}
			}

			DBG_IND("accum_of:%lx\r\n", cb_info.read_accum_offset);
			DBG_IND("used_of:%lx\r\n", cb_info.last_used_offset);

			if(		tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL &&
					tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD
			) {
				// audio filein
	    		//cb_info.event = FILEIN_IN_TYPE_CHK_FILEBLK;
				if(tsk_obj->aud_en){

					cb_info.bs_offset = p_aud_cbinfo->bs_offset;
					cb_info.bs_size   = p_aud_cbinfo->bs_size;

					if(hd_filein_push_in_buf(path_id, (UINT32*)&cb_info, 0) != HD_OK){

						MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
						MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

						DBG_ERR("hd_filein_push_in_buf failed!(audio bs offset = %llx , size = %lx)\n",
								p_aud_cbinfo->bs_offset,
								p_aud_cbinfo->bs_size
						);

						info.ret = HD_ERR_BAD_DATA;
						info.is_video = FALSE;
						info.frame_idx = tsk_obj->aud_idx;

						(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);

						continue;
					}

					DBG_IND("accum_of:%lx\r\n", cb_info.read_accum_offset);
					DBG_IND("used_of:%lx\r\n", cb_info.last_used_offset);
				}
			} else {
				// FF(FR) Step4. output video bs (trigger decode)
				DBG_IND("FF(FR) Step4\r\n");
				set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_VBS_OUT);
			}
#endif
		}
	} // end of while loop

	DBG_FUNC("end\r\n");

	is_movieplay_filein_read_tsk_running = FALSE;
	THREAD_RETURN(0);
}

THREAD_RETTYPE _iamovieplay_Demux_Tsk(void)
{
	DBG_FUNC("start\r\n");

	FLGPTN			flag = 0, wait_flag = 0;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	IAMOVIEPLAY_BSDEMUX_BS_INFO *p_vdo_cbinfo = &g_vdo_bsdemux_cbinfo;
	HD_VIDEODEC_BS video_bitstream = {0};
	HD_VIDEO_FRAME video_frame = {0};
	HD_RESULT      ret = HD_OK;
    HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
    IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
    extern MOVIEPLAY_FILE_DECRYPT g_movply_file_decrypt;

	/* decrypt */
	BOOL is_encrypted = (g_movply_file_decrypt.type == MOVIEPLAY_DECRYPT_TYPE_NONE) ? FALSE : TRUE;
	UINT32 decrypt_type = g_movply_file_decrypt.type;

	THREAD_ENTRY(); //kent_tsk();
	is_movieplay_demux_tsk_running = TRUE;

	clr_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE);

	wait_flag = FLG_IAMOVIEPLAY_DEMUX_START |
				FLG_IAMOVIEPLAY_DEMUX_STOP |
				FLG_IAMOVIEPLAY_DEMUX_VBS_DEMUX |
				FLG_IAMOVIEPLAY_DEMUX_ABS_DEMUX |
				FLG_IAMOVIEPLAY_DEMUX_VBS_OUT	|
				FLG_IAMOVIEPLAY_DEMUX_IFRM_ONESHOT;

	while (movieplay_demux_tsk_run) {
		set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&flag, IAMOVIEPLAY_DEMUX_FLG_ID, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE);

		if (flag & FLG_IAMOVIEPLAY_DEMUX_STOP) {
            tsk_obj->vdo_idx = 0; // reset video frame index.
			break;
		}

		if (flag & FLG_IAMOVIEPLAY_DEMUX_START) {
			DBG_DUMP("start\r\n");
			_iamovieplay_Start();
		}

		if (flag & FLG_IAMOVIEPLAY_DEMUX_VBS_OUT) {
            DBG_IND("[NMP_BsDmx]vout\r\n");

            /* check is bs encrypted */
			if (is_encrypted) {

			    BOOL need_decrypt = FALSE;
			    BOOL is_i_frame = (tsk_obj->vdo_idx % tsk_obj->gop == 0) ? TRUE : FALSE;

				if(	((TRUE == is_i_frame) && (decrypt_type & MOVIEPLAY_DECRYPT_TYPE_I_FRAME)) ||
					((FALSE == is_i_frame) && (decrypt_type & MOVIEPLAY_DECRYPT_TYPE_P_FRAME))
				){
					need_decrypt = TRUE;
				}

				if(TRUE == need_decrypt){

					_iamovieplay_decrypt_hex_dump((TRUE == is_i_frame)?MOVIEPLAY_DECRYPT_TYPE_I_FRAME: MOVIEPLAY_DECRYPT_TYPE_P_FRAME, "FLG_IAMOVIEPLAY_DEMUX_VBS_OUT before decrypt", p_vdo_cbinfo->bs_addr, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);

					/* pcbinfo->bs_addr is a real bs start address from bsdemux, it means no need to skip desc size for H265 */
					_iamovieplay_bs_decrypt(p_vdo_cbinfo->bs_addr, p_vdo_cbinfo->bs_size);

					_iamovieplay_decrypt_hex_dump((TRUE == is_i_frame)?MOVIEPLAY_DECRYPT_TYPE_I_FRAME: MOVIEPLAY_DECRYPT_TYPE_P_FRAME, "FLG_IAMOVIEPLAY_DEMUX_VBS_OUT after decrypt ", p_vdo_cbinfo->bs_addr, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);
				}
			}

			ret = get_video_frame_buf(&video_frame);
			if (ret != HD_OK) {
				DBG_ERR("get video frame error(%d) !!\r\n", ret);
			}

			/* config video bs */
			video_bitstream.sign          = MAKEFOURCC('V','S','T','M');
			video_bitstream.p_next        = NULL;
			video_bitstream.ddr_id        = ddr_id;
			video_bitstream.vcodec_format = (p_obj->vdo_codec == MEDIAPLAY_VIDEO_H264) ? HD_CODEC_TYPE_H264 : HD_CODEC_TYPE_H265;
			video_bitstream.timestamp     = hd_gettime_us();
			video_bitstream.blk           = -2;
			video_bitstream.count         = 0;
			video_bitstream.phy_addr      = imageapp_movieplay_get_hd_phy_addr((void *)p_vdo_cbinfo->bs_addr);
			video_bitstream.size          = p_vdo_cbinfo->bs_size;

			UINT32* ptr = (UINT32*)p_vdo_cbinfo->bs_addr;
			ptr[0] = 0x01000000;

			/* data is written by CPU, flush CPU cache to PHY memory */
			hd_common_mem_flush_cache((void *)p_vdo_cbinfo->bs_addr, p_vdo_cbinfo->bs_size);

			DBG_IND("FF(FR) Step5\r\n");

			DBG_IND("hd_videodec_push_in_buf(idx:%lu va:%lx pa:%lx size:%lx)\n",
					tsk_obj->vdo_idx,
					p_vdo_cbinfo->bs_addr,
					video_bitstream.phy_addr,
					video_bitstream.size);

			ret = hd_videodec_push_in_buf(p_obj->vdec_path, &video_bitstream, &video_frame, -1);
			if (ret != HD_OK) {
				DBG_ERR("push_in error(%d) !!\r\n", ret);
				// release video frame buf

				if (p_play_info->event_cb) {

					MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

					info.ret = ret;
					info.is_video = TRUE;
					info.frame_idx = 0; /* reserved, hdal currently not support store user data */

					(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);
				}


				/* release video frame buf if error occurred */
				ret = hd_videodec_release_out_buf(p_obj->vdec_path, &video_frame);
				if (ret != HD_OK) {
					DBG_ERR("release video frame error(%d)!\r\n", ret);
				}
				break;
			}

			#if 0 // Save decoded YUV RAW file.
			{
				static FST_FILE fp = NULL;
				char sFileName[256];
				UINT32 uiRawYSize = p_obj->width * p_obj->height;
				UINT32 uiRawUVSize = uiRawYSize / 2;
				DBGD(uiRawYSize);
				DBG_DUMP("[VDODEC][%d] dump decoded yuv frame, w = %d, h = %d, raw addr = 0x%08x, raw size = %d\r\n", 0, p_obj->width, p_obj->height, video_frame.phy_addr[HD_VIDEO_PINDEX_Y], uiRawYSize);
				snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.y", p_obj->width, p_obj->height);
				fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
				if (fp != NULL) {
					if (FileSys_WriteFile(fp, (UINT8 *)&video_frame.phy_addr[HD_VIDEO_PINDEX_Y], &uiRawYSize, 0, NULL) != FST_STA_OK) {
						DBG_ERR("[VDODEC][%d] dump decoded y frame err, write file fail\r\n", 0);
					} else {
						DBG_DUMP("[VDODEC][%d] write file ok\r\n", 0);
					}
					FileSys_CloseFile(fp);
				} else {
					DBG_ERR("[VDODEC][%d] dump decoded y frame err, invalid file handle\r\n", 0);
				}

				sFileName[0] = '\0';
				snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.uv", p_obj->width, p_obj->height);
				fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
				if (fp != NULL) {
					if (FileSys_WriteFile(fp, (UINT8 *)&video_frame.phy_addr[HD_VIDEO_PINDEX_UV], &uiRawUVSize, 0, NULL) != FST_STA_OK) {
						DBG_ERR("[VDODEC][%d] dump decoded uv frame err, write file fail\r\n", 0);
					} else {
						DBG_DUMP("[VDODEC][%d] write file ok\r\n", 0);
					}
					FileSys_CloseFile(fp);
				} else {
					DBG_ERR("[VDODEC][%d] dump decoded uv frame err, invalid file handle\r\n", 0);
				}
			}
			#endif

			// release video frame buf
			ret = hd_videodec_release_out_buf(p_obj->vdec_path, &video_frame);
			if (ret != HD_OK) {
				printf("release video frame error(%d) !!\r\n\r\n", ret);
			}

			#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
			DBG_IND("FLG_IAMOVIEPLAY_DISP_TRIG\r\n");
			set_flg(IAMOVIEPLAY_DISP_FLG_ID, FLG_IAMOVIEPLAY_DISP_TRIG);
			#endif
		}

		// Handle audio bs first.
		if (flag & FLG_IAMOVIEPLAY_DEMUX_ABS_DEMUX) {
         	HD_PATH_ID path_id = 0;
           	HD_BSDEMUX_BUF push_in_buf = {0};
        	HD_RESULT ret;

			IAMMOVIEPLAY_COMM_DUMP("abs demux:%d\r\n", tsk_obj->aud_idx);
    		// audio bs
    		push_in_buf.sign = MAKEFOURCC('B','S','D','X');
    		push_in_buf.bs_type = HD_BSDEMUX_BS_TPYE_AUDIO;
    		push_in_buf.index = tsk_obj->aud_idx;
    		ret = hd_bsdemux_push_in_buf(path_id, &push_in_buf, 0);
    		if (ret != HD_OK) {
    			DBG_ERR("push_in audio buf fail(%d)", ret);
    		}
		}

		if (flag & FLG_IAMOVIEPLAY_DEMUX_VBS_DEMUX) {
         	HD_PATH_ID path_id = 0;
           	HD_BSDEMUX_BUF push_in_buf = {0};
        	HD_RESULT ret;

			IAMMOVIEPLAY_COMM_DUMP("vbs demux:%d\r\n", tsk_obj->vdo_idx);
    		// video bs
    		push_in_buf.sign = MAKEFOURCC('B','S','D','X');
    		push_in_buf.bs_type = HD_BSDEMUX_BS_TPYE_VIDEO;
    		push_in_buf.index = tsk_obj->vdo_idx;
			DBG_IND("FF(FR) Step2\r\n");
    		ret = hd_bsdemux_push_in_buf(path_id, &push_in_buf, 0);
    		if (ret != HD_OK) {
    			DBG_ERR("push_in video buf fail(%d)", ret);
    		}
		}

		if (flag & FLG_IAMOVIEPLAY_DEMUX_IFRM_ONESHOT) {
			HD_RESULT ret;

			ret = _iamovieplay_IFrameInOneShot();
    		if (ret != HD_OK) {
    			DBG_ERR("push_in video buf fail(%d)", ret);
    		}
		}

	} /* end of while loop */

	is_movieplay_demux_tsk_running = FALSE;

	DBG_FUNC("END\r\n");

	THREAD_RETURN(0); //ext_tsk();
}

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
THREAD_RETTYPE _iamovieplay_Disp_Tsk(void)
{
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	HD_VIDEO_FRAME video_frame = {0};
	HD_RESULT ret = 0;
	const INT32 wait_ms = 500; /* pull out timeout in ms */

	DBG_DUMP("_iamovieplay_Disp_Tsk() start\r\n");

	THREAD_ENTRY(); //kent_tsk();
	is_movieplay_disp_tsk_running = TRUE;

	// coverity[no_escape]
	while (movieplay_disp_tsk_run) {

		memset(&video_frame, 0, sizeof(HD_VIDEO_FRAME));

		ret = hd_videodec_pull_out_buf(p_obj->vdec_path, &video_frame, wait_ms);
		if (ret != HD_OK){

			if(tsk_obj->state == IAMOVIEPLAY_STATE_PLAYING){

				if(ret != HD_ERR_TIMEDOUT){
					DBG_ERR("hd_videodec_pull_out_buf error(%d)!!\r\n", ret);
				}
				
				if (p_play_info->event_cb) {

					MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

					info.ret = ret;
					info.is_video = TRUE;
					info.frame_idx = 0; /* reserved, hdal currently not support store user data */

					(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);
				}
			}

			continue;
		}

		if (p_play_info->event_cb) {
			(p_play_info->event_cb)(MOVIEPLAY_EVENT_ONE_DISPLAYFRAME, (UINT32)&video_frame, 0, 0);
		}

		ret = hd_videodec_release_out_buf(p_obj->vdec_path, &video_frame);
		if (ret != HD_OK) {
			DBG_ERR("hd_videodec_release_out_buf failed(%d)!!\r\n", ret);
		}
	} // end of while loop


	/* drain pull out buffer until failed in case hdal dump mem not all freed message */

	unsigned int drain_cnt = 0;

	do{

		memset(&video_frame, 0, sizeof(HD_VIDEO_FRAME));
		ret = hd_videodec_pull_out_buf(p_obj->vdec_path, &video_frame, wait_ms);
		if (ret != HD_OK)
			break;

		drain_cnt++;

		ret = hd_videodec_release_out_buf(p_obj->vdec_path, &video_frame);
		if (ret != HD_OK) {
			DBG_ERR("hd_videodec_release_out_buf failed(%d)!!\r\n", ret);
		}

	} while(1);

	IAMMOVIEPLAY_COMM_DUMP("hd_videodec_pull_out_buf drain_cnt = %u\r\n", drain_cnt);

	is_movieplay_disp_tsk_running = FALSE;
	THREAD_RETURN(0);
}

#endif

THREAD_RETTYPE _iamovieplay_audio_playback_Tsk(void)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_BS  bs_in_buf = {0};
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 bs_size = 0, bs_addr = 0;
	FLGPTN uiFlag = 0;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	FLGPTN wait_flag = 0;
	extern MOVIEPLAY_FILE_DECRYPT g_movply_file_decrypt;

	/* decrypt */
	BOOL is_encrypted = (g_movply_file_decrypt.type == MOVIEPLAY_DECRYPT_TYPE_NONE) ? FALSE : TRUE;
	UINT32 decrypt_type = g_movply_file_decrypt.type;

	DBG_DUMP("_iamovieplay_audio_playback_Tsk() start\r\n");

	THREAD_ENTRY(); //kent_tsk();
	is_movieplay_audio_pb_tsk_running = TRUE;

	clr_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE);

#if 0
	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("get block fail, blk = 0x%x\n", blk);
	}
	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk(0x%x)\n", blk);
	}
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
		if (va == 0) {
			DBG_ERR("get va fail, va(0x%x)\n", blk);
		}
		// allocate bs buf
		bs_buf_start = va;
		bs_buf_curr = bs_buf_start;
		bs_buf_end = bs_buf_start + blk_size;
		DBG_ERR("alloc bs_buf: start(0x%x) curr(0x%x) end(0x%x) size(0x%x)\n", bs_buf_start, bs_buf_curr, bs_buf_end, blk_size);
	}
#endif
	wait_flag = FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC |
				FLG_IAMOVIEPLAY_AUDIO_PB_PAUSE |
				FLG_IAMOVIEPLAY_AUDIO_PB_RESUME |
				FLG_IAMOVIEPLAY_AUDIO_PB_STOP;

	//vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC);

#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE

	char fileName[] = "A:\\iamovieplay_dbg_audio_file_encoded.dat";

	g_movply_dbg_audio_file_encoded = FileSys_OpenFile(fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);

	if(g_movply_dbg_audio_file_encoded == NULL){
		DBG_ERR("open audio dbg file(%s) failed\r\n", fileName);
	}

#endif

	while (movieplay_audio_playback_tsk_run) {
	    IAMOVIEPLAY_BSDEMUX_BS_INFO *pcbinfo = &g_aud_bsdemux_cbinfo;

		PROFILE_TASK_IDLE();
		// wait for trigger
		set_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_IDLE);

		vos_flag_wait(&uiFlag, IAMOVIEPLAY_AUDIO_PB_FLG_ID, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();


		if(uiFlag & FLG_IAMOVIEPLAY_AUDIO_PB_STOP){
			break;
		}

		clr_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_IDLE);

		if (uiFlag & FLG_IAMOVIEPLAY_AUDIO_PB_PAUSE) {

			PROFILE_TASK_IDLE();
			// wait for trigger
			bs_addr = 0;
			bs_size = 0;
			set_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_IDLE);

			FLGPTN uiFlag2 = 0;

			vos_flag_wait(&uiFlag2, IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_RESUME | FLG_IAMOVIEPLAY_AUDIO_PB_STOP, TWF_ORW | TWF_CLR);
			clr_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_PAUSE);

			if(uiFlag & FLG_IAMOVIEPLAY_AUDIO_PB_STOP)
				break;

			PROFILE_TASK_BUSY();


			clr_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_IDLE);
		}

		// trigger filein
		//set_flg(IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_TRIG);

		// trigger bsdemux audio bs
		if(tsk_obj->state == IAMOVIEPLAY_STATE_PLAYING && tsk_obj->aud_en) {

			if (((p_obj->file_fmt != _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_idx < tsk_obj->aud_ttfrm)) ||
				((p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_frm_eof == FALSE))
			){
				set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_ABS_DEMUX);
			}
		}

		// trigger audio decode.
		if (tsk_obj->state == IAMOVIEPLAY_STATE_PLAYING && (uiFlag & FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC)) {

			// get bs size
			if (pcbinfo->bs_size == 0 || pcbinfo->bs_size > 12800/*BITSTREAM_SIZE*/) {
				DBG_WRN("Invalid bs_size(%d)\n", bs_size);
				continue;
			}

			if (bs_addr == pcbinfo->bs_addr) {

				IAMMOVIEPLAY_COMM_DUMP("ignore duplicate bs_addr(%lx)\n", bs_addr);

				if (((p_obj->file_fmt != _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_idx < tsk_obj->aud_ttfrm)) ||
					((p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_frm_eof == FALSE))
				){
					vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC);
					vos_util_delay_us(1000);
				}
				continue;
			}


			if(is_encrypted && (decrypt_type & MOVIEPLAY_DECRYPT_TYPE_AUDIO) ){

				_iamovieplay_decrypt_hex_dump(MOVIEPLAY_DECRYPT_TYPE_AUDIO, "FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC before decrypt", pcbinfo->bs_addr_va, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);

				/************************************************************************************************
				 * MP4 container has no ADTS header for AAC format, decrypt offset is different from encrypt.
				 *
				 * Encrypt offset: ADTS(7) + DATA(9) = 16
				 * Decrypt offset: DATA(9)
				 *
				 ************************************************************************************************/
				if ((tsk_obj->file_fmt != IAMOVIEPLAY_FILEFMT_TS) && p_obj->aud_codec == MEDIAAUDIO_CODEC_MP4A) {
					_iamovieplay_bs_decrypt(pcbinfo->bs_addr_va - IAMMOVIEPLAY_ADTS_HEADER_SIZE, pcbinfo->bs_size);
				}
				else{
					_iamovieplay_bs_decrypt(pcbinfo->bs_addr_va , pcbinfo->bs_size);
				}

				_iamovieplay_decrypt_hex_dump(MOVIEPLAY_DECRYPT_TYPE_AUDIO, "FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC after decrypt ", pcbinfo->bs_addr_va, IAMMOVIEPLAY_DECRYPT_DUMP_LEN);

			}

			bs_size = pcbinfo->bs_size;
			bs_addr = pcbinfo->bs_addr;

			bs_in_buf.sign = MAKEFOURCC('A','S','T','M');
			bs_in_buf.acodec_format = p_obj->aud_codec;
			bs_in_buf.phy_addr = bs_addr; // needs to add offset
			bs_in_buf.size = bs_size;
			bs_in_buf.ddr_id = ddr_id;
			bs_in_buf.timestamp = hd_gettime_us();

			hd_common_mem_flush_cache((void *)pcbinfo->bs_addr_va, bs_size);

#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE

			if(g_movply_dbg_audio_file_encoded){

				/* AAC needs ADTS header */
				if(bs_in_buf.acodec_format == HD_AUDIO_CODEC_AAC){
					static UINT8 header[IAMMOVIEPLAY_ADTS_HEADER_SIZE] = {0};
					UINT32 size = sizeof(header);

					addADTStoPacket(header, bs_size, p_obj->aud_chs, p_obj->aud_sr);
					FileSys_WriteFile(g_movply_dbg_audio_file_encoded, header, &size, 0, NULL);
				}

				FileSys_WriteFile(g_movply_dbg_audio_file_encoded, (UINT8*)pcbinfo->bs_addr_va, &bs_size, 0, NULL);
			}
#endif

			// push in buffer
			ret = hd_audiodec_push_in_buf(p_obj->adec_path, &bs_in_buf, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				DBG_ERR("hd_audiodec_push_in_buf fail, ret(%d)\n", ret);

				if (p_play_info->event_cb) {

					MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

					info.ret = ret;
					info.is_video = FALSE;
					info.frame_idx = 0; /* reserved, hdal currently not support store user data */

					(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);
				}

			}
		}

#if 0
		bs_buf_curr += ALIGN_CEIL_4(bs_size); // shift to next
#endif
	}

#if 0
	// release memory
	hd_common_mem_munmap((void*)va, blk_size);
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		DBG_ERR("release blk fail, ret(%d)\n", ret);
	}
#endif


#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE

	if(g_movply_dbg_audio_file_encoded){

		FileSys_FlushFile(g_movply_dbg_audio_file_encoded);
		FileSys_CloseFile(g_movply_dbg_audio_file_encoded);
		g_movply_dbg_audio_file_encoded = NULL;
	}

#endif


	is_movieplay_audio_pb_tsk_running = FALSE;

	THREAD_RETURN(0); //ext_tsk();
}

THREAD_RETTYPE _iamovieplay_audio_decode_Tsk(void)
{
	HD_RESULT ret = HD_OK;
	BOOL done = FALSE;
	HD_AUDIO_FRAME  data_pull = {0};
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	const INT32 wait_ms = 500; /* pull out timeout in ms */

#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE
	UINT32 vir_addr_main = 0;
	HD_AUDIODEC_BUFINFO phy_buf_main = {0};
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
#endif

	DBG_DUMP("_iamovieplay_audio_decode_Tsk() start\r\n");

	THREAD_ENTRY(); //kent_tsk();
	is_movieplay_audio_decode_tsk_running = TRUE;

	clr_flg(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_DECODE_IDLE);


#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE

	hd_audiodec_get(p_obj->adec_path, HD_AUDIODEC_PARAM_BUFINFO, &phy_buf_main);
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	char fileName[] = "A:\\iamovieplay_dbg_audio_file_decoded.pcm";
	g_movply_dbg_audio_file_decoded = FileSys_OpenFile(fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);

	if(g_movply_dbg_audio_file_decoded == NULL){
		DBG_ERR("open audio dbg file(%s) failed\r\n", fileName);
	}

#endif

	// coverity[no_escape]
	while (movieplay_audio_decode_tsk_run) {
		set_flg(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_DECODE_IDLE);

		PROFILE_TASK_IDLE();
		// pull data
		ret = hd_audiodec_pull_out_buf(p_obj->adec_path, &data_pull, wait_ms); // >1 = timeout mode

		if (ret == HD_OK) {
			tsk_obj->aud_count++;

			if (((p_obj->file_fmt != _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_idx < tsk_obj->aud_ttfrm)) ||
				((p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_frm_eof == FALSE))
			){

#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE

				UINT8* buf = (UINT8*)PHY2VIRT_MAIN(data_pull.phy_addr[0]);

				if(g_movply_dbg_audio_file_decoded){
					FileSys_WriteFile(g_movply_dbg_audio_file_decoded, buf, &data_pull.size, 0, NULL);
				}

#endif
				ret = hd_audioout_push_in_buf(p_obj->aout_path, &data_pull, -1);
				if (ret != HD_OK) {
					DBG_ERR("hd_audioout_push_in_buf fail, ret(%d)\n", ret);

					if (p_play_info->event_cb) {

						MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

						info.ret = ret;
						info.is_video = FALSE;
						info.frame_idx = 0; /* reserved, hdal currently not support store user data */

						(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);
					}
				}
			}
			vendor_audioout_get(p_obj->aout_ctrl, VENDOR_AUDIOOUT_ITEM_PRELOAD_DONE, (VOID *)&done);

			if (done != TRUE) { // trigger video demux and deocde if "done" is TRUE to achieve AV sync.
//				DBG_WRN("Not preload done\n");
			}

			// release data
			ret = hd_audiodec_release_out_buf(p_obj->adec_path, &data_pull);
			if (ret != HD_OK) {
				DBG_ERR("release_ouf_buf fail, ret(%d)\r\n", ret);
			}

			// trigger next decode
			if (((p_obj->file_fmt != _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_idx < tsk_obj->aud_ttfrm)) ||
				((p_obj->file_fmt == _MOVPLAY_CFG_FILE_FORMAT_TS) && (tsk_obj->aud_frm_eof == FALSE))
			){
				vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC);
			}
		} else {

			if(ret != HD_ERR_TIMEDOUT){
				DBG_ERR("hd_audiodec_pull_out_buf fail(%d)\r\n", ret);
			}

			if (p_play_info->event_cb) {

				MOVIEPLAY_EVENT_DECODE_ERR_INFO info = {0};

				info.ret = ret;
				info.is_video = FALSE;
				info.frame_idx = 0;

				(p_play_info->event_cb)(MOVIEPLAY_EVENT_DECODE_ERR, (UINT32)&info, 0, 0);
			}

			vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC);
		}
		PROFILE_TASK_BUSY();

		clr_flg(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_DECODE_IDLE);
	} // end of while loop


#if IAMMOVIEPLAY_DBG_RECORD_AUDIO_FILE

	if(g_movply_dbg_audio_file_decoded){

		FileSys_FlushFile(g_movply_dbg_audio_file_decoded);
		FileSys_CloseFile(g_movply_dbg_audio_file_decoded);
		g_movply_dbg_audio_file_decoded = NULL;
	}

#endif

	is_movieplay_audio_decode_tsk_running = FALSE;
	THREAD_RETURN(0);
}

ER iamovieplay_SetVdoTimerIntval(UINT32 Id)
{
	UINT32 time_in_us = 0;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;

	if (p_obj->vdo_fr == 0) {
		DBG_ERR("[%d] invalid vdo_fr = %d\r\n", Id, p_obj->vdo_fr);
		return E_PAR;
	}


	if (p_obj->vdo_en) {
		if (p_obj->vdo_fr < 1000) {
			time_in_us = IAMOVIEPLAY_US_IN_SECOND / p_obj->vdo_fr;
		} else {
			time_in_us = (IAMOVIEPLAY_US_IN_SECOND * 1000) / p_obj->vdo_fr;
		}

#if 0 // Temporarily set timer interval as double to make sure all I/P frame be decoded well.
		time_in_us *= 3;
#endif
		tsk_obj->timer_intval = time_in_us;
		IAMMOVIEPLAY_COMM_DUMP("set timer interval %d us\r\n", tsk_obj->timer_intval);
	}

	return E_OK;
}

ER iamovieplay_TskStart(void)
{
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	ER ret = E_OK;

	movieplay_filein_read_tsk_run = FALSE;
	movieplay_demux_tsk_run = FALSE;
	movieplay_audio_playback_tsk_run = FALSE;
	movieplay_audio_decode_tsk_run = FALSE;
	is_movieplay_demux_tsk_running = FALSE;
	is_movieplay_filein_read_tsk_running = FALSE;
	is_movieplay_audio_pb_tsk_running = FALSE;
	is_movieplay_audio_decode_tsk_running = FALSE;

	if ((IAMOVIEPLAY_READ_TSK_ID = vos_task_create(_iamovieplay_Read_Tsk, 0, "IAMoviePlay_FileReadTsk", IAMOVIEPLAY_READ_PRI, IAMOVIEPLAY_READ_STKSIZE)) == 0) {
		DBG_ERR("IAMovie_AEPullTsk create failed.\r\n");
        ret |= E_SYS;
	} else {
		movieplay_filein_read_tsk_run = TRUE;
	    vos_task_resume(IAMOVIEPLAY_READ_TSK_ID);
	}

	if ((IAMOVIEPLAY_DEMUX_TSK_ID = vos_task_create(_iamovieplay_Demux_Tsk, 0, "IAMoviePlay_DemuxTsk", IAMOVIEPLAY_DEMUX_PRI, IAMOVIEPLAY_DEMUX_STKSIZE)) == 0) {
		DBG_ERR("IAMovie_VEPullTsk create failed.\r\n");
        ret |= E_SYS;
	} else {
		movieplay_demux_tsk_run = TRUE;
	    vos_task_resume(IAMOVIEPLAY_DEMUX_TSK_ID);
	}

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)
	if ((IAMOVIEPLAY_DISP_TSK_ID = vos_task_create(_iamovieplay_Disp_Tsk, 0, "IAMoviePlay_DispTsk", IAMOVIEPLAY_DISP_PRI, IAMOVIEPLAY_DISP_STKSIZE)) == 0) {
		DBG_ERR("IAMovie_VEPullTsk create failed.\r\n");
		ret |= E_SYS;
	} else {
		movieplay_disp_tsk_run = TRUE;
		vos_task_resume(IAMOVIEPLAY_DISP_TSK_ID);
	}
#endif

	if (p_obj->aud_en) {
		// create audio decode_thread (push_in bitstream)
		if ((IAMOVIEPLAY_AUDIO_PB_TSK_ID = vos_task_create(_iamovieplay_audio_playback_Tsk, 0, "IAMoviePlay_AudioPBTsk", IAMOVIEPLAY_AUDIO_PB_PRI, IAMOVIEPLAY_AUDIO_PB_STKSIZE)) == 0) {
			DBG_ERR("IAMoviePlay_AudioPBTsk create failed.\r\n");
	        ret |= E_SYS;
		} else {
			movieplay_audio_playback_tsk_run = TRUE;
		    vos_task_resume(IAMOVIEPLAY_AUDIO_PB_TSK_ID);
		}

		// create audio decode_thread (pull_out frame)
		if ((IAMOVIEPLAY_AUDIO_DECODE_TSK_ID = vos_task_create(_iamovieplay_audio_decode_Tsk, 0, "IAMoviePlay_audio_decode_Tsk", IAMOVIEPLAY_AUDIO_DECODE_PRI, IAMOVIEPLAY_AUDIO_DECODE_STKSIZE)) == 0) {
			DBG_ERR("IAMoviePlay_audio_decode_Tsk create failed.\r\n");
	        ret |= E_SYS;
		} else {
			movieplay_audio_decode_tsk_run = TRUE;
		    vos_task_resume(IAMOVIEPLAY_AUDIO_DECODE_TSK_ID);
		}
	}

	return E_OK;
}

ER iamovieplay_TskDestroy(void)
{
	MOVIEPLAY_IMAGE_STREAM_INFO *pImageStream = g_pImageStream;
	MOVIEPLAY_PLAY_OBJ *p_obj = &pImageStream->play_obj;
	UINT32 delay_cnt/*, tsk_status*/;
	FLGPTN flag = 0;

	if(IAMOVIEPLAY_DEMUX_FLG_ID){

		DBG_IND("bsdmx idle ...\r\n");
		wai_flg(&flag, IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE, TWF_ORW);
		DBG_IND("bsdmx idle ... ok\r\n");
	}

	if(IAMOVIEPLAY_DEMUX_TSK_ID){

		movieplay_demux_tsk_run = FALSE;
		delay_cnt = 10;

		while (is_movieplay_demux_tsk_running && delay_cnt) {
			vos_util_delay_us(10000);
			delay_cnt --;
		}

		if(is_movieplay_demux_tsk_running){
			vos_task_destroy(IAMOVIEPLAY_DEMUX_TSK_ID);
		}

		IAMOVIEPLAY_DEMUX_TSK_ID = 0;
	}

	if(IAMOVIEPLAY_READ_FLG_ID){

		DBG_IND("fr idle ...\r\n");
		wai_flg(&flag, IAMOVIEPLAY_READ_FLG_ID, FLG_IAMOVIEPLAY_READ_IDLE, TWF_ORW);
		DBG_IND("fr idle ... ok\r\n");
	}


	if(IAMOVIEPLAY_READ_TSK_ID){

		delay_cnt = 10;
		movieplay_filein_read_tsk_run = FALSE;

		while (is_movieplay_filein_read_tsk_running && delay_cnt) {
			vos_util_delay_us(10000);
			delay_cnt --;
		}

		if(is_movieplay_filein_read_tsk_running){
			vos_task_destroy(IAMOVIEPLAY_READ_TSK_ID);
			is_movieplay_filein_read_tsk_running = FALSE;
		}

		IAMOVIEPLAY_READ_TSK_ID = 0;
	}

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)

	if(IAMOVIEPLAY_DISP_TSK_ID){

		delay_cnt = 1000;
		movieplay_disp_tsk_run = FALSE;

		while (is_movieplay_disp_tsk_running && delay_cnt) {
			vos_util_delay_us(10000);
			delay_cnt --;
		}

		if(is_movieplay_disp_tsk_running){
			vos_task_destroy(IAMOVIEPLAY_DISP_TSK_ID);
			is_movieplay_disp_tsk_running = FALSE;
		}

		IAMOVIEPLAY_DISP_TSK_ID = 0;
	}

#endif

	if(p_obj->aud_en) {


		if(IAMOVIEPLAY_AUDIO_PB_FLG_ID){

			DBG_IND("audio PB idle ...\r\n");
			wai_flg(&flag, IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_IDLE, TWF_ORW);
			set_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_STOP);
			DBG_IND("audio PB idle ... ok\r\n");
		}

		if(IAMOVIEPLAY_AUDIO_PB_TSK_ID){

			delay_cnt = 1000;
			movieplay_audio_playback_tsk_run = FALSE;

			while (is_movieplay_audio_pb_tsk_running && delay_cnt) {
				vos_util_delay_us(10000);
				delay_cnt --;
			}

			if(is_movieplay_audio_pb_tsk_running){
				vos_task_destroy(IAMOVIEPLAY_AUDIO_PB_TSK_ID);
				is_movieplay_audio_pb_tsk_running = FALSE;
			}

			IAMOVIEPLAY_AUDIO_PB_TSK_ID = 0;
		}

		if(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID){

		DBG_IND("audio decode idle ...\r\n");
	    wai_flg(&flag, IAMOVIEPLAY_AUDIO_DECODE_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_DECODE_IDLE, TWF_ORW);
		DBG_IND("audio decode idle ... ok\r\n");

		}


		if(IAMOVIEPLAY_AUDIO_DECODE_TSK_ID){

			delay_cnt = 1000;
			movieplay_audio_decode_tsk_run = FALSE;

			while (is_movieplay_audio_decode_tsk_running && delay_cnt) {
				vos_util_delay_us(10000);
				delay_cnt --;
			}

			if(is_movieplay_audio_decode_tsk_running){
				vos_task_destroy(IAMOVIEPLAY_AUDIO_DECODE_TSK_ID);
				is_movieplay_audio_decode_tsk_running = FALSE;
			}

			IAMOVIEPLAY_AUDIO_DECODE_TSK_ID = 0;
		}
	}

	return E_OK;
}

UINT32 iamovieplay_get_hd_phy_addr(void *va)
{
    if (va) {
        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
        vir_meminfo.va = va;
        if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) == HD_OK) {
                return vir_meminfo.pa;
        }
    }

    return 0;
}

HD_RESULT iamovieplay_get_hd_common_buf(HD_COMMON_MEM_VB_BLK *pblk, UINT32 *pPa, UINT32 *pVa, UINT32 blk_size)
{
	// get memory
	*pblk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, DDR_ID0); // Get block from mem pool
	if (*pblk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("config_vdo_frm: get blk fail, blk(0x%x)\n", *pblk);
		return HD_ERR_SYS;
	}

	*pPa = hd_common_mem_blk2pa(*pblk); // get physical addr
	if (*pPa == 0) {
		DBG_ERR("config_vdo_frm: blk2pa fail, blk(0x%x)\n", *pblk);
		return HD_ERR_SYS;
	}

	*pVa = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *pPa, blk_size);
	if (*pVa == 0) {
		DBG_ERR("Convert to VA failed for file buffer for decoded buffer!\r\n");
		return HD_ERR_SYS;
	}
	DBG_DUMP("*pblk 0x%x, *pPa 0x%x, *pVa 0x%x\r\n", *pblk, *pPa, *pVa);
	return HD_OK;
}

ER iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_TYPE speed, IAMOVIEPLAY_DIRECT_TYPE direct, INT32 disp_idx)
{
	FLGPTN flag = 0;
	IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;
    IAMOVIEPLAY_BSDEMUX_BS_INFO *p_vdo_cbinfo = &g_vdo_bsdemux_cbinfo;
    IAMOVIEPLAY_BSDEMUX_BS_INFO *p_aud_cbinfo = &g_aud_bsdemux_cbinfo;

	IAMMOVIEPLAY_COMM_DUMP("start play spd=%d, dir=%d, sta=%d\r\n", speed, direct, tsk_obj->state);

	switch (tsk_obj->state) {
	case IAMOVIEPLAY_STATE_CLOSE:
		DBG_ERR("BsDemux is on close state\r\n");
		return E_SYS;
	case IAMOVIEPLAY_STATE_OPEN:
		tsk_obj->speed = speed;
		tsk_obj->direct = direct;
		#if 0
		if (tsk_obj->aud_en) {
			DBG_IND("audio start\r\n");
			// start audio first, and wait for audio preload done
			_iamovieplay_TriggerAudio();
		}
		else
		#endif
		{

			#if 0
			DBG_DUMP("video start\r\n");
			// start video directly
			// reset video/audio frame bitstream offset and size for file in module.
			p_vdo_cbinfo->bs_offset = p_aud_cbinfo->bs_offset = 0;
			p_vdo_cbinfo->bs_size = p_aud_cbinfo->bs_size = FILEIN_BLK_SIZE;

			tsk_obj->vdo_idx = 0; // reset video index.
			tsk_obj->aud_idx = 0; // reset audio index.
			set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_START);
			#else
			DBG_DUMP("video start\r\n");
			p_vdo_cbinfo->bs_offset = p_aud_cbinfo->bs_offset = 0;
			p_vdo_cbinfo->bs_size = p_aud_cbinfo->bs_size = FILEIN_BLK_SIZE;

			tsk_obj->vdo_idx = 0; // reset video index.
			tsk_obj->aud_idx = 0; // reset audio index.
			tsk_obj->aud_count = 0;
			tsk_obj->vdo_frm_eof = 0; // reset eof var.
			#if 1
			// setup timer interval.
		    iamovieplay_SetVdoTimerIntval(0);
			#endif
			set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_START);
			#endif
		}
		return E_OK;
	case IAMOVIEPLAY_STATE_PLAYING:
		// change speed & direction
		if (_iamovieplay_ChangeSpeed(speed, direct) != E_OK) {
			DBG_ERR("change speed fail\r\n");
			return E_SYS;
		}
		// resume to normal play
		if (tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
			if (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS) {
				UINT32 sec;

				// Get current present video index and reload video/audio
				tsk_obj->vdo_idx = _iamovieplay_GetNPIframe(tsk_obj->vdo_idx, tsk_obj->speed, tsk_obj->direct);
				tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);
				tsk_obj->aud_count = tsk_obj->aud_idx;
				sec = tsk_obj->vdo_idx / tsk_obj->vdo_fr;
				_iamovieplay_Seek(sec);
			} else {

				if(disp_idx >= 0){

					if((UINT32)disp_idx >= (tsk_obj->vdo_ttfrm - tsk_obj->gop)){
						disp_idx = (tsk_obj->vdo_ttfrm - tsk_obj->gop - 1);
					}

					DBG_DUMP("display index = %d\r\n", disp_idx);
					tsk_obj->vdo_idx = (UINT32)disp_idx;
				}

				if ((tsk_obj->vdo_idx >= tsk_obj->vdo_ttfrm) && (tsk_obj->vdo_ttfrm != 0)) {
					DBG_ERR("VdoIdx(=%d) is over ttfrm(=%d)\r\n", disp_idx, tsk_obj->vdo_ttfrm);
					return E_SYS;
				}

				if (tsk_obj->gop == 0) {
					DBG_ERR("gop = 0\r\n");
					return E_SYS;
				}

				if ((tsk_obj->vdo_idx % tsk_obj->gop) != 0) {
					tsk_obj->vdo_idx = _iamovieplay_GetNPIframe(tsk_obj->vdo_idx, tsk_obj->speed, tsk_obj->direct);
					DBG_IND("find nearest I=%d\r\n", tsk_obj->vdo_idx);
				}

				// get audio index by video index
				if (tsk_obj->aud_en) {
					tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);
					tsk_obj->aud_count = tsk_obj->aud_idx;
				}

				// wait idle to avoid racing confition (wait OneShot finished)
				DBG_IND("wait idle ...\r\n");
				wai_flg(&flag, IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_IDLE, TWF_ORW);
				DBG_IND("wait idle ... OK!!!\r\n");
				// Reload File block
				if(_iamovieplay_ReloadFileReadBlock() == FALSE)
					return E_SYS;

#if _TODO_
				// Reload Audio
				_iamovieplay_ReloadAudio();

				// Reload Video for normal play
				_iamovieplay_ReloadVideo();
				if (tsk_obj->aud_en) {
					// start audio first, and wait for audio preload done
					_iamovieplay_TriggerAudio();
					//return E_OK;
				}
#endif
			}
		}

	    #if 1
	    // Timer trigger start
	    set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_START);
	    #endif

		break;
	case IAMOVIEPLAY_STATE_PAUSE:

		_iamovieplay_Pause();
		vos_util_delay_ms(5); 		
		DBG_DUMP("^Gpause\r\n");
		#if 0
		// reload audio only
		if (tsk_obj->aud_en && tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
			// get current display index
			if (disp_idx == 0) {
				DBG_ERR("disp index = 0\r\n");
				return E_SYS;
			} else if ((disp_idx >= tsk_obj->vdo_ttfrm) && (tsk_obj->vdo_ttfrm != 0)) {
				DBG_ERR("DispIdx(=%d) is over ttfrm(=%d)\r\n", disp_idx, tsk_obj->vdo_ttfrm);
				return E_SYS;
			}
			tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(disp_idx);
#if _TODO_
			_iamovieplay_ReloadAudio();
#endif
		}
		#endif
		break;

	case IAMOVIEPLAY_STATE_RESUME:
		
		if (tsk_obj->file_fmt == IAMOVIEPLAY_FILEFMT_TS) {
			UINT32 sec;

			// Get current present video index and reload video/audio
			tsk_obj->vdo_idx = _iamovieplay_GetNPIframe(tsk_obj->vdo_idx, tsk_obj->speed, tsk_obj->direct);
			tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);
			tsk_obj->aud_count = tsk_obj->aud_idx;
			sec = tsk_obj->vdo_idx / tsk_obj->vdo_fr;

			DBG_DUMP("sec = %lu, vdo_idx = %lu aud_idx = %lu\n",sec,  tsk_obj->vdo_idx, tsk_obj->aud_idx);

			if(_iamovieplay_Seek(sec) != E_OK){
				MOVIEPLAY_FILEPLAY_INFO *p_play_info = &g_pImageStream->play_info;
				g_movie_finish_event = TRUE;
				(p_play_info->event_cb)(MOVIEPLAY_EVENT_STOP, 0, 0, 0);
			}
		}
		else{

			if ((tsk_obj->vdo_idx % tsk_obj->gop) != 0) {
				tsk_obj->vdo_idx = _iamovieplay_GetNPIframe(tsk_obj->vdo_idx, tsk_obj->speed, tsk_obj->direct);
				DBG_IND("find nearest I=%d\r\n", tsk_obj->vdo_idx);
			}

			if(tsk_obj->vdo_idx == tsk_obj->vdo_ttfrm) {
				if (tsk_obj->vdo_idx % tsk_obj->gop == 0) {
					tsk_obj->vdo_idx -= tsk_obj->gop;
				}

				tsk_obj->vdo_idx = (tsk_obj->vdo_idx / tsk_obj->gop) * tsk_obj->gop;

				DBG_DUMP("2)find nearest I=%d\r\n", tsk_obj->vdo_idx);
				tsk_obj->vdo_frm_eof = TRUE;
			}

			// get audio index by video index
			if (tsk_obj->aud_en) {
				tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(tsk_obj->vdo_idx);
				tsk_obj->aud_count = tsk_obj->aud_idx;

				DBG_DUMP("find nearest I aud=%d \r\n", tsk_obj->aud_idx);
			}



			// Reload File block
			if(_iamovieplay_ReloadFileReadBlock() == FALSE)
				return E_SYS;

			if (tsk_obj->aud_en) {
				// audio Playback system un-initial
				g_bForceFirstAudioPB = TRUE;
			}
		}


		tsk_obj->state = IAMOVIEPLAY_STATE_PLAYING;
		vos_flag_set(IAMOVIEPLAY_AUDIO_PB_FLG_ID, FLG_IAMOVIEPLAY_AUDIO_PB_RESUME);

	    // start video
		// setup timer interval.
	    //iamovieplay_SetVdoTimerIntval(0);
    	_iamovieplay_TimerTriggerStart();

		DBG_DUMP("^Gresume\r\n");
		#if 0
		// reload audio only
		if (tsk_obj->aud_en && tsk_obj->speed == IAMOVIEPLAY_SPEED_NORMAL && tsk_obj->direct == IAMOVIEPLAY_DIRECT_FORWARD) {
			// get current display index
			if (disp_idx == 0) {
				DBG_ERR("disp index = 0\r\n");
				return E_SYS;
			} else if ((disp_idx >= tsk_obj->vdo_ttfrm) && (tsk_obj->vdo_ttfrm != 0)) {
				DBG_ERR("DispIdx(=%d) is over ttfrm(=%d)\r\n", disp_idx, tsk_obj->vdo_ttfrm);
				return E_SYS;
			}
			tsk_obj->aud_idx = _iamovieplay_GetAudFrmIdxByVideo(disp_idx);
#if _TODO_
			_iamovieplay_ReloadAudio();
#endif
		}
		#endif
		break;

	case IAMOVIEPLAY_STATE_STOP:
		_iamovieplay_Stop();
		break;
	case IAMOVIEPLAY_STATE_STEP:
		break;
	default:
		DBG_ERR("invalid play state = %d\r\n", tsk_obj->state);
		break;
	}

    #if 0
    // Timer trigger start
    set_flg(IAMOVIEPLAY_DEMUX_FLG_ID, FLG_IAMOVIEPLAY_DEMUX_START);
    #endif

	return E_OK;
}

ER iamovieplay_SetPlayState(IAMOVIEPLAY_PLAY_STATE  state)
{
    IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;

    tsk_obj->state = state;

    return E_OK;
}

IAMOVIEPLAY_PLAY_STATE iamovieplay_GetPlayState(void)
{
    IAMOVIEPLAY_TSK *tsk_obj = &iamovieplay_tsk_obj;

    return tsk_obj->state;
}

void iamovieplay_InstallID(void)
{
#if defined __UITRON || defined __ECOS

	OS_CONFIG_TASK(IAMOVIEPLAY_DEMUX_TSK_ID, IAMOVIEPLAY_DEMUX_PRI, IAMOVIEPLAY_DEMUX_STKSIZE, _iamovieplay_Demux_Tsk);
	OS_CONFIG_TASK(IAMOVIEPLAY_READ_TSK_ID, IAMOVIEPLAY_READ_PRI, IAMOVIEPLAY_READ_STKSIZE, _iamovieplay_Read_Tsk);

#endif

	OS_CONFIG_FLAG(IAMOVIEPLAY_DEMUX_FLG_ID);
	OS_CONFIG_FLAG(IAMOVIEPLAY_READ_FLG_ID);
	
#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)

	OS_CONFIG_FLAG(IAMOVIEPLAY_DISP_FLG_ID);

#endif

	OS_CONFIG_FLAG(IAMOVIEPLAY_AUDIO_PB_FLG_ID);
	OS_CONFIG_FLAG(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID);

	if(IAMOVIEPLAY_SEMID_DECRYPT == 0){

		int ret;

		if ((ret = vos_sem_create(&IAMOVIEPLAY_SEMID_DECRYPT, 1, "IAMPLAY_SEM_DECRYPT")) != E_OK) {
			DBG_ERR("Create IAMOVIEPLAY_SEMID_DECRYPT fail(%d)\r\n", ret);
		}
	}
}

void iamovieplay_UninstallID(void)
{
	if(IAMOVIEPLAY_DEMUX_FLG_ID){

		if(rel_flg(IAMOVIEPLAY_DEMUX_FLG_ID) != E_OK)
			DBG_WRN("rel_flg IAMOVIEPLAY_DEMUX_FLG_ID failed!\r\n");

		IAMOVIEPLAY_DEMUX_FLG_ID = 0;
	}

	if(IAMOVIEPLAY_READ_FLG_ID){

		if(rel_flg(IAMOVIEPLAY_READ_FLG_ID) != E_OK)
			DBG_WRN("rel_flg IAMOVIEPLAY_READ_FLG_ID failed!\r\n");

		IAMOVIEPLAY_READ_FLG_ID = 0;
	}

#if (IAMMOVIEPLAY_AUTO_BIND_VOUT == DISABLE)

	if(IAMOVIEPLAY_DISP_FLG_ID){

		if(rel_flg(IAMOVIEPLAY_DISP_FLG_ID) != E_OK)
			DBG_WRN("rel_flg IAMOVIEPLAY_DISP_FLG_ID failed!\r\n");

		IAMOVIEPLAY_DISP_FLG_ID = 0;
	}

#endif

	if(IAMOVIEPLAY_AUDIO_PB_FLG_ID){

		if(rel_flg(IAMOVIEPLAY_AUDIO_PB_FLG_ID) != E_OK)
			DBG_WRN("rel_flg IAMOVIEPLAY_AUDIO_PB_FLG_ID failed!\r\n");

		IAMOVIEPLAY_AUDIO_PB_FLG_ID = 0;
	}

	if(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID){

		if(rel_flg(IAMOVIEPLAY_AUDIO_DECODE_FLG_ID) != E_OK)
			DBG_WRN("rel_flg IAMOVIEPLAY_AUDIO_DECODE_FLG_ID failed!\r\n");

		IAMOVIEPLAY_AUDIO_DECODE_FLG_ID = 0;
	}

	if(IAMOVIEPLAY_SEMID_DECRYPT){
		vos_sem_destroy(IAMOVIEPLAY_SEMID_DECRYPT);
		IAMOVIEPLAY_SEMID_DECRYPT = 0;
	}
}

void iamovieplay_set_dbg_level(UINT32 dbg_level)
{
	g_movply_dbglvl = dbg_level;
	DBGD(g_movply_dbglvl);
}

