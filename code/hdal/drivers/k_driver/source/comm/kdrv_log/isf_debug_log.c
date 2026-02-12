/**
	@brief Source file of isf debug buffer.

	@file isf_debug_log.c

	@ingroup isf_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#if defined(__FREERTOS)
#else
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#endif
#include <kwrap/spinlock.h>
#include <kwrap/debug.h>
#include "isf_debug_log.h"
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define MAX_ONE_LINE_SIZE 1023 ///< make one line by vsnprintf
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
#if defined(__KERNEL__)
#define PRINTF printk
#define MALLOC(x) vmalloc((x))
#define FREE(x) vfree((x))
#else
#define PRINTF printf
#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#if defined(__FREERTOS)
#else
static char *m_buf_log = NULL; // ring buffer
static char *m_buf_log_earliest = NULL; // buffer that keep the earliest log
static char *m_buf_vpf = NULL; //for vsnprintf
static int m_buf_log_size = 0;
static int m_buf_log_earliest_size = 0;
static int m_ofs = 0; // current offset
static int m_len = 0; // current lenth that no flushed
static int m_is_backup_earliest = 0;
static VK_DEFINE_SPINLOCK(print_lock);
#endif
/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

#if defined(__FREERTOS)
int isf_debug_log_open(int buffer_size)
{
	return 0;
}

int isf_debug_log_close(void)
{
	return 0;
}

int isf_debug_log_print(const char *fmt, ...)
{
	return 0;
}

int isf_debug_log_dump(void *s)
{
	return 0;
}
#else
int isf_debug_log_open(int buffer_size)
{
	if (m_buf_log) {
		DBG_WRN("open twice? \n");
		FREE(m_buf_log);
		m_buf_log = NULL;
		m_buf_log_size = 0;
	}
	if (m_buf_vpf) {
		FREE(m_buf_vpf);
		m_buf_vpf = NULL;
	}
	m_buf_log_size = buffer_size;
	m_buf_log = (char *)MALLOC(m_buf_log_size);
	if (m_buf_log == NULL) {
		DBG_ERR("failed to alloc m_buf_log.\n");
		return -1;
	}
	m_buf_vpf = (char *)MALLOC(MAX_ONE_LINE_SIZE);
	if (m_buf_vpf == NULL) {
		DBG_ERR("failed to alloc m_buf_vpf.\n");
		FREE(m_buf_log);
		m_buf_log = NULL;
		return -1;
	}
	m_buf_log_earliest_size = m_buf_log_size/4;
	m_buf_log_earliest = (char *)MALLOC(m_buf_log_earliest_size);
	if (m_buf_log_earliest == NULL) {
		DBG_ERR("failed to alloc m_buf_log_earliest.\n");
		FREE(m_buf_log);
		m_buf_log = NULL;
		FREE(m_buf_vpf);
		m_buf_vpf = NULL;
		return -1;
	}

	memset(m_buf_log, 0x0, m_buf_log_size);
	memset(m_buf_log_earliest, 0x0, m_buf_log_earliest_size);
	return 0;
}

int isf_debug_log_close(void)
{
	if (m_buf_log) {
		FREE(m_buf_log);
		m_buf_log = NULL;
		m_buf_log_size = 0;
	}
	if (m_buf_vpf) {
		FREE(m_buf_vpf);
		m_buf_vpf = NULL;
	}
	if (m_buf_log_earliest) {
		FREE(m_buf_log_earliest);
		m_buf_log_earliest = NULL;
		m_buf_log_earliest_size = 0;
	}
	m_ofs = 0;
	m_len = 0;
	return 0;
}

int isf_debug_log_print(const char *fmt, ...)
{
	int len;
	va_list ap;
	unsigned long flags;

	if (m_buf_log == NULL || m_buf_vpf == NULL) {
		return -1;
	}

	vk_spin_lock_irqsave(&print_lock, flags);

	memset(m_buf_vpf, 0x0, MAX_ONE_LINE_SIZE);
	va_start(ap, fmt);
	len = vsnprintf(m_buf_vpf, MAX_ONE_LINE_SIZE, fmt, ap);
	va_end(ap);

	if (len < 0) {
		DBG_ERR("too long msg large than %d characters\n", MAX_ONE_LINE_SIZE);
		vk_spin_unlock_irqrestore(&print_lock, flags);
		return -2;
	}

	if (m_len < m_buf_log_size) {
		// consider ring buffer not full yet
		if (m_len + len < m_buf_log_size) {
			memcpy(m_buf_log + m_len, m_buf_vpf, len);
			m_len += len;
		} else {
			int part0_len = m_buf_log_size - m_len;
			int part1_len = len - part0_len;

			if (m_is_backup_earliest == 0) {
				memcpy(m_buf_log_earliest, m_buf_log, m_buf_log_earliest_size);
				m_is_backup_earliest = 1;
			}
			memcpy(m_buf_log + m_len, m_buf_vpf, part0_len);
			memcpy(m_buf_log, m_buf_vpf+part0_len, part1_len);
			m_len = m_buf_log_size;
			m_ofs = part1_len;
		}
	} else {
		// consider ring buffer has full
		if (m_ofs + len < m_buf_log_size) {
			memcpy(m_buf_log + m_ofs, m_buf_vpf, len);
			m_ofs += len;
		} else {
			int part0_len = m_buf_log_size - m_ofs;
			int part1_len = len - part0_len;
			memcpy(m_buf_log + m_ofs, m_buf_vpf, part0_len);
			memcpy(m_buf_log, m_buf_vpf+part0_len, part1_len);
			m_ofs = part1_len;
		}
	}
	vk_spin_unlock_irqrestore(&print_lock, flags);

	return 0;
}

static void _isf_debug_log_dump_p(struct seq_file *sfile, char *msg)
{
	int len;
	char *curr = NULL;
	const char delimiters[] = {0x0A, 0x0D, '\0'};
	char *argv[1] = {0};
	int quit = 0;
	int total_len;
	total_len = strlen(msg); //max len of msg

	curr = msg; //set current
	while (!quit) {
		char *cnext = curr; //next head
		argv[0] = strsep(&cnext, delimiters); //separate this line & next head
		if (argv[0] == NULL) {  //not exist
			quit = 1;
		} else {
			//get len of this line
			len = strlen(argv[0]);
			//printk("curr 0x%x, len 0x%x, msg 0x%x, total_len 0x%x\r\n", (int)curr, (int)len, (int)msg, (int)total_len);
			if (curr+len >= msg+total_len) { // end of this line is behind end of msg?
				//this line is out of range
				quit = 1;
			} else {
				//this line is inside range
				if (len > 0) {
					if (sfile) {
						seq_printf(sfile, "%s\r\n", argv[0]);
					} else {
						printk(argv[0]);
					}
				}
				curr = cnext; //seek current to next head
			}
		}
	}
}

int isf_debug_log_dump(void *s)
{
	//int len;
	char *msg = NULL;
	//char *curr = NULL;
	unsigned long flags;

	if (m_buf_log == NULL || m_buf_vpf == NULL) {
		DBG_ERR("nothing can be dump.\n");
		return -1;
	}

	msg = MALLOC(m_buf_log_size + 1);
	if (msg == NULL) {
		DBG_ERR("failed to alloc output msg buffer: 0x%X bytes", m_buf_log_size);
		return -1;
	}
	if (m_is_backup_earliest) {
		seq_printf(s, "-------------------------earliest log ----------------------------------------------------------\r\n");
		memcpy((void *)msg, (void *)(m_buf_log_earliest), m_buf_log_earliest_size);
		msg[m_buf_log_earliest_size] = 0;
		_isf_debug_log_dump_p(s, msg);
	}
	//printk("m_len = %d, m_ofs = %d\r\n", m_len, m_ofs);
	vk_spin_lock_irqsave(&print_lock, flags);
	if (m_ofs == 0) {
		memcpy((void *)msg, (void *)(m_buf_log), m_len);
		msg[m_len] = 0;
	} else {
		memcpy((void *)msg, (void *)(m_buf_log + m_ofs), (m_len - m_ofs));
		memcpy((void *)(msg + m_len - m_ofs), (void *)m_buf_log, m_ofs);
		msg[m_len] = 0;
	}
	vk_spin_unlock_irqrestore(&print_lock, flags);
	seq_printf(s, "-------------------------ringbuff log ----------------------------------------------------------\r\n");
	_isf_debug_log_dump_p(s, msg);
	#if 0
	{
		const char delimiters[] = {0x0A, 0x0D, '\0'};
		char *argv[1] = {0};
		int quit = 0;
		int total_len;
		total_len = strlen(msg); //max len of msg

		curr = msg; //set current
		while (!quit) {
			char *cnext = curr; //next head
			argv[0] = strsep(&cnext, delimiters); //separate this line & next head
			if (argv[0] == NULL) {  //not exist
				quit = 1;
			} else {
				//get len of this line
				len = strlen(argv[0]);
				if (curr+len >= msg+total_len) { // end of this line is behind end of msg?
					//this line is out of range
					quit = 1;
				} else {
					//this line is inside range
					if (len > 0) {
						if (s) {
							seq_printf(s, "%s\r\n", argv[0]);
						} else {
							printk(argv[0]);
						}
					}
					curr = cnext; //seek current to next head
				}
			}
		}
	}
	#endif
	FREE(msg);
	return 0;
}
#endif
