#include "sys_api.h"

#include <sys/resource.h>

#include "common_def.h"

namespace SysAPI
{
int GetPriority()
{
#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_LINUX) || defined(OS_ANDRIOD)
    errno = 0;
    int ret = getpriority(PRIO_PROCESS, 0);
    if (ret == -1 && errno != 0) {
        return 0;
    }
    return ret;
#else
    return 0;
#endif
}

int SetPriority(int priority)
{
#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_LINUX) || defined(OS_ANDRIOD)
    return setpriority(PRIO_PROCESS, 0, priority);
#endif
    return 0;
}

};  // namespace SysAPI