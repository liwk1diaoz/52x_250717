/*
 * Audio Noise Reduction (ANR) program is used for commond noise redunction application.
 * Including background noise estimation at the begging of ANR process.
 * Copyright (C) 2017, Echo Hsu, Novatek
 */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef __KERNEL__
#include <string.h>            // memset,
#include <stdlib.h>            // calloc,
#include <stdio.h>             // printf,
#else
#include "audlib_anr_dbg.h"
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#endif
#include "type.h"
#include "kdrv_audioio/audlib_anr.h"
#include "kdrv_audioio/kdrv_audioio.h"

#include "anr_err.h"
//#include "utility.h"
//#include "fxpt_fft_1.h"
#include "fxpt_fft_2.h"
#include "fxpt_sqrt.h"

#define ANR_VER               (0x00020601)

static KDRV_AUDIOLIB_FUNC anr_func =
{
	.anr.get_version			= audlib_anr_get_version,
	.anr.pre_init				= audlib_anr_pre_init,
	.anr.init					= audlib_anr_init,
	.anr.set_snri				= audlib_anr_set_snri,
	.anr.get_snri				= audlib_anr_get_snri,
	.anr.detect_reset			= audlib_anr_detect_reset,
	.anr.detect					= audlib_anr_detect,
	.anr.run					= audlib_anr_run,
	.anr.destroy				= audlib_anr_destroy,
};

// =================== Options ===================
//#define LOCK_813x
//#define ALLOC_MEM_INSIDE
#define free(x) x
#define time0(func) func
// =================== Defines ===================
#define KEY_BASE            (200)

#define MAX_Q_LEN            (14)    // 在 48kHz, Wing_L=5 的情況下算出來的

#define OPTION_CB_MEAN          (1)
#define OPTION_CB_MAX           (2)
#define OPTION_CB_MIN           (3)
//#define OPTION_CB_SUM         (4)        // output should be changed to integer pointer

#define OPTION_CB_SEPARATED     (0)
#define OPTION_CB_CONSEQUENCE   (1)

#define OPTION_PEAKTRIM_NOISE   (0)
#define OPTION_PEAKTRIM_SIGNAL  (1)

#define CHANNEL_L               (0)
#define CHANNEL_R               (1)

#define NPINK_QLEN              (16)
#define NCOLOR_QLEN             (32)

#define NR_FADE_UNIT_EXP        (3)      // 1/(2^3)=0.125, 1dB=1.1220
#define NR_FADE_CEIL_STEPS      (12)

#define DEBUGANR (0)

// =================== Macro ===================
#define SWAP(x,y) (x)^=(y);(y)^=(x);(x)^=(y)
#define ABS(x)     ((x) < 0 ? -(x) : (x))  // get absolute value
#define SIGN(v)      ((v) > 0 ? 1 : ((v) < 0 ? -1 : 0))
#define ALIGN4(x)   ((((x)+3)>>2)<<2)

// =================== Structure ===================
int err_code;

#ifdef LOCK_813x
static CHAR sShit[9];
static INT32 Dead;
#endif

static int Ring_of_Key[MAX_STREAM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define BIAS_CLIP              (8)
#define FRMS_PER_SLICE_EXP     (5)      // 5:0.5秒, 6:1秒, 7:2秒
#define FRMS_PER_SLICE         (1<<FRMS_PER_SLICE_EXP)
struct _Detect {
	INT32 Status;   // NOISE_DETECTING(0), NOISE_UPDATING(1), NOISE_UPDATED(2)

	INT32 Acc[BIAS_CLIP];
	INT32 RepeatHst[BIAS_CLIP];
	INT32 RepeatHstLow[BIAS_CLIP];
	INT32 RepeatHstSum;
	INT32 RepeatIdx;     // 先假設最有可能的索引值 3(Matlab)
	INT32 FCnt;
	INT32 SMode;
	UINT32 EngG1;    // Qu16.3
};

struct _Detect_LR {
	struct _Detect Detect_L;
	struct _Detect Detect_R;
};

struct _Detect_LR Detect_Stream[MAX_STREAM];

struct _Detect *pDetect;

//--------------------------------------

#define OPTION_MEM_REQIRE    (0)
#define OPTION_MEM_SET        (1)
struct stMEM_Info {
	int     ST_CFG_MEM;
	void *pST_CFG_MEM;
	int     ST_VAR_MEM;
	void *pST_VAR_MEM;
	int     ST_NPINK_MEM;
	void *pST_NPINK_MEM;
	int     ST_NCOLOR_MEM;
	void *pST_NCOLOR_MEM;
	int     ST_NTONE_MEM;
	void *pST_NTONE_MEM;
	int     ST_LINK_MEM;
	void *pST_LINK_MEM;
	int     ST_QU_MEM;
	void *pST_QU_MEM;

	int     KFFT_MEM;
	void *pKFFT_MEM;
	int     VAR_MEM;
	void *pVAR_MEM;
	int     NPINK_MEM;
	void *pNPINK_MEM;
	int     NCOLOR_MEM;
	void *pNCOLOR_MEM;
	int     NTONE_MEM;
	void *pNTONE_MEM;
	int     QBASE_MEM;
	void *pQBASE_MEM;

	int     LINK_MEM;
	void *pLINK_MEM;
	int     TMP_MEM;
	void *pTMP_MEM;

	int     QMARK_MEM;
	void *pQMARK_MEM;
};
struct stMEM_Info Mem_Info;

//--------------------------------------

struct kfft_info   *KFFT_Cfg0_Stream[MAX_STREAM];   // fft_2 config
struct kfft_info *pKFFT_Cfg0;    // fft_2 config

struct kfft_info   *KFFT_Cfg1_Stream[MAX_STREAM];   // ifft_2 config
struct kfft_info *pKFFT_Cfg1;    // ifft_2 config

//--------------------------------------
struct _Cfg {
	INT32 sampling_rate;  // 8000,11025,16000,22050,32000,44100,48000
	INT32 stereo;         // 0:Mono, 1:stereo
	INT32 Blk1CSizeW;
	INT32 Freq_Max;
	INT32 FFT_SZ_Exp;
	INT32 FFT_SZ;
	INT32 FFT_OV_SZ;
	INT32 Wing_L;
	INT32 Wing_R;
	INT32 Q_Center;
	INT32 Q_Len;
	INT32 Idx_1k;
	INT32 Link_Nodes;
	INT32 SPEC_LEN;
	INT32 NR_SIdx_Low;
	INT32 NR_SIdx_High;
	INT32 CB_Len;
	const INT16 *CB_Start;
	const INT16 *CB_End;
	const INT16 *CB_Num;
	INT32 Valid_SIdx_Low;
	INT32 Valid_SIdx_High;
	INT32 spec_bias_low;
	INT32 spec_bias_high;
	INT32 nr_db;                // [3~35]
	INT32 bias_sensitive;       // [1~9]
	INT32 noise_est_hold_time; // [1~10]
	INT32 HPF_CutOffBin;
	UINT32 max_bias_limit;       // Qu3.6
	INT32 RLink_Len;
    INT32 m_curve_n1_level;
    INT32 m_curve_n2_level;
};
struct _Cfg *pCfg_Stream[MAX_STREAM];
struct _Cfg *pCfg;
//--------------------------------------

struct _Var {
	UINT32     InFrmCnt;      // 0,1,2,...
	UINT32     OutFrmCnt;     // 0,1,2,...
	UINT32     First_SS;      // 0,1

	UINT32     max_bias;       // Qu26.6

	UINT32     default_bias;  // Qu10.6
	UINT32     BG_Bias_Target;// Qu10.6
	UINT32     BG_Bias_HD;    // Qu10.16

	INT32      HH;            // Qu2.6 :[1.0 ~ 3)
	INT32      LL;            // Qu2.6 :[1.0 ~ 3)
    INT32	   MCurveN1;	  // Qu2.6 :[1.0 ~ HH]
    INT32	   MCurveN2;	  // Qu2.6 :[1.0 ~ LL]

	UINT32     default_eng;   // Qu23, 16bit blur頻譜能量和

	INT32      pQabs_GSum_G     [4][MAX_Q_LEN];
	INT32      pQabs_blur_GSum_G[4][MAX_Q_LEN];

	UINT32     QBias[MAX_Q_LEN];     // Qu10.6, 只用到前 0~Center, 後面沒用到
	UINT32     QEngG[MAX_Q_LEN];     // Qu23, blur頻譜的和(偵測時為blur頻譜的平均和)

	INT32      NRTable[NR_FADE_CEIL_STEPS + 1]; // Qu(1.15-8)=Qu1.7 for 8kHz(256 frm sz), Qu(1.15-9)=Q1.6 for 16kHz(512 frm sz)

	//VOID *     MemBase;
	struct kfft_data *asFFT0;         // [pCfg->FFT_SZ];
	struct kfft_data *asFFT1;         // [pCfg->FFT_SZ];

	UINT16    *Default_CB;    // [pCfg->CB_Len];        // Qu16, pVar->default_spec 的 CB
	INT16     *asNRout;       // [pCfg->FFT_OV_SZ];     // 上一個frame後半 + 現在frame前半
	INT32     *asNRtail;      // [pCfg->FFT_OV_SZ];     // 上一個frame的後半
	INT16     *asBufPadIn;    // [pCfg->FFT_OV_SZ];     // 前一個block的後面半個frame

	UINT32    *BG_Spec_HD;    // [pCfg->SPEC_LEN];      // Qu16.10
	UINT16    *BG_Spec_Target;// [pCfg->SPEC_LEN];      // Qu16
	UINT16    *default_spec;  // [pCfg->SPEC_LEN];      // Qu16, 16bit blur 噪音段頻譜最大值

	UINT16    *asNRabs;       // [pCfg->SPEC_LEN];      // Qu16
	INT16     *Mark_Smooth;   // [pCfg->SPEC_LEN];      // 0~10
	UINT16    *BGboost;       // [pCfg->SPEC_LEN];      // Qu16
	UINT32    *Sig_MCurve;    // [pCfg->SPEC_LEN];        // Qu2.6
};
struct _VAR_LR {
	struct _Var *pVar_L;
	struct _Var *pVar_R;
};
struct _VAR_LR Var_Stream[MAX_STREAM];

struct _Var *pVar;
//--------------------------------------

struct _Tbl {
	const INT16 CB_Start_8k[35];
	const INT16   CB_End_8k[35];
	const INT16   CB_Num_8k[35];

	const INT16 CB_Start_11k[41];
	const INT16   CB_End_11k[41];
	const INT16   CB_Num_11k[41];

	const INT16 CB_Start_16k[47];
	const INT16   CB_End_16k[47];
	const INT16   CB_Num_16k[47];

	const INT16 CB_Start_22k[52];
	const INT16   CB_End_22k[52];
	const INT16   CB_Num_22k[52];

	const INT16 CB_Start_32k[59];
	const INT16   CB_End_32k[59];
	const INT16   CB_Num_32k[59];

	const INT16 CB_Start_44k[64];
	const INT16   CB_End_44k[64];
	const INT16   CB_Num_44k[64];

	const INT16 CB_Start_48k[66];
	const INT16   CB_End_48k[66];
	const INT16   CB_Num_48k[66];

	const INT32  CB_Mul_2e11[71];

	const UINT16 NRTable_03[NR_FADE_CEIL_STEPS + 1];   // Qu1.15, 0x8000=32768 表示為 1
	const UINT16 NRTable_35[NR_FADE_CEIL_STEPS + 1];   // Qu1.15, 32768*10^(-96dB/20)= 0.5193

	// ==================================================================================================
	//                        +0dB,      +1dB,       +2dB,       +3dB,       +4dB,       +5dB,        +6dB
	// Matlab : Tbl.dBup   = [      10^(1/20),  10^(2/20),  10^(3/20),  10^(4/20),  10^(5/20),  10^(6/20)];
	//   C    : Tbl.dBup   = [ 256,       287,        322,        362,        406,        455,        511];    // Qu2.8
	//
	//                        -0dB,      -1dB,       -2dB,       -3dB,       -4dB,       -5dB,        -6dB
	// Matlab : Tbl.dBdown = [     10^(-1/20), 10^(-2/20), 10^(-3/20), 10^(-4/20), 10^(-5/20), 10^(-6/20)];
	//   C    : Tbl.dBdown = [ 256,       228,        203,        181,        162,        144,        128];    // Qu2.8
	// ==================================================================================================
	const UINT16 dBup[7];      // Qu2.8
	const UINT16 dBdown[7];    // Qu2.8

	// =================== Variables ===================
	// Matlab Code:
	//      A = round(hanning(1024, 'period')*32767);
	//      CStyle(A, 16);
	const INT16 fft_window[MAX_FFT_SIZE / 2]; // < Qu15, ONLY for Spectrum Subtraction overlap add

};
struct _Tbl Tbl = {
	/* CB_Start_8k[35]; */ { 0, 1, 2, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 26, 29, 32, 35, 38, 41, 44, 48, 52, 56, 60, 65, 70, 75, 81, 87, 93, 99, 106, 113, 121},
	/* CB_End_8k[35]; */ { 0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 25, 28, 31, 34, 37, 40, 43, 47, 51, 55, 59, 64, 69, 74, 80, 86, 92, 98, 105, 112, 120, 128},
	/* CB_Num_8k[35]; */ { 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 8, 8},

	/* CB_Start_11k[41]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 31, 34, 37, 40, 43, 46, 49, 52, 56, 60, 64, 69, 74, 79, 84, 89, 95, 101, 107, 114, 121},
	/* CB_End_11k[41]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 30, 33, 36, 39, 42, 45, 48, 51, 55, 59, 63, 68, 73, 78, 83, 88, 94, 100, 106, 113, 120, 128},
	/* CB_Num_11k[41]; */ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8},

	/* CB_Start_16k[47]; */ { 0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 25, 28, 31, 34, 37, 40, 43, 47, 51, 55, 59, 63, 68, 73, 78, 84, 90, 96, 103, 110, 117, 125, 133, 141, 150, 160, 170, 180, 191, 203, 215, 228, 242},
	/* CB_End_16k[47]; */ { 0, 1, 2, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 24, 27, 30, 33, 36, 39, 42, 46, 50, 54, 58, 62, 67, 72, 77, 83, 89, 95, 102, 109, 116, 124, 132, 140, 149, 159, 169, 179, 190, 202, 214, 227, 241, 256},
	/* CB_Num_16k[47]; */ { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 13, 14, 15},

	/* CB_Start_22k[52]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 32, 35, 38, 41, 44, 47, 50, 54, 58, 62, 66, 71, 76, 81, 87, 93, 99, 105, 112, 119, 126, 134, 143, 152, 161, 171, 181, 192, 204, 216, 229, 242},
	/* CB_End_22k[52]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 31, 34, 37, 40, 43, 46, 49, 53, 57, 61, 65, 70, 75, 80, 86, 92, 98, 104, 111, 118, 125, 133, 142, 151, 160, 170, 180, 191, 203, 215, 228, 241, 256},
	/* CB_Num_22k[52]; */ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 9, 9, 9, 10, 10, 11, 12, 12, 13, 13, 15},

	/* CB_Start_32k[59]; */ { 0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 25, 28, 31, 34, 37, 40, 43, 47, 51, 55, 59, 64, 69, 74, 79, 85, 91, 97, 103, 110, 117, 125, 133, 142, 151, 161, 171, 181, 192, 204, 217, 230, 244, 259, 274, 291, 308, 326, 345, 366, 387, 410, 433, 459, 485},
	/* CB_End_32k[59]; */ { 0, 1, 2, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 24, 27, 30, 33, 36, 39, 42, 46, 50, 54, 58, 63, 68, 73, 78, 84, 90, 96, 102, 109, 116, 124, 132, 141, 150, 160, 170, 180, 191, 203, 216, 229, 243, 258, 273, 290, 307, 325, 344, 365, 386, 409, 432, 458, 484, 512},
	/* CB_Num_32k[59]; */ { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 10, 11, 12, 13, 13, 14, 15, 15, 17, 17, 18, 19, 21, 21, 23, 23, 26, 26, 28},

	/* CB_Start_44k[64]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 32, 35, 38, 41, 44, 47, 51, 55, 59, 63, 67, 72, 77, 82, 87, 93, 99, 106, 113, 120, 128, 136, 144, 153, 162, 172, 183, 194, 206, 218, 231, 245, 260, 275, 291, 308, 327, 346, 366, 387, 410, 434, 459, 485},
	/* CB_End_44k[64]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 31, 34, 37, 40, 43, 46, 50, 54, 58, 62, 66, 71, 76, 81, 86, 92, 98, 105, 112, 119, 127, 135, 143, 152, 161, 171, 182, 193, 205, 217, 230, 244, 259, 274, 290, 307, 326, 345, 365, 386, 409, 433, 458, 484, 512},
	/* CB_Num_44k[64]; */ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 10, 11, 11, 12, 12, 13, 14, 15, 15, 16, 17, 19, 19, 20, 21, 23, 24, 25, 26, 28},

	/* CB_Start_48k[66]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 34, 37, 40, 43, 46, 49, 53, 57, 61, 65, 69, 74, 79, 84, 90, 96, 102, 108, 115, 122, 130, 138, 146, 155, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 293, 310, 328, 347, 367, 388, 411, 434, 459, 485},
	/* CB_End_48k[66]; */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 33, 36, 39, 42, 45, 48, 52, 56, 60, 64, 68, 73, 78, 83, 89, 95, 101, 107, 114, 121, 129, 137, 145, 154, 164, 174, 184, 195, 207, 219, 232, 246, 261, 276, 292, 309, 327, 346, 366, 387, 410, 433, 458, 484, 512},
	/* CB_Num_48k[66]; */ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 20, 21, 23, 23, 25, 26, 28},

	/*CB_Mul_2e11[71]*/{
		2048, 1024, 683, 512, 410, 341, 293, 256, 228, 205, 186, 171, 158, 146, 137, 128, 120, 114, 108,  102,
		98,   93,  89,  85,  82,  79,  76,  73,  71,  68,  66,  64,  62,  60,  59,  57,  55,  54,  53,   51,
		50,   49,  48,  47,  46,  45,  44,  43,  42,  41,  40,  39,  39,  38,  37,  37,  36,  35,  35,   34,
		34,   33,  33,  32,  32,  31,  31,  30,  30,  29,  29
	},

	/*NRTable_03[NR_FADE_CEIL_STEPS+1]*/ { 16423, 17396, 18427, 19519, 20675, 21900, 23198, 24573, 26029, 27571, 29205, 30935, 32768},
	/*NRTable_35[NR_FADE_CEIL_STEPS+1]*/ {    10,    20,    40,    78,   152,   298,   583,  1141,  2232,  4370,  8553, 16741, 32768},

	/*dBup[7]*/        {256, 287, 322, 362, 406, 455, 511},
	/*dBdown[7]*/      {256, 228, 203, 181, 162, 144, 128},
	/*fft_window[MAX_FFT_SIZE/2]*/
	{
		0,     0,     1,     3,     5,     8,    11,    15,    20,    25,    31,    37,    44,    52,    60,    69,    79,    89,   100,   111,   123,   136,   149,   163,   177,   192,   208,   224,   241,   259,   277,   295,
		315,   335,   355,   376,   398,   420,   443,   467,   491,   516,   541,   567,   593,   621,   648,   677,   705,   735,   765,   796,   827,   859,   891,   924,   958,   992,  1027,  1062,  1098,  1134,  1171,  1209,
		1247,  1286,  1325,  1365,  1406,  1447,  1488,  1530,  1573,  1616,  1660,  1704,  1749,  1795,  1841,  1887,  1935,  1982,  2030,  2079,  2128,  2178,  2229,  2279,  2331,  2383,  2435,  2488,  2542,  2596,  2650,  2706,
		2761,  2817,  2874,  2931,  2989,  3047,  3105,  3165,  3224,  3284,  3345,  3406,  3468,  3530,  3592,  3655,  3719,  3783,  3847,  3912,  3978,  4044,  4110,  4177,  4244,  4312,  4380,  4449,  4518,  4587,  4657,  4728,
		4799,  4870,  4942,  5014,  5086,  5159,  5233,  5307,  5381,  5456,  5531,  5606,  5682,  5759,  5835,  5912,  5990,  6068,  6146,  6225,  6304,  6383,  6463,  6543,  6624,  6705,  6786,  6868,  6950,  7032,  7115,  7198,
		7281,  7365,  7449,  7534,  7618,  7703,  7789,  7875,  7961,  8047,  8134,  8221,  8308,  8396,  8484,  8572,  8660,  8749,  8838,  8928,  9017,  9107,  9197,  9288,  9379,  9470,  9561,  9652,  9744,  9836,  9929, 10021,
		10114, 10207, 10300, 10393, 10487, 10581, 10675, 10770, 10864, 10959, 11054, 11149, 11244, 11340, 11436, 11532, 11628, 11724, 11820, 11917, 12014, 12111, 12208, 12305, 12403, 12500, 12598, 12696, 12794, 12892, 12990, 13089,
		13187, 13286, 13385, 13484, 13583, 13682, 13781, 13880, 13980, 14079, 14179, 14278, 14378, 14478, 14578, 14678, 14778, 14878, 14978, 15078, 15178, 15279, 15379, 15479, 15580, 15680, 15780, 15881, 15981, 16082, 16182, 16283,
		16383, 16484, 16585, 16685, 16786, 16886, 16987, 17087, 17187, 17288, 17388, 17488, 17589, 17689, 17789, 17889, 17989, 18089, 18189, 18289, 18389, 18489, 18588, 18688, 18787, 18887, 18986, 19085, 19184, 19283, 19382, 19481,
		19580, 19678, 19777, 19875, 19973, 20071, 20169, 20267, 20364, 20462, 20559, 20656, 20753, 20850, 20947, 21043, 21139, 21235, 21331, 21427, 21523, 21618, 21713, 21808, 21903, 21997, 22092, 22186, 22280, 22374, 22467, 22560,
		22653, 22746, 22838, 22931, 23023, 23115, 23206, 23297, 23388, 23479, 23570, 23660, 23750, 23839, 23929, 24018, 24107, 24195, 24283, 24371, 24459, 24546, 24633, 24720, 24806, 24892, 24978, 25064, 25149, 25233, 25318, 25402,
		25486, 25569, 25652, 25735, 25817, 25899, 25981, 26062, 26143, 26224, 26304, 26384, 26463, 26542, 26621, 26699, 26777, 26855, 26932, 27008, 27085, 27161, 27236, 27311, 27386, 27460, 27534, 27608, 27681, 27753, 27825, 27897,
		27968, 28039, 28110, 28180, 28249, 28318, 28387, 28455, 28523, 28590, 28657, 28723, 28789, 28855, 28920, 28984, 29048, 29112, 29175, 29237, 29299, 29361, 29422, 29483, 29543, 29602, 29662, 29720, 29778, 29836, 29893, 29950,
		30006, 30061, 30117, 30171, 30225, 30279, 30332, 30384, 30436, 30488, 30538, 30589, 30639, 30688, 30737, 30785, 30832, 30880, 30926, 30972, 31018, 31063, 31107, 31151, 31194, 31237, 31279, 31320, 31361, 31402, 31442, 31481,
		31520, 31558, 31596, 31633, 31669, 31705, 31740, 31775, 31809, 31843, 31876, 31908, 31940, 31971, 32002, 32032, 32062, 32090, 32119, 32146, 32174, 32200, 32226, 32251, 32276, 32300, 32324, 32347, 32369, 32391, 32412, 32432,
		32452, 32472, 32490, 32508, 32526, 32543, 32559, 32575, 32590, 32604, 32618, 32631, 32644, 32656, 32667, 32678, 32688, 32698, 32707, 32715, 32723, 32730, 32736, 32742, 32747, 32752, 32756, 32759, 32762, 32764, 32766, 32767
	}
};
//--------------------------------------

struct _NPink {
	//INT32           QLen;   <== #define NPINK_QLEN   (16)
	INT32    TIdx;      // 0 ~ (NPINK_QLEN-1)
	INT32    PFrm_Cnt;
	INT32    Min_Update_Interval;
	INT32    Max_Update_Interval;

	//VOID *   MemBase;
	UINT16 *Spec[NPINK_QLEN];    // [pCfg->SPEC_LEN]; // Qu16 // pQu->pQabs_blur -> PeakTrim()
	UINT16 *CB[NPINK_QLEN];      // [pCfg->CB_Len];   // Qu16
	UINT32   CBWC[NPINK_QLEN];        // Qu6.4
	UINT32   CBEng[NPINK_QLEN];       // Qu22

	UINT16   Time[NPINK_QLEN];
	UINT16 *Spec_Target;        // [pCfg->SPEC_LEN]; // Qu16
	UINT16 *Spec_Target_CB;     // [pCfg->CB_Len];   // Qu16
	UINT32   Spec_Target_CBWC;
	UINT32   Spec_Target_CBEng;

	UINT32   MaxV;   // Qu6.4
	UINT32   MaxI;
	UINT32   MinV;   // Qu6.4
	UINT32   MinI;

	UINT32   ChooseCnt;
	CHAR     Choose[NPINK_QLEN];
};
struct _NPink_LR {
	struct _NPink *pNPink_L;
	struct _NPink *pNPink_R;
};
struct _NPink_LR NPink_Stream[MAX_STREAM];
struct _NPink *pNPink;
//--------------------------------------

struct _NColor {
	//INT32   QLen;   <== #define NCOLOR_QLEN   (32)
	INT32     TIdx;     // 0 ~ (NCOLOR_QLEN-1)
	INT32     CFrm_Cnt;
	INT32     Min_Update_Interval;
	INT32     Max_Update_Interval;

	//VOID *    MemBase;
	UINT16   *Spec[NCOLOR_QLEN];
	UINT16   *CB[NCOLOR_QLEN];
	UINT32    CBWC[NCOLOR_QLEN];        // Qu6.4
	UINT32    CBEng[NCOLOR_QLEN];        // Qu22

	UINT16    Time[NCOLOR_QLEN];
	UINT16    NoiseTag[NCOLOR_QLEN];   // 0:Signal, 1:Noise
	UINT16    GroupNum;
	UINT16    GroupId[NCOLOR_QLEN];    // 內容值0為強制偵測點, [1~NCOLOR_QLEN]為噪音種類
	UINT16    GroupCnt[NCOLOR_QLEN + 1]; // 為了與 Matlab 對應, 只定義 [1 ~ NCOLOR_QLEN], [0]不使用

	UINT16   *Spec_Target;           // [pCfg->SPEC_LEN];  // Qu16
	UINT16   *Spec_Target_CB;        // [pCfg->CB_Len];  // Qu16
	UINT32    Spec_Target_CBWC;
	UINT32    Spec_Target_CBEng;

	UINT16   *Tmp_Spec;              // [pCfg->SPEC_LEN];     // pQu->pQabs_blur - pNPink->Spec[]
	UINT16   *Tmp_Spec_CB;           // [pCfg->CB_Len];
	UINT32    Tmp_Spec_CBWC;
	UINT32    Tmp_Spec_CBEng;
};
struct _NColor_LR {
	struct _NColor *pNColor_L;
	struct _NColor *pNColor_R;
};
struct _NColor_LR NColor_Stream[MAX_STREAM];
struct _NColor *pNColor;

//--------------------------------------

struct _NTone {
	INT32       ToneMinFrmCnt;
	UINT16     *Spec_Target;   // [pCfg->SPEC_LEN];        // Qu16
};
struct _NTone_LR {
	struct _NTone *pNTone_L;
	struct _NTone *pNTone_R;
};
struct _NTone_LR NTone_Stream[MAX_STREAM];
struct _NTone *pNTone;

//--------------------------------------

struct _BandEng {
	INT32 AbsSum;  // abs blur 的區段頻譜和
	INT32 BGSum;   // BGboost 的區段頻譜和
};

struct _Link {
	struct _BandEng *pCLink_O[MAX_Q_LEN];    // 0~3 不使用, 只用 4~9
	struct _BandEng *pRLink_O[MAX_Q_LEN];      // 0~3 不使用, 只用 4~9

	INT16    *pCLink_S[MAX_Q_LEN];   // 0~3 不使用, 只用 4~9
	INT16    *pCLink_E[MAX_Q_LEN];   // 0~3 不使用, 只用 4~9
	INT16       *pRLink_S[MAX_Q_LEN];     // 0~3 不使用, 只用 4~9
	INT16       *pRLink_E[MAX_Q_LEN];     // 0~3 不使用, 只用 4~9

	INT16    *pQMark[MAX_Q_LEN];   // Qu16, 0~3 不使用, 只用 4~9

	struct _BandEng *pTmp1_O;
	struct _BandEng *pTmp2_O;

	INT16   *pTmp1_S;     // RowLink() 使用
	INT16   *pTmp1_E;
	INT16   *pTmp2_S;
	INT16   *pTmp2_E;

	INT16    Link_Segs[MAX_Q_LEN];
};
struct _Link_LR {
	struct _Link *pLink_L;
	struct _Link *pLink_R;
};
struct _Link_LR Link_Stream[MAX_STREAM];
struct _Link *pLink;

//--------------------------------------

struct _Qu {
	INT32      QIdx;           // 0~pCfg->Q_Len

	//VOID * pQ_base;

	struct kfft_data *pQFFT[MAX_Q_LEN];    // 只用到 [pCfg->Q_Center]~[pCfg->Q_Len-1]
	UINT32     *pQabs_blur_tmp[MAX_Q_LEN];  // Qu18
	UINT16     *pQabs[MAX_Q_LEN];
	UINT16     *pQabs_blur[MAX_Q_LEN];      // Qu16
	UINT16     *pQCB[MAX_Q_LEN];      // calloc(pCfg->Q_Len * pCfg->CB_Len, sizeof(INT16))
};
struct _Qu_LR {
	struct _Qu *pQu_L;
	struct _Qu *pQu_R;
};
struct _Qu_LR Qu_Stream[MAX_STREAM];
struct _Qu *pQu;

// ------------------------

//extern struct Data_File tPCM[32];

#if 0
//--------------------------------------------------------------------------
//  DumpLink()
//--------------------------------------------------------------------------
VOID DumpLink(INT16 *p1, INT16 *p2, struct _BandEng *pi)
{
	INT32 i, Limit = pCfg->Link_Nodes;

	for (i = 0; i < Limit; i++) {
		if (p2[i] != -1) {
			printf("\n %3d( %3d -> %3d : %d, %d)", i, p1[i], p2[i], pi[i].AbsSum, pi[i].BGSum);
		} else {
			break;
		}
	}

	return;
}
#endif


#if 1 // IVOT-IM SYNC

#ifndef __KERNEL__
extern signed long uart_putString(CHAR *pString);
#define printf uart_putString
#else
#define printf printk
#endif


int Get_FFT_SizeExp(int sampling_rate);
int Get_CB_Size(int sampling_rate);
VOID Windowing(struct kfft_data *pIn);
VOID PolarCoordinate(struct kfft_data *pIn,
					 UINT16 *pAbs,
					 INT32    SStart,
					 INT32    SEnd);
VOID RollingPtr(UINT32 *ptr, INT32 S, INT32 E);
VOID Shift_Array(VOID *ptr, INT32 S, INT32 E, INT32 Bytes);
VOID PushInQueueAndBlur(UINT16 *pAbs,
						INT32 SStart,
						INT32    SEnd);
UINT32 Unbalance(
	UINT16 *pAbs[],
	INT32 pGSum[][MAX_Q_LEN],
	INT32 SStart, INT32 SEnd);
VOID CB_Filter(UINT16 *pSrc,
			   UINT16 *pDst,
			   INT32 sampling_rate,
			   INT32 CB_Max,           // bands
			   INT32 Consequency,      // 0:OPTION_CB_SEPARATED, 1:OPTION_CB_CONSEQUENCE
			   INT32 CB_Method);        // 1:OPTION_CB_MEAN, 2:OPTION_CB_MAX, 3:OPTION_CB_MIN
VOID CB_Eng_Fun(UINT16 *pQAbs[],     // Qu16
				UINT16 *pQCB[],                  // Qu16
				UINT32 *pEngG);
INT16 RowLink(INT32 idx_1,
			  char CR1,
			  char CR2);
VOID PeakTrim(UINT16 *pSpec, UINT16 *pCB, UINT16 *pDst, INT32 CutOffBin);
UINT32 EngSum(UINT16 *ps, INT32 Cnt);
UINT32 WeightCenter(UINT16 *ps, INT32 Cnt);
INT32 Value_Like(UINT32 val1, UINT32 val2,
				 UINT32 Mul,    /* Qu2.8 */
				 UINT32 Gap);
VOID ConditionMax(UINT32 pCBV[]);
VOID ConditionMin(UINT32 pCBV[]);
INT32 LargerCnt(UINT32 pCBV[]);
INT32 SmallerCnt(UINT32 pCBV[]);
INT32 CheckHistLike(VOID);
UINT16 GetNewNoiseGroupId(VOID);
INT32 NoiseCheck(VOID);
VOID Temporal_Estimation(VOID);
VOID ColLink(UINT16 *pAbs,
			 UINT16 *pBGboost,
			 INT16 *pCLink_s,
			 INT16 *pCLink_e,
			 struct _BandEng              *pCLink_o);
VOID FitMulCurve(UINT32 *pBG_Spec,
				 UINT16 *pBGboost,
				 UINT32 *pSig_MCurve);
VOID FindRLink(UINT32 First_SS);
VOID MaskSmoothing(INT16 *pMark, INT16 *pMark_Smooth);
VOID Spectrum_Suppress(VOID);
struct kfft_data *GetFFToutSlot(void);
VOID _ANR_process(VOID);
VOID CalculateHHLL(VOID);
VOID CreateMulCurve(UINT32 *pCurve,
					INT32 M1,      /* Qu26.6 */
					INT32 M2,      /* Qu26.6 */
					INT32 CExp,    /* 1~5 */
					INT32 Idx1,    /* 0~125 */
					INT32 Idx3);
INT32 StatisticDefault(UINT32 bias, // Q26.6
					   UINT16 *pSpec,                  // Qu16
					   UINT32 EngG);
VOID Setup_BG_Vars(VOID);
VOID Setup_Inner_Vars(VOID);
INT32 _ANR_detect(struct ANR_CONFIG *ptANR);
void Mem_Dispatch(int inkey, struct ANR_CONFIG *pANR);
void Mem_Structure(int key, struct _Cfg *pCfgTmp, int Opt);
void Mem_kfft(int key, struct _Cfg *pCfgTmp, int Opt);
void Mem_Var(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
void Mem_NPink(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
void Mem_NColor(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
void Mem_Tone(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
void Mem_Qu(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
void Mem_Link(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
void Mem_QMark(int key, struct _Cfg *pCfgTmp, int Opt, int Channel);
int GetMinUpdateInterval(int SR);
int GetMaxUpdateInterval(int SR);
void _ANR_init(int inkey, struct ANR_CONFIG *ptANR, int Channel);
INT32 AUD_ANR_Reset(VOID);
#endif

//--------------------------------------------------------------------------
//   Get_FFT_SizeExp(): return 2's exponential of FFT Size
//--------------------------------------------------------------------------
int Get_FFT_SizeExp(int sampling_rate)
{
	int Exp = 8;//[fixed coverity]

	// Matlab
	// FFT_Table  = [8000, 11025, 16000, 22050, 32000, 44100, 48000 ]      % 取樣率
	//                256,   256,   512,   512,  1024,  1024,  1024,       % FFT Size
	//                2^8,   2^8,   2^9,   2^9,  2^10,  2^10,  2^10        % FFT Size 2'Exp

	switch (sampling_rate) {
	case 8000:
	case 11025:
		Exp = 8;
		break;
	case 16000:
	case 22050:
		Exp = 9;
		break;
	case 32000:
	case 44100:
	case 48000:
		Exp = 10;
		break;
	default:
		err_code = ErrCode_103;
		break;
	}
	return Exp;
}

//--------------------------------------------------------------------------
//   Get_CB_Size(): return CB Size according to CB-Gap on 35 CBs at 8kHz sampling rate.
//--------------------------------------------------------------------------
int Get_CB_Size(int sampling_rate)
{
	int CBs = 0;

	// Matlab
	//    SR_All = [8000, 11025, 16000, 22050, 32000, 44100, 48000];   % 取樣率
	//    MaxFreq = SR_All./2;                                         % 最大頻率
	//    Mel8kGap = (2596*log10(1 + 4000/700))/35;                    % 8kHz 取樣時的 Mel gap (因為Mel是等距的)
	//    CBNum = ceil((2596.*log10(1+MaxFreq./700))./Mel8kGap);       % 根據 8kHz 時的 Mel gap 計算其他頻率的 CB 個數

	// Sampling Rate  = [8000, 11025, 16000, 22050, 32000, 44100, 48000 ]      % 取樣率
	//                     35,    41,    47,    52,    59,    64,    66        % CB Size

	switch (sampling_rate) {
	case 8000:
		CBs = 35;
		break;
	case 11025:
		CBs = 41;
		break;
	case 16000:
		CBs = 47;
		break;
	case 22050:
		CBs = 52;
		break;
	case 32000:
		CBs = 59;
		break;
	case 44100:
		CBs = 64;
		break;
	case 48000:
		CBs = 66;
		break;
	default:
		err_code = ErrCode_104;
		break;
	}
	return CBs;
}


//--------------------------------------------------------------------------
//  Windowing(): Apply 'Periodic' Hanning Window for PCM.
//
//  x = x*hanning
//--------------------------------------------------------------------------
VOID Windowing(struct kfft_data *pIn)
{
	INT32 i, j, k, Step;
	INT32 bShift;

	switch (pCfg->sampling_rate) {
	case 8000:
	case 11025:
		Step = 4;
		break;
	case 16000:
	case 22050:
		Step = 2;
		break;
	case 32000:
	case 44100:
	case 48000:
	default:
		Step = 1;
		break;
	}

	bShift = 15 - pCfg->FFT_SZ_Exp;   // 為了符合 fft_2 的特性,輸入資料乘上FFT_SZ => ifft 輸出資料就會正常
	pIn[0].r = 0;
	for (i = 1, j = pCfg->FFT_SZ - 1, k = Step; i < ((pCfg->FFT_SZ) >> 1); i++, j--, k += Step) {
		pIn[i].r = (Tbl.fft_window[k] * pIn[i].r + (1 << (bShift - 1))) >> bShift; // Q15*Q15 = Q30>>(15-pCfg->FFT_SZ_Exp) = Q15<<pCfg->FFT_SZ_Exp
		pIn[j].r = (Tbl.fft_window[k] * pIn[j].r + (1 << (bShift - 1))) >> bShift; // Q15*Q15 = Q30>>(15-pCfg->FFT_SZ_Exp) = Q15<<pCfg->FFT_SZ_Exp
	}
	pIn[i].r = pIn[i].r << pCfg->FFT_SZ_Exp;

	return;
}

//--------------------------------------------------------------------------
//  PolarCoordinate()
//--------------------------------------------------------------------------
VOID PolarCoordinate(struct kfft_data *pIn,
					 UINT16 *pAbs,
					 INT32    SStart,
					 INT32    SEnd)
{
	INT32 i, rVal, iVal;

	for (i = 0; i < SStart; i++) {
		pAbs[i] = 0;
		//pAng[i] = 0;
	}

	for (i = SStart; i <= SEnd; i++) {
		int Hf = 1 << (pCfg->FFT_SZ_Exp - 1); // for KISS FFT Q scale

		rVal = (pIn[i].r + Hf) >> pCfg->FFT_SZ_Exp;    // (Q15 + pCfg->FFT_SZ_Exp) >> pCfg->FFT_SZ_Exp
		iVal = (pIn[i].i + Hf) >> pCfg->FFT_SZ_Exp;
		//rVal = pIn[i].r;
		//iVal = pIn[i].i;

		pAbs[i] = fxpt_sqrt((UINT32)(rVal * rVal + iVal * iVal));   // sqrt(Q15*Q15 + Q15*Q15) = sqrt(Qu31) = Qu16

		//pAng[i] = fxpt_atan2(iVal, rVal);    // fxpt_atan2(y(虛部), x(實部))
	}
	for (; i < pCfg->SPEC_LEN; i++) { // 129~255
		pAbs[i] = 0;
		//pAng[i] = 0;
	}

	return;
}

/*
//--------------------------------------------------------------------------
//  CartesianCoordinate()
//--------------------------------------------------------------------------
VOID CartesianCoordinate(
    UINT16 * pAbs,  // 0:DC, 1~127:Data, 128:4kHz
    INT16 * pAng,
    INT16 * pReal,
    INT16 * pImag,
    INT32        SStart,
    INT32        SEnd)
{
    INT32 i,j;

    for (i=0;i<SStart;i++)
    {
        pReal[i] = 0;
        pImag[i] = 0;
    }

    for (i=SStart;i<=SEnd;i++)
    {
        pReal[i] = (INT16)(((INT32)pAbs[i] * fxpt_cos(pAng[i])) >> 15);
        pImag[i] = (INT16)(((INT32)pAbs[i] * fxpt_sin(pAng[i])) >> 15);
    }

    for (i=pCfg->FFT_OV_SZ+1,j=pCfg->FFT_OV_SZ-1;i<pCfg->FFT_SZ;i++,j--)
    {
        pReal[i] = pReal[j];
        pImag[i] = -pImag[j];
    }

    return;
}
*/

//--------------------------------------------------------------------------
//  RollingPtr()
//--------------------------------------------------------------------------
VOID RollingPtr(UINT32 *ptr, INT32 S, INT32 E)
{
	INT32 i;
	UINT32 tmp;

	tmp = ptr[S];
	for (i = S; i < E; i++) {
		// [(S+1)~(E)] move to [(S)~(E-1)]
		ptr[i] = ptr[i + 1];
	}
	ptr[i] = tmp;

	return;
}


//--------------------------------------------------------------------------
//  Shift_Array()
//  Parameters : 1. ptr   : any pointer (change to VOID *)
//               2. S     : first index to shift array
//               3. E     : last index to shift array
//               4. Bytes : unit of shifting
//--------------------------------------------------------------------------
VOID Shift_Array(VOID *ptr, INT32 S, INT32 E, INT32 Bytes)
{
	INT32 i;
	CHAR   *ptr1;
	INT16 *ptr2;
	INT32    *ptr4;

	switch (Bytes) {
	case 1:
		ptr1 = (CHAR *)ptr;
		for (i = S; i < E; i++) {
			ptr1[i] = ptr1[i + 1];      // [1~9] move to [0~8]
		}
		ptr1[i] = 0;
		break;
	case 2:
		ptr2 = (INT16 *)ptr;
		for (i = S; i < E; i++) {
			ptr2[i] = ptr2[i + 1];      // [1~9] move to [0~8]
		}
		ptr2[i] = 0;
		break;
	case 4:
		ptr4 = (INT32 *)ptr;
		for (i = S; i < E; i++) {
			ptr4[i] = ptr4[i + 1];      // [1~9] move to [0~8]
		}
		ptr4[i] = 0;
		break;
	default:
		printf("\n Error in Shift_Array!!!");
		break;
	}

	return;
}

//--------------------------------------------------------------------------
//  PushInQueueAndBlur() : push [pVar->asNRabs] into [pQu->pQabs]
//                    push blurred into [pQu->pQabs_blur_tmp],[pQu->pQabs_blur]
//
//--------------------------------------------------------------------------
VOID PushInQueueAndBlur(UINT16 *pAbs,
						INT32 SStart,
						INT32    SEnd)
{
	INT32 i, QIdx_1;
	UINT16 *dst_u1;
	UINT32 *src_i1, * src_i2, * src_i3, * dst_i1;

	if (pQu->QIdx == pCfg->Q_Len) {
		// pQu->QIdx == 10

		// time rolling
		RollingPtr((UINT32 *)pQu->pQabs, 0, pCfg->Q_Len - 1);
		RollingPtr((UINT32 *)pQu->pQabs_blur_tmp, 0, pCfg->Q_Len - 1);
		RollingPtr((UINT32 *)pQu->pQabs_blur, 0, pCfg->Q_Len - 2);  // only rotate 9 elements

		// Copy Abs & Angle data into last buffer
		dst_u1 = pQu->pQabs[pCfg->Q_Len - 1];
		dst_i1 = pQu->pQabs_blur_tmp[pCfg->Q_Len - 1]; // 先計算頻譜(垂直)方向的 [1 2 1]/4 filter

		// blur2 in Spectrum
		dst_u1[SStart] = pAbs[SStart];    // copy (SStart) element into Queue
		dst_i1[SStart] = ((UINT32)pAbs[SStart] + (UINT32)pAbs[SStart + 1]) << 1; // 上邊界必須乘兩倍，[2; 2; ...]
		for (i = SStart + 1; i <= SEnd - 1; i++) { // copy (SStart+1)~(SEnd-1) element into Queue
			dst_u1[i] = pAbs[i];
			dst_i1[i] = (UINT32)pAbs[i - 1] + ((UINT32)pAbs[i] << 1) + (UINT32)pAbs[i + 1]; // blur2 in Spectrum
		}
		dst_u1[i] = pAbs[i];    // copy (SEnd) element into Queue
		dst_i1[i] = ((UINT32)pAbs[i - 1] + (UINT32)pAbs[i]) << 1; // 下邊界必須乘兩倍，[...; 2; 2;]

		// blur2 in Temporal (水平)方向
		src_i1    = pQu->pQabs_blur_tmp[pCfg->Q_Len - 3];
		src_i2    = pQu->pQabs_blur_tmp[pCfg->Q_Len - 2];
		src_i3    = pQu->pQabs_blur_tmp[pCfg->Q_Len - 1];
		dst_u1    = pQu->pQabs_blur[pCfg->Q_Len - 2];
		for (i = SStart; i < SEnd; i++) {
			dst_u1[i] = (UINT16)((src_i1[i] + (src_i2[i] << 1) + src_i3[i] + 8) >> 4);
		}
	} else {
		// pQu->QIdx = 0~9, pQu->QIdx 預設為零
		dst_u1 = pQu->pQabs[pQu->QIdx];

		dst_i1 = pQu->pQabs_blur_tmp[pQu->QIdx];  // 先計算頻譜方向的 [1 2 1]/4 filter

		dst_u1[SStart] = pAbs[SStart];    // copy (SStart) element into Queue
		dst_i1[SStart] = ((UINT32)pAbs[SStart] + (UINT32)pAbs[SStart + 1]) << 1; // blur2 in Spectrum
		for (i = SStart + 1; i < SEnd; i++) {
			// copy (SStart)~(SEnd) element into Queue
			dst_u1[i] = pAbs[i];
			dst_i1[i] = (UINT32)pAbs[i - 1] + ((UINT32)pAbs[i] << 1) + (UINT32)pAbs[i + 1]; // blur2 in Spectrum
		}
		dst_u1[i] = pAbs[i];    // copy (SEnd) element into Queue
		dst_i1[i] = ((UINT32)pAbs[i - 1] + (UINT32)pAbs[i]) << 1; // blur2 in Spectrum

		if (pQu->QIdx >= 2) { //2~9
			// blur2 in Temporal
			QIdx_1 = pQu->QIdx - 1;

			src_i1    = pQu->pQabs_blur_tmp[QIdx_1 - 1];
			src_i2    = pQu->pQabs_blur_tmp[QIdx_1];
			src_i3    = pQu->pQabs_blur_tmp[QIdx_1 + 1];
			dst_u1    = pQu->pQabs_blur[QIdx_1];       // 計算時間方向的 [1 2 1]/4 filter
			for (i = SStart; i <= SEnd; i++) {
				dst_u1[i] = (UINT16)((src_i1[i] + ((INT32)src_i2[i] << 1) + src_i3[i] + 8) >> 4);
			}
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  Unbalance()
//    Description : return
//--------------------------------------------------------------------------
UINT32 Unbalance(
	UINT16 *pAbs[],
	INT32 pGSum[][MAX_Q_LEN],
	INT32 SStart, INT32 SEnd)
{
	INT32 i, j, k, Idx;
	UINT16 *pS1, * pS2;
	INT32 sum1, sum2, sum3, sum4;
	UINT32 max1, max2, P, Q;
	INT32 Out_L[4], Out_R[4];

	if ((pQu->QIdx >= 3) && (pQu->QIdx <= pCfg->Q_Len - 1)) { // pQu->QIdx = 3~9 => only find GSum of 1~7 to match GSum_blur
		Idx = pQu->QIdx - 2;                // Idx = 1~7
	} else if (pQu->QIdx == pCfg->Q_Len) {    // pQu->QIdx = 10
		Idx = pCfg->Q_Len - 3;          // Idx = 7
		for (k = 0; k < 4; k++) {
			for (i = 1; i < pCfg->Q_Len - 3; i++) { // move 2~7 to 1~6, pGSum has data only in [1~7], [0,8,9] are empty
				pGSum[k][i]        = pGSum[k][i + 1];
			}
		}
	} else { // pQu->QIdx==0,1,2
		return pVar->default_bias;    // pQu->QIdx = [0,1,2]
	}

	// pQu->QIdx = [3~9,10]
	// Idx  = [1~7,7]
	sum1 = 0;
	sum2 = 0;
	sum3 = 0;
	sum4 = 0;

	pS1 = pAbs[Idx];        // 1st column [1~7], Qu16
	pS2 = pAbs[Idx + 1];    // 2nd column [2~8], Qu16

	for (i = SStart; i <= SEnd - 1; i++) { // loop 小於 2^[7~9] 次
		// pS1[i]    pS1[j]    <--- 頻譜一
		// pS2[i]    pS2[j]    <--- 頻譜二
		j = i + 1;

		// Matlab : G1 = abs(In(1:h-1,1:w-1) - In(2:h,1:w-1)); (上下差)
		if (pS1[i] > pS1[j]) {
			sum1 += (pS1[i] - pS1[j]);    // 最大值 2^([7~9]+16)
		} else {
			sum1 += (pS1[j] - pS1[i]);
		}

		// Matlab : G2 = abs(In(1:h-1,1:w-1) - In(1:h-1,2:w)); (左右差)
		if (pS1[i] > pS2[i]) {
			sum2 += (pS1[i] - pS2[i]);    // 最大值 2^([7~9]+16)
		} else {
			sum2 += (pS2[i] - pS1[i]);
		}

		// Matlab : G3 = abs(In(1:h-1,1:w-1) - In(2:h,2:w   )); (對角差)
		if (pS1[i] > pS2[j]) {
			sum3 += (pS1[i] - pS2[j]);    // 最大值 2^([7~9]+16)
		} else {
			sum3 += (pS2[j] - pS1[i]);
		}

		// Matlab : G4 = abs(In(1:h-1,2:w   ) - In(2:h,1:w-1)); (反對角差)
		if (pS2[i] > pS1[j]) {
			sum4 += (pS2[i] - pS1[j]);    // 最大值 2^([7~9]+16)
		} else {
			sum4 += (pS1[j] - pS2[i]);
		}
	}
	pGSum[0][Idx] = sum1;    // Qu23~Qu25
	pGSum[1][Idx] = sum2;
	pGSum[2][Idx] = sum3;
	pGSum[3][Idx] = sum4;

	if (Idx == pCfg->Q_Len - 3) {  // idx==7,此時GSum資訊足夠來計算Bias
		for (i = 0; i < 4; i++) { // 四個方向
			Out_L[i] = 0;
			Out_R[i] = 0;
			for (k = 1, j = pCfg->Q_Center; k <= pCfg->Q_Center; k++, j++) {
				Out_L[i] += pGSum[i][k];
				Out_R[i] += pGSum[i][j];
			}
			//Out_L[i] = pGSum[i][1] + pGSum[i][2] + pGSum[i][3] + pGSum[i][4];    // Qu25~Qu27, min 1 to prevent 0 division
			Out_L[i] = Out_L[i] == 0 ? 1 : Out_L[i];
			//Out_R[i] = pGSum[i][4] + pGSum[i][5] + pGSum[i][6] + pGSum[i][7];    // Qu25~Qu27, min 1 to prevent 0 division
			Out_R[i] = Out_R[i] == 0 ? 1 : Out_R[i];
		}

		// Matlab : P = max([Out(1,1) / Out(2,1); Out(2,1) / Out(1,1); Out(1,2) / Out(2,2); Out(2,2) / Out(1,2)]);
		if (Out_L[0] > Out_L[1]) {
			max1 = (Out_L[0] << 6) / Out_L[1];    // Qu27.6
		} else {
			max1 = (Out_L[1] << 6) / Out_L[0];    // Qu27.6
		}

		if (Out_R[0] > Out_R[1]) {
			max2 = (Out_R[0] << 6) / Out_R[1];    // Qu27.6
		} else {
			max2 = (Out_R[1] << 6) / Out_R[0];    // Qu27.6
		}
		P = max1 > max2 ? max1 : max2;        // Qu27.6


		// Matlab : Q = max([Out(3,1) / Out(4,1); Out(4,1) / Out(3,1); Out(3,2) / Out(4,2); Out(4,2) / Out(3,2)]);
		if (Out_L[2] > Out_L[3]) {
			max1 = (Out_L[2] << 6) / Out_L[3];    // Qu27.6
		} else {
			max1 = (Out_L[3] << 6) / Out_L[2];    // Qu27.6
		}

		if (Out_R[2] > Out_R[3]) {
			max2 = (Out_R[2] << 6) / Out_R[3];    // Qu27.6
		} else {
			max2 = (Out_R[3] << 6) / Out_R[2];    // Qu27.6
		}
		Q = max1 > max2 ? max1 : max2;        // Qu27.6

		return ((P + Q - (1 << 6)) & 0x0000ffff); // Qu10.6
	} else { // pQu->QIdx=0~8, 此時還沒有足夠的資訊得到 Bias 所以回傳 pVar->default_bias
		return pVar->default_bias;
	}
}

//--------------------------------------------------------------------------
//  CB_Filter()
//    Description : Critical Band filter
//                  離散型的時候會輸出 pDst[0~CB_Max-1]
//                  連續型的時候會輸出 pDst[1~N-1], pDst[0] 會被清為零
//    Ex. CB_Filter(pIn, pOut, pCfg->sampling_rate, 18, OPTION_CB_SEPARATED, OPTION_CB_MEAN)
//        CB_Filter(pIn, pOut, pCfg->sampling_rate, 18, OPTION_CB_CONSEQUENCE, OPTION_CB_MEAN)
//--------------------------------------------------------------------------
VOID CB_Filter(UINT16 *pSrc,
			   UINT16 *pDst,
			   INT32 sampling_rate,
			   INT32 CB_Max,           // bands
			   INT32 Consequency,      // 0:OPTION_CB_SEPARATED, 1:OPTION_CB_CONSEQUENCE
			   INT32 CB_Method)        // 1:OPTION_CB_MEAN, 2:OPTION_CB_MAX, 3:OPTION_CB_MIN
{
	INT32 k, m, sum, max, min;
	INT32 LWing, RWing;
	INT32 S1, E1, S2, E2;

	// 連續型 或 離散型 的 CB 濾波類型
	if (Consequency == OPTION_CB_CONSEQUENCE) {
		if (CB_Method == OPTION_CB_MEAN) {
			// 1.CB Mean (連續型)
			pDst[0] = 0; // 因為 CB 範圍定義從1開始
			S1 = -1;
			E1 = -1;
			sum = 0;

			for (k = 0; k < CB_Max; k++) {
				LWing = (pCfg->CB_Num[k] - 1) >> 1;
				RWing = pCfg->CB_Num[k] - LWing - 1;
				for (m = pCfg->CB_Start[k]; m <= pCfg->CB_End[k]; m++) {
					S2 = m - LWing;
					S2 = S2 < pCfg->Valid_SIdx_Low ? pCfg->Valid_SIdx_Low : S2;
					E2 = m + RWing;
					E2 = E2 > pCfg->Valid_SIdx_High ? pCfg->Valid_SIdx_High : E2;

					while (E2 > E1) {
						E1++;
						sum += pSrc[E1];
					}
					while (S2 > S1) {
						if (S1 >= 0) {
							sum -= pSrc[S1];
						}
						S1++;
					}
					pDst[m] = (sum * Tbl.CB_Mul_2e11[E2 - S2] + (1 << 10)) >> 11; // E2-S2 比實際個數少一,剛好搭配 CB_Mul_2e11 的索引值少一的特性
				}
			}
		} else if (CB_Method == OPTION_CB_MAX) {
			// 2.CB Max (連續型)
			pDst[0] = 0; // 因為 CB 範圍定義從1開始

			for (k = 0; k < CB_Max; k++) {
				LWing = (pCfg->CB_Num[k] - 1) >> 1;
				RWing = pCfg->CB_Num[k] - LWing - 1;
				for (m = pCfg->CB_Start[k]; m <= pCfg->CB_End[k]; m++) {
					S2 = m - LWing;
					S2 = S2 < pCfg->Valid_SIdx_Low ? pCfg->Valid_SIdx_Low : S2;
					E2 = m + RWing;
					E2 = E2 > pCfg->Valid_SIdx_High ? pCfg->Valid_SIdx_High : E2;

					max = pSrc[S2++];

					while (E2 >= S2) {
						max = max > pSrc[S2] ? max : pSrc[S2];
						S2++;
					}
					pDst[m] = max;        // E2-S2 比實際個數少一,剛好搭配 CB_Mul_2e11 的索引值少一的特性
				}
			}
		} else if (CB_Method == OPTION_CB_MIN) {
			// 3.CB Min (連續型)
			pDst[0] = 0; // 因為 CB 範圍定義從1開始

			for (k = 0; k < CB_Max; k++) {
				LWing = (pCfg->CB_Num[k] - 1) >> 1;
				RWing = pCfg->CB_Num[k] - LWing - 1;
				for (m = pCfg->CB_Start[k]; m <= pCfg->CB_End[k]; m++) {
					S2 = m - LWing;
					S2 = S2 < pCfg->Valid_SIdx_Low ? pCfg->Valid_SIdx_Low : S2;
					E2 = m + RWing;
					E2 = E2 > pCfg->Valid_SIdx_High ? pCfg->Valid_SIdx_High : E2;

					min = pSrc[S2++];

					while (E2 >= S2) {
						min = min < pSrc[S2] ? min : pSrc[S2];
						S2++;
					}
					pDst[m] = min;        // E2-S2 比實際個數少一,剛好搭配 CB_Mul_2e11 的索引值少一的特性
				}
			}
		} else {
			//printf("Not support in CB_Filter()");
			err_code = ErrCode_112;
		}
	} else if (Consequency == OPTION_CB_SEPARATED) {
		if (CB_Method == OPTION_CB_MEAN) {
			// 1.CB Mean (離散型)
			for (k = 0; k < CB_Max; k++) {
				S1 = pCfg->CB_Start[k];
				E1 = pCfg->CB_End[k];
				sum = 0;
				for (m = S1; m <= E1; m++) {
					sum += pSrc[m];
				}
				pDst[k] = (sum * Tbl.CB_Mul_2e11[E1 - S1] + (1 << 10)) >> 11;
			}
		} else if (CB_Method == OPTION_CB_MAX) {
			// 2.CB Max (離散型)
			for (k = 0; k < CB_Max; k++) {
				S1 = pCfg->CB_Start[k];
				E1 = pCfg->CB_End[k];

				max = pSrc[S1++];
				for (m = S1; m <= E1; m++) {
					max = max > pSrc[m] ? max : pSrc[m];
				}
				pDst[k] = max;
			}
		} else if (CB_Method == OPTION_CB_MIN) {
			// 3.CB Min (離散型)
			for (k = 0; k < CB_Max; k++) {
				S1 = pCfg->CB_Start[k];
				E1 = pCfg->CB_End[k];

				min = pSrc[S1++];
				for (m = S1; m <= E1; m++) {
					min = min < pSrc[m] ? min : pSrc[m];
				}
				pDst[k] = min;
			}
		} else {
			//printf("Option is not support in CB_Filter()");
			err_code = ErrCode_112;
		}
	} else {
		//printf("Consequency in CB_Filter() is not support!!!");
		err_code = ErrCode_113;
	}

	return;
}


//--------------------------------------------------------------------------
//  CB_Eng_Fun()
//
//    Description : calculate pQu->pQCB[] & pVar->QEngG[] from pQu->pQabs_blur
//            pQu->QIdx==0,1時,將 pVar->default_spec 和 pVar->default_eng 寫入 QSpec[0] 和 pVar->QEngG[0]
//
//    Caller : CB_Eng_Fun(pQu->pQabs_blur, pQu->pQCB, pVar->QEngG);
//--------------------------------------------------------------------------
VOID CB_Eng_Fun(UINT16 *pQAbs[],     // Qu16
				UINT16 *pQCB[],                  // Qu16
				UINT32 *pEngG)                   // Qu23
{
	INT32 k, sum, idx;
	UINT16 *pSrc, * pDst;

	if (pQu->QIdx == pCfg->Q_Len) {
		RollingPtr((UINT32 *)pQu->pQCB, 0, pCfg->Q_Len - 1);
		Shift_Array((VOID *)pEngG, 0, pCfg->Q_Len - 1, 4);

		CB_Filter(pQAbs[pCfg->Q_Len - 2], pQu->pQCB[pCfg->Q_Len - 2], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);

		sum = 0;
		pSrc = pQAbs[pCfg->Q_Len - 2];
		for (k = pCfg->Valid_SIdx_Low + 1; k <= pCfg->Valid_SIdx_High; k++) { // bypass fft bin 1, loop 2^7 次
			sum += pSrc[k];
		}
		pEngG[pCfg->Q_Len - 2] = sum;  // Qu(7+16)=Qu23

	} else if (pQu->QIdx >= 2) {
		idx = pQu->QIdx - 1;
		CB_Filter(pQAbs[idx], pQu->pQCB[idx], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);

		sum = 0;
		pSrc = pQAbs[idx];
		for (k = pCfg->Valid_SIdx_Low + 1; k <= pCfg->Valid_SIdx_High; k++) {
			sum += pSrc[k];
		}
		pEngG[idx] = sum;
	} else {
		// pQu->QIdx == 0, 1
		idx = 0;
		pDst = pQu->pQCB[idx];
		for (k = 0; k < pCfg->CB_Len; k++) {
			pDst[k] = pVar->Default_CB[k];      // CB_Eng_Fun(),pQu->pQCB[0,1]用Default_CB代替
		}

		pEngG[idx] = pVar->default_eng;

	}
	return;
}

//--------------------------------------------------------------------------
//  RowLink()
//--------------------------------------------------------------------------
INT16 RowLink(INT32 idx_1,
			  char CR1,
			  char CR2)
{
	INT16 *pLink1_1, * pLink1_2, * pLink2_1, * pLink2_2;
	struct _BandEng *pLink1_o, * pLink2_o;
	INT16     Len1, Len2, m, n, idx1, idx2, nPush, mPush, flag, flag1, flag2, s1, e1, sLimit, eLimit;
	INT16    idx_2 = idx_1 + 1;

	pLink1_1 = NULL;
	pLink1_2 = NULL;
	pLink1_o = NULL;
	pLink2_1 = NULL;
	pLink2_2 = NULL;
	pLink2_o = NULL;
	// --- 1st Block Segment ---
	if (CR1 == 'c') {
		pLink1_1 = pLink->pCLink_S[idx_1];
		pLink1_2 = pLink->pCLink_E[idx_1];
		pLink1_o = pLink->pCLink_O[idx_1];
	} else if (CR1 == 'r') {
		pLink1_1 = pLink->pRLink_S[idx_1];
		pLink1_2 = pLink->pRLink_E[idx_1];
		pLink1_o = pLink->pRLink_O[idx_1];
	} else {
		printf("\n Not defined in RowLink()");
		return 0;
	}

	// --- 2nd Block Segment ---
	if (CR2 == 'c') {
		pLink2_1 = pLink->pCLink_S[idx_2];
		pLink2_2 = pLink->pCLink_E[idx_2];
		pLink2_o = pLink->pCLink_O[idx_2];
	} else if (CR2 == 'r') {
		pLink2_1 = pLink->pRLink_S[idx_2];
		pLink2_2 = pLink->pRLink_E[idx_2];
		pLink2_o = pLink->pRLink_O[idx_2];
	} else {
		printf("\n Not defined in RowLink()");
		return 0;
	}

	// 取得 前 Bin Segment 的個數
	Len1 = 0;
	while (pLink1_2[Len1] >= 0) {
		Len1++;
	}

	// 取得 後 Bin Segment 的個數
	Len2 = 0;
	while (pLink2_2[Len2] >= 0) {
		Len2++;
	}
//printf("\n---");
//Dump_Data_UInt(pLink1_1, 2, Len1+1);
//Dump_Data_UInt(pLink1_2, 2, Len1+1);
//Dump_Data_UInt(pLink2_1, 2, Len2+1);
//Dump_Data_UInt(pLink2_2, 2, Len2+1);


	idx1 = 0;       // 輸出左索引值
	idx2 = 0;       // 輸出右索引值
	if ((Len1 > 0) && (Len2 > 0)) {
		m = 0;
		n = 0;
		nPush = -1;
		mPush = -1;


		while ((m < Len1) && (n < Len2)) {  // has segments
			// step 1 : skip non-link segments
			while ((pLink1_2[m] < (pLink2_1[n] - 1)) && (m < Len1)) {
				m = m + 1;
			}
			while ((pLink2_2[n] < (pLink1_1[m] - 1)) && (n < Len2)) {
				n = n + 1;
			}
			if ((m >= Len1) || (n >= Len2)) {
				break;
			}

			// step 2 : add Linked notes
			flag1 = 1;    // try to find link from pLink2 to pLink1
			flag2 = 1;    // try to find link from pLink1 to pLink2
			while (flag1 && flag2) {
				if (flag2) { // 往右找
					s1 = pLink1_1[m];    // get one data in seg 1
					sLimit = (s1 - 1) < pCfg->Valid_SIdx_Low ? pCfg->Valid_SIdx_Low : (s1 - 1);
					e1 = pLink1_2[m];
					eLimit = (e1 + 1) > pCfg->Valid_SIdx_High ? pCfg->Valid_SIdx_High : (e1 + 1);
					flag = 0;   // 假設找不到相連的block
					flag1 = 0;  // 假設不需要往左邊找相連
					while ((pLink2_1[n] <= eLimit) && (pLink2_2[n] >= sLimit)) {
						// seg 2 is connected to seg 1
						pLink->pTmp2_S[idx2] = pLink2_1[n];
						pLink->pTmp2_E[idx2] = pLink2_2[n];
						pLink->pTmp2_O[idx2].AbsSum = pLink2_o[n].AbsSum;
						pLink->pTmp2_O[idx2].BGSum = pLink2_o[n].BGSum;
						idx2++;
						nPush = n;  // 記錄最後一次取出的索引值
						n++;
						flag = 1;    // some data in seg 2 is connected
					}
					if (flag == 1) {  // if seg 2 is connected to seg 1
						n--;
						if (mPush != m) {
							// push data 1
							pLink->pTmp1_S[idx1] = pLink1_1[m];
							pLink->pTmp1_E[idx1] = pLink1_2[m];
							pLink->pTmp1_O[idx1].AbsSum = pLink1_o[m].AbsSum;
							pLink->pTmp1_O[idx1].BGSum = pLink1_o[m].BGSum;
							idx1++;
							mPush = m;
						}
						m++;
						flag1 = 1;  // 右邊有找到相連,所以繼續向左找
					}
				}
				if (flag1) {
					// some data 2 is connected => recheck the seg 1 if any connected
					s1 = pLink2_1[n];
					sLimit = (s1 - 1) < pCfg->Valid_SIdx_Low ? pCfg->Valid_SIdx_Low : (s1 - 1);
					e1 = pLink2_2[n];
					eLimit = (e1 + 1) > pCfg->Valid_SIdx_High ? pCfg->Valid_SIdx_High : (e1 + 1);
					flag = 0;   // 假設找不到左相連
					flag2 = 0;  // 假設不需要往右找

					while ((pLink1_1[m] <= eLimit) && (pLink1_2[m] >= sLimit)) {
						pLink->pTmp1_S[idx1] = pLink1_1[m];
						pLink->pTmp1_E[idx1] = pLink1_2[m];
						pLink->pTmp1_O[idx1].AbsSum = pLink1_o[m].AbsSum;
						pLink->pTmp1_O[idx1].BGSum = pLink1_o[m].BGSum;
						idx1++;
						mPush = m;
						m++;
						flag = 1;
					}
					if (flag == 1) {
						m--;
						if (nPush != n) {
							pLink->pTmp2_S[idx2] = pLink2_1[n];
							pLink->pTmp2_E[idx2] = pLink2_2[n];
							pLink->pTmp2_O[idx2].AbsSum = pLink2_o[n].AbsSum;
							pLink->pTmp2_O[idx2].BGSum = pLink2_o[n].BGSum;
							nPush = n;
							idx2++;
						}
						n++;
						flag2 = 1;
					}
				}
			}
		}
	}

	// 記錄 SBlock 長度(個數)
	pLink->Link_Segs[idx_1] = idx1;
	pLink->Link_Segs[idx_2] = idx2;

	// 拷貝 pTmp1_X --> pRLink_X
	pLink1_1 = pLink->pRLink_S[idx_1];
	pLink1_2 = pLink->pRLink_E[idx_1];
	pLink1_o = pLink->pRLink_O[idx_1];
	m = 0;
	while (m < idx1) {
		pLink1_1[m] = pLink->pTmp1_S[m];
		pLink1_2[m] = pLink->pTmp1_E[m];
		pLink1_o[m].AbsSum = pLink->pTmp1_O[m].AbsSum;
		pLink1_o[m].BGSum = pLink->pTmp1_O[m].BGSum;
		m++;
	}
	pLink1_1[m] = -1;
	pLink1_2[m] = -1;
	pLink1_o[m].AbsSum = -1;
	pLink1_o[m].BGSum = -1;
//printf("\n");
//DumpUInt16(pLink1_1,idx1+1,0);
//DumpUInt16(pLink1_2,idx1+1,0);

	pLink2_1 = pLink->pRLink_S[idx_2];
	pLink2_2 = pLink->pRLink_E[idx_2];
	pLink2_o = pLink->pRLink_O[idx_2];
	n = 0;
	while (n < idx2) {
		pLink2_1[n] = pLink->pTmp2_S[n];
		pLink2_2[n] = pLink->pTmp2_E[n];
		pLink2_o[n].AbsSum = pLink->pTmp2_O[n].AbsSum;
		pLink2_o[n].BGSum = pLink->pTmp2_O[n].BGSum;
		n++;
	}
	pLink2_1[n] = -1;
	pLink2_2[n] = -1;
	pLink2_o[n].AbsSum = -1;
	pLink2_o[n].BGSum = -1;
//DumpUInt16(pLink2_1,idx2+1,0);
//DumpUInt16(pLink2_2,idx2+1,0);

	return idx1;
}

//--------------------------------------------------------------------------
//  PeakTrim()
//  Description : 從低頻到高頻，找出第一個高點，再向高頻方向刪除突出的頻譜訊號
//  Return :
//--------------------------------------------------------------------------
VOID PeakTrim(UINT16 *pSpec, UINT16 *pCB, UINT16 *pDst, INT32 CutOffBin)
{
	INT32 i, S, k, CBBin;
	UINT16 kv;

	// 根據 FFT_Bin 頻率,找出相對應的 CBBin
	CBBin = 0;
	while (pCfg->CB_End[CBBin] < CutOffBin) {
		CBBin++;
	}

	// 找第一個 CB 下降點(或水平點)
	S = 0;
	while ((pCB[S + 1] > pCB[S]) && (S < CBBin)) {
		S++;
	}

	// 從該 CB 結束點往高頻切除
	k = pCfg->CB_End[S];
	for (i = 0; i <= k; i++) {
		pDst[i] = pSpec[i];
	}

	// 往高頻方向切除凸起的頻譜
	kv = pSpec[k];
	k = k + 1;
	while (k < pCfg->SPEC_LEN) {
		if (pSpec[k] > kv) {
			pDst[k] = kv;
		} else {
			pDst[k] = pSpec[k];
			kv = pSpec[k];
		}
		k++;
	}

	return;
}

//--------------------------------------------------------------------------
//  EngSum()
//--------------------------------------------------------------------------
UINT32 EngSum(UINT16 *ps, INT32 Cnt)
{
	INT32 i;
	UINT32 Sum;

	Sum = 0;
	for (i = 0; i < Cnt; i++) {
		Sum += ps[i];
	}

	return Sum;
}

//--------------------------------------------------------------------------
//  WeightCenter()
//--------------------------------------------------------------------------
UINT32 WeightCenter(UINT16 *ps, INT32 Cnt)
{
	INT32 i;
	UINT32 S, Sum, Ret;

	S = 0;
	Sum = 0;
	for (i = 0; i < Cnt; i++) {
		S   += ps[i];       // Qu22 <- sum(Qu16[1~64])
		Sum += ps[i] * (i + 1); // Qu28 <- sum(Qu16*[1~66])
	}
	if (S == 0) {
		Ret = 0;
	} else {
		Ret = ((Sum << 4) + (S >> 1)) / S; // Qu6.4 <- (Qu28<<4)/Qu22
	}

	return Ret;
}

//--------------------------------------------------------------------------
//  Value_Like()
//
//  Ret :   0 (Value is not alike)
//          1 (Value is     alike)
//
//  Caller Ex:  Value_Like(pNPink->CBWC(CIdx), pNPink->CBWC(TIdx), 1<<8, 2<<4)
//--------------------------------------------------------------------------
INT32 Value_Like(UINT32 val1, UINT32 val2,
				 UINT32 Mul,    /* Qu2.8 */
				 UINT32 Gap)
{
	INT32 val;

	if (val1 < val2) {
		val  = val1;
		val1 = val2;    // val1 為較大的數
		val2 = val;        // val2 為較小的數
	}

	if (((val2 * Mul) >= (val1 << 8)) || ((val2 + Gap) >= val1)) {
		return 1;
	} else {
		return 0;
	}
}

//--------------------------------------------------------------------------
//  ConditionMax()
//--------------------------------------------------------------------------
VOID ConditionMax(UINT32 pCBV[])
{

	INT32 m;

	for (m = pNPink->TIdx; m < NPINK_QLEN; m++) {
		if (pNPink->Choose[m] == 1) {
			pNPink->MaxV = pCBV[m];
			pNPink->MaxI  = m;
			break;
		}
	}

	for (m = pNPink->MaxI + 1; m < NPINK_QLEN; m++) {
		if (pNPink->Choose[m] == 1) {
			if (pCBV[m] > pNPink->MaxV) {
				pNPink->MaxV = pCBV[m];
				pNPink->MaxI = m;
			}
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  ConditionMin()
//--------------------------------------------------------------------------
VOID ConditionMin(UINT32 pCBV[])
{
	INT32 m;

	for (m = pNPink->TIdx; m < NPINK_QLEN; m++) {
		if (pNPink->Choose[m] == 1) {
			pNPink->MinV = pCBV[m];
			pNPink->MinI  = m;
			break;
		}
	}

	for (m = pNPink->MinI + 1; m < NPINK_QLEN; m++) {
		if (pNPink->Choose[m] == 1) {
			if (pCBV[m] < pNPink->MinV) {
				pNPink->MinV = pCBV[m];
				pNPink->MinI = m;
			}
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  LargerCnt()
//--------------------------------------------------------------------------
INT32 LargerCnt(UINT32 pCBV[])
{
	INT32 i, Cnt = 0;
	UINT32 Thr = (pNPink->MaxV + pNPink->MinV + 1) >> 1;

	for (i = pNPink->TIdx; i < NPINK_QLEN; i++) {
		if (pNPink->Choose[i] == 1) {
			if (pCBV[i] >= Thr) {
				Cnt++;
			}
		}
	}

	return Cnt;
}

//--------------------------------------------------------------------------
//  SmallerCnt()
//--------------------------------------------------------------------------
INT32 SmallerCnt(UINT32 pCBV[])
{
	INT32 i, Cnt = 0;
	UINT32 Thr = (pNPink->MaxV + pNPink->MinV + 1) >> 1;

	for (i = pNPink->TIdx; i < NPINK_QLEN; i++) {
		if (pNPink->Choose[i] == 1) {
			if (pCBV[i] <= Thr) {
				Cnt++;
			}
		}
	}

	return Cnt;
}

//--------------------------------------------------------------------------
//  CheckHistLike()
//  Description : 1.計算每一個 Group 最小 Eng 值
//                2.把 GC_Sorted 內的值往前集中
//                3.GC_Sorted 隨能量從小->大排序 (泡泡排序法)
//                4.從最低能量開始做Spec Matching
//--------------------------------------------------------------------------
INT32 CheckHistLike(VOID)
{
	INT32 m, n, Found;

	struct {
		UINT16 GCnt;
		UINT16 GId;     // 1,2,...,32
		UINT32   GEng;    // 最大值
	} GC_Sorted[NCOLOR_QLEN + 1], Tmp;

	//------------- 0.初始化 -------------
	for (m = 0; m <= NCOLOR_QLEN; m++) {
		GC_Sorted[m].GId = m;       // 0,1,...,32
		GC_Sorted[m].GEng = 0xFFFFFFFF;
	}   // UINT32 最大值

	//------------- 1.計算每一個 Group 最小 Eng 值 -------------
	for (m = 0; m < NCOLOR_QLEN; m++) {
		UINT16 Idx = pNColor->GroupId[m];
		if (Idx > 0) {
			// 非強制偵測點
			if (pNColor->CBEng[m] < GC_Sorted[Idx].GEng) {
				GC_Sorted[Idx].GEng = pNColor->CBEng[m];
			}
		}
	}
	//------------- 2.把 GC_Sorted 內的值往前集中 -------------
	n = 0;
	m = 1;
	while (n < pNColor->GroupNum) {
		if (pNColor->GroupCnt[m] > 0) {
			GC_Sorted[n].GCnt = pNColor->GroupCnt[m];
			GC_Sorted[n].GId = m;
			GC_Sorted[n].GEng = GC_Sorted[m].GEng;
			n++;
		}
		m++;
	}

	//------------- 3.GC_Sorted 隨能量從小->大排序 (泡泡排序法) -------------
	for (m = 0; m < pNColor->GroupNum - 1; m++) {
		for (n = m + 1; n < pNColor->GroupNum; n++) {
			if (GC_Sorted[m].GEng > GC_Sorted[n].GEng) {
				Tmp.GCnt = GC_Sorted[m].GCnt;
				Tmp.GId  = GC_Sorted[m].GId;
				Tmp.GEng = GC_Sorted[m].GEng;

				GC_Sorted[m].GCnt = GC_Sorted[n].GCnt;
				GC_Sorted[m].GId  = GC_Sorted[n].GId;
				GC_Sorted[m].GEng = GC_Sorted[n].GEng;

				GC_Sorted[n].GCnt = Tmp.GCnt;
				GC_Sorted[n].GId  = Tmp.GId;
				GC_Sorted[n].GEng = Tmp.GEng;
			}
		}
	}

	//------------- 4.從最低能量開始做Spec Matching-------------
	Found = 0;
	n = 0;
	while ((Found == 0) && (n < pNColor->GroupNum)) {
		for (m = NCOLOR_QLEN - 2; m >= 0; m--) { // 從倒數第二個向前比對
			if (pNColor->GroupId[m] == GC_Sorted[n].GId) {
				if (Value_Like(pNColor->CBWC[NCOLOR_QLEN - 1], pNColor->CBWC[m], 1 << 8, 2 << 4) && Value_Like(pNColor->CBEng[NCOLOR_QLEN - 1], pNColor->CBEng[m], Tbl.dBup[3], (UINT32)(0.5 * 128))) {
					pNColor->GroupId[NCOLOR_QLEN - 1] = pNColor->GroupId[m];
					pNColor->GroupCnt[pNColor->GroupId[NCOLOR_QLEN - 1]]++;
					Found = 1;
					break;
				}
			}
		}
		n++;
	}

	return Found;
}

//--------------------------------------------------------------------------
//  GetNewNoiseGroupId()
//  Description : 從 1 往 NCOLOR_QLEN 找第一個沒人用的 Id
//--------------------------------------------------------------------------
UINT16 GetNewNoiseGroupId(VOID)
{
	INT32 m;
	UINT16 Id;

	Id = NCOLOR_QLEN;
	for (m = 1; m <= NCOLOR_QLEN; m++) {
		if (pNColor->GroupCnt[m] == 0) {
			Id = m;
			break;
		}
	}
	return Id;
}

//--------------------------------------------------------------------------
//  NoiseCheck()
//--------------------------------------------------------------------------
INT32 NoiseCheck(VOID)
{
	INT32 m, GId, Update, TwoConnect, TSum;

	Update = 0;

	// ------ 1. 頻譜曾經被認定為雜訊 ------
	GId = pNColor->GroupId[NCOLOR_QLEN - 1];
	for (m = 0; m < NCOLOR_QLEN - 1; m++) {
		if ((pNColor->GroupId[m] == GId) && (pNColor->NoiseTag[m] == 1)) {
			Update++;
			break;
		}
	}

	// ------ 2. Frame 兩兩相連比例 ------
	TwoConnect = 0;
	TSum = 0;
	for (m = 0; m < NCOLOR_QLEN - 1; m++) {
		if (pNColor->GroupId[m] == GId) {
			TSum += pNColor->Time[m];
			if (pNColor->GroupId[m + 1] == GId) {
				TwoConnect++;
			}
		}
	}
	if (((TwoConnect << 1) >= pNColor->GroupCnt[GId]) && (TSum >= 63)) {
		Update++;
	}


	return Update;
}


//--------------------------------------------------------------------------
//  Temporal_Estimation()
//--------------------------------------------------------------------------
VOID Temporal_Estimation(VOID)
{
	INT32 i, j, Center, DetectPoint; // 0(非偵測點), 1(雜亂偵測點), 2(強制更新點)
	UINT32 BiasLL;    // Qu10.22
	UINT16 *pus, * pus1;
	INT32 TSum;
	INT32 WC_Purified;
	INT32 Eng_Purified;
#if 0
	INT32 kk;
#endif
	//if (pVar->InFrmCnt == 771)
	//    printf("%d,", pVar->InFrmCnt);

	DetectPoint = 0;
	pNPink->PFrm_Cnt++;
	pNColor->CFrm_Cnt++;

	Center = pCfg->Q_Center;

	//  (          Qu10.6<<10) > Qu10.16
	if ((pVar->default_bias << 10) > pVar->BG_Bias_HD) {
		BiasLL = (pVar->default_bias << 10) * pVar->LL; // (Qu10.16 * Qu2.6) = Qu10.22
	} else {
		BiasLL = pVar->BG_Bias_HD * pVar->LL;  // (Qu10.16 * Qu2.6) = Qu10.22, Bias 最小值設定為 pVar->default_bias
	}

	// -------------------------------------------------------------------------------
	// 1. 計算偵測點的Pink, Color, Spec, CB, CBEng, CBWC, GroupId
	// -------------------------------------------------------------------------------
	//     (Qu10.6<<16) <= Qu10.22)&& ( Qu23   <=   Qu23 ) && (Qu23    <=Qu23    ))

	if (((pVar->QBias[Center] << 16) <= BiasLL) && (pVar->QEngG[Center] <= pVar->QEngG[Center - 1]) && (pVar->QEngG[Center] <= pVar->QEngG[Center + 1])) {
		// ======> (雜亂度高) 且 (相對低點) : 可能為 噪音, 氣音, 嘶吼, 雜訊改變
		DetectPoint = 1;

		pVar->BG_Bias_Target = pVar->QBias[Center];

		//  ------ 1.1 Pink 歷史資料前移,保留最終背景值 ------
		if (pNPink->TIdx > 0) {
			RollingPtr((UINT32 *)(pNPink->Spec), 0, NPINK_QLEN - 1);
			RollingPtr((UINT32 *)(pNPink->CB), 0, NPINK_QLEN - 1);
			Shift_Array((VOID *)(pNPink->CBWC), 0, NPINK_QLEN - 1, 4);
			Shift_Array((VOID *)(pNPink->CBEng), 0, NPINK_QLEN - 1, 4);
			Shift_Array((VOID *)(pNPink->Time), 0, NPINK_QLEN - 1, 2);
			pNPink->TIdx = pNPink->TIdx - 1;
		} else {
			RollingPtr((UINT32 *)(pNPink->Spec), 1, NPINK_QLEN - 1);
			RollingPtr((UINT32 *)(pNPink->CB), 1, NPINK_QLEN - 1);
			Shift_Array((VOID *)(pNPink->CBWC), 1, NPINK_QLEN - 1, 4);
			Shift_Array((VOID *)(pNPink->CBEng), 1, NPINK_QLEN - 1, 4);
			Shift_Array((VOID *)(pNPink->Time), 1, NPINK_QLEN - 1, 2);
		}

		// ------ 1.1 Pink 最新資料加入 ------
		PeakTrim(pQu->pQabs_blur[Center], pQu->pQCB[Center], pNPink->Spec[NPINK_QLEN - 1], pCfg->HPF_CutOffBin);
		CB_Filter(pNPink->Spec[NPINK_QLEN - 1], pNPink->CB[NPINK_QLEN - 1], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);
		pNPink->CBWC[NPINK_QLEN - 1]  = WeightCenter(pNPink->CB[NPINK_QLEN - 1], pCfg->CB_Len);
		pNPink->CBEng[NPINK_QLEN - 1] = EngSum(pNPink->CB[NPINK_QLEN - 1], pCfg->CB_Len);
		pNPink->Time[NPINK_QLEN - 1]  = pNPink->PFrm_Cnt;
		pNPink->PFrm_Cnt = 0;
	} else {
		// ------ 2.1 (訊號段)每1/4秒強迫偵測粉紅噪音------
		if (pNPink->PFrm_Cnt >= pNPink->Max_Update_Interval) {
			DetectPoint = 2;    // 0(非偵測點), 1(雜亂偵測點), 2(強制更新點)

			if (pNPink->TIdx > 0) {
				RollingPtr((UINT32 *)(pNPink->Spec), 0, NPINK_QLEN - 1);
				RollingPtr((UINT32 *)(pNPink->CB), 0, NPINK_QLEN - 1);
				Shift_Array((VOID *)(pNPink->CBWC), 0, NPINK_QLEN - 1, 4);
				Shift_Array((VOID *)(pNPink->CBEng), 0, NPINK_QLEN - 1, 4);
				Shift_Array((VOID *)(pNPink->Time), 0, NPINK_QLEN - 1, 2);
				pNPink->TIdx -= 1;
			} else {
				RollingPtr((UINT32 *)(pNPink->Spec), 1, NPINK_QLEN - 1);
				RollingPtr((UINT32 *)(pNPink->CB), 1, NPINK_QLEN - 1);
				Shift_Array((VOID *)(pNPink->CBWC), 1, NPINK_QLEN - 1, 4);
				Shift_Array((VOID *)(pNPink->CBEng), 1, NPINK_QLEN - 1, 4);
				Shift_Array((VOID *)(pNPink->Time), 1, NPINK_QLEN - 1, 2);
			}

			// ------ 2.2 Pink 最新資料加入 ------
			PeakTrim(pQu->pQabs_blur[Center], pQu->pQCB[Center], pNPink->Spec[NPINK_QLEN - 1], pCfg->HPF_CutOffBin);
			CB_Filter(pNPink->Spec[NPINK_QLEN - 1], pNPink->CB[NPINK_QLEN - 1], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);
			pNPink->CBWC[NPINK_QLEN - 1]  = WeightCenter(pNPink->CB[NPINK_QLEN - 1], pCfg->CB_Len);
			pNPink->CBEng[NPINK_QLEN - 1] = EngSum(pNPink->CB[NPINK_QLEN - 1], pCfg->CB_Len);
			pNPink->Time[NPINK_QLEN - 1]  = pNPink->PFrm_Cnt;
			pNPink->PFrm_Cnt = 0;
		}
	}

	// *******************************************************************************
	// 2. 尋找 Pink 背景->更新 Spec_Target
	// *******************************************************************************
	if (DetectPoint > 0) {  //% 0(非偵測點), 1(雜亂偵測點), 2(強制更新點)
		INT32 CIdx = NPINK_QLEN - 1;
		INT32 TIdx = pNPink->TIdx;
		if (Value_Like(pNPink->CBWC[CIdx], pNPink->CBWC[TIdx], 1 << 8, 2 << 4) && Value_Like(pNPink->CBEng[CIdx], pNPink->CBEng[TIdx], Tbl.dBup[3], 2)) {
			//-----------------[CB重心] 與 [頻譜能量] 和 背景目標相似=>直接更新 Pink 背景-----------------
			pNPink->TIdx = CIdx;
			pus = pNPink->Spec[CIdx];
			for (i = 0; i < pCfg->SPEC_LEN; i++) {
				pNPink->Spec_Target[i] = pus[i];
			}
		} else {
			//-----------------[CB重心] 或 [頻譜能量] 有較大的變異時=>大於0.25秒尋找最佳背景-----------------
			UINT32 TSum = 0;

			for (i = pNPink->TIdx + 1; i < NPINK_QLEN; i++) { // 計算 TSum
				TSum += pNPink->Time[i];
			}

			if (TSum > 16) { // TSum > 0.25 s 才尋找背景
				for (i = pNPink->TIdx; i < NPINK_QLEN; i++) {
					pNPink->Choose[i] = 1;
				}
				pNPink->ChooseCnt = NPINK_QLEN - pNPink->TIdx;

				WC_Purified = 0;

				// 1.重複裁點，直到頻譜重心(已純化)或(剩兩個點以上)
				while ((WC_Purified == 0) && (pNPink->ChooseCnt >= 2)) {
					ConditionMax(pNPink->CBWC);    // update [pNPink->MaxV,pNPink->MaxI]
					ConditionMin(pNPink->CBWC);    // update [pNPink->MinV,pNPink->MinI]

					if (Value_Like(pNPink->MaxV, pNPink->MinV, 1 << 8, 2 << 4) == 1) { // Matlab: Value_Like(MaxV, MinV, 1, 2)
						WC_Purified = 1;
						break;
					} else {
						INT32 LCnt = LargerCnt(pNPink->CBWC);
						INT32 SCnt = SmallerCnt(pNPink->CBWC);
						if (LCnt >= SCnt) {
							pNPink->Choose[pNPink->MinI] = 0;
							pNPink->ChooseCnt -= 1;
						}
						if (SCnt >= LCnt) {
							pNPink->Choose[pNPink->MaxI] = 0;
							pNPink->ChooseCnt -= 1;
						}
					}
				}

				// 2.重複裁點，直到能量(已純化)或(剩兩個點以上)
				Eng_Purified = 0;

				while ((Eng_Purified == 0) && (pNPink->ChooseCnt >= 2)) {
					ConditionMax(pNPink->CBEng);    // update [pNPink->MaxV,pNPink->MaxI]
					ConditionMin(pNPink->CBEng);    // update [pNPink->MinV,pNPink->MinI]

					if (Value_Like(pNPink->MaxV, pNPink->MinV, Tbl.dBup[3], 1) == 1) {
						Eng_Purified = 1;
						break;
					} else {
						INT32 LCnt = LargerCnt(pNPink->CBEng);
						INT32 SCnt = SmallerCnt(pNPink->CBEng);
						if (LCnt >= SCnt) {
							pNPink->Choose[pNPink->MinI] = 0;
							pNPink->ChooseCnt -= 1;
						}
						if (SCnt >= LCnt) {
							pNPink->Choose[pNPink->MaxI] = 0;
							pNPink->ChooseCnt -= 1;
						}
					}
				}

				// 3.剩餘的點，重心差異在2以內
				if ((WC_Purified == 1) && (Eng_Purified == 1)) {
					for (i = NPINK_QLEN - 1; i >= pNPink->TIdx; i--) {
						if (pNPink->Choose[i] == 1) {
							pNPink->TIdx = i;

							pus = pNPink->Spec[i];
							for (j = 0; j < pCfg->SPEC_LEN; j++) {
								pNPink->Spec_Target[j] = pus[j];
							}

							break;
						}
					}
				}
			}
		}
	}
#if 0
	kk = 0;
	tPCM[kk].psBuf = (INT16 *)(Spec_Target);
	file_write_bin(&tPCM[kk], "..//__BG_Pink_Target.pcm", 129, 1, 1);
#endif

	// *******************************************************************************
	// 3. 計算偵測點的 Color
	// *******************************************************************************
	if (DetectPoint > 0) {
		pus = pQu->pQabs_blur[Center];
		pus1 = pNPink->Spec_Target;
		for (i = 0; i < pCfg->SPEC_LEN; i++) {
			if (pus[i] > pus1[i]) {
				pNColor->Tmp_Spec[i] = pus[i] - pus1[i];
			} else {
				pNColor->Tmp_Spec[i] = 0;
			}
		}

		// ------ 3.1 Color 雜亂低點
		if (DetectPoint == 1) {
			if ((pNColor->CFrm_Cnt <= pNColor->Min_Update_Interval) && (pNColor->Time[NCOLOR_QLEN - 1] < pNColor->Max_Update_Interval)) {
				DetectPoint = 0;
				if (pNColor->TIdx != (NCOLOR_QLEN - 1)) {
					// ------ 3.1 相近 Color 頻譜合併
					pus = pNColor->Spec[NCOLOR_QLEN - 1];
					for (i = 0; i < pCfg->SPEC_LEN; i++) {
						pus[i] = pNColor->Tmp_Spec[i];
					}
					CB_Filter(pNColor->Spec[NCOLOR_QLEN - 1], pNColor->CB[NCOLOR_QLEN - 1], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MAX);
					pNColor->CBWC[NCOLOR_QLEN - 1]  = WeightCenter(pNColor->CB[NCOLOR_QLEN - 1], pCfg->CB_Len);
					pNColor->CBEng[NCOLOR_QLEN - 1] = EngSum(pNColor->CB[NCOLOR_QLEN - 1], pCfg->CB_Len);
					pNColor->Time[NCOLOR_QLEN - 1]  = pNColor->Time[NCOLOR_QLEN - 1] + pNColor->CFrm_Cnt;
					pNColor->CFrm_Cnt = 0;
				}
			} else {
				// ------ 3.1 Color 歷史資料前移 ------
				if (pNColor->TIdx == 0) {
					if (pNColor->GroupId[1] > 0) {
						pNColor->GroupCnt[pNColor->GroupId[1]] -= 1;
						if (pNColor->GroupCnt[pNColor->GroupId[1]] == 0) {
							pNColor->GroupNum -= 1;
						}
					}
					RollingPtr((UINT32 *)(pNColor->Spec), 1, NCOLOR_QLEN - 1);
					RollingPtr((UINT32 *)(pNColor->CB), 1, NCOLOR_QLEN - 1);
					Shift_Array((VOID *)(pNColor->CBWC), 1, NCOLOR_QLEN - 1, 4);
					Shift_Array((VOID *)(pNColor->CBEng), 1, NCOLOR_QLEN - 1, 4);
					Shift_Array((VOID *)(pNColor->Time), 1, NCOLOR_QLEN - 1, 2);
					Shift_Array((VOID *)(pNColor->NoiseTag), 1, NCOLOR_QLEN - 1, 2);
					Shift_Array((VOID *)(pNColor->GroupId), 1, NCOLOR_QLEN - 1, 2);
				} else {
					pNColor->TIdx -= 1;
					if (pNColor->GroupId[0] > 0) {
						pNColor->GroupCnt[pNColor->GroupId[0]] -= 1;
						if (pNColor->GroupCnt[pNColor->GroupId[0]] == 0) {
							pNColor->GroupNum -= 1;
						}
					}
					RollingPtr((UINT32 *)(pNColor->Spec), 0, NCOLOR_QLEN - 1);
					RollingPtr((UINT32 *)(pNColor->CB), 0, NCOLOR_QLEN - 1);
					Shift_Array((VOID *)(pNColor->CBWC), 0, NCOLOR_QLEN - 1, 4);
					Shift_Array((VOID *)(pNColor->CBEng), 0, NCOLOR_QLEN - 1, 4);
					Shift_Array((VOID *)(pNColor->Time), 0, NCOLOR_QLEN - 1, 2);
					Shift_Array((VOID *)(pNColor->NoiseTag), 0, NCOLOR_QLEN - 1, 2);
					Shift_Array((VOID *)(pNColor->GroupId), 0, NCOLOR_QLEN - 1, 2);
				}

				// ------ 3.1 Color 最新資料加入 ------
				pus = pNColor->Spec[NCOLOR_QLEN - 1];
				for (i = 0; i < pCfg->SPEC_LEN; i++) {
					pus[i] = pNColor->Tmp_Spec[i];
				}
				CB_Filter(pNColor->Spec[NCOLOR_QLEN - 1], pNColor->CB[NCOLOR_QLEN - 1], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MAX);
				pNColor->CBWC[NCOLOR_QLEN - 1]  = WeightCenter(pNColor->CB[NCOLOR_QLEN - 1], pCfg->CB_Len);
				pNColor->CBEng[NCOLOR_QLEN - 1] = EngSum(pNColor->CB[NCOLOR_QLEN - 1], pCfg->CB_Len);
				pNColor->Time[NCOLOR_QLEN - 1]  = pNColor->CFrm_Cnt;
				pNColor->CFrm_Cnt = 0;
				pNColor->NoiseTag[NCOLOR_QLEN - 1] = 0;
				pNColor->GroupId[NCOLOR_QLEN - 1]  = 0;
			}
		}

		// ------ 3.2 Color 強制更新點
		if (DetectPoint == 2) {
			if (pNColor->TIdx == 0) {
				if (pNColor->GroupId[1] > 0) {
					pNColor->GroupCnt[pNColor->GroupId[1]] -= 1;
					if (pNColor->GroupCnt[pNColor->GroupId[1]] == 0) {
						pNColor->GroupNum -= 1;
					}
				}
				RollingPtr((UINT32 *)(pNColor->Spec), 1, NCOLOR_QLEN - 1);
				RollingPtr((UINT32 *)(pNColor->CB), 1, NCOLOR_QLEN - 1);
				Shift_Array((VOID *)(pNColor->CBWC), 1, NCOLOR_QLEN - 1, 4);
				Shift_Array((VOID *)(pNColor->CBEng), 1, NCOLOR_QLEN - 1, 4);
				Shift_Array((VOID *)(pNColor->Time), 1, NCOLOR_QLEN - 1, 2);
				Shift_Array((VOID *)(pNColor->NoiseTag), 1, NCOLOR_QLEN - 1, 2);
				Shift_Array((VOID *)(pNColor->GroupId), 1, NCOLOR_QLEN - 1, 2);
			} else {
				pNColor->TIdx -= 1;
				if (pNColor->GroupId[0] > 0) {
					pNColor->GroupCnt[pNColor->GroupId[0]] -= 1;
					if (pNColor->GroupCnt[pNColor->GroupId[0]] == 0) {
						pNColor->GroupNum -= 1;
					}
				}
				RollingPtr((UINT32 *)(pNColor->Spec), 0, NCOLOR_QLEN - 1);
				RollingPtr((UINT32 *)(pNColor->CB), 0, NCOLOR_QLEN - 1);
				Shift_Array((VOID *)(pNColor->CBWC), 0, NCOLOR_QLEN - 1, 4);
				Shift_Array((VOID *)(pNColor->CBEng), 0, NCOLOR_QLEN - 1, 4);
				Shift_Array((VOID *)(pNColor->Time), 0, NCOLOR_QLEN - 1, 2);
				Shift_Array((VOID *)(pNColor->NoiseTag), 0, NCOLOR_QLEN - 1, 2);
				Shift_Array((VOID *)(pNColor->GroupId), 0, NCOLOR_QLEN - 1, 2);
			}

			// ------ 3.2 Color 最新資料加入 ------
			pus  = pNColor->Spec[NCOLOR_QLEN - 1];
			for (i = 0; i < pCfg->SPEC_LEN; i++) {
				pus[i] = pNColor->Tmp_Spec[i];
			}
			CB_Filter(pNColor->Spec[NCOLOR_QLEN - 1], pNColor->CB[NCOLOR_QLEN - 1], pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MAX);
			pNColor->CBWC[NCOLOR_QLEN - 1]  = WeightCenter(pNColor->CB[NCOLOR_QLEN - 1], pCfg->CB_Len);
			pNColor->CBEng[NCOLOR_QLEN - 1] = EngSum(pNColor->CB[NCOLOR_QLEN - 1], pCfg->CB_Len);
			pNColor->Time[NCOLOR_QLEN - 1]  = pNColor->CFrm_Cnt;
			pNColor->CFrm_Cnt = 0;
			pNColor->NoiseTag[NCOLOR_QLEN - 1] = 0;
			pNColor->GroupId[NCOLOR_QLEN - 1]  = 0;
		}
	}

	// *******************************************************************************
	// 4. 尋找 Tone 頻譜->更新 pNTone->Spec_Target
	// *******************************************************************************
	if (DetectPoint > 0) {  // 0(非偵測點), 1(雜亂偵測點), 2(強制更新點)
		pus = pNColor->Spec[NCOLOR_QLEN - 1];
		for (i = 0; i < pCfg->SPEC_LEN; i++) {
			pNTone->Spec_Target[i] = pus[i];
		}

		TSum = 0;
		j = NCOLOR_QLEN - 2;
		while ((j >= 0) && (TSum < pNTone->ToneMinFrmCnt)) {
			pus = pNColor->Spec[j];
			for (i = 0; i < pCfg->SPEC_LEN; i++) {
				pNTone->Spec_Target[i] = pNTone->Spec_Target[i] < pus[i] ? pNTone->Spec_Target[i] : pus[i];
			}
			TSum += pNColor->Time[j];
			j--;
		}
		for (i = 0; i < pCfg->SPEC_LEN; i++) {
			pNTone->Spec_Target[i] = (pNTone->Spec_Target[i] * Tbl.dBup[3] + (1 << 7)) >> 8;
		}
	}
#if 0
	kk = 1;
	tPCM[kk].psBuf = (INT16 *)(pNTone->Spec_Target);
	file_write_bin(&tPCM[kk], "..//__BG_Tone.pcm", 129, 1, 1);
#endif

	// *******************************************************************************
	// 5. 尋找 Color 背景->更新 pNColor->Spec_Target
	// *******************************************************************************
	if (DetectPoint > 0) {  // 0(非偵測點), 1(雜亂偵測點), 2(強制更新點)
		//if ((pVar->InFrmCnt >= 10281) && (pVar->InFrmCnt <= 10297))
		//    printf("\n%d,", pVar->InFrmCnt);

		if (DetectPoint == 1) { // 1(雜亂偵測點)
			INT32 Found = CheckHistLike();

			if (Found == 0) {
				// 歷史上"沒"找到相似背景
				UINT16 NewIdx = GetNewNoiseGroupId();
				pNColor->GroupNum += 1;
				pNColor->GroupId[NCOLOR_QLEN - 1] = NewIdx;
				pNColor->GroupCnt[NewIdx] = 1;

				if ((pNColor->CBEng[NCOLOR_QLEN - 1]*Tbl.dBup[3]) <= pNColor->Spec_Target_CBEng) {
					// Current 的 CBEng 如果小於 Target CBEng,則直接用 Current 取代 Target
					pus = pNColor->Spec[NCOLOR_QLEN - 1];
					for (i = 0; i < pCfg->SPEC_LEN; i++) {
						pNColor->Spec_Target[i] = pus[i];     // pNColor->Spec_Target[] =
					}

					pus = pNColor->CB[NCOLOR_QLEN - 1];
					for (i = 0; i < pCfg->CB_Len; i++) {
						pNColor->Spec_Target_CB[i] = pus[i];
					}

					pNColor->Spec_Target_CBWC  = pNColor->CBWC[NCOLOR_QLEN - 1];
					pNColor->Spec_Target_CBEng = pNColor->CBEng[NCOLOR_QLEN - 1];
					pNColor->TIdx = (NCOLOR_QLEN - 1);
					pNColor->NoiseTag[NCOLOR_QLEN - 1] = 1;
				}
			} else {
				// ------ 歷史上有類似噪音頻譜
				INT32 UpdateNoise = NoiseCheck();
				if (UpdateNoise > 0) {
					// Current 頻譜曾被設定成噪音 或 ((兩兩相連比例 >= 50%) 而且 (時間累積>1秒))
					pus = pNColor->Spec[NCOLOR_QLEN - 1];
					for (i = 0; i < pCfg->SPEC_LEN; i++) {
						pNColor->Spec_Target[i] = pus[i];
					}

					pus = pNColor->CB[NCOLOR_QLEN - 1];
					for (i = 0; i < pCfg->CB_Len; i++) {
						pNColor->Spec_Target_CB[i] = pus[i];
					}

					pNColor->Spec_Target_CBWC  = pNColor->CBWC[NCOLOR_QLEN - 1];
					pNColor->Spec_Target_CBEng = pNColor->CBEng[NCOLOR_QLEN - 1];
					pNColor->TIdx = (NCOLOR_QLEN - 1);
					pNColor->NoiseTag[NCOLOR_QLEN - 1] = 1;
				}
			}
		} else {
			// DetectPoint==2(強制更新點)
			INT32 IdxFound = 0;
			for (j = NCOLOR_QLEN - 2; j >= 0; j--) {
				// 向前尋找
				if (pNColor->NoiseTag[j] == 1) {
					// 曾經是噪音
					pus = pNColor->Spec[j];
					pus1 = pNColor->Spec[NCOLOR_QLEN - 1];
					for (i = 0; i < pCfg->SPEC_LEN; i++) {
						UINT32 Val1 = (UINT32)pus[i] * Tbl.dBup[0];
						UINT32 Val2 = (UINT32)pus1[i] * Tbl.dBup[3];

						pNColor->Tmp_Spec[i] = Val1 < Val2 ? pus[i] : pus1[i];    // 計算 min(曾經是噪音,目前噪音*3dB)
					}
					CB_Filter(pNColor->Tmp_Spec, pNColor->Tmp_Spec_CB, pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);
					pNColor->Tmp_Spec_CBWC = WeightCenter(pNColor->Tmp_Spec_CB, pCfg->CB_Len);
					pNColor->Tmp_Spec_CBEng = EngSum(pNColor->Tmp_Spec_CB, pCfg->CB_Len);
					if (Value_Like(pNColor->CBWC[j], pNColor->Tmp_Spec_CBWC, 1 << 8, 2 << 4) && Value_Like(pNColor->CBEng[j], pNColor->Tmp_Spec_CBEng, Tbl.dBup[6], 1)) {
						// 如果 舊噪音 和 預估噪音 的 重心差異 <= 2 且 重心能量差異<6dB
						pNColor->TIdx = j;
						IdxFound = 1;   // 找到噪音索引值
						break;
					}
				}
			}

			if (IdxFound == 1) {
				// 找到噪音索引值
				pus = pNColor->Spec[pNColor->TIdx];
				for (i = 0; i < pCfg->SPEC_LEN; i++) {
					pNColor->Spec_Target[i] = pus[i];     // 更新 Target
				}
			} else {
				// 沒找到噪音索引值->只好取最小值來更新
				pus = pNColor->Spec[NCOLOR_QLEN - 1];
				for (i = 0; i < pCfg->SPEC_LEN; i++) {
					UINT32 Val1 = (UINT32)pNColor->Spec_Target[i] * Tbl.dBup[0];
					UINT32 Val2 = (UINT32)               pus[i] * Tbl.dBup[3];
					pNColor->Spec_Target[i] = Val1 < Val2 ? pNColor->Spec_Target[i] : pus[i]; // 如果預估頻譜較小,無條件更新 Target
				}
			}
			CB_Filter(pNColor->Spec_Target, pNColor->Spec_Target_CB, pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);
			pNColor->Spec_Target_CBWC = WeightCenter(pNColor->Spec_Target_CB, pCfg->CB_Len);
			pNColor->Spec_Target_CBEng = EngSum(pNColor->Spec_Target_CB, pCfg->CB_Len);
		}
	}
#if 0
	kk = 2;
	tPCM[kk].psBuf = (INT16 *)(pNColor->Spec_Target);
	file_write_bin(&tPCM[kk], "..//__BG_Color_Target.pcm", 129, 1, 1);
#endif

	// *******************************************************************************
	// 6. (合併 pNTone->Spec_Target + pNColor->Spec_Target)
	// *******************************************************************************
	if (DetectPoint > 0) {
		for (i = 0; i < pCfg->SPEC_LEN; i++) {
			pVar->BG_Spec_Target[i] = pNPink->Spec_Target[i] + (pNColor->Spec_Target[i] > pNTone->Spec_Target[i] ? pNColor->Spec_Target[i] : pNTone->Spec_Target[i]);
		}
	}
#if 0
	kk = 3;
	tPCM[kk].psBuf = (INT16 *)(pVar->BG_Spec_Target);
	file_write_bin(&tPCM[kk], "..//__BG_Spec_Target.pcm", 129, 1, 1);
#endif

	// *******************************************************************************
	// 7. Bias, Spec 平滑化
	// *******************************************************************************
	// Qu10.16 = (Qu10.16*31 +         (Qu10.6<<10) + (1<<4))>>5
	pVar->BG_Bias_HD   = (pVar->BG_Bias_HD * 31 + (pVar->BG_Bias_Target << 10) + (1 << 4)) >> 5;
	for (i = 0; i < pCfg->SPEC_LEN; i++) {
		//     Qu16.10 = (Qu16.10       *31 +                            (Qu16<<10) + (1<<4))>>5
		pVar->BG_Spec_HD[i] = (pVar->BG_Spec_HD[i] * 31 + (((UINT32)pVar->BG_Spec_Target[i]) << 10) + (1 << 4)) >> 5;
	}
#if 0
	kk = 4;
	tPCM[kk].psBuf = (INT16 *)(pVar->BG_Spec_HD);
	file_write_bin(&tPCM[kk], "..//__BG_Spec.pcm", 129 * 2, 1, 1);
#endif

	return;
}

//--------------------------------------------------------------------------
//  ColLink()
//--------------------------------------------------------------------------
VOID ColLink(UINT16 *pAbs,
			 UINT16 *pBGboost,
			 INT16 *pCLink_s,
			 INT16 *pCLink_e,
			 struct _BandEng              *pCLink_o)
{
	INT32 k, idx;
	UINT16 Nxt, Nxt1, Cur, SFloor;

//DumpUInt16(pAbs, pCfg->Valid_SIdx_High, 1);
//DumpUInt16(pBGboost, pCfg->Valid_SIdx_High, 1);

	idx = 0;
	k = pCfg->NR_SIdx_Low;
	while (k < pCfg->NR_SIdx_High) {
		Cur    = pAbs[k];
		SFloor = pBGboost[k];
		if (Cur > pBGboost[k]) {
			pCLink_s[idx] = k;
			pCLink_e[idx] = k;
			pCLink_o[idx].AbsSum = Cur;
			pCLink_o[idx].BGSum = SFloor;
			k++;
			while (k < pCfg->NR_SIdx_High) {
				Nxt = pAbs[k];
				Nxt1 = pAbs[k + 1];
				SFloor = pBGboost[k];

				if ((Nxt <= pBGboost[k]) && (Nxt1 <= pBGboost[k + 1])) {
					pCLink_e[idx] = k - 1;
					idx++;
					break;
				} else if (k == (pCfg->NR_SIdx_High - 1)) {
					pCLink_e[idx] = k;
					pCLink_o[idx].AbsSum += Nxt;
					pCLink_o[idx].BGSum += SFloor;
					idx++;
					k++;
					break;
				} else {
					//Cur = Nxt; [fixed coverity]
					pCLink_o[idx].AbsSum += Nxt;
					pCLink_o[idx].BGSum += SFloor;
				}
				k++;
			}
		} else {
			k++;
		}
	}

	pCLink_s[idx] = -1;        // 設為 -1，表示結束
	pCLink_e[idx] = -1;
	pCLink_o[idx].AbsSum = -1;
	pCLink_o[idx].BGSum = -1;

	return;
}

//--------------------------------------------------------------------------
//  FitMulCurve()
//--------------------------------------------------------------------------
VOID FitMulCurve(UINT32 *pBG_Spec,
				 UINT16 *pBGboost,
				 UINT32 *pSig_MCurve)
{
	INT32 i;
	UINT32 n, n1, Lift, CurLift;

	// 計算背景 boost 後的頻譜圖，拿來做 Signal Gate
	i = pCfg->Valid_SIdx_Low + 1;    // 忽略 DC (Bin 0)
	n = (UINT16)((pVar->Sig_MCurve[i] * ((pVar->BG_Spec_HD[i] + (1 << 1)) >> 2) + ((1 << 14) - 1)) >> 14);
	Lift = n - (pVar->BG_Spec_HD[i] >> 10);

	pVar->BGboost[i] = (UINT16)n;

	for (i = pCfg->Valid_SIdx_Low + 2; i <= pCfg->Idx_1k; i++) { // 1k Hz index
		// Qu18 = (Qu2.6        *    Qu16.8             ) + (1<<13))>>14
		n    = (pVar->Sig_MCurve[i] * ((pVar->BG_Spec_HD[i] + (1 << 1)) >> 2) + ((1 << 14) - 1)) >> 14;

		CurLift = n - (pVar->BG_Spec_HD[i] >> 10);

		// limit pVar->BGboost to 0~65535
		if (n > 65535) {
			n = 65535;
		}

		if (CurLift < Lift) {
			Lift = CurLift;
		}

		pVar->BGboost[i] = (UINT16)n;
	}

	for (i = pCfg->Idx_1k + 1; i <= pCfg->Valid_SIdx_High; i++) {
		// Qu18 = (Qu2.6        *    Qu16.8             ) + (1<<13))>>14
		n  = (pVar->Sig_MCurve[i] * ((pVar->BG_Spec_HD[i] + (1 << 1)) >> 2) + ((1 << 14) - 1)) >> 14;
		n1 = ((pVar->BG_Spec_HD[i] + (1 << 9)) >> 10) + Lift;

		if (n > n1) {
			pVar->BGboost[i] = (UINT16)n;
		} else {
			pVar->BGboost[i] = (UINT16)n1;
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  FindRLink()
//--------------------------------------------------------------------------
VOID FindRLink(UINT32 First_SS)
{
	INT32 i, j, k, n, Center, Last;
	//INT32 s,e;
	struct _BandEng *pi;
	INT16 *ps, * ps1, * ps2;

	Center = pCfg->Q_Center;        // 4
	Last   = Center + pCfg->RLink_Len - 1;  // 8

	FitMulCurve(pVar->BG_Spec_HD, pVar->BGboost, pVar->Sig_MCurve);

	if (pVar->First_SS == 1) {
		for (i = Center; i <= Last; i++) {
//Dump_Data_UInt(pQu->pQabs_blur[i], 2, pCfg->SPEC_LEN);
//Dump_Data_UInt(pVar->BGboost, 2, pCfg->SPEC_LEN);
//INT32 kk;
//for (kk=0;kk<pCfg->SPEC_LEN;kk++)
//    printf("\n %3d (%4d,%4d) %d", kk, *(pQu->pQabs_blur[i]+kk), *(pVar->BGboost+kk), *(pQu->pQabs_blur[i]+kk) > *(pVar->BGboost+kk));
			ColLink(pQu->pQabs_blur[i], pVar->BGboost, pLink->pCLink_S[i], pLink->pCLink_E[i], pLink->pCLink_O[i]);     // find all Column Links
//DumpLink(pLink->pCLink_S[i], pLink->pCLink_E[i],pLink->pCLink_O[i]);
		}

		for (k = Center; k <= Last; k++) {
			ps = pLink->pQMark[k];    // Qu1.15
			for (i = pCfg->Valid_SIdx_Low; i <= pCfg->Valid_SIdx_High; i++) {
				ps[i] = 0;       // reset pQMark[4~8] to 0
			}
		}
	} else {
		ps  = pLink->pCLink_S[Center];    // rotate pCLink_S[], pCLink_E[]
		ps1 = pLink->pCLink_E[Center];
		pi  = pLink->pCLink_O[Center];
		for (i = Center; i < Last; i++) { // move [5~8] to [4~7]
			pLink->pCLink_S[i] = pLink->pCLink_S[i + 1];
			pLink->pCLink_E[i] = pLink->pCLink_E[i + 1];
			pLink->pCLink_O[i] = pLink->pCLink_O[i + 1];
		}
		pLink->pCLink_S[Last] = ps;
		pLink->pCLink_E[Last] = ps1;
		pLink->pCLink_O[Last] = pi;

//Dump_Data_UInt(pQu->pQabs_blur[8], 2, pCfg->SPEC_LEN);
//Dump_Data_UInt(pVar->BGboost, 4, pCfg->SPEC_LEN);
//INT32 kk;
//for (kk=0;kk<pCfg->SPEC_LEN;kk++)
//    printf("\n %3d (%4d,%4d) %d", kk, *(pQu->pQabs_blur[i]+kk), *(pVar->BGboost+kk), *(pQu->pQabs_blur[i]+kk) > *(pVar->BGboost+kk));
		ColLink(pQu->pQabs_blur[Last], pVar->BGboost, pLink->pCLink_S[Last], pLink->pCLink_E[Last], pLink->pCLink_O[Last]);     // find last Column Links
//DumpLink(pCLink_S[8], pCLink_E[8], pCLink_O[8]);

		ps  = pLink->pQMark[Center];    // rotate pQMark[4~8]
		for (i = Center; i < Last; i++) {
			pLink->pQMark[i] = pLink->pQMark[i + 1];
		}
		pLink->pQMark[Last] = ps;

		ps = pLink->pQMark[Last];        // set last pQMark[8] to [0.1*32768]
		for (i = pCfg->Valid_SIdx_Low; i <= pCfg->Valid_SIdx_High; i++) {
			ps[i] = 0;       // set pQMark[8] to 0
		}
	}

	// 倒數兩個禎的左右連結性
	RowLink(Last - 1, 'c', 'c');  // link 8->7
//DumpLink(pRLink_S[7], pRLink_E[7], pRLink_O[7]);
//DumpLink(pRLink_S[8], pRLink_E[8], pRLink_O[8]);

	// 其他相鄰禎的左右相連性(右到左)
	for (i = Last - 2; i >= Center; i--) {
		RowLink(i, 'c', 'r');        // link 7->6, 6->5, 5->4
//DumpLink(pRLink_S[i], pRLink_E[i], pRLink_O[i]);
//DumpLink(pRLink_S[i+1], pRLink_E[i+1], pRLink_O[i+1]);
	}

	// 其他相鄰禎的左右相連性(左到右)
	for (i = Center + 1; i < Last; i++) {
		RowLink(i, 'r', 'r');        // link 5->6, 6->7, 7->8
//DumpLink(pRLink_S[i], pRLink_E[i], pRLink_O[i]);
//DumpLink(pRLink_S[i+1], pRLink_E[i+1], pRLink_O[i+1]);
	}
//DumpLink(pRLink_S[8], pRLink_E[8], pRLink_O[8]);
//DumpInt16(RLink_Len, 10, 0);

	// 將 Signal Block List 轉換成頻譜矩陣
	for (i = Center; i <= Last; i++) {
		ps  = pLink->pRLink_S[i];    // 起始 Bin 值
		ps1 = pLink->pRLink_E[i];    // 結束 Bin 值
		pi  = pLink->pRLink_O[i];    // .AbsSum(區段頻譜和), .BGSum(區段背景頻譜和)
		ps2 = pLink->pQMark[i];        // pQMark 值介於 0~10
		for (k = 0; k < pLink->Link_Segs[i]; k++) {
			INT32 MS  = pi[k].BGSum << 7;                    // *128
			INT32 Mul = pi[k].BGSum << (7 - NR_FADE_UNIT_EXP);  // *8      => 1/(128/8)=0.0625
			INT32 Div = pi[k].AbsSum << 7;    // *128
			for (n = 0; n < NR_FADE_CEIL_STEPS; MS += Mul, n++) { // MS = MS*[1, 1.125,..., 1.875, 2]
				if (Div < MS) {
					break;
				}
			}    // 輸出 n [1~10]

			for (j = ps[k]; j <= ps1[k]; j++) { //pRLink_S[k]~pRLink_E[k]
				//ps2[j] = ps2[j] > n ? ps2[j] : n;
				ps2[j] = n;        // 範圍 [1~10], 1 表示                          (ABS和) < (背景和)
				//              2 表示 (背景和*(1 + 0.125*0) <= (ABS和) < (背景和*(1 + 0.125*1)
				//             10 表示 (背景和*(1 + 0.125*8) <= (ABS和)
			}
		}
	}

	// 計算 Signal Bin 個數
	//k = 0;
	//ps = pQMark[4];
	//for (i=pCfg->Valid_SIdx_Low;i<=pCfg->Valid_SIdx_High;i++)
	//{
	//    if (ps[i] >= 1)    // 0 是 Noise, >1 才是 Signal
	//    {
	//        k++;
	//        break;
	//    }
	//}

	return;    // have Signal Bin(s)
}

//--------------------------------------------------------------------------
//  MaskSmoothing()
//--------------------------------------------------------------------------
VOID MaskSmoothing(INT16 *pMark, INT16 *pMark_Smooth)
{
	INT32 i;

	for (i = 0; i < pCfg->SPEC_LEN; i++) {
		if (pMark[i] > 0) {
			INT16 Tmp = pMark_Smooth[i] - 1;
			pMark_Smooth[i] = pMark[i] > Tmp ? pMark[i] : Tmp;
		} else {
			pMark_Smooth[i] = 0;
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  Spectrum_Suppress()
//--------------------------------------------------------------------------
VOID Spectrum_Suppress(VOID)
{
	INT32    i, j;
	struct kfft_data *ptr;
	INT16 *ptr1;
	INT32 sr;       // Qu16+8(8kHz), Qu16+9(16kHz)
	INT32 si;       // Qu16+8(8kHz), Qu16+9(16kHz)
	INT32 s2;      //  Qu1.7(8kHz),  Qu1.6(16kHz)
	INT32 RShift;
	INT32 Half;
	//INT32        Enhance_Bins;

	// Matlab : [Need_Enhance] = FindRLink(pVar->BG_Spec_HD(:,k), fft_abs(:,k:k+RLink_Len-1), Mode_Switch, pVar->Sig_MCurve);    % AAA(1,1,0,fft_abs(:,k-5:k+5), pVar->BG_Spec_HD(:,k-5:k+5))
	FindRLink(pVar->First_SS);

	pVar->First_SS = 0;    // not first frame

	// Matlab : pVar->fft_abs_clr(:,i) = pQMark(:,1) .* (fft_abs(:,i) - NRRatio * BG_Noise);
//DumpUInt16(pQu->pQabs[4], 129, 1);
//DumpUInt16(pQMark[4], 129, 1);

	MaskSmoothing(pLink->pQMark[pCfg->Q_Center], pVar->Mark_Smooth);

	ptr = pQu->pQFFT[pCfg->Q_Center];            // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
	ptr1 = pVar->Mark_Smooth;     //

	sr = ptr[0].r;              // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
	si = ptr[0].i;              // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
	s2 = pVar->NRTable[ptr1[0]];   // Qu1.7(>=8kHz), Qu1.6(>=16kHz), Qu1.5(>=32kHz)
	RShift = 15 - pCfg->FFT_SZ_Exp;
	Half = 1 << (RShift - 1);

	ptr[0].r = ((sr * s2 + Half) >> RShift); // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
	ptr[0].i = ((si * s2 + Half) >> RShift); // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)

	for (i = 1, j = pCfg->FFT_SZ - 1; i < pCfg->SPEC_LEN; i++, j--) {
		s2 = pVar->NRTable[ptr1[i]];   // Qu1.7(>=8kHz), Qu1.6(>=16kHz), Qu1.5(>=32kHz)
		ptr[i].r = ((ptr[i].r * s2 + Half) >> RShift); // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
		ptr[i].i = ((ptr[i].i * s2 + Half) >> RShift); // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
		ptr[j].r = ptr[i].r;    // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
		ptr[j].i = -ptr[i].i;   // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
	}
	s2 = pVar->NRTable[ptr1[i]];   // Qu1.7(>=8kHz), Qu1.6(>=16kHz), Qu1.5(>=32kHz)
	ptr[i].r = ((ptr[i].r * s2 + Half) >> RShift); // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)
	ptr[i].i = ((ptr[i].i * s2 + Half) >> RShift); // Qu16+8(8kHz,11kHz), Qu16+9(16kHz,22kHz), Qu16+10(32kHz,44kHz,48kHz)

	return;
}

//--------------------------------------------------------------------------
//  GetFFToutSlot()
//--------------------------------------------------------------------------
struct kfft_data *GetFFToutSlot(void)
{
	struct kfft_data *pOut;

	if (pQu->QIdx == pCfg->Q_Len) {
		// pQu->QIdx == 10

		// rolling
		RollingPtr((UINT32 *)pQu->pQFFT, pCfg->Q_Center, pCfg->Q_Len - 1);

		pOut = pQu->pQFFT[pCfg->Q_Len - 1];
	} else if (pQu->QIdx <= pCfg->Q_Center) {
		// pQu->QIdx = 0~Center, pQu->QIdx 預設為零
		pOut = pQu->pQFFT[pCfg->Q_Center];  // 0~(Q_Center) 沒配置空間
	} else {
		// pQu->QIdx = 5~9, pQu->QIdx 預設為零
		pOut = pQu->pQFFT[pQu->QIdx];
	}

	return pOut;
}

//--------------------------------------------------------------------------
//  _ANR_process()
//--------------------------------------------------------------------------
VOID _ANR_process(VOID)
{
	INT32 i, Tmp;
	struct kfft_data *pFFTout;

//    --- input sine wave ---
	UINT32 Bias_1, Bias_2;  // Qu10.6


#if DEBUGANR
	INT32 kk = 0;
#endif

#if 0
	kk++;
	tPCM[kk].psBuf = (INT16 *)(pVar->asFFT0);
	file_write_bin(&tPCM[kk], "..//__asFFT0.pcm", 128, 1, 1);
	//Dump_Data_Int(pVar->asFFT0, 2, pCfg->FFT_SZ);
#endif

	pVar->InFrmCnt++;

//for (i=0;i<pCfg->FFT_SZ;i++)   printf("\n %3d, %12d", i, pVar->asFFT0[i].r);
	time0(Windowing(pVar->asFFT0));    // _ANR_process
//for (i=0;i<pCfg->FFT_SZ;i++)   printf("\n %3d(%12d,%12d)", i, pVar->asFFT0[i].r, pVar->asFFT0[i].i);

	time0(pFFTout = GetFFToutSlot());

	time0(_kiss_fft(pKFFT_Cfg0, pVar->asFFT0, pFFTout));
//for (i=0;i<pCfg->FFT_SZ;i++)   printf("\n %3d(%12d,%12d)", i, pFFTout[i].r, pFFTout[i].i);
#if 0
	kk++;
	tPCM[kk].psBuf = (INT16 *)(pVar->asFFT0);
	file_write_bin(&tPCM[kk], "..//__asFFT0_r.pcm", pCfg->FFT_SZ, 1, 1);
	kk = 2;
	tPCM[kk].psBuf = (INT16 *)(pVar->asFFT0_i);
	file_write_bin(&tPCM[kk], "..//__asFFT0_i.pcm", pCfg->FFT_SZ, 1, 1);
#endif

	time0(PolarCoordinate(pFFTout, pVar->asNRabs, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High));      // [2~127],[0,1,128]設為零
//for (i=0;i<pCfg->SPEC_LEN;i++)   printf("\n %3d %12d", i, pVar->asNRabs[i]);
#if DEBUGANR
	kk++;
	tPCM[kk].psBuf = (INT16 *)(pVar->asNRabs);
	file_write_bin(&tPCM[kk], "..//__Abs.pcm", pCfg->SPEC_LEN);
#endif

	time0(PushInQueueAndBlur(pVar->asNRabs, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High));
//printf("\n pQu->pQabs,pQu->QIdx(%d)",pQu->QIdx);   for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQabs[i], 2, 6);
//printf("\n pQu->pQabs_blur_tmp");        for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQabs_blur_tmp[i], 4, 6);
//printf("\n pQu->pQabs_blur");            for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQabs_blur[i], 2, 6);
#if DEBUGANR
	kk++;
	if (pQu->QIdx >= 1) {
		if (pQu->QIdx != pCfg->Q_Len) {
			//kk = 6;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQabs_blur[pQu->QIdx - 1]);
			file_write_bin(&tPCM[kk], "..//__Blur.pcm", pCfg->SPEC_LEN);
		} else {
			// pQu->QIdx == 10
			//kk = 6;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQabs_blur[pCfg->Q_Len - 2]);
			file_write_bin(&tPCM[kk], "..//__Blur.pcm", pCfg->SPEC_LEN);
		}
	}
#endif

	// Unbalance 在 pQu->QIdx 0~8 的時候，會回傳 default_bias
	time0(Bias_1 = Unbalance(pQu->pQabs, pVar->pQabs_GSum_G, pCfg->spec_bias_low, pCfg->spec_bias_high));      // Qu26.6
	time0(Bias_2 = Unbalance(pQu->pQabs_blur, pVar->pQabs_blur_GSum_G, pCfg->spec_bias_low, pCfg->spec_bias_high));      // Qu26.6
#if 0
	if (pQu->QIdx > 4) {
		kk++;
		tPCM[kk].psBuf = (INT16 *)(&Bias_1);
		file_write_bin(&tPCM[kk], "..//__Bias_1.pcm", 2, 1, 1);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(&Bias_2);
		file_write_bin(&tPCM[kk], "..//__Bias_2.pcm", 2, 1, 1);
	}
#endif

	if (pQu->QIdx == pCfg->Q_Len) { // pQu->QIdx 介於 0~8 時, Unbalance 傳回 default_bias
		Shift_Array((VOID *)pVar->QBias, 0, pCfg->Q_Center, 4);
	}
	Bias_1       = Bias_1 > Bias_2 ? Bias_1 : Bias_2;
	pVar->QBias[pCfg->Q_Center] = Bias_1 < (5 << 6) ? Bias_1 : (5 << 6);

#if DEBUGANR
	kk++;
	if (pQu->QIdx > pCfg->Q_Center) {
		tPCM[kk].psBuf = (INT16 *)(&(pVar->QBias[pCfg->Q_Center]));
		file_write_bin(&tPCM[kk], "..//__Bias.pcm", 2);
	}
#endif

	time0(CB_Eng_Fun(pQu->pQabs_blur, pQu->pQCB, &pVar->QEngG[0]));

#if DEBUGANR
	kk++;
	if (pQu->QIdx >= 1) {
		if (pQu->QIdx != pCfg->Q_Len) {
			//kk++;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQCB[pQu->QIdx - 1]);
			file_write_bin(&tPCM[kk], "..//__pQCB.pcm", pCfg->CB_Len);
			//kk++;
			tPCM[kk + 1].psBuf = (INT16 *)(&pVar->QEngG[pQu->QIdx - 1]);
			file_write_bin(&tPCM[kk + 1], "..//__QEngG.pcm", 2);
		} else {
			//kk++;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQCB[pCfg->Q_Len - 2]);
			file_write_bin(&tPCM[kk], "..//__pQCB.pcm", pCfg->CB_Len);
			//kk++;
			tPCM[kk + 1].psBuf = (INT16 *)(&pVar->QEngG[pCfg->Q_Len - 2]);
			file_write_bin(&tPCM[kk + 1], "..//__QEngG.pcm", 2);
		}
	}
#endif

	if (pQu->QIdx >= pCfg->Q_Len - 1) { // 10 個 buffer 內都有資料，開始輸出 pQu->QIdx==Q_Center 的音訊資料
		pVar->OutFrmCnt++;

		time0(Temporal_Estimation());
#if 0
		kk += 2;
		tPCM[kk].psBuf = (INT16 *)(pQu->pQabs[pCfg->Q_Center]);
		file_write_bin(&tPCM[kk], "..//_Abs.pcm", pCfg->SPEC_LEN);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(&(pVar->QBias[pCfg->Q_Center]));
		file_write_bin(&tPCM[kk], "..//_Bias.pcm", 2);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pQu->pQabs_blur[pCfg->Q_Center]);
		file_write_bin(&tPCM[kk], "..//_Blur.pcm", pCfg->SPEC_LEN);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pQu->pQCB[pCfg->Q_Center]);
		file_write_bin(&tPCM[kk], "..//_CB.pcm", pCfg->CB_Len);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(&(pVar->QEngG[pCfg->Q_Center]));
		file_write_bin(&tPCM[kk], "..//_EngG.pcm", 2);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pNPink->Spec_Target);
		file_write_bin(&tPCM[kk], "..//_PinkTarget.pcm", pCfg->SPEC_LEN);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pNColor->Spec_Target);
		file_write_bin(&tPCM[kk], "..//_ColorTarget.pcm", pCfg->SPEC_LEN);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pNTone->Spec_Target);
		file_write_bin(&tPCM[kk], "..//_ToneTarget.pcm", pCfg->SPEC_LEN);
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pVar->BG_Spec_HD);
		file_write_bin(&tPCM[kk], "..//_BG_Spec.pcm", pCfg->SPEC_LEN * 2);
#endif

		time0(Spectrum_Suppress());
#if 0
		kk++;
		tPCM[kk].psBuf = (INT16 *)(pVar->Mark_Smooth);
		file_write_bin(&tPCM[kk], "..//_Mark.pcm", pCfg->SPEC_LEN);
#endif

		//time0(CartesianCoordinate(pVar->fft_abs_clr, pQang[4], pVar->asFFT0, pVar->asFFT0_i, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High));

		//time0(scale = fix_fft(pVar->asFFT0, pVar->asFFT0_i, pCfg->FFT_SZ, 1));
		time0(_kiss_fft(pKFFT_Cfg1, pQu->pQFFT[pCfg->Q_Center], pVar->asFFT0));
//printf("\n"); for (i=0;i<5;i++)   printf("%d(%d,%d) ", i, pVar->asFFT0[i].r, pVar->asFFT0[i].i);
#if 0
		printf("\n FrmCnt(%d)\n", pVar->InFrmCnt);
		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			printf("%d,", pVar->asFFT0[i].r);
		}
#endif
		// 修正定點FFT低頻偏差問題,順便Scaling
		Tmp = pVar->asFFT0[0].r;
		pVar->asFFT0[0].r = 0;
		for (i = 1; i < pCfg->FFT_SZ; i++) {
			pVar->asFFT0[i].r = pVar->asFFT0[i].r - Tmp;
		}

		// Overlap and Add 演算法
		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			Tmp = pVar->asFFT0[i].r + pVar->asNRtail[i];

			if (Tmp > 32767) {
				pVar->asNRout[i] = 32767;
			} else if (Tmp < -32767) {
				pVar->asNRout[i] = -32767;
			} else {
				pVar->asNRout[i]  = (INT16)Tmp;
			}

			pVar->asNRtail[i] = pVar->asFFT0[i + pCfg->FFT_OV_SZ].r;    //_ANR_process()
		}
	}

#if 0
	printf("\n FrmCnt(%d)\n", pVar->InFrmCnt);
	for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
		printf("%d,", pVar->asNRout[i]);
	}
#endif

	if (pQu->QIdx < pCfg->Q_Len) {
		pQu->QIdx++;
	}

	// printf("\n");

	return;
}

//--------------------------------------------------------------------------
//  CalculateHHLL()
//    Description : Calculate pVar->HH, pVar->LL values
//    Return :
//--------------------------------------------------------------------------
VOID CalculateHHLL(VOID)
{

	// 計算 pVar->HH, pVar->LL
	pVar->HH = ((pVar->max_bias << 6) + (((pCfg->max_bias_limit - pVar->max_bias) * (9 - pCfg->bias_sensitive)) << 3)) / pVar->default_bias;
	// ((Qu10.6 * Qu4)<<6)/Qu10.6 = Qu2.6
	pVar->LL = ((pVar->HH - 64 + 1) >> 1) + 64;

	return;
}

//--------------------------------------------------------------------------
//  CreateMulCurve()
//    Description: 製造音頻倍數曲線
//                  Bin[0~Idx1)    為倍數 M1
//                  Bin[Idx1~Idx2) 為倍數 M1~M2, 下凹 CExp 次方
//                  Bin[Idx2~Idx3) 為倍數 M2
//--------------------------------------------------------------------------
VOID CreateMulCurve(UINT32 *pCurve,
					INT32 M1,      /* Qu26.6 */
					INT32 M2,      /* Qu26.6 */
					INT32 CExp,    /* 1~5 */
					INT32 Idx1,    /* 0~125 */
					INT32 Idx3)    /* 2~127 */
{
	//    CExp   =     1,    2,    3,    4,    5
	INT32 SBits[]  = {   31,   15,   10,    7,    6};   // 小數點位數: (Qu1.31)^1=Qu1.31, (Qu1.15)^2=Qu1.30, (Qu1.10)^3=Qu1.30, (Qu1.7)^4=Qu1.28, (Qu1.6)^5=Qu1.30
	INT32 ExpBits[] = { 31 - 6, 30 - 6, 30 - 6, 28 - 6, 30 - 6};
	INT32 CrvValue, CrvExp;
	INT32 S, K, Mul;

	INT32 Idx2 = 1000 * pCfg->FFT_SZ / pCfg->sampling_rate;  // 1kHz bin idx

	if (CExp < 1) {
		CExp = 1;
	}
	if (CExp > 5) {
		CExp = 5;
	}

	// Bin [0~Idx1)
	for (S = 0; S < Idx1; S++) {
		pCurve[S] = M1;
	}

	// Bin [Idx1~Idx2)
	CrvExp = 1;
	for (S = Idx2 - 1, Mul = 1; S >= Idx1; S--, Mul++) {
		CrvValue = Mul * (1 << (SBits[CExp - 1])) / (Idx2 - Idx1);
		CrvExp = 1;
		for (K = 0; K < CExp; K++) {
			CrvExp *= CrvValue;
		}
		CrvExp = (CrvExp + (1 << (ExpBits[CExp - 1] - 1))) >> ExpBits[CExp - 1];
		CrvExp = (CrvExp * (M1 - M2) + (1 << 5)) >> 6;
		pCurve[S] = CrvExp + M2;
	}

	// Bin [Idx2~Idx3]
	for (S = Idx2; S <= Idx3; S++) {
		pCurve[S] = M2;
	}


	return;
}


//--------------------------------------------------------------------------
//  StatisticDefault()
//  Description : 根據 pVar->QBias
//    Return :
//--------------------------------------------------------------------------
INT32 StatisticDefault(UINT32 bias, // Q26.6
					   UINT16 *pSpec,                  // Qu16
					   UINT32 EngG)                    // Qu23
{
	INT32  m, n, MaxGap, RDif, TSum, MSum;
	UINT32 Up, Down;
	UINT32 BiasStep[BIAS_CLIP] = {   72,   80,    88,  96,   104,  112,   120,  128};   //Qu26.6
	// Matlab:{1.125, 1.25, 1.375, 1.5, 1.625, 1.75, 1.875,    2}

	if (pDetect->SMode == NOISE_UPDATING) {
		// 計算 pVar->default_spec, pVar->default_eng
		if (pDetect->FCnt < FRMS_PER_SLICE) {
			Up   = pVar->default_bias + (64 / BIAS_CLIP / 2);  // bias 上限
			Down = pVar->default_bias - (64 / BIAS_CLIP / 2);  // bias 下限

			if ((bias <= Up) && (bias >= Down)) {
				for (m = pCfg->Valid_SIdx_Low; m <= pCfg->Valid_SIdx_High; m++) {
					// 1. 累加頻譜值
					//pVar->BG_Spec_HD[m] += (UINT32)pSpec[m];

					// 2. 頻譜最大值
					pVar->BG_Spec_HD[m] = pVar->BG_Spec_HD[m] > (UINT32)pSpec[m] ? pVar->BG_Spec_HD[m] : (UINT32)pSpec[m];
				}
				pDetect->EngG1 += EngG;    // 累加能量值, EngG1:Qu(6+23)
				pDetect->FCnt++;
			}
		} else {
			// FCnt == FRMS_PER_SLICE,
			for (m = pCfg->Valid_SIdx_Low; m <= pCfg->Valid_SIdx_High; m++) {
				// 1. 累加頻譜值
				//pVar->default_spec[m]   = (pVar->BG_Spec_HD[m]+32)>>6;        // Qu16, 計算 pVar->default_spec[]

				// 2. 頻譜最大值
				pVar->default_spec[m]   = (UINT16)pVar->BG_Spec_HD[m];        // Qu16, 計算 pVar->default_spec[]

				pVar->BG_Spec_HD[m]     = pVar->BG_Spec_HD[m] << 10;    // Qu16.10
				pVar->BG_Spec_Target[m] = pVar->default_spec[m];
			}
			pVar->default_eng = (pDetect->EngG1 + (FRMS_PER_SLICE >> 1)) >> FRMS_PER_SLICE_EXP;     // 計算 pVar->default_eng:Qu23
			CB_Filter(pVar->default_spec, pVar->Default_CB, pCfg->sampling_rate, pCfg->CB_Len, 0, 1);    // StatisticDefault(), 根據 default_spec 計算 Default_CB

			// Reset
			pDetect->FCnt = 0;

			// raise SMode flag
			pDetect->SMode = NOISE_UPDATED;
#if 0
			printf("\n\n ------Noise Updated------");
			printf("\n  unsigned short default_spec[]={");
			for (m = 0; m < pCfg->SPEC_LEN - 1; m++) {
				printf("%d,", pVar->default_spec[m]);
			}
			printf("%d };", pVar->default_spec[m]);
			printf("\n  sANR->max_bias      = %d", pVar->max_bias);
			printf("\n  sANR->default_bias = %d", pVar->default_bias);
			printf("\n  sANR->default_eng  = %d", pVar->default_eng);
#endif
		}
	} else {
		// (SMode==NOISE_DETECTING)或(SMode==NOISE_UPDATED)
		// 1.偵測 Bias 準位

		// 統計 1 SLICE
		if (pDetect->FCnt < FRMS_PER_SLICE) {
			// 統計 Bias, 累積在 Acc[]

			pDetect->SMode = NOISE_DETECTING;

			for (m = 0; m < BIAS_CLIP; m++) { // 統計 bias
				if (bias <= BiasStep[m]) {
					pDetect->Acc[m] = pDetect->Acc[m] + 1;
					pDetect->FCnt++;        // 必須累積滿 64 個
					break;
				}
			}
			//pVar->max_bias = pVar->max_bias > bias ? pVar->max_bias : bias;    // Qu26.6
		} else {
			// FCnt == FRMS_PER_SLICE, Acc[]統計完成 -> 尋找高低點 -> 放入 RepeatHst[], RepeatHstLow[]
			pDetect->FCnt = 0;   // 重新累積

			// 由小到大，尋找第一個夠大的 bias peak
			MaxGap = FRMS_PER_SLICE / BIAS_CLIP;  // 64/8=8
			for (m = 1; m < BIAS_CLIP - 1; m++) {
				if ((pDetect->Acc[m] >= pDetect->Acc[m - 1]) && (pDetect->Acc[m] >= pDetect->Acc[m + 1]) && (pDetect->Acc[m] > MaxGap)) {
					RDif = pDetect->RepeatIdx > m ? (pDetect->RepeatIdx - m) : (m - pDetect->RepeatIdx);
					if (RDif > 1) {
						for (n = 0; n < BIAS_CLIP; n++) { // 與上一次的 Bias Peak Index 距離太遠，則重新累積
							pDetect->RepeatHst[n] = 0;
							pDetect->RepeatHstLow[n] = 0;
						}

						pDetect->RepeatHstSum = 0;
						pDetect->RepeatIdx = m;
					}
					pDetect->RepeatHst[m]++;        // 左數第一極大值存入 RepeatHst[]
					pDetect->RepeatHstSum++;

					// 找出右相鄰的低點索引值
					for (n = m + 1; n < BIAS_CLIP - 1; n++) {
						if (pDetect->Acc[n] <= MaxGap) {
							break;
						}
					}
					pDetect->RepeatHstLow[n] = pDetect->RepeatHstLow[n] - 1;

					break;
				}
			}

			// 重設 ACC[]，重新統計新 bias / Time Slice
			for (m = 0; m < BIAS_CLIP; m++) {
				pDetect->Acc[m] = 0;
			}

			// RepeatHst[],RepeatHstLow[]已經累積了
			if (pDetect->RepeatHstSum == pCfg->noise_est_hold_time) {
				pDetect->SMode = NOISE_UPDATING;

				// 計算平均值作為 pVar->default_bias
				TSum = 0;
				MSum = 0;
				for (m = 1; m < BIAS_CLIP - 1; m++) { // peak 只存在 1~BIAS_CLIP-2 範圍
					if (pDetect->RepeatHst[m] > 0) {
						TSum = TSum + pDetect->RepeatHst[m] * m;
						MSum = MSum + pDetect->RepeatHst[m];
					}
				}
				pVar->default_bias = 72 + (TSum << 3) / MSum;  // Qu10.6, 找到 pVar->default_bias
				// Base{72} + ((TSum{1~6})>>3)/MSum
				// 介於 1.25~1.875 (80~120)

//printf("\n pVar->default_bias=%d",pVar->default_bias);
				// 計算 pVar->max_bias
				TSum = 0;
				MSum = 0;
				for (m = 2; m < BIAS_CLIP; m++) { // peak 右低點只存在 2~BIAS_CLIP-1 範圍
					if (pDetect->RepeatHstLow[m] < 0) {
						TSum = TSum - pDetect->RepeatHstLow[m] * m;
						MSum = MSum - pDetect->RepeatHstLow[m];
					}
				}
				pVar->max_bias = 72 + (TSum << 3) / MSum;  // Qu26.6
				// Base{72} + ((TSum{0~7})>>3)/MSum
//printf("\n pVar->max_bias=%d",pVar->max_bias);
				// 設定 pVar->max_bias 上限
				if (pVar->max_bias > pCfg->max_bias_limit) {  // Qu26.6 > Qu3.6
					pVar->max_bias = pCfg->max_bias_limit;         // 找到 pVar->max_bias
				}

				// 清空 pVar->BG_Spec_HD, 準備尋找背景頻譜值
				for (m = pCfg->Valid_SIdx_Low; m <= pCfg->Valid_SIdx_High; m++) {
					pVar->BG_Spec_HD[m] = 0;
				}

				pDetect->EngG1 = 0;
				pDetect->FCnt = 0;
#if 0
				printf("\n\n 1.Noise Detecting...");
				printf("\n RepeatHst   =");
				for (m = 0; m < BIAS_CLIP; m++) {
					printf("%d,", pDetect->RepeatHst[m]);
				}
				printf("\n RepeatHstLow=");
				for (m = 0; m < BIAS_CLIP; m++) {
					printf("%d,", pDetect->RepeatHstLow[m]);
				}
				printf("\n(pVar->max_bias, pVar->default_bias, pVar->HH, pVar->LL)=(%5d, %5d, %5d, %5d)", pVar->max_bias, pVar->default_bias, pVar->HH, pVar->LL);
				printf("\n(pVar->max_bias, pVar->default_bias, pVar->HH, pVar->LL)=(%f, %f, %f, %f)", pVar->max_bias / 64., pVar->default_bias / 64., pVar->HH / 64., pVar->LL / 64.);
#endif
			}
		}
	}

	return pDetect->SMode;
}

//--------------------------------------------------------------------------
//  Setup_BG_Vars()
//--------------------------------------------------------------------------
VOID Setup_BG_Vars(VOID)
{
	INT32 i, j;
	UINT16 *pus1;

	// 找 Pink 頻譜
	PeakTrim(pVar->default_spec, pVar->Default_CB, pNPink->Spec_Target, pCfg->HPF_CutOffBin);   // Setup_BG_Vars(), 根據 CB 找 Pink

	// 計算 Pink Structure 內容
	CB_Filter(pNPink->Spec_Target, pNPink->Spec_Target_CB, pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);
	pNPink->Spec_Target_CBWC  = WeightCenter(pNPink->Spec_Target_CB, pCfg->CB_Len);
	pNPink->Spec_Target_CBEng = EngSum(pNPink->Spec_Target_CB, pCfg->CB_Len);
	pNPink->TIdx = NPINK_QLEN - 1;
	for (i = 0; i < NPINK_QLEN; i++) {
		pus1 = pNPink->Spec[i];
		for (j = 0; j < pCfg->SPEC_LEN; j++) {
			pus1[j] = pNPink->Spec_Target[j];
		}

		pus1 = pNPink->CB[i];
		for (j = 0; j < pCfg->CB_Len; j++) {
			pus1[j] = pNPink->Spec_Target_CB[j];
		}

		pNPink->CBWC[i]  = pNPink->Spec_Target_CBWC;
		pNPink->CBEng[i] = pNPink->Spec_Target_CBEng;
		pNPink->Time[i]  = 0;
	}


	// 找 Color 頻譜
	pNColor->TIdx = NCOLOR_QLEN - 1;

	for (i = 0; i < pCfg->SPEC_LEN; i++) {
		pNColor->Spec_Target[i] = pVar->default_spec[i] - pNPink->Spec_Target[i];
	}

	// 計算 NColor Structure 內容
	CB_Filter(pNColor->Spec_Target, pNColor->Spec_Target_CB, pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN);
	pNColor->Spec_Target_CBWC  = WeightCenter(pNColor->Spec_Target_CB, pCfg->CB_Len);
	pNColor->Spec_Target_CBEng = EngSum(pNColor->Spec_Target_CB, pCfg->CB_Len);
	for (i = 0; i < NCOLOR_QLEN; i++) {
		pus1 = pNColor->Spec[i];
		for (j = 0; j < pCfg->SPEC_LEN; j++) {
			pus1[j] = pNColor->Spec_Target[j];
		}

		pus1 = pNColor->CB[i];
		for (j = 0; j < pCfg->CB_Len; j++) {
			pus1[j] = pNColor->Spec_Target_CB[j];
		}

		pNColor->CBWC[i]  = pNColor->Spec_Target_CBWC;
		pNColor->CBEng[i] = pNColor->Spec_Target_CBEng;
		pNColor->Time[i]  = pNColor->Max_Update_Interval;
		pNColor->GroupId[i] = 1;      // 0為強制偵測點, [1~NCOLOR_QLEN]為噪音種類
		pNColor->NoiseTag[i] = 1;     // 0:Signal, 1:Noise
	}
	pNColor->GroupNum = 1;
	pNColor->GroupCnt[1] = NCOLOR_QLEN;    // 為了與 Matlab 對應, 只定義 [1 ~ NCOLOR_QLEN], [0]不使用

	return;
}

//--------------------------------------------------------------------------
//  Setup_Inner_Vars()
//--------------------------------------------------------------------------
VOID Setup_Inner_Vars(VOID)
{
	INT32 i, k;
	UINT16 *pS1;

	pVar->BG_Bias_HD = (pVar->default_bias << 10);  // Qu10.16 = (Qu10.6 << 10);
	pVar->BG_Bias_Target  = pVar->default_bias;

	// 計算 pVar->HH, pVar->LL
	CalculateHHLL();
	// 計算 pVar->MCurveHigh, pVar->MCurveLow
	pVar->MCurveN1 = (64 * (8-pCfg->m_curve_n1_level) + (pVar->HH * pCfg->m_curve_n1_level) + (1<<2))>>3;
	pVar->MCurveN2 = (64 * (8-pCfg->m_curve_n2_level) + (pVar->LL * pCfg->m_curve_n2_level) + (1<<2))>>3;

	// 計算 pVar->BG_Spec_HD, pVar->BG_Spec_Target
	for (i = pCfg->Valid_SIdx_Low; i <= pCfg->Valid_SIdx_High; i++) {
		pVar->BG_Spec_HD[i]     = ((UINT32)(pVar->default_spec[i]) << 10);        // Qu16.10
		pVar->BG_Spec_Target[i] = pVar->default_spec[i];              // Qu16
	}

	// 計算 pVar->Default_CB
	CB_Filter(pVar->default_spec, pVar->Default_CB, pCfg->sampling_rate, pCfg->CB_Len, OPTION_CB_SEPARATED, OPTION_CB_MEAN); // StatisticDefault(), 根據 default_spec 計算 Default_CB

	// 設定 Queue 內部預設值
	for (i = 0; i < pCfg->Q_Center; i++) {
		pVar->QBias[i] = pVar->default_bias;
		pVar->QEngG[i] = pVar->default_eng;
		pS1 = pQu->pQCB[i];
		for (k = 0; k < pCfg->CB_Len; k++) {
			pS1[k] = pVar->Default_CB[k];       // NR_Detect(), 設定預設值 pQu->pQCB[0~4]
		}
	}
	for (i = pCfg->Q_Center; i < pCfg->Q_Len; i++) {
		pVar->QBias[i] = 0;       // clear [pVar->QBias]
		pVar->QEngG[i] = 0;       // clear [pVar->QEngG]
		pS1 = pQu->pQCB[i];         // clear [pQu->pQCB]
		for (k = 0; k < pCfg->CB_Len; k++) {
			pS1[k] = 0;
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  _ANR_detect()
//    Description: find default value of pVar->max_bias, pVar->default_bias, pVar->HH, pVar->LL, pVar->default_spec, pVar->default_eng
//  Return:
//        1. NOISE_DETECTING 時期,開始統計
//            pVar->default_bias
//            pVar->max_bias
//            pVar->HH, pVar->LL
//        2. NOISE_UPDATING 時期,更新以下預設值
//            pVar->default_spec
//            pVar->default_eng
//        3. NOISE_UPDATED ,預設值偵測完成
//--------------------------------------------------------------------------
INT32 _ANR_detect(struct ANR_CONFIG *ptANR)
{
	INT32 i, k, ret;
	struct kfft_data *pFFTout;

#if 0
	INT32 kk = -1;
#endif
	//    --- input sine wave ---
	UINT32 Bias_1, Bias_2;  // Qu26.6
//    for (i=0;i<pCfg->FFT_SZ;i++)
//        pVar->asFFT0[i] = Signal[i];

#if 0
	kk++;
	tPCM[kk].psBuf = (short *)(pVar->asFFT0);
	file_write_bin(&tPCM[kk], "..//__y_c2_VC.pcm", pCfg->FFT_SZ * 4);
#endif

	pVar->InFrmCnt++;
//for (k=0;k<pCfg->FFT_SZ;k++)   printf("\n %3d, %12d", k, pVar->asFFT0[k].r);

	time0(Windowing(pVar->asFFT0));   //_ANR_detect()
//for (k=0;k<pCfg->FFT_SZ;k++)   printf("\n %3d(%12d,%12d)", k, pVar->asFFT0[k].r, pVar->asFFT0[k].i);
#if 0
	kk++;
	tPCM[kk].psBuf = (short *)(pVar->asFFT0);
	file_write_bin(&tPCM[kk], "..//__Hanning.pcm", pCfg->FFT_SZ * 4);
#endif

	time0(pFFTout = GetFFToutSlot());

	time0(_kiss_fft(pKFFT_Cfg0, pVar->asFFT0, pFFTout));
//for (k=0;k<pCfg->FFT_SZ;k++)   printf("\n %3d(%12d,%12d)", k, pFFTout[k].r, pFFTout[k].i);
#if 0
	kk++;
	tPCM[kk].psBuf = (short *)(pFFTout);
	file_write_bin(&tPCM[kk], "..//__asFFTout.pcm", pCfg->FFT_SZ * 4);
#endif

	time0(PolarCoordinate(pFFTout, pVar->asNRabs, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High));
//for (k=0;k<pCfg->SPEC_LEN;k++)   printf("\n %3d %12d", k, pVar->asNRabs[k]);
#if 0
	kk++;
	tPCM[kk].psBuf = (INT16 *)(pVar->asNRabs);
	file_write_bin(&tPCM[kk], "..//__asNRabs.pcm", pCfg->SPEC_LEN);
#endif

	time0(PushInQueueAndBlur(pVar->asNRabs, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High));
//printf("\n pQu->pQabs,pQu->QIdx(%d)",pQu->QIdx);   for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQabs[i], 2, 6);
//printf("\n pQu->pQabs_blur_tmp");        for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQabs_blur_tmp[i], 4, 6);
//printf("\n pQu->pQabs_blur");            for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQabs_blur[i], 2, 6);

#if 0
	kk = 3;
	tPCM[kk].psBuf = (INT16 *)(pQu->pQabs[pQu->QIdx == 10 ? 9 : pQu->QIdx]);
	file_write_bin(&tPCM[kk], "..//__pQu->pQabs.pcm", 129, 1, 1);
	//kk=4;
	//tPCM[kk].psBuf=(INT16 *)(pQu->pQabs_blur_tmp[pQu->QIdx == 10 ? 9 : pQu->QIdx]);
	//file_write_bin(&tPCM[kk++], "..//__pQu->pQabs_blur_tmp.pcm", 129, 1, 1);
	if (pQu->QIdx != 0) {
		if (pQu->QIdx != 10) {
			kk = 5;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQabs_blur[pQu->QIdx - 1]);
			file_write_bin(&tPCM[kk], "..//__pQu->pQabs_blur.pcm", 129, 1, 1);
		} else {
			kk = 5;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQabs_blur[8]);
			file_write_bin(&tPCM[kk], "..//__pQu->pQabs_blur.pcm", 129, 1, 1);
		}
	}
#endif

	time0(Bias_1 = Unbalance(pQu->pQabs, pVar->pQabs_GSum_G, pCfg->spec_bias_low, pCfg->spec_bias_high));    // Qu26.6
	time0(Bias_2 = Unbalance(pQu->pQabs_blur, pVar->pQabs_blur_GSum_G, pCfg->spec_bias_low, pCfg->spec_bias_high));    // Qu26.6
// printf("\n pVar->pQu->pQabs_GSum_G");        for (i=0;i<4;i++)        DumpInt32(pVar->pQu->pQabs_GSum_G[i], 10, 0);
// printf("\n pVar->pQu->pQabs_blur_GSum_G");        for (i=0;i<4;i++)        DumpInt32(pVar->pQu->pQabs_blur_GSum_G[i], 10, 0);
// printf("\n pQu->QIdx(%2d) Bias_1(%8d), Bias_2(%8d)", pQu->QIdx, Bias_1, Bias_2);

#if 0
	if (pQu->QIdx > 4) {
		kk = 6;
		tPCM[kk].psBuf = (INT16 *)(&Bias_1);
		file_write_bin(&tPCM[kk], "..//__Bias_1.pcm", 2, 1, 1);
		kk = 7;
		tPCM[kk].psBuf = (INT16 *)(&Bias_2);
		file_write_bin(&tPCM[kk], "..//__Bias_2.pcm", 2, 1, 1);
	}
#endif

	if (pQu->QIdx == pCfg->Q_Len) {
		Shift_Array((VOID *)pVar->QBias, 0, pCfg->Q_Center, 4);
	}
	pVar->QBias[pCfg->Q_Center] = Bias_1 > Bias_2 ? Bias_1 : Bias_2;

	//if (pQu->QIdx >= 5)
	//{
	//    Shift_Array((VOID *)pVar->QBias, 0, pCfg->Q_Len-1, 4);
	//}
	//pVar->QBias[4] = Bias_1 > Bias_2 ? Bias_1 : Bias_2;

	CB_Eng_Fun(pQu->pQabs_blur, pQu->pQCB, &pVar->QEngG[0]);
//printf("\n pQu->pQCB");   for (i=0;i<pCfg->Q_Len;i++)        Dump_Data_UInt(pQu->pQCB[i], 2, 6);
#if 0
	if (pQu->QIdx >= 2) {
		if (pQu->QIdx != pCfg->Q_Len) {
			kk = 8;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQCB[pQu->QIdx - 1]);
			file_write_bin(&tPCM[kk], "..//__pQu->pQCB.pcm", 129, 1, 1);
			kk = 9;
			tPCM[kk].psBuf = (INT16 *)(&pVar->QEngG[pQu->QIdx - 1]);
			file_write_bin(&tPCM[kk], "..//__QEngG.pcm", 2, 1, 1);
		} else {
			kk = 8;
			tPCM[kk].psBuf = (INT16 *)(pQu->pQCB[8]);
			file_write_bin(&tPCM[kk], "..//__pQu->pQCB.pcm", 129, 1, 1);
			kk = 9;
			tPCM[kk].psBuf = (INT16 *)(&pVar->QEngG[8]);
			file_write_bin(&tPCM[kk], "..//__QEngG.pcm", 2, 1, 1);
		}
	}
#endif

	ret = NOISE_DETECTING;
	if (pQu->QIdx >= pCfg->Q_Len - 1) {
		ret = StatisticDefault(pVar->QBias[pCfg->Q_Center], pQu->pQabs_blur[pCfg->Q_Center], pVar->QEngG[pCfg->Q_Center]);
	}

	if (ret == NOISE_UPDATED) {
		// 回傳 AP 層
		ptANR->default_bias = pVar->default_bias;      // 回傳 AP 層
		ptANR->default_eng  = pVar->default_eng;       // 回傳 AP 層
		ptANR->max_bias      = pVar->max_bias;           // 回傳 AP 層
		for (k = 0; k < pCfg->SPEC_LEN; k++) {
			ptANR->default_spec[k] = pVar->default_spec[k];    // 回傳 AP 層
		}

		// 根據 Default 值, 設定內部變數
		Setup_Inner_Vars();

		// 根據 Default 值, 計算 pVar->Sig_MCurve
		CreateMulCurve(pVar->Sig_MCurve, pVar->HH, pVar->LL, 2, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High);
		//CreateMulCurve(pVar->Sig_MCurve, 64, 64, 2, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High);

		// 根據 Default 值, 設定 NPink, pNColor->..etc 相關變數
		Setup_BG_Vars();

		// 清除變數和 Buffer
		pVar->InFrmCnt = 0;
		pQu->QIdx = 0;

		for (i = 0; i < pCfg->Q_Len; i++) {
			UINT32 *pI1 = pQu->pQabs_blur_tmp[i];         // clear [pQu->pQabs_blur_tmp]
			UINT16 *pS2 = pQu->pQabs[i];
			for (k = 0; k < pCfg->SPEC_LEN; k++) {
				pI1[k] = 0;
				pS2[k] = 0;
			}
		}

		for (i = 0; i < pCfg->Q_Len - 1; i++) {
			UINT16 *pS1 = pQu->pQabs_blur[i];             // clear [pQu->pQabs_blur]
			for (k = 0; k < pCfg->SPEC_LEN; k++) {
				pS1[k] = 0;
			}
		}

		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			pVar->asBufPadIn[i] = 0;  // clear [pVar->asBufPadIn]
		}
	} else { // (ret==NOISE_DETECTING)或(ret==NOISE_UPDATING), 偵測中或更新背景中
		if (pQu->QIdx < pCfg->Q_Len) {
			pQu->QIdx++;
		}
	}

	return ret;
}

//--------------------------------------------------------------------------
//  Mem_Dispatch()
//--------------------------------------------------------------------------
void Mem_Dispatch(int inkey, struct ANR_CONFIG *pANR)
{
	Mem_Info.pST_CFG_MEM    = (void *)((UINT32)pANR->p_mem_buffer);
	Mem_Info.pST_VAR_MEM    = (void *)((UINT32)Mem_Info.pST_CFG_MEM    + Mem_Info.ST_CFG_MEM);
	Mem_Info.pST_NPINK_MEM  = (void *)((UINT32)Mem_Info.pST_VAR_MEM    + Mem_Info.ST_VAR_MEM);
	Mem_Info.pST_NCOLOR_MEM = (void *)((UINT32)Mem_Info.pST_NPINK_MEM  + Mem_Info.ST_NPINK_MEM);
	Mem_Info.pST_NTONE_MEM  = (void *)((UINT32)Mem_Info.pST_NCOLOR_MEM + Mem_Info.ST_NCOLOR_MEM);
	Mem_Info.pST_LINK_MEM   = (void *)((UINT32)Mem_Info.pST_NTONE_MEM  + Mem_Info.ST_NTONE_MEM);
	Mem_Info.pST_QU_MEM     = (void *)((UINT32)Mem_Info.pST_LINK_MEM   + Mem_Info.ST_LINK_MEM);
	Mem_Info.pKFFT_MEM      = (void *)((UINT32)Mem_Info.pST_QU_MEM     + Mem_Info.ST_QU_MEM);
	Mem_Info.pVAR_MEM       = (void *)((UINT32)Mem_Info.pKFFT_MEM      + Mem_Info.KFFT_MEM);
	Mem_Info.pNPINK_MEM     = (void *)((UINT32)Mem_Info.pVAR_MEM       + Mem_Info.VAR_MEM);
	Mem_Info.pNCOLOR_MEM    = (void *)((UINT32)Mem_Info.pNPINK_MEM     + Mem_Info.NPINK_MEM);
	Mem_Info.pNTONE_MEM     = (void *)((UINT32)Mem_Info.pNCOLOR_MEM    + Mem_Info.NCOLOR_MEM);
	Mem_Info.pQBASE_MEM     = (void *)((UINT32)Mem_Info.pNTONE_MEM     + Mem_Info.NTONE_MEM);
	Mem_Info.pLINK_MEM      = (void *)((UINT32)Mem_Info.pQBASE_MEM     + Mem_Info.QBASE_MEM);
	Mem_Info.pTMP_MEM       = (void *)((UINT32)Mem_Info.pLINK_MEM      + Mem_Info.LINK_MEM);
	Mem_Info.pQMARK_MEM     = (void *)((UINT32)Mem_Info.pTMP_MEM       + Mem_Info.TMP_MEM);

	pCfg_Stream[inkey] = Mem_Info.pST_CFG_MEM;

	Var_Stream[inkey].pVar_L       = Mem_Info.pST_VAR_MEM;
	NPink_Stream[inkey].pNPink_L   = Mem_Info.pST_NPINK_MEM;
	NColor_Stream[inkey].pNColor_L = Mem_Info.pST_NCOLOR_MEM;
	NTone_Stream[inkey].pNTone_L   = Mem_Info.pST_NTONE_MEM;
	Link_Stream[inkey].pLink_L     = Mem_Info.pST_LINK_MEM;
	Qu_Stream[inkey].pQu_L         = Mem_Info.pST_QU_MEM;

	if (pANR->stereo) {
		Var_Stream[inkey].pVar_R       = (struct _Var *)((UINT32)Var_Stream[inkey].pVar_L       + (Mem_Info.ST_VAR_MEM   >> 1));
		NPink_Stream[inkey].pNPink_R   = (struct _NPink *)((UINT32)NPink_Stream[inkey].pNPink_L   + (Mem_Info.ST_NPINK_MEM >> 1));
		NColor_Stream[inkey].pNColor_R = (struct _NColor *)((UINT32)NColor_Stream[inkey].pNColor_L + (Mem_Info.ST_NCOLOR_MEM >> 1));
		NTone_Stream[inkey].pNTone_R   = (struct _NTone *)((UINT32)NTone_Stream[inkey].pNTone_L   + (Mem_Info.ST_NTONE_MEM >> 1));
		Link_Stream[inkey].pLink_R     = (struct _Link *)((UINT32)Link_Stream[inkey].pLink_L     + (Mem_Info.ST_LINK_MEM  >> 1));
		Qu_Stream[inkey].pQu_R         = (struct _Qu *)((UINT32)Qu_Stream[inkey].pQu_L         + (Mem_Info.ST_QU_MEM    >> 1));
	} else {
		Var_Stream[inkey].pVar_R       = NULL;
		NPink_Stream[inkey].pNPink_R   = NULL;
		NColor_Stream[inkey].pNColor_R = NULL;
		NTone_Stream[inkey].pNTone_R   = NULL;
		Link_Stream[inkey].pLink_R     = NULL;
		Qu_Stream[inkey].pQu_R         = NULL;
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_Structure()
//--------------------------------------------------------------------------
void Mem_Structure(int key, struct _Cfg *pCfgTmp, int Opt)
{
	int stereo = pCfgTmp->stereo;

	Mem_Info.ST_CFG_MEM    = ALIGN4(sizeof(struct _Cfg));
	Mem_Info.ST_VAR_MEM    = ALIGN4(sizeof(struct _Var))   << stereo;
	Mem_Info.ST_NPINK_MEM  = ALIGN4(sizeof(struct _NPink)) << stereo;
	Mem_Info.ST_NCOLOR_MEM = ALIGN4(sizeof(struct _NColor)) << stereo;
	Mem_Info.ST_NTONE_MEM  = ALIGN4(sizeof(struct _NTone)) << stereo;
	Mem_Info.ST_LINK_MEM   = ALIGN4(sizeof(struct _Link))  << stereo;
	Mem_Info.ST_QU_MEM     = ALIGN4(sizeof(struct _Qu))    << stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pST_CFG_MEM    = malloc(Mem_Info.ST_CFG_MEM);    // 請用calloc(not malloc)清除記憶體內容,不然結果會錯
	Mem_Info.pST_VAR_MEM    = malloc(Mem_Info.ST_VAR_MEM);
	Mem_Info.pST_NPINK_MEM  = malloc(Mem_Info.ST_NPINK_MEM);
	Mem_Info.pST_NCOLOR_MEM = malloc(Mem_Info.ST_NCOLOR_MEM);
	Mem_Info.pST_NTONE_MEM  = malloc(Mem_Info.ST_NTONE_MEM);
	Mem_Info.pST_LINK_MEM   = malloc(Mem_Info.ST_LINK_MEM);
	Mem_Info.pST_QU_MEM     = malloc(Mem_Info.ST_QU_MEM);
	memset(Mem_Info.pST_CFG_MEM, 0, Mem_Info.ST_CFG_MEM);
	memset(Mem_Info.pST_VAR_MEM, 0, Mem_Info.ST_VAR_MEM);
	memset(Mem_Info.pST_NPINK_MEM, 0, Mem_Info.ST_NPINK_MEM);
	memset(Mem_Info.pST_NCOLOR_MEM, 0, Mem_Info.ST_NCOLOR_MEM);
	memset(Mem_Info.pST_NTONE_MEM, 0, Mem_Info.ST_NTONE_MEM);
	memset(Mem_Info.pST_LINK_MEM, 0, Mem_Info.ST_LINK_MEM);
	memset(Mem_Info.pST_QU_MEM, 0, Mem_Info.ST_QU_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		pCfg_Stream[key]             = (struct _Cfg *)Mem_Info.pST_CFG_MEM;

		Var_Stream[key].pVar_L       = (struct _Var *)Mem_Info.pST_VAR_MEM;
		NPink_Stream[key].pNPink_L   = (struct _NPink *)Mem_Info.pST_NPINK_MEM;
		NColor_Stream[key].pNColor_L = (struct _NColor *)Mem_Info.pST_NCOLOR_MEM;
		NTone_Stream[key].pNTone_L   = (struct _NTone *)Mem_Info.pST_NTONE_MEM;
		Link_Stream[key].pLink_L     = (struct _Link *)Mem_Info.pST_LINK_MEM;
		Qu_Stream[key].pQu_L         = (struct _Qu *)Mem_Info.pST_QU_MEM;

		if (stereo == 1) {
			Var_Stream[key].pVar_R       = (struct _Var *)((UINT32)Mem_Info.pST_VAR_MEM    + (Mem_Info.ST_VAR_MEM    >> stereo));
			NPink_Stream[key].pNPink_R   = (struct _NPink *)((UINT32)Mem_Info.pST_NPINK_MEM  + (Mem_Info.ST_NPINK_MEM  >> stereo));
			NColor_Stream[key].pNColor_R = (struct _NColor *)((UINT32)Mem_Info.pST_NCOLOR_MEM + (Mem_Info.ST_NCOLOR_MEM >> stereo));
			NTone_Stream[key].pNTone_R   = (struct _NTone *)((UINT32)Mem_Info.pST_NTONE_MEM  + (Mem_Info.ST_NTONE_MEM  >> stereo));
			Link_Stream[key].pLink_R     = (struct _Link *)((UINT32)Mem_Info.pST_LINK_MEM   + (Mem_Info.ST_LINK_MEM   >> stereo));
			Qu_Stream[key].pQu_R         = (struct _Qu *)((UINT32)Mem_Info.pST_QU_MEM     + (Mem_Info.ST_QU_MEM     >> stereo));
		} else {
			// mono
			Var_Stream[key].pVar_R       = NULL;
			NPink_Stream[key].pNPink_R   = NULL;
			NColor_Stream[key].pNColor_R = NULL;
			NTone_Stream[key].pNTone_R   = NULL;
			Link_Stream[key].pLink_R     = NULL;
			Qu_Stream[key].pQu_R         = NULL;
		}
	}

	return;
}


//--------------------------------------------------------------------------
//  Mem_kfft()
//--------------------------------------------------------------------------
void Mem_kfft(int key, struct _Cfg *pCfgTmp, int Opt)
{
	Mem_Info.KFFT_MEM = 2 * (sizeof(struct kfft_info) + sizeof(struct kfft_data_16) * (pCfgTmp->FFT_SZ - 1)); /* twiddle factors */

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pKFFT_MEM = malloc(Mem_Info.KFFT_MEM);    // 請用calloc(not malloc)清除記憶體內容,不然結果會錯
	memset(Mem_Info.pKFFT_MEM, 0, Mem_Info.KFFT_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {

		KFFT_Cfg0_Stream[key] = (struct kfft_info *)Mem_Info.pKFFT_MEM;
		KFFT_Cfg1_Stream[key] = (struct kfft_info *)((UINT32)Mem_Info.pKFFT_MEM + (Mem_Info.KFFT_MEM >> 1));

		kfft_alloc(KFFT_Cfg0_Stream[key], pCfg->FFT_SZ, 0);
		kfft_alloc(KFFT_Cfg1_Stream[key], pCfg->FFT_SZ, 1);
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_Var()
//--------------------------------------------------------------------------
void Mem_Var(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	INT32 TSize[13] = {0};

	TSize[ 0] =         0 + ALIGN4(pCfgTmp->FFT_SZ) * sizeof(struct kfft_data);     // struct kfft_data * asFFT0
	TSize[ 1] = TSize[ 0] + ALIGN4(pCfgTmp->FFT_SZ) * sizeof(struct kfft_data);     // struct kfft_data * asFFT1

	TSize[ 2] = TSize[ 1] + ALIGN4(pCfgTmp->CB_Len) * sizeof(UINT16);    // UINT16 * Default_CB     Qu16
	TSize[ 3] = TSize[ 2] + ALIGN4(pCfgTmp->FFT_OV_SZ) * sizeof(INT16);  // INT16 * asNRout
	TSize[ 4] = TSize[ 3] + ALIGN4(pCfgTmp->FFT_OV_SZ) * sizeof(INT32);  // INT32 * asNRtail
	TSize[ 5] = TSize[ 4] + ALIGN4(pCfgTmp->FFT_OV_SZ) * sizeof(INT16);  // INT16 * asBufPadIn     前一個禎的後面 128 個 sample
	TSize[ 6] = TSize[ 5] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT32);  // UINT32 * BG_Spec_HD     Qu16.10
	TSize[ 7] = TSize[ 6] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);  // UINT16 * BG_Spec_Target Qu16
	TSize[ 8] = TSize[ 7] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);  // UINT16 * default_spec   Qu16
	TSize[ 9] = TSize[ 8] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);  // UINT16 * asNRabs        Qu16
	TSize[10] = TSize[ 9] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(INT16);   // INT16  * Mark_Smooth    0~10
	TSize[11] = TSize[10] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);  // UINT16 * BGboost        Qu16
	TSize[12] = TSize[11] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT32);  // UINT32 * Sig_MCurve

	Mem_Info.VAR_MEM = TSize[12] << pCfgTmp->stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pVAR_MEM = malloc(Mem_Info.VAR_MEM);
	memset((void *)Mem_Info.pVAR_MEM, 0, Mem_Info.VAR_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		if (Channel == CHANNEL_L) {
			pVar->asFFT0 = (struct kfft_data *)(Mem_Info.pVAR_MEM);
		} else {
			pVar->asFFT0 = (struct kfft_data *)((UINT32)Mem_Info.pVAR_MEM + (Mem_Info.VAR_MEM >> 1));
		}

		pVar->asFFT1    = (struct kfft_data *)((UINT32)pVar->asFFT0 + TSize[ 0]);
		pVar->Default_CB         = (UINT16 *)((UINT32)pVar->asFFT0 + TSize[ 1]);
		pVar->asNRout            = (INT16 *)((UINT32)pVar->asFFT0 + TSize[ 2]);
		pVar->asNRtail           = (INT32 *)((UINT32)pVar->asFFT0 + TSize[ 3]);
		pVar->asBufPadIn         = (INT16 *)((UINT32)pVar->asFFT0 + TSize[ 4]);
		pVar->BG_Spec_HD         = (UINT32 *)((UINT32)pVar->asFFT0 + TSize[ 5]);
		pVar->BG_Spec_Target     = (UINT16 *)((UINT32)pVar->asFFT0 + TSize[ 6]);
		pVar->default_spec       = (UINT16 *)((UINT32)pVar->asFFT0 + TSize[ 7]);
		pVar->asNRabs            = (UINT16 *)((UINT32)pVar->asFFT0 + TSize[ 8]);
		pVar->Mark_Smooth        = (INT16 *)((UINT32)pVar->asFFT0 + TSize[ 9]);
		pVar->BGboost            = (UINT16 *)((UINT32)pVar->asFFT0 + TSize[10]);
		pVar->Sig_MCurve         = (UINT32 *)((UINT32)pVar->asFFT0 + TSize[11]);
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_NPink()
//--------------------------------------------------------------------------
void Mem_NPink(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	INT32 i, TSize[4];

	TSize[0] =       0  + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16) * NPINK_QLEN;  // UINT16 * Spec[NPINK_QLEN]
	TSize[1] = TSize[0] + ALIGN4(pCfgTmp->CB_Len) * sizeof(UINT16) * NPINK_QLEN;    // UINT16 * CB[NPINK_QLEN]
	TSize[2] = TSize[1] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);               // UINT16 * Spec_Target
	TSize[3] = TSize[2] + ALIGN4(pCfgTmp->CB_Len) * sizeof(UINT16);                 // UINT16 * Spec_Target_CB

	Mem_Info.NPINK_MEM = TSize[3] << pCfgTmp->stereo;

#ifdef   ALLOC_MEM_INSIDE
	Mem_Info.pNPINK_MEM = malloc(Mem_Info.NPINK_MEM);
	memset((void *)Mem_Info.pNPINK_MEM, 0, Mem_Info.NPINK_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		if (Channel == CHANNEL_L) {
			pNPink->Spec[0] = (UINT16 *)((UINT32)(Mem_Info.pNPINK_MEM));
		} else {
			pNPink->Spec[0] = (UINT16 *)((UINT32)(Mem_Info.pNPINK_MEM) + (Mem_Info.NPINK_MEM >> 1));
		}

		pNPink->CB[0]          = (UINT16 *)((UINT32)pNPink->Spec[0] + TSize[0]);
		pNPink->Spec_Target    = (UINT16 *)((UINT32)pNPink->Spec[0] + TSize[1]);
		pNPink->Spec_Target_CB = (UINT16 *)((UINT32)pNPink->Spec[0] + TSize[2]);

		for (i = 1; i < NPINK_QLEN; i++) {
			pNPink->Spec[i] = pNPink->Spec[i - 1] + ALIGN4(pCfg->SPEC_LEN);
			pNPink->CB[i]   = pNPink->CB  [i - 1] + ALIGN4(pCfg->CB_Len);
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_NColor()
//--------------------------------------------------------------------------
void Mem_NColor(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	INT32 i, TSize[6];

	TSize[0] =        0 + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16) * NCOLOR_QLEN;  // UINT16 * Spec[NCOLOR_QLEN]
	TSize[1] = TSize[0] + ALIGN4(pCfgTmp->CB_Len) * sizeof(UINT16) * NCOLOR_QLEN;    // UINT16 * CB[NCOLOR_QLEN]
	TSize[2] = TSize[1] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);                // UINT16 * Spec_Target
	TSize[3] = TSize[2] + ALIGN4(pCfgTmp->CB_Len) * sizeof(UINT16);                  // UINT16 * Spec_Target_CB
	TSize[4] = TSize[3] + ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);                // UINT16 * Tmp_Spec
	TSize[5] = TSize[4] + ALIGN4(pCfgTmp->CB_Len) * sizeof(UINT16);                  // UINT16 * Tmp_Spec_CB

	Mem_Info.NCOLOR_MEM = TSize[5] << pCfgTmp->stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pNCOLOR_MEM = malloc(Mem_Info.NCOLOR_MEM);
	memset((void *)Mem_Info.pNCOLOR_MEM, 0, Mem_Info.NCOLOR_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		if (Channel == CHANNEL_L) {
			pNColor->Spec[0] = (UINT16 *)((UINT32)(Mem_Info.pNCOLOR_MEM));
		} else {
			pNColor->Spec[0] = (UINT16 *)((UINT32)(Mem_Info.pNCOLOR_MEM) + (Mem_Info.NCOLOR_MEM >> 1));
		}

		pNColor->CB[0]          = (UINT16 *)((UINT32)pNColor->Spec[0] + TSize[0]);
		pNColor->Spec_Target    = (UINT16 *)((UINT32)pNColor->Spec[0] + TSize[1]);
		pNColor->Spec_Target_CB = (UINT16 *)((UINT32)pNColor->Spec[0] + TSize[2]);
		pNColor->Tmp_Spec       = (UINT16 *)((UINT32)pNColor->Spec[0] + TSize[3]);
		pNColor->Tmp_Spec_CB    = (UINT16 *)((UINT32)pNColor->Spec[0] + TSize[4]);

		for (i = 1; i < NCOLOR_QLEN; i++) {
			pNColor->Spec[i] = pNColor->Spec[i - 1] + ALIGN4(pCfg->SPEC_LEN);
			pNColor->CB[i]   = pNColor->CB  [i - 1] + ALIGN4(pCfg->CB_Len);
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_Tone()
//--------------------------------------------------------------------------
void Mem_Tone(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	int TSize = ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT16);  // UINT16 * Spec_Target

	Mem_Info.NTONE_MEM = TSize << pCfgTmp->stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pNTONE_MEM = (void *)malloc(Mem_Info.NTONE_MEM);
	memset((void *)Mem_Info.pNTONE_MEM, 0, Mem_Info.NTONE_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		if (Channel == CHANNEL_L) {
			pNTone->Spec_Target = (UINT16 *)Mem_Info.pNTONE_MEM;
		} else {
			pNTone->Spec_Target = (UINT16 *)((UINT32)Mem_Info.pNTONE_MEM + (Mem_Info.NTONE_MEM >> 1));
		}
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_Qu()
//--------------------------------------------------------------------------
void Mem_Qu(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	INT32 i, TSize[5];

	struct kfft_data      *pQFFT_base;
	UINT32    *pQabs_blur_tmp_base;
	UINT16    *pQabs_base;
	UINT16    *pQabs_blur_base;
	UINT16    *pQCB_base;

	TSize[0] =       0  + (pCfgTmp->Q_Len - pCfgTmp->Wing_L - 1) * ALIGN4(pCfgTmp->FFT_SZ) * sizeof(struct kfft_data); //struct kfft_data * pQFFT[MAX_Q_LEN];
	TSize[1] = TSize[0] + pCfgTmp->Q_Len     * ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(UINT32); // UINT32 * pQabs_blur_tmp[MAX_Q_LEN]
	TSize[2] = TSize[1] + pCfgTmp->Q_Len     * ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(INT16);  // UINT16 * pQabs[MAX_Q_LEN]
	TSize[3] = TSize[2] + pCfgTmp->Q_Len     * ALIGN4(pCfgTmp->CB_Len) * sizeof(INT16);    // UINT16 * pQCB[MAX_Q_LEN]
	TSize[4] = TSize[3] + (pCfgTmp->Q_Len - 1) * ALIGN4(pCfgTmp->SPEC_LEN) * sizeof(INT16); // UINT16 * pQabs_blur[MAX_Q_LEN]

	Mem_Info.QBASE_MEM = TSize[4] << pCfgTmp->stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pQBASE_MEM = malloc(Mem_Info.QBASE_MEM);
	memset((void *)Mem_Info.pQBASE_MEM, 0, Mem_Info.QBASE_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		if (Channel == CHANNEL_L) {
			pQFFT_base  = (struct kfft_data *)Mem_Info.pQBASE_MEM;
		} else {
			pQFFT_base  = (struct kfft_data *)((UINT32)Mem_Info.pQBASE_MEM + (Mem_Info.QBASE_MEM >> 1));
		}

		pQabs_blur_tmp_base = (UINT32 *)((UINT32)pQFFT_base + TSize[0]);
		pQabs_base          = (UINT16 *)((UINT32)pQFFT_base + TSize[1]);
		pQCB_base           = (UINT16 *)((UINT32)pQFFT_base + TSize[2]);
		pQabs_blur_base     = (UINT16 *)((UINT32)pQFFT_base + TSize[3]);

		for (i = pCfg->Q_Center; i < pCfg->Q_Len; i++) {
			pQu->pQFFT[i]            = pQFFT_base          + (i - pCfg->Q_Center) * pCfg->FFT_SZ;
		}

		for (i = 0; i < pCfg->Q_Len; i++) {
			pQu->pQabs_blur_tmp[i]   = pQabs_blur_tmp_base + i * ALIGN4(pCfg->SPEC_LEN);
			pQu->pQabs[i]            = pQabs_base          + i * ALIGN4(pCfg->SPEC_LEN);
			pQu->pQCB[i]             = pQCB_base           + i * ALIGN4(pCfg->CB_Len);
		}
		for (i = 0; i < pCfg->Q_Len - 1; i++) {
			pQu->pQabs_blur[i]       = pQabs_blur_base     + i * ALIGN4(pCfg->SPEC_LEN);
		}

	}

//printf("\nQueue calloc %d bytes.", alloc_byte);

	return;
}

//--------------------------------------------------------------------------
//    Mem_Link()
//--------------------------------------------------------------------------
void Mem_Link(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	int i, TSize;

	TSize = 2 * ALIGN4(pCfgTmp->Link_Nodes) * pCfgTmp->RLink_Len * sizeof(struct _BandEng) +
			4 * ALIGN4(pCfgTmp->Link_Nodes) * pCfgTmp->RLink_Len * sizeof(INT16);

	Mem_Info.LINK_MEM = TSize << pCfgTmp->stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pLINK_MEM = malloc(Mem_Info.LINK_MEM);
	memset((void *)Mem_Info.pLINK_MEM, 0, Mem_Info.LINK_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		struct _BandEng *pCLink_O_base;
		struct _BandEng *pRLink_O_base;
		INT16    *pCLink_S_base;    // (pCfg->Link_Nodes*RLink_Len, sizeof(INT16))
		INT16    *pRLink_S_base;    // (pCfg->Link_Nodes*RLink_Len, sizeof(INT16))
		INT16    *pCLink_E_base;    // (pCfg->Link_Nodes*RLink_Len, sizeof(INT16))
		INT16    *pRLink_E_base;    // (pCfg->Link_Nodes*RLink_Len, sizeof(INT16))

		if (Channel == CHANNEL_L) {
			pCLink_O_base = (struct _BandEng *)Mem_Info.pLINK_MEM;
		} else {
			pCLink_O_base = (struct _BandEng *)((UINT32)Mem_Info.pLINK_MEM + (Mem_Info.LINK_MEM >> 1));
		}

		pRLink_O_base = pCLink_O_base + ALIGN4(pCfg->Link_Nodes) * pCfg->RLink_Len;

		pCLink_S_base = (INT16 *)(pRLink_O_base + ALIGN4(pCfg->Link_Nodes) * pCfg->RLink_Len);
		pCLink_E_base = pCLink_S_base + ALIGN4(pCfg->Link_Nodes) * pCfg->RLink_Len;
		pRLink_S_base = pCLink_E_base + ALIGN4(pCfg->Link_Nodes) * pCfg->RLink_Len;
		pRLink_E_base = pRLink_S_base + ALIGN4(pCfg->Link_Nodes) * pCfg->RLink_Len;

		for (i = pCfg->Q_Center; i <= (pCfg->Q_Center + pCfg->Wing_R); i++) {
			pLink->pCLink_O[i] = pCLink_O_base + (i - pCfg->Q_Center) * ALIGN4(pCfg->Link_Nodes);
			pLink->pRLink_O[i] = pRLink_O_base + (i - pCfg->Q_Center) * ALIGN4(pCfg->Link_Nodes);

			pLink->pCLink_S[i] = pCLink_S_base + (i - pCfg->Q_Center) * ALIGN4(pCfg->Link_Nodes);
			pLink->pCLink_E[i] = pCLink_E_base + (i - pCfg->Q_Center) * ALIGN4(pCfg->Link_Nodes);
			pLink->pRLink_S[i] = pRLink_S_base + (i - pCfg->Q_Center) * ALIGN4(pCfg->Link_Nodes);
			pLink->pRLink_E[i] = pRLink_E_base + (i - pCfg->Q_Center) * ALIGN4(pCfg->Link_Nodes);
		}
	}

	//------------------------------------
	Mem_Info.TMP_MEM = 2 * ALIGN4(pCfgTmp->Link_Nodes) * sizeof(struct _BandEng) +
					   4 * ALIGN4(pCfgTmp->Link_Nodes) * sizeof(INT16);
#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pTMP_MEM = malloc(Mem_Info.TMP_MEM);
	memset((void *)Mem_Info.pTMP_MEM, 0, Mem_Info.TMP_MEM);
#endif
	if (Opt == OPTION_MEM_SET) {
		struct _BandEng      *pTmp_O_base;
		INT16   *pTmp_SE_base;

		pTmp_O_base  = (struct _BandEng *)Mem_Info.pTMP_MEM;
		pTmp_SE_base = (short *)((UINT32)Mem_Info.pTMP_MEM + 2 * ALIGN4(pCfgTmp->Link_Nodes) * sizeof(struct _BandEng));

		pLink->pTmp1_O = pTmp_O_base;
		pLink->pTmp2_O = pLink->pTmp1_O + ALIGN4(pCfgTmp->Link_Nodes);

		pLink->pTmp1_S = pTmp_SE_base;
		pLink->pTmp1_E = pLink->pTmp1_S + ALIGN4(pCfgTmp->Link_Nodes);
		pLink->pTmp2_S = pLink->pTmp1_E + ALIGN4(pCfgTmp->Link_Nodes);
		pLink->pTmp2_E = pLink->pTmp2_S + ALIGN4(pCfgTmp->Link_Nodes);
	}

	return;
}

//--------------------------------------------------------------------------
//  Mem_QMark()
//--------------------------------------------------------------------------
void Mem_QMark(int key, struct _Cfg *pCfgTmp, int Opt, int Channel)
{
	int i, TSize;
	//INT16 * ps;
	UINT32 pBase;

	TSize = ALIGN4(pCfgTmp->SPEC_LEN) * pCfgTmp->RLink_Len * sizeof(INT16); // INT16 * pQMark[MAX_Q_LEN];

	Mem_Info.QMARK_MEM = TSize << pCfgTmp->stereo;

#ifdef ALLOC_MEM_INSIDE
	Mem_Info.pQMARK_MEM = malloc(Mem_Info.QMARK_MEM);
	memset((void *)Mem_Info.pQMARK_MEM, 0, Mem_Info.QMARK_MEM);
#endif

	if (Opt == OPTION_MEM_SET) {
		if (Channel == CHANNEL_L) {
			pBase = (UINT32)Mem_Info.pQMARK_MEM;
		} else {
			pBase = (UINT32)Mem_Info.pQMARK_MEM + (Mem_Info.QMARK_MEM >> 1);
		}

		for (i = pCfg->Q_Center; i < (pCfg->Q_Center + pCfg->RLink_Len); i++) {
			pLink->pQMark[i] = (INT16 *)(pBase + (i - pCfg->Q_Center) * ALIGN4(pCfg->SPEC_LEN) * sizeof(INT16));
			//ps = pLink->pQMark[i];
			//for (j=0;j<pCfg->SPEC_LEN;j++)
			//{    ps[j] = 0;    }    // 0 表示雜訊的索引值
		}
	}

	return;
}

//--------------------------------------------------------------------------
// GetMinUpdateInterval(int SR)
//--------------------------------------------------------------------------
int GetMinUpdateInterval(int SR)
{
	int Value;

	switch (SR) {
	case 8000:
	case 16000:
	case 32000:
		Value = 8;
		break;
	case 11025:
	case 22050:
	case 44100:
		Value = 11;
		break;
	case 48000:
		Value = 12;
		break;
	default:
		Value = 8;
		err_code = ErrCode_110;
		break;
	}

	return Value;
}

//--------------------------------------------------------------------------
// GetMaxUpdateInterval(int SR)
//--------------------------------------------------------------------------
int GetMaxUpdateInterval(int SR)
{
	int Value;

	switch (SR) {
	case 8000:
	case 16000:
	case 32000:
		Value = 16;
		break;
	case 11025:
	case 22050:
	case 44100:
		Value = 22;
		break;
	case 48000:
		Value = 24;
		break;
	default:
		Value = 16;
		err_code = ErrCode_111;
		break;
	}

	return Value;
}

//--------------------------------------------------------------------------
//  _ANR_init()
//--------------------------------------------------------------------------
void _ANR_init(int inkey, struct ANR_CONFIG *ptANR, int Channel)
{
	int i;

	// 0.============= 內部變數 =============
	pQu->QIdx = 0;

	// 1.============= Cfg Structure =============
	pCfg->sampling_rate = ptANR->sampling_rate;
	pCfg->stereo        = ptANR->stereo;
	if (pCfg->stereo < 0) {
		pCfg->stereo = 0;
	}
	if (pCfg->stereo > 1) {
		pCfg->stereo = 1;
	}
	pCfg->Blk1CSizeW   = ptANR->blk_size_w >> pCfg->stereo;
	pCfg->Freq_Max     = ptANR->sampling_rate >> 1;

	pCfg->FFT_SZ_Exp    = Get_FFT_SizeExp(pCfg->sampling_rate);
	pCfg->FFT_SZ        = (1 << pCfg->FFT_SZ_Exp);
	pCfg->FFT_OV_SZ = pCfg->FFT_SZ >> 1;
	if (pCfg->Blk1CSizeW < pCfg->FFT_SZ)                 {
		err_code = ErrCode_101;         // pCfg->Blk1CSizeW 必須大於等於 256.
	}
	if ((pCfg->Blk1CSizeW & (pCfg->FFT_OV_SZ - 1)) != 0) {
		err_code = ErrCode_102;         // pCfg->Blk1CSizeW 必須是 128 的倍數
	}
	pCfg->SPEC_LEN  = (pCfg->FFT_SZ >> 1) + 1;
	pCfg->Link_Nodes = (pCfg->SPEC_LEN >> 2) + 1;     // 頻譜長129時,Node數最多33.頻譜長257時,Node數最多65

	pCfg->NR_SIdx_Low = 3 - 1;                // Matlab: 3
	pCfg->NR_SIdx_High = (pCfg->FFT_SZ >> 1); // Matlab: pCfg->FFT_SZ/2 + 1;

	pCfg->CB_Len = Get_CB_Size(pCfg->sampling_rate);
	switch (pCfg->sampling_rate) {
	case 8000:  // SR:8000Hz
		pCfg->Idx_1k   = 32;    // Matlab: 1000*128/4000
		pCfg->CB_Start = Tbl.CB_Start_8k;
		pCfg->CB_End   = Tbl.CB_End_8k;
		pCfg->CB_Num   = Tbl.CB_Num_8k;
		break;
	case 11025:  // SR:11025Hz
		pCfg->Idx_1k   = 23;    // Matlab: 1000*128/5512.5
		pCfg->CB_Start = Tbl.CB_Start_11k;
		pCfg->CB_End   = Tbl.CB_End_11k;
		pCfg->CB_Num   = Tbl.CB_Num_11k;
		break;
	case 16000:  // SR:16000Hz
		pCfg->Idx_1k   = 32;    // Matlab: 1000*256/8000
		pCfg->CB_Start = Tbl.CB_Start_16k;
		pCfg->CB_End   = Tbl.CB_End_16k;
		pCfg->CB_Num   = Tbl.CB_Num_16k;
		break;
	case 22050:  // SR:22050Hz
		pCfg->Idx_1k   = 23;    // Matlab: 1000*256/11025
		pCfg->CB_Start = Tbl.CB_Start_22k;
		pCfg->CB_End   = Tbl.CB_End_22k;
		pCfg->CB_Num   = Tbl.CB_Num_22k;
		break;
	case 32000:  // SR:32000Hz
		pCfg->Idx_1k   = 32;    // Matlab: 1000*512/16000
		pCfg->CB_Start = Tbl.CB_Start_32k;
		pCfg->CB_End   = Tbl.CB_End_32k;
		pCfg->CB_Num   = Tbl.CB_Num_32k;
		break;
	case 44100:  // SR:44100Hz
		pCfg->Idx_1k   = 23;    // Matlab: 1000*512/22050
		pCfg->CB_Start = Tbl.CB_Start_44k;
		pCfg->CB_End   = Tbl.CB_End_44k;
		pCfg->CB_Num   = Tbl.CB_Num_44k;
		break;
	case 48000:  // SR:48000Hz
		pCfg->Idx_1k   = 21;    // Matlab: 1000*512/24000
		pCfg->CB_Start = Tbl.CB_Start_48k;
		pCfg->CB_End   = Tbl.CB_End_48k;
		pCfg->CB_Num   = Tbl.CB_Num_48k;
		break;
	default:
		err_code = ErrCode_109;
		//printf("\n No support in CB_Filter()");
		break;
	}

	pCfg->Valid_SIdx_Low  = 1 - 1;              // Matlab: 1
	pCfg->Valid_SIdx_High = pCfg->FFT_SZ >> 1;  // Matlab: pCfg->FFT_SZ/2 + 1;

	pCfg->spec_bias_low  = ptANR->spec_bias_low;
	pCfg->spec_bias_high = ptANR->spec_bias_high;

	pCfg->nr_db   = ptANR->nr_db;
	if (pCfg->nr_db < 3) {
		pCfg->nr_db = 3;
	}
	if (pCfg->nr_db > 35) {
		pCfg->nr_db = 35;
	}

	pCfg->bias_sensitive          = ptANR->bias_sensitive;         // 1~9
	if (pCfg->bias_sensitive > 9) {
		pCfg->bias_sensitive = 9;
	}
	if (pCfg->bias_sensitive < 1) {
		pCfg->bias_sensitive = 1;
	}

	pCfg->noise_est_hold_time      = ptANR->noise_est_hold_time;
	if (pCfg->noise_est_hold_time < 1) {
		pCfg->noise_est_hold_time = 1;
	}
	if (pCfg->noise_est_hold_time > 5) {
		pCfg->noise_est_hold_time = 5;
	}

	pCfg->HPF_CutOffBin = (ptANR->hpf_cutoff_freq * pCfg->FFT_SZ + (pCfg->sampling_rate >> 1)) / pCfg->sampling_rate;
	// SR             [8000, 11025, 16000, 22050, 32000, 44100, 48000]
	// HPF_CutOffBin[     3,     2,       3,      2,     3,     2,       2]

	pCfg->max_bias_limit    = ptANR->max_bias_limit;    // Qu3.6

	switch (pCfg->sampling_rate) {
	case 8000:
		pCfg->Wing_L = 3;         // pCfg->Wing_L = round(3*(128*1000/8000)/pCfg->ms_Per_Frm);
		break;
	case 11025:
		pCfg->Wing_L = 4;
		break;
	case 16000:
		pCfg->Wing_L = 3;
		break;
	case 22050:
		pCfg->Wing_L = 4;
		break;
	case 32000:
		pCfg->Wing_L = 3;
		break;
	case 44100:
		pCfg->Wing_L = 4;
		break;
	case 48000:
		pCfg->Wing_L = 5;
		break;
	default:
		err_code = ErrCode_105;
		break;
	}
	pCfg->Wing_R    = pCfg->Wing_L + 1;  // 相差1，因為中心點
	pCfg->Q_Center = pCfg->Wing_L + 1;
	pCfg->Q_Len    = 1 + pCfg->Wing_L + 1 + pCfg->Wing_R + 1;

	pCfg->RLink_Len   = pCfg->Wing_R + 1;

	pCfg->m_curve_n1_level = ptANR->m_curve_n1_level;
		if (pCfg->m_curve_n1_level > 8)  pCfg->m_curve_n1_level = 8;
		if (pCfg->m_curve_n1_level < 0)  pCfg->m_curve_n1_level = 0;
	 pCfg->m_curve_n2_level = ptANR->m_curve_n2_level;
		if (pCfg->m_curve_n2_level > 8)  pCfg->m_curve_n2_level = 8;
		if (pCfg->m_curve_n2_level < 0)  pCfg->m_curve_n2_level = 0;
	// 2.============= kfft =============
	Mem_kfft(inkey, pCfg, OPTION_MEM_SET);

	// 3.============= Var Structure =============
	pVar->InFrmCnt = 0; // debug
	pVar->OutFrmCnt = 0;     // debug

	pVar->First_SS = 1;

	pVar->default_bias         = ptANR->default_bias;    // Qu10.6
	pVar->max_bias              = ptANR->max_bias;
	pVar->default_eng          = ptANR->default_eng;

	Mem_Var(inkey, pCfg, OPTION_MEM_SET, Channel);  // allocate memory space in Var structure

	for (i = 0; i < pCfg->SPEC_LEN; i++) {
		pVar->default_spec[i]      = ptANR->default_spec[i];    // Qu16
	}

	for (i = 0; i <= NR_FADE_CEIL_STEPS; i++) { // 0~12
		// Qu1.15
		pVar->NRTable[i] = (INT32)(((Tbl.NRTable_03[i] * (35 - pCfg->nr_db) + Tbl.NRTable_35[i] * (pCfg->nr_db - 3)) + (1 << 4)) >> 5);
		// Qu(1.15-8)=Qu1.7 for 8kHz(256 frm sz), Qu(1.15-9)=Q1.6 for 16kHz(512 frm sz)
		pVar->NRTable[i] = (pVar->NRTable[i] + (1 << (pCfg->FFT_SZ_Exp - 1))) >> pCfg->FFT_SZ_Exp;
	}

	// 5.============= NPink Structure =============
	pNPink->PFrm_Cnt = 0;
	pNPink->Min_Update_Interval = GetMinUpdateInterval(pCfg->sampling_rate);
	pNPink->Max_Update_Interval = GetMaxUpdateInterval(pCfg->sampling_rate);

	Mem_NPink(inkey, pCfg, OPTION_MEM_SET, Channel);

	// 6.============= NColor Structure =============
	pNColor->CFrm_Cnt = 0;
	pNColor->Min_Update_Interval = GetMinUpdateInterval(pCfg->sampling_rate);
	pNColor->Max_Update_Interval = GetMaxUpdateInterval(pCfg->sampling_rate);

	Mem_NColor(inkey, pCfg, OPTION_MEM_SET, Channel);

	// 7.============= NTone Structure =============
	pNTone->ToneMinFrmCnt = ptANR->tone_min_time;

	Mem_Tone(inkey, pCfg, OPTION_MEM_SET, Channel);

	// 8.============= Queue memory =============
	for (i = 0; i < pCfg->Q_Len; i++) {
		pVar->QBias[i] = 0;
		pVar->QEngG[i] = 0;
	}

	Mem_Qu(inkey, pCfg, OPTION_MEM_SET, Channel);

	// 9.============= CLink + RLink =============
	Mem_Link(inkey, pCfg, OPTION_MEM_SET, Channel);

	// 10.============= pQMark =============
	Mem_QMark(inkey, pCfg, OPTION_MEM_SET, Channel);

	// 11.============= Setup Default values =============
	//   以下3程式碼必須與 ANR_Detect() 同步

	// 1.根據 Default 值, 設定內部變數
	Setup_Inner_Vars();

	// 2.根據 Default 值, 計算 pVar->Sig_MCurve
	//CreateMulCurve(pVar->Sig_MCurve, pVar->HH, pVar->LL, 2, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High);
     //CreateMulCurve(pVar->Sig_MCurve, pVar->LL, 64 , 2, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High);
	 CreateMulCurve(pVar->Sig_MCurve, pVar->MCurveN1, pVar->MCurveN2 , 2, pCfg->Valid_SIdx_Low, pCfg->Valid_SIdx_High);

	// 3.根據 Default 值, 設定 NPink, pNColor->..etc 相關變數
	Setup_BG_Vars();

	//Dump_Cfg();

	return;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_get_version()        <===  API function
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
int audlib_anr_get_version(void)
{
	return ANR_VER;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  AUD_ANR_Reset()        <===  API function
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
INT32 AUD_ANR_Reset(VOID)
{
	INT32 i, j;
	INT16 *ps;

	pVar->InFrmCnt = 0;
	pVar->OutFrmCnt = 0;

	for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
		pVar->asBufPadIn[i] = 0;  //NVT_ANR_Reset()
	}

	// reset Global variables
	pQu->QIdx = 0;
	//pVar->max_bias = 5*64;        // Bias = Qu26.6

	// reset Global Buffers
	for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
		pVar->asNRout[i] = 0;     //NVT_ANR_Reset()
		pVar->asNRtail[i] = 0;    //NVT_ANR_Reset()
	}
	for (i = 0; i < pCfg->SPEC_LEN; i++) {
		pVar->asNRabs[i] = 0;     //NVT_ANR_Reset()
	}

	for (i = 0; i < pCfg->Q_Len; i++) {
		pVar->QEngG[i] = 0;
	}

	for (i = pCfg->Q_Center; i <= pCfg->Wing_R; i++) {
		ps = pLink->pQMark[i];
		for (j = 0; j < pCfg->SPEC_LEN; j++) {
			ps[j] = 0;       // pQMark 預設為零
		}
	}

	return 0;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_pre_init()        <===  API function
//
//    Description: return memory buffer size which should be feed into audlib_anr_init()
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
INT32 audlib_anr_pre_init(struct ANR_CONFIG *ptANR)
{
	int i, TSize;
	struct _Cfg CfgTmp;

	// 0.============= Cfg Structure =============
	CfgTmp.sampling_rate = ptANR->sampling_rate;
	CfgTmp.stereo     = ptANR->stereo;
	if (CfgTmp.stereo < 0) {
		CfgTmp.stereo = 0;
	}
	if (CfgTmp.stereo > 1) {
		CfgTmp.stereo = 1;
	}
	CfgTmp.Blk1CSizeW   = ptANR->blk_size_w >> CfgTmp.stereo;
	CfgTmp.FFT_SZ_Exp   = Get_FFT_SizeExp(CfgTmp.sampling_rate);
	CfgTmp.FFT_SZ       = (1 << CfgTmp.FFT_SZ_Exp);
	CfgTmp.FFT_OV_SZ    = CfgTmp.FFT_SZ >> 1;
	if (CfgTmp.Blk1CSizeW < CfgTmp.FFT_SZ)               {
		err_code = ErrCode_101;         // pCfg->Blk1CSizeW 必須大於等於 256.
	}
	if ((CfgTmp.Blk1CSizeW & (CfgTmp.FFT_OV_SZ - 1)) != 0) {
		err_code = ErrCode_102;           // pCfg->Blk1CSizeW 必須是 128 的倍數
	}
	CfgTmp.SPEC_LEN     = (CfgTmp.FFT_SZ >> 1) + 1;
	CfgTmp.Link_Nodes   = CfgTmp.SPEC_LEN / 3;     // 頻譜長129時,Node數最多33.頻譜長257時,Node數最多65

	CfgTmp.CB_Len = Get_CB_Size(CfgTmp.sampling_rate);

	switch (CfgTmp.sampling_rate) {
	case 8000:
		CfgTmp.Wing_L = 3;
		break;
	case 11025:
		CfgTmp.Wing_L = 4;
		break;
	case 16000:
		CfgTmp.Wing_L = 3;
		break;
	case 22050:
		CfgTmp.Wing_L = 4;
		break;
	case 32000:
		CfgTmp.Wing_L = 3;
		break;
	case 44100:
		CfgTmp.Wing_L = 4;
		break;
	case 48000:
		CfgTmp.Wing_L = 5;        // 更改此處後，需更改 #define MAX_Q_LEN  (1+5+1+6+1)
		break;
	default:
		CfgTmp.Wing_L = 3;
		err_code = ErrCode_105;
		break;
	}
	CfgTmp.Wing_R = CfgTmp.Wing_L + 1;
	CfgTmp.Q_Center = CfgTmp.Wing_L + 1;
	CfgTmp.Q_Len = 1 + CfgTmp.Wing_L + 1 + CfgTmp.Wing_R + 1;

	CfgTmp.RLink_Len = CfgTmp.Wing_R + 1;

	memset((void *)(&Mem_Info), 0, sizeof(struct stMEM_Info));
	// 1.============= kFFT Structure =============
	Mem_Structure(0, &CfgTmp, OPTION_MEM_REQIRE);

	// 2.============= kFFT Structure =============
	Mem_kfft(0, &CfgTmp, OPTION_MEM_REQIRE);

	// 3.============= Var Structure =============
	Mem_Var(0, &CfgTmp, OPTION_MEM_REQIRE, 0);  // allocate memory space in Var structure

	// 4.============= NPink Structure =============
	Mem_NPink(0, &CfgTmp, OPTION_MEM_REQIRE, 0);

	// 5.============= NColor Structure =============
	Mem_NColor(0, &CfgTmp, OPTION_MEM_REQIRE, 0);

	// 6.============= NTone Structure =============
	Mem_Tone(0, &CfgTmp, OPTION_MEM_REQIRE, 0);

	// 7.============= Queue =============
	Mem_Qu(0, &CfgTmp, OPTION_MEM_REQIRE, 0);

	// 8.============= CLink + RLink =============
	Mem_Link(0, &CfgTmp, OPTION_MEM_REQIRE, 0);

	// 9.============= pQMark =============
	Mem_QMark(0, &CfgTmp, OPTION_MEM_REQIRE, 0);

	TSize = 0;
	for (i = 0; i < (int)(sizeof(Mem_Info) / sizeof(INT32)); i += 2) {
		TSize += *((INT32 *)(&Mem_Info) + i);
	}

	return    TSize;
}



//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_init()        <===  API function
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
INT32 audlib_anr_init(int *phandle, struct ANR_CONFIG *ptANR)
{
	INT32  inkey;

	// =============== 尋找空 handle ==============
	for (inkey = 0; inkey < MAX_STREAM; inkey++) {
		if (Ring_of_Key[inkey] == 0) {
			break;
		}
	}
	if (inkey >= MAX_STREAM) {
		err_code = ErrCode_107;
		return err_code;
	}

#ifndef ALLOC_MEM_INSIDE
	memset(ptANR->p_mem_buffer, 0, ptANR->memory_needed);  // 清空外部配置的 buffer
	Mem_Dispatch(inkey, ptANR);
#endif

	// ------ Two Channels ------
	audlib_anr_detect_reset(inkey);

	// ------ Channel Left ------
	pCfg    = pCfg_Stream[inkey];
	pVar    = Var_Stream[inkey].pVar_L;
	pNPink  = NPink_Stream[inkey].pNPink_L;
	pNColor = NColor_Stream[inkey].pNColor_L;
	pNTone  = NTone_Stream[inkey].pNTone_L;
	pLink   = Link_Stream[inkey].pLink_L;
	pQu     = Qu_Stream[inkey].pQu_L;
	_ANR_init(inkey, ptANR, CHANNEL_L);

	if (ptANR->stereo) {
		// ------ Channel Right ------
		pCfg    = pCfg_Stream[inkey];
		pVar    = Var_Stream[inkey].pVar_R;
		pNPink  = NPink_Stream[inkey].pNPink_R;
		pNColor = NColor_Stream[inkey].pNColor_R;
		pNTone  = NTone_Stream[inkey].pNTone_R;
		pLink   = Link_Stream[inkey].pLink_R;
		pQu     = Qu_Stream[inkey].pQu_R;
		_ANR_init(inkey, ptANR, CHANNEL_R);
	}
	// --------------------------
	if (err_code == 0) {
		Ring_of_Key[inkey] = 1;
		*phandle = inkey + KEY_BASE;
	}

#ifdef LOCK_813x
	FILE *fhdl;

	fhdl = fopen("/proc/pmu/chipver", "r");
	fgets(sShit, 9, fhdl);
	//printf("\n sShift = %s", sShit);fflush(stdout);
	fclose(fhdl);

	Dead = 0;
	//if (sShit[3]!='6') { Dead++; }
	if (sShit[1] != '1') {
		Dead++;
	}
	if (sShit[0] != '8') {
		Dead++;
	}
	if (sShit[2] != '3') {
		Dead++;
	}

	//Dead = 1;
	if (Dead > 0) {
		pVar->NRTable[NR_FADE_CEIL_STEPS] = pVar->NRTable[0];
	}
#endif

	return    err_code;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_set_snri()        <===  API function
//        Input : value of SNR Improvement
//        Return :
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VOID audlib_anr_set_snri(INT32 value)
{
	INT32 i;

	pCfg->nr_db            = value;
	if (pCfg->nr_db < 3)  {
		pCfg->nr_db = 3;
	}
	if (pCfg->nr_db > 35) {
		pCfg->nr_db = 35;
	}
	for (i = 0; i <= NR_FADE_CEIL_STEPS; i++) {
		// Q15
		pVar->NRTable[i] = (INT32)(((Tbl.NRTable_03[i] * (35 - pCfg->nr_db) + Tbl.NRTable_35[i] * (pCfg->nr_db - 3)) + (1 << 4)) >> 5);
		// Q(15-8)=Q7 for 8kHz, Q(15-9)=Q6 for 16kHz
		pVar->NRTable[i] = (pVar->NRTable[i] + (1 << (pCfg->FFT_SZ_Exp - 1))) >> pCfg->FFT_SZ_Exp;
	}

	return;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_get_snri()        <===  API function
//        Input : value of SNR Improvement
//        Return :
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
INT32 audlib_anr_get_snri(VOID)
{
	return pCfg->nr_db;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_detect()        <===  API function
//        Input : a block of PCM data
//        Return :
//                NOISE_DETECTING(0) : 統計中
//                NOISE_UPDATING(1)  : 統計完成,更新頻譜和能量
//                NOISE_UPDATED(2)   : 更新完成
//        Update :
//                max_bias
//                pVar->HH, pVar->LL
//                default_bias
//                pVar->default_spec
//                pVar->default_eng
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VOID audlib_anr_detect_reset(int inkey)
{
	memset(&Detect_Stream[inkey], 0, sizeof(struct _Detect_LR));        // 清空

	Detect_Stream[inkey].Detect_L.RepeatIdx = 2;                    // 3 in Matlab
	Detect_Stream[inkey].Detect_L.SMode     = NOISE_DETECTING;

	Detect_Stream[inkey].Detect_R.RepeatIdx = 2;  // 3 in Matlab
	Detect_Stream[inkey].Detect_R.SMode     = NOISE_DETECTING;

	return;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_detect()        <===  API function
//        Input : a block of PCM data
//        Return :
//                NOISE_DETECTING(0) : 統計中
//                NOISE_UPDATING(1)  : 統計完成,更新頻譜和能量
//                NOISE_UPDATED(2)   : 更新完成
//        Update :
//                max_bias
//                pVar->HH, pVar->LL
//                default_bias
//                pVar->default_spec
//                pVar->default_eng
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
INT32 audlib_anr_detect(int outkey, INT16 *pIn, struct ANR_CONFIG *ptANR)
{
	INT32 i, k, c, inkey, ret;
	INT16 *psIn;

	inkey = outkey - KEY_BASE;
	if (Ring_of_Key[inkey] != 1) {
		return ErrCode_108;
	}

	pCfg = pCfg_Stream[inkey];
	pKFFT_Cfg0 = KFFT_Cfg0_Stream[inkey];

	for (c = 0; c <= pCfg->stereo; c++) {
		psIn = pIn;

		switch (c) {
		case 0:        // Mono
			pDetect = &(Detect_Stream[inkey].Detect_L);
			pVar    = Var_Stream[inkey].pVar_L;
			pNPink  = NPink_Stream[inkey].pNPink_L;
			pNColor = NColor_Stream[inkey].pNColor_L;
			pNTone  = NTone_Stream[inkey].pNTone_L;
			pLink   = Link_Stream[inkey].pLink_L;
			pQu     = Qu_Stream[inkey].pQu_L;
			break;
		case 1:        // stereo
			pDetect = &(Detect_Stream[inkey].Detect_R);
			pVar    = Var_Stream[inkey].pVar_R;
			pNPink  = NPink_Stream[inkey].pNPink_R;
			pNColor = NColor_Stream[inkey].pNColor_R;
			pNTone  = NTone_Stream[inkey].pNTone_R;
			pLink   = Link_Stream[inkey].pLink_R;
			pQu     = Qu_Stream[inkey].pQu_R;
			break;
		default:
			break;
		}

		// --------------- First frame ---------------
		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			pVar->asFFT0[i].r = pVar->asBufPadIn[i];  // front padding
			pVar->asFFT0[i].i = 0;
		}

		for (i = pCfg->FFT_OV_SZ; i < pCfg->FFT_SZ; i++) {
//printf("\n(%d < %d)", i, ((i-pCfg->FFT_OV_SZ)<<(pCfg->stereo))+c);
			pVar->asFFT0[i].r = psIn[((i - pCfg->FFT_OV_SZ) << (pCfg->stereo)) + c];
			pVar->asFFT0[i].i = 0;
		}

		if (pDetect->Status != NOISE_UPDATED) {
			time0(pDetect->Status = _ANR_detect(ptANR));
		}

		// --------------- Medium & Last frames ---------------
		//           |---256---|
		//             |---256---|...
		for (k = 0; k < pCfg->Blk1CSizeW - pCfg->FFT_OV_SZ; k += pCfg->FFT_OV_SZ) {
			for (i = 0; i < pCfg->FFT_SZ; i++) {
//printf("\n(%d < %d)", i, ((k + i)<<pCfg->stereo)+c);
				pVar->asFFT0[i].r = psIn[((k + i) << pCfg->stereo) + c]; //audlib_anr_detect()
				pVar->asFFT0[i].i = 0;
			}

			if (pDetect->Status != NOISE_UPDATED) {
				time0(pDetect->Status = _ANR_detect(ptANR));
			}

		}

		// ---------------- Prepare next frame ------------
		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
//printf("\n(%d < %d)", i, ((i+k)<<pCfg->stereo)+c);
			pVar->asBufPadIn[i] = psIn[((i + k) << pCfg->stereo) + c];
		}
	}

	switch (pCfg->stereo) {
	case 0:
		ret = Detect_Stream[inkey].Detect_L.Status;
		break;
	case 1:
		if ((Detect_Stream[inkey].Detect_L.Status == NOISE_UPDATED) && (Detect_Stream[inkey].Detect_R.Status == NOISE_UPDATED)) {
			ret = NOISE_UPDATED;
		} else {
			ret = NOISE_DETECTING;
		}
		break;
	default:
		ret = Detect_Stream[inkey].Detect_L.Status;
		break;
	}

	return ret;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  audlib_anr_run()        <===  API function
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
INT32 audlib_anr_run(int outkey,
				   INT16 *pIn,
				   INT16 *pOut)
{
	INT32 i, k, c, inkey;
	INT16 *psIn, * psOut;

	inkey = outkey - KEY_BASE;
	if (Ring_of_Key[inkey] != 1) {
		return ErrCode_108;
	}

	pCfg = pCfg_Stream[inkey];
	pKFFT_Cfg0 = KFFT_Cfg0_Stream[inkey];
	pKFFT_Cfg1 = KFFT_Cfg1_Stream[inkey];

	for (c = 0; c <= pCfg->stereo; c++) {
		psIn = pIn;
		psOut = pOut;

		switch (c) {
		case 0:        // Mono
			pVar    = Var_Stream[inkey].pVar_L;
			pNPink  = NPink_Stream[inkey].pNPink_L;
			pNColor = NColor_Stream[inkey].pNColor_L;
			pNTone  = NTone_Stream[inkey].pNTone_L;
			pLink   = Link_Stream[inkey].pLink_L;
			pQu     = Qu_Stream[inkey].pQu_L;
			break;
		case 1:        // stereo
			pVar    = Var_Stream[inkey].pVar_R;
			pNPink  = NPink_Stream[inkey].pNPink_R;
			pNColor = NColor_Stream[inkey].pNColor_R;
			pNTone  = NTone_Stream[inkey].pNTone_R;
			pLink   = Link_Stream[inkey].pLink_R;
			pQu     = Qu_Stream[inkey].pQu_R;
			break;
		default:
			break;
		}

		// --------------- First frame ---------------
		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			pVar->asFFT0[i].r = pVar->asBufPadIn[i];  // front padding
			pVar->asFFT0[i].i = 0;
		}

		for (i = pCfg->FFT_OV_SZ; i < pCfg->FFT_SZ; i++) {
			pVar->asFFT0[i].r = psIn[((i - pCfg->FFT_OV_SZ) << (pCfg->stereo)) + c];
			pVar->asFFT0[i].i = 0;
		}

		time0(_ANR_process());    // output:pVar->asNRout[40]
//for (i = 0; i < pCfg->FFT_OV_SZ; i++)
//	pVar->asNRout[i] = pVar->asFFT0[i].r;


		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			psOut[(i << (pCfg->stereo)) + c] = pVar->asNRout[i];
		}
		psOut += (pCfg->FFT_OV_SZ) << pCfg->stereo;

		// --------------- Medium & Last frames ---------------
		//           |---256---|
		//             |---256---|...
		for (k = 0; k < pCfg->Blk1CSizeW - pCfg->FFT_OV_SZ; k += pCfg->FFT_OV_SZ) {
			for (i = 0; i < pCfg->FFT_SZ; i++) {
				pVar->asFFT0[i].r = psIn[((k + i) << pCfg->stereo) + c]; //audlib_anr_run()
				pVar->asFFT0[i].i = 0;
			}

			time0(_ANR_process());    // output:pVar->asNRout[40]
//for (i = 0; i < pCfg->FFT_OV_SZ; i++)
//	pVar->asNRout[i] = pVar->asFFT0[i].r;

			for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
				psOut[(i << pCfg->stereo) + c] = pVar->asNRout[i];
			}
			psOut += (pCfg->FFT_OV_SZ) << pCfg->stereo;
		}

		// ---------------- Prepare next frame ------------
		for (i = 0; i < pCfg->FFT_OV_SZ; i++) {
			pVar->asBufPadIn[i] = psIn[((i + k) << pCfg->stereo) + c];
		}

	}

	return err_code;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  AUD_ANR_Destory()        <===  API function
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VOID audlib_anr_destroy(int *handle)
{

#ifdef ALLOC_MEM_INSIDE
	if (pKFFT_Cfg0 != NULL) {
		free(pKFFT_Cfg0);
		pKFFT_Cfg0 = NULL;
	}
	if (pKFFT_Cfg1 != NULL) {
		free(pKFFT_Cfg1);
		pKFFT_Cfg1 = NULL;
	}
	if (Mem_Info.pKFFT_MEM != NULL) {
		free(Mem_Info.pKFFT_MEM);
		Mem_Info.pKFFT_MEM = NULL;
	}

	if (pVar->Sig_MCurve != NULL) {
		free(pVar->Sig_MCurve);
		pVar->Sig_MCurve = NULL;
	}

	if (pNPink->MemBase != NULL) {
		free(pNPink->MemBase);
		pNPink->MemBase = NULL;
	}

	if (pNColor->MemBase != NULL) {
		free(pNColor->MemBase);
		pNColor->MemBase = NULL;
	}

	if (pNTone->Spec_Target != NULL) {
		free(pNTone->Spec_Target);
		pNTone->Spec_Target = NULL;
	}

	if (pQu->pQ_base != NULL) {
		free(pQu->pQ_base);
		pQu->pQ_base = NULL;
	}

	if (pCLink_O_base != NULL) {
		free(pCLink_O_base);
		pCLink_O_base = NULL;
	}

	if (pCLink_S_base != NULL) {
		free(pCLink_S_base);
		pCLink_S_base = NULL;
	}

	if (pTmp_O_base != NULL) {
		free(pTmp_O_base);
		pTmp_O_base = NULL;
	}

	if (pTmp_SE_base != NULL) {
		free(pTmp_SE_base);
		pTmp_SE_base = NULL;
	}

	if (pQMark_base != NULL) {
		free(pQMark_base);
		pQMark_base = NULL;
	}
#endif

	Ring_of_Key[*handle - KEY_BASE] = 0;
	*handle = 0;

	return;
}

int kdrv_audlib_anr_init(void) {
	int ret;

	ret = kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID_ANR, &anr_func);

	return ret;
}



