/**
    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.

    @file       PrjCfg_Default.h
    @ingroup

    @brief

    @note       This config genernate default config where isn't set in PrjCfg_MODEL.h
*/

#ifndef _PRJCFG_DEFAULT_H_
#define _PRJCFG_DEFAULT_H_

#ifndef MOVIE_DIRECT_FUNC
#define MOVIE_DIRECT_FUNC               DISABLE
#endif

#ifndef MOVIE_YUV_COMPRESS
#define MOVIE_YUV_COMPRESS              DISABLE
#endif

#ifndef MOVIE_EIS
#define MOVIE_EIS                       DISABLE
#endif

#ifndef MOVIE_EIS_GYRO_LOG
#define MOVIE_EIS_GYRO_LOG              DISABLE
#endif

#ifndef MOVIE_DEWRAP_BY_IMAGEAPP
#define MOVIE_DEWRAP_BY_IMAGEAPP        DISABLE
#endif

#ifndef MOVIE_DEWRAP_BY_PROJECT
#define MOVIE_DEWRAP_BY_PROJECT         DISABLE
#endif

#ifndef PHOTO_DIRECT_FUNC
#define PHOTO_DIRECT_FUNC				DISABLE
#endif

#ifndef PHOTO_PREVIEW_SLICE_ENC_FUNC
#define PHOTO_PREVIEW_SLICE_ENC_FUNC	DISABLE
#endif

#ifndef PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC
#define PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC	DISABLE
#endif

/* PHOTO_DIRECT_FUNC is enabled only if PHOTO_PREVIEW_SLICE_ENC_FUNC is enabled */
#if (PHOTO_DIRECT_FUNC == ENABLE)
#if (PHOTO_PREVIEW_SLICE_ENC_FUNC == DISABLE)
#undef PHOTO_PREVIEW_SLICE_ENC_FUNC
#define PHOTO_PREVIEW_SLICE_ENC_FUNC	  ENABLE
#endif
#endif

#if PHOTO_PREVIEW_SLICE_ENC_VER2_FUNC == ENABLE && PHOTO_PREVIEW_SLICE_ENC_FUNC == DISABLE
#undef PHOTO_PREVIEW_SLICE_ENC_FUNC
#define PHOTO_PREVIEW_SLICE_ENC_FUNC	ENABLE
#endif

#ifndef PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP
#define PHOTO_PREVIEW_SLICE_ENC_SRC_STAMP ENABLE
#endif

#ifndef ETH_REARCAM_CAPS_COUNT
#define ETH_REARCAM_CAPS_COUNT              0
#endif

#ifndef ETH_REARCAM_CLONE_FOR_DISPLAY
#define ETH_REARCAM_CLONE_FOR_DISPLAY 	DISABLE
#endif

#ifndef ETHCAM_FILE_FORMAT
#define ETHCAM_FILE_FORMAT                 _CFG_FILE_FORMAT_MP4// ts / MP4 /MOV, ONLY for Rx
#endif

#ifndef ETHCAM_AUDIO_FORMAT
#define ETHCAM_AUDIO_FORMAT                 _CFG_AUD_CODEC_AAC// AAC / PCM, ONLY for Rx
#endif

#ifndef ETHCAM_CHECK_PORTREADY
#define ETHCAM_CHECK_PORTREADY            DISABLE
#endif

#ifndef ETH_REARCAM_RX_SENSOR_CLOSE
#define ETH_REARCAM_RX_SENSOR_CLOSE		       DISABLE
#endif

#ifndef ETHCAM_DECODE_ERR_DBG
#define ETHCAM_DECODE_ERR_DBG 	DISABLE
#endif

#ifndef ETHCAM_EIS
#define ETHCAM_EIS                      DISABLE
#endif

#ifndef WATERLOGO_FUNCTION
#define WATERLOGO_FUNCTION              DISABLE
#endif

#ifndef MOVIE_MULTISTAMP_FUNC
#define MOVIE_MULTISTAMP_FUNC 	DISABLE
#endif

#ifndef MOVIE_MULTIWATERLOGO_FUNC
#define MOVIE_MULTIWATERLOGO_FUNC 	DISABLE
#endif

#if (SENSOR_CAPS_COUNT == 1)
#if (!defined(_SENSOR1_sen_off_))
#define SENSOR_ON_MASK                (SENSOR_1)
#elif (!defined(_SENSOR2_sen_off_))
#define SENSOR_ON_MASK                (SENSOR_2)
#else
#define SENSOR_ON_MASK                (0)
#endif
#elif (SENSOR_CAPS_COUNT == 2)
#if (!defined(_SENSOR1_sen_off_) && !defined(_SENSOR2_sen_off_))
#define SENSOR_ON_MASK                (SENSOR_1|SENSOR_2)
#else
#define SENSOR_ON_MASK                (0)
#endif
#elif (SENSOR_CAPS_COUNT == 3)
#if (!defined(_SENSOR1_sen_off_) && !defined(_SENSOR2_sen_off_) && !defined(_SENSOR3_sen_off_))
#define SENSOR_ON_MASK                (SENSOR_1|SENSOR_2|SENSOR_3)
#else
#define SENSOR_ON_MASK                (0)
#endif
#elif (SENSOR_CAPS_COUNT == 4)
#if (!defined(_SENSOR1_sen_off_) && !defined(_SENSOR2_sen_off_) && !defined(_SENSOR3_sen_off_)&& !defined(_SENSOR4_sen_off_))
#define SENSOR_ON_MASK                (SENSOR_1|SENSOR_2|SENSOR_3|SENSOR_4)
#else
#define SENSOR_ON_MASK                (0)
#endif
#else
#define SENSOR_ON_MASK                (0)
#endif

#ifndef SENSOR_SIEPATGEN
#define SENSOR_SIEPATGEN 	            DISABLE
#endif

#if (ETH_REARCAM_RX_SENSOR_CLOSE==ENABLE && defined(_NVT_ETHREARCAM_RX_))
#undef SENSOR_ON_MASK
#define SENSOR_ON_MASK                (0)
#endif


#ifndef LOGFILE_FUNC
#define LOGFILE_FUNC					DISABLE
#endif

#ifndef USER_PACK_UPDATE
#define USER_PACK_UPDATE				DISABLE
#endif

#ifndef WAV_PLAY_FUNC
#define WAV_PLAY_FUNC					DISABLE
#endif
#ifndef MOVIE_IME_CROP
#define MOVIE_IME_CROP					DISABLE
#endif

#ifndef PHOTO_IME_CROP
#define PHOTO_IME_CROP					DISABLE
#endif

#ifndef PLAY_FULL_DISP
#define PLAY_FULL_DISP					DISABLE
#endif
#ifndef PHOTO_MODE_CAP_YUV420_FUNC
#define PHOTO_MODE_CAP_YUV420_FUNC				ENABLE
#endif
#ifndef PLAY_THUMB_AND_MOVIE
#define PLAY_THUMB_AND_MOVIE			DISABLE
#endif

#ifndef DZOOM_FUNC
#define DZOOM_FUNC			DISABLE
#endif

#ifndef FSCK_FUNC //fsck is for disk checking
#define FSCK_FUNC DISABLE
#endif

//#NT#20220503#Philex lin -begin
// power saving marco
#ifndef USB_POWER_DOWN
#define USB_POWER_DOWN          DISABLE
#endif

#ifndef ETH_POWER_DOWN
#if (defined(_NVT_ETHREARCAM_TX_)||defined(_NVT_ETHREARCAM_RX_))
#define ETH_POWER_DOWN         DISABLE
#else
#define ETH_POWER_DOWN         ENABLE
#endif
#endif

#ifndef GDC_POWER_DOWN
#define GDC_POWER_DOWN         DISABLE
#endif
//#NT#20220503#Philex lin -end

#ifndef LIVESTREAM_FUNC
#define LIVESTREAM_FUNC         DISABLE
#endif

#ifndef _QRCODE_FUNC_
#define _QRCODE_FUNC_         	DISABLE
#endif

#ifndef DRAM_SIZE_64MB_DISP_OFF
#define DRAM_SIZE_64MB_DISP_OFF              DISABLE
#endif

#ifndef PLAY_DEWARP
#define PLAY_DEWARP         	 DISABLE
#endif

#ifndef MANUAL_PUSH_RAW_FRAME
#define MANUAL_PUSH_RAW_FRAME  DISABLE
#endif

#ifndef HUNTING_CAMERA_SYS_PARTITION_NAME
#define HUNTING_CAMERA_SYS_PARTITION_NAME	sys
#endif 

#ifndef DCF_DIR_NAME
#define DCF_DIR_NAME					"HUNTI"   /* 100HUNTI */
#endif

#ifndef DCF_FILE_NAME
#define DCF_FILE_NAME					"IMAG"    /* IMAG0001.JPG */
#endif

/**************************************************
 * HEIC default begin
 * ************************************************/

#ifndef HEIC_FUNC

#if (_PACKAGE_HEIC_)

#define HEIC_FUNC						ENABLE

#ifndef HEIC_DCF_SUPPORT
#define HEIC_DCF_SUPPORT				ENABLE
#endif

#ifndef HEIC_DCF_FILE_TYPE
#define HEIC_DCF_FILE_TYPE		DCF_FILE_TYPE_ASF
#endif

#ifndef HEIC_TMP_EXT_NAME
#define HEIC_TMP_EXT_NAME		".ASF"
#endif

#else

#define HEIC_FUNC						DISABLE

#endif
#endif

/**************************************************
 * HEIC default end
 * ************************************************/

#endif //_PRJCFG_DEFAULT_H_
