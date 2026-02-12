/*
    PLL Configuration module

    Set the clock frequency and clock enable/disable of each module

    @file       pll.c
    @ingroup    mIDrvSys_CG
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <kwrap/error_no.h>
#include <kwrap/spinlock.h>
#include <kwrap/util.h>
#include <stdio.h>
#include "top.h"
#include "pll.h"
#include "pll_protected.h"
#include "pll_reg.h"
#include "pll_int.h"
//#include "clock.h"
//#include "Debug.h"
//#include "SxCmd.h"

#define __MODULE__    rtos_pll
#include <kwrap/debug.h>
unsigned int rtos_pll_debug_level = NVT_DBG_WRN;


static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)



#define loc_multi_cores(flags)   loc_cpu(flags)
#define unl_multi_cores(flags)   unl_cpu(flags)

//static void pll_install_cmd(void);

static UINT32 siemclk_opencnt[3];

/**
    @addtogroup mIDrvSys_CG
*/
//@{

#if 1
//static const PLLCLKCHECK pll_check_item_dummy[] = {};

static const PLLCLKCHECK pll_check_item_pll3[] = {
	{DMA_CLK,       PLL_CLKSEL_MAX_ITEM,    PLL_CLKSEL_MAX_ITEM,            "DMA"},
};

static const PLLCLKCHECK pll_check_item_pll4[] = {
	{IDE1_CLK,      PLL_CLKSEL_IDE_CLKSRC,  PLL_CLKSEL_IDE_CLKSRC_PLL4,     "IDE"},
	{SDIO_CLK,      PLL_CLKSEL_SDIO,        PLL_CLKSEL_SDIO_PLL4,           "SDIO"},
	{SDIO2_CLK,     PLL_CLKSEL_SDIO2,       PLL_CLKSEL_SDIO2_PLL4,          "SDIO2"},
	{SDIO3_CLK,     PLL_CLKSEL_SDIO3,       PLL_CLKSEL_SDIO3_PLL4,          "SDIO3"},
	{SP_CLK,        PLL_CLKSEL_SP,          PLL_CLKSEL_SP_PLL4,             "SP"},
	{SP2_CLK,       PLL_CLKSEL_SP2,         PLL_CLKSEL_SP2_PLL4,            "SP2"},
	{TRNG_RO_CLK,   PLL_CLKSEL_TRNGRO_CLKSRC, PLL_CLKSEL_TRNGRO_CLKSRC_PLL4, "TRNG_RO"},
};

static const PLLCLKCHECK pll_check_item_pll5[] = {
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL5,     "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL5,    "SIE2"},
	{SIE_MCLK,      PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL5,    "SIE MCLK"},
	{SIE_MCLK2,     PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL5,  "SIE MCLK2"},
	{SP_CLK,        PLL_CLKSEL_SP,          PLL_CLKSEL_SP_PLL5,             "SP"},
	{SP2_CLK,       PLL_CLKSEL_SP2,         PLL_CLKSEL_SP2_PLL5,            "SP2"},
};

static const PLLCLKCHECK pll528_check_item_pll5[] = {
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL5,     "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL5,    "SIE2"},
	{SIE_MCLK,      PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL5,    "SIE MCLK"},
	{SIE_MCLK2,     PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL5,  "SIE MCLK2"},
	{SP_CLK,        PLL_CLKSEL_SP,          PLL_CLKSEL_SP_PLL5,             "SP"},
	{SP2_CLK,       PLL_CLKSEL_SP2,         PLL_CLKSEL_SP2_PLL5,            "SP2"},
	{SIE3_CLK,      PLL_CLKSEL_SIE3_CLKSRC, PLL_CLKSEL_SIE3_CLKSRC_PLL5,    "SIE3"},
	{SIE4_CLK,      PLL_CLKSEL_SIE4_CLKSRC, PLL_CLKSEL_SIE4_CLKSRC_PLL5,    "SIE4"},
	{SIE5_CLK,      PLL_CLKSEL_SIE5_CLKSRC, PLL_CLKSEL_SIE5_CLKSRC_PLL5,    "SIE5"},
	{SIE_MCLK3,     PLL_CLKSEL_SIE_MCLK3SRC,PLL_CLKSEL_SIE_MCLK3SRC_PLL5,   "SIE MCLK3"},
};

static const PLLCLKCHECK pll_check_item_pll6[] = {
	{IDE1_CLK,      PLL_CLKSEL_IDE_CLKSRC,  PLL_CLKSEL_IDE_CLKSRC_PLL6,     "IDE"},
	{SP_CLK,        PLL_CLKSEL_SP,          PLL_CLKSEL_SP_PLL6,             "SP"},
	{SP2_CLK,       PLL_CLKSEL_SP2,         PLL_CLKSEL_SP2_PLL6,            "SP2"},
	{ETH_CLK,       PLL_CLKSEL_ETH,         PLL_CLKSEL_ETH_PLL6,            "ETH"},
};

static const PLLCLKCHECK pll528_check_item_pll6[] = {
	{IDE1_CLK,      PLL_CLKSEL_IDE_CLKSRC,  PLL_CLKSEL_IDE_CLKSRC_PLL6,     "IDE"},
	{SP_CLK,        PLL_CLKSEL_SP,          PLL_CLKSEL_SP_PLL6,             "SP"},
	{SP2_CLK,       PLL_CLKSEL_SP2,         PLL_CLKSEL_SP2_PLL6,            "SP2"},
};

static const PLLCLKCHECK pll_check_item_pll7[] = {};

static const PLLCLKCHECK pll_check_item_pll8[] = {
	{0,             PLL_CLKSEL_CPU,         PLL_CLKSEL_CPU_PLL8,            "CPU"},

};

static const PLLCLKCHECK pll_check_item_pll9[] = {
	{IDE1_CLK,      PLL_CLKSEL_IDE_CLKSRC,  PLL_CLKSEL_IDE_CLKSRC_PLL9,    "IDE"},
	{ETH_CLK,       PLL_CLKSEL_ETH,         PLL_CLKSEL_ETH_PLL9,           "ETH"},
	{JPG_CLK,       PLL_CLKSEL_JPEG,        PLL_CLKSEL_JPEG_PLL9,          "JPEG"},
	{TSE_CLK,       PLL_CLKSEL_TSE,         PLL_CLKSEL_TSE_PLL9,           "TSE"},
	{CRYPTO_CLK,    PLL_CLKSEL_CRYPTO,      PLL_CLKSEL_CRYPTO_PLL9,        "CRYPTO"},
	{HASH_CLK,      PLL_CLKSEL_HASH,        PLL_CLKSEL_HASH_PLL9,          "HASH"},
	{RSA_CLK,       PLL_CLKSEL_RSA,         PLL_CLKSEL_RSA_PLL9,           "RSA"},
};

static const PLLCLKCHECK pll528_check_item_pll9[] = {
	{IDE1_CLK,      PLL_CLKSEL_IDE_CLKSRC,  PLL_CLKSEL_IDE_CLKSRC_PLL9,    "IDE"},
	{JPG_CLK,       PLL_CLKSEL_JPEG,        PLL_CLKSEL_JPEG_PLL9,          "JPEG"},
	{TSE_CLK,       PLL_CLKSEL_TSE,         PLL_CLKSEL_TSE_PLL9,           "TSE"},
	{CRYPTO_CLK,    PLL_CLKSEL_CRYPTO,      PLL_CLKSEL_CRYPTO_PLL9,        "CRYPTO"},
	{HASH_CLK,      PLL_CLKSEL_HASH,        PLL_CLKSEL_HASH_PLL9,          "HASH"},
	{RSA_CLK,       PLL_CLKSEL_RSA,         PLL_CLKSEL_RSA_PLL9,           "RSA"},
};

static const PLLCLKCHECK pll_check_item_pll10[] = {
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL10,    "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL10,   "SIE2"},
	{SIE_MCLK,      PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL10,   "SIE MCLK"},
	{SIE_MCLK2,     PLL_CLKSEL_SIE_MCLK2SRC,PLL_CLKSEL_SIE_MCLK2SRC_PLL10,  "SIE MCLK2"},
	{CNN_CLK,       PLL_CLKSEL_CNN,         PLL_CLKSEL_CNN_PLL10,           "CNN"},
	{CNN2_CLK,      PLL_CLKSEL_CNN2,        PLL_CLKSEL_CNN2_PLL10,          "CNN2"},
	{NUE_CLK,       PLL_CLKSEL_NUE,         PLL_CLKSEL_NUE_PLL10,           "NUE"},
};

static const PLLCLKCHECK pll528_check_item_pll10[] = {
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL10,    "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL10,   "SIE2"},
	{SIE_MCLK,      PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL10,   "SIE MCLK"},
	{SIE_MCLK2,     PLL_CLKSEL_SIE_MCLK2SRC,PLL_CLKSEL_SIE_MCLK2SRC_PLL10,  "SIE MCLK2"},
	{CNN_CLK,       PLL_CLKSEL_CNN,         PLL_CLKSEL_CNN_PLL10,           "CNN"},
	{CNN2_CLK,      PLL_CLKSEL_CNN2,        PLL_CLKSEL_CNN2_PLL10,          "CNN2"},
	{NUE_CLK,       PLL_CLKSEL_NUE,         PLL_CLKSEL_NUE_PLL10,           "NUE"},
	{SIE3_CLK,      PLL_CLKSEL_SIE3_CLKSRC, PLL_CLKSEL_SIE3_CLKSRC_PLL10,   "SIE3"},
	{SIE4_CLK,      PLL_CLKSEL_SIE4_CLKSRC, PLL_CLKSEL_SIE4_CLKSRC_PLL10,   "SIE4"},
	{SIE5_CLK,      PLL_CLKSEL_SIE5_CLKSRC, PLL_CLKSEL_SIE5_CLKSRC_PLL10,   "SIE5"},
};

static const PLLCLKCHECK pll_check_item_pll11[] = {};

static const PLLCLKCHECK pll_check_item_pll12[] = {
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL12,     "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL12,    "SIE2"},
	{SIE_MCLK,      PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL12,    "SIE MCLK"},
	{SIE_MCLK2,     PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL12,  "SIE MCLK2"},
};

static const PLLCLKCHECK pll528_check_item_pll12[] = {
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL12,     "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL12,    "SIE2"},
	{SIE3_CLK,      PLL_CLKSEL_SIE3_CLKSRC, PLL_CLKSEL_SIE3_CLKSRC_PLL12,    "SIE3"},
	{SIE4_CLK,      PLL_CLKSEL_SIE4_CLKSRC, PLL_CLKSEL_SIE4_CLKSRC_PLL12,    "SIE4"},
	{SIE5_CLK,      PLL_CLKSEL_SIE5_CLKSRC, PLL_CLKSEL_SIE5_CLKSRC_PLL12,    "SIE5"},
	{SIE_MCLK,      PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL12,    "SIE MCLK"},
	{SIE_MCLK2,     PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL12,  "SIE MCLK2"},
	{SIE_MCLK3,     PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL12,  "SIE MCLK3"},
};

static const PLLCLKCHECK pll_check_item_pll13[] = {
	{H265_CLK,      PLL_CLKSEL_H265,        PLL_CLKSEL_H265_PLL13,          "H.26X"},
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL13,    "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL13,   "SIE2"},
	{DCE_CLK,       PLL_CLKSEL_DCE,         PLL_CLKSEL_DCE_PLL13,           "DCE"},
	{IPE_CLK,       PLL_CLKSEL_IPE,         PLL_CLKSEL_IPE_PLL13,           "IPE"},
	{IME_CLK,       PLL_CLKSEL_IME,         PLL_CLKSEL_IME_PLL13,           "IME"},
	{DIS_CLK,       PLL_CLKSEL_DIS,         PLL_CLKSEL_DIS_PLL13,           "DIS"},
	{NUE2_CLK,      PLL_CLKSEL_NUE2,        PLL_CLKSEL_NUE2_PLL13,          "NUE2"},
	{MDBC_CLK,      PLL_CLKSEL_MDBC,        PLL_CLKSEL_MDBC_PLL13,          "MDBC"},
	{IFE_CLK,       PLL_CLKSEL_IFE,         PLL_CLKSEL_IFE_PLL13,           "IFE"},
	{IFE2_CLK,      PLL_CLKSEL_IFE2,        PLL_CLKSEL_IFE2_PLL13,          "IFE2"},
	{GRAPH_CLK,     PLL_CLKSEL_GRAPHIC,     PLL_CLKSEL_GRAPHIC_PLL13,       "GRAPHIC"},
	{GRAPH2_CLK,    PLL_CLKSEL_GRAPHIC2,    PLL_CLKSEL_GRAPHIC2_PLL13,      "GRAPHIC2"},
	{ISE_CLK,       PLL_CLKSEL_ISE,         PLL_CLKSEL_ISE_PLL13,           "ISE"},
};

static const PLLCLKCHECK pll528_check_item_pll13[] = {
	{H265_CLK,      PLL_CLKSEL_H265,        PLL_CLKSEL_H265_PLL13,          "H.26X"},
	{SIE_CLK,       PLL_CLKSEL_SIE_CLKSRC,  PLL_CLKSEL_SIE_CLKSRC_PLL13,    "SIE"},
	{SIE2_CLK,      PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL13,   "SIE2"},
	{SIE3_CLK,      PLL_CLKSEL_SIE3_CLKSRC, PLL_CLKSEL_SIE3_CLKSRC_PLL13,   "SIE3"},
	{SIE4_CLK,      PLL_CLKSEL_SIE4_CLKSRC, PLL_CLKSEL_SIE4_CLKSRC_PLL13,   "SIE4"},
	{SIE5_CLK,      PLL_CLKSEL_SIE5_CLKSRC, PLL_CLKSEL_SIE5_CLKSRC_PLL13,   "SIE5"},
	{DCE_CLK,       PLL_CLKSEL_DCE,         PLL_CLKSEL_DCE_PLL13,           "DCE"},
	{IPE_CLK,       PLL_CLKSEL_IPE,         PLL_CLKSEL_IPE_PLL13,           "IPE"},
	{IME_CLK,       PLL_CLKSEL_IME,         PLL_CLKSEL_IME_PLL13,           "IME"},
	{DIS_CLK,       PLL_CLKSEL_DIS,         PLL_CLKSEL_DIS_PLL13,           "DIS"},
	{NUE2_CLK,      PLL_CLKSEL_NUE2,        PLL_CLKSEL_NUE2_PLL13,          "NUE2"},
	{MDBC_CLK,      PLL_CLKSEL_MDBC,        PLL_CLKSEL_MDBC_PLL13,          "MDBC"},
	{IFE_CLK,       PLL_CLKSEL_IFE,         PLL_CLKSEL_IFE_PLL13,           "IFE"},
	{IFE2_CLK,      PLL_CLKSEL_IFE2,        PLL_CLKSEL_IFE2_PLL13,          "IFE2"},
	{GRAPH_CLK,     PLL_CLKSEL_GRAPHIC,     PLL_CLKSEL_GRAPHIC_PLL13,       "GRAPHIC"},
	{GRAPH2_CLK,    PLL_CLKSEL_GRAPHIC2,    PLL_CLKSEL_GRAPHIC2_PLL13,      "GRAPHIC2"},
	{ISE_CLK,       PLL_CLKSEL_ISE,         PLL_CLKSEL_ISE_PLL13,           "ISE"},
	{SDE_CLK,       PLL_CLKSEL_SDE,         PLL_CLKSEL_SDE_PLL13,           "SDE"},
};

static const PLLCLKCHECK pll_check_item_pll14[] = {
	{DMA2_CLK,      PLL_CLKSEL_MAX_ITEM,    PLL_CLKSEL_MAX_ITEM,            "DMA2"},
};

static const PLLCLKCHECK pll_check_item_pll15[] = {
	{H265_CLK,      PLL_CLKSEL_H265,        PLL_CLKSEL_H265_PLL15,          "H.26X"},
};

static const PLLCLKCHECK pll528_check_item_pll16[] = {};

static const PLLCLKCHECK pll528_check_item_pll17[] = {
	{VPE_CLK,       PLL_CLKSEL_VPE,         PLL_CLKSEL_VPE_PLL17,           "VPE"},
};

static const PLLCLKCHECK pll528_check_item_pll18[] = {
	{SIE_MCLK3,     PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL18, "SIE MCLK3"},
};

/*
    Check PLL Usage

    @param[in] id           PLL ID

    @return
		- @b TRUE: id is currently ALLOCATED/USED by some module
		- @b FALSE: id is free
*/
static BOOL pll_check_pll_usage(PLL_ID id)
{
	UINT32  i;
	UINT32  number;
	BOOL    clk_enable;
	const char *p_pll_name;
	PPLLCLKCHECK p_check_item;

	const PPLLCLKCHECK pll_items[] = {  (PPLLCLKCHECK)pll_check_item_pll3,
										(PPLLCLKCHECK)pll_check_item_pll4,
										(PPLLCLKCHECK)pll_check_item_pll5,
										(PPLLCLKCHECK)pll_check_item_pll6,
										(PPLLCLKCHECK)pll_check_item_pll7,
										(PPLLCLKCHECK)pll_check_item_pll8,
										(PPLLCLKCHECK)pll_check_item_pll9,
										(PPLLCLKCHECK)pll_check_item_pll10,
										(PPLLCLKCHECK)pll_check_item_pll11,
										(PPLLCLKCHECK)pll_check_item_pll12,
										(PPLLCLKCHECK)pll_check_item_pll13,
										(PPLLCLKCHECK)pll_check_item_pll14,
										(PPLLCLKCHECK)pll_check_item_pll15
									 };

	const UINT32 pll_item_cnt[] = {    sizeof(pll_check_item_pll3) / sizeof(pll_check_item_pll3[0]),
									   sizeof(pll_check_item_pll4) / sizeof(pll_check_item_pll4[0]),
									   sizeof(pll_check_item_pll5) / sizeof(pll_check_item_pll5[0]),
									   sizeof(pll_check_item_pll6) / sizeof(pll_check_item_pll6[0]),
									   sizeof(pll_check_item_pll7) / sizeof(pll_check_item_pll7[0]),
									   sizeof(pll_check_item_pll8) / sizeof(pll_check_item_pll8[0]),
									   sizeof(pll_check_item_pll9) / sizeof(pll_check_item_pll9[0]),
									   sizeof(pll_check_item_pll10) / sizeof(pll_check_item_pll10[0]),
									   sizeof(pll_check_item_pll11) / sizeof(pll_check_item_pll11[0]),
									   sizeof(pll_check_item_pll12) / sizeof(pll_check_item_pll12[0]),
									   sizeof(pll_check_item_pll13) / sizeof(pll_check_item_pll13[0]),
									   sizeof(pll_check_item_pll14) / sizeof(pll_check_item_pll14[0]),
									   sizeof(pll_check_item_pll15) / sizeof(pll_check_item_pll15[0])
								  };
	const PPLLCLKCHECK pll528_items[] = {  (PPLLCLKCHECK)pll_check_item_pll3,
										(PPLLCLKCHECK)pll_check_item_pll4,
										(PPLLCLKCHECK)pll528_check_item_pll5,
										(PPLLCLKCHECK)pll528_check_item_pll6,
										(PPLLCLKCHECK)pll_check_item_pll7,
										(PPLLCLKCHECK)pll_check_item_pll8,
										(PPLLCLKCHECK)pll528_check_item_pll9,
										(PPLLCLKCHECK)pll528_check_item_pll10,
										(PPLLCLKCHECK)pll_check_item_pll11,
										(PPLLCLKCHECK)pll528_check_item_pll12,
										(PPLLCLKCHECK)pll528_check_item_pll13,
										(PPLLCLKCHECK)pll_check_item_pll14,
										(PPLLCLKCHECK)pll_check_item_pll15,
										(PPLLCLKCHECK)pll528_check_item_pll16,
										(PPLLCLKCHECK)pll528_check_item_pll17,
										(PPLLCLKCHECK)pll528_check_item_pll18
									 };

	const UINT32 pll528_item_cnt[] = { sizeof(pll_check_item_pll3)    / sizeof(pll_check_item_pll3[0]),
									   sizeof(pll_check_item_pll4)    / sizeof(pll_check_item_pll4[0]),
									   sizeof(pll528_check_item_pll5) / sizeof(pll528_check_item_pll5[0]),
									   sizeof(pll528_check_item_pll6) / sizeof(pll528_check_item_pll6[0]),
									   sizeof(pll_check_item_pll7)    / sizeof(pll_check_item_pll7[0]),
									   sizeof(pll_check_item_pll8)    / sizeof(pll_check_item_pll8[0]),
									   sizeof(pll528_check_item_pll9) / sizeof(pll528_check_item_pll9[0]),
									   sizeof(pll528_check_item_pll10)/ sizeof(pll528_check_item_pll10[0]),
									   sizeof(pll_check_item_pll11)   / sizeof(pll_check_item_pll11[0]),
									   sizeof(pll528_check_item_pll12)/ sizeof(pll528_check_item_pll12[0]),
									   sizeof(pll528_check_item_pll13)/ sizeof(pll528_check_item_pll13[0]),
									   sizeof(pll_check_item_pll14)   / sizeof(pll_check_item_pll14[0]),
									   sizeof(pll_check_item_pll15)   / sizeof(pll_check_item_pll15[0]),
									   sizeof(pll528_check_item_pll16)   / sizeof(pll528_check_item_pll16[0]),
									   sizeof(pll528_check_item_pll17)   / sizeof(pll528_check_item_pll17[0]),
									   sizeof(pll528_check_item_pll18)   / sizeof(pll528_check_item_pll18[0])
								  };

	const char *pll_names[] = {"PLL3", "PLL4", "PLL5", "PLL6", "PLL7", "PLL8", "PLL9", "PLL10", "PLL11", "PLL12", "PLL13", "PLL14", "PLL15", "PLL16", "PLL17", "PLL18"};

	if (nvt_get_chip_id() == CHIP_NA51084) {

		if ((id > PLL_ID_18) || (id < PLL_ID_3)) {
			return TRUE;
		}

		p_check_item  = pll528_items[id - PLL_ID_3];
		number        = pll528_item_cnt[id - PLL_ID_3];

	} else {

		if ((id > PLL_ID_15) || (id < PLL_ID_3)) {
			return TRUE;
		}

		p_check_item  = pll_items[id - PLL_ID_3];
		number        = pll_item_cnt[id - PLL_ID_3];
	}

	p_pll_name    = pll_names[id - PLL_ID_3];

	// Check all Clock Select related to id (instance of PLL_ID)
	for (i = 0; i < number; i++) {
		if (p_check_item[i].source_module == PLL_CLKSEL_CPU) {
			// CPU's clock is always ON.
			clk_enable = TRUE;
		} else {
			clk_enable = pll_is_clock_enabled(p_check_item[i].source_enabled);
		}


		if (clk_enable) {
			// Module Clock is enabled, Check the clock source
			if (pll_get_clock_rate(p_check_item[i].source_module) == p_check_item[i].check_item) {
				DBG_ERR("Change (%s) clock freq fail! (%s) is using %s. Don't change clock when module is active\r\n", p_pll_name, p_check_item[i].module_name, p_pll_name);
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*
    Get bit mask for specific module clock setting

    Get bit mask for specific module clock setting

    @param[in] ui_num    Specific module

    @return bit mask for specific module clock
*/
static UINT32 pll_get_clock_mask(UINT32 ui_num)
{
	switch (ui_num) {
	/*R0*/
	case PLL_CLKSEL_CPU:
		return PLL_CLKSEL_CPU_MASK;
	case PLL_CLKSEL_APB:
		return PLL_CLKSEL_APB_MASK;
	case PLL_CLKSEL_DMA_ARBT:
		return PLL_CLKSEL_DMA_ARBT_MASK;
	//case PLL_CLKSEL_DDRPHY:
	//  return PLL_CLKSEL_DDRPHY_MASK;

	/*R1*/
	case PLL_CLKSEL_CNN:
		return PLL_CLKSEL_CNN_MASK;
	case PLL_CLKSEL_IVE:
		return PLL_CLKSEL_IVE_MASK;
	case PLL_CLKSEL_IPE:
		return PLL_CLKSEL_IPE_MASK;
	case PLL_CLKSEL_DIS:
		return PLL_CLKSEL_DIS_MASK;
	case PLL_CLKSEL_IME:
		return PLL_CLKSEL_IME_MASK;
	case PLL_CLKSEL_CNN2:
		return PLL_CLKSEL_CNN2_MASK;
	case PLL_CLKSEL_SDE:
		return PLL_CLKSEL_SDE_MASK;
	case PLL_CLKSEL_MDBC:
		return PLL_CLKSEL_MDBC_MASK;
	case PLL_CLKSEL_VPE:
		return PLL_CLKSEL_VPE_MASK;
	case PLL_CLKSEL_ISE:
		return PLL_CLKSEL_ISE_MASK;
	case PLL_CLKSEL_DCE:
		return PLL_CLKSEL_DCE_MASK;

	/*R2*/
	case PLL_CLKSEL_IFE:
		return PLL_CLKSEL_IFE_MASK;
	case PLL_CLKSEL_IFE2:
		return PLL_CLKSEL_IFE2_MASK;
	case PLL_CLKSEL_SIE_MCLKSRC:
		return PLL_CLKSEL_SIE_MCLKSRC_MASK;
	case PLL_CLKSEL_SIE_MCLK2SRC:
		return PLL_CLKSEL_SIE_MCLK2SRC_MASK;
	case PLL_CLKSEL_SIE_MCLK3SRC:
		return PLL_CLKSEL_SIE_MCLK3SRC_MASK;
	case PLL_CLKSEL_SIE_IO_PXCLKSRC:
		return PLL_CLKSEL_SIE_IO_PXCLKSRC_MASK;
	case PLL_CLKSEL_SIE2_IO_PXCLKSRC:
		return PLL_CLKSEL_SIE2_IO_PXCLKSRC_MASK;
	case PLL_CLKSEL_SIE_PXCLKSRC:
		return PLL_CLKSEL_SIE_PXCLKSRC_MASK;
	case PLL_CLKSEL_SIE2_PXCLKSRC:
		return PLL_CLKSEL_SIE2_PXCLKSRC_MASK;
	case PLL_CLKSEL_SIE3_PXCLKSRC:
		return PLL_CLKSEL_SIE3_PXCLKSRC_MASK;
	case PLL_CLKSEL_SIE_MCLKINV:
		return PLL_CLKSEL_SIE_MCLKINV_MASK;
	case PLL_CLKSEL_SIE4_IO_PXCLKSRC:
		return PLL_CLKSEL_SIE4_IO_PXCLKSRC_MASK;
	case PLL_CLKSEL_SIE4_PXCLKSRC:
		return PLL_CLKSEL_SIE4_PXCLKSRC_MASK;
	case PLL_CLKSEL_TGE:
		return PLL_CLKSEL_TGE_MASK;
	case PLL_CLKSEL_TGE2:
		return PLL_CLKSEL_TGE2_MASK;
	case PLL_CLKSEL_SIE_MCLK2INV:
		return PLL_CLKSEL_SIE_MCLK2INV_MASK;
	case PLL_CLKSEL_SIE_MCLK3INV:
		return PLL_CLKSEL_SIE_MCLK3INV_MASK;
	case PLL_CLKSEL_NUE:
		return PLL_CLKSEL_NUE_MASK;
	case PLL_CLKSEL_NUE2:
		return PLL_CLKSEL_NUE2_MASK;

	/*R3*/
	case PLL_CLKSEL_JPEG:
		return PLL_CLKSEL_JPEG_MASK;
	case PLL_CLKSEL_H265:
		return PLL_CLKSEL_H265_MASK;
	case PLL_CLKSEL_GRAPHIC:
		return PLL_CLKSEL_GRAPHIC_MASK;
	case PLL_CLKSEL_AFFINE:
		return PLL_CLKSEL_AFFINE_MASK;
	case PLL_CLKSEL_GRAPHIC2:
		return PLL_CLKSEL_GRAPHIC2_MASK;
	case PLL_CLKSEL_CRYPTO:
		return PLL_CLKSEL_CRYPTO_MASK;
	case PLL_CLKSEL_RSA:
		return PLL_CLKSEL_RSA_MASK;
	case PLL_CLKSEL_SIE_CLKSRC:
		return PLL_CLKSEL_SIE_CLKSRC_MASK;
	case PLL_CLKSEL_SIE2_CLKSRC:
		return PLL_CLKSEL_SIE2_CLKSRC_MASK;
	case PLL_CLKSEL_SIE3_CLKSRC:
		return PLL_CLKSEL_SIE3_CLKSRC_MASK;

	/*R4*/
	case PLL_CLKSEL_SDIO:
		return PLL_CLKSEL_SDIO_MASK;
	case PLL_CLKSEL_SDIO2:
		return PLL_CLKSEL_SDIO2_MASK;
	case PLL_CLKSEL_MIPI_LVDS:
		return PLL_CLKSEL_MIPI_LVDS_MASK;
	case PLL_CLKSEL_LVDS_CLKPHASE:
		return PLL_CLKSEL_LVDS_CLKPHASE_MASK;
	case PLL_CLKSEL_LVDS2_CLKPHASE:
		return PLL_CLKSEL_LVDS2_CLKPHASE_MASK;
	case PLL_CLKSEL_MIPI_LVDS2:
		return PLL_CLKSEL_MIPI_LVDS2_MASK;
	case PLL_CLKSEL_IDE_CLKSRC:
		return PLL_CLKSEL_IDE_CLKSRC_MASK;
	case PLL_CLKSEL_DSI_LPSRC:
		return PLL_CLKSEL_DSI_LPSRC_MASK;
	case PLL_CLKSEL_ADO_MCLKSEL:
		return PLL_CLKSEL_ADO_MCLKSEL_MASK;
	case PLL_CLKSEL_ETH:
		return PLL_CLKSEL_ETH_MASK;

	/*R5*/
	case PLL_CLKSEL_SDIO3:
		return PLL_CLKSEL_SDIO3_MASK;
	case PLL_CLKSEL_TSE:
		return PLL_CLKSEL_TSE_MASK;
	case PLL_CLKSEL_SP:
		return PLL_CLKSEL_SP_MASK;
	case PLL_CLKSEL_SP2:
		return PLL_CLKSEL_SP2_MASK;
	case PLL_CLKSEL_ETH_REFCLK_INV:
		return PLL_CLKSEL_ETH_REFCLK_INV_MASK;
	case PLL_CLKSEL_HASH:
		return PLL_CLKSEL_HASH_MASK;
	case PLL_CLKSEL_TRNG:
		return PLL_CLKSEL_TRNG_MASK;
	case PLL_CLKSEL_TRNGRO_CLKSRC:
		return PLL_CLKSEL_TRNGRO_CLKSRC_MASK;
	case PLL_CLKSEL_DRTC:
		return PLL_CLKSEL_DRTC_MASK;
	case PLL_CLKSEL_REMOTE:
		return PLL_CLKSEL_REMOTE_MASK;
	case PLL_CLKSEL_ADC_PD:
		return PLL_CLKSEL_ADC_PD_MASK;
	case PLL_CLKSEL_ETHPHY_CLKSRC:
		return PLL_CLKSEL_ETHPHY_CLKSRC_MASK;

	/*R6*/
	case PLL_CLKSEL_CSILPCLK_D0:
		return PLL_CLKSEL_CSILPCLK_D0_MASK;
	case PLL_CLKSEL_CSILPCLK_D1:
		return PLL_CLKSEL_CSILPCLK_D1_MASK;
	case PLL_CLKSEL_CSILPCLK_D2:
		return PLL_CLKSEL_CSILPCLK_D2_MASK;
	case PLL_CLKSEL_CSILPCLK_D3:
		return PLL_CLKSEL_CSILPCLK_D3_MASK;

	/*R7*/
	case PLL_CLKSEL_SIE4_CLKSRC:
		return PLL_CLKSEL_SIE4_CLKSRC_MASK;
	case PLL_CLKSEL_SIE5_CLKSRC:
		return PLL_CLKSEL_SIE5_CLKSRC_MASK;
	case PLL_CLKSEL_SIE4_CLKDIV:
		return PLL_CLKSEL_SIE4_CLKDIV_MASK;
	case PLL_CLKSEL_SIE5_CLKDIV:
		return PLL_CLKSEL_SIE5_CLKDIV_MASK;

	/*R8*/
	case PLL_CLKSEL_SIE_MCLKDIV:
		return PLL_CLKSEL_SIE_MCLKDIV_MASK;
	case PLL_CLKSEL_SIE_MCLK2DIV:
		return PLL_CLKSEL_SIE_MCLK2DIV_MASK;
	case PLL_CLKSEL_SIE_CLKDIV:
		return PLL_CLKSEL_SIE_CLKDIV_MASK;
	case PLL_CLKSEL_SIE2_CLKDIV:
		return PLL_CLKSEL_SIE2_CLKDIV_MASK;

	/*R9*/
	case PLL_CLKSEL_IDE_CLKDIV:
		return PLL_CLKSEL_IDE_CLKDIV_MASK;
	case PLL_CLKSEL_IDE_OUTIF_CLKDIV:
		return PLL_CLKSEL_IDE_OUTIF_CLKDIV_MASK;
	case PLL_CLKSEL_SIE_MCLK3DIV:
		return PLL_CLKSEL_SIE_MCLK3DIV_MASK;
	case PLL_CLKSEL_TRNG_CLKDIV:
		return PLL_CLKSEL_TRNG_CLKDIV_MASK;

	/*R10*/
	case PLL_CLKSEL_SP_CLKDIV:
		return PLL_CLKSEL_SP_CLKDIV_MASK;
	case PLL_CLKSEL_SIE3_CLKDIV:
		return PLL_CLKSEL_SIE3_CLKDIV_MASK;
	case PLL_CLKSEL_ADO_CLKDIV:
		return PLL_CLKSEL_ADO_CLKDIV_MASK;
	case PLL_CLKSEL_ADO_OSR_CLKDIV:
		return PLL_CLKSEL_ADO_OSR_CLKDIV_MASK;

	/*R11*/
	case PLL_CLKSEL_SDIO_CLKDIV:
		return PLL_CLKSEL_SDIO_CLKDIV_MASK;
	case PLL_CLKSEL_SDIO2_CLKDIV:
		return PLL_CLKSEL_SDIO2_CLKDIV_MASK;

	/*R12*/
	case PLL_CLKSEL_SDIO3_CLKDIV:
		return PLL_CLKSEL_SDIO3_CLKDIV_MASK;
	case PLL_CLKSEL_NAND_CLKDIV:
		return PLL_CLKSEL_NAND_CLKDIV_MASK;
	case PLL_CLKSEL_SP2_CLKDIV:
		return PLL_CLKSEL_SP2_CLKDIV_MASK;

	/*R13*/
	case PLL_CLKSEL_SPI_CLKDIV:
		return PLL_CLKSEL_SPI_CLKDIV_MASK;
	case PLL_CLKSEL_SPI2_CLKDIV:
		return PLL_CLKSEL_SPI2_CLKDIV_MASK;

	/*R14*/
	case PLL_CLKSEL_SPI3_CLKDIV:
		return PLL_CLKSEL_SPI3_CLKDIV_MASK;
	case PLL_CLKSEL_SPI4_CLKDIV:
		return PLL_CLKSEL_SPI4_CLKDIV_MASK;

	/*R15*/
	case PLL_CLKSEL_UART2_CLKDIV:
		return PLL_CLKSEL_UART2_CLKDIV_MASK;
	case PLL_CLKSEL_UART3_CLKDIV:
		return PLL_CLKSEL_UART3_CLKDIV_MASK;
	case PLL_CLKSEL_UART4_CLKDIV:
		return PLL_CLKSEL_UART4_CLKDIV_MASK;
	case PLL_CLKSEL_UART5_CLKDIV:
		return PLL_CLKSEL_UART5_CLKDIV_MASK;

	/*R20*/
	case PLL_CLKSEL_TRNG_RO_CLKDIV:
		return PLL_CLKSEL_TRNG_RO_CLKDIV_MASK;
	case PLL_CLKSEL_TRNG_RO_DELAY:
		return PLL_CLKSEL_TRNG_RO_DELAY_MASK;
	case PLL_CLKSEL_UART6_CLKDIV:
		return PLL_CLKSEL_UART6_CLKDIV_MASK;
	case PLL_CLKSEL_ADO_MCLKDIV:
		return PLL_CLKSEL_ADO_MCLKDIV_MASK;

	/*R21*/
	case PLL_CLKSEL_SPI5_CLKDIV:
		return PLL_CLKSEL_SPI5_CLKDIV_MASK;

	default:
		DBG_ERR("Non-supported Clk Mask ID! (0x%x)\r\n", (UINT)ui_num);
		return 0;
	}
}

/*
    Check module clock limitation

    Check input clock mux/select.
    If outputted frequency is out of spec, dump warning messsage to UART.

    @param[in] ui_num            Clock mux to check
    @param[in] ui_value          Clock rate of ui_num

    @return void
*/
static void pll_chk_clk_limitation(PLL_CLKSEL ui_num, UINT32 ui_value)
{
	UINT32 ui_freq = 0;

	switch (ui_num) {
	case PLL_CLKSEL_CPU:
		if (ui_value == PLL_CLKSEL_CPU_PLL8) {
			ui_freq = pll_get_pll_freq(PLL_ID_8);
		} else {
			return;
		}
		if (nvt_get_chip_id() == CHIP_NA51084) {
			if (ui_freq > 1100000000) {
				DBG_WRN("CPU Clock frequency exceed 1.1GHz!\r\n");
			}
		} else {
			if (ui_freq > 960000000) {
				DBG_WRN("CPU Clock frequency exceed 960MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_CNN:
		if (ui_value == PLL_CLKSEL_CNN_PLL10) {
			ui_freq = pll_get_pll_freq(PLL_ID_10);
		}
		if (ui_freq > 600000000) {
			DBG_WRN("CNN Clock frequency exceed 600MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_CNN2:
		if (ui_value == PLL_CLKSEL_CNN2_PLL10) {
			ui_freq = pll_get_pll_freq(PLL_ID_10);
		}
		if (ui_freq > 600000000) {
			DBG_WRN("CNN2 Clock frequency exceed 600MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_IPE:
		if (ui_value == PLL_CLKSEL_IPE_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		} else if (ui_value == PLL_CLKSEL_IPE_320) {
			ui_freq = 320000000;
		}
		if (ui_freq > 280000000) {
			DBG_WRN("IPE Clock frequency exceed 280MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_DIS:
		if (ui_value == PLL_CLKSEL_DIS_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		}
		if (ui_freq > 480000000) {
			DBG_WRN("DIS Clock frequency exceed 480MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_IME:
		if (ui_value == PLL_CLKSEL_IME_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		} else if (ui_value == PLL_CLKSEL_IME_320) {
			ui_freq = 320000000;
		}
		if (ui_freq > 280000000) {
			DBG_WRN("IME Clock frequency exceed 280MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_ISE:
		if (ui_value == PLL_CLKSEL_ISE_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		}
		if (ui_freq > 480000000) {
			DBG_WRN("ISE Clock frequency exceed 480MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_DCE:
		if (ui_value == PLL_CLKSEL_DCE_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		} else if (ui_value == PLL_CLKSEL_DCE_320) {
			ui_freq = 320000000;
		}
		if (ui_freq > 280000000) {
			DBG_WRN("DCE Clock frequency exceed 280MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_MDBC:
		if (ui_value == PLL_CLKSEL_MDBC_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		} else if (ui_value == PLL_CLKSEL_MDBC_320) {
			ui_freq = 320000000;
		}
		if (ui_freq > 240000000) {
			DBG_WRN("MDBC Clock frequency exceed 240MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_IFE:
		if (ui_value == PLL_CLKSEL_IFE_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		} else if (ui_value == PLL_CLKSEL_IFE_320) {
			ui_freq = 320000000;
		}
		if (ui_freq > 280000000) {
			DBG_WRN("IFE Clock frequency exceed 280MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_IFE2:
		if (ui_value == PLL_CLKSEL_IFE2_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		} else if (ui_value == PLL_CLKSEL_IFE2_320) {
			ui_freq = 320000000;
		}
		if (ui_freq > 280000000) {
			DBG_WRN("IFE2 Clock frequency exceed 280MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_H265:
		if (ui_value == PLL_CLKSEL_H265_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		}
		if (ui_freq > 320000000) {
			DBG_WRN("H.265 Clock frequency exceed 320MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_GRAPHIC:
		if (ui_value == PLL_CLKSEL_GRAPHIC_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		}
		if (ui_freq > 480000000) {
			DBG_WRN("Graphic Clock frequency exceed 480MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_GRAPHIC2:
		if (ui_value == PLL_CLKSEL_GRAPHIC2_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		}
		if (ui_freq > 480000000) {
			DBG_WRN("Graphic2 Clock frequency exceed 480MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_JPEG:
		if (ui_value == PLL_CLKSEL_JPEG_PLL9) {
			ui_freq = pll_get_pll_freq(PLL_ID_9);
		}
		if (ui_freq > 360000000) {
			DBG_WRN("JPEG Clock frequency exceed 360MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_NUE:
		if (ui_value == PLL_CLKSEL_NUE_PLL10) {
			ui_freq = pll_get_pll_freq(PLL_ID_10);
		}
		if (ui_freq > 600000000) {
			DBG_WRN("NUE Clock frequency exceed 600MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_NUE2:
		if (ui_value == (UINT32)PLL_CLKSEL_NUE2_PLL13) {
			ui_freq = pll_get_pll_freq(PLL_ID_13);
		}
		if (ui_freq > 480000000) {
			DBG_WRN("NUE2 Clock frequency exceed 480MHz!\r\n");
		}
		return;

	case PLL_CLKSEL_SIE_MCLKDIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLKSRC);

			switch (ui_freq) {
			case PLL_CLKSEL_SIE_MCLKSRC_480:
				ui_freq = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_MCLKSRC_PLL5:
				ui_freq = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_MCLKSRC_PLL10:
				ui_freq = pll_get_pll_freq(PLL_ID_10);
				break;

			default:
				return;

			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 150000000) {
				DBG_WRN("SIE-Mclk clock must not exceed 150MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_SIE_MCLK2DIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC);

			switch (ui_freq) {
			case PLL_CLKSEL_SIE_MCLK2SRC_480:
				ui_freq = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_MCLK2SRC_PLL5:
				ui_freq = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_MCLK2SRC_PLL10:
				ui_freq = pll_get_pll_freq(PLL_ID_10);
				break;

			default:
				return;

			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 150000000) {
				DBG_WRN("SIE-Mclk2 clock must not exceed 150MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_SIE_CLKDIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC);

			switch (ui_freq) {
			case PLL_CLKSEL_SIE_CLKSRC_480:
				ui_freq = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL5:
				ui_freq = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL13:
				ui_freq = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL12:
				ui_freq = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_320:
				ui_freq = pll_get_pll_freq(PLL_ID_FIXED320);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL10:
				ui_freq = pll_get_pll_freq(PLL_ID_10);
				break;

			default:
				return;
			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 300000000) {
				DBG_WRN("SIE-clk clock must not exceed 300MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_SIE2_CLKDIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC);

			switch (ui_freq) {
			case PLL_CLKSEL_SIE2_CLKSRC_480:
				ui_freq = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL5:
				ui_freq = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL13:
				ui_freq = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL12:
				ui_freq = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_320:
				ui_freq = pll_get_pll_freq(PLL_ID_FIXED320);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL10:
				ui_freq = pll_get_pll_freq(PLL_ID_10);
				break;

			default:
				return;
			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 300000000) {
				DBG_WRN("SIE2-clk clock must not exceed 300MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_IDE_CLKDIV:
	case PLL_CLKSEL_IDE_OUTIF_CLKDIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC);

			switch (ui_freq) {
			case PLL_CLKSEL_IDE_CLKSRC_480:
				ui_freq = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL6:
				ui_freq = pll_get_pll_freq(PLL_ID_6);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL4:
				ui_freq = pll_get_pll_freq(PLL_ID_4);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL9:
				ui_freq = pll_get_pll_freq(PLL_ID_9);
				break;

			default:
				return;
			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);

			if (ui_freq > 300000000) {
				DBG_WRN("IDE(0x%X) clock must not exceed 300Mhz!\r\n", ui_num);
			}
		}
		return;


	case PLL_CLKSEL_SP_CLKDIV: {
#if defined(_NVT_FPGA_)
			ui_freq = PLL_FPGA_480SRC / ((ui_value >> (ui_num - PLL_CLKSEL_R10_OFFSET)) + 1);
#else
			ui_freq = 240000000 / ((ui_value >> (ui_num & 0x1F)) + 1);
#endif
			if (ui_freq > 80000000) {
				DBG_WRN("SP clock must not exceed 80MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_ADO_CLKDIV: {
			ui_freq = pll_get_pll_freq(PLL_ID_7) / ((pll_get_clock_rate(PLL_CLKSEL_ADO_OSR_CLKDIV) >> (PLL_CLKSEL_ADO_OSR_CLKDIV & 0x1F))+1);
			ui_freq /= ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 25000000) {
				DBG_WRN("ADO clock must not exceed 25MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_SDIO_CLKDIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_SDIO);

			switch (ui_freq) {
			case PLL_CLKSEL_SDIO_PLL4:
				ui_freq = pll_get_pll_freq(PLL_ID_4);
				break;
			default:
				return;
			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 108000000) {
				DBG_WRN("SDIO clock must not exceed 108MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_SDIO2_CLKDIV: {
			ui_freq = pll_get_clock_rate(PLL_CLKSEL_SDIO2);

			switch (ui_freq) {
			case PLL_CLKSEL_SDIO2_PLL4:
				ui_freq = pll_get_pll_freq(PLL_ID_4);
				break;

			default:
				return;
			}

			ui_freq = ui_freq / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 52000000) {
				DBG_WRN("SDIO2 clock must not exceed 52MHz!\r\n");
			}
		}
		return;

	case PLL_CLKSEL_SPI_CLKDIV:
	case PLL_CLKSEL_SPI2_CLKDIV:
	case PLL_CLKSEL_SPI3_CLKDIV:
	case PLL_CLKSEL_SPI4_CLKDIV:
	case PLL_CLKSEL_SPI5_CLKDIV: {
			ui_freq = 192000000 / ((ui_value >> (ui_num & 0x1F)) + 1);
			if (ui_freq > 48000000) {
				DBG_WRN("SPIx clock must not exceed 48MHz!\r\n");
			}
		}
		return;

	default:
		return;
	}



}

#endif

/*
    Set (Driver scope) PLL frequency

    (This API should only be invoked by driver)

    @param[in] id           PLL ID
    @param[in] ui_setting	PLL frequency setting. PLL frequency will be 12MHz * ui_setting / 131072.
							(Valid value: 0 ~ 0xFFFFFF)

    @return
		- @b E_OK: success
		- @b E_ID: PLL ID is out of range
		- @b E_PAR: ui_setting is out of range
		- @b E_MACV: Any module use this PLL and is enabled.
*/
ER pll_set_driver_pll(PLL_ID id, UINT32 ui_setting)
{
#if !defined(_NVT_FPGA_)
	BOOL b_en;
	unsigned long flags = 0;
	T_PLL_PLL2_CR0_REG reg0 = {0};
	T_PLL_PLL2_CR1_REG reg1 = {0};
	T_PLL_PLL2_CR2_REG reg2 = {0};
	const UINT32 pll_address[] = {PLL_PLL3_CR0_REG_OFS, PLL_PLL4_CR0_REG_OFS,
								  PLL_PLL5_CR0_REG_OFS, PLL_PLL6_CR0_REG_OFS, PLL_PLL7_CR0_REG_OFS,
								  PLL_PLL8_CR0_REG_OFS, PLL_PLL9_CR0_REG_OFS, PLL_PLL10_CR0_REG_OFS,
								  PLL_PLL11_CR0_REG_OFS, PLL_PLL12_CR0_REG_OFS, PLL_PLL13_CR0_REG_OFS,
								  PLL_PLL14_CR0_REG_OFS, PLL_PLL15_CR0_REG_OFS
								 };
	const UINT32 pll528_address[] = {PLL528_PLL3_CR0_REG_OFS, PLL528_PLL4_CR0_REG_OFS,
								     PLL528_PLL5_CR0_REG_OFS, PLL528_PLL6_CR0_REG_OFS,  PLL528_PLL7_CR0_REG_OFS,
								     PLL528_PLL8_CR0_REG_OFS, PLL528_PLL9_CR0_REG_OFS,  PLL528_PLL10_CR0_REG_OFS,
								     PLL528_PLL11_CR0_REG_OFS,PLL528_PLL12_CR0_REG_OFS, PLL528_PLL13_CR0_REG_OFS,
								     PLL528_PLL14_CR0_REG_OFS,PLL528_PLL15_CR0_REG_OFS, PLL528_PLL16_CR0_REG_OFS,
								     PLL528_PLL17_CR0_REG_OFS,PLL528_PLL18_CR0_REG_OFS
								 };

	if (ui_setting > 0xFFFFFF) {
		DBG_ERR("ui_setting out of range: 0x%lx\r\n", ui_setting);
		return E_PAR;
	}

	if (nvt_get_chip_id() == CHIP_NA51084) {
		if ((id > PLL_ID_18) || (id < PLL_ID_3)) {
			DBG_ERR("id out of range: PLL%d\r\n", id);
			return E_ID;
		}
	} else {
		if ((id > PLL_ID_15) || (id < PLL_ID_3)) {
			DBG_ERR("id out of range: PLL%d\r\n", id);
			return E_ID;
		}
	}

	if (pll_check_pll_usage(id)) {
		return E_MACV;
	}

	if (nvt_get_chip_id() == CHIP_NA51084 && id == PLL_ID_8) {
		ui_setting = (ui_setting >> 3);
	}

	reg0.bit.PLL_RATIO0 = ui_setting & 0xFF;
	reg1.bit.PLL_RATIO1 = (ui_setting >> 8) & 0xFF;
	reg2.bit.PLL_RATIO2 = (ui_setting >> 16) & 0xFF;

	b_en = pll_get_pll_enable(id);

	// PLL 7/8 can be modified when it is enabled
	if ((id != PLL_ID_7) && (id != PLL_ID_8)) {
		if (b_en)
			DBG_WRN("Should disable before change feq: PLL%d\r\n", id);
	}

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	switch (id) {
	case PLL_ID_4: {
			T_PLL_PLL4_SSPLL1_REG pll4_ctl0_reg;

			pll4_ctl0_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL1_REG_OFS);
			pll4_ctl0_reg.bit.SSC_EN = 0;
			PLL_SETREG(PLL_PLL4_SSPLL1_REG_OFS, pll4_ctl0_reg.reg);
		}
		break;

	default:
		break;
	}

	if (nvt_get_chip_id() == CHIP_NA51084) {
		PLL_SETREG(pll528_address[id - PLL_ID_3], reg0.reg);
		PLL_SETREG(pll528_address[id - PLL_ID_3] + 0x04, reg1.reg);
		PLL_SETREG(pll528_address[id - PLL_ID_3] + 0x08, reg2.reg);
	} else if (id <= PLL_ID_15){
		PLL_SETREG(pll_address[id - PLL_ID_3], reg0.reg);
		PLL_SETREG(pll_address[id - PLL_ID_3] + 0x04, reg1.reg);
		PLL_SETREG(pll_address[id - PLL_ID_3] + 0x08, reg2.reg);
	} else {
		DBG_ERR("id out of range: PLL%d\r\n", id);
		return E_ID;
	}

	//race condition protect. leave critical section
	unl_multi_cores(flags);

	if (b_en && ((id != PLL_ID_7) && (id != PLL_ID_8))) {
		DBG_WRN("PLL%d should be disabled before set\r\n", id + 1);
	}
#else
	pll_check_pll_usage(id);
#endif

	return E_OK;
}

#if 1

/**
    Set PLL frequency

	@param[in] id           PLL ID
		- @b PLL_ID_4
		- @b PLL_ID_5
		- @b PLL_ID_6
		- @b PLL_ID_10
		- @b PLL_ID_11
		- @b PLL_ID_13
		- @b others: NOT support
    @param[in] ui_setting	PLL frequency setting. PLL frequency will be 12MHz * ui_setting / 131072.
							(Valid value: 0 ~ 0xFFFFFF)

    @return
		- @b E_OK: success
		- @b E_ID: PLL ID is out of range
		- @b E_PAR: ui_setting is out of range
		- @b E_MACV: Any module use this PLL and is enabled.
*/
ER pll_set_pll(PLL_ID id, UINT32 ui_setting)
{
	if ((id >= PLL_ID_MAX) || (id == PLL_ID_1)) {
		DBG_ERR("id out of range: PLL%d\r\n", id);
		return E_ID;
	}
	switch (id) {
	case PLL_ID_4:
	case PLL_ID_5:
	case PLL_ID_6:
	case PLL_ID_9:
	case PLL_ID_10:
	case PLL_ID_11:
	case PLL_ID_12:
	case PLL_ID_13:
	case PLL_ID_15:
		return pll_set_driver_pll(id, ui_setting);
	default:
		DBG_WRN("PLL%d Not Valid\r\n", id);
		return E_ID;
	}
}

/**
    Get PLL Enable

    @param[in] id           PLL ID

    @return
		- @b TRUE: PLL is enabled
		- @b FALSE: PLL is disabled or id is out of range
*/
BOOL pll_get_pll_enable(PLL_ID id)
{
	T_PLL_PLL_PWREN_REG pll_en_reg;

	if (id == PLL_ID_1) {
		return TRUE;
	}

	if ((id >= PLL_ID_MAX) || (id == PLL_ID_1)) {
		DBG_ERR("id out of range: PLL%d\r\n", id);
		return FALSE;
	}

	pll_en_reg.reg = PLL_GETREG(PLL_PLL_PWREN_REG_OFS);
	if (pll_en_reg.reg & (1 << id)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    Set PLL Enable

    @param[in] id           PLL ID
    @param[in] bEnable      enable/disable PLL
		- @b TRUE: enable PLL
		- @b FALSE: disable PLL

    @return
		- @b E_OK: success
		- @b E_ID: PLL ID is out of range
*/
ER pll_set_pll_enable(PLL_ID id, BOOL b_enable)
{
	unsigned long flags = 0;
	T_PLL_PLL_PWREN_REG pll_en_reg;
	T_PLL_PLL_STATUS_REG pll_status_reg;
	static int pll_enable_count[PLL_ID_MAX] = {0};

	if ((id >= PLL_ID_MAX) || (id == PLL_ID_1)) {
		DBG_ERR("id out of range: PLL%d\r\n", id);
		return E_ID;
	}

	if (b_enable) {
		pll_enable_count[id]++;
		if (pll_enable_count[id] > 1)
			return E_OK;
	} else {
		if (pll_enable_count[id] == 0) {
			DBG_ERR("invald disable calling: PLL%d\r\n", id);
			return E_ID;
		}

		if (--pll_enable_count[id] > 0)
			return E_OK;
	}

	loc_multi_cores(flags);
	pll_en_reg.reg = PLL_GETREG(PLL_PLL_PWREN_REG_OFS);
	if (b_enable) {
		pll_en_reg.reg |= 1 << id;
	} else {
		pll_en_reg.reg &= ~(1 << id);
	}
	PLL_SETREG(PLL_PLL_PWREN_REG_OFS, pll_en_reg.reg);

	unl_multi_cores(flags);

	if (b_enable) {
		// Wait PLL power is powered on
		while (1) {
			pll_status_reg.reg = PLL_GETREG(PLL_PLL_STATUS_REG_OFS);
			if (pll_status_reg.reg & (1 << id)) {
				break;
			}
		}
	}

	//pll_install_cmd();

	return E_OK;
}

/**
    Set PLL frequency

    Set PLL frequency (unit:Hz)

    @param[in] id           PLL ID
    @param[in] ui_frequency  Target PLL Frequency

    @return
*/
ER pll_set_pll_freq(PLL_ID id, UINT32 ui_frequency)
{
	FLOAT  ftemp;
	UINT32 ui_setting;

	if ((id >= PLL_ID_MAX) || (id == PLL_ID_1)) {
		DBG_ERR("id out of range: PLL%d\r\n", id);
		return E_ID;
	}

	ftemp = (FLOAT)ui_frequency / (FLOAT)12000000;

	ui_setting = (UINT32)(ftemp * 131072.0);

	//DBG_DUMP("PLL%d Setting=0x%08X\r\n",id,ui_setting);

	switch (id) {
	case PLL_ID_4:
	case PLL_ID_5:
	case PLL_ID_6:
	case PLL_ID_9:
	case PLL_ID_10:
	case PLL_ID_11:
	case PLL_ID_12:
	case PLL_ID_13:
	case PLL_ID_15:
	case PLL_ID_17:
	case PLL_ID_18:
		return pll_set_driver_pll(id, ui_setting);
	default:
		DBG_WRN("PLL%d Not Valid\r\n", id);
		return E_ID;
	}


}


/**
    Get PLL frequency

    Get PLL frequency (unit:Hz)
    When spread spectrum is enabled (by pll_set_pll_spread_spectrum()), this API will return lower bound frequency of spread spectrum.

    @param[in] id           PLL ID

    @return PLL frequency (unit:Hz)
*/
UINT32 pll_get_pll_freq(PLL_ID id)
{
#if defined(_NVT_FPGA_)
	if (id == PLL_ID_7) {
		return PLL_FPGA_PLL7SRC;
	} else if ((id == PLL_ID_2) || (id == PLL_ID_5) || (id == PLL_ID_11)) {
		return (PLL_FPGA_PLL2RC * 2);
	} else if (id == PLL_ID_3) {
		return (PLL_FPGA_PLL3SRC * 2);
	} else if (id == PLL_ID_6) {
		return (PLL_FPGA_PLL2RC * 2) / 2;
	} else if (id == PLL_ID_9) {
		return (PLL_FPGA_PLL2RC * 2);
	} else if (id == PLL_ID_10) {
		return (PLL_FPGA_PLL2RC * 2) / 4;
	} else if (id == PLL_ID_11) {
		return (PLL_FPGA_PLL2RC * 2);
	} else if (id == PLL_ID_12) {
		return (PLL_FPGA_PLL12SRC * 2);
	} else if (id == PLL_ID_13) {
		return (PLL_FPGA_PLL2RC * 2) / 3;
	} else if (id == PLL_ID_15) {
		return (PLL_FPGA_PLL2RC * 2);
	} else {
		return (PLL_FPGA_480SRC * 2);
	}
#else
	UINT64 pll_ratio;
	T_PLL_PLL2_CR0_REG reg0 = {0};
	T_PLL_PLL2_CR1_REG reg1 = {0};
	T_PLL_PLL2_CR2_REG reg2 = {0};
	const UINT32 pll_address[] = {PLL_PLL3_CR0_REG_OFS, PLL_PLL4_CR0_REG_OFS,
								  PLL_PLL5_CR0_REG_OFS, PLL_PLL6_CR0_REG_OFS, PLL_PLL7_CR0_REG_OFS,
								  PLL_PLL8_CR0_REG_OFS, PLL_PLL9_CR0_REG_OFS, PLL_PLL10_CR0_REG_OFS,
								  PLL_PLL11_CR0_REG_OFS, PLL_PLL12_CR0_REG_OFS, PLL_PLL13_CR0_REG_OFS,
								  PLL_PLL14_CR0_REG_OFS, PLL_PLL15_CR0_REG_OFS
								 };
	const UINT32 pll528_address[] = {PLL528_PLL3_CR0_REG_OFS, PLL528_PLL4_CR0_REG_OFS,
								     PLL528_PLL5_CR0_REG_OFS, PLL528_PLL6_CR0_REG_OFS,  PLL528_PLL7_CR0_REG_OFS,
								     PLL528_PLL8_CR0_REG_OFS, PLL528_PLL9_CR0_REG_OFS,  PLL528_PLL10_CR0_REG_OFS,
								     PLL528_PLL11_CR0_REG_OFS,PLL528_PLL12_CR0_REG_OFS, PLL528_PLL13_CR0_REG_OFS,
								     PLL528_PLL14_CR0_REG_OFS,PLL528_PLL15_CR0_REG_OFS, PLL528_PLL16_CR0_REG_OFS,
								     PLL528_PLL17_CR0_REG_OFS,PLL528_PLL18_CR0_REG_OFS
								 };

	if ((id == PLL_ID_FIXED320)) {
		return 320000000;
	} else if (id == PLL_ID_1) {
		return 480000000;
	} else {
		if (nvt_get_chip_id() == CHIP_NA51084) {
			if ((id > PLL_ID_18) || (id < PLL_ID_3)) {
				DBG_ERR("id out of range: PLL%d\r\n", id);
				return E_ID;
			}
		} else {
			if ((id > PLL_ID_15) || (id < PLL_ID_3)) {
				DBG_ERR("id out of range: PLL%d\r\n", id);
				return E_ID;
			}
		}
	}
	if (id == PLL_ID_1) {
		return 480000000;
	} else if (id == PLL_ID_4) {
		UINT32 ui_lower=0, ui_upper=0;
		T_PLL_PLL4_SSPLL1_REG pll4_ctl0_reg;

		pll_get_pll_spread_spectrum(PLL_ID_4, &ui_lower, &ui_upper);

		// Get SSC_EN
		pll4_ctl0_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL1_REG_OFS);
		if (pll4_ctl0_reg.bit.SSC_EN) {
			return ui_lower;
		} else {
			return ui_upper;
		}
	}

	if (nvt_get_chip_id() == CHIP_NA51084) {
		reg0.reg = PLL_GETREG(pll528_address[id - PLL_ID_3]);
		reg1.reg = PLL_GETREG(pll528_address[id - PLL_ID_3] + 0x04);
		reg2.reg = PLL_GETREG(pll528_address[id - PLL_ID_3] + 0x08);
	} else if (id <= PLL_ID_15){
		reg0.reg = PLL_GETREG(pll_address[id - PLL_ID_3]);
		reg1.reg = PLL_GETREG(pll_address[id - PLL_ID_3] + 0x04);
		reg2.reg = PLL_GETREG(pll_address[id - PLL_ID_3] + 0x08);
	} else {
		DBG_ERR("id out of range: PLL%d\r\n", id);
		return E_ID;
	}

	pll_ratio = (reg2.bit.PLL_RATIO2 << 16) | (reg1.bit.PLL_RATIO1 << 8) | (reg0.bit.PLL_RATIO0 << 0);

	if(id == PLL_ID_8) {
		if (nvt_get_chip_id() == CHIP_NA51084) {
			pll_ratio = (pll_ratio << 3);
#if defined(_NVT_EMULATION_)
			DBG_DUMP("[NA51084]=>PLL_id[%d], pllRatio[0x%08x]\r\n", (int)id, (int)pll_ratio);
#endif
		} else {
#if defined(_NVT_EMULATION_)
			DBG_DUMP("[NA51055]=>PLL_id[%d], pllRatio[0x%08x]\r\n", (int)id, (int)pll_ratio);
#endif
		}
	}

	return 12000000 * pll_ratio / 131072;
#endif
}

/**
    Set PLL to Spread Spectrum mode

    Set PLL to Spread Spectrum mode
    SD driver and display object will use real lower bound frequency to calculate their target frequency.
    Please aware that the real frequency is still from lower_frequency to upper_frequency.

    @note Only PLL_ID_4 support spread spectrum

    @param[in] id           PLL ID
    @param[in] lower_frequency  lower bound frequency for spread spectrum (valid range: 426000000 ~ 480000000)
    @param[in] upper_frequency  upper bound frequency for spread spectrum (valid range: 426000000 ~ 480000000)

    @return
		- @b E_OK: success
		- @b E_NOSPT: PLL id not support spread spectrum
		- @b E_MACV: PLL id is NOT disabled before invoke this function
*/
ER pll_set_pll_spread_spectrum(PLL_ID id, UINT32 lower_frequency, UINT32 upper_frequency)
{
#if defined(_NVT_FPGA_)
	DBG_ERR("NOT support in FPGA phase\r\n");
	return E_NOSPT;
#else
	unsigned long flags = 0;
	FLOAT f_percent;
	UINT32 real_low=0, real_upper=0, middle;
	UINT32 ui_ratio;
	UINT32 ui_ssp;                           // Store spread spectrum percentage (Q1.11 fixed point format)
	UINT32 ui_ssc_mul_fac;
	UINT32 ui_ssc_step = 0x02;               // 0: 2^17step, 1: 2^18step, 2: 2^19step, 3: 2^20step
	const UINT32 ui_ssc_period_value = 1 << 7; // Fix period value at 2^7

	if (nvt_get_chip_id() == CHIP_NA51084) {
		T_PLL_PLLX_SSPLL0_REG pll2sspl0Reg = {0};
		T_PLL_PLLX_SSPLL1_REG pll2sspl1Reg = {0};
		T_PLL_PLLX_SSPLL2_REG pll2sspl2Reg = {0};
		const UINT32 vPllAddr[] = {PLL528_PLL03_BASE_REG_OFS,
								   PLL528_PLL04_BASE_REG_OFS, PLL528_PLL05_BASE_REG_OFS, PLL528_PLL06_BASE_REG_OFS, PLL528_PLL07_BASE_REG_OFS,
								   PLL528_PLL08_BASE_REG_OFS, PLL528_PLL09_BASE_REG_OFS, PLL528_PLL10_BASE_REG_OFS, PLL528_PLL11_BASE_REG_OFS,
								   PLL528_PLL12_BASE_REG_OFS, PLL528_PLL13_BASE_REG_OFS, PLL528_PLL14_BASE_REG_OFS, PLL528_PLL15_BASE_REG_OFS,
								   PLL528_PLL16_BASE_REG_OFS, PLL528_PLL17_BASE_REG_OFS, PLL528_PLL18_BASE_REG_OFS
								  };

		// 528-> 0: 2^16step, 1: 2^17step, 2: 2^18step, 3: 2^19step
		ui_ssc_step = 0x03;

		if ((id == PLL_ID_3) || (id == PLL_ID_14) || (id < PLL_ID_3) || (id > PLL_ID_18)) {
			DBG_ERR("PLL%d not support spread spectrum\r\n", id);
			return E_NOSPT;
		}

		// Ensure upper bound > lower bound
		if (upper_frequency < lower_frequency) {
			UINT32 t;

			t = upper_frequency;
			upper_frequency = lower_frequency;
			lower_frequency = t;
		}

		// Check if PLL is already disabled
		if (pll_get_pll_enable(id) == TRUE) {
			DBG_ERR("PLL%d must be disabled\r\n", id);
			return E_MACV;
		}

		// Target Freq = RATIO * 12M / 131072
		ui_ratio = ((UINT64)upper_frequency) * 131072 / 12000000;
		pll_setDrvPLL(id, ui_ratio);

		// SSP% = SSC_MULFAC[7..0] * SSC_PERIOD_VALUE[7..0] / 2^(16+SSC_STEP_SEL[2..0])
		// SSP% = (uiUpperFreq - uiLowerFreq) / uiUpperFreq
		// Assume:
		//          1. SSC_STEP_SEL[2..0] = 3
		//          2. SSC_PERIOD_VALUE[7..0] = 2^7
		// ==> SSP_N / 2^M = SSC_MULFAC[7..0] * 2^7 / 2^(16+3)
		// ==> SSP_N / 2^M = SSC_MULFAC[7..0] * / 2^13
		//
		// We can treat:
		//          1. SSP_N = SSC_MULFAC[7..0]
		//          2. M = 13 (i.e. SSP% can be presented by Q1.13 format)
		// Translate SSP from float to Q1.[10+SSC_STEP_SEL]
		ui_ssp = (((FLOAT)(upper_frequency - lower_frequency)) / upper_frequency) * (1 << (16 + ui_ssc_step - 7));
		ui_ssc_mul_fac = ui_ssp;

		// Enter critical section
		loc_multi_cores(flags);

		pll2sspl1Reg.bit.SSC_MULFAC = ui_ssc_mul_fac;

		pll2sspl2Reg.bit.SSC_PERIOD_VALUE = ui_ssc_period_value;

		// 1. Setup period value
		PLL_SETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL2_REG_OFS), pll2sspl2Reg.reg);

		// 2. Setup MULFAC
		PLL_SETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL1_REG_OFS), pll2sspl1Reg.reg);

		// 3. Setup STEP_SEL
		pll2sspl0Reg.reg = PLL_GETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL0_REG_OFS));
		pll2sspl0Reg.bit.SSC_STEP_SEL = ui_ssc_step;
		PLL_SETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL0_REG_OFS), pll2sspl0Reg.reg);

		// 4. Write 1 to SSC_NEW_MODE, DSSC
		pll2sspl0Reg.bit.SSC_NEW_MODE = 1;
		pll2sspl0Reg.bit.DSSC = 1;
		PLL_SETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL0_REG_OFS), pll2sspl0Reg.reg);

		// 5. Write 1 to SSC_EN
		pll2sspl0Reg.bit.SSC_EN = 1;
		PLL_SETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL0_REG_OFS), pll2sspl1Reg.reg);



		// Leave critical section
		unl_multi_cores(flags);

		pll_get_pll_spread_spectrum(id, &real_low, &real_upper);
		if (real_low != lower_frequency) {
			DBG_WRN("PLL%d expect lower bound %d Hz, but real lower bound is %d Hz\r\n", id, (UINT)lower_frequency, (UINT)real_low);
		}
		if (real_upper != upper_frequency) {
			DBG_WRN("PLL%d expect upper bound %d Hz, but real upper bound is %d Hz\r\n", id, (UINT)upper_frequency, (UINT)real_upper);
		}
		{
			middle = (real_low + real_upper) / 2;

			f_percent = real_upper - real_low;
			f_percent /= 2 * middle;
			if (f_percent > 0.03) {
				DBG_WRN("PLL%d percentage %f exceed 0.03\r\n", id, f_percent);
			}
		}

	} else {

		// Only PLL3(DMA) and PLL4(SSPLL) support spread spectrum
		// But we only allow user to change PLL4 due to DMA frequency can not be easily changed.
		if (id != PLL_ID_4) {
			DBG_ERR("PLL%d not support spread spectrum\r\n", id + 1);
			return E_NOSPT;
		}

		// Ensure upper bound > lower bound
		if (upper_frequency < lower_frequency) {
			UINT32 t;

			t = upper_frequency;
			upper_frequency = lower_frequency;
			lower_frequency = t;
		}

		// Check if PLL is already disabled
		if (pll_get_pll_enable(id) == TRUE) {
			DBG_ERR("PLL%d must be disabled\r\n", id + 1);
			return E_MACV;
		}

		// Target Freq = RATIO * 12M / 131072
		ui_ratio = ((UINT64)upper_frequency) * 131072 / 12000000;
		pll_set_driver_pll(id, ui_ratio);

		// SSP% = SSC_MULFAC[7..0] * SSC_PERIOD_VALUE[7..0] / 2^(17+SSC_STEP_SEL[2..0])
		// SSP% = (upper_frequency - lower_frequency) / upper_frequency
		// Assume:
		//          1. SSC_STEP_SEL[2..0] = 3
		//          2. SSC_PERIOD_VALUE[7..0] = 2^7
		// ==> SSP_N / 2^M = SSC_MULFAC[7..0] * 2^7 / 2^(17+3)
		// ==> SSP_N / 2^M = SSC_MULFAC[7..0] * / 2^13
		//
		// We can treat:
		//          1. SSP_N = SSC_MULFAC[7..0]
		//          2. M = 13 (i.e. SSP% can be presented by Q1.13 format)
		// Translate SSP from float to Q1.[10+SSC_STEP_SEL]
		ui_ssp = (((FLOAT)(upper_frequency - lower_frequency)) / upper_frequency) * (1 << (17 + ui_ssc_step - 7));
		ui_ssc_mul_fac = ui_ssp;

		// Enter critical section
		loc_multi_cores(flags);
		if (id == PLL_ID_4) {
			T_PLL_PLL4_SSPLL1_REG pll4_ssp1_reg;
			T_PLL_PLL4_SSPLL2_REG pll4_sspl2_reg = {0};
			T_PLL_PLL4_SSPLL3_REG pll4_sspl3_reg = {0};
			T_PLL_PLL4_SSPLL0_REG pll4_sspl0_reg;

			pll4_sspl2_reg.bit.SSC_MULFAC = ui_ssc_mul_fac;
			pll4_sspl3_reg.bit.SSC_PERIOD_VALUE = ui_ssc_period_value;

			// 1. Setup period value
			PLL_SETREG(PLL_PLL4_SSPLL3_REG_OFS, pll4_sspl3_reg.reg);
			// 2. Setup MULFAC
			PLL_SETREG(PLL_PLL4_SSPLL2_REG_OFS, pll4_sspl2_reg.reg);
			// 3. Setup STEP_SEL
			pll4_sspl0_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL0_REG_OFS);
			pll4_sspl0_reg.bit.SSC_STEP_SEL = ui_ssc_step;
			PLL_SETREG(PLL_PLL4_SSPLL0_REG_OFS, pll4_sspl0_reg.reg);
			// 4. Write 1 to SSC_NEW_MODE, DSSC
			pll4_sspl0_reg.bit.SSC_NEW_MODE = 1;
			pll4_sspl0_reg.bit.DSSC = 1;
			PLL_SETREG(PLL_PLL4_SSPLL0_REG_OFS, pll4_sspl0_reg.reg);
			// 5. Write 1 to SSC_EN
			pll4_ssp1_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL1_REG_OFS);
			pll4_ssp1_reg.bit.SSC_EN = 1;
			PLL_SETREG(PLL_PLL4_SSPLL1_REG_OFS, pll4_ssp1_reg.reg);
		}

		// Leave critical section
		unl_multi_cores(flags);

		pll_get_pll_spread_spectrum(id, &real_low, &real_upper);
		if (real_low != lower_frequency) {
			DBG_WRN("PLL%d expect lower bound %d Hz, but real lower bound is %d Hz\r\n", id + 1, (UINT)lower_frequency, (UINT)real_low);
		}
		if (real_upper != upper_frequency) {
			DBG_WRN("PLL%d expect upper bound %d Hz, but real upper bound is %d Hz\r\n", id + 1, (UINT)upper_frequency, (UINT)real_upper);
		}
		{
			middle = (real_low + real_upper) / 2;

			f_percent = real_upper - real_low;
			f_percent /= 2 * middle;
			if (f_percent > 0.03) {
				DBG_WRN("PLL%d percentage %f exceed 0.03\r\n", id + 1, f_percent);
			}
		}

	}

	return E_OK;
#endif
}

/**
    Get PLL Spread Spectrum mode range

    Get PLL to Spread Spectrum mode range

    @note Only PLL_ID_4 support spread spectrum

    @param[in] id               PLL ID
    @param[out] pui_lower_freq    lower bound frequency for spread spectrum (valid range: 426000000 ~ 480000000)
    @param[out] pui_upper_freq    upper bound frequency for spread spectrum (valid range: 426000000 ~ 480000000)

    @return
		- @b E_OK: success
		- @b E_NOSPT: PLL id not support spread spectrum
		- @b E_MACV: PLL id is NOT disabled before invoke this function
		- @b E_CTX: pui_lower_freq or pui_upper_freq is NULL
*/
ER pll_get_pll_spread_spectrum(PLL_ID id, UINT32 *pui_lower_freq, UINT32 *pui_upper_freq)
{
#if defined(_NVT_FPGA_)
	DBG_ERR("NOT support in FPGA phase\r\n");
	return E_NOSPT;
#else
	UINT64 lower_frequency, upper_frequency;
	UINT32 ui_period, ui_mul_fac, ui_step_sel;
	UINT32 ui_ssc_en = 0;
	UINT64 pll_ratio;
	T_PLL_PLL2_CR0_REG reg0 = {0};
	T_PLL_PLL2_CR1_REG reg1 = {0};
	T_PLL_PLL2_CR2_REG reg2 = {0};

	if ((pui_lower_freq == NULL) || (pui_upper_freq == NULL)) {
		return E_CTX;
	}

	if (nvt_get_chip_id() == CHIP_NA51084) {
		T_PLL_PLLX_SSPLL0_REG pll2sspl0Reg = {0};
		T_PLL_PLLX_SSPLL1_REG pll2sspl1Reg = {0};
		T_PLL_PLLX_SSPLL2_REG pll2sspl2Reg = {0};
		const UINT32 vPllAddr[] = {PLL528_PLL03_BASE_REG_OFS,
								   PLL528_PLL04_BASE_REG_OFS, PLL528_PLL05_BASE_REG_OFS, PLL528_PLL06_BASE_REG_OFS, PLL528_PLL07_BASE_REG_OFS,
								   PLL528_PLL08_BASE_REG_OFS, PLL528_PLL09_BASE_REG_OFS, PLL528_PLL10_BASE_REG_OFS, PLL528_PLL11_BASE_REG_OFS,
								   PLL528_PLL12_BASE_REG_OFS, PLL528_PLL13_BASE_REG_OFS, PLL528_PLL14_BASE_REG_OFS, PLL528_PLL15_BASE_REG_OFS,
								   PLL528_PLL16_BASE_REG_OFS, PLL528_PLL17_BASE_REG_OFS, PLL528_PLL18_BASE_REG_OFS
								  };

		if ((id == PLL_ID_3) || (id == PLL_ID_14) || (id < PLL_ID_3) || (id > PLL_ID_18)) {
			DBG_ERR("PLL%d not support spread spectrum\r\n", id);
			return E_NOSPT;
		}

		// 1. Get period value
		pll2sspl2Reg.reg = PLL_GETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL2_REG_OFS));
		ui_period = pll2sspl2Reg.bit.SSC_PERIOD_VALUE;

		// 2. Setup MULFAC
		pll2sspl1Reg.reg = PLL_GETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL1_REG_OFS));
		ui_mul_fac = pll2sspl1Reg.bit.SSC_MULFAC;

		// 3. Setup STEP_SEL
		pll2sspl0Reg.reg = PLL_GETREG((vPllAddr[id - PLL_ID_3] + PLL_PLLX_SSPLL0_REG_OFS));
		ui_step_sel = pll2sspl0Reg.bit.SSC_STEP_SEL;
		ui_ssc_en = pll2sspl0Reg.bit.SSC_EN;


		reg0.reg = PLL_GETREG(vPllAddr[id - PLL_ID_3] + PLL_CR0_OFS);
		reg1.reg = PLL_GETREG(vPllAddr[id - PLL_ID_3] + PLL_CR1_OFS);
		reg2.reg = PLL_GETREG(vPllAddr[id - PLL_ID_3] + PLL_CR2_OFS);
		pll_ratio = (reg2.bit.PLL_RATIO2 << 16) | (reg1.bit.PLL_RATIO1 << 8) | (reg0.bit.PLL_RATIO0 << 0);

		upper_frequency = 12000000 * pll_ratio / 131072;


		if (ui_ssc_en == FALSE) {
			lower_frequency = upper_frequency;
		} else {
			lower_frequency = ((upper_frequency << (16 + ui_step_sel)) - (upper_frequency * ui_mul_fac * ui_period)) >> (16 + ui_step_sel);
		}

	} else {

		// SSP% = SSC_MULFAC[7..0] * SSC_PERIOD_VALUE[7..0] / 2^(17+SSC_STEP_SEL[2..0])
		if (id == PLL_ID_4) {
			T_PLL_PLL4_SSPLL1_REG pll4_ssp1_reg;
			T_PLL_PLL4_SSPLL2_REG pll4_sspl2_reg = {0};
			T_PLL_PLL4_SSPLL3_REG pll4_sspl3_reg = {0};
			T_PLL_PLL4_SSPLL0_REG pll4_sspl0_reg;

			// 1. Get period value
			pll4_sspl3_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL3_REG_OFS);
			ui_period = pll4_sspl3_reg.bit.SSC_PERIOD_VALUE;
			// 2. Setup MULFAC
			pll4_sspl2_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL2_REG_OFS);
			ui_mul_fac = pll4_sspl2_reg.bit.SSC_MULFAC;
			// 3. Setup STEP_SEL
			pll4_sspl0_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL0_REG_OFS);
			ui_step_sel = pll4_sspl0_reg.bit.SSC_STEP_SEL;
			// 5. Write 1 to SSC_EN
			pll4_ssp1_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL1_REG_OFS);
			ui_ssc_en = pll4_ssp1_reg.bit.SSC_EN;

			reg0.reg = PLL_GETREG(PLL_PLL4_CR0_REG_OFS);
			reg1.reg = PLL_GETREG(PLL_PLL4_CR0_REG_OFS + 0x04);
			reg2.reg = PLL_GETREG(PLL_PLL4_CR0_REG_OFS + 0x08);
			pll_ratio = (reg2.bit.PLL_RATIO2 << 16) | (reg1.bit.PLL_RATIO1 << 8) | (reg0.bit.PLL_RATIO0 << 0);

			upper_frequency = 12000000 * pll_ratio / 131072;
		} else {
			return E_NOSPT;
		}

		if (ui_ssc_en == FALSE) {
			lower_frequency = upper_frequency;
		} else {
			lower_frequency = ((upper_frequency << (17 + ui_step_sel)) - (upper_frequency * ui_mul_fac * ui_period)) >> (17 + ui_step_sel);
		}

	}

	*pui_lower_freq = (UINT32)lower_frequency;
	*pui_upper_freq = (UINT32)upper_frequency;

	return E_OK;
#endif
}

/**
    Get Input Oscillator Frequency

    Get Input Oscillator Frequency

    @return 10000000 or 12000000 Hertz.
*/
UINT32 pll_get_osc_freq(void)
{
	T_PLL_PLL_STATUS2_REG pll_status2_reg;

	pll_status2_reg.reg = PLL_GETREG(PLL_PLL_STATUS2_REG_OFS);
	return pll_status2_reg.bit.OSC_FREQ ? 10000000 : 12000000;
}

#endif

/*
    Get PWM Clock Rate.

    Get PWM Clock Rate. (for Emulation)

    @param[in] pwm_number      PWM ID (vaid value: 0 ~ 11)
    @param[out] pui_divider          Clock divider of specific PWM ID

    @return void
*/
void pll_get_pwm_clock_rate(UINT32 pwm_number, UINT32 *pui_divider)
{
	REGVALUE    reg_data;
	UINT32      ui_bit_shift;
	UINT32      ui_reg_offset;

	if (pwm_number > 11) {
		return;
	}

	if (pwm_number < 8) {
		// PWM0~3, PWM4~7
		pwm_number /= 4;
	} else {
		// PWM8~ has standalond id
		pwm_number -= (8 - 2);
	}

	ui_reg_offset = (pwm_number >> 1) << 2;
	ui_bit_shift  = (pwm_number & 0x01) << 4;

	reg_data = PLL_GETREG(PLL_PWMCR0_REG_OFS + ui_reg_offset);
	reg_data >>= ui_bit_shift;
	reg_data &= 0x3FFF;

	*pui_divider = reg_data;
}


/*
    Get TRNG RO clock sel

    Get TRNG RO clock sel

    @param[out] pui_trng_ro_sel    TRNG RO sel
    @param[out] pui_divider          Clock divider of TRNG

    @return void
*/
void pll_get_trng_ro_sel(UINT32 *pui_trng_ro_sel, UINT32 *pui_divider)
{
	REGVALUE    reg_data;


	reg_data = PLL_GETREG(PLL_TRNG_REG_OFS);

	*pui_divider = reg_data & 0xFF;
	*pui_trng_ro_sel = (reg_data & 0xF00) >> 8;

}

/*
    Module (hardware) reset on for multiple modules.

    This function will reset multiple module.

    @param[in] ui_group      reset register group.
    @param[in] ui_mask       bit mask of module to be reset

    @return void
*/
void pll_write_system_reset_multi(UINT32 ui_group, UINT32 ui_mask)
{
	UINT32      ui_reg_offset;

	if (ui_group == 0) {
		ui_reg_offset = PLL_SYS_RST0_REG_OFS;
	} else if (ui_group == 1) {
		ui_reg_offset = PLL_SYS_RST1_REG_OFS;
	} else if (ui_group == 2) {
		ui_reg_offset = PLL_SYS_RST2_REG_OFS;
	} else {
		ui_reg_offset = PLL_CPU2_RST_REG_OFS;
	}

	PLL_SETREG(ui_reg_offset, ui_mask);
}

/*
    Read Module (hardware) reset for multiple modules.

    This function will read multiple module reset status.

    @param[in] ui_group      reset register group.

    @return bit mask of reset module
*/
UINT32 pll_read_system_reset_multi(UINT32 ui_group)
{
	REGVALUE    reg_data;
	UINT32      ui_reg_offset;

	if (ui_group == 0) {
		ui_reg_offset = PLL_SYS_RST0_REG_OFS;
	} else if (ui_group == 1) {
		ui_reg_offset = PLL_SYS_RST1_REG_OFS;
	} else if (ui_group == 2) {
		ui_reg_offset = PLL_SYS_RST2_REG_OFS;
	} else {
		ui_reg_offset = PLL_CPU2_RST_REG_OFS;
	}

	reg_data = PLL_GETREG(ui_reg_offset);

	return reg_data;
}

/*
    Set Hardware Module Clock Auto Gating Configuration

    This api is used to configure the module clock auto-gating to save power.

    @param[in] clock_select1  Module Gated Clock Select ID Register 1
    @param[in] clock_select2  Module Gated Clock Select ID Register 2

    @return void
*/
void pll_config_clk_auto_gating(UINT32 clock_select1, UINT32 clock_select2)
{
	PLL_SETREG(PLL_CLKGATE0_REG_OFS, clock_select1);
	PLL_SETREG(PLL_CLKGATE1_REG_OFS, clock_select2);
}

/*
    Set Hardware APB-Clock Auto Gating Configuration

    This api is used to configure the module clock auto-gating to save power.

    @param[in] clock_select1  Module APB-Clock Gated Clock Select ID Register 1
    @param[in] clock_select2  Module APB-Clock Gated Clock Select ID Register 2
    @param[in] clock_select3  Module APB-Clock Gated Clock Select ID Register 3

    @return void
*/
void pll_config_pclk_auto_gating(UINT32 clock_select1, UINT32 clock_select2, UINT32 clock_select3)
{
	PLL_SETREG(PLL_PCLKGATE0_REG_OFS, clock_select1);
	PLL_SETREG(PLL_PCLKGATE1_REG_OFS, clock_select2);
	PLL_SETREG(PLL_PCLKGATE2_REG_OFS, clock_select3);
}

#if 1

/**
    Set module clock rate

    Set module clock rate, one module at a time.

    @param[in] ui_num	Module ID(PLL_CLKSEL_*), one module at a time.
						Please refer to pll.h
    @param[in] ui_value	Moudle clock rate(PLL_CLKSEL_*_*), please refer to pll.h

    @return void
*/
void pll_set_clock_rate(PLL_CLKSEL clk_sel, UINT32 ui_value)
{
	REGVALUE reg_data;
	UINT32 ui_mask, ui_reg_offset;
	unsigned long flags = 0;

	// Check if ui_num/ui_value exceeds limitation of NT96510
	pll_chk_clk_limitation(clk_sel, ui_value);

	ui_mask = pll_get_clock_mask(clk_sel);
	ui_reg_offset = (clk_sel >> 5) << 2;

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	reg_data = PLL_GETREG(PLL_SYS_CR_REG_OFS + ui_reg_offset);
	reg_data &= ~ui_mask;
	reg_data |= ui_value;
	PLL_SETREG(PLL_SYS_CR_REG_OFS + ui_reg_offset, reg_data);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Get module clock rate

    Get module clock rate, one module at a time.

    @param[in] ui_num	Module ID(PLL_CLKSEL_*), one module at a time.
						Please refer to pll.h

    @return Moudle clock rate(PLL_CLKSEL_*_*), please refer to pll.h
*/
UINT32 pll_get_clock_rate(PLL_CLKSEL clk_sel)
{
	UINT32      ui_mask, ui_reg_offset;
	REGVALUE    reg_data;

	if (clk_sel == PLL_CLKSEL_MAX_ITEM) {
		return PLL_CLKSEL_MAX_ITEM;
	}

	ui_mask = pll_get_clock_mask(clk_sel);
	ui_reg_offset = (clk_sel >> 5) << 2;

	reg_data = PLL_GETREG(PLL_SYS_CR_REG_OFS + ui_reg_offset);
	reg_data &= ui_mask;

	return (UINT32)reg_data;
}

/**
    Set PWM Clock Rate.

    Set PWM Clock Rate.

    @param[in] pwm_number    PWM ID (vaid value: 0 ~ 11)
    @param[in] ui_divider          Divisor number (valid value: 0x0003 ~ 0x3FFF)

    @return void
*/
void pll_set_pwm_clock_rate(UINT32 pwm_number, UINT32 ui_divider)
{
	REGVALUE    reg_data;
	UINT32      ui_bit_shift;
	UINT32      ui_reg_offset;
	unsigned long flags = 0;

	if (pwm_number > 11) {
		return;
	}

	if (pwm_number < 8) {
		// PWM0~3, PWM4~7
		pwm_number /= 4;
	} else {
		// PWM8~ has standalone id
		pwm_number -= (8 - 2);
	}

	if (ui_divider < 3) {
		DBG_WRN("div should >=3, but %d\r\n", (UINT)ui_divider);
		ui_divider = 3;
	}

	ui_reg_offset = (pwm_number >> 1) << 2;
	ui_bit_shift  = (pwm_number & 0x01) << 4;

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	reg_data = PLL_GETREG(PLL_PWMCR0_REG_OFS + ui_reg_offset);
	reg_data &= ~(0x3FFF << ui_bit_shift);
	reg_data |= ui_divider << ui_bit_shift;

	PLL_SETREG(PLL_PWMCR0_REG_OFS + ui_reg_offset, reg_data);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}



/**
    Set TRNG RO Clock Select.

    Set TRNG RO Clock Select.

    @param[in] trng_ro_select    TRNG RO Sel (vaid value: 0 ~ 7)
    @param[in] ui_divider          Divisor number (valid value: 0x0000 ~ 0xFF)

    @return void
*/
void pll_set_trng_ro_sel(UINT32 trng_ro_select, UINT32 ui_divider)
{
	REGVALUE    reg_data;
	//UINT32      ui_bit_shift;
	unsigned long flags = 0;

	if (trng_ro_select > 7) {
		return;
	}

	if (ui_divider > 0xFF) {
		DBG_WRN("div should <=0xFF, but 0x%x\r\n", (UINT)ui_divider);
		ui_divider = 0xFF;
	}

	//ui_bit_shift  = (ui_divider & 0xFF) ;

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	reg_data = PLL_GETREG(PLL_TRNG_REG_OFS);
	reg_data &= 0xFFFFF000;
	reg_data |= (ui_divider & 0xFF);
	reg_data |= ((trng_ro_select & 0xF) << 8);

	PLL_SETREG(PLL_TRNG_REG_OFS, reg_data);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}



/**
    Enable module clock

    Enable module clock, module clock must be enabled that it could be work correctly
    @param[in] num  Module enable ID, one module at a time

    @return void
*/
void pll_enable_clock(CG_EN num)
{
	REGVALUE    reg_data;
	UINT32      ui_reg_offset;
	unsigned long flags = 0;

	ui_reg_offset = (num >> 5) << 2;

	//race condition protect. enter critical section
	loc_multi_cores(flags);
	reg_data  = PLL_GETREG(PLL_CLKEN0_REG_OFS + ui_reg_offset);

	reg_data |= 1 << (num & 0x1F);

	PLL_SETREG(PLL_CLKEN0_REG_OFS + ui_reg_offset, reg_data);

	if (num == SIE_MCLK) {
		siemclk_opencnt[0]++;
	} else if (num == SIE_MCLK2) {
		siemclk_opencnt[1]++;
	} else if (num == SIE_MCLK3) {
		siemclk_opencnt[2]++;
	}

	//race condition protect. leave critical section
	unl_multi_cores(flags);

}

/**
    Disable module clock

    Disable module clock

    @param[in] num  Module enable ID, one module at a time

    @return void
*/
void pll_disable_clock(CG_EN num)
{
	REGVALUE    reg_data;
	UINT32      ui_reg_offset;
	unsigned long flags = 0;

	ui_reg_offset = (num >> 5) << 2;

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	if (num == SIE_MCLK) {
		if(siemclk_opencnt[0] > 0)
			siemclk_opencnt[0]--;

		if(siemclk_opencnt[0] > 0) {
			unl_multi_cores(flags);
			return;
		}
	} else if (num == SIE_MCLK2) {
		if(siemclk_opencnt[1] > 0)
			siemclk_opencnt[1]--;

		if(siemclk_opencnt[1] > 0) {
			unl_multi_cores(flags);
			return;
		}
	} else if (num == SIE_MCLK3) {
		if(siemclk_opencnt[2] > 0)
			siemclk_opencnt[2]--;

		if(siemclk_opencnt[2] > 0) {
			unl_multi_cores(flags);
			return;
		}
	}

	reg_data = PLL_GETREG(PLL_CLKEN0_REG_OFS + ui_reg_offset);

	reg_data &= ~(1 << (num & 0x1F));

	PLL_SETREG(PLL_CLKEN0_REG_OFS + ui_reg_offset, reg_data);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Check module clock

    Check module clock

    @param[in] num    Module ID, one module at a time

    @return
		- @b TRUE:    Clock is enabled
		- @b FALSE:   Clock is disabled
*/
BOOL pll_is_clock_enabled(CG_EN num)
{
	REGVALUE    reg_data;
	UINT32      ui_reg_offset;

	ui_reg_offset = (num >> 5) << 2;

	reg_data = PLL_GETREG(PLL_CLKEN0_REG_OFS + ui_reg_offset);
	reg_data &= 1 << (num & 0x1F);

	return (reg_data != 0);
}

/**
    Set the module clock frequency

    This api setup the module clock frequency by chnaging module clock divider.
    If the module has multiple source clock choices, user must set the correct
    source clock before calling this API.
\n  If the target frequency can not well divided from source frequency,this api
    would output warning message.

    @param[in] clock_id    Module select ID, refer to structure PLL_CLKFREQ.
    @param[in] ui_freq   Target clock frequency. Unit in Hertz.

    @return
     - @b E_ID:     clock_id is not support in this API.
     - @b E_PAR:    Target frequency can not be divided with no remainder.
     - @b E_OK:     Done and success.
*/
ER pll_set_clock_freq(PLL_CLKFREQ clock_id, UINT32 ui_freq)
{
	UINT32 source_clock, divider, clock_source;

	if (clock_id >= PLL_CLKFREQ_MAXNUM) {
		return E_ID;
	}

	// Get Src Clock Frequency
	switch (clock_id) {
	case SIEMCLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLKSRC);
			if (clock_source == PLL_CLKSEL_SIE_MCLKSRC_480) {
				source_clock = pll_get_pll_freq(PLL_ID_1);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLKSRC_PLL5) {
				source_clock = pll_get_pll_freq(PLL_ID_5);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLKSRC_PLL10) {
				source_clock = pll_get_pll_freq(PLL_ID_10);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLKSRC_PLL12) {
				source_clock = pll_get_pll_freq(PLL_ID_12);
			} else {
				return E_PAR;
			}
		}
		break;
	case SIEMCLK2_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC);
			if (clock_source == PLL_CLKSEL_SIE_MCLK2SRC_480) {
				source_clock = pll_get_pll_freq(PLL_ID_1);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLK2SRC_PLL5) {
				source_clock = pll_get_pll_freq(PLL_ID_5);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLK2SRC_PLL10) {
				source_clock = pll_get_pll_freq(PLL_ID_10);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLK2SRC_PLL12) {
				source_clock = pll_get_pll_freq(PLL_ID_12);
			} else {
				return E_PAR;
			}
		}
		break;
	case SIEMCLK_12SYNC_FREQ: {
			UINT32 clock_source2;

			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLKSRC);
			clock_source2 = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC);

			if ((clock_source == PLL_CLKSEL_SIE_MCLKSRC_480) && (clock_source2 == PLL_CLKSEL_SIE_MCLK2SRC_480)) {
				source_clock = pll_get_pll_freq(PLL_ID_1);
			} else if ((clock_source == PLL_CLKSEL_SIE_MCLKSRC_PLL5) && (clock_source2 == PLL_CLKSEL_SIE_MCLK2SRC_PLL5)) {
				source_clock = pll_get_pll_freq(PLL_ID_5);
			} else if ((clock_source == PLL_CLKSEL_SIE_MCLKSRC_PLL10) && (clock_source2 == PLL_CLKSEL_SIE_MCLK2SRC_PLL10)) {
				source_clock = pll_get_pll_freq(PLL_ID_10);
			} else if ((clock_source == PLL_CLKSEL_SIE_MCLKSRC_PLL12) && (clock_source2 == PLL_CLKSEL_SIE_MCLK2SRC_PLL12)) {
				source_clock = pll_get_pll_freq(PLL_ID_12);
			} else {
				DBG_ERR("SIEMCLK_12SYNC_FREQ CLK Src not matched\r\n");
				return E_PAR;
			}

		}
		break;


	case SIEMCLK3_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC);
			if (clock_source == PLL_CLKSEL_SIE_MCLK3SRC_480) {
				source_clock = pll_get_pll_freq(PLL_ID_1);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLK3SRC_PLL5) {
				source_clock = pll_get_pll_freq(PLL_ID_5);
			} else if (clock_source == PLL_CLKSEL_SIE_MCLK3SRC_PLL10) {
				if (nvt_get_chip_id() == CHIP_NA51084) {// 528 sie_mclk3 source is PLL_ID_18
					source_clock = pll_get_pll_freq(PLL_ID_18);
				} else {
					source_clock = pll_get_pll_freq(PLL_ID_10);
				}
			} else if (clock_source == PLL_CLKSEL_SIE_MCLK3SRC_PLL12) {
				source_clock = pll_get_pll_freq(PLL_ID_12);
			} else {
				return E_PAR;
			}
		}
		break;

	case SIECLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC);
			switch (clock_source) {
			case PLL_CLKSEL_SIE_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_192:
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_PLL2RC * 2 / 4;
#else
				source_clock = 192000000;
#endif
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case SIE2CLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC);
			switch (clock_source) {
			case PLL_CLKSEL_SIE2_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;

			case PLL_CLKSEL_SIE2_CLKSRC_192:
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_PLL2RC * 2 / 4;
#else
				source_clock = 192000000;
#endif
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case SIE3CLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE3_CLKSRC);
			switch (clock_source) {
			case PLL_CLKSEL_SIE3_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;

			case PLL_CLKSEL_SIE2_CLKSRC_192:
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_PLL2RC * 2 / 4;
#else
				source_clock = 192000000;
#endif
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case SIE4CLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE4_CLKSRC);
			switch (clock_source) {
			case PLL_CLKSEL_SIE4_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE4_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE4_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE4_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE4_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE4_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;

			case PLL_CLKSEL_SIE4_CLKSRC_192:
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_PLL2RC * 2 / 4;
#else
				source_clock = 192000000;
#endif
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case SIE5CLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SIE5_CLKSRC);
			switch (clock_source) {
			case PLL_CLKSEL_SIE5_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE5_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE5_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE5_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE5_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE5_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;

			case PLL_CLKSEL_SIE5_CLKSRC_192:
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_PLL2RC * 2 / 4;
#else
				source_clock = 192000000;
#endif
				break;
			default:
				return E_PAR;
			}
		}
		break;


	case IDECLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC);
			if (clock_source == PLL_CLKSEL_IDE_CLKSRC_480) {
				source_clock = pll_get_pll_freq(PLL_ID_1);
			} else if (clock_source == PLL_CLKSEL_IDE_CLKSRC_PLL6) {
				source_clock = pll_get_pll_freq(PLL_ID_6);
			} else if (clock_source == PLL_CLKSEL_IDE_CLKSRC_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else if (clock_source == PLL_CLKSEL_IDE_CLKSRC_PLL9) {
				source_clock = pll_get_pll_freq(PLL_ID_9);
			} else {
				return E_PAR;
			}
		}
		break;

	case IDEOUTIFCLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC);
			if (clock_source == PLL_CLKSEL_IDE_CLKSRC_480) {
				source_clock = pll_get_pll_freq(PLL_ID_1);
			} else if (clock_source == PLL_CLKSEL_IDE_CLKSRC_PLL6) {
				source_clock = pll_get_pll_freq(PLL_ID_6);
			} else if (clock_source == PLL_CLKSEL_IDE_CLKSRC_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else if (clock_source == PLL_CLKSEL_IDE_CLKSRC_PLL9) {
				source_clock = pll_get_pll_freq(PLL_ID_9);
			}

			else {
				return E_PAR;
			}
		}
		break;

	case SPCLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SP);
			if (clock_source == PLL_CLKSEL_SP_480) {
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_480SRC;
#else
				source_clock = 480000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SP_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else if (clock_source == PLL_CLKSEL_SP_PLL5) {
				source_clock = pll_get_pll_freq(PLL_ID_5);
			} else if (clock_source == PLL_CLKSEL_SP_PLL6) {
				source_clock = pll_get_pll_freq(PLL_ID_6);
			} else {
				return E_PAR;
			}
		}
		break;

	case SPCLK2_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SP2);
			if (clock_source == PLL_CLKSEL_SP2_480) {
#if defined(_NVT_FPGA_)
				source_clock = PLL_FPGA_480SRC;
#else
				source_clock = 480000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SP2_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else if (clock_source == PLL_CLKSEL_SP2_PLL5) {
				source_clock = pll_get_pll_freq(PLL_ID_5);
			} else if (clock_source == PLL_CLKSEL_SP2_PLL6) {
				source_clock = pll_get_pll_freq(PLL_ID_6);
			} else {
				return E_PAR;
			}
		}
		break;

	case ADOCLK_FREQ: {
			source_clock = pll_get_pll_freq(PLL_ID_7);
			source_clock /= ((pll_get_clock_rate(PLL_CLKSEL_ADO_OSR_CLKDIV) >> (PLL_CLKSEL_ADO_OSR_CLKDIV & 0x1F))+1);
		}
		break;

	case ADOOSRCLK_FREQ :{
			source_clock = pll_get_pll_freq(PLL_ID_7);
		}
		break;

	case SDIOCLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SDIO);
			if (clock_source == PLL_CLKSEL_SDIO_192) {
#if defined(_NVT_FPGA_)
				source_clock = (PLL_FPGA_480SRC * 2) / 8;
#else
				source_clock = 192000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SDIO_480) {
#if defined(_NVT_FPGA_)
				source_clock = (PLL_FPGA_480SRC * 2) / 6;
#else
				source_clock = 480000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SDIO_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else {
				return E_PAR;
			}
		}
		break;
	case SDIO2CLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SDIO2);
			if (clock_source == PLL_CLKSEL_SDIO2_192) {
#if defined(_NVT_FPGA_)
				source_clock = (PLL_FPGA_480SRC * 2) / 8;
#else
				source_clock = 192000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SDIO2_480) {
#if defined(_NVT_FPGA_)
				source_clock = (PLL_FPGA_480SRC * 2) / 6;
#else
				source_clock = 480000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SDIO2_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else {
				return E_PAR;
			}
		}
		break;
	case SDIO3CLK_FREQ: {
			clock_source = pll_get_clock_rate(PLL_CLKSEL_SDIO3);
			if (clock_source == PLL_CLKSEL_SDIO3_192) {
#if defined(_NVT_FPGA_)
				source_clock = (PLL_FPGA_480SRC * 2) / 8;
#else
				source_clock = 192000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SDIO3_480) {
#if defined(_NVT_FPGA_)
				source_clock = (PLL_FPGA_480SRC * 2) / 6;
#else
				source_clock = 480000000;
#endif
			} else if (clock_source == PLL_CLKSEL_SDIO3_PLL4) {
				source_clock = pll_get_pll_freq(PLL_ID_4);
			} else {
				return E_PAR;
			}
		}
		break;


	case SPICLK_FREQ:
	case SPI2CLK_FREQ:
	case SPI3CLK_FREQ:
	case SPI4CLK_FREQ:
	case SPI5CLK_FREQ:
#if defined(_NVT_FPGA_)
		source_clock = 27000000 / 2;
//		source_clock = PLL_FPGA_480SRC / 2;
#else
		source_clock = 192000000;
#endif
		break;

	default:
		return E_PAR;
	}


	// Calculate the clock divider value
	divider = (source_clock + ui_freq - 1) / ui_freq;

	// prevent error case
	if (divider == 0) {
		divider = 1;
	}
	// Set Clock divider
	switch (clock_id) {
	case SIEMCLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKDIV, PLL_SIE_MCLKDIV(divider - 1));
		break;
	case SIEMCLK2_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2DIV, PLL_SIE_MCLK2DIV(divider - 1));
		break;
	case SIEMCLK_12SYNC_FREQ: {
			REGVALUE reg_data;
			unsigned long flags = 0;

			loc_multi_cores(flags);

			reg_data = PLL_GETREG(PLL_IPP_CLKDIV_REG_OFS);
			reg_data &= 0xFFFF0000;
			PLL_SETREG(PLL_IPP_CLKDIV_REG_OFS, reg_data);

			vos_util_delay_us_polling(2);

			reg_data = reg_data+(divider - 1)+((divider - 1)<<8);
			PLL_SETREG(PLL_IPP_CLKDIV_REG_OFS, reg_data);

			unl_multi_cores(flags);

		} break;

	case SIEMCLK3_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3DIV, PLL_SIE_MCLK3DIV(divider - 1));
		break;
	case SIECLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE_CLKDIV, PLL_SIE_CLKDIV(divider - 1));
		break;
	case SIE2CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE2_CLKDIV, PLL_SIE2_CLKDIV(divider - 1));
		break;
	case SIE3CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE3_CLKDIV, PLL_SIE3_CLKDIV(divider - 1));
		break;
	case SIE4CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE4_CLKDIV, PLL_SIE4_CLKDIV(divider - 1));
		break;
	case SIE5CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SIE5_CLKDIV, PLL_SIE5_CLKDIV(divider - 1));
		break;

	case IDECLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_IDE_CLKDIV, PLL_IDE_CLKDIV(divider - 1));
		break;
	case IDEOUTIFCLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_IDE_OUTIF_CLKDIV, PLL_IDE_OUTIF_CLKDIV(divider - 1));
		break;
	case SPCLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SP_CLKDIV, PLL_SP_CLKDIV(divider - 1));
		break;
	case SPCLK2_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SP2_CLKDIV, PLL_SP2_CLKDIV(divider - 1));
		break;
	case ADOCLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_ADO_CLKDIV, PLL_ADO_CLKDIV(divider - 1));
		break;
	case ADOOSRCLK_FREQ:
#if !defined(_NVT_FPGA_)
		if (divider == 1) {
			// ADO CLKDIV register should >= 1
			divider = 2;
		}
#endif
		pll_set_clock_rate(PLL_CLKSEL_ADO_OSR_CLKDIV, PLL_ADO_OSR_CLKDIV(divider - 1));
		break;		
	case SDIOCLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SDIO_CLKDIV, PLL_SDIO_CLKDIV(divider - 1));
		break;
	case SDIO2CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SDIO2_CLKDIV, PLL_SDIO2_CLKDIV(divider - 1));
		break;
	case SDIO3CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SDIO3_CLKDIV, PLL_SDIO3_CLKDIV(divider - 1));
		break;
	case SPICLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SPI_CLKDIV, PLL_SPI_CLKDIV(divider - 1));
		break;
	case SPI2CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SPI2_CLKDIV, PLL_SPI2_CLKDIV(divider - 1));
		break;
	case SPI3CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SPI3_CLKDIV, PLL_SPI3_CLKDIV(divider - 1));
		break;
	case SPI4CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SPI4_CLKDIV, PLL_SPI4_CLKDIV(divider - 1));
		break;
	case SPI5CLK_FREQ:
		pll_set_clock_rate(PLL_CLKSEL_SPI5_CLKDIV, PLL_SPI5_CLKDIV(divider - 1));
		break;
	//coverity[dead_error_begin]
	default :
		return E_PAR;
	}

	// Output warning msg if target freq can not well divided from source freq
	if (source_clock % ui_freq) {
		UINT32 ui_real_freq = source_clock / divider;

		// Truncate inaccuray under 1000 Hz
		ui_real_freq = (ui_real_freq + 50) / 1000;
		ui_freq /= 1000;
		if (ui_freq != ui_real_freq) {
			DBG_WRN("Target(%d) freq can not be divided with no remainder! Result is %dHz.\r\n", clock_id, (UINT)(source_clock / divider));
			return E_PAR;
		}
	}

	return E_OK;
}

/**
    Get the module clock frequency

    This api get the module clock frequency.

    @param[in] clock_id    Module select ID, refer to structure PLL_CLKFREQ.
    @param[out] p_freq   Return clock frequency. Unit in Hertz.

    @return
     - @b E_ID:     clock_id is not support in this API.
     - @b E_PAR:    Target frequency can not be divided with no remainder.
     - @b E_OK:     Done and success.
*/
ER pll_get_clock_freq(PLL_CLKFREQ clock_id, UINT32 *p_freq)
{
	UINT32 source_clock = 0, divider = 0, clock_source = 0;

	if (clock_id >= PLL_CLKFREQ_MAXNUM) {
		return E_ID;
	}

	if (p_freq == NULL) {
		DBG_ERR("input p_freq is NULL\r\n");
		return E_PAR;
	}

	// Get Src Clock Frequency
	switch (clock_id) {

	case SIEMCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLKSRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLKDIV) >> (PLL_CLKSEL_SIE_MCLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SIE_MCLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_MCLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_MCLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;

			default:
				return E_PAR;
			}
		}
		break;
	case SIEMCLK2_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK2DIV) >> (PLL_CLKSEL_SIE_MCLK2DIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SIE_MCLK2SRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_MCLK2SRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_MCLK2SRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;

			default:
				return E_PAR;
			}
		}
		break;
	case SIECLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_SIE_CLKDIV) >> (PLL_CLKSEL_SIE_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SIE_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;
			case PLL_CLKSEL_SIE_CLKSRC_192:
				source_clock = 192000000;
				break;
			default:
				return E_PAR;
			}
		}
		break;
	case SIE2CLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKDIV) >> (PLL_CLKSEL_SIE2_CLKDIV  & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SIE2_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;
			case PLL_CLKSEL_SIE2_CLKSRC_192:
				source_clock = 192000000;
				break;
			default:
				return E_PAR;
			}
		}
		break;
	case SIE3CLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SIE3_CLKSRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_SIE3_CLKDIV) >> (PLL_CLKSEL_SIE3_CLKDIV  & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SIE3_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL13:
				source_clock = pll_get_pll_freq(PLL_ID_13);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL12:
				source_clock = pll_get_pll_freq(PLL_ID_12);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_PLL10:
				source_clock = pll_get_pll_freq(PLL_ID_10);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_320:
				source_clock = pll_get_pll_freq(PLL_ID_FIXED320);
				break;
			case PLL_CLKSEL_SIE3_CLKSRC_192:
				source_clock = 192000000;
				break;
			default:
				return E_PAR;
			}
		}
		break;
	case IDECLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_IDE_CLKDIV) >> (PLL_CLKSEL_IDE_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_IDE_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL6:
				source_clock = pll_get_pll_freq(PLL_ID_6);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL9:
				source_clock = pll_get_pll_freq(PLL_ID_9);
				break;

			default:
				return E_PAR;
			}
		}
		break;
	case IDEOUTIFCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC);
			divider = pll_get_clock_rate(PLL_CLKSEL_IDE_OUTIF_CLKDIV) >> (PLL_CLKSEL_IDE_OUTIF_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_IDE_CLKSRC_480:
				source_clock = pll_get_pll_freq(PLL_ID_1);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL6:
				source_clock = pll_get_pll_freq(PLL_ID_6);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			case PLL_CLKSEL_IDE_CLKSRC_PLL9:
				source_clock = pll_get_pll_freq(PLL_ID_9);
				break;

			default:
				return E_PAR;
			}
		}
		break;
	case SPCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SP);
			divider = pll_get_clock_rate(PLL_CLKSEL_SP_CLKDIV) >> (PLL_CLKSEL_SP_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SP_480:
				source_clock = 480000000;
				break;
			case PLL_CLKSEL_SP_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			case PLL_CLKSEL_SP_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SP_PLL6:
				source_clock = pll_get_pll_freq(PLL_ID_6);
				break;
			default:
				return E_PAR;
			}
		}
		break;
	case SPCLK2_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SP2);
			divider = pll_get_clock_rate(PLL_CLKSEL_SP2_CLKDIV) >> (PLL_CLKSEL_SP2_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SP2_480:
				source_clock = 480000000;
				break;
			case PLL_CLKSEL_SP2_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			case PLL_CLKSEL_SP2_PLL5:
				source_clock = pll_get_pll_freq(PLL_ID_5);
				break;
			case PLL_CLKSEL_SP2_PLL6:
				source_clock = pll_get_pll_freq(PLL_ID_6);
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case ADOCLK_FREQ: {
			source_clock  = pll_get_pll_freq(PLL_ID_7);
			divider = pll_get_clock_rate(PLL_CLKSEL_ADO_CLKDIV) >> (PLL_CLKSEL_ADO_CLKDIV & 0x1F);
		}
		break;
	case ADOOSRCLK_FREQ: {
			source_clock  = pll_get_pll_freq(PLL_ID_7);
			divider = pll_get_clock_rate(PLL_CLKSEL_ADO_OSR_CLKDIV) >> (PLL_CLKSEL_ADO_OSR_CLKDIV & 0x1F);
		}
		break;		
	case SDIOCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SDIO);
			divider = pll_get_clock_rate(PLL_CLKSEL_SDIO_CLKDIV) >> (PLL_CLKSEL_SDIO_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SDIO_192:
				source_clock = 192000000;
				break;
			case PLL_CLKSEL_SDIO_480:
				source_clock = 480000000;
				break;
			case PLL_CLKSEL_SDIO_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			default:
				return E_PAR;
			}
		}
		break;
	case SDIO2CLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SDIO2);
			divider = pll_get_clock_rate(PLL_CLKSEL_SDIO2_CLKDIV) >> (PLL_CLKSEL_SDIO2_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SDIO2_192:
				source_clock = 192000000;
				break;
			case PLL_CLKSEL_SDIO2_480:
				source_clock = 480000000;
				break;
			case PLL_CLKSEL_SDIO2_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			default:
				return E_PAR;
			}
		}
		break;
	case SDIO3CLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_SDIO3);
			divider = pll_get_clock_rate(PLL_CLKSEL_SDIO3_CLKDIV) >> (PLL_CLKSEL_SDIO3_CLKDIV & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_SDIO3_192:
				source_clock = 192000000;
				break;
			case PLL_CLKSEL_SDIO3_480:
				source_clock = 480000000;
				break;
			case PLL_CLKSEL_SDIO3_PLL4:
				source_clock = pll_get_pll_freq(PLL_ID_4);
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case SPICLK_FREQ: {
#if defined(_NVT_FPGA_)
			source_clock = 27000000 / 2;
//		source_clock = PLL_FPGA_480SRC / 2;
#else
			source_clock = 192000000;
#endif
			divider = pll_get_clock_rate(PLL_CLKSEL_SPI_CLKDIV) >> (PLL_CLKSEL_SPI_CLKDIV & 0x1F);
		}
		break;
	case SPI2CLK_FREQ: {
#if defined(_NVT_FPGA_)
			source_clock = 27000000 / 2;
		//		source_clock = PLL_FPGA_480SRC / 2;
#else
			source_clock = 192000000;
#endif
			divider = pll_get_clock_rate(PLL_CLKSEL_SPI2_CLKDIV) >> (PLL_CLKSEL_SPI2_CLKDIV  & 0x1F);
		}
		break;
	case SPI3CLK_FREQ: {
#if defined(_NVT_FPGA_)
			source_clock = 27000000 / 2;
		//		source_clock = PLL_FPGA_480SRC / 2;
#else
			source_clock = 192000000;
#endif
			divider = pll_get_clock_rate(PLL_CLKSEL_SPI3_CLKDIV) >> (PLL_CLKSEL_SPI3_CLKDIV & 0x1F);
		}
		break;
	case SPI4CLK_FREQ: {
#if defined(_NVT_FPGA_)
			source_clock = 27000000 / 2;
		//		source_clock = PLL_FPGA_480SRC / 2;
#else
			source_clock = 192000000;
#endif
			divider = pll_get_clock_rate(PLL_CLKSEL_SPI4_CLKDIV) >> (PLL_CLKSEL_SPI4_CLKDIV & 0x1F);
		}
		break;
	case SPI5CLK_FREQ: {
#if defined(_NVT_FPGA_)
			source_clock = 27000000 / 2;
		//		source_clock = PLL_FPGA_480SRC / 2;
#else
			source_clock = 192000000;
#endif
			divider = pll_get_clock_rate(PLL_CLKSEL_SPI5_CLKDIV) >> (PLL_CLKSEL_SPI5_CLKDIV & 0x1F);
		}
		break;
	case CPUCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_CPU);

			switch (clock_source) {
			case PLL_CLKSEL_CPU_80:
				source_clock = 80000000;
				break;
			case PLL_CLKSEL_CPU_PLL8:
				source_clock = pll_get_pll_freq(PLL_ID_8);
				break;
			case PLL_CLKSEL_CPU_480:
				source_clock = 480000000;
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case APBCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_APB);

			switch (clock_source) {
			case PLL_CLKSEL_APB_48:
				source_clock = 48000000;
				break;
			case PLL_CLKSEL_APB_60:
				source_clock = 60000000;
				break;
			case PLL_CLKSEL_APB_80:
				source_clock = 80000000;
				break;
			case PLL_CLKSEL_APB_120:
				source_clock = 120000000;
				break;
			default:
				return E_PAR;
			}
		}
		break;

	case TRNGCLK_FREQ: {
			clock_source  = pll_get_clock_rate(PLL_CLKSEL_TRNG);
			divider = pll_get_clock_rate(PLL_CLKSEL_TRNG_CLKDIV) >> (PLL_CLKSEL_TRNG_CLKDIV  & 0x1F);

			switch (clock_source) {
			case PLL_CLKSEL_TRNG_240:
				source_clock = 240000000;
				break;
			case PLL_CLKSEL_TRNG_160:
				source_clock = 160000000;
				break;
			default:
				return E_PAR;
			}
		}
		break;

	default:
		return E_PAR;
	}

	*p_freq = source_clock / (divider + 1);

	return E_OK;
}

/**
    Module (hardware) reset on.

    This function will reset module.

    @param[in] num  Reset bit number of type CG_RSTN, only one at a time

    @return void
*/
void pll_enable_system_reset(CG_RSTN num)
{
	REGVALUE    reg_data;
	UINT32      ui_reg_offset;
	unsigned long flags = 0;

	ui_reg_offset = (num >> 5) << 2;

	//race condition protect. enter critical section
	loc_multi_cores(flags);
	reg_data = PLL_GETREG(PLL_SYS_RST0_REG_OFS + ui_reg_offset);
	reg_data &= ~(1 << (num & 0x1F));

	PLL_SETREG(PLL_SYS_RST0_REG_OFS + ui_reg_offset, reg_data);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Module (hardware) reset off.

    This fulction will enable module.

    @param[in] num  Reset bit number of type CG_RSTN, only one at a time

    @return void
*/
void pll_disable_system_reset(CG_RSTN num)
{
	REGVALUE    reg_data;
	UINT32      ui_reg_offset;
	unsigned long flags = 0;

	ui_reg_offset = (num >> 5) << 2;

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	reg_data = PLL_GETREG(PLL_SYS_RST0_REG_OFS + ui_reg_offset);
	reg_data |= 1 << (num & 0x1F);

	PLL_SETREG(PLL_SYS_RST0_REG_OFS + ui_reg_offset, reg_data);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Hardware Module Clock Auto Gating Enable

    This api is used to enable the module clock auto-gating to save power.

    @param[in] clock_select  Module Gated Clock Select ID, refer to structure GATECLK.

    @return void

*/
void pll_set_clk_auto_gating(M_GATECLK clock_select)
{
	T_PLL_CLKGATE1_REG  reg_gate_clk;
	UINT32              ofs, bit;
	unsigned long flags = 0;

	if (!((clock_select > M_GCLK_BASE) && (clock_select < MCLKGAT_MAXNUM))) {
		DBG_ERR("please use M_GATECLK instead!\r\n");
		return;
	}

	clock_select = clock_select - M_GCLK_BASE;

	ofs = (clock_select >> 5) << 2;
	bit = (0x01 << (clock_select & 0x1F));

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	reg_gate_clk.reg = PLL_GETREG(PLL_CLKGATE0_REG_OFS + ofs);
	reg_gate_clk.reg |= bit;
	PLL_SETREG(PLL_CLKGATE0_REG_OFS + ofs, reg_gate_clk.reg);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Hardware Module Clock Auto Gating Disable

    This api is used to disable the module clock auto-gating.

    @param[in] clock_select  Module Gated Clock Select ID, refer to structure GATECLK.

    @return void

*/
void pll_clear_clk_auto_gating(M_GATECLK clock_select)
{
	T_PLL_CLKGATE1_REG  reg_gate_clk;
	UINT32              ofs, bit;
	unsigned long flags = 0;

	if (!((clock_select > M_GCLK_BASE) && (clock_select < MCLKGAT_MAXNUM))) {
		DBG_ERR("please use M_GATECLK instead!\r\n");
		return;
	}

	clock_select = clock_select - M_GCLK_BASE;

	ofs = (clock_select >> 5) << 2;
	bit = (0x01 << (clock_select & 0x1F));

	//race condition protect. enter critical section
	loc_multi_cores(flags);
	reg_gate_clk.reg = PLL_GETREG(PLL_CLKGATE0_REG_OFS + ofs);
	reg_gate_clk.reg &= ~bit;
	PLL_SETREG(PLL_CLKGATE0_REG_OFS + ofs, reg_gate_clk.reg);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Get Hardware Module Clock Auto Gating Enable/Disable

    This api is used to get the module clock auto-gating status.

    @param[in] clock_select  Module Gated Clock Select ID, refer to structure GATECLK.

    @return
     - @b TRUE:  Auto-Gating is ENABLE.
     - @b FALSE: Auto-Gating is DISABLE.

*/
BOOL pll_get_clk_auto_gating(M_GATECLK clock_select)
{
	T_PLL_CLKGATE1_REG  reg_gate_clk;
	UINT32              ofs, bit;

	if (!((clock_select > M_GCLK_BASE) && (clock_select < MCLKGAT_MAXNUM))) {
		DBG_ERR("please use M_GATECLK instead!\r\n");
		return FALSE;
	}

	clock_select = clock_select - M_GCLK_BASE;

	ofs = (clock_select >> 5) << 2;
	bit = (0x01 << (clock_select & 0x1F));

	reg_gate_clk.reg = PLL_GETREG(PLL_CLKGATE0_REG_OFS + ofs);
	return (reg_gate_clk.reg & bit) > 0;
}

/**
    Hardware PCLK Clock Auto Gating Enable

    This api is used to enable the module PCLK auto-gating to save power.

    @param[in] clock_select  Module Gated Clock Select ID, refer to structure GATECLK.

    @return void
*/
void pll_set_pclk_auto_gating(GATECLK clock_select)
{
	T_PLL_PCLKGATE1_REG  reg_gate_clk;
	UINT32               ofs, bit;
	unsigned long flags = 0;

	if (clock_select >= PCLKGAT_MAXNUM) {
		return;
	}

	ofs = (clock_select >> 5) << 2;
	bit = (0x01 << (clock_select & 0x1F));

	//race condition protect. enter critical section
	loc_multi_cores(flags);

	reg_gate_clk.reg = PLL_GETREG(PLL_PCLKGATE0_REG_OFS + ofs);
	reg_gate_clk.reg |= bit;
	PLL_SETREG(PLL_PCLKGATE0_REG_OFS + ofs, reg_gate_clk.reg);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Hardware PCLK Clock Auto Gating Disable

    This api is used to disable the module PCLK auto-gating.

    @param[in] clock_select  Module Gated Clock Select ID, refer to structure GATECLK.

    @return void
*/
void pll_clear_pclk_auto_gating(GATECLK clock_select)
{
	T_PLL_PCLKGATE1_REG  reg_gate_clk;
	UINT32               ofs, bit;
	unsigned long flags = 0;

	if (clock_select >= PCLKGAT_MAXNUM) {
		return;
	}

	ofs = (clock_select >> 5) << 2;
	bit = (0x01 << (clock_select & 0x1F));

	//race condition protect. enter critical section
	loc_multi_cores(flags);
	reg_gate_clk.reg = PLL_GETREG(PLL_PCLKGATE0_REG_OFS + ofs);
	reg_gate_clk.reg &= ~bit;
	PLL_SETREG(PLL_PCLKGATE0_REG_OFS + ofs, reg_gate_clk.reg);

	//race condition protect. leave critical section
	unl_multi_cores(flags);
}

/**
    Get Hardware PCLK Clock Auto Gating Enable/Disable

    This api is used to get the module PCLK auto-gating status.

    @param[in] clock_select  Module Gated Clock Select ID, refer to structure GATECLK.

    @return
     - @b TRUE:  Auto-Gating is ENABLE.
     - @b FALSE: Auto-Gating is DISABLE.
*/
BOOL pll_get_pclk_auto_gating(GATECLK clock_select)
{
	T_PLL_PCLKGATE1_REG  reg_gate_clk;
	UINT32               ofs, bit;

	if (clock_select >= PCLKGAT_MAXNUM) {
		return FALSE;
	}

	ofs = (clock_select >> 5) << 2;
	bit = (0x01 << (clock_select & 0x1F));

	reg_gate_clk.reg = PLL_GETREG(PLL_PCLKGATE0_REG_OFS + ofs);
	return (reg_gate_clk.reg & bit) > 0;

}

#endif


/**
    Init platform pll setting

    @void

    @return
     - @b E_ID:     clock_id is not support in this API.
     - @b E_OK:     Done and success.
*/
ER pll_init(void)
{
    ER er;

    //PLL13 set to 280Mhz (for IPP max freq)
    er = pll_set_pll_freq(PLL_ID_13, 280000000);

	return er;
}


#if 0
static BOOL pll_print_pll_info_to_uart(CHAR *str_cmd)
{
	T_PLL_PLL4_SSPLL1_REG pll4_ssp1_reg;
	UINT32  lower_frequency = 0, upper_frequency = 0;

	pll4_ssp1_reg.reg = PLL_GETREG(PLL_PLL4_SSPLL1_REG_OFS);
	pll_get_pll_spread_spectrum(PLL_ID_4, &lower_frequency, &upper_frequency);

	DBG_DUMP("       EN        FREQ  SScEn    SSC_LOW   SSC_HIGH\r\n");
	DBG_DUMP("PLL1    1   960000000\r\n");
	DBG_DUMP("PLL3    %d   %9d\r\n", pll_get_pll_enable(PLL_ID_3), pll_get_pll_freq(PLL_ID_3));
	DBG_DUMP("PLL4    %d   %9d      %d  %9d  %9d\r\n", pll_get_pll_enable(PLL_ID_4), pll_get_pll_freq(PLL_ID_4), pll4_ssp1_reg.bit.SSC_EN, lower_frequency, upper_frequency);
	DBG_DUMP("PLL5    %d   %9d\r\n", pll_get_pll_enable(PLL_ID_5), pll_get_pll_freq(PLL_ID_5));
	DBG_DUMP("PLL6    %d   %9d\r\n", pll_get_pll_enable(PLL_ID_6), pll_get_pll_freq(PLL_ID_6));
	DBG_DUMP("PLL7    %d   %9d\r\n", pll_get_pll_enable(PLL_ID_7), pll_get_pll_freq(PLL_ID_7));
	DBG_DUMP("PLL8    %d   %9d\r\n", pll_get_pll_enable(PLL_ID_8), pll_get_pll_freq(PLL_ID_8));
	DBG_DUMP("PLL10   %d   %9d\r\n", pll_get_pll_enable(PLL_ID_10), pll_get_pll_freq(PLL_ID_10));
	DBG_DUMP("PLL11   %d   %9d\r\n", pll_get_pll_enable(PLL_ID_11), pll_get_pll_freq(PLL_ID_11));
	DBG_DUMP("PLL13   %d   %9d\r\n", pll_get_pll_enable(PLL_ID_13), pll_get_pll_freq(PLL_ID_13));
	DBG_DUMP("PLL320  %d   %9d\r\n", pll_get_pll_enable(PLL_ID_FIXED320), pll_get_pll_freq(PLL_ID_FIXED320));

	return TRUE;
}


static BOOL pll_print_module_info_to_uart(CHAR *str_cmd)
{
	UINT32 freq1 = 0, pll_n = 1;

	DBG_DUMP("           EN        FREQ   SRC\r\n");
	pll_get_clock_freq(CPUCLK_FREQ, &freq1);
	DBG_DUMP("CPU1        1   %9d  PLL%d\r\n", freq1, (pll_get_clock_rate(PLL_CLKSEL_CPU) == PLL_CLKSEL_CPU_PLL8) ? 8 : 1);
	pll_get_clock_freq(APBCLK_FREQ, &freq1);
	DBG_DUMP("APB         1   %9d  PLL1\r\n", freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_IPE) == PLL_CLKSEL_IPE_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_IPE) == PLL_CLKSEL_IPE_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("IPE         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(IPE_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_DIS) == PLL_CLKSEL_DIS_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_DIS) == PLL_CLKSEL_DIS_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("DIS         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(DIS_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_IME) == PLL_CLKSEL_IME_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_IME) == PLL_CLKSEL_IME_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("IME         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(IME_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_ISE) == PLL_CLKSEL_ISE_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_ISE) == PLL_CLKSEL_ISE_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("ISE         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(ISE_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_DCE) == PLL_CLKSEL_DCE_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_DCE) == PLL_CLKSEL_DCE_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("DCE         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(DCE_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_IFE) == PLL_CLKSEL_IFE_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_IFE) == PLL_CLKSEL_IFE_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("IFE         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(IFE_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_IFE2) == PLL_CLKSEL_IFE2_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_IFE2) == PLL_CLKSEL_IFE2_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("IFE2        %d   %9d  PLL%d\r\n", pll_is_clock_enabled(IFE2_CLK), freq1, pll_n);
	pll_get_clock_freq(SIECLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC) == PLL_CLKSEL_SIE_CLKSRC_PLL5) {
		pll_n = 5;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC) == PLL_CLKSEL_SIE_CLKSRC_320) {
		pll_n = 320;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC) == PLL_CLKSEL_SIE_CLKSRC_PLL13) {
		pll_n = 13;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC) == PLL_CLKSEL_SIE_CLKSRC_PLL12) {
		pll_n = 12;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE_CLKSRC) == PLL_CLKSEL_SIE_CLKSRC_PLL10) {
		pll_n = 10;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SIE         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SIE_CLK), freq1, pll_n);
	pll_get_clock_freq(SIE2CLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC) == PLL_CLKSEL_SIE2_CLKSRC_PLL5) {
		pll_n = 5;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC) == PLL_CLKSEL_SIE2_CLKSRC_320) {
		pll_n = 320;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC) == PLL_CLKSEL_SIE2_CLKSRC_PLL13) {
		pll_n = 13;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC) == PLL_CLKSEL_SIE2_CLKSRC_PLL12) {
		pll_n = 12;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SIE2_CLKSRC) == PLL_CLKSEL_SIE2_CLKSRC_PLL10) {
		pll_n = 10;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SIE2        %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SIE2_CLK), freq1, pll_n);




	pll_get_clock_freq(SIEMCLK_FREQ, &freq1);
	DBG_DUMP("SIEMCLK     %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SIE_MCLK), freq1, (pll_get_clock_rate(PLL_CLKSEL_SIE_MCLKSRC) == PLL_CLKSEL_SIE_MCLKSRC_480) ? 1 : 5);
	pll_get_clock_freq(SIEMCLK2_FREQ, &freq1);
	DBG_DUMP("SIE2MCLK    %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SIE_MCLK2), freq1, (pll_get_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC) == PLL_CLKSEL_SIE_MCLK2SRC_480) ? 1 : 5);
	DBG_DUMP("TGE         %d              %s\r\n", pll_is_clock_enabled(TGE_CLK), (pll_get_clock_rate(PLL_CLKSEL_TGE) == PLL_CLKSEL_TGE_PXCLKSRC_MCLK) ? "SIEMCLK" : "PXCLKPAD");
	DBG_DUMP("TGE2        %d              %s\r\n", pll_is_clock_enabled(TGE_CLK), (pll_get_clock_rate(PLL_CLKSEL_TGE2) == PLL_CLKSEL_TGE2_PXCLKSRC_MCLK2) ? "SIEMCLK2" : "PXCLKPAD");
	if (pll_get_clock_rate(PLL_CLKSEL_JPEG) == PLL_CLKSEL_JPEG_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_JPEG) == PLL_CLKSEL_JPEG_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("JPEG        %d   %9d  PLL%d\r\n", pll_is_clock_enabled(JPG_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_H265) == PLL_CLKSEL_H265_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_H265) == PLL_CLKSEL_H265_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("H265        %d   %9d  PLL%d\r\n", pll_is_clock_enabled(H265_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC) == PLL_CLKSEL_GRAPHIC_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC) == PLL_CLKSEL_GRAPHIC_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("GRAPHIC     %d   %9d  PLL%d\r\n", pll_is_clock_enabled(GRAPH_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC2) == PLL_CLKSEL_GRAPHIC2_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC2) == PLL_CLKSEL_GRAPHIC2_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("GRAPHIC2    %d   %9d  PLL%d\r\n", pll_is_clock_enabled(GRAPH2_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_CRYPTO) == PLL_CLKSEL_CRYPTO_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_CRYPTO) == PLL_CLKSEL_CRYPTO_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("CRYPTO      %d   %9d  PLL%d\r\n", pll_is_clock_enabled(CRYPTO_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_RSA) == PLL_CLKSEL_RSA_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_RSA) == PLL_CLKSEL_RSA_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("RSA         %d   %9d  PLL%d\r\n", pll_is_clock_enabled(RSA_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_HASH) == PLL_CLKSEL_HASH_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_HASH) == PLL_CLKSEL_HASH_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("HASH        %d   %9d  PLL%d\r\n", pll_is_clock_enabled(HASH_CLK), freq1, pll_n);
	pll_get_clock_freq(SDIOCLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SDIO) == PLL_CLKSEL_SDIO_PLL4) {
		pll_n = 4;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SDIO        %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SDIO_CLK), freq1, pll_n);
	pll_get_clock_freq(SDIO2CLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SDIO2) == PLL_CLKSEL_SDIO2_PLL4) {
		pll_n = 4;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SDIO2       %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SDIO2_CLK), freq1, pll_n);
	pll_get_clock_freq(SDIO3CLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SDIO3) == PLL_CLKSEL_SDIO3_PLL4) {
		pll_n = 4;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SDIO3       %d   %9d  PLL%d\r\n", pll_is_clock_enabled(SDIO3_CLK), freq1, pll_n);
	pll_get_clock_freq(IDEOUTIFCLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC) == PLL_CLKSEL_IDE_CLKSRC_PLL4) {
		pll_n = 4;
	} else if (pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC) == PLL_CLKSEL_IDE_CLKSRC_PLL6) {
		pll_n = 6;
	} else if (pll_get_clock_rate(PLL_CLKSEL_IDE_CLKSRC) == PLL_CLKSEL_IDE_CLKSRC_PLL9) {
		pll_n = 9;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("IDEOUT      %d   %9d  PLL%d\r\n", pll_is_clock_enabled(IDE1_CLK), freq1, pll_n);
	if (pll_get_clock_rate(PLL_CLKSEL_TSE) == PLL_CLKSEL_TSE_240) {
		freq1 = 240000000;
		pll_n = 1;
	} else if (pll_get_clock_rate(PLL_CLKSEL_TSE) == PLL_CLKSEL_TSE_320) {
		freq1 = 320000000;
		pll_n = 320;
	} else {
		freq1 = pll_get_pll_freq(PLL_ID_13);
		pll_n = 13;
	}
	DBG_DUMP("TSE         %d   %9d  PLL%d\r\n", (UINT)pll_is_clock_enabled(TSE_CLK), (UINT)freq1, (UINT)pll_n);

	pll_get_clock_freq(SPCLK_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SP) == PLL_CLKSEL_SP_PLL4) {
		pll_n = 4;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SP) == PLL_CLKSEL_SP_PLL5) {
		pll_n = 5;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SP) == PLL_CLKSEL_SP_PLL6) {
		pll_n = 6;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SP_CLK      %d   %9d  PLL%d\r\n", (UINT)pll_is_clock_enabled(SP_CLK), (UINT)freq1, (UINT)pll_n);
	pll_get_clock_freq(SPCLK2_FREQ, &freq1);
	if (pll_get_clock_rate(PLL_CLKSEL_SP2) == PLL_CLKSEL_SP2_PLL4) {
		pll_n = 4;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SP2) == PLL_CLKSEL_SP2_PLL5) {
		pll_n = 5;
	} else if (pll_get_clock_rate(PLL_CLKSEL_SP2) == PLL_CLKSEL_SP2_PLL6) {
		pll_n = 6;
	} else {
		pll_n = 1;
	}
	DBG_DUMP("SP2_CLK     %d   %9d  PLL%d\r\n", (UINT)pll_is_clock_enabled(SP2_CLK), (UINT)freq1, (UINT)pll_n);
	pll_get_clock_freq(TRNGCLK_FREQ, &freq1);
	DBG_DUMP("TRNG        %d   %9d  PLL1\r\n", (UINT)pll_is_clock_enabled(TRNG_CLK), (UINT)freq1);
	pll_get_clock_freq(ADOCLK_FREQ, &freq1);
	DBG_DUMP("AUDIO       %d   %9d  PLL7\r\n", (UINT)pll_is_clock_enabled(DAI_CLK), (UINT)freq1);
	pll_get_clock_freq(SPICLK_FREQ, &freq1);
	DBG_DUMP("SPI         %d   %9d  PLL1\r\n", (UINT)pll_is_clock_enabled(SPI_CLK), (UINT)freq1);
	pll_get_clock_freq(SPI2CLK_FREQ, &freq1);
	DBG_DUMP("SPI2        %d   %9d  PLL1\r\n", (UINT)pll_is_clock_enabled(SPI2_CLK), (UINT)freq1);
	pll_get_clock_freq(SPI3CLK_FREQ, &freq1);
	DBG_DUMP("SPI3        %d   %9d  PLL1\r\n", (UINT)pll_is_clock_enabled(SPI3_CLK), (UINT)freq1);

	return TRUE;
}


/*
    Install PLL Debug command

    @return void
*/
static void pll_install_cmd(void)
{
	static BOOL installed = FALSE;

	static SXCMD_BEGIN(pll, "print system clock debug info")
	SXCMD_ITEM("pll", pll_print_pll_info_to_uart,  "print pll debug info")
	SXCMD_ITEM("module", pll_print_module_info_to_uart,  "print modules' pll usage map debug info")
	SXCMD_END()

	if (installed == FALSE) {
		SxCmd_AddTable(pll);
		installed = TRUE;
	}
}

#endif
//@}
