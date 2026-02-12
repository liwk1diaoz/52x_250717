/*
    CPU module driver.

    This file is the driver of CPU module.

    @file       Cpu_cp.c
    @ingroup    mIDrvSys_Core
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#define __MODULE__    rtos_cpu
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/spinlock.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "cpu_cp_reg.h"
#include "cpu_cp_int.h"
#include "cpu_protected.h"
#include "cache_protected.h"
#include "plat/cache.h"
#include "pl310.h"
#include "plat/v7_pmu.h"
#include "plat/top.h"
#define __arch_getl(a)			(*(volatile unsigned int *)(a))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))
#define mb()		asm volatile("dsb sy" : : : "memory")
#define dmb()		__asm__ __volatile__ ("" : : : "memory")
#define __iormb()	dmb()
#define __iowmb()	dmb()
#define writel(v,c)	({ unsigned int __v = v; __iowmb(); __arch_putl(__v,c); __v; })
#define readl(c)	({ unsigned int __v = __arch_getl(c); __iormb(); __v; })
#include "plat/pll.h"
static BOOL cpu_count_open = FALSE;

#define HW_WP_CNT   4
#define HW_BP_CNT   6
/* Number of BRP/WRP registers on this CPU. */
static int core_num_brps;
static int core_num_wrps;

static int wp_slots[HW_WP_CNT];
static int bp_slots[HW_BP_CNT];


#define CPU_Y_LOG(fmt, args...)         DBG_DUMP(DBG_COLOR_YELLOW fmt DBG_COLOR_END, ##args)
#define CPU_R_LOG(fmt, args...)         DBG_DUMP(DBG_COLOR_RED fmt DBG_COLOR_END, ##args)
#define CPU_M_LOG(fmt, args...)         DBG_DUMP(DBG_COLOR_MAGENTA fmt DBG_COLOR_END, ##args)
#define CPU_G_LOG(fmt, args...)         DBG_DUMP(DBG_COLOR_GREEN fmt DBG_COLOR_END, ##args)
#define CPU_W_LOG(fmt, args...)         DBG_DUMP(DBG_COLOR_WHITE fmt DBG_COLOR_END, ##args)


/* Maximum supported watchpoint length. */
static UINT32 max_watchpoint_len;

//extern UINT32 ce_aes_sub(UINT32 input);

extern void ce_aes_invert(void *dst, void *src);

extern void ce_aes_ecb_encrypt(UINT8 out[], UINT8 const in[], UINT8 const rk[], int rounds, int blocks);
extern void ce_aes_ecb_decrypt(UINT8 out[], UINT8 const in[], UINT8 const rk[], int rounds, int blocks);
extern void ce_aes_cbc_encrypt(UINT8 out[], UINT8 const in[], UINT8 const rk[], int rounds, int blocks, UINT8 iv[]);
extern void ce_aes_cbc_decrypt(UINT8 out[], UINT8 const in[], UINT8 const rk[], int rounds, int blocks, UINT8 iv[]);
extern void ce_aes_ctr_encrypt(UINT8 out[], UINT8 const in[], UINT8 const rk[], int rounds, int blocks, UINT8 ctr[]);

#define AES_MIN_KEY_SIZE    16
#define AES_MAX_KEY_SIZE    32
#define AES_KEYSIZE_192     24
#define AES_BLOCK_SIZE      16
#define AES_MAX_KEYLENGTH   (15 * 16)
#define AES_MAX_KEYLENGTH_U32    (AES_MAX_KEYLENGTH / sizeof(UINT32))

#define READ_WB_REG_CASE(OP2, M, VAL)           \
	case ((OP2 << 4) + M):              \
	ARM_DBG_READ(c0, c ## M, OP2, VAL); \
	break

#define WRITE_WB_REG_CASE(OP2, M, VAL)          \
	case ((OP2 << 4) + M):              \
	ARM_DBG_WRITE(c0, c ## M, OP2, VAL);    \
	break

#define GEN_READ_WB_REG_CASES(OP2, VAL)     \
	READ_WB_REG_CASE(OP2, 0, VAL);      \
	READ_WB_REG_CASE(OP2, 1, VAL);      \
	READ_WB_REG_CASE(OP2, 2, VAL);      \
	READ_WB_REG_CASE(OP2, 3, VAL);      \
	READ_WB_REG_CASE(OP2, 4, VAL);      \
	READ_WB_REG_CASE(OP2, 5, VAL);      \
	READ_WB_REG_CASE(OP2, 6, VAL);      \
	READ_WB_REG_CASE(OP2, 7, VAL);      \
	READ_WB_REG_CASE(OP2, 8, VAL);      \
	READ_WB_REG_CASE(OP2, 9, VAL);      \
	READ_WB_REG_CASE(OP2, 10, VAL);     \
	READ_WB_REG_CASE(OP2, 11, VAL);     \
	READ_WB_REG_CASE(OP2, 12, VAL);     \
	READ_WB_REG_CASE(OP2, 13, VAL);     \
	READ_WB_REG_CASE(OP2, 14, VAL);     \
	READ_WB_REG_CASE(OP2, 15, VAL)

#define GEN_WRITE_WB_REG_CASES(OP2, VAL)    \
	WRITE_WB_REG_CASE(OP2, 0, VAL);     \
	WRITE_WB_REG_CASE(OP2, 1, VAL);     \
	WRITE_WB_REG_CASE(OP2, 2, VAL);     \
	WRITE_WB_REG_CASE(OP2, 3, VAL);     \
	WRITE_WB_REG_CASE(OP2, 4, VAL);     \
	WRITE_WB_REG_CASE(OP2, 5, VAL);     \
	WRITE_WB_REG_CASE(OP2, 6, VAL);     \
	WRITE_WB_REG_CASE(OP2, 7, VAL);     \
	WRITE_WB_REG_CASE(OP2, 8, VAL);     \
	WRITE_WB_REG_CASE(OP2, 9, VAL);     \
	WRITE_WB_REG_CASE(OP2, 10, VAL);    \
	WRITE_WB_REG_CASE(OP2, 11, VAL);    \
	WRITE_WB_REG_CASE(OP2, 12, VAL);    \
	WRITE_WB_REG_CASE(OP2, 13, VAL);    \
	WRITE_WB_REG_CASE(OP2, 14, VAL);    \
	WRITE_WB_REG_CASE(OP2, 15, VAL)

static inline UINT32 encode_ctrl_reg(struct arch_hw_breakpoint_ctrl ctrl)
{
	return (ctrl.mismatch << 22) | (ctrl.len << 5) | (ctrl.type << 3) |
		   (ctrl.privilege << 1) | ctrl.enabled;
}

static UINT32 read_wb_reg(int n)
{
	UINT32 val = 0;

	switch (n) {
		GEN_READ_WB_REG_CASES(ARM_OP2_BVR, val);
		GEN_READ_WB_REG_CASES(ARM_OP2_BCR, val);
		GEN_READ_WB_REG_CASES(ARM_OP2_WVR, val);
		GEN_READ_WB_REG_CASES(ARM_OP2_WCR, val);
	default:
		DBG_ERR("attempt to read from unknown breakpoint "
				"register %d\n", n);
	}

	return val;
}

static void write_wb_reg(int n, UINT32 val)
{
	switch (n) {
		GEN_WRITE_WB_REG_CASES(ARM_OP2_BVR, val);
		GEN_WRITE_WB_REG_CASES(ARM_OP2_BCR, val);
		GEN_WRITE_WB_REG_CASES(ARM_OP2_WVR, val);
		GEN_WRITE_WB_REG_CASES(ARM_OP2_WCR, val);
	default:
		DBG_ERR("attempt to write to unknown breakpoint "
				"register %d\n", n);
	}
	_ISB();
}


static UINT32 get_max_wp_len(void)
{
	T_CA53_DBGWCRn_REG  ctrl = {0};
	UINT32 size = 4;

	ctrl.bit.bas = ARM_BREAKPOINT_LEN_8;
	write_wb_reg(ARM_BASE_WVR, 0);
	write_wb_reg(ARM_BASE_WCR, ctrl.reg);
	if ((read_wb_reg(ARM_BASE_WCR) & ctrl.reg) == ctrl.reg) {
		size = 8;
	}

	DBG_IND(" WP len register %d\r\n", size);
	return size;
}

/* Determine debug architecture. */
static UINT8 get_debug_arch(void)
{
	UINT32 didr;

	/* Do we implement the extended CPUID interface? */
	if (((CPUID_MIDR() >> 16) & 0xf) != 0xf) {
		DBG_WRN("CPUID feature registers not supported. "
				"Assuming v6 debug is present.\n");
		return ARM_DEBUG_ARCH_V6;
	}

	ARM_DBG_READ(c0, c0, 0, didr);
	return (didr >> 16) & 0xf;
}

/* Determine number of WRP registers available. */
static int get_num_wrp_resources(void)
{
	UINT32 didr;
	ARM_DBG_READ(c0, c0, 0, didr);
	return ((didr >> 28) & 0xf) + 1;
}

/* Determine number of BRP registers available. */
static int get_num_brp_resources(void)
{
	UINT32 didr;
	//Trm page 400
	ARM_DBG_READ(c0, c0, 0, didr);
	return ((didr >> 24) & 0xf) + 1;
}

/* Does this core support mismatch breakpoints? */
static int core_has_mismatch_brps(void)
{
	DBG_IND("get_debug_arch = %d\r\n", get_debug_arch());
	return (get_debug_arch() >= ARM_DEBUG_ARCH_V7_ECP14 &&
			get_num_brp_resources() > 1);
}


/* Determine number of usable WRPs available. */
static int get_num_wrps(void)
{
	/*
	 * On debug architectures prior to 7.1, when a watchpoint fires, the
	 * only way to work out which watchpoint it was is by disassembling
	 * the faulting instruction and working out the address of the memory
	 * access.
	 *
	 * Furthermore, we can only do this if the watchpoint was precise
	 * since imprecise watchpoints prevent us from calculating register
	 * based addresses.
	 *
	 * Providing we have more than 1 breakpoint register, we only report
	 * a single watchpoint register for the time being. This way, we always
	 * know which watchpoint fired. In the future we can either add a
	 * disassembler and address generation emulator, or we can insert a
	 * check to see if the DFAR is set on watchpoint exception entry
	 * [the ARM ARM states that the DFAR is UNKNOWN, but experience shows
	 * that it is set on some implementations].
	 */
	return get_num_wrp_resources();
}

/* Determine number of usable BRPs available. */
static int get_num_brps(void)
{
	int brps = get_num_brp_resources();
	return core_has_mismatch_brps() ? brps - 1 : brps;
}
#if 0
static int hw_breakpoint_slots(int type)
{

	/*
	 * We can be called early, so don't rely on
	 * our static variables being initialised.
	 */
	switch (type) {
	case TYPE_INST:
		return get_num_brps();
	case TYPE_DATA:
		return get_num_wrps();
	default:
		DBG_WRN("unknown slot type: %d\n", type);
		return 0;
	}
}
#endif

#if 0
static inline UINT32 ror32(UINT32 word, unsigned int shift)
{
	return (word >> shift) | (word << (32 - shift));
}

struct aes_block {
	UINT8 b[AES_BLOCK_SIZE];
};

struct crypto_aes_ctx {
	UINT32 key_enc[AES_MAX_KEYLENGTH_U32];
	UINT32 key_dec[AES_MAX_KEYLENGTH_U32];
	UINT32 key_length;
};

static struct crypto_aes_ctx ctx;


static int num_rounds(struct crypto_aes_ctx *ctx)
{
	/*
	 * # of rounds specified by AES:
	 * 128 bit key      10 rounds
	 * 192 bit key      12 rounds
	 * 256 bit key      14 rounds
	 * => n byte key    => 6 + (n/4) rounds
	 */
	return 6 + ctx->key_length / 4;
}
static int ce_aes_expandkey(struct crypto_aes_ctx *ctx, const UINT8 *in_key,
							unsigned int key_len)
{
	/*
	 * The AES key schedule round constants
	 */
	static UINT8 const rcon[] = {
		0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
	};

	UINT32 kwords = key_len / sizeof(UINT32);
	struct aes_block *key_enc, *key_dec;
	int i, j;

	if (key_len != AES_KEYSIZE_128 &&
		key_len != AES_KEYSIZE_192 &&
		key_len != AES_KEYSIZE_256) {
		return -1;
	}

	memcpy(ctx->key_enc, in_key, key_len);
	ctx->key_length = key_len;

	//kernel_neon_begin();
	for (i = 0; i < (int)sizeof(rcon); i++) {
		UINT32 *rki = ctx->key_enc + (i * kwords);
		UINT32 *rko = rki + kwords;

		rko[0] = ror32(ce_aes_sub(rki[kwords - 1]), 8);
		rko[0] = rko[0] ^ rki[0] ^ rcon[i];
		rko[1] = rko[0] ^ rki[1];
		rko[2] = rko[1] ^ rki[2];
		rko[3] = rko[2] ^ rki[3];

		if (key_len == AES_KEYSIZE_192) {
			if (i >= 7) {
				break;
			}
			rko[4] = rko[3] ^ rki[4];
			rko[5] = rko[4] ^ rki[5];
		} else if (key_len == AES_KEYSIZE_256) {
			if (i >= 6) {
				break;
			}
			rko[4] = ce_aes_sub(rko[3]) ^ rki[4];
			rko[5] = rko[4] ^ rki[5];
			rko[6] = rko[5] ^ rki[6];
			rko[7] = rko[6] ^ rki[7];
		}
	}

	/*
	 * Generate the decryption keys for the Equivalent Inverse Cipher.
	 * This involves reversing the order of the round keys, and applying
	 * the Inverse Mix Columns transformation on all but the first and
	 * the last one.
	 */
	key_enc = (struct aes_block *)ctx->key_enc;
	key_dec = (struct aes_block *)ctx->key_dec;
	j = num_rounds(ctx);

	key_dec[0] = key_enc[j];
	for (i = 1, j--; j > 0; i++, j--) {
		ce_aes_invert(key_dec + i, key_enc + j);
	}
	key_dec[i] = key_enc[0];

	//kernel_neon_end();
	return 0;
}

ER ca53_set_aes_key_size(UINT32 uiKeySize)
{
	if (uiKeySize == AES_KEYSIZE_128 || uiKeySize == AES_KEYSIZE_256) {
		ctx.key_length = uiKeySize;
		return E_OK;
	} else {
		DBG_ERR("ARM v8 AES only support 128 or 256 bits\r\n");
		return E_SYS;
	}
}

/*
     AEC ecb encryption.

     AEC ecb encryption by arm v8 instruction

     @param[in]     uiKeySize   AES_KEYSIZE_128 or AES_KEYSIZE_256 bits
     @param[in]     in_key      point of AES key
     @param[out]    out         AES encryption output data
     @param[in]     in          point of AES planeText data

     Example: (Optional)
     @code
     {
        const UINT8 __data[16] = {0x32, 0x88, 0x31, 0xE0, 0x43, 0x5a, 0x31, 0x37, 0xf6, 0x30, 0x98, 0x07, 0xa8, 0x8d, 0xa2, 0x34};
        const UINT8 __key[16]  = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
        UINT8    __cypher[16];
        UINT32  i;
        if(ca53_do_aes_ecb_encrypt(AES_KEYSIZE_128, __key, __cypher, __data) == E_OK)
        {
            emu_msg(("aes ebc ok => "));
            for(i = 0; i < sizeof(__cypher); i++)
            {
                emu_msg(("0x%02x ", __cypher[i]));
            }
            emu_msg(("\r\n"));
        }
        else
        {
            emu_msg(("aes ebc NG\r\n"));
        }
     }
     @endcode
*/
ER  ca53_do_aes_ecb_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], UINT8 const in[])
{
	if (uiKeySize != AES_KEYSIZE_128 && uiKeySize != AES_KEYSIZE_256) {
		DBG_ERR("ARM v8 AES only support 128 or 256 bits\r\n");
		return E_SYS;
	}
	if (ca53_set_aes_key_size(uiKeySize) != E_OK) {
		DBG_ERR("Invalided aes key size %d bits\r\n", (uiKeySize << 3));
	} else {
		DBG_IND("Aes key size %d bits\r\n", (uiKeySize << 3));
	}
	if (ce_aes_expandkey(&ctx, in_key, ctx.key_length) < 0) {
		DBG_ERR("ce_aes_expandkey error\r\n");
	}

	ce_aes_ecb_encrypt(out, in, (UINT8 const *)&ctx.key_enc[0], num_rounds(&ctx), 1);

	return E_OK;
}

#endif
/*
     AEC cbc encryption.

     AEC cbc encryption by arm v8 instruction

     @param[in]     uiKeySize   AES_KEYSIZE_128 or AES_KEYSIZE_256 bits
     @param[in]     in_key      point of AES key
     @param[out]    out         AES encryption output data
     @param[in]     in          point of AES planeText data

     Example: (Optional)
     @code
     {
        const UINT8 __data[16] = {0x32, 0x88, 0x31, 0xE0, 0x43, 0x5a, 0x31, 0x37, 0xf6, 0x30, 0x98, 0x07, 0xa8, 0x8d, 0xa2, 0x34};
        const UINT8 __key[16]  = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
        UINT8    __cypher[16];
        UINT32  i;
        if(ca53_do_aes_ecb_encrypt(AES_KEYSIZE_128, __key, __cypher, __data) == E_OK)
        {
            emu_msg(("aes ebc ok => "));
            for(i = 0; i < sizeof(__cypher); i++)
            {
                emu_msg(("0x%02x ", __cypher[i]));
            }
            emu_msg(("\r\n"));
        }
        else
        {
            emu_msg(("aes ebc NG\r\n"));
        }
     }
     @endcode
*/
ER  ca53_do_aes_cbc_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], UINT8 const in[], UINT8 iv[])
{
	UINT32 i = 0;
	UINT8 cypher[uiKeySize];
	ER er;

	for (i = 0; i < uiKeySize; i++) {
		cypher[i] = iv[i] ^ in[i];
	}

	er = ca53_do_aes_ecb_encrypt(uiKeySize, in_key, out, cypher);

	if (er != E_OK) {
		return er;
	}

	memcpy(iv, out, uiKeySize * sizeof(UINT8));

#if 0
	if (uiKeySize != AES_KEYSIZE_128 && uiKeySize != AES_KEYSIZE_256) {
		DBG_ERR("ARM v8 AES only support 128 or 256 bits\r\n");
		return E_SYS;
	}
	if (ca53_set_aes_key_size(uiKeySize) != E_OK) {
		DBG_ERR("Invalided aes key size %d bits\r\n", (uiKeySize << 3));
	} else {
		DBG_IND("Aes key size %d bits\r\n", (uiKeySize << 3));
	}
	if (ce_aes_expandkey(&ctx, in_key, ctx.key_length) < 0) {
		DBG_ERR("ce_aes_expandkey error\r\n");
	}

	ce_aes_cbc_encrypt(out, in, (UINT8 const *)&ctx.key_enc[0], num_rounds(&ctx), 1, (UINT8 *)iv);
#endif
	return E_OK;

}
ER  ca53_do_aes_cfb_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], const UINT8  in[], UINT8 iv[])
{
	UINT8 cypher[uiKeySize];
	UINT32 i = 0;
	ER er;

    memset((void *)cypher, 0x0, uiKeySize);
	er = ca53_do_aes_ecb_encrypt(uiKeySize, in_key, cypher, iv);

	if (er != E_OK) {
		return er;
	}

	for (i = 0; i < uiKeySize; i++) {
		out[i] = cypher[i] ^ in[i];
	}
	memcpy(iv, out, uiKeySize * sizeof(UINT8));

	return E_OK;
}


ER  ca53_do_aes_ofb_encrypt(UINT32 uiKeySize,  const   UINT8 *in_key, UINT8 out[], const UINT8  in[], UINT8  iv[])
{
	UINT8 cypher[uiKeySize];
	UINT32 i = 0;
	ER er;

    memset((void *)cypher, 0x0, uiKeySize);
	er = ca53_do_aes_ecb_encrypt(uiKeySize, in_key, cypher, iv);

	if (er != E_OK) {
		return er;
	}

	for (i = 0; i < uiKeySize; i++) {
		out[i] = cypher[i] ^ in[i];
	}
	memcpy(iv, cypher, uiKeySize * sizeof(UINT8));

	return E_OK;
}


ER  ca53_do_aes_ctr_encrypt(UINT32 uiKeySize,  const   UINT8 *in_key, UINT8 out[], const UINT8  in[], UINT8  iv[])
{
	UINT8 cypher[uiKeySize];
	UINT32 i = 0;
	ER er;

    memset((void *)cypher, 0x0, uiKeySize);
	er = ca53_do_aes_ecb_encrypt(uiKeySize, in_key, cypher, iv);

	if (er != E_OK) {
		return er;
	}

	for (i = 0; i < uiKeySize; i++) {
		out[i] = cypher[i] ^ in[i];
	}

	return E_OK;
}

/**
    CTI related configuration : halt CA53

    External debug request => halt specific coreX
    bit[24] : core0 / bit[25] : core1, 0 No external debug request. / 1 External debug request.
    The core treats the EDBGRQ input as level-sensitive. The EDBGRQ input must be asserted until the core asserts DBGACK.

    @param[in] uiCore   Specific Core
    @param[in] bEn      enable halt (or disable)

    @return void
*/
/*
void ca53_set_cti_halt_enable(CC_CORE_ID uiCore, BOOL bEn)
{
	T_CA53_GIC_CONFIG_REG   uiCA53GICConfig;

	uiCA53GICConfig.reg = CPU_OSPREY_GETREG(CA53_GIC_CONFIG_REG_OFS);

	uiCA53GICConfig.bit.edbgrq = (1 << uiCore);

	CPU_OSPREY_SETREG(CA53_GIC_CONFIG_REG_OFS, uiCA53GICConfig.reg);
}


void ca53_set_wfi_source(CPU_WFI_SRC wfi_source)
{
	T_CA53_CHANGE_CLOCK_REG    wfi;
	wfi.reg = CPU_OSPREY_GETREG(CA53_CHANGE_CLOCK_REG_OFS);
	wfi.bit.WFI_MASK_BIT = wfi_source;
	CPU_OSPREY_SETREG(CA53_CHANGE_CLOCK_REG_OFS, wfi.reg);
}

void ca53_set_change_clock_interrupt_enable(BOOL int_en)
{
	T_CA53_CHANGE_CLOCK_REG    wfi_source;
	wfi_source.reg = CPU_OSPREY_GETREG(CA53_CHANGE_CLOCK_REG_OFS);
	wfi_source.bit.armclk_chg_done_inten = int_en;
	CPU_OSPREY_SETREG(CA53_CHANGE_CLOCK_REG_OFS, wfi_source.reg);
}

void ca53_trigger_change_clock_operation(BOOL trigger)
{
	T_CA53_CHANGE_CLOCK_REG    wfi_source;
	wfi_source.reg = CPU_OSPREY_GETREG(CA53_CHANGE_CLOCK_REG_OFS);
	wfi_source.bit.armclk_divreg_chg = trigger;
	CPU_OSPREY_SETREG(CA53_CHANGE_CLOCK_REG_OFS, wfi_source.reg);
}
*/
ER ca53_change_internal_clock(CPU_INTERNAL_CLK_DIV l2_arm_clk_div, CPU_INTERNAL_CLK_DIV periph_clk_div, CPU_INTERNAL_CLK_DIV coresight_clk_div)
{
#if 0 //NA51000
	INT_INTC_ENABLE gic_int_en;
	T_CA53_CHANGE_CLOCK_REG    wfi_source;
	if (l2_arm_clk_div >= CA53_INTERNAL_DIV_MAX || periph_clk_div >= CA53_INTERNAL_DIV_MAX || coresight_clk_div >= CA53_INTERNAL_DIV_MAX) {
		return E_PAR;
	}
	while (!(uart_checkIntStatus() & UART_INT_STATUS_TX_EMPTY)) {
	}
	int_get_gic_enable(&gic_int_en);
	int_disable_multi(gic_int_en);
	drv_enableInt(DRV_INT_WFI_GEN_INT);
	int_gic_set_target(DRV_INT_WFI_GEN_INT, 1);
	int_gic_set_trigger_type(DRV_INT_WFI_GEN_INT, 1);

	wfi_source.reg = CPU_OSPREY_GETREG(CA53_CHANGE_CLOCK_REG_OFS);
	wfi_source.bit.L2arm_clksel     = l2_arm_clk_div;
	wfi_source.bit.periph_clksel    = periph_clk_div;
	wfi_source.bit.coresight_clksel = coresight_clk_div;
	CPU_OSPREY_SETREG(CA53_CHANGE_CLOCK_REG_OFS, wfi_source.reg);

	ca53_set_wfi_source(CA53_WFI_L2);
	ca53_set_change_clock_interrupt_enable(TRUE);
	ca53_trigger_change_clock_operation(TRUE);
	__asm__ __volatile__("isb\n\t");
	__asm__ __volatile__("wfi\n\t");
	int_gic_set_target(DRV_INT_WFI_GEN_INT, 0);
	drv_disableInt(DRV_INT_WFI_GEN_INT);
	ca53_set_change_clock_interrupt_enable(FALSE);
	ca53_trigger_change_clock_operation(FALSE);
	int_enable_multi(gic_int_en);
#endif
	DBG_IND("wfi done\r\n");
	return E_OK;
}

ER ca53_debug_init(void)
{
	UINT32  i;
#if (__DBGLVL__ == 5)
	UINT32  debug_arch;
#endif
	UINT32  dbg_ext;


	for (i = 0; i < HW_WP_CNT; i++) {
		wp_slots[i] = 0;
	}

	for (i = 0; i < HW_BP_CNT; i++) {
		bp_slots[i] = 0;
	}


	/* Determine how many BRPs/WRPs are available. */
	core_num_brps = get_num_brps();
	core_num_wrps = get_num_wrps();
	DBG_IND("core_num_brps [%d], core_num_wrps [%d]\r\n", core_num_brps, core_num_wrps);

	max_watchpoint_len = get_max_wp_len();
	/* Work out the maximum supported watchpoint length. */
	//max_watchpoint_len = get_max_wp_len();
	DBG_IND("maximum watchpoint size is %u bytes.\r\n", max_watchpoint_len);
//	debug_arch = CPUID_MIDR();
	//DBG_IND("CPUID_MIDR [0x%08x]\r\n",CPUID_MIDR());
#if (__DBGLVL__ == 5)
	debug_arch = (CPUID_MIDR() >> 16) & 0xFF;
	DBG_IND("debug arch [0x%02x]\r\n", debug_arch);
#endif
	ARM_DBG_READ(c0, c2, 2, dbg_ext);

	DBG_IND("debug ext [0x%02x]\r\n", dbg_ext);
	dbg_ext |= ((1 << 21) | (1 << 14));

	ARM_DBG_WRITE(c0, c2, 2, dbg_ext);

	ARM_DBG_READ(c0, c2, 2, dbg_ext);

	DBG_IND("debug ext2 [0x%02x]\r\n", dbg_ext);

	return E_OK;

}

/*
 * Install a perf counter breakpoint.
 */
ER ca53_install_hw_breakpoint(arch_hw_breakpoint info, UINT32 trigger_type)
{
//	struct arch_hw_breakpoint *info = counter_arch_bp(bp);
//	struct perf_event **slot, **slots;
	int i, max_slots, ctrl_base, val_base;
	UINT32 addr;

	T_CA53_DBGWCRn_REG  ctrl = {0};
//	UINT32 size = 4;

	ctrl.bit.wt     = 0;
	ctrl.bit.bas    = 0;
	ctrl.bit.lsc    = trigger_type;
	ctrl.bit.pac    = 0x3;
	ctrl.bit.enable = 1;



	addr = info.address;
	//ctrl = encode_ctrl_reg(info->ctrl) | 0x1;

	if (trigger_type == ARM_BREAKPOINT_EXECUTE) {
		/* Breakpoint */
		ctrl_base = ARM_BASE_BCR;
		val_base = ARM_BASE_BVR;
//		slots = (struct perf_event **)__get_cpu_var(bp_on_reg);
		max_slots = core_num_brps;
		for (i = 0; i < max_slots; ++i) {
			if (bp_slots[i] == 0) {
				wp_slots[i] = (UINT32)&info;
				break;
			}
		}
		if (i == max_slots) {
			DBG_WRN("Can't find any breakpoint slot\n");
			return E_SYS;
		} else {
			DBG_IND("breakpoint [%d] found\r\n", i);
		}
	} else {
		/* Watchpoint */
		ctrl_base = ARM_BASE_WCR;
		val_base = ARM_BASE_WVR;
//		slots = (struct perf_event **)__get_cpu_var(wp_on_reg);
		max_slots = core_num_wrps;

		for (i = 0; i < max_slots; ++i) {
			if (wp_slots[i] == 0) {
				wp_slots[i] = (UINT32)&info;
				break;
			}
		}
		if (i == max_slots) {
			DBG_WRN("Can't find any watchpoint slot\n");
			return E_SYS;
		} else {
			DBG_IND("watchpoint [%d] found\r\n", i);
		}
		switch (info.size) {
		default:
		case 1:
			ctrl.bit.bas = (0x1 << (addr & 7));
			break;

		case 2:
			ctrl.bit.bas = (0x3 << (addr & 6));
			break;

		case 4:
			ctrl.bit.bas = (0xF << (addr & 4));
			break;

		case 8:
			ctrl.bit.bas = 0xFF;
			break;

		}
	}

#if 0


	/* Override the breakpoint data with the step data. */
	if (info->step_ctrl.enabled) {
		addr = info->trigger & ~0x3;
		ctrl = encode_ctrl_reg(info->step_ctrl);
		if (info->ctrl.type != ARM_BREAKPOINT_EXECUTE) {
			i = 0;
			ctrl_base = ARM_BASE_BCR + core_num_brps;
			val_base = ARM_BASE_BVR + core_num_brps;
		}
	}
#endif
	DBG_IND("write watchpoint [%d] found val_base[%d] ctrl_base[%d] [0x%08x]\r\n", i, val_base, ctrl_base, ctrl.reg);

	/* Setup the control register. */
	write_wb_reg(ctrl_base + i, 0);

	/* Setup the address register. */
	write_wb_reg(val_base + i, addr & 0xFFFFFFF8);

	/* Setup the control register. */
	write_wb_reg(ctrl_base + i, ctrl.reg);

	DBG_IND("val_base ctrl addr[0x%08x] val[0x%08x]\r\n", read_wb_reg(ctrl_base + i), read_wb_reg(val_base + i));
	return 0;
}

/**
     CA53 set outstanding configuration

     Configured CA53 bus outstanding

	 @param[in] brg            	DDR1/2 's bridge
     @param[in] type            Outstanding enable / disable
     @param[in] buffer_mode     fixed mode or by command
     @param[in] buffer_set      non bufferable / bufferable

     @return success or not
         - @b E_OK:   success
         - @b E_SYS:  fail
*/
ER ca9_set_outstanding(CPU_BRIDGE brg, OUTSTANDING_TYPE type, BUFFERABLE_MODE buffer_mode, BUFFERABLE_SET buffer_set)
{
	T_CA9_COMMAND_TYPE_REG    cpu_cmd_type;

	cpu_cmd_type.reg = CA9_OUTSTANDING_GETREG(CA9_COMMAND_TYPE_REG_OFS + (brg * 0x1000));

#if defined(_NVT_EMULATION_)
	DBG_DUMP("ca9_set_outstanding type[%d] buffer mode [%d] buffer set[%d]\r\n", (int)type, buffer_mode, buffer_set);
#endif
	cpu_cmd_type.bit.L2_arb1_postd = type;
	cpu_cmd_type.bit.L2_arb1_pawba_mode = buffer_mode;
	cpu_cmd_type.bit.L2_arb1_awba_set = buffer_set;


	CA9_OUTSTANDING_SETREG(CA9_COMMAND_TYPE_REG_OFS + (brg * 0x1000), cpu_cmd_type.reg);

	DBG_IND("0x2000 + 0x%04x= 0x%08x done\r\n", CA9_OUTSTANDING_GETREG(CA9_COMMAND_TYPE_REG_OFS + (brg * 0x1000)));
#if defined(_NVT_EMULATION_)
	DBG_DUMP("0x2000 + 0x%04x = 0x%08x done\r\n", (brg * 0x1000), (unsigned int) CA9_OUTSTANDING_GETREG(CA9_COMMAND_TYPE_REG_OFS + (brg * 0x1000)));
	cpu_cmd_type.reg = CA9_OUTSTANDING_GETREG(CA9_COMMAND_TYPE_REG_OFS + (brg * 0x1000));
	DBG_DUMP("	=>   Pre-read [%d] \r\n", (unsigned int)cpu_cmd_type.bit.L2_arb1_ppre_rd_off);
	DBG_DUMP("	=>Outstanding [%d] \r\n", (unsigned int)cpu_cmd_type.bit.L2_arb1_postd);

	//if(cpu_cmd_type.bit.L2_arb1_pawba_mode)
	//	DBG_DUMP("	=>By command\r\n");
	//else
	//	DBG_DUMP("	=>Fixed mode\r\n");

	//if(cpu_cmd_type.bit.L2_arb1_awba_set)
	//	DBG_DUMP("	=>Bufferable\r\n");
	//else
	//	DBG_DUMP("	=>Non-bufferable\r\n");
#endif

	return E_OK;
}

/**
     CA9 set bus timeout configuration

     Configured CA53 bus outstanding

	 @param[in] brg            	DDR1/2 's bridge
     @param[in] type            Outstanding enable / disable
     @param[in] buffer_mode     fixed mode or by command
     @param[in] buffer_set      non bufferable / bufferable

     @return success or not
         - @b E_OK:   success
         - @b E_SYS:  fail
*/
ER ca9_set_bus_timeout_enable(BOOL en)
{
	T_CA9_COMMAND_TIMEOUT_REG    cpu_cmd_timeout;

	cpu_cmd_timeout.reg = CA9_OUTSTANDING_GETREG(CA9_COMMAND_TIMEOUT_REG_OFS);

	cpu_cmd_timeout.bit.en_arb_w 	= 1;
	cpu_cmd_timeout.bit.en_arb_r 	= 1;
	cpu_cmd_timeout.bit.en_axi_aw 	= 1;
	cpu_cmd_timeout.bit.en_axi_w 	= 1;
	cpu_cmd_timeout.bit.en_axi_b 	= 1;
	cpu_cmd_timeout.bit.en_axi_ar	= 1;
	cpu_cmd_timeout.bit.en_axi_r 	= 1;

	CA9_OUTSTANDING_SETREG(CA9_COMMAND_TIMEOUT_REG_OFS, cpu_cmd_timeout.reg);

	return E_OK;
}

/**
     CA53 get outstanding configuration

     Get CA53 bus outstanding

	 @param[in] brg            	DDR1/2 's bridge
     @param[out] type           Outstanding enable / disable
     @param[out] buffer_mode    fixed mode or by command
     @param[out] buffer_set     non bufferable / bufferable

     @return success or not
         - @b E_OK:   success
         - @b E_SYS:  fail
*/
ER ca9_get_outstanding(CPU_BRIDGE brg, OUTSTANDING_TYPE *type, BUFFERABLE_MODE *buffer_mode, BUFFERABLE_SET *buffer_set)
{
	T_CA9_COMMAND_TYPE_REG    cpu_cmd_type;

	cpu_cmd_type.reg = CA9_OUTSTANDING_GETREG(CA9_COMMAND_TYPE_REG_OFS  + (brg * 0x1000));

	*type = cpu_cmd_type.bit.L2_arb1_postd;
	*buffer_mode = cpu_cmd_type.bit.L2_arb1_pawba_mode;
	*buffer_set = cpu_cmd_type.bit.L2_arb1_awba_set;
	return E_OK;
}

//trm442
void ca53_cycle_count_start(BOOL do_reset, BOOL enable_divider)
{
	// in general enable all counters (including cycle counter)
	int value = 1;

	if (cpu_count_open == TRUE) {
		DBG_WRN("cpu count start already\r\n");
		return;
	}

	cpu_count_open = TRUE;
	// peform reset:
	// peform reset:
	if (do_reset) {
		value |= 2;     // reset all counters to zero.
		value |= 4;     // reset cycle counter to zero.
	}

	if (enable_divider) {
		value |= 8;    // enable "by 64" divider for CCNT.
	}

	value |= 16;

	// program the performance-counter control-register:
	asm volatile("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(value));

	// enable all counters:
	asm volatile("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));

	// clear overflows:
	asm volatile("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));
	return;
}

/**
     CA53 get CPU clock cycle count

     get CPU clock cycle count

     @param[out] type
     @return success or not
         - @b UINT64:   clock cycle of CPU
*/
UINT32  ca53_get_cycle_count(void)
{
	//UINT64 nowl, nowu;
	//asm volatile("mrrc p15, 0, %0, %1, c14" : "=r" (nowl), "=r" (nowu));
	//To access the PMCCNTR when accessing as a 64-bit register:
	//MRRC p15,0,<Rt>,<Rt2>,c9 ; Read 64-bit PMCCNTR into Rt (low word) and Rt2 (high word)
	//MCRR p15,0,<Rt>,<Rt2>,c9 ; Write Rt (low word) and Rt2 (high word) to 64-bit PMCCNTR
	//__asm__ __volatile__("mrrc p15, 0, %0, %1, c9" : "=r" (nowl), "=r" (nowu));
	//return (UINT64)((UINT64)nowl | (UINT64)(nowu << 32));

	//Accessing the PMCCNTR:
	//To access the PMCCNTR when accessing as a 32-bit register:
	//MRC p15,0,<Rt>,c9,c13,0 ; Read PMCCNTR into Rt
	//MCR p15,0,<Rt>,c9,c13,0 ; Write Rt to PMCCNTR
	//Register access is encoded as follows:
	//UINT32    value;
	unsigned int value;
	__asm__ __volatile__("mrc p15, 0, %0, c9, c13, 0\n\t"
						 : "=r"(value)
						);
	//DBG_IND("PMCCNTR = 0x%08x\r\n", value);

	return value;
}

//trm442
void ca53_cycle_count_stop(void)
{
	T_CA53_PMCR_REG pmcr_reg;

	if (cpu_count_open == FALSE) {
		DBG_WRN("cpu count not start yet\r\n");
		return;
	}

	cpu_count_open = FALSE;
	pmcr_reg.reg = read_PMCR();

	pmcr_reg.bit.DP = 0; //increase each clock cycle
	pmcr_reg.bit.E = 0;
	write_PMCR(pmcr_reg.reg);
	return;
}
#if 1
static int available_evcount = 0;
const CHAR   *evlist_tbl[7] = {
	"Instruction cache dependent stall cycles",
	"       Data cache dependent stall cycles",
	"             Floating point instructions",
	"                       NEON instructions",
	"            Predictable function returns",
	"                           Data eviction",
	"                           Cycle counter",
};
//static int evlist[6] = {0x60, 0x61, 0x11, 0x1D, 0x16, 0x17};
//static int evlist_count = 2;
static int evdebug = 1;
void pmu_init(void) {
	available_evcount = getPMN();
	DBG_DUMP("PMU init => available event = %d\r\n", (int)available_evcount);
}

void pmu_start(unsigned int event_array[],unsigned int count)
{
	unsigned int i;
	enable_pmu();              		// 	Enable the PMU
	//reset_ccnt();              	// 	Reset the CCNT (cycle counter)
	reset_pmn();               		// 	Reset the configurable counters
	write_flags((1 << 31) | 0xf); 	//	Reset overflow flags
	for(i = 0;i < count;i ++)
	{
		pmn_config(i, event_array[i]);
	}
	//ccnt_divider(1); //Enable divide by 64
	//enable_ccnt();  // Enable CCNT
	ca53_cycle_count_start(TRUE, FALSE);

	for(i = 0;i < count;i ++)
		enable_pmn(i);
}

void pmu_stop(UINT32 usec)
{
	unsigned int i;
	unsigned int cycle_count, overflow;
	unsigned int counters = available_evcount;
	UINT8 c = 0x25;

//	unsigned long elapsed, usage;
	UINT32   freq;
	float 	rate;
	UINT32	cnt;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		freq = pll_get_pll_freq(PLL_ID_3) * 2;
		CPU_M_LOG("      DDR Freq => [%d]Hz => [%d]MHz ->  DDR[%d]\r\n", (int)freq, (int)(freq/1000000), (int)(freq*2/1000000));
		freq = pll_get_pll_freq(PLL_ID_8);
		CPU_M_LOG("CPU PLL08 Freq => [%d]Hz => [%d]MHz -> usec[%d]\r\n", (int)freq, (int)(freq/1000000), (int)usec);
	} else {
		freq = pll_get_pll_freq(PLL_ID_3) * 4;
		CPU_M_LOG("      DDR Freq => [%d]Hz => [%d]MHz ->  DDR[%d]\r\n", (int)freq, (int)(freq/1000000), (int)(freq*4/1000000));
	}



//	freq = (freq / 1000000);


//	disable_ccnt();
	ca53_cycle_count_stop();
	cycle_count = ca53_get_cycle_count();// Read CCNT

//	elapsed = (float)((float)cycle_count / (float)usec);

	//if(elapsed >= freq) {
	//	elapsed = freq;
	//}
//	usage = (float)((float)elapsed / (float)freq);

//	CPU_M_LOG("freq = %d, CPU usage %.2f\r\n", elapsed, freq, usage);

	for(i = 0;i < counters;i ++)
		disable_pmn(i);

	for(i = 0;i < counters;i ++)
	{
		if(evdebug == 1) {
			cnt = read_pmn(i);
			rate = (float)(((float)cnt * (float)100)/(float)cycle_count);
			CPU_Y_LOG("%s => [%8.4f]%c %u\r\n", evlist_tbl[i], rate, c, cnt);
		}
	}
	CPU_W_LOG("%s =>             %u\t\r\n", evlist_tbl[6], cycle_count);


	overflow = read_flags();   //Check for overflow flag

	//CPU_M_LOG("PMU[%d] => Cycle count %u\t%u\t\r\n", counters, overflow,cycle_count);

	if(evdebug == 1&&overflow)
		CPU_M_LOG("Overflow %u\t%u\t", overflow,cycle_count);
}
#endif
/**
     CA53 core2 start

     CA53 core2 start

     @return success or not
         - @b E_OK:   success
         - @b E_SYS:  fail
*/
/*
void ca53_core2_start(void)
{
	T_CA53_CORE_RESET_REG    core_reset_reg;

	core_reset_reg.reg = CPU_OSPREY_GETREG(CA53_CORE_RESET_REG_OFS);

	core_reset_reg.bit.CPUPORESET_CORE1 = 1;
	core_reset_reg.bit.CORERESET_CORE1 = 1;

	CPU_OSPREY_SETREG(CA53_CORE_RESET_REG_OFS, core_reset_reg.reg);

	return;
}
*/
/**
     CA53 core2 stop

     CA53 core2 stop

     @return success or not
         - @b E_OK:   success
         - @b E_SYS:  fail
*/
/*
void ca53_core2_stop(void)
{
	T_CA53_CORE_RESET_REG    core_reset_reg;

	core_reset_reg.reg = CPU_OSPREY_GETREG(CA53_CORE_RESET_REG_OFS);

	core_reset_reg.bit.CPUPORESET_CORE1 = 0;
	core_reset_reg.bit.CORERESET_CORE1 = 0;

	CPU_OSPREY_SETREG(CA53_CORE_RESET_REG_OFS, core_reset_reg.reg);

	return;
}*/

/*
UINT32 ca53_get_smpen(void)
{
	T_CA53_INT_STS_REG  core_sts_reg;

	core_sts_reg.reg = CPU_OSPREY_GETREG(CA53_INT_STS_REG_OFS);

	return core_sts_reg.bit.smpen;
}
*/
#if __ARM_CORTEX__
static  UINT32 cpuExceptionInfoTbl[CPU_EXP_INFO_NUM] = {0x0, 0x0, 0x0, 0x0};
#endif
static  CPU_CBEXP cpuExceptionCB  = NULL;

static UINT32 regs_value[CORE_REG_ENTRY_CNT] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0
};

UINT32 pReg_value = (UINT32) &regs_value[0];

UINT32  sys_get_srsctl_cp0(void);

/**
    @addtogroup mIDrvSys_Core
*/
//@{

/**
     Register exception call back function

     Register callback function that driver will called once if exception occurred

     @note  Once cpu core occur exception, driver layer will call this function back to \n
            upper layer

     @param[in] cpuExpCB    function point of callback function

     @return
        - @b E_OK:  register callback function success
        - @b E_PAR: callback function NULL point
*/
ER kdef_ecb(CPU_CBEXP cpuExpCB)
{
	if (cpuExpCB == NULL) {
		return E_PAR;
	}

	cpuExceptionCB = cpuExpCB;

	return E_OK;
}

#if 0
/**
     Lock semaphore while using NEON instruction


     @return
        - @b E_OK:   Done with no error.
        - Others: Error occured.
*/
ER cpu_begin_neon(void)
{
	ER erReturn;

	erReturn = wai_sem(SEMID_NEON);

	if (erReturn != E_OK) {
		return erReturn;
	}

	return E_OK;
}

/**
    Release semaphore while using NEON instruction

    Release semaphore while using NEON instruction
    @return
        - @b E_OK:   Done with no error.
        - Others: Error occured.
*/
ER cpu_end_neon(void)
{
	return sig_sem(SEMID_NEON);
}
#endif

static void pl310_cache_sync(void)
{
	PL310_REGS *const pl310 = (PL310_REGS *)IOADDR_CA9_L2_CTRL_REG_BASE;
	writel(0, &pl310->pl310_cache_sync);
}

static void pl310_background_op_all_ways(unsigned int *op_reg)
{
	PL310_REGS *const pl310 = (PL310_REGS *)IOADDR_CA9_L2_CTRL_REG_BASE;
	unsigned int assoc_16, associativity, way_mask;

	assoc_16 = readl(&pl310->pl310_aux_ctrl) &
			PL310_AUX_CTRL_ASSOCIATIVITY_MASK;
	if (assoc_16)
		associativity = 16;
	else
		associativity = 8;

	way_mask = (1 << associativity) - 1;
	/* Invalidate all ways */
	writel(way_mask, op_reg);
	/* Wait for all ways to be invalidated */
	while (readl(op_reg) && way_mask)
		;
	pl310_cache_sync();
}

static void v7_outer_cache_inval_all(void)
{
	PL310_REGS *const pl310 = (PL310_REGS *)IOADDR_CA9_L2_CTRL_REG_BASE;
	pl310_background_op_all_ways(&pl310->pl310_inv_way);
}

void v7_outer_cache_flush_all(void)
{
	PL310_REGS *const pl310 = (PL310_REGS *)IOADDR_CA9_L2_CTRL_REG_BASE;
	pl310_background_op_all_ways(&pl310->pl310_clean_inv_way);
}

void v7_outer_cache_disable(void)
{
	L2_REG1_CONTROL = (K_L2_REG1_CONTROL_EN_OFF << S_L2_REG1_CONTROL_EN);
	v7_outer_cache_inval_all();
}

ER cpu_disable_interrupt(void)
{
	// disable both irq and fiq
	__asm volatile ( "CPSID if" ::: "memory" );
	__asm volatile ( "DSB" );
	__asm volatile ( "ISB" );

	OUTW(0xFFD01080,0);
	OUTW(0xFFD01084,0);
	OUTW(0xFFD01088,0);
	OUTW(0xFFD0108C,0);
	OUTW(0xFFD01090,0); // for 'perf top' no interrupt on linux
	return E_OK;
}

ER cpu_disable_cache(void)
{
	//refer to uboot cleanup_before_linux_select() on arch/arm/cpu/armv7/cpu.c
	cpu_cleanInvalidateDCacheAll();
	v7_outer_cache_flush_all();
	DISABLE_DCACHE();
	v7_outer_cache_disable();

	/*
	* After D-cache is flushed and before it is disabled there may
	* be some new valid entries brought into the cache. We are
	* sure that these lines are not dirty and will not affect our
	* execution. (because unwinding the call-stack and setting a
	* bit in CP15 SCTRL is all we did during this. We have not
	* pushed anything on to the stack. Neither have we affected
	* any static data) So just invalidate the entire d-cache again
	* to avoid coherency problems for kernel
	*/
	cpu_invalidateDCacheAll();
	DISABLE_ICACHE();
	cpu_invalidateICacheAll();
	return E_OK;
}

ER cpu_disable_mmu(void)
{
	DISABLE_MMU();
	CLEAR_MMU();
	return E_OK;
}
//@}
