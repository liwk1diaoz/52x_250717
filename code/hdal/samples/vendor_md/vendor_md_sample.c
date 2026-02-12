#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "md_lib.h"
#include "vendor_isp.h"

#define HW_MD_ENABLE 1

//============================================================================
// global
//============================================================================
pthread_t md_thread_id;
static UINT32 conti_run = 1;
static ISPT_LA_DATA la_data = {0};

static void *md_thread(void *arg)
{
	ISPT_WAIT_VD wait_vd = {0};
	#if (HW_MD_ENABLE)
	ISPT_MD_DATA md_data = {0};
	#endif
	MD_LIB_RSLT rslt;
	UINT32 i, j;

	while (conti_run) {
		wait_vd.id = 0;
		wait_vd.timeout = 100;

		for (i = 0; i < 30; i++) {
			vendor_isp_get_common(ISPT_ITEM_WAIT_VD, &wait_vd);

			la_data.id = 0;
			vendor_isp_get_common(ISPT_ITEM_LA_DATA, &la_data);
			//printf("1 data = %d, %d, %d \n", la_data.la_rslt.lum_1[0], la_data.la_rslt.lum_1[528], la_data.la_rslt.lum_1[529]);
			md_trig_flow(0, (MD_LA_DATA *)&la_data.la_rslt.lum_1);
		}

		md_get_rslt(0, &rslt);

		printf("sw_md_data, blk_dif_cnt = %d, total_blk_diff = %d, rslt = %d \n", rslt.blk_dif_cnt, rslt.total_blk_diff, rslt.rslt);
		for (i = 0; i < ISP_LA_H_WINNUM; i++) {
			printf("%2d | ", i);
			for (j = 0; j < ISP_LA_W_WINNUM; j++) {
				if (rslt.th[i*ISP_LA_W_WINNUM + j] == 1) {
					printf("\033[1;32;40m %d\033[0m", rslt.th[i*ISP_LA_H_WINNUM + j]);
				} else {
					printf("\033[0m %d\033[0m", rslt.th[i*ISP_LA_H_WINNUM + j]);
				}
			}
			printf("\n");
		}

		#if (HW_MD_ENABLE)
		md_data.id = 0;
		vendor_isp_get_common(ISPT_ITEM_MD_DATA, &md_data);
		printf("hw_md_data, blk_dif_cnt = %d, total_blk_diff = %d \n", md_data.md_rslt.blk_dif_cnt, md_data.md_rslt.total_blk_diff);
		for (i = 0; i < ISP_LA_H_WINNUM; i++) {
			printf("%2d | ", i);
			for (j = 0; j < ISP_LA_W_WINNUM; j++) {
				if (md_data.md_rslt.th[i*ISP_LA_W_WINNUM + j] == 1) {
					printf("\033[1;31;40m %d\033[0m", md_data.md_rslt.th[i*ISP_LA_H_WINNUM + j]);
				} else {
					printf("\033[0m %d\033[0m", md_data.md_rslt.th[i*ISP_LA_H_WINNUM + j]);
				}
			}
			printf("\n");
		}
		#endif
	};

	return 0;
}

static INT32 get_choose_int(void)
{
	char buf[256];
	INT val, rt;

	rt = scanf("%d", &val);

	if (rt != 1) {
		printf("Invalid option. Try again.\n");
		clearerr(stdin);
		fgets(buf, sizeof(buf), stdin);
		val = -1;
	}

	return val;
}

int main(int argc, char *argv[])
{
	UINT32 trig = 1;
	INT32 option;
	MD_LIB_PARAM param;
	#if (HW_MD_ENABLE)
	IQT_MD_PARAM md_param = {0};
	#endif

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
		printf("----------------------------------------\n");
		printf("   1. Get sw md param \n");
		printf("   2. Set sw md param \n");
		#if (HW_MD_ENABLE)
		printf("   3. Get hw md param \n");
		printf("   4. Set hw md param \n");
		#endif
		printf("   5. Display md data \n");
		printf("----------------------------------------\n");
		printf(" 0.  Quit\n");
		printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			md_get_param(0, &param);
			printf("sum_frms = %d, mask = 0x%X 0x%X, thr = %d %d %d \n", param.sum_frms, param.mask0 , param.mask1 , param.blkdiff_thr, param.blkdiff_cnt_thr, param.total_blkdiff_thr);
			break;

		case 2:
			md_get_param(0, &param);
	
			do {
				printf("Set blkdiff_thr>> \n");
				param.blkdiff_thr = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set blkdiff_cnt_thr>> \n");
				param.blkdiff_cnt_thr = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set total_blkdiff_thr>> \n");
				param.total_blkdiff_thr = (UINT32)get_choose_int();
			} while (0);

			md_set_param(0, &param);
			break;

		#if (HW_MD_ENABLE)

		case 3:
			md_param.id = 0;
			vendor_isp_get_iq(IQT_ITEM_MD_PARAM, &md_param);

			printf("sum_frms = %d, mask = 0x%X 0x%X, thr = %d %d %d \n", md_param.md.sum_frms, md_param.md.mask0 , md_param.md.mask1 , md_param.md.blkdiff_thr, md_param.md.blkdiff_cnt_thr, md_param.md.total_blkdiff_thr);
			break;

		case 4:
			md_param.id = 0;
			vendor_isp_get_iq(IQT_ITEM_MD_PARAM, &md_param);

			do {
				printf("Set blkdiff_thr>> \n");
				md_param.md.blkdiff_thr = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set blkdiff_cnt_thr>> \n");
				md_param.md.blkdiff_cnt_thr = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set total_blkdiff_thr>> \n");
				md_param.md.total_blkdiff_thr = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_MD_PARAM, &md_param);
			break;
		#endif

		case 5:
			conti_run = 1;
			if (pthread_create(&md_thread_id, NULL, md_thread, NULL) < 0) {
				printf("create encode thread failed");
				break;
			}

			do {
				printf("Enter 0 to exit >> \n");
				conti_run = (UINT32)get_choose_int();
			} while (0);

			// destroy encode thread
			pthread_join(md_thread_id, NULL);
			break;

		case 0:
		default:
			trig = 0;
			break;

		}
	}

	if (vendor_isp_uninit() == HD_ERR_NG) {
		return -1;
	}

	return 0;
}

