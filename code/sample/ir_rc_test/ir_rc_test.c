#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/input.h>

int main(void)
{
/*******************************************************
*    input subsystem (asscioated with rc framework)    *
********************************************************/
	char protocol[] = "nec";
	int ret;
	int fd;
	int maxfd;
	struct input_event ie;
	fd_set readfds;
	struct timeval tv;

	/* Configure as NEC protocol and enable */
	fd = open("/sys/class/rc/rc0/protocols", O_WRONLY);
	if (fd < 0 ) {
		printf("failed to open /sys/class/rc/rc0/protocols\n");
		return -1;
	}

	ret = write(fd, protocol, sizeof(protocol));
	if (ret < 0) {
		printf("failed to write protocols\n");
	}

	close(fd);

	/* Waiting for input events sent by IR */
	fd = open ("/dev/input/event0", O_RDONLY);
	if (fd <= 0) {
		printf ("failed to open /dev/input/event0\n");
		return -1;
	}

	maxfd = fd + 1;

	while (1) {
		FD_ZERO(&readfds);
		FD_SET(fd,&readfds);

		tv.tv_sec = 0;
		tv.tv_usec = 10000;

		switch (select(maxfd, &readfds, NULL, NULL, &tv)) {
		case -1:
			return -1;
			break;

		case 0:  /* timeout */
			break;

		default:
			if (FD_ISSET(fd, &readfds)) {
				ret = read(fd, &ie, sizeof (ie));
				if (ret < 0) {
					printf("failed to read event0\n");
				}

				printf("type(%d) code(%d) value(%d)\n", ie.type, ie.code, ie.value);

				if (ie.type == EV_KEY) {
					if (ie.code == KEY_POWER) {
						printf("code is KEY_POWER\n");
					}
				}
			}
			break;

		}

	}

	close (fd);

	return 0;
}