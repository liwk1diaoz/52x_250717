/**
	@brief Sample code of video record.\n

	@file video_record.c

	@author Boyan Huang

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include <netinet/in.h>
#include <net/if.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

void rtspd_start(int port);
void rtspd_send_frame(HD_PATH_ID enc_path,HD_VIDEOENC_BS* p_video_bitstream,HD_VIDEOENC_BUFINFO phy_buf_main,UINT32 vir_addr_main);
void rtspd_stop(void);
