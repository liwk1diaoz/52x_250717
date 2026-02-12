#if defined (__UITRON) || defined (__ECOS)
#include <string.h>

#include "kernel.h"
#include "FileSysTsk.h"

#include "MOVReadTag.h"
#include "MOVLib.h"
#include "movPlay.h"
#include "NvtVerInfo.h"
NVTVER_VERSION_ENTRY(MP_MovReadLib, 1, 00, 009, 00)//1.00.016

#define __MODULE__          MOVP
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"

#else
#include <string.h>
#include "FileSysTsk.h"
#include "kwrap/error_no.h"

#include "MOVReadTag.h"
#include "avfile/MOVLib.h"

// INCLUDE PROTECTED
#include "movPlay.h"

#include <kwrap/verinfo.h>

VOS_MODULE_VERSION(MP_MovReadLib, 1, 01, 001, 00);

#define __MODULE__          MOVP
#define __DBGLVL__          2
#include "kwrap/debug.h"
#include "kwrap/error_no.h"

//#define __MODULE__          MOVR
//#define __DBGLVL__          2
unsigned int MOVParse_debug_level = NVT_DBG_WRN;
#endif

typedef struct _MOVREADLIB_INFO {
	UINT32 bufaddr;
	UINT32 bufsize;
	UINT64 moovpos;
	UINT32 moovsize;
	UINT64 filesize;

} MOVREADLIB_INFO;
MOVREADLIB_INFO gMovReadLib_Info;
//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
UINT64          gMovReadLib_ThumbPos = 0;
UINT32          gMovReadLib_ThumbSize = 0;
//#NT#2012/08/21#Calvin Chang -end
//#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
UINT64          gMovReadLib_ScraPos = 0;
UINT32          gMovReadLib_ScraSize = 0;
//#NT#2013/12/17#Calvin Chang -end
extern UINT16  gMovWidth, gMovHeight;
//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
extern UINT16  gMovTkWidth, gMovTkHeight;
//#NT#2013/10/18#Calvin Chang -end

ER MovReadLib_Probe(UINT32 addr, UINT32 readsize);
ER MovReadLib_Initialize(void);
ER MovReadLib_SetMemBuf(UINT32 startAddr, UINT32 size);
ER MovReadLib_ParseHeader(UINT32 hdrAddr, UINT32 hdrSize, void *pobj);
ER MovReadLib_GetInfo(UINT32 type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
ER MovReadLib_SetInfo(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3);
ER MovReadLib_Parse1stVideo(UINT32 hdrAddr, UINT32 hdrSize, void *pobj);

CONTAINERPARSER gMovReadLibObj = {MovReadLib_Probe,
								  MovReadLib_Initialize,
								  MovReadLib_SetMemBuf,
								  MovReadLib_ParseHeader,
								  MovReadLib_GetInfo,
								  MovReadLib_SetInfo,
								  NULL,//custom
								  NULL,//read
								  NULL,//showmsg
								  MOVREADLIB_CHECKID  //check ID
								 };

CONTAINERPARSER *MP_MovReadLib_GerFormatParser(void)
{
	return &gMovReadLibObj;
}

ER MovReadLib_Probe(UINT32 addr, UINT32 readsize)
{
	UINT32 *ptr;
	ptr = (UINT32 *)addr;
	ptr++;

	if ((*ptr == MOV_Probe_mdat) || (*ptr == MOV_Probe_pnot)
		|| (*ptr == MOV_Probe_wide) || (*ptr == MOV_Probe_ftyp)) {
		//if (gMovReaderContainer.cbShowErrMsg)
		//    (gMovReaderContainer.cbShowErrMsg)("MOV yes\r\n");
		return E_OK;
	}
	/*if (gMovReaderContainer.cbShowErrMsg) {
		(gMovReaderContainer.cbShowErrMsg)("not MOV\r\n");
	}*/
	return E_NOSPT;
}
ER MovReadLib_Initialize(void)
{
	gMovReadLib_Info.bufaddr = 0;
	gMovReadLib_Info.bufsize = 0;
	gMovReadLib_Info.filesize = 0;
	gMovStscFix = 0;
	gMov264VidDescLen = 0;
	memset(gMov264VidDesc, 0, sizeof(char) * 0x40);
	return E_OK;
}
ER MovReadLib_SetMemBuf(UINT32 startAddr, UINT32 size)
{
	gMovReadLib_Info.bufaddr = startAddr;
	gMovReadLib_Info.bufsize = size;
	return E_OK;
}
ER MovReadLib_ParseHeader(UINT32 hdrAddr, UINT32 hdrSize, void *pobj)
{
	UINT32 *p32Data;
	UINT64 tagSize = 0, filepos = 0;
	UINT8 finish = 0;
	ER ret;

	if (gMovReaderContainer.cbReadFile == NULL) {
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("ERROR!! cbReadFile=NULL!!!\r\n");
		}
		return E_NOSPT;
	}

	if (gMovReadLib_Info.bufaddr == 0) {
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("ERROR!! bufaddr=NULL!!!\r\n");
		}
		return E_PAR;
	}
	if (gMovReadLib_Info.filesize == 0) {
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("Need filesize, call MovReadLib_SetInfo() firstly!\r\n");
		}
		return E_NOSPT;
	}
	//get tag
	filepos = 0;
	while (!finish) {
		ret = (gMovReaderContainer.cbReadFile)(0, filepos, 0x1000, gMovReadLib_Info.bufaddr);
		if (ret != E_OK) {
			(gMovReaderContainer.cbShowErrMsg)("1 header cb read error! %d\r\n", ret);
			break;
		}

		p32Data = (UINT32 *)gMovReadLib_Info.bufaddr;
		tagSize = MovReadLib_Swap(*p32Data);
		if (tagSize == 0) { //something wrong
			return E_NOSPT;
		}
		p32Data++;

		//co64
		if (*p32Data == MOV32_MDAT && *(p32Data - 1) == 0x01000000) {
			tagSize = (UINT64)MovReadLib_Swap(*(p32Data + 1)) << 32;
			tagSize += MovReadLib_Swap(*(p32Data + 2));
		}

		//if (gMovReaderContainer.cbShowErrMsg)
		//{
		//    (gMovReaderContainer.cbShowErrMsg)("TAG:pos =0x%lx, size=0x%lx, name= 0x%lx!!!\r\n", filepos, tagSize, *p32Data);
		//}
		if (*p32Data == MOV32_MOOV) {
			gMovReadLib_Info.moovpos = filepos;
			gMovReadLib_Info.moovsize = tagSize;
			if (tagSize > (gMovReadLib_Info.bufsize - 0x100000)) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("needBufSize 0x%lx to parse Header!!!\r\n", tagSize + 0x100000);
				}
				break;
			}
			//read whole moov tag
			ret = (gMovReaderContainer.cbReadFile)(0, filepos, tagSize, gMovReadLib_Info.bufaddr);
			if (ret != E_OK) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("2 header cb read error! %d\r\n", ret);
				}
				break;
			}

			MOVRead_ParseHeader(gMovReadLib_Info.bufaddr, pobj);
			break;
		} else { //not moov tag
			filepos += tagSize;
			if (filepos > gMovReadLib_Info.filesize) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("MOOV not found!\r\n");
				}
				finish = 1;
			}
		}
	}
	return E_OK;
}
ER MovReadLib_GetInfo(UINT32 type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
{
	ER resultV = E_PAR;
	UINT32 now, temp;

	switch (type) {
	case MEDIAREADLIB_GETINFO_VIDEOFR:
		if (pparam1 == 0) {
			resultV = E_PAR;
		} else {
			MOV_Read_GetParam(MOVPARAM_VIDEOFR, &temp);
			*pparam1 = temp;
		}
		break;
	case MOVREADLIB_GETINFO_MAXIFRAMESIZE:
		if (pparam1 == 0) {
			resultV = E_PAR;
		} else {
			*pparam1 = g_movReadMaxIFrameSize;
			//p1outsize = 4;
			//if (gMovReaderContainer.cbShowErrMsg)
			//{
			//    (gMovReaderContainer.cbShowErrMsg)("maxsize=0x%lX!\r\n", g_movReadMaxIFrameSize);
			//}
			resultV = E_OK;
		}
		break;
	case MOVREADLIB_GETINFO_IFRAMETOTALCOUNT:
		if (pparam1 == 0) {
			resultV = E_PAR;
		} else {
			*pparam1 = g_movReadIFrameCnt;
			//p1outsize = 4;
			resultV = E_OK;
		}
		break;
	case MOVREADLIB_GETINFO_SECONDS2FILEPOS:
		if (pparam2 == 0) {
			resultV = E_PAR;
		} else {
			*pparam2 = MOVRead_GetSecond2FilePos(*pparam1);
			//p2outsize = 4;
			resultV = E_OK;
		}
		break;
	case MOVREADLIB_GETINFO_SHOWALLSIZE:
		MOVRead_ShowAllSize();
		resultV = E_OK;
		break;
	case MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE:
		if ((pparam2 == 0) || (pparam3 == 0)) {
			resultV = E_PAR;
		} else {

			MOVRead_GetVideoPosAndOffset(*pparam1, (UINT64 *)pparam2, pparam3);
			//p2outsize = 4;
			//p3outsize = 4;
			resultV = E_OK;
		}
		break;
	case MEDIAREADLIB_GETINFO_GETAUDIOPOSSIZE:
		if ((pparam2 == 0) || (pparam3 == 0)) {
			resultV = E_PAR;
		} else {
			MOVRead_GetAudioPosAndOffset(*pparam1, (UINT64 *)pparam2, pparam3);
			//p2outsize = 4;
			//p3outsize = 4;
			resultV = E_OK;
		}
		break;
	case MOVREADLIB_GETINFO_GETVIDEOENTRY:
		if (pparam2 == 0) {
			resultV = E_PAR;
		} else {
			MOVRead_GetVideoEntry(*pparam1, (MOV_Ientry *)pparam2);
			//p2outsize = sizeof(MOV_Ientry);
			resultV = E_OK;
		}
		break;
	case MOVREADLIB_GETINFO_GETAUDIOENTRY:
		if (pparam2 == 0) {
			resultV = E_PAR;
		} else {
			MOVRead_GetAudioEntry(*pparam1, (MOV_Ientry *)pparam2);
			//p2outsize = sizeof(MOV_Ientry);
			resultV = E_OK;
		}
		break;
	case MEDIAREADLIB_GETINFO_VFRAMENUM:
		//p1outsize = 4;
		*pparam1 = g_MovTagReadTrack[0].frameNumber;
		resultV = E_OK;
		break;
	case MEDIAREADLIB_GETINFO_VIDEOTYPE:
		//p1outsize = 4;
		*pparam1 = g_MovTagReadTrack[0].type;
		resultV = E_OK;
		break;

	case MEDIAREADLIB_GETINFO_VIDEO_W_H:
		//p1outsize = 4;
		//p2outsize = 4;
		*pparam1 = gMovWidth;
		*pparam2 = gMovHeight;
		resultV = E_OK;
		break;
	//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
	case MEDIAREADLIB_GETINFO_VIDEO_TK_W_H:
		//p1outsize = 4;
		//p2outsize = 4;
		*pparam1 = gMovTkWidth;
		*pparam2 = gMovTkHeight;
		resultV = E_OK;
		break;
	//#NT#2013/10/18#Calvin Chang -end
	case MEDIAREADLIB_GETINFO_H264DESC:
		//p1outsize = 4;
		//p2outsize = 4;
		*pparam1 = (UINT32)gMov264VidDesc;
		*pparam2 = gMov264VidDescLen;
		resultV = E_OK;
		break;
	case MEDIAREADLIB_GETINFO_NEXTIFRAMENUM:
		//p2outsize = 4;
		now = *pparam1;
#if 0
		*pparam2 = MOVRead_SearchNextIFrame(now);
#else // Hideo test for skip some I
		*pparam2 = MOVRead_SearchNextIFrame(now, *pparam3);
#endif
		resultV = E_OK;
		break;
	case MEDIAREADLIB_GETINFO_PREVIFRAMENUM:
		//p2outsize = 4;
		now = *pparam1;
#if 0
		*pparam2 = MOVRead_SearchPrevIFrame(now);
#else // Hideo test for skip some I
		*pparam2 = MOVRead_SearchPrevIFrame(now, *pparam3);
#endif
		resultV = E_OK;
		break;
	case MOVREADLIB_GETINFO_NEXTPFRAME:
		//p2outsize = 4;
		now = *pparam1;
		*pparam2 = MOVRead_SearchNextPFrame(now);
		resultV = E_OK;
		break;
	case MOVREADLIB_GETINFO_H264_IPB:
		//p1outsize = 4;
		*pparam1 = MOVRead_GetH264IPB();
		resultV = E_OK;
		break;
	case MOVREADLIB_GETINFO_TOTALDURATION:
		if (pparam1 == 0) {
			resultV = E_PAR;
		} else {
			//p1outsize = 4;
			*pparam1 = MOVRead_GetTotalDuration();
			resultV = E_OK;
		}
		break;
	case MOVREADLIB_GETINFO_VID_ENTRY_ADDR:
		//p1outsize = 4;
		*pparam1 = MOVRead_GetVidEntryAddr();
		resultV = E_OK;
		break;
	case MOVREADLIB_GETINFO_AUD_ENTRY_ADDR:
		//p1outsize = 4;
		*pparam1 = MOVRead_GetAudEntryAddr();
		resultV = E_OK;
		break;
	//#NT#2009/11/13#Meg Lin#[0007720] -begin
	//#NT#add for user data
	case MEDIAREADLIB_GETINFO_USERDATA:
		//p1outsize = 4;
		//p2outsize = 4;
		MOVRead_GetUdtaPosSize((UINT64 *)pparam1, pparam2);
		resultV = E_OK;
		break;
	//#NT#2009/11/13#Meg Lin#[0007720] -end
	//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
	case MEDIAREADLIB_GETINFO_VIDEOTHUMB:
		if ((pparam1 == 0) || (pparam2 == 0)) {
			resultV = E_PAR;
		} else {
			if (gMovReadLib_ThumbSize) {
				*(UINT64 *)pparam1 = gMovReadLib_ThumbPos;
				*pparam2 = gMovReadLib_ThumbSize;
			} else {
				*pparam1 = 0;
				*pparam2 = 0;
			}
			resultV = E_OK;
		}
		break;
	//#NT#2012/08/21#Calvin Chang -end
	//#NT#2012/11/21#Meg Lin -begin
	case MEDIAREADLIB_GETINFO_AUDIOFR:
		if (pparam1 == 0) {
			resultV = E_PAR;
		} else {
			MOV_Read_GetParam(MOVPARAM_AUDIOFR, &temp);
			*pparam1 = temp;
		}
		break;
	//#NT#2012/11/21#Meg Lin -end

	//#NT#2013/06/11#Calvin Chang#Support Create/Modified Data inMov/Mp4 File format -begin
	case MOVREADLIB_GETINFO_CMDATE:
		if ((pparam1 == 0) || (pparam2 == 0)) {
			resultV = E_PAR;
		} else {
			MOV_Read_GetParam(MOVPARAM_CTIME, pparam1);
			MOV_Read_GetParam(MOVPARAM_MTIME, pparam2);
		}

		break;
	//#NT#2013/06/11#Calvin Chang -end
	//#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
	case MEDIAREADLIB_GETINFO_VIDEOSCRA:
		if ((pparam1 == 0) || (pparam2 == 0)) {
			resultV = E_PAR;
		} else {
			if (gMovReadLib_ScraSize) {
				*(UINT64 *)pparam1 = gMovReadLib_ScraPos;
				*pparam2 = gMovReadLib_ScraSize;
			} else {
				*pparam1 = 0;
				*pparam2 = 0;
			}
			resultV = E_OK;
		}
		break;
	//#NT#2013/12/17#Calvin Chang -end

	default:
		resultV = E_PAR;
		break;
	}
	//if (gMovReaderContainer.cbShowErrMsg)
	//{
	//    (gMovReaderContainer.cbShowErrMsg)("p1size=%d, p2size=%d, p3size=%d!\r\n", p1outsize, p2outsize, p3outsize);
	//}
	return resultV;
}
ER MovReadLib_SetInfo(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	switch (type) {
	case MEDIAREADLIB_SETINFO_FILESIZE:
		gMovReadLib_Info.filesize = ((UINT64)param1 << 32) | param2;
		break;

	//#NT#2012/11/21#Meg Lin -begin
	case MEDIAREADLIB_SETINFO_VIDENTRYBUF:
		MOV_Read_SetVidEntryAddr(param1, param2);
		break;

	case MEDIAREADLIB_SETINFO_AUDENTRYBUF:
		MOV_Read_SetAudEntryAddr(param1, param2);
		break;
	//#NT#2012/11/21#Meg Lin -end

	default:
		break;
	}
	return E_OK;
}



ER MovReadLib_Parse1stVideo(UINT32 hdrAddr, UINT32 hdrSize, void *pobj)
{
	UINT32 *p32Data;
	UINT64 filepos = 0, tagSize = 0;
	UINT64 pos;
	UINT32 size, tempDur, tempV, tempFrmNum, tempSR, tempChs, tmpPos = 0;
	UINT8 finish = 0;
	//#NT#2012/04/06#Calvin Chang -begin
	//Search Index table solution in Parse 1st video
	UINT32 frm = 0;
	//#NT#2012/04/06#Calvin Chang -end
	MEDIA_FIRST_INFO *ptr;
	ER ret;

	ptr = (MEDIA_FIRST_INFO *)pobj;
	ptr->pos = 0;
	ptr->size = 0;
	//#NT#2010/01/29#Meg Lin# -begin
	ptr->fre1_pos = 0;
	ptr->fre1_size = 0;
	//#NT#2010/01/29#Meg Lin# -end
	size = 0;
	if (gMovReaderContainer.cbReadFile == NULL) {
		DBG_ERR("ERROR!! cbReadFile=NULL!!!\r\n");
		return E_NOSPT;
	}

	//get tag
	filepos = 0;
	gMovReadLib_ThumbSize = 0;
	while (!finish) {
		if ((gMovReadLib_Info.filesize - filepos) > 0x1000) {
            ret = (gMovReaderContainer.cbReadFile)(0, filepos, 0x1000, hdrAddr);
			if (ret != E_OK) {
				DBG_ERR("1 1stVideo cb read error! %d\r\n", ret);
				return E_SYS;
			}
		}
        else {
            ret = (gMovReaderContainer.cbReadFile)(0, filepos, (gMovReadLib_Info.filesize - filepos), hdrAddr);
			if (ret != E_OK) {
				DBG_ERR("2 1stVideo cb read error! %d\r\n", ret);
				return E_SYS;
			}
		}

		p32Data = (UINT32 *)hdrAddr;
		tagSize = MovReadLib_Swap(*p32Data);
		if (tagSize == 0) { //something wrong
			return E_NOSPT;
		}
		p32Data++;

		//co64
		if (*p32Data == MOV32_MDAT && *(p32Data - 1) == 0x01000000) {
			tagSize = (UINT64)MovReadLib_Swap(*(p32Data + 1)) << 32;
			tagSize += MovReadLib_Swap(*(p32Data + 2));
		}

		//if (gMovReaderContainer.cbShowErrMsg)
		//{
		//    (gMovReaderContainer.cbShowErrMsg)("TAG222:pos =0x%lx, size=0x%lx, name= 0x%lx!!!\r\n", filepos, tagSize, *p32Data);
		//}
		if (*p32Data == MOV32_MOOV) {
			gMovReadLib_Info.moovpos = filepos;
			gMovReadLib_Info.moovsize = tagSize;
			if (tagSize > hdrSize) {
				DBG_ERR("222needBufSize 0x%lx to parse Header!!!\r\n", tagSize);
				//tagSize = hdrSize;
				//#NT#2009/12/16#Meg Lin# -begin
				//#NT#new: parse media info
				ptr->dur = 0;
				return E_SYS;
				//#NT#2009/12/16#Meg Lin# -end
			}
			//read whole moov tag
			ret = (gMovReaderContainer.cbReadFile)(0, filepos, tagSize, hdrAddr);
			if (ret != E_OK) {
				DBG_ERR("3 1stVideo cb read error! %d\r\n", ret);
				return E_SYS;
			}

			MOVRead_ParseHeaderFirstVideo(hdrAddr, pobj);
			//#NT#2009/11/13#Meg Lin#[0007720] -begin
			//#NT#add for user data
			MovReadLib_GetInfo(MEDIAREADLIB_GETINFO_USERDATA, &tmpPos, &size, 0);
			pos = tmpPos;
			//#NT#2009/11/13#Meg Lin#[0007720] -end
			//MOV_Read_GetParam(MOVPARAM_MVHDDURATION, &tempDur);
			MOV_Read_GetParam(MOVPARAM_VIDTKHDDURATION, &tempDur);//video tkhd duration //2013/01/29 Calvin
			MOV_Read_GetParam(MOVPARAM_VIDEOTIMESCALE, &tempV);
			if (tempV > 1000) {
				UINT64 tempDur64=0;
				tempDur64 = (UINT64)tempDur*1000;
				ptr->dur = tempDur64 / tempV;
			} else {
				ptr->dur = tempDur / 30;    // Time scale should be 30000
			}
			MOV_Read_GetParam(MOVPARAM_AUDFRAMENUM, &tempFrmNum);
			MOV_Read_GetParam(MOVPARAM_SAMPLERATE, &tempSR);
			MOV_Read_GetParam(MOVPARAM_AUDCHANNELS, &tempChs);
			ptr->audFrmNum = tempFrmNum;
			ptr->audSR = tempSR;
			ptr->audChs = tempChs;
			MOV_Read_GetParam(MOVPARAM_VIDEOTYPE, &tempV);
			ptr->vidtype = tempV;
			MOV_Read_GetParam(MOVPARAM_VIDEOFR, &tempV);
			ptr->vidFR = tempV;
			MOV_Read_GetParam(MOVPARAM_AUDIOFR, &tempV);//2012/11/21 Meg
			ptr->audFR = tempV; //2012/11/21 Meg
			MOV_Read_GetParam(MOVPARAM_AUDIOCODECID, &tempV);
			ptr->audtype = tempV;
			ptr->pos = gMovFirstVInfo.pos;
			ptr->size = gMovFirstVInfo.size;
			//#NT#2012/04/06#Calvin Chang -begin
			// Maximum Video Frames for new H264 driver
			for (frm = 0; frm < MEDIAREADLIB_PREDEC_FRMNUM; frm++) { // Always get maximum frames pos/size on H264 and MJPG
				ptr->vidfrm_pos[frm]  = gMovFirstVInfo.vidfrm_pos[frm];
				ptr->vidfrm_size[frm] = gMovFirstVInfo.vidfrm_size[frm];
			}
			//#NT#2012/04/06#Calvin Chang -end
			ptr->wid = gMovWidth;
			ptr->hei = gMovHeight;
			//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
			ptr->tkwid = gMovTkWidth;
			ptr->tkhei = gMovTkHeight;
			//#NT#2013/10/18#Calvin Chang -end
			//#NT#2009/11/13#Meg Lin#[0007720] -begin
			//#NT#add for user data
			if (size != 0) {
				ptr->udta_pos = filepos + pos;
			} else {
				ptr->udta_pos = 0;
			}
			ptr->udta_size = size;
			//#NT#2009/11/13#Meg Lin#[0007720] -end
			//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
			MOV_Read_GetParam(MOVPARAM_VIDEOROTATE, &tempV);
			ptr->vidrotate = tempV;
			//#NT#2013/04/17#Calvin Chang -end

			break;
		} else { //not moov tag
			//#NT#2010/01/29#Meg Lin# -begin
			if (*p32Data == MOV32_FRE1) {
				ptr->fre1_pos = filepos + 8; //shift len and tag
				ptr->fre1_size = tagSize - 8; //minus len and tag
			}
			//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
			else if (*p32Data == MOV32_THUM) { // Thumbnail pos
				gMovReadLib_ThumbPos  = filepos + 8; //shift len and tag
				gMovReadLib_ThumbSize = tagSize - 8; //minus len and tag
			}
			//#NT#2012/08/21#Calvin Chang -end
			//#NT#2013/05/28#Calvin Chang#Add 'frea' atom for thumbnail -begin
			else if (*p32Data == MOV32_FREA || *p32Data == MOV32_SKIP) {
				// - tima atom (記錄秒數)
				// - thma atom (放置thumbnail : 320x240 420 JPG)
				// - scra atom (放置screennail : 640x480 420 JPG)
				UINT32 usize;
				UINT32 *pfreaData;
				UINT64 upos;

				upos = filepos;
				upos += 8; //shift frea len and tag

				pfreaData = p32Data;
				pfreaData++; // shift frea tag
				usize = MovReadLib_Swap(*pfreaData);
				if(usize == 0xFFFFFFFF) {
					DBG_ERR("1 error usize -1\r\n");
					return E_SYS;
				}
				upos += usize; //shift tima len and tag
				pfreaData += (usize / 4); // shift tima atom (usize should be 0x0c)

				usize = MovReadLib_Swap(*pfreaData);
				if(usize == 0xFFFFFFFF) {
					DBG_ERR("2 error usize -1\r\n");
					return E_SYS;
				}
				if (usize > gMovReadLib_Info.filesize) {
					DBG_ERR("Parse FREA atom fail!!!\r\n");
					//finish = 1;
					return E_SYS;
				}

				if (*(pfreaData + 1) != MOV32_THMA) { // ver atom
					upos += usize;
					pfreaData += (usize / 4); // shift ver atom
                    usize = MovReadLib_Swap(*pfreaData);
					if(usize == 0xFFFFFFFF) {
						DBG_ERR("3 error usize -1\r\n");
						return E_SYS;
					}
				}
				pfreaData++; // shift thma size
                if (*pfreaData == MOV32_THMA)
                {
                    gMovReadLib_ThumbPos  = upos+8;//shift len and tag
                    gMovReadLib_ThumbSize = usize-8;//minus len and tag
                }

                //#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
                upos+=usize; // shift thma atom
				if ((gMovReadLib_Info.filesize - upos) > 0x1000) { // Read file for screen nail
                    ret = (gMovReaderContainer.cbReadFile)(0, upos, 0x1000, hdrAddr);
					if (ret != E_OK) {
						DBG_ERR("4 1stVideo cb read error! %d\r\n", ret);
						return E_SYS;
					}
				}
				else {
                    ret = (gMovReaderContainer.cbReadFile)(0, upos, (gMovReadLib_Info.filesize - upos), hdrAddr);
					if (ret != E_OK) {
						DBG_ERR("5 1stVideo cb read error! %d\r\n", ret);
						return E_SYS;
					}
				}

				pfreaData = (UINT32 *)hdrAddr;
				usize = MovReadLib_Swap(*pfreaData);
				if(usize == 0xFFFFFFFF) {
					DBG_ERR("4 error usize -1\r\n");
					return E_SYS;
				}
				pfreaData++; // shift size
				if (*pfreaData == MOV32_SCRA) {
					// Get screennail
					gMovReadLib_ScraPos  = upos + 8; //shift len and tag
					gMovReadLib_ScraSize = usize - 8; //minus len and tag
				}
				//#NT#2013/12/17#Calvin Chang -end
			}
			//#NT#2013/05/28#Calvin Chang -end
			//#NT#2010/01/29#Meg Lin# -end
			filepos += tagSize;
			if (filepos > gMovReadLib_Info.filesize) {
				DBG_ERR("222MOOV not found!\r\n");
				//finish = 1;
				return E_SYS;
			}
		}
	}
	return E_OK;
}


ER MovReadLib_ParseThumbnail(UINT32 hdrAddr, UINT32 hdrSize, void *pobj)
{
	UINT32 *p32Data;
	UINT64 tagSize = 0, filepos = 0;
	UINT8 finish = 0;
	MEDIA_FIRST_INFO *ptr;
	ER ret;

	ptr = (MEDIA_FIRST_INFO *)pobj;
	ptr->pos = 0;
	ptr->size = 0;
	//#NT#2010/01/29#Meg Lin# -begin
	ptr->fre1_pos = 0;
	ptr->fre1_size = 0;
	//#NT#2010/01/29#Meg Lin# -end
	if (gMovReaderContainer.cbReadFile == NULL) {
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("ERROR!! cbReadFile=NULL!!!\r\n");
		}
		return E_NOSPT;
	}

	//get tag
	filepos = 0;
	gMovReadLib_ThumbSize = 0;

	while (!finish) {
		if ((gMovReadLib_Info.filesize - filepos) > 0x1000) {
            ret = (gMovReaderContainer.cbReadFile)(0, filepos, 0x1000, hdrAddr);
			if (ret != E_OK) {
				(gMovReaderContainer.cbShowErrMsg)("1 thumb cb read error! %d\r\n", ret);
				break;
			}
        }
        else {
            ret = (gMovReaderContainer.cbReadFile)(0, filepos, (gMovReadLib_Info.filesize - filepos), hdrAddr);
			if (ret != E_OK) {
				(gMovReaderContainer.cbShowErrMsg)("2 thumb cb read error! %d\r\n", ret);
				break;
			}
        }

		p32Data = (UINT32 *)hdrAddr;
		tagSize = MovReadLib_Swap(*p32Data);
		if (tagSize == 0) { //something wrong
			return E_NOSPT;
		}
		p32Data++;

		//co64
		if (*p32Data == MOV32_MDAT && *(p32Data - 1) == 0x01000000) {
			tagSize = (UINT64)MovReadLib_Swap(*(p32Data + 1)) << 32;
			tagSize += MovReadLib_Swap(*(p32Data + 2));
		}

		//if (gMovReaderContainer.cbShowErrMsg)
		//{
		//    (gMovReaderContainer.cbShowErrMsg)("TAG222:pos =0x%lx, size=0x%lx, name= 0x%lx!!!\r\n", filepos, tagSize, *p32Data);
		//}
		{
			//#NT#2010/01/29#Meg Lin# -begin
			if (*p32Data == MOV32_FRE1) {
				ptr->fre1_pos = filepos + 8; //shift len and tag
				ptr->fre1_size = tagSize - 8; //minus len and tag
			}
			//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
			else if (*p32Data == MOV32_THUM) { // Thumbnail pos
				gMovReadLib_ThumbPos  = filepos + 8; //shift len and tag
				gMovReadLib_ThumbSize = tagSize - 8; //minus len and tag
//				break;
			}
			//#NT#2012/08/21#Calvin Chang -end
			//#NT#2013/05/28#Calvin Chang#Add 'frea' atom for thumbnail -begin
			else if (*p32Data == MOV32_MOOV) {

				UINT32 tempDur, tempV;

				gMovReadLib_Info.moovpos = filepos;
				gMovReadLib_Info.moovsize = tagSize;
				if (tagSize > hdrSize) {
					DBG_ERR("222needBufSize 0x%lx to parse Header!!!\r\n", tagSize);
					//tagSize = hdrSize;
					//#NT#2009/12/16#Meg Lin# -begin
					//#NT#new: parse media info
					ptr->dur = 0;
					return E_SYS;
					//#NT#2009/12/16#Meg Lin# -end
				}
				//read whole moov tag
				ret = (gMovReaderContainer.cbReadFile)(0, filepos, tagSize, hdrAddr);
				if (ret != E_OK) {
					DBG_ERR("3 1stVideo cb read error! %d\r\n", ret);
					return E_SYS;
				}

				MOVRead_ParseHeaderFirstVideo(hdrAddr, pobj);
				MOV_Read_GetParam(MOVPARAM_VIDTKHDDURATION, &tempDur);//video tkhd duration //2013/01/29 Calvin
				MOV_Read_GetParam(MOVPARAM_VIDEOTIMESCALE, &tempV);
				if (tempV > 1000) {
					UINT64 tempDur64=0;
					tempDur64 = (UINT64)tempDur*1000;
					ptr->dur = tempDur64 / tempV;
				} else {
					ptr->dur = tempDur / 30;    // Time scale should be 30000
				}

				break;
			}
			else if (*p32Data == MOV32_FREA || *p32Data == MOV32_SKIP) {
				// - tima atom (記錄秒數)
				// - thma atom (放置thumbnail : 320x240 420 JPG)
				// - scra atom (放置screennail : 640x480 420 JPG)
				UINT32 upos, usize;
				UINT32 *pfreaData;

				upos = filepos;
				upos += 8; //shift frea len and tag

				pfreaData = p32Data;
				pfreaData++; // shift frea tag
				usize = MovReadLib_Swap(*pfreaData);
				upos += usize; //shift tima len and tag
				pfreaData += (usize / 4); // shift tima atom (usize should be 0x0c)

				usize = MovReadLib_Swap(*pfreaData);

				if (*(pfreaData + 1) != MOV32_THMA) { // ver atom
					upos += usize;
					pfreaData += (usize / 4); // shift ver atom
					usize = MovReadLib_Swap(*pfreaData);
				}

				pfreaData++; // shift thma size
				if (*pfreaData == MOV32_THMA) {
					// Get thumbnail
					gMovReadLib_ThumbPos  = upos + 8; //shift len and tag
					gMovReadLib_ThumbSize = usize - 8; //minus len and tag
//					break;
				}

				//#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
				upos += usize; // shift thma atom
				if ((gMovReadLib_Info.filesize - upos) > 0x1000) { // Read file for screen nail
                    ret = (gMovReaderContainer.cbReadFile)(0, upos, 0x1000, hdrAddr);
					if (ret != E_OK) {
						(gMovReaderContainer.cbShowErrMsg)("3 thumb cb read error! %d\r\n", ret);
						break;
					}
                }
                else {
                    ret = (gMovReaderContainer.cbReadFile)(0, upos, (gMovReadLib_Info.filesize - upos), hdrAddr);
					if (ret != E_OK) {
						(gMovReaderContainer.cbShowErrMsg)("4 thumb cb read error! %d\r\n", ret);
						break;
					}
                }

				pfreaData = (UINT32 *)hdrAddr;
				usize = MovReadLib_Swap(*pfreaData);
				pfreaData++; // shift size
				if (*pfreaData == MOV32_SCRA) {
					// Get screennail
					gMovReadLib_ScraPos  = upos + 8; //shift len and tag
					gMovReadLib_ScraSize = usize - 8; //minus len and tag
				}
				//#NT#2013/12/17#Calvin Chang -end
			}
			//#NT#2013/05/28#Calvin Chang -end
			//#NT#2010/01/29#Meg Lin# -end
			filepos += tagSize;
			if (filepos > gMovReadLib_Info.filesize) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("thumb not found!\r\n");
				}
				//finish = 1;
				return E_SYS;
			}
		}
	}
	return E_OK;
}

ER MovReadLib_ParseVideoInfo(UINT32 hdrAddr, UINT32 hdrSize, void *pobj)
{
	UINT32 *p32Data;
	UINT64 pos, tagSize = 0, filepos = 0;
	UINT32 size, tempDur, tempV, tempSR, tempChs, tmpPos = 0;
	UINT8 finish = 0;
	//#NT#2012/04/06#Calvin Chang -begin
	//Search Index table solution in Parse 1st video
	UINT32 frm = 0;
	//#NT#2012/04/06#Calvin Chang -end
	MEDIA_FIRST_INFO *ptr;
	ER ret;

	ptr = (MEDIA_FIRST_INFO *)pobj;
	ptr->pos = 0;
	ptr->size = 0;
	//#NT#2010/01/29#Meg Lin# -begin
	ptr->fre1_pos = 0;
	ptr->fre1_size = 0;
	//#NT#2010/01/29#Meg Lin# -end
	size = 0;
	if (gMovReaderContainer.cbReadFile == NULL) {
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("ERROR!! cbReadFile=NULL!!!\r\n");
		}
		return E_NOSPT;
	}

	//get tag
	filepos = 0;
	gMovReadLib_ThumbSize = 0;
	while (!finish) {
		if ((gMovReadLib_Info.filesize - filepos) > 0x1000) {
            ret = (gMovReaderContainer.cbReadFile)(0, filepos, 0x1000, hdrAddr);
			if (ret != E_OK) {
				(gMovReaderContainer.cbShowErrMsg)("1 video info cb read error! %d\r\n", ret);
				break;
			}
		}
        else {
            ret = (gMovReaderContainer.cbReadFile)(0, filepos, (gMovReadLib_Info.filesize - filepos), hdrAddr);
			if (ret != E_OK) {
				(gMovReaderContainer.cbShowErrMsg)("2 video info cb read error! %d\r\n", ret);
				break;
			}
        }

		p32Data = (UINT32 *)hdrAddr;
		tagSize = MovReadLib_Swap(*p32Data);
		if (tagSize == 0) { //something wrong
			return E_NOSPT;
		}
		p32Data++;

		//co64
		if (*p32Data == MOV32_MDAT && *(p32Data - 1) == 0x01000000) {
			tagSize = (UINT64)MovReadLib_Swap(*(p32Data + 1)) << 32;
			tagSize += MovReadLib_Swap(*(p32Data + 2));
		}

		//if (gMovReaderContainer.cbShowErrMsg)
		//{
		//    (gMovReaderContainer.cbShowErrMsg)("TAG222:pos =0x%lx, size=0x%lx, name= 0x%lx!!!\r\n", filepos, tagSize, *p32Data);
		//}
		if (*p32Data == MOV32_MOOV) {
			gMovReadLib_Info.moovpos = filepos;
			gMovReadLib_Info.moovsize = tagSize;
			if (tagSize > hdrSize) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("222needBufSize 0x%lx to parse Header!!!\r\n", tagSize);
				}
				//tagSize = hdrSize;
				//#NT#2009/12/16#Meg Lin# -begin
				//#NT#new: parse media info
				ptr->dur = 0;
				return E_SYS;
				//#NT#2009/12/16#Meg Lin# -end
			}
			//read whole moov tag
			ret = (gMovReaderContainer.cbReadFile)(0, filepos, tagSize, hdrAddr);
			if (ret != E_OK) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("3 video info cb read error! %d\r\n", ret);
				}
				break;
			}

			MOVRead_ParseHeaderVideoInfo(hdrAddr, pobj);
			//#NT#2009/11/13#Meg Lin#[0007720] -begin
			//#NT#add for user data
			MovReadLib_GetInfo(MEDIAREADLIB_GETINFO_USERDATA, &tmpPos, &size, 0);
			pos = tmpPos;
			//#NT#2009/11/13#Meg Lin#[0007720] -end
			//MOV_Read_GetParam(MOVPARAM_MVHDDURATION, &tempDur);
			MOV_Read_GetParam(MOVPARAM_VIDTKHDDURATION, &tempDur);//video tkhd duration //2013/01/29 Calvin
			MOV_Read_GetParam(MOVPARAM_VIDEOTIMESCALE, &tempV);
			if (tempV > 1000) {
				ptr->dur = tempDur / (tempV / 1000);    // modified to ms for <1sec movie for MOV file 2013/05/31 -Calvin
			} else {
				ptr->dur = tempDur / 30;    // Time scale should be 30000
			}
			MOV_Read_GetParam(MOVPARAM_SAMPLERATE, &tempSR);
			MOV_Read_GetParam(MOVPARAM_AUDCHANNELS, &tempChs);
			ptr->audSR = tempSR;
			ptr->audChs = tempChs;
			MOV_Read_GetParam(MOVPARAM_VIDEOTYPE, &tempV);
			ptr->vidtype = tempV;
			MOV_Read_GetParam(MOVPARAM_VIDEOFR, &tempV);
			ptr->vidFR = tempV;
			MOV_Read_GetParam(MOVPARAM_AUDIOFR, &tempV);//2012/11/21 Meg
			ptr->audFR = tempV; //2012/11/21 Meg
			MOV_Read_GetParam(MOVPARAM_AUDIOCODECID, &tempV);
			ptr->audtype = tempV;
			ptr->pos = gMovFirstVInfo.pos;
			ptr->size = gMovFirstVInfo.size;
			//#NT#2012/04/06#Calvin Chang -begin
			// Maximum Video Frames for new H264 driver
			for (frm = 0; frm < MEDIAREADLIB_PREDEC_FRMNUM; frm++) { // Always get maximum frames pos/size on H264 and MJPG
				ptr->vidfrm_pos[frm]  = gMovFirstVInfo.vidfrm_pos[frm];
				ptr->vidfrm_size[frm] = gMovFirstVInfo.vidfrm_size[frm];
			}
			//#NT#2012/04/06#Calvin Chang -end
			ptr->wid = gMovWidth;
			ptr->hei = gMovHeight;
			//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
			ptr->tkwid = gMovTkWidth;
			ptr->tkhei = gMovTkHeight;
			//#NT#2013/10/18#Calvin Chang -end
			//#NT#2009/11/13#Meg Lin#[0007720] -begin
			//#NT#add for user data
			if (size != 0) {
				ptr->udta_pos = filepos + pos;
			} else {
				ptr->udta_pos = 0;
			}
			ptr->udta_size = size;
			//#NT#2009/11/13#Meg Lin#[0007720] -end
			//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
			MOV_Read_GetParam(MOVPARAM_VIDEOROTATE, &tempV);
			ptr->vidrotate = tempV;
			//#NT#2013/04/17#Calvin Chang -end

			break;
		} else { //not moov tag
			//#NT#2010/01/29#Meg Lin# -begin
			if (*p32Data == MOV32_FRE1) {
				ptr->fre1_pos = filepos + 8; //shift len and tag
				ptr->fre1_size = tagSize - 8; //minus len and tag
			}
			//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
			else if (*p32Data == MOV32_THUM) { // Thumbnail pos
				gMovReadLib_ThumbPos  = filepos + 8; //shift len and tag
				gMovReadLib_ThumbSize = tagSize - 8; //minus len and tag
			}
			//#NT#2012/08/21#Calvin Chang -end
			//#NT#2013/05/28#Calvin Chang#Add 'frea' atom for thumbnail -begin
			else if (*p32Data == MOV32_FREA || *p32Data == MOV32_SKIP) {
				// - tima atom (記錄秒數)
				// - thma atom (放置thumbnail : 320x240 420 JPG)
				// - scra atom (放置screennail : 640x480 420 JPG)
				UINT32 upos, usize;
				UINT32 *pfreaData;

				upos = filepos;
				upos += 8; //shift frea len and tag

				pfreaData = p32Data;
				pfreaData++; // shift frea tag
				usize = MovReadLib_Swap(*pfreaData);
				upos += usize; //shift tima len and tag
				pfreaData += (usize / 4); // shift tima atom (usize should be 0x0c)

				usize = MovReadLib_Swap(*pfreaData);

				if (*(pfreaData + 1) != MOV32_THMA) { // ver atom
					upos += usize;
					pfreaData += (usize / 4); // shift ver atom
					usize = MovReadLib_Swap(*pfreaData);
				}

				pfreaData++; // shift thma size
				if (*pfreaData == MOV32_THMA) {
					// Get thumbnail
					gMovReadLib_ThumbPos  = upos + 8; //shift len and tag
					gMovReadLib_ThumbSize = usize - 8; //minus len and tag
				}

				//#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
				upos += usize; // shift thma atom
				if ((gMovReadLib_Info.filesize - upos) > 0x1000) {// Read file for screen nail
                    ret = (gMovReaderContainer.cbReadFile)(0, upos, 0x1000, hdrAddr);
					if (ret != E_OK) {
						(gMovReaderContainer.cbShowErrMsg)("4 video info cb read error! %d\r\n", ret);
						break;
					}
				}
                else {
                    ret = (gMovReaderContainer.cbReadFile)(0, upos, (gMovReadLib_Info.filesize - upos), hdrAddr);
					if (ret != E_OK) {
						(gMovReaderContainer.cbShowErrMsg)("5 video info cb read error! %d\r\n", ret);
						break;
					}
				}

				pfreaData = (UINT32 *)hdrAddr;
				usize = MovReadLib_Swap(*pfreaData);
				pfreaData++; // shift size
				if (*pfreaData == MOV32_SCRA) {
					// Get screennail
					gMovReadLib_ScraPos  = upos + 8; //shift len and tag
					gMovReadLib_ScraSize = usize - 8; //minus len and tag
				}
				//#NT#2013/12/17#Calvin Chang -end
			}
			//#NT#2013/05/28#Calvin Chang -end
			//#NT#2010/01/29#Meg Lin# -end
			filepos += tagSize;
			if (filepos > gMovReadLib_Info.filesize) {
				if (gMovReaderContainer.cbShowErrMsg) {
					(gMovReaderContainer.cbShowErrMsg)("222MOOV not found!\r\n");
				}
				//finish = 1;
				return E_SYS;
			}
		}
	}
	return E_OK;
}

