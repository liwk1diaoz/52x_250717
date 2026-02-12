/**
	@brief Source code of internal function.\n
	This file contains common internal functions.

	@file hd_int.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_type.h"
#define HD_MODULE_NAME HD_COMMON
#include "hd_int.h"
#include <string.h>
#include <sys/time.h>
#include "hd_videoprocess.h"

#define _HD_CONVERT_DEST_IN_STR(rv, dev_id, in_id, p_str, str_len) \
	do { \
		(rv) = 0; \
		if((dev_id) == 0) { \
			(rv) = snprintf(p_str, str_len, "(null)"); \
		} else if(((dev_id) >= HD_DAL_VIDEOENC_BASE) && ((dev_id) <= HD_DAL_VIDEOENC_MAX)) { \
			(rv) = _hd_videoenc_in_id_str(dev_id, in_id, p_str, str_len); \
		} else if(((dev_id) >= HD_DAL_VIDEOOUT_BASE) && ((dev_id) <= HD_DAL_VIDEOOUT_MAX)) { \
			(rv) = _hd_videoout_in_id_str(dev_id, in_id, p_str, str_len); \
		} else if(((dev_id) >= HD_DAL_VIDEOPROC_BASE) && ((dev_id) <= HD_DAL_VIDEOPROC_MAX)) { \
			(rv) = _hd_videoproc_in_id_str(dev_id, in_id, p_str, str_len); \
		} else if(((dev_id) >= HD_DAL_AUDIOOUT_BASE) && ((dev_id) <= HD_DAL_AUDIOOUT_MAX)) { \
			(rv) = _hd_audioout_in_id_str(dev_id, in_id, p_str, str_len); \
		} \
	} while(0)

#define _HD_CONVERT_SRC_OUT_STR(rv, dev_id, out_id, p_str, str_len) \
	do { \
		(rv) = 0; \
		if((dev_id) == 0) { \
			(rv) = snprintf(p_str, str_len, "(null)"); \
		} else if(((dev_id) >= HD_DAL_VIDEOCAP_BASE) && ((dev_id) <= HD_DAL_VIDEOCAP_MAX)) { \
			(rv) = _hd_videocap_out_id_str(dev_id, out_id, p_str, str_len); \
		} else if(((dev_id) >= HD_DAL_VIDEOENC_BASE) && ((dev_id) <= HD_DAL_VIDEOENC_MAX)) { \
			(rv) = _hd_videoenc_out_id_str(dev_id, out_id, p_str, str_len); \
		} else if(((dev_id) >= HD_DAL_VIDEOPROC_BASE) && ((dev_id) <= HD_DAL_VIDEOPROC_MAX)) { \
			(rv) = _hd_videoproc_out_id_str(dev_id, out_id, p_str, str_len); \
		} else if(((dev_id) >= HD_DAL_AUDIOCAP_BASE) && ((dev_id) <= HD_DAL_AUDIOCAP_MAX)) { \
			(rv) = _hd_audiocap_out_id_str(dev_id, out_id, p_str, str_len); \
		} \
	} while(0)


int _hd_dest_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len)
{
	int rv;
	_HD_CONVERT_DEST_IN_STR(rv, dev_id, in_id, p_str, str_len);
	return rv;
}

int _hd_src_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len)
{
	int rv;
	_HD_CONVERT_SRC_OUT_STR(rv, dev_id, out_id, p_str, str_len);
	return rv;
}

int _hd_video_pipe_str(UINT32 pipe, CHAR *p_str, INT str_len)
{
	switch (pipe) {
		case HD_VIDEOPROC_PIPE_OFF:             snprintf(p_str, str_len, "OFF");  break;
		case HD_VIDEOPROC_PIPE_RAWALL:          snprintf(p_str, str_len, "RAWALL");  break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		case HD_VIDEOPROC_PIPE_RAWCAP:          snprintf(p_str, str_len, "RAWCAP");  break;
#endif
		case HD_VIDEOPROC_PIPE_YUVALL:          snprintf(p_str, str_len, "YUVALL");  break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		case HD_VIDEOPROC_PIPE_YUVCAP:          snprintf(p_str, str_len, "YUVCAP");  break;
		case HD_VIDEOPROC_PIPE_YUVAUX:          snprintf(p_str, str_len, "YUVAUX");  break;
		case HD_VIDEOPROC_PIPE_DEWARP:          snprintf(p_str, str_len, "DEWARP");  break;
#endif
		case HD_VIDEOPROC_PIPE_COLOR:           snprintf(p_str, str_len, "COLOR");  break;
		case HD_VIDEOPROC_PIPE_SCALE:           snprintf(p_str, str_len, "SCALE");  break;
		case HD_VIDEOPROC_PIPE_PANO360:         snprintf(p_str, str_len, "PANO360");  break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		case HD_VIDEOPROC_PIPE_PANO360_4V:      snprintf(p_str, str_len, "PANO360_4V");  break;
#endif
#if defined(_BSP_NA51055_) // VPE related
		case HD_VIDEOPROC_PIPE_VPE:             snprintf(p_str, str_len, "VPE");  break;
#endif
		case HD_VIDEOPROC_PIPE_RAWALL|0x800:    snprintf(p_str, str_len, "RAWALL-DIS");  break;
		case HD_VIDEOPROC_PIPE_YUVALL|0x800:    snprintf(p_str, str_len, "YUVALL-DIS");  break;
		case HD_VIDEOPROC_PIPE_SCALE|0x800:     snprintf(p_str, str_len, "SCALE-DIS");  break;
		default:  snprintf(p_str, str_len, "(?)");
			return (-1);
	}
	return 0;
}

int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len)
{
	switch (pxlfmt) {
		case HD_VIDEO_PXLFMT_NONE:            	snprintf(p_str, str_len, "0");  break;
		//OSD
		case HD_VIDEO_PXLFMT_I8:                snprintf(p_str, str_len, "INDEX8");    break;
		case HD_VIDEO_PXLFMT_ARGB1555:          snprintf(p_str, str_len, "ARGB1555");    break;
		case HD_VIDEO_PXLFMT_ARGB4444:          snprintf(p_str, str_len, "ARGB4444");    break;
		case HD_VIDEO_PXLFMT_ARGB8565:          snprintf(p_str, str_len, "ARGB8565");    break;
		case HD_VIDEO_PXLFMT_ARGB8888:          snprintf(p_str, str_len, "ARGB8888");    break;
		//RAW
		case HD_VIDEO_PXLFMT_RAW8:            	snprintf(p_str, str_len, "RAW8");  break;
		case HD_VIDEO_PXLFMT_RAW10:            	snprintf(p_str, str_len, "RAW10");  break;
		case HD_VIDEO_PXLFMT_RAW12:            	snprintf(p_str, str_len, "RAW12");  break;
		case HD_VIDEO_PXLFMT_RAW14:            	snprintf(p_str, str_len, "RAW14");  break;
		case HD_VIDEO_PXLFMT_RAW16:            	snprintf(p_str, str_len, "RAW16");  break;
		case HD_VIDEO_PXLFMT_RAW8_SHDR2:       	snprintf(p_str, str_len, "RAW8s2");  break;
		case HD_VIDEO_PXLFMT_RAW10_SHDR2:      	snprintf(p_str, str_len, "RAW10s2");  break;
		case HD_VIDEO_PXLFMT_RAW12_SHDR2:      	snprintf(p_str, str_len, "RAW12s2");  break;
		case HD_VIDEO_PXLFMT_RAW14_SHDR2:      	snprintf(p_str, str_len, "RAW14s2");  break;
		case HD_VIDEO_PXLFMT_RAW16_SHDR2:      	snprintf(p_str, str_len, "RAW16s2");  break;
		case HD_VIDEO_PXLFMT_RAW8_SHDR3:       	snprintf(p_str, str_len, "RAW8s3");  break;
		case HD_VIDEO_PXLFMT_RAW10_SHDR3:      	snprintf(p_str, str_len, "RAW10s3");  break;
		case HD_VIDEO_PXLFMT_RAW12_SHDR3:      	snprintf(p_str, str_len, "RAW12s3");  break;
		case HD_VIDEO_PXLFMT_RAW14_SHDR3:      	snprintf(p_str, str_len, "RAW14s3");  break;
		case HD_VIDEO_PXLFMT_RAW16_SHDR3:      	snprintf(p_str, str_len, "RAW16s3");  break;
		//NRX
		case HD_VIDEO_PXLFMT_NRX12:             snprintf(p_str, str_len, "NRX12");  break;
		case HD_VIDEO_PXLFMT_NRX16:             snprintf(p_str, str_len, "NRX16");  break;
		case HD_VIDEO_PXLFMT_NRX12_SHDR2:       snprintf(p_str, str_len, "NRX12s2");  break;
		case HD_VIDEO_PXLFMT_NRX16_SHDR2:       snprintf(p_str, str_len, "NRX16s2");  break;
		case HD_VIDEO_PXLFMT_NRX12_SHDR3:       snprintf(p_str, str_len, "NRX12s3");  break;
		case HD_VIDEO_PXLFMT_NRX16_SHDR3:       snprintf(p_str, str_len, "NRX16s3");  break;
		//YUV
		case HD_VIDEO_PXLFMT_YUV400:            snprintf(p_str, str_len, "YUV400");  break;
		case HD_VIDEO_PXLFMT_YUV420:            snprintf(p_str, str_len, "YUV420");  break;
		case HD_VIDEO_PXLFMT_YUV422:            snprintf(p_str, str_len, "YUV422");  break;
		case HD_VIDEO_PXLFMT_YUV420_PLANAR:     snprintf(p_str, str_len, "YUV420p");  break;
		case HD_VIDEO_PXLFMT_YUV422_PLANAR:     snprintf(p_str, str_len, "YUV422p");  break;
		case HD_VIDEO_PXLFMT_YUV444_PLANAR:     snprintf(p_str, str_len, "YUV444p");  break;
		//NVX
		case HD_VIDEO_PXLFMT_YUV420_NVX1_H264:  snprintf(p_str, str_len, "NVX1h4");    break;
		case HD_VIDEO_PXLFMT_YUV420_NVX1_H265:  snprintf(p_str, str_len, "NVX1h5");    break;
		case HD_VIDEO_PXLFMT_YUV420_NVX2:       snprintf(p_str, str_len, "NVX2");    break;
		default:  snprintf(p_str, str_len, "(?)");
			return (-1);
	}
	return 0;
}

int _hd_video_dir_str(HD_VIDEO_DIR dir, CHAR *p_str, INT str_len)
{
	CHAR dir_mask[5] = "....";
	if(dir & HD_VIDEO_DIR_MIRRORX) {
		dir_mask[0] = 'X';
	}
	if(dir & HD_VIDEO_DIR_MIRRORY) {
		dir_mask[1] = 'Y';
	}
	if((dir&HD_VIDEO_DIR_ROTATE_MASK) ==HD_VIDEO_DIR_ROTATE_90) {
		dir_mask[2] = 'R';
	}
	if((dir&HD_VIDEO_DIR_ROTATE_MASK) ==HD_VIDEO_DIR_ROTATE_270) {
		dir_mask[3] = 'L';
	}
	return snprintf(p_str, str_len, "%s", dir_mask);
}


