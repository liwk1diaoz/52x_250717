#ifndef __TCP_H_
#define __TCP_H_

struct listener {
	int fd;
};

extern struct listener *listener;
void conn_cleanall(void);
#endif


