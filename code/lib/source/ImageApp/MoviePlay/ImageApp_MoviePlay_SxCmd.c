#include "ImageApp/ImageApp_MoviePlay.h"
#include "iamovieplay_tsk.h"
#include <kwrap/cmdsys.h>
#include <kwrap/sxcmd.h>
#include <kwrap/debug.h>
#include <string.h>



static BOOL _ImageApp_MoviePlay_SxCmd_PlayOpen(unsigned char argc, char **argv)
{
	movieplay_open_main();
	return TRUE;
}

static BOOL _ImageApp_MoviePlay_SxCmd_PlayStart(unsigned char argc, char **argv)
{
    // Start Play
#if 0
    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_OPEN);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
#else
	ImageApp_MoviePlay_Start();
#endif
CHKPNT;
	return TRUE;
}

static BOOL _ImageApp_MoviePlay_SxCmd_PlayStop(unsigned char argc, char **argv)
{
#if 0
    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_STOP);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
#else
	movieplay_close_main();
#endif
CHKPNT;
	return TRUE;
}

static BOOL _ImageApp_MoviePlay_SxCmd_PlayPause(unsigned char argc, char **argv)
{
#if 0
    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_PAUSE);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
#else
	ImageApp_MoviePlay_FilePlay_Pause();
#endif
CHKPNT;
	return TRUE;
}

static BOOL _ImageApp_MoviePlay_SxCmd_PlayResume(unsigned char argc, char **argv)
{
#if 0
    iamovieplay_SetPlayState(IAMOVIEPLAY_STATE_RESUME);
    iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_NORMAL, IAMOVIEPLAY_DIRECT_FORWARD, 0);
#else
	ImageApp_MoviePlay_FilePlay_Resume();
#endif

CHKPNT;
	return TRUE;
}

static BOOL _ImageApp_MoviePlay_SxCmd_SetDebug(unsigned char argc, char **argv)
{
	UINT32 dbg_msg;

	sscanf_s(argv[0], "%d", &dbg_msg);
	
	iamovieplay_set_dbg_level(dbg_msg);

	return TRUE;
}

static SXCMD_BEGIN(iamovieplay_cmd_tbl, "iamovieplay_cmd_tbl")
SXCMD_ITEM("playopen", _ImageApp_MoviePlay_SxCmd_PlayOpen,   "open play")
SXCMD_ITEM("startplay", _ImageApp_MoviePlay_SxCmd_PlayStart, "Start play")
SXCMD_ITEM("stopplay", _ImageApp_MoviePlay_SxCmd_PlayStop,   "Stop play")
SXCMD_ITEM("pauseplay", _ImageApp_MoviePlay_SxCmd_PlayPause,  "Pause play")
SXCMD_ITEM("resumeplay", _ImageApp_MoviePlay_SxCmd_PlayResume,  "Resume play")
SXCMD_ITEM("set_dbg_level", _ImageApp_MoviePlay_SxCmd_SetDebug,  "Set debug level")
SXCMD_END()

static int iamovieplay_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(iamovieplay_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "playback (ImageApp_MoviePlay)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", iamovieplay_cmd_tbl[loop].p_name, iamovieplay_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(movieplay, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(iamovieplay_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		iamovieplay_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], iamovieplay_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = iamovieplay_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}

