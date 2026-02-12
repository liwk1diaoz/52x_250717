#ifndef _VOS_INTERNAL_VERSION_H_
#define _VOS_INTERNAL_VERSION_H_

#include <kwrap/verinfo.h>

#define VOS_VER_MAJOR   1
#define VOS_VER_MINOR   00
#define VOS_VER_BUGFIX  011
#define VOS_VER_EXT     00

#define __str(_x) #_x
#define __xstr(_x) __str(_x)

#define VOS_VERSION __xstr(VOS_VER_MAJOR)"."__xstr(VOS_VER_MINOR)"."__xstr(VOS_VER_BUGFIX)"."__xstr(VOS_VER_EXT)

#endif /* _VOS_INTERNAL_VERSION_H_ */

