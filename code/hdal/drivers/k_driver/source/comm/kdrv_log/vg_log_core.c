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
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
#include <comm/hwclock.h>
#include <comm/log.h>
#include "vg_log_core.h"

#define MAX_PRINTK 1023

#if defined(__KERNEL__)
#define PRINTF printk
#define MALLOC(x) kmalloc((x), GFP_ATOMIC)
#define FREE(x) kfree((x))
#else
#define PRINTF printf
#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

unsigned int hdal_version = 0;
unsigned int impl_version = 0;

char *mmap_msg = 0;

unsigned int log_base_start = 0, log_base_start_pa = 0, log_base_ddr = 0;
unsigned int log_bsize = 0;
unsigned int log_base_end = 0;
unsigned int log_start_ptr = 0;
unsigned int log_real_size = 0;
unsigned int log_ksize = 1;

unsigned int log_slice_ptr[MAX_DUMP_FILE], log_size[MAX_DUMP_FILE];
char log_path[MAX_DUMP_FILE][MAX_PATH_WIDTH];

int mode = MODE_PRINT; //-1:disable  MODE_STORAGE:storage  MODE_CONSOLE:dump console  MODE_PRINT:direct print

static VK_DEFINE_SPINLOCK(printm_lock);

unsigned int get_gm_jiffies(void)
{
	unsigned long long curr_us = hwclock_get_longcounter();

#if defined(__FREERTOS)
	curr_us /= 1000000;
#else
	do_div(curr_us, 1000000); // curr_us to curr_ms
#endif
	return (unsigned int)(curr_us);
}

void print_chars(char *log, int size)
{
	int i;
	PRINTF("\"");
	for (i = 0; i < size; i++) {
		PRINTF("%c", log[i]);
	}
	PRINTF("\"\n");
}

int putlog_safe(char *log, int size)
{
	if ((log_start_ptr + size) >= log_base_end) { //sanity check
		PRINTF("Error log_start_ptr 0x%x end 0x%x size 0x%x\n", log_start_ptr, log_base_end, size);
	}

	memcpy((void *)log_start_ptr, (void *)log, size);
	log_start_ptr = log_start_ptr + size;

	if (log_start_ptr == log_base_end) {
		log_start_ptr = log_base_start;
	} else if (log_start_ptr == (log_base_end - 1)) {
		*(unsigned char *)log_start_ptr = (char)0;
		log_start_ptr = log_base_start;
	}
	return size;
}

void putlog(char *log, int size)
{
	int remain_size;
	int log_size;

	remain_size = size;
	while ((remain_size > 0) && (log_start_ptr + remain_size >= log_base_end - 1)) { //overflow
		log_size = log_base_end - log_start_ptr - 1;
		log = log + putlog_safe(log, log_size);
		if (mode != 0) { //print to console
			putlog_safe("", 1);    //put 0 in the end of log
		}
		remain_size = remain_size - log_size;
	}

	if (remain_size) {
		putlog_safe(log, remain_size);
		if (log_start_ptr == log_base_end - 1) //last one
			if (mode != 0) { //print to console
				putlog_safe("", 1);    //put 0 in the end of log
			}
	}

	if (log_real_size < log_bsize) { //update log size
		if (log_real_size + size >= log_bsize) {
			log_real_size = log_bsize;
		} else {
			log_real_size += size;
		}
	}
}


void printm(char *module, const char *fmt, ...)
{
	va_list args;
	int len, prelen = 0;
	char log[MAX_CHAR], module_str[2];
	char jiffies_char[6];
	unsigned long flags;

#if 0
	if (test_bit(BIT_BUSY_WRITE, log_state)) {
		return;
	}
	if (test_bit(BIT_BUSY_DUMP_BUF, log_state)) {
		return;
	}
	if (in_busyloop) {
		return;
	}
	busyloop_detection(module);
#endif
	if (mode < 0) {
		return;
	}

	vk_spin_lock_irqsave(&printm_lock, flags);

	if (module) {
		if (strlen(module) != 2) {
			PRINTF("Error module naming %s (should be 2 chars)\n", module);
			module_str[0] = '0';
			module_str[1] = '0';
		} else {
			module_str[0] = module[0];
			module_str[1] = module[1];
		}

		sprintf(jiffies_char, "%04x", (int)get_gm_jiffies() & 0xffff);
		log[prelen++] = '[';
		log[prelen++] = jiffies_char[0];
		log[prelen++] = jiffies_char[1];
		log[prelen++] = jiffies_char[2];
		log[prelen++] = jiffies_char[3];
		log[prelen++] = ']';

#if 0
		if ((feature & CPUID_LOG) == 0x2) {
			log[prelen++] = '[';
			log[prelen++] = smp_processor_id() + 0x30;
			log[prelen++] = irqs_disabled() ? 'i' : 'I';
			log[prelen++] = ']';
		}
#endif
		log[prelen++] = '[';
		log[prelen++] = module_str[0];
		log[prelen++] = module_str[1];
		log[prelen++] = ']';
	}

	va_start(args, fmt);
	len = vsnprintf((void *)(log + prelen), (sizeof(log) - prelen), fmt, args) + prelen;
	va_end(args);

	if (mode == MODE_PRINT) {
		PRINTF("%s", log);
	} else {
		putlog(log, len);
		if (mode != 0) { //print to console
			putlog("", 1);
		}
	}

	vk_spin_unlock_irqrestore(&printm_lock, flags);
}

/*
Log Format:
    .............,\0xa,......................,\0xa,......,\0xa,........,\0x0
    ^                     ^                        ^                    ^
    log_base_start        log_print_start          real_print_start     log_end_start
 */
void calculate_log(unsigned int *log_start, unsigned int *log_size)
{
	unsigned int start, size = 0, offset = 0;

	if (log_real_size < log_bsize) {
		start = log_base_start;
		size = log_real_size;
	} else {
		start = log_start_ptr;

search_again:
		while (*(unsigned char *)start != 0xa) {
			start++;
			offset++;
			if (start == log_base_end) {
				start = log_base_start;
			}
			if (offset >= log_real_size) {
				goto end_search;
			}
		}
		start++;
		offset++;
		if (start == log_base_end) {
			start = log_base_start;
			goto search_again;
		}

		if (offset >= log_real_size) {
			goto end_search;
		}

		size = log_real_size - offset;
	}

end_search:
	*log_start = start;
	*log_size = size;
	return;
}

void prepare_dump_storage(unsigned int log_print_ptr, unsigned int size)
{
	log_slice_ptr[0] = log_print_ptr;
	if (size > (log_base_end - log_print_ptr)) {
		log_size[0] = log_base_end - log_print_ptr;
		log_slice_ptr[1] = log_base_start;
		log_size[1] = size - (log_base_end - log_print_ptr);
	} else {
		log_size[0] = size;
		log_slice_ptr[1] = log_size[1] = 0;
	}
}

