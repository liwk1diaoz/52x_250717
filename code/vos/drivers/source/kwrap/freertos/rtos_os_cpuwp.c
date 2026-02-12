#include <string.h>
#include <stdio.h>

#define __MODULE__    rtos_cpuwp
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

UINT32 cwp_address[4] = {0};
UINT32 cwp_enable[4] = {0};



#define CORESIGHT_UNLOCK        0xc5acce55

/* Debug architecture numbers. */
#define ARM_DEBUG_ARCH_RESERVED 0       /* In case of ptrace ABI updates. */
#define ARM_DEBUG_ARCH_V6       1
#define ARM_DEBUG_ARCH_V6_1     2
#define ARM_DEBUG_ARCH_V7_ECP14 3
#define ARM_DEBUG_ARCH_V7_MM    4
#define ARM_DEBUG_ARCH_V7_1     5
#define ARM_DEBUG_ARCH_V8       6

#define ARM_MISMATCH            0

/* Breakpoint */
#define ARM_BREAKPOINT_EXECUTE  0

/* Watchpoints */
#define ARM_BREAKPOINT_LOAD     1
#define ARM_BREAKPOINT_STORE    2
#define ARM_FSR_ACCESS_MASK     (1 << 11)

/* Privilege Levels */
#define ARM_BREAKPOINT_PRIV     1
#define ARM_BREAKPOINT_USER     2

/* Lengths */
#define ARM_BREAKPOINT_LEN_1    0x1
#define ARM_BREAKPOINT_LEN_2    0x3
#define ARM_BREAKPOINT_LEN_4    0xf
#define ARM_BREAKPOINT_LEN_8    0xff

/* Limits */
#define ARM_MAX_BRP             16
#define ARM_MAX_WRP             16
#define ARM_MAX_HBP_SLOTS       (ARM_MAX_BRP + ARM_MAX_WRP)

/* DSCR method of entry bits. */
#define ARM_DSCR_MOE(x)                 ((x >> 2) & 0xf)
#define ARM_ENTRY_BREAKPOINT            0x1
#define ARM_ENTRY_ASYNC_WATCHPOINT      0x2
#define ARM_ENTRY_SYNC_WATCHPOINT       0xa

/* DSCR monitor/halting bits. */
#define ARM_DSCR_HDBGEN         (1 << 14)
#define ARM_DSCR_MDBGEN         (1 << 15)

/* OSLSR os lock model bits */
#define ARM_OSLSR_OSLM0         (1 << 0)

/* opcode2 numbers for the co-processor instructions. */
#define ARM_OP2_BVR             4
#define ARM_OP2_BCR             5
#define ARM_OP2_WVR             6
#define ARM_OP2_WCR             7

#define ARM_BASE_BVR            64
#define ARM_BASE_BCR            80
#define ARM_BASE_WVR            96
#define ARM_BASE_WCR            112

/* Accessor macros for the debug registers. */
#define ARM_DBG_READ(N, M, OP2, VAL) do {\
		DBG_DUMP("mrc p14, 0, %%0, " #N ", " #M ", " #OP2 ": =r " #VAL "\r\n");\
		asm volatile("mrc p14, 0, %0, " #N ", " #M ", " #OP2 : "=r" (VAL));\
		DBG_DUMP("read " #VAL "=%08x\r\n", VAL);\
	} while (0)

#define ARM_DBG_WRITE(N, M, OP2, VAL) do {\
		DBG_DUMP("set " #VAL "=%08x\r\n", VAL);\
		DBG_DUMP("mcr p14, 0, %%0, " #N ", " #M ", " #OP2 ": : r " #VAL "\r\n");\
		asm volatile("mcr p14, 0, %0, " #N ", " #M ", " #OP2 : : "r" (VAL));\
	} while (0)

#define READ_WB_REG_CASE(OP2, M, VAL)                   \
	case ((OP2 << 4) + M):                          \
	ARM_DBG_READ(c0, c ## M, OP2, VAL);     \
	break

#define WRITE_WB_REG_CASE(OP2, M, VAL)                  \
	case ((OP2 << 4) + M):                          \
	ARM_DBG_WRITE(c0, c ## M, OP2, VAL);    \
	break

#define isb(option) __asm__ __volatile__ ("isb " #option : : : "memory")

#define GEN_READ_WB_REG_CASES(OP2, VAL)         \
	READ_WB_REG_CASE(OP2, 0, VAL);          \
	READ_WB_REG_CASE(OP2, 1, VAL);          \
	READ_WB_REG_CASE(OP2, 2, VAL);          \
	READ_WB_REG_CASE(OP2, 3, VAL);          \
	READ_WB_REG_CASE(OP2, 4, VAL);          \
	READ_WB_REG_CASE(OP2, 5, VAL);          \
	READ_WB_REG_CASE(OP2, 6, VAL);          \
	READ_WB_REG_CASE(OP2, 7, VAL);          \
	READ_WB_REG_CASE(OP2, 8, VAL);          \
	READ_WB_REG_CASE(OP2, 9, VAL);          \
	READ_WB_REG_CASE(OP2, 10, VAL);         \
	READ_WB_REG_CASE(OP2, 11, VAL);         \
	READ_WB_REG_CASE(OP2, 12, VAL);         \
	READ_WB_REG_CASE(OP2, 13, VAL);         \
	READ_WB_REG_CASE(OP2, 14, VAL);         \
	READ_WB_REG_CASE(OP2, 15, VAL)

#define GEN_WRITE_WB_REG_CASES(OP2, VAL)        \
	WRITE_WB_REG_CASE(OP2, 0, VAL);         \
	WRITE_WB_REG_CASE(OP2, 1, VAL);         \
	WRITE_WB_REG_CASE(OP2, 2, VAL);         \
	WRITE_WB_REG_CASE(OP2, 3, VAL);         \
	WRITE_WB_REG_CASE(OP2, 4, VAL);         \
	WRITE_WB_REG_CASE(OP2, 5, VAL);         \
	WRITE_WB_REG_CASE(OP2, 6, VAL);         \
	WRITE_WB_REG_CASE(OP2, 7, VAL);         \
	WRITE_WB_REG_CASE(OP2, 8, VAL);         \
	WRITE_WB_REG_CASE(OP2, 9, VAL);         \
	WRITE_WB_REG_CASE(OP2, 10, VAL);        \
	WRITE_WB_REG_CASE(OP2, 11, VAL);        \
	WRITE_WB_REG_CASE(OP2, 12, VAL);        \
	WRITE_WB_REG_CASE(OP2, 13, VAL);        \
	WRITE_WB_REG_CASE(OP2, 14, VAL);        \
	WRITE_WB_REG_CASE(OP2, 15, VAL)

/*
static UINT32 read_wb_reg(int n)
{
        UINT32 val = 0;

        switch (n) {
        GEN_READ_WB_REG_CASES(ARM_OP2_BVR, val);
        GEN_READ_WB_REG_CASES(ARM_OP2_BCR, val);
        GEN_READ_WB_REG_CASES(ARM_OP2_WVR, val);
        GEN_READ_WB_REG_CASES(ARM_OP2_WCR, val);
        default:
                debug_msg("attempt to read from unknown breakpoint register %d\n",
                        n);
        }

        return val;
}
*/

static void write_wb_reg(int n, UINT32 val)
{
	switch (n) {
		GEN_WRITE_WB_REG_CASES(ARM_OP2_BVR, val);
		GEN_WRITE_WB_REG_CASES(ARM_OP2_BCR, val);
		GEN_WRITE_WB_REG_CASES(ARM_OP2_WVR, val);
		GEN_WRITE_WB_REG_CASES(ARM_OP2_WCR, val);
	default:
		DBG_DUMP("attempt to write to unknown breakpoint register %d\n",
				  n);
	}
	isb();
}

/* Determine debug architecture. */
static UINT8 get_debug_arch(void)
{
	UINT32 didr;
#if 0
	/* Do we implement the extended CPUID interface? */
	if (((read_cpuid_id() >> 16) & 0xf) != 0xf) {
		DBG_ERR("CPUID feature registers not supported. "
				"Assuming v6 debug is present.\n");
		return ARM_DEBUG_ARCH_V6;
	}
#endif
	ARM_DBG_READ(c0, c0, 0, didr);
	DBG_DUMP("debug_arch %d\r\n", (didr >> 16) & 0xf);
	return (didr >> 16) & 0xf;
}

/*
 * In order to access the breakpoint/watchpoint control registers,
 * we must be running in debug monitor mode. Unfortunately, we can
 * be put into halting debug mode at any time by an external debugger
 * but there is nothing we can do to prevent that.
 */
static int monitor_mode_enabled(void)
{
	UINT32 dscr;
	ARM_DBG_READ(c0, c1, 0, dscr);
	return !!(dscr & ARM_DSCR_MDBGEN);
}

static int enable_monitor_mode(void)
{
	UINT32 dscr;

	ARM_DBG_READ(c0, c1, 0, dscr);

	/* If monitor mode is already enabled, just return. */
	if (dscr & ARM_DSCR_MDBGEN) {
		goto out;
	}

	/* Write to the corresponding DSCR. */
	switch (get_debug_arch()) {
	case ARM_DEBUG_ARCH_V6:
	case ARM_DEBUG_ARCH_V6_1:
		ARM_DBG_WRITE(c0, c1, 0, (dscr | ARM_DSCR_MDBGEN));
		break;
	case ARM_DEBUG_ARCH_V7_ECP14:
	case ARM_DEBUG_ARCH_V7_1:
	case ARM_DEBUG_ARCH_V8:
		ARM_DBG_WRITE(c0, c2, 2, (dscr | ARM_DSCR_MDBGEN));
		isb();
		break;
	default:
		return -1;
	}

	/* Check that the write made it through. */
	ARM_DBG_READ(c0, c1, 0, dscr);
	if (!(dscr & ARM_DSCR_MDBGEN)) {
		DBG_WRN("Failed to enable monitor mode on CPU.\n");
		return -2;
	}
out:
	return 0;
}

static BOOL has_ossr = FALSE;

static BOOL core_has_os_save_restore(void)
{
	UINT32 oslsr;

	switch (get_debug_arch()) {
	case ARM_DEBUG_ARCH_V7_1:
		return TRUE;
	case ARM_DEBUG_ARCH_V7_ECP14:
		ARM_DBG_READ(c1, c1, 4, oslsr);
		if (oslsr & ARM_OSLSR_OSLM0) {
			return TRUE;
		}
	default:
		return FALSE;
	}
}

static void reset_ctrl_regs(void)
{
	int i;
	//int raw_num_brps;
	int core_num_wrps;
	int err = 0, cpu = 0;
	UINT32 val;

	/*
	 * v7 debug contains save and restore registers so that debug state
	 * can be maintained across low-power modes without leaving the debug
	 * logic powered up. It is IMPLEMENTATION DEFINED whether we can access
	 * the debug registers out of reset, so we must unlock the OS Lock
	 * Access Register to avoid taking undefined instruction exceptions
	 * later on.
	 */
	switch (get_debug_arch()) {
	case ARM_DEBUG_ARCH_V6:
	case ARM_DEBUG_ARCH_V6_1:
		/* ARMv6 cores clear the registers out of reset. */
		goto out_mdbgen;
	case ARM_DEBUG_ARCH_V7_ECP14:
		/*
		 * Ensure sticky power-down is clear (i.e. debug logic is
		 * powered up).
		 */
		ARM_DBG_READ(c1, c5, 4, val);
		if ((val & 0x1) == 0) {
			err = -1;
		}

		if (!has_ossr) {
			goto clear_vcr;
		}
		break;
	case ARM_DEBUG_ARCH_V7_1:
		/*
		 * Ensure the OS double lock is clear.
		 */
		ARM_DBG_READ(c1, c3, 4, val);
		if ((val & 0x1) == 1) {
			err = -1;
		}
		break;
	}

	if (err) {
		DBG_ERR("CPU %d debug is powered down!\n", cpu);
		return;
	}

	/*
	 * Unconditionally clear the OS lock by writing a value
	 * other than CS_LAR_KEY to the access register.
	 */
	ARM_DBG_WRITE(c1, c0, 4, ~CORESIGHT_UNLOCK);
	isb();

	/*
	 * Clear any configured vector-catch events before
	 * enabling monitor mode.
	 */
clear_vcr:
	ARM_DBG_WRITE(c0, c7, 0, 0);
	isb();

	/*
	 * The control/value register pairs are UNKNOWN out of reset so
	 * clear them to avoid spurious debug events.
	 */
	/*
	raw_num_brps = 4; //get_num_brp_resources();
	for (i = 0; i < raw_num_brps; ++i) {
	        write_wb_reg(ARM_BASE_BCR + i, 0UL);
	        write_wb_reg(ARM_BASE_BVR + i, 0UL);
	}
	*/
	core_num_wrps = 4; //get_num_wrp_resources();
	for (i = 0; i < core_num_wrps; ++i) {
		write_wb_reg(ARM_BASE_WCR + i, 0UL);
		write_wb_reg(ARM_BASE_WVR + i, 0UL);
	}

	/*
	 * Have a crack at enabling monitor mode. We don't actually need
	 * it yet, but reporting an error early is useful if it fails.
	 */
out_mdbgen:
	enable_monitor_mode();

}

void vos_cpu_enable_watch(int i, UINT32 addr, UINT32 size)
{
	UINT32 byte_address_select = 0;
	if (i >= 4) {
		DBG_ERR("max 4 wp!\r\n");
		return;
	}
	has_ossr = core_has_os_save_restore();
	if (!monitor_mode_enabled()) {
		reset_ctrl_regs();
	}
	switch (size) {
	case 1:
		if (addr & 0x03) {
			DBG_ERR("addr = 0x%08x not align size %d\r\n", addr, size);
			return;
		}
		byte_address_select = ARM_BREAKPOINT_LEN_1;
		break;
	case 2:
		if (addr & 0x03) {
			DBG_ERR("addr = 0x%08x not align size %d\r\n", addr, size);
			return;
		}
		byte_address_select = ARM_BREAKPOINT_LEN_2;
		break;
	case 4:
		if (addr & 0x03) {
			DBG_ERR("addr = 0x%08x not align size %d\r\n", addr, size);
			return;
		}
		byte_address_select = ARM_BREAKPOINT_LEN_4;
		break;
	case 8:
		if (addr & 0x07) {
			DBG_ERR("addr = 0x%08x not align size %d\r\n", addr, size);
			return;
		}
		byte_address_select = (ARM_BREAKPOINT_LEN_8);
		break;
	default:
		DBG_ERR("Not support size %d, only 1, 2, 4, or 8\r\n", size);
		return;
	}
	//clear
	write_wb_reg(ARM_BASE_WCR + i, 0);
	//set
	write_wb_reg(ARM_BASE_WVR + i, addr);
	write_wb_reg(ARM_BASE_WCR + i, (1) | (1 << ARM_BREAKPOINT_PRIV) | (1 << ARM_BREAKPOINT_USER) | (ARM_BREAKPOINT_STORE << 3) | ((byte_address_select & 0xFF) << 5));
	DBG_DUMP("enable cpu wp[%d] on data write addr = 0x%08x, size %d\r\n", i, addr, size);
}

void vos_cpu_disable_watch(int i)
{
	if (i >= 4) {
		DBG_ERR("max 4 wp!\r\n");
		return;
	}
	//clear
	write_wb_reg(ARM_BASE_WCR + i, 0UL);
	write_wb_reg(ARM_BASE_WVR + i, 0UL);
	DBG_DUMP("disable cpu wp[%d]\r\n", i);
}

