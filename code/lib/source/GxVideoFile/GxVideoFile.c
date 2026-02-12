/*
    GxVideoFile Video Play API function

    This file is for 1st Video Frame and Thumbnail Play API.

    @file       GxVideoFile.c
    @ingroup    mILIBGXVIDFILE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include <string.h>
#include "GxVideoFile.h"
#include "avfile/MediaReadLib.h"
#include "avfile/AVFile_ParserMov.h"
//#include "AVFile_ParserAvi.h"
#include "avfile/AVFile_ParserTs.h"
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "video_decode.h" //#include "VideoDecode.h"
#include "video_codec_h264.h" //#include "VideoCodec_H264.h"
#include "video_codec_h265.h" //#include "VideoCodec_H265.h"
#include "video_codec_mjpg.h"  //#include "VideoCodec_MJPG.h"
#endif
//#include "NvtVerInfo.h"

#include <kwrap/error_no.h>
#include <kwrap/perf.h>
#include <kwrap/verinfo.h>
#include "hdal.h"

#if defined(__FREERTOS)
#include "crypto.h"
#else  // defined(__FREERTOS)
#include <crypto/cryptodev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
static int crypt_fd = -1;
static struct session_op crypt_sess = {0};
static struct crypt_op crypt_cryp = {0};
#endif // defined(__FREERTOS)

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxVidFile
#define __DBGLVL__          (THIS_DBGLVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[IO]"
//#define __DBGFLT__          "[CHK]"
//#include "DebugModule.h"
#include "kwrap/debug.h"

//#define INTERCEPT_TEST

#if _TODO
#else
void debug_msg(char *fmtstr, ...)
{
	//....
}

#define SIZE_64X(a)                              ((((a) + 63)>>6)<<6)

#define GXIMAGE_FALG_INITED           MAKEFOURCC('V','R','A','W') ///< a key value 'V','R','A','W' for indicating image initial.
#define SCALE_UNIT_1X                 0x00010000

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
static ER GxImg_InitBufEx_Temp(PIMG_BUF pImgBuf, UINT32 Width, UINT32 Height, GX_IMAGE_PIXEL_FMT PxlFmt, UINT32 LineOff[], UINT32 PxlAddr[])
{
	UINT32    PlaneNum, i;

	if (!pImgBuf) {
		DBG_ERR("pImgBuf is Null\r\n");
		return E_PAR;
	}
	if (!PxlAddr) {
		DBG_ERR("PxlAddr is Null\r\n");
		return E_PAR;
	}
	if (!LineOff) {
		DBG_ERR("LineOff is Null\r\n");
		return E_PAR;
	}
	switch (PxlFmt) {
	case GX_IMAGE_PIXEL_FMT_YUV422_PLANAR:
		PlaneNum = 3;
		break;
	case GX_IMAGE_PIXEL_FMT_YUV420_PLANAR:
		PlaneNum = 3;
		break;
	case GX_IMAGE_PIXEL_FMT_YUV444_PLANAR:
		PlaneNum = 3;
		break;
	case GX_IMAGE_PIXEL_FMT_YUV422_PACKED:
		PlaneNum = 2;
		break;

	case GX_IMAGE_PIXEL_FMT_YUV420_PACKED:
		PlaneNum = 2;
		break;

	case GX_IMAGE_PIXEL_FMT_Y_ONLY:
		PlaneNum = 1;
		break;

	case GX_IMAGE_PIXEL_FMT_ARGB565:
		PlaneNum = 2;
		break;

	case GX_IMAGE_PIXEL_FMT_RGB888_PLANAR:
		PlaneNum = 3;
		break;

	case GX_IMAGE_PIXEL_FMT_ARGB8888_PACKED:
		PlaneNum = 1;
		break;

	case GX_IMAGE_PIXEL_FMT_ARGB1555_PACKED:
	case GX_IMAGE_PIXEL_FMT_ARGB4444_PACKED:
	case GX_IMAGE_PIXEL_FMT_RGB565_PACKED:
		PlaneNum = 1;
		break;

	case GX_IMAGE_PIXEL_FMT_RAW8:
	case GX_IMAGE_PIXEL_FMT_RAW10:
	case GX_IMAGE_PIXEL_FMT_RAW12:
	case GX_IMAGE_PIXEL_FMT_RAW16:
		PlaneNum = 1;
		break;

	default:
		DBG_ERR("pixel format %d\r\n", PxlFmt);
		return E_PAR;
	}
	memset(pImgBuf, 0x00, sizeof(IMG_BUF));
	pImgBuf->Width = Width;
	pImgBuf->Height = Height;
	pImgBuf->PxlFmt = PxlFmt;
	for (i = 0; i < PlaneNum; i++) {
		pImgBuf->LineOffset[i] = LineOff[i];
		pImgBuf->PxlAddr[i] = PxlAddr[i];
	}
	pImgBuf->flag |= GXIMAGE_FALG_INITED;
	pImgBuf->ScaleRatio.x = SCALE_UNIT_1X;
	pImgBuf->ScaleRatio.y = SCALE_UNIT_1X;

	//DBG_FUNC_CHK("[init]\r\n");
	DBG_MSG("[init]W=%04d, H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%d\r\n", pImgBuf->Width, pImgBuf->Height, pImgBuf->LineOffset[0], pImgBuf->LineOffset[1], pImgBuf->LineOffset[2], pImgBuf->PxlFmt);
	DBG_MSG("[init]A[0]=0x%x, A[1]=0x%x, A[2]=0x%x\r\n", pImgBuf->PxlAddr[0], pImgBuf->PxlAddr[1], pImgBuf->PxlAddr[2]);
	return E_OK;
}
#endif
#endif

/**
    @addtogroup mILIBGXVIDFILE
*/
//@{

//#NT#2013/11/12#Hideo Lin -begin
//#NT#To support max H.264 raw size 1536x1536
#define     GXVIDFILE_H264_1080P_USEDSIZE    0x6E0000                                            // 6.85Mbytes for 1536x1536, 2012/04/24 Calvin Chang
//#define     GXVIDFILE_H264_RAWDATA_USEDSIZE  5120*4096*1.5*MEDIAREADLIB_PREDEC_FRMNUM            // for the Raw Out Reorder Queue in Media Plugin
#define     GXVIDFILE_H264_RAWDATA_USEDSIZE  3840*2176*1.5							             // 2017/08/02 Adam Su
//#NT#2013/11/12#Hideo Lin -end
//#NT#2017/06/20#Adam Su -begin
//#NT#Support max H.265 raw size 5120x4096
#define     GXVIDFILE_H265_1080P_USEDSIZE    0x6E0000                                            // 6.85Mbytes for 1536x1536
#define     GXVIDFILE_H265_RAWDATA_USEDSIZE  3840*2176*1.5                                       // for the Raw Out Reorder Queue in Media Plugin
//#NT#2017/06/20#Adam Su -end

#define     GXVIDFILE_FRAMEMAXSIZE           0x160000                                            // 1.4M bytes max predecode video frame size
#define     GXVIDFILE_PREFRAME_USEDSIZE      GXVIDFILE_FRAMEMAXSIZE*MEDIAREADLIB_PREDEC_FRMNUM   // Pre-decode video frame size
#define     GXVIDFILE_NEEDBUFFERSIZE         GXVIDFILE_H264_1080P_USEDSIZE + GXVIDFILE_H264_RAWDATA_USEDSIZE + GXVIDFILE_PREFRAME_USEDSIZE
//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
#define     GXVIDFILE_THUMB_RAWDATA_USEDSIZE 640*480*2 //4:2:2
#define     GXVIDFILE_THUMB_USERDATA_SIZE    0x20000       // 128K
//#NT#2012/08/21#Calvin Chang -end
#ifdef INTERCEPT_TEST
#define     TEST_TIME_OFFSER_MS              634
#endif

GXVIDFILE_OBJ           gGxVidFileObj = {0};

UINT64                  guiGxVidTotalFileSize = 0;
CONTAINERPARSER         gGxVidFileRContainer = {0};
FPGXVIDEO_READCB64        gGxVidfpReadCB64 = NULL;
FPGXVIDEO_READCB          gGxVidfpReadCB32 = NULL;
static MEDIA_AVIHEADER  gGxVid_AVIHeadInfo = {0};
//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
FPGXVIDEO_GETHUMBCB     gGxVidGetThumbCB = NULL;
//#NT#2012/08/21#Calvin Chang -end
UINT32                  gRealFileType = 0;
UINT32 gGxPoolMain_pa = 0, gGxPoolMain_va = 0, gGxPoolMain_size = 0;
UINT32 gGxPoolHeader_pa = 0, gGxPoolHeader_va = 0, gGxPoolHeader_size = 0;
#ifdef INTERCEPT_TEST
UINT32 gGxPoolTest_pa = 0, gGxPoolTest_va = 0, gGxPoolTest_size = 0;
#endif
MEM_RANGE               gGxMem_Map[GXVIDEO_MEM_CNT] = {0};
UINT32                  gUserSetReadFileSize = 0;
UINT32                  gUserSetReadFileEndSize = 0;
BOOL                    gReadFileFromUser = FALSE;
BOOL                    gReadFileEndFromUser = FALSE;
BOOL                    gbDecrypt = FALSE;
UINT8                   gDecryptKey[32] = {0};
UINT32                  gDecryptAddr = 0;
UINT32                  gDecryptSize = 0;
GXVIDEO_DECRYPT_MODE    gDecryptMode = 0;
UINT32                  gDecryptPos = 0;
_ALIGNED(32) UINT8		gGxH265VidDesc[GXVIDEO_H265_NAL_MAXSIZE] = {0};

#define  DECRTPTO_SIZE    256

VOS_MODULE_VERSION(GxVidFile, 1, 01, 018, 00);

/*
    Read File and Get H265 Description (Vps, Sps, Pps)

    Read file and get h265 description (Vps, Sps, Pps)

    @param [in]   fpReadCB:   file read callback function
    @param [in]   fpReadCB64: file read callback function (co64)
    @param [in]   fileOffset: file offset to get H.265 description.
    @param [out]  DescAddr:   output address of H.265 description.
    @param [out] *pDescSize:  H.265 description size. (0x48, 0x49 or 0x4a)

    @return ER
*/
static ER _GxVideoFile_GetH265Desc(UINT32 fileOffset, UINT32 DescAddr, UINT32 *pDescSize)
{
    ER     EResult;
    UINT32 readSize = GXVIDEO_H265_NAL_MAXSIZE, startCode = 0;
	UINT16 vpsLen = 0, spsLen = 0, ppsLen = 0;
    UINT8  *ptr8 = 0;
    UINT8 descCnt = 0;

    // Check fpReadCB and read file
    if (gGxVidfpReadCB64) {
        EResult = gGxVidfpReadCB64(fileOffset, readSize, DescAddr);
    }
    else if(gGxVidfpReadCB32) {
        EResult = gGxVidfpReadCB32(fileOffset, readSize, DescAddr);
    }
    else {
        DBG_ERR("file reading CB error\r\n");
        return E_SYS;
    }

    if (EResult != E_OK) {
        DBG_ERR("H265Desc file read error!!\r\n");
        return EResult;
    }

    ptr8 = (UINT8*)DescAddr;

	// parse H.265 description and get its length
    while (readSize--) {
		/*** use startcode version ***/
		if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01)) {
            startCode++;
		}
        if (startCode == 4) {
            if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) { // Type: 19 = IDR
                *pDescSize = (UINT32)ptr8 - DescAddr; // addr_dst - addr_src
			}
            else {
                DBG_ERR("Can not find IDR!\r\n");
                *pDescSize = 0;
                EResult = E_SYS;
            }
            break;
        }

		/* ************************************************
		 * startcode with length version
		 * IVOT_N00028-343 support multi-tile
		 * *************************************************/

        do{
            if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 4) == 0x40) && startCode == 0) { // find vps
    			vpsLen = (*(ptr8 + 2) << 8) + *(ptr8 + 3);
    			*(ptr8 + 2) = 0x00; // update start code
    			*(ptr8 + 3) = 0x01;
    			ptr8 += (4 + vpsLen);

    			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && *(ptr8 + 4) == 0x42) { // find sps
    				spsLen = (*(ptr8 + 2) << 8) + *(ptr8 + 3);
    				*(ptr8 + 2) = 0x00; // update start code
    				*(ptr8 + 3) = 0x01;
    				ptr8 += (4 + spsLen);

    				if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && *(ptr8 + 4) == 0x44) { // find pps
    					ppsLen = (*(ptr8 + 2) << 8) + *(ptr8 + 3);
    					*(ptr8 + 2) = 0x00; // update start code
    					*(ptr8 + 3) = 0x01;
    					ptr8 += (4 + ppsLen);

    					descCnt++;
    				}
    			}
    		}
            else
            	break;

        } while(1);

        if(descCnt > 0){

    		if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) { // Type: 19 = IDR
    			*pDescSize = (UINT32)ptr8 - DescAddr; // addr_dst - addr_src
    			DBG_DUMP("vpsLen=0x%x, spsLen=0x%x, ppsLen=0x%x, DescCnt=%u DescSize=0x%x\r\n", vpsLen, spsLen, ppsLen, descCnt, *pDescSize);
    			EResult = E_OK;
    		}
    		else {
                DBG_ERR("Can not find IDR! %u\r\n", __LINE__);
                *pDescSize = 0;
            }
    		break;
        }

        ptr8++;
    }

    return EResult;
}

/*
    Check H265 Description to I-frame Bitstream

    Check H265 Description to I-frame Bitstream

    @param [in]     DesSize:  description size
    @param [in]     *ptr8:    bitstream address

    @return ER
*/

static ER _GxVideoFile_ChkH265IfrmBS(UINT32 DesSize, UINT8 *ptr8)
{
       if (DesSize == 0) {
               DBG_ERR("DesSize error(=%d)\r\n", DesSize);
               return E_SYS;
       }

       ptr8 = ptr8 + DesSize;

       if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) {
               *ptr8 = 0x00;
               *(ptr8+1) = 0x00;
               *(ptr8+2) = 0x00;
               *(ptr8+3) = 0x01;
               DBG_IND("I-frame BSAddr = 0x%x\r\n", ptr8);
       } else {
               DBG_ERR("Get IDR type error!\r\n");
               return E_SYS;
       }

       return E_OK;
}

static UINT32 _GxVidFile_GetDecBufSize(void)
{
	UINT32 DecBufSize = 0;

	if (gGxVidFileObj.max_w == 0) {
		gGxVidFileObj.max_w = ALIGN_CEIL_64(1920);
	}
	if (gGxVidFileObj.max_h == 0) {
		gGxVidFileObj.max_h = ALIGN_CEIL_64(1080);
	}

	DecBufSize = GXVIDFILE_H264_1080P_USEDSIZE + ALIGN_CEIL_64(gGxVidFileObj.max_w) * ALIGN_CEIL_64(gGxVidFileObj.max_h) * 1.5;

	return DecBufSize;
}

static GXVID_ER _GxVidFile_AllocMem(MEDIA_FIRST_INFO Media1stInfoObj)
{
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 all_need_size = 0, total_sec = 0, aud_total_sec = 0, start_addr = 0;
	UINT32 idx;

	total_sec = (Media1stInfoObj.dur/1000)+1; // total second and 1 sec tolerance
	if (Media1stInfoObj.audFrmNum == 0 && Media1stInfoObj.audFR == 0) {
		aud_total_sec = total_sec;
		DBG_DUMP("aud_ttfrm %d, aud_fr %d\r\n", Media1stInfoObj.audFrmNum, Media1stInfoObj.audFR);
	} else {
		aud_total_sec = (Media1stInfoObj.audFrmNum/Media1stInfoObj.audFR) + 1; //audio total second and 1 sec tolerance
	}

	/* calculate need memory size */
	// video frame entry table
	gGxMem_Map[GXVIDEO_MEM_VDOENTTBL].size = total_sec * Media1stInfoObj.vidFR * 0x10; // 0x10: one entry index size

	if (gGxMem_Map[GXVIDEO_MEM_VDOENTTBL].size == 0) {
		DBG_ERR("video entry mem alloc error, total_sec=%d(s), vdo_fr=%d\r\n", total_sec, Media1stInfoObj.vidFR);
	}
	all_need_size += gGxMem_Map[GXVIDEO_MEM_VDOENTTBL].size;

	// audio frame entry table
	gGxMem_Map[GXVIDEO_MEM_AUDENTTBL].size = aud_total_sec * (Media1stInfoObj.audFR+1) * 0x10;

	if (gGxMem_Map[GXVIDEO_MEM_AUDENTTBL].size == 0) {
		DBG_ERR("audio entry mem alloc error, total_sec=%d(s), aud_fr=%d\r\n", aud_total_sec, Media1stInfoObj.audFR);
	}
	all_need_size += gGxMem_Map[GXVIDEO_MEM_AUDENTTBL].size;

	gGxPoolMain_size = all_need_size;
	if ((hd_ret = hd_common_mem_alloc("gxvdo_main", &pa, (void **)&va, gGxPoolMain_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc gxvdo_main failed (size=%x, ret=%d)\r\n", gGxPoolMain_size, hd_ret);
		gGxPoolMain_pa = 0;
		gGxPoolMain_va = 0;
		gGxPoolMain_size = 0;
		return GXVIDEO_PRSERR_SYS;
	}
	gGxPoolMain_va = (UINT32)va;
	gGxPoolMain_pa = (UINT32)pa;

	/* allocate memory */
	start_addr = gGxPoolMain_va;
    for (idx = GXVIDEO_MEM_VDOENTTBL; idx < GXVIDEO_MEM_CNT; idx++)
    {
        gGxMem_Map[idx].addr = start_addr;
        start_addr += gGxMem_Map[idx].size;
    }
	return GXVIDEO_PRSERR_OK;
}

#ifdef INTERCEPT_TEST
static GXVID_ER _GxVidFile_NewTestBuf(UINT32 *pAddr, UINT32 *pSize)
{
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	gGxPoolTest_size = *pSize;
	if ((hd_ret = hd_common_mem_alloc("gxvdo_test", &pa, (void **)&va, gGxPoolTest_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc gxvdo_test failed (size=%x, ret=%d)\r\n", gGxPoolTest_size, hd_ret);
		gGxPoolTest_pa = 0;
		gGxPoolTest_va = 0;
		gGxPoolTest_size = 0;
		return GXVIDEO_PRSERR_SYS;
	}
	gGxPoolTest_va = (UINT32)va;
	gGxPoolTest_pa = (UINT32)pa;
	*pAddr = gGxPoolTest_va;

	return GXVIDEO_PRSERR_OK;
}
#endif

static GXVID_ER _GxVidFile_NewHeaderBuf(UINT32 *pAddr, UINT32 *pSize)
{
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	gGxPoolHeader_size = *pSize;
	if ((hd_ret = hd_common_mem_alloc("gxvdo_header", &pa, (void **)&va, gGxPoolHeader_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc gxvdo_header failed (size=%x, ret=%d)\r\n", gGxPoolHeader_size, hd_ret);
		gGxPoolHeader_pa = 0;
		gGxPoolHeader_va = 0;
		gGxPoolHeader_size = 0;
		return GXVIDEO_PRSERR_SYS;
	}
	gGxPoolHeader_va = (UINT32)va;
	gGxPoolHeader_pa = (UINT32)pa;
	*pAddr = gGxPoolHeader_va;

	return GXVIDEO_PRSERR_OK;
}

static GXVID_ER _GxVidFile_FreeBufPool(UINT32 *pa, UINT32 *va, UINT32 *size)
{
	GXVID_ER ret = GXVIDEO_PRSERR_OK;
	HD_RESULT hd_ret;
	if ((hd_ret = hd_common_mem_free(*pa, (void *)*va)) != HD_OK) {
		DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
        ret = GXVIDEO_PRSERR_SYS;
	}
	*pa = 0;
	*va = 0;
	*size = 0;

	return ret;
}

static UINT32 _GxVidFil_GetVdoGOP(void)
{
	UINT32 Iframe = 0, uiTotalVidFrm = 0;
	UINT32 uiNextIFrm = 0;
	UINT32 uiSkipI = 0;

	// Since there have no GOP number information in H.264 stream level
	// calculate the difference between the Key Frame index by Sync Sample Atoms to get the GOP number
	if (gGxVidFileRContainer.GetInfo) {
		// Get Next I frame number
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_NEXTIFRAMENUM, &Iframe, &uiNextIFrm, &uiSkipI);
		// Get totoal frame number
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VFRAMENUM, &uiTotalVidFrm, 0, 0);
	}
	return uiNextIFrm;
}

static GXVID_ER _GxVidFile_ParseFullHeader(MEDIA_HEADINFO *p_info)
{
	MEM_RANGE buf = {0};

	// new header buffer
    buf.size = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;
	if (_GxVidFile_NewHeaderBuf(&buf.addr, &buf.size) != GXVIDEO_PRSERR_OK) {
        DBG_ERR("New header buffer fail\r\n");
        return GXVIDEO_PRSERR_SYS;
	}

	// set buffer to parser
	if (gGxVidFileRContainer.SetMemBuf) {
		(gGxVidFileRContainer.SetMemBuf)(buf.addr, buf.size);
	}

	// parse full header
	if (gGxVidFileRContainer.ParseHeader) {
		(gGxVidFileRContainer.ParseHeader)(0, 0, p_info);
	}

	// free header buffer
    if (gGxPoolHeader_va) {
		if (_GxVidFile_FreeBufPool(&gGxPoolHeader_pa, &gGxPoolHeader_va, &gGxPoolHeader_size) != GXVIDEO_PRSERR_OK) {
			DBG_ERR("free header buf fail\r\n");
	        return GXVIDEO_PRSERR_SYS;
	    }
    } else {
		DBG_WRN("no need to free header buf\r\n");
    }
	return GXVIDEO_PRSERR_OK;
}

static ER GxVidFile_CBReadFile(UINT8 *path, UINT64 pos, UINT32 size, UINT32 addr)
{
	if (gGxVidfpReadCB64) {
		return gGxVidfpReadCB64(pos, size, addr);
	} else if (gGxVidfpReadCB32) {
		return gGxVidfpReadCB32(pos, size, addr);
	} else {
		DBG_ERR("GxVidFile_CBReadFile pReadCB is NULL!");
		return GXVIDEO_PRSERR_SYS;
	}
}

static ER GxVidFile_CreateContainerObj(UINT16 type)
{
	ER createSuccess = E_OK;
	CONTAINERPARSER *ptr = 0;

	if (type == 0) {
		return E_NOSPT;
	}

	if (type == MEDIA_PARSEINDEX_MOV) {
		ptr = MP_MovReadLib_GerFormatParser();
	} else if (type == MEDIA_PARSEINDEX_AVI) {
#if _TODO
		ptr = MP_AviReadLib_GerFormatParser();
#endif
	} else if (type == MEDIA_PARSEINDEX_TS) {
		ptr = MP_TsReadLib_GerFormatParser();
	}

	//ptr = gFormatParser[type].parser;
	//ptr = MediaReadLib_GerFormatParser(type);
	if (!ptr) {
		return E_NOSPT;
	}

	gGxVidFileRContainer.Probe = ptr->Probe;
	gGxVidFileRContainer.Initialize = ptr->Initialize;
	gGxVidFileRContainer.SetMemBuf = ptr->SetMemBuf;
	gGxVidFileRContainer.ParseHeader = ptr->ParseHeader;
	gGxVidFileRContainer.GetInfo = ptr->GetInfo;
	gGxVidFileRContainer.SetInfo = ptr->SetInfo;
	gGxVidFileRContainer.CustomizeFunc = NULL;
	gGxVidFileRContainer.checkID = ptr->checkID;
	gGxVidFileRContainer.cbShowErrMsg = debug_msg;
	gGxVidFileRContainer.cbReadFile = GxVidFile_CBReadFile;

	if ((type == MEDIA_FILEFORMAT_MOV) || (type == MEDIA_FILEFORMAT_MP4)) {
		createSuccess = MovReadLib_RegisterObjCB(&gGxVidFileRContainer);
	} else if (type == MEDIA_FILEFORMAT_AVI) {
#if _TODO
		createSuccess = MP_AVIReadLib_RegisterObjCB(&gGxVidFileRContainer);
#endif
	} else if (type == MEDIA_FILEFORMAT_TS) {
		createSuccess = TsReadLib_RegisterObjCB(&gGxVidFileRContainer);
	}

	return createSuccess;
}

/*
    Query Video Buffer Needed Size

    Query the needed size of video buffer for decoding 1st video frame

    @param[in] uiTotalFileSize    the total file size of opened video file
    @param[out] uipBufferNeeded   the video buffer needed size

    @return
     - @b GXVIDEO_PRSERR_OK:     Query successfully.
*/
GXVID_ER GxVidFile_Query1stFrameWkBufSize(UINT32 *uipBufferNeeded, UINT64 uiTotalFileSize)
{
	guiGxVidTotalFileSize = uiTotalFileSize;

	*uipBufferNeeded = _GxVidFile_GetDecBufSize() + GXVIDFILE_PREFRAME_USEDSIZE; //GXVIDFILE_NEEDBUFFERSIZE;

	return GXVIDEO_PRSERR_OK;
}

/*
    Decode 1st Video Frame

    Decode 1st video frame for video play mode, must call GxVidFile_Query1stFrameWkBufSize function first.

    @param[in] fpReadCB      the file read callback function pointer
    @param[in] pWorkBuf      the memory address and size
    @param[out] pDstImgBuf   the 1st video fram image

    @return
     - @b GXVIDEO_PRSERR_OK:     Decode 1st video frame successfully.
     - @b GXVIDEO_PRSERR_SYS:    Decode 1st video is failed. Have to check the content of input video file whether is correct.
*/

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
GXVID_ER GxVidFile_Decode1stFrame64(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf)
{
#ifdef INTERCEPT_TEST
	GXVIDFILE_INTERCEPT_PARM *param = 0;
	GXVIDFILE_INTERCEPT_RESULT *ret = 0;
	GxVidFile_GetStreamByTimeOffset(fpReadCB, param, ret);
#endif
	return GxVidFile_Decode1stFrameImpl(0, fpReadCB, pWorkBuf, pDstImgBuf);
}

GXVID_ER GxVidFile_Decode1stFrame(FPGXVIDEO_READCB fpReadCB, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf)
{
	return GxVidFile_Decode1stFrameImpl(fpReadCB, 0, pWorkBuf, pDstImgBuf);
}

GXVID_ER GxVidFile_Decode1stFrameImpl(FPGXVIDEO_READCB fpReadCB, FPGXVIDEO_READCB64 fpReadCB64, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf)
{
	ER                VidER = E_OK;
	ER                TSEndInfoResult = E_OK;
	MP_VDODEC_DECODER     *pVidDecobj;
	MP_VDODEC_1STV_INFO    dsp1stV = {0};
	UINT32            uiLineOff[2] = {0};
	UINT32            PxlAddr[2] = {0};
	UINT32            uiRealfiletype = 0;
	MEDIA_FIRST_INFO  Media1stInfoObj = {0};
	UINT32            uiDecodeBufStartAddr, uiFileReadStartAddr, uiFileReadSize;
	UINT32            uiTSFileEndInfoStartAddr, uiTSFileEndInfoSize;
	UINT32            uifrm = 0, TotalNeedBuf = 0, uiUsedBufSize = 0, uiParseSize = 0;
	CONTAINERPARSER   *ptr = 0;
	UINT32            uiPreDecNum = 0, tsDemuxMemBufSize = 0;
	UINT32            totalSec = 0;

	// decrypt
	BOOL bIsDecrypto = FALSE;
	UINT32 DecryptoPos = 0, Decryptoaddr = 0;
	UINT8 decrypt_buf[DECRTPTO_SIZE] = {0};
	UINT32 decrypt_size = DECRTPTO_SIZE;

	TotalNeedBuf = _GxVidFile_GetDecBufSize() + GXVIDFILE_PREFRAME_USEDSIZE; // refer to GxVidFile_Query1stFrameWkBufSize()
	if (pWorkBuf->size < TotalNeedBuf) {
		DBG_ERR("not enough buf! alloc_size=0x%x, need_size=0x%x\r\n", pWorkBuf->size, TotalNeedBuf);
		return GXVIDEO_PRSERR_SYS;
	}

	// Allocate Memory
	uiDecodeBufStartAddr = pWorkBuf->addr;
	uiFileReadStartAddr = uiTSFileEndInfoStartAddr = uiDecodeBufStartAddr + _GxVidFile_GetDecBufSize();
	uiUsedBufSize += _GxVidFile_GetDecBufSize();
						  // (GXVIDFILE_H264_1080P_USEDSIZE + GXVIDFILE_H264_RAWDATA_USEDSIZE);
	uiParseSize = pWorkBuf->size - uiUsedBufSize;

	// Check file format
	if (fpReadCB64) {
		gGxVidfpReadCB64 = fpReadCB64;
		gGxVidfpReadCB32 = NULL;
	} else if (fpReadCB) {
		gGxVidfpReadCB32 = fpReadCB;
		gGxVidfpReadCB64 = NULL;
	}
	// Read file to data buffer
	if (!gGxVidfpReadCB64 && !gGxVidfpReadCB32) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gReadFileFromUser) {
		uiFileReadSize = gUserSetReadFileSize;
	} else {
		uiFileReadSize = 0x100000;
	}

	uiUsedBufSize += uiFileReadSize*2;
	tsDemuxMemBufSize = uiFileReadSize*2;
	uiUsedBufSize += tsDemuxMemBufSize;
	if (uiUsedBufSize > pWorkBuf->size) {
		DBG_ERR("Work buf not enough, please set FileReadSize < 0x%x\r\n", uiFileReadSize);
		return GXVIDEO_PRSERR_SYS;
	}

	//if file size is small
	if ((uiFileReadSize > guiGxVidTotalFileSize) && (guiGxVidTotalFileSize != 0)) {
		uiFileReadSize = (UINT32)guiGxVidTotalFileSize;
	}

	if (gGxVidfpReadCB64) {
		VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);
	} else {
		VidER = gGxVidfpReadCB32(0, uiFileReadSize, uiFileReadStartAddr);
	}

	if (VidER != E_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_MOV;
			gRealFileType = MEDIA_PARSEINDEX_MOV;
		}
	}

	// Get AVI
#if _TODO
	ptr = MP_AviReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_AVI;
			gRealFileType = MEDIA_PARSEINDEX_AVI;
		}
	}
#endif

	// Get TS
	ptr = MP_TsReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, TS_PACKET_SIZE * TS_CHECK_COUNT);

		if (VidER > 6) { //score
			uiRealfiletype = MEDIA_PARSEINDEX_TS;
			gRealFileType = MEDIA_PARSEINDEX_TS;
		}
	}

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Create container parser
	VidER = GxVidFile_CreateContainerObj(uiRealfiletype);

	if (VidER != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (uiRealfiletype == MEDIA_FILEFORMAT_MP4 || uiRealfiletype == MEDIA_FILEFORMAT_MOV) {
		if (gGxVidFileRContainer.SetMemBuf) {
			(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
		}
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSDEMUXBUF, uiFileReadStartAddr + uiFileReadSize*2, tsDemuxMemBufSize, 0);
	} else {
		DBG_ERR("invalid file_type=0x%x\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
									   guiGxVidTotalFileSize >> 32,
									   guiGxVidTotalFileSize,
									   0);

	// Parse 1st Video information
	if ((uiRealfiletype == MEDIA_FILEFORMAT_MOV) || (uiRealfiletype == MEDIA_FILEFORMAT_MP4)) {
		gGxVidFileRContainer.ParseHeader = MovReadLib_Parse1stVideo;
		uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
#if _TODO
		gGxVidFileRContainer.ParseHeader = MP_AVIReadLib_Parse1stVideo;
		uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;
#endif
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		gGxVidFileRContainer.ParseHeader = TsReadLib_Parse1stVideo;
	}

	//if file size is small
	if (uiFileReadSize > guiGxVidTotalFileSize) {
		uiFileReadSize = (UINT32)guiGxVidTotalFileSize;
	}

	if (gGxVidFileRContainer.ParseHeader) {
		VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);
	}

	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		UINT32 uiRetrySize = 0;
		uiRetrySize = uiFileReadSize * 2;

		if (Media1stInfoObj.bStillVdoPid) {
			DBG_DUMP("retry 1 time, size 0x%x\r\n", uiRetrySize);
			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(0, uiRetrySize, uiFileReadStartAddr);
				if (VidER != E_OK) {
					DBG_ERR("gGxVidfpReadCB64 fail\r\n");
					return GXVIDEO_PRSERR_SYS;
				}
			} else {
				VidER = gGxVidfpReadCB32(0, uiRetrySize, uiFileReadStartAddr);
				if (VidER != E_OK) {
					DBG_ERR("gGxVidfpReadCB32 fail\r\n");
					return GXVIDEO_PRSERR_SYS;
				}
			}
			VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiRetrySize, (void *)&Media1stInfoObj);
			if (VidER != E_OK) {
				DBG_ERR("parse 1st video fail\r\n");
				return GXVIDEO_PRSERR_SYS;
			}
		}

		if (Media1stInfoObj.bStillVdoPid) {
			DBG_ERR("Retry fail, please set lager read file size > 0x%x by user !!!\r\n", uiRetrySize);
			return GXVIDEO_PRSERR_SYS;
		}
	}

	// Parse TS file end info
	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		if (gReadFileEndFromUser) {
			uiTSFileEndInfoSize = gUserSetReadFileEndSize;
			if (uiTSFileEndInfoSize*2 > uiParseSize) {
				DBG_ERR("Work buff not enough, set gUserSetReadFileEndSize <= 0x%x !!!\r\n", uiParseSize >> 1);
				return GXVIDEO_PRSERR_SYS;
			}
		} else {
			uiTSFileEndInfoSize = 0x758; //10 packets size of TS (188bytes * 10)
		}

		if (gGxVidfpReadCB64) {
			VidER = gGxVidfpReadCB64(guiGxVidTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
		} else {
			VidER = gGxVidfpReadCB32(guiGxVidTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
		}

		if (VidER != FST_STA_OK) {
			DBG_ERR("Read File Error!!! %d \r\n", VidER);
			return GXVIDEO_PRSERR_SYS;
		}

		if (gGxVidFileRContainer.SetInfo) {
			TSEndInfoResult = (gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSENDINFO,
										   uiTSFileEndInfoStartAddr, uiTSFileEndInfoSize, 0);
			DBG_IND("TSEndInfoResult: %d \r\n", TSEndInfoResult);

		}

		if (gGxVidFileRContainer.GetInfo) {
			(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_TOTALSEC, &totalSec, 0, 0);
		}

		if (TSEndInfoResult == E_SYS) {
			if (gReadFileEndFromUser) {
				uiTSFileEndInfoSize = gUserSetReadFileEndSize*2;
			} else {
				uiTSFileEndInfoSize = guiGxVidTotalFileSize < 0x200000 ? guiGxVidTotalFileSize : 0x200000;
			}

			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(guiGxVidTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
			} else {
				VidER = gGxVidfpReadCB32(guiGxVidTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
			}

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}

			if (gGxVidFileRContainer.SetInfo)
				(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSENDINFO,
											   uiTSFileEndInfoStartAddr, uiTSFileEndInfoSize, 0);
			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_TOTALSEC, &totalSec, 0, 0);
			}

			if (!Media1stInfoObj.dur) {
				DBG_ERR("retry fail and totalSec is 0, please set gUserSetReadFileEndSize > 0x%x\r\n", uiTSFileEndInfoSize);
				return GXVIDEO_PRSERR_SYS;
			}
		}
	}

	if (VidER != E_OK) {
		DBG_ERR("Parse Header fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	//Read last pre-decode frame position + size to buffer
	// Check Pre-decode number
	for (uifrm = 0; uifrm < MEDIAREADLIB_PREDEC_FRMNUM; uifrm++) {
		if (Media1stInfoObj.vidfrm_size[uifrm] == 0) {
			break;
		}
		uiPreDecNum++;
	}

	if (uiPreDecNum > 0) {
		if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
			uiFileReadSize = (guiGxVidTotalFileSize < 0x100000) ? (UINT32)guiGxVidTotalFileSize : 0x100000;
		} else {
			uiFileReadSize = (Media1stInfoObj.vidfrm_pos[uiPreDecNum - 1] + Media1stInfoObj.vidfrm_size[uiPreDecNum - 1]);
		}
	} else {
		DBG_ERR("PreDecNum is zero!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}


	if (uiFileReadSize > GXVIDFILE_PREFRAME_USEDSIZE) {
		DBG_ERR("File read buffer size for video frame is not enough!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (uiRealfiletype != MEDIA_FILEFORMAT_TS) {
		if (gGxVidfpReadCB64) {
			VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);
		} else {
			VidER = gGxVidfpReadCB32(0, uiFileReadSize, uiFileReadStartAddr);
		}

		if (VidER != FST_STA_OK) {
			DBG_ERR("Read File Error!!! %d \r\n", VidER);
			return GXVIDEO_PRSERR_SYS;
		}
	}

    // Configure the MP_VDODEC_1STV_INFO
    dsp1stV.addr          = uiFileReadStartAddr + Media1stInfoObj.pos; // 1st frame addr
    dsp1stV.size          = Media1stInfoObj.size; // 1st frame size
    dsp1stV.decodeBuf     = uiDecodeBufStartAddr; // Video Decode buffer
    dsp1stV.decodeBufSize = _GxVidFile_GetDecBufSize();//GXVIDFILE_H264_1080P_USEDSIZE + GXVIDFILE_H264_RAWDATA_USEDSIZE; // Video Decode buffer size
    dsp1stV.width         = ALIGN_CEIL_16(Media1stInfoObj.wid); // Algien 16-lines base for H.264 HW limitation
    dsp1stV.height        = Media1stInfoObj.hei; // Video frame height

	// decrypt
	GxVidFile_GetParam(GXVIDFILE_PARAM_IS_DECRYPT, (UINT32*)&bIsDecrypto);
	GxVidFile_GetParam(GXVIDFILE_PARAM_DECRYPT_POS, (UINT32*)&DecryptoPos);
	// I-frame decrypt
	if (bIsDecrypto && (DecryptoPos & GXVIDEO_DECRYPT_TYPE_I_FRAME)) {
		if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H264) {
			Decryptoaddr = dsp1stV.addr + 16;
		} else if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H265) {
			if (uiRealfiletype == MEDIA_FILEFORMAT_MOV) {
				VidER = _GxVideoFile_GetH265Desc(Media1stInfoObj.vidfrm_pos[0], (UINT32)&gGxH265VidDesc[0], &dsp1stV.DescLen);
				if (VidER != E_OK) {
					DBG_ERR("Get H265Desc Error!!\r\n");
					return GXVIDEO_PRSERR_SYS;
				}
			}
			Decryptoaddr = dsp1stV.addr + dsp1stV.DescLen + 16;
			DBG_ERR("H265 dsp1stV.DescLen 0x%x\r\n", dsp1stV.DescLen);
		}
		memcpy((void *)decrypt_buf, (void *)Decryptoaddr, decrypt_size);
		GxVideoFile_Decrypto((UINT32)decrypt_buf, decrypt_size);
		memcpy((void *)Decryptoaddr, (void *)decrypt_buf, decrypt_size);
	}

    if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H264) {
        if (gGxVidFileRContainer.GetInfo)
            (gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_H264DESC, &dsp1stV.DescAddr, &dsp1stV.DescLen, 0);
    }
    else if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H265) {
		if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
			if (gGxVidFileRContainer.GetInfo)
	            (gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_H265DESC, &dsp1stV.DescAddr, &dsp1stV.DescLen, 0);
		} else {
	        VidER = _GxVideoFile_GetH265Desc(Media1stInfoObj.vidfrm_pos[0], (UINT32)&gGxH265VidDesc[0], &dsp1stV.DescLen);
	        if (VidER != E_OK) {
	            DBG_ERR("Get H265Desc Error!!\r\n");
	            return GXVIDEO_PRSERR_SYS;
	        }
	        dsp1stV.DescAddr  = (UINT32)&gGxH265VidDesc[0];
			dsp1stV.decodeBufSize = GXVIDFILE_H265_1080P_USEDSIZE + GXVIDFILE_H265_RAWDATA_USEDSIZE;
		}
    }

    // Get Video Decode Object
    pVidDecobj = MP_MjpgDec_getVideoDecodeObject();
    if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H264) {
        pVidDecobj = MP_H264Dec_getVideoDecodeObject();
        //Search Index table solution in Parse 1st video & pre-decode video frames for new H264 driver
        dsp1stV.decfrmnum = uiPreDecNum;

		for (uifrm = 0; uifrm < dsp1stV.decfrmnum; uifrm++) {
			dsp1stV.frmaddr[uifrm] = (uiFileReadStartAddr + Media1stInfoObj.vidfrm_pos[uifrm]);
			dsp1stV.frmsize[uifrm] = Media1stInfoObj.vidfrm_size[uifrm];

			// 1st video including h264 desc string in AVI file format
			if ((uifrm == 0) && (uiRealfiletype == MEDIA_FILEFORMAT_AVI)) {
				dsp1stV.frmaddr[uifrm] += dsp1stV.DescLen;
				dsp1stV.frmsize[uifrm] -= dsp1stV.DescLen;
			}
		}
	}
    else if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H265) {
        pVidDecobj = MP_H265Dec_getVideoDecodeObject();

		dsp1stV.decfrmnum = 1; // Not support B-frame

		for (uifrm = 0; uifrm < dsp1stV.decfrmnum; uifrm++) {
			dsp1stV.frmaddr[uifrm] = (uiFileReadStartAddr + Media1stInfoObj.vidfrm_pos[uifrm]);

			if (uiRealfiletype != MEDIA_FILEFORMAT_TS) {
				if (_GxVideoFile_ChkH265IfrmBS(dsp1stV.DescLen, (UINT8*)dsp1stV.frmaddr[uifrm]) != E_OK) {
		            DBG_ERR("Get H265 I-frame Error!!\r\n");
		            return GXVIDEO_PRSERR_SYS;
		        }
				dsp1stV.frmaddr[uifrm] += dsp1stV.DescLen;
			}
			dsp1stV.frmsize[uifrm] = Media1stInfoObj.vidfrm_size[uifrm];
		}
    }
	// Call Media Plug-in to decode 1st video
	if (pVidDecobj->CustomizeFunc) {
		VidER = (pVidDecobj->CustomizeFunc)(MP_VDODEC_CUSTOM_DECODE1ST, &dsp1stV);
		if (VidER != E_OK) {
			DBG_ERR("Decoding 1st frame fails...\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	}

	// Store 1st Video ImgBuf
	uiLineOff[0] = uiLineOff[1] = SIZE_64X(dsp1stV.width);
	PxlAddr[0]   = dsp1stV.y_Addr;
	PxlAddr[1]   = dsp1stV.cb_Addr;

	GxImg_InitBufEx_Temp(pDstImgBuf,
					Media1stInfoObj.wid,//dsp1stV.width, // Algien 32-lines base for H.264 HW limitation
					dsp1stV.height,
					GX_IMAGE_PIXEL_FMT_YUV420_PACKED,
					uiLineOff,
					PxlAddr);

	return GXVIDEO_PRSERR_OK;
}
#endif
//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
/*
    Register callback function for thumbnail

    Register callback function for getting thumbnail stream in user data

    @param[in] fpGetThumbCB    FPGXVIDEO_GETHUMBCB callback function pointer

    @return
     - @b GXVIDEO_PRSERR_OK:     Register successfully.
*/
GXVID_ER GxVidFile_RegisterGetThumbCB(FPGXVIDEO_GETHUMBCB fpGetThumbCB)
{
	if (fpGetThumbCB) {
		gGxVidGetThumbCB = fpGetThumbCB;
	} else {
		DBG_ERR("Get Thumb Callback function is NULL!...\r\n");
	}

	return GXVIDEO_PRSERR_OK;
}

/*
    Query Video Buffer Needed Size

    Query the needed size of video buffer for decoding thumbnail frame

    @param[in] uiTotalFileSize    the total file size of opened video file
    @param[out] uipBufferNeeded   the video buffer needed size

    @return
     - @b GXVIDEO_PRSERR_OK:     Query successfully.
*/
GXVID_ER GxVidFile_QueryThumbWkBufSize(UINT32 *uipBufferNeeded, UINT64 uiTotalFileSize)
{
	guiGxVidTotalFileSize = uiTotalFileSize;

	*uipBufferNeeded = GXVIDFILE_THUMB_RAWDATA_USEDSIZE + GXVIDFILE_THUMB_USERDATA_SIZE;

	return GXVIDEO_PRSERR_OK;

}


/*
    Decode Thumbnail Frame

    Decode thumbnail frame for video file play mode, must call GxVidFile_QueryThumbWkBufSize function first.

    @param[in] fpReadCB      the file read callback function pointer
    @param[in] pWorkBuf      the memory address and size
    @param[out] pDstImgBuf   the thumbnail fram image

    @return
     - @b GXVIDEO_PRSERR_OK:     Decode thumbnail frame successfully.
     - @b GXVIDEO_PRSERR_SYS:    Decode thumbnail frame is failed.
*/

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
GXVID_ER GxVidFile_DecodeThumbFrame(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf)
{
	ER                VidER = E_OK;
	MP_VDODEC_DECODER     *pVidDecobj;
	MP_VDODEC_1STV_INFO    dsp1stV = {0};
	UINT32            uiLineOff[2] = {0};
	UINT32            PxlAddr[2] = {0};
	UINT32            uiRealfiletype = 0;
	MEDIA_FIRST_INFO  Media1stInfoObj = {0};
	UINT32            uiDecodeBufStartAddr;
	UINT32            uiFileReadStartAddr, uiFileReadSize;
	UINT32            uiThumbSize;
	UINT64            uiThumbStartPos;
	CONTAINERPARSER   *ptr = 0;

	// Allocate Memory
	if (pWorkBuf->size < (GXVIDFILE_THUMB_RAWDATA_USEDSIZE + GXVIDFILE_THUMB_USERDATA_SIZE)) {
		DBG_ERR("Work buffer size is not enough!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiDecodeBufStartAddr = pWorkBuf->addr;
	uiFileReadStartAddr  = uiDecodeBufStartAddr + GXVIDFILE_THUMB_RAWDATA_USEDSIZE;

	// Check file format
	gGxVidfpReadCB64 = fpReadCB;
	// Read file to data buffer
	if (!gGxVidfpReadCB64) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiFileReadSize = 0x40100; // enlarge the header read size for AVI file format 2013/09/23 Calvin

	VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);

	if (VidER != FST_STA_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_MOV;
			gRealFileType = MEDIA_PARSEINDEX_MOV;
		}
	}

	// Get AVI
#if _TODO
	ptr = MP_AviReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_AVI;
			gRealFileType = MEDIA_PARSEINDEX_AVI;
		}
	}
#endif

	// Get TS
	ptr = MP_TsReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, TS_PACKET_SIZE * TS_CHECK_COUNT);

		if (VidER > 6) { //score
			uiRealfiletype = MEDIA_PARSEINDEX_TS;
			gRealFileType = MEDIA_PARSEINDEX_TS;

			//read more for 1st frame
			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(0, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr);
			} else {
				VidER = gGxVidfpReadCB32(0, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr);
			}

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}
		}
	}

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Create container parser
	VidER = GxVidFile_CreateContainerObj(uiRealfiletype);

	if (VidER != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetMemBuf) {
		(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
	}

	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSDEMUXBUF, uiFileReadStartAddr + GXVIDEO_TSDEMUX_THUMB_OFFSET, 0, 0);
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
									   guiGxVidTotalFileSize >> 32,
									   guiGxVidTotalFileSize,
									   0);

	// Parse 1st Video information
	if ((uiRealfiletype == MEDIA_FILEFORMAT_MOV) || (uiRealfiletype == MEDIA_FILEFORMAT_MP4))
		//gGxVidFileRContainer.ParseHeader = MovReadLib_Parse1stVideo;
	{
		gGxVidFileRContainer.ParseHeader = MovReadLib_ParseThumbnail;
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
#if _TODO
		gGxVidFileRContainer.ParseHeader = MP_AVIReadLib_Parse1stVideo;
#endif
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		gGxVidFileRContainer.ParseHeader = TsReadLib_Parse1stVideo;
	}

	uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;

	if (gGxVidFileRContainer.ParseHeader) {
		VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);
	}

	if (VidER != E_OK) {
		DBG_ERR("Parse Header fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Get Thumbnail by Callback function
	uiThumbStartPos = 0;
	uiThumbSize     = 0;

	if (gGxVidGetThumbCB) {
		// Get User Data in file
		if (Media1stInfoObj.udta_size) {
			VidER = gGxVidfpReadCB64(Media1stInfoObj.udta_pos, Media1stInfoObj.udta_size, uiFileReadStartAddr);

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}

			gGxVidGetThumbCB(uiFileReadStartAddr, Media1stInfoObj.udta_size, &uiThumbStartPos, &uiThumbSize);
			uiThumbStartPos = Media1stInfoObj.udta_pos + uiThumbStartPos;
		} else {
			DBG_ERR("No User Data in file...\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	} else {
		//DBG_WRN("No Get Thumb Callback function...\r\n");

		if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
			if (gGxVidFileRContainer.SetInfo)
				(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSTHUMBINFO,
											   uiFileReadStartAddr, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr + GXVIDEO_TSDEMUX_THUMB_OFFSET);

			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, &uiFileReadStartAddr, &uiThumbSize, 0);
			}
		} else {
			// Get thumb from degfault file position
			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, (UINT32 *)&uiThumbStartPos, &uiThumbSize, 0);
			}
		}
	}

	if (uiThumbSize) {
		if (uiRealfiletype != MEDIA_FILEFORMAT_TS) {
			VidER = gGxVidfpReadCB64(uiThumbStartPos, uiThumbSize, uiFileReadStartAddr);     // Get thumb in file

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}
		}
	} else {
		DBG_ERR("No Thumb frame in file...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Decode Thumb frame
	dsp1stV.addr          = uiFileReadStartAddr;
	dsp1stV.size          = uiThumbSize;
	dsp1stV.decodeBuf     = uiDecodeBufStartAddr;
	dsp1stV.decodeBufSize = GXVIDFILE_THUMB_RAWDATA_USEDSIZE;

	pVidDecobj = MP_MjpgDec_getVideoDecodeObject();

	// Call Media Plug-in to decode
	if (pVidDecobj->CustomizeFunc) {
		VidER = (pVidDecobj->CustomizeFunc)(MP_VDODEC_CUSTOM_DECODE1ST, &dsp1stV);
		if (VidER != E_OK) {
			DBG_ERR("Decoding Thumbnail frame fails...\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	}

	// Store Thumbnail frame ImgBuf
	uiLineOff[0] = uiLineOff[1] = dsp1stV.width;
	PxlAddr[0]   = dsp1stV.y_Addr;
	PxlAddr[1]   = dsp1stV.cb_Addr;

	GxImg_InitBufEx_Temp(pDstImgBuf,
					dsp1stV.width,
					dsp1stV.height,
					GX_IMAGE_PIXEL_FMT_YUV420_PACKED,
					uiLineOff,
					PxlAddr);

	return GXVIDEO_PRSERR_OK;

}
#endif
// for parsing thumbnail information
GXVID_ER GxVidFile_ParseThumbInfo(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo)
{
	ER                VidER = E_OK;
	UINT32            uiRealfiletype = 0;
	MEDIA_FIRST_INFO  Media1stInfoObj = {0};
	UINT32            uiDecodeBufStartAddr;
	UINT32            uiFileReadStartAddr, uiFileReadSize;
	UINT32            uiThumbSize;
	UINT32            uiTotalSec = 0;
	UINT64            uiThumbStartPos;
	CONTAINERPARSER   *ptr = 0;

	// Allocate Memory
	if (pWorkBuf->size < (GXVIDFILE_THUMB_RAWDATA_USEDSIZE + GXVIDFILE_THUMB_USERDATA_SIZE)) {
		DBG_ERR("Work buffer size is not enough!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiDecodeBufStartAddr = pWorkBuf->addr;
	uiFileReadStartAddr  = uiDecodeBufStartAddr + GXVIDFILE_THUMB_RAWDATA_USEDSIZE;

	// Check file format
	gGxVidfpReadCB64 = fpReadCB;
	// Read file to data buffer
	if (!gGxVidfpReadCB64) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiFileReadSize = 0x40100; // enlarge the header read size for AVI file format 2013/09/23 Calvin

	VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);

	if (VidER != FST_STA_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_MOV;
			gRealFileType = MEDIA_PARSEINDEX_MOV;
		}
	}

	// Get AVI
#if _TODO
	ptr = MP_AviReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_AVI;
			gRealFileType = MEDIA_PARSEINDEX_AVI;
		}
	}
#endif

	// Get TS
	ptr = MP_TsReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, TS_PACKET_SIZE * TS_CHECK_COUNT);

		if (VidER > 6) { //score
			uiRealfiletype = MEDIA_PARSEINDEX_TS;
			gRealFileType = MEDIA_PARSEINDEX_TS;

			//read more for 1st frame
			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(0, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr);
			} else {
				VidER = gGxVidfpReadCB32(0, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr);
			}

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}
		}
	}

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Create container parser
	VidER = GxVidFile_CreateContainerObj(uiRealfiletype);

	if (VidER != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetMemBuf) {
		(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
	}

	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSDEMUXBUF, uiFileReadStartAddr + GXVIDEO_TSDEMUX_THUMB_OFFSET, 0, 0);
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
									   guiGxVidTotalFileSize >> 32,
									   guiGxVidTotalFileSize,
									   0);

	// Parse 1st Video information
	if ((uiRealfiletype == MEDIA_FILEFORMAT_MOV) || (uiRealfiletype == MEDIA_FILEFORMAT_MP4))
		//gGxVidFileRContainer.ParseHeader = MovReadLib_Parse1stVideo;
	{
		gGxVidFileRContainer.ParseHeader = MovReadLib_ParseThumbnail;
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
#if _TODO
		gGxVidFileRContainer.ParseHeader = MP_AVIReadLib_Parse1stVideo;
#endif
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		gGxVidFileRContainer.ParseHeader = TsReadLib_Parse1stVideo;
	}

	uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;

	if (gGxVidFileRContainer.ParseHeader) {
		VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);
	}

	if (VidER != E_OK) {
		DBG_ERR("Parse Header fails. VidER=%d\r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get Thumbnail by Callback function
	uiThumbStartPos = 0;
	uiThumbSize     = 0;

	if (gGxVidGetThumbCB) {
		// Get User Data in file
		if (Media1stInfoObj.udta_size) {
			VidER = gGxVidfpReadCB64(Media1stInfoObj.udta_pos, Media1stInfoObj.udta_size, uiFileReadStartAddr);

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}

			gGxVidGetThumbCB(uiFileReadStartAddr, Media1stInfoObj.udta_size, &uiThumbStartPos, &uiThumbSize);
			uiThumbStartPos = Media1stInfoObj.udta_pos + uiThumbStartPos;
		} else {
			DBG_ERR("No User Data in file...\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	} else {
		//DBG_WRN("No Get Thumb Callback function...\r\n");

		if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
			if (gGxVidFileRContainer.SetInfo)
				(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSTHUMBINFO,
											   uiFileReadStartAddr, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr + GXVIDEO_TSDEMUX_THUMB_OFFSET);

			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, &uiFileReadStartAddr, &uiThumbSize, 0);
			}

			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_TOTALSEC, (UINT32 *)&uiTotalSec, 0, 0);
			}

		} else {
			// Get thumb from degfault file position
			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, (UINT32 *)&uiThumbStartPos, &uiThumbSize, 0);
			}
			uiTotalSec = Media1stInfoObj.dur / 1000;
		}
	}

	pVideoInfo->uiThumbOffset = uiThumbStartPos;
	pVideoInfo->uiThumbSize = uiThumbSize;
	pVideoInfo->uiToltalSecs = uiTotalSec;

	return GXVIDEO_PRSERR_OK;
}
GXVID_ER GxVidFile_GetTsThumb64(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT32 *thumbAddr, UINT32 *thumbSize)
{
	return GxVidFile_GetTsThumbImpl(0, fpReadCB, pWorkBuf, thumbAddr, thumbSize);
}

GXVID_ER GxVidFile_GetTsThumb(FPGXVIDEO_READCB fpReadCB, PMEM_RANGE pWorkBuf, UINT32 *thumbAddr, UINT32 *thumbSize)
{
	return GxVidFile_GetTsThumbImpl(fpReadCB, 0, pWorkBuf, thumbAddr, thumbSize);
}

GXVID_ER GxVidFile_GetTsThumbImpl(FPGXVIDEO_READCB fpReadCB, FPGXVIDEO_READCB64 fpReadCB64, PMEM_RANGE pWorkBuf, UINT32 *thumbAddr, UINT32 *thumbSize)
{
	UINT32 uiFileReadStartAddr, uiFileReadSize;
	UINT32            uiRealfiletype = 0, uiThumbSize = 0;
	CONTAINERPARSER   *ptr = 0;
	ER                VidER = E_OK;

	// Check file format
	if (fpReadCB64) {
		gGxVidfpReadCB64 = fpReadCB64;
		gGxVidfpReadCB32 = NULL;
	} else if (fpReadCB) {
		gGxVidfpReadCB32 = fpReadCB;
		gGxVidfpReadCB64 = NULL;
	}

	// Read file to data buffer
	if (!gGxVidfpReadCB64 && !gGxVidfpReadCB32) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Allocate Memory
	if (pWorkBuf->size < GXVIDEO_PARSE_TS_THUMB_BUFFER_SIZE) {
		DBG_ERR("Work buffer size is not enough!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiFileReadStartAddr = pWorkBuf->addr + GXVIDFILE_THUMB_RAWDATA_USEDSIZE;;
	uiFileReadSize = GXVIDEO_TSDEMUX_THUMB_OFFSET;

	if (gGxVidfpReadCB64) {
		VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);
	} else {
		VidER = gGxVidfpReadCB32(0, uiFileReadSize, uiFileReadStartAddr);
	}

	if (VidER != FST_STA_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get TS
	ptr = MP_TsReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, TS_PACKET_SIZE * TS_CHECK_COUNT);

		if (VidER > 6) { //score
			uiRealfiletype = MEDIA_PARSEINDEX_TS;
			gRealFileType = MEDIA_PARSEINDEX_TS;
		}
	}

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Create container parser
	VidER = GxVidFile_CreateContainerObj(uiRealfiletype);

	if (VidER != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetMemBuf) {
		(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSTHUMBINFO,
									   uiFileReadStartAddr, GXVIDEO_TSDEMUX_THUMB_OFFSET, uiFileReadStartAddr + GXVIDEO_TSDEMUX_THUMB_OFFSET);

	if (gGxVidFileRContainer.GetInfo) {
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, &uiFileReadStartAddr, &uiThumbSize, 0);
	}

	*thumbAddr = uiFileReadStartAddr;
	*thumbSize = uiThumbSize;

	return GXVIDEO_PRSERR_OK;
}

/*
    Query Parse Video Info Buffer Needed Size

    Query the needed size of parse video information buffer for decoding thumbnail frame

    @param[out] uipBufferNeeded   the video buffer needed size

    @return
     - @b GXVIDEO_PRSERR_OK:     Query successfully.
*/
GXVID_ER GxVidFile_QueryParseVideoInfoBufSize(UINT32 *uipBufferNeeded)
{
	*uipBufferNeeded = GXVIDEO_PARSE_HEADER_BUFFER_SIZE + GXVIDEO_H26X_WORK_BUFFER_SIZE + GXVIDEO_VID_ENTRY_BUFFER_SIZE + GXVIDEO_AUD_ENTRY_BUFFER_SIZE;

	return GXVIDEO_PRSERR_OK;
}

/*
    Video Information Parser

    Parse the information of video file

    @param[in] fpReadCB       the file read callback function pointer
    @param[in] pWorkBuf       the memory address and size
    @param[in] uiTotalFileSize   the total file size of opened video file
    @param[out] pVideoInfo   the video file information
    @return
     - @b GXVIDEO_PRSERR_OK:     Parse video information successfully.
     - @b GXVID_ER:              Header Parser is failed. Reference ErrCode and check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_ParseVideoInfo64(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo)
{
	return GxVidFile_ParseVideoInfoImpl(0, fpReadCB, pWorkBuf, uiTotalFileSize, pVideoInfo);
}
//#NT#2012/08/21#Calvin Chang -end

GXVID_ER GxVidFile_ParseVideoInfo(FPGXVIDEO_READCB fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo)
{
	return GxVidFile_ParseVideoInfoImpl(fpReadCB, 0, pWorkBuf, uiTotalFileSize, pVideoInfo);
}

GXVID_ER GxVidFile_ParseVideoInfoImpl(FPGXVIDEO_READCB fpReadCB, FPGXVIDEO_READCB64 fpReadCB64, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo)
{
	ER                VidER = E_OK;
	ER                TSEndInfoResult = E_OK;
	GXVID_ER          GxVidER = GXVIDEO_PRSERR_OK;
	UINT32            uiRealfiletype = 0;
	UINT32            uiFileReadStartAddr = 0, uiFileReadSize = 0;
	UINT32            uiTSFileEndInfoStartAddr = 0, uiTSFileEndInfoSize = 0;
	UINT32            uiUsedBufSize = 0, uiTotalBufSize = 0, uiParseSize = 0;
	UINT32            uiThumbSize;
	UINT64            uiThumbStartPos;
	MEDIA_FIRST_INFO  Media1stInfoObj = {0};
	CONTAINERPARSER   *ptr = 0;
	UINT32            tsDemuxMemBufSize = 0;
	UINT32            totalSec = 0;
	MEM_RANGE         H26xWorkBuf, VidEntryBuf, AudEntryBuf;


	// Check file format
	if (fpReadCB64) {
		gGxVidfpReadCB64 = fpReadCB64;
		gGxVidfpReadCB32 = NULL;
	} else if (fpReadCB) {
		gGxVidfpReadCB32 = fpReadCB;
		gGxVidfpReadCB64 = NULL;
	}

	// Read file to data buffer
	if (!gGxVidfpReadCB64 && !gGxVidfpReadCB32) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	//uiFileReadSize = 0x40100;//GXVIDEO_PARSE_HEADER_BUFFER_SIZE; // enlarge the header read size for AVI file format 2013/09/23 Calvin
	//#NT#2018/08/27#Adam Su -begin
    //#NT# fix CARDV_680-583 parsing thumbnail buffer not enough issue
	GxVidFile_QueryParseVideoInfoBufSize(&uiTotalBufSize);
	//#NT#2018/08/27#Adam Su -begin

	// Allocate H26x working buffer for TS parser
	H26xWorkBuf.addr = pWorkBuf->addr;
	H26xWorkBuf.size = GXVIDEO_H26X_WORK_BUFFER_SIZE;
	uiUsedBufSize += H26xWorkBuf.size;

	// Allocate entry buffer
	VidEntryBuf.addr = H26xWorkBuf.addr + H26xWorkBuf.size;
	VidEntryBuf.size = GXVIDEO_VID_ENTRY_BUFFER_SIZE;
	uiUsedBufSize += VidEntryBuf.size;
	AudEntryBuf.addr = VidEntryBuf.addr + VidEntryBuf.size;
	AudEntryBuf.size = GXVIDEO_AUD_ENTRY_BUFFER_SIZE;
	uiUsedBufSize += AudEntryBuf.size;

	//uiFileReadStartAddr = H26xWorkBuf.addr + H26xWorkBuf.size;
	//uiFileReadSize = uiTotalBufSize - H26xWorkBuf.size; // need to minus H26x working buffer size, 2018/10/30, Adam (IVOT_N00017_CO-136)
	uiFileReadStartAddr = uiTSFileEndInfoStartAddr = AudEntryBuf.addr + AudEntryBuf.size;
	uiParseSize = pWorkBuf->size - uiUsedBufSize;
	if (gReadFileFromUser) {
		uiFileReadSize = gUserSetReadFileSize;
	} else {
		uiFileReadSize = 0x100000;
	}

	uiUsedBufSize += uiFileReadSize*2; //use uiFileReadSize*2 if retry
	tsDemuxMemBufSize = uiFileReadSize*2;
	uiUsedBufSize += tsDemuxMemBufSize;

#if 0
	uiFileReadSize = uiTotalBufSize - uiUsedBufSize; // left size for file reading

	//if ((H26xWorkBuf.size + uiFileReadSize) > uiTotalBufSize) {
	if ((uiUsedBufSize + uiFileReadSize) > uiTotalBufSize) {
		DBG_ERR("UsedSize(0x%x) is exceeding the scope of TotalBufSize(0x%x)\r\n",
			uiUsedBufSize, uiTotalBufSize);
	}
#endif
	//if file size is small
	if (uiFileReadSize > uiTotalFileSize) {
		uiFileReadSize = (UINT32)uiTotalFileSize;
	}

	if (gGxVidfpReadCB64) {
		VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);
	} else {
		VidER = gGxVidfpReadCB32(0, uiFileReadSize, uiFileReadStartAddr);
	}

	if (VidER != FST_STA_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_MOV;
			gRealFileType = MEDIA_PARSEINDEX_MOV;
		}
	}

	// Get AVI
#if _TODO
	ptr = MP_AviReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_AVI;
			gRealFileType = MEDIA_PARSEINDEX_AVI;
		}
	}
#endif

	// Get TS
	ptr = MP_TsReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, TS_PACKET_SIZE * TS_CHECK_COUNT);

		if (VidER > 6) { //score
			uiRealfiletype = MEDIA_PARSEINDEX_TS;
			gRealFileType = MEDIA_PARSEINDEX_TS;
		}
	}

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (uiRealfiletype == MEDIA_PARSEINDEX_TS) {
		if (uiUsedBufSize > pWorkBuf->size) {
			DBG_ERR("Work buf not enough, please set FileReadSize < 0x%x\r\n", uiFileReadSize);
			return GXVIDEO_PRSERR_SYS;
		}
	}

	// Create container parser
	if (GxVidFile_CreateContainerObj(uiRealfiletype) != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (uiRealfiletype == MEDIA_FILEFORMAT_MP4 || uiRealfiletype == MEDIA_FILEFORMAT_MOV) {
		if (gGxVidFileRContainer.SetMemBuf) {
			(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
		}
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		if (gGxVidFileRContainer.SetInfo) {
			(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSDEMUXBUF, uiFileReadStartAddr + uiFileReadSize*2, tsDemuxMemBufSize, 0);
		}
		// Only TS H.265 need this work buf
		if (gGxVidFileRContainer.SetInfo) {
			(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_H26X_WORKBUF, (UINT32)&H26xWorkBuf, 0, 0);
		}

		if (gGxVidFileRContainer.SetInfo) {
			(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_VIDENTRYBUF, VidEntryBuf.addr, VidEntryBuf.size, 0);
		}

		if (gGxVidFileRContainer.SetInfo) {
			(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_AUDENTRYBUF, AudEntryBuf.addr, AudEntryBuf.size, 0);
		}
	} else {
		DBG_ERR("invalid file_type=0x%x\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
									   uiTotalFileSize >> 32,
									   uiTotalFileSize,
									   0);

	// Parse 1st Video information
	if ((uiRealfiletype == MEDIA_FILEFORMAT_MOV) || (uiRealfiletype == MEDIA_FILEFORMAT_MP4)) {
		gGxVidFileRContainer.ParseHeader = MovReadLib_Parse1stVideo;
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
#if _TODO
		gGxVidFileRContainer.ParseHeader = MP_AVIReadLib_Parse1stVideo;
#endif
	} else if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		gGxVidFileRContainer.ParseHeader = TsReadLib_Parse1stVideo;
	}

// we already query in the begining of this function, so remove the code below, 2018/10/30, Adam (IVOT_N00017_CO-136)
#if 0
	//uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;
	//#NT#2018/08/27#Adam Su -begin
    //#NT# fix CARDV_680-583 parsing thumbnail buffer not enough issue
	GxVidFile_QueryParseVideoInfoBufSize(&uiFileReadSize);
	//#NT#2018/08/27#Adam Su -begin

	//if file size is small
	if (uiFileReadSize > uiTotalFileSize) {
		uiFileReadSize = (UINT32)uiTotalFileSize;
	}

	if (pWorkBuf->size < uiFileReadSize) {
		DBG_ERR("Work buffer size is not enough 2!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}
#endif

	if (gGxVidFileRContainer.ParseHeader) {
		VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);
	}

	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		UINT32 uiRetrySize = 0;
		uiRetrySize = uiFileReadSize * 2;

		if (Media1stInfoObj.bStillVdoPid) {
			DBG_DUMP("retry 1 time, size 0x%x\r\n", uiRetrySize);
			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(0, uiRetrySize, uiFileReadStartAddr);
				if (VidER != E_OK) {
					DBG_ERR("gGxVidfpReadCB64 fail\r\n");
					return GXVIDEO_PRSERR_SYS;
				}
			} else {
				VidER = gGxVidfpReadCB32(0, uiRetrySize, uiFileReadStartAddr);
				if (VidER != E_OK) {
					DBG_ERR("gGxVidfpReadCB32 fail\r\n");
					return GXVIDEO_PRSERR_SYS;
				}
			}
			VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiRetrySize, (void *)&Media1stInfoObj);
			if (VidER != E_OK) {
				DBG_ERR("parse 1st video fail\r\n");
				return GXVIDEO_PRSERR_SYS;
			}
		}

		if (Media1stInfoObj.bStillVdoPid) {
			DBG_ERR("Retry fail, please set lager read file size > 0x%x by user !!!\r\n", uiRetrySize);
			return GXVIDEO_PRSERR_SYS;
		}
	}

	// Parse TS file end info
	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		if (gReadFileEndFromUser) {
			uiTSFileEndInfoSize = gUserSetReadFileEndSize;
			if (uiTSFileEndInfoSize*2 > uiParseSize) {
				DBG_ERR("Work buff not enough, set gUserSetReadFileEndSize <= 0x%x !!!\r\n", uiParseSize >> 1);
				return GXVIDEO_PRSERR_SYS;
			}
		} else {
			uiTSFileEndInfoSize = 0x758; //10 packets size of TS (188bytes * 10)
		}

		if (gGxVidfpReadCB64) {
			VidER = gGxVidfpReadCB64(uiTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
		} else {
			VidER = gGxVidfpReadCB32(uiTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
		}

		if (VidER != FST_STA_OK) {
			DBG_ERR("Read File Error!!! %d \r\n", VidER);
			return GXVIDEO_PRSERR_SYS;
		}

		if (gGxVidFileRContainer.SetInfo) {
			TSEndInfoResult = (gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSENDINFO,
										   uiTSFileEndInfoStartAddr, uiTSFileEndInfoSize, 0);

			DBG_IND("TSEndInfoResult: %d \r\n", TSEndInfoResult);
		}

		if (gGxVidFileRContainer.GetInfo) {
			(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_TOTALSEC, &totalSec, 0, 0);
		}

		if (TSEndInfoResult == E_SYS) {
			if (gReadFileEndFromUser) {
				uiTSFileEndInfoSize = gUserSetReadFileEndSize*2;
			} else {
				uiTSFileEndInfoSize = uiTotalFileSize < 0x200000 ? uiTotalFileSize : 0x200000;
			}

			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(uiTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
			} else {
				VidER = gGxVidfpReadCB32(uiTotalFileSize - uiTSFileEndInfoSize, uiTSFileEndInfoSize, uiTSFileEndInfoStartAddr);
			}

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}

			if (gGxVidFileRContainer.SetInfo)
				(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_TSENDINFO,
											   uiTSFileEndInfoStartAddr, uiTSFileEndInfoSize, 0);
			if (gGxVidFileRContainer.GetInfo) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_TOTALSEC, &totalSec, 0, 0);
			}

			if (!Media1stInfoObj.dur) {
				DBG_ERR("retry fail and totalSec is 0, please set gUserSetReadFileEndSize > 0x%x\r\n", uiTSFileEndInfoSize);
				return GXVIDEO_PRSERR_SYS;
			}
		}
	}

	if (VidER != E_OK) {
		DBG_ERR("Parse Header fails. VidER=%d\r\n", VidER);

		if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
			GxVidER = Media1stInfoObj.err; // Get Error code
			return GxVidER;
		} else {
			return GXVIDEO_PRSERR_SYS;
		}

	}

	// Store Video_Info
	// Get Header if AVI file
	if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
		if (gGxVidFileRContainer.GetInfo) {
			(gGxVidFileRContainer.GetInfo)(AVIREADLIB_GETINFO_HEADER_MEDIA, (UINT32 *)&gGxVid_AVIHeadInfo, 0, 0);
		}

		pVideoInfo->pHeaderInfo = (VOID *) &gGxVid_AVIHeadInfo;
	} else {
		pVideoInfo->pHeaderInfo = 0;
	}

	uiThumbStartPos = 0;
	uiThumbSize     = 0;

	// Get Thumbnail by Callback function
	if (gGxVidGetThumbCB) {
		// Get User Data in file
		if (Media1stInfoObj.udta_size) {
			if (gGxVidfpReadCB64) {
				VidER = gGxVidfpReadCB64(Media1stInfoObj.udta_pos, Media1stInfoObj.udta_size, uiFileReadStartAddr);
			} else {
				VidER = gGxVidfpReadCB32(Media1stInfoObj.udta_pos, Media1stInfoObj.udta_size, uiFileReadStartAddr);
			}

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}

			gGxVidGetThumbCB(uiFileReadStartAddr, Media1stInfoObj.udta_size, &uiThumbStartPos, &uiThumbSize);
			uiThumbStartPos = Media1stInfoObj.udta_pos + uiThumbStartPos;
		} else {
			DBG_ERR("No User Data in file...\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	} else {
		//DBG_WRN("No Get Thumb Callback function...\r\n");

		// Get thumb from default file position
		if (gGxVidFileRContainer.GetInfo) {
			(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, (UINT32 *)&uiThumbStartPos, &uiThumbSize, 0);
		}
	}

	pVideoInfo->uiThumbOffset  = uiThumbStartPos;
	pVideoInfo->uiThumbSize    = uiThumbSize;
	pVideoInfo->uiVidWidth     = Media1stInfoObj.wid;
	pVideoInfo->uiVidHeight    = Media1stInfoObj.hei;
	//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
	pVideoInfo->uiDispWidth    = Media1stInfoObj.tkwid;
	pVideoInfo->uiDispHeight   = Media1stInfoObj.tkhei;
	//#NT#2013/10/18#Calvin Chang -end
	if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
		pVideoInfo->uiToltalSecs = totalSec;
	} else {

		/* the last sesond was rounded down, align uiToltalSecs with MOVIEPLAY_EVENT_ONE_SECOND by ceiling */
		pVideoInfo->uiToltalSecs   = Media1stInfoObj.dur / 1000;    // modified to ms for <1sec movie 2013/05/31 -Calvin
	}
	pVideoInfo->uiVidRate      = Media1stInfoObj.vidFR;
	pVideoInfo->uiVidRotate    = Media1stInfoObj.vidrotate;
	pVideoInfo->uiVidType      = Media1stInfoObj.vidtype;
	pVideoInfo->uiUserDataAddr = Media1stInfoObj.udta_pos;
	pVideoInfo->uiUserDataSize = Media1stInfoObj.udta_size;
	pVideoInfo->uiFre1DataAddr = Media1stInfoObj.fre1_pos;
	pVideoInfo->uiFre1DataSize = Media1stInfoObj.fre1_size;
	pVideoInfo->AudInfo.uiChs  = Media1stInfoObj.audChs;
	pVideoInfo->AudInfo.uiSR   = Media1stInfoObj.audSR;
	pVideoInfo->AudInfo.uiType = Media1stInfoObj.audtype;
	//#NT#2015/04/07#Isiah Chang -begin
	//Retrieve 1st I frame info.
	pVideoInfo->ui1stFramePos  = Media1stInfoObj.pos;
	pVideoInfo->ui1stFrameSize = Media1stInfoObj.size;
	//#NT#2015/04/07#Isiah Chang -end

	if (gGxVidFileRContainer.GetInfo) {
		// video frame
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VFRAMENUM, &pVideoInfo->uiTotalFrames, 0, 0);
		// SPS/PPS data
		switch (Media1stInfoObj.vidtype){
		case MEDIAPLAY_VIDEO_H264:
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_H264DESC, &pVideoInfo->uiH264DescAddr, &pVideoInfo->uiH264DescSize, 0);
			break;

		case MEDIAPLAY_VIDEO_H265:
			if (uiRealfiletype == MEDIA_FILEFORMAT_TS) {
				(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_H265DESC, &pVideoInfo->uiH264DescAddr, &pVideoInfo->uiH264DescSize, 0);
			}else{
			    pVideoInfo->uiH264DescAddr = (UINT32)&gGxH265VidDesc[0];
		        VidER = _GxVideoFile_GetH265Desc(Media1stInfoObj.vidfrm_pos[0], pVideoInfo->uiH264DescAddr, &pVideoInfo->uiH264DescSize);
		        if (VidER != E_OK) {
		            DBG_ERR("Get H265Desc Error!!\r\n");
		            return GXVIDEO_PRSERR_SYS;
		        }
			}
			break;
		}

		(gGxVidFileRContainer.GetInfo)(MOVREADLIB_GETINFO_CMDATE, &pVideoInfo->uiMovCreateTime, &pVideoInfo->uiMovModificationTime, 0);

		//#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOSCRA, (UINT32 *)&pVideoInfo->uiScraOffset, &pVideoInfo->uiScraSize, 0);
		//#NT#2013/12/17#Calvin Chang -end
	}

	return GxVidER;
}

/*
    Video Information & Entry Parser

    Parse the information and entry table of video file

    @param[in] fpReadCB       the file read callback function pointer
    @param[in] pWorkBuf       the memory address and size
    @param[in] uiTotalFileSize   the total file size of opened video file
    @param[out] pVideoInfo   the video file information
    @return
     - @b GXVIDEO_PRSERR_OK:     Parse video information successfully.
     - @b GXVID_ER:              Header Parser is failed. Reference ErrCode and check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_ParseVideoInfoEntry(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo)
{
	ER                VidER = E_OK;
	GXVID_ER          GxVidER = GXVIDEO_PRSERR_OK;
	UINT32            uiRealfiletype = 0;
	UINT32            uiFileReadStartAddr;
	UINT32            uiFileReadSize;
	UINT32            uiThumbSize;
	UINT64            uiThumbStartPos;
	MEDIA_FIRST_INFO  Media1stInfoObj = {0};
	CONTAINERPARSER   *ptr = 0;


	// Check file format
	gGxVidfpReadCB64 = fpReadCB;
	// Read file to data buffer
	if (!gGxVidfpReadCB64) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiFileReadStartAddr = pWorkBuf->addr;

	uiFileReadSize = 0x200000;//40100;//GXVIDEO_PARSE_HEADER_BUFFER_SIZE; // enlarge the header read size for AVI file format 2013/09/23 Calvin

	if (pWorkBuf->size < uiFileReadSize) {
		DBG_ERR("Work buffer size is not enough!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	uiFileReadSize = 0x40100;

	VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);

	if (VidER != FST_STA_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		return GXVIDEO_PRSERR_SYS;
	}

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_MOV;
		}
	}

	// Get AVI
#if _TODO
	ptr = MP_AviReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_AVI;
		}
	}
#endif

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Create container parser
	if (GxVidFile_CreateContainerObj(uiRealfiletype) != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetMemBuf) {
		(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
									   uiTotalFileSize >> 32,
									   uiTotalFileSize,
									   0);

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_VIDENTRYBUF,
									   uiFileReadStartAddr + 0x40100,
									   0x200000 - uiFileReadSize,
									   0);

	// Parse 1st Video information
	if ((uiRealfiletype == MEDIA_FILEFORMAT_MOV) || (uiRealfiletype == MEDIA_FILEFORMAT_MP4)) {
		gGxVidFileRContainer.ParseHeader = MovReadLib_ParseVideoInfo;
	}

	uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;

	if (gGxVidFileRContainer.ParseHeader) {
		VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);
	}

	if (VidER != E_OK) {
		DBG_ERR("Parse Header fails...\r\n");

		if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
			GxVidER = Media1stInfoObj.err; // Get Error code
			return GxVidER;
		} else {
			return GXVIDEO_PRSERR_SYS;
		}

	}

	// Store Video_Info
	// Get Header if AVI file
	if (uiRealfiletype == MEDIA_FILEFORMAT_AVI) {
		if (gGxVidFileRContainer.GetInfo) {
			(gGxVidFileRContainer.GetInfo)(AVIREADLIB_GETINFO_HEADER_MEDIA, (UINT32 *)&gGxVid_AVIHeadInfo, 0, 0);
		}

		pVideoInfo->pHeaderInfo = (VOID *) &gGxVid_AVIHeadInfo;
	} else {
		pVideoInfo->pHeaderInfo = 0;
	}

	uiThumbStartPos = 0;
	uiThumbSize     = 0;

	// Get Thumbnail by Callback function
	if (gGxVidGetThumbCB) {
		// Get User Data in file
		if (Media1stInfoObj.udta_size) {
			VidER = gGxVidfpReadCB64(Media1stInfoObj.udta_pos, Media1stInfoObj.udta_size, uiFileReadStartAddr);

			if (VidER != FST_STA_OK) {
				DBG_ERR("Read File Error!!! %d \r\n", VidER);
				return GXVIDEO_PRSERR_SYS;
			}

			gGxVidGetThumbCB(uiFileReadStartAddr, Media1stInfoObj.udta_size, &uiThumbStartPos, &uiThumbSize);
			uiThumbStartPos = Media1stInfoObj.udta_pos + uiThumbStartPos;
		} else {
			DBG_ERR("No User Data in file...\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	} else {
		//DBG_WRN("No Get Thumb Callback function...\r\n");

		// Get thumb from default file position
		if (gGxVidFileRContainer.GetInfo) {
			(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOTHUMB, (UINT32 *)&uiThumbStartPos, &uiThumbSize, 0);
		}
	}

	pVideoInfo->uiThumbOffset  = uiThumbStartPos;
	pVideoInfo->uiThumbSize    = uiThumbSize;
	pVideoInfo->uiVidWidth     = Media1stInfoObj.wid;
	pVideoInfo->uiVidHeight    = Media1stInfoObj.hei;
	//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
	pVideoInfo->uiDispWidth    = Media1stInfoObj.tkwid;
	pVideoInfo->uiDispHeight   = Media1stInfoObj.tkhei;
	//#NT#2013/10/18#Calvin Chang -end
	pVideoInfo->uiToltalSecs   = Media1stInfoObj.dur / 1000; // modified to ms for <1sec movie 2013/05/31 -Calvin
	pVideoInfo->uiVidRate      = Media1stInfoObj.vidFR;
	pVideoInfo->uiVidRotate    = Media1stInfoObj.vidrotate;
	pVideoInfo->uiVidType      = Media1stInfoObj.vidtype;
	pVideoInfo->uiUserDataAddr = Media1stInfoObj.udta_pos;
	pVideoInfo->uiUserDataSize = Media1stInfoObj.udta_size;
	pVideoInfo->uiFre1DataAddr = Media1stInfoObj.fre1_pos;
	pVideoInfo->uiFre1DataSize = Media1stInfoObj.fre1_size;
	pVideoInfo->AudInfo.uiChs  = Media1stInfoObj.audChs;
	pVideoInfo->AudInfo.uiSR   = Media1stInfoObj.audSR;
	pVideoInfo->AudInfo.uiType = Media1stInfoObj.audtype;
	//#NT#2015/04/07#Isiah Chang -begin
	//Retrieve 1st I frame info.
	pVideoInfo->ui1stFramePos  = Media1stInfoObj.pos;
	pVideoInfo->ui1stFrameSize = Media1stInfoObj.size;
	//#NT#2015/04/07#Isiah Chang -end

	if (gGxVidFileRContainer.GetInfo) {
		// video frame
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VFRAMENUM, &pVideoInfo->uiTotalFrames, 0, 0);
		// SPS/PPS data
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_H264DESC, &pVideoInfo->uiH264DescAddr, &pVideoInfo->uiH264DescSize, 0);

		(gGxVidFileRContainer.GetInfo)(MOVREADLIB_GETINFO_CMDATE, &pVideoInfo->uiMovCreateTime, &pVideoInfo->uiMovModificationTime, 0);

		//#NT#2013/12/17#Calvin Chang#Add for Screennail -begin
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VIDEOSCRA, (UINT32 *)&pVideoInfo->uiScraOffset, &pVideoInfo->uiScraSize, 0);
		//#NT#2013/12/17#Calvin Chang -end
	}

	return GxVidER;
}


/*
    Get I Frame Offset & Size.

    Get I Frame Offset & Size

    @param[in] uiGetCnt       Get count of I-Frames
    @param[in] uiGetNum       Get number of I-Frames
    @param[out] pOffset       I-Frame offset
    @param[out] pSize         I-Frame size
    @return
     - @b GXVIDEO_PRSERR_OK:     Parse video information successfully.
     - @b GXVID_ER:              Header Parser is failed. Reference ErrCode and check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_GetVideoIFrameOffsetSize(UINT32 uiGetCnt, UINT32 uiGetNum, UINT64 *pOffset, UINT32 *pSize)
{
	UINT32 uiIFrameCnt = 0, uiTotalVidFrm = 0, uiIFrameIdx = 0, uiSkipI = 0, uiGetFrameIdx = 0;

	if (uiGetCnt == 0 || uiGetNum == 0) {
		DBG_ERR("uiGetCnt is zero!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.GetInfo) {
		// Check I Frame count
		(gGxVidFileRContainer.GetInfo)(MOVREADLIB_GETINFO_IFRAMETOTALCOUNT,
									   &uiIFrameCnt,
									   0,
									   0);
		if (!uiIFrameCnt) {
			DBG_ERR("I Frame count is zero!!\r\n");
			return GXVIDEO_PRSERR_SYS;
		}

		// Get totoal frame number
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_VFRAMENUM, &uiTotalVidFrm, 0, 0);

		// Get I frame index
		uiGetFrameIdx = (uiTotalVidFrm / uiGetCnt) * (uiGetNum - 1);

		if (uiGetFrameIdx > uiTotalVidFrm) {
			uiIFrameIdx = uiTotalVidFrm;
		}

		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_PREVIFRAMENUM,
									   &uiGetFrameIdx,
									   &uiIFrameIdx,
									   &uiSkipI);

		// Get I frame offset & size
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE, &uiIFrameIdx, (UINT32 *)pOffset, pSize);

	} else {
		DBG_ERR("Video File Container is zero!!\r\n");
		return GXVIDEO_PRSERR_SYS;
	}

	return GXVIDEO_PRSERR_OK;

}

GXVID_ER GxVidFile_SetParam(GXVIDFILE_PARAM Param, UINT32 Value)
{
	GXVID_ER r = GXVIDEO_PRSERR_OK;

	switch (Param) {
	case GXVIDFILE_PARAM_MAX_W:
		gGxVidFileObj.max_w = Value;
		break;
	case GXVIDFILE_PARAM_MAX_H:
		gGxVidFileObj.max_h = Value;
		break;
	case GXVIDFILE_PARAM_IS_DECRYPT:
		gbDecrypt = (Value == FALSE)? FALSE : TRUE;
		break;
	case GXVIDFILE_PARAM_DECRYPT_KEY:
		{
			UINT32 idx = 0;
			UINT8 *key = (UINT8*)Value;

			for (idx=0; idx<32; idx++) {
				gDecryptKey[idx] = key[idx];
			}
		}
		break;
	case GXVIDFILE_PARAM_DECRYPT_MODE:
		gDecryptMode = Value; //CRYPTO_MODE_AES128; CRYPTO_MODE_AES256
		break;
	case GXVIDFILE_PARAM_DECRYPT_POS:
		gDecryptPos = Value; // refer to GXVIDEO_DECRYPT_TYPE
		break;
	case GXVIDFILE_PARAM_FILEREADSIZE:
		gUserSetReadFileSize = Value;
		gReadFileFromUser = TRUE;
	break;
	case GXVIDFILE_PARAM_FILEENDREADSIZE:
		gUserSetReadFileEndSize = Value;
		gReadFileEndFromUser = TRUE;
	break;
	default:
		DBG_ERR("unknown param (%d)\r\n", Param);
		r = GXVIDEO_PRSERR_SYS;
		break;
	}
	return r;
}

GXVID_ER GxVidFile_GetParam(GXVIDFILE_PARAM Param, UINT32 *pValue)
{
	GXVID_ER r = GXVIDEO_PRSERR_OK;

	switch (Param) {
	case GXVIDFILE_PARAM_MAX_W:
		*pValue = gGxVidFileObj.max_w;
		break;
	case GXVIDFILE_PARAM_MAX_H:
		*pValue = gGxVidFileObj.max_h;
		break;
	case GXVIDFILE_PARAM_IS_DECRYPT:
		*pValue = gbDecrypt;
		break;
	case GXVIDFILE_PARAM_DECRYPT_KEY:
		{
			UINT32 idx = 0;
			UINT8 *key = (UINT8 *)pValue;

			for (idx=0; idx<32; idx++) {
				key[idx] = gDecryptKey[idx];
			}
		}
		break;
	case GXVIDFILE_PARAM_DECRYPT_MODE:
		*pValue = gDecryptMode; //CRYPTO_MODE_AES128; CRYPTO_MODE_AES256
		break;
	case GXVIDFILE_PARAM_DECRYPT_POS:
		*pValue = gDecryptPos; // refer to GXVIDEO_DECRYPT_TYPE
		break;
	default:
		DBG_ERR("unknown param (%d)\r\n", Param);
		r = GXVIDEO_PRSERR_SYS;
		break;
	}
	return r;
}

static BOOL GxVideoFileDecrypto(UINT8 *PlanText,UINT8 *uiKey,UINT8 *CypherText, GXVIDEO_DECRYPT_MODE uiMode)
{
#if defined(__FREERTOS)

	if(crypto_open() != E_OK){
		DBG_ERR("crypto_open() fail\r\n");
		return FALSE;
	}

	crypto_open();
	crypto_setConfig(CRYPTO_CONFIG_ID_MODE, gDecryptMode);
	crypto_setConfig(CRYPTO_CONFIG_ID_TYPE, CRYPTO_TYPE_DECRYPT);
	crypto_setConfig(CRYPTO_CONFIG_ID_OPMODE, CRYPTO_OPMODE_EBC);
	crypto_setConfig(CRYPTO_CONFIG_ID_KEY, CRYPTO_KEY_NORMAL);
	crypto_setKey(uiKey);
	crypto_setInput(PlanText);
	crypto_pio_enable();
	crypto_getOutput(CypherText);

	if(crypto_close() != E_OK){
		DBG_ERR("crypto_close() fail\r\n");
		return FALSE;
	}
	return TRUE;

#else
	BOOL ret = TRUE;
    struct stat cryptodev = {0};
    char dev_name[] = "/dev/crypto";

	if (stat(dev_name, &cryptodev) == 0) {

		if ((crypt_fd = open(dev_name, O_RDWR, 0) ) < 0) {

			DBG_ERR("open %s fail\r\n", dev_name);
			return FALSE;
		}


		crypt_sess.cipher = CRYPTO_AES_ECB;
		crypt_sess.keylen = (gDecryptMode == GXVIDEO_DECRYPT_MODE_AES128)? 16 : 32;
		crypt_sess.key = (void*)uiKey;

		if (ioctl(crypt_fd, CIOCGSESSION, &crypt_sess)) {

			DBG_ERR("ioctl(CIOCGSESSION) fail\r\n");
			ret = FALSE;
			goto exit;
		}


		crypt_cryp.ses = crypt_sess.ses;
		crypt_cryp.len = 16;
		crypt_cryp.src = (void*)PlanText;
		crypt_cryp.dst = (void*)CypherText;
		crypt_cryp.iv = NULL;
		crypt_cryp.op = COP_DECRYPT;

		if (ioctl(crypt_fd, CIOCCRYPT, &crypt_cryp)) {
			DBG_ERR("ioctl(CIOCCRYPT)\r\n");
			ret = FALSE;
			goto exit;
		}



		if (ioctl(crypt_fd, CIOCFSESSION, &(crypt_sess.ses))) {
			DBG_ERR("ioctl(CIOCFSESSION) fail\r\n");
			ret = E_SYS;
			goto exit;
		}


exit:
		if (close(crypt_fd)) {
			DBG_ERR("close(crypt_fd) fail\r\n");
			ret = E_SYS;
		}

		crypt_fd = -1;

	} else {
		DBG_ERR("%s not exist\r\n", dev_name);
		return FALSE;
	}
	return ret;

#endif
}

GXVID_ER GxVideoFile_Decrypto(UINT32 addr, UINT32 size)
{

//	UINT32 temp_time = Perf_GetCurrent(), temp_time2, diff;
	UINT32 temp_time, temp_time2, temp_time3, diff;
	UINT8 gen_key[32] = {0}; //gen key
	UINT8 *key;
	UINT8 *pos; //gen key pos
	UINT32 cnt, i;
	UINT32 key_len = 0;
	UINT32 blk_len = 0;
	GXVIDEO_DECRYPT_MODE mode;

	GXVID_ER r = E_OK;

	vos_perf_mark(&temp_time);

	//trans
	mode = gDecryptMode;
	if (mode == GXVIDEO_DECRYPT_MODE_AES128) {
		key_len = 16;
		blk_len = 16;
	} else if (mode == GXVIDEO_DECRYPT_MODE_AES256) {
		key_len = 32;
		blk_len = 16;
	}
	key = gDecryptKey;
	if (key == NULL) {
		DBG_DUMP("^G crypto key NULL\r\n");
		return r;
	}
	//dump
	if (0) {
		DBG_DUMP("^G EN adr=0x%lx, siz=0x%lx.\r\n", addr, size);
		DBG_DUMP("^G key\r\n");
		for (i = 0; i < key_len; i++) {
			DBG_DUMP("^G %x ", gDecryptKey[i]);
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
	memcpy(&gen_key, key, key_len);
	//dump
	if (0) {
		DBG_DUMP("^G gen key\r\n");
		for (i=0; i<key_len; i++) {
			DBG_DUMP("0x%x ", gen_key[i]);
		}
		DBG_DUMP("\r\n");
	}

	//action
//	temp_time2 = Perf_GetCurrent();
	vos_perf_mark(&temp_time2);

	for (i = 0; i < cnt; i++) {
		pos = (UINT8 *)(addr + i*blk_len);
		GxVideoFileDecrypto(pos, gen_key, pos, mode);
		//dump
		if (0) {
			UINT32 dp;
			for (dp = 0; dp < blk_len; dp++) {
				if (dp%blk_len == 0) {
					DBG_DUMP("\r\n");
				}
				DBG_DUMP("0x%02X ",*(pos+dp));
			}
			DBG_DUMP("\r\n");
		}
	}

	if (0) {
//		diff = (Perf_GetCurrent() - temp_time2) / 1000;
		vos_perf_mark(&temp_time3);
		diff = (temp_time3 - temp_time2) / 1000;

		DBG_DUMP("^C AES_ENCRYP2 %dms\r\n", diff);
	}

	//action (decryt)
	if (0) {
//		temp_time2 = Perf_GetCurrent();
		vos_perf_mark(&temp_time2);

		for (i = 0; i < cnt; i++) {
			pos = (UINT8 *)(addr + i*blk_len);
			GxVideoFileDecrypto(pos, gen_key, pos, mode);
			//dump
			if (0) {
				UINT32 dp;
				for (dp = 0; dp < blk_len; dp++) {
					if (dp%blk_len == 0) {
						DBG_DUMP("\r\n");
					}
					DBG_DUMP("0x%02X ",*(pos+dp));
				}
				DBG_DUMP("\r\n");
			}
		}

		if (0) {
//			diff = (Perf_GetCurrent() - temp_time2) / 1000;
			vos_perf_mark(&temp_time3);
			diff = (temp_time3 - temp_time2) / 1000;

			DBG_DUMP("^C AES_DECRYP2 %dms\r\n", diff);
		}
	}

	//store
	gDecryptAddr = addr;
	gDecryptSize = size;
	//dump
	if (0)
	{
		DBG_DUMP("EN addr %d size %d\r\n", gDecryptAddr, gDecryptSize);
	}
	DBG_IND("^G Encrypto done\r\n");

	if (0)
	{
//		diff = (Perf_GetCurrent() - temp_time) / 1000;
		vos_perf_mark(&temp_time3);
		diff = (temp_time3 - temp_time) / 1000;
		DBG_DUMP("^C EnCrypto %dms\r\n", diff);
	}

	return r;
}


GXVID_ER GxVidFile_GetStreamByTimeOffset(FPGXVIDEO_READCB64 fpReadCB64, GXVIDFILE_INTERCEPT_PARM *param, GXVIDFILE_INTERCEPT_RESULT *ret)
{
	ER                VidER = E_OK;
	CONTAINERPARSER   *ptr = 0;
	UINT32            uiRealfiletype = 0, uifrm = 0, uiPreDecNum = 0;
	UINT32            uiIFrmIdx = 0, uiTargetFrmIdx = 0, uiGOP = 0, uiVdoFR = 0;
	UINT32            uiBsSum = 0;
	UINT32            output_addr = 0;
	UINT64            bs_offset = 0;
	UINT32            bs_size = 0;
	UINT32            uiFileReadStartAddr, uiFileReadSize;
	MEM_RANGE                FileHeader = {0};
	MEDIA_FIRST_INFO  Media1stInfoObj = {0};
	MEDIA_HEADINFO               header_info;
	UINT32 DescAddr = 0, DescLen = 0;
	UINT32 decfrmnum = 0;
	GXVIDFILE_INTERCEPT_PARM     *intercept_param;
	GXVIDFILE_INTERCEPT_RESULT   *intercept_result;

	intercept_param = param;
	intercept_result = ret;

#ifdef INTERCEPT_TEST
	MEM_RANGE                FileTest = {0};
#else
	// check user buffer
	if (intercept_param->bs_buffer == NULL) {
        DBG_ERR("user buffer NULL\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
        return GXVIDEO_PRSERR_SYS;
	}
#endif

	/// new buffer
	FileHeader.size = 0x100000;
	if (_GxVidFile_NewHeaderBuf(&FileHeader.addr, &FileHeader.size) != GXVIDEO_PRSERR_OK) {
        DBG_ERR("New header buffer fail\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
        return GXVIDEO_PRSERR_SYS;
	}
	// Allocate Memory
	uiFileReadStartAddr = FileHeader.addr;

	// Check file format
	if (fpReadCB64) {
		gGxVidfpReadCB64 = fpReadCB64;
		gGxVidfpReadCB32 = NULL;
	} /*else if (fpReadCB) {
		gGxVidfpReadCB32 = fpReadCB;
		gGxVidfpReadCB64 = NULL;
	}*/
	// Read file to data buffer
	if (!gGxVidfpReadCB64 && !gGxVidfpReadCB32) {
		DBG_ERR("File Read Callback fpReadCB is NULL!!\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	uiFileReadSize = 0x100000; // enlarge the header read size for AVI file format 2013/09/23 Calvin

	//if file size is small
	if (uiFileReadSize > guiGxVidTotalFileSize) {
		uiFileReadSize = (UINT32)guiGxVidTotalFileSize;
	}

	if (gGxVidfpReadCB64) {
		VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);
	} else {
		VidER = gGxVidfpReadCB32(0, uiFileReadSize, uiFileReadStartAddr);
	}

	if (VidER != FST_STA_OK) {
		DBG_ERR("Read File Error!!! %d \r\n", VidER);
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	// Get MOV
	ptr = MP_MovReadLib_GerFormatParser();
	if (ptr) {
		VidER = ptr->Probe(uiFileReadStartAddr, uiFileReadSize);

		if (VidER == E_OK) {
			uiRealfiletype = MEDIA_PARSEINDEX_MOV;
			gRealFileType = MEDIA_PARSEINDEX_MOV;
		} else {
			DBG_ERR("Support MOV or MP4 only\r\n");
			intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
			return GXVIDEO_PRSERR_SYS;
		}
	}

	if (!uiRealfiletype) {
		DBG_ERR("File type get error!!\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	// Create container parser
	VidER = GxVidFile_CreateContainerObj(uiRealfiletype);

	if (VidER != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	// Setup container parser
	if (gGxVidFileRContainer.Initialize) {
		VidER = (gGxVidFileRContainer.Initialize)();
	}

	if (VidER != E_OK) {
		DBG_ERR("File Container Initialized fails...\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	if (gGxVidFileRContainer.SetMemBuf) {
		(gGxVidFileRContainer.SetMemBuf)(uiFileReadStartAddr, uiFileReadSize);
	}

	if (gGxVidFileRContainer.SetInfo)
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_FILESIZE,
									   guiGxVidTotalFileSize >> 32,
									   guiGxVidTotalFileSize,
									   0);

	// Parse 1st Video information
	//if ((uiRealfiletype == MEDIA_FILEFORMAT_MOV) || (uiRealfiletype == MEDIA_FILEFORMAT_MP4)) {
	//	gGxVidFileRContainer.ParseHeader = MovReadLib_Parse1stVideo;
	//}

	uiFileReadSize = GXVIDEO_PARSE_HEADER_BUFFER_SIZE;

	//if file size is small
	if (uiFileReadSize > guiGxVidTotalFileSize) {
		uiFileReadSize = (UINT32)guiGxVidTotalFileSize;
	}

	//if (gGxVidFileRContainer.ParseHeader) {
	//	VidER = (gGxVidFileRContainer.ParseHeader)(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);
	//}
	VidER = MovReadLib_Parse1stVideo(uiFileReadStartAddr, uiFileReadSize, (void *)&Media1stInfoObj);

	if (VidER != E_OK) {
		DBG_ERR("Parse Header fails...\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	//Read last pre-decode frame position + size to buffer
	// Check Pre-decode number
	for (uifrm = 0; uifrm < MEDIAREADLIB_PREDEC_FRMNUM; uifrm++) {
		if (Media1stInfoObj.vidfrm_size[uifrm] == 0) {
			break;
		}
		uiPreDecNum++;
	}

	if (uiPreDecNum > 0) {
		uiFileReadSize = (Media1stInfoObj.vidfrm_pos[uiPreDecNum - 1] + Media1stInfoObj.vidfrm_size[uiPreDecNum - 1]);
	} else {
		DBG_ERR("PreDecNum is zero!!\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	if (uiFileReadSize > GXVIDFILE_PREFRAME_USEDSIZE) {
		DBG_ERR("File read buffer size for video frame is not enough!!\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}

	if (uiRealfiletype != MEDIA_FILEFORMAT_TS) {
		if (gGxVidfpReadCB64) {
			VidER = gGxVidfpReadCB64(0, uiFileReadSize, uiFileReadStartAddr);
		} else {
			VidER = gGxVidfpReadCB32(0, uiFileReadSize, uiFileReadStartAddr);
		}

		if (VidER != FST_STA_OK) {
			DBG_ERR("Read File Error!!! %d \r\n", VidER);
			intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
			return GXVIDEO_PRSERR_SYS;
		}
	}

    // Configure the MP_VDODEC_1STV_INFO
    intercept_result->width = Media1stInfoObj.wid;
	intercept_result->height = Media1stInfoObj.hei;

#ifdef INTERCEPT_TEST
	/// new buffer
	FileTest.size = 0x200000;
	if (_GxVidFile_NewTestBuf(&FileTest.addr, &FileTest.size) != GXVIDEO_PRSERR_OK) {
		DBG_ERR("New header buffer fail\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
		return GXVIDEO_PRSERR_SYS;
	}
#endif
    if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H264) {
		intercept_result->image_type = GXVIDFILE_IMAGE_TYPE_H264;
        if (gGxVidFileRContainer.GetInfo)
            (gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_H264DESC, &DescAddr, &DescLen, 0);
#ifdef INTERCEPT_TEST
		memcpy((INT8 *)FileTest.addr, (UINT8 *)DescAddr, DescLen);
		uiBsSum += DescLen;
#else
		memcpy(intercept_param->bs_buffer, (UINT8 *)DescAddr, DescLen);
		uiBsSum += DescLen;
#endif
    }
    else if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H265) {
		intercept_result->image_type = GXVIDFILE_IMAGE_TYPE_H265;
		VidER = _GxVideoFile_GetH265Desc(Media1stInfoObj.vidfrm_pos[0], (UINT32)&gGxH265VidDesc[0], &DescLen);
		if (VidER != E_OK) {
			DBG_ERR("Get H265Desc Error!!\r\n");
			intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
			return GXVIDEO_PRSERR_SYS;
		}
		DescAddr  = (UINT32)&gGxH265VidDesc[0];
    }

	// free header buffer
    if (gGxPoolHeader_va) {
		if (_GxVidFile_FreeBufPool(&gGxPoolHeader_pa, &gGxPoolHeader_va, &gGxPoolHeader_size) != GXVIDEO_PRSERR_OK) {
			DBG_ERR("free header buf fail\r\n");
	        return GXVIDEO_PRSERR_SYS;
	    }
    } else {
		DBG_WRN("no need to free header buf\r\n");
    }

	// allocate memory
	if (_GxVidFile_AllocMem(Media1stInfoObj) != GXVIDEO_PRSERR_OK) {
		DBG_ERR("alloc mem error\r\n");
		return GXVIDEO_PRSERR_SYS;
	}


	// Set entry table
	if (gGxVidFileRContainer.SetInfo) {
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_VIDENTRYBUF,
										gGxMem_Map[GXVIDEO_MEM_VDOENTTBL].addr,
										gGxMem_Map[GXVIDEO_MEM_VDOENTTBL].size,
										0);
		(gGxVidFileRContainer.SetInfo)(MEDIAREADLIB_SETINFO_AUDENTRYBUF,
										gGxMem_Map[GXVIDEO_MEM_AUDENTTBL].addr,
										gGxMem_Map[GXVIDEO_MEM_AUDENTTBL].size,
										0);
	}
	memset(&header_info, 0, sizeof(header_info));
	header_info.checkID = gGxVidFileRContainer.checkID;

	// parse full header
	_GxVidFile_ParseFullHeader(&header_info);

	uiGOP = _GxVidFil_GetVdoGOP();
	uiVdoFR = Media1stInfoObj.vidFR;
#ifdef INTERCEPT_TEST
	intercept_param->time_offset_ms = TEST_TIME_OFFSER_MS;
#endif
	uiTargetFrmIdx = intercept_param->time_offset_ms/(1000/uiVdoFR);
	uiIFrmIdx = (uiTargetFrmIdx/uiGOP)*uiGOP;

	// Get Bitstream
	/// new buffer
	FileHeader.size = 0x100000;
	if (_GxVidFile_NewHeaderBuf(&FileHeader.addr, &FileHeader.size) != GXVIDEO_PRSERR_OK) {
        DBG_ERR("New header buffer fail\r\n");
		intercept_result->return_type = GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST;
        return GXVIDEO_PRSERR_SYS;
	}
	// Allocate Memory
	uiFileReadStartAddr = FileHeader.addr;

	UINT32 i = 0;
	UINT32 uiFrameEndIdx = uiIFrmIdx+uiGOP;
	for (i=uiIFrmIdx; i<uiFrameEndIdx; i++) {
		(gGxVidFileRContainer.GetInfo)(MEDIAREADLIB_GETINFO_GETVIDEOPOSSIZE,
										&i,
										(UINT32 *)&bs_offset,
										&bs_size);

		if (gGxVidfpReadCB64) {
			VidER = gGxVidfpReadCB64(bs_offset, bs_size, uiFileReadStartAddr);
		} else {
			VidER = gGxVidfpReadCB32(bs_offset, bs_size, uiFileReadStartAddr);
		}
		UINT8 *bsdata = (UINT8 *)uiFileReadStartAddr;
		*bsdata = 0x0;
		*(bsdata+1) = 0x0;
		*(bsdata+2) = 0x0;
		*(bsdata+3) = 0x1;

#ifdef INTERCEPT_TEST
		output_addr = FileTest.addr + uiBsSum;
#else
		output_addr = (UINT32)intercept_param->bs_buffer + uiBsSum;
#endif
		memcpy((INT8 *)output_addr, (UINT8 *)uiFileReadStartAddr, bs_size);
		uiBsSum += bs_size;
#ifdef INTERCEPT_TEST
		if (uiBsSum > FileTest.size) {
			DBG_ERR("buffer size not enough bs_size 0x%x\r\n", uiBsSum);
			intercept_result->return_type = GXVIDFILE_RETURN_TYPE_BUFFER_NOT_ENOUGH;
			return GXVIDEO_PRSERR_SYS;
		}
#else
		if (uiBsSum > intercept_param->bs_size) {
			DBG_ERR("buffer size not enough bs_size 0x%x\r\n", uiBsSum);
			intercept_result->return_type = GXVIDFILE_RETURN_TYPE_BUFFER_NOT_ENOUGH;
			return GXVIDEO_PRSERR_SYS;
		}
#endif
	}

    if (Media1stInfoObj.vidtype == MEDIAPLAY_VIDEO_H265) {
#ifdef INTERCEPT_TEST
		output_addr = FileTest.addr;
#else
		output_addr = (UINT32)intercept_param->bs_buffer;
#endif
		memcpy((UINT8 *)output_addr, (UINT8 *)DescAddr, DescLen);
		decfrmnum = 1; // Not support B-frame

		for (uifrm = 0; uifrm < decfrmnum; uifrm++) {
			if (_GxVideoFile_ChkH265IfrmBS(DescLen, (UINT8*)output_addr) != E_OK) {
				DBG_ERR("Get H265 I-frame Error!!\r\n");
				return GXVIDEO_PRSERR_SYS;
			}
		}
    }

	// free header buffer
    if (gGxPoolHeader_va) {
		if (_GxVidFile_FreeBufPool(&gGxPoolHeader_pa, &gGxPoolHeader_va, &gGxPoolHeader_size) != GXVIDEO_PRSERR_OK) {
			DBG_ERR("free header buf fail\r\n");
	        return GXVIDEO_PRSERR_SYS;
	    }
    } else {
		DBG_WRN("no need to free header buf\r\n");
    }

#ifdef INTERCEPT_TEST
	//dump bs data
	static BOOL bFirst = TRUE;
	static FST_FILE fp = NULL;
	if (bFirst) {
		char sFileName[260];
		DBG_DUMP("[GxVideoFile] dump intercept bs\r\n");
		snprintf(sFileName, sizeof(sFileName), "A:\\vdo_bs_intercept.dat");
		fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fp != NULL) {
			if (FileSys_WriteFile(fp, (UINT8 *)FileTest.addr, &uiBsSum, 0, NULL) != FST_STA_OK) {
				DBG_ERR("[GxVideoFile] dump intercept bs err, write file fail\r\n");
			} else {
				DBG_DUMP("[GxVideoFile] write file ok\r\n");
			}
			FileSys_CloseFile(fp);
		} else {
			DBG_ERR("[GxVideoFile] dump intercept bs err, invalid file handle\r\n");
		}

		bFirst = FALSE;
	}

	// free header buffer
	if (gGxPoolTest_va) {
		if (_GxVidFile_FreeBufPool(&gGxPoolTest_pa, &gGxPoolTest_va, &gGxPoolTest_size) != GXVIDEO_PRSERR_OK) {
			DBG_ERR("free header buf fail\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	} else {
		DBG_WRN("no need to free header buf\r\n");
	}
#endif

	// free main memory
	if (gGxPoolMain_va) {
		if (_GxVidFile_FreeBufPool(&gGxPoolMain_pa, &gGxPoolMain_va, &gGxPoolMain_size) != GXVIDEO_PRSERR_OK) {
			DBG_ERR("free main buf fail\r\n");
			return GXVIDEO_PRSERR_SYS;
		}
	} else {
		DBG_WRN("no need to free main buf\r\n");
	}

	intercept_result->return_type = GXVIDFILE_RETURN_TYPE_SUCCESS;
	intercept_result->length = uiBsSum;
	intercept_result->offset = uiTargetFrmIdx - uiIFrmIdx;
	DBG_DUMP("return_type %d\r\n", intercept_result->return_type);
	DBG_DUMP("data len %x\r\n", intercept_result->length);
	DBG_DUMP("image type %d, WxH %dx%d, offset %d\r\n", intercept_result->image_type, intercept_result->width, intercept_result->height,intercept_result->offset);

	return GXVIDEO_PRSERR_OK;
}

//@}


