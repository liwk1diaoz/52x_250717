#include <stdio.h>
#include <sys/time.h>

// times() in normal Linux is 10ms precision.
// I recommend gettimeofday().
static struct timeval __tv;

void start_watch(void)
{
	printf("%s\n", __func__);
	gettimeofday(&__tv, NULL);
}

unsigned long stop_watch(const char *msg)
{
	long usec;
	struct timeval tv2;
	
	gettimeofday(&tv2, NULL);
	printf("%s\n", __func__);

	usec = tv2.tv_usec - __tv.tv_usec;
	if(usec < 0)
	{
		usec += 1000000;
		tv2.tv_sec--;
	}

	printf("%s: %ldsec %03ldmsec %03ldusec\n", msg, (long)(tv2.tv_sec - __tv.tv_sec), usec/1000, usec%1000);

	return ((tv2.tv_sec - __tv.tv_sec)*1000 + usec/1000);
}
