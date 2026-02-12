#ifndef RTSP_MD5_H
#define RTSP_MD5_H

typedef unsigned int u32;
typedef unsigned char u8;

struct RTSP_MD5Context {
	u32 buf[4];
	u32 bits[2];
	u8 in[64];
};

void RTSP_MD5Init(struct RTSP_MD5Context *context);
void RTSP_MD5Update(struct RTSP_MD5Context *ctx, unsigned char const *buf, unsigned len);
void RTSP_MD5Final(unsigned char digest[16], struct RTSP_MD5Context *context);
void RTSP_MD5Transform(u32 buf[4], u32 const in[16]);

typedef struct RTSP_MD5Context RTSP_MD5_CTX;

#endif /* RTSP_MD5_H */
