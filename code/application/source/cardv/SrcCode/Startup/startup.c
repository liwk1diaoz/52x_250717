#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/mailbox.h>
#include <kwrap/task.h>
#include <kwrap/debug.h>
#include <kwrap/cmdsys.h>
#include "startup.h"

static ID sem_shutdown;

static void app_startup(void *p_param)
{
	vos_sem_create(&sem_shutdown, 1, "sem_shutdown");
	vos_sem_wait_interruptible(sem_shutdown); //clear shutdown semaphore

	// do the first program
	app_main();

	// wait system trigger shutdown
	vos_sem_wait_interruptible(sem_shutdown);
	vos_sem_destroy(sem_shutdown);
}

static int check_if_multiple_instance(char *p_name)
{
	char buf[128];
	FILE *pp;
	UINT32 inst_cnt = 0;

	snprintf(buf, sizeof(buf), "ps", p_name);
	if ((pp = popen(buf, "r")) == NULL) {
		printf("popen() error!\n");
		exit(1);
	}

	while (fgets(buf, sizeof buf, pp)) {
		if (strstr(buf, p_name) != NULL) {
			inst_cnt ++;
		}
	}
	pclose(pp);
	if (inst_cnt > 1) {
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (check_if_multiple_instance(argv[0]) == 1) {
		if (argc > 1) {
			nvt_cmdsys_ipc_cmd(argc, argv);
			return 0;
		}
		printf("%s has already in running. quit.\n");
		return -1;
	}
	vos_mbx_init(16);
	app_startup(NULL);
	// here not be reached.
	return 0;
}
