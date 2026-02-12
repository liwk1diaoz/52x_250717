
#include <string.h>
#include <stdlib.h>

#include "dpc_lib.h"
#include "vendor_isp.h"
//#include "ispt_api.h"
#include "kwrap/file.h"
#include "kwrap/util.h"
#define MEASURE_TIME 0
#if (MEASURE_TIME)
#include "hd_common.h"
#include "hd_util.h"
#endif


#define PAUSE {printf("Press Enter key to continue..."); fgetc(stdin);}

#define CALI_MIN(x,y)    ((x) < (y) ? (x) : (y))
#define CALI_MAX(x,y)    ((x) > (y) ? (x) : (y))

static unsigned int get_choose_uint(void)
{
	char str_buf[32];
	unsigned int val, error;
	do {
		error = scanf(" %d", &val);
		if (error != 1) {
			printf("Invalid option. Try again.\n");
			clearerr(stdin);
			fgets(str_buf, sizeof(str_buf), stdin);
			printf(">> ");
		}
	} while(error != 1);

	return val;
}

static IQT_DPC_PARAM dpc_param;
static void set_manual_ae(unsigned int sensor_id)
{
	AET_MANUAL ae={0};

	ae.id = sensor_id;
	ae.manual.mode= 1;
	ae.manual.totalgain= 0;
	printf("----------------------------------------\n");
	printf("SET AE Exposure&Gain: \r\n");
	printf("Enter Exposure:  \r\n");
	do {
		printf(">> ");
		ae.manual.expotime = get_choose_uint();
		printf("exposure=%d \r\n", ae.manual.expotime);
	} while (0);

	printf("Enter Gain:  \r\n");
	do {
		printf(">> ");
		ae.manual.iso_gain = get_choose_uint();
		printf("gain=%d \r\n", ae.manual.iso_gain);
	} while (0);

	set_AE(&ae);
}

int main(int argc, char *argv[])
{
	unsigned int option, infinite_loop = 1;

	ISPT_RAW_INFO raw_info = {0};

	CAL_ALG_DP_RST dp_rst;
	unsigned int sensor_id;
	AET_MANUAL ae={0};
	unsigned int dpc_th = 20; //defual 20
	unsigned int dbg_en = 0;
	unsigned int dpc_en = 0;
	int open_device;
	IQT_CFG_INFO cfg_info = {0};

	int ret = 0;
	//char dbg_raw_file_path[64] = "/mnt/sd/\0";

	ISPT_SENSOR_MODE_INFO sensor_mode_info;
	unsigned int raw_size, avg, j;
	char *raw_buf;
	char *tmp_buf;
	CAL_ALG_DP_PARAM dp_param;
	UINT32* dp_buf = NULL;

	open_device = vendor_isp_init();

	if (open_device < 0) {
		printf("open MCU device fail!\n");
		return E_GET_DEV_FAIL;
	}

	// Enter sensor id
/*
	printf("----------------------------------------\n");
	printf("Available sensor id list: \r\n");
	get_sensor_id();
*/
	printf("----------------------------------------\n");
	printf("sample 2.00, lib: %x\r\n",get_dpc_lib_version());

	printf("Enter sensor id: {0,1,2}  \r\n");
	do {
		printf(">> ");
		sensor_id = get_choose_uint();
		printf("cali_sensor_id=%d \r\n", sensor_id);
		raw_info.id = sensor_id;
		cfg_info.id = sensor_id;
		dpc_param.id = sensor_id;
		ae.id = sensor_id;
	} while (0);



	while (infinite_loop) {
		printf("----------------------------------------\n");
		printf(" 1. Start dpc cali\n");
		printf(" 2. load dpc bin\n");
		printf(" 3. Start dpc cali bright and dark pixel\n");
		printf(" 4. dpc on/off\n");
		printf(" 5. Dbg mode on/off\n");
		printf(" 6. Quit\n");
		printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_uint();
		} while (0);

		switch (option) {
		case 1:


			{
				set_manual_ae(sensor_id);
				// input threshold
			    printf("Enter threshold:  \r\n");
			    do {
			            printf(">> ");
			            dpc_th = get_choose_uint();
			            printf("dpc threshold = %d \r\n", dpc_th);
			    } while (0);

				dpc_get_sensor_info(sensor_id, &sensor_mode_info);
				//allocte raw buffer
				raw_size = sensor_mode_info.info.crp_size.w* sensor_mode_info.info.crp_size.h;
				raw_buf = (char *)malloc(sizeof(char)*raw_size);
				tmp_buf = (char *)malloc(sizeof(char)*raw_size* 3 / 2);
				if (raw_buf != NULL) {
					memset(raw_buf, 0, sizeof(char)*raw_size);
				} else	{
					printf("fail to allocate memory for image buffer!\n");
					break;
				}
				if (tmp_buf != NULL) {
					memset(tmp_buf, 0, sizeof(char)*raw_size* 3 / 2);
				} else	{
					printf("fail to allocate memory for image buffer!\n");
					break;
				}

				raw_info.raw_info.addr = (UINT32)raw_buf;

				if (dpc_get_raw(sensor_id, &raw_info, (UINT32)tmp_buf) == 0) {

					dp_param.raw_info = &raw_info;
					avg = 0;
					for (j = 0; j < raw_size; j++) {
						avg += raw_buf[j];
					}
					avg /= raw_size;
					printf("average lum = %d\r\n", avg);

					dp_buf = (UINT32 *)malloc(raw_info.raw_info.pw * raw_info.raw_info.ph*sizeof(UINT32));
					if (dp_buf != NULL) {
						memset(dp_buf, 0, raw_info.raw_info.pw * raw_info.raw_info.ph*sizeof(UINT32));
					} else{
						printf("fail to allocate memory for image buffer!\n");
						break;
					}


					dp_param.dp_pool_addr = (unsigned int)&dp_buf[0];
					dp_param.setting.threshold = dpc_th;
					dp_param.ori_dp_cnt = 0;
					dp_param.b_chg_dp_format = DP_PARAM_CHGFMT_AUTO;

					cal_dp_process(sensor_id, dp_param.raw_info, &dp_param, &dp_rst, &sensor_mode_info);

					printf("cnt=%d,data_length=%d \r\n", dp_rst.pixel_cnt, dp_rst.data_length);

				}
			}
			// save dp bin
			VOS_FILE fp;
			fp = vos_file_open("/mnt/sd/dp.bin", O_CREAT|O_WRONLY|O_SYNC|O_TRUNC, 0);
			if (fp != -1) {
				int value = 0;
				vos_file_write(fp,dp_buf, dp_rst.data_length);
				vos_file_close(fp);
				if(value != 0)
					vos_util_delay_ms(2000);
				printf("dp_buf:%x\r\n",dp_buf);

			} else {
				printf("fp is NULL\r\n");
			}
			vos_util_delay_ms(1000);

			if (raw_buf != NULL) {
			    free(raw_buf);
			    raw_buf = NULL;
			}
			if (tmp_buf != NULL) {
			    free(tmp_buf);
			    tmp_buf = NULL;
			}
			if (dp_buf != NULL) {
			    free(dp_buf);
			    dp_buf = NULL;
			}

			break;

		case 2:
			// set dp bin
			strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			ret = vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			if (ret < 0) {
				printf("SET IQT_ITEM_RLD_CONFIG fail!\n");
				break;
			}

			// get dpc par
			ret = vendor_isp_get_iq(IQT_ITEM_DPC_PARAM, &dpc_param);
			if (ret < 0) {
				printf("GET IQT_ITEM_DPC_PARAM fail!\n");
				break;
			}
			//printf("dpc_param 0x%x 0x%x\e\n",dpc_param.dpc.table[0],dpc_param.dpc.table[1]);
			break;

		case 3:
			{

				//get raw info
				dpc_get_sensor_info(sensor_id, &sensor_mode_info);
				raw_size = sensor_mode_info.info.crp_size.w* sensor_mode_info.info.crp_size.h;
				//get raw buffer
				raw_buf = (char *)malloc(sizeof(char)*raw_size);
				tmp_buf = (char *)malloc(sizeof(char)*raw_size* 3 / 2);

				if (raw_buf != NULL) {
					memset(raw_buf, 0, sizeof(char)*raw_size);
				} else	{
					printf("fail to allocate memory for image buffer!\n");
					break;
				}
				if (tmp_buf != NULL) {
					memset(tmp_buf, 0, sizeof(char)*raw_size* 3 / 2);
				} else	{
					printf("fail to allocate memory for image buffer!\n");
					break;
				}
				raw_info.raw_info.addr = (UINT32)raw_buf;
				//get dp buffer
				dp_buf = (UINT32 *)malloc(10000*sizeof(UINT32));
				if (dp_buf != NULL) {
					memset(dp_buf, 0, 10000*sizeof(UINT32));
				} else{
					printf("fail to allocate memory for image buffer!\n");
					break;
				}

				//bright pixel
				printf("\n Bright Pixel: SET AE Exposure&Gain \r\n\n");
				set_manual_ae(sensor_id);

				// input threshold
		    	printf("\n Bright Pixel: Enter threshold:  \r\n");
		    	do {
		            printf(">> ");
		            dpc_th = get_choose_uint();
		            printf("dpc threshold = %d \r\n", dpc_th);
		    	} while (0);
				printf("\r\n");

				if (dpc_get_raw(sensor_id, &raw_info, (UINT32)tmp_buf) == 0) {

					dp_param.raw_info = &raw_info;
					avg = 0;
					for (j = 0; j < raw_size; j++) {
						avg += raw_buf[j];
					}
					avg /= raw_size;
					printf("average lum = %d\r\n", avg);

					dp_param.dp_pool_addr = (unsigned int)&dp_buf[0];
					dp_param.setting.threshold = dpc_th;
					dp_param.ori_dp_cnt = 0;
					dp_param.b_chg_dp_format = DP_PARAM_CHGFMT_SKIP;
					cal_dp_process(sensor_id, dp_param.raw_info, &dp_param, &dp_rst, &sensor_mode_info);

					printf("cnt=%d,data_length=%d \r\n", dp_rst.pixel_cnt, dp_rst.data_length);

				}else{
					break;
				}//Bright pixel end

				//Dark pixel
				printf("\n Dright Pixel: SET AE Exposure&Gain \r\n");
				set_manual_ae(sensor_id);

				// input threshold
		    	printf("\n Dright Pixel: Enter threshold:  \r\n");
		    	do {
		            printf(">> ");
		            dpc_th = get_choose_uint();
		            printf("dpc threshold = %d \r\n", dpc_th);
		    	} while (0);
				printf("\r\n");

				if (dpc_get_raw(sensor_id, &raw_info, (UINT32)tmp_buf) == 0) {

					dp_param.raw_info = &raw_info;
					avg = 0;
					for (j = 0; j < raw_size; j++) {
						avg += raw_buf[j];
					}
					avg /= raw_size;
					printf("average lum = %d\r\n", avg);

					dp_param.dp_pool_addr = (unsigned int)&dp_buf[0];
					dp_param.setting.threshold = dpc_th;
					dp_param.ori_dp_cnt = dp_rst.pixel_cnt;
					dp_param.b_chg_dp_format = DP_PARAM_CHGFMT_AUTO;
					cal_dp_process(sensor_id, dp_param.raw_info, &dp_param, &dp_rst, &sensor_mode_info);

					printf("cnt=%d,data_length=%d \r\n", dp_rst.pixel_cnt, dp_rst.data_length);

				}//Dark pixel end

				// save dp bin
				VOS_FILE fp;
				fp = vos_file_open("/mnt/sd/dp.bin", O_CREAT|O_WRONLY|O_SYNC|O_TRUNC, 0);
				if (fp != -1) {
					int value = 0;

					vos_file_write(fp,dp_buf, dp_rst.data_length);
					value = vos_file_close(fp);

					if(value != 0)
						vos_util_delay_ms(2000);
					printf("dp_buf:%x\r\n",dp_buf);

				} else {
					printf("fp is NULL\r\n");
				}

				vos_util_delay_ms(1000);

				if (raw_buf != NULL) {
				    free(raw_buf);
				    raw_buf = NULL;
				}
				if (tmp_buf != NULL) {
				    free(tmp_buf);
				    tmp_buf = NULL;
				}
				if (dp_buf != NULL) {
				    free(dp_buf);
				    dp_buf = NULL;
				}

			}
			break;


		case 4:
			// set dpc enable
			printf("DPC enable: (0:off, 1:on)  \r\n");
			do {
				printf(">> ");
				dpc_en = get_choose_uint();
				printf("dpc_en=%d \r\n", dpc_en);
			} while (0);

			dpc_param.dpc.enable = dpc_en;
			// get dpc par
			ret = vendor_isp_set_iq(IQT_ITEM_DPC_PARAM, &dpc_param);
			if (ret < 0) {
				printf("SET IQT_ITEM_DPC_PARAM fail!\n");
				break;
			}

			break;

		case 5:
			// dbg msg on/off
			printf("dbg enable: (0:off, 1:on)  \r\n");
			do {
				printf(">> ");
				dbg_en = get_choose_uint();
				printf("dbg_en=%d \r\n", dbg_en);
			} while (0);
			cali_dpc_set_dbg_out(dbg_en);
			break;

		case 6:
		default:
			infinite_loop = 0;
			break;
		}
    }

/*
	dpc_exit();
*/
	ae.manual.mode= 0;
	set_AE(&ae);
	ret = vendor_isp_uninit();
	if(ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}


	return 0;
}

