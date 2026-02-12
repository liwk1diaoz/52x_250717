#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "sysdef.h" /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>

#include <rtp.h>
#include <rtp_media.h>

#include "rtp-h264.h"
#include "gm_memory.h"

#define BITSTREAM_WITH_STARTCODE
#define USED_MEM_POLL
#define POLL_NUM    24
#define H264_TRUE   1
#define H264_FALSE  0

#ifdef BITSTREAM_WITH_STARTCODE
#define LENGTH_OF_NALUSIZE  4
#else
#define LENGTH_OF_NALUSIZE  4
#endif

extern int rtp_mtu;
#define MAX_PAYLOAD_SIZE    0x5A4 //0x580 // server define max is data size 1444
//#define MEM_POLL_SIZE         2048    //2048 FU parameters
#define MEM_POLL_SIZE       4096    //2048 FU parameters


#ifdef DEBUG_BITSTREAM_OUT_FILE
char dbs_name[64];
FILE *dbs;
#endif

#ifdef DEBUG_SINGLE_NAL_OUT_FILE
char dsnal_name[64];
FILE *dsnal;
#endif

#ifdef DEBUG_STAPA_OUT_FILE
char dstapa_name[54];
FILE *dstapa;
#endif

#ifdef DEBUG_FU_OUT_FILE
char dfu_name[54];
FILE *dfu;
#endif

#ifdef DEBUG_SEND_DATA
char iov_name[54];
FILE *iov;
#endif

#ifdef H264_DEBUG_IOE_HINT
char ioe_name[54];
FILE *ioe;
#endif

// NALU HEADER
#define NALu_TYPE_Unspecified   0
#define NALu_TYPE_nonIDR        1   // GM support
#define NALu_TYPE_CodeSliceA    2
#define NALu_TYPE_CodeSliceB    3
#define NALu_TYPE_CodeSliceC    4
#define NALu_TYPE_IDR           5       // GM support
#define NALu_TYPE_SEI           6
#define NALu_TYPE_SPS           7       // GM support
#define NALu_TYPE_PPS           8       // GM support
#define NALu_TYPE_AUD           9
#define NALu_TYPE_EOSeq         10
#define NALu_TYPE_EOStr         11
#define NALu_TYPE_Filter        12

#define Get_Type(hdr)  ( hdr & 0x1F)
#define Get_Nri(hdr) (hdr & 0x60)

// SLICE HEADER
#define H264_TYPE_P     0
#define H264_TYPE_B     1
#define H264_TYPE_I     2
#define H264_TYPE_SP    3
#define H264_TYPE_SI    4
#define H264_TYPE2_P    5
#define H264_TYPE2_B    6
#define H264_TYPE2_I    7
#define H264_TYPE2_SP   8
#define H264_TYPE2_SI   9


void avsync(struct rtp_endpoint *ep, unsigned int curr_time);   // Carl


int hinit(struct rtp_h264 *out, unsigned int mem_size)
{
	out->mpoll.pidx = 0;
	out->mpoll.idx = 0;
	out->mpoll.size = mem_size;
	out->mpoll.base = malloc(sizeof(unsigned char) * mem_size);
	if (!out->mpoll.base) {
		printf("H.264: MEM Poll allocate fail\n");
		return H264_FALSE;
	}
	return H264_TRUE;
}

void hflush(struct rtp_h264 *out)
{
	out->mpoll.pidx = 0;
	out->mpoll.idx = 0;
	out->mpoll.size = 0;
	free(out->mpoll.base);
}

unsigned char *hmalloc(struct rtp_h264 *out, unsigned int size)
{
	unsigned int off = out->mpoll.pidx;
	unsigned int align_size;

	align_size = (size + (sizeof(unsigned int) - 1)) / sizeof(unsigned int);
	align_size = align_size * sizeof(unsigned int);

	if (align_size > out->mpoll.size) {
		printf("memory poll size too small\n");
		return NULL;
	}
	if ((out->mpoll.pidx + align_size) >= (out->mpoll.size - sizeof(unsigned int))) {
		out->mpoll.pidx = 0;
		off = 0;
	} else {
		out->mpoll.pidx += align_size;
	}

	return (out->mpoll.base + off);
}

/*
 *  h264_get_nal_size
 *  brief/
 *        read the NAL length from sample bitstream
 *  input/
 *        pData
 *              encode bitstream
 *        sizeLength
 *              byte length of NAL_SIZE
 *
 */
#ifdef BITSTREAM_WITH_STARTCODE
#if 1
#define NAL_SEARCH_THRESHOLD    64

static int h264_get_nal_size(unsigned char *pData, int bslen, int sizeLength)
{
	register int        i = 7;
	register int        len;
	char            found = 0;

	if ((bslen < 4) || (bslen < sizeLength)) {
		return 0;
	}
	if ((pData[0]) || (pData[1]) || (pData[2]) || (pData[3] != 0x01)) {
		return 0;
	}
	if ((bslen - sizeLength) < 4) {
		return bslen - sizeLength;    /* Carl, 20090910. */
	}

	len = (NAL_SEARCH_THRESHOLD < bslen) ? NAL_SEARCH_THRESHOLD : bslen;
	while (i < len) {
		if ((pData[i] == 0x01) && (pData[i - 1] == 0x00) && (pData[i - 2] == 0x00) &&
			(pData[i - 3] == 0x00)) {
			i -= 3;
			found = 1;
			break;
		}
		++i;
	}

	return (found) ? i - sizeLength : bslen - sizeLength;
}
#else
static int h264_get_nal_size(unsigned char *pData, int bslen, int sizeLength)
{
	int i = 0;
	int found = 0;

	if (pData[i] == 0x00 && pData[i + 1] == 0x00 && pData[i + 2] == 0x00 && pData[i + 3] == 0x01) {
		i += 3;
		while (i < (bslen - sizeLength)) {
			if (pData[i] == 0x00 && pData[i + 1] == 0x00 && pData[i + 2] == 0x00 && pData[i + 3] == 0x01) {
				found = 1;
				break;
			}
			i++;
		}
	}

	if (found) {
		return (i - sizeLength);
	} else {
		return i;
	}
}
#endif
#else
static int h264_get_nal_size(unsigned char *pData, int bslen, int sizeLength)
{
	unsigned int len;
	if (sizeLength == 1) {
		return *pData;
	} else if (sizeLength == 2) {
		return (pData[0] << 8) | pData[1];
	} else if (sizeLength == 3) {
		return (pData[0] << 16) | (pData[1] << 8) | pData[2];
	}
	len = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
	return len;
}
#endif


/*
 *  h264_add_to_rtp_buf
 *  brief/
 *        add NAL Unit into RTP buffer
 *  input/
 *        out
 *
 *        d
 *              nal unit
 *        len
 *              nalu size
 *
 */
static void h264_add_to_rtp_buf(struct rtp_h264 *out, unsigned char *d, int len)
{
	out->iov[out->iov_count].iov_base = d;
	out->iov[out->iov_count].iov_len = len;
	out->iov_count ++;
}
/*
  * h264_add_to_config
  * brief/
  * write SPS and PPS
  *
  *
  */

static int h264_add_to_sps(struct rtp_h264 *out, unsigned char *d, int len)
{
	if (out->init_done) {
		return 1;
	}

	if (out->sps_len + len > sizeof(out->sps)) {
		spook_log(SL_WARN,
				  "rtp-h264: sps data is %d bytes too large",
				  out->sps_len + len - sizeof(out->sps));
		return 0;
	}

	memcpy(out->sps + out->sps_len, d, len);
	out->sps_len += len;

	return 1;
}

static int h264_add_to_pps(struct rtp_h264 *out, unsigned char *d, int len)
{
	if (out->init_done) {
		return 1;
	}

	if (out->pps_len + len > sizeof(out->pps)) {
		spook_log(SL_WARN,
				  "rtp-h264: pps data is %d bytes too large",
				  out->pps_len + len - sizeof(out->pps));
		return 0;
	}

	memcpy(out->pps + out->pps_len, d, len);
	out->pps_len += len;

	return 1;
}

/*
  * base64_len
  *   /brief
  *       Get the Base64 output length
  */
#define base64_len(n) ((n+2)/3*4)

/**
 * characters used for Base64 encoding
 */
const char *BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * encode three bytes using base64 (RFC 3548)
 *
 * @param triple three bytes that should be encoded
 * @param result buffer of four characters where the result is stored
 */
void base64_encode_triple(unsigned char triple[3], char result[4])
{
	int tripleValue, i;

	tripleValue = triple[0];
	tripleValue *= 256;
	tripleValue += triple[1];
	tripleValue *= 256;
	tripleValue += triple[2];

	for (i = 0; i < 4; i++) {
		result[3 - i] = BASE64[tripleValue % 64];
		tripleValue /= 64;
	}
}
/**
 * encode an array of bytes using Base64 (RFC 3548)
 *
 * @param source the source buffer
 * @param sourcelen the length of the source buffer
 * @param target the target buffer
 * @param targetlen the length of the target buffer
 * @return 1 on success, 0 otherwise
 */
int Base64_encode(unsigned char *source, int  sourcelen, char *target, int  targetlen)
{

	/* check if the result will fit in the target buffer */
	if ((sourcelen + 2) / 3 * 4 > targetlen - 1) {
		return 0;
	}

	/* encode all full triples */
	while (sourcelen >= 3) {
		base64_encode_triple(source, target);
		sourcelen -= 3;
		source += 3;
		target += 4;
	}

	/* encode the last one or two characters */
	if (sourcelen > 0) {
		unsigned char temp[3];
		memset(temp, 0, sizeof(temp));
		memcpy(temp, source, sourcelen);
		base64_encode_triple(temp, target);
		target[3] = '=';
		if (sourcelen == 1) {
			target[2] = '=';
		}

		target += 4;
	}

	/* terminate the string */
	target[0] = 0;

	return 1;
}


/*
 *  h264_finish_config
 *  brief/
 *        write fmtp string, that support non-interleaved packetization mode
 *  input/
 *        out
 *
 */

static void h264_finish_sdp(struct rtp_h264 *out)
{
	char *o = out->fmtp;
	unsigned int i;

	if (out->init_done) {
		return;
	}

	o += sprintf(o, "profile-level-id=42A01E;packetization-mode=1;sprop-parameter-sets=");

	Base64_encode(out->sps, out->sps_len, (char *)(out->config),  sizeof(out->config) - out->config_len);
	for (i = 0; i < base64_len(out->sps_len); ++i) {
		o += sprintf(o, "%c", out->config[i]);
	}
	o += sprintf(o, ",");
	Base64_encode(out->pps, out->pps_len, (char *)(out->config),  sizeof(out->config) - out->config_len);
	for (i = 0; i < base64_len(out->pps_len); ++i) {
		o += sprintf(o, "%c", out->config[i]);
	}

	return;
}


/*
 *  parse_h264_nalu
 *  brief/
 *        add NAL Unit into RTP buffer
 *  input/
 *        out
 *
 *        d
 *              the bitstream of nalus
 *        lwren
 *              the length of Nal unit field
 *
 */
static int parse_fu_nalu_object(struct rtp_h264 *out, unsigned char *d, int wrlen, unsigned char *payloadhdr, unsigned int hdrlen)
{
	unsigned char *cnt;
	unsigned char *hdr;

	if ((out->iov_count + 3) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h264: FU too many elements to send %d>%d", out->iov_count, IOVSIZE);
		return H264_FALSE;
	}
	cnt = hmalloc(out, sizeof(unsigned char));
	if (!cnt) {
		spook_log(SL_WARN, "FU NALU allocate cnt err");
		return H264_FALSE;
	}
	hdr = hmalloc(out, hdrlen);
	if (!hdr) {
		spook_log(SL_WARN, "FU NALU allocate hdr err");
		return H264_FALSE;
	}
	*cnt = 2;
	// iov num count
	h264_add_to_rtp_buf(out, cnt, sizeof(unsigned char));
	// add header
	memcpy(hdr, payloadhdr, sizeof(unsigned char)*hdrlen);
	h264_add_to_rtp_buf(out, hdr, sizeof(unsigned char)*hdrlen);
	//add raw data
	h264_add_to_rtp_buf(out, d, wrlen);

#ifdef DEBUG_FU_OUT_FILE
	//sprintf(dfu_name,"/tmp/nfs/d/Leo/My_Temp/H264_Stuff/tc_fac_funalu%d_%d_%d.264",getpid(), out->mpoll.idx, out->mpoll->pidx);
	sprintf(dfu_name, "/tmp/tc_fac_funalu%d_%d_%d.264", getpid(), out->mpoll.idx, out->mpoll.pidx);
	dfu = fopen(dfu_name, "wb");
	printf("Use FU output %s\n", dfu_name);
	fwrite(payloadhdr, 2, 1, dfu);
	fwrite(d, wrlen, 1, dfu);
	fclose(dfu);
#endif
	return H264_TRUE;
}

/*
 *  parse_single_nalu_object
 *  brief/
 *        pack RTP header and single NAL unit packet
 *  input/
 *        out
 *
 *        d
 *             the bitstream of nalus
 *   wrlen
 *             the length of single NAL unit
 *       len
 *             the length of Nal unit field
 */
static int parse_single_nalu_object(struct rtp_h264 *out, unsigned char *d, int wrlen)
{
	unsigned char *cnt;

	if ((out->iov_count + 2) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h264: single too many elements to send %d>%d", out->iov_count, IOVSIZE);
		return H264_FALSE;
	}
	cnt = hmalloc(out, sizeof(unsigned char));
	if (!cnt) {
		spook_log(SL_WARN, "single NALU allocate cnt err");
		return H264_FALSE;
	}
	// add cnt
	*cnt = 1;
	h264_add_to_rtp_buf(out, cnt, sizeof(unsigned char));
	// add raw data
	h264_add_to_rtp_buf(out, d, wrlen);
#ifdef DEBUG_SINGLE_NAL_OUT_FILE
	sprintf(dsnal_name, "/tmp/nfs/d/Leo/My_Temp/H264_Stuff/tc_fac_snalu%d_%d_%d.264", getpid(), out->mpoll.idx, out->mpoll.pidx);
	dsnal = fopen(dsnal_name, "wb");
	printf("Use Single output %s\n", dsnal_name);
	fwrite(d, wrlen, 1, dsnal);
	fclose(dsnal);
#endif
	return H264_TRUE;
}

/*
 *  add_stapa_nalu_cnt
 *  brief/
 *        pack RTP header and single NAL unit packet
 *  input/
 *        out
 *
 *        d
 *             the bitstream of nalus
 *   wrlen
 *             the length of single NAL unit
 *       len
 *             the length of Nal unit field
 */
static int parse_stapa_nalu_cnt_object(struct rtp_h264 *out, unsigned char num)
{
	unsigned char *cnt;

	if ((out->iov_count + 1) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h264: STAPA CNT too many elements to send");
		return H264_FALSE;
	}
	cnt = hmalloc(out, sizeof(unsigned char));
	if (!cnt) {
		spook_log(SL_WARN, "STAPA single NALU CNT allocate cnt err");
		return H264_FALSE;
	}
	*cnt = num;
	h264_add_to_rtp_buf(out, cnt, sizeof(unsigned char));

	return H264_TRUE;
}
/*
 *  parse_stapa_nalu_object
 *  brief/
 *        pack RTP header and single NAL unit packet
 *  input/
 *        out
 *
 *        d
 *             the bitstream of nalus
 *   wrlen
 *             the length of single NAL unit
 *       len
 *             the length of Nal unit field
 */
static int parse_stapa_nalu_object(struct rtp_h264 *out, unsigned char *d, int wrlen, unsigned char *payloadhdr, unsigned int hdrlen)
{
	unsigned char *hdr;

	if ((out->iov_count + 2) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h264: STAPA too many elements to send");
		return H264_FALSE;
	}
	hdr = hmalloc(out, sizeof(unsigned char) * hdrlen);
	if (!hdr) {
		spook_log(SL_WARN, "single NALU HDR allocate hdr err");
		return H264_FALSE;
	}
	// add header
	memcpy(hdr, payloadhdr, sizeof(unsigned char)*hdrlen);
	//h264_add_to_rtp_buf(out, hdr, sizeof(unsigned char)*hdrlen);
	// add raw data
	h264_add_to_rtp_buf(out, d,  wrlen);
#ifdef DEBUG_STAPA_OUT_FILE
	sprintf(dstapa_name, "/tmp/nfs/d/Leo/My_Temp/H264_Stuff/tc_fac_stapa%d_%d_%d.264", getpid(), out->mpoll.idx, out->mpoll->pidx);
	dstapa = fopen(dstapa_name, "wb");
	printf("Use STAPA output %s\n", dstapa_name);
	fwrite(payloadhdr, hdrlen, 1, dstapa);
	fwrite(d, wrlen, 1, dstapa);
#endif
	return H264_TRUE;
}

static void finish_stapa_nalu_object(struct rtp_h264 *out)
{
#ifdef DEBUG_STAPA_OUT_FILE
	fclose(dstapa);
#endif
}
///////////////////////////////////////////////////////////////
///////////////////// SLICE FUNCTION //////////////////////////
///////////////////////////////////////////////////////////////

int h264_u_v(int LenInBits, H264Bitstream *bitstream)
{
	SyntaxElement symbol, *sym = &symbol;

	//sym->type = SE_HEADER;
	sym->mapping = linfo_ue;   // Mapping rule
	sym->len = LenInBits;
	readSyntaxElement_FLC(sym, bitstream);
	//UsedBits+=sym->len;
	return sym->inf;
};

int h264_ue_v(H264Bitstream *bitstream)
{
	SyntaxElement symbol, *sym = &symbol;

	sym->mapping = linfo_ue;   // Mapping rule
	readSyntaxElement_VLC(sym, bitstream);
	//UsedBits+=sym->len;
	return sym->value1;
}

int h264_se_v(H264Bitstream *bitstream)
{
	SyntaxElement symbol, *sym = &symbol;

	sym->mapping = linfo_se;   // Mapping rule: signed integer
	readSyntaxElement_VLC(sym, bitstream);
	//UsedBits+=sym->len;
	return sym->value1;
}

int readSyntaxElement_FLC(SyntaxElement *sym, H264Bitstream *currStream)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	unsigned char *buf = currStream->streamBuffer;
	//int BitstreamLengthInBytes = currStream->bitstream_length;

	//if ((GetBits(buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes, sym->len)) < 0)
	//return -1;
	H264GetBits(buf, frame_bitoffset, &(sym->inf), sym->len);

	currStream->frame_bitoffset += sym->len; // move bitstream pointer
	sym->value1 = sym->inf;

	return 1;
}

int readSyntaxElement_VLC(SyntaxElement *sym, H264Bitstream *currStream)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	unsigned char *buf = currStream->streamBuffer;

	sym->len =  GetVLCSymbol(buf, frame_bitoffset, &(sym->inf));
	currStream->frame_bitoffset += sym->len;
	sym->mapping(sym->len, sym->inf, &(sym->value1), &(sym->value2));

	return 1;
}



void linfo_ue(int len, int info, int *value1, int *dummy)
{
	*value1 = (int)pow(2, (len / 2)) + info - 1;
}

void linfo_se(int len,  int info, int *value1, int *dummy)
{
	int n;
	n = (int)pow(2, (len / 2)) + info - 1;
	*value1 = (n + 1) / 2;
	if ((n & 0x01) == 0) {                      // lsb is signed bit
		*value1 = -*value1;
	}
}

int H264GetBits(unsigned char buffer[], int totbitoffset, int *info, int numbits)
{
	register int inf;
	long byteoffset;      // byte from start of buffer
	int bitoffset;      // bit from start of byte

	int bitcounter = numbits;
	byteoffset = totbitoffset / 8;
	bitoffset = 7 - (totbitoffset % 8);

	inf = 0;
	while (numbits) {
		inf <<= 1;
		inf |= (buffer[byteoffset] & (0x01 << bitoffset)) >> bitoffset;
		numbits--;
		bitoffset--;
		if (bitoffset < 0) {
			byteoffset++;
			bitoffset += 8;
		}
	}

	*info = inf;
	return bitcounter;           // return absolute offset in bit from start of frame
}


int GetVLCSymbol(unsigned char *buffer, int totbitoffset, int *info)
{
	register int inf;
	long byteoffset;      // byte from start of buffer
	int bitoffset;      // bit from start of byte
	int ctr_bit = 0;    // control bit for current bit posision
	int bitcounter = 1;
	int len;
	int info_bit;

	byteoffset = totbitoffset / 8;
	bitoffset = 7 - (totbitoffset % 8);
	ctr_bit = (buffer[byteoffset] & (0x01 << bitoffset)); // set up control bit

	len = 1;
	while (ctr_bit == 0) {
		// find leading 1 bit
		len++;
		bitoffset -= 1;
		bitcounter++;
		if (bitoffset < 0) {
			// finish with current byte ?
			bitoffset = bitoffset + 8;
			byteoffset++;
		}
		ctr_bit = buffer[byteoffset] & (0x01 << (bitoffset));
	}
	// make infoword
	inf = 0;                        // shortest possible code is 1, then info is always 0
	for (info_bit = 0; (info_bit < (len - 1)); info_bit++) {
		bitcounter++;
		bitoffset -= 1;
		if (bitoffset < 0) {
			// finished with current byte ?
			bitoffset = bitoffset + 8;
			byteoffset++;
		}
		//if (byteoffset > bytecount)
		//{
		//  return -1;
		//}
		inf = (inf << 1);
		if (buffer[byteoffset] & (0x01 << (bitoffset))) {
			inf |= 1;
		}
	}

	*info = inf;
	return bitcounter;           // return absolute offset in bit from start of frame
}


#if 0   /* Carl. */
static int h264_get_slice_type(unsigned char *pstr)
{
	H264Bitstream bitstream, *pbitstream;
	int slice_type;

	pbitstream = &bitstream;
	pbitstream->frame_bitoffset = 0;
	pbitstream->streamBuffer = pstr;
	h264_ue_v(pbitstream);                 // first_mb_in_slice
	slice_type = h264_ue_v(pbitstream);     // slice_type
	//spook_log( SL_VERBOSE, "SLICE TYPE %d", slice_type );
	return slice_type;
}

static int H264_TYPE_IS_I(int stype)
{
	if ((stype == H264_TYPE_I) || (stype == H264_TYPE2_I)) {
		return 1;
	} else {
		return 0;
	}
}


static void h264_check_slice_I(struct rtp_h264 *out, unsigned char *pstr)
{
	int slice_type;

	slice_type = h264_get_slice_type(pstr);

	if (H264_TYPE_IS_I(slice_type)) {
		//spook_log( SL_VERBOSE, "non_IDR with H264_TYPE2_I" );
		out->keyframe = 1;
	} else {
		//spook_log( SL_VERBOSE, "non_IDR with H264_TYPE2_P" );
		out->keyframe = 0;
	}
}
#endif


///////////////////////////////////////////////////////////////
///////////////////// NALU FUNCTION //////////////////////////
///////////////////////////////////////////////////////////////
/*
 *  h264_get_sample_nal_hdr
 *  brief/
 *        read the NAL header from sample bitstream
 *  input/
 *        pSampleBuffer
 *             encode bitstream: NAL Unit syntax-> NAL_SIZE + NAL_DATA
 *        sampleSize
 *             bitstream total size
 *        sizeLength
 *             byte length of NAL_SIZE
 */
static unsigned char h264_get_nal_hdr(unsigned char *pBuffer, unsigned int offset)
{
	return pBuffer[offset] ;
}


#if 0
/*
  * h264_nalu_type_check
  * brief/
  *       check the NALU type is slice
  *
  *
  */
static int h264_nalu_type_check(unsigned char hdr)
{
	unsigned char type = Get_Type(hdr);
	int ret = H264_FALSE;
	switch (type) {
	case NALu_TYPE_Unspecified:
		break;
	case NALu_TYPE_nonIDR:
		ret = H264_TRUE;
		break;
	case NALu_TYPE_CodeSliceA:
		break;
	case NALu_TYPE_CodeSliceB:
		break;
	case NALu_TYPE_CodeSliceC:
		break;
	case NALu_TYPE_IDR:
		ret = H264_TRUE;
		break;
	case NALu_TYPE_SEI:
		break;
	case NALu_TYPE_SPS:
		ret = H264_TRUE;
		break;
	case NALu_TYPE_PPS:
		ret = H264_TRUE;
		break;
	case NALu_TYPE_AUD:
		break;
	case NALu_TYPE_EOSeq:
		break;
	case NALu_TYPE_EOStr:
		break;
	case NALu_TYPE_Filter:
		break;
	default:
		break;
	}
	return ret;

}
#endif
/*
  * h264_header_parsing
  * brief/
  * 1. check IDP or non_IDP
  * 2. check SPS
  * 3. check PPS
  *
  */
static int h264_header_parsing(
	struct rtp_h264 *out,
	unsigned char hdr,
	unsigned char *str,
	unsigned int str_len)
{
	int ret = 0;

	unsigned char type = Get_Type(hdr);
	switch (type) {
	case NALu_TYPE_IDR:
		//printf("NALu_TYPE_IDR\n");
		out->keyframe = 1;
		break;
	case NALu_TYPE_SPS:
		if (!h264_add_to_sps(out, str, str_len)) {
			printf("add sps err\n");
		}
		break;
	case NALu_TYPE_PPS:
		if (!h264_add_to_pps(out, str, str_len)) {
			printf("add pps err\n");
		}
		h264_finish_sdp(out);
		out->vop_time_increment_resolution = 10;
		out->vop_time_increment = 0;
		out->init_done = 1;
		break;
	case NALu_TYPE_nonIDR:
		//printf("NALu_TYPE_nonIDR\n");
		//h264_check_slice_I(out, str+1);
		out->keyframe = 0;
		break;
	case NALu_TYPE_Unspecified :
		printf("NALu_TYPE_Unspecified: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_CodeSliceA:
		printf("NALu_TYPE_CodeSliceA: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_CodeSliceB:
		printf("NALu_TYPE_CodeSliceB: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_CodeSliceC:
		printf("NALu_TYPE_CodeSliceC: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_SEI:
		//printf("NALu_TYPE_SEI: GM is not support\n");
		//out->init_done = 0;
		//ret = -1;
		break;
	case NALu_TYPE_AUD:
		printf("NALu_TYPE_AUD: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_EOSeq:
		printf("NALu_TYPE_EOSeq: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_EOStr:
		printf("NALu_TYPE_EOStr: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	case NALu_TYPE_Filter:
		printf("NALu_TYPE_Filter: GM is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	default:
		printf("NALu_TYPE: Spec is not support\n");
		out->init_done = 0;
		ret = -1;
		break;
	}

	return ret;
}
/*
 *  h264_process_frame
 *  brief/
 *        process NAL Units of one frame
 *  input/
 *        f
 *
 *        d
 *            the bitstream of nalus
 * output/
 *
 */
static int h264_process_frame(struct frame *f, void *d)
{
	struct rtp_h264 *out = (struct rtp_h264 *)d;
	unsigned int remaining = f->length;
	unsigned int offset = 0 ;

	//printf("xx h264~\n");

#if defined( DEBUG_BITSTREAM_OUT_FILE)  \
    || defined (DEBUG_SINGLE_NAL_OUT_FILE)  \
    || defined (DEBUG_STAPA_OUT_FILE )  \
    || defined (DEBUG_FU_OUT_FILE)

	if ((out->mpoll.idx + 1) >= POLL_NUM) {
		out->mpoll.idx = 0;
	} else {
		out->mpoll.idx++;
	}
#endif

#ifdef DEBUG_BITSTREAM_OUT_FILE
	sprintf(dbs_name, "/tmp/nfs/d/Leo/My_Temp/H264_Stuff/tc_fac_bs%d_%d.264", getpid(), out->mpoll.idx);
	dbs = fopen(dbs_name, "wb");
	printf("Use encode output %s\n", dbs_name);
	fwrite(f->d, f->length, 1, dbs);
	fclose(dbs);
#endif
	out->mpoll.pidx = 0;
	out->iov_count = 0;
	out->keyframe = 0;
	out->sps_len = 0;
	out->pps_len = 0;
	out->config_len = 0;
	if ((f->length - LENGTH_OF_NALUSIZE) < rtp_mtu) {
		unsigned int first_nal = h264_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);
		if (first_nal + LENGTH_OF_NALUSIZE == f->length) {
			// we have a single nal, less than the maxPayloadSize,
			// so, we have Single Nal unit mode
			unsigned char hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
			if (h264_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, first_nal) < 0) {
				return out->init_done;
			}
			offset += LENGTH_OF_NALUSIZE;
			remaining -= LENGTH_OF_NALUSIZE;

			parse_single_nalu_object(out, f->d + offset, first_nal);
#ifdef H264_DEBUG_HINT
			printf("1. SINGLE h 0x%x s %d p %d rem %d\n",
				   hdr,
				   f->length,
				   first_nal,
				   remaining);
#endif

			offset += first_nal;
			remaining -= first_nal;

		}

	}

	while (remaining) {
		unsigned int nal_size = h264_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);

		if (nal_size > rtp_mtu) {
			// FU_A RTP packet
			unsigned char hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
			unsigned char fu_header[2];
			unsigned int write_size;

			if (h264_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
				return out->init_done;
			}
			offset += LENGTH_OF_NALUSIZE;
			remaining -= LENGTH_OF_NALUSIZE;

			fu_header[0] = (unsigned char)((hdr & 0xe0) | 28);   // set F bit | nri | FU-A type
			fu_header[1] = (unsigned char)(0x80 | Get_Type(hdr));            // set start bit |FU-A type
			// NALU type is not included in FU-A
			offset += 1;
			remaining -= 1;
			nal_size -= 1;
			while (nal_size > 0) {
				if ((nal_size + 2) <= rtp_mtu) {
					fu_header[1] |= 0x40;
					write_size = nal_size;
				} else {
					write_size = rtp_mtu - 2;
				}

				parse_fu_nalu_object(out, f->d + offset, write_size, fu_header, 2);
#ifdef H264_DEBUG_HINT
				printf("2. FU h 0x%x s %d p %d\n", hdr, remaining, write_size);
#endif

				fu_header[1] = Get_Type(hdr);
				offset += write_size;
				nal_size -= write_size;
				remaining -= write_size;
			}
		} else {
			// we have a smaller than MTU nal.  check the next sample
			// see if the next one fits;
			unsigned char have_stap = 0;
			unsigned int next_size_offset = offset + LENGTH_OF_NALUSIZE + nal_size;
			if (next_size_offset <  remaining) {
				// we have a remaining NAL
				unsigned int next_nal_size = h264_get_nal_size(f->d + next_size_offset, remaining - next_size_offset, LENGTH_OF_NALUSIZE);
				if (((next_nal_size + 2)
					 + (nal_size + 2)
					 + 1/* STAP-A NAL HDR*/) <= rtp_mtu) {
					have_stap = 1;
				}
			}

			if (have_stap == 0) {
				// we have to fit this nal into a packet - the next one is too big
				unsigned char hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);

				if (h264_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
					return out->init_done;
				}
				offset += LENGTH_OF_NALUSIZE;
				remaining -= LENGTH_OF_NALUSIZE;

				parse_single_nalu_object(out, f->d + offset, nal_size);
#ifdef H264_DEBUG_HINT
				printf("3. Single h 0x%x s %d p %d\n", hdr, remaining, nal_size);
#endif

				offset += nal_size;
				remaining -= nal_size;
			} else { // pack STAPA RTP
				// we can fit multiple NALs into this packet
				unsigned int bytes_in_stap = 0;
				unsigned char max_nri ;
				unsigned char hdr;
				unsigned char last;
				unsigned char data[3];
				unsigned char cnt = 1;

				hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
				max_nri = Get_Nri(hdr);
				{
					unsigned int nri_nal_size = nal_size + 1 + 2;
					unsigned int nri_bytes_in_stap = nal_size + 1 + 2;
					unsigned int nri_remaining = remaining - next_size_offset;
					unsigned int nri_offset = next_size_offset;
					unsigned char nri;

					nri_nal_size = h264_get_nal_size(f->d + nri_offset, nri_remaining, LENGTH_OF_NALUSIZE);
					while (((nri_bytes_in_stap + nri_nal_size + 2) <= rtp_mtu) && (nri_remaining > 0)) {
						nri_offset += nri_nal_size + LENGTH_OF_NALUSIZE;
						nri_remaining -= nri_nal_size + LENGTH_OF_NALUSIZE;
						nri_bytes_in_stap += nri_nal_size + 2;
						cnt++;
						hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
						nri = Get_Nri(hdr);
						//printf("NRI max %d cur %d\n", max_nri, nri);
						if (nri > max_nri) {
							max_nri = nri;
						}
						if (nri_remaining > 0) {
							nri_nal_size = h264_get_nal_size(f->d + nri_offset, nri_remaining, LENGTH_OF_NALUSIZE);
						}
					}
				}

				hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
				if (h264_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
					return out->init_done;
				}

				if (next_size_offset >= (offset +  remaining)
					&&  bytes_in_stap <= rtp_mtu) {
					// stap is last frame
					last = 1;
				} else {
					last = 0;
				}

				data[0] = (hdr & 0x80) | max_nri | 24; // nri | STAPA_TYPE
				data[1] = (unsigned char)((nal_size >> 8) & 0xFF);
				data[2] = (unsigned char)(nal_size & 0xFF);
				offset += LENGTH_OF_NALUSIZE;
				remaining -= LENGTH_OF_NALUSIZE;

				//parse_stapa_nalu_cnt_object(out, cnt*2);
				parse_stapa_nalu_cnt_object(out, 1);
				parse_stapa_nalu_object(out, f->d + offset, nal_size, data, 3);
#ifdef H264_DEBUG_HINT
				printf("4. STAP h 0x%x s %d p %d\n", hdr, remaining, nal_size);
#endif

				offset += nal_size;
				remaining -= nal_size;
				bytes_in_stap = /*1 + 2 +*/ nal_size;
				nal_size = h264_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);
				while (((bytes_in_stap + nal_size + LENGTH_OF_NALUSIZE) <= rtp_mtu) && (remaining > 0)) {
					data[0] = (unsigned char)((nal_size >> 8) & 0xFF);
					data[1] = (unsigned char)(nal_size & 0xFF);

					hdr = h264_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
					if (h264_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
						return out->init_done;
					}

					offset += LENGTH_OF_NALUSIZE;
					remaining -= LENGTH_OF_NALUSIZE;
					parse_stapa_nalu_cnt_object(out, 1);
					parse_stapa_nalu_object(out, f->d + offset, nal_size, data, 2);
#ifdef H264_DEBUG_HINT
					printf("5. STAP h 0x%x s %d p %d\n", hdr, remaining, nal_size);
#endif

					offset += nal_size;
					remaining -= nal_size;
					bytes_in_stap += nal_size /*+ 2*/;
					if (remaining > 0) {
						nal_size = h264_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);
					}
				} // end while stap
				finish_stapa_nalu_object(out);
			} // end have stap
		} // end check size
	}
	return out->init_done;
}

static int h264_get_sdp(char *dest, int len, int payload, int port, void *d)
{
#if 1   // Carl
	dbg("%s should NOT be called.\n", __func__);
	return 0;
#else
	struct rtp_h264 *out = (struct rtp_h264 *)d;

	if (! out->init_done) {
		return -1;
	}

	return snprintf(dest, len, "m=video %d RTP/AVP %d\r\na=rtpmap:%d H264/90000\r\na=fmtp:%d %s\r\n", port, payload, payload, payload, out->fmtp);
#endif
}

/*
 *  h264_send
 *  brief/
 *        read encode bitstream to packed into RTP packet
 *  input/
 *        ep
 *
 *        d
 *
 *        curr_time
 *
 *        delay
 *
 *  output/
 *
 */
char h264_start_code[10] = { 0x00, 0x00, 0x01, 0xB0, 0x01, 0x00, 0x00, 0x01, 0xB5, 0x09};
static int h264_send(struct rtp_endpoint *ep, void *d, unsigned int curr_time, unsigned int *delay)
{
	struct rtp_h264 *out = (struct rtp_h264 *)d;
	unsigned char cnt;
	int i, j;
	struct iovec v[IOVSIZE];

#ifdef DEBUG_SEND_DATA
	int index = 0;
	while (index < out->iov_count) {
		sprintf(iov_name, "/tmp/nfs/d/Leo/My_Temp/H264_Stuff/tc_iov_bf%d_%d_%d.264", getpid(), mpoll.idx, index);
		iov = fopen(iov_name, "wb");
		printf("Use encode output %s\n", iov_name);
		fwrite(out->iov[index].iov_base, out->iov[index].iov_len, 1, iov);
		fclose(iov);
		index++;
	}
#endif

	if (!ep->keyframe_arrival && !out->keyframe) {
		return 0;
	}
	ep->keyframe_arrival = 1;

	avsync(ep, curr_time);  // Carl

	i = 0;
	while (i < out->iov_count) {
		cnt = *((unsigned char *)out->iov[i].iov_base) ;

		i++;
		j = 0;
		while (j < cnt) {
			v[j + 1].iov_base = out->iov[i].iov_base ;
			v[j + 1].iov_len = out->iov[i].iov_len ;
			j++;
			i++;
		}

		if (i < (out->iov_count - 1)) {
#ifdef H264_DEBUG_HINT
			printf("1. h264_send total %d idx %d cnt %d\n", out->iov_count, i, cnt);
#endif
			if (send_rtp_packet(ep, v, cnt + 1, ep->last_timestamp, 0) < 0) {
				return -1;
			}
		} else {
#ifdef H264_DEBUG_HINT
			printf("2. h264_send total %d idx %d cnt %d\n", out->iov_count, i, cnt);
#endif
			if (send_rtp_packet(ep, v, cnt + 1, ep->last_timestamp, 1) < 0) {
				return -1;
			}
		}
	}

	ep->stopdetect = 0;

	return 0;
}

/*
  * Sinagle NAL unit payload : 98
  * non-interleaved NAL unit : 99
  * interleave NAL unit : 100
  *
  */
static int h264_get_payload(int payload, void *d)
{
	return 96;//99;
}


// Carl
int h264_release(void *p)
{
	struct rtp_h264 *rtp = (struct rtp_h264 *)p;
	if (rtp == NULL) {
		return -1;
	}
	if (rtp->mpoll.base) {
		free(rtp->mpoll.base);
	}
	free(rtp);
	return 0;
}


struct rtp_media *new_rtp_media_h264(struct stream *stream)
{
	struct rtp_media    *m;     // Carl
	struct rtp_h264 *out;

	out = (struct rtp_h264 *)malloc(sizeof(struct rtp_h264));
	out->iov_count = 0;
	out->sps_len = 0;
	out->pps_len = 0;
	out->config_len = 0;
	out->init_done = 1;     // Carl
	out->pali = 0;
	out->timestamp = 0;

#ifdef USED_MEM_POLL
	if (!hinit(out, MEM_POLL_SIZE)) {
		printf("######MEM POLL ALLOCATE FAIL######\n");
	}
#endif

	// Carl
	m = new_rtp_media(h264_get_sdp, h264_get_payload, h264_process_frame, h264_send, h264_release, out, 90000);
	if (m == NULL) {
		free(out);
	}

	return m;
}


