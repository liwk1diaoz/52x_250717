#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include "hdal.h"
#include <kwrap/examsys.h>
#include <kwrap/cmdsys.h>

static int check_if_multiple_instance(char *p_name)
{
	char buf[128];
	FILE *pp;

	snprintf(buf, sizeof(buf), "ps | grep %s | wc -l", p_name);
	if ((pp = popen(buf, "r")) == NULL) {
		printf("popen() error!\n");
		exit(1);
	}

	while (fgets(buf, sizeof buf, pp)) {
	}

	pclose(pp);

	if(atoi(buf) > 3) {
		return 1;
	}
	return 0;
}

EXAMFUNC_ENTRY(hd_cmdsys, argc, argv)
{
	if (check_if_multiple_instance(argv[0]) == 1) {
		if (argc > 1) {
			nvt_cmdsys_ipc_cmd(argc, argv);
			return 0;
		}
		printf("%s has already in running. quit.\n", argv[0]);
		return -1;
	}

	nvt_cmdsys_init();      // command system
	nvt_examsys_init();     // exam system

	//start do your program
	sleep(10000);
	return 0;
}
