#include "../include/msdcnvt.h"
#include "msdcnvt_int.h"
#include "msdcnvt_ipc.h"
#include "msdcnvt_uitron.h"

#if defined(WIN32)
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys\timeb.h>
#elif defined(__LINUX)
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#if (CFG_VIA_IPC)
#include <nvtipc.h>
#endif
#if defined(__LINUX_660)
#include <protected/nvtmem_protected.h>
#endif
#elif defined(__ECOS)
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <cyg/nvtipc/NvtIpcAPI.h>
#include <cyg/hal/Hal_cache.h>
#include <cyg/compat/uitron/uit_func.h>
#define CYGNUM_MAX_PORT_NUM 65535
#endif

#define CFG_MAX_CLIENT 8

#define CBW_SIGNATURE  0x43425355
#define CBW_FLAG_IN    0x80
#define CBW_FLAG_OUT   0x00

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

typedef struct _MSDCNVT_CLIENT {
	int client_socket;
	struct addrinfo res;
	long last_tv_sec;
} MSDCNVT_CLIENT, *PMSDCNVT_CLIENT;

int m_server_socket = INVALID_SOCKET;
MSDCNVT_CLIENT m_client_list[CFG_MAX_CLIENT] = { 0 };

#if defined(WIN32)
static LONG initialize_lock_gettimeofday = 0;
int gettimeofday(struct timeval *tp, int *tz)
{
	static LARGE_INTEGER tick_freq, epoch_ofs;
	static BOOL is_initialized = FALSE;

	LARGE_INTEGER tick_now;

	QueryPerformanceCounter(&tick_now);

	if (!is_initialized) {
		if (1 != InterlockedIncrement(&initialize_lock_gettimeofday)) {
			InterlockedDecrement(&initialize_lock_gettimeofday);
			// wait until first caller has initialized static values
			while (!is_initialized) {
				Sleep(1);
			}
		} else {
			// For our first call, use "ftime()", so that we get a time with a proper epoch.
			// For subsequent calls, use "QueryPerformanceCount()", because it's more fine-grain.
			struct timeb tb;

			ftime(&tb);
			tp->tv_sec = (long)tb.time;
			tp->tv_usec = (long)(1000 * tb.millitm);

			// Also get our counter frequency:
			QueryPerformanceFrequency(&tick_freq);

			//NVT_MODIFIED
			UINT64 tm64 = tick_now.QuadPart * 1000000 / tick_freq.QuadPart;

			tp->tv_sec = (long)(tm64 / 1000000);
			tp->tv_usec = (long)(tm64 % 1000000);

			// compute an offset to add to subsequent counter times, so we get a proper epoch:
			epoch_ofs.QuadPart
				= tp->tv_sec * tick_freq.QuadPart + (tp->tv_usec * tick_freq.QuadPart) / 1000000L - tick_now.QuadPart;

			// next caller can use ticks for time calculation
			is_initialized = TRUE;
			return 0;
		}
	}
	UINT64 tm64 = tick_now.QuadPart * 1000000 / tick_freq.QuadPart;

	tp->tv_sec = (long)(tm64 / 1000000);
	tp->tv_usec = (long)(tm64 % 1000000);
	return 0;
}

#endif

static int make_socket_non_blocking(int sock)
{
#if defined(_WIN32)
	unsigned long arg = 1;

	return ioctlsocket(sock, FIONBIO, &arg);
#elif defined(__ECOS)
	int arg = 1;

	return ioctl(sock, FIONBIO, (int)&arg);
#elif defined(__LINUX)
	int cur_flags = fcntl(sock, F_GETFL, 0);

	return fcntl(sock, F_SETFL, cur_flags | O_NONBLOCK);
#endif
}

static int msdcnvt_init_ipc(MSDCNVT_IPC_CFG *p_ipc_cfg, unsigned int ipc_size)
{
	if (ipc_size < sizeof(MSDCNVT_IPC_CFG)) {
		DBG_ERR("MSDCNVT_IPC_CFG not match.\r\n");
		return -1;
	}

	if (p_ipc_cfg->ipc_version != MSDCNVT_IPC_VERSION) {
		DBG_ERR("MSDCNVT_IPC_VERSION not match. %08X(uitron) : %08X(app)\r\n"
				, p_ipc_cfg->ipc_version
				, MSDCNVT_IPC_VERSION);
		return -1;
	}
	return 0;
}

static int msdcnvt_init_socket(MSDCNVT_IPC_CFG *p_ipc_cfg)
{
#if defined(WIN32)
	WSADATA wsa_data;

	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

	int i, err;
	int port_num = p_ipc_cfg->port;

	// check if port number valid
#if defined(__ECOS)
	if (port_num > CYGNUM_MAX_PORT_NUM) {
		DBG_ERR("error port number %d\r\n", port_num);
		return -1;
	}
#endif
	DBG_DUMP("MSDCNVT port=%d\r\n", port_num);

	//init network
	for (i = 0; i < CFG_MAX_CLIENT; i++) {
		m_client_list[i].client_socket = INVALID_SOCKET;
	}

	struct addrinfo *res;
	struct addrinfo server_address;

	memset(&server_address, 0, sizeof(server_address));
	//server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
	server_address.ai_family = AF_INET;
	server_address.ai_socktype = SOCK_STREAM;
	server_address.ai_flags = AI_PASSIVE; // fill in my IP for me

	char   port_num_str[10];

	sprintf(port_num_str, "%d", port_num);
	err = getaddrinfo(NULL, port_num_str, &server_address, &res);
	if (err != 0) {
		DBG_ERR("getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	DBG_IND("create socket\r\n");
	m_server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (m_server_socket <= 0) {
		DBG_ERR("Socket create failed\n");
		freeaddrinfo(res);   // free the linked list
		return -1;
	}

	int flag = 1;

	err = setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (err == -1) {
		DBG_ERR("Couldn't setsockopt(SO_REUSEADDR)\n");
		freeaddrinfo(res);   // free the linked list
		return -1;
	}

#ifdef SO_REUSEPORT
	err = setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
	if (err == -1) {
		DBG_ERR("Couldn't setsockopt(SO_REUSEPORT)\n");
		freeaddrinfo(res);   // free the linked list
		return -1;
	}
#endif

	if (p_ipc_cfg->snd_buf_size != 0) {
		err = setsockopt(m_server_socket, SOL_SOCKET, SO_SNDBUF, (char *)&p_ipc_cfg->snd_buf_size,
						 sizeof(p_ipc_cfg->snd_buf_size));
		if (err == -1) {
			DBG_ERR("Couldn't setsockopt(SO_SNDBUF)\n");
			freeaddrinfo(res);   // free the linked list
			return -1;
		}
	}

	if (p_ipc_cfg->rcv_buf_size != 0) {
		err = setsockopt(m_server_socket, SOL_SOCKET, SO_RCVBUF, (char *)&p_ipc_cfg->rcv_buf_size,
						 sizeof(p_ipc_cfg->rcv_buf_size));
		if (err == -1) {
			DBG_ERR("Couldn't setsockopt(SO_RCVBUF)\n");
			freeaddrinfo(res);   // free the linked list
			return -1;
		}
	}

	DBG_IND("bind %d\r\n", port_num);
	err = bind(m_server_socket, res->ai_addr, res->ai_addrlen);

	if (err != 0) {
		DBG_ERR("bind() returned error\n");
		freeaddrinfo(res);   // free the linked list
		return -1;
	}

	DBG_IND("listen\r\n");
	err = listen(m_server_socket, SOMAXCONN);
	if (err != 0) {
		DBG_ERR("listen() returned error\n");
		freeaddrinfo(res);   // free the linked list
		return -1;
	}

	freeaddrinfo(res);   // free the linked list

	return 0;
}

static int msdcnvt_server_event(MSDCNVT_IPC_CFG *p_ipc_cfg, fd_set *p_master, int *p_fdmax)
{
	int i, ret;
	int max_idle_time = 0;
	MSDCNVT_CLIENT *p_client = NULL;
	//find a empty client
	for (i = 0; i < CFG_MAX_CLIENT; i++) {
		MSDCNVT_CLIENT *p = &m_client_list[i];

		if (p->client_socket == INVALID_SOCKET) {
			p_client = p;
			break;
		} else if (p->last_tv_sec > max_idle_time) {
			max_idle_time = p->last_tv_sec;
			p_client = p;
		}
	}

	//check if kick out a client because of being full.
	if (p_client->client_socket != INVALID_SOCKET) {
		DBG_WRN("kick out socket=%d, idle=%d sec\r\n"
				, p_client->client_socket
				, p_client->last_tv_sec);

		FD_CLR(p_client->client_socket, p_master);
		closesocket(p_client->client_socket);
		p_client->client_socket = INVALID_SOCKET;
	}

	//accept new client
	struct sockaddr client_address;
	struct timeval tv = { 0 };
	socklen_t calen = sizeof(client_address);

	DBG_IND("accept begin\r\n");
	p_client->client_socket = accept(m_server_socket, &client_address, &calen);
	DBG_IND("accept end client_socket=%d\n", p_client->client_socket);

	if (p_client->client_socket == INVALID_SOCKET) {
		DBG_ERR("failed to accept client\r\n");
		return -1;
	}

	if (p_client->client_socket > *p_fdmax) {
		*p_fdmax = p_client->client_socket;
	}

	FD_SET(p_client->client_socket, p_master);

	gettimeofday(&tv, NULL);
	p_client->last_tv_sec = tv.tv_sec;

	if (p_ipc_cfg->snd_buf_size != 0) {
		ret = setsockopt(p_client->client_socket, SOL_SOCKET, SO_SNDBUF, (char *)&p_ipc_cfg->snd_buf_size,
						 sizeof(p_ipc_cfg->snd_buf_size));
		if (ret == -1) {
			DBG_ERR("Couldn't setsockopt(SO_SNDBUF)\n");
			//no need to return
		}
	}
	if (p_ipc_cfg->rcv_buf_size != 0) {
		ret = setsockopt(p_client->client_socket, SOL_SOCKET, SO_RCVBUF, (char *)&p_ipc_cfg->rcv_buf_size,
						 sizeof(p_ipc_cfg->rcv_buf_size));
		if (ret == -1) {
			DBG_ERR("Couldn't setsockopt(SO_RCVBUF)\n");
			//no need to return
		}
	}
	ret = make_socket_non_blocking(p_client->client_socket);
	if (ret == -1) {
		//coverity[returned_null]		
		//coverity[dereference]
		DBG_ERR("%s: ioctl errno = %d\r\n", __func__, errno);
		//no need to return
	}
	if (p_ipc_cfg->tos != 0) {
		int  tos = p_ipc_cfg->tos;

		ret = setsockopt(p_client->client_socket, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(tos));
		if (ret == -1) {
			DBG_ERR("Couldn't setsockopt(IP_TOS)\n");
			//no need to return
		}
		DBG_IND("tos = %d\n", tos);
	}

	return 0;
}

static int msdcnvt_client_recv(int socket_recv, void *p_data, unsigned int n_byte)
{
	int n, n_recv = 0;
	int n_retry = 0;
	struct timeval tv;
	fd_set master;
	fd_set readfds;

	FD_ZERO(&master);
	FD_ZERO(&readfds);
	FD_SET(socket_recv, &master);
	while (n_recv != (int)n_byte) {
		readfds = master;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		int ret = select(socket_recv + 1, &readfds, NULL, NULL, &tv);

		switch (ret) {
		case SOCKET_ERROR:
			DBG_ERR("SOCKET_ERROR\r\n");
			FD_CLR(socket_recv, &master);
			return -1;
		case 0: //timeout
			DBG_ERR("TimeOut\r\n");
			FD_CLR(socket_recv, &master);
			return -1;
		default:
			if (FD_ISSET(socket_recv, &readfds)) {
				n = recv(socket_recv, (char *)p_data + n_recv, n_byte - n_recv, 0);
				if (n == 0) {
					DBG_WRN("Session Closed\r\n");
					FD_CLR(socket_recv, &master);
					return -1;
				} else if (n < 0) {
					if (n_retry++ > 10) {
						DBG_ERR("failed to retry\r\n");
						FD_CLR(socket_recv, &master);
						return -1;
					}
				} else {
					n_recv += n;
				}
			}
			break;
		}
	}
	FD_CLR(socket_recv, &master);
	return (n_recv == (int)n_byte) ? 0 : -1;
}

static int msdcnvt_client_send(int socket_send, void *p_data, unsigned int n_byte)
{
	int n, n_send = 0;
	int n_retry = 0;
	struct timeval tv;
	fd_set master;
	fd_set writefds;

	FD_ZERO(&master);
	FD_ZERO(&writefds);
	FD_SET(socket_send, &master);
	while (n_send != (int)n_byte) {
		writefds = master;
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		int ret = select(socket_send + 1, NULL, &writefds, NULL, &tv);

		switch (ret) {
		case SOCKET_ERROR:
			DBG_ERR("SOCKET_ERROR\r\n");
			FD_CLR(socket_send, &master);
			return -1;
		case 0: //timeout
			DBG_ERR("TimeOut\r\n");
			FD_CLR(socket_send, &master);
			return -1;
		default:
			if (FD_ISSET(socket_send, &writefds)) {
				n = send(socket_send, (char *)p_data + n_send, n_byte - n_send, 0);
				if (n == 0) {
					DBG_WRN("Session Closed\r\n");
					FD_CLR(socket_send, &master);
					return -1;
				} else if (n < 0) {
					if (n_retry++ > 10) {
						DBG_ERR("failed to retry\r\n");
						FD_CLR(socket_send, &master);
						return -1;
					}
				} else {
					n_retry = 0;
					n_send += n;
				}
			}
			break;
		}
	}
	FD_CLR(socket_send, &master);
	return (n_send == (int)n_byte) ? 0 : -1;
}

static int msdcnvt_client_event(MSDCNVT_IPC_CFG *p_ipc_cfg, MSDCNVT_CLIENT *p_client, fd_set *p_master)
{
	int err = 0;
	struct timeval tv = { 0 };
	MSDCNVT_CBW CBW = { 0 };
	MSDCNVT_ICALL_TBL *p_call = &p_ipc_cfg->call_tbl;

	gettimeofday(&tv, NULL);
	p_client->last_tv_sec = tv.tv_sec;

	if (msdcnvt_client_recv(p_client->client_socket, &CBW, sizeof(MSDCNVT_CBW)) != 0) {
		err = -1;
	} else if (CBW.signature != CBW_SIGNATURE) {
		DBG_ERR("client CBW_SIGNATURE.\r\n");
		err = -1;
	}

	if (err < 0) {
		FD_CLR(p_client->client_socket, p_master);
		closesocket(p_client->client_socket);
		p_client->client_socket = INVALID_SOCKET;
		return err;
	}

	unsigned int addr;
	MSDCNVT_ICMD icmd_verify = {
		(unsigned int) p_call->verify,
		(unsigned int) &CBW,
		sizeof(MSDCNVT_CBW),
		(unsigned int) &addr,
		sizeof(unsigned int),
		0,
	};

	err = msdcnvt_uitron_cmd(&icmd_verify);
	if (err != 0 || icmd_verify.err != 0) {
		DBG_ERR("msdcnvt_uitron_cmd.icmd_verify %d,%d\r\n", err, icmd_verify.err);
		err = -1;
	}

	addr = (unsigned int)mem_phy_to_noncache(addr);

	if (err < 0) {
		FD_CLR(p_client->client_socket, p_master);
		closesocket(p_client->client_socket);
		p_client->client_socket = INVALID_SOCKET;
		return err;
	}

	MSDCNVT_VENDOR_OUTPUT output;
	MSDCNVT_ICMD icmd_vendor = {
		(unsigned int) p_call->vendor,
		(unsigned int) &CBW,
		sizeof(CBW),
		(unsigned int) &output,
		sizeof(output),
		0,
	};

	if (CBW.flags == CBW_FLAG_IN) {
		//send data to PC
		err = msdcnvt_uitron_cmd(&icmd_vendor);
		if (err != 0 || icmd_vendor.err != 0) {
			DBG_ERR("msdcnvt_uitron_cmd.icmd_vendor %d,%d\r\n", err, icmd_vendor.err);
		}
		//always send data because PC wait to receive data
		err = msdcnvt_client_send(p_client->client_socket, (void *)addr, CBW.data_trans_len);
	} else { //recv data from PC
		err = msdcnvt_client_recv(p_client->client_socket, (void *)addr, CBW.data_trans_len);
		if (err == 0) {
			if (msdcnvt_uitron_cmd(&icmd_vendor) != 0
				|| icmd_vendor.err != 0) {
				DBG_ERR("msdcnvt_uitron_cmd.icmd_vendor %d,%d\r\n", err, icmd_vendor.err);
			}
		}
	}

	if (err < 0) {
		FD_CLR(p_client->client_socket, p_master);
		closesocket(p_client->client_socket);
		p_client->client_socket = INVALID_SOCKET;
		return err;
	}
	return 0;
}

static int msdcnvt_start_server(MSDCNVT_IPC_CFG *p_ipc_cfg)
{
	int ret;
	int fdmax, i;
	fd_set master;  // master file descriptor list
	fd_set readfds; // temp file descriptor list for select()
	struct timeval tv;
	MSDCNVT_ICALL_TBL *p_call = &p_ipc_cfg->call_tbl;

	// clear the set
	FD_ZERO(&master);
	FD_ZERO(&readfds);
	// add server socket to the set
	FD_SET(m_server_socket, &master);

	fdmax = m_server_socket;

	msdcnvt_uitron_init(p_ipc_cfg);

	MSDCNVT_ICMD icmd_open = {
		(unsigned int)p_call->open,
		0,
		0,
		0,
		0,
		0,
	};

	ret = msdcnvt_uitron_cmd(&icmd_open);
	if (ret != 0) {
		DBG_ERR("failed to run uitron_cmd.open\r\n");
		msdcnvt_uitron_exit();
		return ret;
	}

	while (!p_ipc_cfg->stop_server) {
		readfds = master;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		ret = select(fdmax + 1, &readfds, NULL, NULL, &tv);
		switch (ret) {
		case SOCKET_ERROR:
			DBG_ERR("SOCKET_ERROR\r\n");
			p_ipc_cfg->stop_server = 1;
			break;;
		case 0: //timeout, continue to wait
			break;
		default:
			if (FD_ISSET(m_server_socket, &readfds)) {
				msdcnvt_server_event(p_ipc_cfg, &master, &fdmax);
				break;
			}
			for (i = 0; i < CFG_MAX_CLIENT; i++) {
				MSDCNVT_CLIENT *p = &m_client_list[i];
				//in linux, if socket is -1, it will cause exception in FD_ISSET
				if (p->client_socket != INVALID_SOCKET
					&& FD_ISSET(p->client_socket, &readfds)) {
					DBG_IND("get sock:%d\r\n", p->client_socket);
					msdcnvt_client_event(p_ipc_cfg, p, &master);

				}
			}
			break;
		}
	}

	MSDCNVT_ICMD icmd_close = {
		(unsigned int)p_call->close,
		0,
		0,
		0,
		0,
		0,
	};

	ret = msdcnvt_uitron_cmd(&icmd_close);
	if (ret != 0) {
		DBG_ERR("failed to run uitron_cmd.close\r\n");
		//no need to return
	}

	msdcnvt_uitron_exit();

	// close all the opened sockets
	closesocket(m_server_socket);
	m_server_socket = INVALID_SOCKET;
	for (i = 0; i < CFG_MAX_CLIENT; i++) {
		if (m_client_list[i].client_socket != INVALID_SOCKET) {
			DBG_IND("close socket %d\n", m_client_list[i].client_socket);
			closesocket(m_client_list[i].client_socket);
			m_client_list[i].client_socket = INVALID_SOCKET;
		}
	}

	return 0;
}

#if defined (__LINUX)
int main(int argc, char **argv)
#else
int msdcnvt_main(int argc, char **argv)
#endif
{
	int err;
	unsigned int size;

#if (CFG_VIA_IPC)
	unsigned int addr;

	sscanf(argv[1], "0x%08X", &addr);
	sscanf(argv[2], "0x%08X", &size);

#if defined(__LINUX680)
	NvtIPC_mmap(NVTIPC_MEM_TYPE_NONCACHE, NVTIPC_MEM_ADDR_DRAM1_ALL_FLAG, 0);
#endif

	MSDCNVT_IPC_CFG *p_ipc_cfg = (MSDCNVT_IPC_CFG *)mem_phy_to_noncache(addr);
#elif (CFG_VIA_IOCTL)
	MSDCNVT_IPC_CFG *p_ipc_cfg = msdcnvt_uitron_init2();

	size = sizeof(MSDCNVT_IPC_CFG);
#endif

	if (p_ipc_cfg == NULL) {
		return -1;
	}

	err = msdcnvt_init_ipc(p_ipc_cfg, size);
	if (err < 0) {
		return err;
	}

	err = msdcnvt_init_socket(p_ipc_cfg);
	if (err < 0) {
		return err;
	}

	err = msdcnvt_start_server(p_ipc_cfg);
	if (err < 0) {
		return err;
	}

	return 0;
}

#if defined (__ECOS)
static THREAD_HANDLE m_h_thread;
static THREAD_OBJ m_thread_obj;
static char m_param[3][16] = {0};
static unsigned int m_stack[8192 / sizeof(unsigned int)];
THREAD_DECLARE(msdcnvt_main_thread, lp_param)
{
	char *argv[3] = {m_param[0], m_param[1], m_param[2]};

	msdcnvt_main(3, argv);
	THREAD_EXIT();
}
void msdcnvt_ecos_main(const char *cmd_line)
{
	int argc = sscanf(cmd_line, "%s %s %s", m_param[0], m_param[1], m_param[2]);

	if (argc != 3) {
		DBG_ERR("got incorrect parameters.\r\n");
		return;
	}

	unsigned int stack_size = sizeof(m_stack);

	THREAD_CREATE("msdcnvt_main_thread", m_h_thread, msdcnvt_main_thread, NULL, m_stack, stack_size, &m_thread_obj);
	THREAD_RESUME(m_h_thread);
}
#endif
