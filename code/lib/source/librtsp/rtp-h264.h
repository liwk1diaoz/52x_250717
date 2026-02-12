/**
 *  \file H264Parser.h
 *
 *  H264 parser functions.
 *
 */
#ifndef _H264Parser_H_
#define _H264Parser_H_

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

typedef struct h264bitstream {
	// UVLC Decoding
	int           frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, UVLC only
	unsigned char  *streamBuffer;      //!< actual codebuffer for read bytes
} H264Bitstream;


int h264_u_v(int LenInBits, H264Bitstream *bitstream);
int h264_ue_v(H264Bitstream *bitstream);
int h264_se_v(H264Bitstream *bitstream);

//read FLC codeword from UVLC-partition
int readSyntaxElement_FLC(SyntaxElement *sym, H264Bitstream *currStream);
int readSyntaxElement_VLC(SyntaxElement *sym, H264Bitstream *currStream);

// mapping function
void linfo_ue(int len, int info, int *value1, int *dummy);
void linfo_se(int len,  int info, int *value1, int *dummy);

// to get specified bits
int H264GetBits(unsigned char buffer[], int totbitoffset, int *info, int numbits);
int GetVLCSymbol(unsigned char buffer[], int totbitoffset, int *info);

#endif

