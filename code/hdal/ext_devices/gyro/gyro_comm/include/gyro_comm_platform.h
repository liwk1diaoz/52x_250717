#ifndef __MODULE_PLATFORM_H_
#define __MODULE_PLATFORM_H__


#include <kwrap/mem.h>

#if defined(__LINUX)
#include <linux/string.h>
#else
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/nvt_type.h>
#include <kwrap/task.h>
#include <string.h>
#include <stdio.h>
#endif
#endif
