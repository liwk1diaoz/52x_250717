
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "shading_lib.h"
#include "vendor_isp.h"

#define SAVERAW 1

static ISPT_MEMORY_INFO mem_info;
static ECS_CA ecs_ca;
static VIG_CA vig_ca;
static IQT_SHADING_PARAM shading;

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
	CHAR b1, b2, b3, *ptrSrc;
	UINT16 *ptrDst;
	UINT16 v1, v2;

	if((frame_size % 3) != 0) {
		return FALSE;
	}

	count = frame_size / 3;
	ptrSrc = srcbuffer;
	ptrDst = (UINT16 *)dstbuffer;
	for (i = 0; i < count; i++) {
		b1 = ptrSrc[0];
		b2 = ptrSrc[1];
		b3 = ptrSrc[2];
		v1 = ((b2 & 0xF) << 8) | b1;
		v2 = (b3 << 4) | ((b2 >> 4) & 0xF);

		ptrDst[0] = v1 - ob;
		ptrDst[1] = v2 - ob;

		ptrSrc += 3;
		ptrDst += 2;
	}

	return TRUE;
}


int main(int argc, char *argv[])
{
	INT32 option;
	UINT32 trig = 1;
	INT fd, len;
	HD_RESULT ret;
	UINT32 id = 0;
	ISPT_RAW_INFO raw_info = {0};
	CHAR *src_pack12_buffer = NULL, *dst_unpack12_buffer = NULL;
	UINT32 image_width = 1920, image_height = 1080;
	UINT32 pack12_size = 1920 * 1080 * 3 / 2, unpack12_size = 1920 * 1080 * 2;
	UINT32 data_len, residue_len, data, j, block;
	UINT32 ob_value;
	UINT32 dbg_type;
	BOOL pack_image_ready = FALSE, unpack_image_ready = FALSE, ecs_calibrate_done = FALSE;
	BOOL ecs_enable;
	#if SAVERAW
	CHAR src_raw_path[100], save_raw_path[100];
	FILE *fp = NULL;
	#endif

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf("  1. Get RAW from real board \n");
			printf("  2. Get RAW from mnt/sd \n");
			printf("  3. Pre-process RAW, pack -> unpack -> apply OB \n");
			printf("  4. Run ECS calibrate \n");
			printf("  5. Run ECS simulate \n");
			printf("  6. Run VIG calibrate/simulate \n");
			printf(" 99. Set dbg mode \n");

			printf("  0. Quit\n");
			printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {

		case 1:
			// disable ecs_enable
			do {
				printf("Set ISP id >> \n");
				id = (UINT32)get_choose_int();
			} while (0);
			shading.id = id;
			vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
			ecs_enable = shading.shading.ecs_enable;
			shading.shading.ecs_enable = 0;
			vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);
			
			// get raw from real board
			raw_info.id = id;
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

			printf("get raw, id = %d, %d x %d, %x\n", raw_info.id, raw_info.raw_info.pw, raw_info.raw_info.ph, raw_info.raw_info.pxlfmt);

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
			#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", src_raw_path);
				goto exit;
			}
			#endif

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
					#if SAVERAW
					fclose(fp);
					#endif
					goto exit;
				}

				memcpy(&src_pack12_buffer[data], mem_info.buf, data_len);
				#if SAVERAW
				fwrite(src_pack12_buffer+data, sizeof(CHAR), data_len, fp);
				#endif
				mem_info.addr += 32*1024;
				residue_len -= data_len;
				data += data_len;
				data_len = (residue_len >= 32 * 1024) ? 32 * 1024 : residue_len;
			}

			pack_image_ready = TRUE;

			ret = vendor_isp_set_common(ISPT_ITEM_RAW, &raw_info.id);
			if (ret < 0) {
				printf("SET ISPT_ITEM_RAW fail! \n");
				#if SAVERAW
				fclose(fp);
				#endif
				goto exit;
			}

			// restore ecs_enable
			shading.shading.ecs_enable = ecs_enable;
			vendor_isp_set_iq(IQT_ITEM_SHADING_PARAM, &shading);
			#if SAVERAW
			fclose(fp);
			#endif
			break;

		case 2:
			// Get RAW from mnt/sd
			do {
				printf("Set image width >> \n");
				image_width = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set image height >> \n");
				image_height = (UINT32)get_choose_int();
			} while (0);

			pack12_size = image_width * image_height * 3 / 2;

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
			sprintf(src_raw_path, "/mnt/sd/src_w%d_h%d_12b.raw", (int)image_width, (int)image_height);
			fd = open(src_raw_path, O_RDONLY);
			if (fd == -1){
				printf("fail to open %s \n", src_raw_path);
				goto exit;
			}

			len = read(fd, src_pack12_buffer, pack12_size);
			close(fd);
			if (len != (INT)(pack12_size)){
				printf("fail to read %s \n", src_raw_path);
				goto exit;
			}

			printf("get %s OK! \n", src_raw_path);

			#if SAVERAW
			#if 0
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b.raw", image_width, image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				return -1;
			}
			fwrite(src_pack12_buffer, sizeof(CHAR), pack12_size, fp);
			fclose(fp);
			#endif
			#endif

			pack_image_ready = TRUE;

			break;

		case 3:
			// convert pack12 RAW to unpack12 RAW and process OB
			do {
				printf("Set OB value >> \n");
				ob_value = (UINT32)get_choose_int();
			} while (0);

			if (!pack_image_ready) {
				printf("pack_image_ready not ready. \n");
				break;
			}
			unpack12_size = pack12_size * 4 / 3;
			if (dst_unpack12_buffer == NULL) {
				dst_unpack12_buffer = (CHAR *)malloc(unpack12_size);
			}
			if (dst_unpack12_buffer == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}

			if (convert_pack12_to_unpack12(pack12_size, src_pack12_buffer, dst_unpack12_buffer, ob_value) == FALSE) {
				printf("Fail to convert unpack RAW: size is not correct. (%d) \n", pack12_size);
				goto exit;
			}

			#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp != NULL) {
				fwrite(dst_unpack12_buffer, sizeof(CHAR), unpack12_size, fp);
				fclose(fp);
			} else {
				printf("fail to open %s \n", save_raw_path);
			}
			#endif

			printf("Unpack RAW with OB (%d) is ready ! \n", ob_value);
			unpack_image_ready = TRUE;
			break;

		case 4:
			// calibrate ECS
			if (!unpack_image_ready) {
				printf("unpack_image_ready not ready. \n");
				break;
			}

			ecs_ca.src_buffer = dst_unpack12_buffer;
			if (ecs_ca.out_buffer == NULL) {
				ecs_ca.out_buffer = (CHAR *)malloc(unpack12_size);
			}
			if (ecs_ca.out_buffer == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}
			ecs_ca.width = image_width;
			ecs_ca.height = image_height;
			ecs_ca.start_pix = _R;

			ret = shading_ecs_calibrate(&ecs_ca);
			if (ret == HD_OK) {
				ecs_calibrate_done = TRUE;

				printf("tbl = 0x%X, 0x%X, 0x%X \n", ecs_ca.shad_tbl[0], ecs_ca.shad_tbl[1], ecs_ca.shad_tbl[2]);

				sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_ecs.bin", (int)image_width, (int)image_height);
				fp = fopen(save_raw_path, "wb");
				if (fp == NULL) {
					printf("fail to open %s \n", save_raw_path);
					goto exit;
				}
				fwrite(ecs_ca.shad_tbl, sizeof(CHAR), ECS_TABLE_LEN * sizeof(UINT32), fp);
				fclose(fp);
			} else {
				printf("ECS CA Failed : Please make sure that the G value in the center of image falls between 180 ~ 220\n");
			}

			break;

		case 5:
			// simulate ECS
			if (!ecs_calibrate_done) {
				printf("ECS calibrate not ready. \n");
				break;
			}

			shading_ecs_simulate(&ecs_ca);
			#if SAVERAW
			sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack_ecs.raw", (int)image_width, (int)image_height);
			fp = fopen(save_raw_path, "wb");
			if (fp == NULL) {
				printf("fail to open %s \n", save_raw_path);
				goto exit;
			}
			fwrite(ecs_ca.out_buffer, sizeof(CHAR), unpack12_size, fp);
			fclose(fp);
			#endif

			break;

		case 6:
			do {
				printf("Set VIG calibration weight >> \n");
				vig_ca.vig_weight = (UINT32)get_choose_int();;
			} while (0);

			// convert pack12 RAW to unpack12 RAW and process OB
			if (ecs_calibrate_done) {
				vig_ca.src_buffer = ecs_ca.out_buffer;
			} else {
				printf("ECS calibrate not ready. Color shading will be skipped ! \n");
				if (!unpack_image_ready) {
					printf("unpack_image_ready not ready. \n");
					break;
				}
				vig_ca.src_buffer = dst_unpack12_buffer;
			}

			if (vig_ca.out_buffer == NULL) {
				vig_ca.out_buffer = (CHAR *)malloc(unpack12_size);
			}
			if (vig_ca.out_buffer == NULL) {
				printf("fail to allocate memory for unpack image buffer!\n");
				goto exit;
			}
			vig_ca.width = image_width;
			vig_ca.height = image_height;
			vig_ca.start_pixel = _R;
			
			//Customization
			vig_ca.vig_x = 0;
			vig_ca.vig_y = 0;

			ret = shading_vig_calibrate(&vig_ca);
			if(ret == HD_OK){
				printf("VIG Calibration:\n");
				printf("vig_center_x = %d\n", vig_ca.vig_center_x);
				printf("vig_center_y = %d\n", vig_ca.vig_center_y);
				printf("vig_lut = %d", vig_ca.vig_lut[0]);
				for (int i = 1; i < vig_ca.vig_tap; i++) {
					printf(", %d", vig_ca.vig_lut[i]);
				}
				printf("\n");

				#if SAVERAW
				sprintf(save_raw_path, "/mnt/sd/save_w%d_h%d_12b_unpack_vig.raw", (int)image_width, (int)image_height);
				fp = fopen(save_raw_path, "wb");
				if (fp == NULL) {
					printf("fail to open %s \n", save_raw_path);
					goto exit;
				}
				fwrite(vig_ca.out_buffer, sizeof(CHAR), unpack12_size, fp);
				fclose(fp);
				#endif
			} else {
				printf("VIG CA Failed : Please make sure that the G value in the center of image falls between 180 ~ 220\n");
			}
			break;

		case 99:
			do {
				printf("Set dbg mode >> \n");
				printf("%4d >> SHADING_DBG_TYPE_NULL \n", SHADING_DBG_TYPE_NULL);
				printf("%4d >> SHADING_DBG_TYPE_ECS \n", SHADING_DBG_TYPE_ECS);
				printf("%4d >> SHADING_DBG_TYPE_VIG \n", SHADING_DBG_TYPE_VIG);
				printf("---------------------------------------- \r\n");
				dbg_type = (UINT32)get_choose_int();
				shading_set_dbg_out(dbg_type);
			} while (0);
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

	if (dst_unpack12_buffer != NULL) {
		free(dst_unpack12_buffer);
	}

	if (ecs_ca.out_buffer != NULL) {
		free(ecs_ca.out_buffer);
	}
	if (vig_ca.out_buffer != NULL) {
		free(vig_ca.out_buffer);
	}

	ret = vendor_isp_uninit();
	if (ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}

	return 0;
}

