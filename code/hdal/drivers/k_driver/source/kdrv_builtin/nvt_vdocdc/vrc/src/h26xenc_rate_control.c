#if defined(__LINUX)
#include <linux/string.h>
#include <linux/slab.h>
#elif defined(__FREERTOS)
#include <string.h>
#include <stdbool.h>
#endif

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "h26xenc_rate_control.h"
//#include "../nvt_vencrc_dbg.h"
#include "nvt_vdocdc_dbg.h"

#if (defined __LINUX || defined __FREERTOS)
#define rc_err(fmt, args...)    { if (g_uiDbgLv >= RC_LOG_ERR) { DBG_DUMP(fmt, ##args);  }}
#define rc_wrn(fmt, args...)    { if (g_uiDbgLv >= RC_LOG_WRN) { DBG_DUMP(fmt, ##args);  }}
#define rc_dbg(fmt, args...)    { if (g_uiDbgLv >= RC_LOG_DBG) { DBG_DUMP(fmt, ##args);  }}
#define rc_inf(fmt, args...)    { if (g_uiDbgLv >= RC_LOG_INF) { DBG_DUMP(fmt, ##args);  }}
#if (defined __LINUX)
#define rc_dump_info(fmt, args...)  printk(fmt, ##args);
#else
#define rc_dump_info(fmt, args...)  DBG_DUMP(fmt, ##args);
#endif
#else
#define rc_err      h26xenc_err
#define rc_wrn      h26xenc_wrn
#define rc_dbg      h26xenc_dbg
#define rc_inf      h26xenc_info
#define rc_dump_info(fmt, args...)  printk(fmt, ##args);
#endif

#define LOG_LIST_I	90
#define LOG_LIST_J	10

static const int g_RCInvalidQPValue = -999;

/*
*  shift (1 << 11)
*  0.9 * 2048 = 1843
*  0.1 * 2048 = 205
*  0.5 * 2048 = 1024
*/
#if SMOOTH_FRAME_QP
static int g_RCWeightPicTargetBitInGOP_cbr = 205;         // (int)(0.6 * SHIFT_VALUE);
static int g_RCWeightPicRargetBitInBuffer_cbr = 1843;       // (int)(0.4 * SHIFT_VALUE);
static int g_RCWeightPicTargetBitInGOP_vbr = 1638;         // (int)(0.8 * SHIFT_VALUE);
static int g_RCWeightPicRargetBitInBuffer_vbr = 410;       // (int)(0.2 * SHIFT_VALUE);
static int g_RCWeightPicTargetBitInGOP_evbr = 205;         // (int)(0.1 * SHIFT_VALUE);
static int g_RCWeightPicRargetBitInBuffer_evbr = 1843;     // (int)(0.9 * SHIFT_VALUE);
int g_RCPicGOPWeight_cbr = 10;
int g_RCPicGOPWeight_vbr = 10;
int g_RCPicGOPWeight_evbr = 10;
#else
static const int g_RCWeightPicTargetBitInGOP = 205;         // (int)(0.9 * SHIFT_VALUE);1843
static const int g_RCWeightPicRargetBitInBuffer = 1843;       // SHIFT_VALUE - g_RCWeightPicTargetBitInGOP;205
#endif
static const int g_RCWeightHistoryLambda = 1024;             // (int)(0.5 * SHIFT_VALUE);
static const int g_RCWeightCurrentLambda = 1024;             // SHIFT_VALUE - g_RCWeightHistoryLambda;

//static const int g_RCIterationNum = 20;
//static const int g_RCLCUSmoothWindowSize = 4;

#if WEIGHT_CAL_RESET==2
static const int g_RCAlphaMinValue = 1000;   //102
static const int g_RCAlphaMaxValue = 1024000;
static const int g_RCBetaMinValue = -6144;
static const int g_RCBetaMaxValue = -1000;    //-205
#else
static const int g_RCAlphaMinValue = 102;
static const int g_RCAlphaMaxValue = 1024000;
static const int g_RCBetaMinValue = -6144;
static const int g_RCBetaMaxValue = -205;
#endif
#if WEIGHT_CAL_RESET
unsigned int g_svcAdjustQPWeight = 1;
#endif
static const int     g_log_list[LOG_LIST_I][LOG_LIST_J] = {
	{ 0, 43, 86, 128, 170, 212, 253, 294, 334, 374 }, { 414, 453, 492, 531, 569, 607, 645, 682, 719, 755 },
	{ 792, 828, 864, 899, 934, 969, 1004, 1038, 1072, 1106 }, { 1139, 1173, 1206, 1239, 1271, 1303, 1335, 1367, 1399, 1430 },
	{ 1461, 1492, 1523, 1553, 1584, 1614, 1644, 1673, 1703, 1732 }, { 1761, 1790, 1818, 1847, 1875, 1903, 1931, 1959, 1987, 2014 },
	{ 2041, 2068, 2095, 2122, 2148, 2175, 2201, 2227, 2253, 2279 }, { 2304, 2330, 2355, 2380, 2405, 2430, 2455, 2480, 2504, 2529 },
	{ 2553, 2577, 2601, 2625, 2648, 2672, 2695, 2718, 2742, 2765 }, { 2788, 2810, 2833, 2856, 2878, 2900, 2923, 2945, 2967, 2989 },
	{ 3010, 3032, 3054, 3075, 3096, 3118, 3139, 3160, 3181, 3201 }, { 3222, 3243, 3263, 3284, 3304, 3324, 3345, 3365, 3385, 3404 },
	{ 3424, 3444, 3464, 3483, 3502, 3522, 3541, 3560, 3579, 3598 }, { 3617, 3636, 3655, 3674, 3692, 3711, 3729, 3747, 3766, 3784 },
	{ 3802, 3820, 3838, 3856, 3874, 3892, 3909, 3927, 3945, 3962 }, { 3979, 3997, 4014, 4031, 4048, 4065, 4082, 4099, 4116, 4133 },
	{ 4150, 4166, 4183, 4200, 4216, 4232, 4249, 4265, 4281, 4298 }, { 4314, 4330, 4346, 4362, 4378, 4393, 4409, 4425, 4440, 4456 },
	{ 4472, 4487, 4502, 4518, 4533, 4548, 4564, 4579, 4594, 4609 }, { 4624, 4639, 4654, 4669, 4683, 4698, 4713, 4728, 4742, 4757 },
	{ 4771, 4786, 4800, 4814, 4829, 4843, 4857, 4871, 4886, 4900 }, { 4914, 4928, 4942, 4955, 4969, 4983, 4997, 5011, 5024, 5038 },
	{ 5051, 5065, 5079, 5092, 5105, 5119, 5132, 5145, 5159, 5172 }, { 5185, 5198, 5211, 5224, 5237, 5250, 5263, 5276, 5289, 5302 },
	{ 5315, 5328, 5340, 5353, 5366, 5378, 5391, 5403, 5416, 5428 }, { 5441, 5453, 5465, 5478, 5490, 5502, 5514, 5527, 5539, 5551 },
	{ 5563, 5575, 5587, 5599, 5611, 5623, 5635, 5647, 5658, 5670 }, { 5682, 5694, 5705, 5717, 5729, 5740, 5752, 5763, 5775, 5786 },
	{ 5798, 5809, 5821, 5832, 5843, 5855, 5866, 5877, 5888, 5899 }, { 5911, 5922, 5933, 5944, 5955, 5966, 5977, 5988, 5999, 6010 },
	{ 6021, 6031, 6042, 6053, 6064, 6075, 6085, 6096, 6107, 6117 }, { 6128, 6138, 6149, 6160, 6170, 6180, 6191, 6201, 6212, 6222 },
	{ 6232, 6243, 6253, 6263, 6274, 6284, 6294, 6304, 6314, 6325 }, { 6335, 6345, 6355, 6365, 6375, 6385, 6395, 6405, 6415, 6425 },
	{ 6435, 6444, 6454, 6464, 6474, 6484, 6493, 6503, 6513, 6522 }, { 6532, 6542, 6551, 6561, 6571, 6580, 6590, 6599, 6609, 6618 },
	{ 6628, 6637, 6646, 6656, 6665, 6675, 6684, 6693, 6702, 6712 }, { 6721, 6730, 6739, 6749, 6758, 6767, 6776, 6785, 6794, 6803 },
	{ 6812, 6821, 6830, 6839, 6848, 6857, 6866, 6875, 6884, 6893 }, { 6902, 6911, 6920, 6928, 6937, 6946, 6955, 6964, 6972, 6981 },
	{ 6990, 6998, 7007, 7016, 7024, 7033, 7042, 7050, 7059, 7067 }, { 7076, 7084, 7093, 7101, 7110, 7118, 7126, 7135, 7143, 7152 },
	{ 7160, 7168, 7177, 7185, 7193, 7202, 7210, 7218, 7226, 7235 }, { 7243, 7251, 7259, 7267, 7275, 7284, 7292, 7300, 7308, 7316 },
	{ 7324, 7332, 7340, 7348, 7356, 7364, 7372, 7380, 7388, 7396 }, { 7404, 7412, 7419, 7427, 7435, 7443, 7451, 7459, 7466, 7474 },
	{ 7482, 7490, 7497, 7505, 7513, 7520, 7528, 7536, 7543, 7551 }, { 7559, 7566, 7574, 7582, 7589, 7597, 7604, 7612, 7619, 7627 },
	{ 7634, 7642, 7649, 7657, 7664, 7672, 7679, 7686, 7694, 7701 }, { 7709, 7716, 7723, 7731, 7738, 7745, 7752, 7760, 7767, 7774 },
	{ 7782, 7789, 7796, 7803, 7810, 7818, 7825, 7832, 7839, 7846 }, { 7853, 7860, 7868, 7875, 7882, 7889, 7896, 7903, 7910, 7917 },
	{ 7924, 7931, 7938, 7945, 7952, 7959, 7966, 7973, 7980, 7987 }, { 7993, 8000, 8007, 8014, 8021, 8028, 8035, 8041, 8048, 8055 },
	{ 8062, 8069, 8075, 8082, 8089, 8096, 8102, 8109, 8116, 8122 }, { 8129, 8136, 8142, 8149, 8156, 8162, 8169, 8176, 8182, 8189 },
	{ 8195, 8202, 8209, 8215, 8222, 8228, 8235, 8241, 8248, 8254 }, { 8261, 8267, 8274, 8280, 8287, 8293, 8299, 8306, 8312, 8319 },
	{ 8325, 8331, 8338, 8344, 8351, 8357, 8363, 8370, 8376, 8382 }, { 8388, 8395, 8401, 8407, 8414, 8420, 8426, 8432, 8439, 8445 },
	{ 8451, 8457, 8463, 8470, 8476, 8482, 8488, 8494, 8500, 8506 }, { 8513, 8519, 8525, 8531, 8537, 8543, 8549, 8555, 8561, 8567 },
	{ 8573, 8579, 8585, 8591, 8597, 8603, 8609, 8615, 8621, 8627 }, { 8633, 8639, 8645, 8651, 8657, 8663, 8669, 8675, 8681, 8686 },
	{ 8692, 8698, 8704, 8710, 8716, 8722, 8727, 8733, 8739, 8745 }, { 8751, 8756, 8762, 8768, 8774, 8779, 8785, 8791, 8797, 8802 },
	{ 8808, 8814, 8820, 8825, 8831, 8837, 8842, 8848, 8854, 8859 }, { 8865, 8871, 8876, 8882, 8887, 8893, 8899, 8904, 8910, 8915 },
	{ 8921, 8927, 8932, 8938, 8943, 8949, 8954, 8960, 8965, 8971 }, { 8976, 8982, 8987, 8993, 8998, 9004, 9009, 9015, 9020, 9025 },
	{ 9031, 9036, 9042, 9047, 9053, 9058, 9063, 9069, 9074, 9079 }, { 9085, 9090, 9096, 9101, 9106, 9112, 9117, 9122, 9128, 9133 },
	{ 9138, 9143, 9149, 9154, 9159, 9165, 9170, 9175, 9180, 9186 }, { 9191, 9196, 9201, 9206, 9212, 9217, 9222, 9227, 9232, 9238 },
	{ 9243, 9248, 9253, 9258, 9263, 9269, 9274, 9279, 9284, 9289 }, { 9294, 9299, 9304, 9309, 9315, 9320, 9325, 9330, 9335, 9340 },
	{ 9345, 9350, 9355, 9360, 9365, 9370, 9375, 9380, 9385, 9390 }, { 9395, 9400, 9405, 9410, 9415, 9420, 9425, 9430, 9435, 9440 },
	{ 9445, 9450, 9455, 9460, 9465, 9469, 9474, 9479, 9484, 9489 }, { 9494, 9499, 9504, 9509, 9513, 9518, 9523, 9528, 9533, 9538 },
	{ 9542, 9547, 9552, 9557, 9562, 9566, 9571, 9576, 9581, 9586 }, { 9590, 9595, 9600, 9605, 9609, 9614, 9619, 9624, 9628, 9633 },
	{ 9638, 9643, 9647, 9652, 9657, 9661, 9666, 9671, 9675, 9680 }, { 9685, 9689, 9694, 9699, 9703, 9708, 9713, 9717, 9722, 9727 },
	{ 9731, 9736, 9741, 9745, 9750, 9754, 9759, 9763, 9768, 9773 }, { 9777, 9782, 9786, 9791, 9795, 9800, 9805, 9809, 9814, 9818 },
	{ 9823, 9827, 9832, 9836, 9841, 9845, 9850, 9854, 9859, 9863 }, { 9868, 9872, 9877, 9881, 9886, 9890, 9894, 9899, 9903, 9908 },
	{ 9912, 9917, 9921, 9926, 9930, 9934, 9939, 9943, 9948, 9952 }, { 9956, 9961, 9965, 9969, 9974, 9978, 9983, 9987, 9991, 9996 }
};

#if LOOKUP_TABLE_METHOD
/*  construct the relation of ave_rowLambda and ave_rowQP
*  m_RowLambda[i] = 0.036 * exp(0.2315 * i)
*  QP=0 => 73,  QP=1 => 92,  QP=2 => 117,  QP=3 => 147,  QP=4 => 186  are smaller than 205
*  replace it by 205
*/
#if FIX_BUG_I_QP
static const int g_AveRowLambda_I[52] = {
	205, 205, 205, 205, 205, 231, 292, 368, 463, 584, 735, 927, 1167, 1471, 1853, 2335, 2942,
	3706, 4669, 5883, 7412, 9339, 11766, 14825, 18678, 23533, 29649, 37356, 47065, 59298, 74711,
	94130, 118596, 149422, 188260, 237193, 298844, 376520, 474386, 597688, 753040, 948771, 1195377,
	1506080, 1897542, 2390753, 3012160, 3795084, 4781506, 6024321, 7590169, 9563013
};
static const int g_AveRowLambda_P[52] = {
	205, 205, 205, 205, 205, 223, 281, 354, 446, 562, 708, 893, 1125, 1417, 1785, 2249, 2834,
	3570, 4498, 5667, 7140, 8996, 11335, 14281, 17993, 22670, 28562, 35986, 45339, 57124, 71972,
	90679, 114248, 143943, 181357, 228496, 287887, 362714, 456991, 575773, 725429, 913983, 1151546,
	1450857, 1827966, 2303092, 2901715, 3655931, 4606185, 5803429, 7311862, 9212369
};
#else
static const int g_AveRowLambda[52] = {
	205, 205, 205, 205, 205, 234, 295, 372, 469, 592, 746, 940, 1186, 1495, 1884, 2375, 2994,
	3773, 4757, 5996, 7558, 9526, 12008, 15136, 19079, 24049, 30314, 38211, 48164, 60711, 76525,
	96459, 121586, 153258, 193180, 243502, 306932, 386884, 487664, 614695, 774817, 976649, 1231055,
	1551732, 1955942, 2465444, 3107666, 3917180, 4937563, 6223746, 7844966, 9888496
};
#endif
#endif	// LOOKUP_TABLE_METHOD

#if RC_GOP_QP_LIMITATION
//static int       g_BrTolerance = 2; //Bitrate overflow tolerance; Range: 0 ~ 6; Default: 2
static const int g_GopMaxQpBalanceTbl[7] = { 5,   3,   0,   -2,  -4,  -6,  -8  };
static const int g_GopOverflowThd3Tbl[7] = { 120, 160, 200, 240, 260, 280, 300 };
static const int g_GopOverflowThd2Tbl[7] = { 111, 135, 158, 182, 193, 205, 217 };
static const int g_GopOverflowThd1Tbl[7] = { 105, 115, 125, 136, 141, 146, 151 };
static const int g_GopMaxQpTbl[28] =
{
//  24  25  26  27  28  29  30  31  32  33  34  35  36  37
//  40  40  41  41  42  42  43  43  44  44  45  45  46  47
	16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 10,
//  38  39  40  41  42  43  44  45  46  47  48  49  50  51
//	48  49  50  51  51  51  51  51  51  51  51  51  51  51
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
};
#endif

int g_EvaluateBRChn = 0xFFF;
#if EVALUATE_BITRATE
    static RCBitrate rc_bitrate[MAX_RC_LOG_CHN];
    static unsigned int evaluate_period = 1;
#endif
static uint32_t g_uiDbgLv = RC_LOG_ERR;

unsigned int g_QPOverFactor = 110;
unsigned int g_MaxOverUpdateQP = 10;
unsigned int g_AdjustQPWeight = 0;
#if ADJUST_SVC_CALWEIGHT_FREQCY
unsigned int g_HPPeriod = 0;
#else
unsigned int g_HPPeriod = 4;
#endif
unsigned int g_EVBRChangePos = 100;
unsigned int g_MinStillPercent = 35;
unsigned int g_LimitEVBRStillQP = 1;
#if MAX_SIZE_BUFFER
	unsigned int g_LimitMaxFrameFactor = 90;
	unsigned int g_MSBDisableLimitQP = 1;
	unsigned int g_max_frm_bool[MAX_LOG_CHN];
#endif
#if LIMIT_GOP_DIFFERNT_BIT
	unsigned int g_MaxGOPDiffFactor = 8;	// base 16, 0 disable
	unsigned int g_MinGOPDiffFactor = 8;	// base 16, 0 disable
#endif
#if EVBR_ADJUST_QP
	unsigned int g_EVBRAdjustQPMethod = 1;
	unsigned int g_EVBRPreLimitHPct = 75;
	unsigned int g_EVBRPreLimitMPct = 50;
	unsigned int g_EVBRStillMidDQP = 1;
	unsigned int g_EVBRStillHighDQP = 2;
	unsigned int g_EvbrS2MCnt = 1;
	int g_minDeltaStillPQP = -4;
	// local variable
	uint32_t g_evbrPreLimitState[MAX_LOG_CHN];
	uint32_t g_evbrS2MLimitCnt[MAX_LOG_CHN];
	int g_evbrStillPDQP[MAX_LOG_CHN];
#endif
#if WEIGHT_CAL_RESET==4
	uint32_t g_reset_alpha_bool[MAX_LOG_CHN];
#endif
#if H26X_LOG_RC_INIT_INFO
    H26XEncRCInitInfo g_RCInitInfo[MAX_LOG_CHN] = {0};
#endif
#if EVBR_KEEP_PIC_QUALITY
	unsigned int g_LargeGOPThd = 5;
	unsigned int g_MaxKeyFrameQPStep = 1;
	unsigned int g_DisKeyFrameRRC = 1;
	unsigned int g_keepIQualityEn[MAX_LOG_CHN];
#endif
#if SMALLGOP_SVCOFF
	int g_smllGOPthre = 15;	// YCT, small GOP threshold
#endif
unsigned int g_RCResetAlpgaBetaMethod = 2;	// 0: disable, 1: bit diff, 2: pred error
#if EVALUATE_PRED_DIFF
	int g_RCStartPredGOP = 2;
	int64_t g_predDiff[MAX_LOG_CHN];
	unsigned int g_gopCnt[MAX_LOG_CHN];
#endif
#if ALPHA_BETA_MIN_MAX_ISSUE
	unsigned int g_RCUpdateAlphaCndEn = 1;	// 0: original condition, 1: check udpate alpha
#endif
#if ALPHA_BETA_RESET_COD_ADJUST
	int g_ResetAlphaMinVal = 150;		// if g_ResetAlphaMinVal = (g_RCAlphaMinValue-1) and g_ResetBetaMaxVal = (g_RCBetaMinValue-1)
	int g_ResetBetaMaxVal = -250;		// --> it can skip "alph too small, beta too big" reset condition
#endif
#if ALPHA_CONVERG_DIRECT
	#if ALPHA_CONVERG_DIRECT_TH_ADJ
	int g_AlphaConvergCntThr = 15;		// the worst case: reset once a "g_AlphaConvergCntThr" period. if g_AlphaConvergCntThr=0, skip ALPHA_CONVERG_DIRECT condition
	#else
	int g_AlphaConvergCntThr = 30;		// the worst case: reset once a "g_AlphaConvergCntThr" period. if g_AlphaConvergCntThr=0, skip ALPHA_CONVERG_DIRECT condition
	#endif	
#endif
#if ADUST_VBR2_OVERFLOW_CHECK
	int g_VBR2EstPeriodMin = 15;		// VBR2 bit_record estimate period min value
	int g_VBR2EstPeriodMax = 30;		// VBR2 bit_record estimate period max value
	int g_VBR2UndQPThd = 45;			// VBR2 underflow QP threshold
#endif
#if SUPPORT_VBR2
	unsigned int g_VBR2LimitMinMaxQP = 1;
#endif


/********** local function **********/
#if H26X_LOG_RC_INIT_INFO
    static int h26xEnc_RcSetInitInfo(TEncRateCtrl *encRC);
    static int h26xEnc_RcSetActiveQPLog(int chn, int qp, int dqp, unsigned int evbr_state, unsigned int motion_ratio);
#endif

static void dump_rc_info(TEncRateCtrl *encRC)
{
#if DUMP_RC_INIT_INFO
    if (H26X_RC_CBR == encRC->m_rcMode || H26X_RC_VBR == encRC->m_rcMode || H26X_RC_VBR2 == encRC->m_rcMode) {
        rc_dbg("{chn%d} H26x mode %d, gop %d, fr %d/%d, br %lld, qp I (%d, %d) P (%d, %d), mb cnt %d, smooth win %d, IP.deltaQP %d (ver %d.%d.%d)\n",
            (int)encRC->m_chn, (int)encRC->m_rcMode, (int)encRC->m_GOPSize, (int)encRC->m_frameRateBase, (int)encRC->m_frameRateIncr, (int64_t)encRC->m_GOPbitrate,
            (int)encRC->m_minIQp, (int)encRC->m_maxIQp, (int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)encRC->m_encRCSeq.m_numberOfMB, (int)encRC->m_smoothWindowSize,
            encRC->m_rc_deltaQPip, (H26X_RC_VERSION>>24)&0xFF, (H26X_RC_VERSION>>16)&0xFF, (H26X_RC_VERSION>>8)&0xFF);
    }
    else if (H26X_RC_EVBR == encRC->m_rcMode) {
        rc_dbg("{chn%d} H26x mode %d, gop %d, kp period %d, fr %d/%d, br %lld, qp I (%d, %d) P (%d, %d), mb cnt %d, smooth win %d, IP.deltaQP %d, kp.deltaQP %d, still QP (%d, %d, %d), s.frame cnd %d, m.ratio cnd %d (ver %d.%d.%d)\n",
            (int)encRC->m_chn, (int)encRC->m_rcMode, (int)encRC->m_GOPSize, (int)encRC->m_keyPPeriod, (int)encRC->m_frameRateBase, (int)encRC->m_frameRateIncr,
            (int64_t)encRC->m_GOPbitrate, (int)encRC->m_minIQp, (int)encRC->m_maxIQp, (int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)encRC->m_encRCSeq.m_numberOfMB,
            (int)encRC->m_smoothWindowSize, (int)encRC->m_rc_deltaQPip, (int)encRC->m_rc_deltaQPkp, (int)encRC->m_encRCEvbr.m_stillIQP,
            (int)encRC->m_encRCEvbr.m_stillKeyPQP, (int)encRC->m_encRCEvbr.m_stillPQP, (int)encRC->m_encRCEvbr.m_stillFrameThd, (int)encRC->m_encRCEvbr.m_motionRatioThd,
            (H26X_RC_VERSION>>24)&0xFF, (H26X_RC_VERSION>>16)&0xFF, (H26X_RC_VERSION>>8)&0xFF);
    }
    else
        rc_dbg("H26x[%d] mode %d, fixed qp I %d, P %d (ver %d.%d.%d)", (int)encRC->m_chn, (int)encRC->m_rcMode, (int)encRC->m_fixIQp, (int)encRC->m_fixPQp,
            (H26X_RC_VERSION>>24)&0xFF, (H26X_RC_VERSION>>16)&0xFF, (H26X_RC_VERSION>>8)&0xFF);
#endif
#if H26X_LOG_RC_INIT_INFO
    h26xEnc_RcSetInitInfo(encRC);
#endif
}

#if SMOOTH_FRAME_QP
static void TEncRateCtrl_set_pic_gop_weight(void)
{
    if (g_RCPicGOPWeight_cbr < 0 || g_RCPicGOPWeight_cbr > 100) {
        rc_err("cbr pic gop weight out of range %d\n\r", g_RCPicGOPWeight_cbr);
        goto exit_set;
    }
    if (g_RCPicGOPWeight_vbr < 0 || g_RCPicGOPWeight_vbr > 100) {
        rc_err("vbr pic gop weight out of range %d\n\r", g_RCPicGOPWeight_vbr);
        goto exit_set;
    }
    if (g_RCPicGOPWeight_evbr < 0 || g_RCPicGOPWeight_evbr > 100) {
        rc_err("evbr pic gop weight out of range %d\n\r", g_RCPicGOPWeight_evbr);
        goto exit_set;
    }

    g_RCWeightPicTargetBitInGOP_cbr = 2048*g_RCPicGOPWeight_cbr/100;
    g_RCWeightPicRargetBitInBuffer_cbr = 2048 - g_RCWeightPicTargetBitInGOP_cbr;
    g_RCWeightPicTargetBitInGOP_vbr = 2048*g_RCPicGOPWeight_vbr/100;
    g_RCWeightPicRargetBitInBuffer_vbr = 2048 - g_RCWeightPicTargetBitInGOP_vbr;
    g_RCWeightPicTargetBitInGOP_evbr = 2048*g_RCPicGOPWeight_evbr/100;
    g_RCWeightPicRargetBitInBuffer_evbr = 2048 - g_RCWeightPicTargetBitInGOP_evbr;

    rc_dbg("GOP pic weight. CBR: gop %d, pic %d, VBR: gop %d, pic %d, EVBR: gop %d, pic %d\n\r",
        g_RCWeightPicTargetBitInGOP_cbr, g_RCWeightPicRargetBitInBuffer_cbr, g_RCWeightPicTargetBitInGOP_vbr,
        g_RCWeightPicRargetBitInBuffer_vbr, g_RCWeightPicTargetBitInGOP_evbr, g_RCWeightPicRargetBitInBuffer_evbr);

exit_set:
    return;
}
#endif

static int division_algorithm(int a, int b)
{
	if (0 == a || 0 == b)
		return 0;
	while (a > 0 && b > 0) {
		if (a > b)
			a = a % b;
		else
			b = b % a;
	}
	if (0 == a)
		return b;
	if (0 == b)
		return a;
	return 0;
}

//static int64_t division(int64_t a_num, int64_t b_den)
static uint64_t div_long(uint32_t high, uint32_t low, uint32_t den)
{
    unsigned int f1, f2, f3, f4, f5;
    unsigned int r1, r2, r3, r4;
    unsigned int temp;
    uint64_t result;

    if (0 == den) {
        result = (((uint64_t)high<<32)|low);
        return result;
    }

    if (den > 0xFFFFFF) {
        low = (low>>8) | ((high&0xFF)<<24);
        high = (high>>8);
        den = (den>>8);
    }

    f1 = high / den;
    r1 = high % den;

    temp = (r1 << 8) + ((low & 0xff000000) >> 24);
    f2 = temp / den ;
    r2 = temp % den ;

    temp = (r2 << 8) + ((low & 0xff0000) >> 16);
    f3 = temp / den ;
    r3 = temp % den ;

    temp = (r3 << 8) + ((low & 0xff00) >> 8);
    f4 = temp / den ;
    r4 = temp % den ;

    temp = (r4 << 8) + (low & 0xff);
    f5 = temp / den ;

    result = ((uint64_t)f1 << 24);
    result = (result << 8) + (((uint64_t)f2 << 24) + ((uint64_t)f3 << 16) + ((uint64_t)f4 << 8) + (uint64_t)f5);

    return result;
}

int64_t division(int64_t a_num, int b_den)
{
    unsigned int sign;
    unsigned int high, low;
    int64_t acc;
    if (0 == b_den)
        return a_num;
    sign = (a_num < 0) ^ (b_den < 0);
    if (a_num < 0)
        a_num = -a_num;
    if (b_den < 0)
        b_den = -b_den;
    high = (a_num>>32) & 0xFFFFFFFF;
    low = a_num & 0xFFFFFFFF;
    if (0 == high)
        acc = low / b_den;
    else
        acc = div_long(high, low, b_den);
    if (sign)
        acc = -acc;
    return acc;
}

static int lookupLogListTable(int value, bool multiply)
{
	int record_i, record_j;
	int remainder;
	int ret_value;

	if (value - 1 < 0) {
		ret_value = 0;
	}
    else {
		int rounding = 0;
		int shift_right = 0;
		int count = -1;
        int tmpvalue = value;
		int tmp;
		int estimateLogValue;

		/* adjust displacement */
		if (0 == multiply)
			shift_right = TIMES_10_NUM;

		while (1) {
			if (tmpvalue > 9999) {
				rounding = (tmpvalue % 10 >= 5) ? 1 : 0;
				tmpvalue /= 10;
				shift_right++;
			}
			else
				break;
		}
		tmpvalue = tmpvalue + rounding;

		/* recod the position of log list*/
		tmp = tmpvalue;
		while (tmp) {
			tmp /= 10;
			count++;
		}

		// avoid g_log_list out of boundary
		if (10000 == tmpvalue){ /* worse case */
			ret_value = (count + shift_right - TIMES_10_NUM) * TIMES_10;
			return ret_value;
		}

		switch (count) {
		case 0:
			record_i = (tmpvalue - 1) * 10;
			record_j = 0;
			remainder = -1;
			break;
		case 1:
			record_i = tmpvalue - 10;
			record_j = 0;
			remainder = -1;
			break;
		case 2:
			record_i = tmpvalue / 10 - 10;
			record_j = tmpvalue % 10;
			remainder = -1;
			break;
		default:
			record_i = tmpvalue / 100 - 10;
			record_j = (tmpvalue / 10) % 10;
			remainder = tmpvalue % 10 - 1;
			break;
		}

		/* look up the log list table */
		if (remainder < 0) {
			estimateLogValue = g_log_list[record_i][record_j];
		} else {
			int tmp_i, tmp_j, estimate;
			bool worse_case_flag;
			if (9 == record_j) {
				tmp_i = record_i + 1;
				tmp_j = 0;
				worse_case_flag = (90 == tmp_i) ? 1 : 0;
			}
			else {
				tmp_i = record_i;
				tmp_j = record_j + 1;
				worse_case_flag = 0;
			}
			if (worse_case_flag) {
				estimate = remainder * (10000 - g_log_list[89][9]) + ((10000 - g_log_list[89][9]) >> 1);
			}
			else {
				estimate = remainder * (g_log_list[tmp_i][tmp_j] - g_log_list[record_i][record_j]) + ((g_log_list[tmp_i][tmp_j] - g_log_list[record_i][record_j]) >> 1);
			}
			estimate /= 10;

			estimateLogValue = g_log_list[record_i][record_j] + estimate;
		}

		ret_value = estimateLogValue - (TIMES_10_NUM - count - shift_right) * TIMES_10;
	}

	return ret_value;
}

static int reverse_lookup_table(int coeff, int value)
{
	int tmp_1 = 0;
	int tmp_2 = 0;
	int tmp_3 = 0;
	int tmp_i;
	int i, j;
	int record_cur_i = 0, record_cur_j = 0;
	int record_next_i = 0, record_next_j = 0;
	int64_t ret;
	int estimate;
	int tmp_div;
	int rounding_term;
	bool negative_flag = 0;

	tmp_1 = value >> SHIFT_11;

	if (tmp_1 < 0){
		tmp_2 = tmp_1 % 10000 + 10000;
		tmp_3 = (-tmp_1) / 10000 + 1;
		negative_flag = 1;
	}
	else{
		tmp_2 = tmp_1 % 10000;
		tmp_3 = tmp_1 / 10000;
	}

	if (tmp_2 < g_log_list[0][1] && 0 == tmp_3){
		return coeff;
	}
	else{
		if (tmp_2 <= g_log_list[15][0]){
			tmp_i = 0;
		}
		else if (tmp_2 <= g_log_list[30][0]){
			tmp_i = 16;
		}
		else if (tmp_2 <= g_log_list[45][0]){
			tmp_i = 31;
		}
		else if (tmp_2 <= g_log_list[60][0]){
			tmp_i = 46;
		}
		else if (tmp_2 <= g_log_list[75][0]){
			tmp_i = 61;
		}
		else{
			tmp_i = 76;
		}

		for (i = tmp_i; i <= tmp_i + 15; i++){
			if (i > 89){
				record_cur_i = 89;
				break;
			}
			if (tmp_2 > g_log_list[i][0]){
				continue;
			}
			else{
				record_cur_i = i - 1;
				break;
			}
		}
        if (-1 == record_cur_i){ /* tmp_2 = 0, worse case */
            record_cur_i = 0;
            record_cur_j = 0;
            record_next_i = 0;
            record_next_j = 1;
        }
        else {
    		for (j = 0; j < 10; j++){
    			if (tmp_2 > g_log_list[record_cur_i][j]){
    				if (9 == j){
    					record_cur_j = 9;
    					//record_next_i = record_cur_i + 1;
    					record_next_i = (record_cur_i == (LOG_LIST_I-1)) ? (LOG_LIST_I-1) : record_cur_i + 1;
    					record_next_j = 0;
    				}
    				continue;
    			}
    			else{
    				record_next_i = record_cur_i;
    				record_next_j = j;
    				record_cur_j = j - 1;
    				break;
    			}
    		}
        }
	}

	if (!negative_flag){
		if (tmp_3 < 3){
			tmp_div = g_log_list[record_next_i][record_next_j] - g_log_list[record_cur_i][record_cur_j];
			estimate = 10 * (tmp_2 - g_log_list[record_cur_i][record_cur_j]) / tmp_div;
			rounding_term = (estimate > 4) ? 1 : 0;
			ret = (record_cur_i + 10) * 10 + record_cur_j + rounding_term;
			ret = coeff * ret;
			for (i = 2; i > tmp_3; i--){
				ret = division(ret, 10);
			}
			if (0 == ret){
				rc_inf("warning : value too small, force to 0 \r\n");
				return (int)ret;
			}
		}
		else if (tmp_3 < 4){
			tmp_div = g_log_list[record_next_i][record_next_j] - g_log_list[record_cur_i][record_cur_j];
			estimate = 10 * (tmp_2 - g_log_list[record_cur_i][record_cur_j]) / tmp_div + 1;
			rounding_term = (estimate > 4) ? estimate + 1 : estimate;
			ret = (record_cur_i + 10) * 100 + record_cur_j * 10 + rounding_term;
			ret = coeff * ret;
		}
		else{
			tmp_div = g_log_list[record_next_i][record_next_j] - g_log_list[record_cur_i][record_cur_j];
			estimate = 100 * (tmp_2 - g_log_list[record_cur_i][record_cur_j]) / tmp_div + 16;
			ret = (record_cur_i + 10) * 1000 + record_cur_j * 100 + estimate;
			for (i = 4; i < tmp_3; i++){
				ret *= 10;
				if (ret > MAX_INTEGER){
					ret = (int)MAX_INTEGER;
					rc_inf("warning : over pow(2, 31), force to pow(2, 31) \r\n");
					return (int)ret;
				}
			}
			ret = coeff * ret;
		}
	}
	else{
		tmp_div = g_log_list[record_next_i][record_next_j] - g_log_list[record_cur_i][record_cur_j];
		estimate = 100 * (tmp_2 - g_log_list[record_cur_i][record_cur_j]) / tmp_div + 16;
		ret = ((int64_t)record_cur_i + 10) * 1000 + (int64_t)record_cur_j * 100 + estimate;
		ret = coeff * ret;
		for (i = 0; i < tmp_3 + 4; i++){
			ret = division(ret, 10);
			if (0 == ret){
				rc_inf("warning : value too small, force to 0 \r\n");
				break;
			}
		}
	}

	ret = RC_CLIP3(0, (int64_t)MAX_INTEGER, ret);

	return (int)ret;

}

static int calculatePow(int coeff, int value, int power, bool multiply)
{
	int64_t tmp_64;
	int tmp_value;
	int ret;

	tmp_64 = (int64_t)lookupLogListTable(value, multiply) * power;
	tmp_value = RC_CLIP3((int64_t)(-MAX_INTEGER), (int64_t)MAX_INTEGER, tmp_64);

	ret = reverse_lookup_table(coeff, tmp_value);

	return ret;
}

#if UPDATE_QP_BY_PSNR
static int calculatePSNR(int64_t sse, unsigned int resolution)
{
	int64_t tmp_1 = sse;
	int64_t tmp_2 = 255LL * 255LL * (int64_t)resolution;
	int64_t tmp_3;
	int tmp_div;
	int tmp_cal;
	int ret;
	int record_count;
	int count = 0;
	bool multiply;

	/* tmp_1 >= pow(2, 40) */
	while (tmp_1 >= 0x800000LL){
		tmp_1 >>= 1;
		count++;
	}

	record_count = 17 - count;
	tmp_2 = tmp_2 << record_count;

	/* sse must be 40 bits below */
	tmp_1 = sse >> count;
	tmp_div = (int)RC_CLIP3(0, (int64_t)MAX_INTEGER, tmp_1);

	/*
	 *   pow(255, 2) / MSE
	 */
	tmp_3 = division(tmp_2, tmp_div);		if (tmp_3 > 0x8000000){		tmp_3 = tmp_3 >> 17;		multiply = 0;	}	else{		tmp_3 = (tmp_3 * (int64_t)TIMES_10) >> 17;		multiply = 1;	}

	if (0 == tmp_3){
		ret = 0;
		rc_inf("[PSNR] ERROR - MSE is bigger than pow(255, 2)\r\n");
		return ret;
	}
	else if (tmp_3 > (int64_t)MAX_INTEGER){
		tmp_cal = (int)MAX_INTEGER;
		rc_inf("[PSNR] WARRING - PSNR's calculation exceed the upper bound\r\n");
	}
	else{
		tmp_cal = (int)tmp_3;
	}

    /* PSNR = 10 * log ( pow(255, 2) * resolution / sse ) */
	ret = lookupLogListTable(tmp_cal, multiply);

	/* output : PSNR * 10000 */
	// Tuba 20171213: PSNR * 100

	return ret/10;
}
#endif

#if BITRATE_OVERFLOW_CHECK
static void RCBitrateCheckInit(TRCBitrateRecord *encRCBr, int64_t gopBitrate)
{
    memset(encRCBr, 0, sizeof(TRCBitrateRecord));
}
static int RCBitRecordUpdate(TRCBitrateRecord *encRCBr, int frameBit, int measureSize)
{
    if (encRCBr->recrod_num >= measureSize) {
        encRCBr->bitrate_record -= encRCBr->bit_record[encRCBr->pop_idx];
        encRCBr->pop_idx = (encRCBr->pop_idx + 1) % MAX_GOP_SIZE;
    }
    else
        encRCBr->recrod_num++;
    encRCBr->bitrate_record += frameBit;
    encRCBr->bit_record[encRCBr->push_idx] = frameBit;
    encRCBr->push_idx = (encRCBr->push_idx + 1) % MAX_GOP_SIZE;
    return 0;
}
/* 1: overflow, 0: not overflow */
static int RCBitrateOverflowCheck(TRCBitrateRecord *encRCBr, int64_t thd)
{
    if (thd > 0 && encRCBr->bitrate_record > thd)
        return 1;
    return 0;
}
#if EVBR_MEASURE_BR_METHOD
static int RCEVBRBitrateOverflowCheck(TRCBitrateRecord *encRCBr, int64_t thd,
	int GOPSize, int SPperiod, int lastIsize, int lastKPsize)
{
	int64_t bitrate;// = encRCBr->bitrate_record * (int64_t)GOPSize + (int64_t)(GOPSize - SPperiod) * (int64_t)lastKPsize + (int64_t)SPperiod * (int64_t)lastIsize;

#if EVBR_XA
    if (SPperiod == 0){
        bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize - 1) + (int64_t)lastIsize*(int64_t)(encRCBr->recrod_num+1);
    }
    else {
        //bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize - GOPSize / SPperiod) + (int64_t)(GOPSize - SPperiod) * (int64_t)lastKPsize + (int64_t)SPperiod * (int64_t)lastIsize;
        bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize) + (int64_t)(GOPSize - SPperiod) * (int64_t)lastKPsize + (int64_t)SPperiod * (int64_t)lastIsize;
    }
    if (thd > 0 && bitrate > thd*GOPSize){
        int64_t ratio = division(bitrate*100, GOPSize);
        int64_t tmp_thd = thd;
        int div_bit = 0;

        while (tmp_thd > 0x7FFFFFFF) {
            tmp_thd >>= 1;
            div_bit++;
        }
        ratio = ratio >> div_bit;
        tmp_thd = thd >> div_bit;
        ratio = division(ratio, (int)tmp_thd);
        return (int)ratio;
        //return ((int)(bitrate / (int64_t)GOPSize * 100 / thd));
    }
#else
    bitrate = encRCBr->bitrate_record * (int64_t)GOPSize + (int64_t)(GOPSize - SPperiod) * (int64_t)lastKPsize + (int64_t)SPperiod * (int64_t)lastIsize;

    if (SPperiod > 0) {
        if (thd > 0 && bitrate > thd*GOPSize)
            return 1;
    }
    else {
        if (thd > 0 && encRCBr->bitrate_record > thd)
            return 1;
    }
#endif
    return 0;
}
static int RCEVBRBitrateRatio(TRCBitrateRecord *encRCBr, int64_t thd,
	int GOPSize, int SPperiod, int lastIsize, int lastKPsize)
{
	int64_t bitrate;// = encRCBr->bitrate_record * (int64_t)GOPSize + (int64_t)(GOPSize - SPperiod) * (int64_t)lastKPsize + (int64_t)SPperiod * (int64_t)lastIsize;
	int64_t ratio;// = division(bitrate*100, GOPSize);
	int64_t tmp_thd;// = thd;
	int div_bit = 0;

    if (SPperiod == 0){
        bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize - 1) + (int64_t)lastIsize*(int64_t)(encRCBr->recrod_num+1);
    }
    else {
        bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize) + (int64_t)(GOPSize - SPperiod) * (int64_t)lastKPsize + (int64_t)SPperiod * (int64_t)lastIsize;
	}

	ratio = division(bitrate*100, GOPSize);
	tmp_thd = thd;

    while (tmp_thd > 0x7FFFFFFF) {
        tmp_thd >>= 1;
        div_bit++;
    }
    ratio = ratio >> div_bit;
    tmp_thd = thd >> div_bit;
    ratio = division(ratio, (int)tmp_thd);
    return (int)ratio;
}

#endif  // EVBR_MEASURE_BR_METHOD

#if SUPPORT_VBR2
static int RCBitRecordUpdate2(TRCBitrateRecord *encRCBr, int frameBit, int measureSize, int svcLayer)
{
	int pos;
	int num = measureSize;
	#if ADUST_VBR2_OVERFLOW_CHECK
		int weight1 = 1;
		int weight2 = 1;
	#else
	int weight1 = (svcLayer == 2) ? (measureSize >> 3) : (measureSize >> 2);
	int weight2 = measureSize >> 3;
	#endif
	
	if (encRCBr->recrod_num >= measureSize)
	{
		//		encRCBr->bitrate_record -= encRCBr->bit_record[encRCBr->pop_idx];
		//current 1/4
		encRCBr->bitrate_record = ((int64_t)weight1) * frameBit;
		num -= weight1;

		//-1      1/4
		pos = encRCBr->push_idx - 1;
		pos = pos >= 0 ? pos : (pos + MAX_GOP_SIZE);
		encRCBr->bitrate_record += ((int64_t)weight1) * encRCBr->bit_record[pos];
		num -= weight1;

		//-2      1/8
		pos--;
		pos = pos >= 0 ? pos : (pos + MAX_GOP_SIZE);
		encRCBr->bitrate_record += ((int64_t)weight2) * encRCBr->bit_record[pos];
		num -= weight2;

		//-3      1/8
		pos--;
		pos = pos >= 0 ? pos : (pos + MAX_GOP_SIZE);
		encRCBr->bitrate_record += ((int64_t)weight2) * encRCBr->bit_record[pos];
		num -= weight2;

		//rest    1/measureSize
		for (; num >= 0; num--)
		{
			pos--;
			pos = pos >= 0 ? pos : (pos + MAX_GOP_SIZE);
			encRCBr->bitrate_record += encRCBr->bit_record[pos];
		}
	}
	else
	{
		encRCBr->recrod_num++;
		encRCBr->bitrate_record += frameBit;
	}
	//	encRCBr->bitrate_record += frameBit;
	encRCBr->bit_record[encRCBr->push_idx] = frameBit;
	encRCBr->push_idx = (encRCBr->push_idx + 1) % MAX_GOP_SIZE;
	return 0;
}

static int RCVBR2BitrateOverflowCheck(TRCBitrateRecord *encRCBr, int64_t thd, int GOPSize, int SPPeriod, int lastISize, int lastSPSize, int isI, int isSP)
{
	int64_t bitrate;
	int64_t ratio;
	int64_t tmp_thd;
	int div_bit = 0;

	//	if (isI == 0 && isSP == 0)
	{
		if (SPPeriod == 0)
		{
			#if ADUST_VBR2_OVERFLOW_CHECK			
			    int avePBit = 0;
				avePBit = division(encRCBr->bitrate_record, encRCBr->recrod_num + 1);
				bitrate = avePBit * (int64_t)(GOPSize - 1) + (int64_t)lastISize;
			#else
				bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize - 1) + (int64_t)lastISize * (int64_t)encRCBr->recrod_num;
			#endif
		}
		else
		{
			bitrate = encRCBr->bitrate_record * (int64_t)(GOPSize - GOPSize / SPPeriod) + (int64_t)(GOPSize - SPPeriod) * (int64_t)lastSPSize + (int64_t)SPPeriod * (int64_t)lastISize;
		}
	}

#if FIX_BUG_ALL_INTRA
	if (GOPSize == 1)
	{
		#if ADUST_VBR2_OVERFLOW_CHECK
		bitrate = division(encRCBr->bitrate_record, encRCBr->recrod_num + 1); 
		#else
		bitrate = division(encRCBr->bitrate_record * (int64_t)(encRCBr->recrod_num + 1), encRCBr->recrod_num);
		#endif
	}
#endif

	ratio = division(bitrate * 100, GOPSize);
	tmp_thd = thd;
	while (tmp_thd > 0x7FFFFFFF) {
		tmp_thd >>= 1;
		div_bit++;
	}
	ratio = ratio >> div_bit;
	tmp_thd = thd >> div_bit;
	ratio = division(ratio, (int)tmp_thd);
	return (int)ratio;
	//return ((int)(bitrate / (int64_t)GOPSize * 100 / thd));
}
#endif
#endif

#if EVBR_ADJUST_QP
static int RCSetEvbrPreLimitstate(int chn, RC_EVBR_LIMIT_STATE state)
{
	if (chn < MAX_LOG_CHN)
		g_evbrPreLimitState[chn] = (uint32_t)state;
	return 0;
}
static uint32_t RCGetEvbrPreLimitstate(int chn)
{
	if (chn < MAX_LOG_CHN)
		return g_evbrPreLimitState[chn];
	return (uint32_t)RC_EVBR_LIMIT_LOW;
}
static int RCSetEvbrS2MCnt(int chn, uint32_t value)
{
	if (chn < MAX_LOG_CHN)
		g_evbrS2MLimitCnt[chn] = value;
	return 0;
}
static int RCUpdateS2MQP(TEncRateCtrl *encRC, int qp)
{
	if (encRC->m_chn < MAX_LOG_CHN) {
		if (g_evbrS2MLimitCnt[encRC->m_chn] > 0 && 0 == encRC->m_SP_sign) {
			qp = RC_MAX(encRC->m_encRCEvbr.m_stillPQP, qp);
			encRC->m_currQP = qp;
			g_evbrS2MLimitCnt[encRC->m_chn]--;
		}
	}
	return qp;
}
static int RCClearStillPDQP(int chn)
{
	if (chn < MAX_LOG_CHN)
		g_evbrStillPDQP[chn] = 0;
	return 0;
}
static int RCGetStillPDQP(int chn)
{
	if (chn < MAX_LOG_CHN)
		return g_evbrStillPDQP[chn];
	return 0;
}
static int RCIncStillPDQP(int chn)
{
	if (chn < MAX_LOG_CHN)
		g_evbrStillPDQP[chn] = RC_MIN(g_evbrStillPDQP[chn]+1, 0);
	return 0;
}
static int RCDecStillPDQP(int chn)
{
	if (chn < MAX_LOG_CHN)
		g_evbrStillPDQP[chn] = RC_MAX(g_evbrStillPDQP[chn]-1, g_minDeltaStillPQP);
	return 0;
}
#endif
#if WEIGHT_CAL_RESET==4
static int RCSetResetAlphaBool(int chn, int value)
{
	if (chn < MAX_LOG_CHN)
		g_reset_alpha_bool[chn] = value;
	return 0;
}
static int RCGetResetAlphaBool(int chn)
{
	if (chn < MAX_LOG_CHN)
		return g_reset_alpha_bool[chn];
	return 0;
}
#endif

#if EVBR_KEEP_PIC_QUALITY
static int RCSetKeepIQualityEn(int chn, int en)
{
	if (chn < MAX_LOG_CHN)
		g_keepIQualityEn[chn] = en;
	return 0;
}
static int RCGetKeppIQualityEn(int chn)
{
	if (chn < MAX_LOG_CHN)
		return (int)g_keepIQualityEn[chn];
	return 0;
}
#endif
#if EVALUATE_PRED_DIFF
static int RCClearPredDiff(int chn)
{
	if (chn < MAX_LOG_CHN) {
		g_predDiff[chn] = 0;
		g_gopCnt[chn] = 0;
	}
	return 0;
}
static int RCSetPredDiff(int chn, int64_t pred_diff)
{
	if (chn < MAX_LOG_CHN)
		g_predDiff[chn] = pred_diff;
	return 0;
}
static int64_t RCGetPredDiff(int chn)
{
	if (chn < MAX_LOG_CHN)
		return g_predDiff[chn];
	return 0;
}
static int RCGetGOPCnt(int chn)
{
	if (chn < MAX_LOG_CHN)
		return g_gopCnt[chn];
	return 0;
}
static int RCIncGOPCnt(int chn)
{
	if (chn < MAX_LOG_CHN)
		g_gopCnt[chn]++;
	return 0;
}
#endif

static void RCSeqInit(TEncRCSeq *encRCSeq, int GOPSize, int chn)
{
	int i;

	encRCSeq->m_targetRate = 0;
	encRCSeq->m_fbase = 0;
	encRCSeq->m_fincr = 0;
	encRCSeq->m_targetBits = 0;
	encRCSeq->m_GOPSize = 0;
	encRCSeq->m_mbCount = 0;
	encRCSeq->m_numberOfLevel = 0;
	encRCSeq->m_averageBits = 0;
	encRCSeq->m_numberOfMB = 0;
	encRCSeq->m_bitsDiff = 0;
	#if EVALUATE_PRED_DIFF
	RCClearPredDiff(chn);
	#endif
	#if ALPHA_CONVERG_DIRECT
	encRCSeq->m_AlphaConvergeCnt = 0;
	#endif

	encRCSeq->m_adaptiveBit = 0;
	encRCSeq->m_lastLambda = 0;

    for (i = 0; i < RC_MAX_FRAME_LEVEL; i++) {
		encRCSeq->m_bitsRatio[i] = 1;
		encRCSeq->m_GOPID2Level[i] = 1;
    }

	for (i = 0; i < 8; i++) {
		encRCSeq->m_picPara[i].m_alpha = 6554;        // 3.2003 * 2048;
		encRCSeq->m_picPara[i].m_beta = -2800;        // -1.367 * 2048;
	}
}

static void RCGOPInit(TEncRCGOP *encRCGOP, int GOPSize)
{
	int i;

	encRCGOP->m_numPic = 0;
	encRCGOP->m_targetBits = 0;
	encRCGOP->m_picLeft = 0;
	encRCGOP->m_bitsLeft = 0;

    for (i = 0; i < RC_MAX_FRAME_LEVEL; i++)
        encRCGOP->m_picTargetBitInGOP[i] = 0;
}

static void RCPicInit(TEncRCPic *encRCPic)
{
	encRCPic->m_frameLevel = 0;
	encRCPic->m_numberOfMB = 0;
	encRCPic->m_targetBits = 0;
	encRCPic->m_estHeaderBits = 0;
	encRCPic->m_estPicLambda = 0;
	encRCPic->m_bitsLeft = 0;
	encRCPic->m_pixelsLeft = 0;
	encRCPic->m_picActualHeaderBits = 0;
	encRCPic->m_picActualBits = 0;
	encRCPic->m_estPicQP = 26;
	encRCPic->m_picQP = 26;
	encRCPic->m_picLambda = 0;
	encRCPic->m_totalCostIntra = 0;
}

static void RCInit(TEncRateCtrl *encRC)
{
    RCSeqInit(&encRC->m_encRCSeq, encRC->m_GOPSize, encRC->m_chn);
    RCGOPInit(&encRC->m_encRCGOP, encRC->m_GOPSize);
    RCPicInit(&encRC->m_encRCPic);
}

static void GOPRCInit(TEncRateCtrl *encRC,
    int target_bitrate,
    int idr_period,
    int fincr,
    int fbase)
{
    int i;

	encRC->m_GOPbitrate = (int64_t)target_bitrate * (int64_t)idr_period * (int64_t)fincr;
	encRC->m_GOPbitrate = division(encRC->m_GOPbitrate, fbase);
    encRC->m_GOPMinBitrate = 200;
	encRC->m_GOPSize = idr_period;
	encRC->m_maxIQp = 51;
	encRC->m_minIQp = 1;
	encRC->m_maxPQp = 51;
	encRC->m_minPQp = 1;
	encRC->m_lastSliceType = RC_I_SLICE;
	encRC->m_slicetype = RC_I_SLICE;
	encRC->ave_ROWQP = 26;
    encRC->m_activeQP = 26;
	encRC->m_currQP = 26;
	encRC->m_currlambda = 0;
#if HANDLE_BITSTREAM_OVERFLOW
    encRC->m_overflowIncQP = 0;
#endif
    for (i = 0; i < RC_MAX_FRAME_LEVEL; i++)
    {
        encRC->m_lastLevelLambda[i] = -1;
        encRC->m_lastLevelQP[i] = g_RCInvalidQPValue;
    }
    encRC->m_lastpQP = g_RCInvalidQPValue;
    encRC->m_lastPicQP = g_RCInvalidQPValue;
    encRC->m_lastValidQP = g_RCInvalidQPValue;
    encRC->m_lastPicLambda = -1;
    encRC->m_lastValidLambda = -1;

#if BITRATE_OVERFLOW_CHECK
    RCBitrateCheckInit(&encRC->m_encRCBr, encRC->m_GOPbitrate);
#endif
	encRC->m_SP_index = 0; // only  RC start/restart needs to be initial
#if EVBR_MEASURE_BR_METHOD
	encRC->m_lastIsize = 0;
	encRC->m_lastKeyPsize = 0;
#endif

#if EVBR_MODE_SWITCH_NEW_METHOD
	encRC->m_record_lastQP = 26;
	encRC->m_record_lastLambda = 29626; // calculatePow(1167, 20000, 9556, 1);
#endif
}

static int RCGetSmoothWindow(TEncRateCtrl *encRC, int rc_mode)
{
    int base_size;
    unsigned int smooth_window;

    if (0 == encRC->m_frameRateIncr || 0 == encRC->m_frameRateBase)
        base_size = encRC->m_GOPSize;
    else
        base_size = RC_MAX(encRC->m_GOPSize, encRC->m_frameRateBase/encRC->m_frameRateIncr);
    smooth_window = base_size * 4 / 3;

    return smooth_window;
}

static int TEncRateCtrol_getInitQuant(TEncRateCtrl *encRC)
{
    int resolution = encRC->m_encRCSeq.m_mbCount;
    int64_t tmp_bpp = (int64_t)encRC->m_encRCSeq.m_targetRate*encRC->m_frameRateIncr;
    int tmp_div = encRC->m_frameRateBase * encRC->m_encRCSeq.m_mbCount;
    int sliceQP;// = 26;

#if USER_INIT_QUANT
    if (encRC->m_initIQp <= 51) {
        sliceQP = encRC->m_initIQp;
        goto exit_quant;
    }
#endif

    tmp_bpp <<= (SHIFT_11 - 1);
    tmp_bpp = division(tmp_bpp, tmp_div);

   if (resolution <= 396) {
        if (tmp_bpp <= 39321)          //0.15 * 1024
            sliceQP = 40;
        else if (tmp_bpp <= 117964)     //0.45 * 1024
            sliceQP = 30;
        else if (tmp_bpp <= 2235929)    //0.9  * 1024
            sliceQP = 20;
        else
            sliceQP = 10;
    }
    else {
/************************
    qp = -6.015*ln(bpp) + 18.21
    bpp                 QP
    0.026712    40.00019389
    0.061330    35.00078925
    0.140843    30.00005855
    0.323400    25.00012497
    0.742600    20.00005039
    1.705100    15.00025308
    3.915400    10.00002127
*************************/
        if (tmp_bpp <= 7002)
            sliceQP = 40;
        else if (tmp_bpp <= 16077)
            sliceQP = 35;
        else if (tmp_bpp <= 36921)
            sliceQP = 30;
        else if (tmp_bpp <= 84777)
            sliceQP = 25;
        else if (tmp_bpp <= 194668)
            sliceQP = 20;
        else if (tmp_bpp <= 446981)
            sliceQP = 15;
        else
            sliceQP = 10;
    }
    //rc_dbg("init qp = %d (bpp %lld)\r\n", (int)(sliceQP), (long long)(tmp_bpp));
#if USER_INIT_QUANT
exit_quant:
#endif
    return sliceQP;
}

static void TEncRateCtrl_set_param(TEncRateCtrl* encRC, H26XEncRCParam *pRCParam)
{
/*
#if MAX_SIZE_BUFFER
	int bufferratio[52] = {
		100, 100, 90, 80, 72, 64, 57, 51, 45, 40,
		36, 32, 29, 26, 23, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
		20, 20
	};
	int max_size;
#endif
*/
    encRC->m_chn = pRCParam->uiEncId;
    encRC->m_initIQp = pRCParam->uiInitIQp;
    encRC->m_minIQp = pRCParam->uiMinIQp;
    encRC->m_maxIQp = pRCParam->uiMaxIQp;
    encRC->m_initPQp = pRCParam->uiInitPQp;
    encRC->m_minPQp = pRCParam->uiMinPQp;
    encRC->m_maxPQp = pRCParam->uiMaxPQp;
    encRC->m_fixIQp = pRCParam->uiFixIQp;
    encRC->m_fixPQp = pRCParam->uiFixPQp;
    encRC->m_rowRCEnable = pRCParam->uiRowLevelRCEnable;
    encRC->m_rcMode = pRCParam->uiRCMode;
    encRC->m_frameRateBase = pRCParam->uiFrameRateBase;
    encRC->m_frameRateIncr = pRCParam->uiFrameRateIncr;
    encRC->m_staticTime = pRCParam->uiStaticTime;
    encRC->m_motionAQStrength = pRCParam->iMotionAQStrength;
    // must after set fbase & fincr
	encRC->m_currQP = TEncRateCtrol_getInitQuant(encRC);
    encRC->m_inumpic = 0;
    //if (0 == pRCParam->uiCtuSize)
    //    encRC->m_ctuSize = pRCParam->uiPicSize/4096;
    //else
    //    encRC->m_ctuSize = pRCParam->uiCtuSize;
    //encRC->m_ctuSize = (0 == encRC->m_ctuSize ? 1 : encRC->m_ctuSize);
/*
#if MAX_SIZE_BUFFER
	max_size = pRCParam->uiPicSize * 3 * bufferratio[encRC->m_minIQp];
	max_size = division(max_size, 25);
	encRC->m_max_frm_bit = max_size * 19;
	encRC->m_max_frm_bit = division(encRC->m_max_frm_bit, 20);
#endif
*/
	encRC->m_rc_deltaQPip = RC_CLIP3(-100, 100, pRCParam->iIPWeight);
	encRC->m_rc_deltaQPkp = RC_CLIP3(-100, 100, pRCParam->iKPWeight);
    if (g_AdjustQPWeight) {
        encRC->m_deltaLT_QP = RC_CLIP3(-100, 100, pRCParam->iIPWeight);  //HK QP: LT=I
    } else {
        encRC->m_deltaLT_QP = RC_CLIP3(-100, 100, pRCParam->iLTWeight);
    }

	encRC->m_deltaP2_QP = RC_CLIP3(-100, 100, pRCParam->iP2Weight);
	encRC->m_deltaP3_QP = RC_CLIP3(-100, 100, pRCParam->iP3Weight);

    encRC->m_iPrevPicBitsBias[RC_I_SLICE] = 0;
    encRC->m_iPrevPicBitsBias[RC_P_SLICE] = 0;
    encRC->m_iPrevPicBitsBias[RC_B_SLICE] = 0;

	encRC->m_HP_sign = 0;
	encRC->m_HP_index = 0;
	encRC->m_P2_frm_num = 0;
	encRC->m_P3_frm_num = 0;

	#if ADJUST_SVC_CALWEIGHT_FREQCY
	if (pRCParam->HP_period == 0)
		encRC->m_HP_period = encRC->m_GOPSize;//encRC->m_encRCGOP.m_numPic;
	else if (pRCParam->HP_period > 3)
	#else
	if (pRCParam->HP_period > 3)
	#endif
		encRC->m_HP_period = (pRCParam->HP_period / 4) * 4;
	else
		encRC->m_HP_period = 4;

    if (encRC->m_staticTime > 0) {
        encRC->m_smoothWindowSize = pRCParam->uiGOP * encRC->m_staticTime + 10;
    }
    else
        encRC->m_smoothWindowSize = RCGetSmoothWindow(encRC, encRC->m_rcMode);

    encRC->m_minGOPQP = 26;
    encRC->m_maxGOPQP = 26;
#if VBR_PRELIMIT_BITRATE
    if (pRCParam->uiChangePos >= 50 && pRCParam->uiChangePos <= 100)
        encRC->m_vbrPrelimitBRThd = division(encRC->m_GOPbitrate * pRCParam->uiChangePos, 100);
    else
        encRC->m_vbrPrelimitBRThd = 0;
#endif
    encRC->m_vbrChangePos = pRCParam->uiChangePos;
    encRC->m_keyPPeriod = (pRCParam->uiKeyPPeriod > pRCParam->uiGOP ? pRCParam->uiGOP : pRCParam->uiKeyPPeriod);
	encRC->m_ltrPeriod = pRCParam->uiLTRInterval;
    encRC->m_encRCEvbr.m_evbrState = EVBR_MOTION_STATE;
    encRC->m_encRCEvbr.m_stillIQP = 30;
    encRC->m_encRCEvbr.m_stillKeyPQP = 32;
    encRC->m_encRCEvbr.m_stillPQP = 38;
    #if BITRATE_OVERFLOW_CHECK
        RCBitrateCheckInit(&encRC->m_encRCEvbr.m_encRCBr, encRC->m_GOPbitrate);
        #if EVBR_MEASURE_BR_METHOD
        #if HANDLE_DIFF_LTR_KEYP_PERIOD
        if (encRC->m_ltrPeriod > 0) {
            int64_t br;
            encRC->m_encRCEvbr.m_measurePeriod = encRC->m_ltrPeriod - 1;
            br = division((int64_t)pRCParam->uiBitRate * encRC->m_ltrPeriod * encRC->m_frameRateIncr, encRC->m_frameRateBase);
            #if EVBR_XA
            encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)br*pRCParam->uiChangePos, 100);
            encRC->m_encRCEvbr.m_evbrStillupThd = division((int64_t)br*(pRCParam->uiChangePos-10), 100);
            encRC->m_encRCEvbr.m_evbrStillThd = division((int64_t)br*pRCParam->uiMinStillPercent, 100);
            #else
            encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)br*110, 100);
            #endif
        }
        else
        #endif
        {
            if (encRC->m_keyPPeriod > 0 && encRC->m_keyPPeriod < MAX_GOP_SIZE) {
                int64_t br;
                encRC->m_encRCEvbr.m_measurePeriod = encRC->m_keyPPeriod - 1;
                br = division((int64_t)pRCParam->uiBitRate * encRC->m_keyPPeriod * encRC->m_frameRateIncr, encRC->m_frameRateBase);
                #if EVBR_XA
				encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)br * pRCParam->uiChangePos, 100);
				encRC->m_encRCEvbr.m_evbrStillupThd = division((int64_t)br*(pRCParam->uiChangePos-10), 100);
				encRC->m_encRCEvbr.m_evbrStillThd = division((int64_t)br*pRCParam->uiMinStillPercent, 100);
                #else
                encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)br*110, 100);
                #endif
            }
            else {
                #if EVBR_XA
                if (0 == encRC->m_frameRateIncr)
                    encRC->m_encRCEvbr.m_measurePeriod = encRC->m_frameRateBase - 1;
                else
                    encRC->m_encRCEvbr.m_measurePeriod = encRC->m_frameRateBase / encRC->m_frameRateIncr-1;
                encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)pRCParam->uiBitRate *pRCParam->uiChangePos, 100);
                encRC->m_encRCEvbr.m_evbrStillupThd = division((int64_t)pRCParam->uiBitRate *(pRCParam->uiChangePos-10), 100);
                encRC->m_encRCEvbr.m_evbrStillThd = division((int64_t)pRCParam->uiBitRate *pRCParam->uiMinStillPercent, 100);
                #else
                if (0 == encRC->m_frameRateIncr)
                    encRC->m_encRCEvbr.m_measurePeriod = encRC->m_frameRateBase;
                else
                    encRC->m_encRCEvbr.m_measurePeriod = encRC->m_frameRateBase/encRC->m_frameRateIncr;
                encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)pRCParam->uiBitRate*110, 100);
                #endif
            }
        }
        #else   // EVBR_MEASURE_BR_METHOD
        encRC->m_encRCEvbr.m_evbrOverflowThd = division((int64_t)pRCParam->uiBitRate*90, 100);
        if (0 == encRC->m_frameRateIncr)
            encRC->m_encRCEvbr.m_measurePeriod = encRC->m_frameRateBase
        else
            encRC->m_encRCEvbr.m_measurePeriod = encRC->m_frameRateBase/encRC->m_frameRateIncr;
        #endif  // EVBR_MEASURE_BR_METHOD
    #endif  // BITRATE_OVERFLOW_CHECK
	#if EVBR_ADJUST_QP
	RCSetEvbrPreLimitstate(encRC->m_chn, RC_EVBR_LIMIT_LOW);
	RCSetEvbrS2MCnt(encRC->m_chn, 0);
	RCClearStillPDQP(encRC->m_chn);
	#endif
    encRC->m_encRCEvbr.m_stillFrameCnt = 0;
    encRC->m_encRCEvbr.m_stillFrameThd = pRCParam->uiStillFrameCnd;
    encRC->m_encRCEvbr.m_motionRatioThd = pRCParam->uiMotionRatioThd;
    if (pRCParam->uiIPsnrCnd < 100) {
        if (pRCParam->uiIPsnrCnd > 0 && pRCParam->uiIPsnrCnd <= 51)
            encRC->m_encRCEvbr.m_stillIQP = pRCParam->uiIPsnrCnd;
        encRC->m_encRCEvbr.m_IPsnrCnd = 0;
    }
    else
        encRC->m_encRCEvbr.m_IPsnrCnd = pRCParam->uiIPsnrCnd;
    if (pRCParam->uiPPsnrCnd < 100) {
        if (pRCParam->uiPPsnrCnd > 0 && pRCParam->uiPPsnrCnd <= 51)
            encRC->m_encRCEvbr.m_stillPQP = pRCParam->uiPPsnrCnd;
    	encRC->m_encRCEvbr.m_PPsnrCnd = 0;
    }
    else
        encRC->m_encRCEvbr.m_PPsnrCnd = pRCParam->uiPPsnrCnd;
    if (pRCParam->uiKeyPPsnrCnd < 100) {
        if (pRCParam->uiKeyPPsnrCnd > 0 && pRCParam->uiKeyPPsnrCnd <= 51)
            encRC->m_encRCEvbr.m_stillKeyPQP = pRCParam->uiKeyPPsnrCnd;
        encRC->m_encRCEvbr.m_KeyPPsnrCnd = 0;
    }
    else
    	encRC->m_encRCEvbr.m_KeyPPsnrCnd = pRCParam->uiKeyPPsnrCnd;
    if (g_LimitEVBRStillQP) {
        if (encRC->m_encRCEvbr.m_stillIQP < encRC->m_minIQp || encRC->m_encRCEvbr.m_stillIQP > encRC->m_maxIQp) {
            rc_wrn("{chn%d} still I qp(%d) is out of range, force be limited (%d ~ %d)\n", (int)encRC->m_chn,
                (int)encRC->m_encRCEvbr.m_stillIQP, (int)encRC->m_minIQp, (int)encRC->m_maxIQp);
            encRC->m_encRCEvbr.m_stillIQP = RC_CLIP3(encRC->m_minIQp, encRC->m_maxIQp, encRC->m_encRCEvbr.m_stillIQP);
        }
        if (encRC->m_encRCEvbr.m_stillKeyPQP < encRC->m_minPQp || encRC->m_encRCEvbr.m_stillKeyPQP > encRC->m_maxPQp) {
            rc_wrn("{chn%d} still KP qp(%d) is out of range, force be limited (%d ~ %d)\n", (int)encRC->m_chn,
                (int)encRC->m_encRCEvbr.m_stillKeyPQP, (int)encRC->m_minPQp, (int)encRC->m_maxPQp);
			encRC->m_encRCEvbr.m_stillKeyPQP = RC_CLIP3(encRC->m_minPQp, encRC->m_maxPQp, encRC->m_encRCEvbr.m_stillKeyPQP);
        }
        if (encRC->m_encRCEvbr.m_stillPQP < encRC->m_minPQp || encRC->m_encRCEvbr.m_stillPQP > encRC->m_maxPQp) {
            rc_wrn("{chn%d} still P qp(%d) is out of range, force be limited (%d ~ %d)\n", (int)encRC->m_chn,
                (int)encRC->m_encRCEvbr.m_stillPQP, (int)encRC->m_minPQp, (int)encRC->m_maxPQp);
			encRC->m_encRCEvbr.m_stillPQP = RC_CLIP3(encRC->m_minPQp, encRC->m_maxPQp, encRC->m_encRCEvbr.m_stillPQP);
        }
    }
	encRC->m_ltrPeriod = pRCParam->uiLTRInterval;

#if EVBR_XA
	encRC->m_encRCEvbr.m_minstillPercent = pRCParam->uiMinStillPercent;
	//encRC->m_encRCEvbr.m_evbrState = EVBR_STILL_STATE;
	encRC->m_encRCEvbr.m_startpic = 0;
	encRC->m_encRCEvbr.m_stillOverflow = 0;
	encRC->m_encRCEvbr.m_tmp_stillIQP = encRC->m_encRCEvbr.m_stillIQP;
	encRC->m_encRCEvbr.m_tmp_stillKeyPQP = encRC->m_encRCEvbr.m_stillKeyPQP;
	encRC->m_encRCEvbr.m_stillKeyPadd = 0;
	encRC->m_encRCEvbr.m_stillIadd = 0;
	encRC->m_encRCEvbr.m_tmp2_stillIQP = encRC->m_encRCEvbr.m_stillIQP;
	encRC->m_encRCEvbr.m_tmp2_stillKeyPQP = encRC->m_encRCEvbr.m_stillKeyPQP;
	encRC->m_encRCEvbr.m_last_stillIQP = encRC->m_encRCEvbr.m_stillIQP;
	encRC->m_encRCEvbr.m_last_stillKeyPQP = encRC->m_encRCEvbr.m_stillKeyPQP;
	encRC->m_encRCEvbr.m_last_stillPQP = encRC->m_encRCEvbr.m_stillPQP;
	encRC->m_encRCEvbr.uiMinStillIQp = pRCParam->uiMinStillIQp;
#endif

#if SUPPORT_VBR2
	encRC->m_ltrPeriod = pRCParam->uiLTRInterval;

	encRC->m_encRCVbr2.m_vbr2State = VBR2_CBR_STATE;
	encRC->m_encRCVbr2.m_maxBitrate = pRCParam->uiBitRate;
	encRC->m_encRCVbr2.m_initIQP = pRCParam->uiMinIQp;
	encRC->m_encRCVbr2.m_initPQP = pRCParam->uiMinPQp;
	encRC->m_encRCVbr2.m_initKPQP = pRCParam->uiMinPQp;
	encRC->m_encRCVbr2.m_initLTPQP = pRCParam->uiMinPQp;
	encRC->m_encRCVbr2.m_initP2QP = pRCParam->uiMinPQp;
	encRC->m_encRCVbr2.m_initP3QP = pRCParam->uiMinPQp;
	encRC->m_encRCVbr2.m_minIQP = pRCParam->uiMinIQp;
	encRC->m_encRCVbr2.m_maxIQP = pRCParam->uiMaxIQp;
	encRC->m_encRCVbr2.m_minPQP = pRCParam->uiMinPQp;
	encRC->m_encRCVbr2.m_maxPQP = pRCParam->uiMaxPQp;
	encRC->m_encRCVbr2.m_changePosThd = pRCParam->uiChangePos;
	encRC->m_encRCVbr2.m_deltaIQP = pRCParam->iIPWeight / 10;
	encRC->m_encRCVbr2.m_deltaKPQP = pRCParam->iKPWeight / 10;
	encRC->m_encRCVbr2.m_deltaLTPQP = pRCParam->iLTWeight / 10;
	encRC->m_encRCVbr2.m_deltaP2QP = pRCParam->iP2Weight / 10;
	encRC->m_encRCVbr2.m_deltaP3QP = pRCParam->iP3Weight / 10;

	//Handle weights
	encRC->m_encRCVbr2.m_initIQP = encRC->m_encRCVbr2.m_initPQP - encRC->m_encRCVbr2.m_deltaIQP;
	encRC->m_encRCVbr2.m_initKPQP = encRC->m_encRCVbr2.m_initPQP - encRC->m_encRCVbr2.m_deltaKPQP;
	encRC->m_encRCVbr2.m_initLTPQP = encRC->m_encRCVbr2.m_initPQP - encRC->m_encRCVbr2.m_deltaLTPQP;
	encRC->m_encRCVbr2.m_initP2QP = encRC->m_encRCVbr2.m_initPQP - encRC->m_encRCVbr2.m_deltaP2QP;
	encRC->m_encRCVbr2.m_initP3QP = encRC->m_encRCVbr2.m_initPQP - encRC->m_encRCVbr2.m_deltaP3QP;

	//Clip
	encRC->m_encRCVbr2.m_initIQP = RC_CLIP3(10, encRC->m_encRCVbr2.m_maxIQP, encRC->m_encRCVbr2.m_initIQP);
	encRC->m_encRCVbr2.m_initPQP = RC_CLIP3(10, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_initPQP);
	encRC->m_encRCVbr2.m_initKPQP = RC_CLIP3(10, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_initKPQP);
	encRC->m_encRCVbr2.m_initLTPQP = RC_CLIP3(10, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_initLTPQP);
	encRC->m_encRCVbr2.m_initP2QP = RC_CLIP3(10, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_initP2QP);
	encRC->m_encRCVbr2.m_initP3QP = RC_CLIP3(10, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_initP3QP);
	//Th
	encRC->m_encRCVbr2.m_overflowThd = 100;
	encRC->m_encRCVbr2.m_overflowThd2 = 110;
#if SUPPORT_VBR2_CVTE
#if ADUST_VBR2_OVERFLOW_CHECK
		encRC->m_encRCVbr2.m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
#else
	encRC->m_encRCVbr2.m_changeFrmCnt = 0;
#endif
	encRC->m_encRCVbr2.m_overflowThd = encRC->m_encRCVbr2.m_changePosThd + 10;
	encRC->m_encRCVbr2.m_overflowThd = RC_CLIP3(50, 90, encRC->m_encRCVbr2.m_overflowThd);
	encRC->m_encRCVbr2.m_overflowThd2 = 100;
#endif
	encRC->m_encRCVbr2.m_underflowThd = (2 * (int)encRC->m_encRCVbr2.m_changePosThd - 100) > 0 ? (2 * (int)encRC->m_encRCVbr2.m_changePosThd - 100) : 0;
	encRC->m_encRCVbr2.m_underflowThd = RC_CLIP3(50, 75, encRC->m_encRCVbr2.m_underflowThd);
	encRC->m_encRCVbr2.m_underflowThd = RC_MIN(encRC->m_encRCVbr2.m_changePosThd, encRC->m_encRCVbr2.m_underflowThd);
	encRC->m_encRCVbr2.m_underflowThd2 = encRC->m_encRCVbr2.m_underflowThd >> 1;
#if SUPPORT_VBR2_CVTE
	encRC->m_encRCVbr2.m_underflowThd = encRC->m_encRCVbr2.m_changePosThd - 10;
	encRC->m_encRCVbr2.m_underflowThd = RC_CLIP3(30, 70, encRC->m_encRCVbr2.m_underflowThd);
	if (encRC->m_rcMode == H26X_RC_VBR2)
	{
		encRC->m_encRCSeq.m_averageBits = division(encRC->m_encRCSeq.m_averageBits * 90, 100);
	}
#endif

#if BITRATE_OVERFLOW_CHECK
	RCBitrateCheckInit(&encRC->m_encRCVbr2.m_encRCBr, encRC->m_GOPbitrate);
	if (encRC->m_ltrPeriod > 0 && encRC->m_ltrPeriod < MAX_GOP_SIZE)
	{
		int64_t br;
		encRC->m_encRCVbr2.m_measurePeriod = encRC->m_ltrPeriod - 1;
		br = division((int64_t)encRC->m_encRCVbr2.m_maxBitrate * encRC->m_ltrPeriod * encRC->m_frameRateIncr, encRC->m_frameRateBase);
		encRC->m_encRCVbr2.m_rateOverflowThd = br;
		encRC->m_encRCVbr2.m_rateChangePosThd = division((int64_t)br * encRC->m_encRCVbr2.m_changePosThd, 100);
	}
	else if (encRC->m_keyPPeriod > 0 && encRC->m_keyPPeriod < MAX_GOP_SIZE)
	{
		int64_t br;
		encRC->m_encRCVbr2.m_measurePeriod = encRC->m_keyPPeriod - 1;
		br = division((int64_t)encRC->m_encRCVbr2.m_maxBitrate * encRC->m_keyPPeriod * encRC->m_frameRateIncr, encRC->m_frameRateBase);
		encRC->m_encRCVbr2.m_rateOverflowThd = br;
		encRC->m_encRCVbr2.m_rateChangePosThd = division((int64_t)br * encRC->m_encRCVbr2.m_changePosThd, 100);
	}
	else
	{
		#if ADUST_VBR2_OVERFLOW_CHECK
			 //uint32_t  frameRate = encRC->m_frameRateBase;
			 int64_t br;
			/*
			 if (0 == encRC->m_frameRateIncr)
				  frameRate = encRC->m_frameRateBase;
			 else
				 frameRate = encRC->m_frameRateBase / encRC->m_frameRateIncr;
			*/
			 encRC->m_encRCVbr2.m_measurePeriod = encRC->m_GOPSize - 1;
			 //encRC->m_encRCVbr2.m_measurePeriod = RC_CLIP3(frameRate-1, 30, encRC->m_encRCVbr2.m_measurePeriod);
			 encRC->m_encRCVbr2.m_measurePeriod = RC_CLIP3(g_VBR2EstPeriodMin, g_VBR2EstPeriodMax, encRC->m_encRCVbr2.m_measurePeriod);
			 br = division((int64_t)encRC->m_encRCVbr2.m_maxBitrate  * encRC->m_frameRateIncr, encRC->m_frameRateBase);
			 encRC->m_encRCVbr2.m_rateOverflowThd = br;
		#else
			if (0 == encRC->m_frameRateIncr)
				encRC->m_encRCVbr2.m_measurePeriod = encRC->m_frameRateBase - 1;
			else
				encRC->m_encRCVbr2.m_measurePeriod = encRC->m_frameRateBase / encRC->m_frameRateIncr - 1;
			encRC->m_encRCVbr2.m_rateOverflowThd = encRC->m_encRCVbr2.m_maxBitrate;
		#endif
		encRC->m_encRCVbr2.m_rateChangePosThd = division((int64_t)encRC->m_encRCVbr2.m_maxBitrate * encRC->m_encRCVbr2.m_changePosThd, 100);
	}
#endif  // BITRATE_OVERFLOW_CHECK

	encRC->m_encRCVbr2.m_currIQP = encRC->m_encRCVbr2.m_initIQP;
	encRC->m_encRCVbr2.m_currPQP = encRC->m_encRCVbr2.m_initPQP;
	encRC->m_encRCVbr2.m_currKPQP = encRC->m_encRCVbr2.m_initKPQP;
	encRC->m_encRCVbr2.m_currLTPQP = encRC->m_encRCVbr2.m_initLTPQP;
	encRC->m_encRCVbr2.m_currP2QP = encRC->m_encRCVbr2.m_initP2QP;
	encRC->m_encRCVbr2.m_currP3QP = encRC->m_encRCVbr2.m_initP3QP;

	encRC->m_encRCVbr2.m_lastPQP = encRC->m_encRCVbr2.m_initPQP;

	encRC->m_encRCVbr2.m_lastISize = 0;
	encRC->m_encRCVbr2.m_lastKPSize = 0;
	encRC->m_encRCVbr2.m_lastLTPSize = 0;
	encRC->m_encRCVbr2.m_lastP2Size = 0;
	encRC->m_encRCVbr2.m_lastP3Size = 0;

	encRC->m_encRCVbr2.m_isFirstPic = 1;
#endif

    encRC->m_svcLayer = pRCParam->uiSVCLayer;
#if SUPPORT_SVC_FIXED_WEIGHT_BA
	encRC->m_svcBAMode = encRC->m_svcLayer > 0 ? pRCParam->uiSvcBAMode : 0;
	if (encRC->m_svcBAMode)
	{
		int iWeightP, iWeightP2, iWeightP3, iWeightSum;
		encRC->m_deltaP2_QP = 0;
		encRC->m_deltaP3_QP = 0;
		encRC->m_svcP2Weight = 10;// RC_CLIP3(1, 100, pRCParam->iP2Weight);
		encRC->m_svcP3Weight = 10;// RC_CLIP3(1, 100, pRCParam->iP3Weight);

		//Recalculate bitsRatio
		iWeightP = encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL];
		iWeightP2 = encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL];
		iWeightP3 = encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL];
		iWeightSum = 10 + encRC->m_svcP2Weight;
		if (encRC->m_svcLayer == 1)
		{
			iWeightP += iWeightP2;
			iWeightP = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)iWeightP * 10, iWeightSum));
			iWeightP2 = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)iWeightP * encRC->m_svcP2Weight, 10));
		}
		else
		{
			iWeightSum += 10 + encRC->m_svcP3Weight;
			iWeightP += iWeightP + iWeightP2 + iWeightP3;
			iWeightP = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)iWeightP * 10, iWeightSum));
			iWeightP2 = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)iWeightP * encRC->m_svcP2Weight, 10));
			iWeightP3 = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)iWeightP * encRC->m_svcP3Weight, 10));
		}

		encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL] = iWeightP;
		encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL] = iWeightP2;
		encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL] = iWeightP3;
	}
#endif	// SUPPORT_SVC_FIXED_WEIGHT_BA

	if (H26X_RC_EVBR == encRC->m_rcMode){
		if (encRC->m_ltrPeriod > 0){
			encRC->m_keyPPeriod = encRC->m_ltrPeriod;
		}
	}
	if (g_AdjustQPWeight && encRC->m_svcLayer > 0){
		if (H26X_RC_EVBR == encRC->m_rcMode){
		    encRC->m_rc_deltaQPip=RC_CLIP3(-100, 100, ((int)encRC->m_encRCEvbr.m_stillPQP-(int)encRC->m_encRCEvbr.m_stillIQP)*10);
			encRC->m_deltaP2_QP = RC_CLIP3(-100, 100, ((int)encRC->m_encRCEvbr.m_stillPQP-(int)encRC->m_encRCEvbr.m_stillIQP)*10);  //hk
			encRC->m_deltaP3_QP = RC_CLIP3(-100, 100, 10);
		}
		if (encRC->m_rc_deltaQPip < encRC->m_deltaP2_QP)
			encRC->m_rc_deltaQPip = encRC->m_deltaP2_QP;
		if (encRC->m_deltaLT_QP < encRC->m_deltaP2_QP)
			encRC->m_deltaLT_QP = encRC->m_deltaP2_QP;
		if (encRC->m_rc_deltaQPkp < encRC->m_deltaP2_QP)
			encRC->m_rc_deltaQPkp = encRC->m_deltaP2_QP;
	}

#if EVBR_MODE_SWITCH_NEW_METHOD
	encRC->m_still2motion_KP = 0;
	encRC->m_still2motion_I = 0;
#endif

#if RD_LAMBDA_UPDATE
	encRC->ave_moRatio = 0;
#endif

	encRC->m_glb_max_frm_size = pRCParam->uiMaxFrameSize;

#if SMOOTH_FRAME_QP
    TEncRateCtrl_set_pic_gop_weight();
#endif

#if WEIGHT_CAL_RESET==3
	encRC->m_reset_alpha_bool = 0;
#elif WEIGHT_CAL_RESET==4
	RCSetResetAlphaBool(encRC->m_chn, 0);
#endif
#if RC_GOP_QP_LIMITATION
	encRC->m_BrTolerance = RC_CLIP3(0, 6, pRCParam->uiBRTolerance/*g_BrTolerance*/);
	encRC->m_useGopQpLimitation = encRC->m_BrTolerance > 0 ? 1 : 0;
	if (encRC->m_useGopQpLimitation)
	{
		encRC->m_GopQpLimit = 0;
		encRC->m_QpMinus24 = 27;
		encRC->m_CurrPNum = 0;
		encRC->m_GopMaxQp = 51;
		encRC->m_QpBeforeClip = 26;
		encRC->m_GopMaxQpFromTbl = 51;
		encRC->m_GopMaxQpAdd = 0;
		encRC->m_GopBitsUnderTBR = 0;
		encRC->m_GopBitsEst = 0;
		encRC->m_LastIBits = 0;
		encRC->m_LastKPBits = 0;
		encRC->m_LastLTBits = 0;
		encRC->m_PBitsSum = 0;
		encRC->m_AvgPBits = 0;
		encRC->m_GopOverflowLevel = 0;
		encRC->m_GopMaxQpFromBpp = 0;
	}
#endif
#if EVBR_KEEP_PIC_QUALITY
	if (g_LargeGOPThd > 0 && (pRCParam->uiGOP*pRCParam->uiFrameRateIncr) > (g_LargeGOPThd*pRCParam->uiFrameRateBase)) {
		RCSetKeepIQualityEn(encRC->m_chn, 1);
		//DBG_DUMP("keep quality enable\n");
	}
	else {
		RCSetKeepIQualityEn(encRC->m_chn, 0);
		//DBG_DUMP("keep quality disable\n");
	}
#endif
}

static void TEncRCSeq_create(TEncRCSeq *encRCSeq, int targetBitrate, int fbase, int fincr, int GOPSize, int mbcount, int numberOfLevel, int adaptiveBit, int chn)
{
    int64_t tmp_1;
	int64_t tmp;
    unsigned int tmp_2;
	int64_t tmp_3;
	tmp_3 = (int64_t)targetBitrate * (int64_t)fincr * (int64_t)encRCSeq->m_seqframeleft;

	/* avoid integer overflowing */
	encRCSeq->m_bitsleft = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(tmp_3, fbase));

	encRCSeq->m_targetRate = targetBitrate;
	encRCSeq->m_fbase = fbase;
	encRCSeq->m_fincr = fincr;
	encRCSeq->m_GOPSize = GOPSize;
	encRCSeq->m_mbCount = mbcount;
	encRCSeq->m_numberOfLevel = numberOfLevel;
	encRCSeq->m_numberOfMB = encRCSeq->m_mbCount;
	encRCSeq->m_targetBits = (int64_t)encRCSeq->m_targetRate;

    tmp_1 = ((int64_t)encRCSeq->m_targetRate << 7) * (int64_t)encRCSeq->m_fincr;
    tmp_2 = encRCSeq->m_fbase * encRCSeq->m_numberOfMB;
	tmp = division(tmp_1, (int64_t)tmp_2) >> 8;

	/* avoid integer overflowing */
	encRCSeq->m_seqTargetBpp = RC_CLIP3(0, (int64_t)MAX_INTEGER, tmp);

	if (encRCSeq->m_seqTargetBpp < 4) {    // (int)(0.03 * 128)
    #if WEIGHT_ADJUST
        encRCSeq->m_updateLevel = 0;
    #endif
		encRCSeq->m_alphaUpdate = 20;        // (int)(0.01 * 2048)
		encRCSeq->m_betaUpdate = 10;         // (int)(0.005 * 2048)
	}
    else if (encRCSeq->m_seqTargetBpp < 11) {      // (int)(0.08 * 128)
    #if WEIGHT_ADJUST
        encRCSeq->m_updateLevel = 1;
    #endif
		encRCSeq->m_alphaUpdate = 102;       // (int)(0.05 * 2048)
		encRCSeq->m_betaUpdate = 51;         // (int)(0.025 * 2048)
	}
    else if (encRCSeq->m_seqTargetBpp < 26) {      // (int)(0.2 * 128)
    #if WEIGHT_ADJUST
        encRCSeq->m_updateLevel = 2;
    #endif
		encRCSeq->m_alphaUpdate = 205;       // (int)(0.1 * 2048)
		encRCSeq->m_betaUpdate = 102;        // (int)(0.05 * 2048)
	}
    else if (encRCSeq->m_seqTargetBpp < 64) {      // (int)(0.5 * 128)
    #if WEIGHT_ADJUST
        encRCSeq->m_updateLevel = 3;
    #endif
		encRCSeq->m_alphaUpdate = 410;       // (int)(0.2 * 2048)
		encRCSeq->m_betaUpdate = 205;        // (int)(0.1 * 2048)
	}
    else {
    #if WEIGHT_ADJUST
        encRCSeq->m_updateLevel = 4;
    #endif
		encRCSeq->m_alphaUpdate = 819;       // (int)(0.4 * 2048)
		encRCSeq->m_betaUpdate = 410;        // (int)(0.2 * 2048)
	}

#if WEIGHT_ADJUST
    encRCSeq->m_updatealpha[0] = 20;
    encRCSeq->m_updatebeta[0] = 10;
    encRCSeq->m_updatealpha[1] = 102;
    encRCSeq->m_updatebeta[1] = 51;
    encRCSeq->m_updatealpha[2] = 205;
    encRCSeq->m_updatebeta[2] = 102;
    encRCSeq->m_updatealpha[3] = 410;
    encRCSeq->m_updatebeta[3] = 205;
    encRCSeq->m_updatealpha[4] = 819;
    encRCSeq->m_updatebeta[4] = 410;
	encRCSeq->keyframe_num = 0;
#endif

	tmp = encRCSeq->m_targetBits * (int64_t)encRCSeq->m_fincr;
	encRCSeq->m_averageBits = division(tmp, encRCSeq->m_fbase);

	encRCSeq->m_bitsDiff = 0;
	#if EVALUATE_PRED_DIFF
	RCClearPredDiff(chn);
	#endif
	#if ALPHA_CONVERG_DIRECT 
	encRCSeq->m_AlphaConvergeCnt = 0;
	#endif
	encRCSeq->m_adaptiveBit = adaptiveBit;
	encRCSeq->m_lastLambda = 0;
}

static void initBitsRatio(TEncRCSeq* encRCSeq, int weight_I, int weight_SP, int weight_LT, int weight_P2, int weight_P3, int weight_P, int keyframe)
{
	if (keyframe)
		encRCSeq->m_bitsRatio[RC_I_FRAME_LEVEL] = weight_I;

	encRCSeq->m_bitsRatio[RC_P_FRAME_LEVEL] = weight_P;
    encRCSeq->m_bitsRatio[RC_LT_P_FRAME_LEVEL] = weight_LT;
    encRCSeq->m_bitsRatio[RC_KEY_P_FRAME_LEVEL] = weight_SP;
    encRCSeq->m_bitsRatio[RC_P2_FRAME_LEVEL] = weight_P2;
    encRCSeq->m_bitsRatio[RC_P3_FRAME_LEVEL] = weight_P3;
}

static void initGOPID2Level(TEncRCSeq *encRCSeq, int value)
{
	int i;
    for (i = 0; i < RC_MAX_FRAME_LEVEL; i++) {
        encRCSeq->m_GOPID2Level[i] = value;
    }
}

static void initPicPara(TEncRCSeq *encRCSeq)
{
	int i;

    for (i = 0; i < RC_MAX_FRAME_LEVEL; i++) {
		encRCSeq->m_picPara[i].m_alpha = 6554;        // 3.2003 * 2048;
		encRCSeq->m_picPara[i].m_beta = -2800;        // -1.367 * 2048;
	}
}

static int64_t xEstGOPTargetBits(TEncRateCtrl *encRC, TEncRCSeq *encRCSeq, int GOPSize)
{
	int64_t tmp;
	int64_t targetBits;
	int realInfluencePicture;

    realInfluencePicture = encRC->m_smoothWindowSize;// already normalization * GOPSize / (encRC->m_frameRateBase/encRC->m_frameRateIncr);

	// Tuba 20200506: limit difference bit by target bitrate
#if LIMIT_GOP_DIFFERNT_BIT
	if (g_MinGOPDiffFactor > 0 && g_MaxGOPDiffFactor > 0) {
		encRCSeq->m_bitsDiff = RC_CLIP3(-1*((encRC->m_GOPbitrate*g_MinGOPDiffFactor)>>4), ((encRC->m_GOPbitrate*g_MaxGOPDiffFactor)>>4), encRCSeq->m_bitsDiff);
		#if EVALUATE_PRED_DIFF
		RCSetPredDiff(encRC->m_chn, RC_CLIP3(-1*((encRC->m_GOPbitrate*g_MinGOPDiffFactor)>>4), ((encRC->m_GOPbitrate*g_MaxGOPDiffFactor)>>4), (RCGetPredDiff(encRC->m_chn)>>1)));
		#endif
	}
#endif
#if RC_GOP_QP_LIMITATION
	if (encRC->m_useGopQpLimitation && encRC->m_GopQpLimit && (encRC->m_QpBeforeClip > encRC->m_GopMaxQp + 1))
		tmp = (encRCSeq->m_bitsDiff >> 1) + realInfluencePicture * encRCSeq->m_averageBits;
	else
#endif
		tmp = encRCSeq->m_bitsDiff + realInfluencePicture * encRCSeq->m_averageBits;
    tmp = division(tmp, realInfluencePicture);
    tmp = RC_CLIP3(0, (int64_t)MAX_INTEGER, tmp);

	targetBits = tmp * GOPSize;

    if (targetBits < encRC->m_GOPMinBitrate) {
        targetBits = encRC->m_GOPMinBitrate;
    }

	return targetBits;
}

static int64_t xEstHPGOPTargetBits(TEncRateCtrl *encRC, TEncRCSeq *encRCSeq, int iNumPic)
{
	int64_t tmp64, iTargetBits;
	int iSmoothWindowSize = RC_MIN((int)encRC->m_GOPSize, encRC->m_encRCGOP.m_picLeft);//40 zjz
	tmp64 = (int64_t)encRCSeq->m_averageBits * (encRC->m_encRCGOP.m_picLeft - iSmoothWindowSize);
	tmp64 = encRC->m_encRCGOP.m_bitsLeft - tmp64;
	tmp64 = division(tmp64, iSmoothWindowSize);
	iTargetBits = (tmp64 * iNumPic);
	if (iTargetBits < 200)
	{
		iTargetBits = 200;
	}
	return iTargetBits;
}

static int xEstPicTargetBits(TEncRateCtrl *encRC, TEncRCSeq *encRCSeq, TEncRCGOP *encRCGOP, int frameLevel)
{
	int64_t targetBits = 0;
	int64_t GOPbitsLeft = encRCGOP->m_bitsLeft;
	int i;
	int currPicPosition = encRCGOP->m_numPic - encRCGOP->m_picLeft;
	int currPicRatio;
	int totalPicRatio = 0;
#if SMOOTH_FRAME_QP
	int RCWeightPicRargetBitInBuffer;
	int RCWeightPicTargetBitInGOP;
#endif

	int64_t tmp_1, tmp_2;
    int numpic = encRCGOP->m_picLeft;

    currPicRatio = encRCSeq->m_bitsRatio[frameLevel];

    if (currPicPosition == 0)
    {
        totalPicRatio += encRCSeq->m_bitsRatio[RC_I_FRAME_LEVEL];
        numpic--;
    }
    for (i = 0; i < encRC->m_GOP_SPnumleft; i++)
    {
        totalPicRatio += encRCSeq->m_bitsRatio[RC_KEY_P_FRAME_LEVEL];
        numpic--;
    }
	for (i = 0; i < encRC->m_GOP_LTnumleft; i++)
	{
		totalPicRatio += encRCSeq->m_bitsRatio[RC_LT_P_FRAME_LEVEL];
		numpic--;
	}
	for (i = 0; i < encRC->m_GOP_P2numleft; i++){
		totalPicRatio += encRCSeq->m_bitsRatio[RC_P2_FRAME_LEVEL];
		numpic--;
	}
	for (i = 0; i < encRC->m_GOP_P3numleft; i++){
		totalPicRatio += encRCSeq->m_bitsRatio[RC_P3_FRAME_LEVEL];
		numpic--;
	}
    for (i = 0; i < numpic; i++)
    {
        totalPicRatio += encRCSeq->m_bitsRatio[RC_P_FRAME_LEVEL];
    }

	if (0 == totalPicRatio) {
		targetBits = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(GOPbitsLeft, encRCGOP->m_numPic));
		//totalPicRatio = encRCSeq->m_GOPSize;
	}
	else {
		int64_t tmp_bits;
		tmp_bits = GOPbitsLeft * (int64_t)currPicRatio;
		targetBits = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(tmp_bits, totalPicRatio));
	}

	if (targetBits < 100)
	{
		targetBits = 100;   // at least allocate 100 bits for one picture
	}
    #if SMOOTH_FRAME_QP
	if (encRC->m_rcMode == H26X_RC_CBR){
		RCWeightPicRargetBitInBuffer = g_RCWeightPicRargetBitInBuffer_cbr;
		RCWeightPicTargetBitInGOP = g_RCWeightPicTargetBitInGOP_cbr;
	}
	else if (encRC->m_rcMode == H26X_RC_VBR || encRC->m_rcMode == H26X_RC_VBR2) {
		RCWeightPicRargetBitInBuffer = g_RCWeightPicRargetBitInBuffer_vbr;
		RCWeightPicTargetBitInGOP = g_RCWeightPicTargetBitInGOP_vbr;
	}
    else {
		RCWeightPicRargetBitInBuffer = g_RCWeightPicRargetBitInBuffer_evbr;
		RCWeightPicTargetBitInGOP = g_RCWeightPicTargetBitInGOP_evbr;
    }

	tmp_1 = (targetBits * (int64_t)RCWeightPicRargetBitInBuffer + (SHIFT_VALUE >> 1)) >> SHIFT_11;
    tmp_2 = ((int64_t)encRCGOP->m_picTargetBitInGOP[frameLevel] * RCWeightPicTargetBitInGOP + (SHIFT_VALUE >> 1)) >> SHIFT_11;//1843
    #else
	tmp_1 = (targetBits * (int64_t)g_RCWeightPicRargetBitInBuffer + (SHIFT_VALUE >> 1)) >> SHIFT_11;
    tmp_2 = ((int64_t)encRCGOP->m_picTargetBitInGOP[frameLevel] * g_RCWeightPicTargetBitInGOP + (SHIFT_VALUE >> 1)) >> SHIFT_11;//1843
    #endif
	targetBits = tmp_1 + tmp_2;

	/*if (targetBits < (1024 + encRC->m_iPrevPicBitsBias[encRC->m_slicetype]))//HP
		targetBits = 1024 + encRC->m_iPrevPicBitsBias[encRC->m_slicetype];*/

#if !USE_HP_DELTAQP
    if (encRC->m_svcLayer > 0)
    {
        if (frameLevel == RC_I_FRAME_LEVEL && encRC->m_inumpic > 0)
        {
            int alpha, beta, bpp;
            int estLambda, minLambda, maxLambda;
            int lastILambda = encRC->m_lastLevelLambda[RC_I_FRAME_LEVEL];
            int lastP2Lambda = encRC->m_lastLevelLambda[RC_P2_FRAME_LEVEL];
            tmp_1 = (int64_t)lastP2Lambda << 13;
            estLambda = (int)division(tmp_1, (int)encRC->m_encRCHPGOP.m_lambdaRatioLevel[RC_I_FRAME_LEVEL]);

            //Clip3
            tmp_1 = (int64_t)lastILambda * 203 + 1024; // 2 ^ (-10/3)
            minLambda = (int)(tmp_1 >> SHIFT_11);
            tmp_1 = (int64_t)lastILambda * 65536 + 1024; // 2 ^ (15/3)
            maxLambda = (int)(tmp_1 >> SHIFT_11);
            estLambda = RC_CLIP3(minLambda, maxLambda, estLambda);
            tmp_1 = (int64_t)lastP2Lambda << 13;
            maxLambda = (int)division(tmp_1, 8192);
            if (estLambda > maxLambda)
            {
                estLambda = maxLambda;
            }
            estLambda = RC_CLIP3(3072, 512000, estLambda);

            alpha = encRC->m_encRCSeq.m_picPara[RC_I_FRAME_LEVEL].m_alpha;
            beta = encRC->m_encRCSeq.m_picPara[RC_I_FRAME_LEVEL].m_beta;
            tmp_1 = (int64_t)estLambda << SHIFT_11;
            tmp_1 = division(tmp_1, alpha);
            tmp_1 = (tmp_1 * (int64_t)TIMES_10) >> SHIFT_11;
            tmp_2 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta);
            bpp = calculatePow(SHIFT_VALUE, (int)tmp_1, (int)tmp_2, 1);
            targetBits = (int64_t)bpp * (int64_t)encRC->m_encRCSeq.m_numberOfMB;
            targetBits = (targetBits + 4) >> (SHIFT_11 - 8);
        }
    }
#endif

	/* avoid integer overflowing */
	targetBits = RC_CLIP3(100, (int64_t)MAX_INTEGER, targetBits);
#if MAX_SIZE_BUFFER
	if (encRC->m_max_frm_bit > 0)
		targetBits = RC_CLIP3(100, (int64_t)encRC->m_max_frm_bit, targetBits);
#endif
    encRC->m_prevTargetBits = targetBits;

	rc_inf("end PicTargetBits - targetBits : %lld\n", (long long)(targetBits));

	return (int)targetBits;
}

static int xEstPicHeaderBits(TEncRateCtrl *encRC, int frameLevel)
{
	/* FIXME : this is not correct, CW */
	int estHeaderBits = (0 == frameLevel) ? 0 : 64;
	return estHeaderBits;
}

static void TEncRCGOP_create(TEncRateCtrl *encRC, TEncRCGOP *encRCGOP, int numPic)
{
	int i;
	int totalPicRatio = 0;
	int currPicRatio = 0;
	int64_t targetBits;
	//int P_frm_num;
	int minInterval, numInterval, numRemFrms;

	RCGOPInit(encRCGOP, encRC->m_GOPSize);

	if (encRC->m_ltrPeriod > 0)
	{
		encRC->m_LT_frm_num = 0;
		for (i = encRC->m_ltrPeriod; i < numPic; i += encRC->m_ltrPeriod)
		{
			encRC->m_LT_frm_num++;
		}
		encRC->m_GOP_LTnumleft = encRC->m_LT_frm_num;
	}
	else
	{
		encRC->m_LT_frm_num = 0;
		encRC->m_GOP_LTnumleft = 0;
	}
	encRC->m_LT_sign = 0;

    encRC->m_SP_frm_num = 0;
    if (encRC->m_keyPPeriod > 0){
        for (i = encRC->m_keyPPeriod; i < numPic; i += encRC->m_keyPPeriod)
        {
            if ((encRC->m_ltrPeriod>0 && i % encRC->m_ltrPeriod != 0) || encRC->m_ltrPeriod == 0)
                encRC->m_SP_frm_num++;
        }
        encRC->m_GOP_SPnumleft = encRC->m_SP_frm_num;
        encRC->m_SP_sign = 0;
    }
    else {
        encRC->m_SP_frm_num = 0;
        encRC->m_GOP_SPnumleft = 0;
		encRC->m_SP_sign = 0;
    }

	minInterval = numPic;
	if (encRC->m_ltrPeriod > 0)
	{
		minInterval = RC_MIN(minInterval, (int)encRC->m_ltrPeriod);
	}
	numInterval = numPic / minInterval;
	numRemFrms = numPic % minInterval;

	encRC->m_P2_frm_num = 0;
	encRC->m_P3_frm_num = 0;
	if (encRC->m_svcLayer == 2)
	{
		for (i = 4; i < minInterval; i += 4)
		{
			encRC->m_P2_frm_num++;
		}
		for (i = 2; i < minInterval; i += 4)
		{
			encRC->m_P3_frm_num++;
		}
		encRC->m_P2_frm_num *= numInterval;
		encRC->m_P3_frm_num *= numInterval;
		if (numRemFrms > 0)
		{
			for (i = 4; i < numRemFrms; i += 4)
			{
				encRC->m_P2_frm_num++;
			}
			for (i = 2; i < numRemFrms; i += 4)
			{
				encRC->m_P3_frm_num++;
			}
		}
	}
	else if (encRC->m_svcLayer == 1)
	{
		for (i = 2; i < minInterval; i += 2)
		{
			encRC->m_P2_frm_num++;
		}
		encRC->m_P2_frm_num *= numInterval;
		if (numRemFrms > 0)
		{
			for (i = 2; i < numRemFrms; i += 2)
			{
				encRC->m_P2_frm_num++;
			}
		}
	}
	encRC->m_GOP_P2numleft = encRC->m_P2_frm_num;
	encRC->m_GOP_P3numleft = encRC->m_P3_frm_num;

	targetBits = xEstGOPTargetBits(encRC, &encRC->m_encRCSeq, numPic);
	totalPicRatio += encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL];
	for (i = 0; i < encRC->m_LT_frm_num; i++)
		totalPicRatio += encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL];
	for (i = 0; i < encRC->m_SP_frm_num;i++)
		totalPicRatio += encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL];
	for (i = 0; i < encRC->m_P2_frm_num; i++)
		totalPicRatio += encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL];
	for (i = 0; i < encRC->m_P3_frm_num; i++)
		totalPicRatio += encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL];
    for (i = 0; i<(numPic - 1 - encRC->m_LT_frm_num - encRC->m_SP_frm_num - encRC->m_P2_frm_num - encRC->m_P3_frm_num); i++)
		totalPicRatio += encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL];

    currPicRatio = encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL];
    encRCGOP->m_picTargetBitInGOP[RC_I_FRAME_LEVEL] = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(targetBits * (int64_t)currPicRatio, totalPicRatio));
    currPicRatio = encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL];
    encRCGOP->m_picTargetBitInGOP[RC_P_FRAME_LEVEL] = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(targetBits * (int64_t)currPicRatio, totalPicRatio));
	if (encRC->m_GOP_LTnumleft > 0)
	{
		currPicRatio = encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL];
		encRCGOP->m_picTargetBitInGOP[RC_LT_P_FRAME_LEVEL] = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(targetBits * (int64_t)currPicRatio, totalPicRatio));
	}
	else
		encRCGOP->m_picTargetBitInGOP[RC_LT_P_FRAME_LEVEL] = 0;
	if (encRC->m_GOP_SPnumleft>0){
		currPicRatio = encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL];
		encRCGOP->m_picTargetBitInGOP[RC_KEY_P_FRAME_LEVEL] = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(targetBits * (int64_t)currPicRatio, totalPicRatio));
	}
	else
		encRCGOP->m_picTargetBitInGOP[RC_KEY_P_FRAME_LEVEL] = 0;

	if (encRC->m_P2_frm_num > 0){
		currPicRatio = encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL];
		encRCGOP->m_picTargetBitInGOP[RC_P2_FRAME_LEVEL] = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(targetBits * (int64_t)currPicRatio, totalPicRatio));
	}
	else
		encRCGOP->m_picTargetBitInGOP[RC_P2_FRAME_LEVEL] = 0;
	if (encRC->m_P3_frm_num > 0){
		currPicRatio = encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL];
		encRCGOP->m_picTargetBitInGOP[RC_P3_FRAME_LEVEL] = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(targetBits * (int64_t)currPicRatio, totalPicRatio));
	}
	else
		encRCGOP->m_picTargetBitInGOP[RC_P3_FRAME_LEVEL] = 0;

	encRCGOP->m_numPic = numPic;
	encRCGOP->m_targetBits = targetBits;
	encRCGOP->m_picLeft = encRCGOP->m_numPic;
	encRCGOP->m_bitsLeft = encRCGOP->m_targetBits;
}

static void TEncRCHPGOP_create(TEncRateCtrl *encRC, TEncRCHPGOP *encRCHPGOP, int iIntraPeriod, int iHierLength)
{
    int64_t iHierTargetBits;
	int iNumPic = iHierLength;
	if (encRC->m_encRCGOP.m_picLeft < iHierLength)
	{
		iNumPic = encRC->m_encRCGOP.m_picLeft;
	}
	iHierTargetBits = xEstHPGOPTargetBits(encRC, &encRC->m_encRCSeq, iNumPic);
	encRCHPGOP->m_numPic = iNumPic;
	encRCHPGOP->m_targetBits = iHierTargetBits;
	encRCHPGOP->m_picLeft = iNumPic;
	encRCHPGOP->m_bitsLeft = iHierTargetBits;
}

static void TEncRCPic_create(TEncRateCtrl *encRC, TEncRCPic *encRCPic, int frameLevel)
{
	int targetBits = xEstPicTargetBits(encRC, &encRC->m_encRCSeq, &encRC->m_encRCGOP, frameLevel);
	int estHeaderBits = xEstPicHeaderBits(encRC, frameLevel);

	if (targetBits < estHeaderBits + 100) {
		targetBits = estHeaderBits + 100;   // at least allocate 100 bits for picture data
	}

	encRCPic->m_frameLevel = frameLevel;
	encRCPic->m_numberOfMB = encRC->m_encRCSeq.m_numberOfMB;
	encRCPic->m_estPicLambda = 100;
    encRCPic->m_targetBits = targetBits;
	encRCPic->m_estHeaderBits = estHeaderBits;
	encRCPic->m_bitsLeft = encRCPic->m_targetBits;

	encRCPic->m_bitsLeft -= encRCPic->m_estHeaderBits;
	encRCPic->m_pixelsLeft = encRCPic->m_numberOfMB;

	encRCPic->m_picActualHeaderBits = 0;
	encRCPic->m_picActualBits = 0;
	encRCPic->m_picQP = 26;
	encRCPic->m_picLambda = 0;
}

static void initRCGOP(TEncRateCtrl *encRC, int numberOfPictures)
{
	TEncRCGOP_create(encRC, &encRC->m_encRCGOP, numberOfPictures);
}

static void initRCPic(TEncRateCtrl *encRC, TEncRCPic *encRCPic, int frameLevel)
{
	TEncRCPic_create(encRC, encRCPic, frameLevel);
}

#if MAX_SIZE_BUFFER
static void ResetISize(TEncRateCtrl* encRC, TEncRCGOP* encRCGOP)
{
	int LT_bit, kp_bit, P2_bit, P3_bit, P_bit;
	int I_weight, LT_weight, kp_weight, P2_weight, P3_weight, P_weight;
	int64_t bitleft = encRCGOP->m_bitsLeft - encRC->m_max_frm_bit;
	int P_frm_num = encRCGOP->m_picLeft - 1 - encRC->m_GOP_LTnumleft - encRC->m_GOP_SPnumleft - encRC->m_GOP_P2numleft - encRC->m_GOP_P3numleft;
	int  allweight = encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL] * encRC->m_GOP_LTnumleft + encRC->m_GOP_SPnumleft*encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL]
		+ encRC->m_GOP_P2numleft*encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL] + encRC->m_GOP_P3numleft*encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL] + P_frm_num*encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL];

	LT_bit = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(bitleft  * (int64_t)encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL], allweight));
	kp_bit = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(bitleft  * (int64_t)encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL], allweight));
	P2_bit = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(bitleft  * (int64_t)encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL], allweight));
	P3_bit = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(bitleft  * (int64_t)encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL], allweight));
	P_bit = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(bitleft  * (int64_t)encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL], allweight));

	I_weight = RC_CLIP3(10, (int64_t)MAX_INTEGER, division(((int64_t)encRC->m_max_frm_bit) * 10, P_bit));
	LT_weight = RC_CLIP3(10, (int64_t)MAX_INTEGER, division(((int64_t)LT_bit) * 10, P_bit));
	kp_weight = RC_CLIP3(10, (int64_t)MAX_INTEGER, division(((int64_t)kp_bit) * 10, P_bit));
	P2_weight = RC_CLIP3(10, (int64_t)MAX_INTEGER, division(((int64_t)P2_bit) * 10, P_bit));
	P3_weight = RC_CLIP3(10, (int64_t)MAX_INTEGER, division(((int64_t)P3_bit) * 10, P_bit));
	P_weight = 10;

#if SUPPORT_SVC_FIXED_WEIGHT_BA
	if (encRC->m_svcBAMode)
	{
		//Recalculate bitsRatio
		int iWeightSum = 10 + encRC->m_svcP2Weight;
		if (encRC->m_svcLayer == 1)
		{
			P_weight += P2_weight;
			P_weight = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)P_weight * 10, iWeightSum));
			P2_weight = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)P_weight * encRC->m_svcP2Weight, 10));
		}
		else
		{
			iWeightSum += 10 + encRC->m_svcP3Weight;
			P_weight += P_weight + P2_weight + P3_weight;
			P_weight = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)P_weight * 10, iWeightSum));
			P2_weight = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)P_weight * encRC->m_svcP2Weight, 10));
			P3_weight = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)P_weight * encRC->m_svcP3Weight, 10));
		}
	}
#endif
	initBitsRatio(&encRC->m_encRCSeq, I_weight, kp_weight, LT_weight, P2_weight, P3_weight, P_weight, 1);
	rc_dbg("resetweight:I:%d  kp: %d  LT:%d  P2:%d  P3:%d\n", (int)(I_weight), (int)(kp_weight), (int)(LT_weight), (int)(P2_weight), (int)(P3_weight));
}
#endif

static void initRCHPGOP(TEncRateCtrl *encRC, int iIntraPeriod, int iHierLength)
{
	TEncRCHPGOP_create(encRC, &encRC->m_encRCHPGOP, iIntraPeriod, iHierLength);
}

static void TEncRateCtrl_init(
	TEncRateCtrl *encRC,
	int targetBitrate,
	int fbase,
	int fincr,
	int GOPSize,
	int mbcount,
	int SP_GOPSize,
	int ltr_interval,
	int svc_layer
	)
{
	int64_t tmp_1, bpp;
	unsigned int tmp_2;
	int weight_P=1;
	int weight_I=1;
    int weight_SP=1;
	int LT_GOPSize = ltr_interval;//encRC->m_ltrPeriod;
	int weight_LT = 1;
	int weight_P2 = 1;
	int weight_P3 = 1;
	int numberOfLevel = 1;
	int adaptiveBit = 0;

	numberOfLevel++;    // intra picture
	numberOfLevel++;    // non-reference picture

	tmp_1 = ((int64_t)targetBitrate << 7) * (int64_t)fincr;
	tmp_2 = fbase * mbcount;
	bpp = division(tmp_1, (int64_t)tmp_2) >> 8;

    numberOfLevel = 2; //I, P
    if (bpp > 25)
    {
        weight_P = 3;
        weight_I = 6;
    }
    else if (bpp > 12)
    {
        weight_P = 3;
        weight_I = 10;
    }
    else if (bpp > 6)
    {
        weight_P = 3;
        weight_I = 12;
    }
    else
    {
        weight_P = 3;
        weight_I = 14;
    }
    if (LT_GOPSize > 0 && LT_GOPSize < GOPSize)
    {
        numberOfLevel++;
        if (bpp > 25)
        {
            weight_P = 10;
            weight_LT = 15;
            weight_I = 20;
        }
        else if (bpp > 12)
        {
            weight_P = 10;
            weight_LT = 21;
            weight_I = 33;
        }
        else if (bpp > 6)
        {
            weight_P = 10;
            weight_LT = 25;
            weight_I = 40;
        }
        else
        {
            weight_P = 10;
            weight_LT = 28;
            weight_I = 46;
        }
    #if EVBR_MODE_SWITCH_NEW_METHOD
        encRC->m_record_init_weight[RC_I_FRAME_LEVEL] = weight_I;
        encRC->m_record_init_weight[RC_LT_P_FRAME_LEVEL] = weight_LT;
        encRC->m_record_init_weight[RC_P_FRAME_LEVEL] = weight_P;
    #endif
    }

	if (SP_GOPSize > 0 && SP_GOPSize < GOPSize){
        numberOfLevel++;
		if (bpp > 25)
		{
			weight_P = 10;
			weight_SP = 15;
			weight_I = 20;
		}
		else if (bpp > 12)
		{
			weight_P = 10;
			weight_SP = 21;
			weight_I = 33;
		}
		else if (bpp > 6)
		{
			weight_P = 10;
			weight_SP = 25;
			weight_I = 40;
		}
		else
		{
			weight_P = 10;
			weight_SP = 28;
			weight_I = 46;
		}
#if EVBR_MODE_SWITCH_NEW_METHOD
		encRC->m_record_init_weight[RC_I_FRAME_LEVEL] = weight_I;
		encRC->m_record_init_weight[RC_KEY_P_FRAME_LEVEL] = weight_SP;
		encRC->m_record_init_weight[RC_P_FRAME_LEVEL] = weight_P;
#endif
	}

	//if (encRC->m_svcLayer > 0){
	if (svc_layer) {
		numberOfLevel +=2;
		if (bpp > 25)
		{
			weight_P = 10;
			weight_P3 = 12;
			weight_P2 = 15;
			weight_I = 20;
		}
		else if (bpp > 12)
		{
			weight_P = 10;
			weight_P3 = 12;
			weight_P2 = 22;
			weight_I = 33;
		}
		else if (bpp > 6)
		{
			weight_P = 10;
			weight_P3 = 12;
			weight_P2 = 25;
			weight_I = 40;
		}
		else
		{
			weight_P = 10;
			weight_P3 = 12;
			weight_P2 = 28;
			weight_I = 46;
		}
	}

	GOPRCInit(encRC, targetBitrate, GOPSize, fincr, fbase);
	RCInit(encRC);

	TEncRCSeq_create(&encRC->m_encRCSeq, targetBitrate, fbase, fincr, GOPSize, mbcount, numberOfLevel, adaptiveBit, encRC->m_chn);
    initBitsRatio(&encRC->m_encRCSeq, weight_I, weight_SP, weight_LT, weight_P2, weight_P3, weight_P, 1);
	initGOPID2Level(&encRC->m_encRCSeq, 1);
	initPicPara(&encRC->m_encRCSeq);
}

static void TEncRateCtrl_first(TEncRateCtrl *encRC, int frame_size)
{
    int numPic = 0;

    if (NULL == encRC) {
		rc_err("NULL == encRC\r\n");
        goto exit_first;
    }

    initRCGOP(encRC, encRC->m_GOPSize);

	//JingHE
	if (encRC->m_svcLayer > 0)
	//if (encRC->m_svcLayer == 2)
		numPic = 4;
	initRCHPGOP(encRC, encRC->m_GOPSize, numPic);

    initRCPic(encRC, &encRC->m_encRCPic, RC_I_FRAME_LEVEL);

exit_first:
    return;
}

static int HMRateControlGetPFrameQP(TEncRateCtrl *encRC)
{
	int qp;// = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, encRC->m_currQP);

    if (H26X_RC_EVBR == encRC->m_rcMode && EVBR_STILL_STATE == encRC->m_encRCEvbr.m_evbrState) {
        if (encRC->m_SP_sign)
            qp = encRC->m_encRCEvbr.m_stillKeyPQP;
        else
            qp = encRC->m_encRCEvbr.m_stillPQP;
    }
    else
        qp = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, encRC->m_currQP);
    return qp;
}

#if EVBR_ADJUST_QP
static int HMRateControlGetPFrameEVBRQP(TEncRateCtrl *encRC, int qp, int keyp)
{
	if (keyp) {
		if (RC_EVBR_LIMIT_LOW == RCGetEvbrPreLimitstate(encRC->m_chn))
			qp += RCGetStillPDQP(encRC->m_chn);
	}
	else {
		if (RC_EVBR_LIMIT_MIDDLE == RCGetEvbrPreLimitstate(encRC->m_chn))
			qp += g_EVBRStillMidDQP;
		else if (RC_EVBR_LIMIT_HIGH == RCGetEvbrPreLimitstate(encRC->m_chn))
			qp += g_EVBRStillHighDQP;
		else
			qp += RCGetStillPDQP(encRC->m_chn);
	}
	return qp;
}
#endif

#if ALPHA_CONVERG_DIRECT
static void calculateAlphaConvCnt(TEncRateCtrl *encRC, TEncRCPic *encRCPic, int cur_alpha, int next_alpha)
{
	// alpha increase: cnt>0, alpha decrease: cnt<0, alpha convergence change direction: cnt reset to 0
	if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL)
	{
		int cnt = encRC->m_encRCSeq.m_AlphaConvergeCnt;
		if (cur_alpha > next_alpha)
		{
			if (cnt > 0)
			{
				// previous alpha increase, but now alpha decrease, then reset
				cnt = 0;
			}
			else
			{
				//previous alpha decrease or same
				cnt--;
			}
		}
		else if (cur_alpha < next_alpha)
		{
			if (cnt < 0)
			{
				// previous alpha decrease, but now alpha increase, then reset
				cnt = 0;
			}
			else
			{
				//previous alpha increase or same
				cnt++;
			}
		}
		else
		{
			// alpha same
			if (cnt > 0)
			{
				cnt++;
			}
			else if (cnt < 0)
			{
				cnt--;
			}
		}
		encRC->m_encRCSeq.m_AlphaConvergeCnt = cnt;
	}
}
#endif


#if EVALUATE_PRED_DIFF
static int checkRCResetAlphaBeta(TEncRateCtrl *encRC, TEncRCPic *encRCPic)
{
#if WEIGHT_CAL_RESET==3
	encRC->m_reset_alpha_bool = 0;
	#if ALPHA_BETA_RESET_COD_ADJUST
		if (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL && (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < g_ResetAlphaMinVal || 
			encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta>g_ResetBetaMaxVal) && 
			((g_ResetAlphaMinVal >= g_RCAlphaMinValue) && (g_ResetBetaMaxVal >= g_RCBetaMinValue)) ){
	#else
	if (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL && (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < 150 || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta>-400)){
	#endif
		encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
		encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
		encRC->m_reset_alpha_bool = 1;
	}
#elif WEIGHT_CAL_RESET==4
	//encRC->m_reset_alpha_bool = 0;
	RCSetResetAlphaBool(encRC->m_chn, 0);
	#if ADJUST_ALPHA_RESET_BITRANGE					
	if( (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL) && ((RCGetPredDiff(encRC->m_chn) < (-1*(encRC->m_GOPbitrate>>3))) || (RCGetPredDiff(encRC->m_chn) > (encRC->m_GOPbitrate>>3))) )
	{
		#if ALPHA_CONVERG_DIRECT
		if (( g_AlphaConvergCntThr == 0 ) || 
			( ( RCGetPredDiff(encRC->m_chn) > 0 ) && (encRC->m_encRCSeq.m_AlphaConvergeCnt > 0) && (RC_ABS(encRC->m_encRCSeq.m_AlphaConvergeCnt) > g_AlphaConvergCntThr) ) || 
			( ( RCGetPredDiff(encRC->m_chn) < 0 ) && (encRC->m_encRCSeq.m_AlphaConvergeCnt < 0) && (RC_ABS(encRC->m_encRCSeq.m_AlphaConvergeCnt) > g_AlphaConvergCntThr) ) )
		{
		// predDiff>0 --> bitrate insufficient --> alpha convergence be samller; predDiff<0 --> bitrate too much --> alpha convergence be bigger
		#endif
			// bitrate overflow & alpha/beta extreme value
			#if ALPHA_BETA_RESET_COD_ADJUST
			if ((encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < g_ResetAlphaMinVal || 
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta > g_ResetBetaMaxVal) && 
				((g_ResetAlphaMinVal >= g_RCAlphaMinValue) && (g_ResetBetaMaxVal >= g_RCBetaMinValue)))
			#else
			if (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < 150 || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta > -400)
			#endif
			{
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
				//encRC->m_reset_alpha_bool = 1;
				RCSetResetAlphaBool(encRC->m_chn, 1);
				//DBG_DUMP("==== reset alpha !!! ====\n");
			}
			if (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha >= g_RCAlphaMaxValue || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta >= g_RCBetaMaxValue)
			{
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
				//encRC->m_reset_alpha_bool = 1;
				RCSetResetAlphaBool(encRC->m_chn, 1);
				//DBG_DUMP("==== reset alpha !!! ====\n");
			}
			//DBG_DUMP("[%d] bit diff:\t%ld\tpred diff\t%ld\t\n", (int)encRC->m_encRCSeq.m_gopCnt, (long)encRC->m_encRCSeq.m_bitsDiff, (long)RCGetPredDiff(encRC->m_chn));
		#if ALPHA_CONVERG_DIRECT
		encRC->m_encRCSeq.m_AlphaConvergeCnt = 0;
		}
		#endif
	}
	#else
	if (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL) 
	{
		// bitrate overflow & alpha/beta extreme value
		#if ALPHA_BETA_RESET_COD_ADJUST
		if ((encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < g_ResetAlphaMinVal || 
			encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta > g_ResetBetaMaxVal) &&
			(RCGetPredDiff(encRC->m_chn) < (-1*(encRC->m_GOPbitrate>>3))) && ((g_ResetAlphaMinVal >= g_RCAlphaMinValue) && (g_ResetBetaMaxVal >= g_RCBetaMinValue)) ) {
		#else
		if ((encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < 150 || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta > -400) &&
			(RCGetPredDiff(encRC->m_chn) < (-1*(encRC->m_GOPbitrate>>3))) ) {
		#endif
			encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
			encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
			//encRC->m_reset_alpha_bool = 1;
			RCSetResetAlphaBool(encRC->m_chn, 1);
			//DBG_DUMP("==== reset alpha !!! ====\n");
		}
		if ((encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha >= g_RCAlphaMaxValue || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta >= g_RCBetaMaxValue) &&
			(encRC->m_maxGOPQP == encRC->m_minGOPQP && encRC->m_maxGOPQP == encRC->m_maxPQp) && 
			(RCGetPredDiff(encRC->m_chn) > (encRC->m_GOPbitrate>>3))) {
			encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
			encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
			//encRC->m_reset_alpha_bool = 1;
			RCSetResetAlphaBool(encRC->m_chn, 1);
			//DBG_DUMP("==== reset alpha !!! ====\n");
		}
		//DBG_DUMP("[%d] bit diff:\t%ld\tpred diff\t%ld\t\n", (int)encRC->m_encRCSeq.m_gopCnt, (long)encRC->m_encRCSeq.m_bitsDiff, (long)RCGetPredDiff(encRC->m_chn));
	}
	#endif
#endif
	return 0; 
}
#endif

static int estimatePicLambda(TEncRateCtrl *encRC, TEncRCPic *encRCPic, bool keyframe)
{
	int alpha;// = encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha;
	int beta;// = encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta;
	int estLambda;
	int ialpha;
	int ibeta;
	int ibpp = 0;
	bool multiply;
	int lastLevelLambda;// = -1;
	int lastPicLambda;// = -1;
	int lastValidLambda;// = -1;
	int64_t tmp64;
#if MAX_SIZE_BUFFER
	int max_buffer_lambda;
#endif

	if (encRC->m_SP_sign == 1 && encRC->m_SP_index == 0) {
		//encRC->m_SP_index = 1;
		encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_alpha = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_alpha;
		encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_beta = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_beta;
        if (encRC->m_svcLayer > 0)
        {
            encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_alpha = encRC->m_encRCSeq.m_picPara[RC_P2_FRAME_LEVEL].m_alpha;
            encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_beta  = encRC->m_encRCSeq.m_picPara[RC_P2_FRAME_LEVEL].m_beta;
        }
	}

    if (encRC->m_LT_sign == 1 && encRC->m_LT_index == 0)
    {
        //encRC->m_LT_index = 1;
        encRC->m_encRCSeq.m_picPara[RC_LT_P_FRAME_LEVEL].m_alpha = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_alpha;
        encRC->m_encRCSeq.m_picPara[RC_LT_P_FRAME_LEVEL].m_beta  = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_beta;
        if (encRC->m_svcLayer > 0)
        {
            encRC->m_encRCSeq.m_picPara[RC_LT_P_FRAME_LEVEL].m_alpha = encRC->m_encRCSeq.m_picPara[RC_P2_FRAME_LEVEL].m_alpha;
            encRC->m_encRCSeq.m_picPara[RC_LT_P_FRAME_LEVEL].m_beta  = encRC->m_encRCSeq.m_picPara[RC_P2_FRAME_LEVEL].m_beta;
        }
    }

	alpha = encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha;
	beta = encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta;
	ialpha = alpha;
	ibeta = beta;

	tmp64 = (int64_t)encRCPic->m_targetBits;
	tmp64 <<= SHIFT_11;

	if (tmp64 > 922337203685477ll){/* (2^63 - 1) / TIMES_10 */
		tmp64 = 922337203685477ll;
	}

	tmp64 = tmp64 * TIMES_10;
	tmp64 = division(tmp64, encRCPic->m_numberOfMB);
	tmp64 >>= (SHIFT_11 + 8);
	ibpp = RC_CLIP3(1, (int64_t)MAX_INTEGER, tmp64); /* avoid integer overflowing */

	multiply = 1;
	//rc_inf("start estimatePicLambda - m_targetBits : %d\tbpp : %d\n", (int)(encRCPic->m_targetBits), (int)(ibpp));

	estLambda = calculatePow(ialpha, ibpp, ibeta, multiply);
	rc_inf("estimatePicLambda - ialpha : %d\tibeta : %d\tibpp : %d\testLambda : %d\n", (int)(ialpha), (int)(ibeta), (int)(ibpp), (int)(estLambda));

	/* record previous I/P lambda */
	lastLevelLambda = encRC->m_lastLevelLambda[encRCPic->m_frameLevel];
	lastPicLambda = encRC->m_lastPicLambda;
	lastValidLambda = encRC->m_lastValidLambda;

	/* lastLevel consider P frame and non-second I frame */

#if RD_LAMBDA_UPDATE
	if (lastLevelLambda > 0 && (encRCPic->m_frameLevel>0 || (0 == encRCPic->m_frameLevel && 2 == encRC->m_encRCSeq.keyframe_num)) && encRC->disable_clip == 0) {
#else
	if (lastLevelLambda > 0 && (encRCPic->m_frameLevel>0 || (0 == encRCPic->m_frameLevel && 2 == encRC->m_encRCSeq.keyframe_num))) {
#endif
		lastLevelLambda = RC_CLIP3(205, 20480000, lastLevelLambda);
		estLambda = RC_CLIP3(lastLevelLambda / 2, lastLevelLambda * 2, estLambda);
	}

#if MAX_SIZE_BUFFER
	max_buffer_lambda = estLambda;
#endif

	if (lastPicLambda > 0) {
		lastPicLambda = RC_CLIP3(205, 4096000, lastPicLambda);
		estLambda = RC_CLIP3(lastPicLambda * 99 / 1000, lastPicLambda * 101 / 10, estLambda);
	}
	else if (lastValidLambda > 0) {
		lastValidLambda = RC_CLIP3(205, 4096000, lastValidLambda);
		estLambda = RC_CLIP3(lastValidLambda * 99 / 1000, lastValidLambda * 101 / 10, estLambda);
	}
	else {
		estLambda = RC_CLIP3(205, 20480000, estLambda);
	}

	if (estLambda < 205) {
		estLambda = 205;
	}
#if RD_LAMBDA_UPDATE
	estLambda = RC_CLIP3(205, 20480000, estLambda);
#endif
#if MAX_SIZE_BUFFER
	if (encRC->m_chn < MAX_LOG_CHN) {
		if (g_max_frm_bool[encRC->m_chn] == 1)
			estLambda = max_buffer_lambda;
	}
#endif
	encRCPic->m_estPicLambda = estLambda;

	return estLambda;
}

static int estimatePicQP(TEncRateCtrl *encRC, TEncRCPic *encRCPic, int lambda, int actualTotalBits)
{
	/*
	*  (Int)(2048 * 4.2005) = 8602
	*  (Int)((13.7122 + 0.5) * 2048) = 29107
	*/
	int ilambda;
	int QP;
	int lastLevelQP;
    int currPicPosition;
#if MAX_SIZE_BUFFER
	int max_buffer_qp;
#endif

	/*
	*   if (encRC->m_slicetype == MCP_I_SLICE)
	*	    QP = (int)(4.3281 * log(lambda) + 14.433 + 0.5);
	*   else
	*	    QP = (int)(4.3281 * log(lambda) + 14.595 + 0.5);
	*/
	if (encRC->m_slicetype == RC_I_SLICE) //2048*4.3281=8864 (14.433+0.5)*2048=30582
	{
		if (lambda > 214748){ /* (2^31 - 1) / TIMES_10 */
			ilambda = (lambda + (SHIFT_VALUE >> 1)) >> SHIFT_11;
			QP = lookupLogListTable(ilambda, 0) * 8864 / LOG10_EXP + 30582;
			QP >>= SHIFT_11;
		}
		else{
			ilambda = (lambda * TIMES_10) >> SHIFT_11;
			QP = lookupLogListTable(ilambda, 1) * 8864 / LOG10_EXP + 30582;
			QP >>= SHIFT_11;
		}
	}
	else  // //2048*4.3281=8864 (14.595+0.5)*2048=30914
	{
		if (lambda > 214748){ /* (2^31 - 1) / TIMES_10 */
			ilambda = (lambda + (SHIFT_VALUE >> 1)) >> SHIFT_11;
			QP = lookupLogListTable(ilambda, 0) * 8864 / LOG10_EXP + 30914;
			QP >>= SHIFT_11;
		}
		else{
			ilambda = (lambda * TIMES_10) >> SHIFT_11;
			QP = lookupLogListTable(ilambda, 1) * 8864 / LOG10_EXP + 30914;
			QP >>= SHIFT_11;
		}
	}

	rc_inf("estimatePicQP - QP : %d\n", (int)(QP));

	/* lastLevel consider P frame and non-second I frame */
	lastLevelQP = encRC->m_lastLevelQP[encRCPic->m_frameLevel];
#if RD_LAMBDA_UPDATE
	if (lastLevelQP > g_RCInvalidQPValue && (encRCPic->m_frameLevel > 0 || (0 == encRCPic->m_frameLevel && 2 == encRC->m_encRCSeq.keyframe_num)) && encRC->disable_clip == 0) {
#else
	if (lastLevelQP > g_RCInvalidQPValue && (1 == encRCPic->m_frameLevel || (0 == encRCPic->m_frameLevel && 2 == encRC->m_encRCSeq.keyframe_num))) {
#endif
        QP = RC_CLIP3(lastLevelQP - 3, lastLevelQP + 3, QP);
	}
#if MAX_SIZE_BUFFER
	max_buffer_qp = QP;
#endif
	if (encRC->m_lastPicQP > g_RCInvalidQPValue) {
		QP = RC_CLIP3(encRC->m_lastPicQP - 10, encRC->m_lastPicQP + 10, QP);
	#if RC_GOP_QP_LIMITATION
		if (encRC->m_useGopQpLimitation && encRC->m_GopQpLimit && (encRC->m_lastpQP >= (int)encRC->m_minPQp && encRC->m_lastpQP <= (int)encRC->m_maxPQp))
		{
			if (encRCPic->m_frameLevel == RC_I_FRAME_LEVEL)
			{
				int tmp_QP = encRC->m_lastpQP - (encRC->m_rc_deltaQPip + 9) / 10;
				QP = RC_CLIP3(tmp_QP - 10, tmp_QP + 3, QP);
			}
			else if (encRCPic->m_frameLevel == RC_LT_P_FRAME_LEVEL)
			{
				int tmp_QP = encRC->m_lastpQP - (encRC->m_deltaLT_QP + 9) / 10;
				QP = RC_CLIP3(tmp_QP - 6, tmp_QP + 3, QP);
			}
			else if (encRCPic->m_frameLevel == RC_KEY_P_FRAME_LEVEL)
			{
				int tmp_QP = encRC->m_lastpQP - (encRC->m_rc_deltaQPkp + 9) / 10;
				QP = RC_CLIP3(tmp_QP - 6, tmp_QP + 3, QP);
			}
		}
	#endif
	}
	else if (encRC->m_lastValidQP > g_RCInvalidQPValue) {
		QP = RC_CLIP3(encRC->m_lastValidQP - 10, encRC->m_lastValidQP + 10, QP);
	}

#if WEIGHT_CAL_RESET
	if (g_svcAdjustQPWeight)
#else
    if (g_AdjustQPWeight)
#endif
	{
        currPicPosition = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
        if (encRC->m_HP_index == 1 && (encRC->m_lastpQP<(int)encRC->m_maxPQp)) {  //HK
            //int tmpdeltaQP = 0;
            if (encRCPic->m_frameLevel == RC_I_FRAME_LEVEL)
            {
                int tmp_QP = encRC->m_lastpQP - (encRC->m_rc_deltaQPip + 9) / 10;
                QP = RC_CLIP3(tmp_QP - 6, tmp_QP + 6, QP);
            }

            if (encRCPic->m_frameLevel == RC_LT_P_FRAME_LEVEL)
            {
                int tmp_QP = encRC->m_lastpQP - (encRC->m_deltaLT_QP + 9) / 10;
                QP = RC_CLIP3(tmp_QP - 6, tmp_QP + 6, QP);
            }
            else if (encRCPic->m_frameLevel == RC_KEY_P_FRAME_LEVEL)
            {
                int tmp_QP = encRC->m_lastpQP - (encRC->m_rc_deltaQPkp + 9) / 10;
                QP = RC_CLIP3(tmp_QP - 6, tmp_QP + 6, QP);
            }
            else if (encRCPic->m_frameLevel == RC_P2_FRAME_LEVEL)
            {
                int tmp_QP = encRC->m_lastpQP - (encRC->m_deltaP2_QP + 9) / 10;
                QP = RC_CLIP3(tmp_QP - 3, tmp_QP + 3, QP);
            }
            else if (encRCPic->m_frameLevel == RC_P3_FRAME_LEVEL)
            {
                int tmp_QP = encRC->m_lastpQP - (encRC->m_deltaP3_QP + 9) / 10;
                QP = RC_CLIP3(tmp_QP - 3, tmp_QP + 3, QP);
            }
        }
        if (encRC->m_HP_sign == 1 && encRC->m_HP_index == 0 && (encRC->m_lastPicQP<(int)encRC->m_maxPQp)) { // first HP gop  //HK
            if (encRCPic->m_frameLevel == RC_P3_FRAME_LEVEL)
                QP = encRC->m_lastPicQP - (encRC->m_deltaP3_QP + 9) / 10;
            if (encRCPic->m_frameLevel == RC_P2_FRAME_LEVEL)
                QP = encRC->m_lastPicQP - (encRC->m_deltaP2_QP + 9) / 10;
            if (encRC->m_svcLayer == 2){
                if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL && currPicPosition % 4 == 3)
                    QP = encRC->m_lastPicQP + (encRC->m_deltaP3_QP + 9) / 10;
            }
            if (encRC->m_svcLayer == 1){
                if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL && currPicPosition % 4 == 3)
                    QP = encRC->m_lastPicQP + (encRC->m_deltaP2_QP + 9) / 10;
            }
            if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL && currPicPosition % 4 == 1)
                QP = encRC->m_lastPicQP + (encRC->m_rc_deltaQPip + 9) / 10;
        }
    }
	if (g_RCResetAlpgaBetaMethod) {
		if (1 == g_RCResetAlpgaBetaMethod) {
		#if WEIGHT_CAL_RESET==3
			encRC->m_reset_alpha_bool = 0;
			#if ALPHA_BETA_RESET_COD_ADJUST
			if (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL && (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < g_ResetAlphaMinVal || 
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta>g_ResetBetaMaxVal) && 
				((g_ResetAlphaMinVal >= g_RCAlphaMinValue) && (g_ResetBetaMaxVal >= g_RCBetaMinValue))){
			#else
			if (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL && (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < 150 || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta>-400)){
			#endif
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
				encRC->m_reset_alpha_bool = 1;
			}
		#elif WEIGHT_CAL_RESET==4
			RCSetResetAlphaBool(encRC->m_chn, 0);
			#if ADJUST_ALPHA_RESET_BITRANGE
			if( (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL) && ((encRC->m_encRCSeq.m_bitsDiff < (-1*(encRC->m_GOPbitrate>>3))) || (encRC->m_encRCSeq.m_bitsDiff > (encRC->m_GOPbitrate>>3))) )
			{
				#if ALPHA_BETA_RESET_COD_ADJUST
				if ((encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < g_ResetAlphaMinVal || 
					encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta > g_ResetBetaMaxVal) && 
					((g_ResetAlphaMinVal >= g_RCAlphaMinValue) && (g_ResetBetaMaxVal >= g_RCBetaMinValue)) )
				#else
				if (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha < 150 || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta > -400)
				#endif
				{
					encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
					encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
					//encRC->m_reset_alpha_bool = 1;
					RCSetResetAlphaBool(encRC->m_chn, 1);
					//DBG_DUMP("==== reset alpha !!! ====\n");
				}
				if (encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha >= g_RCAlphaMaxValue || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta >= g_RCBetaMaxValue) 
				{
					encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
					encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
					//encRC->m_reset_alpha_bool = 1;
					RCSetResetAlphaBool(encRC->m_chn, 1);
					//DBG_DUMP("==== reset alpha !!! ====\n");
				}
			}
			#else
			if ((encRCPic->m_frameLevel != RC_I_FRAME_LEVEL) &&
				(encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha >= g_RCAlphaMaxValue || encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta >= g_RCBetaMaxValue) &&
				(encRC->m_maxGOPQP == encRC->m_minGOPQP && encRC->m_maxGOPQP == encRC->m_maxPQp) && 
				(encRC->m_encRCSeq.m_bitsDiff > (encRC->m_GOPbitrate>>3)))
			{
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha = 6554;
				encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta = -2800;
				//encRC->m_reset_alpha_bool = 1;
				RCSetResetAlphaBool(encRC->m_chn, 1);
				//DBG_DUMP("==== reset alpha !!! ====\n");
			}
			#endif
		#endif
		}
		else if (2 == g_RCResetAlpgaBetaMethod) {
		#if EVALUATE_PRED_DIFF
			checkRCResetAlphaBeta(encRC, encRCPic);
		#endif
		}
	}
#if MAX_SIZE_BUFFER
	if (encRC->m_chn < MAX_LOG_CHN) {
		if (g_max_frm_bool[encRC->m_chn] == 1)
			QP = max_buffer_qp;
	}
#endif
	return QP;
}

static void TEncRCPic_setRCLog(TEncRateCtrl *encRC, TEncRCPic *encRCPic, int calLambda, int alpha, int beta)
{
    encRC->rc_log_info.frameLevel = encRCPic->m_frameLevel;
    encRC->rc_log_info.ave_ROWQP = encRC->ave_ROWQP;
    encRC->rc_log_info.currQP = encRC->m_currQP;
    encRC->rc_log_info.picActualBits = encRCPic->m_picActualBits;
    encRC->rc_log_info.picTargetBits = encRCPic->m_targetBits;
    encRC->rc_log_info.calLambda = calLambda;
    encRC->rc_log_info.picLambda = encRCPic->m_picLambda;
    encRC->rc_log_info.alpha = alpha;
    encRC->rc_log_info.beta = beta;
    encRC->rc_log_info.bitsLeft = encRC->m_encRCGOP.m_bitsLeft;
    encRC->rc_log_info.frame_MSE = encRC->frame_MSE;
}

static void TEncRCPic_setVBRLog(TEncRateCtrl *encRC, TEncRCVBR2 *encRCVbr2, unsigned int frame_size)
{
	encRC->rc_log_info.frameLevel = encRC->m_encRCPic.m_frameLevel;
	encRC->rc_log_info.maxBitrate = encRCVbr2->m_maxBitrate;
	encRC->rc_log_info.changePosThd = encRCVbr2->m_changePosThd;
	encRC->rc_log_info.overflowLevel = encRCVbr2->m_overflowLevel;
	encRC->rc_log_info.currIQP = encRCVbr2->m_currIQP;
	encRC->rc_log_info.currPQP = encRCVbr2->m_currPQP;
	encRC->rc_log_info.currKPQP = encRCVbr2->m_currKPQP;
	encRC->rc_log_info.currLTPQP = encRCVbr2->m_currLTPQP;
	encRC->rc_log_info.currP2QP = encRCVbr2->m_currP2QP;
	encRC->rc_log_info.currP3QP = encRCVbr2->m_currP3QP;
	encRC->rc_log_info.targetBits = encRCVbr2->m_targetBits;
	encRC->rc_log_info.frameSize = frame_size;
	encRC->rc_log_info.bitsLeft = encRC->m_encRCGOP.m_bitsLeft;
}

#if ADD_WEIGHTIP_LOG
static void TEncRCPic_setWeightLog(TEncRateCtrl *encRC){
	encRC->rc_log_info.f_rc_weight_I = (encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL] <= 0)? 0: encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL];
    encRC->rc_log_info.f_rc_weight_P = (encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL] <= 0)? 0: encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL];
    encRC->rc_log_info.f_rc_weight_SP = (encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL] <= 0)? 0: encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL];
    encRC->rc_log_info.f_rc_weight_LT = (encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL] <= 0)? 0: encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL];
    encRC->rc_log_info.f_rc_weight_P2 = (encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL] <= 0)? 0: encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL];
    encRC->rc_log_info.f_rc_weight_P3 = (encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL] <= 0)? 0: encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL];
}
#endif
#if ADD_PREDIFF_LOG
static void TEncRCPic_setPredDiffLog(TEncRateCtrl *encRC, int64_t preDiff)
{
	encRC->rc_log_info.predDiff = preDiff;
}
#endif
#if (SUPPORT_VBR2_CVTE && ADD_VBR2_THD_BITREC_LOG)
static void TEncRCPic_setVBRThdLog(TEncRateCtrl *encRC, int overflowThd, int overflowThd2, int underflowThd)
{
	encRC->rc_log_info.overflowThd = overflowThd;
	encRC->rc_log_info.overflowThd2 = overflowThd2;
	encRC->rc_log_info.underflowThd = underflowThd;
}
#endif
#if ADD_VBR2_THD_BITREC_LOG
static void TEncRCPic_setVBRBitRecordLog(TEncRateCtrl *encRC, TRCBitrateRecord *encRCBr)
{
	encRC->rc_log_info.bitrate_record = encRCBr->bitrate_record;
}
#endif

static void TEncRCPic_updateAfterPicture(
	TEncRateCtrl *encRC,
	TEncRCPic *encRCPic,
	int actualHeaderBits,
	int actualTotalBits,
	int averageQP,
	int averageLambda)
{
	int alpha = encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_alpha;
	int beta = encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel].m_beta;
	TRCParameter rcPara;
	#if ALPHA_CONVERG_DIRECT
	int cur_alpha = alpha;
	#endif

	int tmp_targetbits;
	int tmp_nopixel;
	int picActualBpp;
	bool multiply;
	int calLambda;
	int ipicActualBpp;
	int ialpha = alpha;
	int ibeta = beta;
	int inputLambda;
	int iinputLambda;
	int icalLambda;
	bool inputLambda_multiply, calLambda_multiply;
	int lnbpp;

	int tmp_alpha;
	int tmp_beta;
	int currpos = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
#if WEIGHT_ADJUST
    int m_alphaUpdate;
    int m_betaUpdate;
    int m_updateLevel;
    int curr_updateLevel;
#endif

	if (encRC->m_SP_sign == 1) {
        encRC->m_SP_index = 1;
		encRC->m_SP_sign = 0;
		encRC->m_GOP_SPnumleft--;
	}
	if (encRC->m_ltrPeriod > 0)
	{
		currpos %= (int)encRC->m_ltrPeriod;
	}
	if (encRC->m_LT_sign == 1)
	{
		encRC->m_LT_index = 1;
		encRC->m_LT_sign = 0;
		encRC->m_GOP_LTnumleft--;
	}
	if (encRC->m_svcLayer > 0 && currpos > 0){
		if (currpos % 4 == 0){
			encRC->m_HP_index = 1;
			encRC->m_HP_sign = 0;
		}
		encRC->m_encRCHPGOP.m_picLeft--;
		encRC->m_encRCHPGOP.m_bitsLeft -= actualTotalBits;
		if (encRCPic->m_frameLevel == RC_P2_FRAME_LEVEL)
		{
			encRC->m_GOP_P2numleft--;
		}
		else if (encRCPic->m_frameLevel == RC_P3_FRAME_LEVEL)
		{
			encRC->m_GOP_P3numleft--;
		}
	}

	encRCPic->m_picActualHeaderBits = actualHeaderBits;
	encRCPic->m_picActualBits = actualTotalBits;

    averageQP = encRC->ave_ROWQP;

	if (averageQP > 0) {
		encRCPic->m_picQP = averageQP;
        encRC->m_lastLevelQP[encRCPic->m_frameLevel] = averageQP;
        encRC->m_lastValidQP = averageQP;
	}
	else{
		encRCPic->m_picQP = g_RCInvalidQPValue;
	}
    encRC->m_lastPicQP = averageQP;

	encRCPic->m_picLambda = averageLambda;

#if LOOKUP_TABLE_METHOD
	/* add lookup table, 20171110 */
	if (encRC->m_rowRCEnable){
	#if FIX_BUG_I_QP
		if (encRCPic->m_frameLevel == 0)
			encRCPic->m_picLambda = g_AveRowLambda_I[encRC->ave_ROWQP];
		else
			encRCPic->m_picLambda = g_AveRowLambda_P[encRC->ave_ROWQP];
	#else
		encRCPic->m_picLambda = g_AveRowLambda[encRC->ave_ROWQP];
	#endif
        averageLambda = encRCPic->m_picLambda;  //add, jz_fix bug
	}
	//rc_dump_info("QP : %d   \r\nlambda_0 : %d    lambda_1 : %d\r\n", (int)(encRC->ave_ROWQP), (int)(averageLambda), (int)(encRCPic->m_picLambda));
#endif

    // update last lambda
    encRC->m_lastLevelLambda[encRCPic->m_frameLevel] = averageLambda;
	encRC->m_lastPicLambda = averageLambda;
	if (averageLambda > 0) {
		encRC->m_lastValidLambda = averageLambda;
    }
    if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL) {
        encRC->m_lastpQP = averageQP;
    }

	// update parameters
	tmp_targetbits = encRCPic->m_picActualBits << (SHIFT_11 - 8);
	tmp_nopixel = RC_MAX(encRCPic->m_numberOfMB, 1);    // Tuba 20171019
	picActualBpp = tmp_targetbits / tmp_nopixel;

	if (picActualBpp > 214748) { /* (2^31 - 1) / TIMES_10 */
		ipicActualBpp = (picActualBpp + (SHIFT_VALUE >> 1)) >> SHIFT_11;
		multiply = 0;
	}
	else {
        int64_t tmp;
		ipicActualBpp = tmp_targetbits % tmp_nopixel;
    #if 1
        tmp = (int64_t)ipicActualBpp;
        tmp = division(tmp * TIMES_10, tmp_nopixel);
        if (tmp > MAX_INTEGER)
            ipicActualBpp = MAX_INTEGER;
        else
            ipicActualBpp = (int)tmp;
    #else
		if (ipicActualBpp > 214748) /* (2^31 - 1) / TIMES_10 */
			ipicActualBpp = (ipicActualBpp >> 5) * TIMES_10 / (tmp_nopixel >> 5);
		else
			ipicActualBpp = ipicActualBpp * TIMES_10 / tmp_nopixel;
    #endif
		ipicActualBpp = (ipicActualBpp + picActualBpp * TIMES_10) >> SHIFT_11;
		multiply = 1;
	}

	calLambda = calculatePow(ialpha, ipicActualBpp, ibeta, multiply);
	inputLambda = encRCPic->m_picLambda;

#if FIX_BUG_I_QP
#if WEIGHT_CAL_RESET==3
	if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL && encRC->m_reset_alpha_bool == 0)
#elif WEIGHT_CAL_RESET==4
	if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL && RCGetResetAlphaBool(encRC->m_chn) == 0)
#else
	if (encRCPic->m_frameLevel == RC_P_FRAME_LEVEL)
#endif
	{
		//if (actualTotalBits < encRCPic->m_targetBits && calLambda < inputLambda)
		if ((actualTotalBits < encRCPic->m_targetBits && calLambda < inputLambda) || (actualTotalBits > encRCPic->m_targetBits && calLambda > inputLambda))
		{
			int tmp_targetbits2 = encRCPic->m_targetBits << (SHIFT_11 - 8);
			int tmp_nopixel2 = RC_MAX(encRCPic->m_numberOfMB, 1);    // Tuba 20171019
			int picActualBpp2 = tmp_targetbits2 / tmp_nopixel2;
			int ipicActualBpp2;
			bool multiply2;
			#if ALPHA_BETA_MIN_MAX_ISSUE
			int tmp_Lambda, AbsDiff_NewLambda, AbsDiff_OriLambda;		// YCT
			#endif

			if (picActualBpp2 > 214748) { /* (2^31 - 1) / TIMES_10 */
				ipicActualBpp2 = (picActualBpp2 + (SHIFT_VALUE >> 1)) >> SHIFT_11;
				multiply2 = 0;
			}
			else {
				int64_t tmp2;
				ipicActualBpp2 = tmp_targetbits2 % tmp_nopixel2;
				tmp2 = (int64_t)ipicActualBpp2;
				tmp2 = division(tmp2 * TIMES_10, tmp_nopixel2);
				if (tmp2 > MAX_INTEGER)
					ipicActualBpp2 = MAX_INTEGER;
				else
					ipicActualBpp2 = (int)tmp2;

				ipicActualBpp2 = (ipicActualBpp2 + picActualBpp2 * TIMES_10) >> SHIFT_11;
				multiply2 = 1;
			}
		#if ALPHA_BETA_MIN_MAX_ISSUE
			tmp_Lambda = calculatePow(ialpha, ipicActualBpp2, ibeta, multiply2);
			AbsDiff_NewLambda = RC_ABS(tmp_Lambda - calLambda);
			AbsDiff_OriLambda = RC_ABS(inputLambda - calLambda);
			if ((0 == g_RCUpdateAlphaCndEn) || ((AbsDiff_NewLambda < AbsDiff_OriLambda) && (encRC->m_encRCGOP.m_picLeft != 1)))
			{
				encRCPic->m_picLambda = tmp_Lambda;//calculatePow(ialpha, ipicActualBpp2, ibeta, multiply2);
				averageLambda = encRCPic->m_picLambda;
				inputLambda = encRCPic->m_picLambda;
				// update last lambda
				encRC->m_lastLevelLambda[encRCPic->m_frameLevel] = averageLambda;
				encRC->m_lastPicLambda = averageLambda;
				if (averageLambda > 0)
					encRC->m_lastValidLambda = averageLambda;
			}
		#else
			encRCPic->m_picLambda = calculatePow(ialpha, ipicActualBpp2, ibeta, multiply2);
			averageLambda = encRCPic->m_picLambda;
			inputLambda = encRCPic->m_picLambda;
			// update last lambda
			encRC->m_lastLevelLambda[encRCPic->m_frameLevel] = averageLambda;
			encRC->m_lastPicLambda = averageLambda;
			if (averageLambda > 0)
				encRC->m_lastValidLambda = averageLambda;
		#endif
		}
	}
#endif
    rc_dbg("{chn%d} slice_type:%d, rowQP:%d, frameQP:%d, actual_bit:%d, target_bit:%d, callambda:%d, lambda_1:%d, alpha:%d, beta:%d, bitLeft:%lld, sse:%lld\r\n",
        encRC->m_chn, encRCPic->m_frameLevel, encRC->ave_ROWQP, encRC->m_currQP, encRCPic->m_picActualBits, encRCPic->m_targetBits, calLambda, encRCPic->m_picLambda, alpha, beta, encRC->m_encRCGOP.m_bitsLeft, encRC->frame_MSE);
	TEncRCPic_setRCLog(encRC, encRCPic, calLambda, alpha, beta);
#if ADD_WEIGHTIP_LOG
	TEncRCPic_setWeightLog(encRC);
#endif

#if WEIGHT_ADJUST
	if (encRCPic->m_frameLevel == 0)
		encRC->m_encRCSeq.keyframe_num = (0 == encRC->m_encRCSeq.keyframe_num) ? 1 : 2;
#endif
#if RD_LAMBDA_UPDATE
	encRC->disable_clip = 0;
#if RD_LAMBDA_UPDATE==2
	if (encRCPic->m_frameLevel != RC_I_FRAME_LEVEL && encRCPic->m_picActualBits > 2 * encRCPic->m_targetBits && (encRC->MotionRatio>encRC->ave_moRatio || encRC->MotionRatio==encRC->ave_moRatio))
	{
		encRC->disable_clip = 1;
	}
	encRC->ave_moRatio = (encRC->ave_moRatio + encRC->MotionRatio) / 2;

	//if (encRCPic->m_frameLevel == RC_I_FRAME_LEVEL || encRCPic->m_frameLevel == RC_KEY_P_FRAME_LEVEL || (encRCPic->m_picActualBits > 2 * encRCPic->m_targetBits && encRC->MotionRatio > 0))//KP jisuan
	if (encRCPic->m_frameLevel == RC_I_FRAME_LEVEL || encRCPic->m_frameLevel == RC_KEY_P_FRAME_LEVEL || encRCPic->m_frameLevel == RC_LT_P_FRAME_LEVEL || ((encRCPic->m_picActualBits > 2 * encRCPic->m_targetBits) && encRC->MotionRatio>0)) // jz_fix bug
#else
	if (encRCPic->m_frameLevel == RC_I_FRAME_LEVEL || encRCPic->m_frameLevel == RC_KEY_P_FRAME_LEVEL || encRCPic->m_frameLevel == RC_LT_P_FRAME_LEVEL)
#endif
	{
		int64_t tmp_d1;
		int64_t tmp_d4 = (int64_t)encRCPic->m_picActualBits *encRCPic->m_picLambda;
		int tmp_sse;
		int count = 0;
        int64_t tmp2_sse = encRC->frame_MSE;
		int64_t tmp_alpha1;

		//while (tmp2_sse >= MAX_INTEGER){
		while (tmp2_sse >= 1000000){ //jz_fix_bug (MAX_INTEGER)
			tmp2_sse >>= 1;
			count++;
		}
		tmp_sse = (int)RC_CLIP3(0, (int64_t)MAX_INTEGER, tmp2_sse);
		tmp_d4 >>= count;
		tmp_d1 = division(tmp_d4, tmp_sse);
		tmp_alpha1 = division((int64_t)encRCPic->m_picLambda*encRCPic->m_picActualBits,  256 * encRCPic->m_numberOfMB);
		alpha = calculatePow(tmp_alpha1, ipicActualBpp, tmp_d1, multiply);
		beta = -2048 - tmp_d1;
		alpha = RC_CLIP3(g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha);
		beta = RC_CLIP3(g_RCBetaMinValue, g_RCBetaMaxValue, beta);
		rcPara.m_alpha = alpha;
		rcPara.m_beta = beta;
		encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel] = rcPara;
		return;
	}
#endif
	if (inputLambda < 21 || calLambda < 21 || picActualBpp < 1) {
		TRCParameter tmp_rcPara;
    #if WEIGHT_ADJUST
        m_alphaUpdate = encRC->m_encRCSeq.m_alphaUpdate; // assign before used
        m_betaUpdate = encRC->m_encRCSeq.m_betaUpdate;
		m_updateLevel = encRC->m_encRCSeq.m_updateLevel;
        if (calLambda > 10 * inputLambda || calLambda*10 < inputLambda) {
            if (picActualBpp < 11)
			{
				curr_updateLevel = m_updateLevel + 1;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else if (picActualBpp < 26)
			{
				curr_updateLevel = m_updateLevel + 2;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else if (picActualBpp < 64)
			{
				curr_updateLevel = m_updateLevel + 3;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else
			{
				curr_updateLevel = m_updateLevel + 4;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
            alpha = alpha * (SHIFT_VALUE - (m_alphaUpdate >> 1)) >> SHIFT_11;
            beta = beta * (SHIFT_VALUE - (m_betaUpdate >> 1)) >> SHIFT_11;
        }
    #else
		alpha = alpha * (SHIFT_VALUE - (encRC->m_encRCSeq.m_alphaUpdate >> 1)) >> SHIFT_11;
		beta = beta * (SHIFT_VALUE - (encRC->m_encRCSeq.m_betaUpdate >> 1)) >> SHIFT_11;
    #endif
		alpha = RC_CLIP3(g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha);
		beta = RC_CLIP3(g_RCBetaMinValue, g_RCBetaMaxValue, beta);

		tmp_rcPara.m_alpha = alpha;
		tmp_rcPara.m_beta = beta;
		encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel] = tmp_rcPara;

		rc_inf("updateAfterPic - calLambda : %d\tinputLambda : %d\tpicActualBpp : %d\n", (int)(calLambda), (int)(inputLambda), (int)(picActualBpp));
		return;
	}
#if WEIGHT_ADJUST
	m_alphaUpdate = encRC->m_encRCSeq.m_alphaUpdate;
	m_betaUpdate = encRC->m_encRCSeq.m_betaUpdate;
	m_updateLevel = encRC->m_encRCSeq.m_updateLevel;
    #if HANDLE_DIFF_LTR_KEYP_PERIOD
    if (encRCPic->m_frameLevel==0 || encRC->m_ltrFrame)
    #else
	if (encRCPic->m_frameLevel==0)
    #endif
	{
		if (calLambda > 10 * inputLambda || calLambda*10 < inputLambda)
		{
			if (picActualBpp < 11)
			{
				curr_updateLevel = m_updateLevel + 1;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else if (picActualBpp < 26)
			{
				curr_updateLevel = m_updateLevel + 2;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else if (picActualBpp < 64)
			{
				curr_updateLevel = m_updateLevel + 3;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else
			{
				curr_updateLevel = m_updateLevel + 4;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
		}
		else if (calLambda > 5 * inputLambda || calLambda*10 < 3*inputLambda)
		{
			if (picActualBpp < 26)
			{
				curr_updateLevel = m_updateLevel + 1;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else if (picActualBpp < 64)
			{
				curr_updateLevel = m_updateLevel + 2;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else
			{
				curr_updateLevel = m_updateLevel + 3;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}

			//m_alphaUpdate = 410;
			//m_betaUpdate = 205;
		}
		else if ((calLambda*2 > 5*inputLambda || calLambda*10 < 6*inputLambda))
		{
			if (picActualBpp < 64)
			{
				curr_updateLevel = m_updateLevel + 1;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}
			else
			{
				curr_updateLevel = m_updateLevel + 2;
				if (curr_updateLevel > 4)
					curr_updateLevel = 4;
				m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[curr_updateLevel];
				m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[curr_updateLevel];
			}

			//m_alphaUpdate = 205;
			//m_betaUpdate = 102;
		}
	}
#endif

	calLambda = RC_CLIP3(inputLambda / 10, inputLambda * 10, calLambda);

#if FIX_BUG_I_QP
	if (inputLambda < calLambda && encRCPic->m_frameLevel == RC_P_FRAME_LEVEL)
	{
		m_alphaUpdate = encRC->m_encRCSeq.m_updatealpha[m_updateLevel] * 3 / 2;
		m_betaUpdate = encRC->m_encRCSeq.m_updatebeta[m_updateLevel] * 3 / 2;
	}
#endif

	if (inputLambda > 214748) { /* (2^31 - 1) / TIMES_10 */
		iinputLambda = inputLambda;
		inputLambda_multiply = 0;
	}
	else {
		iinputLambda = inputLambda * TIMES_10;
		inputLambda_multiply = 1;
	}
	if (calLambda > 214748) { /* (2^31 - 1) / TIMES_10 */
		icalLambda = calLambda;
		calLambda_multiply = 0;
	}
	else {
		icalLambda = calLambda * TIMES_10;
		calLambda_multiply = 1;
	}

	tmp_alpha = (lookupLogListTable(iinputLambda, inputLambda_multiply) - lookupLogListTable(icalLambda, calLambda_multiply)) * alpha;
	rc_inf("tmp_alpha : %d\n", (int)(tmp_alpha));
#if WEIGHT_ADJUST
    if (tmp_alpha > 2622080 || tmp_alpha < -2622080){ /* (2^31 - 1) / max_alphaUpdate(819) */
    	alpha += tmp_alpha / LOG10_EXP * m_alphaUpdate >> SHIFT_11;
    }
    else{
    	alpha += tmp_alpha * m_alphaUpdate / LOG10_EXP >> SHIFT_11;
    }
    //alpha += encRC->m_encRCSeq->m_alphaUpdate * (lookupLogListTable(iinputLambda, inputLambda_multiply) - lookupLogListTable(icalLambda, calLambda_multiply)) * alpha / lookupLogListTable(INT_EXP, 1) >> SHIFT_11;

    if (picActualBpp > 214748){ /* (2^31 - 1) / TIMES_10 */
    	lnbpp = (lookupLogListTable(ipicActualBpp, 0) << SHIFT_11) / LOG10_EXP;
    }
    else{
    	lnbpp = (lookupLogListTable(ipicActualBpp, 1) << SHIFT_11) / LOG10_EXP;
    }

    //rc_inf("lnbpp : %d\n", (int)(lnbpp));

    lnbpp = RC_CLIP3(-10240, -205, lnbpp);

    tmp_beta = (lookupLogListTable(iinputLambda, inputLambda_multiply) - lookupLogListTable(icalLambda, calLambda_multiply)) * lnbpp;

    //rc_inf("tmp_beta : %d\n", (int)(tmp_beta));

    if (tmp_beta > 5237764 || tmp_beta < -5237764){ /* (2^31 - 1) / max_betaUpdate(410) */
    	beta += tmp_beta / LOG10_EXP * m_betaUpdate >> SHIFT_11;
    }
    else{
    	beta += tmp_beta * m_betaUpdate / LOG10_EXP >> SHIFT_11;
    }
#else
	if (tmp_alpha > 2622080 || tmp_alpha < -2622080) { /* (2^31 - 1) / max_alphaUpdate(819) */
		alpha += (tmp_alpha / LOG10_EXP * encRC->m_encRCSeq.m_alphaUpdate) >> SHIFT_11;
	}
	else {
		alpha += (tmp_alpha * encRC->m_encRCSeq.m_alphaUpdate / LOG10_EXP) >> SHIFT_11;
	}
	//alpha += encRC->m_encRCSeq->m_alphaUpdate * (lookupLogListTable(iinputLambda, inputLambda_multiply) - lookupLogListTable(icalLambda, calLambda_multiply)) * alpha / lookupLogListTable(INT_EXP, 1) >> SHIFT_11;

	if (picActualBpp > 214748) { /* (2^31 - 1) / TIMES_10 */
		lnbpp = (lookupLogListTable(ipicActualBpp, 0) << SHIFT_11) / LOG10_EXP;
	}
	else {
		lnbpp = (lookupLogListTable(ipicActualBpp, 1) << SHIFT_11) / LOG10_EXP;
	}

	rc_inf("lnbpp : %d\n", (int)(lnbpp));

	lnbpp = RC_CLIP3(-10240, -205, lnbpp);

	tmp_beta = (lookupLogListTable(iinputLambda, inputLambda_multiply) - lookupLogListTable(icalLambda, calLambda_multiply)) * lnbpp;

	rc_inf("tmp_beta : %d\n", (int)(tmp_beta));

	if (tmp_beta > 5237764 || tmp_beta < -5237764) { /* (2^31 - 1) / max_betaUpdate(410) */
		beta += (tmp_beta / LOG10_EXP * encRC->m_encRCSeq.m_betaUpdate) >> SHIFT_11;
	}
	else {
		beta += (tmp_beta * encRC->m_encRCSeq.m_betaUpdate / LOG10_EXP) >> SHIFT_11;
	}
#endif
	rc_inf("end - updateAfterPic - alpha : %d\tbeta : %d\n", (int)(alpha), (int)(beta));

	alpha = RC_CLIP3(g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha);
	beta = RC_CLIP3(g_RCBetaMinValue, g_RCBetaMaxValue, beta);


	rcPara.m_alpha = alpha;
	rcPara.m_beta = beta;
	encRC->m_encRCSeq.m_picPara[encRCPic->m_frameLevel] = rcPara;

	#if ALPHA_CONVERG_DIRECT
	calculateAlphaConvCnt(encRC, encRCPic, cur_alpha, rcPara.m_alpha);
	#endif

	if (1 == encRCPic->m_frameLevel) {
		int64_t currLambda = (int64_t)RC_CLIP3(204, 20480000, encRCPic->m_picLambda);
		int64_t tmp_1, tmp_2;
		int64_t updateLastLambda;

		tmp_1 = ((int64_t)encRC->m_encRCSeq.m_lastLambda * (int64_t)g_RCWeightHistoryLambda) >> SHIFT_11;
		tmp_2 = (currLambda * (int64_t)g_RCWeightCurrentLambda) >> SHIFT_11;

		updateLastLambda = tmp_1 + tmp_2;

		/* avoid integer overflowing */
		updateLastLambda = RC_CLIP3(0, (int64_t)MAX_INTEGER, updateLastLambda);

		encRC->m_encRCSeq.m_lastLambda = (int)updateLastLambda;
	}
	rc_inf("updateAfterPic - alpha : %d\tbeta : %d\n", (int)(alpha), (int)(beta));
}

static void updateAfterPic(TEncRCSeq *encRCSeq, int bits, int pred_size, int chn)
{
	encRCSeq->m_bitsleft -= bits;
	encRCSeq->m_seqframeleft--;

	encRCSeq->m_bitsDiff += encRCSeq->m_averageBits - bits;
#if EVALUATE_PRED_DIFF
	if (g_RCStartPredGOP >= 0 && RCGetGOPCnt(chn) >= g_RCStartPredGOP) {
		RCSetPredDiff(chn, RCGetPredDiff(chn)+pred_size - bits);
	}
#endif
}

static void TEncRCGOP_updateAfterPicture(TEncRCGOP *encRCGOP, int bitsCost, bool keyframe)
{
	encRCGOP->m_bitsLeft -= bitsCost;
	encRCGOP->m_picLeft--;
    rc_inf("bitsCost : %d\tGOP_picLeft : %d\tGOP_bitsLeft : %lld\n", (int)(bitsCost), (int)(encRCGOP->m_picLeft), (long long)(encRCGOP->m_bitsLeft));
}

static int updateGOPQP(TEncRateCtrl *encRC, unsigned int sliceQP, int keyframe)
{
    // only consider P frame
    if (keyframe) {
        uint32_t min_qp;
        if (H26X_RC_VBR == encRC->m_rcMode)
            min_qp = encRC->m_initPQp;
        else
            min_qp = encRC->m_minPQp;
        if (encRC->m_maxGOPQP == encRC->m_minGOPQP && (encRC->m_minGOPQP == min_qp || encRC->m_maxGOPQP == encRC->m_maxPQp)) {
            encRC->m_encRCSeq.m_bitsDiff = 0;
			#if EVALUATE_PRED_DIFF
				#if (PREDIFF_MAXQP_RESET_ADJUST==0)
				RCClearPredDiff(encRC->m_chn);
				#endif
			#endif
        }
        encRC->m_maxGOPQP = 0;
        encRC->m_minGOPQP = 52;
    }
    else {
        if (sliceQP > encRC->m_maxGOPQP)
            encRC->m_maxGOPQP = sliceQP;
        if (sliceQP < encRC->m_minGOPQP)
            encRC->m_minGOPQP = sliceQP;
    }
    return 0;
}

static void evaluate_frame_qp(TEncRateCtrl* encRC, unsigned int qp, int keyframe)
{
    updateGOPQP(encRC, qp, keyframe);
}

#if EVALUATE_BITRATE
static int evaluation_gop_bitrate(RCBitrate *pRCBr, unsigned frame_size, int keyframe, int chn)
{
    if (keyframe) {
        if (pRCBr->i_frame_cnt >= evaluate_period) {
            //rc_dump_info("gop bitrate\t%d\n", (int)(0x00/*pRCBr->total_byte*8*/));
            rc_dump_info("{chn%d} == H26x bitrate\t%d ==\r\n", (int)(chn), (int)(pRCBr->total_byte*8));
            pRCBr->frame_cnt = 0;
            pRCBr->total_byte = 0;
            pRCBr->i_frame_cnt = 0;
        }
        pRCBr->i_frame_cnt++;
    }
    pRCBr->total_byte += frame_size;
    pRCBr->frame_cnt++;
    return 0;
}
#endif

static unsigned int HMRateControlGetPredSize(TEncRateCtrl *encRC, bool keyframe)
{
    unsigned int pred_size = 0;

    pred_size = encRC->m_encRCPic.m_targetBits;
	if (encRC->m_max_frm_bit)
		pred_size = RC_MIN(encRC->m_encRCPic.m_targetBits, encRC->m_max_frm_bit);
#if EVBR_KEEP_PIC_QUALITY
	if (g_DisKeyFrameRRC && RCGetKeppIQualityEn(encRC->m_chn) && keyframe)
		pred_size = 0;
#endif

    return pred_size;
}

static unsigned int HMRateControlGetFrameLevel(TEncRateCtrl *encRC, RC_SLICE_TYPE ucPicType)
{
	int currPos = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
	int frame_level = RC_P_FRAME_LEVEL;
	// key frame
	if (RC_I_SLICE == ucPicType || RC_IDR_SLICE == ucPicType)
		frame_level = RC_I_FRAME_LEVEL;
	// SP
	else if (encRC->m_SP_sign)
		frame_level = RC_KEY_P_FRAME_LEVEL;
	else {
		// LTR
		if (encRC->m_ltrPeriod) {
			if (0 == (currPos%encRC->m_ltrPeriod)) {
				frame_level = RC_LT_P_FRAME_LEVEL;
				goto get_level;
			}
			currPos %= (int)encRC->m_ltrPeriod;
		}
		// SVC
		if (encRC->m_svcLayer) {
			if (0 == (currPos % 4))
				frame_level = RC_P2_FRAME_LEVEL;
			if (3 == (currPos % 4))
				frame_level = RC_P_FRAME_LEVEL;
			if (2 == (currPos % 4)) {
				if (encRC->m_svcLayer == 2)
					frame_level = RC_P3_FRAME_LEVEL;
				else
					frame_level = RC_P2_FRAME_LEVEL;
			}
		}
	}
get_level:
	return frame_level;
}

static int HMRateControlVBRPreLimit(TEncRateCtrl *encRC, bool keyframe)
{
    int limit = 1;
#if VBR_PRELIMIT_BITRATE
    if (H26X_RC_VBR == encRC->m_rcMode) {
        int min_qp;
        if (keyframe)
            min_qp = encRC->m_initIQp;
        else
            min_qp = encRC->m_initPQp;
        //encRC->ave_ROWQP;
        if (encRC->m_vbrPrelimitBRThd > 0) {
            if (RCBitrateOverflowCheck(&encRC->m_encRCBr, encRC->m_vbrPrelimitBRThd) || encRC->m_currQP >= min_qp)
                limit = 1;
            else
                limit = 0;
        }
        else {
            if (encRC->m_currQP >= min_qp)
                limit = 1;
            else
                limit = 0;
        }
    }
    else
        limit = 1;
#endif
    return limit;
}

#if EVBR_XA
static int EVBRStillUpdateQP(TEncRateCtrl *encRC, TEncRCEVBR *evbrRC,int keyframe)
{
	int deltaQP = 0;
	if (H26X_RC_EVBR == encRC->m_rcMode){
		#if EVBR_ADJUST_QP
		if (0 == g_EVBRAdjustQPMethod)
		#endif
		{
			int sp_period = encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod;
			if ((sp_period > 0 && encRC->m_lastKeyPsize > 0) || (sp_period == 0)){
				if (encRC->m_encRCEvbr.m_minstillPercent > 0 && encRC->m_lastIsize > 0 && evbrRC->m_encRCBr.recrod_num == (int)encRC->m_encRCEvbr.m_measurePeriod &&
					0 == RCEVBRBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrStillThd, encRC->m_GOPSize, (encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod), encRC->m_lastIsize, encRC->m_lastKeyPsize))
				{
					if (keyframe){
						encRC->m_encRCEvbr.m_tmp2_stillIQP--;
						encRC->m_encRCEvbr.m_tmp2_stillIQP = RC_CLIP3((int)encRC->m_minIQp, (int)encRC->m_maxIQp, (int)encRC->m_encRCEvbr.m_tmp2_stillIQP);
						deltaQP = 1;
					}
					else if (encRC->m_encRCEvbr.m_tmp2_stillIQP == evbrRC->uiMinStillIQp){
						encRC->m_encRCEvbr.m_tmp2_stillKeyPQP--;
						encRC->m_encRCEvbr.m_tmp2_stillKeyPQP = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)encRC->m_encRCEvbr.m_tmp2_stillKeyPQP);
						deltaQP = 2;
					}
				}
				else if (encRC->m_lastIsize > 0 && evbrRC->m_encRCBr.recrod_num == (int)encRC->m_encRCEvbr.m_measurePeriod && 0 == RCEVBRBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrStillupThd, encRC->m_GOPSize, (encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod), encRC->m_lastIsize, encRC->m_lastKeyPsize)){
					if (keyframe){
						if (encRC->m_encRCEvbr.m_tmp_stillIQP > evbrRC->m_stillIQP){
							encRC->m_encRCEvbr.m_tmp_stillIQP--;
							evbrRC->m_tmp_stillIQP = RC_CLIP3((int)evbrRC->m_stillIQP, (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPip / 10, (int)evbrRC->m_tmp_stillIQP);
							evbrRC->m_tmp_stillIQP = RC_CLIP3((int)1, (int)encRC->m_maxIQp, (int)evbrRC->m_tmp_stillIQP);
							deltaQP = -1;
						}
						if (encRC->m_encRCEvbr.m_tmp_stillKeyPQP > evbrRC->m_stillKeyPQP){
							encRC->m_encRCEvbr.m_tmp_stillKeyPQP--;
							evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)evbrRC->m_stillKeyPQP, (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPkp / 10, (int)evbrRC->m_tmp_stillKeyPQP);
							evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)1, (int)encRC->m_maxPQp, (int)evbrRC->m_tmp_stillKeyPQP);
							deltaQP = -1;
						}
					}
					else if (encRC->m_encRCEvbr.m_tmp_stillKeyPQP > evbrRC->m_stillKeyPQP){
						deltaQP = -1;
					}
				}
			}
		}
		#if EVBR_ADJUST_QP
		else {
			if (encRC->m_encRCEvbr.m_minstillPercent > 0 && encRC->m_lastIsize > 0 && evbrRC->m_encRCBr.recrod_num == (int)encRC->m_encRCEvbr.m_measurePeriod &&
				0 == RCEVBRBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrStillThd, encRC->m_GOPSize, (encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod), encRC->m_lastIsize, encRC->m_lastKeyPsize)) {
				//if (evbrRC->m_evbrStillPDQP != g_minDeltaStillPQP)
				//	DBG_DUMP("still dqp = %d\n", evbrRC->m_evbrStillPDQP-1);
				//evbrRC->m_evbrStillPDQP = RC_MAX(evbrRC->m_evbrStillPDQP-1, g_minDeltaStillPQP);
				RCDecStillPDQP(encRC->m_chn);
			}
		}
		#endif
	}
	return deltaQP;
}
#endif

#if SUPPORT_VBR2
static int VBR2UpdateQP(TEncRateCtrl *encRC, TEncRCVBR2 *encRCVbr2, int isI, int isSP)
{
	int fixQP = 0;
	int diffQP;
	int overflowLevel;
#if SUPPORT_VBR2_CVTE
	int currQP;
	int overflowThd;
	int overflowThd2;
	int underflowThd;
#endif
	if (H26X_RC_VBR2 == encRC->m_rcMode)
	{
		if (encRC->m_ltrPeriod > 0)
		{
			if (encRCVbr2->m_lastISize > 0 && encRCVbr2->m_lastLTPSize > 0 && (uint32_t)encRCVbr2->m_encRCBr.recrod_num == encRCVbr2->m_measurePeriod)
			{
				//overflowLevel = RCVBR2BitrateOverflowCheck(&encRCVbr2->m_encRCBr, encRCVbr2->m_rateOverflowThd, encRC->m_GOPSize, encRC->m_ltrPeriod,
				//	encRCVbr2->m_lastISize, encRCVbr2->m_lastLTPSize, isI, isSP);
				overflowLevel = encRCVbr2->m_overflowLevel;
				if (overflowLevel > (int)encRCVbr2->m_overflowThd2)
				{
					fixQP = 2;
				}
				else if (overflowLevel > (int)encRCVbr2->m_changePosThd)
				{
					fixQP = 1;
				}
				if (overflowLevel < (int)encRCVbr2->m_underflowThd2)
				{
					fixQP = -2;
				}
				else if (overflowLevel < (int)encRCVbr2->m_underflowThd)
				{
					fixQP = -1;
				}
			}
		}
		else if (encRC->m_keyPPeriod > 0)
		{
			if (encRCVbr2->m_lastISize > 0 && encRCVbr2->m_lastKPSize > 0 && (uint32_t)encRCVbr2->m_encRCBr.recrod_num == encRCVbr2->m_measurePeriod)
			{
				//overflowLevel = RCVBR2BitrateOverflowCheck(&encRCVbr2->m_encRCBr, encRCVbr2->m_rateOverflowThd, encRC->m_GOPSize, encRC->m_keyPPeriod,
				//	encRCVbr2->m_lastISize, encRCVbr2->m_lastKPSize, isI, isSP);
				overflowLevel = encRCVbr2->m_overflowLevel;
				if (overflowLevel > (int)encRCVbr2->m_overflowThd2)
				{
					fixQP = 2;
				}
				else if (overflowLevel > (int)encRCVbr2->m_changePosThd)
				{
					fixQP = 1;
				}
				if (overflowLevel < (int)encRCVbr2->m_underflowThd2)
				{
					fixQP = -2;
				}
				else if (overflowLevel < (int)encRCVbr2->m_underflowThd)
				{
					fixQP = -1;
				}
			}
		}
		else
		{
			if (encRCVbr2->m_lastISize > 0 && (uint32_t)encRCVbr2->m_encRCBr.recrod_num == encRCVbr2->m_measurePeriod)
			{
				overflowLevel = RCVBR2BitrateOverflowCheck(&encRCVbr2->m_encRCBr, encRCVbr2->m_rateOverflowThd, encRC->m_GOPSize, 0,
					encRCVbr2->m_lastISize, 0, isI, 0);
#if SUPPORT_VBR2_CVTE
				currQP = encRCVbr2->m_currPQP;
				if (encRC->m_svcLayer == 1)
				{
					currQP += encRCVbr2->m_currP2QP + 1;
					currQP >>= 1;
				}
				else if (encRC->m_svcLayer == 2)
				{
					currQP += encRCVbr2->m_currP2QP + encRCVbr2->m_currP3QP + encRCVbr2->m_currPQP + 2;
					currQP >>= 2;
				}
				currQP -= 30;
				currQP = RC_CLIP3(-2, 6, currQP);
				overflowThd = encRCVbr2->m_overflowThd + currQP * 5;
				overflowThd2 = encRCVbr2->m_overflowThd2 + currQP * 5;
				underflowThd = encRCVbr2->m_underflowThd + currQP * 5;
				overflowThd = RC_CLIP3(50, 100, overflowThd);
				overflowThd2 = RC_CLIP3(90, 110, overflowThd2);
				underflowThd = RC_CLIP3(30, 80, underflowThd);

				#if ADUST_VBR2_OVERFLOW_CHECK
					if (overflowLevel > overflowThd2)
					{
						fixQP = 1;
						encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
					}
					else if (overflowLevel > overflowThd)
					{
						if (encRCVbr2->m_changeFrmCnt >= 2*MAX_CHANGE_FRAME_NUM)
						{
							fixQP = 1;
							encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
						}
						else if (encRCVbr2->m_changeFrmCnt < MAX_CHANGE_FRAME_NUM)
						{
							encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
							encRCVbr2->m_changeFrmCnt++;
						}
						else
						{
							 encRCVbr2->m_changeFrmCnt++;
						}
					}
					else if (overflowLevel < underflowThd  && encRC->m_encRCGOP.m_bitsLeft > 100)
					{
						if ( (encRCVbr2->m_changeFrmCnt <= (MAX_CHANGE_FRAME_NUM - MAX_CHANGE_FRAME_NUM_UND)) || (encRCVbr2->m_currPQP > g_VBR2UndQPThd))
						{
							
							fixQP = -1;
							encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
						}
						else if (encRCVbr2->m_changeFrmCnt > MAX_CHANGE_FRAME_NUM)
						{
							encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
							encRCVbr2->m_changeFrmCnt--;			
						}
						else
						{
							encRCVbr2->m_changeFrmCnt--;
						}
					}
					else
					{
						encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
					}
					
					#if ADD_VBR2_THD_BITREC_LOG
					TEncRCPic_setVBRThdLog(encRC, overflowThd, overflowThd2, underflowThd);
					#endif
				#else
					if (overflowLevel > overflowThd2)
					{
						fixQP = 1;
						encRCVbr2->m_changeFrmCnt = 0;
					}
					else if (overflowLevel > overflowThd)
					{
						if (encRCVbr2->m_changeFrmCnt >= MAX_CHANGE_FRAME_NUM)
						{
							fixQP = 1;
							encRCVbr2->m_changeFrmCnt = 0;
						}
						else
						{
							encRCVbr2->m_changeFrmCnt++;
						}
					}
					else if (overflowLevel < underflowThd)
					{
						fixQP = -1;
						encRCVbr2->m_changeFrmCnt = 0;
					}
					else
					{
						encRCVbr2->m_changeFrmCnt = 0;
					}
				#endif
#else
				if (overflowLevel > (int)encRCVbr2->m_overflowThd2)
				{
					fixQP = 2;
				}
				else if (overflowLevel > (int)encRCVbr2->m_changePosThd)
				{
					fixQP = 1;
				}
				if (overflowLevel < (int)encRCVbr2->m_underflowThd2)
				{
					fixQP = -2;
				}
				else if (overflowLevel < (int)encRCVbr2->m_underflowThd)
				{
					fixQP = -1;
				}
#endif
			}
		}

		if (isI)
		{
			diffQP = RC_ABS((int)encRCVbr2->m_currPQP - (int)encRCVbr2->m_lastPQP);
			diffQP >>= 1;
			diffQP = diffQP > 3 ? 3 : diffQP;
			diffQP *= encRCVbr2->m_currPQP >= encRCVbr2->m_lastPQP ? 1 : -1;

			encRCVbr2->m_currIQP += diffQP;
			encRCVbr2->m_currKPQP += diffQP;
			encRCVbr2->m_currLTPQP += diffQP;
			encRCVbr2->m_currP2QP -= diffQP;
			encRCVbr2->m_currP3QP -= diffQP;
			encRCVbr2->m_currPQP -= diffQP;

			encRCVbr2->m_currIQP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaIQP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaIQP + 1, encRCVbr2->m_currIQP);
			encRCVbr2->m_currKPQP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaKPQP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaKPQP + 1, encRCVbr2->m_currKPQP);
			encRCVbr2->m_currLTPQP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaLTPQP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaLTPQP + 1, encRCVbr2->m_currLTPQP);
			encRCVbr2->m_currP2QP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaP2QP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaP2QP, encRCVbr2->m_currP2QP);
			encRCVbr2->m_currP3QP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaP3QP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaP3QP, encRCVbr2->m_currP3QP);
		}
		else if (isSP)
		{
			diffQP = RC_ABS((int)encRCVbr2->m_currPQP - (int)encRCVbr2->m_lastPQP);
			diffQP >>= 1;
			diffQP = diffQP > 3 ? 3 : diffQP;
			diffQP *= encRCVbr2->m_currPQP >= encRCVbr2->m_lastPQP ? 1 : -1;

			encRCVbr2->m_currIQP += diffQP;
			encRCVbr2->m_currKPQP += diffQP;
			encRCVbr2->m_currLTPQP += diffQP;
			encRCVbr2->m_currP2QP -= diffQP;
			encRCVbr2->m_currP3QP -= diffQP;
			encRCVbr2->m_currPQP -= diffQP;

			encRCVbr2->m_currIQP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaIQP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaIQP + 1, encRCVbr2->m_currIQP);
			encRCVbr2->m_currKPQP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaKPQP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaKPQP + 1, encRCVbr2->m_currKPQP);
			encRCVbr2->m_currLTPQP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaLTPQP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaLTPQP + 1, encRCVbr2->m_currLTPQP);
			encRCVbr2->m_currP2QP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaP2QP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaP2QP, encRCVbr2->m_currP2QP);
			encRCVbr2->m_currP3QP = RC_CLIP3(encRCVbr2->m_currPQP - encRCVbr2->m_deltaP3QP - 1, encRCVbr2->m_currPQP - encRCVbr2->m_deltaP3QP, encRCVbr2->m_currP3QP);
		}
	}
	return fixQP;
}

static int VBR2CalculateTargetBits(TEncRateCtrl *encRC, TEncRCVBR2 *encRCVbr2, int frameLevel)
{
	int64_t targetBits;
	int picLeft = encRC->m_encRCGOP.m_picLeft;
	int currRatio = encRCVbr2->m_bitRatio[frameLevel];
	int totalRatio = 0;
	totalRatio += encRC->m_GOP_SPnumleft * encRCVbr2->m_bitRatio[RC_KEY_P_FRAME_LEVEL];
	picLeft -= encRC->m_GOP_SPnumleft;
	totalRatio += encRC->m_GOP_LTnumleft * encRCVbr2->m_bitRatio[RC_LT_P_FRAME_LEVEL];
	picLeft -= encRC->m_GOP_LTnumleft;
	totalRatio += encRC->m_GOP_P2numleft * encRCVbr2->m_bitRatio[RC_P2_FRAME_LEVEL];
	picLeft -= encRC->m_GOP_P2numleft;
	totalRatio += encRC->m_GOP_P3numleft * encRCVbr2->m_bitRatio[RC_P3_FRAME_LEVEL];
	picLeft -= encRC->m_GOP_P3numleft;
	totalRatio += picLeft * encRCVbr2->m_bitRatio[RC_P_FRAME_LEVEL];
	targetBits = division(encRC->m_encRCGOP.m_bitsLeft * currRatio, totalRatio);
	targetBits = RC_CLIP3(100, MAX_INTEGER, targetBits);
	return (int)targetBits;
}

static int HMRateControlGetVBR2QP(TEncRateCtrl *encRC, H26XEncRCPreparePic *pPic, int currPos)
{
    int qp_fix;
    int qp;
	if (VBR2_VBR_STATE == encRC->m_encRCVbr2.m_vbr2State)
	{
		if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType)
		{
			encRC->m_encRCPic.m_frameLevel = RC_I_FRAME_LEVEL;
		#if FIX_BUG_ALL_INTRA
			if (encRC->m_GOPSize > 1) {
		#endif
				qp_fix = VBR2UpdateQP(encRC, &encRC->m_encRCVbr2, 1, 0);
				encRC->m_encRCVbr2.m_currIQP += qp_fix;
				encRC->m_encRCVbr2.m_currKPQP += qp_fix;
				encRC->m_encRCVbr2.m_currLTPQP += qp_fix;
				encRC->m_encRCVbr2.m_currIQP = RC_CLIP3(encRC->m_encRCVbr2.m_initIQP, encRC->m_encRCVbr2.m_maxIQP, encRC->m_encRCVbr2.m_currIQP);
				encRC->m_encRCVbr2.m_currPQP = RC_CLIP3(encRC->m_encRCVbr2.m_initPQP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currPQP);
				encRC->m_encRCVbr2.m_currKPQP = RC_CLIP3(encRC->m_encRCVbr2.m_initKPQP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currKPQP);
				encRC->m_encRCVbr2.m_currLTPQP = RC_CLIP3(encRC->m_encRCVbr2.m_initLTPQP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currLTPQP);
				encRC->m_encRCVbr2.m_currP2QP = RC_CLIP3(encRC->m_encRCVbr2.m_initP2QP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currP2QP);
				encRC->m_encRCVbr2.m_currP3QP = RC_CLIP3(encRC->m_encRCVbr2.m_initP3QP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currP3QP);
		#if FIX_BUG_ALL_INTRA
			}
		#endif
			qp = encRC->m_encRCVbr2.m_currIQP;
			encRC->m_encRCVbr2.m_lastPQP = encRC->m_encRCVbr2.m_currPQP;
			encRC->m_encRCVbr2.m_targetBits = 0;
			pPic->uiMotionAQStr = 0;
		}
		else if (encRC->m_LT_sign > 0 || encRC->m_SP_sign > 0)
		{
			encRC->m_encRCPic.m_frameLevel = encRC->m_LT_sign > 0 ? RC_LT_P_FRAME_LEVEL : RC_KEY_P_FRAME_LEVEL;
			qp_fix = VBR2UpdateQP(encRC, &encRC->m_encRCVbr2, 0, 1);
			encRC->m_encRCVbr2.m_currIQP += qp_fix;
			encRC->m_encRCVbr2.m_currKPQP += qp_fix;
			encRC->m_encRCVbr2.m_currLTPQP += qp_fix;
			encRC->m_encRCVbr2.m_currIQP = RC_CLIP3(encRC->m_encRCVbr2.m_initIQP, encRC->m_encRCVbr2.m_maxIQP, encRC->m_encRCVbr2.m_currIQP);
			encRC->m_encRCVbr2.m_currPQP = RC_CLIP3(encRC->m_encRCVbr2.m_initPQP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currPQP);
			encRC->m_encRCVbr2.m_currKPQP = RC_CLIP3(encRC->m_encRCVbr2.m_initKPQP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currKPQP);
			encRC->m_encRCVbr2.m_currLTPQP = RC_CLIP3(encRC->m_encRCVbr2.m_initLTPQP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currLTPQP);
			encRC->m_encRCVbr2.m_currP2QP = RC_CLIP3(encRC->m_encRCVbr2.m_initP2QP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currP2QP);
			encRC->m_encRCVbr2.m_currP3QP = RC_CLIP3(encRC->m_encRCVbr2.m_initP3QP, encRC->m_encRCVbr2.m_maxPQP, encRC->m_encRCVbr2.m_currP3QP);

			qp = encRC->m_LT_sign > 0 ? encRC->m_encRCVbr2.m_currLTPQP : encRC->m_encRCVbr2.m_currKPQP;
			encRC->m_encRCVbr2.m_lastPQP = encRC->m_encRCVbr2.m_currPQP;
			encRC->m_encRCVbr2.m_targetBits = VBR2CalculateTargetBits(encRC, &encRC->m_encRCVbr2, encRC->m_encRCPic.m_frameLevel);
			pPic->uiMotionAQStr = 0;
		}
		else
		{
			encRC->m_encRCPic.m_frameLevel = RC_P_FRAME_LEVEL;
			qp = encRC->m_encRCVbr2.m_currPQP;
			pPic->uiMotionAQStr = encRC->m_motionAQStrength;
			if (encRC->m_svcLayer > 0)
			{
				if (0 == currPos % 4)
				{
					encRC->m_encRCPic.m_frameLevel = RC_P2_FRAME_LEVEL;
					qp = encRC->m_encRCVbr2.m_currP2QP;
					pPic->uiMotionAQStr = 0;
				}
				else if (2 == currPos % 4)
				{
					if (encRC->m_svcLayer == 2)
					{
						encRC->m_encRCPic.m_frameLevel = RC_P3_FRAME_LEVEL;
						qp = encRC->m_encRCVbr2.m_currP3QP;
					}
					else
					{
						encRC->m_encRCPic.m_frameLevel = RC_P2_FRAME_LEVEL;
						qp = encRC->m_encRCVbr2.m_currP2QP;
						pPic->uiMotionAQStr = 0;
					}
				}
			}
			encRC->m_encRCVbr2.m_targetBits = VBR2CalculateTargetBits(encRC, &encRC->m_encRCVbr2, encRC->m_encRCPic.m_frameLevel);
		}
		encRC->m_encRCPic.m_targetBits = encRC->m_encRCVbr2.m_targetBits;
		pPic->uiPredSize = 0;
#if SUPPORT_VBR2_CVTE
		if ((encRC->m_encRCVbr2.m_overflowLevel > encRC->m_encRCVbr2.m_overflowThd2 + 10) &&
#else
		if ((encRC->m_encRCVbr2.m_overflowLevel > encRC->m_encRCVbr2.m_overflowThd2) &&
#endif
			((encRC->m_encRCPic.m_frameLevel == RC_I_FRAME_LEVEL && (uint32_t)qp < encRC->m_encRCVbr2.m_maxIQP) ||
			 (encRC->m_encRCPic.m_frameLevel > RC_I_FRAME_LEVEL && (uint32_t)qp < encRC->m_encRCVbr2.m_maxPQP)))
		{
			pPic->uiPredSize = encRC->m_encRCVbr2.m_targetBits;
		}
	}
	else
	{
		if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType)
		{
			encRC->m_encRCVbr2.m_currIQP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initIQP, (int)encRC->m_encRCVbr2.m_maxIQP, encRC->m_currQP);
			qp = encRC->m_encRCVbr2.m_currIQP;
			encRC->m_encRCVbr2.m_lastPQP = qp;
			pPic->uiPredSize = HMRateControlGetPredSize(encRC, 1);
		}
		else if (encRC->m_SP_sign > 0)
		{
			encRC->m_encRCVbr2.m_currKPQP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initKPQP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
			qp = encRC->m_encRCVbr2.m_currKPQP;
			pPic->uiPredSize = HMRateControlGetPredSize(encRC, 0);
		}
		else if (encRC->m_LT_sign > 0)
		{
			encRC->m_encRCVbr2.m_currLTPQP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initLTPQP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
			qp = encRC->m_encRCVbr2.m_currLTPQP;
			pPic->uiPredSize = HMRateControlGetPredSize(encRC, 0);
		}
		else
		{
			if (encRC->m_svcLayer > 0)
			{
				if (0 == currPos % 4)
				{
					encRC->m_encRCVbr2.m_currP2QP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initP2QP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
					qp = encRC->m_encRCVbr2.m_currP2QP;
				}
				else if (2 == currPos % 4)
				{
					if (encRC->m_svcLayer == 2)
					{
						encRC->m_encRCVbr2.m_currP3QP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initP3QP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
						qp = encRC->m_encRCVbr2.m_currP3QP;
					}
					else
					{
						encRC->m_encRCVbr2.m_currP2QP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initP2QP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
						qp = encRC->m_encRCVbr2.m_currP2QP;
					}
				}
				else
				{
					encRC->m_encRCVbr2.m_currPQP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initPQP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
					qp = encRC->m_encRCVbr2.m_currPQP;
				}
			}
			else
			{
				encRC->m_encRCVbr2.m_currPQP = RC_CLIP3((int)encRC->m_encRCVbr2.m_initPQP, (int)encRC->m_encRCVbr2.m_maxPQP, encRC->m_currQP);
				qp = encRC->m_encRCVbr2.m_currPQP;
			}
			pPic->uiPredSize = HMRateControlGetPredSize(encRC, 0);
		}
		encRC->m_encRCVbr2.m_targetBits = pPic->uiPredSize;
	}
    return qp;
}
#endif

/* fix BUG : update lambda and QP */
static void TEncRateCtrl_update(TEncRateCtrl *encRC, int frame_size, unsigned int avg_qp, unsigned char slice_type)
{
	int actualHeaderBits;
	int actualTotalBits = frame_size << 3;
	int sliceQP = encRC->m_currQP;
	int lambda = encRC->m_currlambda;
    int keyframe = (RC_I_SLICE == slice_type || RC_IDR_SLICE == slice_type) ? 1 : 0;

    actualHeaderBits = (1 == keyframe) ? 0 : 64;

#if RC_GOP_QP_LIMITATION
	if (encRC->m_useGopQpLimitation)
	{
		if (keyframe)
		{
			encRC->m_GopQpLimit = 0;
			encRC->m_CurrPNum = 0;
			encRC->m_GopMaxQp = 51;
			encRC->m_QpBeforeClip = 26;
			encRC->m_GopMaxQpFromTbl = 51;
			encRC->m_GopMaxQpAdd = 0;
			encRC->m_GopBitsUnderTBR = encRC->m_encRCGOP.m_targetBits;
			encRC->m_GopBitsEst = 0;
			encRC->m_LastIBits = actualTotalBits;
			encRC->m_PBitsSum = 0;
			encRC->m_AvgPBits = 0;
			encRC->m_GopOverflowLevel = 0;
		}
		else
		{
			if (encRC->m_encRCPic.m_frameLevel == RC_P_FRAME_LEVEL)
			{
				int tmp;
				encRC->m_QpMinus24 = RC_CLIP3(0, 27, sliceQP - 24);
				tmp = RC_MIN(51, 24 + encRC->m_QpMinus24 + g_GopMaxQpTbl[encRC->m_QpMinus24] + g_GopMaxQpBalanceTbl[encRC->m_BrTolerance]);
				encRC->m_GopMaxQpFromTbl = RC_MIN(encRC->m_GopMaxQpFromTbl, tmp);
				encRC->m_PBitsSum += actualTotalBits;
				encRC->m_CurrPNum++;
			}
			else if (encRC->m_encRCPic.m_frameLevel == RC_P2_FRAME_LEVEL || encRC->m_encRCPic.m_frameLevel == RC_P3_FRAME_LEVEL)
			{
				encRC->m_PBitsSum += actualTotalBits;
				encRC->m_CurrPNum++;
			}
			else if (encRC->m_encRCPic.m_frameLevel == RC_KEY_P_FRAME_LEVEL)
			{
				encRC->m_LastKPBits = actualTotalBits;
			}
			else if (encRC->m_encRCPic.m_frameLevel == RC_LT_P_FRAME_LEVEL)
			{
				encRC->m_LastLTBits = actualTotalBits;
			}

			if (encRC->m_CurrPNum > 25 &&
				((encRC->m_keyPPeriod == 0 || (encRC->m_keyPPeriod > 0 && encRC->m_LastKPBits > 0)) &&
				(encRC->m_ltrPeriod == 0 || (encRC->m_ltrPeriod > 0 && encRC->m_LastLTBits > 0))))
			{
				int overflowLevelThd1 = g_GopOverflowThd1Tbl[encRC->m_BrTolerance];
				int overflowLevelThd2 = g_GopOverflowThd2Tbl[encRC->m_BrTolerance];
				int overflowLevelThd3 = g_GopOverflowThd3Tbl[encRC->m_BrTolerance];
				if (encRC->m_svcLayer > 0)
				{
					if (encRC->m_encRCPic.m_frameLevel == RC_P2_FRAME_LEVEL)
					{
						int64_t tmp1, tmp2;
						encRC->m_AvgPBits = (int)division(encRC->m_PBitsSum, encRC->m_CurrPNum);
						encRC->m_GopBitsEst = (int64_t)encRC->m_LastIBits + (int64_t)encRC->m_AvgPBits * (int64_t)(encRC->m_encRCSeq.m_GOPSize - 1 - encRC->m_SP_frm_num - encRC->m_LT_frm_num);
						if (encRC->m_keyPPeriod > 0)
						{
							encRC->m_GopBitsEst += ((int64_t)encRC->m_LastKPBits) * encRC->m_SP_frm_num;
						}
						if (encRC->m_ltrPeriod > 0)
						{
							encRC->m_GopBitsEst += ((int64_t)encRC->m_LastLTBits) * encRC->m_LT_frm_num;
						}

						tmp1 = encRC->m_GopBitsEst;
						tmp2 = encRC->m_GopBitsUnderTBR;
						if (tmp1 > tmp2)
						{
							while (tmp2 > MAX_INTEGER)
							{
								tmp1 >>= 1;
								tmp2 >>= 1;
							}
							tmp1 *= 100;
							encRC->m_GopOverflowLevel = (int)division(tmp1, (int)tmp2);
							if (encRC->m_GopOverflowLevel > overflowLevelThd3)
							{
								encRC->m_GopMaxQpAdd++;
								encRC->m_GopMaxQpAdd = RC_MIN(encRC->m_GopMaxQpAdd, 51 - encRC->m_GopMaxQpFromTbl);
							}
							else if (encRC->m_GopOverflowLevel > overflowLevelThd2)
							{
								if (encRC->m_GopMaxQpAdd > 2)
								{
									encRC->m_GopMaxQpAdd--;
									encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 2);
								}
								else if (encRC->m_GopMaxQpAdd < 2)
								{
									encRC->m_GopMaxQpAdd++;
									encRC->m_GopMaxQpAdd = RC_MIN(encRC->m_GopMaxQpAdd, 2);
								}
							}
							else if (encRC->m_GopOverflowLevel > overflowLevelThd1)
							{
								if (encRC->m_GopMaxQpAdd > 1)
								{
									encRC->m_GopMaxQpAdd--;
									encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 1);
								}
								else if (encRC->m_GopMaxQpAdd < 1)
								{
									encRC->m_GopMaxQpAdd++;
									encRC->m_GopMaxQpAdd = RC_MIN(encRC->m_GopMaxQpAdd, 1);
								}
							}
							else
							{
								if (encRC->m_GopMaxQpAdd > 0)
								{
									encRC->m_GopMaxQpAdd--;
									encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 0);
								}
							}
						}
						else
						{
							if (encRC->m_GopMaxQpAdd > 0)
							{
								encRC->m_GopMaxQpAdd--;
								encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 0);
							}
						}
					}
				}
				else
				{
					int64_t tmp1, tmp2;
					encRC->m_AvgPBits = (int)division(encRC->m_PBitsSum, encRC->m_CurrPNum);
					encRC->m_GopBitsEst = (int64_t)encRC->m_LastIBits + (int64_t)encRC->m_AvgPBits * (int64_t)(encRC->m_encRCSeq.m_GOPSize - 1 - encRC->m_SP_frm_num - encRC->m_LT_frm_num);
					if (encRC->m_keyPPeriod > 0)
					{
						encRC->m_GopBitsEst += ((int64_t)encRC->m_LastKPBits) * encRC->m_SP_frm_num;
					}
					if (encRC->m_ltrPeriod > 0)
					{
						encRC->m_GopBitsEst += ((int64_t)encRC->m_LastLTBits) * encRC->m_LT_frm_num;
					}

					tmp1 = encRC->m_GopBitsEst;
					tmp2 = encRC->m_GopBitsUnderTBR;
					if (tmp1 > tmp2)
					{
						while (tmp2 > MAX_INTEGER)
						{
							tmp1 >>= 1;
							tmp2 >>= 1;
						}
						tmp1 *= 100;
						encRC->m_GopOverflowLevel = (int)division(tmp1, (int)tmp2);
						if (encRC->m_GopOverflowLevel > overflowLevelThd3)
						{
							encRC->m_GopMaxQpAdd++;
							encRC->m_GopMaxQpAdd = RC_MIN(encRC->m_GopMaxQpAdd, 51 - encRC->m_GopMaxQpFromTbl);
						}
						else if (encRC->m_GopOverflowLevel > overflowLevelThd2)
						{
							if (encRC->m_GopMaxQpAdd > 2)
							{
								encRC->m_GopMaxQpAdd--;
								encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 2);
							}
							else if (encRC->m_GopMaxQpAdd < 2)
							{
								encRC->m_GopMaxQpAdd++;
								encRC->m_GopMaxQpAdd = RC_MIN(encRC->m_GopMaxQpAdd, 2);
							}
						}
						else if (encRC->m_GopOverflowLevel > overflowLevelThd1)
						{
							if (encRC->m_GopMaxQpAdd > 1)
							{
								encRC->m_GopMaxQpAdd--;
								encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 1);
							}
							else if (encRC->m_GopMaxQpAdd < 1)
							{
								encRC->m_GopMaxQpAdd++;
								encRC->m_GopMaxQpAdd = RC_MIN(encRC->m_GopMaxQpAdd, 1);
							}
						}
						else
						{
							if (encRC->m_GopMaxQpAdd > 0)
							{
								encRC->m_GopMaxQpAdd--;
								encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 0);
							}
						}
					}
					else
					{
						if (encRC->m_GopMaxQpAdd > 0)
						{
							encRC->m_GopMaxQpAdd--;
							encRC->m_GopMaxQpAdd = RC_MAX(encRC->m_GopMaxQpAdd, 0);
						}
					}
				}
			}
			encRC->m_GopMaxQpFromBpp = encRC->m_encRCSeq.m_seqTargetBpp > 10 ? 2 : encRC->m_encRCSeq.m_seqTargetBpp > 5 ? 1 : 0;
			encRC->m_GopMaxQp = encRC->m_GopMaxQpFromTbl + encRC->m_GopMaxQpAdd - encRC->m_GopMaxQpFromBpp;
			encRC->m_GopMaxQp = RC_MIN(encRC->m_GopMaxQp, (int)encRC->m_maxPQp);
			encRC->m_GopQpLimit = 1;
		}
	}
#endif

#if LOOKUP_TABLE_METHOD
	//encRC->ave_ROWQP = (qp_sum + encRC->m_encRCSeq.m_mbCount / 2) / encRC->m_encRCSeq.m_mbCount;
	encRC->ave_ROWQP = avg_qp;
#else
	encRC->ave_ROWQP = encRC->m_currQP;
#endif

#if BITRATE_OVERFLOW_CHECK
    RCBitRecordUpdate(&encRC->m_encRCBr, actualTotalBits, encRC->m_GOPSize);
#endif

	TEncRCPic_updateAfterPicture(encRC,
		&encRC->m_encRCPic,
		actualHeaderBits,
		actualTotalBits,
		sliceQP,
		lambda);

	updateAfterPic(&encRC->m_encRCSeq, actualTotalBits, encRC->m_encRCPic.m_targetBits, encRC->m_chn);
	#if ADD_PREDIFF_LOG
		TEncRCPic_setPredDiffLog(encRC, g_predDiff[encRC->m_chn]);
	#endif

#if 0
	if (!keyframe) { /* slice_type == P_SLICE */
		TEncRCGOP_updateAfterPicture(&encRC->m_encRCGOP, actualTotalBits, keyframe);
	}
	else { /* slice_type == RC_I_SLICE */
		TEncRCGOP_updateAfterPicture(&encRC->m_encRCGOP, encRC->m_encRCPic.m_targetBits, keyframe);
	}
#else /* Adjust method, by XA 20171117 */
	if (!keyframe){
		TEncRCGOP_updateAfterPicture(&encRC->m_encRCGOP, actualTotalBits, keyframe);
	}
	else{
		if (actualTotalBits > encRC->m_encRCGOP.m_bitsLeft || actualTotalBits == encRC->m_encRCGOP.m_bitsLeft)
			TEncRCGOP_updateAfterPicture(&encRC->m_encRCGOP, encRC->m_encRCPic.m_targetBits, keyframe);
		else
			TEncRCGOP_updateAfterPicture(&encRC->m_encRCGOP, actualTotalBits, keyframe);
		#if EVALUATE_PRED_DIFF
		RCIncGOPCnt(encRC->m_chn);
		#endif
	}
#endif
}

static int TEncRateCtrl_changeStillState(TEncRateCtrl *encRC, TEncRCEVBR *evbrRC, H26XEncRCUpdatePic *pUPic)
{
    pUPic->uiUpdate = 1;
    pUPic->uiRowRCEnable = 0;
    pUPic->iMotionAQStr = encRC->m_motionAQStrength;
    evbrRC->m_evbrState = EVBR_STILL_STATE;
    RCBitrateCheckInit(&evbrRC->m_encRCBr, 0);
#if EVBR_MEASURE_BR_METHOD
    encRC->m_lastIsize = 0;
    encRC->m_lastKeyPsize = 0;
#endif
#if EVBR_MODE_SWITCH_NEW_METHOD
    encRC->m_still2motion_I = 0;
    encRC->m_still2motion_KP = 0;
#endif
    rc_dbg("still state\r\n");
    return 0;
}
static int TEncRateCtrl_checkStill(TEncRateCtrl *encRC, TEncRCEVBR *evbrRC, H26XEncRCUpdatePic *pUPic)
{
    int currPicPosition = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
    int isKPFrame = 0;
#if EVBR_XA
    int still_ave_qp;

	if (encRC->m_currQP > (int)encRC->m_maxPQp){
		pUPic->uiUpdate = 1;
		pUPic->uiRowRCEnable = 1;
		pUPic->iMotionAQStr = 0;
	}
	else if (encRC->ave_ROWQP < (int)(encRC->m_maxPQp - encRC->m_motionAQStrength)){
		pUPic->uiUpdate = 1;
		pUPic->uiRowRCEnable = 0;
		pUPic->iMotionAQStr = encRC->m_motionAQStrength;
	}
#endif

    if (encRC->m_keyPPeriod > 0 && currPicPosition%encRC->m_keyPPeriod == 0)
        isKPFrame = 1;
    else
        isKPFrame = 0;
#if EVBR_CHECK_QP_UNDER_STILL
    #if EVBR_XA
    if((encRC->m_svcLayer==1 && (currPicPosition+1)%2!=0)|| (encRC->m_svcLayer==2 && (currPicPosition+1)%4!=0) || encRC->m_svcLayer==0){
    evbrRC->m_motionQPSum += encRC->ave_ROWQP;
    evbrRC->m_motionFrameCnt ++;
    }
    #else
    evbrRC->m_motionQPSum += encRC->ave_ROWQP;
    evbrRC->m_motionFrameCnt ++;
    #endif
#endif

    //rc_dump_info("motion ratio = %d, thd = %d (still cnt = %d)\r\n", (int)(pUPic->uiMotionRatio), (int)(evbrRC->m_motionRatioThd), (int)(evbrRC->m_stillFrameCnt));
    if (pUPic->uiMotionRatio < evbrRC->m_motionRatioThd)
        evbrRC->m_stillFrameCnt++;
    else
        evbrRC->m_stillFrameCnt = 0;

    #if EVBR_XA
	still_ave_qp = evbrRC->m_stillPQP;
	if (evbrRC->m_motionFrameCnt > (encRC->m_GOPSize / 2))
	     still_ave_qp = (evbrRC->m_motionQPSum + evbrRC->m_motionFrameCnt - 1) / evbrRC->m_motionFrameCnt;
    #endif

    rc_dbg("{chn%d} motionratio:%u, stillnum:%u, motionqp:%u, motionnum:%u\n\r", encRC->m_chn, (unsigned int)pUPic->uiMotionRatio,
        (unsigned int)evbrRC->m_stillFrameCnt, (unsigned int)evbrRC->m_motionQPSum, (unsigned int)evbrRC->m_motionFrameCnt);

    if (evbrRC->m_stillFrameCnt > evbrRC->m_stillFrameThd) {
    #if EVBR_STILL_START_FROM_KEY
        if (evbrRC->m_triggerStillMode) {
            //int currPicPosition = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
            //if (encRC->m_keyPPeriod > 0 && currPicPosition%encRC->m_keyPPeriod == 0) {
            if (isKPFrame) {
                #if EVBR_CHECK_QP_UNDER_STILL
                uint32_t avg_qp = 0;
                if (evbrRC->m_motionFrameCnt > 0)
                    avg_qp = (evbrRC->m_motionQPSum+evbrRC->m_motionFrameCnt-1)/evbrRC->m_motionFrameCnt;
                #if EVBR_XA
				if (avg_qp < evbrRC->m_stillPQP || (avg_qp == encRC->m_minPQp && evbrRC->m_stillPQP == encRC->m_minPQp))
                #else
                if (avg_qp < evbrRC->m_stillPQP)
                #endif
                #endif
                {
                    TEncRateCtrl_changeStillState(encRC, evbrRC, pUPic);
                    evbrRC->m_triggerStillMode = 0;
                }
            }
            #if EVBR_XA
			else if (0 == encRC->m_keyPPeriod && (still_ave_qp < (int)evbrRC->m_stillPQP || (still_ave_qp == (int)encRC->m_minPQp && evbrRC->m_stillPQP == encRC->m_minPQp)))
            #else
            else if (0 == encRC->m_keyPPeriod)
            #endif
            {
                TEncRateCtrl_changeStillState(encRC, evbrRC, pUPic);
                evbrRC->m_triggerStillMode = 0;
            }
        }
        else
            evbrRC->m_triggerStillMode = 1;
    #else
        TEncRateCtrl_changeStillState(encRC, evbrRC, pUPic);
    #endif
    }
#if EVBR_CHECK_QP_UNDER_STILL
    if (isKPFrame) {
        evbrRC->m_motionQPSum = 0;
        evbrRC->m_motionFrameCnt = 0;
    }
    #if EVBR_XA
	else if (0 == encRC->m_keyPPeriod && (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType)){
		evbrRC->m_motionQPSum = 0;
		evbrRC->m_motionFrameCnt = 0;
	}
    #endif
#endif
    return 0;
}
static void TEncRateCtrl_evbrUpdate(TEncRateCtrl *encRC, TEncRCEVBR *evbrRC, H26XEncRCUpdatePic *pUPic)
{
    // 1. update bitrate
#if BITRATE_OVERFLOW_CHECK
    #if EVBR_MEASURE_BR_METHOD
        #if HANDLE_DIFF_LTR_KEYP_PERIOD
        if (RC_I_SLICE != pUPic->ucPicType && RC_IDR_SLICE != pUPic->ucPicType) {
            if (encRC->m_ltrPeriod > 0) {
                if (0 == encRC->m_ltrFrame)
                    RCBitRecordUpdate(&evbrRC->m_encRCBr, (pUPic->uiFrameSize << 3), evbrRC->m_measurePeriod);
            }
            else {
                if (0 == encRC->m_SP_sign)
                    RCBitRecordUpdate(&evbrRC->m_encRCBr, (pUPic->uiFrameSize << 3), evbrRC->m_measurePeriod);
            }
        }
        #else   // HANDLE_DIFF_LTR_KEYP_PERIOD
        if (encRC->m_keyPPeriod > 0 && encRC->m_keyPPeriod < MAX_GOP_SIZE) {
    		if (RC_I_SLICE != pUPic->ucPicType && RC_IDR_SLICE != pUPic->ucPicType && 0 == encRC->m_SP_sign)
    			RCBitRecordUpdate(&evbrRC->m_encRCBr, (pUPic->uiFrameSize << 3), evbrRC->m_measurePeriod);
        }
        else
            RCBitRecordUpdate(&evbrRC->m_encRCBr, (pUPic->uiFrameSize << 3), evbrRC->m_measurePeriod);
        #endif  // HANDLE_DIFF_LTR_KEYP_PERIOD
    #else   // EVBR_MEASURE_BR_METHOD
        if (RC_I_SLICE != pUPic->ucPicType && RC_IDR_SLICE != pUPic->ucPicType)
            RCBitRecordUpdate(&evbrRC->m_encRCBr, (pUPic->uiFrameSize<<3), evbrRC->m_measurePeriod);
    #endif  // EVBR_MEASURE_BR_METHOD
#endif
#if EVBR_ADJUST_QP
	if (g_EVBRAdjustQPMethod) {
		int br_ratio = RCEVBRBitrateRatio(&evbrRC->m_encRCBr, evbrRC->m_evbrOverflowThd,
			encRC->m_GOPSize, (encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod), encRC->m_lastIsize, encRC->m_lastKeyPsize);
		if (RC_EVBR_LIMIT_LOW == RCGetEvbrPreLimitstate(encRC->m_chn)) {
			if (br_ratio > g_EVBRPreLimitMPct) {
				RCSetEvbrPreLimitstate(encRC->m_chn, RC_EVBR_LIMIT_MIDDLE);
				RCIncStillPDQP(encRC->m_chn);
			}
			else if (br_ratio > g_MinStillPercent+10) {
				RCIncStillPDQP(encRC->m_chn);			}
		}
		else if (RC_EVBR_LIMIT_MIDDLE == RCGetEvbrPreLimitstate(encRC->m_chn)) {
			if (br_ratio < g_EVBRPreLimitMPct) {
				RCSetEvbrPreLimitstate(encRC->m_chn, RC_EVBR_LIMIT_LOW);
			}
			else if (br_ratio > g_EVBRPreLimitHPct) {
				RCSetEvbrPreLimitstate(encRC->m_chn, RC_EVBR_LIMIT_HIGH);
				RCIncStillPDQP(encRC->m_chn);
			}
		}
		else if (RC_EVBR_LIMIT_HIGH == RCGetEvbrPreLimitstate(encRC->m_chn)) {
			if (br_ratio < g_EVBRPreLimitHPct) {
				RCSetEvbrPreLimitstate(encRC->m_chn, RC_EVBR_LIMIT_MIDDLE);
			}
			RCIncStillPDQP(encRC->m_chn);
		}
	}
#endif
    // 2. check overflow
	//rc_dump_info("motion ratio = %d, cur %lld, thd %lld\r\n", (int)(pUPic->uiMotionRatio), (long long)(evbrRC->m_encRCBr.bitrate_record), (long long)(evbrRC->m_evbrOverflowThd));
#if EVBR_MEASURE_BR_METHOD
    #if HANDLE_DIFF_LTR_KEYP_PERIOD
    #if EVBR_XA
	evbrRC->m_stillOverflow = RCEVBRBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrOverflowThd,
		encRC->m_GOPSize, (encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod), encRC->m_lastIsize, encRC->m_lastKeyPsize);
	if ((g_EVBRAdjustQPMethod && evbrRC->m_stillOverflow>0) ||
		(0 == g_EVBRAdjustQPMethod && evbrRC->m_stillOverflow>0 && ((int)(evbrRC->m_stillPQP - evbrRC->m_tmp_stillIQP) == (encRC->m_rc_deltaQPip / 10)) && evbrRC->m_stillIadd == 0))
    #else   // EVBR_XA
    if (RCEVBRBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrOverflowThd,
        encRC->m_GOPSize, (encRC->m_ltrPeriod > 0 ? encRC->m_ltrPeriod : encRC->m_keyPPeriod), encRC->m_lastIsize, encRC->m_lastKeyPsize))
    #endif  // EVBR_XA
    #else   // HANDLE_DIFF_LTR_KEYP_PERIOD
	if (RCEVBRBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrOverflowThd,
		encRC->m_GOPSize, encRC->m_keyPPeriod, encRC->m_lastIsize, encRC->m_lastKeyPsize))
    #endif  // HANDLE_DIFF_LTR_KEYP_PERIOD
#else
    if (RCBitrateOverflowCheck(&evbrRC->m_encRCBr, evbrRC->m_evbrOverflowThd))
#endif
	{
        // motion enable
        evbrRC->m_evbrState = EVBR_MOTION_STATE;
        evbrRC->m_stillFrameCnt = 0;
		#if EVALUATE_PRED_DIFF
		RCClearPredDiff(encRC->m_chn);
		#endif
        #if EVBR_XA
		if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType) {
			initRCGOP(encRC, encRC->m_GOPSize);
		}
        #endif
        TEncRateCtrl_update(encRC, pUPic->uiFrameSize, pUPic->uiAvgQP, pUPic->ucPicType);
        pUPic->uiUpdate = 1;
        #if EVBR_XA
		pUPic->uiRowRCEnable = 0;
		pUPic->iMotionAQStr = encRC->m_motionAQStrength;
        #else
        pUPic->uiRowRCEnable = 1;
        pUPic->iMotionAQStr = 0;
        #endif
    	#if EVBR_MODE_SWITCH_NEW_METHOD
		encRC->m_still2motion_KP = 1;
		encRC->m_still2motion_I = 1;
	    #endif
        #if EVBR_XA
		evbrRC->m_tmp_stillIQP = evbrRC->m_stillIQP;
		evbrRC->m_tmp_stillKeyPQP = evbrRC->m_stillKeyPQP;
		evbrRC->m_tmp2_stillIQP = evbrRC->m_stillIQP;
		evbrRC->m_tmp2_stillKeyPQP = evbrRC->m_stillKeyPQP;
		evbrRC->m_last_stillIQP = evbrRC->m_stillIQP;
		evbrRC->m_last_stillKeyPQP = evbrRC->m_stillKeyPQP;
		evbrRC->m_last_stillPQP = evbrRC->m_stillPQP;
        #endif
		#if EVBR_ADJUST_QP
		//evbrRC->m_evbrStillPDQP = 0;
		RCClearStillPDQP(encRC->m_chn);
		if (g_EvbrS2MCnt) {
			//evbrRC->m_evbrS2MLimitCnt = evbrRC->m_measurePeriod + g_EvbrS2MCnt;
			RCSetEvbrS2MCnt(encRC->m_chn, evbrRC->m_measurePeriod + g_EvbrS2MCnt);
		}
		#endif
		#if EVBR_KEEP_PIC_QUALITY
		if (RCGetKeppIQualityEn(encRC->m_chn)) {
			encRC->m_lastLevelQP[RC_I_FRAME_LEVEL] = evbrRC->m_stillIQP;
			//DBG_DUMP("set last level qp = %d\n", encRC->m_lastLevelQP[RC_I_FRAME_LEVEL]);
		}
		#endif
		rc_dbg("motion state\r\n");
    }
    else {
        #if UPDATE_QP_BY_PSNR
        int64_t sse;
        #endif
        #if EVBR_XA
		int64_t actualbit = ((int64_t)pUPic->uiFrameSize) << 3;
		int64_t targetbit = division(encRC->m_encRCGOP.m_bitsLeft, encRC->m_encRCGOP.m_picLeft);
		if (evbrRC->m_stillOverflow > 0 && evbrRC->m_stillIadd == 2 && actualbit>targetbit){
			evbrRC->m_last_stillPQP++;
			evbrRC->m_last_stillPQP = RC_CLIP3((int)evbrRC->m_stillPQP, (int)encRC->m_maxPQp, (int)evbrRC->m_last_stillPQP);
			evbrRC->m_last_stillPQP = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)evbrRC->m_last_stillPQP);
		}
		if (evbrRC->m_stillOverflow > 0 && evbrRC->m_last_stillIQP < evbrRC->m_stillIQP){
			evbrRC->m_tmp2_stillIQP = evbrRC->m_stillIQP;
			evbrRC->m_tmp2_stillKeyPQP = evbrRC->m_stillKeyPQP;
		}
		if (evbrRC->m_stillOverflow>110 && (evbrRC->m_last_stillIQP >= evbrRC->m_stillIQP)){
				evbrRC->m_tmp_stillIQP = (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPip / 10;
                evbrRC->m_tmp_stillIQP = RC_CLIP3((int)encRC->m_minIQp, (int)encRC->m_maxIQp, (int)evbrRC->m_tmp_stillIQP);
				evbrRC->m_stillIadd = 2;
				evbrRC->m_tmp_stillKeyPQP = (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPkp / 10;
                evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)evbrRC->m_tmp_stillKeyPQP);
				evbrRC->m_stillKeyPadd = 2;
		}
		else if (evbrRC->m_stillOverflow>0){
			if (evbrRC->m_stillIadd == 0){
				evbrRC->m_tmp_stillIQP++;
				evbrRC->m_tmp_stillIQP = RC_CLIP3((int)evbrRC->m_stillIQP, (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPip / 10, (int)evbrRC->m_tmp_stillIQP);
                evbrRC->m_tmp_stillIQP = RC_CLIP3((int)encRC->m_minIQp, (int)encRC->m_maxIQp, (int)evbrRC->m_tmp_stillIQP);
				evbrRC->m_stillIadd = 1;
				evbrRC->m_tmp_stillKeyPQP++;
				evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)evbrRC->m_stillKeyPQP, (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPkp / 10, (int)evbrRC->m_tmp_stillKeyPQP);
				evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)evbrRC->m_tmp_stillKeyPQP);
				evbrRC->m_stillKeyPadd = 1;
			}
			if (evbrRC->m_stillKeyPadd == 0){
				evbrRC->m_tmp_stillKeyPQP++;
				evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)evbrRC->m_stillKeyPQP, (int)evbrRC->m_stillPQP - encRC->m_rc_deltaQPkp / 10, (int)evbrRC->m_tmp_stillKeyPQP);
				evbrRC->m_tmp_stillKeyPQP = RC_CLIP3((int)encRC->m_minPQp, (int)encRC->m_maxPQp, (int)evbrRC->m_tmp_stillKeyPQP);
				evbrRC->m_stillKeyPadd = 1;
			}

		}

		if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType) {
			if (encRC->m_encRCSeq.m_bitsDiff > 0) {
				encRC->m_encRCSeq.m_bitsDiff = 0;  //STILL MODE
				#if EVALUATE_PRED_DIFF
				RCClearPredDiff(encRC->m_chn);
				#endif
			}
			initRCGOP(encRC, encRC->m_GOPSize);
		}
		#if EVBR_ADJUST_QP
		if (g_EVBRAdjustQPMethod) {
			if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType)
				encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_I_FRAME_LEVEL];
			else if (encRC->m_ltrFrame)
				encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_LT_P_FRAME_LEVEL];
			else if (encRC->m_SP_sign)
				encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_KEY_P_FRAME_LEVEL];
			else
				encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_P_FRAME_LEVEL];
		}
		else
		#endif
		{
			encRC->m_encRCGOP.m_bitsLeft -= actualbit;
			encRC->m_encRCSeq.m_bitsDiff += encRC->m_encRCSeq.m_averageBits - actualbit;
			#if EVALUATE_PRED_DIFF
			RCSetPredDiff(encRC->m_chn, RCGetPredDiff(encRC->m_chn)+encRC->m_encRCPic.m_targetBits - actualbit);
			#endif
		}
        #else   // EVBR_XA
        // 3. update left pic & bit
        if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType) {
            initRCGOP(encRC, encRC->m_GOPSize);
			encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_I_FRAME_LEVEL];
        }
		else if (encRC->m_SP_sign)
			encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_KEY_P_FRAME_LEVEL];
		else
			encRC->m_encRCGOP.m_bitsLeft -= encRC->m_encRCGOP.m_picTargetBitInGOP[RC_P_FRAME_LEVEL];
        #endif  // EVBR_XA
		// 4. compute PSNR
        #if UPDATE_QP_BY_PSNR
        sse = (((int64_t)pUPic->uiYMSE[1])<<32) | pUPic->uiYMSE[0];
        encRC->m_encRCPic.m_psnr = (uint32_t)calculatePSNR(sse, encRC->m_encRCSeq.m_numberOfMB*256);
        if ((RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType) && encRC->m_encRCEvbr.m_IPsnrCnd > 0) {
            if (encRC->m_encRCPic.m_psnr > (encRC->m_encRCEvbr.m_IPsnrCnd*11/10))
                encRC->m_encRCEvbr.m_stillIQP++;
            else if (encRC->m_encRCPic.m_psnr < encRC->m_encRCEvbr.m_IPsnrCnd)
                encRC->m_encRCEvbr.m_stillIQP--;
            //rc_dump_info("I PSNR %d, qp %d\r\n", (int)(encRC->m_encRCPic.m_psnr), (int)(encRC->m_encRCEvbr.m_stillIQP));
        }
        else if (encRC->m_SP_sign && encRC->m_encRCEvbr.m_KeyPPsnrCnd > 0) {
            if (encRC->m_encRCPic.m_psnr > (encRC->m_encRCEvbr.m_KeyPPsnrCnd*11/10))
                encRC->m_encRCEvbr.m_stillKeyPQP++;
            else if (encRC->m_encRCPic.m_psnr < encRC->m_encRCEvbr.m_KeyPPsnrCnd)
                encRC->m_encRCEvbr.m_stillKeyPQP--;
            //rc_dump_info("key P PSNR %d, qp %d\r\n", (int)(encRC->m_encRCPic.m_psnr), (int)(encRC->m_encRCEvbr.m_stillKeyPQP));
        }
        #endif
    	if (encRC->m_SP_sign == 1) {
    		encRC->m_SP_sign = 0;
    		encRC->m_GOP_SPnumleft--;
    	}
        encRC->m_encRCGOP.m_picLeft--;
    }
}

#if SUPPORT_VBR2
static void TEncRateCtrl_Vbr2Update(TEncRateCtrl *encRC, TEncRCVBR2 *encRCVbr2, H26XEncRCUpdatePic *pUPic)
{
	//Update bitrate
	int overflowLevel;
	int fixQP = 0;
#if SUPPORT_VBR2_CVTE
	int currQP;
	int overflowThd;
	int overflowThd2;
	int underflowThd;
#endif
#if BITRATE_OVERFLOW_CHECK
#if FIX_BUG_ALL_INTRA
	if ((RC_I_SLICE != pUPic->ucPicType && RC_IDR_SLICE != pUPic->ucPicType) ||
		(encRC->m_GOPSize == 1))
#else
	if (RC_I_SLICE != pUPic->ucPicType && RC_IDR_SLICE != pUPic->ucPicType)
#endif
	{
		if (0 == encRC->m_LT_sign && 0 == encRC->m_SP_sign)
		{
//			RCBitRecordUpdate(&encRCVbr2->m_encRCBr, (pUPic->uiFrameSize << 3), encRCVbr2->m_measurePeriod);
			RCBitRecordUpdate2(&encRCVbr2->m_encRCBr, (pUPic->uiFrameSize << 3), encRCVbr2->m_measurePeriod, encRC->m_svcLayer);
		}
	}
#endif
	//Change to VBR mode ?
#if FIX_BUG_ALL_INTRA
	if ((!encRCVbr2->m_isFirstPic && VBR2_CBR_STATE == encRCVbr2->m_vbr2State && 0 == encRC->m_encRCGOP.m_picLeft && encRC->m_GOPSize > 1) ||
		(encRC->m_GOPSize == 1 && encRCVbr2->m_encRCBr.recrod_num == (int)encRCVbr2->m_measurePeriod))
#else
	if (!encRCVbr2->m_isFirstPic && VBR2_CBR_STATE == encRCVbr2->m_vbr2State && 0 == encRC->m_encRCGOP.m_picLeft)
#endif
	{
		encRCVbr2->m_vbr2State = VBR2_VBR_STATE;
	}

	//Check overflow
	if (encRC->m_ltrPeriod > 0)
	{
		overflowLevel = RCVBR2BitrateOverflowCheck(&encRCVbr2->m_encRCBr, encRCVbr2->m_rateOverflowThd, encRC->m_GOPSize, encRC->m_ltrPeriod,
			encRCVbr2->m_lastISize, encRCVbr2->m_lastLTPSize, 0, 0);
	}
	else if (encRC->m_keyPPeriod > 0)
	{
		overflowLevel = RCVBR2BitrateOverflowCheck(&encRCVbr2->m_encRCBr, encRCVbr2->m_rateOverflowThd, encRC->m_GOPSize, encRC->m_keyPPeriod,
			encRCVbr2->m_lastISize, encRCVbr2->m_lastKPSize, 0, 0);
	}
	else
	{
		overflowLevel = RCVBR2BitrateOverflowCheck(&encRCVbr2->m_encRCBr, encRCVbr2->m_rateOverflowThd, encRC->m_GOPSize, 0,
			encRCVbr2->m_lastISize, 0, 0, 0);
	}
	encRCVbr2->m_overflowLevel = overflowLevel;

	if (VBR2_VBR_STATE == encRCVbr2->m_vbr2State)
	{
#if SUPPORT_VBR2_CVTE
		currQP = encRCVbr2->m_currPQP;
		if (encRC->m_svcLayer == 1)
		{
			currQP += encRCVbr2->m_currP2QP + 1;
			currQP >>= 1;
		}
		else if (encRC->m_svcLayer == 2)
		{
			currQP += encRCVbr2->m_currP2QP + encRCVbr2->m_currP3QP + encRCVbr2->m_currPQP + 2;
			currQP >>= 2;
		}
		currQP -= 30;
		currQP = RC_CLIP3(-2, 6, currQP);
		overflowThd = encRCVbr2->m_overflowThd + currQP * 5;
		overflowThd2 = encRCVbr2->m_overflowThd2 + currQP * 5;
		underflowThd = encRCVbr2->m_underflowThd + currQP * 5;
		overflowThd = RC_CLIP3(50, 100, overflowThd);
		overflowThd2 = RC_CLIP3(90, 110, overflowThd2);
		underflowThd = RC_CLIP3(30, 80, underflowThd);

		#if ADUST_VBR2_OVERFLOW_CHECK
			if (overflowLevel > overflowThd2)
			{
				fixQP = 1;
				encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
			}
			else if (overflowLevel > overflowThd)
			{
				if (encRCVbr2->m_changeFrmCnt >= 2*MAX_CHANGE_FRAME_NUM)
				{
					fixQP = 1;
					encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
				}
				else if (encRCVbr2->m_changeFrmCnt < MAX_CHANGE_FRAME_NUM)
				{
					encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
					encRCVbr2->m_changeFrmCnt++;
				}
				else
				{
					encRCVbr2->m_changeFrmCnt++;
				}
			}
			else if (overflowLevel < underflowThd  && encRC->m_encRCGOP.m_bitsLeft > 100)
			{		
				if ( (encRCVbr2->m_changeFrmCnt <= (MAX_CHANGE_FRAME_NUM - MAX_CHANGE_FRAME_NUM_UND)) || (encRCVbr2->m_currPQP > g_VBR2UndQPThd))
				{
					fixQP = -1;
					encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
				}
				else if (encRCVbr2->m_changeFrmCnt > MAX_CHANGE_FRAME_NUM)
				{
					encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
					encRCVbr2->m_changeFrmCnt--;			
				}
				else
				{
					encRCVbr2->m_changeFrmCnt--;
				}
			}
			else
			{
				encRCVbr2->m_changeFrmCnt = MAX_CHANGE_FRAME_NUM;
			}

			#if ADD_VBR2_THD_BITREC_LOG
			TEncRCPic_setVBRThdLog(encRC, overflowThd, overflowThd2, underflowThd);
			#endif
		#else
			if (overflowLevel > overflowThd2)
			{
				fixQP = 1;
				encRCVbr2->m_changeFrmCnt = 0;
			}
			else if (overflowLevel > overflowThd)
			{
				if (encRCVbr2->m_changeFrmCnt >= MAX_CHANGE_FRAME_NUM)
				{
					fixQP = 1;
					encRCVbr2->m_changeFrmCnt = 0;
				}
				else
				{
					encRCVbr2->m_changeFrmCnt++;
				}
			}
			else if (overflowLevel < underflowThd)
			{
				fixQP = -1;
				encRCVbr2->m_changeFrmCnt = 0;
			}
			else
			{
				encRCVbr2->m_changeFrmCnt = 0;
			}
		#endif
#else
		if (overflowLevel > (int)encRCVbr2->m_changePosThd)
		{
			fixQP = 1;
		}
		if (overflowLevel < (int)encRCVbr2->m_underflowThd)
		{
			fixQP = -1;
		}
#endif

		if (fixQP == 1)
		{
			if (encRC->m_svcLayer == 2)
			{
				encRCVbr2->m_currPQP += fixQP;
				if (encRCVbr2->m_currPQP - encRCVbr2->m_currP3QP > encRCVbr2->m_deltaP3QP)
				{
					encRCVbr2->m_currP3QP += fixQP;
				}
				if (encRCVbr2->m_currP3QP - encRCVbr2->m_currP2QP > encRCVbr2->m_deltaP2QP - encRCVbr2->m_deltaP3QP)
				{
					encRCVbr2->m_currP2QP += fixQP;
				}
			}
			else if (encRC->m_svcLayer == 1)
			{
				encRCVbr2->m_currPQP += fixQP;
				if (encRCVbr2->m_currPQP - encRCVbr2->m_currP2QP > encRCVbr2->m_deltaP2QP)
				{
					encRCVbr2->m_currP2QP += fixQP;
				}
			}
			else
			{
				encRCVbr2->m_currPQP += fixQP;
				encRCVbr2->m_currP2QP += fixQP;
				encRCVbr2->m_currP3QP += fixQP;
			}
		#if FIX_BUG_ALL_INTRA
			if (encRC->m_GOPSize == 1)
			{
				encRCVbr2->m_currIQP += fixQP;
			}
		#endif
		}
		else if (fixQP == -1)
		{
			if (encRC->m_svcLayer == 2)
			{
				encRCVbr2->m_currP2QP += fixQP;
				if (encRCVbr2->m_currP3QP - encRCVbr2->m_currP2QP > encRCVbr2->m_deltaP2QP - encRCVbr2->m_deltaP3QP)
				{
					encRCVbr2->m_currP3QP += fixQP;
				}
				if (encRCVbr2->m_currPQP - encRCVbr2->m_currP3QP > encRCVbr2->m_deltaP3QP)
				{
					encRCVbr2->m_currPQP += fixQP;
				}
			}
			else if (encRC->m_svcLayer == 1)
			{
				encRCVbr2->m_currP2QP += fixQP;
				if (encRCVbr2->m_currPQP - encRCVbr2->m_currP2QP > encRCVbr2->m_deltaP2QP)
				{
					encRCVbr2->m_currPQP += fixQP;
				}
			}
			else
			{
				encRCVbr2->m_currPQP += fixQP;
				encRCVbr2->m_currP2QP += fixQP;
				encRCVbr2->m_currP3QP += fixQP;
			}
		#if FIX_BUG_ALL_INTRA
			if (encRC->m_GOPSize == 1)
			{
				encRCVbr2->m_currIQP += fixQP;
			}
		#endif
		}
		encRCVbr2->m_currPQP = RC_CLIP3(encRCVbr2->m_initPQP, encRCVbr2->m_maxPQP, encRCVbr2->m_currPQP);
		encRCVbr2->m_currP2QP = RC_CLIP3(encRCVbr2->m_initP2QP, encRCVbr2->m_maxPQP, encRCVbr2->m_currP2QP);
		encRCVbr2->m_currP3QP = RC_CLIP3(encRCVbr2->m_initP3QP, encRCVbr2->m_maxPQP, encRCVbr2->m_currP3QP);
	#if FIX_BUG_ALL_INTRA
		if (encRC->m_GOPSize == 1)
		{
			encRCVbr2->m_currIQP = RC_CLIP3(encRCVbr2->m_initIQP, encRCVbr2->m_maxIQP, encRCVbr2->m_currIQP);
		}
	#endif
	}

	//Update bits
	if (VBR2_VBR_STATE == encRCVbr2->m_vbr2State)
	{
		if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType)
		{
			encRC->m_encRCSeq.m_bitsDiff = 0;
			initRCGOP(encRC, encRC->m_GOPSize);
#if !SUPPORT_VBR2_CVTE
			encRC->m_encRCGOP.m_bitsLeft = division(encRC->m_encRCGOP.m_bitsLeft * encRCVbr2->m_changePosThd, 100);
#endif
		}
		encRC->m_encRCGOP.m_bitsLeft -= ((int64_t)pUPic->uiFrameSize) << 3;

		if (encRC->m_LT_sign)
		{
			encRC->m_GOP_LTnumleft--;
		}
		else if (encRC->m_SP_sign)
		{
			encRC->m_GOP_SPnumleft--;
		}
		if (encRC->m_svcLayer > 0)
		{
			if (encRC->m_encRCPic.m_frameLevel == RC_P2_FRAME_LEVEL)
			{
				encRC->m_GOP_P2numleft--;
			}
			else if (encRC->m_encRCPic.m_frameLevel == RC_P3_FRAME_LEVEL)
			{
				encRC->m_GOP_P3numleft--;
			}
		}
		encRC->m_encRCGOP.m_picLeft--;
	}

	//Update weights
	if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType || encRC->m_LT_sign || encRC->m_SP_sign)
	{
		if ((uint32_t)encRCVbr2->m_encRCBr.recrod_num >= encRCVbr2->m_measurePeriod)
		{
			encRCVbr2->m_bitRatio[RC_P_FRAME_LEVEL] = 1 << 5;
			encRCVbr2->m_bitRatio[RC_KEY_P_FRAME_LEVEL] = (uint32_t)division((int64_t)encRCVbr2->m_lastKPSize * (encRCVbr2->m_measurePeriod << 5), encRCVbr2->m_encRCBr.bitrate_record);
			encRCVbr2->m_bitRatio[RC_LT_P_FRAME_LEVEL] = (uint32_t)division((int64_t)encRCVbr2->m_lastLTPSize * (encRCVbr2->m_measurePeriod << 5), encRCVbr2->m_encRCBr.bitrate_record);
			encRCVbr2->m_bitRatio[RC_P2_FRAME_LEVEL] = (uint32_t)division((int64_t)encRCVbr2->m_lastP2Size << 5, encRCVbr2->m_lastPSize);
			encRCVbr2->m_bitRatio[RC_P3_FRAME_LEVEL] = (uint32_t)division((int64_t)encRCVbr2->m_lastP3Size << 5, encRCVbr2->m_lastPSize);
			encRCVbr2->m_bitRatio[RC_KEY_P_FRAME_LEVEL] = RC_CLIP3(1 << 5, 1 << 12, encRCVbr2->m_bitRatio[RC_KEY_P_FRAME_LEVEL]);
			encRCVbr2->m_bitRatio[RC_LT_P_FRAME_LEVEL] = RC_CLIP3(1 << 5, 1 << 12, encRCVbr2->m_bitRatio[RC_LT_P_FRAME_LEVEL]);
			encRCVbr2->m_bitRatio[RC_P3_FRAME_LEVEL] = RC_CLIP3(1 << 5, 1 << 12, encRCVbr2->m_bitRatio[RC_P3_FRAME_LEVEL]);
			encRCVbr2->m_bitRatio[RC_P2_FRAME_LEVEL] = RC_CLIP3(encRCVbr2->m_bitRatio[RC_P3_FRAME_LEVEL], 1 << 12, encRCVbr2->m_bitRatio[RC_P2_FRAME_LEVEL]);
		}
		encRC->m_LT_sign = 0;
		encRC->m_SP_sign = 0;
	}

	if (encRCVbr2->m_isFirstPic)
	{
		encRCVbr2->m_isFirstPic = 0;
	}

	rc_dbg("{chn%d} slice_type:%d, TBR:%d, ChangePos:%d, Overflowlevel:%d, IQP:%d, PQP:%d, KPQP:%d, LTPQP:%d, P2QP:%d, P3QP:%d, targetbits:%d, actualbits:%d, bitsLeft:%lld\r\n",
		encRC->m_chn, encRC->m_encRCPic.m_frameLevel, (int)encRCVbr2->m_maxBitrate, (int)encRCVbr2->m_changePosThd, (int)encRCVbr2->m_overflowLevel,
		(int)encRCVbr2->m_currIQP, (int)encRCVbr2->m_currPQP, (int)encRCVbr2->m_currKPQP, (int)encRCVbr2->m_currLTPQP, (int)encRCVbr2->m_currP2QP,
		(int)encRCVbr2->m_currP3QP, (int)encRCVbr2->m_targetBits, pUPic->uiFrameSize << 3, encRC->m_encRCGOP.m_bitsLeft);
	TEncRCPic_setVBRLog(encRC, encRCVbr2, pUPic->uiFrameSize << 3);
	#if ADD_VBR2_THD_BITREC_LOG
	TEncRCPic_setVBRBitRecordLog(encRC, &encRCVbr2->m_encRCBr); 
	#endif
}
#endif

static void TEncRateCtrl_CalculateWeight(TEncRateCtrl *encRC, int Keyframe)
{
	int lambdaRatio[RC_MAX_FRAME_LEVEL];  //baoliu
	int f_rc_weight_I, f_rc_weight_LT, f_rc_weight_SP, f_rc_weight_P, f_rc_weight_P2, f_rc_weight_P3;
	int  alpha_I, alpha_LT, alpha_SP, alpha_P, alpha_P2, alpha_P3;
	int  beta_I, beta_LT, beta_SP, beta_P, beta_P2, beta_P3;
	int64_t weight_I, weight_LT, weight_SP, weight_P2, weight_P3;
	int num_lt, num_sp, num_p, num_P2, num_P3;
	int64_t A, A2, A3, A22, A23;
	int64_t B, B2, B3, B22, B23;
	int64_t L, M, L2, M2, L3, M3, L22, M22, L23, M23, fL, fL2, fL3, fL22, fL23;
	int fM, fM2, fM3, fM22, fM23, fgx22, fgx23, gx22, gx23, gx2, fgx2, gx, fgx, gx3, fgx3;
	int tmp_value;
	int targetbpp;
	int i;
	uint64_t tmp_GOPbits, tmp_nopixel;
	int64_t tmp_solution;
	int solution;
	int64_t best;
	int out_bool = 0;
	int tmp_best = (int)MAX_INTEGER;
	int last_solution = (int)MAX_INTEGER;
	int Llast_solution = (int)MAX_INTEGER;
#if WEIGHT_CAL_RESET
	int min_weight_I, min_weight_LT, min_weight_SP, min_weight_P2, min_weight_P3;
	int max_weight_I, max_weight_LT, max_weight_SP, max_weight_P2, max_weight_P3;
	min_weight_I = min_weight_LT = min_weight_SP = min_weight_P2 = min_weight_P3 = 10;
	max_weight_I = max_weight_LT = max_weight_SP = max_weight_P2 = max_weight_P3 = (int)MAX_INTEGER;

	if (encRC->m_rc_deltaQPip < 0)
		min_weight_I = 1;
	if (encRC->m_rc_deltaQPkp < 0)
		min_weight_SP = 1;
	if (encRC->m_deltaLT_QP < 0)
		min_weight_LT = 1;
	if (encRC->m_deltaP2_QP < 0)
		min_weight_P2 = 1;
	if (encRC->m_deltaP3_QP < 0)
		min_weight_P3 = 1;

	if (encRC->m_rc_deltaQPip < 3) {
	#if ALPHA_BETA_MIN_MAX_ISSUE
		max_weight_I = 5000;
	#else
		max_weight_I = 1000;
	#endif
	}

	if (encRC->m_rc_deltaQPkp < 5)
		max_weight_SP = 1000;
	if (encRC->m_deltaLT_QP < 5)
		max_weight_LT = 1000;
	if (encRC->m_deltaP2_QP < 5)
		max_weight_P2 = 1000;
	if (encRC->m_deltaP3_QP < 5)
		max_weight_P3 = 1000;
#endif

	num_lt = num_sp = num_p = num_P2 = num_P3 = 0;
	weight_I = weight_LT = weight_SP = weight_P2 = weight_P3 = 0;
	A = A2 = A3 = A22 = A23 = 1;
	B = B2 = B3 = B22 = B23 = 1;
	L = M = L2 = M2 = L3 = M3 = L22 = M22 = L23 = M23 = fL = fL2 = fL3 = fL22 = fL23 = 1;
	fM = fM2 = fM3 = fM22 = fM23 = fgx22 = fgx23 = gx22 = gx23 = gx2 = fgx2 = gx = fgx = gx3 = fgx3 = 1;

	tmp_GOPbits = encRC->m_encRCGOP.m_bitsLeft << (SHIFT_11 - 8);
	tmp_nopixel = encRC->m_encRCSeq.m_numberOfMB;
	targetbpp = division(tmp_GOPbits, tmp_nopixel);
	targetbpp = RC_CLIP3(0, (int64_t)MAX_INTEGER, targetbpp);
	num_lt = encRC->m_GOP_LTnumleft;
	num_sp = encRC->m_GOP_SPnumleft;
	num_P2 = encRC->m_GOP_P2numleft;
	num_P3 = encRC->m_GOP_P3numleft;
	num_p = encRC->m_encRCGOP.m_picLeft - Keyframe - num_sp - num_P2 - num_P3 - num_lt; //需要修改

	lambdaRatio[RC_I_FRAME_LEVEL] = 2048; //lambdaratio*2048
	tmp_value = 6056 * encRC->m_rc_deltaQPip;// 0.1 * 128 * 0.231 * 2048 encRC->m_rc_deltaQPip;
	tmp_value >>= 7;
	lambdaRatio[RC_P_FRAME_LEVEL] = calculatePow(1973, 27182, tmp_value, 1); // (0.9634 * exp(0.231 * encRC->m_rc_deltaQPip)) * 2048

	tmp_value = 6056 * (encRC->m_rc_deltaQPip - encRC->m_rc_deltaQPkp);
	tmp_value >>= 7;
	lambdaRatio[RC_KEY_P_FRAME_LEVEL] = calculatePow(1973, 27182, tmp_value, 1);//deltaP2=0;

	tmp_value = 6056 * (encRC->m_rc_deltaQPip - encRC->m_deltaLT_QP);// 0.1 * 128 * 0.231 * 2048 encRC->m_rc_deltaQPip;
	tmp_value >>= 7;
	lambdaRatio[RC_LT_P_FRAME_LEVEL] = calculatePow(1973, 27182, tmp_value, 1);//deltaP2=0;

	tmp_value = 6056 * (encRC->m_rc_deltaQPip - encRC->m_deltaP2_QP);// 0.1 * 128 * 0.231 * 2048 encRC->m_rc_deltaQPip;
	tmp_value >>= 7;
	lambdaRatio[RC_P2_FRAME_LEVEL] = calculatePow(1973, 27182, tmp_value, 1);//deltaP2=0;

	tmp_value = 6056 * (encRC->m_rc_deltaQPip - encRC->m_deltaP3_QP);// 0.1 * 128 * 0.231 * 2048 encRC->m_rc_deltaQPip;
	tmp_value >>= 7;
	lambdaRatio[RC_P3_FRAME_LEVEL] = calculatePow(1973, 27182, tmp_value, 1);


	alpha_I = encRC->m_encRCSeq.m_picPara[RC_I_FRAME_LEVEL].m_alpha;
	beta_I = encRC->m_encRCSeq.m_picPara[RC_I_FRAME_LEVEL].m_beta;
	alpha_P = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_alpha;
	beta_P = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_beta;
    #if FIX_BUG_HK
	A = ((int64_t)alpha_P*(int64_t)TIMES_10) << SHIFT_11;
	A = division(A, alpha_I) * lambdaRatio[RC_I_FRAME_LEVEL];
	A = division(A, lambdaRatio[RC_P_FRAME_LEVEL]);
	L = (int64_t)A >> SHIFT_11;
    #else
	A = (int64_t)alpha_P << SHIFT_11;
	A = division(A, alpha_I) * lambdaRatio[RC_I_FRAME_LEVEL];
	A = division(A, lambdaRatio[RC_P_FRAME_LEVEL]);
	L = ((int64_t)A * (int64_t)TIMES_10) >> SHIFT_11;
    #endif
	B = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_I);
	M = (int64_t)beta_P << SHIFT_11;
	L = calculatePow(SHIFT_VALUE, L, B, 1);
	M = division(M, beta_I);
	fL = (L * M) >> SHIFT_11;
	fM = RC_CLIP3((int64_t)(-204800), (int64_t)204800, M - 2048);
	if (num_lt > 0){
		alpha_LT = encRC->m_encRCSeq.m_picPara[RC_LT_P_FRAME_LEVEL].m_alpha;
		beta_LT = encRC->m_encRCSeq.m_picPara[RC_LT_P_FRAME_LEVEL].m_beta;
        #if FIX_BUG_HK
        A3 = ((int64_t)alpha_P * (int64_t)TIMES_10) << SHIFT_11;
		A3 = division(A3, alpha_LT) *lambdaRatio[RC_LT_P_FRAME_LEVEL];
		A3 = division(A3, lambdaRatio[RC_P_FRAME_LEVEL]);
		B3 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_LT);
		L3 = ((int64_t)A3) >> SHIFT_11;
        #else
		A3 = (int64_t)alpha_P << SHIFT_11;
		A3 = division(A3, alpha_LT) *lambdaRatio[RC_LT_P_FRAME_LEVEL];
		A3 = division(A3, lambdaRatio[RC_P_FRAME_LEVEL]);
		B3 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_LT);
		L3 = ((int64_t)A3 * (int64_t)TIMES_10) >> SHIFT_11;
        #endif
		M3 = (int64_t)beta_P << SHIFT_11;
		L3 = calculatePow(SHIFT_VALUE, L3, B3, 1);
		M3 = division(M3, beta_LT);
		fL3 = (L3 * M3) >> SHIFT_11;
		fM3 = RC_CLIP3((int64_t)(-204800), (int64_t)204800, M3 - 2048);
	}
	if (num_sp > 0){
		alpha_SP = encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_alpha;
		beta_SP = encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_beta;
        #if FIX_BUG_HK
		A2 = ((int64_t)alpha_P * (int64_t)TIMES_10) << SHIFT_11;
		A2 = division(A2, alpha_SP) *lambdaRatio[RC_KEY_P_FRAME_LEVEL];
		A2 = division(A2, lambdaRatio[RC_P_FRAME_LEVEL]);
		B2 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_SP);
		L2 = ((int64_t)A2) >> SHIFT_11;
        #else
		A2 = (int64_t)alpha_P << SHIFT_11;
		A2 = division(A2, alpha_SP) *lambdaRatio[RC_KEY_P_FRAME_LEVEL];
		A2 = division(A2, lambdaRatio[RC_P_FRAME_LEVEL]);
		B2 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_SP);
		L2 = ((int64_t)A2 * (int64_t)TIMES_10) >> SHIFT_11;
        #endif
		M2 = (int64_t)beta_P << SHIFT_11;
		L2 = calculatePow(SHIFT_VALUE, L2, B2, 1);
		M2 = division(M2, beta_SP);
		fL2 = (L2 * M2) >> SHIFT_11;
		fM2 = RC_CLIP3((int64_t)(-204800), (int64_t)204800, M2 - 2048);
	}
	if (encRC->m_P2_frm_num > 0){
		alpha_P2 = encRC->m_encRCSeq.m_picPara[RC_P2_FRAME_LEVEL].m_alpha;
		beta_P2 = encRC->m_encRCSeq.m_picPara[RC_P2_FRAME_LEVEL].m_beta;
        #if FIX_BUG_HK
		A22 = ((int64_t)alpha_P*(int64_t)TIMES_10) << SHIFT_11;
		A22 = division(A22, alpha_P2) *lambdaRatio[RC_P2_FRAME_LEVEL];
		A22 = division(A22, lambdaRatio[RC_P_FRAME_LEVEL]);
		B22 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_P2);
		L22 = ((int64_t)A22) >> SHIFT_11;
        #else
		A22 = (int64_t)alpha_P << SHIFT_11;
		A22 = division(A22, alpha_P2) *lambdaRatio[RC_P2_FRAME_LEVEL];
		A22 = division(A22, lambdaRatio[RC_P_FRAME_LEVEL]);
		B22 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_P2);
		L22 = ((int64_t)A22*(int64_t)TIMES_10) >> SHIFT_11;
        #endif
		L22 = calculatePow(SHIFT_VALUE, L22, B22, 1);
		M22 = (int64_t)beta_P << SHIFT_11;
		M22 = division(M22, beta_P2);
		fL22 = ((int64_t)L22*M22) >> SHIFT_11;
		fM22 = M22 - 2048;
	}
	if (encRC->m_P3_frm_num > 0){
		alpha_P3 = encRC->m_encRCSeq.m_picPara[RC_P3_FRAME_LEVEL].m_alpha;
		beta_P3 = encRC->m_encRCSeq.m_picPara[RC_P3_FRAME_LEVEL].m_beta;
        #if FIX_BUG_HK
		A23 = ((int64_t)alpha_P*(int64_t)TIMES_10) << SHIFT_11;
		A23 = division(A23, alpha_P3) *lambdaRatio[RC_P3_FRAME_LEVEL];//可以改为*lambdaratio{p3帧}
		A23 = division(A23, lambdaRatio[RC_P_FRAME_LEVEL]);
		B23 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_P3);
		L23 = ((int64_t)A23) >> SHIFT_11;
        #else
		A23 = (int64_t)alpha_P << SHIFT_11;
		A23 = division(A23, alpha_P3) *lambdaRatio[RC_P3_FRAME_LEVEL];//可以改为*lambdaratio{p3帧}
		A23 = division(A23, lambdaRatio[RC_P_FRAME_LEVEL]);
		B23 = division((int64_t)SHIFT_VALUE << SHIFT_11, beta_P3);
		L23 = ((int64_t)A23*(int64_t)TIMES_10) >> SHIFT_11;
        #endif
		L23 = calculatePow(SHIFT_VALUE, L23, B23, 1);
		M23 = (int64_t)beta_P << SHIFT_11;
		M23 = division(M23, beta_P3);
		fL23 = ((int64_t)L23*M23) >> SHIFT_11;
		fM23 = M23 - 2048;
	}
	tmp_solution = division((int64_t)targetbpp << 7, encRC->m_encRCGOP.m_picLeft);
	tmp_solution = tmp_solution >> 7;
	/* avoid integer overflowing, by CW */
	solution = RC_CLIP3(1, (int64_t)MAX_INTEGER - 1, tmp_solution);

	for (i = 0; i < 20; i++)
	{
		tmp_solution = ((int64_t)solution * (int64_t)TIMES_10) >> SHIFT_11;
		tmp_solution = RC_CLIP3(1, (int64_t)MAX_INTEGER, tmp_solution);
		gx = 0;
		fgx = 0;
		if (Keyframe){
			gx = calculatePow(L, tmp_solution, M, 1);
			weight_I = gx;
			fgx = calculatePow(fL, tmp_solution, fM, 1);
		}
		if (num_lt > 0){
			gx3 = calculatePow(L3, tmp_solution, M3, 1);
			weight_LT = gx3;
			gx += (gx3 * num_lt);
			fgx3 = calculatePow(fL3, tmp_solution, fM3, 1);
			fgx3 = fgx3 * num_lt;
			fgx += fgx3;
		}
		if (num_sp > 0){
			gx2 = calculatePow(L2, tmp_solution, M2, 1);
			weight_SP = gx2;
			gx += (gx2 * num_sp);
			fgx2 = calculatePow(fL2, tmp_solution, fM2, 1);
			fgx2 = fgx2 * num_sp;
			fgx += fgx2;
		}
		if (num_P2 > 0){
			gx22 = calculatePow(L22, tmp_solution, M22, 1);
			weight_P2 = gx22;
			gx += gx22*num_P2;
			fgx22 = calculatePow(fL22, tmp_solution, fM22, 1);
			fgx22 = fgx22*num_P2;
			fgx += fgx22;
		}
		if (num_P3 > 0){
			gx23 = calculatePow(L23, tmp_solution, M23, 1);
			weight_P3 = gx23;
			gx += gx23*num_P3;
			fgx23 = calculatePow(fL23, tmp_solution, fM23, 1);
			fgx23 = fgx23*num_P3;
			fgx += fgx23;
		}

		gx += solution * num_p;//i+sp+p
		gx -= targetbpp;
		fgx += (num_p << SHIFT_11);//i+sp+p

		if (tmp_best == gx)
			break;
		if (RC_ABS(gx) < tmp_best)
		{
			tmp_best = gx;
		}
		if (solution == last_solution || solution == Llast_solution)
			break;

		Llast_solution = last_solution;
		last_solution = solution;

		best = ((int64_t)gx << SHIFT_11);
		best = division(best, fgx);
		best = solution - best;
		if (best < 0 || best == 0)
			best = 1;
		if (out_bool == 1)
			break;

		/* solution is never being zero */
		if (RC_ABS(best - solution) < 1)
		{
			if ((i == 0 || gx>0) && solution > 1)
			{
				solution--;
				out_bool = 1;
			}
			else
				break;
		}
		else
			solution = best;
	}
#if WEIGHT_CAL_RESET
	if (Keyframe)
		f_rc_weight_I = RC_CLIP3((int64_t)min_weight_I, (int64_t)max_weight_I, division(weight_I * 10, solution));//g_max_weight
	else
		f_rc_weight_I = 0;
	if (num_lt == 0)
		f_rc_weight_LT = 0;
	else
		f_rc_weight_LT = RC_CLIP3((int64_t)min_weight_LT, (int64_t)max_weight_LT, division(weight_LT * 10, solution));//g_max_weight
	if (num_sp == 0)
		f_rc_weight_SP = 0;
	else
		f_rc_weight_SP = RC_CLIP3((int64_t)min_weight_SP, (int64_t)max_weight_SP, division(weight_SP * 10, solution));//g_max_weight
	if (num_P2 == 0)
		f_rc_weight_P2 = 0;
	else
		f_rc_weight_P2 = RC_CLIP3((int64_t)min_weight_P2, (int64_t)max_weight_P2, division(weight_P2 * 10, solution));//g_max_weight
	if (num_P3 == 0)
		f_rc_weight_P3 = 0;
	else
		f_rc_weight_P3 = RC_CLIP3((int64_t)min_weight_P3, (int64_t)max_weight_P3, division(weight_P3 * 10, solution));//g_max_weight
#else
	if (Keyframe)
		f_rc_weight_I = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(weight_I * 10, solution));
	else
		f_rc_weight_I = 0;
	if (num_lt == 0)
		f_rc_weight_LT = 0;
	else
		f_rc_weight_LT = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(weight_LT * 10, solution));
	if (num_sp == 0)
		f_rc_weight_SP = 0;
	else
		f_rc_weight_SP = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(weight_SP * 10, solution));
	if (num_P2 == 0)
		f_rc_weight_P2 = 0;
	else
		f_rc_weight_P2 = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(weight_P2 * 10, solution));

	if (num_P3 == 0)
		f_rc_weight_P3 = 0;
	else
		f_rc_weight_P3 = RC_CLIP3(1, (int64_t)MAX_INTEGER, division(weight_P3 * 10, solution));
#endif
	f_rc_weight_P = 10;

#if SUPPORT_SVC_FIXED_WEIGHT_BA
	if (encRC->m_svcBAMode)
	{
		//Recalculate bitsRatio
		int iWeightSum = 10 + encRC->m_svcP2Weight;
		if (encRC->m_svcLayer == 1)
		{
			f_rc_weight_P += f_rc_weight_P2;
			f_rc_weight_P = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)f_rc_weight_P * 10, iWeightSum));
			f_rc_weight_P2 = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)f_rc_weight_P * encRC->m_svcP2Weight, 10));
		}
		else
		{
			iWeightSum += 10 + encRC->m_svcP3Weight;
			f_rc_weight_P += f_rc_weight_P + f_rc_weight_P2 + f_rc_weight_P3;
			f_rc_weight_P = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)f_rc_weight_P * 10, iWeightSum));
			f_rc_weight_P2 = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)f_rc_weight_P * encRC->m_svcP2Weight, 10));
			f_rc_weight_P3 = (int)RC_CLIP3(1, MAX_INTEGER, division((int64_t)f_rc_weight_P * encRC->m_svcP3Weight, 10));
		}
	}
#endif
	initBitsRatio(&encRC->m_encRCSeq, f_rc_weight_I, f_rc_weight_SP, f_rc_weight_LT, f_rc_weight_P2, f_rc_weight_P3, f_rc_weight_P, Keyframe);
#if 0
	if (0 == encRC->m_chn)
		DBG_DUMP("new weight: POC:%d  SIZE: I:%d  P:%d  KP:%d  LT:%d  P2:%d  P3:%d  num_p:%d  num_kp:%d  num_lt:%d  num_p2:%d  num_P3:%d\r\n", (int)(encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_bitsLeft), f_rc_weight_I, f_rc_weight_P, f_rc_weight_SP, f_rc_weight_LT, f_rc_weight_P2, f_rc_weight_P3, num_p, num_sp, num_lt, num_P2, num_P3);
#endif
}

static void TEncRateCtrl_updateIPWeight(TEncRateCtrl *encRC)
{
#if MAX_SIZE_BUFFER
	int p_num;
	int allweight;
	int I_size;
#endif
#if RD_LAMBDA_UPDATE
	encRC->disable_clip = 0;
#endif
    initRCGOP(encRC, encRC->m_GOPSize);
    TEncRateCtrl_CalculateWeight(encRC,1);

#if MAX_SIZE_BUFFER
	p_num = encRC->m_encRCGOP.m_numPic - 1 - encRC->m_GOP_LTnumleft - encRC->m_GOP_SPnumleft - encRC->m_GOP_P2numleft - encRC->m_GOP_P3numleft;
	/*int f_rc_weight_I, f_rc_weight_SP, f_rc_weight_LT, f_rc_weight_P2, f_rc_weight_P3, f_rc_weight_P;
	f_rc_weight_I = encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL];
	f_rc_weight_LT = encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL];
	f_rc_weight_SP = encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL];
	f_rc_weight_P2 = encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL];
	f_rc_weight_P3 = encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL];
    f_rc_weight_P = encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL];
	int  allweight = f_rc_weight_I + f_rc_weight_LT * encRC->m_GOP_LTnumleft + encRC->m_GOP_SPnumleft*f_rc_weight_SP
		+ encRC->m_GOP_P2numleft*f_rc_weight_P2 + encRC->m_GOP_P3numleft*f_rc_weight_P3 + p_num*f_rc_weight_P;
	int I_size = RC_CLIP3(1, MAX_INTEGER, division(encRC->m_encRCGOP.m_bitsLeft * f_rc_weight_I, allweight));*/

	allweight = encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL] + encRC->m_GOP_SPnumleft*encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL] + encRC->m_GOP_LTnumleft*encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL]
		+ encRC->m_P2_frm_num*encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL] + encRC->m_P3_frm_num*encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL] + p_num*encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL];
	I_size = RC_CLIP3(1, MAX_INTEGER, division(encRC->m_encRCGOP.m_bitsLeft *encRC->m_encRCSeq.m_bitsRatio[RC_I_FRAME_LEVEL], allweight));

	if (encRC->m_max_frm_bit > 0 && I_size > encRC->m_max_frm_bit && p_num>0){
		ResetISize(encRC, &encRC->m_encRCGOP);
		if (encRC->m_chn < MAX_LOG_CHN)
			g_max_frm_bool[encRC->m_chn] = g_MSBDisableLimitQP;
	}
#endif

#if 1 /* Adjust, by XA 20171117 */
    initRCGOP(encRC, encRC->m_GOPSize);
#endif
    initRCPic(encRC, &encRC->m_encRCPic, RC_I_FRAME_LEVEL);
}

static void TEncRateCtrl_updateKPWeight(TEncRateCtrl *encRC)
{
    int weight = 0;
    int64_t target_bits;
    int currPicPosition = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
    int currPos;
    currPos = currPicPosition;
    if (encRC->m_ltrPeriod > 0) {
        currPos %= (int)encRC->m_ltrPeriod;
    }
    if ((encRC->m_keyPPeriod> 0 && currPicPosition % encRC->m_keyPPeriod == 0 )||
        (encRC->m_ltrPeriod > 0 && currPicPosition % encRC->m_ltrPeriod == 0 )||
        (encRC->m_svcLayer > 0 && currPos % 4 == 1))    {
        if (encRC->m_ltrPeriod > 0 && currPicPosition % encRC->m_ltrPeriod == 0)
            encRC->m_LT_sign = 1;
        else if (encRC->m_keyPPeriod> 0 && currPicPosition % encRC->m_keyPPeriod == 0)
            encRC->m_SP_sign = 1;
        else if (encRC->m_svcLayer > 0 && currPos % 4 == 1)
            encRC->m_HP_sign = 1;
        if ((encRC->m_SP_index > 0 && encRC->m_SP_sign == 1) || (encRC->m_LT_index > 0 && encRC->m_LT_sign == 1) || (encRC->m_HP_index > 0 && encRC->m_HP_sign == 1 && currPos%encRC->m_HP_period == 1))//weight jisuan
        {
            TEncRateCtrl_CalculateWeight(encRC, 0);
            target_bits = encRC->m_encRCGOP.m_bitsLeft;
            weight += (encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL] * encRC->m_GOP_LTnumleft);
            weight += (encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL] * encRC->m_GOP_SPnumleft);
            weight += (encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL] * encRC->m_GOP_P2numleft);
            weight += (encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL] * encRC->m_GOP_P3numleft);
            weight += (encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL] * (encRC->m_encRCGOP.m_picLeft - encRC->m_GOP_LTnumleft - encRC->m_GOP_SPnumleft - encRC->m_GOP_P2numleft - encRC->m_GOP_P3numleft));
            encRC->m_encRCGOP.m_picTargetBitInGOP[RC_KEY_P_FRAME_LEVEL] =
                RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits*encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL], weight));
            encRC->m_encRCGOP.m_picTargetBitInGOP[RC_P_FRAME_LEVEL] =
                RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits * encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL], weight));
            encRC->m_encRCGOP.m_picTargetBitInGOP[RC_LT_P_FRAME_LEVEL] =
                RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits * encRC->m_encRCSeq.m_bitsRatio[RC_LT_P_FRAME_LEVEL], weight));
            encRC->m_encRCGOP.m_picTargetBitInGOP[RC_P2_FRAME_LEVEL] =
                RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits * encRC->m_encRCSeq.m_bitsRatio[RC_P2_FRAME_LEVEL], weight));
            encRC->m_encRCGOP.m_picTargetBitInGOP[RC_P3_FRAME_LEVEL] =
                RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits * encRC->m_encRCSeq.m_bitsRatio[RC_P3_FRAME_LEVEL], weight));
        }
        if (encRC->m_LT_sign == 1) {
            initRCPic(encRC, &encRC->m_encRCPic, RC_LT_P_FRAME_LEVEL);
        }
        else if (encRC->m_SP_sign == 1) {
            initRCPic(encRC, &encRC->m_encRCPic, RC_KEY_P_FRAME_LEVEL);
        }
        else if (encRC->m_HP_sign == 1) {
            initRCPic(encRC, &encRC->m_encRCPic, RC_P_FRAME_LEVEL);
        }
    }
	else
	{
		{
			if (encRC->m_svcLayer > 0){
                if (currPos % 4 == 0)
                    initRCPic(encRC, &encRC->m_encRCPic, RC_P2_FRAME_LEVEL);
                if (currPos % 4 == 3)
                    initRCPic(encRC, &encRC->m_encRCPic, RC_P_FRAME_LEVEL);
                if (currPos % 4 == 2){
                    if (encRC->m_svcLayer == 2)
                        initRCPic(encRC, &encRC->m_encRCPic, RC_P3_FRAME_LEVEL);
                    else
                        initRCPic(encRC, &encRC->m_encRCPic, RC_P2_FRAME_LEVEL);
                }
			}
			else
			{
				initRCPic(encRC, &encRC->m_encRCPic, RC_P_FRAME_LEVEL);
			}
		}
	}
}

static void TEncRateCtrl_prepare(TEncRateCtrl *encRC, bool keyframe)
{
	int qp;// = 26;
	//int i, j;
	int  lambda;

	/* add by CW */
	if (keyframe){
		#if EVBR_MODE_SWITCH_NEW_METHOD
		if (1 == encRC->m_still2motion_I){
			encRC->m_still2motion_I = 0;
			encRC->m_still2motion_KP = 2;
        #if EVBR_XA
            encRC->m_encRCSeq.m_bitsDiff = 0;//still to motion
        #endif

			encRC->m_encRCSeq.m_picPara[RC_I_FRAME_LEVEL].m_alpha =
				encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_alpha = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_alpha;
			encRC->m_encRCSeq.m_picPara[RC_I_FRAME_LEVEL].m_beta =
				encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_beta = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_beta;

			encRC->m_lastLevelLambda[RC_I_FRAME_LEVEL] =
				encRC->m_lastLevelLambda[RC_P_FRAME_LEVEL] =
				encRC->m_lastLevelLambda[RC_KEY_P_FRAME_LEVEL] = -1;
			encRC->m_lastPicLambda = -1;
			encRC->m_lastValidLambda = -1;
		#if EVBR_KEEP_PIC_QUALITY
			if (0 == RCGetKeppIQualityEn(encRC->m_chn)) {
				encRC->m_lastLevelQP[RC_I_FRAME_LEVEL] = g_RCInvalidQPValue;
				//DBG_DUMP("set last I qp invalid\n");
			}
		#else
			encRC->m_lastLevelQP[RC_I_FRAME_LEVEL] = g_RCInvalidQPValue;
		#endif
			encRC->m_lastLevelQP[RC_P_FRAME_LEVEL] =
			encRC->m_lastLevelQP[RC_KEY_P_FRAME_LEVEL] = g_RCInvalidQPValue;
			encRC->m_lastPicQP = g_RCInvalidQPValue;
			encRC->m_lastValidQP = g_RCInvalidQPValue;

            initBitsRatio(&encRC->m_encRCSeq, encRC->m_record_init_weight[RC_I_FRAME_LEVEL],
                encRC->m_record_init_weight[RC_KEY_P_FRAME_LEVEL], encRC->m_record_init_weight[RC_LT_P_FRAME_LEVEL], encRC->m_record_init_weight[RC_P2_FRAME_LEVEL], encRC->m_record_init_weight[RC_P3_FRAME_LEVEL],
                encRC->m_record_init_weight[RC_P_FRAME_LEVEL], 1);

			initRCGOP(encRC, encRC->m_GOPSize);
			encRC->m_inumpic = 0;
		}
		#endif
		encRC->m_slicetype = (int)RC_I_SLICE;
	}
	else{
		encRC->m_slicetype = (int)RC_P_SLICE;
	}

	if (0 == encRC->m_inumpic)
	{
		int NumberBFrames = 0;//encRC->m_encRCSeq.m_GOPSize - 1;
		int dLambda_scale = 2048 - RC_CLIP3(0, 1024, 102 * NumberBFrames);
		int dQPFactor = (1167 * dLambda_scale) >> SHIFT_11;
		int SHIFT_QP = 12;
		int bitdepth_luma_qp_scale = 0;
		int tmp_qp;

		#if EVBR_MODE_SWITCH_NEW_METHOD
		if (2 == encRC->m_still2motion_KP){
			int tmp_rounding;
			int tmp_qp = encRC->m_record_lastQP * 10 - encRC->m_rc_deltaQPip;
			tmp_rounding = (tmp_qp % 10 > 4) ? 1 : 0;
			qp = tmp_qp / 10 + tmp_rounding;
		}
		else{
			qp = TEncRateCtrol_getInitQuant(encRC);
		}
		#else
        qp = TEncRateCtrol_getInitQuant(encRC);
		#endif
		encRC->m_inumpic = 1;
		#if SUPPORT_VBR2
		if (H26X_RC_VBR2 == encRC->m_rcMode)
		{
			encRC->m_encRCGOP.m_bitsLeft = division(encRC->m_encRCGOP.m_bitsLeft * encRC->m_encRCVbr2.m_changePosThd, 100);
		}
		#endif
        initRCPic(encRC, &encRC->m_encRCPic, RC_I_FRAME_LEVEL);

		tmp_qp = (qp + bitdepth_luma_qp_scale - SHIFT_QP) * SHIFT_VALUE / 3;
		lambda = calculatePow(dQPFactor, 20000, tmp_qp, 1);

		encRC->m_currlambda = lambda;
		encRC->m_currQP = qp;
	}
	else
	{
		#if EVBR_MODE_SWITCH_NEW_METHOD
		bool KP_reset_flag = 0;
		#endif
		if (keyframe) //update weight
		{
		    TEncRateCtrl_updateIPWeight(encRC);
		}
		else{
		#if EVBR_MODE_SWITCH_NEW_METHOD
			int currPicPosition = encRC->m_encRCGOP.m_numPic - encRC->m_encRCGOP.m_picLeft;
			if (encRC->m_keyPPeriod > 0 && currPicPosition % encRC->m_keyPPeriod == 0 && encRC->m_still2motion_KP != 0){
				int weight = 0;
				int64_t target_bits = encRC->m_encRCGOP.m_bitsLeft;

				encRC->m_SP_sign = 1;
				encRC->m_still2motion_KP = 0;
				encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_alpha = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_alpha;
				encRC->m_encRCSeq.m_picPara[RC_KEY_P_FRAME_LEVEL].m_beta = encRC->m_encRCSeq.m_picPara[RC_P_FRAME_LEVEL].m_beta;

				encRC->m_lastLevelLambda[RC_KEY_P_FRAME_LEVEL] = g_RCInvalidQPValue;
				encRC->m_lastLevelQP[RC_KEY_P_FRAME_LEVEL] = g_RCInvalidQPValue;

                initBitsRatio(&encRC->m_encRCSeq, 0, encRC->m_record_init_weight[RC_KEY_P_FRAME_LEVEL], encRC->m_record_init_weight[RC_LT_P_FRAME_LEVEL], encRC->m_record_init_weight[RC_P2_FRAME_LEVEL], encRC->m_record_init_weight[RC_P3_FRAME_LEVEL],
                    encRC->m_record_init_weight[RC_P_FRAME_LEVEL], 0);

				weight += encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL] * encRC->m_GOP_SPnumleft;
				weight += encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL] * (encRC->m_encRCGOP.m_picLeft - encRC->m_GOP_SPnumleft);

				encRC->m_encRCGOP.m_picTargetBitInGOP[RC_KEY_P_FRAME_LEVEL] =
					RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits * encRC->m_encRCSeq.m_bitsRatio[RC_KEY_P_FRAME_LEVEL], weight));
				encRC->m_encRCGOP.m_picTargetBitInGOP[RC_P_FRAME_LEVEL] =
					RC_CLIP3(1, (int64_t)MAX_INTEGER, division(target_bits * encRC->m_encRCSeq.m_bitsRatio[RC_P_FRAME_LEVEL], weight));
				initRCPic(encRC, &encRC->m_encRCPic, RC_KEY_P_FRAME_LEVEL);

				KP_reset_flag = 1;
			}
			else
		#endif
			{
				TEncRateCtrl_updateKPWeight(encRC);
			}
		}
	#if EVBR_MODE_SWITCH_NEW_METHOD
		if (KP_reset_flag){
			int tmp_KP_rounding;
			int tmp_KP_qp = encRC->m_record_lastQP * 10 - encRC->m_rc_deltaQPkp;
			tmp_KP_rounding = (tmp_KP_qp % 10 > 4) ? 1 : 0;
			lambda = encRC->m_record_lastLambda;
			qp = tmp_KP_qp / 10 + tmp_KP_rounding;
		}
		else{
			lambda = estimatePicLambda(encRC, &encRC->m_encRCPic, 0);
			qp = estimatePicQP(encRC, &encRC->m_encRCPic, lambda, 0);
		}
	#else
		lambda = estimatePicLambda(encRC, &encRC->m_encRCPic, 0);
		qp = estimatePicQP(encRC, &encRC->m_encRCPic, lambda, 0);
	#endif
		encRC->m_currlambda = lambda;
		encRC->m_currQP = qp;
	#if EVBR_MODE_SWITCH_NEW_METHOD
		encRC->m_record_lastQP = qp;
		encRC->m_record_lastLambda = lambda;
	#endif
	}
	#if EVBR_KEEP_PIC_QUALITY
	if (g_MaxKeyFrameQPStep > 0 && RCGetKeppIQualityEn(encRC->m_chn) && encRC->m_encRCPic.m_frameLevel == RC_I_FRAME_LEVEL) {
		int lastLevelQP = encRC->m_lastLevelQP[RC_I_FRAME_LEVEL];
		//DBG_DUMP("last I qp = %d (%d)\n", (int)lastLevelQP, qp);
		if (lastLevelQP > g_RCInvalidQPValue) {
			qp = RC_CLIP3(lastLevelQP - 3, lastLevelQP + g_MaxKeyFrameQPStep, qp);
			encRC->m_currQP = qp;
			#if EVBR_MODE_SWITCH_NEW_METHOD
			encRC->m_record_lastQP = qp;
			#endif
		}
		//DBG_DUMP("I QP = %d: last level qp = %d\n", encRC->m_currQP, lastLevelQP);
	}
	#endif
}

#if HANDLE_BITSTREAM_OVERFLOW
static int updateQPByEncRatio(unsigned int enc_ratio)
{
    unsigned int inc_qp;
    unsigned int cur_ratio = enc_ratio*g_QPOverFactor/100;
    for (inc_qp = 1; inc_qp < g_MaxOverUpdateQP; inc_qp++) {
        if (cur_ratio >= 100)
            break;
        cur_ratio = cur_ratio*g_QPOverFactor/100;
    }
    return inc_qp;
}
static int TEncRateCtrl_updateQuant(H26XEncRC *encRc, int qp, unsigned int enc_ratio)
{
    int overflowIncQP = 0;
    if (enc_ratio && enc_ratio < 100) {
        overflowIncQP = updateQPByEncRatio(enc_ratio);
        if (encRc->m_overflowIncQP > 0)
            qp = encRc->m_activeQP + overflowIncQP;
        else
            qp += overflowIncQP;
        encRc->m_overflowIncQP = overflowIncQP;
        qp = RC_CLIP3(1, 51, qp);
        rc_wrn("{chn%d} force increase qp %d -> %d (ratio = %d)\n", (int)(encRc->m_chn), (int)(encRc->m_activeQP), (int)(qp), (int)(enc_ratio));
        encRc->m_currQP = qp;
        if (H26X_RC_FixQp == encRc->m_rcMode) {
            encRc->m_fixIQp = RC_CLIP3(1, 51, encRc->m_fixIQp+encRc->m_overflowIncQP);
            encRc->m_fixPQp = RC_CLIP3(1, 51, encRc->m_fixPQp+encRc->m_overflowIncQP);
            rc_wrn("{chn%d} force fix qp update %d/%d\n", (int)(encRc->m_chn), (int)(encRc->m_fixIQp), (int)(encRc->m_fixPQp));
        }
    }
    else
        encRc->m_overflowIncQP = 0;
    return qp;
}
#endif

int h26xEnc_RcInit(H26XEncRC *pRc, H26XEncRCParam *pRCParam)
{
	//int min_quant, max_quant;
	int fps_gcd;
	int ret = 0;

    //pRCParam->uiFrameRateIncr = FRAMERATE_BASE;   // because uiFrameRateBase = frame rate * 1000
    pRCParam->HP_period = g_HPPeriod;
	pRCParam->uiMinStillPercent = g_MinStillPercent;

	if (pRCParam->uiRCMode == H26X_RC_EVBR)
		pRCParam->uiChangePos = g_EVBRChangePos;

	if (pRCParam->uiRCMode == H26X_RC_VBR2) {	// Tuba 20200513
		if (0 == pRCParam->uiChangePos || pRCParam->uiChangePos > 100)
			pRCParam->uiChangePos = 100;
	}

    if (pRCParam->uiRCMode < H26X_RC_CBR || pRCParam->uiRCMode > H26X_RC_EVBR) {
        pRCParam->uiRCMode = H26X_RC_FixQp;
        pRCParam->uiFixIQp = pRCParam->uiFixPQp = 32;
    }

	/* check parameter value, force */
	if (0 == pRCParam->uiFrameRateBase || 0 == pRCParam->uiFrameRateIncr) {
		rc_wrn("invalid frame rate = %d / %d, force 30 fps\r\n", (int)(pRCParam->uiFrameRateBase), (int)(pRCParam->uiFrameRateIncr));
		pRCParam->uiFrameRateBase = 30;
		pRCParam->uiFrameRateIncr = 1;
	}

	fps_gcd = division_algorithm(pRCParam->uiFrameRateBase, pRCParam->uiFrameRateIncr);
	if (fps_gcd > 1) {
		pRCParam->uiFrameRateBase /= fps_gcd;
		pRCParam->uiFrameRateIncr /= fps_gcd;
	}

    if (0 == pRCParam->uiPicSize) {
        rc_wrn("invalid pic size %d, force 1080p\r\n", (int)(pRCParam->uiPicSize));
        pRCParam->uiPicSize = 1920*1088;
    }

	pRCParam->uiMinIQp = (pRCParam->uiMinIQp > 51 ? 51 : pRCParam->uiMinIQp);//RC_CLIP3(0, 51, pRCParam->uiMinIQp);
	pRCParam->uiMaxIQp = (pRCParam->uiMaxIQp > 51 ? 51 : pRCParam->uiMaxIQp);//RC_CLIP3(0, 51, pRCParam->uiMaxIQp);
    if (pRCParam->uiMaxIQp < pRCParam->uiMinIQp) {
        rc_wrn("I max qp(%d) is less than min qp(%d), force be (15,51)\r\n", (int)(pRCParam->uiMaxIQp), (int)(pRCParam->uiMinIQp));
        pRCParam->uiMinIQp = 15;
        pRCParam->uiMaxIQp = 51;
    }
    if (pRCParam->uiInitIQp <= 51) {
        pRCParam->uiInitIQp = RC_CLIP3(pRCParam->uiMinIQp, pRCParam->uiMaxIQp, pRCParam->uiInitIQp);
    }
	pRCParam->uiMinPQp = (pRCParam->uiMinPQp > 51 ? 51 : pRCParam->uiMinPQp);//RC_CLIP3(0, 51, pRCParam->uiMinPQp);
	pRCParam->uiMaxPQp = (pRCParam->uiMaxPQp > 51 ? 51 : pRCParam->uiMaxPQp);//RC_CLIP3(0, 51, pRCParam->uiMaxPQp);
    if (pRCParam->uiMaxPQp < pRCParam->uiMinPQp) {
        rc_wrn("P max qp(%d) is less than min qp(%d), force be (15,51)\r\n", (int)(pRCParam->uiMaxPQp), (int)(pRCParam->uiMinPQp));
        pRCParam->uiMinPQp = 15;
        pRCParam->uiMaxPQp = 51;
    }
    pRCParam->uiInitPQp = RC_CLIP3(pRCParam->uiMinPQp, pRCParam->uiMaxPQp, pRCParam->uiInitPQp);
/*
    if (H26X_RC_VBR == pRCParam->uiRCMode) {
        pRCParam->uiMinIQp = pRCParam->uiInitIQp;
        pRCParam->uiMinPQp = pRCParam->uiInitPQp;
    }
*/
    pRCParam->uiFixIQp = ((pRCParam->uiFixIQp > 51) ? 51 : pRCParam->uiFixIQp);
    pRCParam->uiFixPQp = ((pRCParam->uiFixPQp > 51) ? 51 : pRCParam->uiFixPQp);

	if (0 == pRCParam->uiBitRate) {
		rc_wrn("CBR bitarte can not be zero, force default value 2M\r\n");
		pRCParam->uiBitRate = 2000*1000;
	}

    if (0 == pRCParam->uiGOP) {
        pRCParam->uiGOP = pRCParam->uiFrameRateBase/pRCParam->uiFrameRateIncr;
		rc_wrn("not support GOP size is zero, force to be frame rate\r\n");
    }
	
#if FIX_BUG_LTR_INT1
	pRCParam->uiLTRInterval = (pRCParam->uiLTRInterval == 1)? 0: pRCParam->uiLTRInterval;
#endif
#if SMALLGOP_SVCOFF
	//// Ying small GOP and SVC on --> SVCBAMode on
	if ((pRCParam->uiGOP <= g_smllGOPthre) && (pRCParam->uiSVCLayer != 0) && (g_smllGOPthre > 0))
	{
		pRCParam->uiSVCLayer = 0;
	}
	//// Ying
#endif
	pRc->m_chn = pRCParam->uiEncId;

	TEncRateCtrl_init(pRc,
		pRCParam->uiBitRate,
		pRCParam->uiFrameRateBase,
		pRCParam->uiFrameRateIncr,
		pRCParam->uiGOP,
		pRCParam->uiPicSize/256,
		pRCParam->uiKeyPPeriod,
		pRCParam->uiLTRInterval,
		pRCParam->uiSVCLayer
		);

    TEncRateCtrl_set_param(pRc, pRCParam);

	TEncRateCtrl_first(pRc, pRCParam->uiPicSize/256);

    dump_rc_info(pRc);

	return ret;
}

int h26xEnc_RcPreparePicture(TEncRateCtrl *pRc, H26XEncRCPreparePic *pPic)
{
	int qp = 26;
#if EVBR_XA
    int qp_bool;
#endif
    int currPos;

	currPos = pRc->m_encRCGOP.m_numPic - pRc->m_encRCGOP.m_picLeft;

#if MAX_SIZE_BUFFER
	if (pRc->m_glb_max_frm_size) {
		unsigned int max_frame_size;
		if (pPic->uiMaxFrameByte)
			max_frame_size = RC_MIN(pRc->m_glb_max_frm_size, pPic->uiMaxFrameByte);
		else
			max_frame_size = pRc->m_glb_max_frm_size;
		pRc->m_max_frm_bit = division((((int64_t)max_frame_size)<<3)*g_LimitMaxFrameFactor, 100);
	}
	else
		pRc->m_max_frm_bit = division((((int64_t)pPic->uiMaxFrameByte)<<3)*g_LimitMaxFrameFactor, 100);
	if (pRc->m_chn < MAX_LOG_CHN) {
		g_max_frm_bool[pRc->m_chn] = 0;
	}
#endif
    #if HANDLE_DIFF_LTR_KEYP_PERIOD
    pRc->m_ltrFrame = pPic->uiLTRFrame;
    #endif
    if (H26X_RC_EVBR == pRc->m_rcMode && EVBR_STILL_STATE == pRc->m_encRCEvbr.m_evbrState) {
        int currPicPosition = pRc->m_encRCGOP.m_numPic - pRc->m_encRCGOP.m_picLeft;
        if (pRc->m_keyPPeriod > 0 && currPicPosition%pRc->m_keyPPeriod == 0)
            pRc->m_SP_sign = 1;
    }
    else
#if SUPPORT_VBR2
		if (H26X_RC_VBR2 == pRc->m_rcMode && VBR2_VBR_STATE == pRc->m_encRCVbr2.m_vbr2State)
		{
			if (pRc->m_ltrPeriod > 0 && currPos % pRc->m_ltrPeriod == 0)
			{
				pRc->m_LT_sign = 1;
			}
			else if (pRc->m_keyPPeriod > 0 && currPos % pRc->m_keyPPeriod == 0)
			{
				pRc->m_SP_sign = 1;
			}
		}
		else
#endif
    		TEncRateCtrl_prepare(pRc, ((RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType) ? 1 : 0));

    if (pRc->m_ltrPeriod > 0)
    {
        currPos %= (int)pRc->m_ltrPeriod;
    }
	pRc->m_aslog2 = pPic->uiAslog2;

    switch (pRc->m_rcMode) {
    case H26X_RC_CBR:
    case H26X_RC_VBR:
        if (0 == pRc->m_motionAQStrength)
            pPic->uiMotionAQStr = 0;
        else
            pPic->uiMotionAQStr = pRc->m_motionAQStrength;
        if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType) {
			qp = RC_CLIP3((int)pRc->m_minIQp, (int)pRc->m_maxIQp, pRc->m_currQP);
            if (H26X_RC_VBR == pRc->m_rcMode)
                qp = RC_MAX((uint32_t)qp, pRc->m_initIQp);
            if (HMRateControlVBRPreLimit(pRc, 1))
                pPic->uiPredSize = HMRateControlGetPredSize(pRc, 1);
            else
                pPic->uiPredSize = 0;
			#if RC_GOP_QP_LIMITATION
			if (pRc->m_useGopQpLimitation)
			{
				if (pRc->m_GopQpLimit)
				{
					qp = RC_MAX(qp, pRc->m_QpBeforeClip - 10);
					qp = RC_MIN(qp, (int)pRc->m_maxIQp);
					qp = RC_MIN(qp, pRc->m_GopMaxQp);
					qp = RC_MAX(qp, (int)pRc->m_minIQp);
				}
				pRc->m_currQP = qp;
			}
			#endif
            evaluate_frame_qp(pRc, qp, 1);
        }
        else {
            //qp = HMRateControlGetPFrameQP(rc);
            qp = RC_CLIP3((int)pRc->m_minPQp, (int)pRc->m_maxPQp, pRc->m_currQP);
            if (H26X_RC_VBR == pRc->m_rcMode)
                qp = RC_MAX((uint32_t)qp, pRc->m_initPQp);
            if (HMRateControlVBRPreLimit(pRc, 0))
                pPic->uiPredSize = HMRateControlGetPredSize(pRc, 0);
            else
                pPic->uiPredSize = 0;
			#if RC_GOP_QP_LIMITATION
			if (pRc->m_useGopQpLimitation)
			{
				if (pRc->m_GopQpLimit)
				{
					pRc->m_QpBeforeClip = qp;
					qp = RC_MIN(qp, pRc->m_GopMaxQp);
					qp = RC_MAX(qp, (int)pRc->m_minPQp);
				}
				pRc->m_currQP = qp;
			}
			#endif
            evaluate_frame_qp(pRc, qp, 0);
        }
		pPic->uiFrameLevel = HMRateControlGetFrameLevel(pRc, pPic->ucPicType);
        break;
#if SUPPORT_VBR2
	case H26X_RC_VBR2:
        if (0 == pRc->m_motionAQStrength)
            pPic->uiMotionAQStr = 0;
        else
            pPic->uiMotionAQStr = pRc->m_motionAQStrength;
        qp = HMRateControlGetVBR2QP(pRc, pPic, currPos);
		if (g_VBR2LimitMinMaxQP) {
			if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType)
				qp = RC_CLIP3((int)pRc->m_minIQp, (int)pRc->m_maxIQp, qp);
			else
				qp = RC_CLIP3((int)pRc->m_minPQp, (int)pRc->m_maxPQp, qp);
		}
		pPic->uiFrameLevel = HMRateControlGetFrameLevel(pRc, pPic->ucPicType);
		break;
#endif
    case H26X_RC_FixQp:
        pPic->uiMotionAQStr = 0;
        if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType)
            qp = pRc->m_fixIQp;
        else
            qp = pRc->m_fixPQp;
        pPic->uiPredSize = 0;
        break;
    case H26X_RC_EVBR:
        if (EVBR_STILL_STATE == pRc->m_encRCEvbr.m_evbrState) {
            // disable row level rc & set weak MD AQ
            if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType) {
                //qp = pRc->m_encRCEvbr.m_stillIQP;
                pPic->uiMotionAQStr = 0;
                #if EVBR_XA
				pRc->m_encRCPic.m_frameLevel = RC_I_FRAME_LEVEL;
				pRc->m_encRCEvbr.m_last_stillPQP = pRc->m_encRCEvbr.m_stillPQP;
				qp_bool = EVBRStillUpdateQP(pRc, &pRc->m_encRCEvbr, 1);
				#if EVBR_ADJUST_QP
				if (g_EVBRAdjustQPMethod) {
					qp = pRc->m_encRCEvbr.m_stillIQP;
				}
				else 
				#endif
				{
					qp = pRc->m_encRCEvbr.m_last_stillIQP;
					if (qp_bool == 1)
						qp = pRc->m_encRCEvbr.m_tmp2_stillIQP;
					else if (qp_bool == -1)
						qp = pRc->m_encRCEvbr.m_tmp_stillIQP;
					else{
						//if (pRc->m_encRCEvbr.m_stillOverflow){
						if (pRc->m_encRCEvbr.m_stillIadd >0){
							qp = pRc->m_encRCEvbr.m_tmp_stillIQP;
							if (pRc->m_encRCEvbr.m_stillIadd == 2){
								pRc->m_encRCSeq.m_bitsDiff = 0;
								//pRc->m_encRCEvbr.m_stillKeyPadd = 0;
							}
							pRc->m_encRCEvbr.m_stillIadd = 0;
						}
					}
				}
                qp = RC_CLIP3((int)pRc->m_minIQp, (int)pRc->m_maxIQp, qp);
				pRc->m_encRCEvbr.m_last_stillIQP = qp;
                #else
                qp = pRc->m_encRCEvbr.m_stillIQP;
                #endif
            }
            else {
                qp = HMRateControlGetPFrameQP(pRc);
                #if EVBR_XA
                if ( pRc->m_SP_sign){
                    pRc->m_encRCPic.m_frameLevel = RC_KEY_P_FRAME_LEVEL;
                    qp_bool = EVBRStillUpdateQP(pRc, &pRc->m_encRCEvbr, 0);
					#if EVBR_ADJUST_QP
					if (g_EVBRAdjustQPMethod) {
						qp = pRc->m_encRCEvbr.m_stillKeyPQP;
					}
					else
					#endif
					{
	                    qp = pRc->m_encRCEvbr.m_last_stillKeyPQP;
	                    if (qp_bool == 2)
	                        qp = pRc->m_encRCEvbr.m_tmp2_stillKeyPQP;
	                    else if (qp_bool == -1){
	                        qp = pRc->m_encRCEvbr.m_tmp_stillKeyPQP;
	                    }
	                    //else if (pRc->m_encRCEvbr.m_stillOverflow){
	                    else if (pRc->m_encRCEvbr.m_stillKeyPadd >0){
	                        qp = pRc->m_encRCEvbr.m_tmp_stillKeyPQP;
	                        //if (pRc->m_encRCEvbr.m_stillKeyPadd ==1)
	                            pRc->m_encRCEvbr.m_stillKeyPadd = 0;
	                    }
					}
					#if EVBR_ADJUST_QP
					qp = HMRateControlGetPFrameEVBRQP(pRc, qp, 1);
					#endif
                    #if EVBR_XA
                    qp = RC_CLIP3((int)pRc->m_minPQp, (int)pRc->m_maxPQp, qp);
                    #endif
                    pRc->m_encRCEvbr.m_last_stillKeyPQP = qp;
                }
                else{
                    #if EVBR_XA
                    if(pRc->m_svcLayer>0){
						if (0 == currPos % 4)
					       qp=pRc->m_encRCEvbr.m_stillIQP;  //p2
						else if (currPos % 4 == 3 || currPos % 4 == 1)
						   qp=pRc->m_encRCEvbr.m_stillPQP;    //p
						else if (currPos % 4 == 2){
						   if(pRc->m_svcLayer==2)
						      qp=pRc->m_encRCEvbr.m_stillPQP-1;
						   else
						      qp=pRc->m_encRCEvbr.m_stillIQP;
						}
					}
                    else
                        qp = pRc->m_encRCEvbr.m_last_stillPQP;
					#if EVBR_ADJUST_QP
					qp = HMRateControlGetPFrameEVBRQP(pRc, qp, 0);
					#endif
                    qp = RC_CLIP3((int)pRc->m_minPQp, (int)pRc->m_maxPQp, qp);
                    #endif
                    pRc->m_encRCPic.m_frameLevel = RC_P_FRAME_LEVEL;
                }
                #endif
                //if (pRc->m_SP_sign)
                if (pRc->m_SP_sign || (pRc->m_svcLayer == 1 && 0 == currPos % 2) || (pRc->m_svcLayer == 2 && 0 == currPos % 4))
                    pPic->uiMotionAQStr = 0;
                else
                    pPic->uiMotionAQStr = pRc->m_motionAQStrength;
            }
            pPic->uiPredSize = 0;
        }
        else {
            if (RC_I_SLICE == pPic->ucPicType || RC_IDR_SLICE == pPic->ucPicType) {
    			qp = RC_CLIP3((int)pRc->m_minIQp, (int)pRc->m_maxIQp, pRc->m_currQP);
                pPic->uiPredSize = HMRateControlGetPredSize(pRc, 1);
				#if RC_GOP_QP_LIMITATION
				if (pRc->m_useGopQpLimitation
					#if EVBR_KEEP_PIC_QUALITY 
					&& 0 == RCGetKeppIQualityEn(pRc->m_chn)
					#endif
					)
				{
					if (pRc->m_GopQpLimit)
					{
						qp = RC_MAX(qp, pRc->m_QpBeforeClip - 10);
						qp = RC_MIN(qp, (int)pRc->m_maxIQp);
						qp = RC_MIN(qp, pRc->m_GopMaxQp);
						qp = RC_MAX(qp, (int)pRc->m_minIQp);
					}
					pRc->m_currQP = qp;
				}
				#endif
                evaluate_frame_qp(pRc, qp, 1);
            }
            else {
                qp = HMRateControlGetPFrameQP(pRc);
                #if EVBR_XA
                if (pRc->m_still2motion_KP == 1 && pRc->m_still2motion_I == 1 && pRc->m_svcLayer==0) //HK
				//if (pRc->m_still2motion_KP == 1 && pRc->m_still2motion_I == 1)
					qp = RC_CLIP3((int)pRc->m_encRCEvbr.m_stillPQP, (int)pRc->m_maxPQp, qp);
				qp = RC_CLIP3((int)pRc->m_minPQp, (int)pRc->m_maxPQp, qp);
                #endif
                //qp = RC_CLIP3((int)pRc->m_minPQp, (int)pRc->m_maxPQp, qp);
				#if RC_GOP_QP_LIMITATION
				if (pRc->m_useGopQpLimitation)
				{
					if (pRc->m_GopQpLimit)
					{
						pRc->m_QpBeforeClip = qp;
						qp = RC_MIN(qp, pRc->m_GopMaxQp);
						qp = RC_MAX(qp, (int)pRc->m_minPQp);
					}
					pRc->m_currQP = qp;
				}
				#endif
				#if EVBR_ADJUST_QP
				qp = RCUpdateS2MQP(pRc, qp);
				#endif
                evaluate_frame_qp(pRc, qp, 0);
                pPic->uiPredSize = HMRateControlGetPredSize(pRc, 0);
            }
			pPic->uiFrameLevel = HMRateControlGetFrameLevel(pRc, pPic->ucPicType);
        }
        break;
    default:
        pPic->uiPredSize = 0;
        break;
    }
    #if HANDLE_BITSTREAM_OVERFLOW
    qp = TEncRateCtrl_updateQuant(pRc, qp, pPic->uiEncodeRatio);
    #endif
    #if H26X_LOG_RC_INIT_INFO
    h26xEnc_RcSetActiveQPLog(pRc->m_chn, qp, RCGetStillPDQP(pRc->m_chn), pRc->m_encRCEvbr.m_evbrState, pRc->m_motionRatio);
    #endif
    pRc->m_activeQP = qp;

	return qp;
}

int h26xEnc_RcUpdatePicture(H26XEncRC *pRc, H26XEncRCUpdatePic *pUPic)
{
#if RD_LAMBDA_UPDATE //zjz
    int64_t sse;
#endif
#if EVALUATE_BITRATE
    int keyframe;
#endif

#if EVALUATE_BITRATE
	keyframe = (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType ? 1 : 0);
    if (g_EvaluateBRChn == pRc->m_chn && pRc->m_chn < MAX_RC_LOG_CHN)
        evaluation_gop_bitrate(&rc_bitrate[pRc->m_chn], pUPic->uiFrameSize, keyframe, pRc->m_chn);
#endif
    #if RD_LAMBDA_UPDATE //zjz
    sse = (((int64_t)pUPic->uiYMSE[1]) << 32) | pUPic->uiYMSE[0];
    pRc->frame_MSE = sse;
    pRc->MotionRatio = pUPic->uiMotionRatio;
    #endif

    #if EVBR_XA
    if (H26X_RC_EVBR == pRc->m_rcMode && 0 == pRc->m_encRCEvbr.m_startpic){
        //TEncRateCtrl_changeStillState(pRc, &pRc->m_encRCEvbr, pUPic);
        pRc->m_encRCEvbr.m_startpic = 1;
    }
    #endif

    // output still flag
    if (H26X_RC_EVBR == pRc->m_rcMode) {
        if (EVBR_STILL_STATE == pRc->m_encRCEvbr.m_evbrState)
            pUPic->bEVBRStillFlag = 1;
        else
            pUPic->bEVBRStillFlag = 0;
    }
    else
        pUPic->bEVBRStillFlag = 0;

    if (H26X_RC_FixQp == pRc->m_rcMode)
        goto exit_update;
    pUPic->uiUpdate = 0;
    pRc->m_motionRatio = pUPic->uiMotionRatio;

#if EVBR_MEASURE_BR_METHOD
	if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType){ // record last I size
		pRc->m_lastIsize = pUPic->uiFrameSize << 3;
	}
    #if HANDLE_DIFF_LTR_KEYP_PERIOD
    else {
        if (pRc->m_ltrPeriod > 0) {
            if (pRc->m_ltrFrame)
                pRc->m_lastKeyPsize = pUPic->uiFrameSize << 3;
        }
        else {
            if (pRc->m_SP_sign)
        		pRc->m_lastKeyPsize = pUPic->uiFrameSize << 3;
        }
    }
    #else   // HANDLE_DIFF_LTR_KEYP_PERIOD
	else if (pRc->m_SP_sign){ // record last Special P size
		pRc->m_lastKeyPsize = pUPic->uiFrameSize << 3;
	}
    #endif  // HANDLE_DIFF_LTR_KEYP_PERIOD
#endif  // EVBR_MEASURE_BR_METHOD
#if SUPPORT_VBR2
	if (H26X_RC_VBR2 == pRc->m_rcMode)
	{
		if (RC_I_SLICE == pUPic->ucPicType || RC_IDR_SLICE == pUPic->ucPicType)
		{
			pRc->m_encRCVbr2.m_lastISize = pUPic->uiFrameSize << 3;
		}
		else if (pRc->m_ltrPeriod > 0 && pRc->m_LT_sign)
		{
			pRc->m_encRCVbr2.m_lastLTPSize = pUPic->uiFrameSize << 3;
		}
		else if (pRc->m_keyPPeriod > 0 && pRc->m_SP_sign)
		{
			pRc->m_encRCVbr2.m_lastKPSize = pUPic->uiFrameSize << 3;
		}
		else
		{
			if (pRc->m_svcLayer > 0)
			{
				if (RC_P2_FRAME_LEVEL == pRc->m_encRCPic.m_frameLevel)
				{
					pRc->m_encRCVbr2.m_lastP2Size >>= 1;
					pRc->m_encRCVbr2.m_lastP2Size += pUPic->uiFrameSize << 2;
					pRc->m_encRCVbr2.m_lastP2Size >>= 1;
				}
				else if (RC_P3_FRAME_LEVEL == pRc->m_encRCPic.m_frameLevel)
				{
					pRc->m_encRCVbr2.m_lastP3Size >>= 1;
					pRc->m_encRCVbr2.m_lastP3Size += pUPic->uiFrameSize << 2;
					pRc->m_encRCVbr2.m_lastP3Size >>= 1;
				}
				else
				{
					pRc->m_encRCVbr2.m_lastPSize >>= 1;
					pRc->m_encRCVbr2.m_lastPSize += pUPic->uiFrameSize << 2;
					pRc->m_encRCVbr2.m_lastPSize >>= 1;
				}
			}
			else
			{
				pRc->m_encRCVbr2.m_lastPSize >>= 1;
				pRc->m_encRCVbr2.m_lastPSize += pUPic->uiFrameSize << 2;
				pRc->m_encRCVbr2.m_lastPSize >>= 1;
			}
		}
	}
#endif
    if (H26X_RC_EVBR == pRc->m_rcMode && EVBR_STILL_STATE == pRc->m_encRCEvbr.m_evbrState) {
        TEncRateCtrl_evbrUpdate(pRc, &pRc->m_encRCEvbr, pUPic);
    }
    else
    {
    #if SUPPORT_VBR2
		if (H26X_RC_VBR2 == pRc->m_rcMode && VBR2_VBR_STATE == pRc->m_encRCVbr2.m_vbr2State)
		{
			TEncRateCtrl_Vbr2Update(pRc, &pRc->m_encRCVbr2, pUPic);
		}
		else
    #endif
		{
			TEncRateCtrl_update(pRc,
				pUPic->uiFrameSize,
    			pUPic->uiAvgQP,
				pUPic->ucPicType);
			// check motion
			if (H26X_RC_EVBR == pRc->m_rcMode)
				TEncRateCtrl_checkStill(pRc, &pRc->m_encRCEvbr, pUPic);
		#if SUPPORT_VBR2
			if (H26X_RC_VBR2 == pRc->m_rcMode)
			{
				TEncRateCtrl_Vbr2Update(pRc, &pRc->m_encRCVbr2, pUPic);
			}
		#endif
		}
    }
    pRc->m_lastFrameSize = pUPic->uiFrameSize;
    pRc->m_lastSliceType = (uint32_t)pUPic->ucPicType;

exit_update:
    return 0;
}

int h26xEnc_RcGetLog(H26XEncRC *pRc, unsigned int *log_addr)
{
    *log_addr = (unsigned int)(&pRc->rc_log_info);
    return sizeof(pRc->rc_log_info);
}

/********** proc info **********/
#if H26X_RC_LOG_VERSION
int h26xEnc_getRcVersion(char *str)
{
    int len = 0;
    if (H26X_RC_VERSION&0xFF) {
        len += sprintf(str+len, "H26xenc rate control version v%d.%d.%d.%d",
            (H26X_RC_VERSION>>24)&0xFF, (H26X_RC_VERSION>>16)&0xFF, (H26X_RC_VERSION>>8)&0xFF, H26X_RC_VERSION&0xFF);
    }
    else {
        len += sprintf(str+len, "H26xenc rate control version v%d.%d.%d",
            (H26X_RC_VERSION>>24)&0xFF, (H26X_RC_VERSION>>16)&0xFF, (H26X_RC_VERSION>>8)&0xFF);
    }
    return len;
}
#endif
#if H26X_LOG_RC_INIT_INFO
static int h26xEnc_RcSetInitInfo(TEncRateCtrl *encRC)
{
    H26XEncRCInitInfo *pInfo;
    if (encRC->m_chn < 0 || encRC->m_chn >= MAX_LOG_CHN)
        goto exit_set;

    pInfo = &g_RCInitInfo[encRC->m_chn];
    //pInfo->chn = encRC->m_chn;
    pInfo->rc_mode = encRC->m_rcMode;
    pInfo->gop = encRC->m_GOPSize;
    pInfo->kp_period = encRC->m_keyPPeriod;
    pInfo->framerate_base = encRC->m_frameRateBase;
    pInfo->framerate_incr = encRC->m_frameRateIncr;
    pInfo->gop_bitrate = encRC->m_GOPbitrate;
	if (H26X_RC_FixQp == encRC->m_rcMode) {
		pInfo->init_i_qp = encRC->m_fixIQp;
		pInfo->init_p_qp = encRC->m_fixPQp;
	}
	else {
		pInfo->init_i_qp = encRC->m_initIQp;
		pInfo->init_p_qp = encRC->m_initPQp;
	}
    pInfo->min_i_qp = encRC->m_minIQp;
    pInfo->max_i_qp = encRC->m_maxIQp;
    pInfo->min_p_qp = encRC->m_minPQp;
    pInfo->max_p_qp = encRC->m_maxPQp;
    pInfo->smooth_win_size = encRC->m_smoothWindowSize;
    pInfo->ip_weight = encRC->m_rc_deltaQPip;
    pInfo->kp_weight = encRC->m_rc_deltaQPkp;
    pInfo->still_i_qp = encRC->m_encRCEvbr.m_stillIQP;
    pInfo->still_kp_qp = encRC->m_encRCEvbr.m_stillKeyPQP;
    pInfo->still_p_qp = encRC->m_encRCEvbr.m_stillPQP;
    pInfo->change_pos = encRC->m_vbrChangePos;
    pInfo->active_qp = encRC->m_initIQp;
    pInfo->motion_aq_str = encRC->m_motionAQStrength;
	pInfo->max_frame_size = encRC->m_glb_max_frm_size;
	pInfo->br_tolerance = encRC->m_BrTolerance;
exit_set:
    return 0;
}
static int h26xEnc_RcSetActiveQPLog(int chn, int qp, int dqp, unsigned int evbr_state, unsigned int motion_ratio)
{
    if (chn < 0 || chn >= MAX_LOG_CHN)
        goto exit_set;
    g_RCInitInfo[chn].active_qp = qp;
	g_RCInitInfo[chn].still_dqp = dqp;
	if (EVBR_STILL_STATE == evbr_state)
		g_RCInitInfo[chn].evbr_state_char = 's';
	else
		g_RCInitInfo[chn].evbr_state_char = 'm';
	g_RCInitInfo[chn].motion_ratio = motion_ratio;
exit_set:
    return 0;
}

int h26xEnc_RcGetInitInfo(int chn, H26XEncRCInitInfo *pInfo)
{
    int ret = 0;
    if (chn < 0 || chn >= MAX_LOG_CHN) {
		ret = -1;
        goto exit_get;
    }
    if (0 == g_RCInitInfo[chn].rc_mode) {
		ret = -1;
        goto exit_get;
    }
	memcpy(pInfo, &g_RCInitInfo[chn], sizeof(H26XEncRCInitInfo));

exit_get:
    return ret;
}
#endif
#if H26X_RC_BR_MEASURE
int h26xEnc_RcSetDumpBR(int log_chn)
{
    g_EvaluateBRChn = log_chn;
    return 0;
}
#endif

/********** proc log level **********/
#if H26X_RC_DBG_LEVEL
int h26xEnc_RcSetDbgLv(unsigned int uiDbgLv)
{
    g_uiDbgLv = uiDbgLv;
    return 0;
}

int h26xEnc_RcGetDbgLv(unsigned int *uiDbgLv)
{
	*uiDbgLv = g_uiDbgLv;
    return 0;
}
#endif

/********** proc cmd **********/
#if H26X_RC_DBG_CMD
#define STX_UINT32  1
#define STX_INT32   2

typedef struct rc_mapping_st
{
    char *tokenName;
    void *value;
    int type;   // 0: uint32_t, 1: int32_t
    int default_value;
    int lb;
    int ub;
    char *note;
} RCMapInfo;

static const RCMapInfo rc_param_syntax[] = {
    {"OverQPFactor",        &g_QPOverFactor,        STX_UINT32, 110,    100,    300,    "overflow qp factor"},
    {"OverMaxQP",           &g_MaxOverUpdateQP,     STX_UINT32, 10,     1,      51,     "overflow max qp"},
    {"MaxFrmFactor",        &g_LimitMaxFrameFactor, STX_UINT32, 90,     1,      100,    "factor of limited max frame"},
    {"AdjustQPWeight",      &g_AdjustQPWeight,      STX_UINT32, 0,      0,      1,      "adjust qp weight"},
    //{"LTWeight",            &g_LTWeight,            STX_INT32,  0,      -100,   100,    "LT QP weight"},
    //{"P2Weight",            &g_P2Weight,            STX_INT32,  10,     -100,   100,    "P2 QP weight"},
    //{"P3Weight",            &g_P3Weight,            STX_INT32,  10,     -100,   100,    "P3 QP weight"},
    {"CBRWeight",           &g_RCPicGOPWeight_cbr,  STX_INT32,  60,     0,      100,    "cbr pic-gop gop weight"},
    {"VBRWeight",           &g_RCPicGOPWeight_vbr,  STX_INT32,  80,     0,      100,    "vbr pic-gop gop weight"},
    {"EVBRWeight",          &g_RCPicGOPWeight_evbr, STX_INT32,  10,     0,      100,    "evbr pic-gop gop weihgt"},
    {"MSBDisLimitQP",		&g_MSBDisableLimitQP,	STX_UINT32,	1,		0,		1,		"limit buffer size disable limit qp"},
    #if ADJUST_SVC_CALWEIGHT_FREQCY
    {"HPPeriod",            &g_HPPeriod,            STX_UINT32, 0,      0,      4096,   "SVC weight update period"},
	#else
    {"HPPeriod",            &g_HPPeriod,            STX_UINT32, 4,      4,      4096,   "SVC weight update period"},
	#endif
#if LIMIT_GOP_DIFFERNT_BIT
	{"MaxGOPDiff",		    &g_MaxGOPDiffFactor,	STX_UINT32,	8,		0,		16,		"max GOP different bit"},
	{"MinGOPDiff",		    &g_MinGOPDiffFactor,	STX_UINT32,	8,		0,		16,		"min GOP different bit"},
#endif
#if SMALLGOP_SVCOFF
	{"SmllGOPthre", 		&g_smllGOPthre, 		STX_INT32, 15,	   -1,		1000,	"small GOP threashold"},
#endif
    {"EVBRChgPos",          &g_EVBRChangePos,       STX_UINT32, 100,    1,      300,    "EVBR change pos"},
    {"EVBRMinPct",          &g_MinStillPercent,     STX_UINT32, 35,     1,      100,    "EVBR minimal still percent"},
    {"EVBRLmtQP",           &g_LimitEVBRStillQP,    STX_UINT32, 1,      0,      51,     "EVBR limit updated still qp"},
#if EVBR_ADJUST_QP
	{"EVBRAdjustMethod",	&g_EVBRAdjustQPMethod,	STX_UINT32,	1,		0,		1,		"EVBR adjust still qp method"},
	{"EVBRPreLimitMPct",	&g_EVBRPreLimitMPct,	STX_UINT32, 50,		1,		100,	"EVBR prelimit middle threshold"},
	{"EVBRPreLimitHPct",	&g_EVBRPreLimitHPct,	STX_UINT32, 75,		1,		100,	"EVBR prelimit high threshold"},
	{"EVBRStillMiddleDQP",	&g_EVBRStillMidDQP,		STX_UINT32,	1,		0,		51,		"EVBR still middle delta qp"},
	{"EVBRStillHighDQP",	&g_EVBRStillHighDQP,	STX_UINT32, 2,		0,		51,		"EVBR still high delta qp"},
	{"EVBRS2MCnt",			&g_EvbrS2MCnt,			STX_UINT32,	1,		0,		4095,	"EVBR still to motion limit conter"},
	{"EVBRMinDeltaQP",		&g_minDeltaStillPQP,	STX_INT32,	-4,		-15,	0,		"EVBR minimal delta qp"},
#endif    
#if EVBR_KEEP_PIC_QUALITY
	{"EVBRGOPThd",			&g_LargeGOPThd,			STX_UINT32,	10,		0,		100,	"EVBR GOP threshold"},
	{"EVBRMaxKeyQPStep",	&g_MaxKeyFrameQPStep,	STX_UINT32,	1,		0,		6,		"EVBR max key frame qp step"},
	//{"EVBRMaxKeyQPDiff",	&g_EVBRMaxIQPDiff,		STX_UINT32,	12,		0,		51,		"EVBR max key frame qp difference"},
	{"EVBRDisKeyRRC",		&g_DisKeyFrameRRC,		STX_UINT32,	1,		0,		1,		"EVBR key frame RRC disable"},
#endif
#if ALPHA_BETA_MIN_MAX_ISSUE
	{"RCUpdateAlphaCnd",	&g_RCUpdateAlphaCndEn,	STX_UINT32,	1,		0,		1,		"update alpha condition"},
#endif
#if ALPHA_BETA_RESET_COD_ADJUST
	{"ResetAlphaMinVal",	&g_ResetAlphaMinVal,	STX_INT32,	150,	101,	1024000,		"Reset alpha min value"},
	{"ResetBetaMaxVal", 	&g_ResetBetaMaxVal, 	STX_INT32,	-250,	-6145,	-205,			"Reset beta max value"},
#endif
#if ALPHA_CONVERG_DIRECT
	#if ALPHA_CONVERG_DIRECT_TH_ADJ
	{"AlphaConvergCntThr",	&g_AlphaConvergCntThr,	STX_INT32,	15, 	0,		10000,	"Alpha convergence threshold"},
	#else
	{"AlphaConvergCntThr",	&g_AlphaConvergCntThr,	STX_INT32,	30, 	0,		10000,	"Alpha convergence threshold"},
	#endif
#endif
#if ADUST_VBR2_OVERFLOW_CHECK
	{"VBR2EstPeriodMin",	&g_VBR2EstPeriodMin,	STX_INT32,	15, 	0,		100,	"VBR2 bitrate estimate period min value"},
	{"VBR2EstPeriodMax",	&g_VBR2EstPeriodMax,	STX_INT32,	30, 	0,		1000,	"VBR2 bitrate estimate period min value"},
	{"VBR2UndQPThd",		&g_VBR2UndQPThd,		STX_INT32,	45, 	10, 	51, 	"VBR2 underflow QP threshold"},
#endif
#if SUPPORT_VBR2
	{"VBR2LimitQPEn",		&g_VBR2LimitMinMaxQP,	STX_UINT32, 1,		0,		1,		"Enable Limit QP for VBR2"},
#endif
    {NULL,                  NULL,                   0,          0,      0,      0,      NULL}
};

int h26xEnc_RcGetCmd(int cmd_idx, char *cmd)
{
    int len = 0;
    if (cmd_idx < 0 || cmd_idx >= (int)(sizeof(rc_param_syntax)/sizeof(RCMapInfo)))
        goto exit_get;
    if (NULL == rc_param_syntax[cmd_idx].tokenName)
        goto exit_get;

    len += sprintf(cmd+len, "%-20s  ", rc_param_syntax[cmd_idx].tokenName);
    switch (rc_param_syntax[cmd_idx].type) {
    case STX_UINT32:
        len += sprintf(cmd+len, " %3u   ", *((unsigned int *)rc_param_syntax[cmd_idx].value));
        break;
    case STX_INT32:
        len += sprintf(cmd+len, " %3d   ", *((signed int *)rc_param_syntax[cmd_idx].value));
        break;
    default:
        len = 0;
		goto exit_get;
        break;
    }
    len += sprintf(cmd+len, "%s (range: %d ~ %d)", rc_param_syntax[cmd_idx].note, rc_param_syntax[cmd_idx].lb, rc_param_syntax[cmd_idx].ub);

exit_get:
    return len;
}

int h26xEnc_RcSetCmd(char *str)
{
    int i, idx = -1;
    int value;
    char cmd_str[0x80];
    unsigned int *u32Val;
    int *i32Val;
	int ret = 0;

    sscanf(str, "%s %d\n", cmd_str, &value);

    for (i = 0; i < (int)(sizeof(rc_param_syntax)/sizeof(RCMapInfo)); i++) {
        if (NULL == rc_param_syntax[i].tokenName)
            break;
        if (strcmp(rc_param_syntax[i].tokenName, cmd_str) == 0) {
            idx = i;
            break;
        }
    }
    if (idx >= 0) {
        if (value < rc_param_syntax[idx].lb || value > rc_param_syntax[idx].ub) {
            rc_dump_info("%s value (%d) is out of range! (%d ~ %d)\n", rc_param_syntax[idx].tokenName, value, rc_param_syntax[idx].lb, rc_param_syntax[idx].ub);
            ret = -1;
            goto exit_set;
        }
        else {
            switch (rc_param_syntax[idx].type) {
            case STX_UINT32:
                u32Val = (unsigned int *)rc_param_syntax[idx].value;
                *u32Val = (unsigned int)value;
                break;
            case STX_INT32:
                i32Val = (int *)rc_param_syntax[idx].value;
                *i32Val = value;
                break;
            default:
                break;
            }
        }
    }
    else {
        rc_dump_info("unknown \"%s\"\n", cmd_str);
		ret = -1;
		goto exit_set;
    }
exit_set:
    return ret;
}
#endif

