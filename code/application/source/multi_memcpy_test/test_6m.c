#include "bas_task_api.h"

void *test_tskdownloadIpcStream(int argchan)
{
	unsigned long size = 0;
	void *buffer1  = NULL;
	void *buffer2  = NULL;
	unsigned int count = 1;
	struct timeval tv1,tv2;
	unsigned long time = 0;
	unsigned long lastrun = 0;
	unsigned long run = 0;
	FILE *fileRead = NULL;
	FILE *fileWrite = NULL;
	char fname[40];

	sprintf(fname,"/mnt/usb/copy_%d.mp4",argchan);
	
	fileRead = fopen("/mnt/usb/oneframe_6M.mp4","rb");
	fileWrite = fopen(fname,"wb");

	if(fileRead != NULL)
	{
		fseek(fileRead,0,SEEK_END);
		size = ftell(fileRead);
		fseek(fileRead,0,SEEK_SET);

		if(size != 0)
		{
			buffer1 = malloc(size);
			buffer2 = malloc(size);
			if(buffer1 == NULL || buffer2 == NULL)
			{
				printf("malloc size : %lu failed\n",size);
				fclose(fileRead);
				return NULL;
			}
			else
			{
				fread(buffer1,1,size,fileRead);
				fclose(fileRead);
				memset(&tv1,0,sizeof(tv1));
				memset(&tv2,0,sizeof(tv2));

				while(1)
				{	
					memcpy(buffer2,buffer1,size);
					if(count == 1)
					{
						if(fileWrite != NULL)
						{
							fwrite(buffer2,1,size,fileWrite);
							fclose(fileWrite);
						}
						else
						{
							printf("fopen fileWrite failed\n");
							return NULL;
						}
						count = 0;
						gettimeofday(&tv1,NULL);
					}
					else
					{
						gettimeofday(&tv2,NULL);
						
						time = (tv2.tv_sec * 1000 * 1000 - tv1.tv_sec * 1000 * 1000) + (tv2.tv_usec - tv1.tv_usec);
						if(time > 1000 * 1000)
						{
							memcpy(&tv1,&tv2,sizeof(struct timeval));
							printf(">>>>a7 chan:%d %ldus run count:%u,cur:%lu last:%lu\n",argchan,time,run - lastrun,run,lastrun);
							lastrun = run;
						}
					}
					
					run++;
					
					usleep(10*1000);
				}
			}
		}
	}
	else
	{
		printf("fopen fileRead failed\n");
	}
}

int main(int argc, char **argv)
{
	int channel = 1;
	int  i = 0;
	pthread_t pidDownloadIPCStream;

	if (argc == 2) {
		channel = atoi(argv[1]);
		printf("channel %d\r\n", channel);
	}

	for(i = 0; i < channel; i++)
	{
		bas_task_creat(&pidDownloadIPCStream, 65, 40960, (void *)test_tskdownloadIpcStream, i, "TSK_DOWNLOADIPCSTREAM");
	}

	while(1)
	{
		sleep(10);
	}

	return 0;
}
