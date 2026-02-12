/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 * Copyright (C) 2006 Jen-Yu Yu <babyfish@vstream.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
//#include <asm/ucontext.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>

#include "sysdef.h"     /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>

#include <rtp.h>
#include <config.h>


// int http_init(void);


static int init_random(void)
{
	srandom(time(0));
	return 0;
}

void random_bytes(unsigned char *dest, int len)
{
	int i;

	for (i = 0; i < len; ++i) {
		dest[i] = random() & 0xff;
	}
}

void random_id(unsigned char *dest, int len)
{
	int i;

	for (i = 0; i < len / 2; ++i)
		sprintf((char *)(dest + i * 2), "%02X",
				(unsigned int)(random() & 0xff));
	dest[len] = 0;
}


int spook_init(void)
{
	spook_log_init(SL_VERBOSE);      // SL_DEBUG, SL_INFO, SL_VERBOSE
	if (init_random() < 0) {
		return -1;
	}
	// live_init();
	// http_init();

	return 0;
}

