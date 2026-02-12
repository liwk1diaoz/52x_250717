#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/type.h>
#include <kwrap/cmdsys.h>
#include <kwrap/task.h>
#include <kwrap/debug.h>

#if !defined(__FREERTOS) && !defined(MODULE)
#define LINUX_USERSPACE
#endif

#if defined(LINUX_USERSPACE)
#include <sys/ipc.h>
#include <sys/msg.h>
static key_t m_key= MAKEFOURCC('U','V','O','S'); //user space VOS
#endif

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

extern const char __MAINFUNCTAB_BEGIN__[] __attribute__((weak));
extern const char __MAINFUNCTAB_END__[] __attribute__((weak));

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

static char gbuf[512] = {0};
static THREAD_DECLARE(nvt_cmdsys_tsk, p_param)
{
#if defined(LINUX_USERSPACE)
    int msqid;
    char *buf = &gbuf[sizeof(long)];

    if ((msqid = msgget(m_key, IPC_CREAT | 0666)) < 0) {
		fprintf(stderr, "nvt_cmdys: msgget failed\n");
        THREAD_RETURN(-1);
	}
#else
    char *buf = &gbuf[0];
#endif

	//coverity[no_escape]
	while (1) {
        size_t i;
#if defined(LINUX_USERSPACE)
        //mtype is long, but we keep 8 bytes alive for coverity
        if (msgrcv(msqid, gbuf, sizeof(gbuf)-8, 1, 0) < 0) {
			printf("nvt_cmdys: msgrcv failed.\r\n");
			THREAD_RETURN(-1);
		}
#else
        printf("%c ", mp_curr->ch);
	    fgets(gbuf, sizeof(gbuf), stdin);
#endif
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

    THREAD_RETURN(-1);
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


#if !defined(__FREERTOS) && !defined(MODULE)

static int nvt_cmdsys_calc_flat_cmd(char *p_flat, int flat_len, int argc, char **argv)
{
    int i;
    int sum = 0;

    for(i = 1; i < argc; i++) {
        sum += strlen(argv[i]) + 1; // +1 is for space or eof
    }

    if (p_flat == NULL) {
        return sum; // just calc size
    }

    memset(p_flat, 0, flat_len);
    for(i = 1; i < argc; i++) {
        strncat(p_flat, argv[i], flat_len);
        flat_len -= strlen(argv[i]);
        if (flat_len <= 0) {
            fprintf(stderr, "nvt_cmdsys: flat_len is too small.\n");
            return -1;
        }
        if (i < argc-1) {
            strncat(p_flat, " ", flat_len);
            flat_len -= 1;
        }
    }

    return sum;
}

int nvt_cmdsys_ipc_cmd(int argc, char **argv)
{
    int msqid = msgget(m_key, 0666);
	if (msqid < 0) {
		//fprintf(stderr, "program may not start.\n");
		return 0;
	}

    int flat_cmd_len = nvt_cmdsys_calc_flat_cmd(NULL, 0, argc, argv);
    if (flat_cmd_len <= 0) {
        return 0;
    }

    //mtype is long, but we keep 8 bytes alive for coverity
    int alloc_size = flat_cmd_len+8;
    char *p_cmd = (char *)malloc(alloc_size);

    if (p_cmd == NULL) {
        fprintf(stderr, "nvt_cmdsys: no mem.\n");
        return -1;
    }

    *(long *)p_cmd = 1; //mtype = 1
    if (nvt_cmdsys_calc_flat_cmd(p_cmd+sizeof(long), flat_cmd_len, argc, argv) <= 0) {
        free(p_cmd);
        return 0;
    }

    //the size is string len exclude mtype
    int er = msgsnd(msqid, p_cmd, flat_cmd_len, 0);
	if (er < 0) {
		fprintf(stderr, "nvt_cmdsys: failed to send command, er=%d.\n", er);
        free(p_cmd);
		return -1;
	}

    free(p_cmd);

    return 0;
}

#endif
