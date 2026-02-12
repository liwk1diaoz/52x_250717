#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "GxGfx/GxGfx.h"

#if (GX_PACK)
#pragma pack (1)
#endif

typedef _PACKED_BEGIN struct _GX_DATA {
	UINT16      signature;  //"GX"
	UINT8       format;     //???
	UINT8       resv;       //
} _PACKED_END GX_DATA;

typedef _PACKED_BEGIN struct _GX_TABLE {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_TABLE
	UINT8       resv;       //
	UINT16      start;
	UINT16      count;
} _PACKED_END GX_TABLE;

typedef _PACKED_BEGIN struct _CHUNK {
	UINT32      offset;     //offset
	UINT32      size;       //size
} _PACKED_END CHUNK;

//  GX_TABLE
//  CHUNK (offset, size)    //uiStartIndex
//  CHUNK (offset, size)    //uiStartIndex+1
//  CHUNK (offset, size)    //uiStartIndex+2
//  CHUNK (offset, size)    //uiStartIndex+3
//  CHUNK (offset, size)    //uiStartIndex+4
//    :
//  CHUNK (offset, size)    //uiStartIndex+uiTotalCount-1
//  DATA POOL

typedef _PACKED_BEGIN struct _GX_BINARY {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_BINARY
	UINT8       resv;       //
} _PACKED_END GX_BINARY;

typedef _PACKED_BEGIN struct _GX_PALETTE {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_PALETTE
	UINT8       bpp;        //bpp
} _PACKED_END GX_PALETTE;

//  GX_PALETTE
//  UINT32 color
//  UINT32 color
//  UINT32 color
//    :
//  UINT32 color

typedef _PACKED_BEGIN struct _GX_MAPPING {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_MAPPING
	UINT8       bpp;        //bpp
} _PACKED_END GX_MAPPING;

//  GX_MAPPING
//  UINT8 index
//  UINT8 index
//  UINT8 index
//    :
//  UINT8 index

typedef _PACKED_BEGIN struct _GX_IMAGETABLE {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_IMAGETABLE_XXX
	UINT8       bpp;        //bpp
} _PACKED_END GX_IMAGETABLE;

typedef _PACKED_BEGIN struct _IMAGE_INFO {
	UINT16      w;          //width
	UINT16      h;          //height
} _PACKED_END IMAGE_INFO;

typedef _PACKED_BEGIN struct _IMAGE_INFO_EX {
	UINT16      w;          //width
	UINT16      h;          //height
	UINT8       format;     //GXFM_IMAGETABLE_XXX
	UINT8       bpp;        //bpp
} _PACKED_END IMAGE_INFO_EX;

typedef _PACKED_BEGIN struct _IMAGE_INFO_EX2 {
	UINT16      w;          //width
	UINT16      h;          //height
	UINT8       format;     //GXFM_IMAGETABLE_XXX
	UINT8       bpp;        //bpp
	UINT32      offset;
	UINT32      size;
} _PACKED_END IMAGE_INFO_EX2;

//  GX_IMAGETABLE
//  GX_TABLE
//  CHUNK (offset, size)    //uiStartIndex
//  CHUNK (offset, size)    //uiStartIndex+1
//  CHUNK (offset, size)    //uiStartIndex+2
//  CHUNK (offset, size)    //uiStartIndex+3
//  CHUNK (offset, size)    //uiStartIndex+4
//    :
//  CHUNK (offset, size)    //uiStartIndex+uiTotalCount-1
//  DATA POOL:
//  IMAGE_INFO+DATA
//  IMAGE_INFO+DATA
//  IMAGE_INFO+DATA
//   :
//  IMAGE_INFO+DATA


typedef _PACKED_BEGIN struct _GX_FONT {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_FONT,
	UINT8       bpp;        //bpp
	UINT16      table_size; //chars per table
	UINT16      table_count;//table counts
	UINT16      w;          //width (max)
	UINT16      h;          //height
	UINT16      w_avg;      //width (avg)
	INT16       y_top;      //y shift to top line
	INT16       y_base;     //y shift to base line
	INT16       y_gap;      //y shift to next y
} _PACKED_END GX_FONT;

typedef _PACKED_BEGIN struct _FONT_TABLE {
	UINT16      i;          //high byte to GX_TABLE index
} _PACKED_END FONT_TABLE;

typedef _PACKED_BEGIN struct _FONT_INFO {
	INT16       a;          //x shift before this
	UINT16      b;          //width
	INT16       c;          //x shift after this
} _PACKED_END FONT_INFO;

//  GX_FONT

//  FONT_TABLE[256] or FONT_TABLE[256]

//  GX_TABLE
//  CHUNK (offset, size)    //uiStartIndex
//  CHUNK (offset, size)    //uiStartIndex+1
//  CHUNK (offset, size)    //uiStartIndex+2
//  CHUNK (offset, size)    //uiStartIndex+3
//  CHUNK (offset, size)    //uiStartIndex+4
//    :
//  CHUNK (offset, size)    //uiStartIndex+uiTotalCount-1

//  DATA POOL 2:
//  FONT_INFO+DATA
//  FONT_INFO+DATA
//  FONT_INFO+DATA
//   :
//  FONT_INFO+DATA



typedef _PACKED_BEGIN struct _GX_STRINGTABLE {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_STRINGTABLE_1BYTE or GXFM_STRINGTABLE_2BYTE
	UINT8       bpc;        //bpc
} _PACKED_END GX_STRINGTABLE;

//  GX_STRINGTABLE
//  GX_TABLE
//  CHUNK (offset, size)    //uiStartIndex
//  CHUNK (offset, size)    //uiStartIndex+1
//  CHUNK (offset, size)    //uiStartIndex+2
//  CHUNK (offset, size)    //uiStartIndex+3
//  CHUNK (offset, size)    //uiStartIndex+4
//    :
//  CHUNK (offset, size)    //uiStartIndex+uiTotalCount-1
//  DATA POOL:
//  DATA
//  DATA
//  DATA
//    :
//  DATA

typedef _PACKED_BEGIN struct _GX_DRAWINGTABLE {
	UINT16      signature;  //"GX"
	UINT8       format;     //GXFM_DRAWINGTABLE
	UINT8       resv;       //
} _PACKED_END GX_DRAWINGTABLE;

typedef _PACKED_BEGIN struct _DRAWING_INFO {
	UINT16      count;      //cmd count
} _PACKED_END DRAWING_INFO;

//  GX_DRAWINGTABLE
//  GX_TABLE
//  CHUNK (offset, size)    //uiStartIndex
//  CHUNK (offset, size)    //uiStartIndex+1
//  CHUNK (offset, size)    //uiStartIndex+2
//  CHUNK (offset, size)    //uiStartIndex+3
//  CHUNK (offset, size)    //uiStartIndex+4
//    :
//  CHUNK (offset, size)    //uiStartIndex+uiTotalCount-1
//  DATA POOL:
//  DRAWING_INFO+DATA
//  DRAWING_INFO+DATA
//  DRAWING_INFO+DATA
//    :
//  DRAWING_INFO+DATA

#if (GX_PACK)
#pragma pack ()
#endif


//signature
#define GXSN                        "GX"

//format
#define GXFM_BINARY                 0x10
#define GXFM_COLOR                  0x20
#define GXFM_PALETTE                0x21
#define GXFM_MAPPING                0x22
#define GXFM_IMAGETABLE_INDEX1      0x30
#define GXFM_IMAGETABLE_INDEX2      0x31
#define GXFM_IMAGETABLE_INDEX4      0x32
#define GXFM_IMAGETABLE_INDEX8      0x33
#define GXFM_IMAGETABLE_RGB565      0x34
#define GXFM_IMAGETABLE_RGBA5654    0x35
#define GXFM_IMAGETABLE_RGBA5658    0x36
#define GXFM_IMAGETABLE_YUV444      0x37
#define GXFM_IMAGETABLE_YUV422      0x38
#define GXFM_IMAGETABLE_YUV420      0x39
#define GXFM_IMAGETABLE_A4          0x3A
#define GXFM_IMAGETABLE_A8          0x3B
#define GXFM_IMAGETABLE_RGBA8888    0x3C
#define GXFM_IMAGETABLE_RGBA4444    0x3D
#define GXFM_IMAGETABLE_RGBA5551    0x3E
#define GXFM_FONT                   0x3F
#define GXFM_STRINGTABLE            0x40
#define GXFM_STRINGTABLE_1BYTE      0x41
#define GXFM_STRINGTABLE_2BYTE      0x42
#define GXFM_DRAWINGTABLE           0x50
#define GXFM_TABLE                  0xF0

#endif //_CHUNK_H_

