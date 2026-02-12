/**
    WavStudio, export commands to SxCmd

    @file       wavstudio_sxcmd.c
    @ingroup    mILIBWAVSTUDIO

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/


#include "wavstudio_int.h"
#include "wavstudio_sxcmd.h"
#define WAVSTUDIO_TEST DISABLE

#if (WAVSTUDIO_TEST == ENABLE)

#ifdef __KERNEL__
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#endif

static INT16 gSine_Buff_8K[AUDIO_SR_48000] = {0};

#ifdef __KERNEL__
#if 1
struct 	file* 	output_handle;
#endif
static const short sin_TABLE[48] = {
        0, 4277, 8481,12539,
    16383,19947,23170,25996,
    28377,30273,31650,32487,
    32767,32487,31650,30273,
    28377,25996,23170,19947,
    16383,12539, 8481, 4277,
        0,-4277,-8481,-12539,
   -16383,-19947,-23170,-25996,
   -28377,-30273,-31650,-32487,
   -32767,-32487,-31650,-30273,
   -28377,-25996,-23170,-19947,
   -16384,-12539,-8481,-4277
};
#endif



/*static void WavPlay_CB(WAVSTUD_ACT act, UINT32 p1, UINT32 p2)
{
    DBG_MSG("^GWavPlay_CB p1=%d,p2=%d\r\n",p1,p2);
    switch(p1)
    {
    case WAVSTUD_CB_EVT_STARTED: //on start
        break;
    case WAVSTUD_CB_EVT_TCHIT: //on time code hit
        break;
    case WAVSTUD_CB_EVT_STOPPED: //on stop
        break;
    default:
        break;
    }
}*/
#if 1
static BOOL WavStudio_CopySinWave(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp)
{
	if (pAudBufQue == NULL) {
		DBG_ERR("Buff is NULL!\r\n");

		return FALSE;
	}
	memcpy((void *)pAudBufQue->uiAddress, (void *)gSine_Buff_8K, pAudBufQue->uiSize);
	return TRUE;
}
#endif

static BOOL xWavStudioSxCmd_TestSpeaker(CHAR *strCmd)
{
	#ifdef __KERNEL__
	UINT16 i,*temp_addr;
	WAVSTUD_AUD_INFO AudInfo = {AUDIO_SR_48000, AUDIO_CH_RIGHT, WAVSTUD_BITS_PER_SAM_16, 1};
	UINT64 StopCount = 0;
	UINT32 sampleRate = AUDIO_SR_48000;
	UINT32 sec;
	frammap_buf_t      buf_info = {0};
	WAVSTUD_APPOBJ wso = {0};
	WAVSTUD_INFO_SET wsis = {0};

	sscanf_s(strCmd, "%d", &sec);

	temp_addr = (INT16 *)&gSine_Buff_8K;

	for(i = 0; i<sampleRate;i++){
	    *(temp_addr+i) = sin_TABLE[i%48];
    }
	//Allocate memory
	buf_info.size = 0x20000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "wavstudio";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);

	wso.mem.uiAddr = (UINT32)buf_info.va_addr;
	if (wso.mem.uiAddr == 0)
	{
	    DBG_ERR("Get buffer failed\r\n");
	    return FALSE;
	}
	wso.mem.uiSize = 0x6000000;
	wso.wavstud_cb = 0;

	wsis.obj_count = 1;
	wsis.data_mode = WAVSTUD_DATA_MODE_PULL;
	wsis.aud_info = AudInfo;
	wsis.buf_count = 5;
	wsis.unit_time = WAVSTUD_UNIT_TIME_1000MS;

	wavstudio_open(WAVSTUD_ACT_PLAY, &wso, 0, &wsis);


	StopCount = sampleRate * sec;

	if (FALSE == wavstudio_start(WAVSTUD_ACT_PLAY, &AudInfo, StopCount, &WavStudio_CopySinWave)) {
		DBG_ERR("Test speaker fail\r\n");
		return FALSE;
	}

	wavstudio_wait_start(WAVSTUD_ACT_PLAY);
	wavstudio_wait_stop(WAVSTUD_ACT_PLAY);
	wavstudio_close(WAVSTUD_ACT_PLAY);

	frm_free_buf_ddr((void *)buf_info.va_addr);
	#endif
	#if 0
	UINT32 sampleRate = AUDIO_SR_8000;
	FLOAT amplitude = 65535.0 / 4.0;
	FLOAT frequency = 500;
	UINT32 n;
	UINT32 sec;
	//WAVSTUD_APPOBJ wso;
	//WAVSTUD_INFO_SET wsis;
	//DX_HANDLE DevAudObj = Dx_GetObject(DX_CLASS_AUDIO_EXT);
	WAVSTUD_AUD_INFO AudInfo = { AUDIO_SR_8000, AUDIO_CH_LEFT, WAVSTUD_BITS_PER_SAM_16, WAVSTUDCODEC_PCM};
	UINT64 StopCount = 0;

	sscanf_s(strCmd, "%d", &sec);

	sec = (sec > 20) ? 20 : sec;

	//generate sine wave
	for (n = 0; n < sampleRate; n++) {
		*(gSine_Buff_8K + n) = (INT16)(amplitude * sin((2 * M_PI * (float)n * frequency) / sampleRate));
	}

	//Open WavStudio sample for play
	/*
	wso.mem.uiAddr = User_GetTempBuffer(0x3000);
	wso.mem.uiSize = 0x3000;
	wso.pWavStudCB = (WAVSTUD_CB)WavPlay_CB;
	wsis.mode = WAVSTUD_MODE_1P0R; //only play 1 stream
	wsis.playInfo = AudInfo;
	wsis.playBufCount = 4;
	wsis.playUnitTime = WAVSTUD_UNIT_TIME_100MS;
	wsis.recInfo = AudInfo;
	wsis.recBufCount = 0;
	wsis.recUnitTime = 0;

	WavStudio_SetConfig(WAVSTUD_CFG_VOL, WAVSTUD_ACT_PLAY, 30);
	WavStudio_Open(&wso, DevAudObj, &wsis);
	*/

	StopCount = sampleRate * sec;

	if (FALSE == WavStudio_Start(WAVSTUD_ACT_PLAY, &AudInfo, StopCount, &WavStudio_CopySinWave)) {
		DBG_ERR("Test speaker fail\r\n");
		return FALSE;
	}
	#endif
	return TRUE;
}

static BOOL WavStudio_OutputRecAvg(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp)
{
	#if 0
	mm_segment_t oldfs;

    oldfs = get_fs();

	DBG_DUMP("addr=%x, write size=%d, offset=%d\r\n", pAudBufQue->uiAddress, pAudBufQue->uiSize, (UINT32)output_handle->f_pos);
	fmem_dcache_sync((void *)pAudBufQue->uiAddress, pAudBufQue->uiSize, DMA_BIDIRECTIONAL);
	set_fs(get_ds());
	vfs_write(output_handle, (char __user *)pAudBufQue->uiAddress, (UINT32)pAudBufQue->uiSize, &output_handle->f_pos);
	set_fs(oldfs);
	#endif

	UINT32 sampleCount = pAudBufQue->uiSize / 2;
	UINT32 i;
	INT16 max = 0;
	INT16 tmp = 0;

	for (i = 0; i < sampleCount; i++) {
		tmp = *(((INT16 *)pAudBufQue->uiAddress) + i);
		tmp = (tmp > 0) ? tmp : -tmp;
		max = (max > tmp) ? max : tmp;
	}

	tmp = max / 500;

	//print '*' according to the recorded amplitude
	if (tmp > 1) {
		for (i = 0; i < (UINT32)tmp; i++) {
			DBG_DUMP("*");
		}
	} else {
		DBG_DUMP("*");
	}
	DBG_DUMP("\r\n");
	return TRUE;
}

static BOOL xWavStudioSxCmd_TestMic(CHAR *strCmd)
{
	UINT32 sec;
	UINT32 sampleRate = AUDIO_SR_8000;
	//WAVSTUD_APPOBJ wso;
	//WAVSTUD_INFO_SET wsis;
	//DX_HANDLE DevAudObj = Dx_GetObject(DX_CLASS_AUDIO_EXT);
	WAVSTUD_AUD_INFO AudInfo = {AUDIO_SR_8000, AUDIO_CH_RIGHT, WAVSTUD_BITS_PER_SAM_16, 1};
	#ifdef __KERNEL__
	frammap_buf_t      buf_info = {0};
	WAVSTUD_APPOBJ wso = {0};
	WAVSTUD_INFO_SET wsis = {0};
	#endif

	UINT64 StopCount = 0;

	sscanf_s(strCmd, "%d", &sec);

	#ifdef __KERNEL__

	output_handle = filp_open("/mnt/sd/wav.pcm", O_CREAT|O_WRONLY|O_SYNC, 0);
	if (IS_ERR(output_handle)) {
		DBG_ERR("unable to open backing file\n");
	}

	//Allocate memory
	buf_info.size = 0x20000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "wavstudio";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);

	wso.mem.uiAddr = (UINT32)buf_info.va_addr;
	if (wso.mem.uiAddr == 0)
	{
	    DBG_ERR("Get buffer failed\r\n");
	    return FALSE;
	}
	wso.mem.uiSize = 0x6000000;
	wso.wavstud_cb = 0;

	wsis.obj_count = 1;
	wsis.data_mode = WAVSTUD_DATA_MODE_PUSH_FILE;
	wsis.aud_info = AudInfo;
	wsis.buf_count = 5;
	wsis.unit_time = WAVSTUD_UNIT_TIME_100MS;

	wavstudio_open(WAVSTUD_ACT_REC, &wso, 0, &wsis);
	#endif
	//Open WavStudio sample for record
	/*
	wso.mem.uiAddr = User_GetTempBuffer(0x3000);
	if (wso.mem.uiAddr == NULL)
	{
	    DBG_ERR("Get buffer failed\r\n");
	    return FALSE;
	}
	wso.mem.uiSize = 0x3000;
	wso.pWavStudCB = (WAVSTUD_CB)WavPlay_CB;
	wsis.mode = WAVSTUD_MODE_0P1R; //only record 1 stream
	wsis.playInfo = AudInfo;
	wsis.playBufCount = 0;
	wsis.playUnitTime = 0;
	wsis.recInfo = AudInfo;
	wsis.recBufCount = 4;
	wsis.recUnitTime = WAVSTUD_UNIT_TIME_100MS;

	WavStudio_SetConfig(WAVSTUD_CFG_VOL, WAVSTUD_ACT_REC, 50);
	WavStudio_Open(&wso, DevAudObj, &wsis);
	*/

	StopCount = (UINT64)sampleRate * (UINT64)sec;

	//Rec start
	if (FALSE == wavstudio_start(WAVSTUD_ACT_REC, &AudInfo, StopCount, &WavStudio_OutputRecAvg)) {
		DBG_ERR("Test mic fail\r\n");
		#ifdef __KERNEL__
		frm_free_buf_ddr((void *)buf_info.va_addr);
		#endif

		return FALSE;
	}

	#ifdef __KERNEL__
	wavstudio_wait_start(WAVSTUD_ACT_REC);
	wavstudio_wait_stop(WAVSTUD_ACT_REC);
	wavstudio_close(WAVSTUD_ACT_REC);

	vfs_fsync(output_handle, 1);
	filp_close(output_handle, NULL);

	frm_free_buf_ddr((void *)buf_info.va_addr);
	#endif

	return TRUE;
}


extern UINT32 wavdbg;

BOOL xWavStudioSxCmd_Dbg(CHAR *strCmd)
{
	UINT32 lvl = 0;

	sscanf_s(strCmd, "%d", &lvl);

	wavdbg = lvl;

	return TRUE;
}

BOOL xWavStudioSxCmd_Dump(CHAR *strCmd)
{
	WavStud_DmpModuleInfo();

	return TRUE;
}

static SXCMD_BEGIN(wavstudio, "WavStudio")
SXCMD_ITEM("dbg %", xWavStudioSxCmd_Dbg, "Debug message")
SXCMD_ITEM("dump", xWavStudioSxCmd_Dump, "Dump WavStudio memory")
#if (WAVSTUDIO_TEST == ENABLE)
SXCMD_ITEM("testspk %", xWavStudioSxCmd_TestSpeaker, "Test speaker (second)")
SXCMD_ITEM("testmic %", xWavStudioSxCmd_TestMic, "Test mic (second)")
#endif
SXCMD_END()

static BOOL bInit = FALSE;

void xWavStudio_InstallCmd(void)
{
	if (!bInit) {
		sxcmd_addtable(wavstudio);
		bInit = TRUE;
	}
}

int _wavstudio_sxcmd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(wavstudio);
	UINT32 loop=1;

	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "wavstudio");
	DBG_DUMP("=====================================================================\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", wavstudio[loop].p_name, wavstudio[loop].p_desc);
	}

	return 0;
}


int _wavstudio_sxcmd(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(wavstudio);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_wavstudio_sxcmd_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, wavstudio[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = wavstudio[loop].p_func(cmd_args);
			break;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/isf_vdoenc/help\" for help.\r\n");
		return -1;
	}


	return 0;
}

#endif