#include <stdio.h>
#include "PrjInc.h"
#include "ImageApp/ImageApp_MovieMulti.h"
#include "avfile/movieinterface_def.h"
#include "hdal.h"

#define __MODULE__          UIMovieRecSetting
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

//#define ENABLE      1
//#define DISABLE     0

MOVIE_RECODE_INFO gMovie_Rec_Info[SENSOR_MAX_NUM] = {
	{
		_CFG_REC_ID_1,			//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{1920, 1080},			//MOVIE_CFG_IMG_SIZE
		30,						//MOVIE_CFG_FRAME_RATE
		2 * 1024 * 1024,		//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC,		//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		#if (defined(_NVT_ETHREARCAM_TX_))
		_CFG_FILE_FORMAT_TS,	//MOVIE_CFG_FILE_FORMAT
		#else
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		#endif
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{1920, 1080},			//MOVIE_CFG_RAWENC_SIZE
		(HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB|HD_VIDEOCAP_FUNC_SHDR),	//MOVIE_CFG_VCAP_FUNC
		ENABLE,					//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
        (HD_VIDEOPROC_FUNC_SHDR   | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_SHARP | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR),	//MOVIE_CFG_VPROC_FUNC
		{16, 9},				//video display aspect ratio
	},

	{
		_CFG_REC_ID_2,			//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{1920, 1080},			//MOVIE_CFG_IMG_SIZE
		30, 					//MOVIE_CFG_FRAME_RATE
		2 * 1024 * 1024,		//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC, 	//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		#if (defined(_NVT_ETHREARCAM_TX_))
		_CFG_FILE_FORMAT_TS,	//MOVIE_CFG_FILE_FORMAT
		#else
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		#endif
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{1920, 1080},			//MOVIE_CFG_RAWENC_SIZE
		(HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB),	//MOVIE_CFG_VCAP_FUNC
		ENABLE, 				//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		(HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR),		//MOVIE_CFG_VPROC_FUNC
		{16, 9},				//video display aspect ratio
	},

	{
		_CFG_REC_ID_3,			//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{1920, 1080},			//MOVIE_CFG_IMG_SIZE
		30, 					//MOVIE_CFG_FRAME_RATE
		2 * 1024 * 1024,		//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC, 	//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{1920, 1080},			//MOVIE_CFG_RAWENC_SIZE
		(HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB),	//MOVIE_CFG_VCAP_FUNC
		ENABLE, 				//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		(HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR),     //MOVIE_CFG_VPROC_FUNC
		{16, 9},				//video display aspect ratio
	},

	{
		_CFG_REC_ID_4,			//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{1920, 1080},			//MOVIE_CFG_IMG_SIZE
		30, 					//MOVIE_CFG_FRAME_RATE
		2 * 1024 * 1024,		//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC, 	//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{1920, 1080},			//MOVIE_CFG_RAWENC_SIZE
		(HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB),	//MOVIE_CFG_VCAP_FUNC
		ENABLE, 				//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		(HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR),     //MOVIE_CFG_VPROC_FUNC
		{16, 9},				//video display aspect ratio
	},
 };

MOVIE_RECODE_INFO gMovie_Clone_Info[SENSOR_MAX_NUM] = {
	{
		_CFG_CLONE_ID_1,		//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{848, 480},				//MOVIE_CFG_IMG_SIZE
		30,						//MOVIE_CFG_FRAME_RATE
		250 * 1024,				//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC,		//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{848, 480},				//MOVIE_CFG_RAWENC_SIZE
		0,						//MOVIE_CFG_VCAP_FUNC (N/A, only refer to Rec_Info setting)
		ENABLE,					//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		0,						//MOVIE_CFG_VPROC_FUNC (N/A, only refer to Rec_Info setting)
		{16, 9},				//video display aspect ratio
	},

 	{
		_CFG_CLONE_ID_2,		//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{848, 480}, 			//MOVIE_CFG_IMG_SIZE
		30, 					//MOVIE_CFG_FRAME_RATE
		250 * 1024, 			//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC, 	//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{848, 480}, 			//MOVIE_CFG_RAWENC_SIZE
		0,						//MOVIE_CFG_VCAP_FUNC (N/A, only refer to Rec_Info setting)
		ENABLE, 				//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		0,						//MOVIE_CFG_VPROC_FUNC (N/A, only refer to Rec_Info setting)
		{16, 9},				//video display aspect ratio
	},

 	{
		_CFG_CLONE_ID_3,		//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{848, 480},				//MOVIE_CFG_IMG_SIZE
		30, 					//MOVIE_CFG_FRAME_RATE
		250 * 1024, 			//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC, 	//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{848, 480}, 			//MOVIE_CFG_RAWENC_SIZE
		0,						//MOVIE_CFG_VCAP_FUNC (N/A, only refer to Rec_Info setting)
		ENABLE, 				//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		0,						//MOVIE_CFG_VPROC_FUNC (N/A, only refer to Rec_Info setting)
		{16, 9},				//video display aspect ratio
	},

 	{
		_CFG_CLONE_ID_4,		//MOVIE_CFG_REC_ID
		_CFG_VID_IN_1,			//MOVIE_CFG_VID_IN
		{848, 480},				//MOVIE_CFG_IMG_SIZE
		30, 					//MOVIE_CFG_FRAME_RATE
		250 * 1024, 			//MOVIE_CFG_TARGET_RATE
		_CFG_CODEC_H264,		//MOVIE_CFG_CODEC
		_CFG_AUD_CODEC_AAC, 	//MOVIE_CFG_AUD_CODEC
		_CFG_REC_TYPE_AV,		//MOVIE_CFG_REC_MODE
		_CFG_FILE_FORMAT_MP4,	//MOVIE_CFG_FILE_FORMAT
		{16, 9},				//MOVIE_CFG_IMG_RATIO
		{848, 480}, 			//MOVIE_CFG_RAWENC_SIZE
		0,						//MOVIE_CFG_VCAP_FUNC (N/A, only refer to Rec_Info setting)
		ENABLE, 				//MOVIE_CFG_DISP_ENABLE
		TRUE,					//ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
		_CFG_DAR_DEFAULT,		//MOVIE_CFG_DAR
		{0},					//MOVIE_CFG_AQ_INFO
		{0},					//MOVIE_CFG_CBR_INFO
		FALSE,					//Sensor rotate setting
		0,						//MOVIE_CFG_VPROC_FUNC (N/A, only refer to Rec_Info setting)
		{16, 9},				//video display aspect ratio
	},

 };

MOVIE_STRM_INFO gMovie_Strm_Info = {
	_CFG_STRM_ID_1,				//MOVIE_CFG_REC_ID
	_CFG_VID_IN_1,				//MOVIE_CFG_VID_IN
	{848,480},					//MOVIE_CFG_IMG_SIZE
	30,							//MOVIE_CFG_FRAME_RATE
	350 * 1024,					//MOVIE_CFG_TARGET_RATE
	_CFG_CODEC_H264,			//MOVIE_CFG_CODEC
	15,							//MOVIE_CFG_GOP_NUM
	_CFG_AUD_CODEC_AAC,			//MOVIE_CFG_AUD_CODEC
	TRUE,						//MOVIE_CFG_RTSP_STRM_OUT
	{0},						//MOVIE_CFG_AQ_INFO, Defined by g_MovieRecSizeTable[].
	{0},						// MOVIE_CFG_CBR_INFO, Defined by g_MovieRecSizeTable[].
	FALSE, 						// ipl_set_enable,  FALSE: can not set sensor info, TRUE: can set sensor info
	{16,9},						//MOVIE_CFG_IMG_RATIO
	FALSE						//userproc_pull
};

MOVIE_ALG_INFO gMovie_Alg_Info[] = {
	// _CFG_REC_ID_1
	{
		// path 3
		HD_VIDEO_PXLFMT_YUV420,                 // format
		#if ((SENSOR_CAPS_COUNT& SENSOR_ON_MASK)==0)
		{0,  0},                            // image size
		#else
		{848,  480},                            // image size
		#endif
		{0, 0, 0, 0},                           // window size
		30,                                     // fps
		// path 4
		HD_VIDEO_PXLFMT_Y8,                     // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size, x/y/w/h
		0,                                      // fps
		// REC_ID
		_CFG_REC_ID_1,                          // MOVIE_CFG_REC_ID
	},
	// _CFG_REC_ID_2
	{
		// path 3
		HD_VIDEO_PXLFMT_YUV420,                 // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size (N/A)
		0,                                      // fps
		// path 4
		HD_VIDEO_PXLFMT_Y8,                     // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size, x/y/w/h
		0,                                      // fps
		// REC_ID
		_CFG_REC_ID_2,                          // MOVIE_CFG_REC_ID
	},
	// _CFG_REC_ID_3
	{
		// path 3
		HD_VIDEO_PXLFMT_YUV420,                 // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size (N/A)
		0,                                      // fps
		// path 4
		HD_VIDEO_PXLFMT_Y8,                     // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size, x/y/w/h
		0,                                      // fps
		// REC_ID
		_CFG_REC_ID_3,                          // MOVIE_CFG_REC_ID
	},
	// _CFG_REC_ID_4
	{
		// path 3
		HD_VIDEO_PXLFMT_YUV420,                 // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size (N/A)
		0,                                      // fps
		// path 4
		HD_VIDEO_PXLFMT_Y8,                     // format
		{0, 0},                                 // image size
		{0, 0, 0, 0},                           // window size, x/y/w/h
		0,                                      // fps
		// REC_ID
		_CFG_REC_ID_4,                          // MOVIE_CFG_REC_ID
	},

};

MOVIEMULTI_AUDIO_INFO   gMovie_Audio_Info = {
#if ((_BOARD_DRAM_SIZE_ == 0x04000000) || (SENSOR_CAPS_COUNT >= 2) ||  (defined(_NVT_ETHREARCAM_RX_) && (ETH_REARCAM_CAPS_COUNT >= 2)))
	16000,                     //MOVIE_CFG_AUD_RATE
	_CFG_AUDIO_CH_RIGHT,            //MOVIE_CFG_AUD_CH
	1,                         //MOVIE_CFG_AUD_CH_NUM
#else
	32000,                     //MOVIE_CFG_AUD_RATE
	_CFG_AUDIO_CH_STEREO,      //MOVIE_CFG_AUD_CH
	2,                         //MOVIE_CFG_AUD_CH_NUM
#endif
};

MOVIE_RECODE_FILE_OPTION gMovie_Rec_Option = {
	_CFG_REC_ID_1,              //MOVIE_CFG_REC_ID
	180,                        //MOVIE_CFG_FILE_SECOND seamless_sec
	FALSE,                      //MOVIE_CFG_FILE_OPTION emr_on
	3,                          //MOVIE_CFG_FILE_OPTION emr_sec (rollback sec) (main path)
	5,                          //MOVIE_CFG_FILE_SECOND keep_sec (main path)
	0,                          //MOVIE_CFG_FILE_SECOND overlap_sec (only 0 or 1 is valid)
	MOVREC_ENDTYPE_CUTOVERLAP,  //MOVIE_CFG_FILE_ENDTYPE  end_type
	10,                         //MOVIE_CFG_FILE_SECOND rollback_sec (for crash)
	10,                         //MOVIE_CFG_FILE_SECOND forward_sec (for crash)
	3,                          //MOVIE_CFG_FILE_OPTION emr_sec (rollback sec) (clone path)
	5,                          //MOVIE_CFG_FILE_SECOND keep_sec (clone path)
};
MOVIEMULTI_RECODE_FOLDER_NAMING gMovie_Folder_Naming[SENSOR_MAX_NUM] = {
	{
		_CFG_REC_ID_1,			//MOVIE_CFG_REC_ID
		"Movie",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR",					//MOVIE_CFG_FOLDER_NAME   emr
		"Movie",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},

	{
		_CFG_REC_ID_2,			//MOVIE_CFG_REC_ID
		"Movie",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR",					//MOVIE_CFG_FOLDER_NAME   emr
		"Movie",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},

	{
		_CFG_REC_ID_3,			//MOVIE_CFG_REC_ID
		"Movie",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR",					//MOVIE_CFG_FOLDER_NAME   emr
		"Movie",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},

	{
		_CFG_REC_ID_4,			//MOVIE_CFG_REC_ID
		"Movie",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR",					//MOVIE_CFG_FOLDER_NAME   emr
		"Movie",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},

 };

MOVIEMULTI_RECODE_FOLDER_NAMING gMovie_Clone_Folder_Naming[SENSOR_MAX_NUM] = {
	{
		_CFG_CLONE_ID_1,		//MOVIE_CFG_REC_ID
		"Movie_S",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR_S",				//MOVIE_CFG_FOLDER_NAME   emr
		"Photo_S",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},
 	{
		_CFG_CLONE_ID_2,		//MOVIE_CFG_REC_ID
		"Movie_S",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR_S",				//MOVIE_CFG_FOLDER_NAME   emr
		"Photo_S",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},
 	{
		_CFG_CLONE_ID_3,		//MOVIE_CFG_REC_ID
		"Movie_S",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR_S",				//MOVIE_CFG_FOLDER_NAME   emr
		"Photo_S",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},
 	{
		_CFG_CLONE_ID_4,		//MOVIE_CFG_REC_ID
		"Movie_S",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR_S",				//MOVIE_CFG_FOLDER_NAME   emr
		"Photo_S",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},

 };

#if defined(_NVT_ETHREARCAM_RX_)
MOVIEMULTI_RECODE_FOLDER_NAMING gMovie_EthCam_Folder_Naming[ETHCAM_PATH_ID_MAX] = {
	{
		_CFG_ETHCAM_ID_1,		//MOVIE_CFG_REC_ID
		"Movie_E",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR_E",				//MOVIE_CFG_FOLDER_NAME   emr
		"Movie_E",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},
	{
		_CFG_ETHCAM_ID_2,		//MOVIE_CFG_REC_ID
		"Movie_E",				//MOVIE_CFG_FOLDER_NAME   movie
		"EMR_E",				//MOVIE_CFG_FOLDER_NAME   emr
		"Movie_E",				//MOVIE_CFG_FOLDER_NAME   snapshot  modify for autotest
		TRUE,					//MOVIE_CFG_FILE_NAMING   2018/02/13
	},

};
#endif

MOVIEMULTI_CFG_DISP_INFO gMovie_Disp_Info;

static UINT32 gSensorRecMask = 0xF; // which sensor is for movie recording; bit [0:3] means sensor 1~4

UINT32 Movie_GetSensorRecMask(void)
{
	return (gSensorRecMask & System_GetEnableSensor());
}

void Movie_SetSensorRecMask(UINT32 mask)
{
	gSensorRecMask = mask & System_GetEnableSensor();
}

// get main movie record mask, bit [0:3] means main movie 1~4
UINT32 Movie_GetMovieRecMask(void)
{
	UINT32 i;
	UINT32 rec_type;
	UINT32 movie_size_idx;
	UINT32 sensor_enable; // sensor enabled
	UINT32 sensor_record; // sensor for recording
	UINT32 movie_rec_mask = 0, mask = 1;
	UINT32 setting_count = 0;

	sensor_enable = System_GetEnableSensor();
	sensor_record = Movie_GetSensorRecMask();
	movie_size_idx = UI_GetData(FL_MOVIE_SIZE);
	rec_type = MovieMapping_GetRecType(movie_size_idx);

	for (i = 0; i < SENSOR_CAPS_COUNT; i++)	{

		if (sensor_enable & mask) {
			if (rec_type == MOVIE_REC_TYPE_FRONT || rec_type == MOVIE_REC_TYPE_CLONE) { // single recording
				if (sensor_record & mask) { // active sensor, for movie recording
					movie_rec_mask = mask;
					break;
				}
			} else if (rec_type == MOVIE_REC_TYPE_DUAL) { // dual recording
				if ((sensor_record & mask) && (setting_count < 2)) { // active sensor, for movie recording
					movie_rec_mask |= mask;
					setting_count++;
#if (SENSOR_CAPS_COUNT >=2)//CID 129672
					if (setting_count == 2)
						break;
#endif
				}

			} else if (rec_type == MOVIE_REC_TYPE_TRI) { // tri recording
				if ((sensor_record & mask) && (setting_count < 3)) { // active sensor, for movie recording
					movie_rec_mask |= mask;
					setting_count++;
#if (SENSOR_CAPS_COUNT >= 3)//CID 129672
					if (setting_count == 3)
						break;
#endif
				}
			} else { // quad recording
				if ((sensor_record & mask) && (setting_count < 4)) { // active sensor, for movie recording
					movie_rec_mask |= mask;
					setting_count++;
#if (SENSOR_CAPS_COUNT >= 4)//CID 129672
					if (setting_count == 4)
						break;
#endif
				}
			}
		}

		mask <<= 1;
	}

	return movie_rec_mask;
}

// get clone movie record mask, bit [0:3] means clone movie 1~4
UINT32 Movie_GetCloneRecMask(void)
{
	UINT32 rec_type;
	UINT32 movie_size_idx;
	UINT32 clone_rec_mask = 0;

	movie_size_idx = UI_GetData(FL_MOVIE_SIZE);
	rec_type = MovieMapping_GetRecType(movie_size_idx);

	if (rec_type == MOVIE_REC_TYPE_CLONE) {
		clone_rec_mask = Movie_GetMovieRecMask();
	}

	return clone_rec_mask;
}

UINT32 Movie_GetImageRecCount(void)
{
	UINT32 i, mask, movie_rec_mask, image_rec_count;

	movie_rec_mask = Movie_GetMovieRecMask();
	mask = 1;
	image_rec_count = 0;
	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		if (movie_rec_mask & mask) {
			image_rec_count++;
		}
		mask <<= 1;
	}

	return image_rec_count;
}

void Movie_GetRecSize(UINT32 rec_id, ISIZE *rec_size)
{
	rec_size->w = gMovie_Rec_Info[rec_id].size.w;
	rec_size->h = gMovie_Rec_Info[rec_id].size.h;
}

UINT32 Movie_GetFreeSec(void)
{
	UINT32 sec;
	UINT32 i, mask, rec_id;
	UINT32 movie_rec_mask, clone_rec_mask;
	UINT64 remain_size;
	static HD_BSMUX_CALC_SEC movie_setting;

	memset(&movie_setting, 0, sizeof(HD_BSMUX_CALC_SEC));

	ImageApp_MovieMulti_GetRemainSize('A', &remain_size);
	if (remain_size == 0xFFFFFFFF) // get remaining size failed
		remain_size = 0;

	movie_rec_mask = Movie_GetMovieRecMask();
	clone_rec_mask = Movie_GetCloneRecMask();

	mask = 1;
	rec_id = 0;
	for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
		if (movie_rec_mask & mask) {
			movie_setting.info[rec_id].vidfps = gMovie_Rec_Info[i].frame_rate;
			movie_setting.info[rec_id].vidTBR = gMovie_Rec_Info[i].target_bitrate;
			movie_setting.info[rec_id].audSampleRate = gMovie_Audio_Info.aud_rate;
			movie_setting.info[rec_id].audChs = gMovie_Audio_Info.aud_ch;
			movie_setting.info[rec_id].gpson = TRUE;
			movie_setting.info[rec_id].nidxon = TRUE;
			rec_id++;
		}

		if (clone_rec_mask & mask) {
			movie_setting.info[rec_id].vidfps = gMovie_Clone_Info[i].frame_rate;
			movie_setting.info[rec_id].vidTBR = gMovie_Clone_Info[i].target_bitrate;
			movie_setting.info[rec_id].audSampleRate = gMovie_Audio_Info.aud_rate;
			movie_setting.info[rec_id].audChs = gMovie_Audio_Info.aud_ch;
			movie_setting.info[rec_id].gpson = TRUE;
			movie_setting.info[rec_id].nidxon = TRUE;
			rec_id++;
		}

		mask <<= 1;
	}
	movie_setting.givenSpace = remain_size;
	sec = ImageApp_MovieMulti_GetFreeRec(&movie_setting);

	return sec;
}

