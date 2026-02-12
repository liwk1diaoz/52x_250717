/**
	@brief 		filein library.\n

	@file 		hd_filein_lib.c

	@ingroup 	mFileIn

	@note 		Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "hdal.h"
#include "filein.h"
#include "hd_filein_lib.h"

#define HD_GET_DEV(id)					((id) >> 16)
#define HD_GET_IN(id)					(((id) & 0xff00) >> 8)
#define HD_GET_OUT(id)					((id) & 0x00ff)
#define HD_GET_CTRL(id)					((id) & 0xffff)

//todo
#define OUT_BASE              0
#define HD_OUT_BASE  		0x01
#define HD_OUT_MAX			(HD_OUT_BASE+128-1)
#define OUT_COUNT                      16

#define IN_BASE              128
#define HD_IN_BASE  		0x01
#define HD_IN_MAX			(HD_IN_BASE+128-1)
#define IN_COUNT                      16

#define DEV_COUNT                         2
#define DEV_BASE                            0
#define HD_DEV_BASE				0x3200
#define HD_DEV(did)				(HD_DEV_BASE + (did))
#define HD_DEV_MAX				(HD_DEV_BASE + 16 - 1)

//static UINT32				_max_path_count = 16;
static UINT32				gFileInPortOpenStatus[ISF_FILEIN_OUT_NUM] = {0};
static UINT32				gFileInPortOpenCount = 0;

// TS format
static UINT32				gISF_FileInFileFormat[ISF_FILEIN_IN_NUM] = {0};

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;	\
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) { \
			UINT32 id = (dev_id) - HD_DEV_BASE; \
			if(id < DEV_COUNT) { \
				(dev_id) = DEV_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_OUT_ID(out_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((out_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((out_id) >= HD_OUT_BASE && (out_id) <= HD_OUT_MAX) { \
			UINT32 id = (out_id) - HD_OUT_BASE; \
			if((id < OUT_COUNT) && (id < _max_path_count)) { \
				(out_id) = OUT_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_IN_ID(in_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((in_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((in_id) >= HD_IN_BASE && (in_id) <= HD_IN_MAX) { \
			UINT32 id = (in_id) - HD_IN_BASE; \
			if((id < OUT_COUNT) && (id < _max_path_count)) { \
				(in_id) = IN_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

HD_RESULT hd_filein_open(HD_PATH_ID path_id)
{
	// Check whether port was opened?
	if (gFileInPortOpenStatus[path_id] == TRUE) {
		DBG_ERR("Port %d is already opened!\r\n");
		return HD_ERR_UNINIT;
	} else {
		gFileInPortOpenStatus[path_id] = TRUE;
	}

	// Check whether is first time opening?
	if (gFileInPortOpenCount == 0) {
		FileIn_Open(path_id);
		FileIn_Action(path_id, FILEIN_ACTION_PRELOAD);
	}

	gFileInPortOpenCount++;

    return HD_OK;
}

HD_RESULT hd_filein_start(HD_PATH_ID path_id)
{
	return HD_OK;
}

HD_RESULT hd_filein_stop(HD_PATH_ID path_id)
{
	return HD_OK;
}

HD_RESULT hd_filein_close(HD_PATH_ID path_id)
{
	UINT32 port_no = path_id;

//	HD_DAL self_id = HD_GET_DEV(path_id);
//	HD_IO in_id = HD_GET_IN(path_id);
//	HD_IO out_id = HD_GET_OUT(path_id);
//	HD_IO ctrl_id = HD_GET_CTRL(path_id);
//	HD_RESULT rv = HD_ERR_NG;
//
//	DBG_DUMP("hd_filein_close:\n");
//	DBG_DUMP("    path_id(0x%x)\n", path_id);


//	if(ctrl_id == HD_CTRL) {
//		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
//		//do nothing
//		return HD_OK;
//	}

	//if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
	/* Show warning instead of return error to release filein resource. */
//	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	DBG_WRN("    self_id error(%d)\n", rv);/*return rv;*/}
//	_HD_CONVERT_OUT_ID(out_id, rv);		if(rv != HD_OK) {	DBG_WRN("    out_id error(%d)\n", rv);/*return rv;*/}
//	_HD_CONVERT_IN_ID(in_id, rv);		if(rv != HD_OK) {	DBG_WRN("    in_id error(%d)\n", rv);/*return rv;*/}


	// Check whether port was closed?
	if (gFileInPortOpenStatus[port_no] == FALSE) {
		DBG_ERR("Port %d is already closed!\r\n");
		return HD_ERR_NOT_OPEN;
	} else {
		gFileInPortOpenStatus[port_no] = FALSE;
	}

	// Check whether is last time closing?
	if (gFileInPortOpenCount > 1) {
		gFileInPortOpenCount --;
		return HD_OK;
	} else {
		port_no = 0;
		FileIn_Close(port_no);
		gFileInPortOpenCount = 0;
	}

	return HD_OK;
}

HD_RESULT hd_filein_get(HD_PATH_ID path_id, UINT32 param, UINT32 *value)
{
	MEM_RANGE* mem_range = NULL;
	HD_FILEIN_BLKINFO filein_blkinfo = {0};

	switch (param) {
	case HD_FILEIN_PARAM_MEM_RANGE:
		FileIn_GetParam(path_id, FILEIN_PARAM_MEM_RANGE, (UINT32 *)&mem_range);
		memcpy((void *)value, (void *)mem_range, sizeof(MEM_RANGE));
		break;
	case HD_FILEIN_PARAM_BLK_INFO:
		FileIn_GetParam(path_id, FILEIN_PARAM_BLK_INFO, (UINT32 *)&filein_blkinfo);
		memcpy((void *)value, (void *)&filein_blkinfo, sizeof(HD_FILEIN_BLKINFO));
		break;
	default:
		DBG_ERR("Not a valid port paramter (0x%x)\r\n", param);
		break;
	}

	return HD_OK;
}

HD_RESULT hd_filein_set(HD_PATH_ID path_id, UINT32 param, UINT32 value)
{
	DBG_IND("parm=0x%x\r\n", param);

	switch (param) {
	case HD_FILEIN_PARAM_EVENT_CB:
		FileIn_SetParam(path_id, FILEIN_PARAM_EVENT_CB, value);
		break;

	case HD_FILEIN_PARAM_FILEHDL:
		FileIn_SetParam(path_id, FILEIN_PARAM_FILEHDL, value);
		break;

	case HD_FILEIN_PARAM_FILESIZE:
		FileIn_SetParam(path_id, FILEIN_PARAM_FILESIZE, value);
		break;

	case HD_FILEIN_PARAM_FILEDUR:
		FileIn_SetParam(path_id, FILEIN_PARAM_FILEDUR,	value);
		break;

	case HD_FILEIN_PARAM_FILEFMT:
		gISF_FileInFileFormat[path_id] = value;
		FileIn_SetParam(path_id, FILEIN_PARAM_FILEFMT,	value);
		break;

	case HD_FILEIN_PARAM_BLK_TIME:
		FileIn_SetParam(path_id, FILEIN_PARAM_BLK_TIME, value);
		break;

	case HD_FILEIN_PARAM_PL_BLKNUM:
		FileIn_SetParam(path_id, FILEIN_PARAM_PL_BLKNUM, value);
		break;

	case HD_FILEIN_PARAM_TT_BLKNUM:
		FileIn_SetParam(path_id, FILEIN_PARAM_TT_BLKNUM, value);
		break;

	case HD_FILEIN_PARAM_CONTAINER:
		FileIn_SetParam(path_id, FILEIN_PARAM_CONTAINER, value);
		break;

	case HD_FILEIN_PARAM_VDO_FR:
		FileIn_SetParam(path_id, FILEIN_PARAM_VDO_FR, value);
		break;

	case HD_FILEIN_PARAM_AUD_FR:
		FileIn_SetParam(path_id, FILEIN_PARAM_AUD_FR, value);
		break;

	case HD_FILEIN_PARAM_VDO_TTFRM:
		FileIn_SetParam(path_id, FILEIN_PARAM_VDO_TTFRM, value);
		break;

	case HD_FILEIN_PARAM_AUD_TTFRM:
		FileIn_SetParam(path_id, FILEIN_PARAM_AUD_TTFRM, value);
		break;

	case HD_FILEIN_PARAM_USER_BLK_DIRECTLY:
		FileIn_SetParam(path_id, FILEIN_PARAM_USER_BLK_DIRECTLY, value);
		break;

	case HD_FILEIN_PARAM_BLK_SIZE:
		FileIn_SetParam(path_id, FILEIN_PARAM_BLK_SIZE, value);
		break;

	default:
		DBG_ERR("Not a valid paramter (0x%x)\r\n", param);
		return HD_ERR_PARAM;
	}

	return HD_OK;
}

HD_RESULT hd_filein_push_in_buf(HD_PATH_ID path_id, UINT32 *pBuf, INT32 wait_ms)
{
	if(FileIn_In(path_id, pBuf) != E_OK)
		return HD_ERR_SYS;
	else
		return HD_OK;
}

HD_RESULT hd_filein_pull_out_buf(HD_PATH_ID path_id, UINT32 *pBuf, INT32 wait_ms)
{
	return HD_OK;
}

HD_RESULT hd_filein_uninit(VOID)
{
	return HD_OK;
}
