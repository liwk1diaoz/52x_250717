/*******************************************************************************
* Copyright  Faraday Technology Corporation 2010-2011.  All rights reserved.   *
*------------------------------------------------------------------------------*
* Name: sdp_encode.c                                                            *
* Description:                                                                 *
*     1: ......                                                                *
* Author: giggs                                                                *
*******************************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int Base64_encode(unsigned char *source, int  sourcelen, char *target, int  targetlen);

void sdp_parameter_encoder_h264(unsigned char *data, int len, char *sdp, int sdp_len_max)
{
	char start_code[4] = {0x00, 0x00, 0x00, 0x01};
	int offset;
	int sps_start = 0, sps_end = 0, pps_start = 0, pps_end = 0;
	int sps_parsing = 0, pps_parsing = 0;
	char sps_base64[128];
	char pps_base64[64];

	for (offset = 0; offset < len - sizeof(start_code); offset++) {
		if (sps_start && sps_end && pps_start && pps_end) {
			break;
		}

		if (memcmp((void *)(&data[offset]), (void *)start_code, sizeof(start_code)) == 0) {
			if (sps_parsing) {  // sps end
				sps_parsing = 0;
				sps_end = offset - 1;
			}

			if (pps_parsing) {  // pps end
				pps_parsing = 0;
				pps_end = offset - 1;
			}

			if ((data[offset + 4] & 0x1F) == 7) { // find sps
				sps_parsing = 1;
				sps_start = offset + 4;
			}

			if ((data[offset + 4] & 0x1F) == 8) { // find pps
				pps_parsing = 1;
				pps_start = offset + 4;
			}
		}
	}

	if (sps_start && sps_end && sps_end > sps_start &&
		pps_start && pps_end && pps_end > pps_start) {
		Base64_encode(&data[sps_start], sps_end - sps_start + 1, sps_base64, 128);
		Base64_encode(&data[pps_start], pps_end - pps_start + 1, pps_base64, 64);
		if ((strlen(sps_base64) + strlen(pps_base64) + 1) < sdp_len_max) {
			memset((void *)sdp, 0, sdp_len_max);
			sprintf(sdp, "%s,%s", sps_base64, pps_base64);

		} else {
			printf("%s error, sdp len > %d\n", __FUNCTION__, sdp_len_max);
		}
	} else {
		printf("%s parse error\n", __FUNCTION__);
	}
}
void sdp_parameter_encoder_h265(unsigned char *data, int len, char *sdp, int sdp_len_max)
{
	char start_code[4] = {0x00, 0x00, 0x00, 0x01};
	int offset;
	int vps_start = 0, vps_end = 0, sps_start = 0, sps_end = 0, pps_start = 0, pps_end = 0;
	int vps_parsing = 0, sps_parsing = 0, pps_parsing = 0;
	char vps_base64[128];
	char sps_base64[128];
	char pps_base64[64];

	for (offset = 0; offset < len - sizeof(start_code); offset++) {
		if (vps_start && vps_end && sps_start && sps_end && pps_start && pps_end) {
			break;
		}

		if (memcmp((void *)(&data[offset]), (void *)start_code, sizeof(start_code)) == 0) {
			if (vps_parsing) {   // sps end
				vps_parsing = 0;
				vps_end = offset - 1;
			}
			if (sps_parsing) {  // sps end
				sps_parsing = 0;
				sps_end = offset - 1;
			}

			if (pps_parsing) {  // pps end
				pps_parsing = 0;
				pps_end = offset - 1;
			}

			if (((data[offset + 4] >> 1) & 0x3F) == 32) { // find vps
				vps_parsing = 1;
				vps_start = offset + 4;
			}

			if (((data[offset + 4] >> 1) & 0x3F) == 33) { // find sps
				sps_parsing = 1;
				sps_start = offset + 4;
			}

			if (((data[offset + 4] >> 1) & 0x3F) == 34) { // find pps
				pps_parsing = 1;
				pps_start = offset + 4;
			}
		}
	}

	if (vps_start && vps_end && vps_end > vps_start && sps_start && sps_end && sps_end > sps_start &&
		pps_start && pps_end && pps_end > pps_start) {
		Base64_encode(&data[vps_start], vps_end - vps_start + 1, vps_base64, 128);
		Base64_encode(&data[sps_start], sps_end - sps_start + 1, sps_base64, 128);
		Base64_encode(&data[pps_start], pps_end - pps_start + 1, pps_base64, 64);
		if ((strlen(vps_base64) + strlen(sps_base64) + strlen(pps_base64) + 1) < sdp_len_max) {
			memset((void *)sdp, 0, sdp_len_max);
			sprintf(sdp, "level-id=%d;sprop-vps=%s;sprop-sps=%s;sprop-pps=%s",
					data[vps_end - 2], vps_base64, sps_base64, pps_base64);

		} else {
			printf("%s error, sdp len %d+%d+%d+1 >= %d\n", __FUNCTION__,
				   strlen(vps_base64), strlen(sps_base64), strlen(pps_base64),
				   sdp_len_max);
		}
	} else {
		printf("%s parse error\n", __FUNCTION__);
	}
}
void sdp_parameter_encoder_mpeg4(unsigned char *data, int len, char *sdp, int sdp_len_max)
{
	char start_code[4] = {0x00, 0x00, 0x01, 0xB6};
	int offset, i;
	char vos_start_code[10] = { 0x00, 0x00, 0x01, 0xB0, 0x01, 0x00, 0x00, 0x01, 0xB5, 0x09};
	char *o = (char *)sdp;

	for (offset = 0; offset < len - sizeof(start_code); offset++) {
		if (memcmp((void *)(&data[offset]), (void *)start_code, sizeof(start_code)) == 0) {
			break;
		}
	}

	if (offset < len - sizeof(start_code)) {
		if (offset + 10 < sdp_len_max) {
			memset((void *)sdp, 0, sdp_len_max);

			for (i = 0; i < 10; ++i) {
				o += sprintf(o, "%02X", vos_start_code[i]);
			}

			for (i = 0; i < offset; ++i) {
				o += sprintf(o, "%02X", data[i]);
			}

			//sdp[19] = 0x86;
		} else {
			printf("%s error, sdp len > %d\n", __FUNCTION__, sdp_len_max);
		}
	} else {
		printf("%s parse error\n", __FUNCTION__);
	}
}

void sdp_parameter_encoder_mjpeg(unsigned char *data, int len, char *sdp, int sdp_len_max)
{
	char start_code[2] = {0xFF, 0xC0};
	int offset;
	int width, height;

	for (offset = 0; offset < len - sizeof(start_code); offset++) {
		if (memcmp((void *)(&data[offset]), (void *)start_code, sizeof(start_code)) == 0) {
			break;    // Find SOF
		}
	}

	if (offset < len - 8 && data[offset + 4] == 0x08) {
		memset((void *)sdp, 0, sdp_len_max);
		height = (data[offset + 5] << 8) + data[offset + 6];
		width = (data[offset + 7] << 8) + data[offset + 8];
		sprintf((char *)sdp, "%02X%02X19", width >> 3, height >> 3);
	} else {
		printf("%s parse error\n", __FUNCTION__);
	}
}

void sdp_parameter_encoder_aac(unsigned char *data, int len, char *sdp, int sdp_len_max)
{
	int object_type, ch_num, sample_rate, config;

	/*frame_len = ((data[3] & 0x03) << 11) +
	            ((data[4] & 0xFF) << 3) +
	            ((data[5] & 0xE0) >> 5);*/

	object_type = ((data[2] & 0xC0) >> 6) + 1;

	sample_rate = ((data[2] & 0x3C) >> 2);

	ch_num = ((data[2] & 0x01) << 2) +
			 ((data[3] & 0xC0) >> 6);

	config = (object_type << 11) + (sample_rate << 7) + (ch_num << 3);

	sprintf((char *)sdp, "%X", config);
}
