#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/cmdsys.h>


const char VERSION_INFO_BEGIN[] __attribute__((section (".nvtversioninfo.begin")))  = "version_info_begin";
const char VERSION_INFO_END[] __attribute__((section (".nvtversioninfo.end")))      = "version_info_end";

extern char _section_verinfostart[] __attribute__((weak));
extern char _section_verinfoend[] __attribute__((weak));

#define MDU_NAME_LEN    40
#define MDU_MAX_COUNT   500
static char  ver_name[MDU_NAME_LEN];
static char  mdu_name[MDU_NAME_LEN];
static char  sha1_name[MDU_NAME_LEN];
typedef struct _UNIT{
    char* p_name;
    char* p_ver;
    void* p_sha1;
}UNIT;

static UNIT p_unit[MDU_MAX_COUNT];

static int comp( const void * p, const void * q)
{
    UNIT* p1 = (UNIT*)p;
    UNIT* q1 = (UNIT*)q;
    return strcmp(p1->p_name,q1->p_name);
}

static char * find_next_string(char** pp)
{
    char* p = NULL;
    while(*(*pp)++==0);
    p = (*pp) - 1;
    while(*(*pp)++!=0);
    return p;
}

/*
    Dump version information of Nvt's library(.a)

    Display all library version information. This function will list library's
\n  module_name     version_code    build_date & build_time

    Example:
    @code
    {
           Startup.a   1.00.000  Sep 11 2013, 14:05:12
        AppControl.a   1.00.002  Sep 11 2013, 14:11:06
           Capture.a   1.00.005  Sep 11 2013, 14:11:20
            DevMan.a   1.00.002  Sep 11 2013, 14:11:22
          LviewNvt.a   1.00.006  Sep 11 2013, 14:11:24
             Debug.a   1.00.012  Sep 11 2013, 14:04:55
            Kernel.a   1.01.006  Sep 11 2013, 14:04:59
            ...
            ...
    }
    @endcode
*/
void dump_verinfo(void)
{
    char    * p;
    char    * p_name;
    char    * p_ver;
    char    * p_sha1;
    unsigned int  curLen;
    unsigned int  i,j;
    unsigned int  n = 0;

    unsigned char *version_begin_addr = (unsigned char *)(_section_verinfostart);
    unsigned char *version_end_addr = (unsigned char *)(_section_verinfoend);

    printf("********************************************************************************\r\n");
    while(version_end_addr > version_begin_addr)
    {
        p = find_next_string((char**)&version_begin_addr);

        if(p != NULL)
        {
            p_name = p;
            p_ver = strstr(p,"#");
            if(p_ver==NULL) {
                continue;
            }
            p_ver++;
            p_sha1 = strstr(p_ver,"_");
			if(p_sha1==NULL) {
                continue;
            }
            p_sha1++;
            p_unit[n].p_name = p_name;
            p_unit[n].p_ver = p_ver;
            p_unit[n].p_sha1 = p_sha1;
            n++;
            if(n >= MDU_MAX_COUNT)
            {
                printf("Module count exceed %03d\r\n", MDU_MAX_COUNT);
                return;
            }
        }
    }

    qsort ( p_unit, n, sizeof (UNIT) , comp) ;

    for(i=0;i<n;i++)
    {
        memset((void *)mdu_name, 0x00, sizeof(mdu_name));
        memset((void *)ver_name, 0x00, sizeof(ver_name));
        memset((void *)sha1_name, 0x00, sizeof(sha1_name));

        sscanf((char *)p_unit[i].p_name, "%[^#]", mdu_name);
        sscanf((char *)p_unit[i].p_ver, "%[^_]", ver_name);
        strncpy(sha1_name,p_unit[i].p_sha1,sizeof(sha1_name)-1);

        curLen = strlen(mdu_name);
        mdu_name[curLen] = '.';
        mdu_name[curLen + 1] = 'a';
        //mdu_name[curLen + 2] = 0x0;
        if((i % 2) == 0)
        {
            for(j = (curLen + 2); j <= 35; j++)
            {
                mdu_name[j] = '-';
            }
        }
        printf("%-36s %-13s %-20s\r\n", mdu_name, ver_name, sha1_name);
    }

    printf("********************************************************************************\r\n");
}

//@}

MAINFUNC_ENTRY(ver, argc, argv)
{
    dump_verinfo();
    return 0;
}
