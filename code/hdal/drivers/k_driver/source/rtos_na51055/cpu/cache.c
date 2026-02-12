/*
    CPU module driver.

    This file is the driver of CPU module.

    @file       Cache.c
    @ingroup    mIDrvSys_Core
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include <kwrap/error_no.h>
#include <kwrap/spinlock.h>
#define __MODULE__    rtos_cpu
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

#include <stdio.h>
#include "plat/cpu.h"
#include "cpu_cp_reg.h"
#include "cpu_cp_int.h"

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define _Y_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_YELLOW fmt DBG_COLOR_END, ##args)
#define _R_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_RED fmt DBG_COLOR_END, ##args)
#define _M_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_MAGENTA fmt DBG_COLOR_END, ##args)
#define _G_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_GREEN fmt DBG_COLOR_END, ##args)
#define _W_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_WHITE fmt DBG_COLOR_END, ##args)
#define _X_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_HI_GRAY fmt DBG_COLOR_END, ##args)
#define _B_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_BLUE fmt DBG_COLOR_END, ##args)
#define _C_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_CYAN fmt DBG_COLOR_END, ##args)

//#define CACHE_IFO_MSG(...)               debug_msg(__VA_ARGS__)

//#define CACHE_RED_MSG(...)               debug_msg("^R\033[1m"__VA_ARGS__)

//#define CACHE_LITEBLUE_MSG(...)          debug_msg("^M\033[1m"__VA_ARGS__)

//#define CACHE_YELLOW_MSG(...)            debug_msg("^Y\033[1m"__VA_ARGS__)

/**
    @addtogroup mIDrvSys_Core
*/
//@{
static UINT32  sys_dcache_lines     = _DC_SETS; //256
static UINT32  sys_dcache_linesize  = _DC_LS;   //32
static UINT32  sys_dcache_assoc     = _DC_W;    //4


static UINT32  sys_icache_lines     = _IC_SETS; //256
static UINT32  sys_icache_linesize  = _IC_LS;   //32
static UINT32  sys_icache_assoc     = _IC_W;    //4

#if 0 //<== 512KB
static UINT32  sys_l2_cache_lines   = 512;
static UINT32  sys_l2_cache_linesize= 64;
static UINT32  sys_l2_cache_assoc   = 16;
#else
/* 512 sets */
/* 8 way */
/* 32 byte line size */
static UINT32  sys_l2_cache_lines   = _SC_SETS; //512
static UINT32  sys_l2_cache_linesize= _SC_LS;   //32
	   UINT32  sys_l2_cache_assoc   = _SC_W;    //8


#endif
static ER _cache_invalidate_data_cache_all(CACHE_LV_TYPE level_type, BOOL DSB);
static ER _cache_clean_invalidate_d_cache_All(CACHE_LV_TYPE level_type, BOOL DSB);

/*
 * Common code for all cache controllers.
 */
static inline void cpu_l2_wait_mask(UINT32 reg, UINT32 mask)
{
	/* wait for cache operation by line or way to complete */
	while (INW(reg) & mask);

}


static void cpu_l2_cache_wait_sync_done(void)
{
	UINT32 sync;

	sync = L2_REG7_CACHE_SYNC;

	while(sync & 0x1) {
		sync = L2_REG7_CACHE_SYNC;
//		DBG_DUMP(".");
	}

}
static void cpu_cleanInvalidateL2cacheBlock(UINT32 start, UINT32 end)
{
    UINT32  size = end - start;
	UINT32  addr = (start & ~(_SC_LS - 1));
	unsigned long flags = 0;
    loc_cpu(flags);
    cpu_l2_cache_wait_sync_done();
	for(;addr < ((start + size + (_SC_LS - 1)) & ~(_SC_LS - 1)); addr += _SC_LS)
	{
		L2_REG7_CLEAN_INV_PA = addr;
	}
    cpu_l2_wait_mask(L2_REG7_BASE + 0xF0, 1);

	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
    cpu_l2_cache_wait_sync_done();
    unl_cpu(flags);
}

static void cpu_invalidateL2cacheBlock(UINT32 start, UINT32 end)
{
    UINT32  size = end - start;
	UINT32  addr = (start & ~(_SC_LS - 1));

	unsigned long flags = 0;
    loc_cpu(flags);
    cpu_l2_cache_wait_sync_done();
	L2_REG7_CLEAN_INV_PA = addr;
	L2_REG7_CLEAN_INV_PA = ((start + size) & ~(_SC_LS - 1));
	for(;addr < ((start + size) & ~(_SC_LS - 1)); addr += _SC_LS)
	{
		L2_REG7_INV_PA = addr;
	}
    cpu_l2_wait_mask(L2_REG7_BASE + 0x70, 1);

	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
    cpu_l2_cache_wait_sync_done();
    unl_cpu(flags);
}

static void cpu_cleanL2cacheBlock(UINT32 start, UINT32 end)
{
    UINT32  size = end - start;
	UINT32  addr = (start & ~(_SC_LS - 1));
	unsigned long flags = 0;
    loc_cpu(flags);
    cpu_l2_cache_wait_sync_done();
	for(;addr < ((start + size + (_SC_LS - 1)) & ~(_SC_LS - 1)); addr += _SC_LS)
	{
		L2_REG7_CLEAN_PA = addr;
	}
    cpu_l2_wait_mask(L2_REG7_BASE + 0xB0, 1);
	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
    cpu_l2_cache_wait_sync_done();
    unl_cpu(flags);
}

/*
    Invalidate all data cache by level

    Invalidate all data cache by level
    All cache lines will be marked as invalidated (cache miss).
\n  For compatible, equal to CPUCleanInvalidateDCacheAll()

    @note One cache line = 64 bytes (16 words)
        sel_CSSELR(0) level 1 data cache
        sel_CSSELR(1) level 1 ins cache
        sel_CSSELR(2) level 2 data cache

    @return void
*/

static ER _cache_invalidate_data_cache_all(CACHE_LV_TYPE level_type, BOOL DSB)
{
	int way, sets, ws;
	int totalWay = 0;
	int wayShift = 0;
	int totalSet = 0;
	int lineSize = 0;
	unsigned long flags = 0;

	if (level_type != LEVEL_1_DCACHE && level_type != LEVEL_2_DCACHE) {
		DBG_ERR("Invalidate level[%d] @ inv d cache\r\n", level_type);
		return E_SYS;
	}

	if (level_type == LEVEL_1_DCACHE) {
		totalWay = sys_dcache_assoc;
		wayShift = S_DC_W;
		totalSet = sys_dcache_lines;
		lineSize = sys_dcache_linesize;
        for (way = 0; way < totalWay; way++) {
    		ws = (way << (32 - wayShift)) + level_type;   //Way shift
    		for (sets = 0; sets < totalSet; sets++) {
    			ws += lineSize;
    			_DCISW(ws);
    		}
    	}
	} else {
		/* Invalidate all ways of L2 cache */
        loc_cpu(flags);
        cpu_l2_cache_wait_sync_done();
    	if((M_L2_REG0_CACHE_TYPE_DA >> S_L2_REG0_CACHE_TYPE_DA) == K_L2_REG0_CACHE_TYPE_DA_16WAY)
    	{
    		L2_REG7_INV_WAY = K_L2_REG7_INV_WAY_16WAY;
    		while((L2_REG7_INV_WAY & K_L2_REG7_INV_WAY_16WAY))
    		{
    		}
    	}
    	else //((M_L2_REG0_CACHE_TYPE_DA >> S_L2_REG0_CACHE_TYPE_DA) == K_L2_REG0_CACHE_TYPE_DA_8WAY)
    	{
    		L2_REG7_INV_WAY = K_L2_REG7_INV_WAY_8WAY;
    		while((L2_REG7_INV_WAY & K_L2_REG7_INV_WAY_8WAY))
    		{
    		}
    	}
    	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
        cpu_l2_cache_wait_sync_done();
        unl_cpu(flags);
	}


	/* DSB to make sure the operation is complete */
	if (DSB) {
		_DSB();
	}

	return E_OK;
}


/*
    Clean all data cache by Set/Way

    Clean all data cache by level
    All cache lines will be marked as invalidated (cache miss).
\n  For compatible, equal to CPUCleanInvalidateDCacheAll()

    @note One cache line = 64 bytes (16 words)
        sel_CSSELR(0) level 1 data cache
        sel_CSSELR(1) level 1 ins cache
        sel_CSSELR(2) level 2 data cache

    @return void
*/

static ER _cache_clean_d_cache_all(CACHE_LV_TYPE level_type, BOOL DSB)
{
	int way, sets, ws;
	int totalWay = 0;
	int wayShift = 0;
	int totalSet = 0;
	int lineSize = 0;
	unsigned long flags = 0;

	if (level_type != LEVEL_1_DCACHE && level_type != LEVEL_2_DCACHE) {
		DBG_ERR("Invalidate level[%d] @ inv d cache\r\n", level_type);
		return E_SYS;
	}

	if (level_type == LEVEL_1_DCACHE) {
		totalWay = sys_dcache_assoc;
		wayShift = S_DC_W;
		totalSet = sys_dcache_lines;
		lineSize = sys_dcache_linesize;
        for (way = 0; way < totalWay; way++) {
    		ws = (way << (32 - wayShift)) + level_type;   //Way shift
    		for (sets = 0; sets < totalSet; sets++) {
    			ws += lineSize;
    			_DCCSW(ws);
    		}
    	}
	} else {
		/* Invalidate all ways of L2 cache */
        loc_cpu(flags);
        cpu_l2_cache_wait_sync_done();
    	if((M_L2_REG0_CACHE_TYPE_DA >> S_L2_REG0_CACHE_TYPE_DA) == K_L2_REG0_CACHE_TYPE_DA_16WAY)
    	{
    		L2_REG7_CLEAN_WAY = K_L2_REG7_CLEAN_WAY_16WAY;
    		while((L2_REG7_CLEAN_WAY & K_L2_REG7_CLEAN_WAY_16WAY))
    		{
    		}
    	}
    	else //((M_L2_REG0_CACHE_TYPE_DA >> S_L2_REG0_CACHE_TYPE_DA) == K_L2_REG0_CACHE_TYPE_DA_8WAY)
    	{
    		L2_REG7_CLEAN_WAY = K_L2_REG7_CLEAN_WAY_8WAY;
    		while((L2_REG7_CLEAN_WAY & K_L2_REG7_CLEAN_WAY_8WAY))
    		{
    		}
    	}
    	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
        cpu_l2_cache_wait_sync_done();
        unl_cpu(flags);
	}


	/* DSB to make sure the operation is complete */
	if (DSB) {
		_DSB();
	}

	return E_OK;
}

/*
    Clean & invalidate all data cache by level

    Clean & invalidate all data cache by level
    the required sequence is
    1. CleanLevel1 Address
    2. DSB
    3. CleanLevel2 Address
    4. Cache sync
    5. InvalLevel2 Address
    6. CACHE SYNC
    7. InvalLevel1 Address
    8. DSB
\n  For compatible, equal to CPUCleanInvalidateDCacheAll()

    @note One cache line = 64 bytes (16 words)
        sel_CSSELR(0) level 1 data cache
        sel_CSSELR(1) level 1 ins cache
        sel_CSSELR(2) level 2 data cache

    @return void
*/
static ER _cache_clean_invalidate_d_cache_All(CACHE_LV_TYPE level_type, BOOL DSB)
{
	int way, sets, ws;
	int totalWay = 0;
	int wayShift = 0;
	int totalSet = 0;
	int lineSize = 0;
	unsigned long flags = 0;

	if (level_type != LEVEL_1_DCACHE && level_type != LEVEL_2_DCACHE) {
		DBG_ERR("Invalidate level[%d] @ inv d cache\r\n", level_type);
		return E_SYS;
	}

	if (level_type == LEVEL_1_DCACHE) {
		totalWay = sys_dcache_assoc;
		wayShift = S_DC_W;
		totalSet = sys_dcache_lines;
		lineSize = sys_dcache_linesize;
        for (way = 0; way < totalWay; way++) {
    		ws = (way << (32 - wayShift)) + level_type;
    		for (sets = 0; sets < totalSet; sets++) {
    			ws += lineSize;
    			_DCCISW(ws);
    		}
    	}
	} else {
		/* Invalidate all ways of L2 cache */
        loc_cpu(flags);
        cpu_l2_cache_wait_sync_done();
    	if((M_L2_REG0_CACHE_TYPE_DA >> S_L2_REG0_CACHE_TYPE_DA) == K_L2_REG0_CACHE_TYPE_DA_16WAY)
    	{
    		L2_REG7_CLEAN_INV_WAY = K_L2_REG7_INV_WAY_16WAY;
    		while((L2_REG7_CLEAN_INV_WAY & K_L2_REG7_INV_WAY_16WAY))
    		{
    		}
    	}
    	else //((M_L2_REG0_CACHE_TYPE_DA >> S_L2_REG0_CACHE_TYPE_DA) == K_L2_REG0_CACHE_TYPE_DA_8WAY)
    	{
    		L2_REG7_CLEAN_INV_WAY = K_L2_REG7_INV_WAY_8WAY;
    		while((L2_REG7_CLEAN_INV_WAY & K_L2_REG7_INV_WAY_8WAY))
    		{
    		}
    	}
    	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
        cpu_l2_cache_wait_sync_done();
        unl_cpu(flags);
	}


	if (DSB) {
		_DSB();
	}
	return E_OK;
}


/*
    Display CPU cache information

    Include cache enable or not, cache line / line size / assoc

    Provide upper layer to make sure cache configuration is correct.

    @return void
*/
void cpu_showCacheConfiguration(void)
{
	UINT32  conf;
	UINT32  ls, a, ns;
//	UINT32  clidr, level_of_coherence, cache_type, level_start_bit = 0;
//	UINT32  level;
//	UINT32  cbar;
    UINT32  L2_ctrl;
    UINT32  L2_aux;
    UINT32  L2_wsize;

	//select to L1 data cache [trm p4-177 CSSELR]
	sel_CSSELR(LEVEL_1_DCACHE);
	conf = CCSIDR();

	ls = (1 << (((conf & M_CCSIDR_LS) >> S_CCSIDR_LS) + 2));
	a = (((conf & M_CCSIDR_A) >> S_CCSIDR_A) + 1);
	ns = (((conf & M_CCSIDR_SETS) >> S_CCSIDR_SETS) + 1);

	//sys_dcache_lines    = ns;
	//sys_dcache_linesize = (ls << 2);
	//sys_dcache_assoc    = a;

	DBG_DUMP("Data Cache size: %d KB\r\n", (int)((ls * a * ns * 4) / 1024));

	if (sys_dcache_linesize != (ls<<2)) {
		DBG_ERR("=>line size: %d words %d bytes\r\n", (int)ls, (int)sys_dcache_linesize);
		DBG_ERR("<=line size: %d words %d bytes\r\n", (int)(sys_dcache_linesize >> 2), (int)sys_dcache_linesize);
	} else {
		DBG_DUMP("line size: %d words %d bytes\r\n", (int)ls, (int)sys_dcache_linesize);
	}

	if (sys_dcache_assoc != a) {
		DBG_ERR("=>Associativity: %d\r\n", (int)a);
		DBG_ERR("<=Associativity: %d\r\n", (int)sys_dcache_assoc);
	} else {
		DBG_DUMP("Associativity: %d\r\n", (int)a);
	}

	if (sys_dcache_lines != ns) {
		DBG_ERR("=>Number of sets: %d\r\n", (int)ns);
		DBG_ERR("<=Number of sets: %d\r\n", (int)sys_dcache_lines);
	} else {
		DBG_DUMP("Number of sets: %d\r\n", (int)ns);
	}


	if ((conf & M_CCSIDR_WT)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Write-Through\r\n");


	if ((conf & M_CCSIDR_WB)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Write-Back\r\n");
	if ((conf & M_CCSIDR_WA)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Write-Allocation\r\n");
	if ((conf & M_CCSIDR_RA)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Read-Allocation\r\n");

	//select to L1 instruction cache [trm p4-177 CSSELR]
	sel_CSSELR(LEVEL_1_ICACHE);
	conf = CCSIDR();

	ls = (1 << (((conf & M_CCSIDR_LS) >> S_CCSIDR_LS) + 2));
	a = (((conf & M_CCSIDR_A) >> S_CCSIDR_A) + 1);
	ns = (((conf & M_CCSIDR_SETS) >> S_CCSIDR_SETS) + 1);

//	sys_icache_lines    = ns;
//	sys_icache_linesize = (ls << 2);
//	sys_icache_assoc    = a;

	DBG_DUMP("Instruction Cache size: %d KB\r\n", (int)((ls * a * ns * 4) / 1024));
	if (sys_icache_linesize != (ls<<2)) {
		DBG_ERR("=>line size: %d words %d bytes\r\n", (int)ls, (int)sys_icache_linesize);
		DBG_ERR("<=line size: %d words %d bytes\r\n", (int)(sys_icache_linesize >> 2), (int)sys_icache_linesize);
	} else {
		DBG_DUMP("line size: %d words %d bytes\r\n", (int)ls, (int)sys_icache_linesize);
	}

	if (sys_icache_assoc != a) {
		DBG_ERR("=>Associativity: %d\r\n", (int)a);
		DBG_ERR("<=Associativity: %d\r\n", (int)sys_icache_assoc);
	} else {
		DBG_DUMP("Associativity: %d\r\n", (int)a);
	}

	if (sys_icache_lines != ns) {
		DBG_ERR("=>Number of sets: %d\r\n", (int)ns);
		DBG_ERR("<=Number of sets: %d\r\n", (int)sys_icache_lines);
	} else {
		DBG_DUMP("Number of sets: %d\r\n", (int)ns);
	}


	DBG_DUMP("line size: %d words %d bytes\r\n", (int)ls, (int)sys_icache_linesize);
	DBG_DUMP("Associativity: %d\r\n", (int)a);
	DBG_DUMP("Number of sets: %d\r\n", (int)ns);


	if ((conf & M_CCSIDR_WT)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Write-Through\r\n");


	if ((conf & M_CCSIDR_WB)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Write-Back\r\n");
	if ((conf & M_CCSIDR_WA)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Write-Allocation\r\n");
	if ((conf & M_CCSIDR_RA)) {
		DBG_DUMP("S");
	} else {
		DBG_DUMP("Not s");
	}
	DBG_DUMP("upport Read-Allocation\r\n");

	//select to L2 cache [trm p4-177 CSSELR]
	L2_ctrl = L2_REG0_CACHE_TYPE;
    L2_aux = L2_REG1_AUX_CTRL;
//	sel_CSSELR(LEVEL_2_DCACHE);
//	conf = CCSIDR();

//	ls = (1 << (((conf & M_CCSIDR_LS) >> S_CCSIDR_LS) + 2));
//	a = (((conf & M_CCSIDR_A) >> S_CCSIDR_A) + 1);
//	ns = (((conf & M_CCSIDR_SETS) >> S_CCSIDR_SETS) + 1);
    if((L2_ctrl & 0x3) == 0x0) {
        ls = 32;
    } else {
        _R_LOG(" unknow L2 cache line size\r\n");
    }
    if((L2_aux & (0x1 << 16)))
        a = 16;
    else
        a = 8;


    L2_wsize = ((L2_ctrl & 0x700000)>>20);

    if(L2_wsize == 0x0)
        L2_wsize = (0x4000 << (L2_wsize));
    else
        L2_wsize = (0x4000 << (L2_wsize-1));

    ns = L2_wsize / ls;

    DBG_DUMP("L2 Cache size: %d KB <=> %d KB\r\n", (int)((L2_wsize * a) >> 10), (int)(sys_l2_cache_lines * sys_l2_cache_linesize * sys_l2_cache_assoc >> 10));

    if(sys_l2_cache_linesize != ls)
        _R_LOG("L2 line siz mismatch => ");
    else
        _G_LOG("L2 line siz match =>");

	DBG_DUMP("line size: %d words %d bytes\r\n", (int)ls, (int)sys_icache_linesize);

    if(sys_l2_cache_assoc != a)
        _R_LOG("L2 ass mismatch =>");
    else
        _G_LOG("L2 ass match =>");

    DBG_DUMP("Associativity: %d\r\n", (int)a);

    if(sys_l2_cache_lines  != ns)
        _R_LOG("L2 set mismatch");
    else
        _G_LOG("L2 set match");
	DBG_DUMP("Number of sets: %d <=> %d\r\n", (int)ns, (int)sys_l2_cache_lines);

    if(!(L2_ctrl & (0x1<<24)))
        DBG_DUMP("=>Unified\r\n");
    else
        DBG_DUMP("=>Harvard\r\n");

	_G_LOG("              AUX ctrl = 0x%08x\r\n", (int)L2_REG1_AUX_CTRL);
	_G_LOG("         Prefetch ctrl = 0x%08x\r\n", (int)L2_REG15_PREF_CTRL);
  	_G_LOG("  Latency TAG RAM ctrl = 0x%08x\r\n", (int)L2_REG1_TAG_RAM_CTRL);
  	_G_LOG(" Latency DATA RAM ctrl = 0x%08x\r\n", (int)L2_REG1_DATA_RAM_CTRL);


}
//@}


/**
    @name   Cache clean/invalidate API(v)
*/
//@{
/*
    Invalidate all instruction cache.

    Invalidate all instruction cache.
    All cache lines will be marked as invalidated (cache miss).

    @note One cache line = 32 bytes (8 words)

    @return void
*/
void cpu_invalidateICacheAll(void)
{
	_ICACHE_INV_ALL();
	_DSB();
	_ISB();
}


/*
    Invalidate instruction cache of specific line that match address.

    Invalidate instruction cache of specific line that match address.
    The cache line will be marked as invalidated (cache miss).

    @note One cache line = 32 bytes (8 words)

    @param[in] addr     The address that you want to invalidate.
    @return void
*/
void cpu_invalidateICache(UINT32 addr)
{
	_ICACHE_INV_MVAU(addr);
	_ISB();
}


/*
    Invalidate instruction cache of specific line(s) that match address.

    Invalidate instruction cache of specific line(s) that match address
    from start to end. The cache line will be marked as invalidated (cache miss).

    @note One cache line = 32 bytes (8 words)

    @param[in] start    The starting address that you want to invalidate.
    @param[in] end      The end address that you want to invalidate.
    @return void
*/
void cpu_invalidateICacheBlock(UINT32 start, UINT32 end)
{
	UINT32 addr = (start & ~(_IC_LS - 1));

	for (; addr < (end & ~(_IC_LS - 1)); addr += _IC_LS) {
		_ICACHE_INV_MVAU(addr);
	}
	_ISB();

}

/*
    Invalidate all data cache.

    Invalidate all data cache.
    All cache lines will be marked as invalidated (cache miss).
\n  For compatible, equal to CPUCleanInvalidateDCacheAll()

    @note One cache line = 32 bytes (8 words)

    @return void
*/
void cpu_invalidateDCacheAll(void)
{
	_cache_invalidate_data_cache_all(LEVEL_1_DCACHE, TRUE);
	_cache_invalidate_data_cache_all(LEVEL_2_DCACHE, FALSE);
	_DSB();
}


/*
    Invalidate data cache of specific line that match address.

    Invalidate data cache of specific line that match address.
    The cache line will be marked as invalidated (cache miss).
\n  For compatible, equal to CPUCleanInvalidateDCache()

    @note One cache line = 32 bytes (8 words)

    @param[in] addr     The address that you want to invalidate.
    @return void
*/
void cpu_invalidateDCache(UINT32 addr)
{
	unsigned long flags = 0;
	_DCACHE_INV_MVAC(addr);

    _DSB();
    loc_cpu(flags);
    cpu_l2_cache_wait_sync_done();
    L2_REG7_INV_PA = addr;
	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
    cpu_l2_cache_wait_sync_done();
    unl_cpu(flags);
	_DSB();
}


/*
    Invalidate data cache of specific line(s) that match address.

    Invalidate data cache of specific line(s) that match address
    from start to end. The cache line will be marked as invalidated (cache miss).
\n  For compatible, equal to CPUCleanInvalidateDCacheBlock()

    @note One cache line = 32 bytes (8 words)

    @param[in] start    The starting address that you want to invalidate.
    @param[in] end      The end address that you want to invalidate.
    @return void
*/
void cpu_invalidateDCacheBlock(UINT32 start, UINT32 end)
{
	UINT32 tmp = (_DC_LS - 1);
	UINT32 addr;

	addr = (start & ~(tmp));

	// cahcle line size alignment
	if (end & tmp) {
		end &= ~tmp;
		end += _DC_LS;
	}

	//_DCCIMVAC(addr) //clean and invalidate data cache line by VA
	//_DCACHE_WBACK_INV_MVAC(addr);
	//_DCACHE_WBACK_INV_MVAC((end) & ~(_DC_LS - 1));

	for (; addr < end; addr += _DC_LS) {
		//Invalidate data cache line by VA
		_DCACHE_INV_MVAC(addr);
	}

    _DSB();
    cpu_invalidateL2cacheBlock(start, end);

	_DSB();
}


/*
    Clean data cache of specific line that match address.

    Clean data cache of specific line that match address.
    The cache line will be flushed to DRAM if dirty.
\n  For compatible, equal to CPUCleanDCache()

    @note One cache line = 64 bytes (8 words)

    @param[in] addr     The address that you want to clean.
    @return void
*/
void cpu_cleanDCache(UINT32 addr)
{
	unsigned long flags = 0;
	addr = addr & ~(_DC_LS - 1);
	_DCACHE_WBACK_MVAC(addr);

	_DSB();
    loc_cpu(flags);
    cpu_l2_cache_wait_sync_done();
    L2_REG7_CLEAN_PA = addr;
	L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
    cpu_l2_cache_wait_sync_done();
    unl_cpu(flags);
	_DSB();
}

/*
    Clean data cache of specific line(s) that match address.

    Clean data cache of specific line(s) that match address from start to end.
    The cache line(s) will be flushed to DRAM if dirty.
\n  For compatible, equal to CPUCleanDCacheBlock()

    @note One cache line = 32 bytes (8 words)

    @param[in] start    The starting address that you want to clean.
    @param[in] end      The end address that you want to clean.
    @return void
*/
void cpu_cleanDCacheBlock(UINT32 start, UINT32 end)
{
	UINT32 addr = (start & ~(_DC_LS - 1));

	for (; addr < ((end + (_DC_LS - 1)) & ~(_DC_LS - 1)); addr += _DC_LS) {
		_DCACHE_WBACK_MVAC(addr);
	}

    _DSB();

    cpu_cleanL2cacheBlock(start, end);

	_DSB();
}

/*
    Clean and invalidate all data cache.

    Clean and invalidate all data cache.
    All cache lines will be marked as invalidated (cache miss) and flushed
    to DRAM if dirty.
    1. CleanL1
    2. DSB
    3. Clean&InvalidateL2
    4. Cache sync
    5. Clean&InvalidateL1
    6. DSB

\n  For compatible, equal to CPUCleanInvalidateDCacheAll()

    @note One cache line = 32 bytes (8 words)

    @return void
*/
void cpu_cleanInvalidateDCacheAll(void)
{
	//_cache_clean_invalidate_d_cache_All(LEVEL_1_DCACHE, FALSE);
	_cache_clean_d_cache_all(LEVEL_1_DCACHE, TRUE);
	_cache_clean_invalidate_d_cache_All(LEVEL_2_DCACHE, TRUE);
    _cache_clean_invalidate_d_cache_All(LEVEL_1_DCACHE, TRUE);
}


/*
    Clean and invalidate data cache of specific line that match address.

    Clean and invalidate data cache of specific line that match address.
    The cache line will be marked as invalidated (cache miss) and flushed
    to DRAM if dirty.

    @note One cache line = sys_dcache_linesize depend on MIPS configuration

    @param[in] addr     The address that you want to clean and invalidate.
    @return void
*/
void cpu_cleanInvalidateDCache(UINT32 addr)
{
	unsigned long flags = 0;
	addr = addr & ~(_DC_LS - 1);
	_DCACHE_WBACK_INV_MVAC(addr);
    _DSB();

    loc_cpu(flags);
    cpu_l2_cache_wait_sync_done();
    L2_REG7_CLEAN_INV_PA = addr;
    L2_REG7_CACHE_SYNC = K_L2_REG7_CACHE_SYNC_C;
    cpu_l2_cache_wait_sync_done();
    unl_cpu(flags);
	_DSB();
}

/*
    Clean and invalidate data cache of specific line(s) that match address.

    Clean and invalidate data cache of specific line(s) that match address
    form start to end. The cache line will be marked as invalidated
    (cache miss) and flushed to DRAM if dirty.

    @note One cache line = sys_dcache_linesize depend on MIPS configuration

    @param[in] start    The starting address that you want to clean and invalidate.
    @param[in] end      The end address that you want to clean and invalidate.
    @return void
*/
void cpu_cleanInvalidateDCacheBlock(UINT32 start, UINT32 end)
{
	UINT32 addr = (start & ~(_DC_LS - 1));

	for (; addr < ((end + (_DC_LS - 1)) & ~(_DC_LS - 1)); addr += _DC_LS) {
		_DCACHE_WBACK_INV_MVAC(addr);
	}

    _DSB();

    cpu_cleanInvalidateL2cacheBlock(start, end);

	_DSB();

}

/*
    Invalidate L1 cache

    Invalidate all data cache.
    All cache lines will be marked as invalidated (cache miss).
\n  For compatible, equal to CPUCleanInvalidateDCacheAll()

    @note One cache line = 32 bytes (8 words)

    @return void
*/
void cpu_invalidateDCacheL1(void)
{
	_cache_invalidate_data_cache_all(LEVEL_1_DCACHE, FALSE);
	_DSB();
}

void cpu_invalidateDCacheL2(void)
{
	_cache_invalidate_data_cache_all(LEVEL_2_DCACHE, FALSE);
	_DSB();
}

void cpu_v7_outer_cache_disable(void)
{
	L2_REG1_CONTROL = 0x0;
}

void cpu_v7_outer_cache_enable(void)
{
	L2_REG1_CONTROL = 0x1;
}


/**
    @name   Cache lock/unlock API
*/
//@{

/**
    CPU lock down data cache.

    CPU lock down specific data into data cache

    @note One cache line = sys_dcache_linesize depend on MIPS configuration

    @param[in] start    The starting address that you want to lock down into data cache
    @param[in] end      The end address that you want to lock down into data cache
    @return void
*/
ER cpu_lockDownDCache(UINT32 start, UINT32 end)
{
	//_lockdownDCache(start, end);
	//return E_OK;
	return E_NOSPT;
}

/**
    CPU lock down instruction cache.

    CPU lock down specific data into instruction cache

    @note One cache line = sys_icache_linesize depend on MIPS configuration

    @param[in] start    The starting address that you want to lock down into data cache
    @param[in] end      The end address that you want to lock down into data cache
    @return void
*/
ER cpu_lockDownICache(UINT32 start, UINT32 end)
{
	//_lockdownICache(start, end);
	//return E_OK;
	return E_NOSPT;
}

/* Invalidate TLB */
void cpu_inval_tlb(void)
{
	/* Invalidate entire unified TLB */
	asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0));
	/* Invalidate entire data TLB */
	asm volatile("mcr p15, 0, %0, c8, c6, 0" : : "r"(0));
	/* Invalidate entire instruction TLB */
	asm volatile("mcr p15, 0, %0, c8, c5, 0" : : "r"(0));

	/* Full system DSB - make sure that the invalidation is complete */
	_DSB();

	/* Full system ISB - make sure the instruction stream sees it */
	_ISB();
}
//@}
