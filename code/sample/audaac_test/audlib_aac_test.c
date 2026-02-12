#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "audlib_aac.h"

#include <linux/input.h>


static int uiAudAecPlyBufAdr,uiAudAecRecBufAdr,uiAudAecOutBufAdr,test_file_size = 0x200000;

int testcode_read_file(CHAR* file_name, CHAR* addr)
{
	FILE *fp;
	char *buf;
	int fsize;


	// load dtb to memory
	fp = fopen(file_name, "rb");
	if (fp == NULL) {
		printf("failed to open %s\n", file_name);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);	
	fseek(fp, 0, SEEK_SET);
	buf = addr;
	
	fread(buf, fsize, 1, fp);
	fclose(fp);

    return fsize;
}

void testcode_write_file(CHAR* file_name, CHAR* addr,UINT32 file_size)
{
	FILE *fp;
	char *buf;
	buf = addr;

	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		printf("failed to open %s\n", file_name);
		return;
	}

	printf("write addr =  %x\n", buf);

	fwrite(buf, file_size, 1, fp);
	fclose(fp);
	return;
}

void _run_aac_test(void)
{

    int                     file_size;
    CHAR                    aac_input[20]        = "/mnt/sd/aac_in.PCM";
    CHAR                    aac_output[20]       = "/mnt/sd/aac_out.PCM";
    CHAR                    aac_bitstream[20]    = "/mnt/sd/aac_bs.PCM";
    UINT32                  buf_in,buf_out,buf_bsout;
    int                     channel              = 2;
    int                     aac_samplerate       = 48000;
    int                     BitRate              = 192000;
    int                     header_enable        = 0;
    int                     encode_stop_freq     = 20000;
	
	AUDLIB_AAC_CFG          aac_encode_cfg;
	AUDLIB_AAC_BUFINFO      aac_encode_buf_info;
    AUDLIB_AACE_RTNINFO     aac_encode_rtn_size;

	AUDLIB_AAC_CFG          aac_decode_cfg;
	AUDLIB_AAC_BUFINFO      aac_decode_buf_info;
	AUDLIB_AACD_RTNINFO     aac_decode_rtn;
    int                     Ret;
    UINT32                  i = 0, Proc = 0, EncodeSize = 0,DecodeSize=0;

    printf("In run_aac_test \r\n");
	
	uiAudAecRecBufAdr = (int)malloc(test_file_size*5);
	uiAudAecPlyBufAdr = uiAudAecRecBufAdr + test_file_size;
	uiAudAecOutBufAdr = uiAudAecRecBufAdr + test_file_size*2;
	
    file_size = testcode_read_file(aac_input, (CHAR *) uiAudAecRecBufAdr);

    buf_in   = uiAudAecRecBufAdr;
    buf_out  = uiAudAecOutBufAdr;
    buf_bsout= uiAudAecPlyBufAdr;


/*
AAC Encode
*/
    aac_encode_cfg.sample_rate      = aac_samplerate;
    aac_encode_cfg.channel_number   = channel;
    aac_encode_cfg.encode_bit_rate  = BitRate;
    aac_encode_cfg.header_enable    = header_enable;
    aac_encode_cfg.encode_stop_freq = encode_stop_freq;
    Ret = audlib_aac_encode_init(&aac_encode_cfg);
    if (Ret != 0) {
        printf("aac_encode_init ErrCode = %d\r\n", Ret);
	}

	Proc = file_size /(2 * channel); //aac needs sample number
	while (Proc >= 1024) {

		aac_encode_buf_info.bitstram_buffer_length       = 1024 * channel;
		aac_encode_buf_info.bitstram_buffer_in  = (int)(buf_in + (1024 * i * 2 * channel));
		aac_encode_buf_info.bitstram_buffer_out = (int)(buf_bsout + EncodeSize);
		Ret = audlib_aac_encode_one_frame(&aac_encode_buf_info, &aac_encode_rtn_size);
		if (Ret != 0) {
			printf("aac_encode_one_frame ErrCode = %d\r\n", Ret);
		}

		EncodeSize += aac_encode_rtn_size.output_size;
		Proc -= 1024;
		i++;
	}



    testcode_write_file(aac_bitstream, (CHAR *) buf_bsout, EncodeSize);

/*
AAC decode
*/

	aac_decode_cfg.sample_rate = aac_samplerate;
	aac_decode_cfg.channel_number = channel;
	aac_decode_cfg.header_enable = header_enable;
	Ret = audlib_aac_decode_init(&aac_decode_cfg);
	if (Ret != 0) {
	    printf("aac_decode_init ErrCode = %d\r\n", Ret);
	}


	Proc = 0;
	while (Proc < EncodeSize) {
	    aac_decode_buf_info.bitstram_buffer_length    = EncodeSize - Proc;
		aac_decode_buf_info.bitstram_buffer_in = (int)(buf_bsout + Proc);
		aac_decode_buf_info.bitstram_buffer_out = (int)(buf_out + DecodeSize);
		Ret = audlib_aac_decode_one_frame(&aac_decode_buf_info, &aac_decode_rtn);
		if (Ret != 0) {
		    printf("aac_decode_one_frame ErrCode = %d\r\n", Ret);
		}

		DecodeSize += (aac_decode_rtn.output_size * aac_decode_rtn.channel_number * 2);
		Proc += aac_decode_rtn.one_frame_consume_bytes;
	}

    testcode_write_file(aac_output,    (CHAR *) buf_out, DecodeSize);

    printf("EncodeSize = [%d] DecodeSize = [%d]\r\n",EncodeSize,DecodeSize);

    printf("Out run_aac_test \r\n");


}
int main(void)
{

	printf("hellow Audio lib AAC test program\n");
	_run_aac_test();

	return 0;
}