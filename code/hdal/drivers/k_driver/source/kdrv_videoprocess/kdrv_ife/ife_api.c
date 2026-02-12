#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
#include <malloc.h>
#else
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "mach/fmem.h"
#include "ife_drv.h"
#endif
#include <kwrap/task.h>
#include <kwrap/file.h>
#include "kwrap/type.h"
#include "ife_api.h"
#include "ife_dbg.h"
#include "ife_lib.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"

#define IFE_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))
#define TEST_FNUM 0

#define IFE_TOOL_KIT_EN     0



#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
extern int nvt_kdrv_ipp_api_ife_test(unsigned char argc, char **pargv);
extern int nvt_ife_api_dump(unsigned char argc, char **pargv);
#else
extern int nvt_kdrv_ipp_api_ife_test(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv);
extern int nvt_ife_api_dump(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv);
extern void ife_test(UINT32 uiaddr);
#endif

#if (IFE_TOOL_KIT_EN == 1)

static UINT32 kdrv_ife_fmd_flag = 0;
static UINT32 vig_ch0_lut[17] = {328, 190, 495, 256, 991, 330, 252, 371, 972, 887, 972, 823, 49, 161, 961, 82, 577};
static UINT32 vig_ch1_lut[17] = {117, 89, 490, 967, 583, 117, 118, 834, 724, 721, 246, 103, 911, 704, 715, 65, 901};
static UINT32 vig_ch2_lut[17] = {1012, 462, 366, 637, 341, 678, 562, 777, 824, 553, 872, 59, 895, 20, 263, 1002, 95};
static UINT32 vig_ch3_lut[17] = {286, 921, 459, 531, 958, 858, 295, 689, 205, 543, 143, 385, 695, 659, 302, 602, 55};

static UINT32  bright_th[OUTL_BRI_TH_NUM] = {0x160, 0x3D1, 0x5E3, 0x753, 0x775};
static UINT32  dark_th [OUTL_DARK_TH_NUM] = {0x352, 0x601, 0x851, 0xBFB, 0xF5D};



static UINT32  outl_cnt[OUTL_CNT_NUM]     = {6, 1};
static UINT8   ord_bri_w[OUTL_ORD_W_NUM]  = {6, 5, 0, 6, 5, 3, 6, 2};
static UINT8   ord_dark_w[OUTL_ORD_W_NUM] = {8, 4, 8, 2, 1, 7, 7, 8};

static UINT8   gbal_ofs[GBAL_OFS_NUM]  = {0x2, 0x3, 0x4, 0x4, 0x6, 0x6, 0x7, 0x8, 0xA, 0xB, 0xB, 0xE, 0x11, 0x13, 0x13, 0x17, 0x1B};

/*
static UINT32 wa[8] = {8, 9, 7, 2, 12, 5, 0, 13};
static UINT32 wb[4] = {14, 3, 2, 14};
static UINT32 wc[7] = {2, 11, 11, 11, 15, 4, 5};
static UINT32 radius[8] = {80, 166, 254, 181, 162, 164, 178, 178};
static UINT32 wbh[8] = {163, 109, 72, 246, 70, 189, 249, 35};
static UINT32 wbl[8] = {176, 83, 69, 93, 53, 239, 81, 213};
static UINT32 wbm[8] = {147, 63, 106, 192, 97, 197, 167, 100};
*/

static UINT32 s_weight[SPATIAL_W_LEN] = {0x16, 0x10, 0x14, 0x2, 0x16, 0x5};

static UINT32 range_a_ch0_lut[RANGE_A_LUT_SIZE] = {0x72, 0x86, 0x8E, 0x9D, 0xBA, 0xCF, 0xDF, 0xFE, 0x107, 0x120, 0x131, 0x15E, 0x161, 0x180, 0x18A, 0x1C9, 0x1E2};
static UINT32 range_a_ch1_lut[RANGE_A_LUT_SIZE] = {0x7B, 0x91, 0x9C, 0xAE, 0xC7, 0xC7, 0x106, 0x139, 0x1A1, 0x1A5, 0x1DD, 0x21B, 0x236, 0x246, 0x24F, 0x26A, 0x27B};
static UINT32 range_a_ch2_lut[RANGE_A_LUT_SIZE] = {0x56, 0x85, 0x8F, 0xB3, 0xED, 0x136, 0x15D, 0x17C, 0x185, 0x1AD, 0x1E4, 0x20D, 0x21E, 0x221, 0x253, 0x255, 0x264};
static UINT32 range_a_ch3_lut[RANGE_A_LUT_SIZE] = {0x59, 0x91, 0xB3, 0xE0, 0x113, 0x14C, 0x17C, 0x196, 0x1A2, 0x1AB, 0x1DE, 0x1F1, 0x1FF, 0x211, 0x21E, 0x22E, 0x22E};

static UINT32 range_b_ch0_lut[RANGE_B_LUT_SIZE] = {0x72, 0x86, 0x8E, 0x9D, 0xBA, 0xCF, 0xDF, 0xFE, 0x107, 0x120, 0x131, 0x15E, 0x161, 0x180, 0x18A, 0x1C9, 0x1E2};
static UINT32 range_b_ch1_lut[RANGE_B_LUT_SIZE] = {0x7B, 0x91, 0x9C, 0xAE, 0xC7, 0xC7, 0x106, 0x139, 0x1A1, 0x1A5, 0x1DD, 0x21B, 0x236, 0x246, 0x24F, 0x26A, 0x27B};
static UINT32 range_b_ch2_lut[RANGE_B_LUT_SIZE] = {0x56, 0x85, 0x8F, 0xB3, 0xED, 0x136, 0x15D, 0x17C, 0x185, 0x1AD, 0x1E4, 0x20D, 0x21E, 0x221, 0x253, 0x255, 0x264};
static UINT32 range_b_ch3_lut[RANGE_B_LUT_SIZE] = {0x59, 0x91, 0xB3, 0xE0, 0x113, 0x14C, 0x17C, 0x196, 0x1A2, 0x1AB, 0x1DE, 0x1F1, 0x1FF, 0x211, 0x21E, 0x22E, 0x22E};

static UINT32 rth_a_ch0[RANGE_A_TH_NUM] = {0xE3, 0xEE, 0xF9, 0x108, 0x10F, 0x266};
static UINT32 rth_a_ch1[RANGE_A_TH_NUM] = {0xCC, 0x179, 0x181, 0x20C, 0x271, 0x275};
static UINT32 rth_a_ch2[RANGE_A_TH_NUM] = {0xA3, 0xA7, 0xC4, 0xC6, 0xD2, 0x168};
static UINT32 rth_a_ch3[RANGE_A_TH_NUM] = {0x1, 0xE, 0x1CE, 0x208, 0x2B0, 0x376};

static UINT32 rth_b_ch0[RANGE_B_TH_NUM] = {0xE3, 0xEE, 0xF9, 0x108, 0x10F, 0x266};
static UINT32 rth_b_ch1[RANGE_B_TH_NUM] = {0xCC, 0x179, 0x181, 0x20C, 0x271, 0x275};
static UINT32 rth_b_ch2[RANGE_B_TH_NUM] = {0xA3, 0xA7, 0xC4, 0xC6, 0xD2, 0x168};
static UINT32 rth_b_ch3[RANGE_B_TH_NUM] = {0x1, 0xE, 0x1CE, 0x208, 0x2B0, 0x376};

static UINT32 rbfill_luma[RBFILL_LUMA_NUM]   = {0xC, 0x1C, 0x14, 0x1F, 0x10, 0x7, 0x14, 0x19, 0x11, 0x5, 0x5, 0xA, 0x4, 0x1C, 0x1A, 0x4, 0xA};

static UINT32 rbfill_ratio[RBFILL_RATIO_NUM] = {0x17, 0x2, 0xE, 0xA, 0x1C, 0x1D, 0x1D, 0x0, 0x8, 0x1C, 0x18, 0x1C, 0x13, 0x1F, 0x12, 0x17, 0x6,
												0x1, 0x18, 0xF, 0x1B, 0x1E, 0x16, 0x14, 0x13, 0x1E, 0xD, 0x13, 0x1, 0x0, 0x12, 0x16
											   };

static UINT32 IFE_F_NRS_ORD_DARK_WLUT[8] = {3, 5, 4, 8, 1, 6, 1, 1};
static UINT32 IFE_F_NRS_ORD_BRI_WLUT[8]  = {8, 0, 3, 3, 0, 3, 3, 5};
static UINT32 NRS_B_OFS[6] = {72, 93, 140, 169, 206, 255}; //48 5D  8C  A9  CE  FF
static UINT32 NRS_B_W[6]   = {26, 23, 19, 17, 15, 15};     //1A 17  13  11  F   F
static UINT32 NRS_B_RNG[5] = {16, 144, 160, 224, 288};     //10 90  A0  E0  120 0
static UINT32 NRS_B_TH[5]  = {0, 3, 0, 2, 2};

static UINT32 NRS_GB_OFS[6] = {3, 8, 22, 27, 54, 60};      //3  8   16  1B  36  3C
static UINT32 NRS_GB_W[6]   = {31, 30, 30, 26, 23, 23};    //1F 1E  1E  1A  17  17
static UINT32 NRS_GB_RNG[5] = {16, 48, 112, 144, 208};     //10 30  70  90  D0  0
static UINT32 NRS_GB_TH[5]  = {0, 1, 2, 1, 2};

static UINT32 FCGAIN_1[5]     = {348, 571, 702, 274, 496};   //15C  23B 2BE 112 1F0
static UINT32 FCGAIN_2[5]     = {557, 439, 761, 688, 503};   //22D  1B7 2F9 2B0 1F7
static UINT32 FCGAIN_OFS_1[5] = {24, 4, 10, 4, 29};          //18   4   A   4   1D
static UINT32 FCGAIN_OFS_2[5] = {4, 29, 20, 3, 3};           //4    1D  14  3   3



static UINT32 FC_Y_W[FCURVE_Y_W_NUM]   = {0x11, 0x6B, 0x14, 0x9F, 0x3C, 0x3B, 0x7E, 0x84, 0xF6, 0xAD, 0xD4, 0xE1, 0x6C,
										  0xFE, 0xF7, 0x6E, 0x99
										 };
static UINT32 FC_IDX[FCURVE_IDX_NUM]   = {0x1, 0x2, 0x3, 0x4, 0x6, 0x7, 0x9, 0xA, 0xC, 0xD, 0xF, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17, 0x18,
										  0x1A, 0x1B, 0x1C, 0x1E, 0x1F, 0x20, 0x22, 0x23, 0x24, 0x28, 0x29, 0x2B, 0x2F, 0x31
										 };
static UINT32 FC_SPLIT[FCURVE_SPLIT_NUM] = {1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, 1, 2, 1};

static UINT32 FC_VAL[FCURVE_VAL_NUM]   = {0x3, 0x46, 0x4B, 0x78, 0x7A, 0xB0, 0xC8, 0xF1, 0x128, 0x14C, 0x169, 0x170, 0x1A3, 0x1C9, 0x1CD, 0x1D6,
										  0x1FD, 0x203, 0x209, 0x221, 0x242, 0x253, 0x260, 0x282, 0x2B2, 0x2EA, 0x317, 0x357, 0x39A, 0x3CA, 0x3F8, 0x417,
										  0x457, 0x45F, 0x466, 0x496, 0x4A8, 0x4C4, 0x4F7, 0x504, 0x531, 0x54F, 0x57D, 0x5B6, 0x5D8, 0x616, 0x646, 0x673,
										  0x69B, 0x6C8, 0x6E9, 0x71D, 0x746, 0x780, 0x781, 0x7AD, 0x7E1, 0x7FF, 0x81C, 0x840, 0x877, 0x8B8, 0x8C6, 0x8D2,
										  0x911
										 };


static UINT32 SCOMP_SUB[4]   = {1629, 1608, 832, 568};
static UINT32 SCOMP_SHIFT[4] = {3, 0, 3, 3};

static UINT32 MC_NEG_DIFF_W[16] = {8, 7, 8, 10, 16, 1, 6, 3, 3, 10, 16, 13, 9, 5, 11, 9};
static UINT32 MC_POS_DIFF_W[16] = {1, 0, 0, 7, 7, 9, 15, 9, 2, 1, 2, 12, 15, 8, 11, 5};

static KDRV_IFE_PARAM_IPL_FUNC_EN g_kdrv_ife_ipl_ctrl;
static KDRV_IFE_NRS_PARAM     g_kdrv_ife_nrs;
static KDRV_IFE_FUSION_INFO   g_kdrv_ife_fusion_info;
static KDRV_IFE_FUSION_PARAM  g_kdrv_ife_fusion;
static KDRV_IFE_FCURVE_PARAM  g_kdrv_ife_fcurve;

static KDRV_IFE_OUTL_PARAM   g_kdrv_ife_outl;
static KDRV_IFE_FILTER_PARAM g_kdrv_ife_2DNR;
static KDRV_IFE_CGAIN_PARAM  g_kdrv_ife_cgain;
static KDRV_IFE_VIG_PARAM    g_kdrv_ife_vig;
static KDRV_IFE_VIG_CENTER   g_kdrv_ife_vig_center;
static KDRV_IFE_GBAL_PARAM   g_kdrv_ife_gbal;
#endif


#if (defined __UITRON || defined __ECOS || defined __FREERTOS)


#if (IFE_TOOL_KIT_EN == 1)
static UINT32 i;


static void set_nrs_para(BOOL func_enable);
static void set_fusion_para(BOOL func_enable, UINT32 frame_num);
static void set_fcurve_para(BOOL func_enable);
static void set_outl_para(BOOL func_enable);
static void set_filter_para(BOOL func_enable);
static void set_cgain_para(BOOL func_enable);
static void set_vig_para(BOOL func_enable);
static void set_gbal_para(BOOL func_enable);
#endif

#endif
//static UINT32 uiTmpPhyBufAddr = 0x08000000, uiTmpBufSz = 16000000;

INT32 KDRV_IFE_ISR_CB(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
#if (IFE_TOOL_KIT_EN == 1)

	if (sts & KDRV_IFE_INT_FMD) {
		kdrv_ife_fmd_flag = 1;
	}
#endif
	return 0;
}

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
int nvt_ife_api_dump(unsigned char argc, char **pargv)
{
	kdrv_ife_dump_info(printf);
	return 0;
}
#else
int nvt_ife_api_dump(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	kdrv_ife_dump_info(printk);
	return 0;
}
#endif

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)

#if (IFE_TOOL_KIT_EN == 1)

static INT32 kdrv_ife_free_buff(UINT32 *buf_addr)
{
	DBG_IND("free buf!\r\n");
	if (buf_addr != NULL) {
		free(buf_addr);
		//buf_addr = 0;
	} else {
		//buf_addr = 0;
		DBG_WRN("buffer is not allocated, nothing happend!\r\n");
	}
	return E_OK;
}
#endif

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_kdrv_ipp_api_ife_test(unsigned char argc, char **pargv)
{
#if (IFE_TOOL_KIT_EN == 1)

	UINT32 *in1_buf_info = NULL, *out_buf_info = NULL;

#if TEST_FNUM
	UINT32 *in2_buf_info = NULL;
#endif
	VOS_FILE fp;
	int len;
	UINT32 in_w, in_h, in_bit, out_bit;//, hn, hm, hl;
	UINT16 crop_w, crop_h, crop_hpos, crop_vpos;
	//UINT8  h_shift;
	KDRV_IFE_OPENCFG ife_opencfg;
	KDRV_IFE_IN_IMG_INFO in_img_info = {0};
	KDRV_IFE_IN_OFS      in_img_lofs = {0};
	KDRV_IFE_IN_ADDR     in_img_addr = {0};
	KDRV_IFE_MIRROR      mirror = {0};
	KDRV_IFE_RDE_INFO    rde_info = {0};
	KDRV_IFE_FUSION_INFO fusion_info = {0};
	KDRV_IFE_RING_BUF_INFO  ring_buf_info = {0};

	KDRV_IFE_OUT_IMG_INFO out_img_info = {0};
	KDRV_IFE_OUT_ADDR     out_addr = {0};
	KDRV_IFE_INTE inte_en;

	char fire_name[256];

	int in_size, out_size;

	UINT8 FNUM;

	UINT32 id = 0, chip = 0, engine = 0x5001, channel = 0;

	in_w = 112;//1920;//simple_strtoul(pargv[1], NULL, 0);
	in_h = 100;//1080;//simple_strtoul(pargv[2], NULL, 0);
	in_bit = 16;//simple_strtoul(pargv[3], NULL, 0);
	out_bit = 12;//simple_strtoul(pargv[5], NULL, 0);

	crop_w  = 112;
	crop_h  = 100;
	crop_hpos = 0;
	crop_vpos = 0;
	/*    h_shift = 0;

	    hn = 0;
	    hl = 112;
	    hm = 0;*/
	FNUM = TEST_FNUM;

	sprintf(fire_name, "/mnt/sd/In.raw");
	DBG_IND("input data %s %d %d %d %d\n", fire_name, (unsigned int)in_w, (unsigned int)in_h, (unsigned int)in_bit, (unsigned int)out_bit);

	/*get in buffer*/


	in_size = IFE_ALIGN(((in_w * in_h * in_bit) >> 3), 4);
	in1_buf_info = (UINT32 *)malloc(in_size);
#if TEST_FNUM
	if (FNUM == 0) {
		in_size = IFE_ALIGN(((in_w * in_h * in_bit) >> 3), 4);
		in1_buf_info = (UINT32 *)malloc(in_size);
	} else {
		in_size = IFE_ALIGN(((in_w * in_h * in_bit) >> 3), 4);
		in2_buf_info = (UINT32 *)malloc(in_size);
		in1_buf_info = (UINT32 *)malloc(in_size);
	}
#endif
	/*get out buffer*/
	out_size = IFE_ALIGN(((crop_w * crop_h * out_bit) >> 3), 4);
	out_buf_info = (UINT32 *)malloc(out_size);


//---------IPL enable -------------//
	g_kdrv_ife_ipl_ctrl.enable = TRUE;
	g_kdrv_ife_ipl_ctrl.ipl_ctrl_func = KDRV_IFE_IQ_FUNC_ALL;

	set_nrs_para(FALSE);
	set_fusion_para(FALSE, FNUM);
	set_fcurve_para(FALSE);
	set_outl_para(FALSE);
	set_filter_para(FALSE);
	set_cgain_para(FALSE);
	set_vig_para(FALSE);
	set_gbal_para(FALSE);

//----------------------------------//
	/* load image */
	fp = vos_file_open(fire_name, O_RDONLY, 0);
	DBG_IND("file:%s, fp:%d\r\n", fire_name, fp);
	if (fp == -1) {
		DBG_ERR("failed in file open:%s\r\n", pargv[0]);

		//kdrv_ife_free_buff(in1_buf_info);
		kdrv_ife_free_buff(out_buf_info);
		kdrv_ife_free_buff(in1_buf_info);
#if TEST_FNUM
		if (FNUM == 0) {
			kdrv_ife_free_buff(in1_buf_info);
		} else {
			kdrv_ife_free_buff(in1_buf_info);
			kdrv_ife_free_buff(in2_buf_info);
		}
#endif
		return fp;
	}

	len = vos_file_read(fp, (in1_buf_info), in_size);
	DBG_IND("file length:%d\r\n", len);
	vos_file_close(fp);
	/*
	    ife_platform_create_resource();
	    ife_platform_set_clk_rate(360);

	    kdrv_ife_install_id();
	    kdrv_ife_flow_init();
	    */

	kdrv_ife_init();
	//kdrv_ife_buf_init(0, 0);
	/******** ife d2d flow *********/
	DBG_IND("ife d2d flow start\n");
	id = KDRV_DEV_ID(chip, engine, channel);
	/*open*/
	ife_opencfg.ife_clock_sel = 240;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OPENCFG, (void *)&ife_opencfg);
	kdrv_ife_open(chip, engine);

	/*set input img parameters*/
	in_img_info.in_src = KDRV_IFE_IN_MODE_D2D;
	in_img_info.type = KDRV_IFE_IN_RGBIR;
	in_img_info.img_info.width = in_w;
	in_img_info.img_info.height = in_h;
	/*in_img_info.img_info.stripe_hn = hn;
	in_img_info.img_info.stripe_hl = hl;
	in_img_info.img_info.stripe_hm = hm;*/
	in_img_info.img_info.st_pix = KDRV_IPP_RGGB_PIX_GR;
	in_img_info.img_info.bit = in_bit;
	//in_img_info.img_info.cfa_pat = cfa_pat;

	in_img_info.img_info.crop_width  = crop_w;
	in_img_info.img_info.crop_height = crop_h;
	in_img_info.img_info.crop_hpos   = crop_hpos;
	in_img_info.img_info.crop_vpos   = crop_vpos;
	//in_img_info.img_info.h_shift     = h_shift;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_IMG, (void *)&in_img_info);

	/*set input img line offset*/
	in_img_lofs.line_ofs_1 = IFE_ALIGN((in_w * in_bit) >> 3, 4);
	in_img_lofs.line_ofs_2 = 0;
#if TEST_FNUM
	if (FNUM == 1) {
		in_img_lofs.line_ofs_2 = IFE_ALIGN((in_w * in_bit) >> 3, 4);
	} else {
		in_img_lofs.line_ofs_2 = 0;
	}
#endif
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_OFS_1, (void *)&in_img_lofs);
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_OFS_2, (void *)&in_img_lofs);

	/*set input1 address*/
	in_img_addr.ife_in_addr_1 = (UINT32)in1_buf_info;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_ADDR_1, (void *)&in_img_addr);
	DBG_IND("^Ykdrv_ife_Set In Addr1: 0x%08x !!\r\n", (unsigned int)in_img_addr.ife_in_addr_1);
#if TEST_FNUM
	if (FNUM == 1) {
		/*set input2 address*/
		in_img_addr.ife_in_addr_2 = (UINT32)in2_buf_info;
		kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_ADDR_2, (void *)&in_img_addr);
		DBG_IND("^Ykdrv_ife_Set In Addr2: 0x%08x !!\r\n", (unsigned int)in_img_addr.ife_in_addr_2);
	}
#endif
	/*set output img parameters*/
	out_img_info.line_ofs = IFE_ALIGN((in_w * out_bit) >> 3, 4);
	out_img_info.bit = out_bit;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);

	/*set output address*/
	out_addr.ife_out_addr = (UINT32)out_buf_info;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OUT_ADDR, (void *)&out_addr);

	//---------Ring buffer -----------//
	ring_buf_info.dmaloop_en   = 0;
	ring_buf_info.dmaloop_line = 10;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_RINGBUF_INFO, (void *)&ring_buf_info);

	//----------Mirror set ------------//
	mirror.enable = FALSE;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_MIRROR, (void *)&mirror);

	//--------- RDE info set -------------//
	rde_info.enable = 0;
	rde_info.encode_rate = 0;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_RDE_INFO, (void *)&rde_info);

	//--------- Fusion info set -------------//
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_FUSION_INFO, (void *)&fusion_info);

	kdrv_ife_fmd_flag = 0;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ISR_CB, (void *)&KDRV_IFE_ISR_CB);

	/*set interrupt enable*/
	inte_en = KDRV_IFE_INTE_ALL;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_INTER, (void *)&inte_en);
	/* set nrs para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_NRS, (void *)&g_kdrv_ife_nrs);
	/* set fusion para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FUSION, (void *)&g_kdrv_ife_fusion);
	/* set fcurve para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FCURVE, (void *)&g_kdrv_ife_fcurve);
	/* set outlier para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_OUTL, (void *)&g_kdrv_ife_outl);
	/* set 2dnr para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FILTER, (void *)&g_kdrv_ife_2DNR);
	/* set color gain para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_CGAIN, (void *)&g_kdrv_ife_cgain);
	/* set Vig para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_VIG, (void *)&g_kdrv_ife_vig);
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_VIG_CENTER, (void *)&g_kdrv_ife_vig_center);
	/* set gbal para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_GBAL, (void *)&g_kdrv_ife_gbal);

	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ALL_FUNC_EN, (void *)&g_kdrv_ife_ipl_ctrl);
	DBG_IND("kdrv_ife_set done\r\n");

	/*trig*/
	kdrv_ife_trigger(id, NULL, NULL, NULL);

	/*wait end*/
	while (!kdrv_ife_fmd_flag) {
		vos_task_delay_ms(10);
	}

	ife_pause();

	/*close*/
	kdrv_ife_close(chip, engine);

	DBG_IND("ife d2d flow end\r\n");

	/******** ife d2d flow *********/

	/* save image */
	fp = vos_file_open("/mnt/sd/output.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == -1) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/output.raw");

		kdrv_ife_free_buff(in1_buf_info);
		kdrv_ife_free_buff(out_buf_info);
#if TEST_FNUM
		if (FNUM == 1) {
			kdrv_ife_free_buff(in2_buf_info);
		}
#endif
		return -14;//EFAULT;
	}
	len = vos_file_write(fp, out_buf_info, out_size);
	DBG_IND("Write length %d!\r\n", len);
	len = vos_file_close(fp);

	free(out_buf_info);
	free(in1_buf_info);

	//out_buf_info = NULL;
	//in1_buf_info = NULL;
	kdrv_ife_buf_uninit();

	//kdrv_ife_free_buff(in1_buf_info);
	//kdrv_ife_free_buff(out_buf_info);
#if TEST_FNUM
	if (FNUM == 1) {
		kdrv_ife_free_buff(in2_buf_info);
	}
#endif
	kdrv_ife_uninit();
	DBG_IND("Dump output done!\n");
	return len;
#else
	return 0;
#endif
}
#endif

#else
#if !defined (CONFIG_NVT_SMALL_HDAL)
int nvt_kdrv_ipp_api_ife_test(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if (IFE_TOOL_KIT_EN == 1)

	//frammap_buf_t in_buf_info = {0};
	//frammap_buf_t out_buf_info = {0};

	struct nvt_fmem_mem_info_t in1_buf_info = {0};
#if TEST_FNUM
	struct nvt_fmem_mem_info_t in2_buf_info = {0};
#endif
	struct nvt_fmem_mem_info_t out_buf_info = {0};
	//mm_segment_t old_fs;
	//struct file *fp;
	VOS_FILE fp;
	int len;
	UINT32 in_w, in_h, in_bit, out_bit;//, hn, hm, hl;
	UINT16 crop_w, crop_h, crop_hpos, crop_vpos;
	//UINT8  h_shift;
	UINT32 handle;
	KDRV_IFE_OPENCFG ife_opencfg;
	KDRV_IFE_IN_IMG_INFO in_img_info = {0};
	KDRV_IFE_IN_OFS      in_img_lofs = {0};
	KDRV_IFE_IN_ADDR     in_img_addr = {0};
	KDRV_IFE_MIRROR      mirror = {0};
	KDRV_IFE_RDE_INFO    rde_info = {0};
	KDRV_IFE_FUSION_INFO fusion_info = {0};
	KDRV_IFE_RING_BUF_INFO   ring_buf_info = {0};
	//KDRV_IFE_IN_ADDR     in_img_addr = {0};
	//UINT32 in_addr;
	KDRV_IFE_OUT_IMG_INFO out_img_info = {0};
	UINT32 out_addr;
	KDRV_IFE_INTE inte_en;
	int ret;
	int size;
	void *In_handle = NULL, *Out_handle = NULL;
	UINT8 FNUM;
	UINT32 i;
	//int i;

	/*
	    KDRV_IFE_OUTL_PARAM   g_kdrv_ife_get_outl;
	    KDRV_IFE_FILTER_PARAM g_kdrv_ife_get_2DNR;
	    KDRV_IFE_CGAIN_PARAM  g_kdrv_ife_get_cgain;
	    KDRV_IFE_VIG_PARAM    g_kdrv_ife_get_vig;
	    KDRV_IFE_VIG_CENTER   g_kdrv_ife_get_vig_center;
	    KDRV_IFE_GBAL_PARAM   g_kdrv_ife_get_gbal;
	*/
	UINT32 id = 0, chip = 0, engine = 0x5001, channel = 0;
	DBG_ERR("argc = %d, pargv[0] = %s, pargv[1] = %s, pargv[2] = %s, pargv[3] = %s\r\n", argc, pargv[0], pargv[1], pargv[2], pargv[3]);

	/*in_path in_w in_h in_bit out_path out_bit*/
	//if (argc != 6) {
	//  DBG_ERR("wrong argument:%d\r\n", argc);
	//  return -EINVAL;
	//}

	in_w = 112;//1920;//simple_strtoul(pargv[1], NULL, 0);
	in_h = 100;//1080;//simple_strtoul(pargv[2], NULL, 0);
	in_bit = 16;//simple_strtoul(pargv[3], NULL, 0);
	out_bit = 12;//simple_strtoul(pargv[5], NULL, 0);

	crop_w  = 112;
	crop_h  = 100;
	crop_hpos = 1;
	crop_vpos = 2;
	/*  h_shift = 0;

	  hn = 0;
	  hl = 28;
	  hm = 0;*/
	FNUM = TEST_FNUM;

	//DBG_IND("input data %s %d %d %d %s %d\n", pargv[0], in_w, in_h, in_bit, pargv[4], out_bit);
	DBG_IND("input data %s %d %d %d %d\n", pargv[0], in_w, in_h, in_bit, out_bit);


	/*get in buffer*/
#if 0 // 680 linux
	in_buf_info.size = IFE_ALIGN(((in_w * in_h * in_bit) >> 3), 4);
	in_buf_info.align = 64;      ///< address alignment
	in_buf_info.name = "ife_t_in";
	in_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &in_buf_info)) {
		DBG_ERR("get input buffer fail\n");
		return -EFAULT;
	}
#else
	size = IFE_ALIGN(((in_w * in_h * in_bit) >> 3), 4);
	ret = nvt_fmem_mem_info_init(&in1_buf_info, NVT_FMEM_ALLOC_CACHE, size, NULL);
	if (ret >= 0) {
		In_handle = fmem_alloc_from_cma(&in1_buf_info, 0);
	}
#endif

#if TEST_FNUM
	if (FNUM == 1) {
		size = IFE_ALIGN(((in_w * in_h * in_bit) >> 3), 4);
		ret = nvt_fmem_mem_info_init(&in2_buf_info, NVT_FMEM_ALLOC_CACHE, size, NULL);
		if (ret >= 0) {
			In_handle = fmem_alloc_from_cma(&in2_buf_info, 0);
		}
	}
#endif
	/*get out buffer*/
#if 0 // 680 linux
	out_buf_info.size = IFE_ALIGN(((in_w * in_h * out_bit) >> 3), 4);
	out_buf_info.align = 64;      ///< address alignment
	out_buf_info.name = "ife_t_out";
	out_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &out_buf_info)) {

		frm_free_buf_ddr(out_buf_info.va_addr);
		DBG_ERR("get output buffer fail\n");
		return -EFAULT;
	}
#else
	size = IFE_ALIGN(((in_w * in_h * out_bit) >> 3), 4);
	ret = nvt_fmem_mem_info_init(&out_buf_info, NVT_FMEM_ALLOC_CACHE, size, NULL);
	if (ret >= 0) {
		Out_handle = fmem_alloc_from_cma(&out_buf_info, 0);
	}
#endif

//---------IPL enable -------------//
	g_kdrv_ife_ipl_ctrl.enable = TRUE;
	g_kdrv_ife_ipl_ctrl.ipl_ctrl_func = KDRV_IFE_IQ_FUNC_ALL;
//---------NRS--------------------//
	g_kdrv_ife_nrs.enable = 1;

	g_kdrv_ife_nrs.order.enable     = 1;
	g_kdrv_ife_nrs.order.rng_bri    = 7;
	g_kdrv_ife_nrs.order.rng_dark   = 1;
	g_kdrv_ife_nrs.order.diff_th    = 591;
	for (i = 0; i < 8; i++) {
		g_kdrv_ife_nrs.order.dark_w[i]  = IFE_F_NRS_ORD_DARK_WLUT[i];
		g_kdrv_ife_nrs.order.bri_w[i]  = IFE_F_NRS_ORD_BRI_WLUT[i];
	}

	g_kdrv_ife_nrs.bilat.b_enable   = 1;
	g_kdrv_ife_nrs.bilat.b_str      = 4;
	for (i = 0; i < 6; i++) {
		g_kdrv_ife_nrs.bilat.b_ofs[i]      = NRS_B_OFS[i];
		g_kdrv_ife_nrs.bilat.b_weight[i]   = NRS_B_W[i];
	}
	for (i = 0; i < 5; i++) {
		g_kdrv_ife_nrs.bilat.b_rng[i]      = NRS_B_RNG[i];
		g_kdrv_ife_nrs.bilat.b_th[i]       = NRS_B_TH[i];
	}

	g_kdrv_ife_nrs.bilat.gb_enable  = 1;
	g_kdrv_ife_nrs.bilat.gb_str     = 4;
	g_kdrv_ife_nrs.bilat.gb_blend_w = 9;
	for (i = 0; i < 6; i++) {
		g_kdrv_ife_nrs.bilat.gb_ofs[i]     = NRS_GB_OFS[i];
		g_kdrv_ife_nrs.bilat.gb_weight[i]  = NRS_GB_W[i];
	}
	for (i = 0; i < 5; i++) {
		g_kdrv_ife_nrs.bilat.gb_rng[i]     = NRS_GB_RNG[i];
		g_kdrv_ife_nrs.bilat.gb_th[i]      = NRS_GB_TH[i];
	}

//---------Fusion--------------------//
	g_kdrv_ife_fusion_info.enable = 0;

	g_kdrv_ife_fusion_info.fnum = FNUM;

	g_kdrv_ife_fusion.fu_ctrl.mode = 1;
	g_kdrv_ife_fusion.fu_ctrl.y_mean_sel = 2;
	g_kdrv_ife_fusion.fu_ctrl.ev_ratio = 231;

	g_kdrv_ife_fusion.bld_cur.nor_sel = 1;
	g_kdrv_ife_fusion.bld_cur.dif_sel = 2;

	g_kdrv_ife_fusion.bld_cur.l_dif_knee[0] = 953;
	//g_kdrv_ife_fusion.bld_cur.l_dif_knee[1] = 2636;
	g_kdrv_ife_fusion.bld_cur.l_dif_range   = 12;
	g_kdrv_ife_fusion.bld_cur.l_dif_w_edge  = 1;
	g_kdrv_ife_fusion.bld_cur.l_nor_knee[0] = 1783;
	//g_kdrv_ife_fusion.bld_cur.l_nor_knee[1] = 3345;
	g_kdrv_ife_fusion.bld_cur.l_nor_range   = 3;
	g_kdrv_ife_fusion.bld_cur.l_nor_w_edge  = 1;

	g_kdrv_ife_fusion.bld_cur.s_dif_knee[0] = 220;
	// g_kdrv_ife_fusion.bld_cur.s_dif_knee[1] = 1577;
	g_kdrv_ife_fusion.bld_cur.s_dif_range   = 1;
	g_kdrv_ife_fusion.bld_cur.s_dif_w_edge  = 1;
	g_kdrv_ife_fusion.bld_cur.s_nor_knee[0] = 1450;
	//g_kdrv_ife_fusion.bld_cur.s_nor_knee[1] = 2299;
	g_kdrv_ife_fusion.bld_cur.s_nor_range   = 7;
	g_kdrv_ife_fusion.bld_cur.s_nor_w_edge  = 1;

	g_kdrv_ife_fusion.dk_sat.th[0]         = 2749;
	g_kdrv_ife_fusion.dk_sat.th[1]         = 3013;
	g_kdrv_ife_fusion.dk_sat.step[0]       = 215;
	g_kdrv_ife_fusion.dk_sat.step[1]       = 12;
	g_kdrv_ife_fusion.dk_sat.low_bound[0]  = 230;
	g_kdrv_ife_fusion.dk_sat.low_bound[1]  = 74;

	g_kdrv_ife_fusion.mc_para.diff_ratio   = 1;
	g_kdrv_ife_fusion.mc_para.lum_th       = 3903;
	g_kdrv_ife_fusion.mc_para.dwd          = 9;

	for (i = 0; i < 16; i++) {
		g_kdrv_ife_fusion.mc_para.neg_diff_w[i]   = MC_NEG_DIFF_W[i];
		g_kdrv_ife_fusion.mc_para.pos_diff_w[i]   = MC_POS_DIFF_W[i];
	}

	g_kdrv_ife_fusion.s_comp.enable      = 1;
	g_kdrv_ife_fusion.s_comp.knee[0]     = 103;
	g_kdrv_ife_fusion.s_comp.knee[1]     = 651;
	g_kdrv_ife_fusion.s_comp.knee[2]     = 1086;
	for (i = 0; i < 4; i++) {
		g_kdrv_ife_fusion.s_comp.shift[i]       = SCOMP_SHIFT[i];
		g_kdrv_ife_fusion.s_comp.sub_point[i]   = SCOMP_SUB[i];
	}

	g_kdrv_ife_fusion.fu_cgain.enable = 1;
	g_kdrv_ife_fusion.fu_cgain.bit_field = 1;

	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_r   = FCGAIN_1[0];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_gr  = FCGAIN_1[1];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_gb  = FCGAIN_1[2];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_b   = FCGAIN_1[3];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_ir  = FCGAIN_1[4];

	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_r   = FCGAIN_2[0];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_gr  = FCGAIN_2[1];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_gb  = FCGAIN_2[2];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_b   = FCGAIN_2[3];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_ir  = FCGAIN_2[4];

	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_r  = FCGAIN_OFS_1[0];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_gr = FCGAIN_OFS_1[1];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_gb = FCGAIN_OFS_1[2];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_b  = FCGAIN_OFS_1[3];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_ir = FCGAIN_OFS_1[4];

	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_r  = FCGAIN_OFS_2[0];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_gr = FCGAIN_OFS_2[1];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_gb = FCGAIN_OFS_2[2];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_b  = FCGAIN_OFS_2[3];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_ir = FCGAIN_OFS_2[4];

//---------Fcurve--------------------//
	g_kdrv_ife_fcurve.enable = 1;

	g_kdrv_ife_fcurve.fcur_ctrl.yv_w = 1;
	g_kdrv_ife_fcurve.fcur_ctrl.y_mean_sel = 2;
	for (i = 0; i < FCURVE_Y_W_NUM; i++) {
		g_kdrv_ife_fcurve.y_weight.y_w_lut[i] = FC_Y_W[i];
	}
	for (i = 0; i < FCURVE_IDX_NUM; i++) {
		g_kdrv_ife_fcurve.index.idx_lut[i]    = FC_IDX[i];
	}
	for (i = 0; i < FCURVE_SPLIT_NUM; i++) {
		g_kdrv_ife_fcurve.split.split_lut[i]    = FC_SPLIT[i];
	}
	for (i = 0; i < FCURVE_VAL_NUM; i++) {
		g_kdrv_ife_fcurve.value.val_lut[i]    = FC_VAL[i];
	}

//---------Outlier--------------------//
	g_kdrv_ife_outl.enable = 1;

	g_kdrv_ife_outl.bright_ofs     = 0xFF;
	g_kdrv_ife_outl.dark_ofs       = 0x2B;
	for (i = 0; i < OUTL_BRI_TH_NUM; i++) {
		g_kdrv_ife_outl.bright_th[i]   = bright_th[i];
		g_kdrv_ife_outl.dark_th[i]     = dark_th[i];
	}
	g_kdrv_ife_outl.outl_cnt[0]    = outl_cnt[0];
	g_kdrv_ife_outl.outl_cnt[1]    = outl_cnt[1];
	g_kdrv_ife_outl.avg_mode       = 1;

	g_kdrv_ife_outl.outl_weight    = 0x90;

	g_kdrv_ife_outl.ord_rng_bri    = 6;
	g_kdrv_ife_outl.ord_rng_dark   = 7;
	g_kdrv_ife_outl.ord_protect_th = 0x123;
#if NAME_ERR
	g_kdrv_ife_outl.ord_blend_w   = 0x57;
#else
	g_kdrv_ife_outl.ord_bleand_w   = 0x57;
#endif
	for (i = 0; i < OUTL_ORD_W_NUM; i++) {
		g_kdrv_ife_outl.ord_bri_w[i]   = ord_bri_w[i];
		g_kdrv_ife_outl.ord_dark_w[i]  = ord_dark_w[i];
	}

//------------2DNR------------------//

	g_kdrv_ife_2DNR.enable = 1;

	for (i = 0; i < SPATIAL_W_LEN; i++) {
		g_kdrv_ife_2DNR.spatial.weight[i] = s_weight[i];
	}

	for (i = 0; i < RANGE_A_LUT_SIZE; i++) {
		g_kdrv_ife_2DNR.rng_filt_r.a_lut[i] = range_a_ch0_lut[i];
		g_kdrv_ife_2DNR.rng_filt_r.b_lut[i] = range_b_ch0_lut[i];

		g_kdrv_ife_2DNR.rng_filt_gr.a_lut[i] = range_a_ch1_lut[i];
		g_kdrv_ife_2DNR.rng_filt_gr.b_lut[i] = range_b_ch1_lut[i];

		g_kdrv_ife_2DNR.rng_filt_gb.a_lut[i] = range_a_ch2_lut[i];
		g_kdrv_ife_2DNR.rng_filt_gb.b_lut[i] = range_b_ch2_lut[i];

		g_kdrv_ife_2DNR.rng_filt_b.a_lut[i] = range_a_ch3_lut[i];
		g_kdrv_ife_2DNR.rng_filt_b.b_lut[i] = range_b_ch3_lut[i];

		g_kdrv_ife_2DNR.rng_filt_ir.a_lut[i] = range_a_ch3_lut[i];
		g_kdrv_ife_2DNR.rng_filt_ir.b_lut[i] = range_b_ch3_lut[i];
	}
	for (i = 0; i < RANGE_A_TH_NUM; i++) {
		g_kdrv_ife_2DNR.rng_filt_r.a_th[i] = rth_a_ch0[i];
		g_kdrv_ife_2DNR.rng_filt_r.b_th[i] = rth_b_ch0[i];

		g_kdrv_ife_2DNR.rng_filt_gr.a_th[i] = rth_a_ch1[i];
		g_kdrv_ife_2DNR.rng_filt_gr.b_th[i] = rth_b_ch1[i];

		g_kdrv_ife_2DNR.rng_filt_gb.a_th[i] = rth_a_ch2[i];
		g_kdrv_ife_2DNR.rng_filt_gb.b_th[i] = rth_b_ch2[i];

		g_kdrv_ife_2DNR.rng_filt_b.a_th[i] = rth_a_ch3[i];
		g_kdrv_ife_2DNR.rng_filt_b.b_th[i] = rth_b_ch3[i];

		g_kdrv_ife_2DNR.rng_filt_ir.a_th[i] = rth_a_ch3[i];
		g_kdrv_ife_2DNR.rng_filt_ir.b_th[i] = rth_b_ch3[i];
	}

	g_kdrv_ife_2DNR.rbfill.enable       = 1;
	for (i = 0; i < RBFILL_LUMA_NUM; i++) {
		g_kdrv_ife_2DNR.rbfill.luma[i]  = rbfill_luma[i];
	}
	for (i = 0; i < RBFILL_RATIO_NUM; i++) {
		g_kdrv_ife_2DNR.rbfill.ratio[i] = rbfill_ratio[i];
	}
	g_kdrv_ife_2DNR.rbfill.ratio_mode  = 1;

	g_kdrv_ife_2DNR.center_mod.enable  = 1;   //0x44
	g_kdrv_ife_2DNR.center_mod.th1     = 65;  //0x44
	g_kdrv_ife_2DNR.center_mod.th2     = 745; //0x44

	g_kdrv_ife_2DNR.rng_th_w   = 5; //<- RTH_W
	g_kdrv_ife_2DNR.blend_w    = 7;
	g_kdrv_ife_2DNR.bin        = 1;

	g_kdrv_ife_2DNR.clamp.dlt  = 302;
	g_kdrv_ife_2DNR.clamp.mul  = 69;
	g_kdrv_ife_2DNR.clamp.th   = 3741;

//------------Color gain------------//
	g_kdrv_ife_cgain.enable = 1;

	g_kdrv_ife_cgain.hinv   = 1;
	g_kdrv_ife_cgain.inv    = 1;
	g_kdrv_ife_cgain.mask   = 4095;

	g_kdrv_ife_cgain.cgain_r  = 314;
	g_kdrv_ife_cgain.cgain_gr = 516;
	g_kdrv_ife_cgain.cgain_gb = 455;
	g_kdrv_ife_cgain.cgain_b  = 742;
	g_kdrv_ife_cgain.cgain_ir = 742;
	g_kdrv_ife_cgain.cofs_r   = 2;
	g_kdrv_ife_cgain.cofs_gr  = 2;
	g_kdrv_ife_cgain.cofs_gb  = 5;
	g_kdrv_ife_cgain.cofs_b   = 10;
	g_kdrv_ife_cgain.cofs_ir  = 10;

	g_kdrv_ife_cgain.bit_field = 1;

//------------- Vig ---------------//
	g_kdrv_ife_vig.enable = 1;

	g_kdrv_ife_vig.dist_th = 109;

	g_kdrv_ife_vig.dither_enable = 1;
	g_kdrv_ife_vig.dither_rst_enable = 1;

	for (i = 0; i < 17; i++) {
		g_kdrv_ife_vig.ch_r_lut [i]  = vig_ch0_lut[i] * 4;
		g_kdrv_ife_vig.ch_gr_lut[i]  = vig_ch1_lut[i] * 4;
		g_kdrv_ife_vig.ch_gb_lut[i]  = vig_ch2_lut[i] * 4;
		g_kdrv_ife_vig.ch_b_lut [i]  = vig_ch3_lut[i] * 4;
		g_kdrv_ife_vig.ch_ir_lut[i]  = vig_ch3_lut[i] * 4;
	}

	g_kdrv_ife_vig_center.ch0.x = 240;
	g_kdrv_ife_vig_center.ch1.x = 240;
	g_kdrv_ife_vig_center.ch2.x = 327;
	g_kdrv_ife_vig_center.ch3.x = 367;

	g_kdrv_ife_vig_center.ch0.y = 43;
	g_kdrv_ife_vig_center.ch1.y = 87;
	g_kdrv_ife_vig_center.ch2.y = 51;
	g_kdrv_ife_vig_center.ch3.y = 55;

//------------ Gbal ----------------//
	g_kdrv_ife_gbal.enable    = 1;

	g_kdrv_ife_gbal.protect_enable   = 1;
	g_kdrv_ife_gbal.diff_th_str      = 0x270;
	g_kdrv_ife_gbal.diff_w_max       = 0x8;
	g_kdrv_ife_gbal.edge_protect_th1 = 0x122;
	g_kdrv_ife_gbal.edge_protect_th0 = 0x11A;
	g_kdrv_ife_gbal.edge_w_max       = 0xA9;
	g_kdrv_ife_gbal.edge_w_min       = 0x2E;

	for (i = 0; i < GBAL_OFS_NUM; i++) {
		g_kdrv_ife_gbal.gbal_ofs[i]  = gbal_ofs[i];
	}

//----------------------------------//
	/* load image */
	fp = vos_file_open(pargv[0], O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", pargv[0]);
		//frm_free_buf_ddr(in_buf_info.va_addr);
		//frm_free_buf_ddr(out_buf_info.va_addr);
		ret = fmem_release_from_cma(In_handle, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
		ret = fmem_release_from_cma(Out_handle, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
		return -EFAULT;
	}
	//old_fs = get_fs();
	//set_fs(get_ds());
	len = vos_file_read(fp, (void *)in1_buf_info.vaddr, in1_buf_info.size);
	vos_file_close(fp);
	//set_fs(old_fs);

	/******** ife d2d flow *********/
	DBG_IND("ife d2d flow start\n");
	id = KDRV_DEV_ID(chip, engine, channel);
	/*open*/
	ife_opencfg.ife_clock_sel = 240;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OPENCFG, (void *)&ife_opencfg);
	handle = kdrv_ife_open(chip, engine);
	//handle = kdrv_ife_open((void *)&dal_ife_open_obj);

	//DBG_IND("kdrv_ife_open\r\n");

	/*set input img parameters*/
	in_img_info.in_src = KDRV_IFE_IN_MODE_D2D;
	in_img_info.type = KDRV_IFE_IN_RGBIR;
	in_img_info.img_info.width = in_w;
	in_img_info.img_info.height = in_h;
	/*in_img_info.img_info.stripe_hn = hn;
	in_img_info.img_info.stripe_hl = hl;
	in_img_info.img_info.stripe_hm = hm;*/
	in_img_info.img_info.st_pix = KDRV_IPP_RGGB_PIX_GR;
	//in_img_info.img_info.cfa_pat = cfa_pat;
	in_img_info.img_info.bit = in_bit;

	in_img_info.img_info.crop_width  = crop_w;
	in_img_info.img_info.crop_height = crop_h;
	in_img_info.img_info.crop_hpos   = crop_hpos;
	in_img_info.img_info.crop_vpos   = crop_vpos;
	//in_img_info.img_info.h_shift     = h_shift;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_IMG, (void *)&in_img_info);

	in_img_lofs.line_ofs_1 = IFE_ALIGN((in_w * in_bit) >> 3, 4);
	in_img_lofs.line_ofs_2 = 0;
#if TEST_FNUM
	if (FNUM == 1) {
		in_img_lofs.line_ofs_2 = IFE_ALIGN((in_w * in_bit) >> 3, 4);
	} else {
		in_img_lofs.line_ofs_2 = 0;
	}
#endif
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_OFS_1, (void *)&in_img_lofs);
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_OFS_2, (void *)&in_img_lofs);

	/*set input1 address*/
	in_img_addr.ife_in_addr_1 = (UINT32)in1_buf_info.vaddr;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_ADDR_1, (void *)&in_img_addr);
	DBG_IND("^Ykdrv_ife_Set In Addr1: 0x%08x !!\r\n", in_img_addr.ife_in_addr_1);
	/*set input2 address*/
	in_img_addr.ife_in_addr_2 = (UINT32)in1_buf_info.vaddr;//in2_buf_info.vaddr;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_ADDR_2, (void *)&in_img_addr);
	DBG_IND("^Ykdrv_ife_Set In Addr2: 0x%08x !!\r\n", in_img_addr.ife_in_addr_2);
	/*set output img parameters*/
	out_img_info.line_ofs = IFE_ALIGN((in_w * out_bit) >> 3, 4);
	out_img_info.bit = out_bit;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);

	/*set output address*/
	out_addr = (UINT32)out_buf_info.vaddr;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OUT_ADDR, (void *)&out_addr);

	//---------Ring buffer -----------//
	ring_buf_info.dmaloop_en   = 1;
	ring_buf_info.dmaloop_line = 10;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_RINGBUF_INFO, (void *)&ring_buf_info);
	mirror.enable = 1;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_MIRROR, (void *)&mirror);

	rde_info.enable = 0;
	rde_info.encode_rate = 0;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_RDE_INFO, (void *)&rde_info);


	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_FUSION_INFO, (void *)&fusion_info);

	kdrv_ife_fmd_flag = 0;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ISR_CB, (void *)&KDRV_IFE_ISR_CB);

	/*set interrupt enable*/
	inte_en = KDRV_IFE_INTE_ALL;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_INTER, (void *)&inte_en);

	/* set nrs para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_NRS, (void *)&g_kdrv_ife_nrs);

	/* set fusion para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FUSION, (void *)&g_kdrv_ife_fusion);

	/* set fcurve para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FCURVE, (void *)&g_kdrv_ife_fcurve);

	/* set outlier para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_OUTL, (void *)&g_kdrv_ife_outl);

	/* set 2dnr para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FILTER, (void *)&g_kdrv_ife_2DNR);

	/* set color gain para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_CGAIN, (void *)&g_kdrv_ife_cgain);

	/* set Vig para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_VIG, (void *)&g_kdrv_ife_vig);
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_VIG_CENTER, (void *)&g_kdrv_ife_vig_center);

	/* set gbal para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_GBAL, (void *)&g_kdrv_ife_gbal);

	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ALL_FUNC_EN, (void *)&g_kdrv_ife_ipl_ctrl);
	DBG_IND("kdrv_ife_set done\r\n");

	/*trig*/
	kdrv_ife_trigger(id, NULL, NULL, NULL);

	/*wait end*/
	while (!kdrv_ife_fmd_flag) {
		msleep(10);
	}
	ife_pause();
	/*close*/
	kdrv_ife_close(chip, engine);
	DBG_IND("ife d2d flow end\n");
	/******** ife d2d flow *********/

	/* save image */
	fp = vos_file_open("/mnt/sd/output.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/output.raw");
		//frm_free_buf_ddr(in_buf_info.va_addr);
		//frm_free_buf_ddr(out_buf_info.va_addr);
		ret = fmem_release_from_cma(In_handle, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
		ret = fmem_release_from_cma(Out_handle, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
		return -EFAULT;
	}
	//old_fs = get_fs();
	//set_fs(get_ds());
	len += vos_file_write(fp, (void *)out_buf_info.vaddr, out_buf_info.size);
	vos_file_close(fp);
	//set_fs(old_fs);

	/* free buffer */
	//frm_free_buf_ddr(in_buf_info.va_addr);
	//frm_free_buf_ddr(out_buf_info.va_addr);
	ret = fmem_release_from_cma(In_handle, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}
	ret = fmem_release_from_cma(Out_handle, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}

	return len;
#else
	return 0;
#endif
}
#endif

int nvt_ife_api_read_reg(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_ife_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

int nvt_ife_api_write_reg(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_ife_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

#if !defined (CONFIG_NVT_SMALL_HDAL)
int nvt_ife_api_hw_power_saving(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable = 1;

	if (argc > 0) {
		if (kstrtoint(pargv[0], 0, &enable)) {
			DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
			return 1;
		}
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		if (clk_get_phase(pmodule_info->pclk[0]) == 0) {
			clk_set_phase(pmodule_info->pclk[0], 1);
		} else {
			DBG_ERR("ife module clock auto gating already enable!\r\n");
			return 1;
		}
		if (clk_get_phase(pmodule_info->p_pclk[0]) == 0) {
			clk_set_phase(pmodule_info->p_pclk[0], 1);
		} else {
			DBG_ERR("ife pclock auto gating already enable!\r\n");
			return 1;
		}
	} else {
		if (clk_get_phase(pmodule_info->pclk[0]) == 1) {
			clk_set_phase(pmodule_info->pclk[0], 0);
		} else {
			DBG_ERR("ife module auto gating already enable!\r\n");
			return 1;
		}
		if (clk_get_phase(pmodule_info->p_pclk[0]) == 1) {
			clk_set_phase(pmodule_info->p_pclk[0], 0);
		} else {
			DBG_ERR("Enable pclock auto gating failed!\r\n");
			return 1;
		}
	}

	DBG_IND("ife hw power saving setting done\r\n");

	return 0;
}

int nvt_ife_api_fw_power_saving(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable = 1;

	if (argc > 0) {
		if (kstrtoint(pargv[0], 0, &enable)) {
			DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
			return 1;
		}
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		ife_fw_power_saving_en = TRUE;
	} else {
		ife_fw_power_saving_en = FALSE;
	}

	DBG_IND("ife fw power saving setting done\r\n");

	return 0;
}
#endif
#if 1
int nvt_ife_api_write_pattern(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if (IFE_TOOL_KIT_EN == 1)

	//mm_segment_t old_fs;
	//struct file *fp;
	VOS_FILE fp;
	int len = 0;
	int ret = 0;
	void *pat_handle = NULL;
	//unsigned char* pbuffer;
	//UINT32 pbuffer;
	//dma_addr_t *phyAddr;
	//UINT32 *uiVirAddr;

	struct nvt_fmem_mem_info_t      buf_info = {0};

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}
	fp = vos_file_open(pargv[0], O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

	//Allocate memory
#if 0 // 680 linux
	buf_info.size = 0x100000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "nvtmpp";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);
#else
	ret = nvt_fmem_mem_info_init(&buf_info, NVT_FMEM_ALLOC_CACHE, 0x100000, NULL);
	if (ret >= 0) {
		pat_handle = fmem_alloc_from_cma(&buf_info, 0);
	}
#endif

	nvt_dbg(IND, "buf_info.va_addr = 0x%08x, buf_info.phy_addr = 0x%08x\r\n", (UINT32)buf_info.vaddr, (UINT32)buf_info.paddr);

	//old_fs = get_fs();
	//set_fs(get_ds());

	len = vos_file_read(fp, (void *)buf_info.vaddr, 256);

	/* Do something after get data from file */
	ife_test((UINT32)buf_info.vaddr);

	//kfree(pbuffer);
	vos_file_close(fp);
	//set_fs(old_fs);

	return len;
#else
	return 0;
#endif
}
#endif

void ife_test(UINT32 addr)
{
#if (IFE_TOOL_KIT_EN == 1)

	IFE_OPENOBJ ObjCB;
	IFE_PARAM FilterInfo = {0};
	UINT32 spatial_w[6] = {0, 0, 0, 0, 0, 0};

	FilterInfo.p_spatial_weight = &spatial_w[0];

	ObjCB.FP_IFEISR_CB = NULL;
	ObjCB.ui_ife_clock_sel = 240;
	ife_open(&ObjCB);

	FilterInfo.width = 16;
	FilterInfo.height = 16;
	FilterInfo.in_addr0 = addr;
	FilterInfo.in_ofs0 = 16;
	FilterInfo.out_addr = addr + 16 * 16 + 16;
	FilterInfo.out_ofs = 16;

	ife_set_mode(&FilterInfo);
	ife_start();
	ife_wait_flag_frame_end();
#endif
}


#endif


#if (defined __UITRON || defined __ECOS || defined __FREERTOS)

#if (IFE_TOOL_KIT_EN == 1)
void set_nrs_para(BOOL func_enable)
{
	//---------NRS--------------------//
	g_kdrv_ife_nrs.enable = func_enable;

	g_kdrv_ife_nrs.order.enable     = 1;
	g_kdrv_ife_nrs.order.rng_bri    = 7;
	g_kdrv_ife_nrs.order.rng_dark   = 1;
	g_kdrv_ife_nrs.order.diff_th    = 591;
	for (i = 0; i < 8; i++) {
		g_kdrv_ife_nrs.order.dark_w[i]  = IFE_F_NRS_ORD_DARK_WLUT[i];
		g_kdrv_ife_nrs.order.bri_w[i]  = IFE_F_NRS_ORD_BRI_WLUT[i];
	}

	g_kdrv_ife_nrs.bilat.b_enable   = 1;
	g_kdrv_ife_nrs.bilat.b_str      = 4;
	for (i = 0; i < 6; i++) {
		g_kdrv_ife_nrs.bilat.b_ofs[i]      = NRS_B_OFS[i];
		g_kdrv_ife_nrs.bilat.b_weight[i]   = NRS_B_W[i];
	}
	for (i = 0; i < 5; i++) {
		g_kdrv_ife_nrs.bilat.b_rng[i]      = NRS_B_RNG[i];
		g_kdrv_ife_nrs.bilat.b_th[i]       = NRS_B_TH[i];
	}

	g_kdrv_ife_nrs.bilat.gb_enable  = 1;
	g_kdrv_ife_nrs.bilat.gb_str     = 4;
	g_kdrv_ife_nrs.bilat.gb_blend_w = 9;
	for (i = 0; i < 6; i++) {
		g_kdrv_ife_nrs.bilat.gb_ofs[i]     = NRS_GB_OFS[i];
		g_kdrv_ife_nrs.bilat.gb_weight[i]  = NRS_GB_W[i];
	}
	for (i = 0; i < 5; i++) {
		g_kdrv_ife_nrs.bilat.gb_rng[i]     = NRS_GB_RNG[i];
		g_kdrv_ife_nrs.bilat.gb_th[i]      = NRS_GB_TH[i];
	}
}

void set_fusion_para(BOOL func_enable, UINT32 frame_num)
{
	//---------Fusion--------------------//
	g_kdrv_ife_fusion_info.enable = func_enable;

	g_kdrv_ife_fusion_info.fnum = frame_num;
	g_kdrv_ife_fusion.fu_ctrl.mode = 1;
	g_kdrv_ife_fusion.fu_ctrl.y_mean_sel = 2;
	g_kdrv_ife_fusion.fu_ctrl.ev_ratio = 231;

	g_kdrv_ife_fusion.bld_cur.nor_sel = 1;
	g_kdrv_ife_fusion.bld_cur.dif_sel = 2;

	g_kdrv_ife_fusion.bld_cur.l_dif_knee[0] = 953;
	//g_kdrv_ife_fusion.bld_cur.l_dif_knee[1] = 2636;
	g_kdrv_ife_fusion.bld_cur.l_dif_range   = 12;
	g_kdrv_ife_fusion.bld_cur.l_dif_w_edge  = 1;
	g_kdrv_ife_fusion.bld_cur.l_nor_knee[0] = 1783;
	//g_kdrv_ife_fusion.bld_cur.l_nor_knee[1] = 3345;
	g_kdrv_ife_fusion.bld_cur.l_nor_range   = 3;
	g_kdrv_ife_fusion.bld_cur.l_nor_w_edge  = 1;

	g_kdrv_ife_fusion.bld_cur.s_dif_knee[0] = 220;
	//g_kdrv_ife_fusion.bld_cur.s_dif_knee[1] = 1577;
	g_kdrv_ife_fusion.bld_cur.s_dif_range   = 1;
	g_kdrv_ife_fusion.bld_cur.s_dif_w_edge  = 1;
	g_kdrv_ife_fusion.bld_cur.s_nor_knee[0] = 1450;
	// g_kdrv_ife_fusion.bld_cur.s_nor_knee[1] = 2299;
	g_kdrv_ife_fusion.bld_cur.s_nor_range   = 7;
	g_kdrv_ife_fusion.bld_cur.s_nor_w_edge  = 1;

	g_kdrv_ife_fusion.dk_sat.th[0]         = 2749;
	g_kdrv_ife_fusion.dk_sat.th[1]         = 3013;
	g_kdrv_ife_fusion.dk_sat.step[0]       = 215;
	g_kdrv_ife_fusion.dk_sat.step[1]       = 12;
	g_kdrv_ife_fusion.dk_sat.low_bound[0]  = 230;
	g_kdrv_ife_fusion.dk_sat.low_bound[1]  = 74;

	g_kdrv_ife_fusion.mc_para.diff_ratio   = 1;
	g_kdrv_ife_fusion.mc_para.lum_th       = 3903;
	g_kdrv_ife_fusion.mc_para.dwd          = 9;

	for (i = 0; i < 16; i++) {
		g_kdrv_ife_fusion.mc_para.neg_diff_w[i]   = MC_NEG_DIFF_W[i];
		g_kdrv_ife_fusion.mc_para.pos_diff_w[i]   = MC_POS_DIFF_W[i];
	}

	g_kdrv_ife_fusion.s_comp.enable      = FALSE;
	g_kdrv_ife_fusion.s_comp.knee[0]     = 103;
	g_kdrv_ife_fusion.s_comp.knee[1]     = 651;
	g_kdrv_ife_fusion.s_comp.knee[2]     = 1086;
	for (i = 0; i < 4; i++) {
		g_kdrv_ife_fusion.s_comp.shift[i]       = SCOMP_SHIFT[i];
		g_kdrv_ife_fusion.s_comp.sub_point[i]   = SCOMP_SUB[i];
	}

	g_kdrv_ife_fusion.fu_cgain.enable = FALSE;
	g_kdrv_ife_fusion.fu_cgain.bit_field = 0;

	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_r   = FCGAIN_1[0];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_gr  = FCGAIN_1[1];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_gb  = FCGAIN_1[2];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_b   = FCGAIN_1[3];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path0_ir  = FCGAIN_1[4];

	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_r   = FCGAIN_2[0];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_gr  = FCGAIN_2[1];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_gb  = FCGAIN_2[2];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_b   = FCGAIN_2[3];
	g_kdrv_ife_fusion.fu_cgain.fcgain_path1_ir  = FCGAIN_2[4];

	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_r   = FCGAIN_OFS_1[0];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_gr  = FCGAIN_OFS_1[1];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_gb  = FCGAIN_OFS_1[2];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_b   = FCGAIN_OFS_1[3];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path0_ir  = FCGAIN_OFS_1[4];

	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_r   = FCGAIN_OFS_2[0];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_gr  = FCGAIN_OFS_2[1];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_gb  = FCGAIN_OFS_2[2];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_b   = FCGAIN_OFS_2[3];
	g_kdrv_ife_fusion.fu_cgain.fcofs_path1_ir  = FCGAIN_OFS_2[4];
}

void set_fcurve_para(BOOL func_enable)
{
	//---------Fcurve--------------------//
	g_kdrv_ife_fcurve.enable = func_enable;

	g_kdrv_ife_fcurve.fcur_ctrl.yv_w = 1;
	g_kdrv_ife_fcurve.fcur_ctrl.y_mean_sel = 2;
	for (i = 0; i < FCURVE_Y_W_NUM; i++) {
		g_kdrv_ife_fcurve.y_weight.y_w_lut[i] = FC_Y_W[i];
	}
	for (i = 0; i < FCURVE_IDX_NUM; i++) {
		g_kdrv_ife_fcurve.index.idx_lut[i]    = FC_IDX[i];
	}
	for (i = 0; i < FCURVE_SPLIT_NUM; i++) {
		g_kdrv_ife_fcurve.split.split_lut[i]    = FC_SPLIT[i];
	}
	for (i = 0; i < FCURVE_VAL_NUM; i++) {
		g_kdrv_ife_fcurve.value.val_lut[i]    = FC_VAL[i];
	}
}

void set_outl_para(BOOL func_enable)
{
	//---------Outlier--------------------//
	g_kdrv_ife_outl.enable = 0;

	g_kdrv_ife_outl.bright_ofs     = 0xFF;
	g_kdrv_ife_outl.dark_ofs       = 0x2B;
	for (i = 0; i < OUTL_BRI_TH_NUM; i++) {
		g_kdrv_ife_outl.bright_th[i]   = bright_th[i];
		g_kdrv_ife_outl.dark_th[i]     = dark_th[i];
	}
	g_kdrv_ife_outl.outl_cnt[0]    = outl_cnt[0];
	g_kdrv_ife_outl.outl_cnt[1]    = outl_cnt[1];
	g_kdrv_ife_outl.avg_mode       = 1;

	g_kdrv_ife_outl.outl_weight    = 0x90;

	g_kdrv_ife_outl.ord_rng_bri    = 6;
	g_kdrv_ife_outl.ord_rng_dark   = 7;
	g_kdrv_ife_outl.ord_protect_th = 0x123;
#if NAME_ERR
	g_kdrv_ife_outl.ord_blend_w   = 0x57;
#else
	g_kdrv_ife_outl.ord_bleand_w   = 0x57;
#endif
	for (i = 0; i < OUTL_ORD_W_NUM; i++) {
		g_kdrv_ife_outl.ord_bri_w[i]   = ord_bri_w[i];
		g_kdrv_ife_outl.ord_dark_w[i]  = ord_dark_w[i];
	}
}

void set_filter_para(BOOL func_enable)
{

	//------------2DNR------------------//

	g_kdrv_ife_2DNR.enable = 0;

	for (i = 0; i < SPATIAL_W_LEN; i++) {
		g_kdrv_ife_2DNR.spatial.weight[i] = s_weight[i];
	}

	for (i = 0; i < RANGE_A_LUT_SIZE; i++) {
		g_kdrv_ife_2DNR.rng_filt_r.a_lut[i] = range_a_ch0_lut[i];
		g_kdrv_ife_2DNR.rng_filt_r.b_lut[i] = range_b_ch0_lut[i];

		g_kdrv_ife_2DNR.rng_filt_gr.a_lut[i] = range_a_ch1_lut[i];
		g_kdrv_ife_2DNR.rng_filt_gr.b_lut[i] = range_b_ch1_lut[i];

		g_kdrv_ife_2DNR.rng_filt_gb.a_lut[i] = range_a_ch2_lut[i];
		g_kdrv_ife_2DNR.rng_filt_gb.b_lut[i] = range_b_ch2_lut[i];

		g_kdrv_ife_2DNR.rng_filt_b.a_lut[i] = range_a_ch3_lut[i];
		g_kdrv_ife_2DNR.rng_filt_b.b_lut[i] = range_b_ch3_lut[i];

		g_kdrv_ife_2DNR.rng_filt_ir.a_lut[i] = range_a_ch3_lut[i];
		g_kdrv_ife_2DNR.rng_filt_ir.b_lut[i] = range_b_ch3_lut[i];
	}
	for (i = 0; i < RANGE_A_TH_NUM; i++) {
		g_kdrv_ife_2DNR.rng_filt_r.a_th[i] = rth_a_ch0[i];
		g_kdrv_ife_2DNR.rng_filt_r.b_th[i] = rth_b_ch0[i];

		g_kdrv_ife_2DNR.rng_filt_gr.a_th[i] = rth_a_ch1[i];
		g_kdrv_ife_2DNR.rng_filt_gr.b_th[i] = rth_b_ch1[i];

		g_kdrv_ife_2DNR.rng_filt_gb.a_th[i] = rth_a_ch2[i];
		g_kdrv_ife_2DNR.rng_filt_gb.b_th[i] = rth_b_ch2[i];

		g_kdrv_ife_2DNR.rng_filt_b.a_th[i] = rth_a_ch3[i];
		g_kdrv_ife_2DNR.rng_filt_b.b_th[i] = rth_b_ch3[i];

		g_kdrv_ife_2DNR.rng_filt_ir.a_th[i] = rth_a_ch3[i];
		g_kdrv_ife_2DNR.rng_filt_ir.b_th[i] = rth_b_ch3[i];
	}

	g_kdrv_ife_2DNR.rbfill.enable       = 1;
	for (i = 0; i < RBFILL_LUMA_NUM; i++) {
		g_kdrv_ife_2DNR.rbfill.luma[i]  = rbfill_luma[i];
	}
	for (i = 0; i < RBFILL_RATIO_NUM; i++) {
		g_kdrv_ife_2DNR.rbfill.ratio[i] = rbfill_ratio[i];
	}
	g_kdrv_ife_2DNR.rbfill.ratio_mode  = 1;

	g_kdrv_ife_2DNR.center_mod.enable  = 1;   //0x44
	g_kdrv_ife_2DNR.center_mod.th1     = 65;  //0x44
	g_kdrv_ife_2DNR.center_mod.th2     = 745; //0x44

	g_kdrv_ife_2DNR.rng_th_w   = 5; //<- RTH_W
	g_kdrv_ife_2DNR.blend_w    = 7;
	g_kdrv_ife_2DNR.bin        = 1;

	g_kdrv_ife_2DNR.clamp.dlt  = 302;
	g_kdrv_ife_2DNR.clamp.mul  = 69;
	g_kdrv_ife_2DNR.clamp.th   = 3741;
}

void set_cgain_para(BOOL func_enable)
{
	//------------Color gain------------//
	g_kdrv_ife_cgain.enable = func_enable;

	g_kdrv_ife_cgain.hinv   = 1;
	g_kdrv_ife_cgain.inv    = 1;
	g_kdrv_ife_cgain.mask   = 4095;

	g_kdrv_ife_cgain.cgain_r  = 314;
	g_kdrv_ife_cgain.cgain_gr = 516;
	g_kdrv_ife_cgain.cgain_gb = 455;
	g_kdrv_ife_cgain.cgain_b  = 742;
	g_kdrv_ife_cgain.cgain_ir = 742;
	g_kdrv_ife_cgain.cofs_r   = 2;
	g_kdrv_ife_cgain.cofs_gr  = 2;
	g_kdrv_ife_cgain.cofs_gb  = 5;
	g_kdrv_ife_cgain.cofs_b   = 10;
	g_kdrv_ife_cgain.cofs_ir  = 10;

	g_kdrv_ife_cgain.bit_field = 1;
}

void set_vig_para(BOOL func_enable)
{
	//------------- Vig ---------------//
	g_kdrv_ife_vig.enable = func_enable;

	g_kdrv_ife_vig.dist_th = 109;

	g_kdrv_ife_vig.dither_enable = 1;
	g_kdrv_ife_vig.dither_rst_enable = 1;

	for (i = 0; i < 17; i++) {
		g_kdrv_ife_vig.ch_r_lut [i]  = vig_ch0_lut[i] * 4;
		g_kdrv_ife_vig.ch_gr_lut[i]  = vig_ch1_lut[i] * 4;
		g_kdrv_ife_vig.ch_gb_lut[i]  = vig_ch2_lut[i] * 4;
		g_kdrv_ife_vig.ch_ir_lut[i]  = vig_ch3_lut[i] * 4;
		g_kdrv_ife_vig.ch_b_lut [i]  = vig_ch3_lut[i] * 4;
	}

	g_kdrv_ife_vig_center.ch0.x = 240;
	g_kdrv_ife_vig_center.ch1.x = 240;
	g_kdrv_ife_vig_center.ch2.x = 327;
	g_kdrv_ife_vig_center.ch3.x = 367;

	g_kdrv_ife_vig_center.ch0.y = 43;
	g_kdrv_ife_vig_center.ch1.y = 87;
	g_kdrv_ife_vig_center.ch2.y = 51;
	g_kdrv_ife_vig_center.ch3.y = 55;
}
void set_gbal_para(BOOL func_enable)
{


	//------------ Gbal ----------------//
	g_kdrv_ife_gbal.enable    = 0;

	g_kdrv_ife_gbal.protect_enable   = 1;
	g_kdrv_ife_gbal.diff_th_str      = 0x270;
	g_kdrv_ife_gbal.diff_w_max       = 0x8;
	g_kdrv_ife_gbal.edge_protect_th1 = 0x122;
	g_kdrv_ife_gbal.edge_protect_th0 = 0x11A;
	g_kdrv_ife_gbal.edge_w_max       = 0xA9;
	g_kdrv_ife_gbal.edge_w_min       = 0x2E;

	for (i = 0; i < GBAL_OFS_NUM; i++) {
		g_kdrv_ife_gbal.gbal_ofs[i]  = gbal_ofs[i];
	}

}
#endif

#endif
