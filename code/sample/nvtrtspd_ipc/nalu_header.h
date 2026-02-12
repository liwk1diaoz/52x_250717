#ifndef _NALU_HEADER_H_
#define _NALU_HEADER_H_

struct stream_t {
	unsigned char *streamBuffer;
	int totalLength;
	unsigned int byteOffset;
};

extern int gen_nalu_sei(int enc_type,unsigned char *out_stream, unsigned char *data, int length);
extern int parse_nalu_sei(int enc_type,unsigned char *user_data, unsigned char *bit_stream, int bs_length);

#endif
