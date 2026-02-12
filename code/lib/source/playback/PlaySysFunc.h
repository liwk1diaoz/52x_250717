#ifndef _PLAYSYSFUNC_H
#define _PLAYSYSFUNC_H

#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include <string.h>
#include "kwrap/flag.h"
#include "FileSysTsk.h"
#include "PBXFile.h"
#include "PBXFileList.h"
#include "exif/Exif.h"
#include "exif/ExifDef.h"
#include "BinaryFormat.h"
#include "hd_type.h"
#include "hdal.h"
#include "GxVideoFile.h"

#if defined(__FREERTOS)
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#define kent_tsk()
#endif

#if _TODO
#include "JpgDec.h"
#include "GxVideoFile.h"
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PB
#define __DBGLVL__          2   // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
//unsigned int playback_debug_level = NVT_DBG_WRN;
//module_param_named(playback_debug_level, playback_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(playback_debug_level, "playback debug level");
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------
// Playback switch define
//-----------------------------------------------------------------------------------------------
#define PB_USE_HW_BRC               ENABLE
#if PB_USE_HW_BRC
#define PB_HW_BRC_QF_FINE           64
#define PB_HW_BRC_QF_NORMAL         128
#endif
#define PB_BRC_DEFAULT_COMPRESS_RATIO  12
#define PB_BRC_DEFAULT_UPBOUND_RATIO   15
#define PB_BRC_DEFAULT_LOWBOUND_RATIO  15
#define PB_BRC_DEFAULT_LIMIT_CNT       5

// Keep rotate display ratio
#define PB_ROTATEDISPLAY_KEEP_RATIO ENABLE
// Limit the display width of images(width < height) in Zoom mode
#define PB_ZOOMROTATED_WIDTH_LIMIT  DISABLE
//#NT#2012/05/17#Lincy Lin -begin
//#NT#Porting Disp srv
#define PB_USE_DISP_SRV             ENABLE
//#NT#2012/05/17#Lincy Lin -end
#define FST_READ_THUMB_BUF_SIZE         0x10000

#define VID_DECRYPT_KEY_LEN            32
#define VID_DECRYPT_LEN                0x100

//-----------------------------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------------------------
#define PLAY_MAX_THUMB_NUM              25      // Maximum 25 thumbnail images



#define PLAY_IMETMP_WIDTH               640
#define PLAY_IMETMP_HEIGHT              540

#define PLAYBUFSIZE_MIN_RAW             0x00100000      // reserved min-size (1M) for Raw buffer
#define PLAYBUFSIZE_MIN_FILE            0x00080000      // reserved min-size (0.5M) for File buffer

#define PLAYBUFSIZE_MIN_ALLBUF          (PLAYBUFSIZE_MIN_RAW+PLAYBUFSIZE_MIN_FILE)
//#NT#2010/08/27#Ben Wang -begin
//#Place Hidden image after primary image
//--------------------------------------------------------------------------------------------------
// PLAYONEPAGE_FILEBUFSIZE must be bigger than (FST_READ_THUMB_BUF_SIZE + FST_READ_SOMEAVI_SIZE)
// and 280K for ASF decode 1st image
//-------------------------------------------------------------------------------------------------
//#NT#2012/11/07#Scottie -begin
//#NT#Modify for 3M screennail (original screennail size is VGA)
//#define PLAYONEPAGE_FILEBUFSIZE         0x00080000
#define PLAYONEPAGE_FILEBUFSIZE         0x00180000
//#NT#2012/11/07#Scottie -end
#define PB_DEFAULT_FILEBUFSIZE          PLAYONEPAGE_FILEBUFSIZE
//#NT#2010/08/27#Ben Wang -end

#define PLAYONEPAGE_MOVFILE_BUFSIZE     0x00200000   //#NT#2010/09/15#Lily Kao

//#Fix copy file bug when buf size is large than 16 MB
#define PB_MAX_COPY_FILE_BUF_SIZE       0x1000000


#define FLG_PB                          FLG_ID_PLAYBACK

#define FLGPB_COMMAND                   FLGPTN_BIT(0) //0x00000001
#define FLGPB_TIMEOUT                   FLGPTN_BIT(1) //0x00000002
#define FLGPB_DONE                      FLGPTN_BIT(2) //0x00000004
#define FLGPB_INITFAIL                  FLGPTN_BIT(3) //0x00000008
#define FLGPB_DECODEERROR               FLGPTN_BIT(4) //0x00000010
#define FLGPB_READERROR                 FLGPTN_BIT(5) //0x00000020
#define FLGPB_WRITEERROR                FLGPTN_BIT(6) //0x00000040
#define FLGPB_NOIMAGE                   FLGPTN_BIT(7) //0x00000080
#define FLGPB_NOTBUSY                   FLGPTN_BIT(8) //0x00000100
#define FLGPB_BUSY                      FLGPTN_BIT(9) //0x00000200

//#NT#2012/05/17#Lincy Lin -begin
//#NT#Porting disply Srv
#define FLGPB_PUSH_FLIPPED              FLGPTN_BIT(14)//0x00004000
#define FLGPB_PULL_DRAWEND              FLGPTN_BIT(15)//0x00008000
#define FLGPB_PULL_FLIPPED              FLGPTN_BIT(16)//0x00010000
#define FLGPB_IMG_0                     FLGPTN_BIT(17)//0x00020000
#define FLGPB_IMG_1                     FLGPTN_BIT(18)//0x00040000
#define FLGPB_IMG_STOP                  FLGPTN_BIT(31)//0x80000000
//#NT#2012/05/15#Lincy Lin -end

#define FLGPB_ONDRAW                    FLGPTN_BIT(0) //0x00000001

#define FLGPB_MASKALL                   (FLGPB_COMMAND | FLGPB_TIMEOUT | FLGPB_DONE | FLGPB_INITFAIL | \
		FLGPB_DECODEERROR | FLGPB_READERROR | FLGPB_WRITEERROR | \
		FLGPB_NOIMAGE | FLGPB_NOTBUSY | FLGPB_BUSY )


#define DECODE_JPG_THUMBOK              1
#define DECODE_JPG_PRIMARY              2
#define DECODE_JPG_AVIOK                3
#define DECODE_JPG_WAVEOK               4
#define DECODE_JPG_ASFOK                5
#define DECODE_JPG_DONE                 6
#define DECODE_MPG_DONE                 7
#define DECODE_JPG_MOVOK                8   //#NT#2010/09/15#Lily Kao

#define DECODE_JPG_BIGFILE              (-1)
#define DECODE_JPG_READERROR            (-2)
#define DECODE_JPG_DECODEERROR          (-3)
#define DECODE_JPG_WRITEERROR           (-4)
#define DECODE_JPG_NOIMAGE              (-5)
#define DECODE_JPG_FILESYSFAIL          (-6)
#define DECODE_JPG_NONDCF               (-7)
#define DECODE_JPG_NONEOI               (-8)
#define DECODE_JPG_CUSTOMERROR          (-9)

//#NT#2010/02/26#Ben Wang#[0010611] -begin
//#NT#Add Hidden Image size macro
// Screennail size
#define PB_SCREENNAIL_WIDTH             640
#define PB_SCREENNAIL_HEIGHT            480
//#NT#2010/02/26#Ben Wang#[0010611] -end

// File Size Control (SW BRC) mechanism
#define BRC_FINETUNE_VALUE              700
#define BRC_UPBOUNDvsTARGET_RATIO       0.86
#define BRC_UPBOUNDvsLOWBOUND_RATIO     0.73
#define BRC_REENC_TIMES                 7

/*
    @name The numbers of pixels for DCF thumbnail
*/
//@{
//#NT#2016/03/25#Scottie -begin
//#NT#Modify the default thumb size to 640x480 for better image quality (for HDMI)
#if 0
#define DCF_THUMB_PIXEL_H               160     ///< Horizontal pixels
#define DCF_THUMB_PIXEL_V               120     ///< Vertical pixels
#else
#define DCF_THUMB_PIXEL_H               640     ///< Horizontal pixels
#define DCF_THUMB_PIXEL_V               480     ///< Vertical pixels
#endif
//#NT#2016/03/25#Scottie -end
#define DCF_THUMB_PIXELS                (DCF_THUMB_PIXEL_H*DCF_THUMB_PIXEL_V)   ///< Horizontal pixels by Vertical pixels
//@}
//#NT#2013/04/18#Ben Wang -begin
//#NT#Add callback for decoding image and video files.
#define DEC_CALLBACK_ERROR  DECODE_JPG_DECODEERROR
//#NT#2013/04/18#Ben Wang -end

#define MAX_TWO_PASS_SCALE_W			1920
#define MAX_TWO_PASS_SCALE_H			1440

#define COLOR_YUV_BLACK             0x00808000
//-----------------------------------------------------------------------------------------------
// Enumerations
//-----------------------------------------------------------------------------------------------
typedef enum {
	PB_CMMD_NONE = 0x00000000,

	// PB_Hdl_DisplayImage
	PB_CMMD_SINGLE,
	PB_CMMD_BROWSER,
	PB_CMMD_ZOOM,
	PB_CMMD_ROTATE_DISPLAY,
	PB_CMMD_DE_SPECFILE,

	// PB_Hdl_EditImage
	PB_CMMD_RE_SIZE,
	PB_CMMD_RE_QTY,
	PB_CMMD_UPDATE_EXIF,
	PB_CMMD_CROP_SAVE,

	// PB_Hdl_OpenFile
	PB_CMMD_OPEN_NEXT,
	PB_CMMD_OPEN_PREV,
	PB_CMMD_OPEN_SPECFILE,

	PB_CMMD_CAPTURE_SCREEN,

	// Customize image effect
	PB_CMMD_CUSTOMIZE_DISPLAY,
	PB_CMMD_CUSTOMIZE_EDIT,

	PB_CMMD_NUM,                    // Total number of Playback Command
	ENUM_DUMMY4WORD(PB_CMMD)
} PB_CMMD;

typedef enum {
	PLAYSYS_COPY2FBBUF,
	PLAYSYS_COPY2TMPBUF,
	PLAYSYS_COPY2IMETMPBUF
} PB_DISPLAY_BUFF;

typedef enum {
	PB_SCALE_IME,
	PB_SCALE_WARP,
	PB_SCALE_FW,
	ENUM_DUMMY4WORD(PB_SCALE_WAY)
} PB_SCALE_WAY;

//#NT#2012/03/26#Lincy Lin -begin
//#NT#Porting PBXFile
typedef enum {
	PB_FILE_READ_PREV = 0,
	PB_FILE_READ_NEXT,
	PB_FILE_READ_SPECIFIC,
	//#NT#2012/06/15#Lincy Lin -begin
	//#NT#Fine tune find last file
	PB_FILE_READ_LAST,
	//#NT#2012/06/15#Lincy Lin -end
	PB_FILE_READ_CONTINUE,
	ENUM_DUMMY4WORD(PB_FILE_READ_CMD)
} PB_FILE_READ_CMD;
//#NT#2012/03/26#Lincy Lin -end

//-----------------------------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------------------------
//#NT#2012/05/17#Lincy Lin -begin
//#NT#


//#NT#2012/05/17#Lincy Lin -begin
//#NT#Porting disply Srv
#define    PB_DISP_SRC_DEC     0
#define    PB_DISP_SRC_TMP     1
#define    PB_DISP_SRC_IMETMP  2
#define    PB_DISP_SRC_THUMB1  3
#define    PB_DISP_SRC_THUMB2  4
#define    PB_DISP_SRC_COUNT   5


//#NT#2012/05/17#Lincy Lin -end

/*
 @struct PLAYMODE_INFO PlaybackTsk.h
     This structuer contain Playback mode information.
*/
typedef struct _PLAYMODE_INFO {
	// Offset 4, thumbnail info, DispFileName[0] is used in play-single mode
	PURECT              pRectDrawThumb;     ///<  Thumbnail rect to display buffer
//#NT#2012/11/27#Scottie -begin
//#NT#Support Dual Display for PB
	PURECT              pRectDrawThumb2;    ///<  Thumbnail rect2 to display buffer for dual view
//#NT#2012/11/27#Scottie -end
	UINT32              PanelBGColor;
	UINT16              DispThumbNums;      ///<  Thumbnail numbers
	UINT32              DispFileName[PLAY_MAX_THUMB_NUM];   ///<  Thumbnail file name
	UINT32              DispDirName[PLAY_MAX_THUMB_NUM];    ///<  Thumbnail dir name
	UINT32              DispFileSeq[PLAY_MAX_THUMB_NUM];    ///<  Thumbnail file seq id
	UINT16              AllThumbFormat[PLAY_MAX_THUMB_NUM]; ///<  Thumbnail file format, 0x01->JPG; 0x02->WAV; 0x04->AVI; 0x80->Read-only
	UINT32              MovieLength[PLAY_MAX_THUMB_NUM]; ///<  Thumbnail movie length info, unit: second
	BOOL                bThumbDecErr[PLAY_MAX_THUMB_NUM];   ///<  Thumbnail file decoding error status, TRUE: decode ERROR; FALSE: decode OK;
	UINT32              JumpOffset;         ///<  Thumbnail read file jump to next(previous) image offset
	UINT8               ThumbInfoRvd[1];    ///<  Thumbnail reserved
	// zoom & pan info
	UINT32              *uipZoomLevelTbl;   ///<  Zoom Level table
	UINT8               ZoomIndex;          ///<  Zoom table index,or zoom level, 0->disable ; 1->1X ; 2->2X ; 3->3X
	UINT16              PanMaxX;            ///<  Pan mode, max X addr
	UINT16              PanMaxY;            ///<  Pan mode, max Y addr
	UINT16              PanSrcStartX;       ///<  Pan mode, current X addr
	UINT16              PanSrcStartY;       ///<  Pan mode, current Y addr
	UINT16              PanSrcWidth;        ///<  Pan mode, current width
	UINT16              PanSrcHeight;       ///<  Pan mode, current height
	UINT16              PanDstStartX;       ///<  Pan mode, moving to X addr
	UINT16              PanDstStartY;       ///<  Pan mode, moving to Y addr
	UINT16              PanDstWidth;        ///<  Pan mode, current dst width
	UINT16              PanDstHeight;       ///<  Pan mode, current dst height
	UINT8               ZoomInfoRvd[1];     ///<  Zoom reserved
	UINT8               RotateDir;          ///<  Rotate mode dir
	// Playback normal info
	//#NT#2012/04/02#Lincy Lin -begin
	//#NT#
	UINT32              CurIndexOfTotal;    ///<  Play current index of total files
	//#NT#2012/04/02#Lincy Lin -end
	UINT16              CurFileFormat;      ///<  Play current file format, 1->JPG ; 2->AVI 3->WAV
	UINT8               CurFileIndex;       ///<  Play current file index in array (DispFileName[], DispDirName[])
	UINT8               Reserved;           ///<  reserved
	UINT8               CurrentMode;        ///<  Play current mode, 0->Primary ; 1->Thumbnail ; 2->Rotate
	UINT8               ProtectOPMode;      ///<  Protect op-mode
	UINT8               DecodePrimaryOK;    ///<  Decoded primary image ok

	UINT32              PlayTimeOutID;      ///<  Playback mode time-out id

	// play files sort by date
	UINT16              PlayDateType;             ///< PLAY_FILE_BY_DAY / PLAY_FILE_BY_MONTH / PLAY_FILE_BY_YEAR
	UINT16              uiPlayDate;               ///< play files which in on play date
	UINT16              uiFirstFileIndex;         ///< first file index in specific day
	UINT16              uiTotalFilesInSpecDate;   ///< play files which in on play date
} PLAYMODE_INFO, *PPLAYMODE_INFO;

//#NT#2012/09/10#Scottie -begin
//#NT#Add to set the default settings of PB
typedef struct {
	UINT32      DispDirection;          ///< Screen display direction (PB_HORIZONTAL/PB_VERTICAL)
	UINT32          ThumbShowMethod;        ///< PB_SHOW_THUMB_IMAGE_ONE_BY_ONE / PB_SHOW_THUMB_IMAGE_IN_THE_SAME_TIME
	UINT32          EnableFlags;            ///< refer to Playback function Enable Flag
	BOOL            bEnableAutoRotate;      ///< Auto-rotate image according to EXIF-Orientation tag value (TRUE/FALSE)
	INT32           FileHandle;             ///< File handle to communicate with file DB
	UINT8          *pSlideEffectSpeedTbl;   ///< The pointer of the speed table for slide effect functions
	//#NT#2013/02/20#Lincy Lin -begin
	//#NT#Support Filelist plugin
	PPBX_FLIST_OBJ  pFileListObj;           ///< The File List object pointer
	//#NT#2013/02/20#Lincy Lin -end
	BOOL            bEnableVidDecrypt;
	UINT8           VidDecryptKey[VID_DECRYPT_KEY_LEN];
	PB_HD_COM_BUF   VidDecryptBuf;
	BOOL            bEnableJpgDecrypt;
	UINT8           *JpgDecryptKey;
	UINT32          JpgDecryptLen;
	UINT32          VidDecryptMode;
	UINT32          VidDecryptPos;
} PB_SETTING, *PPB_SETTING;
//#NT#2012/09/10#Scottie -end

typedef struct _PLAY_ENCODE_NEWFILE_INFO {
	UINT32      NewImgRawBufY;
	UINT32      NewImgRawBufCb;
	UINT32      NewImgRawBufCr;
	UINT32      TmpImgRawBufY;
	UINT32      TmpImgRawBufCb;
	UINT32      TmpImgRawBufCr;
	UINT32      NewImgWidth;
	UINT32      NewImgHeight;
	UINT32      NewFileBufAddr;
	UINT32      NewFileBufSize;
} PLAY_ENCODE_NEWFILE_INFO, *PPLAY_ENCODE_NEWFILE_INFO;

typedef struct _PLAY_SCALE_SIZE_INFO {
	UINT32      in_h;       //input horizontal size
	UINT32      in_v;       //input vertical size
	UINT32      out_h;      //output horizontal size
	UINT32      out_v;      //output vertical size
} PLAY_SCALE_SIZE_INFO, *PPLAY_SCALE_SIZE_INFO;

typedef struct _PLAY_YCBCR_ADDR {
	UINT32      y_addr;     //Y starting address
	UINT32      cb_addr;    //Cb starting address
	UINT32      cr_addr;    //Cr starting address
} PLAY_YCBCR_ADDR, *PPLAY_YCBCR_ADDR;

typedef struct _PLAY_SCALE_INFO {
	PLAY_SCALE_SIZE_INFO    io_size;
	PLAY_YCBCR_ADDR         in_addr;
	PLAY_YCBCR_ADDR         out_addr;
	UINT16                  InputLineOffset_Y;
	UINT16                  InputLineOffset_UV;
	UINT16                  OutputLineOffset_Y;
	UINT16                  OutputLineOffset_UV;
	UINT8                   in_format;
	UINT8                   out_format;
} PLAY_SCALE_INFO, *PPLAY_SCALE_INFO;

typedef struct _PLAY_CMMD_SINGLE {
	UINT8       PlayCommand;            // (PB_SINGLE_CURR / PB_SINGLE_NEXT / PB_SINGLE_PREV / PB_SINGLE_PRIMARY)
	UINT32      slideEffectFunc;
	UINT8       JumpOffset;             // the JumpOffset of read next\prev file
	UINT16      DirId;                  // the dir-name-id of play current file
	UINT16      FileId;                 // the file-name-id of play current file
} PLAY_CMMD_SINGLE, *PPLAY_CMMD_SINGLE;

typedef struct _PLAY_CMMD_BROWSER {
	UINT8       BrowserCommand;         // (PB_BROWSER_CURR / PB_BROWSER_NEXT / PB_BROWSER_PREV)
	UINT8       BrowserNums;            // How many images in thumbnail mode
	UINT8       JumpOffset;             // the JumpOffset of read next\prev file
} PLAY_CMMD_BROWSER, *PPLAY_CMMD_BROWSER;

typedef struct _PLAY_CMMD_USERJPEG {
	UINT32      SrcJPEGAddr;            // the start addr of jpeg
	UINT32      SrcJPEGSize;            // the file-size of jpeg
	URECT       RectOutWin;             // the rect window of decoded data
	UINT8       DstBuf;
} PLAY_CMMD_USERJPEG, *PPLAY_CMMD_USERJPEG;

typedef struct {
	UINT8      ucEXIFOri;
	BOOL       bDecodeIMG;
} PB_EXIF_ORIENTATION, *PPB_EXIF_ORIENTATION;

// Structure for show specific file in video1/video2 and FB buf/tmpFB and spec size
typedef struct {
	PB_SPEC_VDO PlayFileVideo;      // PLAY_SPEC_FILE_IN_VIDEO_1 / PLAY_SPEC_FILE_IN_VIDEO_2
	PB_SPEC_CLR PlayFileClearBuf;   // PLAY_SPEC_FILE_WITH_CLEAR_BUF / PLAY_SPEC_FILE_WITH_NOT_CLEAR_BUF
	URECT PlayRect;                  // the rect-array of file
	BOOL bDisplayWoReDec;           // TRUE / FALSE
} PLAY_CMMD_SPECFILE, *PPLAY_CMMD_SPECFILE;

typedef struct {
	UINT32  DisplayStart_X;         // crop display start x
	UINT32  DisplayStart_Y;         // crop display start y
	UINT32  DisplayWidth;           // crop display width
	UINT32  DisplayHeight;          // crop display height
} PLAY_CMMD_CROPSAVE, *PPLAY_CMMD_CROPSAVE;

// Bit Rate Control
typedef struct {
	UINT32  uiCompressRatio;
	UINT32  uiUpBoundRatio;
	UINT32  uiLowBoundRatio;
	UINT32  uiReEncCnt;
} PB_BRC_INFO, *PPB_BRC_INFO;

// Capturing screen image for logo\wallpaper
typedef struct {
	UINT32 *puiFileAddr;
	UINT32 *puiFileSize;
	UINT32 uiDstWidth;
	UINT32 uiDstHeight;
} PB_CAPTURESN_INFO;

//#NT#2012/11/13#Scottie -begin
//#NT#Support Dual display
// Display info for Playback
typedef struct _PB_DISPLAY_INFO {
	UINT32 uiDisplayW;              // display width
	UINT32 uiDisplayH;              // display height
	UINT32 uiARatioW;               // aspect ratio of width
	UINT32 uiARatioH;               // aspect ratio of height
} PB_DISPLAY_INFO, *PPB_DISPLAY_INFO;

typedef struct _PB_VDO_INFO {
	UINT16      DstLeftUp_X;        // the start-Left-Up-pixel-X of this first video frame
	UINT16      DstLeftUp_Y;        // the start-Left-Up-pixel-Y of this first video frame
	UINT16      DstDisp_W;          // the display-size-X of this first video frame
	UINT16      DstDisp_H;          // the display-size-Y of this first video frame
} PB_VDO_INFO, *PPB_VDO_INFO;
//#NT#2012/11/13#Scottie -end

typedef struct _PB_USER_EDIT {
	UINT32 uiPBX_EditFunc;          // PBX_Edit function pointer
	UINT32 uiNeedBufSize;           // the needed buffer size
	UINT32 uiParamNum;              // the extra parameter number of user edit effect
	UINT32 *puiParam;               // the extra parameter array of user edit effect
} PB_USER_EDIT, *PPB_USER_EDIT;

typedef struct _PB_HD_INFO{
	UINT32          hd_uiMaxRawSize;
	HD_PATH_ID      *p_hd_vdec_path; //p_hd_vdec_path[0]:JPG, p_hd_vdec_path[1]:H264, p_hd_vdec_path[2]:H265
	HD_VIDEODEC_BS  hd_in_video_bs;
	HD_VIDEO_FRAME  hd_out_video_frame;
	HD_VIDEO_PXLFMT hd_vdo_pix_fmt;
}PB_HD_INFO, *PPB_HD_INFO;

typedef struct _PB_HD_JPG_DEC_INFO {
	HD_PATH_ID      *p_vdec_path; 
	HD_VIDEODEC_BS  *p_in_video_bs;    
	HD_VIDEO_FRAME  *p_out_video_frame;       
} PB_HD_DEC_INFO, *PPB_HD_DEC_INFO;

//-----------------------------------------------------------------------------------------------
// Internal Global Variables
//-----------------------------------------------------------------------------------------------
#if defined __UITRON || defined __ECOS
extern UINT32 _SECTION(".kercfg_data") PLAYBAKTSK_ID;
extern UINT32 _SECTION(".kercfg_data") FLG_ID_PLAYBACK;
extern UINT32 _SECTION(".kercfg_data") FLG_ID_PB_DISP;

extern UINT32 _SECTION(".kercfg_data") PLAYBAKDISPTSK_ID;
extern UINT32 _SECTION(".kercfg_data") FLG_ID_PB_DRAW;
#else
extern THREAD_HANDLE PLAYBAKTSK_ID;
extern ID FLG_ID_PLAYBACK;
extern ID FLG_ID_PB_DISP;

extern THREAD_HANDLE PLAYBAKDISPTSK_ID;
extern ID FLG_ID_PB_DRAW;
#endif

//----- PlaySysData.c ---------------------------------------------------------------------------
extern GXVIDEO_INFO        g_PBVideoInfo;
extern PB_BRC_INFO         g_stPBBRCInfo;
extern PB_CAPTURESN_INFO   g_stPBCSNInfo;
extern PB_EXIF_ORIENTATION gPB_EXIFOri;
extern PLAY_CMMD_SINGLE    gPBCmdObjPlaySingle;
extern PLAY_CMMD_BROWSER   gPBCmdObjPlayBrowser;
extern PLAY_CMMD_USERJPEG  gPBCmdObjPlayUserJPEG;
extern PLAY_CMMD_SPECFILE  gPBCmdObjPlaySpecFile;
extern PLAY_CMMD_CROPSAVE  gPBCmdObjCropSave;
extern PLAY_OBJ            g_PBObj;
extern PLAY_SCALE_INFO     gPlayScaleInfo;
extern PLAYMODE_INFO       gMenuPlayInfo;
extern PB_HD_INFO          g_PbHdInfo;
extern PPB_HD_INFO         g_pPbHdInfo;
extern PB_HD_DEC_INFO      g_HdDecInfo;
extern PB_HD_COM_BUF       g_hd_file_buf;
extern PB_HD_COM_BUF       g_hd_raw_buf;
extern PB_HD_COM_BUF       g_hd_tmp1buf;
extern PB_HD_COM_BUF       g_hd_tmp2buf;
extern PB_HD_COM_BUF       g_hd_tmp3buf;
extern PB_HD_COM_BUF       g_hd_exif_buf;
extern PB_SCREEN_CTRL_TYPE gScreenControlType;
//#NT#2013/04/18#Ben Wang -begin
//#NT#Add callback for decoding image and video files.
extern PB_DECIMG_CB        g_fpDecImageCB;
extern PB_DECVIDEO_CB      g_fpDecVideoCB;
//#NT#2013/04/18#Ben Wang -end
//#NT#2013/07/02#Scottie -begin
//#NT#Add callback for doing something before trigger next (DspSrv)
extern PB_CFG4NEXT_CB      g_fpCfg4NextCB;
//#NT#2013/07/02#Scottie -end
extern PB_SETTING          g_PBSetting;
//#NT#2012/11/14#Scottie -begin
//#NT#Support Dual display
extern PB_DISPLAY_INFO     g_PBDispInfo;
//#NT#2012/11/14#Scottie -end
extern PB_USER_EDIT        g_PBUserEdit;

extern BOOL   g_bPBSaveOverWrite;
extern BOOL   g_bExifExist;
extern UINT8  gucPlayReqtyQuality;
extern UINT8  gucPlayTskWorking;
extern UINT8  gucPlayReEncodeType;
extern UINT8  gucZoomUpdateNewFB;
extern UINT8  gusCurrPlayStatus;
extern UINT8  gucRotatePrimary;
extern UINT8  gucCurrPlayZoomCmmd;
extern UINT16 gusPlayResizeNewWidth, gusPlayResizeNewHeight;
extern UINT16 gusPlayFileFormat, gusPlayThisFileFormat;
extern UINT16 gusPlayZoomUserLeftUp_X, gusPlayZoomUserLeftUp_Y, gusPlayZoomUserRightDown_X, gusPlayZoomUserRightDown_Y;
extern UINT32 g_uiPBRotateDir;
extern UINT32 g_uiPBBufStart, g_uiPBBufEnd;
extern UINT32 guiPlayAllBufSize, guiPlayMaxFileBufSize;
extern UINT32 guiPlayFileBuf, guiPlayRawBuf;
extern UINT32 g_uiPBFBBuf, g_uiPBTmpBuf, g_uiPBIMETmpBuf;
extern UINT32 guiPlayIMETmpBufSize;
extern UINT32 guiUserJPGOutWidth, guiUserJPGOutHeight;
extern UINT32 guiUserJPGOutStartX, guiUserJPGOutStartY;
extern UINT32 guiCurrPlayCmmd;
extern UINT32 guiOnPlaybackMode;
extern UINT32 g_uiMAXPanelSize;
extern UINT32 g_uiMaxFileSize;
extern UINT32 g_uiMaxRawSize;
extern UINT32 g_uiMaxDecodeW;
extern UINT32 g_uiMaxDecodeH;


extern UINT32 g_uiDefaultBGColor;
extern UINT32 g_uiExifOrientation;
//#NT#2012/06/21#Lincy Lin -begin
//#NT#Porting disply Srv
extern UINT32 guiIsSuspendPB;
//#NT#2012/06/21#Lincy Lin -end
extern UINT32 gSrcWidth, gSrcHeight;
extern UINT32 ThumbX[PLAY_MAX_THUMB_NUM], ThumbY[PLAY_MAX_THUMB_NUM];
extern UINT32 ThumbSrcW[PLAY_MAX_THUMB_NUM], ThumbSrcH[PLAY_MAX_THUMB_NUM];
extern UINT32 gPbPushSrcIdx;

extern HD_VIDEO_FRAME  gPlayFBImgBuf;
extern HD_VIDEO_FRAME  gPBImgBuf[PB_DISP_SRC_COUNT];
extern HD_VIDEO_FRAME  *g_pDecodedImgBuf;
extern HD_VIDEO_FRAME  *g_pPlayIMETmpImgBuf;
extern HD_VIDEO_FRAME  *g_pPlayTmpImgBuf;
//#NT#2012/10/29#Scottie -begin
//#NT#Implement g_pThumb1ImgBuf buffer for PZoom Navigator Window
extern HD_VIDEO_FRAME  *g_pThumb1ImgBuf;
//#NT#2012/10/29#Scottie -end
extern IRECT    gSrcRegion;
extern IRECT    gDstRegion;
extern URECT    g_rcPbDetailWIN;
//#NT#2012/11/21#Scottie -begin
//#NT#Support Dual Display for PB
// video output window (draw image in this specific area)
extern URECT    g_PBVdoWIN[PBDISP_IDX_MAX];
extern URECT    g_PB1stVdoRect[PBDISP_IDX_MAX];
extern URECT    g_PBIdxView[PBDISP_IDX_MAX][PLAY_MAX_THUMB_NUM];
//#NT#2012/11/21#Scottie -end

extern PB_DISPTRIG_CB g_fpDispTrigCB;
extern PB_ONDRAW_CB  g_fpOnDrawCB;
extern UINT32        gPbViewState;
extern UINT32        gUserState;
extern BOOL          gDualDisp;
extern BOOL gScaleTwice;
extern UINT32 g_uiPbDebugMsg;

//-----------------------------------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------------------------------
void   PlaybackTsk(void);
UINT32 xPB_GetFILEWithMemo(void);
UINT32 xPB_GetCurFileNameId(void);
UINT32 xPB_GetCurFileFormat(void);
UINT32 xPB_GetCurFileDirId(void);
INT32  xPB_SetPBObject(PPLAY_OBJ pPlayObj);
void   PlaybackDispTsk(void);

//----- PlayFuncHWsetting -----------------------------------------------------------------------
INT32   PB_SysInit(void);
void    PB_SysUnInit(void);
void    PB_TimerStart(UINT32 TimerID, UINT32 TimerValue);
void    PB_TimerPause(UINT32 TimerID);
void    PB_JpgDecodeTimerStartCB(void);
void    PB_JpgDecodeTimerWaitCB(void);
void    PB_JpgDecodeTimerPauseCB(void);
void    PB_PlayTimerOutHdl(UINT32 uiEvent);
//#NT#2012/04/16#Lincy Lin -begin
//#NT#
#if 0
void    PB_ShowSpecFileInV2(PUT_VDO_BUFADDR pBufAddr, PUT_CONFIG_VDO pCfgVDO);
#endif
//#NT#2012/04/16#Lincy Lin -end
void    PB_FrameBufSwitch(PB_DISPLAY_BUFF FBAddrMode);
void    PB_IMEScaleSetting(PPLAY_SCALE_INFO pScale);
void    PB_WARPScaleSetting(PPLAY_SCALE_INFO pScale);
void    PB_FWScale2Display(PPLAY_SCALE_INFO pScale);
ER      PB_DecodeOneImg(PPB_HD_DEC_INFO pHDDecJPG);
UINT32  PB_GetJPEGStatus(void);
INT32   PB_ReEncodeHandle(UINT32 NewImgWidth, UINT32 NewImgHeight, UINT32 NewFileBitsAddr, UINT32 NewFileSize);

//----- PlayFuncByFileSys -----------------------------------------------------------------------
INT32   PB_WaitFileSystemInit(void);
ER      PB_ReadFileByFileSys(UINT32 CommandID, INT8 *pFileBuf, UINT32 BufSize, UINT64 FilePosition, UINT32 JumpOffset);
INT32   PB_WriteFileByFileSys(UINT32 uiNewFileStartAddr, UINT64 ui64NewFileSZ, BOOL bIfOverWrite, BOOL bIfSyncOriTime);
//INT32   PB_GetFileSysStatus(void);
UINT16  PB_ParseFileFormat(void);
UINT32 xPB_GetFormat4FileSys(void);

//----- PlayFuncData2Display --------------------------------------------------------------------
void    PB_Scale2Display(UINT32 SrcWidth, UINT32 SrcHeight, UINT32 SrcStart_X, UINT32 SrcStart_Y, UINT32 FileFormat, UINT32 IfCopy2TmpBuf);
void    PB_Thumb2Display(UINT32 SrcWidth, UINT32 SrcHeight, UINT32 index);
void    PB_HWRotate2Display(UINT32 CurrFBFormat, UINT32 RotateDir, BOOL isFlip);
void    PB_ReScaleForRotate2Display(void);
void    PB_CopyImage2Display(PB_DISPLAY_BUFF SrcBuf, PB_DISPLAY_BUFF DstBuf);
void    PB_MoveImageTo(UINT32 JPGFormat);
//#NT#2012/08/22#Scottie -begin
//#NT#Add for clearing frame buffer
void   xPB_DirectClearFrameBuffer(UINT32 uiBGColor);
//#NT#2012/08/22#Scottie -end

//----- PlayFuncZoom ----------------------------------------------------------------------------
void    PB_DigitalZoom(UINT32 ZoomIN_OUT);
void    PB_DigitalZoomMoving(void);
void    PB_DigitalZoomUserSetting(UINT32 LeftUp_X, UINT32 LeftUp_Y, UINT32 RightDown_X, UINT32 RightDown_Y);

//----- PlayFuncUtility -------------------------------------------------------------------------
INT32   PB_DecodeWhichFile(PB_JPG_DEC_TYPE IsDecodeThumb, UINT32 tmpCurFileFormat);
INT32   PB_DecodeOnePageASF(void);
void    PB_GetDefaultFileBufAddr(void);
UINT32  PB_GetMOVDefaultFileBufAddr(void);//#NT#2010/09/15#Lily Kao -begin
void    PB_GetNewFileBufAddr(void);
INT32   PB_JPGScalarHandler(void);
void    PB_InitThumbOffset(void);
void    PB_RotatePrimaryHandle(UINT32 RotateDir);
BOOL    PB_EXIFOrientationHandle(void);
void    PB_PlaySetCmmd(UINT32 PBCmmd);
void    PB_SetFinishFlag(INT32 Error_check);
INT32   PB_ChkImgAspectRatio(UINT32 ImageWidth, UINT32 ImageHeight, UINT32 PanelWidth, UINT32 PanelHeight);
INT32   PB_AllocPBMemory(UINT32 uiFB_W, UINT32 uiFB_H);
INT32   PB_JpgBRCHandler(HD_VIDEO_FRAME  *pSrcImgBuf, PMEM_RANGE pDstBitstream);
void   xPB_MPOScale2Display(BOOL IsMPO2);
UINT32  PB_GetGxImgRotateDir(UINT32 uiRotateDir);
void   xPB_UpdateEXIFOrientation(INT32 iErrCode);

//----- PlayMainHdl -----------------------------------------------------------------------------
void    PB_Hdl_Init(void);
void    PB_Hdl_DisplayImage(UINT32 PlayCMD);
void    PB_Hdl_AddImage(UINT32 PlayCMD);
void    PB_Hdl_EditImage(UINT32 PlayCMD);
void    PB_Hdl_OpenFile(UINT32 PlayCMD);
void    PB_Hdl_FileSetting(UINT32 PlayCMD);
//void    PB_Hdl_FaceDetection(PLAY_FACE_DETECTION_INFO* pInfo);
void    PB_Hdl_RedEyeRemove(void);
void    PB_Hdl_PlaySpecFileHandle(void);
void    PB_SingleHandle(void);
void    PB_BrowserHandle(void);
INT32   PB_ReResoultHandle(UINT32 IfOverWrite);
INT32   PB_ReQualityHandle(UINT32 IfOverWrite);
INT32   PB_CropSaveHandle(UINT32 IfOverWrite);
INT32   PB_SaveCurrRawBuf(UINT32 SrcStart_X, UINT32 SrcStart_Y, UINT32 SrcWidth, UINT32 SrcHeight, UINT32 IfOverWrite);
INT32   PB_UpdateEXIFHandle(BOOL bIfOverWrite);
BOOL    PB_SetExif_IFD0_Orientation(UINT8 **buf_p);
void    PB_RotateDisplayHandle(BOOL isFlip);
void    PB_ZoomHandle(void);
INT32   PB_SearchValidFile(UINT32 CommandID, INT32 *readFileSts);
INT32   PB_SearchMaxFileId(void);
INT32   PB_DecodePrimaryHandler(UINT32 tmpFileSize, UINT32 CurFormat);
void    PB_Hdl_CaptureScreen(void);
void    PB_UserDispHandle(void);
INT32   PB_UserEditHandle(UINT32 IfOverWrite);

//#NT#2012/05/17#Lincy Lin -begin
//#NT#Porting disply Srv
void    PB_DispSrv_Trigger(void);
void    PB_DispSrv_SetDrawAct(UINT32 DrawAct);
void    PB_DispSrv_Cfg(void);
void    PB_DispSrv_SetDrawCb(PB_VIEW_STATE ViewState);
void    PB_DispSrv_Suspend(void);
void    PB_DispSrv_Resume(void);
//#NT#2012/05/17#Lincy Lin -end
void   xPB_DispSrv_Init4PB(void);

void    PB_SetIMEIDESize(UINT32 uiDispW, UINT32 uiDispH);
//-----------------------------------------------------------------------------------------

//----- PlayHdalUtility -------------------------------------------------------------------
extern UINT32 PB_get_hd_phy_addr(void *pa);
extern HD_RESULT PB_get_hd_common_buf(HD_COMMON_MEM_VB_BLK *pblk, UINT32 *pPa, UINT32 *pVa, UINT32 blk_size);
#endif // _PLAYSYSFUNC_H
