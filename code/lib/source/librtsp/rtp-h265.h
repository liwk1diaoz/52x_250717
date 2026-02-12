/**
 *  \file H265Parser.h
 *
 *  H265 parser functions.
 *
 */
#ifndef _H265Parser_H_
#define _H265Parser_H_

typedef struct syntaxelement {
	int           value1;                //!< numerical value of syntax element
	int           value2;                //!< for blocked symbols, e.g. run/level
	int           len;                   //!< length of code
	int           inf;                   //!< info part of UVLC code
	unsigned int  bitpattern;            //!< UVLC bitpattern

	//! for mapping of UVLC to syntaxElement
	void (*mapping)(int len, int info, int *value1, int *value2);
	//! used for CABAC: refers to actual coding method of each individual syntax element type
	//void  (*reading)(struct syntaxelement *, struct inp_par *, struct img_par *, DecodingEnvironmentPtr);
} SyntaxElement;

typedef struct h265bitstream {
	// UVLC Decoding
	int           frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, UVLC only
	unsigned char  *streamBuffer;      //!< actual codebuffer for read bytes
} H265Bitstream;


int h264_u_v(int LenInBits, H265Bitstream *bitstream);
int h264_ue_v(H265Bitstream *bitstream);
int h264_se_v(H265Bitstream *bitstream);

//read FLC codeword from UVLC-partition
int h265_readSyntaxElement_FLC(SyntaxElement *sym, H265Bitstream *currStream);
int h265_readSyntaxElement_VLC(SyntaxElement *sym, H265Bitstream *currStream);

// mapping function
void linfo_ue(int len, int info, int *value1, int *dummy);
void linfo_se(int len,  int info, int *value1, int *dummy);

// to get specified bits
int H265GetBits(unsigned char buffer[], int totbitoffset, int *info, int numbits);
int h265_GetVLCSymbol(unsigned char buffer[], int totbitoffset, int *info);

#endif

