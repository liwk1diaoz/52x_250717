
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "stitch_lib.h"
#include "isp_lib_ver.h"
#include "vendor_isp.h"
#include "vendor_vpe.h"

#define OB 256
#define SAVERAW 0

static ISPT_MEMORY_INFO mem_info;
static ECS_CA ecs_ca;
static IQT_SHADING_PARAM shading;
static IQ_SHADING_PARAM shading_dtsi;
static VPET_2DLUT_PARAM lut2d[2] = {0};

//============================================================================
// global
//============================================================================
static INT32 get_choose_int(void)
{
	CHAR buf[256];
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

BOOL convert_pack12_to_unpack12(UINT32 frame_size, CHAR *srcbuffer, CHAR *dstbuffer, UINT16 ob)
{
	INT32 count, i;
	CHAR b1, b2, b3, *ptr_src;
	UINT16 *ptr_dst;
	UINT16 v1, v2;

	if ((frame_size % 3) != 0) {
		return FALSE;
	}

	count = frame_size / 3;
	ptr_src = srcbuffer;
	ptr_dst = (UINT16 *)dstbuffer;
	for (i = 0; i < count; i++) {
		b1 = ptr_src[0];
		b2 = ptr_src[1];
		b3 = ptr_src[2];
		v1 = ((b2 & 0xF) << 8) | b1;
		v2 = (b3 << 4) | ((b2 >> 4) & 0xF);

		ptr_dst[0] = v1 - ob;
		ptr_dst[1] = v2 - ob;

		ptr_src += 3;
		ptr_dst += 2;
	}

	return TRUE;
}

char lut_bin[2][50] =
{"/mnt/sd/vsp/cam1_LUT257_ROT0_hflip0.bin",
 "/mnt/sd/vsp/cam2_LUT257_ROT0_hflip0.bin"};


int main(int argc, char *argv[])
{
	INT32 option;
	UINT32 trig = 1;
	int dbg_en = 0;
	INT fd, len;
	UINT32 size;
	UINT32 version = 0;
	HD_RESULT ret;
	ISPT_RAW_INFO raw_info = {0};
	UINT32 data_len, residue_len, data;
	UINT32 image_width = 2560, image_height = 1440, j, block;
	CHAR *src_pack12_buffer = NULL, *src_pack12_buffer_1 = NULL, *dst_unpack12_buffer = NULL, *dst_unpack12_buffer_1 = NULL;
	UINT32 pack12_size = 3110400, unpack12_size = 4147200;  // default 1920x1080
	BOOL pack_image_ready = FALSE, unpack_image_ready = FALSE, ecs_calibrate_done = FALSE;
	const char *sensor_name = "os05a10";
	CHAR *shading_data = NULL, *shading_size = NULL, *shading_ver = NULL;
	BOOL ecs_enable;
	AET_MANUAL manual = {0};
	//#if SAVERAW
	CHAR src_raw_path[100], save_raw_path[100];
	FILE *fp = NULL;
	//#endif
	STITCH_PARAM stitch_param = { 0 };
	AUTO_LUMA_RATIO auto_ruma_ratio;
	UINT32 lut_size = 260*257*4;
	UINT32 read_len;
	char   *lut_path;

	ISPT_LA_DATA la_data = {0};

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------------------\n");
			printf("  1. Get RAW from real board \n");
			printf("  2. Get RAW from mnt/sd \n");
			printf("  3. Pre-process RAW, pack -> unpack -> apply OB \n");
			printf("----------------------------------------------------\n");
			printf("  11. Run Stitch ecs calibrate \n");
			printf("  12. Run Stitch ecs simulate \n");
			printf("  13. Save ecs dtsi \n");
			printf("----------------------------------------------------\n");
			printf("  50. auto ratio\n");
			printf("  51. disable auto ratio\n");
			printf("----------------------------------------------------\n");
			printf("  98. get Stitch libary version\n");
			printf("  99. set dbg mode\n");

			printf("  0.  Quit\n");
			printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {

		case 1:
			// disable ecs_enable
			printf("sensor id\n");
			do {
				printf(">> ");
				shading.id = get_choose_int();
				raw_info.id = shading.id;
				manual.id = shading.id;
			} while (0);
			vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
			ecs_enable = shading.shading.ecs_enable;
			shading.shading.ecs_enable = 0;
			vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);

			manual.manual.mode = MANUAL_MODE;
			printf("iso\n");
			do {
				printf(">> ");
				manual.manual.iso_gain = get_choose_int();
			} while (0);
			printf("expotime\n");
			do {
				printf(">> ");
				manual.manual.expotime = get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_MANUAL, &manual);

			usleep(66666);

			// get raw from real board
			ret = vendor_isp_get_common(ISPT_ITEM_RAW, &raw_info);
			if (ret < 0) {
				printf("GET ISPT_ITEM_RAW fail! \n");
				//set raw
				ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
				if (ret < 0) {
					printf("SET ISPT_ITEM_RAW fail! \n");
				}
				goto exit;
			}

			raw_info.raw_info.pxlfmt = raw_info.raw_info.pxlfmt & 0xffff0000;

			printf("get raw, id = %d, %d x %d x %x\n", raw_info.id, raw_info.raw_info.pw, raw_info.raw_info.ph, raw_info.raw_info.pxlfmt);

			if (raw_info.raw_info.pxlfmt != VDO_PXLFMT_RAW12) {
				printf("only support VDO_PXLFMT_RAW12 (input %d)\r\n", raw_info.raw_info.pxlfmt);
				//set raw
				ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
				if (ret < 0) {
					printf("SET ISPT_ITEM_RAW fail! \n");
				}
				goto exit;
			}

			printf("get lineoffset = %d  \r\n", raw_info.raw_info.loff);

			image_width = raw_info.raw_info.pw;
			image_height = raw_info.raw_info.ph;
			pack12_size = image_width * image_height * 3 / 2;

			if ((image_width | image_height) == 0) {
				printf("image_width/image_height can not be zero \r\n");
				//set raw
				ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
				if (ret < 0) {
					printf("SET ISPT_ITEM_RAW fail! \n");
				}
				goto exit;
			}

			// allocate raw buffer(pack)
			if (src_pack12_buffer == NULL) {
				src_pack12_buffer = (CHAR *)malloc(pack12_size);
			}
			if (src_pack12_buffer == NULL) {
				printf("fail to allocate memory for image buffer!\n");
				//set raw
				ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
				if (ret < 0) {
					printf("SET ISPT_ITEM_RAW fail! \n");
				}
				goto exit;
			}

			memset(src_pack12_buffer, 0, pack12_size);

			block = pack12_size % (32 * 1024) ? (pack12_size / 1024 / 32 + 1) : (pack12_size / 1024 / 32);
			printf("block number: %d \r\n", block);

			data_len = 32*1024;
			mem_info.addr = raw_info.raw_info.addr;
			residue_len = pack12_size;
			data = 0;
			//#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/src_w%d_h%d_12b_%d.raw", (int)image_width, (int)image_height, (int)shading.id);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", src_raw_path);
				goto exit;
			}
			//#endif

			for (j = 0; j < block ; j++) {
				mem_info.size = data_len;
				ret = vendor_isp_get_common(ISPT_ITEM_FRAME, &mem_info);

				if (ret < 0) {
					printf("GET ISPT_ITEM_FRAME fail! \n");
					//set raw
					ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
					if (ret < 0) {
						printf("SET ISPT_ITEM_RAW fail! \n");
					}
					//#if SAVERAW
					fclose(fp);
					//#endif
					goto exit;
				}

				memcpy(&src_pack12_buffer[data], mem_info.buf, data_len);
				//#if SAVERAW
				fwrite(src_pack12_buffer+data, sizeof(CHAR), data_len, fp);
				//#endif
				mem_info.addr += 32*1024;
				residue_len -= data_len;
				data += data_len;
				data_len = (residue_len >= 32 * 1024) ? 32 * 1024 : residue_len;
			}

			pack_image_ready = TRUE;

			ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
			if (ret < 0) {
				printf("SET ISPT_ITEM_RAW fail! \n");
				//#if SAVERAW
				fclose(fp);
				//#endif
				goto exit;
			}

			// restore ecs_enable
			shading.shading.ecs_enable = ecs_enable;
			vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);
			//#if SAVERAW
			fclose(fp);
			//#endif
			break;

		case 2:
			// Get RAW from mnt/sd
			image_width = 1920;
			image_height = 1080;
			pack12_size = image_width * image_height * 3 / 2;

			//sensor 0
			// allocate raw buffer(pack)
			if (src_pack12_buffer == NULL) {
				src_pack12_buffer = (CHAR *)malloc(pack12_size);
			}
			if (src_pack12_buffer == NULL) {
				printf("fail to allocate memory for image buffer!\n");
				//set raw
				ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
				if (ret < 0) {
					printf("SET ISPT_ITEM_RAW fail! \n");
				}
				goto exit;
			}

			memset(src_pack12_buffer, 0, pack12_size);

			// load raw file from sd card
			sprintf(src_raw_path, "/mnt/sd/src_w%d_h%d_12b_0.raw", (int)image_width, (int)image_height);
			fd = open(src_raw_path, O_RDONLY);
			if (fd == -1) {
				printf("fail to open %s \n", src_raw_path);
				goto exit;
			}

			len = read(fd, src_pack12_buffer, pack12_size);
			close(fd);
			if (len != (INT)(pack12_size)) {
				printf("fail to read %s \n", src_raw_path);
				goto exit;
			}

			#if SAVERAW
			#if 1
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_0.raw", image_width, image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				return -1;
			}
			fwrite(src_pack12_buffer, sizeof(CHAR), pack12_size, fp);
			fclose(fp);
			#endif
			#endif

			//sensor 1
			// allocate raw buffer(pack)
			if (src_pack12_buffer_1 == NULL) {
				src_pack12_buffer_1 = (CHAR *)malloc(pack12_size);
			}
			if (src_pack12_buffer_1 == NULL) {
				printf("fail to allocate memory for image buffer!\n");
				//set raw
				ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
				if (ret < 0) {
					printf("SET ISPT_ITEM_RAW fail! \n");
				}
				goto exit;
			}

			memset(src_pack12_buffer_1, 0, pack12_size);

			// load raw file from sd card
			sprintf(src_raw_path, "/mnt/sd/src_w%d_h%d_12b_1.raw", (int)image_width, (int)image_height);
			fd = open(src_raw_path, O_RDONLY);
			if (fd == -1) {
				printf("fail to open %s \n", src_raw_path);
				goto exit;
			}

			len = read(fd, src_pack12_buffer_1, pack12_size);
			close(fd);
			if (len != (INT)(pack12_size)) {
				printf("fail to read %s \n", src_raw_path);
				goto exit;
			}

			#if SAVERAW
			#if 1
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_1.raw", image_width, image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				return -1;
			}
			fwrite(src_pack12_buffer_1, sizeof(CHAR), pack12_size, fp);
			fclose(fp);
			#endif
			#endif

			pack_image_ready = TRUE;

			break;

		case 3:
			// convert pack12 RAW to unpack12 RAW and process OB
			if (!pack_image_ready) {
				printf("pack_image_ready not ready. \n");
				break;
			}
			unpack12_size = pack12_size * 4 / 3;
			//sensor0
			if (dst_unpack12_buffer == NULL) {
				dst_unpack12_buffer = (CHAR *)malloc(unpack12_size);
			}
			if (dst_unpack12_buffer == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}

			if (convert_pack12_to_unpack12(pack12_size, src_pack12_buffer, dst_unpack12_buffer, OB) == FALSE) {
				printf("Fail to convert unpack RAW: size is not correct. (%d) \n", pack12_size);
				goto exit;
			}

			#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack_0.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp != NULL) {
				fwrite(dst_unpack12_buffer, sizeof(CHAR), unpack12_size, fp);
				fclose(fp);
			} else {
				printf("fail to open %s \n", save_raw_path);
			}
			#endif
			//sensor1
			if (dst_unpack12_buffer_1 == NULL) {
				dst_unpack12_buffer_1 = (CHAR *)malloc(unpack12_size);
			}
			if (dst_unpack12_buffer_1 == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}

			if (convert_pack12_to_unpack12(pack12_size, src_pack12_buffer_1, dst_unpack12_buffer_1, OB) == FALSE) {
				printf("Fail to convert unpack RAW: size is not correct. (%d) \n", pack12_size);
				goto exit;
			}

			#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack_1.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp != NULL) {
				fwrite(dst_unpack12_buffer_1, sizeof(CHAR), unpack12_size, fp);
				fclose(fp);
			} else {
				printf("fail to open %s \n", save_raw_path);
			}
			#endif

			unpack_image_ready = TRUE;
			break;

		case 11:
			// calibrate ECS
			if (!unpack_image_ready) {
				printf("unpack_image_ready not ready. \n");
				break;
			}

			ecs_ca.src_buffer0 = dst_unpack12_buffer;
			if (ecs_ca.out_buffer0 == NULL) {
				ecs_ca.out_buffer0 = (CHAR *)malloc(unpack12_size);
			}
			if (ecs_ca.out_buffer0 == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}
			ecs_ca.src_buffer1 = dst_unpack12_buffer_1;
			if (ecs_ca.out_buffer1 == NULL) {
				ecs_ca.out_buffer1 = (CHAR *)malloc(unpack12_size);
			}
			if (ecs_ca.out_buffer1 == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}

			ecs_ca.width = image_width;
			ecs_ca.height = image_height;
			ecs_ca.start_pix = _R;

			ecs_ca.avg_mode = AVERAGE_5X5_PIXEL;
			ecs_ca.target_lum.low_bnd = 110 * 16;    // RAW12 format
			ecs_ca.target_lum.high_bnd = 150 * 16;

			//overlap region
			stitch_param.overlap_region.top_blk = 0;
			stitch_param.overlap_region.bottom_blk = 64;
			stitch_param.overlap_region.left_blk = 0;///0 51
			stitch_param.overlap_region.right_blk = 13;///13 64

			//luma ratio
			stitch_param.comp.center_luma_ratio[0]  =  880;
			stitch_param.comp.center_luma_ratio[1]  =  880;
			stitch_param.comp.center_luma_ratio[2]  =  880;
			stitch_param.comp.center_luma_ratio[3]  =  880;
			stitch_param.comp.center_luma_ratio[4]  =  880;
			stitch_param.comp.center_luma_ratio[5]  =  880;
			stitch_param.comp.center_luma_ratio[6]  =  880;
			stitch_param.comp.center_luma_ratio[7]  =  880;
			stitch_param.comp.center_luma_ratio[8]  =  880;
			stitch_param.comp.center_luma_ratio[9]  =  880;
			stitch_param.comp.center_luma_ratio[10] =  880;
			stitch_param.comp.center_luma_ratio[11] =  880;
			stitch_param.comp.center_luma_ratio[12] =  880;
			stitch_param.comp.center_luma_ratio[13] =  880;
			stitch_param.comp.center_luma_ratio[14] =  880;
			stitch_param.comp.center_luma_ratio[15] =  883;
			stitch_param.comp.center_luma_ratio[16] =  887;
			stitch_param.comp.center_luma_ratio[17] =  894;
			stitch_param.comp.center_luma_ratio[18] =  901;
			stitch_param.comp.center_luma_ratio[19] =  908;
			stitch_param.comp.center_luma_ratio[20] =  916;
			stitch_param.comp.center_luma_ratio[21] =  924;
			stitch_param.comp.center_luma_ratio[22] =  933;
			stitch_param.comp.center_luma_ratio[23] =  942;
			stitch_param.comp.center_luma_ratio[24] =  951;
			stitch_param.comp.center_luma_ratio[25] =  960;
			stitch_param.comp.center_luma_ratio[26] =  970;
			stitch_param.comp.center_luma_ratio[27] =  980;
			stitch_param.comp.center_luma_ratio[28] =  990;
			stitch_param.comp.center_luma_ratio[29] = 1000;
			stitch_param.comp.center_luma_ratio[30] = 1000;
			stitch_param.comp.center_luma_ratio[31] = 1000;
			stitch_param.comp.center_luma_ratio[32] = 1000;
			stitch_param.comp.outer_luma_ratio[0]  =  834;
			stitch_param.comp.outer_luma_ratio[1]  =  835;
			stitch_param.comp.outer_luma_ratio[2]  =  836;
			stitch_param.comp.outer_luma_ratio[3]  =  837;
			stitch_param.comp.outer_luma_ratio[4]  =  839;
			stitch_param.comp.outer_luma_ratio[5]  =  841;
			stitch_param.comp.outer_luma_ratio[6]  =  843;
			stitch_param.comp.outer_luma_ratio[7]  =  846;
			stitch_param.comp.outer_luma_ratio[8]  =  849;
			stitch_param.comp.outer_luma_ratio[9]  =  852;
			stitch_param.comp.outer_luma_ratio[10] =  856;
			stitch_param.comp.outer_luma_ratio[11] =  860;
			stitch_param.comp.outer_luma_ratio[12] =  864;
			stitch_param.comp.outer_luma_ratio[13] =  869;
			stitch_param.comp.outer_luma_ratio[14] =  875;
			stitch_param.comp.outer_luma_ratio[15] =  881;
			stitch_param.comp.outer_luma_ratio[16] =  887;
			stitch_param.comp.outer_luma_ratio[17] =  894;
			stitch_param.comp.outer_luma_ratio[18] =  901;
			stitch_param.comp.outer_luma_ratio[19] =  908;
			stitch_param.comp.outer_luma_ratio[20] =  916;
			stitch_param.comp.outer_luma_ratio[21] =  924;
			stitch_param.comp.outer_luma_ratio[22] =  933;
			stitch_param.comp.outer_luma_ratio[23] =  942;
			stitch_param.comp.outer_luma_ratio[24] =  951;
			stitch_param.comp.outer_luma_ratio[25] =  960;
			stitch_param.comp.outer_luma_ratio[26] =  970;
			stitch_param.comp.outer_luma_ratio[27] =  980;
			stitch_param.comp.outer_luma_ratio[28] =  990;
			stitch_param.comp.outer_luma_ratio[29] = 1000;
			stitch_param.comp.outer_luma_ratio[30] = 1000;
			stitch_param.comp.outer_luma_ratio[31] = 1000;
			stitch_param.comp.outer_luma_ratio[32] = 1000;

			//usleep(40000*25);    // wait for AE setting effectively
			ret = stitch_ecs_calibrate(&ecs_ca, &stitch_param);
			if (ret == HD_OK) {
				ecs_calibrate_done = TRUE;

				printf("Stitch done = %d \r\n", ecs_calibrate_done);
				printf("tbl0 = 0x%X, 0x%X, 0x%X \r\n", ecs_ca.ecs_tbl[0][0], ecs_ca.ecs_tbl[0][1], ecs_ca.ecs_tbl[0][2]);
				printf("tbl1 = 0x%X, 0x%X, 0x%X \r\n", ecs_ca.ecs_tbl[1][0], ecs_ca.ecs_tbl[1][1], ecs_ca.ecs_tbl[1][2]);

				sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_ecs0.bin", (int)image_width, (int)image_height);
				fp = fopen(save_raw_path, "wb");
				if (fp == NULL) {
					printf("fail to open %s \n", save_raw_path);
					goto exit;
				}
					fwrite(ecs_ca.ecs_tbl[0], sizeof(CHAR), ECS_TABLE_LEN * sizeof(UINT32), fp);
				fclose(fp);

				sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_ecs1.bin", (int)image_width, (int)image_height);
				fp = fopen(save_raw_path, "wb");
				if (fp == NULL) {
					printf("fail to open %s \n", save_raw_path);
					goto exit;
				}
				fwrite(ecs_ca.ecs_tbl[1], sizeof(CHAR), ECS_TABLE_LEN * sizeof(UINT32), fp);
				fclose(fp);
			} else {
				printf("ECS CA Failed : Please make sure that the G value in the center of image falls between 180 ~ 220\n");
			}

			break;

		case 12:
			// simulate ECS
			if (!ecs_calibrate_done) {
				printf("ECS calibrate not ready. \n");
				break;
			}

			stitch_ecs_simulate(&ecs_ca);
			//#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack_ecs0.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				goto exit;
			}
			fwrite(ecs_ca.out_buffer0, sizeof(CHAR), unpack12_size, fp);
			fclose(fp);

			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack_ecs1.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				goto exit;
			}
			fwrite(ecs_ca.out_buffer1, sizeof(CHAR), unpack12_size, fp);
			fclose(fp);
			//#endif

			break;

		case 13:
			// simulate ECS
			if (!ecs_calibrate_done) {
				printf("ECS calibrate not ready. \n");
				break;
			}
			//shading 0 dtsi
			sprintf(save_raw_path, "/mnt/sd/shading0.dtsi");
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				goto exit;
			}

			size = sizeof(IQ_SHADING_PARAM);
			vendor_isp_get_iq(IQT_ITEM_VERSION, &version);

			shading_dtsi.ecs_enable = 1;
			shading_dtsi.vig_enable = 0;
			for (int i = 0; i < IQ_SHADING_ECS_LEN; i++) {
				shading_dtsi.ecs_map_tbl[i] = ecs_ca.ecs_tbl[0][i];
			}
			shading_dtsi.vig_center_x = 500;
			shading_dtsi.vig_center_y = 500;
			shading_dtsi.vig_reduce_th = 400;
			shading_dtsi.vig_zero_th = 1600;
			for (int i = 0; i< 17; i++) {
				shading_dtsi.vig_lut[i] = 0;
			}

			shading_data = (CHAR *)&shading_dtsi;
			shading_size = (CHAR *)&size;
			shading_ver = (CHAR *)&version;

			fprintf(fp, "/*\n");
			fprintf(fp, " * Novatek Ltd. NA51055 BSP part of dts\n");
			fprintf(fp, " *\n");
			fprintf(fp, " * Cortex-A9\n");
			fprintf(fp, " *\n");
			fprintf(fp, " */\n");
			fprintf(fp, "\n");
			fprintf(fp, "&iq {\n");
			fprintf(fp, "\t%s_iq_shading_0 {\n", sensor_name);
			fprintf(fp, "\t\tversion-info = [%.2x %.2x %.2x %.2x];\n", shading_ver[0], shading_ver[1], shading_ver[2], shading_ver[3]);
			fprintf(fp, "\t\tiq_shading {\n");
			fprintf(fp, "\t\t\tsize = [%.2x %.2x %.2x %.2x];\n", shading_size[0], shading_size[1], shading_size[2], shading_size[3]);
			fprintf(fp, "\t\t\tdata = [%.2x", shading_data[0]);
			for (UINT32 i = 1; i < size; i++) {
				fprintf(fp, " %.2x", shading_data[i]);
			}
			fprintf(fp, "];\n");
			fprintf(fp, "\t\t};\n");
			fprintf(fp, "\t};\n");
			fprintf(fp, "};\n");
			fclose(fp);

			//shading 1 dtsi
			sprintf(save_raw_path, "/mnt/sd/shading1.dtsi");
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				goto exit;
			}
			for (int i = 0; i < IQ_SHADING_ECS_LEN; i++) {
				shading_dtsi.ecs_map_tbl[i] = ecs_ca.ecs_tbl[1][i];
			}

			shading_data = (CHAR *)&shading_dtsi;

			fprintf(fp, "/*\n");
			fprintf(fp, " * Novatek Ltd. NA51055 BSP part of dts\n");
			fprintf(fp, " *\n");
			fprintf(fp, " * Cortex-A9\n");
			fprintf(fp, " *\n");
			fprintf(fp, " */\n");
			fprintf(fp, "\n");
			fprintf(fp, "&iq {\n");
			fprintf(fp, "\t%s_iq_shading_1 {\n", sensor_name);
			fprintf(fp, "\t\tversion-info = [%.2x %.2x %.2x %.2x];\n", shading_ver[0], shading_ver[1], shading_ver[2], shading_ver[3]);
			fprintf(fp, "\t\tiq_shading {\n");
			fprintf(fp, "\t\t\tsize = [%.2x %.2x %.2x %.2x];\n", shading_size[0], shading_size[1], shading_size[2], shading_size[3]);
			fprintf(fp, "\t\t\tdata = [%.2x", shading_data[0]);
			for (UINT32 i = 1; i < size; i++) {
				fprintf(fp, " %.2x", shading_data[i]);
			}
			fprintf(fp, "];\n");
			fprintf(fp, "\t\t};\n");
			fprintf(fp, "\t};\n");
			fprintf(fp, "};\n");
			fclose(fp);

			break;

		case 50:
			auto_ruma_ratio.frame_num = 2;
			auto_ruma_ratio.image_full_size_w = 1440;
			auto_ruma_ratio.image_full_size_h = 1440;
			auto_ruma_ratio.lut2d_scl_out_w[0] = 1440;
			auto_ruma_ratio.lut2d_scl_out_w[1] = 1440;
			auto_ruma_ratio.lut2d_scl_out_h[0] = 1440;
			auto_ruma_ratio.lut2d_scl_out_h[1] = 1440;
			auto_ruma_ratio.blend_width[0] = 300;
			auto_ruma_ratio.blend_width[1] = 0;
			auto_ruma_ratio.blend_height[0] = 1440;
			auto_ruma_ratio.blend_height[1] = 1440;
			auto_ruma_ratio.vpe_2dlut_size = 257;
			if (auto_ruma_ratio.vpe_2dlut_size == 65) {
				lut_size = 68 * 65 * 4;
			} else if (auto_ruma_ratio.vpe_2dlut_size == 129) {
				lut_size = 132 * 129 * 4;
			} else {//VPE_ISP_2DLUT_SZ_257X257
				lut_size = 260 * 257 * 4;
			}

			for (UINT32 i = 0; i < auto_ruma_ratio.frame_num; i++) {

				lut_path = lut_bin[i];
				if ((fp = fopen(lut_path, "rb")) == NULL) {
					printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", lut_path);
					return -1;
				}
				setvbuf(fp, NULL, _IONBF, 0);

				//printf("%lx")

				read_len = fread((void *)(lut2d[i].lut2d.lut), 1, lut_size, fp);
				if (read_len != lut_size) {
					printf("reading 2dlut error\n");
				}
				// close input image file
				fclose(fp);
			}

			for (UINT32 i = 0; i < auto_ruma_ratio.frame_num; i++) {
				auto_ruma_ratio.lut2d_tbl_addr[i] = lut2d[i].lut2d.lut;
			}
			//auto_ruma_ratio.ratio_ub = 281;//1.1x
			//auto_ruma_ratio.ratio_ub = 307;//1.2x
			//auto_ruma_ratio.ratio_ub = 332;//1.3x
			auto_ruma_ratio.ratio_ub = 294;//1.15x

			while (1){
				la_data.id = 0;
				ret = vendor_isp_get_common(ISPT_ITEM_LA_DATA, &la_data);

				if (ret == HD_OK) {
					//printf("id = %d, LA = %d \n", la_data.id, la_data.la_rslt.lum_1[0]);
					memcpy((void *)auto_ruma_ratio.la[0],(void *)la_data.la_rslt.lum_1, sizeof(UINT16)*1024);
				} else {
					printf("Set ISPT_ITEM_LA_DATA fail \n");
				}

				la_data.id = 1;
				ret = vendor_isp_get_common(ISPT_ITEM_LA_DATA, &la_data);

				if (ret == HD_OK) {
					//printf("id = %d, LA = %d \n", la_data.id, la_data.la_rslt.lum_1[0]);
					memcpy((void *)auto_ruma_ratio.la[1],(void *)la_data.la_rslt.lum_1, sizeof(UINT16)*1024);
				} else {
					printf("Set ISPT_ITEM_LA_DATA fail \n");
				}

				ret = stitch_auto_luma_ratio_proces(&auto_ruma_ratio);

				shading.id = 0;
				vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
				shading.shading.vig_enable = 1;
				for (UINT32 i = 0; i < 17; i++) {
					shading.shading.vig_lut[i] = auto_ruma_ratio.luma_ratio[0] * 4 - 1024;
				}
				vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);

				shading.id = 3;
				vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
				shading.shading.vig_enable = 1;
				for (UINT32 i = 0; i < 17; i++) {
					shading.shading.vig_lut[i] = auto_ruma_ratio.luma_ratio[1] * 4 - 1024;
				}
				vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);

				usleep(50000);
			}
			break;

		case 51:
			shading.id = 0;
			vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
			shading.shading.vig_enable = 1;
			for (UINT32 i = 0; i < 17; i++) {
				shading.shading.vig_lut[i] = 0;
			}
			vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);

			shading.id = 1;
			vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
			shading.shading.vig_enable = 1;
			for (UINT32 i = 0; i < 17; i++) {
				shading.shading.vig_lut[i] = 0;
			}
			vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);

			break;

		case 98:
			// get stitch libary version
			printf("%s \r\n", isp_lib_get_stitch_ver());

			break;

		case 99:
			// disable ecs_enable
			printf("debug en(0/1)\n");
			do {
				printf(">> ");
				dbg_en = get_choose_int();
			} while (0);
			stitch_set_dbg_out(dbg_en);

			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

exit:
	if (src_pack12_buffer != NULL) {
		free(src_pack12_buffer);
	}

	if (src_pack12_buffer_1 != NULL) {
		free(src_pack12_buffer_1);
	}

	if (dst_unpack12_buffer != NULL) {
		free(dst_unpack12_buffer);
	}

	if (dst_unpack12_buffer_1 != NULL) {
		free(dst_unpack12_buffer_1);
	}

	if (ecs_ca.out_buffer0 != NULL) {
		free(ecs_ca.out_buffer0);
	}

	if (ecs_ca.out_buffer1 != NULL) {
		free(ecs_ca.out_buffer1);
	}

	ret = vendor_isp_uninit();
	if (ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}

	return 0;
}

