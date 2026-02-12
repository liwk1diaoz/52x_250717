#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/type.h>
#include <kwrap/cmdsys.h>
#include <kwrap/task.h>

#define MAX_ARGS 32

typedef struct _NVT_CMDSYS_DOMAIN {
    char ch; ///< domain character
    NVT_CMDSYS_CB cb; ///< domain running body
} NVT_CMDSYS_DOMAIN;

static NVT_CMDSYS_DOMAIN m_domain[8] = {
	{'>', nvt_cmdsys_runcmd},
	{'\0', NULL},
	{'\0', NULL},
	{'\0', NULL},
	{'\0', NULL},
	{'\0', NULL},
	{'\0', NULL},
	{'\0', NULL},
};

static NVT_CMDSYS_DOMAIN *mp_curr = &m_domain[0];

extern const char __MAINFUNCTAB_BEGIN__[];
extern const char __MAINFUNCTAB_END__[];

static THREAD_HANDLE m_cmdsys_thread;

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

static NVT_CMDSYS_DOMAIN *nvt_cmdys_switch(char ch)
{
	size_t i;
	for (i = 0; i < sizeof(m_domain)/sizeof(m_domain[0]); i++) {
		if (m_domain[i].ch == 0) {
			break;
		} else if (m_domain[i].ch == ch) {
			return &m_domain[i];
		}
	}
	return NULL;
}

static char buf[512] = {0};
static void nvt_cmdsys_tsk(void * p_param)
{
	//coverity[no_escape]
	while (1) {
        size_t i;
        printf("%c ", mp_curr->ch);
	    fgets(buf, sizeof(buf), stdin);
        char *p_proc = &buf[0];
        // removing \r\n and processing backspace
        for(i = 0; i < strlen(buf) ; i++) {
            if (buf[i] == '\r' || buf[i] == '\n') {
                *p_proc = '\0';
                break;
            }
            if (buf[i] == '\b') {
                if(p_proc > &buf[0]) {
                    p_proc--;
                }
            } else {
                *p_proc++ = buf[i];
            }
        }

		//check domain change
		if (strlen(buf) == 1) {
			NVT_CMDSYS_DOMAIN *p_new = nvt_cmdys_switch(buf[0]);
			if (p_new) {
				mp_curr = p_new;
				printf("switch to '%c'\n", mp_curr->ch);
				continue;
			}
		}

        if (strlen(buf) > 0) {
	        printf("run: %s\n", buf);
            mp_curr->cb(buf);
        }
	}
}

int nvt_cmdsys_init(void)
{
    m_cmdsys_thread = vos_task_create(nvt_cmdsys_tsk, NULL, "nvt_cmdsys_tsk", 2, 16*1024);
    vos_task_resume(m_cmdsys_thread);
    return 0;
}

int nvt_cmdsys_regsys(char domain, NVT_CMDSYS_CB cb)
{
	size_t i;

	for (i = 0; i < sizeof(m_domain)/sizeof(m_domain[0]); i++) {
		if (m_domain[i].ch == 0) {
			m_domain[i].ch = domain;
			m_domain[i].cb = cb;
			return 0;
		} else if (m_domain[i].ch == domain) {
			m_domain[i].cb = cb;
			return 0;
		}
	}
	printf("nvt_cmdsys_regsys is full.\n");
	return -1;
}
