/* =================================================================
 *
 *      nvt_gmi_ioctl.h
 *
 *      General Multimedia Interface ioctl header file
 *
 * =================================================================
 */

#ifndef _NVTGMI_IOCTL_H
#define _NVTGMI_IOCTL_H

#include "nvt_type.h"

#define STRING_LEN                      31
#define GMX_MAX_CPU_NUM_PER_CHIP        2
#define GMX_MAX_CHIP_NUM                4
#define GMX_MAX_CAPTURE_CHANNEL_NUM     128
#define GMX_MAX_LCD_NUM                 6
#define GMX_MAX_AU_GRAB_NUM             40
#define GMX_MAX_AU_RENDER_NUM           32
#define GMX_MAX_CAP_PATH                8
#define GMX_MAX_LCD_OUT                 1
#define GMX_MAX_AUDIO_IN_COUNT          1
#define GMX_MAX_AUDIO_OUT_COUNT         1
typedef struct _CAP_INFO {
	UINT32  Valid;
	UINT32  Width;
	UINT32  Height;
	UINT32  FrameRate;
	UINT32  ScanMethod;
	UINT32  Feature;
	UINT32  MaxByteRate;
	UINT32  SensorId;
	UINT32  SensorFrontFps;
	UINT32  ImeId;
	UINT32  ImeCompressed;
	UINT32  Ime3DNR;
	UINT32  ImeFps;
	UINT32  BufferCount;
	UINT32  PathId;
} CAP_INFO;

typedef struct _LCD_INFO {
	UINT32  Valid;
	UINT32  Width;
	UINT32  Height;
	UINT32  FrameRate;
	UINT32  ScanMethod;
	UINT32  VoutId;
} LCD_INFO;

#define	MAX_ENV_INFO_STRING_LEN	31

typedef struct _ENV_INFO {
	char SensorName[MAX_ENV_INFO_STRING_LEN + 1];
	char DisplayName[MAX_ENV_INFO_STRING_LEN + 1];
	char EmbeddedMemoryName[MAX_ENV_INFO_STRING_LEN + 1];
	char DSPFeature[MAX_ENV_INFO_STRING_LEN + 1];
	char BuildDate[MAX_ENV_INFO_STRING_LEN + 1];

	struct {
		unsigned long  addr;
		unsigned long  size;
	} mem_partition[8];	//0:ipc 1:linux 2:uboot 3:dsp1 4:uitron 5:ramdisk 6:loader 7:extdram
	struct {
		unsigned long  addr;
		unsigned long  size;
		unsigned long  free;
		unsigned long  max_free_block;
	} mem_status[2];  //0:ddr-1, 1:ddr-2

	unsigned int  fd_facescale;
	unsigned int  direct_mode : 1;
	unsigned int  define_product : 4;
	unsigned int  define_new_trigger : 1;
	unsigned int  define_ipl_direct : 1;
	unsigned int  define_ipl_3dnr : 1;
	unsigned int  define_enc_3dnr : 1;
	unsigned int  define_disp_wait : 1;
	unsigned int  define_dis : 1;
	unsigned int  define_md_func : 1;
	unsigned int  define_smartroi : 1;
	unsigned int  define_vqa_func : 1;
	unsigned int  define_pd_func : 1;
	unsigned int  define_fd_func : 1;
	unsigned int  define_aec_func : 1;
	unsigned int  define_anr_func : 1;
	unsigned int  define_ltr_func : 1;
	unsigned int  define_wdr_func : 1;
	unsigned int  define_hdr_func : 1;
	unsigned int  define_ftg_func : 1;
	unsigned int  define_buf_level : 4;
	unsigned int  define_reserved : 6;
	unsigned int  md_path;

	unsigned int  audio_enc_pcm : 1;
	unsigned int  audio_enc_aac : 1;
	unsigned int  audio_enc_alaw : 1;
	unsigned int  audio_enc_ulaw : 1;
	unsigned int  audio_enc_reserved : 12;
	unsigned int  audio_dec_pcm : 1;
	unsigned int  audio_dec_aac : 1;
	unsigned int  audio_dec_alaw : 1;
	unsigned int  audio_dec_ulaw : 1;
	unsigned int  audio_dec_reserved : 12;

	unsigned int  max_video_buf_time_ms;
	unsigned int  max_audio_in_buf_time_ms;
	unsigned int  max_audio_out_buf_time_ms;

} ENV_INFO;

#define AU_INFO_SAMPE_RATE_8K       (1 << 0)    /*  8000 Hz */
#define AU_INFO_SAMPE_RATE_11K      (1 << 1)    /* 11025 Hz */
#define AU_INFO_SAMPE_RATE_12K      (1 << 2)    /* 12000 Hz */
#define AU_INFO_SAMPE_RATE_16K      (1 << 3)    /* 16000 Hz */
#define AU_INFO_SAMPE_RATE_22K      (1 << 4)    /* 22050 Hz */
#define AU_INFO_SAMPE_RATE_24K      (1 << 5)    /* 24000 Hz */
#define AU_INFO_SAMPE_RATE_32K      (1 << 6)    /* 32000 Hz */
#define AU_INFO_SAMPE_RATE_44K      (1 << 7)    /* 44100 Hz */
#define AU_INFO_SAMPE_RATE_48K      (1 << 8)    /* 48000 Hz */

#define AU_INFO_SAMPE_SIZE_8BIT     (1 << 0)    /* 8 Bit   */
#define AU_INFO_SAMPE_SIZE_16BIT    (1 << 1)    /* 16 bits */
#define AU_INFO_SAMPE_SIZE_32BIT    (1 << 2)    /* 32 bits */

#define AU_INFO_CHANNEL_LEFT        (1 << 0)
#define AU_INFO_CHANNEL_RIGHT       (1 << 1)
#define AU_INFO_CHANNEL_STEREO      (1 << 2)

typedef struct _AUDIO_IN_INFO {
	UINT32	Valid;      // 0: not used
	UINT32  SampleRate_Support_bitmap;
	UINT32  SampleSize_Support_bitmap;
	UINT32  Channel_Support_bitmap;
	UINT32  Source;

} AUDIO_IN_INFO;

typedef struct _AUDIO_OUT_INFO {
	UINT32	Valid;      // 0: not used
	UINT32  SampleRate_Support_bitmap;
	UINT32  SampleSize_Support_bitmap;
	UINT32  Channel_Support_bitmap;
	UINT32  Source;
} AUDIO_OUT_INFO;

typedef struct _SYS_INFO {
	CAP_INFO CapInfo[GMX_MAX_CAP_PATH];
	LCD_INFO LCDInfo[GMX_MAX_LCD_OUT];
	AUDIO_IN_INFO AuInInfo[GMX_MAX_AUDIO_IN_COUNT];
	AUDIO_OUT_INFO AuOutInfo[GMX_MAX_AUDIO_OUT_COUNT];
	ENV_INFO EnvInfo;
} SYS_INFO;


typedef struct {
	/* 1st U32 */
	int valid: 2;
	int start_ddr_no: 8;
	int end_ddr_no: 8;
	unsigned int reserved1: 14; ///< Reserved bits
	/* 2nd U32 */
	int start_cap_vch: 9;
	int end_cap_vch: 9;
	int reserved2: 14; ///< Reserved bits
	/* 3nd U32 */
	int start_file_vch: 9;
	int end_file_vch: 9;
	int reserved3: 14; ///< Reserved bits
	/* others */
	int reserved[4];         ///< Reserved words
} gmx_chip_info_t;

typedef struct {
	int vcapch;  // vcapch < 0 : the entry is invalid, capture internal ch number
	unsigned int num_of_path;
	unsigned int scan_method;
	unsigned int fps;
	unsigned int width;
	unsigned int height;
	unsigned int feature;
	int sensor_id;
	int ime_id;
	int	path_index;
	unsigned int max_byterate;
	int  sensor_front_fps;
	int  ime_compressed;
	int  ime_3dnr;
	int  ime_fps;
	unsigned int buffer_count;
	unsigned int path_id;

	int reserved[5];         ///< Reserved words
} gmx_cap_sys_info_t;

typedef struct {
	char         name[48];
	unsigned int id;
	unsigned int width;
	unsigned int height;
} gmx_lcd_res_info_t;

typedef struct {
	/* 1st U32 interger */
	unsigned int max_disp_width: 16;
	unsigned int max_disp_height: 16;
	/* 2st U32 interger */
	unsigned int max_disp_rate: 9;
	int reserved1: 23; /* reserved */
	/* others */
	unsigned int reserved[2];         ///< Reserved words
} gmx_lcd_config_info_t;

typedef struct {
	/* 1st U32_int integer */
	int          lcdid: 4;   //lcdid < 0 : the entry is invalid
	unsigned int chipid: 2;
	unsigned int is3di: 1;
	unsigned int fps: 9;
	unsigned int lcd_type: 8;
	unsigned int timestamp_by_ap: 1;
	unsigned int pool_type: 7;
	/* 2nd U32_int integer */
	unsigned int channel_zero: 1;       /*1:support channel zero 0:not support*/
	unsigned int lcd_vch: 3;
	int src_duplicate_vch: 4;  //duplicate from which lcd_vch
	unsigned int src_duplicate_region: 2;  //duplicate region
	int reserved2: 22; /* reserved */
	/* others */
	gmx_lcd_res_info_t fb0_win;
	gmx_lcd_res_info_t desk_res;
	gmx_lcd_res_info_t output_type;
	unsigned int       fb_vaddr;
	unsigned int       vout_id;
	/* 3nd U32_int interger */
	int active: 16;
	int reserved3: 16;
	/* others */
	gmx_lcd_config_info_t lcd_gmlib_cfg[5];
	unsigned int reserved[8];         ///< Reserved words
} gmx_lcd_sys_info_t;

typedef struct {
	int ch: 10;
	unsigned int sample_rate_support_bmp;
	unsigned int sample_size_support_bmp;
	unsigned int channels_type_support_bmp;
	unsigned int source;
	char description[STRING_LEN + 1];
	int reserved2[5];         ///< Reserved words
} gmx_au_grab_sys_info_t;

typedef struct {
	int ch: 10;
	unsigned int sample_rate_support_bmp;
	unsigned int sample_size_support_bmp;
	unsigned int channels_type_support_bmp;
	unsigned int source;
	char description[STRING_LEN + 1];
	int reserved2[5];         ///< Reserved words
} gmx_au_render_sys_info_t;

typedef struct {
	/* 1st U32_int integer */
	int b_frame_nr: 4;
	unsigned int reserved: 28;
	int reserved2[2];         ///< Reserved words
} gmx_dec_sys_info_t;

typedef struct {
	/* 1st U32_int integer */
	int b_frame_nr: 4;
	unsigned int reserved: 28;
	int reserved2[2];         ///< Reserved words
} gmx_enc_sys_info_t;

typedef struct {
	char sensor_name[MAX_ENV_INFO_STRING_LEN + 1];
	char display_name[MAX_ENV_INFO_STRING_LEN + 1];
	char embedded_memory_name[MAX_ENV_INFO_STRING_LEN + 1];
	char dsp_feature[MAX_ENV_INFO_STRING_LEN + 1];
	char build_date[MAX_ENV_INFO_STRING_LEN + 1];

	struct {
		unsigned long  addr;
		unsigned long  size;
	} mem_partition[8];	//0:ipc 1:linux 2:uboot 3:dsp1 4:uitron 5:ramdisk 6:loader 7:extdram
	struct {
		unsigned long  addr;
		unsigned long  size;
		unsigned long  free;
		unsigned long  max_free_block;
	} mem_status[2];  //0:ddr-1, 1:ddr-2

	unsigned int  fd_facescale;
	unsigned int  direct_mode : 1;
	unsigned int  define_product : 4;
	unsigned int  define_new_trigger : 1;
	unsigned int  define_ipl_direct : 1;
	unsigned int  define_ipl_3dnr : 1;
	unsigned int  define_enc_3dnr : 1;
	unsigned int  define_disp_wait : 1;
	unsigned int  define_dis : 1;
	unsigned int  define_md_func : 1;
	unsigned int  define_smartroi : 1;
	unsigned int  define_vqa_func : 1;
	unsigned int  define_pd_func : 1;
	unsigned int  define_fd_func : 1;
	unsigned int  define_aec_func : 1;
	unsigned int  define_anr_func : 1;
	unsigned int  define_ltr_func : 1;
	unsigned int  define_wdr_func : 1;
	unsigned int  define_hdr_func : 1;
	unsigned int  define_ftg_func : 1;
	unsigned int  define_buf_level : 4;
	unsigned int  define_reserved : 6;
	unsigned int  md_path;

	unsigned int  audio_enc_pcm : 1;
	unsigned int  audio_enc_aac : 1;
	unsigned int  audio_enc_alaw : 1;
	unsigned int  audio_enc_ulaw : 1;
	unsigned int  audio_enc_reserved : 12;
	unsigned int  audio_dec_pcm : 1;
	unsigned int  audio_dec_aac : 1;
	unsigned int  audio_dec_alaw : 1;
	unsigned int  audio_dec_ulaw : 1;
	unsigned int  audio_dec_reserved : 12;

	unsigned int  max_video_buf_time_ms;
	unsigned int  max_audio_in_buf_time_ms;
	unsigned int  max_audio_out_buf_time_ms;

} gmx_environment_info_t;

typedef struct {
	unsigned int graph_type;
	char graph_name[48];
	gmx_chip_info_t         chip_info[GMX_MAX_CHIP_NUM];
	gmx_cap_sys_info_t      cap_info[GMX_MAX_CAPTURE_CHANNEL_NUM];
	gmx_lcd_sys_info_t      lcd_info[GMX_MAX_LCD_NUM];
	gmx_au_grab_sys_info_t  au_grab_info[GMX_MAX_AU_GRAB_NUM];
	gmx_au_render_sys_info_t au_render_info[GMX_MAX_AU_RENDER_NUM];
	gmx_dec_sys_info_t      dec_info;
	gmx_enc_sys_info_t      enc_info;
	gmx_environment_info_t  env_info;

	//gmx_spec_info_t         spec_info;
	int transcode_lcd_num;
	//gmx_buffer_fmt_t buffer;
	char liveview_cap_dma;
	char record_cap_dma;
	char reserved1[2];
	int reserved2[8];         ///< Reserved words
} __attribute__((packed, aligned(8))) gmx_sys_info_t;


void printm(char *module, const char *fmt, ...);
int damnit(char *module);


#define MAX_LOG_MSG_LEN		128
typedef struct {
    unsigned int length;
    unsigned char *str;
} nvt_gmi_log_msg_t;

typedef struct {
    unsigned int dbg_level;
    unsigned int dbg_mode;
    unsigned int perf;       //statistic, 0(off)  >0(mini-sec interval for each print)
    unsigned int vout0_type;
    unsigned int vout1_type;
} nvt_gmi_lib_config_t;

#define NVT_GMI_IOC_MAGIC    'G'
#define NVT_GMI_IOCCMD_SEND_LOG        _IOW(NVT_GMI_IOC_MAGIC, 0, nvt_gmi_log_msg_t)
#define NVT_GMI_IOCCMD_GET_LIB_CONFIG  _IOR(NVT_GMI_IOC_MAGIC, 1, nvt_gmi_lib_config_t)
#define NVT_GMI_IOCCMD_SET_LIB_CONFIG  _IOW(NVT_GMI_IOC_MAGIC, 2, nvt_gmi_lib_config_t)



typedef enum {
	GMX_NOTIFY_UPDATE_GMI_CONFIG,
}gmx_notify_type_t;

typedef struct {
	int notify_type;

	union {
		struct {
			nvt_gmi_lib_config_t config;
		} update_gmi_config_mode;
	};
} gmx_notify_data_t;


#endif /* _NVTGMI_IOCTL_H */
