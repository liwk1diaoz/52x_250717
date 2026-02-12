/**
	@brief 		fileout library.\n

	@file 		hd_fileout_lib.c

	@ingroup 	mFileOut

	@note 		Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include "hdal.h"

// INCLUDE PROTECTED
#include "fileout_init.h"
unsigned int FILEOUT_debug_level = NVT_DBG_WRN;
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_FILEOUT_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_FILEOUT // 0
#define DEV_COUNT       FILEOUT_DRV_NUM  // support drive 'A', 'B'
#define IN_BASE         ISF_IN_BASE      // 128
#define IN_COUNT        16
#define OUT_BASE        ISF_OUT_BASE     // 0
#define OUT_COUNT       16
#define PARAM_NUM       FILEOUT_DRV_NUM // support drive 'A', 'B'

#define HD_DEV_BASE		0x3100
#define HD_DEV(did)		(HD_DEV_BASE + (did))
#define HD_DEV_MAX		(HD_DEV_BASE + 16 - 1)

//notice: max path num 16 / max active port num 6
#define HD_MAX_PATH_NUM	OUT_COUNT

// INTERNAL DEBUG
#define DEBUG_COMMON    DISABLE
#define HD_FILEOUT_FUNC(fmtstr, args...) do { if(DEBUG_COMMON) DBG_DUMP("%s(): " fmtstr, __func__, ##args); } while(0)
#define HD_FILEOUT_DUMP(fmtstr, args...) do { if(DEBUG_COMMON) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
//                                            set/get             set/get/start
//                                              ^ |                  ^ |
//                                              | v                  | v
//  [UNINIT] -- init   --> [INIT] -- open  --> [IDLE]  -- start --> [RUN]
//          <-- uninit --          <-- close --         <-- stop  --
//
typedef enum _HD_FILEOUT_STAT {
	HD_FILEOUT_STAT_UNINIT = 0,
	HD_FILEOUT_STAT_INIT,
	HD_FILEOUT_STAT_IDLE,
	HD_FILEOUT_STAT_RUN,
} HD_FILEOUT_STAT;

typedef struct _FILEOUT_INFO_PRIV {
	HD_FILEOUT_STAT           status;
	BOOL                      b_maxmem_set;
	BOOL                      b_need_update_param[PARAM_NUM];
} FILEOUT_INFO_PRIV;

typedef struct _FILEOUT_INFO_PORT {
	HD_FILEOUT_CONFIG         fout_config;
	HD_FILEOUT_REG_CALLBACK   fout_callback;

	//private data
	FILEOUT_INFO_PRIV         priv;
} FILEOUT_INFO_PORT;

typedef struct _FILEOUT_INFO {
	FILEOUT_INFO_PORT *       port;
} FILEOUT_INFO;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
static UINT32 _max_path_count = 0;

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

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
FILEOUT_INFO fileout_info[DEV_COUNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

void _hd_fileout_cfg_max(UINT32 maxpath)
{
	if (!fileout_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > HD_MAX_PATH_NUM) _max_path_count = HD_MAX_PATH_NUM;
	}
}

INT _hd_fileout_check_path_id(HD_PATH_ID path_id)
{
	#if 0
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	#endif
	INT ret = 0;
	#if 0
	//convert self_id
	//_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) { return rv;}

	//convert out_id
	//if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
	//_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { return rv;}

	//convert in_id
	//if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { return HD_ERR_IO;}
	//_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
	#endif
	return ret;
}

INT _hd_fileout_check_status(HD_PATH_ID path_id, HD_FILEOUT_STAT status)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (fileout_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status != status) {
		return -1;
	}

	return ret;
}

INT _hd_fileout_set_status(HD_PATH_ID path_id, HD_FILEOUT_STAT status)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	fileout_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = status;

	return ret;
}

static INT _hd_fileout_verify_param_HD_FILEOUT_CONFIG(HD_FILEOUT_CONFIG *p_setting, CHAR *p_ret_string, INT max_str_len)
{
	// drive
	if ((p_setting->drive < FILEOUT_DRV_NAME_FIRST) || (p_setting->drive > FILEOUT_DRV_NAME_LAST)) {
		DBG_DUMP("fileout set drive (%d) error\r\n", p_setting->drive);
		goto exit;
	}
	// max_pop_size changed to check lower limit
	if (p_setting->max_pop_size < (1024*1024UL)) {
		DBG_DUMP("fileout set max_pop_size (%d) error\r\n", (int)p_setting->max_pop_size);
		goto exit;
	}
	// format_free
	if ((p_setting->format_free != 0) && (p_setting->format_free != 1)) {
		DBG_DUMP("fileout set format_free (%d) error\r\n", p_setting->format_free);
		goto exit;
	}
	// use_mem_blk
	if ((p_setting->use_mem_blk != 0) && (p_setting->use_mem_blk != 1)) {
		DBG_DUMP("fileout set use_mem_blk (%d) error\r\n", p_setting->use_mem_blk);
		goto exit;
	}
	// wait_ready
	if ((p_setting->wait_ready != 0) && (p_setting->wait_ready != 1)) {
		DBG_DUMP("fileout set wait_ready (%d) error\r\n", p_setting->wait_ready);
		goto exit;
	}
	// clo_exec
	if ((p_setting->close_on_exec != 0) && (p_setting->close_on_exec != 1)) {
		DBG_DUMP("fileout set close_on_exec (%d) error\r\n", p_setting->close_on_exec);
		goto exit;
	}
	#if 0
	// max_file_size
	if (p_setting->max_file_size > FILEOUT_QUEUE_MAX_FILE_SIZE) {
		DBG_DUMP("fileout set max_file_size (%d) error\r\n", p_setting->max_file_size);
		goto exit;
	}
	#endif

	return 0;
exit:
	return -1;
}

INT _hd_fileout_check_params_range(HD_FILEOUT_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	INT ret = 0;
	switch (id) {
	case HD_FILEOUT_PARAM_REG_CALLBACK:
		break;
	// Config
	case HD_FILEOUT_PARAM_CONFIG:
		ret = _hd_fileout_verify_param_HD_FILEOUT_CONFIG((HD_FILEOUT_CONFIG *)p_param, p_ret_string, max_str_len);
		break;
	default:
		break;
	}
	return ret;
}

VOID _hd_fileout_get_param(HD_FILEOUT_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	switch (id) {

	case HD_FILEOUT_PARAM_CONFIG:
		{
			HD_FILEOUT_CONFIG *p_info = (HD_FILEOUT_CONFIG *)p_param;

			fileout_queue_get_max_pop_size(&p_info->max_pop_size);
			p_info->format_free = (UINT32)fileout_util_is_formatfree();
			p_info->use_mem_blk = (UINT32)fileout_util_is_usememblk();
			p_info->wait_ready = (UINT32)fileout_util_is_wait_ready();
			p_info->close_on_exec = (UINT32)fileout_util_is_close_on_exec();
			fileout_queue_get_max_file_size(&p_info->max_file_size);
			HD_FILEOUT_DUMP("FILEOUT CONFIG: drive=%c max_pop_size=%x max_file_size=0x%x\r\n",
				p_info->drive, (unsigned int)p_info->max_pop_size, (unsigned int)p_info->max_file_size);
			HD_FILEOUT_DUMP("FILEOUT CONFIG: format_free=%d use_mem_blk=%d wait_ready=%d close_on_exec=%d\r\n",
				p_info->format_free, p_info->use_mem_blk, p_info->wait_ready, p_info->close_on_exec);
		}
		break;

	default:
		break;
	}
	return ;
}

VOID _hd_fileout_set_param(HD_FILEOUT_PARAM_ID id, UINT32 idx, VOID *p_param)
{
	switch (id) {

	case HD_FILEOUT_PARAM_CONFIG:
		{
			HD_FILEOUT_CONFIG *p_info = (HD_FILEOUT_CONFIG *)p_param;

			fileout_queue_set_max_pop_size(p_info->max_pop_size);
			fileout_util_set_formatfree((INT32)p_info->format_free);
			fileout_util_set_usememblk((INT32)p_info->use_mem_blk);
			fileout_util_set_wait_ready((INT32)p_info->wait_ready);
			fileout_util_set_close_on_exec((INT32)p_info->close_on_exec);
			fileout_queue_set_max_file_size(p_info->max_file_size);

			HD_FILEOUT_DUMP("FILEOUT CONFIG: drive=%c max_pop_size=%x max_file_size=0x%x\r\n",
				p_info->drive, (unsigned int)p_info->max_pop_size, (unsigned int)p_info->max_file_size);
			HD_FILEOUT_DUMP("FILEOUT CONFIG: format_free=%d use_mem_blk=%d wait_ready=%d close_on_exec=%d\r\n",
				p_info->format_free, p_info->use_mem_blk, p_info->wait_ready, p_info->close_on_exec);
		}
		break;

	default:
		break;
	}
	return ;
}

HD_RESULT _hd_fileout_set_all_default_params_to_unit(UINT32 self_id, UINT32 out_id)
{
	UINT32 dev  = self_id - DEV_BASE;
	UINT32 port = out_id- OUT_BASE;
	FILEOUT_INFO *p_fileout_info = &fileout_info[dev];

	//Config: init values
	_hd_fileout_set_param(HD_FILEOUT_PARAM_CONFIG, out_id, &p_fileout_info->port[port].fout_config);

	return HD_OK;
}

VOID _hd_fileout_fill_param(HD_FILEOUT_PARAM_ID id, VOID *p_out_param, VOID *p_in_param)
{
	switch (id) {

	case HD_FILEOUT_PARAM_CONFIG:
		{
			HD_FILEOUT_CONFIG *p_info = (HD_FILEOUT_CONFIG *)p_out_param;
			if (p_in_param == NULL) {
				p_info->drive = FILEOUT_DRV_NAME_FIRST;  // 'A'
				p_info->max_pop_size = 4*1024*1024UL;    // 4MB
				p_info->format_free = 0;                 // close
				p_info->use_mem_blk = 0;                 // close
				p_info->wait_ready = 0;                 // close
				p_info->close_on_exec = 1;              // enable
				p_info->max_file_size = FILEOUT_QUEUE_MAX_FILE_SIZE;    // 4GB
			} else {
				HD_FILEOUT_CONFIG *p_setting = (HD_FILEOUT_CONFIG *)p_in_param;
				memcpy(p_info, p_setting, sizeof(HD_FILEOUT_CONFIG));
			}
		}
		break;

	default:
		break;
	}
}

HD_RESULT _hd_ileout_fill_all_default_params(UINT32 self_id, UINT32 out_id)
{
	UINT32 dev  = self_id - DEV_BASE;
	UINT32 port = out_id- OUT_BASE;
	FILEOUT_INFO *p_fileout_info = &fileout_info[dev];

	//Config: init values
	_hd_fileout_fill_param(HD_FILEOUT_PARAM_CONFIG, &p_fileout_info->port[port].fout_config, NULL);

	return HD_OK;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_fileout_init(VOID)
{
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	//notice: max path num 16 / max active port num 6
	_hd_fileout_cfg_max(HD_MAX_PATH_NUM);

	if (fileout_info[0].port != NULL) {
		DBG_ERR("already init or not uninit\n");
		rv = HD_ERR_INIT; ///< module is already initialized
		goto l_hd_fileout_init_end;
	}

	if (_max_path_count == 0) {
		DBG_ERR("check _max_path_count =0?\r\n");
		rv = HD_ERR_NO_CONFIG; ///< module device config is not set
		goto l_hd_fileout_init_end;
	}

	HD_FILEOUT_FUNC("malloc\r\n");
	{
		UINT32 i;
		fileout_info[0].port = (FILEOUT_INFO_PORT*)malloc(sizeof(FILEOUT_INFO_PORT)*_max_path_count);
		if (fileout_info[0].port == NULL) {
			DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
			rv = HD_ERR_HEAP; ///< heap full
			goto l_hd_fileout_init_end;
		}
		for(i = 0; i < _max_path_count; i++) {
			memset(&(fileout_info[0].port[i]), 0, sizeof(FILEOUT_INFO_PORT));
		}
	}
	if (DEV_COUNT == 2)
	{
		UINT32 i;
		fileout_info[1].port = (FILEOUT_INFO_PORT*)malloc(sizeof(FILEOUT_INFO_PORT)*_max_path_count);
		if (fileout_info[1].port == NULL) {
			DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
			rv = HD_ERR_HEAP; ///< heap full
			goto l_hd_fileout_init_end;
		}
		for(i = 0; i < _max_path_count; i++) {
			memset(&(fileout_info[1].port[i]), 0, sizeof(FILEOUT_INFO_PORT));
		}
	}

	HD_FILEOUT_FUNC("action\r\n");
	{
		fileout_install_id();
		fileout_cb_init();    // init cb
		fileout_port_init();  // init port
		fileout_util_init();  // init util
		fileout_tsk_init();   // init tsk
		rv = HD_OK;
	}

	// set status: [UNINT] -> [INIT]
	{
		UINT32 dev, port;
		for (dev = 0; dev < DEV_COUNT; dev++) {
			for (port = 0; port < _max_path_count; port++) {
				// init default params
				_hd_ileout_fill_all_default_params(dev + DEV_BASE, port + OUT_BASE);
				// set status
				fileout_info[dev].port[port].priv.status = HD_FILEOUT_STAT_INIT;
			}
		}
	}

l_hd_fileout_init_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	// check input
	//_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) { return rv;}
	//_HD_CONVERT_SELF_ID(in_dev_id, rv); 	if(rv != HD_OK) { return rv;}
	if (in_dev_id != self_id) {
		DBG_ERR("invalid device id\r\n");
		rv = HD_ERR_DEV; ///< invalid device id
		goto l_hd_fileout_open_end;
	}

	// set path id
	if(!p_path_id) {
		DBG_ERR("p_path_id is null\r\n");
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_fileout_open_end;
	}
	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		p_path_id[0] = _out_id;
		//do nothing
		return HD_OK;
	} else {
		p_path_id[0] = HD_FILEOUT_PATH(self_id, in_id, out_id);
	}

	// check status: [INIT]
	if (0 == _hd_fileout_check_status(p_path_id[0], HD_FILEOUT_STAT_UNINIT)) {
		DBG_ERR("open could not be called, please call hd_fileout_init() first\r\n");
		rv = HD_ERR_UNINIT; ///< module is not initialised yet
		goto l_hd_fileout_open_end;
	}
	if (0 == _hd_fileout_check_status(p_path_id[0], HD_FILEOUT_STAT_IDLE)) {
		DBG_ERR("open could not be called, fileout already opened\r\n");
		rv = HD_ERR_ALREADY_OPEN; ///< path is already open
		goto l_hd_fileout_open_end;
	}
	if (0 == _hd_fileout_check_status(p_path_id[0], HD_FILEOUT_STAT_RUN)) {
		DBG_ERR("open could not be called, fileout already start\r\n");
		rv = HD_ERR_ALREADY_START; ///< path is already start
		goto l_hd_fileout_open_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
			(int)p_path_id[0], (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		fileout_tsk_start('A');
		fileout_tsk_start('B');

		r = fileout_port_is_queue_empty();
		if (r != 0) {
			DBG_ERR("open fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [INIT] -> [IDLE]
	if (rv == HD_OK) {
		// set default params to unit
		_hd_fileout_set_all_default_params_to_unit(self_id, out_id);
		if (0 != _hd_fileout_set_status(p_path_id[0], HD_FILEOUT_STAT_IDLE)) {
			DBG_ERR("fileout set status IDLE fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_fileout_open_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (0 != _hd_fileout_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_fileout_start_end;
	}

	// check status: [IDLE]
	if (0 == _hd_fileout_check_status(path_id, HD_FILEOUT_STAT_UNINIT)) {
		DBG_ERR("could not be called, please call hd_fileout_init() first\r\n");
		rv = HD_ERR_UNINIT; ///< module is not initialised yet
		goto l_hd_fileout_start_end;
	}
	if (0 == _hd_fileout_check_status(path_id, HD_FILEOUT_STAT_INIT)) {
		DBG_ERR("could not be called, please call hd_fileout_open() first\r\n");
		rv = HD_ERR_NOT_OPEN; ///< path is not open yet
		goto l_hd_fileout_start_end;
	}
	if (0 == _hd_fileout_check_status(path_id, HD_FILEOUT_STAT_RUN)) {
		DBG_ERR("could not be called, path is already start\r\n");
		rv = HD_ERR_ALREADY_START; ///< path is already start
		goto l_hd_fileout_start_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		INT32 port_idx = (INT32)out_id;

		// Check whether the queue is empty every time it starts
		#if 1
		fileout_port_is_queue_empty();
		#endif

		r = fileout_port_start(port_idx);
		if (r != 0) {
			DBG_ERR("start fail (%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [IDLE] -> [RUN]
	if (rv == HD_OK) {
		if (0 != _hd_fileout_set_status(path_id, HD_FILEOUT_STAT_RUN)) {
			DBG_ERR("fileout set status RUN fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_fileout_start_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (0 != _hd_fileout_check_path_id(path_id)) {
		DBG_ERR("path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_fileout_stop_end;
	}

	// check status: [RUN]
	if (0 != _hd_fileout_check_status(path_id, HD_FILEOUT_STAT_RUN)) {
		DBG_ERR("stop could not be called, fileout is not start\r\n");
		rv = HD_ERR_NOT_START; ///< path is not start yet
		goto l_hd_fileout_stop_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	{
		int r;
		INT32 port_idx = (INT32) out_id;
		fileout_util_set_slowcard(0, 0);
		r = fileout_port_close(port_idx);
		if (r != 0) {
			DBG_ERR("stop fail(%d)\r\n", r);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

	// set status: [RUN] -> [IDLE]
	if (rv == HD_OK) {
		if (0 != _hd_fileout_set_status(path_id, HD_FILEOUT_STAT_IDLE)) {
			DBG_ERR("fileout set status IDLE fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_fileout_stop_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (0 != _hd_fileout_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_fileout_close_end;
	}

	if (ctrl_id == HD_CTRL) {
		//do nothing
		return HD_OK;
	}

	// check status: [IDLE]
	if (0 != _hd_fileout_check_status(path_id, HD_FILEOUT_STAT_IDLE)) {
		DBG_ERR("close could not be called, fileout is not idle\r\n");
		rv = HD_ERR_NOT_OPEN; ///< path is not open yet
		goto l_hd_fileout_close_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	{
		#if 0
		INT32 port_idx = (INT32) out_id; //(INT32)in_id - ISF_IN_BASE;
		rv = fileout_port_close(port_idx);
		if (HD_OK == rv) {
			DBG_DUMP(">>> [FILEOUT] Close-(%d)\r\n", port_idx);
		}
		#endif
		rv = HD_OK;
	}

	// set status: [IDLE] -> [INIT]
	if (rv == HD_OK) {
		if (0 != _hd_fileout_set_status(path_id, HD_FILEOUT_STAT_INIT)) {
			DBG_ERR("fileout set status IDLE fail\r\n");
			rv = HD_ERR_NG; ///< general failure
		}
	}

l_hd_fileout_close_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_get(HD_PATH_ID path_id, HD_FILEOUT_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (0 != _hd_fileout_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_fileout_get_end;
	}

	if (NULL == p_param) {
		DBG_ERR("check p_param is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_fileout_get_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	if (ctrl_id == HD_CTRL) {
		switch(id) {
		// Config
		case HD_FILEOUT_PARAM_CONFIG:
			{
				UINT32 i = 0;
				HD_FILEOUT_CONFIG *p_user = (HD_FILEOUT_CONFIG *)p_param;
				HD_FILEOUT_CONFIG *p_info = &fileout_info[self_id-DEV_BASE].port[i-OUT_BASE].fout_config;
				_hd_fileout_get_param(id, i, p_info);
				memcpy(p_user, p_info, sizeof(HD_FILEOUT_CONFIG));
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	} else {
		switch (id) {
		// Config
		case HD_FILEOUT_PARAM_CONFIG:
			{
				HD_FILEOUT_CONFIG *p_user = (HD_FILEOUT_CONFIG *)p_param;
				HD_FILEOUT_CONFIG *p_info = &fileout_info[self_id-DEV_BASE].port[out_id-OUT_BASE].fout_config;
				_hd_fileout_get_param(id, out_id, p_info);
				memcpy(p_user, p_info, sizeof(HD_FILEOUT_CONFIG));
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	}

l_hd_fileout_get_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_set(HD_PATH_ID path_id, HD_FILEOUT_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (0 != _hd_fileout_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_fileout_set_end;
	}

	if (NULL == p_param) {
		DBG_ERR("check p_param is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_fileout_set_end;
	}

	if (0 != _hd_fileout_check_params_range(id, p_param, NULL, 0)) {
		DBG_ERR("check p_param value range error, path_id %d\n", (int)path_id);
		rv = HD_ERR_INV; ///< invalid argument passed
		goto l_hd_fileout_set_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	if (ctrl_id == HD_CTRL) {
		switch(id) {
		// Config
		case HD_FILEOUT_PARAM_CONFIG:
			{
				UINT32 i = 0;
				HD_FILEOUT_CONFIG *p_setting = (HD_FILEOUT_CONFIG *)p_param;
				HD_FILEOUT_CONFIG *p_info = &fileout_info[self_id-DEV_BASE].port[i-OUT_BASE].fout_config;
				memcpy(p_info, p_setting, sizeof(HD_FILEOUT_CONFIG));
				_hd_fileout_set_param(id, i, p_info);
				rv = HD_OK;
			}
			break;
		// Callback
		case HD_FILEOUT_PARAM_REG_CALLBACK:
			{
				HD_FILEOUT_REG_CALLBACK *p_cbinfo = &fileout_info[self_id-DEV_BASE].port[out_id-OUT_BASE].fout_callback;
				p_cbinfo->callbackfunc = (HD_FILEOUT_CALLBACK)p_param;
				p_cbinfo->version = 0x20200615;
				HD_FILEOUT_FUNC("callback 0x%X\r\n", (unsigned int)p_param);
				fileout_cb_register((HD_FILEOUT_CALLBACK)p_param);
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	} else {
		switch(id) {
		// Config
		case HD_FILEOUT_PARAM_CONFIG:
			{
				HD_FILEOUT_CONFIG *p_setting = (HD_FILEOUT_CONFIG *)p_param;
				HD_FILEOUT_CONFIG *p_info = &fileout_info[self_id-DEV_BASE].port[out_id-OUT_BASE].fout_config;
				memcpy(p_info, p_setting, sizeof(HD_FILEOUT_CONFIG));
				_hd_fileout_set_param(id, out_id, p_info);
				rv = HD_OK;
			}
			break;
		// Callback
		case HD_FILEOUT_PARAM_REG_CALLBACK:
			{
				HD_FILEOUT_FUNC("callback 0x%X\r\n", (unsigned int)p_param);
				fileout_cb_register((HD_FILEOUT_CALLBACK)p_param);
				rv = HD_OK;
			}
			break;
		// Unsupported
		default:
			DBG_DUMP("PARAM UNSUPPORTED\r\n");
			rv = HD_ERR_NOT_SUPPORT; ///< not support
			break;
		}
	}

l_hd_fileout_set_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_push_in_buf(HD_PATH_ID path_id, HD_FILEOUT_BUF* p_user_in_buf, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (0 != _hd_fileout_check_path_id(path_id)) {
		DBG_ERR("check path_id error, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_PATH; ///< invalid path id
		goto l_hd_fileout_push_in_buf_end;
	}

	if (NULL == p_user_in_buf) {
		DBG_ERR("check p_user_in_video_bs is NULL, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_NULL_PTR; ///< null pointer
		goto l_hd_fileout_push_in_buf_end;
	}

	#if 0
	// check status: [RUN]
	if (0 != _hd_fileout_check_status(path_id, HD_FILEOUT_STAT_RUN)) {
		DBG_ERR("push_in_buf could only be called after START, please call hd_fileout_start() first\r\n");
		return HD_ERR_NOT_START;
	}
	#endif

	if (MAKEFOURCC('F', 'O', 'U', 'T') != p_user_in_buf->sign) {
		DBG_ERR("invalid sign in data, path_id %d\r\n", (int)path_id);
		rv = HD_ERR_SIGN; ///< invalid sign in data
		goto l_hd_fileout_push_in_buf_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	HD_FILEOUT_DUMP("path_id %d, self_id %d, in_id %d out_id %d\r\n",
		(int)path_id, (int)self_id, (int)in_id, (int)out_id);
	HD_FILEOUT_DUMP("p_user_in_buf 0x%X\r\n", (unsigned int)p_user_in_buf);
	{
		HD_FILEOUT_BUF *p_buf = p_user_in_buf;
		INT32 port_idx = (INT32) out_id; //(INT32)in_id - ISF_IN_BASE;
		INT32 ret_qidx = 0;
		int r;

		//get qidx
		ret_qidx = fileout_port_get_queue(port_idx);
		if (ret_qidx < 0) {
			DBG_ERR("get_queue failed, path_id %d\r\n", (int)path_id);
			rv = HD_ERR_NOT_AVAIL; ///< resources not available
			goto l_hd_fileout_push_in_buf_end;
		} else {
			rv = HD_OK;
		}

		//push to queue
		r = fileout_queue_push(p_buf, ret_qidx);
		if (r != 0) {
			DBG_ERR("queue_push failed, path_id %d\r\n", (int)path_id);
			rv = HD_ERR_FAIL; ///< already executed and failed
		} else {
			rv = HD_OK;
		}
	}

l_hd_fileout_push_in_buf_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

HD_RESULT hd_fileout_uninit(VOID)
{
	HD_RESULT rv;

	HD_FILEOUT_FUNC("begin\r\n");

	if (fileout_info[0].port == NULL) {
		DBG_ERR("already uninit or not init\n");
		rv = HD_ERR_UNINIT; ///< module is not initialized yet
		goto l_hd_fileout_uninit_end;
	}

	HD_FILEOUT_FUNC("action\r\n");
	{
		//fileout_tsk_destroy('A');
		//fileout_tsk_destroy('B');
		fileout_tsk_destroy_multi();
		fileout_uninstall_id();
		rv = HD_OK;
	}

	// set status: [INIT] -> [UNINIT]
	{
		UINT32 dev, port;
		for (dev = 0; dev < DEV_COUNT; dev++) {
			for (port = 0; port < _max_path_count; port++) {
				fileout_info[dev].port[port].priv.status = HD_FILEOUT_STAT_UNINIT;
			}
		}
	}

	{
		free(fileout_info[0].port);
		fileout_info[0].port = NULL;
	}
	if (DEV_COUNT == 2)
	{
		free(fileout_info[1].port);
		fileout_info[1].port = NULL;
	}

l_hd_fileout_uninit_end:
	HD_FILEOUT_FUNC("end (result %d)\r\n", (int)rv);
	return rv;
}

