#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/type.h>
#include <kwrap/cmdsys.h>
#include <kwrap/examsys.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/debug.h>

#define MAX_ARGS 32

#define FLGEXAMSYS_IDLE  FLGPTN_BIT(0)
#define FLGEXAMSYS_RUN   FLGPTN_BIT(1)
#define FLGEXAMSYS_POLLING FLGPTN_BIT(2)
#define FLGEXAMSYS_ITEM_WAIT FLGPTN_BIT(3)         //indicate script item is waiting key or command string
#define FLGEXAMSYS_ITEM_WAIT_DONE FLGPTN_BIT(4)    //indicate script item recevied key
#define FLGEXAMSYS_ITEM_WAIT_POLLING FLGPTN_BIT(5) //for check if idle then run

extern const char __EXAMFUNCTAB_BEGIN__[] __attribute__((weak));
extern const char __EXAMFUNCTAB_END__[] __attribute__((weak));

static THREAD_HANDLE m_examsys_thread;
static ID examsys_flag_id = 0;
static char buf[256] = {0};
static char item_input[128] = {0};

static int list_all_cmd(void)
{
	NVT_EXAMSYS_ENTRY *t;
	int cnt = 0;

	//printf("list command set: 0x%x-0x%x\n",(unsigned int)__EXAMFUNCTAB_BEGIN__,(unsigned int)__EXAMFUNCTAB_END__);

	for (t = (NVT_EXAMSYS_ENTRY *)__EXAMFUNCTAB_BEGIN__; t != (NVT_EXAMSYS_ENTRY *)__EXAMFUNCTAB_END__; t++) {
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
	while (*pstr == ' ' || *pstr == '\t' || *pstr == '\r' || *pstr == '\n') {
		pstr++;
	}
	arg = strchr(pstr, ' ');
	if (arg) {
		while (*arg == ' ' || *arg == '\t' || *arg == '\r' || *arg == '\n') {
			arg++;
		}
		*cmdarg = arg;
	} else {
		*cmdarg = pstrend;
	}

	cmd = strtok(pstr, " \t\n");

	if (!cmd) {
		return FALSE;
	}
	*cmdname = cmd;

	//printf("parsecmd: cmd=\"%s\", arg=\"%s\"\n", *cmdname, *cmdarg);

	return TRUE;
}

static int run_func(char *name, NVT_EXAMSYS_MAIN prun_func, char *args)
{
	int ret;
	char *cmdLine;
	int _argc = 0;
	char *_argv[MAX_ARGS];
	char *param;

	cmdLine = strdup(args);

	if (cmdLine == NULL) {
		printf("examsys failed to strdup.\n");
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
	NVT_EXAMSYS_ENTRY *t;

	for (t = (NVT_EXAMSYS_ENTRY *)__EXAMFUNCTAB_BEGIN__; t != (NVT_EXAMSYS_ENTRY *)__EXAMFUNCTAB_END__; t++) {
		if (strcmp(cmdname, t->name) == 0) {
			run_func(cmdname, t->p_main, cmdarg);
			return TRUE;
		}
	}
	return FALSE;
}

int nvt_examsys_runcmd(char *str)
{
	int err = -1;
	char *cmdstr, *cmdname, *cmdarg;

	if (str[0] == '?' || strncmp(str, "ls", 3) == 0) {
		list_all_cmd();
		return 0;
	}

	//fix color end on linux should be in front of \n
	printf(DBG_COLOR_CYAN "\rexam: '%s' -begin%s\n", str, DBG_COLOR_END);
	cmdstr = strdup(str);

	if (cmdstr == NULL) {
		printf("examsys failed to strdup.\n");
		return -1;
	}

	if (parsecmd(cmdstr, &cmdname, &cmdarg)) {
		if (find_and_runcmd(cmdname, cmdarg)) {
			err = 1;
		} else {
			printf("parsecmd: cmd=\"%s\", arg=\"%s\"\n", cmdname, cmdarg);
			printf("command not found!\n");
			err = 0;
		}
	}
	free(cmdstr);
	printf(DBG_COLOR_CYAN "exam: '%s' -end%s\n", str, DBG_COLOR_END);
	return err;
}

int nvt_examsys_runcmd_bk(char *str)
{
	FLGPTN ptn = 0;

	vos_flag_set(examsys_flag_id, FLGEXAMSYS_POLLING);
	vos_flag_wait_interruptible(&ptn, examsys_flag_id, FLGEXAMSYS_POLLING | FLGEXAMSYS_IDLE, TWF_ORW | TWF_CLR);

	if ((ptn & FLGEXAMSYS_IDLE) == 0) {
		// check if waiting key
		vos_flag_set(examsys_flag_id, FLGEXAMSYS_ITEM_WAIT_POLLING);
		vos_flag_wait_interruptible(&ptn, examsys_flag_id, FLGEXAMSYS_ITEM_WAIT_POLLING | FLGEXAMSYS_ITEM_WAIT, TWF_ORW | TWF_CLR);

		if ((ptn & FLGEXAMSYS_ITEM_WAIT) == 0) {
			printf("command '%s' is running\n", buf);
		} else {
			strncpy(item_input, str, sizeof(item_input) - 3);
			//to be safe, because strncpy will not fill /0 when string out of range
			item_input[sizeof(item_input) - 4] = 0;
			strcat(item_input, "\r\n");
			vos_flag_set(examsys_flag_id, FLGEXAMSYS_ITEM_WAIT_DONE);
		}
		return 0;
	}

	strncpy(buf, str, sizeof(buf) - 1);
	vos_flag_set(examsys_flag_id, FLGEXAMSYS_RUN);
	return 0;
}

char *nvt_examsys_fgets(char *str, int n, FILE *stream)
{
	FLGPTN ptn;

	if (vos_task_get_handle() == m_examsys_thread) {
		vos_flag_set(examsys_flag_id, FLGEXAMSYS_ITEM_WAIT);
		vos_flag_wait_interruptible(&ptn, examsys_flag_id, FLGEXAMSYS_ITEM_WAIT_DONE, TWF_ORW | TWF_CLR);
		strncpy(str, item_input, n);
	} else {
		fgets(str, n, stream);
	}
	return item_input;
}

char nvt_examsys_getchar(void)
{
	char ch;
	FLGPTN ptn;

	if (vos_task_get_handle() == m_examsys_thread) {
		vos_flag_set(examsys_flag_id, FLGEXAMSYS_ITEM_WAIT);
		vos_flag_wait_interruptible(&ptn, examsys_flag_id, FLGEXAMSYS_ITEM_WAIT_DONE, TWF_ORW | TWF_CLR);
		ch = item_input[0];
	} else {
		ch = (char)getchar();
	}
	return ch;
}

static void nvt_examsys_tsk(void *p_param)
{
	FLGPTN ptn;
	//coverity[no_escape]
	while (1) {
		vos_flag_set(examsys_flag_id, FLGEXAMSYS_IDLE);
		vos_flag_wait_interruptible(&ptn, examsys_flag_id, FLGEXAMSYS_RUN, TWF_ORW | TWF_CLR);

		if (strlen(buf) > 0) {
			nvt_examsys_runcmd(buf);
			printf("$ ");
		}
	}
}

int nvt_examsys_init(void)
{
	m_examsys_thread = vos_task_create(nvt_examsys_tsk, NULL, "nvt_examsys_tsk", VK_TASK_LOWEST_PRIORITY-5, 16 * 1024);
	vos_flag_create(&examsys_flag_id, NULL, "examsys_flag_id");
	vos_flag_clr(examsys_flag_id, 0);
	vos_task_resume(m_examsys_thread);
	nvt_cmdsys_regsys('$', nvt_examsys_runcmd_bk);
	return 0;
}
