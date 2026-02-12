/* ///////////////////////////////////////////////////////////// */
/*   File: bitstream.c                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Apr/28/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 bitstream read/write module.                     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Copyright, 2005.                                            */
/*   NOVATEK MICROELECTRONICS CORP.                              */
/*   NO.1-2 Innovation Road 1, Hsinchu Science Park,             */
/*   HsinChu 300 Taiwan, R.O.C.                                  */
/*   E-mail : jerry_peng@novatek.com.tw                          */
/* ///////////////////////////////////////////////////////////// */
#include "h26x_bitstream.h"

const UINT8 postfix_mask[9] =
{
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

const UINT32 prefix_mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

#if 0
#pragma mark -
#endif


static void __inline put_word(bstream * const bs, const UINT32 value)
{
    UINT32 b   = value;
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(b);
#endif
    *bs->tail++ = b;
}

static void __inline bs_forward(bstream * const bs, const UINT32 bits)
{
    bs->offset += bits;

    if (bs->offset >= 32)
    {
        UINT32 b = bs->bufl;
#ifndef ARCH_IS_BIG_ENDIAN
        BSWAP(b);
#endif
       *bs->tail++  = b;
        bs->bufl    = 0;
        bs->offset -= 32;
    }
}

static __inline void put_bit(bstream * const bs, const UINT32 bit)
{
    if (bit) bs->bufl |= (0x80000000 >> bs->offset);

    bs_forward(bs, 1);
}


UINT32 put_bits(bstream * const bs, const UINT32 value, const UINT32 size)
{
    UINT32 shift = 32 - bs->offset - size;

    if (shift <= 32)
    {
        bs->bufl   |= value << shift;
        bs->offset += size;
        //bs_forward(bs, size);
    }
    else
    {
        shift     = (~shift)+1;
        put_word(bs, bs->bufl | ((UINT64)value>>shift));
        bs->offset= shift;
        bs->bufl  = value << (32-shift);
    }

    return size;
}


static __inline UINT32 show_bits(bstream * const bs, const UINT32 bits)
{
    return bs->bufl >> (32-bits);
}

static __inline UINT8* get_cur_bs_addr(bstream * const bs)
{
    UINT8 *cur_addr = (UINT8 *)bs->tail + ((bs->offset+7)>>3);
    return cur_addr;
}

static __inline void skip_nth_byte(bstream * const bs, const UINT32 bytes)
{
    UINT8 *cur_addr = get_cur_bs_addr(bs);
    INT32  resi;
    UINT32 tmp;

    cur_addr += bytes;
    resi      = (UINT32)cur_addr & 0x3;
    cur_addr  = (UINT8 *)((UINT32)cur_addr & 0xfffffffc);

    bs->tail  = (UINT32 *)cur_addr;
    tmp = *(UINT32 *) cur_addr;
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    bs->bufl = tmp;

    tmp = *((UINT32 *)cur_addr + 1);
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    bs->bufh = tmp;

    bs->offset= 0;

    if(resi)
    {
        UINT32 bits = resi << 3;
        bs->bufl = (bs->bufl<<bits) | (bs->bufh>>(32-bits));
        bs->bufh <<= bits;

        bs->offset += bits;
    }

}

//static __inline void skip_bits(bstream * const bs, const UINT32 bits)
static void skip_bits(bstream * const bs, const UINT32 bits)
{
    UINT32 sht;
    UINT32 uiLeftBits;

    // @chingho 03 10 2009
    // {
    uiLeftBits = bs->length - bs->uiCurrPos;
    if(bits > uiLeftBits)
    {
        // there is not enough bits to be read
        //DBG_ERR("skip_bits error : bits not enough to be read\r\n");
        return;
    }
    // }

    if(bits == 32)
    {
        bs->bufl = bs->bufh;
    }
    else
    {
        bs->bufl = (bs->bufl<<bits) | (bs->bufh>>(32-bits));
        bs->bufh <<= bits;
    }
    bs->offset += bits;
    bs->uiCurrPos += bits;// @chingho 03 10 2009

    if(bs->offset >= 32)
    {
        UINT32 tmp;

        tmp = *((UINT32 *)bs->tail + 2);
#ifndef ARCH_IS_BIG_ENDIAN
        BSWAP(tmp);
#endif
        bs->offset -= 32;
        sht = 32-bs->offset;
        bs->bufl |= (sht==32) ? 0 : tmp >> (sht);
        bs->bufh = (bs->offset==32) ? 0 : tmp << bs->offset;

        bs->tail++;
    }
}

UINT32 get_bits(bstream *const bs, const UINT32 bits)
{
    UINT32 value = show_bits(bs, bits);

    skip_bits(bs, bits);

    return value;
}

static __inline UINT32 show_nth_byte(bstream *const bs, const UINT32 byte)
{
    UINT32 value = 0, byte_len = ((UINT32)bs->tail - (UINT32)bs->start) + (bs->offset>>3) + byte;

    if(byte_len >= bs->length)
        ;//DBG_ERR("ERROR: show_nth_byte out of range.\n");
    else
    {
        value = bs->tail[byte];
#ifndef ARCH_IS_BIG_ENDIAN
        BSWAP(value);
#endif
        value = (value & 0xff000000) >> 24;
    }

    return value;
}

UINT32 read_uvlc_codeword(bstream *const bs)
{
    UINT32  codeword;
    INT32   leading_zeros = 0;
    INT32   info;
    INT32   bit;

    /* firstly, read in M leading zeros followed by 1 */
    do
    {
        bit = get_bits(bs, 1);

        if(leading_zeros == 0 && bit)
        {
            return 0;
        }

        leading_zeros++;

    }while(bit == 0);

    info = get_bits(bs, leading_zeros - 1);

    codeword = (1 << (leading_zeros-1)) + info - 1;

    return codeword;
}

INT32 read_signed_uvlc_codeword(bstream *const bs)
{
    INT32  codeword;
    UINT32 uvlc_code;
    INT32   sign;
    INT32   leading_zeros = 0;
    INT32   info;
    INT32   bit;

    do
    {
        bit = get_bits(bs, 1);

        if(leading_zeros == 0 && bit)
        {
            return 0;
        }

        leading_zeros++;

    }while(bit == 0);

    info = get_bits(bs, leading_zeros - 1);

    uvlc_code = (1 << (leading_zeros-1)) + info - 1;

    sign      = (uvlc_code % 2) ? 1 : -1;

    codeword  = sign * ((uvlc_code+1) >> 1);

    return codeword;
}

int read_rbsp_trailing_bits(bstream *const bs)
{
    UINT32 uiVal;

    get_bits(bs, 1);

    uiVal = bs->offset & 0x7;
    if(uiVal)
    {
        get_bits(bs, 8-uiVal);
    }
    return 0;
}


__inline int count_bits(UINT32 code)
{
    int nbits = 0, bin = (code+1)>>1;

    while (bin)
    {
        bin >>= 1;
        nbits++;
    }

    return nbits;
}


UINT32 write_uvlc_codeword(bstream *const bs, UINT32 value)
{
    INT32 idx, nbits = count_bits(value);
    UINT32 info;

    if(nbits)
    {
        info = value + 1 - (1 << nbits);

        /* first, write prefix */
        for (idx = 0 ; idx < nbits ; idx++)
        {
            put_bits(bs, 0, 1);
        }

        put_bits(bs, 1, 1);

        /* now, write info bits */
        for(idx = (1<<(nbits-1)) ; idx ; idx>>=1)
        {
            put_bits(bs, (idx & info) ? 1 : 0, 1);
        }
    }
    else
    {
        put_bits(bs, 1, 1);
    }

    return 0;

}


int write_signed_uvlc_codeword(bstream *const bs, int value)
{
    UINT32 pos, code = 0;

    if (value != 0)
    {
        pos = 1;
        if (value < 0) value = -value, pos = 0;
        code = (value<<1) - pos;
    }
    write_uvlc_codeword(bs, code);

    return 0;
}

int write_rbsp_trailing_bits(bstream *const bs)
{
    UINT32 uiVal;

    put_bits(bs, 1, 1);

    uiVal = bs->offset & 0x7;
    if(uiVal)
    {
        put_bits(bs, 0, 8-uiVal);
    }
    return 0;
}


void init_parse_bitstream(bstream * const bs, void * const bitbuffer, UINT32 length)
{
    UINT32 tmp;
    UINT32 uiBuf;

    // @chingho 04 02 2009
    bs->uiWordAlignedOffset = ((UINT32)(bitbuffer)) & 0x3;
    uiBuf = ((UINT32)(bitbuffer)) & 0xFFFFFFFC;

    bs->start = bs->tail = (UINT32 *)uiBuf;
    bs->buf   = (UINT8 *)uiBuf;

    tmp = *(UINT32 *) uiBuf;
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    bs->bufl = tmp;
    tmp = *((UINT32 *)uiBuf + 1);
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    bs->bufh = tmp;

    bs->uiCurrPos = 0;
    bs->offset    = 0;
    bs->length    = length*8;

    // @chingho 04 02 2009
    for(tmp = 0; tmp < bs->uiWordAlignedOffset; tmp++)
    {
        skip_bits(bs, 8);
    }
}

void init_pack_bitstream(bstream * const bs, void * const bitbuffer, UINT32 length)
{
    bs->start    = bs->tail = (UINT32 *) bitbuffer;
    bs->bufl     = bs->bufh = 0;
    bs->offset   = 0;
    bs->length   = length;
    bs->uiWordAlignedOffset = 0;
    bs->uiCurrPos = 0;
}

#if 0
#pragma mark -
#endif

UINT32 show_bits_bytealign(bstream * const bs, const UINT32 bits)
{
    UINT32 remainder = bs->offset % 8;
    UINT32 peek_bits = (8-remainder) + bits;
    UINT32 value     = show_bits(bs, peek_bits) & prefix_mask[bits];

    return value;
}

void move_bytealign_noforcestuffing(bstream * const bs)
{
    UINT32 remainder = bs->offset % 8;
    if (remainder)
        skip_bits(bs, 8 - remainder);
}

void move_nextbyte(bstream * const bs)
{
    /* Note: in MPEG-4, Byte-align operation always inserted    */
    /*       at least one bit.  That is, if remainder == 0, we  */
    /*       must insert 8 bits.                                */

    UINT32 remainder = bs->offset % 8;

    skip_bits(bs, 8 - remainder);
}


void bs_pad(bstream * const bs, const UINT32 stuff_bit)
{
    UINT32 remainder = bs->offset % 8;

    if(stuff_bit)
        put_bits(bs, prefix_mask[7 - remainder], 8-remainder);
    else //H263
    {
        //if(remainder)
            put_bits(bs, 0, 8-remainder);
    }
}

UINT32 bs_pad_ch(bstream * const bs, const UINT32 video_type)
{
    UINT32 len = 0;
    UINT32 remainder = bs->offset % 8;

    if(video_type == MPEG4)
        len = put_bits(bs, prefix_mask[7 - remainder], 8-remainder);
    else //H263
    {
        if(remainder)
            len = put_bits(bs, 0, 8-remainder);
    }
    return len;
}

UINT32 bs_byte_align_zero_padding(bstream * const bs)
{
    UINT32 len = 0;
    UINT32 remainder = bs->offset % 8;

    if(remainder)
        len = put_bits(bs, 0, 8-remainder);
    return len;
}

UINT32 bs_byte_length_p(bstream * const bs)
{
    UINT32 len = ((UINT32)bs->tail - (UINT32)bs->start) + CEILING_DIV(bs->offset, 8);

    // @chingho 04 02 2009
    len -= (bs->uiWordAlignedOffset);
    return len;
}

UINT32 bs_byte_length(bstream * const bs)
{
    UINT32 len = ((UINT32)bs->tail - (UINT32)bs->start) + CEILING_DIV(bs->offset, 8);
    //UINT32 remainder = bs->offset % 8;

    if(bs->offset)
    {
        UINT32 tmp = bs->bufl;
#ifndef ARCH_IS_BIG_ENDIAN
        BSWAP(tmp);
#endif
        *bs->tail = tmp;
    }

    // @chingho 04 02 2009
    len -= (bs->uiWordAlignedOffset);

    return len;
}

UINT32 byte_length(bstream * const bs)
{
    UINT32 byte_len = ((UINT32)bs->tail - (UINT32)bs->start) + (bs->offset>>3);
    return byte_len;
}

#if 0
static UINT32 end_bs(bstream * const bs)
{
    UINT32 byte_len = ((UINT32)bs->tail - (UINT32)bs->start) + (bs->offset>>3);
    return (byte_len >= bs->length);
}
#endif

UINT32 bit_length(bstream * const bs)
{
    UINT32 bit_len = (((UINT32) (bs->tail) - (UINT32) bs->start) << 3) + bs->offset;

    // @chingho 04 02 2009
    bit_len -= (bs->uiWordAlignedOffset << 3);

    return bit_len;
}

UINT32 bs_len(bstream * const bs, const UINT32 stuff_bit)
{
    UINT32 len = ((UINT32)bs->tail - (UINT32)bs->start) + CEILING_DIV(bs->offset, 8);
    UINT32 remainder = bs->offset % 8;

    if(remainder)
        bs_pad(bs, stuff_bit);

    if(bs->offset)
    {
        UINT32 tmp = bs->bufl;
#ifndef ARCH_IS_BIG_ENDIAN
        BSWAP(tmp);
#endif
        *bs->tail = tmp;
    }
    return len;
}


