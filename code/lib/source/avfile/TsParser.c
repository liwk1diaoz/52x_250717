#if defined (__UITRON) || defined (__ECOS)
#include <string.h>

#include "kernel.h"
#include "FileSysTsk.h"

#include "MediaReadLib.h"
#include "AVFile_ParserTs.h"
#include "NvtVerInfo.h"
#include "h265dec_api.h"
#include "crypto.h"

#define __MODULE__          TSRead
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"

NVTVER_VERSION_ENTRY(MP_TsReadLib, 1, 00, 016, 00)
#else
#include <string.h>

#include "FileSysTsk.h"
#include "kwrap/error_no.h"

#include "avfile/MediaReadLib.h"
#include "avfile/AVFile_ParserTs.h"
#include <kwrap/verinfo.h>

VOS_MODULE_VERSION(MP_TsReadLib, 1, 01, 001, 00);

#define __MODULE__          TSRead
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#include "kwrap/debug.h"
unsigned int TSRead_debug_level = NVT_DBG_WRN;
#endif
typedef struct _TSREADLIB_INFO {
	UINT32 bufaddr;
	UINT32 bufsize;
	UINT64 remainFileSize;
	UINT64 parsedFileSize;
	UINT32 memStartAddr;
} TSREADLIB_INFO;

ER TsReadLib_Probe(UINT32 addr, UINT32 readsize);
ER TsReadLib_Initialize(void);
ER TsReadLib_SetMemBuf(UINT32 startAddr, UINT32 size);
ER TsReadLib_ParseHeader(UINT32 hdrAddr, UINT32 hdrSize, void *pobj);
ER TsReadLib_GetInfo(UINT32 type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
ER TsReadLib_SetInfo(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3);

void (*TsReadLib_DemuxCB)(UINT32 demuxSize, UINT32 duplicateSize);

CONTAINERPARSER gTsReadLibObj =  {TsReadLib_Probe,
								  TsReadLib_Initialize,
								  TsReadLib_SetMemBuf,
								  TsReadLib_ParseHeader,
								  TsReadLib_GetInfo,
								  TsReadLib_SetInfo,
								  NULL,//custom
								  NULL,//read
								  NULL,//showmsg
								  MOVREADLIB_CHECKID  //check ID
								 };

#define _USER_ALLOC_ENTRY_BUF_  1

#define DECRYPT_BLK 256

typedef struct {
	UINT32 is_on;
	UINT32 addr;
	UINT32 size;
    unsigned char key[4];
} TSPLAY_CRYPTO_INFO;

TSPLAY_CRYPTO_INFO             g_decrypt_info                         = {0};
CONTAINERPARSER     gTsReaderContainer;
TSREADLIB_INFO      gTsReadLib_Info = {0};
TsFilterManager     gTsFilterManager;
TsPlayInfo          gTsPlayInfo = {0};
//TS_FRAME_ENTRY      gTsVideoEntry[TSVID_ENTRY_SIZE] = {0};
//TS_FRAME_ENTRY      gTsAudioEntry[TSAUD_ENTRY_SIZE] = {0};
TS_FRAME_ENTRY      *gpTsVideoEntry, *gpTsAudioEntry;
UINT32              gTsVideoEntryMax = 0, gTsAudioEntryMax = 0;
SPLITTED_TS_PACKET  gSplittedTSPacket = {0};
UINT32              gTsDemuxAddr = 0;
UINT32              gTsDemuxSize = 0;
UINT32              gPrevPid = 0;
UINT32              gLastI_index = 0;
TS_SEEK_INFO        gTsSeekInfo = {0};
MEM_RANGE           gTsH26xWorkBuf = {0};
static ER g_TsReadLib_ParseEndInfo = E_SYS;

//for NAL parser
const unsigned char *gpTsNALParserStart;
unsigned short gTsNALParserLength;
int gTsNALParserCurrentBit;

CONTAINERPARSER *MP_TsReadLib_GerFormatParser(void)
{
	return &gTsReadLibObj;
}

//NVTVER_VERSION_ENTRY(MP_TsReadLib, 1, 00, 005, 00)//1.00.016

ER TsReadLib_Probe(UINT32 addr, UINT32 readsize)
{
	UINT32 stat[188];
	UINT32 i;
	UINT32 x = 0;
	UINT32 best_score = 0;
	UINT8 *buf = (UINT8 *)addr;

	memset(stat, 0, 188 * sizeof(UINT32));

	for (x = i = 0; i < readsize - 3; i++) {
		if ((buf[i] == 0x47) && !(buf[i + 1] & 0x80) && (buf[i + 3] & 0x30)) {
			stat[x]++;

			if (stat[x] > best_score) {
				best_score = stat[x];
				//if (index)
				//  *index= x;
			}
		}

		x++;
		if (x == 188) {
			x = 0;
		}
	}

	return best_score;
}

ER TsReadLib_Initialize(void)
{
	//if (gTsReaderContainer.cbShowErrMsg)
	//    (gTsReaderContainer.cbShowErrMsg)("TsReadLib_Initialize\r\n");

	gTsFilterManager.patFilter.pid = PAT_PID;
	gTsFilterManager.patFilter.type = MPEGTS_SECTION;

	gTsReadLib_Info.bufaddr = 0;
	gTsReadLib_Info.bufsize = 0;
	gTsReadLib_Info.remainFileSize = 0;
	gTsReadLib_Info.parsedFileSize = 0;
	gTsReadLib_Info.memStartAddr = 0;

	gSplittedTSPacket.pid = 0;
	gSplittedTSPacket.splitted1stHalfSize = 0;

	//memset(&gTsPlayInfo, 0, sizeof(TsPlayInfo));  //need totalVidFrame for seeking
	gTsPlayInfo.vidFPS = 0;
	gTsPlayInfo.audFrmIndex = 0;
	gTsPlayInfo.audFrameSize = 0;
	gTsPlayInfo.vidFrmIndex = 0;
	gTsPlayInfo.vidFrameSize = 0;
	gTsPlayInfo.currentPesType = PESTYPE_NONE;
	gTsPlayInfo.demuxFinished = FALSE;
	gTsPlayInfo.thumbAddr = 0;
	gTsPlayInfo.thumbSize = 0;
	gTsPlayInfo.vidNalLength = 0;

	gLastI_index = 0;

	return E_OK;
}

ER TsReadLib_SetMemBuf(UINT32 startAddr, UINT32 size)
{
	gTsReadLib_Info.bufaddr = startAddr;
	gTsReadLib_Info.bufsize = size;
	return E_OK;
}

static void TsReadLib_SetVidEntryAddr(UINT32 addr, UINT32 size)
{
	UINT32 max;

	max = size / (sizeof(TS_FRAME_ENTRY));
	if (addr) {
		gpTsVideoEntry = (TS_FRAME_ENTRY *)addr;
		gTsVideoEntryMax = max;
		memset((UINT8 *)gpTsVideoEntry, 0, size);
		//g_MovTagReadTrack[0].cluster = gp_movReadEntry;
	} else {
		gpTsVideoEntry = 0;
	}
#if 0
	DBG_DUMP("gTsVideoEntryMax=0x%x\r\n", gTsVideoEntryMax);
#endif
}

static void TsReadLib_SetAudEntryAddr(UINT32 addr, UINT32 size)
{
	UINT32 max;

	max = size / (sizeof(TS_FRAME_ENTRY));
	if (addr) {
		gpTsAudioEntry = (TS_FRAME_ENTRY *)addr;
		gTsAudioEntryMax = max;
		memset((UINT8 *)gpTsAudioEntry, 0, size);
		//g_MovTagReadTrack[0].cluster = gp_movReadEntry;
	} else {
		gpTsAudioEntry = 0;
	}
#if 0
	DBG_DUMP("gTsAudioEntryMax=0x%x\r\n", gTsAudioEntryMax);
#endif
}

ER TsReadLib_ParseHeader(UINT32 hdrAddr, UINT32 hdrSize, void *pobj)
{
	MEDIA_HEADINFO *pHeader;

	pHeader = (MEDIA_HEADINFO *)pobj;
	//(gTsReaderContainer.cbShowErrMsg)("TsReadLib_ParseHeader!!\r\n");

	//pHeader->uiTotalAudioFrames = tempA;
	//pHeader->uiTotalVideoFrames = tempV;
	if (gTsFilterManager.audFilter.pid != 0) {
		pHeader->bAudioSupport = TRUE;
	} else {
		pHeader->bAudioSupport = FALSE;
	}
	pHeader->bVideoSupport = TRUE;
	//pHeader->uiTotalSecs = duration/tempV;//timescale = MOV_TIMESCALE
	pHeader->uiVidScale = 60000;
	if (gTsPlayInfo.vidFPS != 0) {
		pHeader->uiVidRate = gTsPlayInfo.vidFPS;
	} else {
		pHeader->uiVidRate = 30;
	}
	pHeader->uiTotalVideoFrames = gTsPlayInfo.totalVidFrame;

	pHeader->uiAudSampleRate = gTsPlayInfo.audSampleRate;
	pHeader->ucAudChannels = 1;
	pHeader->uiAudType = 2;

	return E_OK;
}

ER TsReadLib_GetInfo(UINT32 type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
{
	ER resultV = E_PAR;
	UINT32 temp;

	switch (type) {
	case MEDIAREADLIB_GETINFO_H264DESC:
		*pparam1 = (UINT32)(gTsPlayInfo.h264VidDesc);
		*pparam2 = gTsPlayInfo.h264VidDescLen;
		resultV = E_OK;
		break;

	case MEDIAREADLIB_GETINFO_H265DESC:
		*pparam1 = (UINT32)(gTsPlayInfo.h264VidDesc);
		*pparam2 = gTsPlayInfo.h264VidDescLen;
		break;

	case MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE:
#if _USER_ALLOC_ENTRY_BUF_
		{
			TS_FRAME_ENTRY *pVidEntry1, *pVidEntry2;

			temp = *pparam1;
			pVidEntry1 = gpTsVideoEntry + (temp % TSVID_ENTRY_SIZE);
			pVidEntry2 = gpTsVideoEntry + ((temp - 1) % TSVID_ENTRY_SIZE);
			if (temp > 0 && pVidEntry1->frameIdx < pVidEntry2->frameIdx) {
				DBG_DUMP("[TsParser] get VDO finished, frm idx = %d\r\n", temp);
				*pparam2 = 0;
				*pparam3 = 0;
			} else {
				*pparam2 = pVidEntry1->memAddr;
				*pparam3 = pVidEntry1->size;
			}
			resultV = E_OK;
		}
#else
		temp = *pparam1;
		if (temp > 0 && gTsVideoEntry[temp % TSVID_ENTRY_SIZE].frameIdx < gTsVideoEntry[(temp - 1) % TSVID_ENTRY_SIZE].frameIdx) {
			DBG_DUMP("[TsParser] get VDO finished, frm idx = %d\r\n", temp);
			*pparam2 = 0;
			*pparam3 = 0;
		} else {
			*pparam2 = gTsVideoEntry[temp % TSVID_ENTRY_SIZE].memAddr;
			*pparam3 = gTsVideoEntry[temp % TSVID_ENTRY_SIZE].size;
		}
		resultV = E_OK;
#endif
		break;

	case MEDIAREADLIB_GETINFO_GETAUDIOPOSSIZE:
#if _USER_ALLOC_ENTRY_BUF_
		{
			TS_FRAME_ENTRY *pAudEntry1, *pAudEntry2;

			temp = *pparam1;
			pAudEntry1 = gpTsAudioEntry + (temp % TSAUD_ENTRY_SIZE);
			pAudEntry2 = gpTsAudioEntry + ((temp - 1) % TSAUD_ENTRY_SIZE);
			if (temp > 0 && pAudEntry1->frameIdx < pAudEntry2->frameIdx) {
				DBG_DUMP("[TsParser] get AUD finished, frm idx = %d pAudEntry1->frameIdx = %d pAudEntry2->frameIdx = %d\r\n", temp, pAudEntry1->frameIdx, pAudEntry2->frameIdx);
				*pparam2 = 0;
				*pparam3 = 0;
			} else {
				*pparam2 = pAudEntry1->memAddr;
				*pparam3 = pAudEntry1->size;
			}
			resultV = E_OK;
		}
#else
		temp = *pparam1;
		if (temp > 0 && gTsAudioEntry[temp % TSAUD_ENTRY_SIZE].frameIdx < gTsAudioEntry[(temp - 1) % TSAUD_ENTRY_SIZE].frameIdx) {
			DBG_DUMP("[TsParser] get AUD finished, frm idx = %d\r\n", temp);
			*pparam2 = 0;
			*pparam3 = 0;
		} else {
			*pparam2 = gTsAudioEntry[temp % TSAUD_ENTRY_SIZE].memAddr;
			*pparam3 = gTsAudioEntry[temp % TSAUD_ENTRY_SIZE].size;
		}
		resultV = E_OK;
#endif
		break;

	case MEDIAREADLIB_GETINFO_GETGOP:
		*pparam1 = gTsPlayInfo.gopNumber;
		break;

	case MEDIAREADLIB_GETINFO_TOTALSEC:
		*pparam1 = gTsPlayInfo.totalMs / 1000;
		break;

	case MEDIAREADLIB_GETINFO_TOTALVIDFRAME:
		*pparam1 = gTsPlayInfo.totalVidFrame;
		break;

	case MEDIAREADLIB_GETINFO_SEEKRESULT:
		*pparam1 = gTsSeekInfo.diff >> 32;
		*pparam2 = gTsSeekInfo.diff & 0xFFFFFFFF;
		*pparam3 = gTsSeekInfo.result;
		break;

	case MEDIAREADLIB_GETINFO_FIRSTPTS:
		*pparam1 = gTsPlayInfo.firstPts >> 32;
		*pparam2 = gTsPlayInfo.firstPts & 0xFFFFFFFF;
		break;

	case MEDIAREADLIB_GETINFO_VIDEOTHUMB:
		*pparam1 = gTsPlayInfo.thumbAddr;
		*pparam2 = gTsPlayInfo.thumbSize;
		break;

	case MEDIAREADLIB_GETINFO_NEXTIFRAMENUM:
		temp = *pparam1;
		*pparam2 = TsReadLib_GetNextIFrame(temp, *pparam3);
		break;

	case MEDIAREADLIB_GETINFO_IFRAMEDISTANCE:
		*pparam1 = gTsPlayInfo.keyFrameDist;
		break;

	default:
		break;
	}

	return resultV;
}

ER TsReadLib_SetInfo(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	switch (type) {
	case MEDIAREADLIB_SETINFO_TSDEMUXBUF:
		gTsDemuxAddr = param1;
		gTsDemuxSize = param2;
		break;

	case MEDIAREADLIB_SETINFO_FILESIZE:
		gTsReadLib_Info.remainFileSize = (UINT64)param1 << 32 | param2;
		break;

	case MEDIAREADLIB_SETINFO_MEMSTARTADDR:
		gTsReadLib_Info.memStartAddr = param1;
		break;

	case MEDIAREADLIB_SETINFO_TSENDINFO:
		TsReadLib_ParseEndInfo(param1, param2);
		return g_TsReadLib_ParseEndInfo;

	case MEDIAREADLIB_SETINFO_PREPARESEEK:
		gTsSeekInfo.targetPts = param1;
		TsReadLib_PrepareSeek(param2, param3);
		break;

	case MEDIAREADLIB_SETINFO_TSTHUMBINFO:
		TsReadLib_ParseThumbInfo(param1, param2, param3);
		break;

	case MEDIAREADLIB_SETINFO_H26X_WORKBUF:
	{
		PMEM_RANGE p_mem = (PMEM_RANGE) param1;

		gTsH26xWorkBuf.addr = p_mem->addr;
		gTsH26xWorkBuf.size = p_mem->size;
		break;
	}

	case MEDIAREADLIB_SETINFO_VIDENTRYBUF:
		TsReadLib_SetVidEntryAddr(param1, param2);
		break;

	case MEDIAREADLIB_SETINFO_AUDENTRYBUF:
		TsReadLib_SetAudEntryAddr(param1, param2);
		break;

	default:
		break;
	}

	return E_OK;
}

ER TsReadLib_RegisterObjCB(void *pobj)
{
	CONTAINERPARSER *pContainer;
	pContainer = (CONTAINERPARSER *)pobj;
	if (pContainer->checkID != MOVREADLIB_CHECKID) {
		return E_NOSPT;
	}

	gTsReaderContainer.cbReadFile = (pContainer->cbReadFile);
	//gTsReaderContainer.cbShowErrMsg = (pContainer->cbShowErrMsg);

	//if (gTsReaderContainer.cbShowErrMsg)
	//    (gTsReaderContainer.cbShowErrMsg)("Create Obj OK\r\n");
	return E_OK;
}

ER TsReadLib_ConfigFilters(UINT32 addr, UINT32 size)
{
	UINT8 *data = (UINT8 *)addr;
	UINT32 remain_size = 0;
	ER ret;

	// find TS sync byte
	for (;;) {
		if (*data == 0x47) {
			break;
		}
		data++;

		if(((UINT32)data - addr) >= size){
			DBG_ERR("buffer overflow!(0x%lx)\n", size);
			return E_MACV;
		}
	}

	remain_size = size - ((UINT32)data - addr);

	ret = TsReadLib_ParsePAT(data, remain_size);
	if(ret != E_OK)
		return ret;

	ret = TsReadLib_ParsePMT(data, remain_size);
	if(ret != E_OK)
		return ret;

	if (gTsFilterManager.audFilter.pid != 0) { // parse only in audio enable
		ret = TsReadLib_ParseADTS(data, remain_size);
		if(ret != E_OK)
			return ret;
	}

	return E_OK;
}

ER TsReadLib_ParsePAT(UINT8 *addr, const UINT32 size)
{
	UINT32 pid = 0, adaptation_length = 0;
	UINT8* data = addr;

	// find PAT packet
	for (;;) {
		if (data == NULL) {
			DBG_ERR("Cannnot find PAT!!\r\n");
			return E_PAR;
		}

		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);
		if (pid == 0x00) {
			break;
		}

		data = data + TS_PACKET_SIZE;

		if((UINT32)(data - addr) >= size){
			DBG_ERR("buffer overflow!(0x%lx)\n", size);
			return E_MACV;
		}
	}

	if ((*(data + 3) & 0x20) != 0) { // Adaptation field
		adaptation_length = *(data + 4) + 1;
	}
	data = data + 13 + adaptation_length;// jump to program number

	gTsFilterManager.pmtFilter.program_number = *data << 8 | *(data + 1);
	gTsFilterManager.pmtFilter.pid = (*(data + 2) & 0x1F) << 8 | *(data + 3);
	gTsFilterManager.pmtFilter.type = MPEGTS_SECTION;

	//program amount = (section_length-9)/4, currently fixed to 1
	//(gTsReaderContainer.cbShowErrMsg)("pmt pid=%d\r\n", gTsFilterManager.pmtFilter.pid);

	return E_OK;
}

ER TsReadLib_ParsePMT(UINT8 *addr, const UINT32 size)
{
	UINT32 pid = 0;
	UINT8* data = addr;

	// find PMT packet
	for (;;) {
		if (data == NULL) {
			DBG_ERR("Cannnot find PMT!!\r\n");
			return E_PAR;
		}

		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);
		if (pid == gTsFilterManager.pmtFilter.pid) {
			break;
		}

		data = data + TS_PACKET_SIZE;

		if((UINT32)(data - addr) >= size){
			DBG_ERR("buffer overflow!(0x%lx)\n", size);
			return E_MACV;
		}
	}

	data += 13;// jump to PCR pid

	gTsFilterManager.pcrFilter.pid = (*(data) & 0x1F) << 8 | *(data + 1);
	gTsFilterManager.pcrFilter.type = MPEGTS_PCR;

	data += 4; // stream type
	if (*data == 0x1B) { //H264 first
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);
		gTsFilterManager.vidFilter.pid = pid;
		gTsFilterManager.vidFilter.type = MPEGTS_PES;
		gTsPlayInfo.vidCodecType = MEDIAVIDENC_H264;

		if (*(data + 5) == 0x0F) {
			pid = (*(data + 6) & 0x1F) << 8 | *(data + 7);
			gTsFilterManager.audFilter.pid = pid;
			gTsFilterManager.audFilter.type = MPEGTS_PES;
		} else {
			//no audio
			gTsFilterManager.audFilter.pid = 0;
		}
	} else if (*data == 0x24) { //H265 first
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);
		gTsFilterManager.vidFilter.pid = pid;
		gTsFilterManager.vidFilter.type = MPEGTS_PES;
		gTsPlayInfo.vidCodecType = MEDIAVIDENC_H265;

		if (*(data + 5) == 0x0F) {
			pid = (*(data + 6) & 0x1F) << 8 | *(data + 7);
			gTsFilterManager.audFilter.pid = pid;
			gTsFilterManager.audFilter.type = MPEGTS_PES;
		} else {
			//no audio
			gTsFilterManager.audFilter.pid = 0;
		}
	} else if (*data == 0x0F) { //AAC first
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);
		gTsFilterManager.audFilter.pid = pid;
		gTsFilterManager.audFilter.type = MPEGTS_PES;

		pid = (*(data + 6) & 0x1F) << 8 | *(data + 7);
		gTsFilterManager.vidFilter.pid = pid;
		gTsFilterManager.vidFilter.type = MPEGTS_PES;
	}
	//(gTsReaderContainer.cbShowErrMsg)("TsReadLib_ParsePMT! PCR pid=%d, video pid=%d, audio pid=%d\r\n",
	//    gTsFilterManager.pcrFilter.pid, gTsFilterManager.vidFilter.pid, gTsFilterManager.audFilter.pid);

	return E_OK;
}

static ER TsReadLib_LookUpSamplingFreq(UINT32 sample_freq_idx, UINT32 *p_sampling_freq)
{
	ER ret = E_OK;

	switch (sample_freq_idx) {
		case SAMPLEFREQ_96000:
			*p_sampling_freq = 96000;
			break;
		case SAMPLEFREQ_88200:
			*p_sampling_freq = 88200;
			break;
		case SAMPLEFREQ_64000:
			*p_sampling_freq = 64000;
			break;
		case SAMPLEFREQ_48000:
			*p_sampling_freq = 48000;
			break;
		case SAMPLEFREQ_44100:
			*p_sampling_freq = 44100;
			break;
		case SAMPLEFREQ_32000:
			*p_sampling_freq = 32000;
			break;
		case SAMPLEFREQ_24000:
			*p_sampling_freq = 24000;
			break;
		case SAMPLEFREQ_22050:
			*p_sampling_freq = 22050;
			break;
		case SAMPLEFREQ_16000:
			*p_sampling_freq = 16000;
			break;
		case SAMPLEFREQ_12000:
			*p_sampling_freq = 12000;
			break;
		case SAMPLEFREQ_11025:
			*p_sampling_freq = 11025;
			break;
		case SAMPLEFREQ_8000:
			*p_sampling_freq = 8000;
			break;
		case SAMPLEFREQ_7350:
			*p_sampling_freq = 7350;
			break;
		default:
			(gTsReaderContainer.cbShowErrMsg)("TsParser: unknown sampling freq index(%d)\r\n", sample_freq_idx);
			*p_sampling_freq = 0;
			ret = E_PAR;
			break;
	}

	return ret;
}

ER TsReadLib_ParseADTS(UINT8 *addr, const UINT32 size)
{
	UINT32 pid = 0;
	UINT32 start_indicator = 0;
	UINT32 sample_freq_idx = 0;
	UINT32 stuffing_len = 0;
	UINT8* data =  addr;

	// find PES packet
	for (;;) {
		if (data == NULL) {
			DBG_ERR("Cannnot find PES!!\r\n");
			return E_PAR;
		}

		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);
		if (pid == gTsFilterManager.audFilter.pid) { // aud pid should be 0x201
			start_indicator = *(data + 1) & 0x40;
			if (start_indicator) {
				break;
			}
		}

		data = data + TS_PACKET_SIZE;

		if((UINT32)(data - addr) >= size){
			DBG_WRN("buffer overflow!(0x%lx)\n", size);
			return E_MACV;
		}
	}

	TsReadLib_ParseStuffing(data, &stuffing_len);
	DBG_IND("TsReadLib_ParseADTS: stuffing_len=%d\r\n", stuffing_len);

	data += 4;                                     // skip sync byte, start_indicator and audio pid
	data += TS_AUD_PESHEADER_LENGTH;               // skip PES header
	data += stuffing_len;                          // skip PES header stuffing bytes if needed

	// parse ADTS header
	if ((*data & 0xFF) && (*(data + 1) & 0xF0)) { // check adts syncword
		//(gTsReaderContainer.cbShowErrMsg)("find ADTS!!\r\n");
		data += 2;                                 // skip Syncword, MPEG version, Layer and Protection Absent
		// sampling frequency
		sample_freq_idx = (*data & 0x3C) >> 2;
		TsReadLib_LookUpSamplingFreq(sample_freq_idx, &gTsPlayInfo.audSampleRate);
		// channel
		gTsPlayInfo.audChannel = ((*data & 0x01) << 2) | ((*(data + 1) & 0xC0) >> 6);
		//(gTsReaderContainer.cbShowErrMsg)("parse ch = %d\r\n", gTsPlayInfo.audChannel);
	}

	return E_OK;
}

ER TsReadLib_ParseStuffing(UINT8 *data, UINT32 *size)
{
#define TS_PTS_LENGTH 5
	UINT32 stuffing_pos, stuffing_len;
	UINT32 af_control = 0, af_length = 0;


	if (*data != 0x47) {
		*size = 0; // not need to parse stuffing bytes
		return E_OK;
	}

	/* skip af */
	af_control = (*(data + 3) & 0x30) >> 4;

	if(af_control == 3){
		af_length = *(data + 4);
		stuffing_pos = 5 + 8 + af_length;
	}
	else{
		stuffing_pos = 4 + 8;
	}

	if (*(data + stuffing_pos) > TS_PTS_LENGTH)
		stuffing_len = *(data + stuffing_pos) - TS_PTS_LENGTH;
	else
		stuffing_len = 0;

	*size = stuffing_len;

	return E_OK;
}

void TsReadLib_SaveIncompletePacketInfo(UINT32 remainLength, UINT8 *data)
{
	if (remainLength > 0) {
		gSplittedTSPacket.splitted1stHalfSize = remainLength;
		memcpy(&(gSplittedTSPacket.data[0]), data, remainLength);
	} else {
		gSplittedTSPacket.pid = 0;
		gSplittedTSPacket.splitted1stHalfSize = 0;
	}
}

void TsReadLib_SaveIncompleteFrameInfo(void)
{
	if (gTsPlayInfo.audFrameSize != 0) {
		gTsPlayInfo.currentPesType = PESTYPE_AUDIO;
	} else if (gTsPlayInfo.vidFrameSize != 0) {
		gTsPlayInfo.currentPesType = PESTYPE_VIDEO;
	} else {
		gTsPlayInfo.currentPesType = PESTYPE_NONE;
	}
}

void TsReadLib_RecoverFrame(UINT8 *data, UINT32 dstAddr, UINT32 *buff_offset)
{
#if _USER_ALLOC_ENTRY_BUF_
	TS_FRAME_ENTRY *pVidEntry, *pAudEntry;

	if (gpTsVideoEntry == NULL || gpTsAudioEntry == NULL) {
		DBG_ERR("pEntry is null\r\n");
		return;
	}
#endif

	if (gTsPlayInfo.currentPesType == PESTYPE_AUDIO) {
		//(gTsReaderContainer.cbShowErrMsg)("recover audio frame at %d\r\n",
		//    gTsPlayInfo.audFrmIndex);
#if _USER_ALLOC_ENTRY_BUF_
		pAudEntry = gpTsAudioEntry + (gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE);
		memcpy((UINT8 *)dstAddr, (UINT8 *)pAudEntry->memAddr, gTsPlayInfo.audFrameSize);
		pAudEntry->memAddr = dstAddr;
#else
		memcpy((UINT8 *)dstAddr, (UINT8 *)gTsAudioEntry[gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE].memAddr,
			   gTsPlayInfo.audFrameSize);
		gTsAudioEntry[gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE].memAddr = dstAddr;
#endif

		*buff_offset += gTsPlayInfo.audFrameSize;
	} else if (gTsPlayInfo.currentPesType == PESTYPE_VIDEO) {
		//(gTsReaderContainer.cbShowErrMsg)("recover video frame at %d\r\n",
		//    gTsPlayInfo.vidFrmIndex);
#if _USER_ALLOC_ENTRY_BUF_
		pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
		memcpy((UINT8 *)dstAddr, (UINT8 *)pVidEntry->memAddr, gTsPlayInfo.vidFrameSize);
		pVidEntry->memAddr = dstAddr;
#else
		memcpy((UINT8 *)dstAddr, (UINT8 *)gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].memAddr,
			   gTsPlayInfo.vidFrameSize);
		gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].memAddr = dstAddr;
#endif

		*buff_offset += gTsPlayInfo.vidFrameSize;
	}
}

void TsReadLib_RecoverTsPacket(UINT8 *data, UINT32 dstAddr, UINT32 *buff_offset, UINT32 splitted2ndHalfSize)
{
	UINT32 start_indicator, af_control, af_length, header_len;
	UINT32 stuffing_len = 0;
	UINT16 tmp;
#if _USER_ALLOC_ENTRY_BUF_
	TS_FRAME_ENTRY *pVidEntry, *pAudEntry;

	if (gpTsVideoEntry == NULL || gpTsAudioEntry == NULL) {
		DBG_ERR("pEntry is null\r\n");
		return;
	}
#endif

	//recover packet
	memcpy(&(gSplittedTSPacket.data[gSplittedTSPacket.splitted1stHalfSize]), data, splitted2ndHalfSize);
	gSplittedTSPacket.pid = (gSplittedTSPacket.data[1] & 0x1F) << 8 | gSplittedTSPacket.data[2];
	start_indicator = gSplittedTSPacket.data[1] & 0x40;
	af_control = (gSplittedTSPacket.data[3] & 0x30) >> 4;

	// splitted at start_indicator...
	if (start_indicator &&
		(gTsPlayInfo.currentPesType == PESTYPE_AUDIO || gTsPlayInfo.currentPesType == PESTYPE_VIDEO)) {
		TsReadLib_SaveFrameSize2Table();
	}

	if (gSplittedTSPacket.pid == gTsFilterManager.audFilter.pid) {
		if (start_indicator) {
			TsReadLib_ParseStuffing(data, &stuffing_len);
			DBG_IND("TsReadLib_RecoverTsPacket: [AUD]stuffing_len=%d\r\n", stuffing_len);
			header_len = 4 + (TS_AUD_PESHEADER_LENGTH + stuffing_len) + TS_AUD_ADTS_LENGTH;
#if _USER_ALLOC_ENTRY_BUF_
			pAudEntry = gpTsAudioEntry + (gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE);
			pAudEntry->memAddr = dstAddr + *buff_offset;
#else
			gTsAudioEntry[gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE].memAddr = dstAddr + *buff_offset;
#endif
			gTsPlayInfo.currentPesType = PESTYPE_AUDIO;
		} else if (af_control == 3) { // stuffing bytes
			af_length = gSplittedTSPacket.data[4];
			header_len = 5 + af_length;
		} else {
			header_len = 4;
		}

		if (header_len > TS_PACKET_SIZE) {
			DBG_ERR("TsReadLib_RecoverTsPacket header_len %d > TS_PACKET_SIZE %d\r\n", header_len, TS_PACKET_SIZE);
			return;
		}

		memcpy((UINT8 *)(dstAddr + *buff_offset), &(gSplittedTSPacket.data[header_len]), TS_PACKET_SIZE - header_len);

		gTsPlayInfo.audFrameSize += (TS_PACKET_SIZE - header_len);
		*buff_offset += (TS_PACKET_SIZE - header_len);
	} else if (gSplittedTSPacket.pid == gTsFilterManager.vidFilter.pid) {
		if (start_indicator) {
			TsReadLib_ParseStuffing(data, &stuffing_len);
			DBG_IND("TsReadLib_RecoverTsPacket: [VID]stuffing_len=%d\r\n", stuffing_len);
			if (af_control == 3) {
				af_length = gSplittedTSPacket.data[4];
				header_len = 5 + af_length + TS_VID_PESHEADER_LENGTH + gTsPlayInfo.vidNalLength + stuffing_len;
			} else {
				tmp = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength;
				// H.264
				if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H264 && gSplittedTSPacket.data[tmp] == 0x00 && gSplittedTSPacket.data[tmp + 1] == 0x00 && gSplittedTSPacket.data[tmp + 2] == 0x00 &&
					gSplittedTSPacket.data[tmp + 3] == 0x01 && gSplittedTSPacket.data[tmp + 4] == 0x67) {
					header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength + gTsPlayInfo.h264VidDescLen;
					gTsPlayInfo.gopNumber = gTsPlayInfo.vidFrmIndex - gLastI_index;
					gLastI_index = gTsPlayInfo.vidFrmIndex;
#if _USER_ALLOC_ENTRY_BUF_
					pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
					pVidEntry->key_frame = 1;
#else
					gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].key_frame = 1;
#endif
				// H.265
				} else if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H265 && gSplittedTSPacket.data[tmp] == 0x00 && gSplittedTSPacket.data[tmp + 1] == 0x00 && gSplittedTSPacket.data[tmp + 2] == 0x00 &&
						   gSplittedTSPacket.data[tmp + 3] == 0x01 && gSplittedTSPacket.data[tmp + 4] == 0x40) {
					header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength + gTsPlayInfo.h264VidDescLen;
					gTsPlayInfo.gopNumber = gTsPlayInfo.vidFrmIndex - gLastI_index;
					gLastI_index = gTsPlayInfo.vidFrmIndex;
#if _USER_ALLOC_ENTRY_BUF_
					pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
					pVidEntry->key_frame = 1;
#else
					gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].key_frame = 1;
#endif
				} else {
					header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength;
				}
			}
#if _USER_ALLOC_ENTRY_BUF_
			pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
			pVidEntry->memAddr = dstAddr + *buff_offset;
			if (pVidEntry->key_frame == 1 && gTsPlayInfo.vidFrmIndex != 0 && gTsPlayInfo.keyFrameDist == 0) {
				gTsPlayInfo.keyFrameDist = pVidEntry->memAddr - gpTsVideoEntry->memAddr;
			}
#else
			gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].memAddr = dstAddr + *buff_offset;
			if (gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].key_frame == 1
				&& gTsPlayInfo.vidFrmIndex != 0
				&& gTsPlayInfo.keyFrameDist == 0) {
				gTsPlayInfo.keyFrameDist = gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].memAddr - gTsVideoEntry[0].memAddr;
			}
#endif
			gTsPlayInfo.currentPesType = PESTYPE_VIDEO;
		} else if (af_control == 3) { // stuffing bytes
			af_length = gSplittedTSPacket.data[4];
			header_len = 5 + af_length;
		} else {
			header_len = 4;
		}

		if (header_len > TS_PACKET_SIZE) {
			DBG_ERR("TsReadLib_RecoverTsPacket header_len %d > TS_PACKET_SIZE %d\r\n", header_len, TS_PACKET_SIZE);
			return;
		}

		memcpy((UINT8 *)(dstAddr + *buff_offset), &(gSplittedTSPacket.data[header_len]), TS_PACKET_SIZE - header_len);

		gTsPlayInfo.vidFrameSize += (TS_PACKET_SIZE - header_len);
		*buff_offset += (TS_PACKET_SIZE - header_len);
	} else {
		gTsPlayInfo.currentPesType = PESTYPE_NONE;
	}
}

void TsReadLib_ClearFrameEntry(void)
{
#if _USER_ALLOC_ENTRY_BUF_
	if (gpTsVideoEntry == NULL || gpTsAudioEntry == NULL) {
		DBG_ERR("pEntry is null\r\n");
		return;
	}

	memset(gpTsVideoEntry, 0, TSVID_ENTRY_SIZE * sizeof(TS_FRAME_ENTRY));
	memset(gpTsAudioEntry, 0, TSAUD_ENTRY_SIZE * sizeof(TS_FRAME_ENTRY));
#else
	memset(gTsVideoEntry, 0, TSVID_ENTRY_SIZE * sizeof(TS_FRAME_ENTRY));
	memset(gTsAudioEntry, 0, TSAUD_ENTRY_SIZE * sizeof(TS_FRAME_ENTRY));
#endif
}

BOOL TsReadLib_DemuxFinished(void)
{
	return gTsPlayInfo.demuxFinished;
}

void TsReadLib_setDemuxFinished(BOOL finished)
{
	gTsPlayInfo.demuxFinished = finished;
}

void TsReadLib_registerTSDemuxCallback(void (*demuxCB)(UINT32, UINT32))
{
	TsReadLib_DemuxCB = demuxCB;
}

UINT64 TsReadLib_ParsePCR(UINT8 *data)
{
	UINT32 pcrExt;
	UINT64 pcrBase;

	if (data == NULL) {
		DBG_ERR("TsReadLib_ParsePCR got NULL!\r\n");
		return E_PAR;
	}

	pcrBase = (UINT64)((*(data + 6) << 25) | (*(data + 7) << 17) | (*(data + 8) << 9) | (*(data + 9) << 1) | ((*(data + 10) >> 7) & 0x01));
	pcrExt = ((*(data + 10) & 0x01) << 8) | *(data + 11);

	gTsPlayInfo.pcrValue = pcrBase * 300 + pcrExt;

	//(gTsReaderContainer.cbShowErrMsg)("gTsPlayInfo.pcrValue=0x%llx\r\n", gTsPlayInfo.pcrValue);
	return gTsPlayInfo.pcrValue;
}

UINT64 TsReadLib_ParsePTS(UINT8 *data)
{
	return (UINT64)(*data & 0x0e) << 29 |
		   ((*(data + 1) << 8 | *(data + 2)) >> 1) << 15 |
		   (*(data + 3) << 8 | *(data + 4)) >> 1;
}

ER TsReadLib_SaveFrameSize2Table(void)
{
#if _USER_ALLOC_ENTRY_BUF_
	TS_FRAME_ENTRY *pVidEntry, *pAudEntry;

	if (gpTsVideoEntry == NULL || gpTsAudioEntry == NULL) {
		DBG_ERR("pEntry is null\r\n");
		return E_SYS;
	}
#endif

	if (gTsPlayInfo.currentPesType == PESTYPE_AUDIO) {
#if _USER_ALLOC_ENTRY_BUF_
		pAudEntry = gpTsAudioEntry + (gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE);
		pAudEntry->size = gTsPlayInfo.audFrameSize;
		pAudEntry->frameIdx = gTsPlayInfo.audFrmIndex;
		DBG_IND("pAudEntry->frameIdx =%lu\n", pAudEntry->frameIdx);
#else
		gTsAudioEntry[gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE].size = gTsPlayInfo.audFrameSize;
		gTsAudioEntry[gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE].frameIdx = gTsPlayInfo.audFrmIndex;
#endif
		gTsPlayInfo.audFrmIndex++;
		gTsPlayInfo.audFrameSize = 0;

		//(gTsReaderContainer.cbShowErrMsg)("aud%d addr=0x%x size=0x%x\r\n",
		//                gTsPlayInfo.audFrmIndex-1, gTsAudioEntry[gTsPlayInfo.audFrmIndex-1].memAddr,
		//                gTsAudioEntry[gTsPlayInfo.audFrmIndex-1].size);
	} else if (gTsPlayInfo.currentPesType == PESTYPE_VIDEO) {
#if _USER_ALLOC_ENTRY_BUF_
		pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
		pVidEntry->size = gTsPlayInfo.vidFrameSize;
		pVidEntry->frameIdx = gTsPlayInfo.vidFrmIndex;
#else
		gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].size = gTsPlayInfo.vidFrameSize;
		gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].frameIdx = gTsPlayInfo.vidFrmIndex;
#endif
		gTsPlayInfo.vidFrmIndex++;
		gTsPlayInfo.vidFrameSize = 0;

		//(gTsReaderContainer.cbShowErrMsg)("vid%d addr=0x%x size=0x%x\r\n",
		//                gTsPlayInfo.vidFrmIndex-1, gTsVideoEntry[gTsPlayInfo.vidFrmIndex-1].memAddr,
		//                gTsVideoEntry[gTsPlayInfo.vidFrmIndex-1].size);
	}

	return E_OK;
}

BOOL TsReadLib_AESDECRYP2(UINT8 *PlanText,UINT8 *uiKey,UINT8 *CypherText)
{
#if defined (__UITRON) || defined (__ECOS)
    crypto_open();
    crypto_setMode(CRYPTO_AES, CRYPTO_EBC, CRYPTO_DECRYPT, CRYPTO_PIO);
    crypto_setKey(uiKey);
    crypto_setInput(PlanText);
    crypto_getOutput(CypherText);
    while(crypto_isBusy()) {
        DBG_ERR("crypto_isBusy\r\n");
    }
    crypto_close();
#endif
    return TRUE;
}

ER TsReadLib_Decrypt(UINT32 addr, UINT32 size)
{
	//UINT32 temp_time = Perf_GetCurrent(), temp_time2;
	UINT32 diff = 0;
	TSPLAY_CRYPTO_INFO *p_crypto = &g_decrypt_info;
	UINT8 gen_key[16] = {0}; //gen key
	UINT8 *key;
	UINT8 *pos; //gen key pos
	UINT32 cnt, i;
	ER r = E_OK;

	//trans
	key = p_crypto->key;
	if (key == NULL) {
		DBG_DUMP("^G crypto key NULL\r\n");
		return r;
	}
	//dump
	if (0) {
		DBG_DUMP("^G EN adr=0x%lx, siz=0x%lx.\r\n", addr, size);
		DBG_DUMP("^G key\r\n");
		for (i = 0; i < 4; i++) {
			DBG_DUMP("^G %x ", p_crypto->key[i]);
		}
		DBG_DUMP("\r\n");
	}

	//size cnt
	if (size % 16) {
		DBG_DUMP("^G crypto size error\r\n");
		return r;
	}
	cnt = size / 16;

	//gen key
	for (i = 0; i < 4; i++) {
		memcpy(&gen_key[i*4], key, 4);
	}
	//dump
	if (0) {
		DBG_DUMP("^G gen key\r\n");
		for (i=0; i<16; i++) {
			DBG_DUMP("0x%x ", gen_key[i]);
		}
		DBG_DUMP("\r\n");
	}

	//action
	//temp_time2 = Perf_GetCurrent();
	for (i = 0; i < cnt; i++) {
		pos = (UINT8 *)(addr + i*16);

		//dump
		if (0) {
			UINT32 dp;
			for (dp = 0; dp < 16; dp++) {
				if (dp%16 == 0) {
					DBG_DUMP("\r\n");
				}
				DBG_DUMP("0x%02X ",*(pos+dp));
			}
			DBG_DUMP("\r\n");
		}

		TsReadLib_AESDECRYP2(pos, gen_key, pos);
		//dump
		if (0) {
			UINT32 dp;
			for (dp = 0; dp < 16; dp++) {
				if (dp%16 == 0) {
					DBG_DUMP("\r\n");
				}
				DBG_DUMP("0x%02X ",*(pos+dp));
			}
			DBG_DUMP("\r\n");
		}
	}

	if (0) {
		//diff = (Perf_GetCurrent() - temp_time2) / 1000;
		DBG_DUMP("^C TsReadLib_AESDECRYP2 %dms\r\n", diff);
	}

	//store
	p_crypto->addr = addr;
	p_crypto->size = size;
	//dump
	if (0)
	{
		DBG_DUMP("EN addr %d size %d\r\n", p_crypto->addr, p_crypto->size);
	}
	DBG_IND("^G Decrypt done\r\n");

	if (0)
	{
		//diff = (Perf_GetCurrent() - temp_time) / 1000;
		DBG_DUMP("^C Decrypt %dms\r\n", diff);
	}

	return r;
}

INT32 TsReadLib_IsDecrypt(void)
{
	TSPLAY_CRYPTO_INFO *p_crypto = &g_decrypt_info;
	return p_crypto->is_on;
}

ER TsReadLib_SetDecryptKey(UINT8 *key)
{
	TSPLAY_CRYPTO_INFO *p_crypto = &g_decrypt_info;
	UINT8 idx;
	ER r = E_OK;

	DBG_DUMP("^G SetDeCryptKey\r\n");

	memset(p_crypto, 0, sizeof(TSPLAY_CRYPTO_INFO));

	if (key != NULL) {
		p_crypto->is_on = 1;
		for (idx = 0; idx < 4; idx++) {
			p_crypto->key[idx] = *(key + idx);
			DBG_IND("^G [%d] %x\r\n", idx, p_crypto->key[idx]);
		}
	}

	return r;
}

ER TsReadLib_DemuxTsStream(UINT32 srcAddr, UINT32 readSize, UINT32 dstAddr, UINT32 *usedSize)
{
	UINT32 packet_num, pid = 0, remain_len, i, start_indicator = 0;
	UINT32 af_control = 0, af_length = 0, splitted2ndHalfSize = 0;
	UINT64 dataSize = 0;
	UINT32 buf_offset = 0, header_len, duplicateSize = 0;
	UINT32 stuffing_len = 0;
	UINT8 *data;
	UINT16 tmp;
	UINT8 decrypt_buf[256] = {0};

#if _USER_ALLOC_ENTRY_BUF_
	TS_FRAME_ENTRY *pVidEntry, *pAudEntry;

	if (gpTsVideoEntry == NULL || gpTsAudioEntry == NULL) {
		DBG_ERR("pEntry is null\r\n");
		return E_SYS;
	}
#endif
	data = (UINT8 *)srcAddr;

	// set crypto key
	//unsigned char key[4] = {0x11, 0x22, 0x33, 0x44};
	//TsReadLib_SetDecryptKey(key);

	// seek mode
	if (gTsSeekInfo.targetPts != 0) {
		UINT32 retAddr;
		retAddr = TsReadLib_SeekTsStream(srcAddr, readSize, gTsSeekInfo.targetPts);
		if (retAddr == 0) {
			DBG_ERR("^R Cannot find PTS!, diff = %lld\r\n", gTsSeekInfo.diff);
			return E_PAR;
		} else {
			data = (UINT8 *)retAddr;
			readSize = readSize - (retAddr - srcAddr);
			gTsReadLib_Info.remainFileSize -= (retAddr - srcAddr);
			if (gTsReadLib_Info.remainFileSize > readSize && (gTsReadLib_Info.remainFileSize - readSize) >= 0x200000) { //read more blocks before resuming normal play to prevent buffer empty
				// insufficient data, need to read one more block
				gTsSeekInfo.result = SEEKRESULT_ONEMORE;
			}
		}
	} else {
		//reset
		gTsSeekInfo.result = SEEKRESULT_AHEAD;
	}

	//(gTsReaderContainer.cbShowErrMsg)("dstAddr is 0x%x\r\n", dstAddr);

	//need to recover audio/video frame after rollback
	if (dstAddr == gTsReadLib_Info.memStartAddr/* && !(*(data + 1) & 0x40)*/) {
		TsReadLib_RecoverFrame(data, dstAddr, &buf_offset);
		duplicateSize = buf_offset;
	}

	dataSize = gTsReadLib_Info.remainFileSize - gTsReadLib_Info.parsedFileSize;
	if (dataSize > readSize) {
		dataSize = readSize;
	}

	//(gTsReaderContainer.cbShowErrMsg)("remainFileSize=0x%llx, parsedsize=0x%llx, dataSize=0x%llx\r\n",
	//    gTsReadLib_Info.remainFileSize, gTsReadLib_Info.parsedFileSize, dataSize);

	if (gSplittedTSPacket.splitted1stHalfSize != 0) {
		splitted2ndHalfSize = TS_PACKET_SIZE - gSplittedTSPacket.splitted1stHalfSize;
	}
	packet_num = (dataSize - splitted2ndHalfSize) / TS_PACKET_SIZE;
	remain_len = (dataSize - splitted2ndHalfSize) % TS_PACKET_SIZE;

	//(gTsReaderContainer.cbShowErrMsg)("packet_num=%d, remain_len=%d, splitted2ndHalfSize=%d\r\n", packet_num, remain_len, splitted2ndHalfSize);

	//splitted packet, merge it back
	if (splitted2ndHalfSize != 0) {
		TsReadLib_RecoverTsPacket(data, dstAddr, &buf_offset, splitted2ndHalfSize);
	}

	// find TS sync byte
	for (;;) {
		if (*data == 0x47 && *(data + TS_PACKET_SIZE) == 0x47 && *(data + TS_PACKET_SIZE * 2) == 0x47) {
			break;
		}
		data++;
	}

	for (i = 0; i < packet_num; i++) {
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);

		if (pid == gTsFilterManager.audFilter.pid) {
			start_indicator = *(data + 1) & 0x40;
			af_control = (*(data + 3) & 0x30) >> 4;

			if (start_indicator) {
				// save previous frame size
				if (gTsPlayInfo.currentPesType == PESTYPE_AUDIO || gTsPlayInfo.currentPesType == PESTYPE_VIDEO) {
					TsReadLib_SaveFrameSize2Table();
				}
				TsReadLib_ParseStuffing(data, &stuffing_len);
				DBG_IND("TsReadLib_DemuxTsStream: [AUD]stuffing_len=%d\r\n", stuffing_len);
				header_len = 4 + (TS_AUD_PESHEADER_LENGTH + stuffing_len) + TS_AUD_ADTS_LENGTH; // skip TS header (0x 47 42 10 00) + PES header (0x 00 00 00 01 C0 ...) + ADTS header (0x FF F9 ...)
#if _USER_ALLOC_ENTRY_BUF_
				pAudEntry = gpTsAudioEntry + (gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE);
				pAudEntry->memAddr = dstAddr + buf_offset;
#else
				gTsAudioEntry[gTsPlayInfo.audFrmIndex % TSAUD_ENTRY_SIZE].memAddr = dstAddr + buf_offset;
#endif
				gTsPlayInfo.currentPesType = PESTYPE_AUDIO;
			} else if (af_control == 3) { // stuffing bytes
				af_length = *(data + 4);
				header_len = 5 + af_length;
			} else {
				header_len = 4;
			}

			if (header_len > TS_PACKET_SIZE) {
				DBG_ERR("TsReadLib_DemuxTsStream header_len %d > TS_PACKET_SIZE %d\r\n", header_len, TS_PACKET_SIZE);
				return E_SYS;
			}

			memcpy((UINT8 *)(dstAddr + buf_offset), data + header_len, TS_PACKET_SIZE - header_len);

			gTsPlayInfo.audFrameSize += (TS_PACKET_SIZE - header_len);
			buf_offset += (TS_PACKET_SIZE - header_len);
		} else if (pid == gTsFilterManager.vidFilter.pid) {
			start_indicator = *(data + 1) & 0x40;
			af_control = (*(data + 3) & 0x30) >> 4;

			if (start_indicator) {
				// save previous frame size
				if (gTsPlayInfo.currentPesType == PESTYPE_AUDIO || gTsPlayInfo.currentPesType == PESTYPE_VIDEO) {
					TsReadLib_SaveFrameSize2Table();
				}

				TsReadLib_ParseStuffing(data, &stuffing_len);
				DBG_IND("TsReadLib_DemuxTsStream: [VID]stuffing_len=%d\r\n", stuffing_len);

				if (af_control == 3) {
					af_length = *(data + 4);
					header_len = 5 + af_length + TS_VID_PESHEADER_LENGTH + gTsPlayInfo.vidNalLength + stuffing_len;
				} else {
					tmp = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength;
					// H.264
					if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H264 && *(data + tmp) == 0x00 && *(data + tmp + 1) == 0x00 && *(data + tmp + 2) == 0x00 &&
						*(data + tmp + 3) == 0x01 && *(data + tmp + 4) == 0x67) {
						gTsPlayInfo.gopNumber = gTsPlayInfo.vidFrmIndex - gLastI_index;
						header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength + gTsPlayInfo.h264VidDescLen;
						gLastI_index = gTsPlayInfo.vidFrmIndex;

						// decrypt data if decrypt is enable
						if (TsReadLib_IsDecrypt()) {
							UINT32 ts_header_len = header_len - gTsPlayInfo.h264VidDescLen;
							UINT8 *next_packet_data = data + TS_PACKET_SIZE;
							//decrypt 16 bytes data for each decrypt process
							memcpy(decrypt_buf, data+ts_header_len+16, TS_PACKET_SIZE-ts_header_len-16);
							memcpy(decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), next_packet_data+4, DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));

							TsReadLib_Decrypt((UINT32)decrypt_buf, DECRYPT_BLK);

							memcpy(data+ts_header_len+16, decrypt_buf, TS_PACKET_SIZE-ts_header_len-16);
							memcpy(next_packet_data+4, decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));
						}

#if _USER_ALLOC_ENTRY_BUF_
						pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
						pVidEntry->key_frame = 1;
#else
						gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].key_frame = 1;
#endif
					// H.265
					} else if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H265 && *(data + tmp) == 0x00 && *(data + tmp + 1) == 0x00 && *(data + tmp + 2) == 0x00 &&
							   *(data + tmp + 3) == 0x01 && *(data + tmp + 4) == 0x40) {
						header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength + gTsPlayInfo.h264VidDescLen;
						gTsPlayInfo.gopNumber = gTsPlayInfo.vidFrmIndex - gLastI_index;
						gLastI_index = gTsPlayInfo.vidFrmIndex;

						// decrypt data if decrypt is enable
						if (TsReadLib_IsDecrypt()) {
							UINT32 ts_header_len = header_len - gTsPlayInfo.h264VidDescLen;
							UINT8 *next_packet_data = data + TS_PACKET_SIZE;
							//decrypt 16 bytes data for each decrypt process
							memcpy(decrypt_buf, data+ts_header_len+16, TS_PACKET_SIZE-ts_header_len-16);
							memcpy(decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), next_packet_data+4, DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));

							TsReadLib_Decrypt((UINT32)decrypt_buf, DECRYPT_BLK);

							memcpy(data+ts_header_len+16, decrypt_buf, TS_PACKET_SIZE-ts_header_len-16);
							memcpy(next_packet_data+4, decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));
						}

#if _USER_ALLOC_ENTRY_BUF_
						pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
						pVidEntry->key_frame = 1;
#else
						gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].key_frame = 1;
#endif
					} else {
						header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength;
					}
				}
#if _USER_ALLOC_ENTRY_BUF_
				pVidEntry = gpTsVideoEntry + (gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE);
				pVidEntry->memAddr = dstAddr + buf_offset;
				if (pVidEntry->key_frame == 1 && gTsPlayInfo.vidFrmIndex != 0 && gTsPlayInfo.keyFrameDist == 0) {
					gTsPlayInfo.keyFrameDist = pVidEntry->memAddr - gpTsVideoEntry->memAddr;
				}
#else
				gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].memAddr = dstAddr + buf_offset;
				if (gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].key_frame == 1
					&& gTsPlayInfo.vidFrmIndex != 0
					&& gTsPlayInfo.keyFrameDist == 0) {
					gTsPlayInfo.keyFrameDist = gTsVideoEntry[gTsPlayInfo.vidFrmIndex % TSVID_ENTRY_SIZE].memAddr - gTsVideoEntry[0].memAddr;
				}
#endif
				//(gTsReaderContainer.cbShowErrMsg)("vid %d addr is 0x%x\r\n", gTsPlayInfo.vidFrmIndex, gTsVideoEntry[gTsPlayInfo.vidFrmIndex].memAddr);
				gTsPlayInfo.currentPesType = PESTYPE_VIDEO;
			} else if (af_control == 3) { // stuffing bytes
				af_length = *(data + 4);
				header_len = 5 + af_length;
			} else {
				header_len = 4;
			}

			if (header_len > TS_PACKET_SIZE) {
				DBG_ERR("TsReadLib_DemuxTsStream header_len %d > TS_PACKET_SIZE %d\r\n", header_len, TS_PACKET_SIZE);
				return E_SYS;
			}

			memcpy((UINT8 *)(dstAddr + buf_offset), data + header_len,
				   TS_PACKET_SIZE - header_len);

			gTsPlayInfo.vidFrameSize += (TS_PACKET_SIZE - header_len);
			buf_offset += (TS_PACKET_SIZE - header_len);
		} else {
			if (gTsPlayInfo.currentPesType == PESTYPE_AUDIO || gTsPlayInfo.currentPesType == PESTYPE_VIDEO) {
				TsReadLib_SaveFrameSize2Table();
			}
			gTsPlayInfo.currentPesType = PESTYPE_NONE;
		}

		data = data + TS_PACKET_SIZE;
	}

	//Handle last frame in file
	if (dataSize < readSize && (gTsPlayInfo.currentPesType == PESTYPE_AUDIO || gTsPlayInfo.currentPesType == PESTYPE_VIDEO)) {
		TsReadLib_SaveFrameSize2Table();
	}

	*usedSize = buf_offset;
	gTsReadLib_Info.parsedFileSize += readSize;

	//save spitted packet/frame info
	TsReadLib_SaveIncompletePacketInfo(remain_len, data);
	TsReadLib_SaveIncompleteFrameInfo();
	gTsPlayInfo.demuxFinished = TRUE;
	TsReadLib_DemuxCB(buf_offset, duplicateSize);

	//(gTsReaderContainer.cbShowErrMsg)("TSdemux finish... use 0x%x, remain_len=0x%x, pid=%d, idx=%d\r\n", buf_offset,
	//    remain_len, pid, gTsPlayInfo.vidFrmIndex);

	return E_OK;
}

ER TsReadLib_Parse1stVideo(UINT32 hdrAddr, UINT32 hdrSize, void *pobj)
{
	UINT32 packet_num, pid, i, j = 0, step = 0, start_indicator = 0, af_control, af_length = 0;
	UINT64 pts1 = 0, pts2 = 0, pts_length = 5;
	UINT8 *data;
	MEDIA_FIRST_INFO *ptr;
	BOOL videoFound = FALSE, startPcrGet = FALSE, bStillVdoPid = TRUE;
	UINT8 decrypt_buf[256] = {0};
	ER ret;


	//(gTsReaderContainer.cbShowErrMsg)("TsReadLib_Parse1stVideo hdrAddr=%x, size=%x\r\n", hdrAddr, hdrSize);
	ptr = (MEDIA_FIRST_INFO *)pobj;
	packet_num = hdrSize / TS_PACKET_SIZE;
	gPrevPid = 0;

	// config section/pes filters
	ret = TsReadLib_ConfigFilters(hdrAddr, hdrSize);
	if(ret != E_OK){
		ptr->bStillVdoPid = bStillVdoPid;
		return ret;
	}

	// set crypto key
	//unsigned char key[4] = {0x11, 0x22, 0x33, 0x44};
	//TsReadLib_SetDecryptKey(key);

	// set video NAL length
	if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H264) {
		gTsPlayInfo.vidNalLength = TS_VID_264_NAL_LENGTH;
	} else if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H265) {
		gTsPlayInfo.vidNalLength = TS_VID_265_NAL_LENGTH;
	} else {
		DBG_ERR("[TS]Parse1stVideo_%d: invalid codec(%d)\r\n", __LINE__, gTsPlayInfo.vidCodecType);
		return E_SYS;
	}

	data = (UINT8 *)hdrAddr;
	// find TS sync byte
	for (;;) {
		if (*data == 0x47) {
			break;
		}
		data++;

		if(((UINT32)data - hdrAddr) >= hdrSize){
			DBG_ERR("buffer overflow!(0x%lx)\n", hdrSize);
			return E_MACV;
		}
	}

	for (i = 0; i < packet_num; i++) {
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);

		if (pid == gTsFilterManager.pcrFilter.pid && !startPcrGet) {
			gTsPlayInfo.pcrStartValue = TsReadLib_ParsePCR(data);
			startPcrGet = TRUE;
		}

		if (pid == gTsFilterManager.vidFilter.pid) {
			start_indicator = *(data + 1) & 0x40;       // start of this PES
			af_control = (*(data + 3) & 0x30) >> 4;     // descript adative field
			if (start_indicator) {
				UINT32 header_len = 4 + TS_VID_PESHEADER_LENGTH + gTsPlayInfo.vidNalLength;
				UINT32 stuffing_len = 0;

				TsReadLib_ParseStuffing(data, &stuffing_len);
				DBG_IND("TsReadLib_Parse1stVideo: stuffing_len=%d\r\n", stuffing_len);
				header_len += stuffing_len;

				if (videoFound) {
					if (af_control == 3) {
						af_length = *(data + 4);
						pts2 = TsReadLib_ParsePTS(data + 5 + af_length + (TS_VID_PESHEADER_LENGTH - pts_length));
						bStillVdoPid = FALSE;
						break;
					} else {
						pts2 = TsReadLib_ParsePTS(data + 4 + (TS_VID_PESHEADER_LENGTH - pts_length));
						bStillVdoPid = FALSE;
						break;
					}
				}

				if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H264) {
					for (j = header_len; j < TS_PACKET_SIZE; j++) {
						// decrypt data if decrypt is enable
						if (*(data + j) == 0x00 && *(data + j + 1) == 0x00 && *(data + j + 2) == 0x00 &&
							*(data + j + 3) == 0x01 && *(data + j + 4) == 0x67 && TsReadLib_IsDecrypt()) {
							UINT8 *next_packet_data = data + TS_PACKET_SIZE;
							//decrypt 16 bytes data for each decrypt process
							memcpy(decrypt_buf, data+header_len+16, TS_PACKET_SIZE-header_len-16);
							memcpy(decrypt_buf+(TS_PACKET_SIZE-header_len-16), next_packet_data+4, DECRYPT_BLK - (TS_PACKET_SIZE-header_len-16));

							TsReadLib_Decrypt((UINT32)decrypt_buf, DECRYPT_BLK);

							memcpy(data+header_len+16, decrypt_buf, TS_PACKET_SIZE-header_len-16);
							memcpy(next_packet_data+4, decrypt_buf+(TS_PACKET_SIZE-header_len-16), DECRYPT_BLK - (TS_PACKET_SIZE-header_len-16));
						}

						if (*(data + j) == 0x00 && *(data + j + 1) == 0x00 && *(data + j + 2) == 0x00 &&
							*(data + j + 3) == 0x01 && *(data + j + 4) == 0x65) {
							break;
						}
					}
				} else if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H265) {
					for (j = header_len; j < TS_PACKET_SIZE; j++) {
						// decrypt data if decrypt is enable
						if (*(data + j) == 0x00 && *(data + j + 1) == 0x00 && *(data + j + 2) == 0x00 &&
							*(data + j + 3) == 0x01 && *(data + j + 4) == 0x40 && TsReadLib_IsDecrypt()) {
							UINT8 *next_packet_data = data + TS_PACKET_SIZE;
							//decrypt 16 bytes data for each decrypt process
							memcpy(decrypt_buf, data+header_len+16, TS_PACKET_SIZE-header_len-16);
							memcpy(decrypt_buf+(TS_PACKET_SIZE-header_len-16), next_packet_data+4, DECRYPT_BLK - (TS_PACKET_SIZE-header_len-16));

							TsReadLib_Decrypt((UINT32)decrypt_buf, DECRYPT_BLK);

							memcpy(data+header_len+16, decrypt_buf, TS_PACKET_SIZE-header_len-16);
							memcpy(next_packet_data+4, decrypt_buf+(TS_PACKET_SIZE-header_len-16), DECRYPT_BLK - (TS_PACKET_SIZE-header_len-16));
						}

						if (*(data + j) == 0x00 && *(data + j + 1) == 0x00 && *(data + j + 2) == 0x00 &&
							*(data + j + 3) == 0x01) {
							if ((*(data + j + 4) & 0x7E) >> 1 == 19) { // Type: 19 = IDR
								//(gTsReaderContainer.cbShowErrMsg)("h265 I found!\r\n");
								break;
							}
						}
					}
				} else {
					DBG_ERR("[TS]Parse1stVideo: invalid codec(%d)\r\n", gTsPlayInfo.vidCodecType);
					return E_SYS;
				}

				if (j < TS_PACKET_SIZE) { //I frame found
					gTsPlayInfo.h264VidDescLen = j - header_len;

					// SPS + PPS + VPS(only H.265)
					memcpy(&(gTsPlayInfo.h264VidDesc), data + header_len, gTsPlayInfo.h264VidDescLen);

					// pure BS
					if ((step < gTsDemuxSize) && (step + TS_PACKET_SIZE - (header_len + gTsPlayInfo.h264VidDescLen) < gTsDemuxSize)) {
						memcpy((UINT8 *)(gTsDemuxAddr + step), data + header_len + gTsPlayInfo.h264VidDescLen,
							   TS_PACKET_SIZE - (header_len + gTsPlayInfo.h264VidDescLen));
					} else {
						DBG_ERR("pure BS over size ! please set buffer size > %x\r\n", gTsDemuxSize);
						return E_SYS;
					}

					step += TS_PACKET_SIZE - (header_len + gTsPlayInfo.h264VidDescLen);
					videoFound = TRUE;
					pts1 = TsReadLib_ParsePTS(data + 4 + (TS_VID_PESHEADER_LENGTH - pts_length));
					gTsPlayInfo.firstPts = pts1;
				}
			} else {
				af_control = (*(data + 3) & 0x30) >> 4;
				// stuffing bytes
				if (af_control == 3) {
					af_length = *(data + 4);
					if ((step < gTsDemuxSize) && (step + TS_PACKET_SIZE - (5 + af_length) < gTsDemuxSize)) {
						memcpy((UINT8 *)(gTsDemuxAddr + step), data + 5 + af_length, TS_PACKET_SIZE - (5 + af_length));
						step += TS_PACKET_SIZE - (5 + af_length);
					} else {
						DBG_ERR("1 stuffing bytes over size ! please set buffer size > %x\r\n", gTsDemuxSize);
						return E_SYS;
					}
				} else {
					if ((step < gTsDemuxSize) && (step + TS_PACKET_SIZE - 4 < gTsDemuxSize)) {
						memcpy((UINT8 *)(gTsDemuxAddr + step), data + 4, TS_PACKET_SIZE - 4);
						step += TS_PACKET_SIZE - 4;
					} else {
						DBG_ERR("2 stuffing bytes over size ! please set buffer size > %x\r\n", gTsDemuxSize);
						return E_SYS;
					}
				}
				//(gTsReaderContainer.cbShowErrMsg)("%x %x %x %x\r\n",gTempVidBuf[step],gTempVidBuf[step+1],
				//    gTempVidBuf[step+2],gTempVidBuf[step+3]);
			}
		}

		else if (gPrevPid == gTsFilterManager.vidFilter.pid && (pid == gTsFilterManager.audFilter.pid || pid == 0)) {
			bStillVdoPid = FALSE;
		}
		gPrevPid = pid;

		data = data + TS_PACKET_SIZE;
	}

	if ((pts2 - pts1) != 0) {
		gTsPlayInfo.vidFPS = 90000 / (pts2 - pts1);
	} else {
		DBG_ERR("ERR: [TS] Can't calc vidFPS, pts2=%lld, pts=%lld\r\n", pts2, pts1);
	}

	if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H264) {
		TsReadLib_ParseH264(&(gTsPlayInfo.h264VidDesc[5]), gTsPlayInfo.h264VidDescLen);
	} else if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H265) {
		TsReadLib_ParseH265(&(gTsPlayInfo.h264VidDesc[0]), gTsPlayInfo.h264VidDescLen);
	} else {
		DBG_ERR("[TS] invalid vidCodec(%d)\r\n", gTsPlayInfo.vidCodecType);
		return E_SYS;
	}

	ptr->pos = gTsDemuxAddr - hdrAddr; //get offset as pos
	ptr->size = step;
	ptr->audSR = gTsPlayInfo.audSampleRate;
	ptr->vidtype = gTsPlayInfo.vidCodecType;
	ptr->vidFR = gTsPlayInfo.vidFPS;
	ptr->audFR = 31;
	ptr->audtype = 2;
	ptr->audChs = gTsPlayInfo.audChannel; // from TsReadLib_ParseADTS()
	//UINT8* dump = (UINT8*)gTsDemuxAddr;
	//(gTsReaderContainer.cbShowErrMsg)("verify %x %x %x %x %x\r\n", *(dump+0),
	//    *(dump+1), *(dump+2), *(dump+3), *(dump+4));

	//#NT#2012/04/06#Calvin Chang -begin
	// Maximum Video Frames for new H264 driver
	//for (frm = 0; frm < MEDIAREADLIB_PREDEC_FRMNUM; frm++) // Always get maximum frames pos/size on H264 and MJPG
	//{
	ptr->vidfrm_pos[0]  = gTsDemuxAddr - hdrAddr;
	ptr->vidfrm_size[0] = step;
	//}
	ptr->wid = gTsPlayInfo.vidWidth;
	ptr->hei = gTsPlayInfo.vidHeight;
	ptr->tkwid = gTsPlayInfo.vidTkWidth;
	ptr->tkhei = gTsPlayInfo.vidTkHeight;
	// no duration info, fixed to 30min
	ptr->dur = gTsPlayInfo.totalMs;
	ptr->bStillVdoPid = bStillVdoPid;
	DBG_DUMP("[TS] WxH = %dx%d, Tk WxH = %dx%d, FPS = %d, bStillVdoPid = %d\r\n", gTsPlayInfo.vidWidth, gTsPlayInfo.vidHeight, gTsPlayInfo.vidTkWidth, gTsPlayInfo.vidTkHeight, gTsPlayInfo.vidFPS, ptr->bStillVdoPid);
	return E_OK;
}

void TsReadLib_ParseEndInfo(UINT32 srcAddr, UINT32 readSize)
{
	UINT32 packet_num, pid, i, start_indicator, af_control, af_length, pts_length = 5;
	UINT64 pcrUnit;
	UINT8 *data;

	g_TsReadLib_ParseEndInfo = E_SYS;

	data = (UINT8 *)srcAddr;
	packet_num = readSize / TS_PACKET_SIZE;
	// find TS sync byte
	for (;;) {
		if (*data == 0x47 && *(data + TS_PACKET_SIZE) == 0x47 && *(data + TS_PACKET_SIZE * 2) == 0x47) {
			break;
		}
		data++;

		if(((UINT32)data - srcAddr) >= readSize){
			DBG_ERR("buffer overflow!(0x%lx)\n", readSize);
			return;
		}
	}

	for (i = 0; i < packet_num; i++) {
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);

		if (pid == gTsFilterManager.pcrFilter.pid) {
			gTsPlayInfo.pcrEndValue = TsReadLib_ParsePCR(data);
			g_TsReadLib_ParseEndInfo = E_OK;
		} else if (pid == gTsFilterManager.vidFilter.pid) {
			start_indicator = *(data + 1) & 0x40;
			af_control = (*(data + 3) & 0x30) >> 4;
			if (start_indicator) {
				if (af_control == 3) {
					af_length = *(data + 4);
					gTsPlayInfo.lastPts = TsReadLib_ParsePTS(data + 5 + af_length + (TS_VID_PESHEADER_LENGTH - pts_length));
				} else {
					gTsPlayInfo.lastPts = TsReadLib_ParsePTS(data + 4 + (TS_VID_PESHEADER_LENGTH - pts_length));
				}
			}
		}
		data = data + TS_PACKET_SIZE;
	}

	if (gTsPlayInfo.pcrEndValue == 0) {
		DBG_ERR("ERR:PCR start=%lld, end=%lld\r\n", gTsPlayInfo.pcrStartValue,
										  gTsPlayInfo.pcrEndValue);
	}

	pcrUnit = 90000 * 300; // 90000: PCR_base, 300: in units of the period of 1/300 times the system clock frequency, 2018/11/2, Adam
	gTsPlayInfo.totalMs = (gTsPlayInfo.pcrEndValue - gTsPlayInfo.pcrStartValue) * 1000 / pcrUnit;
	gTsPlayInfo.totalVidFrame = gTsPlayInfo.totalMs * gTsPlayInfo.vidFPS / 1000;
}

void TsReadLib_PrepareSeek(UINT32 vidIdx, UINT32 audIdx)
{
	gTsReadLib_Info.parsedFileSize = 0;

	gSplittedTSPacket.pid = 0;
	gSplittedTSPacket.splitted1stHalfSize = 0;

	gTsPlayInfo.audFrmIndex = audIdx;
	gTsPlayInfo.audFrameSize = 0;
	gTsPlayInfo.vidFrmIndex = vidIdx;
	gTsPlayInfo.vidFrameSize = 0;
	gTsPlayInfo.currentPesType = PESTYPE_NONE;
	gTsPlayInfo.demuxFinished = FALSE;
}

UINT32 TsReadLib_SeekTsStream(UINT32 srcAddr, UINT32 readSize, UINT64 targetPts)
{
	UINT32 packet_num, i, pid, start_indicator, af_control, af_length, pts_length = 5;
	UINT64 pts = 0;
	UINT8 *data;

	data = (UINT8 *)srcAddr;

	// find TS sync byte
	for (;;) {
		if (*data == 0x47 && *(data + TS_PACKET_SIZE) == 0x47 && *(data + TS_PACKET_SIZE * 2) == 0x47) {
			break;
		}
		data++;

		if(((UINT32)data - srcAddr) >= readSize){
			DBG_ERR("buffer overflow!(0x%lx)\n", readSize);
			return 0;
		}
	}

	packet_num = (readSize - ((UINT32)data - srcAddr)) / TS_PACKET_SIZE;

	for (i = 0; i < packet_num; i++) {
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);

		if (pid == gTsFilterManager.vidFilter.pid) {
			start_indicator = *(data + 1) & 0x40;
			af_control = (*(data + 3) & 0x30) >> 4;

			if (start_indicator) {
				if (af_control == 3) {
					af_length = *(data + 4);
					pts = TsReadLib_ParsePTS(data + 5 + af_length + (TS_VID_PESHEADER_LENGTH - pts_length));
				} else {
					pts = TsReadLib_ParsePTS(data + 4 + (TS_VID_PESHEADER_LENGTH - pts_length));
				}

				//need to search previous block
				if (pts > targetPts) {
					break;
				}

				if (pts == targetPts) {
					DBG_DUMP("got! pts = %lld\r\n", pts);
					gTsSeekInfo.targetPts = 0;
					gTsSeekInfo.diff = 0;
					gTsSeekInfo.result = SEEKRESULT_AHEAD;

					return (UINT32)data;
				}
			}
		}

		data = data + TS_PACKET_SIZE;
	}

	if ((INT64)(targetPts - pts) < 0) {
		gTsSeekInfo.diff = pts - targetPts;
		gTsSeekInfo.result = SEEKRESULT_BEHIND;
	} else {
		gTsSeekInfo.diff = targetPts - pts;
		gTsSeekInfo.result = SEEKRESULT_AHEAD;
	}

	return 0;
}

void TsReadLib_ParseThumbInfo(UINT32 srcAddr, UINT32 readSize, UINT32 dstAddr)
{
	UINT32 packet_num, i, pid, start_indicator = 0, af_control, af_length = 0;
	UINT32 header_len, buf_offset = 0, parseFinished = 0;
	UINT32 thumb_pes_shift = 6;
	UINT8 *data;

	data = (UINT8 *)srcAddr;
	packet_num = readSize / TS_PACKET_SIZE;

	// find TS sync byte
	for (;;) {
		if (*data == 0x47 && *(data + TS_PACKET_SIZE) == 0x47 && *(data + TS_PACKET_SIZE * 2) == 0x47) {
			break;
		}
		data++;

		if(((UINT32)data - srcAddr) >= readSize){
			DBG_ERR("buffer overflow!(0x%lx)\n", readSize);
			gTsPlayInfo.thumbAddr = 0;
			gTsPlayInfo.thumbSize = 0;
			return;
		}
	}

	for (i = 0; i < packet_num; i++) {
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);

		if (pid == TS_THUMBNAIL_PID) {

			start_indicator = *(data + 1) & 0x40;
			af_control = (*(data + 3) & 0x30) >> 4;

			if (start_indicator) {
				header_len = 4 + thumb_pes_shift;
				gTsPlayInfo.thumbAddr = dstAddr + buf_offset;
			} else if (af_control == 3) { // stuffing bytes
				af_length = *(data + 4);
				header_len = 5 + af_length;
				parseFinished = 1;
			} else {
				header_len = 4;
			}

			if (header_len > TS_PACKET_SIZE) {
				DBG_ERR("TsReadLib_ParseThumbInfo header_len %d > TS_PACKET_SIZE %d\r\n", header_len, TS_PACKET_SIZE);
				return;
			}

			memcpy((UINT8 *)(dstAddr + buf_offset), data + header_len, TS_PACKET_SIZE - header_len);

			gTsPlayInfo.thumbSize += (TS_PACKET_SIZE - header_len);
			buf_offset += (TS_PACKET_SIZE - header_len);

			if (parseFinished == 1) {
				break;
			}
		}

		data = data + TS_PACKET_SIZE;
	}

	//(gTsReaderContainer.cbShowErrMsg)("TS parse thumb, size=0x%x\r\n", gTsPlayInfo.thumbSize);
}

ER TsReadLib_SearchIFrame(UINT32 srcAddr, UINT32 srcSize, UINT32 dstAddr, UINT32 *dstSize, UINT32 *direction, UINT32 *keyFrameOfs)
{
	UINT32 packet_num, i, pid, start_indicator = 0, af_control, af_length = 0, tmp;
	UINT32 header_len = 0, buf_offset = 0, garbage = 0;
	UINT32 stuffing_len = 0;
	UINT8 *data;
	BOOL keyFrameFound = FALSE, parseDone = FALSE;
	UINT8 decrypt_buf[256] = {0};

	data = (UINT8 *)srcAddr;
	gTsPlayInfo.vidFrameSize = 0;

	// find TS sync byte
	for (;;) {
		if (*data == 0x47 && *(data + TS_PACKET_SIZE) == 0x47 && *(data + TS_PACKET_SIZE * 2) == 0x47) {
			break;
		}
		data++;
		garbage++;

		if(((UINT32)data - srcAddr) >= srcSize){
			DBG_ERR("buffer overflow!(0x%lx)\n", srcSize);
			return E_MACV;
		}
	}

	if(keyFrameOfs){
		*keyFrameOfs = 0;
	}

	packet_num = (srcSize - garbage) / TS_PACKET_SIZE;

	for (i = 0; i < packet_num; i++) {
		pid = (*(data + 1) & 0x1F) << 8 | *(data + 2);

		if (pid == gTsFilterManager.vidFilter.pid) {
			start_indicator = *(data + 1) & 0x40;
			af_control = (*(data + 3) & 0x30) >> 4;

			if (start_indicator) {
				//I-frame parse finished
				if (parseDone == TRUE) {
					break;
				}

				TsReadLib_ParseStuffing(data, &stuffing_len);
				DBG_IND("TsReadLib_SearchIFrame: stuffing_len=%d\r\n", stuffing_len);

				tmp = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength;
				// H.264
				if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H264 && *(data + tmp) == 0x00 && *(data + tmp + 1) == 0x00 && *(data + tmp + 2) == 0x00 &&
					*(data + tmp + 3) == 0x01 && *(data + tmp + 4) == 0x67) {
					header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength + gTsPlayInfo.h264VidDescLen;
					keyFrameFound = TRUE;

					if(keyFrameOfs)
						*keyFrameOfs = (((UINT32)data) - srcAddr);

					// decrypt data if decrypt is enable
					if (TsReadLib_IsDecrypt() && *direction == 0) {
						UINT32 ts_header_len = header_len - gTsPlayInfo.h264VidDescLen;
						UINT8 *next_packet_data = data + TS_PACKET_SIZE;
						//decrypt 16 bytes data for each decrypt process
						memcpy(decrypt_buf, data+ts_header_len+16, TS_PACKET_SIZE-ts_header_len-16);
						memcpy(decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), next_packet_data+4, DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));

						TsReadLib_Decrypt((UINT32)decrypt_buf, DECRYPT_BLK);

						memcpy(data+ts_header_len+16, decrypt_buf, TS_PACKET_SIZE-ts_header_len-16);
						memcpy(next_packet_data+4, decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));
					}

				// H.265
				} else if (gTsPlayInfo.vidCodecType == MEDIAVIDENC_H265 && *(data + tmp) == 0x00 && *(data + tmp + 1) == 0x00 && *(data + tmp + 2) == 0x00 &&
					*(data + tmp + 3) == 0x01 && *(data + tmp + 4) == 0x40) {
					header_len = 4 + (TS_VID_PESHEADER_LENGTH + stuffing_len) + gTsPlayInfo.vidNalLength + gTsPlayInfo.h264VidDescLen;
					keyFrameFound = TRUE;

					if(keyFrameOfs)
						*keyFrameOfs = (((UINT32)data) - srcAddr);

					// decrypt data if decrypt is enable
					if (TsReadLib_IsDecrypt()) {
						UINT32 ts_header_len = header_len - gTsPlayInfo.h264VidDescLen;
						UINT8 *next_packet_data = data + TS_PACKET_SIZE;
						//decrypt 16 bytes data for each decrypt process
						memcpy(decrypt_buf, data+ts_header_len+16, TS_PACKET_SIZE-ts_header_len-16);
						memcpy(decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), next_packet_data+4, DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));

						TsReadLib_Decrypt((UINT32)decrypt_buf, DECRYPT_BLK);

						memcpy(data+ts_header_len+16, decrypt_buf, TS_PACKET_SIZE-ts_header_len-16);
						memcpy(next_packet_data+4, decrypt_buf+(TS_PACKET_SIZE-ts_header_len-16), DECRYPT_BLK - (TS_PACKET_SIZE-ts_header_len-16));
					}
				}
			} else if (af_control == 3 && keyFrameFound == TRUE) { // stuffing bytes
				af_length = *(data + 4);
				header_len = 5 + af_length;
				parseDone = TRUE;
			} else if (keyFrameFound == TRUE) {
				//I-frame parse finished
				if (parseDone == TRUE) {
					break;
				} else {
					header_len = 4;
				}
			}

			if (keyFrameFound == TRUE) {
				if (header_len > TS_PACKET_SIZE) {
					DBG_ERR("TsReadLib_SearchIFrame header_len %d > TS_PACKET_SIZE %d\r\n", header_len, TS_PACKET_SIZE);
					return E_SYS;
				}

				memcpy((UINT8 *)(dstAddr + buf_offset), data + header_len,
					   TS_PACKET_SIZE - header_len);

				gTsPlayInfo.vidFrameSize += (TS_PACKET_SIZE - header_len);
				buf_offset += (TS_PACKET_SIZE - header_len);
			}
		}
		data = data + TS_PACKET_SIZE;
	}

	if (parseDone) {
		*direction = 0;
		*dstSize = buf_offset;
	} else if (keyFrameFound) {
		//increase offset
		*direction = 1;
		*dstSize = 0;
	} else {
		//decrease offset
		*direction = 2;
		*dstSize = 0;
	}

	return E_OK;
}

UINT32 TsReadLib_GetNextIFrame(UINT32 uiFrmIdx, UINT32 uiSkipI)
{
	UINT32 curFrmIndex = uiFrmIdx;
#if _USER_ALLOC_ENTRY_BUF_
	TS_FRAME_ENTRY *pVidEntry;

	if (gpTsVideoEntry == NULL) {
		DBG_ERR("pEntry is null\r\n");
		return 0;
	}
#endif

	curFrmIndex += 1;

#if _USER_ALLOC_ENTRY_BUF_
	pVidEntry = gpTsVideoEntry + (curFrmIndex % TSVID_ENTRY_SIZE);
	while (pVidEntry->memAddr != 0) {
		if (pVidEntry->key_frame == 1) {
			return curFrmIndex;
		}
		curFrmIndex++;
		pVidEntry = gpTsVideoEntry + (curFrmIndex % TSVID_ENTRY_SIZE);
	}
#else
	while (gTsVideoEntry[curFrmIndex % TSVID_ENTRY_SIZE].memAddr != 0) {
		if (gTsVideoEntry[curFrmIndex % TSVID_ENTRY_SIZE].key_frame == 1) {
			return curFrmIndex;
		}
		curFrmIndex++;
	}
#endif
	//(gTsReaderContainer.cbShowErrMsg)("I frame not found, curFrmIndex=%d\r\n", curFrmIndex);

	return curFrmIndex;
}

INT32 TsReadLib_ReadBit()
{
    INT32 nIndex = 0, nOffset = 0, value = 0;
    nIndex = gTsNALParserCurrentBit / 8;
	nOffset = 7 - (gTsNALParserCurrentBit % 8);
	value = 0x01 & (gpTsNALParserStart[nIndex] >> nOffset);

	gTsNALParserCurrentBit += 1;

    return value;
}

INT32 TsReadLib_ReadBits(INT32 num)
{
	INT32 i = 0, bitVal = 0, value = 0;

	while(i < num) {
		bitVal = TsReadLib_ReadBit();
		value = value | (bitVal << (num - i - 1));
		i++;
	}
	return value;
}

void TsReadLib_SkipBit(UINT32 num)
{
	gTsNALParserCurrentBit += num;
}
INT32 TsReadLib_ReadExpGolCode()
{
	INT32 i = 0, bitsVal = 0;

	for (i = 0; i < 32;) {
		if (TsReadLib_ReadBit() != 0) {
			break;
		} else {
		i++;
		}
	}

	bitsVal = TsReadLib_ReadBits(i);
	bitsVal = bitsVal + ((UINT64)1 << i) - 1;

	return bitsVal;
}

void TsReadLib_SkipExpGolCode()
{
	INT32 i = 0;
	for (i = 0; i < 32;) {
		if (TsReadLib_ReadBit() != 0) {
			break;
		} else {
			i++;
		}
	}
	TsReadLib_SkipBit(i);
}
INT32 TsReadLib_ReadSE()
{
	INT32 res = 0;
	res = TsReadLib_ReadExpGolCode();
	if ((res & 0x01) != 0) {
		res = (res + 1) >> 1;
	} else {
		res = -(res >> 1);
	}
	return res;
}

void TsReadLib_SkipSE()
{
	TsReadLib_SkipExpGolCode();
}
void TsReadLib_ParseH264(const unsigned char *pStart, unsigned short nLen)
{
	INT32 frm_left_off=0, frm_right_off=0, frm_top_off=0, frm_bott_off=0;
	INT32 aspect_info_flg=0, aspect_idc=0;
	INT32 sar_wid=0, sar_hei=0;
	INT32 prof_idc=0, vui_flg=0;
	INT32 picture_order_count_type=0;
	INT32 picture_wid_in_mbs_minus1=0, picture_hei_in_map_units_minus1=0;
	INT32 frm_mbs_only_flg=0;
	gpTsNALParserStart = pStart;
	gTsNALParserLength = nLen;
	gTsNALParserCurrentBit = 0;


    prof_idc = TsReadLib_ReadBits(8);
// coverity[check_return]
   	TsReadLib_SkipBit(6); // constraint_set0~set5 flag
// coverity[check_return]
    TsReadLib_SkipBit(2); // reserved zero 2bits
// coverity[check_return]
    TsReadLib_SkipBit(8); // level idc
    TsReadLib_SkipExpGolCode(); // sequence parameter set id


    if(prof_idc == 44  || prof_idc == 83  || prof_idc == 86  || prof_idc == 118 ||
	   prof_idc == 100 || prof_idc == 110 || prof_idc == 122 || prof_idc == 244)
    {
        if(TsReadLib_ReadExpGolCode() == 3)
        {
            TsReadLib_SkipBit(1); // residual colour transform flag
		}
        TsReadLib_SkipExpGolCode(); // bit depth luma minus8
        TsReadLib_SkipExpGolCode(); // bit depth chroma minus8
        TsReadLib_SkipBit(1); // qpprime y zero transform bypass flag

        if (TsReadLib_ReadBit() & 1) // seq scaling matrix
        {
            INT32 i = 0, sizeOfScalingList = 0, lastScale = 0, nextScale = 0;
            while (i < 8) {
                if (TsReadLib_ReadBit() & 1) { // seq scaling list
                    if (i < 6) {
						sizeOfScalingList = 16;
                    } else {
                    	sizeOfScalingList = 64;
                    }
                    lastScale =
                    nextScale = 8;

                    INT32 j = 0, delta_scale = 0;
                    while (j < sizeOfScalingList)
                    {
                        if (nextScale) {
                            delta_scale = TsReadLib_ReadSE();
							nextScale = (lastScale + delta_scale + 256) % 256;
						}
                        if (nextScale) {
							lastScale = nextScale;
					}
						j++;
				}
			}
				i++;
		}
	}
    }
    TsReadLib_SkipExpGolCode(); // log2 max frame num minus4
    picture_order_count_type = TsReadLib_ReadExpGolCode();
    if(!picture_order_count_type)
    {
        TsReadLib_SkipExpGolCode(); // log2 max pic order cnt lsb minus4
    }
    else if(picture_order_count_type & 1)
    {
        TsReadLib_SkipBit(1); // delta pic order always zero flag
        TsReadLib_SkipSE(); // offset for non ref pic
        TsReadLib_SkipSE(); // offset for top to bottom field

        INT32 i = 0;
        while (i < TsReadLib_ReadExpGolCode()) // num ref frames in pic order cnt cycle
        {
            TsReadLib_SkipSE();
            i++;
		}
	}
    TsReadLib_SkipExpGolCode(); // max num ref frames
    TsReadLib_SkipBit(1); // gaps in frame num value allowed flag
    picture_wid_in_mbs_minus1 = TsReadLib_ReadExpGolCode();
    picture_hei_in_map_units_minus1 = TsReadLib_ReadExpGolCode();
    frm_mbs_only_flg = TsReadLib_ReadBit();
    if(!frm_mbs_only_flg)
    {
        TsReadLib_SkipBit(1); // mb adaptive frame field flag
	}
    TsReadLib_SkipBit(1); // direct 8x8 inference flag
    if(TsReadLib_ReadBit()) // frame cropping flag
    {
        frm_left_off = TsReadLib_ReadExpGolCode();
        frm_right_off = TsReadLib_ReadExpGolCode();
        frm_top_off = TsReadLib_ReadExpGolCode();
        frm_bott_off = TsReadLib_ReadExpGolCode();
	}
    gTsPlayInfo.vidWidth = (picture_wid_in_mbs_minus1 + 1) * 16 - (frm_right_off * 2 + frm_left_off * 2);
    gTsPlayInfo.vidHeight = ((2 - frm_mbs_only_flg) * (picture_hei_in_map_units_minus1 + 1) * 16) - (frm_top_off * 2 + frm_bott_off * 2);
	gTsPlayInfo.vidTkWidth = gTsPlayInfo.vidWidth;
	gTsPlayInfo.vidTkHeight = gTsPlayInfo.vidHeight;

	//Get SAR (Sample aspect ratio)
    vui_flg = TsReadLib_ReadBit();
    if (vui_flg)
    {
        aspect_info_flg = TsReadLib_ReadBit();
        if (aspect_info_flg)
        {
            aspect_idc = TsReadLib_ReadBits(8);
            if (aspect_idc == 255) // Extended_SAR
            {
                sar_wid = TsReadLib_ReadBits(16);
                sar_hei = TsReadLib_ReadBits(16);
                if (sar_wid > 0 && sar_hei > 0 && gTsPlayInfo.vidWidth > 0)
                {
                    gTsPlayInfo.vidTkWidth = (gTsPlayInfo.vidWidth*sar_wid)/sar_hei;
				}
			}
		}
	}
	//(gTsReaderContainer.cbShowErrMsg)("[TS] WxH = %dx%d, Tk WxH = %dx%d\r\n", gTsPlayInfo.vidWidth, gTsPlayInfo.vidHeight, gTsPlayInfo.vidTkWidth, gTsPlayInfo.vidTkHeight);
}

UINT32 TsReadLib_ebspTorbsp(UINT8 *pucAddr, UINT8 *pucBuf, UINT32 uiTotalBytes)
{
    UINT32 uiCount, i, j;

    for(i = 0, j = 0, uiCount = 0; i < uiTotalBytes; i++)
    {
        // starting from begin_bytepos to avoid header information
        if(uiCount == 2 && pucAddr[i] == 0x03)
        {
            i++;
            uiCount = 0;
        }
        pucBuf[j] = pucAddr[i];

        uiCount = (pucAddr[i]) ? 0 : uiCount + 1;
        j++;
    }

    return j;
}

void TsReadLib_PTL(UINT32 sps_max_sub_layers_minus1)
{
	UINT32 i = 0;
	UINT32 sub_layer_profile_present_flag[7] = {0};
	UINT32 sub_layer_level_present_flag[7] = {0};

	TsReadLib_SkipBit(2);
	TsReadLib_SkipBit(1);
	TsReadLib_SkipBit(5);

	for (i = 0; i < 32; i++) {
		TsReadLib_SkipBit(1);
	}

	TsReadLib_SkipBit(1);
	TsReadLib_SkipBit(1);
	TsReadLib_SkipBit(1);
	TsReadLib_SkipBit(1);
	TsReadLib_SkipBit(44);
	TsReadLib_SkipBit(8);

	for (i = 0; i < sps_max_sub_layers_minus1; i++) {
		sub_layer_profile_present_flag[i] = TsReadLib_ReadBits(1);
		sub_layer_level_present_flag[i] = TsReadLib_ReadBits(1);
	}

	if (sps_max_sub_layers_minus1 > 0)
		for (i = sps_max_sub_layers_minus1; i < 8; i++)
			TsReadLib_SkipBit(2);

	for (i = 0; i < sps_max_sub_layers_minus1; i++) {
		if (sub_layer_profile_present_flag[i]) {
			TsReadLib_SkipBit(2);
			TsReadLib_SkipBit(1);
			TsReadLib_SkipBit(5);
			for (i = 0; i < 32; i++) {
				TsReadLib_SkipBit(1);
			}
			TsReadLib_SkipBit(1);
			TsReadLib_SkipBit(1);
			TsReadLib_SkipBit(1);
			TsReadLib_SkipBit(1);
			TsReadLib_SkipBit(44);
		}

		if (sub_layer_level_present_flag[i]) {
			TsReadLib_SkipBit(8);
		}
	}
}

void TsReadLib_ParseH265(const unsigned char *pStart, unsigned short nLen)
{
	UINT32 i = 0;
	UINT32 uiVal = 0, uiHeaderSize = 0;
	UINT32 sps_max_sub_layers_minus1 = 0;
	UINT32 pic_width_in_luma_samples = 0;
	UINT32 pic_height_in_luma_samples = 0;
	UINT8 desc_buf[512] = {0};

	uiHeaderSize = TsReadLib_ebspTorbsp((UINT8 *)pStart, desc_buf, (UINT32)nLen);

	gpTsNALParserStart = (const unsigned char *)desc_buf;
	gTsNALParserLength = uiHeaderSize;
	gTsNALParserCurrentBit = 0;

	//skip VPS
	UINT8 *bsptr = (UINT8 *)gpTsNALParserStart;
	for (i = 0; i < nLen; i++) {
		//DBG_DUMP("%02X %02X %02X %02X %02X\r\n", *(bsptr+i), *(bsptr+i+1), *(bsptr+i+2), *(bsptr+i+3), *(bsptr+i+4));
		if (*(bsptr+i) == 0x0 && *(bsptr+i+1) == 0x0 && *(bsptr+i+2) == 0x0 && *(bsptr+i+3) == 0x1 && *(bsptr+i+4) == 0x42) {
			gpTsNALParserStart += i;
			break;
		}
	}

	// NAL
	uiVal = TsReadLib_ReadBits(24); //start code
	if (uiVal == 0x000000) {
		uiVal = TsReadLib_ReadBits(8); //start code
	}
	if (uiVal != 1) {  //if ( uiVal != 0x00000001 || uiVal != 0x000001){
		DBG_ERR("[H265DecNal]StartCodec Suffix != 0x000001 (0x%08x)\r\n", uiVal);
	}
	TsReadLib_SkipBit(1);//skip_bits //forbidden_zero_bit
	TsReadLib_SkipBit(6);
	TsReadLib_SkipBit(6);
	TsReadLib_SkipBit(3);

	//SPS
	TsReadLib_SkipBit(4);
	uiVal = TsReadLib_ReadBits(3);
	sps_max_sub_layers_minus1 = uiVal;
	TsReadLib_SkipBit(1);

	TsReadLib_PTL(sps_max_sub_layers_minus1);

	uiVal = TsReadLib_ReadExpGolCode();
	uiVal = TsReadLib_ReadExpGolCode();
	if (uiVal == 3) {
		TsReadLib_SkipBit(1);
	}

	pic_width_in_luma_samples = TsReadLib_ReadExpGolCode();
	pic_height_in_luma_samples = TsReadLib_ReadExpGolCode();

	gTsPlayInfo.vidWidth = pic_width_in_luma_samples;
	gTsPlayInfo.vidHeight = pic_height_in_luma_samples;
	gTsPlayInfo.vidTkWidth = pic_width_in_luma_samples;
	gTsPlayInfo.vidTkHeight = pic_height_in_luma_samples;
}


