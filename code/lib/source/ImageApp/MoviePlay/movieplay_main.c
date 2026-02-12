#include <stdio.h>
#include <kwrap/examsys.h>
#include "ImageApp/ImageApp_MoviePlay.h"
#include "ImageApp/ImageApp_Play.h"
#include "hdal.h"
#include "GxVideoFile.h"
#include "iamovieplay_tsk.h"
#include "PlaybackTsk.h"
#include "FileDB.h"

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)
 
///////////////////////////////////////////////////////////////////////////////

static UINT32         g_uiUIFlowWndPlayCurrenSpeed     = SMEDIAPLAY_SPEED_NORMAL;
static UINT32         g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
static MOVIEPLAY_FILEPLAY_INFO gMovie_Play_Info         = {0};
static FST_FILE         Movie_Play_Filehdl = NULL;

///////////////////////////////////////////////////////////////////////////////
static HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

static void _mem_pool_init(void)
{
	UINT32 i = 0;
	// config common pool (decode)
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(VDO_SIZE_W), ALIGN_CEIL_64(VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
	mem_cfg.pool_info[i].blk_cnt = 5;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool (scale & display)
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_16(DISP_SIZE_W), ALIGN_CEIL_16(DISP_SIZE_H), HD_VIDEO_PXLFMT_YUV420);  // align to 16 for rotation panel
	mem_cfg.pool_info[i].blk_cnt = 5;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool for bs pushing in
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[i].blk_size = BS_BLK_SIZE;
	mem_cfg.pool_info[i].blk_cnt = 1;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool for bs description pushing in
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[i].blk_size = BS_BLK_DESC_SIZE;
	mem_cfg.pool_info[i].blk_cnt = 1;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool for filein bs block
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[i].blk_size = 16 * 1024 * 1024;  // default at least 16MBytes for filein bitstream block.
	mem_cfg.pool_info[i].blk_cnt = 1;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
}

void movieplay_MoviePlayCB(UINT32 uiEventID, UINT32 p1, UINT32 p2, UINT32 p3)
{
}

//int main(int argc, char** argv)
//EXAMFUNC_ENTRY(movie_main, argc, argv)
int movieplay_open_main(void)
{
    char   pFilePath[FULL_FILE_PATH_LEN] = {0};
    GXVIDEO_INFO VidInfo = {0};
	UINT32 uiPBFileFmt = PBFMT_MP4;

	printf("*************************************\n");
	printf("*** imageapp_movieplay open test code ***\n");
	printf("*************************************\n");

	// Config file play info
    // Get Current index
	PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)pFilePath);
    printf("pFilePath:%s\r\n", pFilePath);

    // free PB mem before evaluate MoviePlay function.
    #if 1
    ImageApp_Play_Close();
	#endif

    // Config max mem info
	_mem_pool_init();
	ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_MEM_POOL_INFO, (UINT32)&mem_cfg);

	// Open Test Media File
	Movie_Play_Filehdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);

	if (!Movie_Play_Filehdl) {
		printf("UIFlowWndPlay_OnKeySelect: Can't open Video file!\r\n");
        return -1;
	}

	gMovie_Play_Info.fileplay_id = _CFG_FILEPLAY_ID_1;
	gMovie_Play_Info.file_handle = Movie_Play_Filehdl;
	gMovie_Play_Info.curr_speed  = g_uiUIFlowWndPlayCurrenSpeed;
	gMovie_Play_Info.curr_direct = g_uiUIFlowWndPlayCurrenDirection;
	gMovie_Play_Info.event_cb    = movieplay_MoviePlayCB;

    #if 1
	PB_GetParam(PBPRMID_CURR_FILEFMT, &uiPBFileFmt);
	PB_GetParam(PBPRMID_INFO_VDO, (UINT32*) &VidInfo);
    #else // hard code temporarily.
    SMPMaxMemInfo.FileFormat = MEDIA_FILEFORMAT_MP4;
    VidInfo.uiVidRate    = 30;
    VidInfo.AudInfo.uiSR = 32000;  // To do???
    VidInfo.uiToltalSecs = 60;
    VidInfo.uiVidType    = 0;      // To do???
    VidInfo.uiVidWidth   = 1920;
    VidInfo.uiVidHeight  = 1080;
    #endif
	ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_FILEPLAY_INFO, (UINT32) &gMovie_Play_Info);
    ImageApp_MoviePlay_Open();

	return 0;
}

int movieplay_close_main(void)
{
#if 0
    char   pFilePath[FULL_FILE_PATH_LEN] = {0};
    GXVIDEO_INFO VidInfo = {0};
    SMEDIAPLAY_MAX_MEM_INFO SMPMaxMemInfo = {0};
#endif

	printf("*************************************\n");
	printf("*** imageapp_movieplay close test code ***\n");
	printf("*************************************\n");

	ImageApp_MoviePlay_Close();

    #if 0
    // Start Play
    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_OPEN);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
    #endif
	if (gMovie_Play_Info.file_handle != NULL)
		FileSys_CloseFile(gMovie_Play_Info.file_handle);

	ImageApp_Play_Open();

	return 0;
}

