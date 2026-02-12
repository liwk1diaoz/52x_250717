#ifndef _HLS_FILE_OPERATION
#define _HLS_FILE_OPERATION

#include "hdal.h"
#include "hdal_flow.h" // get get VIDEO_RECORD structure

#define HLS_DRIVE "A:\\"
#define HLS_PREFIX_NAME "nvt_ts"
#define HLS_EXTENSION_NAME ".ts"
#define HLS_MAX_OPENED_FILE_NUM 65535
#define HLS_M3U8_INCLUDE_MAX_FILE 2
#define M3U8_FILE_NAME	"stream.m3u8"
#define M3U_FILE_NAME	"stream.m3u"
#define HLS_WLAN0 "wlan0"
#define HLS_ETH0 "eth0"
#define HLS_NET_INTERFACE	HLS_ETH0

HD_RESULT 	get_file_name(CHAR *file_path,UINT32 *name_len);
HD_RESULT 	filesys_init(CHAR *folder);
HD_RESULT 	create_m3u8_file(VIDEO_RECORD *stream);
HD_RESULT 	update_m3u8_file(CHAR *fpath, VIDEO_RECORD *stream);
HD_RESULT 	delete_old_ts_file(CHAR *fpath, VIDEO_RECORD *stream);
INT32 		need_delete_old_ts_file(CHAR *fpath, VIDEO_RECORD *stream);
HD_RESULT 	filesys_close(void);
#endif
