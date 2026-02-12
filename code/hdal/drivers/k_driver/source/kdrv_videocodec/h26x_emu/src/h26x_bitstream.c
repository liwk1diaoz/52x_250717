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

const UINT32 crc32Table[256] = {
	0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L,
	0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
	0x8AD958CFL, 0x78B2DBCCL, 0x6BE22838L, 0x9989AB3BL,
	0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L, 0x5E133C24L,
	0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL,
	0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L,
	0x9A879FA0L, 0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L,
	0x5D1D08BFL, 0xAF768BBCL, 0xBC267848L, 0x4E4DFB4BL,
	0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L, 0x33ED7D2AL,
	0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
	0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L,
	0x6DFE410EL, 0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL,
	0x30E349B1L, 0xC288CAB2L, 0xD1D83946L, 0x23B3BA45L,
	0xF779DEAEL, 0x05125DADL, 0x1642AE59L, 0xE4292D5AL,
	0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL,
	0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L,
	0x417B1DBCL, 0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L,
	0x86E18AA3L, 0x748A09A0L, 0x67DAFA54L, 0x95B17957L,
	0xCBA24573L, 0x39C9C670L, 0x2A993584L, 0xD8F2B687L,
	0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
	0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L,
	0x96BF4DCCL, 0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L,
	0xDBFC821CL, 0x2997011FL, 0x3AC7F2EBL, 0xC8AC71E8L,
	0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L, 0x0F36E6F7L,
	0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L,
	0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L,
	0xEB1FCBADL, 0x197448AEL, 0x0A24BB5AL, 0xF84F3859L,
	0x2C855CB2L, 0xDEEEDFB1L, 0xCDBE2C45L, 0x3FD5AF46L,
	0x7198540DL, 0x83F3D70EL, 0x90A324FAL, 0x62C8A7F9L,
	0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
	0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L,
	0x3CDB9BDDL, 0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L,
	0x82F63B78L, 0x709DB87BL, 0x63CD4B8FL, 0x91A6C88CL,
	0x456CAC67L, 0xB7072F64L, 0xA457DC90L, 0x563C5F93L,
	0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L,
	0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL,
	0x92A8FC17L, 0x60C37F14L, 0x73938CE0L, 0x81F80FE3L,
	0x55326B08L, 0xA759E80BL, 0xB4091BFFL, 0x466298FCL,
	0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL, 0x0B21572CL,
	0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
	0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L,
	0x65D122B9L, 0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL,
	0x2892ED69L, 0xDAF96E6AL, 0xC9A99D9EL, 0x3BC21E9DL,
	0xEF087A76L, 0x1D63F975L, 0x0E330A81L, 0xFC588982L,
	0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL,
	0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L,
	0x38CC2A06L, 0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L,
	0xFF56BD19L, 0x0D3D3E1AL, 0x1E6DCDEEL, 0xEC064EEDL,
	0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L, 0xD0DDD530L,
	0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
	0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL,
	0x8ECEE914L, 0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L,
	0xD3D3E1ABL, 0x21B862A8L, 0x32E8915CL, 0xC083125FL,
	0x144976B4L, 0xE622F5B7L, 0xF5720643L, 0x07198540L,
	0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L,
	0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL,
	0xE330A81AL, 0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL,
	0x24AA3F05L, 0xD6C1BC06L, 0xC5914FF2L, 0x37FACCF1L,
	0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L, 0x7AB90321L,
	0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
	0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L,
	0x34F4F86AL, 0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL,
	0x79B737BAL, 0x8BDCB4B9L, 0x988C474DL, 0x6AE7C44EL,
	0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L, 0xAD7D5351L
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

UINT32 h26x_crc32(UINT32 crc, UINT8 val)
{
    crc = crc32Table[(crc ^ val) & 0xff] ^ (crc >> 8);
	return crc;
}

