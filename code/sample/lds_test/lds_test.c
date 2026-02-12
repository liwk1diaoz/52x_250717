#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hdal.h>

#define FALSE 0
#define TRUE 1

#define MAX_ARGS 32

#ifdef __cplusplus
# define __NVT_CMDSYS_EXTERNC extern "C"
#else
# define __NVT_CMDSYS_EXTERNC extern
#endif
// Also define externC for now - but it is deprecated
#define NVT_CMDSYS_EXTERNC __NVT_CMDSYS_EXTERNC

typedef int (*NVT_CMDSYS_MAIN)(int argc, char **argv); ///< command entry
typedef int (*NVT_CMDSYS_CB)(char *str); ///< register new domain command sys

typedef struct _NVT_CMDSYS_ENTRY {
    NVT_CMDSYS_MAIN p_main;
    char name[32];
} NVT_CMDSYS_ENTRY;

#if 1 //defined(__FREERTOS)
#define MAINFUNC_ENTRY(_l,_argc,_argv) \
NVT_CMDSYS_EXTERNC int _l##_cmdsys_main(int _argc, char **_argv); \
NVT_CMDSYS_ENTRY _l##cmdsys_entry __attribute__ ((section (".cmdsys.table." #_l))) = {    \
   _l##_cmdsys_main,                                      \
   #_l							  \
}; \
NVT_CMDSYS_EXTERNC int _l##_cmdsys_main(int _argc, char **_argv)
#else
#define MAINFUNC_ENTRY(_l,_argc,_argv) \
int main(int _argc, char **_argv)
#endif


MAINFUNC_ENTRY(lds, argc, argv)
{
    printf("i am test\n");
    return 0;
}

extern const char __MAINFUNCTAB_BEGIN__[];
extern const char __MAINFUNCTAB_END__[];

static int list_all_cmd(void)
{
    NVT_CMDSYS_ENTRY *t;
    int cnt = 0;

    //printf("list command set: 0x%x-0x%x\n",(unsigned int)__MAINFUNCTAB_BEGIN__,(unsigned int)__MAINFUNCTAB_END__);

    for (t = (NVT_CMDSYS_ENTRY *)__MAINFUNCTAB_BEGIN__; t != (NVT_CMDSYS_ENTRY *)__MAINFUNCTAB_END__; t++)
    {
        //printf("%s: 0x%x\n", t->name, (unsigned int)t->p_main);
        printf("%s\n", t->name);
        cnt++;
    }

    return cnt;
}

static int parsecmd(char *cmdline, char **cmdname, char **cmdarg)
{
    char *pstr, *cmd, *pstrend;
    char *arg;

    pstr = cmdline;
    pstrend = cmdline + strlen(cmdline);
    while (*pstr==' ' || *pstr=='\t' || *pstr=='\r' || *pstr=='\n') pstr++;
    arg = strchr(pstr, ' ');
    if (arg)
    {
        while (*arg==' ' || *arg=='\t' || *arg=='\r' || *arg=='\n') arg++;
        *cmdarg = arg;
    }
    else
        *cmdarg = pstrend;

    cmd = strtok (pstr," \t\n");

    if (!cmd)
        return FALSE;
    *cmdname = cmd;

    //printf("parsecmd: cmd=\"%s\", arg=\"%s\"\n", *cmdname, *cmdarg);

    return TRUE;
}

static int run_func(char *name, NVT_CMDSYS_MAIN prun_func, char *args)
{
    int ret;
    char *cmdLine;
    int _argc = 0;
    char *_argv[MAX_ARGS];
    char *param;

    cmdLine = strdup(args);

	if (cmdLine == NULL) {
		printf("cmdsys failed to strdup.\n");
		return -1;
	}

    param = strtok(cmdLine, " \t\n");
    _argv[_argc++] = name;
    while (param && _argc < MAX_ARGS) {
        _argv[_argc++] = param;
        param = strtok(NULL, " \t\n");
    }

	if (_argc < MAX_ARGS) {
	    _argv[_argc] = NULL;
	}

    ret = prun_func(_argc, _argv);

    free(cmdLine);

    return ret;
}


static int find_and_runcmd(char *cmdname, char *cmdarg)
{
    NVT_CMDSYS_ENTRY *t;

    for (t = (NVT_CMDSYS_ENTRY *)__MAINFUNCTAB_BEGIN__; t != (NVT_CMDSYS_ENTRY *)__MAINFUNCTAB_END__; t++)
    {
        if (strcmp(cmdname, t->name) == 0)
        {
            run_func(cmdname, t->p_main, cmdarg);
            return TRUE;
        }
    }
    return FALSE;
}


int nvt_cmdsys_runcmd(char *str)
{
    int err = -1;
    char *cmdstr, *cmdname, *cmdarg;

    if (str[0] == '?' || strncmp(str, "ls", 3) == 0) {
        list_all_cmd();
        return 0;
    }

    cmdstr = strdup(str);
	if (cmdstr == NULL) {
		printf("cmdsys failed to strdup.\n");
		return -1;
	}

    if (parsecmd(cmdstr,&cmdname, &cmdarg))
    {
        if (find_and_runcmd(cmdname, cmdarg))
        {
              err = 1;
        }
        else
        {
            printf("parsecmd: cmd=\"%s\", arg=\"%s\"\n", cmdname, cmdarg);
            printf("command not found!\n");
              err = 0;
        }
    }
    free(cmdstr);
    return err;
}


int main(void)
{
    hd_common_init(0);

    printf("Hello World!\r\n");
    list_all_cmd();
    nvt_cmdsys_runcmd("lds");

    return 0;
}
