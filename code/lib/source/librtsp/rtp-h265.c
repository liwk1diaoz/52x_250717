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

#include "rtp-h265.h"
#include "gm_memory.h"

#define BITSTREAM_WITH_STARTCODE
#define USED_MEM_POLL
#define POLL_NUM    24
#define H265_TRUE   1
#define H265_FALSE  0

#ifdef BITSTREAM_WITH_STARTCODE
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

#ifdef H265_DEBUG_IOE_HINT
char ioe_name[54];
FILE *ioe;
#endif

// NALU HEADER
enum {
	NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0
	NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1

	NAL_UNIT_CODED_SLICE_TSA_N,     // 2
	NAL_UNIT_CODED_SLICE_TLA,       // 3   // Current name in the spec: TSA_R

	NAL_UNIT_CODED_SLICE_STSA_N,    // 4
	NAL_UNIT_CODED_SLICE_STSA_R,    // 5

	NAL_UNIT_CODED_SLICE_RADL_N,    // 6
	NAL_UNIT_CODED_SLICE_DLP,       // 7 // Current name in the spec: RADL_R

	NAL_UNIT_CODED_SLICE_RASL_N,    // 8
	NAL_UNIT_CODED_SLICE_TFD,       // 9 // Current name in the spec: RASL_R

	NAL_UNIT_RESERVED_10,
	NAL_UNIT_RESERVED_11,
	NAL_UNIT_RESERVED_12,
	NAL_UNIT_RESERVED_13,
	NAL_UNIT_RESERVED_14,
	NAL_UNIT_RESERVED_15,
	NAL_UNIT_CODED_SLICE_BLA,       // 16   // Current name in the spec: BLA_W_LP
	NAL_UNIT_CODED_SLICE_BLANT,     // 17   // Current name in the spec: BLA_W_DLP
	NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18
	NAL_UNIT_CODED_SLICE_IDR,       // 19  // Current name in the spec: IDR_W_DLP
	NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20
	NAL_UNIT_CODED_SLICE_CRA,       // 21
	NAL_UNIT_RESERVED_22,
	NAL_UNIT_RESERVED_23,

	NAL_UNIT_RESERVED_24,
	NAL_UNIT_RESERVED_25,
	NAL_UNIT_RESERVED_26,
	NAL_UNIT_RESERVED_27,
	NAL_UNIT_RESERVED_28,
	NAL_UNIT_RESERVED_29,
	NAL_UNIT_RESERVED_30,
	NAL_UNIT_RESERVED_31,

	NAL_UNIT_VPS,                   // 32
	NAL_UNIT_SPS,                   // 33
	NAL_UNIT_PPS,                   // 34
	NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
	NAL_UNIT_EOS,                   // 36
	NAL_UNIT_EOB,                   // 37
	NAL_UNIT_FILLER_DATA,           // 38
	NAL_UNIT_SEI,                   // 39 Prefix SEI
	NAL_UNIT_SEI_SUFFIX,            // 40 Suffix SEI
	NAL_UNIT_RESERVED_41,
	NAL_UNIT_RESERVED_42,
	NAL_UNIT_RESERVED_43,
	NAL_UNIT_RESERVED_44,
	NAL_UNIT_RESERVED_45,
	NAL_UNIT_RESERVED_46,
	NAL_UNIT_RESERVED_47,
	NAL_UNIT_UNSPECIFIED_48,
	NAL_UNIT_UNSPECIFIED_49,
	NAL_UNIT_UNSPECIFIED_50,
	NAL_UNIT_UNSPECIFIED_51,
	NAL_UNIT_UNSPECIFIED_52,
	NAL_UNIT_UNSPECIFIED_53,
	NAL_UNIT_UNSPECIFIED_54,
	NAL_UNIT_UNSPECIFIED_55,
	NAL_UNIT_UNSPECIFIED_56,
	NAL_UNIT_UNSPECIFIED_57,
	NAL_UNIT_UNSPECIFIED_58,
	NAL_UNIT_UNSPECIFIED_59,
	NAL_UNIT_UNSPECIFIED_60,
	NAL_UNIT_UNSPECIFIED_61,
	NAL_UNIT_UNSPECIFIED_62,
	NAL_UNIT_UNSPECIFIED_63,
	NAL_UNIT_INVALID
};

#define Get_Type(hdr)  ( (hdr>>1) & 0x3F)
#define Get_Nri(hdr) (hdr & 0x60)




void avsync(struct rtp_endpoint *ep, unsigned int curr_time);   // Carl


int h265_hinit(struct rtp_h265 *out, unsigned int mem_size)
{
	out->mpoll.pidx = 0;
	out->mpoll.idx = 0;
	out->mpoll.size = mem_size;
	out->mpoll.base = malloc(sizeof(unsigned char) * mem_size);
	if (!out->mpoll.base) {
		printf("H.265: MEM Poll allocate fail\n");
		return H265_FALSE;
	}
	return H265_TRUE;
}

void h265_hflush(struct rtp_h265 *out)
{
	out->mpoll.pidx = 0;
	out->mpoll.idx = 0;
	out->mpoll.size = 0;
	free(out->mpoll.base);
}

unsigned char *h265_hmalloc(struct rtp_h265 *out, unsigned int size)
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
 *  h265_get_nal_size
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
#define NAL_SEARCH_THRESHOLD    128
static int h265_get_nal_size(unsigned char *pData, int bslen, int sizeLength)
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
		return bslen - sizeLength;
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
#endif


/*
 *  h265_add_to_rtp_buf
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
static void h265_add_to_rtp_buf(struct rtp_h265 *out, unsigned char *d, int len)
{
	out->iov[out->iov_count].iov_base = d;
	out->iov[out->iov_count].iov_len = len;
	out->iov_count ++;
}
/*
  * h265_add_to_config
  * brief/
  * write VPS , SPS and PPS
  *
  *
  */

static int h265_add_to_vps(struct rtp_h265 *out, unsigned char *d, int len)
{
	if (out->init_done) {
		return 1;
	}

	if (out->vps_len + len > sizeof(out->vps)) {
		spook_log(SL_WARN,
				  "rtp-h265: vps data is %d bytes too large",
				  out->vps_len + len - sizeof(out->vps));
		return 0;
	}

	memcpy(out->vps + out->vps_len, d, len);
	out->vps_len += len;

	return 1;
}

static int h265_add_to_sps(struct rtp_h265 *out, unsigned char *d, int len)
{
	if (out->init_done) {
		return 1;
	}

	if (out->sps_len + len > sizeof(out->sps)) {
		spook_log(SL_WARN,
				  "rtp-h265: sps data is %d bytes too large",
				  out->sps_len + len - sizeof(out->sps));
		return 0;
	}

	memcpy(out->sps + out->sps_len, d, len);
	out->sps_len += len;

	return 1;
}

static int h265_add_to_pps(struct rtp_h265 *out, unsigned char *d, int len)
{
	if (out->init_done) {
		return 1;
	}

	if (out->pps_len + len > sizeof(out->pps)) {
		spook_log(SL_WARN,
				  "rtp-h265: pps data is %d bytes too large",
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
 * encode three bytes using base64 (RFC 3548)
 *
 * @param triple three bytes that should be encoded
 * @param result buffer of four characters where the result is stored
 */
extern void base64_encode_triple(unsigned char triple[3], char result[4]);

/**
 * encode an array of bytes using Base64 (RFC 3548)
 *
 * @param source the source buffer
 * @param sourcelen the length of the source buffer
 * @param target the target buffer
 * @param targetlen the length of the target buffer
 * @return 1 on success, 0 otherwise
 */
extern int Base64_encode(unsigned char *source, int  sourcelen, char *target, int  targetlen);


/*
 *  h265_finish_config
 *  brief/
 *        write fmtp string, that support non-interleaved packetization mode
 *  input/
 *        out
 *
 */

static void h265_finish_sdp(struct rtp_h265 *out)
{
	char *o = out->fmtp;
	unsigned int i;

	if (out->init_done) {
		return;
	}

	o += sprintf(o, "profile-space=0;profile-id=1;tier-flag=0;level-id=%d;interop-constraints=000000000000;sprop-vps=", out->vps[out->vps_len - 3]);
	Base64_encode(out->vps, out->vps_len, (char *)(out->config),  sizeof(out->config) - out->config_len);
	for (i = 0; i < base64_len(out->vps_len); ++i) {
		o += sprintf(o, "%c", out->config[i]);
	}
	o += sprintf(o, ";sprop-sps=");
	Base64_encode(out->sps, out->sps_len, (char *)(out->config),  sizeof(out->config) - out->config_len);
	for (i = 0; i < base64_len(out->sps_len); ++i) {
		o += sprintf(o, "%c", out->config[i]);
	}
	o += sprintf(o, ";sprop-pps=");
	Base64_encode(out->pps, out->pps_len, (char *)(out->config),  sizeof(out->config) - out->config_len);
	for (i = 0; i < base64_len(out->pps_len); ++i) {
		o += sprintf(o, "%c", out->config[i]);
	}

	return;
}


/*
 *  parse_h265_nalu
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
static int h265_parse_fu_nalu_object(struct rtp_h265 *out, unsigned char *d, int wrlen, unsigned char *payloadhdr, unsigned int hdrlen)
{
	unsigned char *cnt;
	unsigned char *hdr;

	if ((out->iov_count + 3) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h265: FU too many elements to send %d>%d", out->iov_count, IOVSIZE);
		return H265_FALSE;
	}
	cnt = h265_hmalloc(out, sizeof(unsigned char));
	if (!cnt) {
		spook_log(SL_WARN, "FU NALU allocate cnt err");
		return H265_FALSE;
	}
	hdr = h265_hmalloc(out, hdrlen);
	if (!hdr) {
		spook_log(SL_WARN, "FU NALU allocate hdr err");
		return H265_FALSE;
	}
	*cnt = 2;
	// iov num count
	h265_add_to_rtp_buf(out, cnt, sizeof(unsigned char));
	// add header
	memcpy(hdr, payloadhdr, sizeof(unsigned char)*hdrlen);
	h265_add_to_rtp_buf(out, hdr, sizeof(unsigned char)*hdrlen);
	//add raw data
	h265_add_to_rtp_buf(out, d, wrlen);

#ifdef DEBUG_FU_OUT_FILE
	sprintf(dfu_name, "/tmp/tc_fac_funalu%d_%d_%d.265", getpid(), out->mpoll.idx, out->mpoll.pidx);
	dfu = fopen(dfu_name, "wb");
	printf("Use FU output %s\n", dfu_name);
	fwrite(payloadhdr, 2, 1, dfu);
	fwrite(d, wrlen, 1, dfu);
	fclose(dfu);
#endif
	return H265_TRUE;
}

/*
 *  h265_parse_single_nalu_object
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
static int h265_parse_single_nalu_object(struct rtp_h265 *out, unsigned char *d, int wrlen)
{
	unsigned char *cnt;

	if ((out->iov_count + 2) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h265: single too many elements to send %d>%d", out->iov_count, IOVSIZE);
		return H265_FALSE;
	}
	cnt = h265_hmalloc(out, sizeof(unsigned char));
	if (!cnt) {
		spook_log(SL_WARN, "single NALU allocate cnt err");
		return H265_FALSE;
	}
	// add cnt
	*cnt = 1;
	h265_add_to_rtp_buf(out, cnt, sizeof(unsigned char));
	// add raw data
	h265_add_to_rtp_buf(out, d, wrlen);
#ifdef DEBUG_SINGLE_NAL_OUT_FILE
	sprintf(dsnal_name, "/tmp/nfs/d/Leo/My_Temp/H265_Stuff/tc_fac_snalu%d_%d_%d.265", getpid(), out->mpoll.idx, out->mpoll.pidx);
	dsnal = fopen(dsnal_name, "wb");
	printf("Use Single output %s\n", dsnal_name);
	fwrite(d, wrlen, 1, dsnal);
	fclose(dsnal);
#endif
	return H265_TRUE;
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
static int h265_parse_stapa_nalu_cnt_object(struct rtp_h265 *out, unsigned char num)
{
	unsigned char *cnt;

	if ((out->iov_count + 1) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h265: STAPA CNT too many elements to send");
		return H265_FALSE;
	}
	cnt = h265_hmalloc(out, sizeof(unsigned char));
	if (!cnt) {
		spook_log(SL_WARN, "STAPA single NALU CNT allocate cnt err");
		return H265_FALSE;
	}
	*cnt = num;
	h265_add_to_rtp_buf(out, cnt, sizeof(unsigned char));

	return H265_TRUE;
}
/*
 *  h265_parse_stapa_nalu_object
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
static int h265_parse_stapa_nalu_object(struct rtp_h265 *out, unsigned char *d, int wrlen, unsigned char *payloadhdr, unsigned int hdrlen)
{
	unsigned char *hdr;

	if ((out->iov_count + 2) >= IOVSIZE) {
		spook_log(SL_WARN, "rtp-h265: STAPA too many elements to send");
		return H265_FALSE;
	}
	hdr = h265_hmalloc(out, sizeof(unsigned char) * hdrlen);
	if (!hdr) {
		spook_log(SL_WARN, "single NALU HDR allocate hdr err");
		return H265_FALSE;
	}
	// add header
	memcpy(hdr, payloadhdr, sizeof(unsigned char)*hdrlen);
	// add raw data
	h265_add_to_rtp_buf(out, d,  wrlen);
#ifdef DEBUG_STAPA_OUT_FILE
	sprintf(dstapa_name, "/tmp/nfs/d/Leo/My_Temp/H265_Stuff/tc_fac_stapa%d_%d_%d.265", getpid(), out->mpoll.idx, out->mpoll->pidx);
	dstapa = fopen(dstapa_name, "wb");
	printf("Use STAPA output %s\n", dstapa_name);
	fwrite(payloadhdr, hdrlen, 1, dstapa);
	fwrite(d, wrlen, 1, dstapa);
#endif
	return H265_TRUE;
}

static void h265_finish_stapa_nalu_object(struct rtp_h265 *out)
{
#ifdef DEBUG_STAPA_OUT_FILE
	fclose(dstapa);
#endif
}
///////////////////////////////////////////////////////////////
///////////////////// SLICE FUNCTION //////////////////////////
///////////////////////////////////////////////////////////////

int h265_u_v(int LenInBits, H265Bitstream *bitstream)
{
	SyntaxElement symbol, *sym = &symbol;

	//sym->type = SE_HEADER;
	sym->mapping = linfo_ue;   // Mapping rule
	sym->len = LenInBits;
	h265_readSyntaxElement_FLC(sym, bitstream);
	//UsedBits+=sym->len;
	return sym->inf;
};

int h265_ue_v(H265Bitstream *bitstream)
{
	SyntaxElement symbol, *sym = &symbol;

	sym->mapping = linfo_ue;   // Mapping rule
	h265_readSyntaxElement_VLC(sym, bitstream);
	//UsedBits+=sym->len;
	return sym->value1;
}

int h265_se_v(H265Bitstream *bitstream)
{
	SyntaxElement symbol, *sym = &symbol;

	sym->mapping = linfo_se;   // Mapping rule: signed integer
	h265_readSyntaxElement_VLC(sym, bitstream);
	//UsedBits+=sym->len;
	return sym->value1;
}

int h265_readSyntaxElement_FLC(SyntaxElement *sym, H265Bitstream *currStream)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	unsigned char *buf = currStream->streamBuffer;
	//int BitstreamLengthInBytes = currStream->bitstream_length;

	//if ((GetBits(buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes, sym->len)) < 0)
	//return -1;
	H265GetBits(buf, frame_bitoffset, &(sym->inf), sym->len);

	currStream->frame_bitoffset += sym->len; // move bitstream pointer
	sym->value1 = sym->inf;

	return 1;
}

int h265_readSyntaxElement_VLC(SyntaxElement *sym, H265Bitstream *currStream)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	unsigned char *buf = currStream->streamBuffer;

	sym->len =  h265_GetVLCSymbol(buf, frame_bitoffset, &(sym->inf));
	currStream->frame_bitoffset += sym->len;
	sym->mapping(sym->len, sym->inf, &(sym->value1), &(sym->value2));

	return 1;
}



extern void linfo_ue(int len, int info, int *value1, int *dummy);
extern void linfo_se(int len,  int info, int *value1, int *dummy);

int H265GetBits(unsigned char buffer[], int totbitoffset, int *info, int numbits)
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


int h265_GetVLCSymbol(unsigned char *buffer, int totbitoffset, int *info)
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

///////////////////////////////////////////////////////////////
///////////////////// NALU FUNCTION //////////////////////////
///////////////////////////////////////////////////////////////
/*
 *  h265_get_sample_nal_hdr
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
static unsigned char h265_get_nal_hdr(unsigned char *pBuffer, unsigned int offset)
{
	return pBuffer[offset] ;
}
/*
  * h265_header_parsing
  * brief/
  * 1. check IDP or non_IDP
  * 2. check SPS
  * 3. check PPS
  * 4. check VPS
  */
static int h265_header_parsing(
	struct rtp_h265 *out,
	unsigned char hdr,
	unsigned char *str,
	unsigned int str_len)
{
	int ret = 0;

	unsigned char type = Get_Type(hdr);
	switch (type) {
	case NAL_UNIT_CODED_SLICE_IDR:
		//printf("NALu_TYPE_IDR\n");
		out->keyframe = 1;
		break;
	case NAL_UNIT_VPS:
		if (!h265_add_to_vps(out, str, str_len)) {
			printf("add vps err\n");
		}
		break;
	case NAL_UNIT_SPS:
		if (!h265_add_to_sps(out, str, str_len)) {
			printf("add sps err\n");
		}
		break;
	case NAL_UNIT_PPS:
		if (!h265_add_to_pps(out, str, str_len)) {
			printf("add pps err\n");
		}
		h265_finish_sdp(out);
		out->vop_time_increment_resolution = 10;
		out->vop_time_increment = 0;
		out->init_done = 1;
		break;
	case NAL_UNIT_CODED_SLICE_TRAIL_R:
		//printf("NALu_TYPE_nonIDR\n");
		out->keyframe = 0;
		break;
	case NAL_UNIT_SEI:
		//printf("NALu_TYPE_SEI: GM is not support\n");
		//out->init_done = 0;
		//ret = -1;
		break;
	default:
		printf("NALu_TYPE: %x is not support\n", type);
		out->init_done = 0;
		ret = -1;
		break;
	}
	return ret;
}
/*
 *  h265_process_frame
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
static int h265_process_frame(struct frame *f, void *d)
{
	struct rtp_h265 *out = (struct rtp_h265 *)d;
	unsigned int remaining = f->length;
	unsigned int offset = 0 ;

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
	sprintf(dbs_name, "/tmp/nfs/d/Leo/My_Temp/H265_Stuff/tc_fac_bs%d_%d.265", getpid(), out->mpoll.idx);
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
		unsigned int first_nal = h265_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);
		if (first_nal + LENGTH_OF_NALUSIZE == f->length) {
			// we have a single nal, less than the maxPayloadSize,
			// so, we have Single Nal unit mode
			unsigned char hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
			if (h265_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, first_nal) < 0) {
				return out->init_done;
			}
			offset += LENGTH_OF_NALUSIZE;
			remaining -= LENGTH_OF_NALUSIZE;

			h265_parse_single_nalu_object(out, f->d + offset, first_nal);
#ifdef H265_DEBUG_HINT
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
		unsigned int nal_size = h265_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);

		if (nal_size > rtp_mtu) {
			// FU_A RTP packet
			unsigned char hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
			unsigned char fu_header[3];
			unsigned int write_size;

			if (h265_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
				return out->init_done;
			}
			offset += LENGTH_OF_NALUSIZE;
			remaining -= LENGTH_OF_NALUSIZE;
			/*
			 create the HEVC payload header and transmit the buffer as fragmentation units (FU)

			 0                   1
			 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
			 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 |F|   Type    |  LayerId  | TID |
			 +-------------+-----------------+

			 F       = 0
			 Type    = 49 (fragmentation unit (FU))
			 LayerId = 0
			 TID     = 1
			 */
			fu_header[0] = 49 << 1;
			fu_header[1] = 1;
			/*
			 create the FU header

			 0 1 2 3 4 5 6 7
			 +-+-+-+-+-+-+-+-+
			 |S|E|  FuType   |
			 +---------------+

			 S       = variable
			 E       = variable
			 FuType  = NAL unit type
			 */
			fu_header[2] = Get_Type(hdr);
			// set start bit |FU-A type
			fu_header[2] |= 1 << 7;
			// NALU type is not included in FU-A
			//h265 nalu header are 2-bytes
			offset += 2;
			remaining -= 2;
			nal_size -= 2;
			while (nal_size > 0) {
				if ((nal_size + 3) <= rtp_mtu) {
					/* set the E bit: mark as last fragment */
					fu_header[2] |= 1 << 6;
					write_size = nal_size;
				} else {
					write_size = rtp_mtu - 3;
				}

				h265_parse_fu_nalu_object(out, f->d + offset, write_size, fu_header, 3);
#ifdef H265_DEBUG_HINT
				printf("2. FU h 0x%x s %d p %d\n", hdr, remaining, write_size);
#endif

				/* reset the S bit */
				fu_header[2] &= ~(1 << 7);
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
				unsigned int next_nal_size = h265_get_nal_size(f->d + next_size_offset, remaining - next_size_offset, LENGTH_OF_NALUSIZE);
				if (((next_nal_size + 2)
					 + (nal_size + 2)
					 + 1/* STAP-A NAL HDR*/) <= rtp_mtu) {
					have_stap = 1;
				}
			}

			if (have_stap == 0) {
				// we have to fit this nal into a packet - the next one is too big
				unsigned char hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);

				if (h265_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
					return out->init_done;
				}
				offset += LENGTH_OF_NALUSIZE;
				remaining -= LENGTH_OF_NALUSIZE;

				h265_parse_single_nalu_object(out, f->d + offset, nal_size);
#ifdef H265_DEBUG_HINT
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
				unsigned char data[4];
				unsigned char cnt = 1;

				hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
				max_nri = Get_Nri(hdr);
				{
					unsigned int nri_nal_size = nal_size + 1 + 2;
					unsigned int nri_bytes_in_stap = nal_size + 1 + 2;
					unsigned int nri_remaining = remaining - next_size_offset;
					unsigned int nri_offset = next_size_offset;
					unsigned char nri;

					nri_nal_size = h265_get_nal_size(f->d + nri_offset, nri_remaining, LENGTH_OF_NALUSIZE);
					while (((nri_bytes_in_stap + nri_nal_size + 2) <= rtp_mtu) && (nri_remaining > 0)) {
						nri_offset += nri_nal_size + LENGTH_OF_NALUSIZE;
						nri_remaining -= nri_nal_size + LENGTH_OF_NALUSIZE;
						nri_bytes_in_stap += nri_nal_size + 2;
						cnt++;
						hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
						nri = Get_Nri(hdr);
						//printf("NRI max %d cur %d\n", max_nri, nri);
						if (nri > max_nri) {
							max_nri = nri;
						}
						if (nri_remaining > 0) {
							nri_nal_size = h265_get_nal_size(f->d + nri_offset, nri_remaining, LENGTH_OF_NALUSIZE);
						}
					}
				}

				hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
				if (h265_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
					return out->init_done;
				}

				if (next_size_offset >= (offset +  remaining)
					&&  bytes_in_stap <= rtp_mtu) {
					// stap is last frame
					last = 1;
				} else {
					last = 0;
				}

				data[0] = 48 << 1 ; //(hdr & 0x80) |max_nri | 24; // nri | STAPA_TYPE
				data[1] = 1; //
				data[2] = (unsigned char)((nal_size >> 8) & 0xFF);
				data[3] = (unsigned char)(nal_size & 0xFF);
				offset += LENGTH_OF_NALUSIZE;
				remaining -= LENGTH_OF_NALUSIZE;

				//h265_parse_stapa_nalu_cnt_object(out, cnt*2);
				h265_parse_stapa_nalu_cnt_object(out, 1);
				h265_parse_stapa_nalu_object(out, f->d + offset, nal_size, data, 4);
#ifdef H265_DEBUG_HINT
				printf("4. STAP h 0x%x s %d p %d\n", hdr, remaining, nal_size);
#endif

				offset += nal_size;
				remaining -= nal_size;
				bytes_in_stap = /*1 + 2 +*/ nal_size;
				nal_size = h265_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);
				while (((bytes_in_stap + nal_size + LENGTH_OF_NALUSIZE) <= rtp_mtu) && (remaining > 0)) {
					data[0] = (unsigned char)((nal_size >> 8) & 0xFF);
					data[1] = (unsigned char)(nal_size & 0xFF);

					hdr = h265_get_nal_hdr(f->d + offset, LENGTH_OF_NALUSIZE);
					if (h265_header_parsing(out, hdr, f->d + offset + LENGTH_OF_NALUSIZE, nal_size) < 0) {
						return out->init_done;
					}

					offset += LENGTH_OF_NALUSIZE;
					remaining -= LENGTH_OF_NALUSIZE;
					h265_parse_stapa_nalu_cnt_object(out, 1);
					h265_parse_stapa_nalu_object(out, f->d + offset, nal_size, data, 2);
#ifdef H265_DEBUG_HINT
					printf("5. STAP h 0x%x s %d p %d\n", hdr, remaining, nal_size);
#endif

					offset += nal_size;
					remaining -= nal_size;
					bytes_in_stap += nal_size /*+ 2*/;
					if (remaining > 0) {
						nal_size = h265_get_nal_size(f->d + offset, remaining, LENGTH_OF_NALUSIZE);
					}
				} // end while stap
				h265_finish_stapa_nalu_object(out);
			} // end have stap
		} // end check size
	}
	return out->init_done;
}

static int h265_get_sdp(char *dest, int len, int payload, int port, void *d)
{
	dbg("%s should NOT be called.\n", __func__);
	return 0;
}

/*
 *  h265_send
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
static int h265_send(struct rtp_endpoint *ep, void *d, unsigned int curr_time, unsigned int *delay)
{
	struct rtp_h265 *out = (struct rtp_h265 *)d;
	unsigned char cnt;
	int i, j;
	struct iovec v[IOVSIZE];

#ifdef DEBUG_SEND_DATA
	int index = 0;
	while (index < out->iov_count) {
		sprintf(iov_name, "/tmp/nfs/d/Leo/My_Temp/H265_Stuff/tc_iov_bf%d_%d_%d.265", getpid(), mpoll.idx, index);
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
#ifdef H265_DEBUG_HINT
			printf("1. h265_send total %d idx %d cnt %d\n", out->iov_count, i, cnt);
#endif
			if (send_rtp_packet(ep, v, cnt + 1, ep->last_timestamp, 0) < 0) {
				return -1;
			}
		} else {
#ifdef H265_DEBUG_HINT
			printf("2. h265_send total %d idx %d cnt %d\n", out->iov_count, i, cnt);
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
static int h265_get_payload(int payload, void *d)
{
	return 96;
}

int h265_release(void *p)
{
	struct rtp_h265 *rtp = (struct rtp_h265 *)p;
	if (rtp == NULL) {
		return -1;
	}
	if (rtp->mpoll.base) {
		free(rtp->mpoll.base);
	}
	free(rtp);
	return 0;
}


struct rtp_media *new_rtp_media_h265(struct stream *stream)
{
	struct rtp_media    *m;     // Carl
	struct rtp_h265 *out;

	out = (struct rtp_h265 *)malloc(sizeof(struct rtp_h265));
	out->iov_count = 0;
	out->vps_len = 0;
	out->sps_len = 0;
	out->pps_len = 0;
	out->config_len = 0;
	out->init_done = 1;     // Carl
	out->pali = 0;
	out->timestamp = 0;

#ifdef USED_MEM_POLL
	if (!h265_hinit(out, MEM_POLL_SIZE)) {
		printf("######MEM POLL ALLOCATE FAIL######\n");
	}
#endif

	// Carl
	m = new_rtp_media(h265_get_sdp, h265_get_payload, h265_process_frame, h265_send, h265_release, out, 90000);
	if (m == NULL) {
		free(out);
	}

	return m;
}


