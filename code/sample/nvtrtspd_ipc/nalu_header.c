#include "nalu_header.h"

static int init_bitstream(struct stream_t *stream, unsigned char *buffer, int length)
{
	stream->streamBuffer = buffer;
	stream->totalLength = length;
	stream->byteOffset = 0;
	return 0;
}

static unsigned int show_bytes(struct stream_t *stream, unsigned int byte_num)
{
	unsigned int i, value = 0;
	if ((int)(stream->byteOffset + byte_num) > stream->totalLength)
		byte_num = stream->totalLength - stream->byteOffset + byte_num;
	for (i = 0; i < byte_num; i++)
		value = (value << 8) | stream->streamBuffer[stream->byteOffset + i];
	return value;
}

static unsigned int read_bytes(struct stream_t *stream, int byte_num)
{
	unsigned int value;
	value = show_bytes(stream, byte_num);
	stream->byteOffset += byte_num;
	return value;
}

static int find_sei_nalu(int enc_type,struct stream_t *stream)
{
	unsigned int start_code;
	int nal_type = 0;
	int ret = 0;

	if(enc_type == 1) //h264
	{
		do {
			start_code = show_bytes(stream, 4);
			while (start_code != 0x00000001 && (start_code >> 8) != 0x000001) {
				if((int)stream->byteOffset >= stream->totalLength) {
					// can not find start code
					ret = -1;
					goto exit_find;
				}
				read_bytes(stream, 1);
				start_code = show_bytes(stream, 4);
			}
			start_code = read_bytes(stream, 4);

			if (start_code == 0x00000001) {
				start_code = read_bytes(stream, 1);
				nal_type = start_code&0x1F;
			}
			else
				nal_type = start_code&0x1F;
		}while (nal_type != 6);
	}
	else //H265
	{
		do{
			start_code = show_bytes(stream, 4);
            while (start_code != 0x00000001 && (start_code >> 8) != 0x000001) {
                if((int)stream->byteOffset >= stream->totalLength) {
                    // can not find start code
                    ret = -1;
                    goto exit_find;
                }
                read_bytes(stream, 1);
                start_code = show_bytes(stream, 4);
            }
            start_code = read_bytes(stream, 4);
			if(start_code == 0x00000001)
				start_code = read_bytes(stream, 1);
			nal_type = ((start_code >> 1) & 0x3F);
			start_code = read_bytes(stream, 1);
		} while (nal_type != 39 && nal_type != 40);
	}
exit_find:
	return ret;
}

/*************************************
 * Contain user data
 *
 * Input parameter:
 *  out_stream: pointer of output bitstream
 *  data:       pointer of input user data
 *  length:     length of user data
 * 
 * Return:
 *  length of output bitstream
 *************************************/
int gen_nalu_sei(int enc_type,unsigned char *out_stream, unsigned char *data, int length)
{
	int byte_pos = 0;
	int tmp = length;
	int i;
	int counter = 0;
	//printf("enc_type = %d\n",enc_type);
	if(enc_type == 1) //for H264
	{
		// 1. encode start code & SEI header
		out_stream[byte_pos++] = 0x00;
		out_stream[byte_pos++] = 0x00;
		out_stream[byte_pos++] = 0x00;
		out_stream[byte_pos++] = 0x01;
		out_stream[byte_pos++] = 0x06;// type SEI
		// 2. encode sei type
		out_stream[byte_pos++] = 0x40;
	}
	else //for H265
	{
		// 1. encode start code & SEI header
		out_stream[byte_pos++] = 0x00;
		out_stream[byte_pos++] = 0x00;
		out_stream[byte_pos++] = 0x00;
		out_stream[byte_pos++] = 0x01;
		out_stream[byte_pos++] = 0x4E;   // type SEI
		out_stream[byte_pos++] = 0x00;   // type SEI
		// 2. encode sei type
		out_stream[byte_pos++] = 0x96;
	}
	// 3. encode sei length
	while (tmp > 255) {
		out_stream[byte_pos++] = 0xFF;
		tmp -= 255;
	}
	out_stream[byte_pos++] = tmp;
	// 4. encode sei data
	for (i = 0; i < length; i++) {
		//if (0 == out_stream[byte_pos-2] && 0 == out_stream[byte_pos-2]
		if (2 == counter && !(data[i] & 0xFC)) {
			out_stream[byte_pos++] = 0x03;
			counter = 0;
		}
		out_stream[byte_pos++] = data[i];
		if (0x00 == data[i])
			counter++;
		else
			counter = 0;
	}
	out_stream[byte_pos++] = 0x80;
	return byte_pos;
}

/*************************************
 * Extract user data
 *
 * Input parameter:
 *  user_data:  pointer of output user data
 *  bit_stream: pointer of input bitstream
 *  bs_length:  length of input bitstream
 * 
 * Return:
 *  length of user data
 *************************************/
int parse_nalu_sei(int enc_type,unsigned char *user_data, unsigned char *bit_stream, int bs_length)
{
	struct stream_t stream;
	int i, tmp;
	int sei_type = 0;
	int sei_length = 0;
	int counter = 0;

	// 0. init stream
	init_bitstream(&stream, bit_stream, bs_length);

	// 1. check start code & SEI header
	if (find_sei_nalu(enc_type,&stream) < 0)
		goto exit_parse;

	// 2. get sei type
	tmp = read_bytes(&stream, 1);
	while (0xFF == tmp) {
		sei_type += 0xFF;
		tmp = read_bytes(&stream, 1);
	}
	sei_type += tmp;

	// 3. encode sei length
	tmp = read_bytes(&stream, 1);
	while (0xFF == tmp) {
		sei_length += 0xFF;
		tmp = read_bytes(&stream, 1);
	}
	sei_length += tmp;

	// 4. encode sei data
	for (i = 0; i < sei_length; i++) {
		tmp = read_bytes(&stream, 1);
		if (2 == counter && 3 == tmp) {
			tmp = read_bytes(&stream, 1);
			counter = 0;
		}

		user_data[i] = tmp;

		if (0x00 == tmp)
			counter++;
		else
			counter = 0;
	}
exit_parse:
	return sei_length;
}

