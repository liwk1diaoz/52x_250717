/* vi: set sw=4 ts=4: */
/*
 * Small lzma deflate implementation for embedded systems (https://github.com/j-d-r/unlzma_tiny)
 * Copyright (C) 2018  Julien Dusser <julien.dusser@free.fr>
 *
 * Based on decompress_unlzma.c from busybox
 * Copyright (C) 2006  Aurelien Jacobs <aurel@gnuage.org>
 *
 * Based on LzmaDecode.c from the LZMA SDK 4.22 (http://www.7-zip.org/)
 * Copyright (C) 1999-2005  Igor Pavlov
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#if defined(__KERNEL__)
#include <asm/io.h>
#include <asm/div64.h>
#include <asm/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#endif
#include <kwrap/nvt_type.h>
#include <unlzma.h>

#if defined(__KERNEL__)
#define PRINTF printk
#define MALLOC(x) kmalloc((x), GFP_ATOMIC)
#define FREE(x) kfree((x))
typedef UINT32 size_t;
#else
#define PRINTF printf
#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

typedef UINT8 u_int8_t;
typedef UINT32 uint32_t;
typedef INT32 int32_t;

#define ALWAYS_INLINE __attribute__ ((always_inline)) inline
#define FAST_FUNC

#define speed_inline

typedef struct {
	uint8_t *ptr;
	uint8_t *buffer_end;
	uint32_t code;
	uint32_t range;
	uint32_t bound;
} rc_t;

#define RC_TOP_BITS 24
#define RC_MOVE_BITS 5
#define RC_MODEL_TOTAL_BITS 11

# define alloc_probs(size) MALLOC(size)
# define free_probs(ptr) FREE(ptr)

/* Called twice, but one callsite is in speed_inline'd rc_is_bit_1() */
static void rc_do_normalize(rc_t *rc)
{
	if (rc->ptr >= rc->buffer_end) {
		//TODO: find a way to exit and free probes
		PRINTF("die: truncated input");
	}
	rc->range <<= 8;
	rc->code = (rc->code << 8) | *rc->ptr++;
}
static ALWAYS_INLINE void rc_normalize(rc_t *rc)
{
	if (rc->range < (1 << RC_TOP_BITS)) {
		rc_do_normalize(rc);
	}
}

/* Called once */
static ALWAYS_INLINE void rc_init(rc_t* rc, uint8_t *ptr, size_t size)
{
	int i;

	rc->ptr = ptr;
	rc->buffer_end = ptr + size;
	rc->range = 0;
	rc->code = 0;
	rc->bound = 0;

	for (i = 0; i < 5; i++) {
		rc_do_normalize(rc);
	}
	rc->range = 0xffffffff;
}

/* rc_is_bit_1 is called 9 times */
static speed_inline int rc_is_bit_1(rc_t *rc, uint16_t *p)
{
	rc_normalize(rc);
	rc->bound = *p * (rc->range >> RC_MODEL_TOTAL_BITS);
	if (rc->code < rc->bound) {
		rc->range = rc->bound;
		*p += ((1 << RC_MODEL_TOTAL_BITS) - *p) >> RC_MOVE_BITS;
		return 0;
	}
	rc->range -= rc->bound;
	rc->code -= rc->bound;
	*p -= *p >> RC_MOVE_BITS;
	return 1;
}

/* Called 4 times in unlzma loop */
static ALWAYS_INLINE int rc_get_bit(rc_t *rc, uint16_t *p, int *symbol)
{
	int ret = rc_is_bit_1(rc, p);
	*symbol = *symbol * 2 + ret;
	return ret;
}

/* Called once */
static ALWAYS_INLINE int rc_direct_bit(rc_t *rc)
{
	rc_normalize(rc);
	rc->range >>= 1;
	if (rc->code >= rc->range) {
		rc->code -= rc->range;
		return 1;
	}
	return 0;
}

/* Called twice */
static speed_inline void
rc_bit_tree_decode(rc_t *rc, uint16_t *p, int num_levels, int *symbol)
{
	int i = num_levels;

	*symbol = 1;
	while (i--)
		rc_get_bit(rc, p + *symbol, symbol);
	*symbol -= 1 << num_levels;
}


typedef struct {
	uint8_t pos;
	uint32_t dict_size;
	uint64_t dst_size;
} lzma_header_t;


/* #defines will force compiler to compute/optimize each one with each usage.
 * Have heart and use enum instead. */
enum {
	LZMA_BASE_SIZE = 1846,
	LZMA_LIT_SIZE  = 768,

	LZMA_NUM_POS_BITS_MAX = 4,

	LZMA_LEN_NUM_LOW_BITS  = 3,
	LZMA_LEN_NUM_MID_BITS  = 3,
	LZMA_LEN_NUM_HIGH_BITS = 8,

	LZMA_LEN_CHOICE     = 0,
	LZMA_LEN_CHOICE_2   = (LZMA_LEN_CHOICE + 1),
	LZMA_LEN_LOW        = (LZMA_LEN_CHOICE_2 + 1),
	LZMA_LEN_MID        = (LZMA_LEN_LOW \
	                      + (1 << (LZMA_NUM_POS_BITS_MAX + LZMA_LEN_NUM_LOW_BITS))),
	LZMA_LEN_HIGH       = (LZMA_LEN_MID \
	                      + (1 << (LZMA_NUM_POS_BITS_MAX + LZMA_LEN_NUM_MID_BITS))),
	LZMA_NUM_LEN_PROBS  = (LZMA_LEN_HIGH + (1 << LZMA_LEN_NUM_HIGH_BITS)),

	LZMA_NUM_STATES     = 12,
	LZMA_NUM_LIT_STATES = 7,

	LZMA_START_POS_MODEL_INDEX = 4,
	LZMA_END_POS_MODEL_INDEX   = 14,
	LZMA_NUM_FULL_DISTANCES    = (1 << (LZMA_END_POS_MODEL_INDEX >> 1)),

	LZMA_NUM_POS_SLOT_BITS = 6,
	LZMA_NUM_LEN_TO_POS_STATES = 4,

	LZMA_NUM_ALIGN_BITS = 4,

	LZMA_MATCH_MIN_LEN  = 2,

	LZMA_IS_MATCH       = 0,
	LZMA_IS_REP         = (LZMA_IS_MATCH + (LZMA_NUM_STATES << LZMA_NUM_POS_BITS_MAX)),
	LZMA_IS_REP_G0      = (LZMA_IS_REP + LZMA_NUM_STATES),
	LZMA_IS_REP_G1      = (LZMA_IS_REP_G0 + LZMA_NUM_STATES),
	LZMA_IS_REP_G2      = (LZMA_IS_REP_G1 + LZMA_NUM_STATES),
	LZMA_IS_REP_0_LONG  = (LZMA_IS_REP_G2 + LZMA_NUM_STATES),
	LZMA_POS_SLOT       = (LZMA_IS_REP_0_LONG \
	                      + (LZMA_NUM_STATES << LZMA_NUM_POS_BITS_MAX)),
	LZMA_SPEC_POS       = (LZMA_POS_SLOT \
	                      + (LZMA_NUM_LEN_TO_POS_STATES << LZMA_NUM_POS_SLOT_BITS)),
	LZMA_ALIGN          = (LZMA_SPEC_POS \
	                      + LZMA_NUM_FULL_DISTANCES - LZMA_END_POS_MODEL_INDEX),
	LZMA_LEN_CODER      = (LZMA_ALIGN + (1 << LZMA_NUM_ALIGN_BITS)),
	LZMA_REP_LEN_CODER  = (LZMA_LEN_CODER + LZMA_NUM_LEN_PROBS),
	LZMA_LITERAL        = (LZMA_REP_LEN_CODER + LZMA_NUM_LEN_PROBS),
};

#define CHKPNT    PRINTF("CHK: %d",__LINE__)
#define LZMA_PROPS_SIZE (5)
#define LZMA_SIZE_OFFSET       (LZMA_PROPS_SIZE)
#define LZMA_DATA_OFFSET       (LZMA_SIZE_OFFSET+sizeof(unsigned long long))

int lzma_inflate(UINT8 *in_ptr, UINT32 in_size, UINT8 *out_ptr, UINT32  out_size)
{
	uint32_t dict_size;
	size_t dst_size, dst_size_high;
	int lc, pb, lp;
	uint32_t pos_state_mask;
	uint32_t literal_pos_mask;
	uint16_t *p = NULL;
	rc_t private_rc;
	rc_t *rc = &private_rc;
	int i;
	uint8_t previous_byte = 0;
	size_t buffer_pos = 0;
	int len = 0;
	int state = 0;
	uint32_t rep0 = 1, rep1 = 1, rep2 = 1, rep3 = 1;
	uint8_t pos = 0;
	uint8_t lzmaprob[LZMA_DATA_OFFSET]; /// for sram access with byte or operate, needing copy to dram first

	memcpy(lzmaprob, in_ptr, LZMA_DATA_OFFSET);

	if(in_size < sizeof(lzma_header_t)) {
		PRINTF("wrn: too short lzma file");
		goto bad;
	}

	pos = in_ptr[0];

	if (pos >= (9 * 5 * 5)) {
		PRINTF("wrn: bad lzma header");
		goto bad;
	}

	i = pos / 9;
	lc = pos % 9;
	pb = i / 5;
	lp = i % 5;
	pos_state_mask = (1 << pb) - 1;
	literal_pos_mask = (1 << lp) - 1;
	/* unaligned access should be handled as struct is packed */
	dict_size = lzmaprob[1] | ((unsigned int)lzmaprob[2] << 8) | ((unsigned int)lzmaprob[3] << 16) | ((unsigned int)lzmaprob[4] << 24);
	dst_size = 0;
	dst_size_high = 0;
	/* Read the uncompressed size */
	for (i = 0; i < 8; i++) {
		unsigned char b = lzmaprob[LZMA_SIZE_OFFSET + i];
		if (i < 4) {
			dst_size += (unsigned int)(b) << (i * 8);
		} else {
			dst_size_high += (unsigned int)(b) << ((i - 4) * 8);
		}
	}

	//debug_msg_var("lc",lc);
	//debug_msg_var("pb",pb);
	//debug_msg_var("lp",lp);
	//debug_msg_var("dst_size",dst_size);
	//debug_msg_var("dst_size_high",dst_size_high);
	//debug_msg_var("dict_size",dict_size);

	in_ptr += LZMA_DATA_OFFSET;
	in_size -= LZMA_DATA_OFFSET;

	if (dict_size == 0) {
		dict_size++;
	}

	{
		int num_probs;
		int probs_size;

		num_probs = LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp));
		probs_size = num_probs * sizeof(*p);
		p = alloc_probs(probs_size);
		//debug_msg_var("wrn: alloc size: ", probs_size);
		if (p == NULL) {
			PRINTF("die: tmp_size need: %d", probs_size);
			goto bad;
		}
		num_probs += LZMA_LITERAL - LZMA_BASE_SIZE;
		for (i = 0; i < num_probs; i++)
			p[i] = (1 << RC_MODEL_TOTAL_BITS) >> 1;
	}

	rc_init(rc, in_ptr, in_size); /*, RC_BUFFER_SIZE); */

	while (buffer_pos < dst_size) {
		int pos_state = buffer_pos & pos_state_mask;
		uint16_t *prob = p + LZMA_IS_MATCH + (state << LZMA_NUM_POS_BITS_MAX) + pos_state;

		if (!rc_is_bit_1(rc, prob)) {
			static const char next_state[LZMA_NUM_STATES] =
				{ 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };
			int mi = 1;

			prob = (p + LZMA_LITERAL
			        + (LZMA_LIT_SIZE * (((buffer_pos & literal_pos_mask) << lc)
			                            + (previous_byte >> (8 - lc))
			                           )
			          )
			);

			if (state >= LZMA_NUM_LIT_STATES) {
				int match_byte;
				uint32_t pos;

				pos = buffer_pos - rep0;
				if ((int32_t)pos < 0)
					pos += dict_size;
				match_byte = out_ptr[pos];
				do {
					int bit;

					match_byte <<= 1;
					bit = match_byte & 0x100;
					bit ^= (rc_get_bit(rc, prob + 0x100 + bit + mi, &mi) << 8); /* 0x100 or 0 */
					if (bit)
						break;
				} while (mi < 0x100);
			}
			while (mi < 0x100) {
				rc_get_bit(rc, prob + mi, &mi);
			}

			state = next_state[state];

			previous_byte = (uint8_t) mi;
			len = 1;
			goto one_byte2;
		} else {
			int num_bits;
			int offset;
			uint16_t *prob2;
#define prob_len prob2

			prob2 = p + LZMA_IS_REP + state;
			if (!rc_is_bit_1(rc, prob2)) {
				rep3 = rep2;
				rep2 = rep1;
				rep1 = rep0;
				state = state < LZMA_NUM_LIT_STATES ? 0 : 3;
				prob2 = p + LZMA_LEN_CODER;
			} else {
				prob2 += LZMA_IS_REP_G0 - LZMA_IS_REP;
				if (!rc_is_bit_1(rc, prob2)) {
					prob2 = (p + LZMA_IS_REP_0_LONG
					        + (state << LZMA_NUM_POS_BITS_MAX)
					        + pos_state
					);
					if (!rc_is_bit_1(rc, prob2)) {
						state = state < LZMA_NUM_LIT_STATES ? 9 : 11;
						len = 1;
						goto string;
					}
				} else {
					uint32_t distance;

					prob2 += LZMA_IS_REP_G1 - LZMA_IS_REP_G0;
					distance = rep1;
					if (rc_is_bit_1(rc, prob2)) {
						prob2 += LZMA_IS_REP_G2 - LZMA_IS_REP_G1;
						distance = rep2;
						if (rc_is_bit_1(rc, prob2)) {
							distance = rep3;
							rep3 = rep2;
						}
						rep2 = rep1;
					}
					rep1 = rep0;
					rep0 = distance;
				}
				state = state < LZMA_NUM_LIT_STATES ? 8 : 11;
				prob2 = p + LZMA_REP_LEN_CODER;
			}

			prob_len = prob2 + LZMA_LEN_CHOICE;
			num_bits = LZMA_LEN_NUM_LOW_BITS;
			if (!rc_is_bit_1(rc, prob_len)) {
				prob_len += LZMA_LEN_LOW - LZMA_LEN_CHOICE
				            + (pos_state << LZMA_LEN_NUM_LOW_BITS);
				offset = 0;
			} else {
				prob_len += LZMA_LEN_CHOICE_2 - LZMA_LEN_CHOICE;
				if (!rc_is_bit_1(rc, prob_len)) {
					prob_len += LZMA_LEN_MID - LZMA_LEN_CHOICE_2
					            + (pos_state << LZMA_LEN_NUM_MID_BITS);
					offset = 1 << LZMA_LEN_NUM_LOW_BITS;
					num_bits += LZMA_LEN_NUM_MID_BITS - LZMA_LEN_NUM_LOW_BITS;
				} else {
					prob_len += LZMA_LEN_HIGH - LZMA_LEN_CHOICE_2;
					offset = ((1 << LZMA_LEN_NUM_LOW_BITS)
					          + (1 << LZMA_LEN_NUM_MID_BITS));
					num_bits += LZMA_LEN_NUM_HIGH_BITS - LZMA_LEN_NUM_LOW_BITS;
				}
			}
			rc_bit_tree_decode(rc, prob_len, num_bits, &len);
			len += offset;

			if (state < 4) {
				int pos_slot;
				uint16_t *prob3;

				state += LZMA_NUM_LIT_STATES;
				prob3 = p + LZMA_POS_SLOT +
				       ((len < LZMA_NUM_LEN_TO_POS_STATES ? len :
				         LZMA_NUM_LEN_TO_POS_STATES - 1)
				         << LZMA_NUM_POS_SLOT_BITS);
				rc_bit_tree_decode(rc, prob3,
					LZMA_NUM_POS_SLOT_BITS, &pos_slot);
				rep0 = pos_slot;
				if (pos_slot >= LZMA_START_POS_MODEL_INDEX) {
					int i2, mi2, num_bits2 = (pos_slot >> 1) - 1;
					rep0 = 2 | (pos_slot & 1);
					if (pos_slot < LZMA_END_POS_MODEL_INDEX) {
						rep0 <<= num_bits2;
						prob3 = p + LZMA_SPEC_POS + rep0 - pos_slot - 1;
					} else {
						for (; num_bits2 != LZMA_NUM_ALIGN_BITS; num_bits2--)
							rep0 = (rep0 << 1) | rc_direct_bit(rc);
						rep0 <<= LZMA_NUM_ALIGN_BITS;
						// handle end of stream marker
						//if ((int32_t)rep0 < 0) {
						//	dbg("%d rep0:%d", __LINE__, rep0);

							//goto bad;
						//}
						prob3 = p + LZMA_ALIGN;
					}
					i2 = 1;
					mi2 = 1;
					while (num_bits2--) {
						if (rc_get_bit(rc, prob3 + mi2, &mi2))
							rep0 |= i2;
						i2 <<= 1;
					}
				}
				if (++rep0 == 0)
					break;
			}

			len += LZMA_MATCH_MIN_LEN;
			/*
			 * LZMA SDK has this optimized:
			 * it precalculates size and copies many bytes
			 * in a loop with simpler checks, a-la:
			 *	do
			 *		*(dest) = *(dest + ofs);
			 *	while (++dest != lim);
			 * and
			 *	do {
			 *		buffer[buffer_pos++] = buffer[pos];
			 *		if (++pos == dict_size)
			 *			pos = 0;
			 *	} while (--cur_len != 0);
			 * Our code is slower (more checks per byte copy):
			 */
	string:
			do {
				uint32_t pos = buffer_pos - rep0;
				if ((int32_t)pos < 0) {
					pos += dict_size;
					/* bug 10436 has an example file where this triggers: */
					//if ((int32_t)pos < 0)
					//	goto bad;
					/* more stringent test (see unzip_bad_lzma_1.zip): */
				}
				if (pos >= out_size) {
					PRINTF("wrn: too small output buffer: %d", (int)pos);
					goto bad;
				}

				previous_byte = out_ptr[pos];
	one_byte2:
				if (buffer_pos >= out_size) {
					if (buffer_pos > 0x1000) {
						// because uboot first part only decode some bytes
						// we dont wanna show it message
						PRINTF("wrn: too small output buffer");
					}
					goto bad;
				}
				out_ptr[buffer_pos++] = previous_byte;
				len--;
			} while (len != 0 && buffer_pos < dst_size);
		}
	}

	free_probs(p);
	return buffer_pos;

 bad:
	if (buffer_pos > 0x1000) {
		// because uboot first part only decode some bytes
		// we dont wanna show it message
		PRINTF("wrn: failed");
	}
	free_probs(p);
	return (size_t)-1;
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(lzma_inflate);
#endif