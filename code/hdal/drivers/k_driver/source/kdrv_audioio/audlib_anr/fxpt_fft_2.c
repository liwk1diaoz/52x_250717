/*
Copyright (c) 2003-2010, Mark Borgerding

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "fxpt_fft_2.h"
#include "anr_err.h"
#ifdef __KERNEL__
#include "audlib_anr_dbg.h"
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#endif

extern int err_code;

#ifdef _WIN32
#define INLINE _inline
#else
#define INLINE inline
#endif

/* The guts header contains all the multiplication and addition macros that are defined for
 fixed or floating point complex numbers.  It also delares the kf_ internal functions.
 */

/*
    Froward FFT:
       cos:   A   B   -A   -B      sin: B   -A  -B   A
             __            __                    ____
         ......＼........／.....     ..........／....＼...
                 ＼____／              ＼____／

    Inverse FFT
       cos:   A   B   -A   -B      sin: -B   A  B   -A
             __            __            ____
            ...＼........／...      ...／....＼............
                 ＼____／                      ＼____／

    define    A    B
             __  |
            ...＼|....
                 |＼__
                 |
*/
//#define MAX_FFT_SIZE  (1024)

//#define FFT_TBL_32BIT
#ifdef FFT_TBL_32BIT
const int COS_TABLE_A[MAX_FFT_SIZE / 4] = {
	0x7fffffff, 0x7fff6215, 0x7ffd8859, 0x7ffa72d0,
	0x7ff62181, 0x7ff09476, 0x7fe9cbbe, 0x7fe1c76a,
	0x7fd8878c, 0x7fce0c3d, 0x7fc25595, 0x7fb563b1,
	0x7fa736b3, 0x7f97cebb, 0x7f872bf1, 0x7f754e7e,
	0x7f62368e, 0x7f4de44f, 0x7f3857f4, 0x7f2191b2,
	0x7f0991c2, 0x7ef0585e, 0x7ed5e5c5, 0x7eba3a38,
	0x7e9d55fb, 0x7e7f3955, 0x7e5fe492, 0x7e3f57fd,
	0x7e1d93e8, 0x7dfa98a6, 0x7dd6668d, 0x7db0fdf6,
	0x7d8a5f3e, 0x7d628ac4, 0x7d3980eb, 0x7d0f4217,
	0x7ce3ceb0, 0x7cb72723, 0x7c894bdc, 0x7c5a3d4e,
	0x7c29fbed, 0x7bf8882f, 0x7bc5e28e, 0x7b920b88,
	0x7b5d039c, 0x7b26cb4e, 0x7aef6322, 0x7ab6cba2,
	0x7a7d055a, 0x7a4210d7, 0x7a05eeac, 0x79c89f6c,
	0x798a23b0, 0x794a7c10, 0x7909a92b, 0x78c7aba0,
	0x78848412, 0x78403327, 0x77fab987, 0x77b417de,
	0x776c4eda, 0x77235f2c, 0x76d94987, 0x768e0ea4,
	0x7641af3b, 0x75f42c09, 0x75a585ce, 0x7555bd4a,
	0x7504d344, 0x74b2c882, 0x745f9dd0, 0x740b53f9,
	0x73b5ebd0, 0x735f6625, 0x7307c3cf, 0x72af05a5,
	0x72552c83, 0x71fa3947, 0x719e2cd1, 0x71410803,
	0x70e2cbc5, 0x708378fd, 0x70231098, 0x6fc19384,
	0x6f5f02b0, 0x6efb5f11, 0x6e96a99b, 0x6e30e348,
	0x6dca0d13, 0x6d6227f9, 0x6cf934fa, 0x6c8f351b,
	0x6c24295f, 0x6bb812d0, 0x6b4af277, 0x6adcc963,
	0x6a6d98a3, 0x69fd6149, 0x698c246b, 0x6919e31f,
	0x68a69e80, 0x683257aa, 0x67bd0fbb, 0x6746c7d6,
	0x66cf811f, 0x66573cba, 0x65ddfbd2, 0x6563bf91,
	0x64e88925, 0x646c59be, 0x63ef328e, 0x637114cb,
	0x62f201ab, 0x6271fa68, 0x61f1003e, 0x616f146a,
	0x60ec382f, 0x60686cce, 0x5fe3b38c, 0x5f5e0db2,
	0x5ed77c88, 0x5e50015c, 0x5dc79d7b, 0x5d3e5235,
	0x5cb420df, 0x5c290acb, 0x5b9d1152, 0x5b1035ce,
	0x5a827999, 0x59f3de11, 0x59646497, 0x58d40e8b,
	0x5842dd53, 0x57b0d255, 0x571deef8, 0x568a34a8,
	0x55f5a4d1, 0x556040e1, 0x54ca0a49, 0x5433027c,
	0x539b2aee, 0x53028517, 0x5269126d, 0x51ced46d,
	0x5133cc93, 0x5097fc5d, 0x4ffb654c, 0x4f5e08e2,
	0x4ebfe8a3, 0x4e210616, 0x4d8162c3, 0x4ce10033,
	0x4c3fdff2, 0x4b9e038f, 0x4afb6c97, 0x4a581c9c,
	0x49b41532, 0x490f57ed, 0x4869e664, 0x47c3c22e,
	0x471cece6, 0x46756827, 0x45cd358e, 0x452456bc,
	0x447acd4f, 0x43d09aec, 0x4325c134, 0x427a41cf,
	0x41ce1e64, 0x4121589a, 0x4073f21c, 0x3fc5ec97,
	0x3f1749b7, 0x3e680b2c, 0x3db832a5, 0x3d07c1d5,
	0x3c56ba6f, 0x3ba51e28, 0x3af2eeb6, 0x3a402dd1,
	0x398cdd31, 0x38d8fe92, 0x382493af, 0x376f9e45,
	0x36ba2013, 0x36041ad8, 0x354d9056, 0x3496824f,
	0x33def286, 0x3326e2c2, 0x326e54c7, 0x31b54a5d,
	0x30fbc54c, 0x3041c760, 0x2f875261, 0x2ecc681d,
	0x2e110a61, 0x2d553afb, 0x2c98fbba, 0x2bdc4e6e,
	0x2b1f34eb, 0x2a61b101, 0x29a3c484, 0x28e5714a,
	0x2826b927, 0x27679df3, 0x26a82185, 0x25e845b5,
	0x25280c5d, 0x24677757, 0x23a6887e, 0x22e541ae,
	0x2223a4c5, 0x2161b39f, 0x209f701c, 0x1fdcdc1a,
	0x1f19f97a, 0x1e56ca1d, 0x1d934fe5, 0x1ccf8cb2,
	0x1c0b826a, 0x1b4732ef, 0x1a82a025, 0x19bdcbf2,
	0x18f8b83c, 0x183366e8, 0x176dd9de, 0x16a81304,
	0x15e21444, 0x151bdf85, 0x145576b1, 0x138edbb0,
	0x12c8106e, 0x120116d4, 0x1139f0ce, 0x1072a047,
	0x0fab272b, 0x0ee38765, 0x0e1bc2e3, 0x0d53db92,
	0x0c8bd35d, 0x0bc3ac35, 0x0afb6805, 0x0a3308bc,
	0x096a9049, 0x08a2009a, 0x07d95b9e, 0x0710a344,
	0x0647d97c, 0x057f0034, 0x04b6195d, 0x03ed26e6,
	0x03242abe, 0x025b26d7, 0x01921d1f, 0x00c90f87
};

const int COS_TABLE_B[MAX_FFT_SIZE / 4] = {
	0x00000000, 0xff36f079, 0xfe6de2e1, 0xfda4d929,
	0xfcdbd542, 0xfc12d91a, 0xfb49e6a3, 0xfa80ffcc,
	0xf9b82684, 0xf8ef5cbc, 0xf826a462, 0xf75dff66,
	0xf6956fb7, 0xf5ccf744, 0xf50497fb, 0xf43c53cb,
	0xf3742ca3, 0xf2ac246e, 0xf1e43d1d, 0xf11c789b,
	0xf054d8d5, 0xef8d5fb9, 0xeec60f32, 0xedfee92c,
	0xed37ef92, 0xec712450, 0xebaa894f, 0xeae4207b,
	0xea1debbc, 0xe957ecfc, 0xe8922622, 0xe7cc9918,
	0xe70747c4, 0xe642340e, 0xe57d5fdb, 0xe4b8cd11,
	0xe3f47d96, 0xe330734e, 0xe26cb01b, 0xe1a935e3,
	0xe0e60686, 0xe02323e6, 0xdf608fe4, 0xde9e4c61,
	0xdddc5b3b, 0xdd1abe52, 0xdc597782, 0xdb9888a9,
	0xdad7f3a3, 0xda17ba4b, 0xd957de7b, 0xd898620d,
	0xd7d946d9, 0xd71a8eb6, 0xd65c3b7c, 0xd59e4eff,
	0xd4e0cb15, 0xd423b192, 0xd3670446, 0xd2aac505,
	0xd1eef59f, 0xd13397e3, 0xd078ad9f, 0xcfbe38a0,
	0xcf043ab4, 0xce4ab5a3, 0xcd91ab39, 0xccd91d3e,
	0xcc210d7a, 0xcb697db1, 0xcab26faa, 0xc9fbe528,
	0xc945dfed, 0xc89061bb, 0xc7db6c51, 0xc727016e,
	0xc67322cf, 0xc5bfd22f, 0xc50d114a, 0xc45ae1d8,
	0xc3a94591, 0xc2f83e2b, 0xc247cd5b, 0xc197f4d4,
	0xc0e8b649, 0xc03a1369, 0xbf8c0de4, 0xbedea766,
	0xbe31e19c, 0xbd85be31, 0xbcda3ecc, 0xbc2f6514,
	0xbb8532b1, 0xbadba944, 0xba32ca72, 0xb98a97d9,
	0xb8e3131a, 0xb83c3dd2, 0xb796199c, 0xb6f0a813,
	0xb64beace, 0xb5a7e364, 0xb5049369, 0xb461fc71,
	0xb3c0200e, 0xb31effcd, 0xb27e9d3d, 0xb1def9ea,
	0xb140175d, 0xb0a1f71e, 0xb0049ab4, 0xaf6803a3,
	0xaecc336d, 0xae312b93, 0xad96ed93, 0xacfd7ae9,
	0xac64d512, 0xabccfd84, 0xab35f5b7, 0xaa9fbf1f,
	0xaa0a5b2f, 0xa975cb58, 0xa8e21108, 0xa84f2dab,
	0xa7bd22ad, 0xa72bf175, 0xa69b9b69, 0xa60c21ef,
	0xa57d8667, 0xa4efca32, 0xa462eeae, 0xa3d6f535,
	0xa34bdf21, 0xa2c1adcb, 0xa2386285, 0xa1affea4,
	0xa1288378, 0xa0a1f24e, 0xa01c4c74, 0x9f979332,
	0x9f13c7d1, 0x9e90eb96, 0x9e0effc2, 0x9d8e0598,
	0x9d0dfe55, 0x9c8eeb35, 0x9c10cd72, 0x9b93a642,
	0x9b1776db, 0x9a9c406f, 0x9a22042e, 0x99a8c346,
	0x99307ee1, 0x98b9382a, 0x9842f045, 0x97cda856,
	0x97596180, 0x96e61ce1, 0x9673db95, 0x96029eb7,
	0x9592675d, 0x9523369d, 0x94b50d89, 0x9447ed30,
	0x93dbd6a1, 0x9370cae5, 0x9306cb06, 0x929dd807,
	0x9235f2ed, 0x91cf1cb8, 0x91695665, 0x9104a0ef,
	0x90a0fd50, 0x903e6c7c, 0x8fdcef68, 0x8f7c8703,
	0x8f1d343b, 0x8ebef7fd, 0x8e61d32f, 0x8e05c6b9,
	0x8daad37d, 0x8d50fa5b, 0x8cf83c31, 0x8ca099db,
	0x8c4a1430, 0x8bf4ac07, 0x8ba06230, 0x8b4d377e,
	0x8afb2cbc, 0x8aaa42b6, 0x8a5a7a32, 0x8a0bd3f7,
	0x89be50c5, 0x8971f15c, 0x8926b679, 0x88dca0d4,
	0x8893b126, 0x884be822, 0x88054679, 0x87bfccd9,
	0x877b7bee, 0x87385460, 0x86f656d5, 0x86b583f0,
	0x8675dc50, 0x86376094, 0x85fa1154, 0x85bdef29,
	0x8582faa6, 0x8549345e, 0x85109cde, 0x84d934b2,
	0x84a2fc64, 0x846df478, 0x843a1d72, 0x840777d1,
	0x83d60413, 0x83a5c2b2, 0x8376b424, 0x8348d8dd,
	0x831c3150, 0x82f0bde9, 0x82c67f15, 0x829d753c,
	0x8275a0c2, 0x824f020a, 0x82299973, 0x8205675a,
	0x81e26c18, 0x81c0a803, 0x81a01b6e, 0x8180c6ab,
	0x8162aa05, 0x8145c5c8, 0x812a1a3b, 0x810fa7a2,
	0x80f66e3e, 0x80de6e4e, 0x80c7a80c, 0x80b21bb1,
	0x809dc972, 0x808ab182, 0x8078d40f, 0x80683145,
	0x8058c94d, 0x804a9c4f, 0x803daa6b, 0x8031f3c3,
	0x80277874, 0x801e3896, 0x80163442, 0x800f6b8a,
	0x8009de7f, 0x80058d30, 0x800277a7, 0x80009deb
};
#else
const short COS_TABLE_A[MAX_FFT_SIZE / 4] = {
	32767, 32766, 32764, 32761,
	32757, 32751, 32744, 32736,
	32727, 32717, 32705, 32692,
	32678, 32662, 32646, 32628,
	32609, 32588, 32567, 32544,
	32520, 32495, 32468, 32441,
	32412, 32382, 32350, 32318,
	32284, 32249, 32213, 32176,
	32137, 32097, 32056, 32014,
	31970, 31926, 31880, 31833,
	31785, 31735, 31684, 31633,
	31580, 31525, 31470, 31413,
	31356, 31297, 31236, 31175,
	31113, 31049, 30984, 30918,
	30851, 30783, 30713, 30643,
	30571, 30498, 30424, 30349,
	30272, 30195, 30116, 30036,
	29955, 29873, 29790, 29706,
	29621, 29534, 29446, 29358,
	29268, 29177, 29085, 28992,
	28897, 28802, 28706, 28608,
	28510, 28410, 28309, 28208,
	28105, 28001, 27896, 27790,
	27683, 27575, 27466, 27355,
	27244, 27132, 27019, 26905,
	26789, 26673, 26556, 26437,
	26318, 26198, 26077, 25954,
	25831, 25707, 25582, 25456,
	25329, 25201, 25072, 24942,
	24811, 24679, 24546, 24413,
	24278, 24143, 24006, 23869,
	23731, 23592, 23452, 23311,
	23169, 23027, 22883, 22739,
	22594, 22448, 22301, 22153,
	22004, 21855, 21705, 21554,
	21402, 21249, 21096, 20942,
	20787, 20631, 20474, 20317,
	20159, 20000, 19840, 19680,
	19519, 19357, 19194, 19031,
	18867, 18702, 18537, 18371,
	18204, 18036, 17868, 17699,
	17530, 17360, 17189, 17017,
	16845, 16672, 16499, 16325,
	16150, 15975, 15799, 15623,
	15446, 15268, 15090, 14911,
	14732, 14552, 14372, 14191,
	14009, 13827, 13645, 13462,
	13278, 13094, 12909, 12724,
	12539, 12353, 12166, 11980,
	11792, 11604, 11416, 11227,
	11038, 10849, 10659, 10469,
	10278, 10087,  9895,  9703,
	9511,  9319,  9126,  8932,
	8739,  8545,  8351,  8156,
	7961,  7766,  7571,  7375,
	7179,  6982,  6786,  6589,
	6392,  6195,  5997,  5799,
	5601,  5403,  5205,  5006,
	4807,  4608,  4409,  4210,
	4011,  3811,  3611,  3411,
	3211,  3011,  2811,  2610,
	2410,  2209,  2009,  1808,
	1607,  1406,  1206,  1005,
	804,   603,   402,   201
};

const short COS_TABLE_B[MAX_FFT_SIZE / 4] = {
	0,    -201,   -402,   -603,
	-804,  -1005,  -1206,  -1406,
	-1607,  -1808,  -2009,  -2209,
	-2410,  -2610,  -2811,  -3011,
	-3211,  -3411,  -3611,  -3811,
	-4011,  -4210,  -4409,  -4608,
	-4807,  -5006,  -5205,  -5403,
	-5601,  -5799,  -5997,  -6195,
	-6392,  -6589,  -6786,  -6982,
	-7179,  -7375,  -7571,  -7766,
	-7961,  -8156,  -8351,  -8545,
	-8739,  -8932,  -9126,  -9319,
	-9511,  -9703,  -9895, -10087,
	-10278, -10469, -10659, -10849,
	-11038, -11227, -11416, -11604,
	-11792, -11980, -12166, -12353,
	-12539, -12724, -12909, -13094,
	-13278, -13462, -13645, -13827,
	-14009, -14191, -14372, -14552,
	-14732, -14911, -15090, -15268,
	-15446, -15623, -15799, -15975,
	-16150, -16325, -16499, -16672,
	-16845, -17017, -17189, -17360,
	-17530, -17699, -17868, -18036,
	-18204, -18371, -18537, -18702,
	-18867, -19031, -19194, -19357,
	-19519, -19680, -19840, -20000,
	-20159, -20317, -20474, -20631,
	-20787, -20942, -21096, -21249,
	-21402, -21554, -21705, -21855,
	-22004, -22153, -22301, -22448,
	-22594, -22739, -22883, -23027,
	-23169, -23311, -23452, -23592,
	-23731, -23869, -24006, -24143,
	-24278, -24413, -24546, -24679,
	-24811, -24942, -25072, -25201,
	-25329, -25456, -25582, -25707,
	-25831, -25954, -26077, -26198,
	-26318, -26437, -26556, -26673,
	-26789, -26905, -27019, -27132,
	-27244, -27355, -27466, -27575,
	-27683, -27790, -27896, -28001,
	-28105, -28208, -28309, -28410,
	-28510, -28608, -28706, -28802,
	-28897, -28992, -29085, -29177,
	-29268, -29358, -29446, -29534,
	-29621, -29706, -29790, -29873,
	-29955, -30036, -30116, -30195,
	-30272, -30349, -30424, -30498,
	-30571, -30643, -30713, -30783,
	-30851, -30918, -30984, -31049,
	-31113, -31175, -31236, -31297,
	-31356, -31413, -31470, -31525,
	-31580, -31633, -31684, -31735,
	-31785, -31833, -31880, -31926,
	-31970, -32014, -32056, -32097,
	-32137, -32176, -32213, -32249,
	-32284, -32318, -32350, -32382,
	-32412, -32441, -32468, -32495,
	-32520, -32544, -32567, -32588,
	-32609, -32628, -32646, -32662,
	-32678, -32692, -32705, -32717,
	-32727, -32736, -32744, -32751,
	-32757, -32761, -32764, -32766
};
#endif

INLINE void kf_bfly2(
	struct kfft_data *Fout,
	const UINT32 fstride,
	struct kfft_info *st,
	int m);

INLINE void kf_bfly2(
	struct kfft_data *Fout,
	const UINT32 fstride,
	struct kfft_info *st,
	int m)
{
	struct kfft_data *Fout2;
	struct kfft_data_16 *tw1 = st->twiddles;
	struct kfft_data t;
	Fout2 = Fout + m;
	do {
		//C_FIXDIV(*Fout,2); C_FIXDIV(*Fout2,2);    // Fout = Fout/2, Fout2 = Fout2/2
		C_FIXSHIFT(*Fout, 1);
		C_FIXSHIFT(*Fout2, 1); // Fout = Fout/2, Fout2 = Fout2/2

		C_MUL_16(t,  *Fout2, *tw1);         // t = Fout2 * tw1
		tw1 += fstride;
		C_SUB(*Fout2,  *Fout, t);        // Fout2 = Fout - t
		C_ADDTO(*Fout,  t);              // Fout  = Fout + t
		++Fout2;
		++Fout;
	} while (--m);
}

INLINE void kf_bfly4(
	struct kfft_data *Fout,
	const UINT32 fstride,
	struct kfft_info *st,
	const UINT32 m);

INLINE void kf_bfly4(
	struct kfft_data *Fout,
	const UINT32 fstride,
	struct kfft_info *st,
	const UINT32 m)
{
	struct kfft_data_16 *tw1, *tw2, *tw3;
	struct kfft_data scratch[6];
	UINT32 k = m;
	const UINT32 m2 = 2 * m;
	const UINT32 m3 = 3 * m;


	tw3 = tw2 = tw1 = st->twiddles;

	do {
		//C_FIXDIV(*Fout,4); C_FIXDIV(Fout[m],4); C_FIXDIV(Fout[m2],4); C_FIXDIV(Fout[m3],4);
		C_FIXSHIFT(*Fout, 2);
		C_FIXSHIFT(Fout[m], 2);
		C_FIXSHIFT(Fout[m2], 2);
		C_FIXSHIFT(Fout[m3], 2);

		C_MUL_16(scratch[0], Fout[m], *tw1);
		C_MUL_16(scratch[1], Fout[m2], *tw2);
		C_MUL_16(scratch[2], Fout[m3], *tw3);

		C_SUB(scratch[5], *Fout, scratch[1]);
		C_ADDTO(*Fout, scratch[1]);
		C_ADD(scratch[3], scratch[0], scratch[2]);
		C_SUB(scratch[4], scratch[0], scratch[2]);
		C_SUB(Fout[m2], *Fout, scratch[3]);
		tw1 += fstride;
		tw2 += fstride * 2;
		tw3 += fstride * 3;
		C_ADDTO(*Fout, scratch[3]);

		if (st->inverse) {
			Fout[m].r  = scratch[5].r - scratch[4].i;
			Fout[m].i  = scratch[5].i + scratch[4].r;
			Fout[m3].r = scratch[5].r + scratch[4].i;
			Fout[m3].i = scratch[5].i - scratch[4].r;
		} else {
			Fout[m].r = scratch[5].r + scratch[4].i;
			Fout[m].i = scratch[5].i - scratch[4].r;
			Fout[m3].r = scratch[5].r - scratch[4].i;
			Fout[m3].i = scratch[5].i + scratch[4].r;
		}
		++Fout;
	} while (--k);
}
/*
static void kf_bfly3(
         struct kfft_data * Fout,
         const size_t fstride,
         struct kfft_info * st,
         size_t m         )
{
     size_t k=m;
     const size_t m2 = 2*m;
     struct kfft_data_16 *tw1,*tw2;
     struct kfft_data scratch[5];
     struct kfft_data_16 epi3;
     epi3 = st->twiddles[fstride*m];

     tw1=tw2=st->twiddles;

     do{
         C_FIXDIV(*Fout,3); C_FIXDIV(Fout[m],3); C_FIXDIV(Fout[m2],3);

         C_MUL_16(scratch[1],Fout[m]  , *tw1);
         C_MUL_16(scratch[2],Fout[m2] , *tw2);

         C_ADD(scratch[3],scratch[1],scratch[2]);
         C_SUB(scratch[0],scratch[1],scratch[2]);
         tw1 += fstride;
         tw2 += fstride*2;

         Fout[m].r = Fout->r - HALF_OF(scratch[3].r);
         Fout[m].i = Fout->i - HALF_OF(scratch[3].i);

         C_MULBYSCALAR_16( scratch[0] , epi3.i );

         C_ADDTO(*Fout,scratch[3]);

         Fout[m2].r = Fout[m].r + scratch[0].i;
         Fout[m2].i = Fout[m].i - scratch[0].r;

         Fout[m].r -= scratch[0].i;
         Fout[m].i += scratch[0].r;

         ++Fout;
     }while(--k);
}

static void kf_bfly5(
        struct kfft_data * Fout,
        const size_t fstride,
        struct kfft_info * st,
        int m        )
{
    struct kfft_data *Fout0,*Fout1,*Fout2,*Fout3,*Fout4;
    int u;
    struct kfft_data scratch[13];
    struct kfft_data_16 * twiddles = st->twiddles;
    struct kfft_data_16 *tw;
    struct kfft_data_16 ya, yb;

    ya = twiddles[fstride*m];
    yb = twiddles[fstride*2*m];

    Fout0=Fout;
    Fout1=Fout0+m;
    Fout2=Fout0+2*m;
    Fout3=Fout0+3*m;
    Fout4=Fout0+4*m;

    tw=st->twiddles;
    for ( u=0; u<m; ++u ) {
        C_FIXDIV( *Fout0,5); C_FIXDIV( *Fout1,5); C_FIXDIV( *Fout2,5); C_FIXDIV( *Fout3,5); C_FIXDIV( *Fout4,5);
        scratch[0] = *Fout0;

        C_MUL_16(scratch[1] ,*Fout1, tw[u*fstride]  );
        C_MUL_16(scratch[2] ,*Fout2, tw[2*u*fstride]);
        C_MUL_16(scratch[3] ,*Fout3, tw[3*u*fstride]);
        C_MUL_16(scratch[4] ,*Fout4, tw[4*u*fstride]);

        C_ADD( scratch[7],scratch[1],scratch[4]);
        C_SUB( scratch[10],scratch[1],scratch[4]);
        C_ADD( scratch[8],scratch[2],scratch[3]);
        C_SUB( scratch[9],scratch[2],scratch[3]);

        Fout0->r += scratch[7].r + scratch[8].r;
        Fout0->i += scratch[7].i + scratch[8].i;

        scratch[5].r = scratch[0].r + S_MUL_16(scratch[7].r, ya.r) + S_MUL_16(scratch[8].r, yb.r);
        scratch[5].i = scratch[0].i + S_MUL_16(scratch[7].i, ya.r) + S_MUL_16(scratch[8].i, yb.r);

        scratch[6].r =  S_MUL_16(scratch[10].i, ya.i) + S_MUL_16(scratch[9].i, yb.i);
        scratch[6].i = -S_MUL_16(scratch[10].r, ya.i) - S_MUL_16(scratch[9].r, yb.i);

        C_SUB(*Fout1, scratch[5], scratch[6]);
        C_ADD(*Fout4, scratch[5], scratch[6]);

        scratch[11].r = scratch[0].r + S_MUL_16(scratch[7].r, yb.r) + S_MUL_16(scratch[8].r, ya.r);
        scratch[11].i = scratch[0].i + S_MUL_16(scratch[7].i, yb.r) + S_MUL_16(scratch[8].i, ya.r);
        scratch[12].r = -S_MUL_16(scratch[10].i, yb.i) + S_MUL_16(scratch[9].i, ya.i);
        scratch[12].i =  S_MUL_16(scratch[10].r, yb.i) - S_MUL_16(scratch[9].r, ya.i);

        C_ADD(*Fout2, scratch[11], scratch[12]);
        C_SUB(*Fout3, scratch[11], scratch[12]);

        ++Fout0;++Fout1;++Fout2;++Fout3;++Fout4;
    }
}

// perform the butterfly for one stage of a mixed radix FFT
static void kf_bfly_generic(
        struct kfft_data * Fout,
        const size_t fstride,
        struct kfft_info * st,
        int m,
        int p       )
{
    int u,k,q1,q;
    struct kfft_data_16 * twiddles = st->twiddles;
    struct kfft_data t;
    int Norig = st->nfft;

    struct kfft_data * scratch = (struct kfft_data*)malloc(sizeof(struct kfft_data)*p);

    for ( u=0; u<m; ++u ) {
        k=u;
        for ( q1=0 ; q1<p ; ++q1 ) {
            scratch[q1] = Fout[ k  ];
            C_FIXDIV(scratch[q1],p);
            k += m;
        }

        k=u;
        for ( q1=0 ; q1<p ; ++q1 ) {
            int twidx=0;
            Fout[ k ] = scratch[0];
            for (q=1;q<p;++q ) {
                twidx += fstride * k;
                if (twidx>=Norig) twidx-=Norig;
                C_MUL_16(t,scratch[q] , twiddles[twidx] );
                C_ADDTO( Fout[ k ] ,t);
            }
            k += m;
        }
    }
    free(scratch);
}
*/

static void kf_work(
	struct kfft_data *Fout,
	const struct kfft_data *f,
	const UINT32 fstride,
	int in_stride,
	int *factors,
	struct kfft_info *st)
{
	struct kfft_data *Fout_beg = Fout;
	const int p = *factors++; /* the radix  */
	const int m = *factors++; /* stage's fft length/p */
	const struct kfft_data *Fout_end = Fout + p * m;

#ifdef _OPENMP
	// use openmp extensions at the
	// top-level (not recursive)
	if (fstride == 1 && p <= 5) {
		int k;

		// execute the p different work units in different threads
		# pragma omp parallel for
		for (k = 0; k < p; ++k) {
			kf_work(Fout + k * m, f + fstride * in_stride * k, fstride * p, in_stride, factors, st);
		}
		// all threads have joined by this point

		switch (p) {
		case 2:
			kf_bfly2(Fout, fstride, st, m);
			break;
		case 3:
			kf_bfly3(Fout, fstride, st, m);
			break;
		case 4:
			kf_bfly4(Fout, fstride, st, m);
			break;
		case 5:
			kf_bfly5(Fout, fstride, st, m);
			break;
		default:
			kf_bfly_generic(Fout, fstride, st, m, p);
			break;
		}
		return;
	}
#endif

	if (m == 1) {
		do {
			*Fout = *f;
			f += fstride * in_stride;
		} while (++Fout != Fout_end);
	} else {
		do {
			// recursive call:
			// DFT of size m*p performed by doing
			// p instances of smaller DFTs of size m,
			// each one takes a decimated version of the input
			kf_work(Fout, f, fstride * p, in_stride, factors, st);
			f += fstride * in_stride;
		} while ((Fout += m) != Fout_end);
	}

	Fout = Fout_beg;

	// recombine the p smaller DFTs
	switch (p) {
	case 2:
		kf_bfly2(Fout, fstride, st, m);
		break;
	//case 3:
	//  kf_bfly3(Fout,fstride,st,m);
	//  break;
	case 4:
		kf_bfly4(Fout, fstride, st, m);
		break;
	//case 5:
	//  kf_bfly5(Fout,fstride,st,m);
	//  break;
	default:
		err_code = ErrCode_106;
		//kf_bfly_generic(Fout,fstride,st,m,p);
		break;
	}
}

/*  facbuf is populated by p1,m1,p2,m2, ...
    where
    p[i] * m[i] = m[i-1]
    m0 = n                  */
static void kf_factor(int n, int *facbuf)
{
#if 0
	int p = 4;
	double floor_sqrt;
	floor_sqrt = floor(sqrt((double)n));

	/*factor out powers of 4, powers of 2, then any remaining primes */
	do {
		while (n % p) {
			switch (p) {
			case 4:
				p = 2;
				break;
			case 2:
				p = 3;
				break;
			default:
				p += 2;
				break;
			}
			if (p > floor_sqrt) {
				p = n;    /* no more factors, skip to end */
			}
		}
		n /= p;
		*facbuf++ = p;
		*facbuf++ = n;
	} while (n > 1);
#else
	switch (n) {
	case 1024:
		facbuf[0] = 4;
		facbuf[1] = 256;
		facbuf[2] = 4;
		facbuf[3] = 64;
		facbuf[4] = 4;
		facbuf[5] = 16;
		facbuf[6] = 4;
		facbuf[7] = 4;
		facbuf[8] = 4;
		facbuf[9] = 1;
		break;
	case 512:
		facbuf[0] = 4;
		facbuf[1] = 128;
		facbuf[2] = 4;
		facbuf[3] = 32;
		facbuf[4] = 4;
		facbuf[5] = 8;
		facbuf[6] = 4;
		facbuf[7] = 2;
		facbuf[8] = 2;
		facbuf[9] = 1;
		break;
	case 256:
		facbuf[0] = 4;
		facbuf[1] = 64;
		facbuf[2] = 4;
		facbuf[3] = 16;
		facbuf[4] = 4;
		facbuf[5] = 4;
		facbuf[6] = 4;
		facbuf[7] = 1;
		facbuf[8] = 2;
		facbuf[9] = 1;
		break;
	default:
		//printf("\n Not support in kfft_alloc()");
		break;
	}
#endif

}

/* User-callable function to allocate all necessary storage space for the fft.
 *
 * The return value is a contiguous block of memory, allocated with malloc.  As such,
 * It can be freed with free(), rather than a kiss_fft-specific function.
 * */
struct kfft_info *kfft_alloc(struct kfft_info *st, int nfft, int inverse_fft)
{
#if 0
	struct kfft_info *st = NULL;
	size_t memneeded = sizeof(struct kfft_info)
					   + sizeof(struct kfft_data) * (nfft - 1); /* twiddle factors*/

	st = (struct kfft_info *)malloc(memneeded);

	if (st) {
		int i;
		st->nfft = nfft;
		st->inverse = inverse_fft;

		for (i = 0; i < nfft; ++i) {
			const double pi = 3.141592653589793238462643383279502884197169399375105820974944;
			double phase = -2 * pi * i / nfft;
			if (st->inverse) {
				phase *= -1;
			}
			//kf_cexp(st->twiddles+i, phase );
			st->twiddles[i].r = cos(phase);
			st->twiddles[i].i = sin(phase);

			//if ((i % 4) == 0) printf("\n(%4d) ",i);
			//printf(" %5d,", (short)(cos(phase)*0x7fff));

		}

		kf_factor(nfft, st->factors);
	}
#else
	int step;
	int fft_2;
	int fft_4;
	// ============ 查表, 只支援 nfft=256 or 512
	//struct kfft_info * st=NULL;
	//size_t memneeded = sizeof(struct kfft_info)
	//    + sizeof(struct kfft_data)*(nfft-1); /* twiddle factors*/
	//st = (struct kfft_info *)malloc( memneeded );

	if (st) {
		int i, j, k;
		st->nfft = nfft;
		st->inverse = inverse_fft;

		switch (nfft) {
		case 1024:
			step = 1;
			break;
		case 512:
			step = 2;
			break;
		case 256:
			step = 4;
			break;
		default:
			step = 1;
			//printf("\n Not support in kfft_alloc()");
			break;
		}
		//--- Create st->twiddles[].r 與 st->inverse 無關
		fft_2 = nfft >> 1;
		fft_4 = nfft >> 2;
		for (i = 0, j = fft_4, k = 0; i < fft_4; i++, j++, k += step) {
			st->twiddles[i      ].r =  COS_TABLE_A[k];
			st->twiddles[i + fft_2].r = -COS_TABLE_A[k];
			st->twiddles[j      ].r =  COS_TABLE_B[k];
			st->twiddles[j + fft_2].r = -COS_TABLE_B[k];
		}
		//--- Create st->twiddles[].i 與 st->inverse 有關
		if (st->inverse) {
			for (i = 0, j = fft_4, k = 0; i < fft_4; i++, j++, k += step) {
				st->twiddles[i      ].i = -COS_TABLE_B[k];
				st->twiddles[i + fft_2].i =  COS_TABLE_B[k];
				st->twiddles[j      ].i =  COS_TABLE_A[k];
				st->twiddles[j + fft_2].i = -COS_TABLE_A[k];
			}
		} else {
			for (i = 0, j = fft_4, k = 0; i < fft_4; i++, j++, k += step) {
				st->twiddles[i      ].i =  COS_TABLE_B[k];
				st->twiddles[i + fft_2].i = -COS_TABLE_B[k];
				st->twiddles[j      ].i = -COS_TABLE_A[k];
				st->twiddles[j + fft_2].i =  COS_TABLE_A[k];
			}
		}

		kf_factor(nfft, st->factors);

	}

#endif

#if 0
	int i;
	printf("\nnfft=%d", nfft);
	for (i = 0; i < nfft; i++) {
		printf("\n %3d(0x%08x,0x%08x)", i, st->twiddles[i].r, st->twiddles[i].i);
	}
	//for (i=0;i<(2*MAXFACTORS);i++){ printf("\n%4d(0x%08x)", i, st->factors[i]);   }
#endif
	return st;
}

void _kiss_fft(struct kfft_info *cfg, const struct kfft_data *fin, struct kfft_data *fout)
{
	kf_work(fout, fin, 1, 1, cfg->factors, cfg);
}

