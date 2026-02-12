#ifndef __module_api_h_
#define __module_api_h_
#include "ctl_vpe_drv.h"


extern void nvt_ctl_vpe_install_cmd(void);
extern void nvt_ctl_vpe_uninstall_cmd(void);
#if defined(__LINUX)
int ctl_vpe_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
