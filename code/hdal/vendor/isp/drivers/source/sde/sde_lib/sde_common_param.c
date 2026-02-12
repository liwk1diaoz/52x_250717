#include "sde_alg.h"
#include "sde_lib.h"

SDE_PARAM sde_param_init = {
	.bScanEn = TRUE,                             ///< SDE scanning functionality enable
	.bDiagEn = FALSE,                            ///< SDE Diag LRC functionaltiy enable
	.bInput2En = FALSE,                          ///< input2 enabled
	.bCostCompMode = TRUE,                       ///< SDE cost volume computation mode (flip-orN)
	.bHflip01 = TRUE,                            ///< SDE mirror 01
	.bVflip01 = TRUE,                            ///< SDE flip 01
	.bHflip2 = FALSE,                            ///< SDE mirror 2
	.bVflip2 = FALSE,                            ///< SDE flip 2
	.cond_disp_en = FALSE,                       ///< SDE conditional disparity enable
	.conf_out2_en = FALSE,                       ///< SDE confidence output enable
	.disp_val_mode = FALSE,                      ///< SDE disparity value mode. 0: out_disp = disp << 2 (6b in 8b MSB ) ; 1: out_disp = disp (6b in 8b LSB )
	.opMode = SDE_MAX_DISP_63,                   ///< SDE opertaion mode, disaprity range selection.
	.ioPara = {                                  ///< input output related parameters
		.Size = {                                ///< input size and offset information
			.uiWidth = 1280,                     ///< image width
			.uiHeight = 720,                     ///< image height
			.uiOfsi0 = 1280,                     ///< image input lineoffset of input 0
			.uiOfsi1 = 1280,                     ///< image input lineoffset of input 1
			.uiOfsi2 = 1280 * 48,                ///< image input lineoffset of input 2
			.uiOfso = 1280,                      ///< image output lineoffset
			.uiOfso2 = 1280,                     ///< image output2 lineoffset
		},
		.uiInAddr0 = 0,                          ///< input starting address of input 0
		.uiInAddr1 = 0,                          ///< input starting address of input 1
		.uiInAddr2 = 0,                          ///< input starting address of input 2
		.uiOutAddr = 0,                          ///< output starting address
		.uiOutAddr2 = 0,                         ///< output 2 starting address
		.uiInAddr_link_list = 0,                 ///<  Link list address
	},
	.invSel = SDE_INV_0,                         ///< invalid value selection
	.outSel = SDE_OUT_VOL,                       ///< engine output selection
	.outVolSel = SDE_OUT_SCAN_VOL,               ///< select which kind of inner volume result to output for outSel
	.conf_min2_sel = SDE_CONFIDENCE_LOCAL,
	.conf_out2_mode = SDE_CONFIDENCE_ONLY,
	.burstSel = SDE_BURST_64W,                   ///< burst length selection

	.uiIntrEn = TRUE,                            ///< interrupt enable

	.costPara = {                                ///< cost computation related parameters
		.uiAdBoundUpper = 15,                    ///< upper bound value of absolute difference
		.uiAdBoundLower = 2,                     ///< lower bound value of absolute difference
		.uiCensusBoundUpper = 20,                ///< upper bound value of census bit code
		.uiCensusBoundLower = 5,                 ///< lower bound value of census bit code
		.uiCensusAdRatio = 153,                  ///< weighted ratio with absolute difference on luminance
	},

	.lumaPara = {
		.uiLumaThreshSaturated = 252,            ///< threshold to determine saturated pixel
		.uiCostSaturatedMatch = 15,              ///< cost value assigned to saturated right image point encountered
		.uiDeltaLumaSmoothThresh = 40,           ///< threshold to let data term cost dominate
	},
	.penaltyValues = {                           ///< scan penalty values
		.uiScanPenaltyNonsmooth = 8,             ///< smallest penalty for big luminance difference
		.uiScanPenaltySmooth0 = 8,               ///< small penalty for small disparity change
		.uiScanPenaltySmooth1 = 22,              ///< small penalty for middle disparity change
		.uiScanPenaltySmooth2 = 30,              ///< small penalty for big disparity change
	},
	.thSmooth = {                                ///< scan threshold of penlaties assigned
		.uiDeltaDispLut0 = 2,                    ///< Threshold to remove smooth penalty
		.uiDeltaDispLut1 = 5,                    ///< Threshold to give small penalty
	},
	.confidence_th = 128,                        ///< threshold of confidence level

	.thDiag = {                                  ///< Threshold for LRC
		.uiDiagThreshLut0 = 1,                   ///< diagonal search threshold look-up table 0
		.uiDiagThreshLut1 = 1,                   ///< diagonal search threshold look-up table 1
		.uiDiagThreshLut2 = 2,                   ///< diagonal search threshold look-up table 2
		.uiDiagThreshLut3 = 3,                   ///< diagonal search threshold look-up table 3
		.uiDiagThreshLut4 = 3,                   ///< diagonal search threshold look-up table 4
		.uiDiagThreshLut5 = 6,                   ///< diagonal search threshold look-up table 5
		.uiDiagThreshLut6 = 9,                   ///< diagonal search threshold look-up table 6
	},
};

