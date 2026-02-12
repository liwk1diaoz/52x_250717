#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kwrap/examsys.h>
#include <kwrap/error_no.h>
#include <kwrap/type.h>
#include <kwrap/util.h>
#include <hdal.h>
#include "hd_fileout_lib.h"

///////////////////////////////////////////////////////////////////////////////
#define FOUT_TEMP_BUF_SIZE ALIGN_CEIL_64(0x150000)

static HD_IN_ID   fileout_in_id[4] = {0, 1, 2, 3};
static HD_OUT_ID  fileout_out_id[4] = {0, 1, 2, 3};
static HD_PATH_ID fileout_path_id[4] = {0};

static UINT32     temp_pa;
static UINT32     temp_va;

///////////////////////////////////////////////////////////////////////////////

static UINT32 SxCmd_GetTempMem(UINT32 size)
{
	CHAR                 name[]= "FoutTmp";
	void                 *va;
	HD_RESULT            ret;
    HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

    if(temp_pa) {
    	printf("need release buf\r\n");
        return 0;
    }
	ret = hd_common_mem_alloc(name, &temp_pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id));
		return 0;
	}

	//printf("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(va));

	temp_va = (UINT32)va;

	return (UINT32)va;
}

static UINT32 SxCmd_RelTempMem(UINT32 addr)
{
	HD_RESULT            ret;

	//printf("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(addr));

    if(!temp_pa||!addr){
        printf("not alloc\r\n");
        return HD_ERR_UNINIT;
    }
	ret = hd_common_mem_free(temp_pa, (void *)addr);
	if (ret != HD_OK) {
		printf("err:free temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(addr), (unsigned int)(addr));
		return HD_ERR_NG;
	}else {
        temp_pa =0;
	}
    return HD_OK;
}

int nvt_fileout_mem_init(void)
{
	int buf;

	buf = (int)SxCmd_GetTempMem(FOUT_TEMP_BUF_SIZE);
	if (0 == buf) {
		printf("SxCmd_GetTempMem failed.\n");
		return -1;
	}

	return 0;
}

int nvt_fileout_mem_uninit(void)
{
	int buf;

	buf = (int)temp_va;
	if (0 == buf) {
		printf("SxCmd_GetTempMem failed.\n");
		return -1;
	}

	SxCmd_RelTempMem(buf);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int nvt_fileout_hdal_init(void)
{
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	ret = hd_common_init(0);
	if(ret != HD_OK) {
		printf("common init fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_common_mem_init(&mem_cfg);
	if(ret != HD_OK) {
		printf("mem init fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_gfx_init();
	if(ret != HD_OK) {
		printf("gfx init fail=%d\n", (int)ret);
		return -1;
	}
	return 0;
}

int nvt_fileout_hdal_uninit(void)
{
	HD_RESULT ret;

	ret = hd_gfx_uninit();
	if(ret != HD_OK) {
		printf("gfx uninit fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_common_mem_uninit();
	if(ret != HD_OK) {
		printf("mem uninit fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common uninit fail=%d\n", (int)ret);
		return -1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int nvt_fileout_cb(CHAR *p_name, HD_FILEOUT_CBINFO *cbinfo, UINT32 *param)
{
	HD_FILEOUT_CB_EVENT event = cbinfo->cb_event;
	const char sample_name[] = "A:\\MOVIE\\20000101";
	const char sample_ext[] = "MP4";
	static UINT32 file_sn = 0;

	switch (event) {
	case HD_FILEOUT_CB_EVENT_NAMING: {
			// Print Info
			printf("[NAMING] type: %d, event: %d, port: %d\r\n",
				(int)cbinfo->type, (int)cbinfo->event, (int)cbinfo->iport);
			// Get Serial Number
			file_sn++;
			if (file_sn > 999)
				file_sn = 1;
			// Return File Naming
			cbinfo->fpath_size = 128;
			snprintf(cbinfo->p_fpath, cbinfo->fpath_size, "%s_%03d.%s",
				sample_name, (int)file_sn, sample_ext);
			// Print Return
			printf("[NAMING] path: %s\r\n", cbinfo->p_fpath);
			break;
		}

	case HD_FILEOUT_CB_EVENT_OPENED: {
			// Print Info
			printf("[OPENED] port: %d\r\n", (int)cbinfo->iport);
			// Return allocate size
			cbinfo->alloc_size = (100*1024*1024);
			// Print Return
			printf("[OPENED] alloc_size: %d\r\n", (int)cbinfo->alloc_size);
			break;
		}

	case HD_FILEOUT_CB_EVENT_CLOSED: {
			// Print Info
			printf("[CLOSED] path: %s, port: %d\r\n",
				cbinfo->p_fpath, (int)cbinfo->iport);
			break;
		}

	case HD_FILEOUT_CB_EVENT_DELETE: {
			break;
		}

	case HD_FILEOUT_CB_EVENT_FS_ERR: {
			// Print Info
			printf("[DELETE] drive: %s, port: %d\r\n",
				cbinfo->drive, (int)cbinfo->iport);
			break;
		}

	default: {
			printf("Invalid event %d\r\n", (int)event);
			break;
		}
	}


	return 0;
}

INT32 nvt_fileout_buf_cb(void *p_data)
{
	return 0;
}

int nvt_fileout_module_init(void)
{
	HD_RESULT ret = HD_ERR_NG;

	if ((ret = hd_fileout_init()) != HD_OK) {
		printf("hd_fileout_init fail(%d)\r\n", (int)ret);
		return -1;
	}

	return 0;
}

int nvt_fileout_module_open(void)
{
	HD_RESULT ret = HD_ERR_NG;
	int i;

	for (i = 0; i < 4; i++) {
		if ((ret = hd_fileout_open(fileout_in_id[i], fileout_out_id[i], &fileout_path_id[i])) != HD_OK) {
			printf("hd_fileout_open fail(%d)\r\n", (int)ret);
			return -1;
		}
	}

	for (i = 0; i < 4; i++) {
		if ((ret = hd_fileout_set(fileout_path_id[i], HD_FILEOUT_PARAM_REG_CALLBACK, (void*)nvt_fileout_cb)) != HD_OK) {
			printf("fileout_set_param cb fail(%d)\r\n", (int)ret);
			return -1;
		}
	}

	return 0;
}

int nvt_fileout_module_start(void)
{
	HD_RESULT ret = HD_ERR_NG;
	int i;

	for (i = 0; i < 4; i++) {
		if ((ret = hd_fileout_start(fileout_path_id[i])) != HD_OK) {
			printf("hd_fileout_start fail(%d)\r\n", (int)ret);
			return -1;
		}
	}

	return 0;
}

int nvt_fileout_module_stop(void)
{
	HD_RESULT ret = HD_ERR_NG;
	int i;

	for (i = 0; i < 4; i++) {
		if ((ret = hd_fileout_stop(fileout_path_id[i])) != HD_OK) {
			printf("hd_fileout_stop fail(%d)\r\n", (int)ret);
			return -1;
		}
	}

	return 0;
}

int nvt_fileout_module_close(void)
{
	HD_RESULT ret = HD_ERR_NG;
	int i;

	for (i = 0; i < 4; i++) {
		if ((ret = hd_fileout_close(fileout_path_id[i])) != HD_OK) {
			printf("hd_fileout_close fail(%d)\r\n", (int)ret);
			return -1;
		}
	}

	return 0;
}

int nvt_fileout_module_uninit(void)
{
	HD_RESULT ret = HD_ERR_NG;

	if ((ret = hd_fileout_uninit()) != HD_OK) {
		printf("hd_fileout_init fail(%d)\r\n", (int)ret);
		return -1;
	}

	return 0;
}

int nvt_fileout_module_push_in(HD_FILEOUT_BUF *buf)
{
	HD_RESULT ret = HD_ERR_NG;
	int id = buf->pathID;

	if ((ret = hd_fileout_push_in_buf(id, buf, -1)) != HD_OK) {
		printf("hd_fileout_push_in_buf fail(%d)\r\n", (int)ret);
		return -1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int test_emu_main(void)
{
	HD_FILEOUT_BUF tmp_buf = {0};
	INT32 num;

	tmp_buf.sign      = MAKEFOURCC('F', 'O', 'U', 'T');
	tmp_buf.event     = 0;   //BSMUXER_FEVENT_NORMAL
	tmp_buf.addr      = (UINT32) temp_pa;
	tmp_buf.size      = FOUT_TEMP_BUF_SIZE;
	tmp_buf.pos       = 0;
	tmp_buf.fp_pushed = nvt_fileout_buf_cb;

	//CREATE&WRITE
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CREATE | HD_FILEOUT_FOP_CONT_WRITE;
		tmp_buf.type      = 0x4; //MEDIA_FILEFORMAT_MP4
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//WRITE&FLUSH
	for (num = 0; num < 2; num ++)
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_FLUSH;
		tmp_buf.type      = 0x4; //MEDIA_FILEFORMAT_MP4
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	// SNAPSHOT
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_SNAPSHOT;
		tmp_buf.type      = 0x10; //MEDIA_FILEFORMAT_JPG
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//WRITE&FLUSH
	for (num = 0; num < 2; num ++)
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_FLUSH;
		tmp_buf.type      = 0x4; //MEDIA_FILEFORMAT_MP4
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//CLOSE
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_SEEK_WRITE | HD_FILEOUT_FOP_CLOSE;
		tmp_buf.type      = 0x4; //MEDIA_FILEFORMAT_MP4
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	return 0;
}

int test_emu_dual(void)
{
	HD_FILEOUT_BUF tmp_buf = {0};
	INT32 num;

	tmp_buf.sign      = MAKEFOURCC('F', 'O', 'U', 'T');
	tmp_buf.addr      = (UINT32) temp_pa;
	tmp_buf.size      = FOUT_TEMP_BUF_SIZE;
	tmp_buf.pos       = 0;
	tmp_buf.type      = 0x4; //MEDIA_FILEFORMAT_MP4
	tmp_buf.fp_pushed = nvt_fileout_buf_cb;

	//CREATE&WRITE
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CREATE | HD_FILEOUT_FOP_CONT_WRITE;
		tmp_buf.event     = 0;   //BSMUXER_FEVENT_NORMAL
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//WRITE&FLUSH
	for (num = 0; num < 2; num ++)
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_FLUSH;
		tmp_buf.event     = 0;   //BSMUXER_FEVENT_NORMAL
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//CREATE&WRITE
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CREATE | HD_FILEOUT_FOP_CONT_WRITE;
		tmp_buf.event     = 1;   //BSMUXER_FEVENT_EMR
		tmp_buf.pathID    = fileout_path_id[1];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//WRITE&FLUSH
	for (num = 0; num < 2; num ++)
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_FLUSH;
		tmp_buf.event     = 0;   //BSMUXER_FEVENT_NORMAL
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);

		tmp_buf.fileop    = HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_FLUSH;
		tmp_buf.event     = 1;   //BSMUXER_FEVENT_EMR
		tmp_buf.pathID    = fileout_path_id[1];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//CLOSE
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_SEEK_WRITE | HD_FILEOUT_FOP_CLOSE;
		tmp_buf.event     = 1;   //BSMUXER_FEVENT_EMR
		tmp_buf.pathID    = fileout_path_id[1];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//WRITE&FLUSH
	for (num = 0; num < 2; num ++)
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_CONT_WRITE | HD_FILEOUT_FOP_FLUSH;
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	//CLOSE
	{
		tmp_buf.fileop    = HD_FILEOUT_FOP_SEEK_WRITE | HD_FILEOUT_FOP_CLOSE;
		tmp_buf.event     = 0;   //BSMUXER_FEVENT_NORMAL
		tmp_buf.pathID    = fileout_path_id[0];
		nvt_fileout_module_push_in(&tmp_buf);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

EXAMFUNC_ENTRY(test_fileout, argc, argv)
{
	nvt_fileout_hdal_init();

	nvt_fileout_mem_init();

	nvt_fileout_module_init();

	nvt_fileout_module_open();

	nvt_fileout_module_start();

	// wait open/start end
	vos_util_delay_ms(4000);

	//test 0
	test_emu_main();

	//test 1
	test_emu_dual();

	nvt_fileout_module_stop();

	nvt_fileout_module_close();

	// wait stop/close end
	vos_util_delay_ms(4000);

	nvt_fileout_module_uninit();

	nvt_fileout_mem_uninit();

	nvt_fileout_hdal_uninit();

 	return 0;
}

