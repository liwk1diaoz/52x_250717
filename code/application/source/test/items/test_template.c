#include <stdio.h>
#include <kwrap/examsys.h>

EXAMFUNC_ENTRY(test_template, argc, argv)
{
    printf("i am test template.\n");
	printf("input q with enter to quit.\n");
	while (NVT_EXAMSYS_GETCHAR() != 'q') {
		printf("input q with enter to quit.\n");
	}
    return 0;
}
