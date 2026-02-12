#ifndef __EIS_API_H
#define __EIS_API_H

#include <kwrap/type.h>

#if defined(__LINUX)
int eis_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
