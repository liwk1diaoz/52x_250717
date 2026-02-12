#include "PatternFrm.h"

#if defined(_WIN32)
#define CFG_FILE_DIR "E:\\Patterns\\Live555\\"
#else
#define CFG_FILE_DIR "/home/ubuntu/patterns/Live555/"
#endif
#define MAKE_BUF_DESC(x) {x, sizeof(x)}

//Stream0: H264 + PCM
static u_int8_t Strm0_SPS[] = { 0x00,0x00,0x00,0x01,0x67,0x4d,0x00,0x32,0x8d,0xa4,0x05,0x01,0xec,0x80 };
static u_int8_t Strm0_PPS[] = { 0x00,0x00,0x00,0x01,0x68,0xee,0x3c,0x80 };
//Stream1: H264 + AAC
static u_int8_t Strm1_SPS[] = { 0x00,0x00,0x00,0x01,0x67,0x4d,0x00,0x32,0x8d,0xa4,0x05,0x01,0xec,0x80 };
static u_int8_t Strm1_PPS[] = { 0x00,0x00,0x00,0x01,0x68,0xee,0x3c,0x80 };
//Stream2: H264(1080P) + PCM
static u_int8_t Strm2_SPS[] = { 0x00,0x00,0x00,0x01,0x67,0x64,0x00,0x29,0xAC,0x1B,0x48,0x07,0x80,0x22,0x7E,0x54 };
static u_int8_t Strm2_PPS[] = { 0x00,0x00,0x00,0x01,0x68,0xEE,0x3C,0xB0 };
//Stream3: H264 + ULAW
static u_int8_t Strm3_SPS[] = { 0x00,0x00,0x00,0x01,0x67,0x4d,0x00,0x32,0x8d,0xa4,0x05,0x01,0xec,0x80 };
static u_int8_t Strm3_PPS[] = { 0x00,0x00,0x00,0x01,0x68,0xee,0x3c,0x80 };
//Stream4: H264 + ALAW
static u_int8_t Strm4_SPS[] = { 0x00,0x00,0x00,0x01,0x67,0x4d,0x00,0x32,0x8d,0xa4,0x05,0x01,0xec,0x80 };
static u_int8_t Strm4_PPS[] = { 0x00,0x00,0x00,0x01,0x68,0xee,0x3c,0x80 };
//Stream5: H265 + PCM
static u_int8_t Strm5_VPS[] = { 0x00,0x00,0x00,0x01,0x40,0x01,0x0C,0x01,0xFF,0xFF,0x01,0x60,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x97,0x02,0x40 };
static u_int8_t Strm5_SPS[] = { 0x00,0x00,0x00,0x01,0x42,0x01,0x01,0x01,0x60,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0xA0,0x02,0x80,0x80,0x2D,0x1F,0xE5,0x97,0x92,0x46,0xD8,0x79,0x79,0x24,0x93,0xF9,0xE7,0xF3,0xCB,0xFF,0xFF,0xFF,0x3F,0x9F,0xCF,0xCF,0xE7,0x6C,0x80 };
static u_int8_t Strm5_PPS[] = { 0x00,0x00,0x00,0x01,0x44,0x01,0xC1,0x90,0x95,0x81,0x12 };

static STATIC_CHANNEL Channels[] = {

	//Stream0: H264 + PCM
	{
		// Video
		{
			{ NULL, 0 }, // VPS
			MAKE_BUF_DESC(Strm0_SPS), // PPS
			MAKE_BUF_DESC(Strm0_PPS), // SPS
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.vid"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_H264,
			60 * 1000,
		},
		// Audio
		{
			32000, // Sample Per Second
			16, // Bit Per Second
			2, // Channel Cnt
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.aud"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_PCM,
			60 * 1000,
		}
	},
	//Stream1: H264 + AAC
	{
		// Video
		{
			{ NULL, 0 }, // VPS
			MAKE_BUF_DESC(Strm1_SPS), // PPS
			MAKE_BUF_DESC(Strm1_PPS), // SPS
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.vid"),{ NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_H264,
			60 * 1000,
		},
		// Audio
		{
			32000, // Sample Per Second
			16, // Bit Per Second
			1, // Channel Cnt
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"aac.aud"), MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"aac.lst") }, // Frm
			NVT_CODEC_TYPE_AAC,
			30 * 1000,
		}
	},
	//Stream2: H264(1080P) + PCM
	{
		// Video
		{
			{ NULL, 0 }, // VPS
			MAKE_BUF_DESC(Strm2_SPS), // PPS
			MAKE_BUF_DESC(Strm2_PPS), // SPS
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov1080.vid"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_H264,
			70 * 1000,
		},
		// Audio
		{
			32000, // Sample Per Second
			16, // Bit Per Second
			2, // Channel Cnt
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov1080.aud"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_PCM,
			70 * 1000,
		}
	},
	//Stream3: H264 + ULAW
	{
		// Video
		{
			{ NULL, 0 }, // VPS
			MAKE_BUF_DESC(Strm3_SPS), // PPS
			MAKE_BUF_DESC(Strm3_PPS), // SPS
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.vid"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_H264,
			60 * 1000,
		},
		// Audio
		{
			32000, // Sample Per Second
			16, // Bit Per Second
			2, // Channel Cnt
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.ulaw"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_G711_ULAW,
			60 * 1000,
		}
	},
	//Stream4: H264 + ALAW
	{
		// Video
		{
			{ NULL, 0 }, // VPS
			MAKE_BUF_DESC(Strm4_SPS), // PPS
			MAKE_BUF_DESC(Strm4_PPS), // SPS
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.vid"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_H264,
			60 * 1000,
		},
		// Audio
		{
			32000, // Sample Per Second
			16, // Bit Per Second
			2, // Channel Cnt
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.alaw"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_G711_ALAW,
			60 * 1000,
		}
	},
	//Stream5: H265 + PCM
	{
		// Video
		{
			MAKE_BUF_DESC(Strm5_VPS), // VPS
			MAKE_BUF_DESC(Strm5_SPS), // PPS
			MAKE_BUF_DESC(Strm5_PPS), // SPS
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"surfing.265.vid"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_H265,
			60 * 1000,
		},
		// Audio
		{
			32000, // Sample Per Second
			16, // Bit Per Second
			2, // Channel Cnt
			{ MAKE_BUF_DESC((unsigned char*)CFG_FILE_DIR"mov.aud"), { NULL, 0 } }, // Frm
			NVT_CODEC_TYPE_PCM,
			60 * 1000,
		}
	},
};

STATIC_CHANNEL *FileFrmGetChannel(int Channel)
{
	if (Channel >= (int)(sizeof(Channels) / sizeof(STATIC_CHANNEL))) {
		return NULL;
	}
	return &Channels[Channel];
}
