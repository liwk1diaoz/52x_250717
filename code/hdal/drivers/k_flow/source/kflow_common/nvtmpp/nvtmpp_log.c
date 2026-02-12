#if defined __LINUX
#include <stdarg.h>                   // for va_list
#include <linux/string.h>             // for memcpy
#else
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#endif
#include "nvtmpp_debug.h"
#include "nvtmpp_log.h"


#if defined __LINUX
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

#if defined __LINUX
#define LOG_BUFF_COUNT                    3
#define LOG_BUFF_LEN                    512
#else
#define LOG_BUFF_COUNT                    5
#define LOG_BUFF_LEN                    250
#endif


static     UINT32       buf_id;
static     char         log_buf[LOG_BUFF_COUNT][LOG_BUFF_LEN+1];
static     char         tmp_buf[LOG_BUFF_LEN+1];
static     int          cur_str_len = 0, cur_remain_len = LOG_BUFF_LEN;
static     NVTMPP_DUMP  dump_func  = vk_printk;



void nvtmpp_log_open(NVTMPP_DUMP dump)
{
	UINT32 i;

	buf_id = 0;
	cur_str_len = 0;
	cur_remain_len = LOG_BUFF_LEN;
	dump_func = dump;
	for (i = 0; i < LOG_BUFF_COUNT; i++) {
		log_buf[i][0] = 0;
	}
}

void nvtmpp_log_close(void)
{

}

static void nvtmpp_log_reset(void)
{
	return nvtmpp_log_open(dump_func);
}


static int nvtmpp_log_change_buf(void)
{
	buf_id++;
	if (buf_id >= LOG_BUFF_COUNT) {
		buf_id = LOG_BUFF_COUNT - 1;
		return -1;
	}
	cur_str_len = 0;
	cur_remain_len = LOG_BUFF_LEN;
	//vk_printk("\r\nchange_buf\r\n");
	return 0;
}


void nvtmpp_log_dump(void)
{
	UINT32 i;

	//vk_printk("\r\n log dump\r\n");
	for (i = 0; i < LOG_BUFF_COUNT; i++) {
		if (log_buf[i][0] != 0) {
			//vk_printk("\r\ndump buf_id(%d)\r\n",i);
			dump_func(log_buf[i]);
		}
	}
	nvtmpp_log_reset();
}

int nvtmpp_log_save_str(const char *fmtstr, ...)
{
	int     len;
	int     ret;
	va_list marker;

	/* Initialize variable arguments. */
	va_start(marker, fmtstr);

	#if 0
	if (cur_remain_len <= LOG_BUFF_REV_LEN) {
		ret = nvtmpp_log_change_buf();
		if (ret < 0) {
			goto len_err;
		}
    }
	#endif
	//len = vsnprintf(&log_buf[buf_id][cur_str_len], cur_remain_len, fmtstr, marker);
	len = vsnprintf(&tmp_buf[0], cur_remain_len, fmtstr, marker);
	//vk_printk("\r\nlen=%d cur_remain_len = %d, tmp_buf = %s\r\n",len, cur_remain_len, tmp_buf);
	if (len <= 0 || len >= cur_remain_len) {
		//log_buf[buf_id][cur_str_len] = 0;
		ret = nvtmpp_log_change_buf();
		if (ret < 0) {
			nvtmpp_log_dump();
		}
		//len = vsnprintf(&log_buf[buf_id][cur_str_len], cur_remain_len, fmtstr, marker);
		//vk_printk("\r\n 1 buf_id = %d, log_buf = %s\r\n", buf_id, log_buf[buf_id]);
		len = vsnprintf(&tmp_buf[0], cur_remain_len, fmtstr, marker);
		if (len <= 0 || len >= cur_remain_len) {
			goto len_err;
		}
	}
	memcpy(&log_buf[buf_id][cur_str_len], tmp_buf, len);
	cur_str_len += len;
	cur_remain_len -= len;
	log_buf[buf_id][cur_str_len] = 0;
	//vk_printk("\r\n 2 buf_id = %d, cur_str_len = %d, log_buf = %s\r\n", buf_id, cur_str_len, log_buf[buf_id]);
	/* Reset variable arguments. */
	va_end(marker);
	return 0;
len_err:
	va_end(marker);
	nvtmpp_log_dump();
	//vk_printk("\r\nbuffer full\r\n");
	return -1;
}






