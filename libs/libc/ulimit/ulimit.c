#include <errno.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <ulimit.h>

#define ULIMIT_TO_RLIMIT_FACTOR 512

long ulimit(int cmd, ...) {
    switch (cmd) {
        case UL_GETFSIZE: {
            struct rlimit lim;
            int ret = getrlimit(RLIMIT_FSIZE, &lim);
            if (ret) {
                return ret;
            }
            return lim.rlim_cur / ULIMIT_TO_RLIMIT_FACTOR;
        }
        case UL_SETFSIZE: {
            va_list args;
            va_start(args, cmd);
            long limit = va_arg(args, long);
            va_end(args);
            struct rlimit lim;
            int ret = getrlimit(RLIMIT_FSIZE, &lim);
            if (ret) {
                return ret;
            }
            lim.rlim_cur = limit * ULIMIT_TO_RLIMIT_FACTOR;
            return setrlimit(RLIMIT_FSIZE, &lim);
        }
        default:
            errno = EINVAL;
            return -1;
    }
}
