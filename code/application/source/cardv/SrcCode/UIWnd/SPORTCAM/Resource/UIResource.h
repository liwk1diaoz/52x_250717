#ifndef _UI_RESORUCE_H_
#define _UI_RESORUCE_H_
#include "GxGfx/GxGfx.h"

#include "DemoKit_String.h"
#include "DemoKit_Image.h"
#include "DemoKit_Font.h"
#include "DemoKit_Palette.h"
#include "BG_Opening.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Palette

#define _OSD_INDEX_TRANSPART             0
#define _OSD_INDEX_BLUE                  1
#define _OSD_INDEX_WHITE                 2
#define _OSD_INDEX_BLACK                 3
#define _OSD_INDEX_GRAY75                8
#define _OSD_INDEX_DARKGRAY              9
#define _OSD_INDEX_ORANGE                15
#define _OSD_INDEX_GREEN                 13
#define _OSD_INDEX_GRAY50                4
#define _OSD_INDEX_RED                   14
#define _OSD_INDEX_LIGHTGREEN            12
#define _OSD_INDEX_YELLOW                16
#define _OSD_INDEX_GRAY                  6
//#define _OSD_INDEX_PINK                  64
//#define _OSD_INDEX_PURPLE                62
#define _OSD_INDEX_BLACK50               0x3E
#define _OSD_INDEX_BLACK75               0x3F
#define _OSD_INDEX_WHITE50               0x40


///////////////////////////////////////////////////////////////////////////////
//
//  Font


///////////////////////////////////////////////////////////////////////////////
//
//  Image (JPG)
///////////////////////////////////////////////////////////////////////////////
//
//  String (LANG)


extern UINT32 Get_LanguageTable(void);

//temp define for compile
#define ICONID_NULL_TMP             0xFFFF
#define ICON_MENU_SETUP_ON          ICONID_NULL

#define STRID_USER_START              0xF000

//MOVIE_SIZE_TAG
enum _STRID_USER_ {
	STRID_MCTF =                  STRID_USER_START,
	STRID_EDGE,
	STRID_NR,
	STRID_WIFI_ETH,
	STRID_ISO6400,
	STRID_ISO12800,
	STRID_FRONT_2880x2160P50,                   /// UHD P50 (DAR)
	STRID_FRONT_3840x2160P30,                   /// UHD P30
	STRID_FRONT_2880x2160P24,                   /// UHD P24 (DAR)
	STRID_FRONT_2704x2032P60,                   /// 2.7K P60
	STRID_FRONT_2560x1440P80,                   /// QHD P80
	STRID_FRONT_2560x1440P60,                   /// QHD P60
	STRID_FRONT_2560x1440P30,                   /// QHD P30
	STRID_FRONT_2304x1296P30,                   /// 3MHD P30
	STRID_FRONT_1920x1080P120,                  /// FHD P120
	STRID_FRONT_1920x1080P96,                   /// FHD P96
	STRID_FRONT_1920x1080P60,                   /// FHD P60
	STRID_FRONT_1920x1080P30,                   /// FHD P30
	STRID_FRONT_1280x720P240,                   /// HD P240
	STRID_FRONT_1280x720P120,                   /// HD P120
	STRID_FRONT_1280x720P60,                    /// HD P60
	STRID_FRONT_1280x720P30,                    /// HD P30
	STRID_FRONT_848x480P30,                     /// WVGA P30
	STRID_FRONT_640x480P240,                    /// VGA P240
	STRID_FRONT_640x480P30,                     /// VGA P30
	STRID_FRONT_320x240P30,                     /// QVGA P30
	STRID_DUAL_3840x2160P30_1920x1080P30,		/// UHD FHD
	STRID_DUAL_2560x1440P30_1280x720P30,		/// QHD HD
	STRID_DUAL_2560x1440P30_1920x1080P30,		/// QHD FHD
	STRID_DUAL_2304x1296P30_1280x720P30,		/// 3MHD HD
	STRID_DUAL_1080P30_1080P30,                 /// FHD FHD
	STRID_DUAL_1920x1080P30_1280x720P30,        /// FHD HD
	STRID_DUAL_1920x1080P30_848x480P30,         /// FHD WVGA
	STRID_TRI_1920x1080P30,					/// 3 x FHD
	STRID_TRI_2560x1440P30_1920x1080P30_1920x1080P30, /// QHD + 3 x FHD
	STRID_QUAD_1920x1080P30,      				   /// 4 x FHD
	STRID_CLONE_1920x1080P30_1920x1080P30,      /// FHD FHD
	STRID_CLONE_1920x1080P30_1280x720P30,       /// FHD HD
	STRID_CLONE_2560x1440P30_848x480P30,        /// QHD WVGA
	STRID_CLONE_2304x1296P30_848x480P30,        /// 3MHD WVGA
	STRID_CLONE_1920x1080P60_848x480P30,        /// FHD WVGA
	STRID_CLONE_1920x1080P60_640x360P30,        /// FHD VGA
	STRID_CLONE_1920x1080P30_848x480P30,        /// FHD WVGA
	STRID_CLONE_2048x2048P30_480x480P30,
	STRID_BOTH2,
	STRID_CAP_SIDE,
    STRID_CAP_BURST_30,
    STRID_5MWXH_USR,
    STRID_CODEC,
    STRID_MJPG,
    STRID_H264,
    STRID_H265,
    STRID_LIVESTREAM,
    STRID_QRCODE_SCAN,
    STRID_CONNECT_TO_HOTSPOT,
    STRID_NOT_FIND_HOTSPOT,
    STRID_HOTSPOT_IS_BROKEN,
    STRID_RSC_EFFECT,
    STRID_SUPER_HIGH,
    STRID_40MWXH_USR,
	STRID_USER_END
};

extern char *UIRes_GetUserString(UINT32 TxtId);
#endif //_UI_RESORUCE_H_
