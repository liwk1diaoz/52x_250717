/**
	@brief Header file of fileout module.\n
	This file contains the functions which is related to fileout in the chip.

	@file hd_fileout_lib.h

	@ingroup mlib

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef	_HD_FILEOUT_LIB_H_
#define	_HD_FILEOUT_LIB_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_common.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define HD_FILEOUT_MAX_IN         64   ///< max count of input of this device (interface)
#define HD_FILEOUT_MAX_OUT        64   ///< max count of output of this device (interface)
#define HD_FILEOUT_MAX_DATA_TYPE   4   ///< max count of output pool of this device (interface)

/**
    fileout push in buf descriptor fileop.
    note: modified from uitron
*/
#define HD_FILEOUT_FOP_NONE       0x00000000 ///< Do nothing. Used for event only
#define HD_FILEOUT_FOP_CREATE     0x00000001 ///< Create a new file (if old one exists, it will be truncated)
#define HD_FILEOUT_FOP_CLOSE      0x00000002 ///< Close the file
#define HD_FILEOUT_FOP_CONT_WRITE 0x00000004 ///< Write continously (from current position)
#define HD_FILEOUT_FOP_SEEK_WRITE 0x00000008 ///< Write after seeking
#define HD_FILEOUT_FOP_FLUSH      0x00000010 ///< Flush the data right away after writing
#define HD_FILEOUT_FOP_DISCARD    0x00000100 ///< Discard operations which not processed yet
#define HD_FILEOUT_FOP_READ_WRITE 0x00001000 ///< Read data from sysinfo and write continously (from current position)
#define HD_FILEOUT_FOP_SNAPSHOT   (HD_FILEOUT_FOP_CREATE | HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_CLOSE) ///< Create a single file in one operation

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/
#define HD_FILEOUT_SET_COUNT(a, b)		((a)*10)+(b)	///< ex: use HD_FILEOUT_SET_COUNT(1, 5) for setting 1.5

#define HD_LIB_FILEOUT_COUNT      16
#define HD_LIB_FILEOUT_BASE       0
#define HD_LIB_FILEOUT(did)       (HD_LIB_FILEOUT_BASE + (did))
#define HD_LIB_FILEOUT_MAX        (HD_LIB_FILEOUT_BASE + HD_LIB_FILEOUT_COUNT - 1)

#define HD_FILEOUT_CTRL(dev_id)         ((HD_LIB_FILEOUT(dev_id) << 16) | HD_CTRL)
#define HD_FILEOUT_IN(dev_id, in_id)    ((HD_LIB_FILEOUT(dev_id) << 16) | (((in_id) & 0x00ff) << 8))
#define HD_FILEOUT_OUT(dev_id, out_id)  ((HD_LIB_FILEOUT(dev_id) << 16) | ((out_id) & 0x00ff))

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
    fileout callback event type.
    note: modified from uitron
*/
typedef enum _HD_FILEOUT_CB_EVENT{
	HD_FILEOUT_CB_EVENT_NAMING          = 0x00000001,    ///< callback for file naming
	HD_FILEOUT_CB_EVENT_OPENED          = 0x00000002,    ///< callback for file falloc
	HD_FILEOUT_CB_EVENT_CLOSED          = 0x00000004,    ///< callback for file closed
	HD_FILEOUT_CB_EVENT_DELETE          = 0x00000008,    ///< callback for file delete
	HD_FILEOUT_CB_EVENT_FS_ERR          = 0x00000010,    ///< callback for fout error
	ENUM_DUMMY4WORD(HD_FILEOUT_CB_EVENT)
} HD_FILEOUT_CB_EVENT;

/**
    fileout callback error code.
    note: modified from uitron
*/
typedef enum _HD_FILEOUT_ERRCODE {
	HD_FILEOUT_ERRCODE_NONE             = 0x00000000,    ///< none
	HD_FILEOUT_ERRCODE_CARD_SLOW        = 0x00000001,    ///< error code of card slow
	HD_FILEOUT_ERRCODE_CARD_WR_ERR      = 0x00000002,    ///< error code of card read/write error
	HD_FILEOUT_ERRCODE_LOOPREC_FULL     = 0x00000004,    ///< error code of loop record full
	HD_FILEOUT_ERRCODE_SNAPSHOT_ERR     = 0x00000008,    ///< error code of snapshot error
	HD_FILEOUT_ERRCODE_FOP_SLOW         = 0x00000010,    ///< error code of fileop slow
	ENUM_DUMMY4WORD(HD_FILEOUT_ERRCODE)
} HD_FILEOUT_ERRCODE;

/**
    fileout callback return value.
*/
typedef enum _HD_FILEOUT_RETVAL {
	HD_FILEOUT_RETVAL_DONE              = 0x00000000,    ///< return for normal
	HD_FILEOUT_RETVAL_NOFREE_SPACE      = 0x00000001,    ///< return for no free space
	HD_FILEOUT_RETVAL_PARAM_ERR         = 0x00000002,    ///< return for parameter error
	HD_FILEOUT_RETVAL_DELETE_FAIL       = 0x00000004,    ///< return for delete file fail
	ENUM_DUMMY4WORD(HD_FILEOUT_RETVAL)
} HD_FILEOUT_RETVAL;

/**
    fileout file event definitions.
    @note: modified from uitron (should be the same as HD_BSMUX_FEVENT_XXX)
*/
typedef enum _HD_FILEOUT_FEVENT {
	HD_FILEOUT_FEVENT_NORMAL            = 0x00000000,    ///< normal recording
	HD_FILEOUT_FEVENT_EMR               = 0x00000001,    ///< emergency recording
	HD_FILEOUT_FEVENT_BSINCARD          = 0x00000002,    ///< stored in cards temporarily
	ENUM_DUMMY4WORD(HD_FILEOUT_FEVENT)
} HD_FILEOUT_FEVENT;

/**
    fileout file type definitions.
    @note: modified from uitron (should be the same as MEDIA_FILEFORMAT_XXX)
*/
typedef enum _HD_FILEOUT_FTYPE {
	HD_FILEOUT_FTYPE_MOV                = 0x00000001,    ///< MOV file format
	HD_FILEOUT_FTYPE_MP4                = 0x00000004,    ///< MP4 file format
	HD_FILEOUT_FTYPE_TS                 = 0x00000008,    ///< TS file format
	HD_FILEOUT_FTYPE_JPG                = 0x00000010,    ///< JPG file format
	ENUM_DUMMY4WORD(HD_FILEOUT_FTYPE)
} HD_FILEOUT_FTYPE;

/**
    fileout callback information.
    note: modified from uitron
*/
typedef struct _HD_FILEOUT_CBINFO{
	UINT32 iport;                       ///< port id
	UINT32 event;                       ///< event defined by the input unit [NAMING ONLY]
	UINT32 type;                        ///< file type, MP4, TS, JPG, THM    [NAMING ONLY]
	char *p_fpath;                      ///< file path
	UINT32 fpath_size;                  ///< length of file path
	BOOL is_reuse;                      ///< resue flag of current file      [NAMING ONLY]
	UINT64 alloc_size;                  ///< falloc size of current file     [OPENED ONLY]
	UINT32 port_num;                    ///< number of active port(s)        [DELETE ONLY]
	UINT64 remain_size;                 ///< remain size of current file     [DELETE ONLY]
	CHAR drive;                         ///< drive name
	UINT32 fop;                         ///< fileop
	UINT32 dur;                         ///< duration of fileop
	HD_FILEOUT_ERRCODE errcode;         ///< fileout callback error code     [FS_ERR ONLY]
	HD_FILEOUT_CB_EVENT cb_event;       ///< fileout callback event type
	HD_FILEOUT_RETVAL ret_val;          ///< fileout callback return value
} HD_FILEOUT_CBINFO;

/**
    fileout callback funcion prototype.

    @param CHAR *              p_name
    @param HD_FILEOUT_CBINFO * cbinfo
    @param UINT32 *            param
*/
typedef INT32 (*HD_FILEOUT_CALLBACK)(CHAR *p_name, HD_FILEOUT_CBINFO *cbinfo, UINT32 *param);

/**
    fileout push in buf descriptor.
    note: modified from uitron
*/
#define HD_FILEOUT_DESC_SIZE  38 ///< size of data desc in WORDS
typedef struct _HD_FILEOUT_BUF {
	UINT32              sign;                       ///< MAKEFOURCC('F','O','U','T')
	UINT32              event;                      ///< defined by the input unit
	UINT32              fileop;                     ///< bitwise: open/close/conti_write or seek_write/flush/none(event only)/discard
	UINT32              addr;                       ///< write data address
	UINT64              size;                       ///< write data size
	UINT64              pos;                        ///< only valid if seek_write
	UINT32              type;                       ///< file type, MP4, TS, JPG, THM
	CHAR                *p_fpath;                   ///< Specify file path (if NULL, it will callback project to get)
	UINT32              fpath_size;                 ///< Length of file path
	INT32               (*fp_pushed)(void *p_data); ///< callback for previous module release buffer
	INT32               ret_push;                   ///< fileop result [ER/INT32]
	UINT32              pathID;					    ///< keep path id
	UINT32              resv[24];                   ///< reserve to meet ImageStream DESC size
} HD_FILEOUT_BUF;
STATIC_ASSERT(sizeof(HD_FILEOUT_BUF) / sizeof(UINT32) == HD_FILEOUT_DESC_SIZE);

/**
    fileout config info.
*/
typedef struct _HD_FILEOUT_CONFIG{
	UINT32 drive;                      ///< drive name
	UINT64 max_pop_size;               ///< max pop size of output
	UINT32 format_free;                ///< format free feature
	UINT32 use_mem_blk;                ///< use meme blk for read_write ops
	UINT32 wait_ready;                 ///< wait until ready is set again
	UINT32 close_on_exec;              ///< enable the close-on-exec flag
	UINT64 max_file_size;              ///< max file size
} HD_FILEOUT_CONFIG;

/**
    fileout callback info.
    note: internal use
*/
typedef struct _HD_FILEOUT_REG_CALLBACK {
	HD_FILEOUT_CALLBACK callbackfunc;
	UINT32 version;
} HD_FILEOUT_REG_CALLBACK;

/**
    fileout param id.
*/
typedef enum _HD_FILEOUT_PARAM_ID {
	HD_FILEOUT_PARAM_MIN      = 0,
	HD_FILEOUT_PARAM_REG_CALLBACK,     ///< support set with ctrl path, using HD_FILEOUT_CALLBACK
	HD_FILEOUT_PARAM_CONFIG,           ///< support get/set with i/o path, using HD_FILEOUT_CONFIG struct
	HD_FILEOUT_PARAM_MAX,
	ENUM_DUMMY4WORD(HD_FILEOUT_PARAM_ID)
} HD_FILEOUT_PARAM_ID;

/**
    NOTE: for hd_fileout_open(),

    Use HD_FILEOUT_CTRL for device control.
    ex: hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl);

    Use HD_FILEOUT_IN/HD_FILEOUT_OUT for device i/o path.
    ex: hd_fileout_open(HD_FILEOUT_IN(0, 0), HD_FILEOUT_OUT(0, 0), &fileout_path);
*/

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT hd_fileout_init(VOID);
HD_RESULT hd_fileout_open(HD_IN_ID in_id, HD_OUT_ID out_id, HD_PATH_ID* p_path_id);
HD_RESULT hd_fileout_start(HD_PATH_ID path_id);
HD_RESULT hd_fileout_stop(HD_PATH_ID path_id);
HD_RESULT hd_fileout_close(HD_PATH_ID path_id);
HD_RESULT hd_fileout_get(HD_PATH_ID path_id, HD_FILEOUT_PARAM_ID id, VOID *p_param);
HD_RESULT hd_fileout_set(HD_PATH_ID path_id, HD_FILEOUT_PARAM_ID id, VOID *p_param);
HD_RESULT hd_fileout_push_in_buf(HD_PATH_ID path_id, HD_FILEOUT_BUF* p_user_in_buf, INT32 wait_ms);
HD_RESULT hd_fileout_uninit(VOID);

#endif //_HD_FILEOUT_LIB_H_

