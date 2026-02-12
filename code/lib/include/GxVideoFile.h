/**
    Header file of Video File Play API

    Exported API of Video 1st-Frame and Thumbnail Play.

    @file       GxVideoFile.h
    @ingroup    mILIBGXVIDFILE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _GX_VIDEOFILE_H_
#define _GX_VIDEOFILE_H_

//#include "Type.h"
#include "kwrap/type.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <stdio.h>
#endif

#if _TODO
#include "GxImage.h"
#else

//--------------- temp-myself define ------------------
#define MAX_PLANE_NUM                 3            ///<  3 if for YUV planner

typedef enum _GX_IMAGE_PIXEL_FMT_ {
	GX_IMAGE_PIXEL_FMT_YUV422_PLANAR  = 0x00,    ///< 3 color plane, pixel=Y(w,h), U(w/2,h), and V(w/2,h)
	GX_IMAGE_PIXEL_FMT_YUV420_PLANAR  = 0x01,    ///< 3 color plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	GX_IMAGE_PIXEL_FMT_YUV422_PACKED  = 0x02,    ///< 2 color plane, pixel=Y(w,h), UV(w,h)
	GX_IMAGE_PIXEL_FMT_YUV420_PACKED  = 0x03,    ///< 2 color plane, pixel=Y(w,h), UV(w,h/2)
	GX_IMAGE_PIXEL_FMT_Y_ONLY         = 0x04,    ///< 1 color plane, pixel=Y(w,h),
	GX_IMAGE_PIXEL_FMT_ARGB565        = 0x05,    ///< 2 color plane, pixel=A8(w,h),RGB565(2w,h)
	GX_IMAGE_PIXEL_FMT_RGB888_PLANAR  = 0x06,    ///< 3 color plane, pixel=R(w,h), G(w,h), B(w,h)
	GX_IMAGE_PIXEL_FMT_ARGB8888_PACKED = 0x07,   ///< 1 color plane, pixel=ARGB(4w,h)
	GX_IMAGE_PIXEL_FMT_ARGB1555_PACKED = 0x08,   ///< 1 color plane, pixel=ARGB(2w,h)
	GX_IMAGE_PIXEL_FMT_ARGB4444_PACKED = 0x09,   ///< 1 color plane, pixel=ARGB(2w,h)
	GX_IMAGE_PIXEL_FMT_RGB565_PACKED   = 0x0A,   ///< 1 color plane, pixel=RGB(2w,h)
	GX_IMAGE_PIXEL_FMT_RAW8            = 0x10,   ///< 1 color plane, pixel=RAW(w,h) x 8bits
	GX_IMAGE_PIXEL_FMT_RAW10           = 0x11,   ///< 1 color plane, pixel=RAW(w,h) x 10bits
	GX_IMAGE_PIXEL_FMT_RAW12           = 0x12,   ///< 1 color plane, pixel=RAW(w,h) x 12bits
	GX_IMAGE_PIXEL_FMT_RAW16           = 0x13,   ///< 1 color plane, pixel=RAW(w,h) x 16bits
	GX_IMAGE_PIXEL_FMT_YUV444_PLANAR   = 0x14,   ///< 3 color plane, pixel=Y(w,h), U(w,h), and V(w,h)
	ENUM_DUMMY4WORD(GX_IMAGE_PIXEL_FMT)
} GX_IMAGE_PIXEL_FMT;

typedef struct _IMG_BUF {
	UINT32              flag;                      ///< used for check if this image buffer is initialized
	UINT32              Width;                     ///< image width
	UINT32              Height;                    ///< image height
	GX_IMAGE_PIXEL_FMT  PxlFmt;                    ///< pixel format
	UINT32              PxlAddr[MAX_PLANE_NUM];    ///< pixel address
	UINT32              LineOffset[MAX_PLANE_NUM]; ///< pixel lineoffset
	IPOINT              ScaleRatio;                ///< Scale ratio for virtual/Real coordinate translation
	IRECT               VirtCoord;                 ///< Virtual coordinate
	IRECT               RealCoord;                 ///< Real    coordinate
	MEM_RANGE           Metadata;                  ///< Metadata memory range
} IMG_BUF, *PIMG_BUF;
#endif // _TODO


#include "avfile/MediaReadLib.h"
#include "avfile/AVFile_ParserMov.h"
//#include "avfile/AVFile_ParserAvi.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	GXVIDFILE_PARAM_MAX_W,           ///< maximum width config from project level
	GXVIDFILE_PARAM_MAX_H,           ///< maximum height config from project level
	GXVIDFILE_PARAM_IS_DECRYPT,      ///< video file is needed to decrypt
	GXVIDFILE_PARAM_DECRYPT_KEY,     ///< video decryption key
	GXVIDFILE_PARAM_FILEREADSIZE,    ///< 1st and 2nd file read size for parse 1st video
	GXVIDFILE_PARAM_FILEENDREADSIZE,
	GXVIDFILE_PARAM_DECRYPT_MODE,    ///< video decryption mode
	GXVIDFILE_PARAM_DECRYPT_POS,     ///< video decryption pos
} GXVIDFILE_PARAM;

/**
    @addtogroup mILIBGXVIDFILE
*/
//@{
/**
    @name Buffer size of video header parser

    Buffer size of video header parser

    @note Refrence by calling GxVidFile_Query1stFrameWkBufSize()
*/
//@{
#define GXVIDEO_PARSE_HEADER_BUFFER_SIZE     0x3E8000
//@}
/**
    @name SMedia Video Decoder H265 Nal Unit Type Max Buffer Size

    SMedia video decoder h265 nal unit type max buffer size
    (please see definition "DecNalBufSize" in h265dec_header.c)
*/
//@{
#define GXVIDEO_H265_NAL_MAXSIZE             512           ///< H265 Nal unit type max buffer size
//@}

/**
    @name Buffer size for H26x working buffer (only use in TS format)

    Buffer size for H26x working buffer (only use in TS format)
*/
//@{
#define GXVIDEO_H26X_WORK_BUFFER_SIZE        64 * 1024
//@}

/**
    @name Entry buffer size for parser

    Entry buffer size for parser
*/
//@{
#define GXVIDEO_VID_ENTRY_BUFFER_SIZE            sizeof(TS_FRAME_ENTRY) * TSVID_ENTRY_SIZE
#define GXVIDEO_AUD_ENTRY_BUFFER_SIZE            sizeof(TS_FRAME_ENTRY) * TSAUD_ENTRY_SIZE
//@}

#define GXVIDEO_PARSE_TS_THUMB_BUFFER_SIZE   0x250000

#define GXVIDEO_TSDEMUX_MEM_OFFSET           0x200000
#define GXVIDEO_TSDEMUX_THUMB_OFFSET         0x200000
/**
    Error Code definition

    @note Returned error code by GxVideoFile function API.
*/
typedef enum _GXVID_ER {
	GXVIDEO_PRSERR_OK               =     0,    ///< Parser OK
	GXVIDEO_PRSERR_SYS              = (-2),     ///< function error
	GXVIDEO_PRSERR_AVI_NOTRIFF      = (-5),     ///< Not four-character code id: FOURCC_RIFF (AVI Header)
	GXVIDEO_PRSERR_AVI_NOTAVI       = (-6),     ///< Not four-character code id: FOURCC_AVI (AVI Header)
	GXVIDEO_PRSERR_AVI_NOLIST       = (-7),     ///< No LIST chunk exist (AVI Header)
	GXVIDEO_PRSERR_AVI_NOxxrl       = (-8),     ///< No four-character code id: FOURCC_HDRL or FOURCC_STRL (AVI Header)
	GXVIDEO_PRSERR_AVI_NOstrf       = (-9),     ///< Not four-character code id: FOURCC_STRF (AVI Header)
	GXVIDEO_PRSERR_AVI_NOTavih      = (-10),    ///< Not four-character code id: FOURCC_avih (AVI Header)
	GXVIDEO_PRSERR_AVI_NOTstrh      = (-11),    ///< Not four-character code id: FOURCC_strh (AVI Header)
	GXVIDEO_PRSERR_AVI_NOTxxds      = (-12),    ///< Not four-character code id: FOURCC_VIDS or FOURCC_AUDS (AVI Header)
	GXVIDEO_PRSERR_AVI_vids         = (-13),    ///< Parse four-character code FOURCC_VIDS error (AVI Header)
	GXVIDEO_PRSERR_AVI_auds         = (-14),    ///< Parse four-character code FOURCC_AUDS error (AVI Header)
	GXVIDEO_PRSERR_AVI_NOmovi       = (-15),    ///< No movi chunk (AVI Header)
	GXVIDEO_PRSERR_AVI_NOidx1       = (-16),    ///< No idx1 chunk (AVI Header)
	ENUM_DUMMY4WORD(GXVID_ER)
} GXVID_ER;

typedef enum {
	GXVIDFILE_IMAGE_TYPE_H264 = 0,
	GXVIDFILE_IMAGE_TYPE_H265,
	ENUM_DUMMY4WORD(GXVIDFILE_IMAGE_TYPE)
} GXVIDFILE_IMAGE_TYPE;

typedef enum {
	GXVIDFILE_RETURN_TYPE_SUCCESS = 0,
	GXVIDFILE_RETURN_TYPE_VIDEO_NOT_EXIST,
	GXVIDFILE_RETURN_TYPE_BUFFER_NOT_ENOUGH,
	GXVIDFILE_RETURN_TYPE_VIDEO_RECORDING,
	GXVIDFILE_RETURN_TYPE_TRANSFER_INTERRUPT,
	GXVIDFILE_RETURN_TYPE_DVR_NON_RESPONSE,
	ENUM_DUMMY4WORD(GXVIDFILE_RETURN_TYPE)
} GXVIDFILE_RETURN_TYPE;

typedef enum _GXVIDEO_MEM_INDEX {
	GXVIDEO_MEM_VDOENTTBL = 0,                  ///< video frame entry table
	GXVIDEO_MEM_AUDENTTBL,                      ///< audio frame entry table
	GXVIDEO_MEM_CNT,
	ENUM_DUMMY4WORD(_GXVIDEO_MEM_INDEX)
} GXVIDEO_MEM_INDEX;

typedef enum {
	GXVIDEO_DECRYPT_TYPE_NONE              = 0x00000000,
	GXVIDEO_DECRYPT_TYPE_CONTAINER         = 0x00000001,
	GXVIDEO_DECRYPT_TYPE_I_FRAME           = 0x00000002,
	GXVIDEO_DECRYPT_TYPE_P_FRAME           = 0x00000004,
	GXVIDEO_DECRYPT_TYPE_ALL_FRAME         = 0x00000006,
	ENUM_DUMMY4WORD(GXVIDEO_DECRYPT_TYPE)
} GXVIDEO_DECRYPT_TYPE;

typedef enum {
	GXVIDEO_DECRYPT_MODE_AES128            = 2,
	GXVIDEO_DECRYPT_MODE_AES256			 = 3,
	ENUM_DUMMY4WORD(GXVIDEO_DECRYPT_MODE)
} GXVIDEO_DECRYPT_MODE;

/**
    Struture of Audio Stream information

    @note Returned by GxVidFile_ParseVideoInfo()
*/
typedef struct _GXVIDEO_AUDINFO {
	UINT32           uiSR;                ///<[out] Audio sample rate information
	UINT32           uiChs;               ///<[out] Audio channel information
	UINT32           uiType;              ///<[out] Audio codec type information
} GXVIDEO_AUDINFO, *PGXVIDEO_AUDINFO;

/**
    Structure of Video File information

    @note Returned by GxVidFile_ParseVideoInfo()
*/
typedef struct _GXVIDEO_INFO {
	UINT32           uiTotalFrames;          ///<[out] Total Video Frames
	UINT32           uiToltalSecs;           ///<[out] Total Seconds
	UINT32           uiVidWidth;             ///<[out] Video Width
	UINT32           uiVidHeight;            ///<[out] Video Height
	UINT32           uiVidRate;              ///<[out] Video Frame Rate
	UINT64           uiUserDataAddr;         ///<[out] User Data Position in File
	UINT32           uiUserDataSize;         ///<[out] User Data Size
	UINT64           uiFre1DataAddr;         ///<[out] Fre1 Data Position in File
	UINT32           uiFre1DataSize;         ///<[out] Fre1 Data Size
	UINT64           uiThumbOffset;          ///<[out] Thumbnail offset, from the start of video file
	UINT32           uiThumbSize;            ///<[out] Thumbnail size.
	UINT32           uiVidRotate;            ///<[out] Video Rotation information
	UINT32           uiVidType;              ///<[out] Video codec type information
	UINT32           uiH264DescAddr;         ///<[out] H264 SPS/PPS data address
	UINT32           uiH264DescSize;         ///<[out] H264 SPS/PPS data size
	UINT32           uiMovCreateTime;        ///<[out] Create time for MOV file
	UINT32           uiMovModificationTime;  ///<[out] Modification time for MOV file
	VOID            *pHeaderInfo;            ///<[out] Header information (MEDIA_AVIHEADER)
	GXVIDEO_AUDINFO  AudInfo;                ///<[out] Audio stream information
	UINT32           uiDispWidth;            ///<[out] Display Width
	UINT32           uiDispHeight;           ///<[out] Display Height
	UINT64           uiScraOffset;           ///<[out] Screennail offset, from the start of video file
	UINT32           uiScraSize;             ///<[out] Screennail size.
	UINT64           ui1stFramePos;          ///<[out] first video frame position
	UINT32           ui1stFrameSize;         ///<[out] first video frame size
} GXVIDEO_INFO, *PGXVIDEO_INFO;

typedef struct {
	UINT32           max_w;                  ///< maximum width config from project level
	UINT32           max_h;                  ///< maximum height config from project level
} GXVIDFILE_OBJ;

typedef struct {
	char              *filename;             ///< the file name
	int                time_offset_ms;
	UINT8             *bs_buffer;
	UINT32             bs_size;
} GXVIDFILE_INTERCEPT_PARM;

typedef struct {
	GXVIDFILE_RETURN_TYPE return_type;       ///< the return error code
	int                   length;            ///< the return bitstream data length
	GXVIDFILE_IMAGE_TYPE  image_type;        ///< the return bitstream type
	int                   width;             ///< the image width
	int                   height;            ///< the image height
	int                   offset;            ///< the frame offset , the first GOP is 0, the second p is 1, ...
} GXVIDFILE_INTERCEPT_RESULT;

/**
    @name   GxVideoFile Video Play API
*/
//@{

/**
    Callback Function Description

    The prototype of callback function for video file reading

    @param[in] pos       file read position
    @param[in] size      file read size
    @param[in] addr      read buffer address

    @return ER
*/
typedef  ER(* FPGXVIDEO_READCB)(UINT32 pos, UINT32 size, UINT32 addr);

typedef  ER(* FPGXVIDEO_READCB64)(UINT64 pos, UINT32 size, UINT32 addr);

//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
/**
    Callback Function Description

    The prototype of callback function for getting thumbnail frame image

    @param[in] userdata_buf    user-data buffer
    @param[in] userdata_size   user-data size
    @param[out] thumb_pos      returned thumbnail position in user-data buffer
    @param[out] thumb_size     returned thumbnail size

    @return ER

*/
typedef  ER(* FPGXVIDEO_GETHUMBCB)(UINT32 userdata_buf, UINT32 userdata_size, UINT64 *thumb_pos, UINT32 *thumb_size);
//#NT#2012/08/21#Calvin Chang -end

/**
    Query Video Buffer Needed Size

    Query the needed size of video buffer for decoding 1st video frame

    @param[in] uiTotalFileSize    the total file size of opened video file
    @param[out] uipBufferNeeded   the video buffer needed size

    @return
     - @b GXVIDEO_PRSERR_OK:     Query successfully.
*/
GXVID_ER GxVidFile_Query1stFrameWkBufSize(UINT32 *uipBufferNeeded, UINT64 uiTotalFileSize);
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
/**
    Decode 1st Video Frame

    Decode 1st video frame for video play mode, must call GxVidFile_Query1stFrameWkBufSize function first.

    @param[in] fpReadCB      the file read callback function pointer
    @param[in] pWorkBuf      the memory address and size
    @param[out] pDstImgBuf   the 1st video fram image

    @return
     - @b GXVIDEO_PRSERR_OK:     Decode 1st video frame successfully.
     - @b GXVIDEO_PRSERR_SYS:    Decode 1st video is failed. Have to check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_Decode1stFrame64(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf);

GXVID_ER GxVidFile_Decode1stFrame(FPGXVIDEO_READCB fpReadCB, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf);

GXVID_ER GxVidFile_Decode1stFrameImpl(FPGXVIDEO_READCB fpReadCB, FPGXVIDEO_READCB64 fpReadCB64, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf);
#endif
//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
/**
    Register callback function for thumbnail

    Register callback function for getting thumbnail stream in user data

    @param[in] fpGetThumbCB    FPGXVIDEO_GETHUMBCB callback function pointer

    @return
     - @b GXVIDEO_PRSERR_OK:     Register successfully.
*/
GXVID_ER GxVidFile_RegisterGetThumbCB(FPGXVIDEO_GETHUMBCB fpGetThumbCB);
//#NT#2012/08/21#Calvin Chang -end

/**
    Query Video Buffer Needed Size

    Query the needed size of video buffer for decoding thumbnail frame

    @param[in] uiTotalFileSize    the total file size of opened video file
    @param[out] uipBufferNeeded   the video buffer needed size

    @return
     - @b GXVIDEO_PRSERR_OK:     Query successfully.
*/
GXVID_ER GxVidFile_QueryThumbWkBufSize(UINT32 *uipBufferNeeded, UINT64 uiTotalFileSize);

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
/**
    Decode Thumbnail Frame

    Decode thumbnail frame for video file play mode, must call GxVidFile_QueryThumbWkBufSize function first.

    @param[in] fpReadCB      the file read callback function pointer
    @param[in] pWorkBuf      the memory address and size
    @param[out] pDstImgBuf   the thumbnail fram image

    @return
     - @b GXVIDEO_PRSERR_OK:     Decode thumbnail frame successfully.
     - @b GXVIDEO_PRSERR_SYS:    Decode thumbnail frame is failed.
*/
GXVID_ER GxVidFile_DecodeThumbFrame(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, PIMG_BUF pDstImgBuf);
#endif
GXVID_ER GxVidFile_ParseThumbInfo(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo);

/**
    Video Information Parser

    Parse the information of video file

    @param[in] fpReadCB       the file read callback function pointer
    @param[in] pWorkBuf       the memory address and size
    @param[in] uiTotalFileSize    the total file size of opened video file
    @param[out] pVideoInfo   the video file information
    @return
     - @b GXVIDEO_PRSERR_OK:     Parse video information successfully.
     - @b GXVID_ER:              Video Information Parser is failed. Reference ErrCode and check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_QueryParseVideoInfoBufSize(UINT32 *uipBufferNeeded);

GXVID_ER GxVidFile_ParseVideoInfo64(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo);

GXVID_ER GxVidFile_ParseVideoInfo(FPGXVIDEO_READCB fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo);

GXVID_ER GxVidFile_ParseVideoInfoImpl(FPGXVIDEO_READCB fpReadCB, FPGXVIDEO_READCB64 fpReadCB64, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo);

/**
    Video Information Parser

    Parse the information of video file

    @param[in] fpReadCB       the file read callback function pointer
    @param[in] pWorkBuf       the memory address and size
    @param[in] uiTotalFileSize    the total file size of opened video file
    @param[out] pVideoInfo   the video file information
    @return
     - @b GXVIDEO_PRSERR_OK:     Parse video information successfully.
     - @b GXVID_ER:              Video Information Parser is failed. Reference ErrCode and check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_ParseVideoInfoEntry(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT64 uiTotalFileSize, PGXVIDEO_INFO pVideoInfo);

/**
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
GXVID_ER GxVidFile_GetVideoIFrameOffsetSize(UINT32 uiGetCnt, UINT32 uiGetNum, UINT64 *pOffset, UINT32 *pSize);

/**
    Get thumbnail in TS file

    Get thumbnail in TS file

    @param[in] fpReadCB       the file read callback function pointer
    @param[in] pWorkBuf       the memory address and size
    @param[out] thumbAddr     the parsed thumbnail address in memory
    @param[out] thumbSize     the thumbnail size
    @return
     - @b GXVIDEO_PRSERR_OK:     Parse thumbnail successfully.
     - @b GXVID_ER:              Thumbnail Parser is failed. Reference ErrCode and check the content of input video file whether is correct.
*/
GXVID_ER GxVidFile_GetTsThumb64(FPGXVIDEO_READCB64 fpReadCB, PMEM_RANGE pWorkBuf, UINT32 *thumbAddr, UINT32 *thumbSize);

GXVID_ER GxVidFile_GetTsThumb(FPGXVIDEO_READCB fpReadCB, PMEM_RANGE pWorkBuf, UINT32 *thumbAddr, UINT32 *thumbSize);

GXVID_ER GxVidFile_GetTsThumbImpl(FPGXVIDEO_READCB fpReadCB, FPGXVIDEO_READCB64 fpReadCB64, PMEM_RANGE pWorkBuf, UINT32 *thumbAddr, UINT32 *thumbSize);

GXVID_ER GxVidFile_SetParam(GXVIDFILE_PARAM Param, UINT32 Value);

GXVID_ER GxVidFile_GetParam(GXVIDFILE_PARAM Param, UINT32 *pValue);

GXVID_ER GxVidFile_GetStreamByTimeOffset(FPGXVIDEO_READCB64 fpReadCB64, GXVIDFILE_INTERCEPT_PARM *param, GXVIDFILE_INTERCEPT_RESULT *ret);

GXVID_ER GxVideoFile_Decrypto(UINT32 addr, UINT32 size);

//@}

//@}

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_GX_VIDEOFILE_H_
